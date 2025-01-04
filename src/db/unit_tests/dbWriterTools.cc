
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


#include "dbWriterTools.h"
#include "tlUnitTest.h"

TEST(1) 
{
  db::WriterCellNameMap m(10);

  m.allow_standard (true, true, true);
  m.replacement ('~');
  m.transform ("# *", "*\t\0");

  m.insert (0, "A+");
  EXPECT_EQ (m.cell_name (0), "A~");

  m.insert (1, "A_");
  EXPECT_EQ (m.cell_name (1), "A~~1");

  m.insert (2, "A#");
  EXPECT_EQ (m.cell_name (2), "A*");

  m.insert (3, "A ");
  EXPECT_EQ (m.cell_name (3), "A~20");

  m.insert (4, "A*");
  EXPECT_EQ (m.cell_name (4), "A~~2");

  m.insert (5, "ABCDEFGHI");
  EXPECT_EQ (m.cell_name (5), "ABCDEFGHI");

  m.insert (6, "ABCDEFGHI");
  EXPECT_EQ (m.cell_name (6), "ABCDEFGH~1");

  m.insert (7, "ABCDEFGHI");
  EXPECT_EQ (m.cell_name (7), "ABCDEFGH~2");

  for (int id = 10; id < 20; ++id) {
    m.insert (id, "ABCDEFGHI");
  }
  EXPECT_EQ (m.cell_name (19), "ABCDEFG~12");

  m.insert (105, "ABCDEFGHIJ");
  EXPECT_EQ (m.cell_name (105), "ABCDEFGHIJ");

  m.insert (106, "ABCDEFGHIJ");
  EXPECT_EQ (m.cell_name (106), "ABCDEFG~13");

  m.insert (107, "ABCDEFGHIJ");
  EXPECT_EQ (m.cell_name (107), "ABCDEFG~14");

  for (int id = 110; id < 210; ++id) {
    m.insert (id, "ABCDEFGHIJ");
  }
  EXPECT_EQ (m.cell_name (209), "ABCDEF~114");

  m.insert (300, "ABCDEFGHIJK");
  EXPECT_EQ (m.cell_name (300), "ABCDEF~115");

  m.insert (301, "ABCDEFGHIJX");
  EXPECT_EQ (m.cell_name (301), "ABCDEF~116");

  m.insert (302, "0BCDEFGHIJX");
  EXPECT_EQ (m.cell_name (302), "0BCDEFGHIJ");

  m.insert (303, "0BCDEFGHIJX");
  EXPECT_EQ (m.cell_name (303), "0BCDEFGH~1");

  m.replacement ('$');

  m.insert (304, "0BCDEFGHIJX");
  EXPECT_EQ (m.cell_name (304), "0BCDEFGH$1");
}


