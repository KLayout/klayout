
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include "tlLog.h"
#include "gsiInterpreter.h"

#include <cstdio>
#include <sstream>

namespace ut
{

extern tl::LogTee ctrl;
extern tl::LogTee noctrl;

/**
 *  @brief Redirects the interpreter output and serves as a general output device
 */
class TestConsole
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

  void send_to (FILE *file);

  void write_str (const char *text, output_stream os);
  void raw_write (const char *text);
  virtual void flush ();

  virtual bool is_tty ();
  virtual int columns ();

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
  static TestConsole *ms_instance;

  void redirect ();
  void restore ();
  void prepare_file ();
};

}

#endif
