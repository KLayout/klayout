
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

#include "dbRegion.h"
#include "dbEdgeNeighborhood.h"
#include "dbReader.h"
#include "dbTestSupport.h"

#include "tlStream.h"

#include <cstdio>

namespace
{

class ENPrimaryCopyVisitor
  : public db::EdgeNeighborhoodVisitor
{
public:
  ENPrimaryCopyVisitor ()
  {
    set_result_type (db::CompoundRegionOperationNode::ResultType::Region);
  }

  void begin_polygon (const db::Layout *, const db::Cell *, const db::Polygon &polygon)
  {
    output_polygon (polygon);
  }
};

class ENPrimaryCopyIntruderVisitor
  : public db::EdgeNeighborhoodVisitor
{
public:
  ENPrimaryCopyIntruderVisitor (unsigned int input)
  {
    set_result_type (db::CompoundRegionOperationNode::ResultType::Region);
    m_input = input;
  }

  void on_edge (const db::Layout * /*layout*/, const db::Cell * /*cell*/, const db::Edge &edge, const neighbors_type &neighbors)
  {
    db::IMatrix3d trans = to_original_trans (edge);

    for (auto n = neighbors.begin (); n != neighbors.end (); ++n) {
      for (auto nn = n->second.begin (); nn != n->second.end (); ++nn) {
        if (nn->first == m_input) {
          for (auto p = nn->second.begin (); p != nn->second.end (); ++p) {
            output_polygon (trans * *p);
          }
        }
      }
    }
  }

private:
  unsigned int m_input;
};

}

static void prep_layer (db::Layout &ly, int gds_layer, db::Region &r, db::DeepShapeStore &dss, bool deep)
{
  unsigned int li = ly.get_layer (db::LayerProperties (gds_layer, 0));
  if (deep) {
    r = db::Region (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), li), dss);
  } else {
    r = db::Region (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), li));
  }
}

static void run_test (tl::TestBase *_this, db::EdgeNeighborhoodVisitor &visitor, const std::string &au_name, bool deep = true, db::Coord bext = 0, db::Coord eext = 0, db::Coord din = 0, db::Coord dout = 0)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/edge_neighborhood.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::DeepShapeStore dss;

  db::Region r1, r2, r3;
  prep_layer (ly, 1, r1, dss, deep);
  prep_layer (ly, 2, r2, dss, deep);
  prep_layer (ly, 3, r3, dss, deep);

  std::vector<db::CompoundRegionOperationNode *> children;
  children.push_back (new db::CompoundRegionOperationPrimaryNode ());
  children.push_back (new db::CompoundRegionOperationForeignNode ());
  children.push_back (new db::CompoundRegionOperationSecondaryNode (&r2));
  children.push_back (new db::CompoundRegionOperationSecondaryNode (&r3));

  db::EdgeNeighborhoodCompoundOperationNode en_node (children, &visitor, bext, eext, din, dout);

  unsigned int l100 = ly.get_layer (db::LayerProperties (100, 0));

  if (en_node.result_type () == db::CompoundRegionOperationNode::ResultType::Region) {
    auto res = r1.cop_to_region (en_node);
    res.insert_into (&ly, *ly.begin_top_down (), l100);
  } else if (en_node.result_type () == db::CompoundRegionOperationNode::ResultType::Edges) {
    auto res = r1.cop_to_edges (en_node);
    res.insert_into (&ly, *ly.begin_top_down (), l100);
  } else if (en_node.result_type () == db::CompoundRegionOperationNode::ResultType::EdgePairs) {
    auto res = r1.cop_to_edge_pairs (en_node);
    res.insert_into (&ly, *ly.begin_top_down (), l100);
  }

  db::compare_layouts (_this, ly, tl::testdata () + au_name);
}

TEST(1)
{
  ENPrimaryCopyVisitor visitor;
  run_test (_this, visitor, "/algo/edge_neighborhood_au1.gds");
}


TEST(2)
{
  ENPrimaryCopyIntruderVisitor visitor (0);
  run_test (_this, visitor, "/algo/edge_neighborhood_au2.gds", true, 100, 100, 100, 2000);
}

TEST(3)
{
  ENPrimaryCopyIntruderVisitor visitor (1);
  run_test (_this, visitor, "/algo/edge_neighborhood_au3.gds", true, 100, 100, 100, 2000);
}

TEST(4)
{
  ENPrimaryCopyIntruderVisitor visitor (2);
  run_test (_this, visitor, "/algo/edge_neighborhood_au4.gds", true, 100, 100, 100, 2000);
}

TEST(5)
{
  ENPrimaryCopyIntruderVisitor visitor (3);
  run_test (_this, visitor, "/algo/edge_neighborhood_au5.gds", true, 100, 100, 100, 2000);
}
