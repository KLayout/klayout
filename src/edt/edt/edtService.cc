
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/


#include "dbClipboard.h"
#include "dbPCellDeclaration.h"
#include "dbLibrary.h"
#include "edtPlugin.h"
#include "edtService.h"
#if defined(HAVE_QT)
#  include "edtEditorOptionsPages.h"
#  include "edtDialogs.h"
#endif
#include "layFinder.h"
#include "layLayoutView.h"
#include "laySnap.h"
#include "tlProgress.h"
#include "tlTimer.h"

namespace edt
{

// -------------------------------------------------------------
//  Convert buttons to an angle constraint

lay::angle_constraint_type 
ac_from_buttons (unsigned int buttons)
{
  if ((buttons & lay::ShiftButton) != 0) {
    if ((buttons & lay::ControlButton) != 0) {
      return lay::AC_Any;
    } else {
      return lay::AC_Ortho;
    }
  } else {
    if ((buttons & lay::ControlButton) != 0) {
      return lay::AC_Diagonal;
    } else {
      return lay::AC_Global;
    }
  }
}

// -------------------------------------------------------------

Service::Service (db::Manager *manager, lay::LayoutViewBase *view, db::ShapeIterator::flags_type flags)
  : lay::EditorServiceBase (view),
    db::Object (manager),
    mp_view (view),
    mp_transient_marker (0), 
    m_editing (false), m_immediate (false), 
    m_cell_inst_service (false),
    m_flags (flags),
    m_move_sel (false), m_moving (false),
    m_connect_ac (lay::AC_Any), m_move_ac (lay::AC_Any), m_alt_ac (lay::AC_Global),
    m_snap_to_objects (true),
    m_snap_objects_to_grid (true),
    m_top_level_sel (false), m_show_shapes_of_instances (true), m_max_shapes_of_instances (1000),
    m_hier_copy_mode (-1),
    m_indicate_secondary_selection (false),
    m_seq (0),
    m_highlights_selected (false),
    dm_selection_to_view (this, &edt::Service::do_selection_to_view)
{ 
  mp_view->geom_changed_event.add (this, &edt::Service::selection_to_view);
}

Service::Service (db::Manager *manager, lay::LayoutViewBase *view)
  : lay::EditorServiceBase (view),
    db::Object (manager),
    mp_view (view),
    mp_transient_marker (0), 
    m_editing (false), m_immediate (false), 
    m_cell_inst_service (true),
    m_flags (db::ShapeIterator::Nothing),
    m_move_sel (false), m_moving (false),
    m_connect_ac (lay::AC_Any), m_move_ac (lay::AC_Any), m_alt_ac (lay::AC_Global),
    m_snap_to_objects (true),
    m_snap_objects_to_grid (true),
    m_top_level_sel (false), m_show_shapes_of_instances (true), m_max_shapes_of_instances (1000),
    m_hier_copy_mode (-1),
    m_indicate_secondary_selection (false),
    m_seq (0),
    m_highlights_selected (false),
    dm_selection_to_view (this, &edt::Service::do_selection_to_view)
{ 
  mp_view->geom_changed_event.add (this, &edt::Service::selection_to_view);
}

Service::~Service ()
{
  for (std::vector<lay::ViewObject *>::iterator r = m_markers.begin (); r != m_markers.end (); ++r) {
    delete *r;
  }
  m_markers.clear ();

  for (std::vector<lay::ViewObject *>::iterator r = m_edit_markers.begin (); r != m_edit_markers.end (); ++r) {
    delete *r;
  }
  m_edit_markers.clear ();

  clear_transient_selection ();
}

lay::angle_constraint_type 
Service::connect_ac () const
{
  //  m_alt_ac (which is set from mouse buttons) can override the specified connect angle constraint
  return m_alt_ac != lay::AC_Global ? m_alt_ac : m_connect_ac;
}

lay::angle_constraint_type 
Service::move_ac () const
{
  //  m_alt_ac (which is set from mouse buttons) can override the specified move angle constraint
  return m_alt_ac != lay::AC_Global ? m_alt_ac : m_move_ac;
}

db::DPoint 
Service::snap (db::DPoint p) const
{
  //  snap according to the grid
  if (m_edit_grid == db::DVector ()) {
    p = lay::snap_xy (p, m_global_grid);
  } else if (m_edit_grid.x () < 1e-6) {
    ; //  nothing 
  } else {
    p = lay::snap_xy (p, m_edit_grid);
  }

  return p;
}

void
Service::update_vector_snapped_point (const db::DPoint &pt, db::DVector &vr, bool &result_set) const
{
  db::DVector v = snap (pt) - pt;

  if (! result_set || v.length () < vr.length ()) {
    result_set = true;
    vr = v;
  }
}

void
Service::update_vector_snapped_marker (const lay::ShapeMarker *sm, const db::DTrans &trans, db::DVector &vr, bool &result_set, size_t &count) const
{
  const db::Shape &shape = sm->shape ();
  db::CplxTrans tr = db::DCplxTrans (trans) * db::DCplxTrans (-sm->trans ().disp ()) * sm->trans ();

  if (shape.is_text ()) {

    update_vector_snapped_point (tr * shape.bbox ().center (), vr, result_set);
    --count;

  } else if (shape.is_point ()) {

    update_vector_snapped_point (tr * shape.point (), vr, result_set);
    --count;

  } else if (shape.is_edge ()) {

    update_vector_snapped_point (tr * shape.edge ().p1 (), vr, result_set);
    --count;
    if (count > 0) {
      update_vector_snapped_point (tr * shape.edge ().p2 (), vr, result_set);
      --count;
    }

  } else if (shape.is_path ()) {

    for (auto pt = shape.begin_point (); pt != shape.end_point () && count > 0; ++pt) {
      update_vector_snapped_point (tr * *pt, vr, result_set);
      --count;
    }

  } else if (shape.is_box ()) {

    db::Box box = shape.bbox ();
    for (unsigned int c = 0; c < 4 && count > 0; ++c) {
      db::Point pt = db::Point ((c & 1) != 0 ? box.left () : box.right (), (c & 2) != 0 ? box.bottom () : box.top ());
      update_vector_snapped_point (tr * pt, vr, result_set);
      --count;
    }

  } else if (shape.is_polygon ()) {

    for (auto pt = shape.begin_hull (); pt != shape.end_hull () && count > 0; ++pt) {
      update_vector_snapped_point (tr * *pt, vr, result_set);
      --count;
    }

    for (unsigned int h = 0; h < shape.holes () && count > 0; ++h) {
      for (auto pt = shape.begin_hole (h); pt != shape.end_hole (h) && count > 0; ++pt) {
        update_vector_snapped_point (tr * *pt, vr, result_set);
        --count;
      }
    }

  }
}

void
Service::update_vector_snapped_marker (const lay::InstanceMarker *im, const db::DTrans &trans, db::DVector &vr, bool &result_set, size_t &count) const
{
  const db::Instance &instance = im->instance ();
  db::CplxTrans tr = db::DCplxTrans (trans) * db::DCplxTrans (-im->trans ().disp ()) * im->trans ();

  update_vector_snapped_point (tr * (instance.complex_trans () * db::Point ()), vr, result_set);
  --count;
}

db::DVector
Service::snap_marker_to_grid (const db::DVector &v, bool &snapped) const
{
  if (! m_snap_objects_to_grid) {
    return v;
  }

  snapped = false;
  db::DVector vr;

  //  max. 10000 checks
  size_t count = 10000;

  db::DVector snapped_to (1.0, 1.0);
  db::DVector vv = lay::snap_angle (v, move_ac (), &snapped_to);

  db::DTrans tt = db::DTrans (vv);

  for (auto m = m_markers.begin (); m != m_markers.end () && count > 0; ++m) {

    const lay::ShapeMarker *sm = dynamic_cast<const lay::ShapeMarker *> (*m);
    const lay::InstanceMarker *im = dynamic_cast<const lay::InstanceMarker *> (*m);
    if (sm) {
      update_vector_snapped_marker (sm, tt, vr, snapped, count);
    } else if (im) {
      update_vector_snapped_marker (im, tt, vr, snapped, count);
    }

  }

  if (snapped) {
    vr += vv;
    return db::DVector (vr.x () * snapped_to.x (), vr.y () * snapped_to.y ());
  } else {
    return db::DVector ();
  }
}

db::DVector
Service::snap (db::DVector v) const
{
  //  snap according to the grid
  if (m_edit_grid == db::DVector ()) {
    v = lay::snap_xy (db::DPoint () + v, m_global_grid) - db::DPoint ();
  } else if (m_edit_grid.x () < 1e-6) {
    ; //  nothing
  } else {
    v = lay::snap_xy (db::DPoint () + v, m_edit_grid) - db::DPoint ();
  }

  return v;
}

db::DVector
Service::snap (const db::DVector &v, bool connect) const
{
  return snap (lay::snap_angle (v, connect ? connect_ac () : move_ac ()));
}

db::DPoint
Service::snap (const db::DPoint &p, const db::DPoint &plast, bool connect) const
{
  db::DPoint ps = plast + lay::snap_angle (db::DVector (p - plast), connect ? connect_ac () : move_ac ());
  return snap (ps);
}

const int sr_pixels = 8; // TODO: make variable

lay::PointSnapToObjectResult
Service::snap2_details (const db::DPoint &p) const
{
  double snap_range = ui ()->mouse_event_trans ().inverted ().ctrans (sr_pixels);
  return lay::obj_snap (m_snap_to_objects ? view () : 0, p, m_edit_grid == db::DVector () ? m_global_grid : m_edit_grid, snap_range);
}

db::DPoint
Service::snap2 (const db::DPoint &p) const
{
  return snap2_details (p).snapped_point;
}

db::DPoint 
Service::snap2 (const db::DPoint &p, const db::DPoint &plast, bool connect) const
{
  double snap_range = ui ()->mouse_event_trans ().inverted ().ctrans (sr_pixels);
  return lay::obj_snap (m_snap_to_objects ? view () : 0, plast, p, m_edit_grid == db::DVector () ? m_global_grid : m_edit_grid, connect ? connect_ac () : move_ac (), snap_range).snapped_point;
}

void
Service::service_configuration_changed ()
{
  //  The base class implementation does nothing
}

bool
Service::configure (const std::string &name, const std::string &value)
{
  edt::EditGridConverter egc;
  edt::ACConverter acc;

  if (name == cfg_edit_global_grid) {

    egc.from_string (value, m_global_grid);
    service_configuration_changed ();

  } else if (name == cfg_edit_show_shapes_of_instances) {

    tl::from_string (value, m_show_shapes_of_instances);
    service_configuration_changed ();

  } else if (name == cfg_edit_max_shapes_of_instances) {

    tl::from_string (value, m_max_shapes_of_instances);
    service_configuration_changed ();

  } else if (name == cfg_edit_grid) {

    egc.from_string (value, m_edit_grid);
    service_configuration_changed ();

    return true;  //  taken

  } else if (name == cfg_edit_snap_to_objects) {

    tl::from_string (value, m_snap_to_objects);
    service_configuration_changed ();

    return true;  //  taken

  } else if (name == cfg_edit_snap_objects_to_grid) {

    tl::from_string (value, m_snap_objects_to_grid);
    service_configuration_changed ();

    return true;  //  taken

  } else if (name == cfg_edit_move_angle_mode) {

    acc.from_string (value, m_move_ac);
    service_configuration_changed ();

    return true;  //  taken

  } else if (name == cfg_edit_connect_angle_mode) {

    acc.from_string (value, m_connect_ac);
    service_configuration_changed ();

    return true;  //  taken

  } else if (name == cfg_edit_top_level_selection) {

    tl::from_string (value, m_top_level_sel);
    service_configuration_changed ();

  } else if (name == cfg_edit_hier_copy_mode) {

    tl::from_string (value, m_hier_copy_mode);
    service_configuration_changed ();

  } else {
    lay::EditorServiceBase::configure (name, value);
  }

  return false;  //  not taken
}

void 
Service::clear_highlights ()
{
  m_highlights_selected = true;
  m_selected_highlights.clear ();
  apply_highlights ();
}

void 
Service::restore_highlights ()
{
  m_highlights_selected = false;
  m_selected_highlights.clear ();
  apply_highlights ();
}

void
Service::highlight (const std::vector<size_t> &n)
{
  m_highlights_selected = true;
  m_selected_highlights = std::set<size_t> (n.begin (), n.end ());
  apply_highlights ();
}

void
Service::apply_highlights ()
{
  for (std::vector<lay::ViewObject *>::iterator r = m_markers.begin (); r != m_markers.end (); ++r) {
    (*r)->visible (! m_highlights_selected || m_selected_highlights.find (r - m_markers.begin ()) != m_selected_highlights.end ());
  }
}

void
Service::cut ()
{
  if (has_selection () && view ()->is_editable ()) {
    //  copy & delete the selected objects
    copy_selected ();
    del_selected ();
  }
}

void 
Service::copy ()
{
  if (view ()->is_editable ()) {
    //  copy the selected objects
    copy_selected ();
  }
}

void
Service::copy_selected ()
{
#if defined(HAVE_QT)
  edt::CopyModeDialog mode_dialog (lay::widget_from_view (view ()));

  bool need_to_ask_for_copy_mode = false;
  unsigned int inst_mode = 0;

  if (m_hier_copy_mode < 0) {
    for (objects::const_iterator r = m_selection.begin (); r != m_selection.end () && ! need_to_ask_for_copy_mode; ++r) {
      if (r->is_cell_inst ()) {
        const db::Cell &cell = view ()->cellview (r->cv_index ())->layout ().cell (r->back ().inst_ptr.cell_index ());
        if (! cell.is_proxy ()) {
          need_to_ask_for_copy_mode = true;
        }
      }
    }
  } else {
    inst_mode = (unsigned int) m_hier_copy_mode;
  }

  bool dont_ask_again = false;

  if (! need_to_ask_for_copy_mode || mode_dialog.exec_dialog (inst_mode, dont_ask_again)) {

    //  store the given value "forever"
    if (dont_ask_again) {
      dispatcher ()->config_set (cfg_edit_hier_copy_mode, tl::to_string (inst_mode));
      dispatcher ()->config_end ();
    }

    copy_selected (inst_mode);

  }
#else

  unsigned int inst_mode = 0;
  if (m_hier_copy_mode >= 0) {
    inst_mode = int (m_hier_copy_mode);
  }

  copy_selected (inst_mode);

#endif
}

void
Service::copy_selected (unsigned int inst_mode)
{
  //  create one ClipboardData object per cv_index because, this one assumes that there is
  //  only one source layout object.
  std::set <unsigned int> cv_indices;
  for (objects::const_iterator r = m_selection.begin (); r != m_selection.end (); ++r) {
    cv_indices.insert (r->cv_index ());
  }

  for (std::set <unsigned int>::const_iterator cvi = cv_indices.begin (); cvi != cv_indices.end (); ++cvi) {

    db::ClipboardValue<edt::ClipboardData> *cd = new db::ClipboardValue<edt::ClipboardData> ();

    //  add the selected objects to the clipboard data objects.
    const lay::CellView &cv = view ()->cellview (*cvi);
    for (objects::const_iterator r = m_selection.begin (); r != m_selection.end (); ++r) {
      if (r->cv_index () == *cvi) {
        if (! r->is_cell_inst ()) {
          cd->get ().add (cv->layout (), r->layer (), r->shape (), cv.context_trans () * r->trans ());
        } else {
          cd->get ().add (cv->layout (), r->back ().inst_ptr, inst_mode, cv.context_trans () * r->trans ());
        }
      }
    }

    db::Clipboard::instance () += cd;

  }
}

bool  
Service::begin_move (lay::Editable::MoveMode mode, const db::DPoint &p, lay::angle_constraint_type /*ac*/)
{
  if (view ()->is_editable () && mode == lay::Editable::Selected) {

    //  flush any pending updates of the markers
    dm_selection_to_view.execute ();

    m_move_start = p;
    m_move_trans = db::DTrans ();
    m_move_sel = true; // TODO: there is no "false". Remove this.
    m_moving = true;

    for (std::vector<lay::ViewObject *>::iterator r = m_markers.begin (); r != m_markers.end (); ++r) {

      (*r)->thaw ();

      //  Show the inner structure of the instances
      lay::InstanceMarker *inst_marker = dynamic_cast<lay::InstanceMarker *> (*r);
      if (inst_marker) {
        inst_marker->set_draw_outline (! m_show_shapes_of_instances);
        inst_marker->set_max_shapes (m_show_shapes_of_instances ? m_max_shapes_of_instances : 0);
      }

    }

  }

  return false;
}

void  
Service::move (const db::DPoint &pu, lay::angle_constraint_type ac)
{
  m_alt_ac = ac;
  if (view ()->is_editable () && m_moving) {
    db::DPoint ref = snap (m_move_start);
    bool snapped = false;
    db::DPoint p = ref + snap_marker_to_grid (pu - m_move_start, snapped);
    if (! snapped) {
      p = ref + snap (pu - m_move_start, false /*move*/);
    }
    move_markers (db::DTrans (p - db::DPoint ()) * db::DTrans (m_move_trans.fp_trans ()) * db::DTrans (db::DPoint () - ref));
  }
  m_alt_ac = lay::AC_Global;
}

void  
Service::move_transform (const db::DPoint &pu, db::DFTrans tr, lay::angle_constraint_type ac)
{
  m_alt_ac = ac;
  if (view ()->is_editable () && m_moving) {
    db::DPoint ref = snap (m_move_start);
    bool snapped = false;
    db::DPoint p = ref + snap_marker_to_grid (pu - m_move_start, snapped);
    if (! snapped) {
      p = ref + snap (pu - m_move_start, false /*move*/);
    }
    move_markers (db::DTrans (p - db::DPoint ()) * db::DTrans (tr * m_move_trans.fp_trans ()) * db::DTrans (db::DPoint () - ref));
  }
  m_alt_ac = lay::AC_Global;
}

void  
Service::end_move (const db::DPoint & /*p*/, lay::angle_constraint_type ac)
{
  m_alt_ac = ac;
  if (view ()->is_editable () && m_moving) {
    transform (db::DCplxTrans (m_move_trans));
    move_cancel (); // formally this functionality fits here
    //  accept changes to guiding shapes
    handle_guiding_shape_changes ();
  }
  m_alt_ac = lay::AC_Global;
}

db::DBox  
Service::selection_bbox ()
{
  //  build the transformation variants cache 
  //  TODO: this is done multiple times - once for each service!
  TransformationVariants tv (view ());
  const db::DCplxTrans &vp = view ()->viewport ().trans ();

  lay::TextInfo text_info (view ());

  db::DBox box;
  for (objects::const_iterator r = m_selection.begin (); r != m_selection.end (); ++r) {

    const lay::CellView &cv = view ()->cellview (r->cv_index ());
    const db::Layout &layout = cv->layout ();

    db::CplxTrans ctx_trans = db::CplxTrans (layout.dbu ()) * cv.context_trans () * r->trans ();

    db::box_convert<db::CellInst> bc (layout);
    if (! r->is_cell_inst ()) {

      const std::vector<db::DCplxTrans> *tv_list = tv.per_cv_and_layer (r->cv_index (), r->layer ());
      if (tv_list != 0) {
        for (std::vector<db::DCplxTrans>::const_iterator t = tv_list->begin (); t != tv_list->end (); ++t) {
          if (r->shape ().is_text ()) {
            db::Text text;
            r->shape ().text (text);
            box += *t * text_info.bbox (ctx_trans * text, vp * *t);
          } else {
            box += *t * (ctx_trans * r->shape ().bbox ());
          }
        }
      }

    } else {

      const std::vector<db::DCplxTrans> *tv_list = tv.per_cv (r->cv_index ());
      if (tv_list != 0) {
        for (std::vector<db::DCplxTrans>::const_iterator t = tv_list->begin (); t != tv_list->end (); ++t) {
          box += *t * (ctx_trans * r->back ().bbox (bc));
        }
      }

    }

  }

  return box;
}

void
Service::set_edit_marker (lay::ViewObject *edit_marker)
{
  for (std::vector<lay::ViewObject *>::iterator r = m_edit_markers.begin (); r != m_edit_markers.end (); ++r) {
    delete *r;
  }
  m_edit_markers.clear ();
  add_edit_marker (edit_marker);
}

void
Service::add_edit_marker (lay::ViewObject *edit_marker)
{
  if (edit_marker) {
    m_edit_markers.push_back (edit_marker);
  }
}

lay::ViewObject *
Service::edit_marker ()
{
  if (! m_edit_markers.empty ()) {
    return m_edit_markers.front ();
  } else {
    return 0;
  }
}

void 
Service::transform (const db::DCplxTrans &trans, const std::vector<db::DCplxTrans> *p_trv)
{
  //  ignore this function in non-editable mode
  if (! view ()->is_editable ()) {
    return;
  }

  //  HINT: sorting the selected shapes/instances ensures that a shape/instance is not moved twice.
  //  This may happen due to per-instance selection of lower-level shapes.
  
  size_t n;

  //  build a list of object references corresponding to the p_trv vector 
  std::vector <objects::iterator> obj_ptrs;
  obj_ptrs.reserve (m_selection.size ());
  n = 0;
  for (objects::iterator r = m_selection.begin (); r != m_selection.end (); ++r, ++n) {
    obj_ptrs.push_back (r);
  }

  //  build the transformation variants cache
  TransformationVariants tv (view ());

  //  1.) first transform all shapes

  //  sort the selected objects (the shapes) by the cell they are in
  //  The key is a triple: cell_index, cv_index, layer
  std::map <std::pair <db::cell_index_type, std::pair <unsigned int, unsigned int> >, std::vector <size_t> > shapes_by_cell;
  n = 0;
  for (objects::iterator r = m_selection.begin (); r != m_selection.end (); ++r, ++n) {
    if (! r->is_cell_inst ()) {
      shapes_by_cell.insert (std::make_pair (std::make_pair (r->cell_index (), std::make_pair (r->cv_index (), r->layer ())), std::vector <size_t> ())).first->second.push_back (n);
    }
  }

  for (std::map <std::pair <db::cell_index_type, std::pair <unsigned int, unsigned int> >, std::vector <size_t> >::iterator sbc = shapes_by_cell.begin (); sbc != shapes_by_cell.end (); ++sbc) {

    const lay::CellView &cv = view ()->cellview (sbc->first.second.first);
    if (cv.is_valid ()) {

      const std::vector<db::DCplxTrans> *tv_list = tv.per_cv_and_layer (sbc->first.second.first, sbc->first.second.second);
      if (tv_list != 0) {

        db::CplxTrans tt = (*tv_list) [0] * db::CplxTrans (cv->layout ().dbu ()) * cv.context_trans ();
        db::DCplxTrans mt_mu (tt.inverted () * trans * tt);

        db::Shapes &shapes = cv->layout ().cell (sbc->first.first).shapes (sbc->first.second.second);

        std::map <db::Shape, db::Shape> new_shapes;

        for (std::vector <size_t>::iterator si = sbc->second.begin (); si != sbc->second.end (); ++si) {

          objects::iterator s = obj_ptrs [*si];

          //  mt = transformation in DBU units
          db::ICplxTrans mt;
          db::CplxTrans t (s->trans ());
          if (p_trv != 0 && *si < p_trv->size ()) {
            db::DCplxTrans t_mu (tt.inverted () * (*p_trv) [*si] * tt);
            mt = db::ICplxTrans (t.inverted () * t_mu * t);
          } else {
            mt = db::ICplxTrans (t.inverted () * mt_mu * t);
          }

          std::map <db::Shape, db::Shape>::iterator ns = new_shapes.find (s->shape ());

          if (ns == new_shapes.end ()) {
            new_shapes.insert (std::make_pair (s->shape (), shapes.transform (s->shape (), mt)));
          } else {
            ns->second = shapes.transform (ns->second, mt);
          }

        }

        for (std::vector <size_t>::iterator si = sbc->second.begin (); si != sbc->second.end (); ++si) {

          objects::iterator &s = obj_ptrs [*si];

          lay::ObjectInstPath new_path (*s);
          new_path.set_shape (new_shapes.find (s->shape ())->second);

          //  modify the selection
          m_selection.erase (s);
          s = m_selection.insert (new_path).first;

        }

      }

    }

  }
  
  //  2.) then transform all instances.
  
  //  sort the selected objects (the instances) by the cell they are in
  //  The key is a pair: cell_index, cv_index
  std::map <std::pair <db::cell_index_type, unsigned int>, std::vector <size_t> > insts_by_cell;
  n = 0;
  for (objects::iterator r = m_selection.begin (); r != m_selection.end (); ++r, ++n) {
    if (r->is_cell_inst ()) {
      insts_by_cell.insert (std::make_pair (std::make_pair (r->cell_index (), r->cv_index ()), std::vector <size_t> ())).first->second.push_back (n);
    }
  }

  for (std::map <std::pair <db::cell_index_type, unsigned int>, std::vector <size_t> >::iterator ibc = insts_by_cell.begin (); ibc != insts_by_cell.end (); ++ibc) {

    const lay::CellView &cv = view ()->cellview (ibc->first.second);
    if (cv.is_valid ()) {

      const std::vector<db::DCplxTrans> *tv_list = tv.per_cv (ibc->first.second);
      if (tv_list != 0) {

        db::CplxTrans tt = (*tv_list) [0] * db::CplxTrans (cv->layout ().dbu ()) * cv.context_trans ();
        db::ICplxTrans mt_mu (tt.inverted () * trans * tt);

        std::map <db::Instance, db::Instance> new_insts;

        db::Cell &cell = cv->layout ().cell (ibc->first.first);

        for (std::vector <size_t>::iterator ii = ibc->second.begin (); ii != ibc->second.end (); ++ii) {

          objects::iterator i = obj_ptrs [*ii];

          //  mt = transformation in DBU units
          db::ICplxTrans mt;
          db::ICplxTrans t (i->trans ());
          if (p_trv != 0 && *ii < p_trv->size ()) {
            db::ICplxTrans t_mu (tt.inverted () * (*p_trv) [*ii] * tt);
            mt = t.inverted () * t_mu * t;
          } else {
            mt = t.inverted () * mt_mu * t;
          }

          db::Instance inst_ptr = i->back ().inst_ptr;

          std::map <db::Instance, db::Instance>::iterator ni = new_insts.find (inst_ptr);

          if (ni == new_insts.end ()) {
            new_insts.insert (std::make_pair (inst_ptr, cell.transform (inst_ptr, mt)));
          } else {
            ni->second = cell.transform (inst_ptr, mt);
          }

        }

        for (std::vector <size_t>::iterator ii = ibc->second.begin (); ii != ibc->second.end (); ++ii) {

          objects::iterator &i = obj_ptrs [*ii];

          lay::ObjectInstPath new_path (*i);
          new_path.back ().inst_ptr = new_insts.find (i->back ().inst_ptr)->second;

          //  modify the selection
          m_selection.erase (i);
          i = m_selection.insert (new_path).first;

        }

      }

    }

  }

  handle_guiding_shape_changes ();
  selection_to_view ();
}

void   
Service::move_cancel ()
{
  if (m_move_trans != db::DTrans () && m_moving) {

    for (std::vector<lay::ViewObject *>::iterator r = m_markers.begin (); r != m_markers.end (); ++r) {
      (*r)->freeze ();
    }

    m_move_trans = db::DTrans ();
    m_move_start = db::DPoint ();

    //  reset to unmoved or clear selection and do not do anything 
    if (m_move_sel) {
      selection_to_view ();
    } else {
      clear_selection ();
    }

    m_moving = false;

  }
}

bool   
Service::mouse_move_event (const db::DPoint &p, unsigned int buttons, bool prio)
{
  if (view ()->is_editable () && prio) {

    if (m_editing || m_immediate) {

      m_alt_ac = ac_from_buttons (buttons);

      if (! m_editing) {
        //  in this mode, ignore exceptions here since it is rather annoying to have messages popping
        //  up then.
        try {
          begin_edit (p);
        } catch (...) {
          set_edit_marker (0);
        }
      }
      if (m_editing) {
        do_mouse_move (p);
      } else {
        do_mouse_move_inactive (p);
      }

      m_alt_ac = lay::AC_Global;

    } else if (prio) {
      do_mouse_move_inactive (p);
    }

  }

  return false;  // not taken to allow the mouse tracker to receive events as well
}

bool   
Service::mouse_press_event (const db::DPoint &p, unsigned int buttons, bool prio)
{
  if (view ()->is_editable () && prio) {
    
    if ((buttons & lay::LeftButton) != 0) {

      m_alt_ac = ac_from_buttons (buttons);

      if (! m_editing) {

        view ()->cancel ();  //  cancel any pending edit operations and clear the selection
        set_edit_marker (0);
        begin_edit (p);

      } else {
        if (do_mouse_click (p)) {
          m_editing = false;
          set_edit_marker (0);
          do_finish_edit ();
        }
      }

      m_alt_ac = lay::AC_Global;

      return true;

    }

  } 

  return false;
}

bool   
Service::mouse_double_click_event (const db::DPoint & /*p*/, unsigned int buttons, bool prio)
{
  if (m_editing && prio && (buttons & lay::LeftButton) != 0) {
    m_alt_ac = ac_from_buttons (buttons);
    do_finish_edit ();
    m_editing = false;
    set_edit_marker (0);
    m_alt_ac = lay::AC_Global;
    return true;
  } else {
    return false;
  }
}

bool   
Service::mouse_click_event (const db::DPoint &p, unsigned int buttons, bool prio)
{
  if (view ()->is_editable () && prio && (buttons & lay::RightButton) != 0 && m_editing) {
    m_alt_ac = ac_from_buttons (buttons);
    do_mouse_transform (p, db::DFTrans (db::DFTrans::r90));
    m_alt_ac = lay::AC_Global;
    return true;
  } else {
    return mouse_press_event (p, buttons, prio);
  }
}

bool
Service::key_event (unsigned int key, unsigned int buttons)
{
  if (view ()->is_editable () && m_editing && buttons == 0 && key == lay::KeyBackspace) {
    do_delete ();
    return true;
  } else {
    return false;
  }
}

void
Service::activated ()
{
  if (view ()->is_editable ()) {

    view ()->cancel ();  //  cancel any pending edit operations and clear the selection
    set_edit_marker (0);

    m_immediate = do_activated ();
    m_editing = false;

  }
}

void   
Service::deactivated ()
{
  lay::EditorServiceBase::deactivated ();

  edit_cancel ();

  m_immediate = false;
}

void   
Service::edit_cancel ()
{
  move_cancel ();
  if (m_editing) {
    do_cancel_edit ();
    m_editing = false;
    set_edit_marker (0);
  }
}

void 
Service::del ()
{
  if (has_selection () && view ()->is_editable ()) {
    //  delete the selected objects
    del_selected ();
  }
}

void
Service::del_selected ()
{
  std::set<db::Layout *> needs_cleanup;

  //  delete all shapes and instances.
  for (objects::const_iterator r = m_selection.begin (); r != m_selection.end (); ++r) {
    const lay::CellView &cv = view ()->cellview (r->cv_index ());
    if (cv.is_valid ()) {
      db::Cell &cell = cv->layout ().cell (r->cell_index ());
      if (! r->is_cell_inst ()) {
        if (r->layer () != cv->layout ().guiding_shape_layer () && cell.shapes (r->layer ()).is_valid (r->shape ())) {
          cell.shapes (r->layer ()).erase_shape (r->shape ());
        }
      } else {
        if (cell.is_valid (r->back ().inst_ptr)) {
          if (cv->layout ().cell (r->back ().inst_ptr.cell_index ()).is_proxy ()) {
            needs_cleanup.insert (& cv->layout ());
          }
          cell.erase (r->back ().inst_ptr);
        }
      }
    }
  }

  //  clean up the layouts that need to do so.
  for (std::set<db::Layout *>::const_iterator l = needs_cleanup.begin (); l != needs_cleanup.end (); ++l) {
    (*l)->cleanup ();
  }
}

bool
Service::has_selection ()
{
  return ! m_selection.empty ();
}

size_t
Service::selection_size ()
{
  return m_selection.size ();
}

bool
Service::has_transient_selection ()
{
  return ! m_transient_selection.empty ();
}

double
Service::catch_distance ()
{
  return double (view ()->search_range ()) / ui ()->mouse_event_trans ().mag ();
}

double
Service::catch_distance_box ()
{
  return double (view ()->search_range_box ()) / ui ()->mouse_event_trans ().mag ();
}

double
Service::click_proximity (const db::DPoint &pos, lay::Editable::SelectionMode mode)
{
  //  compute search box
  double l = catch_distance ();
  db::DBox search_box = db::DBox (pos, pos).enlarged (db::DVector (l, l));

  //  for single-point selections either exclude the current selection or the
  //  accumulated previous selection from the search.
  const objects *exclude = 0;
  if (mode == lay::Editable::Replace) {
    exclude = &m_previous_selection;
  } else if (mode == lay::Editable::Add) {
    exclude = &m_selection;
  } else if (mode == lay::Editable::Reset) {
    //  TODO: the finder should favor the current selection in this case.
  }

  if (m_cell_inst_service) {

    lay::InstFinder finder (true, view ()->is_editable () && m_top_level_sel, view ()->is_editable () /*full arrays in editable mode*/, true /*enclose_inst*/, exclude, true /*visible layers*/);

    //  go through all cell views
    std::set< std::pair<db::DCplxTrans, int> > variants = view ()->cv_transform_variants ();
    for (std::set< std::pair<db::DCplxTrans, int> >::const_iterator v = variants.begin (); v != variants.end (); ++v) {
      finder.find (view (), v->second, v->first, search_box);
    }

    //  Return the finder's proximity value
    if (finder.begin () != finder.end ()) {
      return finder.proximity ();
    } else {
      return lay::Editable::click_proximity (pos, mode); 
    }

  } else {

    lay::ShapeFinder finder (true, view ()->is_editable () && m_top_level_sel, m_flags, exclude);

    //  go through all visible layers of all cellviews
    finder.find (view (), search_box);

    //  Return the finder's proximity value
    if (finder.begin () != finder.end ()) {
      return finder.proximity ();
    } else {
      return lay::Editable::click_proximity (pos, mode); 
    }

  }
}

bool
Service::transient_select (const db::DPoint &pos)
{
  clear_transient_selection ();

  //  if in move mode (which also receives transient_select requests) the move will take the selection,
  //  hence don't do a transient selection if there is one.
  if (view ()->has_selection () && view ()->is_move_mode ()) {
    return false;
  }

  //  compute search box
  double l = catch_distance ();
  db::DBox search_box = db::DBox (pos, pos).enlarged (db::DVector (l, l));

  if (m_cell_inst_service) {

    lay::InstFinder finder (true, view ()->is_editable () && m_top_level_sel, view ()->is_editable () /*full arrays in editable mode*/, true /*enclose instances*/, &m_previous_selection, true /*visible layers only*/);

    //  go through all transform variants
    std::set< std::pair<db::DCplxTrans, int> > variants = view ()->cv_transform_variants ();
    for (std::set< std::pair<db::DCplxTrans, int> >::const_iterator v = variants.begin (); v != variants.end (); ++v) {
      finder.find (view (), v->second, v->first, search_box);
    }

    //  collect the founds from the finder
    lay::InstFinder::iterator r = finder.begin (); 
    if (r != finder.end ()) {

      m_transient_selection.insert (*r);

      const lay::CellView &cv = view ()->cellview (r->cv_index ());

      //  compute the global transformation including movement, context and explicit transformation
      double dbu = cv->layout ().dbu ();
      db::ICplxTrans gt = db::VCplxTrans (1.0 / dbu) * db::DCplxTrans (m_move_trans) * db::CplxTrans (dbu) * cv.context_trans () * r->trans ();

      tl_assert (r->is_cell_inst () == m_cell_inst_service);

      db::Instance inst = r->back ().inst_ptr;

      std::vector<db::DCplxTrans> tv = mp_view->cv_transform_variants (r->cv_index ());
      if (view ()->is_editable ()) {

#if 0
        //  to show the content of the cell when the instance is selected:
        lay::InstanceMarker *marker = new lay::InstanceMarker (view (), r->cv_index (), ! show_shapes_of_instances (), show_shapes_of_instances () ? max_shapes_of_instances () : 0);
#else
        lay::InstanceMarker *marker = new lay::InstanceMarker (view (), r->cv_index ());
#endif
        marker->set_vertex_shape (lay::ViewOp::Cross);
        marker->set_vertex_size (9 /*cross vertex size*/);
        marker->set (inst, gt, tv);
        marker->set_line_width (1);
        marker->set_halo (0);
        marker->set_text_enabled (false);

        mp_transient_marker = marker;

      } else {

        //  In viewer mode, individual instances of arrays can be selected. Since that is not supported by
        //  InstanceMarker, we just indicate the individual instance's bounding box.
        lay::Marker *marker = new lay::Marker (view (), r->cv_index ());
        db::box_convert<db::CellInst> bc (cv->layout ());
        marker->set (bc (r->back ().inst_ptr.cell_inst ().object ()), gt * r->back ().inst_ptr.cell_inst ().complex_trans (*r->back ().array_inst), tv);
        marker->set_vertex_size (0);
        marker->set_line_width (1);
        marker->set_halo (0);

        mp_transient_marker = marker;

      }

      if (! editables ()->has_selection ()) {
        display_status (true);
      }

      return true;

    } else {
      return false;
    }

  } else {

    lay::ShapeFinder finder (true, view ()->is_editable () && m_top_level_sel, m_flags, &m_previous_selection);

    //  go through all visible layers of all cellviews
    finder.find (view (), search_box);

    //  collect the founds from the finder
    lay::ShapeFinder::iterator r = finder.begin ();
    if (r != finder.end ()) {

      m_transient_selection.insert (*r);

      const lay::CellView &cv = view ()->cellview (r->cv_index ());

      //  compute the global transformation including movement, context and explicit transformation
      double dbu = cv->layout ().dbu ();
      db::ICplxTrans gt = db::VCplxTrans (1.0 / dbu) * db::DCplxTrans (m_move_trans) * db::CplxTrans (dbu) * cv.context_trans () * r->trans ();

      tl_assert (r->is_cell_inst () == m_cell_inst_service);

      lay::ShapeMarker *marker = new lay::ShapeMarker (view (), r->cv_index ());
      marker->set (r->shape (), gt, mp_view->cv_transform_variants (r->cv_index (), r->layer ()));

      bool is_point = false;
      if (r->shape ().is_edge () || r->shape ().is_box ()) {
        is_point = r->shape ().bbox ().is_point ();
      } else if (r->shape ().is_point ()) {
        is_point = true;
      }

      if (is_point) {
        marker->set_vertex_shape (lay::ViewOp::Cross);
        marker->set_vertex_size (9 /*cross vertex size*/);
      } else {
        marker->set_vertex_size (0);
      }
      marker->set_line_width (1);
      marker->set_halo (0);

      mp_transient_marker = marker;

      if (! editables ()->has_selection ()) {
        display_status (true);
      }

      return true;

    } else {
      return false;
    }

  }

}

static std::string path_to_string (const db::Layout &layout, const lay::ObjectInstPath &p)
{
  std::string r;

  lay::ObjectInstPath::iterator b = p.begin ();
  lay::ObjectInstPath::iterator e = p.end ();

  if (b != e && p.is_cell_inst ()) {
    --e;
  }

  r += "\\(";  //  group separator for shortening
  if (layout.is_valid_cell_index (p.topcell ())) {
    r += layout.cell_name (p.topcell ());
  } else {
    r += "?";
  }
  r += "\\)";

  while (b != e) {
    r += "\\(";  //  group separator for shortening
    r += "/";
    if (layout.is_valid_cell_index (b->inst_ptr.cell_index ())) {
      r += layout.cell_name (b->inst_ptr.cell_index ());
    } else {
      r += "?";
    }
    r += "\\)";
    ++b;
  }

  r += tl::sprintf ("@%d", p.cv_index () + 1);

  return r;
}

void 
Service::display_status (bool transient)
{
  const objects *selection = transient ? &m_transient_selection : &m_selection;

  if (selection->size () == 1) {

    objects::const_iterator r = selection->begin (); 
    const db::Layout &layout = view ()->cellview (r->cv_index ())->layout ();

    if (m_cell_inst_service) {

      std::string msg;
      if (! transient) {
        msg = tl::to_string (tr ("selected: "));
      }

      db::Instance inst = r->back ().inst_ptr;

      db::Vector a, b;
      unsigned long amax = 0, bmax = 0;
      if (! inst.is_regular_array (a, b, amax, bmax)) {
        msg += tl::sprintf (tl::to_string (tr ("instance(\"%s\" %s)")), layout.display_name (inst.cell_index ()), inst.complex_trans ().to_string ());
      } else {
        msg += tl::sprintf (tl::to_string (tr ("instance(\"%s\" %s %ldx%ld)")), layout.display_name (inst.cell_index ()), inst.complex_trans ().to_string (), amax, bmax);
      }

      msg += tl::to_string (tr (" in "));
      msg += path_to_string (layout, *r);

      view ()->message (msg, transient ? 10 : 10000);

    } else {

      std::string msg;
      if (! transient) {
        msg = tl::to_string (tr ("selected: "));
      }

      if (r->shape ().is_box ()) {
        db::Box b (r->shape ().bbox ());
        msg += tl::sprintf (tl::to_string (tr ("box(%d,%d %d,%d)")), int (b.left ()), int (b.bottom ()), int (b.right ()), int (b.top ()));
      } else if (r->shape ().is_text ()) {
        msg += tl::sprintf (tl::to_string (tr ("text(\"%s\" %s)")), tl::escape_string (r->shape ().text_string ()), r->shape ().text_trans ().to_string ());
      } else if (r->shape ().is_polygon ()) {
        size_t npoints = 0;
        for (db::Shape::polygon_edge_iterator e = r->shape ().begin_edge (); ! e.at_end (); ++e) {
          ++npoints;
        }
        msg += tl::sprintf (tl::to_string (tr ("polygon(#points=%lu)")), npoints);
      } else if (r->shape ().is_path ()) {
        size_t npoints = 0;
        for (db::Shape::point_iterator p = r->shape ().begin_point (); p != r->shape ().end_point (); ++p) {
          ++npoints;
        }
        msg += tl::sprintf (tl::to_string (tr ("path(w=%d #points=%lu)")), int (r->shape ().path_width ()), npoints);
      }

      if (! msg.empty ()) {

        msg += tl::to_string (tr (" on "));

        std::string ln = layout.get_properties (r->layer ()).to_string ();
        for (lay::LayerPropertiesConstIterator lp = view ()->begin_layers (); ! lp.at_end (); ++lp) {
          if (lp->layer_index () == int (r->layer ()) && lp->cellview_index () == int (r->cv_index ())) {
            ln = lp->display_string (view (), true, false);
            break;
          }
        }
        msg += ln;

        msg += tl::to_string (tr (" in "));
        msg += path_to_string (layout, *r);

        view ()->message (msg, transient ? 10 : 10000);

      }

    }

  } else {
    view ()->message (std::string ());
  }
}

void
Service::clear_transient_selection ()
{
  if (mp_transient_marker) {
    delete mp_transient_marker;
    mp_transient_marker = 0;
  }

  m_transient_selection.clear ();
}

bool
Service::selection_applies (const lay::ObjectInstPath & /*sel*/) const
{
  return false;
}

void
Service::transient_to_selection ()
{
  if (! m_transient_selection.empty ()) {
    m_selection.insert (m_transient_selection.begin (), m_transient_selection.end ());
    selection_to_view ();
  }
}

void
Service::clear_previous_selection ()
{
  m_previous_selection.clear ();
}

bool 
Service::select (const db::DBox &box, lay::Editable::SelectionMode mode)
{
  //  compute search box
  double l = box.is_point () ? catch_distance () : catch_distance_box ();
  db::DBox search_box = box.enlarged (db::DVector (l, l));

  bool needs_update = false;
  bool any_selected = false;

  //  clear before unless "add" is selected
  if (mode == lay::Editable::Replace) {
    if (! m_selection.empty ()) {
      m_selection.clear ();
      needs_update = true;
    }
  }

  //  for single-point selections either exclude the current selection or the
  //  accumulated previous selection from the search.
  const objects *exclude = 0;
  if (mode == lay::Editable::Replace) {
    exclude = &m_previous_selection;
  } else if (mode == lay::Editable::Add) {
    exclude = &m_selection;
  } else if (mode == lay::Editable::Reset) {
    //  TODO: the finder should favor the current selection in this case.
  }

  if (box.empty ()) {

    //  unconditional selection
    if (mode == lay::Editable::Reset) {
      if (! m_selection.empty ()) {
        m_selection.clear ();
        needs_update = true;
      }
    } else {

      //  extract all shapes

      //  TODO: not implemented yet 

    }

  } else if (m_cell_inst_service) {

    lay::InstFinder finder (box.is_point (), view ()->is_editable () && m_top_level_sel, view ()->is_editable () /*full arrays in editable mode*/, true /*enclose_inst*/, exclude, true /*only visible layers*/);

    //  go through all cell views
    std::set< std::pair<db::DCplxTrans, int> > variants = view ()->cv_transform_variants ();
    for (std::set< std::pair<db::DCplxTrans, int> >::const_iterator v = variants.begin (); v != variants.end (); ++v) {
      finder.find (view (), v->second, v->first, search_box);
    }

    //  collect the founds from the finder
    for (lay::InstFinder::iterator f = finder.begin (); f != finder.end (); ++f) {
      select (*f, mode);
      if (box.is_point ()) {
        m_previous_selection.insert (*f);
      }
      needs_update = true;
      any_selected = true;
    }

  } else {

    lay::ShapeFinder finder (box.is_point (), view ()->is_editable () && m_top_level_sel, m_flags, exclude);

    //  go through all visible layers of all cellviews
    finder.find (view (), search_box);

    //  guiding shapes are only selected in point selection mode and even then, we
    //  only select the first shape
    lay::ShapeFinder::iterator f0 = finder.begin ();
    if (box.is_point () && f0 != finder.end () && f0->layer () == view ()->cellview (f0->cv_index ())->layout ().guiding_shape_layer ()) {

      m_selection.clear ();
      select (*f0, mode);
      m_previous_selection.insert (*f0);
      needs_update = true;
      any_selected = true;

    } else {

      //  clear the selection if it was consisting of a guiding shape before
      objects::const_iterator s0 = m_selection.begin (); 
      if (s0 != m_selection.end () && s0->layer () == view ()->cellview (s0->cv_index ())->layout ().guiding_shape_layer ()) {
        m_selection.clear ();
      }

      //  collect the founds from the finder
      for (lay::ShapeFinder::iterator f = finder.begin (); f != finder.end (); ++f) {
        if (f->layer () != view ()->cellview (f->cv_index ())->layout ().guiding_shape_layer ()) {
          select (*f, mode);
          if (box.is_point ()) {
            m_previous_selection.insert (*f);
          }
          needs_update = true;
          any_selected = true;
        }
      }

    }
      
  }

  //  if required, update the list of ruler objects to display the selection
  if (needs_update) {
    selection_to_view ();
  }

  if (any_selected) {
    display_status (false);
  }

  return any_selected;
}

void 
Service::get_selection (std::vector <lay::ObjectInstPath> &sel) const
{
  sel.clear ();
  sel.reserve (m_selection.size ());

  //  positions will hold a set of iterators that are to be erased
  for (std::set<lay::ObjectInstPath>::const_iterator r = m_selection.begin (); r != m_selection.end (); ++r) {
    sel.push_back (*r);
  }
}

bool 
Service::select (const lay::ObjectInstPath &obj, lay::Editable::SelectionMode mode)
{
  //  allocate next sequence number
  if (mode == lay::Editable::Replace) {
    m_seq = 0;
  } else if (mode != lay::Editable::Reset) {
    ++m_seq;
  }

  if (mode == lay::Editable::Replace || mode == lay::Editable::Add) {

    //  select
    if (m_selection.find (obj) == m_selection.end ()) {

      obj_iterator o = m_selection.insert (obj).first;
      (const_cast <lay::ObjectInstPath &> (*o)).set_seq (m_seq); // we can do that since the sequence number is not part of the less operator
      selection_to_view ();
      return true;

    }

  } else if (mode == lay::Editable::Reset) {

    //  unselect
    if (m_selection.find (obj) != m_selection.end ()) {

      m_selection.erase (obj);
      selection_to_view ();
      return true;

    }

  } else {

    //  invert selection
    if (m_selection.find (obj) != m_selection.end ()) {
      m_selection.erase (obj);
    } else {
      obj_iterator o = m_selection.insert (obj).first;
      (const_cast <lay::ObjectInstPath &> (*o)).set_seq (m_seq); // we can do that since the sequence number is not part of the less operator
    }

    selection_to_view ();
    return true;

  }

  return false;
}

void  
Service::move_markers (const db::DTrans &t)
{
  if (m_move_trans != t) {

    //  display current move vector
    if (has_selection ()) {
      std::string pos = std::string ("dx: ") + tl::micron_to_string (t.disp ().x ()) + "  dy: " + tl::micron_to_string (t.disp ().y ());
      if (t.rot () != 0) {
        pos += std::string ("  ") + ((const db::DFTrans &) t).to_string ();
      }
      view ()->message (pos);
    }

    for (std::vector<lay::ViewObject *>::iterator r = m_markers.begin (); r != m_markers.end (); ++r) {

      lay::GenericMarkerBase *marker = dynamic_cast<lay::GenericMarkerBase *> (*r);
      if (marker) {
        db::DCplxTrans dt = db::DCplxTrans (t) * db::DCplxTrans (m_move_trans).inverted ();
        marker->set_trans (dt * marker->trans ());
      }

    }

    m_move_trans = t;

  }
}

void
Service::begin_edit (const db::DPoint &p)
{
  do_begin_edit (p);
  m_editing = true;
}

void
Service::tap (const db::DPoint & /*initial*/)
{
  //  .. nothing here ..
}

void 
Service::selection_to_view ()
{
  //  we don't handle the transient selection properly, so clear it for safety reasons
  clear_transient_selection ();

  //  the selection objects need to be recreated since we destroyed the old markers
  for (std::vector<lay::ViewObject *>::iterator r = m_markers.begin (); r != m_markers.end (); ++r) {
    delete *r;
  }
  m_markers.clear ();

  dm_selection_to_view ();
}

void 
Service::do_selection_to_view ()
{
  //  Hint: this is a lower bound:
  m_markers.reserve (m_selection.size ());

  //  build the transformation variants cache
  TransformationVariants tv (view ());

  //  Reduce the selection to valid paths (issue-1145)
  std::vector<std::set<lay::ObjectInstPath>::iterator> invalid_objects;
  for (std::set<lay::ObjectInstPath>::iterator r = m_selection.begin (); r != m_selection.end (); ++r) {
    if (! r->is_valid (view ())) {
      invalid_objects.push_back (r);
    }
  }
  for (auto i = invalid_objects.begin (); i != invalid_objects.end (); ++i) {
    m_selection.erase (*i);
  }

  //  Build markers

  for (std::set<lay::ObjectInstPath>::iterator r = m_selection.begin (); r != m_selection.end (); ++r) {

    const lay::CellView &cv = view ()->cellview (r->cv_index ());

    //  compute the global transformation including movement, context and explicit transformation
    double dbu = cv->layout ().dbu ();
    db::ICplxTrans gt = db::VCplxTrans (1.0 / dbu) * db::DCplxTrans (m_move_trans) * db::CplxTrans (dbu) * cv.context_trans () * r->trans ();

    tl_assert (r->is_cell_inst () == m_cell_inst_service);

    if (m_cell_inst_service) {

      const std::vector<db::DCplxTrans> *tv_list = tv.per_cv (r->cv_index ());
      if (tv_list != 0) {
      
        if (view ()->is_editable ()) {

#if 0
          //  to show the content of the cell when the instance is selected:
          lay::InstanceMarker *marker = new lay::InstanceMarker (view (), r->cv_index (), ! show_shapes_of_instances (), show_shapes_of_instances () ? max_shapes_of_instances () : 0);
#else
          lay::InstanceMarker *marker = new lay::InstanceMarker (view (), r->cv_index ());
#endif
          marker->set_vertex_shape (lay::ViewOp::Cross);
          marker->set_vertex_size (9 /*cross vertex size*/);

          if (r->seq () > 0 && m_indicate_secondary_selection) { 
            marker->set_dither_pattern (3); 
          } 
          marker->set (r->back ().inst_ptr, gt, *tv_list);
          m_markers.push_back (marker);

        } else {

          lay::Marker *marker = new lay::Marker (view (), r->cv_index ());
          marker->set_vertex_shape (lay::ViewOp::Cross);
          marker->set_vertex_size (9 /*cross vertex size*/);

          if (r->seq () > 0 && m_indicate_secondary_selection) { 
            marker->set_dither_pattern (3); 
          } 
          db::box_convert<db::CellInst> bc (cv->layout ());
          marker->set (bc (r->back ().inst_ptr.cell_inst ().object ()), gt * r->back ().inst_ptr.cell_inst ().complex_trans (*r->back ().array_inst), *tv_list);
          m_markers.push_back (marker);

        }

      }

    } else {

      const std::vector<db::DCplxTrans> *tv_list = tv.per_cv_and_layer (r->cv_index (), r->layer ());
      if (tv_list != 0) {

        lay::ShapeMarker *marker = new lay::ShapeMarker (view (), r->cv_index ());
        if (r->seq () > 0 && m_indicate_secondary_selection) { 
          marker->set_dither_pattern (3); 
        } 

        marker->set (r->shape (), gt, *tv_list);

        bool is_point = false;
        if (r->shape ().is_text () || r->shape ().is_point ()) {
          is_point = true;
        } else if (r->shape ().is_edge () || r->shape ().is_box ()) {
          is_point = r->shape ().bbox ().is_point ();
        }

        if (is_point) {
          //  show the origins as crosses for texts
          marker->set_vertex_shape (lay::ViewOp::Cross);
          marker->set_vertex_size (9 /*cross vertex size*/);
        }

        m_markers.push_back (marker);

      }

    }

  }

  apply_highlights ();
}

void 
Service::set_selection (std::vector <lay::ObjectInstPath>::const_iterator s1, std::vector <lay::ObjectInstPath>::const_iterator s2)
{
  m_selection.clear ();
  m_selection.insert (s1, s2);
  selection_to_view ();
}

void 
Service::remove_selection (const lay::ObjectInstPath &sel)
{
  m_selection.erase (sel);
  selection_to_view ();
}

void 
Service::add_selection (const lay::ObjectInstPath &sel)
{
  m_selection.insert (sel);
  selection_to_view ();
}

std::pair<bool, lay::ObjectInstPath>
Service::handle_guiding_shape_changes (const lay::ObjectInstPath &obj) const
{
  unsigned int cv_index = obj.cv_index ();
  lay::CellView cv = view ()->cellview (cv_index);
  db::Layout *layout = &cv->layout ();

  if (obj.is_cell_inst () || obj.layer () != layout->guiding_shape_layer ()) {
    return std::make_pair (false, lay::ObjectInstPath ());
  }

  if (! obj.shape ().has_prop_id ()) {
    return std::make_pair (false, lay::ObjectInstPath ());
  }

  if (! layout->is_pcell_instance (obj.cell_index ()).first) {
    return std::make_pair (false, lay::ObjectInstPath ());
  }

  db::cell_index_type top_cell = std::numeric_limits<db::cell_index_type>::max ();
  db::cell_index_type parent_cell = std::numeric_limits<db::cell_index_type>::max ();
  db::Instance parent_inst;
  db::pcell_parameters_type parameters_for_pcell;

  //  determine parent cell and instance if required
  lay::ObjectInstPath::iterator e = obj.end ();
  if (e == obj.begin ()) {
    top_cell = obj.cell_index ();
  } else {
    --e;
    db::cell_index_type pc = obj.topcell ();
    if (e != obj.begin ()) {
      --e;
      pc = e->inst_ptr.cell_index ();
    }
    parent_cell = pc;
    parent_inst = obj.back ().inst_ptr;
  }

  db::property_names_id_type pn = layout->properties_repository ().prop_name_id ("name");

  const db::PropertiesRepository::properties_set &input_props = layout->properties_repository ().properties (obj.shape ().prop_id ());
  db::PropertiesRepository::properties_set::const_iterator input_pv = input_props.find (pn);
  if (input_pv == input_props.end ()) {
    return std::make_pair (false, lay::ObjectInstPath ());
  }

  std::string shape_name = input_pv->second.to_string ();

  //  Hint: get_parameters_from_pcell_and_guiding_shapes invalidates the shapes because it resets the changed
  //  guiding shapes. We must not access s->shape after that.
  if (! get_parameters_from_pcell_and_guiding_shapes (layout, obj.cell_index (), parameters_for_pcell)) {
    return std::make_pair (false, lay::ObjectInstPath ());
  }

  bool found = false;
  lay::ObjectInstPath new_obj = obj;

  if (parent_cell != std::numeric_limits <db::cell_index_type>::max ()) {

    db::Instance new_inst = layout->cell (parent_cell).change_pcell_parameters (parent_inst, parameters_for_pcell);

    //  try to identify the selected shape in the new shapes and select this one
    db::Shapes::shape_iterator sh = layout->cell (new_inst.cell_index ()).shapes (layout->guiding_shape_layer ()).begin (db::ShapeIterator::All);
    while (! sh.at_end () && !found) {
      const db::PropertiesRepository::properties_set &props = layout->properties_repository ().properties (sh->prop_id ());
      db::PropertiesRepository::properties_set::const_iterator pv = props.find (pn);
      if (pv != props.end ()) {
        if (pv->second.to_string () == shape_name) {
          new_obj.back ().inst_ptr = new_inst;
          new_obj.back ().array_inst = new_inst.begin ();
          new_obj.set_shape (*sh);
          found = true;
        }
      }
      ++sh;
    }

  }

  if (top_cell != std::numeric_limits <db::cell_index_type>::max ()) {
    // TODO: implement the case of a PCell variant being a top cell
    // Currently there is not way to create such a configuration ...
  }

  return std::make_pair (found, new_obj);
}

bool 
Service::handle_guiding_shape_changes ()
{
  //  just allow one guiding shape to be selected
  if (m_selection.empty ()) {
    return false;
  }

  std::pair<bool, lay::ObjectInstPath> gs = handle_guiding_shape_changes (*m_selection.begin ());
  if (gs.first) {

    //  remove superfluous proxies
    view ()->cellview (gs.second.cv_index ())->layout ().cleanup ();

    //  re-set the selection
    std::vector<lay::ObjectInstPath> new_sel;
    new_sel.push_back (gs.second);
    set_selection (new_sel.begin (), new_sel.end ());

    return true;

  } else {
    return false;
  }
}


// -------------------------------------------------------------

} // namespace edt


