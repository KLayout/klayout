
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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


#include "edtPathService.h"

#include "layLayoutViewBase.h"
#include "layFinder.h"

#if defined(HAVE_QT)
#  include "edtPropertiesPages.h"
#  include "layLayoutView.h"
#endif

namespace edt
{

// -----------------------------------------------------------------------------
//  PathService implementation

PathService::PathService (db::Manager *manager, lay::LayoutViewBase *view)
  : ShapeEditService (manager, view, db::ShapeIterator::Paths), 
    m_width (0.1), m_bgnext (0.0), m_endext (0.0), m_type (Flush), m_needs_update (true)
{
  //  .. nothing yet ..
}

PathService::~PathService ()
{
  //  .. nothing yet ..
}

#if defined(HAVE_QT)
std::vector<lay::PropertiesPage *>
PathService::properties_pages (db::Manager *manager, QWidget *parent)
{
  std::vector<lay::PropertiesPage *> pages;
  if (view ()->is_editable ()) {
    pages.push_back (new edt::EditablePathPropertiesPage (this, manager, parent));
  } else {
    pages.push_back (new edt::PathPropertiesPage (this, manager, parent));
  }
  return pages;
}
#endif

void 
PathService::do_begin_edit (const db::DPoint &p)
{
  get_edit_layer ();

  m_previous_segments.clear ();

  db::DPoint pp = snap2 (p);
  m_last = pp;

  m_points.clear ();
  m_points.push_back (pp);
  m_points.push_back (pp);

  open_editor_hooks ();

  set_edit_marker (new lay::Marker (view (), cv_index ()));
  update_marker ();
}

bool
PathService::do_activated ()
{
  return false;  //  don't start editing immediately
}

void 
PathService::set_last_point (const db::DPoint &p)
{
  m_points.back () = snap2 (p, m_last);

  //  for manhattan polygons allow some movement of the projected edge
  if (m_points.size () >= 3 && connect_ac () == lay::AC_Ortho) {

    db::DPoint p_grid = snap2 (p);
    std::pair<bool, db::DPoint> ip = interpolate (m_points.end ()[-3], m_last, p_grid);
    if (ip.first) {

      m_points.end ()[-2] = ip.second;
      m_points.back () = p_grid;

    }

  } else if (m_points.size () >= 2) {
    m_points.end ()[-2] = m_last;
  }
}

void
PathService::do_mouse_move_inactive (const db::DPoint &p)
{
  lay::PointSnapToObjectResult snap_details = snap2_details (p);
  mouse_cursor_from_snap_details (snap_details);
}

void
PathService::do_mouse_move (const db::DPoint &p)
{
  do_mouse_move_inactive (p);

  set_cursor (lay::Cursor::cross);
  if (m_points.size () >= 2) {
    set_last_point (p);
  }

  update_marker ();
  update_via ();
}

bool 
PathService::do_mouse_click (const db::DPoint &p)
{
  if (m_points.size () >= 1) {
    m_last = m_points.back ();
    m_points.push_back (db::DPoint ());
    set_last_point (p);
  }
  return false;
}

void
PathService::do_delete ()
{
  if (m_points.size () > 2) {

    m_points.erase (m_points.end () - 2);
    m_last = m_points.end()[-2];

    update_marker ();
    update_via ();

  } else if (! m_previous_segments.empty ()) {

    pop_segment ();

  }

}

void
PathService::do_finish_edit ()
{
  //  one point is reserved for the "current one"
  if (m_points.size () < 3) {
    throw tl::Exception (tl::to_string (tr ("A path must have at least 2 points")));
  }
  m_points.pop_back ();

  deliver_shape (get_path ());

  commit_recent ();

  close_editor_hooks (true);
}

void
PathService::update_marker ()
{
  lay::Marker *marker = dynamic_cast<lay::Marker *> (edit_marker ());
  if (marker) {

    db::Path path (get_path ());
    marker->set (path, db::VCplxTrans (1.0 / layout ().dbu ()) * trans ().inverted ());

    if (m_points.size () >= 2) {
      view ()->message (std::string ("lx: ") +
                        tl::micron_to_string (m_points.back ().x () - m_points.end () [-2].x ()) + 
                        std::string ("  ly: ") +
                        tl::micron_to_string (m_points.back ().y () - m_points.end () [-2].y ()) + 
                        std::string ("  l: ") +
                        tl::micron_to_string (m_points.back ().distance (m_points.end () [-2])));
    }

  }

  //  call hooks with new shape
  if (! editor_hooks ().empty ()) {
    call_editor_hooks (editor_hooks (), &edt::EditorHooks::begin_new_shapes);
    try {
      deliver_shape_to_hooks (get_path ());
    } catch (...) {
      //  ignore exceptions
    }
    call_editor_hooks (editor_hooks (), &edt::EditorHooks::end_new_shapes);
  }
}

db::Path 
PathService::get_path () const
{
  db::Path path;

  std::vector<db::Point> points_dbu;
  points_dbu.reserve (m_points.size ());
  for (std::vector<db::DPoint>::const_iterator p = m_points.begin (); p != m_points.end (); ++p) {
    points_dbu.push_back (trans () * *p);
  }

  path.width (trans ().ctrans (m_width));

  path.round (m_type == Round);
  if (m_type == Flush) {
    path.bgn_ext (0);
    path.end_ext (0);
  } else if (m_type == Square || m_type == Round) {
    path.bgn_ext (path.width () / 2);
    path.end_ext (path.width () / 2);
  } else {
    path.bgn_ext (trans ().ctrans (m_bgnext));
    path.end_ext (trans ().ctrans (m_endext));
  }
  path.assign (points_dbu.begin (), points_dbu.end ());

  return path;
}

void 
PathService::do_cancel_edit ()
{
  close_editor_hooks (false);
}

bool 
PathService::selection_applies (const lay::ObjectInstPath &sel) const
{
  return !sel.is_cell_inst () && sel.shape ().is_path ();
}

void
PathService::via (int dir)
{
//  see TODO below
#if defined(HAVE_QT)
  if (combine_mode () != CM_Add) {
    throw tl::Exception (tl::to_string (tr ("Vias are only available in 'Add' combination mode")));
  }

  if (editing ()) {
    via_editing (dir);
  } else {
    via_initial (dir);
  }
#endif
}

bool
PathService::get_via_for (const db::LayerProperties &lp, unsigned int cv_index, int dir, db::SelectedViaDefinition &via_def)
{
  const lay::CellView &cv = view ()->cellview (cv_index);
  if (! cv.is_valid ()) {
    return false;
  }

  std::vector<db::SelectedViaDefinition> via_defs = db::find_via_definitions_for (cv->layout ().technology_name (), lp, dir);

  if (via_defs.size () == 0) {

    return false;

  } else if (via_defs.size () == 1) {

    via_def = via_defs.front ();

  } else if (via_defs.size () > 1) {

#if defined(HAVE_QT)
    //  present a menu with the available vias.
    //  TODO: what to do here in Qt-less case?

    QWidget *view_widget = lay::widget_from_view (view ());
    if (! view_widget) {
      return false;
    }

    std::unique_ptr<QMenu> menu (new QMenu (view_widget));
    menu->show ();

    db::DPoint mp_local = view ()->canvas ()->mouse_position ();
    QPoint mp = view ()->canvas ()->widget ()->mapToGlobal (QPoint (mp_local.x (), mp_local.y ()));

    for (auto i = via_defs.begin (); i != via_defs.end (); ++i) {
      QAction *a = menu->addAction (tl::to_qstring (i->via_type.description.empty () ? i->via_type.name : i->via_type.description));
      a->setData (int (i - via_defs.begin ()));
    }

    QAction *action = menu->exec (mp);
    if (! action) {
      return false;
    }

    via_def = via_defs [action->data ().toInt ()];
#endif

  }

  return true;
}

db::Instance
PathService::make_via (const db::SelectedViaDefinition &via_def, double w_bottom, double h_bottom, double w_top, double h_top, const db::DPoint &via_pos)
{
  if (! via_def.via_type.cut.is_null ()) {
    edt::set_or_request_current_layer (view (), via_def.via_type.cut, cv_index (), false /*don't make current*/);
  }

  std::map<std::string, tl::Variant> params;
  params.insert (std::make_pair ("via", tl::Variant (via_def.via_type.name)));
  params.insert (std::make_pair ("w_bottom", tl::Variant (w_bottom)));
  params.insert (std::make_pair ("w_top", tl::Variant (w_top)));
  params.insert (std::make_pair ("h_bottom", tl::Variant (h_bottom)));
  params.insert (std::make_pair ("h_top", tl::Variant (h_top)));

  auto via_lib_cell = via_def.lib->layout ().get_pcell_variant_dict (via_def.pcell, params);
  auto via_cell = layout ().get_lib_proxy (via_def.lib, via_lib_cell);

  return cell ().insert (db::CellInstArray (db::CellInst (via_cell), db::Trans (trans () * via_pos - db::Point ())));
}

void
PathService::via_initial (int dir)
{
  if (! mouse_in_view ()) {
    return;
  }

  //  compute search box
  double l = catch_distance ();
  db::DPoint pos = mouse_pos ();
  db::DBox search_box = db::DBox (pos, pos).enlarged (db::DVector (l, l));

  lay::ShapeFinder finder (true, false, db::ShapeIterator::Regions);

  //  go through all visible layers of all cellviews
  finder.find (view (), search_box);

  //  collect the founds from the finder
  lay::ShapeFinder::iterator r = finder.begin ();
  if (r == finder.end ()) {
    return;
  }

  const lay::CellView &cv = view ()->cellview (r->cv_index ());
  if (! cv.is_valid ()) {
    return;
  }

  db::LayerProperties lp = cv->layout ().get_properties (r->layer ());

  db::SelectedViaDefinition via_def;
  if (! get_via_for (lp, r->cv_index (), dir, via_def)) {
    return;
  }

  set_layer (lp, r->cv_index ());

  bool is_bottom = via_def.via_type.bottom.log_equal (lp);
  db::LayerProperties lp_new = is_bottom ? via_def.via_type.top : via_def.via_type.bottom;

  {
    db::Transaction transaction (manager (), tl::to_string (tr ("Create path segment")));

    change_edit_layer (lp_new);
    begin_edit (pos);

    //  create the via cell
    //  (using 0.0 for all dimensions to indicate "place here")
    db::Instance via_instance = make_via (via_def, 0.0, 0.0, 0.0, 0.0, m_last);

    push_segment (db::Shape (), via_instance, via_def.via_type, transaction.id ());
  }
}

void
PathService::compute_via_wh (double &w, double &h, const db::DVector &dwire, double var_ext, double grid)
{
  w = 0.0, h = 0.0;

  if (m_type == Round) {

    //  a square sitting in the circle at the end
    w = h = sqrt (0.5) * m_width;

  } else {

    double ext = 0.0;
    if (m_type == Square) {
      ext = m_width * 0.5;
    } else if (m_type == Variable) {
      ext = var_ext;
    }

    double vl = dwire.length ();

    if (vl < db::epsilon || ext < -db::epsilon) {

      //  no specific dimension

    } else if (ext < db::epsilon) {

      //  a rectangle enclosing the flush end edge
      db::DVector l = dwire * (m_width / vl);
      w = std::abs (l.y ());
      h = std::abs (l.x ());

    } else if (std::fabs (dwire.x ()) < db::epsilon) {

      //  vertical path
      w = m_width;
      h = ext * 2.0;

    } else if (std::fabs (dwire.y ()) < db::epsilon) {

      //  horizontal path
      h = m_width;
      w = ext * 2.0;

    } else {

      //  compute dimension of max. inscribed box

      db::DVector v = db::DVector (std::abs (dwire.x ()) / vl, std::abs (dwire.y ()) / vl);

      double e = ext, en = m_width * 0.5;

      bool swap_xy = false;
      if (e > en) {
        std::swap (e, en);
        v = db::DVector (v.y (), v.x ());
        swap_xy = true;
      }

      double vd = v.y () * v.y () - v.x () * v.x ();
      double vp = v.x () * v.y ();

      double l = e * 0.5 * vd / vp;

      if (std::abs (vd) > db::epsilon) {
        double l1 = (en - 2 * e * vp) / vd;
        double l2 = (-en - 2 * e * vp) / vd;
        l = std::max (l, std::min (l1, l2));
        l = std::min (l, std::max (l1, l2));
      }

      db::DVector a = v * e + db::DVector (v.y (), -v.x ()) * l;
      w = a.x () * 2.0;
      h = a.y () * 2.0;

      if (swap_xy) {
        std::swap (w, h);
      }

    }

  }

  //  round to grid or DBU

  if (grid < db::epsilon) {
    grid = layout ().dbu ();
  }

  w = floor (w / grid + db::epsilon) * grid;
  h = floor (h / grid + db::epsilon) * grid;
}

void
PathService::via_editing (int dir)
{
  //  not enough points to form a path
  if (m_points.size () < 2) {
    return;
  }

  db::LayerProperties lp = layout ().get_properties (layer ());

  db::SelectedViaDefinition via_def;
  if (! get_via_for (lp, cv_index (), dir, via_def)) {
    return;
  }

  commit_recent ();

  bool is_bottom = via_def.via_type.bottom.log_equal (lp);
  db::LayerProperties lp_new = is_bottom ? via_def.via_type.top : via_def.via_type.bottom;

  //  compute the via parameters

  db::DVector dwire = m_points.back () - m_points [m_points.size () - 2];

  double w = 0.0, h = 0.0;
  compute_via_wh (w, h, dwire, m_endext, is_bottom ? via_def.via_type.bottom_grid : via_def.via_type.top_grid);

  double w_bottom = 0.0, h_bottom = 0.0, w_top = 0.0, h_top = 0.0;
  (is_bottom ? w_bottom : w_top) = w;
  (is_bottom ? h_bottom : h_top) = h;

  //  create the path and via

  db::DPoint via_pos = m_points.back ();

  {
    db::Transaction transaction (manager (), tl::to_string (tr ("Create path segment")));

    db::Shape path_shape = cell ().shapes (layer ()).insert (get_path ());
    db::Instance via_instance = make_via (via_def, w_bottom, h_bottom, w_top, h_top, via_pos);

    push_segment (path_shape, via_instance, via_def.via_type, transaction.id ());

    change_edit_layer (lp_new);
  }

  m_points.clear ();
  m_points.push_back (via_pos);
  m_points.push_back (via_pos);
  m_last = m_points.back ();

  update_marker ();
  update_via ();
}

void
PathService::update_via ()
{
  if (! editing () || m_points.size () < 2) {
    return;
  }

  if (m_previous_segments.empty () || m_previous_segments.back ().via_instance.is_null ()) {
    return;
  }

  PathSegment &ps = m_previous_segments.back ();

  if (! ps.via_instance.instances ()) {
    return;
  }

  db::Cell *via_parent_cell = ps.via_instance.instances ()->cell ();

  //  Compute the parameters to change

  db::LayerProperties lp = layout ().get_properties (layer ());
  bool is_bottom = ps.via_type.bottom.log_equal (lp);

  double w = 0.0, h = 0.0;
  compute_via_wh (w, h, m_points [1] - m_points [0], m_bgnext, is_bottom ? ps.via_type.bottom_grid : ps.via_type.top_grid);

  std::map<std::string, tl::Variant> params;

  if (is_bottom) {
    params.insert (std::make_pair ("w_bottom", tl::Variant (w)));
    params.insert (std::make_pair ("h_bottom", tl::Variant (h)));
  } else {
    params.insert (std::make_pair ("w_top", tl::Variant (w)));
    params.insert (std::make_pair ("h_top", tl::Variant (h)));
  }

  //  change the via PCell

  {
    db::Transaction transaction (manager () && ! manager ()->transacting () ? manager () : 0, std::string (), ps.transaction_id);
    ps.via_instance = via_parent_cell->change_pcell_parameters (ps.via_instance, params);

    layout ().cleanup ();
  }
}

void
PathService::push_segment (const db::Shape &shape, const db::Instance &instance, const db::ViaType &via_type, db::Manager::transaction_id_t transaction_id)
{
  m_previous_segments.push_back (PathSegment ());

  PathSegment &ps = m_previous_segments.back ();
  ps.points = m_points;
  ps.last_point = m_last;
  ps.path_shape = shape;
  ps.via_instance = instance;
  ps.via_type = via_type;
  ps.layer = layout ().get_properties (layer ());
  ps.cv_index = cv_index ();
  ps.transaction_id = transaction_id;

  static std::string path_config_keys [] = {
    cfg_edit_path_width,
    cfg_edit_path_ext_var_begin,
    cfg_edit_path_ext_var_end,
    cfg_edit_path_ext_type
  };

  for (unsigned int i = 0; i < sizeof (path_config_keys) / sizeof (path_config_keys[0]); ++i) {
    ps.config.push_back (std::make_pair (path_config_keys [i], std::string ()));
    dispatcher ()->config_get (ps.config.back ().first, ps.config.back ().second);
  }
}

void
PathService::pop_segment ()
{
  PathSegment ps = m_previous_segments.back ();
  m_previous_segments.pop_back ();

  if (manager () && manager ()->transaction_id_for_undo () == ps.transaction_id) {

    //  should remove shape and via instance
    manager ()->undo ();

    //  empties the undo queue, so we don't keep objects there and spoil subsequent "update_via" actions
    //  TODO: is there a better way to do this?
    manager ()->transaction (std::string ());
    manager ()->cancel ();

  } else {

    //  fallback without using undo
    db::Transaction transaction (manager (), tl::to_string (tr ("Undo path segment")));

    if (! ps.path_shape.is_null () && ps.path_shape.shapes ()) {
      ps.path_shape.shapes ()->erase_shape (ps.path_shape);
    }

    if (! ps.via_instance.is_null () && ps.via_instance.instances ()) {
      ps.via_instance.instances ()->erase (ps.via_instance);
    }

  }

  set_layer (ps.layer, ps.cv_index);

  m_points = ps.points;
  m_last = ps.last_point;

  for (auto i = ps.config.begin (); i != ps.config.end (); ++i) {
    dispatcher ()->config_set (i->first, i->second);
  }

  //  avoids update_via() which might spoil the via we just recovered
  m_needs_update = false;
  dispatcher ()->config_end ();

  update_marker ();
}

bool 
PathService::configure (const std::string &name, const std::string &value)
{
  if (name == cfg_edit_path_width) {
    tl::from_string (value, m_width);
    m_needs_update = true;
    return true; // taken
  }

  if (name == cfg_edit_path_ext_var_begin) {
    tl::from_string (value, m_bgnext);
    m_needs_update = true;
    return true; // taken
  }

  if (name == cfg_edit_path_ext_var_end) {
    tl::from_string (value, m_endext);
    m_needs_update = true;
    return true; // taken
  }

  if (name == cfg_edit_path_ext_type) {
    m_type = Flush;
    if (value == "square") {
      m_type = Square;
    } else if (value == "round") {
      m_type = Round;
    } else if (value == "variable") {
      m_type = Variable;
    }
    m_needs_update = true;
    return true; // taken
  }

  return ShapeEditService::configure (name, value);
}

void 
PathService::config_finalize ()
{
  if (m_needs_update) {
    update_marker ();
    update_via ();
    m_needs_update = false;
  }

  ShapeEditService::config_finalize ();
}

} // namespace edt
