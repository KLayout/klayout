
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

#include "edtShapeService.h"
#include "edtMainService.h"
#include "layLayoutView.h"
#include "dbEdgeProcessor.h"
#include "dbPolygonTools.h"

#if defined(HAVE_QT)
#  include "edtPropertiesPages.h"
#  include "layTipDialog.h"
#  include "layEditorOptionsPages.h"
#endif

namespace edt
{

// -----------------------------------------------------------------------------
//  ShapeEditService implementation

ShapeEditService::ShapeEditService (db::Manager *manager, lay::LayoutViewBase *view, db::ShapeIterator::flags_type shape_types)
  : edt::Service (manager, view, shape_types), 
    m_layer (0), m_cv_index (0), mp_cell (0), mp_layout (0), m_combine_mode (CM_Add), m_update_edit_layer_enabled (true)
{
  view->current_layer_changed_event.add (this, &ShapeEditService::update_edit_layer);
}

bool 
ShapeEditService::configure (const std::string &name, const std::string &value)
{
  if (name == cfg_edit_combine_mode) {
    CMConverter ().from_string (value, m_combine_mode);
    return false; // pass to other plugins
  } else {
    return edt::Service::configure (name, value);
  }
}
  
void
ShapeEditService::activated ()
{
  edt::Service::activated ();

  if (view () == lay::LayoutView::current ()) {
    lay::LayerPropertiesConstIterator cl = view ()->current_layer ();
    update_edit_layer (cl);
  }
}

void
ShapeEditService::config_recent_for_layer (const db::LayerProperties &lp, int cv_index)
{
  if (lp.is_null ()) {
    return;
  }

#if defined(HAVE_QT)
  lay::EditorOptionsPages *eo_pages = view ()->editor_options_pages ();
  if (!eo_pages) {
    return;
  }

  for (std::vector<lay::EditorOptionsPage *>::const_iterator op = eo_pages->pages ().begin (); op != eo_pages->pages ().end (); ++op) {
    if ((*op)->plugin_declaration () == plugin_declaration ()) {
      (*op)->config_recent_for_layer (dispatcher (), lp, cv_index);
    }
  }
#endif
}


void
ShapeEditService::get_edit_layer ()
{
  lay::LayerPropertiesConstIterator cl = view ()->current_layer ();

  if (cl.is_null ()) {
    throw tl::Exception (tl::to_string (tr ("Please select a layer first")));
  } else if (! cl->valid (true)) {
    throw tl::Exception (tl::to_string (tr ("The selected layer is not valid")));
  }

#if defined(HAVE_QT)
  if (! cl->visible (true)) {
    lay::TipDialog td (QApplication::activeWindow (),
                       tl::to_string (tr ("You are now drawing on a hidden layer. The result won't be visible.")),
                       "drawing-on-invisible-layer");
    td.exec_dialog ();
  }
#endif

  int cv_index = cl->cellview_index ();
  const lay::CellView &cv = view ()->cellview (cv_index);

  if (cv_index < 0 || ! cv.is_valid ()) {
    throw tl::Exception (tl::to_string (tr ("Please select a cell first")));
  }

  int layer = cl->layer_index ();
  if (layer < 0 || ! cv->layout ().is_valid_layer ((unsigned int) layer)) {

    if (cl->has_children ()) {
      throw tl::Exception (tl::to_string (tr ("Please select a valid drawing layer first")));
    } else {

      //  create this layer now
      const lay::ParsedLayerSource &source = cl->source (true /*real*/);

      db::LayerProperties db_lp = source.layer_props ();
      cv->layout ().insert_layer (db_lp);

      //  update the layer index inside the layer view
      cl->realize_source ();
        
      //  Hint: we could have taken the new index from insert_layer, but this 
      //  is a nice test:
      layer = cl->layer_index ();
      tl_assert (layer >= 0);

    }
  }

  if (cv.cell ()->is_proxy ()) {
    throw tl::Exception (tl::to_string (tr ("Cannot put a shape into a PCell or library cell")));
  }

  m_layer = (unsigned int) layer;
  m_cv_index = (unsigned int) cv_index;
  m_trans = (cl->trans ().front () * db::CplxTrans (cv->layout ().dbu ()) * cv.context_trans ()).inverted ();
  mp_layout = &(cv->layout ());
  mp_cell = cv.cell ();

  //  fetches the last configuration for the given layer
  view ()->set_active_cellview_index (cv_index);
}

void
ShapeEditService::change_edit_layer (const db::LayerProperties &lp)
{
  if (! mp_layout) {
    return;
  }

  int layer = mp_layout->get_layer_maybe (lp);
  if (layer < 0) {
    layer = mp_layout->insert_layer (lp);
  }
  m_layer = (unsigned int) layer;

  edt::set_or_request_current_layer (view (), lp, m_cv_index);

  if (editing ()) {
    close_editor_hooks (false);
  }

  //  fetches the last configuration for the given layer
  view ()->set_active_cellview_index (m_cv_index);
  config_recent_for_layer (lp, m_cv_index);

  if (editing ()) {
    open_editor_hooks ();
  }
}

void
ShapeEditService::set_layer (const db::LayerProperties &lp, unsigned int cv_index)
{
  const lay::CellView &cv = view ()->cellview (cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  int layer = cv->layout ().get_layer_maybe (lp);
  if (layer < 0) {
    layer = cv->layout ().insert_layer (lp);
  }

  m_layer = (unsigned int) layer;
  m_cv_index = cv_index;
  mp_layout = &(cv->layout ());
  mp_cell = cv.cell ();

  m_update_edit_layer_enabled = false;

  try {

    auto cl = view ()->find_layer (cv_index, lp);
    if (! cl.is_null ()) {
      view ()->set_current_layer (cl);
      m_trans = (cl->trans ().front () * db::CplxTrans (cv->layout ().dbu ()) * cv.context_trans ()).inverted ();
    } else {
      m_trans = (db::CplxTrans (cv->layout ().dbu ()) * cv.context_trans ()).inverted ();
    }

    m_update_edit_layer_enabled = true;

  } catch (...) {
    m_update_edit_layer_enabled = true;
    throw;
  }
}

void
ShapeEditService::update_edit_layer (const lay::LayerPropertiesConstIterator &cl)
{
  if (! m_update_edit_layer_enabled) {
    return;
  }

  if (cl.is_null () || cl->has_children ()) {
    return;
  }

  int cv_index = cl->cellview_index ();
  const lay::CellView &cv = view ()->cellview (cv_index);
  if (cv_index < 0 || ! cv.is_valid ()) {
    return;
  }

  view ()->set_active_cellview_index (cv_index);

  const lay::ParsedLayerSource &source = cl->source (true /*real*/);
  int layer = cl->layer_index ();
  db::LayerProperties db_lp = source.layer_props ();

  if (! editing ()) {

    if (layer < 0 || ! cv->layout ().is_valid_layer ((unsigned int) layer)) {
      config_recent_for_layer (db_lp, cv_index);
    } else {
      config_recent_for_layer (cv->layout ().get_properties ((unsigned int) layer), cv_index);
    }

  } else {

    if (layer < 0 || ! cv->layout ().is_valid_layer ((unsigned int) layer)) {

      //  create this layer now
      cv->layout ().insert_layer (db_lp);

      //  update the layer index inside the layer view
      cl->realize_source ();

      //  Hint: we could have taken the new index from insert_layer, but this
      //  is a nice test:
      layer = cl->layer_index ();
      tl_assert (layer >= 0);

    }

    m_layer = (unsigned int) layer;
    m_cv_index = (unsigned int) cv_index;
    m_trans = (cl->trans ().front () * db::CplxTrans (cv->layout ().dbu ()) * cv.context_trans ()).inverted ();
    mp_layout = &(cv->layout ());
    mp_cell = cv.cell ();

    close_editor_hooks (false);

    //  fetches the last configuration for the given layer
    config_recent_for_layer (cv->layout ().get_properties ((unsigned int) layer), cv_index);

    open_editor_hooks ();

  }
}

void
ShapeEditService::tap (const db::DPoint &initial)
{
  if (editing ()) {
    get_edit_layer ();
  } else {
    begin_edit (initial);
  }
}

/**
 *  @brief Deliver a good interpolation between two points m and p
 *
 *  This method uses an intermediate point o to determine the edge that is emerged from point m.
 *  An edge is searched that emerges from p and intersects with the m->o edge in a way that the intersection
 *  point is closest to o.
 *
 *  This method returns the intersection point ("new o") and a flag if the search was successful (.first of return value).
 */
std::pair <bool, db::DPoint>
ShapeEditService::interpolate (const db::DPoint &m, const db::DPoint &o, const db::DPoint &p) const
{
  if (fabs (m.x () - o.x ()) < 1e-6 && fabs (m.y () - o.y ()) < 1e-6) {
    return std::pair <bool, db::DPoint> (false, db::DPoint ());
  }

  std::vector <db::DVector> delta;
  delta.reserve (4);
  delta.push_back (db::DVector (1.0, 0.0));
  delta.push_back (db::DVector (0.0, 1.0));
  if (connect_ac () == lay::AC_Diagonal) {
    delta.push_back (db::DVector (1.0, -1.0));
    delta.push_back (db::DVector (1.0, 1.0));
  }

  bool c_set = false;
  db::DPoint c;
  for (std::vector <db::DVector>::const_iterator d = delta.begin (); d != delta.end (); ++d) {
    std::pair <bool, db::DPoint> ip = db::DEdge (m, o).cut_point (db::DEdge (p - *d, p));
    if (ip.first && (! c_set || o.sq_distance (ip.second) < o.sq_distance (c))) {
      c = ip.second;
      c_set = true;
    }
  }

  return std::make_pair (c_set, c);
}

void 
ShapeEditService::do_mouse_move_inactive (const db::DPoint &p)
{
  //  display the next (snapped) position where editing would start
  db::DPoint pp = snap (p);
  std::string pos = std::string ("x: ") + tl::micron_to_string (pp.x ()) + 
                    std::string ("  y: ") + tl::micron_to_string (pp.y ());
  view ()->message (pos);
}

void 
ShapeEditService::deliver_shape (const db::Polygon &poly)
{
  if (m_combine_mode == CM_Add) {

    db::Transaction transaction (manager (), tl::to_string (tr ("Create polygon")));
    cell ().shapes (layer ()).insert (poly);

  } else {

    std::vector<db::Shape> shapes;
    std::vector<db::Polygon> result;

    std::vector<db::Polygon> input;
    input.push_back (poly);

    std::vector<db::Polygon> input_left;
    if (m_combine_mode == CM_Diff) {
      input_left = input;
    }

    db::EdgeProcessor ep;
    bool any = false;

    db::ShapeIterator s = cell ().shapes (layer ()).begin_touching (poly.box (), db::ShapeIterator::Polygons | db::ShapeIterator::Paths | db::ShapeIterator::Boxes);
    while (! s.at_end ()) {

      std::vector<db::Polygon> subject;
      subject.push_back (db::Polygon ());
      s->polygon (subject.back ());

      if (db::interact_pp (poly, subject.back ())) {

        any = true;

        if (m_combine_mode == CM_Merge) {
          ep.boolean (subject, input, result, db::BooleanOp::Or);
          input = result;
          input_left.clear ();
          input_left.swap (result);
        } else if (m_combine_mode == CM_Erase) {
          ep.boolean (subject, input, result, db::BooleanOp::ANotB);
        } else if (m_combine_mode == CM_Mask) {
          ep.boolean (subject, input, result, db::BooleanOp::And);
        } else if (m_combine_mode == CM_Diff) {
          ep.boolean (subject, input, result, db::BooleanOp::ANotB);
          std::vector<db::Polygon> l;
          ep.boolean (input_left, subject, l, db::BooleanOp::ANotB);
          l.swap (input_left);
        }

        shapes.push_back (*s);

      } 

      ++s;

    }

    //  If nothing was found, simply pass the input to the result
    if (! any && (m_combine_mode == CM_Merge || m_combine_mode == CM_Diff)) {
      result = input;
    }

    db::Transaction transaction (manager (), tl::to_string (tr ("Combine shape with background")));

    //  Erase existing shapes
    for (std::vector<db::Shape>::const_iterator s = shapes.begin (); s != shapes.end (); ++s) {
      cell ().shapes (layer ()).erase_shape (*s);
    }

    //  Add new shapes
    for (std::vector<db::Polygon>::const_iterator p = result.begin (); p != result.end (); ++p) {
      cell ().shapes (layer ()).insert (*p);
    }
    for (std::vector<db::Polygon>::const_iterator p = input_left.begin (); p != input_left.end (); ++p) {
      cell ().shapes (layer ()).insert (*p);
    }

  }
}

void 
ShapeEditService::deliver_shape (const db::Path &path)
{
  if (m_combine_mode == CM_Add) {
    db::Transaction transaction (manager (), tl::to_string (tr ("Create path")));
    cell ().shapes (layer ()).insert (path);
  } else {
    deliver_shape (path.polygon ());
  }
}

void 
ShapeEditService::deliver_shape (const db::Box &box)
{
  if (m_combine_mode == CM_Add) {
    db::Transaction transaction (manager (), tl::to_string (tr ("Create box")));
    cell ().shapes (layer ()).insert (box);
  } else {
    deliver_shape (db::Polygon (box));
  }
}

void
ShapeEditService::deliver_shape (const db::Point &point)
{
  if (m_combine_mode == CM_Add) {
    db::Transaction transaction (manager (), tl::to_string (tr ("Create point")));
    cell ().shapes (layer ()).insert (point);
  }
}

void
ShapeEditService::open_editor_hooks ()
{
  std::string technology;
  if (mp_layout && mp_layout->technology ()) {
    technology = mp_layout->technology ()->name ();
  }

  m_editor_hooks = edt::EditorHooks::get_editor_hooks (technology);

  lay::CellViewRef cv_ref (view ()->cellview_ref (m_cv_index));
  call_editor_hooks<lay::CellViewRef &, const lay::LayerProperties &> (m_editor_hooks, &edt::EditorHooks::begin_create_shapes, cv_ref, *view ()->current_layer ());
}

void
ShapeEditService::close_editor_hooks (bool with_commit)
{
  if (with_commit) {
    call_editor_hooks (m_editor_hooks, &edt::EditorHooks::commit_shapes);
  }
  call_editor_hooks (m_editor_hooks, &edt::EditorHooks::end_create_shapes);

  m_editor_hooks.clear ();
}

template <class Shape>
void
ShapeEditService::deliver_shape_to_hooks (const Shape &shape)
{
  db::Shapes tmp (true);
  db::Shape s = tmp.insert (shape);
  call_editor_hooks<const db::Shape &, const db::CplxTrans &> (m_editor_hooks, &edt::EditorHooks::create_shape, s, trans ().inverted ());
}

//  explicit instantiations
template void ShapeEditService::deliver_shape_to_hooks<db::Polygon> (const db::Polygon &);
template void ShapeEditService::deliver_shape_to_hooks<db::Path> (const db::Path &);
template void ShapeEditService::deliver_shape_to_hooks<db::Box> (const db::Box &);
template void ShapeEditService::deliver_shape_to_hooks<db::Point> (const db::Point &);
template void ShapeEditService::deliver_shape_to_hooks<db::Text> (const db::Text &);

} // namespace edt

