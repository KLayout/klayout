
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
#include "tlUnitTest.h"

TEST(Triangle_basic)
{
  db::Triangles tris;
  tris.init_box (db::DBox (1, 0, 5, 4));

  EXPECT_EQ (tris.bbox ().to_string (), "(1,0;5,4)");
  EXPECT_EQ (tris.to_string (), "((1, 0), (1, 4), (5, 0)), ((1, 4), (5, 4), (5, 0))");

  EXPECT_EQ (tris.check (), true);
}

TEST(Triangle_flip)
{
  db::Triangles tris;
  tris.init_box (db::DBox (0, 0, 1, 1));
  EXPECT_EQ (tris.to_string (), "((0, 0), (0, 1), (1, 0)), ((0, 1), (1, 1), (1, 0))");

  EXPECT_EQ (tris.num_triangles (), size_t (2));

  const db::Triangle &t1 = *tris.begin ();
  db::TriangleEdge *diag_segment;
  for (int i = 0; i < 3; ++i) {
    diag_segment = t1.edge (i);
    if (diag_segment->side_of (db::DPoint (0.5, 0.5)) == 0) {
      break;
    }
  }
  tris.flip (diag_segment);
  EXPECT_EQ (tris.to_string (), "((1, 1), (0, 0), (0, 1)), ((1, 1), (1, 0), (0, 0))");
  EXPECT_EQ (tris.check (), true);
}

TEST(Triangle_insert)
{
  db::Triangles tris;
  tris.init_box (db::DBox (0, 0, 1, 1));

  tris.insert_point (0.2, 0.2);
  EXPECT_EQ (tris.to_string (), "((0, 0), (0, 1), (0.2, 0.2)), ((1, 0), (0, 0), (0.2, 0.2)), ((1, 1), (0.2, 0.2), (0, 1)), ((1, 1), (1, 0), (0.2, 0.2))");
  EXPECT_EQ (tris.check (), true);
}

TEST(Triangle_split_segment)
{
  db::Triangles tris;
  tris.init_box (db::DBox (0, 0, 1, 1));

  tris.insert_point (0.5, 0.5);
  EXPECT_EQ (tris.to_string (), "((1, 1), (1, 0), (0.5, 0.5)), ((1, 1), (0.5, 0.5), (0, 1)), ((0, 0), (0, 1), (0.5, 0.5)), ((0, 0), (0.5, 0.5), (1, 0))");
  EXPECT_EQ (tris.check(), true);
}

TEST(Triangle_insert_vertex_twice)
{
  db::Triangles tris;
  tris.init_box (db::DBox (0, 0, 1, 1));

  tris.insert_point (0.5, 0.5);
  //  inserted a vertex twice does not change anything
  tris.insert_point (0.5, 0.5);
  EXPECT_EQ (tris.to_string (), "((1, 1), (1, 0), (0.5, 0.5)), ((1, 1), (0.5, 0.5), (0, 1)), ((0, 0), (0, 1), (0.5, 0.5)), ((0, 0), (0.5, 0.5), (1, 0))");
  EXPECT_EQ (tris.check(), true);
}

TEST(Triangle_insert_vertex_convex)
{
  db::Triangles tris;
  tris.insert_point (0.2, 0.2);
  tris.insert_point (0.2, 0.8);
  tris.insert_point (0.6, 0.5);
  tris.insert_point (0.7, 0.5);
  tris.insert_point (0.6, 0.4);
  EXPECT_EQ (tris.to_string (), "((0.2, 0.2), (0.2, 0.8), (0.6, 0.5)), ((0.7, 0.5), (0.6, 0.5), (0.2, 0.8)), ((0.6, 0.5), (0.6, 0.4), (0.2, 0.2)), ((0.6, 0.5), (0.7, 0.5), (0.6, 0.4))");
  EXPECT_EQ (tris.check(), true);
}

TEST(Triangle_insert_vertex_convex2)
{
  db::Triangles tris;
  tris.insert_point (0.25, 0.1);
  tris.insert_point (0.1, 0.4);
  tris.insert_point (0.4, 0.15);
  tris.insert_point (1, 0.7);
  EXPECT_EQ (tris.to_string (), "((0.25, 0.1), (0.1, 0.4), (0.4, 0.15)), ((1, 0.7), (0.4, 0.15), (0.1, 0.4))");
  EXPECT_EQ (tris.check(), true);
}

TEST(Triangle_insert_vertex_convex3)
{
  db::Triangles tris;
  tris.insert_point (0.25, 0.5);
  tris.insert_point (0.25, 0.55);
  tris.insert_point (0.15, 0.8);
  tris.insert_point (1, 0.4);
  EXPECT_EQ (tris.to_string (), "((0.25, 0.5), (0.15, 0.8), (0.25, 0.55)), ((1, 0.4), (0.25, 0.5), (0.25, 0.55)), ((0.15, 0.8), (1, 0.4), (0.25, 0.55))");
  EXPECT_EQ (tris.check(), true);
}

TEST(Triangle_search_edges_crossing)
{
  db::Triangles tris;
  db::Vertex *v1 = tris.insert_point (0.2, 0.2);
  db::Vertex *v2 = tris.insert_point (0.2, 0.8);
  db::Vertex *v3 = tris.insert_point (0.6, 0.5);
  /*db::Vertex *v4 =*/ tris.insert_point (0.7, 0.5);
  db::Vertex *v5 = tris.insert_point (0.6, 0.4);
  db::Vertex *v6 = tris.insert_point (0.7, 0.2);
  EXPECT_EQ (tris.check(), true);

  auto xedges = tris.search_edges_crossing (v2, v6);

  EXPECT_EQ (xedges.size (), size_t (2));
  auto s1 = tris.find_edge_for_points (*v1, *v3);
  auto s2 = tris.find_edge_for_points (*v1, *v5);
  EXPECT_EQ (std::find (xedges.begin (), xedges.end (), s1) != xedges.end (), true);
  EXPECT_EQ (std::find (xedges.begin (), xedges.end (), s2) != xedges.end (), true);
}
