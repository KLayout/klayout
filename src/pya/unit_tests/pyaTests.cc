
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


#ifdef HAVE_PYTHON

#include "pya.h"
#include "gsiDecl.h"

#include "tlUnitTest.h"

static void run_pythontest (tl::TestBase *_this, const std::string &fn);

TEST (basic)
{
  EXPECT_EQ (gsi::has_class ("Value"), true);
  EXPECT_EQ (gsi::has_class ("DoesNotExist"), false);
#if defined(HAVE_QTBINDING)
  EXPECT_EQ (gsi::has_class ("QDialog"), true);
  EXPECT_EQ (gsi::has_class ("QApplication"), true);
#endif

  tl_assert (pya::PythonInterpreter::instance ());

  bool err = false;
  try {
    pya::PythonInterpreter::instance ()->eval_string ("raise Exception(\"an error\")");
  } catch (tl::ScriptError &ex) {
    EXPECT_EQ (ex.basic_msg (), std::string ("Exception: an error"));
    EXPECT_EQ (ex.cls () == std::string ("exceptions.Exception") || ex.cls () == std::string ("Exception"), true);
    err = true;
  }

  EXPECT_EQ (err, true);

  err = false;
  try {
    pya::PythonInterpreter::instance ()->eval_string ("Quatsch");
  } catch (tl::ScriptError &ex) {
    EXPECT_EQ (ex.basic_msg (), std::string ("NameError: name 'Quatsch' is not defined"));
    EXPECT_EQ (ex.cls () == std::string ("exceptions.NameError") || ex.cls () == std::string ("NameError"), true);
    err = true;
  }

  EXPECT_EQ (err, true);

  std::string fn (tl::testsrc ());
  fn += "/testdata/python/basic.py";
  try {
    pya::PythonInterpreter::instance ()->load_file (fn.c_str ());
    pya::PythonInterpreter::instance ()->eval_string ("pya.E.reset_inst()");
  } catch (tl::ExitException &ex) {
    pya::PythonInterpreter::instance ()->eval_string ("pya.E.reset_inst()");
    EXPECT_EQ (ex.status (), 0);
  } catch (...) {
    pya::PythonInterpreter::instance ()->eval_string ("pya.E.reset_inst()");
    throw;
  }
}

void run_pythontest (tl::TestBase *_this, const std::string &fn)
{
  tl_assert (pya::PythonInterpreter::instance ());

  std::string fp (tl::testsrc ());
  fp += "/testdata/python/";
  fp += fn;
  try {
    pya::PythonInterpreter::instance ()->load_file (fp.c_str ());
  } catch (tl::ExitException &ex) {
    EXPECT_EQ (ex.status (), 0);
  } catch (...) {
    throw;
  }
}

#define PYTHONTEST(n, file) \
  TEST(n) { run_pythontest(_this, file); }

PYTHONTEST (dbLayoutTest, "dbLayoutTest.py")
PYTHONTEST (dbRegionTest, "dbRegionTest.py")
PYTHONTEST (dbReaders, "dbReaders.py")
PYTHONTEST (dbPCellsTest, "dbPCells.py")
PYTHONTEST (dbPolygonTest, "dbPolygonTest.py")
PYTHONTEST (dbTransTest, "dbTransTest.py")
PYTHONTEST (dbLayoutToNetlist, "dbLayoutToNetlist.py")
PYTHONTEST (dbLayoutVsSchematic, "dbLayoutVsSchematic.py")
PYTHONTEST (dbNetlistCrossReference, "dbNetlistCrossReference.py")
PYTHONTEST (layLayers, "layLayers.py")
PYTHONTEST (layPixelBuffer, "layPixelBuffer.py")
PYTHONTEST (tlTest, "tlTest.py")
#if defined(HAVE_QT) && defined(HAVE_QTBINDINGS)
PYTHONTEST (qtbinding, "qtbinding.py")
#endif

#endif
