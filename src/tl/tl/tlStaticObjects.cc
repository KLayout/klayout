
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#include "tlStaticObjects.h"

namespace tl
{

StaticObjects StaticObjects::ms_instance;

StaticObjects::~StaticObjects ()
{
  //  nothing here. That is done too late. Use destroy() instead.
}

void
StaticObjects::register_object_base (StaticObjectReferenceBase *o)
{
  m_objects.push_back (o);
}

void
StaticObjects::do_cleanup ()
{
  //  destroy the objects the opposite order they were created
  for (std::vector<StaticObjectReferenceBase *>::iterator o = m_objects.end (); o != m_objects.begin (); ) {
    --o;
    delete (*o);
  }
  m_objects.clear ();
}

}

