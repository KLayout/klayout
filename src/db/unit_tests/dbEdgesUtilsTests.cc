
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

#include "dbEdges.h"
#include "dbEdgesUtils.h"

TEST(1)
{
  EXPECT_EQ (db::edge_interacts (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (0, 0), db::Point (10, 10))), true);
  EXPECT_EQ (db::edge_interacts (db::Edge (db::Point (10, 0), db::Point (20, 00)), db::Edge (db::Point (0, 0), db::Point (10, 0))), true);
  EXPECT_EQ (db::edge_interacts (db::Edge (db::Point (11, 0), db::Point (20, 00)), db::Edge (db::Point (0, 0), db::Point (10, 0))), false);
  EXPECT_EQ (db::edge_interacts (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (0, 1), db::Point (10, 10))), true);
  EXPECT_EQ (db::edge_interacts (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (1, 0), db::Point (1, 10))), true);
  EXPECT_EQ (db::edge_interacts (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (0, 1), db::Point (0, 10))), false);

  EXPECT_EQ (db::edge_interacts (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Polygon (db::Box (0, 0, 10, 10))), true);
  EXPECT_EQ (db::edge_interacts (db::Edge (db::Point (0, 10), db::Point (10, 10)), db::Polygon (db::Box (0, 0, 10, 10))), true);
  EXPECT_EQ (db::edge_interacts (db::Edge (db::Point (0, 11), db::Point (10, 11)), db::Polygon (db::Box (0, 0, 10, 10))), false);
  EXPECT_EQ (db::edge_interacts (db::Edge (db::Point (10, 20), db::Point (10, 10)), db::Polygon (db::Box (0, 0, 10, 10))), true);
  EXPECT_EQ (db::edge_interacts (db::Edge (db::Point (10, 20), db::Point (10, 11)), db::Polygon (db::Box (0, 0, 10, 10))), false);

  EXPECT_EQ (db::edge_is_inside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (0, 0), db::Point (10, 10))), true);
  EXPECT_EQ (db::edge_is_inside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (0, 0), db::Point (10, 20))), false);
  EXPECT_EQ (db::edge_is_inside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (-10, -10), db::Point (10, 20))), false);
  EXPECT_EQ (db::edge_is_inside (db::Edge (db::Point (5, 5), db::Point (10, 10)), db::Edge (db::Point (0, 0), db::Point (10, 20))), false);
  EXPECT_EQ (db::edge_is_inside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (0, 0), db::Point (20, 20))), true);
  EXPECT_EQ (db::edge_is_inside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (0, 2), db::Point (20, 22))), false);
  EXPECT_EQ (db::edge_is_inside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (0, 0), db::Point (0, 20))), false);
  EXPECT_EQ (db::edge_is_inside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (1, 1), db::Point (20, 20))), false);
  EXPECT_EQ (db::edge_is_inside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (30, 30), db::Point (20, 20))), false);

  EXPECT_EQ (db::edge_is_inside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Polygon ()), false);
  EXPECT_EQ (db::edge_is_inside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Polygon (db::Box (0, 0, 10, 10))), true);
  EXPECT_EQ (db::edge_is_inside (db::Edge (db::Point (0, 0), db::Point (15, 15)), db::Polygon (db::Box (0, 0, 10, 10))), false);
  EXPECT_EQ (db::edge_is_inside (db::Edge (db::Point (0, 0), db::Point (10, 0)), db::Polygon (db::Box (0, 0, 10, 10))), true);
  EXPECT_EQ (db::edge_is_inside (db::Edge (db::Point (-10, 0), db::Point (10, 0)), db::Polygon (db::Box (0, 0, 10, 10))), false);
  EXPECT_EQ (db::edge_is_inside (db::Edge (db::Point (0, 10), db::Point (10, 10)), db::Polygon (db::Box (0, 0, 10, 10))), true);
  EXPECT_EQ (db::edge_is_inside (db::Edge (db::Point (0, 11), db::Point (10, 11)), db::Polygon (db::Box (0, 0, 10, 10))), false);
  EXPECT_EQ (db::edge_is_inside (db::Edge (db::Point (0, 5), db::Point (10, 5)), db::Polygon (db::Box (0, 0, 10, 10))), true);
  EXPECT_EQ (db::edge_is_inside (db::Edge (db::Point (-5, 5), db::Point (15, 5)), db::Polygon (db::Box (0, 0, 10, 10))), false);

  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (0, 0), db::Point (10, 10))), false);
  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (0, 0), db::Point (10, 20))), true);
  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (10, 10), db::Point (20, 10))), true);
  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (-10, -10), db::Point (10, 20))), true);
  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (5, 5), db::Point (10, 10)), db::Edge (db::Point (0, 0), db::Point (10, 20))), true);
  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (0, 0), db::Point (20, 20))), false);
  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (0, 2), db::Point (20, 22))), true);
  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (0, 0), db::Point (0, 20))), true);
  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (1, 1), db::Point (20, 20))), false);
  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Edge (db::Point (30, 30), db::Point (20, 20))), true);
  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (1700, 1500), db::Point (1600, 2500)), db::Edge (db::Point (1700, 1000), db::Point (1700, 2000))), true);

  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Polygon ()), true);
  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (0, 0), db::Point (10, 10)), db::Polygon (db::Box (0, 0, 10, 10))), false);
  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (0, 0), db::Point (15, 15)), db::Polygon (db::Box (0, 0, 10, 10))), false);
  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (0, 0), db::Point (10, 0)), db::Polygon (db::Box (0, 0, 10, 10))), true);
  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (-10, 0), db::Point (0, 0)), db::Polygon (db::Box (0, 0, 10, 10))), true);
  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (-10, 0), db::Point (10, 0)), db::Polygon (db::Box (0, 0, 10, 10))), true);
  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (0, 10), db::Point (10, 10)), db::Polygon (db::Box (0, 0, 10, 10))), true);
  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (0, 11), db::Point (10, 11)), db::Polygon (db::Box (0, 0, 10, 10))), true);
  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (0, 5), db::Point (10, 5)), db::Polygon (db::Box (0, 0, 10, 10))), false);
  EXPECT_EQ (db::edge_is_outside (db::Edge (db::Point (-5, 5), db::Point (15, 5)), db::Polygon (db::Box (0, 0, 10, 10))), false);
}

