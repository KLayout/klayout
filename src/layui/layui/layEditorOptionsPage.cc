
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#if defined(HAVE_QT)

#include "tlInternational.h"
#include "layEditorOptionsPage.h"
#include "layEditorOptionsPages.h"
#include "layLayoutViewBase.h"

namespace lay
{

// ------------------------------------------------------------------
//  EditorOptionsPage implementation

EditorOptionsPage::EditorOptionsPage (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
  : QWidget (0), mp_owner (0), m_active (true), mp_plugin_declaration (0), mp_dispatcher (dispatcher), mp_view (view)
{
  attach_events ();
}

EditorOptionsPage::~EditorOptionsPage ()
{
  set_owner (0);
}

void
EditorOptionsPage::attach_events ()
{
  detach_from_all_events ();
  view ()->active_cellview_changed_event.add (this, &EditorOptionsPage::on_active_cellview_changed);
  int cv_index = view ()->active_cellview_index ();
  if (cv_index >= 0) {
    view ()->cellview (cv_index)->technology_changed_event.add (this, &EditorOptionsPage::on_technology_changed);
  }
}

void
EditorOptionsPage::on_active_cellview_changed ()
{
  active_cellview_changed ();
  attach_events ();
}

void
EditorOptionsPage::on_technology_changed ()
{
  technology_changed (view ()->active_cellview_ref ()->tech_name ());
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

#endif
