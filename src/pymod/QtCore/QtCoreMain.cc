
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

#include "../pymodHelper.h"

//  To force linking of the QtCore module
#include "../../gsiqt/qtbasic/gsiQtCoreExternals.h"
FORCE_LINK_GSI_QTCORE

//  And this is *only* required because of QSignalMapper which takes a QWidget argument from
//  the QtGui library and we need to supply the GSI binding for this ...
#include "../../gsiqt/qtbasic/gsiQtGuiExternals.h"
FORCE_LINK_GSI_QTGUI

//  And because we pull in QtGui, we also need to pull in QtWidgets because QtGui bindings
//  use QAction and QWidget which are itself in QtWidgets
#include "../../gsiqt/qtbasic/gsiQtWidgetsExternals.h"
FORCE_LINK_GSI_QTWIDGETS

DEFINE_PYMOD(QtCore, "QtCore", "KLayout/Qt module 'QtCore'")

