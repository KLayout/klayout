
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


#ifndef HDR_tlThreadedWorkers
#define HDR_tlThreadedWorkers

#include "tlCommon.h"
#include "tlThreads.h"

#include <set>
#include <vector>
#include <string>

namespace tl
{

/**
 *  @brief A threaded worker framework
 *
 *  The threaded worker framework provides a way to control multiple workers performing operations
 *  on some database or data repository object. The framework provides a way to control the workers, in 
 *  particular kick off and shut down worker threads. It does not deal with the issue of the database
 *  object being thread-safe. 
 *
 *  The general concept involves 
 *  1.) A boss (tl::Boss class). There is one boss instance per "database" instance. The boss must be 
 *      in some way associated with the database.
 *      The boss provides a way to control several jobs, i.e. stop them if required. 
 *  2.) Jobs: multiple jobs (tl::Job objects) can be registered under one boss. 
 *      A job is not necessarily running or terminated. It can be started, 
 *      stopped, restarted or terminated. A job is initialized in the main thread and
 *      sets up the threads which actually do the job (workers). 
 *      A job may be associated with multiple boss instances.
 *  3.) Workers: a job can be split into multiple tasks which are executed by the workers. A worker is
 *      a thread which receives tasks through a task queue.
 */

class Boss;
class Worker;
class Task;

/**
 *  @brief A task list
 *
 *  This class is used by Job to store tasks.
 *  This class is not thread-safe.
 */
class TL_PUBLIC TaskList
{
public:
  /**
   *  @brief Default ctor
   */
  TaskList ();

  /**
   *  @brief Destructor 
   */
  ~TaskList ();

  /**
   *  @brief Returns true if the list is empty.
   */
  bool is_empty () const 
  {
    return mp_first == 0;
  }
  
  /**
   *  @brief Fetch the next task
   */
  Task *fetch ();

  /**
   *  @brief Put (append) a task to the task list
   */
  void put (Task *task);

  /**
   *  @brief Put (prepend) a task at the beginning of the task list
   */
  void put_front (Task *task);

  /**
   *  @brief Get the next task without taking it
   */
  const Task *peek () const
  {
    return mp_first;
  }

  /**
   *  @brief Gets the number of tasks
   */
  size_t size () const;

private:
  Task *mp_first, *mp_last;

  TaskList (const TaskList &);
  TaskList &operator= (const TaskList &);
};

/**
 *  @brief This object represents a job
 *
 *  A job can be delegated to multiple workers. 
 *  A job is organized in tasks, which are scheduled to the job. Upon \start,
 *  the job takes the tasks from a queue and sends them to the workers for
 *  being processed.
 */
class TL_PUBLIC JobBase
{
public:
  /**
   *  @brief Job constructor
   *
   *  @param nworkers The number of workers to provide. Can be 0 for non-threaded (synchronous) execution of the tasks.
   */
  JobBase (int nworkers = 1);

  /**
   *  @brief Destructor
   */
  virtual ~JobBase ();

  /**
   *  @brief Returns true, if the job is running
   */
  bool is_running ();

  /**
   *  @brief Stop the job
   *
   *  It is guaranteed that the job is stopped and no worker is active after \stop has been called.
   */
  void stop ();

  /**
   *  @brief Schedule a task for being processed
   *
   *  This does not trigger the actual operation yet. It should be done separately before
   *  \start is called. However, it is possible to schedule jobs while the job is running and
   *  even from within other tasks.
   *  It is guaranteed that the order of processing of the tasks is maintained. However, 
   *  it is not guaranteed that previous tasks have been processed already because they
   *  might be send to a different thread.
   */
  void schedule (Task *task);

  /**
   *  @brief Gets the number of tasks in the queue
   */
  size_t tasks () const
  {
    return m_task_list.size ();
  }

  /**
   *  @brief Start the execution of the job
   */
  void start ();

  /**
   *  @brief Wait for the termination of the job
   *
   *  If the job already has terminated, this method does nothing.
   */
  bool wait (long timeout = -1);

  /**
   *  @brief Terminate all threads in this job.
   *
   *  After the job has been terminated, it can be started again using \start.
   *  The difference between \stop and \terminate is that \terminate will 
   *  stop the threads. 
   */
  void terminate ();

  /**
   *  @brief Gets the number of workers specified for this job
   *
   *  Returns 0 if the job is synchronous.
   */
  int num_workers () const
  {
    return m_nworkers;
  }

  /**
   *  @brief Set the number of workers
   *
   *  This function will stop the job before changing the number of workers.
   *  If the number of workers is set to 0, the job is performed synchronously.
   */
  void set_num_workers (int workers);

  /**
   *  @brief Returns true if an error occurred during run()
   */
  bool has_error ();

  /**
   *  @brief Fetch the collected error messages
   */
  std::vector<std::string> error_messages ();

protected:
  /**
   *  @brief Creates a worker object
   */
  virtual Worker *create_worker () = 0;

  /**
   *  @brief Sets up a worker before the first task in performed
   *
   *  This method is called from the main thread where start() is being called to 
   *  set up all workers.
   *  This method is called from the main thread from which start () was called.
   */
  virtual void setup_worker (Worker * /*worker*/) { }

  /**
   *  @brief This method is called before the given task is started in sync mode (workers == 0)
   */
  virtual void before_sync_task (Task * /*task*/) { }

  /**
   *  @brief This method is called after the given task has finished in sync mode (workers == 0)
   */
  virtual void after_sync_task (Task * /*task*/) { }

  /**
   *  @brief Indicates that the job has finished
   *
   *  This method is called when the last worker has terminated.
   *  It is not called, when the job has been stopped.
   *  Caution: this method is called from the thread that was executing the last task. It is called
   *  before the controlling thread is awakened inside wait().
   */
  virtual void finished () { }

  /**
   *  @brief Indicates that a job has stopped
   *
   *  This method is called when a job is stopped rather than finished
   *  normally. This method is called from the main thread.
   */
  virtual void stopped () { }

  /**
   *  @brief Get the nth worker 
   */
  tl::Worker *worker (int n)
  {
    return mp_workers [n];
  }

private:
  friend class Worker;
  friend class Boss;

  TaskList m_task_list;
  TaskList *mp_per_worker_task_lists;

  int m_nworkers;
  int m_idle_workers;
  bool m_stopping;
  bool m_running;

  tl::Mutex m_lock;
  tl::WaitCondition m_task_available_condition;
  tl::WaitCondition m_queue_empty_condition;

  std::vector<Worker *> mp_workers;
  std::set<Boss *> m_bosses;

  std::vector<std::string> m_error_messages;

  Task *get_task (int for_worker);
  void log_error (const std::string &s);
  void cleanup ();
};

/**
 *  @brief A job specialization for a specific worker class
 */
template <class W>
class Job
  : public JobBase
{
public:
  /**
   *  @brief Constructor: create a job based on the given worker class
   *
   *  @param nworkers The number of workers or 0 if synchronous operation is requested.
   */
  Job (int nworkers = 1)
    : JobBase (nworkers)
  { }

protected:
  virtual Worker *create_worker () 
  {
    return new W();
  }
};

/**
 *  @brief A worker 
 *
 *  The worker is the thread doing the actual work. A worker must be reimplemented to
 *  provide the operation implementation by implementing "perform_task".
 *  A worker is provided with a sequence of tasks with define the operations that the
 *  worker is supposed to perform.
 */
class TL_PUBLIC Worker : protected tl::Thread
{
public:
  friend class JobBase;
  friend class WorkerProgressAdaptor;

  /**
   *  @brief The default ctor
   */
  Worker ();

  /**
   *  @brief The destructor
   */
  virtual ~Worker ();

  /** 
   *  @brief Returns the index of the worker 
   */
  int worker_index () const 
  {
    return m_worker_index;
  }

protected:
  /**
   *  @brief Perform one task
   *
   *  The implementation of this method is supposed to regularly call \checkpoint in order to
   *  receive asynchronous abort requests. The scheduler uses this feature to stop operations
   *  asynchronously.
   */
  virtual void perform_task (Task *task) = 0;

  /**
   *  @brief Check for stop requests
   *
   *  This method should be called regularly. It does nothing if no stop request if present. 
   *  Otherwise it throws an exception which is supposed to make \perform_task exit.
   */
  void checkpoint ();

  /**
   *  @brief Returns true, if a stop is requested
   *
   *  This is the same condition that will make checkpoint () throw an exception.
   */
  bool stop_requested () const
  {
    return m_stop_requested;
  }
  
  /**
   *  @brief Returns true, if the worker is waiting for a task
   */
  bool is_idle () const
  {
    return m_is_idle;
  }

  /**
   *  @brief Sets the idle flag
   */
  void set_idle (bool w)
  {
    m_is_idle = w;
  }

private:
  virtual void run ();
  void stop_request ();
  void reset_stop_request ();
  void start (JobBase *job, int worker_index);

  JobBase *mp_job;
  int m_worker_index;
  bool m_stop_requested;
  bool m_is_idle;
};

/**
 *  @brief Represents one task in the task queue
 *
 *  This object must be reimplemented to provide specific
 *  information for a task. This is the base class of all
 *  task objects.
 */
class TL_PUBLIC Task
{
public:
  /**
   *  @brief Default ctor
   */
  Task () 
    : mp_next (0), mp_last (0)
  { }

  /**
   *  @brief Constructor
   */
  virtual ~Task ()
  { }

private:
  friend class TaskList;

  Task *mp_next, *mp_last;
};

/**
 *  @brief The overall job controller
 *
 *  The Boss object controls multiple jobs.
 *  Job must be registered at the Boss with register_job and must
 *  be unregistered with unregister_job. 
 *  The Boss provides services for stopping all jobs and iterating
 *  over them.
 *  If a job is deleted, it is automatically unregistered at the Boss.
 *  Vice versa, the Boss unregisters itself at all jobs registered 
 *  within it.
 */
class TL_PUBLIC Boss
{
public: 
  typedef std::set<JobBase *>::iterator iterator;

  /**
   *  @brief The default ctor
   */
  Boss ();

  /**
   *  @brief The destructor
   *
   *  The destructor will unregister itself at all jobs present.
   */
  virtual ~Boss ();

  /**
   *  @brief Register the job at the Boss. 
   *
   *  After registering, the job is known to the Boss and can be
   *  controlled by it.
   *  The Boss will not become owner of the job object.
   */
  void register_job (JobBase *job);

  /**
   *  @brief Unregister the job at the Boss. 
   *
   *  After unregistering, the job is no longer known to the Boss and cannot be
   *  controlled by it.
   */
  void unregister_job (JobBase *job);

  /**
   *  @brief Iterate over all jobs: begin iterator
   */
  iterator begin () 
  {
    return m_jobs.begin ();
  }

  /**
   *  @brief Iterate over all jobs: end iterator
   */
  iterator end () 
  {
    return m_jobs.end ();
  }

  /**
   *  @brief Send an asynchronous stop to all jobs registered at the Boss
   */
  void stop_all ();

private:
  std::set<JobBase *> m_jobs;
};

}

#endif

