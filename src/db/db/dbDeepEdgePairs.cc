
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


#include "dbDeepEdgePairs.h"
#include "dbCellGraphUtils.h"
#include "dbDeepEdges.h"
#include "dbDeepRegion.h"
#include "dbCellMapping.h"
#include "dbLayoutUtils.h"
#include "dbEdgePairsLocalOperations.h"
#include "dbHierProcessor.h"
#include "dbRegion.h"

#include <sstream>

namespace db
{

/**
 *  @brief An iterator delegate for the deep region
 *  TODO: this is kind of redundant with OriginalLayerIterator ..
 */
class DB_PUBLIC DeepEdgePairsIterator
  : public EdgePairsIteratorDelegate
{
public:
  typedef db::EdgePair value_type;

  DeepEdgePairsIterator (const db::RecursiveShapeIterator &iter)
    : m_iter (iter), m_prop_id (0)
  {
    set ();
  }

  virtual ~DeepEdgePairsIterator () { }

  virtual bool at_end () const
  {
    return m_iter.at_end ();
  }

  virtual void increment ()
  {
    ++m_iter;
    set ();
  }

  virtual bool is_addressable() const
  {
    return false;
  }

  virtual const value_type *get () const
  {
    return &m_edge_pair;
  }

  virtual db::properties_id_type prop_id () const
  {
    return m_prop_id;
  }

  virtual bool equals (const generic_shape_iterator_delegate_base<value_type> *other) const
  {
    const DeepEdgePairsIterator *o = dynamic_cast<const DeepEdgePairsIterator *> (other);
    return o && o->m_iter == m_iter;
  }

  virtual EdgePairsIteratorDelegate *clone () const
  {
    return new DeepEdgePairsIterator (*this);
  }

  virtual void do_reset (const db::Box &region, bool overlapping)
  {
    m_iter.set_region (region);
    m_iter.set_overlapping (overlapping);
    set ();
  }

  virtual db::Box bbox () const
  {
    return m_iter.bbox ();
  }

private:
  friend class Texts;

  db::RecursiveShapeIterator m_iter;
  mutable value_type m_edge_pair;
  mutable db::properties_id_type m_prop_id;

  void set () const
  {
    if (! m_iter.at_end ()) {
      m_iter->edge_pair (m_edge_pair);
      m_edge_pair.transform (m_iter.trans ());
      m_prop_id = m_iter->prop_id ();
    }
  }
};


DeepEdgePairs::DeepEdgePairs ()
  : MutableEdgePairs ()
{
  //  .. nothing yet ..
}

DeepEdgePairs::DeepEdgePairs (const RecursiveShapeIterator &si, DeepShapeStore &dss)
  : MutableEdgePairs ()
{
  set_deep_layer (dss.create_edge_pair_layer (si));
}

DeepEdgePairs::DeepEdgePairs (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans)
  : MutableEdgePairs ()
{
  set_deep_layer (dss.create_edge_pair_layer (si, trans));
}

DeepEdgePairs::DeepEdgePairs (const DeepEdgePairs &other)
  : MutableEdgePairs (other), db::DeepShapeCollectionDelegateBase (other)
{
  //  .. nothing yet ..
}

DeepEdgePairs::DeepEdgePairs (const DeepLayer &dl)
  : MutableEdgePairs ()
{
  set_deep_layer (dl);
}

DeepEdgePairs::~DeepEdgePairs ()
{
  //  .. nothing yet ..
}

EdgePairsDelegate *DeepEdgePairs::clone () const
{
  return new DeepEdgePairs (*this);
}

void DeepEdgePairs::do_insert (const db::EdgePair &edge_pair, db::properties_id_type prop_id)
{
  db::Layout &layout = deep_layer ().layout ();
  if (layout.begin_top_down () != layout.end_top_down ()) {
    db::Cell &top_cell = layout.cell (*layout.begin_top_down ());
    if (prop_id == 0) {
      top_cell.shapes (deep_layer ().layer ()).insert (edge_pair);
    } else {
      top_cell.shapes (deep_layer ().layer ()).insert (db::EdgePairWithProperties (edge_pair, prop_id));
    }
  }

  invalidate_bbox ();
  set_is_merged (false);
}

template <class Trans>
static void transform_deep_layer (db::DeepLayer &deep_layer, const Trans &t)
{
  //  TODO: this is a pretty cheap implementation. At least a plain move can be done with orientation variants.

  db::Layout &layout = deep_layer.layout ();
  if (layout.begin_top_down () != layout.end_top_down ()) {

    db::Cell &top_cell = layout.cell (*layout.begin_top_down ());

    db::Shapes flat_shapes (layout.is_editable ());
    for (db::RecursiveShapeIterator iter (layout, top_cell, deep_layer.layer ()); !iter.at_end (); ++iter) {
      flat_shapes.insert (iter->edge_pair ().transformed (iter.trans ()).transformed (t));
    }

    layout.clear_layer (deep_layer.layer ());
    top_cell.shapes (deep_layer.layer ()).swap (flat_shapes);

  }
}

void DeepEdgePairs::do_transform (const db::Trans &t)
{
  transform_deep_layer (deep_layer (), t);
  invalidate_bbox ();
}

void DeepEdgePairs::do_transform (const db::ICplxTrans &t)
{
  transform_deep_layer (deep_layer (), t);
  invalidate_bbox ();
}

void DeepEdgePairs::do_transform (const db::IMatrix2d &t)
{
  transform_deep_layer (deep_layer (), t);
  invalidate_bbox ();
}

void DeepEdgePairs::do_transform (const db::IMatrix3d &t)
{
  transform_deep_layer (deep_layer (), t);
  invalidate_bbox ();
}

void DeepEdgePairs::reserve (size_t)
{
  //  Not implemented for deep regions
}

void DeepEdgePairs::flatten ()
{
  db::Layout &layout = deep_layer ().layout ();
  if (layout.begin_top_down () != layout.end_top_down ()) {

    db::Cell &top_cell = layout.cell (*layout.begin_top_down ());

    db::Shapes flat_shapes (layout.is_editable ());
    for (db::RecursiveShapeIterator iter (layout, top_cell, deep_layer ().layer ()); !iter.at_end (); ++iter) {
      flat_shapes.insert (iter->edge_pair ().transformed (iter.trans ()));
    }

    layout.clear_layer (deep_layer ().layer ());
    top_cell.shapes (deep_layer ().layer ()).swap (flat_shapes);

  }
}

EdgePairsIteratorDelegate *DeepEdgePairs::begin () const
{
  return new DeepEdgePairsIterator (begin_iter ().first);
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans> DeepEdgePairs::begin_iter () const
{
  const db::Layout &layout = deep_layer ().layout ();
  if (layout.cells () == 0) {

    return std::make_pair (db::RecursiveShapeIterator (), db::ICplxTrans ());

  } else {

    const db::Cell &top_cell = layout.cell (*layout.begin_top_down ());
    db::RecursiveShapeIterator iter (deep_layer ().layout (), top_cell, deep_layer ().layer ());
    return std::make_pair (iter, db::ICplxTrans ());

  }
}

size_t DeepEdgePairs::count () const
{
  size_t n = 0;

  const db::Layout &layout = deep_layer ().layout ();
  db::CellCounter cc (&layout);
  for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
    n += cc.weight (*c) * layout.cell (*c).shapes (deep_layer ().layer ()).size ();
  }

  return n;
}

size_t DeepEdgePairs::hier_count () const
{
  size_t n = 0;

  const db::Layout &layout = deep_layer ().layout ();
  for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
    n += layout.cell (*c).shapes (deep_layer ().layer ()).size ();
  }

  return n;
}

std::string DeepEdgePairs::to_string (size_t nmax) const
{
  return db::AsIfFlatEdgePairs::to_string (nmax);
}

Box DeepEdgePairs::bbox () const
{
  return deep_layer ().initial_cell ().bbox (deep_layer ().layer ());
}

bool DeepEdgePairs::empty () const
{
  return begin_iter ().first.at_end ();
}

const db::EdgePair *DeepEdgePairs::nth (size_t) const
{
  throw tl::Exception (tl::to_string (tr ("Random access to edge pairs is available only for flat edge pair collections")));
}

bool DeepEdgePairs::has_valid_edge_pairs () const
{
  return false;
}

const db::RecursiveShapeIterator *DeepEdgePairs::iter () const
{
  return 0;
}

void DeepEdgePairs::apply_property_translator (const db::PropertiesTranslator &pt)
{
  DeepShapeCollectionDelegateBase::apply_property_translator (pt);
}

EdgePairsDelegate *
DeepEdgePairs::add_in_place (const EdgePairs &other)
{
  if (other.empty ()) {
    return this;
  }

  const DeepEdgePairs *other_deep = dynamic_cast <const DeepEdgePairs *> (other.delegate ());
  if (other_deep) {

    deep_layer ().add_from (other_deep->deep_layer ());

  } else {

    //  non-deep to deep merge (flat)

    db::Shapes &shapes = deep_layer ().initial_cell ().shapes (deep_layer ().layer ());
    for (db::EdgePairs::const_iterator p = other.begin (); ! p.at_end (); ++p) {
      if (p.prop_id () == 0) {
        shapes.insert (*p);
      } else {
        shapes.insert (db::EdgePairWithProperties (*p, p.prop_id ()));
      }
    }
  }

  return this;
}

EdgePairsDelegate *DeepEdgePairs::add (const EdgePairs &other) const
{
  if (other.empty ()) {
    return clone ();
  } else if (empty ()) {
    return other.delegate ()->clone ();
  } else {
    DeepEdgePairs *new_edge_pairs = dynamic_cast<DeepEdgePairs *> (clone ());
    new_edge_pairs->add_in_place (other);
    return new_edge_pairs;
  }
}

EdgePairsDelegate *
DeepEdgePairs::filter_in_place (const EdgePairFilterBase &filter)
{
  //  TODO: implement to be really in-place
  *this = *apply_filter (filter, true, false).first;
  return this;
}

EdgePairsDelegate *
DeepEdgePairs::filtered (const EdgePairFilterBase &filter) const
{
  return apply_filter (filter, true, false).first;
}

std::pair<EdgePairsDelegate *, EdgePairsDelegate *>
DeepEdgePairs::filtered_pair (const EdgePairFilterBase &filter) const
{
  return apply_filter (filter, true, true);
}

std::pair<DeepEdgePairs *, DeepEdgePairs *>
DeepEdgePairs::apply_filter (const EdgePairFilterBase &filter, bool with_true, bool with_false) const
{
  const db::DeepLayer &edge_pairs = deep_layer ();
  db::Layout &layout = const_cast<db::Layout &> (edge_pairs.layout ());

  std::unique_ptr<VariantsCollectorBase> vars;
  if (filter.vars ()) {

    vars.reset (new db::VariantsCollectorBase (filter.vars ()));

    vars->collect (&layout, edge_pairs.initial_cell ().cell_index ());

    if (filter.wants_variants ()) {
      vars->separate_variants ();
    }

  }

  std::map<db::cell_index_type, std::map<db::ICplxTrans, db::Shapes> > to_commit_true, to_commit_false;

  std::unique_ptr<db::DeepEdgePairs> res_true (with_true ? new db::DeepEdgePairs (edge_pairs.derived ()) : 0);
  std::unique_ptr<db::DeepEdgePairs> res_false (with_false ? new db::DeepEdgePairs (edge_pairs.derived ()) : 0);
  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

    const db::Shapes &s = c->shapes (edge_pairs.layer ());

    if (vars.get ()) {

      const std::set<db::ICplxTrans> &vv = vars->variants (c->cell_index ());
      for (auto v = vv.begin (); v != vv.end (); ++v) {

        db::Shapes *st_true = 0, *st_false = 0;
        if (vv.size () == 1) {
          if (with_true) {
            st_true = & c->shapes (res_true->deep_layer ().layer ());
          }
          if (with_false) {
            st_false = & c->shapes (res_false->deep_layer ().layer ());
          }
        } else {
          if (with_true) {
            st_true = & to_commit_true [c->cell_index ()] [*v];
          }
          if (with_false) {
            st_false = & to_commit_false [c->cell_index ()] [*v];
          }
        }

        const db::ICplxTrans &tr = *v;

        for (db::Shapes::shape_iterator si = s.begin (db::ShapeIterator::EdgePairs); ! si.at_end (); ++si) {
          if (filter.selected (si->edge_pair ().transformed (tr), si->prop_id ())) {
            if (st_true) {
              st_true->insert (*si);
            }
          } else {
            if (st_false) {
              st_false->insert (*si);
            }
          }
        }

      }

    } else {

      db::Shapes *st_true = with_true ? &c->shapes (res_true->deep_layer ().layer ()) : 0;
      db::Shapes *st_false = with_false ? &c->shapes (res_false->deep_layer ().layer ()) : 0;

      for (db::Shapes::shape_iterator si = s.begin (db::ShapeIterator::EdgePairs); ! si.at_end (); ++si) {
        if (filter.selected (si->edge_pair (), si->prop_id ())) {
          if (with_true) {
            st_true->insert (*si);
          }
        } else {
          if (with_false) {
            st_false->insert (*si);
          }
        }
      }

    }

  }

  if (! to_commit_true.empty () && vars.get ()) {
    tl_assert (res_true.get () != 0);
    vars->commit_shapes (res_true->deep_layer ().layer (), to_commit_true);
  }
  if (! to_commit_false.empty () && vars.get ()) {
    tl_assert (res_false.get () != 0);
    vars->commit_shapes (res_false->deep_layer ().layer (), to_commit_false);
  }

  return std::make_pair (res_true.release (), res_false.release ());
}

EdgePairsDelegate *DeepEdgePairs::process_in_place (const EdgePairProcessorBase &filter)
{
  //  TODO: implement to be really in-place
  return processed (filter);
}

EdgePairsDelegate *
DeepEdgePairs::processed (const EdgePairProcessorBase &filter) const
{
  return shape_collection_processed_impl<db::EdgePair, db::EdgePair, db::DeepEdgePairs> (deep_layer (), filter);
}

RegionDelegate *
DeepEdgePairs::processed_to_polygons (const EdgePairToPolygonProcessorBase &filter) const
{
  return shape_collection_processed_impl<db::EdgePair, db::Polygon, db::DeepRegion> (deep_layer (), filter);
}

EdgesDelegate *
DeepEdgePairs::processed_to_edges (const EdgePairToEdgeProcessorBase &filter) const
{
  return shape_collection_processed_impl<db::EdgePair, db::Edge, db::DeepEdges> (deep_layer (), filter);
}

RegionDelegate *DeepEdgePairs::polygons (db::Coord e) const
{
  db::DeepLayer new_layer = deep_layer ().derived ();
  db::Layout &layout = const_cast<db::Layout &> (deep_layer ().layout ());

  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
    db::Shapes &output = c->shapes (new_layer.layer ());
    for (db::Shapes::shape_iterator s = c->shapes (deep_layer ().layer ()).begin (db::ShapeIterator::EdgePairs); ! s.at_end (); ++s) {
      db::Polygon poly = s->edge_pair ().normalized ().to_polygon (e);
      if (poly.vertices () >= 3) {
        if (s->prop_id () != 0) {
          output.insert (db::PolygonRefWithProperties (db::PolygonRef (poly, layout.shape_repository ()), s->prop_id ()));
        } else {
          output.insert (db::PolygonRef (poly, layout.shape_repository ()));
        }
      }
    }
  }

  return new db::DeepRegion (new_layer);
}

EdgesDelegate *
DeepEdgePairs::pull_generic (const Edges &other) const
{
  std::unique_ptr<db::DeepEdges> dr_holder;
  const db::DeepEdges *other_deep = dynamic_cast<const db::DeepEdges *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchization
    dr_holder.reset (new db::DeepEdges (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  const db::DeepLayer &edge_pairs = deep_layer ();
  const db::DeepLayer &other_edges = other_deep->merged_deep_layer ();

  DeepLayer dl_out (other_edges.derived ());

  db::EdgePair2EdgePullLocalOperation op;

  db::local_processor<db::EdgePair, db::Edge, db::Edge> proc (const_cast<db::Layout *> (&edge_pairs.layout ()), const_cast<db::Cell *> (&edge_pairs.initial_cell ()), &other_edges.layout (), &other_edges.initial_cell (), edge_pairs.breakout_cells (), other_edges.breakout_cells ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (edge_pairs.store ()->threads ());

  proc.run (&op, edge_pairs.layer (), other_edges.layer (), dl_out.layer ());

  return new db::DeepEdges (dl_out);
}

RegionDelegate *
DeepEdgePairs::pull_generic (const Region &other) const
{
  std::unique_ptr<db::DeepRegion> dr_holder;
  const db::DeepRegion *other_deep = dynamic_cast<const db::DeepRegion *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchization
    dr_holder.reset (new db::DeepRegion (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  const db::DeepLayer &edge_pairs = deep_layer ();
  const db::DeepLayer &other_polygons = other_deep->merged_deep_layer ();

  DeepLayer dl_out (other_polygons.derived ());

  db::EdgePair2PolygonPullLocalOperation op;

  db::local_processor<db::EdgePair, db::PolygonRef, db::PolygonRef> proc (const_cast<db::Layout *> (&edge_pairs.layout ()), const_cast<db::Cell *> (&edge_pairs.initial_cell ()), &other_polygons.layout (), &other_polygons.initial_cell (), edge_pairs.breakout_cells (), other_polygons.breakout_cells ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (edge_pairs.store ()->threads ());

  proc.run (&op, edge_pairs.layer (), other_polygons.layer (), dl_out.layer ());

  return new db::DeepRegion (dl_out);
}

EdgePairsDelegate *
DeepEdgePairs::selected_interacting_generic (const Edges &other, bool inverse, size_t min_count, size_t max_count) const
{
  std::unique_ptr<db::DeepEdges> dr_holder;
  const db::DeepEdges *other_deep = dynamic_cast<const db::DeepEdges *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchization
    dr_holder.reset (new db::DeepEdges (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  min_count = std::max (size_t (1), min_count);
  bool counting = !(min_count == 1 && max_count == std::numeric_limits<size_t>::max ());

  const db::DeepLayer &edge_pairs = deep_layer ();

  DeepLayer dl_out (edge_pairs.derived ());

  db::EdgePair2EdgeInteractingLocalOperation op (inverse ? db::EdgePair2EdgeInteractingLocalOperation::Inverse : db::EdgePair2EdgeInteractingLocalOperation::Normal, min_count, max_count);

  db::local_processor<db::EdgePair, db::Edge, db::EdgePair> proc (const_cast<db::Layout *> (&edge_pairs.layout ()), const_cast<db::Cell *> (&edge_pairs.initial_cell ()), &other_deep->deep_layer ().layout (), &other_deep->deep_layer ().initial_cell (), edge_pairs.breakout_cells (), other_deep->deep_layer ().breakout_cells ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (edge_pairs.store ()->threads ());

  //  NOTE: with counting the other region needs to be merged
  proc.run (&op, edge_pairs.layer (), (counting ? other_deep->merged_deep_layer () : other_deep->deep_layer ()).layer (), dl_out.layer ());

  return new db::DeepEdgePairs (dl_out);
}

EdgePairsDelegate *
DeepEdgePairs::selected_interacting_generic (const Region &other, EdgePairInteractionMode mode, bool inverse, size_t min_count, size_t max_count) const
{
  std::unique_ptr<db::DeepRegion> dr_holder;
  const db::DeepRegion *other_deep = dynamic_cast<const db::DeepRegion *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchization
    dr_holder.reset (new db::DeepRegion (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  min_count = std::max (size_t (1), min_count);
  bool counting = !(min_count == 1 && max_count == std::numeric_limits<size_t>::max ());

  const db::DeepLayer &edge_pairs = deep_layer ();

  DeepLayer dl_out (edge_pairs.derived ());

  db::edge_pair_to_polygon_interacting_local_operation<db::PolygonRef> op (mode, inverse ? db::edge_pair_to_polygon_interacting_local_operation<db::PolygonRef>::Inverse : db::edge_pair_to_polygon_interacting_local_operation<db::PolygonRef>::Normal, min_count, max_count);

  db::local_processor<db::EdgePair, db::PolygonRef, db::EdgePair> proc (const_cast<db::Layout *> (&edge_pairs.layout ()), const_cast<db::Cell *> (&edge_pairs.initial_cell ()), &other_deep->deep_layer ().layout (), &other_deep->deep_layer ().initial_cell (), edge_pairs.breakout_cells (), other_deep->deep_layer ().breakout_cells ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (edge_pairs.store ()->threads ());

  //  NOTE: with counting the other region needs to be merged
  proc.run (&op, edge_pairs.layer (), (counting || mode != EdgePairsInteract ? other_deep->merged_deep_layer () : other_deep->deep_layer ()).layer (), dl_out.layer ());

  return new db::DeepEdgePairs (dl_out);
}

std::pair<EdgePairsDelegate *, EdgePairsDelegate *>
DeepEdgePairs::selected_interacting_pair_generic (const Edges &other, size_t min_count, size_t max_count) const
{
  std::unique_ptr<db::DeepEdges> dr_holder;
  const db::DeepEdges *other_deep = dynamic_cast<const db::DeepEdges *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchization
    dr_holder.reset (new db::DeepEdges (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  min_count = std::max (size_t (1), min_count);
  bool counting = !(min_count == 1 && max_count == std::numeric_limits<size_t>::max ());

  const db::DeepLayer &edge_pairs = deep_layer ();

  DeepLayer dl_out (edge_pairs.derived ());
  DeepLayer dl_out2 (edge_pairs.derived ());

  std::vector<unsigned int> output_layers;
  output_layers.reserve (2);
  output_layers.push_back (dl_out.layer ());
  output_layers.push_back (dl_out2.layer ());

  db::EdgePair2EdgeInteractingLocalOperation op (db::EdgePair2EdgeInteractingLocalOperation::Both, min_count, max_count);

  db::local_processor<db::EdgePair, db::Edge, db::EdgePair> proc (const_cast<db::Layout *> (&edge_pairs.layout ()), const_cast<db::Cell *> (&edge_pairs.initial_cell ()), &other_deep->deep_layer ().layout (), &other_deep->deep_layer ().initial_cell (), edge_pairs.breakout_cells (), other_deep->deep_layer ().breakout_cells ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (edge_pairs.store ()->threads ());

  //  NOTE: with counting the other region needs to be merged
  proc.run (&op, edge_pairs.layer (), (counting ? other_deep->merged_deep_layer () : other_deep->deep_layer ()).layer (), output_layers);

  return std::make_pair (new db::DeepEdgePairs (dl_out), new db::DeepEdgePairs (dl_out2));
}

std::pair<EdgePairsDelegate *, EdgePairsDelegate *>
DeepEdgePairs::selected_interacting_pair_generic (const Region &other, EdgePairInteractionMode mode, size_t min_count, size_t max_count) const
{
  std::unique_ptr<db::DeepRegion> dr_holder;
  const db::DeepRegion *other_deep = dynamic_cast<const db::DeepRegion *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchization
    dr_holder.reset (new db::DeepRegion (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  min_count = std::max (size_t (1), min_count);
  bool counting = !(min_count == 1 && max_count == std::numeric_limits<size_t>::max ());

  const db::DeepLayer &edge_pairs = deep_layer ();

  DeepLayer dl_out (edge_pairs.derived ());
  DeepLayer dl_out2 (edge_pairs.derived ());

  std::vector<unsigned int> output_layers;
  output_layers.reserve (2);
  output_layers.push_back (dl_out.layer ());
  output_layers.push_back (dl_out2.layer ());

  db::edge_pair_to_polygon_interacting_local_operation<db::PolygonRef> op (mode, db::edge_pair_to_polygon_interacting_local_operation<db::PolygonRef>::Both, min_count, max_count);

  db::local_processor<db::EdgePair, db::PolygonRef, db::EdgePair> proc (const_cast<db::Layout *> (&edge_pairs.layout ()), const_cast<db::Cell *> (&edge_pairs.initial_cell ()), &other_deep->deep_layer ().layout (), &other_deep->deep_layer ().initial_cell (), edge_pairs.breakout_cells (), other_deep->deep_layer ().breakout_cells ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (edge_pairs.store ()->threads ());

  //  NOTE: with counting the other region needs to be merged
  proc.run (&op, edge_pairs.layer (), (counting || mode != EdgePairsInteract ? other_deep->merged_deep_layer () : other_deep->deep_layer ()).layer (), output_layers);

  return std::make_pair (new db::DeepEdgePairs (dl_out), new db::DeepEdgePairs (dl_out2));
}

EdgesDelegate *DeepEdgePairs::generic_edges (bool first, bool second) const
{
  db::DeepLayer new_layer = deep_layer ().derived ();
  db::Layout &layout = const_cast<db::Layout &> (deep_layer ().layout ());

  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
    db::Shapes &output = c->shapes (new_layer.layer ());
    for (db::Shapes::shape_iterator s = c->shapes (deep_layer ().layer ()).begin (db::ShapeIterator::EdgePairs); ! s.at_end (); ++s) {
      db::EdgePair ep = s->edge_pair ();
      if (first) {
        if (s->prop_id () != 0) {
          output.insert (db::EdgeWithProperties (ep.first (), s->prop_id ()));
        } else {
          output.insert (ep.first ());
        }
      }
      if (second) {
        if (s->prop_id () != 0) {
          output.insert (db::EdgeWithProperties (ep.second (), s->prop_id ()));
        } else {
          output.insert (ep.second ());
        }
      }
    }
  }

  return new db::DeepEdges (new_layer);
}

EdgesDelegate *DeepEdgePairs::edges () const
{
  return generic_edges (true, true);
}

EdgesDelegate *DeepEdgePairs::first_edges () const
{
  return generic_edges (true, false);
}

EdgesDelegate *DeepEdgePairs::second_edges () const
{
  return generic_edges (false, true);
}

EdgePairsDelegate *DeepEdgePairs::in (const EdgePairs &other, bool invert) const
{
  //  TODO: implement
  return AsIfFlatEdgePairs::in (other, invert);
}

bool DeepEdgePairs::equals (const EdgePairs &other) const
{
  const DeepEdgePairs *other_delegate = dynamic_cast<const DeepEdgePairs *> (other.delegate ());
  if (other_delegate && &other_delegate->deep_layer ().layout () == &deep_layer ().layout ()
      && other_delegate->deep_layer ().layer () == deep_layer ().layer ()) {
    return true;
  } else {
    return AsIfFlatEdgePairs::equals (other);
  }
}

bool DeepEdgePairs::less (const EdgePairs &other) const
{
  const DeepEdgePairs *other_delegate = dynamic_cast<const DeepEdgePairs *> (other.delegate ());
  if (other_delegate && &other_delegate->deep_layer ().layout () == &deep_layer ().layout ()) {
    return other_delegate->deep_layer ().layer () < deep_layer ().layer ();
  } else {
    return AsIfFlatEdgePairs::less (other);
  }
}

void DeepEdgePairs::insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  deep_layer ().insert_into (layout, into_cell, into_layer);
}

void DeepEdgePairs::insert_into_as_polygons (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl) const
{
  deep_layer ().insert_into_as_polygons (layout, into_cell, into_layer, enl);
}

DeepEdgePairs &DeepEdgePairs::operator= (const DeepEdgePairs &other)
{
  if (this != &other) {
    AsIfFlatEdgePairs::operator= (other);
    DeepShapeCollectionDelegateBase::operator= (other);
  }

  return *this;
}

}
