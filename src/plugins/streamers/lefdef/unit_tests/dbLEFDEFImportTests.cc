
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


#include "dbLayoutDiff.h"
#include "dbWriter.h"
#include "dbDEFImporter.h"
#include "dbLEFImporter.h"

#include "tlUnitTest.h"
#include "dbTestSupport.h"

#include <cstdlib>

static db::LEFDEFReaderOptions default_options ()
{
  db::LEFDEFReaderOptions tc;
  tc.set_via_geometry_datatype (0);
  tc.set_via_geometry_suffix ("");
  tc.set_pins_datatype (2);
  tc.set_pins_suffix (".PIN");
  tc.set_obstructions_datatype (3);
  tc.set_obstructions_suffix (".OBS");
  tc.set_routing_datatype (0);
  tc.set_routing_suffix ("");
  tc.set_labels_datatype (1);
  tc.set_labels_suffix (".LABEL");
  tc.set_blockages_datatype (4);
  tc.set_blockages_suffix (".BLK");

  return tc;
}

static db::LayerMap read (db::Layout &layout, const char *lef_dir, const char *filename, const db::LEFDEFReaderOptions &options, bool priv = true, db::CellConflictResolution cc_mode = db::RenameCell)
{
  std::string fn_path (priv ? tl::testdata_private () : tl::testdata ());
  fn_path += "/lefdef/";
  fn_path += lef_dir;
  fn_path += "/";

  tl::Extractor ex (filename);

  db::LEFDEFReaderState ld (&options, layout, fn_path);
  ld.set_conflict_resolution_mode (cc_mode);

  db::DEFImporter imp;
  bool any_def = false;
  bool any_lef = false;

  while (! ex.at_end ()) {

    if (ex.test ("map:")) {

      std::string f;
      ex.read_word_or_quoted (f);

      ld.read_map_file (f, layout, fn_path);

    } else if (ex.test ("def:")) {

      std::string fn = fn_path, f;
      ex.read_word_or_quoted (f);
      fn += f;

      tl::InputStream stream (fn);
      imp.read (stream, layout, ld);

      any_def = true;

    } else if (ex.test ("lef:")) {

      std::string fn = fn_path, f;
      ex.read_word_or_quoted (f);
      fn += f;

      tl::InputStream stream (fn);
      imp.read_lef (stream, layout, ld);

      any_lef = true;

    } else if (ex.test ("gds:")) {

      std::string fn = fn_path, f;
      ex.read_word_or_quoted (f);
      fn += f;

      tl::InputStream stream (fn);
      db::Reader reader (stream);
      reader.read (layout, db::LoadLayoutOptions ());

    } else if (ex.test("read:")) {

      std::string fn = fn_path, f;
      ex.read_word_or_quoted (f);
      fn += f;

      tl::InputStream stream (fn);
      db::Reader reader (stream);
      db::LoadLayoutOptions lo;
      lo.set_options (options);
      reader.read (layout, lo);

      any_def = true;

    } else {

      break;

    }

    if (! ex.test("+")) {
      break;
    }

  }

  if (! any_def && any_lef) {
    imp.finish_lef (layout);
  }

  ld.finish (layout);

  return ld.layer_map ();
}

static db::LayerMap run_test (tl::TestBase *_this, const char *lef_dir, const char *filename, const char *au, const db::LEFDEFReaderOptions &options, bool priv = true, db::CellConflictResolution cc_mode = db::RenameCell)
{
  db::Manager m (false);
  db::Layout layout (&m), layout2 (&m), layout_au (&m);

  db::LayerMap lm = read (layout, lef_dir, filename, options, priv, cc_mode);

  //  normalize the layout by writing to OASIS and reading from ..

  //  generate a "unique" name ...
  unsigned int hash = 0;
  if (au) {
    for (const char *cp = au; *cp; ++cp) {
      hash = (hash << 4) ^ (hash >> 4) ^ ((unsigned int) *cp);
    }
  }

  std::string tmp_file = _this->tmp_file (tl::sprintf ("tmp_%x.oas", hash));

  {
    tl::OutputStream stream (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("OASIS");
    options.set_option_by_name ("oasis_permissive", tl::Variant (true));
    db::Writer writer (options);
    writer.write (layout, stream);
  }

  {
    tl::InputStream stream (tmp_file);
    db::Reader reader (stream);
    reader.read (layout2);
  }

  if (au) {

    std::string fn (priv ? tl::testdata_private () : tl::testdata ());
    fn += "/lefdef/";
    fn += lef_dir;
    fn += "/";
    fn += au;

    try {
      tl::InputStream stream (fn);
      db::Reader reader (stream);
      reader.read (layout_au);
    } catch (...) {
      _this->raise (tl::sprintf ("Compare failed - see %s vs %s (not existing or not readable)\n", tmp_file, fn));
      throw;
    }

    bool equal = db::compare_layouts (layout2, layout_au, db::layout_diff::f_verbose | db::layout_diff::f_flatten_array_insts, 0);
    if (! equal) {
      _this->raise (tl::sprintf ("Compare failed - see %s vs %s\n", tmp_file, fn));
    }

  } else {

    bool equal = db::compare_layouts (layout2, layout_au, db::layout_diff::f_verbose | db::layout_diff::f_flatten_array_insts, 0);
    if (! equal) {
      _this->raise (tl::sprintf ("Compare failed - see %s vs empty file\n", tmp_file));
    }

  }

  return lm;
}

static void run_test2 (tl::TestBase *_this, const char *lef_dir, const char *filename, const char *filename2, const char *au, const db::LEFDEFReaderOptions &options, bool priv = true, db::CellConflictResolution cc_mode = db::RenameCell)
{
  db::Manager m (false);
  db::Layout layout (&m), layout2 (&m), layout_au (&m);

  read (layout, lef_dir, filename, options, priv, cc_mode);
  read (layout, lef_dir, filename2, options, priv, cc_mode);

  //  normalize the layout by writing to OASIS and reading from ..

  //  generate a "unique" name ...
  unsigned int hash = 0;
  if (au) {
    for (const char *cp = au; *cp; ++cp) {
      hash = (hash << 4) ^ (hash >> 4) ^ ((unsigned int) *cp);
    }
  }

  std::string tmp_file = _this->tmp_file (tl::sprintf ("tmp_%x.oas", hash));

  {
    tl::OutputStream stream (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("OASIS");
    options.set_option_by_name ("oasis_permissive", tl::Variant (true));
    db::Writer writer (options);
    writer.write (layout, stream);
  }

  {
    tl::InputStream stream (tmp_file);
    db::Reader reader (stream);
    reader.read (layout2);
  }

  if (au) {

    std::string fn (priv ? tl::testdata_private () : tl::testdata ());
    fn += "/lefdef/";
    fn += lef_dir;
    fn += "/";
    fn += au;

    try {
      tl::InputStream stream (fn);
      db::Reader reader (stream);
      reader.read (layout_au);
    } catch (...) {
      _this->raise (tl::sprintf ("Compare failed - see %s vs %s (not existing or not readable)\n", tmp_file, fn));
      throw;
    }

    bool equal = db::compare_layouts (layout2, layout_au, db::layout_diff::f_verbose | db::layout_diff::f_flatten_array_insts, 0);
    if (! equal) {
      _this->raise (tl::sprintf ("Compare failed - see %s vs %s\n", tmp_file, fn));
    }

  } else {

    bool equal = db::compare_layouts (layout2, layout_au, db::layout_diff::f_verbose | db::layout_diff::f_flatten_array_insts, 0);
    if (! equal) {
      _this->raise (tl::sprintf ("Compare failed - see %s vs empty file\n", tmp_file));
    }

  }
}

TEST(lef1)
{
  run_test (_this, "lef1", "lef:in.lef", 0, default_options ());
}

TEST(lef2)
{
  //  Also tests ability of plugin to properly read LEF
  run_test (_this, "lef2", "read:in.lef", "au.oas.gz", default_options ());
}

TEST(lef3)
{
  db::LEFDEFReaderOptions options = default_options ();
  options.set_cell_outline_layer ("OUTLINE (2/0)");
  run_test (_this, "lef3", "lef:in.lef", "au.oas.gz", options);
}

TEST(lef4)
{
  run_test (_this, "lef4", "lef:in.lef", 0, default_options ());
}

TEST(lef5)
{
  run_test (_this, "lef5", "lef:in.lef", 0, default_options ());
}

TEST(lef6)
{
  run_test (_this, "lef6", "lef:in.lef", 0, default_options ());
}

TEST(lef7)
{
  db::LEFDEFReaderOptions options = default_options ();
  options.set_cell_outline_layer ("OUTLINE (8/0)");
  run_test (_this, "lef7", "lef:in_tech.lef+lef:in.lef", "au.oas.gz", options);
}

TEST(lef8)
{
  //  this is rather a smoke test and throws a number of warnings
  //  (complete example)
  run_test (_this, "lef8", "lef:tech.lef+lef:a.lef", "au.oas.gz", default_options ());
}

TEST(def1)
{
  run_test (_this, "def1", "lef:in.lef+def:in.def", "au2_2.oas.gz", default_options ());
}

TEST(def2)
{
  db::LEFDEFReaderOptions options = default_options ();
  options.set_cell_outline_layer ("OUTLINE (10/0)");
  run_test (_this, "def2", "lef:0.lef+lef:1.lef+def:in.def.gz", "au_3.oas.gz", options);
}

TEST(def3)
{
  db::LEFDEFReaderOptions options = default_options ();
  options.set_cell_outline_layer ("OUTLINE (13/0)");
  run_test (_this, "def3", "lef:in.lef+def:in.def", "au_2.oas.gz", options);
}

TEST(def4)
{
  run_test (_this, "def4", "lef:in.lef+def:in.def", "au2_2.oas.gz", default_options ());
}

TEST(def5)
{
  run_test (_this, "def5", "lef:in.lef+def:in.def", "au.oas.gz", default_options ());
}

TEST(def6)
{
  run_test (_this, "def6", "lef:cells.lef+lef:tech.lef+def:in.def.gz", "au-new_2.oas.gz", default_options ());
}

TEST(def7)
{
  db::LEFDEFReaderOptions options = default_options ();
  options.set_placement_blockage_layer ("PLACEMENT_BLK (11/0)");
  run_test (_this, "def7", "lef:cells.lef+lef:tech.lef+def:in.def.gz", "au-new_2.oas.gz", options);

  options.set_placement_blockage_layer ("PLACEMENT_BLK (60/0)");
  run_test (_this, "def7", "map:in.map+lef:cells.lef+lef:tech.lef+def:in.def.gz", "au2_with_map_file-new_2.oas.gz", options);
}

TEST(def8)
{
  run_test (_this, "def8", "lef:tech.lef+def:in.def", "au.oas.gz", default_options ());
}

TEST(def9)
{
  db::LEFDEFReaderOptions options = default_options ();
  options.set_separate_groups (true);
  run_test (_this, "def9", "lef:tech.lef+lef:cells_modified.lef+def:in.def", "au-new_2.oas.gz", options);

  run_test (_this, "def9", "lef:tech.lef+lef:cells_modified.lef+def:in.def", "au_nogroups-new_2.oas.gz", default_options ());
}

TEST(def10)
{
  db::LEFDEFReaderOptions opt = default_options ();
  opt.set_cell_outline_layer ("OUTLINE (2/0)");
  run_test (_this, "def10", "def:in.def", "au.oas.gz", opt);
}

TEST(def11)
{
  db::LEFDEFReaderOptions opt = default_options ();
  opt.set_cell_outline_layer ("OUTLINE (12/0)");
  run_test (_this, "def11", "lef:test.lef+def:test.def", "au.oas.gz", opt);
}

TEST(def12)
{
  db::LEFDEFReaderOptions opt = default_options ();
  opt.set_cell_outline_layer ("OUTLINE (20/0)");
  run_test (_this, "def12", "lef:test.lef+def:test.def", "au-new.oas.gz", opt);
}

TEST(def13)
{
  db::LEFDEFReaderOptions opt = default_options ();
  run_test (_this, "def13", "map:test.map+lef:test.lef_5.8+def:top.def.gz", "au2.oas.gz", opt);
}

TEST(def14)
{
  db::LEFDEFReaderOptions opt = default_options ();
  opt.set_macro_resolution_mode (1);
  run_test (_this, "def14", "map:test.map+lef:tech.lef+lef:stdlib.lef+def:test.def", "au_2.oas.gz", opt);
}

TEST(def15)
{
  db::LEFDEFReaderOptions opt = default_options ();
  opt.set_macro_resolution_mode (1);
  run_test (_this, "def15", "map:test.map+lef:tech.lef+def:test.def", "au2_2.oas.gz", opt);
}

TEST(def16)
{
  //  this is rather a smoke test
  //  (complete example)
  db::LEFDEFReaderOptions opt = default_options ();
  opt.set_macro_resolution_mode (1);
  run_test (_this, "def16", "lef:a.lef+lef:tech.lef+def:a.def", "au_4b.oas.gz", opt);
}

TEST(100)
{
  run_test (_this, "issue-172", "lef:in.lef+def:in.def", "au.oas.gz", default_options (), false);
}

TEST(101)
{
  db::LEFDEFReaderOptions opt = default_options ();
  opt.set_produce_pin_names (true);
  opt.set_pin_property_name (2);
  opt.set_cell_outline_layer ("OUTLINE (13/0)");
  run_test (_this, "issue-489", "lef:in.lef+def:in.def", "au.oas", opt, false);
}

TEST(102)
{
  db::LEFDEFReaderOptions opt = default_options ();
  opt.set_produce_pin_names (true);
  opt.set_pin_property_name (3);
  opt.set_cell_outline_layer ("OUTLINE (8/0)");
  run_test (_this, "issue-489b", "lef:in_tech.lef+lef:in.lef", "au.oas.gz", opt, false);
}

TEST(103)
{
  db::LEFDEFReaderOptions options = default_options ();
  options.set_cell_outline_layer ("OUTLINE (4/0)");
  run_test (_this, "issue-517", "def:in.def", "au.oas.gz", options, false);
}

TEST(104_doxy_vias)
{
  db::LEFDEFReaderOptions options = default_options ();
  options.set_cell_outline_layer ("OUTLINE (4/0)");
  run_test (_this, "doxy_vias", "def:test.def", "au.oas.gz", options, false);
}

TEST(105_specialnets_geo)
{
  run_test (_this, "specialnets_geo", "lef:test.lef+def:test.def", "au.oas.gz", default_options (), false);

  db::LEFDEFReaderOptions options = default_options ();
  options.set_produce_special_routing (false);
  run_test (_this, "specialnets_geo", "lef:test.lef+def:test.def", "au_no_spnet.oas.gz", options, false);

  options.set_produce_special_routing (true);
  options.set_special_routing_datatype (10);
  options.set_special_routing_suffix (".SPNET");

  options.set_via_geometry_datatype (11);
  options.set_via_geometry_suffix (".VIA");

  run_test (_this, "specialnets_geo", "lef:test.lef+def:test.def", "au_spnet_mapped.oas.gz", options, false);
}

TEST(106_wrongdirection)
{
  run_test (_this, "wrongdirection", "lef:test.lef+def:test.def", "au.oas.gz", default_options (), false);
}

TEST(107_specialwidths)
{
  run_test (_this, "specialwidths", "lef:test.lef+def:test.def", "au.oas.gz", default_options (), false);
}

TEST(108_scanchain)
{
  run_test (_this, "scanchain", "def:test.def", "au.oas.gz", default_options (), false);
}

TEST(109_foreigncell)
{
  db::LEFDEFReaderOptions options = default_options ();
  options.set_cell_outline_layer ("OUTLINE (43/0)");

  run_test (_this, "foreigncell", "gds:foreign.gds+lef:in_tech.lef+lef:in.lef+def:in.def", "au.oas.gz", options, false);

  run_test (_this, "foreigncell", "gds:foreign.gds+lef:in_tech.lef+lef:in2.lef+def:in.def", "au_default.oas.gz", options, false);

  options.set_macro_resolution_mode (1);

  run_test (_this, "foreigncell", "gds:foreign.gds+lef:in_tech.lef+lef:in2.lef+def:in.def", "au_ignore_foreign.oas.gz", options, false);

  options.set_macro_resolution_mode (2);

  run_test (_this, "foreigncell", "gds:foreign.gds+lef:in_tech.lef+lef:in.lef+def:in.def", "au_always_foreign.oas.gz", options, false);
}

TEST(110_lefpins)
{
  db::LEFDEFReaderOptions options = default_options ();
  options.set_produce_lef_pins (false);
  options.set_cell_outline_layer ("OUTLINE (8/0)");
  run_test (_this, "lefpins", "lef:in_tech.lef+lef:in.lef+def:in.def", "au_no_lefpins.oas.gz", options, false);

  options.set_produce_lef_pins (true);
  options.set_lef_pins_datatype (10);
  options.set_lef_pins_suffix (".LEFPIN");

  run_test (_this, "lefpins", "lef:in_tech.lef+lef:in.lef+def:in.def", "au_lefpins_mapped.oas.gz", options, false);
}

TEST(111_mapfile)
{
  db::LEFDEFReaderOptions options = default_options ();
  options.set_map_file ("test.map");

  run_test (_this, "mapfile", "read:in.def", "au.oas.gz", options, false);

  options.set_map_file ("test-nonames.map");

  run_test (_this, "mapfile", "read:in.def", "au.oas.gz", options, false);
}

TEST(112_via_properties)
{
  db::LEFDEFReaderOptions options = default_options ();
  db::LayerMap lm = db::LayerMap::from_string_file_format ("metal1: 1\nvia1: 2\nmetal2: 3");
  options.set_layer_map (lm);

  db::LayerMap lm_read = run_test (_this, "via_properties", "lef:in.lef+def:in.def", "au.oas.gz", options, false);
  EXPECT_EQ (lm_read.to_string (),
    "layer_map('OUTLINE : OUTLINE (4/0)';'metal1.VIA : metal1 (1/0)';'metal2.VIA : metal2 (3/0)';'via1.VIA : via1 (2/0)')"
  )
}

TEST(113_masks_1)
{
  db::LEFDEFReaderOptions options = default_options ();
  options.set_routing_suffix ("");
  options.set_routing_datatype_per_mask (1, 100);
  options.set_routing_datatype_per_mask (2, 200);
  options.set_special_routing_suffix ("");
  options.set_special_routing_datatype_per_mask (1, 101);
  options.set_special_routing_datatype_per_mask (2, 201);
  options.set_via_geometry_suffix ("");
  options.set_via_geometry_datatype_per_mask (1, 102);
  options.set_via_geometry_datatype_per_mask (2, 202);
  options.set_pins_suffix ("");
  options.set_pins_datatype_per_mask (1, 110);
  options.set_pins_datatype_per_mask (2, 210);
  options.set_cell_outline_layer ("OUTLINE (4/0)");

  db::LayerMap lm = db::LayerMap::from_string_file_format ("M1: 3\nM0PO: 1\nVIA0: 2");
  options.set_layer_map (lm);

  db::LayerMap lm_read = run_test (_this, "masks-1", "lef:in_tech.lef+def:in.def", "au.oas.gz", options, false);

  EXPECT_EQ (lm_read.to_string_file_format (),
    "OUTLINE : OUTLINE (4/0)\n"
    "'M0PO.SPNET:1' : M0PO (1/101)\n"
    "'M1.SPNET:2' : M1 (3/201)\n"
    "'M1.SPNET:1' : M1 (3/101)\n"
    "'M0PO.VIA:2' : M0PO (1/202)\n"
    "'M1.VIA:1' : M1 (3/102)\n"
    "'VIA0.VIA:1' : VIA0 (2/102)\n"
    "'M0PO.SPNET:2' : M0PO (1/201)\n"
    "M0PO.PIN : M0PO (1/2)\n"
    "M0PO.LABEL : M0PO.LABEL (1/1)\n"
    "'M0PO.PIN:2' : M0PO (1/210)\n"
    "'M1.PIN:1' : M1 (3/110)\n"
    "M1.LABEL : M1.LABEL (3/1)\n"
    "'M1.NET:1' : M1 (3/100)\n"
    "'M1.NET:2' : M1 (3/200)\n"
    "'M0PO.VIA:1' : M0PO (1/102)\n"
    "'M1.VIA:2' : M1 (3/202)\n"
    "'VIA0.VIA:2' : VIA0 (2/202)\n"
    "'M0PO.NET:1' : M0PO (1/100)\n"
  )

  options = default_options ();
  lm_read = run_test (_this, "masks-1", "map:in.map+lef:in_tech.lef+def:in.def", "au_map.oas.gz", options, false);

  EXPECT_EQ (lm_read.to_string_file_format (),
    "OUTLINE : OUTLINE (4/0)\n"
    "'M0PO.NET:1' : 'M0PO.NET:1' (1/100)\n"
    "'M0PO.NET:2' : 'M0PO.NET:2' (1/200)\n"
    "M0PO.PIN : M0PO.PIN (1/2)\n"
    "'M0PO.PIN:1' : 'M0PO.PIN:1' (1/110)\n"
    "'M0PO.PIN:2' : 'M0PO.PIN:2' (1/210)\n"
    "'M0PO.SPNET:1' : 'M0PO.SPNET:1' (1/101)\n"
    "'M0PO.SPNET:2' : 'M0PO.SPNET:2' (1/201)\n"
    "'M0PO.VIA:1' : 'M0PO.VIA:1' (1/102)\n"
    "'M0PO.VIA:2' : 'M0PO.VIA:2' (1/202)\n"
    "M0PO.LABEL;M0PO.LEFLABEL : 'M0PO.LABEL/M0PO.LEFLABEL' (1/1)\n"
    "'M1.NET:1' : 'M1.NET:1' (3/100)\n"
    "'M1.NET:2' : 'M1.NET:2' (3/200)\n"
    "M1.PIN : M1.PIN (3/2)\n"
    "'M1.PIN:1' : 'M1.PIN:1' (3/110)\n"
    "'M1.PIN:2' : 'M1.PIN:2' (3/210)\n"
    "'M1.SPNET:1' : 'M1.SPNET:1' (3/101)\n"
    "'M1.SPNET:2' : 'M1.SPNET:2' (3/201)\n"
    "'M1.VIA:1' : 'M1.VIA:1' (3/102)\n"
    "'M1.VIA:2' : 'M1.VIA:2' (3/202)\n"
    "M1.LABEL;M1.LEFLABEL : 'M1.LABEL/M1.LEFLABEL' (3/1)\n"
    "'VIA0.NET:1' : 'VIA0.NET:1' (2/100)\n"
    "'VIA0.NET:2' : 'VIA0.NET:2' (2/200)\n"
    "VIA0.PIN : VIA0.PIN (2/2)\n"
    "'VIA0.PIN:1' : 'VIA0.PIN:1' (2/110)\n"
    "'VIA0.PIN:2' : 'VIA0.PIN:2' (2/210)\n"
    "'VIA0.SPNET:1' : 'VIA0.SPNET:1' (2/101)\n"
    "'VIA0.SPNET:2' : 'VIA0.SPNET:2' (2/201)\n"
    "'VIA0.VIA:1' : 'VIA0.VIA:1' (2/102)\n"
    "'VIA0.VIA:2' : 'VIA0.VIA:2' (2/202)\n"
    "VIA0.LABEL;VIA0.LEFLABEL : 'VIA0.LABEL/VIA0.LEFLABEL' (2/1)\n"
  )
}

TEST(114_lef_skips_end_library)
{
  db::LEFDEFReaderOptions opt = default_options ();
  opt.set_produce_pin_names (true);
  opt.set_pin_property_name (2);
  opt.set_cell_outline_layer ("OUTLINE (13/0)");
  run_test (_this, "lef-skips-end-library", "lef:in.lef+def:in.def", "au.oas", opt, false);
}

TEST(115_componentmaskshift)
{
  db::LEFDEFReaderOptions options = default_options ();
  options.set_map_file ("in.map");

  run_test (_this, "masks-2", "lef:in_tech.lef+lef:in.lef+def:in.def", "au.oas.gz", options, false);
}

TEST(116_layer_mapping)
{
  db::LEFDEFReaderOptions options = default_options ();
  db::LayerMap lm = db::LayerMap::from_string_file_format ("metal1: 1\nvia1: 2\nmetal2: 3\nOUTLINE: 42/17");
  options.set_layer_map (lm);

  {
    db::Layout layout;
    db::LayerMap lm_read = read (layout, "via_properties", "lef:in.lef+def:in.def", options, false);
    EXPECT_EQ (lm_read.to_string (),
      "layer_map('OUTLINE : OUTLINE (42/17)';'metal1.VIA : metal1 (1/0)';'metal2.VIA : metal2 (3/0)';'via1.VIA : via1 (2/0)')"
    )
  }

  options.set_layer_map (db::LayerMap ());

  {
    db::Layout layout;
    db::LayerMap lm_read = read (layout, "via_properties", "lef:in.lef+def:in.def", options, false);
    EXPECT_EQ (lm_read.to_string (),
      "layer_map('OUTLINE : OUTLINE (4/0)';'metal1.VIA : metal1 (1/0)';'metal2.VIA : metal2 (3/0)';'via1.VIA : via1 (2/0)')"
    )
  }

  lm = db::LayerMap::from_string_file_format ("metal1: M1\nmetal1.V: M1_V\nvia1: V1\nmetal2: M2\nOUTLINE: OUTL");
  options.set_layer_map (lm);
  options.set_via_geometry_suffix ("V");
  options.set_via_geometry_datatype (42);

  {
    db::Layout layout;
    db::LayerMap lm_read = read (layout, "via_properties", "lef:in.lef+def:in.def", options, false);
    EXPECT_EQ (lm_read.to_string (),
      "layer_map('OUTLINE : OUTL (4/0)';'metal1.VIA : M1V (1/42)';'metal2.VIA : M2V (3/42)';'via1.VIA : V1V (2/42)')"
    )
  }

  lm = db::LayerMap::from_string_file_format ("metal1: M1\nmetal1.V: M1_V\nvia1: V1\nmetal2: M2");
  options.set_layer_map (lm);
  options.set_via_geometry_suffix ("V");
  options.set_via_geometry_datatype (42);
  options.set_read_all_layers (false);

  {
    db::Layout layout;
    db::LayerMap lm_read = read (layout, "via_properties", "lef:in.lef+def:in.def", options, false);
    EXPECT_EQ (lm_read.to_string (),
      "layer_map('metal1.VIA : M1V (1/42)';'metal2.VIA : M2V (3/42)';'via1.VIA : V1V (2/42)')"
    )
  }

  lm = db::LayerMap::from_string_file_format ("metal2: M2 (17/1)");
  options.set_layer_map (lm);

  {
    db::Layout layout;
    db::LayerMap lm_read = read (layout, "via_properties", "lef:in.lef+def:in.def", options, false);
    EXPECT_EQ (lm_read.to_string (),
      "layer_map('metal2.VIA : M2V (17/43)')"
    )
  }

  options.set_produce_via_geometry (false);

  {
    db::Layout layout;
    db::LayerMap lm_read = read (layout, "via_properties", "lef:in.lef+def:in.def", options, false);
    EXPECT_EQ (lm_read.to_string (),
      "layer_map()"
    )
  }

  options.set_produce_via_geometry (true);
  options.set_via_geometry_suffix (".V");
  lm = db::LayerMap::from_string_file_format ("metal2.V: 17/1");
  options.set_layer_map (lm);

  {
    db::Layout layout;
    db::LayerMap lm_read = read (layout, "via_properties", "lef:in.lef+def:in.def", options, false);
    EXPECT_EQ (lm_read.to_string (),
      "layer_map('metal2.VIA : metal2.V (17/1)')"
    )
  }

  lm = db::LayerMap::from_string_file_format ("metal2.V: m2v (17/5)");
  options.set_layer_map (lm);

  {
    db::Layout layout;
    db::LayerMap lm_read = read (layout, "via_properties", "lef:in.lef+def:in.def", options, false);
    EXPECT_EQ (lm_read.to_string (),
      "layer_map('metal2.VIA : m2v (17/5)')"
    )
  }

  lm = db::LayerMap::from_string_file_format ("OUTLINE: OUTL");
  options.set_layer_map (lm);
  options.set_cell_outline_layer ("OUTLINE (42/17)");

  {
    db::Layout layout;
    db::LayerMap lm_read = read (layout, "via_properties", "lef:in.lef+def:in.def", options, false);
    EXPECT_EQ (lm_read.to_string (),
      "layer_map('OUTLINE : OUTL (42/17)')"
    )
  }

  lm = db::LayerMap::from_string_file_format ("OUTLINE: OUTL (18/1)");
  options.set_layer_map (lm);
  options.set_cell_outline_layer ("OUTLINE (42/17)");

  {
    db::Layout layout;
    db::LayerMap lm_read = read (layout, "via_properties", "lef:in.lef+def:in.def", options, false);
    EXPECT_EQ (lm_read.to_string (),
      "layer_map('OUTLINE : OUTL (18/1)')"
    )
  }

  options.set_cell_outline_layer ("OUTLINE (42/17)");
  lm = db::LayerMap::from_string_file_format ("42/17: OUTL (18/1)");
  options.set_layer_map (lm);

  {
    db::Layout layout;
    db::LayerMap lm_read = read (layout, "via_properties", "lef:in.lef+def:in.def", options, false);
    EXPECT_EQ (lm_read.to_string (),
      "layer_map('OUTLINE : OUTL (18/1)')"
    )
  }

  options.set_cell_outline_layer ("42/17");
  lm = db::LayerMap::from_string_file_format ("42/17: 18/1");
  options.set_layer_map (lm);

  {
    db::Layout layout;
    db::LayerMap lm_read = read (layout, "via_properties", "lef:in.lef+def:in.def", options, false);
    EXPECT_EQ (lm_read.to_string (),
      "layer_map('OUTLINE : OUTLINE (18/1)')"
    )
  }
}

TEST(117_mapfile_all)
{
  db::LEFDEFReaderOptions options = default_options ();

  db::Layout layout;
  db::LayerMap lm_read = read (layout, "mapfile", "lef:in.lef+def:in.def+map:all.map", options, false);
  EXPECT_EQ (lm_read.to_string (),
    "layer_map("
      "'OUTLINE : OUTLINE (1/0)';"
      "'+M1.LEFOBS;M1.LEFPIN;M1.NET;M1.PIN;M1.SPNET;M1.VIA : \\'M1.NET/PIN/SPNET/...\\' (1/5)';"
      "'+M1.NET;M1.SPNET : \\'M1.NET/SPNET\\' (16/0)';"
      "'+M1.NET : M1.NET (18/0)';"
      "'+M1.LEFPIN;M1.NET;M1.PIN;M1.SPNET;M1.VIA : \\'M1.NET/PIN/SPNET/...\\' (22/2)';"
      "'+\\'M1.NET:1\\';\\'M1.PIN:1\\';\\'M1.SPNET:1\\';\\'M1.VIA:1\\' : \\'M1.NET:1/PIN:1/...\\' (6/0)';"
      "'+\\'M1.NET:1\\' : \\'M1.NET:1\\' (7/0)';"
      "'+M1.PIN : M1.PIN (3/0)';"
      "'+M1.PIN : M1.PIN (4/0)';"
      "'+M1.FILL : M1.FILL (14/0)';"
      "'+M1.FILL : M1.FILL (15/0)';"
      "'+M1.FILL : M1.FILL (17/0)';"
      "'M1.FILLOPC : M1.FILLOPC (9/0)';"
      "'\\'M1.FILLOPC:1\\' : \\'M1.FILLOPC:1\\' (10/0)';"
      "'\\'M1.FILLOPC:2\\' : \\'M1.FILLOPC:2\\' (11/0)';"
      "'\\'M1.VIA:SIZE0.05X0.05\\' : \\'M1.VIA:SIZE0.05X0.05\\' (20/0)';"
      "'\\'M1.VIA:SIZE3X3\\' : \\'M1.VIA:SIZE3X3\\' (21/0)';"
      "'M1.LABEL : M1.LABEL (26/0)';"
      // NAME M1/NET not supported: "'+M1.LABEL : M1.LABEL (27/0)';"
      // NAME M1/SPNET not supported: "'+M1.LABEL : M1.LABEL (28/1)';"
      "'M1.BLK : M1.BLK (13/0)';"
      "'M1_TEXT.LABEL;M1_TEXT.LEFLABEL : \\'M1_TEXT.LABEL/M1_TEXT.LEFLABEL\\' (29/0)'"
    ")"
  )
}

TEST(118_density)
{
  run_test (_this, "density", "read:in.lef", "au.oas.gz", default_options (), false);
}

TEST(119_multimapping)
{
  db::LEFDEFReaderOptions options = default_options ();
  db::LayerMap lm = db::LayerMap::from_string_file_format ("(M1:1/0)\n(M2:3/0)\n+(M1:100/0)\n+(M2:100/0)\n(VIA1:2/0)");
  options.set_layer_map (lm);

  db::LayerMap lm_read = run_test (_this, "multimap", "def:test.def", "au.oas.gz", options, false);
  EXPECT_EQ (lm_read.to_string (),
    "layer_map("
      "'OUTLINE : OUTLINE (4/0)';"
      "'+M1.VIA : M1 (1/0)';"
      "'+M1.VIA;M2.VIA : \\'M1;M2\\' (100/0)';"
      "'+M2.VIA : M2 (3/0)';"
      "'VIA1.VIA : VIA1 (2/0)'"
    ")"
  )
}

TEST(120_simplefill)
{
  run_test (_this, "fill", "map:simple.map+lef:simple.lef+def:simple.def", "simple_au.oas.gz", default_options (), false);
}

TEST(121_fillwithmask)
{
  run_test (_this, "fill", "map:with_mask.map+lef:with_mask.lef+def:with_mask.def", "with_mask_au.oas.gz", default_options (), false);
}

TEST(130_viasize)
{
  run_test (_this, "viasize", "map:test.map+lef:test.lef+def:test.def", "au.oas.gz", default_options (), false);
}

//  issue-1065
TEST(130_viasize2)
{
  run_test (_this, "viasize2", "map:test_ok.map+lef:test.lef+def:test.def", "au.oas.gz", default_options (), false);
  run_test (_this, "viasize2", "map:test_fail.map+lef:test.lef+def:test.def", "au.oas.gz", default_options (), false);
}

TEST(131_patternname)
{
  run_test (_this, "patternname", "map:v.map+lef:v.lef+def:v.def", "au.oas.gz", default_options (), false);
}

TEST(132_issue1307_pin_names)
{
  db::LEFDEFReaderOptions opt = default_options ();
  opt.set_produce_pin_names (true);
  opt.set_pin_property_name (2);
  opt.set_cell_outline_layer ("OUTLINE (13/0)");
  run_test (_this, "issue-1307c", "lef:in.lef+def:in.def", "au.oas", opt, false);
}

TEST(200_lefdef_plugin)
{
  db::Layout ly;

  std::string fn_path (tl::testdata ());
  fn_path += "/lefdef/masks-1/";

  db::LEFDEFReaderOptions lefdef_opt = default_options ();
  lefdef_opt.set_map_file ("in.map");
  db::LoadLayoutOptions opt;
  opt.set_options (lefdef_opt);

  {
    tl::InputStream is (fn_path + "in.def");
    db::Reader reader (is);
    reader.read (ly, opt);
  }

  db::compare_layouts (_this, ly, fn_path + "au_plugin_def.oas.gz", db::WriteOAS);
}

TEST(201_lefdef_plugin_explicit_lef)
{
  db::Layout ly;

  std::string fn_path (tl::testdata ());
  fn_path += "/lefdef/masks-1/";

  db::LEFDEFReaderOptions lefdef_opt = default_options ();
  lefdef_opt.set_map_file ("in.map");
  std::vector<std::string> lf;
  lf.push_back ("hidden/in_tech.lef");
  lefdef_opt.set_lef_files (lf);
  lefdef_opt.set_read_lef_with_def (false);
  db::LoadLayoutOptions opt;
  opt.set_options (lefdef_opt);

  {
    tl::InputStream is (fn_path + "in.def");
    db::Reader reader (is);
    reader.read (ly, opt);
  }

  db::compare_layouts (_this, ly, fn_path + "au_plugin_alt_lef.oas.gz", db::WriteOAS);
}

TEST(202_lefdef_blend_mode)
{
  db::LEFDEFReaderOptions lefdef_opt = default_options ();

  run_test2 (_this, "blend_mode", "map:layers.map+lef:sub.lef+def:top.def", "map:layers.map+def:sub.def", "au1.oas.gz", lefdef_opt, false);
  run_test2 (_this, "blend_mode", "map:layers.map+lef:sub.lef+def:top.def", "map:layers.map+def:sub.def", "au2.oas.gz", lefdef_opt, false, db::AddToCell);

  lefdef_opt.set_macro_resolution_mode (2);
  run_test2 (_this, "blend_mode", "map:layers.map+lef:sub.lef+def:top.def", "map:layers.map+def:sub.def", "au3.oas.gz", lefdef_opt, false);
}

TEST(203_regionsAndMapfileConcat)
{
  db::LEFDEFReaderOptions lefdef_opt = default_options ();

  run_test (_this, "map_regions", "map:'test.map,test.add.map'+lef:test.lef+def:test.def", "au.oas.gz", lefdef_opt, false);
}

//  issue 1132
TEST(204_concave_pins)
{
  db::LEFDEFReaderOptions lefdef_opt = default_options ();
  lefdef_opt.set_lef_pins_datatype (12);
  lefdef_opt.set_lef_pins_suffix (".LEFPIN");
  lefdef_opt.set_lef_labels_datatype (11);
  lefdef_opt.set_lef_labels_suffix (".LEFLABEL");

  run_test (_this, "issue-1132", "read:test.lef", "au.oas.gz", lefdef_opt, false);
}

//  issue 1214
TEST(205_lef_resistance)
{
  db::LEFDEFReaderOptions lefdef_opt = default_options ();
  lefdef_opt.set_lef_pins_datatype (12);
  lefdef_opt.set_lef_pins_suffix (".LEFPIN");
  lefdef_opt.set_lef_labels_datatype (11);
  lefdef_opt.set_lef_labels_suffix (".LEFLABEL");

  run_test (_this, "issue-1214", "read:merged.nom.lef", "au.oas.gz", lefdef_opt, false);
}

//  issue 1282
TEST(206_lef_spacing)
{
  run_test (_this, "issue-1282", "read:a.lef", 0, default_options (), false);
}

//  issue-1345
TEST(207_joined_paths)
{
  db::LEFDEFReaderOptions lefdef_opt = default_options ();
  lefdef_opt.set_joined_paths (true);
  run_test (_this, "issue-1345", "lef:in.lef+def:in.def", "au.oas.gz", lefdef_opt, false);

  run_test (_this, "issue-1345", "lef:in.lef+def:in.def", "au-nojoin.oas.gz", default_options (), false);
}

//  issue-1432
TEST(208_nets_and_rects)
{
  run_test (_this, "issue-1432", "map:test.map+lef:test.lef+def:test.def", "au.oas", default_options (), false);
}

//  issue-1472
TEST(209_invalid_split_paths)
{
  run_test (_this, "issue-1472", "map:tech.map+lef:tech.lef.gz+def:test.def.gz", "au.oas", default_options (), false);
}

//  issue-1499
TEST(210_overlaps)
{
  run_test (_this, "issue-1499", "map:tech.map+lef:tech.lef+lef:blocks.lef+def:top.def", "au.oas", default_options (), false);
}

//  issue-1531
TEST(211_symlinks)
{
  db::Layout ly;

  std::string fn_path (tl::testdata ());
  fn_path += "/lefdef/issue-1531/";

  db::LEFDEFReaderOptions lefdef_opt = default_options ();
  lefdef_opt.set_map_file ("tech.map");
  std::vector<std::string> lf;
  lf.push_back ("tech.lef");
  lf.push_back ("blocks.lef");
  lefdef_opt.set_lef_files (lf);
  lefdef_opt.set_read_lef_with_def (false);
  db::LoadLayoutOptions opt;
  opt.set_options (lefdef_opt);

  {
    tl::InputStream is (fn_path + "top.def");
    db::Reader reader (is);
    reader.read (ly, opt);
  }

  db::compare_layouts (_this, ly, fn_path + "au.oas", db::WriteOAS);
}

//  issue-1528
TEST(212_widthtable)
{
  run_test (_this, "issue-1528", "map:gds.map+lef:tech.lef+def:routed.def", "au.oas", default_options (), false);
}

//  issue-1724 (skip duplicate LEF)
TEST(213_no_duplicate_LEF)
{
  db::Layout ly;

  std::string fn_path (tl::testdata ());
  fn_path += "/lefdef/issue-1724/";

  db::LEFDEFReaderOptions lefdef_opt = default_options ();
  lefdef_opt.set_map_file ("tech.map");
  std::vector<std::string> lf;
  lf.push_back ("d/tech.lef");
  lf.push_back ("blocks.lef");
  lefdef_opt.set_lef_files (lf);
  lefdef_opt.set_read_lef_with_def (true);
  db::LoadLayoutOptions opt;
  opt.set_options (lefdef_opt);

  {
    tl::InputStream is (fn_path + "top.def");
    db::Reader reader (is);
    reader.read (ly, opt);
  }

  db::compare_layouts (_this, ly, fn_path + "au.oas", db::WriteOAS);
}

