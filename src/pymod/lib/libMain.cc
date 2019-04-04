
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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

//  to force linking of the lib module
#include "../../lib/lib/libForceLink.h"

static PyObject *lib_module_init (const char *pymod_name, const char *mod_name, const char *mod_description)
{
  return module_init (pymod_name, mod_name, mod_description);
}

DEFINE_PYMOD_WITH_INIT(libcore, "lib", "KLayout core module 'lib'", lib_module_init)
