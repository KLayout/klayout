
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


#ifndef HDR_tlArch
#define HDR_tlArch

#include "tlCommon.h"

#include <string>

namespace tl
{

/**
 *  @brief Returns the architecture string
 *
 *  The architecture string is made from the cpu, os and compiler.
 *  For example: i686-win32-mingw or x86_64-linux-gcc.
 */
TL_PUBLIC std::string arch_string ();

}

#endif

