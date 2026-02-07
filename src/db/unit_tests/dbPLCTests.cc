
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

class TestableEdge
  : public db::plc::Edge
{
public:
  using db::plc::Edge::Edge;
  using db::plc::Edge::link;
  using db::plc::Edge::unlink;
  using db::plc::Edge::set_is_segment;
  using db::plc::Edge::set_level;
  using db::plc::Edge::level;

  TestableEdge (db::plc::Vertex *v1, db::plc::Vertex *v2)
    : db::plc::Edge (0, v1, v2)
  { }
};

class TestableVertex
  : public db::plc::Vertex
{
public:
  TestableVertex ()
    : db::plc::Vertex (0)
  { }

  TestableVertex (double x, double y)
    : db::plc::Vertex (0, x, y)
  { }

  TestableVertex (const db::DPoint &pt)
    : db::plc::Vertex (0, pt)
  { }
};

class TestablePolygon
  : public db::plc::Polygon
{
public:
  TestablePolygon ()
    : db::plc::Polygon (0)
  { }

  TestablePolygon (db::plc::Edge *e1, db::plc::Edge *e2, db::plc::Edge *e3)
    : db::plc::Polygon (0, e1, e2, e3)
  { }

  using db::plc::Polygon::set_outside;
};

//  Tests for Vertex class

TEST(Vertex_basic)
{
  TestableVertex v;

  v.set_x (1.5);
  v.set_y (0.5);
  EXPECT_EQ (v.to_string (), "(1.5, 0.5)");
  EXPECT_EQ (v.x (), 1.5);
  EXPECT_EQ (v.y (), 0.5);

  v = TestableVertex (db::DPoint (2, 3));
  EXPECT_EQ (v.to_string (), "(2, 3)");
}

static std::string edges_from_vertex (const TestableVertex &v)
{
  std::string res;
  for (auto e = v.begin_edges (); e != v.end_edges (); ++e) {
    if (! res.empty ()) {
      res += ", ";
    }
    res += (*e)->to_string ();
  }
  return res;
}

static std::string triangles_from_vertex (const TestableVertex &v)
{
  auto tri = v.polygons ();
  std::string res;
  for (auto t = tri.begin (); t != tri.end (); ++t) {
    if (! res.empty ()) {
      res += ", ";
    }
    res += (*t)->to_string ();
  }
  return res;
}

TEST(Vertex_edge_registration)
{
  TestableVertex v1 (0, 0);
  TestableVertex v2 (1, 2);
  TestableVertex v3 (2, 1);

  std::unique_ptr<TestableEdge> e1 (new TestableEdge (&v1, &v2));
  e1->link ();
  EXPECT_EQ (edges_from_vertex (v1), "((0, 0), (1, 2))");
  EXPECT_EQ (edges_from_vertex (v2), "((0, 0), (1, 2))");
  EXPECT_EQ (edges_from_vertex (v3), "");

  std::unique_ptr<TestableEdge> e2 (new TestableEdge (&v2, &v3));
  e2->link ();
  EXPECT_EQ (edges_from_vertex (v1), "((0, 0), (1, 2))");
  EXPECT_EQ (edges_from_vertex (v2), "((0, 0), (1, 2)), ((1, 2), (2, 1))");
  EXPECT_EQ (edges_from_vertex (v3), "((1, 2), (2, 1))");

  e2->unlink ();
  e2.reset (0);
  EXPECT_EQ (edges_from_vertex (v1), "((0, 0), (1, 2))");
  EXPECT_EQ (edges_from_vertex (v2), "((0, 0), (1, 2))");
  EXPECT_EQ (edges_from_vertex (v3), "");

  e1->unlink ();
  e1.reset (0);
  EXPECT_EQ (edges_from_vertex (v1), "");
  EXPECT_EQ (edges_from_vertex (v2), "");
  EXPECT_EQ (edges_from_vertex (v3), "");
}

TEST(Vertex_triangles)
{
  TestableVertex v1 (0, 0);
  TestableVertex v2 (1, 2);
  TestableVertex v3 (2, 1);
  TestableVertex v4 (-1, 2);
  EXPECT_EQ (triangles_from_vertex (v1), "");

  std::unique_ptr<TestableEdge> e1 (new TestableEdge (&v1, &v2));
  e1->link ();
  std::unique_ptr<TestableEdge> e2 (new TestableEdge (&v2, &v3));
  e2->link ();
  std::unique_ptr<TestableEdge> e3 (new TestableEdge (&v3, &v1));
  e3->link ();

  std::unique_ptr<TestablePolygon> tri (new TestablePolygon (e1.get (), e2.get (), e3.get ()));
  EXPECT_EQ (triangles_from_vertex (v1), "((0, 0), (1, 2), (2, 1))");
  EXPECT_EQ (triangles_from_vertex (v2), "((0, 0), (1, 2), (2, 1))");
  EXPECT_EQ (triangles_from_vertex (v3), "((0, 0), (1, 2), (2, 1))");

  std::unique_ptr<TestableEdge> e4 (new TestableEdge (&v1, &v4));
  e4->link ();
  std::unique_ptr<TestableEdge> e5 (new TestableEdge (&v2, &v4));
  e5->link ();
  std::unique_ptr<TestablePolygon> tri2 (new TestablePolygon (e1.get (), e4.get (), e5.get ()));
  EXPECT_EQ (triangles_from_vertex (v1), "((0, 0), (-1, 2), (1, 2)), ((0, 0), (1, 2), (2, 1))");
  EXPECT_EQ (triangles_from_vertex (v2), "((0, 0), (-1, 2), (1, 2)), ((0, 0), (1, 2), (2, 1))");
  EXPECT_EQ (triangles_from_vertex (v3), "((0, 0), (1, 2), (2, 1))");
  EXPECT_EQ (triangles_from_vertex (v4), "((0, 0), (-1, 2), (1, 2))");

  tri->unlink ();
  EXPECT_EQ (triangles_from_vertex (v1), "((0, 0), (-1, 2), (1, 2))");

  tri2->unlink ();
  EXPECT_EQ (triangles_from_vertex (v1), "");
}

//  Tests for Triangle class

TEST(Triangle_basic)
{
  TestableVertex v1;
  TestableVertex v2 (1, 2);
  TestableVertex v3 (2, 1);

  TestableEdge s1 (&v1, &v2);
  TestableEdge s2 (&v2, &v3);
  TestableEdge s3 (&v3, &v1);

  EXPECT_EQ (s1.v1 () == &v1, true);
  EXPECT_EQ (s2.v2 () == &v3, true);

  TestablePolygon tri (&s1, &s2, &s3);
  EXPECT_EQ (tri.to_string (), "((0, 0), (1, 2), (2, 1))");
  EXPECT_EQ (tri.edge (-1) == &s3, true);
  EXPECT_EQ (tri.edge (0) == &s1, true);
  EXPECT_EQ (tri.edge (1) == &s2, true);
  EXPECT_EQ (tri.edge (3) == &s1, true);

  //  ordering
  TestableEdge s11 (&v1, &v2);
  TestableEdge s12 (&v3, &v2);
  TestableEdge s13 (&v1, &v3);

  TestablePolygon tri2 (&s11, &s12, &s13);
  EXPECT_EQ (tri2.to_string (), "((0, 0), (1, 2), (2, 1))");

  //  triangle registration
  EXPECT_EQ (s11.right () == &tri2, true);
  EXPECT_EQ (s11.left () == 0, true);
  EXPECT_EQ (s12.left () == &tri2, true);
  EXPECT_EQ (s12.right () == 0, true);
  EXPECT_EQ (s13.left () == &tri2, true);
  EXPECT_EQ (s13.right () == 0, true);

  EXPECT_EQ (s13.to_string (), "((0, 0), (2, 1))");
  s13.reverse ();
  EXPECT_EQ (s13.to_string (), "((2, 1), (0, 0))");
  EXPECT_EQ (s13.right () == &tri2, true);
  EXPECT_EQ (s13.left () == 0, true);

  //  flags
  EXPECT_EQ (tri.is_outside (), false);
  tri.set_outside (true);
  EXPECT_EQ (tri.is_outside (), true);
}

TEST(Triangle_find_segment_with)
{
  TestableVertex v1;
  TestableVertex v2 (1, 2);
  TestableVertex v3 (2, 1);

  TestableEdge s1 (&v1, &v2);
  TestableEdge s2 (&v2, &v3);
  TestableEdge s3 (&v3, &v1);

  TestablePolygon tri (&s1, &s2, &s3);

  EXPECT_EQ (tri.find_edge_with (&v1, &v2)->to_string (), "((0, 0), (1, 2))");
  EXPECT_EQ (tri.find_edge_with (&v2, &v1)->to_string (), "((0, 0), (1, 2))");
}

TEST(Triangle_ext_vertex)
{
  TestableVertex v1;
  TestableVertex v2 (1, 2);
  TestableVertex v3 (2, 1);

  TestableEdge s1 (&v1, &v2);
  TestableEdge s2 (&v2, &v3);
  TestableEdge s3 (&v3, &v1);

  TestablePolygon tri (&s1, &s2, &s3);

  EXPECT_EQ (tri.opposite (&s1)->to_string (), "(2, 1)");
  EXPECT_EQ (tri.opposite (&s3)->to_string (), "(1, 2)");
}

TEST(Triangle_opposite_vertex)
{
  TestableVertex v1;
  TestableVertex v2 (1, 2);
  TestableVertex v3 (2, 1);

  TestableEdge s1 (&v1, &v2);
  TestableEdge s2 (&v2, &v3);
  TestableEdge s3 (&v3, &v1);

  TestablePolygon tri (&s1, &s2, &s3);

  EXPECT_EQ (tri.opposite (&s1)->to_string (), "(2, 1)");
  EXPECT_EQ (tri.opposite (&s3)->to_string (), "(1, 2)");
}

TEST(Triangle_opposite_edge)
{
  TestableVertex v1;
  TestableVertex v2 (1, 2);
  TestableVertex v3 (2, 1);

  TestableEdge s1 (&v1, &v2);
  TestableEdge s2 (&v2, &v3);
  TestableEdge s3 (&v3, &v1);

  TestablePolygon tri (&s1, &s2, &s3);

  EXPECT_EQ (tri.opposite (&v1)->to_string (), "((1, 2), (2, 1))");
  EXPECT_EQ (tri.opposite (&v3)->to_string (), "((0, 0), (1, 2))");
}

TEST(Triangle_contains)
{
  TestableVertex v1;
  TestableVertex v2 (1, 2);
  TestableVertex v3 (2, 1);

  TestableEdge s1 (&v1, &v2);
  TestableEdge s2 (&v2, &v3);
  TestableEdge s3 (&v3, &v1);

  {
    TestablePolygon tri (&s1, &s2, &s3);
    EXPECT_EQ (tri.contains (db::DPoint (0, 0)), 0);
    EXPECT_EQ (tri.contains (db::DPoint (-1, -2)), -1);
    EXPECT_EQ (tri.contains (db::DPoint (0.5, 1)), 0);
    EXPECT_EQ (tri.contains (db::DPoint (0.5, 2)), -1);
    EXPECT_EQ (tri.contains (db::DPoint (2.5, 1)), -1);
    EXPECT_EQ (tri.contains (db::DPoint (1, -1)), -1);
    EXPECT_EQ (tri.contains (db::DPoint (1, 1)), 1);
  }

  s1.reverse ();
  s2.reverse ();
  s3.reverse ();

  {
    TestablePolygon tri2 (&s3, &s2, &s1);
    EXPECT_EQ (tri2.contains(db::DPoint(0, 0)), 0);
    EXPECT_EQ (tri2.contains(db::DPoint(0.5, 1)), 0);
    EXPECT_EQ (tri2.contains(db::DPoint(0.5, 2)), -1);
    EXPECT_EQ (tri2.contains(db::DPoint(2.5, 1)), -1);
    EXPECT_EQ (tri2.contains(db::DPoint(1, -1)), -1);
    EXPECT_EQ (tri2.contains(db::DPoint(1, 1)), 1);
  }
}

TEST(Triangle_contains_small)
{
  TestableVertex v1;
  TestableVertex v2 (0.001, 0.002);
  TestableVertex v3 (0.002, 0.001);

  TestableEdge s1 (&v1, &v2);
  TestableEdge s2 (&v2, &v3);
  TestableEdge s3 (&v3, &v1);

  {
    TestablePolygon tri (&s1, &s2, &s3);
    EXPECT_EQ (tri.contains (db::DPoint (0, 0)), 0);
    EXPECT_EQ (tri.contains (db::DPoint (-0.001, -0.002)), -1);
    EXPECT_EQ (tri.contains (db::DPoint (0.0005, 0.001)), 0);
    EXPECT_EQ (tri.contains (db::DPoint (0.0005, 0.002)), -1);
    EXPECT_EQ (tri.contains (db::DPoint (0.0025, 0.001)), -1);
    EXPECT_EQ (tri.contains (db::DPoint (0.001, -0.001)), -1);
    EXPECT_EQ (tri.contains (db::DPoint (0.001, 0.001)), 1);
  }

  s1.reverse ();
  s2.reverse ();
  s3.reverse ();

  {
    TestablePolygon tri2 (&s3, &s2, &s1);
    EXPECT_EQ (tri2.contains(db::DPoint(0, 0)), 0);
    EXPECT_EQ (tri2.contains(db::DPoint(0.0005, 0.001)), 0);
    EXPECT_EQ (tri2.contains(db::DPoint(0.0005, 0.002)), -1);
    EXPECT_EQ (tri2.contains(db::DPoint(0.0025, 0.001)), -1);
    EXPECT_EQ (tri2.contains(db::DPoint(0.001, -0.001)), -1);
    EXPECT_EQ (tri2.contains(db::DPoint(0.001, 0.001)), 1);
  }
}

TEST(Triangle_circumcircle)
{
  TestableVertex v1;
  TestableVertex v2 (1, 2);
  TestableVertex v3 (2, 1);

  TestableEdge s1 (&v1, &v2);
  TestableEdge s2 (&v2, &v3);
  TestableEdge s3 (&v3, &v1);

  TestablePolygon tri (&s1, &s2, &s3);

  auto cc = tri.circumcircle ();
  auto center = cc.first;
  auto radius = cc.second;

  EXPECT_EQ (tl::to_string (center), "0.833333333333,0.833333333333");
  EXPECT_EQ (tl::to_string (radius), "1.17851130198");

  EXPECT_EQ (TestableVertex::in_circle (center, center, radius), 1);
  EXPECT_EQ (TestableVertex::in_circle (db::DPoint (-1, -1), center, radius), -1);
  EXPECT_EQ (v1.in_circle (center, radius), 0);
  EXPECT_EQ (v2.in_circle (center, radius), 0);
  EXPECT_EQ (v3.in_circle (center, radius), 0);
}

//  Tests for TriangleEdge class

TEST(TriangleEdge_basic)
{
  TestableVertex v1;
  TestableVertex v2 (1, 0.5);

  TestableEdge edge (&v1, &v2);
  EXPECT_EQ (edge.to_string (), "((0, 0), (1, 0.5))");

  EXPECT_EQ (edge.is_segment (), false);
  edge.set_is_segment (true);
  EXPECT_EQ (edge.is_segment (), true);

  EXPECT_EQ (edge.level (), size_t (0));
  edge.set_level (42);
  EXPECT_EQ (edge.level (), size_t (42));

  EXPECT_EQ (edge.other (&v1) == &v2, true);
  EXPECT_EQ (edge.other (&v2) == &v1, true);
}

TEST(TriangleEdge_triangles)
{
  TestableVertex v1 (0, 0);
  TestableVertex v2 (1, 2);
  TestableVertex v3 (2, 1);
  TestableVertex v4 (-1, 2);

  std::unique_ptr<TestableEdge> e1 (new TestableEdge (&v1, &v2));
  std::unique_ptr<TestableEdge> e2 (new TestableEdge (&v2, &v3));
  std::unique_ptr<TestableEdge> e3 (new TestableEdge (&v3, &v1));

  std::unique_ptr<TestablePolygon> tri (new TestablePolygon (e1.get (), e2.get (), e3.get ()));

  std::unique_ptr<TestableEdge> e4 (new TestableEdge (&v1, &v4));
  std::unique_ptr<TestableEdge> e5 (new TestableEdge (&v2, &v4));
  std::unique_ptr<TestablePolygon> tri2 (new TestablePolygon (e1.get (), e4.get (), e5.get ()));

  EXPECT_EQ (e1->is_outside (), false);
  EXPECT_EQ (e2->is_outside (), true);
  EXPECT_EQ (e4->is_outside (), true);

  EXPECT_EQ (e1->is_for_outside_triangles (), false);
  tri->set_outside (true);
  EXPECT_EQ (e1->is_for_outside_triangles (), true);

  EXPECT_EQ (e1->has_polygon (tri.get ()), true);
  EXPECT_EQ (e1->has_polygon (tri2.get ()), true);
  EXPECT_EQ (e4->has_polygon (tri.get ()), false);
  EXPECT_EQ (e4->has_polygon (tri2.get ()), true);

  EXPECT_EQ (e1->other (tri.get ()) == tri2.get (), true);
  EXPECT_EQ (e1->other (tri2.get ()) == tri.get (), true);

  EXPECT_EQ (e1->common_vertex (e2.get ()) == &v2, true);
  EXPECT_EQ (e2->common_vertex (e4.get ()) == 0, true);

  tri->unlink ();
  EXPECT_EQ (e1->has_polygon (tri.get ()), false);
  EXPECT_EQ (e1->has_polygon (tri2.get ()), true);
}

TEST(TriangleEdge_side_of)
{
  TestableVertex v1;
  TestableVertex v2 (1, 0.5);

  TestableEdge edge (&v1, &v2);
  EXPECT_EQ (edge.to_string (), "((0, 0), (1, 0.5))");

  EXPECT_EQ (edge.side_of (TestableVertex (0, 0)), 0)
  EXPECT_EQ (edge.side_of (TestableVertex (0.5, 0.25)), 0)
  EXPECT_EQ (edge.side_of (TestableVertex (0, 1)), -1)
  EXPECT_EQ (edge.side_of (TestableVertex (0, -1)), 1)
  EXPECT_EQ (edge.side_of (TestableVertex (0.5, 0.5)), -1)
  EXPECT_EQ (edge.side_of (TestableVertex (0.5, 0)), 1)

  TestableVertex v3 (1, 0);
  TestableVertex v4 (0, 1);
  TestableEdge edge2 (&v3, &v4);

  EXPECT_EQ (edge2.side_of (TestableVertex(0.2, 0.2)), -1);
}

namespace {
  class VertexHeap
  {
  public:
    TestableVertex *make_vertex (double x, double y)
    {
      m_heap.push_back (TestableVertex (x, y));
      return &m_heap.back ();
    }
  private:
    std::list<TestableVertex> m_heap;
  };
}

TEST(TriangleEdge_crosses)
{
  VertexHeap heap;

  TestableEdge s1 (heap.make_vertex (0, 0), heap.make_vertex (1, 0.5));
  EXPECT_EQ (s1.crosses (TestableEdge (heap.make_vertex (-1, -0.5), heap.make_vertex(1, -0.5))), false);
  EXPECT_EQ (s1.crosses (TestableEdge (heap.make_vertex (-1, 0), heap.make_vertex(1, 0))), false);  // only cuts
  EXPECT_EQ (s1.crosses (TestableEdge (heap.make_vertex (-1, 0.5), heap.make_vertex(1, 0.5))), false);
  EXPECT_EQ (s1.crosses (TestableEdge (heap.make_vertex (-1, 0.5), heap.make_vertex(2, 0.5))), false);
  EXPECT_EQ (s1.crosses (TestableEdge (heap.make_vertex (-1, 0.25), heap.make_vertex(2, 0.25))), true);
  EXPECT_EQ (s1.crosses (TestableEdge (heap.make_vertex (-1, 0.5), heap.make_vertex(-0.1, 0.5))), false);
  EXPECT_EQ (s1.crosses (TestableEdge (heap.make_vertex (-1, 0.6), heap.make_vertex(0, 0.6))), false);
  EXPECT_EQ (s1.crosses (TestableEdge (heap.make_vertex (-1, 1), heap.make_vertex(1, 1))), false);

  EXPECT_EQ (s1.crosses_including (TestableEdge (heap.make_vertex (-1, -0.5), heap.make_vertex(1, -0.5))), false);
  EXPECT_EQ (s1.crosses_including (TestableEdge (heap.make_vertex (-1, 0), heap.make_vertex(1, 0))), true);  // only cuts
  EXPECT_EQ (s1.crosses_including (TestableEdge (heap.make_vertex (-1, 0.25), heap.make_vertex(2, 0.25))), true);
}

TEST(TriangleEdge_point_on)
{
  VertexHeap heap;

  TestableEdge s1 (heap.make_vertex (0, 0), heap.make_vertex (1, 0.5));
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

  TestableEdge s1 (heap.make_vertex (0, 0), heap.make_vertex (1, 0.5));
  EXPECT_EQ (s1.intersection_point (TestableEdge (heap.make_vertex (-1, 0.25), heap.make_vertex (2, 0.25))).to_string (), "0.5,0.25");
}

TEST(TriangleEdge_can_flip)
{
  TestableVertex v1 (2, -1);
  TestableVertex v2 (0, 0);
  TestableVertex v3 (1, 0);
  TestableVertex v4 (0.5, 1);
  TestableEdge s1 (&v1, &v2);
  TestableEdge s2 (&v1, &v3);
  TestableEdge s3 (&v2, &v3);
  TestableEdge s4 (&v2, &v4);
  TestableEdge s5 (&v3, &v4);
  TestablePolygon t1 (&s1, &s2, &s3);
  TestablePolygon t2 (&s3, &s4, &s5);
  EXPECT_EQ (s3.left () == &t2, true);
  EXPECT_EQ (s3.right () == &t1, true);
  EXPECT_EQ (s3.can_flip(), false);
  v1.set_x (0.5);
  EXPECT_EQ (s3.can_flip(), true);
  v1.set_x (-0.25);
  EXPECT_EQ (s3.can_flip(), true);
  v1.set_x (-0.5);
  EXPECT_EQ (s3.can_flip(), false);
  v1.set_x (-1.0);
  EXPECT_EQ (s3.can_flip(), false);
}

TEST(TriangleEdge_distance)
{
  TestableVertex v1 (0, 0);
  TestableVertex v2 (1, 0);

  TestableEdge seg (&v1, &v2);
  EXPECT_EQ (seg.distance (db::DPoint (0, 0)), 0);
  EXPECT_EQ (seg.distance (db::DPoint (0, 1)), 1);
  EXPECT_EQ (seg.distance (db::DPoint (1, 2)), 2);
  EXPECT_EQ (seg.distance (db::DPoint (1, -1)), 1);
  EXPECT_EQ (seg.distance (db::DPoint (2, 0)), 1);
  EXPECT_EQ (seg.distance (db::DPoint (-2, 0)), 2);
  seg.reverse ();
  EXPECT_EQ (seg.distance (db::DPoint (0, 0)), 0);
  EXPECT_EQ (seg.distance (db::DPoint (0, 1)), 1);
  EXPECT_EQ (seg.distance (db::DPoint (1, 2)), 2);
  EXPECT_EQ (seg.distance (db::DPoint (1, -1)), 1);
  EXPECT_EQ (seg.distance (db::DPoint (2, 0)), 1);
  EXPECT_EQ (seg.distance (db::DPoint (-2, 0)), 2);
}
