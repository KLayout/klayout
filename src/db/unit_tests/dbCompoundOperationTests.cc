
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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
#include "dbCompoundOperation.h"
#include "dbReader.h"
#include "dbRecursiveShapeIterator.h"
#include "dbRegionUtils.h"
#include "dbTestSupport.h"

#include "tlStream.h"

#include <cstdio>

TEST(1_Basic)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/drc/compound_1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  db::CompoundRegionCheckOperationNode width_check (db::WidthRelation, false /*==same polygon*/, 1050, check_options);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l1));

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  db::Region r2 (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l2));

  db::EdgePairs res = r.cop_to_edge_pairs (width_check);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();
  db::CompoundRegionCheckOperationNode space_check (primary, db::SpaceRelation, true /*==different polygons*/, 1050, check_options);

  res = r.cop_to_edge_pairs (space_check);

  unsigned int l1001 = ly.get_layer (db::LayerProperties (1001, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1001);

  db::CompoundRegionOperationSecondaryNode *secondary = new db::CompoundRegionOperationSecondaryNode (&r2);
  db::CompoundRegionCheckOperationNode sep_check (secondary, db::SpaceRelation, true /*==different polygons*/, 1050, check_options);

  res = r.cop_to_edge_pairs (sep_check);

  unsigned int l1002 = ly.get_layer (db::LayerProperties (1002, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1002);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/drc/compound_au1.gds");
}

TEST(2_ChainedOperations)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/drc/compound_2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l1));

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  db::Region r2 (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l2));

  db::CompoundRegionCheckOperationNode *width_check = new db::CompoundRegionCheckOperationNode (db::WidthRelation, false /*==same polygon*/, 1050, check_options);

  db::CompoundRegionEdgePairToPolygonProcessingOperationNode ep2p (new db::EdgePairToPolygonProcessor (0), width_check, true);
  db::Region res = r.cop_to_region (ep2p);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  db::CompoundRegionEdgePairToEdgeProcessingOperationNode ep2e1 (new db::EdgePairToFirstEdgesProcessor (), width_check, true);
  db::Edges eres = r.cop_to_edges (ep2e1);

  unsigned int l1001 = ly.get_layer (db::LayerProperties (1001, 0));
  eres.insert_into (&ly, *ly.begin_top_down (), l1001);

  db::CompoundRegionEdgePairToEdgeProcessingOperationNode ep2e2 (new db::EdgePairToSecondEdgesProcessor (), width_check, true);
  eres = r.cop_to_edges (ep2e2);

  unsigned int l1002 = ly.get_layer (db::LayerProperties (1002, 0));
  eres.insert_into (&ly, *ly.begin_top_down (), l1002);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/drc/compound_au2.gds");
}

TEST(3_BooleanOperations)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/drc/compound_3.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l1));

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  db::Region r2 (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l2));

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
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/drc/compound_au3.gds");
}

TEST(4_SizeOperation)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/drc/compound_4.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l1));

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  db::Region r2 (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l2));

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
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/drc/compound_au4.gds");
}

TEST(5_InteractOperation)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/drc/compound_5.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l1));

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  db::Region r2 (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l2));

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();
  db::CompoundRegionOperationSecondaryNode *secondary = new db::CompoundRegionOperationSecondaryNode (&r2);
  db::CompoundRegionInteractOperationNode interact (primary, secondary, 0, true, false);

  db::Region res = r.cop_to_region (interact);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/drc/compound_au5.gds");
}

TEST(6_InteractWithEdgeOperation)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/drc/compound_6.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l1));

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  db::Region r2 (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l2));

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();
  db::CompoundRegionOperationSecondaryNode *secondary = new db::CompoundRegionOperationSecondaryNode (&r2);
  db::CompoundRegionToEdgeProcessingOperationNode *secondary_edges = new db::CompoundRegionToEdgeProcessingOperationNode (new db::PolygonToEdgeProcessor (), secondary, true);

  db::CompoundRegionInteractWithEdgeOperationNode interact (primary, secondary_edges, false);

  db::Region res = r.cop_to_region (interact);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/drc/compound_au6.gds");
}

TEST(7_PullOperation)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/drc/compound_7.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l1));

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  db::Region r2 (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l2));

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();
  db::CompoundRegionOperationSecondaryNode *secondary = new db::CompoundRegionOperationSecondaryNode (&r2);

  db::CompoundRegionPullOperationNode pull (primary, secondary, 0, true);

  db::Region res = r.cop_to_region (pull);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/drc/compound_au7.gds");
}

TEST(8_PullWithEdgeOperation)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/drc/compound_8.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l1));

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  db::Region r2 (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l2));

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();
  db::CompoundRegionOperationSecondaryNode *secondary = new db::CompoundRegionOperationSecondaryNode (&r2);
  db::CompoundRegionToEdgeProcessingOperationNode *secondary_edges = new db::CompoundRegionToEdgeProcessingOperationNode (new db::PolygonToEdgeProcessor (), secondary, true);

  db::CompoundRegionPullWithEdgeOperationNode pull (primary, secondary_edges);

  db::Edges res = r.cop_to_edges (pull);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/drc/compound_au8.gds");
}

TEST(9_LogicalSelectOperation)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/drc/compound_9.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l1));

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  db::Region r2 (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l2));

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

  db::CompoundRegionFilterOperationNode *condition2 = new db::CompoundRegionFilterOperationNode (new db::RectangleFilter (false), primary, true);
  inputs.push_back (condition2);

  db::CompoundRegionSizeOperationNode *result2 = new db::CompoundRegionSizeOperationNode (-50, -50, 2, primary);
  inputs.push_back (result2);

  db::CompoundRegionProcessingOperationNode *result_default = new db::CompoundRegionProcessingOperationNode (new db::Extents (), primary, true);
  inputs.push_back (result_default);

  db::CompoundRegionLogicalCaseSelectOperationNode select_node (false, inputs);

  db::Region res = r.cop_to_region (select_node);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/drc/compound_au9.gds");
}

TEST(10_LogicalSelectOperation)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/drc/compound_10.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l1));

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  db::Region r2 (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l2));

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();

  db::CompoundRegionFilterOperationNode *condition1 = new db::CompoundRegionFilterOperationNode (new db::RegionAreaFilter (0, 10000000, true), primary, true);
  db::CompoundRegionFilterOperationNode *condition1r = new db::CompoundRegionFilterOperationNode (new db::RegionAreaFilter (0, 10000000, false), primary, true);

  db::CompoundRegionFilterOperationNode *condition2 = new db::CompoundRegionFilterOperationNode (new db::RectangleFilter (false), primary, true);

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
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/drc/compound_au10.gds");
}

