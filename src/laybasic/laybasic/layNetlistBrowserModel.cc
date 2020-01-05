
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


#include "layNetlistBrowserModel.h"
#include "layIndexedNetlistModel.h"
#include "layNetlistCrossReferenceModel.h"
#include "dbNetlistDeviceClasses.h"

#include <QPainter>
#include <QIcon>
#include <QWidget>
#include <QTreeView>

namespace lay
{

// ----------------------------------------------------------------------------------
//  NetColorizer implementation

NetColorizer::NetColorizer ()
{
  m_auto_colors_enabled = false;
  m_update_needed = false;
  m_signals_enabled = true;
}

void
NetColorizer::configure (const QColor &marker_color, const lay::ColorPalette *auto_colors)
{
  m_marker_color = marker_color;
  if (auto_colors) {
    m_auto_colors = *auto_colors;
    m_auto_colors_enabled = true;
  } else {
    m_auto_colors_enabled = false;
  }

  emit_colors_changed ();
}

bool
NetColorizer::has_color_for_net (const db::Net *net)
{
  return net != 0 && (m_auto_colors_enabled || m_custom_color.find (net) != m_custom_color.end ());
}

void
NetColorizer::set_color_of_net (const db::Net *net, const QColor &color)
{
  m_custom_color[net] = color;
  emit_colors_changed ();
}

void
NetColorizer::reset_color_of_net (const db::Net *net)
{
  m_custom_color.erase (net);
  emit_colors_changed ();
}

void
NetColorizer::clear ()
{
  m_net_index_by_object.clear ();
  m_custom_color.clear ();
  emit_colors_changed ();
}

void
NetColorizer::begin_changes ()
{
  if (m_signals_enabled) {
    m_update_needed = false;
    m_signals_enabled = false;
  }
}

void
NetColorizer::end_changes ()
{
  if (! m_signals_enabled) {
    m_signals_enabled = true;
    if (m_update_needed) {
      emit colors_changed ();
    }
    m_update_needed = false;
  }
}

void
NetColorizer::emit_colors_changed ()
{
  if (! m_signals_enabled) {
    m_update_needed = true;
  } else {
    emit colors_changed ();
  }
}

QColor
NetColorizer::color_of_net (const db::Net *net) const
{
  if (! net) {
    return QColor ();
  }

  std::map<const db::Net *, QColor>::const_iterator c = m_custom_color.find (net);
  if (c != m_custom_color.end ()) {
    return c->second;
  }

  if (m_auto_colors_enabled) {

    const db::Circuit *circuit = net->circuit ();

    size_t index = 0;

    std::map<const db::Net *, size_t>::iterator cc = m_net_index_by_object.find (net);
    if (cc == m_net_index_by_object.end ()) {

      size_t i = 0;
      for (db::Circuit::const_net_iterator n = circuit->begin_nets (); n != circuit->end_nets (); ++n, ++i) {
        m_net_index_by_object.insert (std::make_pair (n.operator-> (), i));
        if (n.operator-> () == net) {
          index = i;
        }
      }

    } else {
      index = cc->second;
    }

    return m_auto_colors.color_by_index ((unsigned int) index);

  } else {
    return QColor ();
  }
}

// ----------------------------------------------------------------------------------
//  NetlistBrowserModel implementation

static inline void *make_id (size_t i1)
{
  return reinterpret_cast<void *> (i1);
}

/* not used yet:
static inline void *make_id (size_t i1, size_t n1, size_t i2)
{
  return reinterpret_cast<void *> (i1 + n1 * i2);
}
*/

static inline void *make_id (size_t i1, size_t n1, size_t i2, size_t n2, size_t i3)
{
  return reinterpret_cast<void *> (i1 + n1 * (i2 + n2 * i3));
}

static inline void *make_id (size_t i1, size_t n1, size_t i2, size_t n2, size_t i3, size_t n3, size_t i4)
{
  return reinterpret_cast<void *> (i1 + n1 * (i2 + n2 * (i3 + n3 * i4)));
}

static inline void *make_id (size_t i1, size_t n1, size_t i2, size_t n2, size_t i3, size_t n3, size_t i4, size_t n4, size_t i5)
{
  return reinterpret_cast<void *> (i1 + n1 * (i2 + n2 * (i3 + n3 * (i4 + n4 * i5))));
}

static inline void *make_id (size_t i1, size_t n1, size_t i2, size_t n2, size_t i3, size_t n3, size_t i4, size_t n4, size_t i5, size_t n5, size_t i6)
{
  return reinterpret_cast<void *> (i1 + n1 * (i2 + n2 * (i3 + n3 * (i4 + n4 * (i5 + n5 * i6)))));
}

static inline size_t pop (void *&idp, size_t n)
{
  size_t id = reinterpret_cast<size_t> (idp);
  size_t i = id % n;
  id /= n;
  idp = reinterpret_cast<void *> (id);
  return i;
}

static inline bool always (bool)
{
  return true;
}

static void *no_id = reinterpret_cast<void *> (-1);

NetlistBrowserModel::NetlistBrowserModel (QWidget *parent, db::LayoutToNetlist *l2ndb, NetColorizer *colorizer)
  : QAbstractItemModel (parent), mp_l2ndb (l2ndb), mp_lvsdb (0), mp_colorizer (colorizer)
{
  mp_indexer.reset (new SingleIndexedNetlistModel (l2ndb->netlist ()));
  connect (mp_colorizer, SIGNAL (colors_changed ()), this, SLOT (colors_changed ()));

  m_object_column = 0;
  m_status_column = -1;
  m_first_column = 2;
  m_second_column = -1;
}

NetlistBrowserModel::NetlistBrowserModel (QWidget *parent, db::LayoutVsSchematic *lvsdb, NetColorizer *colorizer)
  : QAbstractItemModel (parent), mp_l2ndb (0), mp_lvsdb (lvsdb), mp_colorizer (colorizer)
{
  mp_indexer.reset (new NetlistCrossReferenceModel (lvsdb->cross_ref ()));
  connect (mp_colorizer, SIGNAL (colors_changed ()), this, SLOT (colors_changed ()));

  m_object_column = 0;
  m_status_column = 1;
  m_first_column = 2;
  m_second_column = 3;
}

NetlistBrowserModel::~NetlistBrowserModel ()
{
  //  .. nothing yet ..
}

void *
NetlistBrowserModel::make_id_circuit (size_t circuit_index) const
{
  if (circuit_index == lay::no_netlist_index) {
    return no_id;
  } else {
    return make_id (circuit_index);
  }
}

void *
NetlistBrowserModel::make_id_circuit_pin (size_t circuit_index, size_t pin_index) const
{
  if (circuit_index == lay::no_netlist_index || pin_index == lay::no_netlist_index) {
    return no_id;
  } else {
    return make_id (circuit_index, mp_indexer->circuit_count (), 1, 8, pin_index);
  }
}

void *
NetlistBrowserModel::make_id_circuit_pin_net (size_t circuit_index, size_t pin_index, size_t net_index) const
{
  if (circuit_index == lay::no_netlist_index || pin_index == lay::no_netlist_index || net_index == lay::no_netlist_index) {
    return no_id;
  } else {
    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (make_id_circuit (circuit_index));
    return make_id (circuit_index, mp_indexer->circuit_count (), 1, 8, pin_index, mp_indexer->pin_count (circuits), net_index + 1);
  }
}

void *
NetlistBrowserModel::make_id_circuit_net (size_t circuit_index, size_t net_index) const
{
  if (circuit_index == lay::no_netlist_index || net_index == lay::no_netlist_index) {
    return no_id;
  } else {
    return make_id (circuit_index, mp_indexer->circuit_count (), 2, 8, net_index);
  }
}

void *
NetlistBrowserModel::make_id_circuit_net_device_terminal (size_t circuit_index, size_t net_index, size_t terminal_ref_index) const
{
  if (circuit_index == lay::no_netlist_index || net_index == lay::no_netlist_index || terminal_ref_index == lay::no_netlist_index) {
    return no_id;
  } else {
    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (make_id_circuit (circuit_index));
    return make_id (circuit_index, mp_indexer->circuit_count (), 2, 8, net_index, mp_indexer->net_count (circuits), 1, 4, terminal_ref_index);
  }
}

void *
NetlistBrowserModel::make_id_circuit_net_device_terminal_others (size_t circuit_index, size_t net_index, size_t terminal_ref_index, size_t other_index) const
{
  if (circuit_index == lay::no_netlist_index || net_index == lay::no_netlist_index || terminal_ref_index == lay::no_netlist_index || other_index == lay::no_netlist_index) {
    return no_id;
  } else {
    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (make_id_circuit (circuit_index));
    IndexedNetlistModel::net_pair nets = nets_from_id (make_id_circuit_net (circuit_index, net_index));
    return make_id (circuit_index, mp_indexer->circuit_count (), 2, 8, net_index, mp_indexer->net_count (circuits), 1, 4, terminal_ref_index, mp_indexer->net_terminal_count (nets), other_index + 1);
  }
}

void *
NetlistBrowserModel::make_id_circuit_net_pin (size_t circuit_index, size_t net_index, size_t pin_index) const
{
  if (circuit_index == lay::no_netlist_index || net_index == lay::no_netlist_index || pin_index == lay::no_netlist_index) {
    return no_id;
  } else {
    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (make_id_circuit (circuit_index));
    return make_id (circuit_index, mp_indexer->circuit_count (), 2, 8, net_index, mp_indexer->net_count (circuits), 2, 4, pin_index);
  }
}

void *
NetlistBrowserModel::make_id_circuit_net_subcircuit_pin (size_t circuit_index, size_t net_index, size_t pin_ref_index) const
{
  if (circuit_index == lay::no_netlist_index || net_index == lay::no_netlist_index || pin_ref_index == lay::no_netlist_index) {
    return no_id;
  } else {
    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (make_id_circuit (circuit_index));
    return make_id (circuit_index, mp_indexer->circuit_count (), 2, 8, net_index, mp_indexer->net_count (circuits), 3, 4, pin_ref_index);
  }
}

void *
NetlistBrowserModel::make_id_circuit_net_subcircuit_pin_others (size_t circuit_index, size_t net_index, size_t pin_ref_index, size_t other_index) const
{
  if (circuit_index == lay::no_netlist_index || net_index == lay::no_netlist_index || pin_ref_index == lay::no_netlist_index || other_index == lay::no_netlist_index) {
    return no_id;
  } else {
    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (make_id_circuit (circuit_index));
    IndexedNetlistModel::net_pair nets = nets_from_id (make_id_circuit_net (circuit_index, net_index));
    return make_id (circuit_index, mp_indexer->circuit_count (), 2, 8, net_index, mp_indexer->net_count (circuits), 3, 4, pin_ref_index, mp_indexer->net_subcircuit_pin_count (nets), other_index + 1);
  }
}

void *
NetlistBrowserModel::make_id_circuit_subcircuit (size_t circuit_index, size_t subcircuit_index) const
{
  if (circuit_index == lay::no_netlist_index || subcircuit_index == lay::no_netlist_index) {
    return no_id;
  } else {
    return make_id (circuit_index, mp_indexer->circuit_count (), 3, 8, subcircuit_index);
  }
}

void *
NetlistBrowserModel::make_id_circuit_subcircuit_pin (size_t circuit_index, size_t subcircuit_index, size_t pin_index) const
{
  if (circuit_index == lay::no_netlist_index || subcircuit_index == lay::no_netlist_index || pin_index == lay::no_netlist_index) {
    return no_id;
  } else {
    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (make_id_circuit (circuit_index));
    return make_id (circuit_index, mp_indexer->circuit_count (), 3, 8, subcircuit_index, mp_indexer->subcircuit_count (circuits), pin_index + 1);
  }
}

void *
NetlistBrowserModel::make_id_circuit_device (size_t circuit_index, size_t device_index) const
{
  if (circuit_index == lay::no_netlist_index || device_index == lay::no_netlist_index) {
    return no_id;
  } else {
    return make_id (circuit_index, mp_indexer->circuit_count (), 4, 8, device_index);
  }
}

void *
NetlistBrowserModel::make_id_circuit_device_terminal (size_t circuit_index, size_t device_index, size_t terminal_index) const
{
  if (circuit_index == lay::no_netlist_index || device_index == lay::no_netlist_index || terminal_index == lay::no_netlist_index) {
    return no_id;
  } else {
    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (make_id_circuit (circuit_index));
    return make_id (circuit_index, mp_indexer->circuit_count (), 4, 8, device_index, mp_indexer->device_count (circuits), terminal_index + 1);
  }
}

bool
NetlistBrowserModel::is_id_circuit (void *id) const
{
  if (mp_indexer->circuit_count () == 0) {
    return false;
  }

  pop (id, mp_indexer->circuit_count ());
  return id == 0;
}

bool
NetlistBrowserModel::is_id_circuit_pin (void *id) const
{
  if (mp_indexer->circuit_count () == 0) {
    return false;
  }

  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  return (pop (id, 8) == 1 && always (pop (id, mp_indexer->pin_count (circuits))) && id == 0);
}

bool
NetlistBrowserModel::is_id_circuit_pin_net (void *id) const
{
  if (mp_indexer->circuit_count () == 0) {
    return false;
  }

  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  return (pop (id, 8) == 1 && always (pop (id, mp_indexer->pin_count (circuits))) && id != 0);
}

bool
NetlistBrowserModel::is_id_circuit_net (void *id) const
{
  if (mp_indexer->circuit_count () == 0) {
    return false;
  }

  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  return (pop (id, 8) == 2 && always (pop (id, mp_indexer->net_count (circuits))) && id == 0);
}

bool
NetlistBrowserModel::is_id_circuit_net_device_terminal (void *id) const
{
  if (mp_indexer->circuit_count () == 0) {
    return false;
  }

  void *org_id = id;

  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  if (pop (id, 8) == 2 && always (pop (id, mp_indexer->net_count (circuits))) && pop (id, 4) == 1) {
    IndexedNetlistModel::net_pair nets = nets_from_id (org_id);
    pop (id, mp_indexer->net_terminal_count (nets));
    return id == 0;
  }

  return false;
}

bool
NetlistBrowserModel::is_id_circuit_net_device_terminal_others (void *id) const
{
  if (mp_indexer->circuit_count () == 0) {
    return false;
  }

  void *org_id = id;

  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  if (pop (id, 8) == 2 && always (pop (id, mp_indexer->net_count (circuits))) && pop (id, 4) == 1) {
    IndexedNetlistModel::net_pair nets = nets_from_id (org_id);
    pop (id, mp_indexer->net_terminal_count (nets));
    return id != 0;
  }

  return false;
}

bool
NetlistBrowserModel::is_id_circuit_net_pin (void *id) const
{
  if (mp_indexer->circuit_count () == 0) {
    return false;
  }

  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  return (pop (id, 8) == 2 && always (pop (id, mp_indexer->net_count (circuits))) && pop (id, 4) == 2);
}

bool
NetlistBrowserModel::is_id_circuit_net_subcircuit_pin (void *id) const
{
  if (mp_indexer->circuit_count () == 0) {
    return false;
  }

  void *org_id = id;

  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  if (pop (id, 8) == 2 && always (pop (id, mp_indexer->net_count (circuits))) && pop (id, 4) == 3) {
    IndexedNetlistModel::net_pair nets = nets_from_id (org_id);
    pop (id, mp_indexer->net_subcircuit_pin_count (nets));
    return id == 0;
  }

  return false;
}

bool
NetlistBrowserModel::is_id_circuit_net_subcircuit_pin_others (void *id) const
{
  if (mp_indexer->circuit_count () == 0) {
    return false;
  }

  void *org_id = id;

  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  if (pop (id, 8) == 2 && always (pop (id, mp_indexer->net_count (circuits))) && pop (id, 4) == 3) {
    IndexedNetlistModel::net_pair nets = nets_from_id (org_id);
    pop (id, mp_indexer->net_subcircuit_pin_count (nets));
    return id != 0;
  }

  return false;
}

bool
NetlistBrowserModel::is_id_circuit_subcircuit (void *id) const
{
  if (mp_indexer->circuit_count () == 0) {
    return false;
  }

  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  return (pop (id, 8) == 3 && always (pop (id, mp_indexer->subcircuit_count (circuits))) && id == 0);
}

bool
NetlistBrowserModel::is_id_circuit_subcircuit_pin (void *id) const
{
  if (mp_indexer->circuit_count () == 0) {
    return false;
  }

  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  return (pop (id, 8) == 3 && always (pop (id, mp_indexer->subcircuit_count (circuits))) && id != 0);
}

bool
NetlistBrowserModel::is_id_circuit_device (void *id) const
{
  if (mp_indexer->circuit_count () == 0) {
    return false;
  }

  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  return (pop (id, 8) == 4 && always (pop (id, mp_indexer->device_count (circuits))) && id == 0);
}

bool
NetlistBrowserModel::is_id_circuit_device_terminal (void *id) const
{
  if (mp_indexer->circuit_count () == 0) {
    return false;
  }

  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  return (pop (id, 8) == 4 && always (pop (id, mp_indexer->device_count (circuits))) && id != 0);
}

size_t
NetlistBrowserModel::circuit_index_from_id (void *id) const
{
  return pop (id, mp_indexer->circuit_count ());
}

size_t
NetlistBrowserModel::circuit_pin_index_from_id (void *id) const
{
  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  pop (id, 8);
  return pop (id, mp_indexer->pin_count (circuits));
}

size_t
NetlistBrowserModel::circuit_device_index_from_id (void *id) const
{
  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  pop (id, 8);
  return pop (id, mp_indexer->device_count (circuits));
}

size_t
NetlistBrowserModel::circuit_device_terminal_index_from_id (void *id) const
{
  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  pop (id, 8);
  pop (id, mp_indexer->device_count (circuits));
  return reinterpret_cast<size_t> (id) - 1;
}

size_t
NetlistBrowserModel::circuit_subcircuit_index_from_id (void *id) const
{
  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  pop (id, 8);
  return pop (id, mp_indexer->subcircuit_count (circuits));
}

size_t
NetlistBrowserModel::circuit_subcircuit_pin_index_from_id (void *id) const
{
  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  pop (id, 8);
  pop (id, mp_indexer->subcircuit_count (circuits));
  return reinterpret_cast<size_t> (id) - 1;
}

size_t
NetlistBrowserModel::circuit_net_index_from_id (void *id) const
{
  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  pop (id, 8);
  return pop (id, mp_indexer->net_count (circuits));
}

size_t
NetlistBrowserModel::circuit_net_pin_index_from_id (void *id) const
{
  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  pop (id, 8);
  pop (id, mp_indexer->net_count (circuits));
  pop (id, 4);
  return reinterpret_cast<size_t> (id);
}

size_t
NetlistBrowserModel::circuit_net_subcircuit_pin_index_from_id (void *id) const
{
  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  IndexedNetlistModel::net_pair nets = nets_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  pop (id, 8);
  pop (id, mp_indexer->net_count (circuits));
  pop (id, 4);
  return pop (id, mp_indexer->net_subcircuit_pin_count (nets));
}

size_t
NetlistBrowserModel::circuit_net_subcircuit_pin_other_index_from_id (void *id) const
{
  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  IndexedNetlistModel::net_pair nets = nets_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  pop (id, 8);
  pop (id, mp_indexer->net_count (circuits));
  pop (id, 4);
  pop (id, mp_indexer->net_subcircuit_pin_count (nets));
  return reinterpret_cast<size_t> (id) - 1;
}

size_t
NetlistBrowserModel::circuit_net_device_terminal_index_from_id (void *id) const
{
  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  IndexedNetlistModel::net_pair nets = nets_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  pop (id, 8);
  pop (id, mp_indexer->net_count (circuits));
  pop (id, 4);
  return pop (id, mp_indexer->net_terminal_count (nets));
}

size_t
NetlistBrowserModel::circuit_net_device_terminal_other_index_from_id (void *id) const
{
  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  IndexedNetlistModel::net_pair nets = nets_from_id (id);
  pop (id, mp_indexer->circuit_count ());
  pop (id, 8);
  pop (id, mp_indexer->net_count (circuits));
  pop (id, 4);
  pop (id, mp_indexer->net_terminal_count (nets));
  return reinterpret_cast<size_t> (id) - 1;
}

int
NetlistBrowserModel::columnCount (const QModelIndex & /*parent*/) const
{
  //  Item type & icon, link or description
  return mp_indexer->is_single () ? 3 : 4;
}

QIcon icon_for_status (db::NetlistCrossReference::Status status)
{
  if (status == db::NetlistCrossReference::NoMatch || status == db::NetlistCrossReference::Mismatch) {
    return QIcon (":/error2_16.png");
  } else if (status == db::NetlistCrossReference::MatchWithWarning || status == db::NetlistCrossReference::Skipped) {
    return QIcon (":/warn_16.png");
  } else {
    return QIcon ();
  }
}

QVariant
NetlistBrowserModel::data (const QModelIndex &index, int role) const
{
  if (! index.isValid ()) {
    return QVariant ();
  }

  if (role == Qt::DecorationRole && index.column () == m_object_column) {
    return QVariant (icon (index));
  } else if (role == Qt::DecorationRole && index.column () == m_status_column) {
    return QVariant (icon_for_status (status (index)));
  } else if (role == Qt::DisplayRole) {
    return QVariant (text (index));
  } else if (role == Qt::ToolTipRole && index.column () == m_status_column) {
    return tooltip (index);
  } else if (role == Qt::UserRole) {
    return QVariant (search_text (index));
  } else if (role == Qt::FontRole) {
    db::NetlistCrossReference::Status st = status (index);
    if (st == db::NetlistCrossReference::NoMatch || st == db::NetlistCrossReference::Mismatch || st == db::NetlistCrossReference::Skipped) {
      QFont font;
      font.setWeight (QFont::Bold);
      return QVariant (font);
    }
  } else if (role == Qt::ForegroundRole) {
    db::NetlistCrossReference::Status st = status (index);
    if (st == db::NetlistCrossReference::Match || st == db::NetlistCrossReference::MatchWithWarning) {
      //  taken from marker browser:
      return QVariant (QColor (0, 192, 0));
    }
  }
  return QVariant ();
}

template <class Obj>
static std::string str_from_expanded_name (const Obj *obj, bool dash_for_empty = false)
{
  if (obj) {
    return obj->expanded_name ();
  } else if (dash_for_empty) {
    return std::string ("-");
  } else {
    return std::string ();
  }
}

template <class Obj>
static std::string str_from_name (const Obj *obj, bool dash_for_empty = false)
{
  if (obj) {
    return obj->name ();
  } else if (dash_for_empty) {
    return std::string ("-");
  } else {
    return std::string ();
  }
}

const std::string var_sep (" ⇔ ");

template <class Obj>
static std::string str_from_expanded_names (const std::pair<const Obj *, const Obj *> &objs, bool is_single)
{
  std::string s = str_from_expanded_name (objs.first, ! is_single);
  if (! is_single) {
    std::string t = str_from_expanded_name (objs.second, ! is_single);
    if (t != s) {
      s += var_sep;
      s += t;
    }
  }
  return s;
}

template <class Obj>
static std::string str_from_names (const std::pair<const Obj *, const Obj *> &objs, bool is_single)
{
  std::string s = str_from_name (objs.first, ! is_single);
  if (! is_single) {
    std::string t = str_from_name (objs.second, ! is_single);
    if (t != s) {
      s += var_sep;
      s += t;
    }
  }
  return s;
}

static
std::string formatted_value (double v)
{
  double va = fabs (v);
  if (va < 100e-15) {
    return tl::to_string (v * 1e15) + "f";
  } else if (va < 100e-12) {
    return tl::to_string (v * 1e12) + "p";
  } else if (va < 100e-9) {
    return tl::to_string (v * 1e9) + "n";
  } else if (va < 100e-6) {
    return tl::to_string (v * 1e6) + "µ";
  } else if (va < 100e-3) {
    return tl::to_string (v * 1e3) + "m";
  } else if (va < 100.0) {
    return tl::to_string (v);
  } else if (va < 100e3) {
    return tl::to_string (v * 1e-3) + "k";
  } else if (va < 100e6) {
    return tl::to_string (v * 1e-6) + "M";
  } else if (va < 100e9) {
    return tl::to_string (v * 1e-9) + "G";
  } else {
    return tl::to_string (v);
  }
}

static
std::string device_parameter_string (const db::Device *device)
{
  std::string s;
  if (! device || ! device->device_class ()) {
    return s;
  }

  bool first = true;
  const std::vector<db::DeviceParameterDefinition> &pd = device->device_class ()->parameter_definitions ();
  for (std::vector<db::DeviceParameterDefinition>::const_iterator p = pd.begin (); p != pd.end (); ++p) {
    if (p->is_primary ()) {
      if (first) {
        s += " [";
        first = false;
      } else {
        s += ", ";
      }
      s += p->name ();
      s += "=";
      s += formatted_value (device->parameter_value (p->id ()));
    }
  }
  if (! first) {
    s += "]";
  }
  return s;
}

static
std::string device_string (const db::Device *device)
{
  if (! device || ! device->device_class ()) {
    return std::string ();
  }

  return device->device_class ()->name () + device_parameter_string (device);
}

static
std::string device_class_string (const db::Device *device, bool dash_for_empty = false)
{
  std::string s;
  if (device && device->device_class ()) {
    s = device->device_class ()->name ();
  } else if (dash_for_empty) {
    s = "-";
  }
  return s;
}

static
std::string devices_string (const std::pair<const db::Device *, const db::Device *> &devices, bool is_single, bool with_parameters)
{
  if (devices.first || devices.second) {

    std::string s;
    s = device_class_string (devices.first, ! is_single);
    if (with_parameters) {
      s += device_parameter_string (devices.first);
    }
    if (! is_single) {
      std::string t = device_class_string (devices.second, ! is_single);
      if (with_parameters) {
        t += device_parameter_string (devices.second);
      }
      if (t != s) {
        s += var_sep;
        s += t;
      }
    }

    return s;

  } else {
    return std::string ();
  }
}

static QString build_url (void *id, const std::string &tag, const std::string &title)
{
  if (id == no_id) {
    //  no link
    return tl::to_qstring (tl::escaped_to_html (title));
  }

  std::string s = std::string ("<a href='int:");
  s += tag;
  s += "?id=";
  s += tl::to_string (reinterpret_cast<size_t> (id));
  s += "'>";

  s += tl::escaped_to_html (title);

  s += "</a>";

  return tl::to_qstring (s);
}

QString
NetlistBrowserModel::make_link_to (const std::pair<const db::Net *, const db::Net *> &nets, int column) const
{
  if ((! nets.first || column == m_second_column) && (! nets.second || column == m_first_column)) {
    return QString ();
  } else {

    IndexedNetlistModel::circuit_pair circuits = mp_indexer->parent_of (nets);
    void *id = no_id;
    //  NOTE: the nets may not be a valid net pair. In this case, circuits is (0, 0) and
    //  no link is generated
    if (circuits.first || circuits.second) {
      id = make_id_circuit_net (mp_indexer->circuit_index (circuits), mp_indexer->net_index (nets));
    }

    if (mp_indexer->is_single () || column == m_first_column) {
      return build_url (id, "net", str_from_expanded_name (nets.first));
    } else if (column == m_second_column) {
      return build_url (id, "net", str_from_expanded_name (nets.second));
    } else {
      return build_url (id, "net", str_from_expanded_names (nets, mp_indexer->is_single ()));
    }

  }
}

QString
NetlistBrowserModel::make_link_to (const std::pair<const db::Device *, const db::Device *> &devices, int column) const
{
  if ((! devices.first || column == m_second_column) && (! devices.second || column == m_first_column)) {
    return QString ();
  } else {

    IndexedNetlistModel::circuit_pair circuits = mp_indexer->parent_of (devices);
    void *id = no_id;
    //  NOTE: the devices may not be a valid device pair. In this case, circuits is (0, 0) and
    //  no link is generated
    if (circuits.first || circuits.second) {
      id = make_id_circuit_device (mp_indexer->circuit_index (circuits), mp_indexer->device_index (devices));
    }

    if (mp_indexer->is_single () || column == m_first_column) {
      return build_url (id, "device", str_from_expanded_name (devices.first));
    } else if (column == m_second_column) {
      return build_url (id, "device", str_from_expanded_name (devices.second));
    } else {
      return build_url (id, "device", str_from_expanded_names (devices, mp_indexer->is_single ()));
    }

  }
}

QString
NetlistBrowserModel::make_link_to (const std::pair<const db::Pin *, const db::Pin *> &pins, const std::pair<const db::Circuit *, const db::Circuit *> &circuits, int column) const
{
  if ((! pins.first || column == m_second_column) && (! pins.second || column == m_first_column)) {
    return QString ();
  } else {
    void *id = make_id_circuit_pin (mp_indexer->circuit_index (circuits), mp_indexer->pin_index (pins, circuits));
    if (mp_indexer->is_single () || column == m_first_column) {
      return build_url (id, "pin", str_from_expanded_name (pins.first));
    } else if (column == m_second_column) {
      return build_url (id, "pin", str_from_expanded_name (pins.second));
    } else {
      return build_url (id, "pin", str_from_expanded_names (pins, mp_indexer->is_single ()));
    }
  }
}

QString
NetlistBrowserModel::make_link_to (const std::pair<const db::Circuit *, const db::Circuit *> &circuits, int column) const
{
  if ((! circuits.first || column == m_second_column) && (! circuits.second || column == m_first_column)) {
    return QString ();
  } else {
    void *id = make_id_circuit (mp_indexer->circuit_index (circuits));
    if (mp_indexer->is_single () || column == m_first_column) {
      return build_url (id, "circuit", str_from_name (circuits.first));
    } else if (column == m_second_column) {
      return build_url (id, "circuit", str_from_name (circuits.second));
    } else {
      return build_url (id, "circuit", str_from_names (circuits, mp_indexer->is_single ()));
    }
  }
}

QString
NetlistBrowserModel::make_link_to (const std::pair<const db::SubCircuit *, const db::SubCircuit *> &subcircuits, int column) const
{
  if ((! subcircuits.first || column == m_second_column) && (! subcircuits.second || column == m_first_column)) {
    return QString ();
  } else {

    IndexedNetlistModel::circuit_pair circuits = mp_indexer->parent_of (subcircuits);
    void *id = no_id;
    //  NOTE: the subcircuits may not be a valid subcircuit pair. In this case, circuits is (0, 0) and
    //  no link is generated
    if (circuits.first || circuits.second) {
      id = make_id_circuit_subcircuit (mp_indexer->circuit_index (circuits), mp_indexer->subcircuit_index (subcircuits));
    }

    if (mp_indexer->is_single () || column == m_first_column) {
      return build_url (id, "subcircuit", str_from_expanded_name (subcircuits.first));
    } else if (column == m_second_column) {
      return build_url (id, "subcircuit", str_from_expanded_name (subcircuits.second));
    } else {
      return build_url (id, "subcircuit", str_from_expanded_names (subcircuits, mp_indexer->is_single ()));
    }

  }
}

static
IndexedNetlistModel::circuit_pair circuit_refs_from_subcircuits (const IndexedNetlistModel::subcircuit_pair &subcircuits)
{
  const db::Circuit *circuit1 = 0, *circuit2 = 0;
  if (subcircuits.first) {
    circuit1 = subcircuits.first->circuit_ref ();
  }
  if (subcircuits.second) {
    circuit2 = subcircuits.second->circuit_ref ();
  }
  return std::make_pair (circuit1, circuit2);
}

static
IndexedNetlistModel::subcircuit_pair subcircuits_from_pinrefs (const IndexedNetlistModel::net_subcircuit_pin_pair &pinrefs)
{
  const db::SubCircuit *subcircuit1 = 0, *subcircuit2 = 0;
  if (pinrefs.first) {
    subcircuit1 = pinrefs.first->subcircuit ();
  }
  if (pinrefs.second) {
    subcircuit2 = pinrefs.second->subcircuit ();
  }

  return std::make_pair (subcircuit1, subcircuit2);
}

static
IndexedNetlistModel::device_pair devices_from_termrefs (const IndexedNetlistModel::net_terminal_pair &termrefs)
{
  const db::Device *device1 = 0, *device2 = 0;
  if (termrefs.first) {
    device1 = termrefs.first->device ();
  }
  if (termrefs.second) {
    device2 = termrefs.second->device ();
  }

  return std::make_pair (device1, device2);
}

static
IndexedNetlistModel::pin_pair pins_from_pinrefs (const IndexedNetlistModel::net_subcircuit_pin_pair &pinrefs)
{
  const db::Pin *pin1 = 0, *pin2 = 0;
  if (pinrefs.first) {
    pin1 = pinrefs.first->pin ();
  }
  if (pinrefs.second) {
    pin2 = pinrefs.second->pin ();
  }

  return std::make_pair (pin1, pin2);
}

static
IndexedNetlistModel::pin_pair pins_from_pinrefs (const IndexedNetlistModel::net_pin_pair &pinrefs)
{
  const db::Pin *pin1 = 0, *pin2 = 0;
  if (pinrefs.first) {
    pin1 = pinrefs.first->pin ();
  }
  if (pinrefs.second) {
    pin2 = pinrefs.second->pin ();
  }

  return std::make_pair (pin1, pin2);
}

static
IndexedNetlistModel::net_pair nets_from_subcircuit_pins (const IndexedNetlistModel::subcircuit_pair &subcircuits, const IndexedNetlistModel::pin_pair &pins)
{
  const db::Net *net1 = 0, *net2 = 0;
  if (pins.first && subcircuits.first) {
    net1 = subcircuits.first->net_for_pin (pins.first->id ());
  }
  if (pins.second && subcircuits.second) {
    net2 = subcircuits.second->net_for_pin (pins.second->id ());
  }

  return std::make_pair (net1, net2);
}

static
IndexedNetlistModel::net_pair nets_from_circuit_pins (const IndexedNetlistModel::circuit_pair &circuits, const IndexedNetlistModel::pin_pair &pins)
{
  const db::Net *net1 = 0, *net2 = 0;
  if (pins.first && circuits.first) {
    net1 = circuits.first->net_for_pin (pins.first->id ());
  }
  if (pins.second && circuits.second) {
    net2 = circuits.second->net_for_pin (pins.second->id ());
  }

  return std::make_pair (net1, net2);
}

static std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes_from_devices (const IndexedNetlistModel::device_pair &devices)
{
  return std::make_pair (devices.first ? devices.first->device_class () : 0, devices.second ? devices.second->device_class () : 0);
}

static std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> terminal_defs_from_terminal_refs (const IndexedNetlistModel::net_terminal_pair &termrefs)
{
  return std::make_pair (termrefs.first ? termrefs.first->terminal_def () : 0, termrefs.second ? termrefs.second->terminal_def () : 0);
}

static std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> terminal_defs_from_device_classes (const std::pair<const db::DeviceClass *, const db::DeviceClass *> &device_classes, size_t terminal_id)
{
  const db::DeviceTerminalDefinition *td1 = 0, *td2 = 0;
  if (device_classes.first && device_classes.first->terminal_definitions ().size () > terminal_id) {
    td1 = &device_classes.first->terminal_definitions () [terminal_id];
  }
  if (device_classes.second && device_classes.second->terminal_definitions ().size () > terminal_id) {
    td2 = &device_classes.second->terminal_definitions () [terminal_id];
  }
  return std::make_pair (td1, td2);
}

static
IndexedNetlistModel::net_pair nets_from_device_terminals (const IndexedNetlistModel::device_pair &devices, const std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> &termdefs)
{
  const db::Net *net1 = 0, *net2 = 0;
  if (termdefs.first && devices.first) {
    net1 = devices.first->net_for_terminal (termdefs.first->id ());
  }
  if (termdefs.second && devices.second) {
    net2 = devices.second->net_for_terminal (termdefs.second->id ());
  }

  return std::make_pair (net1, net2);
}

const std::string field_sep (" / ");

static QString escaped (const std::string &s)
{
  return tl::to_qstring (tl::escaped_to_html (s));
}

QString
NetlistBrowserModel::text (const QModelIndex &index) const
{
  void *id = index.internalPointer ();

  if (is_id_circuit (id)) {

    //  circuit:
    //  + single mode:     name              | <empty>  | <empty>
    //  + dual mode:       name(a)/name(b)   | name(a)  | name(b)
    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
    if (index.column () == m_object_column) {
      return escaped (str_from_names (circuits, mp_indexer->is_single ()));
    } else if (!mp_indexer->is_single () && (index.column () == m_first_column || index.column () == m_second_column)) {
      return escaped (str_from_name (index.column () == m_first_column ? circuits.first : circuits.second));
    }

  } else if (is_id_circuit_pin (id)) {

    //  pin:
    //  + single mode:     xname             | <empty>  | <empty>
    //  + dual mode:       xname(a)/xname(b) | xname(a) | xname(b)
    IndexedNetlistModel::pin_pair pins = pins_from_id (id);
    if (index.column () == m_object_column) {
      return escaped (str_from_expanded_names (pins, mp_indexer->is_single ()));
    } else if (!mp_indexer->is_single () && (index.column () == m_first_column || index.column () == m_second_column)) {
      return escaped (str_from_expanded_name (index.column () == m_first_column ? pins.first : pins.second));
    }

  } else if (is_id_circuit_pin_net (id)) {

    //  circuit/pin/net: header column = name, second column link to net
    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
    IndexedNetlistModel::pin_pair pins = pins_from_id (id);
    IndexedNetlistModel::net_pair nets = nets_from_circuit_pins (circuits, pins);

    if (index.column () == m_object_column) {
      return escaped (str_from_expanded_names (nets, mp_indexer->is_single ()));
    } else if (index.column () == m_first_column || index.column () == m_second_column) {
      return make_link_to (nets, index.column ());
    }

  } else if (is_id_circuit_device (id)) {

    //  circuit/device: header column = class + parameters, second column device name
    IndexedNetlistModel::device_pair devices = devices_from_id (id);

    if (mp_indexer->is_single ()) {

      if (index.column () == m_object_column) {
        return escaped (device_string (devices.first));
      } else if (index.column () == m_first_column) {
        return escaped (str_from_expanded_name (devices.first));
      }

    } else {

      if (index.column () == m_object_column) {
        return escaped (devices_string (devices, mp_indexer->is_single (), false /*without parameters*/));
      } else if (index.column () == m_first_column) {
        return escaped (str_from_expanded_name (devices.first) + field_sep + device_string (devices.first));
      } else if (index.column () == m_second_column) {
        return escaped (str_from_expanded_name (devices.second) + field_sep + device_string (devices.second));
      }

    }

  } else if (is_id_circuit_device_terminal (id)) {

    //  circuit/device/terminal: header column = terminal name, second column link to net
    IndexedNetlistModel::device_pair devices = devices_from_id (id);
    size_t terminal = circuit_device_terminal_index_from_id (id);

    std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (devices);
    std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> termdefs = terminal_defs_from_device_classes (device_classes, terminal);

    if (index.column () == m_object_column) {

      return escaped (str_from_names (termdefs, mp_indexer->is_single ()));

    } else if (index.column () == m_first_column || index.column () == m_second_column) {

      IndexedNetlistModel::net_pair nets = nets_from_device_terminals (devices, termdefs);
      return make_link_to (nets, index.column ());

    }

  } else if (is_id_circuit_subcircuit (id)) {

    //  circuit/subcircuit: header column = circuit name, second column subcircuit name
    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_id (id);
    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (subcircuits);
    if (index.column () == m_object_column) {
      return make_link_to (circuit_refs);
    } else if (index.column () == m_first_column) {
      return escaped (str_from_expanded_name (subcircuits.first));
    } else if (index.column () == m_second_column) {
      return escaped (str_from_expanded_name (subcircuits.second));
    }

  } else if (is_id_circuit_subcircuit_pin (id)) {

    //  circuit/pin: header column = pin name, other columns net name
    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_id (id);
    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (subcircuits);
    IndexedNetlistModel::pin_pair pins = pins_from_id (id);

    if (index.column () == m_object_column) {
      return make_link_to (pins, circuit_refs);
    } else if (index.column () == m_first_column || index.column () == m_second_column) {
      return make_link_to (nets_from_subcircuit_pins (subcircuits, pins), index.column ());
    }

  } else if (is_id_circuit_net (id)) {

    //  circuit/net: header column = node count, second column net name
    IndexedNetlistModel::net_pair nets = nets_from_id (id);
    if (index.column () == m_object_column) {
      return escaped (str_from_expanded_names (nets, mp_indexer->is_single ()));
    } else if (index.column () == m_first_column && nets.first) {
      return escaped (nets.first->expanded_name () + " (" + tl::to_string (nets.first->pin_count () + nets.first->terminal_count () + nets.first->subcircuit_pin_count ()) + ")");
    } else if (index.column () == m_second_column && nets.second) {
      return escaped (nets.second->expanded_name () + " (" + tl::to_string (nets.second->pin_count () + nets.second->terminal_count () + nets.second->subcircuit_pin_count ()) + ")");
    }

  } else if (is_id_circuit_net_pin (id)) {

    //  circuit/net/pin: header column = pin name, second column empty (for now)
    IndexedNetlistModel::net_pin_pair pinrefs = net_pinrefs_from_id (id);
    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
    if (mp_indexer->is_single () && index.column () == m_object_column) {
      return make_link_to (pins_from_pinrefs (pinrefs), circuits);
    } else if (! mp_indexer->is_single () && (index.column () == m_first_column || index.column () == m_second_column)) {
      return make_link_to (pins_from_pinrefs (pinrefs), circuits, index.column ());
    }

  } else if (is_id_circuit_net_subcircuit_pin (id)) {

    //  circuit/net/pin: header column = pin name, second column empty (for now)
    IndexedNetlistModel::net_subcircuit_pin_pair pinrefs = net_subcircuit_pinrefs_from_id (id);
    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_pinrefs (pinrefs);
    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (subcircuits);

    if (index.column () == m_object_column) {
      return make_link_to (pins_from_pinrefs (pinrefs), circuit_refs) + tl::to_qstring (field_sep) + make_link_to (circuit_refs);
    } else if (index.column () == m_first_column || index.column () == m_second_column) {
      return make_link_to (subcircuits_from_pinrefs (pinrefs), index.column ());
    }

  } else if (is_id_circuit_net_subcircuit_pin_others (id)) {

    //  circuit/net/device terminal/more: header column = pin name, second column = net link
    IndexedNetlistModel::net_subcircuit_pin_pair pinrefs = net_subcircuit_pinrefs_from_id (id);
    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_pinrefs (pinrefs);
    size_t other_index = circuit_net_subcircuit_pin_other_index_from_id (id);

    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (subcircuits);
    IndexedNetlistModel::pin_pair pins = mp_indexer->pin_from_index (circuit_refs, other_index).first;

    if (index.column () == m_object_column) {
      return make_link_to (pins, circuit_refs);
    } else if (index.column () == m_first_column || index.column () == m_second_column) {
      return make_link_to (nets_from_subcircuit_pins (subcircuits, pins), index.column ());
    }

  } else if (is_id_circuit_net_device_terminal (id)) {

    //  circuit/net/device terminal: header column = terminal and device string, second column = device name
    IndexedNetlistModel::net_terminal_pair refs = net_terminalrefs_from_id (id);
    IndexedNetlistModel::device_pair devices = devices_from_termrefs (refs);

    if (index.column () == m_object_column) {

      std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> termdefs = terminal_defs_from_terminal_refs (refs);

      if (mp_indexer->is_single ()) {
        return escaped (str_from_name (termdefs.first) + field_sep + device_string (devices.first));
      } else {
        return escaped (str_from_names (termdefs, mp_indexer->is_single ()) + field_sep + devices_string (devices, mp_indexer->is_single (), true /*with parameters*/));
      }

    } else if (index.column () == m_first_column || index.column () == m_second_column) {
      return make_link_to (devices, index.column ());
    }

  } else if (is_id_circuit_net_device_terminal_others (id)) {

    //  circuit/net/device terminal/more: header column = terminal name, second column = net link
    IndexedNetlistModel::net_terminal_pair refs = net_terminalrefs_from_id (id);
    size_t other_index = circuit_net_device_terminal_other_index_from_id (id);

    IndexedNetlistModel::device_pair devices = devices_from_termrefs (refs);
    std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (devices);
    std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> termdefs = terminal_defs_from_device_classes (device_classes, other_index);

    if (index.column () == m_object_column) {

      return escaped (str_from_names (termdefs, mp_indexer->is_single ()));

    } else if (index.column () == m_first_column || index.column () == m_second_column) {

      IndexedNetlistModel::net_pair nets = nets_from_device_terminals (devices, termdefs);
      return make_link_to (nets, index.column ());

    }

  }

  return QString ();
}

static std::string combine_search_strings (const std::string &s1, const std::string &s2)
{
  if (s1.empty ()) {
    return s2;
  } else if (s2.empty ()) {
    return s1;
  } else {
    return s1 + "|" + s2;
  }
}

template <class Obj>
static std::string search_string_from_expanded_names (const std::pair<const Obj *, const Obj *> &objs)
{
  if (objs.first && objs.second) {
    return combine_search_strings (objs.first->expanded_name (), objs.second->expanded_name ());
  } else if (objs.first) {
    return objs.first->expanded_name ();
  } else if (objs.second) {
    return objs.second->expanded_name ();
  } else {
    return std::string ();
  }
}

template <class Obj>
static std::string search_string_from_names (const std::pair<const Obj *, const Obj *> &objs)
{
  if (objs.first && objs.second) {
    return combine_search_strings (objs.first->name (), objs.second->name ());
  } else if (objs.first) {
    return objs.first->name ();
  } else if (objs.second) {
    return objs.second->name ();
  } else {
    return std::string ();
  }
}

bool
NetlistBrowserModel::is_valid_net_pair (const std::pair<const db::Net *, const db::Net *> &nets) const
{
  if (! nets.first && ! nets.second) {
    //  this is a valid case: e.g. two matching subcircuit pins without nets attached
    //  to them
    return true;
  } else {
    IndexedNetlistModel::circuit_pair net_parent = mp_indexer->parent_of (nets);
    return (net_parent.first != 0 || net_parent.second != 0);
  }
}

db::NetlistCrossReference::Status
NetlistBrowserModel::status (const QModelIndex &index) const
{
  void *id = index.internalPointer ();

  if (is_id_circuit (id)) {

    size_t index = circuit_index_from_id (id);
    return mp_indexer->circuit_from_index (index).second;

  } else if (is_id_circuit_pin (id)) {

    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
    size_t index = circuit_pin_index_from_id (id);
    return mp_indexer->pin_from_index (circuits, index).second;

  } else if (is_id_circuit_device (id)) {

    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
    size_t index = circuit_device_index_from_id (id);
    return mp_indexer->device_from_index (circuits, index).second;

  } else if (is_id_circuit_device_terminal (id)) {

    IndexedNetlistModel::device_pair devices = devices_from_id (id);
    std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (devices);
    size_t terminal = circuit_device_terminal_index_from_id (id);

    std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> termdefs = terminal_defs_from_device_classes (device_classes, terminal);

    if (! is_valid_net_pair (nets_from_device_terminals (devices, termdefs))) {
      //  This indicates a wrong connection: the nets are associated in a way which is a not
      //  corresponding to a mapped net pair. Report Mismatch here.
      return db::NetlistCrossReference::NoMatch;
    }

  } else if (is_id_circuit_subcircuit (id)) {

    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
    size_t index = circuit_subcircuit_index_from_id (id);
    return mp_indexer->subcircuit_from_index (circuits, index).second;

  } else if (is_id_circuit_subcircuit_pin (id)) {

    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_id (id);
    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (subcircuits);
    IndexedNetlistModel::pin_pair pins = pins_from_id (id);

    db::NetlistCrossReference::Status status = mp_indexer->pin_from_index (circuit_refs, mp_indexer->pin_index (pins, circuit_refs)).second;
    if (status == db::NetlistCrossReference::Mismatch || status == db::NetlistCrossReference::NoMatch) {
      return status;
    }

    //  Another test here is to check whether the pins may be attached to an invalid net pair
    if (! is_valid_net_pair (nets_from_subcircuit_pins (subcircuits, pins))) {
      //  This indicates a wrong connection: the nets are associated in a way which is a not
      //  corresponding to a mapped net pair. Report Mismatch here.
      return db::NetlistCrossReference::NoMatch;
    }

  } else if (is_id_circuit_net (id)) {

    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
    size_t index = circuit_net_index_from_id (id);
    return mp_indexer->net_from_index (circuits, index).second;

  } else if (is_id_circuit_net_device_terminal (id)) {

    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
    IndexedNetlistModel::net_terminal_pair termrefs = net_terminalrefs_from_id (id);
    IndexedNetlistModel::device_pair devices = devices_from_termrefs (termrefs);

    return mp_indexer->device_from_index (circuits, mp_indexer->device_index (devices)).second;

  } else if (is_id_circuit_net_device_terminal_others (id)) {

    IndexedNetlistModel::net_terminal_pair termrefs = net_terminalrefs_from_id (id);
    size_t other_index = circuit_net_device_terminal_other_index_from_id (id);

    IndexedNetlistModel::device_pair devices = devices_from_termrefs (termrefs);
    std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (devices);
    std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> termdefs = terminal_defs_from_device_classes (device_classes, other_index);

    if (! is_valid_net_pair (nets_from_device_terminals (devices, termdefs))) {
      //  This indicates a wrong connection: the nets are associated in a way which is a not
      //  corresponding to a mapped net pair. Report Mismatch here.
      return db::NetlistCrossReference::NoMatch;
    }

  } else if (is_id_circuit_net_subcircuit_pin (id)) {

    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
    IndexedNetlistModel::net_subcircuit_pin_pair pinrefs = net_subcircuit_pinrefs_from_id (id);
    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_pinrefs (pinrefs);

    return mp_indexer->subcircuit_from_index (circuits, mp_indexer->subcircuit_index (subcircuits)).second;

  } else if (is_id_circuit_net_subcircuit_pin_others (id)) {

    IndexedNetlistModel::net_subcircuit_pin_pair pinrefs = net_subcircuit_pinrefs_from_id (id);
    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_pinrefs (pinrefs);
    size_t other_index = circuit_net_subcircuit_pin_other_index_from_id (id);

    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (subcircuits);
    IndexedNetlistModel::pin_pair pins = mp_indexer->pin_from_index (circuit_refs, other_index).first;

    if (! is_valid_net_pair (nets_from_subcircuit_pins (subcircuits, pins))) {
      //  This indicates a wrong connection: the nets are associated in a way which is a not
      //  corresponding to a mapped net pair. Report Mismatch here.
      return db::NetlistCrossReference::NoMatch;
    }

  }

  return db::NetlistCrossReference::None;
}

static std::string rewire_subcircuit_pins_status_hint ()
{
  return tl::to_string (tr ("Either pins or nets don't form a good pair.\nThis means either pin swapping happens (in this case, the nets will still match)\nor the subcircuit wiring is not correct (you'll see an error on the net)."));
}

QVariant
NetlistBrowserModel::tooltip (const QModelIndex &index) const
{
  void *id = index.internalPointer ();
  std::string hint;

  if (is_id_circuit (id)) {

    size_t index = circuit_index_from_id (id);
    hint = mp_indexer->circuit_status_hint (index);

  } else if (is_id_circuit_pin (id)) {

    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
    size_t index = circuit_pin_index_from_id (id);
    hint = mp_indexer->pin_status_hint (circuits, index);

  } else if (is_id_circuit_device (id)) {

    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
    size_t index = circuit_device_index_from_id (id);
    hint = mp_indexer->device_status_hint (circuits, index);

  } else if (is_id_circuit_subcircuit (id)) {

    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
    size_t index = circuit_subcircuit_index_from_id (id);
    hint = mp_indexer->subcircuit_status_hint (circuits, index);

  } else if (is_id_circuit_subcircuit_pin (id)) {

    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_id (id);
    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (subcircuits);
    IndexedNetlistModel::pin_pair pins = pins_from_id (id);

    hint = mp_indexer->pin_status_hint (circuit_refs, mp_indexer->pin_index (pins, circuit_refs));
    if (hint.empty ()) {

      //  Another test here is to check whether the pins may be attached to an invalid net pair
      if (! is_valid_net_pair (nets_from_subcircuit_pins (subcircuits, pins))) {
        hint = rewire_subcircuit_pins_status_hint ();
      }

    }

  } else if (is_id_circuit_net (id)) {

    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
    size_t index = circuit_net_index_from_id (id);
    hint = mp_indexer->net_status_hint (circuits, index);

  } else if (is_id_circuit_net_device_terminal (id)) {

    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
    IndexedNetlistModel::net_terminal_pair termrefs = net_terminalrefs_from_id (id);
    IndexedNetlistModel::device_pair devices = devices_from_termrefs (termrefs);

    hint = mp_indexer->device_status_hint (circuits, mp_indexer->device_index (devices));

  } else if (is_id_circuit_net_subcircuit_pin (id)) {

    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
    IndexedNetlistModel::net_subcircuit_pin_pair pinrefs = net_subcircuit_pinrefs_from_id (id);
    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_pinrefs (pinrefs);

    hint = mp_indexer->subcircuit_status_hint (circuits, mp_indexer->subcircuit_index (subcircuits));

  } else if (is_id_circuit_net_subcircuit_pin_others (id)) {

    IndexedNetlistModel::net_subcircuit_pin_pair pinrefs = net_subcircuit_pinrefs_from_id (id);
    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_pinrefs (pinrefs);
    size_t other_index = circuit_net_subcircuit_pin_other_index_from_id (id);

    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (subcircuits);
    IndexedNetlistModel::pin_pair pins = mp_indexer->pin_from_index (circuit_refs, other_index).first;

    if (! is_valid_net_pair (nets_from_subcircuit_pins (subcircuits, pins))) {
      //  This indicates a wrong connection: the nets are associated in a way which is a not
      //  corresponding to a mapped net pair. Report Mismatch here.
      hint = rewire_subcircuit_pins_status_hint ();
    }

  }

  if (hint.empty ()) {
    return QVariant ();
  } else {
    return QVariant (tl::to_qstring (hint));
  }
}

QString
NetlistBrowserModel::search_text (const QModelIndex &index) const
{
  void *id = index.internalPointer ();

  if (is_id_circuit (id)) {

    return tl::to_qstring (search_string_from_names (circuits_from_id (id)));

  } else if (is_id_circuit_pin (id)) {

    return tl::to_qstring (search_string_from_expanded_names (pins_from_id (id)));

  } else if (is_id_circuit_pin_net (id)) {

    return tl::to_qstring (search_string_from_expanded_names (nets_from_circuit_pins (circuits_from_id (id), pins_from_id (id))));

  } else if (is_id_circuit_device (id)) {

    IndexedNetlistModel::device_pair devices = devices_from_id (id);
    std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (devices);
    return tl::to_qstring (combine_search_strings (search_string_from_expanded_names (devices), search_string_from_names (device_classes)));

  } else if (is_id_circuit_device_terminal (id)) {

    IndexedNetlistModel::device_pair devices = devices_from_id (id);
    std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (devices);
    size_t terminal = circuit_device_terminal_index_from_id (id);

    std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> termdefs = terminal_defs_from_device_classes (device_classes, terminal);
    IndexedNetlistModel::net_pair nets = nets_from_device_terminals (devices, termdefs);

    return tl::to_qstring (combine_search_strings (search_string_from_names (termdefs), search_string_from_expanded_names (nets)));

  } else if (is_id_circuit_subcircuit (id)) {

    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_id (id);
    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (subcircuits);

    return tl::to_qstring (combine_search_strings (search_string_from_names (circuit_refs), search_string_from_expanded_names (subcircuits)));

  } else if (is_id_circuit_subcircuit_pin (id)) {

    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_id (id);
    IndexedNetlistModel::pin_pair pins = pins_from_id (id);
    IndexedNetlistModel::net_pair nets = nets_from_subcircuit_pins (subcircuits, pins);

    return tl::to_qstring (combine_search_strings (search_string_from_names (pins), search_string_from_expanded_names (nets)));

  } else if (is_id_circuit_net (id)) {

    return tl::to_qstring (search_string_from_expanded_names (nets_from_id (id)));

  } else if (is_id_circuit_net_pin (id)) {

    IndexedNetlistModel::net_pin_pair pinrefs = net_pinrefs_from_id (id);
    IndexedNetlistModel::pin_pair pins = pins_from_pinrefs (pinrefs);

    return tl::to_qstring (search_string_from_names (pins));

  } else if (is_id_circuit_net_subcircuit_pin (id)) {

    IndexedNetlistModel::net_subcircuit_pin_pair pinrefs = net_subcircuit_pinrefs_from_id (id);
    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_pinrefs (pinrefs);
    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (subcircuits);
    IndexedNetlistModel::pin_pair pins = pins_from_pinrefs (pinrefs);

    return tl::to_qstring (combine_search_strings (combine_search_strings (search_string_from_names (pins), search_string_from_names (circuit_refs)), search_string_from_expanded_names (subcircuits)));

  } else if (is_id_circuit_net_subcircuit_pin_others (id)) {

    IndexedNetlistModel::net_subcircuit_pin_pair pinrefs = net_subcircuit_pinrefs_from_id (id);
    size_t other_index = circuit_net_subcircuit_pin_other_index_from_id (id);

    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_pinrefs (pinrefs);
    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (subcircuits);

    IndexedNetlistModel::pin_pair pins = mp_indexer->pin_from_index (circuit_refs, other_index).first;
    IndexedNetlistModel::net_pair nets = nets_from_circuit_pins (circuit_refs, pins);

    return tl::to_qstring (combine_search_strings (search_string_from_names (pins), search_string_from_expanded_names (nets)));

  } else if (is_id_circuit_net_device_terminal (id)) {

    IndexedNetlistModel::net_terminal_pair termrefs = net_terminalrefs_from_id (id);
    IndexedNetlistModel::device_pair devices = devices_from_termrefs (termrefs);
    std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (devices);
    std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> termdefs = terminal_defs_from_terminal_refs (termrefs);

    return tl::to_qstring (combine_search_strings (combine_search_strings (search_string_from_names (termdefs), search_string_from_names (device_classes)), search_string_from_expanded_names (devices)));

  } else if (is_id_circuit_net_device_terminal_others (id)) {

    IndexedNetlistModel::net_terminal_pair termrefs = net_terminalrefs_from_id (id);
    size_t other_index = circuit_net_device_terminal_other_index_from_id (id);

    IndexedNetlistModel::device_pair devices = devices_from_termrefs (termrefs);
    std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (devices);
    std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> termdefs = terminal_defs_from_device_classes (device_classes, other_index);

    IndexedNetlistModel::net_pair nets = nets_from_device_terminals (devices, termdefs);

    return tl::to_qstring (combine_search_strings (search_string_from_names (termdefs), search_string_from_expanded_names (nets)));

  }

  return QString ();
}

static QIcon icon_for_net ()
{
  QIcon icon;
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_net_48.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_net_32.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_net_24.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_net_16.png")));
  return icon;
}

static QIcon light_icon_for_net ()
{
  QIcon icon;
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_net_light_48.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_net_light_32.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_net_light_24.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_net_light_16.png")));
  return icon;
}

static QIcon icon_for_connection ()
{
  QIcon icon;
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_conn_48.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_conn_32.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_conn_24.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_conn_16.png")));
  return icon;
}

static QIcon light_icon_for_connection ()
{
  QIcon icon;
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_conn_light_48.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_conn_light_32.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_conn_light_24.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_conn_light_16.png")));
  return icon;
}

static QIcon icon_for_pin ()
{
  QIcon icon;
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_pin_48.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_pin_32.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_pin_24.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_pin_16.png")));
  return icon;
}

static QIcon icon_for_device (const db::DeviceClass *dc)
{
  QIcon icon;
  //  TODO: inductor, generic device ...
  if (dynamic_cast<const db::DeviceClassResistor *> (dc)) {
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_res_48.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_res_32.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_res_24.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_res_16.png")));
  } else if (dynamic_cast<const db::DeviceClassInductor *> (dc)) {
    //  fake ...
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_res_48.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_res_32.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_res_24.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_res_16.png")));
  } else if (dynamic_cast<const db::DeviceClassCapacitor *> (dc)) {
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_cap_48.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_cap_32.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_cap_24.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_cap_16.png")));
  } else if (dynamic_cast<const db::DeviceClassDiode *> (dc)) {
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_diode_48.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_diode_32.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_diode_24.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_diode_16.png")));
  } else if (dynamic_cast<const db::DeviceClassBJT3Transistor *> (dc)) {
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_bjt_48.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_bjt_32.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_bjt_24.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_bjt_16.png")));
  } else {
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_mos_48.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_mos_32.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_mos_24.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_mos_16.png")));
  }
  return icon;
}

static QIcon icon_for_devices (const std::pair<const db::DeviceClass *, const db::DeviceClass *> &device_classes)
{
  return icon_for_device (device_classes.first ? device_classes.first : device_classes.second);
}

static QIcon icon_for_circuit ()
{
  QIcon icon;
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_circuit_48.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_circuit_32.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_circuit_24.png")));
  icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_circuit_16.png")));
  return icon;
}

static QIcon colored_icon (const QColor &color, const QIcon &original_icon)
{
  if (! color.isValid ()) {
    return icon_for_net ();
  }

  QIcon colored_icon;

  QList<QSize> sizes = original_icon.availableSizes ();
  for (QList<QSize>::const_iterator i = sizes.begin (); i != sizes.end (); ++i) {

    QImage image (*i, QImage::Format_ARGB32);
    image.fill (Qt::transparent);
    QPainter painter (&image);
    original_icon.paint (&painter, 0, 0, i->width (), i->height ());

    for (int x = 0; x < i->width (); ++x) {
      for (int y = 0; y < i->height (); ++y) {
        QRgb pixel = image.pixel (x, y);
        if (pixel != 0xffffffff) {
          pixel = (pixel & ~RGB_MASK) | (color.rgb () & RGB_MASK);
          image.setPixel (x, y, pixel);
        }
      }

    }

    colored_icon.addPixmap (QPixmap::fromImage (image));

  }

  return colored_icon;
}

static QIcon net_icon_with_color (const QColor &color)
{
  return colored_icon (color, light_icon_for_net ());
}

static QIcon connection_icon_with_color (const QColor &color)
{
  return colored_icon (color, light_icon_for_connection ());
}

QIcon
NetlistBrowserModel::icon_for_nets (const std::pair<const db::Net *, const db::Net *> &nets) const
{
  const db::Net *net = nets.first;

  if (mp_colorizer && mp_colorizer->has_color_for_net (net)) {

    QColor color = mp_colorizer->color_of_net (net);

    lay::color_t rgb = lay::color_t (color.rgb ());
    std::map<lay::color_t, QIcon>::const_iterator c = m_net_icon_per_color.find (rgb);
    if (c == m_net_icon_per_color.end ()) {
      c = m_net_icon_per_color.insert (std::make_pair (rgb, net_icon_with_color (color))).first;
    }

    return c->second;

  } else {
    return lay::icon_for_net ();
  }
}

QIcon
NetlistBrowserModel::icon_for_connection (const std::pair<const db::Net *, const db::Net *> &nets) const
{
  const db::Net *net = nets.first;

  if (mp_colorizer && mp_colorizer->has_color_for_net (net)) {

    QColor color = mp_colorizer->color_of_net (net);

    lay::color_t rgb = lay::color_t (color.rgb ());
    std::map<lay::color_t, QIcon>::const_iterator c = m_connection_icon_per_color.find (rgb);
    if (c == m_connection_icon_per_color.end ()) {
      c = m_connection_icon_per_color.insert (std::make_pair (rgb, connection_icon_with_color (color))).first;
    }

    return c->second;

  } else {
    return lay::icon_for_connection ();
  }
}

QIcon
NetlistBrowserModel::icon (const QModelIndex &index) const
{
  void *id = index.internalPointer ();

  if (is_id_circuit (id)) {
    return icon_for_circuit ();
  } else if (is_id_circuit_pin (id)) {
    return icon_for_pin ();
  } else if (is_id_circuit_net (id)) {

    IndexedNetlistModel::net_pair nets = net_from_index (index);
    return icon_for_nets (nets);

  } else if (is_id_circuit_device (id)) {

    IndexedNetlistModel::device_pair devices = devices_from_id (id);
    std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (devices);

    return icon_for_devices (device_classes);

  } else if (is_id_circuit_pin_net (id) || is_id_circuit_device_terminal (id) || is_id_circuit_net_device_terminal_others (id) || is_id_circuit_net_subcircuit_pin_others (id)) {

    IndexedNetlistModel::net_pair nets = net_from_index (index);
    return icon_for_connection (nets);

  } else if (is_id_circuit_subcircuit (id)) {
    return icon_for_circuit ();
  } else if (is_id_circuit_subcircuit_pin (id) || is_id_circuit_net_pin (id)) {
    return icon_for_pin ();
  } else if (is_id_circuit_net_subcircuit_pin (id)) {
    return icon_for_circuit ();
  } else if (is_id_circuit_net_device_terminal (id)) {

    IndexedNetlistModel::net_terminal_pair termrefs = net_terminalrefs_from_id (id);
    IndexedNetlistModel::device_pair devices = devices_from_termrefs (termrefs);
    std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (devices);

    return icon_for_devices (device_classes);

  }

  return QIcon ();
}

Qt::ItemFlags
NetlistBrowserModel::flags (const QModelIndex & /*index*/) const
{
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

static size_t rows_for (const db::Device *device)
{
  if (! device || ! device->device_class ()) {
    return 0;
  } else {
    return device->device_class ()->terminal_definitions ().size ();
  }
}

static size_t rows_for (const db::SubCircuit *subcircuit)
{
  if (! subcircuit || ! subcircuit->circuit_ref ()) {
    return 0;
  } else {
    return subcircuit->circuit_ref ()->pin_count ();
  }
}

static size_t rows_for (const db::NetSubcircuitPinRef *ref)
{
  if (! ref || ! ref->subcircuit () || ! ref->subcircuit ()->circuit_ref ()) {
    return 0;
  } else {
    return ref->subcircuit ()->circuit_ref ()->pin_count ();
  }
}

static size_t rows_for (const db::NetTerminalRef *ref)
{
  if (! ref || ! ref->device_class ()) {
    return 0;
  } else {
    return ref->device_class ()->terminal_definitions ().size ();
  }
}

bool
NetlistBrowserModel::hasChildren (const QModelIndex &parent) const
{
  if (! parent.isValid ()) {

    return mp_indexer.get () && mp_indexer->circuit_count () > 0;

  } else {

    void *id = parent.internalPointer ();

    if (is_id_circuit (id)) {

      IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
      return mp_indexer->device_count (circuits) > 0 ||
             mp_indexer->subcircuit_count (circuits) > 0 ||
             mp_indexer->pin_count (circuits) > 0 ||
             mp_indexer->net_count (circuits) > 0;

    } else if (is_id_circuit_pin (id)) {

      return true;

    } else if (is_id_circuit_device (id)) {

      IndexedNetlistModel::device_pair devices = devices_from_id (id);
      return rows_for (devices.first) > 0 || rows_for (devices.second) > 0;

    } else if (is_id_circuit_subcircuit (id)) {

      IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_id (id);
      return rows_for (subcircuits.first) > 0 || rows_for (subcircuits.second) > 0;

    } else if (is_id_circuit_net (id)) {

      IndexedNetlistModel::net_pair nets = nets_from_id (id);
      return mp_indexer->net_pin_count (nets) > 0 ||
             mp_indexer->net_terminal_count (nets) > 0 ||
             mp_indexer->net_subcircuit_pin_count (nets) > 0;

    } else if (is_id_circuit_net_subcircuit_pin (id)) {

      IndexedNetlistModel::net_subcircuit_pin_pair refs = net_subcircuit_pinrefs_from_id (id);
      return rows_for (refs.first) > 0 || rows_for (refs.second) > 0;

    } else if (is_id_circuit_net_device_terminal (id)) {

      IndexedNetlistModel::net_terminal_pair refs = net_terminalrefs_from_id (id);
      return rows_for (refs.first) > 0 || rows_for (refs.second) > 0;

    } else {
      return false;
    }

  }
}

QVariant
NetlistBrowserModel::headerData (int section, Qt::Orientation /*orientation*/, int role) const
{
  if (role == Qt::DisplayRole) {

    if (mp_indexer->is_single ()) {
      if (section == m_object_column) {
        return tr ("Object");
      } else if (section == m_first_column) {
        return tr ("Connections");
      }
    } else {
      if (section == m_object_column) {
        return tr ("Objects");
      } else if (section == m_first_column) {
        return tr ("Layout");
      } else if (section == m_second_column) {
        return tr ("Reference");
      }
    }

  } else if (role == Qt::DecorationRole && section == m_status_column) {
    return QIcon (":/info_16.png");
  }

  return QVariant ();
}

QModelIndex
NetlistBrowserModel::index (int row, int column, const QModelIndex &parent) const
{
  void *new_id = no_id;

  if (! parent.isValid ()) {

    new_id = make_id_circuit (row);

  } else {

    void *id = parent.internalPointer ();

    if (is_id_circuit (id)) {

      int r = row;
      IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
      int rpins = int (mp_indexer->pin_count (circuits));
      if (r < rpins) {
        new_id = make_id_circuit_pin (circuit_index_from_id (id), size_t (r));
      } else {
        r -= rpins;
        int rnets = int (mp_indexer->net_count (circuits));
        if (r < int (rnets)) {
          new_id = make_id_circuit_net (circuit_index_from_id (id), size_t (r));
        } else {
          r -= int (rnets);
          int rsubcircuits = int (mp_indexer->subcircuit_count (circuits));
          if (r < rsubcircuits) {
            new_id = make_id_circuit_subcircuit (circuit_index_from_id (id), size_t (r));
          } else {
            r -= rsubcircuits;
            if (r < int (mp_indexer->device_count (circuits))) {
              new_id = make_id_circuit_device (circuit_index_from_id (id), size_t (r));
            }
          }
        }
      }

    } else if (is_id_circuit_pin (id)) {

      new_id = make_id_circuit_pin_net (circuit_index_from_id (id), circuit_pin_index_from_id (id), size_t (row));

    } else if (is_id_circuit_device (id)) {

      new_id = make_id_circuit_device_terminal (circuit_index_from_id (id), circuit_device_index_from_id (id), size_t (row));

    } else if (is_id_circuit_subcircuit (id)) {

      new_id = make_id_circuit_subcircuit_pin (circuit_index_from_id (id), circuit_subcircuit_index_from_id (id), size_t (row));

    } else if (is_id_circuit_net (id)) {

      int r = row;
      IndexedNetlistModel::net_pair nets = nets_from_id (id);
      int rterminals = int (mp_indexer->net_terminal_count (nets));
      if (r < rterminals){
        new_id = make_id_circuit_net_device_terminal (circuit_index_from_id (id), circuit_net_index_from_id (id), size_t (r));
      } else {
        r -= rterminals;
        int rpins = int (mp_indexer->net_pin_count (nets));
        if (r < rpins) {
          new_id = make_id_circuit_net_pin (circuit_index_from_id (id), circuit_net_index_from_id (id), size_t (r));
        } else {
          r -= rpins;
          if (r < int (mp_indexer->net_subcircuit_pin_count (nets))) {
            new_id = make_id_circuit_net_subcircuit_pin (circuit_index_from_id (id), circuit_net_index_from_id (id), size_t (r));
          }
        }
      }

    } else if (is_id_circuit_net_subcircuit_pin (id)) {

      new_id = make_id_circuit_net_subcircuit_pin_others (circuit_index_from_id (id), circuit_net_index_from_id (id), circuit_net_subcircuit_pin_index_from_id (id), size_t (row));

    } else if (is_id_circuit_net_device_terminal (id)) {

      new_id = make_id_circuit_net_device_terminal_others (circuit_index_from_id (id), circuit_net_index_from_id (id), circuit_net_device_terminal_index_from_id (id), size_t (row));

    }

  }

  if (new_id != no_id) {
    return createIndex (row, column, new_id);
  } else {
    return QModelIndex ();
  }
}

void
NetlistBrowserModel::colors_changed ()
{
  emit dataChanged (index (0, 0, QModelIndex ()), index (rowCount (QModelIndex()) - 1, 0, QModelIndex ()));
}

QModelIndex
NetlistBrowserModel::index_from_net (const std::pair<const db::Net *, const db::Net *> &nets) const
{
  IndexedNetlistModel::circuit_pair circuits (nets.first ? nets.first->circuit () : 0, nets.second ? nets.second->circuit () : 0);
  void *id = make_id_circuit_net (mp_indexer->circuit_index (circuits), mp_indexer->net_index (nets));
  return index_from_id (id, 0);
}

QModelIndex
NetlistBrowserModel::index_from_net (const db::Net *net) const
{
  return index_from_net (std::make_pair (net, mp_indexer->second_net_for (net)));
}

QModelIndex
NetlistBrowserModel::index_from_circuit (const std::pair<const db::Circuit *, const db::Circuit *> &circuits) const
{
  void *id = make_id_circuit (mp_indexer->circuit_index (circuits));
  return index_from_id (id, 0);
}

QModelIndex
NetlistBrowserModel::index_from_circuit (const db::Circuit *net) const
{
  return index_from_circuit (std::make_pair (net, mp_indexer->second_circuit_for (net)));
}

std::pair<const db::Circuit *, const db::Circuit *>
NetlistBrowserModel::circuit_from_index (const QModelIndex &index) const
{
  return circuits_from_id (index.internalPointer ());
}

std::pair<const db::Net *, const db::Net *>
NetlistBrowserModel::net_from_index (const QModelIndex &index) const
{
  void *id = index.internalPointer ();
  if (is_id_circuit_net (id)) {

    return nets_from_id (id);

  } else if (is_id_circuit_device_terminal (id)) {

    IndexedNetlistModel::device_pair devices = devices_from_id (id);
    std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (devices);

    size_t terminal = circuit_device_terminal_index_from_id (id);
    std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> termdefs = terminal_defs_from_device_classes (device_classes, terminal);

    return nets_from_device_terminals (devices, termdefs);

  } else if (is_id_circuit_pin_net (id)) {

    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
    IndexedNetlistModel::pin_pair pins = pins_from_id (id);
    return nets_from_circuit_pins (circuits, pins);

  } else if (is_id_circuit_subcircuit_pin (id)) {

    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_id (id);
    IndexedNetlistModel::pin_pair pins = pins_from_id (id);
    return nets_from_subcircuit_pins (subcircuits, pins);

  } else if (is_id_circuit_net_subcircuit_pin_others (id)) {

    IndexedNetlistModel::net_subcircuit_pin_pair pinrefs = net_subcircuit_pinrefs_from_id (id);
    size_t other_index = circuit_net_subcircuit_pin_other_index_from_id (id);

    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_pinrefs (pinrefs);
    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (subcircuits);
    IndexedNetlistModel::pin_pair pins = mp_indexer->pin_from_index (circuit_refs, other_index).first;

    return nets_from_subcircuit_pins (subcircuits, pins);

  } else if (is_id_circuit_net_device_terminal_others (id)) {

    IndexedNetlistModel::net_terminal_pair termrefs = net_terminalrefs_from_id (id);
    size_t other_index = circuit_net_device_terminal_other_index_from_id (id);

    IndexedNetlistModel::device_pair devices = devices_from_termrefs (termrefs);
    std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (devices);
    std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> termdefs = terminal_defs_from_device_classes (device_classes, other_index);

    return nets_from_device_terminals (devices, termdefs);

  }

  return std::make_pair ((const db::Net *) 0, (const db::Net *) 0);
}

std::pair<const db::Device *, const db::Device *>
NetlistBrowserModel::device_from_index (const QModelIndex &index) const
{
  void *id = index.internalPointer ();

  if (is_id_circuit_device (id)) {

    return devices_from_id (id);

  } else if (is_id_circuit_net_device_terminal (id)) {

    IndexedNetlistModel::net_terminal_pair termrefs = net_terminalrefs_from_id (id);
    return devices_from_termrefs (termrefs);

  }

  return std::make_pair ((const db::Device *) 0, (const db::Device *) 0);
}

std::pair<const db::SubCircuit *, const db::SubCircuit *>
NetlistBrowserModel::subcircuit_from_index (const QModelIndex &index) const
{
  void *id = index.internalPointer ();

  if (is_id_circuit_subcircuit (id)) {

    return subcircuits_from_id (id);

  } else if (is_id_circuit_net_subcircuit_pin (id)) {

    IndexedNetlistModel::net_subcircuit_pin_pair pinrefs = net_subcircuit_pinrefs_from_id (id);
    return subcircuits_from_pinrefs (pinrefs);

  }

  return std::make_pair ((const db::SubCircuit *) 0, (const db::SubCircuit *) 0);
}

QModelIndex
NetlistBrowserModel::index_from_id (void *id, int column) const
{
  if (id == no_id) {

    return QModelIndex ();

  } else if (is_id_circuit (id)) {

    return createIndex (circuit_index_from_id (id), column, id);

  } else if (is_id_circuit_pin (id)) {

    return createIndex (int (circuit_pin_index_from_id (id)), column, id);

  } else if (is_id_circuit_pin_net (id)) {

    return createIndex (0, column, id);

  } else if (is_id_circuit_net (id)) {

    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
    return createIndex (int (mp_indexer->pin_count (circuits) + circuit_net_index_from_id (id)), column, id);

  } else if (is_id_circuit_net_device_terminal (id)) {

    return createIndex (circuit_net_device_terminal_index_from_id (id), column, id);

  } else if (is_id_circuit_net_device_terminal_others (id)) {

    return createIndex (circuit_net_device_terminal_other_index_from_id (id), column, id);

  } else if (is_id_circuit_net_pin (id)) {

    IndexedNetlistModel::net_pair nets = nets_from_id (id);
    return createIndex (int (mp_indexer->net_terminal_count (nets) + circuit_net_pin_index_from_id (id)), column, id);

  } else if (is_id_circuit_net_subcircuit_pin (id)) {

    IndexedNetlistModel::net_pair nets = nets_from_id (id);
    return createIndex (int (mp_indexer->net_terminal_count (nets) + mp_indexer->net_pin_count (nets) + circuit_net_subcircuit_pin_index_from_id (id)), column, id);

  } else if (is_id_circuit_net_subcircuit_pin_others (id)) {

    return createIndex (circuit_net_subcircuit_pin_other_index_from_id (id), column, id);

  } else if (is_id_circuit_subcircuit (id)) {

    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
    return createIndex (int (mp_indexer->pin_count (circuits) + mp_indexer->net_count (circuits) + circuit_subcircuit_index_from_id (id)), column, id);

  } else if (is_id_circuit_subcircuit_pin (id)) {

    return createIndex (int (circuit_subcircuit_pin_index_from_id (id)), column, id);

  } else if (is_id_circuit_device (id)) {

    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
    return createIndex (int (mp_indexer->pin_count (circuits) + mp_indexer->net_count (circuits) + mp_indexer->subcircuit_count (circuits) + circuit_device_index_from_id (id)), column, id);

  } else if (is_id_circuit_device_terminal (id)) {

    return createIndex (int (circuit_device_terminal_index_from_id (id)), column, id);

  }

  return QModelIndex ();
}

QModelIndex
NetlistBrowserModel::parent (const QModelIndex &index) const
{
  if (! index.isValid ()) {

    return QModelIndex ();

  } else {

    void *id = index.internalPointer ();
    int column = 0;

    if (is_id_circuit (id)) {

      return QModelIndex ();

    } else if (is_id_circuit_pin (id) || is_id_circuit_net (id) || is_id_circuit_device (id) || is_id_circuit_subcircuit (id)) {

      return createIndex (int (circuit_index_from_id (id)), column, make_id_circuit (circuit_index_from_id (id)));

    } else if (is_id_circuit_pin_net (id)) {

      return createIndex (int (circuit_pin_index_from_id (id)), column, make_id_circuit_pin (circuit_index_from_id (id), circuit_pin_index_from_id (id)));

    } else if (is_id_circuit_net_device_terminal (id) || is_id_circuit_net_pin (id) || is_id_circuit_net_subcircuit_pin (id)) {

      IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
      return createIndex (int (mp_indexer->pin_count (circuits) + circuit_net_index_from_id (id)), column, make_id_circuit_net (circuit_index_from_id (id), circuit_net_index_from_id (id)));

    } else if (is_id_circuit_subcircuit_pin (id)) {

      IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
      return createIndex (int (mp_indexer->pin_count (circuits) + mp_indexer->net_count (circuits) + circuit_subcircuit_index_from_id (id)), column, make_id_circuit_subcircuit (circuit_index_from_id (id), circuit_subcircuit_index_from_id (id)));

    } else if (is_id_circuit_device_terminal (id)) {

      IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
      return createIndex (int (mp_indexer->pin_count (circuits) + mp_indexer->net_count (circuits) + mp_indexer->subcircuit_count (circuits) + circuit_device_index_from_id (id)), column, make_id_circuit_device (circuit_index_from_id (id), circuit_device_index_from_id (id)));

    } else if (is_id_circuit_net_device_terminal_others (id)) {

      return createIndex (circuit_net_device_terminal_index_from_id (id), column, make_id_circuit_net_device_terminal (circuit_index_from_id (id), circuit_net_index_from_id (id), circuit_net_device_terminal_index_from_id (id)));

    } else if (is_id_circuit_net_subcircuit_pin_others (id)) {

      IndexedNetlistModel::net_pair nets = nets_from_id (id);
      return createIndex (size_t (mp_indexer->net_terminal_count (nets) + mp_indexer->net_pin_count (nets) + circuit_net_subcircuit_pin_index_from_id (id)), column, make_id_circuit_net_subcircuit_pin (circuit_index_from_id (id), circuit_net_index_from_id (id), circuit_net_subcircuit_pin_index_from_id (id)));

    }

  }

  return QModelIndex ();
}

int
NetlistBrowserModel::rowCount (const QModelIndex &parent) const
{
  if (! parent.isValid ()) {

    return int (mp_indexer.get () ? mp_indexer->circuit_count () : 0);

  } else {

    void *id = parent.internalPointer ();

    if (is_id_circuit (id)) {

      IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
      return int (mp_indexer->pin_count (circuits) + mp_indexer->net_count (circuits) + mp_indexer->subcircuit_count (circuits) + mp_indexer->device_count (circuits));

    } else if (is_id_circuit_pin (id)) {

      return 1;

    } else if (is_id_circuit_device (id)) {

      IndexedNetlistModel::device_pair devices = devices_from_id (id);
      return int (std::max (rows_for (devices.first), rows_for (devices.second)));

    } else if (is_id_circuit_subcircuit (id)) {

      IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_id (id);
      return int (std::max (rows_for (subcircuits.first), rows_for (subcircuits.second)));

    } else if (is_id_circuit_net_subcircuit_pin (id)) {

      IndexedNetlistModel::net_subcircuit_pin_pair refs = net_subcircuit_pinrefs_from_id (id);
      return std::max (rows_for (refs.first), rows_for (refs.second));

    } else if (is_id_circuit_net_device_terminal (id)) {

      IndexedNetlistModel::net_terminal_pair refs = net_terminalrefs_from_id (id);
      return std::max (rows_for (refs.first), rows_for (refs.second));

    } else if (is_id_circuit_net (id)) {

      IndexedNetlistModel::net_pair nets = nets_from_id (id);
      return int (mp_indexer->net_terminal_count (nets) + mp_indexer->net_pin_count (nets) + mp_indexer->net_subcircuit_pin_count (nets));

    }

  }

  return 0;
}

std::pair<const db::Circuit *, const db::Circuit *>
NetlistBrowserModel::circuits_from_id (void *id) const
{
  size_t index = circuit_index_from_id (id);
  return mp_indexer->circuit_from_index (index).first;
}

std::pair<const db::Net *, const db::Net *>
NetlistBrowserModel::nets_from_id (void *id) const
{
  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  size_t index = circuit_net_index_from_id (id);

  return mp_indexer->net_from_index (circuits, index).first;
}

std::pair<const db::NetSubcircuitPinRef *, const db::NetSubcircuitPinRef *>
NetlistBrowserModel::net_subcircuit_pinrefs_from_id (void *id) const
{
  IndexedNetlistModel::net_pair nets = nets_from_id (id);
  size_t index = circuit_net_subcircuit_pin_index_from_id (id);

  return mp_indexer->net_subcircuit_pinref_from_index (nets, index);
}

std::pair<const db::NetPinRef *, const db::NetPinRef *>
NetlistBrowserModel::net_pinrefs_from_id (void *id) const
{
  IndexedNetlistModel::net_pair nets = nets_from_id (id);
  size_t index = circuit_net_pin_index_from_id (id);

  return mp_indexer->net_pinref_from_index (nets, index);
}

std::pair<const db::NetTerminalRef *, const db::NetTerminalRef *>
NetlistBrowserModel::net_terminalrefs_from_id (void *id) const
{
  IndexedNetlistModel::net_pair nets = nets_from_id (id);
  size_t index = circuit_net_device_terminal_index_from_id (id);

  return mp_indexer->net_terminalref_from_index (nets, index);
}

std::pair<const db::Device *, const db::Device *>
NetlistBrowserModel::devices_from_id (void *id) const
{
  IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
  size_t index = circuit_device_index_from_id (id);

  return mp_indexer->device_from_index (circuits, index).first;
}

std::pair<const db::Pin *, const db::Pin *>
NetlistBrowserModel::pins_from_id (void *id) const
{
  if (is_id_circuit_subcircuit_pin (id)) {

    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_id (id);
    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (subcircuits);

    size_t index = circuit_subcircuit_pin_index_from_id (id);

    return mp_indexer->pin_from_index (circuit_refs, index).first;

  } else {

    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);

    size_t index = circuit_pin_index_from_id (id);

    return mp_indexer->pin_from_index (circuits, index).first;

  }
}

std::pair<const db::SubCircuit *, const db::SubCircuit *>
NetlistBrowserModel::subcircuits_from_id (void *id) const
{
  if (is_id_circuit_subcircuit_pin (id) || is_id_circuit_subcircuit (id)) {

    IndexedNetlistModel::circuit_pair circuits = circuits_from_id (id);
    size_t index = circuit_subcircuit_index_from_id (id);

    return mp_indexer->subcircuit_from_index (circuits, index).first;

  } else {

    IndexedNetlistModel::net_subcircuit_pin_pair pinrefs = net_subcircuit_pinrefs_from_id (id);
    return subcircuits_from_pinrefs (pinrefs);

  }
}

void
NetlistBrowserModel::show_or_hide_items (QTreeView *view, const QModelIndex &parent, bool show_all, bool with_warnings, bool with_children)
{
  int n = rowCount (parent);
  for (int i = 0; i < n; ++i) {

    QModelIndex idx = index (i, 0, parent);

    IndexedNetlistModel::Status st = status (idx);
    bool visible = (show_all || (st != db::NetlistCrossReference::Match && (with_warnings || st != db::NetlistCrossReference::MatchWithWarning)));
    view->setRowHidden (int (i), parent, ! visible);

    if (visible && with_children) {
      show_or_hide_items (view, idx, show_all, with_warnings, false /*just two levels of recursion*/);
    }

  }
}

void
NetlistBrowserModel::set_item_visibility (QTreeView *view, bool show_all, bool with_warnings)
{
  //  TODO: this implementation is based on the model but is fairly inefficient
  show_or_hide_items (view, QModelIndex (), show_all, with_warnings, true);
}

}

