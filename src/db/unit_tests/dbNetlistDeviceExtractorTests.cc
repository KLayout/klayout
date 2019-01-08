
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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

#include "dbNetlistDeviceExtractor.h"

#include "tlUnitTest.h"

TEST(1_NetlistDeviceExtractorErrorBasic)
{
  db::NetlistDeviceExtractorError error;

  EXPECT_EQ (error.message (), "");
  error.set_message ("x");
  EXPECT_EQ (error.message (), "x");
  error.set_category_name ("cat");
  EXPECT_EQ (error.category_name (), "cat");
  error.set_category_description ("cdesc");
  EXPECT_EQ (error.category_description (), "cdesc");
  error.set_cell_name ("cell");
  EXPECT_EQ (error.cell_name (), "cell");
  error.set_geometry (db::DPolygon (db::DBox (0, 1, 2, 3)));
  EXPECT_EQ (error.geometry ().to_string (), "(0,1;0,3;2,3;2,1)");

  error = db::NetlistDeviceExtractorError ("cell2", "msg2");
  EXPECT_EQ (error.cell_name (), "cell2");
  EXPECT_EQ (error.message (), "msg2");
  EXPECT_EQ (error.category_name (), "");
  EXPECT_EQ (error.category_description (), "");
  EXPECT_EQ (error.geometry ().to_string (), "()");
}

namespace {
  class DummyDeviceExtractor
    : public db::NetlistDeviceExtractor
  {
  public:
    DummyDeviceExtractor ()
      : db::NetlistDeviceExtractor (std::string ("DUMMY"))
    {
      error ("msg1");
      error ("msg2", db::DPolygon (db::DBox (0, 1, 2, 3)));
      error ("cat1", "desc1", "msg1");
      error ("cat1", "desc1", "msg3", db::DPolygon (db::DBox (10, 11, 12, 13)));
    }
  };
}

static std::string error2string (const db::NetlistDeviceExtractorError &e)
{
  return e.cell_name() + ":" + e.category_name () + ":" + e.category_description () + ":" +
         e.geometry ().to_string () + ":" + e.message ();
}

TEST(2_NetlistDeviceExtractorErrors)
{
  DummyDeviceExtractor dummy_ex;

  EXPECT_EQ (dummy_ex.has_errors (), true);

  std::vector<db::NetlistDeviceExtractorError> errors (dummy_ex.begin_errors (), dummy_ex.end_errors ());
  EXPECT_EQ (int (errors.size ()), 4);
  EXPECT_EQ (error2string (errors [0]), ":::():msg1");
  EXPECT_EQ (error2string (errors [1]), ":::(0,1;0,3;2,3;2,1):msg2");
  EXPECT_EQ (error2string (errors [2]), ":cat1:desc1:():msg1");
  EXPECT_EQ (error2string (errors [3]), ":cat1:desc1:(10,11;10,13;12,13;12,11):msg3");
}
