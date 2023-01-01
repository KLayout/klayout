
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

#ifndef HDR_tlInt128Support
#define HDR_tlInt128Support

#if defined(HAVE_64BIT_COORD)

#include "tlCommon.h"
#include "tlString.h"
#include <inttypes.h>
#include <ostream>

namespace std
{

#if !defined(__GLIBCXX_BITSIZE_INT_N_0) || __GLIBCXX_BITSIZE_INT_N_0 != 128

  //  Provide an implementation for abs(__int128_t) if there isn't one
  inline __int128 abs (__int128 x)
  {
    return x < 0 ? -x : x;
  }

#endif

  //  Provide ostream serialization for 128bit values
  inline std::ostream &operator<< (std::ostream &os, __int128 x)
  {
    os << tl::to_string (x);
    return os;
  }

}

#endif

#endif

