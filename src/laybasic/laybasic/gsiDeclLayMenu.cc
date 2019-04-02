
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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

namespace gsi
{

Class<lay::AbstractMenu> decl_AbstractMenu ("lay", "AbstractMenu",
  method ("action", &lay::AbstractMenu::action,
    "@brief Get the reference to a Action object associated with the given path\n"
    "@args path\n"
    "\n"
    "@param path The path to the item. This must be a valid path.\n"
    "@return A reference to a Action object associated with this path\n"
  ) + 
  method ("items", &lay::AbstractMenu::items,
    "@brief Get the subitems for a given submenu\n"
    "@args path\n"
    "\n"
    "@param path The path to the submenu\n"
    "@return A vector or path strings for the child items or an empty vector if the path is not valid or the item does not have children\n"
  ) +
  method ("is_menu?", &lay::AbstractMenu::is_menu,
    "@brief Query if an item is a menu\n"
    "@args path\n"
    "\n"
    "@param path The path to the item\n"
    "@return false if the path is not valid or is not a menu\n"
  ) +
  method ("is_separator?", &lay::AbstractMenu::is_separator,
    "@brief Query if an item is a separator\n"
    "@args path\n"
    "\n"
    "@param path The path to the item\n"
    "@return false if the path is not valid or is not a separator\n"
    "\n"
    "This method has been introduced in version 0.19.\n"
  ) +
  method ("is_valid?", &lay::AbstractMenu::is_valid,
    "@brief Query if a path is a valid one\n"
    "@args path\n"
    "\n"
    "@param path The path to check\n"
    "@return false if the path is not a valid path to an item\n"
  ) +
  method ("insert_item", (void (lay::AbstractMenu::*) (const std::string &, const std::string &, const lay::Action &)) &lay::AbstractMenu::insert_item,
    "@brief Insert a new item before the one given by the path\n"
    "@args path, name, action\n"
    "\n"
    "The Action object passed as the third parameter references the handler which both implements the "
    "action to perform and the menu item's appearance such as title, icon and keyboard shortcut.\n"
    "\n"
    "@param path The path to the item before which to insert the new item\n"
    "@param name The name of the item to insert \n"
    "@param action The Action object to insert\n"
  ) +
  method ("insert_separator", &lay::AbstractMenu::insert_separator,
    "@brief Insert a new separator before the item given by the path\n"
    "@args path, name\n"
    "\n"
    "@param path The path to the item before which to insert the separator\n"
    "@param name The name of the separator to insert \n"
  ) +
  method ("insert_menu", (void (lay::AbstractMenu::*) (const std::string &, const std::string &, const std::string &)) &lay::AbstractMenu::insert_menu,
    "@brief Insert a new submenu before the item given by the path\n"
    "@args path, name, title\n"
    "\n"
    "The title string optionally encodes the key shortcut and icon resource\n"
    "in the form <text>[\"(\"<shortcut>\")\"][\"<\"<icon-resource>\">\"].\n"
    "\n"
    "@param path The path to the item before which to insert the submenu\n"
    "@param name The name of the submenu to insert \n"
    "@param title The title of the submenu to insert\n"
  ) +
  method ("delete_item", &lay::AbstractMenu::delete_item,
    "@brief Delete the item given by the path\n"
    "@args path\n"
    "\n"
    "@param path The path to the item to delete\n"
  ) +
  method ("group", &lay::AbstractMenu::group,
    "@brief Get the group members\n"
    "@args group\n"
    "\n"
    "@param group The group name\n"
    "@param A vector of all members (by path) of the group\n"
  ),
  "@brief The abstract menu class\n"
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
  method ("title=", &lay::Action::set_title,
    "@brief Sets the title\n"
    "@args title\n"
    "\n"
    "@param title The title string to set (just the title)\n"
  ) +
  method ("title", &lay::Action::get_title,
    "@brief Gets the title\n"
    "\n"
    "@return The current title string\n"
  ) +
  method ("shortcut=", (void (lay::Action::*)(const std::string &)) &lay::Action::set_shortcut,
    "@brief Sets the keyboard shortcut\n"
    "@args shortcut\n"
    "\n"
    "@param shortcut The keyboard shortcut (i.e. \"Ctrl+C\")\n"
  ) +
  method ("shortcut", &lay::Action::get_shortcut,
    "@brief Gets the keyboard shortcut\n"
    "@return The keyboard shortcut as a string\n"
  ) +
  method ("default_shortcut=", (void (lay::Action::*)(const std::string &)) &lay::Action::set_default_shortcut,
    "@brief Sets the default keyboard shortcut\n"
    "@args shortcut\n"
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
    "This is the combined visibility from \\is_visible? and \\is_hidden?."
    "\n"
    "This attribute has been introduced in version 0.25.\n"
  ) +
  method ("separator=", &lay::Action::set_separator,
    "@brief Makes an item a separator or not\n"
    "@args separator\n"
    "\n"
    "@param separator true to make the item a separator\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  method ("checkable=", &lay::Action::set_checkable,
    "@brief Make the item(s) checkable or not\n"
    "@args checkable\n"
    "\n"
    "@param checkable true to make the item checkable\n"
  ) +
  method ("enabled=", &lay::Action::set_enabled,
    "@brief Enable or disable the action\n"
    "@args enabled \n"
    "\n"
    "@param enabled true to enable the item\n"
  ) +
  method ("visible=", &lay::Action::set_visible,
    "@brief Show or hide\n"
    "@args visible \n"
    "\n"
    "@param visible true to make the item visible\n"
  ) +
  method ("hidden=", &lay::Action::set_hidden,
    "@brief Sets a value that makes the item hidden always\n"
    "@args hidden\n"
    "See \\is_hidden? for details.\n"
    "\n"
    "This attribute has been introduced in version 0.25\n"
  ) +
  method ("checked=", &lay::Action::set_checked,
    "@brief Check or uncheck\n"
    "@args checked \n"
    "\n"
    "@param checked true to make the item checked\n"
  ) +
  method ("icon=", &lay::Action::set_icon, 
    "@brief Set the icon to the given picture\n"
    "@args file\n"
    "\n"
    "@param file The image file to load for the icon\n"
    "\n"
    "Passing an empty string will reset the icon\n"
  ) +
  method ("icon_text=", &lay::Action::set_icon_text, 
    "@brief Set the icon's text \n"
    "@args icon_text\n"
    "\n"
    "If an icon text is set, this will be used for the text below the icon.\n"
    "If no icon text is set, the normal text will be used for the icon.\n"
    "Passing an empty string will reset the icon's text.\n"
  ) +
  method ("icon_text", &lay::Action::get_icon_text, 
    "@brief Get the icon's text\n"
  ) +
  method ("tool_tip=", &lay::Action::set_tool_tip, 
    "@brief Set the tool tip text \n"
    "@args text\n"
    "\n"
    "The tool tip text is displayed in the tool tip window of the menu entry.\n"
    "This is in particular useful for entries in the tool bar."
    "\n"
    "This method has been added in version 0.22.\n"
  ) +
  method ("tool_tip", &lay::Action::get_tool_tip, 
    "@brief Get the tool tip text\n"
    "\n"
    "This method has been added in version 0.22.\n"
  ) +
  method ("trigger", &lay::Action::trigger,
    "@brief Trigger the action programmatically"
  ),
  "@hide\n"
  "@alias Action\n"
);
  
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
    on_triggered_event ();
  }

  gsi::Callback triggered_cb;
  tl::Event on_triggered_event;
};

Class<ActionStub> decl_Action (decl_ActionBase, "lay", "Action",
  gsi::callback ("triggered", &ActionStub::triggered, &ActionStub::triggered_cb,
    "@brief This method is called if the menu item is selected"
  ) +
  gsi::event ("on_triggered", &ActionStub::on_triggered_event,
    "@brief This event is called if the menu item is selected\n"
    "\n"
    "This event has been introduced in version 0.21.\n"
  ),
  "@brief The abstraction for an action (i.e. used inside menus)\n"
  "\n"
  "Actions act as a generalisation of menu entries. The action provides the appearance of a menu "
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

