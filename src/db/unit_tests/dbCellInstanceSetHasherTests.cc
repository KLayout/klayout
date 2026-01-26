
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
  // @@@
}
