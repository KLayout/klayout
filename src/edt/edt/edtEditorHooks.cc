
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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


#include "edtEditorHooks.h"
#include "tlObjectCollection.h"
#include "tlStaticObjects.h"

namespace edt
{

// ---------------------------------------------------------------
//  EditorHooksManager definition and implementation

static EditorHooksManager *sp_instance = 0;
static bool sp_instance_initialized = false;

class EditorHooksManager
{
public:
  EditorHooksManager ()
  {
    //  .. nothing yet ..
  }

  ~EditorHooksManager ()
  {
    sp_instance = 0;
  }

  static EditorHooksManager *instance ()
  {
    if (! sp_instance && ! sp_instance_initialized) {
      sp_instance = new EditorHooksManager ();
      sp_instance_initialized = true;
      tl::StaticObjects::reg (&sp_instance);
    }
    return sp_instance;
  }

  void register_editor_hook (EditorHooks *hook)
  {
    m_hooks.push_back (hook);
  }

  std::vector<EditorHooks *> get_editor_hooks (const std::string &for_technology)
  {
    std::vector<EditorHooks *> res;
    for (auto h = m_hooks.begin (); h != m_hooks.end (); ++h) {
      if (! h->for_technologies () || h->is_for_technology (for_technology)) {
        res.push_back (h.operator-> ());
      }
    }

    return res;
  }

private:
  tl::shared_collection<EditorHooks> m_hooks;
};

// ---------------------------------------------------------------
//  EditorHooks implementation

EditorHooks::EditorHooks ()
{
  //  .. nothing yet ..
}

EditorHooks::~EditorHooks ()
{
  //  .. nothing yet ..
}

bool
EditorHooks::is_for_technology (const std::string &name) const
{
  return m_technologies.find (name) != m_technologies.end ();
}

bool
EditorHooks::for_technologies () const
{
  return ! m_technologies.empty ();
}

void
EditorHooks::set_technology (const std::string &t)
{
  m_technologies.clear ();
  if (! t.empty ()) {
    m_technologies.insert (t);
  }
}

void
EditorHooks::clear_technologies ()
{
  m_technologies.clear ();
}

void
EditorHooks::add_technology (const std::string &tech)
{
  m_technologies.insert (tech);
}

void
EditorHooks::register_editor_hook (EditorHooks *hook)
{
  if (EditorHooksManager::instance ()) {
    hook->keep ();
    EditorHooksManager::instance ()->register_editor_hook (hook);
  }
}

std::vector<EditorHooks *>
EditorHooks::get_editor_hooks (const std::string &for_technology)
{
  if (EditorHooksManager::instance ()) {
    return EditorHooksManager::instance ()->get_editor_hooks (for_technology);
  } else {
    return std::vector<EditorHooks *> ();
  }
}

}
