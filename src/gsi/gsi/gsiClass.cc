
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


#include "gsiDecl.h"

#include "tlLog.h"
#include "tlInternational.h"

#include <map>

namespace gsi
{

//  Note: this is just a fallback implementation, mainly provided for debugging purposes.
//  The dummy class is just provided to avoid some assertions. 
struct EmptyClass { };

static gsi::Class<EmptyClass> default_cls ("tl", "EmptyClass", gsi::Methods ());

const ClassBase *fallback_cls_decl (const std::type_info &ti)
{
  //  This is the main purpose of this function: print the missing class
  tl::warn << tl::to_string (tr ("Unable to find GSI class binding for: ")) << ti.name ();
  return &default_cls;
}

}

