
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


#include "dbMAGReader.h"
#include "dbLayoutDiff.h"
#include "dbWriter.h"
#include "dbMAGWriter.h"
#include "tlUnitTest.h"
#include "tlEnv.h"
#include "tlFileUtils.h"

#include <stdlib.h>

static void run_test (tl::TestBase *_this, const std::string &base, const char *file, const char *file_au, const db::LayerMap &lm = db::LayerMap (), bool create_other_layers = false, double lambda = 0.1, double write_lambda = 0.0, double dbu = 0.001, const std::vector<std::string> *lib_paths = 0, const std::string &tmp_format = "CIF", bool with_write_test = true)
{
  db::MAGReaderOptions *opt = new db::MAGReaderOptions();
  opt->dbu = dbu;
  opt->lambda = lambda;
  if (lib_paths) {
    opt->lib_paths = *lib_paths;
  }
  opt->layer_map = lm;
  opt->create_other_layers = create_other_layers;

  if (write_lambda == 0) {
    write_lambda = lambda;
  }

  db::LoadLayoutOptions options;
  options.set_options (opt);

  db::Manager m (false);
  db::Layout layout (&m), layout2 (&m), layout_au (&m);

  {
    std::string fn (base);
    fn += "/magic/";
    fn += file;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout, options);
  }

  std::string tc_name = layout.cell_name (*layout.begin_top_down ());

  //  normalize the layout by writing to an intermediate file and reading from ..

  std::string tmp_intermediate_file = _this->tmp_file (tl::sprintf ("%s.%s", tc_name, tl::to_lower_case (tmp_format)));
  tl::info << "Temp file " << tmp_intermediate_file;
  std::string tmp_mag_file = _this->tmp_file (tl::sprintf ("%s.mag", tc_name));

  {
    tl::OutputStream stream (tmp_intermediate_file);
    db::SaveLayoutOptions options;
    options.set_format (tmp_format);
    db::Writer writer (options);
    writer.write (layout, stream);
  }

  {
    tl::InputStream stream (tmp_intermediate_file);
    db::Reader reader (stream);
    reader.read (layout2);
  }

  //  normalize the layout by writing to MAG and reading from ..

  {
    tl::OutputStream stream (tmp_mag_file);

    db::MAGWriterOptions *opt = new db::MAGWriterOptions();
    opt->lambda = write_lambda;

    db::MAGWriter writer;
    db::SaveLayoutOptions options;
    options.set_options (opt);
    writer.write (layout, stream, options);
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
    _this->raise (tl::sprintf ("Compare failed after reading - see %s vs %s\n", tmp_intermediate_file, file_au));
  }

  if (with_write_test) {

    db::Layout layout2_mag (&m);

    {
      tl::InputStream stream (tmp_mag_file);

      db::MAGReaderOptions *opt = new db::MAGReaderOptions();
      opt->dbu = dbu;
      opt->lambda = write_lambda;
      db::LoadLayoutOptions reread_options;
      reread_options.set_options (opt);

      db::Reader reader (stream);
      reader.read (layout2_mag, reread_options);

      layout2_mag.rename_cell (*layout2_mag.begin_top_down (), layout.cell_name (*layout.begin_top_down ()));
    }

    equal = db::compare_layouts (layout, layout2_mag, db::layout_diff::f_boxes_as_polygons | db::layout_diff::f_verbose | db::layout_diff::f_flatten_array_insts, 1);
    if (! equal) {
      _this->raise (tl::sprintf ("Compare failed after writing - see %s vs %s\n", file, tmp_mag_file));
    }

  }
}

TEST(1)
{
  run_test (_this, tl::testdata (), "MAG_TEST.mag.gz", "mag_test_au.cif.gz", db::LayerMap (), true, 1.0, 0.1);
}

TEST(2)
{
  std::vector<std::string> lp;
  lp.push_back (std::string ("../.."));
  run_test (_this, tl::testdata (), "PearlRiver/Layout/magic/PearlRiver_die.mag", "PearlRiver_au.cif.gz", db::LayerMap (), true, 1.0, 0.0, 0.001, &lp);
}

TEST(3)
{
  run_test (_this, tl::testdata (), "ringo/RINGO.mag", "ringo_au.cif.gz", db::LayerMap (), true, 1.0, 0.1);
}

TEST(4)
{
  run_test (_this, tl::testdata (), "issue_1925/redux.mag", "redux_au.cif.gz", db::LayerMap (), true, 1.0, 0.1);
}

TEST(5)
{
  tl::set_env ("__TESTSRC_ABSPATH", tl::absolute_file_path (tl::testsrc ()));
  run_test (_this, tl::testdata (), "gf180mcu_ocd_sram_test/gf180mcu_ocd_sram_top.mag.gz", "gf180mcu_ocd_sram_test.cif.gz", db::LayerMap (), true, 0.05, 0.005);
}

//  issue #2438
TEST(6)
{
  db::LayerMap lm = db::LayerMap::from_string_file_format ("metal1:100/0");
  run_test (_this, tl::testdata (), "gf180mcu_ocd_sram_test/gf180mcu_ocd_sram_top.mag.gz", "issue_2438_one.gds", lm, false, 0.05, 0.005, 0.001, 0, "GDS2", false);
  run_test (_this, tl::testdata (), "gf180mcu_ocd_sram_test/gf180mcu_ocd_sram_top.mag.gz", "issue_2438_all.gds", lm, true, 0.05, 0.005, 0.001, 0, "GDS2", false);
}

