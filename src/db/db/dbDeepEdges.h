
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


#ifndef HDR_dbDeepEdges
#define HDR_dbDeepEdges

#include "dbCommon.h"

#include "dbMutableEdges.h"
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
  : public db::MutableEdges, public db::DeepShapeCollectionDelegateBase
{
public:
  DeepEdges ();
  DeepEdges (const db::Edges &other, DeepShapeStore &dss);
  DeepEdges (const RecursiveShapeIterator &si, DeepShapeStore &dss, bool as_edges = true);
  DeepEdges (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans, bool as_edges = true, bool merged_semantics = true);

  DeepEdges (const DeepEdges &other);
  DeepEdges (const DeepLayer &dl);

  virtual ~DeepEdges ();

  virtual void do_transform (const db::Trans &t);
  virtual void do_transform (const db::ICplxTrans &t);
  virtual void do_transform (const db::IMatrix2d &t);
  virtual void do_transform (const db::IMatrix3d &t);

  virtual void flatten ();

  virtual void reserve (size_t n);

  virtual void do_insert (const db::Edge &edge, properties_id_type prop_id);

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
  virtual void apply_property_translator (const db::PropertiesTranslator &pt);
  virtual db::PropertiesRepository *properties_repository ();
  virtual const db::PropertiesRepository *properties_repository () const;

  virtual bool equals (const Edges &other) const;
  virtual bool less (const Edges &other) const;

  virtual size_t count () const;
  virtual size_t hier_count () const;
  virtual Box bbox () const;

  virtual DeepEdges::length_type length (const db::Box &) const;

  virtual std::string to_string (size_t nmax) const;

  virtual EdgePairsDelegate *width_check (db::Coord d, const db::EdgesCheckOptions &options) const
  {
    return run_check (db::WidthRelation, 0, d, options);
  }
    
  virtual EdgePairsDelegate *space_check (db::Coord d, const db::EdgesCheckOptions &options) const
  {
    return run_check (db::SpaceRelation, 0, d, options);
  }

  virtual EdgePairsDelegate *enclosing_check (const Edges &other, db::Coord d, const db::EdgesCheckOptions &options) const
  {
    return run_check (db::OverlapRelation, &other, d, options);
  }

  virtual EdgePairsDelegate *overlap_check (const Edges &other, db::Coord d, const db::EdgesCheckOptions &options) const
  {
    return run_check (db::WidthRelation, &other, d, options);
  }

  virtual EdgePairsDelegate *separation_check (const Edges &other, db::Coord d, const db::EdgesCheckOptions &options) const
  {
    return run_check (db::SpaceRelation, &other, d, options);
  }

  virtual EdgePairsDelegate *inside_check (const Edges &other, db::Coord d, const db::EdgesCheckOptions &options) const
  {
    return run_check (db::InsideRelation, &other, d, options);
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
  virtual EdgesDelegate *not_with (const Edges &other) const;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> andnot_with (const Edges &) const;

  virtual EdgesDelegate *and_with (const Region &other) const;
  virtual EdgesDelegate *not_with (const Region &other) const;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> andnot_with (const Region &) const;

  virtual EdgesDelegate *xor_with (const Edges &other) const;

  virtual EdgesDelegate *or_with (const Edges &other) const;

  virtual EdgesDelegate *add_in_place (const Edges &other);
  virtual EdgesDelegate *add (const Edges &other) const;

  virtual EdgesDelegate *intersections (const Edges &other) const;

  virtual EdgesDelegate *inside_part (const Region &other) const;
  virtual EdgesDelegate *outside_part (const Region &other) const;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> inside_outside_part_pair (const Region &) const;

  virtual RegionDelegate *extended (coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i, bool join) const;

  virtual EdgesDelegate *in (const Edges &, bool) const;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> in_and_out (const Edges &) const;

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
  std::pair<DeepLayer, DeepLayer> edge_region_op (const DeepRegion *other, EdgePolygonOp::mode_t op, bool include_borders) const;
  EdgePairsDelegate *run_check (db::edge_relation_type rel, const Edges *other, db::Coord d, const db::EdgesCheckOptions &options) const;
  virtual EdgesDelegate *pull_generic (const Edges &edges) const;
  virtual RegionDelegate *pull_generic (const Region &region) const;
  virtual EdgesDelegate *selected_interacting_generic (const Edges &edges, EdgeInteractionMode mode, bool inverse) const;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_interacting_pair_generic (const Edges &edges, EdgeInteractionMode mode) const;
  virtual EdgesDelegate *selected_interacting_generic (const Region &region, EdgeInteractionMode mode, bool inverse) const;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_interacting_pair_generic (const Region &region, EdgeInteractionMode mode) const;
  DeepEdges *apply_filter (const EdgeFilterBase &filter) const;

  template <class Result, class OutputContainer> OutputContainer *processed_impl (const edge_processor<Result> &filter) const;
};

}

#endif

