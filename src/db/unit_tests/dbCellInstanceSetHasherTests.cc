
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


#include "dbCellInstanceSetHasher.h"
#include "tlUnitTest.h"

TEST(1)
{
  EXPECT_EQ (db::CellInstanceSetHasher::MatrixHash ().to_string (), "(1,0,0) (0,1,0) (0,0,1)");
  EXPECT_EQ (db::CellInstanceSetHasher::MatrixHash (0).to_string (), "(0,0,0) (0,0,0) (0,0,0)");
  EXPECT_EQ (db::CellInstanceSetHasher::MatrixHash (db::ICplxTrans (2.0, 90.0, false, db::Vector (1, 2))).to_string (), "(0,-2,1) (2,0,2) (0,0,1)");

  db::ICplxTrans t0 (2.0, 90.0, false, db::Vector (1, 2));

  db::CellInstArray array (db::CellInst (0), t0, db::Vector (0, 100), db::Vector (100, 0), 2, 3);
  EXPECT_EQ (db::CellInstanceSetHasher::MatrixHash (array).to_string (), "(0,-12,606) (12,0,312) (0,0,6)");

  //  emulate the regular array with an iterated array
  std::vector<db::Vector> dd;
  for (unsigned int i = 0; i < 2; ++i) {
    for (unsigned int j = 0; j < 3; ++j) {
      db::Vector d = db::Vector (0, 100 * i) + db::Vector (100 * j, 0);
      dd.push_back (d);
    }
  }
  db::CellInstArray iter_array (db::CellInst (0), t0, dd.begin (), dd.end ());
  EXPECT_EQ (db::CellInstanceSetHasher::MatrixHash (iter_array).to_string (), db::CellInstanceSetHasher::MatrixHash (array).to_string ());

  //  equivalence of sum of matrices and computed matrix for array
  db::CellInstanceSetHasher::MatrixHash hm (0.0);
  for (unsigned int i = 0; i < 2; ++i) {
    for (unsigned int j = 0; j < 3; ++j) {
      db::Vector d = db::Vector (0, 100 * i) + db::Vector (100 * j, 0);
      hm += db::CellInstanceSetHasher::MatrixHash (db::ICplxTrans (d) * t0);
    }
  }
  EXPECT_EQ (db::CellInstanceSetHasher::MatrixHash (hm).to_string (), db::CellInstanceSetHasher::MatrixHash (array).to_string ());
}

TEST(2)
{
  db::Layout ly;

  db::cell_index_type top = ly.add_cell ("TOP");
  db::cell_index_type c1 = ly.add_cell ("C1");
  db::cell_index_type c2 = ly.add_cell ("C2");
  db::cell_index_type c3 = ly.add_cell ("C3");
  db::cell_index_type c4a = ly.add_cell ("C4A");
  db::cell_index_type c5a = ly.add_cell ("C5A");
  db::cell_index_type c4b = ly.add_cell ("C4B");
  db::cell_index_type c5b = ly.add_cell ("C5B");

  ly.cell (top).insert (db::CellInstArray (db::CellInst (c1), db::Trans (1, true, db::Vector (0, 0))));
  ly.cell (top).insert (db::CellInstArray (db::CellInst (c1), db::Trans (0, false, db::Vector (0, 10000))));

  ly.cell (c1).insert (db::CellInstArray (db::CellInst (c2), db::Trans (1, true, db::Vector (100, 200)), db::Vector (0, 1000), db::Vector (1000, 0), 2l, 3l));

  //  C4 and C5 are single instances in C2, C5 with mag 2
  ly.cell (c2).insert (db::CellInstArray (db::CellInst (c4a), db::ICplxTrans (1.0, 0.0, false, db::Vector (10, 20))));
  ly.cell (c2).insert (db::CellInstArray (db::CellInst (c5a), db::ICplxTrans (2.0, 0.0, false, db::Vector (10, 20))));

  //  C3 has same instances as C2, but flat
  ly.cell (top).insert (db::CellInstArray (db::CellInst (c3), db::Trans (1, true, db::Vector (100, 10200))));
  ly.cell (top).insert (db::CellInstArray (db::CellInst (c3), db::Trans (1, true, db::Vector (100, 11200))));
  ly.cell (top).insert (db::CellInstArray (db::CellInst (c3), db::Trans (1, true, db::Vector (1100, 10200))));
  ly.cell (top).insert (db::CellInstArray (db::CellInst (c3), db::Trans (1, true, db::Vector (1100, 11200))));
  ly.cell (top).insert (db::CellInstArray (db::CellInst (c3), db::Trans (1, true, db::Vector (2100, 10200))));
  ly.cell (top).insert (db::CellInstArray (db::CellInst (c3), db::Trans (1, true, db::Vector (2100, 11200))));
  ly.cell (top).insert (db::CellInstArray (db::CellInst (c3), db::Trans (0, false, db::Vector (200, 100))));
  ly.cell (top).insert (db::CellInstArray (db::CellInst (c3), db::Trans (0, false, db::Vector (200, 1100))));
  ly.cell (top).insert (db::CellInstArray (db::CellInst (c3), db::Trans (0, false, db::Vector (200, 2100))));
  ly.cell (top).insert (db::CellInstArray (db::CellInst (c3), db::Trans (0, false, db::Vector (1200, 100))));
  ly.cell (top).insert (db::CellInstArray (db::CellInst (c3), db::Trans (0, false, db::Vector (1200, 1100))));
  ly.cell (top).insert (db::CellInstArray (db::CellInst (c3), db::Trans (0, false, db::Vector (1200, 2100))));

  //  C4 and C5 are single instances in C3, C5 with a different complex angle (45 degree)
  ly.cell (c3).insert (db::CellInstArray (db::CellInst (c4b), db::ICplxTrans (1.0, 0.0, false, db::Vector (10, 20))));
  ly.cell (c3).insert (db::CellInstArray (db::CellInst (c5b), db::ICplxTrans (1.0, 45.0, false, db::Vector (10, 20))));

  db::CellInstanceSetHasher hasher1 (&ly, top, 0);

  EXPECT_EQ (tl::sprintf ("%08lx", hasher1.instance_set_hash (top)), "00004450");
  EXPECT_EQ (tl::sprintf ("%08lx", hasher1.instance_set_hash (c1)), "00023711");
  EXPECT_EQ (tl::sprintf ("%08lx", hasher1.instance_set_hash (c2)), "001260aa");
  EXPECT_EQ (hasher1.instance_set_hash (c3), hasher1.instance_set_hash (c2));
  EXPECT_EQ (tl::sprintf ("%08lx", hasher1.instance_set_hash (c4a)), "001270ba");
  EXPECT_EQ (hasher1.instance_set_hash (c4a), hasher1.instance_set_hash (c4b));
  EXPECT_EQ (tl::sprintf ("%08lx", hasher1.instance_set_hash (c5a)), "0010da3a"); // != hash of C4A because of mag 2
  EXPECT_EQ (tl::sprintf ("%08lx", hasher1.instance_set_hash (c5b)), "0011d5c4"); // != hash of C5A because of 45 degree angle

  std::set<db::cell_index_type> set1;
  set1.insert (top);
  set1.insert (c1);
  set1.insert (c2);
  set1.insert (c3);
  set1.insert (c4a);
  set1.insert (c5a);
  set1.insert (c4b);
  set1.insert (c5b);
  db::CellInstanceSetHasher hasher2 (&ly, top, &set1);

  EXPECT_EQ (hasher1.instance_set_hash (top), hasher2.instance_set_hash (top));
  EXPECT_EQ (hasher1.instance_set_hash (c1), hasher2.instance_set_hash (c1));
  EXPECT_EQ (hasher1.instance_set_hash (c2), hasher2.instance_set_hash (c2));
  EXPECT_EQ (hasher1.instance_set_hash (c3), hasher2.instance_set_hash (c3));
  EXPECT_EQ (hasher1.instance_set_hash (c4a), hasher2.instance_set_hash (c4a));
  EXPECT_EQ (hasher1.instance_set_hash (c4b), hasher2.instance_set_hash (c4b));
  EXPECT_EQ (hasher1.instance_set_hash (c5a), hasher2.instance_set_hash (c5a));
  EXPECT_EQ (hasher1.instance_set_hash (c5b), hasher2.instance_set_hash (c5b));

  std::set<db::cell_index_type> set2 = set1;
  //  Remove C1 from selected set
  set2.erase (c1);
  db::CellInstanceSetHasher hasher3 (&ly, top, &set2);

  EXPECT_EQ (tl::sprintf ("%08lx", hasher3.instance_set_hash (top)), "00004450");
  //  NOTE: C1 hash is not valid as this cell is not selected
  EXPECT_EQ (tl::sprintf ("%08lx", hasher3.instance_set_hash (c2)), "00000000");  // no path to TOP
  EXPECT_EQ (tl::sprintf ("%08lx", hasher3.instance_set_hash (c3)), "001260aa");
  EXPECT_EQ (tl::sprintf ("%08lx", hasher3.instance_set_hash (c4a)), "00000000");  // no path to TOP
  EXPECT_EQ (tl::sprintf ("%08lx", hasher3.instance_set_hash (c4b)), "001270ba");
  EXPECT_EQ (tl::sprintf ("%08lx", hasher3.instance_set_hash (c5a)), "00000000");  // no path to TOP
  EXPECT_EQ (tl::sprintf ("%08lx", hasher3.instance_set_hash (c5b)), "0011d5c4");  // != hash of C5A because of 45 degree angle
}

