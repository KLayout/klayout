
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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


#include <QMutex>
#include <QApplication>
#include <QInputEvent>

#include "layProgress.h"
#include "layMainWindow.h"
#include "layProgressWidget.h"
#include "tlProgress.h"
#include "tlDeferredExecution.h"

#include <stdlib.h>
#include <math.h>

namespace lay
{

// --------------------------------------------------------------------

static const char *s_alive_prop_name = "klayout_progressAlive";

void mark_widget_alive (QWidget *w, bool alive)
{
  if (alive) {
    w->setProperty (s_alive_prop_name, QVariant (true));
  } else {
    w->setProperty (s_alive_prop_name, QVariant ());
  }
}

static bool is_marked_alive (QObject *obj)
{
  return obj->property (s_alive_prop_name).isValid ();
}

// --------------------------------------------------------------------

ProgressReporter::ProgressReporter ()
  : m_start_time (), mp_pb (0), m_pw_visible (false)
{
  //  .. nothing yet ..
}

ProgressReporter::~ProgressReporter ()
{
  mp_pb = 0;
}

void
ProgressReporter::set_progress_bar (lay::ProgressBar *pb)
{
  if (pb == mp_pb) {
    return;
  }
  if (mp_pb) {
    set_visible (m_pw_visible);
  }
  mp_pb = pb;
  if (mp_pb) {
    set_visible (m_pw_visible);
  }
}

void 
ProgressReporter::register_object (tl::Progress *progress)
{
  if (begin () == end ()) {
    //  to avoid recursions of any kind, disallow any user interaction except
    //  cancelling the operation
    QApplication::instance ()->installEventFilter (this);
  }

  tl::ProgressAdaptor::register_object (progress);

  if (m_start_time == tl::Clock () && ! m_pw_visible) {
    m_start_time = tl::Clock::current ();
  }

  //  make dialog visible after some time has passed
  if (! m_pw_visible && (tl::Clock::current () - m_start_time).seconds () > 1.0) {
    set_visible (true);
  }

  if (progress->is_abstract ()) {
    if (mp_pb) {
      mp_pb->update_progress (progress);
    }
    process_events ();
  } else {
    update_and_yield ();
  }
}

void 
ProgressReporter::unregister_object (tl::Progress *progress)
{
  tl::ProgressAdaptor::unregister_object (progress);

  //  close or refresh window
  if (begin () == end ()) {

    if (m_pw_visible) {
      set_visible (false);
    }

    m_start_time = tl::Clock ();

    if (mp_pb) {
      mp_pb->update_progress (0);
    }

    process_events ();

    QApplication::instance ()->removeEventFilter (this);

  } else {
    update_and_yield ();
  }
}

void 
ProgressReporter::trigger (tl::Progress * /*progress*/)
{
  if (begin () != end ()) {
    //  make dialog visible after some time has passed
    if (! m_pw_visible && (tl::Clock::current () - m_start_time).seconds () > 1.0) {
      set_visible (true);
    }
    update_and_yield ();
  }
}

void 
ProgressReporter::yield (tl::Progress * /*progress*/)
{
  //  make dialog visible after some time has passed
  if (! m_pw_visible && (tl::Clock::current () - m_start_time).seconds () > 1.0) {
    set_visible (true);
    update_and_yield ();
  } else if (m_pw_visible) {
    //  process events if necessary
    process_events ();
  }
}

void
ProgressReporter::update_and_yield ()
{
  if (! m_pw_visible) {
    return;
  }

  if (mp_pb && first ()) {
    mp_pb->update_progress (first ());
    QWidget *w = mp_pb->progress_get_widget ();
    if (w) {
      first ()->render_progress (w);
    }
  }

  process_events (); // Qt4 seems to need this
}

void
ProgressReporter::process_events ()
{
  //  Don't execute deferred methods during progress handling (undesired side effects)
  tl::NoDeferredMethods silent;

  if (m_pw_visible && lay::MainWindow::instance () && QApplication::instance ()) {
    QApplication::instance ()->processEvents (QEventLoop::AllEvents);
  }
}

void
ProgressReporter::set_visible (bool vis)
{
  if (mp_pb) {
    mp_pb->show_progress_bar (vis);
  }

  if (vis != m_pw_visible) {

    //  prevent deferred method execution inside progress events - this might interfere with the
    //  actual operation
    tl::DeferredMethodScheduler::enable (!vis);

    if (mp_pb) {
      if (!vis) {
        mp_pb->progress_remove_widget ();
      } else if (mp_pb->progress_wants_widget () && first ()) {
        mp_pb->progress_add_widget (first ()->progress_widget ());
      }
    }

    m_pw_visible = vis;

  }
}

bool
ProgressReporter::eventFilter (QObject *obj, QEvent *event)
{
  //  do not handle events that are not targeted towards widgets
  if (! dynamic_cast <QWidget *> (obj)) {
    return false;
  }

  //  do not handle events if a modal widget is active (i.e. a message box)
  if (QApplication::activeModalWidget () && ! dynamic_cast<lay::MainWindow *> (QApplication::activeModalWidget ())) {
    return false;
  }

  if (dynamic_cast <QInputEvent *> (event)) {

    QObject *o = obj;
    while (o) {
      //  If the watched object is a child of the progress widget or the macro editor, pass the event on to this.
      //  Including the macro editor keeps it alive while progress events are processed.
      if (dynamic_cast<lay::ProgressWidget *> (o) || is_marked_alive (o)) {
        return false;
      }
      o = o->parent ();
    }

    // eat the event
    return true;

  } else {
    return false;
  }
}

} // namespace lay

