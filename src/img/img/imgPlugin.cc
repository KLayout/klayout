
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


#include "layPlugin.h"
#include "layAbstractMenu.h"
#include "imgService.h"
#include "imgPlugin.h"

namespace img
{

void 
PluginDeclaration::get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
{
  lay::PluginDeclaration::get_menu_entries (menu_entries);
  menu_entries.push_back (lay::MenuEntry ("image_group", "edit_menu.end"));
  menu_entries.push_back (lay::MenuEntry ("img::add_image", "add_image:edit", "edit_menu.end", tl::to_string (QObject::tr ("Add Image"))));
  menu_entries.push_back (lay::MenuEntry ("img::image_menu", "image_menu:edit", "edit_menu.end", tl::to_string (QObject::tr ("Images")), true));
  menu_entries.push_back (lay::MenuEntry ("img::bring_to_front", "bring_to_front:edit", "edit_menu.image_menu.end", tl::to_string (QObject::tr ("Image Stack: Selected Images to Front"))));
  menu_entries.push_back (lay::MenuEntry ("img::bring_to_back", "bring_to_back:edit", "edit_menu.image_menu.end", tl::to_string (QObject::tr ("Image Stack: Selected Images to Back"))));
  menu_entries.push_back (lay::MenuEntry ("img::clear_all_images", "clear_all_images:edit", "edit_menu.image_menu.end", tl::to_string (QObject::tr ("Clear All Images"))));
}

lay::Plugin *
PluginDeclaration::create_plugin (db::Manager *manager, lay::PluginRoot *, lay::LayoutView *view) const
{
  return new img::Service (manager, view);
}

bool 
PluginDeclaration::implements_editable (std::string &title) const
{
  title = tl::to_string (QObject::tr ("Images"));
  return true;
}

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new img::PluginDeclaration (), 4000, "img::Plugin");

}

