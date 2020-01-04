
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
#include "layAbstractMenuProvider.h"
#include "layLayoutView.h"
#include "layPlugin.h"
#include "layProgress.h"
#include "layTextProgress.h"
#include "layTechnology.h"
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

namespace Ui {
  class HelpAboutDialog;
}

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

/**
 *  @brief A dialog for showing the "help about" dialog
 */
class LAY_PUBLIC HelpAboutDialog
  : public QDialog
{
public:
  HelpAboutDialog (QWidget *parent); 
  ~HelpAboutDialog ();

private:
  Ui::HelpAboutDialog *mp_ui;
};

class TextProgressDelegate
  : public lay::TextProgress
{
public:
  TextProgressDelegate (MainWindow *mw, int verbosity);

  virtual void update_progress (tl::Progress *progress);
  virtual void show_progress_bar (bool show);
  virtual bool progress_wants_widget () const;
  virtual void progress_add_widget (QWidget *widget);
  virtual QWidget *progress_get_widget () const;
  virtual void progress_remove_widget ();

private:
  MainWindow *mp_mw;
};

class LAY_PUBLIC MainWindow
  : public QMainWindow,
    public lay::Plugin,
    public lay::AbstractMenuProvider
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
  MainWindow (QApplication *app = 0, lay::Plugin *parent_plugin = 0, const char *name = "main_window");

  /** 
   *  @brief Destructor
   */
  ~MainWindow ();

  /**
   *  @brief Implementation of the AbstractMenuProvider interface
   */
  QWidget *menu_parent_widget ()
  {
    return this;
  }

  /**
   *  @brief Get the main menu abstraction object
   */
  AbstractMenu *menu ()
  {
    return mp_menu;
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
   *  @brief Determine whether a given layer properties file is a single-layout file
   *
   *  @return True, if the file contains definitions of a single layout only.
   */
  bool is_single_cv_layer_properties_file (const std::string &fn);

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
   *  @brief Implementation of the PluginRoot interface
   */
  bool configure (const std::string &name, const std::string &value);

  /** 
   *  @brief Implementation of the PluginRoot interface
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
   *  @brief Handles a generic menu request
   */
  void menu_activated (const std::string &symbol);

  /**
   *  @brief Get the action for a slot
   */
  lay::Action &action_for_slot (const char *slot);

  /**
   *  @brief Create a configuration action with the given title, parameter name and value
   *
   *  The action will be owned by the main window but can be deleted to remove the action from the main window.
   */
  lay::Action *create_config_action (const std::string &title, const std::string &cname, const std::string &cvalue);
  
  /**
   *  @brief Create a configuration action with the given parameter name and value
   *
   *  The action will be owned by the main window but can be deleted to remove the action from the main window.
   *  This version is provided for applications, where the title is set later.
   */
  lay::Action *create_config_action (const std::string &cname, const std::string &cvalue);
  
  /**
   *  @brief Register a configuration action with the given name
   */
  void register_config_action (const std::string &name, lay::ConfigureAction *action);
  
  /**
   *  @brief Unregister a configuration action with the given name
   */
  void unregister_config_action (const std::string &name, lay::ConfigureAction *action);
  
  /**
   *  @brief Open a new layout in mode 'mode'
   *
   *  If mode is 2, the layout is opened in the current view in addition to the existing ones.
   *  If mode is 1, the layout is opened in a new view. If mode is 0, the layout is opened in 
   *  the current view deleting the existing ones.
   */
  void open (int mode);

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
   *  @brief Add an entry to the MRU list with the initial technology
   */
  void add_mru (const std::string &fn);

  /**
   *  @brief Add an entry to the MRU list
   */
  void add_mru (const std::string &fn, const std::string &tech);

  /**
   *  @brief Get the technology used for loading or creating layouts
   */
  const std::string &initial_technology () 
  {
    return m_initial_technology;
  }

  /**
   *  @brief Set the initial technology used for loading or creating layouts
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

  /**
   *  @brief Called when one of the built-in modes (i.e. select, move) is selected
   */
  void intrinsic_mode_triggered ();

  void update_action_states ();
  void cancel ();
  void redraw ();
  void exit ();
  void close_current_view ();
  void tab_close_requested (int);
  void enable_all ();
  void disable_all ();
  void open_recent ();
  void view_selected (int index);
  void view_title_changed ();

  //  menu callbacks for main window
  void cm_reset_window_state ();
  void cm_select_all ();
  void cm_unselect_all ();
  void cm_undo ();
  void cm_redo ();
  void cm_delete ();
  void cm_show_properties ();
  void cm_copy ();
  void cm_paste ();
  void cm_paste_interactive ();
  void cm_duplicate ();
  void cm_duplicate_interactive ();
  void cm_cut ();
  void cm_zoom_fit_sel ();
  void cm_zoom_fit ();
  void cm_zoom_in ();
  void cm_zoom_out ();
  void cm_pan_up ();
  void cm_pan_down ();
  void cm_pan_left ();
  void cm_pan_right ();
  void cm_save_session ();
  void cm_restore_session ();
  void cm_setup ();
  void cm_save_as ();
  void cm_save_all ();
  void cm_save ();
  void cm_reload ();
  void cm_close ();
  void cm_close_all ();
  void cm_clone ();
  void cm_layout_props ();
  void cm_layout_stats ();
  void cm_inc_max_hier ();
  void cm_dec_max_hier ();
  void cm_max_hier ();
  void cm_max_hier_0 ();
  void cm_max_hier_1 ();
  void cm_prev_display_state ();
  void cm_next_display_state ();
  void cm_cancel ();
  void cm_redraw ();
  void cm_screenshot ();
  void cm_save_layer_props ();
  void cm_load_layer_props ();
  void cm_save_bookmarks ();
  void cm_load_bookmarks ();
  void cm_select_cell ();
  void cm_select_current_cell ();
  void cm_print ();
  void cm_exit ();
  void cm_view_log ();
  void cm_bookmark_view ();
  void cm_manage_bookmarks ();
  void cm_goto_position ();
  void cm_show_all_tips ();
  void cm_help_about ();
  void cm_help_about_qt ();
  void cm_macro_editor ();
  void cm_packages ();
  void cm_technologies ();
  void cm_open_too ();
  void cm_open_new_view ();
  void cm_open ();
  void cm_pull_in ();
  void cm_reader_options ();
  void cm_writer_options ();
  void cm_new_layout ();
  void cm_new_panel ();
  void cm_adjust_origin ();
  void cm_new_cell ();
  void cm_new_layer ();
  void cm_clear_layer ();
  void cm_delete_layer ();
  void cm_edit_layer ();
  void cm_copy_layer ();
  void cm_lay_convert_to_static ();
  void cm_lay_flip_x ();
  void cm_lay_flip_y ();
  void cm_lay_rot_cw ();
  void cm_lay_rot_ccw ();
  void cm_lay_free_rot ();
  void cm_lay_scale ();
  void cm_lay_move ();
  void cm_sel_flip_x ();
  void cm_sel_flip_y ();
  void cm_sel_rot_cw ();
  void cm_sel_rot_ccw ();
  void cm_sel_free_rot ();
  void cm_sel_scale ();
  void cm_sel_move ();
  void cm_sel_move_to ();
  void cm_sel_move_interactive ();
  void cm_show_assistant ();

  //  forwarded to the current view: layer list context menu
  void cm_lv_new_tab ();
  void cm_lv_remove_tab ();
  void cm_lv_rename_tab ();
  void cm_lv_make_valid ();
  void cm_lv_make_invalid ();
  void cm_lv_hide ();
  void cm_lv_hide_all ();
  void cm_lv_show ();
  void cm_lv_show_all ();
  void cm_lv_show_only ();
  void cm_lv_rename ();
  void cm_lv_select_all ();
  void cm_lv_delete ();
  void cm_lv_insert ();
  void cm_lv_group ();
  void cm_lv_ungroup ();
  void cm_lv_source ();
  void cm_lv_sort_by_name ();
  void cm_lv_sort_by_ild ();
  void cm_lv_sort_by_idl ();
  void cm_lv_sort_by_ldi ();
  void cm_lv_sort_by_dli ();
  void cm_lv_regroup_by_index ();
  void cm_lv_regroup_by_datatype ();
  void cm_lv_regroup_by_layer ();
  void cm_lv_regroup_flatten ();
  void cm_lv_expand_all ();
  void cm_lv_add_missing ();
  void cm_lv_remove_unused ();
  void cm_lv_copy ();
  void cm_lv_cut ();
  void cm_lv_paste ();

  // forwarded to the current view: cell list context menu
  void cm_cell_user_properties ();
  void cm_cell_delete ();
  void cm_cell_replace ();
  void cm_cell_rename ();
  void cm_cell_flatten ();
  void cm_cell_convert_to_static ();
  void cm_cell_copy ();
  void cm_cell_cut ();
  void cm_cell_paste ();
  void cm_cell_select ();
  void cm_open_current_cell ();
  void cm_save_current_cell_as ();
  void cm_cell_hide ();
  void cm_cell_show ();
  void cm_cell_show_all ();

  //  forwarded to the navigator
  void cm_navigator_freeze ();
  void cm_navigator_close ();

  /**
   *  @brief shows the given URL as a non-modal help window
   *  Intended as a connection target for QLabel linkVisisted signals.
   */
  void show_help (const QString &url);

  /**
   *  @brief shows the given URL as a modal help window
   *  Intended as a connection target for QLabel linkVisisted signals.
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

  void file_changed_timer ();
  void file_changed (const QString &path);
  void file_removed (const QString &path);

protected:
  void update_content ();
  void do_update_menu ();
  void do_update_file_menu ();

private:
  friend class PluginRootToMainWindow;

  TextProgressDelegate m_text_progress;

  //  Main menu
  AbstractMenu *mp_menu;
  QTabBar *mp_tab_bar;
  QToolBar *mp_tool_bar;
  QDockWidget *mp_navigator_dock_widget;
  lay::Navigator *mp_navigator;
  QDockWidget *mp_hp_dock_widget, *mp_lp_dock_widget, *mp_libs_dock_widget, *mp_bm_dock_widget;
  ControlWidgetStack *mp_hp_stack, *mp_lp_stack, *mp_libs_stack, *mp_bm_stack;
  bool m_hp_visible, m_lp_visible, m_libs_visible, m_bm_visible, m_navigator_visible, m_layer_toolbox_visible;
  QDockWidget *mp_layer_toolbox_dock_widget;
  lay::LayerToolbox *mp_layer_toolbox;
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
  std::vector <lay::LayoutView *> mp_views;
  int m_open_mode;
  std::vector<std::pair<std::string, std::string> > m_mru;
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
  tl::DeferredMethod<MainWindow> dm_do_update_file_menu;
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
  bool m_default_grids_updated;
  std::vector<lay::Action *> m_default_grid_actions;
  std::vector<std::pair<std::string, std::string> > m_key_bindings;
  double m_new_cell_window_size;
  bool m_new_layout_current_panel;
  std::string m_new_cell_cell_name;
  bool m_synchronized_views;
  bool m_synchronous;
  bool m_busy;
  QApplication *mp_app;
  lay::HelpDialog *mp_assistant;
  std::string m_current_session;
  std::string m_message;
  std::auto_ptr<QPrinter> mp_printer;
  std::vector<QString> m_changed_files;

  std::map<std::string, lay::Action> m_actions_for_slot;
  std::map<std::string, std::vector<lay::ConfigureAction *> > m_configuration_actions;
  tl::shared_collection<lay::ConfigureAction> m_ca_collection;

  //  the object manager (undo/redo mechanism and others)
  db::Manager m_manager;

  lay::ProgressReporter *mp_pr;

  void init_menu ();

  void closeEvent (QCloseEvent *event);
  void resizeEvent (QResizeEvent *event);

  void do_cm_paste (bool interactive);
  void do_cm_duplicate (bool interactive);

  void format_message ();

  int dirty_files (std::string &dirty_files);

  void close_view (int index);
  void interactive_close_view (int index, bool all_cellviews);
  void call_on_current_view (void (lay::LayoutView::*func) (), const std::string &op_desc);
  void current_view_changed ();
  void update_window_title ();
  void update_tab_title (int i);
  void add_view (LayoutView *view);

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
  void apply_key_bindings ();
  void apply_hidden (const std::vector<std::pair <std::string, bool> > &hidden);
};

class LAY_PUBLIC PluginRootToMainWindow
  : public lay::PluginRoot
{
public:
  PluginRootToMainWindow ();

  void attach_to (lay::MainWindow *main_window);

  virtual void plugin_registered (lay::PluginDeclaration *cls);
  virtual void plugin_removed (lay::PluginDeclaration *cls);
  virtual void select_mode (int mode);
  virtual void menu_activated (const std::string &symbol);

private:
  PluginRootToMainWindow (const PluginRootToMainWindow &);
  PluginRootToMainWindow &operator= (const PluginRootToMainWindow &);

  tl::weak_ptr<MainWindow> mp_main_window;
};

}

namespace tl {
  template <> struct type_traits<lay::MainWindow> : public type_traits<void> {
    typedef tl::false_tag has_copy_constructor;
    typedef tl::false_tag has_default_constructor;
  };
}

#endif
