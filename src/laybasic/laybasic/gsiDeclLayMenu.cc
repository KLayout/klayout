
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
#include "gsiSignals.h"
#include "layAbstractMenu.h"

namespace {

//  The Action stub to allow reimplementation of the triggered method
class ActionStub
  : public lay::Action
{
public:
  virtual void triggered ()
  {
    if (triggered_cb.can_issue ()) {
      triggered_cb.issue<lay::Action> (&lay::Action::triggered);
    }
  }

  virtual void menu_opening ()
  {
    if (menu_opening_cb.can_issue ()) {
      menu_opening_cb.issue<lay::Action> (&lay::Action::menu_opening);
    }
  }

  virtual bool wants_visible () const
  {
    if (wants_visible_cb.can_issue ()) {
      return wants_visible_cb.issue<lay::Action, bool> (&lay::Action::wants_visible);
    } else {
      return true;
    }
  }

  virtual bool wants_enabled () const
  {
    if (wants_enabled_cb.can_issue ()) {
      return wants_enabled_cb.issue<lay::Action, bool> (&lay::Action::wants_enabled);
    } else {
      return true;
    }
  }

  gsi::Callback triggered_cb;
  gsi::Callback menu_opening_cb;
  gsi::Callback wants_visible_cb;
  gsi::Callback wants_enabled_cb;
};

}

namespace gsi
{

static std::string pack_key_binding (const std::map<std::string, std::string> &kb)
{
  std::vector<std::pair<std::string, std::string> > v;
  v.insert (v.end (), kb.begin (), kb.end ());
  return lay::pack_key_binding (v);
}

static std::map<std::string, std::string> unpack_key_binding (const std::string &s)
{
  std::vector<std::pair<std::string, std::string> > v = lay::unpack_key_binding (s);
  std::map<std::string, std::string> kb;
  kb.insert (v.begin (), v.end ());
  return kb;
}

static std::string pack_menu_items_hidden (const std::map<std::string, bool> &kb)
{
  std::vector<std::pair<std::string, bool> > v;
  v.insert (v.end (), kb.begin (), kb.end ());
  return lay::pack_menu_items_hidden (v);
}

static std::map<std::string, bool> unpack_menu_items_hidden (const std::string &s)
{
  std::vector<std::pair<std::string, bool> > v = lay::unpack_menu_items_hidden (s);
  std::map<std::string, bool> kb;
  kb.insert (v.begin (), v.end ());
  return kb;
}

static lay::AbstractMenu *new_menu ()
{
  return new lay::AbstractMenu (0);
}

Class<lay::AbstractMenu> decl_AbstractMenu ("lay", "AbstractMenu",
  //  for test purposes mainly:
  constructor ("new", &new_menu, "@hide") +
  method ("pack_key_binding", &pack_key_binding, gsi::arg ("path_to_keys"),
    "@brief Serializes a key binding definition into a single string\n"
    "The serialized format is used by the 'key-bindings' config key. "
    "This method will take an array of path/key definitions (including the \\Action#NoKeyBound option) "
    "and convert it to a single string suitable for assigning to the config key.\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  method ("unpack_key_binding", &unpack_key_binding, gsi::arg ("s"),
    "@brief Deserializes a key binding definition\n"
    "This method is the reverse of \\pack_key_binding.\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  method ("pack_menu_items_hidden", &pack_menu_items_hidden, gsi::arg ("path_to_visibility"),
    "@brief Serializes a menu item visibility definition into a single string\n"
    "The serialized format is used by the 'menu-items-hidden' config key. "
    "This method will take an array of path/visibility flag definitions "
    "and convert it to a single string suitable for assigning to the config key.\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  method ("unpack_menu_items_hidden", &unpack_menu_items_hidden, gsi::arg ("s"),
    "@brief Deserializes a menu item visibility definition\n"
    "This method is the reverse of \\pack_menu_items_hidden.\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  method ("action", (lay::Action *(lay::AbstractMenu::*) (const std::string &path)) &lay::AbstractMenu::action, gsi::arg ("path"),
    "@brief Gets the reference to a Action object associated with the given path\n"
    "\n"
    "@param path The path to the item.\n"
    "@return A reference to a Action object associated with this path or nil if the path is not valid\n"
  ) + 
  method ("items", &lay::AbstractMenu::items, gsi::arg ("path"),
    "@brief Gets the subitems for a given submenu\n"
    "\n"
    "@param path The path to the submenu\n"
    "@return A vector or path strings for the child items or an empty vector if the path is not valid or the item does not have children\n"
  ) +
  method ("is_menu?", &lay::AbstractMenu::is_menu, gsi::arg ("path"),
    "@brief Returns true if the item is a menu\n"
    "\n"
    "@param path The path to the item\n"
    "@return false if the path is not valid or is not a menu\n"
  ) +
  method ("is_separator?", &lay::AbstractMenu::is_separator, gsi::arg ("path"),
    "@brief Returns true if the item is a separator\n"
    "\n"
    "@param path The path to the item\n"
    "@return false if the path is not valid or is not a separator\n"
    "\n"
    "This method has been introduced in version 0.19.\n"
  ) +
  method ("is_valid?", &lay::AbstractMenu::is_valid, gsi::arg ("path"),
    "@brief Returns true if the path is a valid one\n"
    "\n"
    "@param path The path to check\n"
    "@return false if the path is not a valid path to an item\n"
  ) +
  method ("insert_item", (void (lay::AbstractMenu::*) (const std::string &, const std::string &, const lay::Action *)) &lay::AbstractMenu::insert_item, gsi::arg ("path"), gsi::arg ("name"), gsi::arg ("action"),
    "@brief Inserts a new item before the one given by the path\n"
    "\n"
    "The Action object passed as the third parameter references the handler which both implements the "
    "action to perform and the menu item's appearance such as title, icon and keyboard shortcut.\n"
    "\n"
    "@param path The path to the item before which to insert the new item\n"
    "@param name The name of the item to insert \n"
    "@param action The Action object to insert\n"
  ) +
  method ("insert_separator", &lay::AbstractMenu::insert_separator, gsi::arg ("path"), gsi::arg ("name"),
    "@brief Inserts a new separator before the item given by the path\n"
    "\n"
    "@param path The path to the item before which to insert the separator\n"
    "@param name The name of the separator to insert \n"
  ) +
  method ("insert_menu", (void (lay::AbstractMenu::*) (const std::string &, const std::string &, const std::string &)) &lay::AbstractMenu::insert_menu, gsi::arg ("path"), gsi::arg ("name"), gsi::arg ("title"),
    "@brief Inserts a new submenu before the item given by the path\n"
    "\n"
    "The title string optionally encodes the key shortcut and icon resource\n"
    "in the form <text>[\"(\"<shortcut>\")\"][\"<\"<icon-resource>\">\"].\n"
    "\n"
    "@param path The path to the item before which to insert the submenu\n"
    "@param name The name of the submenu to insert \n"
    "@param title The title of the submenu to insert\n"
  ) +
  method ("insert_menu", (void (lay::AbstractMenu::*) (const std::string &, const std::string &, lay::Action *)) &lay::AbstractMenu::insert_menu, gsi::arg ("path"), gsi::arg ("name"), gsi::arg ("action"),
    "@brief Inserts a new submenu before the item given by the path\n"
    "\n"
    "@param path The path to the item before which to insert the submenu\n"
    "@param name The name of the submenu to insert \n"
    "@param action The action object of the submenu to insert\n"
    "\n"
    "This method variant has been added in version 0.28."
  ) +
  method ("clear_menu", &lay::AbstractMenu::clear_menu, gsi::arg ("path"),
    "@brief Deletes the children of the item given by the path\n"
    "\n"
    "@param path The path to the item whose children to delete\n"
    "\n"
    "This method has been introduced in version 0.28.\n"
  ) +
  method ("delete_item", &lay::AbstractMenu::delete_item, gsi::arg ("path"),
    "@brief Deletes the item given by the path\n"
    "\n"
    "@param path The path to the item to delete\n"
    "\n"
    "This method will also delete all children of the given item. "
    "To clear the children only, use \\clear_menu.\n"
  ) +
  method ("group", &lay::AbstractMenu::group, gsi::arg ("group"),
    "@brief Gets the group members\n"
    "\n"
    "@param group The group name\n"
    "@param A vector of all members (by path) of the group\n"
  ),
  "@brief An abstraction for the application menus\n"
  "\n"
  "The abstract menu is a class that stores a main menu and several popup menus\n"
  "in a generic form such that they can be manipulated and converted into GUI objects.\n"
  "\n"
  "Each item can be associated with a Action, which delivers a title, enabled/disable state etc.\n"
  "The Action is either provided when new entries are inserted or created upon initialisation.\n"
  "\n"
  "The abstract menu class provides methods to manipulate the menu structure (the state of the\n"
  "menu items, their title and shortcut key is provided and manipulated through the Action object). \n"
  "\n"
  "Menu items and submenus are referred to by a \"path\". The path is a string with this interpretation:\n"
  "\n"
  "@<table>\n"
  "  @<tr>@<td>\"\"                 @</td>@<td>is the root@</td>@</tr> \n"
  "  @<tr>@<td>\"[<path>.]<name>\"  @</td>@<td>is an element of the submenu given by <path>. If <path> is omitted, this refers to an element in the root@</td>@</tr> \n"
  "  @<tr>@<td>\"[<path>.]end\"     @</td>@<td>refers to the item past the last item of the submenu given by <path> or root@</td>@</tr>\n"
  "  @<tr>@<td>\"[<path>.]begin\"   @</td>@<td>refers to the first item of the submenu given by <path> or root@</td>@</tr>\n"
  "  @<tr>@<td>\"[<path>.]#<n>\"    @</td>@<td>refers to the nth item of the submenu given by <path> or root (n is an integer number)@</td>@</tr>\n"
  "@</table>\n"
  "\n"
  "Menu items can be put into groups. The path strings of each group can be obtained with the \n"
  "\"group\" method. An item is put into a group by appending \":<group-name>\" to the item's name.\n"
  "This specification can be used several times.\n"
  "\n"
  "Detached menus (i.e. for use in context menus) can be created as virtual top-level submenus\n"
  "with a name of the form \"@@<name>\". "
  "A special detached menu is \"@toolbar\" which represents the tool bar of the main window. "
  "\n"
  "Menus are closely related to the \\Action class. Actions are used to represent selectable items "
  "inside menus, provide the title and other configuration settings. Actions also link the menu items "
  "with code. See the \\Action class description for further details.\n"
);
  
Class<lay::Action> decl_ActionBase ("lay", "ActionBase",
  method ("title=", &lay::Action::set_title, gsi::arg ("title"),
    "@brief Sets the title\n"
    "\n"
    "@param title The title string to set (just the title)\n"
  ) +
  method ("title", &lay::Action::get_title,
    "@brief Gets the title\n"
    "\n"
    "@return The current title string\n"
  ) +
  method ("shortcut=", (void (lay::Action::*)(const std::string &)) &lay::Action::set_shortcut, gsi::arg ("shortcut"),
    "@brief Sets the keyboard shortcut\n"
    "If the shortcut string is empty, the default shortcut will be used. If the string "
    "is equal to \\Action#NoKeyBound, no keyboard shortcut will be assigned.\n"
    "\n"
    "@param shortcut The keyboard shortcut in Qt notation (i.e. \"Ctrl+C\")\n"
    "\n"
    "The NoKeyBound option has been added in version 0.26."
  ) +
  constant ("NoKeyBound", &lay::Action::no_shortcut,
    "@brief Gets a shortcut value indicating that no shortcut shall be assigned\n"
    "This method has been introduced in version 0.26."
  ) +
  method ("shortcut", &lay::Action::get_shortcut,
    "@brief Gets the keyboard shortcut\n"
    "@return The keyboard shortcut as a string\n"
  ) +
  method ("default_shortcut=", (void (lay::Action::*)(const std::string &)) &lay::Action::set_default_shortcut, gsi::arg ("shortcut"),
    "@brief Sets the default keyboard shortcut\n"
    "\n"
    "The default shortcut is used, if \\shortcut is empty.\n"
    "\n"
    "This attribute has been introduced in version 0.25.\n"
  ) +
  method ("default_shortcut", &lay::Action::get_default_shortcut,
    "@brief Gets the default keyboard shortcut\n"
    "@return The default keyboard shortcut as a string\n"
    "\n"
    "This attribute has been introduced in version 0.25.\n"
  ) +
  method ("effective_shortcut", &lay::Action::get_effective_shortcut,
    "@brief Gets the effective keyboard shortcut\n"
    "@return The effective keyboard shortcut as a string\n"
    "\n"
    "The effective shortcut is the one that is taken. It's either \\shortcut or \\default_shortcut.\n"
    "\n"
    "This attribute has been introduced in version 0.25.\n"
  ) +
  method ("is_separator?", &lay::Action::is_separator,
    "@brief Gets a value indicating whether the item is a separator\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  method ("is_checkable?", &lay::Action::is_checkable,
    "@brief Gets a value indicating whether the item is checkable\n"
  ) +
  method ("is_checked?", &lay::Action::is_checked,
    "@brief Gets a value indicating whether the item is checked\n"
  ) +
  method ("is_enabled?", &lay::Action::is_enabled,
    "@brief Gets a value indicating whether the item is enabled\n"
  ) +
  method ("is_visible?", &lay::Action::is_visible,
    "@brief Gets a value indicating whether the item is visible\n"
    "The visibility combines with \\is_hidden?. To get the true visiblity, use \\is_effective_visible?."
  ) +
  method ("is_hidden?", &lay::Action::is_hidden,
    "@brief Gets a value indicating whether the item is hidden\n"
    "If an item is hidden, it's always hidden and \\is_visible? does not have an effect."
    "\n"
    "This attribute has been introduced in version 0.25.\n"
  ) +
  method ("is_effective_visible?", &lay::Action::is_effective_visible,
    "@brief Gets a value indicating whether the item is really visible\n"
    "This is the combined visibility from \\is_visible? and \\is_hidden? and dynamic visibility (\\wants_visible)."
    "\n"
    "This attribute has been introduced in version 0.25.\n"
  ) +
  method ("is_effective_enabled?", &lay::Action::is_effective_enabled,
    "@brief Gets a value indicating whether the item is really enabled\n"
    "This is the combined value from \\is_enabled? and dynamic value (\\wants_enabled)."
    "\n"
    "This attribute has been introduced in version 0.28.\n"
  ) +
  method ("separator=", &lay::Action::set_separator, gsi::arg ("separator"),
    "@brief Makes an item a separator or not\n"
    "\n"
    "@param separator true to make the item a separator\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  method ("checkable=", &lay::Action::set_checkable, gsi::arg ("checkable"),
    "@brief Makes the item(s) checkable or not\n"
    "\n"
    "@param checkable true to make the item checkable\n"
  ) +
  method ("enabled=", &lay::Action::set_enabled, gsi::arg ("enabled"),
    "@brief Enables or disables the action\n"
    "\n"
    "@param enabled true to enable the item\n"
  ) +
  method ("visible=", &lay::Action::set_visible, gsi::arg ("visible"),
    "@brief Sets the item's visibility\n"
    "\n"
    "@param visible true to make the item visible\n"
  ) +
  method ("hidden=", &lay::Action::set_hidden, gsi::arg ("hidden"),
    "@brief Sets a value that makes the item hidden always\n"
    "See \\is_hidden? for details.\n"
    "\n"
    "This attribute has been introduced in version 0.25\n"
  ) +
  method ("checked=", &lay::Action::set_checked, gsi::arg ("checked"),
    "@brief Checks or unchecks the item\n"
    "\n"
    "@param checked true to make the item checked\n"
  ) +
  method ("icon=", &lay::Action::set_icon, gsi::arg ("file"),
    "@brief Sets the icon to the given image file\n"
    "\n"
    "@param file The image file to load for the icon\n"
    "\n"
    "Passing an empty string will reset the icon.\n"
  ) +
#if defined(HAVE_QT) && defined(HAVE_QTBINDINGS)
  method ("icon=", &lay::Action::set_qicon, gsi::arg ("qicon"),
    "@brief Sets the icon to the given \\QIcon object\n"
    "\n"
    "@param qicon The QIcon object\n"
    "\n"
    "This variant has been added in version 0.28.\n"
  ) +
#endif
  method ("icon_text=", &lay::Action::set_icon_text, gsi::arg ("icon_text"),
    "@brief Sets the icon's text\n"
    "\n"
    "If an icon text is set, this will be used for the text below the icon.\n"
    "If no icon text is set, the normal text will be used for the icon.\n"
    "Passing an empty string will reset the icon's text.\n"
  ) +
  method ("icon_text", &lay::Action::get_icon_text, 
    "@brief Gets the icon's text\n"
  ) +
  method ("tool_tip=", &lay::Action::set_tool_tip, gsi::arg ("text"),
    "@brief Sets the tool tip text\n"
    "\n"
    "The tool tip text is displayed in the tool tip window of the menu entry.\n"
    "This is in particular useful for entries in the tool bar."
    "\n"
    "This method has been added in version 0.22.\n"
  ) +
  method ("tool_tip", &lay::Action::get_tool_tip, 
    "@brief Gets the tool tip text.\n"
    "\n"
    "This method has been added in version 0.22.\n"
  ) +
  method ("trigger", &lay::Action::trigger,
    "@brief Triggers the action programmatically"
  ) +
  gsi::event ("on_triggered", &ActionStub::on_triggered_event,
    "@brief This event is called if the menu item is selected.\n"
    "\n"
    "This event has been introduced in version 0.21 and moved to the ActionBase class in 0.28.\n"
  ) +
  gsi::event ("on_menu_opening", &ActionStub::on_menu_opening_event,
    "@brief This event is called if the menu item is a sub-menu and before the menu is opened.\n"
    "\n"
    "This event provides an opportunity to populate the menu before it is opened.\n"
    "\n"
    "This event has been introduced in version 0.28.\n"
  ),
  "@hide\n"
  "@alias Action\n"
);
  
Class<ActionStub> decl_Action (decl_ActionBase, "lay", "Action",
  gsi::callback ("triggered", &ActionStub::triggered, &ActionStub::triggered_cb,
    "@brief This method is called if the menu item is selected.\n"
    "\n"
    "Reimplement this method is a derived class to receive this event. "
    "You can also use the \\on_triggered event instead."
  ) +
  gsi::callback ("menu_opening", &ActionStub::menu_opening, &ActionStub::menu_opening_cb,
    "@brief This method is called if the menu item is a sub-menu and before the menu is opened."
    "\n"
    "Reimplement this method is a derived class to receive this event. "
    "You can also use the \\on_menu_opening event instead.\n"
    "\n"
    "This method has been added in version 0.28."
  ) +
  gsi::callback ("wants_visible", &ActionStub::wants_visible, &ActionStub::wants_visible_cb,
    "@brief Returns a value whether the action wants to become visible\n"
    "This is a dynamic query for visibility which the system uses to dynamically show or hide "
    "menu items, for example in the MRU lists. This visibility information is evaluated in addition "
    "to \\is_visible? and \\is_hidden? and contributes to the effective visibility status from "
    "\\is_effective_visible?.\n"
    "\n"
    "This feature has been introduced in version 0.28.\n"
  ) +
  gsi::callback ("wants_enabled", &ActionStub::wants_enabled, &ActionStub::wants_enabled_cb,
    "@brief Returns a value whether the action wants to become enabled.\n"
    "This is a dynamic query for enabled state which the system uses to dynamically show or hide "
    "menu items. This information is evaluated in addition "
    "to \\is_enabled? and contributes to the effective enabled status from "
    "\\is_effective_enabled?.\n"
    "\n"
    "This feature has been introduced in version 0.28.\n"
  ),
  "@brief The abstraction for an action (i.e. used inside menus)\n"
  "\n"
  "Actions act as a generalization of menu entries. The action provides the appearance of a menu "
  "entry such as title, key shortcut etc. and dispatches the menu events. The action can be manipulated "
  "to change to appearance of a menu entry and can be attached an observer that receives the events "
  "when the menu item is selected.\n"
  "\n"
  "Multiple action objects can refer to the same action internally, in which "
  "case the information and event handler is copied between the incarnations. This way, a single implementation "
  "can be provided for multiple places where an action appears, for example inside the toolbar and "
  "in addition as a menu entry. Both actions will shared the same icon, text, shortcut etc.\n"
  "\n"
  "Actions are mainly used for providing new menu items inside the \\AbstractMenu class. This is some sample Ruby code for that case:\n"
  "\n"
  "@code\n"
  "a = RBA::Action.new\n"
  "a.title = \"Push Me!\"\n"
  "a.on_triggered do \n"
  "  puts \"I was pushed!\"\n"
  "end\n"
  "\n"
  "app = RBA::Application.instance\n"
  "mw = app.main_window\n"
  "\n"
  "menu = mw.menu\n"
  "menu.insert_separator(\"@toolbar.end\", \"name\")\n"
  "menu.insert_item(\"@toolbar.end\", \"my_action\", a)\n"
  "@/code\n"
  "\n"
  "This code will register a custom action in the toolbar. When the toolbar button is pushed "
  "a message is printed. The toolbar is addressed by a path starting with the pseudo root \"@toolbar\".\n"
  "\n"
  "In Version 0.23, the Action class has been merged with the ActionBase class.\n"
);

}
