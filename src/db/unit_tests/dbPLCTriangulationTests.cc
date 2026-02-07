
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


#include "dbPLCTriangulation.h"
#include "dbWriter.h"
#include "dbRegionProcessors.h"
#include "tlUnitTest.h"
#include "tlStream.h"
#include "tlFileUtils.h"

#include <set>
#include <vector>
#include <cstdlib>
#include <cmath>

namespace
{

class TestableTriangulation
  : public db::plc::Triangulation
{
public:
  using db::plc::Triangulation::Triangulation;
  using db::plc::Triangulation::check;
  using db::plc::Triangulation::flip;
  using db::plc::Triangulation::insert_point;
  using db::plc::Triangulation::search_edges_crossing;
  using db::plc::Triangulation::find_edge_for_points;
  using db::plc::Triangulation::find_points_around;
  using db::plc::Triangulation::find_inside_circle;
  using db::plc::Triangulation::create_constrained_delaunay;
  using db::plc::Triangulation::is_illegal_edge;
  using db::plc::Triangulation::find_vertex_for_point;
  using db::plc::Triangulation::remove;
  using db::plc::Triangulation::ensure_edge;
  using db::plc::Triangulation::constrain;
  using db::plc::Triangulation::remove_outside_triangles;
};

class TestableGraph
  : public db::plc::Graph
{
public:
  using db::plc::Graph::Graph;
  using db::plc::Graph::create_vertex;
  using db::plc::Graph::create_edge;
  using db::plc::Graph::create_triangle;
};

}

TEST(basic)
{
  db::plc::Graph plc;
  TestableTriangulation tris (&plc);
  tris.init_box (db::DBox (1, 0, 5, 4));

  EXPECT_EQ (plc.bbox ().to_string (), "(1,0;5,4)");
  EXPECT_EQ (plc.to_string (), "((1, 0), (1, 4), (5, 0)), ((1, 4), (5, 4), (5, 0))");

  EXPECT_EQ (tris.check (), true);

  tris.clear ();

  EXPECT_EQ (plc.bbox ().to_string (), "()");
  EXPECT_EQ (plc.to_string (), "");

  EXPECT_EQ (tris.check (), true);
}

TEST(flip)
{
  db::plc::Graph plc;
  TestableTriangulation tris (&plc);
  tris.init_box (db::DBox (0, 0, 1, 1));
  EXPECT_EQ (plc.to_string (), "((0, 0), (0, 1), (1, 0)), ((0, 1), (1, 1), (1, 0))");

  EXPECT_EQ (plc.num_polygons (), size_t (2));
  EXPECT_EQ (tris.check (), true);

  const db::plc::Polygon &t1 = *plc.begin ();
  db::plc::Edge *diag_segment;
  for (int i = 0; i < 3; ++i) {
    diag_segment = t1.edge (i);
    if (diag_segment->side_of (db::DPoint (0.5, 0.5)) == 0) {
      break;
    }
  }
  tris.flip (diag_segment);
  EXPECT_EQ (plc.to_string (), "((1, 1), (0, 0), (0, 1)), ((1, 1), (1, 0), (0, 0))");
  EXPECT_EQ (tris.check (), true);
}

TEST(insert)
{
  db::plc::Graph plc;
  TestableTriangulation tris (&plc);
  tris.init_box (db::DBox (0, 0, 1, 1));

  tris.insert_point (0.2, 0.2);
  EXPECT_EQ (plc.to_string (), "((0, 0), (0, 1), (0.2, 0.2)), ((1, 0), (0, 0), (0.2, 0.2)), ((1, 1), (0.2, 0.2), (0, 1)), ((1, 1), (1, 0), (0.2, 0.2))");
  EXPECT_EQ (tris.check (), true);
}

TEST(split_segment)
{
  db::plc::Graph plc;
  TestableTriangulation tris (&plc);
  tris.init_box (db::DBox (0, 0, 1, 1));

  tris.insert_point (0.5, 0.5);
  EXPECT_EQ (plc.to_string (), "((1, 1), (1, 0), (0.5, 0.5)), ((1, 1), (0.5, 0.5), (0, 1)), ((0, 0), (0, 1), (0.5, 0.5)), ((0, 0), (0.5, 0.5), (1, 0))");
  EXPECT_EQ (tris.check(), true);
}

TEST(insert_vertex_twice)
{
  db::plc::Graph plc;
  TestableTriangulation tris (&plc);
  tris.init_box (db::DBox (0, 0, 1, 1));

  tris.insert_point (0.5, 0.5);
  //  inserted a vertex twice does not change anything
  tris.insert_point (0.5, 0.5);
  EXPECT_EQ (plc.to_string (), "((1, 1), (1, 0), (0.5, 0.5)), ((1, 1), (0.5, 0.5), (0, 1)), ((0, 0), (0, 1), (0.5, 0.5)), ((0, 0), (0.5, 0.5), (1, 0))");
  EXPECT_EQ (tris.check(), true);
}

TEST(collect_vertexes)
{
  db::plc::Graph plc;
  TestableTriangulation tris (&plc);
  tris.init_box (db::DBox (0, 0, 1, 1));
  tris.insert_point (0.2, 0.2);
  tris.insert_point (0.5, 0.5);

  std::vector<db::plc::Vertex *> vertexes = tris.find_vertexes_along_line (db::DPoint (0, 0), db::DPoint (1.5, 1.5));
  EXPECT_EQ (vertexes.size (), size_t (4));
  if (vertexes.size () >= size_t (4)) {
    EXPECT_EQ (vertexes [0]->to_string (), "(0, 0)");
    EXPECT_EQ (vertexes [1]->to_string (), "(0.2, 0.2)");
    EXPECT_EQ (vertexes [2]->to_string (), "(0.5, 0.5)");
    EXPECT_EQ (vertexes [3]->to_string (), "(1, 1)");
  }

  vertexes = tris.find_vertexes_along_line (db::DPoint (0, 0), db::DPoint (1.0, 1.0));
  EXPECT_EQ (vertexes.size (), size_t (4));
  if (vertexes.size () >= size_t (4)) {
    EXPECT_EQ (vertexes [0]->to_string (), "(0, 0)");
    EXPECT_EQ (vertexes [1]->to_string (), "(0.2, 0.2)");
    EXPECT_EQ (vertexes [2]->to_string (), "(0.5, 0.5)");
    EXPECT_EQ (vertexes [3]->to_string (), "(1, 1)");
  }

  vertexes = tris.find_vertexes_along_line (db::DPoint (1, 1), db::DPoint (0.25, 0.25));
  EXPECT_EQ (vertexes.size (), size_t (2));
  if (vertexes.size () >= size_t (2)) {
    EXPECT_EQ (vertexes [0]->to_string (), "(1, 1)");
    EXPECT_EQ (vertexes [1]->to_string (), "(0.5, 0.5)");
  }
}

TEST(insert_vertex_convex)
{
  db::plc::Graph plc;
  TestableTriangulation tris (&plc);
  tris.insert_point (0.2, 0.2);
  tris.insert_point (0.2, 0.8);
  tris.insert_point (0.6, 0.5);
  tris.insert_point (0.7, 0.5);
  tris.insert_point (0.6, 0.4);
  EXPECT_EQ (plc.to_string (), "((0.2, 0.2), (0.2, 0.8), (0.6, 0.5)), ((0.2, 0.8), (0.7, 0.5), (0.6, 0.5)), ((0.6, 0.4), (0.6, 0.5), (0.7, 0.5)), ((0.6, 0.4), (0.2, 0.2), (0.6, 0.5))");
  EXPECT_EQ (tris.check(), true);
}

TEST(insert_vertex_convex2)
{
  db::plc::Graph plc;
  TestableTriangulation tris (&plc);
  tris.insert_point (0.25, 0.1);
  tris.insert_point (0.1, 0.4);
  tris.insert_point (0.4, 0.15);
  tris.insert_point (1, 0.7);
  EXPECT_EQ (plc.to_string (), "((0.25, 0.1), (0.1, 0.4), (0.4, 0.15)), ((1, 0.7), (0.4, 0.15), (0.1, 0.4))");
  EXPECT_EQ (tris.check(), true);
}

TEST(insert_vertex_convex3)
{
  db::plc::Graph plc;
  TestableTriangulation tris (&plc);
  tris.insert_point (0.25, 0.5);
  tris.insert_point (0.25, 0.55);
  tris.insert_point (0.15, 0.8);
  tris.insert_point (1, 0.4);
  EXPECT_EQ (plc.to_string (), "((0.25, 0.5), (0.15, 0.8), (0.25, 0.55)), ((1, 0.4), (0.25, 0.5), (0.25, 0.55)), ((0.15, 0.8), (1, 0.4), (0.25, 0.55))");
  EXPECT_EQ (tris.check(), true);
}

TEST(search_edges_crossing)
{
  db::plc::Graph plc;
  TestableTriangulation tris (&plc);
  db::plc::Vertex *v1 = tris.insert_point (0.2, 0.2);
  db::plc::Vertex *v2 = tris.insert_point (0.2, 0.8);
  db::plc::Vertex *v3 = tris.insert_point (0.6, 0.5);
  /*db::plc::Vertex *v4 =*/ tris.insert_point (0.7, 0.5);
  db::plc::Vertex *v5 = tris.insert_point (0.6, 0.4);
  db::plc::Vertex *v6 = tris.insert_point (0.7, 0.2);
  EXPECT_EQ (tris.check(), true);

  auto xedges = tris.search_edges_crossing (v2, v6);

  EXPECT_EQ (xedges.size (), size_t (2));
  auto s1 = tris.find_edge_for_points (*v1, *v3);
  auto s2 = tris.find_edge_for_points (*v1, *v5);
  EXPECT_EQ (std::find (xedges.begin (), xedges.end (), s1) != xedges.end (), true);
  EXPECT_EQ (std::find (xedges.begin (), xedges.end (), s2) != xedges.end (), true);
}

TEST(illegal_edge1)
{
  TestableGraph plc;

  db::plc::Vertex *v1 = plc.create_vertex (0, 0);
  db::plc::Vertex *v2 = plc.create_vertex (1.6, 1.6);
  db::plc::Vertex *v3 = plc.create_vertex (1, 2);
  db::plc::Vertex *v4 = plc.create_vertex (2, 1);

  {
    db::plc::Edge *e1 = plc.create_edge (v1, v3);
    db::plc::Edge *e2 = plc.create_edge (v3, v4);
    db::plc::Edge *e3 = plc.create_edge (v4, v1);

    plc.create_triangle (e1, e2, e3);

    db::plc::Edge *ee1 = plc.create_edge (v2, v3);
    db::plc::Edge *ee2 = plc.create_edge (v4, v2);

    plc.create_triangle (ee1, e2, ee2);

    EXPECT_EQ (TestableTriangulation::is_illegal_edge (e2), true);
  }

  {
    //  flipped
    db::plc::Edge *e1 = plc.create_edge (v1, v2);
    db::plc::Edge *e2 = plc.create_edge  (v2, v3);
    db::plc::Edge *e3 = plc.create_edge  (v3, v1);

    plc.create_triangle  (e1, e2, e3);

    db::plc::Edge *ee1 = plc.create_edge (v1, v4);
    db::plc::Edge *ee2 = plc.create_edge  (v4, v2);

    plc.create_triangle (ee1, ee2, e1);

    EXPECT_EQ (TestableTriangulation::is_illegal_edge (e2), false);
  }
}

TEST(illegal_edge2)
{
  TestableGraph plc;

  //  numerical border case
  db::plc::Vertex *v1 = plc.create_vertex (773.94756216690905, 114.45875269431208);
  db::plc::Vertex *v2 = plc.create_vertex (773.29574734131643, 113.47402096138073);
  db::plc::Vertex *v3 = plc.create_vertex (773.10652961562653, 114.25497975904504);
  db::plc::Vertex *v4 = plc.create_vertex (774.08856345337881, 113.60495072750861);

  {
    db::plc::Edge *e1 = plc.create_edge (v1, v2);
    db::plc::Edge *e2 = plc.create_edge (v2, v4);
    db::plc::Edge *e3 = plc.create_edge (v4, v1);

    plc.create_triangle (e1, e2, e3);

    db::plc::Edge *ee1 = plc.create_edge (v2, v3);
    db::plc::Edge *ee2 = plc.create_edge (v3, v4);

    plc.create_triangle (ee1, ee2, e2);

    EXPECT_EQ (TestableTriangulation::is_illegal_edge (e2), false);
  }

  {
    //  flipped
    db::plc::Edge *e1 = plc.create_edge (v1, v2);
    db::plc::Edge *e2 = plc.create_edge (v2, v3);
    db::plc::Edge *e3 = plc.create_edge (v3, v1);

    plc.create_triangle (e1, e2, e3);

    db::plc::Edge *ee1 = plc.create_edge (v1, v4);
    db::plc::Edge *ee2 = plc.create_edge (v4, v2);

    plc.create_triangle (ee1, ee2, e1);

    EXPECT_EQ (TestableTriangulation::is_illegal_edge (e1), false);
  }
}

//  Returns a random float number between 0.0 and 1.0
inline double flt_rand ()
{
  return rand () * (1.0 / double (RAND_MAX));
}

namespace {
  struct PointLessOp
  {
    bool operator() (const db::DPoint &a, const db::DPoint &b) const
    {
      return a.less (b);
    }
  };
}

TEST(insert_many)
{
  srand (0);

  db::plc::Graph plc;
  TestableTriangulation tris (&plc);
  double res = 65536.0;

  db::DBox bbox;

  unsigned int n = 200000;
  for (unsigned int i = 0; i < n; ++i) {
    double x = round (flt_rand () * res) * 0.0001;
    double y = round (flt_rand () * res) * 0.0001;
    tris.insert_point (x, y);
  }

  //  slow: EXPECT_EQ (tris.check (), true);
  EXPECT_LT (double (tris.flips ()) / double (n), 3.1);
  EXPECT_LT (double (tris.hops ()) / double (n), 23.0);
}

TEST(heavy_insert)
{
  tl::info << "Running test_heavy_insert " << tl::noendl;

  for (unsigned int l = 0; l < 100; ++l) {

    srand (l);
    tl::info << "." << tl::noendl;

    db::plc::Graph plc;
    TestableTriangulation tris (&plc);
    double res = 128.0;

    unsigned int n = rand () % 190 + 10;

    db::DBox bbox;
    std::map<db::DPoint, bool, PointLessOp> vmap;

    for (unsigned int i = 0; i < n; ++i) {
      double x = round (flt_rand () * res) * (1.0 / res);
      double y = round (flt_rand () * res) * (1.0 / res);
      db::plc::Vertex *v = tris.insert_point (x, y);
      bbox += db::DPoint (x, y);
      vmap.insert (std::pair<db::DPoint, bool> (*v, false));
    }

    //  not strictly true, but very likely with at least 10 vertexes:
    EXPECT_GT (plc.num_polygons (), size_t (0));
    EXPECT_EQ (plc.bbox ().to_string (), bbox.to_string ());

    bool ok = true;
    for (auto t = plc.begin (); t != plc.end (); ++t) {
      for (int i = 0; i < 3; ++i) {
        auto f = vmap.find (*t->vertex (i));
        if (f == vmap.end ()) {
          tl::error << "Could not identify triangle vertex " << t->vertex (i)->to_string () << " as inserted vertex";
          ok = false;
        } else {
          f->second = true;
        }
      }
    }
    for (auto m = vmap.begin (); m != vmap.end (); ++m) {
      if (!m->second) {
        tl::error << "Could not identify vertex " << m->first.to_string () << " with a triangle";
        ok = false;
      }
    }
    EXPECT_EQ (ok, true);

    EXPECT_EQ (tris.check(), true);

  }

  tl::info << tl::endl << "done.";
}

TEST(heavy_remove)
{
  tl::info << "Running test_heavy_remove " << tl::noendl;

  for (unsigned int l = 0; l < 100; ++l) {

    srand (l);
    tl::info << "." << tl::noendl;

    db::plc::Graph plc;
    TestableTriangulation tris (&plc);
    double res = 128.0;

    unsigned int n = rand () % 190 + 10;

    for (unsigned int i = 0; i < n; ++i) {
      double x = round (flt_rand () * res) * (1.0 / res);
      double y = round (flt_rand () * res) * (1.0 / res);
      tris.insert_point (x, y);
    }

    EXPECT_EQ (tris.check(), true);

    std::set<db::plc::Vertex *> vset;
    std::vector<db::plc::Vertex *> vertexes;
    for (auto t = plc.begin (); t != plc.end (); ++t) {
      for (int i = 0; i < 3; ++i) {
        db::plc::Vertex *v = t->vertex (i);
        if (vset.insert (v).second) {
          vertexes.push_back (v);
        }
      }
    }

    while (! vertexes.empty ()) {

      unsigned int n = rand () % (unsigned int) vertexes.size ();
      db::plc::Vertex *v = vertexes [n];
      tris.remove (v);
      vertexes.erase (vertexes.begin () + n);

      //  just a few times as it wastes time otherwise
      if (vertexes.size () % 10 == 0) {
        EXPECT_EQ (tris.check (), true);
      }

    }

    EXPECT_EQ (plc.num_polygons (), size_t (0));

  }

  tl::info << tl::endl << "done.";
}

TEST(ensure_edge)
{
  srand (0);

  db::plc::Graph plc;
  TestableTriangulation tris (&plc);
  double res = 128.0;

  db::DEdge ee[] = {
    db::DEdge (0.25, 0.25, 0.25, 0.75),
    db::DEdge (0.25, 0.75, 0.75, 0.75),
    db::DEdge (0.75, 0.75, 0.75, 0.25),
    db::DEdge (0.75, 0.25, 0.25, 0.25)
  };

  for (unsigned int i = 0; i < 200; ++i) {
    double x = round (flt_rand () * res) * (1.0 / res);
    double y = round (flt_rand () * res) * (1.0 / res);
    bool ok = true;
    for (unsigned int j = 0; j < sizeof (ee) / sizeof (ee[0]); ++j) {
      if (ee[j].side_of (db::DPoint (x, y)) == 0) {
        --i;
        ok = false;
      }
    }
    if (ok) {
      tris.insert_point (x, y);
    }
  }

  for (unsigned int i = 0; i < sizeof (ee) / sizeof (ee[0]); ++i) {
    tris.insert_point (ee[i].p1 ());
  }

  EXPECT_EQ (tris.check (), true);

  for (unsigned int i = 0; i < sizeof (ee) / sizeof (ee[0]); ++i) {
    tris.ensure_edge (tris.find_vertex_for_point (ee[i].p1 ()), tris.find_vertex_for_point (ee[i].p2 ()));
  }

  EXPECT_EQ (tris.check (false), true);

  double area_in = 0.0;
  db::DBox clip_box;
  for (unsigned int i = 0; i < sizeof (ee) / sizeof (ee[0]); ++i) {
    clip_box += ee[i].p1 ();
  }
  for (auto t = plc.begin (); t != plc.end (); ++t) {
    if (clip_box.overlaps (t->bbox ())) {
      EXPECT_EQ (t->bbox ().inside (clip_box), true);
      area_in += t->area ();
    }
  }

  EXPECT_EQ (tl::to_string (area_in), "0.25");
}

static bool safe_inside (const db::DBox &b1, const db::DBox &b2)
{
  typedef db::coord_traits<db::DBox::coord_type> ct;

  return (ct::less (b2.left (), b1.left ())     || ct::equal (b2.left (), b1.left ())) &&
         (ct::less (b1.right (), b2.right ())   || ct::equal (b1.right (), b2.right ())) &&
         (ct::less (b2.bottom (), b1.bottom ()) || ct::equal (b2.bottom (), b1.bottom ())) &&
         (ct::less (b1.top (), b2.top ())       || ct::equal (b1.top (), b2.top ()));
}

TEST(constrain)
{
  srand (0);

  db::plc::Graph plc;
  TestableTriangulation tris (&plc);
  double res = 128.0;

  db::DEdge ee[] = {
    db::DEdge (0.25, 0.25, 0.25, 0.75),
    db::DEdge (0.25, 0.75, 0.75, 0.75),
    db::DEdge (0.75, 0.75, 0.75, 0.25),
    db::DEdge (0.75, 0.25, 0.25, 0.25)
  };

  for (unsigned int i = 0; i < 200; ++i) {
    double x = round (flt_rand () * res) * (1.0 / res);
    double y = round (flt_rand () * res) * (1.0 / res);
    bool ok = true;
    for (unsigned int j = 0; j < sizeof (ee) / sizeof (ee[0]); ++j) {
      if (ee[j].side_of (db::DPoint (x, y)) == 0) {
        --i;
        ok = false;
      }
    }
    if (ok) {
      tris.insert_point (x, y);
    }
  }

  std::vector<db::plc::Vertex *> contour;
  for (unsigned int i = 0; i < sizeof (ee) / sizeof (ee[0]); ++i) {
    contour.push_back (tris.insert_point (ee[i].p1 ()));
  }
  std::vector<std::vector<db::plc::Vertex *> > contours;
  contours.push_back (contour);

  EXPECT_EQ (tris.check (), true);

  tris.constrain (contours);
  EXPECT_EQ (tris.check (false), true);

  tris.remove_outside_triangles ();

  EXPECT_EQ (tris.check (), true);

  double area_in = 0.0;
  db::DBox clip_box;
  for (unsigned int i = 0; i < sizeof (ee) / sizeof (ee[0]); ++i) {
    clip_box += ee[i].p1 ();
  }
  for (auto t = plc.begin (); t != plc.end (); ++t) {
    EXPECT_EQ (clip_box.overlaps (t->bbox ()), true);
    EXPECT_EQ (safe_inside (t->bbox (), clip_box), true);
    area_in += t->area ();
  }

  EXPECT_EQ (tl::to_string (area_in), "0.25");
}

TEST(heavy_constrain)
{
  tl::info << "Running test_heavy_constrain " << tl::noendl;

  for (unsigned int l = 0; l < 100; ++l) {

    srand (l);
    tl::info << "." << tl::noendl;

    db::plc::Graph plc;
    TestableTriangulation tris (&plc);
    double res = 128.0;

    db::DEdge ee[] = {
      db::DEdge (0.25, 0.25, 0.25, 0.75),
      db::DEdge (0.25, 0.75, 0.75, 0.75),
      db::DEdge (0.75, 0.75, 0.75, 0.25),
      db::DEdge (0.75, 0.25, 0.25, 0.25)
    };

    unsigned int n = rand () % 150 + 50;

    for (unsigned int i = 0; i < n; ++i) {
      double x = round (flt_rand () * res) * (1.0 / res);
      double y = round (flt_rand () * res) * (1.0 / res);
      bool ok = true;
      for (unsigned int j = 0; j < sizeof (ee) / sizeof (ee[0]); ++j) {
        if (ee[j].side_of (db::DPoint (x, y)) == 0) {
          --i;
          ok = false;
        }
      }
      if (ok) {
        tris.insert_point (x, y);
      }
    }

    std::vector<db::plc::Vertex *> contour;
    for (unsigned int i = 0; i < sizeof (ee) / sizeof (ee[0]); ++i) {
      contour.push_back (tris.insert_point (ee[i].p1 ()));
    }
    std::vector<std::vector<db::plc::Vertex *> > contours;
    contours.push_back (contour);

    EXPECT_EQ (tris.check (), true);

    tris.constrain (contours);
    EXPECT_EQ (tris.check (false), true);

    tris.remove_outside_triangles ();

    EXPECT_EQ (tris.check (), true);

    double area_in = 0.0;
    db::DBox clip_box;
    for (unsigned int i = 0; i < sizeof (ee) / sizeof (ee[0]); ++i) {
      clip_box += ee[i].p1 ();
    }
    for (auto t = plc.begin (); t != plc.end (); ++t) {
      EXPECT_EQ (clip_box.overlaps (t->bbox ()), true);
      EXPECT_EQ (safe_inside (t->bbox (), clip_box), true);
      area_in += t->area ();
    }

    EXPECT_EQ (tl::to_string (area_in), "0.25");

  }

  tl::info << tl::endl << "done.";
}

TEST(heavy_find_point_around)
{
  tl::info << "Running Triangle_test_heavy_find_point_around " << tl::noendl;

  for (unsigned int l = 0; l < 100; ++l) {

    srand (l);
    tl::info << "." << tl::noendl;

    db::plc::Graph plc;
    TestableTriangulation tris (&plc);
    double res = 128.0;

    unsigned int n = rand () % 190 + 10;

    std::vector<db::plc::Vertex *> vertexes;

    for (unsigned int i = 0; i < n; ++i) {
      double x = round (flt_rand () * res) * (1.0 / res);
      double y = round (flt_rand () * res) * (1.0 / res);
      vertexes.push_back (tris.insert_point (x, y));
    }

    EXPECT_EQ (tris.check(), true);

    for (int i = 0; i < 100; ++i) {

      unsigned int nv = rand () % (unsigned int) vertexes.size ();
      auto vertex = vertexes [nv];

      double r = round (flt_rand () * res) * (1.0 / res);
      auto p1 = tris.find_points_around (vertex, r);
      auto p2 = tris.find_inside_circle (*vertex, r);

      std::set<db::plc::Vertex *> sp1 (p1.begin (), p1.end ());
      std::set<db::plc::Vertex *> sp2 (p2.begin (), p2.end ());
      sp2.erase (vertex);

      EXPECT_EQ (sp1 == sp2, true);

    }

  }

  tl::info << tl::endl << "done.";
}

TEST(create_constrained_delaunay)
{
  db::Region r;
  r.insert (db::Box (0, 0, 1000, 1000));

  db::Region r2;
  r2.insert (db::Box (200, 200, 800, 800));

  r -= r2;

  db::plc::Graph plc;
  TestableTriangulation tris (&plc);
  tris.create_constrained_delaunay (r);
  tris.remove_outside_triangles ();

  EXPECT_EQ (tris.check (), true);

  EXPECT_EQ (plc.to_string (),
             "((1000, 0), (0, 0), (200, 200)), "
             "((0, 1000), (200, 200), (0, 0)), "
             "((1000, 0), (200, 200), (800, 200)), "
             "((1000, 0), (800, 200), (1000, 1000)), "
             "((800, 200), (800, 800), (1000, 1000)), "
             "((0, 1000), (1000, 1000), (800, 800)), "
             "((0, 1000), (800, 800), (200, 800)), "
             "((0, 1000), (200, 800), (200, 200))");
}

TEST(triangulate_basic)
{
  db::Region r;
  r.insert (db::Box (0, 0, 10000, 10000));

  db::Region r2;
  r2.insert (db::Box (2000, 2000, 8000, 8000));

  r -= r2;

  db::plc::TriangulationParameters param;
  param.min_b = 1.2;
  param.max_area = 1.0;

  db::plc::Graph plc;
  TestableTriangulation tri (&plc);
  tri.triangulate (r, param, 0.001);

  EXPECT_EQ (tri.check (), true);

  for (auto t = plc.begin (); t != plc.end (); ++t) {
    EXPECT_LE (t->area (), param.max_area);
    EXPECT_GE (t->b (), param.min_b);
  }

  EXPECT_GT (plc.num_polygons (), size_t (100));
  EXPECT_LT (plc.num_polygons (), size_t (150));

  //  for debugging:
  //  tri.dump ("debug.gds");

  param.min_b = 1.0;
  param.max_area = 0.1;

  tri.triangulate (r, param, 0.001);

  EXPECT_EQ (tri.check (), true);

  for (auto t = plc.begin (); t != plc.end (); ++t) {
    EXPECT_LE (t->area (), param.max_area);
    EXPECT_GE (t->b (), param.min_b);
  }

  EXPECT_GT (plc.num_polygons (), size_t (900));
  EXPECT_LT (plc.num_polygons (), size_t (1000));
}

static void read_polygons (const std::string &path, db::Region &region, double dbu)
{
  tl::InputStream is (path);
  tl::TextInputStream ti (is);

  unsigned int nvert = 0, nedges = 0;

  {
    tl::Extractor ex (ti.get_line ().c_str ());
    ex.read (nvert);
    ex.read (nedges);
  }

  std::vector<db::Point> v;
  auto dbu_trans = db::CplxTrans (dbu).inverted ();
  for (unsigned int i = 0; i < nvert; ++i) {
    double x = 0, y = 0;
    tl::Extractor ex (ti.get_line ().c_str ());
    ex.read (x);
    ex.read (y);
    v.push_back (dbu_trans * db::DPoint (x, y));
  }

  unsigned int nstart = 0;
  bool new_contour = true;
  std::vector<db::Point> contour;

  for (unsigned int i = 0; i < nedges; ++i) {

    unsigned int n1 = 0, n2 = 0;

    tl::Extractor ex (ti.get_line ().c_str ());
    ex.read (n1);
    ex.read (n2);

    if (new_contour) {
      nstart = n1;
      new_contour = false;
    }

    contour.push_back (v[n1]);

    if (n2 == nstart) {
      //  finish contour
      db::SimplePolygon sp;
      sp.assign_hull (contour.begin (), contour.end ());
      region.insert (sp);
      new_contour = true;
      contour.clear ();
    } else if (n2 <= n1) {
      tl::error << "Invalid polygon wrap in line " << ti.line_number ();
      tl_assert (false);
    }

  }
}

TEST(triangulate_geo)
{
  double dbu = 0.001;

  db::Region r;
  read_polygons (tl::combine_path (tl::testsrc (), "testdata/algo/triangles1.txt"), r, dbu);

  //  for debugging purposes dump the inputs
  if (false) {

    db::Layout layout = db::Layout ();
    layout.dbu (dbu);
    db::Cell &top = layout.cell (layout.add_cell ("DUMP"));
    unsigned int l1 = layout.insert_layer (db::LayerProperties (1, 0));
    r.insert_into (&layout, top.cell_index (), l1);

    {
      tl::OutputStream stream ("input.gds");
      db::SaveLayoutOptions opt;
      db::Writer writer (opt);
      writer.write (layout, stream);
    }

  }

  db::plc::TriangulationParameters param;
  param.min_b = 1.0;
  param.max_area = 0.1;
  param.min_length = 0.001;

  db::plc::Graph plc;
  TestableTriangulation tri (&plc);
  tri.triangulate (r, param, dbu);

  EXPECT_EQ (tri.check (false), true);

  //  for debugging:
  //  tri.dump ("debug.gds");

  size_t n_skinny = 0;
  for (auto t = plc.begin (); t != plc.end (); ++t) {
    EXPECT_LE (t->area (), param.max_area);
    if (t->b () < param.min_b) {
      ++n_skinny;
    }
  }

  EXPECT_LT (n_skinny, size_t (20));
  EXPECT_GT (plc.num_polygons (), size_t (29000));
  EXPECT_LT (plc.num_polygons (), size_t (30000));
}

TEST(triangulate_analytic)
{
  double dbu = 0.0001;

  double star1 = 9.0, star2 = 5.0;
  double r = 1.0;
  int n = 100;

  auto dbu_trans = db::CplxTrans (dbu).inverted ();

  std::vector <db::Point> contour1, contour2;
  for (int i = 0; i < n; ++i) {
    double a = -M_PI * 2.0 * double (i) / double (n);  //  "-" for clockwise orientation
    double rr, x, y;
    rr = r * (1.0 + 0.4 * cos (star1 * a));
    x = rr * cos (a);
    y = rr * sin (a);
    contour1.push_back (dbu_trans * db::DPoint (x, y));
    rr = r * (0.1 + 0.03 * cos (star2 * a));
    x = rr * cos (a);
    y = rr * sin (a);
    contour2.push_back (dbu_trans * db::DPoint (x, y));
  }

  db::Region rg;

  db::SimplePolygon sp1;
  sp1.assign_hull (contour1.begin (), contour1.end ());
  db::SimplePolygon sp2;
  sp2.assign_hull (contour2.begin (), contour2.end ());

  rg = db::Region (sp1) - db::Region (sp2);

  db::plc::TriangulationParameters param;
  param.min_b = 1.0;
  param.max_area = 0.01;

  db::plc::Graph plc;
  TestableTriangulation tri (&plc);
  tri.triangulate (rg, param, dbu);

  EXPECT_EQ (tri.check (false), true);

  //  for debugging:
  //  tri.dump ("debug.gds");

  for (auto t = plc.begin (); t != plc.end (); ++t) {
    EXPECT_LE (t->area (), param.max_area);
    EXPECT_GE (t->b (), param.min_b);
  }

  EXPECT_GT (plc.num_polygons (), size_t (1250));
  EXPECT_LT (plc.num_polygons (), size_t (1300));
}

TEST(triangulate_problematic)
{
  db::DPoint contour[] = {
    db::DPoint (129145.00000, -30060.80000),
    db::DPoint (129145.00000, -28769.50000),
    db::DPoint (129159.50000, -28754.90000), //  this is a very short edge  <-- from here.
    db::DPoint (129159.60000, -28754.80000), //  <-- to here.
    db::DPoint (129159.50000, -28754.70000),
    db::DPoint (129366.32200, -28547.90000),
    db::DPoint (130958.54600, -26955.84600),
    db::DPoint (131046.25000, -27043.55000),
    db::DPoint (130152.15000, -27937.65000),
    db::DPoint (130152.15000, -30060.80000)
  };

  db::DPolygon poly;
  poly.assign_hull (contour + 0, contour + sizeof (contour) / sizeof (contour[0]));

  db::plc::TriangulationParameters param;
  param.min_b = 1.0;
  param.max_area = 100000.0;
  param.min_length = 0.002;

  db::plc::Graph plc;
  TestableTriangulation tri (&plc);
  tri.triangulate (poly, param);

  EXPECT_EQ (tri.check (false), true);

  //  for debugging:
  //  tri.dump ("debug.gds");

  for (auto t = plc.begin (); t != plc.end (); ++t) {
    EXPECT_LE (t->area (), param.max_area);
    EXPECT_GE (t->b (), param.min_b);
  }

  EXPECT_GT (plc.num_polygons (), size_t (540));
  EXPECT_LT (plc.num_polygons (), size_t (560));
}

TEST(triangulate_thin)
{
  db::DPoint contour[] = {
    db::DPoint (18790, 58090),
    db::DPoint (18790, 58940),
    db::DPoint (29290, 58940),
    db::DPoint (29290, 58090)
  };

  db::DPoint hole[] = {
    db::DPoint (18791, 58091),
    db::DPoint (29289, 58091),
    db::DPoint (29289, 58939),
    db::DPoint (18791, 58939)
  };

  db::DPolygon poly;
  poly.assign_hull (contour + 0, contour + sizeof (contour) / sizeof (contour[0]));
  poly.insert_hole (hole + 0, hole + sizeof (hole) / sizeof (hole[0]));

  double dbu = 0.001;

  db::plc::TriangulationParameters param;
  param.min_b = 0.5;
  param.max_area = 0.0;
  param.min_length = 2 * dbu;

  db::plc::Graph plc;
  TestableTriangulation tri (&plc);
  db::DCplxTrans trans = db::DCplxTrans (dbu) * db::DCplxTrans (db::DTrans (db::DPoint () - poly.box ().center ()));
  tri.triangulate (trans * poly, param);

  EXPECT_EQ (tri.check (false), true);

  //  for debugging:
  //  tri.dump ("debug.gds");

  for (auto t = plc.begin (); t != plc.end (); ++t) {
    EXPECT_GE (t->b (), param.min_b);
  }

  EXPECT_GT (plc.num_polygons (), size_t (13000));
  EXPECT_LT (plc.num_polygons (), size_t (13200));
}

TEST(triangulate_issue1996)
{
  db::DPoint contour[] = {
    db::DPoint (-8000, -8075),
    db::DPoint (-8000, 8075),
    db::DPoint (18000, 8075),
    db::DPoint (18000, -8075)
  };

  db::DPolygon poly;
  poly.assign_hull (contour + 0, contour + sizeof (contour) / sizeof (contour[0]));

  double dbu = 0.001;

  db::plc::TriangulationParameters param;
  param.min_b = 0.5;
  param.max_area = 5000.0 * dbu * dbu;

  db::plc::Graph plc;
  TestableTriangulation tri (&plc);
  db::DCplxTrans trans = db::DCplxTrans (dbu) * db::DCplxTrans (db::DTrans (db::DPoint () - poly.box ().center ()));
  tri.triangulate (trans * poly, param);

  EXPECT_EQ (tri.check (false), true);

  //  for debugging:
  //  tri.dump ("debug.gds");

  for (auto t = plc.begin (); t != plc.end (); ++t) {
    EXPECT_LE (t->area (), param.max_area);
    EXPECT_GE (t->b (), param.min_b);
  }

  EXPECT_GT (plc.num_polygons (), size_t (128000));
  EXPECT_LT (plc.num_polygons (), size_t (132000));
}

TEST(triangulate_with_vertexes)
{
  db::Point contour[] = {
    db::Point (0, 0),
    db::Point (0, 100),
    db::Point (1000, 100),
    db::Point (1000, 0)
  };

  db::Polygon poly;
  poly.assign_hull (contour + 0, contour + sizeof (contour) / sizeof (contour[0]));

  double dbu = 0.001;

  db::plc::TriangulationParameters param;
  param.min_b = 0.0;
  param.max_area = 0.0;

  std::vector<db::Point> vertexes;

  db::plc::Graph plc;
  TestableTriangulation tri (&plc);
  db::CplxTrans trans = db::DCplxTrans (dbu) * db::CplxTrans (db::Trans (db::Point () - poly.box ().center ()));
  tri.triangulate (poly, param, trans);

  EXPECT_EQ (plc.to_string (), "((-0.5, -0.05), (-0.5, 0.05), (0.5, 0.05)), ((0.5, -0.05), (-0.5, -0.05), (0.5, 0.05))");

  vertexes.clear ();

  //  outside vertexes are ignored, but lead to a different triangulation
  vertexes.push_back (db::Point (50, 150));
  tri.triangulate (poly, vertexes, param, trans);

  EXPECT_EQ (plc.to_string (), "((-0.5, -0.05), (-0.133333333333, 0.05), (0.5, -0.05)), ((0.5, 0.05), (0.5, -0.05), (-0.133333333333, 0.05)), ((-0.133333333333, 0.05), (-0.5, -0.05), (-0.5, 0.05))");

  for (auto v = vertexes.begin (); v != vertexes.end (); ++v) {
    auto *vp = tri.find_vertex_for_point (trans * *v);
    EXPECT_EQ (vp, 0);
  }

  //  normal triangulation
  vertexes.clear ();
  vertexes.push_back (db::Point (50, 50));
  tri.triangulate (poly, vertexes, param, trans);

  EXPECT_EQ (plc.to_string (), "((-0.5, -0.05), (-0.5, 0.05), (-0.45, 0)), ((-0.5, 0.05), (0.5, 0.05), (-0.45, 0)), ((0.5, -0.05), (-0.45, 0), (0.5, 0.05)), ((0.5, -0.05), (-0.5, -0.05), (-0.45, 0))");

  for (auto v = vertexes.begin (); v != vertexes.end (); ++v) {
    auto *vp = tri.find_vertex_for_point (trans * *v);
    if (! vp) {
      tl::warn << "Vertex not present in output: " << v->to_string ();
      EXPECT_EQ (1, 0);
    }
  }

  //  linear chain of vertexes must not break triangulation
  vertexes.clear ();
  vertexes.push_back (db::Point (50, 50));
  vertexes.push_back (db::Point (100, 50));
  vertexes.push_back (db::Point (150, 50));
  tri.triangulate (poly, vertexes, param, trans);

  EXPECT_EQ (plc.to_string (), "((-0.5, -0.05), (-0.5, 0.05), (-0.45, 0)), ((-0.4, 0), (-0.45, 0), (-0.5, 0.05)), ((-0.5, -0.05), (-0.45, 0), (-0.4, 0)), ((0.5, -0.05), (-0.35, 0), (0.5, 0.05)), ((-0.5, -0.05), (-0.35, 0), (0.5, -0.05)), ((-0.5, -0.05), (-0.4, 0), (-0.35, 0)), ((-0.35, 0), (-0.5, 0.05), (0.5, 0.05)), ((-0.35, 0), (-0.4, 0), (-0.5, 0.05))");

  for (auto v = vertexes.begin (); v != vertexes.end (); ++v) {
    auto *vp = tri.find_vertex_for_point (trans * *v);
    if (! vp) {
      tl::warn << "Vertex not present in output: " << v->to_string ();
      EXPECT_EQ (1, 0);
    }
  }

  //  aggressive triangulation
  param.min_b = 1.0;
  param.max_area = 20 * 20 * dbu * dbu;

  tri.triangulate (poly, vertexes, param, trans);

  EXPECT_GT (plc.num_polygons (), size_t (380));
  EXPECT_LT (plc.num_polygons (), size_t (420));

  for (auto t = plc.begin (); t != plc.end (); ++t) {
    EXPECT_LE (t->area (), param.max_area);
    EXPECT_GE (t->b (), param.min_b);
  }

  for (auto v = vertexes.begin (); v != vertexes.end (); ++v) {
    auto *vp = tri.find_vertex_for_point (trans * *v);
    if (! vp) {
      tl::warn << "Vertex not present in output: " << v->to_string ();
      EXPECT_EQ (1, 0);
    }
  }
}
