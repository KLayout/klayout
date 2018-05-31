
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


#include "gsiQt.h"

#include <QPair>
#include <QAccessibleInterface>

namespace gsi_qt
{

// ------------------------------------------------------------
//  Declarations for QPair<QAccessibleInterface*, QAccessible::Relation>

gsi::Class<QPair<QAccessibleInterface*, QAccessible::Relation> > decl_QAccessibleInterfacePtr_Relation_QPair ("QtGui", "QPair_QAccessibleInterfacePtr_QAccessible_Relation",
  qt_gsi::pair_decl<QAccessibleInterface*, QAccessible::Relation>::methods (),
  "@qt\\n@brief Represents a QPair<QAccessibleInterface*, QAccessible::Relation> >"
);

}
