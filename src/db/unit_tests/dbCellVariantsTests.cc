
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


#include "dbCellVariants.h"
#include "dbTestSupport.h"
#include "dbReader.h"
#include "tlUnitTest.h"
#include "tlStream.h"

std::string var2str (const std::set<db::ICplxTrans> &vars)
{
  std::string res;
  for (auto i = vars.begin (); i != vars.end (); ++i) {
    if (! res.empty ()) {
      res += ";";
    }
    res += i->to_string ();
  }
  return res;
}

std::string var2str (const std::map<db::ICplxTrans, size_t> &vars)
{
  std::string res;
  for (std::map<db::ICplxTrans, size_t>::const_iterator i = vars.begin (); i != vars.end (); ++i) {
    if (! res.empty ()) {
      res += ";";
    }
    res += i->first.to_string ();
    res += "[";
    res += tl::to_string (i->second);
    res += "]";
  }
  return res;
}

std::string vm2str (const db::Layout &ly, const std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > &vm)
{
  std::string res;
  for (std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> >::const_iterator i = vm.begin (); i != vm.end (); ++i) {
    if (! res.empty ()) {
      res += ";";
    }
    res += ly.cell_name (i->first);
    res += ":";
    for (std::map<db::ICplxTrans, db::cell_index_type>::const_iterator j = i->second.begin (); j != i->second.end (); ++j) {
      if (j != i->second.begin ()) {
        res += ",";
      }
      res += ly.cell_name (j->second);
      res += "[";
      res += j->first.to_string ();
      res += "]";
    }
  }
  return res;
}

std::string inst2str (const db::Layout &ly, const db::Cell &cell)
{
  std::string res;
  for (db::Cell::const_iterator i = cell.begin (); ! i.at_end (); ++i) {
    for (db::CellInstArray::iterator ia = i->begin (); ! ia.at_end (); ++ia) {
      db::ICplxTrans rt = i->complex_trans (*ia);
      if (! res.empty ()) {
        res += ";";
      }
      res += ly.cell_name (i->cell_index ());
      res += ":";
      res += rt.to_string ();
    }
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
  db::cell_variants_collector<db::OrientationReducer> vb (red);
  vb.collect (&ly, a.cell_index ());
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "r0 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "");

  std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > vm;
  vb.separate_variants (&vm);
  EXPECT_EQ (vm.empty (), true);
  EXPECT_EQ (vm2str (ly, vm), "");
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
  db::cell_variants_collector<db::OrientationReducer> vb (red);
  vb.collect (&ly, a.cell_index ());
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "m0 *1 0,0;r0 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "");

  EXPECT_EQ (inst2str (ly, a), "B:r0 *1 1,10;B:m0 *1 1,100");

  std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > vm;
  vb.separate_variants (&vm);
  EXPECT_EQ (vm2str (ly, vm), "B:B[m0 *1 0,0],B$VAR1[r0 *1 0,0]");
  EXPECT_EQ (inst2str (ly, a), "B$VAR1:r0 *1 1,10;B:m0 *1 1,100");
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
  db::cell_variants_collector<db::OrientationReducer> vb (red);
  vb.collect (&ly, a.cell_index ());
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "r0 *1 0,0;r90 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "m0 *1 0,0;r0 *1 0,0;m45 *1 0,0;r90 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "");

  EXPECT_EQ (inst2str (ly, a), "B:r0 *1 1,10;B:r90 *1 1,100");
  EXPECT_EQ (inst2str (ly, b), "C:r0 *1 2,10;C:m0 *1 2,100");

  std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > vm;
  vb.separate_variants (&vm);
  EXPECT_EQ (vm2str (ly, vm), "B:B[r0 *1 0,0],B$VAR1[r90 *1 0,0];C:C[m0 *1 0,0],C$VAR1[r0 *1 0,0],C$VAR2[m45 *1 0,0],C$VAR3[r90 *1 0,0]");

  EXPECT_EQ (inst2str (ly, a), "B:r0 *1 1,10;B$VAR1:r90 *1 1,100");
  EXPECT_EQ (inst2str (ly, b), "C$VAR1:r0 *1 2,10;C:m0 *1 2,100");
  EXPECT_EQ (inst2str (ly, ly.cell (ly.cell_by_name ("B$VAR1").second)), "C$VAR3:r0 *1 2,10;C$VAR2:m0 *1 2,100");
}

TEST(4_ThreeLevels)
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
  c.insert (db::CellInstArray (db::CellInst (d.cell_index ()), db::Trans (1, true, db::Vector (0, 0))));

  db::OrientationReducer red;
  db::cell_variants_collector<db::OrientationReducer> vb (red);
  vb.collect (&ly, a.cell_index ());
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "r0 *1 0,0;r90 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "m0 *1 0,0;r0 *1 0,0;m45 *1 0,0;r90 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "r270 *1 0,0;m90 *1 0,0;r0 *1 0,0;m45 *1 0,0");

  EXPECT_EQ (inst2str (ly, a), "B:r0 *1 1,10;B:r90 *1 1,100");
  EXPECT_EQ (inst2str (ly, b), "C:r0 *1 2,10;C:m0 *1 2,100");
  EXPECT_EQ (inst2str (ly, c), "D:m45 *1 0,0");

  std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > vm;
  vb.separate_variants (&vm);
  EXPECT_EQ (vm2str (ly, vm), "B:B[r0 *1 0,0],B$VAR1[r90 *1 0,0];C:C[m0 *1 0,0],C$VAR1[r0 *1 0,0],C$VAR2[m45 *1 0,0],C$VAR3[r90 *1 0,0];D:D[r270 *1 0,0],D$VAR1[m90 *1 0,0],D$VAR2[r0 *1 0,0],D$VAR3[m45 *1 0,0]");

  EXPECT_EQ (inst2str (ly, a), "B:r0 *1 1,10;B$VAR1:r90 *1 1,100");
  EXPECT_EQ (inst2str (ly, b), "C$VAR1:r0 *1 2,10;C:m0 *1 2,100");
  EXPECT_EQ (inst2str (ly, ly.cell (ly.cell_by_name ("B$VAR1").second)), "C$VAR3:r0 *1 2,10;C$VAR2:m0 *1 2,100");
  EXPECT_EQ (inst2str (ly, c), "D:m45 *1 0,0");
  EXPECT_EQ (inst2str (ly, ly.cell (ly.cell_by_name ("C$VAR1").second)), "D$VAR3:m45 *1 0,0");
  EXPECT_EQ (inst2str (ly, ly.cell (ly.cell_by_name ("C$VAR2").second)), "D$VAR2:m45 *1 0,0");
  EXPECT_EQ (inst2str (ly, ly.cell (ly.cell_by_name ("C$VAR3").second)), "D$VAR1:m45 *1 0,0");
}

TEST(5_ComplexTrans)
{
  db::Layout ly;
  db::Cell &a = ly.cell (ly.add_cell ("A"));
  db::Cell &b = ly.cell (ly.add_cell ("B"));
  db::Cell &c = ly.cell (ly.add_cell ("C"));
  db::Cell &d = ly.cell (ly.add_cell ("D"));

  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::ICplxTrans (db::Trans (0, false, db::Vector (1, 10)))));
  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::ICplxTrans (db::Trans (1, false, db::Vector (1, 100)))));
  b.insert (db::CellInstArray (db::CellInst (c.cell_index ()), db::ICplxTrans (db::Trans (0, false, db::Vector (2, 10)))));
  b.insert (db::CellInstArray (db::CellInst (c.cell_index ()), db::ICplxTrans (db::Trans (0, true, db::Vector (2, 100)))));

  db::OrientationReducer red;
  db::cell_variants_collector<db::OrientationReducer> vb (red);
  vb.collect (&ly, a.cell_index ());
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "r0 *1 0,0;r90 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "m0 *1 0,0;r0 *1 0,0;m45 *1 0,0;r90 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "");
}

TEST(6_Arrays)
{
  db::Layout ly;
  db::Cell &a = ly.cell (ly.add_cell ("A"));
  db::Cell &b = ly.cell (ly.add_cell ("B"));
  db::Cell &c = ly.cell (ly.add_cell ("C"));
  db::Cell &d = ly.cell (ly.add_cell ("D"));

  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::Trans (0, false, db::Vector (1, 10)), db::Vector (0, 100), db::Vector (100, 0), 10, 10));
  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::Trans (1, false, db::Vector (1, 100))));
  b.insert (db::CellInstArray (db::CellInst (c.cell_index ()), db::Trans (0, false, db::Vector (2, 10)), db::Vector (0, 101), db::Vector (101, 0), 10, 10));
  b.insert (db::CellInstArray (db::CellInst (c.cell_index ()), db::Trans (0, true, db::Vector (2, 100))));

  db::OrientationReducer red;
  db::cell_variants_collector<db::OrientationReducer> vb (red);
  vb.collect (&ly, a.cell_index ());
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "r0 *1 0,0;r90 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "m0 *1 0,0;r0 *1 0,0;m45 *1 0,0;r90 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "");
}

TEST(7_ScalingVariants)
{
  db::Layout ly;
  db::Cell &a = ly.cell (ly.add_cell ("A"));
  db::Cell &b = ly.cell (ly.add_cell ("B"));
  db::Cell &c = ly.cell (ly.add_cell ("C"));
  db::Cell &d = ly.cell (ly.add_cell ("D"));

  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::ICplxTrans (1.5, 0, false, db::Vector (1, 10)), db::Vector (0, 100), db::Vector (100, 0), 10, 10));
  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::ICplxTrans (1.0, 90.0, false, db::Vector (1, 100))));
  b.insert (db::CellInstArray (db::CellInst (c.cell_index ()), db::ICplxTrans (2.0, 0, false, db::Vector (2, 10)), db::Vector (0, 101), db::Vector (101, 0), 10, 10));
  b.insert (db::CellInstArray (db::CellInst (c.cell_index ()), db::ICplxTrans (1.0, 0, true, db::Vector (2, 100))));

  db::MagnificationReducer red;
  db::cell_variants_collector<db::MagnificationReducer> vb (red);
  vb.collect (&ly, a.cell_index ());
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "r0 *1 0,0;r0 *1.5 0,0");
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "r0 *1 0,0;r0 *1.5 0,0;r0 *2 0,0;r0 *3 0,0");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "");
}

TEST(8_GridVariants)
{
  db::Layout ly;
  db::Cell &a = ly.cell (ly.add_cell ("A"));
  db::Cell &b = ly.cell (ly.add_cell ("B"));
  db::Cell &c = ly.cell (ly.add_cell ("C"));
  db::Cell &d = ly.cell (ly.add_cell ("D"));

  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::ICplxTrans (1.0, 0, false, db::Vector (1, 10)), db::Vector (0, 101), db::Vector (102, 0), 2, 2));
  b.insert (db::CellInstArray (db::CellInst (c.cell_index ()), db::ICplxTrans (1.0, 0, false, db::Vector (2, 3))));

  db::GridReducer red (10);
  db::cell_variants_collector<db::GridReducer> vb (red);
  vb.collect (&ly, a.cell_index ());
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "r0 *1 1,0;r0 *1 3,0;r0 *1 1,1;r0 *1 3,1");

  //  placements are:
  //    b in a: r0 *1 x=1,1+102 y=10,10+101
  //    c in b: r0 *1 x=2,y=3
  //  expanded placements:
  //    c in a: r0 *2 x=1,1+102 y=10,10+101  x  r0 *1 x=2,y=3
  //              = (3,13),(105,13),(3,114),(105,114)
  //  expanded placements mod 10:
  //    c in a: r0 *2 x=1,1+102 y=10,10+101  x  r0 *1 x=2,y=3
  //              = (3,3),(5,3),(3,4),(5,4)
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "r0 *1 -5,3;r0 *1 3,3;r0 *1 -5,4;r0 *1 3,4");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "");

  EXPECT_EQ (inst2str (ly, a), "B:r0 *1 1,10;B:r0 *1 1,111;B:r0 *1 103,10;B:r0 *1 103,111");
  EXPECT_EQ (inst2str (ly, b), "C:r0 *1 2,3");
  EXPECT_EQ (inst2str (ly, c), "");

  std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > vm;
  vb.separate_variants (&vm);
  EXPECT_EQ (vm2str (ly, vm), "B:B[r0 *1 1,0],B$VAR1[r0 *1 3,0],B$VAR2[r0 *1 1,1],B$VAR3[r0 *1 3,1];C:C[r0 *1 -5,3],C$VAR1[r0 *1 3,3],C$VAR2[r0 *1 -5,4],C$VAR3[r0 *1 3,4]");

  EXPECT_EQ (inst2str (ly, a), "B:r0 *1 1,10;B$VAR2:r0 *1 1,111;B$VAR1:r0 *1 103,10;B$VAR3:r0 *1 103,111");
  EXPECT_EQ (inst2str (ly, b), "C$VAR1:r0 *1 2,3");
  EXPECT_EQ (inst2str (ly, ly.cell (ly.cell_by_name ("B$VAR1").second)), "C:r0 *1 2,3");
  EXPECT_EQ (inst2str (ly, ly.cell (ly.cell_by_name ("B$VAR2").second)), "C$VAR3:r0 *1 2,3");
  EXPECT_EQ (inst2str (ly, ly.cell (ly.cell_by_name ("B$VAR3").second)), "C$VAR2:r0 *1 2,3");
  EXPECT_EQ (inst2str (ly, c), "");
}

TEST(9_ComplexGridVariants)
{
  db::Layout ly;
  db::Cell &a = ly.cell (ly.add_cell ("A"));
  db::Cell &b = ly.cell (ly.add_cell ("B"));
  db::Cell &c = ly.cell (ly.add_cell ("C"));
  db::Cell &d = ly.cell (ly.add_cell ("D"));

  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::ICplxTrans (2.0, 0, false, db::Vector (1, 10)), db::Vector (0, 101), db::Vector (102, 0), 2, 2));
  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::ICplxTrans (1.0, 90.0, false, db::Vector (1, 100))));
  b.insert (db::CellInstArray (db::CellInst (c.cell_index ()), db::ICplxTrans (2.0, 0, false, db::Vector (2, 10)), db::Vector (0, 103), db::Vector (105, 0), 2, 2));
  b.insert (db::CellInstArray (db::CellInst (c.cell_index ()), db::ICplxTrans (1.0, 0, true, db::Vector (2, 100))));

  db::GridReducer red (10);
  db::cell_variants_collector<db::GridReducer> vb (red);
  vb.collect (&ly, a.cell_index ());
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "r0 *2 1,0;r90 *1 1,0;r0 *2 3,0;r0 *2 1,1;r0 *2 3,1");

  //  placements are:
  //    b in a: r0 *2 x=1,1+102 y=10,10+101
  //            r90 *1 x=1,y=100
  //    c in b: r0 *2 x=2,2+105 y=10,10+103
  //            m0 *1 x=2,y=100
  //  expanded placements:
  //    c in a: r0 *2 x=1,1+102 y=10,10+101  x  r0 *2 x=2,2+105 y=10,10+103
  //              = (5,30),(215,30),(5,236),(215,236)
  //                (107,30),(317,30),(107,236),(317,236)
  //                (5,131),(215,131),(5,337),(215,337)
  //                (107,131),(317,131),(107,337),(317,337)
  //            r0 *2 x=1,1+102 y=10,10+101  x  m0 *1 x=2,y=100
  //                (5,210),(5,311),(107,210),(107,311)
  //            r90 *1 x=1,y=100  x  r0 *2 x=2,2+105 y=10,10+103
  //                (-9,102),(-9,207),(-112,102),(-112,207)
  //            r90 *1 x=1,y=100  x  m0 *1 x=2,y=100
  //                (-99,102)
  //  expanded ((placements + 5) mod 10) - placements
  //    c in a: r0 *2 x=1,1+102 y=10,10+101  x  r0 *2 x=2,2+105 y=10,10+103
  //              = (5,0),(5,0),(-5,-4),(-5,-4)
  //                (7,0),(7,0),(-3,-4),(-3,-4)
  //                (-5,1),(-5,1),(-5,-3),(-5,-3)
  //                (-3,1),(-3,1),(-3,-3),(-3,-3)
  //            r0 *2 x=1,1+102 y=10,10+101  x  m0 *1 x=2,y=100
  //                (-5,0),(-5,1),(-3,0),(-3,1)
  //            r90 *1 x=1,y=100  x  r0 *2 x=2,2+105 y=10,10+103
  //                (1,2),(1,-3),(-2,2),(-2,-3)
  //            r90 *1 x=1,y=100  x  m0 *1 x=2,y=100
  //                (1,2)
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "r0 *4 -5,-4;r0 *4 -3,-4;r0 *4 -5,-3;r0 *4 -3,-3;r90 *2 -2,-3;"
                                                      "r90 *2 1,-3;m0 *2 -5,0;r0 *4 -5,0;m0 *2 -3,0;r0 *4 -3,0;"
                                                      "m0 *2 -5,1;r0 *4 -5,1;m0 *2 -3,1;r0 *4 -3,1;r90 *2 -2,2;m45 *1 1,2;r90 *2 1,2");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "");
}

TEST(100_OrientationVariantsWithLayout)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/deep_region_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  db::OrientationReducer red;
  db::cell_variants_collector<db::OrientationReducer> vb (red);
  vb.collect (&ly, top_cell.cell_index ());
  vb.separate_variants ();

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testdata () + "/algo/cell_variants_au1.gds");
}

TEST(10_TrivialStatistics)
{
  db::Layout ly;
  db::Cell &a = ly.cell (ly.add_cell ("A"));
  db::Cell &b = ly.cell (ly.add_cell ("B"));
  db::Cell &c = ly.cell (ly.add_cell ("C"));
  db::Cell &d = ly.cell (ly.add_cell ("D"));

  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::Trans (0, false, db::Vector (1, 10))));

  db::OrientationReducer red;
  db::cell_variants_statistics<db::OrientationReducer> vb (red);
  vb.collect (&ly, a.cell_index ());
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0[1]");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "r0 *1 0,0[1]");
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "");
}

TEST(11_TwoVariantsStatistics)
{
  db::Layout ly;
  db::Cell &a = ly.cell (ly.add_cell ("A"));
  db::Cell &b = ly.cell (ly.add_cell ("B"));
  db::Cell &c = ly.cell (ly.add_cell ("C"));
  db::Cell &d = ly.cell (ly.add_cell ("D"));

  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::Trans (0, false, db::Vector (1, 10))));
  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::Trans (0, true, db::Vector (1, 100))));

  db::OrientationReducer red;
  db::cell_variants_statistics<db::OrientationReducer> vb (red);
  vb.collect (&ly, a.cell_index ());
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0[1]");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "m0 *1 0,0[1];r0 *1 0,0[1]");
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "");

  EXPECT_EQ (inst2str (ly, a), "B:r0 *1 1,10;B:m0 *1 1,100");
}

TEST(12_TwoLevelsStatistics)
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
  db::cell_variants_statistics<db::OrientationReducer> vb (red);
  vb.collect (&ly, a.cell_index ());
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0[1]");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "r0 *1 0,0[1];r90 *1 0,0[1]");
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "m0 *1 0,0[1];r0 *1 0,0[1];m45 *1 0,0[1];r90 *1 0,0[1]");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "");

  EXPECT_EQ (inst2str (ly, a), "B:r0 *1 1,10;B:r90 *1 1,100");
  EXPECT_EQ (inst2str (ly, b), "C:r0 *1 2,10;C:m0 *1 2,100");
}

TEST(13_ThreeLevelsStatistics)
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
  c.insert (db::CellInstArray (db::CellInst (d.cell_index ()), db::Trans (1, true, db::Vector (0, 0))));

  db::OrientationReducer red;
  db::cell_variants_statistics<db::OrientationReducer> vb (red);
  vb.collect (&ly, a.cell_index ());
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0[1]");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "r0 *1 0,0[1];r90 *1 0,0[1]");
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "m0 *1 0,0[1];r0 *1 0,0[1];m45 *1 0,0[1];r90 *1 0,0[1]");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "r270 *1 0,0[1];m90 *1 0,0[1];r0 *1 0,0[1];m45 *1 0,0[1]");

  EXPECT_EQ (inst2str (ly, a), "B:r0 *1 1,10;B:r90 *1 1,100");
  EXPECT_EQ (inst2str (ly, b), "C:r0 *1 2,10;C:m0 *1 2,100");
  EXPECT_EQ (inst2str (ly, c), "D:m45 *1 0,0");
}

TEST(14_ComplexTransStatistics)
{
  db::Layout ly;
  db::Cell &a = ly.cell (ly.add_cell ("A"));
  db::Cell &b = ly.cell (ly.add_cell ("B"));
  db::Cell &c = ly.cell (ly.add_cell ("C"));
  db::Cell &d = ly.cell (ly.add_cell ("D"));

  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::ICplxTrans (db::Trans (0, false, db::Vector (1, 10)))));
  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::ICplxTrans (db::Trans (1, false, db::Vector (1, 100)))));
  b.insert (db::CellInstArray (db::CellInst (c.cell_index ()), db::ICplxTrans (db::Trans (0, false, db::Vector (2, 10)))));
  b.insert (db::CellInstArray (db::CellInst (c.cell_index ()), db::ICplxTrans (db::Trans (0, true, db::Vector (2, 100)))));

  db::OrientationReducer red;
  db::cell_variants_statistics<db::OrientationReducer> vb (red);
  vb.collect (&ly, a.cell_index ());
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0[1]");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "r0 *1 0,0[1];r90 *1 0,0[1]");
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "m0 *1 0,0[1];r0 *1 0,0[1];m45 *1 0,0[1];r90 *1 0,0[1]");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "");
}

TEST(15_ArraysStatistics)
{

  db::Layout ly;
  db::Cell &a = ly.cell (ly.add_cell ("A"));
  db::Cell &b = ly.cell (ly.add_cell ("B"));
  db::Cell &c = ly.cell (ly.add_cell ("C"));
  db::Cell &d = ly.cell (ly.add_cell ("D"));

  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::Trans (0, false, db::Vector (1, 10)), db::Vector (0, 100), db::Vector (100, 0), 10, 10));
  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::Trans (1, false, db::Vector (1, 100))));
  b.insert (db::CellInstArray (db::CellInst (c.cell_index ()), db::Trans (0, false, db::Vector (2, 10)), db::Vector (0, 101), db::Vector (101, 0), 10, 10));
  b.insert (db::CellInstArray (db::CellInst (c.cell_index ()), db::Trans (0, true, db::Vector (2, 100))));

  db::OrientationReducer red;
  db::cell_variants_statistics<db::OrientationReducer> vb (red);
  vb.collect (&ly, a.cell_index ());
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0[1]");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "r0 *1 0,0[100];r90 *1 0,0[1]");
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "m0 *1 0,0[100];r0 *1 0,0[10000];m45 *1 0,0[1];r90 *1 0,0[100]");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "");
}

TEST(16_ScalingVariantsStatistics)
{
  db::Layout ly;
  db::Cell &a = ly.cell (ly.add_cell ("A"));
  db::Cell &b = ly.cell (ly.add_cell ("B"));
  db::Cell &c = ly.cell (ly.add_cell ("C"));
  db::Cell &d = ly.cell (ly.add_cell ("D"));

  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::ICplxTrans (1.5, 0, false, db::Vector (1, 10)), db::Vector (0, 100), db::Vector (100, 0), 10, 10));
  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::ICplxTrans (1.0, 90.0, false, db::Vector (1, 100))));
  b.insert (db::CellInstArray (db::CellInst (c.cell_index ()), db::ICplxTrans (2.0, 0, false, db::Vector (2, 10)), db::Vector (0, 101), db::Vector (101, 0), 10, 10));
  b.insert (db::CellInstArray (db::CellInst (c.cell_index ()), db::ICplxTrans (1.0, 0, true, db::Vector (2, 100))));

  db::MagnificationReducer red;
  db::cell_variants_statistics<db::MagnificationReducer> vb (red);
  vb.collect (&ly, a.cell_index ());
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0[1]");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "r0 *1 0,0[1];r0 *1.5 0,0[100]");
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "r0 *1 0,0[1];r0 *1.5 0,0[100];r0 *2 0,0[100];r0 *3 0,0[10000]");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "");
}

TEST(17_GridVariantsStatistics)
{
  db::Layout ly;
  db::Cell &a = ly.cell (ly.add_cell ("A"));
  db::Cell &b = ly.cell (ly.add_cell ("B"));
  db::Cell &c = ly.cell (ly.add_cell ("C"));
  db::Cell &d = ly.cell (ly.add_cell ("D"));

  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::ICplxTrans (1.0, 0, false, db::Vector (1, 10)), db::Vector (0, 101), db::Vector (102, 0), 2, 2));
  b.insert (db::CellInstArray (db::CellInst (c.cell_index ()), db::ICplxTrans (1.0, 0, false, db::Vector (2, 3))));

  db::GridReducer red (10);
  db::cell_variants_statistics<db::GridReducer> vb (red);
  vb.collect (&ly, a.cell_index ());
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0[1]");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "r0 *1 1,0[1];r0 *1 3,0[1];r0 *1 1,1[1];r0 *1 3,1[1]");

  //  placements are:
  //    b in a: r0 *1 x=1,1+102 y=10,10+101
  //    c in b: r0 *1 x=2,y=3
  //  expanded placements:
  //    c in a: r0 *2 x=1,1+102 y=10,10+101  x  r0 *1 x=2,y=3
  //              = (3,13),(105,13),(3,114),(105,114)
  //  expanded placements mod 10:
  //    c in a: r0 *2 x=1,1+102 y=10,10+101  x  r0 *1 x=2,y=3
  //              = (3,3),(5,3),(3,4),(5,4)
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "r0 *1 -5,3[1];r0 *1 3,3[1];r0 *1 -5,4[1];r0 *1 3,4[1]");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "");

  EXPECT_EQ (inst2str (ly, a), "B:r0 *1 1,10;B:r0 *1 1,111;B:r0 *1 103,10;B:r0 *1 103,111");
  EXPECT_EQ (inst2str (ly, b), "C:r0 *1 2,3");
  EXPECT_EQ (inst2str (ly, c), "");
}

TEST(18_ComplexGridVariantsStatistics)
{
  db::Layout ly;
  db::Cell &a = ly.cell (ly.add_cell ("A"));
  db::Cell &b = ly.cell (ly.add_cell ("B"));
  db::Cell &c = ly.cell (ly.add_cell ("C"));
  db::Cell &d = ly.cell (ly.add_cell ("D"));

  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::ICplxTrans (2.0, 0, false, db::Vector (1, 10)), db::Vector (0, 101), db::Vector (102, 0), 2, 2));
  a.insert (db::CellInstArray (db::CellInst (b.cell_index ()), db::ICplxTrans (1.0, 90.0, false, db::Vector (1, 100))));
  b.insert (db::CellInstArray (db::CellInst (c.cell_index ()), db::ICplxTrans (2.0, 0, false, db::Vector (2, 10)), db::Vector (0, 103), db::Vector (105, 0), 2, 2));
  b.insert (db::CellInstArray (db::CellInst (c.cell_index ()), db::ICplxTrans (1.0, 0, true, db::Vector (2, 100))));

  db::GridReducer red (10);
  db::cell_variants_statistics<db::GridReducer> vb (red);
  vb.collect (&ly, a.cell_index ());
  EXPECT_EQ (var2str (vb.variants (a.cell_index ())), "r0 *1 0,0[1]");
  EXPECT_EQ (var2str (vb.variants (b.cell_index ())), "r0 *2 1,0[1];r90 *1 1,0[1];r0 *2 3,0[1];r0 *2 1,1[1];r0 *2 3,1[1]");

  //  placements are:
  //    b in a: r0 *2 x=1,1+102 y=10,10+101
  //            r90 *1 x=1,y=100
  //    c in b: r0 *2 x=2,2+105 y=10,10+103
  //            m0 *1 x=2,y=100
  //  expanded placements:
  //    c in a: r0 *2 x=1,1+102 y=10,10+101  x  r0 *2 x=2,2+105 y=10,10+103
  //              = (5,30),(215,30),(5,236),(215,236)
  //                (107,30),(317,30),(107,236),(317,236)
  //                (5,131),(215,131),(5,337),(215,337)
  //                (107,131),(317,131),(107,337),(317,337)
  //            r0 *2 x=1,1+102 y=10,10+101  x  m0 *1 x=2,y=100
  //                (5,210),(5,311),(107,210),(107,311)
  //            r90 *1 x=1,y=100  x  r0 *2 x=2,2+105 y=10,10+103
  //                (-9,102),(-9,207),(-112,102),(-112,207)
  //            r90 *1 x=1,y=100  x  m0 *1 x=2,y=100
  //                (-99,102)
  //  expanded ((placements + 5) mod 10) - placements
  //    c in a: r0 *2 x=1,1+102 y=10,10+101  x  r0 *2 x=2,2+105 y=10,10+103
  //              = (5,0),(5,0),(-5,-4),(-5,-4)
  //                (7,0),(7,0),(-3,-4),(-3,-4)
  //                (-5,1),(-5,1),(-5,-3),(-5,-3)
  //                (-3,1),(-3,1),(-3,-3),(-3,-3)
  //            r0 *2 x=1,1+102 y=10,10+101  x  m0 *1 x=2,y=100
  //                (-5,0),(-5,1),(-3,0),(-3,1)
  //            r90 *1 x=1,y=100  x  r0 *2 x=2,2+105 y=10,10+103
  //                (1,2),(1,-3),(-2,2),(-2,-3)
  //            r90 *1 x=1,y=100  x  m0 *1 x=2,y=100
  //                (1,2)
  EXPECT_EQ (var2str (vb.variants (c.cell_index ())), "r0 *4 -5,-4[2];r0 *4 -3,-4[2];r0 *4 -5,-3[2];r0 *4 -3,-3[2];r90 *2 -2,-3[1];"
                                                      "r90 *2 1,-3[1];m0 *2 -5,0[1];r0 *4 -5,0[2];m0 *2 -3,0[1];r0 *4 -3,0[2];"
                                                      "m0 *2 -5,1[1];r0 *4 -5,1[2];m0 *2 -3,1[1];r0 *4 -3,1[2];r90 *2 -2,2[1];m45 *1 1,2[1];r90 *2 1,2[1]");
  EXPECT_EQ (var2str (vb.variants (d.cell_index ())), "");
}

TEST(101_Propagation)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/cell_variants_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  std::map<db::cell_index_type, std::map<db::ICplxTrans, db::Shapes> > to_commit;

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::Cell &top_cell = ly.cell (top_cell_index);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.insert_layer (db::LayerProperties (2, 0));

  db::cell_variants_collector<db::MagnificationAndOrientationReducer> vb;
  vb.collect (&ly, top_cell.cell_index ());

  for (db::Layout::const_iterator c = ly.begin (); c != ly.end (); ++c) {

    const std::set<db::ICplxTrans> &vv = vb.variants (c->cell_index ());
    for (auto v = vv.begin (); v != vv.end (); ++v) {

      db::Shapes &out = to_commit [c->cell_index ()][*v];
      for (db::Shapes::shape_iterator s = c->shapes (l1).begin (db::ShapeIterator::All); ! s.at_end (); ++s) {
        db::Box b = s->bbox ().transformed (*v);
        b.enlarge (db::Vector (-100, 0));
        out.insert (b.transformed (v->inverted ()));
      }

    }

  }

  vb.commit_shapes (l2, to_commit);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testdata () + "/algo/cell_variants_au2.gds");
}
