
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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



#include "dbD25TechnologyComponent.h"
#include "tlUnitTest.h"


TEST(1)
{
  db::D25TechnologyComponent comp;

  comp.compile_from_source ("1/0: 1.0 1.5 # a comment");
  EXPECT_EQ (comp.to_string (), "1/0: zstart=1, zstop=1.5");

  comp.compile_from_source ("1/0: zstart=1.0 zstop=1.5");
  EXPECT_EQ (comp.to_string (), "1/0: zstart=1, zstop=1.5");

  comp.compile_from_source ("1/0: zstart=1.0 height=0.5");
  EXPECT_EQ (comp.to_string (), "1/0: zstart=1, zstop=1.5");

  comp.compile_from_source ("1/0: 1.0 height=0.5");
  EXPECT_EQ (comp.to_string (), "1/0: zstart=1, zstop=1.5");

  comp.compile_from_source ("1/0: zstop=1.5 height=0.5");
  EXPECT_EQ (comp.to_string (), "1/0: zstart=1, zstop=1.5");

  comp.compile_from_source ("1/0: zstart=1.0 zstop=1.5\nname: height=3");
  EXPECT_EQ (comp.to_string (), "1/0: zstart=1, zstop=1.5\nname: zstart=1.5, zstop=4.5");

  comp.compile_from_source ("1/0: zstart=1.0 zstop=1.5\nname: zstart=4.0 height=3\n\n# a comment line");
  EXPECT_EQ (comp.to_string (), "1/0: zstart=1, zstop=1.5\nname: zstart=4, zstop=7");

  try {
    comp.compile_from_source ("blabla");
    EXPECT_EQ (false, true);
  } catch (...) { }

  try {
    comp.compile_from_source ("1/0: 1 2 3");
    EXPECT_EQ (false, true);
  } catch (...) { }

  try {
    comp.compile_from_source ("1/0: foo=1 bar=2");
    EXPECT_EQ (false, true);
  } catch (...) { }

  try {
    comp.compile_from_source ("1/0: 1;2");
    EXPECT_EQ (false, true);
  } catch (...) { }
}
