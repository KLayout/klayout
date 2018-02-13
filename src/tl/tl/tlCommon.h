
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


#if !defined(HDR_tlCommon_h)
# define HDR_tlCommon_h

# if defined _WIN32 || defined __CYGWIN__

#   ifdef MAKE_TL_LIBRARY
#     define TL_PUBLIC __declspec(dllexport)
#   else
#     define TL_PUBLIC __declspec(dllimport)
#   endif
#   define TL_LOCAL
#   define TL_PUBLIC_TEMPLATE

# else

#   if __GNUC__ >= 4 || defined(__clang__)
#     define TL_PUBLIC __attribute__ ((visibility ("default")))
#     define TL_PUBLIC_TEMPLATE __attribute__ ((visibility ("default")))
#     define TL_LOCAL  __attribute__ ((visibility ("hidden")))
#   else
#     define TL_PUBLIC
#     define TL_PUBLIC_TEMPLATE
#     define TL_LOCAL
#   endif

# endif

//  NOTE: this is required because we have some forward declarations to
//  gsi::Class and gsi::ClassBase in tlVariant.h.
//  TODO: there should not be any dependency of tl on gsi.
# if defined _WIN32 || defined __CYGWIN__

#   ifdef MAKE_GSI_LIBRARY
#     define GSI_PUBLIC __declspec(dllexport)
#   else
#     define GSI_PUBLIC __declspec(dllimport)
#   endif
#   define GSI_LOCAL
#   define GSI_PUBLIC_TEMPLATE

# else

#   if __GNUC__ >= 4 || defined(__clang__)
#     define GSI_PUBLIC __attribute__ ((visibility ("default")))
#     define GSI_PUBLIC_TEMPLATE __attribute__ ((visibility ("default")))
#     define GSI_LOCAL  __attribute__ ((visibility ("hidden")))
#   else
#     define GSI_PUBLIC
#     define GSI_PUBLIC_TEMPLATE
#     define GSI_LOCAL
#   endif

# endif

#endif
