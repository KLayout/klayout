
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


#if !defined(HDR_antCommon_h)
# define HDR_antCommon_h

# if defined _WIN32 || defined __CYGWIN__

#   ifdef MAKE_ANT_LIBRARY
#     define ANT_PUBLIC __declspec(dllexport)
#   else
#     define ANT_PUBLIC __declspec(dllimport)
#   endif
#   define ANT_LOCAL
#   define ANT_PUBLIC_TEMPLATE

# else

#   if __GNUC__ >= 4 || defined(__clang__)
#     define ANT_PUBLIC __attribute__ ((visibility ("default")))
#     define ANT_PUBLIC_TEMPLATE __attribute__ ((visibility ("default")))
#     define ANT_LOCAL  __attribute__ ((visibility ("hidden")))
#   else
#     define ANT_PUBLIC
#     define ANT_PUBLIC_TEMPLATE
#     define ANT_LOCAL
#   endif

# endif

#endif
