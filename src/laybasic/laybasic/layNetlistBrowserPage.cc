
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


#include "layNetlistBrowserPage.h"
#include "dbLayoutToNetlist.h"

namespace lay
{

extern std::string cfg_l2ndb_show_all;

// ----------------------------------------------------------------------------------
//  NetlistBrowserModel implementation

static inline void *make_id (size_t i1)
{
  return reinterpret_cast<void *> (i1);
}

static inline void *make_id (size_t i1, size_t n1, size_t i2)
{
  return reinterpret_cast<void *> (i1 + n1 * i2);
}

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

NetlistBrowserModel::NetlistBrowserModel (QWidget *parent, db::LayoutToNetlist *l2ndb)
  : QAbstractItemModel (parent), mp_l2ndb (l2ndb)
{
  //  .. nothing yet ..
}

NetlistBrowserModel::~NetlistBrowserModel ()
{
  //  .. nothing yet ..
}

void *
NetlistBrowserModel::make_id_circuit (size_t circuit_index) const
{
  return make_id (circuit_index);
}

void *
NetlistBrowserModel::make_id_circuit_pin (size_t circuit_index, size_t pin_index) const
{
  return make_id (circuit_index, netlist ()->circuit_count (), 1, 8, pin_index);
}

void *
NetlistBrowserModel::make_id_circuit_pin_net (size_t circuit_index, size_t pin_index, size_t net_index) const
{
  db::Circuit *circuit = circuit_from_id (make_id_circuit (circuit_index));
  return make_id (circuit_index, netlist ()->circuit_count (), 1, 8, pin_index, circuit->pin_count (), net_index + 1);
}

void *
NetlistBrowserModel::make_id_circuit_net (size_t circuit_index, size_t net_index) const
{
  return make_id (circuit_index, netlist ()->circuit_count (), 2, 8, net_index);
}

void *
NetlistBrowserModel::make_id_circuit_net_device_terminal (size_t circuit_index, size_t net_index, size_t terminal_ref_index) const
{
  db::Circuit *circuit = circuit_from_id (make_id_circuit (circuit_index));
  return make_id (circuit_index, netlist ()->circuit_count (), 2, 8, net_index, circuit->net_count (), 1, 4, terminal_ref_index);
}

void *
NetlistBrowserModel::make_id_circuit_net_device_terminal_others (size_t circuit_index, size_t net_index, size_t terminal_ref_index, size_t other_index) const
{
  db::Circuit *circuit = circuit_from_id (make_id_circuit (circuit_index));
  db::Net *net = net_from_id (make_id_circuit_net (circuit_index, net_index));
  return make_id (circuit_index, netlist ()->circuit_count (), 2, 8, net_index, circuit->net_count (), 1, 4, terminal_ref_index, net->terminal_count (), other_index + 1);
}

void *
NetlistBrowserModel::make_id_circuit_net_pin (size_t circuit_index, size_t net_index, size_t pin_index) const
{
  db::Circuit *circuit = circuit_from_id (make_id_circuit (circuit_index));
  return make_id (circuit_index, netlist ()->circuit_count (), 2, 8, net_index, circuit->net_count (), 2, 4, pin_index);
}

void *
NetlistBrowserModel::make_id_circuit_net_subcircuit_pin (size_t circuit_index, size_t net_index, size_t pin_ref_index) const
{
  db::Circuit *circuit = circuit_from_id (make_id_circuit (circuit_index));
  return make_id (circuit_index, netlist ()->circuit_count (), 2, 8, net_index, circuit->net_count (), 3, 4, pin_ref_index);
}

void *
NetlistBrowserModel::make_id_circuit_net_subcircuit_pin_others (size_t circuit_index, size_t net_index, size_t pin_ref_index, size_t other_index) const
{
  db::Circuit *circuit = circuit_from_id (make_id_circuit (circuit_index));
  db::Net *net = net_from_id (make_id_circuit_net (circuit_index, net_index));
  return make_id (circuit_index, netlist ()->circuit_count (), 2, 8, net_index, circuit->net_count (), 3, 4, pin_ref_index, net->subcircuit_pin_count (), other_index + 1);
}

void *
NetlistBrowserModel::make_id_circuit_subcircuit (size_t circuit_index, size_t subcircuit_index) const
{
  return make_id (circuit_index, netlist ()->circuit_count (), 3, 8, subcircuit_index);
}

void *
NetlistBrowserModel::make_id_circuit_subcircuit_pin (size_t circuit_index, size_t subcircuit_index, size_t pin_index) const
{
  db::Circuit *circuit = circuit_from_id (make_id_circuit (circuit_index));
  return make_id (circuit_index, netlist ()->circuit_count (), 3, 8, subcircuit_index, circuit->subcircuit_count (), pin_index + 1);
}

void *
NetlistBrowserModel::make_id_circuit_device (size_t circuit_index, size_t device_index) const
{
  return make_id (circuit_index, netlist ()->circuit_count (), 4, 8, device_index);
}

void *
NetlistBrowserModel::make_id_circuit_device_terminal (size_t circuit_index, size_t device_index, size_t terminal_index) const
{
  db::Circuit *circuit = circuit_from_id (make_id_circuit (circuit_index));
  return make_id (circuit_index, netlist ()->circuit_count (), 4, 8, device_index, circuit->device_count (), terminal_index + 1);
}

bool
NetlistBrowserModel::is_id_circuit (void *id) const
{
  pop (id, netlist ()->circuit_count ());
  return id == 0;
}

bool
NetlistBrowserModel::is_id_circuit_pin (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 1 && always (pop (id, circuit->pin_count ())) && id == 0);
}

bool
NetlistBrowserModel::is_id_circuit_pin_net (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 1 && always (pop (id, circuit->pin_count ())) && id != 0);
}

bool
NetlistBrowserModel::is_id_circuit_net (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 2 && always (pop (id, circuit->net_count ())) && id == 0);
}

bool
NetlistBrowserModel::is_id_circuit_net_device_terminal (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  db::Net *net = net_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 2 && always (pop (id, circuit->net_count ())) && pop (id, 4) == 1 && always (pop (id, net->terminal_count ())) && id == 0);
}

bool
NetlistBrowserModel::is_id_circuit_net_device_terminal_others (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  db::Net *net = net_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 2 && always (pop (id, circuit->net_count ())) && pop (id, 4) == 1 && always (pop (id, net->terminal_count ())) && id != 0);
}

bool
NetlistBrowserModel::is_id_circuit_net_pin (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 2 && always (pop (id, circuit->net_count ())) && pop (id, 4) == 2);
}

bool
NetlistBrowserModel::is_id_circuit_net_subcircuit_pin (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  db::Net *net = net_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 2 && always (pop (id, circuit->net_count ())) && pop (id, 4) == 3 && always (pop (id, net->subcircuit_pin_count ())) && id == 0);
}

bool
NetlistBrowserModel::is_id_circuit_net_subcircuit_pin_others (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  db::Net *net = net_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 2 && always (pop (id, circuit->net_count ())) && pop (id, 4) == 3 && always (pop (id, net->subcircuit_pin_count ())) && id != 0);
}

bool
NetlistBrowserModel::is_id_circuit_subcircuit (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 3 && always (pop (id, circuit->subcircuit_count ())) && id == 0);
}

bool
NetlistBrowserModel::is_id_circuit_subcircuit_pin (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 3 && always (pop (id, circuit->subcircuit_count ())) && id != 0);
}

bool
NetlistBrowserModel::is_id_circuit_device (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 4 && always (pop (id, circuit->device_count ())) && id == 0);
}

bool
NetlistBrowserModel::is_id_circuit_device_terminal (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 4 && always (pop (id, circuit->device_count ())) && id != 0);
}

size_t
NetlistBrowserModel::circuit_index_from_id (void *id) const
{
  return pop (id, netlist ()->circuit_count ());
}

size_t
NetlistBrowserModel::circuit_pin_index_from_id (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  return pop (id, circuit->pin_count ());
}

size_t
NetlistBrowserModel::circuit_device_index_from_id (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  return pop (id, circuit->device_count ());
}

size_t
NetlistBrowserModel::circuit_device_terminal_index_from_id (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  pop (id, circuit->device_count ());
  return reinterpret_cast<size_t> (id) - 1;
}

size_t
NetlistBrowserModel::circuit_subcircuit_index_from_id (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  return pop (id, circuit->subcircuit_count ());
}

size_t
NetlistBrowserModel::circuit_subcircuit_pin_index_from_id (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  pop (id, circuit->subcircuit_count ());
  return reinterpret_cast<size_t> (id) - 1;
}

size_t
NetlistBrowserModel::circuit_net_index_from_id (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  return pop (id, circuit->net_count ());
}

size_t
NetlistBrowserModel::circuit_net_pin_index_from_id (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  pop (id, circuit->net_count ());
  pop (id, 4);
  return reinterpret_cast<size_t> (id);
}

size_t
NetlistBrowserModel::circuit_net_subcircuit_pin_index_from_id (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  db::Net *net = net_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  pop (id, circuit->net_count ());
  pop (id, 4);
  return pop (id, net->subcircuit_pin_count ());
}

size_t
NetlistBrowserModel::circuit_net_subcircuit_pin_other_index_from_id (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  db::Net *net = net_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  pop (id, circuit->net_count ());
  pop (id, 4);
  pop (id, net->subcircuit_pin_count ());
  return reinterpret_cast<size_t> (id) - 1;
}

size_t
NetlistBrowserModel::circuit_net_device_terminal_index_from_id (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  db::Net *net = net_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  pop (id, circuit->net_count ());
  pop (id, 4);
  return pop (id, net->terminal_count ());
}

size_t
NetlistBrowserModel::circuit_net_device_terminal_other_index_from_id (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  db::Net *net = net_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  pop (id, circuit->net_count ());
  pop (id, 4);
  pop (id, net->terminal_count ());
  return reinterpret_cast<size_t> (id) - 1;
}

void
NetlistBrowserModel::self_test (const QModelIndex &p)
{
  int rows = rowCount (p);
  for (int r = 0; r != rows; ++r) {

    QModelIndex c, pp;

    c = index (r, 0, p);
    if (c.column () != 0) {
      tl_assert (false);
    }
    pp = parent (c);
    if (pp.isValid () && pp.column () != 0) {
      tl_assert (false);
    }
    if (pp.isValid () && pp.row () != r) {
      tl_assert (false);
    }
    if (pp.isValid () && pp.internalId () != p.internalId ()) {
      tl_assert (false);
    }

    c = index (r, 1, p);
    if (c.column () != 1) {
      tl_assert (false);
    }
    pp = parent (c);
    if (pp.isValid () && pp.column () != 1) {
      tl_assert (false);
    }

    self_test (c);

  }
}


int
NetlistBrowserModel::columnCount (const QModelIndex & /*parent*/) const
{
  //  Item type & icon, link or description
  return 2;
}

QVariant
NetlistBrowserModel::data (const QModelIndex &index, int role) const
{
  if (role != Qt::DisplayRole || ! index.isValid ()) {
    return QVariant ();
  }

  void *id = index.internalPointer ();

  if (is_id_circuit (id)) {

    db::Circuit *circuit = circuit_from_id (id);
    if (circuit) {
      return tl::to_qstring (circuit->name ());
    }

  } else if (is_id_circuit_pin (id) || is_id_circuit_net_pin (id)) {

    db::Pin *pin = pin_from_id (id);
    if (pin) {
      return tl::to_qstring (pin->expanded_name ());
    }

  } else if (is_id_circuit_pin_net (id)) {

    db::Circuit *circuit = circuit_from_id (id);
    db::Pin *pin = pin_from_id (id);
    if (pin) {
      db::Net *net = circuit->net_for_pin (pin->id ());
      if (net) {
        return tl::to_qstring (net->expanded_name ());
      }
    }

  } else if (is_id_circuit_device (id)) {

    db::Device *device = device_from_id (id);
    if (device) {
      return tl::to_qstring (device->expanded_name ());
    }

  } else if (is_id_circuit_subcircuit (id)) {

    db::SubCircuit *subcircuit = subcircuit_from_id (id);
    if (subcircuit) {
      return tl::to_qstring (subcircuit->expanded_name ());
    }

  } else if (is_id_circuit_net (id)) {

    db::Net *net = net_from_id (id);
    if (net) {
      return tl::to_qstring (net->expanded_name ());
    }

  } else if (is_id_circuit_net_subcircuit_pin (id)) {

    const db::NetSubcircuitPinRef *ref = net_pinref_from_id (id);
    if (ref && ref->pin ()) {
      return tl::to_qstring (ref->pin ()->expanded_name ());
    }

  } else if (is_id_circuit_net_subcircuit_pin_others (id)) {

    const db::NetSubcircuitPinRef *ref = net_pinref_from_id (id);
    size_t other_index = circuit_net_device_terminal_other_index_from_id (id);

    if (ref && ref->pin () && ref->subcircuit () && ref->subcircuit ()->circuit_ref () && ref->subcircuit ()->circuit_ref ()->pin_by_id (other_index)) {

      const db::Pin *pin = ref->subcircuit ()->circuit_ref ()->pin_by_id (other_index);
      return tl::to_qstring (pin->expanded_name ());
    }

  } else if (is_id_circuit_net_device_terminal (id)) {

    const db::NetTerminalRef *ref = net_terminalref_from_id (id);
    if (ref && ref->terminal_def ()) {
      return tl::to_qstring (ref->terminal_def ()->name ());
    }

  } else if (is_id_circuit_net_device_terminal_others (id)) {

    const db::NetTerminalRef *ref = net_terminalref_from_id (id);
    size_t other_index = circuit_net_device_terminal_other_index_from_id (id);

    if (ref && ref->device_class () && ref->device_class ()->terminal_definitions ().size () > other_index) {
      const db::DeviceTerminalDefinition &def = ref->device_class ()->terminal_definitions ()[other_index];
      return tl::to_qstring (def.name ());
    }

  }

  return QVariant ();
}

Qt::ItemFlags
NetlistBrowserModel::flags (const QModelIndex & /*index*/) const
{
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool
NetlistBrowserModel::hasChildren (const QModelIndex &parent) const
{
  if (! parent.isValid ()) {

    return mp_l2ndb->netlist () && mp_l2ndb->netlist ()->circuit_count () > 0;

  } else {

    void *id = parent.internalPointer ();

    if (is_id_circuit (id)) {
      db::Circuit *circuit = circuit_from_id (id);
      return circuit->device_count () + circuit->pin_count () + circuit->net_count () + circuit->subcircuit_count () > 0;
    } else if (is_id_circuit_pin (id)) {
      return true;
    } else if (is_id_circuit_device (id)) {
      db::Device *device = device_from_id (id);
      return device->device_class () && ! device->device_class ()->terminal_definitions ().empty ();
    } else if (is_id_circuit_subcircuit (id)) {
      db::SubCircuit *subcircuit = subcircuit_from_id (id);
      return subcircuit->circuit_ref () && subcircuit->circuit_ref ()->pin_count () > 0;
    } else if (is_id_circuit_net (id)) {
      db::Net *net = net_from_id (id);
      return net->pin_count () + net->subcircuit_pin_count () + net->terminal_count () > 0;
    } else if (is_id_circuit_net_subcircuit_pin (id)) {
      const db::NetSubcircuitPinRef *ref = net_pinref_from_id (id);
      return ref->subcircuit ()->circuit_ref () && ref->subcircuit ()->circuit_ref ()->pin_count () > 0;
    } else if (is_id_circuit_net_device_terminal (id)) {
      const db::NetTerminalRef *ref = net_terminalref_from_id (id);
      return ref->device_class () && ! ref->device_class ()->terminal_definitions ().empty ();
    } else {
      return false;
    }

  }
}

QVariant
NetlistBrowserModel::headerData (int /*section*/, Qt::Orientation /*orientation*/, int /*role*/) const
{
  return QVariant ();
}

QModelIndex
NetlistBrowserModel::index (int row, int column, const QModelIndex &parent) const
{
  void *new_id = 0;

  if (! parent.isValid ()) {

    new_id = make_id_circuit (row);

  } else {

    void *id = parent.internalPointer ();

    if (is_id_circuit (id)) {

      db::Circuit *circuit = circuit_from_id (id);
      if (row < int (circuit->pin_count ())) {
        new_id = make_id_circuit_pin (circuit_index_from_id (id), row);
      } else {
        row -= int (circuit->pin_count ());
        if (row < int (circuit->net_count ())) {
          new_id = make_id_circuit_net (circuit_index_from_id (id), row);
        } else {
          row -= int (circuit->net_count ());
          if (row < int (circuit->subcircuit_count ())) {
            new_id = make_id_circuit_subcircuit (circuit_index_from_id (id), row);
          } else {
            row -= int (circuit->subcircuit_count ());
            if (row < int (circuit->device_count ())) {
              new_id = make_id_circuit_device (circuit_index_from_id (id), row);
            }
          }
        }
      }

    } else if (is_id_circuit_pin (id)) {

      new_id = make_id_circuit_pin_net (circuit_index_from_id (id), circuit_pin_index_from_id (id), row);

    } else if (is_id_circuit_device (id)) {

      new_id = make_id_circuit_device_terminal (circuit_index_from_id (id), circuit_device_index_from_id (id), row);

    } else if (is_id_circuit_subcircuit (id)) {

      new_id = make_id_circuit_subcircuit_pin (circuit_index_from_id (id), circuit_subcircuit_index_from_id (id), row);

    } else if (is_id_circuit_net (id)) {

      db::Net *net = net_from_id (id);
      if (row < int (net->terminal_count ())) {
        new_id = make_id_circuit_net_device_terminal (circuit_index_from_id (id), circuit_net_index_from_id (id), row);
      } else {
        row -= int (net->terminal_count ());
        if (row < int (net->pin_count ())) {
          new_id = make_id_circuit_net_pin (circuit_index_from_id (id), circuit_net_index_from_id (id), row);
        } else {
          row -= int (net->pin_count ());
          if (row < int (net->subcircuit_pin_count ())) {
            new_id = make_id_circuit_net_subcircuit_pin (circuit_index_from_id (id), circuit_net_index_from_id (id), row);
          }
        }
      }

    } else if (is_id_circuit_net_subcircuit_pin (id)) {

      new_id = make_id_circuit_net_subcircuit_pin_others (circuit_index_from_id (id), circuit_net_index_from_id (id), circuit_net_subcircuit_pin_index_from_id (id), row);

    } else if (is_id_circuit_net_device_terminal (id)) {

      new_id = make_id_circuit_net_device_terminal_others (circuit_index_from_id (id), circuit_net_index_from_id (id), circuit_net_device_terminal_index_from_id (id), row);

    }

  }

  return createIndex (row, column, new_id);
}

QModelIndex
NetlistBrowserModel::parent (const QModelIndex &index) const
{
  if (! index.isValid ()) {

    return QModelIndex ();

  } else {

    void *id = index.internalPointer ();
    int column = index.column ();

    if (is_id_circuit (id)) {

      return QModelIndex ();

    } else if (is_id_circuit_pin (id) || is_id_circuit_net (id) || is_id_circuit_device (id) || is_id_circuit_subcircuit (id)) {

      return createIndex (int (circuit_index_from_id (id)), column, make_id_circuit (circuit_index_from_id (id)));

    } else if (is_id_circuit_pin_net (id)) {

      return createIndex (int (circuit_pin_index_from_id (id)), column, make_id_circuit_pin (circuit_index_from_id (id), circuit_pin_index_from_id (id)));

    } else if (is_id_circuit_net_device_terminal (id) || is_id_circuit_net_pin (id) || is_id_circuit_net_subcircuit_pin (id)) {

      db::Circuit *circuit = circuit_from_id (id);
      return createIndex (int (circuit->pin_count () + circuit_net_index_from_id (id)), column, make_id_circuit_net (circuit_index_from_id (id), circuit_net_index_from_id (id)));

    } else if (is_id_circuit_subcircuit_pin (id)) {

      db::Circuit *circuit = circuit_from_id (id);
      return createIndex (int (circuit->pin_count () + circuit->net_count () + circuit_subcircuit_index_from_id (id)), column, make_id_circuit_subcircuit (circuit_index_from_id (id), circuit_subcircuit_index_from_id (id)));

    } else if (is_id_circuit_device_terminal (id)) {

      db::Circuit *circuit = circuit_from_id (id);
      return createIndex (int (circuit->pin_count () + circuit->net_count () + circuit->subcircuit_count () + circuit_device_terminal_index_from_id (id)), column, make_id_circuit_device (circuit_index_from_id (id), circuit_device_index_from_id (id)));

    } else if (is_id_circuit_net_device_terminal_others (id)) {

      return createIndex (circuit_net_device_terminal_index_from_id (id), column, make_id_circuit_net_device_terminal (circuit_index_from_id (id), circuit_net_index_from_id (id), circuit_net_device_terminal_index_from_id (id)));

    } else if (is_id_circuit_net_subcircuit_pin_others (id)) {

      db::Net *net = net_from_id (id);
      return createIndex (size_t (net->terminal_count () + net->pin_count () + circuit_net_subcircuit_pin_index_from_id (id)), column, make_id_circuit_net_subcircuit_pin (circuit_index_from_id (id), circuit_net_index_from_id (id), circuit_net_subcircuit_pin_index_from_id (id)));

    }

  }

  return QModelIndex ();
}

int
NetlistBrowserModel::rowCount (const QModelIndex &parent) const
{
  if (! parent.isValid ()) {

    return int (mp_l2ndb->netlist ()->circuit_count ());

  } else {

    void *id = parent.internalPointer ();

    if (is_id_circuit (id)) {

      db::Circuit *circuit = circuit_from_id (id);
      return int (circuit->pin_count () + circuit->net_count () + circuit->subcircuit_count () + circuit->device_count ());

    } else if (is_id_circuit_pin (id)) {

      return 1;

    } else if (is_id_circuit_device (id) || is_id_circuit_net_device_terminal (id)) {

      db::Device *device = device_from_id (id);
      return int (device->device_class () ? device->device_class ()->terminal_definitions ().size () : 0);

    } else if (is_id_circuit_subcircuit (id) || is_id_circuit_net_subcircuit_pin (id)) {

      db::SubCircuit *subcircuit = subcircuit_from_id (id);
      return int (subcircuit->circuit_ref () ? subcircuit->circuit_ref ()->pin_count () : 0);

    } else if (is_id_circuit_net (id)) {

      db::Net *net = net_from_id (id);
      return int (net->terminal_count () + net->pin_count () + net->subcircuit_pin_count ());

    }

  }

  return 0;
}

db::Circuit *
NetlistBrowserModel::circuit_from_id (void *id) const
{
  size_t index = circuit_index_from_id (id);

  std::map<size_t, db::Circuit *>::iterator c = m_circuit_by_index.find (index);
  if (c == m_circuit_by_index.end ()) {

    c = m_circuit_by_index.insert (std::make_pair (index, (db::Circuit *) 0)).first;
    for (db::Netlist::circuit_iterator i = netlist ()->begin_circuits (); i != netlist ()->end_circuits (); ++i) {
      if (index-- == 0) {
        c->second = i.operator-> ();
        break;
      }
    }

  }

  return c->second;
}

db::Net *
NetlistBrowserModel::net_from_id (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  size_t index = circuit_net_index_from_id (id);

  std::map<db::Circuit *, std::map<size_t, db::Net *> >::iterator cc = m_net_by_circuit_and_index.find (circuit);
  if (cc == m_net_by_circuit_and_index.end ()) {
    cc = m_net_by_circuit_and_index.insert (std::make_pair (circuit, std::map<size_t, db::Net *> ())).first;
  }

  std::map<size_t, db::Net *>::iterator c = cc->second.find (index);
  if (c == cc->second.end ()) {

    c = cc->second.insert (std::make_pair (index, (db::Net *) 0)).first;
    for (db::Circuit::net_iterator i = circuit->begin_nets (); i != circuit->end_nets (); ++i) {
      if (index-- == 0) {
        c->second = i.operator-> ();
        break;
      }
    }

  }

  return c->second;
}

const db::NetSubcircuitPinRef *
NetlistBrowserModel::net_pinref_from_id (void *id) const
{
  db::Net *net = net_from_id (id);
  size_t index = circuit_net_subcircuit_pin_index_from_id (id);

  std::map<db::Net *, std::map<size_t, db::NetSubcircuitPinRef *> >::iterator cc = m_pinref_by_net_and_index.find (net);
  if (cc == m_pinref_by_net_and_index.end ()) {
    cc = m_pinref_by_net_and_index.insert (std::make_pair (net, std::map<size_t, db::NetSubcircuitPinRef *> ())).first;
  }

  std::map<size_t, db::NetSubcircuitPinRef *>::iterator c = cc->second.find (index);
  if (c == cc->second.end ()) {

    c = cc->second.insert (std::make_pair (index, (db::NetSubcircuitPinRef *) 0)).first;
    for (db::Net::subcircuit_pin_iterator i = net->begin_subcircuit_pins (); i != net->end_subcircuit_pins (); ++i) {
      if (index-- == 0) {
        c->second = i.operator-> ();
        break;
      }
    }

  }

  return c->second;
}

const db::NetTerminalRef *
NetlistBrowserModel::net_terminalref_from_id (void *id) const
{
  db::Net *net = net_from_id (id);
  size_t index = circuit_net_device_terminal_index_from_id (id);

  std::map<db::Net *, std::map<size_t, db::NetTerminalRef *> >::iterator cc = m_terminalref_by_net_and_index.find (net);
  if (cc == m_terminalref_by_net_and_index.end ()) {
    cc = m_terminalref_by_net_and_index.insert (std::make_pair (net, std::map<size_t, db::NetTerminalRef *> ())).first;
  }

  std::map<size_t, db::NetTerminalRef *>::iterator c = cc->second.find (index);
  if (c == cc->second.end ()) {

    c = cc->second.insert (std::make_pair (index, (db::NetTerminalRef *) 0)).first;
    for (db::Net::terminal_iterator i = net->begin_terminals (); i != net->end_terminals (); ++i) {
      if (index-- == 0) {
        c->second = i.operator-> ();
        break;
      }
    }

  }

  return c->second;
}

db::Device *
NetlistBrowserModel::device_from_id (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  size_t index = circuit_device_index_from_id (id);

  std::map<db::Circuit *, std::map<size_t, db::Device *> >::iterator cc = m_device_by_circuit_and_index.find (circuit);
  if (cc == m_device_by_circuit_and_index.end ()) {
    cc = m_device_by_circuit_and_index.insert (std::make_pair (circuit, std::map<size_t, db::Device *> ())).first;
  }

  std::map<size_t, db::Device *>::iterator c = cc->second.find (index);
  if (c == cc->second.end ()) {

    c = cc->second.insert (std::make_pair (index, (db::Device *) 0)).first;
    for (db::Circuit::device_iterator i = circuit->begin_devices (); i != circuit->end_devices (); ++i) {
      if (index-- == 0) {
        c->second = i.operator-> ();
        break;
      }
    }

  }

  return c->second;
}

db::Pin *
NetlistBrowserModel::pin_from_id (void *id) const
{
  if (is_id_circuit_net_pin (id)) {

    db::Net *net = net_from_id (id);
    size_t index = circuit_net_pin_index_from_id (id);

    std::map<db::Net *, std::map<size_t, db::Pin *> >::iterator cc = m_pin_by_net_and_index.find (net);
    if (cc == m_pin_by_net_and_index.end ()) {
      cc = m_pin_by_net_and_index.insert (std::make_pair (net, std::map<size_t, db::Pin *> ())).first;
    }

    std::map<size_t, db::Pin *>::iterator c = cc->second.find (index);
    if (c == cc->second.end ()) {

      c = cc->second.insert (std::make_pair (index, (db::Pin *) 0)).first;
      for (db::Net::pin_iterator i = net->begin_pins (); i != net->end_pins (); ++i) {
        if (index-- == 0) {
          c->second = const_cast<db::Pin *> (i->pin ());
          break;
        }
      }

    }

    return c->second;

  } else {

    db::Circuit *circuit = circuit_from_id (id);
    size_t index = circuit_pin_index_from_id (id);

    std::map<db::Circuit *, std::map<size_t, db::Pin *> >::iterator cc = m_pin_by_circuit_and_index.find (circuit);
    if (cc == m_pin_by_circuit_and_index.end ()) {
      cc = m_pin_by_circuit_and_index.insert (std::make_pair (circuit, std::map<size_t, db::Pin *> ())).first;
    }

    std::map<size_t, db::Pin *>::iterator c = cc->second.find (index);
    if (c == cc->second.end ()) {

      c = cc->second.insert (std::make_pair (index, (db::Pin *) 0)).first;
      for (db::Circuit::pin_iterator i = circuit->begin_pins (); i != circuit->end_pins (); ++i) {
        if (index-- == 0) {
          c->second = i.operator-> ();
          break;
        }
      }

    }

    return c->second;

  }
}

db::SubCircuit *
NetlistBrowserModel::subcircuit_from_id (void *id) const
{
  db::Circuit *circuit = circuit_from_id (id);
  size_t index = circuit_subcircuit_index_from_id (id);

  std::map<db::Circuit *, std::map<size_t, db::SubCircuit *> >::iterator cc = m_subcircuit_by_circuit_and_index.find (circuit);
  if (cc == m_subcircuit_by_circuit_and_index.end ()) {
    cc = m_subcircuit_by_circuit_and_index.insert (std::make_pair (circuit, std::map<size_t, db::SubCircuit *> ())).first;
  }

  std::map<size_t, db::SubCircuit *>::iterator c = cc->second.find (index);
  if (c == cc->second.end ()) {

    c = cc->second.insert (std::make_pair (index, (db::SubCircuit *) 0)).first;
    for (db::Circuit::subcircuit_iterator i = circuit->begin_subcircuits (); i != circuit->end_subcircuits (); ++i) {
      if (index-- == 0) {
        c->second = i.operator-> ();
        break;
      }
    }

  }

  return c->second;
}

// ----------------------------------------------------------------------------------
//  NetlistBrowserPage implementation

NetlistBrowserPage::NetlistBrowserPage (QWidget * /*parent*/)
  : m_show_all (true),
    m_context (lay::NetlistBrowserConfig::NetlistTop),
    m_window (lay::NetlistBrowserConfig::FitNet),
    m_window_dim (0.0),
    m_max_shape_count (1000),
    m_marker_line_width (-1),
    m_marker_vertex_size (-1),
    m_marker_halo (-1),
    m_marker_dither_pattern (-1),
    mp_view (0),
    m_cv_index (0),
    mp_plugin_root (0)
{
  Ui::NetlistBrowserPage::setupUi (this);

  m_show_all_action = new QAction (QObject::tr ("Show All"), this);
  m_show_all_action->setCheckable (true);
  m_show_all_action->setChecked (m_show_all);

  connect (m_show_all_action, SIGNAL (triggered ()), this, SLOT (show_all_clicked ()));
  connect (filter, SIGNAL (textEdited (const QString &)), this, SLOT (filter_changed ()));
}

NetlistBrowserPage::~NetlistBrowserPage ()
{
  // @@@
}

void
NetlistBrowserPage::set_plugin_root (lay::PluginRoot *pr)
{
  mp_plugin_root = pr;
}

void
NetlistBrowserPage::set_highlight_style (QColor color, int line_width, int vertex_size, int halo, int dither_pattern)
{
  m_marker_color = color;
  m_marker_line_width = line_width;
  m_marker_vertex_size = vertex_size;
  m_marker_halo = halo;
  m_marker_dither_pattern = dither_pattern;
  update_highlights ();
}

void
NetlistBrowserPage::set_view (lay::LayoutView *view, unsigned int cv_index)
{
  mp_view = view;
  m_cv_index = cv_index;
  update_highlights ();
}

void
NetlistBrowserPage::set_window (lay::NetlistBrowserConfig::net_window_type window, double window_dim, lay::NetlistBrowserConfig::net_context_mode_type context)
{
  if (window != m_window || window_dim != m_window_dim || context != m_context) {
    m_window = window;
    m_window_dim = window_dim;
    m_context = context;
  }
}

void
NetlistBrowserPage::set_max_shape_count (size_t max_shape_count)
{
  if (m_max_shape_count != max_shape_count) {
    m_max_shape_count = max_shape_count;
#if 0 // @@@
    update_marker_list (1 /*select first*/);
#endif
  }
}

void
NetlistBrowserPage::filter_changed ()
{
#if 0 // @@@
  MarkerBrowserTreeViewModel *tree_model = dynamic_cast<MarkerBrowserTreeViewModel *> (directory_tree->model ());
  if (tree_model) {
    set_hidden_rec (tree_model, directory_tree, QModelIndex (), m_show_all, cat_filter->text (), cell_filter->text ());
  }
  update_highlights (2 /*select all*/);
#endif
}

void
NetlistBrowserPage::show_all_clicked ()
{
  if (mp_plugin_root) {
    mp_plugin_root->config_set (cfg_l2ndb_show_all, tl::to_string (m_show_all_action->isChecked ()));
  }
}

void
NetlistBrowserPage::show_all (bool f)
{
  if (f != m_show_all) {

    m_show_all = f;
    m_show_all_action->setChecked (f);

#if 0 // @@@
    MarkerBrowserTreeViewModel *tree_model = dynamic_cast<MarkerBrowserTreeViewModel *> (directory_tree->model ());
    if (tree_model) {
      set_hidden_rec (tree_model, directory_tree, QModelIndex (), m_show_all, cat_filter->text (), cell_filter->text ());
    }
#endif

  }
}

void
NetlistBrowserPage::update_highlights ()
{
#if 0
  if (! m_enable_updates) {
    m_update_needed = true;
    return;
  }

  if (m_recursion_sentinel) {
    return;
  }

  m_recursion_sentinel = true;
  try {
    do_update_markers ();
  } catch (...) {
    m_recursion_sentinel = false;
    throw;
  }
  m_recursion_sentinel = false;
#endif
}

void
NetlistBrowserPage::set_l2ndb (db::LayoutToNetlist *database)
{
  if (database != mp_database.get ()) {

    mp_database.reset (database);

    //  @@@ release_markers ();

    QAbstractItemModel *tree_model = directory_tree->model ();

    NetlistBrowserModel *new_model = new NetlistBrowserModel (this, database);
#if !defined(NDEBUG)
    try {
      new_model->self_test (); //  might throw an exception!
    } catch (...) {
      delete new_model;
      mp_database.reset (0);
      throw;
    }
#endif

    directory_tree->setModel (new_model);
    // @@@ connect (directory_tree->selectionModel (), SIGNAL (selectionChanged (const QItemSelection &, const QItemSelection &)), this, SLOT (directory_selection_changed (const QItemSelection &, const QItemSelection &)));

    directory_tree->header ()->setSortIndicatorShown (true);

    filter->setText (QString ());

    if (tree_model) {
      delete tree_model;
    }

  }
}

void
NetlistBrowserPage::enable_updates (bool f)
{
#if 0 // @@@
  if (f != m_enable_updates) {

    m_enable_updates = f;

    if (f && m_update_needed) {
      update_markers ();
      update_info_text ();
    }

    m_update_needed = false;

  }
#endif
}

}

