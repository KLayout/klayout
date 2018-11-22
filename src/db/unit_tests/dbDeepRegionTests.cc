
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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
  std::vector<db::Region> regions;
  std::vector<unsigned int> target_layers;

  for (db::Layout::layer_iterator li = ly.begin_layers (); li != ly.end_layers (); ++li) {

    unsigned int li1 = (*li).first;
    db::RecursiveShapeIterator iter (ly, ly.cell (top_cell_index), li1);
    target_layers.push_back (target.insert_layer (*(*li).second));

    regions.push_back (db::Region (iter, dss));

  }

  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  for (std::vector<db::Region>::const_iterator r = regions.begin (); r != regions.end (); ++r) {
    target.insert (target_top_cell_index, target_layers [r - regions.begin ()], *r);
  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au1.gds");
}

TEST(2)
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
  std::vector<std::pair<db::Region, unsigned int> > regions;

  for (db::Layout::layer_iterator li = ly.begin_layers (); li != ly.end_layers (); ++li) {

    unsigned int li1 = (*li).first;
    unsigned int tl = target.insert_layer (*(*li).second);

    db::RecursiveShapeIterator iter1 (ly, ly.cell (top_cell_index), li1, db::Box (2000, -1000, 6000, 4000));
    regions.push_back (std::make_pair (db::Region (iter1, dss), tl));

    db::RecursiveShapeIterator iter2 (ly, ly.cell (top_cell_index), li1, db::Box (14000, 0, 20000, 3000));
    regions.push_back (std::make_pair (db::Region (iter2, dss), tl));

  }

  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  for (std::vector<std::pair<db::Region, unsigned int> >::const_iterator r = regions.begin (); r != regions.end (); ++r) {
    target.insert (target_top_cell_index, r->second, r->first);
  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au2.gds");
}

TEST(3_BoolAndNot)
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
  unsigned int l42 = ly.get_layer (db::LayerProperties (42, 0));

  db::Region r2 (db::RecursiveShapeIterator (ly, top_cell, l2), dss);
  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3), dss);
  db::Region r42 (db::RecursiveShapeIterator (ly, top_cell, l42), dss);
  db::Region box (db::Box (2000, -1000, 6000, 4000));

  db::Region r2minus3   = r2 - r3;
  db::Region r2minusbox = r2 - box;
  db::Region r2minus42  = r2 - r42;
  db::Region rboxminus3 = box - r3;
  db::Region r42minus3  = r42 - r3;
  db::Region r42minus42 = r42 - r42;

  db::Region r2and3   = r2 & r3;
  db::Region r2andbox = r2 & box;
  db::Region r2and42  = r2 & r42;
  db::Region rboxand3 = box & r3;
  db::Region r42and3  = r42 & r3;
  db::Region r42and42 = r42 & r42;

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r2minus3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r2minusbox);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r2minus42);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), rboxminus3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (14, 0)), r42minus3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (15, 0)), r42minus42);

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), r2and3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (21, 0)), r2andbox);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (22, 0)), r2and42);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (23, 0)), rboxand3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (24, 0)), r42and3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (25, 0)), r42and42);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au3.gds");
}

TEST(4_Add)
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
  unsigned int l42 = ly.get_layer (db::LayerProperties (42, 0));

  db::Region r2 (db::RecursiveShapeIterator (ly, top_cell, l2), dss);
  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3), dss);
  db::Region r42 (db::RecursiveShapeIterator (ly, top_cell, l42), dss);
  db::Region box (db::Box (2000, -1000, 6000, 4000));
  db::Region r2box (db::RecursiveShapeIterator (ly, top_cell, l2, box), dss);
  db::Region r3box (db::RecursiveShapeIterator (ly, top_cell, l3, box), dss);

  //  intra-layout

  {
    db::Layout target;
    unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r2 + r3);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r42 + r3);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r2 + r42);

    db::Region rnew2 = r2;
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), rnew2);
    rnew2 += r3;
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (21, 0)), rnew2);
    rnew2 += r42;
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (22, 0)), rnew2);

    db::Region rnew42 = r42;
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (30, 0)), rnew42);
    rnew42 += r2;
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (31, 0)), rnew42);

    CHECKPOINT();
    db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au4a.gds");
  }

  //  inter-layout

  {
    db::Layout target;
    unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r2box + r3);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r2 + r3box);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r2box + r3box);

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), box + r3);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (21, 0)), r2 + box);

    CHECKPOINT();
    db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au4b.gds");
  }
}

TEST(5_BoolXOR)
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
  unsigned int l42 = ly.get_layer (db::LayerProperties (42, 0));

  db::Region r2 (db::RecursiveShapeIterator (ly, top_cell, l2), dss);
  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3), dss);
  db::Region r42 (db::RecursiveShapeIterator (ly, top_cell, l42), dss);
  db::Region box (db::Box (2000, -1000, 6000, 4000));

  db::Region r2xor3   = r2 ^ r3;
  db::Region r2xorbox = r2 ^ box;
  db::Region r2xor42  = r2 ^ r42;
  db::Region rboxxor3 = box ^ r3;
  db::Region r42xor3  = r42 ^ r3;
  db::Region r42xor42 = r42 ^ r42;

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r2xor3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r2xorbox);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r2xor42);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), rboxxor3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (14, 0)), r42xor3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (15, 0)), r42xor42);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au5.gds");
}

TEST(6_Reduction)
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
  dss.set_max_vertex_count (4);
  dss.set_threads (0);

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));
  unsigned int l42 = ly.get_layer (db::LayerProperties (42, 0));
  unsigned int lbox = ly.insert_layer ();

  db::Region r2 (db::RecursiveShapeIterator (ly, top_cell, l2), dss);
  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3), dss);
  db::Region r42 (db::RecursiveShapeIterator (ly, top_cell, l42), dss);

  top_cell.shapes (lbox).insert (db::Box (2000, -1000, 6000, 4000));
  db::Region box (db::RecursiveShapeIterator (ly, top_cell, lbox), dss);

  db::Region r2xor3   = r2 ^ r3;
  db::Region r2xorbox = r2 ^ box;
  db::Region r2xor42  = r2 ^ r42;
  db::Region rboxxor3 = box ^ r3;
  db::Region r42xor3  = r42 ^ r3;
  db::Region r42xor42 = r42 ^ r42;

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r2xor3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r2xorbox);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r2xor42);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), rboxxor3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (14, 0)), r42xor3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (15, 0)), r42xor42);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au6.gds");
}

