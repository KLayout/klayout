
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

#include "dbRegion.h"
#include "dbEdgePairs.h"
#include "dbEdges.h"
#include "dbEdgesUtils.h"
#include "dbCompoundOperation.h"
#include "dbReader.h"
#include "dbRecursiveShapeIterator.h"
#include "dbRegionUtils.h"
#include "dbTestSupport.h"

#include "tlStream.h"

#include <cstdio>

static void prep_layer (db::Layout &ly, int gds_layer, db::Region &r, db::DeepShapeStore &dss, bool deep)
{
  unsigned int li = ly.get_layer (db::LayerProperties (gds_layer, 0));
  if (deep) {
    r = db::Region (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), li), dss);
  } else {
    r = db::Region (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), li));
  }
}

static std::string make_au (const std::string &num, bool deep)
{
  if (deep) {
    return tl::testdata () + "/drc/compound_au" + num + "d.gds";
  } else {
    return tl::testdata () + "/drc/compound_au" + num + ".gds";
  }
}

void run_test1 (tl::TestBase *_this, bool deep)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/drc/compound_1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  db::CompoundRegionCheckOperationNode width_check (db::WidthRelation, false /*==same polygon*/, 1050, check_options);

  db::DeepShapeStore dss;

  db::Region r, r2;
  prep_layer (ly, 1, r, dss, deep);
  prep_layer (ly, 2, r2, dss, deep);

  db::EdgePairs res = r.cop_to_edge_pairs (width_check);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();
  db::CompoundRegionCheckOperationNode isolation_check (primary, db::SpaceRelation, true /*==different polygons*/, 1050, check_options);

  res = r.cop_to_edge_pairs (isolation_check);

  unsigned int l1001 = ly.get_layer (db::LayerProperties (1001, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1001);

  db::CompoundRegionOperationSecondaryNode *secondary = new db::CompoundRegionOperationSecondaryNode (&r2);
  db::CompoundRegionCheckOperationNode sep_check (secondary, db::SpaceRelation, true /*==different polygons*/, 1050, check_options);

  res = r.cop_to_edge_pairs (sep_check);

  unsigned int l1002 = ly.get_layer (db::LayerProperties (1002, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1002);

  primary = new db::CompoundRegionOperationPrimaryNode ();
  db::CompoundRegionCheckOperationNode space_check (primary, db::SpaceRelation, false /*==all polygons*/, 1050, check_options);

  res = r.cop_to_edge_pairs (space_check);

  unsigned int l1003 = ly.get_layer (db::LayerProperties (1003, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1003);

  CHECKPOINT();
  db::compare_layouts (_this, ly, make_au ("1", deep));
}

TEST(1_Basic)
{
  run_test1 (_this, false);
}

TEST(1d_Basic)
{
  run_test1 (_this, true);
}

void run_test2 (tl::TestBase *_this, bool deep)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/drc/compound_2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  db::DeepShapeStore dss;

  db::Region r, r2;
  prep_layer (ly, 1, r, dss, deep);
  prep_layer (ly, 2, r2, dss, deep);

  db::CompoundRegionCheckOperationNode *width_check = new db::CompoundRegionCheckOperationNode (db::WidthRelation, false /*==same polygon*/, 1050, check_options);

  db::CompoundRegionEdgePairToPolygonProcessingOperationNode ep2p (new db::EdgePairToPolygonProcessor (0), width_check, true);
  db::Region res = r.cop_to_region (ep2p);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  db::CompoundRegionEdgePairToEdgeProcessingOperationNode ep2e1 (new db::EdgePairToLesserEdgesProcessor (), width_check, true);
  db::Edges eres = r.cop_to_edges (ep2e1);

  unsigned int l1001 = ly.get_layer (db::LayerProperties (1001, 0));
  eres.insert_into (&ly, *ly.begin_top_down (), l1001);

  db::CompoundRegionEdgePairToEdgeProcessingOperationNode ep2e2 (new db::EdgePairToGreaterEdgesProcessor (), width_check, true);
  eres = r.cop_to_edges (ep2e2);

  unsigned int l1002 = ly.get_layer (db::LayerProperties (1002, 0));
  eres.insert_into (&ly, *ly.begin_top_down (), l1002);

  CHECKPOINT();
  db::compare_layouts (_this, ly, make_au ("2", deep));
}

TEST(2_ChainedOperations)
{
  run_test2 (_this, false);
}

TEST(2d_ChainedOperations)
{
  run_test2 (_this, true);
}

void run_test3 (tl::TestBase *_this, bool deep)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/drc/compound_3.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::DeepShapeStore dss;

  db::Region r, r2;
  prep_layer (ly, 1, r, dss, deep);
  prep_layer (ly, 2, r2, dss, deep);

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();
  db::CompoundRegionOperationSecondaryNode *secondary = new db::CompoundRegionOperationSecondaryNode (&r2);
  db::CompoundRegionGeometricalBoolOperationNode geo_bool (db::CompoundRegionGeometricalBoolOperationNode::And, primary, secondary);

  db::Region res = r.cop_to_region (geo_bool);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  db::CompoundRegionGeometricalBoolOperationNode geo_bool2 (db::CompoundRegionGeometricalBoolOperationNode::Not, primary, secondary);

  res = r.cop_to_region (geo_bool2);

  unsigned int l1001 = ly.get_layer (db::LayerProperties (1001, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1001);

  CHECKPOINT();
  db::compare_layouts (_this, ly, make_au ("3", deep));
}

TEST(3_BooleanOperations)
{
  run_test3 (_this, false);
}

TEST(3d_BooleanOperations)
{
  run_test3 (_this, true);
}

void run_test4 (tl::TestBase *_this, bool deep)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/drc/compound_4.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::DeepShapeStore dss;

  db::Region r, r2;
  prep_layer (ly, 1, r, dss, deep);
  prep_layer (ly, 2, r2, dss, deep);

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();
  db::CompoundRegionSizeOperationNode geo_size (250, 250, 2, primary);

  db::Region res = r.cop_to_region (geo_size);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  db::CompoundRegionOperationSecondaryNode *secondary = new db::CompoundRegionOperationSecondaryNode (&r2);
  db::CompoundRegionSizeOperationNode geo_size2 (-250, -250, 2, secondary);

  res = r.cop_to_region (geo_size2);

  unsigned int l1001 = ly.get_layer (db::LayerProperties (1001, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1001);

  CHECKPOINT();
  db::compare_layouts (_this, ly, make_au ("4", deep));
}

TEST(4_SizeOperation)
{
  run_test4 (_this, false);
}

TEST(4d_SizeOperation)
{
  run_test4 (_this, true);
}

void run_test5 (tl::TestBase *_this, bool deep)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/drc/compound_5.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  db::DeepShapeStore dss;

  db::Region r, r2;
  prep_layer (ly, 1, r, dss, deep);
  prep_layer (ly, 2, r2, dss, deep);

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();
  db::CompoundRegionOperationSecondaryNode *secondary = new db::CompoundRegionOperationSecondaryNode (&r2);
  db::CompoundRegionInteractOperationNode interact (primary, secondary, 0, true, false);

  db::Region res = r.cop_to_region (interact);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  CHECKPOINT();
  db::compare_layouts (_this, ly, make_au ("5", deep));
}

TEST(5_InteractOperation)
{
  run_test5 (_this, false);
}

TEST(5d_InteractOperation)
{
  run_test5 (_this, true);
}

void run_test6 (tl::TestBase *_this, bool deep)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/drc/compound_6.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  db::DeepShapeStore dss;

  db::Region r, r2;
  prep_layer (ly, 1, r, dss, deep);
  prep_layer (ly, 2, r2, dss, deep);

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();
  db::CompoundRegionOperationSecondaryNode *secondary = new db::CompoundRegionOperationSecondaryNode (&r2);
  db::CompoundRegionToEdgeProcessingOperationNode *secondary_edges = new db::CompoundRegionToEdgeProcessingOperationNode (new db::PolygonToEdgeProcessor (), secondary, true);

  db::CompoundRegionInteractWithEdgeOperationNode interact (primary, secondary_edges, false);

  db::Region res = r.cop_to_region (interact);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  CHECKPOINT();
  db::compare_layouts (_this, ly, make_au ("6", deep));
}

TEST(6_InteractWithEdgeOperation)
{
  run_test6 (_this, false);
}

TEST(6d_InteractWithEdgeOperation)
{
  run_test6 (_this, true);
}

void run_test7 (tl::TestBase *_this, bool deep)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/drc/compound_7.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  db::DeepShapeStore dss;

  db::Region r, r2;
  prep_layer (ly, 1, r, dss, deep);
  prep_layer (ly, 2, r2, dss, deep);

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();
  db::CompoundRegionOperationSecondaryNode *secondary = new db::CompoundRegionOperationSecondaryNode (&r2);

  db::CompoundRegionPullOperationNode pull (primary, secondary, 0, true);

  db::Region res = r.cop_to_region (pull);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  CHECKPOINT();
  db::compare_layouts (_this, ly, make_au ("7", deep));
}

TEST(7_PullOperation)
{
  run_test7 (_this, false);
}

TEST(7d_PullOperation)
{
  run_test7 (_this, true);
}

void run_test8 (tl::TestBase *_this, bool deep)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/drc/compound_8.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  db::DeepShapeStore dss;

  db::Region r, r2;
  prep_layer (ly, 1, r, dss, deep);
  prep_layer (ly, 2, r2, dss, deep);

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();
  db::CompoundRegionOperationSecondaryNode *secondary = new db::CompoundRegionOperationSecondaryNode (&r2);
  db::CompoundRegionToEdgeProcessingOperationNode *secondary_edges = new db::CompoundRegionToEdgeProcessingOperationNode (new db::PolygonToEdgeProcessor (), secondary, true);

  db::CompoundRegionPullWithEdgeOperationNode pull (primary, secondary_edges);

  db::Edges res = r.cop_to_edges (pull);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  CHECKPOINT();
  db::compare_layouts (_this, ly, make_au ("8", deep));
}

TEST(8_PullWithEdgeOperation)
{
  run_test8 (_this, false);
}

TEST(8d_PullWithEdgeOperation)
{
  run_test8 (_this, true);
}

void run_test9 (tl::TestBase *_this, bool deep)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/drc/compound_9.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  db::DeepShapeStore dss;

  db::Region r, r2;
  prep_layer (ly, 1, r, dss, deep);
  prep_layer (ly, 2, r2, dss, deep);

  //  the if/then ladder is:
  //
  //   if (area > 10um2) return sized(+50nm)
  //   else if (is_rectangle) return sized(-50nm)
  //   else return bbox

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();

  std::vector<db::CompoundRegionOperationNode *> inputs;

  db::CompoundRegionFilterOperationNode *condition1 = new db::CompoundRegionFilterOperationNode (new db::RegionAreaFilter (0, 10000000, true), primary, true);
  inputs.push_back (condition1);

  db::CompoundRegionSizeOperationNode *result1 = new db::CompoundRegionSizeOperationNode (50, 50, 2, primary);
  inputs.push_back (result1);

  db::CompoundRegionFilterOperationNode *condition2 = new db::CompoundRegionFilterOperationNode (new db::RectangleFilter (false, false), primary, true);
  inputs.push_back (condition2);

  db::CompoundRegionSizeOperationNode *result2 = new db::CompoundRegionSizeOperationNode (-50, -50, 2, primary);
  inputs.push_back (result2);

  db::CompoundRegionProcessingOperationNode *result_default = new db::CompoundRegionProcessingOperationNode (new db::Extents (), primary, true);
  inputs.push_back (result_default);

  db::CompoundRegionLogicalCaseSelectOperationNode select_node (inputs);

  db::Region res = r.cop_to_region (select_node);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  CHECKPOINT();
  db::compare_layouts (_this, ly, make_au ("9", deep));
}

TEST(9_LogicalSelectOperation)
{
  run_test9 (_this, false);
}

TEST(9d_LogicalSelectOperation)
{
  run_test9 (_this, true);
}

void run_test10 (tl::TestBase *_this, bool deep)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/drc/compound_10.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  db::DeepShapeStore dss;

  db::Region r, r2;
  prep_layer (ly, 1, r, dss, deep);
  prep_layer (ly, 2, r2, dss, deep);

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();

  db::CompoundRegionFilterOperationNode *condition1 = new db::CompoundRegionFilterOperationNode (new db::RegionAreaFilter (0, 10000000, true), primary, true);
  db::CompoundRegionFilterOperationNode *condition1r = new db::CompoundRegionFilterOperationNode (new db::RegionAreaFilter (0, 10000000, false), primary, true);

  db::CompoundRegionFilterOperationNode *condition2 = new db::CompoundRegionFilterOperationNode (new db::RectangleFilter (false, false), primary, true);

  std::vector<db::CompoundRegionOperationNode *> inputs;
  inputs.push_back (condition1r);
  inputs.push_back (condition2);

  db::CompoundRegionLogicalBoolOperationNode and_node (db::CompoundRegionLogicalBoolOperationNode::And, false, inputs);
  db::CompoundRegionLogicalBoolOperationNode not_and_node (db::CompoundRegionLogicalBoolOperationNode::And, true, inputs);

  inputs.clear ();
  inputs.push_back (condition1);
  inputs.push_back (condition2);

  db::CompoundRegionLogicalBoolOperationNode or_node (db::CompoundRegionLogicalBoolOperationNode::Or, false, inputs);
  db::CompoundRegionLogicalBoolOperationNode not_or_node (db::CompoundRegionLogicalBoolOperationNode::Or, true, inputs);

  db::Region res;
  res = r.cop_to_region (and_node);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  res = r.cop_to_region (not_and_node);

  unsigned int l1001 = ly.get_layer (db::LayerProperties (1001, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1001);

  res = r.cop_to_region (or_node);

  unsigned int l1002 = ly.get_layer (db::LayerProperties (1002, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1002);

  res = r.cop_to_region (not_or_node);

  unsigned int l1003 = ly.get_layer (db::LayerProperties (1003, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1003);

  CHECKPOINT();
  db::compare_layouts (_this, ly, make_au ("10", deep));
}

TEST(10_LogicalAndNotOperation)
{
  run_test10 (_this, false);
}

TEST(10d_LogicalAndNotOperation)
{
  run_test10 (_this, true);
}

void run_test11 (tl::TestBase *_this, bool deep)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/drc/compound_11.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  db::DeepShapeStore dss;

  db::Region r, r2;
  prep_layer (ly, 1, r, dss, deep);
  prep_layer (ly, 2, r2, dss, deep);

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();
  db::CompoundRegionToEdgeProcessingOperationNode *primary_edges = new db::CompoundRegionToEdgeProcessingOperationNode (new db::PolygonToEdgeProcessor (), primary, true);

  db::CompoundRegionEdgeFilterOperationNode edge_filter (new db::EdgeLengthFilter (3000, 5000, false), primary_edges, true);
  db::CompoundRegionEdgeFilterOperationNode edge_filterr (new db::EdgeLengthFilter (3000, 5000, true), primary_edges, true);

  db::Edges res = r.cop_to_edges (edge_filter);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  res = r.cop_to_edges (edge_filterr);

  unsigned int l1001 = ly.get_layer (db::LayerProperties (1001, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1001);

  CHECKPOINT();
  db::compare_layouts (_this, ly, make_au ("11", deep));
}

TEST(11_EdgeFilterOperation)
{
  run_test11 (_this, false);
}

TEST(11d_EdgeFilterOperation)
{
  run_test11 (_this, true);
}

void run_test12 (tl::TestBase *_this, bool deep)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/drc/compound_12.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  db::DeepShapeStore dss;

  db::Region r, r2;
  prep_layer (ly, 1, r, dss, deep);
  prep_layer (ly, 2, r2, dss, deep);

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();
  db::CompoundRegionToEdgeProcessingOperationNode *primary_edges = new db::CompoundRegionToEdgeProcessingOperationNode (new db::PolygonToEdgeProcessor (), primary, true);

  db::CompoundRegionOperationSecondaryNode *secondary = new db::CompoundRegionOperationSecondaryNode (&r2);
  db::CompoundRegionToEdgeProcessingOperationNode *secondary_edges = new db::CompoundRegionToEdgeProcessingOperationNode (new db::PolygonToEdgeProcessor (), secondary, true);

  db::CompoundRegionGeometricalBoolOperationNode geo_bool (db::CompoundRegionGeometricalBoolOperationNode::And, primary_edges, secondary);

  db::Edges res = r.cop_to_edges (geo_bool);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  db::CompoundRegionGeometricalBoolOperationNode geo_bool2 (db::CompoundRegionGeometricalBoolOperationNode::Not, primary_edges, secondary);

  res = r.cop_to_edges (geo_bool2);

  unsigned int l1001 = ly.get_layer (db::LayerProperties (1001, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1001);

  db::CompoundRegionGeometricalBoolOperationNode geo_bool3 (db::CompoundRegionGeometricalBoolOperationNode::And, primary, secondary_edges);

  res = r.cop_to_edges (geo_bool3);

  unsigned int l1002 = ly.get_layer (db::LayerProperties (1002, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1002);

  db::CompoundRegionGeometricalBoolOperationNode geo_bool5 (db::CompoundRegionGeometricalBoolOperationNode::And, primary_edges, secondary_edges);

  res = r.cop_to_edges (geo_bool5);

  unsigned int l1004 = ly.get_layer (db::LayerProperties (1004, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1004);

  db::CompoundRegionGeometricalBoolOperationNode geo_bool6 (db::CompoundRegionGeometricalBoolOperationNode::Not, primary_edges, secondary_edges);

  res = r.cop_to_edges (geo_bool6);

  unsigned int l1005 = ly.get_layer (db::LayerProperties (1005, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1005);

  CHECKPOINT();
  db::compare_layouts (_this, ly, make_au ("12", deep));
}

TEST(12_EdgeBooleanOperations)
{
  run_test12 (_this, false);
}

TEST(12d_EdgeBooleanOperations)
{
  run_test12 (_this, true);
}

void run_test13 (tl::TestBase *_this, bool deep)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/drc/compound_13.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  db::DeepShapeStore dss;

  db::Region r, r2;
  prep_layer (ly, 1, r, dss, deep);
  prep_layer (ly, 2, r2, dss, deep);

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();
  db::CompoundRegionToEdgeProcessingOperationNode *primary_edges = new db::CompoundRegionToEdgeProcessingOperationNode (new db::PolygonToEdgeProcessor (), primary, true);

  db::CompoundRegionEdgeProcessingOperationNode edge_proc (new db::EdgeSegmentSelector (-1, 1000, 0.1), primary_edges, true);

  db::Edges res = r.cop_to_edges (edge_proc);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  CHECKPOINT();
  db::compare_layouts (_this, ly, make_au ("13", deep));
}

TEST(13_EdgeProcessor)
{
  run_test13 (_this, false);
}

TEST(13d_EdgeProcessor)
{
  run_test13 (_this, true);
}

void run_test14 (tl::TestBase *_this, bool deep)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/drc/compound_14.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  db::DeepShapeStore dss;

  db::Region r, r2;
  prep_layer (ly, 1, r, dss, deep);
  prep_layer (ly, 2, r2, dss, deep);

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();
  db::CompoundRegionOperationSecondaryNode *secondary = new db::CompoundRegionOperationSecondaryNode (&r2);

  std::vector<db::CompoundRegionOperationNode *> inputs;
  inputs.push_back (primary);
  inputs.push_back (secondary);
  db::CompoundRegionJoinOperationNode *join = new db::CompoundRegionJoinOperationNode (inputs);

  EXPECT_EQ (join->result_type () == db::CompoundRegionJoinOperationNode::Region, true);

  db::CompoundRegionMergeOperationNode merge1 (false, 0, join);
  db::CompoundRegionMergeOperationNode merge2 (false, 1, join);

  db::Region res1 = r.cop_to_region (merge1);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res1.insert_into (&ly, *ly.begin_top_down (), l1000);

  db::Region res2 = r.cop_to_region (merge2);

  unsigned int l1001 = ly.get_layer (db::LayerProperties (1001, 0));
  res2.insert_into (&ly, *ly.begin_top_down (), l1001);

  CHECKPOINT();
  db::compare_layouts (_this, ly, make_au ("14", deep));
}

TEST(14_JoinAndMerged)
{
  run_test14 (_this, false);
}

TEST(14d_JoinAndMerged)
{
  run_test14 (_this, true);
}

void run_test15 (tl::TestBase *_this, bool deep)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/drc/compound_15.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  db::DeepShapeStore dss;

  db::Region r, r2;
  prep_layer (ly, 1, r, dss, deep);
  prep_layer (ly, 2, r2, dss, deep);

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();

  db::CompoundRegionProcessingOperationNode *corners1 = new db::CompoundRegionProcessingOperationNode (new db::CornersAsRectangles (-180.0, true, 180.0, true, false, false, 1), primary, true /*processor is owned*/);
  db::CompoundRegionCountFilterNode count1 (corners1, false, 5, 10000);

  db::CompoundRegionToEdgeProcessingOperationNode *corners2 = new db::CompoundRegionToEdgeProcessingOperationNode (new db::CornersAsDots (-180.0, true, 180.0, true, false, false), primary, true /*processor is owned*/);
  db::CompoundRegionCountFilterNode count2 (corners2, true, 5, 10000);

  EXPECT_EQ (count1.result_type () == db::CompoundRegionJoinOperationNode::Region, true);
  EXPECT_EQ (count2.result_type () == db::CompoundRegionJoinOperationNode::Edges, true);

  db::Region res1 = r.cop_to_region (count1);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res1.insert_into (&ly, *ly.begin_top_down (), l1000);

  db::Edges res2 = r.cop_to_edges (count2);

  unsigned int l1001 = ly.get_layer (db::LayerProperties (1001, 0));
  res2.insert_into (&ly, *ly.begin_top_down (), l1001);

  CHECKPOINT();
  db::compare_layouts (_this, ly, make_au ("15", deep));
}

TEST(15_JoinAndMerged)
{
  run_test15 (_this, false);
}

TEST(15d_JoinAndMerged)
{
  run_test15 (_this, true);
}

void run_test16 (tl::TestBase *_this, bool deep)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/drc/compound_16.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  db::DeepShapeStore dss;

  db::Region r, r2;
  prep_layer (ly, 1, r, dss, deep);
  prep_layer (ly, 2, r2, dss, deep);

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();
  db::CompoundRegionOperationForeignNode *foreign = new db::CompoundRegionOperationForeignNode ();

  db::CompoundRegionProcessingOperationNode *sized = new db::CompoundRegionProcessingOperationNode (new db::PolygonSizer (600, 600, 2), foreign, true /*processor is owned*/, 600 /*dist adder*/);

  db::CompoundRegionGeometricalBoolOperationNode geo_bool (db::CompoundRegionGeometricalBoolOperationNode::And, primary, sized);

  db::Region res1 = r.cop_to_region (geo_bool);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res1.insert_into (&ly, *ly.begin_top_down (), l1000);

  CHECKPOINT();
  db::compare_layouts (_this, ly, make_au ("16", deep));
}

TEST(16_JoinAndMerged)
{
  run_test16 (_this, false);
}

TEST(16d_JoinAndMerged)
{
  run_test16 (_this, true);
}
