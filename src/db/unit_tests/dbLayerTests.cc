
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


#include "dbLayer.h"
#include "tlUnitTest.h"


TEST(1) 
{
  db::layer<db::Box, db::stable_layer_tag> bl;
  db::Box b_empty;

  bl.update_bbox ();
  EXPECT_EQ (bl.bbox (), b_empty);

  db::Box b (0, 100, 1000, 1200);
  bl.insert (b);
  bl.update_bbox ();
  EXPECT_EQ (bl.bbox (), b);
}

TEST(2) 
{
  db::layer<db::Box, db::unstable_layer_tag> bl;
  db::Box b_empty;

  bl.update_bbox ();
  EXPECT_EQ (bl.bbox (), b_empty);

  db::Box b (0, 100, 1000, 1200);
  bl.insert (b);
  bl.update_bbox ();
  EXPECT_EQ (bl.bbox (), b);
}

