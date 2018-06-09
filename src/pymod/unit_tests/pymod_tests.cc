
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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

#define STRINGIFY(s) _STRINGIFY(s)
#define _STRINGIFY(s) #s

#include "tlUnitTest.h"

#include <QProcess>
#include <QProcessEnvironment>


int run_pymodtest (tl::TestBase * /*_this*/, const std::string &fn)
{
  QProcess process;
  process.setProcessChannelMode (QProcess::ForwardedChannels);

  QStringList args;

  std::string fp (tl::testsrc ());
  fp += "/testdata/pymod/";
  fp += fn;
  args << tl::to_qstring (fp);

  QProcessEnvironment env = QProcessEnvironment::systemEnvironment ();
  env.insert("PYTHONPATH", STRINGIFY(PYTHONPATH));
  process.setProcessEnvironment(env);

  process.start (tl::to_qstring (STRINGIFY (PYTHON)), args);
  process.waitForFinished (-1);
  return process.exitCode ();
}

#define PYMODTEST(n, file) \
  TEST(n) { EXPECT_EQ (run_pymodtest(_this, file), 0); }

PYMODTEST (bridge, "bridge.py")

PYMODTEST (import_tl, "import_tl.py")
PYMODTEST (import_db, "import_db.py")
PYMODTEST (import_lay, "import_lay.py")
PYMODTEST (import_QtCore, "import_QtCore.py")
PYMODTEST (import_QtGui, "import_QtGui.py")
PYMODTEST (import_QtXml, "import_QtXml.py")
PYMODTEST (import_QtSql, "import_QtSql.py")
PYMODTEST (import_QtNetwork, "import_QtNetwork.py")
PYMODTEST (import_QtDesigner, "import_QtDesigner.py")

#if QT_VERSION >= 0x50000

PYMODTEST (import_QtWidgets, "import_QtWidgets.py")
PYMODTEST (import_QtMultimedia, "import_QtMultimedia.py")
PYMODTEST (import_QtPrintSupport, "import_QtPrintSupport.py")
PYMODTEST (import_QtSvg, "import_QtSvg.py")
PYMODTEST (import_QtXmlPatterns, "import_QtXmlPatterns.py")

#endif
