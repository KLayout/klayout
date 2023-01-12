
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


#include "dbObject.h"

namespace db
{
  Object::~Object ()
  {
    //  release from any manager
    manager (0);
  }
 
  Object::Object (Manager *m)
    : m_id (0), mp_manager (0)
  {
    //  attach to the manager if required
    manager (m);
  }

  Object::Object (const Object &d)
    : m_id (0), mp_manager (0)
  {
    //  attach to the manager if required
    manager (d.manager ());
  }

  void 
  Object::manager (db::Manager *p_manager)
  {
    if (p_manager != mp_manager) {

      if (mp_manager) { 
        mp_manager->release_object (m_id);
      }

      if (p_manager) {
        mp_manager = p_manager;
        m_id = mp_manager->next_id (this);
      } else {
        mp_manager = 0;
        m_id = 0;
      }

    }
  }

}

