
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


#ifndef HDR_antPlugin
#define HDR_antPlugin

#include "antCommon.h"

#include "layPlugin.h"

namespace ant
{

class Template;

class PluginDeclaration
  : public lay::PluginDeclaration
{
public:
  PluginDeclaration ();
  ~PluginDeclaration ();

  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const;
  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const;
  virtual lay::Plugin *create_plugin (db::Manager *manager, lay::Dispatcher *, lay::LayoutViewBase *view) const;
  virtual bool implements_editable (std::string &title) const;
  virtual bool implements_mouse_mode (std::string &title) const;
  virtual bool configure (const std::string &name, const std::string &value);
#if defined(HAVE_QT)
  virtual std::vector<std::pair <std::string, lay::ConfigPage *> > config_pages (QWidget *parent) const;
#endif
  virtual void config_finalize ();
  virtual void initialized (lay::Dispatcher *);
  virtual void uninitialize (lay::Dispatcher *);
  virtual bool menu_activated (const std::string &symbol) const;

  void register_annotation_template (const ant::Template &t, lay::Plugin *plugin = 0);
  void unregister_annotation_template (const std::string &category, lay::Plugin *plugin = 0);

  static PluginDeclaration *instance ();

private:
  void update_current_template ();
  void update_menu ();
  
  std::vector<ant::Template> m_templates;
  int m_current_template;
  tl::weak_collection<lay::ConfigureAction> m_actions;
  bool m_current_template_updated;
  bool m_templates_updated;
};

}

#endif

