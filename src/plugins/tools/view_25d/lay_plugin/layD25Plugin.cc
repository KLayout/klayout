
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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



#include "layD25View.h"
#include "layDispatcher.h"

#include "layPlugin.h"

#include <QSurfaceFormat>

namespace lay
{

class D25Plugin
  : public lay::Plugin
{
public:
  D25Plugin (Plugin *parent, lay::LayoutView *view)
    : lay::Plugin (parent), mp_view (view)
  {
    mp_dialog = new lay::D25View (0);
  }

  ~D25Plugin ()
  {
    delete mp_dialog;
    mp_dialog = 0;
  }

  void menu_activated (const std::string &symbol) 
  {
    if (symbol == "lay::d25_view") {
      mp_dialog->exec_dialog (mp_view);
    }
  }

private:
  lay::LayoutView *mp_view;
  lay::D25View *mp_dialog;
};

class D25PluginDeclaration
  : public lay::PluginDeclaration
{
public:
  D25PluginDeclaration ()
  {
    //  .. nothing yet ..
  }
  
  virtual void get_options (std::vector < std::pair<std::string, std::string> > & /*options*/) const
  {
    //  .. nothing yet ..
  }

  virtual lay::ConfigPage *config_page (QWidget * /*parent*/, std::string & /*title*/) const
  {
    // .. nothing yet ..
    return 0;
  }

  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    lay::PluginDeclaration::get_menu_entries (menu_entries);
    menu_entries.push_back (lay::menu_item ("lay::d25_view", "d25_view:edit", "tools_menu.post_verification_group", tl::to_string (QObject::tr ("2.5d View - experimental"))));
  }

  virtual bool configure (const std::string & /*name*/, const std::string & /*value*/)
  {
    return false;
  }

  virtual void config_finalize ()
  {
    // .. nothing yet ..
  }

  lay::Plugin *create_plugin (db::Manager *, lay::Dispatcher *root, lay::LayoutView *view) const
  {
    return new D25Plugin (root, view);
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new lay::D25PluginDeclaration (), 3100, "lay::D25Plugin");

}

