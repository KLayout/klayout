
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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
#include "dbEdges.h"
#include "dbRegion.h"
#include "dbDeepShapeStore.h"
#include "tlUnitTest.h"
#include "tlStream.h"

TEST(1)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/deep_region_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();

  db::DeepShapeStore dss;
  db::Layout target;

  //  deliberately using vector to force reallocation ...
  std::vector<db::Edges> edges;
  std::vector<unsigned int> target_layers;

  for (db::Layout::layer_iterator li = ly.begin_layers (); li != ly.end_layers (); ++li) {

    unsigned int li1 = (*li).first;
    db::RecursiveShapeIterator iter (ly, ly.cell (top_cell_index), li1);
    target_layers.push_back (target.insert_layer (*(*li).second));

    edges.push_back (db::Edges (iter, dss));

    EXPECT_EQ (edges.back ().size (), db::Edges (iter).size ());
    EXPECT_EQ (edges.back ().bbox (), db::Edges (iter).bbox ());

  }

  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  for (std::vector<db::Edges>::const_iterator r = edges.begin (); r != edges.end (); ++r) {
    target.insert (target_top_cell_index, target_layers [r - edges.begin ()], *r);
  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_edges_au1.gds");
}

TEST(2_MergeEdges)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/deep_region_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));

  db::Edges r2 (db::RecursiveShapeIterator (ly, top_cell, l2), dss);
  r2.merge ();
  db::Edges r3 (db::RecursiveShapeIterator (ly, top_cell, l3), dss);
  db::Edges r3_merged (r3.merged ());

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r2);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r3_merged);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_edges_au2.gds");
}

TEST(3_Edge2EdgeBooleans)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/deep_region_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));

  db::Region r2 (db::RecursiveShapeIterator (ly, top_cell, l2), dss);
  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3), dss);
  db::Region r2and3 = r2 & r3;

  db::Edges e3 = r3.edges ();
  db::Edges e2and3 = r2and3.edges ();

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (2, 0)), r2);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (3, 0)), r3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), e3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), e2and3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), e3 & e2and3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (21, 0)), e3 - e2and3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (22, 0)), e3 ^ e2and3);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_edges_au3.gds");
}

