
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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
#include "dbHierNetworkProcessor.h"
#include "dbTestSupport.h"
#include "dbShapeRepository.h"
#include "dbPolygon.h"
#include "dbPath.h"
#include "dbText.h"

static std::string l2s (db::Connectivity::layer_iterator b, db::Connectivity::layer_iterator e)
{
  std::string s;
  for (db::Connectivity::layer_iterator i = b; i != e; ++i) {
    if (! s.empty ()) {
      s += ",";
    }
    s += tl::to_string (*i);
  }
  return s;
}

TEST(1_Connectivity)
{
  db::Connectivity conn;

  EXPECT_EQ (l2s (conn.begin_layers (), conn.end_layers ()), "");

  conn.connect (0);
  EXPECT_EQ (l2s (conn.begin_layers (), conn.end_layers ()), "0");
  EXPECT_EQ (l2s (conn.begin_connected (0), conn.end_connected (0)), "0");
  EXPECT_EQ (l2s (conn.begin_connected (1), conn.end_connected (1)), "");

  conn.connect (0, 1);
  EXPECT_EQ (l2s (conn.begin_layers (), conn.end_layers ()), "0,1");
  EXPECT_EQ (l2s (conn.begin_connected (0), conn.end_connected (0)), "0,1");
  EXPECT_EQ (l2s (conn.begin_connected (1), conn.end_connected (1)), "0");

  conn.connect (1);
  EXPECT_EQ (l2s (conn.begin_connected (1), conn.end_connected (1)), "0,1");

  conn.connect (0, 2);
  conn.connect (2);

  EXPECT_EQ (l2s (conn.begin_connected (0), conn.end_connected (0)), "0,1,2");
  EXPECT_EQ (l2s (conn.begin_connected (1), conn.end_connected (1)), "0,1");
  EXPECT_EQ (l2s (conn.begin_connected (2), conn.end_connected (2)), "0,2");
}

TEST(2_ShapeInteractions)
{
  db::Connectivity conn;

  conn.connect (0);
  conn.connect (1);
  conn.connect (0, 1);

  db::Polygon poly;
  tl::from_string ("(0,0;0,1000;1000,1000;1000,0)", poly);
  db::GenericRepository repo;
  db::PolygonRef ref1 (poly, repo);
  db::PolygonRef ref2 (poly.transformed (db::Trans (db::Vector (0, 10))), repo);
  db::PolygonRef ref3 (poly.transformed (db::Trans (db::Vector (0, 2000))), repo);

  EXPECT_EQ (conn.interacts (ref1, 0, ref2, 0), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref2, 1), true);
  EXPECT_EQ (conn.interacts (ref1, 1, ref2, 0), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref3, 0), false);
  EXPECT_EQ (conn.interacts (ref1, 0, ref3, 1), false);
  EXPECT_EQ (conn.interacts (ref1, 1, ref2, 2), false);
}

TEST(2_ShapeInteractionsRealPolygon)
{
  db::Connectivity conn;

  conn.connect (0);
  conn.connect (1);
  conn.connect (0, 1);

  db::Polygon poly;
  tl::from_string ("(0,0;0,1000;500,1000;500,1500;1000,1500;1000,0)", poly);
  db::GenericRepository repo;
  db::PolygonRef ref1 (poly, repo);
  db::PolygonRef ref2 (poly.transformed (db::Trans (db::Vector (0, 10))), repo);
  db::PolygonRef ref3 (poly.transformed (db::Trans (db::Vector (0, 2000))), repo);
  db::PolygonRef ref4 (poly.transformed (db::Trans (db::Vector (0, 1500))), repo);

  EXPECT_EQ (conn.interacts (ref1, 0, ref2, 0), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref2, 1), true);
  EXPECT_EQ (conn.interacts (ref1, 1, ref2, 0), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref3, 0), false);
  EXPECT_EQ (conn.interacts (ref1, 0, ref4, 0), true);
  EXPECT_EQ (conn.interacts (ref1, 0, ref3, 1), false);
  EXPECT_EQ (conn.interacts (ref1, 1, ref2, 2), false);
}
