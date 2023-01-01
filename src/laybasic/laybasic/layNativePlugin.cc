
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

#include "layNativePlugin.h"

#include "gsiDecl.h"

const klp_class_t *klp_class_by_name (const char *name)
{
  return reinterpret_cast<const klp_class_t *> (gsi::class_by_name (name));
}

void *klp_create (const klp_class_t *cls)
{
  return reinterpret_cast<const gsi::ClassBase *> (cls)->create ();
}

void klp_destroy (const klp_class_t *cls, void *obj)
{
  reinterpret_cast<const gsi::ClassBase *> (cls)->destroy (obj);
}

void *klp_clone (const klp_class_t *cls, const void *source)
{
  return reinterpret_cast<const gsi::ClassBase *> (cls)->clone (source);
}

void klp_assign (const klp_class_t *cls, void *target, const void *source)
{
  reinterpret_cast<const gsi::ClassBase *> (cls)->assign (target, source);
}

void klp_require_api_version (const char * /*version*/)
{
  //  TODO: implement
}
