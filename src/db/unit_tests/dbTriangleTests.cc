
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


#include "dbTriangle.h"
#include "tlUnitTest.h"

#include <list>

//  Tests for Triangle class

TEST(Triangle_basic)
{
  db::Vertex v1;
  db::Vertex v2 (1, 2);
  db::Vertex v3 (2, 1);

  db::TriangleEdge s1 (&v1, &v2);
  db::TriangleEdge s2 (&v2, &v3);
  db::TriangleEdge s3 (&v3, &v1);

  db::Triangle tri (&s1, &s2, &s3);
  EXPECT_EQ (tri.to_string (), "((0, 0), (1, 2), (2, 1))");

  //  ordering
  db::TriangleEdge s11 (&v1, &v2);
  db::TriangleEdge s12 (&v3, &v2);
  db::TriangleEdge s13 (&v1, &v3);

  db::Triangle tri2 (&s11, &s12, &s13);
  EXPECT_EQ (tri2.to_string (), "((0, 0), (1, 2), (2, 1))");
}

TEST(Triangle_find_segment_with)
{
  db::Vertex v1;
  db::Vertex v2 (1, 2);
  db::Vertex v3 (2, 1);

  db::TriangleEdge s1 (&v1, &v2);
  db::TriangleEdge s2 (&v2, &v3);
  db::TriangleEdge s3 (&v3, &v1);

  db::Triangle tri (&s1, &s2, &s3);

  EXPECT_EQ (tri.find_edge_with (&v1, &v2)->to_string (), "((0, 0), (1, 2))");
  EXPECT_EQ (tri.find_edge_with (&v2, &v1)->to_string (), "((0, 0), (1, 2))");
}

TEST(Triangle_ext_vertex)
{
  db::Vertex v1;
  db::Vertex v2 (1, 2);
  db::Vertex v3 (2, 1);

  db::TriangleEdge s1 (&v1, &v2);
  db::TriangleEdge s2 (&v2, &v3);
  db::TriangleEdge s3 (&v3, &v1);

  db::Triangle tri (&s1, &s2, &s3);

  EXPECT_EQ (tri.opposite (&s1)->to_string (), "(2, 1)");
  EXPECT_EQ (tri.opposite (&s3)->to_string (), "(1, 2)");
}

TEST(Triangle_opposite_vertex)
{
  db::Vertex v1;
  db::Vertex v2 (1, 2);
  db::Vertex v3 (2, 1);

  db::TriangleEdge s1 (&v1, &v2);
  db::TriangleEdge s2 (&v2, &v3);
  db::TriangleEdge s3 (&v3, &v1);

  db::Triangle tri (&s1, &s2, &s3);

  EXPECT_EQ (tri.opposite (&s1)->to_string (), "(2, 1)");
  EXPECT_EQ (tri.opposite (&s3)->to_string (), "(1, 2)");
}

TEST(Triangle_opposite_edge)
{
  db::Vertex v1;
  db::Vertex v2 (1, 2);
  db::Vertex v3 (2, 1);

  db::TriangleEdge s1 (&v1, &v2);
  db::TriangleEdge s2 (&v2, &v3);
  db::TriangleEdge s3 (&v3, &v1);

  db::Triangle tri (&s1, &s2, &s3);

  EXPECT_EQ (tri.opposite (&v1)->to_string (), "((1, 2), (2, 1))");
  EXPECT_EQ (tri.opposite (&v3)->to_string (), "((0, 0), (1, 2))");
}

TEST(Triangle_contains)
{
  db::Vertex v1;
  db::Vertex v2 (1, 2);
  db::Vertex v3 (2, 1);

  db::TriangleEdge s1 (&v1, &v2);
  db::TriangleEdge s2 (&v2, &v3);
  db::TriangleEdge s3 (&v3, &v1);

  db::Triangle tri (&s1, &s2, &s3);

  EXPECT_EQ (tri.contains (db::DPoint (0, 0)), 0);
  EXPECT_EQ (tri.contains (db::DPoint (-1, -2)), -1);
  EXPECT_EQ (tri.contains (db::DPoint (0.5, 1)), 0);
  EXPECT_EQ (tri.contains (db::DPoint (0.5, 2)), -1);
  EXPECT_EQ (tri.contains (db::DPoint (2.5, 1)), -1);
  EXPECT_EQ (tri.contains (db::DPoint (1, -1)), -1);
  EXPECT_EQ (tri.contains (db::DPoint (1, 1)), 1);

  s1.reverse ();
  s2.reverse ();
  s3.reverse ();

  db::Triangle tri2 (&s3, &s2, &s1);
  EXPECT_EQ (tri2.contains(db::DPoint(0, 0)), 0);
  EXPECT_EQ (tri2.contains(db::DPoint(0.5, 1)), 0);
  EXPECT_EQ (tri2.contains(db::DPoint(0.5, 2)), -1);
  EXPECT_EQ (tri2.contains(db::DPoint(2.5, 1)), -1);
  EXPECT_EQ (tri2.contains(db::DPoint(1, -1)), -1);
  EXPECT_EQ (tri2.contains(db::DPoint(1, 1)), 1);
}

TEST(Triangle_circumcircle)
{
  db::Vertex v1;
  db::Vertex v2 (1, 2);
  db::Vertex v3 (2, 1);

  db::TriangleEdge s1 (&v1, &v2);
  db::TriangleEdge s2 (&v2, &v3);
  db::TriangleEdge s3 (&v3, &v1);

  db::Triangle tri (&s1, &s2, &s3);

  auto cc = tri.circumcircle ();
  auto center = cc.first;
  auto radius = cc.second;

  EXPECT_EQ (tl::to_string (center), "0.833333333333,0.833333333333");
  EXPECT_EQ (tl::to_string (radius), "1.17851130198");

  EXPECT_EQ (db::Vertex::in_circle (center, center, radius), 1);
  EXPECT_EQ (db::Vertex::in_circle (db::DPoint (-1, -1), center, radius), -1);
  EXPECT_EQ (v1.in_circle (center, radius), 0);
  EXPECT_EQ (v2.in_circle (center, radius), 0);
  EXPECT_EQ (v3.in_circle (center, radius), 0);
}

//  Tests for TriangleEdge class

TEST(TriangleEdge_basic)
{
  db::Vertex v1;
  db::Vertex v2 (1, 0.5);

  db::TriangleEdge edge (&v1, &v2);
  EXPECT_EQ (edge.to_string (), "((0, 0), (1, 0.5))");
}

TEST(TriangleEdge_side_of)
{
  db::Vertex v1;
  db::Vertex v2 (1, 0.5);

  db::TriangleEdge edge (&v1, &v2);
  EXPECT_EQ (edge.to_string (), "((0, 0), (1, 0.5))");

  EXPECT_EQ (edge.side_of (db::Vertex (0, 0)), 0)
  EXPECT_EQ (edge.side_of (db::Vertex (0.5, 0.25)), 0)
  EXPECT_EQ (edge.side_of (db::Vertex (0, 1)), -1)
  EXPECT_EQ (edge.side_of (db::Vertex (0, -1)), 1)
  EXPECT_EQ (edge.side_of (db::Vertex (0.5, 0.5)), -1)
  EXPECT_EQ (edge.side_of (db::Vertex (0.5, 0)), 1)

  db::Vertex v3 (1, 0);
  db::Vertex v4 (0, 1);
  db::TriangleEdge edge2 (&v3, &v4);

  EXPECT_EQ (edge2.side_of (db::Vertex(0.2, 0.2)), -1);
}

namespace {
  class VertexHeap
  {
  public:
    db::Vertex *make_vertex (double x, double y)
    {
      m_heap.push_back (db::Vertex (x, y));
      return &m_heap.back ();
    }
  private:
    std::list<db::Vertex> m_heap;
  };
}

TEST(TriangleEdge_crosses)
{
  VertexHeap heap;

  db::TriangleEdge s1 (heap.make_vertex (0, 0), heap.make_vertex (1, 0.5));
  EXPECT_EQ (s1.crosses (db::TriangleEdge (heap.make_vertex (-1, -0.5), heap.make_vertex(1, -0.5))), false);
  EXPECT_EQ (s1.crosses (db::TriangleEdge (heap.make_vertex (-1, 0), heap.make_vertex(1, 0))), false);  // only cuts
  EXPECT_EQ (s1.crosses (db::TriangleEdge (heap.make_vertex (-1, 0.5), heap.make_vertex(1, 0.5))), false);
  EXPECT_EQ (s1.crosses (db::TriangleEdge (heap.make_vertex (-1, 0.5), heap.make_vertex(2, 0.5))), false);
  EXPECT_EQ (s1.crosses (db::TriangleEdge (heap.make_vertex (-1, 0.25), heap.make_vertex(2, 0.25))), true);
  EXPECT_EQ (s1.crosses (db::TriangleEdge (heap.make_vertex (-1, 0.5), heap.make_vertex(-0.1, 0.5))), false);
  EXPECT_EQ (s1.crosses (db::TriangleEdge (heap.make_vertex (-1, 0.6), heap.make_vertex(0, 0.6))), false);
  EXPECT_EQ (s1.crosses (db::TriangleEdge (heap.make_vertex (-1, 1), heap.make_vertex(1, 1))), false);

  EXPECT_EQ (s1.crosses_including (db::TriangleEdge (heap.make_vertex (-1, -0.5), heap.make_vertex(1, -0.5))), false);
  EXPECT_EQ (s1.crosses_including (db::TriangleEdge (heap.make_vertex (-1, 0), heap.make_vertex(1, 0))), true);  // only cuts
  EXPECT_EQ (s1.crosses_including (db::TriangleEdge (heap.make_vertex (-1, 0.25), heap.make_vertex(2, 0.25))), true);
}

TEST(TriangleEdge_point_on)
{
  VertexHeap heap;

  db::TriangleEdge s1 (heap.make_vertex (0, 0), heap.make_vertex (1, 0.5));
  EXPECT_EQ (s1.point_on (db::DPoint (0, 0)), false); //  endpoints are not "on"
  EXPECT_EQ (s1.point_on (db::DPoint (0, -0.5)), false);
  EXPECT_EQ (s1.point_on (db::DPoint (0.5, 0)), false);
  EXPECT_EQ (s1.point_on (db::DPoint (0.5, 0.25)), true);
  EXPECT_EQ (s1.point_on (db::DPoint (1, 0.5)), false); //  endpoints are not "on"
  EXPECT_EQ (s1.point_on (db::DPoint (1, 1)), false);
  EXPECT_EQ (s1.point_on (db::DPoint (2, 1)), false);
}

TEST(TriangleEdge_intersection_point)
{
  VertexHeap heap;

  db::TriangleEdge s1 (heap.make_vertex (0, 0), heap.make_vertex (1, 0.5));
  EXPECT_EQ (s1.intersection_point (db::TriangleEdge (heap.make_vertex (-1, 0.25), heap.make_vertex (2, 0.25))).to_string (), "0.5,0.25");
}

TEST(TriangleEdge_can_flip)
{
}

#if 0
class TestSegment(unittest.TestCase):

  def test_can_flip(self):
    v1 = t.Vertex(2, -1)
    v2 = t.Vertex(0, 0)
    v3 = t.Vertex(1, 0)
    v4 = t.Vertex(0.5, 1)
    s1 = t.TriangleEdge(v1, v2)
    s2 = t.TriangleEdge(v1, v3)
    s3 = t.TriangleEdge(v2, v3)
    s4 = t.TriangleEdge(v2, v4)
    s5 = t.TriangleEdge(v3, v4)
    t1 = t.Triangle(s1, s2, s3)
    t2 = t.Triangle(s3, s4, s5)
    s3.left = t1
    s3.right = t2
    EXPECT_EQ (s3.can_flip(), False)
    v1.x = 0.5
    EXPECT_EQ (s3.can_flip(), True)
    v1.x = -0.25
    EXPECT_EQ (s3.can_flip(), True)
    v1.x = -0.5
    EXPECT_EQ (s3.can_flip(), False)
    v1.x = -1.0
    EXPECT_EQ (s3.can_flip(), False)

  def test_distance(self):
    seg = t.TriangleEdge(t.Vertex(0, 0), t.Vertex(1, 0))
    EXPECT_EQ (seg.distance(t.Point(0, 0)), 0)
    EXPECT_EQ (seg.distance(t.Point(0, 1)), 1)
    EXPECT_EQ (seg.distance(t.Point(1, 2)), 2)
    EXPECT_EQ (seg.distance(t.Point(1, -1)), 1)
    EXPECT_EQ (seg.distance(t.Point(2, 0)), 1)
    EXPECT_EQ (seg.distance(t.Point(-2, 0)), 2)
    seg.reverse()
    EXPECT_EQ (seg.distance(t.Point(0, 0)), 0)
    EXPECT_EQ (seg.distance(t.Point(0, 1)), 1)
    EXPECT_EQ (seg.distance(t.Point(1, 2)), 2)
    EXPECT_EQ (seg.distance(t.Point(1, -1)), 1)
    EXPECT_EQ (seg.distance(t.Point(2, 0)), 1)
    EXPECT_EQ (seg.distance(t.Point(-2, 0)), 2)
#endif
