
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

//  Oh my god ... STRINGIFY(s) will get the argument with MACROS REPLACED.
//  So if the PYTHONPATH is something like build.linux-released, the "linux" macro
//  set to 1 will make this "build.1-release". So STRINGIFY isn't a real solution.
//  On the other hand that is the only documented way to turn a macro into a string.
//  This will prevent that issue:
#undef linux

#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) #s

#include "tlUnitTest.h"
#include "tlStream.h"

int run_pymodtest (tl::TestBase *_this, const std::string &fn)
{
  static std::string pypath;
  if (pypath.empty ()) {
    pypath = "PYTHONPATH=";
    pypath += STRINGIFY (PYTHONPATH);
  }
#if defined(_WIN32)
  _putenv (const_cast<char *> (pypath.c_str ()));
#else
  putenv (const_cast<char *> (pypath.c_str ()));
#endif
  tl::info << pypath;

  std::string fp (tl::testdata ());
  fp += "/pymod/";
  fp += fn;

  std::string text;
  {
    std::string cmd;

#if defined(__APPLE__)
    //  NOTE: because of system integrity, MacOS does not inherit DYLD_LIBRARY_PATH to child
    //  processes like sh. We need to port this variable explicitly.
    const char *ldpath_name = "DYLD_LIBRARY_PATH";
    const char *ldpath = getenv (ldpath_name);
    if (ldpath) {
      cmd += std::string (ldpath_name) + "=\"" + ldpath + "\"; export " + ldpath_name + "; ";
    }
#endif

    cmd += std::string ("\"") + STRINGIFY (PYTHON) + "\" " + fp + " 2>&1";
    
    tl::info << cmd;
    tl::InputPipe pipe (cmd);
    tl::InputStream is (pipe);
    text = is.read_all ();

    //  subprocess exits without error
    EXPECT_EQ (pipe.wait(), 0);
  }

  tl::info << text;
  EXPECT_EQ (text.find ("OK") != std::string::npos, true);

  return 0;
}

#define PYMODTEST(n, file) \
  TEST(n) { EXPECT_EQ (run_pymodtest(_this, file), 0); }

PYMODTEST (bridge, "bridge.py")

PYMODTEST (import_tl, "import_tl.py")
PYMODTEST (import_db, "import_db.py")
PYMODTEST (import_rdb, "import_rdb.py")
PYMODTEST (import_lay, "import_lay.py")

//  others
PYMODTEST (issue1327, "issue1327.py")

#if defined(HAVE_QT) && defined(HAVE_QTBINDINGS)

PYMODTEST (import_QtCore, "import_QtCore.py")
#if QT_VERSION >= 0x60000
PYMODTEST (import_QtGui, "import_QtGui_Qt6.py")
#else
PYMODTEST (import_QtGui, "import_QtGui.py")
#endif
#if defined(HAVE_QT_XML)
PYMODTEST (import_QtXml, "import_QtXml.py")
#endif
#if defined(HAVE_QT_SQL)
PYMODTEST (import_QtSql, "import_QtSql.py")
#endif
#if defined(HAVE_QT_NETWORK)
PYMODTEST (import_QtNetwork, "import_QtNetwork.py")
#endif
#if defined(HAVE_QT_DESIGNER) && QT_VERSION < 0x60000
PYMODTEST (import_QtDesigner, "import_QtDesigner.py")
#endif
#if defined(HAVE_QT_UITOOLS)
PYMODTEST (import_QtUiTools, "import_QtUiTools.py")
#endif

#if QT_VERSION >= 0x50000

#if QT_VERSION >= 0x60000
PYMODTEST (import_QtWidgets, "import_QtWidgets_Qt6.py")
#else
PYMODTEST (import_QtWidgets, "import_QtWidgets.py")
#endif
#if defined(HAVE_QT_MULTIMEDIA)
PYMODTEST (import_QtMultimedia, "import_QtMultimedia.py")
#endif
#if defined(HAVE_QT_PRINTSUPPORT)
PYMODTEST (import_QtPrintSupport, "import_QtPrintSupport.py")
#endif
#if defined(HAVE_QT_SVG)
#if QT_VERSION >= 0x60000
PYMODTEST (import_QtSvg, "import_QtSvg_Qt6.py")
#else
PYMODTEST (import_QtSvg, "import_QtSvg.py")
#endif
#endif
#if defined(HAVE_QT_XML) && QT_VERSION < 0x60000
PYMODTEST (import_QtXmlPatterns, "import_QtXmlPatterns.py")
#endif

#if QT_VERSION >= 0x60000
PYMODTEST (import_QtCore5Compat, "import_QtCore5Compat.py")
#endif

#endif

PYMODTEST (import_pya, "pya_tests.py")

#endif
