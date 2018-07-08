
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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

#include "tlThreads.h"
#include "tlUnitTest.h"

#include <stdio.h>

#if defined(WIN32)
#include <windows.h>
inline void usleep(long us)
{
  Sleep(us / 1000);
}
#else
#include <unistd.h>
#endif

class MyThread : public tl::Thread
{
public:
  MyThread () : m_value (0), m_stop (false) { }

  int value ()
  {
    tl::MutexLocker locker (&m_lock);
    return m_value;
  }

  void run ()
  {
    for (int i = 0; i < 10 && !m_stop; ++i) {
      {
        tl::MutexLocker locker (&m_lock);
        ++m_value;
      }
      usleep (10000);
    }
  }

  void stop ()
  {
    m_stop = true;
  }

private:
  int m_value;
  bool m_stop;
  tl::Mutex m_lock;
};

//  basic: concurrency, ability to stop async, wait
TEST(1)
{
  MyThread my_thread;
  my_thread.start ();
  while (my_thread.value () < 5)
    ;
  my_thread.stop ();
  my_thread.wait ();
  //  stopped before 10 and after 5
  EXPECT_EQ (my_thread.value () >= 5 && my_thread.value () < 10, true);
}

