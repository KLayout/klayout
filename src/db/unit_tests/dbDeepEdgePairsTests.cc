
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


#include "dbHierarchyBuilder.h"
#include "dbReader.h"
#include "dbTestSupport.h"
#include "dbEdgePairs.h"
#include "dbDeepShapeStore.h"
#include "dbRegion.h"
#include "dbEdges.h"
#include "tlUnitTest.h"
#include "tlStream.h"

TEST(1_Basics)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  //  turn boxes into edge pairs to produce a test case
  for (db::Layout::layer_iterator l = ly.begin_layers (); l != ly.end_layers (); ++l) {
    for (db::Layout::iterator c = ly.begin (); c != ly.end (); ++c) {
      db::Shapes out (ly.is_editable ());
      db::Shapes &in = c->shapes ((*l).first);
      for (db::Shapes::shape_iterator s = in.begin (db::ShapeIterator::All); !s.at_end (); ++s) {
        if (s->is_box ()) {
          db::Box b = s->box ();
          db::EdgePair ep (db::Edge (b.p1 (), b.upper_left ()), db::Edge (b.p2 (), b.lower_right ()));
          out.insert (ep);
        }
      }
      in.swap (out);
    }
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));
  unsigned int l100 = ly.get_layer (db::LayerProperties (100, 0));

  db::EdgePairs ep2 (db::RecursiveShapeIterator (ly, top_cell, l2), dss);
  db::EdgePairs ep3 (db::RecursiveShapeIterator (ly, top_cell, l3), dss);
  db::EdgePairs ep100 (db::RecursiveShapeIterator (ly, top_cell, l100), dss);

  EXPECT_EQ (ep100.empty (), true);
  EXPECT_EQ (ep2.empty (), false);
  EXPECT_EQ (ep2.bbox ().to_string (), "(-1050,-475;24810,3275)");
  EXPECT_EQ (ep2.count (), size_t (40));
  EXPECT_EQ (ep2.hier_count (), size_t (1));
  EXPECT_EQ (ep2.to_string ().substr (0, 42), "(-1050,-475;-1050,475)/(250,475;250,-475);");

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  db::Region polygons;
  ep2.polygons (polygons);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), polygons);

  polygons.clear ();
  ep3.polygons (polygons);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), polygons);

  db::Edges edges, first_edges, second_edges;
  ep2.edges (edges);
  ep2.first_edges (first_edges);
  ep2.second_edges (second_edges);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), edges);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), first_edges);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), second_edges);

  //  NOTE: insert ep2 as layer 14/0 from a copy - this tests the ability to copy-construct an EP
  db::EdgePairs ep2_copy (ep2);
  ep2_copy.insert_into_as_polygons (&target, target_top_cell_index, target.get_layer (db::LayerProperties (14, 0)), 1);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/deep_edge_pairs_au1.gds");
}
