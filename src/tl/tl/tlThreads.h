
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

#ifndef HDR_tlThreads
#define HDR_tlThreads

#include "tlCommon.h"

#include <limits>

#if defined(HAVE_QT)
#  include <QMutex>
#  include <QWaitCondition>
#  include <QThread>
#  include <QThreadStorage>
#endif

namespace tl
{

/**
 *  @brief A mutex implementation
 *  This class acts as an abstraction. Qt-based and other implementations are
 *  available.
 */

#if defined(HAVE_QT)

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
  Mutex () { }
  void lock () { }
  void unlock () { }
};

#endif

/**
 *  @brief A wait condition implementation
 *  This class acts as an abstraction. Qt-based and other implementations are
 *  available.
 */

#if defined(HAVE_QT)

class TL_PUBLIC WaitCondition
  : public QWaitCondition
{
public:
  WaitCondition () : QWaitCondition () { }

  bool wait (Mutex *mutex, unsigned long time = std::numeric_limits<unsigned long>::max ()) { return QWaitCondition::wait (mutex, time); }
};

#else

//  The non-Qt version is a dummy implementation as threading is not supported (yet)
class TL_PUBLIC WaitCondition
{
public:
  WaitCondition () { }
  bool wait (Mutex * /*mutex*/, unsigned long /*time*/ = std::numeric_limits<unsigned long>::max ()) { return true; }
  void wakeAll () { }
  void wakeOne () { }
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

#if defined(HAVE_QT)

class TL_PUBLIC Thread
  : public QThread
{
public:
  Thread ()
    : QThread ()
  { }
};

#else

//  TODO: this is a non-threaded dummy implementation
class TL_PUBLIC Thread
{
public:
  Thread () { }
  virtual ~Thread () { }

  void exit (int /*returnCode*/ = 0) { }
  bool isFinished () const { return true; }
  bool isRunning () const { return false; }
  void quit () { }
  void start () { run (); }
  void terminate () { }
  bool wait (unsigned long /*time*/ = std::numeric_limits<unsigned long>::max ()) { return true; }

protected:
  virtual void run () { }
};

#endif

/**
 *  @brief A thread local storage implementation
 *  This class acts as an abstraction. Qt-based and other implementations are
 *  available.
 */

#if defined(HAVE_QT)

template <class T>
class TL_PUBLIC ThreadStorage
  : public QThreadStorage<T>
{
public:
  ThreadStorage () : QThreadStorage<T> () { }
};

#else

//  TODO: this is the non-threaded dummy implementation
template <class T>
class TL_PUBLIC ThreadStorage
{
public:
  ThreadStorage () : m_t (), m_has_data (false) { }

  bool hasLocalData () const { return m_has_data; }
  T &localData () { return m_t; }
  T localData () const { return m_t; }
  void setLocalData (const T &data) { m_t = data; m_has_data = true; }

private:
  T m_t;
  bool m_has_data;
};

#endif

}

#endif
