
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

#if defined(HAVE_QT)

#ifndef HDR_gtf
#define HDR_gtf

#include <string>
#include <vector>
#include <iostream>

#include <QObject>

#include "laybasicCommon.h"
#include "tlVariant.h"
#include "tlLog.h"

class QAction;
class QWidget;
class QTimer;

namespace gtf {

/**
 *  @brief The base class for logged events
 */
class LAYBASIC_PUBLIC LogEventBase
{
public:
  /**
   *   @brief Constructor
   */
  LogEventBase (int xml_line) 
    : m_xml_line (xml_line) 
  { 
    //  .. nothing yet ..
  }

  /**
   *   @brief Destructor
   */
  virtual ~LogEventBase () { }

  /**
   *  @brief Issue the given event in playback mode
   */
  virtual void issue_event () = 0;

  /** 
   *  @brief Get the name of the element
   */
  virtual const char *name () const = 0;

  /** 
   *  @brief Get the attributes of the element
   */
  virtual void attributes (std::vector< std::pair<std::string, std::string> > & /*attr*/) const { }

  /**
   *  @brief Compare the event to an other event
   */
  virtual bool equals (const LogEventBase &b) const = 0;

  /**
   *  @brief Tell, if this event is of "spontaneous" class
   *
   *  Spontaneous events are created internally rather than in response to a user
   *  action. These are recorded for playback purposes but not compared to other
   *  spontaneous ones.
   */
  virtual bool spontaneous () const
  {
    return false;
  }

  /**
   *  @brief Write this element to a stream
   */
  void write (std::ostream &os, bool with_endl = true) const;

  /**
   *  @brief Equality 
   */
  bool operator== (const LogEventBase &b) const 
  {
    return equals (b) && data () == b.data ();
  }

  /**
   *  @brief Inequality 
   */
  bool operator!= (const LogEventBase &b) const 
  {
    return !operator== (b);
  }

  /**
   *  @brief Get the line corresponding to the XML file where the element is stored
   */
  int xml_line () const
  {
    return m_xml_line;
  }

  /**
   *  @brief Access the data object
   */
  const tl::Variant &data () const
  {
    return m_data;
  }

  /**
   *  @brief Set the data object
   */
  void set_data (const tl::Variant &d) 
  { 
    m_data = d;
  }

protected:
  int m_xml_line;
  tl::Variant m_data;
};

/**
 *  @brief A helper class to intercept an action's trigger signal
 */
class ActionInterceptor 
  : public QObject
{
Q_OBJECT
public:
  ActionInterceptor (QObject *parent, QAction *action);

public slots:
  void triggered ();

signals:
  void intercepted_trigger ();

private:
  QAction *mp_action;
};

/** 
 *  @brief A container for the list of events
 */
class LAYBASIC_PUBLIC EventList
{
public:
  typedef std::vector<gtf::LogEventBase *> container_type;
  typedef container_type::const_iterator const_iterator;
  typedef container_type::iterator iterator;

  /** 
   *  @brief Constructor: create an empty list
   */
  EventList ();

  /**
   *  @brief Destructor
   */
  ~EventList ();

  /**
   *  @brief Load the log file
   *
   *  This method parses the given file and stores the events internally.
   *  If the file is not a valid log file or an error occurs, an exception
   *  is thrown. 
   */
  void load (const std::string &filename, bool no_spontaneous = false);

  /**
   *  @brief Save the events recorded so far to the given file
   *
   *  If the file cannot be written, an exception is thrown
   */
  void save (const std::string &file);

  /**
   *  @brief Last element of the list
   */
  const gtf::LogEventBase *back () const
  {
    return m_events.back ();
  }

  /**
   *  @brief Last element of the list (non-const version)
   */
  gtf::LogEventBase *back () 
  {
    return m_events.back ();
  }

  /**
   *  @brief pop an element
   */
  void pop_back () 
  {
    m_events.pop_back ();
  }

  /**
   *  @brief "empty" predicate
   */
  bool empty () const
  {
    return m_events.empty ();
  }

  /**
   *  @brief size of the list
   */
  unsigned int size () const
  {
    return int (m_events.size ());
  }

  /**
   *  @brief Random access 
   */
  const gtf::LogEventBase *operator[] (unsigned int index) const
  {
    return m_events [index];
  }

  /**
   *  @brief Random access (non-const version)
   */
  gtf::LogEventBase *operator[] (unsigned int index) 
  {
    return m_events [index];
  }

  /**
   *  @brief Start iterator for the events
   */
  const_iterator begin () const
  {
    return m_events.begin ();
  }

  /**
   *  @brief End iterator for the events
   */
  const_iterator end () const
  {
    return m_events.end ();
  }

  /**
   *  @brief Start iterator for the events (non-const version)
   */
  iterator begin () 
  {
    return m_events.begin ();
  }

  /**
   *  @brief End iterator for the events (non-const version)
   */
  iterator end ()
  {
    return m_events.end ();
  }

  /**
   *  @brief Add an event to the list 
   */
  void add (gtf::LogEventBase *e)
  {
    m_events.push_back (e);
  }

private:
  std::vector<gtf::LogEventBase *> m_events;
};

/**
 *  @brief The player class 
 *  
 *  The player parses the given log file, holds the events to be played
 *  and issues the event when the replay method is called.
 */
class LAYBASIC_PUBLIC Player
  : public QObject
{
Q_OBJECT
public:
  /**
   *  @brief Instantiate the player
   */
  Player (QObject *parent);

  /**
   *  @brief Destructor
   */
  ~Player ();

  /**
   *  @brief Start replaying the events
   *
   *  This method starts replaying the events loaded formerly. The rate by which the
   *  events are played is given by the "ms" argument, which tells how many
   *  milliseconds to wait before the next event is issued.
   *  The method will return immediately. QApplication::exec() will be required in order
   *  to actually replay the events.
   */
  void replay (int ms, int stop_at_line = -1);

  /**
   *  @brief Tell, if the player is actually playing
   *
   *  Returns true, if the player is actually playing, i.e. if replay has been issued and
   *  there are still events to issue.
   */
  bool playing () const
  {
    return m_playing_active;
  }

  /**
   *  @brief Tell, what event was actually issued by the Player
   *
   *  Returns a reference to the object that was issued by the Player recently.
   *  It returns 0 if no event was issued by the player.
   */
  QEvent *event_issued () const
  {
    return mp_event_issued;
  }

  /**
   *  @brief Tell, to which widget the event was actually issued by the Player
   *
   *  Returns a reference to the widget to which the recent event was issued by the Player.
   *  It returns 0 if no event was issued by the player.
   */
  QWidget *event_target () const
  {
    return mp_event_target;
  }

  /**
   *  @brief Send an event via the player
   */
  void issue_event (QWidget *target, QEvent *event);

  /**
   *  @brief Load the log file
   *
   *  This method parses the given file and stores the events internally.
   *  If the file is not a valid log file or an error occurs, an exception
   *  is thrown. 
   */
  void load (const std::string &filename, bool no_spontaneous = false)
  {
    m_events.load (filename, no_spontaneous);
  }

  /**
   *  @brief Get the instance of the player
   *
   *  Returns the pointer to the instance of the player or 0 if there is none.
   */
  static Player *instance ()
  {
    return ms_instance;
  }

public slots:
  void timer ();

private:
  //  no copying
  Player (const Player &);
  Player &operator= (const Player &);

  EventList m_events;
  QTimer *mp_timer;
  int m_ms;
  bool m_playing_active;
  unsigned int m_playing_index;
  int m_breakpoint;
  QEvent *mp_event_issued;
  QWidget *mp_event_target;
  static Player *ms_instance;
};

/**
 *  @brief The GUI test framework recorder
 *
 *  The recorder records GUI events and stores them.
 *  The can be written to a file using the "save" method.
 *  There should only be one instance of the recorder.
 */
class LAYBASIC_PUBLIC Recorder
  : public QObject
{
public:
  /**
   *  @brief Instantiate the recorder.
   */
  Recorder (QObject *parent, const std::string &log_file);

  /**
   *  @brief Destroy the recorder.
   */
  ~Recorder ();

  /**
   *  @brief Start the recording
   */
  void start ();

  /**
   *  @brief Stop the recording
   */
  void stop ();

  /**
   *  @brief Tell that an action was issued
   *
   *  The GTF does not track events that lead to an action, because
   *  this is not possible in every case (i.e. key shortcuts). Instead,
   *  the action is tracked itself. On each action, this function must be 
   *  called.
   */
  void action (QAction *action);

  /**
   *  @brief Issue a probe statement
   *
   *  Probe statements are used to express a window's property in form of a tl::Variant.
   */
  void probe (QWidget *widget, const tl::Variant &data);

  /**
   *  @brief Probing of standard widgets
   *
   *  This provides probing implementation for some standard widgets such as line edit etc.
   */
  tl::Variant probe_std (QWidget *w);

  /**
   *  @brief Support for logging errors: begin an error message
   */
  void errlog_begin ();

  /**
   *  @brief Support for logging errors: end an error message
   */
  void errlog_end ();

  /**
   *  @brief Support for logging errors: end a line of an error message
   */
  void errlog_endl ();

  /**
   *  @brief Support for logging errors: output a string
   */
  void errlog_puts (const char *s);

  /**
   *  @brief Tell, if we are recording
   *
   *  This method returns true if we are recording currently
   */
  bool recording () const
  {
    return m_recording;
  }

  /**
   *  @brief Save the events recorded every time a new event is added
   *
   *  After issueing this method with a "true" argument, the recorder saves the events to the given
   *  file each time a new event is recorded. This mode may be useful to record
   *  actions in test mode. If a crash happens, a log exists that records the
   *  events up to that point.
   */
  void save_incremental (bool si);

  /**
   *  @brief Save the events recorded so far to the given file
   *
   *  If the file cannot be written, an exception is thrown
   */
  void save ()
  {
    m_events.save (m_log_file);
  }

  /**
   * @brief The (only) recorder instance
   *
   * If there is no recorder, this method returns 0.
   */
  static Recorder *instance ()
  {
    return ms_instance;
  }

protected:
  bool eventFilter (QObject *object, QEvent *event);

private:
  //  no copying
  Recorder (const Recorder &);
  Recorder &operator= (const Recorder &);

  static Recorder *ms_instance;
  EventList m_events;
  bool m_recording;
  bool m_save_incremental;
  std::string m_error_text;
  std::string m_log_file;
  tl::Channel *mp_error_channel;
};

/**
 *  @brief special connect method that replaces the original connect for logged actions
 *
 *  This method can be used instead of a usual "connect" of a logged action's trigger
 *  signal with a corresponding slot of a receiver object. The purpose is to install
 *  an interceptor in case GTF is enabled. This will perform logging and replay.
 */
LAYBASIC_PUBLIC void action_connect (QAction *action, const char *signal, QObject *receiver, const char *slot);

/**
 *  @brief special disconnect method that must be used to disconnect connections that were build using action_connect
 */
LAYBASIC_PUBLIC void action_disconnect (QAction *action, const char *signal, QObject *receiver, const char *slot);

/**
 *  @brief For debugging purposes
 */
LAYBASIC_PUBLIC void dump_widget_tree ();

/**
 *  @brief A utility converting a QImage to a tl::Variant for gtf
 */
LAYBASIC_PUBLIC tl::Variant image_to_variant (const QImage &image);

}

#endif

#endif  //  defined(HAVE_QT)
