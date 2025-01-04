
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


#include "dbLayoutDiff.h"
#include "dbWriter.h"
#include "dbLoadLayoutOptions.h"
#include "dbReader.h"
#include "dbTestSupport.h"
#include "dbGerberImporter.h"

#include "tlUnitTest.h"
#include "tlXMLParser.h"

#include <stdlib.h>

static void run_test (tl::TestBase *_this, const char *dir)
{
  if (! tl::XMLParser::is_available ()) {
    throw tl::CancelException ();
  }

  db::LoadLayoutOptions options;

  db::Layout layout;

  {
    std::string fn (tl::testdata_private ());
    fn += "/pcb/";
    fn += dir;
    fn += "/import.pcb";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout, options);
  }

  db::compare_layouts (_this, layout, tl::testdata_private () + "/pcb/" + dir + "/au.oas.gz", db::WriteOAS, 1);
}

TEST(0_Metadata)
{
  db::GerberMetaData data;

  std::string fn;

  fn = tl::testdata_private ();
  fn += "/pcb/metadata/1.gbr";
  data = db::GerberImporter::scan (fn);

  EXPECT_EQ (data.creation_date, "2017-09-07T21:37;00+01:00");
  EXPECT_EQ (data.generation_software, "KLayout,0.25");
  EXPECT_EQ (data.function, db::GerberMetaData::Copper);
  EXPECT_EQ (data.position, db::GerberMetaData::Top);
  EXPECT_EQ (data.from_cu, 0);
  EXPECT_EQ (data.to_cu, 0);
  EXPECT_EQ (data.cu_layer_number, 1);

  fn = tl::testdata_private ();
  fn += "/pcb/metadata/2.gbr";
  data = db::GerberImporter::scan (fn);

  EXPECT_EQ (data.creation_date, "2017-09-07T21:37;00+01:00");
  EXPECT_EQ (data.generation_software, "KLayout,0.25");
  EXPECT_EQ (data.function, db::GerberMetaData::Copper);
  EXPECT_EQ (data.position, db::GerberMetaData::Bottom);
  EXPECT_EQ (data.from_cu, 0);
  EXPECT_EQ (data.to_cu, 0);
  EXPECT_EQ (data.cu_layer_number, 4);

  fn = tl::testdata_private ();
  fn += "/pcb/metadata/3.gbr";
  data = db::GerberImporter::scan (fn);

  EXPECT_EQ (data.creation_date, "2017-09-07T21:37;00+01:00");
  EXPECT_EQ (data.generation_software, "KLayout,0.25");
  EXPECT_EQ (data.function, db::GerberMetaData::Copper);
  EXPECT_EQ (data.position, db::GerberMetaData::Inner);
  EXPECT_EQ (data.from_cu, 0);
  EXPECT_EQ (data.to_cu, 0);
  EXPECT_EQ (data.cu_layer_number, 2);

  fn = tl::testdata_private ();
  fn += "/pcb/metadata/10.gbr";
  data = db::GerberImporter::scan (fn);

  EXPECT_EQ (data.creation_date, "2017-09-07T21:37;00+01:00");
  EXPECT_EQ (data.generation_software, "KLayout,0.25");
  EXPECT_EQ (data.function, db::GerberMetaData::Legend);
  EXPECT_EQ (data.position, db::GerberMetaData::Top);
  EXPECT_EQ (data.from_cu, 0);
  EXPECT_EQ (data.to_cu, 0);
  EXPECT_EQ (data.cu_layer_number, 0);

  fn = tl::testdata_private ();
  fn += "/pcb/metadata/11.gbr";
  data = db::GerberImporter::scan (fn);

  EXPECT_EQ (data.creation_date, "2017-09-07T21:37;00+01:00");
  EXPECT_EQ (data.generation_software, "KLayout,0.25");
  EXPECT_EQ (data.function, db::GerberMetaData::SolderMask);
  EXPECT_EQ (data.position, db::GerberMetaData::Top);
  EXPECT_EQ (data.from_cu, 0);
  EXPECT_EQ (data.to_cu, 0);
  EXPECT_EQ (data.cu_layer_number, 0);

  fn = tl::testdata_private ();
  fn += "/pcb/metadata/12.gbr";
  data = db::GerberImporter::scan (fn);

  EXPECT_EQ (data.creation_date, "2017-09-07T21:37;00+01:00");
  EXPECT_EQ (data.generation_software, "KLayout,0.25");
  EXPECT_EQ (data.function, db::GerberMetaData::PlatedHole);
  EXPECT_EQ (data.position, db::GerberMetaData::NoPosition);
  EXPECT_EQ (data.from_cu, 1);
  EXPECT_EQ (data.to_cu, 4);
  EXPECT_EQ (data.cu_layer_number, 0);

  fn = tl::testdata_private ();
  fn += "/pcb/metadata/13.gbr";
  data = db::GerberImporter::scan (fn);

  EXPECT_EQ (data.creation_date, "2017-09-07T21:37;00+01:00");
  EXPECT_EQ (data.generation_software, "KLayout,0.25");
  EXPECT_EQ (data.function, db::GerberMetaData::NonPlatedHole);
  EXPECT_EQ (data.position, db::GerberMetaData::NoPosition);
  EXPECT_EQ (data.from_cu, 1);
  EXPECT_EQ (data.to_cu, 4);
  EXPECT_EQ (data.cu_layer_number, 0);

  fn = tl::testdata_private ();
  fn += "/pcb/metadata/20.drl";
  data = db::GerberImporter::scan (fn);

  EXPECT_EQ (data.creation_date, "");
  EXPECT_EQ (data.generation_software, "");
  EXPECT_EQ (data.function, db::GerberMetaData::Hole);
  EXPECT_EQ (data.position, db::GerberMetaData::NoPosition);
  EXPECT_EQ (data.from_cu, 0);
  EXPECT_EQ (data.to_cu, 0);
  EXPECT_EQ (data.cu_layer_number, 0);
}

TEST(1)
{
  test_is_long_runner ();
  run_test (_this, "microchip-1");
}

TEST(2)
{
  run_test (_this, "allegro");
}

TEST(3)
{
  run_test (_this, "sample-board");
}

TEST(4)
{
  run_test (_this, "exc-test");
}

TEST(5)
{
  run_test (_this, "burstDrive");
}

TEST(6)
{
  run_test (_this, "mentor");
}

TEST(7)
{
  test_is_long_runner ();
  run_test (_this, "pcb-1");
}

TEST(8)
{
  test_is_long_runner ();
  run_test (_this, "microchip-2");
}

TEST(9)
{
  test_is_long_runner ();
  run_test (_this, "microchip-3");
}

TEST(10)
{
  run_test (_this, "gerbv_examples/am-test");
}

TEST(11)
{
  run_test (_this, "gerbv_examples/trailing");
}

TEST(12)
{
  run_test (_this, "gerbv_examples/nollezappare");
}

TEST(13)
{
  run_test (_this, "gerbv_examples/thermal");
}

TEST(14)
{
  test_is_long_runner ();
  run_test (_this, "gerbv_examples/amacro-ref");
}

TEST(15)
{
  run_test (_this, "gerbv_examples/polarity");
}

TEST(16)
{
  test_is_long_runner ();
  run_test (_this, "gerbv_examples/protel-pnp");
}

TEST(17)
{
  run_test (_this, "gerbv_examples/hellboard");
}

TEST(18)
{
  run_test (_this, "gerbv_examples/274X");
}

TEST(19)
{
  run_test (_this, "gerbv_examples/eaglecad1");
}

TEST(20)
{
  run_test (_this, "gerbv_examples/jj");
}

TEST(21)
{
  run_test (_this, "gerbv_examples/orcad");
}

TEST(22)
{
  run_test (_this, "gerbv_examples/pick-and-place");
}

TEST(23)
{
  run_test (_this, "gerbv_examples/Mentor-BoardStation");
}

TEST(24)
{
  test_is_long_runner ();
  run_test (_this, "sr-sample");
}

TEST(25)
{
  run_test (_this, "sr-sample2");
}

TEST(26)
{
  test_is_long_runner ();
  run_test (_this, "pos-neg");
}

TEST(27)
{
  run_test (_this, "polygon-mode");
}

TEST(X2_1)
{
  run_test (_this, "x2-1");
}

TEST(X2_2a)
{
  run_test (_this, "x2-2a");
}

TEST(X2_2b)
{
  run_test (_this, "x2-2b");
}

TEST(X2_2c)
{
  run_test (_this, "x2-2c");
}

TEST(X2_2d)
{
  run_test (_this, "x2-2d");
}

TEST(X2_2e)
{
  run_test (_this, "x2-2e");
}

TEST(X2_2f)
{
  run_test (_this, "x2-2f");
}

TEST(X2_2g)
{
  run_test (_this, "x2-2g");
}

TEST(X2_2h)
{
  run_test (_this, "x2-2h");
}

TEST(X2_2i)
{
  run_test (_this, "x2-2i");
}

TEST(X2_2j)
{
  run_test (_this, "x2-2j");
}

TEST(X2_2k)
{
  run_test (_this, "x2-2k");
}

TEST(X2_3)
{
  run_test (_this, "x2-3");
}

TEST(X2_4)
{
  run_test (_this, "x2-4");
}

TEST(X2_5a)
{
  run_test (_this, "x2-5a");
}

TEST(X2_5b)
{
  run_test (_this, "x2-5b");
}
