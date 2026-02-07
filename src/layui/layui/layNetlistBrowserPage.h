
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#ifndef HDR_layNetlistBrowserPage
#define HDR_layNetlistBrowserPage

#include "ui_NetlistBrowserPage.h"
#include "layNetlistBrowserModel.h"
#include "layNetlistBrowser.h"
#include "layuiCommon.h"
#include "dbLayoutToNetlist.h"
#include "dbLayoutVsSchematic.h"
#include "dbLayoutUtils.h"

#include "tlObject.h"
#include "tlEvents.h"

#include <QFrame>

class QAction;

namespace lay
{

class LayoutViewBase;
class Dispatcher;
class Marker;
class NetInfoDialog;
class LayerPropertiesConstIterator;

/**
 *  @brief The main page of the netlist browser
 */
class NetlistBrowserPage
  : public QFrame,
    public Ui::NetlistBrowserPage,
    public tl::Object
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  NetlistBrowserPage (QWidget *parent);

  /**
   *  @brief Destructor
   */
  ~NetlistBrowserPage ();

  /**
   *  @brief Returns true if netlist mode is selected
   */
  bool is_netlist_mode ();

  /**
   *  @brief Sets the plugin root object for this object
   */
  void set_dispatcher (lay::Dispatcher *pr);

  /**
   *  @brief Attaches the page to a view
   *
   *  This method can be given a layout view object.
   *  If that pointer is non-null, the browser will attach itself to
   *  the view and provide highlights for the selected markers inside the given cellview.
   */
  void set_view (LayoutViewBase *view, int cv_index);

  /**
   *  @brief Attaches the page to a L2N DB
   */
  bool set_db(db::LayoutToNetlist *database);

  /**
   *  @brief Gets the database the page is connected to
   */
  db::LayoutToNetlist *db ()
  {
    return mp_database.get ();
  }

  /**
   *  @brief Selects a net or clears the selection if net == 0
   */
  void select_net (const db::Net *net);

  /**
   *  @brief Selects a netlist object (a circuit, a subcircuit, a net or a device)
   */
  void select_path (const lay::NetlistObjectPath &path)
  {
    select_path (lay::NetlistObjectsPath::from_first (path));
  }

  /**
   *  @brief Selects a netlist object (a circuit, a subcircuit, a net or a device)
   *
   *  This variant allows specifying a paired path using either an object from the first,
   *  the second netlist of both.
   */
  void select_path (const lay::NetlistObjectsPath &path);

  /**
   *  @brief Set the window type and window dimensions
   */
  void set_window (lay::NetlistBrowserConfig::net_window_type window_type, double window_dim);

  /**
   *  @brief Set the maximum number of shapes highlighted for a net
   */
  void set_max_shape_count (size_t max_shape_count);

  /**
   *  @brief Set the highlight style
   *
   *  @param color The color or an invalid color to take the default color for selection
   *  @param line_width The line width or negative for the default line width
   *  @param vertex_size The vertex size or negative for the default vertex size
   *  @param halo The halo flag or -1 for default
   *  @param dither_pattern The dither pattern index of -1 to take the default
   */
  void set_highlight_style (tl::Color color, int line_width, int vertex_size, int halo, int dither_pattern, int marker_intensity, bool use_original_colors, const lay::ColorPalette *auto_colors);

  /**
   *  @brief Gets a value indicating whether all items in the netlist tree are shown (specifically for cross-reference DBs)
   */
  bool show_all () const
  {
    return m_show_all;
  }

  /**
   *  @brief Sets a value indicating whether all items in the netlist tree are shown (specifically for cross-reference DBs)
   *
   *  If this property is set to false, only cross-reference entries with error are shown.
   */
  void show_all (bool f);

  /**
   *  @brief Enable or disable updates
   */
  void enable_updates (bool f);

  /**
   *  @brief Updates net highlights
   */
  void update_highlights ();

  /**
   *  @brief Gets the current object's path
   */
  const lay::NetlistObjectsPath &current_path () const
  {
    return m_current_path;
  }

  /**
   *  @brief Gets the selected nets
   */
  const std::vector<lay::NetlistObjectsPath> &selected_paths () const
  {
    return m_selected_paths;
  }

  /**
   *  @brief An event indicating that the selection has changed
   */
  tl::Event selection_changed_event;

public slots:
  void export_all ();
  void export_selected ();

private slots:
  void show_all_clicked ();
  void info_button_pressed ();
  void rerun_button_pressed ();
  void find_button_pressed ();
  void anchor_clicked (const QString &url);
  void navigate_back ();
  void navigate_forward ();
  void nl_current_index_changed (const QModelIndex &index);
  void sch_current_index_changed (const QModelIndex &index);
  void xref_current_index_changed (const QModelIndex &index);
  void nl_current_tree_index_changed (const QModelIndex &index);
  void sch_current_tree_index_changed (const QModelIndex &index);
  void xref_current_tree_index_changed (const QModelIndex &index);
  void nl_selection_changed ();
  void sch_selection_changed ();
  void xref_selection_changed ();
  void log_selection_changed ();
  void browse_color_for_net ();
  void select_color_for_net ();
  void mode_tab_changed (int);

protected:
  bool eventFilter (QObject *watched, QEvent *event);

private:
  bool m_show_all;
  QAction *m_show_all_action;
  NetColorizer m_colorizer;
  NetlistBrowserConfig::net_window_type m_window;
  double m_window_dim;
  size_t m_max_shape_count;
  int m_marker_line_width;
  int m_marker_vertex_size;
  int m_marker_halo;
  int m_marker_dither_pattern;
  int m_marker_intensity;
  bool m_use_original_colors;
  lay::LayoutViewBase *mp_view;
  unsigned int m_cv_index;
  lay::Dispatcher *mp_plugin_root;
  tl::weak_ptr<db::LayoutToNetlist> mp_database;
  db::LayoutToNetlist *mp_last_db;
  std::vector<QModelIndex> m_history;
  size_t m_history_ptr;
  bool m_signals_enabled;
  std::vector <lay::Marker *> mp_markers;
  bool m_enable_updates;
  bool m_update_needed;
  lay::NetlistObjectsPath m_current_path;
  std::vector<lay::NetlistObjectsPath> m_selected_paths;
  std::vector<std::pair<const db::Circuit *, db::DPolygon> > m_markers;
  std::vector<std::pair<const db::Circuit *, const db::Net *> > m_net_markers;
  lay::NetInfoDialog *mp_info_dialog;
  tl::DeferredMethod<NetlistBrowserPage> dm_update_highlights;
  tl::DeferredMethod<NetlistBrowserPage> dm_rerun_macro;
  db::ContextCache m_cell_context_cache;

  void setup_trees ();
  void add_to_history (const QModelIndex &index, bool fwd);
  void navigate_to (const QModelIndex &index, bool forward = true);
  void adjust_view ();
  void clear_markers ();
  void highlight (const NetlistObjectsPath &current_path, const std::vector<NetlistObjectsPath> &selected_paths);
  void clear_highlights ();
  std::vector<const db::Net *> selected_nets ();
  std::vector<const db::Device *> selected_devices ();
  std::vector<const db::SubCircuit *> selected_subcircuits ();
  std::vector<const db::Circuit *> selected_circuits ();
  void set_color_for_selected_nets (const tl::Color &color);
  void layer_list_changed (int);
  tl::Color make_valid_color (const tl::Color &color);
  bool produce_highlights_for_net(const db::Net *net, size_t &n_markers, const std::map<db::LayerProperties, lay::LayerPropertiesConstIterator> &display_by_lp, const std::vector<db::DCplxTrans> &tv);
  bool produce_highlights_for_device (const db::Device *device, size_t &n_markers, const std::vector<db::DCplxTrans> &tv);
  bool produce_highlights_for_circuit (const db::Circuit *circuit, size_t &n_markers, const std::vector<db::DCplxTrans> &tv);
  void configure_marker (lay::Marker *marker, bool with_fill);
  void rerun_macro ();
  void selection_changed (QTreeView *hierarchy_tree, QTreeView *directory_tree);
  void current_tree_index_changed (QTreeView *hierarchy_tree, QTreeView *directory_tree, const QModelIndex &index);
  void current_index_changed (QTreeView *hierarchy_tree, QTreeView *directory_tree, const QModelIndex &index);
  QTreeView *current_hierarchy_tree ();
  QTreeView *current_directory_tree ();

  void export_nets (const std::vector<const db::Net *> *nets);
};

} // namespace lay

#endif

#endif  //  defined(HAVE_QT)
