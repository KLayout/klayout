
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


#ifndef HDR_layDispatcher
#define HDR_layDispatcher

#include "laybasicCommon.h"

#include "tlTypeTraits.h"
#include "tlObject.h"
#include "layPlugin.h"

#include <map>
#include <string>
#include <vector>

namespace lay
{

class AbstractMenu;
class Action;
class ConfigureAction;

/**
 *  @brief The central menu event and configuration dispatcher class
 *
 *  This class acts as the top level dispatcher for plugin events and the menu configuration.
 *
 */
class LAYBASIC_PUBLIC Dispatcher
  : public Plugin
{
public:
  /**
   *  @brief The constructor
   *
   *  @param parent Usually 0, but a dispatcher may have parents. In this case, the dispatcher is not the actual dispatcher, but the real plugin chain's root is.
   *  @param standalone The standalone flag passed to the plugin constructor.
   */
  Dispatcher (Plugin *parent, bool standalone = false);

  /**
   *  @brief Destructor
   */
  ~Dispatcher ();

  /**
   *  @brief Write configuration to a file
   *
   *  If the configuration file cannot be written, false
   *  is returned but no exception is thrown.
   *
   *  @return false, if an error occurred.
   */
  bool write_config (const std::string &config_file);

  /**
   *  @brief Read the configuration from a file
   *
   *  This method siletly does nothing, if the config file does not
   *  exist. If it does and an error occurred, the error message is printed
   *  on stderr. In both cases, false is returned.
   *
   *  @return false, if an error occurred.
   */
  bool read_config (const std::string &config_file);

  /**
   *  @brief The singleton instance of the plugin root
   */
  static Dispatcher *instance ();

  /**
   *  @brief Notifies the plugin root that a new plugin class has been registered
   *
   *  This method is called when a plugin is loaded dynamically during runtime.
   */
  virtual void plugin_registered (lay::PluginDeclaration * /*cls*/) { }

  /**
   *  @brief Notifies the plugin root that a plugin class is about to be removed
   */
  virtual void plugin_removed (lay::PluginDeclaration * /*cls*/) { }

  /**
   *  @brief Selects the given mode
   *
   *  The implementation is supposed to select the given mode on all related plugins.
   */
  virtual void select_mode (int /*mode*/) { }

  /**
   *  @brief Gets the parent widget
   */
  virtual QWidget *menu_parent_widget () { return 0; }

  /**
   *  @brief Returns true, if the dispatcher supplies a user interface
   */
  virtual bool has_ui () { return menu_parent_widget () != 0; }

  /**
   *  @brief Gets the AbstractMenu object
   *
   *  This will deliver the actual menu - the one that is the root dispatcher's menu
   */
  AbstractMenu *menu ()
  {
    return (dispatcher () == this) ? &m_menu : dispatcher ()->menu ();
  }

protected:
  //  capture the configuration events so we can change the value of the configuration actions
  virtual bool configure (const std::string &name, const std::string &value);

private:
  Dispatcher (const Dispatcher &);
  Dispatcher &operator= (const Dispatcher &);

  lay::AbstractMenu m_menu;
};

}

namespace tl
{
  //  disable copy ctor for Dispatcher
  template <> struct type_traits<lay::Dispatcher> : public type_traits<void> {
    typedef tl::false_tag has_copy_constructor;
    typedef tl::false_tag has_default_constructor;
  };
}

#endif


