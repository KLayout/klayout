
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

#include "dbEdgePairs.h"
#include "dbEdgePairFilters.h"
#include "dbEdges.h"
#include "dbRegion.h"
#include "dbTestSupport.h"

TEST(1) 
{
  db::EdgePairs ep;
  EXPECT_EQ (ep.empty (), true);
  EXPECT_EQ (ep.bbox ().to_string (), "()");
  EXPECT_EQ (ep == db::EdgePairs (), true);
  EXPECT_EQ (ep < db::EdgePairs (), false);
  EXPECT_EQ (ep != db::EdgePairs (), false);
  ep.insert (db::Edge (db::Point (10, 20), db::Point (110, 120)), db::Edge (db::Point (-10, -20), db::Point (90, 80)));
  EXPECT_EQ (ep.empty (), false);
  EXPECT_EQ (ep.count (), size_t (1));
  EXPECT_EQ (ep.hier_count (), size_t (1));
  EXPECT_EQ (ep.bbox ().to_string (), "(-10,-20;110,120)");
  EXPECT_EQ (ep.to_string (), "(10,20;110,120)/(-10,-20;90,80)");

  ep.clear ();
  EXPECT_EQ (ep.empty (), true);
  EXPECT_EQ (ep.count (), size_t (0));
  EXPECT_EQ (ep.hier_count (), size_t (0));
  EXPECT_EQ (ep.bbox ().to_string (), "()");
  ep.insert (db::EdgePair (db::Edge (db::Point (10, 20), db::Point (110, 120)), db::Edge (db::Point (-10, -20), db::Point (90, 80))));
  EXPECT_EQ (ep == db::EdgePairs (), false);
  EXPECT_EQ (ep < db::EdgePairs (), true);
  EXPECT_EQ (ep != db::EdgePairs (), true);
  EXPECT_EQ (ep != ep, false);
  EXPECT_EQ (ep == ep, true);
  EXPECT_EQ (ep < ep, false);
  EXPECT_EQ (ep.empty (), false);
  EXPECT_EQ (ep.bbox ().to_string (), "(-10,-20;110,120)");
  EXPECT_EQ (ep.to_string (), "(10,20;110,120)/(-10,-20;90,80)");

  EXPECT_EQ (ep.transformed (db::ICplxTrans (2.0, 0.0, false, db::Vector ())).to_string (), "(20,40;220,240)/(-20,-40;180,160)");
  EXPECT_EQ (ep.to_string (), "(10,20;110,120)/(-10,-20;90,80)");
  ep.transform (db::ICplxTrans (3));
  EXPECT_EQ (ep.empty (), false);
  EXPECT_EQ (ep.bbox ().to_string (), "(-20,-110;120,10)");
  EXPECT_EQ (ep.to_string (), "(20,-10;120,-110)/(-20,10;80,-90)");

  db::EdgePairs ep2;
  EXPECT_EQ (ep2.empty (), true);
  EXPECT_EQ (ep2.count (), size_t (0));
  EXPECT_EQ (ep2.hier_count (), size_t (0));
  EXPECT_EQ (ep2.bbox ().to_string (), "()");
  ep2.swap (ep);
  EXPECT_EQ (ep.empty (), true);
  EXPECT_EQ (ep.count (), size_t (0));
  EXPECT_EQ (ep.hier_count (), size_t (0));
  EXPECT_EQ (ep.bbox ().to_string (), "()");
  EXPECT_EQ (ep2.empty (), false);
  EXPECT_EQ (ep2.count (), size_t (1));
  EXPECT_EQ (ep2.hier_count (), size_t (1));
  EXPECT_EQ (ep2.bbox ().to_string (), "(-20,-110;120,10)");
}

TEST(2) 
{
  db::EdgePairs ep;
  ep.insert (db::EdgePair (db::Edge (db::Point (10, 20), db::Point (110, 120)), db::Edge (db::Point (-10, -20), db::Point (90, 80))));
  ep.insert (db::EdgePair (db::Edge (db::Point (10, 20), db::Point (110, 120)), db::Edge (db::Point (90, 80), db::Point (-10, -20))));

  EXPECT_EQ (db::compare (ep, "(10,20;110,120)/(-10,-20;90,80);(10,20;110,120)/(90,80;-10,-20)"), true);

  db::EdgePairs ee;
  std::string s = ep.to_string ();
  tl::Extractor ex (s.c_str ());
  EXPECT_EQ (ex.try_read (ee), true);
  EXPECT_EQ (db::compare (ee, "(10,20;110,120)/(-10,-20;90,80);(10,20;110,120)/(90,80;-10,-20)"), true);

  db::Edges e;
  ep.edges (e);
  EXPECT_EQ (db::compare (e, "(10,20;110,120);(-10,-20;90,80);(10,20;110,120);(90,80;-10,-20)"), true);
  e.clear ();
  ep.first_edges (e);
  EXPECT_EQ (db::compare (e, "(10,20;110,120);(10,20;110,120)"), true);
  e.clear ();
  ep.second_edges (e);
  EXPECT_EQ (db::compare (e, "(-10,-20;90,80);(90,80;-10,-20)"), true);

  db::Region r;
  ep.polygons (r);
  EXPECT_EQ (db::compare (r, "(-10,-20;10,20;110,120;90,80);(-10,-20;10,20;110,120;90,80)"), true);
}

struct EPTestFilter
  : public db::EdgePairFilterBase
{
  bool selected (const db::EdgePair &ep, db::properties_id_type) const
  {
    return ep.first ().double_length () < 50;
  }

  const db::TransformationReducer *vars () const
  {
    return &m_vars;
  }

  bool wants_variants () const
  {
    return false;
  }

private:
  db::MagnificationReducer m_vars;
};

TEST(3) 
{
  db::EdgePairs ep;
  ep.insert (db::EdgePair (db::Edge (db::Point (10, 20), db::Point (50, 50)), db::Edge (db::Point (-10, -20), db::Point (90, 80))));
  ep.insert (db::EdgePair (db::Edge (db::Point (10, 20), db::Point (110, 120)), db::Edge (db::Point (90, 80), db::Point (-10, -20))));

  EPTestFilter f;
  EXPECT_EQ (ep.filtered (f).to_string (), "");
  ep.filter (f);
  EXPECT_EQ (ep.to_string (), "");
}

TEST(4)
{
  db::EdgePairs ep;
  ep.insert (db::EdgePair (db::Edge (db::Point (10, 20), db::Point (50, 50)), db::Edge (db::Point (-10, -20), db::Point (90, 80))));
  ep.insert (db::EdgePair (db::Edge (db::Point (10, 20), db::Point (110, 120)), db::Edge (db::Point (90, 80), db::Point (-10, -20))));

  db::Layout ly;
  unsigned int l1 = ly.insert_layer (db::LayerProperties (1, 0));
  db::cell_index_type top_cell = ly.add_cell ("TOP");

  ep.insert_into_as_polygons (&ly, top_cell, l1, 1);

  db::Region r (db::RecursiveShapeIterator (ly, ly.cell (top_cell), l1));
  EXPECT_EQ (db::compare (r, "(-10,-21;9,20;50,51;91,80);(-10,-21;9,20;110,121;91,80)"), true);
}

TEST(5_InternalAngleFilter)
{
  db::EdgePair ep0 (db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (100, 0), db::Point (0, 0)));
  db::EdgePair ep45 (db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, 0), db::Point (100, 100)));
  db::EdgePair ep45inv (db::Edge (db::Point (0, 0), db::Point (100, 100)), db::Edge (db::Point (0, 0), db::Point (100, 0)));
  db::EdgePair ep180 (db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, 0), db::Point (100, 0)));
  db::EdgePair ep90 (db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, 0), db::Point (0, 100)));
  db::EdgePair epm90 (db::Edge (db::Point (0, 0), db::Point (100, 0)), db::Edge (db::Point (0, 100), db::Point (0, 0)));

  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, false).selected (ep0, 0), true);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, false).selected (ep180, 0), true);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, false).selected (ep90, 0), false);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, false).selected (epm90, 0), false);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, false).selected (ep45, 0), false);

  EXPECT_EQ (db::InternalAngleEdgePairFilter (90.0, false).selected (ep0, 0), false);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (90.0, false).selected (ep180, 0), false);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (90.0, false).selected (ep90, 0), true);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (90.0, false).selected (epm90, 0), true);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (90.0, false).selected (ep45, 0), false);

  EXPECT_EQ (db::InternalAngleEdgePairFilter (45.0, false).selected (ep0, 0), false);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (45.0, false).selected (ep180, 0), false);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (45.0, false).selected (ep90, 0), false);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (45.0, false).selected (epm90, 0), false);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (45.0, false).selected (ep45, 0), true);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (45.0, false).selected (ep45inv, 0), true);

  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, true).selected (ep0, 0), false);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, true).selected (ep180, 0), false);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, true).selected (ep90, 0), true);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, true).selected (epm90, 0), true);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, true).selected (ep45, 0), true);

  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, true, 45.0, true, false).selected (ep0, 0), true);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, true, 45.0, true, false).selected (ep180, 0), true);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, true, 45.0, true, false).selected (ep90, 0), false);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, true, 45.0, true, false).selected (epm90, 0), false);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, true, 45.0, true, false).selected (ep45, 0), true);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, true, 45.0, true, false).selected (ep45inv, 0), true);

  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, true, 45.0, true, true).selected (ep0, 0), false);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, true, 45.0, true, true).selected (ep180, 0), false);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, true, 45.0, true, true).selected (ep90, 0), true);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, true, 45.0, true, true).selected (epm90, 0), true);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, true, 45.0, true, true).selected (ep45, 0), false);
  EXPECT_EQ (db::InternalAngleEdgePairFilter (0.0, true, 45.0, true, true).selected (ep45inv, 0), false);
}

TEST(6_add_with_properties)
{
  db::DeepShapeStore dss ("TOP", 0.001);
  db::EdgePairs rd1 (dss), rd2 (dss);
  db::EdgePairs rf1, rf2;

  db::PropertiesSet ps;
  ps.insert ("net", 17);
  db::properties_id_type pid = db::properties_id (ps);

  rf1.insert (db::EdgePairWithProperties (db::EdgePair (db::Edge (-10, 20, 20, 60), db::Edge (-10, 30, 20, 70)), pid));
  rd1.insert (db::EdgePairWithProperties (db::EdgePair (db::Edge (-10, 20, 20, 60), db::Edge (-10, 30, 20, 70)), pid));

  rf2.insert (db::EdgePairWithProperties (db::EdgePair (db::Edge (10, 20, -20, 60), db::Edge (10, 30, -20, 70)), pid));
  rd2.insert (db::EdgePairWithProperties (db::EdgePair (db::Edge (10, 20, -20, 60), db::Edge (10, 30, -20, 70)), pid));

  db::Layout ly;
  db::Cell &top_cell = ly.cell (ly.add_cell ("TOP"));
  unsigned int l1 = ly.insert_layer ();
  unsigned int l2 = ly.insert_layer ();

  top_cell.shapes (l1).insert (db::EdgePairWithProperties (db::EdgePair (db::Edge (-10, 20, 20, 60), db::Edge (-10, 30, 20, 70)), pid));
  top_cell.shapes (l2).insert (db::EdgePairWithProperties (db::EdgePair (db::Edge (10, 20, -20, 60), db::Edge (10, 30, -20, 70)), pid));

  db::EdgePairs ro1 (db::RecursiveShapeIterator (ly, top_cell, l1));
  db::EdgePairs ro2 (db::RecursiveShapeIterator (ly, top_cell, l2));

  //  enable properties
  ro1.apply_property_translator (db::PropertiesTranslator::make_pass_all ());
  ro2.apply_property_translator (db::PropertiesTranslator::make_pass_all ());

  db::EdgePairs r;
  r += rf1;
  r += rf2;
  EXPECT_EQ (r.to_string (), "(-10,20;20,60)/(-10,30;20,70){net=>17};(10,20;-20,60)/(10,30;-20,70){net=>17}");
  EXPECT_EQ ((rf1 + rf2).to_string (), "(-10,20;20,60)/(-10,30;20,70){net=>17};(10,20;-20,60)/(10,30;-20,70){net=>17}");

  r = db::EdgePairs ();
  r += rd1;
  r += rf2;
  EXPECT_EQ (r.to_string (), "(-10,20;20,60)/(-10,30;20,70){net=>17};(10,20;-20,60)/(10,30;-20,70){net=>17}");
  EXPECT_EQ ((rd1 + rf2).to_string (), "(-10,20;20,60)/(-10,30;20,70){net=>17};(10,20;-20,60)/(10,30;-20,70){net=>17}");

  r = db::EdgePairs ();
  r += rf1;
  r += rd2;
  EXPECT_EQ (r.to_string (), "(-10,20;20,60)/(-10,30;20,70){net=>17};(10,20;-20,60)/(10,30;-20,70){net=>17}");
  EXPECT_EQ ((rf1 + rd2).to_string (), "(-10,20;20,60)/(-10,30;20,70){net=>17};(10,20;-20,60)/(10,30;-20,70){net=>17}");

  r = db::EdgePairs ();
  r += rd1;
  r += rd2;
  EXPECT_EQ (r.to_string (), "(-10,20;20,60)/(-10,30;20,70){net=>17};(10,20;-20,60)/(10,30;-20,70){net=>17}");
  EXPECT_EQ ((rd1 + rd2).to_string (), "(-10,20;20,60)/(-10,30;20,70){net=>17};(10,20;-20,60)/(10,30;-20,70){net=>17}");

  r = db::EdgePairs ();
  r += ro1;
  r += ro2;
  EXPECT_EQ (r.to_string (), "(-10,20;20,60)/(-10,30;20,70){net=>17};(10,20;-20,60)/(10,30;-20,70){net=>17}");
  EXPECT_EQ ((ro1 + ro2).to_string (), "(-10,20;20,60)/(-10,30;20,70){net=>17};(10,20;-20,60)/(10,30;-20,70){net=>17}");

  r = db::EdgePairs ();
  r += ro1;
  r += rf2;
  EXPECT_EQ (r.to_string (), "(10,20;-20,60)/(10,30;-20,70){net=>17};(-10,20;20,60)/(-10,30;20,70){net=>17}");
  EXPECT_EQ ((ro1 + rf2).to_string (), "(10,20;-20,60)/(10,30;-20,70){net=>17};(-10,20;20,60)/(-10,30;20,70){net=>17}");
}

TEST(7_properties)
{
  db::PropertiesSet ps;

  ps.insert (tl::Variant ("id"), 1);
  db::properties_id_type pid1 = db::properties_id (ps);

  db::EdgePairs edge_pairs;
  db::Edge e1 (db::Point (0, 0), db::Point (10, 20));
  db::Edge e2 (db::Point (1, 2), db::Point (11, 22));
  edge_pairs.insert (db::EdgePairWithProperties (db::EdgePair (e1, e2), pid1));
  edge_pairs.insert (db::EdgePair (e1, e2));

  EXPECT_EQ (edge_pairs.nth (0)->to_string (), "(0,0;10,20)/(1,2;11,22)");
  EXPECT_EQ (edge_pairs.nth (1)->to_string (), "(0,0;10,20)/(1,2;11,22)");
  EXPECT_EQ (edge_pairs.nth (2) == 0, true);

  EXPECT_EQ (edge_pairs.nth_prop_id (0), db::properties_id_type (0));
  EXPECT_EQ (edge_pairs.nth_prop_id (1), pid1);
}

