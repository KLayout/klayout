
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

#include "tlDeferredExecutionQt.h"
#include "tlException.h"
#include "tlLog.h"

#include <QCoreApplication>

namespace tl
{

DeferredMethodSchedulerQt::DeferredMethodSchedulerQt ()
  : QObject (qApp), DeferredMethodScheduler ()
{
  m_event_type = QEvent::registerEventType ();

  connect (&m_timer, SIGNAL (timeout ()), this, SLOT (timer ()));

  m_timer.setInterval (0); // immediately
  m_timer.setSingleShot (true); //  just once

  //  set up a fallback timer that cleans up pending execute jobs if something goes wrong
  connect (&m_fallback_timer, SIGNAL (timeout ()), this, SLOT (timer ()));
  m_fallback_timer.setInterval (500);
  m_fallback_timer.setSingleShot (false);
}

DeferredMethodSchedulerQt::~DeferredMethodSchedulerQt ()
{
  //  .. nothing yet ..
}

void 
DeferredMethodSchedulerQt::queue_event ()
{
  qApp->postEvent (this, new QEvent (QEvent::Type (m_event_type)));
}

bool
DeferredMethodSchedulerQt::event (QEvent *event)
{
  if (event->type () == m_event_type) {
    timer ();
    return true;
  } else {
    return QObject::event (event);
  }
}

void 
DeferredMethodSchedulerQt::timer ()
{
  if (is_disabled ()) {
    //  start again if disabled
    m_timer.start ();
  } else {
    try {
      do_execute ();
    } catch (tl::Exception &ex) {
      tl::error << tl::to_string (QObject::tr ("Exception caught: ")) << ex.msg ();
    } catch (std::exception &ex) {
      tl::error << tl::to_string (QObject::tr ("Exception caught: ")) << ex.what ();
    } catch (...) {
      tl::error << tl::to_string (QObject::tr ("Unspecific exception caught"));
    }
  }
}

}
