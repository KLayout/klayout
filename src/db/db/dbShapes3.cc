
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


#include "dbShape.h"
#include "dbShapeRepository.h"
#include "dbShapes.h"
#include "dbShapes2.h"

namespace db
{

// -------------------------------------------------------------------------------
//  some utilities

template <class Sh>
inline typename layer<Sh, db::stable_layer_tag>::iterator
iterator_from_shape (const db::layer<Sh, db::stable_layer_tag> & /*layer*/, const db::Shape &shape)
{
  return shape.basic_iter (typename Sh::tag ());
}

template <class Sh>
inline typename layer<Sh, db::unstable_layer_tag>::iterator
iterator_from_shape (const db::layer<Sh, db::unstable_layer_tag> &layer, const db::Shape &shape)
{
  //  compute the iterator by some pointer arithmetics assuming that layer uses an contiguous container
  //  in unstable mode ...
  return layer.begin () + (shape.basic_ptr (typename Sh::tag ()) - layer.begin ().operator-> ());
}

template <class Sh>
inline bool
iterator_from_shape_is_valid (const db::layer<Sh, db::stable_layer_tag> &layer, const db::Shape &shape)
{
  typename db::layer<Sh, db::stable_layer_tag>::iterator iter = shape.basic_iter (typename Sh::tag ());
  return iter.vector () == layer.begin ().vector () && iter.is_valid ();
}

template <class Sh>
inline bool
iterator_from_shape_is_valid (const db::layer<Sh, db::unstable_layer_tag> &layer, const db::Shape &shape)
{
  return layer.size () > size_t (shape.basic_ptr (typename Sh::tag ()) - layer.begin ().operator-> ());
}

// -------------------------------------------------------------------------------

template <class Sh, class StableTag>
const db::layer<Sh, StableTag> &
Shapes::get_layer () const
{
  typedef layer_class<Sh, StableTag> lay_cls;

  for (typename tl::vector<LayerBase *>::const_iterator l = m_layers.begin (); l != m_layers.end (); ++l) {
    const lay_cls *lc = dynamic_cast <const lay_cls *> (*l);
    if (lc) {
      return lc->layer ();
    }
  }

  //  there seems to be a bug in gcc that disables
  //  the use of a simple static object here:
  static const db::layer<Sh, StableTag> *empty_layer = 0;
  if (! empty_layer) {
    empty_layer = new db::layer<Sh, StableTag> ();
  }
  return *empty_layer;
}

template <class Sh, class StableTag>
db::layer<Sh, StableTag> &
Shapes::get_layer ()
{
  typedef layer_class<Sh, StableTag> lay_cls;
  lay_cls *lc;

  for (typename tl::vector<LayerBase *>::iterator l = m_layers.begin (); l != m_layers.end (); ++l) {
    lc = dynamic_cast <lay_cls *> (*l);
    if (lc) {
      //  this is what optimizes access times for another access
      //  with this type
      std::swap (m_layers.front (), *l);
      return lc->layer ();
    }
  }

  //  create
  lc = new lay_cls ();
  m_layers.push_back (lc);
  std::swap (m_layers.front (), m_layers.back ());
  return lc->layer ();
}


template DB_PUBLIC layer<db::Shape::polygon_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::polygon_type, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::polygon_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::polygon_type>, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::simple_polygon_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::simple_polygon_type, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::simple_polygon_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::simple_polygon_type>, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::polygon_ref_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::polygon_ref_type, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::polygon_ref_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::polygon_ref_type>, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::simple_polygon_ref_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::simple_polygon_ref_type, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::simple_polygon_ref_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::simple_polygon_ref_type>, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::polygon_ptr_array_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::polygon_ptr_array_type, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::polygon_ptr_array_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::polygon_ptr_array_type>, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::simple_polygon_ptr_array_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::simple_polygon_ptr_array_type, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::simple_polygon_ptr_array_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::simple_polygon_ptr_array_type>, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::path_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::path_type, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::path_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::path_type>, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::path_ref_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::path_ref_type, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::path_ref_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::path_ref_type>, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::path_ptr_array_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::path_ptr_array_type, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::path_ptr_array_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::path_ptr_array_type>, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::edge_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::edge_type, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::edge_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::edge_type>, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::edge_pair_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::edge_pair_type, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::edge_pair_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::edge_pair_type>, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::point_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::point_type, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::point_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::point_type>, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::text_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::text_type, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::text_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::text_type>, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::text_ref_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::text_ref_type, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::text_ref_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::text_ref_type>, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::text_ptr_array_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::text_ptr_array_type, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::text_ptr_array_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::text_ptr_array_type>, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::box_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::box_type, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::box_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::box_type>, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::box_array_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::box_array_type, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::box_array_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::box_array_type>, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::short_box_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::short_box_type, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::short_box_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::short_box_type>, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::short_box_array_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::short_box_array_type, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::short_box_array_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::short_box_array_type>, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::user_object_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::user_object_type, db::stable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::user_object_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::user_object_type>, db::stable_layer_tag> ();

template DB_PUBLIC layer<db::Shape::polygon_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::polygon_type, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::polygon_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::polygon_type>, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::simple_polygon_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::simple_polygon_type, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::simple_polygon_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::simple_polygon_type>, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::polygon_ref_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::polygon_ref_type, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::polygon_ref_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::polygon_ref_type>, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::simple_polygon_ref_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::simple_polygon_ref_type, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::simple_polygon_ref_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::simple_polygon_ref_type>, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::polygon_ptr_array_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::polygon_ptr_array_type, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::polygon_ptr_array_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::polygon_ptr_array_type>, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::simple_polygon_ptr_array_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::simple_polygon_ptr_array_type, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::simple_polygon_ptr_array_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::simple_polygon_ptr_array_type>, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::path_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::path_type, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::path_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::path_type>, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::path_ref_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::path_ref_type, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::path_ref_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::path_ref_type>, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::path_ptr_array_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::path_ptr_array_type, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::path_ptr_array_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::path_ptr_array_type>, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::edge_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::edge_type, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::edge_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::edge_type>, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::edge_pair_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::edge_pair_type, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::edge_pair_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::edge_pair_type>, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::point_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::point_type, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::point_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::point_type>, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::text_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::text_type, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::text_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::text_type>, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::text_ref_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::text_ref_type, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::text_ref_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::text_ref_type>, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::text_ptr_array_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::text_ptr_array_type, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::text_ptr_array_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::text_ptr_array_type>, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::box_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::box_type, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::box_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::box_type>, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::box_array_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::box_array_type, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::box_array_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::box_array_type>, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::short_box_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::short_box_type, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::short_box_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::short_box_type>, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::short_box_array_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::short_box_array_type, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::short_box_array_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::short_box_array_type>, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::Shape::user_object_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::user_object_type, db::unstable_layer_tag> ();
template DB_PUBLIC layer<db::object_with_properties<db::Shape::user_object_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::user_object_type>, db::unstable_layer_tag> ();

template DB_PUBLIC const layer<db::Shape::polygon_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::polygon_type, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::polygon_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::polygon_type>, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::simple_polygon_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::simple_polygon_type, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::simple_polygon_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::simple_polygon_type>, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::polygon_ref_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::polygon_ref_type, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::polygon_ref_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::polygon_ref_type>, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::simple_polygon_ref_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::simple_polygon_ref_type, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::simple_polygon_ref_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::simple_polygon_ref_type>, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::polygon_ptr_array_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::polygon_ptr_array_type, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::polygon_ptr_array_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::polygon_ptr_array_type>, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::simple_polygon_ptr_array_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::simple_polygon_ptr_array_type, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::simple_polygon_ptr_array_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::simple_polygon_ptr_array_type>, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::path_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::path_type, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::path_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::path_type>, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::path_ref_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::path_ref_type, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::path_ref_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::path_ref_type>, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::path_ptr_array_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::path_ptr_array_type, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::path_ptr_array_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::path_ptr_array_type>, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::edge_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::edge_type, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::edge_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::edge_type>, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::edge_pair_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::edge_pair_type, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::edge_pair_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::edge_pair_type>, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::point_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::point_type, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::point_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::point_type>, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::text_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::text_type, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::text_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::text_type>, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::text_ref_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::text_ref_type, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::text_ref_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::text_ref_type>, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::text_ptr_array_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::text_ptr_array_type, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::text_ptr_array_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::text_ptr_array_type>, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::box_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::box_type, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::box_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::box_type>, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::box_array_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::box_array_type, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::box_array_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::box_array_type>, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::short_box_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::short_box_type, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::short_box_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::short_box_type>, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::short_box_array_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::short_box_array_type, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::short_box_array_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::short_box_array_type>, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::user_object_type, db::stable_layer_tag> &Shapes::get_layer<db::Shape::user_object_type, db::stable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::user_object_type>, db::stable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::user_object_type>, db::stable_layer_tag> () const;

template DB_PUBLIC const layer<db::Shape::polygon_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::polygon_type, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::polygon_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::polygon_type>, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::simple_polygon_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::simple_polygon_type, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::simple_polygon_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::simple_polygon_type>, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::polygon_ref_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::polygon_ref_type, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::polygon_ref_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::polygon_ref_type>, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::simple_polygon_ref_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::simple_polygon_ref_type, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::simple_polygon_ref_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::simple_polygon_ref_type>, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::polygon_ptr_array_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::polygon_ptr_array_type, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::polygon_ptr_array_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::polygon_ptr_array_type>, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::simple_polygon_ptr_array_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::simple_polygon_ptr_array_type, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::simple_polygon_ptr_array_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::simple_polygon_ptr_array_type>, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::path_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::path_type, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::path_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::path_type>, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::path_ref_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::path_ref_type, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::path_ref_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::path_ref_type>, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::path_ptr_array_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::path_ptr_array_type, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::path_ptr_array_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::path_ptr_array_type>, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::edge_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::edge_type, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::edge_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::edge_type>, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::edge_pair_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::edge_pair_type, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::edge_pair_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::edge_pair_type>, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::point_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::point_type, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::point_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::point_type>, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::text_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::text_type, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::text_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::text_type>, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::text_ref_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::text_ref_type, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::text_ref_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::text_ref_type>, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::text_ptr_array_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::text_ptr_array_type, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::text_ptr_array_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::text_ptr_array_type>, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::box_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::box_type, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::box_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::box_type>, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::box_array_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::box_array_type, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::box_array_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::box_array_type>, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::short_box_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::short_box_type, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::short_box_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::short_box_type>, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::short_box_array_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::short_box_array_type, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::short_box_array_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::short_box_array_type>, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::Shape::user_object_type, db::unstable_layer_tag> &Shapes::get_layer<db::Shape::user_object_type, db::unstable_layer_tag> () const;
template DB_PUBLIC const layer<db::object_with_properties<db::Shape::user_object_type>, db::unstable_layer_tag> &Shapes::get_layer<db::object_with_properties<db::Shape::user_object_type>, db::unstable_layer_tag> () const;


template <class Tag>
bool
Shapes::is_valid_shape_by_tag (Tag /*tag*/, const shape_type &shape) const
{
  if (! is_editable ()) {
    if (! shape.has_prop_id ()) {
      typedef typename Tag::object_type s_type;
      return iterator_from_shape_is_valid (get_layer<s_type, db::unstable_layer_tag> (), shape);
    } else {
      typedef db::object_with_properties<typename Tag::object_type> swp_type;
      return iterator_from_shape_is_valid (get_layer<swp_type, db::unstable_layer_tag> (), shape);
    }
  } else {
    if (! shape.has_prop_id ()) {
      typedef typename Tag::object_type s_type;
      return iterator_from_shape_is_valid (get_layer<s_type, db::stable_layer_tag> (), shape);
    } else {
      typedef db::object_with_properties<typename Tag::object_type> swp_type;
      return iterator_from_shape_is_valid (get_layer<swp_type, db::stable_layer_tag> (), shape);
    }
  }
}

bool
Shapes::is_valid (const Shapes::shape_type &shape) const
{
  switch (shape.m_type) {
  case shape_type::Null:
  default:
    return false;
  case shape_type::Polygon:
    return is_valid_shape_by_tag (shape_type::polygon_type::tag (), shape);
  case shape_type::PolygonRef:
    return is_valid_shape_by_tag (shape_type::polygon_ref_type::tag (), shape);
  case shape_type::PolygonPtrArrayMember:
  case shape_type::PolygonPtrArray:
    return is_valid_shape_by_tag (shape_type::polygon_ptr_array_type::tag (), shape);
  case shape_type::SimplePolygon:
    return is_valid_shape_by_tag (shape_type::simple_polygon_type::tag (), shape);
  case shape_type::SimplePolygonRef:
    return is_valid_shape_by_tag (shape_type::simple_polygon_ref_type::tag (), shape);
  case shape_type::SimplePolygonPtrArrayMember:
  case shape_type::SimplePolygonPtrArray:
    return is_valid_shape_by_tag (shape_type::simple_polygon_ptr_array_type::tag (), shape);
  case shape_type::Edge:
    return is_valid_shape_by_tag (shape_type::edge_type::tag (), shape);
  case shape_type::EdgePair:
    return is_valid_shape_by_tag (shape_type::edge_pair_type::tag (), shape);
  case shape_type::Point:
    return is_valid_shape_by_tag (shape_type::point_type::tag (), shape);
  case shape_type::Path:
    return is_valid_shape_by_tag (shape_type::path_type::tag (), shape);
  case shape_type::PathRef:
    return is_valid_shape_by_tag (shape_type::path_ref_type::tag (), shape);
  case shape_type::PathPtrArrayMember:
  case shape_type::PathPtrArray:
    return is_valid_shape_by_tag (shape_type::path_ptr_array_type::tag (), shape);
  case shape_type::Box:
    return is_valid_shape_by_tag (shape_type::box_type::tag (), shape);
  case shape_type::BoxArrayMember:
  case shape_type::BoxArray:
    return is_valid_shape_by_tag (shape_type::box_array_type::tag (), shape);
  case shape_type::ShortBox:
    return is_valid_shape_by_tag (shape_type::short_box_type::tag (), shape);
  case shape_type::ShortBoxArrayMember:
  case shape_type::ShortBoxArray:
    return is_valid_shape_by_tag (shape_type::short_box_array_type::tag (), shape);
  case shape_type::Text:
    return is_valid_shape_by_tag (shape_type::text_type::tag (), shape);
  case shape_type::TextRef:
    return is_valid_shape_by_tag (shape_type::text_ref_type::tag (), shape);
  case shape_type::TextPtrArrayMember:
  case shape_type::TextPtrArray:
    return is_valid_shape_by_tag (shape_type::text_ptr_array_type::tag (), shape);
  case shape_type::UserObject:
    return is_valid_shape_by_tag (shape_type::user_object_type::tag (), shape);
  };
}

template <class Tag>
void
Shapes::erase_shape_by_tag (Tag tag, const shape_type &shape)
{
  if (is_editable ()) {
    erase_shape_by_tag_ws (tag, db::stable_layer_tag (), shape);
  } else {
    erase_shape_by_tag_ws (tag, db::unstable_layer_tag (), shape);
  }
}

template <class Tag, class StableTag>
void
Shapes::erase_shape_by_tag_ws (Tag /*tag*/, StableTag /*stable_tag*/, const shape_type &shape)
{
  if (! is_editable ()) {
    throw tl::Exception (tl::to_string (tr ("Function 'erase' is permitted only in editable mode")));
  }
  if (! shape.has_prop_id ()) {

    db::layer<typename Tag::object_type, StableTag> &l = get_layer<typename Tag::object_type, StableTag> ();
    typename db::layer<typename Tag::object_type, StableTag>::iterator i = iterator_from_shape (l, shape);
    if (manager () && manager ()->transacting ()) {
      check_is_editable_for_undo_redo ();
      db::layer_op<typename Tag::object_type, StableTag>::queue_or_append (manager (), this, false /*not insert*/, *i);
    }
    invalidate_state ();  //  HINT: must come before the change is done!
    l.erase (i);

  } else {

    typedef db::object_with_properties<typename Tag::object_type> swp_type;

    db::layer<swp_type, StableTag> &l = get_layer<swp_type, StableTag> ();
    typename db::layer<swp_type, StableTag>::iterator i = iterator_from_shape (l, shape);
    if (manager () && manager ()->transacting ()) {
      check_is_editable_for_undo_redo ();
      db::layer_op<swp_type, StableTag>::queue_or_append (manager (), this, false /*not insert*/, *i);
    }
    invalidate_state ();  //  HINT: must come before the change is done!
    l.erase (i);

  }
}

template <class Tag, class StableTag>
void
Shapes::erase_shapes_by_tag_ws (Tag /*tag*/, StableTag /*stable_tag*/, std::vector<Shapes::shape_type>::const_iterator s1, std::vector<Shapes::shape_type>::const_iterator s2)
{
  if (! s1->has_prop_id ()) {

    std::vector<typename db::layer<typename Tag::object_type, StableTag>::iterator> iters;
    iters.reserve (std::distance (s1, s2));

    for (std::vector<shape_type>::const_iterator s = s1; s != s2; ++s) {
      typename db::layer<typename Tag::object_type, StableTag>::iterator i = iterator_from_shape (get_layer<typename Tag::object_type, StableTag> (), *s);
      //  in the "whole array" case it may happen that we delete one object multiple times ..
      if (iters.empty () || ! (iters.back () == i)) {
        iters.push_back (i);
      }
    }

    erase_positions (Tag (), StableTag (), iters.begin (), iters.end ());

  } else {

    typedef db::object_with_properties<typename Tag::object_type> swp_type;
    std::vector<typename db::layer<swp_type, StableTag>::iterator> iters;
    iters.reserve (std::distance (s1, s2));

    for (std::vector<shape_type>::const_iterator s = s1; s != s2; ++s) {
      typename db::layer<swp_type, StableTag>::iterator i = iterator_from_shape (get_layer<swp_type, StableTag> (), *s);
      //  in the "whole array" case it may happen that we delete one object multiple times ..
      if (iters.empty () || ! (iters.back () == i)) {
        iters.push_back (i);
      }
    }

    erase_positions (typename swp_type::tag (), StableTag (), iters.begin (), iters.end ());

  }
}

template <class Tag>
void
Shapes::erase_shapes_by_tag (Tag tag, std::vector<Shapes::shape_type>::const_iterator s1, std::vector<Shapes::shape_type>::const_iterator s2)
{
  if (is_editable ()) {
    erase_shapes_by_tag_ws (tag, db::stable_layer_tag (), s1, s2);
  } else {
    erase_shapes_by_tag_ws (tag, db::unstable_layer_tag (), s1, s2);
  }
}

void
Shapes::erase_shape (const Shapes::shape_type &shape)
{
  if (! is_editable ()) {
    throw tl::Exception (tl::to_string (tr ("Function 'erase' is permitted only in editable mode")));
  }

  switch (shape.m_type) {
  case shape_type::Null:
    break;
  case shape_type::Polygon:
    erase_shape_by_tag (shape_type::polygon_type::tag (), shape);
    break;
  case shape_type::PolygonRef:
    erase_shape_by_tag (shape_type::polygon_ref_type::tag (), shape);
    break;
  case shape_type::PolygonPtrArrayMember:
    //  HINT: since we are in editing mode, this should never happen:
  case shape_type::PolygonPtrArray:
    //  HINT: since we are in editing mode, this should never happen:
    erase_shape_by_tag (shape_type::polygon_ptr_array_type::tag (), shape);
    break;
  case shape_type::SimplePolygon:
    erase_shape_by_tag (shape_type::simple_polygon_type::tag (), shape);
    break;
  case shape_type::SimplePolygonRef:
    erase_shape_by_tag (shape_type::simple_polygon_ref_type::tag (), shape);
    break;
  case shape_type::SimplePolygonPtrArrayMember:
    //  HINT: since we are in editing mode, this should never happen:
  case shape_type::SimplePolygonPtrArray:
    //  HINT: since we are in editing mode, this should never happen:
    erase_shape_by_tag (shape_type::simple_polygon_ptr_array_type::tag (), shape);
    break;
  case shape_type::Edge:
    erase_shape_by_tag (shape_type::edge_type::tag (), shape);
    break;
  case shape_type::EdgePair:
    erase_shape_by_tag (shape_type::edge_pair_type::tag (), shape);
    break;
  case shape_type::Point:
    erase_shape_by_tag (shape_type::point_type::tag (), shape);
    break;
  case shape_type::Path:
    erase_shape_by_tag (shape_type::path_type::tag (), shape);
    break;
  case shape_type::PathRef:
    erase_shape_by_tag (shape_type::path_ref_type::tag (), shape);
    break;
  case shape_type::PathPtrArrayMember:
    //  HINT: since we are in editing mode, this should never happen:
  case shape_type::PathPtrArray:
    //  HINT: since we are in editing mode, this should never happen:
    erase_shape_by_tag (shape_type::path_ptr_array_type::tag (), shape);
    break;
  case shape_type::Box:
    erase_shape_by_tag (shape_type::box_type::tag (), shape);
    break;
  case shape_type::BoxArrayMember:
    //  HINT: since we are in editing mode, this should never happen:
  case shape_type::BoxArray:
    //  HINT: since we are in editing mode, this should never happen:
    erase_shape_by_tag (shape_type::box_array_type::tag (), shape);
    break;
  case shape_type::ShortBox:
    erase_shape_by_tag (shape_type::short_box_type::tag (), shape);
    break;
  case shape_type::ShortBoxArrayMember:
    //  HINT: since we are in editing mode, this should never happen:
  case shape_type::ShortBoxArray:
    //  HINT: since we are in editing mode, this should never happen:
    erase_shape_by_tag (shape_type::short_box_array_type::tag (), shape);
    break;
  case shape_type::Text:
    erase_shape_by_tag (shape_type::text_type::tag (), shape);
    break;
  case shape_type::TextRef:
    erase_shape_by_tag (shape_type::text_ref_type::tag (), shape);
    break;
  case shape_type::TextPtrArrayMember:
    //  HINT: since we are in editing mode, this should never happen:
  case shape_type::TextPtrArray:
    //  HINT: since we are in editing mode, this should never happen:
    erase_shape_by_tag (shape_type::text_ptr_array_type::tag (), shape);
    break;
  case shape_type::UserObject:
    erase_shape_by_tag (shape_type::user_object_type::tag (), shape);
    break;
  };
}

void
Shapes::erase_shapes (const std::vector<Shapes::shape_type> &shapes)
{
  if (! is_editable ()) {
    throw tl::Exception (tl::to_string (tr ("Function 'erase' is permitted only in editable mode")));
  }

  for (std::vector<shape_type>::const_iterator s = shapes.begin (); s != shapes.end (); ) {

    std::vector<shape_type>::const_iterator snext = s;
    while (snext != shapes.end () && snext->type () == s->type () && snext->has_prop_id () == s->has_prop_id ()) {
      ++snext;
    }

    switch (s->m_type) {
    case shape_type::Null:
      break;
    case shape_type::Polygon:
      erase_shapes_by_tag (shape_type::polygon_type::tag (), s, snext);
      break;
    case shape_type::PolygonRef:
      erase_shapes_by_tag (shape_type::polygon_ref_type::tag (), s, snext);
      break;
    case shape_type::PolygonPtrArrayMember:
      //  HINT: since we are in editing mode, this should never happen:
    case shape_type::PolygonPtrArray:
      //  HINT: since we are in editing mode, this should never happen:
      erase_shapes_by_tag (shape_type::polygon_ptr_array_type::tag (), s, snext);
      break;
    case shape_type::SimplePolygon:
      erase_shapes_by_tag (shape_type::simple_polygon_type::tag (), s, snext);
      break;
    case shape_type::SimplePolygonRef:
      erase_shapes_by_tag (shape_type::simple_polygon_ref_type::tag (), s, snext);
      break;
    case shape_type::SimplePolygonPtrArrayMember:
      //  HINT: since we are in editing mode, this should never happen:
    case shape_type::SimplePolygonPtrArray:
      //  HINT: since we are in editing mode, this should never happen:
      erase_shapes_by_tag (shape_type::simple_polygon_ptr_array_type::tag (), s, snext);
      break;
    case shape_type::Edge:
      erase_shapes_by_tag (shape_type::edge_type::tag (), s, snext);
      break;
    case shape_type::Point:
      erase_shapes_by_tag (shape_type::point_type::tag (), s, snext);
      break;
    case shape_type::EdgePair:
      erase_shapes_by_tag (shape_type::edge_pair_type::tag (), s, snext);
      break;
    case shape_type::Path:
      erase_shapes_by_tag (shape_type::path_type::tag (), s, snext);
      break;
    case shape_type::PathRef:
      erase_shapes_by_tag (shape_type::path_ref_type::tag (), s, snext);
      break;
    case shape_type::PathPtrArrayMember:
      //  HINT: since we are in editing mode, this should never happen:
    case shape_type::PathPtrArray:
      //  HINT: since we are in editing mode, this should never happen:
      erase_shapes_by_tag (shape_type::path_ptr_array_type::tag (), s, snext);
      break;
    case shape_type::Box:
      erase_shapes_by_tag (shape_type::box_type::tag (), s, snext);
      break;
    case shape_type::BoxArrayMember:
      //  HINT: since we are in editing mode, this should never happen:
    case shape_type::BoxArray:
      //  HINT: since we are in editing mode, this should never happen:
      erase_shapes_by_tag (shape_type::box_array_type::tag (), s, snext);
      break;
    case shape_type::ShortBox:
      erase_shapes_by_tag (shape_type::short_box_type::tag (), s, snext);
      break;
    case shape_type::ShortBoxArrayMember:
      //  HINT: since we are in editing mode, this should never happen:
    case shape_type::ShortBoxArray:
      //  HINT: since we are in editing mode, this should never happen:
      erase_shapes_by_tag (shape_type::short_box_array_type::tag (), s, snext);
      break;
    case shape_type::Text:
      erase_shapes_by_tag (shape_type::text_type::tag (), s, snext);
      break;
    case shape_type::TextRef:
      erase_shapes_by_tag (shape_type::text_ref_type::tag (), s, snext);
      break;
    case shape_type::TextPtrArrayMember:
      //  HINT: since we are in editing mode, this should never happen:
    case shape_type::TextPtrArray:
      //  HINT: since we are in editing mode, this should never happen:
      erase_shapes_by_tag (shape_type::text_ptr_array_type::tag (), s, snext);
      break;
    case shape_type::UserObject:
      erase_shapes_by_tag (shape_type::user_object_type::tag (), s, snext);
      break;
    };

    s = snext;

  }
}

}
