
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

#include "tlSleep.h"

#if defined(_MSC_VER) || defined(_WIN32)
#  include <Windows.h>
#  include <Synchapi.h>
#else
#  include <unistd.h>
#  include <signal.h>
#  include <sys/time.h>
#  include <sys/select.h>
#endif

#include <stdio.h>

namespace tl
{

#if !defined(_WIN32)

static void init_sigmask_for_sleep (sigset_t *mask)
{
  sigemptyset (mask);
  //  disable a couple of signals we don't want to interfere with our sleep
  sigaddset (mask, SIGCHLD);
  sigaddset (mask, SIGALRM);
  sigaddset (mask, SIGVTALRM);
  sigaddset (mask, SIGPROF);
  sigaddset (mask, SIGWINCH);
}

#endif

void usleep (unsigned long us)
{
#if defined(_WIN32)

    Sleep ((DWORD) ((us + 999) / 1000));

#else

    // Portable sleep for platforms other than Windows.

    struct timespec wait;
    wait.tv_sec = (us / 1000000ul);
    wait.tv_nsec = (us % 1000000ul) * 1000ul;

    sigset_t mask;
    init_sigmask_for_sleep (&mask);

    pselect (0, NULL, NULL, NULL, &wait, &mask);

#endif
}

void msleep (unsigned long ms)
{
#if defined(_WIN32)

    Sleep ((DWORD) ms);

#else

    // Portable sleep for platforms other than Windows.

    struct timespec wait;
    wait.tv_sec = (ms / 1000ul);
    wait.tv_nsec = (ms % 1000ul) * 1000000ul;

    sigset_t mask;
    init_sigmask_for_sleep (&mask);

    pselect (0, NULL, NULL, NULL, &wait, &mask);

#endif
}

}

