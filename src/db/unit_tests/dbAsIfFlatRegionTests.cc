
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
#include "dbCellGraphUtils.h"
#include "dbTestSupport.h"
#include "dbCompoundOperation.h"
#include "dbFlatRegion.h"
#include "tlUnitTest.h"
#include "tlStream.h"

TEST(1_Basic)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();

  db::Layout target;

  //  deliberately using vector to force reallocation ...
  std::vector<db::Region> regions;
  std::vector<unsigned int> target_layers;

  for (db::Layout::layer_iterator li = ly.begin_layers (); li != ly.end_layers (); ++li) {

    unsigned int li1 = (*li).first;
    db::RecursiveShapeIterator iter (ly, ly.cell (top_cell_index), li1);
    target_layers.push_back (target.insert_layer (*(*li).second));

    regions.push_back (db::Region (iter));

    size_t n = 0, nhier = 0;
    db::CellCounter cc (&ly);
    for (db::Layout::top_down_const_iterator c = ly.begin_top_down (); c != ly.end_top_down (); ++c) {
      size_t ns = 0;
      for (db::Shapes::shape_iterator is = ly.cell (*c).shapes (li1).begin (db::ShapeIterator::Regions); !is.at_end (); ++is) {
        ++ns;
      }
      n += cc.weight (*c) * ns;
      nhier += ns;
    }

    EXPECT_EQ (db::Region (iter).count (), n);
    EXPECT_EQ (regions.back ().count (), n);
    EXPECT_EQ (regions.back ().hier_count (), nhier);
    EXPECT_EQ (regions.back ().bbox (), db::Region (iter).bbox ());
    EXPECT_EQ (regions.back ().is_merged (), false);

  }

  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  for (std::vector<db::Region>::const_iterator r = regions.begin (); r != regions.end (); ++r) {
    target.insert (target_top_cell_index, target_layers [r - regions.begin ()], *r);
  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au1.gds");

  //  some operations
  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));
  db::Region r2 (db::RecursiveShapeIterator (ly, ly.cell (top_cell_index), l2));
  db::Region r3 (db::RecursiveShapeIterator (ly, ly.cell (top_cell_index), l3));

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
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();

  db::Layout target;

  //  deliberately using vector to force reallocation ...
  std::vector<std::pair<db::Region, unsigned int> > regions;

  for (db::Layout::layer_iterator li = ly.begin_layers (); li != ly.end_layers (); ++li) {

    unsigned int li1 = (*li).first;
    unsigned int tl = target.insert_layer (*(*li).second);

    db::RecursiveShapeIterator iter1 (ly, ly.cell (top_cell_index), li1, db::Box (2000, -1000, 6000, 4000));
    regions.push_back (std::make_pair (db::Region (iter1), tl));
    //  TODO: currently, original layer regions don't clip - emulate this
    regions.back ().first &= db::Region (iter1.region ());

    db::RecursiveShapeIterator iter2 (ly, ly.cell (top_cell_index), li1, db::Box (14000, 0, 20000, 3000));
    regions.push_back (std::make_pair (db::Region (iter2), tl));
    //  TODO: currently, original layer regions don't clip - emulate this
    regions.back ().first &= db::Region (iter2.region ());

  }

  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  for (std::vector<std::pair<db::Region, unsigned int> >::const_iterator r = regions.begin (); r != regions.end (); ++r) {
    target.insert (target_top_cell_index, r->second, r->first);
  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au2.gds");
}

TEST(3_BoolAndNot)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));
  unsigned int l42 = ly.get_layer (db::LayerProperties (42, 0));

  db::Region r2 (db::RecursiveShapeIterator (ly, top_cell, l2));
  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3));
  db::Region r42 (db::RecursiveShapeIterator (ly, top_cell, l42));
  db::Region box (db::Box (2000, -1000, 6000, 4000));

  db::Region r2minus3   = r2 - r3;
  db::Region r2minusbox = r2 - box;
  db::Region r2minus42  = r2 - r42;
  db::Region rboxminus3 = box - r3;
  db::Region r42minus3  = r42 - r3;
  db::Region r42minus42 = r42 - r42;

  db::Region tr2minus3   = r2.andnot (r3).second;
  db::Region tr2minusbox = r2.andnot (box).second;
  db::Region tr2minus42  = r2.andnot (r42).second;
  db::Region trboxminus3 = box.andnot (r3).second;
  db::Region tr42minus3  = r42.andnot (r3).second;
  db::Region tr42minus42 = r42.andnot (r42).second;

  db::Region r2and3   = r2 & r3;
  db::Region r2andbox = r2 & box;
  db::Region r2and42  = r2 & r42;
  db::Region rboxand3 = box & r3;
  db::Region r42and3  = r42 & r3;
  db::Region r42and42 = r42 & r42;

  db::Region tr2and3   = r2.andnot (r3).first;
  db::Region tr2andbox = r2.andnot (box).first;
  db::Region tr2and42  = r2.andnot (r42).first;
  db::Region trboxand3 = box.andnot (r3).first;
  db::Region tr42and3  = r42.andnot (r3).first;
  db::Region tr42and42 = r42.andnot (r42).first;

  {
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
    db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au3.gds");
  }

  {
    db::Layout target;
    unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), tr2minus3);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), tr2minusbox);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), tr2minus42);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), trboxminus3);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (14, 0)), tr42minus3);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (15, 0)), tr42minus42);

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), tr2and3);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (21, 0)), tr2andbox);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (22, 0)), tr2and42);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (23, 0)), trboxand3);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (24, 0)), tr42and3);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (25, 0)), tr42and42);

    CHECKPOINT();
    db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au3b.gds");
  }
}

TEST(4_Add)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));
  unsigned int l42 = ly.get_layer (db::LayerProperties (42, 0));

  db::Region r2 (db::RecursiveShapeIterator (ly, top_cell, l2));
  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3));
  db::Region r42 (db::RecursiveShapeIterator (ly, top_cell, l42));
  db::Region box (db::Box (2000, -1000, 6000, 4000));
  db::Region r2box (db::RecursiveShapeIterator (ly, top_cell, l2, box));
  //  TODO: currently, original layer regions don't clip - emulate this
  r2box &= db::Region (box);
  db::Region r3box (db::RecursiveShapeIterator (ly, top_cell, l3, box));
  //  TODO: currently, original layer regions don't clip - emulate this
  r3box &= db::Region (box);

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
    db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au4a.gds");
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
    db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au4b.gds");
  }
}

TEST(5_BoolXOR)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));
  unsigned int l42 = ly.get_layer (db::LayerProperties (42, 0));

  db::Region r2 (db::RecursiveShapeIterator (ly, top_cell, l2));
  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3));
  db::Region r42 (db::RecursiveShapeIterator (ly, top_cell, l42));
  db::Region box (db::Box (2000, -1000, 6000, 4000));

  db::Region r2xor3   = r2 ^ r3;
  db::Region r2xorbox = r2 ^ box;
  db::Region r2xor42  = r2 ^ r42;
  db::Region rboxxor3 = box ^ r3;
  db::Region r42xor3  = r42 ^ r3;
  db::Region r42xor42 = r42 ^ r42;

  EXPECT_EQ (r2xor3.is_merged (), true);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r2xor3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r2xorbox);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r2xor42);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), rboxxor3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (14, 0)), r42xor3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (15, 0)), r42xor42);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au5.gds");
}

TEST(7_Merge)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l6 = ly.get_layer (db::LayerProperties (6, 0));

  db::Region r6 (db::RecursiveShapeIterator (ly, top_cell, l6));

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
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au7.gds");
}

TEST(8_AreaAndPerimeter)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_area_peri_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1));
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
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l6 = ly.get_layer (db::LayerProperties (6, 0));

  db::Region r6 (db::RecursiveShapeIterator (ly, top_cell, l6));
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
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au9a.gds");
}

TEST(10_HullsAndHoles)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_area_peri_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1));
  db::Region r1_sized = r1.sized (2000);
  r1_sized -= r1;

  db::Region hulls = r1_sized.hulls ();
  db::Region holes = r1_sized.holes ();
  EXPECT_EQ (hulls.is_merged (), false);
  EXPECT_EQ (holes.is_merged (), false);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r1_sized);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), hulls);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), holes);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au10.gds");
}

TEST(11_RoundAndSmoothed)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_area_peri_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1));
  db::Region r1_sized = r1.sized (2000);
  r1_sized -= r1;

  db::Region rounded = r1_sized.rounded_corners (3000, 5000, 100);
  db::Region smoothed = rounded.smoothed (100, false);
  db::Region smoothed_keep_hv = rounded.smoothed (100, true);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r1_sized);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), rounded);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), smoothed);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), smoothed_keep_hv);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au11.gds");
}

TEST(12_GridSnap)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));

  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3));
  db::Region r3snapped = r3.snapped (50, 50);
  EXPECT_EQ (r3snapped.is_merged (), false);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r3snapped);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au12.gds");
}

TEST(13_Edges)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));

  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3));
  db::Edges r3edges = r3.edges ();
  EXPECT_EQ (r3edges.is_merged (), false);

  db::EdgeLengthFilter f (0, 500, true);
  db::Edges r3edges_filtered = r3.edges (f);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r3);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r3edges);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r3edges_filtered);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au13.gds");
}

TEST(13b_Edges)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_edges.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1));
  db::Edges r1edges = r1.edges ();
  EXPECT_EQ (r1edges.is_merged (), false);

  db::Region r2 (db::RecursiveShapeIterator (ly, top_cell, l2));
  db::Edges r2edges = r2.edges ();
  EXPECT_EQ (r2edges.is_merged (), false);


  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (1, 0)), r1);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (2, 0)), r2);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r1edges);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r2edges);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au13b.gds");
}

TEST(14_Interacting)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l6 = ly.get_layer (db::LayerProperties (6, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1));
  db::Region r2 (db::RecursiveShapeIterator (ly, top_cell, l2));
  db::Region r6 (db::RecursiveShapeIterator (ly, top_cell, l6));
  db::Region r1f (db::RecursiveShapeIterator (ly, top_cell, l1));
  db::Region r1r = r1;
  r1r.set_merged_semantics (false);
  db::Region r2r = r2;
  r2r.set_merged_semantics (false);
  db::Region r6r = r6;
  r6r.set_merged_semantics (false);

  db::Edges r1e = r1.edges ();
  db::Edges r1ef = r1f.edges ();
  db::Edges r1er = r1r.edges ();
  r1er.set_merged_semantics (false);

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

    EXPECT_EQ (r2.selected_interacting (r1).is_merged (), true);
    EXPECT_EQ (r2r.selected_interacting (r1).is_merged (), false);
    EXPECT_EQ (r2r.selected_interacting (r1.merged ()).is_merged (), false);
    EXPECT_EQ (r2.selected_interacting (r1r).is_merged (), true);
    EXPECT_EQ (r2.selected_inside (r1).is_merged (), true);
    EXPECT_EQ (r2r.selected_inside (r1).is_merged (), false);
    EXPECT_EQ (r2.selected_inside (r1).is_merged (), true);

    CHECKPOINT();
    db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au14a.gds");
  }

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
    EXPECT_EQ (r6.selected_interacting (r1er).is_merged (), true);
    EXPECT_EQ (r6r.selected_interacting (r1e).is_merged (), false);
    EXPECT_EQ (r6r.selected_interacting (r1er).is_merged (), false);

    CHECKPOINT();
    db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au14b.gds");
  }
}

TEST(15_Filtered)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_area_peri_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1));
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
    db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au15a.gds");
  }

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));

  db::Region r2 (db::RecursiveShapeIterator (ly, top_cell, l2));
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
    db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au15b.gds");
  }
}

TEST(16_MergeWithMinWC)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_area_peri_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1));
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
    db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au16.gds");
  }
}

TEST(17_SinglePolygonChecks)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));
  unsigned int l6 = ly.get_layer (db::LayerProperties (6, 0));
  unsigned int l4 = ly.get_layer (db::LayerProperties (4, 0));

  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3));
  db::Region r6 (db::RecursiveShapeIterator (ly, top_cell, l6));
  db::Region r4 (db::RecursiveShapeIterator (ly, top_cell, l4));

  {
    db::Layout target;
    unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (3, 0)), r3);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (4, 0)), r4);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (6, 0)), r6);

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r3.width_check (260, db::RegionCheckOptions (false, db::Euclidian, 90, 0)));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r3.width_check (260, db::RegionCheckOptions (true, db::Projection, 90, 2000)));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), r6.notch_check (1300, db::RegionCheckOptions (false, db::Euclidian, 90, 0)));

    CHECKPOINT();
    db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au17.gds");
  }
}

TEST(18_MultiPolygonChecks)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));
  unsigned int l6 = ly.get_layer (db::LayerProperties (6, 0));
  unsigned int l4 = ly.get_layer (db::LayerProperties (4, 0));

  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3));
  db::Region r6 (db::RecursiveShapeIterator (ly, top_cell, l6));
  db::Region r4 (db::RecursiveShapeIterator (ly, top_cell, l4));

  {
    db::Layout target;
    unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (3, 0)), r3);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (4, 0)), r4);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (6, 0)), r6);

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r3.space_check (500, db::RegionCheckOptions (false, db::Projection, 90, 0)));
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r3.space_check (500, db::RegionCheckOptions (true, db::Projection, 90, 300)));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), r3.separation_check (r4, 200, db::RegionCheckOptions (false, db::Projection, 90, 0)));

    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (30, 0)), r6.enclosing_check (r4, 100, db::RegionCheckOptions (true, db::Projection, 90, 0)));

    CHECKPOINT();
    db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au18.gds");
  }
}

TEST(19_GridCheck)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));

  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3));
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
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au19.gds");
}

TEST(20_AngleCheck)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/angle_check_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1));
  db::EdgePairs ep1_ac1 = r1.angle_check (0, 91, true);
  db::EdgePairs ep1_ac2 = r1.angle_check (0, 45, false);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (1, 0)), r1);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (2, 0)), ep1_ac1);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (3, 0)), ep1_ac2);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au20.gds");
}

TEST(21_Processors)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_area_peri_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1));

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (1, 0)), r1);

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r1.processed (db::CornersAsDots (-180.0, true, 180.0, true, false, false)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r1.processed (db::CornersAsDots (0.0, true, 180.0, true, false, false)));
  db::Region ext;
  r1.processed (db::CornersAsDots (0.0, true, 180.0, true, false, false)).extended (ext, 1000, 1000, 2000, 2000);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), ext);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), r1.processed (db::CornersAsRectangles (-180.0, true, 180.0, true, false, false, 2000)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (14, 0)), r1.processed (db::CornersAsRectangles (0.0, true, 180.0, true, false, false, 2000)));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), r1.processed (db::extents_processor<db::Polygon> (0, 0)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (21, 0)), r1.processed (db::extents_processor<db::Polygon> (1000, 2000)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (22, 0)), r1.processed (db::RelativeExtents (0, 0, 1.0, 1.0, 0, 0)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (23, 0)), r1.processed (db::RelativeExtents (0.25, 0.4, 0.75, 0.6, 1000, 2000)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (24, 0)), r1.processed (db::RelativeExtentsAsEdges (0, 0, 1.0, 1.0)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (25, 0)), r1.processed (db::RelativeExtentsAsEdges (0.5, 0.5, 0.5, 0.5)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (26, 0)), r1.processed (db::RelativeExtentsAsEdges (0.25, 0.4, 0.75, 0.6)));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (30, 0)), r1.processed (db::minkowski_sum_computation<db::Box> (db::Box (-1000, -2000, 3000, 4000))));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (31, 0)), r1.processed (db::minkowski_sum_computation<db::Edge> (db::Edge (-1000, 0, 3000, 0))));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (40, 0)), r1.processed (db::TrapezoidDecomposition (db::TD_htrapezoids)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (41, 0)), r1.processed (db::ConvexDecomposition (db::PO_vertical)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (42, 0)), r1.processed (db::ConvexDecomposition (db::PO_horizontal)));

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au21.gds");
}

TEST(22_TwoLayoutsWithDifferentDBU)
{
  db::Layout ly1;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_area_peri_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly1);
  }

  db::cell_index_type top_cell_index1 = *ly1.begin_top_down ();
  db::Cell &top_cell1 = ly1.cell (top_cell_index1);

  db::Layout ly2;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_area_peri_l1_dbu2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly2);
  }

  db::cell_index_type top_cell_index2 = *ly2.begin_top_down ();
  db::Cell &top_cell2 = ly2.cell (top_cell_index2);

  unsigned int l11 = ly1.get_layer (db::LayerProperties (1, 0));
  db::Region r11 (db::RecursiveShapeIterator (ly1, top_cell1, l11));

  unsigned int l12 = ly2.get_layer (db::LayerProperties (2, 0));
  db::Region r12 (db::RecursiveShapeIterator (ly2, top_cell2, l12), db::ICplxTrans (ly2.dbu () / ly1.dbu ()));

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly1.cell_name (top_cell_index1));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (1, 0)), r11.sized (1000) ^ r12);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au22.gds");
}

TEST(27a_snap)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/scale_and_snap.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1));
  r1.set_merged_semantics (false);
  db::Region r2 = r1.snapped (19, 19);

  r2.insert_into (&ly, top_cell_index, ly.get_layer (db::LayerProperties (100, 0)));

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testdata () + "/algo/flat_region_au27.gds");
}

TEST(27b_snap)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/scale_and_snap.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1));
  r1.set_merged_semantics (false);
  r1.snap (19, 19);

  r1.insert_into (&ly, top_cell_index, ly.get_layer (db::LayerProperties (100, 0)));

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testdata () + "/algo/flat_region_au27.gds");
}

TEST(28a_snap)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/scale_and_snap.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1));
  r1.set_merged_semantics (false);
  db::Region r2 = r1.scaled_and_snapped (19, 2, 10, 19, 2, 10);

  r2.insert_into (&ly, top_cell_index, ly.get_layer (db::LayerProperties (100, 0)));

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testdata () + "/algo/flat_region_au28.gds");
}

TEST(28b_snap)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/scale_and_snap.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1));
  r1.set_merged_semantics (false);
  r1.scale_and_snap (19, 2, 10, 19, 2, 10);

  r1.insert_into (&ly, top_cell_index, ly.get_layer (db::LayerProperties (100, 0)));

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testdata () + "/algo/flat_region_au28.gds");
}

TEST(29_InteractionsWithTexts)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_texts_l2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l8 = ly.get_layer (db::LayerProperties (8, 0));

  db::Texts texts2 (db::RecursiveShapeIterator (ly, top_cell, l2));
  db::Region polygons8 (db::RecursiveShapeIterator (ly, top_cell, l8));

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
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au29.gds");
}

TEST(30a_interact_with_count_region)
{
  db::Layout ly;
  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));

  db::Cell &top = ly.cell (ly.add_cell ("TOP"));
  db::cell_index_type ci1 = ly.add_cell ("C1");
  db::Cell &c1 = ly.cell (ci1);
  db::cell_index_type ci2 = ly.add_cell ("C2");
  db::Cell &c2 = ly.cell (ci2);
  top.insert (db::CellInstArray (db::CellInst (ci1), db::Trans ()));
  top.insert (db::CellInstArray (db::CellInst (ci2), db::Trans ()));

  c1.shapes (l1).insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  c1.shapes (l1).insert (db::Box (db::Point (-100, -100), db::Point (0, 0)));

  c2.shapes (l2).insert (db::Box (db::Point (-10, -10), db::Point (10, 0)));
  c2.shapes (l2).insert (db::Box (db::Point (-10, 0), db::Point (10, 10)));
  c2.shapes (l2).insert (db::Box (db::Point (-110, -10), db::Point (-90, 10)));
  c2.shapes (l2).insert (db::Box (db::Point (-110, -210), db::Point (-90, -190)));

  ly.copy_layer (l2, l3);
  top.shapes (l2).insert (db::Box (db::Point (90, -10), db::Point (110, 10)));
  top.shapes (l2).insert (db::Box (db::Point (-110, -110), db::Point (-90, -90)));

  db::Region r (db::RecursiveShapeIterator (ly, top, l1));
  r.set_merged_semantics (true);
  r.set_min_coherence (false);

  db::Region empty;

  db::Region rr (db::RecursiveShapeIterator (ly, top, l2));
  db::Region rr2 (db::RecursiveShapeIterator (ly, top, l3));

  EXPECT_EQ (r.selected_interacting (empty).to_string (), "");
  EXPECT_EQ (r.selected_interacting (rr).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr, 0, 2).to_string (), "");
  EXPECT_EQ (r.selected_interacting (rr, 1, 2).to_string (), "");
  EXPECT_EQ (r.selected_interacting (rr, 1, 4).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr, 2, 4).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr, 2, 1).to_string (), "");
  EXPECT_EQ (r.selected_interacting (rr, 3, 4).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr, 4, 5).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr, 5, 5).to_string (), "");
  EXPECT_EQ (r.selected_interacting (rr2).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr2, 1, 2).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr2, 1, 4).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr2, 2, 5).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr2, 4, 5).to_string (), "");

  EXPECT_EQ (r.selected_not_interacting (empty).to_string (), "(0,0;0,200;100,200;100,0);(-100,-100;-100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr, 0, 2).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr, 1, 2).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr, 1, 4).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr, 2, 4).to_string (), "");
  //  Shortcut delivers clone of original, not merged one:
  //    EXPECT_EQ (r.selected_not_interacting (rr, 2, 1).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  //  so that is:
  EXPECT_EQ (r.selected_not_interacting (rr, 2, 1).to_string (), "(0,0;0,200;100,200;100,0);(-100,-100;-100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr, 3, 4).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr, 4, 5).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr, 5, 5).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr2).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr2, 1, 2).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr2, 1, 4).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr2, 2, 5).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr2, 4, 5).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");

  r.set_merged_semantics (false);

  EXPECT_EQ (r.selected_interacting (empty).to_string (), "");
  EXPECT_EQ (db::compare (r.selected_interacting (rr), "(0,0;0,200;100,200;100,0);(-100,-100;-100,0;0,0;0,-100)"), true);
  EXPECT_EQ (r.selected_interacting (rr, 0, 2).to_string (), "(0,0;0,200;100,200;100,0)");
  EXPECT_EQ (r.selected_interacting (rr, 1, 2).to_string (), "(0,0;0,200;100,200;100,0)");
  EXPECT_EQ (db::compare (r.selected_interacting (rr, 1, 4), "(0,0;0,200;100,200;100,0);(-100,-100;-100,0;0,0;0,-100)"), true);
  EXPECT_EQ (db::compare (r.selected_interacting (rr, 2, 4), "(0,0;0,200;100,200;100,0);(-100,-100;-100,0;0,0;0,-100)"), true);
  EXPECT_EQ (r.selected_interacting (rr, 2, 1).to_string (), "");
  EXPECT_EQ (r.selected_interacting (rr, 3, 4).to_string (), "(-100,-100;-100,0;0,0;0,-100)");

  EXPECT_EQ (db::compare (r.selected_not_interacting (empty), "(0,0;0,200;100,200;100,0);(-100,-100;-100,0;0,0;0,-100)"), true);
  EXPECT_EQ (r.selected_not_interacting (rr).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr, 0, 2).to_string (), "(-100,-100;-100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr, 1, 2).to_string (), "(-100,-100;-100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr, 1, 4).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr, 2, 4).to_string (), "");
  EXPECT_EQ (db::compare (r.selected_not_interacting (rr, 2, 1), "(0,0;0,200;100,200;100,0);(-100,-100;-100,0;0,0;0,-100)"), true);
  EXPECT_EQ (r.selected_not_interacting (rr, 3, 4).to_string (), "(0,0;0,200;100,200;100,0)");
}

TEST(30b_interact_with_count_edge)
{
  db::Layout ly;
  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));

  db::Cell &top = ly.cell (ly.add_cell ("TOP"));
  db::cell_index_type ci1 = ly.add_cell ("C1");
  db::Cell &c1 = ly.cell (ci1);
  db::cell_index_type ci2 = ly.add_cell ("C2");
  db::Cell &c2 = ly.cell (ci2);
  top.insert (db::CellInstArray (db::CellInst (ci1), db::Trans ()));
  top.insert (db::CellInstArray (db::CellInst (ci2), db::Trans ()));

  c1.shapes (l1).insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  c1.shapes (l1).insert (db::Box (db::Point (-100, -100), db::Point (0, 0)));

  c2.shapes (l2).insert (db::Edge (db::Point (-10, -10), db::Point (0, 0)));
  c2.shapes (l2).insert (db::Edge (db::Point (0, 0), db::Point (10, 10)));
  c2.shapes (l2).insert (db::Edge (db::Point (-110, -10), db::Point (-90, 10)));
  c2.shapes (l2).insert (db::Edge (db::Point (-110, -210), db::Point (-90, -190)));

  ly.copy_layer (l2, l3);
  top.shapes (l2).insert (db::Edge (db::Point (90, -10), db::Point (110, 10)));
  top.shapes (l2).insert (db::Edge (db::Point (-110, -110), db::Point (-90, -90)));

  db::Region r (db::RecursiveShapeIterator (ly, top, l1));
  r.set_merged_semantics (true);
  r.set_min_coherence (false);

  db::Region empty;

  db::Edges rr (db::RecursiveShapeIterator (ly, top, l2));
  db::Edges rr2 (db::RecursiveShapeIterator (ly, top, l3));

  EXPECT_EQ (r.selected_interacting (empty).to_string (), "");
  EXPECT_EQ (r.selected_interacting (rr).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr, 0, 2).to_string (), "");
  EXPECT_EQ (r.selected_interacting (rr, 1, 2).to_string (), "");
  EXPECT_EQ (r.selected_interacting (rr, 1, 4).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr, 2, 4).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr, 2, 1).to_string (), "");
  EXPECT_EQ (r.selected_interacting (rr, 3, 4).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr, 4, 5).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr, 5, 5).to_string (), "");
  EXPECT_EQ (r.selected_interacting (rr2).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr2, 1, 2).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr2, 1, 4).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr2, 2, 5).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr2, 4, 5).to_string (), "");

  EXPECT_EQ (r.selected_not_interacting (empty).to_string (), "(0,0;0,200;100,200;100,0);(-100,-100;-100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr, 0, 2).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr, 1, 2).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr, 1, 4).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr, 2, 4).to_string (), "");
  //  Shortcut delivers clone of original, not merged one:
  //    EXPECT_EQ (r.selected_not_interacting (rr, 2, 1).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  //  so that is:
  EXPECT_EQ (r.selected_not_interacting (rr, 2, 1).to_string (), "(0,0;0,200;100,200;100,0);(-100,-100;-100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr, 3, 4).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr, 4, 5).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr, 5, 5).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr2).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr2, 1, 2).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr2, 1, 4).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr2, 2, 5).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr2, 4, 5).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");

  r.set_merged_semantics (false);

  EXPECT_EQ (r.selected_interacting (empty).to_string (), "");
  EXPECT_EQ (db::compare (r.selected_interacting (rr), "(-100,-100;-100,0;0,0;0,-100);(0,0;0,200;100,200;100,0)"), true);
  EXPECT_EQ (r.selected_interacting (rr, 0, 2).to_string (), "(0,0;0,200;100,200;100,0)");
  EXPECT_EQ (r.selected_interacting (rr, 1, 2).to_string (), "(0,0;0,200;100,200;100,0)");
  EXPECT_EQ (db::compare (r.selected_interacting (rr, 1, 4), "(-100,-100;-100,0;0,0;0,-100);(0,0;0,200;100,200;100,0)"), true);
  EXPECT_EQ (db::compare (r.selected_interacting (rr, 2, 4), "(-100,-100;-100,0;0,0;0,-100);(0,0;0,200;100,200;100,0)"), true);
  EXPECT_EQ (r.selected_interacting (rr, 2, 1).to_string (), "");
  EXPECT_EQ (r.selected_interacting (rr, 3, 4).to_string (), "(-100,-100;-100,0;0,0;0,-100)");

  EXPECT_EQ (db::compare (r.selected_not_interacting (empty), "(0,0;0,200;100,200;100,0);(-100,-100;-100,0;0,0;0,-100)"), true);
  EXPECT_EQ (r.selected_not_interacting (rr).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr, 0, 2).to_string (), "(-100,-100;-100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr, 1, 2).to_string (), "(-100,-100;-100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr, 1, 4).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr, 2, 4).to_string (), "");
  EXPECT_EQ (db::compare (r.selected_not_interacting (rr, 2, 1), "(-100,-100;-100,0;0,0;0,-100);(0,0;0,200;100,200;100,0)"), true);
  EXPECT_EQ (r.selected_not_interacting (rr, 3, 4).to_string (), "(0,0;0,200;100,200;100,0)");
}

TEST(30c_interact_with_count_text)
{
  db::Layout ly;
  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));

  db::Cell &top = ly.cell (ly.add_cell ("TOP"));
  db::cell_index_type ci1 = ly.add_cell ("C1");
  db::Cell &c1 = ly.cell (ci1);
  db::cell_index_type ci2 = ly.add_cell ("C2");
  db::Cell &c2 = ly.cell (ci2);
  top.insert (db::CellInstArray (db::CellInst (ci1), db::Trans ()));
  top.insert (db::CellInstArray (db::CellInst (ci2), db::Trans ()));

  c1.shapes (l1).insert (db::Box (db::Point (0, 0), db::Point (100, 200)));
  c1.shapes (l1).insert (db::Box (db::Point (-100, -100), db::Point (0, 0)));

  c2.shapes (l2).insert (db::Text ("a", db::Trans (db::Vector (0, 0))));
  c2.shapes (l2).insert (db::Text ("b", db::Trans (db::Vector (-100, 0))));
  c2.shapes (l2).insert (db::Text ("c", db::Trans (db::Vector (-100, -200))));

  ly.copy_layer (l2, l3);
  top.shapes (l2).insert (db::Text ("x", db::Trans (db::Vector (100, 0))));
  top.shapes (l2).insert (db::Text ("y", db::Trans (db::Vector (-100, -100))));

  db::Region r (db::RecursiveShapeIterator (ly, top, l1));
  r.set_merged_semantics (true);
  r.set_min_coherence (false);

  db::Region empty;

  db::Texts rr (db::RecursiveShapeIterator (ly, top, l2));
  db::Texts rr2 (db::RecursiveShapeIterator (ly, top, l3));

  EXPECT_EQ (r.selected_interacting (empty).to_string (), "");
  EXPECT_EQ (r.selected_interacting (rr).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr, 0, 2).to_string (), "");
  EXPECT_EQ (r.selected_interacting (rr, 1, 2).to_string (), "");
  EXPECT_EQ (r.selected_interacting (rr, 1, 4).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr, 2, 4).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr, 2, 1).to_string (), "");
  EXPECT_EQ (r.selected_interacting (rr, 3, 4).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr, 4, 5).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr, 5, 5).to_string (), "");
  EXPECT_EQ (r.selected_interacting (rr2).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr2, 1, 2).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr2, 1, 4).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr2, 2, 5).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_interacting (rr2, 4, 5).to_string (), "");

  EXPECT_EQ (r.selected_not_interacting (empty).to_string (), "(0,0;0,200;100,200;100,0);(-100,-100;-100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr, 0, 2).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr, 1, 2).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr, 1, 4).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr, 2, 4).to_string (), "");
  //  Shortcut delivers clone of original, not merged one:
  //    EXPECT_EQ (r.selected_not_interacting (rr, 2, 1).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  //  so that is:
  EXPECT_EQ (r.selected_not_interacting (rr, 2, 1).to_string (), "(0,0;0,200;100,200;100,0);(-100,-100;-100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr, 3, 4).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr, 4, 5).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr, 5, 5).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr2).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr2, 1, 2).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr2, 1, 4).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr2, 2, 5).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr2, 4, 5).to_string (), "(-100,-100;-100,0;0,0;0,200;100,200;100,0;0,0;0,-100)");

  r.set_merged_semantics (false);

  EXPECT_EQ (r.selected_interacting (empty).to_string (), "");
  EXPECT_EQ (db::compare (r.selected_interacting (rr), "(-100,-100;-100,0;0,0;0,-100);(0,0;0,200;100,200;100,0)"), true);
  EXPECT_EQ (r.selected_interacting (rr, 0, 2).to_string (), "(0,0;0,200;100,200;100,0)");
  EXPECT_EQ (r.selected_interacting (rr, 1, 2).to_string (), "(0,0;0,200;100,200;100,0)");
  EXPECT_EQ (db::compare (r.selected_interacting (rr, 1, 4), "(-100,-100;-100,0;0,0;0,-100);(0,0;0,200;100,200;100,0)"), true);
  EXPECT_EQ (db::compare (r.selected_interacting (rr, 2, 4), "(-100,-100;-100,0;0,0;0,-100);(0,0;0,200;100,200;100,0)"), true);
  EXPECT_EQ (r.selected_interacting (rr, 2, 1).to_string (), "");
  EXPECT_EQ (r.selected_interacting (rr, 3, 4).to_string (), "(-100,-100;-100,0;0,0;0,-100)");

  EXPECT_EQ (db::compare (r.selected_not_interacting (empty), "(0,0;0,200;100,200;100,0);(-100,-100;-100,0;0,0;0,-100)"), true);
  EXPECT_EQ (r.selected_not_interacting (rr).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr, 0, 2).to_string (), "(-100,-100;-100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr, 1, 2).to_string (), "(-100,-100;-100,0;0,0;0,-100)");
  EXPECT_EQ (r.selected_not_interacting (rr, 1, 4).to_string (), "");
  EXPECT_EQ (r.selected_not_interacting (rr, 2, 4).to_string (), "");
  EXPECT_EQ (db::compare (r.selected_not_interacting (rr, 2, 1), "(-100,-100;-100,0;0,0;0,-100);(0,0;0,200;100,200;100,0)"), true);
  EXPECT_EQ (r.selected_not_interacting (rr, 3, 4).to_string (), "(0,0;0,200;100,200;100,0)");
}

TEST(31_in)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_l31.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));  //  empty

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1));
  db::Region r2 (db::RecursiveShapeIterator (ly, top_cell, l2));
  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3));
  db::Region r1r = r1;
  r1r.set_merged_semantics (false);
  db::Region r2r = r2;
  r2r.set_merged_semantics (false);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (1, 0)), r1);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (2, 0)), r2);

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r2.in (r1));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r2.in (r1, true));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r2.in (r3, false));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), r2.in (r3, true));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (14, 0)), r3.in (r1, false));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (15, 0)), r3.in (r1, true));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), r2r.in (r1));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (21, 0)), r2r.in (r1, true));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (30, 0)), r2.in (r1r));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (31, 0)), r2.in (r1r, true));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (40, 0)), r2r.in (r1r));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (41, 0)), r2r.in (r1r, true));

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au31.gds");
}

TEST(31_in_and_out)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_l31.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));  //  empty

  db::Region r1 (db::RecursiveShapeIterator (ly, top_cell, l1));
  db::Region r2 (db::RecursiveShapeIterator (ly, top_cell, l2));
  db::Region r3 (db::RecursiveShapeIterator (ly, top_cell, l3));

  db::Region r1r = r1;
  r1r.set_merged_semantics (false);
  db::Region r2r = r2;
  r2r.set_merged_semantics (false);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (1, 0)), r1);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (2, 0)), r2);

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r2.in_and_out (r1).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r2.in_and_out (r1).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r2.in_and_out (r3).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), r2.in_and_out (r3).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (14, 0)), r3.in_and_out (r1).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (15, 0)), r3.in_and_out (r1).second);

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), r2r.in_and_out (r1).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (21, 0)), r2r.in_and_out (r1).second);

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (30, 0)), r2.in_and_out (r1r).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (31, 0)), r2.in_and_out (r1r).second);

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (40, 0)), r2r.in_and_out (r1r).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (41, 0)), r2r.in_and_out (r1r).second);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au31.gds");
}

TEST(40_BoolWithProperties)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_40.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));  //  empty

  db::RecursiveShapeIterator si1 (ly, top_cell, l1);
  si1.apply_property_translator (db::PropertiesTranslator::make_pass_all ());
  db::Region r1 (si1);

  db::RecursiveShapeIterator si2 (ly, top_cell, l2);
  si2.apply_property_translator (db::PropertiesTranslator::make_pass_all ());
  db::Region r2 (si2);

  db::RecursiveShapeIterator si3 (ly, top_cell, l3);
  si3.apply_property_translator (db::PropertiesTranslator::make_pass_all ());
  db::Region r3 (si3);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (1, 0)), r1);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (2, 0)), r2);

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r1.merged ());
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r2.merged ());

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), r1 & r2);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (21, 0)), r1.bool_and (r2, db::NoPropertyConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (22, 0)), r1.bool_and (r2, db::SamePropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (23, 0)), r1.bool_and (r2, db::DifferentPropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (24, 0)), r3.bool_and (r2, db::SamePropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (25, 0)), r3.bool_and (r2, db::DifferentPropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (26, 0)), r1.bool_and (r3, db::SamePropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (27, 0)), r1.bool_and (r3, db::DifferentPropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (22, 1)), r1.bool_and (r2, db::SamePropertiesConstraintDrop));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (23, 1)), r1.bool_and (r2, db::DifferentPropertiesConstraintDrop));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (24, 1)), r3.bool_and (r2, db::SamePropertiesConstraintDrop));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (25, 1)), r3.bool_and (r2, db::DifferentPropertiesConstraintDrop));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (26, 1)), r1.bool_and (r3, db::SamePropertiesConstraintDrop));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (27, 1)), r1.bool_and (r3, db::DifferentPropertiesConstraintDrop));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (30, 0)), r1 - r2);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (31, 0)), r1.bool_not (r2, db::NoPropertyConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (32, 0)), r1.bool_not (r2, db::SamePropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (33, 0)), r1.bool_not (r2, db::DifferentPropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (34, 0)), r3.bool_not (r2, db::SamePropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (35, 0)), r3.bool_not (r2, db::DifferentPropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (36, 0)), r1.bool_not (r3, db::SamePropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (37, 0)), r1.bool_not (r3, db::DifferentPropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (32, 1)), r1.bool_not (r2, db::SamePropertiesConstraintDrop));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (33, 1)), r1.bool_not (r2, db::DifferentPropertiesConstraintDrop));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (34, 1)), r3.bool_not (r2, db::SamePropertiesConstraintDrop));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (35, 1)), r3.bool_not (r2, db::DifferentPropertiesConstraintDrop));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (36, 1)), r1.bool_not (r3, db::SamePropertiesConstraintDrop));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (37, 1)), r1.bool_not (r3, db::DifferentPropertiesConstraintDrop));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (40, 0)), r1.andnot (r2).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (41, 0)), r1.andnot (r2).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (42, 0)), r1.andnot (r2, db::SamePropertiesConstraint).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (43, 0)), r1.andnot (r2, db::SamePropertiesConstraint).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (44, 0)), r1.andnot (r2, db::DifferentPropertiesConstraint).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (45, 0)), r1.andnot (r2, db::DifferentPropertiesConstraint).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (46, 0)), r3.andnot (r2, db::SamePropertiesConstraint).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (47, 0)), r3.andnot (r2, db::SamePropertiesConstraint).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (48, 0)), r3.andnot (r2, db::DifferentPropertiesConstraint).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (49, 0)), r3.andnot (r2, db::DifferentPropertiesConstraint).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (50, 0)), r1.andnot (r3, db::SamePropertiesConstraint).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (51, 0)), r1.andnot (r3, db::SamePropertiesConstraint).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (52, 0)), r1.andnot (r3, db::DifferentPropertiesConstraint).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (53, 0)), r1.andnot (r3, db::DifferentPropertiesConstraint).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (42, 1)), r1.andnot (r2, db::SamePropertiesConstraintDrop).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (43, 1)), r1.andnot (r2, db::SamePropertiesConstraintDrop).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (44, 1)), r1.andnot (r2, db::DifferentPropertiesConstraintDrop).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (45, 1)), r1.andnot (r2, db::DifferentPropertiesConstraintDrop).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (46, 1)), r3.andnot (r2, db::SamePropertiesConstraintDrop).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (47, 1)), r3.andnot (r2, db::SamePropertiesConstraintDrop).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (48, 1)), r3.andnot (r2, db::DifferentPropertiesConstraintDrop).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (49, 1)), r3.andnot (r2, db::DifferentPropertiesConstraintDrop).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (50, 1)), r1.andnot (r3, db::SamePropertiesConstraintDrop).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (51, 1)), r1.andnot (r3, db::SamePropertiesConstraintDrop).second);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (52, 1)), r1.andnot (r3, db::DifferentPropertiesConstraintDrop).first);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (53, 1)), r1.andnot (r3, db::DifferentPropertiesConstraintDrop).second);

  db::Box clip (0, 8000, 10000, 15000);
  db::Region clip_region;
  clip_region.insert (clip);

  db::Region clip_region_wp (new db::FlatRegion ());
  db::PropertiesSet ps;
  ps.insert (tl::Variant (1), "42");   //  "42" needs to be a string as GDS properties are strings as well
  db::properties_id_type pid42 = db::properties_id (ps);
  clip_region_wp.insert (db::BoxWithProperties (clip, pid42));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (60, 0)), r1.bool_and (clip_region));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (61, 0)), r1.bool_and (clip_region_wp));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (62, 0)), clip_region.bool_and (r1));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (63, 0)), clip_region_wp.bool_and (r1));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (64, 0)), clip_region_wp.bool_and (clip_region));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (65, 0)), clip_region_wp.bool_and (clip_region_wp));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (70, 0)), r1.bool_and (clip_region, db::SamePropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (71, 0)), r1.bool_and (clip_region_wp, db::SamePropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (72, 0)), clip_region.bool_and (r1, db::SamePropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (73, 0)), clip_region_wp.bool_and (r1, db::SamePropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (74, 0)), clip_region_wp.bool_and (clip_region, db::SamePropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (75, 0)), clip_region_wp.bool_and (clip_region_wp, db::SamePropertiesConstraint));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (80, 0)), r1.bool_and (clip_region, db::SamePropertiesConstraintDrop));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (81, 0)), r1.bool_and (clip_region_wp, db::SamePropertiesConstraintDrop));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (82, 0)), clip_region.bool_and (r1, db::SamePropertiesConstraintDrop));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (83, 0)), clip_region_wp.bool_and (r1, db::SamePropertiesConstraintDrop));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (84, 0)), clip_region_wp.bool_and (clip_region, db::SamePropertiesConstraintDrop));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (85, 0)), clip_region_wp.bool_and (clip_region_wp, db::SamePropertiesConstraintDrop));

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au40.gds");
}

TEST(41_EdgesWithProperties)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_40.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));

  db::RecursiveShapeIterator si1 (ly, top_cell, l1);
  si1.apply_property_translator (db::PropertiesTranslator::make_pass_all ());
  db::Region r1wp (si1);
  db::Region r1wp_nomerge = r1wp;
  r1wp_nomerge.set_merged_semantics (false);

  si1 = db::RecursiveShapeIterator (ly, top_cell, l1);
  db::Region r1 (si1);

  db::RecursiveShapeIterator si2 (ly, top_cell, l2);
  si2.apply_property_translator (db::PropertiesTranslator::make_pass_all ());
  db::Region r2wp (si2);
  db::Region r2wp_nomerge = r2wp;
  r2wp_nomerge.set_merged_semantics (false);

  si2.apply_property_translator (db::PropertiesTranslator::make_remove_all ());
  db::Region r2 (si2);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (1, 0)), r1wp);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (2, 0)), r2wp);

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r1.edges ());
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r1wp.edges ());
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r1wp_nomerge.edges ());

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), r2.edges ());
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (21, 0)), r2wp.edges ());
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (22, 0)), r2wp_nomerge.edges ());

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au41.gds");
}

TEST(42_DRCWithProperties)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_42.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));

  db::RecursiveShapeIterator si1 (ly, top_cell, l1);
  si1.apply_property_translator (db::PropertiesTranslator::make_pass_all ());
  db::Region r1 (si1);
  db::Region r1_nomerge (r1);
  r1_nomerge.set_merged_semantics (false);

  db::RecursiveShapeIterator si2 (ly, top_cell, l2);
  si2.apply_property_translator (db::PropertiesTranslator::make_pass_all ());
  db::Region r2 (si2);
  db::Region r2_nomerge (r2);
  r2_nomerge.set_merged_semantics (false);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  db::RegionCheckOptions opt;
  opt.metrics = db::Projection;

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (1, 0)), r1);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (2, 0)), r2);

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r1.space_check (1000, opt));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r1.separation_check (r2, 1000, opt));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r2.space_check (1000, opt));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), r1_nomerge.space_check (1000, opt));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (14, 0)), r1_nomerge.separation_check (r2, 1000, opt));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (15, 0)), r1.separation_check (r2_nomerge, 1000, opt));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (16, 0)), r1_nomerge.separation_check (r2_nomerge, 1000, opt));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (17, 0)), r2_nomerge.space_check (1000, opt));

  opt.prop_constraint = db::NoPropertyConstraint;

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), r1.space_check (1000, opt));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (21, 0)), r1.separation_check (r2, 1000, opt));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (22, 0)), r2.space_check (1000, opt));

  opt.prop_constraint = db::SamePropertiesConstraint;

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (30, 0)), r1.space_check (1000, opt));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (31, 0)), r1.separation_check (r2, 1000, opt));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (32, 0)), r2.space_check (1000, opt));

  opt.prop_constraint = db::SamePropertiesConstraintDrop;

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (30, 1)), r1.space_check (1000, opt));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (31, 1)), r1.separation_check (r2, 1000, opt));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (32, 1)), r2.space_check (1000, opt));

  opt.prop_constraint = db::DifferentPropertiesConstraint;

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (40, 0)), r1.space_check (1000, opt));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (41, 0)), r1.separation_check (r2, 1000, opt));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (42, 0)), r2.space_check (1000, opt));

  opt.prop_constraint = db::DifferentPropertiesConstraintDrop;

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (40, 1)), r1.space_check (1000, opt));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (41, 1)), r1.separation_check (r2, 1000, opt));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (42, 1)), r2.space_check (1000, opt));

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au42.gds");
}

TEST(43_ComplexOpsWithProperties)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_42.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));

  db::RecursiveShapeIterator si1 (ly, top_cell, l1);
  si1.apply_property_translator (db::PropertiesTranslator::make_pass_all ());
  db::Region r1 (si1);

  db::RecursiveShapeIterator si2 (ly, top_cell, l2);
  si2.apply_property_translator (db::PropertiesTranslator::make_pass_all ());
  db::Region r2 (si2);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  db::RegionCheckOptions opt;
  opt.metrics = db::Projection;

  db::CompoundRegionOperationSecondaryNode *secondary = new db::CompoundRegionOperationSecondaryNode (&r2);
  db::CompoundRegionCheckOperationNode sep_check (secondary, db::SpaceRelation, true /*==different polygons*/, 1000, opt);

  db::CompoundRegionOperationSecondaryNode *secondary2 = new db::CompoundRegionOperationSecondaryNode (&r2);
  db::CompoundRegionCheckOperationNode *sep_check2 = new db::CompoundRegionCheckOperationNode (secondary2, db::SpaceRelation, true /*==different polygons*/, 1000, opt);
  db::CompoundRegionEdgePairToPolygonProcessingOperationNode sep_check2p (new db::EdgePairToPolygonProcessor (0), sep_check2, true);

  db::CompoundRegionOperationSecondaryNode *secondary3 = new db::CompoundRegionOperationSecondaryNode (&r2);
  db::CompoundRegionCheckOperationNode *sep_check3 = new db::CompoundRegionCheckOperationNode (secondary3, db::SpaceRelation, true /*==different polygons*/, 1000, opt);
  db::CompoundRegionEdgePairToEdgeProcessingOperationNode sep_check2e (new db::EdgePairToEdgesProcessor (), sep_check3, true);

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (1, 0)), r1);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (2, 0)), r2);

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r1.cop_to_edge_pairs (sep_check));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r1.cop_to_region (sep_check2p));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), r1.cop_to_edges (sep_check2e));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), r1.cop_to_edge_pairs (sep_check, db::NoPropertyConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (21, 0)), r1.cop_to_region (sep_check2p, db::NoPropertyConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (22, 0)), r1.cop_to_edges (sep_check2e, db::NoPropertyConstraint));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (30, 0)), r1.cop_to_edge_pairs (sep_check, db::SamePropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (31, 0)), r1.cop_to_region (sep_check2p, db::SamePropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (32, 0)), r1.cop_to_edges (sep_check2e, db::SamePropertiesConstraint));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (30, 1)), r1.cop_to_edge_pairs (sep_check, db::SamePropertiesConstraintDrop));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (31, 1)), r1.cop_to_region (sep_check2p, db::SamePropertiesConstraintDrop));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (32, 1)), r1.cop_to_edges (sep_check2e, db::SamePropertiesConstraintDrop));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (40, 0)), r1.cop_to_edge_pairs (sep_check, db::DifferentPropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (41, 0)), r1.cop_to_region (sep_check2p, db::DifferentPropertiesConstraint));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (42, 0)), r1.cop_to_edges (sep_check2e, db::DifferentPropertiesConstraint));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (40, 1)), r1.cop_to_edge_pairs (sep_check, db::DifferentPropertiesConstraintDrop));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (41, 1)), r1.cop_to_region (sep_check2p, db::DifferentPropertiesConstraintDrop));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (42, 1)), r1.cop_to_edges (sep_check2e, db::DifferentPropertiesConstraintDrop));

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au43.gds");
}

TEST(44_SizeWithProperties)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_42.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));

  db::RecursiveShapeIterator si1 (ly, top_cell, l1);
  si1.apply_property_translator (db::PropertiesTranslator::make_pass_all ());
  db::Region r1 (si1);

  db::RecursiveShapeIterator si2 (ly, top_cell, l2);
  si2.apply_property_translator (db::PropertiesTranslator::make_pass_all ());
  db::Region r2 (si2);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (1, 0)), r1);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (2, 0)), r2);

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), r1.sized (200));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), r1.sized (250, 50, 2));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), r2.sized (200));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (21, 0)), r2.sized (250, 50, 2));

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/flat_region_au44.gds");
}

TEST(100_Issue1275)
{
  db::Point pts[] = {
    db::Point (-21983, -43808),
    db::Point (-37841, 16636),
    db::Point (-20484, 35228),
    db::Point (30428, 41627),
    db::Point (38312, 28960),
    db::Point (-7811, -37922)
  };

  db::Polygon polygon;
  polygon.assign_hull (&pts[0], &pts[sizeof(pts) / sizeof(pts[0])]);

  db::Region region;

  region.insert (polygon);
  region.smooth (100, false);

  EXPECT_EQ (region.sized (100).to_string (), "(-22037,-43939;-22054,-43930;-37952,16664;-20532,35323;30479,41734;38432,28957;-7745,-38003)");

  region = db::Region ();
  region.insert (polygon);
  region = region.smoothed (100, false);

  EXPECT_EQ (region.sized (100).to_string (), "(-22037,-43939;-22054,-43930;-37952,16664;-20532,35323;30479,41734;38432,28957;-7745,-38003)");

  db::RegionAreaFilter rf (0, 10000000000, false);

  region = db::Region ();
  region.insert (polygon);
  region.filter (rf);

  EXPECT_EQ (region.sized (100).to_string (), "(-22037,-43939;-22054,-43930;-37952,16664;-20532,35323;30479,41734;38432,28957;-7745,-38003)");

  region = db::Region ();
  region.insert (polygon);
  region = region.filtered (rf);

  EXPECT_EQ (region.sized (100).to_string (), "(-22037,-43939;-22054,-43930;-37952,16664;-20532,35323;30479,41734;38432,28957;-7745,-38003)");
}

