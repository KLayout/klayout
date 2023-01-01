
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

#include "tlArch.h"

namespace tl
{

std::string
arch_string ()
{

#if defined(_WIN32) || defined(_WIN64)
# if defined(_WIN64)
#   if defined(_MSC_VER)
  return "x86_64-win32-msvc";
#   elif defined(__MINGW32__)
  return "x86_64-win32-mingw";
#   endif
# else
#   if defined(_MSC_VER)
  return "i686-win32-msvc";
#   elif defined(__MINGW32__)
  return "i686-win32-mingw";
#   endif
# endif
#elif defined(__clang__)
# if defined(__x86_64__)
  return "x86_64-linux-clang";
# else
  return "i686-linux-clang";
# endif
#elif defined(__GNUC__)
# if defined(__x86_64__)
  return "x86_64-linux-gcc";
# else
  return "i686-linux-gcc";
# endif
#else
  return "";
#endif

}

}

