
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


#include "pexTriangulationRExtractor.h"
#include "tlUnitTest.h"

namespace
{

class TestableTriangulationRExtractor
  : public pex::TriangulationRExtractor
{
public:
  TestableTriangulationRExtractor ()
    : pex::TriangulationRExtractor (0.001)
  { }
};

}

TEST(extraction)
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

  pex::RNetwork rn;
  pex::TriangulationRExtractor rex (dbu);

  std::vector<db::Point> vertex_ports;
  vertex_ports.push_back (db::Point (0, 50));     //  V0
  vertex_ports.push_back (db::Point (1000, 50));  //  V1

  std::vector<db::Polygon> polygon_ports;

  rex.extract (poly, vertex_ports, polygon_ports, rn);

  EXPECT_EQ (rn.to_string (),
    "R V1 V0 10.0938"
  )
}

TEST(extraction_with_polygon_ports)
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

  pex::RNetwork rn;
  pex::TriangulationRExtractor rex (dbu);

  std::vector<db::Point> vertex_ports;

  std::vector<db::Polygon> polygon_ports;
  polygon_ports.push_back (db::Polygon (db::Box (-100, 0, 0, 100)));
  polygon_ports.push_back (db::Polygon (db::Box (1000, 0, 1100, 100)));

  rex.extract (poly, vertex_ports, polygon_ports, rn);

  EXPECT_EQ (rn.to_string (),
    "R P1 P0 10"
  )
}

TEST(extraction_with_polygon_ports_inside)
{
  db::Point contour[] = {
    db::Point (-100, 0),
    db::Point (-100, 100),
    db::Point (1100, 100),
    db::Point (1100, 0)
  };

  db::Polygon poly;
  poly.assign_hull (contour + 0, contour + sizeof (contour) / sizeof (contour[0]));

  double dbu = 0.001;

  pex::RNetwork rn;
  pex::TriangulationRExtractor rex (dbu);

  std::vector<db::Point> vertex_ports;

  std::vector<db::Polygon> polygon_ports;
  polygon_ports.push_back (db::Polygon (db::Box (-100, 0, 0, 100)));
  polygon_ports.push_back (db::Polygon (db::Box (1000, 0, 1100, 100)));

  rex.extract (poly, vertex_ports, polygon_ports, rn);

  EXPECT_EQ (rn.to_string (),
    "R P1 P0 10"
  )
}

TEST(extraction_split_by_ports)
{
  db::Point contour[] = {
    db::Point (-100, 0),
    db::Point (-100, 100),
    db::Point (1100, 100),
    db::Point (1100, 0)
  };

  db::Polygon poly;
  poly.assign_hull (contour + 0, contour + sizeof (contour) / sizeof (contour[0]));

  double dbu = 0.001;

  pex::RNetwork rn;
  pex::TriangulationRExtractor rex (dbu);

  std::vector<db::Point> vertex_ports;

  std::vector<db::Polygon> polygon_ports;
  polygon_ports.push_back (db::Polygon (db::Box (-100, 0, 0, 100)));
  polygon_ports.push_back (db::Polygon (db::Box (1100, 0, 1200, 100)));
  polygon_ports.push_back (db::Polygon (db::Box (500, 0, 600, 100)));

  rex.extract (poly, vertex_ports, polygon_ports, rn);

  EXPECT_EQ (rn.to_string (),
    "R P2 P0 5\n"
    "R P1 P2 5"
  )
}

TEST(extraction_split_by_butting_port)
{
  db::Point contour[] = {
    db::Point (-100, 0),
    db::Point (-100, 100),
    db::Point (1100, 100),
    db::Point (1100, 0)
  };

  db::Polygon poly;
  poly.assign_hull (contour + 0, contour + sizeof (contour) / sizeof (contour[0]));

  double dbu = 0.001;

  pex::RNetwork rn;
  pex::TriangulationRExtractor rex (dbu);

  std::vector<db::Point> vertex_ports;

  std::vector<db::Polygon> polygon_ports;
  polygon_ports.push_back (db::Polygon (db::Box (-100, 0, 0, 100)));
  polygon_ports.push_back (db::Polygon (db::Box (1100, 0, 1200, 100)));
  polygon_ports.push_back (db::Polygon (db::Box (500, 100, 600, 200)));

  rex.extract (poly, vertex_ports, polygon_ports, rn);

  EXPECT_EQ (rn.to_string (),
    "R P2 P0 4.84211\n"
    "R P1 P2 4.84211\n"
    "R P0 P1 281.111"
  )
}

TEST(extraction_with_outside_polygon_port)
{
  db::Point contour[] = {
    db::Point (-100, 0),
    db::Point (-100, 100),
    db::Point (1100, 100),
    db::Point (1100, 0)
  };

  db::Polygon poly;
  poly.assign_hull (contour + 0, contour + sizeof (contour) / sizeof (contour[0]));

  double dbu = 0.001;

  pex::RNetwork rn;
  pex::TriangulationRExtractor rex (dbu);

  std::vector<db::Point> vertex_ports;

  std::vector<db::Polygon> polygon_ports;
  polygon_ports.push_back (db::Polygon (db::Box (-100, 0, 0, 100)));
  polygon_ports.push_back (db::Polygon (db::Box (1100, 0, 1200, 100)));
  polygon_ports.push_back (db::Polygon (db::Box (500, 200, 600, 300)));

  rex.extract (poly, vertex_ports, polygon_ports, rn);

  EXPECT_EQ (rn.to_string (),
    "R P1 P0 11"
  )
}

TEST(extraction_with_polygon_ports_and_vertex_port_inside)
{
  db::Point contour[] = {
    db::Point (-100, 0),
    db::Point (-100, 100),
    db::Point (1100, 100),
    db::Point (1100, 0)
  };

  db::Polygon poly;
  poly.assign_hull (contour + 0, contour + sizeof (contour) / sizeof (contour[0]));

  double dbu = 0.001;

  pex::RNetwork rn;
  pex::TriangulationRExtractor rex (dbu);

  std::vector<db::Point> vertex_ports;
  vertex_ports.push_back (db::Point (-50, 50));

  std::vector<db::Polygon> polygon_ports;
  polygon_ports.push_back (db::Polygon (db::Box (-100, 0, 0, 100)));
  polygon_ports.push_back (db::Polygon (db::Box (1000, 0, 1100, 100)));

  rex.extract (poly, vertex_ports, polygon_ports, rn);

  EXPECT_EQ (rn.to_string (),
    "R V0 P0 0\n"    //  shorted because V0 is inside P0
    "R P1 P0 10"
  )
}
