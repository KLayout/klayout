
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

#include "dbReader.h"
#include "dbRegion.h"
#include "dbDXFWriter.h"
#include "dbDXFFormat.h"
#include "dbTestSupport.h"
#include "dbRecursiveShapeIterator.h"
#include "tlUnitTest.h"
#include "tlStream.h"

#include <stdlib.h>

static void do_run_test (tl::TestBase *_this, db::Layout &layout, const std::string &fn_au, const db::DXFWriterOptions &opt)
{
  std::string tmp = _this->tmp_file ("tmp.dxf");

  db::SaveLayoutOptions options;
  options.set_options (new db::DXFWriterOptions (opt));
  options.set_format ("DXF");

  {
    tl::OutputStream stream (tmp);
    db::Writer writer (options);
    writer.write (layout, stream);
  }

  _this->compare_text_files (tmp, fn_au);
}

static void run_test (tl::TestBase *_this, const char *file, const char *file_au, const db::DXFWriterOptions &opt = db::DXFWriterOptions ())
{
  std::string fn = tl::testdata_private () + "/dxf/" + file;

  db::Layout layout;

  {
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout);
  }

  std::string fn_au = tl::testdata_private () + std::string ("/dxf/") + file_au;

  do_run_test (_this, layout, fn_au, opt);
}

static void run_test_public (tl::TestBase *_this, const char *file, const char *file_au, const db::DXFWriterOptions &opt = db::DXFWriterOptions ())
{
  std::string fn = tl::testdata () + "/dxf/" + file;
  std::string fn_au = tl::testdata () + std::string ("/dxf/") + file_au;

  db::Layout layout;

  {
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout);
  }

  do_run_test (_this, layout, fn_au, opt);
}

TEST(Polygons1a)
{
  db::DXFWriterOptions opt;
  run_test_public (_this, "dxf1.gds", "dxf1a_au.dxf", opt);
}

TEST(Polygons1b)
{
  db::DXFWriterOptions opt;
  opt.polygon_mode = 1;
  run_test_public (_this, "dxf1.gds", "dxf1b_au.dxf", opt);
}

TEST(Polygons1c)
{
  db::DXFWriterOptions opt;
  opt.polygon_mode = 2;
  run_test_public (_this, "dxf1.gds", "dxf1c_au.dxf", opt);
}

TEST(Polygons1d)
{
  db::DXFWriterOptions opt;
  opt.polygon_mode = 3;
  run_test_public (_this, "dxf1.gds", "dxf1d_au.dxf", opt);
}

TEST(Polygons1e)
{
  db::DXFWriterOptions opt;
  opt.polygon_mode = 4;
  run_test_public (_this, "dxf1.gds", "dxf1e_au.dxf", opt);
}

TEST(Polygons2)
{
  db::DXFWriterOptions opt;
  run_test_public (_this, "dxf2.gds", "dxf2_au.dxf", opt);
}

TEST(Polygons3)
{
  db::DXFWriterOptions opt;
  run_test_public (_this, "dxf3.gds", "dxf3_au.dxf", opt);
}

TEST(Polygons4a)
{
  db::Layout l;
  std::string fn = tl::testdata () + "/dxf/dxf4.gds";

  {
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l);
  }

  unsigned int l1 = l.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = l.get_layer (db::LayerProperties (2, 0));
  unsigned int l100 = l.get_layer (db::LayerProperties (100, 0));

  db::Region r1 = db::Region (db::RecursiveShapeIterator (l, l.cell (*l.begin_top_down ()), l1));
  db::Region r2 = db::Region (db::RecursiveShapeIterator (l, l.cell (*l.begin_top_down ()), l2));
  (r1 ^ r2).insert_into (&l, *l.begin_top_down (), l100);

  db::DXFWriterOptions opt;
  do_run_test (_this, l, tl::testdata () + std::string ("/dxf/") + "dxf4a_au.dxf", opt);
}

TEST(Polygons4b)
{
  db::Layout l;
  std::string fn = tl::testdata () + "/dxf/dxf4.gds";

  {
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l);
  }

  unsigned int l1 = l.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = l.get_layer (db::LayerProperties (2, 0));
  unsigned int l100 = l.get_layer (db::LayerProperties (100, 0));

  db::Region r1 = db::Region (db::RecursiveShapeIterator (l, l.cell (*l.begin_top_down ()), l1));
  db::Region r2 = db::Region (db::RecursiveShapeIterator (l, l.cell (*l.begin_top_down ()), l2));
  (r1 ^ r2).insert_into (&l, *l.begin_top_down (), l100);

  db::DXFWriterOptions opt;
  opt.polygon_mode = 1;
  do_run_test (_this, l, tl::testdata () + std::string ("/dxf/") + "dxf4b_au.dxf", opt);
}

TEST(Polygons4c)
{
  db::Layout l;
  std::string fn = tl::testdata () + "/dxf/dxf4.gds";

  {
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l);
  }

  unsigned int l1 = l.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = l.get_layer (db::LayerProperties (2, 0));
  unsigned int l100 = l.get_layer (db::LayerProperties (100, 0));

  db::Region r1 = db::Region (db::RecursiveShapeIterator (l, l.cell (*l.begin_top_down ()), l1));
  db::Region r2 = db::Region (db::RecursiveShapeIterator (l, l.cell (*l.begin_top_down ()), l2));
  (r1 ^ r2).insert_into (&l, *l.begin_top_down (), l100);

  db::DXFWriterOptions opt;
  opt.polygon_mode = 2;
  do_run_test (_this, l, tl::testdata () + std::string ("/dxf/") + "dxf4c_au.dxf", opt);
}

TEST(Polygons4d)
{
  db::Layout l;
  std::string fn = tl::testdata () + "/dxf/dxf4.gds";

  {
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l);
  }

  unsigned int l1 = l.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = l.get_layer (db::LayerProperties (2, 0));
  unsigned int l100 = l.get_layer (db::LayerProperties (100, 0));

  db::Region r1 = db::Region (db::RecursiveShapeIterator (l, l.cell (*l.begin_top_down ()), l1));
  db::Region r2 = db::Region (db::RecursiveShapeIterator (l, l.cell (*l.begin_top_down ()), l2));
  (r1 ^ r2).insert_into (&l, *l.begin_top_down (), l100);

  db::DXFWriterOptions opt;
  opt.polygon_mode = 3;
  do_run_test (_this, l, tl::testdata () + std::string ("/dxf/") + "dxf4d_au.dxf", opt);
}

TEST(Polygons4e)
{
  db::Layout l;
  std::string fn = tl::testdata () + "/dxf/dxf4.gds";

  {
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (l);
  }

  unsigned int l1 = l.get_layer (db::LayerProperties (1, 0));
  unsigned int l2 = l.get_layer (db::LayerProperties (2, 0));
  unsigned int l100 = l.get_layer (db::LayerProperties (100, 0));

  db::Region r1 = db::Region (db::RecursiveShapeIterator (l, l.cell (*l.begin_top_down ()), l1));
  db::Region r2 = db::Region (db::RecursiveShapeIterator (l, l.cell (*l.begin_top_down ()), l2));
  (r1 ^ r2).insert_into (&l, *l.begin_top_down (), l100);

  db::DXFWriterOptions opt;
  opt.polygon_mode = 4;
  do_run_test (_this, l, tl::testdata () + std::string ("/dxf/") + "dxf4e_au.dxf", opt);
}
