
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


#ifndef HDR_edtPlugin
#define HDR_edtPlugin

#include "layPlugin.h"

#include <vector>

namespace lay
{
  class PluginRoot;
}

namespace edt
{
  class EditorOptionsPage;

  /**
   *  @brief A helper class for plugin declarations for editor services
   */
  class PluginDeclarationBase
    : public lay::PluginDeclaration
  {
  public:
    virtual void get_editor_options_pages (std::vector<edt::EditorOptionsPage *> &, lay::PluginRoot *) const = 0;
  };

  /**
   *  @brief Show the editor options dialog
   *
   *  This dialog is a global resource which is managed by the main plugin declaration
   */
  void show_editor_options_dialog ();

  /**
   *  @brief Activate or deactivate a certain service
   *
   *  This will show or hide the editor properties pages for the respective service.
   */
  void activate_service (const lay::PluginDeclaration *pd, bool active);
}

#endif

