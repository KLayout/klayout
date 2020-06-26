
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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

#include "tlStream.h"
#include "tlUnitTest.h"
#include "tlFileUtils.h"

//  Secret mode switchers for testing
namespace tl
{
TL_PUBLIC void file_utils_force_windows ();
TL_PUBLIC void file_utils_force_linux ();
TL_PUBLIC void file_utils_force_reset ();
}

TEST(InputPipe1)
{
  tl::InputPipe pipe ("echo HELLOWORLD");
  tl::InputStream str (pipe);
  tl::TextInputStream tstr (str);
  EXPECT_EQ (tstr.get_line (), "HELLOWORLD");
  EXPECT_EQ (pipe.wait (), 0);
}

TEST(InputPipe2)
{
  tl::InputPipe pipe ("thiscommanddoesnotexistithink 2>&1");
  tl::InputStream str (pipe);
  tl::TextInputStream tstr (str);
  tstr.get_line ();
  int ret = pipe.wait ();
  tl::info << "Process exit code: " << ret;
  EXPECT_NE (ret, 0);
}

TEST(OutputPipe1)
{
  std::string tf = tmp_file ("pipe_out");

  {
    tl::OutputPipe pipe ("cat >" + tf);
    tl::OutputStream str (pipe);
    str << "Hello, world!";
  }

  {
    tl::InputStream is (tf);
    std::string s = is.read_all ();
    EXPECT_EQ (s, "Hello, world!");
  }
}

TEST(TextOutputStream)
{
  std::string fn = tmp_file ("test.txt");

  {
    tl::OutputStream os (fn, tl::OutputStream::OM_Auto, false);
    os << "Hello, world!\nWith another line\n\r\r\nseparated by a LFCR and CRLF.";
  }

  {
    tl::InputStream is (fn);
    std::string s = is.read_all ();
    EXPECT_EQ (s, "Hello, world!\nWith another line\n\r\r\nseparated by a LFCR and CRLF.");
  }

  try {

    tl::file_utils_force_linux ();

    {
      tl::OutputStream os (fn, tl::OutputStream::OM_Auto, true);
      os << "Hello, world!\nWith another line\n\r\r\nseparated by a LFCR and CRLF.";
    }

    tl::InputStream is (fn);
    std::string s = is.read_all ();

    EXPECT_EQ (s, "Hello, world!\nWith another line\n\nseparated by a LFCR and CRLF.");
    tl::file_utils_force_reset ();

  } catch (...) {
    tl::file_utils_force_reset ();
    throw;
  }

  try {

    tl::file_utils_force_windows ();

    {
      tl::OutputStream os (fn, tl::OutputStream::OM_Auto, true);
      os << "Hello, world!\nWith another line\n\r\r\nseparated by a LFCR and CRLF.";
    }

    tl::InputStream is (fn);
    std::string s = is.read_all ();

    EXPECT_EQ (s, "Hello, world!\r\nWith another line\r\n\r\nseparated by a LFCR and CRLF.");
    tl::file_utils_force_reset ();

  } catch (...) {
    tl::file_utils_force_reset ();
    throw;
  }
}

TEST(TextInputStream)
{
  std::string fn = tmp_file ("test.txt");

  {
    tl::OutputStream os (fn, tl::OutputStream::OM_Auto, false);
    os << "Hello, world!\nWith another line\n\r\r\nseparated by a LFCR and CRLF.";
  }

  {
    tl::InputStream is (fn);
    tl::TextInputStream tis (is);
    EXPECT_EQ (tis.get_line (), "Hello, world!");
    EXPECT_EQ (tis.line_number (), 1);
    EXPECT_EQ (tis.get_line (), "With another line");
    EXPECT_EQ (tis.line_number (), 2);
    EXPECT_EQ (tis.peek_char (), '\n');
    EXPECT_EQ (tis.get_line (), "");
    EXPECT_EQ (tis.line_number (), 3);
    EXPECT_EQ (tis.peek_char (), 's');
    EXPECT_EQ (tis.get_line (), "separated by a LFCR and CRLF.");
    EXPECT_EQ (tis.line_number (), 4);
    EXPECT_EQ (tis.at_end (), true);
  }

  {
    tl::InputStream is (fn);
    tl::TextInputStream tis (is);
    EXPECT_EQ (tis.read_all (5), "Hello");
  }

  {
    tl::InputStream is (fn);
    tl::TextInputStream tis (is);
    EXPECT_EQ (tis.read_all (), "Hello, world!\nWith another line\n\nseparated by a LFCR and CRLF.");
  }
}
