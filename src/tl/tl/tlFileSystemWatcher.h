
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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

#ifndef HDR_tlFileSystemWatcher
#define HDR_tlFileSystemWatcher

#if !defined(HAVE_QT)
# error tl::FileSystemWatcher not available
#endif

#include "tlEvents.h"

#include <QObject>
#include <QDateTime>

#include <map>
#include <string>

class QTimer;

namespace tl
{

/**
 *  @brief A file system watcher, similar to QFileSystemWatcher
 *  The advantage of this object is to be less resource intensive.
 *  Thousands of files can be added, but delay between detection of file
 *  system updates and signals can increase.
 */
class TL_PUBLIC FileSystemWatcher
  : public QObject
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  FileSystemWatcher (QObject *parent = 0);

  /**
   *  @brief Global enable/disable
   */
  static void global_enable (bool en);

  /**
   *  @brief Enables or disables the file watcher
   */
  void enable (bool en);

  /**
   *  @brief Sets the batch size (the number of files checked per timer event)
   *  The default batch size is 100.
   */
  void set_batch_size (size_t n);

  /**
   *  @brief Gets the batch size
   */
  size_t batch_size () const
  {
    return m_batch_size;
  }

  /**
   *  @brief Clears the file watcher
   */
  void clear ();

  /**
   *  @brief Adds a file for being watched
   *  Files can be added multiple times. In that case they need to be removed the same
   *  number of times before they really disappear.
   */
  void add_file (const std::string &path);

  /**
   *  @brief Removes a file from the list of files to watch
   *  See \add_file for details.
   */
  void remove_file (const std::string &path);

  /**
   *  @brief This event is triggered after the given file has changed
   */
  tl::event<const std::string &> file_changed;

  /**
   *  @brief This event is triggered after the given file has been removed
   */
  tl::event<const std::string &> file_removed;

signals:
  /**
   *  @brief An alternative, Qt-style signal for file_removed
   */
  void fileRemoved (const QString &path);

  /**
   *  @brief An alternative, Qt-style signal for file_changed
   */
  void fileChanged (const QString &path);

private slots:
  void timeout ();

private:

  struct FileEntry {
    FileEntry () : refcount (0), size (0) { }
    FileEntry (int r, size_t s, const QDateTime &t) : refcount (r), size (s), time (t) { }
    int refcount;
    size_t size;
    QDateTime time;
  };

  QTimer *m_timer;
  size_t m_batch_size;
  std::map<std::string, FileEntry> m_files;
  size_t m_index;
  std::map<std::string, FileEntry>::iterator m_iter;
};

/**
 *  @brief A class employing RIIA for locking the file system watcher
 */
class TL_PUBLIC FileSystemWatcherDisabled
{
public:
  FileSystemWatcherDisabled ()
  {
    tl::FileSystemWatcher::global_enable (false);
  }

  ~FileSystemWatcherDisabled ()
  {
    tl::FileSystemWatcher::global_enable (true);
  }
};

}

#endif

