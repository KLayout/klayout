
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#include "utTestConsole.h"
#include "tlUnitTest.h"

#if !defined(_MSC_VER)
#  include <unistd.h>
#endif

#if !defined(_WIN32)
#  include <sys/ioctl.h>
#  include <dlfcn.h>
#endif
#if defined(_WIN32)
#  include <Windows.h>
#endif

namespace ut
{

// ------------------------------------------------
//  tl::Channel implementations for redirecting the log output

class InfoChannel : public tl::Channel
{
public:
  InfoChannel (int verbosity)
    : m_verbosity (verbosity)
  {
    //  .. nothing yet ..
  }

protected:
  virtual void puts (const char *s)
  {
    if (tl::verbosity () >= m_verbosity) {
      TestConsole::instance ()->basic_write (s);
    }
  }

  virtual void endl ()
  {
    if (tl::verbosity () >= m_verbosity) {
      TestConsole::instance ()->basic_write ("\n");
    }
  }

  virtual void end ()
  {
    TestConsole::instance ()->flush ();
  }

  virtual void begin ()
  {
    //  .. nothing yet ..
  }

  virtual void yield () { }

private:
  int m_verbosity;
};

class WarningChannel : public tl::Channel
{
public:
  WarningChannel ()
  {
    //  .. nothing yet ..
  }

protected:
  virtual void puts (const char *s)
  {
    TestConsole::instance ()->basic_write (s);
  }

  virtual void endl ()
  {
    TestConsole::instance ()->basic_write ("\n");
  }

  virtual void end ()
  {
    TestConsole::instance ()->end ();
    TestConsole::instance ()->flush ();
  }

  virtual void begin ()
  {
    TestConsole::instance ()->begin_warn ();
  }

  virtual void yield () { }
};

class ErrorChannel : public tl::Channel
{
public:
  ErrorChannel ()
  {
    //  .. nothing yet ..
  }

protected:
  virtual void puts (const char *s)
  {
    TestConsole::instance ()->basic_write (s);
  }

  virtual void endl ()
  {
    TestConsole::instance ()->basic_write ("\n");
  }

  virtual void end ()
  {
    TestConsole::instance ()->end ();
    TestConsole::instance ()->flush ();
  }

  virtual void begin ()
  {
    TestConsole::instance ()->begin_error ();
  }

  virtual void yield () { }
};

class CtrlChannel : public tl::Channel
{
public:
  CtrlChannel (bool with_xml)
    : m_with_xml (with_xml)
  {
    //  .. nothing yet ..
  }

protected:
  virtual void puts (const char *s)
  {
    if (m_with_xml == tl::xml_format ()) {
      TestConsole::instance ()->raw_write (s);
    }
  }

  virtual void endl ()
  {
    if (m_with_xml == tl::xml_format ()) {
      TestConsole::instance ()->raw_write ("\n");
    }
  }

  virtual void end ()
  {
    if (m_with_xml == tl::xml_format ()) {
      TestConsole::instance ()->end ();
      TestConsole::instance ()->flush ();
    }
  }

  virtual void begin ()
  {
    if (m_with_xml == tl::xml_format ()) {
      TestConsole::instance ()->begin_info ();
    }
  }

  virtual void yield () { }

private:
  bool m_with_xml;
};

// ------------------------------------------------
//  TestConsole implementation

const char *ANSI_RED = "\033[31;1m";
const char *ANSI_BLUE = "\033[34m";
const char *ANSI_GREEN = "\033[32m";
const char *ANSI_RESET = "\033[0m";

TestConsole::TestConsole (FILE *file)
  : m_file (file), m_col (0), m_max_col (400), m_columns (50), m_rows (0), m_file_is_tty (false)
{
  ms_instance = this;

  prepare_file ();
  redirect ();
}

TestConsole::~TestConsole ()
{
  restore ();

  if (ms_instance == this) {
    ms_instance = 0;
  }
}

void TestConsole::prepare_file ()
{
#if defined(_MSC_VER)
  m_file_is_tty = false;
#else
  m_file_is_tty = isatty (fileno (m_file));
#endif

#if !defined(_WIN32)
  if (m_file_is_tty) {
    struct winsize ws;
    ioctl (fileno (stdout), TIOCGWINSZ, &ws);
    m_columns = std::max (0, (int) ws.ws_col);
    m_rows = std::max (0, (int) ws.ws_row);
  }
#endif
}

void
TestConsole::send_to (FILE *file)
{
  if (file != m_file) {
    flush ();
    m_file = file;
    prepare_file ();
  }
}

int
TestConsole::columns ()
{
  int c = m_columns - tl::indent ();
  return c > 0 ? c : 0;
}

void
TestConsole::write_str (const char *text, output_stream os)
{
  if (os == OS_stderr) {
    begin_error ();
    basic_write (text);
    end ();
  } else {
    basic_write (text);
  }
}

void
TestConsole::raw_write (const char *text)
{
  fputs (text, m_file);
}

void
TestConsole::flush ()
{
  fflush (m_file);
}

bool
TestConsole::is_tty ()
{
  //  NOTE: this assumes we are delivering to stdout
  return m_file_is_tty && ! tl::xml_format ();
}

void
TestConsole::begin_error ()
{
  if (is_tty ()) {
    fputs (ANSI_RED, m_file);
  }
}

void
TestConsole::begin_info ()
{
  if (is_tty ()) {
    fputs (ANSI_GREEN, m_file);
  }
}

void
TestConsole::begin_warn ()
{
  if (is_tty ()) {
    fputs (ANSI_BLUE, m_file);
  }
}

void
TestConsole::end ()
{
  if (is_tty ()) {
    fputs (ANSI_RESET, m_file);
  }
}

void
TestConsole::basic_write (const char *s)
{
  if (tl::xml_format ()) {

    for (const char *cp = s; *cp; ++cp) {
      if (*cp == '&') {
        fputs ("&amp;", m_file);
      } else if (*cp == '<') {
        fputs ("&lt;", m_file);
      } else if (*cp == '>') {
        fputs ("&gt;", m_file);
      } else {
        fputc (*cp, m_file);
      }
    }

  } else {

    //  line length limitation - this assumes we are always printing to the same terminal
    //  or we don't mix stderr/stdout.
    const char *cp;
    for (cp = s; *cp; ++cp) {
      if (*cp == '\n' || *cp == '\r') {
        m_col = 0;
        fputc (*cp, m_file);
      } else {
        if (m_col == 0) {
          for (int i = 0; i < tl::indent (); ++i) {
            fputc (' ', m_file);
          }
          m_col = tl::indent ();
        }
        if (m_col > m_max_col) {
          //  ignore char
        } else if (m_col == m_max_col) {
          fputs (" ...", m_file);
          ++m_col;
        } else if (*cp == '\033') {
          //  skip ANSI escape sequences (no increment of s_col)
          const char *cpend = cp + 1;
          if (*cpend == '[') {
            ++cpend;
            while (*cpend && *cpend != 'm') {
              ++cpend;
            }
            if (*cpend) {
              ++cpend;
            }
          }
          while (cp != cpend) {
            fputc (*cp++, m_file);
          }
          --cp;
        } else {
          fputc (*cp, m_file);
          ++m_col;
        }
      }
    }

  }
}

void
TestConsole::redirect ()
{
  //  redirect the log channels
  tl::warn.clear ();
  tl::warn.add (new ut::WarningChannel (), true);
  tl::info.clear ();
  tl::info.add (new ut::InfoChannel (0), true);
  tl::log.clear ();
  tl::log.add (new ut::InfoChannel (10), true);
  tl::error.clear ();
  tl::error.add (new ut::ErrorChannel (), true);
}

void
TestConsole::restore ()
{
  //  TODO: we should basically restore the original channels
  tl::warn.clear ();
  tl::info.clear ();
  tl::log.clear ();
  tl::error.clear ();
}

TestConsole *TestConsole::ms_instance = 0;

tl::LogTee noctrl (new CtrlChannel (false), true);
tl::LogTee ctrl (new CtrlChannel (true), true);


}
