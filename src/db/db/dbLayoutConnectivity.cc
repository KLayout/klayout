
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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


#include "dbLayoutConnectivity.h"
#include "dbLayout.h"
#include "dbShapes.h"
#include "tlVariant.h"

namespace db
{

//  TODO: use an ID to indicate an internal use?
db::property_names_id_type pin_property_name_id = db::property_names_id ("klayout:pin");
db::property_names_id_type instance_connections_property_name_id = db::property_names_id ("klayout:instance-connections");
db::property_names_id_type shape_net_property_name_id = db::property_names_id ("klayout:net");

// ---------------------------------------------------------------------------
//  LayoutPin class

LayoutPin::LayoutPin ()
  : m_name (0), m_must_connect (false)
{
  //  .. nothing yet ..
}

LayoutPin::LayoutPin (const std::string &name, bool must_connect)
  : m_name (0), m_must_connect (must_connect)
{
  m_name = db::property_names_id (name);
}

void
LayoutPin::set_net_name (const std::string &n)
{
  if (n.empty ()) {
    m_net_name = 0;
  } else {
    m_net_name = db::property_names_id (n);
  }
}

const char *
LayoutPin::net_name () const
{
  if (m_net_name == 0) {
    return "";  //  or null?
  } else {
    return db::property_name (m_net_name).to_string ();
  }
}

void
LayoutPin::set_name (const std::string &n)
{
  if (n.empty ()) {
    m_name = 0;
  } else {
    m_name = db::property_names_id (n);
  }
}

const char *
LayoutPin::name () const
{
  if (m_name == 0) {
    return "";  //  or null?
  } else {
    return db::property_name (m_name).to_string ();
  }
}

std::string
LayoutPin::basic_name () const
{
  const char *n = name ();
  const char *cp = n;
  while (*cp) {
    if (*cp == '$') {
      const char *eos = cp;
      do {
        ++cp;
      } while (*cp && isdigit (*cp));
      if (! *cp) {
        return std::string (n, eos - n);
      }
    } else {
      ++cp;
    }
  }

  return std::string (n, cp - n);
}

void
LayoutPin::set_must_connect (bool mc)
{
  m_must_connect = mc;
}

//  NOTE: serialization uses the generic tl::Variant dict
//  format with a number of keys. This format will be
//  compatible to future enhancements.

static const std::string lp_name_key ("name");
static const std::string lp_must_connect_key ("must_connect");

std::string
LayoutPin::to_string () const
{
  tl::Variant value = tl::Variant::empty_array ();
  value.insert (lp_name_key, name ());
  if (must_connect ()) {
    value.insert (lp_must_connect_key, must_connect ());
  }
  return value.to_parsable_string ();
}

bool
LayoutPin::parse (tl::Extractor &ex)
{
  tl::Variant dict;
  ex.read (dict);
  if (! dict.is_array ()) {
    return false;
  }

  m_name = 0;
  m_must_connect = false;

  const auto *nv = dict.find (lp_name_key);
  if (nv) {
    set_name (nv->to_string ());
  }

  const auto *mcv = dict.find (lp_must_connect_key);
  if (mcv) {
    set_must_connect (mcv->to_bool ());
  }

  return true;
}

// ---------------------------------------------------------------------------
//  LayoutInstanceConnections class

LayoutInstanceConnections::LayoutInstanceConnections ()
{
  //  .. nothing yet ..
}

void
LayoutInstanceConnections::clear ()
{
  m_connections.clear ();
}

void
LayoutInstanceConnections::add_connection (const std::string &pin_name, const std::string &net)
{
  m_connections.insert (std::make_pair (db::property_names_id (pin_name), db::property_names_id (net)));
}

void
LayoutInstanceConnections::add_connection (db::property_names_id_type pin_name_id, db::property_names_id_type net_id)
{
  m_connections.insert (std::make_pair (pin_name_id, net_id));
}

bool
LayoutInstanceConnections::has_pin (const std::string &pin_name) const
{
  return m_connections.find (db::property_names_id (pin_name)) != m_connections.end ();
}

bool
LayoutInstanceConnections::has_pin (db::property_names_id_type pin_name_id) const
{
  return m_connections.find (pin_name_id) != m_connections.end ();
}

const char *
LayoutInstanceConnections::net_for_pin (const std::string &pin_name) const
{
  auto n = m_connections.find (db::property_names_id (pin_name));
  if (n != m_connections.end ()) {
    return db::property_name (n->second).to_string ();
  } else {
    return 0;
  }
}

db::property_names_id_type
LayoutInstanceConnections::net_id_for_pin (const std::string &pin_name) const
{
  auto n = m_connections.find (db::property_names_id (pin_name));
  return n != m_connections.end () ? n->second : 0;
}

db::property_names_id_type
LayoutInstanceConnections::net_id_for_pin (db::property_names_id_type pin_name_id) const
{
  auto n = m_connections.find (pin_name_id);
  return n != m_connections.end () ? n->second : 0;
}

//  NOTE: serialization uses the generic tl::Variant dict
//  format with a number of keys. This format will be
//  compatible to future enhancements.

static const std::string lic_connections_key ("connections");

std::string
LayoutInstanceConnections::to_string () const
{
  tl::Variant value = tl::Variant::empty_array ();

  tl::Variant connections = tl::Variant::empty_array ();
  for (auto i = m_connections.begin (); i != m_connections.end (); ++i) {
    connections.insert (db::property_name (i->first), db::property_name (i->second));
  }
  value.insert (lic_connections_key, connections);

  return value.to_parsable_string ();
}

bool
LayoutInstanceConnections::parse (tl::Extractor &ex)
{
  tl::Variant dict;
  ex.read (dict);
  if (! dict.is_array ()) {
    return false;
  }

  m_connections.clear ();

  const auto *connections = dict.find (lic_connections_key);
  if (connections && connections->is_array ()) {
    for (auto c = connections->begin_array (); c != connections->end_array (); ++c) {
      m_connections.insert (std::make_pair (db::property_names_id (c->first), db::property_names_id (c->second)));
    }
  }

  return true;
}

// ---------------------------------------------------------------------------
//  LayoutConnectivityIndex class

LayoutConnectivityIndex::LayoutConnectivityIndex (const db::Cell *cell)
  : mp_cell (cell), m_pins_available (false), m_nets_available (false)
{
  //  .. nothing yet ..
}

void
LayoutConnectivityIndex::clear ()
{
  m_pins_available = m_nets_available = false;
  m_pins.clear ();
  m_nets.clear ();
}

void
LayoutConnectivityIndex::ensure_pins () const
{
  if (m_pins_available) {
    return;
  }

  m_pins_available = true;

  const db::Layout *layout = mp_cell ? mp_cell->layout () : 0;
  if (! layout) {
    return;
  }

  for (auto l = layout->begin_layers (); l != layout->end_layers (); ++l) {

    unsigned int li = (*l).first;
    for (auto s = mp_cell->shapes (li).begin (db::ShapeIterator::AllWithProperties); ! s.at_end (); ++s) {

      const tl::Variant &v = db::properties (s->prop_id ()) [pin_property_name_id];
      if (v.is_user<db::LayoutPin> ()) {

        const db::LayoutPin &pin = v.to_user<db::LayoutPin> ();
        if (pin.name_id () != 0) {

          auto name_id = db::property_names_id (pin.basic_name ());
          db::LayoutConnectivityPin &pin_info = m_pins [name_id];
          if (pin.must_connect ()) {
            pin_info.must_connect = true;
          }
          pin_info.pin_shapes.push_front (std::make_pair (li, *s));

        }

      }

    }

  }
}

void
LayoutConnectivityIndex::ensure_nets () const
{
  if (m_nets_available && m_pins_available) {
    return;
  }

  ensure_pins ();

  m_nets_available = true;

  const db::Layout *layout = mp_cell ? mp_cell->layout () : 0;
  if (! layout) {
    return;
  }

  //  search for annotated shapes and sort into pin shapes and ordinary shapes

  for (auto l = layout->begin_layers (); l != layout->end_layers (); ++l) {

    unsigned int li = (*l).first;
    for (auto s = mp_cell->shapes (li).begin (db::ShapeIterator::AllWithProperties); ! s.at_end (); ++s) {

      const db::PropertiesSet &ps = db::properties (s->prop_id ());

      const auto &pi = ps [pin_property_name_id];
      if (pi.is_user<db::LayoutPin> ()) {

        const db::LayoutPin &lp = pi.to_user<db::LayoutPin> ();

        db::LayoutConnectivityNet &net_info = m_nets [lp.net_name_id ()];

        db::LayoutConnectivityNet::PinShape pin_shape;
        pin_shape.layer = li;
        pin_shape.pin_name = lp.name_id ();
        pin_shape.shape = *s;
        net_info.pins.push_back (pin_shape);

      } else {

        const tl::Variant &v = ps [shape_net_property_name_id];
        if (! v.is_nil ()) {

          db::property_names_id_type net_name_id = db::property_names_id (v);
          db::LayoutConnectivityNet &net_info = m_nets [net_name_id];

          db::LayoutConnectivityNet::NetShape net_shape;
          net_shape.layer = li;
          net_shape.shape = *s;
          net_info.shapes.push_back (net_shape);

        }

      }

    }

  }

  //  fill in the instance connections from the instance annotations

  for (auto i = mp_cell->begin (); ! i.at_end (); ++i) {

    const tl::Variant &v = db::properties (i->prop_id ()) [instance_connections_property_name_id];
    if (v.is_user<db::LayoutInstanceConnections> ()) {

      const db::LayoutInstanceConnections &ic = v.to_user<db::LayoutInstanceConnections> ();
      for (auto c = ic.begin (); c != ic.end (); ++c) {
        LayoutConnectivityNet::InstancePin ip;
        ip.instance = *i;
        ip.pin_name = c->first;
        m_nets [c->second].instance_pins.push_front (ip);
      }

    }

  }
}

const LayoutConnectivityNet *
LayoutConnectivityIndex::net_by_name (const std::string &name) const
{
  return net_by_name (db::property_names_id (name));
}

const LayoutConnectivityNet *
LayoutConnectivityIndex::net_by_name (db::property_names_id_type name_id) const
{
  auto i = m_nets.find (name_id);
  if (i != m_nets.end ()) {
    return &i->second;
  } else {
    return 0;
  }
}

db::LayoutConnectivityIndex::net_iterator
LayoutConnectivityIndex::begin_nets () const
{
  ensure_nets ();
  return m_nets.begin ();
}

db::LayoutConnectivityIndex::net_iterator
LayoutConnectivityIndex::end_nets () const
{
  ensure_nets ();
  return m_nets.end ();
}

const LayoutConnectivityPin *
LayoutConnectivityIndex::pin_by_name (const std::string &name) const
{
  return pin_by_name (db::property_names_id (name));
}

const LayoutConnectivityPin *
LayoutConnectivityIndex::pin_by_name (db::property_names_id_type name_id) const
{
  auto i = m_pins.find (name_id);
  if (i != m_pins.end ()) {
    return &i->second;
  } else {
    return 0;
  }
}

db::LayoutConnectivityIndex::pin_iterator
LayoutConnectivityIndex::begin_pins () const
{
  ensure_pins ();
  return m_pins.begin ();
}

db::LayoutConnectivityIndex::pin_iterator
LayoutConnectivityIndex::end_pins () const
{
  ensure_pins ();
  return m_pins.end ();
}

}

