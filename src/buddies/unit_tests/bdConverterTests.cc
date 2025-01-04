
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

#include "bdConverterMain.h"
#include "bdWriterOptions.h"
#include "dbStream.h"
#include "dbTestSupport.h"
#include "dbReader.h"
#include "tlUnitTest.h"

//  Testing the converter main implementation (CIF)
TEST(1)
{
  std::string input = tl::testdata ();
  input += "/gds/t10.gds";

  std::string output = this->tmp_file ();

  const char *argv[] = { "x", input.c_str (), output.c_str () };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::cif_format_name), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    db::Reader reader (stream);
    reader.read (layout, options);
    EXPECT_EQ (reader.format (), "CIF");
  }

  db::compare_layouts (this, layout, input, db::NoNormalization);
}

//  Testing the converter main implementation (DXF)
TEST(2)
{
  std::string input = tl::testdata ();
  input += "/gds/t10.gds";

  std::string output = this->tmp_file ();

  const char *argv[] = { "x", input.c_str (), output.c_str () };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::dxf_format_name), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
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
  std::string input = tl::testdata ();
  input += "/gds/t10.gds";

  std::string output = this->tmp_file ();

  const char *argv[] = { "x", input.c_str (), output.c_str () };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::gds2_format_name), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    db::Reader reader (stream);
    reader.read (layout, options);
    EXPECT_EQ (reader.format (), "GDS2");
  }

  db::compare_layouts (this, layout, input, db::NoNormalization);
}

//  Testing the converter main implementation (GDS2Text)
TEST(4)
{
  std::string input = tl::testdata ();
  input += "/gds/t10.gds";

  std::string output = this->tmp_file ();

  const char *argv[] = { "x", input.c_str (), output.c_str () };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::gds2text_format_name), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    db::Reader reader (stream);
    reader.read (layout, options);
    EXPECT_EQ (reader.format (), "GDS2Text");
  }

  db::compare_layouts (this, layout, input, db::NoNormalization);
}

//  Testing the converter main implementation (OASIS)
TEST(5)
{
  std::string input = tl::testdata ();
  input += "/gds/t10.gds";

  std::string output = this->tmp_file ();

  const char *argv[] = { "x", input.c_str (), output.c_str () };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::oasis_format_name), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    db::Reader reader (stream);
    reader.read (layout, options);
    EXPECT_EQ (reader.format (), "OASIS");
  }

  db::compare_layouts (this, layout, input, db::NoNormalization);
}

//  Testing the converter main implementation (MAG)
TEST(6)
{
  std::string input = tl::testdata ();
  input += "/gds/t10.gds";

  std::string input_au = tl::testdata ();
  input_au += "/magic/strm2mag_au.gds";

  std::string output = this->tmp_file ("RINGO.mag");

  const char *argv[] = { "x", input.c_str (), output.c_str (), "--magic-lambda-out=0.005" };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), (char **) argv, bd::GenericWriterOptions::mag_format_name), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::LoadLayoutOptions options;
    options.set_option_by_name ("mag_lambda", 0.005);
    db::Reader reader (stream);
    reader.read (layout, options);
    EXPECT_EQ (reader.format (), "MAG");
  }

  db::compare_layouts (this, layout, input_au, db::WriteGDS2);
}
