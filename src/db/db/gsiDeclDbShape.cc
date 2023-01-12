
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


#include "gsiDecl.h"
#include "dbShape.h"
#include "dbShapes.h"
#include "dbLayout.h"

namespace gsi
{

// ---------------------------------------------------------------
//  db::Shape binding

static db::Layout *layout_ptr (db::Shape *s)
{
  db::Shapes *shapes = s->shapes ();
  return shapes ? shapes->layout () : 0;
}

static const db::Layout *layout_ptr_const (const db::Shape *s)
{
  const db::Shapes *shapes = s->shapes ();
  return shapes ? shapes->layout () : 0;
}

static double shape_dbu (const db::Shape *s)
{
  const db::Layout *layout = layout_ptr_const (s);
  if (! layout) {
    throw tl::Exception (tl::to_string (tr ("Shape does not reside inside a layout - cannot obtain database unit")));
  }
  return layout->dbu ();
}

static db::Shapes *shapes_checked (db::Shape *s)
{
  db::Shapes *shapes = s->shapes ();
  if (! shapes) {
    throw tl::Exception (tl::to_string (tr ("Shape does not reside inside a shape container - cannot change the shape")));
  }
  return shapes;
}

static void check_is_path (const db::Shape *s)
{
  if (! s->is_path ()) {
    throw tl::Exception (tl::to_string (tr ("Shape is not a path")));
  }
}

static void check_is_text (const db::Shape *s)
{
  if (! s->is_text ()) {
    throw tl::Exception (tl::to_string (tr ("Shape is not a text")));
  }
}

static void check_is_box (const db::Shape *s)
{
  if (! s->is_box ()) {
    throw tl::Exception (tl::to_string (tr ("Shape is not a box")));
  }
}

static void transform_shape (db::Shape *s, const db::Trans &trans)
{
  db::Shapes *shapes = shapes_checked (s);
  *s = shapes->transform (*s, trans);
}

static void transform_shape_dtrans (db::Shape *s, const db::DTrans &trans)
{
  db::CplxTrans dbu_trans (shape_dbu (s));
  db::Shapes *shapes = shapes_checked (s);
  *s = shapes->transform (*s, dbu_trans.inverted () * db::DCplxTrans (trans) * dbu_trans);
}

static void transform_shape_icplx (db::Shape *s, const db::ICplxTrans &trans)
{
  db::Shapes *shapes = shapes_checked (s);
  *s = shapes->transform (*s, trans);
}

static void transform_shape_dcplx (db::Shape *s, const db::DCplxTrans &trans)
{
  db::CplxTrans dbu_trans (shape_dbu (s));
  db::Shapes *shapes = shapes_checked (s);
  *s = shapes->transform (*s, dbu_trans.inverted () * trans * dbu_trans);
}

static void delete_shape (db::Shape *s)
{
  db::Shapes *shapes = shapes_checked (s);
  shapes->erase_shape (*s);
  *s = db::Shape ();
}

static bool shape_is_valid (const db::Shape *s)
{
  db::Shapes *shapes = s->shapes ();
  return shapes != 0 && shapes->is_valid (*s);
}

static int object_type (const db::Shape *s)
{
  return int (s->type ());
}

static std::string text_string (const db::Shape *s)
{
  check_is_text (s);
  return s->text_string ();
}

static void set_text_string (db::Shape *s, const std::string &t)
{
  check_is_text (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::text_type p;
  s->text (p);
  p.string (t);
  *s = shapes->replace (*s, p);
}

static db::Vector text_pos (const db::Shape *s)
{
  check_is_text (s);
  db::Shape::text_type p;
  s->text (p);
  return p.trans ().disp ();
}

static db::DVector text_dpos (const db::Shape *s)
{
  check_is_text (s);
  db::Shape::text_type p;
  s->text (p);
  return db::CplxTrans (shape_dbu (s)) * p.trans ().disp ();
}

static void set_text_pos (db::Shape *s, const db::Vector &q)
{
  check_is_text (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::text_type p;
  s->text (p);
  p.trans (db::Trans (p.trans ().rot (), q));
  *s = shapes->replace (*s, p);
}

static void set_text_dpos (db::Shape *s, const db::DVector &q)
{
  check_is_text (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::text_type p;
  s->text (p);
  p.trans (db::Trans (p.trans ().rot (), db::CplxTrans (shape_dbu (s)).inverted () * q));
  *s = shapes->replace (*s, p);
}

static int text_rot (const db::Shape *s)
{
  check_is_text (s);
  db::Shape::text_type p;
  s->text (p);
  return p.trans ().rot ();
}

static void set_text_rot (db::Shape *s, int rot) 
{
  check_is_text (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::text_type p;
  s->text (p);
  p.trans (db::Trans (rot, p.trans ().disp ()));
  *s = shapes->replace (*s, p);
}

static db::Trans text_trans (const db::Shape *s)
{
  check_is_text (s);
  return s->text_trans ();
}

static db::DTrans text_dtrans (const db::Shape *s)
{
  db::CplxTrans dbu_trans (shape_dbu (s));
  check_is_text (s);
  return db::DTrans (dbu_trans * db::ICplxTrans (s->text_trans ()) * dbu_trans.inverted ());
}

static void set_text_trans (db::Shape *s, const db::Trans &t)
{
  check_is_text (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::text_type p;
  s->text (p);
  p.trans (t);
  *s = shapes->replace (*s, p);
}

static void set_text_dtrans (db::Shape *s, const db::DTrans &t)
{
  db::CplxTrans dbu_trans (shape_dbu (s));
  set_text_trans (s, db::Trans (dbu_trans.inverted () * db::DCplxTrans (t) * dbu_trans));
}

static db::Shape::coord_type text_size (const db::Shape *s)
{
  check_is_text (s);
  return s->text_size ();
}

static db::DCoord text_dsize (const db::Shape *s)
{
  check_is_text (s);
  return s->text_size () * shape_dbu (s);
}

static void set_text_size (db::Shape *s, db::Shape::coord_type t)
{
  check_is_text (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::text_type p;
  s->text (p);
  p.size (t);
  *s = shapes->replace (*s, p);
}

static void set_text_dsize (db::Shape *s, db::DCoord dt)
{
  db::Coord t = db::coord_traits<db::Coord>::rounded (dt / shape_dbu (s));
  check_is_text (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::text_type p;
  s->text (p);
  p.size (t);
  *s = shapes->replace (*s, p);
}

static int text_font (const db::Shape *s)
{
  check_is_text (s);
  return int (s->text_font ());
}

static void set_text_font (db::Shape *s, int f)
{
  check_is_text (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::text_type p;
  s->text (p);
  p.font (db::Font (f));
  *s = shapes->replace (*s, p);
}

static int text_halign (const db::Shape *s)
{
  check_is_text (s);
  return int (s->text_halign ());
}

static void set_text_halign (db::Shape *s, int a)
{
  check_is_text (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::text_type p;
  s->text (p);
  p.halign (db::HAlign (a));
  *s = shapes->replace (*s, p);
}

static int text_valign (const db::Shape *s)
{
  check_is_text (s);
  return int (s->text_valign ());
}

static void set_text_valign (db::Shape *s, int a)
{
  check_is_text (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::text_type p;
  s->text (p);
  p.valign (db::VAlign (a));
  *s = shapes->replace (*s, p);
}

static db::Shape::coord_type path_bgnext (const db::Shape *s)
{
  check_is_path (s);
  return s->path_extensions ().first;
}

static db::DCoord path_dbgnext (const db::Shape *s)
{
  check_is_path (s);
  return s->path_extensions ().first * shape_dbu (s);
}

static void set_path_bgnext (db::Shape *s, db::Shape::coord_type e)
{
  check_is_path (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::path_type p;
  s->path (p);
  p.bgn_ext (e);
  *s = shapes->replace (*s, p);
}

static void set_path_dbgnext (db::Shape *s, db::DCoord de)
{
  db::Coord e = db::coord_traits<db::Coord>::rounded (de / shape_dbu (s));
  check_is_path (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::path_type p;
  s->path (p);
  p.bgn_ext (e);
  *s = shapes->replace (*s, p);
}

static db::Shape::coord_type path_endext (const db::Shape *s)
{
  check_is_path (s);
  return s->path_extensions ().second;
}

static db::DCoord path_dendext (const db::Shape *s)
{
  check_is_path (s);
  return s->path_extensions ().second * shape_dbu (s);
}

static void set_path_endext (db::Shape *s, db::Shape::coord_type e)
{
  check_is_path (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::path_type p;
  s->path (p);
  p.end_ext (e);
  *s = shapes->replace (*s, p);
}

static void set_path_dendext (db::Shape *s, db::DCoord de)
{
  db::Coord e = db::coord_traits<db::Coord>::rounded (de / shape_dbu (s));
  check_is_path (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::path_type p;
  s->path (p);
  p.end_ext (e);
  *s = shapes->replace (*s, p);
}

static bool round_path (const db::Shape *s)
{
  check_is_path (s);
  return s->round_path ();
}

static void set_round_path (db::Shape *s, bool r)
{
  check_is_path (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::path_type p;
  s->path (p);
  p.round (r);
  *s = shapes->replace (*s, p);
}

static db::Shape::coord_type path_length (const db::Shape *s)
{
  check_is_path (s);
  return s->path_length ();
}

static db::DCoord path_dlength (const db::Shape *s)
{
  check_is_path (s);
  return s->path_length () * shape_dbu (s);
}

static db::Shape::coord_type path_width (const db::Shape *s)
{
  check_is_path (s);
  return s->path_width ();
}

static db::DCoord path_dwidth (const db::Shape *s)
{
  check_is_path (s);
  return s->path_width () * shape_dbu (s);
}

static void set_path_width (db::Shape *s, db::Shape::coord_type w)
{
  check_is_path (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::path_type p;
  s->path (p);
  p.width (w);
  *s = shapes->replace (*s, p);
}

static void set_path_dwidth (db::Shape *s, db::DCoord dw)
{
  db::Coord w = db::coord_traits<db::Coord>::rounded (dw / shape_dbu (s));
  check_is_path (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::path_type p;
  s->path (p);
  p.width (w);
  *s = shapes->replace (*s, p);
}

static db::Coord box_width (const db::Shape *s)
{
  check_is_box (s);
  return s->box ().width ();
}

static db::DCoord box_dwidth (const db::Shape *s)
{
  check_is_box (s);
  return s->box ().width () * shape_dbu (s);
}

static void set_box_width (db::Shape *s, db::Coord w)
{
  check_is_box (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::box_type p;
  s->box (p);
  *s = shapes->replace (*s, db::Shape::box_type (p.center ().x () - w / 2, p.p1 ().y (), p.center ().x () + (w - w / 2), p. p2 ().y ()));
}

static void set_box_dwidth (db::Shape *s, db::DCoord dw)
{
  db::Coord w = db::coord_traits<db::Coord>::rounded (dw / shape_dbu (s));
  check_is_box (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::box_type p;
  s->box (p);
  *s = shapes->replace (*s, db::Shape::box_type (p.center ().x () - w / 2, p.p1 ().y (), p.center ().x () + (w - w / 2), p. p2 ().y ()));
}

static db::Coord box_height (const db::Shape *s)
{
  check_is_box (s);
  return s->box ().height ();
}

static db::DCoord box_dheight (const db::Shape *s)
{
  check_is_box (s);
  return s->box ().height () * shape_dbu (s);
}

static void set_box_height (db::Shape *s, db::Coord h)
{
  check_is_box (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::box_type p;
  s->box (p);
  *s = shapes->replace (*s, db::Shape::box_type (p.p1 ().x (), p.center ().y () - h / 2, p.p2 ().x (), p.center ().y () + (h - h / 2)));
}

static void set_box_dheight (db::Shape *s, db::DCoord dh)
{
  db::Coord h = db::coord_traits<db::Coord>::rounded (dh / shape_dbu (s));
  check_is_box (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::box_type p;
  s->box (p);
  *s = shapes->replace (*s, db::Shape::box_type (p.p1 ().x (), p.center ().y () - h / 2, p.p2 ().x (), p.center ().y () + (h - h / 2)));
}

static db::Point box_center (const db::Shape *s)
{
  check_is_box (s);
  return s->box ().center ();
}

static db::DPoint box_dcenter (const db::Shape *s)
{
  check_is_box (s);
  return s->box ().center () * shape_dbu (s);
}

static void set_box_center (db::Shape *s, const db::Point &c)
{
  check_is_box (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::box_type p;
  s->box (p);
  *s = shapes->replace (*s, p.moved (c - p.center ())); 
}

static void set_box_dcenter (db::Shape *s, const db::DPoint &dc)
{
  db::Point c = db::CplxTrans (shape_dbu (s)).inverted () * dc;
  check_is_box (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::box_type p;
  s->box (p);
  *s = shapes->replace (*s, p.moved (c - p.center ()));
}

static db::Point box_p1 (const db::Shape *s)
{
  check_is_box (s);
  return s->box ().p1 ();
}

static db::DPoint box_dp1 (const db::Shape *s)
{
  check_is_box (s);
  return s->box ().p1 () * shape_dbu (s);
}

static void set_box_p1 (db::Shape *s, const db::Point &p1)
{
  check_is_box (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::box_type p;
  s->box (p);
  *s = shapes->replace (*s, db::Shape::box_type (p1, p.p2 ()));
}

static void set_box_dp1 (db::Shape *s, const db::DPoint &dp1)
{
  db::Point p1 = db::CplxTrans (shape_dbu (s)).inverted () * dp1;
  check_is_box (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::box_type p;
  s->box (p);
  *s = shapes->replace (*s, db::Shape::box_type (p1, p.p2 ()));
}

static db::Point box_p2 (const db::Shape *s)
{
  check_is_box (s);
  return s->box ().p2 ();
}

static db::DPoint box_dp2 (const db::Shape *s)
{
  check_is_box (s);
  return s->box ().p2 () * shape_dbu (s);
}

static void set_box_p2 (db::Shape *s, const db::Point &p2)
{
  check_is_box (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::box_type p;
  s->box (p);
  *s = shapes->replace (*s, db::Shape::box_type (p.p1 (), p2));
}

static void set_box_dp2 (db::Shape *s, const db::DPoint &dp2)
{
  db::Point p2 = db::CplxTrans (shape_dbu (s)).inverted () * dp2;
  check_is_box (s);
  db::Shapes *shapes = shapes_checked (s);
  db::Shape::box_type p;
  s->box (p);
  *s = shapes->replace (*s, db::Shape::box_type (p.p1 (), p2));
}

static tl::Variant get_path (const db::Shape *s)
{
  db::Shape::path_type p;
  if (s->path (p)) {
    return tl::Variant (p);
  } else {
    return tl::Variant ();
  }
}

static tl::Variant get_dpath (const db::Shape *s)
{
  db::Shape::path_type p;
  if (s->path (p)) {
    return tl::Variant (db::CplxTrans (shape_dbu (s)) * p);
  } else {
    return tl::Variant ();
  }
}

static tl::Variant get_polygon (const db::Shape *s)
{
  db::Shape::polygon_type p;
  if (s->polygon (p)) {
    return tl::Variant (p);
  } else {
    return tl::Variant ();
  }
}

static tl::Variant get_dpolygon (const db::Shape *s)
{
  db::Shape::polygon_type p;
  if (s->polygon (p)) {
    return tl::Variant (db::CplxTrans (shape_dbu (s)) * p);
  } else {
    return tl::Variant ();
  }
}

static tl::Variant get_simple_polygon (const db::Shape *s)
{
  db::Shape::simple_polygon_type p;
  if (s->simple_polygon (p)) {
    return tl::Variant (p);
  } else {
    return tl::Variant ();
  }
}

static tl::Variant get_dsimple_polygon (const db::Shape *s)
{
  db::Shape::simple_polygon_type p;
  if (s->simple_polygon (p)) {
    return tl::Variant (db::CplxTrans (shape_dbu (s)) * p);
  } else {
    return tl::Variant ();
  }
}

static tl::Variant get_box (const db::Shape *s)
{
  db::Shape::box_type p;
  if (s->box (p)) {
    return tl::Variant (p);
  } else {
    return tl::Variant ();
  }
}

static tl::Variant get_dbox (const db::Shape *s)
{
  db::Shape::box_type p;
  if (s->box (p)) {
    return tl::Variant (db::CplxTrans (shape_dbu (s)) * p);
  } else {
    return tl::Variant ();
  }
}

static tl::Variant get_edge (const db::Shape *s)
{
  db::Shape::edge_type p;
  if (s->edge (p)) {
    return tl::Variant (p);
  } else {
    return tl::Variant ();
  }
}

static tl::Variant get_dedge (const db::Shape *s)
{
  db::Shape::edge_type p;
  if (s->edge (p)) {
    return tl::Variant (db::CplxTrans (shape_dbu (s)) * p);
  } else {
    return tl::Variant ();
  }
}

static tl::Variant get_edge_pair (const db::Shape *s)
{
  db::Shape::edge_pair_type p;
  if (s->edge_pair (p)) {
    return tl::Variant (p);
  } else {
    return tl::Variant ();
  }
}

static tl::Variant get_dedge_pair (const db::Shape *s)
{
  db::Shape::edge_pair_type p;
  if (s->edge_pair (p)) {
    return tl::Variant (db::CplxTrans (shape_dbu (s)) * p);
  } else {
    return tl::Variant ();
  }
}

static tl::Variant get_point (const db::Shape *s)
{
  db::Shape::point_type p;
  if (s->point (p)) {
    return tl::Variant (p);
  } else {
    return tl::Variant ();
  }
}

static tl::Variant get_dpoint (const db::Shape *s)
{
  db::Shape::point_type p;
  if (s->point (p)) {
    return tl::Variant (db::CplxTrans (shape_dbu (s)) * p);
  } else {
    return tl::Variant ();
  }
}

static tl::Variant get_text (const db::Shape *s)
{
  db::Shape::text_type p;
  if (s->text (p)) {
    return tl::Variant (p);
  } else {
    return tl::Variant ();
  }
}

static tl::Variant get_dtext (const db::Shape *s)
{
  db::Shape::text_type p;
  if (s->text (p)) {
    return tl::Variant (db::CplxTrans (shape_dbu (s)) * p);
  } else {
    return tl::Variant ();
  }
}

static void set_prop_id (db::Shape *s, db::properties_id_type id)
{
  db::Shapes *shapes = shapes_checked (s);
  *s = shapes->replace_prop_id (*s, id);
}

static db::Shapes *shapes_ptr (db::Shape *s)
{
  return s->shapes ();
}

static unsigned int shape_layer_index (const db::Shape *s)
{
  db::Shapes *shapes = s->shapes ();
  if (! shapes) {
    throw tl::Exception (tl::to_string (tr ("Shape does not belong to a shape container")));
  }

  db::Cell *cell = shapes->cell ();
  if (! cell) {
    throw tl::Exception (tl::to_string (tr ("Shape does not belong to a cell")));
  }

  const db::Layout *layout = cell->layout ();
  if (! layout) {
    throw tl::Exception (tl::to_string (tr ("Shape does not belong to a layout")));
  }

  for (db::Layout::layer_iterator l = layout->begin_layers (); l != layout->end_layers (); ++l) {
    if (& cell->shapes ((*l).first) == shapes) {
      return (*l).first;
    }
  }

  throw tl::Exception (tl::to_string (tr ("Cannot identify layer of shape")));
}

static void set_shape_layer_index (db::Shape *s, unsigned int layer)
{
  db::Shapes *shapes = s->shapes ();
  if (! shapes) {
    throw tl::Exception (tl::to_string (tr ("Shape does not belong to a shape container")));
  }

  db::Cell *cell = shapes->cell ();
  if (! cell) {
    throw tl::Exception (tl::to_string (tr ("Shape does not belong to a cell")));
  }

  const db::Layout *layout = cell->layout ();
  if (! layout) {
    throw tl::Exception (tl::to_string (tr ("Shape does not belong to a layout")));
  }

  if (! layout->is_valid_layer (layer)) {
    throw tl::Exception (tl::to_string (tr ("Layer index does not point to a valid layer")));
  }

  if (& cell->shapes (layer) != shapes) {
    db::Shape s_old = *s;
    *s = cell->shapes (layer).insert (s_old);
    shapes->erase_shape (s_old);
  }
}

static db::LayerProperties shape_layer (const db::Shape *s)
{
  db::Shapes *shapes = s->shapes ();
  if (! shapes) {
    throw tl::Exception (tl::to_string (tr ("Shape does not belong to a shape container")));
  }

  db::Cell *cell = shapes->cell ();
  if (! cell) {
    throw tl::Exception (tl::to_string (tr ("Shape does not belong to a cell")));
  }

  const db::Layout *layout = cell->layout ();
  if (! layout) {
    throw tl::Exception (tl::to_string (tr ("Shape does not belong to a layout")));
  }

  for (db::Layout::layer_iterator l = layout->begin_layers (); l != layout->end_layers (); ++l) {
    if (& cell->shapes ((*l).first) == shapes) {
      return *(*l).second;
    }
  }

  throw tl::Exception (tl::to_string (tr ("Cannot identify layer of shape")));
}

static void set_shape_layer (db::Shape *s, const db::LayerProperties &lp)
{
  db::Shapes *shapes = s->shapes ();
  if (! shapes) {
    throw tl::Exception (tl::to_string (tr ("Shape does not belong to a shape container")));
  }

  db::Cell *cell = shapes->cell ();
  if (! cell) {
    throw tl::Exception (tl::to_string (tr ("Shape does not belong to a cell")));
  }

  const db::Layout *layout = cell->layout ();
  if (! layout) {
    throw tl::Exception (tl::to_string (tr ("Shape does not belong to a layout")));
  }

  for (db::Layout::layer_iterator l = layout->begin_layers (); l != layout->end_layers (); ++l) {

    if ((*l).second->log_equal (lp)) {

      unsigned int layer = (*l).first;
      if (& cell->shapes (layer) != shapes) {
        db::Shape s_old = *s;
        *s = cell->shapes (layer).insert (s_old);
        shapes->erase_shape (s_old);
      }

      return;

    }

  }

  throw tl::Exception (tl::to_string (tr ("Layer info object is not giving a valid layer")));
}

static db::Cell *cell_ptr (db::Shape *s)
{
  db::Shapes *shapes = s->shapes ();
  return shapes ? shapes->cell () : 0;
}

static void set_cell_ptr (db::Shape *s, db::Cell *new_cell)
{
  db::Shapes *shapes = shapes_checked (s);
  if (layout_ptr (s) != new_cell->layout ()) {
    throw tl::Exception (tl::to_string (tr ("Current and new cell belong to a different layout")));
  }

  unsigned int l = shape_layer_index (s);

  db::Shape s_old = *s;
  *s = new_cell->shapes (l).insert (s_old);
  shapes->erase_shape (s_old);
}

template <class SH>
void set_shape (db::Shape *s, const SH &obj)
{
  db::Shapes *shapes = shapes_checked (s);
  *s = shapes->replace (*s, obj);
}

template <class SH>
void set_dshape (db::Shape *s, const SH &obj)
{
  db::CplxTrans dbu_trans (shape_dbu (s));
  db::Shapes *shapes = shapes_checked (s);
  *s = shapes->replace (*s, dbu_trans.inverted () * obj);
}

static void delete_property (db::Shape *s, const tl::Variant &key)
{
  db::properties_id_type id = s->prop_id ();
  if (id == 0) {
    return;
  }

  db::Layout *layout = layout_ptr (s);
  if (! layout) {
    throw tl::Exception (tl::to_string (tr ("Shape does not reside inside a layout - cannot delete properties")));
  }

  std::pair<bool, db::property_names_id_type> nid = layout->properties_repository ().get_id_of_name (key);
  if (! nid.first) {
    return;
  }

  db::PropertiesRepository::properties_set props = layout->properties_repository ().properties (id);
  db::PropertiesRepository::properties_set::iterator p = props.find (nid.second);
  if (p != props.end ()) {
    props.erase (p);
  }
  set_prop_id (s, layout->properties_repository ().properties_id (props));
}

static void set_property (db::Shape *s, const tl::Variant &key, const tl::Variant &value)
{
  db::properties_id_type id = s->prop_id ();

  db::Layout *layout = layout_ptr (s);
  if (! layout) {
    throw tl::Exception (tl::to_string (tr ("Shape does not reside inside a layout - cannot set properties")));
  }

  db::property_names_id_type nid = layout->properties_repository ().prop_name_id (key);

  db::PropertiesRepository::properties_set props = layout->properties_repository ().properties (id);
  db::PropertiesRepository::properties_set::iterator p = props.find (nid);
  if (p != props.end ()) {
    p->second = value;
  } else {
    props.insert (std::make_pair (nid, value));
  }
  set_prop_id (s, layout->properties_repository ().properties_id (props));
}

static tl::Variant get_property (const db::Shape *s, const tl::Variant &key)
{
  db::properties_id_type id = s->prop_id ();
  if (id == 0) {
    return tl::Variant ();
  }

  const db::Layout *layout = layout_ptr_const (s);
  if (! layout) {
    throw tl::Exception (tl::to_string (tr ("Shape does not reside inside a layout - cannot retrieve properties")));
  }

  std::pair<bool, db::property_names_id_type> nid = layout->properties_repository ().get_id_of_name (key);
  if (! nid.first) {
    return tl::Variant ();
  }

  const db::PropertiesRepository::properties_set &props = layout->properties_repository ().properties (id);
  db::PropertiesRepository::properties_set::const_iterator p = props.find (nid.second);
  if (p != props.end ()) {
    return p->second;
  } else {
    return tl::Variant ();
  }
}

namespace
{

template <class I, class T>
struct ConvertingIteratorWrapper
{
  typedef void difference_type;
  typedef T value_type;
  typedef T reference;
  typedef void pointer;
  typedef std::forward_iterator_tag iterator_category;

  ConvertingIteratorWrapper (double dbu, const I &b, const I &e)
    : m_b (b), m_e (e), m_dbu (dbu)
  {
    //  .. nothing yet ..
  }

  void operator++ ()
  {
    ++m_b;
  }

  reference operator* () const
  {
    return *m_b * m_dbu;
  }

  bool at_end () const
  {
    return m_b == m_e;
  }

private:
  I m_b, m_e;
  double m_dbu;
};

template <class I, class T>
struct ConvertingFreeIteratorWrapper
{
  typedef void difference_type;
  typedef T value_type;
  typedef T reference;
  typedef void pointer;
  typedef std::forward_iterator_tag iterator_category;

  ConvertingFreeIteratorWrapper (double dbu, const I &b)
    : m_b (b), m_dbu (dbu)
  {
    //  .. nothing yet ..
  }

  void operator++ ()
  {
    ++m_b;
  }

  reference operator* () const
  {
    return *m_b * m_dbu;
  }

  bool at_end () const
  {
    return m_b.at_end ();
  }

private:
  I m_b;
  double m_dbu;
};

}

static ConvertingIteratorWrapper<db::Shape::point_iterator, db::DPoint> shape_dpoint_iter (const db::Shape *s)
{
  return ConvertingIteratorWrapper<db::Shape::point_iterator, db::DPoint> (shape_dbu (s), s->begin_point (), s->end_point ());
}

static ConvertingIteratorWrapper<db::Shape::point_iterator, db::DPoint> shape_dpoint_hull_iter (const db::Shape *s)
{
  return ConvertingIteratorWrapper<db::Shape::point_iterator, db::DPoint> (shape_dbu (s), s->begin_hull (), s->end_hull ());
}

static ConvertingIteratorWrapper<db::Shape::point_iterator, db::DPoint> shape_dpoint_hole_iter (const db::Shape *s, unsigned int hole)
{
  return ConvertingIteratorWrapper<db::Shape::point_iterator, db::DPoint> (shape_dbu (s), s->begin_hole (hole), s->end_hole (hole));
}

static ConvertingFreeIteratorWrapper<db::Shape::polygon_edge_iterator, db::DEdge> shape_edge_iter (const db::Shape *s)
{
  return ConvertingFreeIteratorWrapper<db::Shape::polygon_edge_iterator, db::DEdge> (shape_dbu (s), s->begin_edge ());
}

static ConvertingFreeIteratorWrapper<db::Shape::polygon_edge_iterator, db::DEdge> shape_edge_iter_per_contour (const db::Shape *s, unsigned int contour)
{
  return ConvertingFreeIteratorWrapper<db::Shape::polygon_edge_iterator, db::DEdge> (shape_dbu (s), s->begin_edge (contour));
}

static db::DTrans array_dtrans (const db::Shape *s)
{
  db::CplxTrans dbu_trans (shape_dbu (s));
  return db::DTrans (dbu_trans * db::ICplxTrans (s->array_trans ()) * dbu_trans.inverted ());
}

static db::DBox dbbox (const db::Shape *s)
{
  return db::CplxTrans (shape_dbu (s)) * s->bbox ();
}

#if defined(HAVE_64BIT_COORD)
//  workaround for missing 128bit binding of GSI
static double area (const db::Shape *shape)
#else
static db::Shape::area_type area (const db::Shape *shape)
#endif
{ 
  return shape->area ();
}

static double darea (const db::Shape *shape)
{
  double dbu = shape_dbu (shape);
  return double (shape->area ()) * dbu * dbu;
}

static double dperimeter (const db::Shape *shape)
{
  double dbu = shape_dbu (shape);
  return double (shape->perimeter ()) * dbu;
}

static int t_null ()                        { return db::Shape::Null; }
static int t_polygon ()                     { return db::Shape::Polygon; }
static int t_polygonRef ()                  { return db::Shape::PolygonRef; }
static int t_polygonPtrArray ()             { return db::Shape::PolygonPtrArray; }
static int t_polygonPtrArrayMember ()       { return db::Shape::PolygonPtrArrayMember; }
static int t_simplePolygon ()               { return db::Shape::SimplePolygon; }
static int t_simplePolygonRef ()            { return db::Shape::SimplePolygonRef; }
static int t_simplePolygonPtrArray ()       { return db::Shape::SimplePolygonPtrArray; }
static int t_simplePolygonPtrArrayMember () { return db::Shape::SimplePolygonPtrArrayMember; }
static int t_edge ()                        { return db::Shape::Edge; }
static int t_edge_pair ()                   { return db::Shape::EdgePair; }
static int t_point ()                       { return db::Shape::Point; }
static int t_path ()                        { return db::Shape::Path; }
static int t_pathRef ()                     { return db::Shape::PathRef; }
static int t_pathPtrArray ()                { return db::Shape::PathPtrArray; }
static int t_pathPtrArrayMember ()          { return db::Shape::PathPtrArrayMember; }
static int t_box ()                         { return db::Shape::Box; }
static int t_boxArray ()                    { return db::Shape::BoxArray; }
static int t_boxArrayMember ()              { return db::Shape::BoxArrayMember; }
static int t_shortBox ()                    { return db::Shape::ShortBox; }
static int t_shortBoxArray ()               { return db::Shape::ShortBoxArray; }
static int t_shortBoxArrayMember ()         { return db::Shape::ShortBoxArrayMember; }
static int t_text ()                        { return db::Shape::Text; }
static int t_textRef ()                     { return db::Shape::TextRef; }
static int t_textPtrArray ()                { return db::Shape::TextPtrArray; }
static int t_textPtrArrayMember ()          { return db::Shape::TextPtrArrayMember; }
static int t_userObject ()                  { return db::Shape::UserObject; }

Class<db::Shape> decl_Shape ("db", "Shape",
  gsi::method ("prop_id", (db::properties_id_type (db::Shape::*) () const) &db::Shape::prop_id,
    "@brief Gets the properties ID associated with the shape\n"
    "\n"
    "The \\Layout object can be used to retrieve the actual properties associated with the ID."
  ) +
  gsi::method_ext ("prop_id=", &set_prop_id,
    "@brief Sets the properties ID of this shape\n"
    "\n"
    "The \\Layout object can be used to retrieve an ID for a given set of properties. "
    "Calling this method will invalidate any iterators. It should not be called inside a "
    "loop iterating over shapes.\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  gsi::method ("has_prop_id?", &db::Shape::has_prop_id,
    "@brief Returns true, if the shape has properties, i.e. has a properties ID\n"
  ) +
  gsi::method_ext ("shapes", &shapes_ptr,
    "@brief Gets a reference to the Shapes container the shape lives in\n"
    "\n"
    "This reference can be nil, if the Shape object is not referring to an actual shape.\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  gsi::method_ext ("cell", &cell_ptr,
    "@brief Gets a reference to the cell the shape belongs to\n"
    "\n"
    "This reference can be nil, if the Shape object is not living inside a cell\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  gsi::method_ext ("cell=", &set_cell_ptr, gsi::arg ("cell"),
    "@brief Moves the shape to a different cell\n"
    "\n"
    "Both the current and the target cell must reside in the same layout.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method_ext ("layout", &layout_ptr,
    "@brief Gets a reference to the Layout the shape belongs to\n"
    "\n"
    "This reference can be nil, if the Shape object is not living inside a layout.\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) +
  gsi::method_ext ("box=", &set_shape<db::Box>, gsi::arg ("box"),
    "@brief Replaces the shape by the given box\n"
    "This method replaces the shape by the given box. This method can only be called "
    "for editable layouts. It does not change the user properties of the shape.\n"
    "Calling this method will invalidate any iterators. It should not be called inside a "
    "loop iterating over shapes.\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) + 
  gsi::method_ext ("box=|dbox=", &set_dshape<db::DBox>, gsi::arg("box"),
    "@brief Replaces the shape by the given box (in micrometer units)\n"
    "This method replaces the shape by the given box, like \\box= with a \\Box argument does. "
    "This version translates the box from micrometer units to database units internally.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method_ext ("path=", &set_shape<db::Path>, gsi::arg ("box"),
    "@brief Replaces the shape by the given path object\n"
    "This method replaces the shape by the given path object. This method can only be called "
    "for editable layouts. It does not change the user properties of the shape.\n"
    "Calling this method will invalidate any iterators. It should not be called inside a "
    "loop iterating over shapes.\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) + 
  gsi::method_ext ("path=|dpath=", &set_dshape<db::DPath>, gsi::arg("path"),
    "@brief Replaces the shape by the given path (in micrometer units)\n"
    "This method replaces the shape by the given path, like \\path= with a \\Path argument does. "
    "This version translates the path from micrometer units to database units internally.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method_ext ("polygon=", &set_shape<db::Polygon>, gsi::arg ("box"),
    "@brief Replaces the shape by the given polygon object\n"
    "This method replaces the shape by the given polygon object. This method can only be called "
    "for editable layouts. It does not change the user properties of the shape.\n"
    "Calling this method will invalidate any iterators. It should not be called inside a "
    "loop iterating over shapes.\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) + 
  gsi::method_ext ("polygon=|dpolygon=", &set_dshape<db::DPolygon>, gsi::arg("polygon"),
    "@brief Replaces the shape by the given polygon (in micrometer units)\n"
    "This method replaces the shape by the given polygon, like \\polygon= with a \\Polygon argument does. "
    "This version translates the polygon from micrometer units to database units internally.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method_ext ("text=", &set_shape<db::Text>, gsi::arg ("box"),
    "@brief Replaces the shape by the given text object\n"
    "This method replaces the shape by the given text object. This method can only be called "
    "for editable layouts. It does not change the user properties of the shape.\n"
    "Calling this method will invalidate any iterators. It should not be called inside a "
    "loop iterating over shapes.\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) + 
  gsi::method_ext ("text=|dtext=", &set_dshape<db::DText>, gsi::arg("text"),
    "@brief Replaces the shape by the given text (in micrometer units)\n"
    "This method replaces the shape by the given text, like \\text= with a \\Text argument does. "
    "This version translates the text from micrometer units to database units internally.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method_ext ("edge=", &set_shape<db::Edge>, gsi::arg("edge"),
    "@brief Replaces the shape by the given edge\n"
    "This method replaces the shape by the given edge. This method can only be called "
    "for editable layouts. It does not change the user properties of the shape.\n"
    "Calling this method will invalidate any iterators. It should not be called inside a "
    "loop iterating over shapes.\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) + 
  gsi::method_ext ("edge=|dedge=", &set_dshape<db::DEdge>, gsi::arg("edge"),
    "@brief Replaces the shape by the given edge (in micrometer units)\n"
    "This method replaces the shape by the given edge, like \\edge= with a \\Edge argument does. "
    "This version translates the edge from micrometer units to database units internally.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method_ext ("point=", &set_shape<db::Point>, gsi::arg("point"),
    "@brief Replaces the shape by the given point\n"
    "This method replaces the shape by the given point. This method can only be called "
    "for editable layouts. It does not change the user properties of the shape.\n"
    "Calling this method will invalidate any iterators. It should not be called inside a "
    "loop iterating over shapes.\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  gsi::method_ext ("point=|dpoint=", &set_dshape<db::DPoint>, gsi::arg("point"),
    "@brief Replaces the shape by the given point (in micrometer units)\n"
    "This method replaces the shape by the given point, like \\point= with a \\Point argument does. "
    "This version translates the point from micrometer units to database units internally.\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  gsi::method_ext ("edge_pair=", &set_shape<db::EdgePair>, gsi::arg("edge_pair"),
    "@brief Replaces the shape by the given edge pair\n"
    "This method replaces the shape by the given edge pair. This method can only be called "
    "for editable layouts. It does not change the user properties of the shape.\n"
    "Calling this method will invalidate any iterators. It should not be called inside a "
    "loop iterating over shapes.\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  gsi::method_ext ("edge_pair=|dedge_pair=", &set_dshape<db::DEdgePair>, gsi::arg("edge_pair"),
    "@brief Replaces the shape by the given edge pair (in micrometer units)\n"
    "This method replaces the shape by the given edge pair, like \\edge_pair= with a \\EdgePair argument does. "
    "This version translates the edge pair from micrometer units to database units internally.\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  gsi::method_ext ("delete_property", &delete_property, gsi::arg ("key"),
    "@brief Deletes the user property with the given key\n"
    "This method is a convenience method that deletes the property with the given key. "
    "It does nothing if no property with that key exists. Using that method is more "
    "convenient than creating a new property set with a new ID and assigning that properties ID.\n"
    "This method may change the properties ID. "
    "Calling this method will invalidate any iterators. It should not be called inside a "
    "loop iterating over shapes.\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) + 
  gsi::method_ext ("set_property", &set_property, gsi::arg ("key"), gsi::arg ("value"),
    "@brief Sets the user property with the given key to the given value\n"
    "This method is a convenience method that sets the property with the given key to the given value. "
    "If no property with that key exists, it will create one. Using that method is more "
    "convenient than creating a new property set with a new ID and assigning that properties ID.\n"
    "This method may change the properties ID. "
    "Note: GDS only supports integer keys. OASIS supports numeric and string keys. "
    "Calling this method will invalidate any iterators. It should not be called inside a "
    "loop iterating over shapes.\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) + 
  gsi::method_ext ("property", &get_property, gsi::arg ("key"),
    "@brief Gets the user property with the given key\n"
    "This method is a convenience method that gets the property with the given key. "
    "If no property with that key does not exist, it will return nil. Using that method is more "
    "convenient than using the layout object and the properties ID to retrieve the property value. "
    "\n"
    "This method has been introduced in version 0.22."
  ) + 
  gsi::iterator ("each_point", &db::Shape::begin_point, &db::Shape::end_point,
    "@brief Iterates over all points of the object\n"
    "\n"
    "This method applies to paths and delivers all points of the path's center line.\n"
    "It will throw an exception for other objects.\n"
  ) +
  gsi::iterator_ext ("each_dpoint", &shape_dpoint_iter,
    "@brief Iterates over all points of the object and returns points in micrometer units\n"
    "\n"
    "This method iterates over all points of the object like \\each_point, but it returns "
    "\\DPoint objects that are given in micrometer units already. Multiplication with "
    "the database unit happens internally.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::iterator ("each_point_hull", &db::Shape::begin_hull, &db::Shape::end_hull,
    "@brief Iterates over the hull contour of the object\n"
    "\n"
    "This method applies to polygons and delivers all points of the polygon hull contour.\n"
    "It will throw an exception for other objects.\n"
  ) +
  gsi::iterator_ext ("each_dpoint_hull", &shape_dpoint_hull_iter,
    "@brief Iterates over the hull contour of the object and returns points in micrometer units\n"
    "\n"
    "This method iterates over all points of the object's contour' like \\each_point_hull, but it returns "
    "\\DPoint objects that are given in micrometer units already. Multiplication with "
    "the database unit happens internally.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::iterator ("each_point_hole", &db::Shape::begin_hole, &db::Shape::end_hole, gsi::arg ("hole_index"),
    "@brief Iterates over the points of a hole contour\n"
    "\n"
    "This method applies to polygons and delivers all points of the respective hole contour.\n"
    "It will throw an exception for other objects.\n"
    "Simple polygons deliver an empty sequence.\n"
    "\n"
    "@param hole The hole index (see holes () method)\n"
  ) +
  gsi::iterator_ext ("each_dpoint_hole", &shape_dpoint_hole_iter, gsi::arg ("hole_index"),
    "@brief Iterates over a hole contour of the object and returns points in micrometer units\n"
    "\n"
    "This method iterates over all points of the object's contour' like \\each_point_hole, but it returns "
    "\\DPoint objects that are given in micrometer units already. Multiplication with "
    "the database unit happens internally.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method ("holes", &db::Shape::holes,
    "@brief Returns the number of holes\n"
    "\n"
    "This method applies to polygons and will throw an exception for other objects..\n"
    "Simple polygons deliver a value of zero.\n"
  ) +
  gsi::iterator ("each_edge", (db::Shape::polygon_edge_iterator (db::Shape::*) () const) &db::Shape::begin_edge,
    "@brief Iterates over the edges of the object\n"
    "\n"
    "This method applies to polygons and simple polygons and delivers all edges that form the polygon's contours. "
    "Hole edges are oriented counterclockwise while hull edges are oriented clockwise.\n"
    "\n"
    "It will throw an exception if the object is not a polygon.\n"
  ) +
  gsi::iterator_ext ("each_dedge", &shape_edge_iter,
    "@brief Iterates over the edges of the object and returns edges in micrometer units\n"
    "\n"
    "This method iterates over all edges of polygons and simple polygons like \\each_edge, but will deliver "
    "edges in micrometer units. Multiplication by the database unit is done internally.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::iterator ("each_edge", (db::Shape::polygon_edge_iterator (db::Shape::*) (unsigned int) const) &db::Shape::begin_edge, gsi::arg ("contour"),
    "@brief Iterates over the edges of a single contour of the object\n"
    "@param contour The contour number (0 for hull, 1 for first hole ...)\n"
    "\n"
    "This method applies to polygons and simple polygons and delivers all edges that form the given contour of the polygon. "
    "The hull has contour number 0, the first hole has contour 1 etc.\n"
    "Hole edges are oriented counterclockwise while hull edges are oriented clockwise.\n"
    "\n"
    "It will throw an exception if the object is not a polygon.\n"
    "\n"
    "This method was introduced in version 0.24."
  ) +
  gsi::iterator_ext ("each_dedge", &shape_edge_iter_per_contour, gsi::arg ("contour"),
    "@brief Iterates over the edges of a single contour of the object and returns edges in micrometer units\n"
    "\n"
    "This method iterates over all edges of polygons and simple polygons like \\each_edge, but will deliver "
    "edges in micrometer units. Multiplication by the database unit is done internally.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method_ext ("box_width", &box_width,
    "@brief Returns the width of the box\n"
    "\n"
    "Applies to boxes only. Returns the width of the box and throws an exception if the shape is not a box.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("box_dwidth", &box_dwidth,
    "@brief Returns the width of the box in micrometer units\n"
    "\n"
    "Applies to boxes only. Returns the width of the box in micrometers and throws an exception if the shape is not a box.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("box_width=", &set_box_width, gsi::arg ("w"),
    "@brief Sets the width of the box\n"
    "\n"
    "Applies to boxes only. Changes the width of the box and throws an exception if the shape is not a box.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("box_dwidth=", &set_box_dwidth, gsi::arg ("w"),
    "@brief Sets the width of the box in micrometer units\n"
    "\n"
    "Applies to boxes only. Changes the width of the box to the value given in micrometer units and throws an exception if the shape is not a box.\n"
    "Translation to database units happens internally.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("box_height", &box_height,
    "@brief Returns the height of the box\n"
    "\n"
    "Applies to boxes only. Returns the height of the box and throws an exception if the shape is not a box.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("box_dheight", &box_dheight,
    "@brief Returns the height of the box in micrometer units\n"
    "\n"
    "Applies to boxes only. Returns the height of the box in micrometers and throws an exception if the shape is not a box.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("box_height=", &set_box_height, gsi::arg ("h"),
    "@brief Sets the height of the box\n"
    "\n"
    "Applies to boxes only. Changes the height of the box and throws an exception if the shape is not a box.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("box_dheight=", &set_box_dheight, gsi::arg ("h"),
    "@brief Sets the height of the box\n"
    "\n"
    "Applies to boxes only. Changes the height of the box to the value given in micrometer units and throws an exception if the shape is not a box.\n"
    "Translation to database units happens internally.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("box_center", &box_center,
    "@brief Returns the center of the box\n"
    "\n"
    "Applies to boxes only. Returns the center of the box and throws an exception if the shape is not a box.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("box_dcenter", &box_dcenter,
    "@brief Returns the center of the box as a \\DPoint object in micrometer units\n"
    "\n"
    "Applies to boxes only. Returns the center of the box and throws an exception if the shape is not a box.\n"
    "Conversion from database units to micrometers is done internally.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("box_center=", &set_box_center, gsi::arg ("c"),
    "@brief Sets the center of the box\n"
    "\n"
    "Applies to boxes only. Changes the center of the box and throws an exception if the shape is not a box.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("box_center=|box_dcenter=", &set_box_dcenter, gsi::arg ("c"),
    "@brief Sets the center of the box with the point being given in micrometer units\n"
    "\n"
    "Applies to boxes only. Changes the center of the box and throws an exception if the shape is not a box.\n"
    "Translation from micrometer units to database units is done internally.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("box_p1", &box_p1,
    "@brief Returns the lower left point of the box\n"
    "\n"
    "Applies to boxes only. Returns the lower left point of the box and throws an exception if the shape is not a box.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("box_dp1", &box_dp1,
    "@brief Returns the lower left point of the box as a \\DPoint object in micrometer units\n"
    "\n"
    "Applies to boxes only. Returns the lower left point of the box and throws an exception if the shape is not a box.\n"
    "Conversion from database units to micrometers is done internally.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("box_p1=", &set_box_p1, gsi::arg ("p"),
    "@brief Sets the lower left point of the box\n"
    "\n"
    "Applies to boxes only. Changes the lower left point of the box and throws an exception if the shape is not a box.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("box_p1=|box_dp1=", &set_box_dp1, gsi::arg ("p"),
    "@brief Sets the lower left corner of the box with the point being given in micrometer units\n"
    "\n"
    "Applies to boxes only. Changes the lower left point of the box and throws an exception if the shape is not a box.\n"
    "Translation from micrometer units to database units is done internally.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("box_p2", &box_p2,
    "@brief Returns the upper right point of the box\n"
    "\n"
    "Applies to boxes only. Returns the upper right point of the box and throws an exception if the shape is not a box.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("box_dp2", &box_dp2,
    "@brief Returns the upper right point of the box as a \\DPoint object in micrometer units\n"
    "\n"
    "Applies to boxes only. Returns the upper right point of the box and throws an exception if the shape is not a box.\n"
    "Conversion from database units to micrometers is done internally.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("box_p2=", &set_box_p2, gsi::arg ("p"),
    "@brief Sets the upper right point of the box\n"
    "\n"
    "Applies to boxes only. Changes the upper right point of the box and throws an exception if the shape is not a box.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("box_p2=|box_dp2=", &set_box_dp2, gsi::arg ("p"),
    "@brief Sets the upper right corner of the box with the point being given in micrometer units\n"
    "\n"
    "Applies to boxes only. Changes the upper right point of the box and throws an exception if the shape is not a box.\n"
    "Translation from micrometer units to database units is done internally.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("type", &object_type,
    "@brief Return the type of the shape\n"
    "\n"
    "The returned values are the t_... constants available through the corresponding class members.\n"
  ) +
  gsi::method ("is_null?", &db::Shape::is_null,
    "@brief Returns true, if the shape reference is a null reference (not referring to a shape)\n"
  ) +
  gsi::method ("is_polygon?", &db::Shape::is_polygon,
    "@brief Returns true, if the shape is a polygon\n"
    "\n"
    "This method returns true only if the object is a polygon or a simple polygon. "
    "Other objects can convert to polygons, for example paths, so it may be possible to use the \\polygon method also "
    "if is_polygon? does not return true."
  ) +
  gsi::method_ext ("polygon", &get_polygon,
    "@brief Returns the polygon object\n"
    "\n"
    "Returns the polygon object that this shape refers to or converts the object to a polygon. "
    "Paths, boxes and simple polygons are converted to polygons. For paths this operation renders the "
    "path's hull contour.\n"
    "\n"
    "Starting with version 0.23, this method returns nil, if the shape does not represent a geometrical "
    "primitive that can be converted to a polygon.\n"
  ) +
  gsi::method_ext ("dpolygon", &get_dpolygon,
    "@brief Returns the polygon object in micrometer units\n"
    "\n"
    "Returns the polygon object that this shape refers to or converts the object to a polygon. "
    "The method returns the same object than \\polygon, but translates it to micrometer units internally.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method ("is_simple_polygon?", &db::Shape::is_simple_polygon,
    "@brief Returns true, if the shape is a simple polygon\n"
    "\n"
    "This method returns true only if the object is a simple polygon. The simple polygon identity is "
    "contained in the polygon identity, so usually it is sufficient to use \\is_polygon? and "
    "\\polygon instead of specifically handle simply polygons. This method is provided only for "
    "specific optimisation purposes."
  ) +
  gsi::method_ext ("simple_polygon", &get_simple_polygon,
    "@brief Returns the simple polygon object\n"
    "\n"
    "Returns the simple polygon object that this shape refers to or converts the object to a simple polygon. "
    "Paths, boxes and polygons are converted to simple polygons. Polygons with holes will have their holes removed but introducing cut lines that connect the hole contours with the outer contour. "
    "\n"
    "Starting with version 0.23, this method returns nil, if the shape does not represent a geometrical "
    "primitive that can be converted to a simple polygon.\n"
  ) +
  gsi::method_ext ("dsimple_polygon", &get_dsimple_polygon,
    "@brief Returns the simple polygon object in micrometer units\n"
    "\n"
    "Returns the simple polygon object that this shape refers to or converts the object to a simple polygon. "
    "The method returns the same object than \\simple_polygon, but translates it to micrometer units internally.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("simple_polygon=", &set_shape<db::SimplePolygon>, gsi::arg ("polygon"),
    "@brief Replaces the shape by the given simple polygon object\n"
    "This method replaces the shape by the given simple polygon object. This method can only be called "
    "for editable layouts. It does not change the user properties of the shape.\n"
    "Calling this method will invalidate any iterators. It should not be called inside a "
    "loop iterating over shapes.\n"
    "\n"
    "This method has been introduced in version 0.22."
  ) + 
  gsi::method_ext ("simple_polygon=|dsimple_polygon=", &set_dshape<db::DSimplePolygon>, gsi::arg ("polygon"),
    "@brief Replaces the shape by the given simple polygon (in micrometer units)\n"
    "This method replaces the shape by the given text, like \\simple_polygon= with a \\SimplePolygon argument does. "
    "This version translates the polygon from micrometer units to database units internally.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method ("is_path?", &db::Shape::is_path,
    "@brief Returns true, if the shape is a path\n"
  ) +
  gsi::method_ext ("path_width", &path_width,
    "@brief Gets the path width\n"
    "\n"
    "Applies to paths only. Will throw an exception if the object is not a path.\n"
  ) +
  gsi::method_ext ("path_dwidth", &path_dwidth,
    "@brief Gets the path width in micrometer units\n"
    "\n"
    "Applies to paths only. Will throw an exception if the object is not a path.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method_ext ("path_width=", &set_path_width, gsi::arg ("w"),
    "@brief Sets the path width\n"
    "Applies to paths only. Will throw an exception if the object is not a path.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method_ext ("path_dwidth=", &set_path_dwidth, gsi::arg ("w"),
    "@brief Sets the path width in micrometer units\n"
    "Applies to paths only. Will throw an exception if the object is not a path.\n"
    "Conversion to database units is done internally.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method_ext ("round_path?", &round_path,
    "@brief Returns true, if the path has round ends\n"
    "\n"
    "Applies to paths only. Will throw an exception if the object is not a path.\n"
  ) +
  gsi::method_ext ("round_path=", &set_round_path, gsi::arg ("r"),
    "@brief The path will be a round-ended path if this property is set to true\n"
    "\n"
    "Applies to paths only. Will throw an exception if the object is not a path.\n"
    "Please note that the extensions will apply as well. To get a path with circular ends, set the begin and "
    "end extensions to half the path's width.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method_ext ("path_bgnext", &path_bgnext,
    "@brief Gets the path's starting vertex extension\n"
    "\n"
    "Applies to paths only. Will throw an exception if the object is not a path.\n"
  ) +
  gsi::method_ext ("path_dbgnext", &path_dbgnext,
    "@brief Gets the path's starting vertex extension in micrometer units\n"
    "\n"
    "Applies to paths only. Will throw an exception if the object is not a path.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method_ext ("path_endext", &path_endext,
    "@brief Obtain the path's end vertex extension\n"
    "\n"
    "Applies to paths only. Will throw an exception if the object is not a path.\n"
  ) +
  gsi::method_ext ("path_dendext", &path_dendext,
    "@brief Gets the path's end vertex extension in micrometer units\n"
    "\n"
    "Applies to paths only. Will throw an exception if the object is not a path.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method_ext ("path_bgnext=", &set_path_bgnext, gsi::arg ("e"),
    "@brief Sets the path's starting vertex extension\n"
    "Applies to paths only. Will throw an exception if the object is not a path.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method_ext ("path_dbgnext=", &set_path_dbgnext, gsi::arg ("e"),
    "@brief Sets the path's starting vertex extension in micrometer units\n"
    "Applies to paths only. Will throw an exception if the object is not a path.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method_ext ("path_endext=", &set_path_endext, gsi::arg ("e"),
    "@brief Sets the path's end vertex extension\n"
    "Applies to paths only. Will throw an exception if the object is not a path.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method_ext ("path_dendext=", &set_path_dendext, gsi::arg ("e"),
    "@brief Sets the path's end vertex extension in micrometer units\n"
    "Applies to paths only. Will throw an exception if the object is not a path.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method_ext ("path_length", &path_length,
    "@brief Returns the length of the path\n"
    "\n"
    "Applies to paths only. Will throw an exception if the object is not a path.\n"
    "This method returns the length of the spine plus extensions if present.\n"
    "\n"
    "This method has been added in version 0.23.\n"
  ) +
  gsi::method_ext ("path_dlength", &path_dlength,
    "@brief Returns the length of the path in micrometer units\n"
    "\n"
    "Applies to paths only. Will throw an exception if the object is not a path.\n"
    "This method returns the length of the spine plus extensions if present.\n"
    "The value returned is given in micrometer units.\n"
    "\n"
    "This method has been added in version 0.25.\n"
  ) +
  gsi::method_ext ("path", &get_path,
    "@brief Returns the path object\n"
    "\n"
    "Starting with version 0.23, this method returns nil, if the shape does not represent a path."
  ) +
  gsi::method_ext ("dpath", &get_dpath,
    "@brief Returns the path object as a \\DPath object in micrometer units\n"
    "See \\path for a description of this method. This method returns the path after translation to "
    "micrometer units.\n"
    "\n"
    "This method has been added in version 0.25.\n"
  ) +
  gsi::method ("is_edge?", &db::Shape::is_edge,
    "@brief Returns true, if the object is an edge\n"
  ) +
  gsi::method_ext ("edge", &get_edge,
    "@brief Returns the edge object\n"
    "\n"
    "Starting with version 0.23, this method returns nil, if the shape does not represent an edge."
  ) +
  gsi::method_ext ("dedge", &get_dedge,
    "@brief Returns the edge object as a \\DEdge object in micrometer units\n"
    "See \\edge for a description of this method. This method returns the edge after translation to "
    "micrometer units.\n"
    "\n"
    "This method has been added in version 0.25.\n"
  ) +
  gsi::method ("is_edge_pair?", &db::Shape::is_edge_pair,
    "@brief Returns true, if the object is an edge pair\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  gsi::method_ext ("edge_pair", &get_edge_pair,
    "@brief Returns the edge pair object\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  gsi::method_ext ("dedge_pair", &get_dedge_pair,
    "@brief Returns the edge pair object as a \\DEdgePair object in micrometer units\n"
    "See \\edge_pair for a description of this method. This method returns the edge pair after translation to "
    "micrometer units.\n"
    "\n"
    "This method has been added in version 0.26.\n"
  ) +
  gsi::method ("is_point?", &db::Shape::is_point,
    "@brief Returns true, if the object is an point\n"
    "\n"
    "This method has been introduced in version 0.28.\n"
  ) +
  gsi::method_ext ("point", &get_point,
    "@brief Returns the point object\n"
    "\n"
    "This method has been introduced in version 0.28.\n"
  ) +
  gsi::method_ext ("dpoint", &get_dpoint,
    "@brief Returns the point object as a \\DPoint object in micrometer units\n"
    "See \\point for a description of this method. This method returns the point after translation to "
    "micrometer units.\n"
    "\n"
    "This method has been introduced in version 0.28.\n"
  ) +
  gsi::method ("is_text?", &db::Shape::is_text,
    "@brief Returns true, if the object is a text\n"
  ) +
  gsi::method_ext ("text", &get_text,
    "@brief Returns the text object\n"
    "\n"
    "Starting with version 0.23, this method returns nil, if the shape does not represent a text."
  ) +
  gsi::method_ext ("dtext", &get_dtext,
    "@brief Returns the path object as a \\DText object in micrometer units\n"
    "See \\text for a description of this method. This method returns the text after translation to "
    "micrometer units.\n"
    "\n"
    "This method has been added in version 0.25.\n"
  ) +
  gsi::method_ext ("text_string", &text_string,
    "@brief Obtain the text string\n"
    "\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
  ) +
  gsi::method_ext ("text_string=", &set_text_string, gsi::arg ("string"),
    "@brief Sets the text string\n"
    "\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method_ext ("text_rot", &text_rot,
    "@brief Gets the text's orientation code (see \\Trans)\n"
    "\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
  ) +
  gsi::method_ext ("text_rot=", &set_text_rot, gsi::arg ("o"),
    "@brief Sets the text's orientation code (see \\Trans)\n"
    "\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
  ) +
  gsi::method_ext ("text_pos", &text_pos,
    "@brief Gets the text's position\n"
    "\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
  ) +
  gsi::method_ext ("text_dpos", &text_dpos,
    "@brief Gets the text's position in micrometer units\n"
    "\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
    "\n"
    "This method has been added in version 0.25.\n"
  ) +
  gsi::method_ext ("text_pos=", &set_text_pos, gsi::arg ("p"),
    "@brief Sets the text's position\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
  ) +
  gsi::method_ext ("text_pos=|text_dpos=", &set_text_dpos, gsi::arg ("p"),
    "@brief Sets the text's position in micrometer units\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
    "\n"
    "This method has been added in version 0.25.\n"
  ) +
  gsi::method_ext ("text_trans", &text_trans,
    "@brief Gets the text transformation\n"
    "\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
  ) +
  gsi::method_ext ("text_dtrans", &text_dtrans,
    "@brief Gets the text transformation in micrometer units\n"
    "\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
    "\n"
    "This method has been added in version 0.25.\n"
  ) +
  gsi::method_ext ("text_trans=", &set_text_trans, gsi::arg ("trans"),
    "@brief Sets the text transformation\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method_ext ("text_trans=|text_dtrans=", &set_text_dtrans, gsi::arg ("trans"),
    "@brief Sets the text transformation in micrometer units\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method_ext ("text_size", &text_size,
    "@brief Gets the text size\n"
    "\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
  ) +
  gsi::method_ext ("text_dsize", &text_dsize,
    "@brief Gets the text size in micrometer units\n"
    "\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method_ext ("text_size=", &set_text_size, gsi::arg ("size"),
    "@brief Sets the text size\n"
    "\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method_ext ("text_dsize=", &set_text_dsize, gsi::arg ("size"),
    "@brief Sets the text size in micrometer units\n"
    "\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  gsi::method_ext ("text_font", &text_font,
    "@brief Gets the text's font\n"
    "\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
  ) +
  gsi::method_ext ("text_font=", &set_text_font, gsi::arg ("font"),
    "@brief Sets the text's font\n"
    "\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  gsi::method_ext ("text_halign", &text_halign,
    "@brief Gets the text's horizontal alignment\n"
    "\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
    "The return value is 0 for left alignment, 1 for center alignment and 2 to right alignment.\n"
    "\n"
    "This method has been introduced in version 0.22.\n"
  ) +
  gsi::method_ext ("text_halign=", &set_text_halign, gsi::arg ("a"),
    "@brief Sets the text's horizontal alignment\n"
    "\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
    "See \\text_halign for a description of that property.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("text_valign", &text_valign,
    "@brief Gets the text's vertical alignment\n"
    "\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
    "The return value is 0 for top alignment, 1 for center alignment and 2 to bottom alignment.\n"
    "\n"
    "This method has been introduced in version 0.22.\n"
  ) +
  gsi::method_ext ("text_valign=", &set_text_valign, gsi::arg ("a"),
    "@brief Sets the text's vertical alignment\n"
    "\n"
    "Applies to texts only. Will throw an exception if the object is not a text.\n"
    "See \\text_valign for a description of that property.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("is_box?", &db::Shape::is_box,
    "@brief Returns true if the shape is a box\n"
  ) +
  gsi::method_ext ("box", &get_box,
    "@brief Gets the box object\n"
    "\n"
    "Starting with version 0.23, this method returns nil, if the shape does not represent a box."
  ) +
  gsi::method_ext ("dbox", &get_dbox,
    "@brief Gets the box object in micrometer units\n"
    "See \\box for a description of this method. This method returns the box after translation to "
    "micrometer units.\n"
    "\n"
    "This method has been added in version 0.25.\n"
  ) +
  gsi::method ("is_user_object?", &db::Shape::is_user_object,
    "@brief Returns true if the shape is a user defined object\n"
  ) +
  gsi::method ("is_array_member?", &db::Shape::is_array_member,
    "@brief Returns true, if the shape is a member of a shape array\n"
  ) +
  gsi::method_ext ("transform", &transform_shape, gsi::arg ("trans"),
    "@brief Transforms the shape with the given transformation\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("transform", &transform_shape_dtrans, gsi::arg ("trans"),
    "@brief Transforms the shape with the given transformation, given in micrometer units\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("transform", &transform_shape_icplx, gsi::arg ("trans"),
    "@brief Transforms the shape with the given complex transformation\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("transform", &transform_shape_dcplx, gsi::arg ("trans"),
    "@brief Transforms the shape with the given complex transformation, given in micrometer units\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("is_valid?", &shape_is_valid,
    "@brief Returns true, if the shape is valid\n"
    "\n"
    "After the shape is deleted, the shape object is no longer valid and this method returns false.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("delete", &delete_shape,
    "@brief Deletes the shape\n"
    "\n"
    "After the shape is deleted, the shape object is emptied and points to nothing.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("array_trans", &db::Shape::array_trans,
    "@brief Gets the array instance member transformation\n"
    "\n"
    "This attribute is valid only if \\is_array_member? is true.\n"
    "The transformation returned describes the relative transformation of the \n"
    "array member addressed.\n"
  ) +
  gsi::method_ext ("array_dtrans", &array_dtrans,
    "@brief Gets the array instance member transformation in micrometer units\n"
    "\n"
    "This attribute is valid only if \\is_array_member? is true.\n"
    "The transformation returned describes the relative transformation of the \n"
    "array member addressed. The displacement is given in micrometer units.\n"
    "\n"
    "This method has been added in version 0.25.\n"
  ) +
  gsi::method ("bbox", &db::Shape::bbox,
    "@brief Returns the bounding box of the shape\n"
  ) +
  gsi::method_ext ("dbbox", &dbbox,
    "@brief Returns the bounding box of the shape in micrometer units\n"
    "This method has been added in version 0.25.\n"
  ) +
  gsi::method_ext ("area", &area,
    "@brief Returns the area of the shape\n"
    "This method has been added in version 0.22.\n"
  ) +
  gsi::method_ext ("darea", &darea,
    "@brief Returns the area of the shape in square micrometer units\n"
    "This method has been added in version 0.25.\n"
  ) +
  gsi::method ("perimeter", &db::Shape::perimeter,
    "@brief Returns the perimeter of the shape\n"
    "\n"
    "This method will return an approximation of the perimeter for paths.\n"
    "\n"
    "This method has been added in version 0.23.\n"
  ) +
  gsi::method_ext ("dperimeter", &dperimeter,
    "@brief Returns the perimeter of the shape in micrometer units\n"
    "\n"
    "This method will return an approximation of the perimeter for paths.\n"
    "\n"
    "This method has been added in version 0.25.\n"
  ) +
  gsi::method_ext ("layer_info", &shape_layer,
    "@brief Returns the \\LayerInfo object of the layer the shape is on\n"
    "If the shape does not reside inside a cell, an empty layer is returned.\n"
    "\n"
    "This method has been added in version 0.23.\n"
  ) +
  gsi::method_ext ("layer_info=", &set_shape_layer, gsi::arg ("layer_info"),
    "@brief Moves the shape to a layer given by a \\LayerInfo object\n"
    "If no layer with the given properties exists, an exception is thrown.\n"
    "\n"
    "This method has been added in version 0.23.\n"
  ) +
  gsi::method_ext ("layer", &shape_layer_index,
    "@brief Returns the layer index of the layer the shape is on\n"
    "Throws an exception if the shape does not reside inside a cell.\n"
    "\n"
    "This method has been added in version 0.23.\n"
  ) +
  gsi::method_ext ("layer=", &set_shape_layer_index, gsi::arg ("layer_index"),
    "@brief Moves the shape to a layer given by the layer index object\n"
    "\n"
    "This method has been added in version 0.23.\n"
  ) +
  gsi::method ("!=", &db::Shape::operator!=, gsi::arg ("other"),
    "@brief Inequality operator\n"
  ) +
  gsi::method ("==", &db::Shape::operator==, gsi::arg ("other"),
    "@brief Equality operator\n"
    "\n"
    "Equality of shapes is not specified by the identity of the objects but by the\n"
    "identity of the pointers - both shapes must refer to the same object.\n"
  ) +
  gsi::method ("to_s", &db::Shape::to_string,
    "@brief Create a string showing the contents of the reference\n"
    "\n"
    "This method has been introduced with version 0.16."
  ) +
  gsi::method ("TNull|#t_null", &t_null) +
  gsi::method ("TPolygon|#t_polygon", &t_polygon) +
  gsi::method ("TPolygonRef|#t_polygon_ref", &t_polygonRef) +
  gsi::method ("TPolygonPtrArray|#t_polygon_ptr_array", &t_polygonPtrArray) +
  gsi::method ("TPolygonPtrArrayMember|#t_polygon_ptr_array_member", &t_polygonPtrArrayMember) +
  gsi::method ("TSimplePolygon|#t_simple_polygon", &t_simplePolygon) +
  gsi::method ("TSimplePolygonRef|#t_simple_polygon_ref", &t_simplePolygonRef) +
  gsi::method ("TSimplePolygonPtrArray|#t_simple_polygon_ptr_array", &t_simplePolygonPtrArray) +
  gsi::method ("TSimplePolygonPtrArrayMember|#t_simple_polygon_ptr_array_member", &t_simplePolygonPtrArrayMember) +
  gsi::method ("TEdge|#t_edge", &t_edge) +
  gsi::method ("TEdgePair|#t_edge_pair", &t_edge_pair) +
  gsi::method ("TPoint|#t_point", &t_point) +
  gsi::method ("TPath|#t_path", &t_path) +
  gsi::method ("TPathRef|#t_path_ref", &t_pathRef) +
  gsi::method ("TPathPtrArray|#t_path_ptr_array", &t_pathPtrArray) +
  gsi::method ("TPathPtrArrayMember|#t_path_ptr_array_member", &t_pathPtrArrayMember) +
  gsi::method ("TBox|#t_box", &t_box) +
  gsi::method ("TBoxArray|#t_box_array", &t_boxArray) +
  gsi::method ("TBoxArrayMember|#t_box_array_member", &t_boxArrayMember) +
  gsi::method ("TShortBox|#t_short_box", &t_shortBox) +
  gsi::method ("TShortBoxArray|#t_short_box_array", &t_shortBoxArray) +
  gsi::method ("TShortBoxArrayMember|#t_short_box_array_member", &t_shortBoxArrayMember) +
  gsi::method ("TText|#t_text", &t_text) +
  gsi::method ("TTextRef|#t_text_ref", &t_textRef) +
  gsi::method ("TTextPtrArray|#t_text_ptr_array", &t_textPtrArray) +
  gsi::method ("TTextPtrArrayMember|#t_text_ptr_array_member", &t_textPtrArrayMember) +
  gsi::method ("TUserObject|#t_user_object", &t_userObject),
  "@brief An object representing a shape in the layout database\n"
  "\n"
  "The shape proxy is basically a pointer to a shape of different kinds.\n"
  "No copy of the shape is created: if the shape proxy is copied the copy still\n"
  "points to the original shape. If the original shape is modified or deleted,\n"
  "the shape proxy will also point to a modified or invalid shape.\n"
  "The proxy can be \"null\" which indicates an invalid reference.\n"
  "\n"
  "Shape objects are used together with the \\Shapes container object which\n"
  "stores the actual shape objects and uses Shape references as pointers inside the\n"
  "actual data storage. Shape references are used in various places, i.e. when removing or\n"
  "transforming objects inside a \\Shapes container.\n"
);

}
