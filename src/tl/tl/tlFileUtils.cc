
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

#include "tlFileUtils.h"
#include "tlLog.h"
#include "tlInternational.h"

#include <QDir>
#include <QFileInfo>

namespace tl
{

bool
is_parent_path (const QString &parent, const QString &path)
{
  QFileInfo parent_info (parent);
  if (! parent_info.exists ()) {
    //  If the parent path does not exist, we always return false. This cannot be a parent.
    return false;
  }

  QFileInfo path_info (path);

  while (parent_info != path_info) {
    if (path_info.isRoot ()) {
      return false;
    }
    path_info = path_info.path ();
  }

  return true;
}

bool
rm_dir_recursive (const QString &path)
{
  QDir dir (path);

  QStringList entries = dir.entryList (QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
  for (QStringList::const_iterator e = entries.begin (); e != entries.end (); ++e) {
    QFileInfo fi (dir.absoluteFilePath (*e));
    if (fi.isDir ()) {
      if (! rm_dir_recursive (fi.filePath ())) {
        return false;
      }
    } else if (fi.isFile ()) {
      if (! dir.remove (*e)) {
        tl::error << QObject::tr ("Unable to remove file: %1").arg (dir.absoluteFilePath (*e));
        return false;
      }
    }
  }

  QString name = dir.dirName ();
  if (dir.cdUp ()) {
    if (! dir.rmdir (name)) {
      tl::error << QObject::tr ("Unable to remove directory: %1").arg (dir.absoluteFilePath (name));
      return false;
    }
  }

  return true;
}

bool
cp_dir_recursive (const QString &source, const QString &target)
{
  QDir dir (source);
  QDir dir_target (target);

  QStringList entries = dir.entryList (QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs);
  for (QStringList::const_iterator e = entries.begin (); e != entries.end (); ++e) {

    QFileInfo fi (dir.absoluteFilePath (*e));
    QFileInfo fi_target (dir_target.absoluteFilePath (*e));

    if (fi.isDir ()) {

      //  Copy subdirectory
      if (! fi_target.exists ()) {
        if (! dir_target.mkdir (*e)) {
          tl::error << QObject::tr ("Unable to create target directory: %1").arg (dir_target.absoluteFilePath (*e));
          return false;
        }
      } else if (! fi_target.isDir ()) {
        tl::error << QObject::tr ("Unable to create target directory (is a file already): %1").arg (dir_target.absoluteFilePath (*e));
        return false;
      }
      if (! cp_dir_recursive (fi.filePath (), fi_target.filePath ())) {
        return false;
      }

    //  TODO: leave symlinks symlinks? How to copy symlinks with Qt?
    } else if (fi.isFile ()) {

      QFile file (fi.filePath ());
      QFile file_target (fi_target.filePath ());

      if (! file.open (QIODevice::ReadOnly)) {
        tl::error << QObject::tr ("Unable to open source file for reading: %1").arg (fi.filePath ());
        return false;
      }
      if (! file_target.open (QIODevice::WriteOnly)) {
        tl::error << QObject::tr ("Unable to open target file for writing: %1").arg (fi_target.filePath ());
        return false;
      }

      size_t chunk_size = 64 * 1024;

      while (! file.atEnd ()) {
        QByteArray data = file.read (chunk_size);
        file_target.write (data);
      }

      file.close ();
      file_target.close ();

    }

  }

  return true;
}

}
