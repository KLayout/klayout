
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


#include "dbTriangles.h"

namespace db
{

Triangles::Triangles ()
  : m_is_constrained (false)
{
  //  .. nothing yet ..
}

Triangles::~Triangles ()
{
  while (! mp_triangles.empty ()) {
    remove (mp_triangles.front ());
  }

  tl_assert (mp_edges.empty ());
}

db::Vertex *
Triangles::create_vertex (double x, double y)
{
  m_vertex_heap.push_back (db::Vertex (x, y));
  return &m_vertex_heap.back ();
}

db::Vertex *
Triangles::create_vertex (const db::DPoint &pt)
{
  m_vertex_heap.push_back (pt);
  return &m_vertex_heap.back ();
}

db::TriangleEdge *
Triangles::create_edge (db::Vertex *v1, db::Vertex *v2)
{
  db::TriangleEdge *res = new db::TriangleEdge (v1, v2);
  mp_edges.push_back (res);
  return res;
}

db::Triangle *
Triangles::create_triangle (TriangleEdge *e1, TriangleEdge *e2, TriangleEdge *e3)
{
  db::Triangle *res = new db::Triangle (e1, e2, e3);
  mp_triangles.push_back (res);
  return res;
}

void
Triangles::remove (db::Triangle *tri)
{
  db::TriangleEdge *edges [3];
  for (int i = 0; i < 3; ++i) {
    edges [i] = tri->edge (i);
  }

  delete tri;

  //  clean up edges we do no longer need
  for (int i = 0; i < 3; ++i) {
    if (edges [i]->left () == 0 && edges [i]->right () == 0) {
      delete edges [i];
    }
  }
}

void
Triangles::init_box (const db::DBox &box)
{
  double xmin = box.left (), xmax = box.right ();
  double ymin = box.bottom (), ymax = box.top ();

  db::Vertex *vbl = create_vertex (xmin, ymin);
  db::Vertex *vtl = create_vertex (xmin, ymax);
  db::Vertex *vbr = create_vertex (xmax, ymin);
  db::Vertex *vtr = create_vertex (xmax, ymax);

  db::TriangleEdge *sl = create_edge (vbl, vtl);
  db::TriangleEdge *sd = create_edge (vtl, vbr);
  db::TriangleEdge *sb = create_edge (vbr, vbl);

  db::TriangleEdge *sr = create_edge (vbr, vtr);
  db::TriangleEdge *st = create_edge (vtr, vtl);

  create_triangle (sl, sd, sb);
  create_triangle (sd, sr, st);
}

std::string
Triangles::to_string ()
{

}

db::DBox
Triangles::bbox () const
{
  db::DBox box;
  for (auto t = mp_triangles.begin (); t != mp_triangles.end (); ++t) {
    for (int i = 0; i < 3; ++i) {
      box += *t->vertex (i);
    }
  }
  return box;
}

bool
Triangles::check (bool check_delaunay) const
{

}

std::vector<db::Vertex *>
Triangles::find_points_around (const db::Vertex *vertex, double radius)
{

}

db::Vertex *
Triangles::insert_point (const db::DPoint &point, std::vector<db::Triangle *> *new_triangles)
{

}

db::Vertex *
Triangles::insert (db::Vertex *vertex, std::vector<db::Triangle *> *new_triangles)
{

}

void
Triangles::remove (db::Vertex *vertex, std::vector<db::Triangle *> *new_triangles)
{

}

std::vector<db::Vertex *>
Triangles::find_touching (const db::DBox &box)
{

}

std::vector<db::Vertex *>
Triangles::find_inside_circle (const db::DPoint &center, double radius)
{

}

void
Triangles::remove_outside_vertex (db::Vertex *vertex, std::vector<db::Triangle *> *new_triangles)
{

}

void
Triangles::remove_inside_vertex (db::Vertex *vertex, std::vector<db::Triangle *> *new_triangles)
{

}

std::vector<db::Triangle *>
Triangles::fill_concave_corners (const std::vector<db::TriangleEdge> &edges)
{

}

}
