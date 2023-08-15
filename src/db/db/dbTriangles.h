
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



#ifndef HDR_dbTriangles
#define HDR_dbTriangles

#include "dbCommon.h"
#include "dbTriangle.h"
#include "dbBox.h"
#include "dbRegion.h"

#include "tlObjectCollection.h"

namespace db
{

class Layout;

class DB_PUBLIC Triangles
{
public:
  struct TriangulateParameters
  {
    TriangulateParameters ()
      : b (1.0),
        max_area (0.0),
        max_area_border (0.0)
    { }

    /**
     *  @brief Min. readius-to-shortest edge ratio
     */
    double b;

    /**
     *  @brief Max area or zero for "no constraint"
     */
    double max_area;

    /**
     *  @brief Max area for border triangles or zero for "use max_area"
     */
    double max_area_border;
  };

  typedef tl::shared_collection<db::Triangle> triangles_type;
  typedef triangles_type::const_iterator triangle_iterator;

  Triangles ();
  ~Triangles ();

  /**
   *  @brief Initializes the triangle collection with a box
   *  Two triangles will be created.
   */
  void init_box (const db::DBox &box);

  /**
   *  @brief Returns a string representation of the triangle graph.
   */
  std::string to_string ();

  /**
   *  @brief Returns the bounding box of the triangle graph.
   */
  db::DBox bbox () const;

  /**
   *  @brief Iterates the triangles in the graph (begin iterator)
   */
  triangle_iterator begin () const { return mp_triangles.begin (); }

  /**
   *  @brief Iterates the triangles in the graph (end iterator)
   */
  triangle_iterator end () const { return mp_triangles.end (); }

  /**
   *  @brief Returns the number of triangles in the graph
   */
  size_t num_triangles () const { return mp_triangles.size (); }

  /**
   *  @brief Clears the triangle set
   */
  void clear ();

  /**
   *  @brief Creates a refined Delaunay triangulation for the given region
   */
  void triangulate (const db::Region &region, const TriangulateParameters &parameters, double dbu = 1.0);


  //  -- exposed for testing purposes --

  /**
   *  @brief Checks the triangle graph for consistency
   *  This method is for testing purposes mainly.
   */
  bool check (bool check_delaunay = true) const;

  /**
   *  @brief Dumps the triangle graph to a GDS file at the given path
   *  This method is for testing purposes mainly.
   */
  void dump (const std::string &path) const;

  /**
   *  @brief Creates a new layout object representing the triangle graph
   *  This method is for testing purposes mainly.
   */
  db::Layout *to_layout () const;

  /**
   *  @brief Finds the points within (not "on") a circle of radius "radius" around the given vertex.
   */
  std::vector<db::Vertex *> find_points_around (Vertex *vertex, double radius);

  /**
   *  @brief Inserts a new vertex as the given point
   *
   *  If "new_triangles" is not null, it will receive the list of new triangles created during
   *  the remove step.
   */
  db::Vertex *insert_point (const db::DPoint &point, std::list<tl::weak_ptr<db::Triangle> > *new_triangles = 0);

  /**
   *  @brief Inserts a new vertex as the given point
   *
   *  If "new_triangles" is not null, it will receive the list of new triangles created during
   *  the remove step.
   */
  db::Vertex *insert_point (db::DCoord x, db::DCoord y, std::list<tl::weak_ptr<db::Triangle> > *new_triangles = 0);

  /**
   *  @brief Removes the given vertex
   *
   *  If "new_triangles" is not null, it will receive the list of new triangles created during
   *  the remove step.
   */
  void remove (db::Vertex *vertex, std::list<tl::weak_ptr<db::Triangle> > *new_triangles = 0);

  /**
   *  @brief Flips the given edge
   */
  std::pair<std::pair<db::Triangle *, db::Triangle *>, db::TriangleEdge *> flip (TriangleEdge *edge);

  /**
   *  @brief Finds all edges that cross the given one for a convex triangulation
   *
   *  Requirements:
   *  * self must be a convex triangulation
   *  * edge must not contain another vertex from the triangulation except p1 and p2
   */
  std::vector<db::TriangleEdge *> search_edges_crossing (db::Vertex *from, db::Vertex *to);

  /**
   *  @brief Finds the edge for two given points
   */
  db::TriangleEdge *find_edge_for_points (const db::DPoint &p1, const db::DPoint &p2);

  /**
   *  @brief Finds the vertex for a point
   */
  db::Vertex *find_vertex_for_point (const db::DPoint &pt);

  /**
   *  @brief Ensures all points between from an to are connected by edges and makes these segments
   */
  std::vector<db::TriangleEdge *> ensure_edge (db::Vertex *from, db::Vertex *to);

  /**
   *  @brief Given a set of contours with edges, mark outer triangles
   *
   *  The edges must be made from existing vertexes. Edge orientation is
   *  clockwise.
   *
   *  This will also mark triangles as outside ones.
   */
  void constrain (const std::vector<std::vector<Vertex *> > &contours);

  /**
   *  @brief Removes the outside triangles.
   */
  void remove_outside_triangles ();

  /**
   *  @brief Creates a constrained Delaunay triangulation from the given Region
   */
  void create_constrained_delaunay (const db::Region &region, double dbu = 1.0);

  /**
   *  @brief Returns a value indicating whether the edge is "illegal" (violates the Delaunay criterion)
   */
  static bool is_illegal_edge (db::TriangleEdge *edge);

  //  NOTE: these functions are SLOW and intended to test purposes only
  std::vector<db::Vertex *> find_touching (const db::DBox &box) const;
  std::vector<db::Vertex *> find_inside_circle (const db::DPoint &center, double radius) const;

private:
  tl::shared_collection<db::Triangle> mp_triangles;
  tl::weak_collection<db::TriangleEdge> mp_edges;
  std::list<db::Vertex> m_vertex_heap;
  bool m_is_constrained;
  size_t m_level;
  size_t m_id;

  db::Vertex *create_vertex (double x, double y);
  db::Vertex *create_vertex (const db::DPoint &pt);
  db::TriangleEdge *create_edge (db::Vertex *v1, db::Vertex *v2);
  db::Triangle *create_triangle (db::TriangleEdge *e1, db::TriangleEdge *e2, db::TriangleEdge *e3);
  void remove (db::Triangle *tri);

  void remove_outside_vertex (db::Vertex *vertex, std::list<tl::weak_ptr<db::Triangle> > *new_triangles = 0);
  void remove_inside_vertex (db::Vertex *vertex, std::list<tl::weak_ptr<db::Triangle> > *new_triangles_out = 0);
  std::vector<db::Triangle *> fill_concave_corners (const std::vector<TriangleEdge *> &edges);
  int fix_triangles(const std::vector<db::Triangle *> &tris, const std::vector<db::TriangleEdge *> &fixed_edges, std::list<tl::weak_ptr<db::Triangle> > *new_triangles);
  std::vector<db::Triangle *> find_triangle_for_point (const db::DPoint &point);
  db::TriangleEdge *find_closest_edge (const db::DPoint &p, db::Vertex *vstart = 0, bool inside_only = false);
  db::Vertex *insert (db::Vertex *vertex, std::list<tl::weak_ptr<db::Triangle> > *new_triangles = 0);
  void split_triangle (db::Triangle *t, db::Vertex *vertex, std::list<tl::weak_ptr<db::Triangle> > *new_triangles_out);
  void split_triangles_on_edge (const std::vector<db::Triangle *> &tris, db::Vertex *vertex, db::TriangleEdge *split_edge, std::list<tl::weak_ptr<db::Triangle> > *new_triangles_out);
  void add_more_triangles (std::vector<Triangle *> &new_triangles,
                                 db::TriangleEdge *incoming_edge,
                                 db::Vertex *from_vertex, db::Vertex *to_vertex,
                                 db::TriangleEdge *conn_edge);
  void insert_new_vertex(db::Vertex *vertex, std::list<tl::weak_ptr<Triangle> > *new_triangles_out);
  std::vector<db::TriangleEdge *> ensure_edge_inner (db::Vertex *from, db::Vertex *to);
  void join_edges (std::vector<TriangleEdge *> &edges);
};

}

#endif

