
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


#ifndef HDR_edtPlugin
#define HDR_edtPlugin

#include "layPlugin.h"

#include <vector>

namespace lay
{
  class Dispatcher;
  class EditorOptionsPage;
  class LayoutViewBase;
}

namespace edt
{
  /**
   *  @brief A helper class for plugin declarations for editor services
   */
  class PluginDeclarationBase
    : public lay::PluginDeclaration
  {
    //  .. nothing yet ..
  };

  /**
   * @brief Returns a value indicating whether polygons are enabled in the "Select" menu
   */
  bool polygons_enabled ();

  //  other types ...
  bool paths_enabled ();
  bool boxes_enabled ();
  bool points_enabled ();
  bool texts_enabled ();
  bool instances_enabled ();

  /**
   *  @brief Commits the current configuration for the recently used configuration list
   */
  void commit_recent (lay::LayoutViewBase *view);
}

#endif

