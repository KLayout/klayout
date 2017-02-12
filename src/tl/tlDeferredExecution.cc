
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


#include "tlDeferredExecution.h"
#include "tlAssert.h"
#include "tlLog.h"

#include <stdio.h>

#include <QApplication>

namespace tl
{

static DeferredMethodScheduler *s_inst = 0;

DeferredMethodScheduler::DeferredMethodScheduler (QObject *parent)
  : QObject (parent), 
    m_disabled (0), m_scheduled (false)
{
  connect (&m_timer, SIGNAL (timeout ()), this, SLOT (timer ()));

  m_timer.setInterval (0); // immediately
  m_timer.setSingleShot (true); //  just once

  //  set up a fallback timer that cleans up pending execute jobs if something goes wrong
  connect (&m_fallback_timer, SIGNAL (timeout ()), this, SLOT (timer ()));
  m_fallback_timer.setInterval (500);
  m_fallback_timer.setSingleShot (false);
}

DeferredMethodScheduler::~DeferredMethodScheduler ()
{
  s_inst = 0;
}

DeferredMethodScheduler *
DeferredMethodScheduler::instance ()
{
  static QMutex lock;
  lock.lock ();
  if (s_inst == 0) {
    s_inst = new DeferredMethodScheduler (qApp);
  }
  lock.unlock ();
  return s_inst;
}

void 
DeferredMethodScheduler::schedule (DeferredMethodBase *method)
{
  m_lock.lock ();
  if (! method->m_scheduled || ! method->m_compressed) {
    m_methods.push_back (method);
    if (! m_scheduled) {
      qApp->postEvent (this, new QEvent (QEvent::User));
      m_scheduled = true;
    }
    method->m_scheduled = true;
  }
  m_lock.unlock ();
}

void 
DeferredMethodScheduler::unqueue (DeferredMethodBase *method)
{
  m_lock.lock ();
  for (std::list<DeferredMethodBase *>::iterator m = m_methods.begin (); m != m_methods.end (); ) {
    std::list<DeferredMethodBase *>::iterator mm = m;
    ++mm;
    if (*m == method) {
      method->m_scheduled = false;
      m_methods.erase (m);
    }
    m = mm;
  }
  m_lock.unlock ();
}

void 
DeferredMethodScheduler::enable (bool en)
{
  m_lock.lock ();
  if (en) {
    tl_assert (m_disabled > 0);
    --m_disabled;
  } else {
    ++m_disabled;
  }
  m_lock.unlock ();
}

bool
DeferredMethodScheduler::event (QEvent *event)
{
  if (event->type () == QEvent::User) {
    timer ();
    return true;
  } else {
    return QObject::event (event);
  }
}

void 
DeferredMethodScheduler::timer ()
{
  if (m_disabled) {
    //  start again if disabled
    m_timer.start ();
  } else {
    try {
      execute ();
    } catch (tl::Exception &ex) {
      tl::error << tl::to_string (QObject::tr ("Exception caught: ")) << ex.msg ();
    } catch (std::exception &ex) {
      tl::error << tl::to_string (QObject::tr ("Exception caught: ")) << ex.what ();
    } catch (...) {
      tl::error << tl::to_string (QObject::tr ("Unspecific exception caught"));
    }
  }
}

void
DeferredMethodScheduler::execute ()
{
  std::list<DeferredMethodBase *> methods;

  m_lock.lock ();
  methods.swap (m_methods);
  m_scheduled = false;
  m_lock.unlock ();

  //  do the execution outside the locked range to avoid deadlocks if the method's execution
  //  schedules another call.
  for (std::list<DeferredMethodBase *>::iterator m = methods.begin (); m != methods.end (); ++m) {
    (*m)->execute ();
    (*m)->m_scheduled = false;
  }
}

}

