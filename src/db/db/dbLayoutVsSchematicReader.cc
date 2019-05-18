
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

#include "dbLayoutVsSchematicReader.h"
#include "dbLayoutVsSchematicFormatDefs.h"

namespace db
{

typedef lvs_std_format::keys<true> skeys;
typedef lvs_std_format::keys<false> lkeys;

LayoutVsSchematicStandardReader::LayoutVsSchematicStandardReader (tl::InputStream &stream)
  : LayoutToNetlistStandardReader (stream)
{
  //  .. nothing yet ..
}

void LayoutVsSchematicStandardReader::read_lvs (db::LayoutVsSchematic *l2n)
{
  try {
    do_read (l2n);
  } catch (tl::Exception &ex) {
    throw tl::Exception (tl::sprintf (tl::to_string (tr ("%s in line: %d of %s")), ex.msg (), stream ().line_number (), path ()));
  }
}

void LayoutVsSchematicStandardReader::do_read (db::LayoutVsSchematic *lvs)
{
  int version = 0;
  std::string description;

  tl_assert (lvs->internal_layout ());
  lvs->internal_layout ()->dbu (1.0); //  mainly for testing

  if (lvs->internal_layout ()->cells () == 0) {
    lvs->internal_layout ()->add_cell ("TOP");
  }
  tl_assert (lvs->internal_top_cell () != 0);

  lvs->make_netlist ();

  while (! at_end ()) {

    if (test (skeys::version_key) || test (lkeys::version_key)) {

      Brace br (this);
      version = read_int ();
      br.done ();

    } else if (test (skeys::description_key) || test (lkeys::description_key)) {

      Brace br (this);
      read_word_or_quoted (description);
      br.done ();

    } else if (test (skeys::layout_key) || test (lkeys::layout_key)) {

      Brace br (this);
      LayoutToNetlistStandardReader::do_read (lvs, true /*nested*/);
      br.done ();

    } else if (test (skeys::reference_key) || test (lkeys::reference_key)) {

      Brace br (this);
      std::auto_ptr<db::Netlist> netlist (new db::Netlist ());
      read_netlist (netlist.get ());
      lvs->set_reference_netlist (netlist.release ());
      br.done ();

    } else if (test (skeys::xref_key) || test (lkeys::xref_key)) {

      if (! lvs->reference_netlist () || ! lvs->netlist ()) {
        throw tl::Exception (tl::to_string (tr ("xref section before reference or layout netlist")));
      }

      db::NetlistCrossReference *xref = lvs->make_cross_ref ();
      xref->gen_begin_netlist (lvs->netlist (), lvs->reference_netlist ());
      read_xref (xref);
      xref->gen_end_netlist (lvs->netlist (), lvs->reference_netlist ());

    } else {
      throw tl::Exception (tl::to_string (tr ("Invalid keyword")));
    }

  }
}

void LayoutVsSchematicStandardReader::read_netlist (db::Netlist *netlist)
{
  Brace br (this);
  while (br) {

    if (test (skeys::circuit_key) || test (lkeys::circuit_key)) {

      Brace br (this);
      std::string name;
      read_word_or_quoted (name);

      db::Circuit *circuit = new db::Circuit ();
      circuit->set_name (name);
      netlist->add_circuit (circuit);

      std::map<unsigned int, Net *> id2net;

      while (br) {

        if (test (skeys::net_key) || test (lkeys::net_key)) {
          read_net (netlist, circuit, id2net);
        } else if (test (skeys::pin_key) || test (lkeys::pin_key)) {
          read_pin (netlist, circuit, id2net);
        } else if (test (skeys::device_key) || test (lkeys::device_key)) {
          read_device (netlist, circuit, id2net);
        } else if (test (skeys::circuit_key) || test (lkeys::circuit_key)) {
          read_subcircuit (netlist, circuit, id2net);
        } else {
          throw tl::Exception (tl::to_string (tr ("Invalid keyword inside circuit definition (net, pin, device or circuit expected)")));
        }

      }
      br.done ();

    }

  }
  br.done ();
}

void
LayoutVsSchematicStandardReader::read_net (db::Netlist * /*netlist*/, db::Circuit *circuit, std::map<unsigned int, Net *> &id2net)
{
  Brace br (this);

  unsigned int id = (unsigned int) read_int ();
  std::string name;

  if (test (skeys::name_key) || test (lkeys::name_key)) {
    Brace br_name (this);
    read_word_or_quoted (name);
    br_name.done ();
  }

  db::Net *net = new db::Net ();
  net->set_name (name);
  circuit->add_net (net);

  id2net.insert (std::make_pair (id, net));

  br.done ();
}

void
LayoutVsSchematicStandardReader::read_pin (db::Netlist * /*netlist*/, db::Circuit *circuit, std::map<unsigned int, Net *> &id2net)
{
  Brace br (this);
  std::string name;
  read_word_or_quoted (name);
  unsigned int netid = (unsigned int) read_int ();
  br.done ();

  const db::Pin &pin = circuit->add_pin (name);

  db::Net *net = id2net [netid];
  if (!net) {
    throw tl::Exception (tl::to_string (tr ("Not a valid net ID: ")) + tl::to_string (netid));
  }

  circuit->connect_pin (pin.id (), net);
}


void
LayoutVsSchematicStandardReader::read_device (db::Netlist *netlist, db::Circuit *circuit, std::map<unsigned int, Net *> &id2net)
{
  Brace br (this);

  std::string name;
  read_word_or_quoted (name);

  std::string dmname;
  read_word_or_quoted (dmname);

  db::DeviceAbstract *dm = device_model_by_name (netlist, dmname);

  db::Device *device = new db::Device ();
  device->set_device_class (const_cast<db::DeviceClass *> (dm->device_class ()));
  device->set_device_abstract (dm);
  device->set_name (name);
  circuit->add_device (device);

  size_t max_tid = 0;

  while (br) {

    if (test (skeys::terminal_key) || test (lkeys::terminal_key)) {

      Brace br2 (this);
      std::string tname;
      read_word_or_quoted (tname);
      unsigned int netid = (unsigned int) read_int ();
      br2.done ();

      size_t tid = terminal_id (dm->device_class (), tname);
      max_tid = std::max (max_tid, tid + 1);

      db::Net *net = id2net [netid];
      if (!net) {
        throw tl::Exception (tl::to_string (tr ("Not a valid net ID: ")) + tl::to_string (netid));
      }

      device->connect_terminal (tid, net);

    } else if (test (skeys::param_key) || test (lkeys::param_key)) {

      Brace br2 (this);
      std::string pname;
      read_word_or_quoted (pname);
      double value = read_double ();
      br2.done ();

      size_t pid = std::numeric_limits<size_t>::max ();
      const std::vector<db::DeviceParameterDefinition> &pd = dm->device_class ()->parameter_definitions ();
      for (std::vector<db::DeviceParameterDefinition>::const_iterator p = pd.begin (); p != pd.end (); ++p) {
        if (p->name () == pname) {
          pid = p->id ();
          break;
        }
      }

      //  if no parameter with this name exists, create one
      if (pid == std::numeric_limits<size_t>::max ()) {
        //  TODO: this should only happen for generic devices
        db::DeviceClass *dc = const_cast<db::DeviceClass *> (dm->device_class ());
        pid = dc->add_parameter_definition (db::DeviceParameterDefinition (pname, std::string ())).id ();
      }

      device->set_parameter_value (pid, value);

    } else {
      throw tl::Exception (tl::to_string (tr ("Invalid keyword inside device definition (location, param or terminal expected)")));
    }

  }

  br.done ();
}

void
LayoutVsSchematicStandardReader::read_subcircuit (db::Netlist *netlist, db::Circuit *circuit, std::map<unsigned int, Net *> &id2net)
{
  Brace br (this);

  std::string name;
  read_word_or_quoted (name);

  std::string xname;
  read_word_or_quoted (xname);

  db::Circuit *circuit_ref = netlist->circuit_by_name (xname);
  if (! circuit_ref) {
    throw tl::Exception (tl::to_string (tr ("Not a valid device circuit name: ")) + xname);
  }

  db::SubCircuit *subcircuit = new db::SubCircuit (circuit_ref);
  subcircuit->set_name (name);
  circuit->add_subcircuit (subcircuit);

  while (br) {

    if (test (skeys::pin_key) || test (lkeys::pin_key)) {

      Brace br2 (this);
      std::string pname;
      read_word_or_quoted (pname);
      unsigned int netid = (unsigned int) read_int ();
      br2.done ();

      const db::Pin *sc_pin = circuit_ref->pin_by_name (pname);
      if (! sc_pin) {
        throw tl::Exception (tl::to_string (tr ("Not a valid pin name: ")) + pname + tl::to_string (tr (" for circuit: ")) + circuit_ref->name ());
      }

      db::Net *net = id2net [netid];
      if (!net) {
        throw tl::Exception (tl::to_string (tr ("Not a valid net ID: ")) + tl::to_string (netid));
      }

      subcircuit->connect_pin (sc_pin->id (), net);

    } else {
      throw tl::Exception (tl::to_string (tr ("Invalid keyword inside subcircuit definition (location, rotation, mirror, scale or pin expected)")));
    }

  }

  br.done ();
}

bool LayoutVsSchematicStandardReader::read_status (db::NetlistCrossReference::Status &status)
{
  if (test (skeys::match_key) || test (lkeys::match_key)) {
    status = db::NetlistCrossReference::Match;
    return true;
  } else if (test (skeys::nomatch_key) || test (lkeys::nomatch_key)) {
    status = db::NetlistCrossReference::NoMatch;
    return true;
  } else if (test (skeys::mismatch_key) || test (lkeys::mismatch_key)) {
    status = db::NetlistCrossReference::Mismatch;
    return true;
  } else if (test (skeys::warning_key) || test (lkeys::warning_key)) {
    status = db::NetlistCrossReference::MatchWithWarning;
    return true;
  } else if (test (skeys::skipped_key) || test (lkeys::skipped_key)) {
    status = db::NetlistCrossReference::Skipped;
    return true;
  } else {
    return false;
  }
}

void LayoutVsSchematicStandardReader::read_xrefs_for_circuits (db::NetlistCrossReference *xref, const db::Circuit *circuit_a, const db::Circuit *circuit_b)
{
  Brace br (this);
  while (br) {

    if (test (skeys::net_key) || test (lkeys::net_key)) {
      read_net_pair (xref, circuit_a, circuit_b);
    } else if (test (skeys::pin_key) || test (lkeys::pin_key)) {
      read_pin_pair (xref, circuit_a, circuit_b);
    } else if (test (skeys::device_key) || test (lkeys::device_key)) {
      read_device_pair (xref, circuit_a, circuit_b);
    } else if (test (skeys::circuit_key) || test (lkeys::circuit_key)) {
      read_subcircuit_pair (xref, circuit_a, circuit_b);
    } else {
      throw tl::Exception (tl::to_string (tr ("Invalid keyword inside circuit definition (net, pin, device or circuit expected)")));
    }

  }
  br.done ();
}

void LayoutVsSchematicStandardReader::read_xref (db::NetlistCrossReference *xref)
{
  Brace br (this);
  while (br) {

    if (test (skeys::circuit_key) || test (lkeys::circuit_key)) {

      Brace br (this);

      std::string name_a, name_b;
      read_word_or_quoted (name_a);
      read_word_or_quoted (name_b);

      const db::Circuit *circuit_a = xref->netlist_a ()->circuit_by_name (name_a);
      if (! circuit_a) {
        throw tl::Exception (tl::to_string (tr ("Not a valid circuit name: ")) + name_a);
      }

      const db::Circuit *circuit_b = xref->netlist_b ()->circuit_by_name (name_b);
      if (! circuit_b) {
        throw tl::Exception (tl::to_string (tr ("Not a valid circuit name: ")) + name_b);
      }

      xref->gen_begin_circuit (circuit_a, circuit_b);

      db::NetlistCrossReference::Status status = db::NetlistCrossReference::None;

      while (br) {

        if (read_status (status)) {
          //  continue
        } else if (test (skeys::xref_key) || test (lkeys::xref_key)) {
          read_xrefs_for_circuits (xref, circuit_a, circuit_b);
        } else {
          throw tl::Exception (tl::to_string (tr ("Invalid keyword inside circuit definition (status keyword of xrefs expected)")));
        }

      }

      xref->gen_end_circuit (circuit_a, circuit_b, status);

      br.done ();

    }

  }
  br.done ();
}

std::pair<std::string, bool> LayoutVsSchematicStandardReader::read_non_string ()
{
  if (test ("(")) {
    expect (")");
    return std::make_pair (std::string (), false);
  } else {
    std::string s;
    read_word_or_quoted (s);
    return std::make_pair (s, true);
  }
}

std::pair<unsigned int, bool> LayoutVsSchematicStandardReader::read_non_numerical ()
{
  if (test ("(")) {
    expect (")");
    return std::make_pair (0, false);
  } else {
    return std::make_pair ((unsigned int) read_int (), true);
  }
}

static const db::Net *net_by_numerical_id (const db::Circuit *circuit, const std::pair<unsigned int, bool> &non)
{
  if (non.second && circuit) {
    const db::Net *net = 0; // @@@ circuit->net_by_numerical_id (non.first);
    if (! net) {
      throw tl::Exception (tl::to_string (tr ("Not a valid net id: ")) + tl::to_string (non.first));
    }
    return net;
  } else {
    return 0;
  }
}

void LayoutVsSchematicStandardReader::read_net_pair (db::NetlistCrossReference *xref, const db::Circuit *circuit_a, const db::Circuit *circuit_b)
{
  Brace br (this);

  std::pair<unsigned int, bool> non_a, non_b;
  non_a = read_non_numerical ();
  non_b = read_non_numerical ();

  db::NetlistCrossReference::Status status = db::NetlistCrossReference::None;
  read_status (status);

  br.done ();

  xref->gen_nets (net_by_numerical_id (circuit_a, non_a), net_by_numerical_id (circuit_b, non_b), status);
}

static const db::Pin *pin_by_name (const db::Circuit *circuit, const std::pair<std::string, bool> &non)
{
  if (non.second && circuit) {
    const db::Pin *pin = circuit->pin_by_name (non.first);
    if (! pin) {
      throw tl::Exception (tl::to_string (tr ("Not a valid pin name: ")) + non.first);
    }
    return pin;
  } else {
    return 0;
  }
}

void LayoutVsSchematicStandardReader::read_pin_pair (db::NetlistCrossReference *xref, const db::Circuit *circuit_a, const db::Circuit *circuit_b)
{
  Brace br (this);

  std::pair<std::string, bool> non_a, non_b;
  non_a = read_non_string ();
  non_b = read_non_string ();

  db::NetlistCrossReference::Status status = db::NetlistCrossReference::None;
  read_status (status);

  br.done ();

  xref->gen_pins (pin_by_name (circuit_a, non_a), pin_by_name (circuit_b, non_b), status);
}

static const db::Device *device_by_name (const db::Circuit *circuit, const std::pair<std::string, bool> &non)
{
  if (non.second && circuit) {
    const db::Device *device = circuit->device_by_name (non.first);
    if (! device) {
      throw tl::Exception (tl::to_string (tr ("Not a valid device name: ")) + non.first);
    }
    return device;
  } else {
    return 0;
  }
}

void LayoutVsSchematicStandardReader::read_device_pair (db::NetlistCrossReference *xref, const db::Circuit *circuit_a, const db::Circuit *circuit_b)
{
  Brace br (this);

  std::pair<std::string, bool> non_a, non_b;
  non_a = read_non_string ();
  non_b = read_non_string ();

  db::NetlistCrossReference::Status status = db::NetlistCrossReference::None;
  read_status (status);

  br.done ();

  xref->gen_devices (device_by_name (circuit_a, non_a), device_by_name (circuit_b, non_b), status);
}

static const db::SubCircuit *subcircuit_by_name (const db::Circuit *circuit, const std::pair<std::string, bool> &non)
{
  if (non.second && circuit) {
    const db::SubCircuit *subcircuit = circuit->subcircuit_by_name (non.first);
    if (! subcircuit) {
      throw tl::Exception (tl::to_string (tr ("Not a valid subcircuit name: ")) + non.first);
    }
    return subcircuit;
  } else {
    return 0;
  }
}

void LayoutVsSchematicStandardReader::read_subcircuit_pair (db::NetlistCrossReference *xref, const db::Circuit *circuit_a, const db::Circuit *circuit_b)
{
  Brace br (this);

  std::pair<std::string, bool> non_a, non_b;
  non_a = read_non_string ();
  non_b = read_non_string ();

  db::NetlistCrossReference::Status status = db::NetlistCrossReference::None;
  read_status (status);

  br.done ();

  xref->gen_subcircuits (subcircuit_by_name (circuit_a, non_a), subcircuit_by_name (circuit_b, non_b), status);
}

}
