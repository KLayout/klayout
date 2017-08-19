
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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

#include "utHead.h"
#include "bdConverterMain.h"
#include "dbCIFReader.h"
#include "dbDXFReader.h"
#include "dbOASISReader.h"
#include "dbGDS2Reader.h"
#include "contrib/dbGDS2TextReader.h"

//  Testing the converter main implementation (CIF)
TEST(1)
{
  std::string input = ut::testsrc ();
  input += "/testdata/gds/t10.gds";

  std::string output = this->tmp_file ();

  char *argv[] = { "x", const_cast<char *> (input.c_str ()), const_cast<char *> (output.c_str ()) };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), argv, db::CIFReaderOptions ().format_name ()), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::CIFReader reader (stream);
    reader.read (layout, db::LoadLayoutOptions ());
  }

  this->compare_layouts (layout, input, ut::NoNormalization);
}

//  Testing the converter main implementation (DXF)
TEST(2)
{
  std::string input = ut::testsrc ();
  input += "/testdata/gds/t10.gds";

  std::string output = this->tmp_file ();

  char *argv[] = { "x", const_cast<char *> (input.c_str ()), const_cast<char *> (output.c_str ()) };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), argv, db::DXFReaderOptions ().format_name ()), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::DXFReader reader (stream);
    reader.read (layout, db::LoadLayoutOptions ());
  }

  //  Fix top cell name (which is TOP in DXF, not RINGO as in reference)
  std::pair<bool, db::cell_index_type> top = layout.cell_by_name ("TOP");
  EXPECT_EQ (top.first, true);
  layout.rename_cell (top.second, "RINGO");

  //  Use GDS2 normalization to solve the box vs. polygon issue
  this->compare_layouts (layout, input, ut::WriteGDS2);
}

//  Testing the converter main implementation (GDS2)
TEST(3)
{
  std::string input = ut::testsrc ();
  input += "/testdata/gds/t10.gds";

  std::string output = this->tmp_file ();

  char *argv[] = { "x", const_cast<char *> (input.c_str ()), const_cast<char *> (output.c_str ()) };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), argv, db::GDS2ReaderOptions ().format_name ()), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::GDS2Reader reader (stream);
    reader.read (layout, db::LoadLayoutOptions ());
  }

  this->compare_layouts (layout, input, ut::NoNormalization);
}

//  Testing the converter main implementation (GDS2Text)
TEST(4)
{
  std::string input = ut::testsrc ();
  input += "/testdata/gds/t10.gds";

  std::string output = this->tmp_file ();

  char *argv[] = { "x", const_cast<char *> (input.c_str ()), const_cast<char *> (output.c_str ()) };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), argv, db::GDS2ReaderOptions ().format_name () + "Text"), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::GDS2ReaderText reader (stream);
    reader.read (layout, db::LoadLayoutOptions ());
  }

  this->compare_layouts (layout, input, ut::NoNormalization);
}

//  Testing the converter main implementation (OASIS)
TEST(5)
{
  std::string input = ut::testsrc ();
  input += "/testdata/gds/t10.gds";

  std::string output = this->tmp_file ();

  char *argv[] = { "x", const_cast<char *> (input.c_str ()), const_cast<char *> (output.c_str ()) };

  EXPECT_EQ (bd::converter_main (sizeof (argv) / sizeof (argv[0]), argv, db::OASISReaderOptions ().format_name ()), 0);

  db::Layout layout;

  {
    tl::InputStream stream (output);
    db::OASISReader reader (stream);
    reader.read (layout, db::LoadLayoutOptions ());
  }

  this->compare_layouts (layout, input, ut::NoNormalization);
}

