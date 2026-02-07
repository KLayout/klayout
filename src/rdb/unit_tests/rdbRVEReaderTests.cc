
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#include "rdb.h"
#include "rdbReader.h"
#include "tlUnitTest.h"
#include "tlFileUtils.h"
#include "tlLog.h"

void run_rve_test (tl::TestBase *_this, const std::string &fn_rve, const std::string &fn_au)
{
  rdb::Database db;

  {
    tl::InputFile input (tl::testdata_private () + "/rve/" + fn_rve);
    tl::InputStream is (input);
    rdb::Reader reader (is);
    reader.read (db);
  }

  std::string tmp = _this->tmp_file ();
  db.save (tmp);

  std::string au_path = tl::absolute_file_path (tl::testdata_private () + "/rve/" + fn_au);

  std::string txt, au_txt;

  try {
    tl::InputFile input (au_path);
    tl::InputStream is (input);
    tl::TextInputStream ts (is);
    au_txt = ts.read_all ();
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
  }

  {
    tl::InputFile input (tmp);
    tl::InputStream is (input);
    tl::TextInputStream ts (is);
    txt = ts.read_all ();
  }

  if (au_txt != txt) {
    tl::error << "Golden and actual data differs:";
    tl::error << "  cp " << tmp << " " << au_path;
  }
  EXPECT_EQ (au_txt == txt, true);
}

TEST(1)
{
  run_rve_test (_this, "rve1.db", "rve1_au_2.txt");
}

TEST(2)
{
  run_rve_test (_this, "rve2.db", "rve2_au_2.txt");
}

TEST(3)
{
  run_rve_test (_this, "rve3.db", "rve3_au_2.txt");
}

TEST(4)
{
  run_rve_test (_this, "rve4.db", "rve4_au.txt");
}

TEST(5)
{
  run_rve_test (_this, "rve5.db", "rve5_au.txt");
}
