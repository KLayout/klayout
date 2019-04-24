
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


#ifndef HDR_layNetlistBrowserPage
#define HDR_layNetlistBrowserPage

#include "ui_NetlistBrowserPage.h"
#include "layNetlistBrowser.h"
#include "laybasicCommon.h"
#include "dbBox.h"
#include "dbLayoutToNetlist.h"

#include <QFrame>
#include <QAbstractItemModel>

class QAction;

namespace lay
{

class LayoutView;
class PluginRoot;

// ----------------------------------------------------------------------------------
//  NetlistBrowserModel definition

/**
 *  @brief The NetlistBrowserModel
 *
 *  The model hierarchy is the following
 *  - circuits
 *    - 0..#pins: pins
 *      - net (1x)
 *    - #pins..#pins+#nets: nets
 *      - 0..#devices: terminals
 *        - other terminals and nets
 *      - #devices..#devices+#pins: pins
 *      - #devices+#pins..: subcircuit pins
 *        - other pins and nets
 *    - #pins+#nets..#pins+#nets+#subcircuits: subcircuits
 *      - pins and nets
 *    - #pins+#nets+#subcircuits..: devices
 *      - terminals and nets
 */
class LAYBASIC_PUBLIC NetlistBrowserModel
  : public QAbstractItemModel
{
public:
  NetlistBrowserModel (QWidget *parent, db::LayoutToNetlist *l2ndb);
  ~NetlistBrowserModel ();

  virtual int columnCount (const QModelIndex &parent) const;
  virtual QVariant data (const QModelIndex &index, int role) const;
  virtual Qt::ItemFlags flags (const QModelIndex &index) const;
  virtual bool hasChildren (const QModelIndex &parent) const;
  virtual QVariant headerData (int section, Qt::Orientation orientation, int role) const;
  virtual QModelIndex index (int row, int column, const QModelIndex &parent) const;
  virtual QModelIndex parent (const QModelIndex &index) const;
  virtual int rowCount (const QModelIndex &parent) const;

  QModelIndex index_from_id (void *id, int column) const;

private:
  void *make_id_circuit (size_t circuit_index) const;
  void *make_id_circuit_pin (size_t circuit_index, size_t pin_index) const;
  void *make_id_circuit_pin_net (size_t circuit_index, size_t pin_index, size_t net_index) const;
  void *make_id_circuit_net (size_t circuit_index, size_t net_index) const;
  void *make_id_circuit_net_device_terminal (size_t circuit_index, size_t net_index, size_t terminal_ref_index) const;
  void *make_id_circuit_net_device_terminal_others (size_t circuit_index, size_t net_index, size_t terminal_ref_index, size_t other_index) const;
  void *make_id_circuit_net_pin (size_t circuit_index, size_t net_index, size_t pin_index) const;
  void *make_id_circuit_net_subcircuit_pin (size_t circuit_index, size_t net_index, size_t pin_ref_index) const;
  void *make_id_circuit_net_subcircuit_pin_others (size_t circuit_index, size_t net_index, size_t pin_ref_index, size_t other_index) const;
  void *make_id_circuit_subcircuit (size_t circuit_index, size_t subcircuit_index) const;
  void *make_id_circuit_subcircuit_pin (size_t circuit_index, size_t subcircuit_index, size_t pin_index) const;
  void *make_id_circuit_device (size_t circuit_index, size_t device_index) const;
  void *make_id_circuit_device_terminal (size_t circuit_index, size_t device_index, size_t terminal_index) const;
  bool is_id_circuit (void *id) const;
  bool is_id_circuit_pin (void *id) const;
  bool is_id_circuit_pin_net (void *id) const;
  bool is_id_circuit_net (void *id) const;
  bool is_id_circuit_net_device_terminal (void *id) const;
  bool is_id_circuit_net_device_terminal_others (void *id) const;
  bool is_id_circuit_net_pin (void *id) const;
  bool is_id_circuit_net_subcircuit_pin (void *id) const;
  bool is_id_circuit_net_subcircuit_pin_others (void *id) const;
  bool is_id_circuit_subcircuit (void *id) const;
  bool is_id_circuit_subcircuit_pin (void *id) const;
  bool is_id_circuit_device (void *id) const;
  bool is_id_circuit_device_terminal (void *id) const;
  size_t circuit_index_from_id (void *id) const;
  size_t circuit_pin_index_from_id (void *id) const;
  size_t circuit_device_index_from_id (void *id) const;
  size_t circuit_device_terminal_index_from_id (void *id) const;
  size_t circuit_subcircuit_index_from_id (void *id) const;
  size_t circuit_subcircuit_pin_index_from_id (void *id) const;
  size_t circuit_net_index_from_id (void *id) const;
  size_t circuit_net_pin_index_from_id (void *id) const;
  size_t circuit_net_subcircuit_pin_index_from_id (void *id) const;
  size_t circuit_net_subcircuit_pin_other_index_from_id (void *id) const;
  size_t circuit_net_device_terminal_index_from_id (void *id) const;
  size_t circuit_net_device_terminal_other_index_from_id (void *id) const;
  db::Circuit *circuit_from_id (void *id) const;
  db::Net *net_from_id (void *id) const;
  const db::NetSubcircuitPinRef *net_subcircuit_pinref_from_id (void *id) const;
  const db::NetTerminalRef *net_terminalref_from_id (void *id) const;
  const db::NetPinRef *net_pinref_from_id (void *id) const;
  db::Device *device_from_id (void *id) const;
  db::Pin *pin_from_id (void *id) const;
  db::SubCircuit *subcircuit_from_id (void *id) const;
  QString text (const QModelIndex &index) const;
  QIcon icon (const QModelIndex &index) const;
  size_t circuit_index (const db::Circuit *attr) const;
  size_t net_index (const db::Net *attr) const;
  QString make_link_to (const db::Net *net) const;

  db::Netlist *netlist () const
  {
    return const_cast<db::Netlist *> (mp_l2ndb->netlist ());
  }

  db::LayoutToNetlist *mp_l2ndb;
  mutable std::map<size_t, db::Circuit *> m_circuit_by_index;
  mutable std::map<db::Circuit *, std::map<size_t, db::Net *> > m_net_by_circuit_and_index;
  mutable std::map<db::Net *, std::map<size_t, db::NetSubcircuitPinRef *> > m_subcircuit_pinref_by_net_and_index;
  mutable std::map<db::Net *, std::map<size_t, db::NetTerminalRef *> > m_terminalref_by_net_and_index;
  mutable std::map<db::Net *, std::map<size_t, db::NetPinRef *> > m_pinref_by_net_and_index;
  mutable std::map<db::Circuit *, std::map<size_t, db::Device *> > m_device_by_circuit_and_index;
  mutable std::map<db::Circuit *, std::map<size_t, db::Pin *> > m_pin_by_circuit_and_index;
  mutable std::map<db::Circuit *, std::map<size_t, db::SubCircuit *> > m_subcircuit_by_circuit_and_index;
  mutable std::map<const db::Circuit *, size_t> m_circuit_index_by_object;
  mutable std::map<const db::Net *, size_t> m_net_index_by_object;
};

/**
 *  @brief A marker browser page
 */
class NetlistBrowserPage
  : public QFrame,
    public Ui::NetlistBrowserPage
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
   *  @brief Sets the plugin root object for this object
   */
  void set_plugin_root (lay::PluginRoot *pr);

  /**
   *  @brief Attaches the page to a view
   *
   *  This method can be given a layout view object.
   *  If that pointer is non-null, the browser will attach itself to
   *  the view and provide highlights for the selected markers inside the given cellview.
   */
  void set_view (lay::LayoutView *view, unsigned int cv_index);

  /**
   *  @brief Attach the page to a L2N DB
   *
   *  To detach the page from any L2N DB, pass 0 for the pointer.
   */
  void set_l2ndb (db::LayoutToNetlist *database);


  /**
   *  @brief Set the window type and window dimensions
   */
  void set_window (lay::NetlistBrowserConfig::net_window_type window_type, double window_dim, lay::NetlistBrowserConfig::net_context_mode_type context);

  /**
   *  @brief Update the net highlights
   *
   *  This method should be called if the cellview has changed so the highlights can
   *  be recomputed and shown in the new cell context.
   */
  void update_highlights ();

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
  void set_highlight_style (QColor color, int line_width, int vertex_size, int halo, int dither_pattern);

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

private slots:
  void show_all_clicked ();
  void filter_changed ();
  void anchor_clicked (const QString &url);
  void navigate_back ();
  void navigate_forward ();
  void current_index_changed (const QModelIndex &index);

private:
  bool m_show_all;
  QAction *m_show_all_action;
  NetlistBrowserConfig::net_context_mode_type m_context;
  NetlistBrowserConfig::net_window_type m_window;
  double m_window_dim;
  size_t m_max_shape_count;
  QColor m_marker_color;
  int m_marker_line_width;
  int m_marker_vertex_size;
  int m_marker_halo;
  int m_marker_dither_pattern;
  lay::LayoutView *mp_view;
  unsigned int m_cv_index;
  lay::PluginRoot *mp_plugin_root;
  tl::weak_ptr<db::LayoutToNetlist> mp_database;
  std::vector<void *> m_history;
  size_t m_history_ptr;

  void add_to_history (void *id, bool fwd);
  void navigate_to (void *id, bool forward = true);
};

} // namespace lay

#endif

