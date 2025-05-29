
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

#ifndef HDR_dbPLCTriangulation
#define HDR_dbPLCTriangulation

#include "dbCommon.h"
#include "dbPLC.h"

#include <limits>
#include <list>
#include <vector>
#include <algorithm>

namespace db
{

namespace plc
{

struct DB_PUBLIC TriangulationParameters
{
  TriangulationParameters ()
    : min_b (1.0),
      min_length (0.0),
      max_area (0.0),
      max_area_border (0.0),
      max_iterations (std::numeric_limits<size_t>::max ()),
      base_verbosity (30),
      mark_triangles (false),
      remove_outside_triangles (true)
  { }

  /**
   *  @brief Min. readius-to-shortest edge ratio
   */
  double min_b;

  /**
   *  @brief Min. edge length
   *
   *  This parameter does not provide a guarantee about a minimume edge length, but
   *  helps avoiding ever-reducing triangle splits in acute corners of the input polygon.
   *  Splitting of edges stops when the edge is less than the min length.
   */
  double min_length;

  /**
   *  @brief Max area or zero for "no constraint"
   */
  double max_area;

  /**
   *  @brief Max area for border triangles or zero for "use max_area"
   */
  double max_area_border;

  /**
   *  @brief Max number of iterations
   */
  size_t max_iterations;

  /**
   *  @brief The verbosity level above which triangulation reports details
   */
  int base_verbosity;

  /**
   *  @brief If true, final triangles are marked using the "id" integer as a bit field
   *
   *  This provides information about the result quality.
   *
   *  Bit 0: skinny triangle
   *  Bit 1: bad-quality (skinny or area too large)
   *  Bit 2: non-Delaunay (in the strict sense)
   */
  bool mark_triangles;

  /**
   *  @brief If false, the outside triangles are not removed after triangulation
   */
  bool remove_outside_triangles;
};

/**
 *  @brief A Triangulation algorithm
 *
 *  This class implements a constrained refined Delaunay triangulation using Chew's algorithm.
 */
class DB_PUBLIC Triangulation
{
public:
  /**
   *  @brief The constructor
   *
   *  The graph will be one filled by the triangulation.
   */
  Triangulation (Graph *graph);

  /**
   *  @brief Clears the triangulation
   */
  void clear ();

  /**
   *  @brief Initializes the triangle collection with a box
   *  Two triangles will be created.
   */
  void init_box (const db::DBox &box);

  /**
   *  @brief Creates a refined Delaunay triangulation for the given region
   *
   *  The database unit should be chosen in a way that target area values are "in the order of 1".
   *  For inputs featuring acute angles (angles < ~25 degree), the parameters should defined a min
   *  edge length ("min_length").
   *  "min_length" should be at least 1e-4. If a min edge length is given, the max area constaints
   *  may not be satisfied.
   *
   *  Edges in the input should not be shorter than 1e-4.
   */
  void triangulate (const db::Region &region, const TriangulationParameters &parameters, double dbu = 1.0);

  //  more versions
  void triangulate (const db::Region &region, const TriangulationParameters &parameters, const db::CplxTrans &trans = db::CplxTrans ());
  void triangulate (const db::Region &region, const std::vector<db::Point> &vertexes, const TriangulationParameters &parameters, const db::CplxTrans &trans = db::CplxTrans ());
  void triangulate (const db::Polygon &poly, const TriangulationParameters &parameters, double dbu = 1.0);
  void triangulate (const db::Polygon &poly, const std::vector<db::Point> &vertexes, const TriangulationParameters &parameters, double dbu = 1.0);
  void triangulate (const db::Polygon &poly, const TriangulationParameters &parameters, const db::CplxTrans &trans = db::CplxTrans ());
  void triangulate (const db::Polygon &poly, const std::vector<db::Point> &vertexes, const TriangulationParameters &parameters, const db::CplxTrans &trans = db::CplxTrans ());

  /**
   *  @brief Triangulates a floating-point polygon
   */
  void triangulate (const db::DPolygon &poly, const TriangulationParameters &parameters, const db::DCplxTrans &trans = db::DCplxTrans ());
  void triangulate (const db::DPolygon &poly, const std::vector<db::DPoint> &vertexes, const TriangulationParameters &parameters, const db::DCplxTrans &trans = db::DCplxTrans ());

  /**
   *  @brief Inserts a new vertex as the given point
   *
   *  If "new_triangles" is not null, it will receive the list of new triangles created during
   *  the remove step.
   *
   *  This method can be called after "triangulate" to add new points and adjust the triangulation.
   *  Inserting new points will maintain the (constrained) Delaunay condition.
   */
  Vertex *insert_point (const db::DPoint &point, std::list<tl::weak_ptr<Polygon> > *new_triangles = 0);

  /**
   *  @brief Finds the edge for two given points
   */
  Edge *find_edge_for_points (const db::DPoint &p1, const db::DPoint &p2) const;

  /**
   *  @brief Finds the vertex for a point
   */
  Vertex *find_vertex_for_point (const db::DPoint &pt) const;

  /**
   *  @brief Finds the vertexes along the line given from p1 and p2
   *
   *  At least one of the points p1 and p2 must be existing vertexes.
   */
  std::vector<Vertex *> find_vertexes_along_line (const db::DPoint &p1, const db::DPoint &p2) const;

  /**
   *  @brief Removes the outside triangles.
   *
   *  This method is useful in combination with the "remove_outside_triangles = false" triangulation
   *  parameter. In this mode, outside triangles are not removed after triangulation (the
   *  triangulated area is convex). This enables use of the "find" functions.
   *
   *  This method can be used to finally remove the outside triangles if no longer needed.
   */
  void remove_outside_triangles ();

  /**
   *  @brief Statistics: number of flips (fixing)
   */
  size_t flips () const
  {
    return m_flips;
  }

  /**
   *  @brief Statistics: number of hops (searching)
   */
  size_t hops () const
  {
    return m_hops;
  }

  /**
   *  @brief Creates a constrained Delaunay triangulation from the given Region
   *
   *  This method is used internally by the "triangulation" method to create the basic triangulation,
   *  followed by a "refine" step.
   */
  void create_constrained_delaunay (const db::Region &region, const db::CplxTrans &trans = db::CplxTrans ());

  /**
   *  @brief Creates a constrained Delaunay triangulation from the given Polygon
   *
   *  This method is used internally by the "triangulation" method to create the basic triangulation,
   *  followed by a "refine" step.
   */
  void create_constrained_delaunay (const db::Polygon &poly, const db::CplxTrans &trans = db::CplxTrans ());

  /**
   *  @brief Creates a constrained Delaunay triangulation from the given DPolygon
   *
   *  This method is used internally by the "triangulation" method to create the basic triangulation,
   *  followed by a "refine" step.
   */
  void create_constrained_delaunay (const db::DPolygon &poly, const DCplxTrans &trans = db::DCplxTrans ());

  /**
   *  @brief Refines the triangulation using the given parameters
   *
   *  This method is used internally by the "triangulate" method after creating the basic triangulation.
   *
   *  This method is provided as a partial solution of a triangulation for special cases.
   */
  void refine (const TriangulationParameters &param);

  /**
   *  @brief Given a set of contours with edges, mark outer triangles
   *
   *  The edges must be made from existing vertexes. Edge orientation is
   *  clockwise.
   *
   *  This will also mark triangles as outside ones.
   *  This method is used internally by the "triangulate" method after creating the basic triangulation.
   *
   *  This method is provided as a partial solution of a triangulation for special cases.
   */
  void constrain (const std::vector<std::vector<Vertex *> > &contours);

  /**
   *  @brief Inserts a contours of a polygon
   *
   *  This method fills the contours of the given polygon by doint an "insert_point"
   *  on all points and logging the outer edges ("segments") into the "contours"
   *  array. The latter can be passed to "constrain" to create a constrained
   *  triangulation.
   *
   *  This method is used internally by the "triangulate" method to create the basic triangulation.
   *  This method is provided as a partial solution of a triangulation for special cases.
   */
  template<class Poly, class Trans> void make_contours (const Poly &poly, const Trans &trans, std::vector<std::vector<Vertex *> > &contours);

protected:
  /**
   *  @brief Checks the polygon graph for consistency
   *  This method is for testing purposes mainly.
   */
  bool check (bool check_delaunay = true) const;

  /**
   *  @brief Finds the points within (not "on") a circle of radius "radius" around the given vertex.
   */
  std::vector<Vertex *> find_points_around (Vertex *vertex, double radius);

  /**
   *  @brief Inserts a new vertex as the given point
   *
   *  If "new_triangles" is not null, it will receive the list of new triangles created during
   *  the remove step.
   */
  Vertex *insert_point (db::DCoord x, db::DCoord y, std::list<tl::weak_ptr<Polygon> > *new_triangles = 0);

  /**
   *  @brief Removes the given vertex
   *
   *  If "new_triangles" is not null, it will receive the list of new triangles created during
   *  the remove step.
   */
  void remove (Vertex *vertex, std::list<tl::weak_ptr<Polygon> > *new_triangles = 0);

  /**
   *  @brief Flips the given edge
   */
  std::pair<std::pair<Polygon *, Polygon *>, Edge *> flip (Edge *edge);

  /**
   *  @brief Finds all edges that cross the given one for a convex triangulation
   *
   *  Requirements:
   *  * self must be a convex triangulation
   *  * edge must not contain another vertex from the triangulation except p1 and p2
   */
  std::vector<Edge *> search_edges_crossing (Vertex *from, Vertex *to);

  /**
   *  @brief Ensures all points between from an to are connected by edges and makes these segments
   */
  std::vector<Edge *> ensure_edge (Vertex *from, Vertex *to);

  /**
   *  @brief Returns a value indicating whether the edge is "illegal" (violates the Delaunay criterion)
   */
  static bool is_illegal_edge (Edge *edge);

  //  NOTE: these functions are SLOW and intended to test purposes only
  std::vector<Vertex *> find_touching (const db::DBox &box) const;
  std::vector<Vertex *> find_inside_circle (const db::DPoint &center, double radius) const;

private:
  Graph *mp_graph;
  bool m_is_constrained;
  size_t m_level;
  size_t m_id;
  mutable size_t m_flips, m_hops;

  void remove_outside_vertex (Vertex *vertex, std::list<tl::weak_ptr<Polygon> > *new_triangles = 0);
  void remove_inside_vertex (Vertex *vertex, std::list<tl::weak_ptr<Polygon> > *new_triangles_out = 0);
  std::vector<Polygon *> fill_concave_corners (const std::vector<Edge *> &edges);
  void fix_triangles (const std::vector<Polygon *> &tris, const std::vector<Edge *> &fixed_edges, std::list<tl::weak_ptr<Polygon> > *new_triangles);
  std::vector<Polygon *> find_triangle_for_point (const db::DPoint &point);
  Edge *find_closest_edge (const db::DPoint &p, Vertex *vstart = 0, bool inside_only = false) const;
  Vertex *insert (Vertex *vertex, std::list<tl::weak_ptr<Polygon> > *new_triangles = 0);
  void split_triangle (Polygon *t, Vertex *vertex, std::list<tl::weak_ptr<Polygon> > *new_triangles_out);
  void split_triangles_on_edge (Vertex *vertex, Edge *split_edge, std::list<tl::weak_ptr<Polygon> > *new_triangles_out);
  void add_more_triangles (std::vector<Polygon *> &new_triangles,
                                 Edge *incoming_edge,
                                 Vertex *from_vertex, Vertex *to_vertex,
                                 Edge *conn_edge);
  void insert_new_vertex(Vertex *vertex, std::list<tl::weak_ptr<Polygon> > *new_triangles_out);
  std::vector<Edge *> ensure_edge_inner (Vertex *from, Vertex *to);
  void join_edges (std::vector<Edge *> &edges);
};

} //  namespace plc

} //  namespace db

#endif

