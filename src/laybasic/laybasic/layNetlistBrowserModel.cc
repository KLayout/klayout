
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
//  Implementation of the item classes

const std::string field_sep (" / ");

static QString escaped (const std::string &s)
{
  return tl::to_qstring (tl::escaped_to_html (s));
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

QString
NetlistBrowserModel::text (const QModelIndex &index) const
{
  NetlistModelItemData *d = (NetlistModelItemData *) (index.internalPointer ());
  if (! d) {
    return QString ();
  } else {
    return d->text (this);
  }
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

// ----------------------------------------------------------------------------------
//  item class declarations

// ----------------------------------------------------------------------------------
//  item class implementations

class CircuitItemData
  : public NetlistModelItemData
{
private:
  CircuitItemData (const IndexedNetlistModel::circuit_pair &cp)
    : NetlistModelItemData (), m_cp (cp)
  { }

  virtual void do_ensure_children (NetlistBrowserModel *model)
  {
    size_t n;

    n = model->indexer ()->pin_count (cp ());
    for (size_t i = 0; i < n; ++i) {
      push_back (new CircuitPinItemData (this, model->indexer ()->pin_from_index (cp (), i)));
    }

    n = model->indexer ()->net_count (cp ());
    for (size_t i = 0; i < n; ++i) {
      push_back (new CircuitNetItemData (this, model->indexer ()->net_from_index (cp (), i)));
    }

    n = model->indexer ()->subcircuit_count (cp ());
    for (size_t i = 0; i < n; ++i) {
      push_back (new CircuitSubCircuitItemData (this, model->indexer ()->subcircuit_from_index (cp (), i)));
    }

    n = model->indexer ()->device_count (cp ());
    for (size_t i = 0; i < n; ++i) {
      push_back (new CircuitDeviceItemData (this, model->indexer ()->device_from_index (cp (), i)));
    }
  }

  virtual QIcon icon ()
  {
    return icon_for_circuit ();
  }

  virtual QString text (int column, NetlistBrowserModel *model)
  {
    //  circuit:
    //  + single mode:     name              | <empty>  | <empty>
    //  + dual mode:       name(a)/name(b)   | name(a)  | name(b)
    if (column == model->object_column ()) {
      return escaped (str_from_names (m_cp, model->indexer ()->is_single ()));
    } else if (!model->indexer ()->is_single () && (column == model->first_column () || column == model->second_column ())) {
      return escaped (str_from_name (column == model->first_column () ? m_cp.first : m_cp.second));
    }
  }

  virtual QString search_text ()
  {
    return tl::to_qstring (search_string_from_names (cp ()));
  }

  virtual std::string tooltip (NetlistBrowserModel *model)
  {
    size_t index = model->indexer ()->circuit_index (m_cp);
    return model->indexer ()->circuit_status_hint (index);
  }

  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model)
  {
    size_t index = model->indexer ()->circuit_index (m_cp);
    return model->indexer ()->circuit_from_index (index).second;
  }

  const IndexedNetlistModel::circuit_pair &cp ()
  {
    return m_cp;
  }

private:
  IndexedNetlistModel::circuit_pair m_cp;
};

// ----------------------------------------------------------------------------------

class CircuitPinItemData
  : public NetlistModelItemData
{
private:
  CircuitPinItemData (CircuitItemData *parent, const IndexedNetlistModel::pin_pair &pp)
    : NetlistModelItemData (parent), m_pp (pp)
  { }

  virtual void do_ensure_children (NetlistBrowserModel * /*model*/)
  {
    push_back (new CircuitPinNetItemData (this, nets_from_circuit_pins (cp (), pp ())));
  }

  virtual QIcon icon ()
  {
    return icon_for_pin ();
  }

  virtual QString text (int column, NetlistBrowserModel *model)
  {
    //  pin:
    //  + single mode:     xname             | <empty>  | <empty>
    //  + dual mode:       xname(a)/xname(b) | xname(a) | xname(b)
    if (column == model->object_column ()) {
      return escaped (str_from_expanded_names (pp (), model->indexer ()->is_single ()));
    } else if (!model->indexer ()->is_single () && (column == model->first_column () || column == model->second_column ())) {
      return escaped (str_from_expanded_name (column == model->first_column () ? pp ().first : pp ().second));
    }

    return QString ();
  }

  virtual QString search_text ()
  {
    return tl::to_qstring (search_string_from_expanded_names (pp ()));
  }

  virtual std::string tooltip (NetlistBrowserModel *model)
  {
    size_t index = model->indexer ()->pin_index (pp (), cp ());
    return model->indexer ()->pin_status_hint (cp (), index);
  }

  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model)
  {
    size_t index = model->indexer ()->pin_index (pp (), cp ());
    return model->indexer ()->pin_from_index (cp (), index).second;
  }

  const IndexedNetlistModel::circuit_pair &cp ()
  {
    CircuitItemData *p = static_cast<CircuitItemData *> (parent ());
    return p->cp ();
  }

  const IndexedNetlistModel::pin_pair &pp ()
  {
    return m_pp;
  }

private:
  IndexedNetlistModel::pin_pair m_pp;
};

// ----------------------------------------------------------------------------------

class CircuitPinNetItemData
  : public NetlistModelItemData
{
private:
  CircuitPinNetItemData (CircuitPinItemData *parent, const IndexedNetlistModel::net_pair &np)
    : NetlistModelItemData (parent), m_np (np)
  { }

  virtual void do_ensure_children (NetlistBrowserModel *model)
  {
    //  nothing (leaf node)
  }

  virtual QIcon icon ()
  {
    return icon_for_connection (m_np);
  }

  virtual QString text (int column, NetlistBrowserModel *model)
  {
    //  circuit/pin/net: header column = name, second column link to net
    if (column == model->object_column ()) {
      return escaped (str_from_expanded_names (m_np, model->indexer ()->is_single ()));
    } else if (column == model->first_column () || column == model->second_column ()) {
      return make_link_to (m_np, column);
    }

    return QString ();
  }

  virtual QString search_text ()
  {
    return tl::to_qstring (search_string_from_expanded_names (nets_from_circuit_pins (cp (), pp ())));
  }

  virtual std::string tooltip (NetlistBrowserModel *model)
  {
    return std::string ();
  }

  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model)
  {
    return db::NetlistCrossReference::None;
  }

  const IndexedNetlistModel::circuit_pair &cp ()
  {
    CircuitPinItemData *p = static_cast<CircuitPinItemData *> (parent ());
    return p->cp ();
  }

  const IndexedNetlistModel::pin_pair &pp ()
  {
    CircuitPinItemData *p = static_cast<CircuitPinItemData *> (parent ());
    return p->pp ();
  }

private:
  IndexedNetlistModel::net_pair m_np;
};

// ----------------------------------------------------------------------------------

class CircuitNetItemData
  : public NetlistModelItemData
{
private:
  CircuitNetItemData (CircuitItemData *parent, const IndexedNetlistModel::net_pair &np)
    : NetlistModelItemData (parent), m_np (np)
  { }

  virtual void do_ensure_children (NetlistBrowserModel *model)
  {
    size_t n;

    n = model->indexer ()->net_terminal_count (cp ());
    for (size_t i = 0; i < n; ++i) {
      push_back (new CircuitNetDeviceTerminalItemData (this, model->indexer ()->net_terminalref_from_index (np (), i)));
    }

    n = model->indexer ()->net_pin_count (cp ());
    for (size_t i = 0; i < n; ++i) {
      push_back (new CircuitNetPinItemData (this, model->indexer ()->net_pinref_from_index (np (), i)));
    }

    n = model->indexer ()->net_subcircuit_pin_count (cp ());
    for (size_t i = 0; i < n; ++i) {
      push_back (new CircuitNetSubCircuitPinItemData (this, model->indexer ()->net_subcircuit_pinref_from_index (np (), i)));
    }
  }

  virtual QIcon icon ()
  {
    return icon_for_nets (m_np);
  }

  virtual QString text (int column, NetlistBrowserModel *model)
  {
    //  circuit/net: header column = node count, second column net name
    if (column == model->object_column ()) {
      return escaped (str_from_expanded_names (m_np, model->indexer ()->is_single ()));
    } else if (column == model->first_column () && m_np.first) {
      return escaped (m_np.first->expanded_name () + " (" + tl::to_string (m_np.first->pin_count () + m_np.first->terminal_count () + m_np.first->subcircuit_pin_count ()) + ")");
    } else if (column == model->second_column () && m_np.second) {
      return escaped (m_np.second->expanded_name () + " (" + tl::to_string (m_np.second->pin_count () + m_np.second->terminal_count () + m_np.second->subcircuit_pin_count ()) + ")");
    }

    return QString ();
  }

  virtual QString search_text ()
  {
    return tl::to_qstring (search_string_from_expanded_names (m_np));
  }

  virtual std::string tooltip (NetlistBrowserModel *model)
  {
    size_t index = model->indexer ()->net_index (m_np);
    return model->indexer ()->net_status_hint (cp (), index);
  }

  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model)
  {
    size_t index = model->indexer ()->net_index (m_np);
    return model->indexer ()->net_from_index (cp (), index).second;
  }

  const IndexedNetlistModel::circuit_pair &cp ()
  {
    CircuitItemData *p = static_cast<CircuitItemData *> (parent ());
    return p->cp ();
  }

  const IndexedNetlistModel::net_pair &np ()
  {
    return m_np;
  }

private:
  IndexedNetlistModel::net_pair m_np;
};

// ----------------------------------------------------------------------------------

class CircuitNetDeviceTerminalItemData
  : public NetlistModelItemData
{
private:
  CircuitNetDeviceTerminalItemData (CircuitNetItemData *parent, const IndexedNetlistModel::device_pair &dp, const IndexedNetlistModel::net_terminal_pair &tp)
    : NetlistModelItemData (parent), m_dp (dp), m_tp (tp)
  { }

  virtual void do_ensure_children (NetlistBrowserModel * /*model*/)
  {
    size_t n = std::max (rows_for (m_tp.first), rows_for (m_tp.second));
    for (size_t i = 0; i < n; ++i) {
      std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> termdefs = terminal_defs_from_terminal_refs (m_tp);
      IndexedNetlistModel::net_pair nets = nets_from_device_terminals (dp (), termdefs);
      push_back (new CircuitNetDeviceTerminalOthersItemData (this, nets, termdefs));
    }
  }

  virtual QIcon icon ()
  {
    std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (m_dp);
    return icon_for_devices (device_classes);
  }

  virtual QString text (int column, NetlistBrowserModel *model)
  {
    //  circuit/net/device terminal: header column = terminal and device string, second column = device name
    IndexedNetlistModel::device_pair devices = devices_from_termrefs (m_tp);

    if (column == model->object_column ()) {

      std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> termdefs = terminal_defs_from_terminal_refs (m_tp);

      if (model->indexer ()->is_single ()) {
        return escaped (str_from_name (termdefs.first) + field_sep + device_string (m_dp.first));
      } else {
        return escaped (str_from_names (termdefs, model->indexer ()->is_single ()) + field_sep + devices_string (m_dp, model->indexer ()->is_single (), true /*with parameters*/));
      }

    } else if (column == model->first_column () || column == model->second_column ()) {
      return make_link_to (m_dp, column);
    }

    return QString ();
  }

  virtual QString search_text ()
  {
    std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (dp ());
    std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> termdefs = terminal_defs_from_terminal_refs (tp ());
    return tl::to_qstring (combine_search_strings (combine_search_strings (search_string_from_names (termdefs), search_string_from_names (device_classes)), search_string_from_expanded_names (dp ())));
  }

  virtual std::string tooltip (NetlistBrowserModel *model)
  {
    return model->indexer ()->device_status_hint (cp (), model->indexer ()->device_index (dp ()));
  }

  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model)
  {
    return model->indexer ()->device_from_index (cp (), model->indexer ()->device_index (dp ())).second;
  }

  const IndexedNetlistModel::circuit_pair &cp ()
  {
    CircuitNetItemData *p = static_cast<CircuitNetItemData *> (parent ());
    return p->cp ();
  }

  const IndexedNetlistModel::net_pair &np ()
  {
    CircuitNetItemData *p = static_cast<CircuitNetItemData *> (parent ());
    return p->np ();
  }

  IndexedNetlistModel::device_pair dp ()
  {
    return devices_from_termrefs (tp ());
  }

  const IndexedNetlistModel::net_terminal_pair &tp ()
  {
    return m_tp;
  }

private:
  IndexedNetlistModel::device_pair m_dp;
  IndexedNetlistModel::net_terminal_pair m_tp;
};

// ----------------------------------------------------------------------------------

class CircuitNetDeviceTerminalOthersItemData
  : public NetlistModelItemData
{
private:
  CircuitNetDeviceTerminalOthersItemData (CircuitNetItemData *parent, const IndexedNetlistModel::net_pair &np, const std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> &tp)
    : NetlistModelItemData (parent), m_np (np), m_tp (tp)
  { }

  virtual void do_ensure_children (NetlistBrowserModel *)
  {
    //  nothing (leaf node)
  }

  virtual QIcon icon ()
  {
    return icon_for_connection (nets_from_device_terminals (dp (), m_tp));
  }

  virtual QString text (int column, NetlistBrowserModel *model)
  {
    //  circuit/net/device terminal/more: header column = terminal name, second column = net link
    if (column == model->object_column ()) {

      return escaped (str_from_names (m_tp, model->indexer ()->is_single ()));

    } else if (column == model->first_column () || column == model->second_column ()) {

      return make_link_to (m_np, column);

    }

    return QString ();
  }

  virtual QString search_text ()
  {
    std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> termdefs = terminal_defs_from_terminal_refs (m_tp);
    IndexedNetlistModel::net_pair nets = nets_from_device_terminals (dp (), termdefs);
    return tl::to_qstring (combine_search_strings (search_string_from_names (termdefs), search_string_from_expanded_names (nets)));
  }

  virtual std::string tooltip (NetlistBrowserModel *model)
  {
    return std::string ();
  }

  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel * /*model*/)
  {
    if (! is_valid_net_pair (nets_from_device_terminals (dp (), m_tp))) {
      //  This indicates a wrong connection: the nets are associated in a way which is a not
      //  corresponding to a mapped net pair. Report Mismatch here.
      return db::NetlistCrossReference::NoMatch;
    }

    return db::NetlistCrossReference::None;
  }

  const IndexedNetlistModel::circuit_pair &cp ()
  {
    CircuitNetDeviceTerminalItemData *p = static_cast<CircuitNetDeviceTerminalItemData *> (parent ());
    return p->cp ();
  }

  const IndexedNetlistModel::device_pair &dp ()
  {
    CircuitNetDeviceTerminalItemData *p = static_cast<CircuitNetDeviceTerminalItemData *> (parent ());
    return p->dp ();
  }

private:
  std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> m_tp;
  IndexedNetlistModel::net_pair m_np;
};

// ----------------------------------------------------------------------------------

class CircuitNetSubCircuitPinItemData
  : public NetlistModelItemData
{
private:
  CircuitNetSubCircuitPinItemData (CircuitNetItemData *parent, const IndexedNetlistModel::net_subcircuit_pin_pair &pp)
    : NetlistModelItemData (parent), m_pp (pp)
  { }

  virtual void do_ensure_children (NetlistBrowserModel *model)
  {
    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_pinrefs (m_pp);
    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (subcircuits);

    size_t n = model->indexer ()->pin_count (circuit_refs);
    for (size_t i = 0; i < n; ++i) {
      IndexedNetlistModel::pin_pair pp = model->indexer ()->pin_from_index (circuit_refs, i);
      push_back (new CircuitNetSubCircuitPinOthersItemData (this, pp));
    }
  }

  virtual QIcon icon ()
  {
    return icon_for_pin ();
  }

  virtual QString text (int column, NetlistBrowserModel *model)
  {
    //  circuit/net/pin: header column = pin name, second column empty (for now)
    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_pinrefs (m_pp);
    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (subcircuits);

    if (column == model->object_column ()) {
      return make_link_to (pins_from_pinrefs (m_pp), circuit_refs) + tl::to_qstring (field_sep) + make_link_to (circuit_refs);
    } else if (column == model->first_column () || column == model->second_column ()) {
      return make_link_to (subcircuits, column);
    }

    return QString ();
  }

  virtual QString search_text ()
  {
    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_pinrefs (m_sp);
    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (subcircuits);
    return tl::to_qstring (combine_search_strings (combine_search_strings (search_string_from_names (m_pp), search_string_from_names (circuit_refs)), search_string_from_expanded_names (subcircuits)));
  }

  virtual std::string tooltip (NetlistBrowserModel *model)
  {
    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_pinrefs (pp ());
    return model->indexer ()->subcircuit_status_hint (cp (), model->indexer ()->subcircuit_index (subcircuits));
  }

  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model)
  {
    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_pinrefs (pp ());
    return model->indexer ()->subcircuit_from_index (cp (), model->indexer ()->subcircuit_index (subcircuits)).second;
  }

  const IndexedNetlistModel::circuit_pair &cp ()
  {
    CircuitNetItemData *p = static_cast<CircuitNetItemData *> (parent ());
    return p->cp ();
  }

  const IndexedNetlistModel::net_pair &np ()
  {
    CircuitNetItemData *p = static_cast<CircuitNetItemData *> (parent ());
    return p->np ();
  }

  const IndexedNetlistModel::net_subcircuit_pin_pair &pp ()
  {
    return m_pp;
  }

private:
  IndexedNetlistModel::net_subcircuit_pin_pair m_sp;
  IndexedNetlistModel::pin_pair m_pp;
};

// ----------------------------------------------------------------------------------

class CircuitNetSubCircuitPinOthersItemData
  : public NetlistModelItemData
{
private:
  CircuitNetSubCircuitPinOthersItemData (CircuitNetItemData *parent, const IndexedNetlistModel::pin_pair &pp)
    : NetlistModelItemData (parent), m_pp (pp)
  { }

  virtual void do_ensure_children (NetlistBrowserModel *)
  {
    //  nothing (leaf node)
  }

  virtual QIcon icon ()
  {
    CircuitNetSubCircuitPinItemData *p = static_cast<CircuitNetSubCircuitPinItemData *> (parent ());
    return icon_for_connection (nets_from_subcircuit_pins (subcircuits_from_pinrefs (p->pp ()), m_pp));
  }

  virtual QString text (int column, NetlistBrowserModel *model)
  {
    //  circuit/net/device terminal/more: header column = pin name, second column = net link
    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_pinrefs (pp ());
    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (subcircuits);

    if (column == model->object_column ()) {
      return make_link_to (m_pp, circuit_refs);
    } else if (column == model->first_column () || column == model->second_column ()) {
      return make_link_to (nets_from_subcircuit_pins (subcircuits, m_pp), column);
    }

    return QString ();
  }

  virtual QString search_text ()
  {
    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_pinrefs (pp ());
    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (subcircuits);
    IndexedNetlistModel::net_pair nets = nets_from_circuit_pins (circuit_refs, m_pp);
    return tl::to_qstring (combine_search_strings (search_string_from_names (m_pp), search_string_from_expanded_names (nets)));
  }

  virtual std::string tooltip (NetlistBrowserModel * /*model*/)
  {
    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_pinrefs (pp ());
    std::string hint;

    if (! is_valid_net_pair (nets_from_subcircuit_pins (subcircuits, m_pp))) {
      //  This indicates a wrong connection: the nets are associated in a way which is a not
      //  corresponding to a mapped net pair. Report Mismatch here.
      hint = rewire_subcircuit_pins_status_hint ();
    }

    return hint;
  }

  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel * /*model*/)
  {
    IndexedNetlistModel::subcircuit_pair subcircuits = subcircuits_from_pinrefs (pp ());
    if (! is_valid_net_pair (nets_from_subcircuit_pins (subcircuits, m_pp))) {
      //  This indicates a wrong connection: the nets are associated in a way which is a not
      //  corresponding to a mapped net pair. Report Mismatch here.
      return db::NetlistCrossReference::NoMatch;
    }

    return db::NetlistCrossReference::None;
  }

  const IndexedNetlistModel::circuit_pair &cp ()
  {
    CircuitNetSubCircuitPinItemData *p = static_cast<CircuitNetSubCircuitPinItemData *> (parent ());
    return p->cp ();
  }

  const IndexedNetlistModel::device_pair &dp ()
  {
    CircuitNetSubCircuitPinItemData *p = static_cast<CircuitNetSubCircuitPinItemData *> (parent ());
    return p->dp ();
  }

  const IndexedNetlistModel::net_subcircuit_pin_pair &pp ()
  {
    CircuitNetSubCircuitPinItemData *p = static_cast<CircuitNetSubCircuitPinItemData *> (parent ());
    return p->pp ();
  }

private:
  IndexedNetlistModel::pin_pair m_pp;
};

// ----------------------------------------------------------------------------------

class CircuitNetPinItemData
  : public NetlistModelItemData
{
private:
  CircuitNetPinItemData (CircuitNetItemData *parent, const IndexedNetlistModel::net_pin_pair &pp)
    : NetlistModelItemData (parent), m_pp (pp)
  { }

  virtual void do_ensure_children (NetlistBrowserModel *)
  {
    //  nothing (leaf node)
  }

  virtual QIcon icon ()
  {
    return icon_for_pin ();
  }

  virtual QString text (int column, NetlistBrowserModel *model)
  {
    //  circuit/net/pin: header column = pin name, second column empty (for now)
    IndexedNetlistModel::circuit_pair circuits (m_pp.first && m_pp.first->net () ? m_pp.first->net ()->circuit () : 0, m_pp.second && m_pp.second->net () ? m_pp.second->net ()->circuit () : 0);
    if (model->indexer ()->is_single () && column == model->object_column ()) {
      return make_link_to (pins_from_pinrefs (m_pp), circuits);
    } else if (! model->indexer ()->is_single () && (column == model->first_column () || column == model->second_column ())) {
      return make_link_to (pins_from_pinrefs (m_pp), circuits, column);
    }

    return QString ();
  }

  virtual QString search_text ()
  {
    return tl::to_qstring (search_string_from_names (m_pp));
  }

  virtual std::string tooltip (NetlistBrowserModel *model)
  {
    return std::string ();
  }

  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model)
  {
    return db::NetlistCrossReference::None;
  }

private:
  IndexedNetlistModel::net_pin_pair m_pp;
};

// ----------------------------------------------------------------------------------

class CircuitSubCircuitItemData
  : public NetlistModelItemData
{
private:
  CircuitSubCircuitItemData (CircuitItemData *parent, const IndexedNetlistModel::subcircuit_pair &sp)
    : NetlistModelItemData (parent), m_sp (sp)
  { }

  virtual void do_ensure_children (NetlistBrowserModel *model)
  {
    size_t n = std::max (rows_for (sp ().first), rows_for (sp ().second));
    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (sp ());
    for (size_t i = 0; i < n; ++i) {
      IndexedNetlistModel::pin_pair pp = model->indexer ()->pin_from_index (circuit_refs, i).first;
      push_back (new CircuitSubCircuitPinItemData (this, pp));
    }
  }

  virtual QIcon icon ()
  {
    return icon_for_circuit ();
  }

  virtual QString text (int column, NetlistBrowserModel *model)
  {
    //  circuit/subcircuit: header column = circuit name, second column subcircuit name
    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (m_sp);
    if (column == model->object_column ()) {
      return make_link_to (circuit_refs);
    } else if (column == model->first_column ()) {
      return escaped (str_from_expanded_name (m_sp.first));
    } else if (column == model->second_column ()) {
      return escaped (str_from_expanded_name (subcircuits.second));
    }

    return QString ();
  }

  virtual QString search_text ()
  {
    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (sp ());
    return tl::to_qstring (combine_search_strings (search_string_from_names (circuit_refs), search_string_from_expanded_names (sp ())));
  }

  virtual std::string tooltip (NetlistBrowserModel *model)
  {
    size_t index = model->indexer ()->subcircuit_index (sp ());
    return model->indexer ()->subcircuit_status_hint (cp (), index);
  }

  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model)
  {
    size_t index = model->indexer ()->subcircuit_index (sp ());
    return model->indexer ()->subcircuit_from_index (cp (), index).second;
  }

  const IndexedNetlistModel::circuit_pair &cp ()
  {
    CircuitItemData *p = static_cast<CircuitItemData *> (parent ());
    return p->cp ();
  }

  const IndexedNetlistModel::subcircuit_pair &sp ()
  {
    return m_sp;
  }

private:
  IndexedNetlistModel::subcircuit_pair m_sp;
};

// ----------------------------------------------------------------------------------

class CircuitSubCircuitPinItemData
  : public NetlistModelItemData
{
private:
  CircuitSubCircuitPinItemData (CircuitItemData *parent, const IndexedNetlistModel::pin_pair &pp)
    : NetlistModelItemData (parent), m_pp (pp)
  { }

  virtual void do_ensure_children (NetlistBrowserModel *)
  {
    //  nothing (leaf node)
  }

  virtual QIcon icon ()
  {
    return icon_for_pin ();
  }

  virtual QString text (int column, NetlistBrowserModel *model)
  {
    //  circuit/pin: header column = pin name, other columns net name
    const IndexedNetlistModel::subcircuit_pair &sp = static_cast<CircuitSubCircuitItemData *> (parent ())->sp ();

    if (column == model->object_column ()) {
      return make_link_to (m_pp, circuit_refs_from_subcircuits (sp));
    } else if (column == model->first_column () || column == model->second_column ()) {
      return make_link_to (nets_from_subcircuit_pins (sp, m_pp), column);
    }

    return QString ();
  }

  virtual QString search_text ()
  {
    IndexedNetlistModel::net_pair nets = nets_from_subcircuit_pins (sp (), m_pp);
    return tl::to_qstring (combine_search_strings (search_string_from_names (m_pp), search_string_from_expanded_names (nets)));
  }

  virtual std::string tooltip (NetlistBrowserModel *model)
  {
    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (sp ());

    std::string hint = model->indexer ()->pin_status_hint (circuit_refs, model->indexer ()->pin_index (m_pp, circuit_refs));
    if (hint.empty ()) {

      //  Another test here is to check whether the pins may be attached to an invalid net pair
      if (! is_valid_net_pair (nets_from_subcircuit_pins (sp (), m_pp))) {
        hint = rewire_subcircuit_pins_status_hint ();
      }

    }

    return hint;
  }

  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model)
  {
    IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (sp ());

    db::NetlistCrossReference::Status status = model->indexer ()->pin_from_index (circuit_refs, model->indexer ()->pin_index (m_pp, circuit_refs)).second;
    if (status == db::NetlistCrossReference::Mismatch || status == db::NetlistCrossReference::NoMatch) {
      return status;
    }

    //  Another test here is to check whether the pins may be attached to an invalid net pair
    if (! is_valid_net_pair (nets_from_subcircuit_pins (sp (), m_pp))) {
      //  This indicates a wrong connection: the nets are associated in a way which is a not
      //  corresponding to a mapped net pair. Report Mismatch here.
      return db::NetlistCrossReference::NoMatch;
    }

    return db::NetlistCrossReference::None;
  }

  const IndexedNetlistModel::circuit_pair &cp ()
  {
    CircuitSubCircuitItemData *p = static_cast<CircuitSubCircuitItemData *> (parent ());
    return p->cp ();
  }

  const IndexedNetlistModel::subcircuit_pair &sp ()
  {
    CircuitSubCircuitItemData *p = static_cast<CircuitSubCircuitItemData *> (parent ());
    return p->sp ();
  }

private:
  IndexedNetlistModel::pin_pair m_pp;
};

// ----------------------------------------------------------------------------------

class CircuitDeviceItemData
  : public NetlistModelItemData
{
private:
  CircuitDeviceItemData (CircuitItemData *parent, const IndexedNetlistModel::device_pair &dp)
    : NetlistModelItemData (parent), m_dp (dp)
  { }

  virtual void do_ensure_children (NetlistBrowserModel * /*model*/)
  {
    size_t n = std::max (rows_for (dp ().first), rows_for (dp ().second));
    for (size_t i = 0; i < n; ++i) {
      std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> tp = terminal_defs_from_device_classes (dp (), i);
      push_back (new CircuitDeviceTerminalItemData (this, tp));
    }
  }

  virtual QIcon icon ()
  {
    std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (m_dp);
    return icon_for_devices (device_classes);
  }

  virtual QString text (int column, NetlistBrowserModel *model)
  {
    //  circuit/device: header column = class + parameters, second column device name
    if (model->indexer ()->is_single ()) {

      if (column == model->object_column ()) {
        return escaped (device_string (m_dp.first));
      } else if (column == model->first_column ()) {
        return escaped (str_from_expanded_name (m_dp.first));
      }

    } else {

      if (column == model->object_column ()) {
        return escaped (devices_string (m_dp, model->indexer ()->is_single (), false /*without parameters*/));
      } else if (column == model->first_column ()) {
        return escaped (str_from_expanded_name (m_dp.first) + field_sep + device_string (m_dp.first));
      } else if (column == model->second_column ()) {
        return escaped (str_from_expanded_name (m_dp.second) + field_sep + device_string (m_dp.second));
      }

    }
  }

  virtual QString search_text ()
  {
    std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (dp ());
    return tl::to_qstring (combine_search_strings (search_string_from_expanded_names (dp ()), search_string_from_names (device_classes)));
  }

  virtual std::string tooltip (NetlistBrowserModel *model)
  {
    size_t index = model->indexer ()->device_index (m_dp);
    hint = model->indexer ()->device_status_hint (cp (), index);
  }

  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model)
  {
    size_t index = model->indexer ()->device_index (m_dp);
    return model->indexer ()->device_from_index (cp (), index).second;
  }

  const IndexedNetlistModel::device_pair &dp ()
  {
    return m_dp;
  }

  const IndexedNetlistModel::circuit_pair &cp ()
  {
    CircuitItemData *p = static_cast<CircuitItemData *> (parent ());
    return p->cp ();
  }

private:
  IndexedNetlistModel::device_pair m_dp;
};

// ----------------------------------------------------------------------------------

class CircuitDeviceTerminalItemData
  : public NetlistModelItemData
{
private:
  CircuitDeviceTerminalItemData (CircuitDeviceItemData *parent, const std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> &tp)
    : NetlistModelItemData (parent), m_tp (tp)
  { }

  virtual void do_ensure_children (NetlistBrowserModel *)
  {
    //  nothing (leaf node)
  }

  virtual QIcon icon ()
  {
    const IndexedNetlistModel::device_pair &dp = static_cast<CircuitDeviceItemData *> (parent ())->dp ();
    std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> termdefs (m_tp.first ? m_tp.first->terminal_def () : 0, m_tp.second ? m_tp.second->terminal_def () : 0);
    return icon_for_connection (nets_from_device_terminals (dp, termdefs));
  }

  virtual QString text (int column, NetlistBrowserModel *model)
  {
    //  circuit/device/terminal: header column = terminal name, second column link to net
    std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> termdefs = terminal_defs_from_terminal_refs (m_tp);;

    if (column == model->object_column ()) {

      return escaped (str_from_names (termdefs, model->indexer ()->is_single ()));

    } else if (column == model->first_column () || column == model->second_column ()) {

      IndexedNetlistModel::net_pair nets = nets_from_device_terminals (dp (), termdefs);
      return make_link_to (nets, column);

    }

    return QString ();
  }

  virtual QString search_text ()
  {
    IndexedNetlistModel::net_pair nets = nets_from_device_terminals (dp (), m_tp);
    return tl::to_qstring (combine_search_strings (search_string_from_names (m_tp), search_string_from_expanded_names (nets)));
  }

  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel * /*model*/)
  {
    if (! is_valid_net_pair (nets_from_device_terminals (dp (), m_tp))) {
      //  This indicates a wrong connection: the nets are associated in a way which is a not
      //  corresponding to a mapped net pair. Report Mismatch here.
      return db::NetlistCrossReference::NoMatch;
    }

    return db::NetlistCrossReference::None;
  }

  const IndexedNetlistModel::device_pair &dp ()
  {
    CircuitDeviceItemData *p = static_cast<CircuitDeviceItemData *> (parent ());
    return p->dp ();
  }

  const IndexedNetlistModel::circuit_pair &cp ()
  {
    CircuitDeviceItemData *p = static_cast<CircuitDeviceItemData *> (parent ());
    return p->cp ();
  }

private:
  std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> m_tp;
};

// ----------------------------------------------------------------------------------
//  NetlistBrowserModel implementation

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
  NetlistModelItemData *d = (NetlistModelItemData *) (index.internalPointer ());
  if (! d) {
    return db::NetlistCrossReference::None;
  } else {
    return d->status (this);
  }
}

static std::string rewire_subcircuit_pins_status_hint ()
{
  return tl::to_string (tr ("Either pins or nets don't form a good pair.\nThis means either pin swapping happens (in this case, the nets will still match)\nor the subcircuit wiring is not correct (you'll see an error on the net)."));
}

QVariant
NetlistBrowserModel::tooltip (const QModelIndex &index) const
{
  std::string hint;
  NetlistModelItemData *d = (NetlistModelItemData *) (index.internalPointer ());
  if (d) {
    hint = d->tooltip (this);
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
  NetlistModelItemData *d = (NetlistModelItemData *) (index.internalPointer ());
  if (d) {
    return d->search_text ();
  } else {
    return QString ();
  }
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
  NetlistModelItemData *d = (NetlistModelItemData *) (index.internalPointer ());
  if (! d) {
    return QIcon ();
  } else {
    return d->icon ();
  }
}

Qt::ItemFlags
NetlistBrowserModel::flags (const QModelIndex & /*index*/) const
{
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

bool
NetlistBrowserModel::hasChildren (const QModelIndex &parent) const
{
  NetlistModelItemData *d = (NetlistModelItemData *) (index.internalPointer ());
  if (! d) {
    return false;
  } else {
    d->ensure_children (this);
    return d->begin () != d->end ();
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
  NetlistModelItemData *d = (NetlistModelItemData *) (index.internalPointer ());
  if (! d) {
    return QModelIndex ();
  } else {
    d->ensure_children (this);
    return createIndex (row, column, (void *) d->child (size_t (row)));
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
  NetlistModelItemData *d = (NetlistModelItemData *) (index.internalPointer ());
  if (! d || ! d->parent ()) {
    return QModelIndex ();
  } else {
    return createIndex (d->parent ()->index (), column, (void *) d->parent ());
  }
}

int
NetlistBrowserModel::rowCount (const QModelIndex &parent) const
{
  NetlistModelItemData *d = (NetlistModelItemData *) (parent.internalPointer ());
  if (! d) {
    return 0;
  } else {
    return int (d->child_count ());
  }
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

