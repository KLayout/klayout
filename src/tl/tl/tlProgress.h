
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


#ifndef HDR_tlProgress
#define HDR_tlProgress

#include "tlCommon.h"

#include <string>
#include "tlException.h"
#include "tlTimer.h"
#include "tlList.h"

#include <set>

class QWidget;

namespace tl
{

class Progress;
class ProgressAdaptor;
class RelativeProgress;
class AbstractProgress;
class AbsoluteProgress;

/**
 *  @brief A helper class to clean up pending progress objects
 *
 *  Pending progress objects may be created in scripts. If scripts are aborted
 *  (e.g. in the debugger), progress objects may stay behind a block the application.
 *  To prevent this, this object keeps track of progress objects created between
 *  it's constructor and destructor and cleans up the objects created but not
 *  destroyed.
 */

class TL_PUBLIC ProgressGarbageCollector
{
public:
  ProgressGarbageCollector ();
  ~ProgressGarbageCollector ();

private:
  std::set<tl::Progress *> mp_valid_objects;
};

/**
 *  @brief The "break" exception class
 *
 *  Exceptions of this kind are thrown if a break was encountered
 */

class TL_PUBLIC BreakException
  : public tl::Exception
{
public:
  BreakException () : tl::Exception ("Operation cancelled") { }
};

/**
 *  @brief A "progress" reporter class 
 *
 *  A progress can be reported in two ways: as
 *  a relative value with a target "max" values and as
 *  an absolute value. 
 *  This implementation delegates the actual implementation
 *  to an "adaptor" class that handles the actual display
 *  of the progress.
 *
 *  This class also allows for some kind of background processing:
 *  once the test() method is called, the 
 *  adaptor's yield() method is called once that "yield_interval" 
 *  calls have passed. The yield_interval value should be chosen
 *  sufficiently large so that no performance penalty is to be paid
 *  for this. In addition, each test() call tests whether
 *  the operation may have been cancelled and may throw an 
 *  BreakException in this case.
 *
 *  This class is basically a base class for the actual implementation.
 *  A functionality every progress object must provide is the 
 *  ability to deliver a relative value and a textual description of 
 *  the progress. By default the progress is displayed as a bar 
 *  showing the relative progress.
 */

class TL_PUBLIC Progress
  : public tl::list_node<Progress>
{
public:
  /**
   *  @brief Constructor
   *
   *  This initializes the progress reporter object.
   *  The string passed is the initial description and title string.
   * 
   *  @param desc The description and title string
   *  @param yield_interval See above.
   *  @param can_cancel If set to true, the progress may be cancelled which results in a BreakException begin raised
   */
  Progress (const std::string &desc, size_t yield_interval = 0, bool can_cancel = true);

  /**
   *  @brief The destructor
   */
  virtual ~Progress ();

  /**
   *  @brief Delivers the current progress as a string
   */
  virtual std::string formatted_value () const = 0;

  /**
   *  @brief Delivers the relative progress (a value between 0 and 100 for 0 to 100%).
   *
   *  The value can be bigger and the default progress bar will wrap around for 
   *  values >= 100.
   */
  virtual double value () const = 0;

  /**
   *  @brief Returns true if the progress is an abstract one
   *
   *  Abstract progress objcts don't have a value but mark a section begin executed as a top level progress.
   *  Technically they will open a channel for the UI - e.g. leaving a progress dialog open while the
   *  operation is running.
   */
  virtual bool is_abstract () const = 0;

  /**
   *  @brief Creates a widget that renders the progress graphically
   *
   *  The widget is not the progress bar - the progress bar is always shown.
   *  This method returns 0 if no graphical representation is required.
   */
  virtual QWidget *progress_widget () const { return 0; }

  /**
   *  @brief Renders the progress on the widget that was created by progress_widget
   */
  virtual void render_progress (QWidget * /*widget*/) const { }

  /**
   *  @brief Gets a value indicating whether the operation can be cancelled
   */
  bool can_cancel () const
  {
    return m_can_cancel;
  }

  /**
   *  @brief Set the description text
   */
  void set_desc (const std::string &desc);

  /**
   *  @brief Render the description string
   */
  const std::string &desc () const
  {
    return m_desc;
  }

  /**
   *  @brief Sets a value indicating whether the progress is a "final" one
   *
   *  A final progress will prevent child progress objects from showing. It basically summarizes child operations.
   *  By default, a progress object is not final.
   */
  void set_final (bool f)
  {
    m_final = f;
  }

  /**
   *  @brief Gets a value indicating whether the progress is a "final" one
   */
  bool final () const
  {
    return m_final;
  }

  /**
   *  @brief Render the title string
   */
  const std::string &title () const
  {
    return m_title;
  }

  /**
   *  @brief Used by the adaptor to signal a break condition
   */
  void signal_break ();

  /**
   *  @brief Returns true, if a break is scheduled
   */
  bool break_scheduled () const
  {
    return m_cancelled;
  }

protected:
  /**
   *  @brief Indicates that a new value has arrived
   *
   *  This method must be called by the derived class.
   *  If "force_yield" is true, the events are always processed.
   */
  bool test (bool force_yield = false);

  /**
   *  @brief This method needs to be called by all derived classes after initialization has happened
   */
  void initialize ();

  /**
   *  @brief This method needs to be called by all derived classes in the destructor
   */
  void shutdown ();

private:
  friend class ProgressAdaptor;
  friend class ProgressGarbageCollector;

  std::string m_desc, m_last_desc;
  std::string m_title;
  bool m_final;
  size_t m_interval_count;
  size_t m_yield_interval;
  double m_last_value;
  bool m_can_cancel;
  bool m_cancelled;
  bool m_registered;
  tl::Clock m_last_yield;

  static tl::ProgressAdaptor *adaptor ();
  static void register_adaptor (tl::ProgressAdaptor *pa);
};

/**
 *  @brief The receivers for progress reports
 *
 *  The progress adaptors form a thread-local stack of receivers. New receivers can override the
 *  previous receivers. It is important that receivers are always created in a nested fashion inside
 *  a single thread.
 */

class TL_PUBLIC ProgressAdaptor
{
public:
  typedef tl::list<tl::Progress>::iterator iterator;

  ProgressAdaptor ();
  virtual ~ProgressAdaptor ();

  virtual void register_object (Progress *progress);
  virtual void unregister_object (Progress *progress);
  virtual void trigger (Progress *progress) = 0;
  virtual void yield (Progress *progress) = 0;

  void prev (ProgressAdaptor *pa);
  ProgressAdaptor *prev ();

  bool is_busy () const
  {
    return !mp_objects.empty ();
  }

  tl::Progress *first ();

  void signal_break ();

protected:
  iterator begin ()
  {
    return mp_objects.begin ();
  }

  iterator end ()
  {
    return mp_objects.end ();
  }

private:
  friend class ProgressGarbageCollector;

  ProgressAdaptor *mp_prev;
  tl::list<tl::Progress> mp_objects;
};

/**
 *  @brief The abstract progress
 *
 *  An abstract progress object can be used as a top-level progress object to mark a section
 *  in an operation flow. This will provide a hint for the UI to leave the progress dialog open
 *  for example.
 */
class TL_PUBLIC AbstractProgress
  : public Progress
{
public:
  /**
   *  @brief Constructor
   */
  AbstractProgress (const std::string &desc);

  /**
   *  @brief Destructor
   */
  ~AbstractProgress ();

  /**
   *  @brief Delivers the current progress as a string (empty for the abstract progress)
   */
  std::string formatted_value () const { return std::string (); }

  /**
   *  @brief Delivers the relative progress (0 for the abstract progress)
   */
  double value () const { return 0.0; }

  /**
   *  @brief Indicates this progress reporter is abstract
   */
  bool is_abstract() const { return true; }
};

/**
 *  @brief A relative progress value
 *
 *  The relative value is computed from a maximum and current value.
 *  The ratio is converted to a 0 to 100% completion value.
 */
class TL_PUBLIC RelativeProgress
  : public Progress
{
public:
  /**
   *  @brief Constructor
   *
   *  This initializes the progress reporter object.
   *  The string passed is the initial description and title string.
   * 
   *  @param desc The description and title string
   *  @param max_count The limit "max" value. 0 for absolute display of values.
   *  @param yield_interval See above.
   *  @param can_cancel If set to true, the progress may be cancelled which results in a BreakException begin raised
   */
  RelativeProgress (const std::string &desc, size_t max_count = 0, size_t yield_interval = 0, bool can_cancel = true);

  ~RelativeProgress ();

  /**
   *  @brief Delivers the current progress as a string
   */
  std::string formatted_value () const;

  /**
   *  @brief Delivers the relative progress (a value between 0 and 1 for 0 to 100%).
   *
   *  The value can be bigger and the default progress bar will wrap around for 
   *  values >= 1.
   */
  double value () const;

  /**
   *  @brief Indicates this progress reporter isn't abstract
   */
  bool is_abstract() const { return false; }

  /** 
   *  @brief Set the format of the output.
   *
   *  This is a sprintf format string with the value being
   *  passed as a single double argument.
   */
  void set_format (const std::string &fmt)
  {
    m_format = fmt;
  }
  
  /**
   *  @brief Increment the count
   */
  RelativeProgress &operator++ ()
  {
    return set (m_count + 1);
  }

  /**
   *  @brief Set the count absolutely
   */
  RelativeProgress &set (size_t count, bool force_yield = false);

private:
  friend class ProgressAdaptor;

  std::string m_format;
  size_t m_count;
  size_t m_last_count;
  double m_unit;
};

/**
 *  @brief An absolute progress value
 *
 *  The absolute progress indicates a current value with an unknown upper limit, i.e. the
 *  file size already read or written.
 *  The value can be formatted in various ways and the translation into a progress value
 *  of 0 to 100% can be configured within this class.
 */
class TL_PUBLIC AbsoluteProgress
  : public Progress
{
public:
  /**
   *  @brief Constructor
   *
   *  This initializes the progress reporter object.
   *  The string passed is the initial description and title string.
   * 
   *  @param desc The description and title string
   *  @param yield_interval See above.
   */
  AbsoluteProgress (const std::string &desc, size_t yield_interval = 0, bool can_cancel = true);

  /**
   *  @brief Destructor
   */
  ~AbsoluteProgress ();

  /**
   *  @brief Delivers the current progress as a string
   */
  std::string formatted_value () const;

  /**
   *  @brief Delivers the relative progress (a value between 0 and 1 for 0 to 100%).
   *
   *  The value can be bigger and the default progress bar will wrap around for 
   *  values >= 1.
   */
  double value () const;

  /**
   *  @brief Indicates this progress reporter isn't abstract
   */
  bool is_abstract() const { return false; }

  /**
   *  @brief Set the format of the output.
   *
   *  This is a sprintf format string with the value being
   *  passed as a single double argument.
   */
  void set_format (const std::string &fmt)
  {
    m_format = fmt;
  }
  
  /**
   *  @brief Set the unit
   *
   *  This is the unit of the output. The unit is the 
   *  value by which the current count is divided to render
   *  the value passed to the format string.
   */
  void set_unit (double unit)
  {
    m_unit = unit;
  }

  /**
   *  @brief Set the format unit
   *
   *  This is the unit used for formatted output. This allows separating the format value
   *  from the bar value which is percent.
   */
  void set_format_unit (double unit)
  {
    m_format_unit = unit;
  }

  /**
   *  @brief Increment the count
   */
  AbsoluteProgress &operator++ ()
  {
    return set (m_count + 1);
  }

  /**
   *  @brief Set the count absolutely
   */
  AbsoluteProgress &set (size_t count, bool force_yield = false);

private:
  friend class ProgressAdaptor;

  std::string m_format;
  size_t m_count;
  double m_unit;
  double m_format_unit;
};

}

#endif

