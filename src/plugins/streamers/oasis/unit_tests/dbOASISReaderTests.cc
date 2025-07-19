
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


#include "dbOASISReader.h"
#include "dbOASISWriter.h"
#include "dbTextWriter.h"
#include "dbTestSupport.h"
#include "tlLog.h"
#include "tlUnitTest.h"
#include "tlStream.h"
#include "tlFileUtils.h"

#include <stdlib.h>

void
compare_ref (tl::TestBase *_this, const char *test, const db::Layout &layout)
{
  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  std::string oss = os.string ();

  std::string fn_au (tl::testdata ());
  fn_au += "/oasis/t";
  fn_au += test;
  fn_au += "_au.txt";

  std::string au;
  try {
    tl::InputFile is (fn_au);
    tl::InputStream istream (is);
    au = istream.read_all ();
  } catch (...) {
    //  ignore read errors on au files -> this way they can be updated easily
  }

  //  Normalize the golden data's CRLF line breaks on Windows
  au = tl::replaced (au, "\r\n", "\n");

  if (au != oss) {

    EXPECT_EQ (oss, au)

    std::string fn (_this->tmp_file (std::string ("t") + test + "_au.txt"));
    {
      tl::OutputFile ofs (fn);
      tl::OutputStream ofstream (ofs);
      ofstream.put (oss);
    }

    tl::info << "To update golden data use";
    tl::info << "  cp " << fn << " " << tl::absolute_file_path (fn_au);
#if 0 //  auto update
    system ((std::string ("cp ") + fn + " " + tl::absolute_file_path (fn_au)).c_str ());
    tl::info << "Golden data updated.";
#endif

  }
}

void
run_test (tl::TestBase *_this, const char *test)
{
  db::Manager m (false);
  db::Layout layout (&m);
  std::string fn (tl::testdata ());
  fn += "/oasis/t";
  fn += test;
  fn += ".oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);

  bool error = false;
  try {
    reader.read (layout);
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, false)

  compare_ref (_this, test, layout);
}

void
run_test_error (tl::TestBase *_this, const char *test, const char *msg_au)
{
  db::Manager m (false);
  db::Layout layout (&m);
  std::string fn (tl::testdata ());
  fn += "/oasis/t";
  fn += test;
  fn += ".oas";
  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.set_warnings_as_errors (true);

  std::string msg;

  bool error = false;
  try {
    reader.read (layout);
  } catch (tl::Exception &ex) {
    msg = ex.msg ();
    error = true;
  }
  EXPECT_EQ (error, true)
  EXPECT_EQ (msg.find (msg_au), size_t (0));
}

TEST(1_1)
{
  run_test (_this, "1.1");
}

TEST(1_2)
{
  run_test (_this, "1.2");
}

TEST(1_3)
{
  run_test (_this, "1.3");
}

TEST(1_4)
{
  run_test (_this, "1.4");
}

TEST(1_5)
{
  run_test (_this, "1.5");
}

TEST(10_1)
{
  run_test (_this, "10.1");
}

TEST(11_1)
{
  run_test (_this, "11.1");
}

TEST(11_2)
{
  run_test (_this, "11.2");
}

TEST(11_3)
{
  run_test (_this, "11.3");
}

TEST(11_4)
{
  run_test (_this, "11.4");
}

TEST(11_5)
{
  run_test (_this, "11.5");
}

TEST(11_6)
{
  run_test (_this, "11.6");
}

TEST(11_7)
{
  run_test (_this, "11.7");
}

TEST(11_8)
{
  run_test_error (_this, "11.8", "Modal variable accessed before being defined: last-value-list (position=96, cell=)");
}

TEST(11_9)
{
  run_test_error (_this, "11.9", "Modal variable accessed before being defined: last-value-list (position=118, cell=)");
}

TEST(12_1)
{
  run_test (_this, "12.1");
}

TEST(13_1)
{
  run_test (_this, "13.1");
}

TEST(13_2)
{
  run_test (_this, "13.2");
}

TEST(13_3)
{
  run_test (_this, "13.3");
}

TEST(13_4)
{
  run_test (_this, "13.4");
}

TEST(14_1)
{
  run_test (_this, "14.1");
}

TEST(2_1)
{
  run_test (_this, "2.1");
}

TEST(2_2)
{
  run_test (_this, "2.2");
}

TEST(2_3)
{
  run_test_error (_this, "2.3", "Explicit and implicit CELLNAME modes cannot be mixed (position=45, cell=)");
}

TEST(2_4)
{
  run_test (_this, "2.4");
}

TEST(2_5)
{
  run_test_error (_this, "2.5", "No cellname defined for cell name id 2 (position=305, cell=)");
}

TEST(2_6)
{
  run_test (_this, "2.6");
}

TEST(2_7)
{
  run_test (_this, "2.7");
}

TEST(3_1)
{
  run_test (_this, "3.1");
}

TEST(3_10)
{
  run_test (_this, "3.10");
}

TEST(3_11)
{
  run_test_error (_this, "3.11", "Modal variable accessed before being defined: text-string (position=50, cell=ABC)");
}

TEST(3_12)
{
  run_test (_this, "3.12");
}

TEST(3_2)
{
  run_test (_this, "3.2");
}

TEST(3_3)
{
  run_test_error (_this, "3.3", "Explicit and implicit TEXTSTRING modes cannot be mixed (position=41, cell=)");
}

TEST(3_4)
{
  run_test_error (_this, "3.4", "No text string defined for text string id 2 (position=309, cell=)");
}

TEST(3_5)
{
  run_test (_this, "3.5");
}

TEST(3_6)
{
  run_test_error (_this, "3.6", "Modal variable accessed before being defined: repetition (position=52, cell=ABC)");
}

TEST(3_7)
{
  run_test_error (_this, "3.7", "Modal variable accessed before being defined: textlayer (position=50, cell=ABC)");
}

TEST(3_8)
{
  run_test_error (_this, "3.8", "Modal variable accessed before being defined: texttype (position=50, cell=ABC)");
}

TEST(3_9)
{
  run_test (_this, "3.9");
}

TEST(4_1)
{
  run_test (_this, "4.1");
}

TEST(4_2)
{
  run_test (_this, "4.2");
}

TEST(5_1)
{
  run_test (_this, "5.1");
}

TEST(5_2)
{
  run_test (_this, "5.2");
}

TEST(5_3)
{
  run_test (_this, "5.3");
}

TEST(6_1)
{
  run_test (_this, "6.1");
}

TEST(7_1)
{
  run_test (_this, "7.1");
}

TEST(8_1)
{
  run_test (_this, "8.1");
}

TEST(8_2)
{
  run_test (_this, "8.2");
}

TEST(8_3)
{
  run_test (_this, "8.3");
}

TEST(8_4)
{
  run_test (_this, "8.4");
}

TEST(8_5)
{
  run_test (_this, "8.5");
}

TEST(8_6)
{
  run_test (_this, "8.6");
}

TEST(8_7)
{
  run_test (_this, "8.7");
}

TEST(8_8)
{
  run_test (_this, "8.8");
}

TEST(9_1)
{
  run_test (_this, "9.1");
}

TEST(9_2)
{
  run_test (_this, "9.2");
}

//  Tests add-on reading 
TEST(99)
{
  const char *expected = 
    "begin_lib 0.001\n"
    "begin_cell {A}\n"
    "boundary 1 2 {-100 200} {-100 400} {100 200} {-100 200}\n"
    "boundary 1 2 {-100 600} {-100 800} {100 600} {-100 600}\n"
    "box 1 2 {300 -400} {400 -200}\n"
    "end_cell\n"
    "begin_cell {TOP}\n"
    "sref {A} 0 0 1 {-300 1200}\n"
    "sref {A} 0 0 1 {-300 400}\n"
    "sref {A} 0 0 1 {-300 800}\n"
    "sref {A} 0 0 1 {0 1200}\n"
    "sref {A} 0 1 1 {700 400}\n"
    "sref {A} 90 0 1 {700 1400}\n"
    "sref {A} 90 1 1 {700 2400}\n"
    "end_cell\n"
    "begin_cell {B}\n"
    "boundary 1 2 {-100 200} {100 400} {300 200} {-100 200}\n"
    "boundary 1 2 {-100 600} {100 800} {300 600} {-100 600}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m (false);
  db::Layout layout (&m);

  {
    std::string fn (tl::testdata ());
    fn += "/oasis/t9.2.oas";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.set_warnings_as_errors (true);
    
    bool error = false;
    try {
      db::LayerMap map = reader.read (layout);
    } catch (tl::Exception &ex) {
      tl::error << ex.msg ();
      error = true;
    }
    EXPECT_EQ (error, false)
  }

  {
    std::string fn (tl::testdata ());
    fn += "/oasis/t8.7.oas";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.set_warnings_as_errors (true);
    
    bool error = false;
    try {
      db::LayerMap map = reader.read (layout);
    } catch (tl::Exception &ex) {
      tl::error << ex.msg ();
      error = true;
    }
    EXPECT_EQ (error, false)
  }

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

//  XGEOMTRY tests (#773)
TEST(100)
{
  const char *expected =
    "begin_lib 0.0001\n"
    "begin_cell {mask}\n"
    "boundary 1 0 {17922 6288} {17922 6510} {18150 6510} {18150 6288} {17922 6288}\n"
    "boundary 1 0 {18157 647} {18157 676} {21630 676} {21630 647} {18157 647}\n"
    "boundary 1 0 {21956 0} {21956 89} {22047 89} {22047 0} {21956 0}\n"
    "boundary 2 3 {21642 3613} {21642 4005} {19409 4005} {19409 6980} {21812 6980} {21812 4958} {21942 4958} {21942 4005} {21812 4005} {21812 3613} {21642 3613}\n"
    "boundary 2 4 {21642 4005} {21642 4958} {21812 4958} {21812 4005} {21642 4005}\n"
    "boundary 3 0 {15392 1744} {15392 1774} {15672 1774} {15672 1744} {15392 1744}\n"
    "boundary 4 0 {10772 1658} {10772 1744} {14510 1744} {14510 1658} {10772 1658}\n"
    "boundary 4 0 {14510 1658} {14510 1744} {15672 1744} {15672 1658} {14510 1658}\n"
    "boundary 4 0 {18157 647} {18157 676} {21642 676} {21642 647} {18157 647}\n"
    "boundary 5 1 {15550 1658} {15550 1673} {15570 1673} {15570 1658} {15550 1658}\n"
    "boundary 5 1 {15661 1657} {15641 1659} {15642 1671} {15662 1669} {15661 1657}\n"
    "boundary 5 1 {18150 7440} {18150 7460} {18162 7460} {18162 7440} {18150 7440}\n"
    "boundary 5 1 {18150 8488} {18150 8508} {18162 8508} {18162 8488} {18150 8488}\n"
    "boundary 5 1 {18150 9480} {18150 9500} {18162 9500} {18162 9480} {18150 9480}\n"
    "boundary 5 1 {18670 3411} {18670 3468} {18690 3468} {18690 3411} {18670 3411}\n"
    "boundary 5 1 {19470 3411} {19470 3468} {19490 3468} {19490 3411} {19470 3411}\n"
    "boundary 5 1 {20217 3411} {20217 3468} {20237 3468} {20237 3411} {20217 3411}\n"
    "boundary 5 1 {21630 2048} {21630 2068} {21642 2068} {21642 2048} {21630 2048}\n"
    "boundary 5 1 {21630 2293} {21630 2313} {21642 2313} {21642 2293} {21630 2293}\n"
    "boundary 5 1 {21930 9308} {21930 9328} {21942 9328} {21942 9308} {21930 9308}\n"
    "boundary 5 1 {21930 9600} {21930 9620} {21942 9620} {21942 9600} {21930 9600}\n"
    "boundary 5 1 {23570 6128} {23570 6148} {23582 6148} {23582 6128} {23570 6128}\n"
    "boundary 5 1 {23570 6147} {23570 6167} {23582 6167} {23582 6147} {23570 6147}\n"
    "boundary 5 1 {25710 1978} {25710 1998} {25722 1998} {25722 1978} {25710 1978}\n"
    "boundary 5 1 {25710 2800} {25710 2820} {25722 2820} {25722 2800} {25710 2800}\n"
    "boundary 5 2 {18074 6408} {17971 6486} {17983 6502} {18086 6424} {18074 6408}\n"
    "boundary 6 0 {6743 2449} {6743 4230} {9061 4230} {9061 2449} {6743 2449}\n"
    "boundary 7 1 {13237 5356} {13210 5490} {13192 5530} {13170 5563} {13130 5586} {13090 5583} {13070 5570} {13050 5551} {13037 5530} {13021 5490} {12988 5378} {12938 5390} {12963 5530} {12977 5570} {12998 5610} {13034 5650} {13051 5663} {13090 5678} {13130 5679} {13171 5667} {13210 5638} {13232 5611} {13253 5570} {13274 5490} {13291 5365} {13237 5356}\n"
    "boundary 8 0 {21680 4106} {21640 4107} {21600 4118} {21574 4130} {21560 4138} {21520 4163} {21509 4170} {21480 4194} {21458 4210} {21440 4227} {21411 4250} {21400 4262} {21366 4290} {21360 4298} {21324 4330} {21320 4335} {21282 4370} {21280 4373} {21241 4410} {21240 4411} {21200 4450} {21160 4490} {21159 4490} {21039 4610} {21000 4650} {20960 4690} {20960 4691} {20921 4730} {20920 4732} {20896 4770} {20886 4810} {20882 4850} {20880 4930} {20880 5330} {20920 5370} {20960 5370} {21000 5340} {21013 5330} {21040 5325} {21080 5309} {21120 5291} {21121 5290} {21160 5276} {21200 5258} {21210 5250} {21240 5240} {21280 5222} {21295 5210} {21320 5202} {21360 5181} {21374 5170} {21400 5160} {21440 5136} {21447 5130} {21480 5112} {21510 5090} {21520 5086} {21560 5058} {21568 5050} {21600 5027} {21617 5010} {21640 4993} {21662 4970} {21680 4955} {21701 4930} {21720 4910} {21735 4890} {21760 4856} {21764 4850} {21786 4810} {21800 4781} {21805 4770} {21818 4730} {21828 4690} {21836 4650} {21840 4616} {21841 4610} {21845 4530} {21845 4450} {21844 4410} {21841 4370} {21840 4358} {21836 4330} {21829 4290} {21818 4250} {21803 4210} {21800 4205} {21778 4170} {21760 4148} {21738 4130} {21720 4118} {21680 4106}\n"
    "end_cell\n"
    "end_lib\n"
  ;

  db::Manager m (false);
  db::Layout layout (&m);

  {
    std::string fn (tl::testdata ());
    fn += "/oasis/xgeometry_test.oas";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.set_warnings_as_errors (true);
    
    reader.read (layout);
  }

  tl::OutputStringStream os;
  tl::OutputStream ostream (os);
  db::TextWriter writer (ostream);
  writer.write (layout);
  EXPECT_EQ (std::string (os.string ()), std::string (expected))
}

//  Empty layers through LAYERMAP
TEST(101)
{
  db::Layout ly;
  ly.add_cell ("TOP");
  ly.insert_layer (db::LayerProperties (1, 0, "A"));
  ly.insert_layer (db::LayerProperties (2, 0, ""));
  ly.insert_layer (db::LayerProperties (3, 0, "C"));

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_OASISReader101.oas");

  {
    tl::OutputStream stream (tmp_file);
    db::OASISWriter writer;
    db::SaveLayoutOptions options;
    writer.write (ly, stream, options);
  }

  db::Layout ly_new;

  {
    tl::InputStream stream (tmp_file);
    db::Reader reader (stream);
    reader.read (ly_new);
  }

  //  NOTE: only named layers are written into layer table
  EXPECT_EQ (ly_new.cell_by_name ("TOP").first, true);
  EXPECT_EQ (int (ly_new.layers ()), 2);
  if (int (ly_new.layers ()) == 2) {
    EXPECT_EQ (ly_new.get_properties (0).to_string (), "A (1/0)");
    EXPECT_EQ (ly_new.get_properties (1).to_string (), "C (3/0)");
  }
}

TEST(Bug_121_1)
{
  db::Manager m (false);
  db::Layout layout (&m);

  {
    tl::InputStream file (tl::testdata () + "/oasis/bug_121a.oas");
    db::OASISReader reader (file);
    reader.read (layout);
  }

  {
    tl::InputStream file (tl::testdata () + "/oasis/bug_121b.oas");
    db::OASISReader reader (file);
    reader.read (layout);
  }

  std::string fn_au (tl::testdata () + "/oasis/bug_121_au1.gds");
  db::compare_layouts (_this, layout, fn_au, db::WriteGDS2, 1);
}

TEST(Bug_121_2)
{
  db::Manager m (false);
  db::Layout layout (&m);

  {
    tl::InputStream file (tl::testdata () + "/oasis/bug_121a.oas");
    db::OASISReader reader (file);
    reader.read (layout);
  }

  {
    tl::InputStream file (tl::testdata () + "/oasis/bug_121c.oas");
    db::OASISReader reader (file);
    reader.read (layout);
  }

  std::string fn_au (tl::testdata () + "/oasis/bug_121_au2.gds");
  db::compare_layouts (_this, layout, fn_au, db::WriteGDS2, 1);
}

TEST(Bug_1474)
{
  db::Manager m (false);
  db::Layout layout (&m);

  try {
    tl::InputStream file (tl::testdata_private () + "/oasis/issue_1474.oas");
    db::OASISReader reader (file);
    reader.read (layout);
    EXPECT_EQ (false, true);
  } catch (tl::CancelException &ex) {
    //  Seen when private test data is not installed
    throw;
  } catch (tl::Exception &ex) {
    EXPECT_EQ (ex.msg ().find ("Cell named ADDHX2 with ID 4 was already given name SEDFFTRX2 (position=763169, cell=)"), size_t (0));
  }
}

TEST(Bug_1799)
{
  db::Manager m (false);
  db::Layout layout (&m);

  {
    tl::InputStream file (tl::testdata () + "/oasis/issue_1799.oas");
    db::OASISReader reader (file);
    reader.read (layout);
  }

  db::properties_id_type pn = db::property_names_id (tl::Variant (1));
  db::PropertiesSet ps;
  ps.insert (pn, tl::Variant ("hello, world!"));

  auto pid = db::properties_id (ps);

  auto ps2 = db::properties (pid);
  EXPECT_EQ (ps2.size (), size_t (1));
  EXPECT_EQ (ps2.has_value (pn), true);
  EXPECT_EQ (ps2.value (pn).to_string (), "hello, world!");
}

//  Modified in #2088 to give a warning
TEST(DuplicateCellname)
{
  db::Manager m (false);
  db::Layout layout (&m);

  tl::InputStream file (tl::testdata () + "/oasis/duplicate_cellname.oas");
  db::OASISReader reader (file);
  reader.read (layout);

  std::string fn_au (tl::testdata () + "/oasis/duplicate_cellname_au.oas");
  db::compare_layouts (_this, layout, fn_au, db::NoNormalization, 1);
}

TEST(BlendCrash)
{
  db::Manager m (false);
  db::Layout layout (&m);

  {
    tl::InputStream file (tl::testdata () + "/oasis/blend_crash1.oas");
    db::OASISReader reader (file);
    reader.read (layout);
  }

  {
    tl::InputStream file (tl::testdata () + "/oasis/blend_crash2.oas");
    db::OASISReader reader (file);
    reader.read (layout);
  }

  std::string fn_au (tl::testdata () + "/oasis/blend_crash_au.gds.gz");
  db::compare_layouts (_this, layout, fn_au, db::WriteGDS2, 1);
}
