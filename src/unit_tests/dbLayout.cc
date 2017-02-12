
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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
#include "tlString.h"
#include "utHead.h"

std::string set2string (const std::set<db::cell_index_type> &set)
{
  std::string r;
  for (std::set<db::cell_index_type>::const_iterator s = set.begin (); s != set.end (); ++s) {
    if (s != set.begin ()) {
      r += ",";
    }
    r += tl::to_string (*s);
  }
  return r;
}


TEST(1) 
{
  db::Layout g;
  EXPECT_EQ (g.end_top_cells () - g.begin_top_down (), 0);
  db::Cell &c1 (g.cell (g.add_cell ()));
  EXPECT_EQ (g.end_top_cells () - g.begin_top_down (), 1);
  db::Cell &c2 (g.cell (g.add_cell ()));
  db::Cell &c3 (g.cell (g.add_cell ()));
  db::Cell &c4 (g.cell (g.add_cell ()));
  db::Cell &c5 (g.cell (g.add_cell ()));
  EXPECT_EQ (g.end_top_cells () - g.begin_top_down (), 5);

  db::FTrans f (1, true);
  db::Vector p (-10, 20);
  db::Trans t (f.rot (), p);
  db::Vector pp (10, -20);
  db::Trans tt (0, pp);

  //  c4->c1
  c4.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  //  c5->c1
  c5.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  //  c3->c5
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), tt));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), t));
  //  c4->c3
  c4.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c3.cell_index ()), t));
  //  c4->c1
  c4.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), tt));
  //  c2->c1
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), tt));
  //  c2->c4
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c4.cell_index ()), t));
  //  c2->c5
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), tt));
  EXPECT_EQ (g.end_top_down () - g.begin_top_down (), 5);
  EXPECT_EQ (g.end_top_cells () - g.begin_top_down (), 1);

  unsigned int m;
  m = 0;
  for (db::Layout::bottom_up_iterator c = g.begin_bottom_up (); c != g.end_bottom_up (); ++c) {
    m = (m << 4) + *c;
  }
  EXPECT_EQ (m, 0x04231); // c1,c5,c3,c4,c2

  //  check relation informations ..
  db::Cell::child_cell_iterator ch;
  db::Cell::const_iterator chi;
  db::Cell::parent_cell_iterator pa;
  db::Cell::parent_inst_iterator pai;

  //  .. for c1
  EXPECT_EQ (c1.child_cells (), 0);
  ch = c1.begin_child_cells ();
  EXPECT_EQ (ch.at_end (), true);
  chi = c1.begin ();
  EXPECT_EQ (chi.at_end (), true);
  EXPECT_EQ (c1.parent_cells (), 3);
  pa = c1.begin_parent_cells ();
  EXPECT_EQ (*pa, c2.cell_index ());
  ++pa;
  EXPECT_EQ (*pa, c4.cell_index ());
  ++pa;
  EXPECT_EQ (*pa, c5.cell_index ());
  ++pa;
  EXPECT_EQ (pa == c1.end_parent_cells (), true);
  pai = c1.begin_parent_insts ();
  EXPECT_EQ ((*pai).parent_cell_index (), c2.cell_index ());
  EXPECT_EQ ((*pai).child_inst ().front (), tt);
  ++pai;
  EXPECT_EQ ((*pai).parent_cell_index (), c2.cell_index ());
  EXPECT_EQ ((*pai).child_inst ().front (), t);
  ++pai;
  EXPECT_EQ (pai->parent_cell_index (), c4.cell_index ());
  EXPECT_EQ (pai->child_inst ().front (), tt);
  ++pai;
  EXPECT_EQ (pai->parent_cell_index (), c4.cell_index ());
  EXPECT_EQ (pai->child_inst ().front (), t);
  ++pai;
  EXPECT_EQ (pai->parent_cell_index (), c5.cell_index ());
  EXPECT_EQ (pai->child_inst ().front (), t);
  ++pai;
  EXPECT_EQ (pai.at_end (), true);

  //  .. for c2
  ch = c2.begin_child_cells ();
  EXPECT_EQ (c2.child_cells (), 3);
  EXPECT_EQ (*ch, c1.cell_index ());
  ++ch;
  EXPECT_EQ (*ch, c4.cell_index ());
  ++ch;
  EXPECT_EQ (*ch, c5.cell_index ());
  ++ch;
  EXPECT_EQ (ch.at_end (), true);
  chi = c2.begin ();
  EXPECT_EQ (chi->cell_index (), c1.cell_index ());
  EXPECT_EQ (chi->front (), t);
  ++chi;
  EXPECT_EQ ((*chi).cell_index (), c1.cell_index ());
  EXPECT_EQ ((*chi).front (), tt);
  ++chi;
  EXPECT_EQ ((*chi).cell_index (), c4.cell_index ());
  EXPECT_EQ ((*chi).front (), t);
  ++chi;
  EXPECT_EQ ((*chi).cell_index (), c5.cell_index ());
  EXPECT_EQ ((*chi).front (), t);
  ++chi;
  EXPECT_EQ (chi->cell_index (), c5.cell_index ());
  EXPECT_EQ (chi->front (), tt);
  ++chi;
  EXPECT_EQ (chi.at_end (), true);
  // ...
  EXPECT_EQ (c2.parent_cells (), 0);
  pa = c2.begin_parent_cells ();
  EXPECT_EQ (pa == c2.end_parent_cells (), true);
  pai = c2.begin_parent_insts ();
  EXPECT_EQ (pai.at_end (), true);

  //  .. for c3,c4,c5
  EXPECT_EQ (c3.child_cells (), 1);
  EXPECT_EQ (c3.parent_cells (), 1);
  EXPECT_EQ (c4.child_cells (), 2);
  EXPECT_EQ (c4.parent_cells (), 1);
  EXPECT_EQ (c5.child_cells (), 1);
  EXPECT_EQ (c5.parent_cells (), 2);

  //  get some called cell sets
  std::set<db::cell_index_type> cc;
  cc.clear ();
  c3.collect_called_cells (cc);
  EXPECT_EQ (set2string (cc), "0,4");
  cc.clear ();
  c2.collect_called_cells (cc);
  EXPECT_EQ (set2string (cc), "0,2,3,4");
  cc.clear ();
  c5.collect_called_cells (cc);
  EXPECT_EQ (set2string (cc), "0");
  cc.clear ();
  c1.collect_called_cells (cc);
  EXPECT_EQ (set2string (cc), "");

  cc.clear ();
  c3.collect_called_cells (cc);
  EXPECT_EQ (set2string (cc), "0,4");
  c2.collect_called_cells (cc);
  EXPECT_EQ (set2string (cc), "0,2,3,4");
  c5.collect_called_cells (cc);
  EXPECT_EQ (set2string (cc), "0,2,3,4");
  
  //  detect recursive graphs ..
  bool ex = false;
  try {
    c5.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c2.cell_index ()), t));
    g.update ();
  } catch (tl::InternalException &) {
    ex = true;
  }
  EXPECT_EQ (ex, true);
}

