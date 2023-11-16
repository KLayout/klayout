
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


#ifndef HDR_dbEdgePairRelations
#define HDR_dbEdgePairRelations

#include "dbEdge.h"
#include "dbEdgePair.h"

namespace db
{

/**
 *  @brief Definition of the metrics constants
 */
enum metrics_type
{
  /**
   *  @brief Euclidian metrics
   *
   *  The distance between two points is defined as the euclidian 
   *  distance, i.e. d = sqrt(dx * dx + dy * dy).
   *  All points within a circle with radius r around another point 
   *  have a distance less than r to this point.
   */
  Euclidian = 1, 

  /**
   *  @brief Square metrics
   *
   *  The distance between two points is the minimum of x and
   *  y distance, i.e. d = min(abs(dx), abs(dy)).
   *  All points within a square with length 2*r round another point
   *  have a distance less than r to this point.
   */
  Square = 2,

  /**
   *  @brief Projection metrics
   *
   *  The distance between a point and another point on an edge 
   *  is measured by the distance of the point to the edge.
   */
  Projection = 3
};

/**
 *  @brief An enum describing the relation of two edges
 */
enum edge_relation_type
{
  /**
   *  @brief Two edges form a width relation
   *
   *  The edges are oriented such that their inside sides face each other.
   */
  WidthRelation = 1,

  /**
   *  @brief Two edges form a space relation
   *
   *  The edges are oriented such that their outside sides face each other.
   */
  SpaceRelation = 2,

  /**
   *  @brief Two edges form an overlap relation
   *
   *  The first edge's inside side faces the second edge's outside side.
   */
  OverlapRelation = 3,

  /**
   *  @brief Two edges form an inside relation
   *
   *  The first edge's outside side faces the second edge's inside side.
   */
  InsideRelation = 4
};

/**
 *  @brief A filter based on the edge pair relation
 *
 *  This filter supports distance filtering (less than a certain value) plus 
 *  various selection criteria such as 
 */
struct DB_PUBLIC EdgeRelationFilter
{
  typedef db::Edge::distance_type distance_type;

  /**
   *  Constructs an edge relation filter
   *
   *  The metrics parameter specifies which metrics to use. "Euclidian", "Square" and "Projected"
   *  metrics are available.
   *
   *  ignore_angle allows specification of a maximum angle edges can form. 
   *  Corners with an angle larger or equal to this angle are not checked. 
   *  By choosing 90 degree, corners of 90 degree and larger are not checked,
   *  but acute corners are. Hence "opposing" edges are checked.
   *
   *  With min_projection and max_projection it is possible to specify how edges must be related 
   *  to each other. If the length of the projection of either edge on the other is >= min_projection
   *  or < max_projection, the edges are considered for the check.
   */
  EdgeRelationFilter (edge_relation_type r, distance_type d, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ());

  /** 
   *  @brief Tests whether two edges fulfil the check fail criterion 
   *
   *  If the output pointer is non-null, the object will receive the edge pair that
   *  represents the marker for this type of check.
   */
  bool check (const db::Edge &a, const db::Edge &b, db::EdgePair *output = 0) const;

  /**
   *  @brief Sets a flag indicating whether to report whole edges instead of partial ones
   */
  void set_whole_edges (bool f)
  {
    m_whole_edges = f;
  }

  /**
   *  @brief Gets a flag indicating whether to report whole edges instead of partial ones
   */
  bool whole_edges () const
  {
    return m_whole_edges;
  }

  /**
   *  @brief Sets a flag indicating whether zero distance shall be included in the check
   */
  void set_include_zero (bool f)
  {
    m_include_zero = f;
  }

  /**
   *  @brief Gets a flag indicating whether zero distance shall be included in the check
   */
  bool include_zero () const
  {
    return m_include_zero;
  }

  /**
   *  @brief Sets the metrics type
   */
  void set_metrics (metrics_type m)
  {
    m_metrics = m;
  }

  /**
   *  @brief Gets the metrics type
   */
  metrics_type metrics () const
  {
    return m_metrics;
  }

  /**
   *  @brief Sets the ignore corner angle parameter
   *
   *  This is the minimum angle connected edges must have so they are not ignored.
   */
  void set_ignore_angle (double a);

  /**
   *  @brief Gets the ignore corner angle
   */
  double ignore_angle () const
  {
    return m_ignore_angle;
  }

  /**
   *  @brief Sets the minimum projection parameter in database units
   */
  void set_min_projection (distance_type d)
  {
    m_min_projection = d;
  }

  /**
   *  @brief Gets the minimum projection parameter
   */
  distance_type min_projection () const
  {
    return m_min_projection;
  }

  /**
   *  @brief Sets the maximum projection parameter in database units
   */
  void set_max_projection (distance_type d)
  {
    m_max_projection = d;
  }

  /**
   *  @brief Gets the maximum projection parameter
   */
  distance_type max_projection () const
  {
    return m_max_projection;
  }

  /**
   *  @brief sets the check distance
   */
  void set_distance (distance_type d)
  {
    m_d = d;
  }

  /**
   *  @brief Gets the check distance
   */
  distance_type distance () const
  {
    return m_d;
  }

  /**
   *  @brief Sets the relation
   */
  void set_relation (edge_relation_type r)
  {
    m_r = r;
  }

  /**
   *  @brief Gets the relation
   */
  edge_relation_type relation () const
  {
    return m_r;
  }

private:
  bool m_whole_edges;
  bool m_include_zero;
  edge_relation_type m_r;
  distance_type m_d;
  metrics_type m_metrics;
  double m_ignore_angle, m_ignore_angle_cos;
  distance_type m_min_projection;
  distance_type m_max_projection;
};

//  Internal methods exposed for testing purposes

DB_PUBLIC bool projected_near_part_of_edge (bool include_zero, db::coord_traits<db::Coord>::distance_type d, const db::Edge &e, const db::Edge &g, db::Edge *output);
DB_PUBLIC bool square_near_part_of_edge (bool include_zero, db::coord_traits<db::Coord>::distance_type d, const db::Edge &e, const db::Edge &g, db::Edge *output);
DB_PUBLIC bool euclidian_near_part_of_edge (bool include_zero, db::coord_traits<db::Coord>::distance_type d, const db::Edge &e, const db::Edge &g, db::Edge *output);
DB_PUBLIC db::Edge::distance_type edge_projection (const db::Edge &a, const db::Edge &b);

}

#endif

