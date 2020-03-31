
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


#include "dbLayoutDiff.h"
#include "dbWriter.h"
#include "dbDEFImporter.h"
#include "dbLEFImporter.h"

#include "tlUnitTest.h"

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

static void run_test (tl::TestBase *_this, const char *lef_dir, const char *filename, const char *au, const db::LEFDEFReaderOptions &tc, bool priv = true)
{
  db::LEFDEFLayerDelegate ld (&tc);

  db::Manager m (false);
  db::Layout layout (&m), layout2 (&m), layout_au (&m);

  tl::Extractor ex (filename);

  ld.prepare (layout);

  db::DEFImporter imp;

  while (! ex.at_end ()) {

    if (ex.test ("def:")) {

      std::string fn (priv ? tl::testsrc_private () : tl::testsrc ());
      fn += "/testdata/lefdef/";
      fn += lef_dir;
      fn += "/";
      std::string f;
      ex.read_word_or_quoted (f);
      fn += f;

      tl::InputStream stream (fn);
      imp.read (stream, layout, ld);

    } else if (ex.test ("lef:")) {

      std::string fn (priv ? tl::testsrc_private () : tl::testsrc ());
      fn += "/testdata/lefdef/";
      fn += lef_dir;
      fn += "/";
      std::string f;
      ex.read_word_or_quoted (f);
      fn += f;

      tl::InputStream stream (fn);
      imp.read_lef (stream, layout, ld);

    } else {

      break;

    }

    if (! ex.test("+")) {
      break;
    }

  }

  ld.finish (layout);

  //  normalize the layout by writing to OASIS and reading from ..

  std::string tmp_file = _this->tmp_file ("tmp.oas");

  {
    tl::OutputStream stream (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("OASIS");
    db::Writer writer (options);
    writer.write (layout, stream);
  }

  {
    tl::InputStream stream (tmp_file);
    db::Reader reader (stream);
    reader.read (layout2);
  }

  if (au) {

    std::string fn (priv ? tl::testsrc_private () : tl::testsrc ());
    fn += "/testdata/lefdef/";
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

TEST(1)
{
  run_test (_this, "lef1", "lef:in.lef", 0, default_options ());
}

TEST(2)
{
  run_test (_this, "lef2", "lef:in.lef", "au.oas.gz", default_options ());
}

TEST(3)
{
  run_test (_this, "lef3", "lef:in.lef", "au.oas.gz", default_options ());
}

TEST(4)
{
  run_test (_this, "lef4", "lef:in.lef", 0, default_options ());
}

TEST(5)
{
  run_test (_this, "lef5", "lef:in.lef", 0, default_options ());
}

TEST(6)
{
  run_test (_this, "lef6", "lef:in.lef", 0, default_options ());
}

TEST(7)
{
  run_test (_this, "lef7", "lef:in_tech.lef+lef:in.lef", "au.oas.gz", default_options ());
}

TEST(10)
{
  run_test (_this, "def1", "lef:in.lef+def:in.def", "au.oas.gz", default_options ());
}

TEST(11)
{
  run_test (_this, "def2", "lef:0.lef+lef:1.lef+def:in.def.gz", "au.oas.gz", default_options ());
}

TEST(12)
{
  run_test (_this, "def3", "lef:in.lef+def:in.def", "au.oas.gz", default_options ());
}

TEST(13)
{
  run_test (_this, "def4", "lef:in.lef+def:in.def", "au.oas.gz", default_options ());
}

TEST(14)
{
  run_test (_this, "def5", "lef:in.lef+def:in.def", "au.oas.gz", default_options ());
}

TEST(15)
{
  run_test (_this, "def6", "lef:cells.lef+lef:tech.lef+def:in.def.gz", "au.oas.gz", default_options ());
}

TEST(16)
{
  run_test (_this, "def7", "lef:cells.lef+lef:tech.lef+def:in.def.gz", "au.oas.gz", default_options ());
}

TEST(17)
{
  run_test (_this, "def8", "lef:tech.lef+def:in.def", "au.oas.gz", default_options ());
}

TEST(18)
{
  run_test (_this, "def9", "lef:tech.lef+lef:cells_modified.lef+def:in.def", "au.oas.gz", default_options ());
}

TEST(19)
{
  run_test (_this, "def10", "def:in.def", "au.oas.gz", default_options ());
}

TEST(20)
{
  run_test (_this, "def11", "lef:test.lef+def:test.def", "au.oas.gz", default_options ());
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
  run_test (_this, "issue-489", "lef:in.lef+def:in.def", "au.oas", opt, false);
}

TEST(102)
{
  db::LEFDEFReaderOptions opt = default_options ();
  opt.set_produce_pin_names (true);
  opt.set_pin_property_name (3);
  run_test (_this, "issue-489b", "lef:in_tech.lef+lef:in.lef", "au.oas.gz", opt, false);
}

TEST(103)
{
  run_test (_this, "issue-517", "def:in.def", "au.oas.gz", default_options (), false);
}

TEST(104)
{
  run_test (_this, "doxy_vias", "def:test.def", "au.oas.gz", default_options (), false);
}
