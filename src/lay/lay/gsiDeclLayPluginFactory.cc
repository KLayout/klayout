
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
#include "gsiEnums.h"

#include "gsiDeclLayEditorOptionsPage.h"
#include "gsiDeclLayConfigPage.h"
#include "gsiDeclLayPlugin.h"

#include "layEditorOptionsPages.h"

namespace gsi
{

class PluginFactoryBase;

static std::map <std::string, PluginFactoryBase *> s_factories;
extern bool s_in_create_plugin;

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
    if (f != s_factories.end () && f->second != this) {
      //  NOTE: this also removes the plugin from the s_factories list
      delete f->second;
    }
    s_factories[name] = this;

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

#if defined(HAVE_QTBINDINGS)
  void add_editor_options_page (EditorOptionsPageImpl *page) const
  {
    page->keep ();
    m_editor_options_pages.push_back (page);
  }

  void get_editor_options_pages_impl () const
  {
    //  .. nothing here ..
  }

  virtual void get_editor_options_pages (std::vector<lay::EditorOptionsPage *> &pages_out, lay::LayoutViewBase *view, lay::Dispatcher *dispatcher) const
  {
    try {

      m_editor_options_pages.clear ();

      if (f_get_editor_options_pages.can_issue ()) {
        f_get_editor_options_pages.issue<PluginFactoryBase> (&PluginFactoryBase::get_editor_options_pages_impl);
      } else {
        get_editor_options_pages_impl ();
      }

      for (auto i = m_editor_options_pages.begin (); i != m_editor_options_pages.end (); ++i) {
        if (*i) {
          (*i)->init (view, dispatcher);
          (*i)->set_plugin_declaration (this);
          pages_out.push_back (*i);
        }
      }

      m_editor_options_pages.clear ();

    } catch (tl::Exception &ex) {
      tl::error << ex.msg ();
    } catch (std::exception &ex) {
      tl::error << ex.what ();
    } catch (...) {
    }
  }

  void add_config_page (ConfigPageImpl *page) const
  {
    page->keep ();
    m_config_pages.push_back (page);
  }

  void get_config_pages_impl () const
  {
    //  .. nothing here ..
  }

  virtual std::vector<std::pair <std::string, lay::ConfigPage *> > config_pages (QWidget *parent) const
  {
    std::vector<std::pair <std::string, lay::ConfigPage *> > pages_out;

    try {

      m_config_pages.clear ();

      if (f_config_pages.can_issue ()) {
        f_config_pages.issue<PluginFactoryBase> (&PluginFactoryBase::get_config_pages_impl);
      } else {
        get_config_pages_impl ();
      }

      pages_out.clear ();
      for (auto i = m_config_pages.begin (); i != m_config_pages.end (); ++i) {
        if (*i) {
          (*i)->setParent (parent);
          pages_out.push_back (std::make_pair ((*i)->title (), *i));
        }
      }

      m_config_pages.clear ();

    } catch (tl::Exception &ex) {
      tl::error << ex.msg ();
    } catch (std::exception &ex) {
      tl::error << ex.what ();
    } catch (...) {
    }

    return pages_out;
  }
#endif

  virtual lay::Plugin *create_plugin (db::Manager *manager, lay::Dispatcher *root, lay::LayoutViewBase *view) const
  {
    if (f_create_plugin.can_issue ()) {
      return create_plugin_gsi (manager, root, view);
    } else {
      return lay::PluginDeclaration::create_plugin (manager, root, view);
    }
  }

  virtual gsi::PluginImpl *create_plugin_gsi (db::Manager *manager, lay::Dispatcher *root, lay::LayoutViewBase *view) const
  { 
    s_in_create_plugin = true;

    gsi::PluginImpl *ret = 0;
    try {

      ret = f_create_plugin.issue<PluginFactoryBase, gsi::PluginImpl *, db::Manager *, lay::Dispatcher *, lay::LayoutViewBase *> (&PluginFactoryBase::create_plugin_gsi, manager, root, view);
      if (ret) {
        ret->init (view, root);
      }

    } catch (tl::Exception &ex) {
      tl::error << ex.msg ();
    } catch (std::exception &ex) {
      tl::error << ex.what ();
    } catch (...) {
    }

    s_in_create_plugin = false;

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
  gsi::Callback f_get_editor_options_pages;
  gsi::Callback f_config_pages;

private:
  std::vector<std::pair<std::string, std::string> > m_options;
  std::vector<lay::MenuEntry> m_menu_entries;
  bool m_implements_mouse_mode;
  std::string m_mouse_mode_title;
  tl::RegisteredClass <lay::PluginDeclaration> *mp_registration;
#if defined(HAVE_QTBINDINGS)
  mutable std::vector<ConfigPageImpl *> m_config_pages;
  mutable std::vector<EditorOptionsPageImpl *> m_editor_options_pages;
#endif
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
    "If 'cvalue' is a string with a single question mark (\"?\"), the item is a check box that reflects the boolean "
    "value of the configuration item.\n"
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
  ) +
#if defined(HAVE_QTBINDINGS)
  method ("add_editor_options_page", &PluginFactoryBase::add_editor_options_page, gsi::arg ("page"),
    "@brief Adds the given editor options page\n"
    "See \\create_editor_options_pages how to use this function. The method is effective only in "
    "the reimplementation context of this function.\n"
    "\n"
    "This method has been introduced in version 0.30.4."
  ) +
  callback ("create_editor_options_pages", &PluginFactoryBase::get_editor_options_pages_impl, &PluginFactoryBase::f_get_editor_options_pages,
    "@brief Creates the editor option pages\n"
    "The editor option pages are widgets of type \\EditorOptionsPage. These Qt widgets "
    "are displayed in a seperate dock (the 'editor options') and become visible when the plugin is active - i.e. "
    "its mode is selected. Use this method to provide customized pages that will be displayed in the "
    "editor options dock.\n"
    "\n"
    "In order to create config pages, instantiate a \\EditorOptionsPage object and "
    "call \\add_editor_options_page to register it.\n"
    "\n"
    "This method has been introduced in version 0.30.4."
  ) +
  method ("add_config_page", &PluginFactoryBase::add_config_page, gsi::arg ("page"),
    "@brief Adds the given configuration page\n"
    "See \\create_config_pages how to use this function. The method is effective only in "
    "the reimplementation context of this function.\n"
    "\n"
    "This method has been introduced in version 0.30.4."
  ) +
  callback ("create_config_pages", &PluginFactoryBase::get_config_pages_impl, &PluginFactoryBase::f_config_pages,
    "@brief Creates the configuration widgets\n"
    "The configuration pages are widgets that are displayed in the "
    "configuration dialog ('File/Setup'). Every plugin can create multiple such "
    "widgets and specify, where these widgets are displayed. The widgets are of type \\ConfigPage.\n"
    "\n"
    "The title string also specifies the location of the widget in the "
    "configuration page hierarchy. See \\ConfigPage for more details.\n"
    "\n"
    "In order to create config pages, instantiate a \\ConfigPage object and "
    "call \\add_config_page to register it.\n"
    "\n"
    "This method has been introduced in version 0.30.4."
  ) +
#endif
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

}
