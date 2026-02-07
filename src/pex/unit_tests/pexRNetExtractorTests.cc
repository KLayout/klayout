
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


#include "pexRNetExtractor.h"
#include "pexRExtractorTech.h"
#include "pexRNetwork.h"
#include "dbReader.h"
#include "dbLayout.h"
#include "tlUnitTest.h"

class TestableRNetExtractor
  : public pex::RNetExtractor
{
public:
  TestableRNetExtractor (double dbu) : pex::RNetExtractor (dbu) { }

  using pex::RNetExtractor::create_via_ports;
};

static std::string network2s (const pex::RNetwork &network)
{
  std::vector<std::string> r;

  for (auto e = network.begin_elements (); e != network.end_elements (); ++e) {

    const pex::RElement &element = *e;

    std::string na = (element.a ()->type != pex::RNode::Internal ? element.a ()->to_string () : "") +
                        element.a ()->location.to_string ();
    std::string nb = (element.b ()->type != pex::RNode::Internal ? element.b ()->to_string () : "") +
                        element.b ()->location.to_string ();

    if (nb < na) {
      std::swap (na, nb);
    }

    std::string s = "R " + na + " " + nb + " " + tl::to_string (element.resistance ());
    r.push_back (s);

  }

  std::sort (r.begin (), r.end ());

  return tl::join (r, "\n");
}

TEST(basic)
{
  unsigned int l1 = 1;
  unsigned int l2 = 1;
  unsigned int l3 = 1;

  pex::RExtractorTech tech;

  pex::RExtractorTechVia via1;
  via1.bottom_conductor = l1;
  via1.cut_layer = l2;
  via1.top_conductor = l3;
  via1.resistance = 2.0;
  via1.merge_distance = 0.2;
  tech.vias.push_back (via1);

  pex::RExtractorTechConductor cond1;
  cond1.layer = l1;
  cond1.resistance = 0.5;
  tech.conductors.push_back (cond1);

  pex::RExtractorTechConductor cond2;
  cond2.layer = l3;
  cond2.resistance = 0.25;
  cond2.algorithm = pex::RExtractorTechConductor::Tesselation;
  cond2.triangulation_max_area = 1.5;
  cond2.triangulation_min_b = 0.5;
  tech.conductors.push_back (cond2);

  tech.skip_simplify = true;

  EXPECT_EQ (tech.to_string (),
    "skip_simplify=true\n"
    "Via(bottom=L1, cut=L1, top=L1, R=2 \xC2\xB5m\xC2\xB2*Ohm, d_merge=0.2 \xC2\xB5m)\n"
    "Conductor(layer=L1, R=0.5 Ohm/sq, algo=SquareCounting)\n"
    "Conductor(layer=L1, R=0.25 Ohm/sq, algo=Tesselation, tri_min_b=0.5 \xC2\xB5m, tri_max_area=1.5 \xC2\xB5m\xC2\xB2)"
  );
}

TEST(netex_viagen1)
{
  db::Layout ly;

  {
    std::string fn = tl::testdata () + "/pex/netex_viagen1.gds";
    tl::InputStream is (fn);
    db::Reader reader (is);
    reader.read (ly);
  }

  TestableRNetExtractor rex (ly.dbu ());

  auto tc = ly.cell_by_name ("TOP");
  tl_assert (tc.first);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));

  std::map<unsigned int, db::Region> geo;
  geo.insert (std::make_pair (l2, db::Region (db::RecursiveShapeIterator (ly, ly.cell (tc.second), l2))));

  pex::RNetwork network;

  pex::RExtractorTech tech;

  pex::RExtractorTechVia via1;
  via1.bottom_conductor = l1;
  via1.cut_layer = l2;
  via1.top_conductor = l3;
  via1.resistance = 2.0;
  tech.vias.push_back (via1);

  std::map<unsigned int, std::vector<pex::RNetExtractor::ViaPort> > via_ports;
  rex.create_via_ports (tech, geo, via_ports, network);

  EXPECT_EQ (via_ports [l1].size (), size_t (4));
  EXPECT_EQ (via_ports [l2].size (), size_t (0));
  EXPECT_EQ (via_ports [l3].size (), size_t (4));

  EXPECT_EQ (network2s (network),
    "R (0.4,0.5;0.6,0.7) (0.4,0.5;0.6,0.7) 50\n"
    "R (0.8,0.5;1,0.7) (0.8,0.5;1,0.7) 50\n"
    "R (1.7,0.1;1.9,0.3) (1.7,0.1;1.9,0.3) 50\n"
    "R (2.9,0.5;3.1,0.7) (2.9,0.5;3.1,0.7) 50"
  );
}

TEST(netex_viagen2)
{
  db::Layout ly;

  {
    std::string fn = tl::testdata () + "/pex/netex_viagen2.gds";
    tl::InputStream is (fn);
    db::Reader reader (is);
    reader.read (ly);
  }

  TestableRNetExtractor rex (ly.dbu ());

  auto tc = ly.cell_by_name ("TOP");
  tl_assert (tc.first);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));

  std::map<unsigned int, db::Region> geo;
  geo.insert (std::make_pair (l2, db::Region (db::RecursiveShapeIterator (ly, ly.cell (tc.second), l2))));

  pex::RNetwork network;

  pex::RExtractorTech tech;

  pex::RExtractorTechVia via1;
  via1.bottom_conductor = l1;
  via1.cut_layer = l2;
  via1.top_conductor = l3;
  via1.resistance = 2.0;
  via1.merge_distance = 0.2;
  tech.vias.push_back (via1);

  std::map<unsigned int, std::vector<pex::RNetExtractor::ViaPort> > via_ports;
  rex.create_via_ports (tech, geo, via_ports, network);

  EXPECT_EQ (via_ports [l1].size (), size_t (6));
  EXPECT_EQ (via_ports [l2].size (), size_t (0));
  EXPECT_EQ (via_ports [l3].size (), size_t (6));

  EXPECT_EQ (network2s (network),
    "R (0.4,0.4;2.2,4.2) (0.4,0.4;2.2,4.2) 1\n"
    "R (0.6,4.9;1.2,5.1) (0.6,4.9;1.2,5.1) 25\n"
    "R (2.2,1.2;3.4,3.4) (2.2,1.2;3.4,3.4) 2.77777777778\n"
    "R (2.5,3.7;2.7,3.9) (2.5,3.7;2.7,3.9) 50\n"
    "R (3,3.7;3.2,3.9) (3,3.7;3.2,3.9) 50\n"
    "R (4.6,2.8;4.8,3) (4.6,2.8;4.8,3) 50"
  );
}

TEST(netex_2layer)
{
  db::Layout ly;

  {
    std::string fn = tl::testdata () + "/pex/netex_test1.gds";
    tl::InputStream is (fn);
    db::Reader reader (is);
    reader.read (ly);
  }

  TestableRNetExtractor rex (ly.dbu ());

  auto tc = ly.cell_by_name ("TOP");
  tl_assert (tc.first);

  unsigned int l1  = ly.get_layer (db::LayerProperties (1, 0));
  unsigned int l1p = ly.get_layer (db::LayerProperties (1, 1));
  unsigned int l1v = ly.get_layer (db::LayerProperties (1, 2));
  unsigned int l2  = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l3  = ly.get_layer (db::LayerProperties (3, 0));
  unsigned int l3p = ly.get_layer (db::LayerProperties (3, 1));
  unsigned int l3v = ly.get_layer (db::LayerProperties (3, 2));

  //  That is coincidence, but it needs to be that way for the strings to match
  EXPECT_EQ (l1, 1u);
  EXPECT_EQ (l2, 0u);
  EXPECT_EQ (l3, 2u);

  std::map<unsigned int, db::Region> geo;
  geo.insert (std::make_pair (l1, db::Region (db::RecursiveShapeIterator (ly, ly.cell (tc.second), l1))));
  geo.insert (std::make_pair (l2, db::Region (db::RecursiveShapeIterator (ly, ly.cell (tc.second), l2))));
  geo.insert (std::make_pair (l3, db::Region (db::RecursiveShapeIterator (ly, ly.cell (tc.second), l3))));

  pex::RNetwork network;

  pex::RExtractorTech tech;
  tech.skip_simplify = true;

  pex::RExtractorTechVia via1;
  via1.bottom_conductor = l1;
  via1.cut_layer = l2;
  via1.top_conductor = l3;
  via1.resistance = 2.0;
  via1.merge_distance = 0.2;
  tech.vias.push_back (via1);

  pex::RExtractorTechConductor cond1;
  cond1.layer = l1;
  cond1.resistance = 0.5;
  tech.conductors.push_back (cond1);

  pex::RExtractorTechConductor cond2;
  cond2.layer = l3;
  cond2.resistance = 0.25;
  tech.conductors.push_back (cond2);

  std::map<unsigned int, std::vector<db::Point> > vertex_ports;
  std::map<unsigned int, std::vector<db::Polygon> > polygon_ports;

  db::Region l1p_region (db::RecursiveShapeIterator (ly, ly.cell (tc.second), l1p));
  for (auto p = l1p_region.begin_merged (); ! p.at_end (); ++p) {
    polygon_ports[l1].push_back (*p);
  }

  db::Region l3p_region (db::RecursiveShapeIterator (ly, ly.cell (tc.second), l3p));
  for (auto p = l3p_region.begin_merged (); ! p.at_end (); ++p) {
    polygon_ports[l3].push_back (*p);
  }

  db::Region l1v_region (db::RecursiveShapeIterator (ly, ly.cell (tc.second), l1v));
  for (auto p = l1v_region.begin_merged (); ! p.at_end (); ++p) {
    vertex_ports[l1].push_back (p->box ().center ());
  }

  db::Region l3v_region (db::RecursiveShapeIterator (ly, ly.cell (tc.second), l3v));
  for (auto p = l3v_region.begin_merged (); ! p.at_end (); ++p) {
    vertex_ports[l3].push_back (p->box ().center ());
  }

  rex.extract (tech, geo, vertex_ports, polygon_ports, network);

  EXPECT_EQ (network2s (network),
    "R (0.1,0.1;0.7,0.7) (0.1,0.1;0.7,0.7) 12.5\n"
    "R (0.1,0.1;0.7,0.7) V0.1(5.2,0.4;5.2,0.4) 3\n"
    "R (0.1,0.1;0.7,0.7) V0.2(0.4,-5.6;0.4,-5.6) 1.875\n"
    "R (0.3,-5.7;0.5,-5.5) (0.3,-5.7;0.5,-5.5) 50\n"
    "R (0.3,-5.7;0.5,-5.5) (9.3,-5.9;9.9,-5.3) 5.75\n"
    "R (0.3,-5.7;0.5,-5.5) V0.2(0.4,-5.6;0.4,-5.6) 0\n"
    "R (10,-3.5;10,-2.7) (9.3,-5.9;9.9,-5.3) 0.78125\n"
    "R (10,-3.5;10,-2.7) (9.3,0.1;9.9,0.3) 1.03125\n"
    "R (10,-3.5;10,-2.7) P0.2(12.9,-3.4;13.5,-2.8) 1\n"
    "R (9.3,-5.9;9.9,-5.3) (9.3,-5.9;9.9,-5.3) 12.5\n"
    "R (9.3,-5.9;9.9,-5.3) P0.1(12.9,-5.9;13.5,-5.3) 2.25\n"
    "R (9.3,0.1;9.9,0.3) (9.3,0.1;9.9,0.3) 25\n"
    "R (9.3,0.1;9.9,0.3) V0.1(5.2,0.4;5.2,0.4) 2.75"
  );

  tech.skip_simplify = false;

  rex.extract (tech, geo, vertex_ports, polygon_ports, network);

  EXPECT_EQ (network2s (network),
    "R (10,-3.5;10,-2.7) (9.3,-5.9;9.9,-5.3) 13.28125\n"
    "R (10,-3.5;10,-2.7) P0.2(12.9,-3.4;13.5,-2.8) 1\n"
    "R (10,-3.5;10,-2.7) V0.1(5.2,0.4;5.2,0.4) 28.78125\n"
    "R (9.3,-5.9;9.9,-5.3) P0.1(12.9,-5.9;13.5,-5.3) 2.25\n"
    "R (9.3,-5.9;9.9,-5.3) V0.2(0.3,-5.7;0.5,-5.5) 55.75\n"
    "R V0.1(5.2,0.4;5.2,0.4) V0.2(0.3,-5.7;0.5,-5.5) 17.375"
  );
}
