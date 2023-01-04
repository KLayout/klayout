
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

#include "tlDefs.h"

#if !defined(HDR_gsiCommon_h)
# define HDR_gsiCommon_h

//  NOTE: this is required because we have some forward declarations to
//  gsi::Class and gsi::ClassBase in tlVariant.h.
//  TODO: there should not be any dependency of tl on gsi.
# ifdef MAKE_GSI_LIBRARY
#   define GSI_PUBLIC           DEF_INSIDE_PUBLIC
#   define GSI_PUBLIC_TEMPLATE  DEF_INSIDE_PUBLIC_TEMPLATE
#   define GSI_LOCAL            DEF_INSIDE_LOCAL
# else
#   define GSI_PUBLIC           DEF_OUTSIDE_PUBLIC
#   define GSI_PUBLIC_TEMPLATE  DEF_OUTSIDE_PUBLIC_TEMPLATE
#   define GSI_LOCAL            DEF_OUTSIDE_LOCAL
# endif

#endif
