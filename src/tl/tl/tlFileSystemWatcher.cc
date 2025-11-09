
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
#include "tlLog.h"

#include <QFileInfo>
#include <QTimer>
#include <list>

namespace tl
{

//  The global enable counter (<0: disable)
static int s_global_enable = 0;

//  The maximum allowed processing time in seconds
const double processing_time = 0.02;

FileSystemWatcher::FileSystemWatcher (QObject *parent)
  : QObject (parent)
{
  m_timer = new QTimer (this);
  connect (m_timer, SIGNAL (timeout ()), this, SLOT (timeout ()));
  m_timer->setSingleShot (false);
  m_timer->setInterval (100);
  m_timer->start ();
  m_index = 0;
  m_iter = m_files.end ();
  m_batch_size = 1000;
}

void
FileSystemWatcher::global_enable (bool en)
{
  if (en) {
    ++s_global_enable;
  } else {
    --s_global_enable;
  }
}

void
FileSystemWatcher::enable (bool en)
{
  if (en) {
    m_timer->start ();
  } else {
    m_timer->stop ();
  }
}

void
FileSystemWatcher::clear ()
{
  m_files.clear ();
  m_files_removed.clear ();
  m_iter = m_files.begin ();
  m_index = 0;
}

void
FileSystemWatcher::set_batch_size( size_t n)
{
  m_batch_size = n;
}

void
FileSystemWatcher::add_file (const std::string &path)
{
  if (path.empty ()) {
    return;
  }

  m_files_removed.erase (path);

  size_t size = 0;
  QDateTime time;

  QFileInfo fi (tl::to_qstring (path));
  if (! fi.exists () || ! fi.isReadable ()) {
    return;
  }

  size = size_t (fi.size ());
  time = fi.lastModified ();

  std::map<std::string, FileEntry>::iterator i = m_files.find (path);
  if (i != m_files.end ()) {
    i->second.refcount += 1;
    i->second.size = size;
    i->second.time = time;
  } else {
    m_files.insert (std::make_pair (path, FileEntry (1, size, time)));
    if (tl::verbosity () >= 30) {
      tl::info << tl::to_string (tr ("Start watching file: ")) << path;
    }
  }

  m_iter = m_files.begin ();
  m_index = 0;
}

void
FileSystemWatcher::remove_file (const std::string &path)
{
  if (path.empty ()) {
    return;
  }

  m_files_removed.erase (path);

  std::map<std::string, FileEntry>::iterator i = m_files.find (path);
  if (i != m_files.end () && --(i->second.refcount) <= 0) {
    m_files.erase (i);
    m_iter = m_files.begin ();
    m_index = 0;
    if (tl::verbosity () >= 30) {
      tl::info << tl::to_string (tr ("Stop watching file: ")) << path;
    }
  }
}

void
FileSystemWatcher::timeout ()
{
  if (s_global_enable < 0) {
    return;
  }

  tl::Clock start = tl::Clock::current ();

  if (m_iter == m_files.end ()) {
    m_iter = m_files.begin ();
    m_index = 0;
  }

  size_t i0 = m_index;

  std::list<std::string> files_removed, files_changed;

  while (m_index < i0 + m_batch_size && m_iter != m_files.end () && (tl::Clock::current () - start).seconds () < processing_time) {

    QFileInfo fi (tl::to_qstring (m_iter->first));
    if (! fi.exists ()) {

      if (m_files_removed.find (m_iter->first) == m_files_removed.end ()) {
        files_removed.push_back (m_iter->first);
        m_files_removed.insert (m_iter->first);
      }

      // don't drop files that temporarily does not exist - keep them monitored.
      // This way, programs that delete and write the file will not mess with the 
      // file watcher.
#if 0 
      std::map<std::string, FileEntry>::iterator i = m_iter;
      ++m_iter;
      m_files.erase (i);
#else
      ++m_iter;
#endif

    } else {

      m_files_removed.erase (m_iter->first);

      size_t size = size_t (fi.size ());
      QDateTime time = fi.lastModified ();

      if (m_iter->second.size != size || m_iter->second.time != time) {
        files_changed.push_back (m_iter->first);
      }

      m_iter->second.size = size;
      m_iter->second.time = time;

      ++m_iter;

    }

    ++m_index;

  }

  for (std::list<std::string>::const_iterator i = files_removed.begin (); i != files_removed.end (); ++i) {
    if (tl::verbosity () >= 40) {
      tl::info << tl::to_string (tr ("File removed: ")) << *i;
    }
    file_removed (*i);
    emit fileRemoved (tl::to_qstring (*i));
  }

  for (std::list<std::string>::const_iterator i = files_changed.begin (); i != files_changed.end (); ++i) {
    if (tl::verbosity () >= 40) {
      tl::info << tl::to_string (tr ("File changed: ")) << *i;
    }
    file_changed (*i);
    emit fileChanged (tl::to_qstring (*i));
  }
}

}
#endif
