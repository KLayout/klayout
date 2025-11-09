
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

#include "lstrCompressed.h"

#include "dbPluginCommon.h"
#include "dbShapes.h"
#include "dbHash.h"

namespace lstr
{

Compressed::Compressed () 
  : m_next_id (0) 
{ 
  //  .. nothing yet ..
}

uint64_t 
Compressed::make_rep_id (const RegularArray &array, const std::vector<db::Vector> &irregular)
{
  if (! array.is_null ()) {

    auto ia = m_array_to_rep_id.find (array);
    if (ia != m_array_to_rep_id.end ()) {
      return ia->second;
    } else {
      ++m_next_id;
      m_array_to_rep_id.insert (std::make_pair (array, m_next_id));
      return m_next_id;
    }

  } else if (! irregular.empty ()) {

    auto ii = m_irregular_to_rep_id.find (irregular);
    if (ii != m_irregular_to_rep_id.end ()) {
      return ii->second;
    } else {
      ++m_next_id;
      m_irregular_to_rep_id.insert (std::make_pair (irregular, m_next_id));
      return m_next_id;
    }

  } else {

    return 0;

  }
}

template <class Obj>
void 
Compressed::write_shape (const db::Shape &shape, const db::Vector &disp, RegularArray &regular, std::vector<db::Vector> &irregular_array)
{
  Obj sh;
  shape.instantiate (sh);
  if (! object_is_empty (sh)) {
    sh.move (disp);
    if (shape.prop_id () != 0) {
      write (db::object_with_properties<Obj> (sh, shape.prop_id ()), regular, irregular_array);
    } else {
      write (sh, regular, irregular_array);
    }
  }
}

void 
Compressed::compress_shapes (const db::Shapes &shapes, unsigned int level, bool recompress)
{
  RegularArray no_array;
  std::vector<db::Vector> no_irregular_array;

  Compressor<db::Path> path_compressor (level);
  Compressor<db::SimplePolygon> simple_polygon_compressor (level);
  Compressor<db::Polygon> polygon_compressor (level);
  Compressor<db::Edge> edge_compressor (level);
  Compressor<db::EdgePair> edge_pair_compressor (level);
  Compressor<db::Box> box_compressor (level);
  Compressor<db::Text> text_compressor (level);
  Compressor<db::Point> point_compressor (level);

  Compressor<db::PathWithProperties> path_with_properties_compressor (level);
  Compressor<db::SimplePolygonWithProperties> simple_polygon_with_properties_compressor (level);
  Compressor<db::PolygonWithProperties> polygon_with_properties_compressor (level);
  Compressor<db::EdgeWithProperties> edge_with_properties_compressor (level);
  Compressor<db::EdgePairWithProperties> edge_pair_with_properties_compressor (level);
  Compressor<db::BoxWithProperties> box_with_properties_compressor (level);
  Compressor<db::TextWithProperties> text_with_properties_compressor (level);
  Compressor<db::PointWithProperties> point_with_properties_compressor (level);

  for (db::ShapeIterator shape = shapes.begin (db::ShapeIterator::All); ! shape.at_end (); ) {

    if (level <= 0 || (! recompress && shape.in_array ())) {

      RegularArray array;
      std::vector<db::Vector> irregular_array;

      bool transfer_array = (shape.in_array () && level > 0);
      db::Vector disp;
      if (transfer_array) {
        disp = create_repetition (shape.array (), array, irregular_array);
      }

      if (shape->is_simple_polygon ()) {
        write_shape<db::SimplePolygon> (*shape, disp, array, irregular_array);
      } else if (shape->is_polygon ()) {
        write_shape<db::Polygon> (*shape, disp, array, irregular_array);
      } else if (shape->is_path ()) {
        write_shape<db::Path> (*shape, disp, array, irregular_array);
      } else if (shape->is_text ()) {
        write_shape<db::Text> (*shape, disp, array, irregular_array);
      } else if (shape->is_edge ()) {
        write_shape<db::Edge> (*shape, disp, array, irregular_array);
      } else if (shape->is_edge_pair ()) {
        write_shape<db::EdgePair> (*shape, disp, array, irregular_array);
      } else if (shape->is_box ()) {
        write_shape<db::Box> (*shape, disp, array, irregular_array);
      } else if (shape->is_point ()) {
        write_shape<db::Point> (*shape, disp, array, irregular_array);
      } else if (shape->is_user_object ()) {
        // ignore
      } else {
        tl_assert (false); // unknown shape type
      }

      if (transfer_array) {
        shape.finish_array ();
      }

    } else {

      switch (shape->type ()) {
      case db::Shape::Polygon:

        if (shape->has_prop_id ()) {
          auto polygon = *shape->basic_ptr (db::PolygonWithProperties::tag ());
          polygon_with_properties_compressor.add (polygon);
        } else {
          auto polygon = *shape->basic_ptr (db::Polygon::tag ());
          polygon_compressor.add (polygon);
        }

        break;

      case db::Shape::PolygonRef:

        if (shape->has_prop_id ()) {
          auto polygon_ref = *shape->basic_ptr (db::object_with_properties<db::PolygonRef>::tag ());
          db::PolygonWithProperties polygon (polygon_ref.obj (), polygon_ref.properties_id ());
          polygon_with_properties_compressor.add (polygon, polygon_ref.trans ().disp ());
        } else {
          auto polygon_ref = *shape->basic_ptr (db::PolygonRef::tag ());
          polygon_compressor.add (polygon_ref.obj (), polygon_ref.trans ().disp ());
        }

        break;

      case db::Shape::PolygonPtrArrayMember:

        if (shape->has_prop_id ()) {
          auto polygon_ref = *shape->basic_ptr (db::object_with_properties<db::Shape::polygon_ptr_array_type>::tag ());
          db::PolygonWithProperties polygon (polygon_ref.object ().obj (), polygon_ref.properties_id ());
          polygon_with_properties_compressor.add (polygon, shape->array_trans ().disp ());
        } else {
          auto polygon_ref = *shape->basic_ptr (db::Shape::polygon_ptr_array_type::tag ());
          polygon_compressor.add (polygon_ref.object ().obj (), shape->array_trans ().disp ());
        }

        break;

      case db::Shape::SimplePolygon:

        if (shape->has_prop_id ()) {
          auto simple_polygon = *shape->basic_ptr (db::SimplePolygonWithProperties::tag ());
          simple_polygon_with_properties_compressor.add (simple_polygon);
        } else {
          auto simple_polygon = *shape->basic_ptr (db::SimplePolygon::tag ());
          simple_polygon_compressor.add (simple_polygon);
        }

        break;

      case db::Shape::SimplePolygonRef:

        if (shape->has_prop_id ()) {
          auto polygon_ref = *shape->basic_ptr (db::object_with_properties<db::SimplePolygonRef>::tag ());
          db::SimplePolygonWithProperties polygon (polygon_ref.obj (), polygon_ref.properties_id ());
          simple_polygon_with_properties_compressor.add (polygon, polygon_ref.trans ().disp ());
        } else {
          auto polygon_ref = *shape->basic_ptr (db::SimplePolygonRef::tag ());
          simple_polygon_compressor.add (polygon_ref.obj (), polygon_ref.trans ().disp ());
        }

        break;

      case db::Shape::SimplePolygonPtrArrayMember:
           
        if (shape->has_prop_id ()) {
          auto simple_polygon_ref = *shape->basic_ptr (db::object_with_properties<db::Shape::simple_polygon_ptr_array_type>::tag ());
          db::SimplePolygonWithProperties simple_polygon (simple_polygon_ref.object ().obj (), simple_polygon_ref.properties_id ());
          simple_polygon_with_properties_compressor.add (simple_polygon, shape->array_trans ().disp ());
        } else {
          auto simple_polygon_ref = *shape->basic_ptr (db::Shape::simple_polygon_ptr_array_type::tag ());
          simple_polygon_compressor.add (simple_polygon_ref.object ().obj (), shape->array_trans ().disp ());
        }

        break;

      case db::Shape::Edge:

        if (shape->has_prop_id ()) {
          auto edge = *shape->basic_ptr (db::EdgeWithProperties::tag ());
          edge_with_properties_compressor.add (edge);
        } else {
          auto edge = *shape->basic_ptr (db::Edge::tag ());
          edge_compressor.add (edge);
        }

        break;

      case db::Shape::EdgePair:

        if (shape->has_prop_id ()) {
          auto edge_pair = *shape->basic_ptr (db::EdgePairWithProperties::tag ());
          edge_pair_with_properties_compressor.add (edge_pair);
        } else {
          auto edge_pair = *shape->basic_ptr (db::EdgePair::tag ());
          edge_pair_compressor.add (edge_pair);
        }

        break;

      case db::Shape::Path:

        if (shape->has_prop_id ()) {
          auto path = *shape->basic_ptr (db::PathWithProperties::tag ());
          path_with_properties_compressor.add (path);
        } else {
          auto path = *shape->basic_ptr (db::Path::tag ());
          path_compressor.add (path);
        }

        break;

      case db::Shape::PathRef:

        if (shape->has_prop_id ()) {
          auto path_ref = *shape->basic_ptr (db::object_with_properties<db::PathRef>::tag ());
          db::PathWithProperties path (path_ref.obj (), path_ref.properties_id ());
          path_with_properties_compressor.add (path, path_ref.trans ().disp ());
        } else {
          const db::PathRef &path_ref = *shape->basic_ptr (db::PathRef::tag ());
          path_compressor.add (path_ref.obj (), path_ref.trans ().disp ());
        }

        break;

      case db::Shape::PathPtrArrayMember:

        if (shape->has_prop_id ()) {
          auto path_ref = *shape->basic_ptr (db::object_with_properties<db::Shape::path_ptr_array_type>::tag ());
          db::PathWithProperties path (path_ref.object ().obj (), path_ref.properties_id ());
          path_with_properties_compressor.add (path, shape->array_trans ().disp ());
        } else {
          const db::Shape::path_ptr_array_type &path_ref = *shape->basic_ptr (db::Shape::path_ptr_array_type::tag ());
          path_compressor.add (path_ref.object ().obj (), shape->array_trans ().disp ());
        }

        break;

      case db::Shape::Box:

        if (shape->has_prop_id ()) {
          auto box = *shape->basic_ptr (db::BoxWithProperties::tag ());
          box_with_properties_compressor.add (box);
        } else {
          auto box = *shape->basic_ptr (db::Box::tag ());
          box_compressor.add (box);
        }

        break;

      case db::Shape::Point:

        if (shape->has_prop_id ()) {
          auto point = *shape->basic_ptr (db::PointWithProperties::tag ());
          point_with_properties_compressor.add (point);
        } else {
          auto point = *shape->basic_ptr (db::Point::tag ());
          point_compressor.add (point);
        }

        break;

      case db::Shape::BoxArray:
      case db::Shape::BoxArrayMember:
      case db::Shape::ShortBox:
      case db::Shape::ShortBoxArrayMember:

        if (shape->has_prop_id ()) {
          db::BoxWithProperties box;
          shape->instantiate (box);
          box.properties_id (shape->prop_id ());
          box_with_properties_compressor.add (box);
        } else {
          db::Box box;
          shape->instantiate (box);
          box_compressor.add (box);
        }

        break;

      case db::Shape::Text:

        if (shape->has_prop_id ()) {
          auto text = *shape->basic_ptr (db::TextWithProperties::tag ());
          text_with_properties_compressor.add (text);
        } else {
          auto text = *shape->basic_ptr (db::Text::tag ());
          text_compressor.add (text);
        }

        break;

      case db::Shape::TextRef:

        if (shape->has_prop_id ()) {
          auto text_ref = *shape->basic_ptr (db::object_with_properties<db::TextRef>::tag ());
          db::TextWithProperties text (text_ref.obj (), text_ref.properties_id ());
          text_with_properties_compressor.add (text, text_ref.trans ().disp ());
        } else {
          auto text_ref = *shape->basic_ptr (db::TextRef::tag ());
          text_compressor.add (text_ref.obj (), text_ref.trans ().disp ());
        }

        break;

      case db::Shape::TextPtrArrayMember:

        if (shape->has_prop_id ()) {
          auto text_ref = *shape->basic_ptr (db::object_with_properties<db::Shape::text_ptr_array_type>::tag ());
          db::TextWithProperties text (text_ref.object ().obj (), text_ref.properties_id ());
          text_with_properties_compressor.add (text, shape->array_trans ().disp ());
        } else {
          auto text_ref = *shape->basic_ptr (db::Shape::text_ptr_array_type::tag ());
          text_compressor.add (text_ref.object ().obj (), shape->array_trans ().disp ());
        }

        break;

      case db::Shape::UserObject:
        //  ignore.
        break;

      default:
        tl_assert (false);
      }

      ++shape;

    }

  }

  path_compressor.flush (this);
  simple_polygon_compressor.flush (this);
  polygon_compressor.flush (this);
  edge_compressor.flush (this);
  edge_pair_compressor.flush (this);
  box_compressor.flush (this);
  point_compressor.flush (this);
  text_compressor.flush (this);

  path_with_properties_compressor.flush (this);
  simple_polygon_with_properties_compressor.flush (this);
  polygon_with_properties_compressor.flush (this);
  edge_with_properties_compressor.flush (this);
  edge_pair_with_properties_compressor.flush (this);
  box_with_properties_compressor.flush (this);
  point_with_properties_compressor.flush (this);
  text_with_properties_compressor.flush (this);
}

template <class Array>
static db::Vector
create_repetition_from_array (const Array *array, RegularArray &regular, std::vector<db::Vector> &irregular_array)
{
  db::Vector a, b;
  unsigned long na = 0, nb = 0;

  if (array->is_iterated_array (&irregular_array)) {

    //  Take out the first displacement and move that to the shape and sort the displacements.
    //  This way, sequences get normalized and there is a better chance of getting identical 
    //  repetition vectors.
    tl_assert (! irregular_array.empty ());
    db::Vector po = irregular_array.front ();
    std::vector<db::Vector>::iterator pw = irregular_array.begin();
    for (std::vector<db::Vector>::iterator p = pw + 1; p != irregular_array.end (); ++p) {
      *pw++ = *p - po;
    }
    irregular_array.erase (pw, irregular_array.end ());
    std::sort (irregular_array.begin (), irregular_array.end (), vector_cmp_x ());

    return po;

  } else if (array->is_regular_array (a, b, na, nb)) {

    regular = RegularArray (a, b, size_t (na), size_t (nb));
    return db::Vector ();

  } else {
    tl_assert (false);
  }
}

db::Vector
Compressed::create_repetition (const db::Shape &array_shape, RegularArray &regular, std::vector<db::Vector> &irregular_array)
{
  switch (array_shape.type ()) {
  case db::Shape::PolygonPtrArray:
    return create_repetition_from_array (array_shape.basic_ptr (db::Shape::polygon_ptr_array_type::tag ()), regular, irregular_array);
  case db::Shape::SimplePolygonPtrArray:
    return create_repetition_from_array (array_shape.basic_ptr (db::Shape::simple_polygon_ptr_array_type::tag ()), regular, irregular_array);
  case db::Shape::PathPtrArray:
    return create_repetition_from_array (array_shape.basic_ptr (db::Shape::path_ptr_array_type::tag ()), regular, irregular_array);
  case db::Shape::BoxArray:
    return create_repetition_from_array (array_shape.basic_ptr (db::Shape::box_array_type::tag ()), regular, irregular_array);
  case db::Shape::ShortBoxArray:
    return create_repetition_from_array (array_shape.basic_ptr (db::Shape::short_box_array_type::tag ()), regular, irregular_array);
  case db::Shape::TextPtrArray:
    return create_repetition_from_array (array_shape.basic_ptr (db::Shape::text_ptr_array_type::tag ()), regular, irregular_array);
  default: 
    tl_assert (false);
    break;
  }
}

void 
Compressed::compress_instances (const db::Cell::const_iterator &begin_instances, const std::set<db::cell_index_type> &cells_to_write, unsigned int level)
{
  //  use compression 0 for the instances - this preserves the arrays and does not create new ones, the
  //  remaining ones are compressed into irregular arrays
  Compressor<db::CellInstArray> inst_compressor (0);
  Compressor<db::CellInstArrayWithProperties> inst_with_properties_compressor (0);

  //  Collect all instances 
  for (auto inst_iterator = begin_instances; ! inst_iterator.at_end (); ++inst_iterator) {

    if (cells_to_write.find (inst_iterator->cell_index ()) != cells_to_write.end ()) {

      db::properties_id_type prop_id = inst_iterator->prop_id ();
      db::CellInstArray inst_array = inst_iterator->cell_inst ();

      if (level == 0 || inst_array.size () > 1) {

        //  Recode the instance array into a regular array or irregular array spec (the latter hardly used) and 
        //  a single instance.
        RegularArray array;
        std::vector<db::Vector> irregular_array;

        db::Vector disp;
        bool transfer_array = (inst_array.size () > 1 && level > 0);
        if (transfer_array) {
          disp = create_repetition_from_array (&inst_array, array, irregular_array);
        }

        db::CellInstArray single_inst;
        if (! inst_array.is_complex ()) {
          single_inst = db::CellInstArray (inst_array.object (), db::Trans (-disp) * inst_array.front ());
        } else {
          single_inst = db::CellInstArray (inst_array.object (), inst_array.complex_trans (db::Trans (-disp) * inst_array.front ()));
        }

        //  no compression -> just keep as is
        if (prop_id != 0) {
          write (db::CellInstArrayWithProperties (single_inst, prop_id), array, irregular_array);
        } else {
          write (single_inst, array, irregular_array);
        }

      } else {

        //  We have a single instance: reduce by displacement and compress into enumerated (irregular) arrays.
        //  As we configured the compressor with level 0, no regular array formation will happen here. This is
        //  good for instances as array instances are special.
        db::Vector disp = inst_array.front ().disp ();

        db::CellInstArray single_inst;
        if (! inst_array.is_complex ()) {
          single_inst = db::CellInstArray (inst_array.object (), db::Trans (-disp) * inst_array.front ());
        } else {
          single_inst = db::CellInstArray (inst_array.object (), inst_array.complex_trans (db::Trans (-disp) * inst_array.front ()));
        }

        if (prop_id != 0) {
          inst_with_properties_compressor.add (db::CellInstArrayWithProperties (single_inst, prop_id), disp);
        } else {
          inst_compressor.add (single_inst, disp);
        }

      }

    }

  }

  inst_compressor.flush (this);
  inst_with_properties_compressor.flush (this);
}

}
