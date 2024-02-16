
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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
 *  @brief An enum specifying whether how edges with zero distance are handled in checks
 */
enum zero_distance_mode {

  /**
   *  @brief Never include zero-distance edges
   */
  NeverIncludeZeroDistance = 0,

  /**
   *  @brief Include zero-distance edges when they share at least one common point
   */
  IncludeZeroDistanceWhenTouching = 1,

  /**
   *  @brief Include zero-distance edges when they share at least one common point and are collinear
   */
  IncludeZeroDistanceWhenCollinearAndTouching = 2,

  /**
   *  @brief Include zero-distance edges when they share more than a single common point (this implies that they are collinear)
   */
  IncludeZeroDistanceWhenOverlapping = 3,

  /**
   *  @brief Always include zero-distance edges (hardly useful)
   */
  AlwaysIncludeZeroDistance = 4
};

/**
 *  @brief A structure holding the options for the region checks (space, width, ...)
 */
struct DB_PUBLIC EdgesCheckOptions
{
  typedef db::coord_traits<db::Coord>::distance_type distance_type;

  /**
   *  @brief Constructor
   */
  EdgesCheckOptions (bool _whole_edges = false,
                      metrics_type _metrics = db::Euclidian,
                      double _ignore_angle = 90,
                      distance_type _min_projection = 0,
                      distance_type _max_projection = std::numeric_limits<distance_type>::max (),
                      zero_distance_mode _zd_mode = IncludeZeroDistanceWhenTouching)
    : whole_edges (_whole_edges),
      metrics (_metrics),
      ignore_angle (_ignore_angle),
      min_projection (_min_projection),
      max_projection (_max_projection),
      zd_mode (_zd_mode)
  { }

  /**
   *  @brief Specifies is whole edges are to be delivered
   *
   *  Without "whole_edges", the parts of
   *  the edges are returned which violate the condition. If "whole_edges" is true, the
   *  result will contain the complete edges participating in the result.
   */
  bool whole_edges;

  /**
   *  @brief Measurement metrics
   *
   *  The metrics parameter specifies which metrics to use. "Euclidian", "Square" and "Projected"
   *  metrics are available.
   */
  metrics_type metrics;

  /**
   *  @brief Specifies the obtuse angle threshold
   *
   *  "ignore_angle" allows specification of a maximum angle that connected edges can have to not participate
   *  in the check. By choosing 90 degree, edges with angles of 90 degree and larger are not checked,
   *  but acute corners are for example.
   */
  double ignore_angle;

  /**
   *  @brief Specifies the projection limit's minimum value
   *
   *  With min_projection and max_projection it is possible to specify how edges must be related
   *  to each other. If the length of the projection of either edge on the other is >= min_projection
   *  or < max_projection, the edges are considered for the check.
   */
  distance_type min_projection;

  /**
   *  @brief Specifies the projection limit's maximum value
   */
  distance_type max_projection;

  /**
   *  @brief Specifies zero-distance edge handling
   *
   *  This allows implementing the "kissing corners" case. When set to "IncludeZeroDistanceWhenTouching", kissing corners will
   *  be reported as errors, when set to "NeverIncludeZeroDistance", they won't. Note that with merged inputs, edges
   *  will not overlap except at the corners.
   */
  zero_distance_mode zd_mode;
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
  EdgeRelationFilter (edge_relation_type r, distance_type d, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max (), zero_distance_mode include_zero = AlwaysIncludeZeroDistance);

  /**
   *  Constructs an edge relation filter from a CheckOptions structure
   */
  EdgeRelationFilter (edge_relation_type r, distance_type d, const EdgesCheckOptions &options);

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
   *  @brief Sets a value indicating whether zero-distance edges shall be included in the check
   */
  void set_zero_distance_mode (zero_distance_mode f)
  {
    m_zero_distance_mode = f;
  }

  /**
   *  @brief Gets a value indicating whether zero-distance edges shall be included in the check
   */
  zero_distance_mode get_zero_distance_mode () const
  {
    return m_zero_distance_mode;
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
  zero_distance_mode m_zero_distance_mode;
  edge_relation_type m_r;
  distance_type m_d;
  metrics_type m_metrics;
  double m_ignore_angle, m_ignore_angle_cos;
  distance_type m_min_projection;
  distance_type m_max_projection;
};

//  Internal methods exposed for testing purposes

DB_PUBLIC bool projected_near_part_of_edge (zero_distance_mode include_zero, db::coord_traits<db::Coord>::distance_type d, const db::Edge &e, const db::Edge &g, db::Edge *output);
DB_PUBLIC bool square_near_part_of_edge (zero_distance_mode include_zero, db::coord_traits<db::Coord>::distance_type d, const db::Edge &e, const db::Edge &g, db::Edge *output);
DB_PUBLIC bool euclidian_near_part_of_edge (zero_distance_mode include_zero, db::coord_traits<db::Coord>::distance_type d, const db::Edge &e, const db::Edge &g, db::Edge *output);
DB_PUBLIC db::Edge::distance_type edge_projection (const db::Edge &a, const db::Edge &b);

}

#endif

