
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


#include "tlLog.h"
#include "tlString.h"
#include "tlEnv.h"

#include <stdio.h>
#if !defined(_MSC_VER)
#  include <unistd.h>
#endif

namespace tl
{

// ------------------------------------------------
//  Verbosity implementation

static int default_verbosity ()
{
  int verbosity = 0;
  std::string verbosity_str = tl::get_env ("KLAYOUT_VERBOSITY");
  if (! verbosity_str.empty ()) {
    try {
      tl::from_string (verbosity_str, verbosity);
    } catch (...) {
    }
  }
  return verbosity;
}

static int m_verbosity_level = default_verbosity ();

void
verbosity (int level)
{
  m_verbosity_level = level;
}

int
verbosity ()
{
  return m_verbosity_level;
}

// ------------------------------------------------
//  Channel implementation

Channel::Channel ()
  : m_no_endl (false), m_active (false), m_in_yield (false)
{
  //  .. nothing else ..
}

Channel::~Channel ()
{
  // .. nothing yet ..
}

ChannelProxy
Channel::issue_proxy ()
{
  m_no_endl = false;
  m_lock.lock ();
  if (! m_active) {
    begin ();
    m_active = true;
  }
  return ChannelProxy (this);
}

void
Channel::release_proxy ()
{
  if (! m_no_endl) {
    endl (); //  this helps remembering that there is something implicit going on here ..
  }
  end ();
  m_active = false;
  m_no_endl = false;
  bool in_yield = m_in_yield;
  m_in_yield = true;
  m_lock.unlock ();

  //  after we have released the lock we give the receivers an opportunity to process events. Note that we allow only one thread to yield
  //  at the same time and no recursive yields.
  if (! in_yield) {
    yield ();
    //  this should be atomic, but execution reordering may place it before yield(). Hence the lock.
    m_lock.lock ();
    m_in_yield = false;
    m_lock.unlock ();
  }
}

ChannelEndl endl;
ChannelNoendl noendl;

// ------------------------------------------------
//  LogTee implementation

LogTee::LogTee ()
{
  //  .. nothing yet ..
}

LogTee::LogTee (Channel *first, bool owned)
{
  add (first, owned);
}

void
LogTee::add (Channel *other, bool owned)
{
  m_lock.lock ();
  m_channels.push_back (other);
  if (owned) {
    m_owned_channels.push_back (other);
  }
  m_lock.unlock ();
}

void
LogTee::prepend (Channel *other, bool owned)
{
  m_lock.lock ();
  m_channels.insert (m_channels.begin (), other);
  if (owned) {
    m_owned_channels.push_back (other);
  }
  m_lock.unlock ();
}

void
LogTee::clear ()
{
  m_lock.lock ();
  m_channels.clear ();
  m_owned_channels.clear ();
  m_lock.unlock ();
}

void
LogTee::puts (const char *s)
{
  try {
    for (tl::weak_collection<tl::Channel>::iterator c = m_channels.begin (); c != m_channels.end (); ++c) {
      c->puts (s);
    }
  } catch (...) {
    //  ignore exceptions here
  }
}

void
LogTee::yield ()
{
  try {
    for (tl::weak_collection<tl::Channel>::iterator c = m_channels.begin (); c != m_channels.end (); ++c) {
      c->yield ();
    }
  } catch (...) {
    //  ignore exceptions here
  }
}

void
LogTee::endl ()
{
  try {
    for (tl::weak_collection<tl::Channel>::iterator c = m_channels.begin (); c != m_channels.end (); ++c) {
      c->endl ();
    }
  } catch (...) {
    //  ignore exceptions here
  }
}

void
LogTee::end ()
{
  try {
    for (tl::weak_collection<tl::Channel>::iterator c = m_channels.begin (); c != m_channels.end (); ++c) {
      c->end ();
    }
  } catch (...) {
    //  ignore exceptions here
  }
}

void
LogTee::begin ()
{
  try {
    for (tl::weak_collection<tl::Channel>::iterator c = m_channels.begin (); c != m_channels.end (); ++c) {
      c->begin ();
    }
  } catch (...) {
    //  ignore exceptions here
  }
}

// ------------------------------------------------
//  ChannelProxy implementation

ChannelProxy::ChannelProxy (Channel *channel)
  : mp_channel (channel)
{
}

ChannelProxy::~ChannelProxy ()
{
  if (mp_channel) {
    mp_channel->release_proxy ();
    mp_channel = 0;
  }
}

ChannelProxy &
ChannelProxy::operator= (const ChannelProxy &d)
{
  if (mp_channel != d.mp_channel) {
    mp_channel = d.mp_channel;
    // transfer channel to the assigned object
    (const_cast<ChannelProxy &> (d)).mp_channel = 0;
  }
  return *this;
}

ChannelProxy::ChannelProxy (const ChannelProxy &d)
  : mp_channel (d.mp_channel)
{
  // transfer channel to the assigned object
  (const_cast<ChannelProxy &> (d)).mp_channel = 0;
}

// ------------------------------------------------
//  Some utilities for colorized terminal output

#if defined(_MSC_VER)
static bool
can_colorize (FILE * /*stream*/)
{
  return false;
}
#else
static bool
can_colorize (FILE *stream)
{
  return isatty (fileno (stream));
}
#endif

#define ANSI_RED "\033[31;1m"
#define ANSI_BLUE "\033[34m"
#define ANSI_GREEN "\033[32m"
#define ANSI_RESET "\033[0m"

// ------------------------------------------------
//  InfoChannel implementation

/**
 *  @brief A special implementation for information messages to stdout
 */

class InfoChannel : public Channel
{
public:
  InfoChannel (int verbosity);
  ~InfoChannel ();

protected:
  virtual void puts (const char *s);
  virtual void endl ();
  virtual void end ();
  virtual void begin ();
  virtual void yield () { }

private:
  int m_verbosity;
  bool m_colorized;
};

InfoChannel::InfoChannel (int verbosity)
  : m_verbosity (verbosity)
{
  m_colorized = can_colorize (stdout);
}

InfoChannel::~InfoChannel ()
{
  //  .. nothing yet ..
}

void
InfoChannel::puts (const char *s)
{
  if (verbosity () >= m_verbosity) {
    fprintf (stdout, "%s", s);
  }
}

void
InfoChannel::endl ()
{
  if (verbosity () >= m_verbosity) {
    fprintf (stdout, "\n");
  }
}

void
InfoChannel::end ()
{
  if (verbosity () >= m_verbosity) {
    if (m_verbosity == 0 && m_colorized) {
      fputs (ANSI_RESET, stdout);
    }
    fflush (stdout);
  }
}

void
InfoChannel::begin ()
{
  if (verbosity () >= m_verbosity) {
    if (m_verbosity == 0 && m_colorized) {
      fputs (ANSI_GREEN, stdout);
    }
  }
}

// ------------------------------------------------
//  WarningChannel implementation

/**
 *  @brief A special implementation for warnings to stdout
 */

class WarningChannel : public Channel
{
public:
  WarningChannel ();
  ~WarningChannel ();

protected:
  virtual void puts (const char *s);
  virtual void endl ();
  virtual void end ();
  virtual void begin ();
  virtual void yield () { }

private:
  bool m_colorized;
  bool m_new_line;
};

WarningChannel::WarningChannel ()
{
  m_colorized = can_colorize (stdout);
  m_new_line = true;
}

WarningChannel::~WarningChannel ()
{
  //  .. nothing yet ..
}

void
WarningChannel::puts (const char *s)
{
  fputs (s, stdout);
}

void
WarningChannel::endl ()
{
  fputs ("\n", stdout);
  m_new_line = true;
}

void
WarningChannel::end ()
{
  if (m_colorized) {
    fputs (ANSI_RESET, stdout);
  }
  fflush (stdout);
}

void
WarningChannel::begin ()
{
  if (m_colorized) {
    fputs (ANSI_BLUE, stdout);
  }
  if (m_new_line) {
    fputs ("Warning: ", stdout);
    m_new_line = false;
  }
}


// ------------------------------------------------
//  ErrorChannel implementation

/**
 *  @brief A special implementation for errors to stderr
 */

class ErrorChannel : public Channel
{
public:
  ErrorChannel ();
  ~ErrorChannel ();

protected:
  virtual void puts (const char *s);
  virtual void endl ();
  virtual void end ();
  virtual void begin ();
  virtual void yield () { }

private:
  bool m_colorized;
  bool m_new_line;
};

ErrorChannel::ErrorChannel ()
{
  m_colorized = can_colorize (stderr);
  m_new_line = true;
}

ErrorChannel::~ErrorChannel ()
{
  //  .. nothing yet ..
}

void
ErrorChannel::puts (const char *s)
{
  fputs (s, stderr);
}

void
ErrorChannel::endl ()
{
  fputs ("\n", stderr);
  m_new_line = true;
}

void
ErrorChannel::end ()
{
  if (m_colorized) {
    fputs (ANSI_RESET, stderr);
  }
  fflush (stderr);
}

void
ErrorChannel::begin ()
{
  if (m_colorized) {
    fputs (ANSI_RED, stderr);
  }
  if (m_new_line) {
    fputs ("ERROR: ", stderr);
    m_new_line = false;
  }
}

// ------------------------------------------------
//  The instances of the log channels

TL_PUBLIC LogTee warn (new WarningChannel (), true);
TL_PUBLIC LogTee info (new InfoChannel (0), true);
TL_PUBLIC LogTee log (new InfoChannel (10), true);
TL_PUBLIC LogTee error (new ErrorChannel (), true);

} // namespace tl

