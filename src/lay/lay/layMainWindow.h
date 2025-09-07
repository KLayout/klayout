
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

#ifndef HDR_layMainWindow
#define HDR_layMainWindow

#include "layCommon.h"

#include <QStackedWidget>
#include <QMainWindow>
#include <QPrinter>
#include <QDialog>
#include <QTimer>
#include <QByteArray>

#include <vector>
#include <memory>

#include "dbManager.h"
#include "dbLoadLayoutOptions.h"
#include "layAbstractMenu.h"
#include "layDispatcher.h"
#include "layLayoutView.h"
#include "layPlugin.h"
#include "layProgress.h"
#include "layTextProgress.h"
#include "layTechnology.h"
#include "layTextProgressDelegate.h"
#include "tlException.h"
#include "tlDeferredExecution.h"
#include "tlObjectCollection.h"
#include "tlDeferredExecution.h"
#include "gsi.h"

class QTabBar;
class QToolBar;
class QLabel;
class QAction;
class QMenu;
class QStackedWidget;
class QDockWidget;
class QAction;

namespace lay {

class SettingsForm;
class ViewWidgetStack;
class ControlWidgetStack;
class ProgressReporter;
class ProgressWidget;
class ProgressDialog;
class FileDialog;
class AbstractMenu;
class SaveLayoutAsOptionsDialog;
class SaveLayoutOptionsDialog;
class LoadLayoutOptionsDialog;
class LogViewerDialog;
class CellView;
class Navigator;
class LayerToolbox;
class MainWindow;
class HelpDialog;
class HelpAboutDialog;
class ControlWidgetStack;
class ViewWidgetStack;
class ProgressWidget;

/**
 *  @brief A big main window class
 *
 *  The main window is the core UI feature of the application.
 *  The main window is view container, basic controller, configuration root
 *  and holder of many resources.
 *  The main window is a singleton.
 */
class LAY_PUBLIC MainWindow
  : public QMainWindow,
    public tl::Object,
    public gsi::ObjectBase,
    public lay::DispatcherDelegate
{
Q_OBJECT
public:

  /**
   *  @brief The (only) instance of the main window
   */
  static MainWindow *instance ();

  /**
   *  @brief Constructor
   */
  MainWindow (QApplication *app = 0, const char *name = "main_window", bool undo_enabled = true);

  /**
   *  @brief Destructor
   */
  ~MainWindow ();

  /**
   *  @brief Gets the dispatcher interface
   */
  lay::Dispatcher *dispatcher () const
  {
    return const_cast<lay::Dispatcher *> (&m_dispatcher);
  }

  /**
   *  @brief Shows the window
   *
   *  This is the function that is called after the application has set up the window but
   *  before the window is populated and configured
   */
  void show ();

  /**
   *  @brief Close all views
   */
  void close_all ();

  /**
   *  @brief Create a new view
   *
   *  @return The index of the newly created view
   */
  int create_view ();

  /**
   *  @brief Startup behaviour immediately before the application is executed.
   *
   *  This method will be called shortly the before the application is executed.
   *  It is supposed to perform some startup procedure, i.e. displaying tip-of-the-day
   *  windows.
   */
  void about_to_exec ();

  /**
   *  @brief Display a technology message
   */
  void tech_message (const std::string &s);

  /**
   *  @brief Save the session to a file
   */
  void save_session (const std::string &fn);

  /**
   *  @brief Restore the session from a file
   */
  void restore_session (const std::string &fn);

  /**
   *  @brief Reload the given cellview into the current view
   *
   *  The cellview is given by index in the current view's cellview list.
   */
  void reload_layout (unsigned int cv_index);

  /**
   *  @brief Load a (new) file into the layout
   *
   *  The mode param controls how to open the file:
   *    0 = replace current view, 1 = create new view, 2 = add to current view
   */
  lay::CellViewRef load_layout (const std::string &filename, int mode = 0)
  {
    return load_layout (filename, m_initial_technology, mode);
  }

  /**
   *  @brief Load a (new) file into the layout and associate it with the given technology
   *
   *  The mode param controls how to open the file:
   *    0 = replace current view, 1 = create new view, 2 = add to current view
   */
  lay::CellViewRef load_layout (const std::string &filename, const std::string &technology, int mode = 0)
  {
    return load_layout (filename, db::Technologies::instance ()->technology_by_name (technology)->load_layout_options (), technology, mode);
  }

  /**
   *  @brief Load a (new) file into the layout using the given options
   *
   *  The mode param controls how to open the file:
   *    0 = replace current view, 1 = create new view, 2 = add to current view
   */
  lay::CellViewRef load_layout (const std::string &filename, const db::LoadLayoutOptions &options, int mode = 0)
  {
    return load_layout (filename, options, m_initial_technology, mode);
  }

  /**
   *  @brief Load a (new) file into the layout using the given options and technology
   *
   *  The mode param controls how to open the file:
   *    0 = replace current view, 1 = create new view, 2 = add to current view
   */
  lay::CellViewRef load_layout (const std::string &filename, const db::LoadLayoutOptions &options, const std::string &technology, int mode = 0);

  /**
   *  @brief Create new, empty layout
   *
   *  The mode param controls how to create the layout
   *    0 = replace current view, 1 = create new view, 2 = add to current view
   */
  lay::CellViewRef create_layout (int mode = 0)
  {
    return create_layout (m_initial_technology, mode);
  }

  /**
   *  @brief Create new, empty layout and associate it with the given technology
   *
   *  The mode param controls how to create the layout
   *    0 = replace current view, 1 = create new view, 2 = add to current view
   */
  lay::CellViewRef create_layout (const std::string &technology, int mode = 0);

  /**
   *  @brief Clone the current view
   */
  void clone_current_view ();

  /**
   *  @brief Load a layer definition file
   *
   *  This version will load the layer properties file "as it is". No mapping of cellview index
   *  is performed. Only adding of missing layers is supported.
   *  The file name can be an empty string.
   *
   *  @param fn The name of the .lyp file to load.
   *  @param all_views Apply the .lyp file to all views. If false, load the layer properties for the current view.
   *  @param add_default Add the missing layers for each view.
   */
  void load_layer_properties (const std::string &fn, bool all_views, bool add_default);

  /**
   *  @brief Load a layer definition file for a specific cv
   *
   *  This version will load the layer properties file and apply it to the given cv. The cv_index
   *  parameter will denote the cv index to which to apply the layer properties file. It is assumed
   *  that the .lyp file will contain definitions for a single layout only. This definition is
   *  mapped to the specified cv_index after all definitions for this layout have been removed
   *  from the layer properties list.
   *  "cv_index" can be -1. In that case, the layer properties file will be loaded multiple times,
   *  once for each cv present.
   *
   *  @param fn The name of the .lyp file to load.
   *  @param cv_index See above
   *  @param all_views Apply the .lyp file to all views. If false, load the layer properties for the current view.
   *  @param add_default Add the missing layers for each view.
   */
  void load_layer_properties (const std::string &fn, int cv_index, bool all_views, bool add_default);

  /**
   *  @brief Grid accessor: get the grid value
   *
   *  This is the version delivering a double value in micron.
   */
  double grid_micron () const;

  /**
   *  @brief Hierarchy level selection setter
   */
  void set_hier_levels (std::pair<int, int> l);

  /**
   *  @brief Hierarchy level selection setter
   */
  void set_min_hier_levels (int l)
  {
    set_hier_levels (std::pair<int, int> (l, get_max_hier_levels ()));
  }

  /**
   *  @brief Hierarchy level selection setter
   */
  void set_max_hier_levels (int l)
  {
    set_hier_levels (std::pair<int, int> (get_min_hier_levels (), l));
  }

  /**
   *  @brief Hierarchy level selection getter
   */
  std::pair<int, int> get_hier_levels () const;

  /**
   *  @brief Hierarchy level selection getter
   */
  int get_min_hier_levels () const
  {
    return get_hier_levels ().first;
  }

  /**
   *  @brief Hierarchy level selection getter
   */
  int get_max_hier_levels () const
  {
    return get_hier_levels ().second;
  }

  /**
   *  @brief Return true, if there is a "last" display state
   */
  bool has_prev_display_state ();

  /**
   *  @brief Return true, if there is a "next" display state
   */
  bool has_next_display_state ();

  /**
   *  @brief Get the global manager object
   */
  db::Manager &manager ()
  {
    return m_manager;
  }

  /**
   *  @brief Implementation of the lay::ProgressBar interface: set the flag indicating whether we can cancel the operation
   */
  bool update_progress (tl::Progress *progress);

  /**
   *  @brief Implementation of the lay::ProgressBar interface: Returns a value indicating whether a progress widget is wanted
   */
  bool progress_wants_widget () const;

  /**
   *  @brief Implementation of the lay::ProgressBar interface: adds a progress widget
   */
  void progress_add_widget (QWidget *widget);

  /**
   *  @brief Implementation of the lay::ProgressBar interface: gets the progress widget
   */
  QWidget *progress_get_widget () const;

  /**
   *  @brief Implementation of the lay::ProgressBar interface: removes a progress widget
   */
  void progress_remove_widget ();

  /**
   *  @brief Implementation of the lay::ProgressBar interface: Make the progress widget visible or invisible
   */
  bool show_progress_bar (bool show);

  /**
   *  @brief Implementation of the Plugin interface
   */
  bool configure (const std::string &name, const std::string &value);

  /**
   *  @brief Implementation of the Plugin interface
   */
  void config_finalize ();

  /**
   *  @brief Tell, if exit was called
   *
   *  This method returns true if exit () has been called. This state is never reset.
   *  The main purpose is to detect "exit ()" calls before the event loop is executed.
   */
  bool exited ()
  {
    return m_exited;
  }

  /**
   *  @brief Enter or leave busy state
   *
   *  In busy state, the main window will not try to ask for saving changes etc.
   *  Instead, it will ask whether to shut down the application anyway.
   *  The intention of the busy state is to implement a special behavior while inside a processEvent
   *  loop.
   */
  void enter_busy_mode (bool bm)
  {
    m_busy = bm;
  }

  /**
   *  @brief Returns true if the application is busy
   */
  bool is_busy () const
  {
    return m_busy;
  }

  /**
   *  @brief Enable synchronous mode or disable it
   *
   *  In synchronous mode, the program waits until the drawing has finished.
   *  If this mode is disabled, drawing is done in the background thread.
   *  Synchronous mode is mainly intended for automation purposes.
   */
  void set_synchronous (bool sync_mode);

  /**
   *  @brief Tell the state of the synchronous mode flag
   */
  bool synchronous () const
  {
    return m_synchronous;
  }

  /**
   *  @brief Sets the title string
   *
   *  If empty, the default title will be created. Otherwise this string will
   *  be used. It is subject to expression interpolation.
   */
  void set_title (const std::string &title);

  /**
   *  @brief Gets the title string
   */
  const std::string &title () const
  {
    return m_title;
  }

  /**
   *  @brief Returns true, if the edit functions of the current view are enabled
   */
  bool edits_enabled () const;

  /**
   *  @brief Retrieve index of the given view
   *
   *  @return the current view's index or -1 if no index is available
   */
  int index_of (const lay::LayoutView *view) const;

  /**
   *  @brief Retrieve view with the given index
   */
  lay::LayoutView *view (int index);

  /**
   *  @brief Retrieve current the current view (const mode)
   */
  const lay::LayoutView *view (int index) const;

  /**
   *  @brief Gets the current view
   *
   *  This is just a convience method. It's equivalent to lay::LayoutView::current ().
   */
  lay::LayoutView *current_view () const;

  /**
   *  @brief Gets the index of the current view
   */
  int current_view_index () const;

  /**
   *  @brief Return the number of number of views
   */
  unsigned int views () const
  {
    return (unsigned int) mp_views.size ();
  }

  /**
   *  @brief Select the view with the given index
   */
  void select_view (int index);

  /**
   *  @brief Get the instance of the assistant
   */
  lay::HelpDialog *assistant () const
  {
    return mp_assistant;
  }

  /**
   *  @brief Show the assistant with the given URL
   */
  void show_assistant_url (const std::string &url, bool modal = false);

  /**
   *  @brief Show the assistant with the given topic
   */
  void show_assistant_topic (const std::string &s, bool modal = false);

  /**
   *  @brief Show the macro editor
   *
   *  The category can be given to specify the category to show. If empty, the current category
   *  is shown.
   *  "add" can be true, to create a new macro initially.
   */
  void show_macro_editor (const std::string &cat = std::string (), bool add = false);

  /**
   *  @brief Gets the main window's menu
   */
  AbstractMenu *menu ()
  {
    return m_dispatcher.menu ();
  }

  /**
   *  @brief Handles a generic menu request
   */
  void menu_activated (const std::string &symbol);

  /**
   *  @brief Gets the available menu symbols
   */
  static std::vector<std::string> menu_symbols ();

  /**
   *  @brief For internal use: apply current key bindings
   */
  void apply_key_bindings ();

  /**
   *  @brief For internal use: apply hidden menu flags
   */
  void apply_hidden ();

  /**
   *  @brief Open a new layout in mode 'mode'
   *
   *  If mode is 2, the layout is opened in the current view in addition to the existing ones.
   *  If mode is 1, the layout is opened in a new view. If mode is 0, the layout is opened in
   *  the current view deleting the existing ones.
   */
  void open (int mode);

  /**
   *  @brief Gets a format string with all registered layout formats
   */
  std::string all_layout_file_formats () const;

  /**
   *  @brief Adds an entry to the MRU list with the initial technology
   */
  void add_mru (const std::string &fn);

  /**
   *  @brief Adds an entry to the MRU list
   */
  void add_mru (const std::string &fn, const std::string &tech);

  /**
   *  @brief Adds an entry to a specific MRU list given by the "cfg" configuration option
   */
  void add_to_other_mru (const std::string &fn_rel, const std::string &cfg);

  /**
   *  @brief Gets the technology used for loading or creating layouts
   */
  const std::string &initial_technology ()
  {
    return m_initial_technology;
  }

  /**
   *  @brief Sets the initial technology used for loading or creating layouts
   */
  void set_initial_technology (const std::string &tech)
  {
    m_initial_technology = tech;
  }

  /**
   *  @brief Reimplementation of the dragEnterEvent event handler
   */
  void dragEnterEvent(QDragEnterEvent *event);

  /**
   *  @brief Reimplementation of the dropEvent event handler
   */
  void dropEvent(QDropEvent *event);

  /**
   *  @brief An event indicating that the current view has changed
   *  This event is fired if the current view was changed.
   *  The current view's reference is updated if the event is issued.
   */
  tl::Event current_view_changed_event;

  /**
   *  @brief An event indicating that a view was closed
   *  If a view is closed, this event is triggered. When the signal is sent,
   *  the view still points to the view being closed which is still valid.
   *  The integer parameter will receive the index of the view about to be closed.
   */
  tl::event<int> view_closed_event;

  /**
   *  @brief An event indicating that a new view is created
   *  If a new view is created, this event will be triggered.
   *  The integer parameter will receive the index of the view that was created.
   */
  tl::event<int> view_created_event;

  /**
   *  @brief An event indicating the start of a session restore
   */
  tl::Event begin_restore_session;

  /**
   *  @brief An event indicating the end of a session restore
   */
  tl::Event end_restore_session;

signals:
  void closed ();

public slots:
  /**
   *  @brief Displays the current position
   */
  void current_pos (double x, double y, bool dbu_units);

  /**
   *  @brief Clears the current position
   */
  void clear_current_pos ();

  /**
   *  @brief Displays a status message next to the coordinates
   */
  void message (const std::string &s, int ms);

  /**
   *  @brief Clears the current message
   */
  void clear_message ();

  /**
   *  @brief Selects the given mode
   *
   *  @param The index of the mode to select
   */
  void select_mode (int m);

  void update_action_states ();
  void cancel ();
  void redraw ();
  void exit ();
  void close_current_view ();
  void close_view (int index);
  void close_all_views ();
  void close_all_except_this ();
  void close_all_views_left ();
  void close_all_views_right ();
  void clone ();
  void tab_close_requested (int);
  void open_recent (size_t n);
  void open_recent_session (size_t n);
  void open_recent_layer_properties (size_t n);
  void open_recent_bookmarks (size_t n);
  bool is_available_recent(size_t n);
  bool is_available_recent_session (size_t n);
  bool is_available_recent_layer_properties (size_t n);
  bool is_available_recent_bookmarks (size_t n);
  void view_selected (int index);
  void view_title_changed (lay::LayoutView *view);

  /**
   *  @brief shows the given URL as a non-modal help window
   *  Intended as a connection target for QLabel linkVisited signals.
   */
  void show_help (const QString &url);

  /**
   *  @brief shows the given URL as a modal help window
   *  Intended as a connection target for QLabel linkVisited signals.
   */
  void show_modal_help (const QString &url);

  /**
   *  @brief visibility of one of the dock widgets changed
   */
  void dock_widget_visibility_changed (bool visible);

protected slots:
  void menu_changed ();
  void message_timer ();
  void edits_enabled_changed ();
  void menu_needs_update ();
  void technology_changed ();

  void file_changed_timer ();
  void file_changed (const QString &path);
  void file_removed (const QString &path);

protected:
  void update_content ();
  void do_update_menu ();
  void do_update_grids ();
  void do_update_mru_menus ();
  bool eventFilter (QObject *watched, QEvent *event);

private:
  lay::Dispatcher m_dispatcher;

  TextProgressDelegate m_text_progress;

  //  Main menu
  QTabBar *mp_tab_bar;
  QPoint m_mouse_pos;
  QToolBar *mp_tool_bar;
  QDockWidget *mp_navigator_dock_widget;
  lay::Navigator *mp_navigator;
  QDockWidget *mp_hp_dock_widget, *mp_lp_dock_widget, *mp_libs_dock_widget, *mp_eo_dock_widget, *mp_bm_dock_widget;
  ControlWidgetStack *mp_hp_stack, *mp_lp_stack, *mp_layer_toolbox_stack, *mp_libs_stack, *mp_eo_stack, *mp_bm_stack;
  bool m_hp_visible, m_lp_visible, m_libs_visible, m_eo_visible, m_bm_visible, m_navigator_visible, m_layer_toolbox_visible, m_always_exit_without_saving;
  QDockWidget *mp_layer_toolbox_dock_widget;
  ViewWidgetStack *mp_view_stack;
  lay::FileDialog *mp_bookmarks_fdia;
  lay::FileDialog *mp_session_fdia;
  lay::FileDialog *mp_lprops_fdia;
  lay::FileDialog *mp_screenshot_fdia;
  lay::FileDialog *mp_layout_fdia;
  lay::SaveLayoutAsOptionsDialog *mp_layout_save_as_options;
  lay::SaveLayoutOptionsDialog *mp_layout_save_options;
  lay::LoadLayoutOptionsDialog *mp_layout_load_options;
  lay::LogViewerDialog *mp_log_viewer_dialog;
  int m_mode;
  SettingsForm *mp_setup_form;
  std::vector <lay::LayoutViewWidget *> mp_views;
  int m_open_mode;
  int m_keep_backups;
  std::vector<std::pair<std::string, std::string> > m_mru;
  std::vector<std::string> m_mru_sessions, m_mru_layer_properties, m_mru_bookmarks;
  QStatusBar *mp_status_bar;
  QStackedWidget *mp_main_stack_widget;
  ProgressWidget *mp_progress_widget;
  tl::shared_ptr<ProgressDialog> mp_progress_dialog;
  QFrame *mp_cp_frame;
  QFrame *mp_main_frame;
  QLabel *mp_cpx_label, *mp_cpy_label;
  QLabel *mp_msg_label;
  QLabel *mp_tech_status_label;
  bool m_disable_tab_selected;
  bool m_exited;
  tl::DeferredMethod<MainWindow> dm_do_update_menu;
  tl::DeferredMethod<MainWindow> dm_do_update_grids;
  tl::DeferredMethod<MainWindow> dm_do_update_mru_menus;
  tl::DeferredMethod<MainWindow> dm_exit;
  QTimer m_message_timer;
  QTimer m_file_changed_timer;
  QTimer m_menu_update_timer;
  std::string m_config_window_state;
  QByteArray m_default_window_state;
  QByteArray m_default_window_geometry;

  std::string m_initial_technology;
  double m_grid_micron;
  std::vector<double> m_default_grids;
  double m_default_grid;
  bool m_default_grids_updated;
  std::vector<std::pair<std::string, std::string> > m_key_bindings;
  std::vector<std::pair<std::string, bool> > m_hidden;
  bool m_new_layout_current_panel;
  bool m_synchronized_views;
  bool m_synchronous;
  bool m_busy;
  QApplication *mp_app;
  lay::HelpDialog *mp_assistant;
  std::string m_current_session;
  std::string m_message;
  std::unique_ptr<QPrinter> mp_printer;
  std::vector<QString> m_changed_files;
  std::string m_title;

  //  the object manager (undo/redo mechanism and others)
  db::Manager m_manager;

  lay::ProgressReporter *mp_pr;

  void init_menu ();

  void closeEvent (QCloseEvent *event);
  void resizeEvent (QResizeEvent *event);

  void cm_navigator_freeze ();
  void cm_navigator_close ();
  void cm_view_log ();
  void cm_print ();
  void cm_exit ();
  void cm_reset_window_state ();
  void cm_undo ();
  void cm_redo ();
  void cm_undo_list ();
  void cm_redo_list ();
  void cm_goto_position ();
  void cm_manage_bookmarks ();
  void cm_bookmark_view ();
  void cm_cancel ();
  void cm_save_layer_props ();
  void cm_load_layer_props ();
  void cm_save_session ();
  void cm_restore_session ();
  void cm_save_bookmarks ();
  void cm_load_bookmarks ();
  void cm_screenshot ();
  void cm_screenshot_to_clipboard ();
  void cm_save_current_cell_as ();
  void cm_save ();
  void cm_save_as ();
  void cm_save_all ();
  void cm_setup ();
  void cm_open_too ();
  void cm_open_new_view ();
  void cm_open ();
  void cm_pull_in ();
  void cm_reader_options ();
  void cm_writer_options ();
  void cm_new_panel ();
  void cm_new_layout ();
  void cm_clone ();
  void cm_close_all ();
  void cm_close ();
  void cm_packages ();
  void cm_technologies ();
  void cm_macro_editor ();
  void cm_show_assistant ();
  void cm_show_all_tips ();
  void cm_help_about ();
  void cm_help_about_qt ();

  void format_message ();

  int dirty_files (std::string &dirty_files);

  void load_layer_props_from_file (const std::string &fn);
  void interactive_close_view (int from, int to, bool invert_range, bool all_cellviews);
  void call_on_current_view (void (lay::LayoutView::*func) (), const std::string &op_desc);
  void current_view_changed ();
  void update_editor_options_dock ();
  void update_window_title ();
  void update_tab_title (int i);
  void add_view (LayoutViewWidget *view);

  bool can_close ();
  lay::CellViewRef create_or_load_layout (const std::string *filename, const db::LoadLayoutOptions *options, const std::string &tech, const int mode);
  int do_create_view ();
  void do_save (bool as);
  void do_close ();
  void save_state_to_config ();

  void update_dock_widget_state ();
  void read_dock_widget_state ();

  void plugin_registered (lay::PluginDeclaration *cls);
  void plugin_removed (lay::PluginDeclaration *cls);

  void libraries_changed ();
};

}

#endif
