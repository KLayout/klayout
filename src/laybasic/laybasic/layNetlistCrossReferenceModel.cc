
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


#include "layNetlistCrossReferenceModel.h"

namespace lay
{

NetlistCrossReferenceModel::NetlistCrossReferenceModel (const db::NetlistCrossReference *cross_ref)
  : mp_cross_ref (const_cast<db::NetlistCrossReference *> (cross_ref))
{
  //  .. nothing yet ..
}

size_t NetlistCrossReferenceModel::circuit_count () const
{
  return mp_cross_ref->circuit_count ();
}

size_t NetlistCrossReferenceModel::net_count (const circuit_pair &circuits) const
{
  const db::NetlistCrossReference::PerCircuitData *data = mp_cross_ref->per_circuit_data_for (circuits);
  return data ? data->nets.size () : 0;
}

size_t NetlistCrossReferenceModel::net_terminal_count (const net_pair &nets) const
{
  const db::NetlistCrossReference::PerNetData *data = mp_cross_ref->per_net_data_for (nets);
  return data ? data->terminals.size () : 0;
}

size_t NetlistCrossReferenceModel::net_subcircuit_pin_count (const net_pair &nets) const
{
  const db::NetlistCrossReference::PerNetData *data = mp_cross_ref->per_net_data_for (nets);
  return data ? data->subcircuit_pins.size () : 0;
}

size_t NetlistCrossReferenceModel::net_pin_count (const net_pair &nets) const
{
  const db::NetlistCrossReference::PerNetData *data = mp_cross_ref->per_net_data_for (nets);
  return data ? data->pins.size () : 0;
}

size_t NetlistCrossReferenceModel::device_count (const circuit_pair &circuits) const
{
  const db::NetlistCrossReference::PerCircuitData *data = mp_cross_ref->per_circuit_data_for (circuits);
  return data ? data->devices.size () : 0;
}

size_t NetlistCrossReferenceModel::pin_count (const circuit_pair &circuits) const
{
  const db::NetlistCrossReference::PerCircuitData *data = mp_cross_ref->per_circuit_data_for (circuits);
  return data ? data->pins.size () : 0;
}

size_t NetlistCrossReferenceModel::subcircuit_count (const circuit_pair &circuits) const
{
  const db::NetlistCrossReference::PerCircuitData *data = mp_cross_ref->per_circuit_data_for (circuits);
  return data ? data->subcircuits.size () : 0;
}

namespace {

template <class Obj> struct DataGetter;

template <>
struct DataGetter<const db::Net *>
{
  typedef typename std::vector<db::NetlistCrossReference::NetPairData>::const_iterator iterator_type;
  iterator_type begin (const db::NetlistCrossReference::PerCircuitData &data) const { return data.nets.begin (); }
  iterator_type end (const db::NetlistCrossReference::PerCircuitData &data) const { return data.nets.end (); }
};

template <>
struct DataGetter<const db::Device *>
{
  typedef typename std::vector<db::NetlistCrossReference::DevicePairData>::const_iterator iterator_type;
  iterator_type begin (const db::NetlistCrossReference::PerCircuitData &data) const { return data.devices.begin (); }
  iterator_type end (const db::NetlistCrossReference::PerCircuitData &data) const { return data.devices.end (); }
};

template <>
struct DataGetter<const db::SubCircuit *>
{
  typedef typename std::vector<db::NetlistCrossReference::SubCircuitPairData>::const_iterator iterator_type;
  iterator_type begin (const db::NetlistCrossReference::PerCircuitData &data) const { return data.subcircuits.begin (); }
  iterator_type end (const db::NetlistCrossReference::PerCircuitData &data) const { return data.subcircuits.end (); }
};

}

template <class Pair>
static IndexedNetlistModel::circuit_pair get_parent_of (const Pair &pair, const db::NetlistCrossReference *cross_ref, std::map<Pair, IndexedNetlistModel::circuit_pair> &cache)
{
  typename std::map<Pair, IndexedNetlistModel::circuit_pair>::iterator i = cache.find (pair);
  if (i == cache.end ()) {

    for (db::NetlistCrossReference::per_circuit_data_iterator c = cross_ref->begin_per_circuit_data (); c != cross_ref->end_per_circuit_data (); ++c) {
      typedef DataGetter<typename Pair::first_type> getter_type;
      typedef typename getter_type::iterator_type iterator_type;
      iterator_type b = getter_type ().begin (c->second);
      iterator_type e = getter_type ().end (c->second);
      for (iterator_type j = b; j != e; ++j) {
        cache.insert (std::make_pair (j->pair, c->first));
      }
    }

    i = cache.find (pair);
    tl_assert (i != cache.end ());

  }
  return i->second;
}

IndexedNetlistModel::circuit_pair NetlistCrossReferenceModel::parent_of (const IndexedNetlistModel::net_pair &net_pair) const
{
  return get_parent_of (net_pair, mp_cross_ref.get (), m_parents_of_nets);
}

IndexedNetlistModel::circuit_pair NetlistCrossReferenceModel::parent_of (const IndexedNetlistModel::device_pair &device_pair) const
{
  return get_parent_of (device_pair, mp_cross_ref.get (), m_parents_of_devices);
}

IndexedNetlistModel::circuit_pair NetlistCrossReferenceModel::parent_of (const IndexedNetlistModel::subcircuit_pair &subcircuit_pair) const
{
  return get_parent_of (subcircuit_pair, mp_cross_ref.get (), m_parents_of_subcircuits);
}

IndexedNetlistModel::circuit_pair NetlistCrossReferenceModel::circuit_from_index (size_t index) const
{
  return mp_cross_ref->begin_circuits () [index];
}

IndexedNetlistModel::net_pair NetlistCrossReferenceModel::net_from_index (const circuit_pair &circuits, size_t index) const
{
  const db::NetlistCrossReference::PerCircuitData *data = mp_cross_ref->per_circuit_data_for (circuits);
  tl_assert (data != 0);
  return data->nets [index].pair;
}

const db::Net *NetlistCrossReferenceModel::second_net_for (const db::Net *first) const
{
  return mp_cross_ref->other_net_for (first);
}

IndexedNetlistModel::net_subcircuit_pin_pair NetlistCrossReferenceModel::net_subcircuit_pinref_from_index (const net_pair &nets, size_t index) const
{
  const db::NetlistCrossReference::PerNetData *data = mp_cross_ref->per_net_data_for (nets);
  tl_assert (data != 0);
  return data->subcircuit_pins [index];
}

IndexedNetlistModel::net_terminal_pair NetlistCrossReferenceModel::net_terminalref_from_index (const net_pair &nets, size_t index) const
{
  const db::NetlistCrossReference::PerNetData *data = mp_cross_ref->per_net_data_for (nets);
  tl_assert (data != 0);
  return data->terminals [index];
}

IndexedNetlistModel::net_pin_pair NetlistCrossReferenceModel::net_pinref_from_index (const net_pair &nets, size_t index) const
{
  const db::NetlistCrossReference::PerNetData *data = mp_cross_ref->per_net_data_for (nets);
  tl_assert (data != 0);
  return data->pins [index];
}

IndexedNetlistModel::device_pair NetlistCrossReferenceModel::device_from_index (const circuit_pair &circuits, size_t index) const
{
  const db::NetlistCrossReference::PerCircuitData *data = mp_cross_ref->per_circuit_data_for (circuits);
  tl_assert (data != 0);
  return data->devices [index].pair;
}

IndexedNetlistModel::pin_pair NetlistCrossReferenceModel::pin_from_index (const circuit_pair &circuits, size_t index) const
{
  const db::NetlistCrossReference::PerCircuitData *data = mp_cross_ref->per_circuit_data_for (circuits);
  tl_assert (data != 0);
  return data->pins [index].pair;
}

IndexedNetlistModel::subcircuit_pair NetlistCrossReferenceModel::subcircuit_from_index (const circuit_pair &circuits, size_t index) const
{
  const db::NetlistCrossReference::PerCircuitData *data = mp_cross_ref->per_circuit_data_for (circuits);
  tl_assert (data != 0);
  return data->subcircuits [index].pair;
}

template <class Pair, class Iter>
static size_t get_index_of (const Pair &pair, Iter begin, Iter end, std::map<Pair, size_t> &cache)
{
  typename std::map<Pair, size_t>::iterator i = cache.find (pair);
  if (i != cache.end ()) {

    size_t index = 0;
    for (Iter j = begin; j != end; ++j, ++index) {
      cache.insert (std::make_pair (j->pair, index));
    }

    i = cache.find (pair);
    tl_assert (i != cache.end ());
  }

  return i->second;
}


size_t NetlistCrossReferenceModel::circuit_index (const circuit_pair &circuits) const
{
  typename std::map<circuit_pair, size_t>::iterator i = m_index_of_circuits.find (circuits);
  if (i != m_index_of_circuits.end ()) {

    size_t index = 0;
    for (db::NetlistCrossReference::circuits_iterator j = mp_cross_ref->begin_circuits (); j != mp_cross_ref->end_circuits (); ++j, ++index) {
      m_index_of_circuits.insert (std::make_pair (*j, index));
    }

    i = m_index_of_circuits.find (circuits);
    tl_assert (i != m_index_of_circuits.end ());
  }

  return i->second;
}

size_t NetlistCrossReferenceModel::net_index (const net_pair &nets) const
{
  circuit_pair circuits = parent_of (nets);
  PerCircuitCacheData *data = & m_per_circuit_data [circuits];
  const db::NetlistCrossReference::PerCircuitData *org_data = mp_cross_ref->per_circuit_data_for (circuits);
  tl_assert (org_data != 0);
  return get_index_of (nets, org_data->nets.begin (), org_data->nets.end (), data->index_of_nets);
}

size_t NetlistCrossReferenceModel::device_index (const device_pair &devices) const
{
  circuit_pair circuits = parent_of (devices);
  PerCircuitCacheData *data = & m_per_circuit_data [circuits];
  const db::NetlistCrossReference::PerCircuitData *org_data = mp_cross_ref->per_circuit_data_for (circuits);
  tl_assert (org_data != 0);
  return get_index_of (devices, org_data->devices.begin (), org_data->devices.end (), data->index_of_devices);
}

size_t NetlistCrossReferenceModel::pin_index (const pin_pair &pins, const circuit_pair &circuits) const
{
  PerCircuitCacheData *data = & m_per_circuit_data [circuits];
  const db::NetlistCrossReference::PerCircuitData *org_data = mp_cross_ref->per_circuit_data_for (circuits);
  tl_assert (org_data != 0);
  return get_index_of (pins, org_data->pins.begin (), org_data->pins.end (), data->index_of_pins);
}

size_t NetlistCrossReferenceModel::subcircuit_index (const subcircuit_pair &subcircuits) const
{
  circuit_pair circuits = parent_of (subcircuits);
  PerCircuitCacheData *data = & m_per_circuit_data [circuits];
  const db::NetlistCrossReference::PerCircuitData *org_data = mp_cross_ref->per_circuit_data_for (circuits);
  tl_assert (org_data != 0);
  return get_index_of (subcircuits, org_data->subcircuits.begin (), org_data->subcircuits.end (), data->index_of_subcircuits);
}

}
