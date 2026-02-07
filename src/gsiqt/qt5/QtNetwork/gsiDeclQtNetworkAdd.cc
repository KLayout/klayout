
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


#include "gsiQt.h"

#include <QPair>
#include <QHostAddress>
#include <QUrl>

namespace gsi_qt
{

// ------------------------------------------------------------
//  Declarations for QPair<QHostAddress, int>

gsi::Class<QPair<QHostAddress, int> > decl_QHostAddress_int_QPair ("QtNetwork", "QPair_QHostAddress_int",
  qt_gsi::pair_decl<QHostAddress, int>::methods (),
  "@qt\\n@brief Represents a QPair<QHostAddress, int>"
);

}
