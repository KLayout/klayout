
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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

static const std::string endl ("\n");
static const std::string indent1 (" ");

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
//  TokenizedOutput implementation

TokenizedOutput::TokenizedOutput (tl::OutputStream &s)
  : mp_stream (&s), mp_parent (0), m_first (true), m_inline (false), m_newline (false), m_indent (-1)
{
  //  .. nothing yet ..
}

TokenizedOutput::TokenizedOutput (tl::OutputStream &s, const std::string &token)
  : mp_stream (&s), mp_parent (0), m_first (true), m_inline (false), m_newline (false), m_indent (0)
{
  stream () << token << "(";
}

TokenizedOutput::TokenizedOutput (tl::OutputStream &s, int indent, const std::string &token)
  : mp_stream (&s), mp_parent (0), m_first (true), m_inline (false), m_newline (false)
{
  m_indent = indent;
  for (int i = 0; i < m_indent; ++i) {
    stream () << indent1;
  }
  stream () << token << "(";
}

TokenizedOutput::TokenizedOutput (TokenizedOutput &output, const std::string &token, bool inl)
  : mp_stream (&output.stream ()), mp_parent (&output), m_first (true), m_inline (inl), m_newline (false)
{
  m_indent = output.indent () + 1;
  output.emit_sep ();
  stream () << token << "(";
}

TokenizedOutput::~TokenizedOutput ()
{
  if (m_newline) {
    for (int i = 0; i < m_indent; ++i) {
      stream () << indent1;
    }
  }
  if (m_indent >= 0) {
    stream () << ")";
    if (! m_inline) {
      if (mp_parent) {
        *mp_parent << endl;
      } else {
        stream () << endl;
      }
    }
  }
}

void TokenizedOutput::emit_sep ()
{
  if (m_newline) {
    for (int i = 0; i <= m_indent; ++i) {
      stream () << indent1;
    }
    m_newline = false;
  } else if (! m_first) {
    stream () << " ";
  }
  m_first = false;
}

TokenizedOutput &TokenizedOutput::operator<< (const std::string &s)
{
  if (s == endl) {
    m_newline = true;
    stream () << s;
  } else if (! s.empty ()) {
    emit_sep ();
    stream () << s;
  }

  return *this;
}

// -------------------------------------------------------------------------------------------

static void write_point (TokenizedOutput &out, const db::Point &pt, db::Point &ref, bool relative)
{
  if (relative) {

    TokenizedOutput (out, std::string (), true) << tl::to_string (pt.x () - ref.x ()) << tl::to_string (pt.y () - ref.y ());

  } else {

    if (pt.x () == 0 || pt.x () != ref.x ()) {
      out << tl::to_string (pt.x ());
    } else {
      out << "*";
    }

    if (pt.y () == 0 || pt.y () != ref.y ()) {
      out << tl::to_string (pt.y ());
    } else {
      out << "*";
    }

  }

  ref = pt;
}

template <class T, class Tr>
static void write_points (TokenizedOutput &out, const T &poly, const Tr &tr, db::Point &ref, bool relative)
{
  for (typename T::polygon_contour_iterator c = poly.begin_hull (); c != poly.end_hull (); ++c) {
    write_point (out, tr * *c, ref, relative);
  }
}

// -------------------------------------------------------------------------------------------

namespace l2n_std_format
{

template <class Keys>
std_writer_impl<Keys>::std_writer_impl (tl::OutputStream &stream, double dbu, const std::string &progress_description)
  : mp_stream (&stream), m_dbu (dbu), mp_netlist (0),
    m_progress (progress_description.empty () ? tl::to_string (tr ("Writing L2N database")) : progress_description, 10000)
{
  m_progress.set_format (tl::to_string (tr ("%.0f MB")));
  m_progress.set_unit (1024 * 1024);
}

template <class Keys>
std::string std_writer_impl<Keys>::message_to_s (const std::string &msg)
{
  if (msg.empty ()) {
    return std::string ();
  } else {
    return Keys::description_key + "(" + tl::to_word_or_quoted_string (msg) + ")";
  }
}

template <class Keys>
std::string std_writer_impl<Keys>::severity_to_s (const db::Severity severity)
{
  if (severity == db::Info) {
    return Keys::info_severity_key;
  } else if (severity == db::Warning) {
    return Keys::warning_severity_key;
  } else if (severity == db::Error) {
    return Keys::error_severity_key;
  } else {
    return std::string ();
  }
}

template <class Keys>
void std_writer_impl<Keys>::write_log_entry (TokenizedOutput &stream, const LogEntryData &le)
{
  stream << severity_to_s (le.severity ());
  stream << message_to_s (le.message ());

  if (! le.cell_name ().empty ()) {
    TokenizedOutput (stream, Keys::cell_key, true) << tl::to_word_or_quoted_string (le.cell_name ());
  }

  if (! le.category_name ().empty ()) {
    TokenizedOutput o (stream, Keys::cat_key, true);
    o << tl::to_word_or_quoted_string (le.category_name ());
    if (! le.category_description ().empty ()) {
      o << tl::to_word_or_quoted_string (le.category_description ());
    }
  }

  if (le.geometry () != db::DPolygon ()) {
    TokenizedOutput o (stream, Keys::polygon_key, true);
    o << tl::to_word_or_quoted_string (le.geometry ().to_string ());
  }
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

    {
      TokenizedOutput stream (*mp_stream);
      write (false, stream, 0);
    }

    mp_netlist = 0;
    mp_l2n = 0;

  } catch (...) {
    mp_netlist = 0;
    mp_l2n = 0;
    throw;
  }
}

template <class Keys>
void std_writer_impl<Keys>::write (TokenizedOutput &stream, bool nested, const db::Netlist *netlist, const db::LayoutToNetlist *l2n, std::map<const db::Circuit *, std::map<const db::Net *, unsigned int> > *net2id_per_circuit)
{
  try {

    mp_netlist = netlist;
    mp_l2n = l2n;

    write (nested, stream, net2id_per_circuit);

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
void std_writer_impl<Keys>::write_device_class (TokenizedOutput &stream, const db::DeviceClass *cls, const std::string &temp_name, const db::DeviceClass *temp_class)
{
  TokenizedOutput out (stream, Keys::class_key);
  out << tl::to_word_or_quoted_string (cls->name ()) << tl::to_word_or_quoted_string (temp_name);

  bool any_def = false;

  const std::vector<DeviceParameterDefinition> &pd = cls->parameter_definitions ();
  for (std::vector<DeviceParameterDefinition>::const_iterator p = pd.begin (); p != pd.end (); ++p) {
    if (! temp_class->has_parameter_with_name (p->name ()) || !same_parameter (*p, *temp_class->parameter_definition (temp_class->parameter_id_for_name (p->name ())))) {
      if (! any_def) {
        out << endl;
      }
      TokenizedOutput (out, Keys::param_key) << tl::to_word_or_quoted_string (p->name ()) << tl::to_string (p->is_primary () ? 1 : 0) << tl::to_string (p->default_value ());
      any_def = true;
    }
  }

  const std::vector<DeviceTerminalDefinition> &td = cls->terminal_definitions ();
  for (std::vector<DeviceTerminalDefinition>::const_iterator t = td.begin (); t != td.end (); ++t) {
    if (! temp_class->has_terminal_with_name (t->name ())) {
      if (! any_def) {
        out << endl;
      }
      TokenizedOutput (out, Keys::terminal_key) << tl::to_word_or_quoted_string (t->name ());
      any_def = true;
    }
  }
}

template <class Keys>
void std_writer_impl<Keys>::write (bool nested, TokenizedOutput &stream, std::map<const db::Circuit *, std::map<const db::Net *, unsigned int> > *net2id_per_circuit)
{
  const int version = 0;

  const db::Layout *ly = mp_l2n ? mp_l2n->internal_layout () : 0;

  if (! nested) {
    stream << Keys::l2n_magic_string << endl;
  }

  if (version > 0) {
    TokenizedOutput (stream, Keys::version_key) << tl::to_string (version);
    stream << endl;
  }
  if (ly) {
    TokenizedOutput (stream, Keys::top_key) << tl::to_word_or_quoted_string (ly->cell_name (mp_l2n->internal_top_cell ()->cell_index ()));
    TokenizedOutput (stream, Keys::unit_key) << tl::to_string (m_dbu);
  }

  bool any = false;

  if (mp_l2n) {

    if (! Keys::is_short ()) {
      stream << endl << "# Layer section" << endl;
      stream << "# This section lists the mask layers (drawing or derived) and their connections." << endl;
    }

    if (! Keys::is_short ()) {
      stream << endl << "# Mask layers" << endl;
    }
    for (db::Connectivity::layer_iterator l = mp_l2n->connectivity ().begin_layers (); l != mp_l2n->connectivity ().end_layers (); ++l) {
      TokenizedOutput out (stream, Keys::layer_key);
      out << name_for_layer (mp_l2n, *l);
      db::LayerProperties lp = ly->get_properties (*l);
      if (! lp.is_null ()) {
        out << tl::to_word_or_quoted_string (lp.to_string ());
      }
      m_progress.set (mp_stream->pos ());
    }

    if (! Keys::is_short ()) {
      stream << endl << "# Mask layer connectivity" << endl;
    }
    for (db::Connectivity::layer_iterator l = mp_l2n->connectivity ().begin_layers (); l != mp_l2n->connectivity ().end_layers (); ++l) {

      db::Connectivity::layer_iterator ce = mp_l2n->connectivity ().end_connected (*l);
      db::Connectivity::layer_iterator cb = mp_l2n->connectivity ().begin_connected (*l);
      if (cb != ce) {
        TokenizedOutput out (stream, Keys::connect_key);
        out << name_for_layer (mp_l2n, *l);
        for (db::Connectivity::layer_iterator c = mp_l2n->connectivity ().begin_connected (*l); c != ce; ++c) {
          out << name_for_layer (mp_l2n, *c);
        }
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
            stream << endl << "# Global nets and connectivity" << endl;
          }
          any = true;
        }
        TokenizedOutput out (stream, Keys::global_key);
        out << name_for_layer (mp_l2n, *l);
        for (db::Connectivity::global_nets_iterator g = gb; g != ge; ++g) {
          out << tl::to_word_or_quoted_string (mp_l2n->connectivity ().global_net_name (*g));
        }
        m_progress.set (mp_stream->pos ());
      }

    }

    if (! mp_l2n->log_entries ().empty ()) {
      if (! Keys::is_short ()) {
        stream << endl << "# Log entries" << endl;
      }
      for (auto l = mp_l2n->begin_log_entries (); l != mp_l2n->end_log_entries (); ++l) {
        TokenizedOutput out (stream, Keys::message_key);
        this->write_log_entry (out, *l);
        m_progress.set (mp_stream->pos ());
      }
    }

  }

  if (mp_netlist->begin_device_classes () != mp_netlist->end_device_classes () && ! Keys::is_short ()) {
    stream << endl << "# Device class section" << endl;
  }
  for (db::Netlist::const_device_class_iterator c = mp_netlist->begin_device_classes (); c != mp_netlist->end_device_classes (); ++c) {
    db::DeviceClassTemplateBase *temp = db::DeviceClassTemplateBase::is_a (c.operator-> ());
    if (temp) {
      std::unique_ptr<db::DeviceClass> temp_class (temp->create ());
      write_device_class (stream, c.operator-> (), temp->name (), temp_class.get ());
    } else {
      db::DeviceClass empty;
      write_device_class (stream, c.operator-> (), std::string (), &empty);
    }
    m_progress.set (mp_stream->pos ());
  }

  if (mp_netlist->begin_device_abstracts () != mp_netlist->end_device_abstracts () && ! Keys::is_short ()) {
    stream << endl << "# Device abstracts section" << endl;
    stream << "# Device abstracts list the pin shapes of the devices." << endl;
  }
  for (db::Netlist::const_abstract_model_iterator m = mp_netlist->begin_device_abstracts (); m != mp_netlist->end_device_abstracts (); ++m) {
    if (m->device_class ()) {
      TokenizedOutput out (stream, Keys::device_key);
      out << tl::to_word_or_quoted_string (m->name ()) << tl::to_word_or_quoted_string (m->device_class ()->name ()) << endl;
      write (out, *m);
      m_progress.set (mp_stream->pos ());
    }
  }

  if (! Keys::is_short ()) {
    stream << endl << "# Circuit section" << endl;
    stream << "# Circuits are the hierarchical building blocks of the netlist." << endl;
  }
  for (db::Netlist::const_bottom_up_circuit_iterator i = mp_netlist->begin_bottom_up (); i != mp_netlist->end_bottom_up (); ++i) {
    const db::Circuit *x = i.operator-> ();
    TokenizedOutput out (stream, Keys::circuit_key);
    out << tl::to_word_or_quoted_string (x->name ()) << endl;
    write (out, *x, net2id_per_circuit);
    m_progress.set (mp_stream->pos ());
  }
}

template <class Keys>
void std_writer_impl<Keys>::write (TokenizedOutput &stream, const db::Circuit &circuit, std::map<const db::Circuit *, std::map<const db::Net *, unsigned int> > *net2id_per_circuit)
{
  if (circuit.boundary ().vertices () > 0) {

    if (! Keys::is_short ()) {
      stream << endl << "# Circuit boundary" << endl;
    }

    reset_geometry_ref ();

    db::Polygon poly = circuit.boundary ().transformed (db::VCplxTrans (1.0 / m_dbu));
    if (poly.is_box ()) {

      db::Box box = poly.box ();

      TokenizedOutput out (stream, Keys::rect_key);
      write_point (out, box.p1 (), m_ref, true);
      write_point (out, box.p2 (), m_ref, true);

    } else {

      TokenizedOutput out (stream, Keys::polygon_key);
      if (poly.holes () > 0) {
        db::SimplePolygon sp = db::polygon_to_simple_polygon (poly);
        write_points (out, sp, db::UnitTrans (), m_ref, true);
      } else {
        write_points (out, poly, db::UnitTrans (), m_ref, true);
      }

    }

  }

  for (db::NetlistObject::property_iterator p = circuit.begin_properties (); p != circuit.end_properties (); ++p) {
    if (p == circuit.begin_properties() && ! Keys::is_short ()) {
      stream << endl << "# Properties" << endl;
    }
    TokenizedOutput (stream, Keys::property_key) << p->first.to_parsable_string () << p->second.to_parsable_string ();
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
        stream << endl << "# Nets with their geometries" << endl;
      } else {
        stream << endl << "# Nets" << endl;
      }
    }
    for (db::Circuit::const_net_iterator n = circuit.begin_nets (); n != circuit.end_nets (); ++n) {
      write (stream, *n, (*net2id) [n.operator-> ()]);
      m_progress.set (mp_stream->pos ());
    }
  }

  if (circuit.begin_pins () != circuit.end_pins ()) {
    if (! Keys::is_short ()) {
      stream << endl << "# Outgoing pins and their connections to nets" << endl;
    }
    for (db::Circuit::const_pin_iterator p = circuit.begin_pins (); p != circuit.end_pins (); ++p) {
      TokenizedOutput out (stream, Keys::pin_key);
      const db::Net *net = circuit.net_for_pin (p->id ());
      if (net) {
        out << tl::to_string ((*net2id) [net]);
      }
      if (! p->name ().empty ()) {
        TokenizedOutput (out, Keys::name_key, true) << tl::to_word_or_quoted_string (p->name ());
      }
      m_progress.set (mp_stream->pos ());
    }
  }

  if (circuit.begin_devices () != circuit.end_devices ()) {
    if (! Keys::is_short ()) {
      stream << endl << "# Devices and their connections" << endl;
    }
    for (db::Circuit::const_device_iterator d = circuit.begin_devices (); d != circuit.end_devices (); ++d) {
      write (stream, *d, *net2id);
      m_progress.set (mp_stream->pos ());
    }
  }

  if (circuit.begin_subcircuits () != circuit.end_subcircuits ()) {
    if (! Keys::is_short ()) {
      stream << endl << "# Subcircuits and their connections" << endl;
    }
    for (db::Circuit::const_subcircuit_iterator x = circuit.begin_subcircuits (); x != circuit.end_subcircuits (); ++x) {
      write (stream, *x, *net2id);
      m_progress.set (mp_stream->pos ());
    }
  }

  if (! Keys::is_short ()) {
    stream << endl;
  }
}

template <class Keys>
void std_writer_impl<Keys>::reset_geometry_ref ()
{
  m_ref = db::Point ();
}

template <class Keys>
void std_writer_impl<Keys>::write (TokenizedOutput &stream, const db::NetShape *s, const db::ICplxTrans &tr, const std::string &lname, bool relative)
{
  if (s->type () == db::NetShape::Polygon) {

    db::PolygonRef pr = s->polygon_ref ();
    db::ICplxTrans t = tr * db::ICplxTrans (pr.trans ());

    const db::Polygon &poly = pr.obj ();
    if (poly.is_box ()) {

      db::Box box = t * poly.box ();
      TokenizedOutput out (stream, Keys::rect_key);
      out << lname;
      write_point (out, box.p1 (), m_ref, relative);
      write_point (out, box.p2 (), m_ref, relative);

    } else {

      TokenizedOutput out (stream, Keys::polygon_key);
      out << lname;
      if (poly.holes () > 0) {
        db::SimplePolygon sp = db::polygon_to_simple_polygon (poly);
        write_points (out, sp, t, m_ref, relative);
      } else {
        write_points (out, poly, t, m_ref, relative);
      }

    }

  } else if (s->type () == db::NetShape::Text) {

    TokenizedOutput out (stream, Keys::text_key);
    out << lname;

    db::TextRef txtr = s->text_ref ();
    db::ICplxTrans t = tr * db::ICplxTrans (txtr.trans ());

    out << tl::to_word_or_quoted_string (txtr.obj ().string ());

    db::Point pt = t * (db::Point () + txtr.obj ().trans ().disp ());
    write_point (out, pt, m_ref, relative);

  }
}

template <class Keys>
bool std_writer_impl<Keys>::new_cell (cell_index_type ci) const
{
  return ! (mp_netlist->circuit_by_cell_index (ci) || mp_netlist->device_abstract_by_cell_index (ci));
}

template <class Keys>
void std_writer_impl<Keys>::write (TokenizedOutput &stream, const db::Net &net, unsigned int id)
{
  const db::hier_clusters<db::NetShape> &clusters = mp_l2n->net_clusters ();
  const db::Circuit *circuit = net.circuit ();
  const db::Connectivity &conn = mp_l2n->connectivity ();

  std::unique_ptr<TokenizedOutput> outp;

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

          if (! outp) {

            outp.reset (new TokenizedOutput (stream, Keys::net_key));

            *outp << tl::to_string (id);
            if (! net.name ().empty ()) {
              TokenizedOutput (*outp, Keys::name_key, true) << tl::to_word_or_quoted_string (net.name ());
            }

            *outp << endl;

            for (db::NetlistObject::property_iterator p = net.begin_properties (); p != net.end_properties (); ++p) {
              TokenizedOutput (*outp, Keys::property_key) << p->first.to_parsable_string () << p->second.to_parsable_string ();
            }

          }

          write (*outp, si.operator-> (), si.trans (), name_for_layer (mp_l2n, *l), true);
          m_progress.set (mp_stream->pos ());

          prev_ci = ci;

          ++si;

        }

      }

    }

  }

  if (! outp) {

    outp.reset (new TokenizedOutput (stream, Keys::net_key));
    *outp << tl::to_string (id);

    if (! net.name ().empty ()) {
      TokenizedOutput (*outp, Keys::name_key, true) << tl::to_word_or_quoted_string (net.name ());
    }

    if (net.begin_properties () != net.end_properties ()) {
      *outp << endl;
      for (db::NetlistObject::property_iterator p = net.begin_properties (); p != net.end_properties (); ++p) {
        TokenizedOutput (*outp, Keys::property_key) << p->first.to_parsable_string () << p->second.to_parsable_string ();
      }
    }

  }
}

template <class Keys>
void std_writer_impl<Keys>::write (TokenizedOutput &stream, const db::SubCircuit &subcircuit, std::map<const db::Net *, unsigned int> &net2id)
{
  TokenizedOutput out (stream, Keys::circuit_key);
  out << tl::to_string (subcircuit.id ());
  out << tl::to_word_or_quoted_string (subcircuit.circuit_ref ()->name ());

  if (! subcircuit.name ().empty ()) {
    TokenizedOutput (out, Keys::name_key, true) << tl::to_word_or_quoted_string (subcircuit.name ());
  }

  if (mp_l2n) {
    write (out, subcircuit.trans ());
  }

  //  each pin in one line for more than a few pins
  bool separate_lines = (subcircuit.circuit_ref ()->pin_count () > 1) || subcircuit.begin_properties () != subcircuit.end_properties ();

  if (separate_lines) {
    out << endl;
  }

  for (db::NetlistObject::property_iterator p = subcircuit.begin_properties (); p != subcircuit.end_properties (); ++p) {
    TokenizedOutput (out, Keys::property_key, ! separate_lines) << p->first.to_parsable_string () << p->second.to_parsable_string ();
  }

  unsigned int pin_id = 0;
  for (db::Circuit::const_pin_iterator p = subcircuit.circuit_ref ()->begin_pins (); p != subcircuit.circuit_ref ()->end_pins (); ++p, ++pin_id) {
    const db::Net *net = subcircuit.net_for_pin (p->id ());
    if (net) {
      TokenizedOutput (out, Keys::pin_key, ! separate_lines) << tl::to_string (pin_id) << tl::to_string (net2id [net]);
      m_progress.set (mp_stream->pos ());
    }
  }
}

template <class Keys>
void std_writer_impl<Keys>::write (TokenizedOutput &stream, const db::DeviceAbstract &device_abstract)
{
  tl_assert (mp_l2n);

  const std::vector<db::DeviceTerminalDefinition> &td = device_abstract.device_class ()->terminal_definitions ();

  const db::hier_clusters<db::NetShape> &clusters = mp_l2n->net_clusters ();
  const db::Connectivity &conn = mp_l2n->connectivity ();

  for (std::vector<db::DeviceTerminalDefinition>::const_iterator t = td.begin (); t != td.end (); ++t) {

    TokenizedOutput out (stream, Keys::terminal_key);
    out << t->name ();

    reset_geometry_ref ();

    bool any = false;

    for (db::Connectivity::layer_iterator l = conn.begin_layers (); l != conn.end_layers (); ++l) {

      size_t cid = device_abstract.cluster_id_for_terminal (t->id ());
      if (cid == 0) {
        //  no geometry
        continue;
      }

      const db::local_cluster<db::NetShape> &lc = clusters.clusters_per_cell (device_abstract.cell_index ()).cluster_by_id (cid);
      for (db::local_cluster<db::NetShape>::shape_iterator s = lc.begin (*l); ! s.at_end (); ++s) {

        if (! any) {
          out << endl;
        }

        write (out, s.operator-> (), db::ICplxTrans (), name_for_layer (mp_l2n, *l), true);
        m_progress.set (mp_stream->pos ());

        any = true;

      }

    }

    m_progress.set (mp_stream->pos ());

  }
}

template <class Keys>
void std_writer_impl<Keys>::write (TokenizedOutput &stream, const db::DCplxTrans &tr)
{
  if (tr.is_mag ()) {
    TokenizedOutput (stream, Keys::scale_key, true) << tl::to_string (tr.mag ());
  }

  if (tr.is_mirror ()) {
    stream << Keys::mirror_key;
  }

  if (fabs (tr.angle ()) > 1e-6) {
    TokenizedOutput (stream, Keys::rotation_key, true) << tl::to_string (tr.angle ());
  }

  TokenizedOutput (stream, Keys::location_key, true) << tl::to_string (floor (0.5 + tr.disp ().x () / m_dbu)) << tl::to_string (floor (0.5 + tr.disp ().y () / m_dbu));
}

template <class Keys>
void std_writer_impl<Keys>::write (TokenizedOutput &stream, const db::Device &device, std::map<const Net *, unsigned int> &net2id)
{
  tl_assert (device.device_class () != 0);
  const std::vector<DeviceTerminalDefinition> &td = device.device_class ()->terminal_definitions ();
  const std::vector<DeviceParameterDefinition> &pd = device.device_class ()->parameter_definitions ();

  TokenizedOutput out (stream, Keys::device_key);
  out << tl::to_string (device.id ());

  if (device.device_abstract ()) {

    out << tl::to_word_or_quoted_string (device.device_abstract ()->name ()) << endl;

    const std::vector<db::DeviceAbstractRef> &other_abstracts = device.other_abstracts ();
    for (std::vector<db::DeviceAbstractRef>::const_iterator a = other_abstracts.begin (); a != other_abstracts.end (); ++a) {

      TokenizedOutput o (out, Keys::device_key);
      o << tl::to_word_or_quoted_string (a->device_abstract->name ());
      write (o, a->trans);

    }

    const std::map<unsigned int, std::vector<db::DeviceReconnectedTerminal> > &reconnected_terminals = device.reconnected_terminals ();
    for (std::map<unsigned int, std::vector<db::DeviceReconnectedTerminal> >::const_iterator t = reconnected_terminals.begin (); t != reconnected_terminals.end (); ++t) {

      for (std::vector<db::DeviceReconnectedTerminal>::const_iterator c = t->second.begin (); c != t->second.end (); ++c) {
        TokenizedOutput (out, Keys::connect_key) << tl::to_string (c->device_index) << tl::to_word_or_quoted_string (td [t->first].name ()) << tl::to_word_or_quoted_string (td [c->other_terminal_id].name ());
      }

    }

    write (out, device.trans ());
    out << endl;

  } else {
    out << tl::to_word_or_quoted_string (device.device_class ()->name ()) << endl;
  }

  if (! device.name ().empty ()) {
    TokenizedOutput (out, Keys::name_key) << tl::to_word_or_quoted_string (device.name ());
  }

  for (db::NetlistObject::property_iterator p = device.begin_properties (); p != device.end_properties (); ++p) {
    TokenizedOutput (out, Keys::property_key) << p->first.to_parsable_string () << p->second.to_parsable_string ();
  }

  for (std::vector<DeviceParameterDefinition>::const_iterator i = pd.begin (); i != pd.end (); ++i) {
    TokenizedOutput (out, Keys::param_key) << tl::to_word_or_quoted_string (i->name ()) << tl::sprintf ("%.12g", device.parameter_value (i->id ()));
  }

  for (std::vector<DeviceTerminalDefinition>::const_iterator i = td.begin (); i != td.end (); ++i) {
    const db::Net *net = device.net_for_terminal (i->id ());
    TokenizedOutput o (out, Keys::terminal_key);
    o << tl::to_word_or_quoted_string (i->name ());
    if (net) {
      o << tl::to_string (net2id [net]);
    }
  }
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
