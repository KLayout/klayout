
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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


#include "dbGDS2Reader.h"
#include "dbGDS2Writer.h"
#include "dbLayoutDiff.h"
#include "dbLibraryManager.h"
#include "dbLibrary.h"
#include "dbShapeProcessor.h"
#include "dbWriter.h"
#include "dbTextWriter.h"
#include "tlUnitTest.h"

#include <stdlib.h>

void run_test (tl::TestBase *_this, const char *file, const char *file_ref, bool priv = false, const db::GDS2WriterOptions &opt = db::GDS2WriterOptions ())
{
  db::Manager m (false);
  db::Layout layout_org (&m);
  {
    std::string fn (priv ? tl::testdata_private () : tl::testdata ());
    fn += "/gds/";
    fn += file;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout_org);
  }

  std::string tmp_file = _this->tmp_file ("tmp.gds");

  {
    tl::OutputStream stream (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("GDS2");
    options.set_options (new db::GDS2WriterOptions (opt));
    db::Writer writer (options);
    writer.write (layout_org, stream);
  }

  db::Layout layout_read (&m);
  {
    tl::InputStream file (tmp_file);
    db::Reader reader (file);
    reader.read (layout_read);
  }

  db::Layout layout_ref (&m);
  {
    std::string fn (priv ? tl::testdata_private () : tl::testdata ());
    fn += "/gds/";
    fn += file_ref;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout_ref);
  }

  bool equal = db::compare_layouts (layout_read, layout_ref, db::layout_diff::f_verbose, 0);
  if (! equal) {
    _this->raise (tl::sprintf ("Compare failed - see %s vs %s\n", tmp_file, file_ref));
  }
}

TEST(1)
{
  run_test (_this, "arefs.gds", "arefs_ref.gds");
}

TEST(1a)
{
  db::GDS2WriterOptions opt;
  run_test (_this, "arefs_skew.gds", "arefs_skew1.gds", false, opt);
}

TEST(1b)
{
  db::GDS2WriterOptions opt;
  opt.resolve_skew_arrays = true;
  run_test (_this, "arefs_skew.gds", "arefs_skew2.gds", false, opt);
}

TEST(2)
{
  db::Manager m (false);
  db::Layout layout_org (&m);

  db::cell_index_type cid = layout_org.add_cell ("TOP");
  db::LayerProperties lp;
  lp.layer = 1;
  lp.datatype = 0;
  unsigned int lid = layout_org.insert_layer (lp); 

  std::vector <db::Point> pts;
  for (int i = 0; i < 20000; ++i) {
    db::DPoint dp (i * cos (i * 0.01), i * sin (i * 0.01));
    pts.push_back (db::Point (dp));
  }

  db::Polygon poly;
  poly.assign_hull (pts.begin (), pts.end ());
  layout_org.cell (cid).shapes (lid).insert (poly);

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_GDS2Writer_2.gds");

  {
    tl::OutputStream stream (tmp_file);
    db::SaveLayoutOptions options;
    db::GDS2WriterOptions *opt = new db::GDS2WriterOptions ();
    opt->multi_xy_records = true;
    options.set_options (opt);
    options.set_format (opt->format_name ());
    db::Writer writer (options);
    writer.write (layout_org, stream);
  }

  db::Layout layout_read (&m);
  {
    tl::InputStream file (tmp_file);
    db::Reader reader (file);
    reader.read (layout_read);
  }

  bool equal = db::compare_layouts (layout_org, layout_read, db::layout_diff::f_verbose, 0);
  EXPECT_EQ (equal, true);
}

// Test the writer's capabilities to cut a polygon into small pieces correctly
TEST(3)
{
  db::Manager m (false);
  db::Layout layout_org (&m);
  {
    std::string fn (tl::testdata ());
    fn += "/other/d1.oas.gz";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout_org);
  }

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_GDS2Writer_3.gds");

  {
    tl::OutputStream stream (tmp_file);
    db::SaveLayoutOptions options;
    db::GDS2WriterOptions *opt = new db::GDS2WriterOptions ();
    opt->max_vertex_count = 4;
    options.set_options (opt);
    options.set_format (opt->format_name ());
    db::Writer writer (options);
    writer.write (layout_org, stream);
  }

  db::Layout layout_read (&m);
  {
    tl::InputStream file (tmp_file);
    db::Reader reader (file);
    reader.read (layout_read);
  }

  db::Cell &top_org = layout_org.cell (*layout_org.begin_top_down ());
  db::Cell &top_read = layout_read.cell (*layout_org.begin_top_down ());

  unsigned int xor_layer = layout_org.insert_layer (db::LayerProperties ());

  for (unsigned int i = 0; i < layout_org.layers (); ++i) {
    if (layout_org.is_valid_layer (i)) {
      const db::LayerProperties lp_org = layout_org.get_properties (i);
      for (unsigned int j = 0; j < layout_read.layers (); ++j) {
        if (layout_read.is_valid_layer (j) && layout_read.get_properties (j) == lp_org) {
          db::ShapeProcessor sp;
          EXPECT_EQ (top_org.shapes (i).size () * 30 < top_read.shapes (j).size (), true);
          sp.boolean (layout_org, top_org, i, 
                      layout_read, top_read, j, 
                      top_org.shapes (xor_layer), db::BooleanOp::Xor, true, false); 
          EXPECT_EQ (top_org.shapes (xor_layer).size () > 210, true);
          sp.size (layout_org, top_org, xor_layer, top_org.shapes (xor_layer), db::Coord (-1), db::Coord (-1));
          EXPECT_EQ (top_org.shapes (xor_layer).size () == 0, true);
        }
      }
    }
  }
}

// Test the writer's capabilities to write polygon's with holes
TEST(4)
{
  db::ShapeProcessor sp;

  db::Manager m (false);
  db::Layout layout_org (&m);
  {
    std::string fn (tl::testdata ());
    fn += "/other/d1.oas.gz";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout_org);
  }

  db::Cell &top_org = layout_org.cell (*layout_org.begin_top_down ());
  for (unsigned int i = 0; i < layout_org.layers (); ++i) {
    if (layout_org.is_valid_layer (i)) {
      sp.merge (layout_org, top_org, i, top_org.shapes (i), true, 0, false /*don't resolve holes*/);
    }
  }

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_GDS2Writer_4.gds");

  {
    tl::OutputStream stream (tmp_file);
    db::SaveLayoutOptions options;
    db::Writer writer (options);
    writer.write (layout_org, stream);
  }

  db::Layout layout_read (&m);
  {
    tl::InputStream file (tmp_file);
    db::Reader reader (file);
    reader.read (layout_read);
  }

  db::Cell &top_read = layout_read.cell (*layout_org.begin_top_down ());

  unsigned int xor_layer = layout_org.insert_layer (db::LayerProperties ());

  for (unsigned int i = 0; i < layout_org.layers (); ++i) {
    if (layout_org.is_valid_layer (i)) {
      const db::LayerProperties lp_org = layout_org.get_properties (i);
      for (unsigned int j = 0; j < layout_read.layers (); ++j) {
        if (layout_read.is_valid_layer (j) && layout_read.get_properties (j) == lp_org) {
          EXPECT_EQ (top_org.shapes (i).size () != top_read.shapes (j).size (), true);
          EXPECT_EQ (top_org.shapes (i).size () > 0, true);
          sp.boolean (layout_org, top_org, i, 
                      layout_read, top_read, j, 
                      top_org.shapes (xor_layer), db::BooleanOp::Xor, true, false); 
          sp.size (layout_org, top_org, xor_layer, top_org.shapes (xor_layer), db::Coord (-1), db::Coord (-1));
          EXPECT_EQ (top_org.shapes (xor_layer).size () == 0, true);
        }
      }
    }
  }
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

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_GDS2Writer_100.gds");

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("GDS2");
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
  reader.read (gg);

  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {$1}\n"
    "box 1 0 {0 100} {1000 1200}\n"
    "end_cell\n"
    "begin_cell {$4}\n"
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

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_GDS2Writer_101.gds");

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("GDS2");
    options.add_layer (0);
    options.set_dont_write_empty_cells (true);
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
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

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_GDS2Writer_102.gds");

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("GDS2");
    options.add_layer (1);
    options.set_dont_write_empty_cells (true);
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
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

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_GDS2Writer_103.gds");

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("GDS2");
    options.add_layer (2);
    options.set_dont_write_empty_cells (true);
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
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

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_GDS2Writer_110.gds");

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("GDS2");
    options.set_dont_write_empty_cells (true);
    options.add_cell (c3.cell_index ());
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
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

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_GDS2Writer_111.gds");

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("GDS2");
    options.add_cell (c3.cell_index ());
    options.add_layer (0);
    options.set_dont_write_empty_cells (true);
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
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

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_GDS2Writer_112.gds");

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("GDS2");
    options.add_cell (c3.cell_index ());
    options.add_layer (1);
    options.set_dont_write_empty_cells (true);
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
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

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_GDS2Writer_113.gds");

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("GDS2");
    options.add_cell (c3.cell_index ());
    options.add_layer (2);
    options.set_dont_write_empty_cells (true);
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
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
  // text alignment flags, font and text size

  db::Manager m (false);
  db::Layout g (&m);

  db::LayerProperties lp0;
  lp0.layer = 0;
  lp0.datatype = 0;
  g.insert_layer (0, lp0);

  db::Cell &c1 (g.cell (g.add_cell ()));

  c1.shapes (0).insert (db::Text (db::Trans (1, false, db::Vector (100, 200))));
  c1.shapes (0).insert (db::Text (db::Trans (1, false, db::Vector (100, 200)), 1000));
  c1.shapes (0).insert (db::Text (db::Trans (1, false, db::Vector (100, 200)), 1000, db::Font (7)));
  c1.shapes (0).insert (db::Text (db::Trans (1, false, db::Vector (100, 200)), 1000, db::NoFont, db::HAlignCenter, db::VAlignBottom));
  c1.shapes (0).insert (db::Text (db::Trans (1, false, db::Vector (100, 200)), 1000, db::Font (7), db::HAlignCenter, db::VAlignBottom));
  c1.shapes (0).insert (db::Text (db::Trans (1, false, db::Vector (100, 200)), 1000, db::Font (7), db::HAlignLeft, db::VAlignCenter));

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_GDS2Writer_114.gds");

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("GDS2");
    options.add_cell (c1.cell_index ());
    options.add_layer (0);
    options.set_dont_write_empty_cells (true);
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
  reader.read (gg);

  db::ShapeIterator s = gg.cell(0).shapes(0).begin(db::ShapeIterator::All);
  EXPECT_EQ (s.at_end (), false);
  EXPECT_EQ (s->is_text (), true);
  EXPECT_EQ (s->text_size (), 0);
  EXPECT_EQ ((int)s->text_font (), (int)db::NoFont);
  EXPECT_EQ ((int)s->text_halign (), (int)db::NoHAlign);
  EXPECT_EQ ((int)s->text_valign (), (int)db::NoVAlign);
  ++s;
  EXPECT_EQ (s.at_end (), false);
  EXPECT_EQ (s->is_text (), true);
  EXPECT_EQ (s->text_size (), 1000);
  EXPECT_EQ ((int)s->text_font (), (int)db::NoFont);
  EXPECT_EQ ((int)s->text_halign (), (int)db::NoHAlign);
  EXPECT_EQ ((int)s->text_valign (), (int)db::NoVAlign);
  ++s;
  EXPECT_EQ (s.at_end (), false);
  EXPECT_EQ (s->is_text (), true);
  EXPECT_EQ (s->text_size (), 1000);
  // Right now, the font is not written
  // EXPECT_EQ ((int)s->text_font (), (int)db::Font (7));
  EXPECT_EQ ((int)s->text_halign (), (int)db::HAlignLeft); //  NoAlign -> default
  EXPECT_EQ ((int)s->text_valign (), (int)db::VAlignBottom); //  NoAlign -> default
  ++s;
  EXPECT_EQ (s.at_end (), false);
  EXPECT_EQ (s->is_text (), true);
  EXPECT_EQ (s->text_size (), 1000);
  // Right now, the font is not written
  // EXPECT_EQ ((int)s->text_font (), (int)db::Font (0));
  EXPECT_EQ ((int)s->text_halign (), (int)db::HAlignCenter);
  EXPECT_EQ ((int)s->text_valign (), (int)db::VAlignBottom);
  ++s;
  EXPECT_EQ (s.at_end (), false);
  EXPECT_EQ (s->is_text (), true);
  EXPECT_EQ (s->text_size (), 1000);
  // Right now, the font is not written
  // EXPECT_EQ ((int)s->text_font (), (int)db::Font (7));
  EXPECT_EQ ((int)s->text_halign (), (int)db::HAlignCenter);
  EXPECT_EQ ((int)s->text_valign (), (int)db::VAlignBottom);
  ++s;
  EXPECT_EQ (s.at_end (), false);
  EXPECT_EQ (s->is_text (), true);
  EXPECT_EQ (s->text_size (), 1000);
  // Right now, the font is not written
  // EXPECT_EQ ((int)s->text_font (), (int)db::Font (7));
  EXPECT_EQ ((int)s->text_halign (), (int)db::HAlignLeft);
  EXPECT_EQ ((int)s->text_valign (), (int)db::VAlignCenter);
  ++s;
  EXPECT_EQ (s.at_end (), true);
  
}

TEST(115)
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

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_GDS2Writer_115.gds");

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("GDS2");
    db::Writer writer (options);
    writer.write (g, out);
  }

  tl::InputStream in (tmp_file);
  db::Reader reader (in);
  db::Layout gg;
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

TEST(116)
{
  //  big paths with multi-xy

  db::Manager m (false);
  db::Layout g (&m);

  db::LayerProperties lp1;
  lp1.layer = 1;
  lp1.datatype = 0;

  g.insert_layer (0, lp1);

  db::Cell &c1 (g.cell (g.add_cell ("TOP")));

  db::Path path;
  path.width (100);
  std::vector<db::Point> pts;
  for (int i = 0; i < 10000; ++i) {
    pts.push_back (db::Point (i * 10, (i % 10) * 1000));
  }
  path.assign (pts.begin (), pts.end ());
  c1.shapes (0).insert (path);

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_GDS2Writer_116.gds");

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    db::GDS2WriterOptions gds2_options;
    gds2_options.multi_xy_records = true;
    options.set_format ("GDS2");
    options.set_options (gds2_options);
    db::Writer writer (options);
    writer.write (g, out);
  }

  db::Layout gg;

  {
    db::LoadLayoutOptions options;
    db::GDS2ReaderOptions gds2_options;
    gds2_options.allow_multi_xy_records = true;
    options.set_options (gds2_options);
    tl::InputStream in (tmp_file);
    db::Reader reader (in);
    reader.read (gg);
  }

  EXPECT_EQ (gg.cell_by_name ("TOP").first, true);
  const db::Cell &cc1 (gg.cell (gg.cell_by_name ("TOP").second));

  EXPECT_EQ (gg.get_properties (0) == lp1, true);
  EXPECT_EQ (cc1.shapes (0).size (), size_t (1));

  db::Shape s1 = *cc1.shapes (0).begin (db::ShapeIterator::All);
  db::Path pp;
  s1.path (pp);
  EXPECT_EQ (pp == path, true);
}

TEST(117)
{
  //  big polygons with multi-xy

  db::Manager m (false);
  db::Layout g (&m);

  db::LayerProperties lp1;
  lp1.layer = 1;
  lp1.datatype = 0;

  g.insert_layer (0, lp1);

  db::Cell &c1 (g.cell (g.add_cell ("TOP")));

  db::Polygon poly;
  std::vector<db::Point> pts;
  for (int i = 0; i < 10000; ++i) {
    pts.push_back (db::Point (i * 10, (i % 10) * 1000));
  }
  poly.assign_hull (pts.begin (), pts.end ());
  c1.shapes (0).insert (poly);

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_GDS2Writer_117.gds");

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    db::GDS2WriterOptions gds2_options;
    gds2_options.multi_xy_records = true;
    options.set_format ("GDS2");
    options.set_options (gds2_options);
    db::Writer writer (options);
    writer.write (g, out);
  }

  db::Layout gg;

  {
    db::LoadLayoutOptions options;
    db::GDS2ReaderOptions gds2_options;
    gds2_options.allow_multi_xy_records = true;
    options.set_options (gds2_options);
    tl::InputStream in (tmp_file);
    db::Reader reader (in);
    reader.read (gg);
  }

  EXPECT_EQ (gg.cell_by_name ("TOP").first, true);
  const db::Cell &cc1 (gg.cell (gg.cell_by_name ("TOP").second));

  EXPECT_EQ (gg.get_properties (0) == lp1, true);
  EXPECT_EQ (cc1.shapes (0).size (), size_t (1));

  db::Shape s1 = *cc1.shapes (0).begin (db::ShapeIterator::All);
  db::Polygon pp;
  s1.polygon (pp);
  EXPECT_EQ (pp == poly, true);
}

//  error on duplicate cell name
TEST(118)
{
  db::Manager m (false);
  db::Layout layout_org (&m);

  db::cell_index_type cid1 = layout_org.add_cell ("A");
  db::cell_index_type cid2 = layout_org.add_cell ("B");
  layout_org.rename_cell (cid2, "A");  //  creates a duplicate cell

  db::LayerProperties lp;
  lp.layer = 1;
  lp.datatype = 0;
  unsigned int lid = layout_org.insert_layer (lp);

  layout_org.cell (cid1).shapes (lid).insert (db::Box (0, 0, 1000, 2000));
  layout_org.cell (cid2).shapes (lid).insert (db::Box (0, 0, 1000, 2000));

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_GDS2Writer_117.gds");

  bool error = false;
  try {
    tl::OutputStream stream (tmp_file);
    db::SaveLayoutOptions options;
    db::Writer writer (options);
    writer.write (layout_org, stream);
  } catch (tl::Exception &ex) {
    tl::warn << ex.msg ();
    error = true;
  }

  EXPECT_EQ (error, true);
}

//  Extreme fracturing by max. points
TEST(120)
{
  db::GDS2WriterOptions opt;
  opt.max_vertex_count = 4;
  run_test (_this, "t120a.oas.gz", "t120a_au.gds.gz", true, opt);
  run_test (_this, "t120b.oas.gz", "t120b_au.gds.gz", true, opt);
}

//  Extreme fracturing by max. points
TEST(121)
{
  db::GDS2WriterOptions opt;
  opt.max_vertex_count = 4;
  run_test (_this, "t121.oas.gz", "t121_au.gds.gz", true, opt);
}

//  Meta info
TEST(130)
{
  db::Layout layout_org;

  db::Library *lib = db::LibraryManager::instance ().lib_ptr_by_name ("Basic");
  std::pair<bool, db::pcell_id_type> pc = lib->layout ().pcell_by_name ("TEXT");

  layout_org.add_cell ("U");
  db::cell_index_type ci = layout_org.add_cell ("X");

  std::map<std::string, tl::Variant> params;
  params.insert (std::make_pair ("text", "ABC"));
  params.insert (std::make_pair ("layer", db::LayerProperties (1, 0)));
  db::cell_index_type lib_cell = lib->layout ().get_pcell_variant_dict (pc.second, params);
  db::cell_index_type cil = layout_org.get_lib_proxy (lib, lib_cell);

  //  the proxy needs an instance otherwise it is cleaned away before writing
  layout_org.cell (ci).insert (db::CellInstArray (cil, db::Trans ()));

  layout_org.add_meta_info ("a", db::MetaInfo ("description", 17.5, true));
  layout_org.add_meta_info ("b", db::MetaInfo ("", "value", true));

  layout_org.add_meta_info (ci, "a", db::MetaInfo ("dd", true, true));
  layout_org.add_meta_info (ci, "c", db::MetaInfo ("d", -1, true));

  layout_org.add_meta_info (cil, "x", db::MetaInfo ("", 42, true));
  layout_org.add_meta_info (cil, "y", db::MetaInfo ("", -17, true));

  //  complex type
  tl::Variant v2;
  v2.set_array ();
  v2.insert ("x", "value_for_x");
  v2.insert ("y", db::DBox (1.5, 2.5, 3.5, 4.5));
  tl::Variant v1;
  v1.set_list (0);
  v1.push (-1.5);
  v1.push (v2);
  layout_org.add_meta_info (ci, "complex", db::MetaInfo ("", v1, true));
  layout_org.add_meta_info ("complex", db::MetaInfo ("", v1, true));

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_GDS2Writer_130a.gds");

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    db::Writer writer (options);
    writer.write (layout_org, out);
  }

  db::Layout layout_read;

  {
    tl::InputStream in (tmp_file);
    db::Reader reader (in);
    reader.read (layout_read);
  }

  EXPECT_EQ (layout_read.has_meta_info ("x"), false);
  EXPECT_EQ (layout_read.has_meta_info ("a"), true);
  EXPECT_EQ (layout_read.meta_info ("x").value.to_string (), "nil");
  EXPECT_EQ (layout_read.meta_info ("a").value.to_string (), "17.5");
  EXPECT_EQ (layout_read.meta_info ("a").description, "description");
  EXPECT_EQ (layout_read.has_meta_info ("b"), true);
  EXPECT_EQ (layout_read.meta_info ("b").value.to_string (), "value");
  EXPECT_EQ (layout_read.meta_info ("b").description, "");
  EXPECT_EQ (layout_read.has_meta_info ("complex"), true);
  EXPECT_EQ (layout_read.meta_info ("complex").value.is_list (), true);
  EXPECT_EQ (layout_read.meta_info ("complex").value.size (), size_t (2));
  EXPECT_EQ (layout_read.meta_info ("complex").value.begin () [1].is_array (), true);
  EXPECT_EQ (layout_read.meta_info ("complex").value.to_string (), "-1.5,x=>value_for_x,y=>(1.5,2.5;3.5,4.5)");

  db::cell_index_type ci2 = layout_read.cell_by_name ("X").second;
  db::cell_index_type cil2 = layout_read.cell_by_name ("TEXT").second;

  EXPECT_EQ (layout_read.meta_info (ci2, "x").value.to_string (), "nil");
  EXPECT_EQ (layout_read.meta_info (ci2, "a").value.to_string (), "true");
  EXPECT_EQ (layout_read.meta_info (ci2, "a").description, "dd");
  EXPECT_EQ (layout_read.meta_info (ci2, "c").value.to_string (), "-1");
  EXPECT_EQ (layout_read.meta_info (ci2, "c").description, "d");
  EXPECT_EQ (layout_read.meta_info (ci2, "complex").value.is_list (), true);
  EXPECT_EQ (layout_read.meta_info (ci2, "complex").value.size (), size_t (2));
  EXPECT_EQ (layout_read.meta_info (ci2, "complex").value.begin () [1].is_array (), true);
  EXPECT_EQ (layout_read.meta_info (ci2, "complex").value.to_string (), "-1.5,x=>value_for_x,y=>(1.5,2.5;3.5,4.5)");

  EXPECT_EQ (layout_read.meta_info (cil2, "x").value.to_string (), "42");
  EXPECT_EQ (layout_read.meta_info (cil2, "y").value.to_string (), "-17");

  tmp_file = tl::TestBase::tmp_file ("tmp_GDS2Writer_130b.gds");

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
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

//  Giant meta info (issue #1794)
TEST(131)
{
  db::Layout layout_org;

  layout_org.add_cell ("U");
  db::cell_index_type ci = layout_org.add_cell ("X");

  std::vector<tl::Variant> ll1;
  std::vector<tl::Variant> ll2;

  for (unsigned int i = 0; i < 100000; ++i) {
    ll1.push_back (tl::Variant (i));
    ll2.push_back ("C" + tl::to_string (i));
  }

  layout_org.add_meta_info ("a", db::MetaInfo ("", ll1, true));
  layout_org.add_meta_info ("b", db::MetaInfo ("", "value", true));

  layout_org.add_meta_info (ci, "a", db::MetaInfo ("", ll2, true));
  layout_org.add_meta_info (ci, "c", db::MetaInfo ("", -1, true));

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_GDS2Writer_131.gds");

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    db::Writer writer (options);
    writer.write (layout_org, out);
  }

  db::Layout layout_read;

  {
    tl::InputStream in (tmp_file);
    db::Reader reader (in);
    reader.read (layout_read);
  }

  EXPECT_EQ (layout_read.has_meta_info ("x"), false);
  EXPECT_EQ (layout_read.has_meta_info ("a"), true);
  EXPECT_EQ (layout_read.meta_info ("x").value.to_string (), "nil");
  EXPECT_EQ (layout_read.meta_info ("a").value == ll1, true);
  EXPECT_EQ (layout_read.has_meta_info ("b"), true);
  EXPECT_EQ (layout_read.meta_info ("b").value.to_string (), "value");

  db::cell_index_type ci2 = layout_read.cell_by_name ("X").second;

  EXPECT_EQ (layout_read.meta_info (ci2, "x").value.to_string (), "nil");
  EXPECT_EQ (layout_read.meta_info (ci2, "a").value == ll2, true);
  EXPECT_EQ (layout_read.meta_info (ci2, "c").value.to_string (), "-1");
}

//  Many meta info (issue #1794)
TEST(132)
{
  db::Layout layout_org;

  layout_org.add_cell ("U");
  db::cell_index_type ci = layout_org.add_cell ("X");

  for (unsigned int i = 0; i < 100000; ++i) {
    layout_org.add_meta_info ("a" + tl::to_string (i), db::MetaInfo ("", i, true));
  }
  layout_org.add_meta_info ("b", db::MetaInfo ("", "value", true));

  for (unsigned int i = 0; i < 100000; ++i) {
    layout_org.add_meta_info (ci, "a" + tl::to_string (i * 2), db::MetaInfo ("", i * 2, true));
  }
  layout_org.add_meta_info (ci, "c", db::MetaInfo ("", -1, true));

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_GDS2Writer_132.gds");

  {
    tl::OutputStream out (tmp_file);
    db::SaveLayoutOptions options;
    db::Writer writer (options);
    writer.write (layout_org, out);
  }

  db::Layout layout_read;

  {
    tl::InputStream in (tmp_file);
    db::Reader reader (in);
    reader.read (layout_read);
  }

  EXPECT_EQ (layout_read.has_meta_info ("x"), false);
  EXPECT_EQ (layout_read.meta_info ("x").value.to_string (), "nil");
  for (unsigned int i = 0; i < 10; ++i) {
    EXPECT_EQ (layout_read.has_meta_info ("a" + tl::to_string (i)), true);
    EXPECT_EQ (layout_read.meta_info ("a" + tl::to_string (i)).value.to_string (), tl::Variant (i).to_string ());
  }
  EXPECT_EQ (layout_read.has_meta_info ("b"), true);
  EXPECT_EQ (layout_read.meta_info ("b").value.to_string (), "value");

  db::cell_index_type ci2 = layout_read.cell_by_name ("X").second;

  EXPECT_EQ (layout_read.meta_info (ci2, "x").value.to_string (), "nil");
  for (unsigned int i = 0; i < 10; ++i) {
    EXPECT_EQ (layout_read.has_meta_info (ci2, "a" + tl::to_string (i * 2)), true);
    EXPECT_EQ (layout_read.meta_info (ci2, "a" + tl::to_string (i * 2)).value.to_string (), tl::Variant (i * 2).to_string ());
  }
  EXPECT_EQ (layout_read.meta_info (ci2, "c").value.to_string (), "-1");
}

//  Limits
static std::string run_test_with_error (double sf, db::Layout &layout)
{
  try {

    tl::OutputMemoryStream buffer;
    tl::OutputStream stream (buffer);

    db::SaveLayoutOptions options;
    options.set_format ("GDS2");
    options.set_scale_factor (sf);

    db::Writer writer (options);
    writer.write (layout, stream);

    return std::string ();

  } catch (tl::Exception &ex) {
    return ex.msg ();
  }
}

static std::string huge_string ()
{
  std::string n;
  for (unsigned int i = 0; i < 100000; ++i) {
    n += "A";
  }
  return n;
}

//  Exceeding limits
TEST(140)
{
  db::Layout layout;
  db::cell_index_type top_index = layout.add_cell ("TOP");
  db::Cell &top = layout.cell (top_index);
  unsigned int l1 = layout.insert_layer (db::LayerProperties (1, 0));
  top.shapes (l1).insert (db::Text (huge_string (), db::Trans ()));

  EXPECT_EQ (run_test_with_error (1.0, layout), "String max. length overflow, writing layer 1/0, writing cell 'TOP'");
}

TEST(141)
{
  db::Layout layout;
  db::cell_index_type top_index = layout.add_cell ("TOP");
  db::Cell &top = layout.cell (top_index);
  unsigned int l1 = layout.insert_layer (db::LayerProperties (100000, 0));
  top.shapes (l1).insert (db::Box (0, 0, 100, 200));

  EXPECT_EQ (run_test_with_error (1.0, layout), "Cannot write layer numbers larger than 65535 to GDS2 streams, writing cell 'TOP'");
}

TEST(142)
{
  db::Layout layout;
  db::cell_index_type top_index = layout.add_cell ("TOP");
  db::Cell &top = layout.cell (top_index);
  db::cell_index_type child_index = layout.add_cell ("CHILD");
  db::Cell &child = layout.cell (child_index);
  unsigned int l1 = layout.insert_layer (db::LayerProperties (1, 0));
  child.shapes (l1).insert (db::Box (0, 0, 100, 200));

  top.insert (db::CellInstArray (child_index, db::Trans (), db::Vector (100000000, 0), db::Vector (0, 100000000), 10, 10));
  EXPECT_EQ (run_test_with_error (1.0, layout), "");  //  no error

  top.clear_insts ();

  top.insert (db::CellInstArray (child_index, db::Trans (), db::Vector (100000000, 0), db::Vector (0, 100000000), 100, 100));
  EXPECT_EQ (run_test_with_error (1.0, layout), "Coordinate overflow, writing instances, writing cell 'TOP'");

  top.clear_insts ();

  top.insert (db::CellInstArray (child_index, db::Trans (), db::Vector (100, 0), db::Vector (0, 100), 100000, 100));
  EXPECT_EQ (run_test_with_error (1.0, layout), "Cannot write array references with more than 32767 columns or rows to GDS2 streams, writing instances, writing cell 'TOP'");
}

TEST(143)
{
  db::Layout layout;
  db::cell_index_type top_index = layout.add_cell ("TOP");
  db::Cell &top = layout.cell (top_index);
  unsigned int l1 = layout.insert_layer (db::LayerProperties (1, 0));
  top.shapes (l1).insert (db::Box (-2000000000, 0, 0, 200000000));

  EXPECT_EQ (run_test_with_error (1.0, layout), "");
  EXPECT_EQ (run_test_with_error (23.0, layout), "Scaling failed: coordinate underflow, writing layer 1/0, writing cell 'TOP'");
}

//  Extreme fracturing by max. points
TEST(166)
{
  db::GDS2WriterOptions opt;
  opt.max_vertex_count = 4;
  run_test (_this, "t166.oas.gz", "t166_au.gds.gz", false, opt);
}



