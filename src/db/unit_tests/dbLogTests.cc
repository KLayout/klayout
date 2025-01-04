
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

#include "dbLog.h"

#include "tlUnitTest.h"

TEST(1_Basic)
{
  db::LogEntryData data;
  EXPECT_EQ (data.severity (), db::NoSeverity);
  EXPECT_EQ (data.message (), std::string ());
  EXPECT_EQ (data.category_description (), std::string ());
  EXPECT_EQ (data.category_name (), std::string ());
  EXPECT_EQ (data.cell_name (), std::string ());
  EXPECT_EQ (data.geometry ().to_string (), "()");

  EXPECT_EQ (data == db::LogEntryData (), true);
  EXPECT_EQ (data != db::LogEntryData (), false);
}

TEST(2_Attributes)
{
  db::LogEntryData data;
  data.set_severity (db::Error);
  data.set_message ("Message");
  data.set_category_name ("42");
  data.set_cell_name ("cell");
  data.set_category_description ("the answer");
  data.set_geometry (db::DPolygon (db::DBox (db::DPoint (1, 2), db::DPoint (3, 4))));

  db::LogEntryData data2 = data;

  EXPECT_EQ (data == db::LogEntryData (), false);
  EXPECT_EQ (data != db::LogEntryData (), true);
  EXPECT_EQ (data == data2, true);
  EXPECT_EQ (data != data2, false);

  EXPECT_EQ (data.severity (), db::Error);
  EXPECT_EQ (data.message (), std::string ("Message"));
  EXPECT_EQ (data.category_description (), std::string ("the answer"));
  EXPECT_EQ (data.category_name (), std::string ("42"));
  EXPECT_EQ (data.cell_name (), std::string ("cell"));
  EXPECT_EQ (data.geometry ().to_string (), "(1,2;1,4;3,4;3,2)");
}

TEST(3_toString)
{
  db::LogEntryData data;
  data.set_severity (db::Error);
  data.set_message ("Message");
  data.set_category_name ("42");
  data.set_cell_name ("cell");
  data.set_category_description ("the answer");
  data.set_geometry (db::DPolygon (db::DBox (db::DPoint (1, 2), db::DPoint (3, 4))));

  EXPECT_EQ (data.to_string (), std::string ("[the answer] In cell cell: Message, shape: (1,2;1,4;3,4;3,2)"));

  data.set_category_description (std::string ());

  EXPECT_EQ (data.to_string (), std::string ("[42] In cell cell: Message, shape: (1,2;1,4;3,4;3,2)"));

  data.set_category_name (std::string ());

  EXPECT_EQ (data.to_string (), std::string ("In cell cell: Message, shape: (1,2;1,4;3,4;3,2)"));

  data.set_geometry (db::DPolygon ());

  EXPECT_EQ (data.to_string (), std::string ("In cell cell: Message"));

  data.set_cell_name (std::string ());

  EXPECT_EQ (data.to_string (), std::string ("Message"));
}
