
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

#if !defined(HAVE_QT) || defined(HAVE_PTHREADS)

#include "tlThreads.h"
#include "tlUtils.h"
#include "tlTimer.h"
#include "tlLog.h"
#include "tlInternational.h"

#include <map>
#define _TIMESPEC_DEFINED   //  avoids errors with pthread-win and MSVC2017
#include <pthread.h>
#include <errno.h>
#if defined(_WIN32)
#  define NOMINMAX
#  include <Windows.h>
#else
#  include <unistd.h>
#endif

namespace tl
{

// -------------------------------------------------------------------------------
//  WaitCondition implementation


class WaitConditionPrivate
{
public:
  WaitConditionPrivate ()
    : m_initialized (false)
  {
    if (pthread_mutex_init (&m_mutex, NULL) != 0) {
      tl::error << tr ("Unable to create pthread Mutex for WaitCondition");
    } else if (pthread_cond_init(&m_cond, NULL) != 0) {
      tl::error << tr ("Unable to create pthread Condition for WaitCondition");
    } else {
      m_initialized = true;
    }
  }

  ~WaitConditionPrivate ()
  {
    if (m_initialized) {
      pthread_cond_destroy (&m_cond);
      pthread_mutex_destroy (&m_mutex);
    }
  }

  bool wait (Mutex *mutex, unsigned long time)
  {
    if (! m_initialized) {
      return false;
    }

    //  transfer the lock from our own implementation to the pthread mutex
    pthread_mutex_lock (&m_mutex);
    mutex->unlock ();

    //  this code is executed concurrently ...

    bool woken = true;

    if (time < std::numeric_limits<unsigned long>::max ()) {

      struct timespec end_time;
      current_utc_time (&end_time);

      end_time.tv_sec += (time / 1000);
      end_time.tv_nsec += (time % 1000) * 1000000;
      if (end_time.tv_nsec > 1000000000) {
        end_time.tv_nsec -= 1000000000;
        end_time.tv_sec += 1;
      }

      int res = pthread_cond_timedwait (&m_cond, &m_mutex, &end_time);
      if (res == ETIMEDOUT) {
        woken = false;
      } else if (res) {
        tl::error << tr ("Error waiting for pthread Condition (timed)");
      }

    } else {

      if (pthread_cond_wait (&m_cond, &m_mutex) != 0) {
        tl::error << tr ("Error waiting for pthread Condition (timed)");
      }

    }

    //  transfers the lock back
    pthread_mutex_unlock (&m_mutex);
    mutex->lock ();

    return woken;
  }

  void wake_all ()
  {
    if (pthread_mutex_lock (&m_mutex) == 0) {
      pthread_cond_broadcast (&m_cond);
      pthread_mutex_unlock (&m_mutex);
    }
  }

  void wake_one ()
  {
    if (pthread_mutex_lock (&m_mutex) == 0) {
      pthread_cond_signal (&m_cond);
      pthread_mutex_unlock (&m_mutex);
    }
  }

private:
  pthread_mutex_t m_mutex;
  pthread_cond_t m_cond;
  bool m_initialized;
};

WaitCondition::WaitCondition ()
{
  mp_data = new WaitConditionPrivate ();
}

WaitCondition::~WaitCondition ()
{
  delete mp_data;
  mp_data = 0;
}

bool WaitCondition::wait (Mutex *mutex, unsigned long time)
{
  return mp_data->wait (mutex, time);
}

void WaitCondition::wakeAll ()
{
  mp_data->wake_all ();
}

void WaitCondition::wakeOne ()
{
  mp_data->wake_one ();
}

// -------------------------------------------------------------------------------
//  Thread implementation

class ThreadPrivateData
{
public:
  ThreadPrivateData ()
    : pthread (), initialized (false), return_code (0), running (false)
  {
    //  .. nothing yet ..
  }

  pthread_t pthread;
  bool initialized;
  void *return_code;
  bool running;
};

void *start_thread (void *data)
{
  ((Thread *) data)->do_run ();
  return 0;
}

Thread::Thread ()
  : mp_data (new ThreadPrivateData ())
{
  //  .. nothing yet ..
}

Thread::~Thread ()
{
  terminate ();
  wait ();
  delete mp_data;
  mp_data = 0;
}

void Thread::do_run ()
{
  try {
    run ();
    mp_data->running = false;
  } catch (tl::Exception &ex) {
    tl::error << tr ("Exception from thread : ") << ex.msg ();
    mp_data->running = false;
  } catch (...) {
    tl::error << tr ("Unspecific exception from thread");
    mp_data->running = false;
  }
}

void Thread::exit (int return_code)
{
  pthread_exit (reinterpret_cast<void *> (size_t (return_code)));
}

bool Thread::isFinished () const
{
  if (! mp_data->initialized) {
    return false;
  } else {
    return ! mp_data->running;
  }
}

bool Thread::isRunning () const
{
  if (! mp_data->initialized) {
    return false;
  } else {
    return mp_data->running;
  }
}

void Thread::quit ()
{
  exit (0);
}

void Thread::start ()
{
  if (isRunning ()) {
    return;
  }

  mp_data->initialized = true;
  mp_data->running = true;
  if (pthread_create (&mp_data->pthread, NULL, &start_thread, (void *) this) != 0) {
    tl::error << tr ("Failed to create thread");
  }
}

void Thread::terminate ()
{
  if (isRunning () && pthread_cancel (mp_data->pthread) != 0) {
    tl::error << tr ("Failed to terminate thread");
  }
}

bool Thread::wait (unsigned long time)
{
  if (! isRunning ()) {
    return true;
  }

  if (time < std::numeric_limits<unsigned long>::max ()) {

    struct timespec end_time;
    current_utc_time (&end_time);

    end_time.tv_sec += (time / 1000);
    end_time.tv_nsec += (time % 1000) * 1000000;
    if (end_time.tv_nsec > 1000000000) {
      end_time.tv_nsec -= 1000000000;
      end_time.tv_sec += 1;
    }

#if defined(_WIN32) || defined(__APPLE__)

    //  wait if the thread terminated or the timeout has expired
    while (isRunning ()) {

      struct timespec current_time;
      current_utc_time (&current_time);
      if (end_time.tv_sec < current_time.tv_sec || (end_time.tv_sec == current_time.tv_sec && end_time.tv_nsec < current_time.tv_nsec)) {
        return false;
      }

#if defined(_WIN32)
      Sleep (1);
#else
      usleep (1000);
#endif

    }

#else

    int res = pthread_timedjoin_np (mp_data->pthread, &mp_data->return_code, &end_time);
    if (res == ETIMEDOUT) {
      return false;
    } else if (res) {
      tl::error << tr ("Could not join threads");
    }

#endif

    return true;

  } else {

    if (pthread_join (mp_data->pthread, &mp_data->return_code) != 0) {
      tl::error << tr ("Could not join threads");
    }

    return true;

  }
}

// -------------------------------------------------------------------------------
//  ThreadStorage implementation

class ThreadStorageObjectList
{
public:
  ThreadStorageObjectList () { }

  ~ThreadStorageObjectList ()
  {
    for (std::map<void *, ThreadStorageHolderBase *>::iterator i = m_objects.begin (); i != m_objects.end (); ++i) {
      delete i->second;
    }
    m_objects.clear ();
  }

  void add (void *index, ThreadStorageHolderBase *holder)
  {
    std::map<void *, ThreadStorageHolderBase *>::iterator h = m_objects.find (holder);
    if (h != m_objects.end ()) {
      delete h->second;
      h->second = holder;
    } else {
      m_objects.insert (std::make_pair (index, holder));
    }
  }

  ThreadStorageHolderBase *get (void *index)
  {
    std::map<void *, ThreadStorageHolderBase *>::const_iterator h = m_objects.find (index);
    return h != m_objects.end () ? h->second : 0;
  }

private:
  std::map<void *, ThreadStorageHolderBase *> m_objects;
};

static pthread_key_t s_storage_key;
static pthread_once_t s_keycreated = PTHREAD_ONCE_INIT;

static void free_key (void *data)
{
  delete (ThreadStorageObjectList *) data;
}

static void create_key ()
{
  pthread_key_create (&s_storage_key, free_key);
}

ThreadStorageBase::ThreadStorageBase ()
{
  //  .. nothing yet ..
}

void ThreadStorageBase::add (ThreadStorageHolderBase *holder)
{
  pthread_once (&s_keycreated, create_key);
  if (! pthread_getspecific (s_storage_key)) {
    pthread_setspecific (s_storage_key, new ThreadStorageObjectList ());
  }

  ((ThreadStorageObjectList *) pthread_getspecific (s_storage_key))->add ((void *) this, holder);
}

ThreadStorageHolderBase *ThreadStorageBase::holder ()
{
  pthread_once (&s_keycreated, create_key);
  if (! pthread_getspecific (s_storage_key)) {
    return 0;
  } else {
    return ((ThreadStorageObjectList *) pthread_getspecific (s_storage_key))->get ((void *) this);
  }
}

}

#endif
