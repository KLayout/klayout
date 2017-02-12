
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


#ifndef HDR_layLogViewerDialog
#define HDR_layLogViewerDialog

#include "ui_LogViewerDialog.h"
#include "tlLog.h"

#include <QTimer>
#include <QMutex>
#include <QDialog>
#include <QAbstractListModel>

#include <deque>
#include <string>

namespace lay
{

class LogFile;

class LogFileEntry
{
public:
  enum mode_type { Warning, WarningContinued, Error, ErrorContinued, Info, InfoContinued, Separator };

  LogFileEntry (mode_type mode, const std::string &s, bool cont)
    : m_mode (mode), m_text (s), m_continued (cont)
  { }

  const std::string &text () const
  {
    return m_text;
  }

  mode_type mode () const
  {
    return m_mode;
  }

  bool continued () const
  {
    return m_continued;
  }

private:
  mode_type m_mode;
  std::string m_text;
  bool m_continued;
};

class LogReceiver 
  : public tl::Channel 
{
public:
  LogReceiver (LogFile *file, int verbosity, void (LogFile::*method)(const std::string &, bool));

protected:
  virtual void puts (const char *s);
  virtual void endl ();
  virtual void end ();
  virtual void begin ();

private:
  LogFile *mp_file;
  void (LogFile::*m_method)(const std::string &, bool);
  std::string m_text;
  bool m_continued;
  int m_verbosity;
  QMutex m_lock;
};

class LogFile 
  : public QAbstractListModel
{
Q_OBJECT

public:
  LogFile (size_t max_entries);

  void error (const std::string &msg, bool continued)
  {
    add (continued ? LogFileEntry::ErrorContinued : LogFileEntry::Error, msg, continued);
  }

  void info (const std::string &msg, bool continued)
  {
    add (continued ? LogFileEntry::InfoContinued : LogFileEntry::Info, msg, continued);
  }

  void warn (const std::string &msg, bool continued)
  {
    add (continued ? LogFileEntry::WarningContinued : LogFileEntry::Warning, msg, continued);
  }

  int rowCount(const QModelIndex &parent) const;

  QVariant data(const QModelIndex &index, int role) const;

private slots:
  void timeout (); 
  void clear ();
  void separator ();
  void copy ();

public:
  QTimer m_timer;
  mutable QMutex m_lock;
  LogReceiver m_error_receiver;
  LogReceiver m_warn_receiver;
  LogReceiver m_log_receiver;
  LogReceiver m_info_receiver;
  std::deque<LogFileEntry> m_messages;
  size_t m_max_entries;
  size_t m_generation_id;
  size_t m_last_generation_id;

  void add (LogFileEntry::mode_type mode, const std::string &msg, bool continued);
};

class LogViewerDialog
  : public QDialog, 
    public Ui::LogViewerDialog
{
Q_OBJECT

public:
  LogViewerDialog (QWidget *parent);

public slots:
  void verbosity_changed (int l);
  
private:
  LogFile m_file;
};

}

#endif

