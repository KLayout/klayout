
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

#if defined(HAVE_CURL) || defined(HAVE_QT)

#include "tlWebDAV.h"
#include "tlHttpStream.h"
#include "tlUnitTest.h"
#include "tlFileUtils.h"

static std::string test_url1 ("http://www.klayout.org/svn-public/klayout-resources/trunk/testdata");
static std::string test_url2 ("http://www.klayout.org/svn-public/klayout-resources/trunk/testdata/text");

static std::string collection2string (const tl::WebDAVObject &coll)
{
  std::string s;
  for (tl::WebDAVObject::iterator c = coll.begin (); c != coll.end (); ++c) {
    if (!s.empty ()) {
      s += "\n";
    }
    if (c->is_collection ()) {
      s += "[dir] ";
    }
    s += c->name ();
    s += " ";
    s += c->url ();
  }
  return s;
}

TEST(1)
{
  if (! tl::InputHttpStream::is_available ()) {
    throw tl::CancelException ();
  }

  tl::WebDAVObject collection;
  collection.read (test_url1, 1);

  EXPECT_EQ (collection.is_collection (), true);
  EXPECT_EQ (collection.url (), "http://www.klayout.org/svn-public/klayout-resources/trunk/testdata/");

  std::string s = collection2string (collection);

  //  normalize
  std::vector<std::string> sl = tl::split (s, "\n");
  std::sort (sl.begin (), sl.end ());
  s = tl::join (sl, "\n");

  EXPECT_EQ (s,
    "[dir] dir1 http://www.klayout.org/svn-public/klayout-resources/trunk/testdata/dir1/\n"
    "[dir] dir2 http://www.klayout.org/svn-public/klayout-resources/trunk/testdata/dir2/\n"
    "text http://www.klayout.org/svn-public/klayout-resources/trunk/testdata/text\n"
    "text2 http://www.klayout.org/svn-public/klayout-resources/trunk/testdata/text2"
  );
}

TEST(2)
{
  if (! tl::InputHttpStream::is_available ()) {
    throw tl::CancelException ();
  }

  tl::WebDAVObject collection;
  collection.read (test_url1, 0);

  EXPECT_EQ (collection.is_collection (), true);
  EXPECT_EQ (collection.url (), "http://www.klayout.org/svn-public/klayout-resources/trunk/testdata/");
  EXPECT_EQ (collection2string (collection), "");
}

TEST(3)
{
  if (! tl::InputHttpStream::is_available ()) {
    throw tl::CancelException ();
  }

  tl::WebDAVObject collection;
  collection.read (test_url2, 1);

  EXPECT_EQ (collection.is_collection (), false);
  EXPECT_EQ (collection.url (), "http://www.klayout.org/svn-public/klayout-resources/trunk/testdata/text");
  EXPECT_EQ (collection2string (collection), "");
}

TEST(4)
{
  if (! tl::InputHttpStream::is_available ()) {
    throw tl::CancelException ();
  }

  tl::WebDAVObject collection;
  collection.read (test_url2, 0);

  EXPECT_EQ (collection.is_collection (), false);
  EXPECT_EQ (collection.url (), "http://www.klayout.org/svn-public/klayout-resources/trunk/testdata/text");
  EXPECT_EQ (collection2string (collection), "");
}

TEST(5)
{
  if (! tl::InputHttpStream::is_available ()) {
    throw tl::CancelException ();
  }

  tl::WebDAVObject collection;

  std::string tmp_dir (tmp_file ("tmp"));
  EXPECT_EQ (tl::file_exists (tmp_dir), false);

  tl::mkpath (tmp_dir);
  EXPECT_EQ (tl::file_exists (tmp_dir), true);

  bool res = collection.download (test_url1, tl::absolute_file_path (tmp_dir));
  EXPECT_EQ (res, true);

  std::string dir1 (tl::absolute_file_path (tl::combine_path (tmp_dir, "dir1")));
  std::string dir2 (tl::absolute_file_path (tl::combine_path (tmp_dir, "dir2")));
  std::string dir21 (tl::absolute_file_path (tl::combine_path (dir2, "dir21")));
  EXPECT_EQ (tl::file_exists (dir1), true);
  EXPECT_EQ (tl::file_exists (dir2), true);
  EXPECT_EQ (tl::file_exists (dir21), true);

  tl::InputStream text1 (tl::combine_path (dir1, "text"));
  std::string ba1 = text1.read_all ();
  EXPECT_EQ (ba1, "A text.\n");
  text1.close ();

  tl::InputStream text21 (tl::combine_path (dir21, "text"));
  std::string ba21 = text21.read_all ();
  EXPECT_EQ (ba21, "A text II.I.\n");
  text21.close ();
}

#endif

