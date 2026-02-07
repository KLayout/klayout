
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


#include "dbRecursiveShapeIterator.h"
#include "dbRegion.h"
#include "dbLayoutDiff.h"
#include "tlString.h"
#include "tlUnitTest.h"
#include "tlFileUtils.h"
#include "tlStream.h"
#include "dbReader.h"
#include "dbWriter.h"

#include <vector>

std::string collect(db::RecursiveShapeIterator &s, const db::Layout &layout, bool with_layer = false) 
{
  std::string res;
  while (! s.at_end ()) {
    if (! res.empty ()) {
      res += "/";
    }
    if (s.cell ()) {
      res += std::string ("[") + layout.cell_name (s.cell ()->cell_index ()) + "]";
    } else {
      res += "[]";
    }
    if (s->is_box ()) {
      db::Box box;
      s->box (box);
      res += (s.trans () * box).to_string ();
    } else {
      res += "X";
    }
    if (with_layer) {
      res += "*";
      res += tl::to_string (s.layer ());
    }
    ++s;
  }
  return res;
}

std::string collect_with_copy(db::RecursiveShapeIterator s, const db::Layout &layout, bool with_layer = false)
{
  s.reset ();
  return collect (s, layout, with_layer);
}

TEST(1) 
{
  db::Manager m (true);
  db::Layout g (&m);
  g.insert_layer (0);
  g.insert_layer (1);
  g.insert_layer (2);

  db::Cell &c0 (g.cell (g.add_cell ()));

  db::RecursiveShapeIterator idef;
  EXPECT_EQ (idef.at_end (), true);
  EXPECT_EQ (collect (idef, g), "");
  EXPECT_EQ (collect_with_copy (idef, g), "");

  db::RecursiveShapeIterator i00 (g, c0, 0, db::Box (0, 0, 100, 100));
  EXPECT_EQ (collect (i00, g), "");
  EXPECT_EQ (collect_with_copy (i00, g), "");

  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));
  db::Cell &c3 (g.cell (g.add_cell ()));

  db::RecursiveShapeIterator i0 (g, c0, 0, db::Box (0, 0, 100, 100));
  EXPECT_EQ (collect (i0, g), "");
  EXPECT_EQ (collect_with_copy (i0, g), "");

  db::Box b (0, 100, 1000, 1200);
  c0.shapes (0).insert (b);
  c1.shapes (0).insert (b);
  c2.shapes (0).insert (b);
  c3.shapes (0).insert (b);

  c0.shapes (2).insert (b);
  c0.shapes (2).insert (b.moved (db::Vector (50, 50)));

  db::Trans tt;
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), tt));
  c0.insert (db::CellInstArray (db::CellInst (c2.cell_index ()), db::Trans (db::Vector (100, -100))));
  c0.insert (db::CellInstArray (db::CellInst (c3.cell_index ()), db::Trans (1)));
  c2.insert (db::CellInstArray (db::CellInst (c3.cell_index ()), db::Trans (db::Vector (1100, 0))));

  std::string x;

  db::RecursiveShapeIterator i0s (g, c0, 0);
  x = collect(i0s, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");
  EXPECT_EQ (i0s.bbox ().to_string (), "(-1200,0;2200,1200)");

  i0s.set_global_trans (db::ICplxTrans (2.0));
  x = collect(i0s, g);
  EXPECT_EQ (x, "[$1](0,200;2000,2400)/[$2](0,200;2000,2400)/[$3](200,0;2200,2200)/[$4](2400,0;4400,2200)/[$4](-2400,0;-200,2000)");
  EXPECT_EQ (i0s.bbox ().to_string (), "(-2400,0;4400,2400)");

  db::RecursiveShapeIterator i1 (g, c0, 0, db::Box (0, 0, 100, 100));
  x = collect(i1, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)");
  x = collect_with_copy(i1, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)");

  i1.set_global_trans (db::ICplxTrans (db::Trans (db::Vector (10, 20))));
  i1.set_region (db::Box (10, 20, 110, 120));
  x = collect(i1, g);
  EXPECT_EQ (x, "[$1](10,120;1010,1220)/[$2](10,120;1010,1220)/[$3](110,20;1110,1120)");
  x = collect_with_copy(i1, g);
  EXPECT_EQ (x, "[$1](10,120;1010,1220)/[$2](10,120;1010,1220)/[$3](110,20;1110,1120)");

  i1.reset ();
  x = collect(i1, g);
  EXPECT_EQ (x, "[$1](10,120;1010,1220)/[$2](10,120;1010,1220)/[$3](110,20;1110,1120)");
  x = collect_with_copy(i1, g);
  EXPECT_EQ (x, "[$1](10,120;1010,1220)/[$2](10,120;1010,1220)/[$3](110,20;1110,1120)");

  db::RecursiveShapeIterator i1_1inf (g, c0, 0, db::Box (0, 0, 100, 100));
  i1_1inf.min_depth(1);
  x = collect(i1_1inf, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)");
  x = collect_with_copy(i1_1inf, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)");

  db::RecursiveShapeIterator i1_11 (g, c0, 0, db::Box (0, 0, 100, 100));
  i1_11.min_depth(1);
  i1_11.max_depth(1);
  x = collect(i1_11, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)");
  x = collect_with_copy(i1_11, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)");

  db::RecursiveShapeIterator i1_12 (g, c0, 0, db::Box (0, 0, 100, 100));
  i1_12.min_depth(1);
  i1_12.max_depth(2);
  x = collect(i1_12, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)");
  x = collect_with_copy(i1_12, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)");

  db::RecursiveShapeIterator i1_22 (g, c0, 0, db::Box (0, 0, 100, 100));
  i1_22.min_depth(2);
  i1_22.max_depth(2);
  x = collect(i1_22, g);
  EXPECT_EQ (x, "");
  x = collect_with_copy(i1_22, g);
  EXPECT_EQ (x, "");

  db::RecursiveShapeIterator i1o (g, c0, 0, db::Box (0, 0, 100, 100), true);
  x = collect(i1o, g);
  EXPECT_EQ (x, "");
  x = collect_with_copy(i1o, g);
  EXPECT_EQ (x, "");
  i1o = db::RecursiveShapeIterator (g, c0, 0, db::Box (0, 0, 100, 101), true);
  x = collect(i1o, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)");
  x = collect_with_copy(i1o, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)");
  i1o = db::RecursiveShapeIterator (g, c0, 0, db::Box (0, 0, 101, 101), true);
  x = collect(i1o, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)");
  x = collect_with_copy(i1o, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)");

  db::RecursiveShapeIterator i2 (g, c0, 0, db::Box (-100, 0, 100, 100));
  db::RecursiveShapeIterator i2c = i2;
  x = collect(i2, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](-1200,0;-100,1000)");
  x = collect_with_copy(i2, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](-1200,0;-100,1000)");
  x = collect(i2c, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](-1200,0;-100,1000)");
  x = collect_with_copy(i2c, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](-1200,0;-100,1000)");
  db::RecursiveShapeIterator i2o (g, c0, 0, db::Box (-100, 0, 100, 100), true);
  x = collect(i2o, g);
  EXPECT_EQ (x, "");
  x = collect_with_copy(i2o, g);
  EXPECT_EQ (x, "");
  i2o = db::RecursiveShapeIterator (g, c0, 0, db::Box (-101, 0, 101, 101), true);
  x = collect(i2o, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](-1200,0;-100,1000)");
  x = collect_with_copy(i2o, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](-1200,0;-100,1000)");

  db::RecursiveShapeIterator i4 (g, c0, 0, db::Box (-100, 0, 2000, 100));
  db::RecursiveShapeIterator i4_copy (g, c0, 0, db::Box (-100, 0, 2000, 100));
  i4.max_depth (0);
  x = collect(i4, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)");
  x = collect_with_copy(i4, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)");

  EXPECT_EQ (i4 == i4, true);
  EXPECT_EQ (i4 != i4, false);
  EXPECT_EQ (i4 == i4_copy, false);
  EXPECT_EQ (i4 != i4_copy, true);
  i4 = i4_copy;
  EXPECT_EQ (i4 == i4_copy, true);
  EXPECT_EQ (i4 != i4_copy, false);
  i4.max_depth (1);
  x = collect(i4, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](-1200,0;-100,1000)");
  x = collect_with_copy(i4, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](-1200,0;-100,1000)");

  i4 = i4_copy;
  x = collect(i4, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");
  x = collect_with_copy(i4, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");

  db::RecursiveShapeIterator i5 (g, c0, 0, db::Box::world ());
  x = collect(i5, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");
  x = collect_with_copy(i5, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");

  std::set<db::cell_index_type> cc;
  db::RecursiveShapeIterator ii;

  ii = db::RecursiveShapeIterator (g, c0, 0, db::Box::world ());
  cc.clear ();
  cc.insert (c3.cell_index ());
  ii.unselect_all_cells ();
  ii.select_cells (cc);
  x = collect(ii, g);
  EXPECT_EQ (x, "[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");
  x = collect_with_copy(ii, g);
  EXPECT_EQ (x, "[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");
  ii.reset ();
  x = collect(ii, g);
  EXPECT_EQ (x, "[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");
  x = collect_with_copy(ii, g);
  EXPECT_EQ (x, "[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");

  ii.reset_selection ();
  x = collect(ii, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");
  x = collect_with_copy(ii, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");

  ii.reset_selection ();
  cc.clear ();
  cc.insert (c0.cell_index ());
  cc.insert (c2.cell_index ());
  ii.unselect_cells (cc);
  cc.clear ();
  cc.insert (c2.cell_index ());
  ii.select_cells (cc);
  x = collect(ii, g);
  EXPECT_EQ (x, "[$3](100,0;1100,1100)/[$4](1200,0;2200,1100)");
  x = collect_with_copy(ii, g);
  EXPECT_EQ (x, "[$3](100,0;1100,1100)/[$4](1200,0;2200,1100)");

  ii = db::RecursiveShapeIterator (g, c0, 0, db::Box::world ());
  ii.unselect_all_cells ();
  cc.clear ();
  cc.insert (c3.cell_index ());
  cc.insert (c0.cell_index ());
  ii.select_cells (cc);
  x = collect(ii, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");
  x = collect_with_copy(ii, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");

  ii = db::RecursiveShapeIterator (g, c0, 0, db::Box::world ());
  ii.unselect_all_cells ();
  cc.clear ();
  cc.insert (c0.cell_index ());
  cc.insert (c1.cell_index ());
  ii.select_cells (cc);
  x = collect(ii, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)");
  x = collect_with_copy(ii, g);
  EXPECT_EQ (x, "[$1](0,100;1000,1200)/[$2](0,100;1000,1200)");

  //  Shapes iterators

  ii = db::RecursiveShapeIterator (c0.shapes (0));
  x = collect(ii, g);
  EXPECT_EQ (x, "[](0,100;1000,1200)");
  x = collect_with_copy(ii, g);
  EXPECT_EQ (x, "[](0,100;1000,1200)");

  ii = db::RecursiveShapeIterator (c0.shapes (0), db::Box (0, 0, 10, 10));
  x = collect(ii, g);
  EXPECT_EQ (x, "");
  x = collect_with_copy(ii, g);
  EXPECT_EQ (x, "");

  ii.set_region (db::Box (0, 100, 0, 110));
  x = collect(ii, g);
  EXPECT_EQ (x, "[](0,100;1000,1200)");
  x = collect_with_copy(ii, g);
  EXPECT_EQ (x, "[](0,100;1000,1200)");

  ii = db::RecursiveShapeIterator (c0.shapes (1), db::Box::world ());
  x = collect(ii, g);
  EXPECT_EQ (x, "");
  x = collect_with_copy(ii, g);
  EXPECT_EQ (x, "");

  ii = db::RecursiveShapeIterator (c0.shapes (2), db::Box::world ());
  x = collect(ii, g);
  EXPECT_EQ (x, "[](0,100;1000,1200)/[](50,150;1050,1250)");
  x = collect_with_copy(ii, g);
  EXPECT_EQ (x, "[](0,100;1000,1200)/[](50,150;1050,1250)");

  ii = db::RecursiveShapeIterator (c0.shapes (2), db::Box (0, 0, 100, 100));
  x = collect(ii, g);
  EXPECT_EQ (x, "[](0,100;1000,1200)");
  x = collect_with_copy(ii, g);
  EXPECT_EQ (x, "[](0,100;1000,1200)");

  ii.set_overlapping (true);
  x = collect(ii, g);
  EXPECT_EQ (x, "");
  x = collect_with_copy(ii, g);
  EXPECT_EQ (x, "");

  ii.set_region (db::Box (0, 0, 101, 101));
  x = collect(ii, g);
  EXPECT_EQ (x, "[](0,100;1000,1200)");
  x = collect_with_copy(ii, g);
  EXPECT_EQ (x, "[](0,100;1000,1200)");
}

TEST(1a) 
{
  db::Manager m (true);
  db::Layout g (&m);
  g.insert_layer (0);
  g.insert_layer (1);
  db::Cell &c0 (g.cell (g.add_cell ()));
  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));
  db::Cell &c3 (g.cell (g.add_cell ()));

  db::Box b (0, 100, 1000, 1200);
  c1.shapes (0).insert (b);
  c2.shapes (0).insert (b);
  c3.shapes (0).insert (b);

  db::Box bb (1, 101, 1001, 1201);
  c2.shapes (1).insert (bb);

  db::Trans tt;
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), tt));
  c0.insert (db::CellInstArray (db::CellInst (c2.cell_index ()), db::Trans (db::Vector (100, -100))));
  c0.insert (db::CellInstArray (db::CellInst (c3.cell_index ()), db::Trans (1)));
  c2.insert (db::CellInstArray (db::CellInst (c3.cell_index ()), db::Trans (db::Vector (1100, 0))));

  std::string x;

  db::RecursiveShapeIterator i0 (g, c0, 0, db::Box ());
  x = collect_with_copy(i0, g);
  EXPECT_EQ (x, "");
  x = collect(i0, g);
  EXPECT_EQ (x, "");

  db::RecursiveShapeIterator i1 (g, c0, 0, db::Box (0, 0, 100, 100));
  x = collect_with_copy(i1, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)");
  x = collect(i1, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)");

  db::RecursiveShapeIterator i1o (g, c0, 0, db::Box (0, 0, 100, 100), true);
  x = collect_with_copy(i1o, g);
  EXPECT_EQ (x, "");
  x = collect(i1o, g);
  EXPECT_EQ (x, "");
  i1o = db::RecursiveShapeIterator (g, c0, 0, db::Box (0, 0, 100, 101), true);
  x = collect_with_copy(i1o, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)");
  x = collect(i1o, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)");
  i1o = db::RecursiveShapeIterator (g, c0, 0, db::Box (0, 0, 101, 101), true);
  x = collect_with_copy(i1o, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)");
  x = collect(i1o, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)");

  db::RecursiveShapeIterator i2 (g, c0, 0, db::Box (-100, 0, 100, 100));
  x = collect_with_copy(i2, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](-1200,0;-100,1000)");
  x = collect(i2, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](-1200,0;-100,1000)");
  db::RecursiveShapeIterator i2o (g, c0, 0, db::Box (-100, 0, 100, 100), true);
  x = collect_with_copy(i2o, g);
  EXPECT_EQ (x, "");
  x = collect(i2o, g);
  EXPECT_EQ (x, "");
  i2o = db::RecursiveShapeIterator (g, c0, 0, db::Box (-101, 0, 101, 101), true);
  x = collect_with_copy(i2o, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](-1200,0;-100,1000)");
  x = collect(i2o, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](-1200,0;-100,1000)");

  db::RecursiveShapeIterator i4 (g, c0, 0, db::Box (-100, 0, 2000, 100));
  db::RecursiveShapeIterator i4_copy (g, c0, 0, db::Box (-100, 0, 2000, 100));
  i4.max_depth (0);
  x = collect_with_copy(i4, g);
  EXPECT_EQ (x, "");
  x = collect(i4, g);
  EXPECT_EQ (x, "");

  EXPECT_EQ (i4 == i4, true);
  EXPECT_EQ (i4 != i4, false);
  EXPECT_EQ (i4 == i4_copy, false);
  EXPECT_EQ (i4 != i4_copy, true);
  i4 = i4_copy;
  EXPECT_EQ (i4 == i4_copy, true);
  EXPECT_EQ (i4 != i4_copy, false);
  i4.max_depth (1);
  x = collect_with_copy(i4, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](-1200,0;-100,1000)");
  x = collect(i4, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](-1200,0;-100,1000)");

  i4 = i4_copy;
  x = collect_with_copy(i4, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");
  x = collect(i4, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");

  db::RecursiveShapeIterator i5 (g, c0, 0, db::Box::world ());
  x = collect_with_copy(i5, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");
  x = collect(i5, g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");

  i5.set_layer (1);
  x = collect_with_copy(i5, g);
  EXPECT_EQ (x, "[$3](101,1;1101,1101)");
  x = collect(i5, g);
  EXPECT_EQ (x, "[$3](101,1;1101,1101)");

  std::set<unsigned int> ll;

  db::RecursiveShapeIterator i5a (g, c0, ll, db::Box::world ());
  x = collect_with_copy(i5a, g, true);
  EXPECT_EQ (x, "");
  x = collect(i5a, g, true);
  EXPECT_EQ (x, "");

  ll.insert (0);
  db::RecursiveShapeIterator i5b (g, c0, ll, db::Box::world ());
  x = collect_with_copy(i5b, g, true);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)*0/[$3](100,0;1100,1100)*0/[$4](1200,0;2200,1100)*0/[$4](-1200,0;-100,1000)*0");
  x = collect(i5b, g, true);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)*0/[$3](100,0;1100,1100)*0/[$4](1200,0;2200,1100)*0/[$4](-1200,0;-100,1000)*0");

  ll.insert (1);
  db::RecursiveShapeIterator i5c (g, c0, ll, db::Box::world ());
  db::RecursiveShapeIterator i5cc = i5c;
  x = collect_with_copy(i5c, g, true);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)*0/[$3](100,0;1100,1100)*0/[$3](101,1;1101,1101)*1/[$4](1200,0;2200,1100)*0/[$4](-1200,0;-100,1000)*0");
  x = collect(i5c, g, true);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)*0/[$3](100,0;1100,1100)*0/[$3](101,1;1101,1101)*1/[$4](1200,0;2200,1100)*0/[$4](-1200,0;-100,1000)*0");
  x = collect_with_copy(i5cc, g, true);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)*0/[$3](100,0;1100,1100)*0/[$3](101,1;1101,1101)*1/[$4](1200,0;2200,1100)*0/[$4](-1200,0;-100,1000)*0");
  x = collect(i5cc, g, true);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)*0/[$3](100,0;1100,1100)*0/[$3](101,1;1101,1101)*1/[$4](1200,0;2200,1100)*0/[$4](-1200,0;-100,1000)*0");

  std::vector<unsigned int> ll_new;
  ll_new.push_back (0);
  i5c.set_layers (ll_new);
  x = collect_with_copy(i5c, g, true);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)*0/[$3](100,0;1100,1100)*0/[$4](1200,0;2200,1100)*0/[$4](-1200,0;-100,1000)*0");
  x = collect(i5c, g, true);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)*0/[$3](100,0;1100,1100)*0/[$4](1200,0;2200,1100)*0/[$4](-1200,0;-100,1000)*0");
}

TEST(1b) 
{
  db::Manager m (true);
  db::Layout g (&m);
  g.insert_layer (0);
  db::Cell &c0 (g.cell (g.add_cell ()));
  db::Cell &c1 (g.cell (g.add_cell ()));

  db::Box b;
  b = db::Box (0, 0, 2000, 2000000);
  c1.shapes (0).insert (b);
  b = db::Box (1998000, 0, 2000000, 2000000);
  c1.shapes (0).insert (b);
  b = db::Box (0, 0, 2000000, 2000);
  c1.shapes (0).insert (b);
  b = db::Box (0, 1998000, 2000000, 2000000);
  c1.shapes (0).insert (b);

  db::Trans tt;
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), tt));
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), tt));

  db::RecursiveShapeIterator i (g, c0, 0, db::Box (1000000, 1000000, 10001000, 10001000));
  std::string x;
  x = collect_with_copy(i, g);
  EXPECT_EQ (x, "[$2](1998000,0;2000000,2000000)/[$2](0,1998000;2000000,2000000)/[$2](1998000,0;2000000,2000000)/[$2](0,1998000;2000000,2000000)");
  x = collect(i, g);
  EXPECT_EQ (x, "[$2](1998000,0;2000000,2000000)/[$2](0,1998000;2000000,2000000)/[$2](1998000,0;2000000,2000000)/[$2](0,1998000;2000000,2000000)");

  db::RecursiveShapeIterator i2 (g, c0, 0, db::Box (1000000, 1000000, 1001000, 1001000));
  x = collect_with_copy(i2, g);
  EXPECT_EQ (x, "");
  x = collect(i2, g);
  EXPECT_EQ (x, "");
}

TEST(2) 
{
  db::Manager m (true);
  db::Layout g (&m);
  g.insert_layer(0);

  db::Cell &c0 (g.cell (g.add_cell ()));
  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));

  db::Box b (1000, -500, 2000, 500);
  c2.shapes (0).insert (b);

  db::Trans tt;
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), tt, db::Vector (0, 6000), db::Vector (6000, 0), 2, 2));
  c1.insert (db::CellInstArray (db::CellInst (c2.cell_index ()), tt, db::Vector (0, 2000), db::Vector (3000, 1000), 2, 2));

  std::string x;

  db::RecursiveShapeIterator i0 (g, c0, 0, db::Box ());
  x = collect(i0, g);
  EXPECT_EQ (x, "");

  db::RecursiveShapeIterator i (g, c0, 0, db::Box::world ());
  x = collect_with_copy(i, g);
  EXPECT_EQ (x, "[$3](1000,-500;2000,500)/[$3](1000,1500;2000,2500)/[$3](4000,500;5000,1500)/[$3](4000,2500;5000,3500)/[$3](1000,5500;2000,6500)/[$3](1000,7500;2000,8500)/[$3](4000,6500;5000,7500)/[$3](4000,8500;5000,9500)/[$3](7000,-500;8000,500)/[$3](7000,1500;8000,2500)/[$3](10000,500;11000,1500)/[$3](10000,2500;11000,3500)/[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)/[$3](10000,6500;11000,7500)/[$3](10000,8500;11000,9500)");
  x = collect(i, g);
  EXPECT_EQ (x, "[$3](1000,-500;2000,500)/[$3](1000,1500;2000,2500)/[$3](4000,500;5000,1500)/[$3](4000,2500;5000,3500)/[$3](1000,5500;2000,6500)/[$3](1000,7500;2000,8500)/[$3](4000,6500;5000,7500)/[$3](4000,8500;5000,9500)/[$3](7000,-500;8000,500)/[$3](7000,1500;8000,2500)/[$3](10000,500;11000,1500)/[$3](10000,2500;11000,3500)/[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)/[$3](10000,6500;11000,7500)/[$3](10000,8500;11000,9500)");

  db::RecursiveShapeIterator i2 (g, c0, 0, db::Box (3400, 3450, 5600, 6500));
  x = collect_with_copy(i2, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)/[$3](4000,6500;5000,7500)");
  x = collect(i2, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)/[$3](4000,6500;5000,7500)");

  db::RecursiveShapeIterator i3 (g, c0, 0, db::Box (6650, 5300, 10000, 7850));
  x = collect_with_copy(i3, g);
  EXPECT_EQ (x, "[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)/[$3](10000,6500;11000,7500)");
  x = collect(i3, g);
  EXPECT_EQ (x, "[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)/[$3](10000,6500;11000,7500)");

  db::RecursiveShapeIterator i2o (g, c0, 0, db::Box (3400, 3450, 5600, 6500), true);
  x = collect_with_copy(i2o, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)");
  x = collect(i2o, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)");

  db::RecursiveShapeIterator i3o (g, c0, 0, db::Box (6650, 5300, 10000, 7850), true);
  x = collect_with_copy(i3o, g);
  EXPECT_EQ (x, "[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)");
  x = collect(i3o, g);
  EXPECT_EQ (x, "[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)");
}

TEST(3)
{
  db::Manager m (true);
  db::Layout g (&m);
  g.insert_layer(0);

  db::Cell &c0 (g.cell (g.add_cell ()));
  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));

  db::Box b (1000, -500, 2000, 500);
  c2.shapes (0).insert (b);

  db::Trans tt;
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), tt, db::Vector (0, 6000), db::Vector (6000, 0), 2, 2));
  c1.insert (db::CellInstArray (db::CellInst (c2.cell_index ()), tt, db::Vector (0, 2000), db::Vector (3000, 1000), 2, 2));

  std::string x;

  db::RecursiveShapeIterator i (g, c0, 0, db::Box::world ());
  x = collect_with_copy(i, g);
  EXPECT_EQ (x, "[$3](1000,-500;2000,500)/[$3](1000,1500;2000,2500)/[$3](4000,500;5000,1500)/[$3](4000,2500;5000,3500)/[$3](1000,5500;2000,6500)/[$3](1000,7500;2000,8500)/[$3](4000,6500;5000,7500)/[$3](4000,8500;5000,9500)/[$3](7000,-500;8000,500)/[$3](7000,1500;8000,2500)/[$3](10000,500;11000,1500)/[$3](10000,2500;11000,3500)/[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)/[$3](10000,6500;11000,7500)/[$3](10000,8500;11000,9500)");
  x = collect(i, g);
  EXPECT_EQ (x, "[$3](1000,-500;2000,500)/[$3](1000,1500;2000,2500)/[$3](4000,500;5000,1500)/[$3](4000,2500;5000,3500)/[$3](1000,5500;2000,6500)/[$3](1000,7500;2000,8500)/[$3](4000,6500;5000,7500)/[$3](4000,8500;5000,9500)/[$3](7000,-500;8000,500)/[$3](7000,1500;8000,2500)/[$3](10000,500;11000,1500)/[$3](10000,2500;11000,3500)/[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)/[$3](10000,6500;11000,7500)/[$3](10000,8500;11000,9500)");

  db::RecursiveShapeIterator i2 (g, c0, 0, db::Region (db::Box (3400, 3450, 5600, 6500)));
  EXPECT_EQ (i2.has_complex_region (), false);
  EXPECT_EQ (i2.region ().to_string (), "(3400,3450;5600,6500)");
  x = collect_with_copy(i2, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)/[$3](4000,6500;5000,7500)");
  x = collect(i2, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)/[$3](4000,6500;5000,7500)");

  db::RecursiveShapeIterator i3 (g, c0, 0, db::Region (db::Box (6650, 5300, 10000, 7850)));
  x = collect_with_copy(i3, g);
  EXPECT_EQ (x, "[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)/[$3](10000,6500;11000,7500)");
  x = collect(i3, g);
  EXPECT_EQ (x, "[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)/[$3](10000,6500;11000,7500)");

  db::Region rr;
  rr.insert (db::Box (3400, 3450, 5600, 6500));
  rr.insert (db::Box (6650, 5300, 10000, 7850));

  db::RecursiveShapeIterator i23 (g, c0, 0, rr);
  x = collect_with_copy(i23, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)/[$3](4000,6500;5000,7500)/[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)/[$3](10000,6500;11000,7500)");
  x = collect(i23, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)/[$3](4000,6500;5000,7500)/[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)/[$3](10000,6500;11000,7500)");

  db::RecursiveShapeIterator i2o (g, c0, 0, db::Region (db::Box (3400, 3450, 5600, 6500)), true);
  x = collect_with_copy(i2o, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)");
  x = collect(i2o, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)");

  db::RecursiveShapeIterator i3o (g, c0, 0, db::Region (db::Box (6650, 5300, 10000, 7850)), true);
  x = collect_with_copy(i3o, g);
  EXPECT_EQ (x, "[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)");
  x = collect(i3o, g);
  EXPECT_EQ (x, "[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)");

  db::Region rro;
  rro.insert (db::Box (3400, 3450, 5600, 6500));
  rro.insert (db::Box (6650, 5300, 10000, 7850));

  db::RecursiveShapeIterator i23o (g, c0, 0, rro, true);
  EXPECT_EQ (i23o.has_complex_region (), true);
  EXPECT_EQ (i23o.complex_region ().to_string (), "(3400,3450;3400,6500;5600,6500;5600,3450);(6650,5300;6650,7850;10000,7850;10000,5300)");

  db::RecursiveShapeIterator i23ocopy = i23o;

  x = collect_with_copy(i23o, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)/[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)");
  x = collect(i23o, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)/[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)");

  x = collect_with_copy (i23ocopy, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)/[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)");
  x = collect (i23ocopy, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)/[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)");

  //  reset
  i23o.reset ();
  x = collect_with_copy (i23o, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)/[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)");
  x = collect (i23o, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)/[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)");

  //  copy constructor
  i23ocopy = i23o;
  i23ocopy.reset ();
  x = collect_with_copy (i23ocopy, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)/[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)");
  x = collect (i23ocopy, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)/[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)");

  //  setting of region

  db::Region rg;
  i23o.set_region (rg);
  x = collect_with_copy(i23o, g);
  EXPECT_EQ (x, "");
  x = collect(i23o, g);
  EXPECT_EQ (x, "");

  rg.insert (db::Box (3400, 3450, 5600, 6500));
  rg.insert (db::Box (16650, 5300, 20000, 7850));

  i23o.set_region (rg);
  x = collect_with_copy(i23o, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)");
  x = collect(i23o, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)");

  i23o.set_region (db::Box (6650, 5300, 10000, 7850));
  x = collect_with_copy(i23o, g);
  EXPECT_EQ (x, "[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)");
  x = collect(i23o, g);
  EXPECT_EQ (x, "[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)");

  //  region confinement

  i23o.confine_region (db::Box (3400, 3450, 5600, 6500));
  x = collect_with_copy(i23o, g);
  EXPECT_EQ (x, "");
  x = collect(i23o, g);
  EXPECT_EQ (x, "");

  i23o.set_region (rro);
  i23o.confine_region (db::Box (3400, 3450, 5600, 6500));
  x = collect_with_copy(i23o, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)");
  x = collect(i23o, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)");

  i23o.set_region (db::Box (3400, 3450, 5600, 6500));
  i23o.confine_region (rro);
  x = collect_with_copy(i23o, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)");
  x = collect(i23o, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)");

  i23o.set_region (rro);
  i23o.confine_region (rro);
  x = collect_with_copy(i23o, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)/[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)");
  x = collect(i23o, g);
  EXPECT_EQ (x, "[$3](4000,2500;5000,3500)/[$3](7000,5500;8000,6500)/[$3](7000,7500;8000,8500)");
}

static db::Layout boxes2layout (const std::set<db::Box> &boxes)
{
  db::Layout l;
  l.insert_layer(0, db::LayerProperties (1, 0));
  db::Cell &top (l.cell (l.add_cell ()));

  for (std::set<db::Box>::const_iterator b = boxes.begin (); b != boxes.end (); ++b) {
    top.shapes (0).insert (*b);
  }

  return l;
}

namespace {

  class FlatPusher
    : public db::RecursiveShapeReceiver
  {
  public:
    FlatPusher (std::set<db::Box> *boxes = 0) : mp_boxes (boxes ? boxes : &m_boxes) { }

    void shape (const db::RecursiveShapeIterator * /*iter*/, const db::Shape &shape, const db::ICplxTrans & /*always_apply*/, const db::ICplxTrans &trans, const db::Box & /*region*/, const box_tree_type * /*complex_region*/)
    {
      mp_boxes->insert (trans * shape.bbox ());
    }

    std::string to_string () const
    {
      std::vector<std::string> s;
      for (auto i = mp_boxes->begin (); i != mp_boxes->end (); ++i) {
        s.push_back (i->to_string ());
      }
      return tl::join (s.begin (), s.end (), ";");
    }

  private:
    std::set<db::Box> *mp_boxes;
    std::set<db::Box> m_boxes;
  };

}

TEST(4)
{
  //  Big fun

  db::Manager m (true);
  db::Layout g (&m);
  g.insert_layer(0);

  db::Cell &c0 (g.cell (g.add_cell ()));

  std::set<db::Box> boxes;

  for (int i = 0; i < 100000; ++i) {

    int x = rand () % 10000;
    int y = rand () % 10000;
    db::Box box (x, y, x + 10, y + 10);

    boxes.insert (box);

    c0.shapes (0).insert (box);

  }

  // ...

  db::Box search_box (2500, 2500, 7500, 7500);

  std::set<db::Box> selected_boxes;
  std::set<db::Box> selected_boxes2;

  for (db::RecursiveShapeIterator iter = db::RecursiveShapeIterator (g, c0, 0, search_box, true); !iter.at_end (); ++iter) {
    selected_boxes.insert (iter->bbox ());
  }

  for (std::set<db::Box>::const_iterator b = boxes.begin (); b != boxes.end (); ++b) {
    if (search_box.overlaps (*b)) {
      selected_boxes2.insert (*b);
    }
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);

  //  push mode
  {
    selected_boxes.clear ();
    FlatPusher pusher (&selected_boxes);
    db::RecursiveShapeIterator (g, c0, 0, search_box, true).push (&pusher);
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);

  //  with global trans

  selected_boxes.clear ();
  selected_boxes2.clear ();
  db::ICplxTrans ctr (db::Trans (db::Vector (10, 20)));

  {
    db::RecursiveShapeIterator iter (g, c0, 0, search_box, true);
    iter.set_global_trans (ctr);
    for ( ; !iter.at_end (); ++iter) {
      selected_boxes.insert (iter->bbox ().transformed (iter.trans ()));
    }
  }

  {
    for (std::set<db::Box>::const_iterator b = boxes.begin (); b != boxes.end (); ++b) {
      if (search_box.overlaps (b->transformed (ctr))) {
        selected_boxes2.insert (b->transformed (ctr));
      }
    }
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);

  //  push mode
  {
    selected_boxes.clear ();
    FlatPusher pusher (&selected_boxes);
    db::RecursiveShapeIterator iter (g, c0, 0, search_box, true);
    iter.set_global_trans (ctr);
    iter.push (&pusher);
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);

  // ...

  db::Box search_box2 (500, 500, 1000, 1000);

  selected_boxes.clear ();
  selected_boxes2.clear ();

  db::Region reg;
  reg.insert (search_box);
  reg.insert (search_box2);

  for (db::RecursiveShapeIterator iter = db::RecursiveShapeIterator (g, c0, 0, reg, true); !iter.at_end (); ++iter) {
    selected_boxes.insert (iter->bbox ());
  }

  for (std::set<db::Box>::const_iterator b = boxes.begin (); b != boxes.end (); ++b) {
    if (search_box.overlaps (*b) || search_box2.overlaps (*b)) {
      selected_boxes2.insert (*b);
    }
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);

  //  push mode
  {
    selected_boxes.clear ();
    FlatPusher pusher (&selected_boxes);
    db::RecursiveShapeIterator (g, c0, 0, reg, true).push (&pusher);
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);
}

TEST(5)
{
  //  Big fun with cells

  db::Manager m (true);
  db::Layout g (&m);
  g.insert_layer(0);

  db::Cell &c0 (g.cell (g.add_cell ()));
  db::Cell &c1 (g.cell (g.add_cell ()));

  db::Box basic_box (0, 0, 10, 10);
  c1.shapes (0).insert (basic_box);

  std::set<db::Box> boxes;

  for (int i = 0; i < 100000; ++i) {

    int x = rand () % 10000;
    int y = rand () % 10000;

    boxes.insert (basic_box.moved (db::Vector (x, y)));

    c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), db::Trans (db::Vector (x, y))));

  }

  db::Box search_box (2500, 2500, 7500, 7500);

  std::set<db::Box> selected_boxes;
  std::set<db::Box> selected_boxes2;

  for (db::RecursiveShapeIterator iter = db::RecursiveShapeIterator (g, c0, 0, search_box, true); !iter.at_end (); ++iter) {
    selected_boxes.insert (iter.trans () * iter->bbox ());
  }

  for (std::set<db::Box>::const_iterator b = boxes.begin (); b != boxes.end (); ++b) {
    if (search_box.overlaps (*b)) {
      selected_boxes2.insert (*b);
    }
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);

  //  push mode
  {
    selected_boxes.clear ();
    FlatPusher pusher (&selected_boxes);
    db::RecursiveShapeIterator (g, c0, 0, search_box, true).push (&pusher);
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);

  selected_boxes.clear ();
  selected_boxes2.clear ();

  db::ICplxTrans ctr (db::Trans (db::Vector (10, 20)));

  {
    db::RecursiveShapeIterator iter = db::RecursiveShapeIterator (g, c0, 0, search_box, true);
    iter.set_global_trans (ctr);
    for ( ; !iter.at_end (); ++iter) {
      selected_boxes.insert (iter.trans () * iter->bbox ());
    }
  }

  for (std::set<db::Box>::const_iterator b = boxes.begin (); b != boxes.end (); ++b) {
    if (search_box.overlaps (b->transformed (ctr))) {
      selected_boxes2.insert (b->transformed (ctr));
    }
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);

  //  push mode
  {
    selected_boxes.clear ();
    FlatPusher pusher (&selected_boxes);
    db::RecursiveShapeIterator iter (g, c0, 0, search_box, true);
    iter.set_global_trans (ctr);
    iter.push (&pusher);
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);

  db::Box search_box2 (500, 500, 1000, 1000);

  selected_boxes.clear ();
  selected_boxes2.clear ();

  db::Region reg;
  reg.insert (search_box);
  reg.insert (search_box2);

  for (db::RecursiveShapeIterator iter = db::RecursiveShapeIterator (g, c0, 0, reg, true); !iter.at_end (); ++iter) {
    selected_boxes.insert (iter.trans () * iter->bbox ());
  }

  for (std::set<db::Box>::const_iterator b = boxes.begin (); b != boxes.end (); ++b) {
    if (search_box.overlaps (*b) || search_box2.overlaps (*b)) {
      selected_boxes2.insert (*b);
    }
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);

  //  push mode
  {
    selected_boxes.clear ();
    FlatPusher pusher (&selected_boxes);
    db::RecursiveShapeIterator (g, c0, 0, reg, true).push (&pusher);
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);
}

class LoggingReceiver
  : public db::RecursiveShapeReceiver
{
public:
  LoggingReceiver () { }

  const std::string &text () const { return m_text; }

  virtual void begin (const db::RecursiveShapeIterator * /*iter*/) { m_text += "begin\n"; }
  virtual void end (const db::RecursiveShapeIterator * /*iter*/) { m_text += "end\n"; }

  virtual void enter_cell (const db::RecursiveShapeIterator *iter, const db::Cell *cell, const db::Box & /*region*/, const box_tree_type * /*complex_region*/)
  {
    m_text += std::string ("enter_cell(") + iter->layout ()->cell_name (cell->cell_index ()) + ")\n";
  }

  virtual void leave_cell (const db::RecursiveShapeIterator *iter, const db::Cell *cell)
  {
    m_text += std::string ("leave_cell(") + iter->layout ()->cell_name (cell->cell_index ()) + ")\n";
  }

  virtual new_inst_mode new_inst (const db::RecursiveShapeIterator *iter, const db::CellInstArray &inst, const db::ICplxTrans & /*always_apply*/, const db::Box & /*region*/, const box_tree_type * /*complex_region*/, bool all, bool /*skip_shapes*/)
  {
    m_text += std::string ("new_inst(") + iter->layout ()->cell_name (inst.object ().cell_index ());
    if (all) {
      m_text += ",all";
    }
    m_text += ")\n";
    return NI_all;
  }

  virtual bool new_inst_member (const db::RecursiveShapeIterator *iter, const db::CellInstArray &inst, const db::ICplxTrans &always_apply, const db::ICplxTrans &trans, const db::Box & /*region*/, const box_tree_type * /*complex_region*/, bool all, bool /*skip_shapes*/)
  {
    m_text += std::string ("new_inst_member(") + iter->layout ()->cell_name (inst.object ().cell_index ()) + "," + tl::to_string (always_apply * trans);
    if (all) {
      m_text += ",all";
    }
    m_text += ")\n";
    return true;
  }

  virtual void shape (const db::RecursiveShapeIterator * /*iter*/, const db::Shape &shape, const db::ICplxTrans & /*always_apply*/, const db::ICplxTrans &trans, const db::Box & /*region*/, const box_tree_type * /*complex_region*/)
  {
    m_text += "shape(" + shape.to_string () + "," + tl::to_string (trans) + ")\n";
  }

private:
  std::string m_text;
};

class ReceiverRejectingACellInstanceArray
  : public LoggingReceiver
{
public:
  ReceiverRejectingACellInstanceArray (db::cell_index_type rejected) : m_rejected (rejected) { }

  virtual new_inst_mode new_inst (const db::RecursiveShapeIterator *iter, const db::CellInstArray &inst, const db::ICplxTrans &always_apply, const db::Box &region, const box_tree_type *complex_region, bool all, bool skip_shapes)
  {
    LoggingReceiver::new_inst (iter, inst, always_apply, region, complex_region, all, skip_shapes);
    return inst.object ().cell_index () != m_rejected ? NI_all : NI_skip;
  }

private:
  db::cell_index_type m_rejected;
};

class ReceiverRejectingACellInstanceArrayExceptOne
  : public LoggingReceiver
{
public:
  ReceiverRejectingACellInstanceArrayExceptOne (db::cell_index_type rejected) : m_rejected (rejected) { }

  virtual new_inst_mode new_inst (const db::RecursiveShapeIterator *iter, const db::CellInstArray &inst, const db::ICplxTrans &always_apply, const db::Box &region, const box_tree_type *complex_region, bool all, bool skip_shapes)
  {
    LoggingReceiver::new_inst (iter, inst, always_apply, region, complex_region, all, skip_shapes);
    return inst.object ().cell_index () != m_rejected ? NI_all : NI_single;
  }

private:
  db::cell_index_type m_rejected;
};

class ReceiverRejectingACellInstance
  : public LoggingReceiver
{
public:
  ReceiverRejectingACellInstance (db::cell_index_type rejected, const db::ICplxTrans &trans_rejected) : m_rejected (rejected), m_trans_rejected (trans_rejected) { }

  virtual bool new_inst_member (const db::RecursiveShapeIterator *iter, const db::CellInstArray &inst, const db::ICplxTrans &always_apply, const db::ICplxTrans &trans, const db::Box &region, const box_tree_type *complex_region, bool all, bool skip_shapes)
  {
    LoggingReceiver::new_inst_member (iter, inst, always_apply, trans, region, complex_region, all, skip_shapes);
    return inst.object ().cell_index () != m_rejected || trans != m_trans_rejected;
  }

private:
  db::cell_index_type m_rejected;
  db::ICplxTrans m_trans_rejected;
};

//  Push mode with cells
TEST(10)
{
  db::Manager m (true);
  db::Layout g (&m);
  g.insert_layer(0);

  db::Cell &c0 (g.cell (g.add_cell ()));
  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));

  db::Box b (1000, -500, 2000, 500);
  c2.shapes (0).insert (b);
  c0.shapes (0).insert (b.moved (db::Vector (-1000, 500)));
  c0.shapes (0).insert (b.moved (db::Vector (-2000, 500)));

  db::Trans tt;
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), tt, db::Vector (0, 6000), db::Vector (6000, 0), 2, 2));
  c1.insert (db::CellInstArray (db::CellInst (c2.cell_index ()), tt, db::Vector (0, 2000), db::Vector (3000, 1000), 2, 2));

  LoggingReceiver lr1;
  db::RecursiveShapeIterator i1 (g, c0, 0);
  i1.push (&lr1);

  EXPECT_EQ (lr1.text (),
    "begin\n"
    "new_inst($2,all)\n"
    "new_inst_member($2,r0 *1 0,0,all)\n"
    //  It's a bit weird to have shape events after new_inst_member, but remember, new_inst_member is a query callback, not an event.
    "shape(box (0,0;1000,1000),r0 *1 0,0)\n"
    "shape(box (-1000,0;0,1000),r0 *1 0,0)\n"
    "enter_cell($2)\n"
    "new_inst($3,all)\n"
    "new_inst_member($3,r0 *1 0,0,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 0,0)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 0,2000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 0,2000)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,1000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 3000,1000)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,3000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 3000,3000)\n"
    "leave_cell($3)\n"
    "leave_cell($2)\n"
    "new_inst_member($2,r0 *1 0,6000,all)\n"
    "enter_cell($2)\n"
    "new_inst($3,all)\n"
    "new_inst_member($3,r0 *1 0,0,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 0,6000)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 0,2000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 0,8000)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,1000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 3000,7000)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,3000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 3000,9000)\n"
    "leave_cell($3)\n"
    "leave_cell($2)\n"
    "new_inst_member($2,r0 *1 6000,0,all)\n"
    "enter_cell($2)\n"
    "new_inst($3,all)\n"
    "new_inst_member($3,r0 *1 0,0,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 6000,0)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 0,2000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 6000,2000)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,1000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 9000,1000)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,3000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 9000,3000)\n"
    "leave_cell($3)\n"
    "leave_cell($2)\n"
    "new_inst_member($2,r0 *1 6000,6000,all)\n"
    "enter_cell($2)\n"
    "new_inst($3,all)\n"
    "new_inst_member($3,r0 *1 0,0,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 6000,6000)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 0,2000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 6000,8000)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,1000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 9000,7000)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,3000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 9000,9000)\n"
    "leave_cell($3)\n"
    "leave_cell($2)\n"
    "end\n"
  );

  LoggingReceiver lr1_gt;
  db::RecursiveShapeIterator i1_gt (g, c0, 0);
  i1_gt.set_global_trans (db::ICplxTrans (db::Trans (db::Vector (10, 20))));
  i1_gt.push (&lr1_gt);

  EXPECT_EQ (lr1_gt.text (),
    "begin\n"
    "new_inst($2,all)\n"
    "new_inst_member($2,r0 *1 10,20,all)\n"
    //  It's a bit weird to have shape events after new_inst_member, but remember, new_inst_member is a query callback, not an event.
    "shape(box (0,0;1000,1000),r0 *1 10,20)\n"
    "shape(box (-1000,0;0,1000),r0 *1 10,20)\n"
    "enter_cell($2)\n"
    "new_inst($3,all)\n"
    "new_inst_member($3,r0 *1 0,0,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 10,20)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 0,2000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 10,2020)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,1000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 3010,1020)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,3000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 3010,3020)\n"
    "leave_cell($3)\n"
    "leave_cell($2)\n"
    "new_inst_member($2,r0 *1 10,6020,all)\n"
    "enter_cell($2)\n"
    "new_inst($3,all)\n"
    "new_inst_member($3,r0 *1 0,0,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 10,6020)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 0,2000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 10,8020)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,1000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 3010,7020)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,3000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 3010,9020)\n"
    "leave_cell($3)\n"
    "leave_cell($2)\n"
    "new_inst_member($2,r0 *1 6010,20,all)\n"
    "enter_cell($2)\n"
    "new_inst($3,all)\n"
    "new_inst_member($3,r0 *1 0,0,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 6010,20)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 0,2000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 6010,2020)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,1000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 9010,1020)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,3000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 9010,3020)\n"
    "leave_cell($3)\n"
    "leave_cell($2)\n"
    "new_inst_member($2,r0 *1 6010,6020,all)\n"
    "enter_cell($2)\n"
    "new_inst($3,all)\n"
    "new_inst_member($3,r0 *1 0,0,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 6010,6020)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 0,2000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 6010,8020)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,1000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 9010,7020)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,3000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 9010,9020)\n"
    "leave_cell($3)\n"
    "leave_cell($2)\n"
    "end\n"
  );

  ReceiverRejectingACellInstanceArray rr1 (c2.cell_index ());
  db::RecursiveShapeIterator ir1 (g, c0, 0);
  ir1.push (&rr1);

  EXPECT_EQ (rr1.text (),
    "begin\n"
    "new_inst($2,all)\n"
    "new_inst_member($2,r0 *1 0,0,all)\n"
    "shape(box (0,0;1000,1000),r0 *1 0,0)\n"
    "shape(box (-1000,0;0,1000),r0 *1 0,0)\n"
    "enter_cell($2)\n"
    "new_inst($3,all)\n"
    "leave_cell($2)\n"
    "new_inst_member($2,r0 *1 0,6000,all)\n"
    "enter_cell($2)\n"
    "new_inst($3,all)\n"
    "leave_cell($2)\n"
    "new_inst_member($2,r0 *1 6000,0,all)\n"
    "enter_cell($2)\n"
    "new_inst($3,all)\n"
    "leave_cell($2)\n"
    "new_inst_member($2,r0 *1 6000,6000,all)\n"
    "enter_cell($2)\n"
    "new_inst($3,all)\n"
    "leave_cell($2)\n"
    "end\n"
  );

  ReceiverRejectingACellInstanceArrayExceptOne rs1 (c2.cell_index ());
  db::RecursiveShapeIterator is1 (g, c0, 0);
  is1.push (&rs1);

  EXPECT_EQ (rs1.text (),
    "begin\n"
    "new_inst($2,all)\n"
    "new_inst_member($2,r0 *1 0,0,all)\n"
    "shape(box (0,0;1000,1000),r0 *1 0,0)\n"
    "shape(box (-1000,0;0,1000),r0 *1 0,0)\n"
    "enter_cell($2)\n"
    "new_inst($3,all)\n"
    "new_inst_member($3,r0 *1 0,0,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 0,0)\n"
    "leave_cell($3)\n"
    "leave_cell($2)\n"
    "new_inst_member($2,r0 *1 0,6000,all)\n"
    "enter_cell($2)\n"
    "new_inst($3,all)\n"
    "new_inst_member($3,r0 *1 0,0,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 0,6000)\n"
    "leave_cell($3)\n"
    "leave_cell($2)\n"
    "new_inst_member($2,r0 *1 6000,0,all)\n"
    "enter_cell($2)\n"
    "new_inst($3,all)\n"
    "new_inst_member($3,r0 *1 0,0,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 6000,0)\n"
    "leave_cell($3)\n"
    "leave_cell($2)\n"
    "new_inst_member($2,r0 *1 6000,6000,all)\n"
    "enter_cell($2)\n"
    "new_inst($3,all)\n"
    "new_inst_member($3,r0 *1 0,0,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 6000,6000)\n"
    "leave_cell($3)\n"
    "leave_cell($2)\n"
    "end\n"
  );

  ReceiverRejectingACellInstance rri1 (c2.cell_index (), db::ICplxTrans ());
  db::RecursiveShapeIterator iri1 (g, c0, 0);
  iri1.push (&rri1);

  EXPECT_EQ (rri1.text (),
    "begin\n"
    "new_inst($2,all)\n"
    "new_inst_member($2,r0 *1 0,0,all)\n"
    "shape(box (0,0;1000,1000),r0 *1 0,0)\n"
    "shape(box (-1000,0;0,1000),r0 *1 0,0)\n"
    "enter_cell($2)\n"
    "new_inst($3,all)\n"
    "new_inst_member($3,r0 *1 0,0,all)\n"     // -> skipped
    "new_inst_member($3,r0 *1 0,2000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 0,2000)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,1000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 3000,1000)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,3000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 3000,3000)\n"
    "leave_cell($3)\n"
    "leave_cell($2)\n"
    "new_inst_member($2,r0 *1 0,6000,all)\n"
    "enter_cell($2)\n"
    "new_inst($3,all)\n"
    "new_inst_member($3,r0 *1 0,0,all)\n"
    "new_inst_member($3,r0 *1 0,2000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 0,8000)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,1000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 3000,7000)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,3000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 3000,9000)\n"
    "leave_cell($3)\n"
    "leave_cell($2)\n"
    "new_inst_member($2,r0 *1 6000,0,all)\n"
    "enter_cell($2)\n"
    "new_inst($3,all)\n"
    "new_inst_member($3,r0 *1 0,0,all)\n"
    "new_inst_member($3,r0 *1 0,2000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 6000,2000)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,1000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 9000,1000)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,3000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 9000,3000)\n"
    "leave_cell($3)\n"
    "leave_cell($2)\n"
    "new_inst_member($2,r0 *1 6000,6000,all)\n"
    "enter_cell($2)\n"
    "new_inst($3,all)\n"
    "new_inst_member($3,r0 *1 0,0,all)\n"
    "new_inst_member($3,r0 *1 0,2000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 6000,8000)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,1000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 9000,7000)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,3000,all)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 9000,9000)\n"
    "leave_cell($3)\n"
    "leave_cell($2)\n"
    "end\n"
  );

  ReceiverRejectingACellInstanceArray rr2 (c1.cell_index ());
  db::RecursiveShapeIterator ir2 (g, c0, 0);
  ir2.push (&rr2);

  EXPECT_EQ (rr2.text (),
    "begin\n"
    "new_inst($2,all)\n"
    "shape(box (0,0;1000,1000),r0 *1 0,0)\n"
    "shape(box (-1000,0;0,1000),r0 *1 0,0)\n"
    "end\n"
  );

  LoggingReceiver lr2;
  db::RecursiveShapeIterator i2 (g, c0, 0, db::Box (0, 0, 5000, 5000));
  i2.push (&lr2);

  EXPECT_EQ (lr2.text (),
    "begin\n"
    "new_inst($2)\n"
    "new_inst_member($2,r0 *1 0,0)\n"
    "shape(box (0,0;1000,1000),r0 *1 0,0)\n"
    "shape(box (-1000,0;0,1000),r0 *1 0,0)\n"
    "enter_cell($2)\n"
    "new_inst($3)\n"
    "new_inst_member($3,r0 *1 0,0)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 0,0)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 0,2000)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 0,2000)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,1000)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 3000,1000)\n"
    "leave_cell($3)\n"
    "new_inst_member($3,r0 *1 3000,3000)\n"
    "enter_cell($3)\n"
    "shape(box (1000,-500;2000,500),r0 *1 3000,3000)\n"
    "leave_cell($3)\n"
    "leave_cell($2)\n"
    "end\n"
  );
}

TEST(11_LayoutIsWeakPointer)
{
  std::unique_ptr<db::Layout> g (new db::Layout ());
  g->insert_layer (0);
  g->insert_layer (1);
  db::Cell &c0 (g->cell (g->add_cell ()));
  db::Cell &c1 (g->cell (g->add_cell ()));
  db::Cell &c2 (g->cell (g->add_cell ()));
  db::Cell &c3 (g->cell (g->add_cell ()));

  db::Box b (0, 100, 1000, 1200);
  c1.shapes (0).insert (b);
  c2.shapes (0).insert (b);
  c3.shapes (0).insert (b);

  db::Box bb (1, 101, 1001, 1201);
  c2.shapes (1).insert (bb);

  db::Trans tt;
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), tt));
  c0.insert (db::CellInstArray (db::CellInst (c2.cell_index ()), db::Trans (db::Vector (100, -100))));
  c0.insert (db::CellInstArray (db::CellInst (c3.cell_index ()), db::Trans (1)));
  c2.insert (db::CellInstArray (db::CellInst (c3.cell_index ()), db::Trans (db::Vector (1100, 0))));

  std::string x;

  db::RecursiveShapeIterator i1 (*g, c0, 0, db::Box (0, 0, 100, 100));
  x = collect(i1, *g);
  EXPECT_EQ (x, "[$2](0,100;1000,1200)/[$3](100,0;1100,1100)");

  g.reset (new db::Layout ());

  //  now the layout is gone and the iterator stays silent (weak pointer to layout)
  //  NOTE: this only works on reset or re-initialization. Not during iteration.
  i1.reset ();
  x = collect(i1, *g);
  EXPECT_EQ (x, "");
}

TEST(12_ForMerged)
{
  std::unique_ptr<db::Layout> g (new db::Layout ());
  g->insert_layer (0);
  g->insert_layer (1);
  db::Cell &c0 (g->cell (g->add_cell ()));
  db::Cell &c1 (g->cell (g->add_cell ()));
  db::Cell &c2 (g->cell (g->add_cell ()));
  db::Cell &c3 (g->cell (g->add_cell ()));

  db::Box b (0, 100, 1000, 1200);
  c0.shapes (0).insert (db::Box (0, 0, 3000, 2000));
  c1.shapes (0).insert (b);
  c2.shapes (0).insert (b);
  c3.shapes (0).insert (b);

  db::Trans tt;
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), tt));
  c0.insert (db::CellInstArray (db::CellInst (c2.cell_index ()), db::Trans (db::Vector (100, -100))));
  c0.insert (db::CellInstArray (db::CellInst (c3.cell_index ()), db::Trans (1)));
  c2.insert (db::CellInstArray (db::CellInst (c3.cell_index ()), db::Trans (db::Vector (1100, 0))));

  std::string x;

  db::RecursiveShapeIterator i1 (*g, c0, 0);
  x = collect(i1, *g);
  EXPECT_EQ (x, "[$1](0,0;3000,2000)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");
  EXPECT_EQ (collect_with_copy(i1, *g), x);

  i1.set_for_merged_input (true);
  x = collect(i1, *g);
  EXPECT_EQ (x, "[$1](0,0;3000,2000)/[$4](-1200,0;-100,1000)");
  EXPECT_EQ (collect_with_copy(i1, *g), x);

  std::vector<unsigned int> lv;
  lv.push_back (0);
  i1 = db::RecursiveShapeIterator (*g, c0, lv);
  x = collect(i1, *g);
  EXPECT_EQ (x, "[$1](0,0;3000,2000)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");
  EXPECT_EQ (collect_with_copy(i1, *g), x);

  i1.set_for_merged_input (true);
  x = collect(i1, *g);
  EXPECT_EQ (x, "[$1](0,0;3000,2000)/[$4](-1200,0;-100,1000)");
  EXPECT_EQ (collect_with_copy(i1, *g), x);

  lv.push_back (1); //  empty, but kills "for merged" optimization
  i1 = db::RecursiveShapeIterator (*g, c0, lv);
  x = collect(i1, *g);
  EXPECT_EQ (x, "[$1](0,0;3000,2000)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");
  EXPECT_EQ (collect_with_copy(i1, *g), x);

  {
    FlatPusher f;
    i1.reset ();
    i1.push (&f);
    EXPECT_EQ (f.to_string (), "(-1200,0;-100,1000);(0,0;3000,2000);(100,0;1100,1100);(1200,0;2200,1100);(0,100;1000,1200)");
  }

  i1.set_for_merged_input (true);
  x = collect(i1, *g);
  //  no longer optimized
  EXPECT_EQ (x, "[$1](0,0;3000,2000)/[$2](0,100;1000,1200)/[$3](100,0;1100,1100)/[$4](1200,0;2200,1100)/[$4](-1200,0;-100,1000)");
  EXPECT_EQ (collect_with_copy(i1, *g), x);

  {
    FlatPusher f;
    i1.reset ();
    i1.push (&f);
    EXPECT_EQ (f.to_string (), "(-1200,0;-100,1000);(0,0;3000,2000);(100,0;1100,1100);(1200,0;2200,1100);(0,100;1000,1200)");
  }

  i1 = db::RecursiveShapeIterator (*g, c0, 0, db::Box (-100, 0, 100, 50));
  x = collect(i1, *g);
  EXPECT_EQ (x, "[$1](0,0;3000,2000)/[$3](100,0;1100,1100)/[$4](-1200,0;-100,1000)");

  {
    FlatPusher f;
    i1.reset ();
    i1.push (&f);
    EXPECT_EQ (f.to_string (), "(-1200,0;-100,1000);(0,0;3000,2000);(100,0;1100,1100)");
  }

  i1.set_for_merged_input (true);
  x = collect(i1, *g);
  EXPECT_EQ (x, "[$1](0,0;3000,2000)/[$4](-1200,0;-100,1000)");
  EXPECT_EQ (collect_with_copy(i1, *g), x);

  {
    FlatPusher f;
    i1.reset ();
    i1.push (&f);
    EXPECT_EQ (f.to_string (), "(-1200,0;-100,1000);(0,0;3000,2000)");
  }

  i1 = db::RecursiveShapeIterator (*g, c0, 0, db::Box (-101, 0, 100, 50));
  i1.set_overlapping (true);
  x = collect(i1, *g);
  EXPECT_EQ (x, "[$1](0,0;3000,2000)/[$4](-1200,0;-100,1000)");
  EXPECT_EQ (collect_with_copy(i1, *g), x);

  {
    FlatPusher f;
    i1.reset ();
    i1.push (&f);
    EXPECT_EQ (f.to_string (), "(-1200,0;-100,1000);(0,0;3000,2000)");
  }

  i1.set_for_merged_input (true);
  x = collect(i1, *g);
  EXPECT_EQ (x, "[$1](0,0;3000,2000)/[$4](-1200,0;-100,1000)");
  EXPECT_EQ (collect_with_copy(i1, *g), x);

  {
    FlatPusher f;
    i1.reset ();
    i1.push (&f);
    EXPECT_EQ (f.to_string (), "(-1200,0;-100,1000);(0,0;3000,2000)");
  }
}

TEST(12b_ForMerged)
{
  std::unique_ptr<db::Layout> g (new db::Layout ());
  g->insert_layer (0);
  g->insert_layer (1);
  db::Cell &c0 (g->cell (g->add_cell ()));
  db::Cell &c1 (g->cell (g->add_cell ()));

  db::Box b (0, 100, 1000, 1200);
  c0.shapes (0).insert (db::Box (0, 0, 3000, 2200));
  c1.shapes (0).insert (b);

  db::Trans tt;
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), tt));
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), db::Trans (db::Vector (2000, 1000)), db::Vector (0, 2000), db::Vector (2000, 0), 2l, 2l));

  std::string x;

  db::RecursiveShapeIterator i1 (*g, c0, 0);
  x = collect(i1, *g);
  EXPECT_EQ (x, "[$1](0,0;3000,2200)/[$2](0,100;1000,1200)/[$2](2000,1100;3000,2200)/[$2](2000,3100;3000,4200)/[$2](4000,1100;5000,2200)/[$2](4000,3100;5000,4200)");
  EXPECT_EQ (collect_with_copy(i1, *g), x);

  {
    FlatPusher f;
    i1.reset ();
    i1.push (&f);
    EXPECT_EQ (f.to_string (), "(0,0;3000,2200);(0,100;1000,1200);(2000,1100;3000,2200);(4000,1100;5000,2200);(2000,3100;3000,4200);(4000,3100;5000,4200)");
  }

  i1.set_for_merged_input (true);
  x = collect(i1, *g);
  EXPECT_EQ (x, "[$1](0,0;3000,2200)/[$2](2000,3100;3000,4200)/[$2](4000,1100;5000,2200)/[$2](4000,3100;5000,4200)");
  EXPECT_EQ (collect_with_copy(i1, *g), x);

  {
    FlatPusher f;
    i1.reset ();
    i1.push (&f);
    EXPECT_EQ (f.to_string (), "(0,0;3000,2200);(4000,1100;5000,2200);(2000,3100;3000,4200);(4000,3100;5000,4200)");
  }

  i1.set_for_merged_input (false);
  x = collect(i1, *g);
  EXPECT_EQ (x, "[$1](0,0;3000,2200)/[$2](0,100;1000,1200)/[$2](2000,1100;3000,2200)/[$2](2000,3100;3000,4200)/[$2](4000,1100;5000,2200)/[$2](4000,3100;5000,4200)");
  EXPECT_EQ (collect_with_copy(i1, *g), x);

  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), db::Trans (db::Vector (0, 2000))));

  db::RecursiveShapeIterator i2 (*g, c0, 0);

  x = collect(i2, *g);
  EXPECT_EQ (x, "[$1](0,0;3000,2200)/[$2](0,100;1000,1200)/[$2](2000,1100;3000,2200)/[$2](2000,3100;3000,4200)/[$2](4000,1100;5000,2200)/[$2](4000,3100;5000,4200)/[$2](0,2100;1000,3200)");
  EXPECT_EQ (collect_with_copy(i2, *g), x);

  {
    FlatPusher f;
    i2.reset ();
    i2.push (&f);
    EXPECT_EQ (f.to_string (), "(0,0;3000,2200);(0,100;1000,1200);(2000,1100;3000,2200);(4000,1100;5000,2200);(0,2100;1000,3200);(2000,3100;3000,4200);(4000,3100;5000,4200)");
  }

  i2.set_for_merged_input (true);
  x = collect(i2, *g);
  EXPECT_EQ (x, "[$1](0,0;3000,2200)/[$2](2000,3100;3000,4200)/[$2](4000,1100;5000,2200)/[$2](4000,3100;5000,4200)/[$2](0,2100;1000,3200)");
  EXPECT_EQ (collect_with_copy(i2, *g), x);

  {
    FlatPusher f;
    i2.reset ();
    i2.push (&f);
    EXPECT_EQ (f.to_string (), "(0,0;3000,2200);(4000,1100;5000,2200);(0,2100;1000,3200);(2000,3100;3000,4200);(4000,3100;5000,4200)");
  }
}

TEST(13_ForMergedPerformance)
{
  test_is_long_runner ();

  std::string fn (tl::combine_path (tl::testdata_private (), "oasis/caravel.oas.gz"));

  db::Layout ly;

  {
    tl::InputStream is (fn);
    db::Reader reader (is);
    reader.read (ly);
  }

  unsigned l1 = ly.get_layer (db::LayerProperties (66, 20));
  unsigned l2 = ly.get_layer (db::LayerProperties (235, 4));

  db::RecursiveShapeIterator si1 (ly, ly.cell (*ly.begin_top_down ()), l1);
  db::RecursiveShapeIterator si2 (ly, ly.cell (*ly.begin_top_down ()), l2);

  size_t n1_expected_full = db::default_editable_mode () ? 1203072 : 1203078;
  size_t n2_expected_full = 10;

  {
    tl::SelfTimer timer ("Standard loop on 66/20");
    size_t n = 0;
    while (! si1.at_end ()) {
      ++si1;
      ++n;
    }
    tl::info << "Counted " << n << " shapes on 66/20";
    EXPECT_EQ (n, size_t (1218378));
  }

  {
    tl::SelfTimer timer ("Standard loop on 235/4");
    size_t n = 0;
    while (! si2.at_end ()) {
      ++si2;
      ++n;
    }
    tl::info << "Counted " << n << " shapes on 235/4";
    EXPECT_EQ (n, size_t (57462));
  }

  si1.set_for_merged_input (true);
  si2.set_for_merged_input (true);

  {
    tl::SelfTimer timer ("'for_merged' loop on 66/20");
    size_t n = 0;
    while (! si1.at_end ()) {
      ++si1;
      ++n;
    }
    tl::info << "Counted " << n << " shapes on 66/20";
    EXPECT_EQ (n, size_t (n1_expected_full));
  }

  {
    tl::SelfTimer timer ("'for_merged' loop on 235/4");
    size_t n = 0;
    while (! si2.at_end ()) {
      ++si2;
      ++n;
    }
    tl::info << "Counted " << n << " shapes on 235/4";
    EXPECT_EQ (n, size_t (n2_expected_full));
  }

  si1.set_for_merged_input (false);
  si1.set_region (db::Box (0, 0, 1000000, 1000000));
  si2.set_for_merged_input (false);
  si2.set_region (db::Box (0, 0, 1000000, 1000000));

  {
    tl::SelfTimer timer ("Standard loop on 66/20");
    size_t n = 0;
    while (! si1.at_end ()) {
      ++si1;
      ++n;
    }
    tl::info << "Counted " << n << " shapes on 66/20";
    EXPECT_EQ (n, size_t (218823));
  }

  {
    tl::SelfTimer timer ("Standard loop on 235/4");
    size_t n = 0;
    while (! si2.at_end ()) {
      ++si2;
      ++n;
    }
    tl::info << "Counted " << n << " shapes on 235/4";
    EXPECT_EQ (n, size_t (2578));
  }

  si1.set_for_merged_input (true);
  si2.set_for_merged_input (true);

  size_t n1_expected = db::default_editable_mode () ? 218068 : 218069;
  size_t n2_expected = 2;

  {
    tl::SelfTimer timer ("'for_merged' loop on 66/20");
    size_t n = 0;
    while (! si1.at_end ()) {
      ++si1;
      ++n;
    }
    tl::info << "Counted " << n << " shapes on 66/20";
    EXPECT_EQ (n, size_t (n1_expected));
  }

  {
    tl::SelfTimer timer ("'for_merged' loop on 235/4");
    size_t n = 0;
    while (! si2.at_end ()) {
      ++si2;
      ++n;
    }
    tl::info << "Counted " << n << " shapes on 235/4";
    EXPECT_EQ (n, size_t (n2_expected));
  }

  {
    tl::SelfTimer timer ("XOR on tile of 66/20");
    si1.set_for_merged_input (false);
    db::Region r1 (si1);
    si1.set_for_merged_input (true);
    db::Region r2 (si1);

    EXPECT_EQ (r1.count (), size_t (218823));
    EXPECT_EQ (r2.count (), size_t (n1_expected));
    EXPECT_EQ ((r1 ^ r2).count (), size_t (0));
  }

  {
    tl::SelfTimer timer ("XOR on tile of 235/4");
    si2.set_for_merged_input (false);
    db::Region r1 (si2);
    si2.set_for_merged_input (true);
    db::Region r2 (si2);

    EXPECT_EQ (r1.count (), size_t (2578));
    EXPECT_EQ (r2.count (), size_t (n2_expected));
    EXPECT_EQ ((r1 ^ r2).count (), size_t (0));
  }
}

//  layout locking
TEST(14_LayoutLocking)
{
  db::Layout layout;

  layout.insert_layer (0);

  db::Cell &c0 (layout.cell (layout.add_cell ()));
  db::Cell &c1 (layout.cell (layout.add_cell ()));

  db::Box b (0, 100, 1000, 1200);
  c1.shapes (0).insert (b);

  db::Trans tt;
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), tt));
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), db::Trans (db::Vector (2000, -2000))));

  EXPECT_EQ (layout.under_construction (), false);

  db::RecursiveShapeIterator iter (layout, c0, 0);

  EXPECT_EQ (layout.under_construction (), false);

  EXPECT_EQ (iter.at_end (), false);
  EXPECT_EQ (layout.under_construction (), true);

  EXPECT_EQ (iter.shape ().to_string (), "box (0,100;1000,1200)");
  EXPECT_EQ (layout.under_construction (), true);
  ++iter;

  EXPECT_EQ (iter.at_end (), false);

  EXPECT_EQ (iter.shape ().to_string (), "box (0,100;1000,1200)");
  EXPECT_EQ (layout.under_construction (), true);
  ++iter;

  EXPECT_EQ (layout.under_construction (), false);
  EXPECT_EQ (iter.at_end (), true);

  //  reset will restart
  iter.reset ();

  EXPECT_EQ (layout.under_construction (), false);

  EXPECT_EQ (iter.at_end (), false);
  EXPECT_EQ (layout.under_construction (), true);

  //  a copy will hold the lock
  iter.reset ();

  EXPECT_EQ (layout.under_construction (), false);
  EXPECT_EQ (iter.at_end (), false);

  EXPECT_EQ (layout.under_construction (), true);
  db::RecursiveShapeIterator iter_copy = iter;

  while (! iter.at_end ()) {
    ++iter;
  }

  EXPECT_EQ (layout.under_construction (), true);
  iter_copy = db::RecursiveShapeIterator ();

  EXPECT_EQ (layout.under_construction (), false);
}

