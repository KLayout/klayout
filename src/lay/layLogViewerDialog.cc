
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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


#include "layLogViewerDialog.h"

#include <QMutex>
#include <QMutexLocker>
#include <QTimer>
#include <QClipboard>

#include <stdio.h>

namespace lay
{

// -----------------------------------------------------------------
//  LogReceiver implementation

LogReceiver::LogReceiver (LogFile *file, int verbosity, void (LogFile::*method)(const std::string &, bool)) 
    : mp_file (file), m_method (method), m_continued (false), m_verbosity (verbosity)
{ 
  // .. nothing yet ..  
}

void 
LogReceiver::puts (const char *s) 
{ 
  if (tl::verbosity () >= m_verbosity) {

    while (*s) {

      const char *s0 = s;
      while (*s && *s != '\n') {
        ++s;
      }

      {
        QMutexLocker locker (&m_lock);
        m_text += std::string (s0, s - s0); 
      }

      if (*s == '\n') {
        endl ();
        ++s;
      }

    }

  }
}

void 
LogReceiver::endl () 
{ 
  if (tl::verbosity () >= m_verbosity) {
    QMutexLocker locker (&m_lock);
    (mp_file->*m_method) (m_text, m_continued);
    m_text.clear ();
    m_continued = true;
  }
}

void 
LogReceiver::end () 
{ 
  //  .. nothing yet ..
}

void 
LogReceiver::begin () 
{ 
  QMutexLocker locker (&m_lock);
  m_continued = false;
  m_text.clear ();
}

// -----------------------------------------------------------------
//  LogFile implementation

LogFile::LogFile (size_t max_entries)
  : m_error_receiver (this, 0, &LogFile::error),
    m_warn_receiver (this, 0, &LogFile::warn),
    m_log_receiver (this, 10, &LogFile::info),
    m_info_receiver (this, 0, &LogFile::info),
    m_max_entries (max_entries),
    m_generation_id (0), 
    m_last_generation_id (0)
{
  connect (&m_timer, SIGNAL (timeout ()), this, SLOT (timeout ()));

  m_timer.setSingleShot (true);
  m_timer.setInterval (100);
  m_timer.start ();

  tl::info.add (&m_info_receiver, false);
  tl::log.add (&m_log_receiver, false);
  tl::error.add (&m_error_receiver, false);
  tl::warn.add (&m_warn_receiver, false);
}

void
LogFile::clear ()
{
  QMutexLocker locker (&m_lock);

  m_messages.clear ();
  ++m_generation_id;
}

void
LogFile::separator ()
{
  m_lock.lock ();
  bool has_separator = false;
  if (m_messages.size () > 0 && m_messages.back ().mode () == LogFileEntry::Separator) {
    has_separator = true;
  }
  m_lock.unlock ();

  if (! has_separator) {
    add (LogFileEntry::Separator, tl::to_string (QObject::tr ("<-- New section -->")), false);
  }
}

void 
LogFile::copy ()
{
  QMutexLocker locker (&m_lock);

  QString text;
  for (std::deque<LogFileEntry>::const_iterator m = m_messages.begin (); m != m_messages.end (); ++m) {
    text += tl::to_qstring (m->text ());
    text += QString::fromUtf8 ("\n");
  }
  QApplication::clipboard ()->setText (text);
}

void 
LogFile::timeout ()
{
  bool changed = false;

  m_lock.lock ();
  if (m_generation_id != m_last_generation_id) {
    m_last_generation_id = m_generation_id;
    changed = true;
  }
  m_lock.unlock ();

  if (changed) {
    emit layoutChanged ();
  }

  m_timer.start ();
}

void 
LogFile::add (LogFileEntry::mode_type mode, const std::string &msg, bool continued)
{
  QMutexLocker locker (&m_lock);

  if (m_messages.size () >= m_max_entries) {
    m_messages.pop_front ();
  }

  m_messages.push_back (LogFileEntry (mode, msg, continued));

  ++m_generation_id;
}

int 
LogFile::rowCount(const QModelIndex & /*parent*/) const
{
  QMutexLocker locker (&m_lock);

  return int (m_messages.size ());
}

QVariant 
LogFile::data(const QModelIndex &index, int role) const
{
  QMutexLocker locker (&m_lock);

  if (role == Qt::DisplayRole) {

    if (index.row () < int (m_messages.size ()) && index.row () >= 0) {
      LogFileEntry::mode_type mode = m_messages [index.row ()].mode ();
      std::string message = m_messages [index.row ()].text ();
      if (mode == LogFileEntry::Error) {
        return QVariant (tl::to_qstring (tl::to_string (QObject::tr ("ERROR: ")) + message));
      } else if (mode == LogFileEntry::Warning) {
        return QVariant (tl::to_qstring (tl::to_string (QObject::tr ("Warning: ")) + message));
      } else {
        return QVariant (tl::to_qstring (message));
      }
    } 

  } else if (role == Qt::FontRole) {

    if (index.row () < int (m_messages.size ()) && index.row () >= 0) {
      LogFileEntry::mode_type mode = m_messages [index.row ()].mode ();
      if (mode == LogFileEntry::Error || mode == LogFileEntry::ErrorContinued) {
        QFont f;
        f.setBold (true);
        return QVariant (f);
      } else if (mode == LogFileEntry::Separator) {
        QFont f;
        f.setItalic (true);
        return QVariant (f);
      }
    } 

  } else if (role == Qt::ForegroundRole) {

    if (index.row () < int (m_messages.size ()) && index.row () >= 0) {
      LogFileEntry::mode_type mode = m_messages [index.row ()].mode ();
      if (mode == LogFileEntry::Separator) {
        return QColor (0, 255, 0);
      } else if (mode == LogFileEntry::Error || mode == LogFileEntry::ErrorContinued) {
        return QColor (255, 0, 0);
      } else if (mode == LogFileEntry::Warning || mode == LogFileEntry::WarningContinued) {
        return QColor (0, 0, 255);
      }
    } 

  } 

  return QVariant ();
}

// -----------------------------------------------------------------
//  LogViewerDialog implementation

LogViewerDialog::LogViewerDialog (QWidget *parent)
  : QDialog (parent), 
    m_file (50000)  //  TODO: make this variable ..
{
  setupUi (this);

  log_view->setModel (&m_file);

  verbosity_cbx->setCurrentIndex (std::min (4, tl::verbosity () / 10));

  connect (&m_file, SIGNAL (layoutChanged ()), log_view, SLOT (scrollToBottom ()));
  connect (verbosity_cbx, SIGNAL (currentIndexChanged (int)), this, SLOT (verbosity_changed (int)));
  connect (clear_pb, SIGNAL (clicked ()), &m_file, SLOT (clear ()));
  connect (separator_pb, SIGNAL (clicked ()), &m_file, SLOT (separator ()));
  connect (copy_pb, SIGNAL (clicked ()), &m_file, SLOT (copy ()));
}

void
LogViewerDialog::verbosity_changed (int index)
{
  tl::verbosity (index * 10 + 1);
}

}

