
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


#include "dbPLC.h"
#include "tlUnitTest.h"

#include <list>
#include <memory>

namespace
{

class TestableGraph
  : public db::plc::Graph
{
public:
  using db::plc::Graph::Graph;
  using db::plc::Graph::create_vertex;
  using db::plc::Graph::create_edge;
  using db::plc::Graph::create_triangle;
  using db::plc::Graph::create_polygon;
};

}

TEST(basic)
{
  TestableGraph plc;

  db::plc::Vertex *v1 = plc.create_vertex (db::DPoint (1, 2));
  EXPECT_EQ (v1->to_string (), "(1, 2)");

  v1 = plc.create_vertex (db::DPoint (2, 1));
  EXPECT_EQ (v1->to_string (), "(2, 1)");

  EXPECT_EQ (v1->is_precious (), false);
  v1->set_is_precious (true, 17);
  EXPECT_EQ (v1->is_precious (), true);
  EXPECT_EQ (v1->ids ().size (), 1u);
  EXPECT_EQ (*v1->ids ().begin (), 17u);
  v1->set_is_precious (true, 1);
  EXPECT_EQ (v1->is_precious (), true);
  EXPECT_EQ (v1->ids ().size (), 2u);
  EXPECT_EQ (*v1->ids ().begin (), 1u);
}

TEST(edge)
{
  TestableGraph plc;

  db::plc::Vertex *v1 = plc.create_vertex (db::DPoint (1, 2));
  db::plc::Vertex *v2 = plc.create_vertex (db::DPoint (3, 4));

  db::plc::Edge *e = plc.create_edge (v1, v2);
  EXPECT_EQ (e->to_string (), "((1, 2), (3, 4))");

  EXPECT_EQ (v1->num_edges (), size_t (1));
  EXPECT_EQ (v2->num_edges (), size_t (1));

  EXPECT_EQ ((*v1->begin_edges ())->edge ().to_string (), "(1,2;3,4)");
  EXPECT_EQ ((*v2->begin_edges ())->edge ().to_string (), "(1,2;3,4)");
}

TEST(polygon)
{
  TestableGraph plc;
  EXPECT_EQ (plc.num_polygons (), size_t (0));
  EXPECT_EQ (plc.bbox ().to_string (), "()");

  db::plc::Vertex *v1 = plc.create_vertex (db::DPoint (1, 2));
  db::plc::Vertex *v2 = plc.create_vertex (db::DPoint (3, 4));
  db::plc::Vertex *v3 = plc.create_vertex (db::DPoint (3, 2));

  db::plc::Edge *e1 = plc.create_edge (v1, v2);
  db::plc::Edge *e2 = plc.create_edge (v1, v3);
  db::plc::Edge *e3 = plc.create_edge (v2, v3);

  db::plc::Polygon *tri = plc.create_triangle (e1, e2, e3);
  EXPECT_EQ (tri->to_string (), "((1, 2), (3, 4), (3, 2))");
  EXPECT_EQ (tri->polygon ().to_string (), "(1,2;3,4;3,2)");
  EXPECT_EQ (plc.bbox ().to_string (), "(1,2;3,4)");
  EXPECT_EQ (plc.num_polygons (), size_t (1));

  EXPECT_EQ (v1->num_edges (), size_t (2));
  EXPECT_EQ (v2->num_edges (), size_t (2));
  EXPECT_EQ (v3->num_edges (), size_t (2));

  EXPECT_EQ (tri->edge (0) == e1, true);
  EXPECT_EQ (tri->edge (3) == e1, true);
  EXPECT_EQ (tri->edge (1) == e3, true);
  EXPECT_EQ (tri->edge (2) == e2, true);
  EXPECT_EQ (tri->edge (-1) == e2, true);

  EXPECT_EQ (e1->left () == 0, true);
  EXPECT_EQ (e1->right () == tri, true);
  EXPECT_EQ (e2->left () == tri, true);
  EXPECT_EQ (e2->right () == 0, true);
  EXPECT_EQ (e3->left () == 0, true);
  EXPECT_EQ (e3->right () == tri, true);

  delete tri;

  EXPECT_EQ (e1->left () == 0, true);
  EXPECT_EQ (e1->right () == 0, true);
  EXPECT_EQ (e2->left () == 0, true);
  EXPECT_EQ (e2->right () == 0, true);
  EXPECT_EQ (e3->left () == 0, true);
  EXPECT_EQ (e3->right () == 0, true);
}
