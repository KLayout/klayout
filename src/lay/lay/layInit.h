
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


#ifndef HDR_layInit
#define HDR_layInit

#include "layCommon.h"
#include "layNativePlugin.h"

#include <string>
#include <vector>
#include <list>

namespace lay
{

/**
 *  @brief A tiny struct describing a native plugin
 */
struct PluginDescriptor
{
  typedef void (*runner_func_t) ();

  runner_func_t autorun;
  runner_func_t autorun_early;
  std::string version;
  std::string path;
  std::string description;

  PluginDescriptor ()
    : autorun (0), autorun_early (0)
  { }
};

/**
 *  @brief The main initialization function for the "db" module
 *  This function needs to be called initially by all code using the db
 *  module. It will load the plugins and perform initialization of all
 *  of them.
 */
LAY_PUBLIC void init (const std::vector<std::string> &paths = std::vector<std::string> ());

/**
 *  @brief Explicitly load a specific plugin
 */
LAY_PUBLIC void load_plugin (const std::string &pp);

/**
 *  @brief Gets a list of all plugins registered
 */
LAY_PUBLIC const std::list<lay::PluginDescriptor> &plugins ();

}

#endif
