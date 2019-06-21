
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


#ifndef HDR_layNetlistBrowserModel
#define HDR_layNetlistBrowserModel

#include "layColorPalette.h"
#include "laybasicCommon.h"

#include "dbLayoutToNetlist.h"
#include "dbLayoutVsSchematic.h"

#include <QAbstractItemModel>
#include <QColor>

#include <map>
#include <memory>

class QTreeView;

namespace lay
{

class IndexedNetlistModel;

// ----------------------------------------------------------------------------------
//  NetColorizer definition

class LAYBASIC_PUBLIC NetColorizer
  : public QObject
{
Q_OBJECT

public:
  NetColorizer ();

  void configure (const QColor &marker_color, const lay::ColorPalette *auto_colors);
  bool has_color_for_net (const db::Net *net);
  void set_color_of_net (const db::Net *net, const QColor &color);
  void reset_color_of_net (const db::Net *net);
  void clear ();

  QColor color_of_net (const db::Net *net) const;

  const QColor &marker_color () const
  {
    return m_marker_color;
  }

  void begin_changes ();
  void end_changes ();

signals:
  void colors_changed ();

private:
  QColor m_marker_color;
  lay::ColorPalette m_auto_colors;
  bool m_auto_colors_enabled;
  std::map<const db::Net *, QColor> m_custom_color;
  bool m_update_needed;
  bool m_signals_enabled;
  mutable std::map<const db::Net *, size_t> m_net_index_by_object;

  void emit_colors_changed ();
};

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
Q_OBJECT

public:
  NetlistBrowserModel (QWidget *parent, db::LayoutToNetlist *l2ndb, NetColorizer *colorizer);
  NetlistBrowserModel (QWidget *parent, db::LayoutVsSchematic *lvsdb, NetColorizer *colorizer);
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

  int status_column () const
  {
    return m_status_column;
  }

  std::pair<const db::Net *, const db::Net *> net_from_index (const QModelIndex &index) const;
  QModelIndex index_from_net (const std::pair<const db::Net *, const db::Net *> &net) const;
  QModelIndex index_from_net (const db::Net *net) const;
  std::pair<const db::Circuit *, const db::Circuit *> circuit_from_index (const QModelIndex &index) const;
  QModelIndex index_from_circuit (const std::pair<const db::Circuit *, const db::Circuit *> &circuit) const;
  QModelIndex index_from_circuit (const db::Circuit *circuit) const;

  std::pair<const db::SubCircuit *, const db::SubCircuit *> subcircuit_from_index (const QModelIndex &index) const;

  std::pair<const db::Device *, const db::Device *> device_from_index (const QModelIndex &index) const;

  bool is_circuit_index (const QModelIndex &index) const
  {
    return is_id_circuit (index.internalPointer ());
  }

  void set_item_visibility (QTreeView *view, bool show_all, bool with_warnings);

private slots:
  void colors_changed ();

private:
  NetlistBrowserModel (const NetlistBrowserModel &);
  NetlistBrowserModel &operator= (const NetlistBrowserModel &);

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
  std::pair<const db::Circuit *, const db::Circuit *> circuits_from_id (void *id) const;
  std::pair<const db::Net *, const db::Net *> nets_from_id (void *id) const;
  std::pair<const db::NetSubcircuitPinRef *, const db::NetSubcircuitPinRef *> net_subcircuit_pinrefs_from_id (void *id) const;
  std::pair<const db::NetTerminalRef *, const db::NetTerminalRef *> net_terminalrefs_from_id (void *id) const;
  std::pair<const db::NetPinRef *, const db::NetPinRef *> net_pinrefs_from_id (void *id) const;
  std::pair<const db::Device *, const db::Device *> devices_from_id (void *id) const;
  std::pair<const db::Pin *, const db::Pin *> pins_from_id (void *id) const;
  std::pair<const db::SubCircuit *, const db::SubCircuit *> subcircuits_from_id (void *id) const;
  QString text (const QModelIndex &index) const;
  QString search_text (const QModelIndex &index) const;
  db::NetlistCrossReference::Status status (const QModelIndex &index) const;
  QIcon icon (const QModelIndex &index) const;
  QString make_link_to (const std::pair<const db::Net *, const db::Net *> &nets, int column = 0) const;
  QString make_link_to (const std::pair<const db::Device *, const db::Device *> &devices, int column = 0) const;
  QString make_link_to (const std::pair<const db::Pin *, const db::Pin *> &pins, const std::pair<const db::Circuit *, const db::Circuit *> &circuits, int column = 0) const;
  QString make_link_to (const std::pair<const db::Circuit *, const db::Circuit *> &circuits, int column = 0) const;
  QString make_link_to (const std::pair<const db::SubCircuit *, const db::SubCircuit *> &sub_circuits, int column = 0) const;

  std::pair<const db::Netlist *, const db::Netlist *> netlists () const
  {
    return std::pair<const db::Netlist *, const db::Netlist *> (mp_l2ndb->netlist (), (const db::Netlist *)0);
  }

  QIcon icon_for_nets (const std::pair<const db::Net *, const db::Net *> &net) const;
  QIcon icon_for_connection (const std::pair<const db::Net *, const db::Net *> &net) const;

  void show_or_hide_items (QTreeView *view, const QModelIndex &parent, bool show_all, bool with_warnings, bool with_children);

  db::LayoutToNetlist *mp_l2ndb;
  db::LayoutVsSchematic *mp_lvsdb;
  NetColorizer *mp_colorizer;
  std::auto_ptr<IndexedNetlistModel> mp_indexer;
  mutable std::map<lay::color_t, QIcon> m_net_icon_per_color;
  mutable std::map<lay::color_t, QIcon> m_connection_icon_per_color;
  int m_object_column;
  int m_status_column;
  int m_first_column;
  int m_second_column;
};

} // namespace lay

#endif

