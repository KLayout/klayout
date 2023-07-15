
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
#include "layMainWindow.h"
#include "layConfig.h"

#if defined(HAVE_QTBINDINGS)
# include "gsiQtGuiExternals.h"
# include "gsiQtWidgetsExternals.h"  // for Qt5
#else
# define QT_EXTERNAL_BASE(x)
#endif

namespace gsi
{

void enable_edits (lay::MainWindow * /*main_window*/, bool enable)
{
  //  NOTE: this is for backward compatibility - this method only changes
  //  the current view's state.
  if (lay::LayoutView::current ()) {
    lay::LayoutView::current ()->enable_edits (enable);
  }
}

//  NOTE: match this with the cm_method_decl's below
static const char *cm_symbols[] = {
  "cm_reset_window_state",
  "cm_select_all",
  "cm_unselect_all",
  "cm_undo",
  "cm_redo",
  "cm_delete",
  "cm_show_properties",
  "cm_copy",
  "cm_paste",
  "cm_cut",
  "cm_zoom_fit_sel",
  "cm_zoom_fit",
  "cm_zoom_in",
  "cm_zoom_out",
  "cm_pan_up",
  "cm_pan_down",
  "cm_pan_left",
  "cm_pan_right",
  "cm_save_session",
  "cm_restore_session",
  "cm_setup",
  "cm_save_as",
  "cm_save",
  "cm_save_all",
  "cm_reload",
  "cm_close",
  "cm_close_all",
  "cm_clone",
  "cm_layout_props",
  "cm_inc_max_hier",
  "cm_dec_max_hier",
  "cm_max_hier",
  "cm_max_hier_0",
  "cm_max_hier_1",
  "cm_prev_display_state|#cm_last_display_state",
  "cm_next_display_state",
  "cm_cancel",
  "cm_redraw",
  "cm_screenshot",
  "cm_screenshot_to_clipboard",
  "cm_save_layer_props",
  "cm_load_layer_props",
  "cm_save_bookmarks",
  "cm_load_bookmarks",
  "cm_select_cell",
  "cm_select_current_cell",
  "cm_print",
  "cm_exit",
  "cm_view_log",
  "cm_bookmark_view",
  "cm_manage_bookmarks",
  "cm_macro_editor",
  "cm_goto_position",
  "cm_help_about",
  "cm_technologies",
  "cm_packages",
  "cm_open_too",
  "cm_open_new_view",
  "cm_open",
  "cm_pull_in",
  "cm_reader_options",
  "cm_new_layout",
  "cm_new_panel",
  "cm_adjust_origin",
  "cm_new_cell",
  "cm_new_layer",
  "cm_clear_layer",
  "cm_delete_layer",
  "cm_edit_layer",
  "cm_copy_layer",
  "cm_sel_flip_x",
  "cm_sel_flip_y",
  "cm_sel_rot_cw",
  "cm_sel_rot_ccw",
  "cm_sel_free_rot",
  "cm_sel_scale",
  "cm_sel_move",
  "cm_sel_move_to",
  "cm_lv_new_tab",
  "cm_lv_remove_tab",
  "cm_lv_rename_tab",
  "cm_lv_hide",
  "cm_lv_hide_all",
  "cm_lv_show",
  "cm_lv_show_all",
  "cm_lv_show_only",
  "cm_lv_rename",
  "cm_lv_select_all",
  "cm_lv_delete",
  "cm_lv_insert",
  "cm_lv_group",
  "cm_lv_ungroup",
  "cm_lv_source",
  "cm_lv_sort_by_name",
  "cm_lv_sort_by_ild",
  "cm_lv_sort_by_idl",
  "cm_lv_sort_by_ldi",
  "cm_lv_sort_by_dli",
  "cm_lv_regroup_by_index",
  "cm_lv_regroup_by_datatype",
  "cm_lv_regroup_by_layer",
  "cm_lv_regroup_flatten",
  "cm_lv_expand_all",
  "cm_lv_add_missing",
  "cm_lv_remove_unused",
  "cm_cell_delete",
  "cm_cell_rename",
  "cm_cell_copy",
  "cm_cell_cut",
  "cm_cell_paste",
  "cm_cell_select",
  "cm_open_current_cell",
  "cm_save_current_cell_as",
  "cm_cell_hide",
  "cm_cell_flatten",
  "cm_cell_show",
  "cm_cell_show_all",
  "cm_navigator_close",
  "cm_navigator_freeze"
};

template <unsigned int NUM>
void call_cm_method (lay::MainWindow *mw)
{
  tl_assert (NUM < sizeof (cm_symbols) / sizeof (cm_symbols [0]));
  mw->menu_activated (cm_symbols [NUM]);
}

template <unsigned int NUM>
gsi::Methods cm_method_decl ()
{
  tl_assert (NUM < sizeof (cm_symbols) / sizeof (cm_symbols [0]));
  return gsi::method_ext (std::string ("#") + cm_symbols [NUM], &call_cm_method<NUM>,
    std::string ("@brief '") + cm_symbols[NUM] + "' action.\n"
      "This method is deprecated in version 0.27.\n"
      "Use \"call_menu('" + std::string (cm_symbols[NUM]) + "')\" instead.");
}

//  NOTE: this avoids an issue with binding: C++ (at least clang
//  will resolve lay::MainWindow::menu to lay::Dispatcher::menu,
//  the method declaration will be based on "lay::Dispatcher", and
//  calling this on a MainWindow object fails, because Dispatcher
//  is not the first base class.
static lay::AbstractMenu *menu (lay::MainWindow *mw)
{
  return mw->dispatcher ()->menu ();
}

static void clear_config (lay::MainWindow *mw)
{
  mw->dispatcher ()->clear_config ();
}

static bool write_config (lay::MainWindow *mw, const std::string &config_file)
{
  return mw->dispatcher ()->write_config (config_file);
}

static bool read_config (lay::MainWindow *mw, const std::string &config_file)
{
  return mw->dispatcher ()->read_config (config_file);
}

static tl::Variant get_config (lay::MainWindow *mw, const std::string &name)
{
  std::string value;
  if (mw->dispatcher ()->config_get (name, value)) {
    return tl::Variant (value);
  } else {
    return tl::Variant ();
  }
}

static void set_config (lay::MainWindow *mw, const std::string &name, const std::string &value)
{
  mw->dispatcher ()->config_set (name, value);
}

static std::vector<std::string>
get_config_names (lay::MainWindow *mw)
{
  std::vector<std::string> names;
  mw->dispatcher ()->get_config_names (names);
  return names;
}

static void
config_end (lay::MainWindow *mw)
{
  mw->dispatcher ()->config_end ();
}

static void
set_key_bindings (lay::MainWindow *mw, const std::map<std::string, std::string> &bindings)
{
  std::map<std::string, std::string> b = mw->menu ()->get_shortcuts (false);
  std::map<std::string, std::string> b_def = mw->menu ()->get_shortcuts (true);

  for (std::map<std::string, std::string>::const_iterator i = bindings.begin (); i != bindings.end (); ++i) {
    b[i->first] = i->second;
  }

  //  cfg_key_bindings needs a special notation: lay::Action::no_shortcut () to force "none" instead of default
  //  and and empty string to restore default.
  for (std::map<std::string, std::string>::iterator i = b.begin (); i != b.end (); ++i) {
    std::map<std::string, std::string>::const_iterator j = b_def.find (i->first);
    if (j != b_def.end ()) {
      if (i->second != j->second) {
        if (i->second.empty ()) {
          i->second = lay::Action::no_shortcut ();
        }
      } else {
        i->second.clear ();
      }
    }
  }

  std::vector<std::pair<std::string, std::string> > bv (b.begin (), b.end ());
  mw->dispatcher ()->config_set (lay::cfg_key_bindings, lay::pack_key_binding (bv));
}

static std::map<std::string, std::string>
get_key_bindings (lay::MainWindow *mw)
{
  return mw->menu ()->get_shortcuts (false);
}

static std::map<std::string, std::string>
get_default_key_bindings (lay::MainWindow *mw)
{
  return mw->menu ()->get_shortcuts (true);
}


static std::map<std::string, bool>
get_menu_items_hidden (lay::MainWindow *mw)
{
  std::map<std::string, std::string> kb = get_key_bindings (mw);
  std::map<std::string, bool> h;

  if (mw->dispatcher ()->menu ()) {
    for (std::map<std::string, std::string>::const_iterator i = kb.begin (); i != kb.end (); ++i) {
      lay::Action *a = mw->dispatcher ()->menu ()->action (i->first);
      if (a) {
        h.insert (std::make_pair (i->first, a->is_hidden ()));
      }
    }
  }

  return h;
}

static std::map<std::string, bool>
get_default_menu_items_hidden (lay::MainWindow *mw)
{
  std::map<std::string, std::string> kb = get_key_bindings (mw);

  //  currently, all menu items are visible by default
  std::map<std::string, bool> h;
  for (std::map<std::string, std::string>::const_iterator i = kb.begin (); i != kb.end (); ++i) {
    h.insert (std::make_pair (i->first, false));
  }

  return h;
}

static void
set_menu_items_hidden (lay::MainWindow *mw, const std::map<std::string, bool> &hidden)
{
  std::map<std::string, bool> h = get_menu_items_hidden (mw);
  for (std::map<std::string, bool>::const_iterator i = hidden.begin (); i != hidden.end (); ++i) {
    h[i->first] = i->second;
  }

  std::vector<std::pair<std::string, bool> > hv;
  hv.insert (hv.end (), h.begin (), h.end ());
  mw->dispatcher ()->config_set (lay::cfg_menu_items_hidden, lay::pack_menu_items_hidden (hv));
}

Class<lay::MainWindow> decl_MainWindow (QT_EXTERNAL_BASE (QMainWindow) "lay", "MainWindow",

  //  Dispatcher interface and convenience functions
  method ("dispatcher", &lay::MainWindow::dispatcher,
    "@brief Gets the dispatcher interface (the plugin root configuration space)\n"
    "This method has been introduced in version 0.27."
  ) +
  method_ext ("clear_config", &clear_config,
    "@brief Clears the configuration parameters\n"
    "This method is provided for using MainWindow without an Application object. "
    "It's a convience method which is equivalent to 'dispatcher().clear_config()'. See \\Dispatcher#clear_config for details.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  method_ext ("write_config", &write_config, gsi::arg ("file_name"),
    "@brief Writes configuration to a file\n"
    "This method is provided for using MainWindow without an Application object. "
    "It's a convience method which is equivalent to 'dispatcher().write_config(...)'. See \\Dispatcher#write_config for details.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  method_ext ("read_config", &read_config, gsi::arg ("file_name"),
    "@brief Reads the configuration from a file\n"
    "This method is provided for using MainWindow without an Application object. "
    "It's a convience method which is equivalent to 'dispatcher().read_config(...)'. See \\Dispatcher#read_config for details.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  method_ext ("get_config", &get_config, gsi::arg ("name"),
    "@brief Gets the value of a local configuration parameter\n"
    "This method is provided for using MainWindow without an Application object. "
    "It's a convience method which is equivalent to 'dispatcher().get_config(...)'. See \\Dispatcher#get_config for details.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  method_ext ("set_config", &set_config, gsi::arg ("name"), gsi::arg ("value"),
    "@brief Set a local configuration parameter with the given name to the given value\n"
    "This method is provided for using MainWindow without an Application object. "
    "It's a convience method which is equivalent to 'dispatcher().set_config(...)'. See \\Dispatcher#set_config for details.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  method_ext ("get_config_names", &get_config_names,
    "@brief Gets the configuration parameter names\n"
    "This method is provided for using MainWindow without an Application object. "
    "It's a convience method which is equivalent to 'dispatcher().get_config_names(...)'. See \\Dispatcher#get_config_names for details.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  method_ext ("commit_config", &config_end,
    "@brief Commits the configuration settings\n"
    "This method is provided for using MainWindow without an Application object. "
    "It's a convience method which is equivalent to 'dispatcher().commit_config(...)'. See \\Dispatcher#commit_config for details.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +

  //  key binding configuration
  gsi::method_ext ("get_key_bindings", &get_key_bindings,
    "@brief Gets the current key bindings\n"
    "This method returns a hash with the key binding vs. menu item path.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method_ext ("get_default_key_bindings", &get_default_key_bindings,
    "@brief Gets the default key bindings\n"
    "This method returns a hash with the default key binding vs. menu item path.\n"
    "You can use this hash with \\set_key_bindings to reset all key bindings to the default ones.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method_ext ("set_key_bindings", &set_key_bindings, gsi::arg ("bindings"),
    "@brief Sets key bindings.\n"
    "Sets the given key bindings. "
    "Pass a hash listing the key bindings per menu item paths. Key strings follow the usual notation, e.g. 'Ctrl+A', 'Shift+X' or just 'F2'.\n"
    "Use an empty value to remove a key binding from a menu entry.\n"
    "\n"
    "\\get_key_bindings will give you the current key bindings, \\get_default_key_bindings will give you the default ones.\n"
    "\n"
    "Examples:\n"
    "\n"
    "@code\n"
    "# reset all key bindings to default:\n"
    "mw = RBA::MainWindow.instance()\n"
    "mw.set_key_bindings(mw.get_default_key_bindings())\n"
    "\n"
    "# disable key binding for 'copy':\n"
    "RBA::MainWindow.instance.set_key_bindings({ \"edit_menu.copy\" => \"\" })\n"
    "\n"
    "# configure 'copy' to use Shift+K and 'cut' to use Ctrl+K:\n"
    "RBA::MainWindow.instance.set_key_bindings({ \"edit_menu.copy\" => \"Shift+K\", \"edit_menu.cut\" => \"Ctrl+K\" })\n"
    "@/code\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method_ext ("get_menu_items_hidden", &get_menu_items_hidden,
    "@brief Gets the flags indicating whether menu items are hidden\n"
    "This method returns a hash with the hidden flag vs. menu item path.\n"
    "You can use this hash with \\set_menu_items_hidden.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method_ext ("get_default_menu_items_hidden", &get_default_menu_items_hidden,
    "@brief Gets the flags indicating whether menu items are hidden by default\n"
    "You can use this hash with \\set_menu_items_hidden to restore the visibility of all menu items.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method_ext ("set_menu_items_hidden", &set_menu_items_hidden,
    "@brief sets the flags indicating whether menu items are hidden\n"
    "This method allows hiding certain menu items. It takes a hash with hidden flags vs. menu item paths. "
    "\n"
    "Examples:\n"
    "\n"
    "@code\n"
    "# show all menu items:\n"
    "mw = RBA::MainWindow.instance()\n"
    "mw.set_menu_items_hidden(mw.get_default_menu_items_hidden())\n"
    "\n"
    "# hide the 'copy' entry from the 'Edit' menu:\n"
    "RBA::MainWindow.instance().set_menu_items_hidden({ \"edit_menu.copy\" => true })\n"
    "@/code\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +

  //  QMainWindow interface
  gsi::method_ext ("menu", &menu,
    "@brief Returns a reference to the abstract menu\n"
    "\n"
    "@return A reference to an \\AbstractMenu object representing the menu system"
  ) +
  gsi::method ("instance", &lay::MainWindow::instance,
    "@brief Gets application's main window instance\n"
    "\n"
    "This method has been added in version 0.24."
  ) +
  gsi::method ("manager", &lay::MainWindow::manager,
    "@brief Gets the \\Manager object of this window\n"
    "\n"
    "The manager object is responsible to managing the undo/redo stack. Usually this object "
    "is not required. It's more convenient and safer to use the related methods provided by "
    "\\LayoutView (\\LayoutView#transaction, \\LayoutView#commit) and \\MainWindow (such as "
    "\\MainWindow#cm_undo and \\MainWindow#cm_redo).\n"
    "\n"
    "This method has been added in version 0.24."
  ) +
  gsi::method ("message", &lay::MainWindow::message, gsi::arg ("message"), gsi::arg ("time", -1, "infinite"),
    "@brief Displays a message in the status bar\n"
    "\n"
    "@param message The message to display\n"
    "@param time The time how long to display the message in ms. A negative value means 'infinitely'.\n"
    "\n"
    "This given message is shown in the status bar for the given time.\n" 
    "\n"
    "This method has been added in version 0.18. The 'time' parameter was made optional in version 0.28.10."
  ) +
  gsi::method ("resize", (void (lay::MainWindow::*)(int, int)) &lay::MainWindow::resize, gsi::arg ("width"), gsi::arg ("height"),
    "@brief Resizes the window\n"
    "\n"
    "@param width The new width of the window\n"
    "@param height The new width of the window\n"
    "\n"
    "This method resizes the window to the given target size including decoration such as menu bar "
    "and control panels"
  ) +
  //  MainWindow interface
  gsi::method ("grid_micron", &lay::MainWindow::grid_micron,
    "@brief Gets the global grid in micron\n"
    "\n"
    "@return The global grid in micron\n"
    "\n"
    "The global grid is used at various places, i.e. for ruler snapping, for grid display etc."
  ) +
  gsi::method ("index_of", &lay::MainWindow::index_of, gsi::arg ("view"),
    "@brief Gets the index of the given view\n"
    "\n"
    "@return The index of the view that was given\n"
    "\n"
    "If the given view is not a view object within the main window, a negative value will be returned.\n"
    "\n"
    "This method has been added in version 0.25.\n"
  ) +
  gsi::method ("create_view", &lay::MainWindow::create_view,
    "@brief Creates a new, empty view\n"
    "\n"
    "@return The index of the view that was created\n"
    "\n"
    "Creates an empty view that can be filled with layouts using the load_layout and create_layout "
    "methods on the view object. Use the \\view method to obtain the view object from the view index."
    "\n"
    "This method has been added in version 0.22.\n"
  ) +
  gsi::method ("create_layout", (lay::CellViewRef (lay::MainWindow::*) (int)) &lay::MainWindow::create_layout, gsi::arg ("mode"),
    "@brief Creates a new, empty layout\n"
    "\n"
    "@param mode An integer value of 0, 1 or 2 that determines how the layout is created\n"
    "@return The cellview of the layout that was created\n"
    "\n"
    "Create the layout in the current view, replacing the current layouts (mode 0), "
    "in a new view (mode 1) or adding it to the current view (mode 2).\n"
    "In mode 1, the new view is made the current one.\n"
    "\n"
    "This version uses the initial technology and associates it with the new layout.\n"
    "\n"
    "Starting with version 0.25, this method returns a cellview object that can be modified to configure the cellview.\n"
  ) +
  gsi::method ("create_layout", (lay::CellViewRef (lay::MainWindow::*) (const std::string &, int)) &lay::MainWindow::create_layout, gsi::arg ("tech"), gsi::arg ("mode"),
    "@brief Creates a new, empty layout with the given technology\n"
    "\n"
    "@param mode An integer value of 0, 1 or 2 that determines how the layout is created\n"
    "@param tech The name of the technology to use for that layout.\n"
    "@return The cellview of the layout that was created\n"
    "\n"
    "Create the layout in the current view, replacing the current layouts (mode 0), "
    "in a new view (mode 1) or adding it to the current view (mode 2).\n"
    "In mode 1, the new view is made the current one.\n"
    "\n"
    "If the technology name is not a valid technology name, the default technology will be used.\n"
    "\n"
    "This version was introduced in version 0.22.\n"
    "Starting with version 0.25, this method returns a cellview object that can be modified to configure the cellview.\n"
  ) +
  gsi::method ("load_layout", (lay::CellViewRef (lay::MainWindow::*) (const std::string &, int)) &lay::MainWindow::load_layout, gsi::arg ("filename"), gsi::arg ("mode", 1),
    "@brief Loads a new layout\n"
    "\n"
    "@param filename The name of the file to load\n"
    "@param mode An integer value of 0, 1 or 2 that determines how the file is loaded\n"
    "@return The cellview into which the layout was loaded\n"
    "\n"
    "Loads the given file into the current view, replacing the current layouts (mode 0), "
    "into a new view (mode 1) or adding the layout to the current view (mode 2).\n"
    "In mode 1, the new view is made the current one.\n"
    "\n"
    "This version will use the initial technology and the default reader options. "
    "Others versions are provided which allow specification of technology and reader options explicitly.\n"
    "\n"
    "Starting with version 0.25, this method returns a cellview object that can be modified to configure the cellview. The 'mode' argument has been made optional in version 0.28.\n"
  ) +
  gsi::method ("load_layout", (lay::CellViewRef (lay::MainWindow::*) (const std::string &, const std::string &, int)) &lay::MainWindow::load_layout, gsi::arg ("filename"), gsi::arg ("tech"), gsi::arg ("mode", 1),
    "@brief Loads a new layout and associate it with the given technology\n"
    "\n"
    "@param filename The name of the file to load\n"
    "@param tech The name of the technology to use for that layout.\n"
    "@param mode An integer value of 0, 1 or 2 that determines how the file is loaded\n"
    "@return The cellview into which the layout was loaded\n"
    "\n"
    "Loads the given file into the current view, replacing the current layouts (mode 0), "
    "into a new view (mode 1) or adding the layout to the current view (mode 2).\n"
    "In mode 1, the new view is made the current one.\n"
    "\n"
    "If the technology name is not a valid technology name, the default technology will be used. The 'mode' argument has been made optional in version 0.28.\n"
    "\n"
    "This version was introduced in version 0.22.\n"
    "Starting with version 0.25, this method returns a cellview object that can be modified to configure the cellview.\n"
  ) +
  gsi::method ("load_layout", (lay::CellViewRef (lay::MainWindow::*) (const std::string &, const db::LoadLayoutOptions &, int)) &lay::MainWindow::load_layout, gsi::arg ("filename"), gsi::arg ("options"), gsi::arg ("mode", 1),
    "@brief Loads a new layout with the given options\n"
    "\n"
    "@param filename The name of the file to load\n"
    "@param options The reader options to use.\n"
    "@param mode An integer value of 0, 1 or 2 that determines how the file is loaded\n"
    "@return The cellview into which the layout was loaded\n"
    "\n"
    "Loads the given file into the current view, replacing the current layouts (mode 0), "
    "into a new view (mode 1) or adding the layout to the current view (mode 2).\n"
    "In mode 1, the new view is made the current one.\n"
    "\n"
    "This version was introduced in version 0.22.\n"
    "Starting with version 0.25, this method returns a cellview object that can be modified to configure the cellview. The 'mode' argument has been made optional in version 0.28.\n"
  ) +
  gsi::method ("load_layout", (lay::CellViewRef (lay::MainWindow::*) (const std::string &, const db::LoadLayoutOptions &, const std::string &, int)) &lay::MainWindow::load_layout, gsi::arg ("filename"), gsi::arg ("options"), gsi::arg ("tech"), gsi::arg ("mode", 1),
    "@brief Loads a new layout with the given options and associate it with the given technology\n"
    "\n"
    "@param filename The name of the file to load\n"
    "@param options The reader options to use.\n"
    "@param tech The name of the technology to use for that layout.\n"
    "@param mode An integer value of 0, 1 or 2 that determines how the file is loaded\n"
    "@return The cellview into which the layout was loaded\n"
    "\n"
    "Loads the given file into the current view, replacing the current layouts (mode 0), "
    "into a new view (mode 1) or adding the layout to the current view (mode 2).\n"
    "In mode 1, the new view is made the current one.\n"
    "\n"
    "If the technology name is not a valid technology name, the default technology will be used.\n"
    "\n"
    "This version was introduced in version 0.22.\n"
    "Starting with version 0.25, this method returns a cellview object that can be modified to configure the cellview. The 'mode' argument has been made optional in version 0.28.\n"
  ) +
  gsi::method ("clone_current_view", &lay::MainWindow::clone_current_view,
    "@brief Clones the current view and make it current\n"
  ) +
  gsi::method ("save_session", &lay::MainWindow::save_session, gsi::arg ("fn"),
    "@brief Saves the session to the given file\n"
    "\n"
    "@param fn The path to the session file\n"
    "\n"
    "The session is saved to the given session file. Any existing layout edits are not automatically saved together with "
    "the session. The session just holds display settings and annotation objects. If layout edits exist, they have to be "
    "saved explicitly in a separate step.\n"
    "\n"
    "This method was added in version 0.18."
  ) +
  gsi::method ("restore_session", &lay::MainWindow::restore_session, gsi::arg ("fn"),
    "@brief Restores a session from the given file\n"
    "\n"
    "@param fn The path to the session file\n"
    "\n"
    "The session stored in the given session file is restored. All existing views are closed and all "
    "layout edits are discarded without notification.\n"
    "\n"
    "This method was added in version 0.18."
  ) +
  gsi::method_ext ("#enable_edits", &enable_edits, gsi::arg ("enable"),
    "@brief Enables or disables editing\n"
    "\n"
    "@param enable Enable edits if set to true\n"
    "\n"
    "Starting from version 0.25, this method enables/disables edits on the current view only. \n"
    "Use LayoutView#enable_edits instead.\n"
  ) +
  gsi::method ("synchronous=|#synchronous", &lay::MainWindow::set_synchronous, gsi::arg ("sync_mode"),
    "@brief Puts the main window into synchronous mode\n"
    "\n"
    "@param sync_mode 'true' if the application should behave synchronously\n"
    "\n"
    "In synchronous mode, an application is allowed to block on redraw. While redrawing, "
    "no user interactions are possible. Although this is not desirable for smooth operation, "
    "it can be beneficial for test or automation purposes, i.e. if a screenshot needs to be "
    "produced once the application has finished drawing."
  ) +
  gsi::method ("close_all", &lay::MainWindow::close_all,
    "@brief Closes all views\n"
    "\n"
    "This method unconditionally closes all views. No dialog will be opened if unsaved edits exist.\n"
    "\n"
    "This method was added in version 0.18."
  ) +
  gsi::method ("close_current_view", &lay::MainWindow::close_current_view,
    "@brief Closes the current view\n"
    "\n"
    "This method does not open a dialog to ask which cell view to close if multiple cells "
    "are opened in the view, but rather closes all cells."
  ) +
  gsi::method ("cancel", &lay::MainWindow::cancel,
    "@brief Cancels current editing operations\n"
    "\n"
    "This method call cancels all current editing operations and restores normal mouse mode."
  ) +
  gsi::method ("redraw", &lay::MainWindow::redraw,
    "@brief Redraws the current view\n"
    "\n"
    "Issues a redraw request to the current view. This usually happens automatically, so this method does not "
    "need to be called in most relevant cases. "
  ) +
  gsi::method ("exit", &lay::MainWindow::exit,
    "@brief Schedules an exit for the application\n"
    "\n"
    "This method does not immediately exit the application but sends an exit request "
    "to the application which will cause a clean shutdown of the GUI. "
  ) +
  gsi::method ("current_view_index=|#select_view", &lay::MainWindow::select_view, gsi::arg ("index"),
    "@brief Selects the view with the given index\n"
    "\n"
    "@param index The index of the view to select (0 is the first)\n"
    "\n"
    "This method will make the view with the given index the current (front) view.\n"
    "\n"
    "This method was renamed from select_view to current_view_index= in version 0.25. The old name is still available, but deprecated."
  ) +
  gsi::method ("current_view_index", &lay::MainWindow::current_view_index,
    "@brief Returns the current view's index\n"
    "\n"
    "@return The index of the current view\n"
    "\n"
    "This method will return the index of the current view."
  ) +
  gsi::method ("current_view", (lay::LayoutView *(lay::MainWindow::*)()) &lay::MainWindow::current_view,
    "@brief Returns a reference to the current view's object\n"
    "\n"
    "@return A reference to a \\LayoutView object representing the current view."
  ) +
  gsi::method ("views", &lay::MainWindow::views,
    "@brief Returns the number of views\n"
    "\n"
    "@return The number of views available so far.\n"
  ) +
  gsi::method ("view", (lay::LayoutView *(lay::MainWindow::*)(int)) &lay::MainWindow::view, gsi::arg ("n"),
    "@brief Returns a reference to a view object by index\n"
    "\n"
    "@return The view object's reference for the view with the given index.\n"
  ) +
  gsi::method ("initial_technology", &lay::MainWindow::initial_technology,
    "@brief Gets the technology used for creating or loading layouts (unless explicitly specified)\n"
    "\n"
    "@return The current initial technology"
    "\n"
    "This method was added in version 0.22."
  ) +
  gsi::method ("initial_technology=", &lay::MainWindow::set_initial_technology, gsi::arg ("tech"),
    "@brief Sets the technology used for creating or loading layouts (unless explicitly specified)\n"
    "\n"
    "Setting the technology will have an effect on the next load_layout or create_layout operation which does not explicitly specify the technology but "
    "might not be reflected correctly in the reader options dialog and changes will be reset when the "
    "application is restarted."
    "\n"
    "@param tech The new initial technology\n"
    "\n"
    "This method was added in version 0.22."
  ) +
  gsi::event ("on_current_view_changed", &lay::MainWindow::current_view_changed_event,
    "@brief An event indicating that the current view has changed\n"
    "\n"
    "This event is triggered after the current view has changed. This happens, if the user switches the layout tab.\n"
    "\n"
    "Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods "
    "(add_current_view_observer/remove_current_view_observer) have been removed in 0.25.\n"
  ) +
  gsi::event ("on_view_created", &lay::MainWindow::view_created_event, gsi::arg ("index"),
    "@brief An event indicating that a new view was created\n"
    "@param index The index of the view that was created\n"
    "\n"
    "This event is triggered after a new view was created. For example, if a layout is loaded into a new panel.\n"
    "\n"
    "Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods "
    "(add_new_view_observer/remove_new_view_observer) have been removed in 0.25.\n"
  ) +
  gsi::event ("on_view_closed", &lay::MainWindow::view_closed_event, gsi::arg ("index"),
    "@brief An event indicating that a view was closed\n"
    "@param index The index of the view that was closed\n"
    "\n"
    "This event is triggered after a view was closed. For example, because the tab was closed.\n"
    "\n"
    "This event has been added in version 0.25.\n"
  ) +
  gsi::event ("on_session_about_to_be_restored", &lay::MainWindow::begin_restore_session,
    "@brief An event indicating that a session is about to be restored\n"
    "\n"
    "This event has been added in version 0.28.8.\n"
  ) +
  gsi::event ("on_session_restored", &lay::MainWindow::end_restore_session,
    "@brief An event indicating that a session was restored\n"
    "\n"
    "This event has been added in version 0.28.8.\n"
  ) +
  gsi::method ("show_macro_editor", &lay::MainWindow::show_macro_editor, gsi::arg ("cat", std::string ()), gsi::arg ("add", false),
    "@brief Shows the macro editor\n"
    "If 'cat' is given, this category will be selected in the category tab. "
    "If 'add' is true, the 'new macro' dialog will be opened.\n"
    "\n"
    "This method has been introduced in version 0.26.\n"
  ) +
  gsi::method ("call_menu", &lay::MainWindow::menu_activated, gsi::arg ("symbol"),
    "@brief Calls the menu item with the provided symbol.\n"
    "To obtain all symbols, use menu_symbols.\n"
    "\n"
    "This method has been introduced in version 0.27 and replaces the previous cm_... methods. "
    "Instead of calling a specific cm_... method, use LayoutView#call_menu with 'cm_...' as the symbol."
  ) +
  gsi::method ("menu_symbols", &lay::MainWindow::menu_symbols,
    "@brief Gets all available menu symbols (see \\call_menu).\n"
    "NOTE: currently this method delivers a superset of all available symbols. Depending on the context, no all symbols may trigger actual functionality.\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  //  backward compatibility (cm_... methods, deprecated)
  cm_method_decl<0> () +
  cm_method_decl<1> () +
  cm_method_decl<2> () +
  cm_method_decl<3> () +
  cm_method_decl<4> () +
  cm_method_decl<5> () +
  cm_method_decl<6> () +
  cm_method_decl<7> () +
  cm_method_decl<8> () +
  cm_method_decl<9> () +
  cm_method_decl<10> () +
  cm_method_decl<11> () +
  cm_method_decl<12> () +
  cm_method_decl<13> () +
  cm_method_decl<14> () +
  cm_method_decl<15> () +
  cm_method_decl<16> () +
  cm_method_decl<17> () +
  cm_method_decl<18> () +
  cm_method_decl<19> () +
  cm_method_decl<20> () +
  cm_method_decl<21> () +
  cm_method_decl<22> () +
  cm_method_decl<23> () +
  cm_method_decl<24> () +
  cm_method_decl<25> () +
  cm_method_decl<26> () +
  cm_method_decl<27> () +
  cm_method_decl<28> () +
  cm_method_decl<29> () +
  cm_method_decl<30> () +
  cm_method_decl<31> () +
  cm_method_decl<32> () +
  cm_method_decl<33> () +
  cm_method_decl<34> () +
  cm_method_decl<35> () +
  cm_method_decl<36> () +
  cm_method_decl<37> () +
  cm_method_decl<38> () +
  cm_method_decl<39> () +
  cm_method_decl<40> () +
  cm_method_decl<41> () +
  cm_method_decl<42> () +
  cm_method_decl<43> () +
  cm_method_decl<44> () +
  cm_method_decl<45> () +
  cm_method_decl<46> () +
  cm_method_decl<47> () +
  cm_method_decl<48> () +
  cm_method_decl<49> () +
  cm_method_decl<50> () +
  cm_method_decl<51> () +
  cm_method_decl<52> () +
  cm_method_decl<53> () +
  cm_method_decl<54> () +
  cm_method_decl<55> () +
  cm_method_decl<56> () +
  cm_method_decl<57> () +
  cm_method_decl<58> () +
  cm_method_decl<59> () +
  cm_method_decl<60> () +
  cm_method_decl<61> () +
  cm_method_decl<62> () +
  cm_method_decl<63> () +
  cm_method_decl<64> () +
  cm_method_decl<65> () +
  cm_method_decl<66> () +
  cm_method_decl<67> () +
  cm_method_decl<68> () +
  cm_method_decl<69> () +
  cm_method_decl<70> () +
  cm_method_decl<71> () +
  cm_method_decl<72> () +
  cm_method_decl<73> () +
  cm_method_decl<74> () +
  cm_method_decl<75> () +
  cm_method_decl<76> () +
  cm_method_decl<77> () +
  cm_method_decl<78> () +
  cm_method_decl<79> () +
  cm_method_decl<80> () +
  cm_method_decl<81> () +
  cm_method_decl<82> () +
  cm_method_decl<83> () +
  cm_method_decl<84> () +
  cm_method_decl<85> () +
  cm_method_decl<86> () +
  cm_method_decl<87> () +
  cm_method_decl<88> () +
  cm_method_decl<89> () +
  cm_method_decl<90> () +
  cm_method_decl<91> () +
  cm_method_decl<92> () +
  cm_method_decl<93> () +
  cm_method_decl<94> () +
  cm_method_decl<95> () +
  cm_method_decl<96> () +
  cm_method_decl<97> () +
  cm_method_decl<98> () +
  cm_method_decl<99> () +
  cm_method_decl<100> () +
  cm_method_decl<101> () +
  cm_method_decl<102> () +
  cm_method_decl<103> () +
  cm_method_decl<104> () +
  cm_method_decl<105> () +
  cm_method_decl<106> () +
  cm_method_decl<107> () +
  cm_method_decl<108> () +
  cm_method_decl<109> () +
  cm_method_decl<110> () +
  cm_method_decl<111> () +
  cm_method_decl<112> () +
  cm_method_decl<113> () +
  cm_method_decl<114> () +
  cm_method_decl<115> () +
  cm_method_decl<116> () +
  cm_method_decl<117> (),
  "@brief The main application window and central controller object\n"
  "\n"
  "This object first is the main window but also the main controller. The main controller "
  "is the port by which access can be gained to all the data objects, view and other aspects "
  "of the program."
);

//  extend lay::LayoutView with a "close" method

static void lv_close (lay::LayoutView *view)
{
  int index = lay::MainWindow::instance ()->index_of (view);
  if (index >= 0) {
    lay::MainWindow::instance ()->close_view (index);
  }
}

static
gsi::ClassExt<lay::LayoutView> ext_layout_view (
  gsi::method_ext ("close", &lv_close,
    "@brief Closes the view\n"
    "\nThis method has been added in version 0.27.\n"
  ),
  ""
);

}
