
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
#include "dbPCellHeader.h"
#include "dbPCellDeclaration.h"
#include "dbPCellVariant.h"
#include "dbLibrary.h"
#include "dbLibraryManager.h"
#include "dbLibraryProxy.h"
#include "dbWriter.h"
#include "dbReader.h"
#include "dbLayoutDiff.h"
#include "tlStream.h"
#include "tlStaticObjects.h"
#include "tlUnitTest.h"

#include <memory>

class LIBT_PD 
  : public db::PCellDeclaration
{
  virtual std::vector<db::PCellLayerDeclaration> get_layer_declarations (const db::pcell_parameters_type &) const
  {
    std::vector<db::PCellLayerDeclaration> layers;

    layers.push_back(db::PCellLayerDeclaration ());
    layers.back ().symbolic = "gate";
    layers.back ().layer = 16;
    layers.back ().datatype = 0;

    layers.push_back(db::PCellLayerDeclaration ());
    layers.back ().symbolic = "metal0";
    layers.back ().layer = 24;
    layers.back ().datatype = 0;

    layers.push_back(db::PCellLayerDeclaration ());
    layers.back ().symbolic = "cont";
    layers.back ().layer = 23;
    layers.back ().datatype = 0;

    return layers;
  }

  virtual std::vector<db::PCellParameterDeclaration> get_parameter_declarations () const
  {
    std::vector<db::PCellParameterDeclaration> parameters;

    parameters.push_back (db::PCellParameterDeclaration ("length"));
    parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
    parameters.push_back (db::PCellParameterDeclaration ("width"));
    parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
    parameters.push_back (db::PCellParameterDeclaration ("orientation"));
    parameters.back ().set_type (db::PCellParameterDeclaration::t_int);

    return parameters;
  }

  virtual void produce (const db::Layout &layout, const std::vector<unsigned int> &layer_ids, const db::pcell_parameters_type &parameters, db::Cell &cell) const
  {
    db::Coord width = db::coord_traits<db::Coord>::rounded (parameters[0].to_double () / layout.dbu ());
    db::Coord height = db::coord_traits<db::Coord>::rounded (parameters[1].to_double () / layout.dbu ());

    int orientation = parameters[2].to_long ();

    //unsigned int l_gate = layer_ids[0];
    unsigned int l_metal0 = layer_ids[1];
    //unsigned int l_cont = layer_ids[2];

    const db::Cell &cell_a = layout.cell (layout.cell_by_name ("A").second);

    cell.insert (db::CellInstArray (db::CellInst (cell_a.cell_index ()), db::Trans (orientation, db::Vector (width / 2 - 50, height / 2 - 100))));

    cell.shapes (l_metal0).insert (db::Box (0, 0, width, height));
  }
};

class LIBT_L
  : public db::Library
{
public:
  LIBT_L (tl::TestBase *_this)
    : Library () 
  {
    set_name("L");
    set_description("A test library.");

    layout ().dbu (0.001);
    
    db::LayerProperties p;

    p.layer = 23;
    p.datatype = 0;
    unsigned int l_cont = layout ().insert_layer (p);

    p.layer = 16;
    p.datatype = 0;
    unsigned int l_gate = layout ().insert_layer (p);

    db::Cell &cell_a = layout ().cell (layout ().add_cell ("A"));
    cell_a.shapes(l_cont).insert(db::Box (50, 50, 150, 150));
    cell_a.shapes(l_gate).insert(db::Box (0, 0, 200, 1000));

    db::Cell &top = layout ().cell (layout ().add_cell ("TOP"));

    db::pcell_id_type pd = layout ().register_pcell ("PD", new LIBT_PD ());

    std::vector<tl::Variant> parameters;
    parameters.push_back (tl::Variant ());
    parameters.push_back (tl::Variant ());
    parameters.push_back (tl::Variant ());
    tl::Variant &width = parameters[0];
    tl::Variant &height = parameters[1];
    tl::Variant &orientation = parameters[2];

    width = 0.5;
    height = 1.0;
    orientation = long (0);

    db::cell_index_type pd1 = layout ().get_pcell_variant (pd, parameters);
    db::Instance i1 = top.insert (db::CellInstArray (db::CellInst (pd1), db::Trans (db::Vector (0, 0))));

    width = width.to_double () * 0.1;
    width = width.to_double () * 10.0;

    db::cell_index_type pd2 = layout ().get_pcell_variant (pd, parameters);
    db::Instance i2 = top.insert (db::CellInstArray (db::CellInst (pd2), db::Trans (db::Vector (0, 2000))));

    EXPECT_EQ (pd1, pd2);

    width = 0.4;
    height = 0.8;
    orientation = long (1);

    db::cell_index_type pd3 = layout ().get_pcell_variant (pd, parameters);
    db::Instance i3 = top.insert (db::CellInstArray (db::CellInst (pd3), db::Trans (db::Vector (2000, 0))));
  }

  ~LIBT_L()
  {
    // .. nothing yet ..
  }
};

class LIBT_A
  : public db::Library
{
public:
  LIBT_A () 
    : Library () 
  {
    set_name("A");

    layout ().dbu (0.001);
    
    db::LayerProperties p;

    p.layer = 1;
    p.datatype = 0;
    unsigned int l1 = layout ().insert_layer (p);

    p.layer = 2;
    p.datatype = 0;
    unsigned int l2 = layout ().insert_layer (p);

    db::Cell &cell_a = layout ().cell (layout ().add_cell ("A"));
    cell_a.shapes(l1).insert(db::Box (50, 50, 150, 150));
    cell_a.shapes(l2).insert(db::Box (0, 0, 200, 1000));
  }
};

class LIBT_B
  : public db::Library
{
public:
  LIBT_B () 
    : Library () 
  {
    set_name("B");

    layout ().dbu (0.001);
    
    db::LayerProperties p;

    p.layer = 1;
    p.datatype = 0;
    unsigned int l1 = layout ().insert_layer (p);

    p.layer = 3;
    p.datatype = 0;
    unsigned int l3 = layout ().insert_layer (p);

    db::Cell &cell_b = layout ().cell (layout ().add_cell ("B"));
    cell_b.shapes(l1).insert(db::Box (10, 20, 30, 40));
    cell_b.shapes(l3).insert(db::Box (0, 0, 10, 20));

    db::Library *lib_a = db::LibraryManager::instance ().lib_ptr_by_name ("A");
    tl_assert (lib_a != 0);

    std::pair<bool, db::cell_index_type> a = lib_a->layout ().cell_by_name ("A");
    tl_assert (a.first);
      
    db::cell_index_type cp = layout ().get_lib_proxy (lib_a, a.second);
    cell_b.insert (db::CellInstArray (db::CellInst (cp), db::ICplxTrans (0.1, 0.0, false, db::Vector (1.0, 2.0))));
     
  }
};

static bool compare_vs_au (const tl::TestBase *tb, const db::Layout &layout, const std::string &filename)
{
  db::Layout layout_au;

  std::string fn (tl::testsrc ());
  fn += "/testdata/gds/";
  fn += filename;
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.read (layout_au);

  //  generate a "unique" name ...
  unsigned int hash = 0;
  for (const char *cp = filename.c_str (); *cp; ++cp) {
    hash = (hash << 4) ^ (hash >> 4) ^ ((unsigned int) *cp);
  }

  std::string tmp_file = tb->tmp_file (tl::sprintf ("tmp_%x.gds", hash));

  //  "normalize" the layout by writing and reading ...
  {
    db::Writer writer = db::Writer (db::SaveLayoutOptions ());
    tl::OutputStream stream (tmp_file);
    writer.write (const_cast<db::Layout &> (layout), stream);
  }

  db::Layout tmp;
  tl::InputStream tmp_stream (tmp_file);
  db::Reader reader_tmp (tmp_stream);
  reader_tmp.read (tmp);
    
  bool equal = db::compare_layouts (tmp, layout_au, db::layout_diff::f_verbose, 0);
  if (! equal) {
    tl::warn << tl::sprintf ("Compare failed - see %s vs %s\n", tmp_file, fn);
  }
  return equal;
}

TEST(1) 
{
  bool equal;

  std::vector<std::string> libnames_before;
  for (db::LibraryManager::iterator il = db::LibraryManager::instance ().begin (); il != db::LibraryManager::instance ().end (); ++il) {
    libnames_before.push_back (il->first);
  }
  std::sort (libnames_before.begin (), libnames_before.end ());

  LIBT_L *l = new LIBT_L (_this);
  db::lib_id_type lib_id = db::LibraryManager::instance ().register_lib (l);

  try {

    std::vector<std::string> libnames_withl;
    for (db::LibraryManager::iterator il = db::LibraryManager::instance ().begin (); il != db::LibraryManager::instance ().end (); ++il) {
      libnames_withl.push_back (il->first);
    }
    std::sort (libnames_withl.begin (), libnames_withl.end ());

    std::vector<std::string> ll = libnames_before;
    ll.push_back ("L");
    std::sort (ll.begin (), ll.end ());

    EXPECT_EQ (tl::join (libnames_withl, ","), tl::join (ll, ","));

    std::pair<bool, db::lib_id_type> lbn;
    lbn = db::LibraryManager::instance ().lib_by_name ("X");
    EXPECT_EQ (lbn.first, false);
    lbn = db::LibraryManager::instance ().lib_by_name ("L");
    EXPECT_EQ (lbn.first, true);
    EXPECT_EQ (lbn.second, lib_id);

    db::Library *lib = db::LibraryManager::instance ().lib (lib_id);
    EXPECT_EQ (lib == l, true);
    EXPECT_EQ (lib->get_id (), lib_id);
    EXPECT_EQ (lib->get_name (), "L");
    EXPECT_EQ (lib->get_description (), "A test library.");

    EXPECT_EQ (lib->layout ().get_properties(0).to_string (), "23/0");
    EXPECT_EQ (lib->layout ().get_properties(1).to_string (), "16/0");
    EXPECT_EQ (lib->layout ().get_properties(2).to_string (), "24/0");

    db::Manager m (true);
    db::Layout layout(&m);
    layout.dbu (0.001);

    db::Cell &top = layout.cell (layout.add_cell ("TOP"));

    EXPECT_EQ (lib->layout ().cell_by_name ("TOP").first, true);
    db::cell_index_type lib_top = lib->layout ().cell_by_name ("TOP").second;
    db::cell_index_type lp1 = layout.get_lib_proxy (lib, lib_top);

    EXPECT_EQ (std::string (layout.cell_name (lp1)), "TOP$1");
    EXPECT_EQ (layout.basic_name (lp1), "TOP");
    EXPECT_EQ (layout.display_name (lp1), "L.TOP");

    EXPECT_EQ (layout.get_properties(0).to_string (), "23/0");
    EXPECT_EQ (layout.get_properties(1).to_string (), "16/0");
    EXPECT_EQ (layout.get_properties(2).to_string (), "24/0");

    db::Instance i1 = top.insert (db::CellInstArray (db::CellInst (lp1), db::Trans (db::Vector (0, 0))));

    std::vector<tl::Variant> parameters;
    parameters.push_back (tl::Variant ());
    parameters.push_back (tl::Variant ());
    parameters.push_back (tl::Variant ());
    tl::Variant &width = parameters[0];
    tl::Variant &height = parameters[1];
    tl::Variant &orientation = parameters[2];
    width = 2.0;
    height = 10.0;
    orientation = (long)3;

    EXPECT_EQ (lib->layout ().pcell_by_name ("PD").first, true);
    db::pcell_id_type pd = lib->layout ().pcell_by_name ("PD").second;
    db::cell_index_type lib_pd1 = lib->layout ().get_pcell_variant (pd, parameters);
    db::cell_index_type lp2 = layout.get_lib_proxy (lib, lib_pd1);
    EXPECT_EQ (std::string (layout.cell_name (lp2)), "PD$2");
    EXPECT_EQ (layout.basic_name (lp2), "PD");
    EXPECT_EQ (layout.display_name (lp2), "L.PD*");

    const db::Cell *lp2_cell = &layout.cell (lp2);
    EXPECT_EQ (dynamic_cast<const db::LibraryProxy *> (lp2_cell) != 0, true);
    EXPECT_EQ (lp2_cell->is_proxy (), true);
    EXPECT_EQ (layout.is_pcell_instance (lp2).first, true);
    EXPECT_EQ (layout.is_pcell_instance (lp2).second, pd);
    EXPECT_EQ (layout.get_pcell_parameters (lp2)[0].to_string(), std::string ("2"));
    EXPECT_EQ (layout.get_pcell_parameters (lp2)[1].to_string(), std::string ("10"));

    db::Instance i2 = top.insert (db::CellInstArray (db::CellInst (lp2), db::Trans (db::Vector (10000, 0))));

    db::Writer writer = db::Writer (db::SaveLayoutOptions ());
    /* produce golden:
    tl::OutputStream stream ("lib_test.gds");
    writer.write (layout, stream);
    */

    equal = compare_vs_au (this, layout, "lib_test.gds");
    EXPECT_EQ (equal, true);

    // if not in editable mode, we could have lost the reference to the second instance
    if (db::default_editable_mode ()) {

      m.transaction ("x");

      height = 5.0;
      db::cell_index_type i2_cid = i2.cell_index ();
      i2 = top.change_pcell_parameters (i2, parameters);
      EXPECT_NE (i2.cell_index (), i2_cid);

      EXPECT_EQ (std::string (layout.cell_name (i2.cell_index ())), "PD$3");
      EXPECT_EQ (layout.basic_name (i2.cell_index ()), "PD");
      EXPECT_EQ (layout.display_name (i2.cell_index ()), "L.PD*");
    
      /* produce golden:
      tl::OutputStream stream2 ("lib_test2.gds");
      writer.write (layout, stream2);
      */

      equal = compare_vs_au (this, layout, "lib_test2.gds");
      EXPECT_EQ (equal, true);

      m.commit ();

      m.transaction ("y");

      width = 0.5;
      height = 1.0;
      orientation = long (0);

      i2 = top.change_pcell_parameters (i2, parameters);

      /* produce golden:
      tl::OutputStream stream3 ("lib_test3.gds");
      writer.write (layout, stream3);
      */

      EXPECT_EQ (std::string (layout.cell_name (i2.cell_index ())), "PD");
      EXPECT_EQ (layout.basic_name (i2.cell_index ()), "PD");
      EXPECT_EQ (layout.display_name (i2.cell_index ()), "L.PD*");
    
      equal = compare_vs_au (this, layout, "lib_test3.gds");
      EXPECT_EQ (equal, true);

      m.commit ();

      m.undo ();

      equal = compare_vs_au (this, layout, "lib_test2.gds");
      EXPECT_EQ (equal, true);

      m.undo ();

      equal = compare_vs_au (this, layout, "lib_test.gds");
      EXPECT_EQ (equal, true);

      m.redo ();

      equal = compare_vs_au (this, layout, "lib_test2.gds");
      EXPECT_EQ (equal, true);
    }

    //  because we switch to editable mode in between we have to clear the repository explicitly. Otherwise it's being cleared 
    //  on next entry of TEST which will cause a segmentation fault if editable mode is different then.
    db::LibraryManager::instance ().delete_lib (l);

  } catch (...) {

    //  because we switch to editable mode in between we have to clear the repository explicitly. Otherwise it's being cleared 
    //  on next entry of TEST which will cause a segmentation fault if editable mode is different then.
    db::LibraryManager::instance ().delete_lib (l);
    throw;

  }

  std::vector<std::string> libnames_after;
  for (db::LibraryManager::iterator il = db::LibraryManager::instance ().begin (); il != db::LibraryManager::instance ().end (); ++il) {
    libnames_after.push_back (il->first);
  }
  std::sort (libnames_after.begin (), libnames_after.end ());

  EXPECT_EQ (tl::join (libnames_before, ","), tl::join (libnames_after, ","));
}

TEST(2) 
{
  LIBT_L *lib = new LIBT_L (_this);
  db::LibraryManager::instance ().register_lib (lib);

  try {

    bool equal;
    db::Writer writer = db::Writer (db::SaveLayoutOptions ());

    db::Manager m (true);
    db::Layout layout(&m);
    layout.dbu (0.001);

    db::Cell &top = layout.cell (layout.add_cell ("TOP"));

    db::cell_index_type lib_top = lib->layout ().cell_by_name ("TOP").second;
    db::cell_index_type lp1 = layout.get_lib_proxy (lib, lib_top);
    db::Instance i1 = top.insert (db::CellInstArray (db::CellInst (lp1), db::Trans (db::Vector (0, 0))));

    std::vector<tl::Variant> parameters;
    parameters.push_back (tl::Variant ());
    parameters.push_back (tl::Variant ());
    parameters.push_back (tl::Variant ());
    tl::Variant &width = parameters[0];
    tl::Variant &height = parameters[1];
    tl::Variant &orientation = parameters[2];
    width = 2.0;
    height = 10.0;
    orientation = (long)3;

    db::pcell_id_type pd = lib->layout ().pcell_by_name ("PD").second;
    db::cell_index_type lib_pd1 = lib->layout ().get_pcell_variant (pd, parameters);
    db::cell_index_type lp2 = layout.get_lib_proxy (lib, lib_pd1);
    db::Instance i2 = top.insert (db::CellInstArray (db::CellInst (lp2), db::Trans (db::Vector (10000, 0))));

    EXPECT_EQ (std::string (layout.cell_name (lp2)), "PD$2");
    EXPECT_EQ (layout.basic_name (lp2), "PD");
    EXPECT_EQ (layout.display_name (lp2), "L.PD*");

    std::string tmp_file = tl::TestBase::tmp_file (tl::sprintf ("tmp_dbLibraries2.gds"));

    {
      tl::OutputStream stream (tmp_file);
      writer.write (layout, stream);
    }

    db::Layout tmp;
    {
      tl::InputStream tmp_stream (tmp_file);
      db::Reader reader_tmp (tmp_stream);
      reader_tmp.read (tmp);
    }
    
    std::pair<bool, db::cell_index_type> tmp_pd2 = tmp.cell_by_name ("PD$2");
    EXPECT_EQ (tmp_pd2.first, true);
    EXPECT_EQ (tmp.basic_name (tmp_pd2.second), "PD");
    EXPECT_EQ (tmp.display_name (tmp_pd2.second), "L.PD*");

    db::Instance tmp_i2 = tmp.cell (tmp_pd2.second).begin_parent_insts ()->child_inst ();
    EXPECT_EQ (tmp_i2.cell_index (), tmp_pd2.second);
    std::vector<tl::Variant> new_param = tmp.get_pcell_parameters (tmp_pd2.second);

    EXPECT_EQ (new_param.size (), size_t (3));
    EXPECT_EQ (new_param[0].to_string (), std::string ("2"));
    EXPECT_EQ (new_param[1].to_string (), std::string ("10"));
    EXPECT_EQ (new_param[2].to_string (), std::string ("3"));

    std::pair<bool, db::cell_index_type> tt = tmp.cell_by_name ("TOP");
    EXPECT_EQ (tt.first, true);
    db::Cell &tmp_top = tmp.cell (tt.second);
  
    if (db::default_editable_mode ()) {

      new_param[1] = 5.0;
      db::cell_index_type tmp_i2_cid = tmp_i2.cell_index ();
      tmp_i2 = tmp_top.change_pcell_parameters (tmp_i2, new_param);

      EXPECT_NE (tmp_i2.cell_index (), tmp_i2_cid);

      EXPECT_EQ (std::string (tmp.cell_name (tmp_i2.cell_index ())), "PD$3");
      EXPECT_EQ (tmp.basic_name (tmp_i2.cell_index ()), "PD");
      EXPECT_EQ (tmp.display_name (tmp_i2.cell_index ()), "L.PD*");
    
      /* produce golden:
      tl::OutputStream stream3 ("lib_test2.gds");
      writer.write (tmp, stream3);
      */
 
      equal = compare_vs_au (this, tmp, "lib_test2.gds");
      EXPECT_EQ (equal, true);

    }

    //  because we switch to editable mode in between we have to clear the repository explicitly. Otherwise it's being cleared 
    //  on next entry of TEST which will cause a segmentation fault if editable mode is different then.
    db::LibraryManager::instance ().delete_lib (lib);

  } catch (...) {

    //  because we switch to editable mode in between we have to clear the repository explicitly. Otherwise it's being cleared 
    //  on next entry of TEST which will cause a segmentation fault if editable mode is different then.
    db::LibraryManager::instance ().delete_lib (lib);
    throw;

  }
}

TEST(3) 
{
  LIBT_A *lib_a = new LIBT_A ();
  db::LibraryManager::instance ().register_lib (lib_a);

  LIBT_B *lib_b = new LIBT_B ();
  db::LibraryManager::instance ().register_lib (lib_b);

  try {

    //  This test tests the ability to reference libraries out of other libraries ("B" references "A"),
    //  the ability to persist that and whether this survives a write/read cycle.
    
    db::Manager m (true);
    db::Layout layout(&m);
    layout.dbu (0.001);

    db::Cell &top = layout.cell (layout.add_cell ("TOP"));

    db::cell_index_type lib_bb = lib_b->layout ().cell_by_name ("B").second;
    db::cell_index_type lp = layout.get_lib_proxy (lib_b, lib_bb);
    db::Instance i1 = top.insert (db::CellInstArray (db::CellInst (lp), db::Trans (db::Vector (0, 0))));

    std::string tmp_file = tl::TestBase::tmp_file (tl::sprintf ("tmp_dbLibraries3.gds"));

    {
      db::Writer writer = db::Writer (db::SaveLayoutOptions ());
      tl::OutputStream stream (tmp_file);
      writer.write (layout, stream);
    }

    layout.clear ();

    db::Layout tmp;
    {
      tl::InputStream tmp_stream (tmp_file);
      db::Reader reader_tmp (tmp_stream);
      reader_tmp.read (tmp);
    }
    
    bool equal = compare_vs_au (this, tmp, "lib_test4.gds");
    EXPECT_EQ (equal, true);

    //  because we switch to editable mode in between we have to clear the repository explicitly. Otherwise it's being cleared 
    //  on next entry of TEST which will cause a segmentation fault if editable mode is different then.
    db::LibraryManager::instance ().delete_lib (lib_a);
    db::LibraryManager::instance ().delete_lib (lib_b);

  } catch (...) {

    //  because we switch to editable mode in between we have to clear the repository explicitly. Otherwise it's being cleared 
    //  on next entry of TEST which will cause a segmentation fault if editable mode is different then.
    db::LibraryManager::instance ().delete_lib (lib_a);
    db::LibraryManager::instance ().delete_lib (lib_b);
    throw;

  }
}

