
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


#include "tlFileUtils.h"
#include "tlUnitTest.h"

#if defined(HAVE_QT)

//  A few things we cross-check against Qt

#include <QDir>
#include <QFileInfo>
#include <QFile>

TEST (1)
{
  EXPECT_EQ (tl::is_parent_path (std::string ("."), "./doesnotexist"), true);
  EXPECT_EQ (tl::is_parent_path (std::string ("./doesnotexist"), "./alsodoesnotexist"), false);
  EXPECT_EQ (tl::is_parent_path (std::string ("."), "."), true);

  QString p = QFileInfo (tl::to_qstring (tmp_file ())).path ();
  QDir (p).mkdir (QString::fromUtf8 ("x"));
  QDir (p).mkdir (QString::fromUtf8 ("y"));
  EXPECT_EQ (tl::is_parent_path (tl::to_string (p), tl::to_string (p)), true);
  EXPECT_EQ (tl::is_parent_path (tl::to_string (p), tl::to_string (p) + "/x"), true);
  EXPECT_EQ (tl::is_parent_path (tl::to_string (p) + "/x", tl::to_string (p) + "/x"), true);
  EXPECT_EQ (tl::is_parent_path (tl::to_string (p) + "/x", tl::to_string (p) + "/y"), false);
  EXPECT_EQ (tl::is_parent_path (tl::to_string (QDir::root ().absolutePath ()), tl::to_string (p) + "/y"), true);
  EXPECT_EQ (tl::is_parent_path (tl::to_string (QDir::root ().absolutePath ()), tl::to_string (p)), true);
}

TEST (2)
{
  QDir tmp_dir = QFileInfo (tl::to_qstring (tmp_file ())).absoluteDir ();
  tmp_dir.mkdir (QString::fromUtf8 ("a"));

  QDir adir = tmp_dir;
  adir.cd (QString::fromUtf8 ("a"));

  EXPECT_EQ (adir.exists (), true);
  EXPECT_EQ (tl::rm_dir_recursive (tl::to_string (adir.absolutePath ())), true);
  EXPECT_EQ (adir.exists (), false);

  tmp_dir.mkdir (QString::fromUtf8 ("a"));
  EXPECT_EQ (adir.exists (), true);

  EXPECT_EQ (tl::rm_dir_recursive (tl::to_string (adir.absolutePath ())), true);
  EXPECT_EQ (adir.exists (), false);

  tmp_dir.mkdir (QString::fromUtf8 ("a"));
  EXPECT_EQ (adir.exists (), true);

  adir.mkdir (QString::fromUtf8 ("b1"));
  QDir b1dir = adir;
  b1dir.cd (QString::fromUtf8 ("b1"));

  adir.mkdir (QString::fromUtf8 ("b2"));
  QDir b2dir = adir;
  b2dir.cd (QString::fromUtf8 ("b2"));

  {
    QFile file (b2dir.absoluteFilePath (QString::fromUtf8 ("x")));
    file.open (QIODevice::WriteOnly);
    file.write ("hello, world!\n");
    file.close ();
  }

  {
    QFile file (b2dir.absoluteFilePath (QString::fromUtf8 ("y")));
    file.open (QIODevice::WriteOnly);
    file.write ("hello, world!\n");
    file.close ();
  }

  EXPECT_EQ (adir.exists (), true);
  EXPECT_EQ (tl::rm_dir_recursive (tl::to_string (adir.absolutePath ())), true);
  EXPECT_EQ (adir.exists (), false);
  EXPECT_EQ (b1dir.exists (), false);
  EXPECT_EQ (b2dir.exists (), false);
  EXPECT_EQ (QFileInfo (b2dir.absoluteFilePath (QString::fromUtf8 ("x"))).exists (), false);
}

TEST (3)
{
  QDir tmp_dir = QFileInfo (tl::to_qstring (tmp_file ())).absoluteDir ();

  tl::rm_dir_recursive (tl::to_string (tmp_dir.filePath (QString::fromUtf8 ("a"))));
  tmp_dir.mkdir (QString::fromUtf8 ("a"));

  QDir adir = tmp_dir;
  adir.cd (QString::fromUtf8 ("a"));

  adir.mkdir (QString::fromUtf8 ("b1"));
  QDir b1dir = adir;
  b1dir.cd (QString::fromUtf8 ("b1"));

  adir.mkdir (QString::fromUtf8 ("b2"));
  QDir b2dir = adir;
  b2dir.cd (QString::fromUtf8 ("b2"));

  {
    QFile file (b2dir.absoluteFilePath (QString::fromUtf8 ("x")));
    file.open (QIODevice::WriteOnly);
    file.write ("hello, world!\n");
    file.close ();
  }

  {
    QFile file (b2dir.absoluteFilePath (QString::fromUtf8 ("y")));
    file.open (QIODevice::WriteOnly);
    file.write ("hello, world II!\n");
    file.close ();
  }

  tl::rm_dir_recursive (tl::to_string (tmp_dir.filePath (QString::fromUtf8 ("acopy"))));
  tmp_dir.mkdir (QString::fromUtf8 ("acopy"));

  tl::cp_dir_recursive (tl::to_string (tmp_dir.absoluteFilePath (QString::fromUtf8 ("a"))), tl::to_string (tmp_dir.absoluteFilePath (QString::fromUtf8 ("acopy"))));

  QDir acopydir = tmp_dir;
  EXPECT_EQ (acopydir.cd (QString::fromUtf8 ("acopy")), true);
  EXPECT_EQ (acopydir.exists (), true);

  QDir b1copydir = acopydir;
  EXPECT_EQ (b1copydir.cd (QString::fromUtf8 ("b1")), true);
  EXPECT_EQ (b1copydir.exists (), true);

  QDir b2copydir = acopydir;
  EXPECT_EQ (b2copydir.cd (QString::fromUtf8 ("b2")), true);
  EXPECT_EQ (b2copydir.exists (), true);

  {
    QFile file (b2copydir.absoluteFilePath (QString::fromUtf8 ("x")));
    EXPECT_EQ (file.exists (), true);
    file.open (QIODevice::ReadOnly);
    EXPECT_EQ (file.readAll ().constData (), "hello, world!\n");
    file.close ();
  }

  {
    QFile file (b2copydir.absoluteFilePath (QString::fromUtf8 ("y")));
    EXPECT_EQ (file.exists (), true);
    file.open (QIODevice::ReadOnly);
    EXPECT_EQ (file.readAll ().constData (), "hello, world II!\n");
    file.close ();
  }
}

//  Secret mode switchers for testing
namespace tl
{
TL_PUBLIC void file_utils_force_windows ();
TL_PUBLIC void file_utils_force_linux ();
TL_PUBLIC void file_utils_force_reset ();
}

//  Fake Windows-tests
TEST (10)
{
  tl::file_utils_force_windows ();
  try {

    EXPECT_EQ (tl::join (tl::split_path ("\\hello\\world"), "+"), "\\hello+\\world");
    EXPECT_EQ (tl::join (tl::split_path ("\\hello\\\\world\\"), "+"), "\\hello+\\world");
    EXPECT_EQ (tl::join (tl::split_path ("hello\\\\world\\"), "+"), "hello+\\world");
    EXPECT_EQ (tl::join (tl::split_path ("\\\\SERVER\\hello\\world"), "+"), "\\\\SERVER+\\hello+\\world");
    EXPECT_EQ (tl::join (tl::split_path ("c:\\hello\\\\world\\"), "+"), "C:+\\hello+\\world");

    //  slashes are good too:
    EXPECT_EQ (tl::join (tl::split_path ("/hello/world"), "+"), "\\hello+\\world");
    EXPECT_EQ (tl::join (tl::split_path ("/hello//world/"), "+"), "\\hello+\\world");
    EXPECT_EQ (tl::join (tl::split_path ("hello//world/"), "+"), "hello+\\world");
    EXPECT_EQ (tl::join (tl::split_path ("//SERVER/hello/world"), "+"), "\\\\SERVER+\\hello+\\world");
    EXPECT_EQ (tl::join (tl::split_path ("c:/hello//world/"), "+"), "C:+\\hello+\\world");

    //  boundary cases
    EXPECT_EQ (tl::join (tl::split_path (""), "+"), "");
    EXPECT_EQ (tl::join (tl::split_path ("\\"), "+"), "\\");
    EXPECT_EQ (tl::join (tl::split_path ("/"), "+"), "\\");
    EXPECT_EQ (tl::join (tl::split_path ("d:"), "+"), "D:");
    EXPECT_EQ (tl::join (tl::split_path ("\\\\"), "+"), "\\\\");
    EXPECT_EQ (tl::join (tl::split_path ("//"), "+"), "\\\\");
    EXPECT_EQ (tl::join (tl::split_path ("d:\\"), "+"), "D:+\\");
    EXPECT_EQ (tl::join (tl::split_path ("d:\\\\"), "+"), "D:+\\");
    EXPECT_EQ (tl::join (tl::split_path ("d:/"), "+"), "D:+\\");
    EXPECT_EQ (tl::join (tl::split_path ("d://"), "+"), "D:+\\");

    EXPECT_EQ (tl::dirname ("/hello/world"), "\\hello");
    EXPECT_EQ (tl::dirname ("\\hello\\world"), "\\hello");
    EXPECT_EQ (tl::dirname ("/hello//world/"), "\\hello\\world");
    EXPECT_EQ (tl::dirname ("\\hello\\\\world\\"), "\\hello\\world");
    EXPECT_EQ (tl::dirname ("hello//world/"), "hello\\world");
    EXPECT_EQ (tl::dirname ("hello\\\\world\\"), "hello\\world");
    EXPECT_EQ (tl::dirname ("\\\\SERVER\\hello\\world"), "\\\\SERVER\\hello");
    EXPECT_EQ (tl::dirname ("//SERVER/hello/world"), "\\\\SERVER\\hello");
    EXPECT_EQ (tl::dirname ("c:\\hello\\world"), "C:\\hello");
    EXPECT_EQ (tl::dirname ("c:\\hello\\\\world"), "C:\\hello");
    EXPECT_EQ (tl::dirname ("c:/hello//world"), "C:\\hello");
    EXPECT_EQ (tl::dirname ("c:/hello//world/"), "C:\\hello\\world");

    EXPECT_EQ (tl::filename ("/hello/world"), "world");
    EXPECT_EQ (tl::filename ("\\hello\\world"), "world");
    EXPECT_EQ (tl::filename ("/hello//world/"), "");
    EXPECT_EQ (tl::filename ("\\hello\\\\world\\"), "");
    EXPECT_EQ (tl::filename ("hello//world/"), "");
    EXPECT_EQ (tl::filename ("hello\\\\world\\"), "");
    EXPECT_EQ (tl::filename ("\\\\SERVER\\hello\\world"), "world");
    EXPECT_EQ (tl::filename ("//SERVER/hello/world"), "world");
    EXPECT_EQ (tl::filename ("c:\\hello\\world"), "world");
    EXPECT_EQ (tl::filename ("c:\\hello\\\\world"), "world");
    EXPECT_EQ (tl::filename ("c:/hello//world"), "world");
    EXPECT_EQ (tl::filename ("c:/hello//world/"), "");

    EXPECT_EQ (tl::basename ("/hello/world"), "world");
    EXPECT_EQ (tl::basename ("/hello/world.tar"), "world");
    EXPECT_EQ (tl::basename ("/hello/world.tar.gz"), "world");
    EXPECT_EQ (tl::basename ("\\hello\\.world"), ".world");
    EXPECT_EQ (tl::basename ("\\hello\\.world.gz"), ".world");
    EXPECT_EQ (tl::basename ("/hello//world/"), "");

    EXPECT_EQ (tl::extension ("/hello/world"), "");
    EXPECT_EQ (tl::extension ("/hello/world.tar"), "tar");
    EXPECT_EQ (tl::extension ("/hello/world.tar.gz"), "tar.gz");
    EXPECT_EQ (tl::extension ("\\hello\\.world"), "");
    EXPECT_EQ (tl::extension ("\\hello\\.world.gz"), "gz");
    EXPECT_EQ (tl::extension ("/hello//world/"), "");

    EXPECT_EQ (tl::combine_path ("hello", "world"), "hello\\world");
    EXPECT_EQ (tl::combine_path ("hello", ""), "hello");
    EXPECT_EQ (tl::combine_path ("hello", "", true), "hello\\");

    tl::file_utils_force_reset ();

  } catch (...) {
    tl::file_utils_force_reset ();
    throw;
  }
}

//  Fake Linux-tests
TEST (11)
{
  tl::file_utils_force_linux ();
  try {

    EXPECT_EQ (tl::join (tl::split_path ("/hello/world"), "+"), "/hello+/world");
    EXPECT_EQ (tl::join (tl::split_path ("/hel\\/\\\\lo/world"), "+"), "/hel\\/\\\\lo+/world");
    EXPECT_EQ (tl::join (tl::split_path ("/hello//world/"), "+"), "/hello+/world");
    EXPECT_EQ (tl::join (tl::split_path ("hello//world/"), "+"), "hello+/world");

    //  boundary cases
    EXPECT_EQ (tl::join (tl::split_path (""), "+"), "");
    EXPECT_EQ (tl::join (tl::split_path ("/"), "+"), "/");
    EXPECT_EQ (tl::join (tl::split_path ("//"), "+"), "/");

    EXPECT_EQ (tl::dirname ("/hello/world"), "/hello");
    EXPECT_EQ (tl::dirname ("/hello//world/"), "/hello/world");
    EXPECT_EQ (tl::dirname ("hello//world/"), "hello/world");

    EXPECT_EQ (tl::filename ("/hello/world"), "world");
    EXPECT_EQ (tl::filename ("/hello//world/"), "");
    EXPECT_EQ (tl::filename ("hello//world/"), "");

    EXPECT_EQ (tl::basename ("/hello/world"), "world");
    EXPECT_EQ (tl::basename ("/hello/world.tar"), "world");
    EXPECT_EQ (tl::basename ("/hello/world.tar.gz"), "world");
    EXPECT_EQ (tl::basename ("/hello/.world"), ".world");
    EXPECT_EQ (tl::basename ("/hello/.world.gz"), ".world");
    EXPECT_EQ (tl::basename ("/hello//world/"), "");

    EXPECT_EQ (tl::extension ("/hello/world"), "");
    EXPECT_EQ (tl::extension ("/hello///world.tar"), "tar");
    EXPECT_EQ (tl::extension ("/hello/world.tar.gz"), "tar.gz");
    EXPECT_EQ (tl::extension ("/hello//.world"), "");
    EXPECT_EQ (tl::extension ("/hello/.world.gz"), "gz");
    EXPECT_EQ (tl::extension ("/hello//world/"), "");

    EXPECT_EQ (tl::combine_path ("hello", "world"), "hello/world");
    EXPECT_EQ (tl::combine_path ("hello", ""), "hello");
    EXPECT_EQ (tl::combine_path ("hello", "", true), "hello/");

    tl::file_utils_force_reset ();

  } catch (...) {
    tl::file_utils_force_reset ();
    throw;
  }
}

//  current_dir
TEST (12)
{
  std::string currdir = tl::current_dir ();
  std::string currdir_abs = tl::absolute_file_path (".");
  EXPECT_EQ (currdir, currdir_abs);

  std::string above = tl::absolute_file_path ("..");
  EXPECT_EQ (tl::is_same_file (currdir, above), false);
  EXPECT_EQ (tl::is_parent_path (currdir, above), false);
  EXPECT_EQ (tl::is_parent_path (currdir, currdir), true);
  EXPECT_EQ (tl::is_parent_path (above, currdir), true);
  EXPECT_EQ (tl::is_parent_path (above, above), true);
  EXPECT_EQ (tl::is_same_file (tl::combine_path (currdir, ".."), above), true);
}

#endif
