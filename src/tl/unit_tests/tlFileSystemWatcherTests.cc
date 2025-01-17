
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

#if defined(HAVE_QT)

#include "tlFileSystemWatcher.h"
#include "tlString.h"
#include "tlTimer.h"
#include "tlUnitTest.h"

#include <QFileInfo>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QFile>
#include <QDir>

#include <set>

void wait_for_ms (int ms)
{
  tl::Clock start = tl::Clock::current ();
  while ((tl::Clock::current () - start).seconds () < ms * 0.001) {
    QCoreApplication::processEvents ();
  }
}

TEST(1)
{
  QList<QVariant> args;

  std::string f1 = tmp_file ("a");
  QFile f1_file (tl::to_qstring (f1));

  f1_file.open (QFile::WriteOnly);
  f1_file.write ("first line.\n");
  f1_file.close ();

  tl::FileSystemWatcher w;
  QSignalSpy changed_spy (&w, SIGNAL (fileChanged (const QString &)));
  QSignalSpy removed_spy (&w, SIGNAL (fileRemoved (const QString &)));

  w.add_file (f1);

  QFileInfo fi (tl::to_qstring (f1));
  EXPECT_EQ (fi.exists (), true);

  f1_file.open (QFile::WriteOnly);
  f1_file.write ("something.");
  f1_file.close ();

  //  make sure the events get processed
  wait_for_ms (300);

  //  should have modified the file
  EXPECT_EQ (changed_spy.count (), 1);
  EXPECT_EQ (removed_spy.count (), 0);
  args = changed_spy.takeFirst ();
  EXPECT_EQ (tl::to_string (args.at (0).toString ()), f1);
  EXPECT_EQ (changed_spy.count (), 0);

  EXPECT_EQ (f1_file.remove (), true);

  //  make sure the events get processed
  wait_for_ms (300);

  EXPECT_EQ (changed_spy.count (), 0);
  EXPECT_EQ (removed_spy.count (), 1);
  args = removed_spy.takeFirst ();
  EXPECT_EQ (tl::to_string (args.at (0).toString ()), f1);
  EXPECT_EQ (removed_spy.count (), 0);
}

TEST(2)
{
  QList<QVariant> args;

  std::string d1 = tmp_file ("dir");
  QFileInfo d1_fi (tl::to_qstring (d1));

  QDir (d1_fi.path ()).mkdir (d1_fi.fileName ());

  tl::FileSystemWatcher w;
  QSignalSpy changed_spy (&w, SIGNAL (fileChanged (const QString &)));
  QSignalSpy removed_spy (&w, SIGNAL (fileRemoved (const QString &)));

  w.add_file (d1);  //  actually a dir

  {
    QFileInfo fi (tl::to_qstring (d1));
    EXPECT_EQ (fi.exists (), true);
    EXPECT_EQ (fi.isDir (), true);
  }

  //  make sure the events get processed
  wait_for_ms (200);

  EXPECT_EQ (changed_spy.count (), 0);
  EXPECT_EQ (removed_spy.count (), 0);

  //  This is required to make the file timestamps between mkdir and file writing differ
  wait_for_ms (1100);

  QFile dir_file (QDir (tl::to_qstring (d1)).filePath (tl::to_qstring ("x")));
  dir_file.open (QFile::WriteOnly);
  dir_file.write ("something.");
  dir_file.close ();

  wait_for_ms (200);

  //  should have modified the directory
  EXPECT_EQ (changed_spy.count (), 1);
  EXPECT_EQ (removed_spy.count (), 0);
  args = changed_spy.takeFirst ();
  EXPECT_EQ (tl::to_string (args.at (0).toString ()), d1);
  EXPECT_EQ (changed_spy.count (), 0);

  //  This is required to make the file timestamps between mkdir and file writing differ
  wait_for_ms (1100);

  EXPECT_EQ (dir_file.remove (), true);

  //  make sure the events get processed
  wait_for_ms (200);

  //  should have modified the directory
  EXPECT_EQ (changed_spy.count (), 1);
  EXPECT_EQ (removed_spy.count (), 0);
  args = changed_spy.takeFirst ();
  EXPECT_EQ (tl::to_string (args.at (0).toString ()), d1);
  EXPECT_EQ (changed_spy.count (), 0);

  EXPECT_EQ (QDir (d1_fi.path ()).rmdir (d1_fi.fileName ()), true);

  //  make sure the events get processed
  wait_for_ms (200);

  EXPECT_EQ (changed_spy.count (), 0);
  EXPECT_EQ (removed_spy.count (), 1);
  args = removed_spy.takeFirst ();
  EXPECT_EQ (tl::to_string (args.at (0).toString ()), d1);
  EXPECT_EQ (removed_spy.count (), 0);
}

TEST(3)
{
  //  5k files
  QList<QVariant> args;

  tl::FileSystemWatcher w;
  QSignalSpy changed_spy (&w, SIGNAL (fileChanged (const QString &)));
  QSignalSpy removed_spy (&w, SIGNAL (fileRemoved (const QString &)));

  for (int i = 0; i < 5000; ++i) {

    std::string f1 = tmp_file (tl::sprintf ("a%d", i));
    QFile f1_file (tl::to_qstring (f1));

    f1_file.open (QFile::WriteOnly);
    f1_file.write ("first line.\n");
    f1_file.close ();

    w.add_file (f1);

  }

  //  make sure the events get processed
  wait_for_ms (5000);

  //  should have modified the file
  EXPECT_EQ (changed_spy.count (), 0);
  EXPECT_EQ (removed_spy.count (), 0);

  std::set<std::string> changed_files;

  for (int i = 0; i < 5000; i += 100) {

    std::string f1 = tmp_file (tl::sprintf ("a%d", i));
    changed_files.insert (f1);

    QFile f1_file (tl::to_qstring (f1));

    f1_file.open (QFile::WriteOnly);
    f1_file.write ("something");
    f1_file.close ();

  }

  //  make sure the events get processed
  wait_for_ms (5000);

  //  should have modified the file
  EXPECT_EQ (changed_spy.count (), 50);
  EXPECT_EQ (removed_spy.count (), 0);
  while (changed_spy.count () > 0) {
    args = changed_spy.takeFirst ();
    std::string p = tl::to_string (args.at (0).toString ());
    EXPECT_EQ (changed_files.find (p) != changed_files.end (), true);
    changed_files.erase (p);
  }
  EXPECT_EQ (changed_spy.count (), 0);
}

#endif
