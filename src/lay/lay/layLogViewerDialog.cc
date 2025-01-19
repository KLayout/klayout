
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


#include "layLogViewerDialog.h"
#include "layApplication.h"

#include <QMutex>
#include <QMutexLocker>
#include <QTimer>
#include <QClipboard>
#include <QFrame>
#include <QThread>
#include <QAbstractEventDispatcher>

#include <stdio.h>

namespace lay
{

// -----------------------------------------------------------------
//  LogReceiver implementation

LogReceiver::LogReceiver (LogFile *file, int verbosity, void (LogFile::*method)(const std::string &, bool)) 
    : mp_file (file), m_method (method), m_verbosity (verbosity)
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
        QMutexLocker locker (&m_lock);
        (mp_file->*m_method) (m_text, true);
        m_text.clear ();
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
    (mp_file->*m_method) (m_text, false);
    m_text.clear ();
  }
}

void
LogReceiver::yield ()
{
  mp_file->yield ();
}

void 
LogReceiver::end () 
{ 
  //  .. nothing yet ..
}

void 
LogReceiver::begin () 
{ 
  //  .. nothing yet ..
}

// -----------------------------------------------------------------
//  LogFile implementation

LogFile::LogFile (size_t max_entries, bool register_global)
  : m_error_receiver (this, -10, &LogFile::add_error),
    m_warn_receiver (this, 0, &LogFile::add_warn),
    m_log_receiver (this, 10, &LogFile::add_info),
    m_info_receiver (this, 0, &LogFile::add_info),
    m_max_entries (max_entries),
    m_generation_id (0), 
    m_last_generation_id (0),
    m_has_errors (false),
    m_has_warnings (false),
    m_last_attn (false)
{
  connect (&m_timer, SIGNAL (timeout ()), this, SLOT (timeout ()));

  m_last_yield = tl::Clock::current ();

  m_timer.setSingleShot (true);
  m_timer.setInterval (0);

  if (register_global) {
    tl::info.add (&m_info_receiver, false);
    tl::log.add (&m_log_receiver, false);
    tl::error.add (&m_error_receiver, false);
    tl::warn.add (&m_warn_receiver, false);
  }
}

void
LogFile::clear ()
{
  QMutexLocker locker (&m_lock);

  if (!m_messages.empty ()) {
    m_messages.clear ();
    m_has_errors = m_has_warnings = false;
    ++m_generation_id;
  }
}

bool
LogFile::has_errors () const
{
  return m_has_errors;
}

bool
LogFile::has_warnings () const
{
  return m_has_warnings;
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
  bool attn = false, last_attn = false;

  m_lock.lock ();

  m_last_yield = tl::Clock::current ();

  if (m_generation_id != m_last_generation_id) {
    attn = m_has_errors || m_has_warnings;
    last_attn = m_last_attn;
    m_last_attn = attn;
    m_last_generation_id = m_generation_id;
    changed = true;
  }

  m_lock.unlock ();

  if (changed) {
    emit layoutChanged ();
    if (last_attn != attn) {
      emit attention_changed (attn);
    }
  }
}

void
LogFile::set_max_entries (size_t n)
{
  QMutexLocker locker (&m_lock);

  m_max_entries = n;

  while (m_messages.size () > m_max_entries) {
    m_messages.pop_front ();
  }
}

size_t
LogFile::max_entries () const
{
  return m_max_entries;
}

void 
LogFile::add (LogFileEntry::mode_type mode, const std::string &msg, bool continued)
{
  QMutexLocker locker (&m_lock);

  if (m_max_entries == 0) {
    return;
  }

  if (m_messages.size () >= m_max_entries) {
    m_messages.pop_front ();
  }

  if (mode == LogFileEntry::Warning || mode == LogFileEntry::WarningContinued) {
    m_has_warnings = true;
  } else if (mode == LogFileEntry::Error || mode == LogFileEntry::ErrorContinued) {
    m_has_errors = true;
  }

  m_messages.push_back (LogFileEntry (mode, msg, continued));

  ++m_generation_id;
}

void
LogFile::yield ()
{
  //  will update on next processEvents
  if (lay::ApplicationBase::instance ()->qapp_gui () && QThread::currentThread () == lay::ApplicationBase::instance ()->qapp_gui ()->thread ()) {
    if ((tl::Clock::current () - m_last_yield).seconds () > 0.2) {
      m_timer.start ();
    }
  }
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

  if (role == Qt::DecorationRole) {

    if (index.row () < int (m_messages.size ()) && index.row () >= 0) {
      LogFileEntry::mode_type mode = m_messages [index.row ()].mode ();
      if (mode == LogFileEntry::Error) {
        return QIcon (QString::fromUtf8 (":/error_16px.png"));
      } else if (mode == LogFileEntry::Warning) {
        return QIcon (QString::fromUtf8 (":/warn_16px.png"));
      } else if (mode == LogFileEntry::Info) {
        return QIcon (QString::fromUtf8 (":/info_16px.png"));
      } else {
        return QIcon (QString::fromUtf8 (":/empty_16px.png"));
      }
    }

  } else if (role == Qt::DisplayRole) {

    if (index.row () < int (m_messages.size ()) && index.row () >= 0) {
      return QVariant (tl::to_qstring (m_messages [index.row ()].text ()));
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

LogViewerDialog::LogViewerDialog (QWidget *parent, bool register_global, bool interactive)
  : QDialog (parent), 
    m_file (50000, register_global)  //  TODO: make this variable ..
{
  setupUi (this);

  //  For non-global log views, the verbosity selector does not make sense
  if (!register_global) {
    verbosity_cbx->hide ();
    verbosity_label->hide ();
  } else {
    verbosity_cbx->setCurrentIndex (std::max (-2, std::min (4, tl::verbosity () / 10)) + 2);
    connect (verbosity_cbx, SIGNAL (currentIndexChanged (int)), this, SLOT (verbosity_changed (int)));
  }

  if (!interactive) {
    clear_pb->hide ();
    separator_pb->hide ();
    copy_pb->hide ();
  } else {
    connect (clear_pb, SIGNAL (clicked ()), &m_file, SLOT (clear ()));
    connect (separator_pb, SIGNAL (clicked ()), &m_file, SLOT (separator ()));
    connect (copy_pb, SIGNAL (clicked ()), &m_file, SLOT (copy ()));
  }

  attn_frame->hide ();
  log_view->setModel (&m_file);

  connect (&m_file, SIGNAL (layoutChanged ()), log_view, SLOT (scrollToBottom ()));
  connect (&m_file, SIGNAL (attention_changed (bool)), attn_frame, SLOT (setVisible (bool)));
}

void
LogViewerDialog::verbosity_changed (int index)
{
  tl::verbosity ((index - 2) * 10 + 1);
}

// -----------------------------------------------------------------
//  AlertLog implementation

AlertLogButton::AlertLogButton (QWidget *parent)
  : QToolButton (parent)
{
  mp_logger = new LogViewerDialog (this, false, false);
  hide ();
  connect (&mp_logger->file (), SIGNAL (attention_changed (bool)), this, SLOT (attention_changed (bool)));
  connect (this, SIGNAL (clicked ()), mp_logger, SLOT (exec ()));
}

void
AlertLogButton::attention_changed (bool attn)
{
  setVisible (attn);

  //  as a special service, enlarge and color any surrounding frame red -
  //  this feature allows putting the alert button together with other entry fields into a frame and
  //  make this frame highlighted on error or warning.
  QFrame *frame = dynamic_cast<QFrame *> (parent ());
  if (frame) {

    if (frame->layout ()) {
      int l = 0, t = 0, r = 0, b = 0;
      frame->layout ()->getContentsMargins (&l, &t, &r, &b);
      if (attn) {
        l += 3; t += 3; r += 2; b += 2;
      } else {
        l -= 3; t -= 3; r -= 2; b -= 2;
      }
      frame->layout ()->setContentsMargins (l, t, r, b);
    }

    if (attn) {

      frame->setAutoFillBackground (true);
      QPalette palette = frame->palette ();
      palette.setColor (QPalette::Window, QColor (255, 160, 160));
      frame->setPalette (palette);

    } else {

      frame->setAutoFillBackground (false);
      frame->setPalette (QPalette ());

    }

  }
}

}

