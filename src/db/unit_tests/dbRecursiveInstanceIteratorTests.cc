
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


#include "dbRecursiveInstanceIterator.h"
#include "dbRegion.h"
#include "dbLayoutDiff.h"
#include "dbReader.h"
#include "tlString.h"
#include "tlUnitTest.h"
#include "tlStream.h"

#include <vector>

std::string collect (db::RecursiveInstanceIterator &s, const db::Layout &layout)
{
  std::string res;
  while (! s.at_end ()) {
    if (! res.empty ()) {
      res += "/";
    }
    if (s.cell ()) {
      res += std::string ("[") + layout.cell_name (s.cell ()->cell_index ()) + "]";
    } else {
      res += "[]";
    }
    res += s->inst_ptr.to_string (true);
    ++s;
  }
  return res;
}

std::string collect_with_copy (db::RecursiveInstanceIterator s, const db::Layout &layout)
{
  s.reset ();
  return collect (s, layout);
}

std::string collect2 (db::RecursiveInstanceIterator &s, const db::Layout &layout)
{
  std::string res;
  while (! s.at_end ()) {
    if (! res.empty ()) {
      res += "\n";
    }
    res += std::string (layout.cell_name (s->inst_ptr.cell_index ())) + "@" + (s.trans () * s.instance ().complex_trans ()).to_string ();
    ++s;
  }
  return res;
}

TEST(1)
{
  db::Manager m (true);
  db::Layout g (&m);
  g.insert_layer (0);
  g.insert_layer (1);
  g.insert_layer (2);

  db::Cell &c0 (g.cell (g.add_cell ()));

  db::RecursiveInstanceIterator idef;
  EXPECT_EQ (idef.at_end (), true);
  EXPECT_EQ (collect (idef, g), "");
  EXPECT_EQ (collect_with_copy (idef, g), "");

  db::RecursiveInstanceIterator i00 (g, c0, db::Box (0, 0, 100, 100));
  EXPECT_EQ (collect (i00, g), "");
  EXPECT_EQ (collect_with_copy (i00, g), "");

  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));
  db::Cell &c3 (g.cell (g.add_cell ()));

  db::RecursiveInstanceIterator i0 (g, c0, db::Box (0, 0, 100, 100));
  EXPECT_EQ (collect (i0, g), "");
  EXPECT_EQ (collect_with_copy (i0, g), "");

  db::Box b (0, 100, 1000, 1200);
  c0.shapes (0).insert (b);
  c1.shapes (0).insert (b);
  c2.shapes (0).insert (b);
  c3.shapes (0).insert (b);

  c0.shapes (2).insert (b);
  c0.shapes (2).insert (b.moved (db::Vector (50, 50)));

  db::Trans tt;
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), tt));
  c0.insert (db::CellInstArray (db::CellInst (c2.cell_index ()), db::Trans (db::Vector (100, -100))));
  c0.insert (db::CellInstArray (db::CellInst (c3.cell_index ()), db::Trans (1)));
  c2.insert (db::CellInstArray (db::CellInst (c3.cell_index ()), db::Trans (db::Vector (1100, 0))));

  std::string x;

  db::RecursiveInstanceIterator i1 (g, c0, db::Box (0, 0, 100, 100));
  x = collect(i1, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$1]$3 r0 100,-100");
  x = collect_with_copy(i1, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$1]$3 r0 100,-100");

  db::RecursiveInstanceIterator i1_1inf (g, c0, db::Box (0, 0, 100, 100));
  i1_1inf.min_depth(0);
  x = collect(i1_1inf, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$1]$3 r0 100,-100");
  x = collect_with_copy(i1_1inf, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$1]$3 r0 100,-100");

  db::RecursiveInstanceIterator i1_11 (g, c0, db::Box (0, 0, 2000, 100));
  i1_11.min_depth(0);
  i1_11.max_depth(0);
  x = collect(i1_11, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$1]$3 r0 100,-100");
  x = collect_with_copy(i1_11, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$1]$3 r0 100,-100");

  db::RecursiveInstanceIterator i1_12 (g, c0, db::Box (0, 0, 2000, 100));
  i1_12.min_depth(0);
  i1_12.max_depth(1);
  x = collect(i1_12, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$3]$4 r0 1100,0/[$1]$3 r0 100,-100");
  x = collect_with_copy(i1_12, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$3]$4 r0 1100,0/[$1]$3 r0 100,-100");

  db::RecursiveInstanceIterator i1_22 (g, c0, db::Box (0, 0, 2000, 100));
  i1_22.min_depth(1);
  i1_22.max_depth(1);
  x = collect(i1_22, g);
  EXPECT_EQ (x, "[$3]$4 r0 1100,0");
  x = collect_with_copy(i1_22, g);
  EXPECT_EQ (x, "[$3]$4 r0 1100,0");

  db::RecursiveInstanceIterator i1o (g, c0, db::Box (0, 0, 100, 100), true);
  x = collect(i1o, g);
  EXPECT_EQ (x, "");
  x = collect_with_copy(i1o, g);
  EXPECT_EQ (x, "");
  i1o = db::RecursiveInstanceIterator (g, c0, db::Box (0, 0, 100, 101), true);
  x = collect(i1o, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0");
  x = collect_with_copy(i1o, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0");
  i1o = db::RecursiveInstanceIterator (g, c0, db::Box (0, 0, 101, 101), true);
  x = collect(i1o, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$1]$3 r0 100,-100");
  x = collect_with_copy(i1o, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$1]$3 r0 100,-100");

  db::RecursiveInstanceIterator i2 (g, c0, db::Box (-100, 0, 100, 100));
  db::RecursiveInstanceIterator i2c = i2;
  x = collect(i2, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");
  x = collect_with_copy(i2, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");
  x = collect(i2c, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");
  x = collect_with_copy(i2c, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");
  db::RecursiveInstanceIterator i2o (g, c0, db::Box (-100, 0, 100, 100), true);
  x = collect(i2o, g);
  EXPECT_EQ (x, "");
  x = collect_with_copy(i2o, g);
  EXPECT_EQ (x, "");
  i2o = db::RecursiveInstanceIterator (g, c0, db::Box (-101, 0, 101, 101), true);
  x = collect(i2o, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");
  x = collect_with_copy(i2o, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");

  db::Region r;
  r.insert (db::Box (-600, -100, -500, 0));
  r.insert (db::Box (1600, 0, 1700, 100));
  db::RecursiveInstanceIterator i2r (g, c0, r);
  db::RecursiveInstanceIterator i2rc = i2r;
  x = collect(i2r, g);
  EXPECT_EQ (x, "[$3]$4 r0 1100,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");
  x = collect_with_copy(i2r, g);
  EXPECT_EQ (x, "[$3]$4 r0 1100,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");
  x = collect(i2rc, g);
  EXPECT_EQ (x, "[$3]$4 r0 1100,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");
  x = collect_with_copy(i2rc, g);
  EXPECT_EQ (x, "[$3]$4 r0 1100,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");
  db::RecursiveInstanceIterator i2ro (g, c0, r, true);
  x = collect(i2ro, g);
  EXPECT_EQ (x, "[$3]$4 r0 1100,0/[$1]$3 r0 100,-100");
  x = collect_with_copy(i2ro, g);
  EXPECT_EQ (x, "[$3]$4 r0 1100,0/[$1]$3 r0 100,-100");

  db::RecursiveInstanceIterator i4 (g, c0, db::Box (-100, 0, 2000, 100));
  db::RecursiveInstanceIterator i4_copy (g, c0, db::Box (-100, 0, 2000, 100));
  i4.max_depth (0);
  x = collect(i4, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");
  x = collect_with_copy(i4, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");

  EXPECT_EQ (i4 == i4, true);
  EXPECT_EQ (i4 != i4, false);
  EXPECT_EQ (i4 == i4_copy, false);
  EXPECT_EQ (i4 != i4_copy, true);
  i4 = i4_copy;
  EXPECT_EQ (i4 == i4_copy, true);
  EXPECT_EQ (i4 != i4_copy, false);
  i4.max_depth (1);
  x = collect(i4, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$3]$4 r0 1100,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");
  x = collect_with_copy(i4, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$3]$4 r0 1100,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");

  i4 = i4_copy;
  x = collect(i4, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$3]$4 r0 1100,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");
  x = collect_with_copy(i4, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$3]$4 r0 1100,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");

  db::RecursiveInstanceIterator i5 (g, c0, db::Box::world ());
  x = collect(i5, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$3]$4 r0 1100,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");
  x = collect_with_copy(i5, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$3]$4 r0 1100,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");

  std::set<db::cell_index_type> cc;
  db::RecursiveInstanceIterator ii;

  ii = db::RecursiveInstanceIterator (g, c0, db::Box::world ());
  cc.clear ();
  cc.insert (c2.cell_index ());
  ii.unselect_all_cells ();
  ii.select_cells (cc);
  x = collect(ii, g);
  EXPECT_EQ (x, "[$3]$4 r0 1100,0");
  x = collect_with_copy(ii, g);
  EXPECT_EQ (x, "[$3]$4 r0 1100,0");
  ii.reset ();
  x = collect(ii, g);
  EXPECT_EQ (x, "[$3]$4 r0 1100,0");
  x = collect_with_copy(ii, g);
  EXPECT_EQ (x, "[$3]$4 r0 1100,0");

  ii.reset_selection ();
  x = collect(ii, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$3]$4 r0 1100,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");
  x = collect_with_copy(ii, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$3]$4 r0 1100,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");

  ii.reset_selection ();
  cc.clear ();
  cc.insert (c0.cell_index ());
  cc.insert (c2.cell_index ());
  ii.unselect_cells (cc);
  cc.clear ();
  cc.insert (c2.cell_index ());
  ii.select_cells (cc);
  x = collect(ii, g);
  EXPECT_EQ (x, "[$3]$4 r0 1100,0");
  x = collect_with_copy(ii, g);
  EXPECT_EQ (x, "[$3]$4 r0 1100,0");

  ii = db::RecursiveInstanceIterator (g, c0, db::Box::world ());
  ii.unselect_all_cells ();
  cc.clear ();
  cc.insert (c0.cell_index ());
  ii.select_cells (cc);
  x = collect(ii, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");
  x = collect_with_copy(ii, g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$1]$3 r0 100,-100/[$1]$4 r90 0,0");

  db::RecursiveInstanceIterator i1z (g, c0);
  EXPECT_EQ (i1z.all_targets_enabled (), true);
  std::set<db::cell_index_type> ct;
  ct.insert (c3.cell_index ());
  i1z.set_targets (ct);
  EXPECT_EQ (i1z.all_targets_enabled (), false);
  EXPECT_EQ (i1z.targets () == ct, true);
  i1z.enable_all_targets ();
  EXPECT_EQ (i1z.all_targets_enabled (), true);

  i1z.set_targets (ct);
  EXPECT_EQ (i1z.all_targets_enabled (), false);

  x = collect(i1z, g);
  EXPECT_EQ (x, "[$3]$4 r0 1100,0/[$1]$4 r90 0,0");
  x = collect_with_copy(i1z, g);
  EXPECT_EQ (x, "[$3]$4 r0 1100,0/[$1]$4 r90 0,0");
}

static db::Layout boxes2layout (const std::set<db::Box> &boxes)
{
  db::Layout l;
  l.insert_layer(0, db::LayerProperties (1, 0));
  db::Cell &top (l.cell (l.add_cell ()));

  for (std::set<db::Box>::const_iterator b = boxes.begin (); b != boxes.end (); ++b) {
    top.shapes (0).insert (*b);
  }

  return l;
}

namespace {

  class FlatPusher
    : public db::RecursiveInstanceReceiver
  {
  public:
    FlatPusher (std::set<db::Box> *boxes) : mp_boxes (boxes) { }

    void enter_cell (const db::RecursiveInstanceIterator *iter, const db::Cell *cell, const db::Box & /*region*/, const box_tree_type * /*complex_region*/)
    {
      mp_boxes->insert (iter->trans () * cell->bbox ());
    }

  private:
    std::set<db::Box> *mp_boxes;
  };

}

TEST(2)
{
  //  Big fun with cells

  db::Manager m (true);
  db::Layout g (&m);
  g.insert_layer(0);

  db::Cell &c0 (g.cell (g.add_cell ()));
  db::Cell &c1 (g.cell (g.add_cell ()));

  db::Box basic_box (0, 0, 10, 10);
  c1.shapes (0).insert (basic_box);

  std::set<db::Box> boxes;

  for (int i = 0; i < 100000; ++i) {

    int x = rand () % 10000;
    int y = rand () % 10000;

    boxes.insert (basic_box.moved (db::Vector (x, y)));

    c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), db::Trans (db::Vector (x, y))));

  }

  db::Box search_box (2500, 2500, 7500, 7500);

  std::set<db::Box> selected_boxes;
  std::set<db::Box> selected_boxes2;

  for (db::RecursiveInstanceIterator iter = db::RecursiveInstanceIterator (g, c0, search_box, true); !iter.at_end (); ++iter) {
    selected_boxes.insert (iter.trans () * iter->inst_ptr.bbox ());
  }

  for (std::set<db::Box>::const_iterator b = boxes.begin (); b != boxes.end (); ++b) {
    if (search_box.overlaps (*b)) {
      selected_boxes2.insert (*b);
    }
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);

  //  push mode
  {
    selected_boxes.clear ();
    FlatPusher pusher (&selected_boxes);
    db::RecursiveInstanceIterator (g, c0, search_box, true).push (&pusher);
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);

  db::Box search_box2 (500, 500, 1000, 1000);

  selected_boxes.clear ();
  selected_boxes2.clear ();

  db::Region reg;
  reg.insert (search_box);
  reg.insert (search_box2);

  for (db::RecursiveInstanceIterator iter = db::RecursiveInstanceIterator (g, c0, reg, true); !iter.at_end (); ++iter) {
    selected_boxes.insert (iter.trans () * iter->bbox (db::box_convert<db::CellInst> (g)));
  }

  for (std::set<db::Box>::const_iterator b = boxes.begin (); b != boxes.end (); ++b) {
    if (search_box.overlaps (*b) || search_box2.overlaps (*b)) {
      selected_boxes2.insert (*b);
    }
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);

  //  push mode
  {
    selected_boxes.clear ();
    FlatPusher pusher (&selected_boxes);
    db::RecursiveInstanceIterator (g, c0, reg, true).push (&pusher);
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);
}

TEST(3)
{
  //  Big fun with cells - 2 hierarchy levels

  db::Manager m (true);
  db::Layout g (&m);
  g.insert_layer(0);

  db::Cell &c0 (g.cell (g.add_cell ()));
  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));

  db::Box basic_box (0, 0, 10, 10);
  c2.shapes (0).insert (basic_box);
  c1.insert (db::CellInstArray (c2.cell_index (), db::Trans (db::Vector (1, -1))));

  std::set<db::Box> boxes;

  int nboxes = 100000;
  for (int i = 0; i < nboxes; ++i) {

    int x, y;

    do {
      x = rand () % 10000;
      y = rand () % 10000;
    } while (boxes.find (basic_box.moved (db::Vector (x + 1, y - 1))) != boxes.end ());

    boxes.insert (basic_box.moved (db::Vector (x + 1, y - 1)));

    c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), db::Trans (db::Vector (x, y))));

  }

  db::Box search_box (2500, 2500, 7500, 7500);

  std::set<db::Box> selected_boxes;
  std::set<db::Box> selected_boxes2;

  db::RecursiveInstanceIterator iter = db::RecursiveInstanceIterator (g, c0, search_box, true);
  std::set<db::cell_index_type> tc;
  tc.insert (c2.cell_index ());
  iter.set_targets (tc);
  int n = 0;
  for ( ; !iter.at_end (); ++iter) {
    ++n;
    selected_boxes.insert (iter.trans () * iter->bbox (db::box_convert<db::CellInst> (g)));
  }

  int nn = 0;
  for (std::set<db::Box>::const_iterator b = boxes.begin (); b != boxes.end (); ++b) {
    if (search_box.overlaps (*b)) {
      ++nn;
      selected_boxes2.insert (*b);
    }
  }

  EXPECT_EQ (n, nn);

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);

  //  push mode
  {
    selected_boxes.clear ();
    FlatPusher pusher (&selected_boxes);
    db::RecursiveInstanceIterator (g, c0, search_box, true).push (&pusher);
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);

  db::Box search_box2 (500, 500, 1000, 1000);

  selected_boxes.clear ();
  selected_boxes2.clear ();

  db::Region reg;
  reg.insert (search_box);
  reg.insert (search_box2);

  for (db::RecursiveInstanceIterator iter = db::RecursiveInstanceIterator (g, c0, reg, true); !iter.at_end (); ++iter) {
    selected_boxes.insert (iter.trans () * iter->bbox (db::box_convert<db::CellInst> (g)));
  }

  for (std::set<db::Box>::const_iterator b = boxes.begin (); b != boxes.end (); ++b) {
    if (search_box.overlaps (*b) || search_box2.overlaps (*b)) {
      selected_boxes2.insert (*b);
    }
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);

  //  push mode
  {
    selected_boxes.clear ();
    FlatPusher pusher (&selected_boxes);
    db::RecursiveInstanceIterator (g, c0, reg, true).push (&pusher);
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);
}

TEST(4)
{
  //  Big fun with cells - 2 hierarchy levels + touching mode

  db::Manager m (true);
  db::Layout g (&m);
  g.insert_layer(0);

  db::Cell &c0 (g.cell (g.add_cell ()));
  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));

  db::Box basic_box (0, 0, 10, 10);
  c2.shapes (0).insert (basic_box);
  c1.insert (db::CellInstArray (c2.cell_index (), db::Trans (db::Vector (1, -1))));

  std::set<db::Box> boxes;

  int nboxes = 100000;
  for (int i = 0; i < nboxes; ++i) {

    int x, y;

    do {
      x = rand () % 10000;
      y = rand () % 10000;
    } while (boxes.find (basic_box.moved (db::Vector (x + 1, y - 1))) != boxes.end ());

    boxes.insert (basic_box.moved (db::Vector (x + 1, y - 1)));

    c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), db::Trans (db::Vector (x, y))));

  }

  db::Box search_box (2500, 2500, 7500, 7500);

  std::set<db::Box> selected_boxes;
  std::set<db::Box> selected_boxes2;

  db::RecursiveInstanceIterator iter = db::RecursiveInstanceIterator (g, c0, search_box);
  std::set<db::cell_index_type> tc;
  tc.insert (c2.cell_index ());
  iter.set_targets (tc);
  int n = 0;
  for ( ; !iter.at_end (); ++iter) {
    ++n;
    selected_boxes.insert (iter.trans () * iter->bbox (db::box_convert<db::CellInst> (g)));
  }

  int nn = 0;
  for (std::set<db::Box>::const_iterator b = boxes.begin (); b != boxes.end (); ++b) {
    if (search_box.touches (*b)) {
      ++nn;
      selected_boxes2.insert (*b);
    }
  }

  EXPECT_EQ (n, nn);

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);

  //  push mode
  {
    selected_boxes.clear ();
    FlatPusher pusher (&selected_boxes);
    db::RecursiveInstanceIterator (g, c0, search_box).push (&pusher);
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);

  db::Box search_box2 (500, 500, 1000, 1000);

  selected_boxes.clear ();
  selected_boxes2.clear ();

  db::Region reg;
  reg.insert (search_box);
  reg.insert (search_box2);

  for (db::RecursiveInstanceIterator iter = db::RecursiveInstanceIterator (g, c0, reg); !iter.at_end (); ++iter) {
    selected_boxes.insert (iter.trans () * iter->bbox (db::box_convert<db::CellInst> (g)));
  }

  for (std::set<db::Box>::const_iterator b = boxes.begin (); b != boxes.end (); ++b) {
    if (search_box.touches (*b) || search_box2.touches (*b)) {
      selected_boxes2.insert (*b);
    }
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);

  //  push mode
  {
    selected_boxes.clear ();
    FlatPusher pusher (&selected_boxes);
    db::RecursiveInstanceIterator (g, c0, reg).push (&pusher);
  }

  EXPECT_EQ (selected_boxes.size () > 100, true);
  EXPECT_EQ (db::compare_layouts (boxes2layout (selected_boxes), boxes2layout (selected_boxes2), db::layout_diff::f_verbose, 0, 100 /*max diff lines*/), true);
}

TEST(5)
{
  std::unique_ptr<db::Layout> g (new db::Layout ());
  g->insert_layer (0);
  g->insert_layer (1);
  g->insert_layer (2);

  db::Cell &c0 (g->cell (g->add_cell ()));
  db::Cell &c1 (g->cell (g->add_cell ()));
  db::Cell &c2 (g->cell (g->add_cell ()));
  db::Cell &c3 (g->cell (g->add_cell ()));

  db::Box b (0, 100, 1000, 1200);
  c0.shapes (0).insert (b);
  c1.shapes (0).insert (b);
  c2.shapes (0).insert (b);
  c3.shapes (0).insert (b);

  c0.shapes (2).insert (b);
  c0.shapes (2).insert (b.moved (db::Vector (50, 50)));

  db::Trans tt;
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), tt));
  c0.insert (db::CellInstArray (db::CellInst (c2.cell_index ()), db::Trans (db::Vector (100, -100))));
  c0.insert (db::CellInstArray (db::CellInst (c3.cell_index ()), db::Trans (1)));
  c2.insert (db::CellInstArray (db::CellInst (c3.cell_index ()), db::Trans (db::Vector (1100, 0))));

  std::string x;

  db::RecursiveInstanceIterator i1 (*g, c0, db::Box (0, 0, 100, 100));
  x = collect(i1, *g);
  EXPECT_EQ (x, "[$1]$2 r0 0,0/[$1]$3 r0 100,-100");

  g.reset (new db::Layout ());

  //  now the layout is gone and the iterator stays silent (weak pointer to layout)
  //  NOTE: this only works on reset or re-initialization. Not during iteration.
  i1.reset ();
  x = collect(i1, *g);
  EXPECT_EQ (x, "");
}

//  issue-1353
TEST(6)
{
  db::Layout layout;

  {
    std::string fn (tl::testdata ());
    fn += "/gds/issue-1353.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout);
  }

  std::string x;

  db::cell_index_type c1 = layout.cell_by_name ("TOP_CELL_3_C").second;
  db::cell_index_type c2 = layout.cell_by_name ("TOP_CELL_3_B").second;
  db::RecursiveInstanceIterator i1 (layout, layout.cell (c1));

  x = collect2(i1, layout);
  EXPECT_EQ (x,
    //  depth-first traversal
    "CHILD_CELL_3_1_1@r0 *1 30000,0\n"
    "CHILD_CELL_3_1@r0 *1 30000,0\n"
    "CHILD_CELL_3@r0 *1 30000,0\n"
    "CHILD_CELL_3_1_1@r0 *1 55000,0\n"
    "CHILD_CELL_3_1@r0 *1 55000,0\n"
    "CHILD_CELL_3@r0 *1 55000,0\n"
    "CHILD_CELL_3_1_1@r0 *1 55000,20000\n"
    "CHILD_CELL_3_1@r0 *1 55000,20000\n"
    "CHILD_CELL_3@r0 *1 55000,20000\n"
    "CHILD_CELL_3_1_1@r0 *1 55000,40000\n"
    "CHILD_CELL_3_1@r0 *1 55000,40000\n"
    "CHILD_CELL_3@r0 *1 55000,40000\n"
    "CHILD_CELL_3_1_1@r0 *1 75000,0\n"
    "CHILD_CELL_3_1@r0 *1 75000,0\n"
    "CHILD_CELL_3@r0 *1 75000,0\n"
    "CHILD_CELL_3_1_1@r0 *1 75000,20000\n"
    "CHILD_CELL_3_1@r0 *1 75000,20000\n"
    "CHILD_CELL_3@r0 *1 75000,20000\n"
    "CHILD_CELL_3_1_1@r0 *1 75000,40000\n"
    "CHILD_CELL_3_1@r0 *1 75000,40000\n"
    "CHILD_CELL_3@r0 *1 75000,40000\n"
    "CHILD_CELL_3_1_1@r0 *1 95000,0\n"
    "CHILD_CELL_3_1@r0 *1 95000,0\n"
    "CHILD_CELL_3@r0 *1 95000,0\n"
    "CHILD_CELL_3_1_1@r0 *1 95000,20000\n"
    "CHILD_CELL_3_1@r0 *1 95000,20000\n"
    "CHILD_CELL_3@r0 *1 95000,20000\n"
    "CHILD_CELL_3_1_1@r0 *1 95000,40000\n"
    "CHILD_CELL_3_1@r0 *1 95000,40000\n"
    "CHILD_CELL_3@r0 *1 95000,40000\n"
    "CHILD_CELL_3_1_1@r0 *1 30000,20000\n"
    "CHILD_CELL_3_1@r0 *1 30000,20000\n"
    "CHILD_CELL_3@r0 *1 30000,20000\n"
    "CHILD_CELL_3_1_1@r0 *1 30000,40000\n"
    "CHILD_CELL_3_1@r0 *1 30000,40000\n"
    "CHILD_CELL_3@r0 *1 30000,40000"
  );

  std::set<db::cell_index_type> t;
  t.insert (layout.cell_by_name ("TOP_CELL_3_1_1").second);
  i1.set_targets (t);

  x = collect2(i1, layout);
  EXPECT_EQ (x,
    "CHILD_CELL_3_1_1@r0 *1 30000,0\n"
    "CHILD_CELL_3_1_1@r0 *1 55000,0\n"
    "CHILD_CELL_3_1_1@r0 *1 55000,20000\n"
    "CHILD_CELL_3_1_1@r0 *1 55000,40000\n"
    "CHILD_CELL_3_1_1@r0 *1 75000,0\n"
    "CHILD_CELL_3_1_1@r0 *1 75000,20000\n"
    "CHILD_CELL_3_1_1@r0 *1 75000,40000\n"
    "CHILD_CELL_3_1_1@r0 *1 95000,0\n"
    "CHILD_CELL_3_1_1@r0 *1 95000,20000\n"
    "CHILD_CELL_3_1_1@r0 *1 95000,40000\n"
    "CHILD_CELL_3_1_1@r0 *1 30000,20000\n"
    "CHILD_CELL_3_1_1@r0 *1 30000,40000"
  );

  db::RecursiveInstanceIterator i2 (layout, layout.cell (c2));
  i2.set_targets (t);

  x = collect2(i2, layout);
  EXPECT_EQ (x,
    "CHILD_CELL_3_1_1@r0 *1 30000,0\n"
    "CHILD_CELL_3_1_1@r0 *1 55000,0\n"
    "CHILD_CELL_3_1_1@r0 *1 55000,20000\n"
    "CHILD_CELL_3_1_1@r0 *1 55000,40000\n"
    "CHILD_CELL_3_1_1@r0 *1 75000,0\n"
    "CHILD_CELL_3_1_1@r0 *1 75000,20000\n"
    "CHILD_CELL_3_1_1@r0 *1 75000,40000\n"
    "CHILD_CELL_3_1_1@r0 *1 95000,0\n"
    "CHILD_CELL_3_1_1@r0 *1 95000,20000\n"
    "CHILD_CELL_3_1_1@r0 *1 95000,40000\n"
    "CHILD_CELL_3_1_1@r0 *1 30000,20000\n"
    "CHILD_CELL_3_1_1@r0 *1 30000,40000\n"
    "CHILD_CELL_3_1_1@r0 *1 120000,20000\n"
    "CHILD_CELL_3_1_1@r0 *1 120000,40000\n"
    "CHILD_CELL_3_1_1@r0 *1 120000,0"
  );

  std::set<db::cell_index_type> sc;
  sc.insert (layout.cell_by_name ("CHILD_CELL_3").second);
  i2.unselect_cells (sc);

  x = collect2(i2, layout);
  EXPECT_EQ (x,
    "CHILD_CELL_3_1_1@r0 *1 120000,20000\n"
    "CHILD_CELL_3_1_1@r0 *1 120000,40000\n"
    "CHILD_CELL_3_1_1@r0 *1 120000,0"
  );
}

//  layout locking
TEST(7_LayoutLocking)
{
  db::Layout layout;

  layout.insert_layer (0);

  db::Cell &c0 (layout.cell (layout.add_cell ()));
  db::Cell &c1 (layout.cell (layout.add_cell ()));

  db::Box b (0, 100, 1000, 1200);
  c1.shapes (0).insert (b);

  db::Trans tt;
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), tt));
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), db::Trans (db::Vector (2000, -2000))));

  EXPECT_EQ (layout.under_construction (), false);

  db::RecursiveInstanceIterator iter (layout, c0);

  EXPECT_EQ (layout.under_construction (), false);

  EXPECT_EQ (iter.at_end (), false);
  EXPECT_EQ (layout.under_construction (), true);

  EXPECT_EQ (iter.instance ().to_string (), "cell_index=1 r0 *1 0,0");
  EXPECT_EQ (layout.under_construction (), true);
  ++iter;

  EXPECT_EQ (iter.at_end (), false);

  EXPECT_EQ (iter.instance ().to_string (), "cell_index=1 r0 *1 2000,-2000");
  EXPECT_EQ (layout.under_construction (), true);
  ++iter;

  EXPECT_EQ (layout.under_construction (), false);
  EXPECT_EQ (iter.at_end (), true);

  //  reset will restart
  iter.reset ();

  EXPECT_EQ (layout.under_construction (), false);

  EXPECT_EQ (iter.at_end (), false);
  EXPECT_EQ (layout.under_construction (), true);

  //  a copy will hold the lock
  iter.reset ();

  EXPECT_EQ (layout.under_construction (), false);
  EXPECT_EQ (iter.at_end (), false);

  EXPECT_EQ (layout.under_construction (), true);
  db::RecursiveInstanceIterator iter_copy = iter;

  while (! iter.at_end ()) {
    ++iter;
  }

  EXPECT_EQ (layout.under_construction (), true);
  iter_copy = db::RecursiveInstanceIterator ();

  EXPECT_EQ (layout.under_construction (), false);
}

