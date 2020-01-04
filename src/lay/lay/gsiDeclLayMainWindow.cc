
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

#include "gsiDecl.h"
#include "gsiSignals.h"
#include "layMainWindow.h"

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

Class<lay::MainWindow> decl_MainWindow (QT_EXTERNAL_BASE (QMainWindow) "lay", "MainWindow",

  //  QMainWindow interface
  gsi::method ("menu", &lay::MainWindow::menu,
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
  gsi::method ("message", &lay::MainWindow::message,
    "@brief Displays a message in the status bar\n"
    "\n"
    "@args message,time\n"
    "@param message The message to display\n"
    "@param time The time how long to display the message in ms\n"
    "\n"
    "This given message is shown in the status bar for the given time.\n" 
    "\n"
    "This method has been added in version 0.18."
  ) +
  gsi::method ("resize", (void (lay::MainWindow::*)(int, int)) &lay::MainWindow::resize,
    "@brief Resizes the window\n"
    "\n"
    "@args width, height\n"
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
  gsi::method ("create_layout", (lay::CellViewRef (lay::MainWindow::*) (int)) &lay::MainWindow::create_layout,
    "@brief Creates a new, empty layout\n"
    "\n"
    "@args mode\n"
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
  gsi::method ("create_layout", (lay::CellViewRef (lay::MainWindow::*) (const std::string &, int)) &lay::MainWindow::create_layout,
    "@brief Creates a new, empty layout with the given technology\n"
    "\n"
    "@args tech, mode\n"
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
  gsi::method ("load_layout", (lay::CellViewRef (lay::MainWindow::*) (const std::string &, int)) &lay::MainWindow::load_layout,
    "@brief Loads a new layout\n"
    "\n"
    "@args filename, mode\n"
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
    "Starting with version 0.25, this method returns a cellview object that can be modified to configure the cellview.\n"
  ) +
  gsi::method ("load_layout", (lay::CellViewRef (lay::MainWindow::*) (const std::string &, const std::string &, int)) &lay::MainWindow::load_layout,
    "@brief Loads a new layout and associate it with the given technology\n"
    "\n"
    "@args filename, tech, mode\n"
    "@param filename The name of the file to load\n"
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
    "Starting with version 0.25, this method returns a cellview object that can be modified to configure the cellview.\n"
  ) +
  gsi::method ("load_layout", (lay::CellViewRef (lay::MainWindow::*) (const std::string &, const db::LoadLayoutOptions &, int)) &lay::MainWindow::load_layout,
    "@brief Loads a new layout with the given options\n"
    "\n"
    "@args filename, options, mode\n"
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
    "Starting with version 0.25, this method returns a cellview object that can be modified to configure the cellview.\n"
  ) +
  gsi::method ("load_layout", (lay::CellViewRef (lay::MainWindow::*) (const std::string &, const db::LoadLayoutOptions &, const std::string &, int)) &lay::MainWindow::load_layout,
    "@brief Loads a new layout with the given options and associate it with the given technology\n"
    "\n"
    "@args filename, options, tech, mode\n"
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
    "Starting with version 0.25, this method returns a cellview object that can be modified to configure the cellview.\n"
  ) +
  gsi::method ("clone_current_view", &lay::MainWindow::clone_current_view,
    "@brief Clones the current view and make it current\n"
  ) +
  gsi::method ("save_session", &lay::MainWindow::save_session,
    "@brief Saves the session to the given file\n"
    "\n"
    "@args fn\n"
    "@param fn The path to the session file\n"
    "\n"
    "The session is saved to the given session file. Any existing layout edits are not automatically saved together with "
    "the session. The session just holds display settings and annotation objects. If layout edits exist, they have to be "
    "saved explicitly in a separate step.\n"
    "\n"
    "This method was added in version 0.18."
  ) +
  gsi::method ("restore_session", &lay::MainWindow::restore_session,
    "@brief Restores a session from the given file\n"
    "\n"
    "@args fn\n"
    "@param fn The path to the session file\n"
    "\n"
    "The session stored in the given session file is restored. All existing views are closed and all "
    "layout edits are discarded without notification.\n"
    "\n"
    "This method was added in version 0.18."
  ) +
  gsi::method_ext ("#enable_edits", &enable_edits,
    "@brief Enables or disables editing\n"
    "\n"
    "@args enable\n"
    "@param enable Enable edits if set to true\n"
    "\n"
    "Starting from version 0.25, this method enables/disables edits on the current view only. \n"
    "Use LayoutView#enable_edits instead.\n"
  ) +
  gsi::method ("synchronous=|#synchroneous", &lay::MainWindow::set_synchronous,
    "@brief Puts the main window into synchronous mode\n"
    "\n"
    "@args sync_mode\n"
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
  gsi::method ("current_view_index=|#select_view", &lay::MainWindow::select_view,
    "@brief Selects the view with the given index\n"
    "\n"
    "@args index\n"
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
  gsi::method ("view", (lay::LayoutView *(lay::MainWindow::*)(int)) &lay::MainWindow::view,
    "@brief Returns a reference to a view object by index\n"
    "@args n\n"
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
  gsi::method ("initial_technology=", &lay::MainWindow::set_initial_technology,
    "@brief Sets the technology used for creating or loading layouts (unless explicitly specified)\n"
    "\n"
    "Setting the technology will have an effect on the next load_layout or create_layout operation which does not explicitly specify the technology but "
    "might not be reflected correctly in the reader options dialog and changes will be reset when the "
    "application is restarted."
    "\n"
    "@args tech\n"
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
  gsi::event ("on_view_created", &lay::MainWindow::view_created_event,
    "@brief An event indicating that a new view was created\n"
    "@args index\n"
    "@param index The index of the view that was created\n"
    "\n"
    "This event is triggered after a new view was created. For example, if a layout is loaded into a new panel.\n"
    "\n"
    "Before version 0.25 this event was based on the observer pattern obsolete now. The corresponding methods "
    "(add_new_view_observer/remove_new_view_observer) have been removed in 0.25.\n"
  ) +
  gsi::event ("on_view_closed", &lay::MainWindow::view_closed_event,
    "@brief An event indicating that a view was closed\n"
    "@args index\n"
    "@param index The index of the view that was closed\n"
    "\n"
    "This event is triggered after a view was closed. For example, because the tab was closed.\n"
    "\n"
    "This event has been added in version 0.25.\n"
  ) +
  gsi::method ("show_macro_editor", &lay::MainWindow::show_macro_editor, gsi::arg ("cat", std::string ()), gsi::arg ("add", false),
    "@brief Shows the macro editor\n"
    "If 'cat' is given, this category will be selected in the category tab. "
    "If 'add' is true, the 'new macro' dialog will be opened.\n"
    "\n"
    "This method has been introduced in version 0.26.\n"
  ) +
  gsi::method ("cm_reset_window_state", &lay::MainWindow::cm_reset_window_state,
    "@brief 'cm_reset_window_state' action (bound to a menu)"
    "\n"
    "This method has been added in version 0.25.\n"
  ) +
  gsi::method ("cm_select_all", &lay::MainWindow::cm_select_all,
    "@brief 'cm_select_all' action (bound to a menu)"
    "\n"
    "This method has been added in version 0.22.\n"
  ) +
  gsi::method ("cm_unselect_all", &lay::MainWindow::cm_unselect_all,
    "@brief 'cm_unselect_all' action (bound to a menu)"
    "\n"
    "This method has been added in version 0.22.\n"
  ) +
  gsi::method ("cm_undo", &lay::MainWindow::cm_undo,
    "@brief 'cm_undo' action (bound to a menu)"
  ) +
  gsi::method ("cm_redo", &lay::MainWindow::cm_redo,
    "@brief 'cm_redo' action (bound to a menu)"
  ) +
  gsi::method ("cm_delete", &lay::MainWindow::cm_delete,
    "@brief 'cm_delete' action (bound to a menu)"
  ) +
  gsi::method ("cm_show_properties", &lay::MainWindow::cm_show_properties,
    "@brief 'cm_show_properties' action (bound to a menu)"
  ) +
  gsi::method ("cm_copy", &lay::MainWindow::cm_copy,
    "@brief 'cm_copy' action (bound to a menu)"
  ) +
  gsi::method ("cm_paste", &lay::MainWindow::cm_paste,
    "@brief 'cm_paste' action (bound to a menu)"
  ) +
  gsi::method ("cm_cut", &lay::MainWindow::cm_cut,
    "@brief 'cm_cut' action (bound to a menu)"
  ) +
  gsi::method ("cm_zoom_fit_sel", &lay::MainWindow::cm_zoom_fit,
    "@brief 'cm_zoom_fit_sel' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_zoom_fit", &lay::MainWindow::cm_zoom_fit,
    "@brief 'cm_zoom_fit' action (bound to a menu)"
  ) +
  gsi::method ("cm_zoom_in", &lay::MainWindow::cm_zoom_in,
    "@brief 'cm_zoom_in' action (bound to a menu)"
  ) +
  gsi::method ("cm_zoom_out", &lay::MainWindow::cm_zoom_out,
    "@brief 'cm_zoom_out' action (bound to a menu)"
  ) +
  gsi::method ("cm_pan_up", &lay::MainWindow::cm_pan_up,
    "@brief 'cm_pan_up' action (bound to a menu)"
  ) +
  gsi::method ("cm_pan_down", &lay::MainWindow::cm_pan_down,
    "@brief 'cm_pan_down' action (bound to a menu)"
  ) +
  gsi::method ("cm_pan_left", &lay::MainWindow::cm_pan_left,
    "@brief 'cm_pan_left' action (bound to a menu)"
  ) +
  gsi::method ("cm_pan_right", &lay::MainWindow::cm_pan_right,
    "@brief 'cm_pan_right' action (bound to a menu)"
  ) +
  gsi::method ("cm_save_session", &lay::MainWindow::cm_save_session,
    "@brief 'cm_save_session' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_restore_session", &lay::MainWindow::cm_restore_session,
    "@brief 'cm_restore_session' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_setup", &lay::MainWindow::cm_setup,
    "@brief 'cm_setup' action (bound to a menu)"
  ) +
  gsi::method ("cm_save_as", &lay::MainWindow::cm_save_as,
    "@brief 'cm_save_as' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_save", &lay::MainWindow::cm_save,
    "@brief 'cm_save' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_save_all", &lay::MainWindow::cm_save_all,
    "@brief 'cm_save_all' action (bound to a menu)\n"
    "This method has been added in version 0.24.\n"
  ) +
  gsi::method ("cm_reload", &lay::MainWindow::cm_reload,
    "@brief 'cm_reload' action (bound to a menu)"
  ) +
  gsi::method ("cm_close", &lay::MainWindow::cm_close,
    "@brief 'cm_close' action (bound to a menu)"
  ) +
  gsi::method ("cm_close_all", &lay::MainWindow::cm_close_all,
    "@brief 'cm_close_all' action (bound to a menu)\n"
    "This method has been added in version 0.24.\n"
  ) +
  gsi::method ("cm_clone", &lay::MainWindow::cm_clone,
    "@brief 'cm_clone' action (bound to a menu)"
  ) +
  gsi::method ("cm_layout_props", &lay::MainWindow::cm_layout_props,
    "@brief 'cm_layout_props' action (bound to a menu)"
  ) +
  gsi::method ("cm_inc_max_hier", &lay::MainWindow::cm_inc_max_hier,
    "@brief 'cm_inc_max_hier' action (bound to a menu)"
  ) +
  gsi::method ("cm_dec_max_hier", &lay::MainWindow::cm_dec_max_hier,
    "@brief 'cm_dec_max_hier' action (bound to a menu)"
  ) +
  gsi::method ("cm_max_hier", &lay::MainWindow::cm_max_hier,
    "@brief 'cm_max_hier' action (bound to a menu)"
  ) +
  gsi::method ("cm_max_hier_0", &lay::MainWindow::cm_max_hier_0,
    "@brief 'cm_max_hier_0' action (bound to a menu)"
  ) +
  gsi::method ("cm_max_hier_1", &lay::MainWindow::cm_max_hier_1,
    "@brief 'cm_max_hier_1' action (bound to a menu)"
  ) +
  gsi::method ("cm_prev_display_state|#cm_last_display_state", &lay::MainWindow::cm_prev_display_state,
    "@brief 'cm_prev_display_state' action (bound to a menu)"
  ) +
  gsi::method ("cm_next_display_state", &lay::MainWindow::cm_next_display_state,
    "@brief 'cm_next_display_state' action (bound to a menu)"
  ) +
  gsi::method ("cm_cancel", &lay::MainWindow::cm_cancel,
    "@brief 'cm_cancel' action (bound to a menu)"
  ) +
  gsi::method ("cm_redraw", &lay::MainWindow::cm_redraw,
    "@brief 'cm_redraw' action (bound to a menu)"
  ) +
  gsi::method ("cm_screenshot", &lay::MainWindow::cm_screenshot,
    "@brief 'cm_screenshot' action (bound to a menu)"
  ) +
  gsi::method ("cm_save_layer_props", &lay::MainWindow::cm_save_layer_props,
    "@brief 'cm_save_layer_props' action (bound to a menu)"
  ) +
  gsi::method ("cm_load_layer_props", &lay::MainWindow::cm_load_layer_props,
    "@brief 'cm_load_layer_props' action (bound to a menu)"
  ) +
  gsi::method ("cm_save_bookmarks", &lay::MainWindow::cm_save_bookmarks,
    "@brief 'cm_save_bookmarks' action (bound to a menu)"
  ) +
  gsi::method ("cm_load_bookmarks", &lay::MainWindow::cm_load_bookmarks,
    "@brief 'cm_load_bookmarks' action (bound to a menu)"
  ) +
  gsi::method ("cm_select_cell", &lay::MainWindow::cm_select_cell,
    "@brief 'cm_select_cell' action (bound to a menu)"
  ) +
  gsi::method ("cm_select_current_cell", &lay::MainWindow::cm_select_current_cell,
    "@brief 'cm_select_current_cell' action (bound to a menu)"
  ) +
  gsi::method ("cm_print", &lay::MainWindow::cm_print,
    "@brief 'cm_print' action (bound to a menu)\n"
    "This method has been added in version 0.21.13."
  ) +
  gsi::method ("cm_exit", &lay::MainWindow::cm_exit,
    "@brief 'cm_exit' action (bound to a menu)"
  ) +
  gsi::method ("cm_view_log", &lay::MainWindow::cm_view_log,
    "@brief 'cm_view_log' action (bound to a menu)"
    "\nThis method has been added in version 0.20."
  ) +
  gsi::method ("cm_bookmark_view", &lay::MainWindow::cm_bookmark_view,
    "@brief 'cm_bookmark_view' action (bound to a menu)"
  ) +
  gsi::method ("cm_manage_bookmarks", &lay::MainWindow::cm_manage_bookmarks,
    "@brief 'cm_manage_bookmarks' action (bound to a menu)"
  ) +
  gsi::method ("cm_macro_editor", &lay::MainWindow::cm_macro_editor,
    "@brief 'cm_macro_editor' action (bound to a menu)"
  ) +
  gsi::method ("cm_goto_position", &lay::MainWindow::cm_goto_position,
    "@brief 'cm_goto_position' action (bound to a menu)"
  ) +
  gsi::method ("cm_help_about", &lay::MainWindow::cm_help_about,
    "@brief 'cm_help_about' action (bound to a menu)"
  ) +
  gsi::method ("cm_technologies", &lay::MainWindow::cm_technologies,
    "@brief 'cm_technologies' action (bound to a menu)"
    "\nThis method has been added in version 0.22."
  ) +
  gsi::method ("cm_packages", &lay::MainWindow::cm_packages,
    "@brief 'cm_packages' action (bound to a menu)"
    "\nThis method has been added in version 0.25."
  ) +
  gsi::method ("cm_open_too", &lay::MainWindow::cm_open_too,
    "@brief 'cm_open_too' action (bound to a menu)"
  ) +
  gsi::method ("cm_open_new_view", &lay::MainWindow::cm_open_new_view,
    "@brief 'cm_open_new_view' action (bound to a menu)"
  ) +
  gsi::method ("cm_open", &lay::MainWindow::cm_open,
    "@brief 'cm_open' action (bound to a menu)"
  ) +
  gsi::method ("cm_pull_in", &lay::MainWindow::cm_pull_in,
    "@brief 'cm_pull_in' action (bound to a menu)"
    "\nThis method has been added in version 0.20."
  ) +
  gsi::method ("cm_reader_options", &lay::MainWindow::cm_reader_options,
    "@brief 'cm_reader_options' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_new_layout", &lay::MainWindow::cm_new_layout,
    "@brief 'cm_new_layout' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_new_panel", &lay::MainWindow::cm_new_panel,
    "@brief 'cm_new_panel' action (bound to a menu)"
    "\nThis method has been added in version 0.20."
  ) +
  gsi::method ("cm_adjust_origin", &lay::MainWindow::cm_adjust_origin,
    "@brief 'cm_adjust_origin' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_new_cell", &lay::MainWindow::cm_new_cell,
    "@brief 'cm_new_cell' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_new_layer", &lay::MainWindow::cm_new_layer,
    "@brief 'cm_new_layer' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_clear_layer", &lay::MainWindow::cm_clear_layer,
    "@brief 'cm_clear_layer' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_delete_layer", &lay::MainWindow::cm_delete_layer,
    "@brief 'cm_delete_layer' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_edit_layer", &lay::MainWindow::cm_edit_layer,
    "@brief 'cm_edit_layer' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_copy_layer", &lay::MainWindow::cm_copy_layer,
    "@brief 'cm_copy_layer' action (bound to a menu)"
    "\nThis method has been added in version 0.22."
  ) +
  gsi::method ("cm_sel_flip_x", &lay::MainWindow::cm_sel_flip_x,
    "@brief 'cm_sel_flip_x' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_sel_flip_y", &lay::MainWindow::cm_sel_flip_y,
    "@brief 'cm_sel_flip_y' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_sel_rot_cw", &lay::MainWindow::cm_sel_rot_cw,
    "@brief 'cm_sel_rot_cw' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_sel_rot_ccw", &lay::MainWindow::cm_sel_rot_ccw,
    "@brief 'cm_sel_rot_ccw' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_sel_free_rot", &lay::MainWindow::cm_sel_free_rot,
    "@brief 'cm_sel_free_rot' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_sel_scale", &lay::MainWindow::cm_sel_scale,
    "@brief 'cm_sel_scale' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_sel_move", &lay::MainWindow::cm_sel_move,
    "@brief 'cm_sel_move' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_sel_move_to", &lay::MainWindow::cm_sel_move_to,
    "@brief 'cm_sel_move_to' action (bound to a menu)"
    "\nThis method has been added in version 0.24."
  ) +
  gsi::method ("cm_lv_new_tab", &lay::MainWindow::cm_lv_new_tab,
    "@brief 'cm_lv_new_tab' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_remove_tab", &lay::MainWindow::cm_lv_remove_tab,
    "@brief 'cm_lv_remove_tab' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_rename_tab", &lay::MainWindow::cm_lv_rename_tab,
    "@brief 'cm_lv_rename_tab' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_hide", &lay::MainWindow::cm_lv_hide,
    "@brief 'cm_lv_hide' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_hide_all", &lay::MainWindow::cm_lv_hide_all,
    "@brief 'cm_lv_hide_all' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_show", &lay::MainWindow::cm_lv_show,
    "@brief 'cm_lv_show' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_show_all", &lay::MainWindow::cm_lv_show_all,
    "@brief 'cm_lv_show_all' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_show_only", &lay::MainWindow::cm_lv_show_only,
    "@brief 'cm_lv_show_only' action (bound to a menu)"
    "\nThis method has been added in version 0.20."
  ) +
  gsi::method ("cm_lv_rename", &lay::MainWindow::cm_lv_rename,
    "@brief 'cm_lv_rename' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_select_all", &lay::MainWindow::cm_lv_select_all,
    "@brief 'cm_lv_select_all' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_delete", &lay::MainWindow::cm_lv_delete,
    "@brief 'cm_lv_delete' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_insert", &lay::MainWindow::cm_lv_insert,
    "@brief 'cm_lv_insert' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_group", &lay::MainWindow::cm_lv_group,
    "@brief 'cm_lv_group' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_ungroup", &lay::MainWindow::cm_lv_ungroup,
    "@brief 'cm_lv_ungroup' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_source", &lay::MainWindow::cm_lv_source,
    "@brief 'cm_lv_source' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_sort_by_name", &lay::MainWindow::cm_lv_sort_by_name,
    "@brief 'cm_lv_sort_by_name' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_sort_by_ild", &lay::MainWindow::cm_lv_sort_by_ild,
    "@brief 'cm_lv_sort_by_ild' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_sort_by_idl", &lay::MainWindow::cm_lv_sort_by_idl,
    "@brief 'cm_lv_sort_by_idl' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_sort_by_ldi", &lay::MainWindow::cm_lv_sort_by_ldi,
    "@brief 'cm_lv_sort_by_ldi' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_sort_by_dli", &lay::MainWindow::cm_lv_sort_by_dli,
    "@brief 'cm_lv_sort_by_dli' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_regroup_by_index", &lay::MainWindow::cm_lv_regroup_by_index,
    "@brief 'cm_lv_regroup_by_index' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_regroup_by_datatype", &lay::MainWindow::cm_lv_regroup_by_datatype,
    "@brief 'cm_lv_regroup_by_datatype' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_regroup_by_layer", &lay::MainWindow::cm_lv_regroup_by_layer,
    "@brief 'cm_lv_regroup_by_layer' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_regroup_flatten", &lay::MainWindow::cm_lv_regroup_flatten,
    "@brief 'cm_lv_regroup_flatten' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_expand_all", &lay::MainWindow::cm_lv_expand_all,
    "@brief 'cm_lv_expand_all' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_add_missing", &lay::MainWindow::cm_lv_add_missing,
    "@brief 'cm_lv_add_missing' action (bound to a menu)"
  ) +
  gsi::method ("cm_lv_remove_unused", &lay::MainWindow::cm_lv_remove_unused,
    "@brief 'cm_lv_remove_unused' action (bound to a menu)"
  ) +
  gsi::method ("cm_cell_delete", &lay::MainWindow::cm_cell_delete,
    "@brief 'cm_cell_delete' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_cell_rename", &lay::MainWindow::cm_cell_rename,
    "@brief 'cm_cell_rename' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_cell_copy", &lay::MainWindow::cm_cell_copy,
    "@brief 'cm_cell_copy' action (bound to a menu)"
    "\nThis method has been added in version 0.20."
  ) +
  gsi::method ("cm_cell_cut", &lay::MainWindow::cm_cell_cut,
    "@brief 'cm_cell_cut' action (bound to a menu)"
    "\nThis method has been added in version 0.20."
  ) +
  gsi::method ("cm_cell_paste", &lay::MainWindow::cm_cell_paste,
    "@brief 'cm_cell_paste' action (bound to a menu)"
    "\nThis method has been added in version 0.20."
  ) +
  gsi::method ("cm_cell_select", &lay::MainWindow::cm_cell_select,
    "@brief 'cm_cell_select' action (bound to a menu)"
  ) +
  gsi::method ("cm_open_current_cell", &lay::MainWindow::cm_open_current_cell,
    "@brief 'cm_open_current_cell' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_save_current_cell_as", &lay::MainWindow::cm_save_current_cell_as,
    "@brief 'cm_save_current_cell_as' action (bound to a menu)"
    "\nThis method has been added in version 0.18."
  ) +
  gsi::method ("cm_cell_hide", &lay::MainWindow::cm_cell_hide,
    "@brief 'cm_cell_hide' action (bound to a menu)"
  ) +
  gsi::method ("cm_cell_flatten", &lay::MainWindow::cm_cell_flatten,
    "@brief 'cm_cell_flatten' action (bound to a menu)"
  ) +
  gsi::method ("cm_cell_show", &lay::MainWindow::cm_cell_show,
    "@brief 'cm_cell_show' action (bound to a menu)"
  ) +
  gsi::method ("cm_cell_show_all", &lay::MainWindow::cm_cell_show_all,
    "@brief 'cm_cell_show_all' action (bound to a menu)"
  ) +
  gsi::method ("cm_navigator_close", &lay::MainWindow::cm_navigator_close,
    "@brief 'cm_navigator_close' action (bound to a menu)"
  ) +
  gsi::method ("cm_navigator_freeze", &lay::MainWindow::cm_navigator_freeze,
    "@brief 'cm_navigator_freeze' action (bound to a menu)"
  ),

  "@brief The main application window and central controller object\n"
  "\n"
  "This object first is the main window but also the main controller. The main controller "
  "is the port by which access can be gained to all the data objects, view and other aspects "
  "of the program."
);

}

