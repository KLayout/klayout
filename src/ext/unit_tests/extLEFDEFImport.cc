
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


#include "dbLayoutDiff.h"
#include "dbWriter.h"
#include "dbOASISWriter.h"
#include "dbGDS2Writer.h"
#include "extDEFImporter.h"
#include "extLEFImporter.h"

#include "utHead.h"

#include <cstdlib>
#include <QDir>

static void run_test (ut::TestBase *_this, const char *lef_dir, const char *filename, const char *au)
{
  ext::LEFDEFReaderOptions tc;
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
  ext::LEFDEFLayerDelegate ld (&tc);

  db::Manager m;
  db::Layout layout (&m), layout2 (&m), layout_au (&m);

  tl::Extractor ex (filename);

  ld.prepare (layout);

  ext::DEFImporter imp;

  while (! ex.at_end ()) {

    if (ex.test ("def:")) {

      std::string fn (ut::testsrc_private ());
      fn += "/testdata/lefdef/";
      fn += lef_dir;
      fn += "/";
      std::string f;
      ex.read_word_or_quoted (f);
      fn += f;

      tl::InputStream stream (fn);
      imp.read (stream, layout, ld);

    } else if (ex.test ("lef:")) {

      std::string fn (ut::testsrc_private ());
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
    db::OASISWriter writer;
    db::SaveLayoutOptions options;
    writer.write (layout, stream, options);
  }

  {
    tl::InputStream stream (tmp_file);
    db::Reader reader (stream);
    reader.read (layout2);
  }

  if (au) {

    std::string fn (ut::testsrc_private ());
    fn += "/testdata/lefdef/";
    fn += lef_dir;
    fn += "/";
    fn += au;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout_au);

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
  run_test (_this, "lef1", "lef:in.lef", 0);
}

TEST(2)
{
  run_test (_this, "lef2", "lef:in.lef", "au.oas.gz");
}

TEST(3)
{
  run_test (_this, "lef3", "lef:in.lef", "au.oas.gz");
}

TEST(4)
{
  run_test (_this, "lef4", "lef:in.lef", 0);
}

TEST(5)
{
  run_test (_this, "lef5", "lef:in.lef", 0);
}

TEST(6)
{
  run_test (_this, "lef6", "lef:in.lef", 0);
}

TEST(7)
{
  run_test (_this, "lef7", "lef:in_tech.lef+lef:in.lef", "au.oas.gz");
}

TEST(10)
{
  run_test (_this, "def1", "lef:in.lef+def:in.def", "au.oas.gz");
}

TEST(11)
{
  run_test (_this, "def2", "lef:0.lef+lef:1.lef+def:in.def.gz", "au.oas.gz");
}

TEST(12)
{
  run_test (_this, "def3", "lef:in.lef+def:in.def", "au.oas.gz");
}

TEST(13)
{
  run_test (_this, "def4", "lef:in.lef+def:in.def", "au.oas.gz");
}

TEST(14)
{
  run_test (_this, "def5", "lef:in.lef+def:in.def", "au.oas.gz");
}

TEST(15)
{
  run_test (_this, "def6", "lef:cells.lef+lef:tech.lef+def:in.def.gz", "au.oas.gz");
}

TEST(16)
{
  run_test (_this, "def7", "lef:cells.lef+lef:tech.lef+def:in.def.gz", "au.oas.gz");
}

TEST(17)
{
  run_test (_this, "def8", "lef:tech.lef+def:in.def", "au.oas.gz");
}
