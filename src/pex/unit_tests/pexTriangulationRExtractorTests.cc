
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

  EXPECT_EQ (rn.to_string (true),
    "R V0(0,0.05;0,0.05) V1(1,0.05;1,0.05) 10.0938"
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

  EXPECT_EQ (rn.to_string (true),
    "R P0(-0.1,0;0,0.1) P1(1,0;1.1,0.1) 10"
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
    "R P0 P1 10"
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
    "R P0 P2 5\n"
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
    "R P0 P2 4.84211\n"
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
    "R P0 P1 11"
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
    "R P0 V0 0\n"    //  shorted because V0 is inside P0
    "R P0 P1 10"
  )
}

static db::Polygon ellipse (const db::Box &box, int npoints)
{
  npoints = std::max (3, std::min (10000000, npoints));

  std::vector<db::Point> pts;
  pts.reserve (npoints);

  double da = M_PI * 2.0 / npoints;
  for (int i = 0; i < npoints; ++i) {
    double x = box.center ().x () - box.width () * 0.5 * cos (da * i);
    double y = box.center ().y () + box.height () * 0.5 * sin (da * i);
    pts.push_back (db::Point (x, y));
  }

  db::Polygon c;
  c.assign_hull (pts.begin (), pts.end (), false);
  return c;
}

TEST(extraction_analytic_disc)
{
  db::Coord r1 = 2000;
  db::Coord r2 = 10000;
  db::Coord r2pin = 10000 + 1000;

  db::Polygon outer = ellipse (db::Box (-r2pin, -r2pin, r2pin, r2pin), 64);
  db::Polygon disc = ellipse (db::Box (-r2, -r2, r2, r2), 64);
  db::Polygon inner = ellipse (db::Box (-r1, -r1, r1, r1), 64);

  db::Polygon outer_port = *(db::Region (outer) - db::Region (disc)).nth (0);

  double dbu = 0.001;

  pex::RNetwork rn;
  pex::TriangulationRExtractor rex (dbu);

  std::vector<db::Point> vertex_ports;

  std::vector<db::Polygon> polygon_ports;
  polygon_ports.push_back (inner);
  polygon_ports.push_back (outer_port);

  rex.extract (disc, vertex_ports, polygon_ports, rn);

  EXPECT_EQ (rn.to_string (),
    "R P0 P1 0.245558"    //  theoretical: 1/(2*PI)*log(r2/r1) = 0.25615 with r2=10000, r1=2000
  )

  rex.triangulation_parameters ().max_area = 100000 * dbu * dbu;

  rex.extract (disc, vertex_ports, polygon_ports, rn);

  EXPECT_EQ (rn.to_string (),
    "R P0 P1 0.255609"    //  theoretical: 1/(2*PI)*log(r2/r1) = 0.25615 with r2=10000, r1=2000
  )
}

TEST(extraction_meander)
{
  db::Point contour[] = {
    db::Point (0, 0),
    db::Point (0, 1000),
    db::Point (1600, 1000),
    db::Point (1600, 600),
    db::Point (2000, 600),
    db::Point (2000, 1000),
    db::Point (3600, 1000),
    db::Point (3600, 600),
    db::Point (4000, 600),
    db::Point (4000, 1000),
    db::Point (4600, 1000),
    db::Point (4600, 0),
    db::Point (3000, 0),
    db::Point (3000, 400),
    db::Point (2600, 400),
    db::Point (2600, 0),
    db::Point (1000, 0),
    db::Point (1000, 400),
    db::Point (600, 400),
    db::Point (600, 0)
  };

  db::Polygon poly;
  poly.assign_hull (contour + 0, contour + sizeof (contour) / sizeof (contour[0]));

  double dbu = 0.001;

  pex::RNetwork rn;
  pex::TriangulationRExtractor rex (dbu);
  rex.triangulation_parameters ().max_area = 10000 * dbu * dbu;
  rex.triangulation_parameters ().min_b = 0.3;

  std::vector<db::Point> vertex_ports;
  vertex_ports.push_back (db::Point (300, 0));      //  V0
  vertex_ports.push_back (db::Point (4300, 1000));  //  V1

  std::vector<db::Polygon> polygon_ports;

  rex.extract (poly, vertex_ports, polygon_ports, rn);

  EXPECT_EQ (rn.to_string (),
    "R V0 V1 8.61417"          //  what is the "real" value?
  )
}

