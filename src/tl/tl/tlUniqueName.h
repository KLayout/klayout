
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


#ifndef HDR_tlUniqueName
#define HDR_tlUniqueName

#include "tlCommon.h"
#include "tlString.h"

namespace tl
{

/**
 *  @brief An object delivering a unique name given a set of existing names
 *
 *  If no object with the given name exists, the original name is returned.
 *  Otherwise, the unique name is built from the original name plus
 *  the separator and an integer disambiguator.
 */
template <class Set>
TL_PUBLIC_TEMPLATE std::string unique_name (const std::string &org_name, const Set &present_names, const std::string &sep = "$")
{
  if (present_names.find (org_name) == present_names.end ()) {
    return org_name;
  }

  std::string b;

  unsigned int j = 0;
  for (unsigned int m = (unsigned int) 1 << (sizeof (unsigned int) * 8 - 2); m > 0; m >>= 1) {
    j += m;
    b = org_name + sep + tl::to_string (j);
    if (present_names.find (b) == present_names.end ()) {
      j -= m;
    }
  }

  return org_name + sep + tl::to_string (j + 1);
}

} // namespace tl

#endif

