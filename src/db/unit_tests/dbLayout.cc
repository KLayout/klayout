
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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
#include "tlUnitTest.h"

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
  EXPECT_EQ (m, (unsigned int) 0x04231); // c1,c5,c3,c4,c2

  //  check relation information ..
  db::Cell::child_cell_iterator ch;
  db::Cell::const_iterator chi;
  db::Cell::parent_cell_iterator pa;
  db::Cell::parent_inst_iterator pai;

  //  .. for c1
  EXPECT_EQ (c1.child_cells (), size_t (0));
  ch = c1.begin_child_cells ();
  EXPECT_EQ (ch.at_end (), true);
  chi = c1.begin ();
  EXPECT_EQ (chi.at_end (), true);
  EXPECT_EQ (c1.parent_cells (), size_t (3));
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
  EXPECT_EQ (c2.child_cells (), size_t (3));
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
  EXPECT_EQ (c2.parent_cells (), size_t (0));
  pa = c2.begin_parent_cells ();
  EXPECT_EQ (pa == c2.end_parent_cells (), true);
  pai = c2.begin_parent_insts ();
  EXPECT_EQ (pai.at_end (), true);

  //  .. for c3,c4,c5
  EXPECT_EQ (c3.child_cells (), size_t (1));
  EXPECT_EQ (c3.parent_cells (), size_t (1));
  EXPECT_EQ (c4.child_cells (), size_t (2));
  EXPECT_EQ (c4.parent_cells (), size_t (1));
  EXPECT_EQ (c5.child_cells (), size_t (1));
  EXPECT_EQ (c5.parent_cells (), size_t (2));

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

namespace
{

struct EventListener
  : public tl::Object
{
  EventListener ()
    : flags (0),
      bboxes_dirty (false),
      bboxes_all_dirty (false),
      hier_dirty (false),
      dbu_dirty (false),
      cell_name_dirty (false),
      property_ids_dirty (false),
      layer_properties_dirty (false)
  { }

  void reset () { *this = EventListener (); }

  void bboxes_changed (unsigned int i)
  {
    if (i < 31) {
      flags |= (1 << i);
    } else {
      bboxes_all_dirty = true;
    }
  }

  void bboxes_any_changed () { bboxes_dirty = true; }
  void hier_changed () { hier_dirty = true; }
  void dbu_changed () { dbu_dirty = true; }
  void cell_name_changed () { cell_name_dirty = true; }
  void property_ids_changed () { property_ids_dirty = true; }
  void layer_properties_changed () { layer_properties_dirty = true; }

  unsigned int flags;
  bool bboxes_dirty, bboxes_all_dirty, hier_dirty;
  bool dbu_dirty, cell_name_dirty, property_ids_dirty, layer_properties_dirty;
};

}

TEST(2)
{
  //  LayoutStateModel hierarchy events

  db::Layout g;
  EventListener el;

  g.hier_changed_event.add (&el, &EventListener::hier_changed);
  g.bboxes_changed_any_event.add (&el, &EventListener::bboxes_any_changed);
  g.bboxes_changed_event.add (&el, &EventListener::bboxes_changed);

  db::cell_index_type ci;
  db::Cell *top;

  ci = g.add_cell ("TOP");

  EXPECT_EQ (el.flags, (unsigned int) 0);
  EXPECT_EQ (el.bboxes_dirty, false);
  EXPECT_EQ (el.bboxes_all_dirty, false);
  EXPECT_EQ (el.hier_dirty, true);

  el.reset ();
  top = &g.cell (ci);
  ci = g.add_cell ("A");

  EXPECT_EQ (el.flags, (unsigned int) 0);
  EXPECT_EQ (el.bboxes_dirty, false);
  EXPECT_EQ (el.bboxes_all_dirty, false);
  EXPECT_EQ (el.hier_dirty, false);  //  needs g.update() before being issues again

  el.reset ();
  top->insert (db::CellInstArray (ci, db::Trans ()));

  EXPECT_EQ (el.flags, (unsigned int) 0);
  EXPECT_EQ (el.bboxes_dirty, true);
  EXPECT_EQ (el.bboxes_all_dirty, true);
  EXPECT_EQ (el.hier_dirty, false);  //  needs g.update() before being issues again

  g.clear ();
  g.update ();
  el.reset ();

  ci = g.add_cell ("TOP");

  EXPECT_EQ (el.flags, (unsigned int) 0);
  EXPECT_EQ (el.bboxes_dirty, false);
  EXPECT_EQ (el.bboxes_all_dirty, false);
  EXPECT_EQ (el.hier_dirty, true);

  el.reset ();
  g.update ();
  top = &g.cell (ci);
  ci = g.add_cell ("A");

  EXPECT_EQ (el.flags, (unsigned int) 0);
  EXPECT_EQ (el.bboxes_dirty, false);
  EXPECT_EQ (el.bboxes_all_dirty, false);
  EXPECT_EQ (el.hier_dirty, true);  //  OK - see above

  el.reset ();
  g.update ();
  top->insert (db::CellInstArray (ci, db::Trans ()));

  EXPECT_EQ (el.flags, (unsigned int) 0);
  EXPECT_EQ (el.bboxes_dirty, true);
  EXPECT_EQ (el.bboxes_all_dirty, true);
  EXPECT_EQ (el.hier_dirty, true);  //  OK - see above

  //  busy mode will make events issued always
  g.clear ();
  g.set_busy (true);
  el.reset ();

  ci = g.add_cell ("TOP");

  EXPECT_EQ (el.flags, (unsigned int) 0);
  EXPECT_EQ (el.bboxes_dirty, false);
  EXPECT_EQ (el.bboxes_all_dirty, false);
  EXPECT_EQ (el.hier_dirty, true);

  el.reset ();
  top = &g.cell (ci);
  ci = g.add_cell ("A");

  EXPECT_EQ (el.flags, (unsigned int) 0);
  EXPECT_EQ (el.bboxes_dirty, false);
  EXPECT_EQ (el.bboxes_all_dirty, false);
  EXPECT_EQ (el.hier_dirty, true);  //  OK - see above

  el.reset ();
  top->insert (db::CellInstArray (ci, db::Trans ()));

  EXPECT_EQ (el.flags, (unsigned int) 0);
  EXPECT_EQ (el.bboxes_dirty, true);
  EXPECT_EQ (el.bboxes_all_dirty, true);
  EXPECT_EQ (el.hier_dirty, true);  //  OK - see above

}

TEST(3)
{
  //  LayoutStateModel bbox events

  db::Layout g;
  EventListener el;

  g.insert_layer (0);
  g.insert_layer (1);

  g.hier_changed_event.add (&el, &EventListener::hier_changed);
  g.bboxes_changed_any_event.add (&el, &EventListener::bboxes_any_changed);
  g.bboxes_changed_event.add (&el, &EventListener::bboxes_changed);

  db::cell_index_type ci;
  db::Cell *top;

  ci = g.add_cell ("TOP");
  top = &g.cell (ci);

  EXPECT_EQ (el.flags, (unsigned int) 0);
  EXPECT_EQ (el.bboxes_dirty, false);
  EXPECT_EQ (el.bboxes_all_dirty, false);
  EXPECT_EQ (el.hier_dirty, true);

  el.reset ();
  g.update ();

  top->shapes (0).insert (db::Box (0, 0, 10, 20));
  top->shapes (1).insert (db::Box (0, 0, 10, 20));

  EXPECT_EQ (el.flags, (unsigned int) 3);
  EXPECT_EQ (el.bboxes_dirty, true);
  EXPECT_EQ (el.bboxes_all_dirty, false);
  EXPECT_EQ (el.hier_dirty, false);

  el.reset ();

  top->shapes (0).insert (db::Box (0, 0, 10, 20));

  EXPECT_EQ (el.flags, (unsigned int) 0);  //  g.update () is missing -> no new events
  EXPECT_EQ (el.bboxes_dirty, false);   //  g.update () is missing -> no new events
  EXPECT_EQ (el.bboxes_all_dirty, false);
  EXPECT_EQ (el.hier_dirty, false);

  el.reset ();
  g.update ();

  top->shapes (0).insert (db::Box (0, 0, 10, 20));

  EXPECT_EQ (el.flags, (unsigned int) 1);  //  voila
  EXPECT_EQ (el.bboxes_dirty, true);   //  :-)
  EXPECT_EQ (el.bboxes_all_dirty, false);
  EXPECT_EQ (el.hier_dirty, false);

  top->shapes (1).insert (db::Box (0, 0, 10, 20));

  EXPECT_EQ (el.flags, (unsigned int) 3);  //  and yet another one
  EXPECT_EQ (el.bboxes_dirty, true);
  EXPECT_EQ (el.bboxes_all_dirty, false);
  EXPECT_EQ (el.hier_dirty, false);
}

TEST(4)
{
  //  Other events

  db::Layout g;
  EventListener el;

  g.insert_layer (0);
  g.insert_layer (1);
  db::cell_index_type top = g.add_cell ("TOP");

  g.dbu_changed_event.add (&el, &EventListener::dbu_changed);
  g.cell_name_changed_event.add (&el, &EventListener::cell_name_changed);
  g.prop_ids_changed_event.add (&el, &EventListener::property_ids_changed);
  g.layer_properties_changed_event.add (&el, &EventListener::layer_properties_changed);

  EXPECT_EQ (el.dbu_dirty, false);
  EXPECT_EQ (el.cell_name_dirty, false);
  EXPECT_EQ (el.property_ids_dirty, false);
  EXPECT_EQ (el.layer_properties_dirty, false);

  g.set_properties (0, db::LayerProperties (1, 0));
  EXPECT_EQ (el.layer_properties_dirty, true);
  el.reset ();
  g.set_properties (0, db::LayerProperties (1, 0));
  EXPECT_EQ (el.layer_properties_dirty, false);  //  no change
  g.set_properties (0, db::LayerProperties (1, 1));
  EXPECT_EQ (el.layer_properties_dirty, true);  //  but this is

  g.dbu (1.0);
  EXPECT_EQ (el.dbu_dirty, true);
  el.reset ();
  g.dbu (1.0);
  EXPECT_EQ (el.dbu_dirty, false);  //  no change
  g.dbu (0.5);
  EXPECT_EQ (el.dbu_dirty, true);  //  but this is

  g.rename_cell (top, "TIP");
  EXPECT_EQ (el.cell_name_dirty, true);
  el.reset ();
  g.rename_cell (top, "TIP");
  EXPECT_EQ (el.cell_name_dirty, false);  //  no change
  g.rename_cell (top, "TAP");
  EXPECT_EQ (el.cell_name_dirty, true);  //  but this is

  db::properties_id_type prop_id;

  db::PropertiesRepository::properties_set ps;
  ps.insert (std::make_pair (g.properties_repository ().prop_name_id (tl::Variant (1)), tl::Variant ("XYZ")));
  prop_id = g.properties_repository ().properties_id (ps);
  EXPECT_EQ (el.property_ids_dirty, true);
  el.reset ();

  ps.clear ();
  ps.insert (std::make_pair (g.properties_repository ().prop_name_id (tl::Variant (1)), tl::Variant ("XXX")));
  prop_id = g.properties_repository ().properties_id (ps);
  EXPECT_EQ (el.property_ids_dirty, true);
}
