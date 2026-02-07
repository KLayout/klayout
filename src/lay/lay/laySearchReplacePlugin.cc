
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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


#include "laySearchReplaceDialog.h"
#include "laySearchReplaceConfigPage.h"

#include "layMainWindow.h"
#include "layApplication.h"
#include "layUtils.h"

namespace lay
{

// ------------------------------------------------------------
//  Declaration of the configuration options

class SearchReplacePluginDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const
  {
    options.push_back (std::pair<std::string, std::string> (cfg_sr_window_mode, "fit-marker"));
    options.push_back (std::pair<std::string, std::string> (cfg_sr_window_state, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_sr_window_dim, "1.0"));
    options.push_back (std::pair<std::string, std::string> (cfg_sr_max_item_count, "1000"));
  }

  virtual lay::ConfigPage *config_page (QWidget *parent, std::string &title) const
  {
    title = tl::to_string (QObject::tr ("Browsers|Search Result Browser"));
    return new SearchReplaceConfigPage (parent); 
  }

  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    lay::PluginDeclaration::get_menu_entries (menu_entries);

    //  separator:
    menu_entries.push_back (lay::separator ("search_replace_sep", "edit_menu.utils_group+"));

    //  two entries - one for view mode and one for edit mode:
    menu_entries.push_back (lay::menu_item ("search_replace::show", "search_replace_editor:edit:edit_mode", "edit_menu.utils_group+", tl::to_string (QObject::tr ("Search and Replace"))));
    menu_entries.push_back (lay::menu_item ("search_replace::show", "search_replace_viewer:edit:view_mode", "edit_menu.utils_group+", tl::to_string (QObject::tr ("Search"))));
  }
 
  virtual lay::Plugin *create_plugin (db::Manager *, lay::Dispatcher *root, lay::LayoutViewBase *view) const
  {
    if (lay::has_gui ()) {
      return new SearchReplaceDialog (root, view);
    } else {
      return 0;
    }
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new SearchReplacePluginDeclaration (), 20000, "SearchReplacePlugin");

}

