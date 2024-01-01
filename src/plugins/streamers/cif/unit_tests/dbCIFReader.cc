
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


#include "dbCIFReader.h"
#include "dbLayoutDiff.h"
#include "dbWriter.h"
#include "dbCIFWriter.h"
#include "tlUnitTest.h"

#include <stdlib.h>

static void run_test (tl::TestBase *_this, const std::string &base, const char *file, const char *file_au, const char *map = 0, double dbu = 0.001, bool dummy_calls = false, bool blank_sep = false)
{
  db::CIFReaderOptions *opt = new db::CIFReaderOptions();
  opt->dbu = dbu;

  db::LayerMap lm;
  if (map) {
    unsigned int ln = 0;
    tl::Extractor ex (map);
    while (! ex.at_end ()) {
      lm.add_expr (ex, ln++);
      ex.test (",");
    }
    opt->layer_map = lm;
    opt->create_other_layers = true;
  }

  db::LoadLayoutOptions options;
  options.set_options (opt);

  db::Manager m (false);
  db::Layout layout (&m), layout2 (&m), layout2_cif (&m), layout_au (&m);

  {
    std::string fn (base);
    fn += "/cif/";
    fn += file;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout, options);
  }

  //  generate a "unique" name ...
  unsigned int hash = 0;
  for (const char *cp = file_au; *cp; ++cp) {
    hash = (hash << 4) ^ (hash >> 4) ^ ((unsigned int) *cp);
  }

  //  normalize the layout by writing to GDS and reading from ..

  std::string tmp_gds_file = _this->tmp_file (tl::sprintf ("tmp_%x.gds", hash));
  std::string tmp_cif_file = _this->tmp_file (tl::sprintf ("tmp_%x.cif", hash));

  {
    tl::OutputStream stream (tmp_gds_file);
    db::SaveLayoutOptions options;
    options.set_format ("GDS2");
    db::Writer writer (options);
    writer.write (layout, stream);
  }

  {
    tl::InputStream stream (tmp_gds_file);
    db::Reader reader (stream);
    reader.read (layout2);
  }

  //  normalize the layout by writing to CIF and reading from ..
  {
    tl::OutputStream stream (tmp_cif_file);

    db::CIFWriterOptions *opt = new db::CIFWriterOptions();
    opt->dummy_calls = dummy_calls;
    opt->blank_separator = blank_sep;

    db::CIFWriter writer;
    db::SaveLayoutOptions options;
    options.set_options (opt);
    writer.write (layout, stream, options);
  }

  {
    tl::InputStream stream (tmp_cif_file);

    db::CIFReaderOptions *opt = new db::CIFReaderOptions();
    opt->dbu = dbu;
    db::LoadLayoutOptions reread_options;
    reread_options.set_options (opt);

    db::Reader reader (stream);
    reader.read (layout2_cif, reread_options);
  }

  {
    std::string fn (base);
    fn += "/cif/";
    fn += file_au;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout_au);
  }

  bool equal = db::compare_layouts (layout2, layout_au, db::layout_diff::f_boxes_as_polygons | db::layout_diff::f_verbose | db::layout_diff::f_flatten_array_insts, 1);
  if (! equal) {
    _this->raise (tl::sprintf ("Compare failed after reading - see %s vs %s\n", tmp_gds_file, file_au));
  }

  equal = db::compare_layouts (layout, layout2_cif, db::layout_diff::f_boxes_as_polygons | db::layout_diff::f_verbose | db::layout_diff::f_flatten_array_insts, 1);
  if (! equal) {
    _this->raise (tl::sprintf ("Compare failed after writing - see %s vs %s\n", file, tmp_cif_file));
  }
}

static void run_test2 (tl::TestBase *_this, const std::string &base, db::Layout &layout, const char *file_au, const char *file_au_cif, const char *map = 0, double dbu = 0.001, bool dummy_calls = false, bool blank_sep = false)
{
  db::CIFReaderOptions *opt = new db::CIFReaderOptions();
  opt->dbu = dbu;

  db::LayerMap lm;
  if (map) {
    unsigned int ln = 0;
    tl::Extractor ex (map);
    while (! ex.at_end ()) {
      lm.add_expr (ex, ln++);
      ex.test (",");
    }
    opt->layer_map = lm;
    opt->create_other_layers = true;
  }

  db::LoadLayoutOptions options;
  options.set_options (opt);

  db::Manager m (false);
  db::Layout layout2 (&m), layout2_cif (&m), layout_au (&m), layout_au_cif (&m);

  //  generate a "unique" name ...
  unsigned int hash = 0;
  for (const char *cp = file_au; *cp; ++cp) {
    hash = (hash << 4) ^ (hash >> 4) ^ ((unsigned int) *cp);
  }

  //  normalize the layout by writing to GDS and reading from ..

  std::string tmp_gds_file = _this->tmp_file (tl::sprintf ("tmp_%x.gds", hash));
  std::string tmp_cif_file = _this->tmp_file (tl::sprintf ("tmp_%x.cif", hash));

  {
    tl::OutputStream stream (tmp_gds_file);
    db::SaveLayoutOptions options;
    options.set_format ("GDS2");
    db::Writer writer (options);
    writer.write (layout, stream);
  }

  {
    tl::InputStream stream (tmp_gds_file);
    db::Reader reader (stream);
    reader.read (layout2);
  }

  //  normalize the layout by writing to CIF and reading from ..

  {
    tl::OutputStream stream (tmp_cif_file);

    db::CIFWriterOptions *opt = new db::CIFWriterOptions();
    opt->dummy_calls = dummy_calls;
    opt->blank_separator = blank_sep;

    db::CIFWriter writer;
    db::SaveLayoutOptions options;
    options.set_options (opt);
    writer.write (layout, stream, options);
  }

  {
    tl::InputStream stream (tmp_cif_file);

    db::CIFReaderOptions *opt = new db::CIFReaderOptions();
    opt->dbu = dbu;
    db::LoadLayoutOptions reread_options;
    reread_options.set_options (opt);

    db::Reader reader (stream);
    reader.read (layout2_cif, reread_options);
  }

  {
    std::string fn (base);
    fn += "/cif/";
    fn += file_au;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout_au);
  }

  {
    std::string fn (base);
    fn += "/cif/";
    fn += file_au_cif;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout_au_cif);
  }

  bool equal = db::compare_layouts (layout2, layout_au, db::layout_diff::f_boxes_as_polygons | db::layout_diff::f_verbose | db::layout_diff::f_flatten_array_insts, 1);
  if (! equal) {
    _this->raise (tl::sprintf ("Compare failed after reading - see %s vs %s\n", tmp_gds_file, file_au));
  }

  equal = db::compare_layouts (layout2_cif, layout_au_cif, db::layout_diff::f_boxes_as_polygons | db::layout_diff::f_verbose | db::layout_diff::f_flatten_array_insts, 1);
  if (! equal) {
    _this->raise (tl::sprintf ("Compare failed after writing - see %s vs %s\n", tmp_cif_file, file_au_cif));
  }
}

TEST(1a)
{
  run_test (_this, tl::testdata_private (), "t1.cif.gz", "t1a_au.gds.gz");
}

TEST(1b)
{
  run_test (_this, tl::testdata_private (), "t1.cif.gz", "t1b_au.gds.gz", 0, 0.01);
}

TEST(1c)
{
  run_test (_this, tl::testdata_private (), "t1.cif.gz", "t1b_au.gds.gz", 0, 0.01, true);
}

TEST(1d)
{
  run_test (_this, tl::testdata_private (), "t1.cif.gz", "t1b_au.gds.gz", 0, 0.01, false, true);
}

TEST(2)
{
  run_test (_this, tl::testdata_private (), "t2.cif.gz", "t2_au.gds.gz");
}

TEST(3a)
{
  run_test (_this, tl::testdata_private (), "t3.cif.gz", "t3a_au.gds.gz", "CAA:43,CCA:48,CCP:47,CMF:49,CMS:51,CPG:46,CSN:45,CSP:44,CVA:50,CWN:42,XP:26");
}

TEST(3b)
{
  run_test (_this, tl::testdata_private (), "t3.cif.gz", "t3b_au.gds.gz", "CAA:43,CCA:48,CCP:47,CMF:49,CMS:51,CPG:46,CSN:45,CSP:44,CVA:50,CWN:42,XP:26", 0.00012);
}

TEST(3c)
{
  run_test (_this, tl::testdata_private (), "t3.cif.gz", "t3c_au.gds.gz", "(CPG:1/0) +(CPG:1000/0) (CCP:1/0) (CMF:2/0) +(CMF:1000/0) (CVA:3/0)", 0.00012);
}

TEST(4)
{
  run_test (_this, tl::testdata_private (), "t4.cif.gz", "t4_au.gds.gz");
}

TEST(5)
{
  run_test (_this, tl::testdata_private (), "t5.cif.gz", "t5_au.gds.gz");
}

//  Issue #28
TEST(lasi)
{
  run_test (_this, tl::testdata (), "lasi.cif.gz", "lasi_au.gds.gz");
}

//  Issue #305
TEST(rot_boxes)
{
  run_test (_this, tl::testdata (), "issue_305.cif", "issue_305_au.gds");
}

//  Issue #568
TEST(rot_instances)
{
  run_test (_this, tl::testdata (), "issue_568.cif", "issue_568_au.gds");
}

//  Issue #578
TEST(rot_instances2)
{
  run_test (_this, tl::testdata (), "issue_578.cif", "issue_578_au.gds");
}

//  issue #972
TEST(bad_names)
{
  db::Layout ly;

  db::cell_index_type ci = ly.add_cell ("(bad_cell,a b/c)");
  db::Cell &cell = ly.cell (ci);

  unsigned int l1 = ly.insert_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.insert_layer (db::LayerProperties (1, 5));
  unsigned int l3 = ly.insert_layer (db::LayerProperties ("a b c"));
  unsigned int l4 = ly.insert_layer (db::LayerProperties ("(a b c)"));
  unsigned int l5 = ly.insert_layer (db::LayerProperties ("a,b/c"));

  cell.shapes (l1).insert (db::Box (0, 0, 10, 10));
  cell.shapes (l2).insert (db::Box (0, 0, 20, 20));
  cell.shapes (l3).insert (db::Box (0, 0, 30, 30));
  cell.shapes (l4).insert (db::Box (0, 0, 40, 40));
  cell.shapes (l5).insert (db::Box (0, 0, 50, 50));

  run_test2 (_this, tl::testdata (), ly, "issue_972_au.gds", "issue_972_au.cif");
}
