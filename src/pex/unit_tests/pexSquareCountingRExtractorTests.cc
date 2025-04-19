
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


#include "pexSquareCountingRExtractor.h"
#include "tlUnitTest.h"

TEST(basic)
{
  db::Point contour[] = {
    db::Point (0, 0),
    db::Point (0, 100),
    db::Point (1000, 100),
    db::Point (1000, 1000),
    db::Point (1100, 1000),
    db::Point (1100, 100),
    db::Point (1700, 100),
    db::Point (1700, 0)
  };

  db::Polygon poly;
  poly.assign_hull (contour + 0, contour + sizeof (contour) / sizeof (contour[0]));

  double dbu = 0.001;

  pex::RNetwork rn;
  pex::SquareCountingRExtractor rex (dbu);

  std::vector<db::Point> vertex_ports;
  vertex_ports.push_back (db::Point (0, 50));
  vertex_ports.push_back (db::Point (1650, 50));

  std::vector<db::Polygon> polygon_ports;
  polygon_ports.push_back (db::Polygon (db::Box (1000, 900, 1100, 1000)));

  rex.extract (poly, vertex_ports, polygon_ports, rn);

  EXPECT_EQ (rn.to_string (),
    ""
  )
}
