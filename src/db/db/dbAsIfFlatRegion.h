
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


#ifndef HDR_dbAsIfFlatRegion
#define HDR_dbAsIfFlatRegion

#include "dbCommon.h"

#include "dbRegionDelegate.h"
#include "dbPolygon.h"
#include "dbEdge.h"
#include "dbBoxScanner.h"
#include "dbEdgePairRelations.h"

#include <set>

namespace db {

/**
 *  @brief Provides default flat implementations
 */
class DB_PUBLIC AsIfFlatRegion
  : public RegionDelegate
{
public:
  AsIfFlatRegion ();
  virtual ~AsIfFlatRegion ();

  virtual bool is_box () const;
  virtual size_t size () const;

  virtual area_type area (const db::Box &box) const;
  virtual perimeter_type perimeter (const db::Box &box) const;
  virtual Box bbox () const;

  virtual std::string to_string (size_t nmax) const;

  EdgePairsDelegate *width_check (db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_single_polygon_check (db::WidthRelation, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairsDelegate *space_check (db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::SpaceRelation, false, 0, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairsDelegate *isolated_check (db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::SpaceRelation, true, 0, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairsDelegate *notch_check (db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_single_polygon_check (db::SpaceRelation, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairsDelegate *enclosing_check (const Region &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::OverlapRelation, true, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairsDelegate *overlap_check (const Region &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::WidthRelation, true, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairsDelegate *separation_check (const Region &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::SpaceRelation, true, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairsDelegate *inside_check (const Region &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::InsideRelation, true, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  virtual EdgePairsDelegate *grid_check (db::Coord gx, db::Coord gy) const;
  virtual EdgePairsDelegate *angle_check (double min, double max, bool inverse) const;

  virtual RegionDelegate *snapped_in_place (db::Coord gx, db::Coord gy)
  {
    return snapped (gx, gy);
  }

  virtual RegionDelegate *snapped (db::Coord gx, db::Coord gy);

  virtual RegionDelegate *scaled_and_snapped_in_place (db::Coord gx, db::Coord mx, db::Coord dx, db::Coord gy, db::Coord my, db::Coord dy)
  {
    return scaled_and_snapped (gx, mx, dx, gy, my, dy);
  }

  virtual RegionDelegate *scaled_and_snapped (db::Coord gx, db::Coord mx, db::Coord dx, db::Coord gy, db::Coord my, db::Coord dy);

  virtual EdgesDelegate *edges (const EdgeFilterBase *) const;

  virtual RegionDelegate *process_in_place (const PolygonProcessorBase &filter)
  {
    return processed (filter);
  }

  virtual RegionDelegate *processed (const PolygonProcessorBase &filter) const;
  virtual EdgesDelegate *processed_to_edges (const PolygonToEdgeProcessorBase &filter) const;
  virtual EdgePairsDelegate *processed_to_edge_pairs (const PolygonToEdgePairProcessorBase &filter) const;

  virtual RegionDelegate *filter_in_place (const PolygonFilterBase &filter)
  {
    return filtered (filter);
  }

  virtual RegionDelegate *filtered (const PolygonFilterBase &filter) const;

  virtual RegionDelegate *merged_in_place ()
  {
    return merged ();
  }

  virtual RegionDelegate *merged_in_place (bool min_coherence, unsigned int min_wc)
  {
    return merged (min_coherence, min_wc);
  }

  virtual RegionDelegate *merged () const
  {
    return merged (min_coherence (), 0);
  }

  virtual RegionDelegate *merged (bool min_coherence, unsigned int min_wc) const;

  virtual RegionDelegate *sized (coord_type d, unsigned int mode) const;
  virtual RegionDelegate *sized (coord_type dx, coord_type dy, unsigned int mode) const;

  virtual RegionDelegate *and_with (const Region &other) const;
  virtual RegionDelegate *not_with (const Region &other) const;
  virtual RegionDelegate *xor_with (const Region &other) const;
  virtual RegionDelegate *or_with (const Region &other) const;

  virtual RegionDelegate *add_in_place (const Region &other)
  {
    return add (other);
  }

  virtual RegionDelegate *add (const Region &other) const;

  virtual RegionDelegate *selected_outside (const Region &other) const
  {
    return selected_interacting_generic (other, 1, false, false);
  }

  virtual RegionDelegate *selected_not_outside (const Region &other) const
  {
    return selected_interacting_generic (other, 1, false, true);
  }

  virtual RegionDelegate *selected_inside (const Region &other) const
  {
    return selected_interacting_generic (other, -1, true, false);
  }

  virtual RegionDelegate *selected_not_inside (const Region &other) const
  {
    return selected_interacting_generic (other, -1, true, true);
  }

  virtual RegionDelegate *selected_interacting (const Region &other) const
  {
    return selected_interacting_generic (other, 0, true, false);
  }

  virtual RegionDelegate *selected_not_interacting (const Region &other) const
  {
    return selected_interacting_generic (other, 0, true, true);
  }

  virtual RegionDelegate *selected_interacting (const Edges &other) const
  {
    return selected_interacting_generic (other, false);
  }

  virtual RegionDelegate *selected_not_interacting (const Edges &other) const
  {
    return selected_interacting_generic (other, true);
  }

  virtual RegionDelegate *selected_interacting (const Texts &other) const
  {
    return selected_interacting_generic (other, false);
  }

  virtual RegionDelegate *selected_not_interacting (const Texts &other) const
  {
    return selected_interacting_generic (other, true);
  }

  virtual RegionDelegate *selected_overlapping (const Region &other) const
  {
    return selected_interacting_generic (other, 0, false, false);
  }

  virtual RegionDelegate *selected_not_overlapping (const Region &other) const
  {
    return selected_interacting_generic (other, 0, false, true);
  }

  virtual RegionDelegate *pull_inside (const Region &other) const
  {
    return pull_generic (other, -1, true);
  }

  virtual RegionDelegate *pull_interacting (const Region &other) const
  {
    return pull_generic (other, 0, true);
  }

  virtual EdgesDelegate *pull_interacting (const Edges &other) const
  {
    return pull_generic (other);
  }

  virtual TextsDelegate *pull_interacting (const Texts &other) const
  {
    return pull_generic (other);
  }

  virtual RegionDelegate *pull_overlapping (const Region &other) const
  {
    return pull_generic (other, 0, false);
  }

  virtual RegionDelegate *in (const Region &other, bool invert) const;

  virtual bool equals (const Region &other) const;
  virtual bool less (const Region &other) const;

  virtual void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const;

protected:
  void update_bbox (const db::Box &box);
  void invalidate_bbox ();

  EdgePairsDelegate *run_check (db::edge_relation_type rel, bool different_polygons, const Region *other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const;
  EdgePairsDelegate *run_single_polygon_check (db::edge_relation_type rel, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const;
  virtual RegionDelegate *selected_interacting_generic (const Region &other, int mode, bool touching, bool inverse) const;
  virtual RegionDelegate *selected_interacting_generic (const Edges &other, bool inverse) const;
  virtual RegionDelegate *selected_interacting_generic (const Texts &other, bool inverse) const;
  virtual RegionDelegate *pull_generic (const Region &other, int mode, bool touching) const;
  virtual EdgesDelegate *pull_generic (const Edges &other) const;
  virtual TextsDelegate *pull_generic (const Texts &other) const;

  template <class Trans>
  static void produce_markers_for_grid_check (const db::Polygon &poly, const Trans &tr, db::Coord gx, db::Coord gy, db::Shapes &shapes);
  template <class Trans>
  static void produce_markers_for_angle_check (const db::Polygon &poly, const Trans &tr, double min, double max, bool inverse, db::Shapes &shapes);

  AsIfFlatRegion &operator= (const AsIfFlatRegion &other);
  AsIfFlatRegion (const AsIfFlatRegion &other);

private:

  mutable bool m_bbox_valid;
  mutable db::Box m_bbox;

  virtual db::Box compute_bbox () const;
  static RegionDelegate *region_from_box (const db::Box &b);
};

}

#endif

