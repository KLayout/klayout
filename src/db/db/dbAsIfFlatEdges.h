
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


#ifndef HDR_dbAsIfFlatEdges
#define HDR_dbAsIfFlatEdges

#include "dbCommon.h"
#include "dbBoxScanner.h"
#include "dbEdgesDelegate.h"

#include <map>
#include <vector>

namespace db {

class PolygonSink;

/**
 *  @brief A helper class to turn joined edge sequences into polygons
 *
 *  This object is an edge cluster so it can connect to a cluster collector
 *  driven by a box scanner.
 */
struct JoinEdgesCluster
  : public db::cluster<db::Edge, size_t>
{
  typedef db::Edge::coord_type coord_type;

  JoinEdgesCluster (db::PolygonSink *output, coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i);
  void finish ();

private:
  db::PolygonSink *mp_output;
  coord_type m_ext_b, m_ext_e, m_ext_o, m_ext_i;
};

/**
 *  @brief Provides default flat implementations
 */
class DB_PUBLIC AsIfFlatEdges
  : public EdgesDelegate
{
public:
  AsIfFlatEdges ();
  virtual ~AsIfFlatEdges ();

  virtual size_t size () const;
  virtual std::string to_string (size_t) const;
  virtual distance_type length (const db::Box &) const;
  virtual Box bbox () const;

  virtual EdgePairs width_check (db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::WidthRelation, 0, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }
    
  virtual EdgePairs space_check (db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::SpaceRelation, 0, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  virtual EdgePairs enclosing_check (const Edges &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::OverlapRelation, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  virtual EdgePairs overlap_check (const Edges &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::WidthRelation, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  virtual EdgePairs separation_check (const Edges &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::SpaceRelation, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  virtual EdgePairs inside_check (const Edges &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::InsideRelation, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  virtual EdgesDelegate *filter_in_place (const EdgeFilterBase &filter)
  {
    return filtered (filter);
  }

  virtual EdgesDelegate *filtered (const EdgeFilterBase &) const;

  virtual EdgesDelegate *merged_in_place ()
  {
    return merged ();
  }

  virtual EdgesDelegate *merged () const
  {
    return boolean (0, EdgeOr);
  }

  virtual EdgesDelegate *and_with (const Edges &other) const
  {
    return boolean (&other, EdgeAnd);
  }

  virtual EdgesDelegate *and_with (const Region &other) const
  {
    return edge_region_op (other, false /*inside*/, true /*include borders*/);
  }

  virtual EdgesDelegate *not_with (const Edges &other) const
  {
    return boolean (&other, EdgeNot);
  }

  virtual EdgesDelegate *not_with (const Region &other) const
  {
    return edge_region_op (other, true /*outside*/, true /*include borders*/);
  }

  virtual EdgesDelegate *xor_with (const Edges &other) const
  {
    return boolean (&other, EdgeXor);
  }

  virtual EdgesDelegate *or_with (const Edges &other) const
  {
    return boolean (&other, EdgeOr);
  }

  virtual EdgesDelegate *add_in_place (const Edges &other)
  {
    return add (other);
  }

  virtual EdgesDelegate *add (const Edges &other) const;

  virtual EdgesDelegate *inside_part (const Region &other) const
  {
    return edge_region_op (other, false /*inside*/, false /*don't include borders*/);
  }

  virtual EdgesDelegate *outside_part (const Region &other) const
  {
    return edge_region_op (other, true /*outside*/, false /*don't include borders*/);
  }

  virtual RegionDelegate *extended (coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i, bool join) const;
  virtual EdgesDelegate *start_segments (length_type length, double fraction) const;
  virtual EdgesDelegate *end_segments (length_type length, double fraction) const;
  virtual EdgesDelegate *centers (length_type length, double fraction) const;

  virtual EdgesDelegate *selected_interacting (const Edges &) const;
  virtual EdgesDelegate *selected_not_interacting (const Edges &) const;
  virtual EdgesDelegate *selected_interacting (const Region &) const;
  virtual EdgesDelegate *selected_not_interacting (const Region &) const;

  virtual EdgesDelegate *in (const Edges &, bool) const;

  virtual bool equals (const Edges &other) const;
  virtual bool less (const Edges &other) const;

  virtual void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const;

protected:
  void update_bbox (const db::Box &box);
  void invalidate_bbox ();
  EdgePairs run_check (db::edge_relation_type rel, const Edges *other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const;
  static db::Polygon extended_edge (const db::Edge &edge, coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i);
  static db::Edge compute_partial (const db::Edge &edge, int mode, length_type length, double fraction);

private:
  AsIfFlatEdges &operator= (const AsIfFlatEdges &other);

  mutable bool m_bbox_valid;
  mutable db::Box m_bbox;

  virtual db::Box compute_bbox () const;
  EdgesDelegate *boolean (const Edges *other, EdgeBoolOp op) const;
  EdgesDelegate *edge_region_op (const Region &other, bool outside, bool include_borders) const;
  EdgesDelegate *segments (int mode, length_type length, double fraction) const;
};

}

#endif

