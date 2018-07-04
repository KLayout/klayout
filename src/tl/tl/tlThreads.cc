
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

#if !defined(HAVE_QT)

#include "tlThreads.h"
#include "tlLog.h"
#include "tlInternational.h"

#include <map>
#include <pthread.h>

namespace tl
{

// -------------------------------------------------------------------------------
//  WaitCondition implementation

WaitCondition::WaitCondition ()
{
  // @@@
}

bool WaitCondition::wait (Mutex *mutex, unsigned long /*time*/)
{
  mutex->unlock();
  // @@@
  mutex->lock();
  return true;
}

void WaitCondition::wakeAll ()
{
  // @@@
}

void WaitCondition::wakeOne ()
{
  // @@@
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
  delete mp_data;
  mp_data = 0;
}

void Thread::do_run ()
{
  try {
    mp_data->running = true;
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

  if (! mp_data->initialized) {
    mp_data->initialized = true;
    if (pthread_create (&mp_data->pthread, NULL, &start_thread, (void *) this) != 0) {
      tl::error << tr ("Failed to create thread");
    }
  }
}

void Thread::terminate ()
{
  if (isRunning () && pthread_cancel (mp_data->pthread) != 0) {
    tl::error << tr ("Failed to terminate thread");
  }
}

bool Thread::wait (unsigned long /*time*/)
{
  if (! mp_data->initialized) {
    return true;
  }

  //  @@@ TODO: timed join
  if (pthread_join (mp_data->pthread, &mp_data->return_code) != 0) {
    tl::error << tr ("Could not join threads");
  }
  return true;
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
