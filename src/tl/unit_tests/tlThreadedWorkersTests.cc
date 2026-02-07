
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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
#include "tlTimer.h"
#include "tlUnitTest.h"
#include "tlThreads.h"
#include "tlSleep.h"

#include <stdio.h>

class Sum
{
public:
  Sum () : m_sum(0), m_flag (false) { }

  void reset () { lock.lock (); m_sum = 0; lock.unlock (); m_flag = false; }
  void add(int n) { lock.lock (); m_sum += n; lock.unlock (); m_flag = true; }

  int sum() {
    int s;
    lock.lock ();
    s = m_sum;
    lock.unlock ();
    return s;
  }

  bool flag() const { return m_flag; }

private:
  tl::Mutex lock;
  int m_sum;
  volatile bool m_flag;
};

static Sum s_sum[4];

class SchedulerTask : public tl::Task
{
public:
  SchedulerTask (tl::JobBase *job, int m, int n) : mp_job (job), m_m (m), m_n (n) { }
  tl::JobBase *mp_job;
  int m_m, m_n;
};

class MyTask : public tl::Task
{
public:
  MyTask (int n) : m_n (n) { }
  int m_n;
};

class MyWorker : public tl::Worker
{
public:
  MyWorker () : tl::Worker () { }

protected:
  void perform_task(tl::Task *task) 
  { 
    MyTask *mytask = dynamic_cast<MyTask *> (task);
    if (mytask) {
      for (int i = 0; i < mytask->m_n; ++i) {
        checkpoint ();
        if (worker_index () >= 0) {
          s_sum[worker_index ()].add (1);
        } else {
          s_sum[0].add (1);
        }
      }
    } else {
      SchedulerTask *schtask = dynamic_cast<SchedulerTask *> (task);
      if (schtask) {
        for (int i = 0; i < schtask->m_m; ++i) {
          schtask->mp_job->schedule (new MyTask (schtask->m_n));
        }
      }
    }
  }
};

class MyJob : public tl::Job<MyWorker>
{
public:
  MyJob (int w) : tl::Job<MyWorker> (w) { }
  std::string m_name;
};

TEST(1) 
{
  size_t n;

  tl::Boss boss1, boss2;
  MyJob *job1 = new MyJob (2);
  MyJob *job2 = new MyJob (1);

  boss1.register_job(job1);
  boss2.register_job(job1);

  n = 0; for (tl::Boss::iterator j = boss1.begin (); j != boss1.end (); ++j) { ++n; }
  EXPECT_EQ (n, size_t (1));

  n = 0; for (tl::Boss::iterator j = boss2.begin (); j != boss2.end (); ++j) { ++n; }
  EXPECT_EQ (n, size_t (1));

  delete job1;
  job1 = new MyJob (2);

  n = 0; for (tl::Boss::iterator j = boss1.begin (); j != boss1.end (); ++j) { ++n; }
  EXPECT_EQ (n, size_t (0));

  n = 0; for (tl::Boss::iterator j = boss2.begin (); j != boss2.end (); ++j) { ++n; }
  EXPECT_EQ (n, size_t (0));

  tl::Boss *tmp_boss = new tl::Boss ();
  tmp_boss->register_job(job1);
  tmp_boss->register_job(job2);
  boss1.register_job(job1);
  boss2.register_job(job1);
  boss2.register_job(job2);
  delete tmp_boss;

  n = 0; for (tl::Boss::iterator j = boss1.begin (); j != boss1.end (); ++j) { ++n; }
  EXPECT_EQ (n, size_t (1));

  n = 0; for (tl::Boss::iterator j = boss2.begin (); j != boss2.end (); ++j) { ++n; }
  EXPECT_EQ (n, size_t (2));

  delete job1;
  delete job2;

  n = 0; for (tl::Boss::iterator j = boss1.begin (); j != boss1.end (); ++j) { ++n; }
  EXPECT_EQ (n, size_t (0));

  n = 0; for (tl::Boss::iterator j = boss2.begin (); j != boss2.end (); ++j) { ++n; }
  EXPECT_EQ (n, size_t (0));
}

TEST(2) 
{
  MyJob job (1);

  s_sum[0].reset ();

  for (int i = 0; i < 100; ++i) {
    job.schedule (new MyTask (100000));
  }

  job.start ();

  tl::usleep (2000000);

  EXPECT_EQ (s_sum[0].sum (), 10000000);
}

TEST(3) 
{
  MyJob job (1);

  s_sum[0].reset ();

  for (int i = 0; i < 100; ++i) {
    job.schedule (new MyTask (100000));
  }

  job.start ();
  int status = job.wait ();

  EXPECT_EQ (status, 1);
  EXPECT_EQ (s_sum[0].sum (), 10000000);
}

TEST(4) 
{
  MyJob job (1);

  s_sum[0].reset ();

  for (int i = 0; i < 10000; ++i) {
    job.schedule (new MyTask (100000));
  }

  job.start ();
  bool status = job.wait (100);

  EXPECT_EQ (status, false /*timed out*/);
  EXPECT_EQ (s_sum[0].sum () < 10000000, true);
}

TEST(5) 
{
  MyJob job (1);

  s_sum[0].reset ();

  for (int i = 0; i < 10000; ++i) {
    job.schedule (new MyTask (100000));
  }

  job.start ();

  tl::usleep (100000);
  job.terminate ();

  EXPECT_EQ (s_sum[0].sum () < 10000000, true);
}

TEST(10) 
{
  MyJob job (4);

  s_sum[0].reset ();
  s_sum[1].reset ();
  s_sum[2].reset ();
  s_sum[3].reset ();

  for (int i = 0; i < 1000; ++i) {
    job.schedule (new MyTask (10000));
  }

  job.start ();

  tl::usleep (2000000);

  EXPECT_EQ (s_sum[0].sum () > 0, true);
  EXPECT_EQ (s_sum[1].sum () > 0, true);
  EXPECT_EQ (s_sum[2].sum () > 0, true);
  EXPECT_EQ (s_sum[3].sum () > 0, true);
  EXPECT_EQ (s_sum[0].sum () % 1000, 0);
  EXPECT_EQ (s_sum[1].sum () % 1000, 0);
  EXPECT_EQ (s_sum[2].sum () % 1000, 0);
  EXPECT_EQ (s_sum[3].sum () % 1000, 0);
  EXPECT_EQ (s_sum[0].sum () + s_sum[1].sum() + s_sum[2].sum() + s_sum[3].sum(), 10000000);
}

TEST(11) 
{
  MyJob job (4);

  s_sum[0].reset ();
  s_sum[1].reset ();
  s_sum[2].reset ();
  s_sum[3].reset ();

  for (int i = 0; i < 10000; ++i) {
    job.schedule (new MyTask (1000));
  }

  job.start ();
  int status = job.wait ();

  EXPECT_EQ (job.is_running (), false);

  EXPECT_EQ (status, 1);

  EXPECT_EQ (s_sum[0].sum () > 0, true);
  EXPECT_EQ (s_sum[1].sum () > 0, true);
  EXPECT_EQ (s_sum[2].sum () > 0, true);
  EXPECT_EQ (s_sum[3].sum () > 0, true);
  EXPECT_EQ (s_sum[0].sum () % 1000, 0);
  EXPECT_EQ (s_sum[1].sum () % 1000, 0);
  EXPECT_EQ (s_sum[2].sum () % 1000, 0);
  EXPECT_EQ (s_sum[3].sum () % 1000, 0);
  EXPECT_EQ (s_sum[0].sum () + s_sum[1].sum() + s_sum[2].sum() + s_sum[3].sum(), 10000000);

  //  check the restart capabilities ..
  job.terminate ();

  for (int i = 0; i < 10000; ++i) {
    job.schedule (new MyTask (1000));
  }

  job.start ();
  status = job.wait ();

  EXPECT_EQ (job.is_running (), false);

  EXPECT_EQ (status, 1);

  EXPECT_EQ (s_sum[0].sum () > 0, true);
  EXPECT_EQ (s_sum[1].sum () > 0, true);
  EXPECT_EQ (s_sum[2].sum () > 0, true);
  EXPECT_EQ (s_sum[3].sum () > 0, true);
  EXPECT_EQ (s_sum[0].sum () % 1000, 0);
  EXPECT_EQ (s_sum[1].sum () % 1000, 0);
  EXPECT_EQ (s_sum[2].sum () % 1000, 0);
  EXPECT_EQ (s_sum[3].sum () % 1000, 0);
  EXPECT_EQ (s_sum[0].sum () + s_sum[1].sum() + s_sum[2].sum() + s_sum[3].sum(), 20000000);
}

TEST(12) 
{
  MyJob job (4);

  s_sum[0].reset ();
  s_sum[1].reset ();
  s_sum[2].reset ();
  s_sum[3].reset ();

  for (int i = 0; i < 10000; ++i) {
    job.schedule (new MyTask (1000));
  }

  job.start ();
  bool status = job.wait (100);

  EXPECT_EQ (status, false /*timed out*/);
  EXPECT_EQ (job.is_running (), true);

  //  at least one must be caught in the perform task ...
  EXPECT_EQ ((s_sum[0].sum () % 1000) + (s_sum[1].sum () % 1000) + (s_sum[2].sum () % 1000) + (s_sum[3].sum () % 1000) > 0, true);
  EXPECT_EQ (s_sum[0].sum () + s_sum[1].sum() + s_sum[2].sum() + s_sum[3].sum() < 10000000, true);
}

void run_thread_tests (tl::TestBase *_this, int wait)
{
  int tries = 4;
  bool stopped_in_action = false;

  for (int i = 0; i < tries && !stopped_in_action; ++i) {

    MyJob job (4);

    s_sum[0].reset ();
    s_sum[1].reset ();
    s_sum[2].reset ();
    s_sum[3].reset ();

    for (int i = 0; i < 10000; ++i) {
      job.schedule (new MyTask (100000));
    }

    job.start ();
    tl::usleep (wait);
    job.terminate ();

    EXPECT_EQ (job.is_running (), false);
    EXPECT_EQ (s_sum[0].sum () + s_sum[1].sum() + s_sum[2].sum() + s_sum[3].sum() < 400000000, true);

    //  at least one must be stopped in the perform task - as this is not always the case,
    //  we retry a few times.
    stopped_in_action = (s_sum[0].sum () % 10000) + (s_sum[1].sum () % 10000) + (s_sum[2].sum () % 10000) + (s_sum[3].sum () % 10000) > 0;

  }

  EXPECT_EQ (stopped_in_action, true);
}

TEST(13)
{
  run_thread_tests (_this, 20000);
}

TEST(14)
{
  run_thread_tests (_this, 200000);
}

TEST(20) 
{
  MyJob job (4);

  for (int l = 0; l < 100; ++l) {

    s_sum[0].reset ();
    s_sum[1].reset ();
    s_sum[2].reset ();
    s_sum[3].reset ();

    for (int i = 0; i < 1000; ++i) {
      job.schedule (new MyTask (100));
    }

    job.start ();
    job.wait ();
    EXPECT_EQ (job.is_running (), false);

    EXPECT_EQ (s_sum[0].sum () + s_sum[1].sum() + s_sum[2].sum() + s_sum[3].sum(), 100000);

  }
}

TEST(21) 
{
  MyJob job (1);

  for (int l = 0; l < 100; ++l) {

    s_sum[0].reset ();

    for (int i = 0; i < 1000; ++i) {
      job.schedule (new MyTask (100));
    }

    job.start ();
    job.wait ();
    EXPECT_EQ (job.is_running (), false);

    EXPECT_EQ (s_sum[0].sum (), 100000);

  }
}

TEST(22) 
{
  tl::SelfTimer timer ("4 threads, 20 iterations with all threads running");
  MyJob job (4);

  for (int l = 0; l < 20; ++l) {

    s_sum[0].reset ();
    s_sum[1].reset ();
    s_sum[2].reset ();
    s_sum[3].reset ();

    for (int i = 0; i < 1000; ++i) {
      job.schedule (new MyTask (100000));
    }

    job.start ();
    while (!s_sum[0].flag () || !s_sum[1].flag () || !s_sum[2].flag () || !s_sum[3].flag ()) {
      tl::usleep (10000);
    }
    job.stop ();
    EXPECT_EQ (job.is_running (), false);

    EXPECT_EQ (s_sum[0].sum () + s_sum[1].sum() + s_sum[2].sum() + s_sum[3].sum() < 100000000, true);

  }
}

TEST(23) 
{
  tl::SelfTimer timer ("2 threads, 40 iterations with all threads running");
  MyJob job (2);

  for (int l = 0; l < 40; ++l) {

    s_sum[0].reset ();
    s_sum[1].reset ();
    s_sum[2].reset ();
    s_sum[3].reset ();

    for (int i = 0; i < 1000; ++i) {
      job.schedule (new MyTask (100000));
    }

    job.start ();
    while (!s_sum[0].flag () || !s_sum[1].flag ()) {
      tl::usleep (10000);
    }
    job.stop ();
    EXPECT_EQ (job.is_running (), false);

    EXPECT_EQ (s_sum[0].sum () + s_sum[1].sum() < 100000000, true);

  }
}

TEST(24) 
{
  tl::SelfTimer timer ("4 threads, 20 iterations with at least one thread running");
  MyJob job (4);

  for (int l = 0; l < 20; ++l) {

    s_sum[0].reset ();
    s_sum[1].reset ();
    s_sum[2].reset ();
    s_sum[3].reset ();

    for (int i = 0; i < 1000; ++i) {
      job.schedule (new MyTask (100000));
    }

    job.start ();
    while (!s_sum[0].flag () && !s_sum[1].flag () && !s_sum[2].flag () && !s_sum[3].flag ()) {
      tl::usleep (10000);
    }
    job.stop ();
    EXPECT_EQ (job.is_running (), false);

    EXPECT_EQ (s_sum[0].sum () + s_sum[1].sum() + s_sum[2].sum() + s_sum[3].sum() < 100000000, true);

  }
}

TEST(25) 
{
  tl::SelfTimer timer ("2 threads, 40 iterations with all at least one thread running");
  MyJob job (2);

  for (int l = 0; l < 40; ++l) {

    s_sum[0].reset ();
    s_sum[1].reset ();
    s_sum[2].reset ();
    s_sum[3].reset ();

    for (int i = 0; i < 1000; ++i) {
      job.schedule (new MyTask (100000));
    }

    job.start ();
    while (!s_sum[0].flag () && !s_sum[1].flag ()) {
      tl::usleep (10000);
    }
    job.stop ();
    EXPECT_EQ (job.is_running (), false);

    EXPECT_EQ (s_sum[0].sum () + s_sum[1].sum() < 100000000, true);

  }
}

TEST(26) 
{
  tl::SelfTimer timer ("2 threads, 500 iterations with waiting");
  MyJob job (2);

  for (int l = 0; l < 500; ++l) {

    s_sum[0].reset ();
    s_sum[1].reset ();
    s_sum[2].reset ();
    s_sum[3].reset ();

    for (int i = 0; i < 100; ++i) {
      job.schedule (new MyTask (100));
    }

    job.start ();
    job.wait ();
    EXPECT_EQ (job.is_running (), false);

    EXPECT_EQ (s_sum[0].sum () + s_sum[1].sum() == 10000, true);

  }
}

TEST(27) 
{
  tl::SelfTimer timer ("4 threads, 500 iterations with waiting");
  MyJob job (4);

  for (int l = 0; l < 500; ++l) {

    s_sum[0].reset ();
    s_sum[1].reset ();
    s_sum[2].reset ();
    s_sum[3].reset ();

    for (int i = 0; i < 100; ++i) {
      job.schedule (new MyTask (100));
    }

    job.start ();
    job.wait ();
    EXPECT_EQ (job.is_running (), false);

    EXPECT_EQ (s_sum[0].sum () + s_sum[1].sum() + s_sum[2].sum() + s_sum[3].sum () == 10000, true);

  }
}

TEST(28) 
{
  tl::SelfTimer timer ("4 threads, 500 self-scheduled iterations with waiting");
  MyJob job (4);

  for (int l = 0; l < 500; ++l) {

    s_sum[0].reset ();
    s_sum[1].reset ();
    s_sum[2].reset ();
    s_sum[3].reset ();

    job.schedule (new SchedulerTask (&job, 100, 100));

    job.start ();
    job.wait ();
    EXPECT_EQ (job.is_running (), false);

    EXPECT_EQ (s_sum[0].sum () + s_sum[1].sum() + s_sum[2].sum() + s_sum[3].sum () == 10000, true);

  }
}

TEST(29) 
{
  tl::SelfTimer timer ("0 threads, 500 self-scheduled iterations with waiting");
  MyJob job (0);

  for (int l = 0; l < 500; ++l) {

    s_sum[0].reset ();
    s_sum[1].reset ();
    s_sum[2].reset ();
    s_sum[3].reset ();

    job.schedule (new SchedulerTask (&job, 100, 100));

    job.start ();
    job.wait ();
    EXPECT_EQ (job.is_running (), false);

    EXPECT_EQ (s_sum[0].sum () + s_sum[1].sum() + s_sum[2].sum() + s_sum[3].sum () == 10000, true);

  }
}

