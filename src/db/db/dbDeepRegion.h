
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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

#include "dbAsIfFlatRegion.h"
#include "dbDeepShapeStore.h"

namespace db {

/**
 *  @brief A deep, polygon-set delegate
 */
class DB_PUBLIC DeepRegion
  : public AsIfFlatRegion
{
public:
  typedef db::layer<db::Polygon, db::unstable_layer_tag> polygon_layer_type;
  typedef polygon_layer_type::iterator polygon_iterator_type;

  DeepRegion ();
  DeepRegion (const RecursiveShapeIterator &si, DeepShapeStore &dss, double area_ratio = 0.0, size_t max_vertex_count = 0);
  DeepRegion (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans, bool merged_semantics = true, double area_ratio = 0.0, size_t max_vertex_count = 0);

  DeepRegion (const DeepRegion &other);
  DeepRegion (const DeepLayer &dl);

  virtual ~DeepRegion ();

  RegionDelegate *clone () const;

  virtual RegionIteratorDelegate *begin () const;
  virtual RegionIteratorDelegate *begin_merged () const;

  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const;
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_merged_iter () const;

  virtual bool empty () const;

  virtual bool is_merged () const;

  virtual const db::Polygon *nth (size_t n) const;
  virtual bool has_valid_polygons () const;
  virtual bool has_valid_merged_polygons () const;

  virtual const db::RecursiveShapeIterator *iter () const;

  virtual bool equals (const Region &other) const;
  virtual bool less (const Region &other) const;

  virtual RegionDelegate *and_with (const Region &other) const;
  virtual RegionDelegate *not_with (const Region &other) const;
  virtual RegionDelegate *xor_with (const Region &other) const;

  virtual RegionDelegate *add_in_place (const Region &other);
  virtual RegionDelegate *add (const Region &other) const;

  virtual bool is_box () const;
  virtual size_t size () const;

  virtual area_type area (const db::Box &box) const;
  virtual perimeter_type perimeter (const db::Box &box) const;
  virtual Box bbox () const;

  virtual std::string to_string (size_t nmax) const;

  EdgePairs width_check (db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_single_polygon_check (db::WidthRelation, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairs space_check (db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::SpaceRelation, false, 0, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairs isolated_check (db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::SpaceRelation, true, 0, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairs notch_check (db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_single_polygon_check (db::SpaceRelation, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairs enclosing_check (const Region &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::OverlapRelation, true, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairs overlap_check (const Region &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::WidthRelation, true, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairs separation_check (const Region &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::SpaceRelation, true, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairs inside_check (const Region &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::InsideRelation, true, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  virtual EdgePairs grid_check (db::Coord gx, db::Coord gy) const;
  virtual EdgePairs angle_check (double min, double max, bool inverse) const;

  virtual RegionDelegate *snapped_in_place (db::Coord gx, db::Coord gy)
  {
    return snapped (gx, gy);
  }

  virtual RegionDelegate *snapped (db::Coord gx, db::Coord gy);

  virtual Edges edges (const EdgeFilterBase *) const;

  virtual RegionDelegate *filter_in_place (const PolygonFilterBase &filter)
  {
    return filtered (filter);
  }

  virtual RegionDelegate *filtered (const PolygonFilterBase &filter) const;

  virtual RegionDelegate *merged_in_place ();
  virtual RegionDelegate *merged_in_place (bool min_coherence, unsigned int min_wc);

  virtual RegionDelegate *merged () const;
  virtual RegionDelegate *merged (bool min_coherence, unsigned int min_wc) const;

  virtual RegionDelegate *strange_polygon_check () const;

  virtual RegionDelegate *sized (coord_type d, unsigned int mode) const;
  virtual RegionDelegate *sized (coord_type dx, coord_type dy, unsigned int mode) const;

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

  virtual RegionDelegate *selected_overlapping (const Region &other) const
  {
    return selected_interacting_generic (other, 0, false, false);
  }

  virtual RegionDelegate *selected_not_overlapping (const Region &other) const
  {
    return selected_interacting_generic (other, 0, false, true);
  }

  virtual RegionDelegate *holes () const;
  virtual RegionDelegate *hulls () const;
  virtual RegionDelegate *in (const Region &other, bool invert) const;
  virtual RegionDelegate *rounded_corners (double rinner, double router, unsigned int n) const;
  virtual RegionDelegate *smoothed (coord_type d) const;

  virtual void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const;

  const DeepLayer &deep_layer () const
  {
    return m_deep_layer;
  }

  DeepLayer &deep_layer ()
  {
    return m_deep_layer;
  }

protected:
  virtual void merged_semantics_changed ();

private:
  DeepRegion &operator= (const DeepRegion &other);

  DeepLayer m_deep_layer;
  mutable DeepLayer m_merged_polygons;
  mutable bool m_merged_polygons_valid;

  void init ();
  void ensure_merged_polygons_valid () const;
  DeepLayer and_or_not_with(const DeepRegion *other, bool and_op) const;
  void add_from (const DeepLayer &dl);
  EdgePairs run_check (db::edge_relation_type rel, bool different_polygons, const Region *other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const;
  EdgePairs run_single_polygon_check (db::edge_relation_type rel, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const;
  RegionDelegate *selected_interacting_generic (const Region &other, int mode, bool touching, bool inverse) const;
  RegionDelegate *selected_interacting_generic (const Edges &other, bool inverse) const;
};

}

#endif

