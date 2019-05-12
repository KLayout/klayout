
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

#include <QAbstractItemModel>
#include <QColor>

#include <map>
#include <memory>

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

  const db::Net *net_from_index (const QModelIndex &index) const;
  QModelIndex index_from_net (const db::Net *net) const;

  const db::SubCircuit *subcircuit_from_index (const QModelIndex &index) const;
  const db::Device *device_from_index (const QModelIndex &index) const;

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
  const db::Circuit *circuit_from_id (void *id) const;
  const db::Net *net_from_id (void *id) const;
  const db::NetSubcircuitPinRef *net_subcircuit_pinref_from_id (void *id) const;
  const db::NetTerminalRef *net_terminalref_from_id (void *id) const;
  const db::NetPinRef *net_pinref_from_id (void *id) const;
  const db::Device *device_from_id (void *id) const;
  const db::Pin *pin_from_id (void *id) const;
  const db::SubCircuit *subcircuit_from_id (void *id) const;
  QString text (const QModelIndex &index) const;
  QString search_text (const QModelIndex &index) const;
  QIcon icon (const QModelIndex &index) const;
  QString make_link_to (const db::Net *net) const;
  QString make_link_to (const db::Device *device) const;
  QString make_link_to (const db::Pin *pin, const db::Circuit *circuit) const;
  QString make_link_to (const db::Circuit *circuit) const;
  QString make_link_to (const db::SubCircuit *sub_circuit) const;

  db::Netlist *netlist () const
  {
    return const_cast<db::Netlist *> (mp_l2ndb->netlist ());
  }

  QIcon icon_for_net (const db::Net *net) const;
  QIcon icon_for_connection (const db::Net *net) const;

  db::LayoutToNetlist *mp_l2ndb;
  NetColorizer *mp_colorizer;
  std::auto_ptr<IndexedNetlistModel> mp_indexer;
  mutable std::map<lay::color_t, QIcon> m_net_icon_per_color;
  mutable std::map<lay::color_t, QIcon> m_connection_icon_per_color;
};

} // namespace lay

#endif

