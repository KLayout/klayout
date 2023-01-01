
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

#include "rba.h"
#include "tlExceptions.h"

namespace rba
{

static void fail (const char *file, int line)
{
  throw tl::ScriptError (tl::to_string (tr ("Ruby support not compiled in")).c_str (), file, line, "missing_feature", std::vector<tl::BacktraceElement> ());
}

static RubyInterpreter *sp_rba_interpreter = 0;

RubyInterpreter::RubyInterpreter ()
  : gsi::Interpreter (0, "rba")
{
  tl_assert (! sp_rba_interpreter);
  sp_rba_interpreter = this;
}

RubyInterpreter::~RubyInterpreter ()
{
  sp_rba_interpreter = 0;
}

RubyInterpreter *RubyInterpreter::instance ()
{
  return sp_rba_interpreter;
}

void
RubyInterpreter::add_package_location (const std::string &)
{
  // .. nothing ..
}

void
RubyInterpreter::remove_package_location (const std::string &)
{
  // .. nothing ..
}

void
RubyInterpreter::add_path (const std::string &)
{
  // .. nothing ..
}

void
RubyInterpreter::require (const std::string &)
{
  // .. nothing ..
}

void 
RubyInterpreter::set_debugger_scope (const std::string &)
{
  // .. nothing ..
}

void 
RubyInterpreter::remove_debugger_scope ()
{
  // .. nothing ..
}

void 
RubyInterpreter::ignore_next_exception ()
{
  // .. nothing ..
}

void
RubyInterpreter::load_file (const std::string &)
{
  // .. nothing ..
}

void
RubyInterpreter::eval_string (const char *, const char *file, int line, int)
{
  fail (file, line);
}

void
RubyInterpreter::eval_string_and_print (const char *, const char *file, int line, int)
{
  fail (file, line);
}

void
RubyInterpreter::define_variable (const std::string &, const tl::Variant &)
{
  // .. nothing ..
}

gsi::Inspector *RubyInterpreter::inspector (int)
{
  return 0;
}

bool
RubyInterpreter::available () const
{
  return false;
}

int
RubyInterpreter::initialize (int &argc, char **argv, int (*main_cont)(int &, char **))
{
  int res = 1;
BEGIN_PROTECTED
  res = (*main_cont) (argc, argv);
END_PROTECTED
  return res;
}

void 
RubyInterpreter::push_exec_handler (gsi::ExecutionHandler *)
{
  // .. nothing ..
}

void
RubyInterpreter::remove_exec_handler (gsi::ExecutionHandler *)
{
  // .. nothing ..
}

void 
RubyInterpreter::push_console (gsi::Console *)
{
  // .. nothing ..
}

void 
RubyInterpreter::remove_console (gsi::Console *)
{
  // .. nothing ..
}

std::string
RubyInterpreter::version () const
{
  return std::string ();
}

tl::Variant
RubyInterpreter::eval_expr (const char *, const char *file, int line, int)
{
  fail (file, line);
  return tl::Variant ();
}

}

