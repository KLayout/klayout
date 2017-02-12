
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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



#include "dbStreamLayers.h"
#include "utHead.h"

TEST(1) 
{
  db::LayerMap lm;

  lm.map (db::LDPair (1, 5), 17);
  EXPECT_EQ (lm.logical (db::LDPair (1, 6)).first, false);
  EXPECT_EQ (lm.logical (db::LDPair (1, 5)).first, true);
  EXPECT_EQ (lm.logical (db::LDPair (1, 5)).second, 17);

  lm.map (db::LDPair (1, 0), db::LDPair (5,0), 18);
  EXPECT_EQ (lm.logical (db::LDPair (2, 0)).first, true);
  EXPECT_EQ (lm.logical (db::LDPair (2, 0)).second, 18);
  EXPECT_EQ (lm.logical (db::LDPair (0, 0)).first, false);

  EXPECT_EQ (lm.mapping_str (18), "1/0;2-5/0");
  EXPECT_EQ (lm.mapping_str (17), "1/5");

  lm.map (db::LDPair (2, 2), 18);
  EXPECT_EQ (lm.mapping_str (18), "1/0;2/0,2;3-5/0");
  EXPECT_EQ (lm.mapping (18).to_string (), "3/0"); // any of those above!

  lm.map (db::LDPair (2, 3), 15, db::LayerProperties (17, 18));
  EXPECT_EQ (lm.mapping_str (15), "2/3 : 17/18");

  lm.map ("WN", 22);
  EXPECT_EQ (lm.mapping_str (22), "WN");
  EXPECT_EQ (lm.mapping (22).to_string (), "WN");
  lm.map (db::LDPair (2, 8), 22);
  EXPECT_EQ (lm.mapping (22).to_string (), "WN (2/8)");

  lm.map ("AA", 14, db::LayerProperties ("GC"));
  EXPECT_EQ (lm.mapping_str (14), "AA : GC");
  EXPECT_EQ (lm.mapping (14).to_string (), "GC");
  lm.map (db::LDPair (7, 8), 14);
  EXPECT_EQ (lm.mapping (14).to_string (), "GC (7/8)");

  lm.map_expr ("XP;10/7-8 : XN", 13);
  EXPECT_EQ (lm.mapping_str (13), "10/7-8;XP : XN");
  EXPECT_EQ (lm.logical ("XP").second, 13);
  EXPECT_EQ (lm.logical ("XP").first, true);
  EXPECT_EQ (lm.logical (db::LDPair(10, 6)).first, false);
  EXPECT_EQ (lm.logical (db::LDPair(10, 7)).first, true);
  EXPECT_EQ (lm.logical (db::LDPair(10, 7)).second, 13);

  EXPECT_EQ (lm.mapping (13).to_string (), "XN (10/7)");

  lm.clear ();
  EXPECT_EQ (lm.logical (db::LDPair(10, 7)).first, false);
  lm.map_expr ("'XP';10/7-8 : XN", 13);
  EXPECT_EQ (lm.mapping_str (13), "10/7-8;XP : XN");
}

TEST(2) 
{
  db::LayerMap lm;

  lm.map (db::LDPair (1, 5), 17);
  lm.map (db::LDPair (1, 0), db::LDPair (5,0), 18);
  lm.map (db::LDPair (2, 2), 18);
  lm.map (db::LDPair (2, 3), 15, db::LayerProperties (17, 18));
  lm.map ("WN", 22);
  lm.map ("AA", 14, db::LayerProperties ("GC"));
  lm.map_expr ("XP;10/7-8 : XN", 13);

  EXPECT_EQ (lm.to_string (), "layer_map('10/7-8;XP : XN';'AA : GC';'2/3 : 17/18';'1/5';'1/0;2/0,2;3-5/0';'WN')");
  EXPECT_EQ (lm.to_string_file_format (), "10/7-8;XP : XN\nAA : GC\n2/3 : 17/18\n1/5\n1/0;2/0,2;3-5/0\nWN\n");

  db::LayerMap lm2;
  db::LayerMap lm2read;
  lm2 = db::LayerMap::from_string_file_format (lm2.to_string_file_format ());
  EXPECT_EQ (lm2.to_string (), "layer_map()");
  tl::Extractor (lm2.to_string ()).read (lm2read);
  EXPECT_EQ (lm2read.to_string (), "layer_map()");
  lm2 = db::LayerMap::from_string_file_format (lm.to_string_file_format ());
  EXPECT_EQ (lm2.to_string (), "layer_map('10/7-8;XP : XN';'AA : GC';'2/3 : 17/18';'1/5';'1/0;2/0,2;3-5/0';'WN')");
  tl::Extractor (lm2.to_string ()).read (lm2read);
  EXPECT_EQ (lm2read.to_string (), "layer_map('10/7-8;XP : XN';'AA : GC';'2/3 : 17/18';'1/5';'1/0;2/0,2;3-5/0';'WN')");

  std::string ff = 
    "\n"
    "\t //  a comment\n"
    "10/7-8;XP:XN \t # another comment\n"
    "\n"
    "     AA\t: GC\n"
    " 2/3 : 17/18\n"
    "    1 / 5    \n"
    "\t\t1/0;2/0,2;3-5/0\n"
    "# commented out: 1/0;2/0,2;3-5/0\n"
    "WN";

  lm2 = db::LayerMap::from_string_file_format (ff);
  EXPECT_EQ (lm2.to_string (), "layer_map('10/7-8;XP : XN';'AA : GC';'2/3 : 17/18';'1/5';'1/0;2/0,2;3-5/0';'WN')");
}

