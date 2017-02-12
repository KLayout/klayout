
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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


#ifdef HAVE_PYTHON

//  For PY_MAJOR_VERSION
#include <Python.h>

#include "pya.h"
#include "gsiTest.h"

#include "utHead.h"

static void run_pythontest (ut::TestBase *_this, const std::string &fn);

TEST (1)
{
  EXPECT_EQ (gsi::has_class ("Value"), true);
  EXPECT_EQ (gsi::has_class ("DoesNotExist"), false);
#if defined(HAVE_QTBINDING)
  EXPECT_EQ (gsi::has_class ("QDialog"), true);
  EXPECT_EQ (gsi::has_class ("QApplication"), true);
#endif

  bool err = false;
  try {
    ut::python_interpreter ()->eval_string ("raise Exception(\"an error\")");
  } catch (tl::ScriptError &ex) {
    EXPECT_EQ (ex.basic_msg (), std::string ("an error"));
#if PY_MAJOR_VERSION < 3
    EXPECT_EQ (ex.cls (), std::string ("exceptions.Exception"));
#else
    EXPECT_EQ (ex.cls (), std::string ("Exception"));
#endif
    err = true;
  }

  EXPECT_EQ (err, true);

  err = false;
  try {
    ut::python_interpreter ()->eval_string ("Quatsch");
  } catch (tl::ScriptError &ex) {
    EXPECT_EQ (ex.basic_msg (), std::string ("name 'Quatsch' is not defined"));
#if PY_MAJOR_VERSION < 3
    EXPECT_EQ (ex.cls (), std::string ("exceptions.NameError"));
#else
    EXPECT_EQ (ex.cls (), std::string ("NameError"));
#endif
    err = true;
  }

  EXPECT_EQ (err, true);

  std::string fn (ut::testsrc ());
  fn += "/testdata/python/basic.py";
  try {
    ut::python_interpreter ()->load_file (fn.c_str ());
    gsi_test::E::reset_inst ();
  } catch (tl::ExitException &ex) {
    gsi_test::E::reset_inst ();
    EXPECT_EQ (ex.status (), 0);
  } catch (...) {
    gsi_test::E::reset_inst ();
    throw;
  }
}

void run_pythontest (ut::TestBase *_this, const std::string &fn)
{
  std::string fp (ut::testsrc ());
  fp += "/testdata/python/";
  fp += fn;
  try {
    ut::python_interpreter ()->load_file (fp.c_str ());
  } catch (tl::ExitException &ex) {
    EXPECT_EQ (ex.status (), 0);
  } catch (...) {
    throw;
  }
}

#define PYTHONTEST(n, file) \
  TEST(2_##n) { run_pythontest(_this, file); }

PYTHONTEST (dbLayoutTest, "dbLayoutTest.py")
PYTHONTEST (dbRegionTest, "dbRegionTest.py")
PYTHONTEST (dbPCellsTest, "dbPCells.py")
PYTHONTEST (tlTest, "tlTest.py")
#if defined(HAVE_QTBINDINGS)
PYTHONTEST (qtbinding, "qtbinding.py")
#endif

#endif

