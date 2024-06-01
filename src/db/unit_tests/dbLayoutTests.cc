
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
#include "dbLibraryManager.h"
#include "dbLibrary.h"
#include "dbColdProxy.h"
#include "dbLibraryProxy.h"
#include "dbTextWriter.h"
#include "dbCellMapping.h"
#include "dbInstElement.h"
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

  EXPECT_EQ (g.hier_generation_id (), size_t (0));

  ci = g.add_cell ("TOP");

  EXPECT_EQ (el.flags, (unsigned int) 0);
  EXPECT_EQ (el.bboxes_dirty, false);
  EXPECT_EQ (el.bboxes_all_dirty, false);
  EXPECT_EQ (el.hier_dirty, true);
  EXPECT_EQ (g.hier_generation_id (), size_t (1));

  el.reset ();
  top = &g.cell (ci);
  ci = g.add_cell ("A");

  EXPECT_EQ (el.flags, (unsigned int) 0);
  EXPECT_EQ (el.bboxes_dirty, false);
  EXPECT_EQ (el.bboxes_all_dirty, false);
  EXPECT_EQ (el.hier_dirty, false);  //  needs g.update() before being issues again
  EXPECT_EQ (g.hier_generation_id (), size_t (2));

  el.reset ();
  top->insert (db::CellInstArray (ci, db::Trans ()));

  EXPECT_EQ (el.flags, (unsigned int) 0);
  EXPECT_EQ (el.bboxes_dirty, true);
  EXPECT_EQ (el.bboxes_all_dirty, true);
  EXPECT_EQ (el.hier_dirty, false);  //  needs g.update() before being issues again
  EXPECT_EQ (g.hier_generation_id (), size_t (3));

  g.clear ();
  g.update ();
  el.reset ();
  EXPECT_EQ (g.hier_generation_id (), size_t (4));

  ci = g.add_cell ("TOP");

  EXPECT_EQ (el.flags, (unsigned int) 0);
  EXPECT_EQ (el.bboxes_dirty, false);
  EXPECT_EQ (el.bboxes_all_dirty, false);
  EXPECT_EQ (el.hier_dirty, true);
  EXPECT_EQ (g.hier_generation_id (), size_t (5));

  el.reset ();
  g.update ();
  top = &g.cell (ci);
  ci = g.add_cell ("A");

  EXPECT_EQ (el.flags, (unsigned int) 0);
  EXPECT_EQ (el.bboxes_dirty, false);
  EXPECT_EQ (el.bboxes_all_dirty, false);
  EXPECT_EQ (el.hier_dirty, true);  //  OK - see above
  EXPECT_EQ (g.hier_generation_id (), size_t (6));

  el.reset ();
  g.update ();
  top->insert (db::CellInstArray (ci, db::Trans ()));

  EXPECT_EQ (el.flags, (unsigned int) 0);
  EXPECT_EQ (el.bboxes_dirty, true);
  EXPECT_EQ (el.bboxes_all_dirty, true);
  EXPECT_EQ (el.hier_dirty, true);  //  OK - see above
  EXPECT_EQ (g.hier_generation_id (), size_t (7));

  //  busy mode will make events issued always
  g.clear ();
  g.set_busy (true);
  el.reset ();

  ci = g.add_cell ("TOP");

  EXPECT_EQ (el.flags, (unsigned int) 0);
  EXPECT_EQ (el.bboxes_dirty, false);
  EXPECT_EQ (el.bboxes_all_dirty, false);
  EXPECT_EQ (el.hier_dirty, true);
  EXPECT_EQ (g.hier_generation_id (), size_t (9));

  el.reset ();
  top = &g.cell (ci);
  ci = g.add_cell ("A");

  EXPECT_EQ (el.flags, (unsigned int) 0);
  EXPECT_EQ (el.bboxes_dirty, false);
  EXPECT_EQ (el.bboxes_all_dirty, false);
  EXPECT_EQ (el.hier_dirty, true);  //  OK - see above
  EXPECT_EQ (g.hier_generation_id (), size_t (10));

  el.reset ();
  top->insert (db::CellInstArray (ci, db::Trans ()));

  EXPECT_EQ (el.flags, (unsigned int) 0);
  EXPECT_EQ (el.bboxes_dirty, true);
  EXPECT_EQ (el.bboxes_all_dirty, true);
  EXPECT_EQ (el.hier_dirty, true);  //  OK - see above
  EXPECT_EQ (g.hier_generation_id (), size_t (11));

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

  db::PropertiesRepository::properties_set ps;
  ps.insert (std::make_pair (g.properties_repository ().prop_name_id (tl::Variant (1)), tl::Variant ("XYZ")));
  g.properties_repository ().properties_id (ps);
  EXPECT_EQ (el.property_ids_dirty, true);
  el.reset ();

  ps.clear ();
  ps.insert (std::make_pair (g.properties_repository ().prop_name_id (tl::Variant (1)), tl::Variant ("XXX")));
  g.properties_repository ().properties_id (ps);
  EXPECT_EQ (el.property_ids_dirty, true);

  el.layer_properties_dirty = false;
  EXPECT_EQ (g.get_layer_maybe (db::LayerProperties (42, 17)), -1);
  EXPECT_EQ (el.layer_properties_dirty, false);
  g.get_layer (db::LayerProperties (42, 17));
  EXPECT_EQ (el.layer_properties_dirty, true);  //  new layer got inserted
}

static std::string l2s (const db::Layout &layout)
{
  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  return os.string ();
}

TEST(5)
{
  //  Technology management and library substitution

  db::cell_index_type ci;
  unsigned int li;
  db::Cell *cell;

  db::Library *lib_a = new db::Library ();
  lib_a->set_name ("LIB");
  ci = lib_a->layout ().add_cell ("LIBCELL");
  li = lib_a->layout ().insert_layer (db::LayerProperties (1, 0));
  lib_a->layout ().cell (ci).shapes (li).insert (db::Box (0, 0, 100, 200));
  lib_a->add_technology ("A");
  db::LibraryManager::instance ().register_lib (lib_a);

  EXPECT_EQ (db::LibraryManager::instance ().lib_by_name ("LIB", "A").first, true);
  EXPECT_EQ (db::LibraryManager::instance ().lib_by_name ("LIB", "A").second, lib_a->get_id ());
  EXPECT_EQ (db::LibraryManager::instance ().lib_ptr_by_name ("LIB", "A") == lib_a, true);

  db::Library *lib_b = new db::Library ();
  lib_b->set_name ("LIB");
  ci = lib_b->layout ().add_cell ("LIBCELL");
  li = lib_b->layout ().insert_layer (db::LayerProperties (2, 0));
  lib_b->layout ().cell (ci).shapes (li).insert (db::Box (0, 0, 200, 100));
  lib_b->add_technology ("B");
  db::LibraryManager::instance ().register_lib (lib_b);

  EXPECT_EQ (db::LibraryManager::instance ().lib_by_name ("LIB", "B").first, true);
  EXPECT_EQ (db::LibraryManager::instance ().lib_by_name ("LIB", "B").second, lib_b->get_id ());
  EXPECT_EQ (db::LibraryManager::instance ().lib_ptr_by_name ("LIB", "B") == lib_b, true);

  db::Library *lib_c = new db::Library ();
  lib_c->set_name ("LIB");
  ci = lib_c->layout ().add_cell ("LIBCELL2");
  li = lib_c->layout ().insert_layer (db::LayerProperties (2, 0));
  lib_c->layout ().cell (ci).shapes (li).insert (db::Box (0, 0, 200, 100));
  lib_c->add_technology ("C");
  db::LibraryManager::instance ().register_lib (lib_c);

  EXPECT_EQ (db::LibraryManager::instance ().lib_by_name ("LIB", "C").first, true);
  EXPECT_EQ (db::LibraryManager::instance ().lib_by_name ("LIB", "C").second, lib_c->get_id ());
  EXPECT_EQ (db::LibraryManager::instance ().lib_ptr_by_name ("LIB", "C") == lib_c, true);

  db::Manager m;
  db::Layout l (&m);
  EXPECT_EQ (l.technology_name (), "");

  db::LayoutOrCellContextInfo info;
  info.lib_name = "LIB";
  info.cell_name = "LIBCELL";

  cell = l.recover_proxy (info);
  EXPECT_EQ (dynamic_cast<db::ColdProxy *> (cell) != 0, true);
  EXPECT_EQ (cell->get_qualified_name (), "<defunct>LIB.LIBCELL");
  EXPECT_EQ (cell->get_basic_name (), "LIBCELL");
  EXPECT_EQ (cell->get_display_name (), "<defunct>LIB.LIBCELL");

  EXPECT_EQ (l2s (l), "begin_lib 0.001\nbegin_cell {LIBCELL}\nend_cell\nend_lib\n");

  //  now restore the proxies
  l.set_technology_name ("A");
  EXPECT_EQ (l.technology_name (), "A");

  EXPECT_EQ (l2s (l), "begin_lib 0.001\nbegin_cell {LIBCELL}\nbox 1 0 {0 0} {100 200}\nend_cell\nend_lib\n");

  //  now switch to cold proxies again as the technology does not have "LIBCELL" (but rather LIBCELL2)
  l.set_technology_name ("C");
  EXPECT_EQ (l.technology_name (), "C");

  cell = &l.cell (l.cell_by_name ("LIBCELL").second);
  EXPECT_EQ (dynamic_cast<db::ColdProxy *> (cell) != 0, true);
  EXPECT_EQ (cell->get_qualified_name (), "<defunct>LIB.LIBCELL");
  EXPECT_EQ (cell->get_basic_name (), "LIBCELL");
  EXPECT_EQ (cell->get_display_name (), "<defunct>LIB.LIBCELL");

  //  NOTE: the box on 1/0 retained
  EXPECT_EQ (l2s (l), "begin_lib 0.001\nbegin_cell {LIBCELL}\nbox 1 0 {0 0} {100 200}\nend_cell\nend_lib\n");

  //  switch to another LIBCELL, this time using layer 2/0
  if (l.is_editable ()) {
    m.transaction ("switch_to_b");
    l.set_technology_name ("B");
    m.commit ();
  } else {
    l.set_technology_name ("B");
  }

  EXPECT_EQ (l.technology_name (), "B");
  cell = &l.cell (l.cell_by_name ("LIBCELL").second);
  EXPECT_EQ (dynamic_cast<db::LibraryProxy *> (cell) != 0, true);
  EXPECT_EQ (l2s (l), "begin_lib 0.001\nbegin_cell {LIBCELL}\nbox 2 0 {0 0} {200 100}\nend_cell\nend_lib\n");

  if (l.is_editable ()) {

    m.undo ();
    EXPECT_EQ (l.technology_name (), "C");

    cell = &l.cell (l.cell_by_name ("LIBCELL").second);
    EXPECT_EQ (dynamic_cast<db::ColdProxy *> (cell) != 0, true);
    EXPECT_EQ (cell->get_qualified_name (), "<defunct>LIB.LIBCELL");
    EXPECT_EQ (cell->get_basic_name (), "LIBCELL");
    EXPECT_EQ (cell->get_display_name (), "<defunct>LIB.LIBCELL");
    EXPECT_EQ (l2s (l), "begin_lib 0.001\nbegin_cell {LIBCELL}\nbox 1 0 {0 0} {100 200}\nend_cell\nend_lib\n");

    m.redo ();

    EXPECT_EQ (l.technology_name (), "B");
    cell = &l.cell (l.cell_by_name ("LIBCELL").second);
    EXPECT_EQ (dynamic_cast<db::LibraryProxy *> (cell) != 0, true);
    EXPECT_EQ (l2s (l), "begin_lib 0.001\nbegin_cell {LIBCELL}\nbox 2 0 {0 0} {200 100}\nend_cell\nend_lib\n");

  }

  db::LibraryManager::instance ().delete_lib (lib_a);
  db::LibraryManager::instance ().delete_lib (lib_b);
  db::LibraryManager::instance ().delete_lib (lib_c);
}

TEST(6)
{
  //  Cold proxies and context serialization
  db::Cell *cell;

  db::Manager m;
  db::Layout l (&m);

  EXPECT_EQ (l.technology_name (), "");

  db::LayoutOrCellContextInfo info;
  info.lib_name = "Basic";
  info.pcell_name = "CIRCLE";
  info.pcell_parameters ["actual_radius"] = tl::Variant (10.0);
  info.pcell_parameters ["npoints"] = tl::Variant (8);
  info.pcell_parameters ["layer"] = tl::Variant (db::LayerProperties (1, 0));

  if (l.is_editable ()) {
    m.transaction ("import");
  }
  cell = l.recover_proxy (info);
  if (l.is_editable ()) {
    m.commit ();
  }
  EXPECT_EQ (cell->get_qualified_name (), "Basic.CIRCLE");
  EXPECT_EQ (cell->get_basic_name (), "CIRCLE");
  EXPECT_EQ (cell->get_display_name (), "Basic.CIRCLE(l=1/0,r=10,n=8)");

  EXPECT_EQ (l2s (l), "begin_lib 0.001\nbegin_cell {CIRCLE}\nboundary 1 0 {-4142 -10000} {-10000 -4142} {-10000 4142} {-4142 10000} {4142 10000} {10000 4142} {10000 -4142} {4142 -10000} {-4142 -10000}\nend_cell\nend_lib\n");

  db::LayoutOrCellContextInfo info2;
  l.get_context_info (cell->cell_index (), info2);
  info2.pcell_parameters ["actual_radius"] = tl::Variant (5.0);

  if (l.is_editable ()) {
    m.transaction ("modify");
  }
  db::cell_index_type ci = cell->cell_index ();
  l.recover_proxy_as (ci, info2);
  if (l.is_editable ()) {
    m.commit ();
  }
  cell = &l.cell (ci);
  EXPECT_EQ (cell->get_qualified_name (), "Basic.CIRCLE");
  EXPECT_EQ (cell->get_basic_name (), "CIRCLE");
  EXPECT_EQ (cell->get_display_name (), "Basic.CIRCLE(l=1/0,r=5,n=8)");

  EXPECT_EQ (l2s (l), "begin_lib 0.001\nbegin_cell {CIRCLE}\nboundary 1 0 {-2071 -5000} {-5000 -2071} {-5000 2071} {-2071 5000} {2071 5000} {5000 2071} {5000 -2071} {2071 -5000} {-2071 -5000}\nend_cell\nend_lib\n");

  if (l.is_editable ()) {
    m.undo ();
    EXPECT_EQ (l2s (l), "begin_lib 0.001\nbegin_cell {CIRCLE}\nboundary 1 0 {-4142 -10000} {-10000 -4142} {-10000 4142} {-4142 10000} {4142 10000} {10000 4142} {10000 -4142} {4142 -10000} {-4142 -10000}\nend_cell\nend_lib\n");
    m.redo ();
    EXPECT_EQ (l2s (l), "begin_lib 0.001\nbegin_cell {CIRCLE}\nboundary 1 0 {-2071 -5000} {-5000 -2071} {-5000 2071} {-2071 5000} {2071 5000} {5000 2071} {5000 -2071} {2071 -5000} {-2071 -5000}\nend_cell\nend_lib\n");
  }
}

TEST(7_LayerProperties)
{
  db::Manager m;
  db::Layout l (&m);

  EXPECT_EQ (l.is_valid_layer (0), false);
  EXPECT_EQ (l.guiding_shape_layer (), (unsigned int) 0);
  EXPECT_EQ (l.is_special_layer (0), true);
  EXPECT_EQ (int (l.layers ()), 1);

  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (1, 0)), -1);
  unsigned int l1 = l.get_layer (db::LayerProperties (1, 0));
  EXPECT_EQ (1, int (l1));
  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (1, 0)), int (l1));
  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (2, 0)), -1);
  EXPECT_EQ (int (l.layers ()), 2);

  unsigned int l2 = l.get_layer (db::LayerProperties (2, 0));
  EXPECT_EQ (2, int (l2));
  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (1, 0)), int (l1));
  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (2, 0)), int (l2));
  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (3, 0)), -1);
  EXPECT_EQ (int (l.layers ()), 3);

  l.insert_layer (l2, db::LayerProperties (3, 0));
  EXPECT_EQ (int (l.layers ()), 3);
  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (1, 0)), int (l1));
  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (2, 0)), -1);
  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (3, 0)), int (l2));
  EXPECT_EQ (l.get_properties (l2).to_string (), "3/0");

  l.transaction (std::string ());
  l.delete_layer (l2);
  l.commit ();

  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (1, 0)), int (l1));
  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (2, 0)), -1);
  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (3, 0)), -1);

  m.undo ();
  EXPECT_EQ (int (l.layers ()), 3);
  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (1, 0)), int (l1));
  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (2, 0)), -1);
  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (3, 0)), int (l2));

  auto li = l.begin_layers ();
  EXPECT_EQ (li != l.end_layers (), true);
  EXPECT_EQ ((*li).second->to_string (), "1/0");
  ++li;
  EXPECT_EQ (li != l.end_layers (), true);
  EXPECT_EQ ((*li).second->to_string (), "3/0");
  ++li;
  EXPECT_EQ (li == l.end_layers (), true);

  l.set_properties (l2, db::LayerProperties (10, 0));
  EXPECT_EQ (int (l.layers ()), 3);
  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (1, 0)), int (l1));
  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (2, 0)), -1);
  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (3, 0)), -1);
  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (10, 0)), int (l2));

  l.clear ();
  EXPECT_EQ (int (l.layers ()), 0);
  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (1, 0)), -1);
  EXPECT_EQ (l.get_layer_maybe (db::LayerProperties (2, 0)), -1);
}

TEST(8_MetaInfo)
{
  db::Layout ly;

  EXPECT_EQ (ly.meta_info_name_id ("a"), (unsigned int) 0);
  EXPECT_EQ (ly.meta_info_name_id ("b"), (unsigned int) 1);
  EXPECT_EQ (ly.meta_info_name_id ("a"), (unsigned int) 0);
  EXPECT_EQ (ly.has_context_info (), false);

  ly.add_meta_info ("a", db::MetaInfo ("description", tl::Variant (17.5), false));
  ly.add_meta_info ("b", db::MetaInfo ("", tl::Variant ("value"), true));

  EXPECT_EQ (ly.has_context_info (), true);

  EXPECT_EQ (ly.meta_info ("x").value.to_string (), "nil");
  EXPECT_EQ (ly.meta_info ("x").description, "");
  EXPECT_EQ (ly.meta_info ("x").persisted, false);

  EXPECT_EQ (ly.meta_info ("a").value.to_string (), "17.5");
  EXPECT_EQ (ly.meta_info ("a").description, "description");
  EXPECT_EQ (ly.meta_info ("a").persisted, false);

  EXPECT_EQ (ly.meta_info (1).value.to_string (), "value");
  EXPECT_EQ (ly.meta_info (1).description, "");
  EXPECT_EQ (ly.meta_info (1).persisted, true);

  db::cell_index_type ci = ly.add_cell ("X");

  EXPECT_EQ (ly.has_context_info (ci), false);

  ly.add_meta_info (ci, "a", db::MetaInfo ("dd", tl::Variant (-1), false));
  ly.add_meta_info (ci, "b", db::MetaInfo ("d", tl::Variant ("u"), true));

  EXPECT_EQ (ly.has_context_info (ci), true);

  EXPECT_EQ (ly.meta_info (ci, "x").value.to_string (), "nil");
  EXPECT_EQ (ly.meta_info (ci, "x").description, "");
  EXPECT_EQ (ly.meta_info (ci, "x").persisted, false);

  EXPECT_EQ (ly.meta_info (ci, "a").value.to_string (), "-1");
  EXPECT_EQ (ly.meta_info (ci, "a").description, "dd");
  EXPECT_EQ (ly.meta_info (ci, "a").persisted, false);

  EXPECT_EQ (ly.meta_info (ci, 1).value.to_string (), "u");
  EXPECT_EQ (ly.meta_info (ci, 1).description, "d");
  EXPECT_EQ (ly.meta_info (ci, 1).persisted, true);

  EXPECT_EQ (ly.has_context_info (), true);
  ly.clear_meta ();
  EXPECT_EQ (ly.has_context_info (), false);
  EXPECT_EQ (ly.meta_info ("a").value.to_string (), "nil");

  EXPECT_EQ (ly.has_context_info (ci), true);
  ly.clear_meta (ci);
  EXPECT_EQ (ly.has_context_info (ci), false);
  EXPECT_EQ (ly.meta_info (ci, "a").value.to_string (), "nil");
}

TEST(9_ErrorLayer)
{
  db::Manager m;
  db::Layout l (&m);

  EXPECT_EQ (l.is_valid_layer (0), false);
  EXPECT_EQ (l.guiding_shape_layer (), (unsigned int) 0);
  EXPECT_EQ (l.is_valid_layer (1), false);
  EXPECT_EQ (l.error_layer (), (unsigned int) 1);
  EXPECT_EQ (l.is_special_layer (1), true);
  EXPECT_EQ (int (l.layers ()), 2);
}

TEST(10_TranslateStringRefs)
{
  db::Manager m;
  db::Layout l (&m);
  db::Cell &top = l.cell (l.add_cell ("TOP"));
  l.insert_layer (db::LayerProperties (1, 0));

  {
    std::unique_ptr<db::Layout> t (new db::Layout ());
    db::Cell &ttop = t->cell (t->add_cell ("TOP"));
    unsigned int tl1 = t->insert_layer (db::LayerProperties (1, 0));

    const db::StringRef *string_ref = db::StringRepository::instance ()->create_string_ref ();
    db::StringRepository::change_string_ref (string_ref, "TEXT");
    db::Text txt (string_ref, db::Trans ());
    ttop.shapes (tl1).insert (db::TextRef (txt, t->shape_repository ()));
    ttop.shapes (tl1).insert (txt);

    EXPECT_EQ (l2s (*t), "begin_lib 0.001\nbegin_cell {TOP}\ntext 1 0 0 0 {0 0} {TEXT}\ntext 1 0 0 0 {0 0} {TEXT}\nend_cell\nend_lib\n");

    db::CellMapping cm;
    cm.create_single_mapping (l, top.cell_index (), *t, ttop.cell_index ());
    l.copy_tree_shapes (*t, cm);
    EXPECT_EQ (l2s (l), "begin_lib 0.001\nbegin_cell {TOP}\ntext 1 0 0 0 {0 0} {TEXT}\ntext 1 0 0 0 {0 0} {TEXT}\nend_cell\nend_lib\n");

    db::StringRepository::change_string_ref (string_ref, "TEXT_NEW");

    EXPECT_EQ (l2s (*t), "begin_lib 0.001\nbegin_cell {TOP}\ntext 1 0 0 0 {0 0} {TEXT_NEW}\ntext 1 0 0 0 {0 0} {TEXT_NEW}\nend_cell\nend_lib\n");
    //  also the copy changes:
    EXPECT_EQ (l2s (l), "begin_lib 0.001\nbegin_cell {TOP}\ntext 1 0 0 0 {0 0} {TEXT_NEW}\ntext 1 0 0 0 {0 0} {TEXT_NEW}\nend_cell\nend_lib\n");
  }

  //  after deleting the tmp layout, l is still valid
  EXPECT_EQ (l2s (l), "begin_lib 0.001\nbegin_cell {TOP}\ntext 1 0 0 0 {0 0} {TEXT_NEW}\ntext 1 0 0 0 {0 0} {TEXT_NEW}\nend_cell\nend_lib\n");
}

TEST(11_FindPath)
{
  db::Manager m;
  db::Layout l (&m);
  db::Cell &top = l.cell (l.add_cell ("TOP"));
  db::Cell &a = l.cell (l.add_cell ("A"));
  db::Cell &b = l.cell (l.add_cell ("B"));
  db::Cell &c = l.cell (l.add_cell ("C"));
  l.insert_layer (db::LayerProperties (1, 0));

  top.insert (db::CellInstArray (a.cell_index (), db::Trans (1, db::Vector ())));
  a.insert (db::CellInstArray (b.cell_index (), db::Trans (0, db::Vector (100, 200))));

  std::vector<db::InstElement> path;
  EXPECT_EQ (db::find_path (l, c.cell_index (), top.cell_index (), path), false);
  EXPECT_EQ (path.size (), size_t (0));

  EXPECT_EQ (db::find_path (l, top.cell_index (), top.cell_index (), path), true);
  EXPECT_EQ (path.size (), size_t (0));

  EXPECT_EQ (db::find_path (l, b.cell_index (), top.cell_index (), path), true);
  EXPECT_EQ (path.size (), size_t (2));

  std::string d = tl::join (path.begin (), path.end (), ";");
  EXPECT_EQ (d, "cell_index=1 r90 *1 0,0;cell_index=2 r0 *1 100,200");
}
