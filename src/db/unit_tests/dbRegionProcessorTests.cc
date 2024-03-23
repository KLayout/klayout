

/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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
#include "tlString.h"

#include "dbRegionProcessors.h"


TEST(1_RegionToEdgesProcessor)
{
  db::Point hull[] = {
    db::Point (0, 0),
    db::Point (0, 1000),
    db::Point (1000, 1000),
    db::Point (1000, 2000),
    db::Point (2000, 2000),
    db::Point (2000, 1000),
    db::Point (3000, 1000),
    db::Point (3000, 0)
  };

  db::Point hole[] = {
    db::Point (100, 100),
    db::Point (2900, 100),
    db::Point (2900, 900),
    db::Point (100, 900)
  };

  db::Polygon poly;
  poly.assign_hull (hull + 0, hull + sizeof (hull) / sizeof (hull[0]));
  poly.insert_hole (hole + 0, hole + sizeof (hole) / sizeof (hole[0]));

  std::vector<db::Edge> result;

  result.clear ();
  db::PolygonToEdgeProcessor ().process (poly, result);
  EXPECT_EQ (tl::join (result.begin (), result.end (), ";"), "(0,0;0,1000);(0,1000;1000,1000);(1000,1000;1000,2000);(1000,2000;2000,2000);(2000,2000;2000,1000);(2000,1000;3000,1000);(3000,1000;3000,0);(3000,0;0,0);(100,100;2900,100);(2900,100;2900,900);(2900,900;100,900);(100,900;100,100)");

  result.clear ();
  db::PolygonToEdgeProcessor (db::PolygonToEdgeProcessor::Concave).process (poly, result);
  EXPECT_EQ (tl::join (result.begin (), result.end (), ";"), "(2900,100;2900,900);(2900,900;100,900);(100,900;100,100);(100,100;2900,100)");

  result.clear ();
  db::PolygonToEdgeProcessor (db::PolygonToEdgeProcessor::Convex).process (poly, result);
  EXPECT_EQ (tl::join (result.begin (), result.end (), ";"), "(1000,2000;2000,2000);(3000,1000;3000,0);(3000,0;0,0);(0,0;0,1000)");

  result.clear ();
  db::PolygonToEdgeProcessor (db::PolygonToEdgeProcessor::Step).process (poly, result);
  EXPECT_EQ (tl::join (result.begin (), result.end (), ";"), "(0,1000;1000,1000);(1000,1000;1000,2000);(2000,2000;2000,1000);(2000,1000;3000,1000)");

  result.clear ();
  db::PolygonToEdgeProcessor (db::PolygonToEdgeProcessor::StepOut).process (poly, result);
  EXPECT_EQ (tl::join (result.begin (), result.end (), ";"), "(1000,1000;1000,2000);(2000,1000;3000,1000)");

  result.clear ();
  db::PolygonToEdgeProcessor (db::PolygonToEdgeProcessor::StepIn).process (poly, result);
  EXPECT_EQ (tl::join (result.begin (), result.end (), ";"), "(0,1000;1000,1000);(2000,2000;2000,1000)");
}
