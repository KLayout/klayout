
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

//  NOTE: klayout_main_tests is actually a Ruby test which does all test automation
//  The tests will also test Python capabilities, so Python is required too.

#if defined(HAVE_RUBY) && defined(HAVE_PYTHON)

#include "rba.h"
#include "gsiDecl.h"

// On Windows, ruby.h is not compatible with windows.h which is included by utHead - at least not if 
// windows.h is included before ruby.h ...
#include "tlUnitTest.h"

void run_rubytest (tl::TestBase * /*_this*/, const std::string &fn)
{
  tl_assert (rba::RubyInterpreter::instance ());

  std::string fp (tl::testsrc ());
  fp += "/testdata/klayout_main/";
  fp += fn;
  rba::RubyInterpreter::instance ()->load_file (fp.c_str ());
}

#define RUBYTEST(n, file) \
  TEST(n) { run_rubytest(_this, file); }

RUBYTEST (main, "main.rb")

#endif

