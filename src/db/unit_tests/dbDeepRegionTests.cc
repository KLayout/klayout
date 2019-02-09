
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

TEST(7_Merge)
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

  unsigned int l6 = ly.get_layer (db::LayerProperties (6, 0));

  db::Region r6 (db::RecursiveShapeIterator (ly, top_cell, l6), dss);

  db::Region r6_merged = r6.merged ();
  db::Region r6_merged_minwc = r6.merged (false, 1);

  db::Region r6_minwc = r6;
  r6_minwc.merge (false, 1);

  r6.merge ();

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r6);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r6_minwc);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r6_merged);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), r6_merged_minwc);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au7.gds");
}

TEST(8_AreaAndPerimeter)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/deep_region_area_peri_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;
  dss.set_max_vertex_count (4);
  dss.set_threads (0);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1), dss);
  EXPECT_EQ (r1.area (), db::coord_traits<db::Coord>::area_type (9722000000));
  EXPECT_EQ (r1.perimeter (), db::coord_traits<db::Coord>::perimeter_type (1360000));

  EXPECT_EQ (r1.area (r1.bbox ()), db::coord_traits<db::Coord>::area_type (9722000000));
  EXPECT_EQ (r1.perimeter (r1.bbox ()), db::coord_traits<db::Coord>::perimeter_type (1360000));

  EXPECT_EQ (r1.area (db::Box (40000, -90000, 50000, -80000)), db::coord_traits<db::Coord>::area_type (100000000));
  EXPECT_EQ (r1.perimeter (db::Box (40000, -90000, 50000, -80000)), db::coord_traits<db::Coord>::perimeter_type (0));
  EXPECT_EQ (r1.area (db::Box (-40000, -90000, -50000, -80000)), db::coord_traits<db::Coord>::area_type (0));
}

TEST(9_SizingSimple)
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

  unsigned int l6 = ly.get_layer (db::LayerProperties (6, 0));

  db::Region r6 (db::RecursiveShapeIterator (ly, top_cell, l6), dss);
  db::Region r6_sized = r6.sized (-50);
  db::Region r6_sized_aniso = r6.sized (-20, -100);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r6);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r6_sized);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r6_sized_aniso);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au9a.gds");
}

TEST(9_SizingWithScaleVariants)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/deep_region_area_peri_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;
  dss.set_max_vertex_count (4);
  dss.set_threads (0);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1), dss);
  db::Region r1_sized = r1.sized (-2000);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (1, 0)), r1);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r1_sized);

  //  copy another layer - this challenges the ability to map to multiple variants

  unsigned int l1b = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r1b (db::RecursiveShapeIterator (ly, top_cell, l1b), dss);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r1b.merged ());

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au9b.gds");
}

TEST(9_SizingWithScaleAndXYVariants)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/deep_region_area_peri_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;
  dss.set_max_vertex_count (4);
  dss.set_threads (0);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1), dss);
  db::Region r1_sized = r1.sized (-2000);
  db::Region r1_sized_aniso = r1.sized (-1000, -2000);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (1, 0)), r1);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r1_sized);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r1_sized_aniso);

  //  copy another layer - this challenges the ability to map to multiple variants

  unsigned int l1b = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r1b (db::RecursiveShapeIterator (ly, top_cell, l1b), dss);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r1b.merged ());

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au9c.gds");

  //  merge back to original - this challenges the ability to map back the variants

  ly.insert (top_cell_index, ly.get_layer (db::LayerProperties (11, 0)), r1_sized);
  ly.insert (top_cell_index, ly.get_layer (db::LayerProperties (12, 0)), r1_sized_aniso);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/algo/deep_region_au9d.gds");
}

TEST(9_SizingWithBoolean)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/deep_region_area_peri_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;
  dss.set_max_vertex_count (4);
  dss.set_threads (0);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1), dss);
  db::Region r1_sized = r1.sized (2000);
  r1_sized -= r1;
  db::Region r1_sized_aniso = r1.sized (1000, 2000);
  r1_sized_aniso -= r1;

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (1, 0)), r1);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r1_sized);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r1_sized_aniso);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au9e.gds");
}

TEST(10_HullsAndHoles)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/deep_region_area_peri_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;
  dss.set_max_vertex_count (4);
  dss.set_threads (0);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1), dss);
  db::Region r1_sized = r1.sized (2000);
  r1_sized -= r1;

  db::Region hulls = r1_sized.hulls ();
  db::Region holes = r1_sized.holes ();

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r1_sized);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), hulls);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), holes);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au10.gds");
}

TEST(11_RoundAndSmoothed)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/deep_region_area_peri_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;
  dss.set_max_vertex_count (4);
  dss.set_threads (0);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1), dss);
  db::Region r1_sized = r1.sized (2000);
  r1_sized -= r1;

  db::Region rounded = r1_sized.rounded_corners (3000, 5000, 100);
  db::Region smoothed = rounded.smoothed (100);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r1_sized);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), rounded);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), smoothed);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au11.gds");
}

TEST(100_Integration)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/vexriscv_clocked_r.oas.gz";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;
  dss.set_max_vertex_count (4);
  dss.set_threads (0);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));
  unsigned int l4 = ly.get_layer (db::LayerProperties (4, 0));
  unsigned int l5 = ly.get_layer (db::LayerProperties (5, 0));
  unsigned int l6 = ly.get_layer (db::LayerProperties (6, 0));
  unsigned int l7 = ly.get_layer (db::LayerProperties (7, 0));
  unsigned int l10 = ly.get_layer (db::LayerProperties (10, 0));
  unsigned int l11 = ly.get_layer (db::LayerProperties (11, 0));
  unsigned int l14 = ly.get_layer (db::LayerProperties (14, 0));
  unsigned int l16 = ly.get_layer (db::LayerProperties (16, 0));
  unsigned int l18 = ly.get_layer (db::LayerProperties (18, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1), dss);
  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3), dss);
  db::Region r4 (db::RecursiveShapeIterator (ly, top_cell, l4), dss);
  db::Region r5 (db::RecursiveShapeIterator (ly, top_cell, l5), dss);
  db::Region r6 (db::RecursiveShapeIterator (ly, top_cell, l6), dss);
  db::Region r7 (db::RecursiveShapeIterator (ly, top_cell, l7), dss);
  db::Region r10 (db::RecursiveShapeIterator (ly, top_cell, l10), dss);
  db::Region r11 (db::RecursiveShapeIterator (ly, top_cell, l11), dss);
  db::Region r14 (db::RecursiveShapeIterator (ly, top_cell, l14), dss);
  db::Region r16 (db::RecursiveShapeIterator (ly, top_cell, l16), dss);
  db::Region r18 (db::RecursiveShapeIterator (ly, top_cell, l18), dss);

  db::Region psd = r4 - r7;
  db::Region nsd = r3 - r7;
  db::Region pgate = r4 & r7;
  db::Region ngate = r3 & r7;
  db::Region poly_cont = r10 & r7;
  db::Region diff_cont = r10 - r7;

  r1.merge ();
  r3.merge ();
  r4.merge ();
  r5.merge ();
  r6.merge ();
  r7.merge ();
  r10.merge ();
  r11.merge ();
  r14.merge ();
  r16.merge ();
  r18.merge ();

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (1, 0)), r1);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (3, 0)), r3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (4, 0)), r4);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (5, 0)), r5);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (6, 0)), r6);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (7, 0)), r7);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r10);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r11);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (14, 0)), r14);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (16, 0)), r16);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (18, 0)), r18);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (100, 0)), psd);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (101, 0)), nsd);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (102, 0)), pgate);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (103, 0)), ngate);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (104, 0)), poly_cont);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (105, 0)), diff_cont);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au100.gds");
}
