
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

#include "gsiDecl.h"
#include "dbNetlist.h"

namespace gsi
{

extern Class<db::NetlistObject> decl_dbNetlistObject;

Class<db::Pin> decl_dbPin (decl_dbNetlistObject, "db", "Pin",
  gsi::method ("id", &db::Pin::id,
    "@brief Gets the ID of the pin.\n"
  ) +
  gsi::method ("name", &db::Pin::name,
    "@brief Gets the name of the pin.\n"
  ) +
  gsi::method ("expanded_name", &db::Pin::expanded_name,
    "@brief Gets the expanded name of the pin.\n"
    "The expanded name is the name or a generic identifier made from the ID if the name is empty."
  ),
  "@brief A pin of a circuit.\n"
  "Pin objects are used to describe the outgoing pins of "
  "a circuit. To create a new pin of a circuit, use \\Circuit#create_pin.\n"
  "\n"
  "This class has been added in version 0.26."
);

}

