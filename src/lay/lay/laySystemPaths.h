
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


#ifndef HDR_laySystemPaths
#define HDR_laySystemPaths

#include "layCommon.h"

#include <string>
#include <vector>

namespace lay
{

/**
 *  @brief Gets the application data path 
 *  The application data path is the path where the application stores it's
 *  data for each user.
 *  By default this is HOME/.klayout or HOME/KLayout (Windows). The value
 *  can be overridden by the KLAYOUT_HOME environment variable.
 */
LAY_PUBLIC std::string get_appdata_path ();

/**
 *  @brief Gets the KLayout path 
 *  This is a path (in the sense of a search path - i.e. multiple paths)
 *  where KLayout (and derived applications) looks for configuration files.
 *  The path is created from the appdata and inst path by default. It can
 *  be overridden by the KLAYOUT_PATH environment variable.
 */
LAY_PUBLIC std::vector<std::string> get_klayout_path ();

/**
 *  @brief Sets the KLayout path
 *  This method is mainly used for test purposes. It will force the application
 *  to use a specific path. Use reset_klayout_path to restore the
 *  default behavior.
 */
LAY_PUBLIC void set_klayout_path (const std::vector<std::string> &path);

/**
 *  @brief Resets the KLayout path
 *  See "set_klayout_path" for a description.
 */
LAY_PUBLIC void reset_klayout_path ();

/**
 *  @brief Gets the package manager URL
 *  The default value can be overridden by the KLAYOUT_SALT_MINE environment
 *  variable.
 */
LAY_PUBLIC std::string salt_mine_url ();

}

#endif
