
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

namespace
{

class TestableSquareCountingRExtractor
  : public pex::SquareCountingRExtractor
{
public:
  TestableSquareCountingRExtractor ()
    : pex::SquareCountingRExtractor (0.001)
  { }

  using pex::SquareCountingRExtractor::PortDefinition;
  using pex::SquareCountingRExtractor::do_extract;
};

}

TEST(basic)
{
  db::Point contour[] = {
    db::Point (0, 0),
    db::Point (0, 100),
    db::Point (1000, 1000),
    db::Point (2100, 1000),
    db::Point (2100, 0)
  };

  db::Polygon poly;
  poly.assign_hull (contour + 0, contour + sizeof (contour) / sizeof (contour[0]));
  TestableSquareCountingRExtractor rex;
  pex::RNetwork rn;

  TestableSquareCountingRExtractor::PortDefinition pd1 (pex::RNode::Internal, db::Point (-50, 50), 0);
  TestableSquareCountingRExtractor::PortDefinition pd2 (pex::RNode::Internal, db::Point (1000, 100), 1);
  TestableSquareCountingRExtractor::PortDefinition pd3 (pex::RNode::Internal, db::Point (1000, 500), 2);
  TestableSquareCountingRExtractor::PortDefinition pd4 (pex::RNode::Internal, db::Point (2000, 500), 3);

  std::vector<TestableSquareCountingRExtractor::PortDefinition> pds;
  pds.push_back (TestableSquareCountingRExtractor::PortDefinition (pex::RNode::Internal, db::Point (0, 50), 0));
  pds.push_back (TestableSquareCountingRExtractor::PortDefinition (pex::RNode::Internal, db::Point (1000, 100), 1));
  pds.push_back (TestableSquareCountingRExtractor::PortDefinition (pex::RNode::Internal, db::Point (1000, 500), 2));
  pds.push_back (TestableSquareCountingRExtractor::PortDefinition (pex::RNode::Internal, db::Point (2000, 500), 3));

  std::vector<std::pair<TestableSquareCountingRExtractor::PortDefinition, pex::RNode *> > ports;
  for (auto pd = pds.begin (); pd != pds.end (); ++pd) {
    ports.push_back (std::make_pair (*pd, rn.create_node (pd->type, pd->port_index)));
  }

  rex.do_extract (poly, ports, rn);

  EXPECT_EQ (rn.to_string (),
    "R $0 $1 0.390865\n"   //  w ramp w=100 to 1000 over x=0 to 1000 (squares = (x2-x1)/(w2-w1)*log(w2/w1) by integration)
    "R $1 $2 0\n"          //  transition from y=50 to y=500 parallel to current direction
    "R $2 $3 1"            //  1 square between x=1000 and 2000 (w=1000)
  );

  //  After rotation

  rn.clear ();

  db::Trans r90 (db::Trans::r90);

  poly.transform (r90);

  ports.clear ();
  for (auto pd = pds.begin (); pd != pds.end (); ++pd) {
    ports.push_back (std::make_pair (*pd, rn.create_node (pd->type, pd->port_index)));
    ports.back ().first.location.transform (r90);
  }

  rex.do_extract (poly, ports, rn);

  //  Same network, but opposite order. $1 and $2 are shorted, hence can be swapped.
  EXPECT_EQ (rn.to_string (),
    "R $3 $1 1\n"
    "R $1 $2 0\n"
    "R $2 $0 0.390865"
  );
}

TEST(extraction)
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
  vertex_ports.push_back (db::Point (0, 50));     //  V0
  vertex_ports.push_back (db::Point (1650, 50));  //  V1

  std::vector<db::Polygon> polygon_ports;
  polygon_ports.push_back (db::Polygon (db::Box (1000, 900, 1100, 1000)));   // P0

  rex.extract (poly, vertex_ports, polygon_ports, rn);

  EXPECT_EQ (rn.to_string (),
    "R V0 $0 0.0952381\n"
    "R $0 V1 0.166667\n"
    "R P0 $0 0.117647"
  )
}
