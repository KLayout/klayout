
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

#if defined(HAVE_QT)

#ifndef HDR_layLayoutViewQt
#define HDR_layLayoutViewQt

#include "layviewCommon.h"

#include "layLayoutViewBase.h"
#include "layLayerProperties.h"
#include "layAbstractMenu.h"
#include "layAnnotationShapes.h"
#include "layDispatcher.h"
#include "layLayoutCanvas.h"
#include "layColorPalette.h"
#include "layStipplePalette.h"
#include "layLineStylePalette.h"
#include "layCellView.h"
#include "layViewOp.h"
#include "layEditable.h"
#include "layPlugin.h"
#include "layDisplayState.h"
#include "layBookmarkList.h"
#include "layEditorOptionsFrame.h"
#include "gsi.h"
#include "tlException.h"
#include "tlEvents.h"
#include "dbInstElement.h"

#include <vector>
#include <map>
#include <set>
#include <list>
#include <string>
#include <memory>

#include <QImage>
#include <QPointer>
#include <QVBoxLayout>

class QSpinBox;

namespace rdb {
  class Database;
}

namespace db {
  class Layout;
  class Manager;
  class SaveLayoutOptions;
  class LayoutToNetlist;
}

namespace lay {

class LayoutViewWidget;
class AbstractMenu;
class LayerControlPanel;
class LayerToolbox;
class HierarchyControlPanel;
class LibrariesView;
class BookmarksView;
class MouseTracker;
class ZoomService;
class SelectionService;
class MoveService;
class Browser;
class ColorButton;
class ConfigureAction;
class EditorOptionsPages;
class PropertiesDialog;

/**
 *  @brief An object connecting child widget signals with methods from LayoutView
 */
class LayoutViewSignalConnector
  : public QObject
{
Q_OBJECT

public:
  LayoutViewSignalConnector (QObject *parent, lay::LayoutView *view);

  typedef lay::LayoutViewBase::cell_path_type cell_path_type;

public slots:
  void active_cellview_changed (int index);
  void active_library_changed (int index);
  void side_panel_destroyed ();
  void current_layer_changed_slot (const lay::LayerPropertiesConstIterator &iter);
  void layer_tab_changed ();
  void layer_order_changed ();
  void select_cell_dispatch (const cell_path_type &path, int cellview_index);
  void min_hier_changed (int i);
  void max_hier_changed (int i);
  void app_terminated ();

  void timer ();

private:
  LayoutView *mp_view;
};

/**
 *  @brief The layout view object
 *
 *  The layout view is responsible for displaying one or a set of layouts.
 *  It is composed of a canvas and controls to control the appearance.
 *  It manages the layer display list, bookmark list etc.
 */
class LAYVIEW_PUBLIC LayoutView
  : public LayoutViewBase
{
public:
  /**
   *  @brief Constructor
   */
  LayoutView (db::Manager *mgr, bool editable, lay::Plugin *plugin_parent, unsigned int options = (unsigned int) LV_Normal);

  /**
   *  @brief Constructor (clone from another view)
   */
  LayoutView (lay::LayoutView *source, db::Manager *mgr, bool editable, lay::Plugin *plugin_parent, unsigned int options = (unsigned int) LV_Normal);

  /**
   *  @brief Destructor
   */
  ~LayoutView ();

  /**
   *  @brief Gets the widget object that view is embedded in
   */
  QWidget *widget ();

  /**
   *  @brief Makes this view the current one
   */
  void set_current ();

  /**
   *  @brief Makes the given view the current one
   *
   *  The argument can be 0 which means there is no current view.
   */
  static void set_current (LayoutView *);

  /**
   *  @brief Gets the current view
   */
  static LayoutView *current ();

  /**
   *  @brief Determine if there is something to copy
   *
   *  This reimplementation of the lay::Editables interface additionally
   *  looks for content providers in the tree views for example.
   */
  virtual bool has_selection ();

  /**
   *  @brief Displays a status message
   */
  void message (const std::string &s = "", int timeout = 10);

  /**
   *  @brief Select a certain mode (by index)
   */
  virtual void mode (int m);

  /**
   *  @brief Gets the current mode
   */
  int mode () const
  {
    return LayoutViewBase::mode ();
  }

  /**
   *  @brief Switches the application's mode
   *
   *  Switches the mode on application level. Use this method to initiate
   *  a mode switch from the view.
   */
  virtual void switch_mode (int m);

  /**
   *  @brief Updates the menu for the given view
   *  If the view is 0, the menu shall be updated to reflect "no view active"
   */
  static void update_menu (lay::LayoutView *view, lay::AbstractMenu &menu);

  /**
   *  @brief Create all plugins
   *
   *  If plugins already exist, they are deleted and created again
   */
  virtual void create_plugins (const lay::PluginDeclaration *except_this = 0);

  /**
   *  @brief Sets the currently active layer by layer properties and cell view index
   *
   *  This method will look up that layer in the layer view tree and select that layer.
   *  This method will also select this layer.
   *
   *  Returns false if the layer is not a valid one.
   */
  bool set_current_layer (unsigned int cv_index, const db::LayerProperties &properties)
  {
    return LayoutViewBase::set_current_layer (cv_index, properties);
  }

  /**
   *  @brief Sets the currently active layer
   *
   *  The active layer is the one that is active in the layer
   *  browser panel. This method will also select this layer.
   */
  virtual void set_current_layer (const lay::LayerPropertiesConstIterator &l);

  /**
   *  @brief Retrieve the index of the currently active layer
   *
   *  The active layer is the one that is active in the layer
   *  browser panel.
   *  This method returns a null iterator, if no layer is active.
   */
  virtual lay::LayerPropertiesConstIterator current_layer () const;

  /**
   *  @brief Return the layers that are selected in the layer browser
   *
   *  Returns an empty list if no layer is selected.
   */
  std::vector<lay::LayerPropertiesConstIterator> selected_layers () const;

  /**
   *  @brief Sets the layers that are selected in the layer browser
   */
  void set_selected_layers (const std::vector<lay::LayerPropertiesConstIterator> &sel);

  /**
   *  @brief Get the index of the active cellview (shown in hierarchy browser)
   */
  int active_cellview_index () const;

  /**
   *  @brief Select a certain cellview for the active one
   */
  virtual void set_active_cellview_index (int index);

  /**
   *  @brief Cell paths of the selected cells
   *
   *  The current cell is the one highlighted in the browser with the focus rectangle. The
   *  current path is returned for the cellview given by cv_index.
   */
  void selected_cells_paths (int cv_index, std::vector<cell_path_type> &paths) const;

  /**
   *  @brief Cell path of the current cell
   *
   *  The current cell is the one highlighted in the browser with the focus rectangle. The
   *  current path is returned for the cellview given by cv_index.
   */
  void current_cell_path (int cv_index, cell_path_type &path) const;

  /**
   *  @brief Cell path of the current cell in the active cellview
   *
   *  This is a convenience function returning the path for the active cellview.
   */
  void current_cell_path (cell_path_type &path) const
  {
    current_cell_path (active_cellview_index (), path);
  }

  /**
   *  @brief Set the path to the current cell
   *
   *  The current cell is the one highlighted in the browser with the focus rectangle. The
   *  cell given by the path is highlighted and scrolled into view.
   */
  virtual void set_current_cell_path (int cv_index, const cell_path_type &path);

  /**
   *  @brief Internal method: check, if the layer tree is and an consistent state.
   *
   *  This method is used by the layer tree model to check, if the tree has been brought into
   *  a consistent state.
   *  HINT: for the layout this is solved more consistently with the "under construction" attribute
   *  of the layout. There is no equivalent object for the layer tree currently.
   */
  bool layer_model_updated ();

  /**
   *  @brief Returns true, if the layer source shall be shown always in the layer properties tree
   */
  virtual bool always_show_source () const
  {
    return m_always_show_source;
  }

  /**
   *  @brief Returns true, if the layer/datatype shall be shown always in the layer properties tree
   */
  virtual bool always_show_ld () const
  {
    return m_always_show_ld;
  }

  /**
   *  @brief Returns true, if the layout index shall be shown always in the layer properties tree
   */
  virtual bool always_show_layout_index () const
  {
    return m_always_show_layout_index;
  }

  /**
   *  @brief Indicates the current position
   */
  virtual void current_pos (double x, double y);

  /**
   *  @brief Asks for a bookmark name and bookmark the current view under this name
   */
  void bookmark_current_view ();

  /**
   *  @brief Show the bookmark management form
   */
  void manage_bookmarks ();

  /**
   *  @brief Open the RDB browser for a given database and associated cv index
   */
  virtual void open_rdb_browser (int rdb_index, int cv_index);

  /**
   *  @brief Open the L2NDB browser for a given database and associated cv index
   */
  virtual void open_l2ndb_browser (int l2ndb_index, int cv_index);

  /**
   *  @brief Gets the editor options pages
   */
  virtual lay::EditorOptionsPages *editor_options_pages ();

  /**
   *  @brief Gets the layer control panel
   */
  virtual lay::LayerControlPanel *control_panel ()
  {
    return mp_control_panel;
  }

  /**
   *  @brief Gets the hierarchy panel
   */
  virtual lay::HierarchyControlPanel *hierarchy_panel ()
  {
    return mp_hierarchy_panel;
  }

  /**
   *  @brief Gets the layer toolbox
   */
  virtual lay::LayerToolbox *layer_toolbox ()
  {
    return mp_toolbox;
  }

  /**
   *  @brief Resizes the view to the given width and height in pixels
   */
  void resize (unsigned int width, unsigned int height)
  {
    LayoutViewBase::resize (width, height);
  }

  /**
   *  @brief Copies to clipboard
   *
   *  This reimplementation of the lay::Editables interface additionally
   *  looks for copy providers in the tree views for example.
   */
  virtual void copy ();

  /**
   *  @brief Cuts to clipboard
   *
   *  This reimplementation of the lay::Editables interface additionally
   *  looks for cut & copy providers in the tree views for example.
   */
  virtual void cut ();

  /**
   *  @brief An event signalling that the view is going to close
   */
  tl::Event close_event;

  /**
   *  @brief An event signalling that the view is going to become visible
   */
  tl::Event show_event;

  /**
   *  @brief An event signalling that the view is going to become invisible
   */
  tl::Event hide_event;

  /**
   *  @brief Store the current state on the "previous states" stack
   */
  void store_state ()
  {
    LayoutViewBase::store_state ();
  }

  /**
   *  @brief Cancels all edit operations, clears the selection and resets the mode to "Select"
   */
  void cancel_esc ()
  {
    LayoutViewBase::cancel_esc ();
  }

  /**
   *  @brief Cancels all edit operations and clears the selection
   */
  void cancel ()
  {
    LayoutViewBase::cancel ();
  }

  /**
   *  @brief Cancels all edit operations but maintains selection
   */
  void cancel_edits ();

  /**
   *  @brief Select all levels of hierarchy available
   */
  void max_hier ()
  {
    LayoutViewBase::max_hier ();
  }

  /**
   *  @brief Stop activities like shape browser and redrawing thread
   */
  void stop ()
  {
    LayoutViewBase::stop ();
  }

  /**
   *  @brief Stop redrawing thread
   */
  void stop_redraw ()
  {
    LayoutViewBase::stop_redraw ();
  }

  /** 
   *  @brief Select last display state
   */
  void prev_display_state ()
  {
    LayoutViewBase::prev_display_state ();
  }

  /** 
   *  @brief Select next display state
   */
  void next_display_state ()
  {
    LayoutViewBase::next_display_state ();
  }

  /**
   *  @brief Ensure the selection is visible 
   */
  void ensure_selection_visible ()
  {
    LayoutViewBase::ensure_selection_visible ();
  }

  //  zoom slots
  void zoom_fit ()
  {
    LayoutViewBase::zoom_fit ();
  }

  void zoom_fit_sel ()
  {
    LayoutViewBase::zoom_fit_sel ();
  }

  void zoom_in ()
  {
    LayoutViewBase::zoom_in ();
  }

  void zoom_out ()
  {
    LayoutViewBase::zoom_out ();
  }

  void pan_left ()
  {
    LayoutViewBase::pan_left ();
  }

  void pan_up ()
  {
    LayoutViewBase::pan_up ();
  }

  void pan_right ()
  {
    LayoutViewBase::pan_right ();
  }

  void pan_down ()
  {
    LayoutViewBase::pan_down ();
  }

  void pan_left_fast ()
  {
    LayoutViewBase::pan_left_fast ();
  }

  void pan_up_fast ()
  {
    LayoutViewBase::pan_up_fast ();
  }

  void pan_right_fast ()
  {
    LayoutViewBase::pan_right_fast ();
  }

  void pan_down_fast ()
  {
    LayoutViewBase::pan_down_fast ();
  }

  //  called by children and owner
  void redraw ()
  {
    LayoutViewBase::redraw ();
  }

  void redraw_layer (unsigned int index)
  {
    LayoutViewBase::redraw_layer (index);
  }

  void redraw_deco_layer ()
  {
    LayoutViewBase::redraw_deco_layer ();
  }

  void redraw_cell_boxes ()
  {
    LayoutViewBase::redraw_cell_boxes ();
  }

  void deactivate_all_browsers ();

  void close ();

private:
  friend class LayoutViewSignalConnector;
  friend class LayoutViewWidget;

  QTimer *mp_timer;
  LayoutViewWidget *mp_widget;
  LayoutViewSignalConnector *mp_connector;
  bool m_activated;
  QFrame *mp_left_frame;
  lay::LayerControlPanel *mp_control_panel;
  lay::HierarchyControlPanel *mp_hierarchy_panel;
  lay::LibrariesView *mp_libraries_view;
  lay::BookmarksView *mp_bookmarks_view;
  lay::LayerToolbox *mp_toolbox;
  QWidget *mp_control_frame, *mp_toolbox_frame, *mp_hierarchy_frame, *mp_libraries_frame, *mp_bookmarks_frame;
  lay::EditorOptionsFrame *mp_editor_options_frame;
  QSpinBox *mp_min_hier_spbx;
  QSpinBox *mp_max_hier_spbx;
  QPointer<lay::PropertiesDialog> mp_properties_dialog;
  bool m_always_show_source;
  bool m_always_show_ld;
  bool m_always_show_layout_index;

  tl::DeferredMethod<lay::LayoutView> dm_setup_editor_option_pages;

  void active_library_changed (int index);
  void side_panel_destroyed (QObject *sender);
  void layer_tab_changed ();
  void layer_order_changed ();
  void min_hier_changed (int i);
  void max_hier_changed (int i);

  bool event_filter (QObject *obj, QEvent *ev, bool &taken);
  QSize size_hint () const;

  void init_ui (db::Manager *manager);
  void do_setup_editor_options_pages ();

  QWidget *layer_control_frame () { return mp_control_frame; }
  QWidget *layer_toolbox_frame () { return mp_toolbox_frame; }
  QWidget *hierarchy_control_frame () { return mp_hierarchy_frame; }
  QWidget *libraries_frame () { return mp_libraries_frame; }
  QWidget *bookmarks_frame () { return mp_bookmarks_frame; }
  QWidget *editor_options_frame () { return mp_editor_options_frame; }

protected:
  /**
   *  @brief Constructor with widget
   */
  LayoutView (db::Manager *mgr, bool editable, lay::Plugin *plugin_parent, LayoutViewWidget *widget, unsigned int options = (unsigned int) LV_Normal);

  /**
   *  @brief Constructor (clone from another view) with widget
   */
  LayoutView (lay::LayoutView *source, db::Manager *mgr, bool editable, lay::Plugin *plugin_parent, LayoutViewWidget *widget, unsigned int options = (unsigned int) LV_Normal);

  void activate ();
  void deactivate ();

  virtual bool configure (const std::string &name, const std::string &value);
  virtual void config_finalize ();

  virtual void finish ();
  virtual tl::Color default_background_color ();
  virtual void do_set_background_color (tl::Color color, tl::Color contrast);
  virtual void do_paste ();
  virtual void begin_layer_updates ();
  virtual void end_layer_updates ();
  virtual void update_content_for_cv (int cv_index);
  virtual void do_set_no_stipples (bool no_stipples);
  virtual void do_set_phase (int phase);
  virtual bool set_hier_levels_basic (std::pair<int, int> l);
  virtual void do_change_active_cellview ();
  virtual bool is_activated () const;
  virtual void bookmarks_changed ();
  virtual void show_properties ();

  //  overrides Editables method to display a message
  void signal_selection_changed ();

  virtual void emit_edits_enabled_changed ();
  virtual void emit_title_changed ();
  virtual void emit_dirty_changed ();
  virtual void emit_layer_order_changed ();

private:
  using LayoutViewBase::ui;

  void activate_editor_option_pages ();
};

/**
 *  @brief Descriptor for a notification inside the layout view
 *
 *  Notifications are popups added at the top of the view to indicate need for reloading for example.
 *  Notifications have a name, a title, optional actions (id, title) and a parameter (e.g. file path to reload).
 *  Actions are mapped to QPushButtons.
 */
class LAYVIEW_PUBLIC LayoutViewNotification
{
public:
  LayoutViewNotification (const std::string &name, const std::string &title, const tl::Variant &parameter = tl::Variant ())
    : m_name (name), m_title (title), m_parameter (parameter)
  {
    //  .. nothing yet ..
  }

  void add_action (const std::string &name, const std::string &title)
  {
    m_actions.push_back (std::make_pair (name, title));
  }

  const std::vector<std::pair<std::string, std::string> > &actions () const
  {
    return m_actions;
  }

  const std::string &name () const
  {
    return m_name;
  }

  const std::string &title () const
  {
    return m_title;
  }

  const tl::Variant &parameter () const
  {
    return m_parameter;
  }

  bool operator<(const LayoutViewNotification &other) const
  {
    if (m_name != other.name ()) {
      return m_name < other.name ();
    }
    return m_parameter < other.parameter ();
  }

  bool operator==(const LayoutViewNotification &other) const
  {
    if (m_name != other.name ()) {
      return false;
    }
    return m_parameter == other.parameter ();
  }

private:
  std::string m_name;
  std::string m_title;
  tl::Variant m_parameter;
  std::vector<std::pair<std::string, std::string> > m_actions;
};

/**
 *  @brief A widget representing a notification
 */
class LAYVIEW_PUBLIC LayoutViewNotificationWidget
  : public QFrame
{
Q_OBJECT

public:
  LayoutViewNotificationWidget (LayoutViewWidget *parent, const LayoutViewNotification *notification);

private slots:
  void action_triggered ();
  void close_triggered ();

private:
  LayoutViewWidget *mp_parent;
  const LayoutViewNotification *mp_notification;
  std::map<QObject *, std::string> m_action_buttons;
};

/**
 *  @brief The layout view widget
 *
 *  This is a LayoutView which actually is a widget. It can be used in a widget tree
 *  but only created if a QApplication is present.
 */
class LAYVIEW_PUBLIC LayoutViewWidget
  : public QFrame, public gsi::ObjectBase
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  LayoutViewWidget (db::Manager *mgr, bool editable, lay::Plugin *plugin_parent, QWidget *parent = 0, unsigned int options = (unsigned int) LayoutView::LV_Normal);

  /**
   *  @brief Constructor (clone from another view)
   */
  LayoutViewWidget (lay::LayoutView *source, db::Manager *mgr, bool editable, lay::Plugin *plugin_parent, QWidget *parent = 0, unsigned int options = (unsigned int) LayoutView::LV_Normal);

  /**
   *  @brief Destructor
   */
  ~LayoutViewWidget ();

  /**
   *  @brief Adds a notification
   */
  void add_notification (const LayoutViewNotification &notificaton);

  /**
   *  @brief Removes a notification
   */
  void remove_notification (const LayoutViewNotification &notificaton);

  /**
   *  @brief Gets the LayoutView embedded into this widget
   */
  lay::LayoutView *view ()
  {
    return mp_view;
  }

  virtual QSize sizeHint () const;
  virtual bool eventFilter(QObject *obj, QEvent *event);
  virtual void showEvent (QShowEvent *);
  virtual void hideEvent (QHideEvent *);

  void emit_title_changed (lay::LayoutView *view) { emit title_changed (view); }
  void emit_dirty_changed (lay::LayoutView *view) { emit dirty_changed (view); }
  void emit_show_message (const std::string &s, int ms) { emit show_message (s, ms); }
  void emit_current_pos_changed (double x, double y, bool dbu_units) { emit current_pos_changed (x, y, dbu_units); }
  void emit_clear_current_pos () { emit clear_current_pos (); }
  void emit_edits_enabled_changed () { emit edits_enabled_changed (); }
  void emit_mode_change (int m) { emit mode_change (m); }
  void emit_current_layer_changed (const lay::LayerPropertiesConstIterator &l) { emit current_layer_changed (l); }
  void emit_menu_needs_update () { emit menu_needs_update (); }
  void emit_layer_order_changed () { emit layer_order_changed (); }

  /**
   *  @brief Gets the container with the layer control panel
   */
  QWidget *layer_control_frame ();

  /**
   *  @brief Gets the container with the layer toolbox
   */
  QWidget *layer_toolbox_frame ();

  /**
   *  @brief Gets the container with the hierarchy control panel
   */
  QWidget *hierarchy_control_frame ();

  /**
   *  @brief Gets the container with the libraries view
   */
  QWidget *libraries_frame ();

  /**
   *  @brief Gets the container with the bookmarks view
   */
  QWidget *bookmarks_frame ();

  /**
   *  @brief Gets the container with the editor options
   */
  QWidget *editor_options_frame ();

signals:
  /**
   *  @brief This signal is emitted when the title changes
   */
  void title_changed (lay::LayoutView *view);

  /**
   *  @brief This signal is emitted when the "dirty" flag changes
   */
  void dirty_changed (lay::LayoutView *view);

  /**
   *  @brief This signal is emitted when the view wants to show a message for the given time (of infinitely for ms == 0)
   */
  void show_message (const std::string &s, int ms);

  /**
   *  @brief This signal is emitted when the view wants to indicate a mouse position change
   */
  void current_pos_changed (double x, double y, bool dbu_units);

  /**
   *  @brief This signal is emitted when the view wants to clear the mouse position
   */
  void clear_current_pos ();

  /**
   *  @brief This signal is sent when the "edits_enabled" state has changed
   */
  void edits_enabled_changed ();

  /**
   *  @brief This signal is sent when the view wants to update the menu
   */
  void menu_needs_update ();

  /**
   *  @brief The view initiated a mode change
   */
  void mode_change (int m);

  /**
   *  @brief The current layer changed
   */
  void current_layer_changed (const lay::LayerPropertiesConstIterator &l);

  /**
   *  @brief The layer order has changed
   */
  void layer_order_changed ();

private:
  friend class LayoutView;
  friend class LayoutViewNotificationWidget;

  void view_deleted (lay::LayoutView *view);
  void notification_action (const LayoutViewNotification &notification, const std::string &action);

  void resizeEvent (QResizeEvent *event);

  struct CompareNotificationPointers
  {
    bool operator() (const LayoutViewNotification *a, const LayoutViewNotification *b) const
    {
      return *a < *b;
    }
  };

  LayoutView *mp_view;
  QVBoxLayout *mp_layout;
  std::list<lay::LayoutViewNotification> m_notifications;
  std::map<const LayoutViewNotification *, QWidget *, CompareNotificationPointers> m_notification_widgets;
};

/**
 *  @brief Gets the container widget for a LayoutViewBase object
 */
inline QWidget *widget_from_view (lay::LayoutViewBase *view_base)
{
  lay::LayoutView *view = dynamic_cast<lay::LayoutView *> (view_base);
  return view ? view->widget () : 0;
}

}

#endif

#endif
