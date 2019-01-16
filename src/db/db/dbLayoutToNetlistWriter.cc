
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

#include "dbLayoutToNetlistWriter.h"
#include "dbLayoutToNetlist.h"

namespace db
{


// -------------------------------------------------------------------------------------------
//  LayoutToNetlistStandardWriter implementation

/**
 *  Comments are introduced by hash: # ...
 *  Names are words (alphanumerical plus "$", "_", ".") or enclosed in single or double quotes.
 *  Escape character is backslash.
 *  Separator is either , or whitespace. Keywords and names are case sensitive.
 *  Short keys are provided for compacter representation. Short keys can be
 *  non-alpha (e.g. "*") or empty.
 *  Single-valued attributes can be given without brackets.
 *  All dimensions are in units of database unit.
 *  The file follows the declaration-before-use principle
 *  (circuits before subcircuits, nets before use ...)
 *
 *  Global statements:
 *
 *    version(<number>)             - file format version
 *    description(<text>)           - an arbitrary description text
 *    unit(<unit>)                  - specifies the database unit [short key: U]
 *    top(<circuit>)                - specifies the name of the top circuit [short key: T]
 *    layer(<name>)                 - define a layer [short key: L]
 *    connect(<layer1> <name> ...)  - connects layer1 with the following layers [short key: C]
 *    global(<layer> <net> ...)     - connects a layer with the given global nets [short key: G]
 *    circuit(<name> [circuit-def]) - circuit (cell) [short key: X]
 *    device(<name> <class> [device-abstract-def])
 *                                  - device abstract [short key: D]
 *
 *  [circuit-def]:
 *
 *    net(<name> [geometry-def])    - net geometry [short key: N]
 *                                    A net declaration shall be there also if no geometry
 *                                    is present
 *    pin(<name> <net-name>)        - outgoing pin connection [short key: P]
 *    device(<name> <class> [device-def])
 *                                  - device with connections [short key: D]
 *    subcircuit(<name> [subcircuit-def])
 *                                  - subcircuit with connections [short key: X]
 *
 *  [geometry-def]:
 *
 *    polygon(<layer> <x> <y> ...)  - defines a polygon [short key: P]
 *    rect(<layer> <left> <bottom> <right> <top>)
 *                                  - defines a rectangle [short key: R]
 *
 *  [device-abstract-def]:
 *
 *    terminal(<terminal-name> [geometry-def])
 *                                  - specifies the terminal geometry [short key: empty]
 *
 *  [device-def]:
 *
 *    param(<name> <value>)         - defines a parameter [short key P]
 *    abstract(<name>)              - links to a geometrical device abstract on top level [short key A]
 *    terminal(<terminal-name> <net-name>)
 *                                  - specifies connection of the terminal with
 *                                    a net (short key: empty)
 *    location(<x> <y>)             - location of the device [short key L]
 *
 *  [subcircuit-def]:
 *
 *    location(<x> <y>)             - location of the subcircuit [short key L]
 *    rotation(<angle>)             - rotation angle [short key O]
 *    mirror                        - if specified, the instance is mirrored before rotation [short key M]
 *    scale(<mag>)                  - magnification [short key *]
 *    pin(<pin-name> <net-name>)    - specifies connection of the pin with a net [short key: P]
 */

static std::string version_key ("version");
static std::string description_key ("description");
static std::string top_key ("top");
static std::string unit_key ("unit");
static std::string layer_key ("layer");
static std::string text_key ("text");
static std::string connect_key ("connect");
static std::string global_key ("global");
static std::string circuit_key ("circuit");
static std::string net_key ("net");
static std::string device_key ("device");
static std::string subcircuit_key ("subcircuit");
static std::string polygon_key ("polygon");
static std::string rect_key ("rect");
static std::string terminal_key ("terminal");
static std::string abstract_key ("abstract");
static std::string label_key ("label");
static std::string param_key ("param");
static std::string location_key ("location");
static std::string rotation_key ("rotation");
static std::string mirror_key ("mirror");
static std::string scale_key ("scale");
static std::string pin_key ("pin");
static std::string indent1 (" ");
static std::string indent2 ("  ");
static std::string endl ("\n");

LayoutToNetlistStandardWriter::LayoutToNetlistStandardWriter (tl::OutputStream &stream)
  : mp_stream (&stream)
{
  //  .. nothing yet ..
}

static std::string name_for_layer (const db::Layout *layout, unsigned int l)
{
  const db::LayerProperties &lp = layout->get_properties (l);
  if (lp.is_named ()) {
    return tl::to_word_or_quoted_string (lp.name);
  } else {
    return "L" + tl::to_string (l);
  }
}

void LayoutToNetlistStandardWriter::write (const db::LayoutToNetlist *l2n)
{
  bool any = false;

  const int version = 0;

  const db::Layout *ly = l2n->internal_layout ();
  const db::Netlist *nl = l2n->netlist ();

  *mp_stream << "# General section" << endl;
  *mp_stream << "# Lists general definitions." << endl << endl;
  if (version > 0) {
    *mp_stream << version_key << "(" << version << ")" << endl;
  }
  *mp_stream << top_key << "(" << tl::to_word_or_quoted_string (ly->cell_name (l2n->internal_top_cell ()->cell_index ())) << ")" << endl;
  *mp_stream << unit_key << "(" << ly->dbu () << ")" << endl;

  *mp_stream << endl << "# Layer section" << endl;
  *mp_stream << "# This section lists the mask layers (drawing or derived) and their connections." << endl;

  *mp_stream << endl << "# Mask layers" << endl;
  for (db::Connectivity::layer_iterator l = l2n->connectivity ().begin_layers (); l != l2n->connectivity ().end_layers (); ++l) {
    *mp_stream << layer_key << "(" << name_for_layer (ly, *l) << ")" << endl;
  }

  *mp_stream << endl << "# Mask layer connectivity" << endl;
  for (db::Connectivity::layer_iterator l = l2n->connectivity ().begin_layers (); l != l2n->connectivity ().end_layers (); ++l) {

    db::Connectivity::layer_iterator ce = l2n->connectivity ().end_connected (*l);
    db::Connectivity::layer_iterator cb = l2n->connectivity ().begin_connected (*l);
    if (cb != ce) {
      *mp_stream << connect_key << "(" << name_for_layer (ly, *l);
      for (db::Connectivity::layer_iterator c = l2n->connectivity ().begin_connected (*l); c != ce; ++c) {
        *mp_stream << " " << name_for_layer (ly, *c);
      }
      *mp_stream << ")" << endl;
    }

  }

  any = false;
  for (db::Connectivity::layer_iterator l = l2n->connectivity ().begin_layers (); l != l2n->connectivity ().end_layers (); ++l) {

    db::Connectivity::global_nets_iterator ge = l2n->connectivity ().end_global_connections (*l);
    db::Connectivity::global_nets_iterator gb = l2n->connectivity ().begin_global_connections (*l);
    if (gb != ge) {
      if (! any) {
        *mp_stream << endl << "# Global nets and connectivity" << endl;
        any = true;
      }
      *mp_stream << global_key << "(" << name_for_layer (ly, *l);
      for (db::Connectivity::global_nets_iterator g = gb; g != ge; ++g) {
        *mp_stream << " " << tl::to_word_or_quoted_string (l2n->connectivity ().global_net_name (*g));
      }
      *mp_stream << ")" << endl;
    }

  }

  if (nl->begin_device_models () != nl->end_device_models ()) {
    *mp_stream << endl << "# Device abstracts section" << endl;
    *mp_stream << "# Device abstracts list the pin shapes of the devices." << endl;
  }
  for (db::Netlist::const_device_model_iterator m = nl->begin_device_models (); m != nl->end_device_models (); ++m) {
    if (m->device_class ()) {
      *mp_stream << device_key << "(" << tl::to_word_or_quoted_string (m->name ()) << " " << tl::to_word_or_quoted_string (m->device_class ()->name ()) << endl;
      write (l2n, *m);
      *mp_stream << ")" << endl;
    }
  }

  *mp_stream << endl << "# Circuit section" << endl;
  *mp_stream << "# Circuits are the hierarchical building blocks of the netlist." << endl;
  for (db::Netlist::const_top_down_circuit_iterator i = nl->begin_top_down (); i != nl->end_top_down (); ++i) {
    const db::Circuit *x = *i;
    *mp_stream << endl << "# Circuit " << x->name () << endl;
    *mp_stream << circuit_key << "(" << tl::to_word_or_quoted_string (x->name ()) << endl;
    write (l2n, *x);
    *mp_stream << ")" << endl;
  }
}

void LayoutToNetlistStandardWriter::write (const db::LayoutToNetlist *l2n, const db::Circuit &circuit)
{
  if (circuit.begin_nets () != circuit.end_nets ()) {
    *mp_stream << endl << indent1 << "# Nets with their geometries" << endl;
    for (db::Circuit::const_net_iterator n = circuit.begin_nets (); n != circuit.end_nets (); ++n) {
      write (l2n, *n);
    }
  }

  if (circuit.begin_pins () != circuit.end_pins ()) {
    *mp_stream << endl << indent1 << "# Outgoing pins and their connections to nets" << endl;
    for (db::Circuit::const_pin_iterator p = circuit.begin_pins (); p != circuit.end_pins (); ++p) {
      const db::Net *net = circuit.net_for_pin (p->id ());
      if (net) {
        *mp_stream << indent1 << pin_key << "(" << tl::to_word_or_quoted_string (p->expanded_name ()) << " " << tl::to_word_or_quoted_string (net->expanded_name ()) << ")" << endl;
      }
    }
  }

  if (circuit.begin_devices () != circuit.end_devices ()) {
    *mp_stream << endl << indent1 << "# Devices and their connections" << endl;
    for (db::Circuit::const_device_iterator d = circuit.begin_devices (); d != circuit.end_devices (); ++d) {
      write (l2n, *d);
    }
  }

  if (circuit.begin_subcircuits () != circuit.end_subcircuits ()) {
    *mp_stream << endl << indent1 << "# Subcircuits and their connections" << endl;
    for (db::Circuit::const_subcircuit_iterator x = circuit.begin_subcircuits (); x != circuit.end_subcircuits (); ++x) {
      write (l2n, *x);
    }
  }

  *mp_stream << endl;
}

template <class T, class Tr>
void write_points (tl::OutputStream &stream, const T &poly, const Tr &tr)
{
  for (typename T::polygon_contour_iterator c = poly.begin_hull (); c != poly.end_hull (); ++c) {
    typename T::point_type pt = tr * *c;
    stream << " " << pt.x () << " " << pt.y ();
  }
}

void LayoutToNetlistStandardWriter::write (const db::PolygonRef *s, const db::ICplxTrans &tr, const std::string &lname)
{
  db::ICplxTrans t = tr * db::ICplxTrans (s->trans ());

  const db::Polygon &poly = s->obj ();
  if (poly.is_box ()) {

    db::Box box = t * poly.box ();
    *mp_stream << rect_key << "(" << lname;
    *mp_stream << " " << box.left () << " " << box.bottom ();
    *mp_stream << " " << box.right () << " " << box.top ();
    *mp_stream << ")";

  } else {

    *mp_stream << polygon_key << "(" << lname;
    if (poly.holes () > 0) {
      db::SimplePolygon sp (poly);
      write_points (*mp_stream, sp, t);
    } else {
      write_points (*mp_stream, poly, t);
    }
    *mp_stream << ")";

  }
}

void LayoutToNetlistStandardWriter::write (const db::LayoutToNetlist *l2n, const db::Net &net)
{
  const db::Layout *ly = l2n->internal_layout ();
  const db::hier_clusters<db::PolygonRef> &clusters = l2n->net_clusters ();
  const db::Circuit *circuit = net.circuit ();
  const db::Connectivity &conn = l2n->connectivity ();

  bool any = false;

  for (db::Connectivity::layer_iterator l = conn.begin_layers (); l != conn.end_layers (); ++l) {

    db::cell_index_type cci = circuit->cell_index ();
    db::cell_index_type prev_ci = cci;

    for (db::recursive_cluster_shape_iterator<db::PolygonRef> si (clusters, *l, cci, net.cluster_id ()); ! si.at_end (); ) {

      //  NOTE: we don't recursive into circuits which will later be output. However, as circuits may
      //  vanish in "purge" but the clusters will still be there we need to recursive into clusters from
      //  unknown cells.
      db::cell_index_type ci = si.cell_index ();
      if (ci != prev_ci && ci != cci && l2n->netlist ()->circuit_by_cell_index (ci)) {

        si.skip_cell ();

      } else {

        if (! any) {
          *mp_stream << indent1 << net_key << "(" << tl::to_word_or_quoted_string (net.expanded_name ()) << endl;
          any = true;
        }

        *mp_stream << indent2;
        write (si.operator-> (), si.trans (), name_for_layer (ly, *l));
        *mp_stream << endl;

        prev_ci = ci;

        ++si;

      }

    }

  }

  if (any) {
    *mp_stream << indent1 << ")" << endl;
  } else {
    *mp_stream << indent1 << net_key << "(" << tl::to_word_or_quoted_string (net.expanded_name ()) << ")" << endl;
  }
}

void LayoutToNetlistStandardWriter::write (const db::LayoutToNetlist *l2n, const db::SubCircuit &subcircuit)
{
  const db::Layout *ly = l2n->internal_layout ();
  double dbu = ly->dbu ();

  *mp_stream << indent1 << subcircuit_key << "(" << tl::to_word_or_quoted_string (subcircuit.expanded_name ());

  const db::DCplxTrans &tr = subcircuit.trans ();
  if (tr.is_mag ()) {
    *mp_stream << " " << scale_key << "(" << tr.mag () << ")";
  }
  if (tr.is_mirror ()) {
    *mp_stream << " " << mirror_key;
  }
  if (fabs (tr.angle ()) > 1e-6) {
    *mp_stream << " " << rotation_key << "(" << tr.angle () << ")";
  }
  *mp_stream << " " << location_key << "(" << tr.disp ().x () / dbu << " " << tr.disp ().y () / dbu << ")";

  //  each pin in one line for more than a few pins
  bool separate_lines = (subcircuit.circuit_ref ()->pin_count () > 1);

  if (separate_lines) {
    *mp_stream << endl;
  }

  for (db::Circuit::const_pin_iterator p = subcircuit.circuit_ref ()->begin_pins (); p != subcircuit.circuit_ref ()->end_pins (); ++p) {
    const db::Net *net = subcircuit.net_for_pin (p->id ());
    if (net) {
      if (separate_lines) {
        *mp_stream << indent2;
      } else {
        *mp_stream << " ";
      }
      *mp_stream << pin_key << "(" << tl::to_word_or_quoted_string (p->expanded_name ()) << " " << tl::to_word_or_quoted_string (net->expanded_name ()) << ")";
      if (separate_lines) {
        *mp_stream << endl;
      }
    }
  }

  if (separate_lines) {
    *mp_stream << indent1;
  }

  *mp_stream << ")" << endl;
}

void LayoutToNetlistStandardWriter::write (const db::LayoutToNetlist *l2n, const db::DeviceModel &device_model)
{
  const std::vector<db::DeviceTerminalDefinition> &td = device_model.device_class ()->terminal_definitions ();

  const db::Layout *ly = l2n->internal_layout ();
  const db::hier_clusters<db::PolygonRef> &clusters = l2n->net_clusters ();
  const db::Connectivity &conn = l2n->connectivity ();

  for (std::vector<db::DeviceTerminalDefinition>::const_iterator t = td.begin (); t != td.end (); ++t) {

    *mp_stream << indent1 << terminal_key << "(" << t->name () << endl;

    for (db::Connectivity::layer_iterator l = conn.begin_layers (); l != conn.end_layers (); ++l) {

      const db::local_cluster<db::PolygonRef> &lc = clusters.clusters_per_cell (device_model.cell_index ()).cluster_by_id (device_model.cluster_id_for_terminal (t->id ()));
      for (db::local_cluster<db::PolygonRef>::shape_iterator s = lc.begin (*l); ! s.at_end (); ++s) {

        *mp_stream << indent2;
        write (s.operator-> (), db::ICplxTrans (), name_for_layer (ly, *l));
        *mp_stream << endl;

      }

    }

    *mp_stream << indent1 << ")" << endl;

  }
}

void LayoutToNetlistStandardWriter::write (const db::LayoutToNetlist *l2n, const db::Device &device)
{
  const db::Layout *ly = l2n->internal_layout ();
  double dbu = ly->dbu ();

  *mp_stream << indent1 << device_key << "(" << tl::to_word_or_quoted_string (device.expanded_name ());
  *mp_stream << " " << tl::to_word_or_quoted_string (device.device_class ()->name ()) << endl;

  *mp_stream << indent2 << location_key << "(" << device.position ().x () / dbu << " " << device.position ().y () / dbu << ")" << endl;

  if (device.device_model ()) {
    *mp_stream << indent2 << abstract_key << "(" << tl::to_word_or_quoted_string (device.device_model ()->name ()) << ")" << endl;
  }

  const std::vector<DeviceParameterDefinition> &pd = device.device_class ()->parameter_definitions ();
  for (std::vector<DeviceParameterDefinition>::const_iterator i = pd.begin (); i != pd.end (); ++i) {
    *mp_stream << indent2 << param_key << "(" << tl::to_word_or_quoted_string (i->name ()) << " " << device.parameter_value (i->id ()) << ")" << endl;
  }

  const std::vector<DeviceTerminalDefinition> &td = device.device_class ()->terminal_definitions ();
  for (std::vector<DeviceTerminalDefinition>::const_iterator i = td.begin (); i != td.end (); ++i) {
    *mp_stream << indent2 << terminal_key << "(" << tl::to_word_or_quoted_string (i->name ()) << " " << tl::to_word_or_quoted_string (device.net_for_terminal (i->id ())->expanded_name ()) << ")" << endl;
  }

  *mp_stream << indent1 << ")" << endl;
}

}
