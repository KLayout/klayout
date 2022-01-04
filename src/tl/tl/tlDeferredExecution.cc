
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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

#include "tlDeferredExecution.h"
#include "tlAssert.h"

#include <stdio.h>

#if defined(HAVE_QT)
#  include "tlDeferredExecutionQt.h"
#endif

namespace tl
{

static DeferredMethodScheduler *s_inst = 0;

DeferredMethodScheduler::DeferredMethodScheduler ()
  : m_disabled (0), m_scheduled (false)
{
  tl_assert (! s_inst);
  s_inst = this;
}

DeferredMethodScheduler::~DeferredMethodScheduler ()
{
  s_inst = 0;
}

DeferredMethodScheduler *
DeferredMethodScheduler::instance ()
{
//  TODO: provide a way to register non-Qt schedulers
#if defined(HAVE_QT)
  if (! s_inst) {
    new DeferredMethodSchedulerQt ();
  }
#endif
  return s_inst;
}

void 
DeferredMethodScheduler::schedule (DeferredMethodBase *method)
{
  tl::MutexLocker locker (&m_lock);
  if (! method->m_scheduled || ! method->m_compressed) {
    m_methods.push_back (method);
    if (! m_scheduled) {
      queue_event ();
      m_scheduled = true;
    }
    method->m_scheduled = true;
  }
}

void 
DeferredMethodScheduler::unqueue (DeferredMethodBase *method)
{
  tl::MutexLocker locker (&m_lock);
  for (std::list<DeferredMethodBase *>::iterator m = m_methods.begin (); m != m_methods.end (); ) {
    std::list<DeferredMethodBase *>::iterator mm = m;
    ++mm;
    if (*m == method) {
      method->m_scheduled = false;
      m_methods.erase (m);
    }
    m = mm;
  }
  for (std::list<DeferredMethodBase *>::iterator m = m_executing.begin (); m != m_executing.end (); ++m) {
    if (*m == method) {
      m_unqueued.insert (method);
      break;
    }
  }
}

void 
DeferredMethodScheduler::do_enable (bool en)
{
  tl::MutexLocker locker (&m_lock);
  if (en) {
    tl_assert (m_disabled > 0);
    --m_disabled;
  } else {
    ++m_disabled;
  }
}

void
DeferredMethodScheduler::do_execute ()
{
  m_lock.lock ();
  m_executing.clear ();
  m_unqueued.clear ();
  m_executing.swap (m_methods);
  m_scheduled = false;
  m_lock.unlock ();

  //  do the execution outside the locked range to avoid deadlocks if the method's execution
  //  schedules another call.
  for (std::list<DeferredMethodBase *>::iterator m = m_executing.begin (); m != m_executing.end (); ++m) {

    bool still_valid;

    m_lock.lock ();
    //  during execution a method may be unqueued - make sure this is not executed
    still_valid = (m_unqueued.find (*m) == m_unqueued.end ());
    m_lock.unlock ();

    if (still_valid) {

      (*m)->m_scheduled = false;
      (*m)->execute ();

      //  execute() may have triggered another do_execute which we should consider here and stop:
      if (m_executing.empty ()) {
        break;
      }

    }

  }

  m_lock.lock ();
  m_unqueued.clear ();
  m_executing.clear ();
  m_lock.unlock ();
}

}
