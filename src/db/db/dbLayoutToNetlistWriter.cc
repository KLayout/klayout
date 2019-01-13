
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
 *
 *  Global statements:
 *
 *    description(<text>)           - an arbitrary description text
 *    dbu(<dbu>)                    - specifies the database unit [short key: D]
 *    top(<circuit>)                - specifies the name of the top circuit [short key: T]
 *    layer(<name>)                 - define a layer [short key: L]
 *    connect(<layer1> <name> ...)  - connects layer1 with the following layers [short key: C]
 *    global(<layer> <net> ...)     - connects a layer with the given global nets [short key: G]
 *    circuit(<name> ...)           - defines a circuit (cell) [short key: X]
 *
 *  Inside the circuit:
 *
 *    net(<name> ...)               - specifies net geometry [short key: N]
 *    device(<name> <class> ...)    - defines a device [short key: D]
 *    subcircuit(<name> ...)        - defines a subcircuit [short key: X]
 *
 *  Inside a net:
 *
 *    polygon(<layer> <x> <y> ...)  - defines a polygon [short key: P]
 *    rect(<layer> <left> <bottom> <right> <top>)
 *                                  - defines a rectangle [short key: R]
 *
 *  Inside a device:
 *
 *    param(<name> <value>)         - defines a parameter [short key P]
 *    terminal(<terminal-name> <net-name>)
 *                                  - specifies connection of the terminal with
 *                                    a net (short key: empty)
 *    location(<x> <y>)             - location of the device [short key L]
 *
 *  Inside a subcircuit:
 *
 *    location(<x> <y>)             - location of the subcircuit [short key L]
 *    rotation(<angle>)             - rotation angle [short key O]
 *    mirror                        - if specified, the instance is mirrored before rotation [short key M]
 *    scale(<mag>)                  - magnification [short key *]
 *    pin(<pin-name> <net-name>)    - specifies connection of the pin with a net
 */

static std::string description_key ("description");
static std::string top_key ("top");
static std::string dbu_key ("dbu");
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
  const db::Layout *ly = l2n->internal_layout ();
  const db::Netlist *nl = l2n->netlist ();

  *mp_stream << top_key << "(" << tl::to_word_or_quoted_string (ly->cell_name (l2n->internal_top_cell ()->cell_index ())) << ")" << endl;
  *mp_stream << dbu_key << "(" << ly->dbu () << ")" << endl;

  for (db::Connectivity::layer_iterator l = l2n->connectivity ().begin_layers (); l != l2n->connectivity ().end_layers (); ++l) {

    *mp_stream << layer_key << "(" << name_for_layer (ly, *l) << ")" << endl;

    db::Connectivity::layer_iterator ce = l2n->connectivity ().end_connected (*l);
    db::Connectivity::layer_iterator cb = l2n->connectivity ().begin_connected (*l);
    if (cb != ce) {
      *mp_stream << connect_key << "(" << name_for_layer (ly, *l);
      for (db::Connectivity::layer_iterator c = l2n->connectivity ().begin_connected (*l); c != ce; ++c) {
        *mp_stream << " " << name_for_layer (ly, *l);
      }
      *mp_stream << ")" << endl;
    }

    db::Connectivity::global_nets_iterator ge = l2n->connectivity ().end_global_connections (*l);
    db::Connectivity::global_nets_iterator gb = l2n->connectivity ().begin_global_connections (*l);
    if (gb != ge) {
      *mp_stream << global_key << "(" << name_for_layer (ly, *l);
      for (db::Connectivity::global_nets_iterator g = gb; g != ge; ++g) {
        *mp_stream << " " << tl::to_word_or_quoted_string (l2n->connectivity ().global_net_name (*g));
      }
      *mp_stream << ")" << endl;
    }

  }

  for (db::Netlist::const_circuit_iterator x = nl->begin_circuits (); x != nl->end_circuits (); ++x) {
    *mp_stream << circuit_key << "(" << tl::to_word_or_quoted_string (x->name ()) << endl;
    write (l2n, *x);
    *mp_stream << ")" << endl;
  }
}

void LayoutToNetlistStandardWriter::write (const db::LayoutToNetlist *l2n, const db::Circuit &circuit)
{
  for (db::Circuit::const_net_iterator n = circuit.begin_nets (); n != circuit.end_nets (); ++n) {
    write (l2n, *n);
  }

  for (db::Circuit::const_device_iterator d = circuit.begin_devices (); d != circuit.end_devices (); ++d) {
    write (l2n, *d);
  }

  for (db::Circuit::const_subcircuit_iterator x = circuit.begin_subcircuits (); x != circuit.end_subcircuits (); ++x) {
    write (l2n, *x);
  }
}

template <class T, class Tr>
void write_points (tl::OutputStream &stream, const T &poly, const Tr &tr)
{
  for (typename T::polygon_contour_iterator c = poly.begin_hull (); c != poly.end_hull (); ++c) {
    typename T::point_type pt = tr * *c;
    stream << " " << pt.x () << " " << pt.y ();
  }
}

void LayoutToNetlistStandardWriter::write (const db::LayoutToNetlist *l2n, const db::Net &net)
{
  const db::Layout *ly = l2n->internal_layout ();
  const db::hier_clusters<db::PolygonRef> &clusters = l2n->net_clusters ();
  const db::Circuit *circuit = net.circuit ();
  const db::Connectivity &conn = l2n->connectivity ();

  *mp_stream << indent1 << net_key << "(" << tl::to_word_or_quoted_string (net.expanded_name ()) << endl;

  for (db::Connectivity::layer_iterator l = conn.begin_layers (); l != conn.end_layers (); ++l) {

    const db::local_cluster<db::PolygonRef> &lc = clusters.clusters_per_cell (circuit->cell_index ()).cluster_by_id (net.cluster_id ());
    for (db::local_cluster<db::PolygonRef>::shape_iterator s = lc.begin (*l); ! s.at_end (); ++s) {

      *mp_stream << indent2;

      const db::Polygon &poly = s->obj ();
      if (poly.is_box ()) {

        db::Box box = s->trans () * poly.box ();
        *mp_stream << rect_key << "(" << name_for_layer (ly, *l);
        *mp_stream << " " << box.left () << " " << box.bottom ();
        *mp_stream << " " << box.right () << " " << box.top ();
        *mp_stream << ")";

      } else {

        *mp_stream << polygon_key << "(" << name_for_layer (ly, *l);
        if (poly.holes () > 0) {
          db::SimplePolygon sp (poly);
          write_points (*mp_stream, sp, s->trans ());
        } else {
          write_points (*mp_stream, poly, s->trans ());
        }
        *mp_stream << ")";

      }

    }

  }

  *mp_stream << indent1 << ")" << endl;
}

void LayoutToNetlistStandardWriter::write (const db::LayoutToNetlist * /*l2n*/, const db::SubCircuit &subcircuit)
{
  *mp_stream << indent1 << subcircuit_key << "(" << tl::to_word_or_quoted_string (subcircuit.name ());

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
  *mp_stream << " " << location_key << "(" << tr.disp ().x () << " " << tr.disp ().y () << ")";

  //  each pin in one line for more than 16 pins
  bool separate_lines = (subcircuit.circuit ()->pin_count () > 16);

  if (separate_lines) {
    *mp_stream << endl;
  }

  for (db::Circuit::const_pin_iterator p = subcircuit.circuit ()->begin_pins (); p != subcircuit.circuit ()->end_pins (); ++p) {
    if (separate_lines) {
      *mp_stream << indent2;
    } else {
      *mp_stream << " ";
    }
    *mp_stream << pin_key << "(" << tl::to_word_or_quoted_string (p->name ()) << " " << tl::to_word_or_quoted_string (subcircuit.net_for_pin (p->id ())->expanded_name ()) << ")";
    if (separate_lines) {
      *mp_stream << endl;
    }
  }

  if (separate_lines) {
    *mp_stream << indent1;
  }

  *mp_stream << ")" << endl;
}

void LayoutToNetlistStandardWriter::write (const db::LayoutToNetlist * /*l2n*/, const db::Device &device)
{
  //  @@@ TODO: add location

  *mp_stream << indent1 << device_key << "(" << tl::to_word_or_quoted_string (device.name ());
  *mp_stream << " " << tl::to_word_or_quoted_string (device.device_class ()->name ());

  const std::vector<DeviceParameterDefinition> &pd = device.device_class ()->parameter_definitions ();
  for (std::vector<DeviceParameterDefinition>::const_iterator i = pd.begin (); i != pd.end (); ++i) {
    *mp_stream << " " << param_key << "(" << tl::to_word_or_quoted_string (i->name ()) << " " << device.parameter_value (i->id ()) << ")";
  }

  const std::vector<DeviceTerminalDefinition> &td = device.device_class ()->terminal_definitions ();
  for (std::vector<DeviceTerminalDefinition>::const_iterator i = td.begin (); i != td.end (); ++i) {
    *mp_stream << " " << terminal_key << "(" << tl::to_word_or_quoted_string (i->name ()) << " " << tl::to_word_or_quoted_string (device.net_for_terminal (i->id ())->expanded_name ()) << ")";
  }

  *mp_stream << ")" << endl;
}

}
