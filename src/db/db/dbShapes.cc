
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
#include "dbTrans.h"
#include "dbUserObject.h"
#include "dbLayout.h"

#include <limits>

namespace db
{

// -------------------------------------------------------------------------------

LayerBase::LayerBase ()
{
  //  .. nothing yet ..
}

//  instantiates the vtable
LayerBase::~LayerBase ()
{
  //  .. nothing yet ..
}

void
LayerBase::mem_stat (MemStatistics * /*stat*/, MemStatistics::purpose_t /*purpose*/, int /*cat*/, bool /*no_self*/, void * /*parent*/) const
{
  //  .. nothing yet ..
}

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
  return layer.begin () + (shape.basic_ptr (typename Sh::tag ()) - &*layer.begin ());
}

/**
 *  @brief A predicate returning true if the shape needs translation through translate ()
 */
template <class Sh>
inline bool needs_translate (object_tag<Sh> /*tag*/)
{
  return tl::is_equal_type<typename shape_traits<Sh>::can_deref, tl::True> () || tl::is_equal_type<typename shape_traits<Sh>::is_array, tl::True> ();
}

inline bool type_mask_applies (const db::LayerBase *layer, unsigned int flags)
{
  unsigned int tm = layer->type_mask ();
  return (((flags & db::ShapeIterator::Properties) == 0 || (tm & db::ShapeIterator::Properties) != 0) && (flags & tm) != 0);
}

// ---------------------------------------------------------------------------------------
//  layer_op implementation

template <class Sh, class StableTag>
void 
layer_op<Sh, StableTag>::insert (Shapes *shapes)
{
  shapes->insert (m_shapes.begin (), m_shapes.end ());
}

template <class Sh, class StableTag>
void 
layer_op<Sh, StableTag>::erase (Shapes *shapes)
{
  if (shapes->size (typename Sh::tag (), StableTag ()) <= m_shapes.size ()) {
    //  If all shapes are to be removed, just clear the shapes
    shapes->erase (typename Sh::tag (), StableTag (), shapes->begin (typename Sh::tag (), StableTag ()), shapes->end (typename Sh::tag (), StableTag ()));
  } else {

    //  look up the shapes to delete and collect them in a sorted list. Then pass this to 
    //  the erase method of the shapes object
    std::vector<bool> done;
    done.resize (m_shapes.size (), false);

    std::sort (m_shapes.begin (), m_shapes.end ());

    typename std::vector<Sh>::const_iterator s_begin = m_shapes.begin ();
    typename std::vector<Sh>::const_iterator s_end = m_shapes.end ();

    std::vector<typename db::layer<Sh, StableTag>::iterator> to_erase;
    to_erase.reserve (m_shapes.size ());

    //  This is not quite effective but seems to be the simplest way
    //  of implementing this: search for each element and erase these.
    for (typename db::layer<Sh, StableTag>::iterator lsh = shapes->begin (typename Sh::tag (), StableTag ()); lsh != shapes->end (typename Sh::tag (), StableTag ()); ++lsh) {
      typename std::vector<Sh>::const_iterator s = std::lower_bound (s_begin, s_end, *lsh);
      while (s != s_end && done [std::distance(s_begin, s)] && *s == *lsh) {
        ++s;
      }
      if (s != s_end && *s == *lsh) {
        done [std::distance(s_begin, s)] = true;
        to_erase.push_back (lsh);
      }
    }

    shapes->erase_positions (typename Sh::tag (), StableTag (), to_erase.begin (), to_erase.end ());

  }
}

// ---------------------------------------------------------------------------------------
//  FullLayerOp implementation

void
FullLayerOp::insert (Shapes *shapes)
{
  for (tl::vector<LayerBase *>::iterator l = shapes->get_layers ().end (); l != shapes->get_layers ().begin (); ) {

    --l;

    if (*l == mp_layer) {

      return;

    } else if ((*l)->is_same_type (mp_layer)) {

      delete (*l);
      *l = mp_layer;
      m_owns_layer = false;
      shapes->invalidate_state ();
      return;

    }

  }

  shapes->get_layers ().push_back (mp_layer);
  shapes->invalidate_state ();
  m_owns_layer = false;
}

void
FullLayerOp::erase (Shapes *shapes)
{
  for (tl::vector<LayerBase *>::iterator l = shapes->get_layers ().begin (); l != shapes->get_layers ().end (); ++l) {
    if (*l == mp_layer) {
      shapes->get_layers ().erase (l);
      shapes->invalidate_state ();
      m_owns_layer = true;
      break;
    }
  }
}

// ---------------------------------------------------------------------------------------
//  Shapes implementation

Shapes &
Shapes::operator= (const Shapes &d)
{
  if (&d != this) {
    clear ();
    if (! d.empty()) {
      invalidate_state ();
      do_insert (d);
    }
  }

  return *this;
}

db::Layout *
Shapes::layout () const 
{
  db::Cell *c = cell ();
  return c ? c->layout () : 0;
}

void
Shapes::check_is_editable_for_undo_redo () const
{
  if (! is_editable ()) {
    throw tl::Exception (tl::to_string (tr ("No undo/redo support on non-editable shape lists")));
  }
}

void
Shapes::insert (const Shapes &d)
{
  do_insert (d);
}

void
Shapes::insert (const Shapes &d, unsigned int flags)
{
  do_insert (d, flags);
}

void
Shapes::do_insert (const Shapes &d, unsigned int flags)
{
  //  shortcut for "nothing to do"
  if (d.empty ()) {
    return;
  }

  if (layout () == d.layout ()) {

    //  both shape containers reside in the same repository space - simply copy
    if (m_layers.empty ()) {

      m_layers.reserve (d.m_layers.size ());
      for (tl::vector<LayerBase *>::const_iterator l = d.m_layers.begin (); l != d.m_layers.end (); ++l) {
        if (type_mask_applies (*l, flags)) {
          m_layers.push_back ((*l)->clone ());
          if (manager () && manager ()->transacting ()) {
            check_is_editable_for_undo_redo ();
            manager ()->queue (this, new FullLayerOp (true, m_layers.back ()));
          }
        }
      }

      invalidate_state ();

    } else {
      for (tl::vector<LayerBase *>::const_iterator l = d.m_layers.begin (); l != d.m_layers.end (); ++l) {
        if (type_mask_applies (*l, flags)) {
          (*l)->insert_into (this);
        }
      }
    }

  } else if (layout () == 0) {

    //  the target is standalone - dereference
    for (tl::vector<LayerBase *>::const_iterator l = d.m_layers.begin (); l != d.m_layers.end (); ++l) {
      if (type_mask_applies (*l, flags)) {
        (*l)->deref_into (this);
      }
    }

  } else {

    //  both shape containers are in separate spaces - translate
    for (tl::vector<LayerBase *>::const_iterator l = d.m_layers.begin (); l != d.m_layers.end (); ++l) {
      if (type_mask_applies (*l, flags)) {
        (*l)->translate_into (this, shape_repository (), array_repository ());
      }
    }

  }
}

//  get the shape repository associated with this container
db::GenericRepository &
Shapes::shape_repository () const 
{
  return layout ()->shape_repository ();
}

//  get the array repository associated with this container
db::ArrayRepository &
Shapes::array_repository () const 
{
  return layout ()->array_repository ();
}

void
Shapes::invalidate_state ()
{
  if (! is_dirty ()) {
    set_dirty (true);
    if (layout () && cell ()) {
      unsigned int index = cell ()->index_of_shapes (this);
      if (index != std::numeric_limits<unsigned int>::max ()) {
        layout ()->invalidate_bboxes (index);
      }
    }
  }
}

void  
Shapes::swap (Shapes &d)
{
  // HINT: undo support for swap is implemented one level above (i.e. in the cell) since
  // two Shapes objects are involved.
  d.invalidate_state ();  //  HINT: must come before the change is done!
  invalidate_state ();
  m_layers.swap (d.m_layers);
}

static
Shapes::shape_type safe_insert_text (Shapes &shapes, const Shapes::shape_type &shape, tl::func_delegate_base <db::properties_id_type> &pm)
{
  //  for texts referring to a string repository we go the safe way and
  //  simply instantiate and re-insert the text:
  Shapes::shape_type::text_type p;
  shape.text (p);
  if (! shape.has_prop_id ()) {
    return shapes.insert (p);
  } else {
    return shapes.insert (db::object_with_properties<Shapes::shape_type::text_type> (p, pm (shape.prop_id ())));
  }
}

Shapes::shape_type 
Shapes::do_insert (const Shapes::shape_type &shape, const Shapes::unit_trans_type & /*t*/, tl::func_delegate_base <db::properties_id_type> &pm)
{
  switch (shape.m_type) {
  case shape_type::Null:
  default:
    return shape_type ();
  case shape_type::Polygon:
    return shape_type (insert_by_tag (shape_type::polygon_type::tag (), shape, pm));
  case shape_type::PolygonRef:
  case shape_type::PolygonPtrArrayMember:
    if (! layout ()) {
      shape_type::polygon_type p;
      shape.polygon (p);
      if (! shape.has_prop_id ()) {
        return insert (p);
      } else {
        return insert (db::object_with_properties<shape_type::polygon_type> (p, pm (shape.prop_id ())));
      }
    } else if (shape.m_type == shape_type::PolygonRef) {
      return shape_type (insert_by_tag (shape_type::polygon_ref_type::tag (), shape, shape_repository (), pm));
    } else {
      shape_type::polygon_ref_type s (shape.polygon_ref ());
      if (! shape.has_prop_id ()) {
        return insert (shape_type::polygon_ref_type (s, shape_repository ()));
      } else {
        typedef db::object_with_properties<shape_type::polygon_ref_type> swp_type;
        return insert (swp_type (shape_type::polygon_ref_type (s, shape_repository ()), pm (shape.prop_id ())));
      }
    }
  case shape_type::PolygonPtrArray:
    tl_assert (layout () != 0);  //  cannot translate the array members
    return shape_type (insert_array_by_tag (shape_type::polygon_ptr_array_type::tag (), shape, shape_repository (), pm));
  case shape_type::SimplePolygon:
    return (insert_by_tag (shape_type::simple_polygon_type::tag (), shape, pm));
  case shape_type::SimplePolygonRef:
  case shape_type::SimplePolygonPtrArrayMember:
    if (! layout ()) {
      shape_type::simple_polygon_type p;
      shape.simple_polygon (p);
      if (! shape.has_prop_id ()) {
        return insert (p);
      } else {
        return insert (db::object_with_properties<shape_type::simple_polygon_type> (p, pm (shape.prop_id ())));
      }
    } else if (shape.m_type == shape_type::SimplePolygonRef) {
      return (insert_by_tag (shape_type::simple_polygon_ref_type::tag (), shape, shape_repository (), pm));
    } else {
      shape_type::simple_polygon_ref_type s (shape.simple_polygon_ref ());
      if (! shape.has_prop_id ()) {
        return insert (shape_type::simple_polygon_ref_type (s, shape_repository ()));
      } else {
        typedef db::object_with_properties<shape_type::simple_polygon_ref_type> swp_type;
        return insert (swp_type (shape_type::simple_polygon_ref_type (s, shape_repository ()), pm (shape.prop_id ())));
      }
    }
  case shape_type::SimplePolygonPtrArray:
    tl_assert (layout () != 0);  //  cannot translate the array members
    return (insert_array_by_tag (shape_type::simple_polygon_ptr_array_type::tag (), shape, shape_repository (), pm));
  case shape_type::Edge:
    return (insert_by_tag (shape_type::edge_type::tag (), shape, pm));
  case shape_type::EdgePair:
    return (insert_by_tag (shape_type::edge_pair_type::tag (), shape, pm));
  case shape_type::Point:
    return (insert_by_tag (shape_type::point_type::tag (), shape, pm));
  case shape_type::Path:
    return (insert_by_tag (shape_type::path_type::tag (), shape, pm));
  case shape_type::PathRef:
  case shape_type::PathPtrArrayMember:
    if (! layout ()) {
      shape_type::path_type p;
      shape.path (p);
      if (! shape.has_prop_id ()) {
        return insert (p);
      } else {
        return insert (db::object_with_properties<shape_type::path_type> (p, pm (shape.prop_id ())));
      }
    } else if (shape.m_type == shape_type::PathRef) {
      return (insert_by_tag (shape_type::path_ref_type::tag (), shape, shape_repository (), pm));
    } else {
      shape_type::path_ref_type s (shape.path_ref ());
      if (! shape.has_prop_id ()) {
        return insert (shape_type::path_ref_type (s, shape_repository ()));
      } else {
        typedef db::object_with_properties<shape_type::path_ref_type> swp_type;
        return insert (swp_type (shape_type::path_ref_type (s, shape_repository ()), pm (shape.prop_id ())));
      }
    }
  case shape_type::PathPtrArray:
    tl_assert (layout () != 0);  //  cannot translate the array members
    return (insert_array_by_tag (shape_type::path_ptr_array_type::tag (), shape, shape_repository (), pm));
  case shape_type::Box:
    return (insert_by_tag (shape_type::box_type::tag (), shape, pm));
  case shape_type::BoxArrayMember:
    {
      shape_type::box_type s (shape.box ());
      if (! shape.has_prop_id ()) {
        return insert (s);
      } else {
        typedef db::object_with_properties<shape_type::box_type> swp_type;
        return insert (swp_type (s, pm (shape.prop_id ())));
      }
    }
  case shape_type::BoxArray:
    return (insert_by_tag (shape_type::box_array_type::tag (), shape, pm));
  case shape_type::ShortBox:
    return (insert_by_tag (shape_type::short_box_type::tag (), shape, pm));
  case shape_type::ShortBoxArrayMember:
    {
      shape_type::short_box_type s (shape.box ());
      if (! shape.has_prop_id ()) {
        return insert (s);
      } else {
        typedef db::object_with_properties<shape_type::short_box_type> swp_type;
        return insert (swp_type (s, pm (shape.prop_id ())));
      }
    }
  case shape_type::ShortBoxArray:
    return (insert_by_tag (shape_type::short_box_array_type::tag (), shape, pm));
  case shape_type::Text:
    {
      if (shape.text ().string_ref () != 0) {
        return safe_insert_text (*this, shape, pm);
      } else {
        return (insert_by_tag (shape_type::text_type::tag (), shape, pm));
      }
    }
  case shape_type::TextRef:
    {
      if (! layout ()) {
        shape_type::text_type t;
        shape.text (t);
        if (! shape.has_prop_id ()) {
          return insert (t);
        } else {
          return insert (db::object_with_properties<shape_type::text_type> (t, pm (shape.prop_id ())));
        }
      } else if (shape.text_ref ().obj ().string_ref () != 0) {
        return safe_insert_text (*this, shape, pm);
      } else {
        return (insert_by_tag (shape_type::text_ref_type::tag (), shape, shape_repository (), pm));
      }
    }
  case shape_type::TextPtrArrayMember:
    return safe_insert_text (*this, shape, pm);
  case shape_type::TextPtrArray:
    tl_assert (layout () != 0);  //  cannot translate the array members
    return insert_array_by_tag (shape_type::text_ptr_array_type::tag (), shape, shape_repository (), pm);
  case shape_type::UserObject:
    return insert_by_tag (shape_type::user_object_type::tag (), shape, pm);
  };
}

template <class Trans>
Shapes::shape_type 
Shapes::do_insert (const Shapes::shape_type &shape, const Trans &t, tl::func_delegate_base <db::properties_id_type> &pm)
{
  db::properties_id_type new_pid = shape.has_prop_id () ? pm (shape.prop_id ()) : 0;

  switch (shape.m_type) {
  case shape_type::Null:
  default:
    return shape;
  case shape_type::Polygon:
    {
      shape_type::polygon_type p (shape.polygon ());
      //  Hint: we don't compress so we don't loose information
      p.transform (t, false);
      if (new_pid == 0) {
        return insert (p);
      } else {
        return insert (db::object_with_properties<shape_type::polygon_type> (p, new_pid));
      }
    }
  case shape_type::PolygonRef:
  case shape_type::PolygonPtrArrayMember:
    {
      shape_type::polygon_type p;
      shape.polygon (p);
      //  Hint: we don't compress so we don't loose information
      p.transform (t, false);
      //  TODO: could create a reference again, but this is what a transform would to as well.
      if (new_pid == 0) {
        return insert (p);
      } else {
        return insert (db::object_with_properties<shape_type::polygon_type> (p, new_pid));
      }
    }
  case shape_type::SimplePolygon:
    {
      shape_type::simple_polygon_type p (shape.simple_polygon ());
      //  Hint: we don't compress so we don't loose information
      p.transform (t, false);
      if (new_pid == 0) {
        return insert (p);
      } else {
        return insert (db::object_with_properties<shape_type::simple_polygon_type> (p, new_pid));
      }
    }
  case shape_type::SimplePolygonRef:
  case shape_type::SimplePolygonPtrArrayMember:
    {
      shape_type::simple_polygon_type p;
      shape.simple_polygon (p);
      //  Hint: we don't compress so we don't loose information
      p.transform (t, false);
      //  TODO: could create a reference again, but this is what a transform would to as well.
      if (new_pid == 0) {
        return insert (p);
      } else {
        return insert (db::object_with_properties<shape_type::simple_polygon_type> (p, new_pid));
      }
    }
  case shape_type::Edge:
    {
      shape_type::edge_type p (shape.edge ());
      p.transform (t);
      if (new_pid == 0) {
        return insert (p);
      } else {
        return insert (db::object_with_properties<shape_type::edge_type> (p, new_pid));
      }
    }
  case shape_type::Point:
    {
      shape_type::point_type p (shape.point ());
      p = t.trans (p);
      if (new_pid == 0) {
        return insert (p);
      } else {
        return insert (db::object_with_properties<shape_type::point_type> (p, new_pid));
      }
    }
  case shape_type::EdgePair:
    {
      shape_type::edge_pair_type p (shape.edge_pair ());
      p.transform (t);
      if (new_pid == 0) {
        return insert (p);
      } else {
        return insert (db::object_with_properties<shape_type::edge_pair_type> (p, new_pid));
      }
    }
  case shape_type::Path:
    {
      shape_type::path_type p (shape.path ());
      p.transform (t);
      if (new_pid == 0) {
        return insert (p);
      } else {
        return insert (db::object_with_properties<shape_type::path_type> (p, new_pid));
      }
    }
  case shape_type::PathRef:
  case shape_type::PathPtrArrayMember:
    {
      shape_type::path_type p;
      shape.path (p);
      p.transform (t);
      //  TODO: could create a reference again, but this is what a transform would to as well.
      if (new_pid == 0) {
        return insert (p);
      } else {
        return insert (db::object_with_properties<shape_type::path_type> (p, new_pid));
      }
    }
  case shape_type::Box:
  case shape_type::BoxArrayMember:
  case shape_type::ShortBox:
  case shape_type::ShortBoxArrayMember:
    {
      if (t.is_ortho ()) {
        shape_type::box_type p (shape.box ());
        p.transform (t);
        if (new_pid == 0) {
          return insert (p);
        } else {
          return insert (db::object_with_properties<shape_type::box_type> (p, new_pid));
        }
      } else {
        //  A box cannot stay a box in this case ...
        shape_type::simple_polygon_type p (shape.box ());
        p.transform (t);
        if (new_pid == 0) {
          return insert (p);
        } else {
          return insert (db::object_with_properties<shape_type::simple_polygon_type> (p, new_pid));
        }
      }
    }
  case shape_type::Text:
    {
      shape_type::text_type p (shape.text ());
      p.transform (t);
      if (new_pid == 0) {
        return insert (p);
      } else {
        return insert (db::object_with_properties<shape_type::text_type> (p, new_pid));
      }
    }
  case shape_type::TextRef:
  case shape_type::TextPtrArrayMember:
    {
      shape_type::text_type p;
      shape.text (p);
      p.transform (t);
      //  TODO: could create a reference again, but this is what a transform would to as well.
      if (new_pid == 0) {
        return insert (p);
      } else {
        return insert (db::object_with_properties<shape_type::text_type> (p, new_pid));
      }
    }
  case shape_type::UserObject:
    {
      shape_type::user_object_type p (shape.user_object ());
      p.transform (t);
      if (new_pid == 0) {
        return insert (p);
      } else {
        return insert (db::object_with_properties<shape_type::user_object_type> (p, new_pid));
      }
    }
  case shape_type::PolygonPtrArray:
  case shape_type::SimplePolygonPtrArray:
  case shape_type::PathPtrArray:
  case shape_type::BoxArray:
  case shape_type::ShortBoxArray:
  case shape_type::TextPtrArray:
    //  Arrays are not supported yet 
    //  TODO: implement
    throw tl::Exception (tl::to_string (tr ("Function 'insert' with transformation does not support shape arrays")));
  };
}

Shapes::shape_type 
Shapes::find (const Shapes::shape_type &shape) const
{
  switch (shape.m_type) {
  case shape_type::Null:
  default:
    return shape_type ();
  case shape_type::Polygon:
    return find_shape_by_tag (shape_type::polygon_type::tag (), shape);
  case shape_type::PolygonRef:
    return find_shape_by_tag (shape_type::polygon_ref_type::tag (), shape);
  case shape_type::PolygonPtrArrayMember:
  case shape_type::PolygonPtrArray:
    return find_shape_by_tag (shape_type::polygon_ptr_array_type::tag (), shape);
  case shape_type::SimplePolygon:
    return find_shape_by_tag (shape_type::simple_polygon_type::tag (), shape);
  case shape_type::SimplePolygonRef:
    return find_shape_by_tag (shape_type::simple_polygon_ref_type::tag (), shape);
  case shape_type::SimplePolygonPtrArrayMember:
  case shape_type::SimplePolygonPtrArray:
    return find_shape_by_tag (shape_type::simple_polygon_ptr_array_type::tag (), shape);
  case shape_type::Edge:
    return find_shape_by_tag (shape_type::edge_type::tag (), shape);
  case shape_type::EdgePair:
    return find_shape_by_tag (shape_type::edge_pair_type::tag (), shape);
  case shape_type::Point:
    return find_shape_by_tag (shape_type::point_type::tag (), shape);
  case shape_type::Path:
    return find_shape_by_tag (shape_type::path_type::tag (), shape);
  case shape_type::PathRef:
    return find_shape_by_tag (shape_type::path_ref_type::tag (), shape);
  case shape_type::PathPtrArrayMember:
  case shape_type::PathPtrArray:
    return find_shape_by_tag (shape_type::path_ptr_array_type::tag (), shape);
  case shape_type::Box:
    return find_shape_by_tag (shape_type::box_type::tag (), shape);
  case shape_type::BoxArrayMember:
  case shape_type::BoxArray:
    return find_shape_by_tag (shape_type::box_array_type::tag (), shape);
  case shape_type::ShortBox:
    return find_shape_by_tag (shape_type::short_box_type::tag (), shape);
  case shape_type::ShortBoxArrayMember:
  case shape_type::ShortBoxArray:
    return find_shape_by_tag (shape_type::short_box_array_type::tag (), shape);
  case shape_type::Text:
    return find_shape_by_tag (shape_type::text_type::tag (), shape);
  case shape_type::TextRef:
    return find_shape_by_tag (shape_type::text_ref_type::tag (), shape);
  case shape_type::TextPtrArrayMember:
  case shape_type::TextPtrArray:
    return find_shape_by_tag (shape_type::text_ptr_array_type::tag (), shape);
  case shape_type::UserObject:
    return find_shape_by_tag (shape_type::user_object_type::tag (), shape);
  };
}

Shapes::shape_type
Shapes::replace_prop_id (const Shapes::shape_type &ref, db::properties_id_type prop_id)
{
  tl_assert (! ref.is_array_member ());
  if (! is_editable ()) {
    throw tl::Exception (tl::to_string (tr ("Function 'replace_prop_id' is permitted only in editable mode")));
  }

  if (ref.has_prop_id ()) {

    //  this assumes we can simply patch the properties ID ..
    switch (ref.m_type) {
    case shape_type::Null:
      break;
    case shape_type::Polygon:
      replace_prop_id (ref.basic_ptr (object_with_properties<shape_type::polygon_type>::tag ()), prop_id);
      break;
    case shape_type::PolygonRef:
      replace_prop_id (ref.basic_ptr (object_with_properties<shape_type::polygon_ref_type>::tag ()), prop_id);
      break;
    case shape_type::PolygonPtrArray:
      //  HINT: since we are in editing mode, this type should not appear ..
      replace_prop_id (ref.basic_ptr (object_with_properties<shape_type::polygon_ptr_array_type>::tag ()), prop_id);
      break;
    case shape_type::SimplePolygon:
      replace_prop_id (ref.basic_ptr (object_with_properties<shape_type::simple_polygon_type>::tag ()), prop_id);
      break;
    case shape_type::SimplePolygonRef:
      replace_prop_id (ref.basic_ptr (object_with_properties<shape_type::simple_polygon_ref_type>::tag ()), prop_id);
      break;
    case shape_type::SimplePolygonPtrArray:
      //  HINT: since we are in editing mode, this type should not appear ..
      replace_prop_id (ref.basic_ptr (object_with_properties<shape_type::simple_polygon_ptr_array_type>::tag ()), prop_id);
      break;
    case shape_type::Edge:
      replace_prop_id (ref.basic_ptr (object_with_properties<shape_type::edge_type>::tag ()), prop_id);
      break;
    case shape_type::EdgePair:
      replace_prop_id (ref.basic_ptr (object_with_properties<shape_type::edge_pair_type>::tag ()), prop_id);
      break;
    case shape_type::Point:
      replace_prop_id (ref.basic_ptr (object_with_properties<shape_type::point_type>::tag ()), prop_id);
      break;
    case shape_type::Path:
      replace_prop_id (ref.basic_ptr (object_with_properties<shape_type::path_type>::tag ()), prop_id);
      break;
    case shape_type::PathRef:
      replace_prop_id (ref.basic_ptr (object_with_properties<shape_type::path_ref_type>::tag ()), prop_id);
      break;
    case shape_type::PathPtrArray:
      //  HINT: since we are in editing mode, this type should not appear ..
      replace_prop_id (ref.basic_ptr (object_with_properties<shape_type::path_ptr_array_type>::tag ()), prop_id);
      break;
    case shape_type::Box:
      replace_prop_id (ref.basic_ptr (object_with_properties<shape_type::box_type>::tag ()), prop_id);
      break;
    case shape_type::BoxArray:
      //  HINT: since we are in editing mode, this type should not appear ..
      replace_prop_id (ref.basic_ptr (object_with_properties<shape_type::box_array_type>::tag ()), prop_id);
      break;
    case shape_type::ShortBox:
      replace_prop_id (ref.basic_ptr (object_with_properties<shape_type::short_box_type>::tag ()), prop_id);
      break;
    case shape_type::ShortBoxArray:
      //  HINT: since we are in editing mode, this type should not appear ..
      replace_prop_id (ref.basic_ptr (object_with_properties<shape_type::short_box_array_type>::tag ()), prop_id);
      break;
    case shape_type::Text:
      replace_prop_id (ref.basic_ptr (object_with_properties<shape_type::text_type>::tag ()), prop_id);
      break;
    case shape_type::TextRef:
      replace_prop_id (ref.basic_ptr (object_with_properties<shape_type::text_ref_type>::tag ()), prop_id);
      break;
    case shape_type::TextPtrArray:
      //  HINT: since we are in editing mode, this type should not appear ..
      replace_prop_id (ref.basic_ptr (object_with_properties<shape_type::text_ptr_array_type>::tag ()), prop_id);
      break;
    case shape_type::UserObject:
      replace_prop_id (ref.basic_ptr (object_with_properties<shape_type::user_object_type>::tag ()), prop_id);
    default:
      break;
    };

    return ref;

  } else {

    switch (ref.m_type) {
    case shape_type::Null:
      return ref;
    case shape_type::Polygon:
      return replace_prop_id_iter (shape_type::polygon_type::tag (), ref.basic_iter (shape_type::polygon_type::tag ()), prop_id);
    case shape_type::PolygonRef:
      return replace_prop_id_iter (shape_type::polygon_ref_type::tag (), ref.basic_iter (shape_type::polygon_ref_type::tag ()), prop_id);
    case shape_type::PolygonPtrArray:
      return replace_prop_id_iter (shape_type::polygon_ptr_array_type::tag (), ref.basic_iter (shape_type::polygon_ptr_array_type::tag ()), prop_id);
    case shape_type::SimplePolygon:
      return replace_prop_id_iter (shape_type::simple_polygon_type::tag (), ref.basic_iter (shape_type::simple_polygon_type::tag ()), prop_id);
    case shape_type::SimplePolygonRef:
      return replace_prop_id_iter (shape_type::simple_polygon_ref_type::tag (), ref.basic_iter (shape_type::simple_polygon_ref_type::tag ()), prop_id);
    case shape_type::SimplePolygonPtrArray:
      //  HINT: since we are in editing mode, this type should not appear ..
      return replace_prop_id_iter (shape_type::simple_polygon_ptr_array_type::tag (), ref.basic_iter (shape_type::simple_polygon_ptr_array_type::tag ()), prop_id);
    case shape_type::Edge:
      return replace_prop_id_iter (shape_type::edge_type::tag (), ref.basic_iter (shape_type::edge_type::tag ()), prop_id);
    case shape_type::Point:
      return replace_prop_id_iter (shape_type::point_type::tag (), ref.basic_iter (shape_type::point_type::tag ()), prop_id);
    case shape_type::EdgePair:
      return replace_prop_id_iter (shape_type::edge_pair_type::tag (), ref.basic_iter (shape_type::edge_pair_type::tag ()), prop_id);
    case shape_type::Path:
      return replace_prop_id_iter (shape_type::path_type::tag (), ref.basic_iter (shape_type::path_type::tag ()), prop_id);
    case shape_type::PathRef:
      return replace_prop_id_iter (shape_type::path_ref_type::tag (), ref.basic_iter (shape_type::path_ref_type::tag ()), prop_id);
    case shape_type::PathPtrArray:
      //  HINT: since we are in editing mode, this type should not appear ..
      return replace_prop_id_iter (shape_type::path_ptr_array_type::tag (), ref.basic_iter (shape_type::path_ptr_array_type::tag ()), prop_id);
    case shape_type::Box:
      return replace_prop_id_iter (shape_type::box_type::tag (), ref.basic_iter (shape_type::box_type::tag ()), prop_id);
    case shape_type::BoxArray:
      //  HINT: since we are in editing mode, this type should not appear ..
      return replace_prop_id_iter (shape_type::box_array_type::tag (), ref.basic_iter (shape_type::box_array_type::tag ()), prop_id);
    case shape_type::ShortBox:
      return replace_prop_id_iter (shape_type::short_box_type::tag (), ref.basic_iter (shape_type::short_box_type::tag ()), prop_id);
    case shape_type::ShortBoxArray:
      //  HINT: since we are in editing mode, this type should not appear ..
      return replace_prop_id_iter (shape_type::short_box_array_type::tag (), ref.basic_iter (shape_type::short_box_array_type::tag ()), prop_id);
    case shape_type::Text:
      return replace_prop_id_iter (shape_type::text_type::tag (), ref.basic_iter (shape_type::text_type::tag ()), prop_id);
    case shape_type::TextRef:
      return replace_prop_id_iter (shape_type::text_ref_type::tag (), ref.basic_iter (shape_type::text_ref_type::tag ()), prop_id);
    case shape_type::TextPtrArray:
      //  HINT: since we are in editing mode, this type should not appear ..
      return replace_prop_id_iter (shape_type::text_ptr_array_type::tag (), ref.basic_iter (shape_type::text_ptr_array_type::tag ()), prop_id);
    case shape_type::UserObject:
      return replace_prop_id_iter (shape_type::user_object_type::tag (), ref.basic_iter (shape_type::user_object_type::tag ()), prop_id);
    default:
      return ref;
    };

  }
}

template <class Trans>
Shapes::shape_type 
Shapes::transform (const Shapes::shape_type &ref, const Trans &t)
{
  tl_assert (! ref.is_array_member ());
  if (! is_editable ()) {
    throw tl::Exception (tl::to_string (tr ("Function 'transform' is permitted only in editable mode")));
  }

  switch (ref.m_type) {
  case shape_type::Null:
    return ref;
  case shape_type::Polygon:
    {
      shape_type::polygon_type p (ref.polygon ());
      p.transform (t, false /* don't compress */);
      return replace_member_with_props (shape_type::polygon_type::tag (), ref, p);
    }
  case shape_type::PolygonRef:
    {
      shape_type::polygon_type p;
      ref.polygon (p);
      p.transform (t, false /* don't compress */);
      return replace_member_with_props (shape_type::polygon_ref_type::tag (), ref, p);
    }
  case shape_type::SimplePolygon:
    {
      shape_type::simple_polygon_type p (ref.simple_polygon ());
      p.transform (t, false /* don't compress */);
      return replace_member_with_props (shape_type::simple_polygon_type::tag (), ref, p);
    }
  case shape_type::SimplePolygonRef:
    {
      shape_type::simple_polygon_type p;
      ref.simple_polygon (p);
      p.transform (t, false /* don't compress */);
      return replace_member_with_props (shape_type::simple_polygon_ref_type::tag (), ref, p);
    }
  case shape_type::Edge:
    {
      shape_type::edge_type p (ref.edge ());
      p.transform (t);
      return replace_member_with_props (shape_type::edge_type::tag (), ref, p);
    }
  case shape_type::EdgePair:
    {
      shape_type::edge_pair_type p (ref.edge_pair ());
      p.transform (t);
      return replace_member_with_props (shape_type::edge_pair_type::tag (), ref, p);
    }
  case shape_type::Point:
    {
      shape_type::point_type p (ref.point ());
      p = t.trans (p);
      return replace_member_with_props (shape_type::point_type::tag (), ref, p);
    }
  case shape_type::Path:
    {
      shape_type::path_type p (ref.path ());
      p.transform (t);
      return replace_member_with_props (shape_type::path_type::tag (), ref, p);
    }
  case shape_type::PathRef:
    {
      shape_type::path_type p;
      ref.path (p);
      p.transform (t);
      return replace_member_with_props (shape_type::path_ref_type::tag (), ref, p);
    }
  case shape_type::Box:
  case shape_type::ShortBox:
    {
      if (t.is_ortho ()) {
        shape_type::box_type p (ref.box ());
        p.transform (t);
        return replace_member_with_props (shape_type::box_type::tag (), ref, p);
      } else {
        //  A box cannot stay a box in this case ...
        shape_type::simple_polygon_type p (ref.box ());
        p.transform (t);
        return replace_member_with_props (shape_type::box_type::tag (), ref, p);
      }
    }
  case shape_type::Text:
    {
      shape_type::text_type p (ref.text ());
      p.transform (t);
      return replace_member_with_props (shape_type::text_type::tag (), ref, p);
    }
  case shape_type::TextRef:
    {
      shape_type::text_type p;
      ref.text (p);
      p.transform (t);
      return replace_member_with_props (shape_type::text_ref_type::tag (), ref, p);
    }
  case shape_type::UserObject:
    {
      shape_type::user_object_type p (ref.user_object ());
      p.transform (t);
      return replace_member_with_props (shape_type::user_object_type::tag (), ref, p);
    }
  case shape_type::PolygonPtrArray:
  case shape_type::SimplePolygonPtrArray:
  case shape_type::PathPtrArray:
  case shape_type::BoxArray:
  case shape_type::ShortBoxArray:
  case shape_type::TextPtrArray:
    tl_assert (false); //  TODO: not supported yet
  default:
    return ref;
  };
}

template <class Sh>
Shapes::shape_type 
Shapes::replace (const Shapes::shape_type &ref, const Sh &sh)
{
  tl_assert (! ref.is_array_member ());
  if (! is_editable ()) {
    throw tl::Exception (tl::to_string (tr ("Function 'replace' is permitted only in editable mode")));
  }

  switch (ref.m_type) {
  case shape_type::Null:
    return ref;
  case shape_type::Polygon:
    return replace_member_with_props (shape_type::polygon_type::tag (), ref, sh);
  case shape_type::PolygonRef:
    return replace_member_with_props (shape_type::polygon_ref_type::tag (), ref, sh);
  case shape_type::PolygonPtrArray:
    //  HINT: since we are in editing mode, this type should not appear ..
    return replace_member_with_props (shape_type::polygon_ptr_array_type::tag (), ref, sh);
  case shape_type::SimplePolygon:
    return replace_member_with_props (shape_type::simple_polygon_type::tag (), ref, sh);
  case shape_type::SimplePolygonRef:
    return replace_member_with_props (shape_type::simple_polygon_ref_type::tag (), ref, sh);
  case shape_type::SimplePolygonPtrArray:
    //  HINT: since we are in editing mode, this type should not appear ..
    return replace_member_with_props (shape_type::simple_polygon_ptr_array_type::tag (), ref, sh);
  case shape_type::Edge:
    return replace_member_with_props (shape_type::edge_type::tag (), ref, sh);
  case shape_type::EdgePair:
    return replace_member_with_props (shape_type::edge_pair_type::tag (), ref, sh);
  case shape_type::Point:
    return replace_member_with_props (shape_type::point_type::tag (), ref, sh);
  case shape_type::Path:
    return replace_member_with_props (shape_type::path_type::tag (), ref, sh);
  case shape_type::PathRef:
    return replace_member_with_props (shape_type::path_ref_type::tag (), ref, sh);
  case shape_type::PathPtrArray:
    //  HINT: since we are in editing mode, this type should not appear ..
    return replace_member_with_props (shape_type::path_ptr_array_type::tag (), ref, sh);
  case shape_type::Box:
    return replace_member_with_props (shape_type::box_type::tag (), ref, sh);
  case shape_type::BoxArray:
    //  HINT: since we are in editing mode, this type should not appear ..
    return replace_member_with_props (shape_type::box_array_type::tag (), ref, sh);
  case shape_type::ShortBox:
    return replace_member_with_props (shape_type::short_box_type::tag (), ref, sh);
  case shape_type::ShortBoxArray:
    //  HINT: since we are in editing mode, this type should not appear ..
    return replace_member_with_props (shape_type::short_box_array_type::tag (), ref, sh);
  case shape_type::Text:
    return replace_member_with_props (shape_type::text_type::tag (), ref, sh);
  case shape_type::TextRef:
    return replace_member_with_props (shape_type::text_ref_type::tag (), ref, sh);
  case shape_type::TextPtrArray:
    //  HINT: since we are in editing mode, this type should not appear ..
    return replace_member_with_props (shape_type::text_ptr_array_type::tag (), ref, sh);
  case shape_type::UserObject:
    return replace_member_with_props (shape_type::user_object_type::tag (), ref, sh);
  default:
    return ref;
  };
}

void 
Shapes::clear ()
{
  if (!m_layers.empty ()) {

    invalidate_state ();  //  HINT: must come before the change is done!

    for (tl::vector<LayerBase *>::const_iterator l = m_layers.end (); l != m_layers.begin (); ) {
      //  because the undo stack will do a push, we need to remove layers from the back (this is the last undo
      //  element to be executed)
      --l;
      if (manager () && manager ()->transacting ()) {
        check_is_editable_for_undo_redo ();
        manager ()->queue (this, new FullLayerOp (false, (*l)));
      } else {
        delete *l;
      }
    }

    m_layers.clear ();

  }
}

void
Shapes::clear (unsigned int flags)
{
  if (!m_layers.empty ()) {

    invalidate_state ();  //  HINT: must come before the change is done!

    tl::vector<LayerBase *> new_layers;

    for (tl::vector<LayerBase *>::const_iterator l = m_layers.end (); l != m_layers.begin (); ) {

      //  because the undo stack will do a push, we need to remove layers from the back (this is the last undo
      //  element to be executed)
      --l;

      if (type_mask_applies (*l, flags)) {

        if (manager () && manager ()->transacting ()) {
          check_is_editable_for_undo_redo ();
          manager ()->queue (this, new FullLayerOp (false, (*l)));
        } else {
          delete *l;
        }

      } else {
        new_layers.push_back (*l);
      }

    }

    m_layers.swap (new_layers);

  }
}

void Shapes::reset_bbox_dirty ()
{
  set_dirty (false);
}

void Shapes::update ()
{
  for (tl::vector<LayerBase *>::const_iterator l = m_layers.begin (); l != m_layers.end (); ++l) {
    (*l)->sort ();
    (*l)->update_bbox ();
  }
  set_dirty (false);
}

bool Shapes::is_bbox_dirty () const
{
  if (is_dirty ()) {
    return true;
  }
  for (tl::vector<LayerBase *>::const_iterator l = m_layers.begin (); l != m_layers.end (); ++l) {
    if ((*l)->is_tree_dirty ()) {
      return true;
    }
  }
  return false;
}

Shapes::box_type Shapes::bbox () const
{
  box_type box;
  for (tl::vector<LayerBase *>::const_iterator l = m_layers.begin (); l != m_layers.end (); ++l) {
    if ((*l)->is_bbox_dirty ()) {
      (*l)->update_bbox ();
    }
    box += (*l)->bbox ();
  }
  return box;
}

void Shapes::sort () 
{
  for (tl::vector<LayerBase *>::const_iterator l = m_layers.begin (); l != m_layers.end (); ++l) {
    (*l)->sort ();
  }
}

void 
Shapes::redo (db::Op *op)
{
  db::LayerOpBase *layop = dynamic_cast<db::LayerOpBase *> (op);
  if (layop) {
    layop->redo (this);
  } 
}

void 
Shapes::undo (db::Op *op)
{
  db::LayerOpBase *layop = dynamic_cast<db::LayerOpBase *> (op);
  if (layop) {
    layop->undo (this);
  } 
}

void
Shapes::mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
{
  if (! no_self) {
    stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
  }
  db::mem_stat (stat, purpose, cat, m_layers, true, (void *) this);
  db::mem_stat (stat, purpose, cat, mp_cell, true, (void *) this);
  for (tl::vector<LayerBase *>::const_iterator l = m_layers.begin (); l != m_layers.end (); ++l) {
    (*l)->mem_stat (stat, purpose, cat, false, (void *) this);
  }
}

template <class Tag, class PropIdMap>
Shapes::shape_type 
Shapes::insert_array_by_tag (Tag tag, const shape_type &shape, repository_type &rep, PropIdMap &pm)
{
  if (! shape.has_prop_id ()) {
    typename Tag::object_type n (*shape.basic_ptr (tag));
    n.object ().translate (rep);
    return insert (n);
  } else {
    db::object_with_properties<typename Tag::object_type> n (*shape.basic_ptr (tag), pm (shape.prop_id ()));
    n.object ().translate (rep);
    return insert (n);
  }
}

/** 
 *  @brief (Internal) Insert from a generic pointer
 */
template <class Tag, class PropIdMap>
Shapes::shape_type 
Shapes::insert_by_tag (Tag tag, const shape_type &shape, repository_type &rep, PropIdMap &pm)
{
  if (! shape.has_prop_id ()) {
    return insert (typename Tag::object_type (*shape.basic_ptr (tag), rep));
  } else {
    typedef db::object_with_properties<typename Tag::object_type> swp_type;
    return insert (swp_type (typename Tag::object_type (*shape.basic_ptr (tag), rep), pm (shape.prop_id ())));
  }
}

/** 
 *  @brief (Internal) Insert from a generic pointer
 */
template <class Tag, class PropIdMap>
Shapes::shape_type 
Shapes::insert_by_tag (Tag tag, const shape_type &shape, PropIdMap &pm)
{
  if (! shape.has_prop_id ()) {
    return insert (*shape.basic_ptr (tag));
  } else {
    typedef db::object_with_properties<typename Tag::object_type> swp_type;
    return insert (swp_type (*shape.basic_ptr (tag), pm (shape.prop_id ())));
  }
}

template <class Tag>
Shapes::shape_type  
Shapes::find_shape_by_tag (Tag tag, const shape_type &shape) const
{
  if (! is_editable ()) {
    throw tl::Exception (tl::to_string (tr ("Function 'find' is permitted only in editable mode")));
  }
  if (! shape.has_prop_id ()) {
    typename db::layer<typename Tag::object_type, db::stable_layer_tag>::iterator i = get_layer<typename Tag::object_type, db::stable_layer_tag> ().find (*shape.basic_ptr (tag));
    if (i == get_layer<typename Tag::object_type, db::stable_layer_tag> ().end ()) {
      return shape_type ();
    } else {
      return shape_type (this, i);
    }
  } else {
    typedef db::object_with_properties<typename Tag::object_type> swp_type;
    typename db::layer<swp_type, db::stable_layer_tag>::iterator i = get_layer<swp_type, db::stable_layer_tag> ().find (*shape.basic_ptr (typename swp_type::tag ()));
    if (i == get_layer<swp_type, db::stable_layer_tag> ().end ()) {
      return shape_type ();
    } else {
      return shape_type (this, i);
    }
  }
} 

template <class Sh>
void
Shapes::replace_prop_id (const Sh *pos, db::properties_id_type prop_id)
{
  if (pos->properties_id () != prop_id) {
    if (! is_editable ()) {
      throw tl::Exception (tl::to_string (tr ("Function 'replace' is permitted only in editable mode")));
    }
    if (manager () && manager ()->transacting ()) {
      check_is_editable_for_undo_redo ();
      db::layer_op<Sh, db::stable_layer_tag>::queue_or_append (manager (), this, false /*not insert*/, *pos);
    }
    invalidate_state ();  //  HINT: must come before the change is done!
    ((Sh *) pos)->properties_id (prop_id);
    if (manager () && manager ()->transacting ()) {
      db::layer_op<Sh, db::stable_layer_tag>::queue_or_append (manager (), this, true /*insert*/, *pos);
    }
  }
}

template <class Sh, class Iter>
Shapes::shape_type
Shapes::replace_prop_id_iter (typename db::object_tag<Sh>, const Iter &iter, db::properties_id_type prop_id)
{
  if (! is_editable ()) {
    throw tl::Exception (tl::to_string (tr ("Function 'replace' is permitted only in editable mode")));
  }

  if (manager () && manager ()->transacting ()) {
    check_is_editable_for_undo_redo ();
    db::layer_op<Sh, db::stable_layer_tag>::queue_or_append (manager (), this, false /*not insert*/, *iter);
  }
  db::object_with_properties <Sh> wp (*iter, prop_id);
  invalidate_state ();  //  HINT: must come before the change is done!
  get_layer<Sh, db::stable_layer_tag> ().erase (iter); 
  if (manager () && manager ()->transacting ()) {
    db::layer_op<db::object_with_properties <Sh>, db::stable_layer_tag>::queue_or_append (manager (), this, true /*insert*/, wp);
  }
  return shape_type (this, get_layer <db::object_with_properties <Sh>, db::stable_layer_tag> ().insert (wp)); 
}

template <class Sh1, class Sh2>
Shapes::shape_type 
Shapes::reinsert_member_with_props (typename db::object_tag<Sh1>, const shape_type &ref, const Sh2 &sh)
{
  if (! is_editable ()) {
    throw tl::Exception (tl::to_string (tr ("Function 'replace' is permitted only in editable mode")));
  }

  //  the shape types are not equal - resolve into erase and insert (of new)
  if (! ref.has_prop_id ()) {
    erase_shape (ref);
    return insert (sh);
  } else {
    db::properties_id_type pid = ref.prop_id ();
    erase_shape (ref);
    return insert (db::object_with_properties<Sh2> (sh, pid));
  }
}

template <class Sh1, class Sh2>
Shapes::shape_type 
Shapes::replace_member_with_props (typename db::object_tag<Sh1>, const shape_type &ref, const Sh2 &sh)
{
  if (! is_editable ()) {
    throw tl::Exception (tl::to_string (tr ("Function 'replace' is permitted only in editable mode")));
  }

  //  the shape types are not equal - resolve into erase and insert (of new)
  if (! ref.has_prop_id ()) {
    erase_shape (ref);
    return insert (sh);
  } else {
    db::properties_id_type pid = ref.prop_id ();
    erase_shape (ref);
    return insert (db::object_with_properties<Sh2> (sh, pid));
  }
}

template <class Sh>
Shapes::shape_type 
Shapes::replace_member_with_props (typename db::object_tag<Sh> tag, const shape_type &ref, const Sh &sh)
{
  //  avoid creating a undo entry if the shape is equal to the current one
  if (*ref.basic_ptr (tag) == sh) {
    return ref;
  }
  
  if (! layout ()) {

    if (needs_translate (tag)) {
      return reinsert_member_with_props (tag, ref, sh);
    } else {

      //  simple replace case

      if (manager () && manager ()->transacting ()) {
        check_is_editable_for_undo_redo ();
        db::layer_op<Sh, db::stable_layer_tag>::queue_or_append (manager (), this, false /*not insert*/, *ref.basic_ptr (tag));
      }

      invalidate_state ();  //  HINT: must come before the change is done!

      get_layer<Sh, db::stable_layer_tag> ().replace (ref.basic_iter (tag), sh);

      if (manager () && manager ()->transacting ()) {
        db::layer_op<Sh, db::stable_layer_tag>::queue_or_append (manager (), this, true /*insert*/, sh);
      }

      return ref;

    }

  } else {

    if (! is_editable ()) {
      throw tl::Exception (tl::to_string (tr ("Function 'replace' is permitted only in editable mode")));
    }

    if (! ref.has_prop_id ()) {

      if (manager () && manager ()->transacting ()) {
        check_is_editable_for_undo_redo ();
        db::layer_op<Sh, db::stable_layer_tag>::queue_or_append (manager (), this, false /*not insert*/, *ref.basic_ptr (tag));
      }

      invalidate_state ();  //  HINT: must come before the change is done!

      if (needs_translate (tag)) {

        Sh sh_trans;
        sh_trans.translate (sh, shape_repository (), array_repository ());
        get_layer<Sh, db::stable_layer_tag> ().replace (ref.basic_iter (tag), sh_trans);

        if (manager () && manager ()->transacting ()) {
          db::layer_op<Sh, db::stable_layer_tag>::queue_or_append (manager (), this, true /*insert*/, sh_trans);
        }

      } else {

        get_layer<Sh, db::stable_layer_tag> ().replace (ref.basic_iter (tag), sh);

        if (manager () && manager ()->transacting ()) {
          check_is_editable_for_undo_redo ();
          db::layer_op<Sh, db::stable_layer_tag>::queue_or_append (manager (), this, true /*insert*/, sh);
        }

      }

    } else {

      if (manager () && manager ()->transacting ()) {
        check_is_editable_for_undo_redo ();
        db::layer_op<db::object_with_properties<Sh>, db::stable_layer_tag>::queue_or_append (manager (), this, false /*not insert*/, *ref.basic_ptr (typename db::object_with_properties<Sh>::tag ()));
      }

      invalidate_state ();  //  HINT: must come before the change is done!

      db::object_with_properties<Sh> swp;
      swp.translate (db::object_with_properties<Sh> (sh, ref.prop_id ()), shape_repository (), array_repository ());
      get_layer<db::object_with_properties<Sh>, db::stable_layer_tag> ().replace (ref.basic_iter (typename db::object_with_properties<Sh>::tag ()), swp);

      if (manager () && manager ()->transacting ()) {
        db::layer_op<db::object_with_properties<Sh>, db::stable_layer_tag>::queue_or_append (manager (), this, true /*insert*/, swp);
      }

    }

    return ref;

  }
}

//  explicit instantiations

template DB_PUBLIC Shape Shapes::replace<>(const Shape &, const Box &);
template DB_PUBLIC Shape Shapes::replace<>(const Shape &, const ShortBox &);
template DB_PUBLIC Shape Shapes::replace<>(const Shape &, const Path &);
template DB_PUBLIC Shape Shapes::replace<>(const Shape &, const Polygon &);
template DB_PUBLIC Shape Shapes::replace<>(const Shape &, const SimplePolygon &);
template DB_PUBLIC Shape Shapes::replace<>(const Shape &, const Text &);
template DB_PUBLIC Shape Shapes::replace<>(const Shape &, const Edge &);
template DB_PUBLIC Shape Shapes::replace<>(const Shape &, const EdgePair &);
template DB_PUBLIC Shape Shapes::replace<>(const Shape &, const Point &);

template DB_PUBLIC Shape Shapes::transform<> (const Shape &, const ICplxTrans &);
template DB_PUBLIC Shape Shapes::transform<> (const Shape &, const Trans &);

template DB_PUBLIC Shape Shapes::do_insert<> (const Shape &, const ICplxTrans &, tl::func_delegate_base <db::properties_id_type> &);
template DB_PUBLIC Shape Shapes::do_insert<> (const Shape &, const Trans &, tl::func_delegate_base <db::properties_id_type> &);

template class DB_PUBLIC layer_op<db::Shape::polygon_type, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::polygon_type>, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::simple_polygon_type, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::simple_polygon_type>, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::polygon_ref_type, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::polygon_ref_type>, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::simple_polygon_ref_type, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::simple_polygon_ref_type>, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::polygon_ptr_array_type, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::polygon_ptr_array_type>, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::simple_polygon_ptr_array_type, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::simple_polygon_ptr_array_type>, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::path_type, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::path_type>, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::path_ref_type, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::path_ref_type>, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::path_ptr_array_type, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::path_ptr_array_type>, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::edge_type, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::edge_type>, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::edge_pair_type, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::edge_pair_type>, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::point_type, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::point_type>, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::text_type, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::text_type>, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::text_ref_type, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::text_ref_type>, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::text_ptr_array_type, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::text_ptr_array_type>, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::box_type, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::box_type>, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::box_array_type, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::box_array_type>, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::short_box_type, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::short_box_type>, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::short_box_array_type, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::short_box_array_type>, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::user_object_type, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::user_object_type>, db::stable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::polygon_type, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::polygon_type>, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::simple_polygon_type, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::simple_polygon_type>, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::polygon_ref_type, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::polygon_ref_type>, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::simple_polygon_ref_type, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::simple_polygon_ref_type>, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::polygon_ptr_array_type, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::polygon_ptr_array_type>, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::simple_polygon_ptr_array_type, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::simple_polygon_ptr_array_type>, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::path_type, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::path_type>, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::path_ref_type, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::path_ref_type>, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::path_ptr_array_type, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::path_ptr_array_type>, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::edge_type, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::edge_type>, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::edge_pair_type, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::edge_pair_type>, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::point_type, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::point_type>, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::text_type, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::text_type>, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::text_ref_type, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::text_ref_type>, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::text_ptr_array_type, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::text_ptr_array_type>, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::box_type, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::box_type>, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::box_array_type, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::box_array_type>, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::short_box_type, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::short_box_type>, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::short_box_array_type, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::short_box_array_type>, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::Shape::user_object_type, db::unstable_layer_tag>;
template class DB_PUBLIC layer_op<db::object_with_properties<db::Shape::user_object_type>, db::unstable_layer_tag>;

}

