
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


#include "dbHierarchyBuilder.h"
#include "dbReader.h"
#include "dbTestSupport.h"
#include "dbRegion.h"
#include "dbEdges.h"
#include "dbRegionUtils.h"
#include "dbRegionProcessors.h"
#include "dbEdgesUtils.h"
#include "dbDeepShapeStore.h"
#include "dbOriginalLayerRegion.h"
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

    EXPECT_EQ (regions.back ().size (), db::Region (iter).size ());
    EXPECT_EQ (regions.back ().bbox (), db::Region (iter).bbox ());
    EXPECT_EQ (regions.back ().is_merged (), false);

  }

  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  for (std::vector<db::Region>::const_iterator r = regions.begin (); r != regions.end (); ++r) {
    target.insert (target_top_cell_index, target_layers [r - regions.begin ()], *r);
  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au1.gds");

  //  some operations
  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));
  db::Region r2 (db::RecursiveShapeIterator (ly, ly.cell (top_cell_index), l2), dss);
  db::Region r3 (db::RecursiveShapeIterator (ly, ly.cell (top_cell_index), l3), dss);

  EXPECT_EQ (r2.is_merged (), false);
  r2.merge ();
  EXPECT_EQ (r2.is_merged (), true);
  r2 += r3;
  EXPECT_EQ (r2.is_merged (), false);
  EXPECT_EQ (r2.merged ().is_merged (), true);
  EXPECT_EQ (r2.is_merged (), false);
  r2.merge ();
  EXPECT_EQ (r2.is_merged (), true);
  r2.flatten ();
  EXPECT_EQ (r2.is_merged (), true);
  r2.insert (db::Box (0, 0, 1000, 2000));
  EXPECT_EQ (r2.is_merged (), false);
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

  EXPECT_EQ (r2and3.is_merged (), false);

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

  EXPECT_EQ (r2xor3.is_merged (), false);

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
  EXPECT_EQ (r6_sized.is_merged (), true);
  db::Region r6_sized_aniso = r6.sized (-20, -100, 2);
  EXPECT_EQ (r6_sized_aniso.is_merged (), true);
  db::Region r6_sized_plus = r6.sized (50);
  EXPECT_EQ (r6_sized_plus.is_merged (), false);
  db::Region r6_sized_aniso_plus = r6.sized (20, 100, 2);
  EXPECT_EQ (r6_sized_aniso_plus.is_merged (), false);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r6);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r6_sized);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r6_sized_aniso);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), r6_sized_plus);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (14, 0)), r6_sized_aniso_plus);

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
  db::Region r1_sized_aniso = r1.sized (-1000, -2000, 2);

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
  db::Region r1_sized_aniso = r1.sized (1000, 2000, 2);
  r1_sized_aniso -= r1;
  EXPECT_EQ (r1_sized_aniso.is_merged (), false);

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
  EXPECT_EQ (hulls.is_merged (), true);
  EXPECT_EQ (holes.is_merged (), true);

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

TEST(12_GridSnap)
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

  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));

  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3), dss);
  db::Region r3snapped = r3.snapped (50, 50);
  EXPECT_EQ (r3snapped.is_merged (), false);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r3snapped);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au12.gds");
}

TEST(13_Edges)
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

  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));

  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3), dss);
  db::Edges r3edges = r3.edges ();
  EXPECT_EQ (r3edges.is_merged (), true);

  db::EdgeLengthFilter f (0, 500, true);
  db::Edges r3edges_filtered = r3.edges (f);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r3edges);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r3edges_filtered);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au13.gds");
}

TEST(14_Interacting)
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

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l6 = ly.get_layer (db::LayerProperties (6, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1), dss);
  db::Region r2 (db::RecursiveShapeIterator (ly, top_cell, l2), dss);
  db::Region r6 (db::RecursiveShapeIterator (ly, top_cell, l6), dss);
  db::Region r1f (db::RecursiveShapeIterator (ly, top_cell, l1));
  db::Region r2f (db::RecursiveShapeIterator (ly, top_cell, l2));
  db::Region r6f (db::RecursiveShapeIterator (ly, top_cell, l6));
  db::Region r1r = r1;
  r1r.set_merged_semantics (false);
  db::Region r2r = r2;
  r2r.set_merged_semantics (false);
  db::Region r6r = r6;
  r6r.set_merged_semantics (false);

  {
    db::Layout target;
    unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r2.selected_interacting (r1));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r2.selected_not_interacting (r1));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r2.selected_inside (r1));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), r2.selected_not_inside (r1));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (14, 0)), r2.selected_outside (r1));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (15, 0)), r2.selected_not_outside (r1));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (16, 0)), r2.selected_overlapping (r1));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (17, 0)), r2.selected_not_overlapping (r1));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), r6.selected_interacting (r1));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (21, 0)), r6.selected_not_interacting (r1));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (22, 0)), r6.selected_inside (r1));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (23, 0)), r6.selected_not_inside (r1));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (24, 0)), r6.selected_outside (r1));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (25, 0)), r6.selected_not_outside (r1));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (26, 0)), r6.selected_overlapping (r1));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (27, 0)), r6.selected_not_overlapping (r1));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (30, 0)), r2.selected_interacting (r1f));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (31, 0)), r2.selected_not_interacting (r1f));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (32, 0)), r2.selected_inside (r1f));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (33, 0)), r2.selected_not_inside (r1f));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (34, 0)), r2.selected_outside (r1f));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (35, 0)), r2.selected_not_outside (r1f));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (36, 0)), r2.selected_overlapping (r1f));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (37, 0)), r2.selected_not_overlapping (r1f));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (40, 0)), r6.selected_interacting (r1f));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (41, 0)), r6.selected_not_interacting (r1f));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (42, 0)), r6.selected_inside (r1f));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (43, 0)), r6.selected_not_inside (r1f));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (44, 0)), r6.selected_outside (r1f));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (45, 0)), r6.selected_not_outside (r1f));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (46, 0)), r6.selected_overlapping (r1f));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (47, 0)), r6.selected_not_overlapping (r1f));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (50, 0)), r2r.selected_interacting (r1r));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (51, 0)), r2r.selected_not_interacting (r1r));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (52, 0)), r2r.selected_inside (r1r));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (53, 0)), r2r.selected_not_inside (r1r));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (54, 0)), r2r.selected_outside (r1r));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (55, 0)), r2r.selected_not_outside (r1r));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (56, 0)), r2r.selected_overlapping (r1r));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (57, 0)), r2r.selected_not_overlapping (r1r));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (60, 0)), r6r.selected_interacting (r1r));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (61, 0)), r6r.selected_not_interacting (r1r));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (62, 0)), r6r.selected_inside (r1r));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (63, 0)), r6r.selected_not_inside (r1r));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (64, 0)), r6r.selected_outside (r1r));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (65, 0)), r6r.selected_not_outside (r1r));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (66, 0)), r6r.selected_overlapping (r1r));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (67, 0)), r6r.selected_not_overlapping (r1r));

    EXPECT_EQ (r2.selected_interacting (r1).is_merged (), false);
    EXPECT_EQ (r2.selected_interacting (r1.merged ()).is_merged (), true);
    EXPECT_EQ (r2.selected_inside (r1).is_merged (), true);
    EXPECT_EQ (r2r.selected_interacting (r1).is_merged (), false);
    EXPECT_EQ (r2.selected_interacting (r1r).is_merged (), false);
    EXPECT_EQ (r2r.selected_interacting (r1r).is_merged (), false);

    CHECKPOINT();
    db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au14a.gds");
  }

  db::Edges r1e = r1.edges ();
  db::Edges r1ef = r1f.edges ();
  db::Edges r1er = r1r.edges ();
  r1er.set_merged_semantics (false);

  {
    db::Layout target;
    unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r6);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r1e);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r6.selected_interacting (r1e));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), r6.selected_not_interacting (r1e));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (22, 0)), r6.selected_interacting (r1ef));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (23, 0)), r6.selected_not_interacting (r1ef));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (32, 0)), r6r.selected_interacting (r1er));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (33, 0)), r6r.selected_not_interacting (r1er));

    EXPECT_EQ (r6.selected_interacting (r1e).is_merged (), true);
    EXPECT_EQ (r6.selected_interacting (r1er).is_merged (), false);
    EXPECT_EQ (r6r.selected_interacting (r1e).is_merged (), false);
    EXPECT_EQ (r6r.selected_interacting (r1er).is_merged (), false);

    CHECKPOINT();
    db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au14b.gds");
  }
}

TEST(15_Filtered)
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

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1), dss);
  db::RegionAreaFilter af1 (0, 1000000000, false);
  db::Region af1_filtered = r1.filtered (af1);
  db::RegionAreaFilter af1inv (0, 1000000000, true);
  db::Region af1_else = r1.filtered (af1inv);
  EXPECT_EQ (af1_filtered.is_merged (), true);
  EXPECT_EQ (af1_else.is_merged (), true);

  {
    db::Layout target;
    unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r1);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), af1_filtered);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), af1_else);

    CHECKPOINT();
    db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au15a.gds");
  }

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));

  db::Region r2 (db::RecursiveShapeIterator (ly, top_cell, l2), dss);
  db::RegionBBoxFilter bwf (0, 50000, false, db::RegionBBoxFilter::BoxWidth);
  db::RegionBBoxFilter bhf (0, 50000, false, db::RegionBBoxFilter::BoxHeight);
  db::Region r2_bwf = r2.filtered (bwf);
  db::Region r2_bhf = r2.filtered (bhf);

  {
    db::Layout target;
    unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r2);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r2_bwf);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r2_bhf);

    CHECKPOINT();
    db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au15b.gds");
  }
}

TEST(16_MergeWithMinWC)
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

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1), dss);
  db::Region r1_merged_wc0 = r1.merged (true, 0);
  db::Region r1_merged_wc1 = r1.merged (true, 1);
  db::Region r1_merged_wc2 = r1.merged (true, 2);
  EXPECT_EQ (r1_merged_wc0.is_merged (), true);
  EXPECT_EQ (r1_merged_wc1.is_merged (), true);
  EXPECT_EQ (r1_merged_wc2.is_merged (), true);


  {
    db::Layout target;
    unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r1_merged_wc0);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r1_merged_wc1);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r1_merged_wc2);

    CHECKPOINT();
    db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au16.gds");
  }
}

TEST(17_SinglePolygonChecks)
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

  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));
  unsigned int l6 = ly.get_layer (db::LayerProperties (6, 0));
  unsigned int l4 = ly.get_layer (db::LayerProperties (4, 0));

  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3), dss);
  db::Region r6 (db::RecursiveShapeIterator (ly, top_cell, l6), dss);
  db::Region r4 (db::RecursiveShapeIterator (ly, top_cell, l4), dss);

  {
    db::Layout target;
    unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (3, 0)), r3);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (4, 0)), r4);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (6, 0)), r6);

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r3.width_check (260, false, db::Euclidian, 90, 0));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r3.width_check (260, true, db::Projection, 90, 2000));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), r6.notch_check (1300, false, db::Euclidian, 90, 0));

    CHECKPOINT();
    db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au17.gds");
  }
}

TEST(18_MultiPolygonChecks)
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

  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));
  unsigned int l6 = ly.get_layer (db::LayerProperties (6, 0));
  unsigned int l4 = ly.get_layer (db::LayerProperties (4, 0));

  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3), dss);
  db::Region r6 (db::RecursiveShapeIterator (ly, top_cell, l6), dss);
  db::Region r4 (db::RecursiveShapeIterator (ly, top_cell, l4), dss);

  {
    db::Layout target;
    unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (3, 0)), r3);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (4, 0)), r4);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (6, 0)), r6);

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r3.space_check (500, false, db::Projection, 90, 0));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r3.space_check (500, true, db::Projection, 90, 300));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), r3.separation_check (r4, 200, false, db::Projection, 90, 0));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (30, 0)), r6.enclosing_check (r4, 100, true, db::Projection, 90, 0));

    CHECKPOINT();
    db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au18.gds");
  }
}

TEST(19_GridCheck)
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

  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));

  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3), dss);
  db::Region r3_gc1;
  r3.grid_check (25, 25).polygons (r3_gc1, 100);
  db::Region r3_gc2;
  r3.grid_check (40, 40).polygons (r3_gc2, 100);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r3_gc1);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r3_gc2);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au19.gds");
}

TEST(20_AngleCheck)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/angle_check_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1), dss);
  db::EdgePairs ep1_ac1 = r1.angle_check (0, 91, true);
  db::EdgePairs ep1_ac2 = r1.angle_check (0, 45, false);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (1, 0)), r1);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (2, 0)), ep1_ac1);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (3, 0)), ep1_ac2);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au20.gds");
}

TEST(21_Processors)
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

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1), dss);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (1, 0)), r1);

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r1.processed (db::CornersAsDots (-180.0, 180.0)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r1.processed (db::CornersAsDots (0.0, 180.0)));
  db::Region ext;
  r1.processed (db::CornersAsDots (0.0, 180.0)).extended (ext, 1000, 1000, 2000, 2000);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), ext);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), r1.processed (db::CornersAsRectangles (-180.0, 180.0, 2000)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (14, 0)), r1.processed (db::CornersAsRectangles (0.0, 180.0, 2000)));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), r1.processed (db::extents_processor<db::Polygon> (0, 0)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (21, 0)), r1.processed (db::extents_processor<db::Polygon> (1000, 2000)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (22, 0)), r1.processed (db::RelativeExtents (0, 0, 1.0, 1.0, 0, 0)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (23, 0)), r1.processed (db::RelativeExtents (0.25, 0.4, 0.75, 0.6, 1000, 2000)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (24, 0)), r1.processed (db::RelativeExtentsAsEdges (0, 0, 1.0, 1.0)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (25, 0)), r1.processed (db::RelativeExtentsAsEdges (0.5, 0.5, 0.5, 0.5)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (26, 0)), r1.processed (db::RelativeExtentsAsEdges (0.25, 0.4, 0.75, 0.6)));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (30, 0)), r1.processed (db::minkowsky_sum_computation<db::Box> (db::Box (-1000, -2000, 3000, 4000))));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (31, 0)), r1.processed (db::minkowsky_sum_computation<db::Edge> (db::Edge (-1000, 0, 3000, 0))));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (40, 0)), r1.processed (db::TrapezoidDecomposition (db::TD_htrapezoids)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (41, 0)), r1.processed (db::ConvexDecomposition (db::PO_vertical)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (42, 0)), r1.processed (db::ConvexDecomposition (db::PO_horizontal)));

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au21.gds");
}

TEST(22_TwoLayoutsWithDifferentDBU)
{
  db::Layout ly1;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/deep_region_area_peri_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly1);
  }

  db::cell_index_type top_cell_index1 = *ly1.begin_top_down ();
  db::Cell &top_cell1 = ly1.cell (top_cell_index1);

  db::Layout ly2;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/deep_region_area_peri_l1_dbu2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly2);
  }

  db::cell_index_type top_cell_index2 = *ly2.begin_top_down ();
  db::Cell &top_cell2 = ly2.cell (top_cell_index2);

  db::DeepShapeStore dss;

  unsigned int l11 = ly1.get_layer (db::LayerProperties (1, 0));
  db::Region r11 (db::RecursiveShapeIterator (ly1, top_cell1, l11), dss);

  unsigned int l12 = ly2.get_layer (db::LayerProperties (2, 0));
  db::Region r12 (db::RecursiveShapeIterator (ly2, top_cell2, l12), dss, db::ICplxTrans (ly2.dbu () / ly1.dbu ()));

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly1.cell_name (top_cell_index1));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (1, 0)), r11.sized (1000) ^ r12);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au22.gds");
}

TEST(23_Texts)
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

  unsigned int l6 = ly.get_layer (db::LayerProperties (6, 1));
  unsigned int l8 = ly.get_layer (db::LayerProperties (8, 1));

  db::Region r6boxes (db::Region (db::RecursiveShapeIterator (ly, top_cell, l6)).texts_as_boxes ("*", true, 100, dss));
  db::Region r6dots;
  db::Edges (db::Region (db::RecursiveShapeIterator (ly, top_cell, l6)).texts_as_dots ("*", true, dss)).extended (r6dots, 20, 20, 20, 20);
  db::Region r8boxes (db::Region (db::RecursiveShapeIterator (ly, top_cell, l8)).texts_as_boxes ("VDD", false, 100, dss));
  db::Region r8dots;
  db::Edges (db::Region (db::RecursiveShapeIterator (ly, top_cell, l8)).texts_as_dots ("V*", true, dss)).extended (r8dots, 20, 20, 20, 20);

  db::Region rf6boxes (db::Region (db::RecursiveShapeIterator (ly, top_cell, l6)).texts_as_boxes ("*", true, 100));
  db::Region rf6dots;
  db::Edges (db::Region (db::RecursiveShapeIterator (ly, top_cell, l6)).texts_as_dots ("*", true)).extended (rf6dots, 20, 20, 20, 20);
  db::Region rf8boxes (db::Region (db::RecursiveShapeIterator (ly, top_cell, l8)).texts_as_boxes ("VDD", false, 100));
  db::Region rf8dots;
  db::Edges (db::Region (db::RecursiveShapeIterator (ly, top_cell, l8)).texts_as_dots ("V*", true)).extended (rf8dots, 20, 20, 20, 20);

  {
    db::Layout target;
    unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r6boxes);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r6dots);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), rf6boxes);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), rf6dots);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), r8boxes);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (21, 0)), r8dots);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (22, 0)), rf8boxes);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (23, 0)), rf8dots);

    CHECKPOINT();
    db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au23.gds");
  }
}

TEST(24_TextsFromDeep)
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
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("textstring"));

  unsigned int l6 = ly.get_layer (db::LayerProperties (6, 1));
  unsigned int l8 = ly.get_layer (db::LayerProperties (8, 1));

  db::Region r6boxes (db::Region (db::RecursiveShapeIterator (ly, top_cell, l6), dss).texts_as_boxes ("*", true, 100, dss));
  db::Region r6dots;
  db::Edges (db::Region (db::RecursiveShapeIterator (ly, top_cell, l6), dss).texts_as_dots ("*", true, dss)).extended (r6dots, 20, 20, 20, 20);
  db::Region r8boxes (db::Region (db::RecursiveShapeIterator (ly, top_cell, l8), dss).texts_as_boxes ("VDD", false, 100, dss));
  db::Region r8dots;
  db::Edges (db::Region (db::RecursiveShapeIterator (ly, top_cell, l8), dss).texts_as_dots ("V*", true, dss)).extended (r8dots, 20, 20, 20, 20);

  db::Region rf6boxes (db::Region (db::RecursiveShapeIterator (ly, top_cell, l6), dss).texts_as_boxes ("*", true, 100));
  db::Region rf6dots;
  db::Edges (db::Region (db::RecursiveShapeIterator (ly, top_cell, l6), dss).texts_as_dots ("*", true)).extended (rf6dots, 20, 20, 20, 20);
  db::Region rf8boxes (db::Region (db::RecursiveShapeIterator (ly, top_cell, l8), dss).texts_as_boxes ("VDD", false, 100));
  db::Region rf8dots;
  db::Edges (db::Region (db::RecursiveShapeIterator (ly, top_cell, l8), dss).texts_as_dots ("V*", true)).extended (rf8dots, 20, 20, 20, 20);

  {
    db::Layout target;
    unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r6boxes);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r6dots);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), rf6boxes);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), rf6dots);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), r8boxes);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (21, 0)), r8dots);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (22, 0)), rf8boxes);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (23, 0)), rf8dots);

    CHECKPOINT();
    db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au24.gds");
  }
}

TEST(25_Pull)
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

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l6 = ly.get_layer (db::LayerProperties (6, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1), dss);
  db::Region r2 (db::RecursiveShapeIterator (ly, top_cell, l2), dss);
  db::Region r6 (db::RecursiveShapeIterator (ly, top_cell, l6), dss);
  db::Region r1f (db::RecursiveShapeIterator (ly, top_cell, l1));
  db::Region r2f (db::RecursiveShapeIterator (ly, top_cell, l2));
  db::Region r6f (db::RecursiveShapeIterator (ly, top_cell, l6));
  db::Region r1r = r1;
  r1r.set_merged_semantics (false);
  db::Region r2r = r2;
  r2r.set_merged_semantics (false);
  db::Region r6r = r6;
  r6r.set_merged_semantics (false);

  {
    db::Layout target;
    unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r1.pull_interacting (r2));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r1.pull_inside (r2));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r1.pull_overlapping (r2));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), r2.pull_interacting (r6));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (21, 0)), r2.pull_inside (r6));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (22, 0)), r2.pull_overlapping (r6));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (30, 0)), r1.pull_interacting (r2f));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (31, 0)), r1.pull_inside (r2f));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (32, 0)), r1.pull_overlapping (r2f));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (40, 0)), r2.pull_interacting (r6f));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (41, 0)), r2.pull_inside (r6f));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (42, 0)), r2.pull_overlapping (r6f));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (50, 0)), r1r.pull_interacting (r2r));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (51, 0)), r1r.pull_inside (r2r));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (52, 0)), r1r.pull_overlapping (r2r));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (60, 0)), r2r.pull_interacting (r6r));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (61, 0)), r2r.pull_inside (r6r));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (62, 0)), r2r.pull_overlapping (r6r));

    EXPECT_EQ (r2.pull_inside (r6).is_merged (), true);
    EXPECT_EQ (r2.pull_interacting (r6).is_merged (), false);
    EXPECT_EQ (r2r.pull_interacting (r6).is_merged (), false);
    EXPECT_EQ (r2.pull_interacting (r6r).is_merged (), false);
    EXPECT_EQ (r2r.pull_interacting (r6r).is_merged (), false);

    CHECKPOINT();
    db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au25a.gds");
  }

  db::Edges r1e = r1.edges ();
  db::Edges r1ef = r1f.edges ();
  db::Edges r1er = r1r.edges ();
  r1er.set_merged_semantics (false);

  db::Edges r6e = r6.edges ();
  db::Edges r6ef = r6f.edges ();
  db::Edges r6er = r6r.edges ();
  r6er.set_merged_semantics (false);

  {
    db::Layout target;
    unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r6);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r1e);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r6.pull_interacting (r1e));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), r6.pull_interacting (r1ef));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (14, 0)), r6r.pull_interacting (r1e));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (15, 0)), r6.pull_interacting (r1er));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (16, 0)), r6r.pull_interacting (r1er));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (22, 0)), r1.pull_interacting (r6e));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (23, 0)), r1.pull_interacting (r6ef));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (24, 0)), r1r.pull_interacting (r6e));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (25, 0)), r1.pull_interacting (r6er));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (26, 0)), r1r.pull_interacting (r6er));

    EXPECT_EQ (r6.pull_interacting (r1e).is_merged (), false);
    EXPECT_EQ (r6.merged ().pull_interacting (r1e).is_merged (), true);
    EXPECT_EQ (r6r.pull_interacting (r1er).is_merged (), false);
    EXPECT_EQ (r6r.pull_interacting (r1e).is_merged (), false);
    EXPECT_EQ (r6.pull_interacting (r1er).is_merged (), false);

    CHECKPOINT();
    db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au25b.gds");
  }
}

TEST(26_BreakoutCells)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/deep_region_l26.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1), dss);
  db::Region r2 (db::RecursiveShapeIterator (ly, top_cell, l2), dss);

  dss.add_breakout_cell (0, ly.cell_by_name ("CHILD").second);

  db::Region r12 = r1 & r2;
  db::Region r1m2 = r1 - r2;
  db::Region r21 = r2 & r1;
  db::Region r2m1 = r2 - r1;

  ly.insert (top_cell.cell_index (), ly.get_layer (db::LayerProperties (100, 0)), r12);
  ly.insert (top_cell.cell_index (), ly.get_layer (db::LayerProperties (101, 0)), r1m2);
  ly.insert (top_cell.cell_index (), ly.get_layer (db::LayerProperties (102, 0)), r21);
  ly.insert (top_cell.cell_index (), ly.get_layer (db::LayerProperties (103, 0)), r2m1);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/algo/deep_region_au26.gds");
}

TEST(27a_snap)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/scale_and_snap.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1), dss);
  r1.set_merged_semantics (false);
  db::Region r2 = r1.snapped (19, 19);

  r2.insert_into (&ly, top_cell_index, ly.get_layer (db::LayerProperties (100, 0)));

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/algo/deep_region_au27.gds");
}

TEST(27b_snap)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/scale_and_snap.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1), dss);
  r1.set_merged_semantics (false);
  r1.snap (19, 19);

  r1.insert_into (&ly, top_cell_index, ly.get_layer (db::LayerProperties (100, 0)));

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/algo/deep_region_au27.gds");
}

TEST(28a_snap)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/scale_and_snap.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1), dss);
  r1.set_merged_semantics (false);
  db::Region r2 = r1.scaled_and_snapped (19, 2, 10, 19, 2, 10);

  r2.insert_into (&ly, top_cell_index, ly.get_layer (db::LayerProperties (100, 0)));

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/algo/deep_region_au28.gds");
}

TEST(28b_snap)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/scale_and_snap.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1), dss);
  r1.set_merged_semantics (false);
  r1.scale_and_snap (19, 2, 10, 19, 2, 10);

  r1.insert_into (&ly, top_cell_index, ly.get_layer (db::LayerProperties (100, 0)));

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/algo/deep_region_au28.gds");
}

TEST(29_InteractionsWithTexts)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/deep_texts_l2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l8 = ly.get_layer (db::LayerProperties (8, 0));

  db::Texts texts2 (db::RecursiveShapeIterator (ly, top_cell, l2), dss);
  db::Region polygons8 (db::RecursiveShapeIterator (ly, top_cell, l8), dss);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  db::Region polygons;
  polygons = polygons8.selected_interacting (texts2);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), polygons);

  polygons = polygons8.selected_not_interacting (texts2);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), polygons);

  {
    db::Region polygons8_copy = polygons8;
    polygons8_copy.select_interacting (texts2);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), polygons8_copy);
  }

  {
    db::Region polygons8_copy = polygons8;
    polygons8_copy.select_not_interacting (texts2);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), polygons8_copy);
  }

  {
    db::Texts t = polygons8.pull_interacting (texts2);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), t);
  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au29.gds");
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

TEST(101_DeepFlatCollaboration)
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
  db::Region r2_flat (db::RecursiveShapeIterator (ly, top_cell, l2));
  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3), dss);
  db::Region r3_flat (db::RecursiveShapeIterator (ly, top_cell, l3));

  db::Region r2fminus3   = r2_flat - r3;
  db::Region r2minus3f   = r2 - r3_flat;

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r2fminus3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r2minus3f);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au101.gds");
}

TEST(issue_277)
{
  db::Layout ly;
  db::cell_index_type top_cell_index = ly.add_cell ("TOP");
  db::Cell &top_cell = ly.cell (top_cell_index);
  unsigned int l1 = ly.insert_layer ();

  db::Shapes &s = top_cell.shapes (l1);
  s.insert (db::Box (0, 0, 400, 400));
  s.insert (db::Box (400, 400, 800, 800));


  db::DeepShapeStore dss;

  db::Region r (db::RecursiveShapeIterator (ly, top_cell, l1), dss);

  EXPECT_EQ (r.sized (1).merged (false, 1).to_string (), "");

  r.set_min_coherence (true);
  EXPECT_EQ (r.sized (1).merged (false, 1).to_string (), "(399,399;399,401;401,401;401,399)");

  r.merge ();
  EXPECT_EQ (r.sized (1).merged (false, 1).to_string (), "(399,399;399,401;401,401;401,399)");

  r.set_min_coherence (false);  //  needs to merge again
  EXPECT_EQ (r.sized (1).merged (false, 1).to_string (), "");
}

TEST(issue_400)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/gds/t10.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;
  //  keeps a reference to the DSS
  db::Region rr (db::RecursiveShapeIterator (ly, top_cell, (*ly.begin_layers ()).first), dss);

  for (db::Layout::layer_iterator l = ly.begin_layers (); l != ly.end_layers (); ++l) {

    unsigned int li = (*l).first;
    db::Region r (db::RecursiveShapeIterator (ly, top_cell, li), dss);

    r.set_merged_semantics (false);
    r.snap (19, 19);

    ly.clear_layer (li);
    r.insert_into (&ly, top_cell_index, li);

  }

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/algo/deep_region_au400a.gds");
}

TEST(issue_400_dont_keep_regions)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/gds/t10.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;

  for (db::Layout::layer_iterator l = ly.begin_layers (); l != ly.end_layers (); ++l) {

    unsigned int li = (*l).first;
    db::Region r (db::RecursiveShapeIterator (ly, top_cell, li), dss);

    r.set_merged_semantics (false);
    r.snap (19, 19);

    ly.clear_layer (li);
    r.insert_into (&ly, top_cell_index, li);

  }

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/algo/deep_region_au400b.gds");
}

TEST(issue_400_with_region)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/gds/t10.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::DBox rbox (2.61, -1.6, 12.76, 4.7);

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;

  for (db::Layout::layer_iterator l = ly.begin_layers (); l != ly.end_layers (); ++l) {

    unsigned int li = (*l).first;
    db::Region r (db::RecursiveShapeIterator (ly, top_cell, li, rbox.transformed (db::CplxTrans (ly.dbu ()).inverted ())), dss);

    r.set_merged_semantics (false);
    r.snap (19, 19);

    ly.clear_layer (li);
    r.insert_into (&ly, top_cell_index, li);

  }

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/algo/deep_region_au400c.gds");
}
