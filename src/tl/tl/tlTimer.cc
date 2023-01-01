
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


#include "tlTimer.h"
#include "tlLog.h"
#include "tlString.h"

#if defined(_MSC_VER) || defined(_WIN32)
#  include <Windows.h>
#  include <Psapi.h>
#elif defined(__MACH__) && defined(__APPLE__)
#  include <mach/clock.h>
#  include <mach/mach.h>
#  include <sys/times.h>
#  include <unistd.h>
#else
#  include <sys/times.h>
#  include <unistd.h>
#endif

#include <stdio.h>

namespace tl
{

// -------------------------------------------------------------

#if defined(_MSC_VER)
//  FILETIME and Posix use different zero time. This is the difference
//  between Jan 1, 1601 and Jan 1, 1970. This value is taken from pthread-win32.
const uint64_t ft_to_epoch_offset = uint64_t (11644473600) * uint64_t (10000000);
#endif

void current_utc_time (struct timespec *ts)
{

#if defined(__MACH__) && defined(__APPLE__)

  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);
  ts->tv_sec = mts.tv_sec;
  ts->tv_nsec = mts.tv_nsec;

#elif defined(_MSC_VER)

  FILETIME ft;
  GetSystemTimeAsFileTime (&ft);

  uint64_t t = (uint64_t (ft.dwHighDateTime) << (sizeof (ft.dwHighDateTime) * 8)) | uint64_t (ft.dwLowDateTime);
  t -= ft_to_epoch_offset;

  //  t is in 100ns units
  ts->tv_nsec = (t % 10000000) * 100;
  ts->tv_sec = (t / 10000000);

#else
  clock_gettime(CLOCK_REALTIME, ts);
#endif
}

// -------------------------------------------------------------
//  Gets the current time in ms from epoch

static int64_t ms_time ()
{
#if defined(__MACH__) && defined(__APPLE__)

  clock_serv_t cclock;
  mach_timespec_t mts;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &cclock);
  clock_get_time(cclock, &mts);
  mach_port_deallocate(mach_task_self(), cclock);

  return int64_t (mts.tv_sec) * 1000 + int64_t (0.5 + mts.tv_nsec / 1.0e6);

#elif defined(_MSC_VER)

  FILETIME ft;
  GetSystemTimeAsFileTime (&ft);

  uint64_t t = (uint64_t (ft.dwHighDateTime) << (sizeof (ft.dwHighDateTime) * 8)) | uint64_t (ft.dwLowDateTime);
  t -= ft_to_epoch_offset;

  //  FILETIME uses 100ns resolution, hence divide by 10000 to get ms:
  return int64_t (t / 10000);

#else

  timespec ts;
  clock_gettime (CLOCK_REALTIME, &ts);
  return int64_t (ts.tv_sec) * 1000 + int64_t (0.5 + ts.tv_nsec / 1.0e6);

#endif
}

// -------------------------------------------------------------
//  Implementation of Timer

Timer::Timer ()
    : m_user_ms (0), m_sys_ms (0), m_wall_ms (0),
      m_user_ms_res (0), m_sys_ms_res (0), m_wall_ms_res (0)
{
  // ..
}

void
Timer::start ()
{
#ifdef _WIN32
  timer_t clks_ms = (timer_t) ((double) clock () * (1000.0 / CLOCKS_PER_SEC) + 0.5);
  m_user_ms += clks_ms;
  //  no system time available ..
#else
  struct tms clks;
  times (&clks);
  const double clk2msec = 1000.0 / sysconf (_SC_CLK_TCK);

  m_user_ms += (timer_t) ((clks.tms_utime + clks.tms_cutime) * clk2msec + 0.5);
  m_sys_ms += (timer_t) ((clks.tms_stime + clks.tms_cstime) * clk2msec + 0.5);
#endif

  m_wall_ms += ms_time ();
}

void
Timer::stop ()
{
  m_user_ms = -m_user_ms;
  m_sys_ms = -m_sys_ms;
  m_wall_ms = -m_wall_ms;
  start ();

  m_user_ms_res = m_user_ms;
  m_sys_ms_res = m_sys_ms;
  m_wall_ms_res = m_wall_ms;
  m_user_ms = 0;
  m_sys_ms = 0;
  m_wall_ms = 0;
}

void
Timer::take ()
{
  timer_t user_ms = m_user_ms;
  timer_t sys_ms = m_sys_ms;
  timer_t wall_ms = m_wall_ms;

  m_user_ms = -m_user_ms;
  m_sys_ms = -m_sys_ms;
  m_wall_ms = -m_wall_ms;
  start ();

  m_user_ms_res = m_user_ms;
  m_sys_ms_res = m_sys_ms;
  m_wall_ms_res = m_wall_ms;

  m_user_ms = user_ms;
  m_sys_ms = sys_ms;
  m_wall_ms = wall_ms;
}

size_t
Timer::memory_size ()
{
#ifdef _WIN32

  size_t mem = 0;

  HANDLE h_process = OpenProcess (PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId ());
  if (h_process != NULL) {

    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo (h_process, &pmc, sizeof (pmc))) {
      mem = size_t (pmc.WorkingSetSize);
    }

    CloseHandle (h_process);

  }

  return mem;

#else

  unsigned long memsize = 0;
  FILE *procfile = fopen ("/proc/self/stat", "r");
  if (procfile != NULL) {
    int n = fscanf (procfile, "%*d " // pid
                              "%*s " // comm
                              "%*c " // state
                              "%*d " // ppid
                              "%*d " // pgrp
                              "%*d " // session
                              "%*d " // tty_nr
                              "%*d " // tpgid
                              "%*u " // flags
                              "%*u " // minflt
                              "%*u " // cminflt
                              "%*u " // majflt
                              "%*u " // cmajflt
                              "%*u " // utime
                              "%*u " // stime
                              "%*d " // cutime
                              "%*d " // cstime
                              "%*d " // priority
                              "%*d " // nice
                              "%*d " // 0
                              "%*d " // itrealvalue
                              "%*u " // starttime
                              "%lu " // vsize
                              "%*d " // rss
                              "%*u " // rlim
                              "%*u " // startcode
                              "%*u " // endcode
                              "%*u " // startstack
                              "%*u " // kstkesp
                              "%*u " // kstkeip
                              "%*u " // signal
                              "%*u " // blocked
                              "%*u " // sigignore
                              "%*u " // sigcatch
                              "%*u " // wchan
                              "%*u " // nswap
                              "%*u " // cnswap
                              "%*d " // exit_signal
                              "%*d " // processor
                              "%*u " // rt_priority
                              "%*u" // policy
                              , &memsize);
    fclose (procfile);
    if (n == 0) {
      memsize = 0;
    }
  }

  return size_t (memsize);

#endif
}

void
SelfTimer::start_report () const
{
  tl::info << m_desc << ": " << tl::to_string (tr ("started"));
}

void
SelfTimer::report () const
{
  size_t memsize = memory_size ();

  tl::info << m_desc << ": " << sec_user () << " (user) "
           << sec_sys () << " (sys) "
           << sec_wall () << " (wall)" << tl::noendl;

  if (memsize > 0) {
    tl::info << " " << tl::sprintf ("%.2fM", double (memsize) / (1024.0 * 1024.0)) << " (mem)";
  } else {
    tl::info << "";
  }
}

// -------------------------------------------------------------
//  Implementation of Clock

Clock::Clock (double s)
{
  m_clock_ms = s * 1000.0;
}

double 
Clock::seconds () const
{
  return double (m_clock_ms) * 0.001;
}

Clock
Clock::current ()
{
  Clock c;
  c.m_clock_ms += ms_time ();
  return c;
}

}

