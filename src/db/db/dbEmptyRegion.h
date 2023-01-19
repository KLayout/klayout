
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


#ifndef HDR_dbEmptyRegion
#define HDR_dbEmptyRegion

#include "dbCommon.h"
#include "dbRegionDelegate.h"
#include "dbEmptyEdges.h"
#include "dbEmptyTexts.h"

namespace db {

/**
 *  @brief An empty Region
 */
class DB_PUBLIC EmptyRegion
  : public RegionDelegate
{
public:
  EmptyRegion ();
  virtual ~EmptyRegion ();

  EmptyRegion (const EmptyRegion &other);
  RegionDelegate *clone () const;

  virtual RegionIteratorDelegate *begin () const { return 0; }
  virtual RegionIteratorDelegate *begin_merged () const { return 0; }

  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const { return std::make_pair (db::RecursiveShapeIterator (), db::ICplxTrans ()); }
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_merged_iter () const { return std::make_pair (db::RecursiveShapeIterator (), db::ICplxTrans ()); }

  virtual bool empty () const { return true; }
  virtual size_t count () const { return 0; }
  virtual size_t hier_count () const { return 0; }
  virtual std::string to_string (size_t) const { return std::string (); }

  virtual bool is_box () const { return false; }
  virtual bool is_merged () const { return true; }
  virtual area_type area (const db::Box &) const { return 0; }
  virtual perimeter_type perimeter (const db::Box &) const { return 0; }

  virtual Box bbox () const { return Box (); }

  virtual EdgePairsDelegate *cop_to_edge_pairs (db::CompoundRegionOperationNode &node, PropertyConstraint);
  virtual RegionDelegate *cop_to_region (db::CompoundRegionOperationNode &node, PropertyConstraint);
  virtual EdgesDelegate *cop_to_edges (db::CompoundRegionOperationNode &node, PropertyConstraint);

  virtual EdgePairsDelegate *width_check (db::Coord, const RegionCheckOptions &) const;
  virtual EdgePairsDelegate *space_check (db::Coord, const RegionCheckOptions &) const;
  virtual EdgePairsDelegate *isolated_check (db::Coord, const RegionCheckOptions &) const;
  virtual EdgePairsDelegate *notch_check (db::Coord, const RegionCheckOptions &) const;
  virtual EdgePairsDelegate *enclosing_check (const Region &, db::Coord, const RegionCheckOptions &) const;
  virtual EdgePairsDelegate *overlap_check (const Region &, db::Coord, const RegionCheckOptions &) const;
  virtual EdgePairsDelegate *separation_check (const Region &, db::Coord, const RegionCheckOptions &) const;
  virtual EdgePairsDelegate *inside_check (const Region &, db::Coord, const RegionCheckOptions &) const;
  virtual EdgePairsDelegate *grid_check (db::Coord, db::Coord) const;
  virtual EdgePairsDelegate *angle_check (double, double, bool) const;

  virtual RegionDelegate *snapped_in_place (db::Coord, db::Coord) { return this; }
  virtual RegionDelegate *snapped (db::Coord, db::Coord) { return new EmptyRegion (); }
  virtual RegionDelegate *scaled_and_snapped_in_place (db::Coord, db::Coord, db::Coord, db::Coord, db::Coord, db::Coord) { return this; }
  virtual RegionDelegate *scaled_and_snapped (db::Coord, db::Coord, db::Coord, db::Coord, db::Coord, db::Coord) { return new EmptyRegion (); }

  virtual EdgesDelegate *edges (const EdgeFilterBase *) const;
  virtual RegionDelegate *filter_in_place (const PolygonFilterBase &) { return this; }
  virtual RegionDelegate *filtered (const PolygonFilterBase &) const { return new EmptyRegion (); }
  virtual RegionDelegate *process_in_place (const PolygonProcessorBase &) { return this; }
  virtual RegionDelegate *processed (const PolygonProcessorBase &) const { return new EmptyRegion (); }
  virtual EdgesDelegate *processed_to_edges (const PolygonToEdgeProcessorBase &) const;
  virtual EdgePairsDelegate *processed_to_edge_pairs (const PolygonToEdgePairProcessorBase &) const;

  virtual RegionDelegate *merged_in_place () { return this; }
  virtual RegionDelegate *merged_in_place (bool, unsigned int) { return this; }
  virtual RegionDelegate *merged () const { return new EmptyRegion (); }
  virtual RegionDelegate *merged (bool, unsigned int) const { return new EmptyRegion (); }

  virtual RegionDelegate *sized (coord_type, unsigned int) const { return new EmptyRegion (); }
  virtual RegionDelegate *sized (coord_type, coord_type, unsigned int) const { return new EmptyRegion (); }

  virtual RegionDelegate *and_with (const Region &, db::PropertyConstraint) const { return new EmptyRegion (); }
  virtual RegionDelegate *not_with (const Region &, db::PropertyConstraint) const { return new EmptyRegion (); }
  virtual std::pair<RegionDelegate *, RegionDelegate *> andnot_with (const Region &, db::PropertyConstraint) const { return std::make_pair (new EmptyRegion (), new EmptyRegion ()); }
  virtual RegionDelegate *xor_with (const Region &other, db::PropertyConstraint prop_constraint) const;
  virtual RegionDelegate *or_with (const Region &other, db::PropertyConstraint prop_constraint) const;
  virtual RegionDelegate *add_in_place (const Region &other);
  virtual RegionDelegate *add (const Region &other) const;

  virtual RegionDelegate *selected_outside (const Region &) const { return new EmptyRegion (); }
  virtual RegionDelegate *selected_not_outside (const Region &) const { return new EmptyRegion (); }
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_outside_pair (const Region &) const { return std::make_pair (new EmptyRegion (), new EmptyRegion ()); }
  virtual RegionDelegate *selected_inside (const Region &) const { return new EmptyRegion (); }
  virtual RegionDelegate *selected_not_inside (const Region &) const { return new EmptyRegion (); }
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_inside_pair (const Region &) const { return std::make_pair (new EmptyRegion (), new EmptyRegion ()); }
  virtual RegionDelegate *selected_enclosing (const Region &, size_t, size_t) const { return new EmptyRegion (); }
  virtual RegionDelegate *selected_not_enclosing (const Region &, size_t, size_t) const { return new EmptyRegion (); }
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_enclosing_pair (const Region &, size_t, size_t) const { return std::make_pair (new EmptyRegion (), new EmptyRegion ()); }
  virtual RegionDelegate *selected_interacting (const Region &, size_t, size_t) const { return new EmptyRegion (); }
  virtual RegionDelegate *selected_not_interacting (const Region &, size_t, size_t) const { return new EmptyRegion (); }
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_interacting_pair (const Region &, size_t, size_t) const { return std::make_pair (new EmptyRegion (), new EmptyRegion ()); }
  virtual RegionDelegate *selected_interacting (const Edges &, size_t, size_t) const { return new EmptyRegion (); }
  virtual RegionDelegate *selected_not_interacting (const Edges &, size_t, size_t) const { return new EmptyRegion (); }
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_interacting_pair (const Edges &, size_t, size_t) const { return std::make_pair (new EmptyRegion (), new EmptyRegion ()); }
  virtual RegionDelegate *selected_interacting (const Texts &, size_t, size_t) const { return new EmptyRegion (); }
  virtual RegionDelegate *selected_not_interacting (const Texts &, size_t, size_t) const { return new EmptyRegion (); }
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_interacting_pair (const Texts &, size_t, size_t) const { return std::make_pair (new EmptyRegion (), new EmptyRegion ()); }
  virtual RegionDelegate *selected_overlapping (const Region &, size_t, size_t) const { return new EmptyRegion (); }
  virtual RegionDelegate *selected_not_overlapping (const Region &, size_t, size_t) const { return new EmptyRegion (); }
  virtual std::pair<RegionDelegate *, RegionDelegate *> selected_overlapping_pair (const Region &, size_t, size_t) const { return std::make_pair (new EmptyRegion (), new EmptyRegion ()); }
  virtual RegionDelegate *pull_inside (const Region &) const  { return new EmptyRegion (); }
  virtual RegionDelegate *pull_interacting (const Region &) const  { return new EmptyRegion (); }
  virtual EdgesDelegate *pull_interacting (const Edges &) const  { return new EmptyEdges (); }
  virtual TextsDelegate *pull_interacting (const Texts &) const  { return new EmptyTexts (); }
  virtual RegionDelegate *pull_overlapping (const Region &) const  { return new EmptyRegion (); }
  virtual RegionDelegate *in (const Region &, bool) const { return new EmptyRegion (); }
  virtual std::pair<RegionDelegate *, RegionDelegate *> in_and_out (const Region &) const { return std::make_pair (new EmptyRegion (), new EmptyRegion ()); }

  virtual bool has_valid_polygons () const { return true; }
  virtual bool has_valid_merged_polygons () const { return true; }
  virtual const db::Polygon *nth (size_t) const { tl_assert (false); }
  virtual db::properties_id_type nth_prop_id (size_t) const { tl_assert (false); }

  virtual const db::RecursiveShapeIterator *iter () const { return 0; }
  virtual void apply_property_translator (const db::PropertiesTranslator &) { }
  virtual db::PropertiesRepository *properties_repository () { return 0; }
  virtual const db::PropertiesRepository *properties_repository () const { return 0; }

  virtual bool equals (const Region &other) const;
  virtual bool less (const Region &other) const;

  virtual void insert_into (Layout *, db::cell_index_type, unsigned int) const { }

  virtual RegionDelegate *nets (LayoutToNetlist *, NetPropertyMode, const tl::Variant &, const std::vector<const db::Net *> *) const { return new EmptyRegion (); }

private:
  EmptyRegion &operator= (const EmptyRegion &other);
};

}  // namespace db

#endif

