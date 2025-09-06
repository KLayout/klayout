
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


#include "tlUnitTest.h"


#include "dbEdgePairRelations.h"

TEST(1)
{
  EXPECT_EQ (edge_projection (db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (50, 0), db::Point (75, 0))), db::Edge::distance_type (25));
  EXPECT_EQ (edge_projection (db::Edge (db::Point (100, 0), db::Point (0, 0)), db::Edge (db::Point (50, 0), db::Point (75, 0))), db::Edge::distance_type (25));
  EXPECT_EQ (edge_projection (db::Edge (db::Point (100, 0), db::Point (0, 0)), db::Edge (db::Point (75, 0), db::Point (50, 0))), db::Edge::distance_type (25));
  EXPECT_EQ (edge_projection (db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (75, 0), db::Point (50, 0))), db::Edge::distance_type (25));
  EXPECT_EQ (edge_projection (db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (50, 10), db::Point (75, 100))), db::Edge::distance_type (25));
  EXPECT_EQ (edge_projection (db::Edge (db::Point (10, 10), db::Point (100, 100)), db::Edge (db::Point (0, 0), db::Point (60, 0))), db::Edge::distance_type (28));
  EXPECT_EQ (edge_projection (db::Edge (db::Point (10, 10), db::Point (100, 100)), db::Edge (db::Point (0, 0), db::Point (0, 0))), db::Edge::distance_type (0));
  EXPECT_EQ (edge_projection (db::Edge (db::Point (0, 0), db::Point (0, 0)), db::Edge (db::Point (0, 0), db::Point (0, 0))), db::Edge::distance_type (0));
  EXPECT_EQ (edge_projection (db::Edge (db::Point (0, 0), db::Point (0, 0)), db::Edge (db::Point (0, 0), db::Point (10, 0))), db::Edge::distance_type (0));
  EXPECT_EQ (edge_projection (db::Edge (db::Point (10, 10), db::Point (100, 100)), db::Edge (db::Point (0, 0), db::Point (10, 0))), db::Edge::distance_type (0));
  EXPECT_EQ (edge_projection (db::Edge (db::Point (-10, -10), db::Point (100, 100)), db::Edge (db::Point (0, 0), db::Point (10, 0))), db::Edge::distance_type (7));
  EXPECT_EQ (edge_projection (db::Edge (db::Point (100, 100), db::Point (-10, -10)), db::Edge (db::Point (0, 0), db::Point (10, 0))), db::Edge::distance_type (7));
  EXPECT_EQ (edge_projection (db::Edge (db::Point (100, 100), db::Point (-10, -10)), db::Edge (db::Point (10, 0), db::Point (0, 0))), db::Edge::distance_type (7));
  EXPECT_EQ (edge_projection (db::Edge (db::Point (-10, -10), db::Point (100, 100)), db::Edge (db::Point (10, 0), db::Point (0, 0))), db::Edge::distance_type (7));
  EXPECT_EQ (edge_projection (db::Edge (db::Point (-10, -10), db::Point (100, 100)), db::Edge (db::Point (0, 0), db::Point (12, -10))), db::Edge::distance_type (1));
  EXPECT_EQ (edge_projection (db::Edge (db::Point (-10, -10), db::Point (100, 100)), db::Edge (db::Point (0, 0), db::Point (10, -12))), db::Edge::distance_type (1));
  EXPECT_EQ (edge_projection (db::Edge (db::Point (-5, -15), db::Point (105, 95)), db::Edge (db::Point (-20, 24), db::Point (20, -24))), db::Edge::distance_type (6));
  EXPECT_EQ (edge_projection (db::Edge (db::Point (-15, -5), db::Point (95, 105)), db::Edge (db::Point (24, -20), db::Point (-24, 20))), db::Edge::distance_type (6));
}

TEST(2)
{
  db::Edge output;
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, 10), db::Point (100, 200)), &output), false);
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, 200), db::Point (100, 200)), &output), false);
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, 50), db::Point (100, 50)), &output), false);
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, -50), db::Point (100, -50)), &output), true);
  EXPECT_EQ (output.to_string (), "(0,-50;100,-50)");
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, -50), db::Point (300, -50)), &output), true);
  EXPECT_EQ (output.to_string (), "(0,-50;187,-50)");
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (100, -50), db::Point (300, -50)), &output), true);
  EXPECT_EQ (output.to_string (), "(100,-50;187,-50)");
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (-300, -50), db::Point (300, -50)), &output), true);
  EXPECT_EQ (output.to_string (), "(-87,-50;187,-50)");
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (-300, -50), db::Point (0, -50)), &output), true);
  EXPECT_EQ (output.to_string (), "(-87,-50;0,-50)");
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (-300, -100), db::Point (300, -100)), &output), false);
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (-300, 0), db::Point (300, -100)), &output), true);
  EXPECT_EQ (output.to_string (), "(-94,-34;164,-77)");
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, 0), db::Point (100, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(0,0;50,-100)");
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (40, 0), db::Point (140, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(40,0;90,-100)");
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (100, 0), db::Point (200, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(100,0;145,-89)");
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, -200), db::Point (200, -200)), &output), false);
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, 200), db::Point (200, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(100,0;145,-89)");
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (120, 200), db::Point (120, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(120,0;120,-98)");
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (100, 200), db::Point (100, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(100,0;100,-100)");
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (80, 200), db::Point (80, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(80,0;80,-100)");
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (-80, 200), db::Point (-80, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(-80,0;-80,-60)");
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (80, 0), db::Point (-200, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(80,0;-45,-89)");
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (-100, 200), db::Point (-100, -200)), &output), false);
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (), db::Edge (db::Point (-100, 200), db::Point (-100, -200)), &output), false);
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (100, 50), db::Point (100, 50)), &output), false);
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (100, -50), db::Point (100, -50)), &output), true);
  EXPECT_EQ (output.to_string (), "(100,-50;100,-50)");
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (50, -50), db::Point (50, -50)), &output), true);
  EXPECT_EQ (output.to_string (), "(50,-50;50,-50)");
  EXPECT_EQ (euclidian_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (190, -50), db::Point (190, -50)), &output), false);
}

TEST(3)
{
  db::Edge output;
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, 200), db::Point (100, 200)), &output), false);
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, 50), db::Point (100, 50)), &output), false);
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, -50), db::Point (100, -50)), &output), true);
  EXPECT_EQ (output.to_string (), "(0,-50;100,-50)");
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, -50), db::Point (300, -50)), &output), true);
  EXPECT_EQ (output.to_string (), "(0,-50;200,-50)");
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (100, -50), db::Point (300, -50)), &output), true);
  EXPECT_EQ (output.to_string (), "(100,-50;200,-50)");
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (-300, -50), db::Point (300, -50)), &output), true);
  EXPECT_EQ (output.to_string (), "(-100,-50;200,-50)");
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (-300, -50), db::Point (0, -50)), &output), true);
  EXPECT_EQ (output.to_string (), "(-100,-50;0,-50)");
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (-300, -100), db::Point (300, -100)), &output), false);
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (-300, 0), db::Point (300, -100)), &output), true);
  EXPECT_EQ (output.to_string (), "(-100,-33;200,-83)");
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, 0), db::Point (100, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(0,0;50,-100)");
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (40, 0), db::Point (140, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(40,0;90,-100)");
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (100, 0), db::Point (200, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(100,0;150,-100)");
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, -200), db::Point (200, -200)), &output), false);
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, 200), db::Point (200, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(100,0;150,-100)");
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (120, 200), db::Point (120, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(120,0;120,-100)");
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (100, 200), db::Point (100, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(100,0;100,-100)");
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (80, 200), db::Point (80, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(80,0;80,-100)");
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (-80, 200), db::Point (-80, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(-80,0;-80,-100)");
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (80, 0), db::Point (-200, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(80,0;-60,-100)");
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (-100, 200), db::Point (-100, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(-100,0;-100,-100)");
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (), db::Edge (db::Point (-100, 200), db::Point (-100, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(-100,100;-100,-100)");   //  dot vs. line (issue #2141)
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (100, 50), db::Point (100, 50)), &output), false);
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (100, -50), db::Point (100, -50)), &output), true);
  EXPECT_EQ (output.to_string (), "(100,-50;100,-50)");
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (50, -50), db::Point (50, -50)), &output), true);
  EXPECT_EQ (output.to_string (), "(50,-50;50,-50)");
  EXPECT_EQ (square_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (190, -50), db::Point (190, -50)), &output), true);
  EXPECT_EQ (output.to_string (), "(190,-50;190,-50)");
}

TEST(4)
{
  db::Edge output;
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, 200), db::Point (100, 200)), &output), false);
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, 50), db::Point (100, 50)), &output), false);
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, -50), db::Point (100, -50)), &output), true);
  EXPECT_EQ (output.to_string (), "(0,-50;100,-50)");
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, -50), db::Point (300, -50)), &output), true);
  EXPECT_EQ (output.to_string (), "(0,-50;100,-50)");
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (100, -50), db::Point (300, -50)), &output), false);
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (-300, -50), db::Point (300, -50)), &output), true);
  EXPECT_EQ (output.to_string (), "(0,-50;100,-50)");
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (-300, -50), db::Point (0, -50)), &output), false);
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (-300, -100), db::Point (300, -100)), &output), false);
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (-300, 0), db::Point (300, -100)), &output), true);
  EXPECT_EQ (output.to_string (), "(0,-50;100,-67)");
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, 0), db::Point (100, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(0,0;50,-100)");
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (40, 0), db::Point (140, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(40,0;90,-100)");
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (100, 0), db::Point (200, -200)), &output), false);
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, -200), db::Point (200, -200)), &output), false);
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, 200), db::Point (200, -200)), &output), false);
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (120, 200), db::Point (120, -200)), &output), false);
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (100, 200), db::Point (100, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(100,0;100,0)");
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (80, 200), db::Point (80, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(80,0;80,0)");
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (-80, 200), db::Point (-80, -200)), &output), false);
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (80, 0), db::Point (-200, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(80,0;0,-57)");
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (-100, 200), db::Point (-100, -200)), &output), false);
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (), db::Edge (db::Point (-100, 200), db::Point (-100, -200)), &output), true);
  EXPECT_EQ (output.to_string (), "(-100,0;-100,0)");  //  dot vs. line (issue #2141)
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (100, 50), db::Point (100, 50)), &output), false);
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (100, -50), db::Point (100, -50)), &output), true);
  EXPECT_EQ (output.to_string (), "(100,-50;100,-50)");
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (50, -50), db::Point (50, -50)), &output), true);
  EXPECT_EQ (output.to_string (), "(50,-50;50,-50)");
  EXPECT_EQ (projected_near_part_of_edge (db::AlwaysIncludeZeroDistance, 100, db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (190, -50), db::Point (190, -50)), &output), false);
}

TEST(5)
{
  db::EdgePair output;
  bool res;

  db::EdgeRelationFilter f (db::WidthRelation, 50);
  res = f.check (db::Edge (db::Point (100, 200), db::Point (0, 10)), db::Edge (db::Point (0, 0), db::Point (100, 10)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 10), db::Point (100, 200)), db::Edge (db::Point (0, 0), db::Point (100, 10)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (100, 200), db::Point (0, 10)), db::Edge (db::Point (100, 10), db::Point (0, 0)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (100, 10), db::Point (0, 0)), db::Edge (db::Point (0, 10), db::Point (100, 200)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(54,5;0,0):(0,10;22,52)");
  res = f.check (db::Edge (db::Point (100, 10), db::Point (0, 0)), db::Edge (db::Point (0, 0), db::Point (100, 10)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(100,10;0,0):(0,0;100,10)");

  res = f.check (db::Edge (db::Point (100, 10), db::Point (0, 10)), db::Edge (db::Point (0, 10), db::Point (100, 200)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(57,10;0,10):(0,10;26,60)");

  f.set_ignore_angle (80.0);
  res = f.check (db::Edge (db::Point (100, 10), db::Point (0, 10)), db::Edge (db::Point (0, 10), db::Point (100, 200)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(57,10;0,10):(0,10;26,60)");

  f.set_ignore_angle (10.0);
  res = f.check (db::Edge (db::Point (100, 10), db::Point (0, 10)), db::Edge (db::Point (0, 10), db::Point (100, 200)), &output);
  EXPECT_EQ (res, false);
  
  db::EdgeRelationFilter ff (db::SpaceRelation, 50);
  res = ff.check (db::Edge (db::Point (100, 10), db::Point (0, 0)), db::Edge (db::Point (0, 10), db::Point (100, 200)), &output);
  EXPECT_EQ (res, false);
  res = ff.check (db::Edge (db::Point (0, 0), db::Point (100, 10)), db::Edge (db::Point (0, 10), db::Point (100, 200)), &output);
  EXPECT_EQ (res, false);
  res = ff.check (db::Edge (db::Point (100, 10), db::Point (0, 0)), db::Edge (db::Point (100, 200), db::Point (0, 10)), &output);
  EXPECT_EQ (res, false);
  res = ff.check (db::Edge (db::Point (0, 0), db::Point (100, 10)), db::Edge (db::Point (100, 200), db::Point (0, 10)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,0;54,5):(22,52;0,10)");

  db::EdgeRelationFilter f2 (db::OverlapRelation, 50);
  res = f2.check (db::Edge (db::Point (100, 10), db::Point (0, 0)), db::Edge (db::Point (0, 10), db::Point (100, 200)), &output);
  EXPECT_EQ (res, false);
  res = f2.check (db::Edge (db::Point (0, 0), db::Point (100, 10)), db::Edge (db::Point (0, 10), db::Point (100, 200)), &output);
  EXPECT_EQ (res, false);
  res = f2.check (db::Edge (db::Point (0, 0), db::Point (100, 10)), db::Edge (db::Point (100, 200), db::Point (0, 10)), &output);
  EXPECT_EQ (res, false);
  res = f2.check (db::Edge (db::Point (100, 10), db::Point (0, 0)), db::Edge (db::Point (100, 200), db::Point (0, 10)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(54,5;0,0):(22,52;0,10)");

  db::EdgeRelationFilter f3 (db::InsideRelation, 50);
  res = f3.check (db::Edge (db::Point (100, 10), db::Point (0, 0)), db::Edge (db::Point (0, 10), db::Point (100, 200)), &output);
  EXPECT_EQ (res, false);
  res = f3.check (db::Edge (db::Point (100, 10), db::Point (0, 0)), db::Edge (db::Point (100, 200), db::Point (0, 10)), &output);
  EXPECT_EQ (res, false);
  res = f3.check (db::Edge (db::Point (0, 0), db::Point (100, 10)), db::Edge (db::Point (100, 200), db::Point (0, 10)), &output);
  EXPECT_EQ (res, false);
  res = f3.check (db::Edge (db::Point (0, 0), db::Point (100, 10)), db::Edge (db::Point (0, 10), db::Point (100, 200)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,0;54,5):(0,10;22,52)");

  f3.set_min_projection (1000);
  res = f3.check (db::Edge (db::Point (0, 0), db::Point (100, 10)), db::Edge (db::Point (0, 10), db::Point (100, 200)), &output);
  EXPECT_EQ (res, false);
  f3.set_min_projection (10);
  res = f3.check (db::Edge (db::Point (0, 0), db::Point (100, 10)), db::Edge (db::Point (0, 10), db::Point (100, 200)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,0;54,5):(0,10;22,52)");
  f3.set_max_projection (40);
  res = f3.check (db::Edge (db::Point (0, 0), db::Point (100, 10)), db::Edge (db::Point (0, 10), db::Point (100, 200)), &output);
  EXPECT_EQ (res, false);
  f3.set_max_projection (200);
  res = f3.check (db::Edge (db::Point (0, 0), db::Point (100, 10)), db::Edge (db::Point (0, 10), db::Point (100, 200)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,0;54,5):(0,10;22,52)");
}

TEST(6)
{
  db::EdgeRelationFilter f (db::WidthRelation, 70000);
  db::EdgePair output;
  bool res = f.check (db::Edge (db::Point (20570000, -18890000), db::Point (20650000, -18890000)), db::Edge (db::Point (20650000, -18950000), db::Point (20550000, -18950000)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(20570000,-18890000;20650000,-18890000):(20650000,-18950000;20550000,-18950000)");
}

TEST(7)
{
  db::EdgeRelationFilter f (db::WidthRelation, 100);
  db::EdgePair output;
  bool res;

  f.set_metrics (db::Euclidian);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (0, 30), db::Point (0, 20)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,0;0,10):(0,30;0,20)");
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (1, 30), db::Point (1, 20)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,0;0,10):(1,30;1,20)");
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (-1, 30), db::Point (-1, 20)), &output);
  EXPECT_EQ (res, false);
  f.set_zero_distance_mode (db::IncludeZeroDistanceWhenTouching);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (0, 30), db::Point (0, 20)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (1, 30), db::Point (1, 20)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,0;0,10):(1,30;1,20)");
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (-1, 30), db::Point (-1, 20)), &output);
  EXPECT_EQ (res, false);

  f.set_zero_distance_mode (db::AlwaysIncludeZeroDistance);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (0, 300), db::Point (0, -200)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,0;0,10):(0,110;0,-100)");
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (1, 300), db::Point (1, -200)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,0;0,10):(1,110;1,-100)");

  f.set_metrics (db::Square);
  f.set_zero_distance_mode (db::AlwaysIncludeZeroDistance);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (0, 30), db::Point (0, 20)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,0;0,10):(0,30;0,20)");
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (1, 30), db::Point (1, 20)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,0;0,10):(1,30;1,20)");
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (-1, 30), db::Point (-1, 20)), &output);
  EXPECT_EQ (res, false);
  f.set_zero_distance_mode (db::IncludeZeroDistanceWhenTouching);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (0, 30), db::Point (0, 20)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (1, 30), db::Point (1, 20)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,0;0,10):(1,30;1,20)");
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (-1, 30), db::Point (-1, 20)), &output);
  EXPECT_EQ (res, false);

  f.set_zero_distance_mode (db::AlwaysIncludeZeroDistance);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (0, 300), db::Point (0, -200)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,0;0,10):(0,110;0,-100)");
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (1, 300), db::Point (1, -200)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,0;0,10):(1,110;1,-100)");

  f.set_metrics (db::Projection);
  f.set_zero_distance_mode (db::AlwaysIncludeZeroDistance);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (0, 30), db::Point (0, -20)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,0;0,10):(0,10;0,0)");
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (1, 30), db::Point (1, -20)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,0;0,10):(1,10;1,0)");
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (-1, 30), db::Point (-1, -20)), &output);
  EXPECT_EQ (res, false);
  f.set_zero_distance_mode (db::IncludeZeroDistanceWhenTouching);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (0, 30), db::Point (0, 11)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (1, 30), db::Point (1, -20)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,0;0,10):(1,10;1,0)");
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 10)), db::Edge (db::Point (-1, 30), db::Point (-1, -20)), &output);
  EXPECT_EQ (res, false);
}

TEST(8_KissingCornerProblem)
{
  //  The kissing corner problem is solved by allowing distance-0 width and space relations and checking them
  //  if the projection is >0.

  db::EdgeRelationFilter f (db::WidthRelation, 10);
  f.set_zero_distance_mode (db::IncludeZeroDistanceWhenTouching);
  db::EdgePair output;
  bool res;

  f.set_metrics (db::Euclidian);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 201), db::Point (0, 101)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (1, 0), db::Point (1, 100)), db::Edge (db::Point (0, 201), db::Point (0, 0)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 200), db::Point (0, 100)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,90;0,100):(0,110;0,100)");
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 200), db::Point (0, 50)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,40;0,100):(0,110;0,50)");
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 0), db::Point (0, -100)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,0;0,10):(0,0;0,-10)");
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, -1), db::Point (0, -100)), &output);
  EXPECT_EQ (res, false);

  f.set_zero_distance_mode (db::IncludeZeroDistanceWhenOverlapping);

  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 201), db::Point (0, 101)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (1, 0), db::Point (1, 100)), db::Edge (db::Point (0, 201), db::Point (0, 0)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 200), db::Point (0, 100)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 200), db::Point (0, 50)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,40;0,100):(0,110;0,50)");
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 0), db::Point (0, -100)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, -1), db::Point (0, -100)), &output);
  EXPECT_EQ (res, false);

  f.set_zero_distance_mode (db::NeverIncludeZeroDistance);

  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 201), db::Point (0, 101)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (1, 0), db::Point (1, 100)), db::Edge (db::Point (0, 201), db::Point (0, 0)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 200), db::Point (0, 100)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 200), db::Point (0, 50)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 0), db::Point (0, -100)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, -1), db::Point (0, -100)), &output);
  EXPECT_EQ (res, false);

  f = db::EdgeRelationFilter (db::SpaceRelation, 10);
  f.set_zero_distance_mode (db::IncludeZeroDistanceWhenTouching);

  f.set_metrics (db::Euclidian);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 101), db::Point (0, 201)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (1, 100), db::Point (1, 0)), db::Edge (db::Point (0, 0), db::Point (0, 200)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 100), db::Point (0, 200)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,100;0,90):(0,100;0,110)");
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 50), db::Point (0, 200)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,100;0,40):(0,50;0,110)");
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, -100), db::Point (0, 0)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,10;0,0):(0,-10;0,0)");
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, -100), db::Point (0, -1)), &output);
  EXPECT_EQ (res, false);

  f.set_zero_distance_mode (db::IncludeZeroDistanceWhenOverlapping);

  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 101), db::Point (0, 201)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (1, 100), db::Point (1, 0)), db::Edge (db::Point (0, 0), db::Point (0, 200)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 100), db::Point (0, 200)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 50), db::Point (0, 200)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,100;0,40):(0,50;0,110)");
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, -100), db::Point (0, 0)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, -100), db::Point (0, -1)), &output);
  EXPECT_EQ (res, false);

  f.set_zero_distance_mode (db::NeverIncludeZeroDistance);

  f.set_metrics (db::Euclidian);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 101), db::Point (0, 201)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (1, 100), db::Point (1, 0)), db::Edge (db::Point (0, 0), db::Point (0, 200)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 100), db::Point (0, 200)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 50), db::Point (0, 200)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, -100), db::Point (0, 0)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, -100), db::Point (0, -1)), &output);
  EXPECT_EQ (res, false);
}

TEST(9_KissingCornerProblemSquareMetrics)
{
  //  The kissing corner problem is solved by allowing distance-0 width and space relations and checking them
  //  if the projection is >0.

  db::EdgeRelationFilter f (db::WidthRelation, 10);
  f.set_zero_distance_mode (db::IncludeZeroDistanceWhenTouching);
  db::EdgePair output;
  bool res;

  f.set_metrics (db::Square);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 201), db::Point (0, 101)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (1, 0), db::Point (1, 100)), db::Edge (db::Point (0, 201), db::Point (0, 0)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 200), db::Point (0, 100)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,90;0,100):(0,110;0,100)");
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 200), db::Point (0, 50)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,40;0,100):(0,110;0,50)");
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 0), db::Point (0, -100)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,0;0,10):(0,0;0,-10)");
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, -1), db::Point (0, -100)), &output);
  EXPECT_EQ (res, false);

  f.set_zero_distance_mode (db::NeverIncludeZeroDistance);

  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 201), db::Point (0, 101)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (1, 0), db::Point (1, 100)), db::Edge (db::Point (0, 201), db::Point (0, 0)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 200), db::Point (0, 100)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 200), db::Point (0, 50)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 0), db::Point (0, -100)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, -1), db::Point (0, -100)), &output);
  EXPECT_EQ (res, false);

  f = db::EdgeRelationFilter (db::SpaceRelation, 10);
  f.set_zero_distance_mode (db::IncludeZeroDistanceWhenTouching);

  f.set_metrics (db::Square);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 101), db::Point (0, 201)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (1, 100), db::Point (1, 0)), db::Edge (db::Point (0, 0), db::Point (0, 200)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 100), db::Point (0, 200)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,100;0,90):(0,100;0,110)");
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 50), db::Point (0, 200)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,100;0,40):(0,50;0,110)");
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, -100), db::Point (0, 0)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,10;0,0):(0,-10;0,0)");
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, -100), db::Point (0, -1)), &output);
  EXPECT_EQ (res, false);

  f.set_zero_distance_mode (db::NeverIncludeZeroDistance);

  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 101), db::Point (0, 201)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (1, 100), db::Point (1, 0)), db::Edge (db::Point (0, 0), db::Point (0, 200)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 100), db::Point (0, 200)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 50), db::Point (0, 200)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, -100), db::Point (0, 0)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, -100), db::Point (0, -1)), &output);
  EXPECT_EQ (res, false);
}

TEST(10_KissingCornerProblemProjectionMetrics)
{
  //  The kissing corner problem is solved by allowing distance-0 width and space relations and checking them
  //  if the projection is >0. It is not effective in projection metrics as there is no overlap.

  db::EdgeRelationFilter f (db::WidthRelation, 10);
  f.set_zero_distance_mode (db::IncludeZeroDistanceWhenTouching);
  db::EdgePair output;
  bool res;

  f.set_metrics (db::Projection);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 201), db::Point (0, 101)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (1, 0), db::Point (1, 100)), db::Edge (db::Point (0, 201), db::Point (0, 0)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 200), db::Point (0, 100)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 200), db::Point (0, 50)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,50;0,100):(0,100;0,50)");
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 0), db::Point (0, -100)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, -1), db::Point (0, -100)), &output);
  EXPECT_EQ (res, false);

  f.set_zero_distance_mode (db::NeverIncludeZeroDistance);

  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 201), db::Point (0, 101)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (1, 0), db::Point (1, 100)), db::Edge (db::Point (0, 201), db::Point (0, 0)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 200), db::Point (0, 100)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 200), db::Point (0, 50)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, 0), db::Point (0, -100)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 0), db::Point (0, 100)), db::Edge (db::Point (0, -1), db::Point (0, -100)), &output);
  EXPECT_EQ (res, false);

  f = db::EdgeRelationFilter (db::SpaceRelation, 10);
  f.set_zero_distance_mode (db::IncludeZeroDistanceWhenTouching);

  f.set_metrics (db::Projection);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 101), db::Point (0, 201)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (1, 100), db::Point (1, 0)), db::Edge (db::Point (0, 0), db::Point (0, 200)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 100), db::Point (0, 200)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 50), db::Point (0, 200)), &output);
  EXPECT_EQ (res, true);
  EXPECT_EQ (output.first ().to_string () + ":" + output.second ().to_string (), "(0,100;0,50):(0,50;0,100)");
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, -100), db::Point (0, 0)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, -100), db::Point (0, -1)), &output);
  EXPECT_EQ (res, false);

  f.set_zero_distance_mode (db::NeverIncludeZeroDistance);

  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 101), db::Point (0, 201)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (1, 100), db::Point (1, 0)), db::Edge (db::Point (0, 0), db::Point (0, 200)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 100), db::Point (0, 200)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, 50), db::Point (0, 200)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, -100), db::Point (0, 0)), &output);
  EXPECT_EQ (res, false);
  res = f.check (db::Edge (db::Point (0, 100), db::Point (0, 0)), db::Edge (db::Point (0, -100), db::Point (0, -1)), &output);
  EXPECT_EQ (res, false);
}
