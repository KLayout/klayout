
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


#include "dbCellVariants.h"
#include "tlUnitTest.h"

std::string var2str (const std::set<db::ICplxTrans> &vars)
{
  std::string res;
  for (std::set<db::ICplxTrans>::const_iterator i = vars.begin (); i != vars.end (); ++i) {
    if (! res.empty ()) {
      res += ";";
    }
    res += i->to_string ();
  }
  return res;
}

TEST(1_Trivial)
{

  db::Layout ly;
  db::Cell &a = ly.cell (ly.add_cell ("A"));
  db::Cell &b = ly.cell (ly.add_cell ("B"));
  db::Cell &c = ly.cell (ly.add_cell ("C"));
  db::Cell &d = ly.cell (ly.add_cell ("D"));

  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::Trans (0, false, db::Vector (1, 10))));

  db::OrientationReducer red;
  db::cell_variants_builder<db::OrientationReducer> vb (red);
  vb.build (ly, a);
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "r0 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "");
}

TEST(2_TwoVariants)
{

  db::Layout ly;
  db::Cell &a = ly.cell (ly.add_cell ("A"));
  db::Cell &b = ly.cell (ly.add_cell ("B"));
  db::Cell &c = ly.cell (ly.add_cell ("C"));
  db::Cell &d = ly.cell (ly.add_cell ("D"));

  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::Trans (0, false, db::Vector (1, 10))));
  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::Trans (0, true, db::Vector (1, 100))));

  db::OrientationReducer red;
  db::cell_variants_builder<db::OrientationReducer> vb (red);
  vb.build (ly, a);
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "m0 *1 0,0;r0 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "");
}

TEST(3_TwoLevels)
{

  db::Layout ly;
  db::Cell &a = ly.cell (ly.add_cell ("A"));
  db::Cell &b = ly.cell (ly.add_cell ("B"));
  db::Cell &c = ly.cell (ly.add_cell ("C"));
  db::Cell &d = ly.cell (ly.add_cell ("D"));

  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::Trans (0, false, db::Vector (1, 10))));
  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::Trans (1, false, db::Vector (1, 100))));
  b.insert (db::CellInstArray (db::CellInst (c.cell_index ()), db::Trans (0, false, db::Vector (2, 10))));
  b.insert (db::CellInstArray (db::CellInst (c.cell_index ()), db::Trans (0, true, db::Vector (2, 100))));

  db::OrientationReducer red;
  db::cell_variants_builder<db::OrientationReducer> vb (red);
  vb.build (ly, a);
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "r0 *1 0,0;r90 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "m0 *1 0,0;r0 *1 0,0;m45 *1 0,0;r90 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "");
}
