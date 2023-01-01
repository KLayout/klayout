
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

#include "gsiInterpreter.h"

namespace gsi
{

GSI_PUBLIC tl::Registrar<Interpreter> interpreters;

Interpreter::Interpreter (int position, const char *name)
  : tl::RegisteredClass<Interpreter> (this, position, name, false)
{
  //  .. nothing yet ..
}

Interpreter::~Interpreter ()
{
  //  .. nothing yet ..
}

}

