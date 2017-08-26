
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


#include "tlWebDAV.h"
#include "utHead.h"

#include <QDir>

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
  tl::WebDAVObject collection;
  collection.read (test_url1, 1);

  EXPECT_EQ (collection.is_collection (), true);
  EXPECT_EQ (collection.url (), "http://www.klayout.org/svn-public/klayout-resources/trunk/testdata/");

  EXPECT_EQ (collection2string (collection),
    "[dir] dir1 http://www.klayout.org/svn-public/klayout-resources/trunk/testdata/dir1/\n"
    "[dir] dir2 http://www.klayout.org/svn-public/klayout-resources/trunk/testdata/dir2/\n"
    "text http://www.klayout.org/svn-public/klayout-resources/trunk/testdata/text\n"
    "text2 http://www.klayout.org/svn-public/klayout-resources/trunk/testdata/text2"
  );
}

TEST(2)
{
  tl::WebDAVObject collection;
  collection.read (test_url1, 0);

  EXPECT_EQ (collection.is_collection (), true);
  EXPECT_EQ (collection.url (), "http://www.klayout.org/svn-public/klayout-resources/trunk/testdata/");
  EXPECT_EQ (collection2string (collection), "");
}

TEST(3)
{
  tl::WebDAVObject collection;
  collection.read (test_url2, 1);

  EXPECT_EQ (collection.is_collection (), false);
  EXPECT_EQ (collection.url (), "http://www.klayout.org/svn-public/klayout-resources/trunk/testdata/text");
  EXPECT_EQ (collection2string (collection), "");
}

TEST(4)
{
  tl::WebDAVObject collection;
  collection.read (test_url2, 0);

  EXPECT_EQ (collection.is_collection (), false);
  EXPECT_EQ (collection.url (), "http://www.klayout.org/svn-public/klayout-resources/trunk/testdata/text");
  EXPECT_EQ (collection2string (collection), "");
}

TEST(5)
{
  tl::WebDAVObject collection;

  QDir tmp_dir (tl::to_qstring (tmp_file ("tmp")));
  EXPECT_EQ (tmp_dir.exists (), false);

  tmp_dir.cdUp ();
  tmp_dir.mkdir (tl::to_qstring ("tmp"));
  tmp_dir.cd (tl::to_qstring ("tmp"));

  bool res = collection.download (test_url1, tl::to_string (tmp_dir.absolutePath ()));
  EXPECT_EQ (res, true);

  QDir dir1 (tmp_dir.absoluteFilePath (QString::fromUtf8 ("dir1")));
  QDir dir2 (tmp_dir.absoluteFilePath (QString::fromUtf8 ("dir2")));
  QDir dir21 (dir2.absoluteFilePath (QString::fromUtf8 ("dir21")));
  EXPECT_EQ (dir1.exists (), true);
  EXPECT_EQ (dir2.exists (), true);
  EXPECT_EQ (dir21.exists (), true);

  QByteArray ba;

  QFile text1 (dir1.absoluteFilePath (QString::fromUtf8 ("text")));
  text1.open (QIODevice::ReadOnly);
  ba = text1.read (10000);
  EXPECT_EQ (ba.constData (), "A text.\n");
  text1.close ();

  QFile text21 (dir21.absoluteFilePath (QString::fromUtf8 ("text")));
  text21.open (QIODevice::ReadOnly);
  ba = text21.read (10000);
  EXPECT_EQ (ba.constData (), "A text II.I.\n");
  text21.close ();
}
