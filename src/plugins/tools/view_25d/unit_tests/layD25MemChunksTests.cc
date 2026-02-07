
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


#include "layD25MemChunks.h"
#include "tlUnitTest.h"

TEST(1_Basic)
{
  lay::mem_chunks<int, 2> ch;
  EXPECT_EQ (ch.begin () == ch.end (), true);

  ch.add (1);
  EXPECT_EQ (ch.begin () == ch.end (), false);
  EXPECT_EQ (ch.begin ()->size (), size_t (1));
  EXPECT_EQ (ch.begin ()->front () [0], 1);

  ch.add (17);
  EXPECT_EQ (ch.begin () == ch.end (), false);
  EXPECT_EQ (ch.begin ()->size (), size_t (2));
  EXPECT_EQ (ch.begin ()->front () [0], 1);
  EXPECT_EQ (ch.begin ()->front () [1], 17);

  lay::mem_chunks<int, 2>::iterator c = ch.begin ();
  EXPECT_EQ (c == ch.end (), false);
  ++c;
  EXPECT_EQ (c == ch.end (), true);

  ch.add (42);
  c = ch.begin ();
  EXPECT_EQ (c == ch.end (), false);
  EXPECT_EQ (c->size (), size_t (2));
  EXPECT_EQ (c->front () [0], 1);
  EXPECT_EQ (c->front () [1], 17);
  ++c;
  EXPECT_EQ (c == ch.end (), false);
  EXPECT_EQ (c->size (), size_t (1));
  EXPECT_EQ (c->front () [0], 42);
  ++c;
  EXPECT_EQ (c == ch.end (), true);

  ch.clear ();
  EXPECT_EQ (ch.begin () == ch.end (), true);
}

TEST(2_Copy)
{
  lay::mem_chunks<int, 2> ch1;
  ch1.add (1);
  ch1.add (17);
  ch1.add (42);

  lay::mem_chunks<int, 2> ch (ch1);

  lay::mem_chunks<int, 2>::iterator c = ch.begin ();

  EXPECT_EQ (c == ch.end (), false);
  EXPECT_EQ (c->size (), size_t (2));
  EXPECT_EQ (c->front () [0], 1);
  EXPECT_EQ (c->front () [1], 17);
  ++c;
  EXPECT_EQ (c == ch.end (), false);
  EXPECT_EQ (c->size (), size_t (1));
  EXPECT_EQ (c->front () [0], 42);
  ++c;
  EXPECT_EQ (c == ch.end (), true);

  ch1.clear ();
  ch = ch1;
  EXPECT_EQ (ch.begin () == ch.end (), true);
}
