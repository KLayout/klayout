
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

#include "dbNetlistSpiceWriter.h"
#include "dbNetlist.h"
#include "dbNetlistDeviceClasses.h"

#include "tlStream.h"
#include "tlUniqueName.h"
#include "tlTimer.h"
#include "tlLog.h"

#include <sstream>
#include <set>

namespace db
{

static const char *allowed_name_chars = "_.:,!+$/&\\#[]<>";
static const char *not_connect_prefix = "nc_";

// --------------------------------------------------------------------------------

NetlistSpiceWriterDelegate::NetlistSpiceWriterDelegate ()
  : mp_writer (0)
{
  //  .. nothing yet ..
}

NetlistSpiceWriterDelegate::~NetlistSpiceWriterDelegate ()
{
  //  .. nothing yet ..
}

std::string NetlistSpiceWriterDelegate::net_to_string (const db::Net *net) const
{
  tl_assert (mp_writer != 0);
  return mp_writer->net_to_string (net);
}

std::string NetlistSpiceWriterDelegate::format_name (const std::string &name) const
{
  tl_assert (mp_writer != 0);
  return mp_writer->format_name (name);
}

void NetlistSpiceWriterDelegate::emit_line (const std::string &line) const
{
  tl_assert (mp_writer != 0);
  mp_writer->emit_line (line);
}

void NetlistSpiceWriterDelegate::emit_comment (const std::string &comment) const
{
  tl_assert (mp_writer != 0);
  mp_writer->emit_comment (comment);
}

void NetlistSpiceWriterDelegate::attach_writer (NetlistSpiceWriter *writer)
{
  mp_writer = writer;
}

void NetlistSpiceWriterDelegate::write_header () const
{
  //  .. nothing yet ..
}

void NetlistSpiceWriterDelegate::write_device_intro (const db::DeviceClass &) const
{
  //  .. nothing yet ..
}

void NetlistSpiceWriterDelegate::write_device (const db::Device &dev) const
{
  const db::DeviceClass *dc = dev.device_class ();
  const db::DeviceClassCapacitor *cap = dynamic_cast<const db::DeviceClassCapacitor *> (dc);
  const db::DeviceClassCapacitor *cap3 = dynamic_cast<const db::DeviceClassCapacitorWithBulk *> (dc);
  const db::DeviceClassInductor *ind = dynamic_cast<const db::DeviceClassInductor *> (dc);
  const db::DeviceClassResistor *res = dynamic_cast<const db::DeviceClassResistor *> (dc);
  const db::DeviceClassResistor *res3 = dynamic_cast<const db::DeviceClassResistorWithBulk *> (dc);
  const db::DeviceClassDiode *diode = dynamic_cast<const db::DeviceClassDiode *> (dc);
  const db::DeviceClassMOS3Transistor *mos3 = dynamic_cast<const db::DeviceClassMOS3Transistor *> (dc);
  const db::DeviceClassMOS4Transistor *mos4 = dynamic_cast<const db::DeviceClassMOS4Transistor *> (dc);
  const db::DeviceClassBJT3Transistor *bjt3 = dynamic_cast<const db::DeviceClassBJT3Transistor *> (dc);
  const db::DeviceClassBJT3Transistor *bjt4 = dynamic_cast<const db::DeviceClassBJT4Transistor *> (dc);

  std::ostringstream os;

  if (cap || cap3) {

    os << "C";
    os << format_name (dev.expanded_name ());
    os << format_terminals (dev, size_t (3));
    os << " ";
    os << tl::sprintf ("%.12g", dev.parameter_value (db::DeviceClassCapacitor::param_id_C));
    if (! dev.device_class ()->name ().empty ()) {
      os << " ";
      os << format_name (dev.device_class ()->name ());
    }
    os << format_params (dev, db::DeviceClassCapacitor::param_id_C, true);

  } else if (ind) {

    os << "L";
    os << format_name (dev.expanded_name ());
    os << format_terminals (dev, size_t (2));
    os << " ";
    os << tl::sprintf ("%.12g", dev.parameter_value (db::DeviceClassInductor::param_id_L));
    if (! dev.device_class ()->name ().empty ()) {
      os << " ";
      os << format_name (dev.device_class ()->name ());
    }
    os << format_params (dev, db::DeviceClassInductor::param_id_L, true);

  } else if (res || res3) {

    os << "R";
    os << format_name (dev.expanded_name ());
    os << format_terminals (dev, size_t (3));
    os << " ";
    os << tl::sprintf ("%.12g", dev.parameter_value (db::DeviceClassResistor::param_id_R));
    if (! dev.device_class ()->name ().empty ()) {
      os << " ";
      os << format_name (dev.device_class ()->name ());
    }
    os << format_params (dev, db::DeviceClassResistor::param_id_R, true);

  } else if (diode) {

    os << "D";
    os << format_name (dev.expanded_name ());
    os << format_terminals (dev);

    //  Use device class name for the model
    os << " ";
    os << format_name (dev.device_class ()->name ());
    os << format_params (dev);

  } else if (mos3 || mos4) {

    os << "M";
    os << format_name (dev.expanded_name ());

    //  issue #1304
    os << " ";
    os << net_to_string (dev.net_for_terminal (db::DeviceClassMOS3Transistor::terminal_id_D));
    os << " ";
    os << net_to_string (dev.net_for_terminal (db::DeviceClassMOS3Transistor::terminal_id_G));
    os << " ";
    os << net_to_string (dev.net_for_terminal (db::DeviceClassMOS3Transistor::terminal_id_S));
      os << " ";

    if (! mos4) {
      //  we assume for the MOS3 type the bulk is connected to Source
      os << net_to_string (dev.net_for_terminal (db::DeviceClassMOS3Transistor::terminal_id_S));
    } else {
      os << net_to_string (dev.net_for_terminal (db::DeviceClassMOS4Transistor::terminal_id_B));
    }

    //  Use device class name for the model
    os << " ";
    os << format_name (dev.device_class ()->name ());
    os << format_params (dev);

  } else if (bjt3 || bjt4) {

    os << "Q";
    os << format_name (dev.expanded_name ());
    os << format_terminals (dev);

    //  Use device class name for the model
    os << " ";
    os << format_name (dev.device_class ()->name ());
    os << format_params (dev);

  } else {

    //  Write unknown devices as subcircuits (CAUTION: potential name clash)
    os << "XD_" << format_name (dev.expanded_name ());
    os << format_terminals (dev);
    os << " ";
    os << format_name (dev.device_class ()->name ());
    os << " PARAMS:";
    os << format_params (dev);

  }

  emit_line (os.str ());
}

std::string NetlistSpiceWriterDelegate::format_terminals (const db::Device &dev, size_t nmax) const
{
  std::ostringstream os;

  const std::vector<db::DeviceTerminalDefinition> &td = dev.device_class ()->terminal_definitions ();
  size_t n = 0;
  for (std::vector<db::DeviceTerminalDefinition>::const_iterator i = td.begin (); i != td.end () && n < nmax; ++i, ++n) {
    os << " " << net_to_string (dev.net_for_terminal (i->id ()));
  }

  return os.str ();
}

std::string NetlistSpiceWriterDelegate::format_params (const db::Device &dev, size_t without_id, bool only_primary) const
{
  std::ostringstream os;

  const std::vector<db::DeviceParameterDefinition> &pd = dev.device_class ()->parameter_definitions ();
  for (std::vector<db::DeviceParameterDefinition>::const_iterator i = pd.begin (); i != pd.end (); ++i) {
    if (i->id () != without_id && (! only_primary || i->is_primary ())) {
      double sis = i->si_scaling ();
      os << " " << i->name () << "=";
      //  for compatibility
      if (fabs (sis * 1e6 - 1.0) < 1e-10) {
        os << tl::to_string (dev.parameter_value (i->id ())) << "U";
      } else if (fabs (sis * 1e12 - 1.0) < 1e-10) {
        os << tl::to_string (dev.parameter_value (i->id ())) << "P";
      } else {
        os << tl::to_string (dev.parameter_value (i->id ()) * sis);
      }
    }
  }

  return os.str ();
}

// --------------------------------------------------------------------------------

NetlistSpiceWriter::NetlistSpiceWriter (NetlistSpiceWriterDelegate *delegate)
  : mp_netlist (0), mp_stream (0), mp_delegate (delegate), m_next_net_id (0), m_use_net_names (false), m_with_comments (true)
{
  static NetlistSpiceWriterDelegate std_delegate;
  if (! delegate) {
    mp_delegate.reset (&std_delegate);
  }
}

NetlistSpiceWriter::~NetlistSpiceWriter ()
{
  //  .. nothing yet ..
}

void NetlistSpiceWriter::set_use_net_names (bool use_net_names)
{
  m_use_net_names = use_net_names;
}

void NetlistSpiceWriter::set_with_comments (bool with_comments)
{
  m_with_comments = with_comments;
}

void NetlistSpiceWriter::write (tl::OutputStream &stream, const db::Netlist &netlist, const std::string &description)
{
  tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Writing netlist ")) + stream.path ());

  mp_stream = &stream;
  mp_netlist = &netlist;
  mp_delegate->attach_writer (this);

  try {

    do_write (description);

    mp_stream = 0;
    mp_netlist = 0;
    mp_delegate->attach_writer (0);

  } catch (...) {

    mp_stream = 0;
    mp_netlist = 0;
    mp_delegate->attach_writer (0);
    throw;

  }
}

std::string NetlistSpiceWriter::net_to_string (const db::Net *net) const
{
  if (m_use_net_names) {

    if (! net) {

      return std::string (not_connect_prefix) + tl::to_string (++m_next_net_id);

    } else {

      //  Tested with ngspice: this tool likes in net names: . $ ! & \ # + : |  (but not at beginning)
      //  It does not like: , ;
      //  We translate , to | for the net separator

      std::map<const db::Net *, std::string>::const_iterator ni = m_net_to_spice_name.find (net);
      tl_assert (ni != m_net_to_spice_name.end ());

      const std::string &n = ni->second;
      std::string nn;
      nn.reserve (n.size () + 1);
      if (!isalnum (*n.c_str ())) {
        nn += "\\";
      }
      for (const char *cp = n.c_str (); *cp; ++cp) {
        if (! isalnum (*cp) && strchr (allowed_name_chars, *cp) == 0) {
          nn += tl::sprintf ("\\x%02x", (unsigned char) *cp);
        } else if (*cp == ',') {
          nn += "|";
        } else {
          nn += *cp;
        }
      }

      return nn;

    }

  } else {

    std::map<const db::Net *, size_t>::const_iterator n = m_net_to_spice_id.find (net);
    if (! net || n == m_net_to_spice_id.end ()) {
      return tl::to_string (++m_next_net_id);
    } else {
      return tl::to_string (n->second);
    }

  }
}

void NetlistSpiceWriter::emit_line (const std::string &line) const
{
  tl_assert (mp_stream != 0);

  int max_length = 80;
  bool first = true;

  const char *cp = line.c_str ();
  do {

    const char *cpn = cp;
    const char *cspc = 0;
    int c = 0;

    int l = first ? max_length : max_length - 2;
    while (*cpn && (c < l || ! cspc)) {
      if (isspace (*cpn)) {
        cspc = cpn;
      }
      ++c;
      ++cpn;
    }

    if (! first) {
      *mp_stream << "+ ";
    }

    if (! *cpn) {
      *mp_stream << cp << "\n";
      break;
    } else {
      while (*cp && (cp != cspc || ! cspc)) {
        *mp_stream << *cp++;
      }
      *mp_stream << "\n";
    }

    first = false;

    while (*cp && isspace (*cp)) {
      ++cp;
    }

  } while (*cp);
}

void NetlistSpiceWriter::emit_comment (const std::string &comment) const
{
  tl_assert (mp_stream != 0);

  //  TODO: should do some line breaking or reduction for long lines
  //  or when lines contain newlines
  *mp_stream << "* " << comment << "\n";
}

std::string NetlistSpiceWriter::format_name (const std::string &s) const
{
  //  TODO: escape or replace special chars
  return s;
}

void NetlistSpiceWriter::do_write (const std::string &description)
{
  if (! description.empty ()) {
    emit_comment (description);
  }

  mp_delegate->write_header ();

  for (db::Netlist::const_device_class_iterator dc = mp_netlist->begin_device_classes (); dc != mp_netlist->end_device_classes (); ++dc) {
    mp_delegate->write_device_intro (*dc);
  }

  for (db::Netlist::const_top_down_circuit_iterator c = mp_netlist->begin_top_down (); c != mp_netlist->end_top_down (); ++c) {

    const db::Circuit &circuit = *c;

    //  assign internal node numbers to the nets
    m_net_to_spice_id.clear ();
    m_net_to_spice_name.clear ();

    m_next_net_id = 0;
    if (! m_use_net_names) {

      for (db::Circuit::const_net_iterator n = circuit.begin_nets (); n != circuit.end_nets (); ++n) {
        m_net_to_spice_id.insert (std::make_pair (n.operator-> (), ++m_next_net_id));
      }

    } else {

      //  create unique names for those nets with a name
      std::set<std::string> names;
      for (db::Circuit::const_net_iterator n = circuit.begin_nets (); n != circuit.end_nets (); ++n) {
        std::string nn = tl::unique_name (n->expanded_name (), names);
        names.insert (nn);
        m_net_to_spice_name.insert (std::make_pair (n.operator-> (), nn));
      }

      //  determine the next net id for non-connected nets such that there is no clash with
      //  existing names
      size_t prefix_len = strlen (not_connect_prefix);

      for (std::set<std::string>::const_iterator n = names.begin (); n != names.end (); ++n) {
        if (n->find (not_connect_prefix) == 0 && n->size () > prefix_len) {
          size_t num = 0;
          tl::from_string (n->c_str () + prefix_len, num);
          m_next_net_id = std::max (m_next_net_id, num);
        }
      }

    }

    write_circuit_header (circuit);

    for (db::Circuit::const_subcircuit_iterator i = circuit.begin_subcircuits (); i != circuit.end_subcircuits (); ++i) {
      write_subcircuit_call (*i);
    }

    for (db::Circuit::const_device_iterator i = circuit.begin_devices (); i != circuit.end_devices (); ++i) {

      if (m_with_comments) {
        std::string comment = "device instance " + i->expanded_name () + " " + i->trans ().to_string () + " " + i->device_class ()->name ();
        emit_comment (comment);
      }

      mp_delegate->write_device (*i);

    }

    write_circuit_end (circuit);

  }
}

void NetlistSpiceWriter::write_subcircuit_call (const db::SubCircuit &subcircuit) const
{
  if (m_with_comments) {
    std::string comment = "cell instance " + subcircuit.expanded_name() + " " + subcircuit.trans ().to_string ();
    emit_comment (comment);
  }

  std::ostringstream os;
  os << "X";
  os << format_name (subcircuit.expanded_name ());

  for (db::Circuit::const_pin_iterator p = subcircuit.circuit_ref ()->begin_pins (); p != subcircuit.circuit_ref ()->end_pins (); ++p) {
    os << " ";
    os << net_to_string (subcircuit.net_for_pin (p->id ()));
  }

  os << " ";
  os << format_name (subcircuit.circuit_ref ()->name ());

  emit_line (os.str ());
}

void NetlistSpiceWriter::write_circuit_header (const db::Circuit &circuit) const
{
  emit_line ("");

  if (m_with_comments) {
    emit_comment ("cell " + circuit.name ());
    for (db::Circuit::const_pin_iterator p = circuit.begin_pins (); p != circuit.end_pins (); ++p) {
      emit_comment ("pin " + p->name ());
    }
  }

  std::ostringstream os;

  os << ".SUBCKT ";
  os << format_name (circuit.name ());

  for (db::Circuit::const_pin_iterator p = circuit.begin_pins (); p != circuit.end_pins (); ++p) {
    os << " ";
    os << net_to_string (circuit.net_for_pin (p->id ()));
  }

  emit_line (os.str ());

  if (! m_use_net_names && m_with_comments) {
    for (db::Circuit::const_net_iterator n = circuit.begin_nets (); n != circuit.end_nets (); ++n) {
      if (! n->name ().empty ()) {
        emit_comment ("net " + net_to_string (n.operator-> ()) + " " + n->name ());
      }
    }
  }
}

void NetlistSpiceWriter::write_circuit_end (const db::Circuit &circuit) const
{
  emit_line (".ENDS " + format_name (circuit.name ()));
}

}
