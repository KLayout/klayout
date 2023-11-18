
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


#ifndef HDR_dbShapeCollectionUtils
#define HDR_dbShapeCollectionUtils

#include "dbCommon.h"

#include "dbShape.h"
#include "dbShapes.h"
#include "dbLayout.h"
#include "dbCellVariants.h"
#include "dbShapeCollection.h"
#include "dbDeepShapeStore.h"
#include "tlObject.h"

#include <list>

namespace db {

/**
 *  @brief A template base class for a shape processors
 *
 *  A shape processor can turn a shape into something else.
 */
template <class Shape, class Result>
class DB_PUBLIC_TEMPLATE shape_collection_processor
  : public tl::Object
{
public:
  /**
   *  @brief Constructor
   */
  shape_collection_processor () { }

  /**
   *  @brief Destructor
   */
  virtual ~shape_collection_processor () { }

  /**
   *  @brief Performs the actual processing
   *  This method will take the input edge from "edge" and puts the results into "res".
   *  "res" can be empty - in this case, the edge will be skipped.
   */
  virtual void process (const Shape &shape, std::vector<Result> &res) const = 0;

  /**
   *  @brief Returns the transformation reducer for building cell variants
   *  This method may return 0. In this case, not cell variants are built.
   */
  virtual const TransformationReducer *vars () const { return 0; }

  /**
   *  @brief Returns true, if the result of this operation can be regarded "merged" always.
   */
  virtual bool result_is_merged () const { return false; }

  /**
   *  @brief Returns true, if the result of this operation must not be merged.
   *  This feature can be used, if the result represents "degenerated" objects such
   *  as point-like edges. These must not be merged. Otherwise they disappear.
   */
  virtual bool result_must_not_be_merged () const { return false; }

  /**
   *  @brief Returns true, if the processor wants raw (not merged) input
   */
  virtual bool requires_raw_input () const { return false; }

  /**
   *  @brief Returns true, if the processor wants to build variants
   *  If not true, the processor accepts shape propagation as variant resolution.
   */
  virtual bool wants_variants () const { return false; }
};

/**
 *  @brief A shape delivery class for the shape collection processor
 */
template <class Result> struct shape_collection_processor_delivery;

/**
 *  @brief A shape delivery implementation for polygons
 */
template <>
struct DB_PUBLIC shape_collection_processor_delivery<db::Polygon>
{
  shape_collection_processor_delivery (db::Layout *layout, db::Shapes *shapes)
    : mp_layout (layout), mp_shapes (shapes)
  { }

  void put (const db::Polygon &result)
  {
    tl::MutexLocker locker (&mp_layout->lock ());
    mp_shapes->insert (db::PolygonRef (result, mp_layout->shape_repository ()));
  }

private:
  db::Layout *mp_layout;
  db::Shapes *mp_shapes;
};

/**
 *  @brief A shape delivery implementation for texts
 */
template <>
struct DB_PUBLIC shape_collection_processor_delivery<db::Text>
{
  shape_collection_processor_delivery (db::Layout *layout, db::Shapes *shapes)
    : mp_layout (layout), mp_shapes (shapes)
  { }

  void put (const db::Text &result)
  {
    tl::MutexLocker locker (&mp_layout->lock ());
    mp_shapes->insert (db::TextRef (result, mp_layout->shape_repository ()));
  }

private:
  db::Layout *mp_layout;
  db::Shapes *mp_shapes;
};

/**
 *  @brief A generic delivery
 */
template <class Result>
struct DB_PUBLIC shape_collection_processor_delivery
{
  shape_collection_processor_delivery (db::Layout *, db::Shapes *shapes)
    : mp_shapes (shapes)
  { }

  void put (const Result &result)
  {
    mp_shapes->insert (result);
  }

private:
  db::Shapes *mp_shapes;
};

/**
 *  @brief Provides a generic implementation of the shape collection processor
 */
template <class Shape, class Result, class OutputContainer>
DB_PUBLIC_TEMPLATE
OutputContainer *
shape_collection_processed_impl (const db::DeepLayer &input, const shape_collection_processor<Shape, Result> &filter)
{
  db::Layout &layout = const_cast<db::Layout &> (input.layout ());

  std::unique_ptr<VariantsCollectorBase> vars;
  if (filter.vars ()) {

    vars.reset (new db::VariantsCollectorBase (filter.vars ()));

    vars->collect (&layout, input.initial_cell ().cell_index ());

    if (filter.wants_variants ()) {
      vars->separate_variants ();
    }

  }

  std::vector<Result> heap;
  std::map<db::cell_index_type, std::map<db::ICplxTrans, db::Shapes> > to_commit;

  std::unique_ptr<OutputContainer> res (new OutputContainer (input.derived ()));
  if (filter.result_must_not_be_merged ()) {
    res->set_merged_semantics (false);
  }

  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

    const db::Shapes &s = c->shapes (input.layer ());

    if (vars.get ()) {

      const std::set<db::ICplxTrans> &vv = vars->variants (c->cell_index ());
      for (auto v = vv.begin (); v != vv.end (); ++v) {

        db::Shapes *st;
        if (vv.size () == 1) {
          st = & c->shapes (res->deep_layer ().layer ());
        } else {
          st = & to_commit [c->cell_index ()] [*v];
        }

        shape_collection_processor_delivery<Result> delivery (&layout, st);
        shape_collection_processor_delivery<db::object_with_properties<Result> > delivery_wp (&layout, st);

        const db::ICplxTrans &tr = *v;
        db::ICplxTrans trinv = tr.inverted ();

        for (db::Shapes::shape_iterator si = s.begin (db::ShapeIterator::All); ! si.at_end (); ++si) {
          Shape s;
          si->instantiate (s);
          s.transform (tr);
          heap.clear ();
          filter.process (s, heap);
          for (typename std::vector<Result>::const_iterator i = heap.begin (); i != heap.end (); ++i) {
            if (si->prop_id ()) {
              delivery_wp.put (db::object_with_properties<Result> (i->transformed (trinv), si->prop_id ()));
            } else {
              delivery.put (i->transformed (trinv));
            }
          }
        }

      }

    } else {

      db::Shapes &st = c->shapes (res->deep_layer ().layer ());
      shape_collection_processor_delivery<Result> delivery (&layout, &st);
      shape_collection_processor_delivery<db::object_with_properties<Result> > delivery_wp (&layout, &st);

      for (db::Shapes::shape_iterator si = s.begin (db::ShapeIterator::All); ! si.at_end (); ++si) {
        Shape s;
        si->instantiate (s);
        heap.clear ();
        filter.process (s, heap);
        for (typename std::vector<Result>::const_iterator i = heap.begin (); i != heap.end (); ++i) {
          if (si->prop_id ()) {
            delivery_wp.put (db::object_with_properties<Result> (*i, si->prop_id ()));
          } else {
            delivery.put (*i);
          }
        }
      }

    }

  }

  if (! to_commit.empty () && vars.get ()) {
    vars->commit_shapes (res->deep_layer ().layer (), to_commit);
  }

  if (filter.result_is_merged ()) {
    res->set_is_merged (true);
  }
  return res.release ();
}

/**
 *  @brief A generic processor to compute the extents of an object
 */
template <class Shape>
class extents_processor
  : public db::shape_collection_processor<Shape, db::Polygon>
{
public:
  extents_processor (db::Coord dx, db::Coord dy)
    : m_dx (dx), m_dy (dy)
  { }

  virtual void process (const Shape &s, std::vector<db::Polygon> &res) const
  {
    db::box_convert<Shape> bc;
    db::Box box = bc (s).enlarged (db::Vector (m_dx, m_dy));
    if (! box.empty ()) {
      res.push_back (db::Polygon (box));
    }
  }

  virtual const db::TransformationReducer *vars () const
  {
    if (m_dx == 0 && m_dy == 0) {
      return 0;
    } else if (m_dx == m_dy) {
      return & m_isotropic_reducer;
    } else {
      return & m_anisotropic_reducer;
    }
  }

  virtual bool result_is_merged () const { return false; }
  virtual bool requires_raw_input () const { return false; }
  virtual bool result_must_not_be_merged () const { return false; }
  virtual bool wants_variants () const { return true; }

private:
  db::XYAnisotropyAndMagnificationReducer m_anisotropic_reducer;
  db::MagnificationReducer m_isotropic_reducer;
  db::Coord m_dx, m_dy;
};

}

#endif

