
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



#include "layXORToolDialog.h"
#include "layDispatcher.h"

#include "layPlugin.h"

namespace lay
{

class XORPlugin
  : public lay::Plugin
{
public:
  XORPlugin (Plugin *parent, lay::LayoutView *view)
    : lay::Plugin (parent), mp_view (view)
  {
    mp_dialog = new lay::XORToolDialog (0);
  }

  ~XORPlugin ()
  {
    delete mp_dialog;
    mp_dialog = 0;
  }

  void menu_activated (const std::string &symbol) 
  {
    if (symbol == "lay::xor_tool") {

      if (mp_dialog->exec_dialog (mp_view)) {

        // ... implementation is in layXORToolDialog.cc ...

      }

    }
  }

private:
  lay::LayoutView *mp_view;
  lay::XORToolDialog *mp_dialog;
};

class XORPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  XORPluginDeclaration ()
  {
    //  .. nothing yet ..
  }
  
  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const
  {
    options.push_back (std::pair<std::string, std::string> (cfg_xor_input_mode, "all"));
    options.push_back (std::pair<std::string, std::string> (cfg_xor_output_mode, "rdb"));
    options.push_back (std::pair<std::string, std::string> (cfg_xor_nworkers, "1"));
    options.push_back (std::pair<std::string, std::string> (cfg_xor_layer_offset, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_xor_axorb, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_xor_anotb, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_xor_bnota, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_xor_summarize, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_xor_tolerances, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_xor_tiling, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_xor_region_mode, "all"));
  }

  virtual lay::ConfigPage *config_page (QWidget * /*parent*/, std::string & /*title*/) const
  {
    // .. nothing yet ..
    return 0;
  }

  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    lay::PluginDeclaration::get_menu_entries (menu_entries);
    menu_entries.push_back (lay::menu_item ("lay::xor_tool", "xor_tool:edit", "tools_menu.post_verification_group", tl::to_string (QObject::tr ("XOR Tool"))));
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
    return new XORPlugin (root, view);
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new lay::XORPluginDeclaration (), 3000, "lay::XORPlugin");

}

