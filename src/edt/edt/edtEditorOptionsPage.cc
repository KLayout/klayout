
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


#include "tlInternational.h"
#include "edtEditorOptionsPage.h"
#include "edtEditorOptionsPages.h"

namespace edt
{

// ------------------------------------------------------------------
//  EditorOptionsPage implementation

EditorOptionsPage::EditorOptionsPage (lay::Dispatcher *dispatcher)
  : QWidget (0), mp_owner (0), m_active (true), mp_plugin_declaration (0), mp_dispatcher (dispatcher)
{
  //  nothing yet ..
}

EditorOptionsPage::~EditorOptionsPage ()
{
  set_owner (0);
}

void
EditorOptionsPage::set_owner (EditorOptionsPages *owner)
{
  if (mp_owner) {
    mp_owner->unregister_page (this);
  }
  mp_owner = owner;
}

void
EditorOptionsPage::activate (bool active)
{
  if (m_active != active) {
    m_active = active;
    if (mp_owner) {
      mp_owner->activate_page (this);
    }
  }
}

}
