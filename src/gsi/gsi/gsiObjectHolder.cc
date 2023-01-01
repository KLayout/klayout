
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


#include "gsiObjectHolder.h"
#include "gsiDecl.h"

namespace gsi
{

ObjectHolder::ObjectHolder (const gsi::ClassBase *cls, void *obj)
  : mp_cls (cls), mp_obj (obj)
{
  //  .. nothing yet ..
}

ObjectHolder::~ObjectHolder ()
{
  reset (0, 0);
}

void ObjectHolder::reset (const gsi::ClassBase *cls, void *obj)
{
  if (mp_cls == cls && mp_obj == obj) {
    return;
  }

  if (mp_cls) {
    if (mp_obj) {
      mp_cls->destroy (mp_obj);
      mp_obj = 0;
    }
    mp_cls = 0;
  }

  if (cls) {
    mp_cls = cls;
    mp_obj = obj;
  }
}

void *ObjectHolder::release ()
{
  void *obj = mp_obj;
  mp_obj = 0;
  mp_cls = 0;
  return obj;
}

}

