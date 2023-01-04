
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

#if !defined(HDR_tlDefs_h)
# define HDR_tlDefs_h

//  templates provided for building the external symbol
//  declarations per library

# if defined _WIN32 || defined __CYGWIN__

#   define DEF_INSIDE_PUBLIC __declspec(dllexport)
#   define DEF_INSIDE_LOCAL
#   define DEF_INSIDE_PUBLIC_TEMPLATE

#   define DEF_OUTSIDE_PUBLIC __declspec(dllimport)
#   define DEF_OUTSIDE_LOCAL
#   define DEF_OUTSIDE_PUBLIC_TEMPLATE

# else

#   if __GNUC__ >= 4 || defined(__clang__)
#     define DEF_INSIDE_PUBLIC __attribute__ ((visibility ("default")))
#     define DEF_INSIDE_PUBLIC_TEMPLATE __attribute__ ((visibility ("default")))
#     define DEF_INSIDE_LOCAL  __attribute__ ((visibility ("hidden")))
#   else
#     define DEF_INSIDE_PUBLIC
#     define DEF_INSIDE_PUBLIC_TEMPLATE
#     define DEF_INSIDE_LOCAL
#   endif

#   define DEF_OUTSIDE_PUBLIC DEF_INSIDE_PUBLIC
#   define DEF_OUTSIDE_PUBLIC_TEMPLATE DEF_INSIDE_PUBLIC_TEMPLATE
#   define DEF_OUTSIDE_LOCAL DEF_INSIDE_LOCAL

# endif

#endif
