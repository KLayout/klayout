
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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
#include "dbTexts.h"
#include "dbDeepShapeStore.h"
#include "dbRegion.h"
#include "dbEdges.h"
#include "dbTextsUtils.h"
#include "tlUnitTest.h"
#include "tlStream.h"

TEST(1_Basics)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_texts_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l3 = ly.get_layer (db::LayerProperties (3, 0));
  unsigned int l100 = ly.get_layer (db::LayerProperties (100, 0));

  db::Texts texts2 (db::RecursiveShapeIterator (ly, top_cell, l2), dss);
  db::Texts texts3 (db::RecursiveShapeIterator (ly, top_cell, l3), dss);
  db::Texts texts100 (db::RecursiveShapeIterator (ly, top_cell, l100), dss);

  EXPECT_EQ (texts100.empty (), true);
  EXPECT_EQ (texts2.empty (), false);
  EXPECT_EQ (texts2.bbox ().to_string (), "(-520,0;24040,2800)");
  EXPECT_EQ (texts2.count (), size_t (40));
  EXPECT_EQ (texts2.hier_count (), size_t (1));
  EXPECT_EQ (texts2.to_string ().substr (0, 42), "('L2',r0 -520,0);('L2',r0 -520,2800);('L2'");

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  db::Region polygons;
  texts2.polygons (polygons);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), polygons);

  polygons.clear ();
  texts3.polygons (polygons);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), polygons);

  db::Edges edges;
  texts2.edges (edges);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), edges);

  //  NOTE: insert texts2 as layer 14/0 from a copy - this tests the ability to copy-construct an TC
  db::Texts texts2_copy (texts2);
  texts2_copy.insert_into_as_polygons (&target, target_top_cell_index, target.get_layer (db::LayerProperties (14, 0)), 1);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/deep_texts_au1.gds");
}

TEST(2_Interactions)
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

  db::DeepShapeStore dss;

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  unsigned int l8 = ly.get_layer (db::LayerProperties (8, 0));

  db::Texts texts2 (db::RecursiveShapeIterator (ly, top_cell, l2), dss);
  db::Region polygons8 (db::RecursiveShapeIterator (ly, top_cell, l8), dss);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  db::Region polygons;
  texts2.selected_interacting (polygons8).polygons (polygons);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), polygons);

  polygons.clear ();
  texts2.selected_not_interacting (polygons8).polygons (polygons);
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), polygons);

  {
    db::Texts texts2_copy = texts2;
    texts2_copy.select_interacting (polygons8);
    polygons.clear ();
    texts2_copy.polygons (polygons);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), polygons);
  }

  {
    db::Texts texts2_copy = texts2;
    texts2_copy.select_not_interacting (polygons8);
    polygons.clear ();
    texts2_copy.polygons (polygons);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), polygons);
  }

  {
    db::Texts texts2_copy = texts2;
    db::Region polygons;
    texts2_copy.pull_interacting (polygons, polygons8);
    target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (20, 0)), polygons);
  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/deep_texts_au2.gds");
}

TEST(3_Filtering)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_texts_l3.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::DeepShapeStore dss;

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));

  db::Texts texts2 (db::RecursiveShapeIterator (ly, top_cell, l2), dss);

  db::Layout target;
  unsigned int target_top_cell_index = target.add_cell (ly.cell_name (top_cell_index));

  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (10, 0)), texts2.filtered (db::TextStringFilter ("L2", false)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (11, 0)), texts2.filtered (db::TextStringFilter ("L2", true)));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (12, 0)), texts2.filtered (db::TextPatternFilter ("L*A", false)));

  texts2.filter (db::TextPatternFilter ("L*A", true));
  target.insert (target_top_cell_index, target.get_layer (db::LayerProperties (13, 0)), texts2);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/deep_texts_au3.gds");
}
