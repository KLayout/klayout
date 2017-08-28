
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


#ifndef HDR_utTestConsole
#define HDR_utTestConsole

#include "utCommon.h"
#include "tlLog.h"
#include "gsiInterpreter.h"

#include <cstdio>
#include <sstream>

namespace ut
{

/**
 *  @brief A utility class to capture the warning, error and info channels
 *
 *  Instantiate this class inside a test. Then run the test and finally
 *  obtain the collected output with CaptureChannel::captured_text().
 */
class UT_PUBLIC CaptureChannel : public tl::Channel
{
public:
  CaptureChannel ();

  std::string captured_text () const
  {
    return m_text.str ();
  }

  void clear ()
  {
    m_text.str (std::string ());
  }

protected:
  virtual void puts (const char *s);
  virtual void endl ();
  virtual void end ();
  virtual void begin ();

private:
  std::ostringstream m_text;
};

/**
 *  @brief Redirects the interpreter output and serves as a general output device
 */
class UT_PUBLIC TestConsole
  : public gsi::Console
{
public:
  static TestConsole *instance ()
  {
    tl_assert (ms_instance != 0);
    return ms_instance;
  }

  TestConsole (FILE *file);
  ~TestConsole ();

  int indent () const
  {
    return m_indent;
  }

  void write_str (const char *text, output_stream os);
  void raw_write (const char *text);
  virtual void flush ();

  virtual bool is_tty ();

  virtual int columns ()
  {
    return std::max (m_columns - m_indent, 0);
  }

  virtual int rows ()
  {
    return m_rows;
  }

  int real_columns ()
  {
    return m_columns;
  }

  void begin_error ();
  void begin_info ();
  void begin_warn ();
  void end ();
  void basic_write (const char *s);

private:
  FILE *m_file;
  int m_col;
  int m_max_col;
  int m_columns, m_rows;
  bool m_file_is_tty;
  int m_indent;
  static TestConsole *ms_instance;

  void redirect ();
  void restore ();
};

}

#endif
