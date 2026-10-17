
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


#ifndef HDR_tlCpp
#define HDR_tlCpp

//  A subsitute for C++11 [[noreturn]]
#ifdef __GNUC__
#define NO_RETURN __attribute__((noreturn))
#elif __MINGW32__
#define NO_RETURN __attribute__((noreturn))
#elif __clang__
#define NO_RETURN __attribute__((noreturn))
#elif _MSC_VER
#define NO_RETURN __declspec(noreturn)
#endif

//  "TL_IF_UNLIKELY (cond) { ... }" is an if statement whose body is a cold path
#if defined(__has_cpp_attribute) && __has_cpp_attribute(unlikely) && ((defined(__cplusplus) && __cplusplus >= 202002L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L))
#  define TL_IF_UNLIKELY(c) if (c) [[unlikely]]
#elif defined(__GNUC__) || defined(__clang__)
#  define TL_IF_UNLIKELY(c) if (__builtin_expect (!!(c), 0))
#else
#  define TL_IF_UNLIKELY(c) if (c)
#endif

namespace tl
{

//  .. nothing yet ..

}

#endif

