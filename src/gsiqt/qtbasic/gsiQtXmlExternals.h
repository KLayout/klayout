
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

#if !defined(HAVE_QT_XML)
#  define FORCE_LINK_GSI_QTXML
#elif QT_VERSION >= 0x060000
#  include "../qt6/QtXml/gsiQtExternals.h"
#elif QT_VERSION >= 0x050000
#  include "../qt5/QtXml/gsiQtExternals.h"
#else
#  include "../qt4/QtXml/gsiQtExternals.h"
#endif
