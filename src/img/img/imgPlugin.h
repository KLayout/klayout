
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


#ifndef HDR_imgPlugin
#define HDR_imgPlugin

#include "layPlugin.h"

namespace img
{

extern const std::string cfg_images_visible;

class PluginDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const;
  virtual lay::Plugin *create_plugin (db::Manager *manager, lay::Dispatcher *, lay::LayoutViewBase *view) const;
  virtual bool implements_editable (std::string &title) const;
  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const;
};

}

#endif

