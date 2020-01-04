
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


#include "tlProgress.h"
#include "tlString.h"
#include "tlAssert.h"
#include "tlThreads.h"

#include <stdio.h>
#include <math.h>

namespace tl
{

// ---------------------------------------------------------------------------------------------
//  ProgressAdaptor implementation

ProgressAdaptor::ProgressAdaptor ()
  : mp_prev (0)
{
  tl::Progress::register_adaptor (this);
}

ProgressAdaptor::~ProgressAdaptor ()
{
  tl::Progress::register_adaptor (0);
}

void
ProgressAdaptor::prev (ProgressAdaptor *pa)
{
  mp_prev = pa;
}

ProgressAdaptor *
ProgressAdaptor::prev ()
{
  return mp_prev;
}

// ---------------------------------------------------------------------------------------------
//  Progress implementation

//  Hint: we don't want the ThreadStorage take ownership over the object. Hence we don't
//  store a pointer but a pointer to a pointer.
static tl::ThreadStorage<ProgressAdaptor **> s_thread_data;

Progress::Progress (const std::string &desc, size_t yield_interval)
  : m_desc (desc), m_title (desc),
    m_interval_count (0), 
    m_yield_interval (yield_interval), 
    m_last_value (-1.0),
    m_can_cancel (true),
    m_cancelled (false)
{
  //  .. nothing yet ..
}

Progress::~Progress ()
{
  //  .. nothing yet ..
}

void
Progress::initialize ()
{
  ProgressAdaptor *a = adaptor ();
  if (a) {
    a->register_object (this);
  }
}

void
Progress::shutdown ()
{
  ProgressAdaptor *a = adaptor ();
  if (a) {
    a->unregister_object (this);
  }
}

void 
Progress::register_adaptor (ProgressAdaptor *pa)
{
  ProgressAdaptor *current_pa = adaptor ();
  if (current_pa) {
    if (! pa) {
      pa = current_pa->prev ();
    } else {
      pa->prev (current_pa);
    }
  }

  s_thread_data.setLocalData (new (ProgressAdaptor *) (pa));
}

ProgressAdaptor *
Progress::adaptor () 
{
  if (! s_thread_data.hasLocalData ()) {
    return 0;
  } else {
    return *s_thread_data.localData ();
  }
}

void 
Progress::signal_break ()
{
  m_cancelled = true;
}

void
Progress::set_desc (const std::string &d)
{
  ProgressAdaptor *a = adaptor ();
  if (a && d != m_desc) {

    m_desc = d;
    a->trigger (this);
    a->yield (this);

    if (m_cancelled) {
      m_cancelled = false;
      throw tl::BreakException ();
    }

  }
}

bool Progress::test(bool force_yield)
{
  if (++m_interval_count >= m_yield_interval || force_yield) {

    ProgressAdaptor *a = adaptor ();

    bool needs_trigger = false;
    double v = value ();
    if (fabs (v - m_last_value) > 1e-6) {
      m_last_value = v;
      needs_trigger = true;
    }

    m_interval_count = 0;

    if (a) {
      tl::Clock now = tl::Clock::current ();
      if ((now - m_last_yield).seconds () > 0.1) {
        m_last_yield = now;
        if (needs_trigger) {
          a->trigger (this);
        }
        a->yield (this);
      }
    }

    if (m_cancelled) {
      m_cancelled = false;
      throw tl::BreakException ();
    }

    return true;

  } else {
    return false;
  }
}

// ---------------------------------------------------------------------------------------------
//  Progress implementation

RelativeProgress::RelativeProgress (const std::string &desc, size_t max_count, size_t yield_interval)
  : Progress (desc, yield_interval)
{
  m_format = "%.0f%%";
  m_unit = double (max_count) / 100.0;
  m_count = 0;
  m_last_count = 0;

  initialize ();
}

RelativeProgress::~RelativeProgress ()
{
  shutdown ();
}

double
RelativeProgress::value () const
{
  if (m_unit < 1e-10) {
    return 0.0;
  } else {
    return double (m_count) / m_unit;
  }
}

std::string 
RelativeProgress::formatted_value () const
{
  return tl::sprintf (m_format, value ());
}

RelativeProgress &
RelativeProgress::set (size_t count, bool force_yield)
{
  m_count = count;
  if (test (force_yield || m_count - m_last_count >= m_unit)) {
    m_last_count = m_count;
  }
  return *this;
}

// ---------------------------------------------------------------------------------------------
//  Progress implementation

AbsoluteProgress::AbsoluteProgress (const std::string &desc, size_t yield_interval)
  : Progress (desc, yield_interval)
{
  m_format = "%.0f";
  m_unit = 1.0;
  m_format_unit = 0.0;
  m_count = 0;

  initialize ();
}

AbsoluteProgress::~AbsoluteProgress ()
{
  shutdown ();
}

double
AbsoluteProgress::value () const
{
  if (m_unit < 1e-10) {
    return 0.0;
  } else {
    return double (m_count) / m_unit;
  }
}

std::string 
AbsoluteProgress::formatted_value () const
{
  double v = 0.0;
  double u = m_format_unit;
  if (u < 1e-10) {
    u = m_unit;
  }
  if (u > 1e-10) {
    v = double (m_count) / u;
  }
  return tl::sprintf (m_format, v);
}

AbsoluteProgress &
AbsoluteProgress::set (size_t count, bool /*force_yield*/)
{
  m_count = count;
  test ();
  return *this;
}

} // namespace tl


