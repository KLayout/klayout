
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


#include "tlThreadedWorkers.h"
#include "tlLog.h"
#include "tlProgress.h"
#include "tlAssert.h"

#include <memory>
#include <stdio.h>

namespace tl
{

// -----------------------------------------------------------------------------
//  Some definitions

//  The maximum number of errors collected per worker
const size_t max_errors = 100;

// -----------------------------------------------------------------------------
//  Some special tasks and exceptions

class ExitTask : public Task 
{
public:
  ExitTask ()
  { }
};

class StartTask : public Task 
{
public:
  StartTask ()
  { }
};

struct WorkerTerminatedException { };
struct TaskTerminatedException { };

// -----------------------------------------------------------------------------
//  tl::Boss implementation

Boss::Boss ()
{
  //  .. nothing yet ..
}

Boss::~Boss ()
{
  //  unregister ourself all jobs
  for (iterator j = begin (); j != end (); ++j) {
    (*j)->m_bosses.erase (this);
  }
  m_jobs.clear ();
}

void 
Boss::register_job (JobBase *job)
{
  m_jobs.insert (job);
  job->m_bosses.insert (this);
}

void 
Boss::unregister_job (JobBase *job)
{
  m_jobs.erase (job);
  job->m_bosses.erase (this);
}

void 
Boss::stop_all ()
{
  for (iterator j = begin (); j != end (); ++j) {
    (*j)->stop ();
  }
}

// -----------------------------------------------------------------------------
//  tl::TaskList implementation

TaskList::TaskList ()
  : mp_first (0), mp_last (0)
{
  // .. nothing yet ..
}

TaskList::~TaskList ()
{
  while (! is_empty ()) {
    delete fetch ();
  }
}

Task *
TaskList::fetch ()
{
  Task *task = mp_first;

  mp_first = task->mp_next;
  if (! mp_first) {
    mp_last = 0;
  } else {
    mp_first->mp_last = 0;
  }

  tl_assert (task->mp_last == 0);
  task->mp_next = 0;

  return task;
}

void 
TaskList::put (Task *task)
{
  task->mp_next = 0;
  task->mp_last = mp_last;

  mp_last = task;
  if (task->mp_last) {
    task->mp_last->mp_next = task;
  } else {
    mp_first = task;
  }
}

void 
TaskList::put_front (Task *task)
{
  task->mp_last = 0;
  task->mp_next = mp_first;

  mp_first = task;
  if (task->mp_next) {
    task->mp_next->mp_last = task;
  } else {
    mp_last = task;
  }
}

size_t
TaskList::size () const
{
  size_t n = 0;
  for (Task *t = mp_first; t; t = t->mp_next) {
    ++n;
  }
  return n;
}

// -----------------------------------------------------------------------------
//  tl::JobBase implementation

JobBase::JobBase (int nworkers)
  : m_nworkers (nworkers), m_idle_workers (0), m_stopping (false), m_running (false)
{
  if (nworkers > 0) {
    mp_per_worker_task_lists = new TaskList[nworkers];
  } else {
    mp_per_worker_task_lists = 0;
  }
}

JobBase::~JobBase ()
{
  terminate ();

  while (! m_bosses.empty ()) {
    (*(m_bosses.begin ()))->unregister_job (this);
  }

  if (mp_per_worker_task_lists) {
    delete[] mp_per_worker_task_lists;
    mp_per_worker_task_lists = 0;
  }
}

void
JobBase::log_error (const std::string &s)
{
  tl::error << tl::to_string (tr ("Worker thread: ")) << s;

  m_lock.lock ();
  if (m_error_messages.size () == max_errors) {
    m_error_messages.push_back (tl::to_string (tr ("Error list abbreviated (more errors were ignored)")));
  } else if (m_error_messages.size () < max_errors) {
    m_error_messages.push_back (s);
  }
  m_lock.unlock ();
}

bool
JobBase::has_error () 
{
  bool r;
  m_lock.lock ();
  r = ! m_error_messages.empty ();
  m_lock.unlock ();
  return r;
}

std::vector<std::string>
JobBase::error_messages () 
{
  std::vector<std::string> r;
  m_lock.lock ();
  r = m_error_messages;
  m_lock.unlock ();
  return r;
}

void
JobBase::set_num_workers (int nworkers)
{
  terminate ();

  m_nworkers = nworkers;
  m_idle_workers = 0;

  if (mp_per_worker_task_lists) {
    delete[] mp_per_worker_task_lists;
  }

  if (nworkers > 0) {
    mp_per_worker_task_lists = new TaskList[nworkers];
  } else {
    mp_per_worker_task_lists = 0;
  }
}

void 
JobBase::start ()
{
  m_lock.lock ();

  m_error_messages.clear ();

  tl_assert (! m_running);

  m_running = true;
  
  //  Add a start task for each worker
  //  This serves as a synchronization measure such that each task gets called once and
  //  the empty queue detection works properly.
  for (int i = 0; i < m_nworkers; ++i) {
    mp_per_worker_task_lists[i].put_front (new StartTask ());
  }

  m_task_available_condition.wakeAll ();

  while (m_nworkers > int (mp_workers.size ())) {
    mp_workers.push_back (create_worker ());
    mp_workers.back ()->start (this, int (mp_workers.size ()) - 1);
  }

  while (m_nworkers < int (mp_workers.size ())) {
    delete mp_workers.back ();
    mp_workers.pop_back ();
  }

  for (int i = 0; i < int (mp_workers.size ()); ++i) {
    setup_worker (mp_workers [i]);
    mp_workers [i]->reset_stop_request ();
  }

  m_lock.unlock ();

  if (mp_workers.empty ()) {

    //  synchronous case: create a temporary worker and 
    //  perform the tasks in the order they were delivered
    std::unique_ptr <Worker> sync_worker (create_worker ());
    setup_worker (sync_worker.get ());

    try {

      while (! m_task_list.is_empty ()) {

        std::unique_ptr<Task> task (m_task_list.fetch ());
        before_sync_task (task.get ());

        try {
          sync_worker->perform_task (task.get ());
        } catch (TaskTerminatedException) {
          //  Stop the thread.
          break;
        } catch (WorkerTerminatedException) {
          //  Stop the thread.
          break;
        } catch (tl::Exception &ex) {
          log_error (ex.msg ());
        } catch (std::exception &ex) {
          log_error (ex.what ());
        } catch (...) {
          log_error (tl::to_string (tr ("Unspecific error")));
        }

        after_sync_task (task.get ());

      }

    } catch (...) {
      //  handle exceptions raised by before_sync_task or after_sync_task
      cleanup ();
      m_running = false;
      throw;
    }

    cleanup ();
    finished ();
    m_running = false;

  }
}

void
JobBase::cleanup ()
{
  //  clean up any remaining tasks
  while (! m_task_list.is_empty ()) {
    Task *task = m_task_list.fetch ();
    if (task) {
      delete task;
    }
  }
}

bool 
JobBase::is_running () 
{
  return m_running;
}

bool 
JobBase::wait (long timeout) 
{
  //  return value will be false if the wait timed out
  bool status = true;

  m_lock.lock ();
  if (m_nworkers > 0 && m_running && ! m_queue_empty_condition.wait (&m_lock, timeout >= 0 ? (unsigned long) timeout : std::numeric_limits<unsigned long>::max ())) {
    status = false;
  }
  m_lock.unlock ();

  return status;
}

void 
JobBase::stop ()
{
  if (! m_running) {
    return;
  }

  m_lock.lock ();

  m_stopping = true;

  //  Remove all pending tasks
  while (! m_task_list.is_empty ()) {
    delete m_task_list.fetch ();
  }

  if (! mp_workers.empty ()) {

    bool any_working = false;

    //  Add a stop task for each worker which is not idle
    for (int i = 0; i < int (mp_workers.size ()); ++i) {
      if (! mp_workers[i]->is_idle ()) {
        mp_workers [i]->stop_request ();
        any_working = true;
      }
    }

    if (any_working) {

      //  signal that we have new tasks
      m_task_available_condition.wakeAll ();

      //  Wait for the stop tasks being processed ...
      m_queue_empty_condition.wait (&m_lock);

    }

    //  Unless new tasks are scheduled, we can be sure that now all workers
    //  are idle.
  }

  m_stopping = false;
  m_running = false;

  m_lock.unlock ();

  stopped ();
}

void
JobBase::terminate ()
{
  stop ();

  if (! mp_workers.empty ()) {

    m_lock.lock ();

    //  Add a stop task for each worker and request a stop
    for (int i = 0; i < int (mp_workers.size ()); ++i) {
      mp_workers [i]->stop_request ();
      mp_per_worker_task_lists[i].put (new ExitTask ());
    }

    //  signal that we have new tasks
    m_task_available_condition.wakeAll ();

    //  Unless new tasks are scheduled, we can be sure that now all workers
    //  are terminating.

    m_lock.unlock ();

    //  Wait for the threads to complete
    for (int i = 0; i < int (mp_workers.size ()); ++i) {
      mp_workers [i]->wait ();
    }

    for (std::vector <Worker *>::iterator w = mp_workers.begin (); w != mp_workers.end (); ++w) {
      delete (*w);
    }

    mp_workers.clear ();

  }
}

void 
JobBase::schedule (Task *task)
{
  m_lock.lock ();

  if (m_stopping) {

    //  Don't allow tasks to be scheduled while stopping or exiting (waiting for m_queue_empty_condition)
    delete task;

  } else {

    //  Add the task to the task queue
    m_task_list.put (task);

    if (m_running) {
      m_task_available_condition.wakeAll ();
    }

  }

  m_lock.unlock ();
}

Task *
JobBase::get_task (int worker)
{
  while (true) {

    m_lock.lock ();

    //  wait for new relevant entries in the task queue
    while (m_task_list.is_empty () && mp_per_worker_task_lists [worker].is_empty ()) {

      //  if the queue is empty, mark this worker as idle.
      ++m_idle_workers;

      //  signal empty queue if all workers are waiting
      if (m_idle_workers == m_nworkers) {
        if (! m_stopping) {
          finished ();
        }
        m_running = false;
        m_queue_empty_condition.wakeAll ();
      }

      //  wait until we receive a task
      while (m_task_list.is_empty () && mp_per_worker_task_lists [worker].is_empty ()) {
        mp_workers [worker]->set_idle (true);
        m_task_available_condition.wait (&m_lock);
        mp_workers [worker]->set_idle (false);
      }

      --m_idle_workers;

    } 

    Task *task = 0;
    if (! mp_per_worker_task_lists [worker].is_empty ()) {
      task = mp_per_worker_task_lists [worker].fetch ();
    } else if (! m_task_list.is_empty ()) {
      task = m_task_list.fetch ();
    }

    m_lock.unlock ();

    if (dynamic_cast <ExitTask *> (task) != 0) {
      delete task;
      //  stops the thread
      throw WorkerTerminatedException ();
    } else if (dynamic_cast <StartTask *> (task) != 0) {
      delete task;
      //  dummy task for synchronization - wait for new tasks to arrive.
    } else if (task) {
      return task;
    }

  }
}

// -----------------------------------------------------------------------------
//  tl::WorkerProgressAdaptor definition and implementation

/**
 *  @brief A progress adaptor that will create a thread-specific progress environment
 *
 *  Currently the main focus is on providing a "cancel" condition.
 */
class TL_PUBLIC WorkerProgressAdaptor : public tl::ProgressAdaptor
{
public:
  WorkerProgressAdaptor (Worker *worker);
  
  virtual void trigger (Progress *progress);
  virtual void yield (Progress *progress);

private:
  Worker *mp_worker;
};

WorkerProgressAdaptor::WorkerProgressAdaptor (Worker *worker)
  : mp_worker (worker)
{
  // .. nothing yet .. 
}
  
void WorkerProgressAdaptor::trigger (Progress * /*progress*/)
{
  // .. nothing yet .. 
}

void WorkerProgressAdaptor::yield (Progress * /*progress*/)
{
  //  throw an exception if the job is aborted.
  mp_worker->checkpoint ();
}

// -----------------------------------------------------------------------------
//  tl::Worker implementation

Worker::Worker ()
  : mp_job (0), m_worker_index (-1), m_stop_requested (false), m_is_idle (false)
{
  // .. nothing yet ..
}

Worker::~Worker ()
{
  // .. nothing yet ..
}

void 
Worker::start (JobBase *job, int worker_index)
{
  mp_job = job;
  m_worker_index = worker_index;
  tl::Thread::start ();
}

void 
Worker::checkpoint ()
{
  if (m_stop_requested) {
    throw TaskTerminatedException ();
  }
}

void  
Worker::run ()
{
  WorkerProgressAdaptor progress_adaptor (this);

  while (true)
  {
    try {
      std::unique_ptr<Task> task (mp_job->get_task (m_worker_index));
      perform_task (task.get ());
    } catch (TaskTerminatedException) {
      //  .. try again
    } catch (WorkerTerminatedException) {
      //  Stop the thread.
      break;
    } catch (tl::Exception &ex) {
      mp_job->log_error (ex.msg ());
    } catch (std::exception &ex) {
      mp_job->log_error (ex.what ());
    } catch (...) {
      mp_job->log_error (tl::to_string (tr ("Unspecific error")));
    }
  }
}

void 
Worker::reset_stop_request ()
{
  m_stop_requested = false;
}

void 
Worker::stop_request ()
{
  m_stop_requested = true;
}

}

