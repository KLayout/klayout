
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


#include "gsiDecl.h"
#include "gsiDeclBasic.h"
#include "layDispatcher.h"

namespace gsi
{

static std::vector<std::string>
get_config_names (lay::Dispatcher *dispatcher)
{
  std::vector<std::string> names;
  dispatcher->get_config_names (names);
  return names;
}

static lay::Dispatcher *dispatcher_instance ()
{
  return lay::Dispatcher::instance ();
}

static tl::Variant get_config (lay::Dispatcher *dispatcher, const std::string &name, const tl::Variant &default_value)
{
  std::string value;
  if (dispatcher->config_get (name, value)) {
    return tl::Variant (value);
  } else {
    return default_value;
  }
}

/**
 *  @brief Exposes the Dispatcher interface
 *
 *  This interface is intentionally not derived from Plugin. It is used currently to 
 *  identify the dispatcher node for configuration. The Plugin nature of this interface
 *  is somewhat artificial and may be removed later. 
 *
 *  TODO: this is a duplicate of the respective methods in LayoutView and Application.
 *  This is intentional since we don't want to spend the only derivation path on this.
 *  Once there is a mixin concept, provide a path through that concept.
 */
Class<lay::Dispatcher> decl_Dispatcher ("lay", "Dispatcher",
  method ("clear_config", &lay::Dispatcher::clear_config,
    "@brief Clears the configuration parameters\n"
  ) +
  method ("instance", &dispatcher_instance,
    "@brief Gets the singleton instance of the Dispatcher object\n"
    "\n"
    "@return The instance\n"
  ) +
  method ("write_config", &lay::Dispatcher::write_config, gsi::arg ("file_name"),
    "@brief Writes configuration to a file\n"
    "@return A value indicating whether the operation was successful\n"
    "\n"
    "If the configuration file cannot be written, false \n"
    "is returned but no exception is thrown.\n"
  ) +
  method ("read_config", &lay::Dispatcher::read_config, gsi::arg ("file_name"),
    "@brief Reads the configuration from a file\n"
    "@return A value indicating whether the operation was successful\n"
    "\n"
    "This method silently does nothing, if the config file does not\n"
    "exist. If it does and an error occurred, the error message is printed\n"
    "on stderr. In both cases, false is returned.\n"
  ) +
  method_ext ("get_config", &get_config, gsi::arg ("name"), gsi::arg ("default", tl::Variant (), "nil"),
    "@brief Gets the value of a local configuration parameter\n"
    "\n"
    "@param name The name of the configuration parameter whose value shall be obtained (a string)\n"
    "\n"
    "@return The value of the parameter or the default value if there is no such parameter\n"
    "\n"
    "The default value has been added in version 0.30.6."
  ) +
  method ("set_config", (void (lay::Dispatcher::*) (const std::string &, const std::string &)) &lay::Dispatcher::config_set, gsi::arg ("name"), gsi::arg ("value"),
    "@brief Set a local configuration parameter with the given name to the given value\n"
    "\n"
    "@param name The name of the configuration parameter to set\n"
    "@param value The value to which to set the configuration parameter\n"
    "\n"
    "This method sets a configuration parameter with the given name to the given value. "
    "Values can only be strings. Numerical values have to be converted into strings first. "
    "Local configuration parameters override global configurations for this specific view. "
    "This allows for example to override global settings of background colors. "
    "Any local settings are not written to the configuration file. "
  ) +
  method_ext ("get_config_names", &get_config_names,
    "@brief Gets the configuration parameter names\n"
    "\n"
    "@return A list of configuration parameter names\n"
    "\n"
    "This method returns the names of all known configuration parameters. These names can be used to "
    "get and set configuration parameter values.\n"
  ) +
  method ("commit_config", &lay::Dispatcher::config_end,
    "@brief Commits the configuration settings\n"
    "\n"
    "Some configuration options are queued for performance reasons and become active only after 'commit_config' has been called. "
    "After a sequence of \\set_config calls, this method should be called to activate the "
    "settings made by these calls.\n"
  ),
  "@brief Root of the configuration space in the plugin context and menu dispatcher\n"
  "\n"
  "This class provides access to the root configuration space in the context "
  "of plugin programming. You can use this class to obtain configuration parameters "
  "from the configuration tree during plugin initialization. However, the "
  "preferred way of plugin configuration is through \\Plugin#configure.\n"
  "\n"
  "Currently, the application object provides an identical entry point for configuration modification. "
  "For example, \"Application::instance.set_config\" is identical to \"Dispatcher::instance.set_config\". "
  "Hence there is little motivation for the Dispatcher class currently and "
  "this interface may be modified or removed in the future."
  "\n"
  "This class has been introduced in version 0.25 as 'PluginRoot'.\n"
  "It is renamed and enhanced as 'Dispatcher' in 0.27."
);

}
