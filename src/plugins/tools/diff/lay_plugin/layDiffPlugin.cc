
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

#include "layDiffToolDialog.h"
#include "layDispatcher.h"

#include "layPlugin.h"
#include "layLayoutViewBase.h"
#include "layUtils.h"

#include <QPointer>

namespace lay
{

class DiffPlugin
  : public lay::Plugin
{
public:
  DiffPlugin (lay::LayoutViewBase *view)
    : lay::Plugin (view), mp_view (view)
  {
    if (lay::has_gui ()) {
      mp_dialog = new lay::DiffToolDialog (0);
    }
  }

  ~DiffPlugin ()
  {
    if (mp_dialog) {
      delete mp_dialog.data ();
    }
  }

  void menu_activated (const std::string &symbol) 
  {
    if (symbol == "lay::diff_tool") {

      if (mp_dialog && mp_dialog->exec_dialog (mp_view)) {

        // ... implementation is in layDiffToolDialog.cc ...

      }

    }
  }

private:
  lay::LayoutViewBase *mp_view;
  QPointer<lay::DiffToolDialog> mp_dialog;
};

class DiffPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  DiffPluginDeclaration ()
  {
    //  .. nothing yet ..
  }
  
  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const
  {
    options.push_back (std::pair<std::string, std::string> (cfg_diff_run_xor, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_diff_detailed, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_diff_summarize, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_diff_expand_cell_arrays, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_diff_exact, "false"));
  }

  virtual lay::ConfigPage *config_page (QWidget * /*parent*/, std::string & /*title*/) const
  {
    // .. nothing yet ..
    return 0;
  }

  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    lay::PluginDeclaration::get_menu_entries (menu_entries);
    menu_entries.push_back (lay::menu_item ("lay::diff_tool", "diff_tool:edit", "tools_menu.post_verification_group", tl::to_string (QObject::tr ("Diff Tool"))));
  }

  virtual bool configure (const std::string & /*name*/, const std::string & /*value*/)
  {
    return false;
  }

  virtual void config_finalize ()
  {
    // .. nothing yet ..
  }

  lay::Plugin *create_plugin (db::Manager *, lay::Dispatcher *, lay::LayoutViewBase *view) const
  {
    return new DiffPlugin (view);
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new lay::DiffPluginDeclaration (), 3001, "lay::DiffPlugin");

}

