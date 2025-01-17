
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

#if !defined(HAVE_QT_DESIGNER)
#  define FORCE_LINK_GSI_QTDESIGNER
#elif QT_VERSION >= 0x060000
//  Not present in Qt6
#  define FORCE_LINK_GSI_QTDESIGNER
#elif QT_VERSION >= 0x050000
#  include "../qt5/QtDesigner/gsiQtExternals.h"
#else
#  include "../qt4/QtDesigner/gsiQtExternals.h"
#endif
