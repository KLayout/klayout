
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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
#include "dbLayoutToNetlistFormatDefs.h"
#include "dbPolygonTools.h"
#include "tlMath.h"

namespace db
{

// -------------------------------------------------------------------------------------------
//  LayoutToNetlistWriterBase implementation

LayoutToNetlistWriterBase::LayoutToNetlistWriterBase ()
{
  //  .. nothing yet ..
}

LayoutToNetlistWriterBase::~LayoutToNetlistWriterBase ()
{
  //  .. nothing yet ..
}

void LayoutToNetlistWriterBase::write (const db::LayoutToNetlist *l2n)
{
  do_write (l2n);
}

// -------------------------------------------------------------------------------------------

namespace l2n_std_format
{

static const std::string endl ("\n");
static const std::string indent1 (" ");
static const std::string indent2 ("  ");

template <class Keys>
std_writer_impl<Keys>::std_writer_impl (tl::OutputStream &stream, double dbu, const std::string &progress_description)
  : mp_stream (&stream), m_dbu (dbu), mp_netlist (0),
    m_progress (progress_description.empty () ? tl::to_string (tr ("Writing L2N database")) : progress_description, 10000)
{
  m_progress.set_format (tl::to_string (tr ("%.0f MB")));
  m_progress.set_unit (1024 * 1024);
}

static std::string name_for_layer (const db::LayoutToNetlist *l2n, unsigned int l)
{
  std::string n = l2n->name (l);
  if (n.empty ()) {
    n = "L" + tl::to_string (l);
  }
  return n;
}

template <class Keys>
void std_writer_impl<Keys>::write (const db::LayoutToNetlist *l2n)
{
  try {

    mp_netlist = l2n->netlist ();
    mp_l2n = l2n;

    write (false, 0);

    mp_netlist = 0;
    mp_l2n = 0;

  } catch (...) {
    mp_netlist = 0;
    mp_l2n = 0;
    throw;
  }
}

template <class Keys>
void std_writer_impl<Keys>::write (const db::Netlist *netlist, const db::LayoutToNetlist *l2n, bool nested, std::map<const db::Circuit *, std::map<const db::Net *, unsigned int> > *net2id_per_circuit)
{
  try {

    mp_netlist = netlist;
    mp_l2n = l2n;

    write (nested, net2id_per_circuit);

    mp_netlist = 0;
    mp_l2n = 0;

  } catch (...) {
    mp_netlist = 0;
    mp_l2n = 0;
    throw;
  }
}

static bool same_parameter (const DeviceParameterDefinition &a, const DeviceParameterDefinition &b)
{
  if (a.is_primary () != b.is_primary ()) {
    return false;
  }
  if (! tl::equal (a.default_value (), b.default_value ())) {
    return false;
  }
  return true;
}

template <class Keys>
void std_writer_impl<Keys>::write_device_class (const std::string &indent, const db::DeviceClass *cls, const std::string &temp_name, const db::DeviceClass *temp_class)
{
  *mp_stream << indent << Keys::class_key << "(" << tl::to_word_or_quoted_string (cls->name ()) << " " << tl::to_word_or_quoted_string (temp_name);

  bool any_def = false;

  const std::vector<DeviceParameterDefinition> &pd = cls->parameter_definitions ();
  for (std::vector<DeviceParameterDefinition>::const_iterator p = pd.begin (); p != pd.end (); ++p) {
    if (! temp_class->has_parameter_with_name (p->name ()) || !same_parameter (*p, *temp_class->parameter_definition (temp_class->parameter_id_for_name (p->name ())))) {
      *mp_stream << endl << indent << indent1 << Keys::param_key << "(" << tl::to_word_or_quoted_string (p->name ()) << " " << tl::to_string (p->is_primary () ? 1 : 0) << " " << tl::to_string (p->default_value ()) << ")";
      any_def = true;
    }
  }

  const std::vector<DeviceTerminalDefinition> &td = cls->terminal_definitions ();
  for (std::vector<DeviceTerminalDefinition>::const_iterator t = td.begin (); t != td.end (); ++t) {
    if (! temp_class->has_terminal_with_name (t->name ())) {
      *mp_stream << endl << indent << indent1 << Keys::terminal_key << "(" << tl::to_word_or_quoted_string (t->name ()) << ")";
      any_def = true;
    }
  }

  if (any_def) {
    *mp_stream << endl << indent << ")" << endl;
  } else {
    *mp_stream << ")" << endl;
  }
}

template <class Keys>
void std_writer_impl<Keys>::write (bool nested, std::map<const db::Circuit *, std::map<const db::Net *, unsigned int> > *net2id_per_circuit)
{
  bool any = false;

  const int version = 0;

  const db::Layout *ly = mp_l2n ? mp_l2n->internal_layout () : 0;
  const std::string indent (nested ? indent1 : "");

  if (! nested) {
    *mp_stream << Keys::l2n_magic_string << endl;
  }

  if (version > 0) {
    *mp_stream << indent << Keys::version_key << "(" << version << ")" << endl;
  }
  if (ly) {
    *mp_stream << indent << Keys::top_key << "(" << tl::to_word_or_quoted_string (ly->cell_name (mp_l2n->internal_top_cell ()->cell_index ())) << ")" << endl;
    *mp_stream << indent << Keys::unit_key << "(" << m_dbu << ")" << endl;
  }

  if (mp_l2n) {

    if (! Keys::is_short ()) {
      *mp_stream << endl << indent << "# Layer section" << endl;
      *mp_stream << indent << "# This section lists the mask layers (drawing or derived) and their connections." << endl;
    }

    if (! Keys::is_short ()) {
      *mp_stream << endl << indent << "# Mask layers" << endl;
    }
    for (db::Connectivity::layer_iterator l = mp_l2n->connectivity ().begin_layers (); l != mp_l2n->connectivity ().end_layers (); ++l) {
      *mp_stream << indent << Keys::layer_key << "(" << name_for_layer (mp_l2n, *l);
      db::LayerProperties lp = ly->get_properties (*l);
      if (! lp.is_null ()) {
        *mp_stream << " " << tl::to_word_or_quoted_string (lp.to_string ());
      }
      *mp_stream << ")" << endl;
      m_progress.set (mp_stream->pos ());
    }

    if (! Keys::is_short ()) {
      *mp_stream << endl << indent << "# Mask layer connectivity" << endl;
    }
    for (db::Connectivity::layer_iterator l = mp_l2n->connectivity ().begin_layers (); l != mp_l2n->connectivity ().end_layers (); ++l) {

      db::Connectivity::layer_iterator ce = mp_l2n->connectivity ().end_connected (*l);
      db::Connectivity::layer_iterator cb = mp_l2n->connectivity ().begin_connected (*l);
      if (cb != ce) {
        *mp_stream << indent << Keys::connect_key << "(" << name_for_layer (mp_l2n, *l);
        for (db::Connectivity::layer_iterator c = mp_l2n->connectivity ().begin_connected (*l); c != ce; ++c) {
          *mp_stream << " " << name_for_layer (mp_l2n, *c);
        }
        *mp_stream << ")" << endl;
        m_progress.set (mp_stream->pos ());
      }

    }

    any = false;
    for (db::Connectivity::layer_iterator l = mp_l2n->connectivity ().begin_layers (); l != mp_l2n->connectivity ().end_layers (); ++l) {

      db::Connectivity::global_nets_iterator ge = mp_l2n->connectivity ().end_global_connections (*l);
      db::Connectivity::global_nets_iterator gb = mp_l2n->connectivity ().begin_global_connections (*l);
      if (gb != ge) {
        if (! any) {
          if (! Keys::is_short ()) {
            *mp_stream << endl << indent << "# Global nets and connectivity" << endl;
          }
          any = true;
        }
        *mp_stream << indent << Keys::global_key << "(" << name_for_layer (mp_l2n, *l);
        for (db::Connectivity::global_nets_iterator g = gb; g != ge; ++g) {
          *mp_stream << " " << tl::to_word_or_quoted_string (mp_l2n->connectivity ().global_net_name (*g));
        }
        *mp_stream << ")" << endl;
        m_progress.set (mp_stream->pos ());
      }

    }

  }

  if (mp_netlist->begin_device_classes () != mp_netlist->end_device_classes () && ! Keys::is_short ()) {
    *mp_stream << endl << indent << "# Device class section" << endl;
  }
  for (db::Netlist::const_device_class_iterator c = mp_netlist->begin_device_classes (); c != mp_netlist->end_device_classes (); ++c) {
    db::DeviceClassTemplateBase *temp = db::DeviceClassTemplateBase::is_a (c.operator-> ());
    if (temp) {
      std::unique_ptr<db::DeviceClass> temp_class (temp->create ());
      write_device_class (indent, c.operator-> (), temp->name (), temp_class.get ());
    } else {
      db::DeviceClass empty;
      write_device_class (indent, c.operator-> (), std::string (), &empty);
    }
    m_progress.set (mp_stream->pos ());
  }

  if (mp_netlist->begin_device_abstracts () != mp_netlist->end_device_abstracts () && ! Keys::is_short ()) {
    *mp_stream << endl << indent << "# Device abstracts section" << endl;
    *mp_stream << indent << "# Device abstracts list the pin shapes of the devices." << endl;
  }
  for (db::Netlist::const_abstract_model_iterator m = mp_netlist->begin_device_abstracts (); m != mp_netlist->end_device_abstracts (); ++m) {
    if (m->device_class ()) {
      *mp_stream << indent << Keys::device_key << "(" << tl::to_word_or_quoted_string (m->name ()) << " " << tl::to_word_or_quoted_string (m->device_class ()->name ()) << endl;
      write (*m, indent);
      *mp_stream << indent << ")" << endl;
      m_progress.set (mp_stream->pos ());
    }
  }

  if (! Keys::is_short ()) {
    *mp_stream << endl << indent << "# Circuit section" << endl;
    *mp_stream << indent << "# Circuits are the hierarchical building blocks of the netlist." << endl;
  }
  for (db::Netlist::const_bottom_up_circuit_iterator i = mp_netlist->begin_bottom_up (); i != mp_netlist->end_bottom_up (); ++i) {
    const db::Circuit *x = i.operator-> ();
    *mp_stream << indent << Keys::circuit_key << "(" << tl::to_word_or_quoted_string (x->name ()) << endl;
    write (*x, indent, net2id_per_circuit);
    *mp_stream << indent << ")" << endl;
    m_progress.set (mp_stream->pos ());
  }
}

void write_point (tl::OutputStream &stream, const db::Point &pt, db::Point &ref, bool relative)
{
  if (relative) {

    stream << "(";
    stream << pt.x () - ref.x ();
    stream << " ";
    stream << pt.y () - ref.y ();
    stream << ")";

  } else {

    if (pt.x () == 0 || pt.x () != ref.x ()) {
      stream << pt.x ();
    } else {
      stream << "*";
    }

    if (pt.y () == 0 || pt.y () != ref.y ()) {
      stream << pt.y ();
    } else {
      stream << "*";
    }

  }

  ref = pt;
}

template <class T, class Tr>
void write_points (tl::OutputStream &stream, const T &poly, const Tr &tr, db::Point &ref, bool relative)
{
  for (typename T::polygon_contour_iterator c = poly.begin_hull (); c != poly.end_hull (); ++c) {

    typename T::point_type pt = tr * *c;

    stream << " ";
    write_point (stream, pt, ref, relative);

  }
}

template <class Keys>
void std_writer_impl<Keys>::write (const db::Circuit &circuit, const std::string &indent, std::map<const db::Circuit *, std::map<const db::Net *, unsigned int> > *net2id_per_circuit)
{
  if (circuit.boundary ().vertices () > 0) {

    if (! Keys::is_short ()) {
      *mp_stream << endl << indent << indent1 << "# Circuit boundary" << endl;
    }

    reset_geometry_ref ();

    db::Polygon poly = circuit.boundary ().transformed (db::VCplxTrans (1.0 / m_dbu));
    if (poly.is_box ()) {

      db::Box box = poly.box ();
      *mp_stream << indent << indent1 << Keys::rect_key << "(";
      write_point (*mp_stream, box.p1 (), m_ref, true);
      *mp_stream << " ";
      write_point (*mp_stream, box.p2 (), m_ref, true);
      *mp_stream << ")" << endl;

    } else {

      *mp_stream << indent << indent1 << Keys::polygon_key << "(";
      if (poly.holes () > 0) {
        db::SimplePolygon sp = db::polygon_to_simple_polygon (poly);
        write_points (*mp_stream, sp, db::UnitTrans (), m_ref, true);
      } else {
        write_points (*mp_stream, poly, db::UnitTrans (), m_ref, true);
      }
      *mp_stream << ")" << endl;

    }

  }

  for (db::NetlistObject::property_iterator p = circuit.begin_properties (); p != circuit.end_properties (); ++p) {
    if (p == circuit.begin_properties() && ! Keys::is_short ()) {
      *mp_stream << endl << indent << indent1 << "# Properties" << endl;
    }
    *mp_stream << indent << indent1 << Keys::property_key << "(" << p->first.to_parsable_string () << " " << p->second.to_parsable_string () << ")" << endl;
  }

  std::map<const db::Net *, unsigned int> net2id_local;
  std::map<const db::Net *, unsigned int> *net2id = &net2id_local;
  if (net2id_per_circuit) {
    net2id = &(*net2id_per_circuit) [&circuit];
  }

  unsigned int id = 0;
  for (db::Circuit::const_net_iterator n = circuit.begin_nets (); n != circuit.end_nets (); ++n) {
    net2id->insert (std::make_pair (n.operator-> (), ++id));
  }

  if (circuit.begin_nets () != circuit.end_nets ()) {
    if (! Keys::is_short ()) {
      if (mp_l2n) {
        *mp_stream << endl << indent << indent1 << "# Nets with their geometries" << endl;
      } else {
        *mp_stream << endl << indent << indent1 << "# Nets" << endl;
      }
    }
    for (db::Circuit::const_net_iterator n = circuit.begin_nets (); n != circuit.end_nets (); ++n) {
      write (*n, (*net2id) [n.operator-> ()], indent);
      m_progress.set (mp_stream->pos ());
    }
  }

  if (circuit.begin_pins () != circuit.end_pins ()) {
    if (! Keys::is_short ()) {
      *mp_stream << endl << indent << indent1 << "# Outgoing pins and their connections to nets" << endl;
    }
    for (db::Circuit::const_pin_iterator p = circuit.begin_pins (); p != circuit.end_pins (); ++p) {
      *mp_stream << indent << indent1 << Keys::pin_key << "(";
      const db::Net *net = circuit.net_for_pin (p->id ());
      if (net) {
        *mp_stream << (*net2id) [net];
      }
      if (! p->name ().empty ()) {
        if (net) {
          *mp_stream << " ";
        }
        *mp_stream << Keys::name_key << "(" << tl::to_word_or_quoted_string (p->name ()) << ")";
      }
      *mp_stream << ")" << endl;
      m_progress.set (mp_stream->pos ());
    }
  }

  if (circuit.begin_devices () != circuit.end_devices ()) {
    if (! Keys::is_short ()) {
      *mp_stream << endl << indent << indent1 << "# Devices and their connections" << endl;
    }
    for (db::Circuit::const_device_iterator d = circuit.begin_devices (); d != circuit.end_devices (); ++d) {
      write (*d, *net2id, indent);
      m_progress.set (mp_stream->pos ());
    }
  }

  if (circuit.begin_subcircuits () != circuit.end_subcircuits ()) {
    if (! Keys::is_short ()) {
      *mp_stream << endl << indent << indent1 << "# Subcircuits and their connections" << endl;
    }
    for (db::Circuit::const_subcircuit_iterator x = circuit.begin_subcircuits (); x != circuit.end_subcircuits (); ++x) {
      write (*x, *net2id, indent);
      m_progress.set (mp_stream->pos ());
    }
  }

  if (! Keys::is_short ()) {
    *mp_stream << endl;
  }
}

template <class Keys>
void std_writer_impl<Keys>::reset_geometry_ref ()
{
  m_ref = db::Point ();
}

template <class Keys>
void std_writer_impl<Keys>::write (const db::NetShape *s, const db::ICplxTrans &tr, const std::string &lname, bool relative)
{
  if (s->type () == db::NetShape::Polygon) {

    db::PolygonRef pr = s->polygon_ref ();
    db::ICplxTrans t = tr * db::ICplxTrans (pr.trans ());

    const db::Polygon &poly = pr.obj ();
    if (poly.is_box ()) {

      db::Box box = t * poly.box ();
      *mp_stream << Keys::rect_key << "(" << lname;
      *mp_stream << " ";
      write_point (*mp_stream, box.p1 (), m_ref, relative);
      *mp_stream << " ";
      write_point (*mp_stream, box.p2 (), m_ref, relative);
      *mp_stream << ")";

    } else {

      *mp_stream << Keys::polygon_key << "(" << lname;
      if (poly.holes () > 0) {
        db::SimplePolygon sp = db::polygon_to_simple_polygon (poly);
        write_points (*mp_stream, sp, t, m_ref, relative);
      } else {
        write_points (*mp_stream, poly, t, m_ref, relative);
      }
      *mp_stream << ")";

    }

  } else if (s->type () == db::NetShape::Text) {

    *mp_stream << Keys::text_key << "(" << lname;

    db::TextRef txtr = s->text_ref ();
    db::ICplxTrans t = tr * db::ICplxTrans (txtr.trans ());

    *mp_stream << " " << tl::to_word_or_quoted_string (txtr.obj ().string ()) << " ";

    db::Point pt = t * (db::Point () + txtr.obj ().trans ().disp ());
    write_point (*mp_stream, pt, m_ref, relative);

    *mp_stream << ")";

  }
}

template <class Keys>
bool std_writer_impl<Keys>::new_cell (cell_index_type ci) const
{
  return ! (mp_netlist->circuit_by_cell_index (ci) || mp_netlist->device_abstract_by_cell_index (ci));
}

template <class Keys>
void std_writer_impl<Keys>::write (const db::Net &net, unsigned int id, const std::string &indent)
{
  const db::hier_clusters<db::NetShape> &clusters = mp_l2n->net_clusters ();
  const db::Circuit *circuit = net.circuit ();
  const db::Connectivity &conn = mp_l2n->connectivity ();

  bool any = false;

  if (mp_l2n) {

    reset_geometry_ref ();

    for (db::Connectivity::layer_iterator l = conn.begin_layers (); l != conn.end_layers (); ++l) {

      db::cell_index_type cci = circuit->cell_index ();
      db::cell_index_type prev_ci = cci;

      for (db::recursive_cluster_shape_iterator<db::NetShape> si (clusters, *l, cci, net.cluster_id (), this); ! si.at_end (); ) {

        //  NOTE: we don't recursive into circuits which will later be output. However, as circuits may
        //  vanish in "purge" but the clusters will still be there we need to recursive into clusters from
        //  unknown cells.
        db::cell_index_type ci = si.cell_index ();
        if (ci != prev_ci && ci != cci && (mp_netlist->circuit_by_cell_index (ci) || mp_netlist->device_abstract_by_cell_index (ci))) {

          si.skip_cell ();

        } else {

          if (! any) {

            *mp_stream << indent << indent1 << Keys::net_key << "(" << id;
            if (! net.name ().empty ()) {
              *mp_stream << " " << Keys::name_key << "(" << tl::to_word_or_quoted_string (net.name ()) << ")";
            }
            *mp_stream << endl;

            for (db::NetlistObject::property_iterator p = net.begin_properties (); p != net.end_properties (); ++p) {
              *mp_stream << indent << indent2 << Keys::property_key << "(" << p->first.to_parsable_string () << " " << p->second.to_parsable_string () << ")" << endl;
            }

            any = true;

          }

          *mp_stream << indent << indent2;
          write (si.operator-> (), si.trans (), name_for_layer (mp_l2n, *l), true);
          *mp_stream << endl;
          m_progress.set (mp_stream->pos ());

          prev_ci = ci;

          ++si;

        }

      }

    }

  }

  if (any) {
    *mp_stream << indent << indent1 << ")" << endl;
  } else {

    *mp_stream << indent << indent1 << Keys::net_key << "(" << id;
    if (! net.name ().empty ()) {
      *mp_stream << " " << Keys::name_key << "(" << tl::to_word_or_quoted_string (net.name ()) << ")";
    }
    if (net.begin_properties () != net.end_properties ()) {
      *mp_stream << endl;
      for (db::NetlistObject::property_iterator p = net.begin_properties (); p != net.end_properties (); ++p) {
        *mp_stream << indent << indent2 << Keys::property_key << "(" << p->first.to_parsable_string () << " " << p->second.to_parsable_string () << ")" << endl;
      }
      *mp_stream << indent << ")" << endl;
    } else {
      *mp_stream << ")" << endl;
    }

  }
}

template <class Keys>
void std_writer_impl<Keys>::write (const db::SubCircuit &subcircuit, std::map<const db::Net *, unsigned int> &net2id, const std::string &indent)
{
  *mp_stream << indent << indent1 << Keys::circuit_key << "(" << tl::to_string (subcircuit.id ());
  *mp_stream << " " << tl::to_word_or_quoted_string (subcircuit.circuit_ref ()->name ());

  if (! subcircuit.name ().empty ()) {
    *mp_stream << " " << Keys::name_key << "(" << tl::to_word_or_quoted_string (subcircuit.name ()) << ")";
  }

  if (mp_l2n) {
    *mp_stream << " ";
    write (subcircuit.trans ());
  }

  //  each pin in one line for more than a few pins
  bool separate_lines = (subcircuit.circuit_ref ()->pin_count () > 1) || subcircuit.begin_properties () != subcircuit.end_properties ();

  if (separate_lines) {
    *mp_stream << endl;
  }

  for (db::NetlistObject::property_iterator p = subcircuit.begin_properties (); p != subcircuit.end_properties (); ++p) {
    *mp_stream << indent << indent2 << Keys::property_key << "(" << p->first.to_parsable_string () << " " << p->second.to_parsable_string () << ")" << endl;
  }

  unsigned int pin_id = 0;
  for (db::Circuit::const_pin_iterator p = subcircuit.circuit_ref ()->begin_pins (); p != subcircuit.circuit_ref ()->end_pins (); ++p, ++pin_id) {
    const db::Net *net = subcircuit.net_for_pin (p->id ());
    if (net) {
      if (separate_lines) {
        *mp_stream << indent << indent2;
      } else {
        *mp_stream << " ";
      }
      *mp_stream << Keys::pin_key << "(" << tl::to_string (pin_id) << " " << net2id [net] << ")";
      if (separate_lines) {
        *mp_stream << endl;
      }
      m_progress.set (mp_stream->pos ());
    }
  }

  if (separate_lines) {
    *mp_stream << indent << indent1;
  }

  *mp_stream << ")" << endl;
}

template <class Keys>
void std_writer_impl<Keys>::write (const db::DeviceAbstract &device_abstract, const std::string &indent)
{
  const std::vector<db::DeviceTerminalDefinition> &td = device_abstract.device_class ()->terminal_definitions ();

  const db::hier_clusters<db::NetShape> &clusters = mp_l2n->net_clusters ();
  const db::Connectivity &conn = mp_l2n->connectivity ();

  for (std::vector<db::DeviceTerminalDefinition>::const_iterator t = td.begin (); t != td.end (); ++t) {

    *mp_stream << indent << indent1 << Keys::terminal_key << "(" << t->name () << endl;

    reset_geometry_ref ();

    for (db::Connectivity::layer_iterator l = conn.begin_layers (); l != conn.end_layers (); ++l) {

      size_t cid = device_abstract.cluster_id_for_terminal (t->id ());
      if (cid == 0) {
        //  no geometry
        continue;
      }

      const db::local_cluster<db::NetShape> &lc = clusters.clusters_per_cell (device_abstract.cell_index ()).cluster_by_id (cid);
      for (db::local_cluster<db::NetShape>::shape_iterator s = lc.begin (*l); ! s.at_end (); ++s) {

        *mp_stream << indent << indent2;
        write (s.operator-> (), db::ICplxTrans (), name_for_layer (mp_l2n, *l), true);
        *mp_stream << endl;
        m_progress.set (mp_stream->pos ());

      }

    }

    *mp_stream << indent << indent1 << ")" << endl;
    m_progress.set (mp_stream->pos ());

  }
}

template <class Keys>
void std_writer_impl<Keys>::write (const db::DCplxTrans &tr)
{
  bool first = true;

  if (tr.is_mag ()) {
    *mp_stream << Keys::scale_key << "(" << tr.mag () << ")";
    first = false;
  }

  if (tr.is_mirror ()) {
    if (! first) {
      *mp_stream << " ";
    }
    *mp_stream << Keys::mirror_key;
    first = false;
  }

  if (fabs (tr.angle ()) > 1e-6) {
    if (! first) {
      *mp_stream << " ";
    }
    *mp_stream << Keys::rotation_key << "(" << tr.angle () << ")";
    first = false;
  }

  if (! first) {
    *mp_stream << " ";
  }
  *mp_stream << Keys::location_key << "(" << floor (0.5 + tr.disp ().x () / m_dbu) << " " << floor (0.5 + tr.disp ().y () / m_dbu) << ")";
}

template <class Keys>
void std_writer_impl<Keys>::write (const db::Device &device, std::map<const Net *, unsigned int> &net2id, const std::string &indent)
{
  tl_assert (device.device_class () != 0);
  const std::vector<DeviceTerminalDefinition> &td = device.device_class ()->terminal_definitions ();
  const std::vector<DeviceParameterDefinition> &pd = device.device_class ()->parameter_definitions ();

  *mp_stream << indent << indent1 << Keys::device_key << "(" << tl::to_string (device.id ());

  if (device.device_abstract ()) {

    *mp_stream << " " << tl::to_word_or_quoted_string (device.device_abstract ()->name ()) << endl;

    const std::vector<db::DeviceAbstractRef> &other_abstracts = device.other_abstracts ();
    for (std::vector<db::DeviceAbstractRef>::const_iterator a = other_abstracts.begin (); a != other_abstracts.end (); ++a) {

      *mp_stream << indent << indent2 << Keys::device_key << "(" << tl::to_word_or_quoted_string (a->device_abstract->name ()) << " ";
      write (a->trans);
      *mp_stream << ")" << endl;

    }

    const std::map<unsigned int, std::vector<db::DeviceReconnectedTerminal> > &reconnected_terminals = device.reconnected_terminals ();
    for (std::map<unsigned int, std::vector<db::DeviceReconnectedTerminal> >::const_iterator t = reconnected_terminals.begin (); t != reconnected_terminals.end (); ++t) {

      for (std::vector<db::DeviceReconnectedTerminal>::const_iterator c = t->second.begin (); c != t->second.end (); ++c) {
        *mp_stream << indent << indent2 << Keys::connect_key << "(" << c->device_index << " " << tl::to_word_or_quoted_string (td [t->first].name ()) << " " << tl::to_word_or_quoted_string (td [c->other_terminal_id].name ()) << ")" << endl;
      }

    }

    *mp_stream << indent << indent2;
    write (device.trans ());
    *mp_stream << endl;

  } else {
    *mp_stream << " " << tl::to_word_or_quoted_string (device.device_class ()->name ()) << endl;
  }

  if (! device.name ().empty ()) {
    *mp_stream << indent << indent2 << Keys::name_key << "(" << tl::to_word_or_quoted_string (device.name ()) << ")" << endl;
  }

  for (db::NetlistObject::property_iterator p = device.begin_properties (); p != device.end_properties (); ++p) {
    *mp_stream << indent << indent2 << Keys::property_key << "(" << p->first.to_parsable_string () << " " << p->second.to_parsable_string () << ")" << endl;
  }

  for (std::vector<DeviceParameterDefinition>::const_iterator i = pd.begin (); i != pd.end (); ++i) {
    *mp_stream << indent << indent2 << Keys::param_key << "(" << tl::to_word_or_quoted_string (i->name ()) << " " << tl::sprintf ("%.12g", device.parameter_value (i->id ())) << ")" << endl;
  }

  for (std::vector<DeviceTerminalDefinition>::const_iterator i = td.begin (); i != td.end (); ++i) {
    const db::Net *net = device.net_for_terminal (i->id ());
    if (net) {
      *mp_stream << indent << indent2 << Keys::terminal_key << "(" << tl::to_word_or_quoted_string (i->name ()) << " " << net2id [net] << ")" << endl;
    } else {
      *mp_stream << indent << indent2 << Keys::terminal_key << "(" << tl::to_word_or_quoted_string (i->name ()) << ")" << endl;
    }
  }

  *mp_stream << indent << indent1 << ")" << endl;
}

//  explicit instantiation
template class std_writer_impl<l2n_std_format::keys<false> >;
template class std_writer_impl<l2n_std_format::keys<true> >;

}

// -------------------------------------------------------------------------------------------
//  LayoutToNetlistStandardWriter implementation

LayoutToNetlistStandardWriter::LayoutToNetlistStandardWriter (tl::OutputStream &stream, bool short_version)
  : mp_stream (&stream), m_short_version (short_version)
{
  //  .. nothing yet ..
}

void LayoutToNetlistStandardWriter::do_write (const db::LayoutToNetlist *l2n)
{
  if (! l2n->netlist ()) {
    throw tl::Exception (tl::to_string (tr ("Can't write annotated netlist before the netlist has been created")));
  }
  if (! l2n->internal_layout ()) {
    throw tl::Exception (tl::to_string (tr ("Can't write annotated netlist before the layout has been loaded")));
  }

  double dbu = l2n->internal_layout ()->dbu ();

  if (m_short_version) {
    l2n_std_format::std_writer_impl<l2n_std_format::keys<true> > writer (*mp_stream, dbu);
    writer.write (l2n);
  } else {
    l2n_std_format::std_writer_impl<l2n_std_format::keys<false> > writer (*mp_stream, dbu);
    writer.write (l2n);
  }
}

}
