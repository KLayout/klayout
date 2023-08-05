
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


#include "gsiDecl.h"
#include "gsiDeclBasic.h"
#include "layPlugin.h"
#include "layViewObject.h"
#include "layLayoutViewBase.h"
#include "layCursor.h"

namespace gsi
{
  class PluginFactoryBase;
  class PluginBase;

//  TODO: these static variables are a bad hack!
//  However it's not easy to pass parameters to a C++ classes constructor in Ruby without
//  compromising the capability to derive from that class (at least I have not learned how
//  to create a "new" in the super class *and* allow a "new" of the derived class). Anyway,
//  since PluginBase object are only allowed to be created inside the create_plugin method
//  of the factory, this hack is a quick but dirty workaround.
static bool s_in_create_plugin = false;
static lay::LayoutViewBase *sp_view = 0;
static lay::Dispatcher *sp_dispatcher = 0;

class PluginBase
  : public lay::Plugin, public lay::ViewService
{
public:
  PluginBase ()
    : lay::Plugin (sp_dispatcher), lay::ViewService (sp_view ? sp_view->canvas () : 0)
  {
    if (! s_in_create_plugin) {
      throw tl::Exception (tl::to_string (tr ("A PluginBase object can only be created in the PluginFactory's create_plugin method")));
    }
  }

  void grab_mouse ()
  {
    if (ui ()) {
      ui ()->grab_mouse (this, false);
    }
  }

  void ungrab_mouse ()
  {
    if (ui ()) {
      ui ()->ungrab_mouse (this);
    }
  }

  void set_cursor (int c)
  {
    if (ui ()) {
      lay::ViewService::set_cursor ((enum lay::Cursor::cursor_shape) c);
    }
  }

  virtual lay::ViewService *view_service_interface ()
  {
    return this;
  }

  virtual void menu_activated (const std::string &symbol) 
  {
    if (f_menu_activated.can_issue ()) {
      f_menu_activated.issue<lay::Plugin, const std::string &> (&lay::Plugin::menu_activated, symbol);
    } else {
      lay::Plugin::menu_activated (symbol);
    }
  }

  virtual bool configure (const std::string &name, const std::string &value)
  {
    return f_configure.can_issue () ? f_configure.issue<PluginBase, bool, const std::string &, const std::string &> (&PluginBase::configure, name, value) : lay::Plugin::configure (name, value);
  }

  virtual void config_finalize ()
  {
    f_config_finalize.can_issue () ? f_config_finalize.issue<PluginBase> (&PluginBase::config_finalize) : lay::Plugin::config_finalize ();
  }

  virtual bool key_event (unsigned int key, unsigned int buttons) 
  {
    if (f_key_event.can_issue ()) {
      return f_key_event.issue<lay::ViewService, bool, unsigned int, unsigned int> (&lay::ViewService::key_event, key, buttons);
    } else {
      return lay::ViewService::key_event (key, buttons);
    }
  }

  virtual bool mouse_press_event (const db::DPoint &p, unsigned int buttons, bool prio) 
  {
    if (f_mouse_press_event.can_issue ()) {
      return f_mouse_press_event.issue (&PluginBase::mouse_press_event_noref, p, buttons, prio);
    } else {
      return lay::ViewService::mouse_press_event (p, buttons, prio);
    }
  }

  //  NOTE: this version doesn't take a point reference which allows up to store the point
  bool mouse_press_event_noref (db::DPoint p, unsigned int buttons, bool prio)
  {
    return mouse_press_event (p, buttons, prio);
  }

  virtual bool mouse_click_event (const db::DPoint &p, unsigned int buttons, bool prio) 
  {
    if (f_mouse_click_event.can_issue ()) {
      return f_mouse_click_event.issue (&PluginBase::mouse_click_event_noref, p, buttons, prio);
    } else {
      return lay::ViewService::mouse_click_event (p, buttons, prio);
    }
  }

  //  NOTE: this version doesn't take a point reference which allows up to store the point
  bool mouse_click_event_noref (db::DPoint p, unsigned int buttons, bool prio)
  {
    return mouse_click_event (p, buttons, prio);
  }

  virtual bool mouse_double_click_event (const db::DPoint &p, unsigned int buttons, bool prio)
  {
    if (f_mouse_double_click_event.can_issue ()) {
      return f_mouse_double_click_event.issue (&PluginBase::mouse_double_click_event_noref, p, buttons, prio);
    } else {
      return lay::ViewService::mouse_double_click_event (p, buttons, prio);
    }
  }

  //  NOTE: this version doesn't take a point reference which allows up to store the point
  bool mouse_double_click_event_noref (db::DPoint p, unsigned int buttons, bool prio)
  {
    return mouse_double_click_event (p, buttons, prio);
  }

  virtual bool leave_event (bool prio)
  {
    if (f_leave_event.can_issue ()) {
      return f_leave_event.issue<lay::ViewService, bool, bool> (&lay::ViewService::leave_event, prio);
    } else {
      return lay::ViewService::leave_event (prio);
    }
  }

  virtual bool enter_event (bool prio)
  {
    if (f_enter_event.can_issue ()) {
      return f_enter_event.issue<lay::ViewService, bool, bool> (&lay::ViewService::enter_event, prio);
    } else {
      return lay::ViewService::enter_event (prio);
    }
  }

  virtual bool mouse_move_event (const db::DPoint &p, unsigned int buttons, bool prio)
  {
    if (f_mouse_move_event.can_issue ()) {
      return f_mouse_move_event.issue (&PluginBase::mouse_move_event_noref, p, buttons, prio);
    } else {
      return lay::ViewService::mouse_move_event (p, buttons, prio);
    }
  }

  //  NOTE: this version doesn't take a point reference which allows up to store the point
  bool mouse_move_event_noref (db::DPoint p, unsigned int buttons, bool prio)
  {
    return mouse_move_event (p, buttons, prio);
  }

  virtual bool mouse_release_event (const db::DPoint &p, unsigned int buttons, bool prio)
  {
    if (f_mouse_release_event.can_issue ()) {
      return f_mouse_release_event.issue (&PluginBase::mouse_release_event_noref, p, buttons, prio);
    } else {
      return lay::ViewService::mouse_release_event (p, buttons, prio);
    }
  }

  //  NOTE: this version doesn't take a point reference which allows up to store the point
  bool mouse_release_event_noref (db::DPoint p, unsigned int buttons, bool prio)
  {
    return mouse_release_event (p, buttons, prio);
  }

  virtual bool wheel_event (int delta, bool horizontal, const db::DPoint &p, unsigned int buttons, bool prio)
  {
    if (f_wheel_event.can_issue ()) {
      return f_wheel_event.issue (&PluginBase::wheel_event_noref, delta, horizontal, p, buttons, prio);
    } else {
      return lay::ViewService::wheel_event (delta, horizontal, p, buttons, prio);
    }
  }

  //  NOTE: this version doesn't take a point reference which allows up to store the point
  bool wheel_event_noref (int delta, bool horizontal, db::DPoint p, unsigned int buttons, bool prio)
  {
    return wheel_event (delta, horizontal, p, buttons, prio);
  }

  virtual void activated ()
  {
    if (f_activated.can_issue ()) {
      f_activated.issue<lay::ViewService> (&lay::ViewService::activated);
    } else {
      lay::ViewService::activated ();
    }
  }

  virtual void deactivated ()
  {
    if (f_deactivated.can_issue ()) {
      f_deactivated.issue<lay::ViewService> (&lay::ViewService::deactivated);
    } else {
      lay::ViewService::deactivated ();
    }
  }

  virtual void drag_cancel ()
  {
    if (f_drag_cancel.can_issue ()) {
      f_drag_cancel.issue<lay::ViewService> (&lay::ViewService::drag_cancel);
    } else {
      lay::ViewService::drag_cancel ();
    }
  }

  virtual void update ()
  {
    if (f_update.can_issue ()) {
      f_update.issue<lay::ViewService> (&lay::ViewService::update);
    } else {
      lay::ViewService::update ();
    }
  }

  virtual bool has_tracking_position () const
  {
    if (f_has_tracking_position.can_issue ()) {
      return f_has_tracking_position.issue<lay::ViewService, bool> (&lay::ViewService::has_tracking_position);
    } else {
      return lay::ViewService::has_tracking_position ();
    }
  }

  virtual db::DPoint tracking_position () const
  {
    if (f_tracking_position.can_issue ()) {
      return f_tracking_position.issue<lay::ViewService, db::DPoint> (&lay::ViewService::tracking_position);
    } else {
      return lay::ViewService::tracking_position ();
    }
  }

  gsi::Callback f_menu_activated;
  gsi::Callback f_configure;
  gsi::Callback f_config_finalize;
  gsi::Callback f_key_event;
  gsi::Callback f_mouse_press_event;
  gsi::Callback f_mouse_click_event;
  gsi::Callback f_mouse_double_click_event;
  gsi::Callback f_leave_event;
  gsi::Callback f_enter_event;
  gsi::Callback f_mouse_move_event;
  gsi::Callback f_mouse_release_event;
  gsi::Callback f_wheel_event;
  gsi::Callback f_activated;
  gsi::Callback f_deactivated;
  gsi::Callback f_drag_cancel;
  gsi::Callback f_update;
  gsi::Callback f_has_tracking_position;
  gsi::Callback f_tracking_position;
};

static std::map <std::string, PluginFactoryBase *> s_factories;

class PluginFactoryBase
  : public lay::PluginDeclaration
{
public:
  PluginFactoryBase ()
    : PluginDeclaration (), 
      m_implements_mouse_mode (true), mp_registration (0) 
  {
    //  .. nothing yet ..
  }

  ~PluginFactoryBase ()
  {
    for (auto f = s_factories.begin (); f != s_factories.end (); ++f) {
      if (f->second == this) {
        s_factories.erase (f);
        break;
      }
    }

    delete mp_registration;
    mp_registration = 0;
  }

  void register_gsi (int position, const char *name, const char *title)
  {
    register_gsi2 (position, name, title, 0);
  }

  void register_gsi2 (int position, const char *name, const char *title, const char *icon)
  {
    //  makes the object owned by the C++ side
    keep ();

    //  remove an existing factory with the same name
    std::map <std::string, PluginFactoryBase *>::iterator f = s_factories.find (name);
    if (f != s_factories.end ()) {
      delete f->second;
      f->second = this;
    } else {
      s_factories.insert (std::make_pair (std::string (name), this));
    }

    //  cancel any previous registration and register (again)
    delete mp_registration;
    mp_registration = new tl::RegisteredClass<lay::PluginDeclaration> (this, position, name, false /*does not own object*/);

    m_mouse_mode_title = name;
    if (title) {
      m_mouse_mode_title += "\t";
      m_mouse_mode_title += title;
    }
    if (icon) {
      m_mouse_mode_title += "\t<";
      m_mouse_mode_title += icon;
      m_mouse_mode_title += ">";
    }

    //  (dynamically) register the plugin class. This will also call initialize if the main window is 
    //  present already.
    register_plugin ();
  }

  virtual bool configure (const std::string &name, const std::string &value)
  {
    if (f_configure.can_issue ()) {
      return f_configure.issue<lay::PluginDeclaration, bool, const std::string &, const std::string &> (&lay::PluginDeclaration::configure, name, value);
    } else {
      return lay::PluginDeclaration::configure (name, value);
    }
  }

  virtual void config_finalize ()
  {
    if (f_config_finalize.can_issue ()) {
      f_config_finalize.issue<lay::PluginDeclaration> (&lay::PluginDeclaration::config_finalize);
    } else {
      lay::PluginDeclaration::config_finalize ();
    }
  }

  virtual bool menu_activated (const std::string &symbol) const
  {
    if (f_menu_activated.can_issue ()) {
      return f_menu_activated.issue<lay::PluginDeclaration, bool, const std::string &> (&lay::PluginDeclaration::menu_activated, symbol);
    } else {
      return lay::PluginDeclaration::menu_activated (symbol);
    }
  }

  virtual void initialize (lay::Dispatcher *root)
  {
    if (f_initialize.can_issue ()) {
      f_initialize.issue<lay::PluginDeclaration> (&lay::PluginDeclaration::initialize, root);
    } else {
      lay::PluginDeclaration::initialize (root);
    }
  }
    
  virtual void uninitialize (lay::Dispatcher *root)
  {
    if (f_uninitialize.can_issue ()) {
      f_uninitialize.issue<lay::PluginDeclaration> (&lay::PluginDeclaration::uninitialize, root);
    } else {
      lay::PluginDeclaration::uninitialize (root);
    }
  }

  virtual lay::Plugin *create_plugin (db::Manager *manager, lay::Dispatcher *root, lay::LayoutViewBase *view) const
  { 
    if (f_create_plugin.can_issue ()) {
      return create_plugin_gsi (manager, root, view);
    } else {
      return lay::PluginDeclaration::create_plugin (manager, root, view);
    }
  }

  virtual gsi::PluginBase *create_plugin_gsi (db::Manager *manager, lay::Dispatcher *root, lay::LayoutViewBase *view) const
  { 
    //  TODO: this is a hack. See notes above at s_in_create_plugin
    s_in_create_plugin = true;
    sp_view = view;
    sp_dispatcher = root;
    gsi::PluginBase *ret = 0;
    try {
      ret = f_create_plugin.issue<PluginFactoryBase, gsi::PluginBase *, db::Manager *, lay::Dispatcher *, lay::LayoutViewBase *> (&PluginFactoryBase::create_plugin_gsi, manager, root, view);
      s_in_create_plugin = false;
      sp_view = 0;
      sp_dispatcher = 0;
    } catch (...) {
      s_in_create_plugin = false;
      sp_view = 0;
      sp_dispatcher = 0;
    }

    return ret;
  }

  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    menu_entries = m_menu_entries;
  }

  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const 
  {
    options = m_options;
  }

  void add_menu_entry1 (const std::string &menu_name, const std::string &insert_pos)
  {
    m_menu_entries.push_back (lay::separator (menu_name, insert_pos));
  }

  void add_menu_entry2 (const std::string &symbol, const std::string &menu_name, const std::string &insert_pos, const std::string &title)
  {
    m_menu_entries.push_back (lay::menu_item (symbol, menu_name, insert_pos, title));
  }

  void add_menu_entry_copy (const std::string &symbol, const std::string &menu_name, const std::string &insert_pos, const std::string &copy_from)
  {
    m_menu_entries.push_back (lay::menu_item_copy (symbol, menu_name, insert_pos, copy_from));
  }

  void add_submenu (const std::string &menu_name, const std::string &insert_pos, const std::string &title)
  {
    m_menu_entries.push_back (lay::submenu (menu_name, insert_pos, title));
  }

  void add_config_menu_item (const std::string &menu_name, const std::string &insert_pos, const std::string &title, const std::string &cname, const std::string &cvalue)
  {
    m_menu_entries.push_back (lay::config_menu_item (menu_name, insert_pos, title, cname, cvalue));
  }

  void add_menu_entry3 (const std::string &symbol, const std::string &menu_name, const std::string &insert_pos, const std::string &title, bool sub_menu)
  {
    if (sub_menu) {
      m_menu_entries.push_back (lay::submenu (symbol, menu_name, insert_pos, title));
    } else {
      m_menu_entries.push_back (lay::menu_item (symbol, menu_name, insert_pos, title));
    }
  }

  void add_option (const std::string &name, const std::string &default_value)
  {
    m_options.push_back (std::make_pair (name, default_value));
  }

  void has_tool_entry (bool f)
  {
    m_implements_mouse_mode = f;
  }

  virtual bool implements_mouse_mode (std::string &title) const
  {
    title = m_mouse_mode_title;
    return m_implements_mouse_mode;
  }

  gsi::Callback f_create_plugin;
  gsi::Callback f_initialize;
  gsi::Callback f_uninitialize;
  gsi::Callback f_configure;
  gsi::Callback f_config_finalize;
  gsi::Callback f_menu_activated;

private:
  std::vector<std::pair<std::string, std::string> > m_options;
  std::vector<lay::MenuEntry> m_menu_entries;
  bool m_implements_mouse_mode;
  std::string m_mouse_mode_title;
  tl::RegisteredClass <lay::PluginDeclaration> *mp_registration;
};

Class<gsi::PluginFactoryBase> decl_PluginFactory ("lay", "PluginFactory",
  method ("register", &PluginFactoryBase::register_gsi, gsi::arg ("position"), gsi::arg ("name"), gsi::arg ("title"),
    "@brief Registers the plugin factory\n"
    "@param position An integer that determines the order in which the plugins are created. The internal plugins use the values from 1000 to 50000.\n"
    "@param name The plugin name. This is an arbitrary string which should be unique. Hence it is recommended to use a unique prefix, i.e. \"myplugin::ThePluginClass\".\n"
    "@param title The title string which is supposed to appear in the tool bar and menu related to this plugin.\n"
    "\n"
    "Registration of the plugin factory makes the object known to the system. Registration requires that the menu items have been set "
    "already. Hence it is recommended to put the registration at the end of the initialization method of the factory class.\n"
  ) + 
  method ("register", &PluginFactoryBase::register_gsi2, gsi::arg ("position"), gsi::arg ("name"), gsi::arg ("title"), gsi::arg ("icon"),
    "@brief Registers the plugin factory\n"
    "@param position An integer that determines the order in which the plugins are created. The internal plugins use the values from 1000 to 50000.\n"
    "@param name The plugin name. This is an arbitrary string which should be unique. Hence it is recommended to use a unique prefix, i.e. \"myplugin::ThePluginClass\".\n"
    "@param title The title string which is supposed to appear in the tool bar and menu related to this plugin.\n"
    "@param icon The path to the icon that appears in the tool bar and menu related to this plugin.\n" 
    "\n"
    "This version also allows registering an icon for the tool bar.\n"
    "\n"
    "Registration of the plugin factory makes the object known to the system. Registration requires that the menu items have been set "
    "already. Hence it is recommended to put the registration at the end of the initialization method of the factory class.\n"
  ) + 
  callback ("configure", &gsi::PluginFactoryBase::configure, &gsi::PluginFactoryBase::f_configure, gsi::arg ("name"), gsi::arg ("value"),
    "@brief Gets called for configuration events for the plugin singleton\n"
    "This method can be reimplemented to receive configuration events "
    "for the plugin singleton. Before a configuration can be received it must be "
    "registered by calling \\add_option in the plugin factories' constructor.\n"
    "\n"
    "The implementation of this method may return true indicating that the configuration request "
    "will not be handled by further modules. It's more cooperative to return false which will "
    "make the system distribute the configuration request to other receivers as well.\n"
    "\n"
    "@param name The configuration key\n"
    "@param value The value of the configuration variable\n"
    "@return True to stop further processing\n"
  ) +
  callback ("config_finalize", &gsi::PluginFactoryBase::config_finalize, &gsi::PluginFactoryBase::f_config_finalize,
    "@brief Gets called after a set of configuration events has been sent\n"
    "This method can be reimplemented and is called after a set of configuration events "
    "has been sent to the plugin factory singleton with \\configure. It can be used to "
    "set up user interfaces properly for example.\n"
  ) +
  callback ("menu_activated", &gsi::PluginFactoryBase::menu_activated, &gsi::PluginFactoryBase::f_menu_activated, gsi::arg ("symbol"),
    "@brief Gets called when a menu item is selected\n"
    "\n"
    "Usually, menu-triggered functionality is implemented in the per-view instance of the plugin. "
    "However, using this method it is possible to implement functionality globally for all plugin "
    "instances. The symbol is the string registered with the specific menu item in the \\add_menu_item "
    "call.\n"
    "\n"
    "If this method was handling the menu event, it should return true. This indicates that the event "
    "will not be propagated to other plugins hence avoiding duplicate calls.\n"
  ) +
  callback ("initialized", &gsi::PluginFactoryBase::initialize, &gsi::PluginFactoryBase::f_initialize, gsi::arg ("dispatcher"),
    "@brief Gets called when the plugin singleton is initialized, i.e. when the application has been started.\n"
    "@param dispatcher The reference to the \\MainWindow object\n"
  ) +
  callback ("uninitialized", &gsi::PluginFactoryBase::uninitialize, &gsi::PluginFactoryBase::f_uninitialize, gsi::arg ("dispatcher"),
    "@brief Gets called when the application shuts down and the plugin is unregistered\n"
    "This event can be used to free resources allocated with this factory singleton.\n"
    "@param dispatcher The reference to the \\MainWindow object\n"
  ) +
  factory_callback ("create_plugin", &gsi::PluginFactoryBase::create_plugin_gsi, &gsi::PluginFactoryBase::f_create_plugin, gsi::arg ("manager"), gsi::arg ("dispatcher"), gsi::arg ("view"),
    "@brief Creates the plugin\n"
    "This is the basic functionality that the factory must provide. This method must create a plugin of the "
    "specific type.\n"
    "@param manager The database manager object responsible for handling database transactions\n"
    "@param dispatcher The reference to the \\MainWindow object\n"
    "@param view The \\LayoutView that is plugin is created for\n"
    "@return The new \\Plugin implementation object\n"
  ) +
  method ("add_menu_entry", &gsi::PluginFactoryBase::add_menu_entry1, gsi::arg ("menu_name"), gsi::arg ("insert_pos"),
    "@brief Specifies a separator\n"
    "Call this method in the factory constructor to build the menu items that this plugin shall create.\n"
    "This specific call inserts a separator at the given position (insert_pos). The position uses abstract menu item paths "
    "and \"menu_name\" names the component that will be created. See \\AbstractMenu for a description of the path.\n"
  ) +
  method ("add_menu_entry", &gsi::PluginFactoryBase::add_menu_entry2, gsi::arg ("symbol"), gsi::arg ("menu_name"), gsi::arg ("insert_pos"), gsi::arg ("title"),
    "@brief Specifies a menu item\n"
    "Call this method in the factory constructor to build the menu items that this plugin shall create.\n"
    "This specific call inserts a menu item at the specified position (insert_pos). The position uses abstract menu item paths "
    "and \"menu_name\" names the component that will be created. See \\AbstractMenu for a description of the path.\n"
    "When the menu item is selected \"symbol\" is the string that is sent to the \\menu_activated callback (either the global one for the factory ot the one of the per-view plugin instance).\n"
    "\n"
    "@param symbol The string to send to the plugin if the menu is triggered\n"
    "@param menu_name The name of entry to create at the given position\n"
    "@param insert_pos The position where to create the entry\n"
    "@param title The title string for the item. The title can contain a keyboard shortcut in round braces after the title text, i.e. \"My Menu Item(F12)\"\n"
  ) +
  method ("#add_menu_entry", &gsi::PluginFactoryBase::add_menu_entry3, gsi::arg ("symbol"), gsi::arg ("menu_name"), gsi::arg ("insert_pos"), gsi::arg ("title"), gsi::arg ("sub_menu"),
    "@brief Specifies a menu item or sub-menu\n"
    "Similar to the previous form of \"add_menu_entry\", but this version allows also to create sub-menus by setting the "
    "last parameter to \"true\".\n"
    "\n"
    "With version 0.27 it's more convenient to use \\add_submenu."
  ) +
  method ("add_menu_item_clone", &gsi::PluginFactoryBase::add_menu_entry_copy, gsi::arg ("symbol"), gsi::arg ("menu_name"), gsi::arg ("insert_pos"), gsi::arg ("copy_from"),
    "@brief Specifies a menu item as a clone of another one\n"
    "Using this method, a menu item can be made a clone of another entry (given as path by 'copy_from').\n"
    "The new item will share the \\Action object with the original one, so manipulating the action will change both the original entry "
    "and the new entry.\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  method ("add_submenu", &gsi::PluginFactoryBase::add_submenu, gsi::arg ("menu_name"), gsi::arg ("insert_pos"), gsi::arg ("title"),
    "@brief Specifies a menu item or sub-menu\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  method ("add_config_menu_item", &gsi::PluginFactoryBase::add_config_menu_item, gsi::arg ("menu_name"), gsi::arg ("insert_pos"), gsi::arg ("title"), gsi::arg ("cname"), gsi::arg ("cvalue"),
    "@brief Adds a configuration menu item\n"
    "\n"
    "Menu items created this way will send a configuration request with 'cname' as the configuration parameter name "
    "and 'cvalue' as the configuration parameter value.\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  method ("add_option", &gsi::PluginFactoryBase::add_option, gsi::arg ("name"), gsi::arg ("default_value"),
    "@brief Specifies configuration variables.\n"
    "Call this method in the factory constructor to add configuration key/value pairs to the configuration repository. "
    "Without specifying configuration variables, the status of a plugin cannot be persisted. "
    "\n\n"
    "Once the configuration variables are known, they can be retrieved on demand using \"get_config\" from "
    "\\MainWindow or listening to \\configure callbacks (either in the factory or the plugin instance). Configuration variables can "
    "be set using \"set_config\" from \\MainWindow. This scheme also works without registering the configuration options, but "
    "doing so has the advantage that it is guaranteed that a variable with this keys exists and has the given default value initially."
    "\n\n"
  ) +
  method ("has_tool_entry=", &gsi::PluginFactoryBase::has_tool_entry, gsi::arg ("f"),
    "@brief Enables or disables the tool bar entry\n"
    "Initially this property is set to true. This means that the plugin will have a visible entry in the toolbar. "
    "This property can be set to false to disable this feature. In that case, the title and icon given on registration will be ignored. "
  ),
  "@brief The plugin framework's plugin factory object\n"
  "\n"
  "Plugins are components that extend KLayout's functionality in various aspects. Scripting support exists "
  "currently for providing mouse mode handlers and general on-demand functionality connected with a menu "
  "entry.\n"
  "\n"
  "Plugins are objects that implement the \\Plugin interface. Each layout view is associated with one instance "
  "of such an object. The PluginFactory is a singleton which is responsible for creating \\Plugin objects and "
  "providing certain configuration information such as where to put the menu items connected to this plugin and "
  "what configuration keys are used.\n"
  "\n"
  "An implementation of PluginFactory must at least provide an implementation of \\create_plugin. This method "
  "must instantiate a new object of the specific plugin.\n"
  "\n"
  "After the factory has been created, it must be registered in the system using one of the \\register methods. "
  "It is therefore recommended to put the call to \\register at the end of the \"initialize\" method. For the registration "
  "to work properly, the menu items must be defined before \\register is called.\n"
  "\n"
  "The following features can also be implemented:\n"
  "\n"
  "@<ul>\n"
  "  @<li>Reserve keys in the configuration file using \\add_option in the constructor@</li>\n"
  "  @<li>Create menu items by using \\add_menu_entry in the constructor@</li>\n"
  "  @<li>Set the title for the mode entry that appears in the tool bar using the \\register argument@</li>\n"
  "  @<li>Provide global functionality (independent from the layout view) using \\configure or \\menu_activated@</li>\n"
  "@</ul>\n"
  "\n"
  "This is a simple example for a plugin in Ruby. It switches the mouse cursor to a 'cross' cursor when it is active:\n"
  "\n"
  "@code\n"
  "class PluginTestFactory < RBA::PluginFactory\n"
  "\n"
  "  # Constructor\n"
  "  def initialize\n"
  "    # registers the new plugin class at position 100000 (at the end), with name\n"
  "    # \"my_plugin_test\" and title \"My plugin test\"\n"
  "    register(100000, \"my_plugin_test\", \"My plugin test\")\n"
  "  end\n"
  "  \n"
  "  # Create a new plugin instance of the custom type\n"
  "  def create_plugin(manager, dispatcher, view)\n"
  "    return PluginTest.new\n"
  "  end\n"
  "\n"
  "end\n"
  "\n"
  "# The plugin class\n"
  "class PluginTest < RBA::Plugin\n"
  "  def mouse_moved_event(p, buttons, prio)\n"
  "    if prio\n"
  "      # Set the cursor to cross if our plugin is active.\n"
  "      set_cursor(RBA::Cursor::Cross)\n"
  "    end\n"
  "    # Returning false indicates that we don't want to consume the event.\n"
  "    # This way for example the cursor position tracker still works.\n"
  "    false\n"
  "  end\n"
  "  def mouse_click_event(p, buttons, prio)\n"
  "    if prio\n"
  "      puts \"mouse button clicked.\"\n"
  "      # This indicates we want to consume the event and others don't receive the mouse click\n"
  "      # with prio = false.\n"
  "      return true\n"
  "    end\n"
  "    # don't consume the event if we are not active.\n"
  "    false\n"
  "  end\n"
  "end\n"
  "\n"
  "# Instantiate the new plugin factory.\n"
  "PluginTestFactory.new\n"
  "@/code\n"
  "\n"
  "This class has been introduced in version 0.22.\n"
);

Class<gsi::PluginBase> decl_Plugin ("lay", "Plugin",
  callback ("menu_activated", &gsi::PluginBase::menu_activated, &gsi::PluginBase::f_menu_activated, gsi::arg ("symbol"),
    "@brief Gets called when a custom menu item is selected\n"
    "When a menu item is clicked which was registered with the plugin factory, the plugin's 'menu_activated' method is "
    "called for the current view. The symbol registered for the menu item is passed in the 'symbol' argument."
  ) +
  callback ("configure", &gsi::PluginBase::configure, &gsi::PluginBase::f_configure, gsi::arg ("name"), gsi::arg ("value"),
    "@brief Sends configuration requests to the plugin\n"
    "@param name The name of the configuration variable as registered in the plugin factory\n"
    "@param value The value of the configuration variable\n"
    "When a configuration variable is changed, the new value is reported to the plugin by calling the 'configure' method."
  ) +
  callback ("config_finalize", &gsi::PluginBase::config_finalize, &gsi::PluginBase::f_config_finalize,
    "@brief Sends the post-configuration request to the plugin\n"
    "After all configuration parameters have been sent, 'config_finalize' is called to given the plugin a chance to "
    "update its internal state according to the new configuration.\n"
  ) +
  callback ("key_event", &gsi::PluginBase::key_event, &gsi::PluginBase::f_key_event, gsi::arg ("key"), gsi::arg ("buttons"),
    "@brief Handles the key pressed event\n"
    "This method will called by the view on the active plugin when a button is pressed on the mouse.\n"
    "\n"
    "If the plugin handles the event, it should return true to indicate that the event should not be processed further."
    "\n"
    "@param key The Qt key code of the key that was pressed\n"
    "@param buttons A combination of the constants in the \\ButtonState class which codes both the mouse buttons and the key modifiers (.e. ShiftButton etc).\n"
    "@return True to terminate dispatcher\n"
  ) +
  callback ("mouse_button_pressed_event", &gsi::PluginBase::mouse_press_event_noref, &gsi::PluginBase::f_mouse_press_event, gsi::arg ("p"), gsi::arg ("buttons"), gsi::arg ("prio"),
    "@brief Handles the mouse button pressed event\n"
    "This method will called by the view when a button is pressed on the mouse.\n"
    "\n"
    "First, the plugins that grabbed the mouse with \\grab_mouse will receive this event with 'prio' set to true "
    "in the reverse order the plugins grabbed the mouse. The loop will terminate if one of the mouse event handlers "
    "returns true.\n"
    "\n"
    "If that is not the case or no plugin has grabbed the mouse, the active plugin receives the mouse event with 'prio' set to true.\n"
    "\n"
    "If no receiver accepted the mouse event by returning true, it is sent again to all plugins with 'prio' set to false.\n"
    "Again, the loop terminates if one of the receivers returns true. The second pass gives inactive plugins a chance to monitor the mouse "
    "and implement specific actions - i.e. displaying the current position.\n"
    "\n"
    "This event is not sent immediately when the mouse button is pressed but when a signification movement for the mouse cursor away from the "
    "original position is detected. If the mouse button is released before that, a mouse_clicked_event is sent rather than a press-move-release "
    "sequence."
    "\n"
    "@param p The point at which the button was pressed\n"
    "@param buttons A combination of the constants in the \\ButtonState class which codes both the mouse buttons and the key modifiers (.e. LeftButton, ShiftButton etc).\n"
    "@return True to terminate dispatcher\n"
  ) +
  callback ("mouse_click_event", &gsi::PluginBase::mouse_click_event_noref, &gsi::PluginBase::f_mouse_click_event, gsi::arg ("p"), gsi::arg ("buttons"), gsi::arg ("prio"),
    "@brief Handles the mouse button click event (after the button has been released)\n"
    "The behaviour of this callback is the same than for \\mouse_press_event, except that it is called when the mouse button has been released without moving it.\n"
  ) +
  callback ("mouse_double_click_event", &gsi::PluginBase::mouse_double_click_event_noref, &gsi::PluginBase::f_mouse_double_click_event, gsi::arg ("p"), gsi::arg ("buttons"), gsi::arg ("prio"),
    "@brief Handles the mouse button double-click event\n"
    "The behaviour of this callback is the same than for \\mouse_press_event, except that it is called when the mouse button has been double-clicked.\n"
  ) +
  callback ("leave_event", &gsi::PluginBase::leave_event, &gsi::PluginBase::f_leave_event, gsi::arg ("prio"),
    "@brief Handles the leave event (mouse leaves canvas area of view)\n"
    "The behaviour of this callback is the same than for \\mouse_press_event, except that it is called when the mouse leaves the canvas area.\n"
    "This method does not have a position nor button flags.\n"
  ) +
  callback ("enter_event", &gsi::PluginBase::enter_event, &gsi::PluginBase::f_enter_event, gsi::arg ("prio"),
    "@brief Handles the enter event (mouse enters canvas area of view)\n"
    "The behaviour of this callback is the same than for \\mouse_press_event, except that it is called when the mouse enters the canvas area.\n"
    "This method does not have a position nor button flags.\n"
  ) +
  callback ("mouse_moved_event", &gsi::PluginBase::mouse_move_event_noref, &gsi::PluginBase::f_mouse_move_event, gsi::arg ("p"), gsi::arg ("buttons"), gsi::arg ("prio"),
    "@brief Handles the mouse move event\n"
    "The behaviour of this callback is the same than for \\mouse_press_event, except that it is called when the mouse is moved in the canvas area.\n"
  ) +
  callback ("mouse_button_released_event", &gsi::PluginBase::mouse_release_event_noref, &gsi::PluginBase::f_mouse_release_event, gsi::arg ("p"), gsi::arg ("buttons"), gsi::arg ("prio"),
    "@brief Handles the mouse button release event\n"
    "The behaviour of this callback is the same than for \\mouse_press_event, except that it is called when the mouse button is released.\n"
  ) +
  callback ("wheel_event", &gsi::PluginBase::wheel_event_noref, &gsi::PluginBase::f_wheel_event, gsi::arg ("delta"), gsi::arg ("horizontal"), gsi::arg ("p"), gsi::arg ("buttons"), gsi::arg ("prio"),
    "The behaviour of this callback is the same than for \\mouse_press_event, except that it is called when the mouse wheel is rotated.\n"
    "Additional parameters for this event are 'delta' (the rotation angle in units of 1/8th degree) and 'horizontal' which is true when the horizontal wheel was rotated and "
    "false if the vertical wheel was rotated.\n"
  ) +
  callback ("activated", &gsi::PluginBase::activated, &gsi::PluginBase::f_activated,
    "@brief Gets called when the plugin is activated (selected in the tool bar)\n"
  ) +
  callback ("deactivated", &gsi::PluginBase::deactivated, &gsi::PluginBase::f_deactivated,
    "@brief Gets called when the plugin is deactivated and another plugin is activated\n"
  ) +
  callback ("drag_cancel", &gsi::PluginBase::drag_cancel, &gsi::PluginBase::f_drag_cancel,
    "@brief Gets called on various occasions when a drag operation should be canceled\n"
    "If the plugin implements some press-and-drag or a click-and-drag operation, this callback should "
    "cancel this operation and return in some state waiting for a new mouse event."
  ) +
  callback ("update", &gsi::PluginBase::update, &gsi::PluginBase::f_update,
    "@brief Gets called when the view has changed\n"
    "This method is called in particular if the view has changed the visible rectangle, i.e. after zooming in or out or panning. "
    "This callback can be used to update any internal states that depend on the view's state."
  ) + 
  method ("grab_mouse", &gsi::PluginBase::grab_mouse,
    "@brief Redirects mouse events to this plugin, even if the plugin is not active.\n"
  ) + 
  method ("ungrab_mouse", &gsi::PluginBase::ungrab_mouse,
    "@brief Removes a mouse grab registered with \\grab_mouse.\n"
  ) + 
  method ("set_cursor", &gsi::PluginBase::set_cursor, gsi::arg ("cursor_type"),
    "@brief Sets the cursor in the view area to the given type\n"
    "Setting the cursor has an effect only inside event handlers, i.e. mouse_press_event. The cursor is not set permanently. Is is reset "
    "in the mouse move handler unless a button is pressed or the cursor is explicitly set again in the mouse_move_event.\n"
    "\n"
    "The cursor type is one of the cursor constants in the \\Cursor class, i.e. 'CursorArrow' for the normal cursor."
  ) +
  callback ("has_tracking_position", &gsi::PluginBase::has_tracking_position, &gsi::PluginBase::f_has_tracking_position,
    "@brief Gets a value indicating whether the plugin provides a tracking position\n"
    "The tracking position is shown in the lower-left corner of the layout window to indicate the current position.\n"
    "If this method returns true for the active service, the application will fetch the position by calling \\tracking_position "
    "rather than displaying the original mouse position.\n"
    "\n"
    "This method has been added in version 0.27.6."
  ) +
  callback ("tracking_position", &gsi::PluginBase::tracking_position, &gsi::PluginBase::f_tracking_position,
    "@brief Gets the tracking position\n"
    "See \\has_tracking_position for details.\n"
    "\n"
    "This method has been added in version 0.27.6."
  ),
  "@brief The plugin object\n"
  "\n"
  "This class provides the actual plugin implementation. Each view gets its own instance of the plugin class. The plugin factory \\PluginFactory class "
  "must be specialized to provide a factory for new objects of the Plugin class. See the documentation there for details about the plugin mechanism and "
  "the basic concepts.\n"
  "\n"
  "This class has been introduced in version 0.22.\n"
);

class CursorNamespace { };

static int cursor_shape_none () { return int (lay::Cursor::none); }
static int cursor_shape_arrow () { return int (lay::Cursor::arrow); }
static int cursor_shape_up_arrow () { return int (lay::Cursor::up_arrow); }
static int cursor_shape_cross () { return int (lay::Cursor::cross); }
static int cursor_shape_wait () { return int (lay::Cursor::wait); }
static int cursor_shape_i_beam () { return int (lay::Cursor::i_beam); }
static int cursor_shape_size_ver () { return int (lay::Cursor::size_ver); }
static int cursor_shape_size_hor () { return int (lay::Cursor::size_hor); }
static int cursor_shape_size_bdiag () { return int (lay::Cursor::size_bdiag); }
static int cursor_shape_size_fdiag () { return int (lay::Cursor::size_fdiag); }
static int cursor_shape_size_all () { return int (lay::Cursor::size_all); }
static int cursor_shape_blank () { return int (lay::Cursor::blank); }
static int cursor_shape_split_v () { return int (lay::Cursor::split_v); }
static int cursor_shape_split_h () { return int (lay::Cursor::split_h); }
static int cursor_shape_pointing_hand () { return int (lay::Cursor::pointing_hand); }
static int cursor_shape_forbidden () { return int (lay::Cursor::forbidden); }
static int cursor_shape_whats_this () { return int (lay::Cursor::whats_this); }
static int cursor_shape_busy () { return int (lay::Cursor::busy); }
static int cursor_shape_open_hand () { return int (lay::Cursor::open_hand); }
static int cursor_shape_closed_hand () { return int (lay::Cursor::closed_hand); }

Class<gsi::CursorNamespace> decl_Cursor ("lay", "Cursor",
  method ("None", &cursor_shape_none, "@brief 'No cursor (default)' constant for \\set_cursor (resets cursor to default)") +
  method ("Arrow", &cursor_shape_arrow, "@brief 'Arrow cursor' constant") +
  method ("UpArrow", &cursor_shape_up_arrow, "@brief 'Upward arrow cursor' constant") +
  method ("Cross", &cursor_shape_cross, "@brief 'Cross cursor' constant") +
  method ("Wait", &cursor_shape_wait, "@brief 'Waiting cursor' constant") +
  method ("IBeam", &cursor_shape_i_beam, "@brief 'I beam (text insert) cursor' constant") +
  method ("SizeVer", &cursor_shape_size_ver, "@brief 'Vertical resize cursor' constant") +
  method ("SizeHor", &cursor_shape_size_hor, "@brief 'Horizontal resize cursor' constant") +
  method ("SizeBDiag", &cursor_shape_size_bdiag, "@brief 'Backward diagonal resize cursor' constant") +
  method ("SizeFDiag", &cursor_shape_size_fdiag, "@brief 'Forward diagonal resize cursor' constant") +
  method ("SizeAll", &cursor_shape_size_all, "@brief 'Size all directions cursor' constant") +
  method ("Blank", &cursor_shape_blank, "@brief 'Blank cursor' constant") +
  method ("SplitV", &cursor_shape_split_v, "@brief 'Split vertical cursor' constant") +
  method ("SplitH", &cursor_shape_split_h, "@brief 'split_horizontal cursor' constant") +
  method ("PointingHand", &cursor_shape_pointing_hand, "@brief 'Pointing hand cursor' constant") +
  method ("Forbidden", &cursor_shape_forbidden, "@brief 'Forbidden area cursor' constant") +
  method ("WhatsThis", &cursor_shape_whats_this, "@brief 'Question mark cursor' constant") +
  method ("Busy", &cursor_shape_busy, "@brief 'Busy state cursor' constant") +
  method ("OpenHand", &cursor_shape_open_hand, "@brief 'Open hand cursor' constant") +
  method ("ClosedHand", &cursor_shape_closed_hand, "@brief 'Closed hand cursor' constant"),
  "@brief The namespace for the cursor constants\n"
  "This class defines the constants for the cursor setting (for example for class \\Plugin, method set_cursor)."
  "\n"
  "This class has been introduced in version 0.22.\n"
);

class ButtonStateNamespace { };

static int const_ShiftButton()      { return (int) lay::ShiftButton; }
static int const_ControlButton()    { return (int) lay::ControlButton; }
static int const_AltButton()        { return (int) lay::AltButton; }
static int const_LeftButton()       { return (int) lay::LeftButton; }
static int const_MidButton()        { return (int) lay::MidButton; }
static int const_RightButton()      { return (int) lay::RightButton; }

Class<gsi::ButtonStateNamespace> decl_ButtonState ("lay", "ButtonState",
  method ("ShiftKey", &const_ShiftButton, "@brief Indicates that the Shift key is pressed\nThis constant is combined with other constants within \\ButtonState") +
  method ("ControlKey", &const_ControlButton, "@brief Indicates that the Control key is pressed\nThis constant is combined with other constants within \\ButtonState") +
  method ("AltKey", &const_AltButton, "@brief Indicates that the Alt key is pressed\nThis constant is combined with other constants within \\ButtonState") +
  method ("LeftButton", &const_LeftButton, "@brief Indicates that the left mouse button is pressed\nThis constant is combined with other constants within \\ButtonState") +
  method ("MidButton", &const_MidButton, "@brief Indicates that the middle mouse button is pressed\nThis constant is combined with other constants within \\ButtonState") +
  method ("RightButton", &const_RightButton, "@brief Indicates that the right mouse button is pressed\nThis constant is combined with other constants within \\ButtonState"),
  "@brief The namespace for the button state flags in the mouse events of the Plugin class.\n"
  "This class defines the constants for the button state. In the event handler, the button state is "
  "indicated by a bitwise combination of these constants. See \\Plugin for further details."
  "\n"
  "This class has been introduced in version 0.22.\n"
);

class KeyCodesNamespace { };

static int const_KeyEscape()        { return (int) lay::KeyEscape; }
static int const_KeyTab()           { return (int) lay::KeyTab; }
static int const_KeyBacktab()       { return (int) lay::KeyBacktab; }
static int const_KeyBackspace()     { return (int) lay::KeyBackspace; }
static int const_KeyReturn()        { return (int) lay::KeyReturn; }
static int const_KeyEnter()         { return (int) lay::KeyEnter; }
static int const_KeyInsert()        { return (int) lay::KeyInsert; }
static int const_KeyDelete()        { return (int) lay::KeyDelete; }
static int const_KeyHome()          { return (int) lay::KeyHome; }
static int const_KeyEnd()           { return (int) lay::KeyEnd; }
static int const_KeyDown()          { return (int) lay::KeyDown; }
static int const_KeyUp()            { return (int) lay::KeyUp; }
static int const_KeyLeft()          { return (int) lay::KeyLeft; }
static int const_KeyRight()         { return (int) lay::KeyRight; }
static int const_KeyPageUp()        { return (int) lay::KeyPageUp; }
static int const_KeyPageDown()      { return (int) lay::KeyPageDown; }

Class<gsi::KeyCodesNamespace> decl_KeyCode ("lay", "KeyCode",
  method ("Escape", &const_KeyEscape, "@brief Indicates the Escape key") +
  method ("Tab", &const_KeyTab, "@brief Indicates the Tab key") +
  method ("Backtab", &const_KeyBacktab, "@brief Indicates the Backtab key") +
  method ("Backspace", &const_KeyBackspace, "@brief Indicates the Backspace key") +
  method ("Return", &const_KeyReturn, "@brief Indicates the Return key") +
  method ("Enter", &const_KeyEnter, "@brief Indicates the Enter key") +
  method ("Insert", &const_KeyInsert, "@brief Indicates the Insert key") +
  method ("Delete", &const_KeyDelete, "@brief Indicates the Delete key") +
  method ("Home", &const_KeyHome, "@brief Indicates the Home key") +
  method ("End", &const_KeyEnd, "@brief Indicates the End key") +
  method ("Down", &const_KeyDown, "@brief Indicates the Down key") +
  method ("Up", &const_KeyUp, "@brief Indicates the Up key") +
  method ("Left", &const_KeyLeft, "@brief Indicates the Left key") +
  method ("Right", &const_KeyRight, "@brief Indicates the Right key") +
  method ("PageUp", &const_KeyPageUp, "@brief Indicates the PageUp key") +
  method ("PageDown", &const_KeyPageDown, "@brief Indicates the PageDown key"),
  "@brief The namespace for the some key codes.\n"
  "This namespace defines some key codes understood by built-in \\LayoutView components. "
  "When compiling with Qt, these codes are compatible with Qt's key codes.\n"
  "The key codes are intended to be used when directly interfacing with \\LayoutView in non-Qt-based environments.\n"
  "\n"
  "This class has been introduced in version 0.28.\n"
);

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

static tl::Variant get_config (lay::Dispatcher *dispatcher, const std::string &name)
{
  std::string value;
  if (dispatcher->config_get (name, value)) {
    return tl::Variant (value);
  } else {
    return tl::Variant ();
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
  method_ext ("get_config", &get_config, gsi::arg ("name"),
    "@brief Gets the value of a local configuration parameter\n"
    "\n"
    "@param name The name of the configuration parameter whose value shall be obtained (a string)\n"
    "\n"
    "@return The value of the parameter or nil if there is no such parameter\n"
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
