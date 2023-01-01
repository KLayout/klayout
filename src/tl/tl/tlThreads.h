
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

#ifndef HDR_tlThreads
#define HDR_tlThreads

#include "tlCommon.h"

#include <limits>

#if defined(HAVE_QT) && !defined(HAVE_PTHREADS)
#  include <QMutex>
#  include <QWaitCondition>
#  include <QThread>
#  include <QThreadStorage>
#else
//  atomics taken from https://github.com/mbitsnbites/atomic
#  include "atomic/spinlock.h"
#endif

namespace tl
{

/**
 *  @brief A mutex implementation
 *  This class acts as an abstraction. Qt-based and other implementations are
 *  available.
 */

#if defined(HAVE_QT) && !defined(HAVE_PTHREADS)

class TL_PUBLIC Mutex
  : public QMutex
{
public:
  Mutex () : QMutex () { }
};

#else

//  The non-Qt version is a dummy implementation as threading is not supported (yet)
class TL_PUBLIC Mutex
{
public:
  Mutex () : m_spinlock () { }
  void lock() { m_spinlock.lock(); }
  void unlock() { m_spinlock.unlock(); }
private:
  atomic::spinlock m_spinlock;
};

#endif

/**
 *  @brief A wait condition implementation
 *  This class acts as an abstraction. Qt-based and other implementations are
 *  available.
 */

#if defined(HAVE_QT) && !defined(HAVE_PTHREADS)

class TL_PUBLIC WaitCondition
  : public QWaitCondition
{
public:
  WaitCondition () : QWaitCondition () { }

  bool wait (Mutex *mutex, unsigned long time = std::numeric_limits<unsigned long>::max ()) { return QWaitCondition::wait (mutex, time); }
};

#else

class WaitConditionPrivate;

//  The non-Qt version is a dummy implementation as threading is not supported (yet)
class TL_PUBLIC WaitCondition
{
public:
  WaitCondition ();
  ~WaitCondition ();
  bool wait (Mutex * /*mutex*/, unsigned long /*time*/ = std::numeric_limits<unsigned long>::max ());
  void wakeAll ();
  void wakeOne ();

private:
  WaitConditionPrivate *mp_data;
};

#endif

/**
 *  @brief A RAII-based Mutex locker
 */
class TL_PUBLIC MutexLocker
{
public:
  MutexLocker (Mutex *mutex)
    : mp_mutex (mutex)
  {
    mp_mutex->lock ();
  }

  ~MutexLocker ()
  {
    mp_mutex->unlock ();
  }

private:
  Mutex *mp_mutex;
};

/**
 *  @brief A thread implementation
 *  This class acts as an abstraction. Qt-based and other implementations are
 *  available.
 */

#if defined(HAVE_QT) && !defined(HAVE_PTHREADS)

class TL_PUBLIC Thread
  : public QThread
{
public:
  Thread ()
    : QThread ()
  { }
};

#else

class ThreadPrivateData;

class TL_PUBLIC Thread
{
public:
  Thread ();
  virtual ~Thread ();

  void exit (int /*returnCode*/ = 0);
  bool isFinished () const;
  bool isRunning () const;
  void quit ();
  void start ();
  void terminate ();
  bool wait (unsigned long /*time*/ = std::numeric_limits<unsigned long>::max ());

protected:
  virtual void run () { }

private:
  friend void *start_thread (void *);
  ThreadPrivateData *mp_data;
  void do_run ();
};

#endif

/**
 *  @brief A thread local storage implementation
 *  This class acts as an abstraction. Qt-based and other implementations are
 *  available.
 */

#if defined(HAVE_QT) && !defined(HAVE_PTHREADS)

template <class T>
class ThreadStorage
  : public QThreadStorage<T>
{
public:
  ThreadStorage () : QThreadStorage<T> () { }
};

#else

class TL_PUBLIC ThreadStorageHolderBase
{
public:
  ThreadStorageHolderBase (void *obj) : mp_obj (obj) { }
  virtual ~ThreadStorageHolderBase () { }

protected:
  void *obj () { return mp_obj; }
  void *mp_obj;
};

template <class T>
class ThreadStorageHolder
  : public ThreadStorageHolderBase
{
public:
  ThreadStorageHolder (T *t) : ThreadStorageHolderBase ((void *) t) { }
  ~ThreadStorageHolder () { delete data (); }
  T *data () { return (T *) obj (); }
};

class TL_PUBLIC ThreadStorageBase
{
public:
  ThreadStorageBase ();

protected:
  void add (ThreadStorageHolderBase *holder);
  ThreadStorageHolderBase *holder ();

  const ThreadStorageHolderBase *holder () const
  {
    return (const_cast<ThreadStorageBase *> (this)->holder ());
  }
};

template <class T>
class ThreadStorage
  : public ThreadStorageBase
{
public:
  ThreadStorage () : ThreadStorageBase () { }

  bool hasLocalData () const
  {
    return dynamic_cast<const ThreadStorageHolder<T> *> (holder ()) != 0;
  }

  T &localData ()
  {
    return *(dynamic_cast<ThreadStorageHolder<T> *> (holder ())->data ());
  }

  T localData () const
  {
    return *(dynamic_cast<const ThreadStorageHolder<T> *> (holder ())->data ());
  }

  void setLocalData (const T &data)
  {
    if (hasLocalData ()) {
      localData () = data;
    } else {
      add (new ThreadStorageHolder<T> (new T (data)));
    }
  }
};

#endif

}

#endif
