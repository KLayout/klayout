
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

#ifndef HDR_rdbMarkerBrowserPage
#define HDR_rdbMarkerBrowserPage

#include "ui_MarkerBrowserPage.h"
#include "rdbMarkerBrowser.h"
#include "layMargin.h"
#include "tlDeferredExecution.h"
#include "tlColor.h"
#include "dbBox.h"

#include <QFrame>

class QAction;

namespace lay
{
  class LayoutViewBase;
  class DMarker;
  class Dispatcher;
}

namespace rdb
{

class Database;

/**
 *  @brief A marker browser page
 */
class MarkerBrowserPage
  : public QFrame,
    public Ui::MarkerBrowserPage
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  MarkerBrowserPage (QWidget *parent);

  /**
   *  @brief Destructor
   */
  ~MarkerBrowserPage ();

  /**
   *  @brief Sets the plugin root object for this object
   */
  void set_dispatcher (lay::Dispatcher *pr);

  /**
   *  @brief Attach the page to a view
   *
   *  This method can be given a layout view object.
   *  If that pointer is non-null, the browser will attach itself to
   *  the view and provide highlights for the selected markers inside the given cellview.
   */
  void set_view (lay::LayoutViewBase *view, unsigned int cv_index);

  /**
   *  @brief Attach the page to a RDB
   *
   *  To detach the page from any RDB, pass 0 for the RDB pointer.
   */
  void set_rdb (rdb::Database *database);

  /**
   *  @brief Gets a value indicating whether all items in the directory tree are shown
   */
  bool show_all () const
  {
    return m_show_all;
  }

  /**
   *  @brief Sets a value indicating whether all items in the directory tree are shown
   *
   *  If this property is set to false, items with not markers are not shown.
   */
  void show_all (bool f);

  /**
   *  @brief Gets a value indicating whether to list the shapes in the info panel
   */
  bool list_shapes () const
  {
    return m_list_shapes;
  }

  /**
   *  @brief Sets a value indicating whether to list the shapes in the info panel
   *
   *  If this property is set to false, shapes will not be listed in the info panel.
   */
  void list_shapes (bool f);

  /**
   *  @brief Update the contents 
   *
   *  This method must be called when the database has been updated.
   *  Unlike attachment to a RDB, this method will try to keep the current selections, etc.
   */
  void update_content ();

  /**
   *  @brief Update the marker objects
   *
   *  This method should be called if the cellview has changed so the markers can 
   *  be recomputed and shown in the new cell context.
   */
  void update_markers ();

  /**
   *  @brief Set the window type and window dimensions
   */
  void set_window (rdb::window_type window_type, const lay::Margin &window_dim, rdb::context_mode_type context);

  /**
   *  @brief Set the maximum number of markers shown in the marker selection list
   */
  void set_max_marker_count (size_t max_marker_count);

  /**
   *  @brief Set the marker style
   *
   *  @param color The color or an invalid color to take the default color for selection
   *  @param line_width The line width or negative for the default line width
   *  @param vertex_size The vertex size or negative for the default vertex size 
   *  @param halo The halo flag or -1 for default 
   *  @param dither_pattern The dither pattern index of -1 to take the default
   */
  void set_marker_style (tl::Color color, int line_width, int vertex_size, int halo, int dither_pattern);

  /**
   *  @brief Enable or disable updates
   */
  void enable_updates (bool f);

public slots:
  void directory_header_clicked (int section);
  void directory_sorting_changed (int section, Qt::SortOrder order);
  void directory_selection_changed (const QItemSelection &selected, const QItemSelection &deselected);
  void markers_header_clicked (int section);
  void markers_sorting_changed (int section, Qt::SortOrder order);
  void markers_selection_changed (const QItemSelection &selected, const QItemSelection &deselected);
  void markers_current_changed (const QModelIndex &current, const QModelIndex &previous);
  void marker_double_clicked (const QModelIndex &index);
  void dir_up_clicked ();
  void dir_down_clicked ();
  void list_up_clicked ();
  void list_down_clicked ();
  void rerun_button_pressed ();
  void flag_button_clicked ();
  void flag_menu_selected ();
  void important_button_clicked ();
  void waived_button_clicked ();
  void snapshot_button_clicked ();
  void remove_snapshot_button_clicked ();
  void revisit_non_waived ();
  void revisit_important ();
  void revisit_all ();
  void mark_important ();
  void mark_unimportant ();
  void mark_visited ();
  void mark_notvisited ();
  void unwaive_all ();
  void waive ();
  void unwaive ();
  void show_all_clicked ();
  void list_shapes_clicked ();
  void info_anchor_clicked (const QUrl &link);
  void filter_changed ();

private:
  bool m_enable_updates;
  bool m_update_needed;
  rdb::Database *mp_database;
  bool m_show_all;
  bool m_list_shapes;
  QAction *m_show_all_action;
  lay::LayoutViewBase *mp_view;
  unsigned int m_cv_index;
  std::vector<lay::DMarker *> mp_markers;
  db::DBox m_markers_bbox;
  size_t m_num_items;
  bool m_view_changed;
  bool m_recursion_sentinel;
  bool m_in_directory_selection_change;
  rdb::context_mode_type m_context;
  rdb::window_type m_window;
  lay::Margin m_window_dim;
  size_t m_max_marker_count;
  tl::Color m_marker_color;
  int m_marker_line_width;
  int m_marker_vertex_size;
  int m_marker_halo;
  int m_marker_dither_pattern;
  int m_current_flag;
  std::string m_error_text;
  int m_marker_list_sorted_section;
  Qt::SortOrder m_marker_list_sort_order;
  int m_directory_tree_sorted_section;
  Qt::SortOrder m_directory_tree_sort_order;
  lay::Dispatcher *mp_plugin_root;
  tl::DeferredMethod<MarkerBrowserPage> dm_rerun_macro;

  void release_markers ();
  void update_marker_list (int selection_mode);
  bool eventFilter (QObject *watched, QEvent *event);
  bool adv_tree (bool up);
  bool adv_list (bool up);
  void mark_visited (bool visited);
  void do_update_markers ();
  void update_info_text ();
  void rerun_macro ();
};

} // namespace rdb

#endif

#endif  //  defined(HAVE_QT)
