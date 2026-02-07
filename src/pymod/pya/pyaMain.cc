
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#include "../pymodHelper.h"

//  to force linking of the layview module
#include "../db/dbMain.h"
#include "../rdb/rdbMain.h"
#include "../lib/libMain.h"
#include "../lay/layMain.h"

#if defined(HAVE_QT)
#  if defined(HAVE_QTBINDINGS)
#    include "../QtCore/QtCoreMain.h"
#    include "../QtGui/QtGuiMain.h"
#    if defined(INCLUDE_QTNETWORK)
#      include "../QtNetwork/QtNetworkMain.h"
#    endif
#    if defined(INCLUDE_QTWIDGETS)
#      include "../QtWidgets/QtWidgetsMain.h"
#    endif
#    if defined(INCLUDE_QTPRINTSUPPORT)
#      include "../QtPrintSupport/QtPrintSupportMain.h"
#    endif
#    if defined(INCLUDE_QTSVG)
#      include "../QtSvg/QtSvgMain.h"
#    endif
#    if defined(INCLUDE_QTXML)
#      include "../QtXml/QtXmlMain.h"
#    endif
#    if defined(INCLUDE_QTXMLPATTERNS)
#      include "../QtXmlPatterns/QtXmlPatternsMain.h"
#    endif
#    if defined(INCLUDE_QTSQL)
#      include "../QtSql/QtSqlMain.h"
#    endif
#    if defined(INCLUDE_QTDESIGNER)
#      include "../QtDesigner/QtDesignerMain.h"
#    endif
#    if defined(INCLUDE_QTUITOOLS)
#      include "../QtUiTools/QtUiToolsMain.h"
#    endif
#    if defined(INCLUDE_QTCORE5COMPAT)
#      include "../QtCore5Compat/QtCore5CompatMain.h"
#    endif
#  endif
#endif
  
#include "../../db/db/dbInit.h"

static PyObject *pya_module_init (const char *pymod_name, const char *mod_name, const char *mod_description)
{
  db::init ();
  return module_init (pymod_name, mod_name, mod_description);
}

DEFINE_PYMOD_WITH_INIT(pyacore, 0, "KLayout generic Python module (pya)", pya_module_init)
