
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

#include "dbNetTracerIO.h"

#include "layNetTracerTechComponentEditor.h"
#include "layNetTracerDialog.h"
#include "layNetTracerConfig.h"

#include "layConverters.h"
#include "layCellView.h"
#include "layLayoutView.h"
#include "layUtils.h"

#include "gsiDecl.h"

namespace lay
{

// -----------------------------------------------------------------------------------
//  NetTracerPlugin definition and implementation

class NetTracerPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const
  {
    options.push_back (std::pair<std::string, std::string> (cfg_nt_window_mode, "fit-net"));
    options.push_back (std::pair<std::string, std::string> (cfg_nt_window_dim, "1.0"));
    options.push_back (std::pair<std::string, std::string> (cfg_nt_max_shapes_highlighted, "10000"));
    options.push_back (std::pair<std::string, std::string> (cfg_nt_marker_color, lay::ColorConverter ().to_string (QColor ())));
    options.push_back (std::pair<std::string, std::string> (cfg_nt_marker_cycle_colors_enabled, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_nt_marker_cycle_colors, "255,0,0 0,255,0 0,0,255 255,255,0 255,0,255 0,255,255 160,80,255 255,160,0"));
    options.push_back (std::pair<std::string, std::string> (cfg_nt_marker_line_width, "-1"));
    options.push_back (std::pair<std::string, std::string> (cfg_nt_marker_vertex_size, "-1"));
    options.push_back (std::pair<std::string, std::string> (cfg_nt_marker_halo, "-1"));
    options.push_back (std::pair<std::string, std::string> (cfg_nt_marker_dither_pattern, "-1"));
    options.push_back (std::pair<std::string, std::string> (cfg_nt_marker_intensity, "50"));
  }

  virtual std::vector<std::pair <std::string, lay::ConfigPage *> > config_pages (QWidget *parent) const
  {
    std::vector<std::pair <std::string, lay::ConfigPage *> > pages;
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Other Tools|Net Tracer")), new NetTracerConfigPage (parent)));
    return pages;
  }

  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    //  TODO: where should that go?
    lay::PluginDeclaration::get_menu_entries (menu_entries);
    menu_entries.push_back (lay::separator ("net_trace_group", "tools_menu.end"));
    menu_entries.push_back (lay::menu_item ("lay::net_trace", "net_trace", "tools_menu.end", tl::to_string (QObject::tr ("Trace Net"))));
    menu_entries.push_back (lay::submenu ("", "trace_all_nets_menu", "tools_menu.end", tl::to_string (QObject::tr ("Trace All Nets"))));
    menu_entries.push_back (lay::menu_item ("lay::trace_all_nets", "trace_all_nets", "tools_menu.trace_all_nets_menu.end", tl::to_string (QObject::tr ("Hierarchical"))));
    menu_entries.push_back (lay::menu_item ("lay::trace_all_nets_flat", "trace_all_nets_flat", "tools_menu.trace_all_nets_menu.end", tl::to_string (QObject::tr ("Flat"))));
    menu_entries.push_back (lay::menu_item ("lay::edit_layer_stack", "edit_layer_stack", "tools_menu.end", tl::to_string (QObject::tr ("Edit Layer Stack"))));
  }

  virtual lay::Plugin *create_plugin (db::Manager * /*manager*/, lay::Dispatcher *root, lay::LayoutViewBase *view) const
  {
    if (lay::has_gui ()) {
      return new NetTracerDialog (root, view);
    } else {
      return 0;
    }
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new NetTracerPluginDeclaration (), 13000, "NetTracerPlugin");

class NetTracerTechnologyEditorProvider
  : public lay::TechnologyEditorProvider
{
public:
  virtual lay::TechnologyComponentEditor *create_editor (QWidget *parent) const
  {
    return new NetTracerTechComponentEditor (parent);
  }
};

static tl::RegisteredClass<lay::TechnologyEditorProvider> editor_decl (new NetTracerTechnologyEditorProvider (), 13000, db::net_tracer_component_name ().c_str ());

}

