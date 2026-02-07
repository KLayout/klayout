
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

#include "dbLayoutUtils.h"
#include "dbLayerMapping.h"
#include "dbCellMapping.h"
#include "dbTestSupport.h"
#include "dbReader.h"
#include "dbLayoutDiff.h"
#include "dbPropertiesRepository.h"
#include "tlString.h"
#include "tlUnitTest.h"

unsigned int find_layer (const db::Layout &l, int ly, int dt)
{
  for (db::Layout::layer_iterator li = l.begin_layers (); li != l.end_layers (); ++li) {
    if ((*li).second->log_equal (db::LayerProperties (ly, dt))) {
      return (*li).first;
    }
  }
  tl_assert (false);
}

TEST(1)
{
  db::Layout l1;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l1);
  }

  db::Layout l2;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l2);
  }

  db::LayerMapping lm;
  lm.create (l2, l1);

  unsigned int li1 = find_layer (l1, 1, 0);
  unsigned int li2 = find_layer (l1, 2, 0);
  unsigned int li3 = find_layer (l1, 3, 0);

  EXPECT_EQ (lm.has_mapping (li1), true);
  EXPECT_EQ (lm.layer_mapping_pair (li1).first, true);
  EXPECT_EQ (l2.get_properties (lm.layer_mapping_pair (li1).second).to_string (), "1/0");
  EXPECT_EQ (lm.has_mapping (li2), true);
  EXPECT_EQ (lm.layer_mapping_pair (li2).first, true);
  EXPECT_EQ (l2.get_properties (lm.layer_mapping_pair (li2).second).to_string (), "2/0");
  EXPECT_EQ (lm.has_mapping (li3), false);
  EXPECT_EQ (lm.layer_mapping_pair (li3).first, false);

  lm.clear ();
  EXPECT_EQ (lm.has_mapping (li1), false);
  EXPECT_EQ (lm.has_mapping (li2), false);
  EXPECT_EQ (lm.has_mapping (li3), false);

  lm.create_full (l2, l1);

  EXPECT_EQ (lm.has_mapping (li1), true);
  EXPECT_EQ (lm.layer_mapping_pair (li1).first, true);
  EXPECT_EQ (l2.get_properties (lm.layer_mapping_pair (li1).second).to_string (), "1/0");
  EXPECT_EQ (lm.has_mapping (li2), true);
  EXPECT_EQ (lm.layer_mapping_pair (li2).first, true);
  EXPECT_EQ (l2.get_properties (lm.layer_mapping_pair (li2).second).to_string (), "2/0");
  EXPECT_EQ (lm.has_mapping (li3), true);
  EXPECT_EQ (lm.layer_mapping_pair (li3).first, true);
  EXPECT_EQ (l2.get_properties (lm.layer_mapping_pair (li3).second).to_string (), "3/0");
}

//  Tests merge_layout with no specific mapping (plain duplication of the tree)
TEST(2)
{
  db::Layout l1;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l1);
  }

  db::Layout l2;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l2);
  }

  unsigned int li1 = find_layer (l1, 1, 0);
  unsigned int li2 = find_layer (l1, 2, 0);
  unsigned int li3 = find_layer (l1, 3, 0);

  db::LayerMapping lm;
  lm.map (li1, l2.insert_layer (db::LayerProperties (11, 0)));
  lm.map (li2, l2.insert_layer (db::LayerProperties (12, 0)));
  lm.map (li3, l2.insert_layer (db::LayerProperties (13, 0)));

  db::CellMapping cm;
  std::map<db::cell_index_type, db::cell_index_type> fm;
  std::vector<db::cell_index_type> src;
  src.push_back (l1.cell_by_name ("TOP").second);
  db::merge_layouts (l2, l1, db::ICplxTrans (), src, cm.table (), lm.table (), &fm);

  CHECKPOINT();
  db::compare_layouts (_this, l2, tl::testdata () + "/algo/layout_utils_au2.gds");

  EXPECT_EQ (fm.find (l1.cell_by_name ("TOP").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("TOP").second)->second), "TOP$1");
  EXPECT_EQ (fm.find (l1.cell_by_name ("A").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("A").second)->second), "A$1");
  EXPECT_EQ (fm.find (l1.cell_by_name ("B").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("B").second)->second), "B$1");
  EXPECT_EQ (fm.find (l1.cell_by_name ("C").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("C").second)->second), "C$1");
}

//  Tests merge_layout with a single mapped cell (the others are mapped automatically)
TEST(3)
{
  db::Layout l1;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l1);
  }

  db::Layout l2;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l2);
  }

  unsigned int li1 = find_layer (l1, 1, 0);
  unsigned int li2 = find_layer (l1, 2, 0);
  unsigned int li3 = find_layer (l1, 3, 0);

  db::LayerMapping lm;
  lm.map (li1, l2.insert_layer (db::LayerProperties (11, 0)));
  lm.map (li2, l2.insert_layer (db::LayerProperties (12, 0)));
  lm.map (li3, l2.insert_layer (db::LayerProperties (13, 0)));

  db::CellMapping cm;
  std::map<db::cell_index_type, db::cell_index_type> fm;
  std::vector<db::cell_index_type> src;
  src.push_back (l1.cell_by_name ("TOP").second);
  cm.map (src.front (), l2.add_cell ("TOPTOP"));
  db::merge_layouts (l2, l1, db::ICplxTrans (), src, cm.table (), lm.table (), &fm);

  CHECKPOINT();
  db::compare_layouts (_this, l2, tl::testdata () + "/algo/layout_utils_au3.gds");

  EXPECT_EQ (fm.find (l1.cell_by_name ("TOP").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("TOP").second)->second), "TOPTOP");
  EXPECT_EQ (fm.find (l1.cell_by_name ("A").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("A").second)->second), "A$1");
  EXPECT_EQ (fm.find (l1.cell_by_name ("B").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("B").second)->second), "B$1");
  EXPECT_EQ (fm.find (l1.cell_by_name ("C").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("C").second)->second), "C$1");
}

//  Tests merge_layout with a mapped tree (by name)
TEST(4)
{
  db::Layout l1;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l1);
  }

  db::Layout l2;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l2);
  }

  unsigned int li1 = find_layer (l1, 1, 0);
  unsigned int li2 = find_layer (l1, 2, 0);
  unsigned int li3 = find_layer (l1, 3, 0);

  db::LayerMapping lm;
  lm.map (li1, l2.insert_layer (db::LayerProperties (11, 0)));
  lm.map (li2, l2.insert_layer (db::LayerProperties (12, 0)));
  lm.map (li3, l2.insert_layer (db::LayerProperties (13, 0)));

  db::CellMapping cm;
  std::vector<db::cell_index_type> src;
  src.push_back (l1.cell_by_name ("TOP").second);
  cm.create_from_names_full (l2, l2.cell_by_name ("TOP").second, l1, src.front ());
  std::map<db::cell_index_type, db::cell_index_type> fm;
  db::merge_layouts (l2, l1, db::ICplxTrans (), src, cm.table (), lm.table (), &fm);

  CHECKPOINT();
  db::compare_layouts (_this, l2, tl::testdata () + "/algo/layout_utils_au4.gds");

  EXPECT_EQ (fm.find (l1.cell_by_name ("TOP").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("TOP").second)->second), "TOP");
  EXPECT_EQ (fm.find (l1.cell_by_name ("A").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("A").second)->second), "A");
  EXPECT_EQ (fm.find (l1.cell_by_name ("B").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("B").second)->second), "B");
  EXPECT_EQ (fm.find (l1.cell_by_name ("C").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("C").second)->second), "C");
}

//  Tests merge_layout with a equivalence-mapped tree
TEST(5)
{
  db::Layout l1;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l1);
  }

  db::Layout l2;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l2);
  }

  unsigned int li1 = find_layer (l1, 1, 0);
  unsigned int li2 = find_layer (l1, 2, 0);
  unsigned int li3 = find_layer (l1, 3, 0);

  db::LayerMapping lm;
  lm.map (li1, l2.insert_layer (db::LayerProperties (11, 0)));
  lm.map (li2, l2.insert_layer (db::LayerProperties (12, 0)));
  lm.map (li3, l2.insert_layer (db::LayerProperties (13, 0)));

  db::CellMapping cm;
  std::vector<db::cell_index_type> src;
  src.push_back (l1.cell_by_name ("TOP").second);
  cm.create_from_geometry_full (l2, l2.cell_by_name ("TOP").second, l1, src.front ());
  std::map<db::cell_index_type, db::cell_index_type> fm;
  db::merge_layouts (l2, l1, db::ICplxTrans (), src, cm.table (), lm.table (), &fm);

  CHECKPOINT();
  db::compare_layouts (_this, l2, tl::testdata () + "/algo/layout_utils_au5.gds");

  EXPECT_EQ (fm.find (l1.cell_by_name ("TOP").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("TOP").second)->second), "TOP");
  EXPECT_EQ (fm.find (l1.cell_by_name ("A").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("A").second)->second), "A");
  EXPECT_EQ (fm.find (l1.cell_by_name ("B").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("B").second)->second), "B");
  EXPECT_EQ (fm.find (l1.cell_by_name ("C").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("C").second)->second), "C$1");
}

//  Tests merge_layout with dropping of cell B
TEST(6)
{
  db::Layout l1;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l1);
  }

  db::Layout l2;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l2);
  }

  unsigned int li1 = find_layer (l1, 1, 0);
  unsigned int li2 = find_layer (l1, 2, 0);
  unsigned int li3 = find_layer (l1, 3, 0);

  db::LayerMapping lm;
  lm.map (li1, l2.insert_layer (db::LayerProperties (11, 0)));
  lm.map (li2, l2.insert_layer (db::LayerProperties (12, 0)));
  lm.map (li3, l2.insert_layer (db::LayerProperties (13, 0)));

  db::CellMapping cm;
  //  Drop cell B
  cm.map (l1.cell_by_name ("B").second, db::DropCell);
  cm.map (l1.cell_by_name ("TOP").second, l2.cell_by_name ("TOP").second);

  std::map<db::cell_index_type, db::cell_index_type> fm;
  std::vector<db::cell_index_type> src;
  src.push_back (l1.cell_by_name ("TOP").second);
  db::merge_layouts (l2, l1, db::ICplxTrans (), src, cm.table (), lm.table (), &fm);

  CHECKPOINT();
  db::compare_layouts (_this, l2, tl::testdata () + "/algo/layout_utils_au6.gds");

  EXPECT_EQ (fm.find (l1.cell_by_name ("TOP").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("TOP").second)->second), "TOP");
  EXPECT_EQ (fm.find (l1.cell_by_name ("A").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("A").second)->second), "A$1");
  EXPECT_EQ (fm.find (l1.cell_by_name ("B").second) != fm.end (), false);
  EXPECT_EQ (fm.find (l1.cell_by_name ("C").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("C").second)->second), "C$1");
}

//  Tests merge_layout with transformation
TEST(7)
{
  db::Layout l1;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l1);
  }

  db::Layout l2;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l3.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l2);
  }

  unsigned int li1 = find_layer (l1, 1, 0);
  unsigned int li2 = find_layer (l1, 2, 0);
  unsigned int li3 = find_layer (l1, 3, 0);

  db::LayerMapping lm;
  lm.map (li1, l2.insert_layer (db::LayerProperties (11, 0)));
  lm.map (li2, l2.insert_layer (db::LayerProperties (12, 0)));
  lm.map (li3, l2.insert_layer (db::LayerProperties (13, 0)));

  db::Layout l2copy = l2;

  db::CellMapping cm;
  cm.map (l1.cell_by_name ("TOP").second, l2.cell_by_name ("TOP").second);

  std::map<db::cell_index_type, db::cell_index_type> fm;
  std::vector<db::cell_index_type> src;
  src.push_back (l1.cell_by_name ("TOP").second);
  db::merge_layouts (l2, l1, db::ICplxTrans (4.0), src, cm.table (), lm.table (), &fm);

  CHECKPOINT();
  db::compare_layouts (_this, l2, tl::testdata () + "/algo/layout_utils_au7.gds");

  EXPECT_EQ (fm.find (l1.cell_by_name ("TOP").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("TOP").second)->second), "TOP");
  EXPECT_EQ (fm.find (l1.cell_by_name ("A").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("A").second)->second), "A$1");
  EXPECT_EQ (fm.find (l1.cell_by_name ("B").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("B").second)->second), "B$1");
  EXPECT_EQ (fm.find (l1.cell_by_name ("C").second) != fm.end (), true);
  EXPECT_EQ (l2.cell_name (fm.find (l1.cell_by_name ("C").second)->second), "C");

  //  Once with final_mapping = 0 ...
  db::merge_layouts (l2copy, l1, db::ICplxTrans (4.0), src, cm.table (), lm.table ());

  CHECKPOINT();
  db::compare_layouts (_this, l2copy, tl::testdata () + "/algo/layout_utils_au7.gds");
}

//  Tests copy_shapes with no specific mapping (flattening)
TEST(12)
{
  db::Layout l1;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l1);
  }

  db::Layout l2;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l2);
  }

  unsigned int li1 = find_layer (l1, 1, 0);
  unsigned int li2 = find_layer (l1, 2, 0);
  unsigned int li3 = find_layer (l1, 3, 0);

  db::LayerMapping lm;
  lm.map (li1, l2.insert_layer (db::LayerProperties (11, 0)));
  lm.map (li2, l2.insert_layer (db::LayerProperties (12, 0)));
  lm.map (li3, l2.insert_layer (db::LayerProperties (13, 0)));

  db::CellMapping cm;
  std::map<db::cell_index_type, db::cell_index_type> fm;
  std::vector<db::cell_index_type> src;
  src.push_back (l1.cell_by_name ("TOP").second);
  cm.map (src.front (), l2.cell_by_name ("TOP").second);
  db::copy_shapes (l2, l1, db::ICplxTrans (), src, cm.table (), lm.table ());

  CHECKPOINT();
  db::compare_layouts (_this, l2, tl::testdata () + "/algo/layout_utils_au12.gds");
}

//  Tests copy_shapes with full name mapping
TEST(13)
{
  db::Layout l1;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l1);
  }

  db::Layout l2;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l2);
  }

  unsigned int li1 = find_layer (l1, 1, 0);
  unsigned int li2 = find_layer (l1, 2, 0);
  unsigned int li3 = find_layer (l1, 3, 0);

  db::LayerMapping lm;
  lm.map (li1, l2.insert_layer (db::LayerProperties (11, 0)));
  lm.map (li2, l2.insert_layer (db::LayerProperties (12, 0)));
  lm.map (li3, l2.insert_layer (db::LayerProperties (13, 0)));

  db::CellMapping cm;
  std::vector<db::cell_index_type> src;
  src.push_back (l1.cell_by_name ("TOP").second);
  cm.create_from_names_full (l2, l2.cell_by_name ("TOP").second, l1, src.front ());
  db::copy_shapes (l2, l1, db::ICplxTrans (), src, cm.table (), lm.table ());

  CHECKPOINT();
  db::compare_layouts (_this, l2, tl::testdata () + "/algo/layout_utils_au13.gds");
}

//  Tests copy_shapes with geo mapping
TEST(14)
{
  db::Layout l1;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l1);
  }

  db::Layout l2;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l2);
  }

  unsigned int li1 = find_layer (l1, 1, 0);
  unsigned int li2 = find_layer (l1, 2, 0);
  unsigned int li3 = find_layer (l1, 3, 0);

  db::LayerMapping lm;
  lm.map (li1, l2.insert_layer (db::LayerProperties (11, 0)));
  lm.map (li2, l2.insert_layer (db::LayerProperties (12, 0)));
  lm.map (li3, l2.insert_layer (db::LayerProperties (13, 0)));

  db::CellMapping cm;
  std::vector<db::cell_index_type> src;
  src.push_back (l1.cell_by_name ("TOP").second);
  cm.create_from_geometry_full (l2, l2.cell_by_name ("TOP").second, l1, src.front ());
  db::copy_shapes (l2, l1, db::ICplxTrans (), src, cm.table (), lm.table ());

  CHECKPOINT();
  db::compare_layouts (_this, l2, tl::testdata () + "/algo/layout_utils_au14.gds");
}

//  Tests copy_shapes with flattening minus one cell
TEST(15)
{
  db::Layout l1;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l1);
  }

  db::Layout l2;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l2);
  }

  unsigned int li1 = find_layer (l1, 1, 0);
  unsigned int li2 = find_layer (l1, 2, 0);
  unsigned int li3 = find_layer (l1, 3, 0);

  db::LayerMapping lm;
  lm.map (li1, l2.insert_layer (db::LayerProperties (11, 0)));
  lm.map (li2, l2.insert_layer (db::LayerProperties (12, 0)));
  lm.map (li3, l2.insert_layer (db::LayerProperties (13, 0)));

  db::CellMapping cm;
  std::vector<db::cell_index_type> src;
  src.push_back (l1.cell_by_name ("TOP").second);
  cm.map (src.front (), l2.cell_by_name ("TOP").second);
  cm.map (l1.cell_by_name ("B").second, db::DropCell);
  db::copy_shapes (l2, l1, db::ICplxTrans (), src, cm.table (), lm.table ());

  CHECKPOINT();
  db::compare_layouts (_this, l2, tl::testdata () + "/algo/layout_utils_au15.gds");
}

//  Tests copy_shapes/move_shapes with no specific mapping (flattening)
TEST(16)
{
  db::Layout l1;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l1);
  }

  db::Layout l2;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/layout_utils_l3.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l2);
  }

  unsigned int li1 = find_layer (l1, 1, 0);
  unsigned int li2 = find_layer (l1, 2, 0);
  unsigned int li3 = find_layer (l1, 3, 0);

  db::LayerMapping lm;
  lm.map (li1, l2.insert_layer (db::LayerProperties (11, 0)));
  lm.map (li2, l2.insert_layer (db::LayerProperties (12, 0)));
  lm.map (li3, l2.insert_layer (db::LayerProperties (13, 0)));

  db::Layout l2copy = l2;

  db::CellMapping cm;
  std::map<db::cell_index_type, db::cell_index_type> fm;
  std::vector<db::cell_index_type> src;
  src.push_back (l1.cell_by_name ("TOP").second);
  cm.map (src.front (), l2.cell_by_name ("TOP").second);
  db::copy_shapes (l2, l1, db::ICplxTrans (4.0), src, cm.table (), lm.table ());

  CHECKPOINT();
  db::compare_layouts (_this, l2, tl::testdata () + "/algo/layout_utils_au16.gds");

  //  ... and one test for move:
  db::move_shapes (l2copy, l1, db::ICplxTrans (4.0), src, cm.table (), lm.table ());

  CHECKPOINT();
  db::compare_layouts (_this, l2copy, tl::testdata () + "/algo/layout_utils_au16.gds");
  db::compare_layouts (_this, l1, tl::testdata () + "/algo/layout_utils_au16b.gds");

}

TEST(17_scale_and_snap)
{
  db::Layout l1;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/scale_and_snap.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l1);
  }

  db::scale_and_snap (l1, l1.cell (*l1.begin_top_down ()), 1, 20, 19);

  CHECKPOINT();
  db::compare_layouts (_this, l1, tl::testdata () + "/algo/layout_utils_au_sns1.gds");

  db::scale_and_snap (l1, l1.cell (*l1.begin_top_down ()), 1, 19, 20);

  CHECKPOINT();
  db::compare_layouts (_this, l1, tl::testdata () + "/algo/layout_utils_au_sns2.gds");
}

TEST(18_scale_and_snap)
{
  db::Layout l1;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/scale_and_snap.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l1);
  }

  db::scale_and_snap (l1, l1.cell (*l1.begin_top_down ()), 19, 1, 1);

  CHECKPOINT();
  db::compare_layouts (_this, l1, tl::testdata () + "/algo/layout_utils_au_sns3.gds");
}

TEST(19_scale_and_snap_basic)
{
  db::Layout l1;
  db::Layout l2;

  db::PropertiesSet ps1;
  ps1.insert (tl::Variant ("p"), tl::Variant (17));
  db::properties_id_type pid1 = db::properties_id (ps1);

  db::PropertiesSet ps2;
  ps2.insert (tl::Variant ("p"), tl::Variant (17));
  db::properties_id_type pid2 = db::properties_id (ps2);

  db::Cell &top1 = l1.cell (l1.add_cell ("TOP"));
  db::Cell &top2 = l2.cell (l2.add_cell ("TOP"));

  db::Cell &a1 = l1.cell (l1.add_cell ("A"));
  db::Cell &a2a = l2.cell (l2.add_cell ("A"));

  unsigned int layer1 = l1.insert_layer (db::LayerProperties (1, 0));
  unsigned int layer2 = l2.insert_layer (db::LayerProperties (1, 0));

  a1.shapes (layer1).insert (db::Box (0, 0, 100, 100));
  a2a.shapes (layer2).insert (db::Box (0, 0, 100, 100));

  top1.shapes (layer1).insert (db::Box (11, 21, 31, 41));
  top2.shapes (layer2).insert (db::Box (10, 20, 30, 40));

  top1.shapes (layer1).insert (db::BoxWithProperties (db::Box (11, 21, 31, 41), pid1));
  top2.shapes (layer2).insert (db::BoxWithProperties (db::Box (10, 20, 30, 40), pid2));

  top1.shapes (layer1).insert (db::Edge (11, 21, 31, 41));
  top2.shapes (layer2).insert (db::Edge (10, 20, 30, 40));

  top1.shapes (layer1).insert (db::EdgeWithProperties (db::Edge (11, 21, 31, 41), pid1));
  top2.shapes (layer2).insert (db::EdgeWithProperties (db::Edge (10, 20, 30, 40), pid2));

  top1.shapes (layer1).insert (db::EdgePair (db::Edge (11, 21, 31, 41), db::Edge (111, 121, 131, 141)));
  top2.shapes (layer2).insert (db::EdgePair (db::Edge (10, 20, 30, 40), db::Edge (110, 120, 130, 140)));

  top1.shapes (layer1).insert (db::EdgePairWithProperties (db::EdgePair (db::Edge (11, 21, 31, 41), db::Edge (111, 121, 131, 141)), pid1));
  top2.shapes (layer2).insert (db::EdgePairWithProperties (db::EdgePair (db::Edge (10, 20, 30, 40), db::Edge (110, 120, 130, 140)), pid2));

  top1.shapes (layer1).insert (db::Polygon (db::Box (11, 21, 31, 41)));
  top2.shapes (layer2).insert (db::Polygon (db::Box (10, 20, 30, 40)));

  top1.shapes (layer1).insert (db::PolygonWithProperties (db::Polygon (db::Box (11, 21, 31, 41)), pid1));
  top2.shapes (layer2).insert (db::PolygonWithProperties (db::Polygon (db::Box (10, 20, 30, 40)), pid2));

  db::Point pts1[] = {
    db::Point (1, 101),
    db::Point (101, 101),
    db::Point (101, 201)
  };

  db::Point pts2[] = {
    db::Point (0, 100),
    db::Point (100, 100),
    db::Point (100, 200)
  };

  top1.shapes (layer1).insert (db::Path (&pts1 [0], &pts1 [sizeof (pts1) / sizeof(pts1 [0])], 20));
  top2.shapes (layer2).insert (db::Path (&pts2 [0], &pts2 [sizeof (pts2) / sizeof(pts2 [0])], 20));

  top1.shapes (layer1).insert (db::PathWithProperties (db::Path (&pts1 [0], &pts1 [sizeof (pts1) / sizeof(pts1 [0])], 20), pid1));
  top2.shapes (layer2).insert (db::PathWithProperties (db::Path (&pts2 [0], &pts2 [sizeof (pts2) / sizeof(pts2 [0])], 20), pid2));

  top1.shapes (layer1).insert (db::Text ("t1", db::Trans (db::Vector (11, 21))));
  top2.shapes (layer2).insert (db::Text ("t1", db::Trans (db::Vector (10, 20))));

  top1.shapes (layer1).insert (db::TextWithProperties (db::Text ("t1", db::Trans (db::Vector (11, 21))), pid1));
  top2.shapes (layer2).insert (db::TextWithProperties (db::Text ("t1", db::Trans (db::Vector (10, 20))), pid2));

  top1.insert (db::CellInstArray (db::CellInst (a1.cell_index ()), db::Trans (db::Vector (11, 21))));
  top2.insert (db::CellInstArray (db::CellInst (a2a.cell_index ()), db::Trans (db::Vector (10, 20))));

  top1.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (a1.cell_index ()), db::Trans (db::Vector (11, 21))), pid1));
  top2.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (a2a.cell_index ()), db::Trans (db::Vector (10, 20))), pid2));

  top1.insert (db::CellInstArray (db::CellInst (a1.cell_index ()), db::Trans (db::Vector (11, 21)), db::Vector (0, 10), db::Vector (10, 0), 2, 3));
  top2.insert (db::CellInstArray (db::CellInst (a2a.cell_index ()), db::Trans (db::Vector (10, 20)), db::Vector (0, 10), db::Vector (10, 0), 2, 3));

  top1.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (a1.cell_index ()), db::Trans (db::Vector (11, 21)), db::Vector (0, 10), db::Vector (10, 0), 2, 3), pid1));
  top2.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (a2a.cell_index ()), db::Trans (db::Vector (10, 20)), db::Vector (0, 10), db::Vector (10, 0), 2, 3), pid2));

  std::vector<db::Vector> ia;
  ia.push_back (db::Vector (0, 0));
  ia.push_back (db::Vector (10, 0));
  ia.push_back (db::Vector (0, 10));

  top1.insert (db::CellInstArray (db::CellInst (a1.cell_index ()), db::Trans (db::Vector (11, 21)), ia.begin (), ia.end ()));
  top2.insert (db::CellInstArray (db::CellInst (a2a.cell_index ()), db::Trans (db::Vector (10, 20)), ia.begin (), ia.end ()));

  top1.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (a1.cell_index ()), db::Trans (db::Vector (11, 21)), ia.begin (), ia.end ()), pid1));
  top2.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (a2a.cell_index ()), db::Trans (db::Vector (10, 20)), ia.begin (), ia.end ()), pid2));

  db::scale_and_snap (l1, top1, 10, 1, 1);

  bool equal = db::compare_layouts (l1, l2,
                                     db::layout_diff::f_verbose
                                     | db::layout_diff::f_boxes_as_polygons
                                     | db::layout_diff::f_paths_as_polygons
                                   , 0, 100 /*max diff lines*/);
  EXPECT_EQ (equal, true);
}

TEST(20_scale_and_snap)
{
  db::Layout l1;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/scale_and_snap4.oas";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l1);
  }

  db::scale_and_snap (l1, l1.cell (*l1.begin_top_down ()), 10, 95, 100);

  CHECKPOINT();
  db::compare_layouts (_this, l1, tl::testdata () + "/algo/layout_utils_au_sns4.oas", db::NormalizationMode (db::WriteOAS + db::WithArrays));
}


TEST(21_break1)
{
  db::Layout l1;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/break_polygons_test.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l1);
  }

  db::break_polygons (l1, 10, 3.0);

  CHECKPOINT();
  db::compare_layouts (_this, l1, tl::testdata () + "/algo/layout_utils_au_bp1.gds");
}

TEST(22_break2)
{
  db::Layout l1;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/break_polygons_test.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l1);
  }

  unsigned int li1 = find_layer (l1, 1, 0);
  unsigned int li2 = find_layer (l1, 2, 0);

  db::break_polygons (l1, li1, 10, 0.0);
  db::break_polygons (l1, li2, 0, 3.0);

  CHECKPOINT();
  db::compare_layouts (_this, l1, tl::testdata () + "/algo/layout_utils_au_bp2.gds");
}

