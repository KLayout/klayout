
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

#include "tlObjectCollection.h"

namespace db
{

class Layout;

class DB_PUBLIC Triangles
{
public:
  Triangles ();
  ~Triangles ();

  void init_box (const db::DBox &box);
  std::string to_string ();

  db::DBox bbox () const;

  bool check (bool check_delaunay = true) const;
  void dump (const std::string &path) const;

  /**
   *  @brief Finds the points within (not "on") a circle of radius "radius" around the given vertex.
   */
  std::vector<db::Vertex *> find_points_around (Vertex *vertex, double radius);

  db::Vertex *insert_point (const db::DPoint &point, std::vector<db::Triangle *> *new_triangles = 0);
  db::Vertex *insert (db::Vertex *vertex, std::vector<db::Triangle *> *new_triangles = 0);
  void remove (db::Vertex *vertex, std::vector<db::Triangle *> *new_triangles = 0);

private:
  tl::shared_collection<db::Triangle> mp_triangles;
  tl::weak_collection<db::TriangleEdge> mp_edges;
  std::list<db::Vertex> m_vertex_heap;
  bool m_is_constrained;

  db::Vertex *create_vertex (double x, double y);
  db::Vertex *create_vertex (const db::DPoint &pt);
  db::TriangleEdge *create_edge (db::Vertex *v1, db::Vertex *v2);
  db::Triangle *create_triangle (db::TriangleEdge *e1, db::TriangleEdge *e2, db::TriangleEdge *e3);
  void remove (db::Triangle *tri);

  //  NOTE: these functions are SLOW and intended to test purposes only
  std::vector<db::Vertex *> find_touching (const db::DBox &box) const;
  std::vector<db::Vertex *> find_inside_circle (const db::DPoint &center, double radius) const;
  db::Layout *to_layout () const;

  void remove_outside_vertex (db::Vertex *vertex, std::vector<db::Triangle *> *new_triangles = 0);
  void remove_inside_vertex (db::Vertex *vertex, std::vector<db::Triangle *> *new_triangles_out = 0);
  std::vector<db::Triangle *> fill_concave_corners (const std::vector<TriangleEdge *> &edges);
  void fix_triangles (const std::vector<db::Triangle *> &tris, const std::vector<db::TriangleEdge *> &fixed_edges, std::vector<db::Triangle *> *new_triangles);
  std::pair<std::pair<db::Triangle *, db::Triangle *>, db::TriangleEdge *> flip (TriangleEdge *edge);
};

}

#endif

