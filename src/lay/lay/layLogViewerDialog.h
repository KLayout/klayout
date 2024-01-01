
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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
#include "tlTimer.h"
#include "layCommon.h"

#include <QTimer>
#include <QMutex>
#include <QDialog>
#include <QAbstractListModel>
#include <QToolButton>

#include <deque>
#include <string>

namespace lay
{

class LogFile;

/**
 *  @brief A helper class describing one log entry
 */
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

/**
 *  @brief The log receiver abstraction that connects a channel with the LogFile object
 */
class LAY_PUBLIC LogReceiver
  : public tl::Channel 
{
public:
  LogReceiver (LogFile *file, int verbosity, void (LogFile::*method)(const std::string &, bool));

protected:
  virtual void puts (const char *s);
  virtual void endl ();
  virtual void end ();
  virtual void begin ();
  virtual void yield ();

private:
  LogFile *mp_file;
  void (LogFile::*m_method)(const std::string &, bool);
  std::string m_text;
  int m_verbosity;
  QMutex m_lock;
};

/**
 *  @brief A log collection ("log file")
 *
 *  The log collector collects warnings, errors and info messages
 *  and presents this collection as a QAbstractListModel view
 *  viewing inside a QTreeWidget or the LogViewerDialog.
 *
 *  The log collector can either be used standalone or as a
 *  global receiver that will collect the global log
 *  messages.
 */
class LAY_PUBLIC LogFile
  : public QAbstractListModel
{
Q_OBJECT

public:
  /**
   *  @brief Constructs a log file receiver
   *  If "register_global" is true, the receiver will register itself as a global log receiver.
   *  Otherwise it's a private one that can be used with the "error", "warn" and "info" channels
   *  provided by the respective methods.
   */
  LogFile (size_t max_entries, bool register_global = true);

  /**
   *  @brief Implementation of the QAbstractItemModel interface
   */
  int rowCount(const QModelIndex &parent) const;

  /**
   *  @brief Implementation of the QAbstractItemModel interface
   */
  QVariant data(const QModelIndex &index, int role) const;

  /**
   *  @brief Gets a value indicating whether errors are present
   */
  bool has_errors () const;

  /**
   *  @brief Gets a value indicating whether warnings are present
   */
  bool has_warnings () const;

public slots:
  /**
   *  @brief Clears the log
   */
  void clear ();

  /**
   *  @brief Adds a separator
   */
  void separator ();

  /**
   *  @brief copies the contents to the clipboard
   */
  void copy ();

  /**
   *  @brief Gets the error channel
   */
  tl::Channel &error ()
  {
    return m_error_receiver;
  }

  /**
   *  @brief Gets the warning channel
   */
  tl::Channel &warn ()
  {
    return m_warn_receiver;
  }

  /**
   *  @brief Gets the info channel
   */
  tl::Channel &info ()
  {
    return m_info_receiver;
  }

  /**
   *  @brief Gets the log channel
   */
  tl::Channel &log ()
  {
    return m_log_receiver;
  }

  /**
   *  @brief Implementation of post-log action
   */
  void yield ();

  /**
   *  @brief Sets the maximum number of entries to show
   *
   *  Setting this value to 0 basically disables the log collection
   */
  void set_max_entries (size_t n);

  /**
   *  @brief Gets the maximum number of entries to show
   */
  size_t max_entries () const;

private slots:
  void timeout ();

signals:
  /**
   *  @brief This signal is emitted if the log's attention state has changed
   *  Attention state is "true" if either errors or warnings are present.
   */
  void attention_changed (bool f);

private:
  tl::Clock m_last_yield;
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
  bool m_has_errors, m_has_warnings;
  bool m_last_attn;

  /**
   *  @brief Adds an error
   */
  void add_error (const std::string &msg, bool continued)
  {
    add (continued ? LogFileEntry::ErrorContinued : LogFileEntry::Error, msg, continued);
  }

  /**
   *  @brief Adds a info message
   */
  void add_info (const std::string &msg, bool continued)
  {
    add (continued ? LogFileEntry::InfoContinued : LogFileEntry::Info, msg, continued);
  }

  /**
   *  @brief Adds a warning
   */
  void add_warn (const std::string &msg, bool continued)
  {
    add (continued ? LogFileEntry::WarningContinued : LogFileEntry::Warning, msg, continued);
  }

  /**
   *  @brief Adds anything
   */
  void add (LogFileEntry::mode_type mode, const std::string &msg, bool continued);
};

/**
 *  @brief A dialog presenting the log file
 */
class LAY_PUBLIC LogViewerDialog
  : public QDialog, 
    public Ui::LogViewerDialog
{
Q_OBJECT

public:
  /**
   *  @brief The constructor
   *  If "register_global" is true, the log is registered globally
   *  and will receiver global log messages.
   */
  LogViewerDialog (QWidget *parent, bool register_global = true, bool interactive = true);

  /**
   *  @brief Gets the log file object
   */
  LogFile &file ()
  {
    return m_file;
  }

public slots:
  void verbosity_changed (int l);
  
private:
  LogFile m_file;
};

/**
 *  @brief A tool button that collects logs and makes itself visible once attention is required
 */
class LAY_PUBLIC AlertLogButton
  : public QToolButton
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  AlertLogButton (QWidget *parent);

  /**
   *  @brief Gets the error channel
   */
  tl::Channel &error () const
  {
    return mp_logger->file ().error ();
  }

  /**
   *  @brief Gets the warn channel
   */
  tl::Channel &warn () const
  {
    return mp_logger->file ().warn ();
  }

  /**
   *  @brief Gets the info channel
   */
  tl::Channel &info () const
  {
    return mp_logger->file ().info ();
  }

  /**
   *  @brief Gets the log channel
   */
  tl::Channel &log () const
  {
    return mp_logger->file ().log ();
  }

  /**
   *  @brief Gets the error status of the log
   */
  bool has_errors () const
  {
    return mp_logger->file ().has_errors ();
  }

  /**
   *  @brief Gets the warning status of the log
   */
  bool has_warnings () const
  {
    return mp_logger->file ().has_warnings ();
  }

  /**
   *  @brief Gets the attention status of the log
   *  (either warnings or errors are present)
   */
  bool needs_attention () const
  {
    return has_errors () || has_warnings ();
  }

public slots:
  /**
   *  @brief Clears the log (and makes the button invisible)
   */
  void clear ()
  {
    mp_logger->file ().clear ();
  }

private slots:
  void attention_changed (bool);

private:
  LogViewerDialog *mp_logger;
};

}

#endif
