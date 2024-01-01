
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

#include "tlThreads.h"
#include "tlUnitTest.h"
#include "tlSleep.h"

#include <stdio.h>

class MyThread : public tl::Thread
{
public:
  MyThread () : m_value (0), m_stop (false) { }

  int value ()
  {
    tl::MutexLocker locker (&m_lock);
    return m_value;
  }

  void reset ()
  {
    m_value = 0;
    m_stop = false;
  }

  void run ()
  {
    for (int i = 0; i < 10 && !m_stop; ++i) {
      {
        tl::MutexLocker locker (&m_lock);
        ++m_value;
      }
      tl::usleep (10000);
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
TEST(1_basic)
{
  MyThread my_thread;

  EXPECT_EQ (my_thread.isRunning (), false);
  EXPECT_EQ (my_thread.isFinished (), false);

  my_thread.start ();

  EXPECT_EQ (my_thread.isRunning (), true);
  EXPECT_EQ (my_thread.isFinished (), false);

  while (my_thread.value () < 5)
    ;

  my_thread.stop ();
  my_thread.wait ();

  EXPECT_EQ (my_thread.isRunning (), false);
  EXPECT_EQ (my_thread.isFinished (), true);

  my_thread.reset ();
  my_thread.start ();

  EXPECT_EQ (my_thread.isRunning (), true);
  EXPECT_EQ (my_thread.isFinished (), false);

  while (my_thread.value () < 5)
    ;

  my_thread.stop ();
  my_thread.wait ();

  EXPECT_EQ (my_thread.isRunning (), false);
  EXPECT_EQ (my_thread.isFinished (), true);

  //  stopped before 10 and after 5
  EXPECT_EQ (my_thread.value () >= 5 && my_thread.value () < 10, true);
}

#if !defined(HAVE_QT)
//  basic: thread dtor while running
//  NOTE: QThread can't handle this - the pthread-based implementation will terminate the thread in this case
TEST(1_brute_shutdown)
{
  MyThread my_thread;
  my_thread.start ();
  EXPECT_EQ (true, true); // makes the compiler happy
}
#endif

//  basic: concurrency, ability to stop async, wait
TEST(1_timed_wait)
{
  MyThread my_thread;
  my_thread.start ();

  EXPECT_EQ (my_thread.wait (1), false);
  while (my_thread.value () < 5) {
    EXPECT_EQ (my_thread.wait (1), false);
  }

  EXPECT_EQ (my_thread.wait (100000 /*"enough"*/), true);
}

int s_mythread2_increment = 1;
void inc (volatile int &value)
{
  value += s_mythread2_increment;
}

class MyThread2 : public tl::Thread
{
public:
  MyThread2 (bool locked) : m_value (0), m_locked (locked) { }

  int value ()
  {
    return m_value;
  }

  void run ()
  {
    if (m_locked) {
      for (int i = 0; i < 10000000; ++i) {
        tl::MutexLocker locker (&m_lock);
        //  Do it more elaborate than ++m_value to prevent compiler optimization
        inc (m_value);
      }
    } else {
      for (int i = 0; i < 10000000; ++i) {
        inc (m_value);
      }
    }
  }

private:
  int m_value;
  tl::Mutex m_lock;
  bool m_locked;
};

//  Heavily loaded mutex
TEST(2_locked)
{
  MyThread2 my_thread (true);
  my_thread.start ();
  //  two times - once in the background and once in the main thread
  my_thread.run ();
  my_thread.wait ();
  EXPECT_EQ (my_thread.value (), 20000000);
}

//  Cross-check: unlocked
TEST(2_nonlocked)
{
  MyThread2 my_thread (false);
  my_thread.start ();
  //  two times - once in the background and once in the main thread
  my_thread.run ();
  my_thread.wait ();
  EXPECT_EQ (my_thread.value () < 20000000, true);
}

//  NOTE: ThreadStorage is broken on Qt 4.6.2 (invalid static_cast from type 'void*' to type 'int')
#if !defined(HAVE_QT) || QT_VERSION >= 0x40700

static tl::ThreadStorage<int> s_tls;

class MyThread3 : public tl::Thread
{
public:
  MyThread3 () : m_value (0) { }

  int value ()
  {
    return m_value;
  }

  void run ()
  {
    m_value = do_run (10000000);
  }

  int do_run (int n)
  {
    s_tls.setLocalData (0);
    for (int i = 0; i < n; ++i) {
      s_tls.localData () += s_mythread2_increment;
    }
    return s_tls.localData ();
  }

private:
  int m_value;
};

//  Thread-local storage
TEST(3)
{
  MyThread3 my_thread;
  my_thread.start ();
  //  While we start the loop inside the thread we run it outside. Since
  //  the counter is TLS, both loops will do the same but with different data.
  //  A mutex is not involved.
  EXPECT_EQ (my_thread.do_run (9999999), 9999999);
  my_thread.wait ();
  EXPECT_EQ (my_thread.value (), 10000000);
}

#endif

static tl::WaitCondition s_condition;
static tl::Mutex s_wait_mutex;

class MyThread4 : public tl::Thread
{
public:
  MyThread4 (int nstop) : m_value (0), m_nstop (nstop), m_stopped (false) { }

  int value ()
  {
    return m_value;
  }

  bool stopped ()
  {
    return m_stopped;
  }

  void run ()
  {
    while (m_value < 10000000) {
      m_value += s_mythread2_increment;
      if (m_value == m_nstop) {
        tl::MutexLocker locker (&s_wait_mutex);
        m_stopped = true;
        s_condition.wait (&s_wait_mutex);
        m_stopped = false;
      }
    }
  }

private:
  int m_value;
  int m_nstop;
  bool m_stopped;
};


//  WaitCondition
TEST(4_wakeAll)
{
  MyThread4 thr1 (3000000), thr2 (7000000);

  thr1.start ();
  thr2.start ();

  while (true) {
    bool res;
    {
      tl::MutexLocker locker (&s_wait_mutex);
      res = thr1.stopped () && thr2.stopped ();
    }
    if (res) {
      break;
    }
    EXPECT_EQ (thr1.isRunning (), true);
    EXPECT_EQ (thr2.isRunning (), true);
    tl_assert (thr1.isRunning () && thr2.isRunning ());
  }

  EXPECT_EQ (thr1.value (), 3000000);
  EXPECT_EQ (thr2.value (), 7000000);

  s_condition.wakeAll ();
  thr1.wait ();
  thr2.wait ();

  EXPECT_EQ (thr1.value (), 10000000);
  EXPECT_EQ (thr2.value (), 10000000);
}

//  WaitCondition with two wakeOne
TEST(4_wakeOne)
{
  MyThread4 thr1 (3000000), thr2 (7000000);

  thr1.start ();
  thr2.start ();

  while (true) {
    bool res;
    {
      tl::MutexLocker locker (&s_wait_mutex);
      res = thr1.stopped () && thr2.stopped ();
    }
    if (res) {
      break;
    }
    EXPECT_EQ (thr1.isRunning (), true);
    EXPECT_EQ (thr2.isRunning (), true);
    tl_assert (thr1.isRunning () && thr2.isRunning ());
  }

  EXPECT_EQ (thr1.value (), 3000000);
  EXPECT_EQ (thr2.value (), 7000000);

  s_condition.wakeOne ();
  s_condition.wakeOne ();
  thr1.wait ();
  thr2.wait ();

  EXPECT_EQ (thr1.value (), 10000000);
  EXPECT_EQ (thr2.value (), 10000000);
}
