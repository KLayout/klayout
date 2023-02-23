
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


#ifndef HDR_layDispatcher
#define HDR_layDispatcher

#include "laybasicCommon.h"

#include "tlTypeTraits.h"
#include "tlObject.h"
#include "layPlugin.h"

#include <map>
#include <string>
#include <vector>

#if defined(HAVE_QT)
class QWidget;
#endif

namespace lay
{

class AbstractMenu;
class Action;
class ConfigureAction;

/**
 *  @brief A delegate by which the dispatcher can submit notification events
 */
class LAYBASIC_PUBLIC DispatcherDelegate
{
public:
  /**
   *  @brief Notifies the plugin root that a new plugin class has been registered
   *
   *  This method is called when a plugin is loaded dynamically during runtime.
   */
  virtual void plugin_registered (lay::PluginDeclaration * /*cls*/)
  {
    // .. this implementation does nothing ..
  }

  /**
   *  @brief Notifies the plugin root that a plugin class is about to be removed
   */
  virtual void plugin_removed (lay::PluginDeclaration * /*cls*/)
  {
    // .. this implementation does nothing ..
  }

  /**
   *  @brief Selects the given mode
   *
   *  The implementation is supposed to select the given mode on all related plugins.
   */
  virtual void select_mode (int /*mode*/)
  {
    // .. this implementation does nothing ..
  }

  /**
   *  @brief Menu command handler
   */
  virtual void menu_activated (const std::string & /*symbol*/)
  {
    // .. this implementation does nothing ..
  }

  /**
   *  @brief Receives configuration events
   */
  virtual bool configure (const std::string & /*name*/, const std::string & /*value*/)
  {
    return false;
  }

  /**
   *  @brief Configuration finalization
   */
  virtual void config_finalize ()
  {
    //  .. the default implementation does nothing ..
  }
};

/**
 *  @brief The central menu event and configuration dispatcher class
 *
 *  This class acts as the top level dispatcher for plugin events and the menu configuration.
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
  Dispatcher (Plugin *parent = 0, bool standalone = false);

  /**
   *  @brief The root constructor
   *
   *  @param delegate The notification receiver for dispatcher events
   */
  Dispatcher (DispatcherDelegate *delegate, Plugin *parent = 0, bool standalone = false);

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
   *  This method silently does nothing, if the config file does not
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
  virtual void plugin_registered (lay::PluginDeclaration *cls)
  {
    if (mp_delegate) {
      mp_delegate->plugin_registered (cls);
    }
  }

  /**
   *  @brief Notifies the plugin root that a plugin class is about to be removed
   */
  virtual void plugin_removed (lay::PluginDeclaration *cls)
  {
    if (mp_delegate) {
      mp_delegate->plugin_removed (cls);
    }
  }

  /**
   *  @brief Selects the given mode
   *
   *  The implementation is supposed to select the given mode on all related plugins.
   */
  virtual void select_mode (int mode)
  {
    if (mp_delegate) {
      mp_delegate->select_mode (mode);
    }
  }

  /**
   *  @brief Called, when a menu item is selected
   */
  virtual void menu_activated (const std::string &symbol)
  {
    if (mp_delegate) {
      mp_delegate->menu_activated (symbol);
    }
  }

#if defined(HAVE_QT)
  /**
   *  @brief Gets the parent widget
   */
  QWidget *menu_parent_widget ()
  {
    return mp_menu_parent_widget;
  }

  /**
   *  @brief Sets the parent widget
   */
  void set_menu_parent_widget (QWidget *pw);

  /**
   *  @brief Returns true, if the dispatcher supplies a user interface
   */
  bool has_ui () { return menu_parent_widget () != 0; }

#else
  /**
   *  @brief Returns true, if the dispatcher supplies a user interface
   */
  bool has_ui () { return false; }
#endif

  /**
   *  @brief Creates the menu object
   *
   *  This method will only have an effect on the root dispatcher.
   */
  void make_menu ();

  /**
   *  @brief Gets the AbstractMenu object
   *
   *  This will deliver the actual menu - the one that is the root dispatcher's menu
   */
  AbstractMenu *menu ()
  {
    return (dispatcher () == this) ? mp_menu.get () : dispatcher ()->menu ();
  }

protected:
  //  capture the configuration events so we can change the value of the configuration actions
  virtual bool configure (const std::string &name, const std::string &value);
  virtual void config_finalize ();

private:
  Dispatcher (const Dispatcher &);
  Dispatcher &operator= (const Dispatcher &);

  std::unique_ptr<lay::AbstractMenu> mp_menu;
#if defined(HAVE_QT)
  QWidget *mp_menu_parent_widget;
#endif
  DispatcherDelegate *mp_delegate;
};

}

#endif
