
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


#ifndef HDR_dbDeepRegion
#define HDR_dbDeepRegion

#include "dbCommon.h"

#include "dbMutableRegion.h"
#include "dbDeepShapeStore.h"

namespace db {

/**
 *  @brief A deep, polygon-set delegate
 */
class DB_PUBLIC DeepRegion
  : public MutableRegion, public DeepShapeCollectionDelegateBase
{
public:
  typedef db::layer<db::Polygon, db::unstable_layer_tag> polygon_layer_type;
  typedef polygon_layer_type::iterator polygon_iterator_type;

  DeepRegion ();
  DeepRegion (const db::Region &other, DeepShapeStore &dss);
  DeepRegion (const RecursiveShapeIterator &si, DeepShapeStore &dss, double area_ratio = 0.0, size_t max_vertex_count = 0);
  DeepRegion (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans, bool merged_semantics = true, double area_ratio = 0.0, size_t max_vertex_count = 0);

  DeepRegion (const DeepRegion &other);
  DeepRegion (const DeepLayer &dl);

  virtual ~DeepRegion ();

  RegionDelegate *clone () const;

  virtual void do_insert (const db::Polygon &polygon, db::properties_id_type prop_id);

  virtual void do_transform (const db::Trans &t);
  virtual void do_transform (const db::ICplxTrans &t);
  virtual void do_transform (const db::IMatrix2d &t);
  virtual void do_transform (const db::IMatrix3d &t);

  virtual void flatten ();

  virtual void reserve (size_t);

  virtual RegionIteratorDelegate *begin () const;
  virtual RegionIteratorDelegate *begin_merged () const;

  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const;
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_merged_iter () const;

  virtual bool empty () const;
  virtual bool is_merged () const;

  virtual const db::Polygon *nth (size_t n) const;
  virtual db::properties_id_type nth_prop_id (size_t n) const;
  virtual bool has_valid_polygons () const;
  virtual bool has_valid_merged_polygons () const;

  virtual const db::RecursiveShapeIterator *iter () const;
  virtual void apply_property_translator (const db::PropertiesTranslator &pt);
  virtual db::PropertiesRepository *properties_repository ();
  virtual const db::PropertiesRepository *properties_repository () const;

  virtual bool equals (const Region &other) const;
  virtual bool less (const Region &other) const;

  virtual bool is_box () const;
  virtual size_t count () const;
  virtual size_t hier_count () const;

  virtual area_type area (const db::Box &box) const;
  virtual perimeter_type perimeter (const db::Box &box) const;
  virtual Box bbox () const;

  virtual std::string to_string (size_t nmax) const;

  virtual EdgePairsDelegate *cop_to_edge_pairs (db::CompoundRegionOperationNode &node, db::PropertyConstraint prop_constraint);
  virtual RegionDelegate *cop_to_region (db::CompoundRegionOperationNode &node, db::PropertyConstraint prop_constraint);
  virtual EdgesDelegate *cop_to_edges (db::CompoundRegionOperationNode &node, db::PropertyConstraint prop_constraint);

  virtual RegionDelegate *and_with (const Region &other, db::PropertyConstraint property_constraint) const;
  virtual RegionDelegate *not_with (const Region &other, db::PropertyConstraint property_constraint) const;
  virtual RegionDelegate *xor_with (const Region &other, db::PropertyConstraint property_constraint) const;
  virtual RegionDelegate *or_with (const Region &other, db::PropertyConstraint property_constraint) const;
  virtual std::pair<RegionDelegate *, RegionDelegate *> andnot_with (const Region &, db::PropertyConstraint property_constraint) const;

  virtual RegionDelegate *add_in_place (const Region &other);
  virtual RegionDelegate *add (const Region &other) const;

  virtual EdgePairsDelegate *grid_check (db::Coord gx, db::Coord gy) const;
  virtual EdgePairsDelegate *angle_check (double min, double max, bool inverse) const;

  virtual RegionDelegate *snapped_in_place (db::Coord gx, db::Coord gy)
  {
    return snapped (gx, gy);
  }

  virtual RegionDelegate *snapped (db::Coord gx, db::Coord gy);

  virtual EdgesDelegate *edges (const EdgeFilterBase *) const;

  virtual RegionDelegate *process_in_place (const PolygonProcessorBase &filter);
  virtual RegionDelegate *processed (const PolygonProcessorBase &filter) const;
  virtual EdgesDelegate *processed_to_edges (const PolygonToEdgeProcessorBase &filter) const;
  virtual EdgePairsDelegate *processed_to_edge_pairs (const PolygonToEdgePairProcessorBase &filter) const;
  virtual RegionDelegate *filter_in_place (const PolygonFilterBase &filter);
  virtual RegionDelegate *filtered (const PolygonFilterBase &filter) const;

  virtual RegionDelegate *merged_in_place ();
  virtual RegionDelegate *merged_in_place (bool min_coherence, unsigned int min_wc);

  virtual RegionDelegate *merged () const;
  virtual RegionDelegate *merged (bool min_coherence, unsigned int min_wc) const;

  virtual RegionDelegate *sized (coord_type d, unsigned int mode) const;
  virtual RegionDelegate *sized (coord_type dx, coord_type dy, unsigned int mode) const;

  virtual void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const;

  virtual RegionDelegate *nets (LayoutToNetlist *l2n, NetPropertyMode prop_mode, const tl::Variant &net_prop_name, const std::vector<const Net *> *nets) const;

  virtual DeepShapeCollectionDelegateBase *deep ()
  {
    return this;
  }

  void set_is_merged (bool f);

  bool merged_polygons_available () const;
  const DeepLayer &merged_deep_layer () const;

protected:
  virtual void merged_semantics_changed ();
  virtual void min_coherence_changed ();

  virtual EdgePairsDelegate *run_check (db::edge_relation_type rel, bool different_polygons, const Region *other, db::Coord d, const RegionCheckOptions &options) const;
  virtual EdgePairsDelegate *run_single_polygon_check (db::edge_relation_type rel, db::Coord d, const RegionCheckOptions &options) const;
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_interacting_generic (const Region &other, int mode, bool touching, InteractingOutputMode output_mode, size_t min_count, size_t max_count) const;
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_interacting_generic (const Edges &other, InteractingOutputMode output_mode, size_t min_count, size_t max_count) const;
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_interacting_generic (const Texts &other, InteractingOutputMode output_mode, size_t min_count, size_t max_count) const;
  virtual RegionDelegate *pull_generic (const Region &other, int mode, bool touching) const;
  virtual EdgesDelegate *pull_generic (const Edges &other) const;
  virtual TextsDelegate *pull_generic (const Texts &other) const;
  virtual std::pair<RegionDelegate *, RegionDelegate *> in_and_out_generic (const Region &other, InteractingOutputMode output_mode) const;

private:
  friend class DeepEdges;
  friend class DeepTexts;

  DeepRegion &operator= (const DeepRegion &other);

  mutable DeepLayer m_merged_polygons;
  mutable bool m_merged_polygons_valid;
  bool m_is_merged;

  void init ();
  void ensure_merged_polygons_valid () const;
  DeepLayer and_or_not_with(const DeepRegion *other, bool and_op, PropertyConstraint property_constraint) const;
  std::pair<DeepLayer, DeepLayer> and_and_not_with (const DeepRegion *other, PropertyConstraint property_constraint) const;
  DeepRegion *apply_filter (const PolygonFilterBase &filter) const;

  template <class Result, class OutputContainer> OutputContainer *processed_impl (const polygon_processor<Result> &filter) const;

  template <class Proc>
  void configure_proc (Proc &proc) const
  {
    proc.set_description (progress_desc ());
    proc.set_report_progress (report_progress ());
    proc.set_base_verbosity (base_verbosity ());
  }
};

}

#endif

