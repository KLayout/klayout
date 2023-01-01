
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


#ifndef HDR_layPlugin
#define HDR_layPlugin

#include "laybasicCommon.h"

#include "tlString.h"
#include "tlClassRegistry.h"
#include "tlDeferredExecution.h"
#include "gsiObject.h"
#include "layAbstractMenu.h"

#include <map>
#include <vector>
#include <string>

namespace db
{
  class Manager;
}

namespace lay
{

class Plugin;
class Dispatcher;
class LayoutViewBase;
class ViewService;
class Editable;
class Drawing;
class TechnologyComponentProvider;
#if defined(HAVE_QT)
class Browser;
class EditorOptionsPage;
class ConfigPage;
#endif

/**
 *  @brief A menu entry declaration
 *  See separator, menu_item, submenu and config_menu_item below.
 */
struct LAYBASIC_PUBLIC MenuEntry
{
  MenuEntry () : sub_menu (false), checkable (false), separator (false) { }

  std::string menu_name;
  std::string symbol;
  std::string insert_pos;
  std::string title;
  std::string copy_from;
  std::string cname;
  std::string cvalue;
  std::string exclusive_group;
  bool sub_menu;
  bool checkable;
  bool separator;
};

/**
 *  @brief Creates a separator menu entry
 *
 *  @param menu_name The name of the menu item (see layAbstractMenu.h)
 *  @param insert_pos The position where to insert (see layAbstractMenu.h)
 */
LAYBASIC_PUBLIC MenuEntry separator (const std::string &menu_name, const std::string &insert_pos);

/**
 *  @brief Creates a normal menu entry
 *
 *  @param symbol The symbol to send when this menu item is selected
 *  @param menu_name The name of the menu item (see layAbstractMenu.h)
 *  @param insert_pos The position where to insert (see layAbstractMenu.h)
 *  @param title The title to display plus optional icon resource and keyboard shortcut. The format of the string is: <text>["("shortcut")"]["<"icon-resource">"][{"tool-tip"}].
 */
LAYBASIC_PUBLIC MenuEntry menu_item (const std::string &symbol, const std::string &menu_name, const std::string &insert_pos, const std::string &title);

/**
 *  @brief Creates a menu entry as a linked copy from another one
 *
 *  @param symbol The symbol to send when this menu item is selected
 *  @param menu_name The name of the menu item (see layAbstractMenu.h)
 *  @param insert_pos The position where to insert (see layAbstractMenu.h)
 *  @param copy_from The path of the item where to copy from (must exist)
 */
LAYBASIC_PUBLIC MenuEntry menu_item_copy (const std::string &symbol, const std::string &menu_name, const std::string &insert_pos, const std::string &copy_from);

/**
 *  @brief Creates a submenu entry
 *
 *  @param menu_name The name of the menu item (see layAbstractMenu.h)
 *  @param insert_pos The position where to insert (see layAbstractMenu.h)
 *  @param title The title to display plus optional icon resource and keyboard shortcut. The format of the string is: <text>["("shortcut")"]["<"icon-resource">"][{"tool-tip"}].
 */
LAYBASIC_PUBLIC MenuEntry submenu (const std::string &menu_name, const std::string &insert_pos, const std::string &title);

/**
 *  @brief Creates a submenu entry
 *
 *  @param symbol The symbol to send when this menu item is selected
 *  @param menu_name The name of the menu item (see layAbstractMenu.h)
 *  @param insert_pos The position where to insert (see layAbstractMenu.h)
 *  @param title The title to display plus optional icon resource and keyboard shortcut. The format of the string is: <text>["("shortcut")"]["<"icon-resource">"][{"tool-tip"}].
 */
LAYBASIC_PUBLIC MenuEntry submenu (const std::string &symbol, const std::string &menu_name, const std::string &insert_pos, const std::string &title);

/**
 *  @brief Creates a configuration entry
 *
 *  @param menu_name The name of the menu item (see layAbstractMenu.h)
 *  @param insert_pos The position where to insert (see layAbstractMenu.h)
 *  @param title The title to display plus optional icon resource and keyboard shortcut. The format of the string is: <text>["("shortcut")"]["<"icon-resource">"][{"tool-tip"}].
 *  @param cname The name of the configuration item
 *  @param cvalue The value to set for the configuration item (optional for boolean configuration items)
 */
LAYBASIC_PUBLIC MenuEntry config_menu_item (const std::string &menu_name, const std::string &insert_pos, const std::string &title, const std::string &cname, const std::string &cvalue = std::string ());

/**
 *  @brief The configuration declaration
 *
 *  For each configurable class a declaration object derived from
 *  this class must be provided through the tl::Registrar registration
 *  mechanism (instantiate a tl::Registrar<tl::Plugin>::Class<Y>
 *  object).
 */
class LAYBASIC_PUBLIC PluginDeclaration :
#if defined(HAVE_QT)
    public QObject,
#endif
    public gsi::ObjectBase
{
#if defined(HAVE_QT)
Q_OBJECT
#endif

public:
  /** 
   *  @brief Constructor
   */
  PluginDeclaration ();
  
  /**
   *  @brief Destructor
   */
  virtual ~PluginDeclaration ();

  /**
   *  @brief This method is supposed to deliver the option names available
   *  
   *  @param options A vector of names and default value strings.
   */
  virtual void get_options (std::vector < std::pair<std::string, std::string> > & /*options*/) const
  {
    //  the default implementation does not add any options
  }

#if defined(HAVE_QT)
  /**
   *  @brief Fetch the configuration page for the configuration dialog
   *
   *  @param title The title to display in the corresponding tab of
   *               the configuration dialog.
   *  @return The configuration page or 0 if there is no page.
   */
  virtual ConfigPage *config_page (QWidget * /*parent*/, std::string & /*title*/) const
  {
    return 0;
  }

  /**
   *  @brief Fetch the configuration pages for the configuration dialog
   *
   *  @return A list of titles and ConfigPage instances
   *
   *  This method can be reimplemented alternatively to "config_page" when multiple pages shall be
   *  used. The pages are inserted in the order they are defined. 
   */
  virtual std::vector<std::pair <std::string, ConfigPage *> > config_pages (QWidget * /*parent*/) const
  {
    return std::vector<std::pair <std::string, ConfigPage *> > ();
  }
#endif

  /**
   *  @brief The global configuration 
   *
   *  By implementing this method, a configuration can be treated globally.
   *  Otherwise the individual plugin object (associated with a view) must handle
   *  the configuration. One example for handling the configuration globally is
   *  dynamic menu configuration.
   */
  virtual bool configure (const std::string & /*name*/, const std::string & /*value*/)
  {
    return false;
  }

  /**
   *  @brief Global menu handler
   *
   *  This menu handler is called before the view specific handler is 
   *  called (lay::Plugin::menu_activated).
   *  The implementation is supposed to return true if it handles the menu entry.
   */
  virtual bool menu_activated (const std::string & /*symbol*/) const
  {
    //  The default implementation does nothing.
    return false;
  }

  /**
   *  @brief Configuration finalization
   *
   *  This method is called after all configuration changes have been applied.
   *  A derived class might implement this method in order to do some post-
   *  processing of the configuration.
   */
  virtual void config_finalize ()
  {
    //  .. the default implementation does nothing ..
  }

  /**
   *  @brief Basic initialization
   *
   *  Reimplementation of this method offers a chance to initialize static resources such as 
   *  dialogs etc.
   */
  virtual void initialize (lay::Dispatcher * /*dispatcher*/)
  {
    //  .. the default implementation does nothing ..
  }

  /**
   *  @brief Basic initialization
   *
   *  Reimplementation of this method offers a chance to initialize static resources such as 
   *  dialogs etc.
   *  While initialize is called before any configuration is loaded, "initialized" will be
   *  called after the plugin system has been initially configured.
   */
  virtual void initialized (lay::Dispatcher * /*dispatcher*/)
  {
    //  .. the default implementation does nothing ..
  }

  /**
   *  @brief Uninitialize the plugin
   */
  virtual void uninitialize (lay::Dispatcher * /*dispatcher*/)
  {
    //  .. the default implementation does nothing ..
  }

  /**
   *  @brief Gets a value indicating whether the plugin can exit
   *
   *  If the plugin wants to prevent the application from closing, it may return false
   *  in this method.
   */
  virtual bool can_exit (lay::Dispatcher * /*dispatcher*/) const
  {
    return true;
  }

  /**
   *  @brief Fetches the menu objects for this plugin
   *
   *  The implementation of this method is supposed to call the base
   *  classes "get_menu_entries" method and add it's own entries.
   */
  virtual void get_menu_entries (std::vector<lay::MenuEntry> & /*menu_entries*/) const
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Creates a plugin object of the respective kind
   *
   *  This method may return 0 for "dummy" plugins that just register menu entries
   *  or configuration options.
   */
  virtual lay::Plugin *create_plugin (db::Manager * /*manager*/, lay::Dispatcher * /*dispatcher*/, lay::LayoutViewBase * /*view*/) const
  {
    return 0;
  }

  /**
   *  @brief Tells if the plugin implements the "lay::Editable" interface
   *
   *  This method is supposed to return true if the plugin implements the 
   *  lay::Editable interface and wants to be listed as an "edit" mode in the
   *  "Edit" menu. The string returned in "title" is displayed in the "Edit" menu's
   *  corresponding mode entry.
   */
  virtual bool implements_editable (std::string & /*title*/) const
  {
    return false;
  }

#if defined(HAVE_QT)
  /**
   *  @brief Gets the editor options pages
   *
   *  Editor options pages are shown in the editor options dockable tool widget. Each plugin can create such pages
   *  and these will be shown in tabs inside this widget.
   *
   *  The new pages are returned in the "pages" vector. The layout view will take ownership of these pages.
   */
  virtual void get_editor_options_pages (std::vector<lay::EditorOptionsPage *> & /*pages*/, lay::LayoutViewBase * /*view*/, lay::Dispatcher * /*dispatcher*/) const
  {
    //  .. no pages in the default implementation ..
  }
#endif

  /**
   *  @brief Tells if the plugin implements a "lay::ViewService" active mouse mode
   *
   *  This method is supposed to return true if the plugin implements the 
   *  lay::ViewService interface and wants to be listed as an mouse mode in the
   *  "Edit" menu. The string returned in "title" is displayed in the "Edit" menu's
   *  corresponding mode entry.
   */
  virtual bool implements_mouse_mode (std::string & /*title*/) const
  {
    return false;
  }

  /**
   *  @brief Specifies the primary mouse modes
   *
   *  These are built-in modes from the LayoutViewBase. This method is intended for
   *  the LayoutViewBase's standard modes only.
   */
  virtual void implements_primary_mouse_modes (std::vector<std::pair<std::string, std::pair<std::string, int> > > & /*modes*/)
  {
    //  .. nothing yet ..
  }
  
  /**
   *  @brief Returns the technology component provider
   *
   *  If this plugin wants to register a technology component, it's declaration must 
   *  include a technology component provider. This is responsible for creating 
   *  a technology component and an associated editor.
   *
   *  @return The provide object (no ownership is taken) or 0 if no technology component is created.
   */
  virtual const TechnologyComponentProvider *technology_component_provider () const 
  {
    return 0;
  }

  /**
   *  @brief Delivers a unique ID
   *
   *  This ID can be used to connect plugins to menus for example.
   *  Currently the ID is just the address of the declaration object.
   *  The ID is guaranteed to be larger than 0, so negative ID's can be 
   *  used for different purposed.
   */
  int id () const
  {
    return m_id;
  }

  /**
   *  @brief Gets the available menu symbols from all plugins
   *
   *  This does not mean all symbols will be available.
   */
  static std::vector<std::string> menu_symbols ();

  /**
   *  @brief Creates the menu resources for this plugin
   *
   *  This method will create the menu resources for the plugin and perform the 
   *  required connect operations. The dispatcher provides the menu and the
   *  event endpoints.
   */
  void init_menu (lay::Dispatcher *dispatcher);

  /**
   *  @brief Removes the menu resources associated with this plugin
   */
  void remove_menu_items (lay::Dispatcher *dispatcher);

  /**
   *  @brief Enables this editable part of the plugin
   *
   *  If the editable part of this plugin is enabled, the objects managed by this mode can be manipulated.
   *  Otherwise, the are not considered for selection.
   *  This property can be monitored with the editable_enabled observer.
   */
  void set_editable_enabled (bool f);

  /**
   *  @brief Gets a value indicating whether the editable part of the plugin is enabled
   */
  bool editable_enabled () const
  {
    return m_editable_enabled;
  }

  /**
   *  @brief Gets a value indicating whether the plugin will accept a dropped file with the given URL or path
   */
  virtual bool accepts_drop (const std::string & /*path_or_url*/) const
  {
    return false;
  }

  /**
   *  @brief Gets called when a file or URL is dropped on the plugin
   */
  virtual void drop_url (const std::string & /*path_or_url*/)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Notifies that dispatcher that a new plugin was registered
   *
   *  This method must be called when a plugin is dynamically created. It is important
   *  that when this method is called, the menu items and other properties are set already. 
   */
  void register_plugin ();

  /**
   *  @brief An event indicating that the editable state has changed
   */
  tl::Event editable_enabled_changed_event;

#if defined(HAVE_QT)
private slots:
  void toggle_editable_enabled ();
#endif

private:
  int m_id;
  tl::weak_ptr<lay::Action> mp_editable_mode_action;
  tl::weak_ptr<lay::Action> mp_mouse_mode_action;
  tl::weak_collection<lay::Action> m_menu_actions;
  bool m_editable_enabled;
};

/**
 *  @brief The plugin interface
 *
 *  This is a basic interface providing several services in a
 *  hierarchically organized fashion. It also provides a configuration
 *  space (key/value pairs).
 *
 *  Each object participating in the plugin scheme must be derived from this interface.
 *  An plugin can have a parent. This way, a hierarchy of plugin objects is created.
 *  The root object not having a parent acts as the main entry point: it will try to dispatch
 *  configuration requests to the children.
 *
 *  Each node has a local configuration space which overrides the configuration changes
 *  made to be parent.
 *
 *  Each plugin also has a static or global configuration space inside the
 *  "plugin declaration". Configuration changes made to top level nodes are
 *  reflected in the static space too.
 *
 *  A "standalone" node is one without a parent, but which does not communicate
 *  with the static configuration space.
 *
 *  The "Dispatcher" adds the concept of a (singleton) root plugin to this hierarchical
 *  configuration tree.
 */

class LAYBASIC_PUBLIC Plugin
  : public gsi::ObjectBase,
    virtual public tl::Object
{
public:
  typedef std::map <std::string, std::string>::const_iterator iterator;

  /**
   *  @brief The constructor
   *  
   *  See above for a explanation of the parent-child relationship.
   */
  Plugin (Plugin *parent, bool standalone = false);

  /**
   *  @brief The destructor
   */
  virtual ~Plugin ();

  /**
   *  @brief Gets a value indicating whether this plugin is a standlone plugin
   *
   *  Standalone-Plugins don't receive common events such as menu requests or
   *  mode changes.
   */
  bool is_standalone () const
  {
    return m_standalone;
  }

  /**
   *  @brief Setup
   *
   *  Calling this method will make the root configurable object dump
   *  it's current configuration to this object. The setup method can 
   *  be called in the derived object's constructor for example.
   */
  void config_setup ();
  
  /**
   *  @brief Clear (reset to default) the configuration
   *
   *  If called on a child, local configurations are cleared.
   *  If called on the root, the default configuration is restored.
   */
  void clear_config ();

  /**
   *  @brief Set a certain configuration parameter
   *
   *  This method will promote the configuration to the first child
   *  that accepts it and store the value in the repository for the
   *  root element.
   */
  void config_set (const std::string &name, const std::string &value);

  /**
   *  @brief Set a certain configuration parameter
   *
   *  This method will promote the configuration to the first child
   *  that accepts it and store the value in the repository for the
   *  root element.
   */
  void config_set (const std::string &name, const char *value);

  /**
   *  @brief Set a certain configuration parameter with a given type
   *
   *  This method will promote the configuration to the first child
   *  that accepts it and store the value in the repository for the
   *  root element. It will use tl::to_string to convert the type into
   *  a string.
   */
  template <class T> 
  void config_set (const std::string &name, const T &value)
  {
    config_set (name, tl::to_string (value));
  }

  /**
   *  @brief Set a certain configuration parameter with a given type using a custom string converter
   *
   *  This method will promote the configuration to the first child
   *  that accepts it and store the value in the repository for the
   *  root element. It will use conv.to_string to convert the type into
   *  a string.
   */
  template <class T, class C> 
  void config_set (const std::string &name, const T &value, C conv)
  {
    config_set (name, conv.to_string (value));
  }

  /**
   *  @brief Terminate a sequence of configuration setups
   *
   *  In order to make configuration changes effective, this method
   *  must be called. It calls config_finalize recursively on the 
   *  children. In GUI-enabled applications this step is optional
   *  and is performed automatically through a timer.
   */
  void config_end ();

  /**
   *  @brief Get a certain configuration parameter
   *
   *  This method will fetch the parameter from the repository.
   *  It can only be called on the root element.
   *  If no parameter with the given name is known, this method will 
   *  return false and set "value" to empty.
   */
  bool config_get (const std::string &name, std::string &value) const;

  /**
   *  @brief Get a certain configuration parameter
   *
   *  This is a convenience method which behaves similar. It will return the
   *  configuration value for the given configuration option name. However, if the
   *  configuration option with the given name does not exist, simply an 
   *  empty string is returned.
   */
  std::string config_get (const std::string &name) const
  {
    std::string v;
    config_get (name, v);
    return v;
  }

  /**
   *  @brief Get a certain configuration parameter
   *
   *  This method will fetch the parameter from the repository
   *  and try to convert it into the value requested. To convert
   *  it, tl::from_string is used. If an exception is thrown during
   *  the conversion or the value is not present, false is returned.
   */
  template <class T>
  bool config_get (const std::string &name, T &value) const
  {
    T t;
    std::string s;
    if (! config_get (name, s)) {
      return false;
    }
    try {
      tl::from_string (s, t);
      value = t;
      return true;
    } catch (...) {
      return false;
    }
  }

  /**
   *  @brief Get a certain configuration parameter using a custom converter
   *
   *  This method will fetch the parameter from the repository
   *  and try to convert it into the value requested. To convert
   *  it, conv.from_string is used. If an exception is thrown during
   *  the conversion or the value is not present, false is returned.
   */
  template <class T, class C>
  bool config_get (const std::string &name, T &value, C conv) const
  {
    T t;
    std::string s;
    if (! config_get (name, s)) {
      return false;
    }
    try {
      conv.from_string (s, t);
      value = t;
      return true;
    } catch (...) {
      return false;
    }
  }

  /**
   *  @brief Iterator for the key/value pairs: begin iterator
   */
  iterator begin () const
  {
    return m_repository.begin ();
  }

  /**
   *  @brief Iterator for the key/value pairs: end iterator
   */
  iterator end () const
  {
    return m_repository.end ();
  }

  /**
   *  @brief Get the names of all configuration options available
   */
  void get_config_names (std::vector<std::string> &names) const;

  /**
   *  @brief Gets the dispatcher (the top level end of the plugin chain)
   *  The returned pointer is guaranteed to be non-zero.
   */
  Dispatcher *dispatcher ();

  /**
   *  @brief Notifies the plugin that a child plugin got deleted
   */
  virtual void unregister_plugin (lay::Plugin * /*plugin*/)
  {
    // .. this implementation does nothing ..
  }

  /**
   *  @brief Menu command handler
   *
   *  This method is called if a menu entry registered in the 
   *  plugin declaration is activated. The string passed is the 
   *  symbol of the menu entry.
   */
  virtual void menu_activated (const std::string & /*symbol*/) 
  {
    // .. this implementation does nothing ..
  }

#if defined(HAVE_QT)
  /**
   *  @brief Return the lay::Browser interface if this object has one
   *
   *  If the object implements the lay::Browser interface, return the pointer to this.
   *  Otherwise the implementation is supposed to return 0.
   */
  virtual lay::Browser *browser_interface ()
  {
    return 0;
  }
#endif

  /**
   *  @brief Return the lay::ViewService interface if this object has one
   *
   *  If the object implements the lay::ViewService interface, return the pointer to this.
   *  Otherwise the implementation is supposed to return 0.
   */
  virtual lay::ViewService *view_service_interface ()
  {
    return 0;
  }

  /**
   *  @brief Return the lay::Drawing interface if this object has one
   *
   *  If the object implements the lay::Drawing interface, return the pointer to this.
   *  Otherwise the implementation is supposed to return 0.
   */
  virtual lay::Drawing *drawing_interface ()
  {
    return 0;
  }

  /**
   *  @brief Return the lay::Editable interface if this object has one
   *
   *  If the object implements the lay::Editable interface, return the pointer to this.
   *  Otherwise the implementation is supposed to return 0.
   */
  virtual lay::Editable *editable_interface ()
  {
    return 0;
  }

  /**
   *  @brief Gets a value indicating whether the plugin will accept a dropped file with the given URL or path
   */
  virtual bool accepts_drop (const std::string & /*path_or_url*/) const
  {
    return false;
  }

  /**
   *  @brief Gets called when a file or URL is dropped on the plugin
   */
  virtual void drop_url (const std::string & /*path_or_url*/)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Associate a plugin with the plugin declaration for that service (getter)
   */
  const PluginDeclaration *plugin_declaration () const
  {
    return mp_plugin_declaration;
  }

  /**
   *  @brief Gets the plugin parent
   */
  Plugin *plugin_parent ()
  {
    return mp_parent;
  }

  /**
   *  @brief Associate a service with the plugin declaration for that service (setter)
   *
   *  The association is done when the plugin is created. It should not be changed.
   */
  void set_plugin_declaration (const PluginDeclaration *pd) 
  {
    mp_plugin_declaration = pd;
  }

protected:
  /**
   *  @brief Configure method
   *
   *  This method must be supplied by derived classes. If the class
   *  knows about the configuration option it must consume the value
   *  and return 'true'.
   *  No selection is made for "matching" configuration options for
   *  this class.
   */
  virtual bool configure (const std::string & /*name*/, const std::string & /*value*/)
  {
    return false;
  }

  /**
   *  @brief Configuration finalization
   *
   *  This method is called after all configuration changes have been applied.
   *  A derived class might implement this method in order to do some post-
   *  processing of the configuration.
   */
  virtual void config_finalize ()
  {
    //  .. the default implementation does nothing ..
  }

private:
  Plugin (const Plugin &);
  Plugin &operator= (const Plugin &);

  /**
   *  @brief Do the actual setup or pass to the parent if not the root
   */
  void do_config_setup (Plugin *target);

  /**
   *  @brief Do the actual set or pass to the children if not taken 
   */
  bool do_config_set (const std::string &name, const std::string &value, bool for_child);

  /**
   *  @brief Recursively call config_finalize
   */
  void do_config_end ();

  Plugin *mp_parent;
  const PluginDeclaration *mp_plugin_declaration;
  tl::weak_collection<Plugin> m_children;
  std::map <std::string, std::string> m_repository;
  tl::DeferredMethod<lay::Plugin> dm_finalize_config;
  bool m_standalone;
};

/**
 *  @brief A handy function for implementing the configure method
 *
 *  This template compares two values and overwrites the target
 *  with the source if both are different. It returns true, if 
 *  the target was updated indicating that something needs to be
 *  updated.
 */
template <class T>
inline bool test_and_set (T &target, const T &source)
{
  if (target != source) {
    target = source;
    return true;
  } else {
    return false;
  }
}

}

#endif


