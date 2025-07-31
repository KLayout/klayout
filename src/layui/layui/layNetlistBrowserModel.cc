
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include "layNetlistBrowserModel.h"
#include "layIndexedNetlistModel.h"
#include "layNetlistCrossReferenceModel.h"
#include "dbNetlistDeviceClasses.h"
#include "tlMath.h"

#include <QPainter>
#include <QIcon>
#include <QWidget>
#include <QTreeView>
#include <QUrl>
#if QT_VERSION >= 0x050000
#  include <QUrlQuery>
#endif

namespace lay
{

// ----------------------------------------------------------------------------------
//  NetlistObjectPath implementation

NetlistObjectsPath
NetlistObjectsPath::from_first (const NetlistObjectPath &p)
{
  NetlistObjectsPath pp;
  pp.root.first = p.root;
  for (NetlistObjectPath::path_iterator i = p.path.begin (); i != p.path.end (); ++i) {
    pp.path.push_back (std::make_pair (*i, (const db::SubCircuit *) 0));
  }
  pp.device.first = p.device;
  pp.net.first = p.net;
  return pp;
}

NetlistObjectsPath
NetlistObjectsPath::from_second (const NetlistObjectPath &p)
{
  NetlistObjectsPath pp;
  pp.root.second = p.root;
  for (NetlistObjectPath::path_iterator i = p.path.begin (); i != p.path.end (); ++i) {
    pp.path.push_back (std::make_pair ((const db::SubCircuit *) 0, *i));
  }
  pp.device.second = p.device;
  pp.net.second = p.net;
  return pp;
}

NetlistObjectPath
NetlistObjectsPath::first () const
{
  NetlistObjectPath p;
  p.root = root.first;
  for (NetlistObjectsPath::path_iterator i = path.begin (); i != path.end (); ++i) {
    if (! i->first) {
      return NetlistObjectPath ();
    }
    p.path.push_back (i->first);
  }
  p.device = device.first;
  p.net = net.first;
  return p;
}

NetlistObjectPath
NetlistObjectsPath::second () const
{
  NetlistObjectPath p;
  p.root = root.second;
  for (NetlistObjectsPath::path_iterator i = path.begin (); i != path.end (); ++i) {
    if (! i->second) {
      return NetlistObjectPath ();
    }
    p.path.push_back (i->second);
  }
  p.device = device.second;
  p.net = net.second;
  return p;
}

static bool
translate_circuit (const db::Circuit *&a, const db::NetlistCrossReference &xref)
{
  if (a) {
    a = xref.other_circuit_for (a);
    if (! a) {
      return false;
    }
  }
  return true;
}

static bool
translate_subcircuit (const db::SubCircuit *&a, const db::NetlistCrossReference &xref)
{
  if (a) {
    a = xref.other_subcircuit_for (a);
    if (! a) {
      return false;
    }
  }
  return true;
}

static bool
translate_device (const db::Device *&a, const db::NetlistCrossReference &xref)
{
  if (a) {
    a = xref.other_device_for (a);
    if (! a) {
      return false;
    }
  }
  return true;
}

static bool
translate_net (const db::Net *&a, const db::NetlistCrossReference &xref)
{
  if (a) {
    a = xref.other_net_for (a);
    if (! a) {
      return false;
    }
  }
  return true;
}

bool
NetlistObjectsPath::translate (NetlistObjectsPath &p, const db::NetlistCrossReference &xref)
{
  if (! translate_circuit (p.root.first, xref) || ! translate_circuit (p.root.second, xref)) {
    return false;
  }
  for (NetlistObjectsPath::path_type::iterator i = p.path.begin (); i != p.path.end (); ++i) {
    if (! translate_subcircuit (i->first, xref) || ! translate_subcircuit (i->second, xref)) {
      return false;
    }
  }
  if (! translate_device (p.device.first, xref) || ! translate_device (p.device.second, xref)) {
    return false;
  }
  if (! translate_net (p.net.first, xref) || ! translate_net (p.net.second, xref)) {
    return false;
  }
  return true;
}

// ----------------------------------------------------------------------------------
//  Implementation of the item classes

const std::string field_sep (" / ");

static QString escaped (const std::string &s)
{
  return tl::to_qstring (tl::escaped_to_html (s));
}

template <class Obj>
static std::string str_from_expanded_name (const Obj *obj, bool indicate_empty = false)
{
  if (obj) {
    return obj->expanded_name ();
  } else if (indicate_empty) {
    return std::string ("-");
  } else {
    return std::string ();
  }
}

template <class Obj>
static std::string str_from_name (const Obj *obj, bool indicate_empty = false)
{
  if (obj) {
    return obj->name ();
  } else if (indicate_empty) {
    return std::string ("-");
  } else {
    return std::string ();
  }
}

const std::string var_sep (" \u21D4 ");

template <class Obj>
static std::string str_from_expanded_names (const std::pair<const Obj *, const Obj *> &objs, bool is_single)
{
  std::string s = str_from_expanded_name (objs.first, ! is_single);
  if (! is_single) {
    std::string t = str_from_expanded_name (objs.second, ! is_single);
    if (t != s || ! objs.first || ! objs.second) {
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
    if (t != s || ! objs.first || ! objs.second) {
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
  if (va < 1e-20) {
    return "0";
  } else if (va < 100e-15) {
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
  std::string term;

  const std::vector<db::DeviceParameterDefinition> &pd = device->device_class ()->parameter_definitions ();

  for (std::vector<db::DeviceParameterDefinition>::const_iterator p = pd.begin (); p != pd.end (); ++p) {
    if (p->is_primary ()) {
      double v = device->parameter_value (p->id ());
      if (first) {
        s += " [";
      } else {
        s += ", ";
      }
      s += p->name ();
      s += "=";
      s += formatted_value (v);
      term = "]";
      first = false;
    }
  }

  bool first_sec = true;

  for (std::vector<db::DeviceParameterDefinition>::const_iterator p = pd.begin (); p != pd.end (); ++p) {
    double v = device->parameter_value (p->id ());
    std::string vs = formatted_value (v);
    std::string vs_def = formatted_value (p->default_value ());
    if (! p->is_primary () && vs != vs_def) {
      if (first) {
        s += " [(";
      } else if (first_sec) {
        s += ", (";
      } else {
        s += ", ";
      }
      s += p->name ();
      s += "=";
      s += vs;
      term = ")]";
      first = false;
      first_sec = false;
    }
  }

  return s + term;
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
IndexedNetlistModel::net_pair nets_from_pinrefs (const IndexedNetlistModel::net_subcircuit_pin_pair &pinrefs)
{
  const db::Net *net1 = 0, *net2 = 0;
  if (pinrefs.first) {
    net1 = pinrefs.first->net ();
  }
  if (pinrefs.second) {
    net2 = pinrefs.second->net ();
  }

  return std::make_pair (net1, net2);
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
IndexedNetlistModel::pin_pair pins_from_netrefs (const IndexedNetlistModel::net_subcircuit_pin_pair &netrefs)
{
  const db::Pin *pin1 = 0, *pin2 = 0;
  if (netrefs.first) {
    pin1 = netrefs.first->pin ();
  }
  if (netrefs.second) {
    pin2 = netrefs.second->pin ();
  }

  return std::make_pair (pin1, pin2);
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

static std::vector<std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> > terminal_defs_from_device_classes (IndexedNetlistModel *model, const std::pair<const db::DeviceClass *, const db::DeviceClass *> &device_classes, const std::pair<const db::Device *, const db::Device *> &devices)
{
  std::vector<std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> > result;

  std::map<size_t, std::pair<std::vector<std::pair<const db::DeviceTerminalDefinition *, const db::Net *> >, std::vector<std::pair<const db::DeviceTerminalDefinition *, const db::Net *> > > > nets;

  size_t n1 = 0;
  if (device_classes.first) {
    n1 = device_classes.first->terminal_definitions ().size ();
  }

  size_t n2 = 0;
  if (device_classes.second) {
    n2 = device_classes.second->terminal_definitions ().size ();
  }

  for (size_t i = 0; i < n1 || i < n2; ++i) {

    if (i < n2) {
      const db::DeviceTerminalDefinition &td = device_classes.second->terminal_definitions () [i];
      size_t id = td.id ();
      size_t id_norm = device_classes.second->normalize_terminal_id (id);
      nets [id_norm].second.push_back (std::make_pair (&td, devices.second->net_for_terminal (id)));
    }

    if (i < n1) {
      const db::DeviceTerminalDefinition &td = device_classes.first->terminal_definitions () [i];
      size_t id = td.id ();
      size_t id_norm = device_classes.first->normalize_terminal_id (id);
      nets [id_norm].first.push_back (std::make_pair (&td, devices.first->net_for_terminal (id)));
    }

  }

  for (std::map<size_t, std::pair<std::vector<std::pair<const db::DeviceTerminalDefinition *, const db::Net *> >, std::vector<std::pair<const db::DeviceTerminalDefinition *, const db::Net *> > > >::iterator n = nets.begin (); n != nets.end (); ++n) {

    std::vector<std::pair<const db::DeviceTerminalDefinition *, const db::Net *> > &nn1 = n->second.first;
    std::vector<std::pair<const db::DeviceTerminalDefinition *, const db::Net *> > &nn2 = n->second.second;

    if (nn2.empty ()) {

      for (std::vector<std::pair<const db::DeviceTerminalDefinition *, const db::Net *> >::const_iterator i = nn1.begin (); i != nn1.end (); ++i) {
        result.push_back (std::make_pair (i->first, (const db::DeviceTerminalDefinition *) 0));
      }

    } else if (nn1.empty ()) {

      for (std::vector<std::pair<const db::DeviceTerminalDefinition *, const db::Net *> >::const_iterator j = nn2.begin (); j != nn2.end (); ++j) {
        result.push_back (std::make_pair ((const db::DeviceTerminalDefinition *) 0, j->first));
      }

    } else {

      std::vector<std::pair<const db::DeviceTerminalDefinition *, const db::Net *> >::iterator w = nn1.begin ();
      for (std::vector<std::pair<const db::DeviceTerminalDefinition *, const db::Net *> >::const_iterator i = nn1.begin (); i != nn1.end (); ++i) {

        bool found = false;

        for (std::vector<std::pair<const db::DeviceTerminalDefinition *, const db::Net *> >::iterator j = nn2.begin (); j != nn2.end () && ! found; ++j) {
          const db::Net *n2 = model->second_net_for (i->second);
          if (n2 == j->second) {
            result.push_back (std::make_pair (i->first, j->first));
            nn2.erase (j);
            found = true;
            break;
          }
        }

        if (! found) {
          *w++ = *i;
        }

      }

      nn1.erase (w, nn1.end ());

      for (size_t i = 0; i < nn1.size () && i < nn2.size (); ++i) {
        result.push_back (std::make_pair (nn1 [i].first, nn2 [i].first));
      }

    }

  }

  return result;
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
    return d->text (index.column (), const_cast<NetlistBrowserModel *> (this));
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

static QIcon icon_for_net ()
{
  static QIcon icon;
  if (icon.isNull ()) {
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_net_48.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_net_32.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_net_24.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_net_16.png")));
  }
  return icon;
}

static QIcon light_icon_for_net ()
{
  static QIcon icon;
  if (icon.isNull ()) {
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_net_light_48.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_net_light_32.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_net_light_24.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_net_light_16.png")));
  }
  return icon;
}

static QIcon icon_for_connection ()
{
  static QIcon icon;
  if (icon.isNull ()) {
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_conn_48.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_conn_32.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_conn_24.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_conn_16.png")));
  }
  return icon;
}

static QIcon light_icon_for_connection ()
{
  static QIcon icon;
  if (icon.isNull ()) {
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_conn_light_48.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_conn_light_32.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_conn_light_24.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_conn_light_16.png")));
  }
  return icon;
}

static QIcon icon_for_pin ()
{
  static QIcon icon;
  if (icon.isNull ()) {
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_pin_48.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_pin_32.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_pin_24.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_pin_16.png")));
  }
  return icon;
}

static QIcon icon_for_device (const db::DeviceClass *dc, size_t term_id = 0)
{
  static QIcon icon_for_res;
  static QIcon icon_for_ind;
  static QIcon icon_for_cap;
  static QIcon icons_for_diode[2];
  static QIcon icons_for_bjt[4];
  static QIcon icons_for_mos[4];

  if (icon_for_res.isNull ()) {
    icon_for_res.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_res_48.png")));
    icon_for_res.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_res_32.png")));
    icon_for_res.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_res_24.png")));
    icon_for_res.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_res_16.png")));
  }
  if (icon_for_ind.isNull ()) {
    //  fake ...
    icon_for_ind.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_res_48.png")));
    icon_for_ind.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_res_32.png")));
    icon_for_ind.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_res_24.png")));
    icon_for_ind.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_res_16.png")));
  }
  if (icon_for_cap.isNull ()) {
    icon_for_cap.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_cap_48.png")));
    icon_for_cap.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_cap_32.png")));
    icon_for_cap.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_cap_24.png")));
    icon_for_cap.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_device_cap_16.png")));
  }
  if (icons_for_diode[0].isNull ()) {
    QImage i48 (QString::fromUtf8 (":/images/icon_device_diode_48.png"));
    QImage i32 (QString::fromUtf8 (":/images/icon_device_diode_32.png"));
    QImage i24 (QString::fromUtf8 (":/images/icon_device_diode_24.png"));
    QImage i16 (QString::fromUtf8 (":/images/icon_device_diode_16.png"));
    QTransform tr;
    for (size_t i = 0; i < sizeof (icons_for_diode) / sizeof (icons_for_diode [0]); ++i) {
      icons_for_diode[i].addPixmap (QPixmap::fromImage (i48.transformed (tr)));
      icons_for_diode[i].addPixmap (QPixmap::fromImage (i32.transformed (tr)));
      icons_for_diode[i].addPixmap (QPixmap::fromImage (i24.transformed (tr)));
      icons_for_diode[i].addPixmap (QPixmap::fromImage (i16.transformed (tr)));
      tr.rotate (180.0);
    }
  }
  if (icons_for_bjt[0].isNull ()) {
    QImage i48 (QString::fromUtf8 (":/images/icon_device_bjt_48.png"));
    QImage i32 (QString::fromUtf8 (":/images/icon_device_bjt_32.png"));
    QImage i24 (QString::fromUtf8 (":/images/icon_device_bjt_24.png"));
    QImage i16 (QString::fromUtf8 (":/images/icon_device_bjt_16.png"));
    QTransform tr;
    for (size_t i = 0; i < sizeof (icons_for_bjt) / sizeof (icons_for_bjt [0]); ++i) {
      icons_for_bjt[i].addPixmap (QPixmap::fromImage (i48.transformed (tr)));
      icons_for_bjt[i].addPixmap (QPixmap::fromImage (i32.transformed (tr)));
      icons_for_bjt[i].addPixmap (QPixmap::fromImage (i24.transformed (tr)));
      icons_for_bjt[i].addPixmap (QPixmap::fromImage (i16.transformed (tr)));
      tr.rotate (90.0);
    }
  }
  if (icons_for_mos[0].isNull ()) {
    QImage i48 (QString::fromUtf8 (":/images/icon_device_mos_48.png"));
    QImage i32 (QString::fromUtf8 (":/images/icon_device_mos_32.png"));
    QImage i24 (QString::fromUtf8 (":/images/icon_device_mos_24.png"));
    QImage i16 (QString::fromUtf8 (":/images/icon_device_mos_16.png"));
    QTransform tr;
    for (size_t i = 0; i < sizeof (icons_for_mos) / sizeof (icons_for_mos [0]); ++i) {
      icons_for_mos[i].addPixmap (QPixmap::fromImage (i48.transformed (tr)));
      icons_for_mos[i].addPixmap (QPixmap::fromImage (i32.transformed (tr)));
      icons_for_mos[i].addPixmap (QPixmap::fromImage (i24.transformed (tr)));
      icons_for_mos[i].addPixmap (QPixmap::fromImage (i16.transformed (tr)));
      tr.rotate (90.0);
    }
  }

  //  TODO: generic device ...
  if (dynamic_cast<const db::DeviceClassResistor *> (dc)) {
    return icon_for_res;
  } else if (dynamic_cast<const db::DeviceClassInductor *> (dc)) {
    return icon_for_ind;
  } else if (dynamic_cast<const db::DeviceClassCapacitor *> (dc)) {
    return icon_for_cap;
  } else if (dynamic_cast<const db::DeviceClassDiode *> (dc)) {
    return icons_for_diode [term_id >= sizeof (icons_for_diode) / sizeof (icons_for_diode [0]) ? sizeof (icons_for_diode) / sizeof (icons_for_diode [0]) - 1 : term_id];
  } else if (dynamic_cast<const db::DeviceClassBJT3Transistor *> (dc) || dynamic_cast<const db::DeviceClassBJT4Transistor *> (dc)) {
    return icons_for_bjt [term_id >= sizeof (icons_for_bjt) / sizeof (icons_for_bjt [0]) ? sizeof (icons_for_bjt) / sizeof (icons_for_bjt [0]) - 1 : term_id];
  } else if (dynamic_cast<const db::DeviceClassMOS3Transistor *> (dc) || dynamic_cast<const db::DeviceClassMOS4Transistor *> (dc)) {
    return icons_for_mos [term_id >= sizeof (icons_for_mos) / sizeof (icons_for_mos [0]) ? sizeof (icons_for_mos) / sizeof (icons_for_mos [0]) - 1 : term_id];
  } else {
    return icons_for_mos [0];
  }
}

static QIcon icon_for_devices (const std::pair<const db::DeviceClass *, const db::DeviceClass *> &device_classes)
{
  return icon_for_device (device_classes.first ? device_classes.first : device_classes.second);
}

static QIcon icon_for_devices (const std::pair<const db::DeviceClass *, const db::DeviceClass *> &device_classes,
                               const std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> &terminal_defs)
{
  return icon_for_device (device_classes.first ? device_classes.first : device_classes.second, terminal_defs.first ? terminal_defs.first->id () : (terminal_defs.second ? terminal_defs.second->id () : 0));
}

static QIcon icon_for_circuit ()
{
  static QIcon icon;
  if (icon.isNull ()) {
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_circuit_48.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_circuit_32.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_circuit_24.png")));
    icon.addPixmap (QPixmap (QString::fromUtf8 (":/images/icon_circuit_16.png")));
  }
  return icon;
}

static QIcon icon_for_subcircuit ()
{
  static QIcon icon;
  if (icon.isNull ()) {
    QTransform tr;
    tr.rotate (90.0);
    icon.addPixmap (QPixmap::fromImage (QImage (QString::fromUtf8 (":/images/icon_circuit_48.png"))).transformed (tr));
    icon.addPixmap (QPixmap::fromImage (QImage (QString::fromUtf8 (":/images/icon_circuit_32.png"))).transformed (tr));
    icon.addPixmap (QPixmap::fromImage (QImage (QString::fromUtf8 (":/images/icon_circuit_24.png"))).transformed (tr));
    icon.addPixmap (QPixmap::fromImage (QImage (QString::fromUtf8 (":/images/icon_circuit_16.png"))).transformed (tr));
  }
  return icon;
}

static QIcon colored_icon (const tl::Color &color, const QIcon &original_icon)
{
  if (! color.is_valid ()) {
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

static QIcon net_icon_with_color (const tl::Color &color)
{
  return colored_icon (color, light_icon_for_net ());
}

static QIcon connection_icon_with_color (const tl::Color &color)
{
  return colored_icon (color, light_icon_for_connection ());
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

// ----------------------------------------------------------------------------------
//  item class declarations

class RootItemData
  : public NetlistModelItemData
{
public:
  RootItemData ();

  virtual void do_ensure_children (NetlistBrowserModel *model);
  virtual QIcon icon (NetlistBrowserModel *model);
  virtual QString text (int column, NetlistBrowserModel *model);
  virtual QString search_text ();
  virtual std::string tooltip (NetlistBrowserModel *model);
  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model);
  virtual bool has_children (NetlistBrowserModel *) { return true; }

  CircuitItemData *circuit_item (NetlistBrowserModel *model, const IndexedNetlistModel::circuit_pair &cp);
};

// ----------------------------------------------------------------------------------

class CircuitItemData
  : public NetlistModelItemData
{
public:
  CircuitItemData (NetlistModelItemData *parent, const IndexedNetlistModel::circuit_pair &cp);

  virtual void do_ensure_children (NetlistBrowserModel *model);
  virtual QIcon icon (NetlistBrowserModel *model);
  virtual QString text (int column, NetlistBrowserModel *model);
  virtual QString search_text ();
  virtual std::string tooltip (NetlistBrowserModel *model);
  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model);
  virtual bool has_children (NetlistBrowserModel *model);

  virtual std::pair<const db::Circuit *, const db::Circuit *> circuits_of_this ()
  {
    return m_cp;
  }

  CircuitNetItemData *circuit_net_item (NetlistBrowserModel *model, const IndexedNetlistModel::net_pair &np);
  CircuitDeviceItemData *circuit_device_item (NetlistBrowserModel *model, const IndexedNetlistModel::device_pair &dp);
  CircuitSubCircuitItemData *circuit_subcircuit_item (NetlistBrowserModel *model, const IndexedNetlistModel::subcircuit_pair &sp);

private:
  IndexedNetlistModel::circuit_pair m_cp;
};

// ----------------------------------------------------------------------------------

class CircuitItemForSubCircuitData
  : public CircuitItemData
{
public:
  CircuitItemForSubCircuitData (NetlistModelItemData *parent, const IndexedNetlistModel::subcircuit_pair &sp);

  virtual QString text (int column, NetlistBrowserModel *model);
  virtual QString search_text ();
  virtual std::string tooltip (NetlistBrowserModel *model);
  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model);

  virtual std::pair<const db::SubCircuit *, const db::SubCircuit *> subcircuits_of_this ()
  {
    return m_sp;
  }

private:
  IndexedNetlistModel::subcircuit_pair m_sp;
};

// ----------------------------------------------------------------------------------

class CircuitItemNodeData
  : public NetlistModelItemData
{
public:
  enum type { Nets, Devices, Pins, SubCircuits };

  CircuitItemNodeData (NetlistModelItemData *parent, type t);

  virtual void do_ensure_children (NetlistBrowserModel *model);
  virtual QIcon icon (NetlistBrowserModel *model);
  virtual QString text (int column, NetlistBrowserModel *model);
  virtual QString search_text ();
  virtual std::string tooltip (NetlistBrowserModel *model);
  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model);
  virtual bool has_children (NetlistBrowserModel *model);

  CircuitNetItemData *circuit_net_item (NetlistBrowserModel *model, const IndexedNetlistModel::net_pair &np);
  CircuitDeviceItemData *circuit_device_item (NetlistBrowserModel *model, const IndexedNetlistModel::device_pair &dp);
  CircuitSubCircuitItemData *circuit_subcircuit_item (NetlistBrowserModel *model, const IndexedNetlistModel::subcircuit_pair &sp);

private:
  type m_type;
};

// ----------------------------------------------------------------------------------

class CircuitNetItemData
  : public NetlistModelItemData
{
public:
  CircuitNetItemData (NetlistModelItemData *parent, const IndexedNetlistModel::net_pair &np);

  virtual void do_ensure_children (NetlistBrowserModel *model);
  virtual QIcon icon (NetlistBrowserModel *model);
  virtual QString text (int column, NetlistBrowserModel *model);
  virtual QString search_text ();
  virtual std::string tooltip (NetlistBrowserModel *model);
  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model);
  virtual bool has_children (NetlistBrowserModel *) { return true; }

  const IndexedNetlistModel::net_pair &np ()
  {
    return m_np;
  }

  virtual std::pair<const db::Net *, const db::Net *> nets_of_this ()
  {
    return m_np;
  }

  bool seen () const
  {
    return m_seen;
  }

private:
  IndexedNetlistModel::net_pair m_np;
  bool m_seen;
};

// ----------------------------------------------------------------------------------

class CircuitNetDeviceTerminalItemData
  : public NetlistModelItemData
{
public:
  CircuitNetDeviceTerminalItemData (NetlistModelItemData *parent, const IndexedNetlistModel::net_terminal_pair &tp);

  virtual void do_ensure_children (NetlistBrowserModel * /*model*/);
  virtual QIcon icon (NetlistBrowserModel *model);
  virtual QString text (int column, NetlistBrowserModel *model);
  virtual QString search_text ();
  virtual std::string tooltip (NetlistBrowserModel *model);
  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model);
  virtual bool has_children (NetlistBrowserModel *) { return true; }

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

  virtual std::pair<const db::Device *, const db::Device *> devices_of_this ()
  {
    return dp ();
  }

private:
  IndexedNetlistModel::net_terminal_pair m_tp;
  bool m_device_seen;
};

// ----------------------------------------------------------------------------------

class CircuitNetDeviceTerminalOthersItemData
  : public CircuitNetItemData
{
public:
  CircuitNetDeviceTerminalOthersItemData (NetlistModelItemData *parent, const IndexedNetlistModel::net_pair &np, const std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> &tp);

  virtual QIcon icon (NetlistBrowserModel *model);
  virtual QString text (int column, NetlistBrowserModel *model);
  virtual QString search_text ();

private:
  std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> m_tp;
  IndexedNetlistModel::net_pair m_np;
};

// ----------------------------------------------------------------------------------

class CircuitNetSubCircuitPinItemData
  : public NetlistModelItemData
{
public:
  CircuitNetSubCircuitPinItemData (NetlistModelItemData *parent, const IndexedNetlistModel::net_subcircuit_pin_pair &pp);

  virtual void do_ensure_children (NetlistBrowserModel *model);
  virtual QIcon icon (NetlistBrowserModel *model);
  virtual QString text (int column, NetlistBrowserModel *model);
  virtual QString search_text ();
  virtual std::string tooltip (NetlistBrowserModel *model);
  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model);
  virtual bool has_children (NetlistBrowserModel *) { return true; }

  const IndexedNetlistModel::net_pair &np ()
  {
    CircuitNetItemData *p = static_cast<CircuitNetItemData *> (parent ());
    return p->np ();
  }

  const IndexedNetlistModel::net_subcircuit_pin_pair &sp ()
  {
    return m_sp;
  }

  IndexedNetlistModel::pin_pair pp ()
  {
    return pins_from_pinrefs (m_sp);
  }

  virtual std::pair<const db::SubCircuit *, const db::SubCircuit *> subcircuits_of_this ()
  {
    return subcircuits_from_pinrefs (m_sp);
  }

  //  NOTE: this is important as this node acts as parent for nets inside this circuit
  virtual std::pair<const db::Circuit *, const db::Circuit *> circuits_of_this ()
  {
    return circuit_refs_from_subcircuits (subcircuits_of_this ());
  }

  virtual std::pair<const db::Pin *, const db::Pin *> pins_of_this ()
  {
    return m_pp;
  }

private:
  IndexedNetlistModel::net_subcircuit_pin_pair m_sp;
  IndexedNetlistModel::pin_pair m_pp;
  bool m_subcircuit_seen;
};

// ----------------------------------------------------------------------------------

class CircuitNetPinItemData
  : public NetlistModelItemData
{
public:
  CircuitNetPinItemData (NetlistModelItemData *parent, const IndexedNetlistModel::net_pin_pair &pp);

  virtual void do_ensure_children (NetlistBrowserModel *);
  virtual QIcon icon (NetlistBrowserModel *model);
  virtual QString text (int column, NetlistBrowserModel *model);
  virtual QString search_text ();
  virtual std::string tooltip (NetlistBrowserModel *model);
  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model);
  virtual bool has_children (NetlistBrowserModel *) { return false; }

  virtual std::pair<const db::Pin *, const db::Pin *> pins_of_this ()
  {
    return pins_from_pinrefs (m_pp);
  }

private:
  IndexedNetlistModel::net_pin_pair m_pp;
};

// ----------------------------------------------------------------------------------

class CircuitSubCircuitItemData
  : public NetlistModelItemData
{
public:
  CircuitSubCircuitItemData (NetlistModelItemData *parent, const IndexedNetlistModel::subcircuit_pair &sp);

  virtual void do_ensure_children (NetlistBrowserModel *model);
  virtual QIcon icon (NetlistBrowserModel *model);
  virtual QString text (int column, NetlistBrowserModel *model);
  virtual QString search_text ();
  virtual std::string tooltip (NetlistBrowserModel *model);
  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model);
  virtual bool has_children (NetlistBrowserModel *) { return true; }

  const IndexedNetlistModel::subcircuit_pair &sp ()
  {
    return m_sp;
  }

  CircuitItemForSubCircuitData *circuit_item ()
  {
    return mp_circuit_node;
  }

private:
  IndexedNetlistModel::subcircuit_pair m_sp;
  CircuitItemForSubCircuitData *mp_circuit_node;
};

// ----------------------------------------------------------------------------------

class CircuitPinItemData
  : public CircuitNetItemData
{
public:
  CircuitPinItemData (NetlistModelItemData *parent, const IndexedNetlistModel::pin_pair &pp);

  virtual QIcon icon (NetlistBrowserModel *model);
  virtual QString text (int column, NetlistBrowserModel *model);
  virtual QString search_text ();

  virtual std::pair<const db::Pin *, const db::Pin *> pins_of_this ()
  {
    return m_pp;
  }

private:
  IndexedNetlistModel::pin_pair m_pp;
};

// ----------------------------------------------------------------------------------

class CircuitSubCircuitPinsItemData
  : public NetlistModelItemData
{
public:
  CircuitSubCircuitPinsItemData (NetlistModelItemData *parent, const IndexedNetlistModel::subcircuit_pair &sp);

  virtual void do_ensure_children (NetlistBrowserModel *model);
  virtual QIcon icon (NetlistBrowserModel *model);
  virtual QString text (int column, NetlistBrowserModel *model);
  virtual QString search_text ();
  virtual std::string tooltip (NetlistBrowserModel *model);
  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model);
  virtual bool has_children (NetlistBrowserModel *model);

  const IndexedNetlistModel::subcircuit_pair &sp ()
  {
    return m_sp;
  }

private:
  IndexedNetlistModel::subcircuit_pair m_sp;
};

// ----------------------------------------------------------------------------------

class CircuitSubCircuitPinItemData
  : public CircuitNetItemData
{
public:
  CircuitSubCircuitPinItemData (NetlistModelItemData *parent, const IndexedNetlistModel::net_subcircuit_pin_pair &pp);

  virtual QIcon icon (NetlistBrowserModel *model);
  virtual QString text (int column, NetlistBrowserModel *model);
  virtual QString search_text ();

  IndexedNetlistModel::subcircuit_pair sp ()
  {
    return subcircuits_from_pinrefs (m_pp);
  }

  virtual std::pair<const db::Pin *, const db::Pin *> pins_of_this ()
  {
    return pins_from_netrefs (m_pp);
  }

private:
  IndexedNetlistModel::net_subcircuit_pin_pair m_pp;
};

// ----------------------------------------------------------------------------------

class CircuitDeviceItemData
  : public NetlistModelItemData
{
public:
  CircuitDeviceItemData (NetlistModelItemData *parent, const IndexedNetlistModel::device_pair &dp);

  virtual void do_ensure_children (NetlistBrowserModel * /*model*/);
  virtual QIcon icon (NetlistBrowserModel *model);
  virtual QString text (int column, NetlistBrowserModel *model);
  virtual QString search_text ();
  virtual std::string tooltip (NetlistBrowserModel *model);
  virtual db::NetlistCrossReference::Status status (NetlistBrowserModel *model);
  virtual bool has_children (NetlistBrowserModel *) { return true; }

  const IndexedNetlistModel::device_pair &dp ()
  {
    return m_dp;
  }

  virtual std::pair<const db::Device *, const db::Device *> devices_of_this ()
  {
    return m_dp;
  }

private:
  IndexedNetlistModel::device_pair m_dp;
};

// ----------------------------------------------------------------------------------

class CircuitDeviceTerminalItemData
  : public CircuitNetItemData
{
public:
  CircuitDeviceTerminalItemData (NetlistModelItemData *parent, const std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> &tp);

  virtual QIcon icon (NetlistBrowserModel *model);
  virtual QString text (int column, NetlistBrowserModel *model);
  virtual QString search_text ();

private:
  std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> m_tp;
};

// ----------------------------------------------------------------------------------
//  item class implementations

NetlistModelItemData::NetlistModelItemData ()
  : mp_parent (0), m_children_made (false), m_index (0)
{ }

NetlistModelItemData::NetlistModelItemData (NetlistModelItemData *parent)
  : mp_parent (parent), m_children_made (false), m_index (0)
{ }

NetlistModelItemData::~NetlistModelItemData ()
{ }

void
NetlistModelItemData::ensure_children (NetlistBrowserModel *model)
{
  if (! m_children_made) {

    m_children.clear ();
    m_children_per_index.clear ();

    do_ensure_children (model);

    size_t n = 0;
    for (iterator i = begin (); i != end (); ++i) {
      ++n;
    }
    m_children_per_index.reserve (n);

    size_t index = 0;
    for (iterator i = begin (); i != end (); ++i) {
      m_children_per_index.push_back (i.operator-> ());
      i->set_index (index++);
    }

    m_children_made = true;

  }
}

void
NetlistModelItemData::push_back (NetlistModelItemData *child)
{
  m_children.push_back (child);
}

NetlistModelItemData *
NetlistModelItemData::child (size_t n)
{
  return (n < m_children_per_index.size () ? m_children_per_index [n] : 0);
}

std::pair<const db::Circuit *, const db::Circuit *>
NetlistModelItemData::circuits_of_this ()
{
  return std::pair<const db::Circuit *, const db::Circuit *> ((const db::Circuit *) 0, (const db::Circuit *) 0);
}

std::pair<const db::Circuit *, const db::Circuit *>
NetlistModelItemData::circuits ()
{
  std::pair<const db::Circuit *, const db::Circuit *> r = circuits_of_this ();
  if (! mp_parent || r.first || r.second) {
    return r;
  } else {
    return mp_parent->circuits ();
  }
}

bool
NetlistModelItemData::derived_from_circuits (const std::pair<const db::Circuit *, const db::Circuit *> &cp)
{
  if (! cp.first && ! cp.second) {
    return false;
  } else if (circuits_of_this () == cp) {
    return true;
  } else if (mp_parent) {
    return mp_parent->derived_from_circuits (cp);
  } else {
    return false;
  }
}

std::pair<const db::Device *, const db::Device *>
NetlistModelItemData::devices_of_this ()
{
  return std::pair<const db::Device *, const db::Device *> ((const db::Device *) 0, (const db::Device *) 0);
}

std::pair<const db::Device *, const db::Device *>
NetlistModelItemData::devices ()
{
  std::pair<const db::Device *, const db::Device *> r = devices_of_this ();
  if (! mp_parent || r.first || r.second) {
    return r;
  } else {
    return mp_parent->devices ();
  }
}

bool
NetlistModelItemData::derived_from_devices (const std::pair<const db::Device *, const db::Device *> &sp)
{
  if (! sp.first && ! sp.second) {
    return false;
  } else if (devices_of_this () == sp) {
    return true;
  } else if (mp_parent) {
    return mp_parent->derived_from_devices (sp);
  } else {
    return false;
  }
}

std::pair<const db::Pin *, const db::Pin *>
NetlistModelItemData::pins_of_this ()
{
  return std::pair<const db::Pin *, const db::Pin *> ((const db::Pin *) 0, (const db::Pin *) 0);
}

std::pair<const db::Pin *, const db::Pin *>
NetlistModelItemData::pins ()
{
  std::pair<const db::Pin *, const db::Pin *> r = pins_of_this ();
  if (! mp_parent || r.first || r.second) {
    return r;
  } else {
    return mp_parent->pins ();
  }
}

bool
NetlistModelItemData::derived_from_pins (const std::pair<const db::Pin *, const db::Pin *> &sp)
{
  if (! sp.first && ! sp.second) {
    return false;
  } else if (pins_of_this () == sp) {
    return true;
  } else if (mp_parent) {
    return mp_parent->derived_from_pins (sp);
  } else {
    return false;
  }
}

std::pair<const db::SubCircuit *, const db::SubCircuit *>
NetlistModelItemData::subcircuits_of_this ()
{
  return std::pair<const db::SubCircuit *, const db::SubCircuit *> ((const db::SubCircuit *) 0, (const db::SubCircuit *) 0);
}

std::pair<const db::SubCircuit *, const db::SubCircuit *>
NetlistModelItemData::subcircuits ()
{
  std::pair<const db::SubCircuit *, const db::SubCircuit *> r = subcircuits_of_this ();
  if (! mp_parent || r.first || r.second) {
    return r;
  } else {
    return mp_parent->subcircuits ();
  }
}

bool
NetlistModelItemData::derived_from_subcircuits (const std::pair<const db::SubCircuit *, const db::SubCircuit *> &sp)
{
  if (! sp.first && ! sp.second) {
    return false;
  } else if (subcircuits_of_this () == sp) {
    return true;
  } else if (mp_parent) {
    return mp_parent->derived_from_subcircuits (sp);
  } else {
    return false;
  }
}

std::pair<const db::Net *, const db::Net *>
NetlistModelItemData::nets_of_this ()
{
  return std::pair<const db::Net *, const db::Net *> ((const db::Net *) 0, (const db::Net *) 0);
}

std::pair<const db::Net *, const db::Net *>
NetlistModelItemData::nets ()
{
  std::pair<const db::Net *, const db::Net *> r = nets_of_this ();
  if (! mp_parent || r.first || r.second) {
    return r;
  } else {
    return mp_parent->nets ();
  }
}

bool
NetlistModelItemData::derived_from_nets (const std::pair<const db::Net *, const db::Net *> &np)
{
  if (! np.first && ! np.second) {
    return false;
  } else if (nets_of_this () == np) {
    return true;
  } else if (mp_parent) {
    return mp_parent->derived_from_nets (np);
  } else {
    return false;
  }
}

// ----------------------------------------------------------------------------------

RootItemData::RootItemData ()
{
  //  .. nothing yet ..
}

void
RootItemData::do_ensure_children (NetlistBrowserModel *model)
{
  size_t n = model->indexer ()->circuit_count ();
  for (size_t i = 0; i < n; ++i) {
    push_back (new CircuitItemData (0 /*intentionally*/, model->indexer ()->circuit_from_index (i).first));
  }
}

QIcon
RootItemData::icon (NetlistBrowserModel * /*model*/)
{
  return QIcon ();
}

QString
RootItemData::text (int /*column*/, NetlistBrowserModel * /*model*/)
{
  return QString ();
}

QString
RootItemData::search_text ()
{
  return QString ();
}

std::string
RootItemData::tooltip (NetlistBrowserModel * /*model*/)
{
  return std::string ();
}

db::NetlistCrossReference::Status
RootItemData::status (NetlistBrowserModel * /*model*/)
{
  return db::NetlistCrossReference::None;
}

CircuitItemData *
RootItemData::circuit_item (NetlistBrowserModel *model, const IndexedNetlistModel::circuit_pair &cp)
{
  if (! cp.first && ! cp.second) {
    return 0;
  }

  size_t index = model->indexer ()->circuit_index (cp);
  ensure_children (model);
  return dynamic_cast<CircuitItemData *> (child (index));
}

// ----------------------------------------------------------------------------------

CircuitItemData::CircuitItemData (NetlistModelItemData *parent, const IndexedNetlistModel::circuit_pair &cp)
  : NetlistModelItemData (parent), m_cp (cp)
{ }

void
CircuitItemData::do_ensure_children (NetlistBrowserModel *model)
{
  if (model->indexer ()->pin_count (circuits ()) > 0) {
    push_back (new CircuitItemNodeData (this, CircuitItemNodeData::Pins));
  }
  if (model->indexer ()->net_count (circuits ()) > 0) {
    push_back (new CircuitItemNodeData (this, CircuitItemNodeData::Nets));
  }
  if (model->indexer ()->subcircuit_count (circuits ()) > 0) {
    push_back (new CircuitItemNodeData (this, CircuitItemNodeData::SubCircuits));
  }
  if (model->indexer ()->device_count (circuits ()) > 0) {
    push_back (new CircuitItemNodeData (this, CircuitItemNodeData::Devices));
  }
}

bool
CircuitItemData::has_children (NetlistBrowserModel *model)
{
  if (model->indexer ()->pin_count (circuits ()) > 0) {
    return true;
  }
  if (model->indexer ()->net_count (circuits ()) > 0) {
    return true;
  }
  if (model->indexer ()->subcircuit_count (circuits ()) > 0) {
    return true;
  }
  if (model->indexer ()->device_count (circuits ()) > 0) {
    return true;
  }
  return false;
}


QIcon
CircuitItemData::icon (NetlistBrowserModel * /*model*/)
{
  return icon_for_circuit ();
}

QString
CircuitItemData::text (int column, NetlistBrowserModel *model)
{
  //  circuit:
  //  + single mode:     name              | <empty>  | <empty>
  //  + dual mode:       name(a)/name(b)   | name(a)  | name(b)
  if (column == model->object_column ()) {
    return escaped (str_from_names (m_cp, model->indexer ()->is_single ()));
  } else if (!model->indexer ()->is_single () && (column == model->first_column () || column == model->second_column ())) {
    return escaped (str_from_name (column == model->first_column () ? m_cp.first : m_cp.second));
  } else {
    return QString ();
  }
}

QString
CircuitItemData::search_text ()
{
  return tl::to_qstring (search_string_from_names (circuits ()));
}

std::string
CircuitItemData::tooltip (NetlistBrowserModel *model)
{
  size_t index = model->indexer ()->circuit_index (m_cp);
  return model->indexer ()->circuit_status_hint (index);
}

db::NetlistCrossReference::Status
CircuitItemData::status (NetlistBrowserModel *model)
{
  size_t index = model->indexer ()->circuit_index (m_cp);
  return model->indexer ()->circuit_from_index (index).second.first;
}

CircuitNetItemData *
CircuitItemData::circuit_net_item (NetlistBrowserModel *model, const IndexedNetlistModel::net_pair &np)
{
  ensure_children (model);

  for (size_t i = 0; i < child_count (); ++i) {
    CircuitNetItemData *d = static_cast<CircuitItemNodeData *> (child (i))->circuit_net_item (model, np);
    if (d) {
      return d;
    }
  }
  return 0;
}

CircuitDeviceItemData *
CircuitItemData::circuit_device_item (NetlistBrowserModel *model, const IndexedNetlistModel::device_pair &dp)
{
  ensure_children (model);

  for (size_t i = 0; i < child_count (); ++i) {
    CircuitDeviceItemData *d = static_cast<CircuitItemNodeData *> (child (i))->circuit_device_item (model, dp);
    if (d) {
      return d;
    }
  }
  return 0;
}

CircuitSubCircuitItemData *
CircuitItemData::circuit_subcircuit_item (NetlistBrowserModel *model, const IndexedNetlistModel::subcircuit_pair &sp)
{
  ensure_children (model);

  for (size_t i = 0; i < child_count (); ++i) {
    CircuitSubCircuitItemData *d = static_cast<CircuitItemNodeData *> (child (i))->circuit_subcircuit_item (model, sp);
    if (d) {
      return d;
    }
  }
  return 0;
}

// ----------------------------------------------------------------------------------

CircuitItemForSubCircuitData::CircuitItemForSubCircuitData (NetlistModelItemData *parent, const IndexedNetlistModel::subcircuit_pair &sp)
  : CircuitItemData (parent, circuit_refs_from_subcircuits (sp)), m_sp (sp)
{ }

QString
CircuitItemForSubCircuitData::text (int column, NetlistBrowserModel *model)
{
  if (column == model->object_column ()) {
    return tr ("Circuit");
  } else {
    return QString ();
  }
}

QString
CircuitItemForSubCircuitData::search_text ()
{
  return QString ();
}

std::string
CircuitItemForSubCircuitData::tooltip (NetlistBrowserModel * /*model*/)
{
  return std::string ();
}

db::NetlistCrossReference::Status
CircuitItemForSubCircuitData::status (NetlistBrowserModel * /*model*/)
{
  return db::NetlistCrossReference::None;
}

// ----------------------------------------------------------------------------------

CircuitItemNodeData::CircuitItemNodeData (NetlistModelItemData *parent, CircuitItemNodeData::type t)
  : NetlistModelItemData (parent), m_type (t)
{ }

bool
CircuitItemNodeData::has_children (NetlistBrowserModel *)
{
  //  the node only exists if it has children
  return true;
}

void
CircuitItemNodeData::do_ensure_children (NetlistBrowserModel *model)
{
  size_t n;

  if (m_type == Pins) {

    n = model->indexer ()->pin_count (circuits ());
    for (size_t i = 0; i < n; ++i) {
      push_back (new CircuitPinItemData (this, model->indexer ()->pin_from_index (circuits (), i).first));
    }

  } else if (m_type == Nets) {

    n = model->indexer ()->net_count (circuits ());
    for (size_t i = 0; i < n; ++i) {
      push_back (new CircuitNetItemData (this, model->indexer ()->net_from_index (circuits (), i).first));
    }

  } else if (m_type == SubCircuits) {

    n = model->indexer ()->subcircuit_count (circuits ());
    for (size_t i = 0; i < n; ++i) {
      push_back (new CircuitSubCircuitItemData (this, model->indexer ()->subcircuit_from_index (circuits (), i).first));
    }

  } else if (m_type == Devices) {

    n = model->indexer ()->device_count (circuits ());
    for (size_t i = 0; i < n; ++i) {
      push_back (new CircuitDeviceItemData (this, model->indexer ()->device_from_index (circuits (), i).first));
    }

  }
}

QIcon
CircuitItemNodeData::icon (NetlistBrowserModel * /*model*/)
{
  if (m_type == Pins) {
    return icon_for_pin ();
  } else if (m_type == SubCircuits) {
    return icon_for_circuit ();
  } else if (m_type == Devices) {
    return icon_for_device (0);
  } else if (m_type == Nets) {
    return icon_for_net ();
  } else {
    return QIcon ();
  }
}

QString
CircuitItemNodeData::text (int column, NetlistBrowserModel *model)
{
  if (column == model->object_column ()) {
    if (m_type == Pins) {
      return tr ("Pins");
    } else if (m_type == Devices) {
      return tr ("Devices");
    } else if (m_type == Nets) {
      return tr ("Nets");
    } else if (m_type == SubCircuits) {
      return tr ("Subcircuits");
    }
  }
  return QString ();
}

QString
CircuitItemNodeData::search_text ()
{
  return QString ();
}

std::string
CircuitItemNodeData::tooltip (NetlistBrowserModel * /*model*/)
{
  return std::string ();
}

db::NetlistCrossReference::Status
CircuitItemNodeData::status (NetlistBrowserModel * /*model*/)
{
  return db::NetlistCrossReference::None;
}

CircuitNetItemData *
CircuitItemNodeData::circuit_net_item (NetlistBrowserModel *model, const IndexedNetlistModel::net_pair &np)
{
  if (! np.first && ! np.second) {
    return 0;
  }

  ensure_children (model);

  if (m_type == Nets) {
    ensure_children (model);
    size_t index = model->indexer ()->net_index (np);
    return dynamic_cast<CircuitNetItemData *> (child (index));
  } else {
    return 0;
  }
}

CircuitDeviceItemData *
CircuitItemNodeData::circuit_device_item (NetlistBrowserModel *model, const IndexedNetlistModel::device_pair &dp)
{
  if (! dp.first && ! dp.second) {
    return 0;
  }

  ensure_children (model);

  if (m_type == Devices) {
    ensure_children (model);
    size_t index = model->indexer ()->device_index (dp);
    return dynamic_cast<CircuitDeviceItemData *> (child (index));
  } else {
    return 0;
  }
}

CircuitSubCircuitItemData *
CircuitItemNodeData::circuit_subcircuit_item (NetlistBrowserModel *model, const IndexedNetlistModel::subcircuit_pair &sp)
{
  if (! sp.first && ! sp.second) {
    return 0;
  }

  ensure_children (model);

  if (m_type == SubCircuits) {
    ensure_children (model);
    size_t index = model->indexer ()->subcircuit_index (sp);
    return dynamic_cast<CircuitSubCircuitItemData *> (child (index));
  } else {
    return 0;
  }
}

// ----------------------------------------------------------------------------------

CircuitPinItemData::CircuitPinItemData (NetlistModelItemData *parent, const IndexedNetlistModel::pin_pair &pp)
  : CircuitNetItemData (parent, nets_from_circuit_pins (parent->circuits (), pp)),
    m_pp (pp)
{
  //  .. nothing yet ..
}

QIcon
CircuitPinItemData::icon (NetlistBrowserModel * /*model*/)
{
  return icon_for_pin ();
}

QString
CircuitPinItemData::text (int column, NetlistBrowserModel *model)
{
  if (column == model->object_column ()) {
    std::string suffix;
    if (seen ()) {
      suffix = tl::to_string (tr (" (already seen)"));
    }
    return escaped (str_from_expanded_names (m_pp, model->indexer ()->is_single ()) + suffix);
  } else {
    return CircuitNetItemData::text (column, model);
  }
}

QString
CircuitPinItemData::search_text ()
{
  return tl::to_qstring (combine_search_strings (search_string_from_expanded_names (m_pp), search_string_from_expanded_names (nets ())));
}

// ----------------------------------------------------------------------------------

CircuitNetItemData::CircuitNetItemData (NetlistModelItemData *parent, const IndexedNetlistModel::net_pair &np)
  : NetlistModelItemData (parent), m_np (np), m_seen (parent && parent->derived_from_nets (np))
{ }

void
CircuitNetItemData::do_ensure_children (NetlistBrowserModel *model)
{
  //  no subnodes if already seen
  if (m_seen) {
    return;
  }

  size_t n;

  n = model->indexer ()->net_terminal_count (np ());
  for (size_t i = 0; i < n; ++i) {
    push_back (new CircuitNetDeviceTerminalItemData (this, model->indexer ()->net_terminalref_from_index (np (), i)));
  }

  n = model->indexer ()->net_pin_count (np ());
  for (size_t i = 0; i < n; ++i) {
    push_back (new CircuitNetPinItemData (this, model->indexer ()->net_pinref_from_index (np (), i)));
  }

  n = model->indexer ()->net_subcircuit_pin_count (np ());
  for (size_t i = 0; i < n; ++i) {
    push_back (new CircuitNetSubCircuitPinItemData (this, model->indexer ()->net_subcircuit_pinref_from_index (np (), i)));
  }
}

QIcon
CircuitNetItemData::icon (NetlistBrowserModel *model)
{
  return model->icon_for_nets (m_np);
}

QString
CircuitNetItemData::text (int column, NetlistBrowserModel *model)
{
  //  circuit/net: header column = node count, second column net name
  if (column == model->object_column ()) {
    std::string suffix;
    if (seen ()) {
      suffix = tl::to_string (tr (" (already seen)"));
    }
    return escaped (str_from_expanded_names (m_np, model->indexer ()->is_single ()) + suffix);
  } else if (column == model->first_column () && m_np.first) {
    return escaped (m_np.first->expanded_name () + " (" + tl::to_string (m_np.first->pin_count () + m_np.first->terminal_count () + m_np.first->subcircuit_pin_count ()) + ")");
  } else if (column == model->second_column () && m_np.second) {
    return escaped (m_np.second->expanded_name () + " (" + tl::to_string (m_np.second->pin_count () + m_np.second->terminal_count () + m_np.second->subcircuit_pin_count ()) + ")");
  }

  return QString ();
}

QString
CircuitNetItemData::search_text ()
{
  return tl::to_qstring (search_string_from_expanded_names (m_np));
}

std::string
CircuitNetItemData::tooltip (NetlistBrowserModel *model)
{
  if (m_np.first || m_np.second) {
    size_t index = model->indexer ()->net_index (m_np);
    if (index == lay::no_netlist_index) {
      return std::string ();
    } else {
      return model->indexer ()->net_status_hint (circuits (), index);
    }
  } else {
    return std::string ();
  }
}

db::NetlistCrossReference::Status
CircuitNetItemData::status (NetlistBrowserModel *model)
{
  if (m_np.first || m_np.second) {
    size_t index = model->indexer ()->net_index (m_np);
    if (index == lay::no_netlist_index) {
      return db::NetlistCrossReference::None;
    } else {
      return model->indexer ()->net_from_index (circuits (), index).second.first;
    }
  } else {
    return db::NetlistCrossReference::None;
  }
}

// ----------------------------------------------------------------------------------

CircuitNetDeviceTerminalItemData::CircuitNetDeviceTerminalItemData (NetlistModelItemData *parent, const IndexedNetlistModel::net_terminal_pair &tp)
  : NetlistModelItemData (parent), m_tp (tp), m_device_seen (parent && parent->derived_from_devices (dp ()))
{ }

void
CircuitNetDeviceTerminalItemData::do_ensure_children (NetlistBrowserModel *model)
{
  if (m_device_seen) {
    return;
  }

  std::pair<const db::Device *, const db::Device *> devices = dp ();
  std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (devices);

  std::vector<std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> > tp = terminal_defs_from_device_classes (model->indexer (), device_classes, devices);

  for (std::vector<std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> >::const_iterator i = tp.begin (); i != tp.end (); ++i) {
    IndexedNetlistModel::net_pair nets = nets_from_device_terminals (dp (), *i);
    push_back (new CircuitNetDeviceTerminalOthersItemData (this, nets, *i));
  }
}

QIcon
CircuitNetDeviceTerminalItemData::icon (NetlistBrowserModel * /*model*/)
{
  std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (dp ());
  return icon_for_devices (device_classes, terminal_defs_from_terminal_refs (m_tp));
}

QString
CircuitNetDeviceTerminalItemData::text (int column, NetlistBrowserModel *model)
{
  //  circuit/net/device terminal: header column = terminal and device string, second column = device name
  if (column == model->object_column ()) {

    std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> termdefs = terminal_defs_from_terminal_refs (m_tp);

    std::string suffix;
    if (m_device_seen) {
      suffix = tl::to_string (tr (" (already seen)"));
    }

    if (model->indexer ()->is_single ()) {
      return escaped (str_from_name (termdefs.first) + field_sep + device_string (dp ().first) + suffix);
    } else {
      return escaped (str_from_names (termdefs, model->indexer ()->is_single ()) + field_sep + devices_string (dp (), model->indexer ()->is_single (), true /*with parameters*/) + suffix);
    }

  } else if (column == model->first_column () || column == model->second_column ()) {
    return model->make_link_to (dp (), column);
  }

  return QString ();
}

QString
CircuitNetDeviceTerminalItemData::search_text ()
{
  std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (dp ());
  std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> termdefs = terminal_defs_from_terminal_refs (tp ());
  return tl::to_qstring (combine_search_strings (combine_search_strings (search_string_from_names (termdefs), search_string_from_names (device_classes)), search_string_from_expanded_names (dp ())));
}

std::string
CircuitNetDeviceTerminalItemData::tooltip (NetlistBrowserModel *model)
{
  return model->indexer ()->device_status_hint (circuits (), model->indexer ()->device_index (dp ()));
}

db::NetlistCrossReference::Status
CircuitNetDeviceTerminalItemData::status (NetlistBrowserModel *model)
{
  return model->indexer ()->device_from_index (circuits (), model->indexer ()->device_index (dp ())).second.first;
}

// ----------------------------------------------------------------------------------

CircuitNetDeviceTerminalOthersItemData::CircuitNetDeviceTerminalOthersItemData (NetlistModelItemData *parent, const IndexedNetlistModel::net_pair &np, const std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> &tp)
  : CircuitNetItemData (parent, nets_from_device_terminals (parent->devices (), tp)), m_tp (tp), m_np (np)
{ }

QIcon
CircuitNetDeviceTerminalOthersItemData::icon (NetlistBrowserModel *model)
{
  return model->icon_for_connection (nets_from_device_terminals (devices (), m_tp));
}

QString
CircuitNetDeviceTerminalOthersItemData::text (int column, NetlistBrowserModel *model)
{
  //  circuit/net/device terminal/more: header column = terminal name, second column = net link
  if (column == model->object_column ()) {
    return escaped (str_from_names (m_tp, model->indexer ()->is_single ()) + (seen () ? tl::to_string (tr (" (already seen)")) : std::string ()));
  } else {
    return CircuitNetItemData::text (column, model);
  }
}

QString
CircuitNetDeviceTerminalOthersItemData::search_text ()
{
  return tl::to_qstring (combine_search_strings (search_string_from_names (m_tp), search_string_from_expanded_names (nets_of_this ())));
}

// ----------------------------------------------------------------------------------

CircuitNetSubCircuitPinItemData::CircuitNetSubCircuitPinItemData (NetlistModelItemData *parent, const IndexedNetlistModel::net_subcircuit_pin_pair &sp)
  : NetlistModelItemData (parent), m_sp (sp), m_subcircuit_seen (parent && parent->derived_from_subcircuits (subcircuits_from_pinrefs (sp)))
{ }

void
CircuitNetSubCircuitPinItemData::do_ensure_children (NetlistBrowserModel *model)
{
  if (m_subcircuit_seen) {
    return;
  }

  IndexedNetlistModel::pin_pair pins = pins_from_pinrefs (m_sp);
  IndexedNetlistModel::net_pair nets = nets_from_circuit_pins (circuits (), pins);

  //  Because of pin ambiguities the net identity might need to be fixed
  if (nets.first) {
    nets.second = model->indexer ()->second_net_for (nets.first);
  }

  push_back (new CircuitNetItemData (this, nets));
}

QIcon
CircuitNetSubCircuitPinItemData::icon (NetlistBrowserModel * /*model*/)
{
  return icon_for_subcircuit ();
}

QString
CircuitNetSubCircuitPinItemData::text (int column, NetlistBrowserModel *model)
{
  //  circuit/net/pin: header column = pin name, second column empty (for now)
  if (column == model->object_column ()) {

    QString suffix;
    if (m_subcircuit_seen) {
      suffix = tr (" (already seen)");
    }

    return model->make_link_to (pp (), circuits ()) + tl::to_qstring (field_sep) + model->make_link_to (circuits ()) + suffix;

  } else if (column == model->first_column () || column == model->second_column ()) {
    return model->make_link_to (subcircuits (), column);
  }

  return QString ();
}

QString
CircuitNetSubCircuitPinItemData::search_text ()
{
  return tl::to_qstring (combine_search_strings (combine_search_strings (search_string_from_names (pp ()), search_string_from_names (circuits ())), search_string_from_expanded_names (subcircuits ())));
}

std::string
CircuitNetSubCircuitPinItemData::tooltip (NetlistBrowserModel *model)
{
  return model->indexer ()->subcircuit_status_hint (parent ()->circuits (), model->indexer ()->subcircuit_index (subcircuits ()));
}

db::NetlistCrossReference::Status
CircuitNetSubCircuitPinItemData::status (NetlistBrowserModel *model)
{
  return model->indexer ()->subcircuit_from_index (parent ()->circuits (), model->indexer ()->subcircuit_index (subcircuits ())).second.first;
}

// ----------------------------------------------------------------------------------

CircuitNetPinItemData::CircuitNetPinItemData (NetlistModelItemData *parent, const IndexedNetlistModel::net_pin_pair &pp)
  : NetlistModelItemData (parent), m_pp (pp)
{ }

void
CircuitNetPinItemData::do_ensure_children (NetlistBrowserModel *)
{
  //  nothing (leaf node)
}

QIcon
CircuitNetPinItemData::icon (NetlistBrowserModel * /*model*/)
{
  return icon_for_pin ();
}

QString
CircuitNetPinItemData::text (int column, NetlistBrowserModel *model)
{
  //  circuit/net/pin: header column = pin name, second column empty (for now)
  IndexedNetlistModel::circuit_pair circuits (m_pp.first && m_pp.first->net () ? m_pp.first->net ()->circuit () : 0, m_pp.second && m_pp.second->net () ? m_pp.second->net ()->circuit () : 0);
  if (model->indexer ()->is_single () && column == model->object_column ()) {
    return model->make_link_to (pins_from_pinrefs (m_pp), circuits);
  } else if (! model->indexer ()->is_single () && (column == model->first_column () || column == model->second_column ())) {
    return model->make_link_to (pins_from_pinrefs (m_pp), circuits, column);
  }

  return QString ();
}

QString
CircuitNetPinItemData::search_text ()
{
  return tl::to_qstring (search_string_from_names (pins_from_pinrefs (m_pp)));
}

std::string
CircuitNetPinItemData::tooltip (NetlistBrowserModel * /*model*/)
{
  return std::string ();
}

db::NetlistCrossReference::Status
CircuitNetPinItemData::status (NetlistBrowserModel * /*model*/)
{
  return db::NetlistCrossReference::None;
}

// ----------------------------------------------------------------------------------

CircuitSubCircuitPinsItemData::CircuitSubCircuitPinsItemData (NetlistModelItemData *parent, const IndexedNetlistModel::subcircuit_pair &sp)
  : NetlistModelItemData (parent), m_sp (sp)
{ }

bool
CircuitSubCircuitPinsItemData::has_children (NetlistBrowserModel *model)
{
  return model->indexer ()->subcircuit_pin_count (sp ()) > 0;
}

void
CircuitSubCircuitPinsItemData::do_ensure_children (NetlistBrowserModel *model)
{
  size_t n = model->indexer ()->subcircuit_pin_count (sp ());
  for (size_t i = 0; i < n; ++i) {
    IndexedNetlistModel::net_subcircuit_pin_pair pp = model->indexer ()->subcircuit_pinref_from_index (sp (), i);
    push_back (new CircuitSubCircuitPinItemData (this, pp));
  }
}

QIcon
CircuitSubCircuitPinsItemData::icon (NetlistBrowserModel * /*model*/)
{
  return icon_for_pin ();
}

QString
CircuitSubCircuitPinsItemData::text (int column, NetlistBrowserModel *model)
{
  if (column == model->object_column ()) {
    return tr ("Connections");
  } else {
    return QString ();
  }
}

QString
CircuitSubCircuitPinsItemData::search_text ()
{
  return QString ();
}

std::string
CircuitSubCircuitPinsItemData::tooltip (NetlistBrowserModel * /*model*/)
{
  return std::string ();
}

db::NetlistCrossReference::Status
CircuitSubCircuitPinsItemData::status (NetlistBrowserModel * /*model*/)
{
  return db::NetlistCrossReference::None;
}

// ----------------------------------------------------------------------------------

CircuitSubCircuitItemData::CircuitSubCircuitItemData (NetlistModelItemData *parent, const IndexedNetlistModel::subcircuit_pair &sp)
  : NetlistModelItemData (parent), m_sp (sp), mp_circuit_node (0)
{ }

void
CircuitSubCircuitItemData::do_ensure_children (NetlistBrowserModel * /*model*/)
{
  push_back (new CircuitSubCircuitPinsItemData (this, m_sp));

  //  plus one item for the circuit reference
  mp_circuit_node = new CircuitItemForSubCircuitData (this, m_sp);
  push_back (mp_circuit_node);
}

QIcon
CircuitSubCircuitItemData::icon (NetlistBrowserModel * /*model*/)
{
  return icon_for_subcircuit ();
}

QString
CircuitSubCircuitItemData::text (int column, NetlistBrowserModel *model)
{
  //  circuit/subcircuit: header column = circuit name, second column subcircuit name
  IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (m_sp);
  if (column == model->object_column ()) {
    return model->make_link_to (circuit_refs);
  } else if (column == model->first_column ()) {
    return escaped (str_from_expanded_name (m_sp.first));
  } else if (column == model->second_column ()) {
    return escaped (str_from_expanded_name (m_sp.second));
  }

  return QString ();
}

QString
CircuitSubCircuitItemData::search_text ()
{
  IndexedNetlistModel::circuit_pair circuit_refs = circuit_refs_from_subcircuits (sp ());
  return tl::to_qstring (combine_search_strings (search_string_from_names (circuit_refs), search_string_from_expanded_names (sp ())));
}

std::string
CircuitSubCircuitItemData::tooltip (NetlistBrowserModel *model)
{
  size_t index = model->indexer ()->subcircuit_index (sp ());
  return model->indexer ()->subcircuit_status_hint (circuits (), index);
}

db::NetlistCrossReference::Status
CircuitSubCircuitItemData::status (NetlistBrowserModel *model)
{
  size_t index = model->indexer ()->subcircuit_index (sp ());
  return model->indexer ()->subcircuit_from_index (circuits (), index).second.first;
}

// ----------------------------------------------------------------------------------

CircuitSubCircuitPinItemData::CircuitSubCircuitPinItemData (NetlistModelItemData *parent, const IndexedNetlistModel::net_subcircuit_pin_pair &pp)
  : CircuitNetItemData (parent, nets_from_pinrefs (pp)), m_pp (pp)
{ }

QIcon
CircuitSubCircuitPinItemData::icon (NetlistBrowserModel * /*model*/)
{
  return icon_for_pin ();
}

QString
CircuitSubCircuitPinItemData::text (int column, NetlistBrowserModel *model)
{
  if (column == model->object_column ()) {
    return model->make_link_to (pins (), circuit_refs_from_subcircuits (sp ())) + (seen () ? tr (" (already seen)") : QString ());
  } else {
    return CircuitNetItemData::text (column, model);
  }
}

QString
CircuitSubCircuitPinItemData::search_text ()
{
  return tl::to_qstring (combine_search_strings (search_string_from_names (pins ()), search_string_from_expanded_names (nets_from_pinrefs (m_pp))));
}

// ----------------------------------------------------------------------------------

CircuitDeviceItemData::CircuitDeviceItemData (NetlistModelItemData *parent, const IndexedNetlistModel::device_pair &dp)
  : NetlistModelItemData (parent), m_dp (dp)
{ }

void
CircuitDeviceItemData::do_ensure_children (NetlistBrowserModel *model)
{
  std::pair<const db::Device *, const db::Device *> devices = dp ();
  std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (devices);

  std::vector<std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> > tp = terminal_defs_from_device_classes (model->indexer (), device_classes, devices);

  for (std::vector<std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> >::const_iterator i = tp.begin (); i != tp.end (); ++i) {
    push_back (new CircuitDeviceTerminalItemData (this, *i));
  }
}

QIcon
CircuitDeviceItemData::icon (NetlistBrowserModel * /*model*/)
{
  std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (m_dp);
  return icon_for_devices (device_classes);
}

QString
CircuitDeviceItemData::text (int column, NetlistBrowserModel *model)
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

  return QString ();
}

QString
CircuitDeviceItemData::search_text ()
{
  std::pair<const db::DeviceClass *, const db::DeviceClass *> device_classes = device_classes_from_devices (dp ());
  return tl::to_qstring (combine_search_strings (search_string_from_expanded_names (dp ()), search_string_from_names (device_classes)));
}

std::string
CircuitDeviceItemData::tooltip (NetlistBrowserModel *model)
{
  size_t index = model->indexer ()->device_index (m_dp);
  return model->indexer ()->device_status_hint (circuits (), index);
}

db::NetlistCrossReference::Status
CircuitDeviceItemData::status (NetlistBrowserModel *model)
{
  size_t index = model->indexer ()->device_index (m_dp);
  return model->indexer ()->device_from_index (circuits (), index).second.first;
}

// ----------------------------------------------------------------------------------

CircuitDeviceTerminalItemData::CircuitDeviceTerminalItemData (NetlistModelItemData *parent, const std::pair<const db::DeviceTerminalDefinition *, const db::DeviceTerminalDefinition *> &tp)
  : CircuitNetItemData (parent, nets_from_device_terminals (parent->devices (), tp)), m_tp (tp)
{ }

QIcon
CircuitDeviceTerminalItemData::icon (NetlistBrowserModel *model)
{
  return model->icon_for_connection (nets_of_this ());
}

QString
CircuitDeviceTerminalItemData::text (int column, NetlistBrowserModel *model)
{
  if (column == model->object_column ()) {
    std::string suffix;
    if (seen ()) {
      suffix = tl::to_string (tr (" (already seen)"));
    }
    return escaped (str_from_names (m_tp, model->indexer ()->is_single ()) + suffix);
  } else {
    return CircuitNetItemData::text (column, model);
  }
}

QString
CircuitDeviceTerminalItemData::search_text ()
{
  return tl::to_qstring (combine_search_strings (search_string_from_names (m_tp), search_string_from_expanded_names (nets_of_this ())));
}

// ----------------------------------------------------------------------------------
//  NetlistBrowserModel implementation

NetlistBrowserModel::NetlistBrowserModel (QWidget *parent, db::Netlist *netlist, NetColorizer *colorizer)
  : QAbstractItemModel (parent), mp_l2ndb (0), mp_lvsdb (0), mp_colorizer (colorizer)
{
  mp_root.reset (new RootItemData ());
  mp_indexer.reset (new SingleIndexedNetlistModel (netlist));
  mp_colorizer->colors_changed.add (this, &NetlistBrowserModel::colors_changed);

  m_object_column = 0;
  m_status_column = -1;
  m_first_column = 2;
  m_second_column = -1;
}

NetlistBrowserModel::NetlistBrowserModel (QWidget *parent, db::LayoutToNetlist *l2ndb, NetColorizer *colorizer)
  : QAbstractItemModel (parent), mp_l2ndb (l2ndb), mp_lvsdb (0), mp_colorizer (colorizer)
{
  mp_root.reset (new RootItemData ());
  mp_indexer.reset (new SingleIndexedNetlistModel (l2ndb->netlist ()));
  mp_colorizer->colors_changed.add (this, &NetlistBrowserModel::colors_changed);

  m_object_column = 0;
  m_status_column = -1;
  m_first_column = 2;
  m_second_column = -1;
}

NetlistBrowserModel::NetlistBrowserModel (QWidget *parent, db::LayoutVsSchematic *lvsdb, NetColorizer *colorizer)
  : QAbstractItemModel (parent), mp_l2ndb (0), mp_lvsdb (lvsdb), mp_colorizer (colorizer)
{
  mp_root.reset (new RootItemData ());
  mp_indexer.reset (new NetlistCrossReferenceModel (lvsdb->cross_ref ()));
  mp_colorizer->colors_changed.add (this, &NetlistBrowserModel::colors_changed);

  m_object_column = 0;
  m_status_column = 1;
  m_first_column = 2;
  m_second_column = 3;
}

NetlistBrowserModel::~NetlistBrowserModel ()
{
  //  .. nothing yet ..
}

RootItemData *
NetlistBrowserModel::root () const
{
  return dynamic_cast<RootItemData *> (mp_root.get ());
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
    return QIcon (":/error2_16px.png");
  } else if (status == db::NetlistCrossReference::MatchWithWarning || status == db::NetlistCrossReference::Skipped) {
    return QIcon (":/warn_16px.png");
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
  } else if (role == Qt::ToolTipRole) {
    if (index.column () == m_status_column) {
      return tooltip (index);
    } else {
      return QVariant (text (index));
    }
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

NetlistObjectsPath
NetlistBrowserModel::path_from_index (const QModelIndex &index) const
{
  NetlistObjectsPath np;
  np.net = net_from_index (index, false);
  np.device = device_from_index (index, false);

  QModelIndex i = index;
  while (i.isValid ()) {

    IndexedNetlistModel::subcircuit_pair sp = subcircuit_from_index (i, false);
    if (sp.first || sp.second) {
      np.path.push_front (sp);
    } else {
      IndexedNetlistModel::circuit_pair cp = circuit_from_index (i, false);
      if (cp.first || cp.second) {
        np.root = cp;
      }
    }

    i = parent (i);

  }

  return np;
}

QModelIndex
NetlistBrowserModel::index_from_path (const NetlistObjectsPath &path)
{
  QModelIndex index = index_from_circuit (path.root);

  CircuitItemData *node = dynamic_cast<CircuitItemData *> ((NetlistModelItemData *) (index.internalPointer ()));

  for (std::list<std::pair<const db::SubCircuit *, const db::SubCircuit *> >::const_iterator p = path.path.begin (); p != path.path.end () && node; ++p) {
    CircuitSubCircuitItemData *sc_node = node->circuit_subcircuit_item (this, *p);
    if (sc_node) {
      sc_node->ensure_children (this);
      node = sc_node->circuit_item ();
    } else {
      node = 0;
    }
  }

  CircuitNetItemData *net_node = 0;
  CircuitDeviceItemData *device_node = 0;

  if (node) {
    net_node = node->circuit_net_item (const_cast<NetlistBrowserModel *> (this), path.net);
    device_node = node->circuit_device_item (const_cast<NetlistBrowserModel *> (this), path.device);
  }

  if (net_node) {
    return createIndex (int (net_node->index ()), 0, (void *) net_node);
  } else if (device_node) {
    return createIndex (int (device_node->index ()), 0, (void *) device_node);
  } else if (node) {
    return createIndex (int (node->index ()), 0, (void *) node);
  } else {
    return QModelIndex ();
  }
}

QModelIndex
NetlistBrowserModel::index_from_url (const QString &a) const
{
  QUrl url (a);

  std::string ids;
#if QT_VERSION >= 0x050000
  ids = tl::to_string (QUrlQuery (url.query ()).queryItemValue (QString::fromUtf8 ("path")));
#else
  ids = tl::to_string (url.queryItemValue (QString::fromUtf8 ("path")));
#endif

  QModelIndex idx;

  tl::Extractor ex (ids.c_str ());
  while (! ex.at_end ()) {
    int n = 0;
    if (! ex.try_read (n)) {
      break;
    }
    idx = index (n, 0, idx);
    ex.test (",");
  }

  return idx;
}

QString
NetlistBrowserModel::build_url (const QModelIndex &index, const std::string &title) const
{
  if (! index.isValid ()) {
    //  no link
    return tl::to_qstring (tl::escaped_to_html (title));
  }

  QModelIndex i = index;

  std::string pstr;
  while (i.isValid ()) {
    if (pstr.empty ()) {
      pstr = tl::to_string (i.row ());
    } else {
      pstr = tl::to_string (i.row ()) + "," + pstr;
    }
    i = parent (i);
  }

  std::string s = std::string ("<a href='int:netlist");
  s += "?path=";
  s += pstr;
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

    QModelIndex idx = index_from_net (nets);

    if (mp_indexer->is_single () || column == m_first_column) {
      return build_url (idx, str_from_expanded_name (nets.first));
    } else if (column == m_second_column) {
      return build_url (idx, str_from_expanded_name (nets.second));
    } else {
      return build_url (idx, str_from_expanded_names (nets, mp_indexer->is_single ()));
    }

  }
}

QString
NetlistBrowserModel::make_link_to (const std::pair<const db::Device *, const db::Device *> &devices, int column) const
{
  QModelIndex idx;

  if ((! devices.first || column == m_second_column) && (! devices.second || column == m_first_column)) {
    return QString ();
  } else {

    if (mp_indexer->is_single () || column == m_first_column) {
      return build_url (idx, str_from_expanded_name (devices.first));
    } else if (column == m_second_column) {
      return build_url (idx, str_from_expanded_name (devices.second));
    } else {
      return build_url (idx, str_from_expanded_names (devices, mp_indexer->is_single ()));
    }

  }
}

QString
NetlistBrowserModel::make_link_to (const std::pair<const db::Pin *, const db::Pin *> &pins, const std::pair<const db::Circuit *, const db::Circuit *> & /*circuits*/, int column) const
{
  QModelIndex idx;

  if ((! pins.first || column == m_second_column) && (! pins.second || column == m_first_column)) {
    return QString ();
  } else {
    if (mp_indexer->is_single () || column == m_first_column) {
      return build_url (idx, str_from_expanded_name (pins.first));
    } else if (column == m_second_column) {
      return build_url (idx, str_from_expanded_name (pins.second));
    } else {
      return build_url (idx, str_from_expanded_names (pins, mp_indexer->is_single ()));
    }
  }
}

QString
NetlistBrowserModel::make_link_to (const std::pair<const db::Circuit *, const db::Circuit *> &circuits, int column) const
{
  if ((! circuits.first || column == m_second_column) && (! circuits.second || column == m_first_column)) {
    return QString ();
  } else {

    QModelIndex idx = index_from_circuit (circuits);

    if (mp_indexer->is_single () || column == m_first_column) {
      return build_url (idx, str_from_name (circuits.first));
    } else if (column == m_second_column) {
      return build_url (idx, str_from_name (circuits.second));
    } else {
      return build_url (idx, str_from_names (circuits, mp_indexer->is_single ()));
    }

  }
}

QString
NetlistBrowserModel::make_link_to (const std::pair<const db::SubCircuit *, const db::SubCircuit *> &subcircuits, int column) const
{
  if ((! subcircuits.first || column == m_second_column) && (! subcircuits.second || column == m_first_column)) {
    return QString ();
  } else {

    QModelIndex idx = index_from_subcircuit (subcircuits);

    if (mp_indexer->is_single () || column == m_first_column) {
      return build_url (idx, str_from_expanded_name (subcircuits.first));
    } else if (column == m_second_column) {
      return build_url (idx, str_from_expanded_name (subcircuits.second));
    } else {
      return build_url (idx, str_from_expanded_names (subcircuits, mp_indexer->is_single ()));
    }

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
    return d->status (const_cast<NetlistBrowserModel *> (this));
  }
}

QVariant
NetlistBrowserModel::tooltip (const QModelIndex &index) const
{
  std::string hint;
  NetlistModelItemData *d = (NetlistModelItemData *) (index.internalPointer ());
  if (d) {
    hint = d->tooltip (const_cast<NetlistBrowserModel *> (this));
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

QIcon
NetlistBrowserModel::icon_for_nets (const std::pair<const db::Net *, const db::Net *> &nets) const
{
  const db::Net *net = nets.first;

  if (mp_colorizer && mp_colorizer->has_color_for_net (net)) {

    tl::Color color = mp_colorizer->color_of_net (net);

    tl::color_t rgb = tl::color_t (color.rgb ());
    std::map<tl::color_t, QIcon>::const_iterator c = m_net_icon_per_color.find (rgb);
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

    tl::Color color = mp_colorizer->color_of_net (net);

    tl::color_t rgb = tl::color_t (color.rgb ());
    std::map<tl::color_t, QIcon>::const_iterator c = m_connection_icon_per_color.find (rgb);
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
    return d->icon (const_cast<NetlistBrowserModel *> (this));
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
  NetlistModelItemData *d = 0;
  if (parent.isValid ()) {
    d = (NetlistModelItemData *) (parent.internalPointer ());
  } else {
    d = mp_root.get ();
  }
  if (d) {
    return d->has_children (const_cast<NetlistBrowserModel *> (this));
  } else {
    return false;
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
    return QIcon (":/info_16px.png");
  }

  return QVariant ();
}

QModelIndex
NetlistBrowserModel::index (int row, int column, const QModelIndex &parent) const
{
  NetlistModelItemData *d = 0;
  if (! parent.isValid ()) {
    d = mp_root.get ();
  } else {
    d = (NetlistModelItemData *) (parent.internalPointer ());
  }
  if (d) {
    d->ensure_children (const_cast<NetlistBrowserModel *> (this));
    return createIndex (row, column, (void *) d->child (size_t (row)));
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
  CircuitItemData *ci = root ()->circuit_item (const_cast<NetlistBrowserModel *> (this), circuits);
  if (ci) {
    CircuitNetItemData *ni = ci->circuit_net_item (const_cast<NetlistBrowserModel *> (this), nets);
    if (ni) {
      return createIndex (int (ni->index ()), 0, (void *) ni);
    }
  }

  return QModelIndex ();
}

QModelIndex
NetlistBrowserModel::index_from_net (const db::Net *net) const
{
  return index_from_net (std::make_pair (net, mp_indexer->second_net_for (net)));
}

QModelIndex
NetlistBrowserModel::index_from_circuit (const std::pair<const db::Circuit *, const db::Circuit *> &circuits) const
{
  CircuitItemData *ci = root ()->circuit_item (const_cast<NetlistBrowserModel *> (this), circuits);
  if (ci) {
    return createIndex (int (ci->index ()), 0, (void *) ci);
  }

  return QModelIndex ();
}

QModelIndex
NetlistBrowserModel::index_from_subcircuit (const std::pair<const db::SubCircuit *, const db::SubCircuit *> &subcircuits) const
{
  IndexedNetlistModel::circuit_pair circuits (subcircuits.first ? subcircuits.first->circuit () : 0, subcircuits.second ? subcircuits.second->circuit () : 0);
  CircuitItemData *ci = root ()->circuit_item (const_cast<NetlistBrowserModel *> (this), circuits);
  if (ci) {
    CircuitSubCircuitItemData *si = ci->circuit_subcircuit_item (const_cast<NetlistBrowserModel *> (this), subcircuits);
    if (si) {
      return createIndex (int (si->index ()), 0, (void *) si);
    }
  }

  return QModelIndex ();
}

QModelIndex
NetlistBrowserModel::index_from_circuit (const db::Circuit *net) const
{
  return index_from_circuit (std::make_pair (net, mp_indexer->second_circuit_for (net)));
}

std::pair<const db::Circuit *, const db::Circuit *>
NetlistBrowserModel::circuit_from_index (const QModelIndex &index, bool include_parents) const
{
  NetlistModelItemData *d = (NetlistModelItemData *) (index.internalPointer ());
  if (! d) {
    return std::pair<const db::Circuit *, const db::Circuit *> ((const db::Circuit *) 0, (const db::Circuit *) 0);
  } else {
    return include_parents ? d->circuits () : d->circuits_of_this ();
  }
}

std::pair<const db::Net *, const db::Net *>
NetlistBrowserModel::net_from_index (const QModelIndex &index, bool include_parents) const
{
  NetlistModelItemData *d = (NetlistModelItemData *) (index.internalPointer ());
  if (! d) {
    return std::pair<const db::Net *, const db::Net *> ((const db::Net *) 0, (const db::Net *) 0);
  } else {
    return include_parents ? d->nets () : d->nets_of_this ();
  }
}

std::pair<const db::Device *, const db::Device *>
NetlistBrowserModel::device_from_index (const QModelIndex &index, bool include_parents) const
{
  NetlistModelItemData *d = (NetlistModelItemData *) (index.internalPointer ());
  if (! d) {
    return std::pair<const db::Device *, const db::Device *> ((const db::Device *) 0, (const db::Device *) 0);
  } else {
    return include_parents ? d->devices () : d->devices_of_this ();
  }
}

std::pair<const db::SubCircuit *, const db::SubCircuit *>
NetlistBrowserModel::subcircuit_from_index (const QModelIndex &index, bool include_parents) const
{
  NetlistModelItemData *d = (NetlistModelItemData *) (index.internalPointer ());
  if (! d) {
    return std::pair<const db::SubCircuit *, const db::SubCircuit *> ((const db::SubCircuit *) 0, (const db::SubCircuit *) 0);
  } else {
    return include_parents ? d->subcircuits () : d->subcircuits_of_this ();
  }
}

QModelIndex
NetlistBrowserModel::parent (const QModelIndex &index) const
{
  NetlistModelItemData *d = (NetlistModelItemData *) (index.internalPointer ());
  if (! d || ! d->parent ()) {
    return QModelIndex ();
  } else {
    return createIndex ((int) d->parent ()->index (), 0, (void *) d->parent ());
  }
}

int
NetlistBrowserModel::rowCount (const QModelIndex &parent) const
{
  NetlistModelItemData *d = 0;
  if (parent.isValid ()) {
    d = (NetlistModelItemData *) (parent.internalPointer ());
  } else {
    d = mp_root.get ();
  }
  if (d) {
    d->ensure_children (const_cast<NetlistBrowserModel *> (this));
    return int (d->child_count ());
  } else {
    return 0;
  }
}

void
NetlistBrowserModel::show_or_hide_items (QTreeView *view, const QModelIndex &parent, bool show_all, bool with_warnings, int levels)
{
  int n = rowCount (parent);
  for (int i = 0; i < n; ++i) {

    QModelIndex idx = index (i, 0, parent);

    IndexedNetlistModel::Status st = status (idx);
    bool visible = (show_all || (st != db::NetlistCrossReference::Match && (with_warnings || st != db::NetlistCrossReference::MatchWithWarning)));
    view->setRowHidden (int (i), parent, ! visible);

    if (visible && levels > 1) {
      show_or_hide_items (view, idx, show_all, with_warnings, levels - 1);
    }

  }
}

void
NetlistBrowserModel::set_item_visibility (QTreeView *view, bool show_all, bool with_warnings)
{
  //  TODO: this implementation is based on the model but is fairly inefficient
  show_or_hide_items (view, QModelIndex (), show_all, with_warnings, 3);
}

}

#endif
