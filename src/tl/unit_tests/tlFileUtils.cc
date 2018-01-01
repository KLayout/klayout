
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
  EXPECT_EQ (tl::rm_dir_recursive (adir.absolutePath ()), true);
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
  EXPECT_EQ (tl::rm_dir_recursive (adir.absolutePath ()), true);
  EXPECT_EQ (adir.exists (), false);
  EXPECT_EQ (b1dir.exists (), false);
  EXPECT_EQ (b2dir.exists (), false);
  EXPECT_EQ (QFileInfo (b2dir.absoluteFilePath (QString::fromUtf8 ("x"))).exists (), false);
}

TEST (3)
{
  QDir tmp_dir = QFileInfo (tl::to_qstring (tmp_file ())).absoluteDir ();

  tl::rm_dir_recursive (tmp_dir.filePath (QString::fromUtf8 ("a")));
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

  tl::rm_dir_recursive (tmp_dir.filePath (QString::fromUtf8 ("acopy")));
  tmp_dir.mkdir (QString::fromUtf8 ("acopy"));

  tl::cp_dir_recursive (tmp_dir.absoluteFilePath (QString::fromUtf8 ("a")), tmp_dir.absoluteFilePath (QString::fromUtf8 ("acopy")));

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
