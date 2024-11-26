
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

TEST(InputPipe3)
{
  tl::InputStream str ("pipe:echo HELLOWORLD");
  tl::TextInputStream tstr (str);
  EXPECT_EQ (tstr.get_line (), "HELLOWORLD");
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
      tl::rm_file (fn);  //  avoids trouble with wrong path delimeters and backup files
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
      tl::rm_file (fn);  //  avoids trouble with wrong path delimeters and backup files
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
    EXPECT_EQ (tis.line_number (), size_t (1));
    EXPECT_EQ (tis.get_line (), "With another line");
    EXPECT_EQ (tis.line_number (), size_t (2));
    EXPECT_EQ (tis.peek_char (), '\n');
    EXPECT_EQ (tis.get_line (), "");
    EXPECT_EQ (tis.line_number (), size_t (3));
    EXPECT_EQ (tis.peek_char (), 's');
    EXPECT_EQ (tis.get_line (), "separated by a LFCR and CRLF.");
    EXPECT_EQ (tis.line_number (), size_t (4));
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

TEST(DataInputStream)
{
  tl::InputStream is ("data:SGVsbG8sIHdvcmxkIQpXaXRoIGFub3RoZXIgbGluZQoNDQpzZXBhcmF0ZWQgYnkgYSBMRkNSIGFuZCBDUkxGLg==");
  tl::TextInputStream tis (is);
  EXPECT_EQ (tis.get_line (), "Hello, world!");
  EXPECT_EQ (tis.line_number (), size_t (1));
  EXPECT_EQ (tis.get_line (), "With another line");
  EXPECT_EQ (tis.line_number (), size_t (2));
  EXPECT_EQ (tis.peek_char (), '\n');
  EXPECT_EQ (tis.get_line (), "");
  EXPECT_EQ (tis.line_number (), size_t (3));
  EXPECT_EQ (tis.peek_char (), 's');
  EXPECT_EQ (tis.get_line (), "separated by a LFCR and CRLF.");
  EXPECT_EQ (tis.line_number (), size_t (4));
  EXPECT_EQ (tis.at_end (), true);
}

namespace
{

class BrokenOutputStream
  : public tl::OutputFile
{
public:
  BrokenOutputStream (const std::string &path, int keep_backups)
    : tl::OutputFile (path, keep_backups)
  { }

  void write_file(const char *b, size_t n)
  {
    for (const char *p = b; p < b + n; ++p) {
      if (*p == '!') {
        throw tl::Exception ("Bang!");
      }
    }
    tl::OutputFile::write (b, n);
  }
};

}

TEST(SafeOutput)
{
  std::string tp = tmp_file ("x");

  {
    tl::OutputStream os (tp);
    os << "blabla\n";
  }

  EXPECT_EQ (tl::file_exists (tp + ".~backup"), false);
  EXPECT_EQ (tl::file_exists (tp), true);

  {
    tl::OutputStream os (tp);
    EXPECT_EQ (tl::file_exists (tp + ".~backup"), true);
    EXPECT_EQ (tl::file_exists (tp), true);
    os << "Hello, world!\n";
  }

  EXPECT_EQ (tl::file_exists (tp + ".~backup"), false);
  EXPECT_EQ (tl::file_exists (tp), true);

  {
    tl::InputStream is (tp);
    EXPECT_EQ (is.read_all (), "Hello, world!\n");
  }

  try {
    BrokenOutputStream broken (tp, 0);
    tl::OutputStream os (broken);
    EXPECT_EQ (tl::file_exists (tp + ".~backup"), true);
    EXPECT_EQ (tl::file_exists (tp), true);
    os << "Hi!\n";
    os.flush ();   //  raises the exception
    EXPECT_EQ (true, false);
  } catch (...) {
    //  '!' raises an exception
  }

  //  The original content is restored now

  EXPECT_EQ (tl::file_exists (tp + ".~backup"), false);
  EXPECT_EQ (tl::file_exists (tp), true);

  {
    tl::InputStream is (tp);
    EXPECT_EQ (is.read_all (), "Hello, world!\n");
  }


  try {
    BrokenOutputStream *broken = new BrokenOutputStream (tp, 0);
    tl::OutputStream os (broken);
    EXPECT_EQ (tl::file_exists (tp + ".~backup"), true);
    EXPECT_EQ (tl::file_exists (tp), true);
    os << "Hi!\n";
    os.flush ();   //  raises the exception
    EXPECT_EQ (true, false);
  } catch (...) {
    //  '!' raises an exception
  }

  //  The original content is restored now

  EXPECT_EQ (tl::file_exists (tp + ".~backup"), false);
  EXPECT_EQ (tl::file_exists (tp), true);

  {
    tl::InputStream is (tp);
    EXPECT_EQ (is.read_all (), "Hello, world!\n");
  }
}

TEST(SafeOutput2)
{
  std::string cd = tl::current_dir ();
  tl_assert (tl::chdir (tmp_file (".")));

  try {

    std::string tmp_path = "x";
    tl::rm_dir_recursive (tmp_path);
    tl::mkpath (tmp_path);
    std::string tp = tl::combine_path (tmp_path, "y");

    {
      tl::OutputStream os (tp);
      os << "blabla\n";
    }

    EXPECT_EQ (tl::file_exists (tp + ".~backup"), false);
    EXPECT_EQ (tl::file_exists (tp), true);

    {
      tl::OutputStream os (tp);
      EXPECT_EQ (tl::file_exists (tp + ".~backup"), true);
      EXPECT_EQ (tl::file_exists (tp), true);
      os << "Hello, world!\n";
    }

    EXPECT_EQ (tl::file_exists (tp + ".~backup"), false);
    EXPECT_EQ (tl::file_exists (tp), true);

    {
      tl::InputStream is (tp);
      EXPECT_EQ (is.read_all (), "Hello, world!\n");
    }

    tl::chdir (cd);

  } catch (...) {
    tl::chdir (cd);
    throw;
  }
}

TEST(Backups)
{
  std::string tp = tmp_file ("x");

  {
    tl::OutputStream os (tp, tl::OutputStream::OM_Auto, false, 2);
    os << "1\n";
  }

  EXPECT_EQ (tl::file_exists (tp + ".~backup"), false);
  EXPECT_EQ (tl::file_exists (tp + ".1"), false);
  EXPECT_EQ (tl::file_exists (tp + ".2"), false);
  EXPECT_EQ (tl::file_exists (tp + ".3"), false);
  EXPECT_EQ (tl::file_exists (tp), true);

  {
    tl::InputStream is (tp);
    EXPECT_EQ (is.read_all (), "1\n");
  }

  {
    tl::OutputStream os (tp, tl::OutputStream::OM_Auto, false, 2);
    EXPECT_EQ (tl::file_exists (tp + ".~backup"), true);
    EXPECT_EQ (tl::file_exists (tp), true);
    os << "2\n";
  }

  EXPECT_EQ (tl::file_exists (tp + ".~backup"), false);
  EXPECT_EQ (tl::file_exists (tp + ".1"), true);
  EXPECT_EQ (tl::file_exists (tp + ".2"), false);
  EXPECT_EQ (tl::file_exists (tp + ".3"), false);
  EXPECT_EQ (tl::file_exists (tp), true);

  {
    tl::InputStream is (tp);
    EXPECT_EQ (is.read_all (), "2\n");
  }

  {
    tl::InputStream is (tp + ".1");
    EXPECT_EQ (is.read_all (), "1\n");
  }

  {
    tl::OutputStream os (tp, tl::OutputStream::OM_Auto, false, 2);
    EXPECT_EQ (tl::file_exists (tp + ".~backup"), true);
    EXPECT_EQ (tl::file_exists (tp), true);
    os << "3\n";
  }

  EXPECT_EQ (tl::file_exists (tp + ".~backup"), false);
  EXPECT_EQ (tl::file_exists (tp + ".1"), true);
  EXPECT_EQ (tl::file_exists (tp + ".2"), true);
  EXPECT_EQ (tl::file_exists (tp + ".3"), false);
  EXPECT_EQ (tl::file_exists (tp), true);

  {
    tl::InputStream is (tp);
    EXPECT_EQ (is.read_all (), "3\n");
  }

  {
    tl::InputStream is (tp + ".1");
    EXPECT_EQ (is.read_all (), "2\n");
  }

  {
    tl::InputStream is (tp + ".2");
    EXPECT_EQ (is.read_all (), "1\n");
  }

  {
    tl::OutputStream os (tp, tl::OutputStream::OM_Auto, false, 2);
    EXPECT_EQ (tl::file_exists (tp + ".~backup"), true);
    EXPECT_EQ (tl::file_exists (tp), true);
    os << "4\n";
  }

  EXPECT_EQ (tl::file_exists (tp + ".~backup"), false);
  EXPECT_EQ (tl::file_exists (tp + ".1"), true);
  EXPECT_EQ (tl::file_exists (tp + ".2"), true);
  EXPECT_EQ (tl::file_exists (tp + ".3"), false);
  EXPECT_EQ (tl::file_exists (tp), true);

  {
    tl::InputStream is (tp);
    EXPECT_EQ (is.read_all (), "4\n");
  }

  {
    tl::InputStream is (tp + ".1");
    EXPECT_EQ (is.read_all (), "3\n");
  }

  {
    tl::InputStream is (tp + ".2");
    EXPECT_EQ (is.read_all (), "2\n");
  }

  try {
    BrokenOutputStream broken (tp, 2);
    tl::OutputStream os (broken);
    EXPECT_EQ (tl::file_exists (tp + ".~backup"), true);
    EXPECT_EQ (tl::file_exists (tp), true);
    os << "5!\n";
    os.flush ();   //  raises the exception
    EXPECT_EQ (true, false);
  } catch (...) {
    //  '!' raises an exception
  }

  EXPECT_EQ (tl::file_exists (tp + ".~backup"), false);
  EXPECT_EQ (tl::file_exists (tp + ".1"), true);
  EXPECT_EQ (tl::file_exists (tp + ".2"), true);
  EXPECT_EQ (tl::file_exists (tp + ".3"), false);
  EXPECT_EQ (tl::file_exists (tp), true);

  {
    tl::InputStream is (tp);
    EXPECT_EQ (is.read_all (), "4\n");
  }

  {
    tl::InputStream is (tp + ".1");
    EXPECT_EQ (is.read_all (), "3\n");
  }

  {
    tl::InputStream is (tp + ".2");
    EXPECT_EQ (is.read_all (), "2\n");
  }
}

TEST(RefuseToWrite)
{
  try {
    tl::OutputStream os ("");
    EXPECT_EQ (1, 0);
  } catch (tl::Exception &ex) {
    EXPECT_EQ (ex.msg (), "Path cannot be an empty string");
  }

  try {
    tl::OutputStream os (".");
    EXPECT_EQ (1, 0);
  } catch (tl::Exception &ex) {
    EXPECT_EQ (ex.msg ().find ("Path exists and is a directory"), size_t (0));
  }
}

TEST(AbstractPathFunctions)
{
  EXPECT_EQ (tl::InputStream::absolute_file_path (""), tl::absolute_file_path ("."));
  EXPECT_EQ (tl::InputStream::absolute_file_path ("."), tl::absolute_file_path ("."));
  EXPECT_EQ (tl::InputStream::absolute_file_path ("pipe:xyz"), "pipe:xyz");
  EXPECT_EQ (tl::InputStream::absolute_file_path ("data:xyz"), "data:xyz");
  EXPECT_EQ (tl::InputStream::absolute_file_path ("https:xyz"), "https:xyz");
  EXPECT_EQ (tl::InputStream::absolute_file_path ("http:xyz"), "http:xyz");
  EXPECT_EQ (tl::InputStream::absolute_file_path (":xyz"), ":xyz");
  EXPECT_EQ (tl::InputStream::absolute_file_path ("file:xyz"), tl::absolute_file_path ("xyz"));
  EXPECT_EQ (tl::InputStream::absolute_file_path ("xyz"), tl::absolute_file_path ("xyz"));
  EXPECT_EQ (tl::InputStream::absolute_file_path ("xyz/uvw"), tl::absolute_file_path ("xyz/uvw"));
  EXPECT_EQ (tl::InputStream::absolute_file_path ("/xyz/uvw"), tl::absolute_file_path ("/xyz/uvw"));
  tl::file_utils_force_windows ();
  EXPECT_EQ (tl::InputStream::absolute_file_path ("xyz\\uvw"), tl::absolute_file_path ("xyz\\uvw"));
  EXPECT_EQ (tl::InputStream::absolute_file_path ("\\\\server\\xyz\\uvw"), "\\\\server\\xyz\\uvw");
  EXPECT_EQ (tl::InputStream::absolute_file_path ("C:\\xyz\\uvw"), "C:\\xyz\\uvw");
  tl::file_utils_force_reset ();

  EXPECT_EQ (tl::InputStream::is_absolute (""), false);
  EXPECT_EQ (tl::InputStream::is_absolute ("."), false);
  EXPECT_EQ (tl::InputStream::is_absolute ("pipe:xyz"), true);
  EXPECT_EQ (tl::InputStream::is_absolute ("data:xyz"), true);
  EXPECT_EQ (tl::InputStream::is_absolute ("https:xyz"), true);
  EXPECT_EQ (tl::InputStream::is_absolute ("http:xyz"), true);
  EXPECT_EQ (tl::InputStream::is_absolute (":xyz"), true);
  EXPECT_EQ (tl::InputStream::is_absolute ("file:xyz"), false);
  EXPECT_EQ (tl::InputStream::is_absolute ("xyz"), false);
  EXPECT_EQ (tl::InputStream::is_absolute ("xyz/uvw"), false);
  tl::file_utils_force_linux ();
  EXPECT_EQ (tl::InputStream::is_absolute ("/xyz/uvw"), true);
  tl::file_utils_force_windows ();
  EXPECT_EQ (tl::InputStream::is_absolute ("xyz\\uvw"), false);
  EXPECT_EQ (tl::InputStream::is_absolute ("\\\\server\\xyz\\uvw"), true);
  EXPECT_EQ (tl::InputStream::is_absolute ("c:\\xyz\\uvw"), true);
  tl::file_utils_force_reset ();

  tl::file_utils_force_windows ();
  EXPECT_EQ (tl::InputStream::combine ("a", ""), "a");
  EXPECT_EQ (tl::InputStream::combine ("", "b"), "\\b");
  EXPECT_EQ (tl::InputStream::combine ("a", "b"), "a\\b");
  EXPECT_EQ (tl::InputStream::combine ("a", "b/c"), "a\\b/c");
  EXPECT_EQ (tl::InputStream::combine ("a", "b\\c"), "a\\b\\c");
  EXPECT_EQ (tl::InputStream::combine ("a", "data:abc"), "data:abc");
  EXPECT_EQ (tl::InputStream::combine ("data:a", "b"), "b");
  EXPECT_EQ (tl::InputStream::combine ("pipe:a", "b"), "b");
  EXPECT_EQ (tl::InputStream::combine (":a", "b"), ":a/b");
  EXPECT_EQ (tl::InputStream::combine ("https://a", "b"), "https://a/b");
  EXPECT_EQ (tl::InputStream::combine ("https://a", "https:b"), "https:b");
  EXPECT_EQ (tl::InputStream::combine ("a", "https:b"), "https:b");
  EXPECT_EQ (tl::InputStream::combine ("a", "file:b"), "a\\b");
  EXPECT_EQ (tl::InputStream::combine ("a", "file:\\b"), "file:\\b");
  EXPECT_EQ (tl::InputStream::combine ("file:a", "file:b"), "file:a/b");
  EXPECT_EQ (tl::InputStream::combine ("file:a", "file:b/c"), "file:a/b/c");
  EXPECT_EQ (tl::InputStream::combine ("file:a", "b\\c"), "file:a/b/c");
  tl::file_utils_force_linux ();
  EXPECT_EQ (tl::InputStream::combine ("a", "b"), "a/b");
  EXPECT_EQ (tl::InputStream::combine ("", "b"), "/b");
  EXPECT_EQ (tl::InputStream::combine ("a", "b/c"), "a/b/c");
  EXPECT_EQ (tl::InputStream::combine ("a", "data:abc"), "data:abc");
  EXPECT_EQ (tl::InputStream::combine ("data:a", "b"), "b");
  EXPECT_EQ (tl::InputStream::combine ("pipe:a", "b"), "b");
  EXPECT_EQ (tl::InputStream::combine (":a", "b"), ":a/b");
  EXPECT_EQ (tl::InputStream::combine ("https://a", "b"), "https://a/b");
  EXPECT_EQ (tl::InputStream::combine ("https://a", "https:b"), "https:b");
  EXPECT_EQ (tl::InputStream::combine ("a", "https:b"), "https:b");
  EXPECT_EQ (tl::InputStream::combine ("a", "file:b"), "a/b");
  EXPECT_EQ (tl::InputStream::combine ("a", "file:/b"), "file:/b");
  EXPECT_EQ (tl::InputStream::combine ("file:a", "file:b"), "file:a/b");
  EXPECT_EQ (tl::InputStream::combine ("file:a", "file:b/c"), "file:a/b/c");
  EXPECT_EQ (tl::InputStream::combine ("file:a", "b/c"), "file:a/b/c");
  tl::file_utils_force_reset ();

  tl::file_utils_force_linux ();
  EXPECT_EQ (tl::InputStream::relative_path ("", "file:/a/b/c"), "/a/b/c");
  EXPECT_EQ (tl::InputStream::relative_path (".", "file:/a/b/c"), "/a/b/c");
  EXPECT_EQ (tl::InputStream::relative_path ("https://x", "a/b/c"), "a/b/c");
  EXPECT_EQ (tl::InputStream::relative_path ("file:/a/b", "file:/a/b/c"), "c");
  EXPECT_EQ (tl::InputStream::relative_path ("/a/b", "/a/b/c"), "c");
  EXPECT_EQ (tl::InputStream::relative_path ("/a/b", "/x/b/c"), "/x/b/c");
  EXPECT_EQ (tl::InputStream::relative_path ("file:/a/b", "file:/a/b/c"), "c");
  EXPECT_EQ (tl::InputStream::relative_path ("/a/b", "/a/b/c"), "c");
  tl::file_utils_force_windows ();
  EXPECT_EQ (tl::InputStream::relative_path ("/a/b", "/a/b/c"), "c");
  EXPECT_EQ (tl::InputStream::relative_path ("/a/b", "\\a\\b\\c\\d"), "c\\d");
  tl::file_utils_force_reset ();

  EXPECT_EQ (tl::InputStream::is_file_path (""), true);
  EXPECT_EQ (tl::InputStream::is_file_path (":abc"), false);
  EXPECT_EQ (tl::InputStream::is_file_path ("pipe:abc"), false);
  EXPECT_EQ (tl::InputStream::is_file_path ("data:abc"), false);
  EXPECT_EQ (tl::InputStream::is_file_path ("http:abc"), false);
  EXPECT_EQ (tl::InputStream::is_file_path ("file:abc"), true);
  EXPECT_EQ (tl::InputStream::is_file_path ("a/b/c"), true);
  tl::file_utils_force_windows ();
  EXPECT_EQ (tl::InputStream::is_file_path ("a\\b\\c"), true);
  tl::file_utils_force_reset ();

  EXPECT_EQ (tl::InputStream::as_file_path (""), std::string ());
  EXPECT_EQ (tl::InputStream::as_file_path (":abc"), std::string ());
  EXPECT_EQ (tl::InputStream::as_file_path ("pipe:abc"), std::string ());
  EXPECT_EQ (tl::InputStream::as_file_path ("data:abc"), std::string ());
  EXPECT_EQ (tl::InputStream::as_file_path ("http:abc"), std::string ());
  EXPECT_EQ (tl::InputStream::as_file_path ("file:abc"), "abc");
  EXPECT_EQ (tl::InputStream::as_file_path ("a/b/c"), "a/b/c");
  tl::file_utils_force_windows ();
  EXPECT_EQ (tl::InputStream::as_file_path ("a\\b\\c"), "a\\b\\c");
  tl::file_utils_force_reset ();
}
