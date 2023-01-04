
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

namespace db
{

// -------------------------------------------------------------------------------

template <class Sh, class Iter>
inline db::Shape
iterator_to_shape (db::Shapes *shapes, const db::layer<Sh, db::unstable_layer_tag> & /*l*/, const Iter &iter)
{
  //  for unstable containers, we simply use the pointer as a reference
  return db::Shape (shapes, *iter);
}

template <class Sh, class Iter>
inline db::Shape
iterator_to_shape (db::Shapes *shapes, const db::layer<Sh, db::stable_layer_tag> &l, const Iter &iter)
{
  //  for stable containers, we derive the primitive iterator via the pointer
  return db::Shape (shapes, l.iterator_from_pointer (&*iter));
}

template <class Sh, class Iter>
inline db::Shape
iterator_to_shape (const db::Shapes *shapes, const db::layer<Sh, db::unstable_layer_tag> & /*l*/, const Iter &iter)
{
  //  for unstable containers, we simply use the pointer as a reference
  return db::Shape (shapes, *iter);
}

template <class Sh, class Iter>
inline db::Shape
iterator_to_shape (const db::Shapes *shapes, const db::layer<Sh, db::stable_layer_tag> &l, const Iter &iter)
{
  //  for stable containers, we derive the primitive iterator via the pointer
  return db::Shape (shapes, l.iterator_from_pointer (&*iter));
}

// -------------------------------------------------------------------------------
//  ShapeIterator implementation

ShapeIterator::ShapeIterator ()
  : m_region_mode (None),
    m_type (Null),
    m_box (),
    m_flags (0),
    mp_shapes (0),
    mp_prop_sel (0),
    m_inv_prop_sel (false),
    m_array_iterator_valid (false),
    m_editable (false),
    m_quad_id (0)
{
  m_valid = false;
  m_with_props = false;
}

ShapeIterator::ShapeIterator (const ShapeIterator &d)
  : m_region_mode (None),
    m_type (Null),
    m_box (),
    m_flags (0),
    mp_shapes (0),
    mp_prop_sel (0),
    m_inv_prop_sel (false),
    m_array_iterator_valid (false),
    m_editable (false),
    m_quad_id (0)
{
  m_valid = false;
  m_with_props = false;
  operator= (d);
}

ShapeIterator::ShapeIterator (const shapes_type &shapes, unsigned int flags, const property_selector *prop_sel, bool inv_prop_sel)
  : m_region_mode (None),
    m_type (object_type (0)),
    m_box (),
    m_flags (flags),
    mp_shapes (&shapes),
    mp_prop_sel (prop_sel),
    m_inv_prop_sel (inv_prop_sel),
    m_array_iterator_valid (false),
    m_editable (shapes.is_editable ()),
    m_quad_id (0)
{
  //  optimize: empty property selection plus inverse = no property selection at all
  //            any property selection and not inverse = only shapes with properties
  if (mp_prop_sel) {
    if (mp_prop_sel->empty () && m_inv_prop_sel) {
      mp_prop_sel = 0;
      m_inv_prop_sel = false;
    } else if (! m_inv_prop_sel) {
      m_flags |= Properties;
    }
  }
  m_valid = false;
  m_with_props = false;

  //  look for the first type selected
  for (unsigned int m = 1; m_type != Null && (m_flags & m) == 0; m <<= 1) {
    m_type = object_type ((unsigned int) m_type + 1);
  }

  advance (0); // validate
}

ShapeIterator::ShapeIterator (const shapes_type &shapes, const box_type &box, region_mode mode, unsigned int flags, const property_selector *prop_sel, bool inv_prop_sel)
  : m_region_mode (mode),
    m_type (object_type (0)),
    m_box (box),
    m_flags (flags),
    mp_shapes (&shapes),
    mp_prop_sel (prop_sel),
    m_inv_prop_sel (inv_prop_sel),
    m_array_iterator_valid (false),
    m_editable (shapes.is_editable ()),
    m_quad_id (0)
{
  //  optimize: empty property selection plus inverse = no property selection at all
  //            any property selection and not inverse = only shapes with properties
  if (mp_prop_sel) {
    if (mp_prop_sel->empty () && m_inv_prop_sel) {
      mp_prop_sel = 0;
      m_inv_prop_sel = false;
    } else if (! m_inv_prop_sel) {
      m_flags |= Properties;
    }
  }
  m_valid = false;
  m_with_props = false;

  //  look for the first type selected
  for (unsigned int m = 1; m_type != Null && (m_flags & m) == 0; m <<= 1) {
    m_type = object_type ((unsigned int) m_type + 1);
  }

  advance (0); // validate
}

ShapeIterator &
ShapeIterator::operator= (const ShapeIterator &d)
{
  if (&d != this) {

    cleanup ();

    m_d = d.m_d;
    m_valid = d.m_valid;
    m_with_props = d.m_with_props;
    m_region_mode = d.m_region_mode;
    m_type = d.m_type;
    m_box = d.m_box;
    m_shape = d.m_shape;
    m_array = d.m_array;
    m_flags = d.m_flags;
    mp_shapes = d.mp_shapes;
    mp_prop_sel = d.mp_prop_sel;
    m_inv_prop_sel = d.m_inv_prop_sel;
    m_array_iterator_valid = d.m_array_iterator_valid;
    m_editable = d.m_editable;
    m_quad_id = d.m_quad_id;

    if (m_type != Null) {

      if (m_array_iterator_valid) {

        if (m_type == PolygonPtrArray) {

          polygon_ptr_array_iterator_type *d_arr_iter = (polygon_ptr_array_iterator_type *) d.m_ad.iter;
          polygon_ptr_array_iterator_type *arr_iter = (polygon_ptr_array_iterator_type *) m_ad.iter;
          new (arr_iter) polygon_ptr_array_iterator_type (*d_arr_iter);

        } else if (m_type == SimplePolygonPtrArray) {

          simple_polygon_ptr_array_iterator_type *d_arr_iter = (simple_polygon_ptr_array_iterator_type *) d.m_ad.iter;
          simple_polygon_ptr_array_iterator_type *arr_iter = (simple_polygon_ptr_array_iterator_type *) m_ad.iter;
          new (arr_iter) simple_polygon_ptr_array_iterator_type (*d_arr_iter);

        } else if (m_type == PathPtrArray) {

          path_ptr_array_iterator_type *d_arr_iter = (path_ptr_array_iterator_type *) d.m_ad.iter;
          path_ptr_array_iterator_type *arr_iter = (path_ptr_array_iterator_type *) m_ad.iter;
          new (arr_iter) path_ptr_array_iterator_type (*d_arr_iter);

        } else if (m_type == TextPtrArray) {

          text_ptr_array_iterator_type *d_arr_iter = (text_ptr_array_iterator_type *) d.m_ad.iter;
          text_ptr_array_iterator_type *arr_iter = (text_ptr_array_iterator_type *) m_ad.iter;
          new (arr_iter) text_ptr_array_iterator_type (*d_arr_iter);

        } else if (m_type == BoxArray) {

          box_array_iterator_type *d_arr_iter = (box_array_iterator_type *) d.m_ad.iter;
          box_array_iterator_type *arr_iter = (box_array_iterator_type *) m_ad.iter;
          new (arr_iter) box_array_iterator_type (*d_arr_iter);

        } else if (m_type == ShortBoxArray) {

          short_box_array_iterator_type *d_arr_iter = (short_box_array_iterator_type *) d.m_ad.iter;
          short_box_array_iterator_type *arr_iter = (short_box_array_iterator_type *) m_ad.iter;
          new (arr_iter) short_box_array_iterator_type (*d_arr_iter);

        }

      }

    }

  }
  return *this;
}

template <class Iter>
inline void
ShapeIterator::skip_array_iter ()
{
  Iter *arr_iter = (Iter *) m_ad.iter;
  arr_iter->~Iter ();
}

void
ShapeIterator::skip_array ()
{
  if (m_array_iterator_valid) {
    if (m_type == PolygonPtrArray) {
      skip_array_iter<polygon_ptr_array_iterator_type> ();
    } else if (m_type == SimplePolygonPtrArray) {
      skip_array_iter<simple_polygon_ptr_array_iterator_type> ();
    } else if (m_type == PathPtrArray) {
      skip_array_iter<path_ptr_array_iterator_type> ();
    } else if (m_type == TextPtrArray) {
      skip_array_iter<text_ptr_array_iterator_type> ();
    } else if (m_type == BoxArray) {
      skip_array_iter<box_array_iterator_type> ();
    } else if (m_type == ShortBoxArray) {
      skip_array_iter<short_box_array_iterator_type> ();
    }
    m_array_iterator_valid = false;
  }
}

template <class Sh, class StableTag, class RegionTag>
struct advance_algorithm_traits;

template <class Sh, class StableTag>
struct advance_algorithm_traits<Sh, StableTag, ShapeIterator::NoRegionTag>
{
  typedef typename db::layer<Sh, StableTag>::flat_iterator iterator_type;
  typedef typename db::layer<db::object_with_properties<Sh>, StableTag >::flat_iterator iterator_with_props_type;

  inline static void advance (iterator_type *iter, int /*mode*/)
  {
    ++*iter;
  }

  inline static void advance (iterator_with_props_type *iter, int /*mode*/)
  {
    ++*iter;
  }

  inline static size_t quad_id (iterator_type * /*iter*/)
  {
    return 0;
  }

  inline static size_t quad_id (iterator_with_props_type * /*iter*/)
  {
    return 0;
  }

  inline static iterator_type begin (const db::Shapes *shapes, const db::Box & /*box*/)
  {
    //  use get_layer().begin..() in order to suppress update() - this might change the container
    //  while iterating.
    return shapes->template get_layer <Sh, StableTag> ().begin_flat ();
  }

  inline static iterator_with_props_type begin_with_props (const db::Shapes *shapes, const db::Box & /*box*/)
  {
    //  use get_layer().begin..() in order to suppress update() - this might change the container
    //  while iterating.
    return shapes->template get_layer <db::object_with_properties <Sh>, StableTag> ().begin_flat ();
  }
};

template <class Sh, class StableTag>
struct advance_algorithm_traits<Sh, StableTag, ShapeIterator::TouchingRegionTag>
{
  typedef typename db::layer<Sh, StableTag>::touching_iterator iterator_type;
  typedef typename db::layer<db::object_with_properties<Sh>, StableTag >::touching_iterator iterator_with_props_type;

  inline static void advance (iterator_type *iter, int mode)
  {
    if (mode > 0) {
      ++*iter;
    } else {
      iter->skip_quad ();
    }
  }

  inline static void advance (iterator_with_props_type *iter, int mode)
  {
    if (mode > 0) {
      ++*iter;
    } else {
      iter->skip_quad ();
    }
  }

  inline static size_t quad_id (iterator_type *iter)
  {
    return iter->quad_id ();
  }

  inline static size_t quad_id (iterator_with_props_type *iter)
  {
    return iter->quad_id ();
  }

  inline static iterator_type begin (const db::Shapes *shapes, const db::Box &box)
  {
    //  use get_layer().begin..() in order to suppress update() - this might change the container
    //  while iterating.
    return shapes->template get_layer <Sh, StableTag> ().begin_touching (box);
  }

  inline static iterator_with_props_type begin_with_props (const db::Shapes *shapes, const db::Box &box)
  {
    //  use get_layer().begin..() in order to suppress update() - this might change the container
    //  while iterating.
    return shapes->template get_layer <db::object_with_properties <Sh>, StableTag> ().begin_touching (box);
  }
};

template <class Sh, class StableTag>
struct advance_algorithm_traits<Sh, StableTag, ShapeIterator::OverlappingRegionTag>
{
  typedef typename db::layer<Sh, StableTag>::overlapping_iterator iterator_type;
  typedef typename db::layer<db::object_with_properties<Sh>, StableTag >::overlapping_iterator iterator_with_props_type;

  inline static void advance (iterator_type *iter, int mode)
  {
    if (mode > 0) {
      ++*iter;
    } else {
      iter->skip_quad ();
    }
  }

  inline static void advance (iterator_with_props_type *iter, int mode)
  {
    if (mode > 0) {
      ++*iter;
    } else {
      iter->skip_quad ();
    }
  }

  inline static size_t quad_id (iterator_type *iter)
  {
    return iter->quad_id ();
  }

  inline static size_t quad_id (iterator_with_props_type *iter)
  {
    return iter->quad_id ();
  }

  inline static iterator_type begin (const db::Shapes *shapes, const db::Box &box)
  {
    //  use get_layer().begin..() in order to suppress update() - this might change the container
    //  while iterating.
    return shapes->template get_layer <Sh, StableTag> ().begin_overlapping (box);
  }

  inline static iterator_with_props_type begin_with_props (const db::Shapes *shapes, const db::Box &box)
  {
    //  use get_layer().begin..() in order to suppress update() - this might change the container
    //  while iterating.
    return shapes->template get_layer <db::object_with_properties <Sh>, StableTag> ().begin_overlapping (box);
  }
};

template <class Sh, class StableTag, class RegionTag>
bool
ShapeIterator::advance_shape (int &mode)
{
  typedef advance_algorithm_traits<Sh, StableTag, RegionTag> algorithm_traits;
  typedef typename algorithm_traits::iterator_type iterator_type;
  typedef typename algorithm_traits::iterator_with_props_type iterator_with_props_type;

  if (mode) {

    tl_assert (m_valid);

    if (! m_with_props) {
      iterator_type *iter = (iterator_type *) m_d.iter;
      algorithm_traits::advance (iter, mode);
    } else {
      iterator_with_props_type *iter = (iterator_with_props_type *) m_d.iter;
      do {
        algorithm_traits::advance (iter, mode);
      } while (mp_prop_sel && ! iter->at_end () && (mp_prop_sel->find ((*iter)->properties_id ()) == mp_prop_sel->end ()) != m_inv_prop_sel);
    }

    //  further steps are validation only
    mode = 0;

  }

  bool sel = (m_flags & (1 << (unsigned int) m_type)) != 0;
  bool props_only = (m_flags & Properties) != 0;

  if (! m_with_props && ! props_only) {

    iterator_type *iter = (iterator_type *) m_d.iter;

    if (!m_valid && sel) {
      iterator_type i = algorithm_traits::begin (mp_shapes, m_box);
      if (! i.at_end ()) {
        new (iter) iterator_type (i);
        m_valid = true;
      }
    }

    if (m_valid) {
      if (!sel || iter->at_end ()) {
        m_valid = false;
      } else {
        m_shape = iterator_to_shape (mp_shapes, mp_shapes->template get_layer<Sh, StableTag> (), *iter);
        m_quad_id = algorithm_traits::quad_id (iter);
        return true;
      }
    }

  }

  m_with_props = true;

  {

    iterator_with_props_type *iter = (iterator_with_props_type *) m_d.iter;

    if (!m_valid && sel) {
      //  use get_layer().begin_flat() in order to suppress update() - this might change the container
      //  while iterating.
      iterator_with_props_type i = algorithm_traits::begin_with_props (mp_shapes, m_box);
      if (mp_prop_sel) {
        while (! i.at_end () && (mp_prop_sel->find (i->properties_id ()) == mp_prop_sel->end ()) != m_inv_prop_sel) {
          ++i;
        }
      }
      if (! i.at_end ()) {
        new (iter) iterator_with_props_type (i);
        m_valid = true;
      }
    }

    if (m_valid) {
      if (!sel || iter->at_end ()) {
        m_valid = false;
      } else {
        m_shape = iterator_to_shape (mp_shapes, mp_shapes->template get_layer<db::object_with_properties<Sh>, StableTag> (), *iter);
        m_quad_id = algorithm_traits::quad_id (iter);
        return true;
      }
    }

  }

  m_with_props = false;
  return false;
}

template <class Array>
void
ShapeIterator::init_array_iter (typename ShapeIterator::NoRegionTag)
{
  typedef typename Array::iterator array_iterator;

  array_iterator *arr_iter = (array_iterator *) m_ad.iter;
  if (m_with_props) {
    new (arr_iter) array_iterator (m_array.basic_ptr (typename db::object_with_properties<Array>::tag ())->begin ());
  } else {
    new (arr_iter) array_iterator (m_array.basic_ptr (typename Array::tag ())->begin ());
  }
}

template <class Array>
void
ShapeIterator::init_array_iter (typename ShapeIterator::TouchingRegionTag)
{
  typedef typename Array::iterator array_iterator;
  typedef typename Array::object_type shape_ptr_type;

  array_iterator *arr_iter = (array_iterator *) m_ad.iter;
  db::box_convert<shape_ptr_type> bc;
  new (arr_iter) array_iterator (m_array.basic_ptr (typename Array::tag ())->begin_touching (m_box, bc));
}

template <class Array>
void
ShapeIterator::init_array_iter (typename ShapeIterator::OverlappingRegionTag)
{
  typedef typename Array::iterator array_iterator;
  typedef typename Array::object_type shape_ptr_type;

  array_iterator *arr_iter = (array_iterator *) m_ad.iter;
  db::box_convert<shape_ptr_type> bc;
  box_type box (m_box);
  box.enlarge (vector_type (-1, -1));
  new (arr_iter) array_iterator (m_array.basic_ptr (typename Array::tag ())->begin_touching (m_box, bc));
}

template <class Array, class StableTag, class RegionTag>
bool
ShapeIterator::advance_aref (int &mode)
{
  typedef typename Array::iterator array_iterator;

  if (mode && m_array_iterator_valid) {

    if (mode == 1) {
      array_iterator *arr_iter = (array_iterator *) m_ad.iter;
      ++*arr_iter;
    } else if (mode == 2) {
      //  skip array quad -> skip rest of array quad and move to shape in the next quad or to end
      do_skip_array_quad ();
      mode = 1;
    } else {
      //  skip quad -> skip rest of array and move to next shape array
      skip_array ();  //  sets m_array_iterator_valid = false
    }

  }

  while (true) {

    if (m_array_iterator_valid) {
      array_iterator *arr_iter = (array_iterator *) m_ad.iter;
      if (! arr_iter->at_end ()) {
        break;
      } else {
        arr_iter->~array_iterator ();
        m_array_iterator_valid = false;
        mode = 1; // force move to next item in increment mode
      }
    }

    //  move to next item (increment on mode == 1, skip quad on mode == -1) or validate this one (if mode == 0)
    if (! advance_shape<Array, StableTag, RegionTag> (mode)) {
      return false;
    }

    m_array = m_shape;
    RegionTag region_tag;
    init_array_iter <Array> (region_tag);
    m_array_iterator_valid = true;

  }

  array_iterator *arr_iter = (array_iterator *) m_ad.iter;
  typename array_iterator::result_type t = **arr_iter;

  //  HINT: since the array references store "pointers" without an intrinsic
  //        transformation, we can drop this:
  //  t = t * (*iter)->obj ().trans ();

  //  This creates a local reference object to reference an array member
  if (m_editable) {
    if (m_with_props) {
      m_shape = shape_type (mp_shapes, m_array.basic_iter (typename db::object_with_properties<Array>::tag ()), t);
    } else {
      m_shape = shape_type (mp_shapes, m_array.basic_iter (typename Array::tag ()), t);
    }
  } else {
    if (m_with_props) {
      m_shape = shape_type (mp_shapes, *m_array.basic_ptr (typename db::object_with_properties<Array>::tag ()), t);
    } else {
      m_shape = shape_type (mp_shapes, *m_array.basic_ptr (typename Array::tag ()), t);
    }
  }

  return true;
}

template <class RegionTag, class StableTag>
void
ShapeIterator::advance_generic (int mode)
{
  while (m_type != Null) {

    switch (m_type) {
    case Polygon:
      if (advance_shape<polygon_type, StableTag, RegionTag> (mode)) return;
      break;
    case PolygonRef:
      if (advance_shape<polygon_ref_type, StableTag, RegionTag> (mode)) return;
      break;
    case PolygonPtrArray:
      if (advance_aref<polygon_ptr_array_type, StableTag, RegionTag> (mode)) return;
      break;
    case SimplePolygon:
      if (advance_shape<simple_polygon_type, StableTag, RegionTag> (mode)) return;
      break;
    case SimplePolygonRef:
      if (advance_shape<simple_polygon_ref_type, StableTag, RegionTag> (mode)) return;
      break;
    case SimplePolygonPtrArray:
      if (advance_aref<simple_polygon_ptr_array_type, StableTag, RegionTag> (mode)) return;
      break;
    case Edge:
      if (advance_shape<edge_type, StableTag, RegionTag> (mode)) return;
      break;
    case EdgePair:
      if (advance_shape<edge_pair_type, StableTag, RegionTag> (mode)) return;
      break;
    case Point:
      if (advance_shape<point_type, StableTag, RegionTag> (mode)) return;
      break;
    case Path:
      if (advance_shape<path_type, StableTag, RegionTag> (mode)) return;
      break;
    case PathRef:
      if (advance_shape<path_ref_type, StableTag, RegionTag> (mode)) return;
      break;
    case PathPtrArray:
      if (advance_aref<path_ptr_array_type, StableTag, RegionTag> (mode)) return;
      break;
    case Box:
      if (advance_shape<box_type, StableTag, RegionTag> (mode)) return;
      break;
    case BoxArray:
      if (advance_aref<box_array_type, StableTag, RegionTag> (mode)) return;
      break;
    case ShortBox:
      if (advance_shape<short_box_type, StableTag, RegionTag> (mode)) return;
      break;
    case ShortBoxArray:
      if (advance_aref<short_box_array_type, StableTag, RegionTag> (mode)) return;
      break;
    case Text:
      if (advance_shape<text_type, StableTag, RegionTag> (mode)) return;
      break;
    case TextRef:
      if (advance_shape<text_ref_type, StableTag, RegionTag> (mode)) return;
      break;
    case TextPtrArray:
      if (advance_aref<text_ptr_array_type, StableTag, RegionTag> (mode)) return;
      break;
    case UserObject:
      if (advance_shape<user_object_type, StableTag, RegionTag> (mode)) return;
      break;
    default:
      break;
    }

    //  look for the next type selected
    m_type = object_type ((unsigned int) m_type + 1);
    for (unsigned int m = 1 << (unsigned int) m_type; m_type != Null && (m_flags & m) == 0; m <<= 1) {
      m_type = object_type ((unsigned int) m_type + 1);
    }

  }
}

void
ShapeIterator::finish_array ()
{
  skip_array ();
  advance (1);
}

void
ShapeIterator::advance (int mode)
{
  if (m_editable) {
    if (m_region_mode == None) {
      advance_generic<NoRegionTag, db::stable_layer_tag> (mode);
    } else if (m_region_mode == Touching) {
      advance_generic<TouchingRegionTag, db::stable_layer_tag> (mode);
    } else if (m_region_mode == Overlapping) {
      advance_generic<OverlappingRegionTag, db::stable_layer_tag> (mode);
    }
  } else {
    if (m_region_mode == None) {
      advance_generic<NoRegionTag, db::unstable_layer_tag> (mode);
    } else if (m_region_mode == Touching) {
      advance_generic<TouchingRegionTag, db::unstable_layer_tag> (mode);
    } else if (m_region_mode == Overlapping) {
      advance_generic<OverlappingRegionTag, db::unstable_layer_tag> (mode);
    }
  }
}

template <class Sh, class StableTag>
db::Box
ShapeIterator::quad_box_by_shape (typename ShapeIterator::TouchingRegionTag) const
{
  tl_assert (m_valid);
  if (! m_with_props) {
    typename db::layer<Sh, StableTag>::touching_iterator *iter = (typename db::layer<Sh, StableTag>::touching_iterator *) m_d.iter;
    return iter->quad_box ();
  } else {
    typename db::layer<db::object_with_properties<Sh>, StableTag >::touching_iterator *iter = (typename db::layer< db::object_with_properties<Sh>, StableTag>::touching_iterator *) m_d.iter;
    return iter->quad_box ();
  }
}

template <class Sh, class StableTag>
db::Box
ShapeIterator::quad_box_by_shape (typename ShapeIterator::OverlappingRegionTag) const
{
  tl_assert (m_valid);
  if (! m_with_props) {
    typename db::layer<Sh, StableTag>::overlapping_iterator *iter = (typename db::layer<Sh, StableTag>::overlapping_iterator *) m_d.iter;
    return iter->quad_box ();
  } else {
    typename db::layer<db::object_with_properties<Sh>, StableTag >::overlapping_iterator *iter = (typename db::layer< db::object_with_properties<Sh>, StableTag>::overlapping_iterator *) m_d.iter;
    return iter->quad_box ();
  }
}

template <class RegionTag, class StableTag>
db::Box
ShapeIterator::quad_box_generic () const
{
  RegionTag region_tag = RegionTag ();

  switch (m_type) {
  case Polygon:
    return (quad_box_by_shape<polygon_type, StableTag> (region_tag));
  case PolygonRef:
    return (quad_box_by_shape<polygon_ref_type, StableTag> (region_tag));
  case PolygonPtrArray:
    return (quad_box_by_shape<polygon_ptr_array_type, StableTag> (region_tag));
  case SimplePolygon:
    return (quad_box_by_shape<simple_polygon_type, StableTag> (region_tag));
  case SimplePolygonRef:
    return (quad_box_by_shape<simple_polygon_ref_type, StableTag> (region_tag));
  case SimplePolygonPtrArray:
    return (quad_box_by_shape<simple_polygon_ptr_array_type, StableTag> (region_tag));
  case Edge:
    return (quad_box_by_shape<edge_type, StableTag> (region_tag));
  case EdgePair:
    return (quad_box_by_shape<edge_pair_type, StableTag> (region_tag));
  case Point:
    return (quad_box_by_shape<point_type, StableTag> (region_tag));
  case Path:
    return (quad_box_by_shape<path_type, StableTag> (region_tag));
  case PathRef:
    return (quad_box_by_shape<path_ref_type, StableTag> (region_tag));
  case PathPtrArray:
    return (quad_box_by_shape<path_ptr_array_type, StableTag> (region_tag));
  case Box:
    return (quad_box_by_shape<box_type, StableTag> (region_tag));
  case BoxArray:
    return (quad_box_by_shape<box_array_type, StableTag> (region_tag));
  case ShortBox:
    return (quad_box_by_shape<short_box_type, StableTag> (region_tag));
  case ShortBoxArray:
    return (quad_box_by_shape<short_box_array_type, StableTag> (region_tag));
  case Text:
    return (quad_box_by_shape<text_type, StableTag> (region_tag));
  case TextRef:
    return (quad_box_by_shape<text_ref_type, StableTag> (region_tag));
  case TextPtrArray:
    return (quad_box_by_shape<text_ptr_array_type, StableTag> (region_tag));
  case UserObject:
    return (quad_box_by_shape<user_object_type, StableTag> (region_tag));
  default:
    return db::Box ();
  }
}

db::Box
ShapeIterator::quad_box () const
{
  if (m_editable) {
    if (m_region_mode == None) {
      return db::Box::world ();
    } else if (m_region_mode == Touching) {
      return quad_box_generic<TouchingRegionTag, db::stable_layer_tag> ();
    } else if (m_region_mode == Overlapping) {
      return quad_box_generic<OverlappingRegionTag, db::stable_layer_tag> ();
    }
  } else {
    if (m_region_mode == None) {
      return db::Box::world ();
    } else if (m_region_mode == Touching) {
      return quad_box_generic<TouchingRegionTag, db::unstable_layer_tag> ();
    } else if (m_region_mode == Overlapping) {
      return quad_box_generic<OverlappingRegionTag, db::unstable_layer_tag> ();
    }
  }

  return db::Box ();
}

template <class Iter>
void
ShapeIterator::do_skip_array_quad_iter ()
{
  Iter *arr_iter = (Iter *) m_ad.iter;
  arr_iter->skip_quad ();
}

void
ShapeIterator::do_skip_array_quad ()
{
  if (m_array_iterator_valid) {
    if (m_type == PolygonPtrArray) {
      do_skip_array_quad_iter<polygon_ptr_array_iterator_type> ();
    } else if (m_type == SimplePolygonPtrArray) {
      do_skip_array_quad_iter<simple_polygon_ptr_array_iterator_type> ();
    } else if (m_type == PathPtrArray) {
      do_skip_array_quad_iter<path_ptr_array_iterator_type> ();
    } else if (m_type == TextPtrArray) {
      do_skip_array_quad_iter<text_ptr_array_iterator_type> ();
    } else if (m_type == BoxArray) {
      do_skip_array_quad_iter<box_array_iterator_type> ();
    } else if (m_type == ShortBoxArray) {
      do_skip_array_quad_iter<short_box_array_iterator_type> ();
    }
  }
}

template <class Iter>
size_t
ShapeIterator::get_array_quad_id () const
{
  Iter *arr_iter = (Iter *) m_ad.iter;
  return arr_iter->quad_id ();
}

size_t
ShapeIterator::array_quad_id () const
{
  if (m_array_iterator_valid) {
    if (m_type == PolygonPtrArray) {
      return get_array_quad_id<polygon_ptr_array_iterator_type> ();
    } else if (m_type == SimplePolygonPtrArray) {
      return get_array_quad_id<simple_polygon_ptr_array_iterator_type> ();
    } else if (m_type == PathPtrArray) {
      return get_array_quad_id<path_ptr_array_iterator_type> ();
    } else if (m_type == TextPtrArray) {
      return get_array_quad_id<text_ptr_array_iterator_type> ();
    } else if (m_type == BoxArray) {
      return get_array_quad_id<box_array_iterator_type> ();
    } else if (m_type == ShortBoxArray) {
      return get_array_quad_id<short_box_array_iterator_type> ();
    }
  }

  return 0;
}

template <class Iter, class Array>
db::Box
ShapeIterator::get_array_quad_box () const
{
  const Array *arr = m_array.basic_ptr (typename Array::tag ());
  Iter *arr_iter = (Iter *) m_ad.iter;
  db::box_convert<typename Array::object_type> bc;
  return arr->quad_box (*arr_iter, bc);
}

db::Box
ShapeIterator::array_quad_box () const
{
  if (m_array_iterator_valid) {
    if (m_type == PolygonPtrArray) {
      return get_array_quad_box<polygon_ptr_array_iterator_type, polygon_ptr_array_type> ();
    } else if (m_type == SimplePolygonPtrArray) {
      return get_array_quad_box<simple_polygon_ptr_array_iterator_type, simple_polygon_ptr_array_type> ();
    } else if (m_type == PathPtrArray) {
      return get_array_quad_box<path_ptr_array_iterator_type, path_ptr_array_type> ();
    } else if (m_type == TextPtrArray) {
      return get_array_quad_box<text_ptr_array_iterator_type, text_ptr_array_type> ();
    } else if (m_type == BoxArray) {
      return get_array_quad_box<box_array_iterator_type, box_array_type> ();
    } else if (m_type == ShortBoxArray) {
      return get_array_quad_box<short_box_array_iterator_type, short_box_array_type> ();
    }
  }

  return db::Box::world ();
}

void
ShapeIterator::cleanup ()
{
  //  this trick destroys all iterators that have been allocated in the generic union
  if (m_type != Null) {

    skip_array ();
    m_flags = 0;
    advance (0);
    tl_assert (m_type == Null);

  }
}

}
