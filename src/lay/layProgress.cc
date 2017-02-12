
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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

#include <stdlib.h>
#include <math.h>

#include "layProgress.h"
#include "layMainWindow.h"
#include "layApplication.h"
#include "tlProgress.h"

namespace lay
{

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
    mp_pb->show_progress_bar (m_pw_visible);
  }
  mp_pb = pb;
  if (mp_pb) {
    mp_pb->show_progress_bar (m_pw_visible);
  }
}

void 
ProgressReporter::register_object (tl::Progress *progress)
{
  mp_objects.push_back (progress); // this keeps the outmost one visible. push_front would make the latest one visible.
  // mp_objects.push_front (progress);

  if (m_start_time == tl::Clock () && ! m_pw_visible) {
    m_start_time = tl::Clock::current ();
  }

  //  make dialog visible after some time has passed
  if (! m_pw_visible && (tl::Clock::current () - m_start_time).seconds () > 1.0) {
    if (mp_pb) {
      mp_pb->show_progress_bar (true);
    }
    m_pw_visible = true;
  }

  update_and_yield ();
}

void 
ProgressReporter::unregister_object (tl::Progress *progress)
{
  for (std::list <tl::Progress *>::iterator k = mp_objects.begin (); k != mp_objects.end (); ++k) {

    if (*k == progress) {

      mp_objects.erase (k);

      //  close or refresh window
      if (mp_objects.empty ()) {
        if (mp_pb) {
          mp_pb->show_progress_bar (false);
        }
        m_pw_visible = false;
        m_start_time = tl::Clock ();
      } 
      
      update_and_yield ();
      return;

    }

  }
}

void 
ProgressReporter::trigger (tl::Progress *progress)
{
  if (! mp_objects.empty () && mp_objects.front () == progress) {
    //  make dialog visible after some time has passed
    if (! m_pw_visible && (tl::Clock::current () - m_start_time).seconds () > 1.0) {
      if (mp_pb) {
        mp_pb->show_progress_bar (true);
      }
      m_pw_visible = true;
    }
    update_and_yield ();
  }
}

void 
ProgressReporter::yield (tl::Progress * /*progress*/)
{
  //  make dialog visible after some time has passed
  if (! m_pw_visible && (tl::Clock::current () - m_start_time).seconds () > 1.0) {
    if (mp_pb) {
      mp_pb->show_progress_bar (true);
    }
    m_pw_visible = true;
    update_and_yield ();
  } else if (m_pw_visible) {
    //  process events if necessary
    process_events ();
  }
}

void 
ProgressReporter::signal_break ()
{
  for (std::list <tl::Progress *>::iterator k = mp_objects.begin (); k != mp_objects.end (); ++k) {
    (*k)->signal_break ();
  }
}

void
ProgressReporter::update_and_yield ()
{
  if (m_pw_visible && ! mp_objects.empty ()) {
    if (mp_pb) {
      //  not supported yet: mp_pb->progress_widget ()->set_title ((*mp_objects.begin ())->title ());
      mp_pb->set_progress_can_cancel (mp_objects.front ()->can_cancel ());
      mp_pb->set_progress_text (mp_objects.front ()->desc ());
      mp_pb->set_progress_value (mp_objects.front ()->value (), mp_objects.front ()->formatted_value ());
    }
    process_events (); // Qt4 seems to need this
  }
}

void
ProgressReporter::process_events ()
{
  if (m_pw_visible && lay::MainWindow::instance () && lay::Application::instance ()) {
    lay::Application::instance ()->process_events ();
  }
}

} // namespace lay

