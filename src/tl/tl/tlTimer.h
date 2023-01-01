
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


#ifndef HDR_tlTimer
#define HDR_tlTimer

#include "tlCommon.h"

#include <string>
#include <stdint.h>
#include <time.h>

class QDateTime;

namespace tl
{

/**
 *  @brief clock_gettime is not implemented in Mac OS X 10.11 and lower
 *  From: https://gist.githubusercontent.com/jbenet/1087739/raw/638b37f76cdd9dc46d617443cab27eac297e2ee3/current_utc_time.c
 */
TL_PUBLIC void current_utc_time (struct timespec *ts);

/**
 *  @brief A basic timer class
 *
 *  Measure the time between start() and stop() and 
 *  report the time through several methods
 */

class TL_PUBLIC Timer
{
public:
  typedef int64_t timer_t;

  Timer ();
  
  /** 
   *  @brief Starts the timer
   */
  void start ();

  /** 
   *  @brief Stops the timer
   *  Only after stop or take, the time can be read with sec_user etc.
   */
  void stop ();

  /**
   *  @brief Takes the time, but does not stop
   */
  void take ();

  /**
   *  @brief Reports the time spent between start() and stop() in user space
   */
  double sec_user () const
  { 
    return (double (m_user_ms_res) * 0.001); 
  }

  /** 
   *  @brief Reports the time spent between start() and stop() in system space
   */
  double sec_sys () const
  { 
    return (double (m_sys_ms_res) * 0.001); 
  }

  /**
   *  @brief Reports the real time spent between start() and stop()
   */
  double sec_wall () const
  {
    return (double (m_wall_ms_res) * 0.001);
  }

  /**
   *  @brief Reports the current memory usage
   */
  static size_t memory_size ();

private:
  timer_t m_user_ms, m_sys_ms, m_wall_ms;
  timer_t m_user_ms_res, m_sys_ms_res, m_wall_ms_res;
};

/**
 *  @brief A self-timing class 
 *
 *  Starts the timer upon construction of the object and
 *  reports the object's lifetime upon destruction
 */

class TL_PUBLIC SelfTimer
  : public Timer 
{
public:
  /**
   *  @brief Instantiate the timer and start 
   */
  SelfTimer (const std::string &desc) : Timer (), m_desc (desc)
  {
    m_enabled = true;
    start ();
    start_report ();
  }

  /**
   *  @brief Instantiate the timer and start if the first parameter is true
   *
   *  This constructor is useful in conjunction with the verbosity level: 
   *  SelfTimer (tl::verbosity () >= 30, ...).
   */
  SelfTimer (bool enabled, const std::string &desc) : Timer (), m_desc (desc)
  {
    m_enabled = enabled;
    if (enabled) {
      start ();
      start_report ();
    }
  }

  ~SelfTimer ()
  {  
    if (m_enabled) {
      stop ();
      report ();
    }
  }

private:
  void report () const;
  void start_report () const;

  std::string m_desc;
  bool m_enabled;
};

/**
 *  @brief An abstraction for a time measurement feature
 *  
 *  This class can be used to take the current time and 
 *  compute time differences. The clock represents real
 *  time and can be used to time user interface actions.
 *
 *  @code
 *  tl::Clock start = tl::Clock::current ();
 *  ... do something
 *  if ((tl::Clock::current () - start).seconds () > 2) {
 *    ... took longer than 2 seconds
 *  }
 *  @/code
 */
class TL_PUBLIC Clock 
{
public:
  typedef int64_t timer_t;

  typedef unsigned long clock_value;

  /**
   *  @brief Default constructor: construct a clock object pointing to an arbitrary value
   */
  Clock () : m_clock_ms (0)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Create a clock value corresponding to a certain number of seconds
   */
  Clock (double s);

  /**
   *  @brief Copy constructor
   */
  Clock (const Clock &d)
    : m_clock_ms (d.m_clock_ms)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Assignment 
   */
  Clock &operator= (Clock d)
  {
    m_clock_ms = d.m_clock_ms;
    return *this;
  }

  /**
   *  @brief Equality 
   */
  bool operator== (Clock d) const
  {
    return m_clock_ms == d.m_clock_ms;
  }

  /**
   *  @brief Inequality 
   */
  bool operator!= (Clock d) const
  {
    return ! operator== (d);
  }

  /**
   *  @brief Comparison 
   */
  bool operator< (Clock d) const
  {
    return m_clock_ms < d.m_clock_ms;
  }

  /**
   *  @brief Difference (in place)
   */
  Clock &operator-= (Clock d)
  {
    m_clock_ms -= d.m_clock_ms;
    return *this;
  }

  /**
   *  @brief Difference
   */
  Clock operator- (Clock d) const
  {
    Clock c (*this);
    c -= d;
    return c;
  }

  /**
   *  @brief Convert into seconds
   */
  double seconds () const;

  /**
   *  @brief Current time (user time from start of process)
   */
  static Clock current ();

private:
  timer_t m_clock_ms;
};

} // namespace tl

#endif

