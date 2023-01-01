
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


#ifndef _HDR_pyaInspector
#define _HDR_pyaInspector

#include "pyaRefs.h"

#include "gsiInspector.h"

#include <list>
#include <string>

namespace pya
{

/**
 *  @brief Creates an inspector object from a Python dict, tuple or list
 *
 *  The returned object must be deleted by the caller.
 *  If symbolic is true, the dict is interpreted as a variable list.
 */
gsi::Inspector *
create_inspector (PyObject *container, bool symbolic = false);

}

#endif

