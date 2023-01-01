
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


#ifndef HDR_tlBase64
#define HDR_tlBase64

#include "tlCommon.h"

#include <string>
#include <vector>

namespace tl
{

/**
 *  Converts a base64-encoded string into binary data
 */
TL_PUBLIC std::vector<unsigned char> from_base64 (const char *s);

/**
 *  Converts binary data into a base64-encoded string
 */
TL_PUBLIC std::string to_base64 (const unsigned char *data, size_t size);

}

#endif
