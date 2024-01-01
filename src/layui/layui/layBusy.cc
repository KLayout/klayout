
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#include "layBusy.h"
#include "tlThreads.h"
#include "tlFileSystemWatcher.h"

namespace lay
{

tl::Mutex s_lock;

BusyMode *sp_busy_mode = 0;

// ----------------------------------------------------------------------------------------------------------

BusyMode::BusyMode ()
{
  tl::MutexLocker locker (&s_lock);
  if (sp_busy_mode == 0) {
    sp_busy_mode = this;
  }
}

BusyMode::~BusyMode ()
{
  tl::MutexLocker locker (&s_lock);
  if (sp_busy_mode == this) {
    sp_busy_mode = 0;
  }
}

// ----------------------------------------------------------------------------------------------------------

BusySection::BusySection ()
{
  tl::MutexLocker locker (&s_lock);
  mp_busy_mode = sp_busy_mode;
  m_previous_mode = false;
  if (mp_busy_mode) {
    m_previous_mode = mp_busy_mode->is_busy ();
    mp_busy_mode->enter_busy_mode (true);
  }

  //  disable file system watchers during busy periods
  tl::FileSystemWatcher::global_enable (false);
}

BusySection::~BusySection ()
{
  tl::MutexLocker locker (&s_lock);
  if (sp_busy_mode == mp_busy_mode && mp_busy_mode) {
    mp_busy_mode->enter_busy_mode (m_previous_mode);
  }
  mp_busy_mode = 0;

  tl::FileSystemWatcher::global_enable (true);
}

bool 
BusySection::is_busy ()
{
  tl::MutexLocker locker (&s_lock);
  return sp_busy_mode && sp_busy_mode->is_busy ();
}

// ----------------------------------------------------------------------------------------------------------

}

#endif
