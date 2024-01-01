
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



#include "dbLayout.h"
#include "dbCellMapping.h"
#include "tlString.h"
#include "tlUnitTest.h"

std::string nc2s (const std::vector<db::cell_index_type> &nc, const db::Layout &a)
{
  std::string res;
  for (std::vector<db::cell_index_type>::const_iterator i = nc.begin (); i != nc.end (); ++i) {
    if (! res.empty ()) {
      res += ";";
    }
    res += a.cell_name (*i);
  }
  return res;
}

std::string m2s (const db::CellMapping &cm, const db::Layout &a, const db::Layout &b)
{
  std::string res;
  for (db::CellMapping::iterator i = cm.begin (); i != cm.end (); ++i) {
    if (! res.empty ()) {
      res += ";";
    }
    res += a.cell_name (i->second);
    res += "->";
    res += b.cell_name (i->first);
  }
  return res;
}

std::string m2sr (const db::CellMapping &cm, const db::Layout &a, const db::Layout &b)
{
  std::string res;
  for (db::CellMapping::iterator i = cm.begin (); i != cm.end (); ++i) {
    if (! res.empty ()) {
      res += ";";
    }
    res += a.cell_name (i->first);
    res += "->";
    res += b.cell_name (i->second);
  }
  return res;
}

std::string l2s (const db::Layout &a)
{
  std::string res;
  for (db::cell_index_type ci = 0; ci < a.cells (); ++ci) {
    if (a.is_valid_cell_index (ci)) {
      if (! res.empty ()) {
        res += ";";
      }
      res += a.cell_name (ci);
      res += tl::sprintf ("#%d:", int (ci));
      std::string istr;
      for (db::Cell::const_iterator i = a.cell (ci).begin (); ! i.at_end (); ++i) {
        if (! istr.empty ()) {
          istr += ",";
        }
        istr += i->to_string ();
      }
      res += istr;
    }
  }
  return res;
}

TEST(1) 
{
  // some basic example

  db::Layout g;
  db::Cell &a0 (g.cell (g.add_cell ("a0")));
  db::Cell &a1 (g.cell (g.add_cell ("a1")));
  db::Cell &a2 (g.cell (g.add_cell ("a2")));
  db::Cell &a3 (g.cell (g.add_cell ("a3")));
  db::Cell &a4 (g.cell (g.add_cell ("a4")));

  a0.insert (db::CellInstArray (db::CellInst (a1.cell_index ()), db::Trans (), db::Vector(), db::Vector(), 5, 2));
  a0.insert (db::CellInstArray (db::CellInst (a2.cell_index ()), db::Trans ()));
  a4.insert (db::CellInstArray (db::CellInst (a2.cell_index ()), db::Trans (), db::Vector(), db::Vector(), 3, 4));
  a0.insert (db::CellInstArray (db::CellInst (a3.cell_index ()), db::Trans ()));
  a2.insert (db::CellInstArray (db::CellInst (a3.cell_index ()), db::Trans ()));
  a2.insert (db::CellInstArray (db::CellInst (a3.cell_index ()), db::Trans ()));

  db::Layout h;
  db::Cell &b0 (h.cell (h.add_cell ("b0")));
  db::Cell &b1 (h.cell (h.add_cell ("b1")));
  db::Cell &b2 (h.cell (h.add_cell ("b2")));
  db::Cell &b3 (h.cell (h.add_cell ("b3")));
  db::Cell &b4 (h.cell (h.add_cell ("b4")));

  b0.insert (db::CellInstArray (db::CellInst (b1.cell_index ()), db::Trans (), db::Vector(), db::Vector(), 5, 2));
  b0.insert (db::CellInstArray (db::CellInst (b2.cell_index ()), db::Trans ()));
  b4.insert (db::CellInstArray (db::CellInst (b2.cell_index ()), db::Trans (), db::Vector(), db::Vector(), 3, 4));
  b0.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans ()));
  b2.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans ()));
  b2.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans ()));

  db::CellMapping cm;
  cm.create_from_geometry (g, a0.cell_index (), h, b0.cell_index ());
  EXPECT_EQ (m2s (cm, g, h), "a0->b0;a1->b1;a2->b2;a3->b3");
  cm.clear ();
  cm.create_from_geometry (h, b0.cell_index (), g, a0.cell_index ());
  EXPECT_EQ (m2sr (cm, g, h), "a0->b0;a1->b1;a2->b2;a3->b3");
}

TEST(2) 
{
  for (int pass = 0; pass < 3; ++pass) {

    db::Layout g;
    db::Cell &a0 (g.cell (g.add_cell ("a0")));
    // db::Cell &a1 (g.cell (g.add_cell ("a1")));
    // db::Cell &a2 (g.cell (g.add_cell ("a2")));
    // db::Cell &a3 (g.cell (g.add_cell ("a3")));
    db::Cell &a4 (g.cell (g.add_cell ("a4")));

    if (pass == 0) {
      a0.insert (db::CellInstArray (db::CellInst (a4.cell_index ()), db::Trans (1/*r90*/), db::Vector(0, 10), db::Vector(10, 0), 5, 2));
    } else if (pass == 1) {
      a0.insert (db::CellInstArray (db::CellInst (a4.cell_index ()), db::ICplxTrans (1.0, 90.0, false, db::Vector(0, 0)), db::Vector(0, 10), db::Vector(10, 0), 5, 2));
    } else if (pass == 2) {
      a0.insert (db::CellInstArray (db::CellInst (a4.cell_index ()), db::ICplxTrans (0.1, 90.0, false, db::Vector(0, 0)), db::Vector(0, 10), db::Vector(10, 0), 5, 2));
    }

    db::Layout h;
    db::Cell &b0 (h.cell (h.add_cell ("b0")));
    db::Cell &b1 (h.cell (h.add_cell ("b1")));
    db::Cell &b2 (h.cell (h.add_cell ("b2")));
    db::Cell &b3 (h.cell (h.add_cell ("b3")));
    db::Cell &b4 (h.cell (h.add_cell ("b4")));

    if (pass < 2) {
      b0.insert (db::CellInstArray (db::CellInst (b4.cell_index ()), db::Trans (1/*r90*/), db::Vector(0, 10), db::Vector(10, 0), 5, 1));
      b0.insert (db::CellInstArray (db::CellInst (b1.cell_index ()), db::Trans (db::Vector (10, 0))));
      b1.insert (db::CellInstArray (db::CellInst (b2.cell_index ()), db::Trans ()));
      b1.insert (db::CellInstArray (db::CellInst (b2.cell_index ()), db::Trans (db::Vector (0, 20))));
      b1.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 40))));
      b2.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 0))));
      b2.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 10))));
      b3.insert (db::CellInstArray (db::CellInst (b4.cell_index ()), db::Trans (1/*r90*/)));
    } else {
      b0.insert (db::CellInstArray (db::CellInst (b4.cell_index ()), db::ICplxTrans (0.1, 90.0, false, db::Vector(0, 0)), db::Vector(0, 10), db::Vector(10, 0), 5, 1));
      b0.insert (db::CellInstArray (db::CellInst (b1.cell_index ()), db::ICplxTrans (0.1, 0.0, false, db::Vector(10, 0))));
      b1.insert (db::CellInstArray (db::CellInst (b2.cell_index ()), db::Trans ()));
      b1.insert (db::CellInstArray (db::CellInst (b2.cell_index ()), db::Trans (db::Vector (0, 200))));
      b1.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 400))));
      b2.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 0))));
      b2.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 100))));
      b3.insert (db::CellInstArray (db::CellInst (b4.cell_index ()), db::Trans (1/*r90*/)));
    }

    db::CellMapping cm;
    cm.create_from_geometry (g, a0.cell_index (), h, b0.cell_index ());
    EXPECT_EQ (m2s (cm, g, h), "a0->b0;a4->b4");
    cm.clear ();
    cm.create_from_geometry (h, b0.cell_index (), g, a0.cell_index ());
    EXPECT_EQ (m2sr (cm, g, h), "a0->b0;a4->b4");

  }
}

TEST(3) 
{
  for (int order = 0; order < 2; ++order) {

    for (int pass = 0; pass < 4; ++pass) {

      db::Layout g;
      db::Cell &a0 (g.cell (g.add_cell ("a0")));
      // db::Cell &a1 (g.cell (g.add_cell ("a1")));
      // db::Cell &a2 (g.cell (g.add_cell ("a2")));
      // db::Cell &a3 (g.cell (g.add_cell ("a3")));
      db::Cell *a4p, *a5p;
      if (order == 0) {
        a4p = &(g.cell (g.add_cell ("a4")));
        a5p = &(g.cell (g.add_cell ("a5")));
      } else {
        a5p = &(g.cell (g.add_cell ("a5")));
        a4p = &(g.cell (g.add_cell ("a4")));
      }
      db::Cell &a4 (*a4p);
      db::Cell &a5 (*a5p);

      if (pass == 0) {
        a0.insert (db::CellInstArray (db::CellInst (a4.cell_index ()), db::Trans (1/*r90*/), db::Vector(0, 10), db::Vector(10, 0), 5, 2));
        a0.insert (db::CellInstArray (db::CellInst (a5.cell_index ()), db::Trans (1/*r90*/), db::Vector(0, 10), db::Vector(10, 0), 5, 2));
      } else if (pass == 1) {
        a0.insert (db::CellInstArray (db::CellInst (a4.cell_index ()), db::ICplxTrans (1.0, 90.0, false, db::Vector(0, 0)), db::Vector(0, 10), db::Vector(10, 0), 5, 2));
        a0.insert (db::CellInstArray (db::CellInst (a5.cell_index ()), db::Trans (1/*r90*/), db::Vector(0, 10), db::Vector(10, 0), 5, 2));
      } else if (pass == 2) {
        a0.insert (db::CellInstArray (db::CellInst (a4.cell_index ()), db::ICplxTrans (0.1, 90.0, false, db::Vector(0, 0)), db::Vector(0, 10), db::Vector(10, 0), 5, 2));
        a0.insert (db::CellInstArray (db::CellInst (a5.cell_index ()), db::ICplxTrans (0.1, 90.0, false, db::Vector(0, 0)), db::Vector(0, 10), db::Vector(10, 0), 5, 2));
      } else if (pass == 3) {
        a0.insert (db::CellInstArray (db::CellInst (a4.cell_index ()), db::ICplxTrans (0.1, 90.0, false, db::Vector(0, 0)), db::Vector(0, 10), db::Vector(10, 0), 5, 2));
      }

      db::Layout h;
      db::Cell &b0 (h.cell (h.add_cell ("b0")));
      db::Cell &b1 (h.cell (h.add_cell ("b1")));
      db::Cell &b2 (h.cell (h.add_cell ("b2")));
      db::Cell &b3 (h.cell (h.add_cell ("b3")));
      db::Cell &b4 (h.cell (h.add_cell ("b4")));
      db::Cell &b5 (h.cell (h.add_cell ("b5")));

      if (pass < 2) {
        b0.insert (db::CellInstArray (db::CellInst (b4.cell_index ()), db::Trans (1/*r90*/), db::Vector(0, 10), db::Vector(10, 0), 5, 1));
        b0.insert (db::CellInstArray (db::CellInst (b5.cell_index ()), db::Trans (1/*r90*/), db::Vector(0, 10), db::Vector(10, 0), 5, 2));
        b0.insert (db::CellInstArray (db::CellInst (b1.cell_index ()), db::Trans (db::Vector (10, 0))));
        b1.insert (db::CellInstArray (db::CellInst (b2.cell_index ()), db::Trans ()));
        b1.insert (db::CellInstArray (db::CellInst (b2.cell_index ()), db::Trans (db::Vector (0, 20))));
        b1.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 40))));
        b2.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 0))));
        b2.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 10))));
        b3.insert (db::CellInstArray (db::CellInst (b4.cell_index ()), db::Trans (1/*r90*/)));
      } else {
        b0.insert (db::CellInstArray (db::CellInst (b4.cell_index ()), db::ICplxTrans (0.1, 90.0, false, db::Vector(0, 0)), db::Vector(0, 10), db::Vector(10, 0), 5, 1));
        b0.insert (db::CellInstArray (db::CellInst (b5.cell_index ()), db::ICplxTrans (0.1, 90.0, false, db::Vector(0, 0)), db::Vector(0, 10), db::Vector(10, 0), 5, 2));
        b0.insert (db::CellInstArray (db::CellInst (b1.cell_index ()), db::ICplxTrans (0.1, 0.0, false, db::Vector(10, 0))));
        b1.insert (db::CellInstArray (db::CellInst (b2.cell_index ()), db::Trans ()));
        b1.insert (db::CellInstArray (db::CellInst (b2.cell_index ()), db::Trans (db::Vector (0, 200))));
        b1.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 400))));
        b2.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 0))));
        b2.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 100))));
        b3.insert (db::CellInstArray (db::CellInst (b4.cell_index ()), db::Trans (1/*r90*/)));
      }

      db::CellMapping cm;
      cm.create_from_geometry (g, a0.cell_index (), h, b0.cell_index ());
      if (pass < 3) {
        EXPECT_EQ (m2s (cm, g, h), "a0->b0;a4->b4;a5->b5");
      } else {
        EXPECT_EQ (m2s (cm, g, h), "a0->b0;a4->b4");
      }
      cm.clear ();
      cm.create_from_geometry (h, b0.cell_index (), g, a0.cell_index ());
      if (pass < 3) {
        if (order == 1) {
          EXPECT_EQ (m2sr (cm, g, h), "a0->b0;a5->b5;a4->b4");
        } else {
          EXPECT_EQ (m2sr (cm, g, h), "a0->b0;a4->b4;a5->b5");
        }
      } else {
        EXPECT_EQ (m2sr (cm, g, h), "a0->b0;a4->b4");
      }

      db::Layout gg = g;
      std::vector<db::cell_index_type> nc;

      cm.clear ();
      gg = g;
      nc = cm.create_from_geometry_full (gg, a0.cell_index (), h, b0.cell_index ());
      if (pass < 3) {
        EXPECT_EQ (m2s (cm, gg, h), "a0->b0;b1->b1;b2->b2;b3->b3;a4->b4;a5->b5");
        EXPECT_EQ (nc2s (nc, gg), "b1;b2;b3");
      } else {
        EXPECT_EQ (m2s (cm, gg, h), "a0->b0;b1->b1;b2->b2;b3->b3;a4->b4;b5->b5");
        EXPECT_EQ (nc2s (nc, gg), "b1;b2;b3;b5");
      }

      cm.clear ();
      nc = cm.create_from_geometry_full (h, b0.cell_index (), g, a0.cell_index ());
      EXPECT_EQ (nc.size (), size_t (0));
      if (pass < 3) {
        if (order == 1) {
          EXPECT_EQ (m2sr (cm, g, h), "a0->b0;a5->b5;a4->b4");
        } else {
          EXPECT_EQ (m2sr (cm, g, h), "a0->b0;a4->b4;a5->b5");
        }
      } else {
        EXPECT_EQ (m2sr (cm, g, h), "a0->b0;a4->b4");
      }

    }

  }
}

TEST(4) 
{
  db::Layout g;
  db::Cell &a0 (g.cell (g.add_cell ("a0")));
  // db::Cell &a1 (g.cell (g.add_cell ("a1")));
  // db::Cell &a2 (g.cell (g.add_cell ("a2")));
  // db::Cell &a3 (g.cell (g.add_cell ("a3")));
  db::Cell *a4p, *a5p;
  a4p = &(g.cell (g.add_cell ("a4")));
  a5p = &(g.cell (g.add_cell ("a5")));
  db::Cell &a4 (*a4p);
  db::Cell &a5 (*a5p);

  a0.insert (db::CellInstArray (db::CellInst (a4.cell_index ()), db::Trans (1/*r90*/), db::Vector(0, 10), db::Vector(10, 0), 5, 2));
  a0.insert (db::CellInstArray (db::CellInst (a5.cell_index ()), db::Trans (1/*r90*/), db::Vector(0, 10), db::Vector(10, 0), 5, 2));

  db::Layout h;
  db::Cell &b0 (h.cell (h.add_cell ("a0top")));
  db::Cell &b1 (h.cell (h.add_cell ("a1")));
  db::Cell &b2 (h.cell (h.add_cell ("a2")));
  db::Cell &b3 (h.cell (h.add_cell ("a3")));
  db::Cell &b4 (h.cell (h.add_cell ("a4")));
  db::Cell &b5 (h.cell (h.add_cell ("a5")));

  b0.insert (db::CellInstArray (db::CellInst (b4.cell_index ()), db::Trans (1/*r90*/), db::Vector(0, 10), db::Vector(10, 0), 5, 1));
  b0.insert (db::CellInstArray (db::CellInst (b5.cell_index ()), db::Trans (1/*r90*/), db::Vector(0, 10), db::Vector(10, 0), 5, 2));
  b0.insert (db::CellInstArray (db::CellInst (b1.cell_index ()), db::Trans (db::Vector (10, 0))));
  b1.insert (db::CellInstArray (db::CellInst (b2.cell_index ()), db::Trans ()));
  b1.insert (db::CellInstArray (db::CellInst (b2.cell_index ()), db::Trans (db::Vector (0, 20))));
  b1.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 40))));
  b2.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 0))));
  b2.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 10))));
  b3.insert (db::CellInstArray (db::CellInst (b4.cell_index ()), db::Trans (1/*r90*/)));

  db::CellMapping cm;
  cm.create_from_names (g, a0.cell_index (), h, b0.cell_index ());
  EXPECT_EQ (m2s (cm, g, h), "a0->a0top;a4->a4;a5->a5");
  cm.clear ();
  cm.create_from_names (h, b0.cell_index (), g, a0.cell_index ());
  EXPECT_EQ (m2sr (cm, g, h), "a0->a0top;a4->a4;a5->a5");

  std::vector<db::cell_index_type> nc;

  cm.clear ();
  db::Layout gg = g;
  nc = cm.create_from_names_full (gg, a0.cell_index (), h, b0.cell_index ());
  EXPECT_EQ (m2s (cm, gg, h), "a0->a0top;a1->a1;a2->a2;a3->a3;a4->a4;a5->a5");
  EXPECT_EQ (nc2s (nc, gg), "a1;a2;a3");

  cm.clear ();
  db::Layout hh = h;
  nc = cm.create_from_names_full (hh, b0.cell_index (), g, a0.cell_index ());
  EXPECT_EQ (nc.size (), size_t (0));
  EXPECT_EQ (m2sr (cm, g, hh), "a0->a0top;a4->a4;a5->a5");
}

TEST(5) 
{
  db::Layout g;
  db::Cell &a0 (g.cell (g.add_cell ("a0")));
  // db::Cell &a1 (g.cell (g.add_cell ("a1")));
  // db::Cell &a2 (g.cell (g.add_cell ("a2")));
  // db::Cell &a3 (g.cell (g.add_cell ("a3")));
  db::Cell *a4p, *a5p;
  a4p = &(g.cell (g.add_cell ("a4")));
  a5p = &(g.cell (g.add_cell ("a5")));
  db::Cell &a4 (*a4p);
  db::Cell &a5 (*a5p);

  a0.insert (db::CellInstArray (db::CellInst (a4.cell_index ()), db::Trans (1/*r90*/), db::Vector(0, 10), db::Vector(10, 0), 5, 2));
  a0.insert (db::CellInstArray (db::CellInst (a5.cell_index ()), db::Trans (1/*r90*/), db::Vector(0, 10), db::Vector(10, 0), 5, 2));

  db::Layout h;
  db::Cell &b0 (h.cell (h.add_cell ("a0top")));
  db::Cell &b1 (h.cell (h.add_cell ("a1")));
  db::Cell &b2 (h.cell (h.add_cell ("a2")));
  db::Cell &b3 (h.cell (h.add_cell ("a3")));
  db::Cell &b4 (h.cell (h.add_cell ("a4")));
  db::Cell &b5 (h.cell (h.add_cell ("a5")));

  b0.insert (db::CellInstArray (db::CellInst (b4.cell_index ()), db::Trans (1/*r90*/), db::Vector(0, 10), db::Vector(10, 0), 5, 1));
  b0.insert (db::CellInstArray (db::CellInst (b5.cell_index ()), db::Trans (1/*r90*/), db::Vector(0, 10), db::Vector(10, 0), 5, 2));
  b0.insert (db::CellInstArray (db::CellInst (b1.cell_index ()), db::Trans (db::Vector (10, 0))));
  b1.insert (db::CellInstArray (db::CellInst (b2.cell_index ()), db::Trans ()));
  b1.insert (db::CellInstArray (db::CellInst (b2.cell_index ()), db::Trans (db::Vector (0, 20))));
  b1.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 40))));
  b2.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 0))));
  b2.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 10))));
  b3.insert (db::CellInstArray (db::CellInst (b4.cell_index ()), db::Trans (1/*r90*/)));

  db::CellMapping cm;
  cm.create_single_mapping (g, a0.cell_index (), h, b0.cell_index ());
  EXPECT_EQ (m2s (cm, g, h), "a0->a0top");
  cm.clear ();
  cm.create_single_mapping (h, b0.cell_index (), g, a0.cell_index ());
  EXPECT_EQ (m2sr (cm, g, h), "a0->a0top");

  std::vector<db::cell_index_type> nc;

  cm.clear ();
  db::Layout gg = g;
  nc = cm.create_single_mapping_full (gg, a0.cell_index (), h, b0.cell_index ());
  EXPECT_EQ (m2s (cm, gg, h), "a0->a0top;a1->a1;a2->a2;a3->a3;a4$1->a4;a5$1->a5");
  EXPECT_EQ (nc2s (nc, gg), "a1;a2;a3;a4$1;a5$1");

  cm.clear ();
  db::Layout hh = h;
  nc = cm.create_single_mapping_full (hh, b0.cell_index (), g, a0.cell_index ());
  EXPECT_EQ (nc.size (), size_t (2));
  EXPECT_EQ (m2sr (cm, g, hh), "a0->a0top;a4->a4$1;a5->a5$1");

  EXPECT_EQ (l2s (g), "a0#0:cell_index=1 r90 0,0 array=(0,10,10,0 5x2),cell_index=2 r90 0,0 array=(0,10,10,0 5x2);a4#1:;a5#2:");
  EXPECT_EQ (l2s (h), "a0top#0:cell_index=4 r90 0,0 array=(0,10,10,0 5x1),cell_index=5 r90 0,0 array=(0,10,10,0 5x2),cell_index=1 r0 10,0;a1#1:cell_index=2 r0 0,0,cell_index=2 r0 0,20,cell_index=3 r0 0,40;a2#2:cell_index=3 r0 0,0,cell_index=3 r0 0,10;a3#3:cell_index=4 r90 0,0;a4#4:;a5#5:");
  EXPECT_EQ (l2s (hh), "a0top#0:cell_index=4 r90 0,0 array=(0,10,10,0 5x1),cell_index=5 r90 0,0 array=(0,10,10,0 5x2),cell_index=1 r0 10,0,cell_index=6 r90 0,0 array=(0,10,10,0 5x2),cell_index=7 r90 0,0 array=(0,10,10,0 5x2);a1#1:cell_index=2 r0 0,0,cell_index=2 r0 0,20,cell_index=3 r0 0,40;a2#2:cell_index=3 r0 0,0,cell_index=3 r0 0,10;a3#3:cell_index=4 r90 0,0;a4#4:;a5#5:;a4$1#6:;a5$1#7:");

  cm.clear ();
  hh = h;
  hh.dbu (hh.dbu () * 0.5);
  nc = cm.create_single_mapping_full (hh, b0.cell_index (), g, a0.cell_index ());
  EXPECT_EQ (nc.size (), size_t (2));
  EXPECT_EQ (m2sr (cm, g, hh), "a0->a0top;a4->a4$1;a5->a5$1");

  EXPECT_EQ (l2s (hh), "a0top#0:cell_index=4 r90 0,0 array=(0,10,10,0 5x1),cell_index=5 r90 0,0 array=(0,10,10,0 5x2),cell_index=1 r0 10,0,cell_index=6 r90 0,0 array=(0,20,20,0 5x2),cell_index=7 r90 0,0 array=(0,20,20,0 5x2);a1#1:cell_index=2 r0 0,0,cell_index=2 r0 0,20,cell_index=3 r0 0,40;a2#2:cell_index=3 r0 0,0,cell_index=3 r0 0,10;a3#3:cell_index=4 r90 0,0;a4#4:;a5#5:;a4$1#6:;a5$1#7:");
}

//  Resolution of array references
TEST(6)
{
  std::unique_ptr<db::Layout> g (new db::Layout ());
  db::Cell &a0 (g->cell (g->add_cell ("a0")));
  db::Cell *a4p, *a5p;
  a4p = &(g->cell (g->add_cell ("a4")));
  a5p = &(g->cell (g->add_cell ("a5")));
  db::Cell &a4 (*a4p);
  db::Cell &a5 (*a5p);

  a0.insert (db::CellInstArray (db::CellInst (a4.cell_index ()), db::Trans (1/*r90*/), g->array_repository (), db::Vector(0, 10), db::Vector(10, 0), 5, 2));
  a0.insert (db::CellInstArray (db::CellInst (a5.cell_index ()), db::Trans (1/*r90*/), g->array_repository (), db::Vector(0, 10), db::Vector(10, 0), 5, 2));

  db::Layout h;
  db::Cell &b0 (h.cell (h.add_cell ("a0top")));

  db::CellMapping cm;
  cm.create_single_mapping_full (h, b0.cell_index (), *g, a0.cell_index ());
  EXPECT_EQ (m2s (cm, *g, h), "a0->a0top;a4->a4;a5->a5");

  g.reset (0);

  EXPECT_EQ (l2s (h), "a0top#0:cell_index=1 r90 0,0 array=(0,10,10,0 5x2),cell_index=2 r90 0,0 array=(0,10,10,0 5x2);a4#1:;a5#2:");
}

//  Multi-mapping
TEST(7)
{
  std::unique_ptr<db::Layout> g (new db::Layout ());
  db::Cell &a0 (g->cell (g->add_cell ("a0")));
  db::Cell &a1 (g->cell (g->add_cell ("a1")));
  db::Cell &a2 (g->cell (g->add_cell ("a2")));
  db::Cell &a3 (g->cell (g->add_cell ("a3")));
  db::Cell &a4 = g->cell (g->add_cell ("a4"));
  db::Cell &a5 = g->cell (g->add_cell ("a5"));

  a3.insert (db::CellInstArray (db::CellInst (a4.cell_index ()), db::Trans ()));
  a3.insert (db::CellInstArray (db::CellInst (a5.cell_index ()), db::Trans ()));

  a1.insert (db::CellInstArray (db::CellInst (a4.cell_index ()), db::Trans ()));
  a1.insert (db::CellInstArray (db::CellInst (a3.cell_index ()), db::Trans ()));
  a2.insert (db::CellInstArray (db::CellInst (a4.cell_index ()), db::Trans ()));

  db::Layout h;
  db::Cell &b0 (h.cell (h.add_cell ("b0")));
  db::Cell &b1 (h.cell (h.add_cell ("b1")));
  db::Cell &b2 (h.cell (h.add_cell ("b2")));

  db::CellMapping cm;
  std::vector<db::cell_index_type> cib, cia;
  cia.push_back (a0.cell_index ());
  cia.push_back (a1.cell_index ());
  cia.push_back (a2.cell_index ());
  cib.push_back (b0.cell_index ());
  cib.push_back (b1.cell_index ());
  cib.push_back (b2.cell_index ());
  cm.create_multi_mapping_full (h, cib, *g, cia);
  EXPECT_EQ (m2s (cm, *g, h), "a0->b0;a1->b1;a2->b2;a3->a3;a4->a4;a5->a5");

  EXPECT_EQ (l2s (h), "b0#0:;b1#1:cell_index=3 r0 0,0,cell_index=4 r0 0,0;b2#2:cell_index=4 r0 0,0;a3#3:cell_index=4 r0 0,0,cell_index=5 r0 0,0;a4#4:;a5#5:");
}

