
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


#include "dbOASISWriter.h"
#include "dbOASISReader.h"
#include "dbLayoutDiff.h"
#include "dbWriter.h"
#include "dbTextWriter.h"
#include "dbLibraryProxy.h"
#include "dbTestSupport.h"

#include "tlUnitTest.h"

#include <cstdlib>

void run_test (tl::TestBase *_this, const char *file, bool scaling_test, int compr, bool recompress, bool tables_at_end)
{
  {
    db::Manager m (false);
    db::Layout layout_org (&m);
    std::string fn (tl::testdata ());
    fn += "/oasis/";
    fn += file;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.set_warnings_as_errors (true);
    reader.read (layout_org);

    //  in between test the capabilities of a layout to copy itself
    db::Layout layout;
    layout = layout_org;
    layout_org.clear ();

    std::string tmp_file = _this->tmp_file ("tmp_1.oas");

    {
      tl::OutputStream stream (tmp_file);
      db::OASISWriter writer;
      db::SaveLayoutOptions options;
      db::OASISWriterOptions oasis_options;
      oasis_options.write_cblocks = false;
      oasis_options.strict_mode = false;
      oasis_options.tables_at_end = tables_at_end;
      options.set_options (oasis_options);
      writer.write (layout, stream, options);
    }

    db::Layout layout2 (&m);

    {
      tl::InputStream stream2 (tmp_file);
      db::Reader reader2 (stream2);
      db::LoadLayoutOptions options;
      db::OASISReaderOptions oasis_options;
      options.set_options (oasis_options);
      reader2.set_warnings_as_errors (true);
      reader2.read (layout2);
    }

    CHECKPOINT ();
    bool equal = db::compare_layouts (layout, layout2, db::layout_diff::f_verbose | db::layout_diff::f_flatten_array_insts, 0);
    if (! equal) {
      _this->raise (tl::sprintf ("Compare failed - see %s vs %s\n", fn, tmp_file));
    }

  }

  {
    db::Manager m (false);
    db::Layout layout_org (&m);
    std::string fn (tl::testdata ());
    fn += "/oasis/";
    fn += file;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.set_warnings_as_errors (true);
    reader.read (layout_org);

    //  in between test the capabilities of a layout to copy itself
    db::Layout layout;
    layout = layout_org;
    layout_org.clear ();

    //  generate a "unique" name ...
    unsigned int hash = 0;
    for (const char *cp = file; *cp; ++cp) {
      hash = (hash << 4) ^ (hash >> 4) ^ ((unsigned int) *cp);
    }

    std::string tmp_file = _this->tmp_file ("tmp_2.oas");

    {
      tl::OutputStream stream (tmp_file);
      db::OASISWriter writer;
      db::SaveLayoutOptions options;
      db::OASISWriterOptions oasis_options;
      oasis_options.write_cblocks = true;
      oasis_options.strict_mode = true;
      oasis_options.tables_at_end = tables_at_end;
      options.set_options (oasis_options);
      writer.write (layout, stream, options);
    }

    db::Layout layout2 (&m);

    {
      tl::InputStream stream2 (tmp_file);
      db::Reader reader2 (stream2);
      db::LoadLayoutOptions options;
      db::OASISReaderOptions oasis_options;
      oasis_options.expect_strict_mode = 1;
      options.set_options (oasis_options);
      reader2.set_warnings_as_errors (true);
      reader2.read (layout2, options);
    }

    CHECKPOINT ();
    bool equal = db::compare_layouts (layout, layout2, db::layout_diff::f_verbose | db::layout_diff::f_flatten_array_insts, 0);
    if (! equal) {
      _this->raise (tl::sprintf ("Compare failed - see %s vs %s\n", fn, tmp_file));
    }

  }

  {
    db::Manager m (false);
    db::Layout layout_org (&m);
    std::string fn (tl::testdata ());
    fn += "/oasis/";
    fn += file;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout_org);

    //  in between test the capabilities of a layout to copy itself
    db::Layout layout;
    layout = layout_org;
    layout_org.clear ();

    std::string tmp_file = _this->tmp_file ("tmp_3.oas");

    {
      tl::OutputStream stream (tmp_file);
      db::OASISWriter writer;
      db::SaveLayoutOptions options;
      db::OASISWriterOptions oasis_options;
      oasis_options.write_cblocks = false;
      oasis_options.strict_mode = false;
      oasis_options.tables_at_end = tables_at_end;
      oasis_options.write_std_properties = 2;
      options.set_options (oasis_options);
      writer.write (layout, stream, options);
    }

    db::Layout layout2 (&m);

    {
      tl::InputStream stream2 (tmp_file);
      db::Reader reader2 (stream2);
      db::LoadLayoutOptions options;
      db::OASISReaderOptions oasis_options;
      oasis_options.expect_strict_mode = 0;
      options.set_options (oasis_options);
      reader2.set_warnings_as_errors (true);
      reader2.read (layout2, options);
    }

    CHECKPOINT ();
    bool equal = db::compare_layouts (layout, layout2, db::layout_diff::f_verbose | db::layout_diff::f_flatten_array_insts, 0);
    if (! equal) {
      _this->raise (tl::sprintf ("Compare failed - see %s vs %s\n", fn, tmp_file));
    }

  }

  {
    db::Manager m (false);
    db::Layout layout_org (&m);
    std::string fn (tl::testdata ());
    fn += "/oasis/";
    fn += file;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout_org);

    //  in between test the capabilities of a layout to copy itself
    db::Layout layout;
    layout = layout_org;
    layout_org.clear ();

    std::string tmp_file = _this->tmp_file ("tmp_4.oas");

    {
      tl::OutputStream stream (tmp_file);
      db::OASISWriter writer;
      db::SaveLayoutOptions options;
      db::OASISWriterOptions oasis_options;
      oasis_options.write_cblocks = true;
      oasis_options.strict_mode = true;
      oasis_options.tables_at_end = tables_at_end;
      oasis_options.write_std_properties = 2;
      options.set_options (oasis_options);
      writer.write (layout, stream, options);
    }

    db::Layout layout2 (&m);

    {
      tl::InputStream stream2 (tmp_file);
      db::Reader reader2 (stream2);
      db::LoadLayoutOptions options;
      db::OASISReaderOptions oasis_options;
      oasis_options.expect_strict_mode = 1;
      options.set_options (oasis_options);
      reader2.set_warnings_as_errors (true);
      reader2.read (layout2, options);
    }

    CHECKPOINT ();
    bool equal = db::compare_layouts (layout, layout2, db::layout_diff::f_verbose | db::layout_diff::f_flatten_array_insts, 0);
    if (! equal) {
      _this->raise (tl::sprintf ("Compare failed - see %s vs %s\n", fn, tmp_file));
    }

  }

  if (scaling_test) {

    db::Manager m (false);
    db::Layout layout (&m);
    std::string fn (tl::testdata ());
    fn += "/oasis/";
    fn += file;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout);

    db::SaveLayoutOptions options;
    db::OASISWriterOptions oasis_options;
    oasis_options.compression_level = compr;
    oasis_options.recompress = recompress;
    oasis_options.tables_at_end = tables_at_end;
    options.set_options (oasis_options);
    options.set_scale_factor (3.0);
    options.set_dbu (0.0005);

    //  generate a "unique" name ...
    unsigned int hash = 0;
    for (const char *cp = file; *cp; ++cp) {
      hash = (hash << 4) ^ (hash >> 4) ^ ((unsigned int) *cp);
    }

    std::string tmp1_file = _this->tmp_file ("tmp_s1.gds");
    std::string tmp2_file = _this->tmp_file ("tmp_s2.oas");

    {
      tl::OutputStream stream (tmp1_file);
      db::SaveLayoutOptions gds2_options = options;
      gds2_options.set_format ("GDS2");
      db::Writer writer (gds2_options);
      writer.write (layout, stream);
    }

    {
      tl::OutputStream stream (tmp2_file);
      db::OASISWriter writer;
      writer.write (layout, stream, options);
    }

    db::Layout layout1 (&m);
    {
      tl::InputStream file (tmp1_file);
      db::Reader reader (file);
      reader.read (layout1);
    }

    db::Layout layout2 (&m);
    {
      tl::InputStream file (tmp2_file);
      db::Reader reader (file);
      reader.read (layout2);
    }

    CHECKPOINT ();
    bool equal = db::compare_layouts (layout1, layout2, db::layout_diff::f_verbose | db::layout_diff::f_flatten_array_insts | db::layout_diff::f_no_properties | db::layout_diff::f_no_layer_names | db::layout_diff::f_boxes_as_polygons, 0);
    if (! equal) {
      _this->raise (tl::sprintf ("Compare failed - see %s vs %s\n", tmp1_file, tmp2_file));
    }

  }
}

void run_test (tl::TestBase *_this, const char *file, bool scaling_test = true)
{
  for (int recompress = 0; recompress < 2; ++recompress) {
    run_test (_this, file, scaling_test, 0, recompress, false);
    run_test (_this, file, scaling_test, 1, recompress, false);
    run_test (_this, file, scaling_test, 2, recompress, false);
    run_test (_this, file, scaling_test, 10, recompress, false);
  }

  //  tables at end
  run_test (_this, file, scaling_test, 2, false, true);
}

TEST(1)
{
  run_test (_this, "t10.1.oas");
}

TEST(2)
{
  run_test (_this, "t11.1.oas");
}

TEST(3)
{
  run_test (_this, "t11.2.oas");
}

TEST(4)
{
  run_test (_this, "t11.3.oas");
}

TEST(4A)
{
  run_test (_this, "t11.4.oas");
}

TEST(5)
{
  run_test (_this, "t1.1.oas");
}

TEST(6)
{
  run_test (_this, "t12.1.oas");
}

TEST(7)
{
  run_test (_this, "t1.2.oas");
}

TEST(8)
{
  run_test (_this, "t13.1.oas");
}

TEST(9)
{
  run_test (_this, "t13.2.oas");
}

TEST(10)
{
  run_test (_this, "t13.3.oas");
}

TEST(11)
{
  run_test (_this, "t1.3.oas");
}

TEST(12)
{
  run_test (_this, "t14.1.oas");
}

TEST(13)
{
  run_test (_this, "t1.4.oas");
}

TEST(14)
{
  run_test (_this, "t1.5.oas");
}

TEST(15)
{
  run_test (_this, "t2.1.oas");
}

TEST(16)
{
  run_test (_this, "t2.2.oas");
}

TEST(17)
{
  run_test (_this, "t2.4.oas");
}

TEST(19)
{
  run_test (_this, "t3.10.oas");
}

TEST(20)
{
  run_test (_this, "t3.1.oas");
}

TEST(21)
{
  run_test (_this, "t3.2.oas");
}

TEST(22)
{
  run_test (_this, "t3.5.oas");
}

TEST(23)
{
  run_test (_this, "t3.9.oas");
}

TEST(24)
{
  run_test (_this, "t4.1.oas");
}

TEST(25)
{
  run_test (_this, "t4.2.oas");
}

TEST(26)
{
  run_test (_this, "t5.1.oas");
}

TEST(27)
{
  //  no scaling test, since this test contains polygons with >8000 points that cannot be written to GDS
  run_test (_this, "t5.2.oas", false);
}

TEST(28)
{
  run_test (_this, "t5.3.oas");
}

TEST(29)
{
  run_test (_this, "t6.1.oas");
}

TEST(30)
{
  run_test (_this, "t7.1.oas");
}

TEST(31)
{
  run_test (_this, "t8.1.oas");
}

TEST(32)
{
  run_test (_this, "t8.2.oas");
}

TEST(33)
{
  run_test (_this, "t8.3.oas");
}

TEST(34)
{
  run_test (_this, "t8.4.oas");
}

TEST(35)
{
  run_test (_this, "t8.5.oas");
}

TEST(36)
{
  run_test (_this, "t8.6.oas");
}

TEST(37)
{
  run_test (_this, "t8.7.oas");
}

TEST(38)
{
  run_test (_this, "t8.8.oas");
}

TEST(39)
{
  run_test (_this, "t9.1.oas");
}

TEST(40)
{
  run_test (_this, "t9.2.oas");
}

TEST(100)
{
  db::Manager m (false);
  db::Layout g (&m);

  db::LayerProperties lp0;
  lp0.layer = 0;
  lp0.datatype = 0;

  db::LayerProperties lp1;
  lp1.layer = 1;
  lp1.datatype = 0;

  db::LayerProperties lp2;
  lp2.layer = 2;
  lp2.datatype = 0;

  g.insert_layer (0, lp0);
  g.insert_layer (1, lp1);
  g.insert_layer (2, lp2);

  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));
  db::Cell &c3 (g.cell (g.add_cell ()));
  db::Cell &c4 (g.cell (g.add_cell ()));

  db::Box b (0, 100, 1000, 1200);
  c1.shapes (1).insert (b);

  db::Box bb (0, -100, 2000, 2200);
  c2.shapes (2).insert (bb);

  //  inserting instances ..
  db::FTrans f (1, true);
  db::Vector p (-10, 20);
  db::Trans t (f.rot (), p);
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c3.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c4.cell_index ()), t));

  std::string tmp_file = tl::TestBase::tmp_file (tl::sprintf ("tmp_dbOASISWriter100.gds"));

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_option_by_name ("oasis_strict_mode", false);
    options.set_format ("OASIS");
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
  reader.set_warnings_as_errors (true);
  reader.read (gg);

  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {$4}\n"
    "end_cell\n"
    "begin_cell {$1}\n"
    "box 1 0 {0 100} {1000 1200}\n"
    "end_cell\n"
    "begin_cell {$3}\n"
    "sref {$1} 90 1 1 {-10 20}\n"
    "sref {$4} 90 1 1 {-10 20}\n"
    "end_cell\n"
    "begin_cell {$2}\n"
    "sref {$1} 90 1 1 {-10 20}\n"
    "sref {$3} 90 1 1 {-10 20}\n"
    "box 2 0 {0 -100} {2000 2200}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  tl::OutputStringStream os;
  tl::OutputStream stream (os);
  db::TextWriter textwriter (stream);
  textwriter.write (gg);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(101)
{
  db::Manager m (false);
  db::Layout g (&m);

  db::LayerProperties lp0;
  lp0.layer = 0;
  lp0.datatype = 0;

  db::LayerProperties lp1;
  lp1.layer = 1;
  lp1.datatype = 0;

  db::LayerProperties lp2;
  lp2.layer = 2;
  lp2.datatype = 0;

  g.insert_layer (0, lp0);
  g.insert_layer (1, lp1);
  g.insert_layer (2, lp2);

  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));
  db::Cell &c3 (g.cell (g.add_cell ()));
  db::Cell &c4 (g.cell (g.add_cell ()));

  db::Box b (0, 100, 1000, 1200);
  c1.shapes (1).insert (b);

  db::Box bb (0, -100, 2000, 2200);
  c2.shapes (2).insert (bb);

  //  inserting instances ..
  db::FTrans f (1, true);
  db::Vector p (-10, 20);
  db::Trans t (f.rot (), p);
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c3.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c4.cell_index ()), t));

  std::string tmp_file = tl::TestBase::tmp_file (tl::sprintf ("tmp_dbOASISWriter101.gds"));

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("OASIS");
    options.add_layer (0);
    options.set_dont_write_empty_cells (true);
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
  reader.set_warnings_as_errors (true);
  reader.read (gg);

  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {$2}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  tl::OutputStringStream os;
  tl::OutputStream stream (os);
  db::TextWriter textwriter (stream);
  textwriter.write (gg);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(102)
{
  db::Manager m (false);
  db::Layout g (&m);

  db::LayerProperties lp0;
  lp0.layer = 0;
  lp0.datatype = 0;

  db::LayerProperties lp1;
  lp1.layer = 1;
  lp1.datatype = 0;

  db::LayerProperties lp2;
  lp2.layer = 2;
  lp2.datatype = 0;

  g.insert_layer (0, lp0);
  g.insert_layer (1, lp1);
  g.insert_layer (2, lp2);

  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));
  db::Cell &c3 (g.cell (g.add_cell ()));
  db::Cell &c4 (g.cell (g.add_cell ()));

  db::Box b (0, 100, 1000, 1200);
  c1.shapes (1).insert (b);

  db::Box bb (0, -100, 2000, 2200);
  c2.shapes (2).insert (bb);

  //  inserting instances ..
  db::FTrans f (1, true);
  db::Vector p (-10, 20);
  db::Trans t (f.rot (), p);
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c3.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c4.cell_index ()), t));

  std::string tmp_file = tl::TestBase::tmp_file (tl::sprintf ("tmp_dbOASISWriter102.gds"));

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("OASIS");
    options.add_layer (1);
    options.set_dont_write_empty_cells (true);
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
  reader.set_warnings_as_errors (true);
  reader.read (gg);

  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {$1}\n"
    "box 1 0 {0 100} {1000 1200}\n"
    "end_cell\n"
    "begin_cell {$3}\n"
    "sref {$1} 90 1 1 {-10 20}\n"
    "end_cell\n"
    "begin_cell {$2}\n"
    "sref {$1} 90 1 1 {-10 20}\n"
    "sref {$3} 90 1 1 {-10 20}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  tl::OutputStringStream os;
  tl::OutputStream stream (os);
  db::TextWriter textwriter (stream);
  textwriter.write (gg);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(103)
{
  db::Manager m (false);
  db::Layout g (&m);

  db::LayerProperties lp0;
  lp0.layer = 0;
  lp0.datatype = 0;

  db::LayerProperties lp1;
  lp1.layer = 1;
  lp1.datatype = 0;

  db::LayerProperties lp2;
  lp2.layer = 2;
  lp2.datatype = 0;

  g.insert_layer (0, lp0);
  g.insert_layer (1, lp1);
  g.insert_layer (2, lp2);

  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));
  db::Cell &c3 (g.cell (g.add_cell ()));
  db::Cell &c4 (g.cell (g.add_cell ()));

  db::Box b (0, 100, 1000, 1200);
  c1.shapes (1).insert (b);

  db::Box bb (0, -100, 2000, 2200);
  c2.shapes (2).insert (bb);

  //  inserting instances ..
  db::FTrans f (1, true);
  db::Vector p (-10, 20);
  db::Trans t (f.rot (), p);
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c3.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c4.cell_index ()), t));

  std::string tmp_file = tl::TestBase::tmp_file (tl::sprintf ("tmp_dbOASISWriter103.gds"));

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("OASIS");
    options.add_layer (2);
    options.set_dont_write_empty_cells (true);
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
  reader.set_warnings_as_errors (true);
  reader.read (gg);

  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {$2}\n"
    "box 2 0 {0 -100} {2000 2200}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  tl::OutputStringStream os;
  tl::OutputStream stream (os);
  db::TextWriter textwriter (stream);
  textwriter.write (gg);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(110)
{
  db::Manager m (false);
  db::Layout g (&m);

  db::LayerProperties lp0;
  lp0.layer = 0;
  lp0.datatype = 0;

  db::LayerProperties lp1;
  lp1.layer = 1;
  lp1.datatype = 0;

  db::LayerProperties lp2;
  lp2.layer = 2;
  lp2.datatype = 0;

  g.insert_layer (0, lp0);
  g.insert_layer (1, lp1);
  g.insert_layer (2, lp2);

  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));
  db::Cell &c3 (g.cell (g.add_cell ()));
  db::Cell &c4 (g.cell (g.add_cell ()));

  db::Box b (0, 100, 1000, 1200);
  c1.shapes (1).insert (b);

  db::Box bb (0, -100, 2000, 2200);
  c2.shapes (2).insert (bb);

  //  inserting instances ..
  db::FTrans f (1, true);
  db::Vector p (-10, 20);
  db::Trans t (f.rot (), p);
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c3.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c4.cell_index ()), t));

  std::string tmp_file = tl::TestBase::tmp_file (tl::sprintf ("tmp_dbOASISWriter110.gds"));

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("OASIS");
    options.set_dont_write_empty_cells (true);
    options.add_cell (c3.cell_index ());
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
  reader.set_warnings_as_errors (true);
  reader.read (gg);

  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {$1}\n"
    "box 1 0 {0 100} {1000 1200}\n"
    "end_cell\n"
    "begin_cell {$3}\n"
    "sref {$1} 90 1 1 {-10 20}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  tl::OutputStringStream os;
  tl::OutputStream stream (os);
  db::TextWriter textwriter (stream);
  textwriter.write (gg);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(111)
{
  db::Manager m (false);
  db::Layout g (&m);

  db::LayerProperties lp0;
  lp0.layer = 0;
  lp0.datatype = 0;

  db::LayerProperties lp1;
  lp1.layer = 1;
  lp1.datatype = 0;

  db::LayerProperties lp2;
  lp2.layer = 2;
  lp2.datatype = 0;

  g.insert_layer (0, lp0);
  g.insert_layer (1, lp1);
  g.insert_layer (2, lp2);

  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));
  db::Cell &c3 (g.cell (g.add_cell ()));
  db::Cell &c4 (g.cell (g.add_cell ()));

  db::Box b (0, 100, 1000, 1200);
  c1.shapes (1).insert (b);

  db::Box bb (0, -100, 2000, 2200);
  c2.shapes (2).insert (bb);

  //  inserting instances ..
  db::FTrans f (1, true);
  db::Vector p (-10, 20);
  db::Trans t (f.rot (), p);
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c3.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c4.cell_index ()), t));

  std::string tmp_file = tl::TestBase::tmp_file (tl::sprintf ("tmp_dbOASISWriter111.gds"));

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("OASIS");
    options.add_cell (c3.cell_index ());
    options.add_layer (0);
    options.set_dont_write_empty_cells (true);
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
  reader.set_warnings_as_errors (true);
  reader.read (gg);

  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {$3}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  tl::OutputStringStream os;
  tl::OutputStream stream (os);
  db::TextWriter textwriter (stream);
  textwriter.write (gg);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(112)
{
  db::Manager m (false);
  db::Layout g (&m);

  db::LayerProperties lp0;
  lp0.layer = 0;
  lp0.datatype = 0;

  db::LayerProperties lp1;
  lp1.layer = 1;
  lp1.datatype = 0;

  db::LayerProperties lp2;
  lp2.layer = 2;
  lp2.datatype = 0;

  g.insert_layer (0, lp0);
  g.insert_layer (1, lp1);
  g.insert_layer (2, lp2);

  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));
  db::Cell &c3 (g.cell (g.add_cell ()));
  db::Cell &c4 (g.cell (g.add_cell ()));

  db::Box b (0, 100, 1000, 1200);
  c1.shapes (1).insert (b);

  db::Box bb (0, -100, 2000, 2200);
  c2.shapes (2).insert (bb);

  //  inserting instances ..
  db::FTrans f (1, true);
  db::Vector p (-10, 20);
  db::Trans t (f.rot (), p);
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c3.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c4.cell_index ()), t));

  std::string tmp_file = tl::TestBase::tmp_file (tl::sprintf ("tmp_dbOASISWriter112.gds"));

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("OASIS");
    options.add_cell (c3.cell_index ());
    options.add_layer (1);
    options.set_dont_write_empty_cells (true);
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
  reader.set_warnings_as_errors (true);
  reader.read (gg);

  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {$1}\n"
    "box 1 0 {0 100} {1000 1200}\n"
    "end_cell\n"
    "begin_cell {$3}\n"
    "sref {$1} 90 1 1 {-10 20}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  tl::OutputStringStream os;
  tl::OutputStream stream (os);
  db::TextWriter textwriter (stream);
  textwriter.write (gg);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(113)
{
  db::Manager m (false);
  db::Layout g (&m);

  db::LayerProperties lp0;
  lp0.layer = 0;
  lp0.datatype = 0;

  db::LayerProperties lp1;
  lp1.layer = 1;
  lp1.datatype = 0;

  db::LayerProperties lp2;
  lp2.layer = 2;
  lp2.datatype = 0;

  g.insert_layer (0, lp0);
  g.insert_layer (1, lp1);
  g.insert_layer (2, lp2);

  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));
  db::Cell &c3 (g.cell (g.add_cell ()));
  db::Cell &c4 (g.cell (g.add_cell ()));

  db::Box b (0, 100, 1000, 1200);
  c1.shapes (1).insert (b);

  db::Box bb (0, -100, 2000, 2200);
  c2.shapes (2).insert (bb);

  //  inserting instances ..
  db::FTrans f (1, true);
  db::Vector p (-10, 20);
  db::Trans t (f.rot (), p);
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c3.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c4.cell_index ()), t));

  std::string tmp_file = tl::TestBase::tmp_file (tl::sprintf ("tmp_dbOASISWriter113.gds"));

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("OASIS");
    options.add_cell (c3.cell_index ());
    options.add_layer (2);
    options.set_dont_write_empty_cells (true);
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
  reader.set_warnings_as_errors (true);
  reader.read (gg);

  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {$3}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  tl::OutputStringStream os;
  tl::OutputStream stream (os);
  db::TextWriter textwriter (stream);
  textwriter.write (gg);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(114)
{
  db::Manager m (false);
  db::Layout g (&m);

  db::LayerProperties lp1;
  lp1.layer = 1;
  lp1.datatype = 0;

  g.insert_layer (1, lp1);

  db::Cell &c1 (g.cell (g.add_cell ()));

  db::Edge e1 (0, 100, 1000, 1200);
  c1.shapes (1).insert (e1);

  db::Edge e2 (0, 100, 0, 1200);
  c1.shapes (1).insert (e2);

  db::Edge e3 (0, 1200, 1000, 1200);
  c1.shapes (1).insert (e3);

  std::string tmp_file = tl::TestBase::tmp_file (tl::sprintf ("tmp_dbOASISWriter114.gds"));

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("OASIS");
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
  reader.set_warnings_as_errors (true);
  reader.read (gg);

  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {$1}\n"
    "path 1 0 0 0 0 {0 100} {0 1200}\n"
    "path 1 0 0 0 0 {0 100} {1000 1200}\n"
    "path 1 0 0 0 0 {0 1200} {1000 1200}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  tl::OutputStringStream os;
  tl::OutputStream stream (os);
  db::TextWriter textwriter (stream);
  textwriter.write (gg);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(115)
{
  db::Manager m (false);
  db::Layout g (&m);

  db::property_names_id_type n1, n2, n3;
  n1 = g.properties_repository ().prop_name_id (tl::Variant (17));
  n2 = g.properties_repository ().prop_name_id (tl::Variant ("name"));
  n3 = g.properties_repository ().prop_name_id (tl::Variant ((unsigned int) 42));

  db::PropertiesRepository::properties_set s1;
  s1.insert (std::make_pair (n1, tl::Variant ("17value")));
  s1.insert (std::make_pair (n2, tl::Variant (117)));

  db::PropertiesRepository::properties_set s2;
  s2.insert (std::make_pair (n3, tl::Variant (42)));

  db::properties_id_type p1 = g.properties_repository ().properties_id (s1);
  db::properties_id_type p2 = g.properties_repository ().properties_id (s2);

  g.prop_id (p1);

  db::LayerProperties lp1;
  lp1.layer = 1;
  lp1.datatype = 0;

  g.insert_layer (1, lp1);

  db::Cell &c1 (g.cell (g.add_cell ()));
  c1.prop_id (p2);

  db::Edge e1 (0, 100, 1000, 1200);
  c1.shapes (1).insert (e1);

  db::Edge e2 (0, 100, 0, 1200);
  c1.shapes (1).insert (e2);

  db::Edge e3 (0, 1200, 1000, 1200);
  c1.shapes (1).insert (e3);

  std::string tmp_file = tl::TestBase::tmp_file (tl::sprintf ("tmp_dbOASISWriter115.gds"));

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_option_by_name ("oasis_strict_mode", false);
    options.set_format ("OASIS");
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
  reader.set_warnings_as_errors (true);
  reader.read (gg);

  const char *expected = 
    "set props {\n"
    "  {17 {17value}}\n"
    "  {{name} {117}}\n"
    "}\n"
    "begin_libp $props 0.001\n"
    "set props {\n"
    "  {42 {42}}\n"
    "}\n"
    "begin_cellp $props {$1}\n"
    "path 1 0 0 0 0 {0 100} {0 1200}\n"
    "path 1 0 0 0 0 {0 100} {1000 1200}\n"
    "path 1 0 0 0 0 {0 1200} {1000 1200}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  tl::OutputStringStream os;
  tl::OutputStream stream (os);
  db::TextWriter textwriter (stream);
  textwriter.write (gg);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(116)
{
  db::Manager m (false);
  db::Layout g (&m);

  db::property_names_id_type n1, n2, n3;
  n1 = g.properties_repository ().prop_name_id (tl::Variant (17));
  n2 = g.properties_repository ().prop_name_id (tl::Variant ("name"));
  n3 = g.properties_repository ().prop_name_id (tl::Variant ((unsigned int) 42));

  db::PropertiesRepository::properties_set s1;
  s1.insert (std::make_pair (n1, tl::Variant ("17value")));
  s1.insert (std::make_pair (n2, tl::Variant (117)));

  db::PropertiesRepository::properties_set s2;
  s2.insert (std::make_pair (n3, tl::Variant (42)));

  db::properties_id_type p1 = g.properties_repository ().properties_id (s1);
  db::properties_id_type p2 = g.properties_repository ().properties_id (s2);

  g.prop_id (p1);

  db::LayerProperties lp1;
  lp1.layer = 1;
  lp1.datatype = 0;

  g.insert_layer (1, lp1);

  db::Cell &c1 (g.cell (g.add_cell ()));
  c1.prop_id (p2);

  db::Edge e1 (0, 100, 1000, 1200);
  c1.shapes (1).insert (e1);

  db::Cell &c2 (g.cell (g.add_cell ()));

  {
    std::string tmp_file = tl::TestBase::tmp_file (tl::sprintf ("tmp_dbOASISWriter116a.gds"));

    {
      tl::OutputStream out (tmp_file);
      db::SaveLayoutOptions write_options;
      write_options.set_option_by_name ("oasis_strict_mode", false);
      write_options.set_format ("OASIS");
      db::Writer writer (write_options);
      writer.write (g, out);
    }

    tl::InputStream in (tmp_file);
    db::OASISReaderOptions oas_options;
    oas_options.read_all_properties = true;
    db::LoadLayoutOptions options;
    options.set_options (oas_options);
    db::Reader reader (in);
    db::Layout gg;
    reader.set_warnings_as_errors (true);
    reader.read (gg, options);

    const char *expected = 
      "set props {\n"
#if defined(HAVE_64BIT_COORD)
      "  {{S_MAX_SIGNED_INTEGER_WIDTH} {8}}\n"
      "  {{S_MAX_UNSIGNED_INTEGER_WIDTH} {8}}\n"
#else
      "  {{S_MAX_SIGNED_INTEGER_WIDTH} {4}}\n"
      "  {{S_MAX_UNSIGNED_INTEGER_WIDTH} {4}}\n"
#endif
      "  {{S_TOP_CELL} {$2}}\n"
      "  {{S_TOP_CELL} {$1}}\n"
      "  {17 {17value}}\n"
      "  {{name} {117}}\n"
      "}\n"
      "begin_libp $props 0.001\n"
      "begin_cell {$2}\n"
      "end_cell\n"
      "set props {\n"
      "  {42 {42}}\n"
      "}\n"
      "begin_cellp $props {$1}\n"
      "path 1 0 0 0 0 {0 100} {1000 1200}\n"
      "end_cell\n"
      "end_lib\n"
    ;

    tl::OutputStringStream os;
    tl::OutputStream stream (os);
    db::TextWriter textwriter (stream);
    textwriter.write (gg);
    EXPECT_EQ (std::string (os.string ()), std::string (expected))
  }

  {
    std::string tmp_file = tl::TestBase::tmp_file (tl::sprintf ("tmp_dbOASISWriter116b.gds"));

    {
      tl::OutputStream out (tmp_file);
      db::SaveLayoutOptions write_options;
      write_options.set_format ("OASIS");
      db::OASISWriterOptions oas_write_options;
      oas_write_options.write_std_properties = 0;
      oas_write_options.strict_mode = false;
      write_options.set_options (oas_write_options);
      db::Writer writer (write_options);
      writer.write (g, out);
    }

    tl::InputStream in (tmp_file);
    db::OASISReaderOptions oas_options;
    oas_options.read_all_properties = true;
    oas_options.expect_strict_mode = 0;
    db::LoadLayoutOptions options;
    options.set_options (oas_options);
    db::Reader reader (in);
    db::Layout gg;
    reader.set_warnings_as_errors (true);
    reader.read (gg, options);

    const char *expected = 
      "set props {\n"
      "  {17 {17value}}\n"
      "  {{name} {117}}\n"
      "}\n"
      "begin_libp $props 0.001\n"
      "begin_cell {$2}\n"
      "end_cell\n"
      "set props {\n"
      "  {42 {42}}\n"
      "}\n"
      "begin_cellp $props {$1}\n"
      "path 1 0 0 0 0 {0 100} {1000 1200}\n"
      "end_cell\n"
      "end_lib\n"
    ;

    tl::OutputStringStream os;
    tl::OutputStream stream (os);
    db::TextWriter textwriter (stream);
    textwriter.write (gg);
    EXPECT_EQ (std::string (os.string ()), std::string (expected))
  }

  {
    std::string tmp_file = tl::TestBase::tmp_file (tl::sprintf ("tmp_dbOASISWriter116c.gds"));

    {
      tl::OutputStream out (tmp_file);
      db::SaveLayoutOptions write_options;
      write_options.set_format ("OASIS");
      db::OASISWriterOptions oas_write_options;
      oas_write_options.write_std_properties = 2;
      oas_write_options.strict_mode = false;
      write_options.set_options (oas_write_options);
      db::Writer writer (write_options);
      writer.write (g, out);
    }

    tl::InputStream in (tmp_file);
    db::OASISReaderOptions oas_options;
    oas_options.read_all_properties = true;
    oas_options.expect_strict_mode = 0;
    db::LoadLayoutOptions options;
    options.set_options (oas_options);
    db::Reader reader (in);
    db::Layout gg;
    reader.set_warnings_as_errors (true);
    reader.read (gg, options);

    const char *expected = 
      "set props {\n"
#if defined(HAVE_64BIT_COORD)
      "  {{S_MAX_SIGNED_INTEGER_WIDTH} {8}}\n"
      "  {{S_MAX_UNSIGNED_INTEGER_WIDTH} {8}}\n"
#else
      "  {{S_MAX_SIGNED_INTEGER_WIDTH} {4}}\n"
      "  {{S_MAX_UNSIGNED_INTEGER_WIDTH} {4}}\n"
#endif
      "  {{S_TOP_CELL} {$2}}\n"
      "  {{S_TOP_CELL} {$1}}\n"
      "  {{S_BOUNDING_BOXES_AVAILABLE} {2}}\n"
      "  {17 {17value}}\n"
      "  {{name} {117}}\n"
      "}\n"
      "begin_libp $props 0.001\n"
      "set props {\n"
      "  {{S_BOUNDING_BOX} {2,0,0,0,0}}\n"
      "}\n"
      "begin_cellp $props {$2}\n"
      "end_cell\n"
      "set props {\n"
      "  {{S_BOUNDING_BOX} {0,0,100,1000,1100}}\n"
      "  {42 {42}}\n"
      "}\n"
      "begin_cellp $props {$1}\n"
      "path 1 0 0 0 0 {0 100} {1000 1200}\n"
      "end_cell\n"
      "end_lib\n"
    ;

    tl::OutputStringStream os;
    tl::OutputStream stream (os);
    db::TextWriter textwriter (stream);
    textwriter.write (gg);
    EXPECT_EQ (std::string (os.string ()), std::string (expected))
  }

  {
    std::string tmp_file = tl::TestBase::tmp_file ("tmp_dbOASISWriter116d.gds");

    {
      tl::OutputStream out (tmp_file);
      db::SaveLayoutOptions write_options;
      write_options.set_format ("OASIS");
      db::OASISWriterOptions oas_write_options;
      oas_write_options.write_std_properties = 2;
      oas_write_options.strict_mode = true;
      oas_write_options.write_cblocks = false;
      write_options.set_options (oas_write_options);
      db::Writer writer (write_options);
      writer.write (g, out);
    }

    tl::InputStream in (tmp_file);
    db::OASISReaderOptions oas_options;
    oas_options.read_all_properties = true;
    oas_options.expect_strict_mode = 1;
    db::LoadLayoutOptions options;
    options.set_options (oas_options);
    db::Reader reader (in);
    db::Layout gg;
    reader.set_warnings_as_errors (true);
    reader.read (gg, options);

    const char *expected = 
      "set props {\n"
#if defined(HAVE_64BIT_COORD)
      "  {{S_MAX_SIGNED_INTEGER_WIDTH} {8}}\n"
      "  {{S_MAX_UNSIGNED_INTEGER_WIDTH} {8}}\n"
#else
      "  {{S_MAX_SIGNED_INTEGER_WIDTH} {4}}\n"
      "  {{S_MAX_UNSIGNED_INTEGER_WIDTH} {4}}\n"
#endif
      "  {{S_TOP_CELL} {$2}}\n"
      "  {{S_TOP_CELL} {$1}}\n"
      "  {{S_BOUNDING_BOXES_AVAILABLE} {2}}\n"
      "  {{name} {117}}\n"
      "  {17 {17value}}\n"
      "}\n"
      "begin_libp $props 0.001\n"
      "set props {\n"
      "  {42 {42}}\n"
      "  {{S_BOUNDING_BOX} {0,0,100,1000,1100}}\n"
      "  {{S_CELL_OFFSET} {231}}\n"
      "}\n"
      "begin_cellp $props {$1}\n"
      "path 1 0 0 0 0 {0 100} {1000 1200}\n"
      "end_cell\n"
      "set props {\n"
      "  {{S_BOUNDING_BOX} {2,0,0,0,0}}\n"
      "  {{S_CELL_OFFSET} {229}}\n"
      "}\n"
      "begin_cellp $props {$2}\n"
      "end_cell\n"
      "end_lib\n"
    ;

    tl::OutputStringStream os;
    tl::OutputStream stream (os);
    db::TextWriter textwriter (stream);
    textwriter.write (gg);
    EXPECT_EQ (std::string (os.string ()), std::string (expected))
  }

  {
    std::string tmp_file = tl::TestBase::tmp_file ("tmp_dbOASISWriter116d2.gds");

    {
      tl::OutputStream out (tmp_file);
      db::SaveLayoutOptions write_options;
      write_options.set_format ("OASIS");
      db::OASISWriterOptions oas_write_options;
      oas_write_options.write_std_properties = 1;
      oas_write_options.strict_mode = true;
      oas_write_options.write_cblocks = false;
      write_options.set_options (oas_write_options);
      db::Writer writer (write_options);
      writer.write (g, out);
    }

    tl::InputStream in (tmp_file);
    db::OASISReaderOptions oas_options;
    oas_options.read_all_properties = true;
    oas_options.expect_strict_mode = 1;
    db::LoadLayoutOptions options;
    options.set_options (oas_options);
    db::Reader reader (in);
    db::Layout gg;
    reader.set_warnings_as_errors (true);
    reader.read (gg, options);

    const char *expected =
      "set props {\n"
#if defined(HAVE_64BIT_COORD)
      "  {{S_MAX_SIGNED_INTEGER_WIDTH} {8}}\n"
      "  {{S_MAX_UNSIGNED_INTEGER_WIDTH} {8}}\n"
#else
      "  {{S_MAX_SIGNED_INTEGER_WIDTH} {4}}\n"
      "  {{S_MAX_UNSIGNED_INTEGER_WIDTH} {4}}\n"
#endif
      "  {{S_TOP_CELL} {$2}}\n"
      "  {{S_TOP_CELL} {$1}}\n"
      "  {{name} {117}}\n"
      "  {17 {17value}}\n"
      "}\n"
      "begin_libp $props 0.001\n"
      "set props {\n"
      "  {42 {42}}\n"
      "  {{S_CELL_OFFSET} {182}}\n"
      "}\n"
      "begin_cellp $props {$1}\n"
      "path 1 0 0 0 0 {0 100} {1000 1200}\n"
      "end_cell\n"
      "set props {\n"
      "  {{S_CELL_OFFSET} {180}}\n"
      "}\n"
      "begin_cellp $props {$2}\n"
      "end_cell\n"
      "end_lib\n"
    ;

    tl::OutputStringStream os;
    tl::OutputStream stream (os);
    db::TextWriter textwriter (stream);
    textwriter.write (gg);
    EXPECT_EQ (std::string (os.string ()), std::string (expected))
  }

  c1.insert (db::CellInstArray (c2.cell_index (), db::Trans ()));

  {
    std::string tmp_file = tl::TestBase::tmp_file ("tmp_dbOASISWriter116e.gds");

    {
      tl::OutputStream out (tmp_file);
      db::SaveLayoutOptions write_options;
      write_options.set_format ("OASIS");
      write_options.set_option_by_name ("oasis_strict_mode", false);
      db::Writer writer (write_options);
      writer.write (g, out);
    }

    tl::InputStream in (tmp_file);
    db::OASISReaderOptions oas_options;
    oas_options.read_all_properties = true;
    db::LoadLayoutOptions options;
    options.set_options (oas_options);
    db::Reader reader (in);
    db::Layout gg;
    reader.set_warnings_as_errors (true);
    reader.read (gg, options);

    const char *expected = 
      "set props {\n"
#if defined(HAVE_64BIT_COORD)
      "  {{S_MAX_SIGNED_INTEGER_WIDTH} {8}}\n"
      "  {{S_MAX_UNSIGNED_INTEGER_WIDTH} {8}}\n"
#else
      "  {{S_MAX_SIGNED_INTEGER_WIDTH} {4}}\n"
      "  {{S_MAX_UNSIGNED_INTEGER_WIDTH} {4}}\n"
#endif
      "  {{S_TOP_CELL} {$1}}\n"
      "  {17 {17value}}\n"
      "  {{name} {117}}\n"
      "}\n"
      "begin_libp $props 0.001\n"
      "begin_cell {$2}\n"
      "end_cell\n"
      "set props {\n"
      "  {42 {42}}\n"
      "}\n"
      "begin_cellp $props {$1}\n"
      "sref {$2} 0 0 1 {0 0}\n"
      "path 1 0 0 0 0 {0 100} {1000 1200}\n"
      "end_cell\n"
      "end_lib\n"
    ;

    tl::OutputStringStream os;
    tl::OutputStream stream (os);
    db::TextWriter textwriter (stream);
    textwriter.write (gg);
    EXPECT_EQ (std::string (os.string ()), std::string (expected))
  }

  {
    std::string tmp_file = tl::TestBase::tmp_file ("tmp_dbOASISWriter116f.gds");

    {
      tl::OutputStream out (tmp_file);
      db::SaveLayoutOptions write_options;
      write_options.select_cell (c2.cell_index ());
      write_options.set_format ("OASIS");
      write_options.set_option_by_name ("oasis_strict_mode", false);
      db::Writer writer (write_options);
      writer.write (g, out);
    }

    tl::InputStream in (tmp_file);
    db::OASISReaderOptions oas_options;
    oas_options.read_all_properties = true;
    db::LoadLayoutOptions options;
    options.set_options (oas_options);
    db::Reader reader (in);
    db::Layout gg;
    reader.set_warnings_as_errors (true);
    reader.read (gg, options);

    const char *expected = 
      "set props {\n"
#if defined(HAVE_64BIT_COORD)
      "  {{S_MAX_SIGNED_INTEGER_WIDTH} {8}}\n"
      "  {{S_MAX_UNSIGNED_INTEGER_WIDTH} {8}}\n"
#else
      "  {{S_MAX_SIGNED_INTEGER_WIDTH} {4}}\n"
      "  {{S_MAX_UNSIGNED_INTEGER_WIDTH} {4}}\n"
#endif
      "  {{S_TOP_CELL} {$2}}\n"
      "  {17 {17value}}\n"
      "  {{name} {117}}\n"
      "}\n"
      "begin_libp $props 0.001\n"
      "begin_cell {$2}\n"
      "end_cell\n"
      "end_lib\n"
    ;

    tl::OutputStringStream os;
    tl::OutputStream stream (os);
    db::TextWriter textwriter (stream);
    textwriter.write (gg);
    EXPECT_EQ (std::string (os.string ()), std::string (expected))
  }
}

TEST(117)
{
  //  polygons and boxes without area
  
  db::Manager m (false);
  db::Layout g (&m);

  db::LayerProperties lp1;
  lp1.layer = 1;
  lp1.datatype = 0;

  g.insert_layer (0, lp1);

  db::Cell &c1 (g.cell (g.add_cell ()));

  c1.shapes (0).insert (db::Box (100, 0, 100, 200));
  c1.shapes (0).insert (db::Box (100, -20, 100, -20));

  db::Point pts[] = {
    db::Point (100, 15),
    db::Point (150, 15),
    db::Point (120, 15)
  };

  db::Polygon p;
  p.assign_hull (&pts[0], &pts[sizeof (pts) / sizeof(pts[0])], false);
  c1.shapes (0).insert (p);

  db::SimplePolygon ps;
  ps.assign_hull (&pts[0], &pts[sizeof (pts) / sizeof(pts[0])], false);
  ps.transform (db::FTrans (db::FTrans::r90), false);
  c1.shapes (0).insert (ps);

  std::string tmp_file = tl::TestBase::tmp_file (tl::sprintf ("tmp_dbOASISWriter117.gds"));

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("OASIS");
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
  reader.set_warnings_as_errors (true);
  reader.read (gg);

  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {$1}\n"
    "boundary 1 0 {-15 100} {-15 120} {-15 150} {-15 100}\n"
    "boundary 1 0 {100 15} {150 15} {120 15} {100 15}\n"
    "box 1 0 {100 -20} {100 -20}\n"
    "box 1 0 {100 0} {100 200}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  tl::OutputStringStream os;
  tl::OutputStream stream (os);
  db::TextWriter textwriter (stream);
  textwriter.write (gg);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(118)
{
  //  1x1 arrays (#902)

  db::Manager m (false);
  db::Layout g (&m);

  db::LayerProperties lp1;
  lp1.layer = 1;
  lp1.datatype = 0;

  g.insert_layer (0, lp1);

  db::Cell &c1 (g.cell (g.add_cell ()));
  c1.shapes (0).insert (db::Box (100, 0, 100, 200));

  db::Cell &c2 (g.cell (g.add_cell ()));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), db::Trans (), db::Vector (0, 1), db::Vector (1, 0), 1, 1));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), db::Trans (db::Vector (17, -42)), db::Vector (0, 1), db::Vector (1, 0), 1, 1));

  std::string tmp_file = tl::TestBase::tmp_file ("tmp.oas");

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("OASIS");
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
  reader.set_warnings_as_errors (true);
  reader.read (gg);

  const char *expected =
    "begin_lib 0.001\n"
    "begin_cell {$1}\n"
    "box 1 0 {100 0} {100 200}\n"
    "end_cell\n"
    "begin_cell {$2}\n"
    "sref {$1} 0 0 1 {0 0}\n"
    "sref {$1} 0 0 1 {17 -42}\n"
    "end_cell\n"
    "end_lib\n"
    ;

  tl::OutputStringStream os;
  tl::OutputStream stream (os);
  db::TextWriter textwriter (stream);
  textwriter.write (gg);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

TEST(119_WithAndWithoutContext)
{
  //  PCells with context and without

  db::Manager m (false);
  db::Layout g (&m);

  //  Note: this sample requires the BASIC lib

  {
    std::string fn (tl::testdata ());
    fn += "/oasis/pcell_test.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (g);
  }

  std::string tmp_file = tl::TestBase::tmp_file (tl::sprintf ("tmp_dbOASISWriter119a.oas"));

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("OASIS");
    db::Writer writer (options);
    writer.write (g, out);
  }

  {
    tl::InputStream in (tmp_file);
    db::Reader reader (in);
    db::Layout gg;
    reader.set_warnings_as_errors (true);
    reader.read (gg);

    std::pair<bool, db::cell_index_type> tc = gg.cell_by_name ("TEXT");
    tl_assert (tc.first);

    const db::Cell &text_cell = gg.cell (tc.second);
    EXPECT_EQ (text_cell.is_proxy (), true);
    EXPECT_EQ (text_cell.get_display_name (), "Basic.TEXT(l=1/0,'KLAYOUT RULES')");

    CHECKPOINT ();
    db::compare_layouts (_this, gg, tl::testdata () + "/oasis/dbOASISWriter119_au.gds", db::NoNormalization);
  }

  tmp_file = tl::TestBase::tmp_file (tl::sprintf ("tmp_dbOASISWriter119b.oas"));

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_write_context_info (false);
    options.set_format ("OASIS");
    db::Writer writer (options);
    writer.write (g, out);
  }

  {
    tl::InputStream in (tmp_file);
    db::Reader reader (in);
    db::Layout gg;
    reader.set_warnings_as_errors (true);
    reader.read (gg);

    std::pair<bool, db::cell_index_type> tc = gg.cell_by_name ("TEXT");
    tl_assert (tc.first);

    const db::Cell &text_cell = gg.cell (tc.second);
    EXPECT_EQ (text_cell.is_proxy (), false);
    EXPECT_EQ (text_cell.get_display_name (), "TEXT");

    CHECKPOINT ();
    db::compare_layouts (_this, gg, tl::testdata () + "/oasis/dbOASISWriter119_au.gds", db::NoNormalization);
  }

}

TEST(120_IrregularInstRepetitions)
{
  db::Manager m (false);
  db::Layout g (&m);

  db::cell_index_type top = g.add_cell ("TOP");
  db::cell_index_type c1 = g.add_cell ("C1");

  db::Vector pts[3] = { db::Vector (0, 10), db::Vector (0, 20), db::Vector (0, 30) };

  unsigned int l1 = g.insert_layer (db::LayerProperties (1, 0));
  g.cell (c1).shapes (l1).insert (db::Box (-5, -5, 5, 5));
  db::iterated_array<db::Coord> *reps = new db::iterated_array<db::Coord> (pts + 0, pts + 3);
  g.cell (top).shapes (l1).insert (db::array<db::Box, db::UnitTrans> (db::Box (-5, -5, 5, 5), db::UnitTrans (), reps));

  db::iterated_array<db::Coord> *rep = new db::iterated_array<db::Coord> (pts + 0, pts + 3);
  db::CellInstArray ci1 (db::CellInst (c1), db::Trans (db::Vector (10, 0)), rep);
  g.cell (top).insert (ci1);

  std::string tmp_file = tl::TestBase::tmp_file (tl::sprintf ("tmp_dbOASISWriter120.oas"));

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("OASIS");
    db::Writer writer (options);
    writer.write (g, out);
  }

  {
    tl::InputStream in (tmp_file);
    db::Reader reader (in);
    db::Layout gg;
    reader.set_warnings_as_errors (true);
    reader.read (gg);

    CHECKPOINT ();
    db::compare_layouts (_this, gg, tl::testdata () + "/oasis/dbOASISWriter120_au.gds", db::NoNormalization);
  }

}

//  Meta info
static void
run_test130 (tl::TestBase *_this, bool strict, bool tables_at_end)
{
  db::Layout layout_org;

  layout_org.add_cell ("U");
  db::cell_index_type ci = layout_org.add_cell ("X");

  layout_org.add_meta_info ("a", db::MetaInfo ("description", 17.5, true));
  layout_org.add_meta_info ("b", db::MetaInfo ("", "value", true));

  layout_org.add_meta_info (ci, "a", db::MetaInfo ("dd", true, true));
  layout_org.add_meta_info (ci, "c", db::MetaInfo ("d", -1, true));

  std::string tmp_file = _this->tmp_file ("tmp_OASISWriter1.oas");

  {
    tl::OutputStream out (tmp_file);
    db::OASISWriterOptions oasis_options;
    oasis_options.strict_mode = strict;
    oasis_options.tables_at_end = tables_at_end;
    db::SaveLayoutOptions options;
    options.set_format ("OASIS");
    options.set_options (oasis_options);
    db::Writer writer (options);
    writer.write (layout_org, out);
  }

  db::Layout layout_read;

  {
    tl::InputStream in (tmp_file);
    db::Reader reader (in);
    reader.read (layout_read);
  }

  EXPECT_EQ (layout_read.meta_info ("x").value.to_string (), "nil");
  EXPECT_EQ (layout_read.meta_info ("a").value.to_string (), "17.5");
  EXPECT_EQ (layout_read.meta_info ("a").description, "description");
  EXPECT_EQ (layout_read.meta_info ("b").value.to_string (), "value");
  EXPECT_EQ (layout_read.meta_info ("b").description, "");

  db::cell_index_type ci2 = layout_read.cell_by_name ("X").second;

  EXPECT_EQ (layout_read.meta_info (ci2, "x").value.to_string (), "nil");
  EXPECT_EQ (layout_read.meta_info (ci2, "a").value.to_string (), "true");
  EXPECT_EQ (layout_read.meta_info (ci2, "a").description, "dd");
  EXPECT_EQ (layout_read.meta_info (ci2, "c").value.to_string (), "-1");
  EXPECT_EQ (layout_read.meta_info (ci2, "c").description, "d");

  tmp_file = _this->tmp_file ("tmp_OASISWriter2.oas");

  {
    tl::OutputStream out (tmp_file);
    db::OASISWriterOptions oasis_options;
    oasis_options.strict_mode = strict;
    oasis_options.tables_at_end = tables_at_end;
    db::SaveLayoutOptions options;
    options.set_format ("OASIS");
    options.set_options (oasis_options);
    options.set_write_context_info (false);
    db::Writer writer (options);
    writer.write (layout_org, out);
  }

  layout_read = db::Layout ();

  {
    tl::InputStream in (tmp_file);
    db::Reader reader (in);
    reader.read (layout_read);
  }

  EXPECT_EQ (layout_read.meta_info ("x").value.to_string (), "nil");
  EXPECT_EQ (layout_read.meta_info ("a").value.to_string (), "nil");
  EXPECT_EQ (layout_read.meta_info ("b").value.to_string (), "nil");

  ci2 = layout_read.cell_by_name ("X").second;

  EXPECT_EQ (layout_read.meta_info (ci2, "x").value.to_string (), "nil");
  EXPECT_EQ (layout_read.meta_info ("a").value.to_string (), "nil");
  EXPECT_EQ (layout_read.meta_info ("b").value.to_string (), "nil");
}

//  Meta info

TEST(130a)
{
  run_test130 (_this, false, false);
}

TEST(130b)
{
  run_test130 (_this, true, false);
}

TEST(130c)
{
  run_test130 (_this, false, true);
}

TEST(130d)
{
  run_test130 (_this, true, true);
}

