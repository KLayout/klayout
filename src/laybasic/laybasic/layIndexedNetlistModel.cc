
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


#include "layIndexedNetlistModel.h"
#include "dbNetlist.h"

namespace lay
{

// ----------------------------------------------------------------------------------
//  SingleIndexedNetlistModel implementation

namespace {

  template <class Obj>
  struct sort_single_by_name
  {
    inline bool operator() (const Obj *a, const Obj *b) const
    {
      return a->name () < b->name ();
    }
  };

  template <class Obj>
  struct sort_single_by_expanded_name
  {
    inline bool operator() (const Obj *a, const Obj *b) const
    {
      return a->expanded_name () < b->expanded_name ();
    }
  };

  template <class Obj>
  struct sort_single_by_pin_name
  {
    inline bool operator() (const Obj *a, const Obj *b) const
    {
      return a->pin ()->expanded_name () < b->pin ()->expanded_name ();
    }
  };

  template <class Obj>
  struct sort_single_by_terminal_id
  {
    inline bool operator() (const Obj *a, const Obj *b) const
    {
      return a->terminal_id () < b->terminal_id ();
    }
  };

  template <class Obj, class SortBy>
  struct sort_with_null
  {
    inline bool operator() (const Obj *a, const Obj *b) const
    {
      SortBy order;
      if ((a != 0) != (b != 0)) {
        return (a != 0) < (b != 0);
      }
      if (a) {
        if (order (a, b)) {
          return true;
        } else if (order (b, a)) {
          return false;
        }
      }
      return false;
    }
  };

  template <class Obj, class SortBy>
  struct sort_pair
  {
    bool operator() (const std::pair<const Obj *, const Obj *> &a, const std::pair<const Obj *, const Obj *> &b) const
    {
      SortBy order;
      if (order (a.first, b.first)) {
        return true;
      } else if (order (b.first, a.first)) {
        return false;
      }
      return order (a.second, b.second);
    }
  };

  template <class Obj>
  struct sort_by_name
    : public sort_pair<Obj, sort_with_null<Obj, sort_single_by_name<Obj> > >
  {
    //  .. nothing yet ..
  };

  template <class Obj>
  struct sort_by_expanded_name
    : public sort_pair<Obj, sort_with_null<Obj, sort_single_by_expanded_name<Obj> > >
  {
    //  .. nothing yet ..
  };

  template <class Obj>
  struct sort_by_pin_name
    : public sort_pair<Obj, sort_with_null<Obj, sort_single_by_pin_name<Obj> > >
  {
    //  .. nothing yet ..
  };

  template <class Obj>
  struct sort_by_terminal_id
    : public sort_pair<Obj, sort_with_null<Obj, sort_single_by_terminal_id<Obj> > >
  {
    //  .. nothing yet ..
  };

}

template <class Attr, class Iter, class SortBy>
static void fill_map (std::vector<std::pair<const Attr *, const Attr *> > &map, const Iter &begin1, const Iter &end1, const Iter &begin2, const Iter &end2, const SortBy &sorter)
{
  size_t n1 = 0, n2 = 0;
  for (Iter i = begin1; i != end1; ++i) {
    ++n1;
  }
  for (Iter i = begin2; i != end2; ++i) {
    ++n2;
  }
  map.resize (std::max (n1, n2), typename std::pair<const Attr *, const Attr *> (0, 0));

  typename std::vector<std::pair<const Attr *, const Attr *> >::iterator j;
  j = map.begin ();
  for (Iter i = begin1; i != end1; ++i, ++j) {
    j->first = i.operator-> ();
  }
  j = map.begin ();
  for (Iter i = begin2; i != end2; ++i, ++j) {
    j->second = i.operator-> ();
  }

  std::sort (map.begin (), map.end (), sorter);
}

template <class Obj, class Attr, class Iter, class SortBy>
static std::pair<const Attr *, const Attr *> attr_by_object_and_index (const std::pair<const Obj *, const Obj *> &obj, size_t index, const Iter &begin1, const Iter &end1, const Iter &begin2, const Iter &end2, std::map<std::pair<const Obj *, const Obj *>, std::vector<std::pair<const Attr *, const Attr *> > > &cache, const SortBy &sorter)
{
  typename std::map<std::pair<const Obj *, const Obj *>, std::vector<std::pair<const Attr *, const Attr *> > >::iterator cc = cache.find (obj);
  if (cc == cache.end ()) {
    cc = cache.insert (std::make_pair (obj, std::vector<std::pair<const Attr *, const Attr *> > ())).first;
    fill_map (cc->second, begin1, end1, begin2, end2, sorter);
  }

  tl_assert (index < cc->second.size ());
  return cc->second [index];
}

template <class Attr, class Iter, class SortBy>
static size_t index_from_attr (const std::pair<const Attr *, const Attr *> &attrs, const Iter &begin1, const Iter &end1, const Iter &begin2, const Iter &end2, std::map<std::pair<const Attr *, const Attr *>, size_t> &cache, const SortBy &sorter)
{
  typename std::map<std::pair<const Attr *, const Attr *>, size_t>::iterator cc = cache.find (attrs);
  if (cc != cache.end ()) {
    return cc->second;
  }

  std::vector<std::pair<const Attr *, const Attr *> > map;
  fill_map (map, begin1, end1, begin2, end2, sorter);

  for (size_t i = 0; i < map.size (); ++i) {
    cache.insert (std::make_pair (map [i], i));
  }

  cc = cache.find (attrs);
  tl_assert (cc != cache.end ());
  return cc->second;
}

std::string
SingleIndexedNetlistModel::column_title (int section) const
{
  if (section == 0) {
    return tl::to_string (tr ("Object"));
  } else if (section == 1) {
    return tl::to_string (tr ("Connections"));
  }
}

size_t
SingleIndexedNetlistModel::circuit_count () const
{
  return mp_netlist->circuit_count ();
}

size_t
SingleIndexedNetlistModel::net_count (const circuit_pair &circuits) const
{
  return circuits.first->net_count ();
}

size_t
SingleIndexedNetlistModel::net_terminal_count (const net_pair &nets) const
{
  return nets.first->terminal_count ();
}

size_t
SingleIndexedNetlistModel::net_subcircuit_pin_count (const net_pair &nets) const
{
  return nets.first->subcircuit_pin_count ();
}

size_t
SingleIndexedNetlistModel::net_pin_count (const net_pair &nets) const
{
  return nets.first->pin_count ();
}

size_t
SingleIndexedNetlistModel::device_count (const circuit_pair &circuits) const
{
  return circuits.first->device_count ();
}

size_t
SingleIndexedNetlistModel::pin_count (const circuit_pair &circuits) const
{
  return circuits.first->pin_count ();
}

size_t
SingleIndexedNetlistModel::subcircuit_count (const circuit_pair &circuits) const
{
  return circuits.first->subcircuit_count ();
}

IndexedNetlistModel::circuit_pair
SingleIndexedNetlistModel::parent_of (const net_pair &nets) const
{
  return std::make_pair (nets.first->circuit (), (const db::Circuit *) 0);
}

IndexedNetlistModel::circuit_pair
SingleIndexedNetlistModel::parent_of (const device_pair &devices) const
{
  return std::make_pair (devices.first->circuit (), (const db::Circuit *) 0);
}

IndexedNetlistModel::circuit_pair
SingleIndexedNetlistModel::parent_of (const subcircuit_pair &subcircuits) const
{
  return std::make_pair (subcircuits.first->circuit (), (const db::Circuit *) 0);
}

IndexedNetlistModel::circuit_pair
SingleIndexedNetlistModel::circuit_from_index (size_t index) const
{
  db::Netlist::const_circuit_iterator none;
  return attr_by_object_and_index (std::make_pair (mp_netlist, (const db::Netlist *) 0), index, mp_netlist->begin_circuits (), mp_netlist->end_circuits (), none, none, m_circuit_by_index, sort_by_name<db::Circuit> ());
}

IndexedNetlistModel::net_pair
SingleIndexedNetlistModel::net_from_index (const circuit_pair &circuits, size_t index) const
{
  db::Circuit::const_net_iterator none;
  return attr_by_object_and_index (circuits, index, circuits.first->begin_nets (), circuits.first->end_nets (), none, none, m_net_by_circuit_and_index, sort_by_expanded_name<db::Net> ());
}

const db::Net *
SingleIndexedNetlistModel::second_net_for (const db::Net * /*first*/) const
{
  return 0;
}

IndexedNetlistModel::net_subcircuit_pin_pair
SingleIndexedNetlistModel::net_subcircuit_pinref_from_index (const net_pair &nets, size_t index) const
{
  db::Net::const_subcircuit_pin_iterator none;
  return attr_by_object_and_index (nets, index, nets.first->begin_subcircuit_pins (), nets.first->end_subcircuit_pins (), none, none, m_subcircuit_pinref_by_net_and_index, sort_by_pin_name<db::NetSubcircuitPinRef> ());
}

IndexedNetlistModel::net_terminal_pair
SingleIndexedNetlistModel::net_terminalref_from_index (const net_pair &nets, size_t index) const
{
  db::Net::const_terminal_iterator none;
  return attr_by_object_and_index (nets, index, nets.first->begin_terminals (), nets.first->end_terminals (), none, none, m_terminalref_by_net_and_index, sort_by_terminal_id<db::NetTerminalRef> ());
}

IndexedNetlistModel::net_pin_pair
SingleIndexedNetlistModel::net_pinref_from_index (const net_pair &nets, size_t index) const
{
  db::Net::const_pin_iterator none;
  return attr_by_object_and_index (nets, index, nets.first->begin_pins (), nets.first->end_pins (), none, none, m_pinref_by_net_and_index, sort_by_pin_name<db::NetPinRef> ());
}

IndexedNetlistModel::device_pair
SingleIndexedNetlistModel::device_from_index (const circuit_pair &circuits, size_t index) const
{
  db::Circuit::const_device_iterator none;
  return attr_by_object_and_index (circuits, index, circuits.first->begin_devices (), circuits.first->end_devices (), none, none, m_device_by_circuit_and_index, sort_by_expanded_name<db::Device> ());
}

IndexedNetlistModel::pin_pair
SingleIndexedNetlistModel::pin_from_index (const circuit_pair &circuits, size_t index) const
{
  db::Circuit::const_pin_iterator none;
  return attr_by_object_and_index (circuits, index, circuits.first->begin_pins (), circuits.first->end_pins (), none, none, m_pin_by_circuit_and_index, sort_by_expanded_name<db::Pin> ());
}

IndexedNetlistModel::subcircuit_pair
SingleIndexedNetlistModel::subcircuit_from_index (const circuit_pair &circuits, size_t index) const
{
  db::Circuit::const_subcircuit_iterator none;
  return attr_by_object_and_index (circuits, index, circuits.first->begin_subcircuits (), circuits.first->end_subcircuits (), none, none, m_subcircuit_by_circuit_and_index, sort_by_expanded_name<db::SubCircuit> ());
}

size_t
SingleIndexedNetlistModel::circuit_index (const circuit_pair &circuits) const
{
  db::Netlist::const_circuit_iterator none;
  return index_from_attr (circuits, mp_netlist->begin_circuits (), mp_netlist->end_circuits (), none, none, m_circuit_index_by_object, sort_by_name<db::Circuit> ());
}

size_t
SingleIndexedNetlistModel::net_index (const net_pair &nets) const
{
  db::Circuit::const_net_iterator none;

  circuit_pair circuits = parent_of (nets);
  return index_from_attr (nets,
                          circuits.first ? circuits.first->begin_nets () : none, circuits.first ? circuits.first->end_nets () : none,
                          circuits.second ? circuits.second->begin_nets () : none, circuits.second ? circuits.second->end_nets () : none,
                          m_net_index_by_object, sort_by_expanded_name<db::Net> ());
}

size_t
SingleIndexedNetlistModel::device_index (const device_pair &devices) const
{
  db::Circuit::const_device_iterator none;

  circuit_pair circuits = parent_of (devices);
  return index_from_attr (devices,
                          circuits.first ? circuits.first->begin_devices () : none, circuits.first ? circuits.first->end_devices () : none,
                          circuits.second ? circuits.second->begin_devices () : none, circuits.second ? circuits.second->end_devices () : none,
                          m_device_index_by_object, sort_by_expanded_name<db::Device> ());
}

size_t
SingleIndexedNetlistModel::pin_index (const pin_pair &pins, const circuit_pair &circuits) const
{
  db::Circuit::const_pin_iterator none;

  return index_from_attr (pins,
                          circuits.first ? circuits.first->begin_pins () : none, circuits.first ? circuits.first->end_pins () : none,
                          circuits.second ? circuits.second->begin_pins () : none, circuits.second ? circuits.second->end_pins () : none,
                          m_pin_index_by_object, sort_by_expanded_name<db::Pin> ());
}

size_t
SingleIndexedNetlistModel::subcircuit_index (const subcircuit_pair &subcircuits) const
{
  db::Circuit::const_subcircuit_iterator none;

  circuit_pair circuits = parent_of (subcircuits);
  return index_from_attr (subcircuits,
                          circuits.first ? circuits.first->begin_subcircuits () : none, circuits.first ? circuits.first->end_subcircuits () : none,
                          circuits.second ? circuits.second->begin_subcircuits () : none, circuits.second ? circuits.second->end_subcircuits () : none,
                          m_subcircuit_index_by_object, sort_by_expanded_name<db::SubCircuit> ());
}

}
