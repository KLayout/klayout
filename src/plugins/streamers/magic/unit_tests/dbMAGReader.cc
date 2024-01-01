
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


#include "dbMAGReader.h"
#include "dbLayoutDiff.h"
#include "dbWriter.h"
#include "dbMAGWriter.h"
#include "tlUnitTest.h"

#include <stdlib.h>

static void run_test (tl::TestBase *_this, const std::string &base, const char *file, const char *file_au, const char *map = 0, double lambda = 0.1, double dbu = 0.001, const std::vector<std::string> *lib_paths = 0)
{
  db::MAGReaderOptions *opt = new db::MAGReaderOptions();
  opt->dbu = dbu;
  if (lib_paths) {
    opt->lib_paths = *lib_paths;
  }

  db::LayerMap lm;
  if (map) {
    unsigned int ln = 0;
    tl::Extractor ex (map);
    while (! ex.at_end ()) {
      std::string n;
      int l;
      ex.read_word_or_quoted (n);
      ex.test (":");
      ex.read (l);
      ex.test (",");
      lm.map (n, ln++, db::LayerProperties (l, 0));
    }
    opt->layer_map = lm;
    opt->create_other_layers = true;
  }

  db::LoadLayoutOptions options;
  options.set_options (opt);

  db::Manager m (false);
  db::Layout layout (&m), layout2 (&m), layout2_mag (&m), layout_au (&m);

  {
    std::string fn (base);
    fn += "/magic/";
    fn += file;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout, options);
  }

  std::string tc_name = layout.cell_name (*layout.begin_top_down ());

  //  normalize the layout by writing to GDS and reading from ..

  std::string tmp_cif_file = _this->tmp_file (tl::sprintf ("%s.cif", tc_name));
  std::string tmp_mag_file = _this->tmp_file (tl::sprintf ("%s.mag", tc_name));

  {
    tl::OutputStream stream (tmp_cif_file);
    db::SaveLayoutOptions options;
    options.set_format ("CIF");
    db::Writer writer (options);
    writer.write (layout, stream);
  }

  {
    tl::InputStream stream (tmp_cif_file);
    db::Reader reader (stream);
    reader.read (layout2);
  }

  //  normalize the layout by writing to MAG and reading from ..

  {
    tl::OutputStream stream (tmp_mag_file);

    db::MAGWriterOptions *opt = new db::MAGWriterOptions();
    opt->lambda = lambda;

    db::MAGWriter writer;
    db::SaveLayoutOptions options;
    options.set_options (opt);
    writer.write (layout, stream, options);
  }

  {
    tl::InputStream stream (tmp_mag_file);

    db::MAGReaderOptions *opt = new db::MAGReaderOptions();
    opt->dbu = dbu;
    opt->lambda = lambda;
    db::LoadLayoutOptions reread_options;
    reread_options.set_options (opt);

    db::Reader reader (stream);
    reader.read (layout2_mag, reread_options);

    layout2_mag.rename_cell (*layout2_mag.begin_top_down (), layout.cell_name (*layout.begin_top_down ()));
  }

  {
    std::string fn (base);
    fn += "/magic/";
    fn += file_au;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout_au);
  }

  bool equal = db::compare_layouts (layout2, layout_au, db::layout_diff::f_boxes_as_polygons | db::layout_diff::f_verbose | db::layout_diff::f_flatten_array_insts, 1);
  if (! equal) {
    _this->raise (tl::sprintf ("Compare failed after reading - see %s vs %s\n", tmp_cif_file, file_au));
  }

  equal = db::compare_layouts (layout, layout2_mag, db::layout_diff::f_boxes_as_polygons | db::layout_diff::f_verbose | db::layout_diff::f_flatten_array_insts, 1);
  if (! equal) {
    _this->raise (tl::sprintf ("Compare failed after writing - see %s vs %s\n", file, tmp_mag_file));
  }
}

TEST(1)
{
  run_test (_this, tl::testdata (), "MAG_TEST.mag.gz", "mag_test_au.cif.gz");
}

TEST(2)
{
  std::vector<std::string> lp;
  lp.push_back (std::string ("../.."));
  run_test (_this, tl::testdata (), "PearlRiver/Layout/magic/PearlRiver_die.mag", "PearlRiver_au.cif.gz", 0, 1.0, 0.001, &lp);
}

TEST(3)
{
  run_test (_this, tl::testdata (), "ringo/RINGO.mag", "ringo_au.cif.gz");
}

