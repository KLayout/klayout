
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

#if defined(HAVE_64BIT_COORD)

#include "tlInt128Support.h"

namespace tl
{

std::string
int128_to_string (__int128 x)
{
  std::string r;

  //  this is the max. power of 10 that can be represented with __int128
  __int128 m = (unsigned long long) 0x4b3b4ca85a86c47a;
  m <<= 64;
  m |= (unsigned long long) 0x98a224000000000;

  if (x < 0) {
    r = "-";
    x = -x;
  } else if (x == 0) {
    return "0";
  }

  bool first = true;
  while (m > 1) {
    int d = 0;
    while (x >= m) {
      d += 1;
      x -= m;
    }
    if (d > 0 || !first) {
      r += char ('0' + d);
      first = false;
    }
    m /= 10;
  }

  r += char('0' + int(x));
  return r;
}

}

#endif
