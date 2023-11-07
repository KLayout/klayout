
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

#include "dbLayoutVsSchematicReader.h"
#include "dbLayoutVsSchematicFormatDefs.h"

namespace db
{

typedef lvs_std_format::keys<true> skeys;
typedef lvs_std_format::keys<false> lkeys;

// -------------------------------------------------------------------------------------------------
//  LayoutVsSchematicStandardReader implementation

LayoutVsSchematicStandardReader::LayoutVsSchematicStandardReader (tl::InputStream &stream)
  : LayoutToNetlistStandardReader (stream)
{
  //  .. nothing yet ..
}

void LayoutVsSchematicStandardReader::do_read_lvs (db::LayoutVsSchematic *lvs)
{
  try {
    read_netlist (lvs);
  } catch (tl::Exception &ex) {
    throw tl::Exception (tl::sprintf (tl::to_string (tr ("%s in line: %d of %s")), ex.msg (), stream ().line_number (), path ()));
  }
}

void LayoutVsSchematicStandardReader::read_netlist (db::LayoutVsSchematic *lvs)
{
  int version = 0;
  std::string description;
  m_map_per_circuit_a.clear ();
  m_map_per_circuit_b.clear ();

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
      LayoutToNetlistStandardReader::read_netlist (0, lvs, &br, &m_map_per_circuit_a);
      br.done ();

    } else if (test (skeys::reference_key) || test (lkeys::reference_key)) {

      Brace br (this);
      std::unique_ptr<db::Netlist> netlist (new db::Netlist ());
      LayoutToNetlistStandardReader::read_netlist (netlist.get (), 0, &br, &m_map_per_circuit_b);
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

    } else if (at_end ()) {
      throw tl::Exception (tl::to_string (tr ("Unexpected end of file")));
    } else {
      skip_element ();
    }

  }

  if (version > 1) {
    throw tl::Exception (tl::to_string (tr ("This program version only supports version 1 of the LVS DB format. File version is: ")) + tl::to_string (version));
  }
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

void LayoutVsSchematicStandardReader::read_log_entry (db::NetlistCrossReference *xref)
{
  db::Severity severity = db::NoSeverity;
  std::string msg;

  Brace br (this);
  while (br) {
    if (read_severity (severity)) {
      //  continue
    } else if (read_message (msg)) {
      //  continue
    } else {
      skip_element ();
    }
  }
  br.done ();

  //  NOTE: this API does not use the full feature set of db::LogEntryData, so
  //  we do not use this object here.
  xref->log_entry (severity, msg);
}

void LayoutVsSchematicStandardReader::read_logs (db::NetlistCrossReference *xref)
{
  Brace br (this);
  while (br) {

    if (test (skeys::log_entry_key) || test (lkeys::log_entry_key)) {
      read_log_entry (xref);
    } else if (at_end ()) {
      throw tl::Exception (tl::to_string (tr ("Unexpected end of file inside log section (entry expected)")));
    } else {
      skip_element ();
    }

  }
  br.done ();
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
    } else if (at_end ()) {
      throw tl::Exception (tl::to_string (tr ("Unexpected end of file inside circuit definition (net, pin, device or circuit expected)")));
    } else {
      skip_element ();
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

      std::pair<std::string, bool> non_a, non_b;
      non_a = read_non ();
      non_b = read_non ();

      const db::Circuit *circuit_a = 0;
      if (non_a.second) {
        circuit_a = xref->netlist_a ()->circuit_by_name (non_a.first);
        if (! circuit_a) {
          throw tl::Exception (tl::to_string (tr ("Not a valid circuit name: ")) + non_a.first);
        }
      }

      const db::Circuit *circuit_b = 0;
      if (non_b.second) {
        circuit_b = xref->netlist_b ()->circuit_by_name (non_b.first);
        if (! circuit_b) {
          throw tl::Exception (tl::to_string (tr ("Not a valid circuit name: ")) + non_b.first);
        }
      }

      xref->gen_begin_circuit (circuit_a, circuit_b);

      db::NetlistCrossReference::Status status = db::NetlistCrossReference::None;
      std::string msg;

      while (br) {

        if (read_status (status)) {
          //  continue
        } else if (read_message (msg)) {
          //  continue
        } else if (test (skeys::xref_key) || test (lkeys::xref_key)) {
          read_xrefs_for_circuits (xref, circuit_a, circuit_b);
        } else if (test (skeys::log_key) || test (lkeys::log_key)) {
          read_logs (xref);
        } else if (at_end ()) {
          throw tl::Exception (tl::to_string (tr ("Unexpected end of file inside circuit definition (status keyword of xrefs expected)")));
        } else {
          skip_element ();
        }

      }

      xref->gen_end_circuit (circuit_a, circuit_b, status, msg);

      br.done ();

    } else if (test (skeys::log_key) || test (lkeys::log_key)) {
      read_logs (xref);
    } else {
      skip_element ();
    }

  }
  br.done ();
}

std::pair<std::string, bool> LayoutVsSchematicStandardReader::read_non ()
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

std::pair<unsigned int, bool> LayoutVsSchematicStandardReader::read_ion ()
{
  if (test ("(")) {
    expect (")");
    return std::make_pair (0, false);
  } else {
    return std::make_pair ((unsigned int) read_int (), true);
  }
}


static const db::Net *net_by_numerical_id (const db::Circuit *circuit, const std::pair<unsigned int, bool> &ion, std::map<const db::Circuit *, db::LayoutToNetlistStandardReader::ObjectMap> &map_per_circuit)
{
  if (ion.second && circuit) {

    std::map<const db::Circuit *, db::LayoutToNetlistStandardReader::ObjectMap>::const_iterator i = map_per_circuit.find (circuit);
    if (i != map_per_circuit.end ()) {

      std::map<unsigned int, Net *>::const_iterator j = i->second.id2net.find (ion.first);
      if (j != i->second.id2net.end ()) {
        return j->second;
      }

    }

    throw tl::Exception (tl::to_string (tr ("Not a valid net ID: ")) + tl::to_string (ion.first));

  }

  return 0;
}

static const db::Device *device_by_numerical_id (const db::Circuit *circuit, const std::pair<unsigned int, bool> &ion, std::map<const db::Circuit *, db::LayoutToNetlistStandardReader::ObjectMap> &map_per_circuit)
{
  if (ion.second && circuit) {

    std::map<const db::Circuit *, db::LayoutToNetlistStandardReader::ObjectMap>::const_iterator i = map_per_circuit.find (circuit);
    if (i != map_per_circuit.end ()) {

      std::map<unsigned int, Device *>::const_iterator j = i->second.id2device.find (ion.first);
      if (j != i->second.id2device.end ()) {
        return j->second;
      }

    }

    throw tl::Exception (tl::to_string (tr ("Not a valid device ID: ")) + tl::to_string (ion.first));

  }

  return 0;
}

static const db::SubCircuit *subcircuit_by_numerical_id (const db::Circuit *circuit, const std::pair<unsigned int, bool> &ion, std::map<const db::Circuit *, db::LayoutToNetlistStandardReader::ObjectMap> &map_per_circuit)
{
  if (ion.second && circuit) {

    std::map<const db::Circuit *, db::LayoutToNetlistStandardReader::ObjectMap>::const_iterator i = map_per_circuit.find (circuit);
    if (i != map_per_circuit.end ()) {

      std::map<unsigned int, SubCircuit *>::const_iterator j = i->second.id2subcircuit.find (ion.first);
      if (j != i->second.id2subcircuit.end ()) {
        return j->second;
      }

    }

    throw tl::Exception (tl::to_string (tr ("Not a subcircuit device ID: ")) + tl::to_string (ion.first));

  }

  return 0;
}

static const db::Pin *pin_by_numerical_id (const db::Circuit *circuit, const std::pair<unsigned int, bool> &ion)
{
  if (ion.second && circuit) {

    const db::Pin *pin = circuit->pin_by_id (ion.first);
    if (! pin) {
      throw tl::Exception (tl::to_string (tr ("Not a valid pin ID: ")) + tl::to_string (ion.first));
    }

    return pin;

  } else {
    return 0;
  }
}

void LayoutVsSchematicStandardReader::read_net_pair (db::NetlistCrossReference *xref, const db::Circuit *circuit_a, const db::Circuit *circuit_b)
{
  Brace br (this);

  std::pair<unsigned int, bool> ion_a, ion_b;
  ion_a = read_ion ();
  ion_b = read_ion ();

  db::NetlistCrossReference::Status status = db::NetlistCrossReference::None;
  std::string msg;
  read_status (status);
  read_message (msg);

  while (br) {
    skip_element ();
  }

  br.done ();

  xref->gen_nets (net_by_numerical_id (circuit_a, ion_a, m_map_per_circuit_a), net_by_numerical_id (circuit_b, ion_b, m_map_per_circuit_b), status, msg);
}

void LayoutVsSchematicStandardReader::read_pin_pair (db::NetlistCrossReference *xref, const db::Circuit *circuit_a, const db::Circuit *circuit_b)
{
  Brace br (this);

  std::pair<unsigned int, bool> ion_a, ion_b;
  ion_a = read_ion ();
  ion_b = read_ion ();

  db::NetlistCrossReference::Status status = db::NetlistCrossReference::None;
  std::string msg;
  read_status (status);
  read_message (msg);

  while (br) {
    skip_element ();
  }

  br.done ();

  xref->gen_pins (pin_by_numerical_id (circuit_a, ion_a), pin_by_numerical_id (circuit_b, ion_b), status, msg);
}

void LayoutVsSchematicStandardReader::read_device_pair (db::NetlistCrossReference *xref, const db::Circuit *circuit_a, const db::Circuit *circuit_b)
{
  Brace br (this);

  std::pair<unsigned int, bool> ion_a, ion_b;
  ion_a = read_ion ();
  ion_b = read_ion ();

  db::NetlistCrossReference::Status status = db::NetlistCrossReference::None;
  std::string msg;
  read_status (status);
  read_message (msg);

  while (br) {
    skip_element ();
  }

  br.done ();

  xref->gen_devices (device_by_numerical_id (circuit_a, ion_a, m_map_per_circuit_a), device_by_numerical_id (circuit_b, ion_b, m_map_per_circuit_b), status, msg);
}

void LayoutVsSchematicStandardReader::read_subcircuit_pair (db::NetlistCrossReference *xref, const db::Circuit *circuit_a, const db::Circuit *circuit_b)
{
  Brace br (this);

  std::pair<unsigned int, bool> ion_a, ion_b;
  ion_a = read_ion ();
  ion_b = read_ion ();

  db::NetlistCrossReference::Status status = db::NetlistCrossReference::None;
  std::string msg;
  read_status (status);
  read_message (msg);

  while (br) {
    skip_element ();
  }

  br.done ();

  xref->gen_subcircuits (subcircuit_by_numerical_id (circuit_a, ion_a, m_map_per_circuit_a), subcircuit_by_numerical_id (circuit_b, ion_b, m_map_per_circuit_b), status, msg);
}

}
