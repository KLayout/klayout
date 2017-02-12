
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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



#ifndef HDR_tlDeferredExecution
#define HDR_tlDeferredExecution

#include "tlCommon.h"
#include "tlObject.h"

#include <QObject>
#include <QTimer>
#include <QMutex>

#include <list>

namespace tl
{

/**
 *  @brief A base class for method descriptors to be executed later
 *
 */
class TL_PUBLIC DeferredMethodBase
  : public tl::Object
{
public:
  DeferredMethodBase (bool compressed) 
    : m_compressed (compressed), m_scheduled (false)
  {
    //  .. nothing yet ..
  }

  virtual ~DeferredMethodBase () { }
  virtual void execute () = 0;

private:
  bool m_compressed, m_scheduled;

  //  see comment below about who manages m_compressed and m_scheduled.
  friend class DeferredMethodScheduler;
};

template <class T> class DeferredMethod;

/**
 *  @brief The deferred method scheduler
 */
class TL_PUBLIC DeferredMethodScheduler
  : public QObject
{
Q_OBJECT
public:
  /**
   *  @brief The singleton instance of the scheduler
   */
  static DeferredMethodScheduler *instance ();

  /**
   *  @brief Schedule a call to the specified method
   */
  void schedule (DeferredMethodBase *method);

  /**
   *  @brief Remove the specified method call(s) from the call queue
   */
  void unqueue (DeferredMethodBase *method);

  /**
   *  @brief Enable or disable execution of deferred calls
   *
   *  One use case for this method is to block execution of these methods
   *  when the application is supposed only to show a progress bar and
   *  therefore will process events but is not supposed to execute anything else.
   *
   *  Enabling is cumulative: multiple enable(true) calls must be matched to 
   *  the same number of enable(false) calls. 
   */
  void enable (bool en);

  /**
   *  @brief Execute all queued methods
   *  
   *  This method can be called to force execution of all queued methods.
   */
  void execute ();

private slots:
  void timer ();

private:
  int m_disabled;
  bool m_scheduled;
  std::list<DeferredMethodBase *> m_methods;
  QTimer m_timer, m_fallback_timer;
  QMutex m_lock;

  DeferredMethodScheduler (QObject *parent);
  ~DeferredMethodScheduler ();

  virtual bool event (QEvent *event);
};

/**
 *  @brief Deferred execution of a const method
 *
 *  This template allows to schedule a deferred execution of a certain method.
 *  The method is not called immediately but as soon as the applications becomes
 *  idle - i.e. GUI events are being processed.
 *  This allows to schedule GUI update requests or similar. 
 *  The compress parameter controls whether multiple calls to the same method are 
 *  compressed and combined into a single call. This is an efficient way to 
 *  schedule a single GUI update request from frequently called and time critical
 *  code for example.
 *
 *  The intention is to create these objects as members of a class:
 *
 *  class T {
 *  public:
 *    T : m_def(this, &T::m) { }
 *    void f() {
 *      m_def(); // deferred call to m() 
 *    }
 *    void m() { .. }
 *  private:
 *    DeferredMethod<T> m_def;
 *  };
 *
 *  The call can be scheduled from any thread but will always be executed in the
 *  main thread.
 */

template <class T>
class DeferredMethod
  : public DeferredMethodBase
{
public:
  /**
   *  @brief Construct a deferred method call from a non-const method
   */
  DeferredMethod (T *t, void (T::*method)(), bool compressed = true)
    : DeferredMethodBase (compressed), mp_t (t), m_method (method)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Destructor
   */
  ~DeferredMethod ()
  {
    DeferredMethodScheduler::instance ()->unqueue (this);
  }

  /**
   *  @brief Schedule a call to the method
   */
  void operator() ()
  {
    //  management of m_compressed and m_scheduled is done in the DeferredMethodScheduler -
    //  there it is safely locked. Doing it there saves a private mutex for this object.
    DeferredMethodScheduler::instance ()->schedule (this);
  }

  /**
   *  @brief Cancel any pending calls
   */
  void cancel ()
  {
    DeferredMethodScheduler::instance ()->unqueue (this);
  }

  /**
   *  @brief Execute the call immediately
   */
  void execute ()
  {
    //  cancel execution which might be pending
    DeferredMethodScheduler::instance ()->unqueue (this);
    (mp_t->*m_method) ();
  }

private:
  T *mp_t;
  void (T::*m_method) ();
};

}



#endif

