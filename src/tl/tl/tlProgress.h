
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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

class QWidget;

namespace tl
{

class Progress;

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
  ProgressAdaptor ();
  virtual ~ProgressAdaptor ();

  virtual void register_object (Progress *progress) = 0;
  virtual void unregister_object (Progress *progress) = 0;
  virtual void trigger (Progress *progress) = 0;
  virtual void yield (Progress *progress) = 0;

  void prev (ProgressAdaptor *pa);
  ProgressAdaptor *prev ();

private:
  ProgressAdaptor *mp_prev;
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

class Progress;

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
   */
  Progress (const std::string &desc, size_t yield_interval = 1000);

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
   *  @brief Set a value indicating whether the operation can be cancelled
   *  
   *  The progress object will throw a BreakException is cancelled and this
   *  flag is set to true. The default is "true".
   */
  void can_cancel (bool f)
  {
    m_can_cancel = f;
  }

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

  std::string m_desc;
  std::string m_title;
  size_t m_interval_count;
  size_t m_yield_interval;
  double m_last_value;
  bool m_can_cancel;
  bool m_cancelled;
  tl::Clock m_last_yield;

  static tl::ProgressAdaptor *adaptor ();
  static void register_adaptor (tl::ProgressAdaptor *pa);
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
   */
  RelativeProgress (const std::string &desc, size_t max_count = 0, size_t yield_interval = 1000);

  /**
   *  @brief Destructor
   */
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
  AbsoluteProgress (const std::string &desc, size_t yield_interval = 1000);

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

