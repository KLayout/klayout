
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
#include "layItemDelegates.h"
#include "layCellView.h"
#include "layLayoutView.h"
#include "layMarker.h"
#include "layNetInfoDialog.h"
#include "layNetExportDialog.h"
#include "tlProgress.h"
#include "dbLayoutToNetlist.h"
#include "dbNetlistDeviceClasses.h"
#include "dbCellMapping.h"
#include "dbLayerMapping.h"

#include <QUrl>
#include <QPainter>
#include <QColorDialog>
#include <QRegExp>

namespace lay
{

extern std::string cfg_l2ndb_show_all;

// ----------------------------------------------------------------------------------
//  Utilities

template <class Obj, class Attr, class Iter>
static const Attr *attr_by_object_and_index (const Obj *obj, size_t index, const Iter &begin, const Iter &end, std::map<const Obj *, std::map<size_t, const Attr *> > &cache)
{
  typename std::map<const Obj *, std::map<size_t, const Attr *> >::iterator cc = cache.find (obj);
  if (cc == cache.end ()) {
    cc = cache.insert (std::make_pair (obj, std::map<size_t, const Attr *> ())).first;
  }

  typename std::map<size_t, const Attr *>::iterator c = cc->second.find (index);
  if (c == cc->second.end ()) {

    c = cc->second.insert (std::make_pair (index, (const Attr *) 0)).first;
    for (Iter i = begin; i != end; ++i) {
      if (index-- == 0) {
        c->second = i.operator-> ();
        break;
      }
    }

  }

  return c->second;
}

template <class Attr, class Iter>
static size_t index_from_attr (const Attr *attr, const Iter &begin, const Iter &end, std::map<const Attr *, size_t> &cache)
{
  typename std::map<const Attr *, size_t>::iterator cc = cache.find (attr);
  if (cc != cache.end ()) {
    return cc->second;
  }

  size_t index = 0;
  for (Iter i = begin; i != end; ++i, ++index) {
    if (i.operator-> () == attr) {
      cache.insert (std::make_pair (i.operator-> (), index));
      return index;
    }
  }

  tl_assert (false);
}

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
  std::map<const db::Net *, QColor>::const_iterator c = m_custom_color.find (net);
  if (c != m_custom_color.end ()) {
    return c->second;
  }

  if (m_auto_colors_enabled) {
    const db::Circuit *circuit = net->circuit ();
    size_t index = index_from_attr (net, circuit->begin_nets (), circuit->end_nets (), m_net_index_by_object);
    return m_auto_colors.color_by_index ((unsigned int) index);
  } else {
    return m_marker_color;
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

NetlistBrowserModel::NetlistBrowserModel (QWidget *parent, db::LayoutToNetlist *l2ndb, NetColorizer *colorizer)
  : QAbstractItemModel (parent), mp_l2ndb (l2ndb), mp_colorizer (colorizer)
{
  connect (mp_colorizer, SIGNAL (colors_changed ()), this, SLOT (colors_changed ()));
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
  const db::Circuit *circuit = circuit_from_id (make_id_circuit (circuit_index));
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
  const db::Circuit *circuit = circuit_from_id (make_id_circuit (circuit_index));
  return make_id (circuit_index, netlist ()->circuit_count (), 2, 8, net_index, circuit->net_count (), 1, 4, terminal_ref_index);
}

void *
NetlistBrowserModel::make_id_circuit_net_device_terminal_others (size_t circuit_index, size_t net_index, size_t terminal_ref_index, size_t other_index) const
{
  const db::Circuit *circuit = circuit_from_id (make_id_circuit (circuit_index));
  const db::Net *net = net_from_id (make_id_circuit_net (circuit_index, net_index));
  return make_id (circuit_index, netlist ()->circuit_count (), 2, 8, net_index, circuit->net_count (), 1, 4, terminal_ref_index, net->terminal_count (), other_index + 1);
}

void *
NetlistBrowserModel::make_id_circuit_net_pin (size_t circuit_index, size_t net_index, size_t pin_index) const
{
  const db::Circuit *circuit = circuit_from_id (make_id_circuit (circuit_index));
  return make_id (circuit_index, netlist ()->circuit_count (), 2, 8, net_index, circuit->net_count (), 2, 4, pin_index);
}

void *
NetlistBrowserModel::make_id_circuit_net_subcircuit_pin (size_t circuit_index, size_t net_index, size_t pin_ref_index) const
{
  const db::Circuit *circuit = circuit_from_id (make_id_circuit (circuit_index));
  return make_id (circuit_index, netlist ()->circuit_count (), 2, 8, net_index, circuit->net_count (), 3, 4, pin_ref_index);
}

void *
NetlistBrowserModel::make_id_circuit_net_subcircuit_pin_others (size_t circuit_index, size_t net_index, size_t pin_ref_index, size_t other_index) const
{
  const db::Circuit *circuit = circuit_from_id (make_id_circuit (circuit_index));
  const db::Net *net = net_from_id (make_id_circuit_net (circuit_index, net_index));
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
  const db::Circuit *circuit = circuit_from_id (make_id_circuit (circuit_index));
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
  const db::Circuit *circuit = circuit_from_id (make_id_circuit (circuit_index));
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
  const db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 1 && always (pop (id, circuit->pin_count ())) && id == 0);
}

bool
NetlistBrowserModel::is_id_circuit_pin_net (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 1 && always (pop (id, circuit->pin_count ())) && id != 0);
}

bool
NetlistBrowserModel::is_id_circuit_net (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 2 && always (pop (id, circuit->net_count ())) && id == 0);
}

bool
NetlistBrowserModel::is_id_circuit_net_device_terminal (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  const db::Net *net = net_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 2 && always (pop (id, circuit->net_count ())) && pop (id, 4) == 1 && always (pop (id, net->terminal_count ())) && id == 0);
}

bool
NetlistBrowserModel::is_id_circuit_net_device_terminal_others (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  const db::Net *net = net_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 2 && always (pop (id, circuit->net_count ())) && pop (id, 4) == 1 && always (pop (id, net->terminal_count ())) && id != 0);
}

bool
NetlistBrowserModel::is_id_circuit_net_pin (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 2 && always (pop (id, circuit->net_count ())) && pop (id, 4) == 2);
}

bool
NetlistBrowserModel::is_id_circuit_net_subcircuit_pin (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  const db::Net *net = net_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 2 && always (pop (id, circuit->net_count ())) && pop (id, 4) == 3 && always (pop (id, net->subcircuit_pin_count ())) && id == 0);
}

bool
NetlistBrowserModel::is_id_circuit_net_subcircuit_pin_others (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  const db::Net *net = net_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 2 && always (pop (id, circuit->net_count ())) && pop (id, 4) == 3 && always (pop (id, net->subcircuit_pin_count ())) && id != 0);
}

bool
NetlistBrowserModel::is_id_circuit_subcircuit (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 3 && always (pop (id, circuit->subcircuit_count ())) && id == 0);
}

bool
NetlistBrowserModel::is_id_circuit_subcircuit_pin (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 3 && always (pop (id, circuit->subcircuit_count ())) && id != 0);
}

bool
NetlistBrowserModel::is_id_circuit_device (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  return (pop (id, 8) == 4 && always (pop (id, circuit->device_count ())) && id == 0);
}

bool
NetlistBrowserModel::is_id_circuit_device_terminal (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
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
  const db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  return pop (id, circuit->pin_count ());
}

size_t
NetlistBrowserModel::circuit_device_index_from_id (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  return pop (id, circuit->device_count ());
}

size_t
NetlistBrowserModel::circuit_device_terminal_index_from_id (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  pop (id, circuit->device_count ());
  return reinterpret_cast<size_t> (id) - 1;
}

size_t
NetlistBrowserModel::circuit_subcircuit_index_from_id (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  return pop (id, circuit->subcircuit_count ());
}

size_t
NetlistBrowserModel::circuit_subcircuit_pin_index_from_id (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  pop (id, circuit->subcircuit_count ());
  return reinterpret_cast<size_t> (id) - 1;
}

size_t
NetlistBrowserModel::circuit_net_index_from_id (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  return pop (id, circuit->net_count ());
}

size_t
NetlistBrowserModel::circuit_net_pin_index_from_id (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  pop (id, circuit->net_count ());
  pop (id, 4);
  return reinterpret_cast<size_t> (id);
}

size_t
NetlistBrowserModel::circuit_net_subcircuit_pin_index_from_id (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  const db::Net *net = net_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  pop (id, circuit->net_count ());
  pop (id, 4);
  return pop (id, net->subcircuit_pin_count ());
}

size_t
NetlistBrowserModel::circuit_net_subcircuit_pin_other_index_from_id (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  const db::Net *net = net_from_id (id);
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
  const db::Circuit *circuit = circuit_from_id (id);
  const db::Net *net = net_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  pop (id, circuit->net_count ());
  pop (id, 4);
  return pop (id, net->terminal_count ());
}

size_t
NetlistBrowserModel::circuit_net_device_terminal_other_index_from_id (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  const db::Net *net = net_from_id (id);
  pop (id, netlist ()->circuit_count ());
  pop (id, 8);
  pop (id, circuit->net_count ());
  pop (id, 4);
  pop (id, net->terminal_count ());
  return reinterpret_cast<size_t> (id) - 1;
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
  if (! index.isValid ()) {
    return QVariant ();
  }

  if (role == Qt::DecorationRole && index.column () == 0) {
    return QVariant (icon (index));
  } else if (role == Qt::DisplayRole) {
    return QVariant (text (index));
  } else if (role == Qt::UserRole) {
    return QVariant (search_text (index));
  } else {
    return QVariant ();
  }
}

QString
NetlistBrowserModel::make_link_to (const db::Net *net) const
{
  if (! net) {
    return QString ();
  } else {
    void *id = make_id_circuit_net (circuit_index (net->circuit ()), net_index (net));
    return tl::to_qstring (tl::sprintf ("<a href='int:net?id=%s'>%s</a>", tl::to_string (reinterpret_cast<size_t> (id)), net->expanded_name ()));
  }
}

QString
NetlistBrowserModel::make_link_to (const db::Pin *pin, const db::Circuit *circuit) const
{
  if (! pin) {
    return QString ();
  } else {
    void *id = make_id_circuit_pin (circuit_index (circuit), pin_index (pin, circuit));
    return tl::to_qstring (tl::sprintf ("<a href='int:pin?id=%s'>%s</a>", tl::to_string (reinterpret_cast<size_t> (id)), pin->expanded_name ()));
  }
}

QString
NetlistBrowserModel::make_link_to (const db::Circuit *circuit) const
{
  if (! circuit) {
    return QString ();
  } else {
    void *id = make_id_circuit (circuit_index (circuit));
    return tl::to_qstring (tl::sprintf ("<a href='int:circuit?id=%s'>%s</a>", tl::to_string (reinterpret_cast<size_t> (id)), circuit->name ()));
  }
}

QString
NetlistBrowserModel::make_link_to (const db::SubCircuit *sub_circuit) const
{
  if (! sub_circuit) {
    return QString ();
  } else {
    void *id = make_id_circuit_subcircuit (circuit_index (sub_circuit->circuit ()), subcircuit_index (sub_circuit));
    return tl::to_qstring (tl::sprintf ("<a href='int:subcircuit?id=%s'>%s</a>", tl::to_string (reinterpret_cast<size_t> (id)), sub_circuit->expanded_name ()));
  }
}

static
std::string device_string (const db::Device *device)
{
  if (! device || ! device->device_class ()) {
    return std::string ();
  }

  std::string s = device->device_class ()->name ();
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
      double v = device->parameter_value (p->id ());
      s += tl::to_string (v);
    }
  }
  s += "]";
  return s;
}

QString
NetlistBrowserModel::text (const QModelIndex &index) const
{
  void *id = index.internalPointer ();

  if (is_id_circuit (id)) {

    //  circuit: header column = name, other columns empty
    if (index.column () == 0) {
      const db::Circuit *circuit = circuit_from_id (id);
      if (circuit) {
        return tl::to_qstring (circuit->name ());
      }
    }

  } else if (is_id_circuit_pin (id)) {

    //  circuit/pin: header column = pin name, other columns pin name
    const db::Pin *pin = pin_from_id (id);
    if (pin) {
      return tl::to_qstring (pin->expanded_name ());
    }
    //  TODO: in case of compare, column 1 is name(a):name(b), 2 is name(a) and 3 is name(b)

  } else if (is_id_circuit_pin_net (id)) {

    //  circuit/pin/net: header column = name, second column link to net
    const db::Circuit *circuit = circuit_from_id (id);
    const db::Pin *pin = pin_from_id (id);
    if (pin) {
      const db::Net *net = circuit->net_for_pin (pin->id ());
      if (net) {
        if (index.column () == 0) {
          return tl::to_qstring (net->expanded_name ());
        } else {
          return make_link_to (net);
        }
      }
    }
    //  TODO: in case of compare, column 1 is name(a):name(b), 2 is link name(a) and 3 is link name(b)

  } else if (is_id_circuit_device (id)) {

    //  circuit/device: header column = class + parameters, second column device name
    const db::Device *device = device_from_id (id);
    if (device && device->device_class ()) {

      if (index.column () == 0) {
        return tl::to_qstring (device_string (device));
      } else {
        return tl::to_qstring (device->expanded_name ());
      }

    }
    //  TODO: in case of compare, column 1 is name(a):name(b) [param], 2 is name(a) and 3 is name(b)

  } else if (is_id_circuit_device_terminal (id)) {

    //  circuit/device/terminal: header column = terminal name, second column link to net
    const db::Device *device = device_from_id (id);
    if (device && device->device_class ()) {
      size_t terminal = circuit_device_terminal_index_from_id (id);
      if (device->device_class ()->terminal_definitions ().size () > terminal) {
        const db::DeviceTerminalDefinition &td = device->device_class ()->terminal_definitions () [terminal];
        if (index.column () == 0) {
          return tl::to_qstring (td.name ());
        } else {
          return make_link_to (device->net_for_terminal (td.id ()));
        }
      }
    }
    //  TODO: in case of compare, column 1 is terminal, 2 is linke net(a) and 3 is link net(b)

  } else if (is_id_circuit_subcircuit (id)) {

    //  circuit/subcircuit: header column = circuit name, second column subcircuit name
    const db::SubCircuit *subcircuit = subcircuit_from_id (id);
    if (subcircuit) {
      if (index.column () == 0) {
        return make_link_to (subcircuit->circuit_ref ());
      } else {
        return tl::to_qstring (subcircuit->expanded_name ());
      }
    }
    //  TODO: in case of compare, column 1 is circuit name(a):circuit name(b), 2 is subcircuit name(a) and 3 is subcircuit name(b)

  } else if (is_id_circuit_subcircuit_pin (id)) {

    //  circuit/pin: header column = pin name, other columns net name
    const db::SubCircuit *subcircuit = subcircuit_from_id (id);
    if (subcircuit && subcircuit->circuit () && subcircuit->circuit_ref ()) {
      const db::Pin *pin = pin_from_id (id);
      if (pin) {
        if (index.column () == 0) {
          return make_link_to (pin, subcircuit->circuit_ref ());
        } else {
          return make_link_to (subcircuit->net_for_pin (pin->id ()));
        }
      }
    }
    //  TODO: in case of compare, column 1 is name(a):name(b), 2 is link net(a) and 3 is link net(b)

  } else if (is_id_circuit_net (id)) {

    //  circuit/net: header column = node count, second column net name
    const db::Net *net = net_from_id (id);
    if (net) {
      if (index.column () == 0) {
        return tl::to_qstring ("(" + tl::to_string (net->pin_count () + net->terminal_count () + net->subcircuit_pin_count ()) + ")");
      } else {
        return tl::to_qstring (net->expanded_name ());
      }
    }
    //  TODO: in case of compare, column 1 is (size), 2 is net name(a) and 3 is net name(b)

  } else if (is_id_circuit_net_pin (id)) {

    //  circuit/net/pin: header column = pin name, second column empty (for now)
    const db::NetPinRef *ref = net_pinref_from_id (id);
    if (ref && ref->pin ()) {
      if (index.column () == 0) {
        return make_link_to (ref->pin (), circuit_from_id (id));
      }
      //  TODO: in case of compare use second and third column
    }

  } else if (is_id_circuit_net_subcircuit_pin (id)) {

    //  circuit/net/subcircuit pin: header column = pin name - circuit name, second column link to subcircuit
    const db::NetSubcircuitPinRef *ref = net_subcircuit_pinref_from_id (id);
    if (ref && ref->pin () && ref->subcircuit ()) {
      const db::Circuit *circuit = ref->subcircuit ()->circuit_ref ();
      if (circuit) {
        if (index.column () == 0) {
          return make_link_to (ref->pin (), circuit) + tl::to_qstring (" - ") + make_link_to (circuit);
        } else {
          return make_link_to (ref->subcircuit ());
        }
      }
    }
    //  TODO: in case of compare, column 1 is pin link(a):pin link(b) - circuit link(a):circuit link(b),
    //  columns 2 is subcircuit link(a), columns (3) is subcircuit link(b)

  } else if (is_id_circuit_net_subcircuit_pin_others (id)) {

    //  circuit/net/device terminal/more: header column = pin name, second column = net link
    const db::NetSubcircuitPinRef *ref = net_subcircuit_pinref_from_id (id);
    size_t other_index = circuit_net_subcircuit_pin_other_index_from_id (id);

    if (ref && ref->pin () && ref->subcircuit ()) {
      const db::Circuit *circuit = ref->subcircuit ()->circuit_ref ();
      if (circuit && circuit->pin_by_id (other_index)) {
        const db::Pin *pin = circuit->pin_by_id (other_index);
        if (index.column () == 0) {
          return make_link_to (pin, circuit);
        } else {
          return make_link_to (ref->subcircuit ()->net_for_pin (pin->id ()));
        }
      }
    }

  } else if (is_id_circuit_net_device_terminal (id)) {

    //  circuit/net/device terminal: header column = terminal and device string, second column = device name
    const db::NetTerminalRef *ref = net_terminalref_from_id (id);
    if (ref && ref->terminal_def ()) {
      if (index.column () == 0) {
        return tl::to_qstring (ref->terminal_def ()->name () + " - " + device_string (ref->device ()));
      } else {
        return tl::to_qstring (ref->device ()->expanded_name ());
      }
    }

  } else if (is_id_circuit_net_device_terminal_others (id)) {

    //  circuit/net/device terminal/more: header column = terminal name, second column = net link
    const db::NetTerminalRef *ref = net_terminalref_from_id (id);
    size_t other_index = circuit_net_device_terminal_other_index_from_id (id);

    if (ref && ref->device_class () && ref->device_class ()->terminal_definitions ().size () > other_index) {
      const db::DeviceTerminalDefinition &def = ref->device_class ()->terminal_definitions ()[other_index];
      if (index.column () == 0) {
        return tl::to_qstring (def.name ());
      } else {
        return make_link_to (ref->device ()->net_for_terminal (def.id ()));
      }
    }

  }

  return QString ();
}

QString
NetlistBrowserModel::search_text (const QModelIndex &index) const
{
  void *id = index.internalPointer ();

  if (is_id_circuit (id)) {

    const db::Circuit *circuit = circuit_from_id (id);
    if (circuit) {
      return tl::to_qstring (circuit->name ());
    }
    //  TODO: in case of compare, use circuit_name(a)|circuit_name(b)

  } else if (is_id_circuit_pin (id)) {

    const db::Pin *pin = pin_from_id (id);
    if (pin) {
      return tl::to_qstring (pin->expanded_name ());
    }
    //  TODO: in case of compare, use pin_name(a)|pin_name(b)

  } else if (is_id_circuit_pin_net (id)) {

    const db::Circuit *circuit = circuit_from_id (id);
    const db::Pin *pin = pin_from_id (id);
    if (pin) {
      const db::Net *net = circuit->net_for_pin (pin->id ());
      if (net) {
        return tl::to_qstring (net->expanded_name ());
      }
    }
    //  TODO: in case of compare, use net_name(a)|net_name(b)

  } else if (is_id_circuit_device (id)) {

    const db::Device *device = device_from_id (id);
    if (device && device->device_class ()) {
      return tl::to_qstring (device->device_class ()->name () + "|" + device->expanded_name ());
    }
    //  TODO: in case of compare, use class(a)|device_name(a)|class(b)|device_name(b)

  } else if (is_id_circuit_device_terminal (id)) {

    const db::Device *device = device_from_id (id);
    if (device && device->device_class ()) {
      size_t terminal = circuit_device_terminal_index_from_id (id);
      if (device->device_class ()->terminal_definitions ().size () > terminal) {
        const db::DeviceTerminalDefinition &td = device->device_class ()->terminal_definitions () [terminal];
        const db::Net *net = device->net_for_terminal (td.id ());
        if (net) {
          return tl::to_qstring (td.name () + "|" + net->expanded_name ());
        }
      }
    }
    //  TODO: in case of compare, use terminal_name(a)|net_name(a)|terminal_name(b)|net_name(b)

  } else if (is_id_circuit_subcircuit (id)) {

    const db::SubCircuit *subcircuit = subcircuit_from_id (id);
    if (subcircuit && subcircuit->circuit_ref ()) {
      return tl::to_qstring (subcircuit->circuit_ref ()->name () + "|" + subcircuit->expanded_name ());
    }
    //  TODO: in case of compare, use circuit_name(a)|subcircuit_name(a)|circuit_name(b)|subcircuit_name(b)

  } else if (is_id_circuit_subcircuit_pin (id)) {

    const db::SubCircuit *subcircuit = subcircuit_from_id (id);
    if (subcircuit && subcircuit->circuit () && subcircuit->circuit_ref ()) {
      const db::Pin *pin = pin_from_id (id);
      if (pin) {
        const db::Net *net = subcircuit->net_for_pin (pin->id ());
        if (net) {
          return tl::to_qstring (pin->name () + "|" + net->expanded_name ());
        }
      }
    }
    //  TODO: in case of compare, use pin_name(a)|net_name(a)|pin_name(b)|net_name(b)

  } else if (is_id_circuit_net (id)) {

    const db::Net *net = net_from_id (id);
    if (net) {
      return tl::to_qstring (net->expanded_name ());
    }
    //  TODO: in case of compare, use name(a)|name(b)

  } else if (is_id_circuit_net_pin (id)) {

    const db::NetPinRef *ref = net_pinref_from_id (id);
    if (ref && ref->pin ()) {
      return tl::to_qstring (ref->pin ()->name ());
    }
    //  TODO: in case of compare, use pin_name(a)|pin_name(b)

  } else if (is_id_circuit_net_subcircuit_pin (id)) {

    const db::NetSubcircuitPinRef *ref = net_subcircuit_pinref_from_id (id);
    if (ref && ref->pin () && ref->subcircuit ()) {
      const db::Circuit *circuit = ref->subcircuit ()->circuit_ref ();
      if (circuit) {
        return tl::to_qstring (ref->pin ()->name () + "|" + circuit->name () + "|" + ref->subcircuit ()->name ());
      }
    }
    //  TODO: in case of compare, use pin_name(a)|circuit_name(a)|subcircuit_name(a)|pin_name(b)|circuit_name(b)|subcircuit_name(b)

  } else if (is_id_circuit_net_subcircuit_pin_others (id)) {

    const db::NetSubcircuitPinRef *ref = net_subcircuit_pinref_from_id (id);
    size_t other_index = circuit_net_subcircuit_pin_other_index_from_id (id);

    if (ref && ref->pin () && ref->subcircuit ()) {
      const db::Circuit *circuit = ref->subcircuit ()->circuit_ref ();
      if (circuit && circuit->pin_by_id (other_index)) {
        const db::Pin *pin = circuit->pin_by_id (other_index);
        const db::Net *net = ref->subcircuit ()->net_for_pin (pin->id ());
        if (net) {
          return tl::to_qstring (pin->name () + "|" + net->expanded_name ());
        }
      }
    }
    //  TODO: in case of compare, use pin_name(a)|net_name(a)|pin_name(b)|net_name(b)

  } else if (is_id_circuit_net_device_terminal (id)) {

    const db::NetTerminalRef *ref = net_terminalref_from_id (id);
    if (ref && ref->terminal_def () && ref->device () && ref->device ()->device_class ()) {
      return tl::to_qstring (ref->terminal_def ()->name () + "|" + ref->device ()->device_class ()->name () + "|" + ref->device ()->expanded_name ());
    }
    //  TODO: in case of compare, use terminal_name(a)|device_class(a)|device_name(a)|terminal_name(b)|device_class(b)|device_name(b)

  } else if (is_id_circuit_net_device_terminal_others (id)) {

    const db::NetTerminalRef *ref = net_terminalref_from_id (id);
    size_t other_index = circuit_net_device_terminal_other_index_from_id (id);

    if (ref && ref->device_class () && ref->device_class ()->terminal_definitions ().size () > other_index) {
      const db::DeviceTerminalDefinition &def = ref->device_class ()->terminal_definitions ()[other_index];
      const db::Net *net = ref->device ()->net_for_terminal (def.id ());
      if (net) {
        return tl::to_qstring (def.name () + "|" + net->expanded_name ());
      }
    }
    //  TODO: in case of compare, use terminal_name(a)|net_name(a)|terminal_name(b)|net_name(b)

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
  //  TODO: diode, inductor, generic device ...
  if (dynamic_cast<const db::DeviceClassResistor *> (dc)) {
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_res_48.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_res_32.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_res_24.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_res_16.png")));
  } else if (dynamic_cast<const db::DeviceClassCapacitor *> (dc)) {
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_cap_48.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_cap_32.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_cap_24.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_cap_16.png")));
  } else {
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_mos_48.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_mos_32.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_mos_24.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_mos_16.png")));
  }
  return icon;
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
NetlistBrowserModel::icon_for_net (const db::Net *net) const
{
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
NetlistBrowserModel::icon_for_connection (const db::Net *net) const
{
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

    const db::Net *net = net_from_index (index);
    return icon_for_net (net);

  } else if (is_id_circuit_device (id)) {

    const db::Device *device = device_from_id (id);
    if (device) {
      return icon_for_device (device->device_class ());
    }

  } else if (is_id_circuit_net_device_terminal_others (id) || is_id_circuit_net_subcircuit_pin_others (id)) {

    const db::Net *net = net_from_index (index);
    return icon_for_connection (net);

  } else if (is_id_circuit_subcircuit (id)) {
    return icon_for_circuit ();
  } else if (is_id_circuit_net_pin (id) || is_id_circuit_net_subcircuit_pin_others (id)) {
    return icon_for_pin ();
  } else if (is_id_circuit_net_subcircuit_pin (id)) {
    return icon_for_circuit ();
  } else if (is_id_circuit_net_device_terminal (id)) {

    const db::NetTerminalRef *ref = net_terminalref_from_id (id);
    if (ref && ref->device ()) {
      return icon_for_device (ref->device ()->device_class ());
    }

  }

  return QIcon ();
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
      const db::Circuit *circuit = circuit_from_id (id);
      return circuit->device_count () + circuit->pin_count () + circuit->net_count () + circuit->subcircuit_count () > 0;
    } else if (is_id_circuit_pin (id)) {
      return true;
    } else if (is_id_circuit_device (id)) {
      const db::Device *device = device_from_id (id);
      return device->device_class () && ! device->device_class ()->terminal_definitions ().empty ();
    } else if (is_id_circuit_subcircuit (id)) {
      const db::SubCircuit *subcircuit = subcircuit_from_id (id);
      return subcircuit->circuit_ref () && subcircuit->circuit_ref ()->pin_count () > 0;
    } else if (is_id_circuit_net (id)) {
      const db::Net *net = net_from_id (id);
      return net->pin_count () + net->subcircuit_pin_count () + net->terminal_count () > 0;
    } else if (is_id_circuit_net_subcircuit_pin (id)) {
      const db::NetSubcircuitPinRef *ref = net_subcircuit_pinref_from_id (id);
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
NetlistBrowserModel::headerData (int section, Qt::Orientation /*orientation*/, int role) const
{
  if (role == Qt::DisplayRole) {
    if (section == 0) {
      return tr ("Object");
    } else if (section == 1) {
      return tr ("Attribute");
    }
  }

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

      int r = row;
      const db::Circuit *circuit = circuit_from_id (id);
      if (r < int (circuit->pin_count ())) {
        new_id = make_id_circuit_pin (circuit_index_from_id (id), size_t (r));
      } else {
        r -= int (circuit->pin_count ());
        if (r < int (circuit->net_count ())) {
          new_id = make_id_circuit_net (circuit_index_from_id (id), size_t (r));
        } else {
          r -= int (circuit->net_count ());
          if (r < int (circuit->subcircuit_count ())) {
            new_id = make_id_circuit_subcircuit (circuit_index_from_id (id), size_t (r));
          } else {
            r -= int (circuit->subcircuit_count ());
            if (r < int (circuit->device_count ())) {
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
      const db::Net *net = net_from_id (id);
      if (r < int (net->terminal_count ())) {
        new_id = make_id_circuit_net_device_terminal (circuit_index_from_id (id), circuit_net_index_from_id (id), size_t (r));
      } else {
        r -= int (net->terminal_count ());
        if (r < int (net->pin_count ())) {
          new_id = make_id_circuit_net_pin (circuit_index_from_id (id), circuit_net_index_from_id (id), size_t (r));
        } else {
          r -= int (net->pin_count ());
          if (r < int (net->subcircuit_pin_count ())) {
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

  return createIndex (row, column, new_id);
}

void
NetlistBrowserModel::colors_changed ()
{
  emit dataChanged (index (0, 0, QModelIndex ()), index (rowCount (QModelIndex()) - 1, 0, QModelIndex ()));
}

QModelIndex
NetlistBrowserModel::index_from_net (const db::Net *net) const
{
  void *id = make_id_circuit_net (circuit_index (net->circuit ()), net_index (net));
  return index_from_id (id, 0);
}

const db::Net *
NetlistBrowserModel::net_from_index (const QModelIndex &index) const
{
  void *id = index.internalPointer ();
  if (is_id_circuit_net (id)) {

    return net_from_id (id);

  } else if (is_id_circuit_device_terminal (id)) {

    const db::Device *device = device_from_id (id);
    if (device && device->device_class ()) {
      size_t terminal = circuit_device_terminal_index_from_id (id);
      if (device->device_class ()->terminal_definitions ().size () > terminal) {
        return device->net_for_terminal (device->device_class ()->terminal_definitions () [terminal].id ());
      }
    }

  } else if (is_id_circuit_pin_net (id)) {

    const db::Circuit *circuit = circuit_from_id (id);
    const db::Pin *pin = pin_from_id (id);
    if (pin) {
      return circuit->net_for_pin (pin->id ());
    }

  } else if (is_id_circuit_subcircuit_pin (id)) {

    const db::SubCircuit *subcircuit = subcircuit_from_id (id);
    if (subcircuit && subcircuit->circuit () && subcircuit->circuit_ref ()) {
      const db::Pin *pin = pin_from_id (id);
      if (pin) {
        return subcircuit->net_for_pin (pin->id ());
      }
    }

  } else if (is_id_circuit_net_subcircuit_pin_others (id)) {

    const db::NetSubcircuitPinRef *ref = net_subcircuit_pinref_from_id (id);
    size_t other_index = circuit_net_subcircuit_pin_other_index_from_id (id);

    if (ref && ref->pin () && ref->subcircuit ()) {
      const db::Circuit *circuit = ref->subcircuit ()->circuit_ref ();
      if (circuit && circuit->pin_by_id (other_index)) {
        const db::Pin *pin = circuit->pin_by_id (other_index);
        return ref->subcircuit ()->net_for_pin (pin->id ());
      }
    }

  } else if (is_id_circuit_net_device_terminal_others (id)) {

    const db::NetTerminalRef *ref = net_terminalref_from_id (id);
    size_t other_index = circuit_net_device_terminal_other_index_from_id (id);

    if (ref && ref->device_class () && ref->device_class ()->terminal_definitions ().size () > other_index) {
      const db::DeviceTerminalDefinition &def = ref->device_class ()->terminal_definitions ()[other_index];
      return ref->device ()->net_for_terminal (def.id ());
    }

  }

  return 0;
}

QModelIndex
NetlistBrowserModel::index_from_id (void *id, int column) const
{
  if (is_id_circuit (id)) {

    return createIndex (circuit_index_from_id (id), column, id);

  } else if (is_id_circuit_pin (id)) {

    return createIndex (int (circuit_pin_index_from_id (id)), column, id);

  } else if (is_id_circuit_pin_net (id)) {

    return createIndex (0, column, id);

  } else if (is_id_circuit_net (id)) {

    const db::Circuit *circuit = circuit_from_id (id);
    return createIndex (int (circuit->pin_count () + circuit_net_index_from_id (id)), column, id);

  } else if (is_id_circuit_net_device_terminal (id)) {

    return createIndex (circuit_net_device_terminal_index_from_id (id), column, id);

  } else if (is_id_circuit_net_device_terminal_others (id)) {

    return createIndex (circuit_net_device_terminal_other_index_from_id (id), column, id);

  } else if (is_id_circuit_net_pin (id)) {

    const db::Net *net = net_from_id (id);
    return createIndex (net->terminal_count () + circuit_net_pin_index_from_id (id), column, id);

  } else if (is_id_circuit_net_subcircuit_pin (id)) {

    const db::Net *net = net_from_id (id);
    return createIndex (net->terminal_count () + net->pin_count () + circuit_net_subcircuit_pin_index_from_id (id), column, id);

  } else if (is_id_circuit_net_subcircuit_pin_others (id)) {

    return createIndex (circuit_net_subcircuit_pin_other_index_from_id (id), column, id);

  } else if (is_id_circuit_subcircuit (id)) {

    const db::Circuit *circuit = circuit_from_id (id);
    return createIndex (int (circuit->pin_count () + circuit->net_count () + circuit_subcircuit_index_from_id (id)), column, id);

  } else if (is_id_circuit_subcircuit_pin (id)) {

    return createIndex (int (circuit_subcircuit_pin_index_from_id (id)), column, id);

  } else if (is_id_circuit_device (id)) {

    const db::Circuit *circuit = circuit_from_id (id);
    return createIndex (int (circuit->pin_count () + circuit->net_count () + circuit->subcircuit_count () + circuit_device_index_from_id (id)), column, id);

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

      const db::Circuit *circuit = circuit_from_id (id);
      return createIndex (int (circuit->pin_count () + circuit_net_index_from_id (id)), column, make_id_circuit_net (circuit_index_from_id (id), circuit_net_index_from_id (id)));

    } else if (is_id_circuit_subcircuit_pin (id)) {

      const db::Circuit *circuit = circuit_from_id (id);
      return createIndex (int (circuit->pin_count () + circuit->net_count () + circuit_subcircuit_index_from_id (id)), column, make_id_circuit_subcircuit (circuit_index_from_id (id), circuit_subcircuit_index_from_id (id)));

    } else if (is_id_circuit_device_terminal (id)) {

      const db::Circuit *circuit = circuit_from_id (id);
      return createIndex (int (circuit->pin_count () + circuit->net_count () + circuit->subcircuit_count () + circuit_device_index_from_id (id)), column, make_id_circuit_device (circuit_index_from_id (id), circuit_device_index_from_id (id)));

    } else if (is_id_circuit_net_device_terminal_others (id)) {

      return createIndex (circuit_net_device_terminal_index_from_id (id), column, make_id_circuit_net_device_terminal (circuit_index_from_id (id), circuit_net_index_from_id (id), circuit_net_device_terminal_index_from_id (id)));

    } else if (is_id_circuit_net_subcircuit_pin_others (id)) {

      const db::Net *net = net_from_id (id);
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

      const db::Circuit *circuit = circuit_from_id (id);
      return int (circuit->pin_count () + circuit->net_count () + circuit->subcircuit_count () + circuit->device_count ());

    } else if (is_id_circuit_pin (id)) {

      return 1;

    } else if (is_id_circuit_device (id) || is_id_circuit_net_device_terminal (id)) {

      const db::Device *device = device_from_id (id);
      return int (device->device_class () ? device->device_class ()->terminal_definitions ().size () : 0);

    } else if (is_id_circuit_subcircuit (id) || is_id_circuit_net_subcircuit_pin (id)) {

      const db::SubCircuit *subcircuit = subcircuit_from_id (id);
      return int (subcircuit->circuit_ref () ? subcircuit->circuit_ref ()->pin_count () : 0);

    } else if (is_id_circuit_net (id)) {

      const db::Net *net = net_from_id (id);
      return int (net->terminal_count () + net->pin_count () + net->subcircuit_pin_count ());

    }

  }

  return 0;
}

const db::Circuit *
NetlistBrowserModel::circuit_from_id (void *id) const
{
  size_t index = circuit_index_from_id (id);

  std::map<size_t, const db::Circuit *>::iterator c = m_circuit_by_index.find (index);
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

const db::Net *
NetlistBrowserModel::net_from_id (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  size_t index = circuit_net_index_from_id (id);

  return attr_by_object_and_index (circuit, index, circuit->begin_nets (), circuit->end_nets (), m_net_by_circuit_and_index);
}

const db::NetSubcircuitPinRef *
NetlistBrowserModel::net_subcircuit_pinref_from_id (void *id) const
{
  const db::Net *net = net_from_id (id);
  size_t index = circuit_net_subcircuit_pin_index_from_id (id);

  return attr_by_object_and_index (net, index, net->begin_subcircuit_pins (), net->end_subcircuit_pins (), m_subcircuit_pinref_by_net_and_index);
}

const db::NetPinRef *
NetlistBrowserModel::net_pinref_from_id (void *id) const
{
  const db::Net *net = net_from_id (id);
  size_t index = circuit_net_pin_index_from_id (id);

  return attr_by_object_and_index (net, index, net->begin_pins (), net->end_pins (), m_pinref_by_net_and_index);
}

const db::NetTerminalRef *
NetlistBrowserModel::net_terminalref_from_id (void *id) const
{
  const db::Net *net = net_from_id (id);
  size_t index = circuit_net_device_terminal_index_from_id (id);

  return attr_by_object_and_index (net, index, net->begin_terminals (), net->end_terminals (), m_terminalref_by_net_and_index);
}

const db::Device *
NetlistBrowserModel::device_from_id (void *id) const
{
  const db::Circuit *circuit = circuit_from_id (id);
  size_t index = circuit_device_index_from_id (id);

  return attr_by_object_and_index (circuit, index, circuit->begin_devices (), circuit->end_devices (), m_device_by_circuit_and_index);
}

const db::Pin *
NetlistBrowserModel::pin_from_id (void *id) const
{
  if (is_id_circuit_subcircuit_pin (id)) {

    const db::SubCircuit *subcircuit = subcircuit_from_id (id);
    const db::Circuit *circuit = subcircuit->circuit_ref ();
    size_t index = circuit_subcircuit_pin_index_from_id (id);

    return attr_by_object_and_index (circuit, index, circuit->begin_pins (), circuit->end_pins (), m_pin_by_circuit_and_index);

  } else {

    const db::Circuit *circuit = circuit_from_id (id);
    size_t index = circuit_pin_index_from_id (id);

    return attr_by_object_and_index (circuit, index, circuit->begin_pins (), circuit->end_pins (), m_pin_by_circuit_and_index);

  }
}

const db::SubCircuit *
NetlistBrowserModel::subcircuit_from_id (void *id) const
{
  if (is_id_circuit_subcircuit_pin (id) || is_id_circuit_subcircuit (id)) {

    const db::Circuit *circuit = circuit_from_id (id);
    size_t index = circuit_subcircuit_index_from_id (id);

    return attr_by_object_and_index (circuit, index, circuit->begin_subcircuits (), circuit->end_subcircuits (), m_subcircuit_by_circuit_and_index);

  } else {

    const db::NetSubcircuitPinRef *pinref = net_subcircuit_pinref_from_id (id);
    return pinref ? pinref->subcircuit () : 0;

  }
}

size_t
NetlistBrowserModel::circuit_index (const db::Circuit *circuit) const
{
  return index_from_attr (circuit, netlist ()->begin_circuits (), netlist ()->end_circuits (), m_circuit_index_by_object);
}

size_t
NetlistBrowserModel::net_index (const db::Net *net) const
{
  const db::Circuit *circuit = net->circuit ();
  return index_from_attr (net, circuit->begin_nets (), circuit->end_nets (), m_net_index_by_object);
}

size_t
NetlistBrowserModel::pin_index (const db::Pin *pin, const db::Circuit *circuit) const
{
  return index_from_attr (pin, circuit->begin_pins (), circuit->end_pins (), m_pin_index_by_object);
}

size_t
NetlistBrowserModel::subcircuit_index (const db::SubCircuit *sub_circuit) const
{
  const db::Circuit *circuit = sub_circuit->circuit ();
  return index_from_attr (sub_circuit, circuit->begin_subcircuits (), circuit->end_subcircuits (), m_subcircuit_index_by_object);
}

// ----------------------------------------------------------------------------------
//  NetlistBrowserPage implementation

NetlistBrowserPage::NetlistBrowserPage (QWidget * /*parent*/)
  : m_show_all (true),
    m_window (lay::NetlistBrowserConfig::FitNet),
    m_window_dim (0.0),
    m_max_shape_count (1000),
    m_marker_line_width (-1),
    m_marker_vertex_size (-1),
    m_marker_halo (-1),
    m_marker_dither_pattern (-1),
    m_marker_intensity (0),
    mp_view (0),
    m_cv_index (0),
    mp_plugin_root (0),
    m_history_ptr (0),
    m_signals_enabled (true),
    m_enable_updates (true),
    m_update_needed (true),
    mp_info_dialog (0)
{
  Ui::NetlistBrowserPage::setupUi (this);

  //  @@@ insert into menu
  m_show_all_action = new QAction (QObject::tr ("Show All"), this);
  m_show_all_action->setCheckable (true);
  m_show_all_action->setChecked (m_show_all);

  QAction *color_action = new QAction (QObject::tr ("Colorize Nets"), directory_tree);
  QMenu *menu = new QMenu (directory_tree);
  lay::ColorButton::build_color_menu (menu, this, SLOT (browse_color_for_net ()), SLOT (select_color_for_net ()));
  color_action->setMenu (menu);

  directory_tree->addAction (actionCollapseAll);
  directory_tree->addAction (actionExpandAll);
  QAction *sep;
  sep = new QAction (directory_tree);
  sep->setSeparator (true);
  directory_tree->addAction (sep);
  directory_tree->addAction (color_action);
  sep = new QAction (directory_tree);
  sep->setSeparator (true);
  directory_tree->addAction (sep);
  directory_tree->addAction (actionExportSelected);
  directory_tree->addAction (actionExportAll);

  lay::HTMLItemDelegate *delegate;

  delegate = new lay::HTMLItemDelegate (this);
  delegate->set_text_margin (2);
  delegate->set_anchors_clickable (true);
  connect (delegate, SIGNAL (anchor_clicked (const QString &)), this, SLOT (anchor_clicked (const QString &)));
  directory_tree->setItemDelegateForColumn (1, delegate);

  delegate = new lay::HTMLItemDelegate (this);
  delegate->set_text_margin (2);
  delegate->set_anchors_clickable (true);
  connect (delegate, SIGNAL (anchor_clicked (const QString &)), this, SLOT (anchor_clicked (const QString &)));
  directory_tree->setItemDelegateForColumn (0, delegate);

  QMenu *find_edit_menu = new QMenu (find_text);
  find_edit_menu->addAction (actionUseRegularExpressions);
  find_edit_menu->addAction (actionCaseSensitive);

  find_text->set_clear_button_enabled (true);
  find_text->set_options_button_enabled (true);
  find_text->set_options_menu (find_edit_menu);
#if QT_VERSION >= 0x40700
  find_text->setPlaceholderText (tr ("Find text ..."));
#endif

  connect (m_show_all_action, SIGNAL (triggered ()), this, SLOT (show_all_clicked ()));
  connect (info_button, SIGNAL (pressed ()), this, SLOT (info_button_pressed ()));
  connect (find_button, SIGNAL (pressed ()), this, SLOT (find_button_pressed ()));
  connect (forward, SIGNAL (clicked ()), this, SLOT (navigate_forward ()));
  connect (backward, SIGNAL (clicked ()), this, SLOT (navigate_back ()));

  connect (actionExportAll, SIGNAL (triggered ()), this, SLOT (export_all ()));
  connect (actionExportSelected, SIGNAL (triggered ()), this, SLOT (export_selected ()));

  forward->setEnabled (false);
  backward->setEnabled (false);
}

NetlistBrowserPage::~NetlistBrowserPage ()
{
  clear_markers ();
}

void
NetlistBrowserPage::set_plugin_root (lay::PluginRoot *pr)
{
  mp_plugin_root = pr;
}

void
NetlistBrowserPage::set_highlight_style (QColor color, int line_width, int vertex_size, int halo, int dither_pattern, int marker_intensity, const lay::ColorPalette *auto_colors)
{
  m_colorizer.configure (color, auto_colors);
  m_marker_line_width = line_width;
  m_marker_vertex_size = vertex_size;
  m_marker_halo = halo;
  m_marker_dither_pattern = dither_pattern;
  m_marker_intensity = marker_intensity;
  update_highlights ();
}

void
NetlistBrowserPage::set_view (lay::LayoutView *view, unsigned int cv_index)
{
  if (mp_view) {
    mp_view->layer_list_changed_event.remove (this, &NetlistBrowserPage::layer_list_changed);
  }

  mp_view = view;
  m_cv_index = cv_index;

  if (mp_view) {
    mp_view->layer_list_changed_event.add (this, &NetlistBrowserPage::layer_list_changed);
  }

  update_highlights ();
}

void
NetlistBrowserPage::set_window (lay::NetlistBrowserConfig::net_window_type window, double window_dim)
{
  if (window != m_window || window_dim != m_window_dim) {
    m_window = window;
    m_window_dim = window_dim;
  }
}

void
NetlistBrowserPage::set_max_shape_count (size_t max_shape_count)
{
  if (m_max_shape_count != max_shape_count) {
    m_max_shape_count = max_shape_count;
    update_highlights ();
  }
}

void
NetlistBrowserPage::layer_list_changed (int)
{
  update_highlights ();
}

void
NetlistBrowserPage::anchor_clicked (const QString &a)
{
  QUrl url (a);
  QString ids = url.queryItemValue (QString::fromUtf8 ("id"));
  if (ids.isEmpty ()) {
    return;
  }

  void *id = reinterpret_cast<void *> (ids.toULongLong ());
  navigate_to (id, true);
}

void
NetlistBrowserPage::current_index_changed (const QModelIndex &index)
{
  if (index.isValid () && m_signals_enabled) {

    void *id = index.internalPointer ();
    add_to_history (id, true);

  }
}

void
NetlistBrowserPage::select_net (const db::Net *net)
{
  if (! net || ! net->circuit ()) {
    directory_tree->clearSelection ();
  } else {
    NetlistBrowserModel *model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
    tl_assert (model != 0);
    directory_tree->setCurrentIndex (model->index_from_net (net));
  }
}

std::vector<const db::Net *>
NetlistBrowserPage::selected_nets ()
{
  NetlistBrowserModel *model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
  tl_assert (model != 0);

  std::vector<const db::Net *> nets;

  QModelIndexList selection = directory_tree->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = selection.begin (); i != selection.end (); ++i) {
    if (i->column () == 0) {
      const db::Net *net = model->net_from_index (*i);
      if (net) {
        nets.push_back (net);
      }
    }
  }

  return nets;
}

void
NetlistBrowserPage::net_selection_changed ()
{
  std::vector<const db::Net *> nets = selected_nets ();
  if (mp_info_dialog) {
    mp_info_dialog->set_nets (mp_database.get (), nets);
  }
  highlight_nets (nets);
}

void
NetlistBrowserPage::set_color_for_selected_nets (const QColor &color)
{
  std::vector<const db::Net *> nets = selected_nets ();

  m_colorizer.begin_changes ();
  for (std::vector<const db::Net *>::const_iterator n = nets.begin (); n != nets.end (); ++n) {
    if (color.isValid ()) {
      m_colorizer.set_color_of_net (*n, color);
    } else {
      m_colorizer.reset_color_of_net (*n);
    }
  }
  m_colorizer.end_changes ();

  update_highlights ();
}

void
NetlistBrowserPage::browse_color_for_net ()
{
  QColor c = QColorDialog::getColor (QColor (), this);
  if (c.isValid ()) {
    set_color_for_selected_nets (c);
  }
}

void
NetlistBrowserPage::select_color_for_net ()
{
  QAction *action = dynamic_cast<QAction *> (sender ());
  if (action) {
    set_color_for_selected_nets (action->data ().value<QColor> ());
  }
}

void
NetlistBrowserPage::navigate_to (void *id, bool fwd)
{
  NetlistBrowserModel *model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
  if (! model) {
    return;
  }

  QModelIndex index = model->index_from_id (id, 0);
  if (! index.isValid ()) {
    return;
  }

  m_signals_enabled = false;
  try {
    directory_tree->setCurrentIndex (index);
  } catch (...) {
  }
  m_signals_enabled = true;

  add_to_history (id, fwd);

  net_selection_changed ();
}

void
NetlistBrowserPage::add_to_history (void *id, bool fwd)
{
  if (! fwd) {
    if (m_history_ptr > 1) {
      --m_history_ptr;
      m_history [m_history_ptr - 1] = id;
    }
  } else if (m_history_ptr >= m_history.size ()) {
    m_history.push_back (id);
    m_history_ptr = m_history.size ();
  } else {
    if (m_history [m_history_ptr] != id) {
      m_history.erase (m_history.begin () + m_history_ptr + 1, m_history.end ());
    }
    m_history [m_history_ptr] = id;
    ++m_history_ptr;
  }

  backward->setEnabled (m_history_ptr > 1);
  forward->setEnabled (m_history_ptr < m_history.size ());
}

void
NetlistBrowserPage::navigate_back ()
{
  if (m_history_ptr > 1) {
    navigate_to (m_history [m_history_ptr - 2], false);
  }
}

void
NetlistBrowserPage::navigate_forward ()
{
  if (m_history_ptr < m_history.size ()) {
    navigate_to (m_history [m_history_ptr]);
  }
}

void
NetlistBrowserPage::info_button_pressed ()
{
  if (! mp_info_dialog) {
    mp_info_dialog = new lay::NetInfoDialog (this);
  }

  mp_info_dialog->set_nets (mp_database.get (), selected_nets ());
  mp_info_dialog->show ();
}

static QModelIndex find_next (QAbstractItemModel *model, const QRegExp &to_find, const QModelIndex &from)
{
  QModelIndex index = from;

  if (! index.isValid () && model->hasChildren (index)) {
    index = model->index (0, 0, index);
  }

  if (! index.isValid ()) {
    return index;
  }

  QModelIndex current = index;

  std::vector<QModelIndex> parent_stack;
  std::vector<std::pair<int, int> > rows_stack;

  while (index.isValid ()) {

    parent_stack.push_back (model->parent (index));
    rows_stack.push_back (std::make_pair (index.row (), model->rowCount (parent_stack.back ())));

    index = parent_stack.back ();

  }

  std::reverse (parent_stack.begin (), parent_stack.end ());
  std::reverse (rows_stack.begin (), rows_stack.end ());

  tl::AbsoluteProgress progress (tl::to_string (tr ("Searching ...")));

  do {

    ++progress;

    bool has_next = false;

    if (model->hasChildren (current) && rows_stack.size () < 2) {

      int row_count = model->rowCount (current);
      if (row_count > 0) {

        parent_stack.push_back (current);
        rows_stack.push_back (std::make_pair (0, row_count));

        current = model->index (0, 0, current);
        has_next = true;

      }

    }

    while (! has_next && ! rows_stack.empty ()) {

      ++rows_stack.back ().first;

      if (rows_stack.back ().first >= rows_stack.back ().second) {

        //  up
        current = parent_stack.back ();
        rows_stack.pop_back ();
        parent_stack.pop_back ();

      } else {

        current = model->index (rows_stack.back ().first, 0, parent_stack.back ());
        has_next = true;

      }

    }

    if (has_next) {

      QString text = model->data (current, Qt::UserRole).toString ();
      if (text.indexOf (to_find) >= 0) {
        return current;
      }

    }

  } while (current.internalPointer () != from.internalPointer () || current.row () != from.row ());

  return QModelIndex ();
}

void
NetlistBrowserPage::find_button_pressed ()
{
  QRegExp re (find_text->text (),
              actionCaseSensitive->isChecked () ? Qt::CaseSensitive : Qt::CaseInsensitive,
              actionUseRegularExpressions->isChecked () ? QRegExp::RegExp : QRegExp::FixedString);

  QModelIndex next = find_next (directory_tree->model (), re, directory_tree->currentIndex ());
  if (next.isValid ()) {
    navigate_to (next.internalPointer ());
  }
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
NetlistBrowserPage::set_l2ndb (db::LayoutToNetlist *database)
{
  if (mp_info_dialog) {
    delete mp_info_dialog;
    mp_info_dialog = 0;
  }

  mp_database.reset (database);
  clear_markers ();
  highlight_nets (std::vector<const db::Net *> ());

  if (! database) {
    delete directory_tree->model ();
    directory_tree->setModel (0);
    return;
  }

  //  NOTE: with the tree as the parent, the tree will take over ownership of the model
  NetlistBrowserModel *new_model = new NetlistBrowserModel (directory_tree, database, &m_colorizer);

  delete directory_tree->model ();
  directory_tree->setModel (new_model);
  connect (directory_tree->selectionModel (), SIGNAL (currentChanged (const QModelIndex &, const QModelIndex &)), this, SLOT (current_index_changed (const QModelIndex &)));
  connect (directory_tree->selectionModel (), SIGNAL (selectionChanged (const QItemSelection &, const QItemSelection &)), this, SLOT (net_selection_changed ()));

  directory_tree->header ()->setSortIndicatorShown (true);

  find_text->setText (QString ());
}

void
NetlistBrowserPage::highlight_nets (const std::vector<const db::Net *> &nets)
{
  if (nets != m_current_nets) {

    m_current_nets = nets;
    clear_markers ();

    if (! nets.empty ()) {
      adjust_view ();
      update_highlights ();
    }

  }
}

void
NetlistBrowserPage::enable_updates (bool f)
{
  if (f != m_enable_updates) {

    m_enable_updates = f;

    if (f && m_update_needed) {
      update_highlights ();
      // @@@ update_info_text ();
    }

    m_update_needed = false;

  }
}

void
NetlistBrowserPage::adjust_view ()
{
  if (! mp_database.get () || ! mp_view) {
    return;
  }

  const lay::CellView &cv = mp_view->cellview (m_cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  if (m_window != lay::NetlistBrowserConfig::FitNet && m_window != lay::NetlistBrowserConfig::Center && m_window != lay::NetlistBrowserConfig::CenterSize) {
    return;
  }

  const db::Layout *layout = mp_database->internal_layout ();
  if (! layout) {
    return;
  }

  db::DBox bbox;

  for (std::vector<const db::Net *>::const_iterator net = m_current_nets.begin (); net != m_current_nets.end (); ++net) {

    if (! (*net)->circuit ()) {
      continue;
    }

    db::ICplxTrans net_trans = trans_for_net (*net);

    db::cell_index_type cell_index = (*net)->circuit ()->cell_index ();
    size_t cluster_id = (*net)->cluster_id ();

    // @@@std::map<unsigned int, std::vector<db::DCplxTrans> > tv_by_layer = mp_view->cv_transform_variants_by_layer (m_cv_index);
    std::vector<db::DCplxTrans> tv = mp_view->cv_transform_variants (m_cv_index);

    const db::Connectivity &conn = mp_database->connectivity ();
    for (db::Connectivity::layer_iterator layer = conn.begin_layers (); layer != conn.end_layers (); ++layer) {

      //  @@@ TODO: how to get the original layer?

      db::Box layer_bbox;
      db::recursive_cluster_shape_iterator<db::PolygonRef> shapes (mp_database->net_clusters (), *layer, cell_index, cluster_id);
      while (! shapes.at_end ()) {
        layer_bbox += shapes.trans () * shapes->box ();
        ++shapes;
      }

      for (std::vector<db::DCplxTrans>::const_iterator t = tv.begin (); t != tv.end (); ++t) {
        bbox += *t * db::CplxTrans (layout->dbu ()) * net_trans * layer_bbox;
      }

    }

  }

  if (! bbox.empty ()) {

    if (m_window == lay::NetlistBrowserConfig::FitNet) {

      mp_view->zoom_box (bbox.enlarged (db::DVector (m_window_dim, m_window_dim)));

    } else if (m_window == lay::NetlistBrowserConfig::Center) {

      mp_view->pan_center (bbox.p1 () + (bbox.p2 () - bbox.p1 ()) * 0.5);

    } else if (m_window == lay::NetlistBrowserConfig::CenterSize) {

      double w = std::max (bbox.width (), m_window_dim);
      double h = std::max (bbox.height (), m_window_dim);
      db::DPoint center (bbox.p1() + (bbox.p2 () - bbox.p1 ()) * 0.5);
      db::DVector d (w * 0.5, h * 0.5);
      mp_view->zoom_box (db::DBox (center - d, center + d));

    }

  }
}

bool
NetlistBrowserPage::produce_highlights_for_net (const db::Net *net, size_t &n_markers, const std::map<db::LayerProperties, lay::LayerPropertiesConstIterator> &display_by_lp, const std::vector<db::DCplxTrans> &tv)
{
  db::ICplxTrans net_trans = trans_for_net (net);
  const db::Layout *layout = mp_database->internal_layout ();

  db::cell_index_type cell_index = net->circuit ()->cell_index ();
  size_t cluster_id = net->cluster_id ();

  QColor net_color = m_colorizer.color_of_net (net);

  const db::Connectivity &conn = mp_database->connectivity ();
  for (db::Connectivity::layer_iterator layer = conn.begin_layers (); layer != conn.end_layers (); ++layer) {

    db::LayerProperties lp = layout->get_properties (*layer);
    std::map<db::LayerProperties, lay::LayerPropertiesConstIterator>::const_iterator display = display_by_lp.find (lp);

    db::recursive_cluster_shape_iterator<db::PolygonRef> shapes (mp_database->net_clusters (), *layer, cell_index, cluster_id);
    while (! shapes.at_end ()) {

      if (n_markers == m_max_shape_count) {
        return true;
      }

      mp_markers.push_back (new lay::Marker (mp_view, m_cv_index));
      mp_markers.back ()->set (*shapes, net_trans * shapes.trans (), tv);

      if (net_color.isValid ()) {

        mp_markers.back ()->set_color (net_color);
        mp_markers.back ()->set_frame_color (net_color);

      } else if (display != display_by_lp.end ()) {

        mp_markers.back ()->set_line_width (display->second->width (true));
        mp_markers.back ()->set_vertex_size (1);
        mp_markers.back ()->set_dither_pattern (display->second->dither_pattern (true));
        if (mp_view->background_color ().green () < 128) {
          mp_markers.back ()->set_color (display->second->eff_fill_color_brighter (true, (m_marker_intensity * 255) / 100));
          mp_markers.back ()->set_frame_color (display->second->eff_frame_color_brighter (true, (m_marker_intensity * 255) / 100));
        } else {
          mp_markers.back ()->set_color (display->second->eff_fill_color_brighter (true, (-m_marker_intensity * 255) / 100));
          mp_markers.back ()->set_frame_color (display->second->eff_frame_color_brighter (true, (-m_marker_intensity * 255) / 100));
        }

      } else {

        //  fallback color
        QColor net_color = mp_view->background_color ().green () < 128 ? QColor (Qt::white) : QColor (Qt::black);
        mp_markers.back ()->set_color (net_color);
        mp_markers.back ()->set_frame_color (net_color);

      }

      if (m_marker_line_width >= 0) {
        mp_markers.back ()->set_line_width (m_marker_line_width);
      }

      if (m_marker_vertex_size >= 0) {
        mp_markers.back ()->set_vertex_size (m_marker_vertex_size);
      }

      if (m_marker_halo >= 0) {
        mp_markers.back ()->set_halo (m_marker_halo);
      }

      if (m_marker_dither_pattern >= 0) {
        mp_markers.back ()->set_dither_pattern (m_marker_dither_pattern);
      }

      ++shapes;
      ++n_markers;

    }

  }

  return false;
}

db::ICplxTrans
NetlistBrowserPage::trans_for_net (const db::Net *net)
{
  db::DCplxTrans t;

  const db::Circuit *circuit = net->circuit ();
  while (circuit) {
    if (circuit->begin_refs () != circuit->end_refs ()) {
      const db::SubCircuit &ref = *circuit->begin_refs ();
      t = ref.trans () * t;
      circuit = ref.circuit ();
    } else {
      break;
    }
  }

  double dbu = mp_database->internal_layout ()->dbu ();
  db::CplxTrans dbu_trans (dbu);

  return dbu_trans.inverted () * t * dbu_trans;
}

void
NetlistBrowserPage::update_highlights ()
{
  if (! m_enable_updates) {
    m_update_needed = true;
    return;
  }

  clear_markers ();
  if (! mp_database.get () || ! mp_view) {
    return;
  }

  const db::Layout &original_layout = mp_view->cellview (m_cv_index)->layout ();

  const db::Layout *layout = mp_database->internal_layout ();
  if (! layout) {
    return;
  }

  //  a map of display properties vs. layer properties

  std::map<db::LayerProperties, lay::LayerPropertiesConstIterator> display_by_lp;
  for (lay::LayerPropertiesConstIterator lp = mp_view->begin_layers (); ! lp.at_end (); ++lp) {
    if (! lp->has_children () && lp->cellview_index () == int (m_cv_index) && lp->layer_index () >= 0 && (unsigned int) lp->layer_index () < original_layout.layers ()) {
      display_by_lp.insert (std::make_pair (original_layout.get_properties (lp->layer_index ()), lp));
    }
  }

  // @@@std::map<unsigned int, std::vector<db::DCplxTrans> > tv_by_layer = mp_view->cv_transform_variants_by_layer (m_cv_index);
  std::vector<db::DCplxTrans> tv = mp_view->cv_transform_variants (m_cv_index);

  //  correct DBU differences between the storage layout and the original layout
  for (std::vector<db::DCplxTrans>::iterator t = tv.begin (); t != tv.end (); ++t) {
    *t = *t * db::DCplxTrans (layout->dbu () / original_layout.dbu ());
  }

  size_t n_markers = 0;
  bool not_all_shapes_are_shown = false;

  for (std::vector<const db::Net *>::const_iterator net = m_current_nets.begin (); net != m_current_nets.end () && ! not_all_shapes_are_shown; ++net) {
    if ((*net)->circuit ()) {
      not_all_shapes_are_shown = produce_highlights_for_net (*net, n_markers, display_by_lp, tv);
    }
  }

  if (not_all_shapes_are_shown) {
    info_label->setText (tl::to_qstring ("<html><p style=\"color:red; font-weight: bold\">" +
        tl::to_string (QObject::tr ("Not all shapes are highlighted")) +
        "</p></html>"));
    info_label->show ();
  } else {
    info_label->hide ();
  }
}

void
NetlistBrowserPage::clear_markers ()
{
  for (std::vector <lay::Marker *>::iterator m = mp_markers.begin (); m != mp_markers.end (); ++m) {
    delete *m;
  }

  mp_markers.clear ();
}

static std::map<unsigned int, const db::Region *>
create_layermap (const db::LayoutToNetlist *database, db::Layout &target_layout, int ln)
{
  std::map<unsigned int, const db::Region *> lm;

  const db::Layout &source_layout = *database->internal_layout ();

  std::set<unsigned int> layers_to_copy;
  const db::Connectivity &conn = database->connectivity ();
  for (db::Connectivity::layer_iterator layer = conn.begin_layers (); layer != conn.end_layers (); ++layer) {
    layers_to_copy.insert (*layer);
  }

  for (std::set<unsigned int>::const_iterator l = layers_to_copy.begin (); l != layers_to_copy.end (); ++l) {
    const db::LayerProperties &lp = source_layout.get_properties (*l);
    unsigned int tl;
    if (! lp.is_null ()) {
      tl = target_layout.insert_layer (lp);
    } else {
      tl = target_layout.insert_layer (db::LayerProperties (ln++, 0, database->name (*l)));
    }
    lm.insert (std::make_pair (tl, (const_cast<db::LayoutToNetlist *> (database))->layer_by_index (*l)));
  }

  return lm;
}

void
NetlistBrowserPage::export_selected ()
{
  std::vector<const db::Net *> nets = selected_nets ();
  if (nets.empty ()) {
    return;
  }

  export_nets (&nets);
}

void
NetlistBrowserPage::export_all ()
{
  export_nets (0);
}

void
NetlistBrowserPage::export_nets (const std::vector<const db::Net *> *nets)
{
  if (! mp_view || ! mp_database.get () || ! mp_database->internal_layout ()) {
    return;
  }

  const db::Layout &source_layout = *mp_database->internal_layout ();
  if (source_layout.begin_top_down () == source_layout.end_top_cells ()) {
    //  nothing to export
    return;
  }

  const db::Cell &source_top = source_layout.cell (*source_layout.begin_top_down ());

  std::auto_ptr<lay::NetExportDialog> dialog (new lay::NetExportDialog (this));
  if (dialog->exec (mp_plugin_root)) {

    //  NOTE: mp_view and database might get reset to 0 in create_layout
    lay::LayoutView *view = mp_view;
    db::LayoutToNetlist *database = mp_database.get ();

    unsigned int cv_index = view->create_layout (true);
    db::Layout &target_layout = view->cellview (cv_index)->layout ();

    db::cell_index_type target_top_index = target_layout.add_cell (source_layout.cell_name (source_top.cell_index ()));

    db::CellMapping cm = database->cell_mapping_into (target_layout, target_layout.cell (target_top_index));
    std::map<unsigned int, const db::Region *> lm = create_layermap (database, target_layout, dialog->start_layer_number ());

    std::set<const db::Net *> net_set;
    if (nets) {
      net_set.insert (nets->begin (), nets->end ());
    }

    database->build_nets (nets ? &net_set : 0, cm, target_layout, lm,
                          dialog->net_prefix ().empty () ? 0 : dialog->net_prefix ().c_str (),
                          dialog->net_propname (),
                          dialog->produce_circuit_cells () ? db::LayoutToNetlist::BNH_SubcircuitCells : db::LayoutToNetlist::BNH_Flatten,
                          dialog->produce_circuit_cells () ? dialog->circuit_cell_prefix ().c_str () : 0,
                          dialog->produce_device_cells () ? dialog->device_cell_prefix ().c_str () : 0);

    view->zoom_fit ();
    view->max_hier ();
    view->add_missing_layers ();
    view->select_cell (target_top_index, cv_index);

  }
}

}

