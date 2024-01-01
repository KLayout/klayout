
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


#include "pya.h"
#include "tlInternational.h"

namespace pya
{

static void fail (const char *file, int line)
{
  throw tl::ScriptError (tl::to_string (tr ("Python support not compiled in")).c_str (), file, line, "missing_feature", std::vector<tl::BacktraceElement> ());
}

static PythonInterpreter *sp_pya_interpreter = 0;

PythonInterpreter::PythonInterpreter ()
  : gsi::Interpreter (0, "pya")
{
  tl_assert (! sp_pya_interpreter);
  sp_pya_interpreter = this;
}

PythonInterpreter::~PythonInterpreter ()
{
  //  This prevents reinitialization
  sp_pya_interpreter = reinterpret_cast<PythonInterpreter *> (1);
}

PythonInterpreter *PythonInterpreter::instance ()
{
  return sp_pya_interpreter;
}

void 
PythonInterpreter::add_path (const std::string &)
{
  // .. nothing ..
}

void
PythonInterpreter::add_package_location (const std::string &)
{
  // .. nothing ..
}

void
PythonInterpreter::remove_package_location (const std::string &)
{
  // .. nothing ..
}

void
PythonInterpreter::require (const std::string &)
{
  // .. nothing ..
}

void 
PythonInterpreter::set_debugger_scope (const std::string &)
{
  // .. nothing ..
}

void 
PythonInterpreter::remove_debugger_scope ()
{
  // .. nothing ..
}

void 
PythonInterpreter::ignore_next_exception ()
{
  // .. nothing ..
}

void
PythonInterpreter::load_file (const std::string &)
{
  // .. nothing ..
}

void
PythonInterpreter::eval_string (const char *, const char *file, int line, int)
{
  fail (file, line);
}

void
PythonInterpreter::eval_string_and_print (const char *, const char *file, int line, int)
{
  fail (file, line);
}

gsi::Inspector *
PythonInterpreter::inspector (int)
{
  return 0;
}

void
PythonInterpreter::define_variable (const std::string &, const tl::Variant &)
{
  // .. nothing ..
}

bool
PythonInterpreter::available () const
{
  return false;
}

void 
PythonInterpreter::initialize ()
{
  // .. nothing ..
}

void 
PythonInterpreter::push_exec_handler (gsi::ExecutionHandler *)
{
  // .. nothing ..
}

void
PythonInterpreter::remove_exec_handler (gsi::ExecutionHandler *)
{
  // .. nothing ..
}

void 
PythonInterpreter::push_console (gsi::Console *)
{
  // .. nothing ..
}

void 
PythonInterpreter::remove_console (gsi::Console *)
{
  // .. nothing ..
}

std::string
PythonInterpreter::version () const
{
  return std::string ();
}

tl::Variant
PythonInterpreter::eval_expr (const char *, const char *file, int line, int)
{
  fail (file, line);
  return tl::Variant ();
}

}

