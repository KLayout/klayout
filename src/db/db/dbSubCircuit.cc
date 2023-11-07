
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

#include "dbSubCircuit.h"
#include "dbCircuit.h"
#include "tlIteratorUtils.h"

namespace db
{

// --------------------------------------------------------------------------------
//  SubCircuit class implementation

SubCircuit::SubCircuit ()
  : db::NetlistObject (), m_id (0), mp_circuit (0)
{
  //  .. nothing yet ..
}

SubCircuit::~SubCircuit()
{
  for (std::vector<Net::subcircuit_pin_iterator>::const_iterator p = m_pin_refs.begin (); p != m_pin_refs.end (); ++p) {
    if (! tl::is_null_iterator (*p) && (*p)->net ()) {
      (*p)->net ()->erase_subcircuit_pin (*p);
    }
  }
}

SubCircuit::SubCircuit (Circuit *circuit, const std::string &name)
  : db::NetlistObject (), m_circuit_ref (0), m_name (name), m_id (0), mp_circuit (0)
{
  set_circuit_ref (circuit);
}

SubCircuit::SubCircuit (const SubCircuit &other)
  : db::NetlistObject (other), m_id (0), mp_circuit (0)
{
  operator= (other);
}

SubCircuit &SubCircuit::operator= (const SubCircuit &other)
{
  if (this != &other) {
    db::NetlistObject::operator= (other);
    m_name = other.m_name;
    m_trans = other.m_trans;
    set_circuit_ref (const_cast<Circuit *> (other.circuit_ref ()));
  }
  return *this;
}

void SubCircuit::set_name (const std::string &n)
{
  m_name = n;
  if (mp_circuit) {
    mp_circuit->m_subcircuit_by_name.invalidate ();
  }
}

std::string SubCircuit::expanded_name () const
{
  if (name ().empty ()) {
    return "$" + tl::to_string (id ());
  } else {
    return name ();
  }
}

void SubCircuit::set_trans (const db::DCplxTrans &t)
{
  m_trans = t;
}

void SubCircuit::erase_pin (size_t pin_id)
{
  Net *net = net_for_pin (pin_id);

  if (! tl::is_null_iterator (m_pin_refs [pin_id])) {
    net->erase_subcircuit_pin (m_pin_refs [pin_id]);
  }

  m_pin_refs.erase (m_pin_refs.begin () + pin_id);

  //  correct pin IDs for the pins with ID > pin_id
  for (auto p = m_pin_refs.begin () + pin_id; p != m_pin_refs.end (); ++p) {
    if (! tl::is_null_iterator (*p)) {
      (*p)->set_pin_id ((*p)->pin_id () - 1);
    }
  }
}

void SubCircuit::set_pin_ref_for_pin (size_t pin_id, Net::subcircuit_pin_iterator iter)
{
  if (m_pin_refs.size () < pin_id + 1) {
    m_pin_refs.resize (pin_id + 1, Net::subcircuit_pin_iterator ());
  }
  m_pin_refs [pin_id] = iter;
}

void SubCircuit::set_circuit_ref (Circuit *c)
{
  if (m_circuit_ref.get ()) {
    m_circuit_ref->unregister_ref (this);
  }
  m_circuit_ref.reset (c);
  if (m_circuit_ref.get ()) {
    m_circuit_ref->register_ref (this);
  }
}

const Net *SubCircuit::net_for_pin (size_t pin_id) const
{
  if (pin_id < m_pin_refs.size ()) {
    Net::subcircuit_pin_iterator p = m_pin_refs [pin_id];
    if (! tl::is_null_iterator (p)) {
      return p->net ();
    }
  }
  return 0;
}

const NetSubcircuitPinRef *SubCircuit::netref_for_pin (size_t pin_id) const
{
  if (pin_id < m_pin_refs.size ()) {
    Net::subcircuit_pin_iterator p = m_pin_refs [pin_id];
    if (! tl::is_null_iterator (p)) {
      return p.operator-> ();
    }
  }
  return 0;
}

void SubCircuit::connect_pin (size_t pin_id, Net *net)
{
  if (net_for_pin (pin_id) == net) {
    return;
  }

  if (pin_id < m_pin_refs.size ()) {
    Net::subcircuit_pin_iterator p = m_pin_refs [pin_id];
    if (! tl::is_null_iterator (p) && p->net ()) {
      p->net ()->erase_subcircuit_pin (p);
    }
    m_pin_refs [pin_id] = Net::subcircuit_pin_iterator ();
  }

  if (net) {
    net->add_subcircuit_pin (NetSubcircuitPinRef (this, pin_id));
  }
}

}
