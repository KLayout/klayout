
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


#ifndef _HDR_gsi
#define _HDR_gsi

/**
 *  @brief GSI - the generic scripting interface
 *
 *  The generic scripting interface provides a way to export C++ classes to a scripting 
 *  language in some generic way. The main target scripting language is Ruby, but the concept
 *  is not restricted to this. 
 */

#include "tlAssert.h"
#include "tlInternational.h"
#include "tlException.h"
#include "tlTypeTraits.h"

#include "gsiObject.h"

namespace gsi
{
  /**
   *  @brief Initialize the GSI system 
   *
   *  This function needs to be called before all other operations. It builds the GSI
   *  method table and registers the classes for the tl::variant container.
   */
  void GSI_PUBLIC initialize ();
}

#endif

