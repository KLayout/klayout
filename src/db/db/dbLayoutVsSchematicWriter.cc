
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
#include "dbLayoutVsSchematicWriter.h"
#include "dbLayoutVsSchematic.h"
#include "dbLayoutVsSchematicFormatDefs.h"

namespace db
{

// -------------------------------------------------------------------------------------------
//  LayoutVsSchematicWriterBase implementation

LayoutVsSchematicWriterBase::LayoutVsSchematicWriterBase ()
{
  //  .. nothing yet ..
}

LayoutVsSchematicWriterBase::~LayoutVsSchematicWriterBase ()
{
  //  .. nothing yet ..
}

void LayoutVsSchematicWriterBase::write (const db::LayoutVsSchematic *lvs)
{
  do_write_lvs (lvs);
}

// -------------------------------------------------------------------------------------------

namespace lvs_std_format
{

// -------------------------------------------------------------------------------------------
//  std_writer_impl<Keys> implementation

template <class Keys>
class std_writer_impl
  : public l2n_std_format::std_writer_impl<typename Keys::l2n_keys>
{
public:
  std_writer_impl (tl::OutputStream &stream, double dbu, const std::string &progress_description = std::string ());

  void write (const db::LayoutVsSchematic *l2n);

private:
  tl::OutputStream &stream ()
  {
    return l2n_std_format::std_writer_impl<typename Keys::l2n_keys>::stream ();
  }

  std::string status_to_s (const db::NetlistCrossReference::Status status);
  std::string message_to_s (const std::string &msg);
  void write (const db::NetlistCrossReference *xref);

  std::map<const db::Circuit *, std::map<const db::Net *, unsigned int> > m_net2id_per_circuit_a, m_net2id_per_circuit_b;
};

static const std::string endl ("\n");
static const std::string indent1 (" ");
static const std::string indent2 ("  ");

template <class Keys>
std_writer_impl<Keys>::std_writer_impl (tl::OutputStream &stream, double dbu, const std::string &progress_description)
  : l2n_std_format::std_writer_impl<typename Keys::l2n_keys> (stream, dbu, progress_description.empty () ? tl::to_string (tr ("Writing LVS database")) : progress_description)
{
  //  .. nothing yet ..
}

template <class Keys>
void std_writer_impl<Keys>::write (const db::LayoutVsSchematic *lvs)
{
  const int version = 0;

  stream () << Keys::lvs_magic_string << endl;

  if (version > 0) {
    stream () << Keys::version_key << "(" << version << ")" << endl;
  }

  if (lvs->netlist ()) {
    if (! Keys::is_short ()) {
      stream () << endl << "# Layout" << endl;
    }
    stream () << Keys::layout_key << "(" << endl;
    l2n_std_format::std_writer_impl<typename Keys::l2n_keys>::write (lvs->netlist (), lvs, true, &m_net2id_per_circuit_a);
    stream () << ")" << endl;
  }

  if (lvs->reference_netlist ()) {
    if (! Keys::is_short ()) {
      stream () << endl << "# Reference netlist" << endl;
    }
    stream () << Keys::reference_key << "(" << endl;
    l2n_std_format::std_writer_impl<typename Keys::l2n_keys>::write (lvs->reference_netlist (), 0, true, &m_net2id_per_circuit_b);
    stream () << ")" << endl;
  }

  if (lvs->cross_ref ()) {
    if (! Keys::is_short ()) {
      stream () << endl << "# Cross reference" << endl;
    }
    stream () << Keys::xref_key << "(" << endl;
    write (lvs->cross_ref ());
    stream () << ")" << endl;
  }
}

template <class Obj>
std::string name_to_s (const Obj *obj)
{
  if (obj) {
    return tl::to_word_or_quoted_string (obj->name ());
  } else {
    return "()";
  }
}

template <class Obj>
std::string ion_to_s (const Obj *obj)
{
  if (obj) {
    return tl::to_string (obj->id ());
  } else {
    return "()";
  }
}

static std::string net_id_to_s (const db::Net *net, const std::map<const db::Net *, unsigned int> &net2id)
{
  if (net) {
    std::map<const db::Net *, unsigned int>::const_iterator i = net2id.find (net);
    tl_assert (i != net2id.end ());
    return tl::to_string (i->second);
  } else {
    return "()";
  }
}

static void build_pin_index_map (const db::Circuit *c, std::map<const db::Pin *, unsigned int> &pin2index)
{
  if (c) {
    unsigned int pi = 0;
    for (db::Circuit::const_pin_iterator p = c->begin_pins (); p != c->end_pins (); ++p, ++pi) {
      pin2index.insert (std::make_pair (p.operator-> (), pi));
    }
  }
}

static std::string pin_id_to_s (const db::Pin *pin, const std::map<const db::Pin *, unsigned int> &pin2index)
{
  if (pin) {
    std::map<const db::Pin *, unsigned int>::const_iterator i = pin2index.find (pin);
    tl_assert (i != pin2index.end ());
    return tl::to_string (i->second);
  } else {
    return "()";
  }
}

template <class Keys>
std::string std_writer_impl<Keys>::message_to_s (const std::string &msg)
{
  if (msg.empty ()) {
    return std::string ();
  } else {
    return " " + Keys::description_key + "(" + tl::to_word_or_quoted_string (msg) + ")";
  }
}

template <class Keys>
std::string std_writer_impl<Keys>::status_to_s (const db::NetlistCrossReference::Status status)
{
  if (status == db::NetlistCrossReference::Match) {
    return " " + Keys::match_key;
  } else if (status == db::NetlistCrossReference::NoMatch) {
    return " " + Keys::nomatch_key;
  } else if (status == db::NetlistCrossReference::Mismatch) {
    return " " + Keys::mismatch_key;
  } else if (status == db::NetlistCrossReference::MatchWithWarning) {
    return " " + Keys::warning_key;
  } else if (status == db::NetlistCrossReference::Skipped) {
    return " " + Keys::skipped_key;
  } else {
    return std::string ();
  }
}

template <class Keys>
void std_writer_impl<Keys>::write (const db::NetlistCrossReference *xref)
{
  for (db::NetlistCrossReference::circuits_iterator c = xref->begin_circuits (); c != xref->end_circuits (); ++c) {

    const db::NetlistCrossReference::PerCircuitData *pcd = xref->per_circuit_data_for (*c);
    tl_assert (pcd != 0);

    stream () << indent1 << Keys::circuit_key << "(" << name_to_s (c->first) << " " << name_to_s (c->second) << status_to_s (pcd->status) << message_to_s (pcd->msg) << endl;
    stream () << indent2 << Keys::xref_key << "(" << endl;

    for (db::NetlistCrossReference::PerCircuitData::net_pairs_const_iterator n = pcd->nets.begin (); n != pcd->nets.end (); ++n) {
      stream () << indent1 << indent2 << Keys::net_key << "(" << net_id_to_s (n->pair.first, m_net2id_per_circuit_a [c->first]) << " " << net_id_to_s (n->pair.second, m_net2id_per_circuit_b [c->second]) << status_to_s (n->status) << message_to_s (n->msg) << ")" << endl;
    }

    std::map<const db::Pin *, unsigned int> pin2index_a, pin2index_b;
    build_pin_index_map (c->first, pin2index_a);
    build_pin_index_map (c->second, pin2index_b);

    for (db::NetlistCrossReference::PerCircuitData::pin_pairs_const_iterator n = pcd->pins.begin (); n != pcd->pins.end (); ++n) {
      stream () << indent1 << indent2 << Keys::pin_key << "(" << pin_id_to_s (n->pair.first, pin2index_a) << " " << pin_id_to_s (n->pair.second, pin2index_b) << status_to_s (n->status) << message_to_s (n->msg) << ")" << endl;
    }

    for (db::NetlistCrossReference::PerCircuitData::device_pairs_const_iterator n = pcd->devices.begin (); n != pcd->devices.end (); ++n) {
      stream () << indent1 << indent2 << Keys::device_key << "(" << ion_to_s (n->pair.first) << " " << ion_to_s (n->pair.second) << status_to_s (n->status) << message_to_s (n->msg) << ")" << endl;
    }

    for (db::NetlistCrossReference::PerCircuitData::subcircuit_pairs_const_iterator n = pcd->subcircuits.begin (); n != pcd->subcircuits.end (); ++n) {
      stream () << indent1 << indent2 << Keys::circuit_key << "(" << ion_to_s (n->pair.first) << " " << ion_to_s (n->pair.second) << status_to_s (n->status) << message_to_s (n->msg) << ")" << endl;
    }

    stream () << indent2 << ")" << endl;
    stream () << indent1 << ")" << endl;

  }
}

}

// -------------------------------------------------------------------------------------------
//  LayoutVsSchematicStandardWriter implementation

LayoutVsSchematicStandardWriter::LayoutVsSchematicStandardWriter (tl::OutputStream &stream, bool short_version)
  : mp_stream (&stream), m_short_version (short_version)
{
  //  .. nothing yet ..
}

void LayoutVsSchematicStandardWriter::do_write_lvs (const db::LayoutVsSchematic *lvs)
{
  if (! lvs->netlist ()) {
    throw tl::Exception (tl::to_string (tr ("Can't write LVS DB before the netlist has been created")));
  }
  if (! lvs->internal_layout ()) {
    throw tl::Exception (tl::to_string (tr ("Can't write LVS DB before the layout has been loaded")));
  }

  double dbu = lvs->internal_layout ()->dbu ();

  if (m_short_version) {
    lvs_std_format::std_writer_impl<lvs_std_format::keys<true> > writer (*mp_stream, dbu);
    writer.write (lvs);
  } else {
    lvs_std_format::std_writer_impl<lvs_std_format::keys<false> > writer (*mp_stream, dbu);
    writer.write (lvs);
  }
}

}
