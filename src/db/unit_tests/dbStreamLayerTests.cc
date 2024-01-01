
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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
#include "dbLayout.h"
#include "tlUnitTest.h"

TEST(1) 
{
  db::LayerMap lm;

  lm.map (db::LDPair (1, 5), 17);
  EXPECT_EQ (lm.first_logical (db::LDPair (1, 6)).first, false);
  EXPECT_EQ (lm.first_logical (db::LDPair (1, 5)).first, true);
  EXPECT_EQ (lm.first_logical (db::LDPair (1, 5)).second, (unsigned int) 17);

  lm.map (db::LDPair (1, 0), db::LDPair (5,0), 18);
  EXPECT_EQ (lm.first_logical (db::LDPair (2, 0)).first, true);
  EXPECT_EQ (lm.first_logical (db::LDPair (2, 0)).second, (unsigned int) 18);
  EXPECT_EQ (lm.first_logical (db::LDPair (0, 0)).first, false);

  EXPECT_EQ (lm.mapping_str (18), "1-5/0");
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
  EXPECT_EQ (lm.first_logical ("XP").second, (unsigned int) 13);
  EXPECT_EQ (lm.first_logical ("XP").first, true);
  EXPECT_EQ (lm.first_logical (db::LDPair(10, 6)).first, false);
  EXPECT_EQ (lm.first_logical (db::LDPair(10, 7)).first, true);
  EXPECT_EQ (lm.first_logical (db::LDPair(10, 7)).second, (unsigned int) 13);

  EXPECT_EQ (lm.mapping (13).to_string (), "XN (10/7)");

  lm.clear ();
  EXPECT_EQ (lm.first_logical (db::LDPair(10, 7)).first, false);
  lm.map_expr ("'XP';10/7-8 : XN", 13);
  EXPECT_EQ (lm.mapping_str (13), "10/7-8;XP : XN");

  //  brackets, "add_expr"
  lm.clear ();
  lm.add_expr ("[1-10/*]", 1);
  EXPECT_EQ (lm.mapping_str (1), "1-10/* : */*");
  lm.add_expr ("-(5/*)", 0);
  EXPECT_EQ (lm.mapping_str (1), "1-4/*;6-10/* : */*");

  lm.clear ();
  lm.add_expr ("[1/15]", 1);
  lm.add_expr ("+(1/5:1001/5)", 1);
  //  NOTE: the target is taken from the second expression (the last one wins)
  EXPECT_EQ (lm.mapping_str (1), "1/5,15 : 1001/5");

  lm.clear ();
  lm.add_expr ("+(1/5:1001/5)", 1);
  lm.add_expr ("[1/15]", 1);
  //  NOTE: the target is taken from the second expression (the last one wins)
  EXPECT_EQ (lm.mapping_str (1), "1/5,15 : */*");
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

TEST(3)
{
  EXPECT_EQ (db::is_relative_ld (1), false);
  EXPECT_EQ (db::is_relative_ld (0), false);
  EXPECT_EQ (db::is_static_ld (0), true);
  EXPECT_EQ (db::is_relative_ld (db::relative_ld (0)), true);
  EXPECT_EQ (db::is_relative_ld (db::any_ld ()), true);
  EXPECT_EQ (db::is_relative_ld (db::relative_ld (1)), true);
  EXPECT_EQ (db::is_relative_ld (db::relative_ld (-1)), true);
  EXPECT_EQ (db::is_static_ld (db::relative_ld (-1)), false);
  EXPECT_EQ (db::is_any_ld (db::any_ld ()), true);
  EXPECT_EQ (db::is_any_ld (1), false);
  EXPECT_EQ (db::is_any_ld (db::relative_ld (-1)), false);
  EXPECT_EQ (db::ld_offset (db::relative_ld (-1)), -1);
  EXPECT_EQ (db::ld_offset (db::relative_ld (-100)), -100);
  EXPECT_EQ (db::ld_offset (db::relative_ld (0)), 0);
  EXPECT_EQ (db::ld_offset (db::relative_ld (1)), 1);
  EXPECT_EQ (db::ld_offset (db::relative_ld (100)), 100);
  EXPECT_EQ (db::ld_offset (100), 100);
  EXPECT_EQ (db::ld_combine (1, db::relative_ld (100)), 101);
  EXPECT_EQ (db::ld_combine (1, 100), 100);
  EXPECT_EQ (db::ld_combine (100, db::relative_ld (-1)), 99);
}

TEST(4)
{
  db::LayerMap lm;

  unsigned int n = 0;

  //  named, no catch-all
  lm.map (db::LayerProperties ("NAME"), n++);

  EXPECT_EQ (lm.to_string (),
    "layer_map('NAME')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());

  lm.clear ();
  n = 0;

  //  single layer
  lm.map (db::LayerProperties (1, 2), n++);
  //  single layer, wildcard target
  lm.map (db::LayerProperties (1, 3), n++, db::LayerProperties (db::any_ld (), db::any_ld ()));
  lm.map (db::LayerProperties (1, 4), n++, db::LayerProperties (2, db::any_ld ()));
  lm.map (db::LayerProperties (1, 5), n++, db::LayerProperties (db::any_ld (), 15));
  //  single layer, relative target
  lm.map (db::LayerProperties (1, 6), n++, db::LayerProperties (db::any_ld (), db::relative_ld (3)));

  EXPECT_EQ (lm.to_string (),
    "layer_map('1/2';'1/3 : */*';'1/4 : 2/*';'1/5 : */15';'1/6 : */*+3')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());

  lm.clear ();
  n = 0;

  //  datatype catch-all
  lm.map (db::LayerProperties (1, db::any_ld ()), n++);
  //  datatype catch-all, fixed targets
  lm.map (db::LayerProperties (2, db::any_ld ()), n++, db::LayerProperties (12, 2));
  //  datatype catch-all, wildcard targets
  lm.map (db::LayerProperties (3, db::any_ld ()), n++, db::LayerProperties (db::any_ld (), 2));
  lm.map (db::LayerProperties (4, db::any_ld ()), n++, db::LayerProperties (db::any_ld (), db::any_ld ()));
  //  datatype catch-all, relative targets
  lm.map (db::LayerProperties (5, db::any_ld ()), n++, db::LayerProperties (15, db::relative_ld (0)));
  lm.map (db::LayerProperties (6, db::any_ld ()), n++, db::LayerProperties (16, db::relative_ld (-1)));
  lm.map (db::LayerProperties (7, db::any_ld ()), n++, db::LayerProperties (17, db::relative_ld (1)));

  EXPECT_EQ (lm.to_string (),
    "layer_map('1/*';'2/* : 12/2';'3/* : */2';'4/* : */*';'5/* : 15/*';'6/* : 16/*-1';'7/* : 17/*+1')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());

  lm.clear ();
  n = 0;

  //  layer catch-all
  lm.map (db::LayerProperties (db::any_ld (), 1), n++);
  //  layer catch-all, fixed targets
  lm.map (db::LayerProperties (db::any_ld (), 2), n++, db::LayerProperties (1, 12));
  //  layer catch-all, wildcard targets
  lm.map (db::LayerProperties (db::any_ld (), 3), n++, db::LayerProperties (db::any_ld (), 2));
  lm.map (db::LayerProperties (db::any_ld (), 4), n++, db::LayerProperties (db::any_ld (), db::any_ld ()));
  //  layer catch-all, relative targets
  lm.map (db::LayerProperties (db::any_ld (), 5), n++, db::LayerProperties (2, db::relative_ld (0)));
  lm.map (db::LayerProperties (db::any_ld (), 6), n++, db::LayerProperties (2, db::relative_ld (-1)));
  lm.map (db::LayerProperties (db::any_ld (), 7), n++, db::LayerProperties (2, db::relative_ld (1)));

  EXPECT_EQ (lm.to_string (),
    "layer_map('*/1';'*/2 : 1/12';'*/3 : */2';'*/4 : */*';'*/5 : 2/*';'*/6 : 2/*-1';'*/7 : 2/*+1')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());

  lm.clear ();
  n = 0;

  //  layer and datatype catch-all
  lm.map (db::LayerProperties (db::any_ld (), db::any_ld ()), n++);

  EXPECT_EQ (lm.to_string (),
    "layer_map('*/*')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());

  lm.clear ();
  n = 0;

  //  layer and datatype catch-all, fixed targets
  lm.map (db::LayerProperties (db::any_ld (), db::any_ld ()), n++, db::LayerProperties (1, 2));

  EXPECT_EQ (lm.to_string (),
    "layer_map('*/* : 1/2')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());

  lm.clear ();
  n = 0;

  //  layer and datatype catch-all, wildcard targets
  lm.map (db::LayerProperties (db::any_ld (), db::any_ld ()), n++, db::LayerProperties (db::any_ld (), 2));

  EXPECT_EQ (lm.to_string (),
    "layer_map('*/* : */2')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());

  lm.clear ();
  n = 0;

  lm.map (db::LayerProperties (db::any_ld (), db::any_ld ()), n++, db::LayerProperties (db::any_ld (), db::any_ld ()));

  EXPECT_EQ (lm.to_string (),
    "layer_map('*/* : */*')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());

  lm.clear ();
  n = 0;

  //  layer and datatype catch-all, relative targets
  lm.map (db::LayerProperties (db::any_ld (), db::any_ld ()), n++, db::LayerProperties (2, db::relative_ld (0)));

  EXPECT_EQ (lm.to_string (),
    "layer_map('*/* : 2/*')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());

  lm.clear ();
  n = 0;

  lm.map (db::LayerProperties (db::any_ld (), db::any_ld ()), n++, db::LayerProperties (2, db::relative_ld (-1)));

  EXPECT_EQ (lm.to_string (),
    "layer_map('*/* : 2/*-1')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());

  lm.clear ();
  n = 0;

  lm.map (db::LayerProperties (db::any_ld (), db::any_ld ()), n++, db::LayerProperties (2, db::relative_ld (1)));

  EXPECT_EQ (lm.to_string (),
    "layer_map('*/* : 2/*+1')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());
}

TEST(5)
{
  db::LayerMap lm;

  unsigned int n = 0;

  //  refinement
  //  all
  lm.map_expr ("*/*", n++);
  //  some
  lm.map_expr ("*/1-10", n++);
  //  others
  lm.map_expr ("*/5,15", n++);

  EXPECT_EQ (lm.to_string (),
    "layer_map('*/0,11-14,16-*';'*/1-4,6-10';'*/5,15')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());

  //  orthogonal layer refinement
  lm.map_expr ("17/*", n++);

  EXPECT_EQ (lm.to_string (),
    "layer_map('0-16/0,11-14,16-*;18-*/0,11-14,16-*';'0-16/1-4,6-10;18-*/1-4,6-10';'0-16/5,15;18-*/5,15';'17/*')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());
}

static std::string layers_to_string (const db::Layout &ly)
{
  std::string s;

  for (unsigned int i = 0; i < ly.layers (); ++i) {
    if (ly.is_valid_layer (i)) {
      if (! s.empty ()) {
        s += ",";
      }
      s += ly.get_properties (i).to_string ();
    }
  }

  return s;
}

TEST(6)
{
  db::Layout ly;
  db::LayerMap lm;

  EXPECT_EQ (layers_to_string (ly), "");

  unsigned int n = 0;
  lm.map_expr ("1/0", n++);
  lm.map_expr ("2/* : */*", n++);
  lm.map_expr ("3/10-*", n++);  //  all layers are mapped to 3/10

  lm.prepare (ly);

  EXPECT_EQ (layers_to_string (ly), "1/0,3/10");

  std::pair<bool, unsigned int> p;
  p = lm.first_logical (db::LayerProperties (1, 0));
  EXPECT_EQ (p.first, true);
  EXPECT_EQ (p.second, (unsigned int) 0);

  p = lm.first_logical (db::LayerProperties (2, 0));
  EXPECT_EQ (p.first, false);

  p = lm.first_logical (db::LayerProperties (3, 0));
  EXPECT_EQ (p.first, false);

  p = lm.first_logical (db::LayerProperties (3, 10));
  EXPECT_EQ (p.first, true);
  EXPECT_EQ (p.second, (unsigned int) 1);

  p = lm.first_logical (db::LayerProperties (3, 99));
  EXPECT_EQ (p.first, true);
  EXPECT_EQ (p.second, (unsigned int) 1);

  EXPECT_EQ (layers_to_string (ly), "1/0,3/10");

  //  this will create layer 2/0 in the layout
  p = lm.first_logical (db::LayerProperties (2, 0), ly);
  EXPECT_EQ (p.first, true);
  EXPECT_EQ (p.second, (unsigned int) 2);

  EXPECT_EQ (layers_to_string (ly), "1/0,3/10,2/0");

  p = lm.first_logical (db::LayerProperties (2, 0));
  EXPECT_EQ (p.first, true);
  EXPECT_EQ (p.second, (unsigned int) 2);

  p = lm.first_logical (db::LayerProperties (2, 0), ly);
  EXPECT_EQ (p.first, true);
  EXPECT_EQ (p.second, (unsigned int) 2);

  EXPECT_EQ (layers_to_string (ly), "1/0,3/10,2/0");

  //  this will create layer 2/42 in the layout
  p = lm.first_logical (db::LayerProperties (2, 42), ly);
  EXPECT_EQ (p.first, true);
  EXPECT_EQ (p.second, (unsigned int) 3);

  EXPECT_EQ (layers_to_string (ly), "1/0,3/10,2/0,2/42");

  p = lm.first_logical (db::LayerProperties (2, 42));
  EXPECT_EQ (p.first, true);
  EXPECT_EQ (p.second, (unsigned int) 3);

  p = lm.first_logical (db::LayerProperties (2, 42), ly);
  EXPECT_EQ (p.first, true);
  EXPECT_EQ (p.second, (unsigned int) 3);

  EXPECT_EQ (layers_to_string (ly), "1/0,3/10,2/0,2/42");

  EXPECT_EQ (lm.to_string (),
    "layer_map('1/0';'3/10-*';'2/0 : 2/0';'2/42 : 2/42';'2/1-41,43-* : */*')"
  );
}

// issue #592
TEST(7)
{
  db::Layout ly;

  unsigned int l1 = ly.insert_layer (db::LayerProperties (85, 0));
  unsigned int l2 = ly.insert_layer (db::LayerProperties (185, 0));
  ly.insert_layer ();
  ly.insert_layer ();

  db::LayerMap lm;
  lm.map (db::LayerProperties (10001, 0), l1);
  lm.map (db::LayerProperties (10000, 0), l2);

  EXPECT_EQ (layers_to_string (ly), "85/0,185/0,,");

  lm.prepare (ly);

  EXPECT_EQ (layers_to_string (ly), "85/0,185/0,,");

  std::pair<bool, unsigned int> p;
  p = lm.first_logical (db::LayerProperties (85, 0));
  EXPECT_EQ (p.first, false);
  EXPECT_EQ (p.second, (unsigned int) 0);

  p = lm.first_logical (db::LayerProperties (185, 0));
  EXPECT_EQ (p.first, false);
  EXPECT_EQ (p.second, (unsigned int) 0);

  p = lm.first_logical (db::LayerProperties (10000, 0));
  EXPECT_EQ (p.first, true);
  EXPECT_EQ (p.second, (unsigned int) 1);

  p = lm.first_logical (db::LayerProperties (10001, 0));
  EXPECT_EQ (p.first, true);
  EXPECT_EQ (p.second, (unsigned int) 0);
}

static std::string set2string (const std::set<unsigned int> &set)
{
  std::string s;
  for (std::set<unsigned int>::const_iterator i = set.begin (); i != set.end (); ++i) {
    if (i != set.begin ()) {
      s += ",";
    }
    s += tl::to_string (*i);
  }
  return s;
}

//  multi-mapping, unmapping
TEST(8)
{
  db::LayerMap lm;

  unsigned int n = 0;

  //  refinement
  //  all
  lm.mmap_expr ("*/*", n++);
  EXPECT_EQ (lm.mapping_str (0), "*/*");
  EXPECT_EQ (lm.to_string (),
    "layer_map('*/*')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());

  //  some
  lm.mmap_expr ("*/1-10", n++);
  EXPECT_EQ (lm.to_string (),
    "layer_map('+*/*';'+*/1-10')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());

  //  others
  lm.mmap_expr ("*/5,15", n++);

  EXPECT_EQ (lm.to_string (),
    "layer_map('+*/*';'+*/1-10';'+*/5,15')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());

  EXPECT_EQ (set2string (lm.logical (db::LDPair (0, 1000))), "0");
  EXPECT_EQ (set2string (lm.logical (db::LDPair (1, 1000))), "0");
  EXPECT_EQ (set2string (lm.logical (db::LDPair (0, 5))), "0,1,2");
  EXPECT_EQ (set2string (lm.logical (db::LDPair (0, 15))), "0,2");
  EXPECT_EQ (set2string (lm.logical (db::LDPair (0, 10))), "0,1");

  //  NOTE: the leading "+" indicates that the listed layers may go somewhere else, so we can't plainly map them
  EXPECT_EQ (lm.mapping_str (0), "+*/*");
  EXPECT_EQ (lm.mapping_str (1), "+*/1-10");
  EXPECT_EQ (lm.mapping_str (2), "+*/5,15");
  EXPECT_EQ (lm.mapping_str (3), "");

  lm = db::LayerMap ();
  n = 0;

  //  refinement
  //  all
  lm.mmap_expr ("*/*", n++);
  EXPECT_EQ (lm.mapping_str (0), "*/*");
  EXPECT_EQ (lm.to_string (),
    "layer_map('*/*')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());

  //  some
  lm.mmap_expr ("1-10/*", n++);
  EXPECT_EQ (lm.to_string (),
    "layer_map('+*/*';'+1-10/*')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());

  //  others
  lm.mmap_expr ("5,15/*", n++);

  EXPECT_EQ (lm.to_string (),
    "layer_map('+*/*';'+1-10/*';'+5/*;15/*')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());

  EXPECT_EQ (set2string (lm.logical (db::LDPair (1000, 0))), "0");
  EXPECT_EQ (set2string (lm.logical (db::LDPair (1000, 1))), "0");
  EXPECT_EQ (set2string (lm.logical (db::LDPair (5, 0))), "0,1,2");
  EXPECT_EQ (set2string (lm.logical (db::LDPair (15, 0))), "0,2");
  EXPECT_EQ (set2string (lm.logical (db::LDPair (10, 0))), "0,1");

  //  NOTE: the leading "+" indicates that the listed layers may go somewhere else, so we can't plainly map them
  EXPECT_EQ (lm.mapping_str (0), "+*/*");
  EXPECT_EQ (lm.mapping_str (1), "+1-10/*");
  EXPECT_EQ (lm.mapping_str (2), "+5/*;15/*");
  EXPECT_EQ (lm.mapping_str (3), "");

  lm = db::LayerMap ();
  n = 0;

  lm.mmap_expr ("*/*", n++);
  EXPECT_EQ (lm.mapping_str (0), "*/*");
  EXPECT_EQ (lm.to_string (),
    "layer_map('*/*')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());

  //  some
  lm.mmap_expr ("1-10/0-20", n++);
  EXPECT_EQ (lm.to_string (),
    "layer_map('+*/*';'+1-10/0-20')"
  );
  EXPECT_EQ (db::LayerMap::from_string_file_format (lm.to_string_file_format ()).to_string (), lm.to_string ());
}

