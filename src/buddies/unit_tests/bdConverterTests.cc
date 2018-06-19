
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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

#include "bdConverterMain.h"
#include "dbStream.h"
#include "dbCIFFormat.h"
#include "dbDXFReader.h"
#include "dbOASISReader.h"
#include "dbGDS2Reader.h"
#include "dbTestSupport.h"
#include "contrib/dbGDS2TextReader.h"
#include "tlUnitTest.h"

//  Testing the converter main implementation (CIF)
TEST(1)
{
  std::string input = tl::testsrc ();
  input += "/testdata/gds/t10.gds";

  std::string output = this->tmp_file ();

  const char *argv[] = { "x", input.c_str (), output.c_str () };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, db::CIFReaderOptions ().format_name ()), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    options.set_options (new db::CIFReaderOptions ());
    db::Reader reader (stream);
    reader.read (layout, options);
    EXPECT_EQ (reader.format (), "CIF");
  }

  db::compare_layouts (this, layout, input, db::NoNormalization);
}

//  Testing the converter main implementation (DXF)
TEST(2)
{
  std::string input = tl::testsrc ();
  input += "/testdata/gds/t10.gds";

  std::string output = this->tmp_file ();

  const char *argv[] = { "x", input.c_str (), output.c_str () };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, db::DXFReaderOptions ().format_name ()), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    options.set_options (new db::DXFReaderOptions ());
    db::Reader reader (stream);
    reader.read (layout, options);
    EXPECT_EQ (reader.format (), "DXF");
  }

  //  Fix top cell name (which is TOP in DXF, not RINGO as in reference)
  std::pair<bool, db::cell_index_type> top = layout.cell_by_name ("TOP");
  EXPECT_EQ (top.first, true);
  layout.rename_cell (top.second, "RINGO");

  //  Use GDS2 normalization to solve the box vs. polygon issue
  db::compare_layouts (this, layout, input, db::WriteGDS2);
}

//  Testing the converter main implementation (GDS2)
TEST(3)
{
  std::string input = tl::testsrc ();
  input += "/testdata/gds/t10.gds";

  std::string output = this->tmp_file ();

  const char *argv[] = { "x", input.c_str (), output.c_str () };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, db::GDS2ReaderOptions ().format_name ()), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    options.set_options (new db::GDS2ReaderOptions ());
    db::Reader reader (stream);
    reader.read (layout, options);
    EXPECT_EQ (reader.format (), "GDS2");
  }

  db::compare_layouts (this, layout, input, db::NoNormalization);
}

//  Testing the converter main implementation (GDS2Text)
TEST(4)
{
  std::string input = tl::testsrc ();
  input += "/testdata/gds/t10.gds";

  std::string output = this->tmp_file ();

  const char *argv[] = { "x", input.c_str (), output.c_str () };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, db::GDS2ReaderOptions ().format_name () + "Text"), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    options.set_options (new db::GDS2ReaderOptions ());
    db::Reader reader (stream);
    reader.read (layout, options);
    EXPECT_EQ (reader.format (), "GDS2Text");
  }

  db::compare_layouts (this, layout, input, db::NoNormalization);
}

//  Testing the converter main implementation (OASIS)
TEST(5)
{
  std::string input = tl::testsrc ();
  input += "/testdata/gds/t10.gds";

  std::string output = this->tmp_file ();

  const char *argv[] = { "x", input.c_str (), output.c_str () };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, db::OASISReaderOptions ().format_name ()), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    options.set_options (new db::OASISReaderOptions ());
    db::Reader reader (stream);
    reader.read (layout, options);
    EXPECT_EQ (reader.format (), "OASIS");
  }

  db::compare_layouts (this, layout, input, db::NoNormalization);
}

