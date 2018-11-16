
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
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/deep_region_au1.gds");
}

