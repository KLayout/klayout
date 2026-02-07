

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


#include "tlUnitTest.h"
#include "tlStringEx.h"

#include "dbRegionCheckUtils.h"

TEST(1_SimpleLShape)
{
  std::set<db::EdgePair> ep;
  std::set<db::Edge> ee1, ee2;

  db::EdgeRelationFilter er (db::WidthRelation, 1001, db::Projection);

  db::edge2edge_check_with_negative_output<std::set<db::EdgePair>, std::set<db::Edge> > e2e (er, ep, ee1, ee2, false, false, false, true);

  db::Point pts[] = {
    db::Point (0, 0),
    db::Point (0, 2000),
    db::Point (2000, 2000),
    db::Point (2000, 1000),
    db::Point (1000, 1000),
    db::Point (1000, 0)
  };

  db::Polygon poly;
  poly.assign_hull (pts, pts + sizeof (pts) / sizeof (pts[0]));

  db::poly2poly_check<db::Polygon> poly_check (e2e);

  do {
    //  single polygon check
    poly_check.single (poly, 0);
  } while (e2e.prepare_next_pass ());

  EXPECT_EQ (tl::to_string (ep), "(0,0;0,1000)|(1000,1000;1000,0),(2000,1000;1000,1000)|(1000,2000;2000,2000)");
  EXPECT_EQ (tl::to_string (ee1), "");
  EXPECT_EQ (tl::to_string (ee2), "");
}

TEST(1s_SimpleLShape)
{
  std::set<db::EdgePair> ep;
  std::set<db::Edge> ee1, ee2;

  db::EdgeRelationFilter er (db::WidthRelation, 1001, db::Projection);

  db::edge2edge_check_with_negative_output<std::set<db::EdgePair>, std::set<db::Edge> > e2e (er, ep, ee1, ee2, false, false, false, false);

  db::Point pts[] = {
    db::Point (0, 0),
    db::Point (0, 2000),
    db::Point (2000, 2000),
    db::Point (2000, 1000),
    db::Point (1000, 1000),
    db::Point (1000, 0)
  };

  db::Polygon poly;
  poly.assign_hull (pts, pts + sizeof (pts) / sizeof (pts[0]));

  db::poly2poly_check<db::Polygon> poly_check (e2e);

  do {
    //  single polygon check
    poly_check.single (poly, 0);
  } while (e2e.prepare_next_pass ());

  EXPECT_EQ (tl::to_string (ep), "(0,0;0,1000)/(1000,1000;1000,0),(1000,2000;2000,2000)/(2000,1000;1000,1000)");
  EXPECT_EQ (tl::to_string (ee1), "");
  EXPECT_EQ (tl::to_string (ee2), "");
}

TEST(2_SimpleLWithBigPart)
{
  std::set<db::EdgePair> ep;
  std::set<db::Edge> ee1, ee2;

  db::EdgeRelationFilter er (db::WidthRelation, 1001, db::Projection);

  db::edge2edge_check_with_negative_output<std::set<db::EdgePair>, std::set<db::Edge> > e2e (er, ep, ee1, ee2, false, false, false, true);

  db::Point pts[] = {
    db::Point (0, 0),
    db::Point (0, 2500),
    db::Point (2000, 2500),
    db::Point (2000, 1000),
    db::Point (1000, 1000),
    db::Point (1000, 0)
  };

  db::Polygon poly;
  poly.assign_hull (pts, pts + sizeof (pts) / sizeof (pts[0]));

  db::poly2poly_check<db::Polygon> poly_check (e2e);

  do {
    //  single polygon check
    poly_check.single (poly, 0);
  } while (e2e.prepare_next_pass ());

  EXPECT_EQ (tl::to_string (ep), "(0,0;0,1000)|(1000,1000;1000,0)");
  EXPECT_EQ (tl::to_string (ee1), "(0,1000;0,2500),(2000,1000;1000,1000),(0,2500;2000,2500),(2000,2500;2000,1000)");
  EXPECT_EQ (tl::to_string (ee2), "");
}

TEST(3_SimpleTWithBigPart)
{
  std::set<db::EdgePair> ep;
  std::set<db::Edge> ee1, ee2;

  db::EdgeRelationFilter er (db::WidthRelation, 1001, db::Projection);

  db::edge2edge_check_with_negative_output<std::set<db::EdgePair>, std::set<db::Edge> > e2e (er, ep, ee1, ee2, false, false, false, true);

  db::Point pts[] = {
    db::Point (0, 0),
    db::Point (0, 3500),
    db::Point (1000, 3500),
    db::Point (1000, 2500),
    db::Point (2000, 2500),
    db::Point (2000, 1000),
    db::Point (1000, 1000),
    db::Point (1000, 0)
  };

  db::Polygon poly;
  poly.assign_hull (pts, pts + sizeof (pts) / sizeof (pts[0]));

  db::poly2poly_check<db::Polygon> poly_check (e2e);

  do {
    //  single polygon check
    poly_check.single (poly, 0);
  } while (e2e.prepare_next_pass ());

  EXPECT_EQ (tl::to_string (ep), "(0,0;0,1000)|(1000,1000;1000,0),(0,2500;0,3500)|(1000,3500;1000,2500)");
  EXPECT_EQ (tl::to_string (ee1), "(0,1000;0,2500),(2000,1000;1000,1000),(1000,2500;2000,2500),(2000,2500;2000,1000)");
  EXPECT_EQ (tl::to_string (ee2), "");
}

TEST(4_SimpleNotch)
{
  std::set<db::EdgePair> ep;
  std::set<db::Edge> ee1, ee2;

  db::EdgeRelationFilter er (db::SpaceRelation, 1001, db::Projection);

  db::edge2edge_check_with_negative_output<std::set<db::EdgePair>, std::set<db::Edge> > e2e (er, ep, ee1, ee2, false, false, false, true);

  db::Point pts[] = {
    db::Point (0, 0),
    db::Point (0, 3000),
    db::Point (2000, 3000),
    db::Point (2000, 2000),
    db::Point (1000, 2000),
    db::Point (1000, 1000),
    db::Point (2000, 1000),
    db::Point (2000, 0)
  };

  db::Polygon poly;
  poly.assign_hull (pts, pts + sizeof (pts) / sizeof (pts[0]));

  db::poly2poly_check<db::Polygon> poly_check (e2e);

  do {
    //  single polygon check
    poly_check.single (poly, 0);
  } while (e2e.prepare_next_pass ());

  EXPECT_EQ (tl::to_string (ep), "(1000,1000;2000,1000)|(2000,2000;1000,2000)");
  EXPECT_EQ (tl::to_string (ee1), "(0,0;0,3000),(2000,0;0,0),(2000,1000;2000,0),(0,3000;2000,3000),(2000,3000;2000,2000)");
  EXPECT_EQ (tl::to_string (ee2), "");
}

TEST(5_LShapeNotch)
{
  std::set<db::EdgePair> ep;
  std::set<db::Edge> ee1, ee2;

  db::EdgeRelationFilter er (db::SpaceRelation, 1001, db::Projection);

  db::edge2edge_check_with_negative_output<std::set<db::EdgePair>, std::set<db::Edge> > e2e (er, ep, ee1, ee2, false, false, false, true);

  db::Point pts[] = {
    db::Point (0, 0),
    db::Point (0, 3000),
    db::Point (2000, 3000),
    db::Point (2000, 1500),
    db::Point (1500, 1500),
    db::Point (1500, 2500),
    db::Point (500, 2500),
    db::Point (500, 500),
    db::Point (2000, 500),
    db::Point (2000, 0)
  };

  db::Polygon poly;
  poly.assign_hull (pts, pts + sizeof (pts) / sizeof (pts[0]));

  db::poly2poly_check<db::Polygon> poly_check (e2e);

  do {
    //  single polygon check
    poly_check.single (poly, 0);
  } while (e2e.prepare_next_pass ());

  EXPECT_EQ (tl::to_string (ep), "(1500,500;2000,500)|(2000,1500;1500,1500),(1500,1500;1500,2500)|(500,2500;500,1500)");
  EXPECT_EQ (tl::to_string (ee1), "(0,0;0,3000),(2000,0;0,0),(2000,500;2000,0),(0,3000;2000,3000),(2000,3000;2000,1500)");
  EXPECT_EQ (tl::to_string (ee2), "");
}

TEST(6_SeparationLvsBox)
{
  std::set<db::EdgePair> ep;
  std::set<db::Edge> ee1, ee2;

  db::EdgeRelationFilter er (db::SpaceRelation, 1001, db::Projection);

  db::edge2edge_check_with_negative_output<std::set<db::EdgePair>, std::set<db::Edge> > e2e (er, ep, ee1, ee2, false, true /*different layers*/, false, false);

  db::Point pts1[] = {
    db::Point (0, 0),
    db::Point (0, 3000),
    db::Point (3000, 3000),
    db::Point (3000, 2000),
    db::Point (1000, 2000),
    db::Point (1000, 0)
  };

  db::Polygon poly1;
  poly1.assign_hull (pts1, pts1 + sizeof (pts1) / sizeof (pts1[0]));

  db::Point pts2[] = {
    db::Point (2000, 0),
    db::Point (2000, 1000),
    db::Point (3000, 1000),
    db::Point (3000, 0)
  };

  db::Polygon poly2;
  poly2.assign_hull (pts2, pts2 + sizeof (pts2) / sizeof (pts2[0]));

  db::poly2poly_check<db::Polygon> poly_check (e2e);
  poly_check.enter (poly1, 0);  //  layer 0
  poly_check.enter (poly2, 1);  //  layer 1

  do {
    poly_check.process ();
  } while (e2e.prepare_next_pass ());

  EXPECT_EQ (tl::to_string (ep), "(1000,1000;1000,0)/(2000,0;2000,1000),(3000,2000;2000,2000)/(2000,1000;3000,1000)");
  EXPECT_EQ (tl::to_string (ee1), "(0,0;0,3000),(1000,0;0,0),(0,3000;3000,3000),(3000,3000;3000,2000)");
  EXPECT_EQ (tl::to_string (ee2), "(3000,0;2000,0),(3000,1000;3000,0)");
}
