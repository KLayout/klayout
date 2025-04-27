
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


#include "tlFileUtils.h"
#include "tlUnitTest.h"
#include "tlStream.h"
#include "tlString.h"

#include <fstream>

#if defined(HAVE_QT)
//  A few things we cross-check against Qt
#  include <QDir>
#  include <QFileInfo>
#  include <QFile>
#endif

#if defined(HAVE_QT)
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
#endif

TEST (1_NOQT)
{
  std::string root = tl::absolute_file_path ("/");
  EXPECT_EQ (tl::is_parent_path (std::string ("."), "./doesnotexist"), true);
  EXPECT_EQ (tl::is_parent_path (std::string ("./doesnotexist"), "./alsodoesnotexist"), false);
  EXPECT_EQ (tl::is_parent_path (std::string ("."), "."), true);

  std::string p = tl::absolute_path (tmp_file ());
  tl::mkpath (tl::combine_path (tl::combine_path (p, "x"), "y"));
  EXPECT_EQ (tl::is_parent_path (p, tl::to_string (p)), true);
  EXPECT_EQ (tl::is_parent_path (p, tl::to_string (p) + "/x"), true);
  EXPECT_EQ (tl::is_parent_path (p + "/x", tl::to_string (p) + "/x"), true);
  EXPECT_EQ (tl::is_parent_path (p + "/x", tl::to_string (p) + "/y"), false);
  EXPECT_EQ (tl::is_parent_path (root, tl::to_string (p) + "/y"), true);
  EXPECT_EQ (tl::is_parent_path (root, tl::to_string (p)), true);
}

#if defined(HAVE_QT)
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
#endif

TEST (2_NOQT)
{
  std::string tmp_dir = tl::absolute_file_path (tmp_file ());
  std::string adir = tl::combine_path (tmp_dir, "a");
  tl::mkpath (adir);

  EXPECT_EQ (tl::file_exists (adir), true);
  EXPECT_EQ (tl::rm_dir_recursive (adir), true);
  EXPECT_EQ (tl::file_exists (adir), false);

  tl::mkpath (adir);
  EXPECT_EQ (tl::file_exists (adir), true);

  EXPECT_EQ (tl::rm_dir_recursive (adir), true);
  EXPECT_EQ (tl::file_exists (adir), false);

  std::string b1dir = tl::combine_path (adir, "b1");
  tl::mkpath (b1dir);
  EXPECT_EQ (tl::file_exists (b1dir), true);

  std::string b2dir = tl::combine_path (adir, "b2");
  tl::mkpath (b2dir);
  EXPECT_EQ (tl::file_exists (b1dir), true);

  {
    tl::OutputStream os (tl::absolute_file_path (tl::combine_path (b2dir, "x")));
    os << "hello, world!\n";
  }

  {
    tl::OutputStream os (tl::absolute_file_path (tl::combine_path (b2dir, "y")));
    os << "hello, world!\n";
  }

  EXPECT_EQ (tl::file_exists (adir), true);
  EXPECT_EQ (tl::rm_dir_recursive (adir), true);
  EXPECT_EQ (tl::file_exists (adir), false);
  EXPECT_EQ (tl::file_exists (b1dir), false);
  EXPECT_EQ (tl::file_exists (tl::combine_path (b2dir, "x")), false);
  EXPECT_EQ (tl::file_exists (tl::combine_path (b2dir, "y")), false);

  //  with dotfiles and dotdirs

  adir = tl::combine_path (tmp_dir, ".a");
  tl::mkpath (adir);
  EXPECT_EQ (tl::file_exists (adir), true);

  EXPECT_EQ (tl::rm_dir_recursive (adir), true);
  EXPECT_EQ (tl::file_exists (adir), false);

  b1dir = tl::combine_path (adir, ".b1");
  tl::mkpath (b1dir);
  EXPECT_EQ (tl::file_exists (b1dir), true);

  b2dir = tl::combine_path (adir, ".b2");
  tl::mkpath (b2dir);
  EXPECT_EQ (tl::file_exists (b1dir), true);

  {
    tl::OutputStream os (tl::absolute_file_path (tl::combine_path (b2dir, ".x")));
    os << "hello, world!\n";
  }

  {
    tl::OutputStream os (tl::absolute_file_path (tl::combine_path (b2dir, ".y")));
    os << "hello, world!\n";
  }

  EXPECT_EQ (tl::file_exists (adir), true);
  EXPECT_EQ (tl::rm_dir_recursive (adir), true);
  EXPECT_EQ (tl::file_exists (adir), false);
  EXPECT_EQ (tl::file_exists (b1dir), false);
  EXPECT_EQ (tl::file_exists (tl::combine_path (b2dir, ".x")), false);
  EXPECT_EQ (tl::file_exists (tl::combine_path (b2dir, ".y")), false);
}

#if defined(HAVE_QT)
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
#endif

TEST (3_NOQT)
{
  std::string tmp_dir = tl::absolute_file_path (tmp_file ());
  std::string adir = tl::combine_path (tmp_dir, "a");

  std::string b1dir = tl::combine_path (adir, "b1");
  tl::mkpath (b1dir);
  EXPECT_EQ (tl::file_exists (b1dir), true);

  std::string b2dir = tl::combine_path (adir, "b2");
  tl::mkpath (b2dir);
  EXPECT_EQ (tl::file_exists (b1dir), true);

  {
    tl::OutputStream os (tl::absolute_file_path (tl::combine_path (b2dir, "x")));
    os << "hello, world!\n";
  }

  {
    tl::OutputStream os (tl::absolute_file_path (tl::combine_path (b2dir, "y")));
    os << "hello, world II!\n";
  }

  std::string acopydir = tl::combine_path (tmp_dir, "acopy");
  tl::rm_dir_recursive (acopydir);
  tl::mkpath (acopydir);

  tl::cp_dir_recursive (adir, acopydir);

  EXPECT_EQ (tl::file_exists (acopydir), true);

  std::string b1copydir = tl::combine_path (acopydir, "b1");
  EXPECT_EQ (tl::file_exists (b1copydir), true);
  std::string b2copydir = tl::combine_path (acopydir, "b2");
  EXPECT_EQ (tl::file_exists (b2copydir), true);

  {
    std::string xfile = tl::combine_path (b2copydir, "x");
    EXPECT_EQ (tl::file_exists (xfile), true);
    tl::InputStream is (xfile);
    EXPECT_EQ (is.read_all (), "hello, world!\n");
  }

  {
    std::string yfile = tl::combine_path (b2copydir, "y");
    EXPECT_EQ (tl::file_exists (yfile), true);
    tl::InputStream is (yfile);
    EXPECT_EQ (is.read_all (), "hello, world II!\n");
  }
}

TEST (4_mv_dir)
{
  std::string tmp_dir = tl::absolute_file_path (tmp_file ());
  std::string adir = tl::combine_path (tmp_dir, "a");

  std::string b1dir = tl::combine_path (adir, "b1");
  tl::mkpath (b1dir);
  EXPECT_EQ (tl::file_exists (b1dir), true);

  std::string b2dir = tl::combine_path (adir, "b2");
  tl::mkpath (b2dir);
  EXPECT_EQ (tl::file_exists (b1dir), true);

  {
    tl::OutputStream os (tl::absolute_file_path (tl::combine_path (b2dir, "x")));
    os << "hello, world!\n";
  }

  {
    tl::OutputStream os (tl::absolute_file_path (tl::combine_path (b2dir, "y")));
    os << "hello, world II!\n";
  }

  std::string acopydir = tl::combine_path (tmp_dir, "acopy");
  tl::rm_dir_recursive (acopydir);
  tl::mkpath (acopydir);

  tl::mv_dir_recursive (adir, acopydir);

  EXPECT_EQ (tl::file_exists (acopydir), true);
  EXPECT_EQ (tl::file_exists (adir), false);

  std::string b1copydir = tl::combine_path (acopydir, "b1");
  EXPECT_EQ (tl::file_exists (b1copydir), true);
  std::string b2copydir = tl::combine_path (acopydir, "b2");
  EXPECT_EQ (tl::file_exists (b2copydir), true);

  {
    std::string xfile = tl::combine_path (b2copydir, "x");
    EXPECT_EQ (tl::file_exists (xfile), true);
    tl::InputStream is (xfile);
    EXPECT_EQ (is.read_all (), "hello, world!\n");
  }

  {
    std::string yfile = tl::combine_path (b2copydir, "y");
    EXPECT_EQ (tl::file_exists (yfile), true);
    tl::InputStream is (yfile);
    EXPECT_EQ (is.read_all (), "hello, world II!\n");
  }
}

//  Secret mode switchers for testing
namespace tl
{
TL_PUBLIC void file_utils_force_windows ();
TL_PUBLIC void file_utils_force_linux ();
TL_PUBLIC void file_utils_force_reset ();
}

//  Fake Windows-tests (split_path, dirname, filename, basename, extension, combine_path)
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

    EXPECT_EQ (tl::normalize_path ("\\hello\\world"), "\\hello\\world");
    EXPECT_EQ (tl::normalize_path ("\\hello\\\\world\\"), "\\hello\\world");
    EXPECT_EQ (tl::normalize_path ("hello\\\\world\\"), "hello\\world");
    EXPECT_EQ (tl::normalize_path ("\\\\SERVER\\hello\\world"), "\\\\SERVER\\hello\\world");
    EXPECT_EQ (tl::normalize_path ("c:\\hello\\\\world\\"), "C:\\hello\\world");

    //  slashes are good too:
    EXPECT_EQ (tl::normalize_path ("/hello/world"), "\\hello\\world");
    EXPECT_EQ (tl::normalize_path ("/hello//world/"), "\\hello\\world");
    EXPECT_EQ (tl::normalize_path ("hello//world/"), "hello\\world");
    EXPECT_EQ (tl::normalize_path ("//SERVER/hello/world"), "\\\\SERVER\\hello\\world");
    EXPECT_EQ (tl::normalize_path ("c:/hello//world/"), "C:\\hello\\world");

    //  boundary cases
    EXPECT_EQ (tl::normalize_path (""), "");
    EXPECT_EQ (tl::normalize_path ("\\"), "\\");
    EXPECT_EQ (tl::normalize_path ("/"), "\\");
    EXPECT_EQ (tl::normalize_path ("d:"), "D:");
    EXPECT_EQ (tl::normalize_path ("/d:"), "D:");
    EXPECT_EQ (tl::normalize_path ("\\d:"), "D:");
    EXPECT_EQ (tl::normalize_path ("\\\\"), "\\\\");
    EXPECT_EQ (tl::normalize_path ("//"), "\\\\");
    EXPECT_EQ (tl::normalize_path ("d:\\"), "D:\\");
    EXPECT_EQ (tl::normalize_path ("d:\\\\"), "D:\\");
    EXPECT_EQ (tl::normalize_path ("d:/"), "D:\\");
    EXPECT_EQ (tl::normalize_path ("d://"), "D:\\");

    EXPECT_EQ (tl::dirname ("hello"), ".");
    EXPECT_EQ (tl::dirname (".\\hello"), ".");
    EXPECT_EQ (tl::dirname ("/hello"), "");
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

    EXPECT_EQ (tl::extension_last ("/hello/world"), "");
    EXPECT_EQ (tl::extension_last ("/hello/world.tar"), "tar");
    EXPECT_EQ (tl::extension_last ("/hello/world.tar.gz"), "gz");
    EXPECT_EQ (tl::extension_last ("\\hello\\.world"), "");
    EXPECT_EQ (tl::extension_last ("\\hello\\.world.gz"), "gz");
    EXPECT_EQ (tl::extension_last ("/hello//world/"), "");

    EXPECT_EQ (tl::is_absolute ("~/world"), true);
    EXPECT_EQ (tl::is_absolute ("~"), true);
    EXPECT_EQ (tl::is_absolute ("world"), false);
    EXPECT_EQ (tl::is_absolute ("world/"), false);
    EXPECT_EQ (tl::is_absolute ("hello//world/"), false);
    EXPECT_EQ (tl::is_absolute ("/hello//world/"), true);

    EXPECT_EQ (tl::combine_path ("hello", "world"), "hello\\world");
    EXPECT_EQ (tl::combine_path ("hello", ""), "hello");
    EXPECT_EQ (tl::combine_path ("hello", "", true), "hello\\");
    EXPECT_EQ (tl::combine_path ("", "hello", true), "\\hello");
    EXPECT_EQ (tl::combine_path (".", "hello", true), ".\\hello");

    EXPECT_EQ (tl::combine_path (tl::dirname ("hello"), tl::filename ("hello")), ".\\hello");
    EXPECT_EQ (tl::combine_path (tl::dirname ("\\hello"), tl::filename ("\\hello")), "\\hello");
    EXPECT_EQ (tl::combine_path (tl::dirname ("c:\\hello"), tl::filename ("c:\\hello")), "C:\\hello");
    EXPECT_EQ (tl::combine_path (tl::dirname ("\\c:\\hello"), tl::filename ("\\c:\\hello")), "C:\\hello");
    EXPECT_EQ (tl::combine_path (tl::dirname ("\\\\hello"), tl::filename ("\\\\hello")), "\\hello");
    EXPECT_EQ (tl::combine_path (tl::dirname ("\\\\server:\\hello"), tl::filename ("\\\\server:\\hello")), "\\\\server:\\hello");
    EXPECT_EQ (tl::combine_path (tl::dirname ("\\hello\\x"), tl::filename ("\\hello\\x")), "\\hello\\x");
    EXPECT_EQ (tl::combine_path (tl::dirname ("c:\\hello\\x"), tl::filename ("c:\\hello\\x")), "C:\\hello\\x");
    EXPECT_EQ (tl::combine_path (tl::dirname ("\\c:\\hello\\x"), tl::filename ("\\c:\\hello\\x")), "C:\\hello\\x");
    EXPECT_EQ (tl::combine_path (tl::dirname ("\\\\hello\\x"), tl::filename ("\\\\hello\\x")), "\\\\hello\\x");
    EXPECT_EQ (tl::combine_path (tl::dirname ("\\hello\\x\\y"), tl::filename ("\\hello\\x\\y")), "\\hello\\x\\y");

    EXPECT_EQ (tl::combine_path (tl::dirname ("hello/x"), tl::filename ("hello/x")), "hello\\x");
    EXPECT_EQ (tl::combine_path (tl::dirname ("/hello"), tl::filename ("/hello")), "\\hello");
    EXPECT_EQ (tl::combine_path (tl::dirname ("c:/hello"), tl::filename ("c:/hello")), "C:\\hello");
    EXPECT_EQ (tl::combine_path (tl::dirname ("/c:/hello"), tl::filename ("/c:/hello")), "C:\\hello");
    EXPECT_EQ (tl::combine_path (tl::dirname ("//hello"), tl::filename ("//hello")), "\\hello");
    EXPECT_EQ (tl::combine_path (tl::dirname ("//server:/hello"), tl::filename ("//server:/hello")), "\\\\server:\\hello");
    EXPECT_EQ (tl::combine_path (tl::dirname ("/hello/x"), tl::filename ("/hello/x")), "\\hello\\x");
    EXPECT_EQ (tl::combine_path (tl::dirname ("c:/hello/x"), tl::filename ("c:/hello/x")), "C:\\hello\\x");
    EXPECT_EQ (tl::combine_path (tl::dirname ("/c:/hello/x"), tl::filename ("/c:/hello/x")), "C:\\hello\\x");
    EXPECT_EQ (tl::combine_path (tl::dirname ("//hello/x"), tl::filename ("//hello/x")), "\\\\hello\\x");
    EXPECT_EQ (tl::combine_path (tl::dirname ("/hello/x/y"), tl::filename ("/hello/x/y")), "\\hello\\x\\y");
    EXPECT_EQ (tl::combine_path (tl::dirname ("hello/x"), tl::filename ("hello/x")), "hello\\x");

    tl::file_utils_force_reset ();

  } catch (...) {
    tl::file_utils_force_reset ();
    throw;
  }
}

//  Fake Linux-tests (split_path, dirname, filename, basename, extension, combine_path)
TEST (11)
{
  tl::file_utils_force_linux ();
  try {

    EXPECT_EQ (tl::join (tl::split_path ("/hello/world"), "+"), "/hello+/world");
    EXPECT_EQ (tl::join (tl::split_path ("/hel\\/\\\\lo/world"), "+"), "/hel\\/\\\\lo+/world");
    EXPECT_EQ (tl::join (tl::split_path ("/hello//world/"), "+"), "/hello+/world");
    EXPECT_EQ (tl::join (tl::split_path ("hello//world/"), "+"), "hello+/world");

    EXPECT_EQ (tl::normalize_path ("/hello/world"), "/hello/world");
    EXPECT_EQ (tl::normalize_path ("/hel\\/\\\\lo/world"), "/hel\\/\\\\lo/world");
    EXPECT_EQ (tl::normalize_path ("/hello//world/"), "/hello/world");
    EXPECT_EQ (tl::normalize_path ("hello//world/"), "hello/world");

    //  boundary cases
    EXPECT_EQ (tl::join (tl::split_path (""), "+"), "");
    EXPECT_EQ (tl::join (tl::split_path ("/"), "+"), "/");
    EXPECT_EQ (tl::join (tl::split_path ("//"), "+"), "/");

    EXPECT_EQ (tl::dirname ("hello"), ".");
    EXPECT_EQ (tl::dirname ("./hello"), ".");
    EXPECT_EQ (tl::dirname ("/hello"), "");
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
    EXPECT_EQ (tl::combine_path ("", "hello", true), "/hello");
    EXPECT_EQ (tl::combine_path (".", "hello", true), "./hello");

    EXPECT_EQ (tl::combine_path (tl::dirname ("hello"), tl::filename ("hello")), "./hello");
    EXPECT_EQ (tl::combine_path (tl::dirname ("/hello"), tl::filename ("/hello")), "/hello");
    EXPECT_EQ (tl::combine_path (tl::dirname ("/hello/x"), tl::filename ("/hello/x")), "/hello/x");
    EXPECT_EQ (tl::combine_path (tl::dirname ("/hello/x/y"), tl::filename ("/hello/x/y")), "/hello/x/y");
    EXPECT_EQ (tl::combine_path (tl::dirname ("hello/x"), tl::filename ("hello/x")), "hello/x");

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

//  mkpath
TEST(13)
{
  std::string tp = tl::absolute_file_path (tmp_file ());
  std::string tt = tl::combine_path (tp, "mkpathtest");
  std::string tta = tl::combine_path (tt, "a");
  std::string ttab = tl::combine_path (tta, "b");
  tl::rm_dir_recursive (tt);

  EXPECT_EQ (tl::file_exists (tt), false);
  EXPECT_EQ (tl::is_readable (tt), false);
  EXPECT_EQ (tl::is_writable (tt), false);
  EXPECT_EQ (tl::mkpath (tt), true);
  EXPECT_EQ (tl::file_exists (tt), true);
  EXPECT_EQ (tl::is_readable (tt), true);
  EXPECT_EQ (tl::is_writable (tt), true);
  tl::rm_dir_recursive (tt);

  EXPECT_EQ (tl::file_exists (tt), false);
  EXPECT_EQ (tl::mkpath (tta), true);
  EXPECT_EQ (tl::file_exists (tta), true);
  EXPECT_EQ (tl::file_exists (tt), true);
  tl::rm_dir_recursive (tt);
  EXPECT_EQ (tl::file_exists (tta), false);

  EXPECT_EQ (tl::file_exists (tt), false);
  EXPECT_EQ (tl::mkpath (ttab), true);
  EXPECT_EQ (tl::file_exists (ttab), true);
  EXPECT_EQ (tl::file_exists (tta), true);
  EXPECT_EQ (tl::file_exists (tt), true);
  tl::rm_dir_recursive (tt);
  EXPECT_EQ (tl::file_exists (ttab), false);
  EXPECT_EQ (tl::file_exists (tta), false);

  EXPECT_EQ (tl::file_exists (tt), false);
}

#if defined(HAVE_QT)
//  absolute_path vs. Qt
TEST(14)
{
  std::string xpath_qt = tl::to_string (QDir::current ().absoluteFilePath ("doesnotexist"));
  std::string xpath = tl::absolute_file_path ("doesnotexist");
  EXPECT_EQ (tl::replaced (xpath_qt, "\\", "/"), tl::replaced (xpath, "\\", "/"));

  std::string xpath2_qt = tl::to_string (QDir::current ().absolutePath ());
  std::string xpath2 = tl::absolute_path ("./doesnotexist");
  EXPECT_EQ (tl::replaced (xpath2_qt, "\\", "/"), tl::replaced (xpath2, "\\", "/"));

  xpath2 = tl::absolute_file_path (xpath2);
  EXPECT_EQ (tl::replaced (xpath2_qt, "\\", "/"), tl::replaced (xpath2, "\\", "/"));
}
#endif

//  relative_path and absolute_file_path
TEST(15)
{
  std::string xpath = tl::absolute_file_path ("doesnotexist");
  std::string xpath2 = tl::absolute_path ("./doesnotexist");

  EXPECT_EQ (tl::relative_path (xpath2, xpath2), "");
  EXPECT_EQ (tl::relative_path (xpath2, xpath), "doesnotexist");
  EXPECT_EQ (tl::replaced (tl::relative_path (xpath2, tl::combine_path (xpath, "a")), "\\", "/"), "doesnotexist/a");
}

//  dir_entries
TEST(16)
{
  std::string tp = tl::absolute_file_path (tmp_file ());
  std::string tt = tl::combine_path (tp, "detest");
  EXPECT_EQ (tl::mkpath (tt), true);

  {
    std::ofstream of (tl::combine_path (tt, "x").c_str ());
    of << "Hello, world!\n";
  }

  EXPECT_EQ (tl::file_exists (tl::combine_path (tt, "x")), true);
  EXPECT_EQ (tl::is_dir (tl::combine_path (tt, "x")), false);

  {
    std::ofstream of (tl::combine_path (tt, "y").c_str ());
    of << "Hello, world!\n";
  }

  EXPECT_EQ (tl::file_exists (tl::combine_path (tt, "y")), true);
  EXPECT_EQ (tl::is_dir (tl::combine_path (tt, "y")), false);

  {
    std::ofstream of (tl::combine_path (tt, ".z").c_str ());
    of << "Hello, world!\n";
  }

  EXPECT_EQ (tl::file_exists (tl::combine_path (tt, ".z")), true);
  EXPECT_EQ (tl::is_dir (tl::combine_path (tt, ".z")), false);

  EXPECT_EQ (tl::mkpath (tl::combine_path (tt, "u")), true);
  EXPECT_EQ (tl::file_exists (tl::combine_path (tt, "u")), true);
  EXPECT_EQ (tl::is_dir (tl::combine_path (tt, "u")), true);

  EXPECT_EQ (tl::mkpath (tl::combine_path (tt, "v")), true);
  EXPECT_EQ (tl::file_exists (tl::combine_path (tt, "v")), true);
  EXPECT_EQ (tl::is_dir (tl::combine_path (tt, "v")), true);

  EXPECT_EQ (tl::mkpath (tl::combine_path (tt, ".w")), true);
  EXPECT_EQ (tl::file_exists (tl::combine_path (tt, ".w")), true);
  EXPECT_EQ (tl::is_dir (tl::combine_path (tt, ".w")), true);

  std::vector<std::string> ee;
  ee = tl::dir_entries (tt, true, true);
  std::sort (ee.begin (), ee.end ());
  EXPECT_EQ (tl::join (ee, "+"), ".w+.z+u+v+x+y");

  ee = tl::dir_entries (tt, true, true, true);
  std::sort (ee.begin (), ee.end ());
  EXPECT_EQ (tl::join (ee, "+"), "u+v+x+y");

  ee = tl::dir_entries (tt, false, true);
  std::sort (ee.begin (), ee.end ());
  EXPECT_EQ (tl::join (ee, "+"), ".w+u+v");

  ee = tl::dir_entries (tt, false, true, true);
  std::sort (ee.begin (), ee.end ());
  EXPECT_EQ (tl::join (ee, "+"), "u+v");

  ee = tl::dir_entries (tt, true, false);
  std::sort (ee.begin (), ee.end ());
  EXPECT_EQ (tl::join (ee, "+"), ".z+x+y");

  ee = tl::dir_entries (tt, true, false, true);
  std::sort (ee.begin (), ee.end ());
  EXPECT_EQ (tl::join (ee, "+"), "x+y");

  EXPECT_EQ (tl::rm_file (tl::combine_path (tt, "xxx")), false);

  EXPECT_EQ (tl::rm_file (tl::combine_path (tt, "x")), true);

  ee = tl::dir_entries (tt, true, false);
  std::sort (ee.begin (), ee.end ());
  EXPECT_EQ (tl::join (ee, "+"), ".z+y");

  EXPECT_EQ (tl::rm_dir (tl::combine_path (tt, "u")), true);

  ee = tl::dir_entries (tt, false, true);
  std::sort (ee.begin (), ee.end ());
  EXPECT_EQ (tl::join (ee, "+"), ".w+v");

  EXPECT_EQ (tl::rm_dir (tt), false);  //  not empty
}

//  is_same_file
TEST (17)
{
  std::string currdir = tl::current_dir ();
  std::string currdir_abs = tl::absolute_file_path (".");
  EXPECT_EQ (currdir, currdir_abs);
  EXPECT_EQ (tl::is_same_file (currdir, "."), true);
  EXPECT_EQ (tl::is_same_file (".", currdir), true);

  std::string above = tl::absolute_file_path ("..");
  EXPECT_EQ (tl::is_same_file (currdir, above), false);
  EXPECT_EQ (tl::is_same_file (above, currdir), false);

  std::string tp = tl::absolute_file_path (tmp_file ());
  std::string dpath = tl::combine_path (tp, "d");
  EXPECT_EQ (tl::mkpath (dpath), true);
  std::string xfile = tl::combine_path (dpath, "x");
  std::string yfile = tl::combine_path (dpath, "y");
  {
    tl::OutputStream os (xfile);
    os << "hello, world!";
  }
  {
    tl::OutputStream os (yfile);
    os << "hello, world II!";
  }
  EXPECT_EQ (tl::file_exists (xfile), true);
  EXPECT_EQ (tl::is_same_file (xfile, tp), false);
  EXPECT_EQ (tl::is_same_file (dpath, xfile), false);
  EXPECT_EQ (tl::is_parent_path (dpath, xfile), true);
  EXPECT_EQ (tl::is_parent_path (xfile, dpath), false);

  EXPECT_EQ (tl::is_same_file (tl::combine_path (dpath, "../d/x"), xfile), true);
  EXPECT_EQ (tl::is_same_file (tl::combine_path (dpath, "../d/y"), xfile), false);
  EXPECT_EQ (tl::is_same_file (tl::combine_path (dpath, "../d/y"), yfile), true);
  EXPECT_EQ (tl::is_same_file (xfile, tl::combine_path (dpath, "../d/x")), true);
  EXPECT_EQ (tl::is_same_file (xfile, tl::combine_path (dpath, "../d/y")), false);
  EXPECT_EQ (tl::is_same_file (yfile, tl::combine_path (dpath, "../d/y")), true);
}

//  rename_file
TEST (18)
{
  std::string tp = tl::absolute_file_path (tmp_file ());
  std::string xfile = tl::combine_path (tp, "x");
  std::string yfile = tl::combine_path (tp, "y");
  std::string zfile = tl::combine_path (tl::combine_path (tp, "dir"), "z");

  tl::mkpath (tl::combine_path (tp, "dir"));

  {
    tl::OutputStream os (xfile);
    os << "hello, world!\n";
  }

  EXPECT_EQ (tl::file_exists (xfile), true);
  EXPECT_EQ (tl::file_exists (yfile), false);
  EXPECT_EQ (tl::file_exists (zfile), false);

  {
    tl::InputStream is (xfile);
    EXPECT_EQ (is.read_all (), "hello, world!\n");
  }

  tl::rename_file (xfile, yfile);

  EXPECT_EQ (tl::file_exists (xfile), false);
  EXPECT_EQ (tl::file_exists (yfile), true);
  EXPECT_EQ (tl::file_exists (zfile), false);

  {
    tl::InputStream is (yfile);
    EXPECT_EQ (is.read_all (), "hello, world!\n");
  }

  tl::rename_file (yfile, "x");

  EXPECT_EQ (tl::file_exists (xfile), true);
  EXPECT_EQ (tl::file_exists (yfile), false);
  EXPECT_EQ (tl::file_exists (zfile), false);

  {
    tl::InputStream is (xfile);
    EXPECT_EQ (is.read_all (), "hello, world!\n");
  }

  tl::rename_file (xfile, zfile);

  EXPECT_EQ (tl::file_exists (xfile), false);
  EXPECT_EQ (tl::file_exists (yfile), false);
  EXPECT_EQ (tl::file_exists (zfile), true);

  {
    tl::InputStream is (zfile);
    EXPECT_EQ (is.read_all (), "hello, world!\n");
  }

  tl::rename_file (zfile, xfile);

  EXPECT_EQ (tl::file_exists (xfile), true);
  EXPECT_EQ (tl::file_exists (yfile), false);
  EXPECT_EQ (tl::file_exists (zfile), false);

  {
    tl::InputStream is (xfile);
    EXPECT_EQ (is.read_all (), "hello, world!\n");
  }

  tl::rename_file (xfile, tl::combine_path ("dir", "z"));

  EXPECT_EQ (tl::file_exists (xfile), false);
  EXPECT_EQ (tl::file_exists (yfile), false);
  EXPECT_EQ (tl::file_exists (zfile), true);

  {
    tl::InputStream is (zfile);
    EXPECT_EQ (is.read_all (), "hello, world!\n");
  }

  //  rename directory
  tl::rename_file (tl::combine_path (tp, "dir"), "dirx");

  EXPECT_EQ (tl::file_exists (tl::combine_path (tp, "dir")), false);
  EXPECT_EQ (tl::file_exists (tl::combine_path (tp, "dirx")), true);
  EXPECT_EQ (tl::is_dir (tl::combine_path (tp, "dirx")), true);
}

//  get_home_path
TEST (19)
{
  std::string home = tl::get_home_path ();
  //  no specific value, just something ...
  EXPECT_EQ (home.size () > 5, true);
#if defined(HAVE_QT)
  EXPECT_EQ (tl::replaced (home, "\\", "/"), tl::replaced (tl::to_string (QDir::homePath ()), "\\", "/"));
#endif
}

//  absolute path with "~" expansion
TEST (20)
{
  EXPECT_EQ (tl::absolute_file_path ("~"), tl::get_home_path ());
  EXPECT_EQ (tl::absolute_file_path (tl::combine_path ("~", "test")), tl::combine_path (tl::get_home_path (), "test"));
}

//  tmpfile
TEST (21)
{
  std::string p = tl::tmpfile ("tl_tests");
  EXPECT_EQ (tl::file_exists (p), true);

  std::ofstream os (p);
  os << "A test";
  os.close ();

  {
    tl::InputStream is (p);
    EXPECT_EQ (is.read_all (), "A test");
  }

  EXPECT_EQ (tl::rm_file (p), true);
  EXPECT_EQ (tl::file_exists (p), false);
}

//  TemporaryFile
TEST (22)
{
  std::string p;

  {
    tl::TemporaryFile tf ("tl_tests");
    p = tf.path ();
    EXPECT_EQ (tl::file_exists (tf.path ()), true);

    std::ofstream os (tf.path ());
    os << "A test";
    os.close ();

    tl::InputStream is (tf.path ());
    EXPECT_EQ (is.read_all (), "A test");
  }

  EXPECT_EQ (tl::file_exists (p), false);
}

//  tmpdir
TEST (23)
{
  std::string p = tl::tmpdir ("tl_tests");
  EXPECT_EQ (tl::file_exists (p), true);
  EXPECT_EQ (tl::is_dir (p), true);

  std::ofstream os (tl::combine_path (p, "test"));
  os << "A test";
  os.close ();

  {
    tl::InputStream is (tl::combine_path (p, "test"));
    EXPECT_EQ (is.read_all (), "A test");
  }

  EXPECT_EQ (tl::rm_dir_recursive (p), true);
  EXPECT_EQ (tl::file_exists (p), false);
}

//  TemporaryDirectory object
TEST (24)
{
  std::string p;

  {
    tl::TemporaryDirectory tmpdir ("tl_tests");
    p = tmpdir.path ();

    EXPECT_EQ (tl::file_exists (p), true);
    EXPECT_EQ (tl::is_dir (p), true);

    std::ofstream os (tl::combine_path (p, "test"));
    os << "A test";
    os.close ();

    tl::InputStream is (tl::combine_path (p, "test"));
    EXPECT_EQ (is.read_all (), "A test");
  }

  EXPECT_EQ (tl::file_exists (p), false);
}

//  glob_expand
TEST (25)
{
  tl::TemporaryDirectory tmpdir ("tl_tests");
  auto p = tmpdir.path ();

  auto ad = tl::combine_path (p, "a");
  tl::mkpath (ad);
  auto aad = tl::combine_path (ad, "a");
  tl::mkpath (aad);
  auto aaad = tl::combine_path (aad, "a");
  tl::mkpath (aaad);
  auto bd = tl::combine_path (p, "b");
  tl::mkpath (bd);

  {
    std::ofstream os (tl::combine_path (ad, "test.txt"));
    os << "A test";
    os.close ();
  }

  {
    std::ofstream os (tl::combine_path (aad, "test.txt"));
    os << "A test";
    os.close ();
  }

  {
    std::ofstream os (tl::combine_path (aaad, "test.txt"));
    os << "A test";
    os.close ();
  }

  {
    std::ofstream os (tl::combine_path (aaad, "test2.txt"));
    os << "A test";
    os.close ();
  }

  {
    std::ofstream os (tl::combine_path (bd, "test.txt"));
    os << "A test";
    os.close ();
  }

  {
    std::ofstream os (tl::combine_path (p, "test2.txt"));
    os << "A test";
    os.close ();
  }

  std::vector<std::string> au;

  auto res = tl::glob_expand (tl::combine_path (p, "*.txt"));
  au.push_back (tl::combine_path (p, "test2.txt"));

  std::sort (res.begin (), res.end ());
  std::sort (au.begin (), au.end ());
  EXPECT_EQ (tl::join (res, "\n"), tl::join (au, "\n"));

  res = tl::glob_expand (tl::combine_path (tl::combine_path (p, "**"), "*.txt"));
  au.clear ();
  au.push_back (tl::combine_path (p, "test2.txt"));
  au.push_back (tl::combine_path (ad, "test.txt"));
  au.push_back (tl::combine_path (aad, "test.txt"));
  au.push_back (tl::combine_path (aaad, "test.txt"));
  au.push_back (tl::combine_path (aaad, "test2.txt"));
  au.push_back (tl::combine_path (bd, "test.txt"));

  std::sort (res.begin (), res.end ());
  std::sort (au.begin (), au.end ());
  EXPECT_EQ (tl::join (res, "\n"), tl::join (au, "\n"));

  res = tl::glob_expand (tl::combine_path (tl::combine_path (p, "**"), "*2.txt"));
  au.clear ();
  au.push_back (tl::combine_path (p, "test2.txt"));
  au.push_back (tl::combine_path (aaad, "test2.txt"));

  std::sort (res.begin (), res.end ());
  std::sort (au.begin (), au.end ());
  EXPECT_EQ (tl::join (res, "\n"), tl::join (au, "\n"));

  res = tl::glob_expand (tl::combine_path (tl::combine_path (tl::combine_path (p, "**"), "a"), "*2.txt"));
  au.clear ();
  au.push_back (tl::combine_path (aaad, "test2.txt"));

  std::sort (res.begin (), res.end ());
  std::sort (au.begin (), au.end ());
  EXPECT_EQ (tl::join (res, "\n"), tl::join (au, "\n"));
}

