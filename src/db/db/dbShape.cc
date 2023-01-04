
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
#include "dbBoxConvert.h"
#include "dbPolygonTools.h"
#include "tlCpp.h"

namespace db
{

static NO_RETURN void raise_no_path ()
{
  throw tl::Exception (tl::to_string (tr ("Shape is not a path")));
}

static NO_RETURN void raise_no_polygon ()
{
  throw tl::Exception (tl::to_string (tr ("Shape is not a general or simple polygon")));
}

static NO_RETURN void raise_no_general_polygon ()
{
  throw tl::Exception (tl::to_string (tr ("Shape is not a general polygon")));
}

static NO_RETURN void raise_no_simple_polygon ()
{
  throw tl::Exception (tl::to_string (tr ("Shape is not a simple polygon-type")));
}

static NO_RETURN void raise_no_box ()
{
  throw tl::Exception (tl::to_string (tr ("Shape is not a box")));
}

static NO_RETURN void raise_no_text ()
{
  throw tl::Exception (tl::to_string (tr ("Shape is not a text")));
}

static NO_RETURN void raise_invalid_hole_index_on_polygon ()
{
  throw tl::Exception (tl::to_string (tr ("Invalid hole index")));
}

static NO_RETURN void raise_invalid_hole_index_on_simple_polygon ()
{
  throw tl::Exception (tl::to_string (tr ("A simple polygon doesn't have holes")));
}

// -------------------------------------------------------------------------------
//  Shape implementation

db::properties_id_type Shape::prop_id () const
{
  if (m_with_props) {
    if (m_stable) {
      switch (m_type) {
      case Polygon:
        return (**((ppolygon_iter_type *) m_generic.iter)).properties_id ();
      case PolygonRef:
        return (**((ppolygon_ref_iter_type *) m_generic.iter)).properties_id ();
      case PolygonPtrArray:
      case PolygonPtrArrayMember:
        return (**((ppolygon_ptr_array_iter_type *) m_generic.iter)).properties_id ();
      case SimplePolygon:
        return (**((psimple_polygon_iter_type *) m_generic.iter)).properties_id ();
      case SimplePolygonRef:
        return (**((psimple_polygon_ref_iter_type *) m_generic.iter)).properties_id ();
      case SimplePolygonPtrArray:
      case SimplePolygonPtrArrayMember:
        return (**((psimple_polygon_ptr_array_iter_type *) m_generic.iter)).properties_id ();
      case Edge:
        return (**((pedge_iter_type *) m_generic.iter)).properties_id ();
      case EdgePair:
        return (**((pedge_pair_iter_type *) m_generic.iter)).properties_id ();
      case Point:
        return (**((ppoint_iter_type *) m_generic.iter)).properties_id ();
      case Path:
        return (**((ppath_iter_type *) m_generic.iter)).properties_id ();
      case PathRef:
        return (**((ppath_ref_iter_type *) m_generic.iter)).properties_id ();
      case PathPtrArray:
      case PathPtrArrayMember:
        return (**((ppath_ptr_array_iter_type *) m_generic.iter)).properties_id ();
      case Box:
        return (**((pbox_iter_type *) m_generic.iter)).properties_id ();
      case BoxArray:
      case BoxArrayMember:
        return (**((pbox_array_iter_type *) m_generic.iter)).properties_id ();
      case ShortBox:
        return (**((pshort_box_iter_type *) m_generic.iter)).properties_id ();
      case ShortBoxArray:
      case ShortBoxArrayMember:
        return (**((pshort_box_array_iter_type *) m_generic.iter)).properties_id ();
      case Text:
        return (**((ptext_iter_type *) m_generic.iter)).properties_id ();
      case TextRef:
        return (**((ptext_ref_iter_type *) m_generic.iter)).properties_id ();
      case TextPtrArray:
      case TextPtrArrayMember:
        return (**((ptext_ptr_array_iter_type *) m_generic.iter)).properties_id ();
      case UserObject:
        return (**((puser_object_iter_type *) m_generic.iter)).properties_id ();
      default:
        return 0;
      }
    } else {
      switch (m_type) {
      case Polygon:
        return m_generic.ppolygon->properties_id ();
      case PolygonRef:
        return m_generic.ppolygon_ref->properties_id ();
      case PolygonPtrArray:
      case PolygonPtrArrayMember:
        return m_generic.ppolygon_aref->properties_id ();
      case SimplePolygon:
        return m_generic.psimple_polygon->properties_id ();
      case SimplePolygonRef:
        return m_generic.psimple_polygon_ref->properties_id ();
      case SimplePolygonPtrArray:
      case SimplePolygonPtrArrayMember:
        return m_generic.psimple_polygon_aref->properties_id ();
      case Edge:
        return m_generic.pedge->properties_id ();
      case EdgePair:
        return m_generic.pedge_pair->properties_id ();
      case Point:
        return m_generic.ppoint->properties_id ();
      case Path:
        return m_generic.ppath->properties_id ();
      case PathRef:
        return m_generic.ppath_ref->properties_id ();
      case PathPtrArray:
      case PathPtrArrayMember:
        return m_generic.ppath_aref->properties_id ();
      case Box:
        return m_generic.pbox->properties_id ();
      case BoxArray:
      case BoxArrayMember:
        return m_generic.pbox_array->properties_id ();
      case ShortBox:
        return m_generic.pshort_box->properties_id ();
      case ShortBoxArray:
      case ShortBoxArrayMember:
        return m_generic.pshort_box_array->properties_id ();
      case Text:
        return m_generic.ptext->properties_id ();
      case TextRef:
        return m_generic.ptext_ref->properties_id ();
      case TextPtrArray:
      case TextPtrArrayMember:
        return m_generic.ptext_aref->properties_id ();
      case UserObject:
        return m_generic.puser_object->properties_id ();
      default:
        return 0;
      }
    }
  } else {
    return 0;
  }
}

Shape::point_iterator Shape::begin_point () const
{
  if (m_type == Path) {
    return point_iterator (path ().begin ());
  } else if (m_type == PathRef || m_type == PathPtrArrayMember) {
    return point_iterator (path_ref ().begin ());
  } else {
    raise_no_path ();
  }
}

Shape::point_iterator Shape::end_point () const
{
  if (m_type == Path) {
    return point_iterator (path ().end ());
  } else if (m_type == PathRef || m_type == PathPtrArrayMember) {
    return point_iterator (path_ref ().end ());
  } else {
    raise_no_path ();
  }
}

Shape::point_iterator Shape::begin_hull () const
{
  if (m_type == SimplePolygon) {
    return point_iterator (simple_polygon ().begin_hull ());
  } else if (m_type == SimplePolygonRef || m_type == SimplePolygonPtrArrayMember) {
    return point_iterator (simple_polygon_ref ().begin_hull ());
  } else if (m_type == Polygon) {
    return point_iterator (polygon ().begin_hull ());
  } else if (m_type == PolygonRef || m_type == PolygonPtrArrayMember) {
    return point_iterator (polygon_ref ().begin_hull ());
  } else {
    raise_no_polygon ();
  }
}

Shape::point_iterator Shape::end_hull () const
{
  if (m_type == SimplePolygon) {
    return point_iterator (simple_polygon ().end_hull ());
  } else if (m_type == SimplePolygonRef || m_type == SimplePolygonPtrArrayMember) {
    return point_iterator (simple_polygon_ref ().end_hull ());
  } else if (m_type == Polygon) {
    return point_iterator (polygon ().end_hull ());
  } else if (m_type == PolygonRef || m_type == PolygonPtrArrayMember) {
    return point_iterator (polygon_ref ().end_hull ());
  } else {
    raise_no_polygon ();
  }
}

Shape::point_iterator Shape::begin_hole (unsigned int hole) const
{
  if (m_type == SimplePolygon || m_type == SimplePolygonRef || m_type == SimplePolygonPtrArrayMember) {
    raise_invalid_hole_index_on_simple_polygon ();
  } else if (m_type == Polygon) {
    if (hole >= polygon ().holes ()) {
      raise_invalid_hole_index_on_polygon ();
    }
    return point_iterator (polygon ().begin_hole (hole));
  } else if (m_type == PolygonRef || m_type == PolygonPtrArrayMember) {
    if (hole >= polygon_ref ().obj ().holes ()) {
      raise_invalid_hole_index_on_polygon ();
    }
    return point_iterator (polygon_ref ().begin_hole (hole));
  } else {
    raise_no_polygon ();
  }
}

Shape::point_iterator Shape::end_hole (unsigned int hole) const
{
  if (m_type == SimplePolygon || m_type == SimplePolygonRef || m_type == SimplePolygonPtrArrayMember) {
    raise_invalid_hole_index_on_simple_polygon ();
  } else if (m_type == Polygon) {
    if (hole >= polygon ().holes ()) {
      raise_invalid_hole_index_on_polygon ();
    }
    return point_iterator (polygon ().end_hole (hole));
  } else if (m_type == PolygonRef || m_type == PolygonPtrArrayMember) {
    if (hole >= polygon_ref ().obj ().holes ()) {
      raise_invalid_hole_index_on_polygon ();
    }
    return point_iterator (polygon_ref ().end_hole (hole));
  } else {
    raise_no_polygon ();
  }
}

unsigned int Shape::holes () const
{
  if (m_type == SimplePolygon) {
    return simple_polygon ().holes ();
  } else if (m_type == SimplePolygonRef || m_type == SimplePolygonPtrArrayMember) {
    return simple_polygon_ref ().obj ().holes ();
  } else if (m_type == Polygon) {
    return polygon ().holes ();
  } else if (m_type == PolygonRef || m_type == PolygonPtrArrayMember) {
    return polygon_ref ().obj ().holes ();
  } else {
    raise_no_polygon ();
  }
}

Shape::polygon_edge_iterator Shape::begin_edge () const
{
  if (m_type == SimplePolygon) {
    return polygon_edge_iterator (simple_polygon ().begin_edge ());
  } else if (m_type == SimplePolygonRef || m_type == SimplePolygonPtrArrayMember) {
    return polygon_edge_iterator (simple_polygon_ref ().begin_edge ());
  } else if (m_type == Polygon) {
    return polygon_edge_iterator (polygon ().begin_edge ());
  } else if (m_type == PolygonRef || m_type == PolygonPtrArrayMember) {
    return polygon_edge_iterator (polygon_ref ().begin_edge ());
  } else {
    raise_no_polygon ();
  }
}

Shape::polygon_edge_iterator Shape::begin_edge (unsigned int c) const
{
  if (m_type == SimplePolygon) {
    if (c > 0) {
      return polygon_edge_iterator ();
    } else {
      return polygon_edge_iterator (simple_polygon ().begin_edge ());
    }
  } else if (m_type == SimplePolygonRef || m_type == SimplePolygonPtrArrayMember) {
    if (c > 0) {
      return polygon_edge_iterator ();
    } else {
      return polygon_edge_iterator (simple_polygon_ref ().begin_edge ());
    }
  } else if (m_type == Polygon) {
    return polygon_edge_iterator (polygon ().begin_edge (c));
  } else if (m_type == PolygonRef || m_type == PolygonPtrArrayMember) {
    return polygon_edge_iterator (polygon_ref ().begin_edge (c));
  } else {
    raise_no_polygon ();
  }
}

Shape::polygon_ref_type Shape::polygon_ref () const
{
  if (m_type == PolygonRef) {
    return *basic_ptr (polygon_ref_type::tag ());
  } else if (m_type == PolygonPtrArrayMember) {
    tl_assert (m_trans.rot () == 0);
    return polygon_ref_type (&basic_ptr (polygon_ptr_array_type::tag ())->object ().obj (), m_trans.disp ());
  } else {
    raise_no_general_polygon ();
  }
}

Shape::simple_polygon_ref_type Shape::simple_polygon_ref () const
{
  if (m_type == SimplePolygonRef) {
    return *basic_ptr (simple_polygon_ref_type::tag ());
  } else if (m_type == SimplePolygonPtrArrayMember) {
    tl_assert (m_trans.rot () == 0);
    return simple_polygon_ref_type (&basic_ptr (simple_polygon_ptr_array_type::tag ())->object ().obj (), m_trans.disp ());
  } else {
    raise_no_simple_polygon ();
  }
}

bool Shape::polygon (Shape::polygon_type &p) const
{
  if (m_type == Polygon) {
    p = polygon ();
    return true;
  } else if (m_type == PolygonRef || m_type == PolygonPtrArrayMember) {
    polygon_ref ().instantiate (p);
    return true;
  } else if (m_type == SimplePolygon) {
    p.clear ();
    const simple_polygon_type &sp = simple_polygon ();
    p.assign_hull (sp.hull ());
    return true;
  } else if (m_type == SimplePolygonRef || m_type == SimplePolygonPtrArrayMember) {
    p.clear ();
    const simple_polygon_ref_type &sp = simple_polygon_ref ();
    p.assign_hull (sp.obj ().begin_hull (), sp.obj ().end_hull (), sp.trans (), false /*no additional compression*/);
    return true;
  } else if (m_type == Path) {
    p = path ().polygon ();
    return true;
  } else if (m_type == PathRef || m_type == PathPtrArrayMember) {
    p = path_ref ().obj ().polygon ();
    p.transform (path_ref ().trans (), false /*no additional compression*/);
    return true;
  } else if (is_box ()) {
    p = Shape::polygon_type (box ());
    return true;
  } else {
    return false;
  }
}

bool Shape::simple_polygon (Shape::simple_polygon_type &p) const
{
  if (m_type == Polygon) {
    Shape::polygon_type pp = polygon ();
    p = db::polygon_to_simple_polygon (pp);
    return true;
  } else if (m_type == PolygonRef || m_type == PolygonPtrArrayMember) {
    Shape::polygon_type pp;
    polygon_ref ().instantiate (pp);
    p = db::polygon_to_simple_polygon (pp);
    return true;
  } else if (m_type == SimplePolygon) {
    p = simple_polygon ();
    return true;
  } else if (m_type == SimplePolygonRef || m_type == SimplePolygonPtrArrayMember) {
    p.clear ();
    p.assign_hull (simple_polygon_ref ().obj ().begin_hull (), simple_polygon_ref ().obj ().end_hull (), simple_polygon_ref ().trans (), false /*no additional compression*/);
    return true;
  } else if (m_type == Path) {
    p = path ().simple_polygon ();
    return true;
  } else if (m_type == PathRef || m_type == PathPtrArrayMember) {
    p = path_ref ().obj ().simple_polygon ();
    p.transform (path_ref ().trans (), false /*no additional compression*/);
    return true;
  } else if (is_box ()) {
    p = Shape::simple_polygon_type (box ());
    return true;
  } else {
    return false;
  }
}

Shape::path_ref_type Shape::path_ref () const
{
  if (m_type == PathRef) {
    return *basic_ptr (path_ref_type::tag ());
  } else if (m_type == PathPtrArrayMember) {
    tl_assert (m_trans.rot () == 0);
    return path_ref_type (&basic_ptr (path_ptr_array_type::tag ())->object ().obj (), m_trans.disp ());
  } else {
    raise_no_path ();
  }
}

Shape::coord_type Shape::path_width () const
{
  if (m_type == Path) {
    return path ().width ();
  } else {
    return path_ref ().obj ().width ();
  }
}

Shape::distance_type Shape::path_length () const
{
  if (m_type == Path) {
    return path ().length ();
  } else {
    return path_ref ().obj ().length ();
  }
}

std::pair<Shape::coord_type, Shape::coord_type> Shape::path_extensions () const
{
  if (m_type == Path) {
    return path ().extensions ();
  } else {
    return path_ref ().obj ().extensions ();
  }
}

bool Shape::round_path () const
{
  if (m_type == Path) {
    return path ().round ();
  } else {
    return path_ref ().obj ().round ();
  }
}

bool Shape::path (Shape::path_type &p) const
{
  if (m_type == Path) {
    p = path ();
    return true;
  } else if (m_type == PathRef || m_type == PathPtrArrayMember) {
    path_ref ().instantiate (p);
    return true;
  } else {
    return false;
  }
}

Shape::text_ref_type Shape::text_ref () const
{
  if (m_type == TextRef) {
    return *basic_ptr (text_ref_type::tag ());
  } else if (m_type == TextPtrArrayMember) {
    tl_assert (m_trans.rot () == 0);
    return text_ref_type (&basic_ptr (text_ptr_array_type::tag ())->object ().obj (), disp_type (m_trans.disp ()));
  } else {
    raise_no_text ();
  }
}

bool Shape::text (Shape::text_type &t) const
{
  if (m_type == Text) {
    t = text ();
    t.resolve_ref ();
    return true;
  } else if (m_type == TextRef || m_type == TextPtrArrayMember) {
    text_ref ().instantiate (t);
    t.resolve_ref ();
    return true;
  } else {
    return false;
  }
}

const char *Shape::text_string () const
{
  if (m_type == Text) {
    return text ().string ();
  } else {
    return text_ref ().obj ().string ();
  }
}

Shape::text_type::trans_type Shape::text_trans () const
{
  if (m_type == Text) {
    return text ().trans ();
  } else {
    return trans_type (text_ref ().trans ()) * text_ref ().obj ().trans ();
  }
}

Shape::coord_type Shape::text_size () const
{
  if (m_type == Text) {
    return text ().size ();
  } else {
    return text_ref ().obj ().size ();
  }
}

db::Font Shape::text_font () const
{
  if (m_type == Text) {
    return text ().font ();
  } else {
    return text_ref ().obj ().font ();
  }
}

db::HAlign Shape::text_halign () const
{
  if (m_type == Text) {
    return text ().halign ();
  } else {
    return text_ref ().obj ().halign ();
  }
}

db::VAlign Shape::text_valign () const
{
  if (m_type == Text) {
    return text ().valign ();
  } else {
    return text_ref ().obj ().valign ();
  }
}

Shape::box_type Shape::box () const
{
  if (m_type == Box) {
    return *basic_ptr (box_type::tag ());
  } else if (m_type == ShortBox) {
    return Shape::box_type (*basic_ptr (short_box_type::tag ()));
  } else if (m_type == BoxArrayMember) {
    return m_trans * basic_ptr (box_array_type::tag ())->object ();
  } else if (m_type == ShortBoxArrayMember) {
    return m_trans * basic_ptr (short_box_array_type::tag ())->object ();
  } else if (m_type == Point) {
    point_type pt = point ();
    return m_trans * box_type (pt, pt);
  } else {
    raise_no_box ();
  }
}

Shape::perimeter_type Shape::perimeter () const
{
  switch (m_type) {
  case Null:
    return distance_type ();
  case Polygon:
    return polygon ().perimeter ();
  case PolygonRef:
  case PolygonPtrArrayMember:
    return polygon_ref ().perimeter ();
  case PolygonPtrArray:
    {
      const polygon_ptr_array_type *arr = basic_ptr (polygon_ptr_array_type::tag ());
      return arr->size () * arr->object ().perimeter ();
    }
  case SimplePolygon:
    return simple_polygon ().perimeter ();
  case SimplePolygonRef:
  case SimplePolygonPtrArrayMember:
    return simple_polygon_ref ().perimeter ();
  case SimplePolygonPtrArray:
    {
      const simple_polygon_ptr_array_type *arr = basic_ptr (simple_polygon_ptr_array_type::tag ());
      return arr->size () * arr->object ().perimeter ();
    }
  case Path:
    return path ().perimeter ();
  case PathRef:
  case PathPtrArrayMember:
    return path_ref ().obj ().perimeter ();
  case PathPtrArray:
    {
      const path_ptr_array_type *arr = basic_ptr (path_ptr_array_type::tag ());
      return arr->size () * arr->object ().obj ().perimeter ();
    }
  case BoxArray:
    {
      const box_array_type *arr = basic_ptr (box_array_type::tag ());
      return arr->size () * arr->object ().perimeter ();
    }
  case ShortBoxArray:
    {
      const short_box_array_type *arr = basic_ptr (short_box_array_type::tag ());
      return arr->size () * arr->object ().perimeter ();
    }
  case Point:
    return 0;
  case Edge:
    return edge ().length ();
  case EdgePair:
    return edge_pair ().perimeter ();
  case Box:
  case ShortBox:
  case BoxArrayMember:
  case ShortBoxArrayMember:
    return box ().perimeter ();
  default:
    return 0;
  }
}

size_t Shape::array_size () const
{
  switch (m_type) {
  case Null:
    return 0;
  case Point:
  case Edge:
  case EdgePair:
  case Polygon:
  case PolygonRef:
  case PolygonPtrArrayMember:
    return 1;
  case PolygonPtrArray:
    {
      const polygon_ptr_array_type *arr = basic_ptr (polygon_ptr_array_type::tag ());
      return arr->size ();
    }
  case SimplePolygon:
  case SimplePolygonRef:
  case SimplePolygonPtrArrayMember:
    return 1;
  case SimplePolygonPtrArray:
    {
      const simple_polygon_ptr_array_type *arr = basic_ptr (simple_polygon_ptr_array_type::tag ());
      return arr->size ();
    }
  case Path:
  case PathRef:
  case PathPtrArrayMember:
    return 1;
  case PathPtrArray:
    {
      const path_ptr_array_type *arr = basic_ptr (path_ptr_array_type::tag ());
      return arr->size ();
    }
  case BoxArray:
    {
      const box_array_type *arr = basic_ptr (box_array_type::tag ());
      return arr->size ();
    }
  case ShortBoxArray:
    {
      const short_box_array_type *arr = basic_ptr (short_box_array_type::tag ());
      return arr->size ();
    }
  case Box:
  case ShortBox:
  case BoxArrayMember:
  case ShortBoxArrayMember:
    return 1;
  default:
    return 1;
  }
}

Shape::area_type Shape::area () const
{
  switch (m_type) {
  case Null:
    return area_type ();
  case Point:
  case Edge:
    return 0;
  case EdgePair:
    return edge_pair ().area ();
  case Polygon:
    return polygon ().area ();
  case PolygonRef:
  case PolygonPtrArrayMember:
    return polygon_ref ().area ();
  case PolygonPtrArray:
    {
      const polygon_ptr_array_type *arr = basic_ptr (polygon_ptr_array_type::tag ());
      return arr->size () * arr->object ().area ();
    }
  case SimplePolygon:
    return simple_polygon ().area ();
  case SimplePolygonRef:
  case SimplePolygonPtrArrayMember:
    return simple_polygon_ref ().area ();
  case SimplePolygonPtrArray:
    {
      const simple_polygon_ptr_array_type *arr = basic_ptr (simple_polygon_ptr_array_type::tag ());
      return arr->size () * arr->object ().area ();
    }
  case Path:
    return path ().area ();
  case PathRef:
  case PathPtrArrayMember:
    return path_ref ().obj ().area ();
  case PathPtrArray:
    {
      const path_ptr_array_type *arr = basic_ptr (path_ptr_array_type::tag ());
      return arr->size () * arr->object ().obj ().area ();
    }
  case BoxArray:
    {
      const box_array_type *arr = basic_ptr (box_array_type::tag ());
      return arr->size () * arr->object ().area ();
    }
  case ShortBoxArray:
    {
      const short_box_array_type *arr = basic_ptr (short_box_array_type::tag ());
      return arr->size () * arr->object ().area ();
    }
  case Box:
  case ShortBox:
  case BoxArrayMember:
  case ShortBoxArrayMember:
    return box ().area ();
  default:
    return 0;
  }
}

Shape::box_type Shape::bbox () const
{
  switch (m_type) {
  case Null:
    return box_type ();
  case Polygon:
    return polygon ().box ();
  case PolygonRef:
  case PolygonPtrArrayMember:
    return polygon_ref ().box ();
  case PolygonPtrArray:
    return basic_ptr (polygon_ptr_array_type::tag ())->bbox (db::box_convert<polygon_ptr_type> ());
  case SimplePolygon:
    return simple_polygon ().box ();
  case SimplePolygonRef:
  case SimplePolygonPtrArrayMember:
    return simple_polygon_ref ().box ();
  case SimplePolygonPtrArray:
    return basic_ptr (simple_polygon_ptr_array_type::tag ())->bbox (db::box_convert<simple_polygon_ptr_type> ());
  case Text:
    return text ().box ();
  case TextRef:
  case TextPtrArrayMember:
    return text_ref ().box ();
  case TextPtrArray:
    return basic_ptr (text_ptr_array_type::tag ())->bbox (db::box_convert<text_ptr_type> ());
  case Edge:
    return box_type (edge ().p1 (), edge ().p2 ());
  case EdgePair:
    return edge_pair ().bbox ();
  case Point:
    return box_type (point (), point ());
  case Path:
    return path ().box ();
  case PathRef:
  case PathPtrArrayMember:
    return path_ref ().box ();
  case PathPtrArray:
    return basic_ptr (path_ptr_array_type::tag ())->bbox (db::box_convert<path_ptr_type> ());
  case BoxArray:
    return basic_ptr (box_array_type::tag ())->bbox (db::box_convert<box_type> ());
  case ShortBoxArray:
    return basic_ptr (short_box_array_type::tag ())->bbox (db::box_convert<short_box_type> ());
  case Box:
  case ShortBox:
  case BoxArrayMember:
  case ShortBoxArrayMember:
    return box ();
  case UserObject:
    return user_object ().box ();
  default:
    return box_type ();
  }
}

std::string
Shape::to_string () const
{
  std::string r; 

  switch (m_type) {
  case Null:
    r = "null";
    break;
  case Polygon:
  case PolygonRef:
  case PolygonPtrArrayMember:
    {
      polygon_type p;
      polygon (p);
      r = "polygon " + p.to_string ();
    }
    break;
  case PolygonPtrArray:
    r = "polygon_array";
    break;
  case SimplePolygon:
  case SimplePolygonRef:
  case SimplePolygonPtrArrayMember:
    {
      simple_polygon_type p;
      simple_polygon (p);
      r = "simple_polygon " + p.to_string ();
    }
    break;
  case SimplePolygonPtrArray:
    r = "simple_polygon_array";
    break;
  case Text:
  case TextRef:
  case TextPtrArrayMember:
    {
      text_type p;
      text (p);
      r = "text " + p.to_string ();
    }
    break;
  case TextPtrArray:
    r = "text_array";
    break;
  case Edge:
    r = "edge " + edge ().to_string ();
    break;
  case EdgePair:
    r = "edge_pair " + edge_pair ().to_string ();
    break;
  case Point:
    r = "point " + point ().to_string ();
    break;
  case Path:
  case PathRef:
  case PathPtrArrayMember:
    {
      path_type p;
      path (p);
      r = "path " + p.to_string ();
    }
    break;
  case PathPtrArray:
    r = "path_array";
    break;
  case BoxArray:
    r = "box_array";
    break;
  case ShortBoxArray:
    r = "short_box_array";
    break;
  case Box:
  case ShortBox:
  case BoxArrayMember:
  case ShortBoxArrayMember:
    r = "box " + box ().to_string ();
    break;
  case UserObject:
    r = "user_object";
    break;
  default:
    r = "invalid";
  }
  
  if (has_prop_id ()) {
    r += " prop_id=" + tl::to_string (prop_id ());
  }

  return r;
}

}
