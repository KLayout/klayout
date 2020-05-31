
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#ifndef HDR_dbDeepEdges
#define HDR_dbDeepEdges

#include "dbCommon.h"

#include "dbAsIfFlatEdges.h"
#include "dbDeepShapeStore.h"
#include "dbEdgeBoolean.h"
#include "dbEdgePairs.h"

namespace db {

class Edges;
class DeepRegion;

/**
 *  @brief Provides hierarchical edges implementation
 */
class DB_PUBLIC DeepEdges
  : public db::AsIfFlatEdges, public db::DeepShapeCollectionDelegateBase
{
public:
  DeepEdges ();
  DeepEdges (const db::Edges &other, DeepShapeStore &dss);
  DeepEdges (const RecursiveShapeIterator &si, DeepShapeStore &dss, bool as_edges = true);
  DeepEdges (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans, bool as_edges = true, bool merged_semantics = true);

  DeepEdges (const DeepEdges &other);
  DeepEdges (const DeepLayer &dl);

  virtual ~DeepEdges ();

  EdgesDelegate *clone () const;

  virtual EdgesIteratorDelegate *begin () const;
  virtual EdgesIteratorDelegate *begin_merged () const;

  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const;
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_merged_iter () const;

  virtual bool empty () const;
  virtual bool is_merged () const;

  virtual const db::Edge *nth (size_t n) const;
  virtual bool has_valid_edges () const;
  virtual bool has_valid_merged_edges () const;

  virtual const db::RecursiveShapeIterator *iter () const;

  virtual bool equals (const Edges &other) const;
  virtual bool less (const Edges &other) const;

  virtual size_t size () const;
  virtual Box bbox () const;

  virtual DeepEdges::length_type length (const db::Box &) const;

  virtual std::string to_string (size_t nmax) const;

  virtual EdgePairsDelegate *width_check (db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::WidthRelation, 0, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }
    
  virtual EdgePairsDelegate *space_check (db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::SpaceRelation, 0, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  virtual EdgePairsDelegate *enclosing_check (const Edges &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::OverlapRelation, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  virtual EdgePairsDelegate *overlap_check (const Edges &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::WidthRelation, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  virtual EdgePairsDelegate *separation_check (const Edges &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::SpaceRelation, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  virtual EdgePairsDelegate *inside_check (const Edges &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::InsideRelation, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  virtual EdgesDelegate *filter_in_place (const EdgeFilterBase &filter);
  virtual EdgesDelegate *filtered (const EdgeFilterBase &) const;
  virtual EdgesDelegate *process_in_place (const EdgeProcessorBase &);
  virtual EdgesDelegate *processed (const EdgeProcessorBase &) const;
  virtual EdgePairsDelegate *processed_to_edge_pairs (const EdgeToEdgePairProcessorBase &filter) const;
  virtual RegionDelegate *processed_to_polygons (const EdgeToPolygonProcessorBase &filter) const;

  virtual EdgesDelegate *merged_in_place ();
  virtual EdgesDelegate *merged () const;

  virtual EdgesDelegate *and_with (const Edges &other) const;
  virtual EdgesDelegate *and_with (const Region &other) const;

  virtual EdgesDelegate *not_with (const Edges &other) const;
  virtual EdgesDelegate *not_with (const Region &other) const;

  virtual EdgesDelegate *xor_with (const Edges &other) const;

  virtual EdgesDelegate *or_with (const Edges &other) const;

  virtual EdgesDelegate *add_in_place (const Edges &other);
  virtual EdgesDelegate *add (const Edges &other) const;

  virtual EdgesDelegate *intersections (const Edges &other) const;

  virtual EdgesDelegate *inside_part (const Region &other) const;
  virtual EdgesDelegate *outside_part (const Region &other) const;

  virtual RegionDelegate *extended (coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i, bool join) const;

  virtual EdgesDelegate *in (const Edges &, bool) const;

  virtual void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const;

  virtual DeepShapeCollectionDelegateBase *deep ()
  {
    return this;
  }

  void set_is_merged (bool f);

protected:
  virtual void merged_semantics_changed ();

private:
  friend class DeepRegion;

  DeepEdges &operator= (const DeepEdges &other);

  mutable DeepLayer m_merged_edges;
  mutable bool m_merged_edges_valid;
  bool m_is_merged;

  void init ();
  void ensure_merged_edges_valid () const;
  const DeepLayer &merged_deep_layer () const;
  DeepLayer and_or_not_with(const DeepEdges *other, EdgeBoolOp op) const;
  DeepLayer edge_region_op (const DeepRegion *other, bool outside, bool include_borders) const;
  EdgePairsDelegate *run_check (db::edge_relation_type rel, const Edges *other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const;
  virtual EdgesDelegate *pull_generic (const Edges &edges) const;
  virtual RegionDelegate *pull_generic (const Region &region) const;
  virtual EdgesDelegate *selected_interacting_generic (const Edges &edges, bool invert) const;
  virtual EdgesDelegate *selected_interacting_generic (const Region &region, bool invert) const;
  DeepEdges *apply_filter (const EdgeFilterBase &filter) const;

  template <class Result, class OutputContainer> OutputContainer *processed_impl (const edge_processor<Result> &filter) const;
};

}

#endif

