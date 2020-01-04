
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


#ifndef HDR_layAbstractMenu
#define HDR_layAbstractMenu

#include <QKeySequence>
#include <QShortcut>
#include <QAction>
#include <QMenu>
#include <QObject>

#include <string>
#include <set>
#include <map>
#include <vector>
#include <list>

#include "gsi.h"
#include "gsiObject.h"
#include "tlStableVector.h"
#include "tlObject.h"
#include "laybasicCommon.h"

class QFrame;
class QMenuBar;
class QToolBar;
class QMenu;

namespace lay
{

class AbstractMenuProvider;
class Action;
class AbstractMenu;
class PluginRoot;

/**
 *  @brief A utility function to convert the packed key binding in the cfg_key_bindings string to a vector
 */
LAYBASIC_PUBLIC std::vector<std::pair<std::string, std::string> > unpack_key_binding (const std::string &packed);

/**
 *  @brief A utility function to convert the key binding (as path/shortcut pair vector) to a packed string for cfg_key_bindings
 */
LAYBASIC_PUBLIC std::string pack_key_binding (const std::vector<std::pair<std::string, std::string> > &unpacked);

/**
 *  @brief A utility function to convert the packed hidden flags in the cfg_menu_items_hidden string to a vector
 */
LAYBASIC_PUBLIC std::vector<std::pair<std::string, bool> > unpack_menu_items_hidden (const std::string &packed);

/**
 *  @brief A utility function to convert the hidde flags (as path/bool pair vector) to a packed string for cfg_menu_items_hidden
 */
LAYBASIC_PUBLIC std::string pack_menu_items_hidden (const std::vector<std::pair<std::string, bool> > &unpacked);

/**
 *  @brief A helper class that does reference counting for the QAction object
 */
class ActionHandle
  : public QObject
{
Q_OBJECT

public:
  ActionHandle (QWidget *parent);
  ActionHandle (QAction *action, bool owned = true);
  ActionHandle (QMenu *menu, bool owned = true);
  ~ActionHandle ();
  void add_ref ();
  void remove_ref ();
  QAction *ptr () const;
  QMenu *menu () const;

  void set_visible (bool v);
  void set_hidden (bool h);
  bool is_visible () const;
  bool is_hidden () const;
  bool is_effective_visible () const;

  void set_default_shortcut (const std::string &sc);
  void set_shortcut (const std::string &sc);
  std::string get_default_shortcut () const;
  std::string get_shortcut() const;
  QKeySequence get_key_sequence () const;
  QKeySequence get_key_sequence_for (const std::string &sc) const;

protected slots:
  void destroyed (QObject *obj);

private:
  QMenu *mp_menu;
  QAction *mp_action;
  int m_ref_count;
  bool m_owned;
  bool m_visible;
  bool m_hidden;
  std::string m_default_shortcut;
  QKeySequence m_default_key_sequence;
  std::string m_shortcut;
  QKeySequence m_key_sequence;
  bool m_no_key_sequence;

  //  no copying
  ActionHandle (const ActionHandle &);
  ActionHandle &operator= (const ActionHandle &);
};

/**
 *  @brief A QAction proxy 
 *
 *  This class is provided to be better suited to automation because it provides the corresponding interfaces
 *  and allows implementing the "trigger" event by reimplementation of the slot (after being connected).
 *  In addition, by acting as a proxy, it can act as a reference to a QAction object created by Qt itself (i.e.
 *  within a QMenu).
 *  To reimplement the "triggered" method, derive a class from Action and reimplement "triggered". Then
 *  pass a reference to this object to "insert_item" of AbstractMenu and store away the derived object.
 *  This object then will receive the triggered events, although the menu holds a pure Action object. 
 *  This works since the basic object is the QAction to which both the derived object and the Action 
 *  point to. A triggered event will be forwarded from this QAction to all referencing Action objects.
 */
class LAYBASIC_PUBLIC Action
  : public QObject
{
Q_OBJECT

public:
  /**
   *  @brief The default constructor
   *
   *  This constructor creates an QAction object internally which is owned by the Action object.
   */
  Action (); 

  /**
   *  @brief The copy constructor
   *
   *  This constructor creates an QAction object internally which is owned by the Action object.
   */
  Action (const Action &action); 

  /**
   *  @brief Create an action with the given title (icon, keyboard shortcut)
   *
   *  The format of the title string is: <text>["("shortcut")"]["<"icon-resource">"]
   *
   *  @param title The title string encoding icon and keyboard shortcut if applicable.
   */
  Action (const std::string &title); 

  /**
   *  @brief Creates a new free action
   *  This constructor wil create a new action with it's own QAction object
   *  under the given parent.
   */
  static Action create_free_action (QWidget *parent);

  /**
   *  @brief Assignement
   */
  Action &operator= (const Action &action);

  /**
   *  @brief The destructor
   */
  ~Action (); 

  /**
   *  @brief Set the title
   */
  void set_title (const std::string &t);

  /**
   *  @brief Get the title
   */
  std::string get_title () const;

  /**
   *  @brief Gets the shortcut string for "no shortcut present"
   */
  static const std::string &no_shortcut ();

  /**
   *  @brief Sets the keyboard shortcut
   *  If no shortcut is set (empty string), the default shortcut will be taken.
   *  If the shortcut string is "no_shortcut()", no shortcut will be assigned to the item.
   */
  void set_shortcut (const std::string &s);

  /**
   *  @brief Gets the keyboard shortcut
   *  To get the effective shortcut (combination of default shortcut and shortcut),
   *  use "get_effective_shortcut".
   */
  std::string get_shortcut () const;

  /**
   *  @brief Sets the default keyboard shortcut
   *  This shortcut is used when no specific shortcut is set.
   */
  void set_default_shortcut (const std::string &s);

  /**
   *  @brief Gets the default keyboard shortcut
   *  To get the effective shortcut (combination of default shortcut and shortcut),
   *  use "get_effective_shortcut".
   */
  std::string get_default_shortcut () const;

  /**
   *  @brief Gets the effective shortcut
   */
  std::string get_effective_shortcut () const;

  /**
   *  @brief Gets the effective shortcut for a given key sequence string
   */
  std::string get_effective_shortcut_for (const std::string &sc) const;

  /**
   *  @brief "is_checkable" attribute
   */
  bool is_checkable () const;

  /**
   *  @brief "is_checked" attribute
   */
  bool is_checked () const;

  /**
   *  @brief "is_enabled" attribute
   */
  bool is_enabled () const;

  /**
   *  @brief "is_visible" attribute
   */
  bool is_visible () const;

  /**
   *  @brief Gets a value indicating whether the action is intentionally hidden
   *  This flag combines with the visibility. "is_effective_visible" is false
   *  if hidden is true. This feature allows implementation of the menu configuration
   *  feature where users can deliberately switch off and on menu items.
   */
  bool is_hidden () const;

  /**
   *  @brief Gets the effective visibility
   *  See "is_hidden" for details.
   */
  bool is_effective_visible () const;

  /**
   *  @brief "is_separator" attribute
   */
  bool is_separator () const;

  /**
   *  @brief Enable or disable the action
   */
  void set_enabled (bool b);

  /**
   *  @brief Show or hide
   */
  void set_visible (bool v);

  /**
   *  @brief Sets a value indicating whether the menu item is hidden
   *  See "is_hidden" for details.
   */
  void set_hidden (bool h);

  /**
   *  @brief Make checkable or not
   */
  void set_checkable (bool c);

  /**
   *  @brief Check or uncheck
   */
  void set_checked (bool c);

  /**
   *  @brief Make a separator (or not)
   */
  void set_separator (bool s);

  /**
   *  @brief Set the tool tip text 
   *
   *  @param text The text to display in the tool tip
   */
  void set_tool_tip (const std::string &text);

  /**
   *  @brief Set the icon to the given picture
   *
   *  @param file The file to load for the icon
   *
   *  Passing an empty string will reset the icon
   */
  void set_icon (const std::string &filename);

  /**
   *  @brief Set the icon's text 
   *
   *  If an icon text is set, this will be used for the text below the icon.
   *  If no icon text is set, the normal text will be used for the icon.
   *  Passing an empty string will reset the icon's text.
   */
  void set_icon_text (const std::string &icon_text);

  /** 
   *  @brief Set the action's QObject name (for testing for example)
   */
  void set_object_name (const std::string &name);

  /**
   *  @brief Makes this action part of the exclusive group with the given name
   */
  void add_to_exclusive_group (lay::AbstractMenu *menu, const std::string &group_name);

  /**
   *  @brief Get the tool tip text 
   */
  std::string get_tool_tip () const;

  /**
   *  @brief Get the icon's text 
   */
  std::string get_icon_text () const;

  /**
   *  @brief Trigger the action
   */
  void trigger ();

  /**
   *  @brief Reimplement this method to implement a trigger handler
   */
  virtual void triggered ();

  /**
   *  @brief Get the underlying QAction object
   *
   *  Hint: do not use for changing the shortcut!
   */
  QAction *qaction () const;

  /**
   *  @brief Gets the QMenu object if the action is a menu action
   */
  QMenu *menu () const;

  /**
   *  @brief Compares two action objects 
   *
   *  Two action objects are equal when they refer to the same ActionHandle.
   */
  bool operator== (const Action &other) const
  {
    return mp_handle == other.mp_handle;
  }

public slots:
  void triggered_slot ();

private:
  friend class AbstractMenu;

  ActionHandle *mp_handle;

  /**
   *  @brief The proxy constructor
   *
   *  This constructor takes a QAction object that it will refer to.
   *  If the Action is copied, the copy will refer to the same QAction.
   *  The QAction object is deleted if the last Action referring to QAction is deleted.
   */
  Action (ActionHandle *action);
};

/**
 *  @brief A specialisation for the Action to issue a "configure" request on "triggered"
 *
 *  When this action is triggered, a "configure" request is issued to the PluginRoot instance
 *  (which is the root of the configuration hierarchy). The name and value is given by the
 *  respective parameters passed to the constructor or set with the write accessors.
 */
class LAYBASIC_PUBLIC ConfigureAction
  : public Action,
    public tl::Object
{
public:
  enum type { setter_type = 0, boolean_type = 1, choice_type = 2 };

  /**
   *  @brief The default constructor
   *
   *  @param pr The reference to the plugin root object which receives the configure request 
   */
  ConfigureAction (lay::PluginRoot *pr); 

  /**
   *  @brief Create an configure action with the given name and value
   *
   *  @param pr The reference to the plugin root object which receives the configure request 
   *  @param cname The name of the configuration parameter to set
   *  @param cvalue The value to set "cname" to
   *
   *  The value can be "?" in which case the configuration action describes 
   *  a boolean parameter which is mapped to a checkable action.
   */
  ConfigureAction (lay::PluginRoot *pr, const std::string &cname, const std::string &value); 

  /**
   *  @brief Create an configure action with the given title (icon, keyboard shortcut), name and value
   *
   *  The format of the title string is: <text>["("shortcut")"]["<"icon-resource">"]
   *
   *  @param pr The reference to the plugin root object which receives the configure request 
   *  @param title The title string encoding icon and keyboard shortcut if applicable.
   *  @param cname The name of the configuration parameter to set
   *  @param cvalue The value to set "cname" to
   *
   *  The value can be "?" in which case the configuration action describes 
   *  a boolean parameter which is mapped to a checkable action.
   */
  ConfigureAction (lay::PluginRoot *pr, const std::string &title, const std::string &cname, const std::string &value); 

  /**
   *  @brief Destructor
   */
  ~ConfigureAction ();

  /**
   *  @brief Configuration parameter name setter
   */
  void set_cname (const std::string &cname)
  {
    m_cname = cname;
  }

  /**
   *  @brief Configuration parameter name getter
   */
  const std::string &get_cname () const
  {
    return m_cname;
  }

  /**
   *  @brief Configuration parameter value setter
   */
  void set_cvalue (const std::string &cvalue)
  {
    m_cvalue = cvalue;
  }

  /**
   *  @brief Configuration parameter value getter
   */
  const std::string &get_cvalue () const
  {
    return m_cvalue;
  }

  /**
   *  @brief Transfer the value into this action
   *
   *  This method is called by the main window when the respective configuration changes.
   */
  void configure (const std::string &value);
  
protected:
  virtual void triggered ();

  void reg ();
  void unreg ();

private:
  ConfigureAction (const ConfigureAction &action); 
  ConfigureAction &operator= (const ConfigureAction &action); 

  lay::PluginRoot *m_pr;
  std::string m_cname, m_cvalue;
  type m_type;
};

/**
 *  @brief A menu layout entry for static initialisation
 *
 *  These objects act as the "source" for creating the initial AbstractLayout setup.
 */
struct LAYBASIC_PUBLIC MenuLayoutEntry {

  MenuLayoutEntry (const char *_name, 
                   const std::string &_title, 
                   const std::pair<std::string, std::string> &_kv_pair)
    : name (_name), title (_title), slot (0), kv_pair (_kv_pair), submenu (0)
  { }

  MenuLayoutEntry (const char *_name, 
                   const std::string &_title, 
                   const char *_slot, 
                   const MenuLayoutEntry *_submenu = 0)
    : name (_name), title (_title), slot (_slot), submenu (_submenu)
  { }

  MenuLayoutEntry (const char *_name, 
                   const std::string &_title, 
                   const MenuLayoutEntry *_submenu = 0)
    : name (_name), title (_title), slot (0), submenu (_submenu)
  { }

  MenuLayoutEntry (const char *_name)
    : name (_name), slot (0), submenu (0)
  { }

  MenuLayoutEntry ()
    : name (0), slot (0), submenu (0)
  { }

  static MenuLayoutEntry separator (const char *name) 
  {
    return MenuLayoutEntry (name);
  }

  static MenuLayoutEntry last () 
  {
    return MenuLayoutEntry ();
  }

  const char *name;
  std::string title;
  const char *slot;
  std::pair<std::string, std::string> kv_pair;
  const MenuLayoutEntry *submenu;

};

/**
 *  @brief One item in the abstract menu
 */
struct LAYBASIC_PUBLIC AbstractMenuItem
{
  AbstractMenuItem ();

  //  No copy constructor semantics because we don't need it (we use list's) and Action does not provide one.
  AbstractMenuItem (const AbstractMenuItem &);

  /**
   *  @brief Internal method used to set up the item
   */
  void setup_item (const std::string &pn, const std::string &n, const Action &a); 

  const std::string &name () const 
  {
    return m_name;    
  }

  const std::set<std::string> &groups () const
  {
    return m_groups;
  }

  void set_action (const Action &a, bool copy_properties);

  void set_action_title (const std::string &t);

  const Action &action () const
  {
    return m_action;
  }

  QMenu *menu () const
  {
    return m_action.menu ();
  }

  void set_has_submenu ();

  bool has_submenu () const
  {
    return m_has_submenu;
  }

  void set_remove_on_empty ();

  bool remove_on_empty () const
  {
    return m_remove_on_empty;
  }

  std::list <AbstractMenuItem> children;

private:
  bool m_has_submenu;
  bool m_remove_on_empty;
  Action m_action;
  std::string m_name;
  std::string m_basename;
  std::set<std::string> m_groups;
};

/**
 *  @brief The abstract menu class
 *
 *  The abstract menu is a class that stores a main menu and several popup menus
 *  in a generic form such that they can be manipulated and converted into QMenu's or
 *  QMenuBar setups.
 *
 *  Each item can be associated with a Action, which delivers a title, enabled/disable state etc.
 *  The Action is either provided when new entries are inserted or created upon initialisation.
 *
 *  An abstract menu is initialised from a initialisation sequence given by a list of
 *  MenuLayoutEntry objects terminated with a MenuLayoutEntry::last() item.
 *  Each of these entries may provide a submenu. Beside for initialisation, these entries
 *  are not used further.
 *
 *  The abstract menu class provides methods to manipulate the menu structure (the state of the
 *  menu items, their title and shortcut key is provided and manipulated through the Action object). 
 *
 *  Menu items and submenus are referred to by a "path". The path is a string with this interpretation:
 *
 *    ""                 is the root 
 *    "[<path>.]<name>"  is an element of the submenu given by <path>. If <path> is omitted, this refers to 
 *                       an element in the root 
 *    "[<path>.]end      refers to the item past the last item of the submenu given by <path> or root
 *    "[<path>.]begin    refers to the first item of the submenu given by <path> or root
 *    "[<path>.]#<n>     refers to the nth item of the submenu given by <path> or root (n is an integer number)
 *
 *  The abstract menu provides methods to obtain QMenu and Action objects associated with a certain
 *  item by specifying a path.
 *
 *  Menu items can be put into groups. The path strings of each group can be obtained with the 
 *  "group" method. An item is put into a group by appending ":<group-name>" to the item's name.
 *  This specification can be used several times.
 *
 *  Detached menus (i.e. for use in context menus) can be created as virtual top-level submenus
 *  with a name of the form "@<name>". Such menu's are created as detached QMenu object's and not
 *  inserted into the main menu. A special detached menu is "@toolbar" which describes all elements
 *  placed into the toolbar passed to build (). Detached menu objects can be retrieved with the
 *  detached_menu () method, passing the desired name (without the "@"). 
 *  A detached (separate menu bar) can be created as well from detached menu bars. Such menu
 *  bars are specified by submenus starting with "@@". Detached menu bars can be filled using the 
 *  build_detached () method.
 *
 *  The abstract menu class issues a signal ("changed") if the structure has changed. To recreate the
 *  QMenu objects and fill the QMenuBar, the "build" method needs to be called. This will delete all
 *  QMenu's created so far (except the detached menus), clear the QMenuBar, recreate the QMenu objects (note, that the 
 *  addresses will change this way!) and refill the QToolBar and QMenuBar.
 */
class LAYBASIC_PUBLIC AbstractMenu
  : public QObject,
    public gsi::ObjectBase
{
Q_OBJECT

public:
  /**
   *  @brief Create the abstract menu object attached to the given main window
   */
  AbstractMenu (AbstractMenuProvider *provider);
  
  /** 
   *  @brief Destroy the abstract menu object
   */
  ~AbstractMenu ();

  /**
   *  @brief Initialise from a sequence of MenuLayoutEntry objects
   */
  void init (const MenuLayoutEntry *layout);

  /**
   *  @brief Rebuild the QMenu's and refill the QMenuBar object
   */
  void build (QMenuBar *mbar, QToolBar *tbar);

  /**
   *  @brief Rebuild a menu into a 
   *
   *  This will fill the given menu bar with a menu from
   *  a "@.." top level entry. The menu bar is cleared and rebuilt.
   *
   *  @param name The name of the detached menu, without the "@"
   *  @param mbar The menu bar into which to build the menu
   */
  void build_detached (const std::string &name, QFrame *mbar);

  /**
   *  @brief Get the reference to a QMenu object
   *
   *  @param path The path to the submenu
   *  @return A reference to a QMenu object or 0 if the path is not valid or does not refer to a submenu
   */
  QMenu *menu (const std::string &path);

  /**
   *  @brief Get the Action object for a given item
   *
   *  The Action object returned is basically a pointer to the underlying QAction object,
   *  which is potentially shared between multiple menu items.
   *
   *  @param path The path to the item. This must be a valid path.
   *  @return The action object
   */
  Action action (const std::string &path) const;

  /**
   *  @brief Get the subitems for a given submenu
   *
   *  @param path The path to the submenu
   *  @return A vector or path strings for the child items or an empty vector if the path is not valid or the item does not have children
   */
  std::vector<std::string> items (const std::string &path) const;

  /**
   *  @brief Query if an item is a menu
   *
   *  @param path The path to the item
   *  @return false if the path is not valid or is not a menu
   */
  bool is_menu (const std::string &path) const;

  /**
   *  @brief Query if an item is a separator
   *
   *  @param path The path to the item
   *  @return false if the path is not valid or is not a separator
   */
  bool is_separator (const std::string &path) const;

  /**
   *  @brief Query if a path is a valid one
   *
   *  @param path The path to check
   *  @return false if the path is not a valid path to an item
   */
  bool is_valid (const std::string &path) const;

  /**
   *  @brief Insert a new item before the one given by the path
   *
   *  The action passed to this method is copied as the item's action. The item will refer to 
   *  this action and issue triggered events on this.
   *
   *  @param path The path to the item before which to insert the new item
   *  @param name The name of the item to insert 
   *  @param action The action associated with the item
   */
  void insert_item (const std::string &path, const std::string &name, const Action &action); 

  /**
   *  @brief Insert a new separator before the one item by the path
   *
   *  @param path The path to the item before which to insert the separator
   *  @param name The name of the separator to insert 
   */
  void insert_separator (const std::string &path, const std::string &name); 

  /**
   *  @brief Insert a new submenu before the item given by the path
   *
   *  The action passed to this method must be provided and owned by the caller
   *  unless "owned" is true.
   *
   *  @param path The path to the item before which to insert the new submenu
   *  @param name The name of the submenu to insert 
   *  @param action The action associated with the submenu
   */
  void insert_menu (const std::string &path, const std::string &name, const Action &action);

  /**
   *  @brief Insert a new submenu before the item given by the path
   *
   *  The title string optionally encodes the key shortcut and icon resource
   *  in the form <text>["("shortcut")"]["<"icon-resource">"].
   *
   *  @param path The path to the item before which to insert the submenu
   *  @param name The name of the submenu to insert 
   *  @param title The title of the submenu to insert
   */
  void insert_menu (const std::string &path, const std::string &name, const std::string &title); 

  /**
   *  @brief Deletes the children of the item with the given path
   *
   *  If the item does not exist or is not a menu, this method does nothing.
   */
  void clear_menu (const std::string &path);

  /**
   *  @brief Delete the item given by the path
   *
   *  @param path The path to the item to delete
   */
  void delete_item (const std::string &path);

  /**
   *  @brief Delete the items referring to the given action
   */
  void delete_items (const Action &action);

  /**
   *  @brief Get the group members
   *
   *  @param group The group name
   *  @param A vector of all members (by path) of the group
   */
  std::vector<std::string> group (const std::string &name) const;

  /**
   *  @brief Get the detached menu
   *
   *  This will return a QMenu pointer to a detached menu that is created
   *  with a "@.." top level entry. In any case, a valid QMenu object is returned
   *  which never changes, even if the menu hierarchy is rebuilt. The QMenu returned
   *  may be empty.
   *
   *  @param name The name of the detached menu, without the "@"
   *  @return a valid QMenu object
   */
  QMenu *detached_menu (const std::string &name);

  /**
   *  @brief Creates a new exclusive action group
   *
   *  If a group with that name already exists, this method does nothing.
   *
   *  @return The QActionGroup object of that group
   */
  QActionGroup *make_exclusive_group (const std::string &name);

  /**
   *  @brief Gets the root node of the menu
   */
  const AbstractMenuItem &root () const
  {
    return m_root;
  }

signals:
  /**
   *  @brief this signal is emitted whenever something changes on the menu
   */
  void changed ();

private:
  friend class Action;

  std::vector<std::pair<AbstractMenuItem *, std::list<AbstractMenuItem>::iterator> > find_item (const std::string &path);
  const AbstractMenuItem *find_item_exact (const std::string &path) const;
  AbstractMenuItem *find_item_exact (const std::string &path);
  void transfer (const MenuLayoutEntry *layout, AbstractMenuItem &item);
  void build (QMenu *menu, std::list<AbstractMenuItem> &items);
  void build (QToolBar *tbar, std::list<AbstractMenuItem> &items);
  void collect_group (std::vector<std::string> &grp, const std::string &name, const AbstractMenuItem &item) const;

  /**
   *  @brief Create a action from a string
   *
   *  The format of the string is: <text>["("shortcut")"]["<"icon-resource">"]
   *
   *  @param s The title, key and icon resource string in the format given above
   *  @param parent The widget to which to attach the QAction object
   *  @return The ActionHandle object created
   */
  static ActionHandle *create_action (const std::string &s);

  AbstractMenuProvider *mp_provider;
  AbstractMenuItem m_root;
  tl::stable_vector<QMenu> m_helper_menu_items;
  std::map<std::string, QActionGroup *> m_action_groups;
};

}

namespace tl
{
  template <> struct type_traits<lay::AbstractMenu> : public type_traits<void> {
    typedef tl::false_tag has_copy_constructor;
    typedef tl::false_tag has_default_constructor;
  };

}

#endif


