
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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


#include "dbPropertiesRepository.h"
#include "dbLayoutStateModel.h"
#include "tlException.h"
#include "tlString.h"
#include "tlAssert.h"
#include "tlHash.h"

namespace db
{

// ----------------------------------------------------------------------------------
//  Static methods implementation

const tl::Variant &property_name (db::property_names_id_type id)
{
  if (id == 0) {
    static tl::Variant nil;
    return nil;
  } else {
    return *(reinterpret_cast <const tl::Variant *> (id));
  }
}

db::property_names_id_type property_names_id (const tl::Variant &pn)
{
  return PropertiesRepository::instance ().prop_name_id (pn);
}

const tl::Variant &property_value (db::property_values_id_type id)
{
  if (id == 0) {
    static tl::Variant nil;
    return nil;
  } else {
    return *(reinterpret_cast <const tl::Variant *> (id));
  }
}

db::property_values_id_type property_values_id (const tl::Variant &pv)
{
  return PropertiesRepository::instance ().prop_value_id (pv);
}

const PropertiesSet &properties (db::properties_id_type id)
{
  if (id == 0) {
    static db::PropertiesSet empty;
    return empty;
  } else {
    return *(reinterpret_cast <const PropertiesSet *> (id));
  }
}

db::properties_id_type properties_id (const PropertiesSet &ps)
{
  return PropertiesRepository::instance ().properties_id (ps);
}

size_t hash_for_properties_id (properties_id_type id)
{
  return id == 0 ? 0 : db::properties (id).hash ();
}

bool properties_id_less (properties_id_type a, properties_id_type b)
{
  if (a == b) {
    return false;
  }

  if (a == 0 || b == 0) {
    return a < b;
  }

  size_t ha = hash_for_properties_id (a);
  size_t hb = hash_for_properties_id (b);
  if (ha != hb) {
    return ha < hb;
  }

  //  This is the unlikely case of identical hash, but different value
  return db::properties (a).to_map () < db::properties (b).to_map ();
}

// ----------------------------------------------------------------------------------
//  PropertiesSet implementation

PropertiesSet::PropertiesSet ()
  : m_map (), m_hash (0)
{
  //  .. nothing yet ..
}

PropertiesSet::PropertiesSet (const PropertiesSet &other)
  : m_map (other.m_map), m_hash (other.m_hash)
{
  //  .. nothing yet ..
}

PropertiesSet::PropertiesSet (const PropertiesSet &&other)
  : m_map (std::move (other.m_map)), m_hash (other.m_hash)
{
  //  .. nothing yet ..
}

PropertiesSet &
PropertiesSet::operator= (const PropertiesSet &other)
{
  m_map = other.m_map;
  m_hash = other.m_hash;
  return *this;
}

PropertiesSet &
PropertiesSet::operator= (const PropertiesSet &&other)
{
  m_map = std::move (other.m_map);
  m_hash = other.m_hash;
  return *this;
}

bool
PropertiesSet::operator== (const PropertiesSet &other) const
{
  return m_map == other.m_map;
}

bool
PropertiesSet::operator< (const PropertiesSet &other) const
{
  return m_map < other.m_map;
}

bool
PropertiesSet::has_value (const tl::Variant &name) const
{
  db::property_names_id_type nid = db::property_names_id (name);
  return m_map.find (nid) != m_map.end ();
}

bool
PropertiesSet::has_value (db::property_names_id_type nid) const
{
  return m_map.find (nid) != m_map.end ();
}

const tl::Variant &
PropertiesSet::value (const tl::Variant &name) const
{
  db::property_names_id_type nid = db::property_names_id (name);
  auto i = m_map.find (nid);
  if (i == m_map.end () || i->first != nid) {
    static tl::Variant nil;
    return nil;
  } else {
    return property_value (i->second);
  }
}

const tl::Variant &
PropertiesSet::value (db::property_names_id_type nid) const
{
  auto i = m_map.find (nid);
  if (i == m_map.end () || i->first != nid) {
    static tl::Variant nil;
    return nil;
  } else {
    return property_value (i->second);
  }
}

void
PropertiesSet::clear ()
{
  m_map.clear ();
}

void
PropertiesSet::erase (const tl::Variant &name)
{
  db::property_names_id_type nid = db::property_names_id (name);
  auto i = m_map.find (nid);
  auto ii = i;
  while (i != m_map.end () && i->first == nid) {
    ++i;
  }
  if (i != ii) {
    m_map.erase (ii, i);
  }
}

void
PropertiesSet::erase (db::property_names_id_type nid)
{
  auto i = m_map.find (nid);
  auto ii = i;
  while (i != m_map.end () && i->first == nid) {
    ++i;
  }
  if (i != ii) {
    m_map.erase (ii, i);
  }
}

void
PropertiesSet::insert (const tl::Variant &name, const tl::Variant &value)
{
  m_map.insert (std::make_pair (db::property_names_id (name), db::property_values_id (value)));
}

void
PropertiesSet::insert (db::property_names_id_type nid, const tl::Variant &value)
{
  m_map.insert (std::make_pair (nid, db::property_values_id (value)));
}

void
PropertiesSet::insert_by_id (db::property_names_id_type nid, db::property_values_id_type vid)
{
  m_map.insert (std::make_pair (nid, vid));
}

void
PropertiesSet::merge (const db::PropertiesSet &other)
{
  m_map.insert (other.m_map.begin (), other.m_map.end ());
}

std::multimap<tl::Variant, tl::Variant>
PropertiesSet::to_map () const
{
  std::multimap<tl::Variant, tl::Variant> result;

  for (auto i = m_map.begin (); i != m_map.end (); ++i) {
    result.insert (std::make_pair (db::property_name (i->first), db::property_value (i->second)));
  }
  return result;
}

tl::Variant
PropertiesSet::to_dict_var () const
{
  tl::Variant var = tl::Variant::empty_array ();
  for (auto i = m_map.begin (); i != m_map.end (); ++i) {
    var.insert (db::property_name (i->first), db::property_value (i->second));
  }
  return var;
}

tl::Variant
PropertiesSet::to_list_var () const
{
  tl::Variant var = tl::Variant::empty_list ();
  auto map = to_map ();
  for (auto i = map.begin (); i != map.end (); ++i) {
    tl::Variant el = tl::Variant::empty_list ();
    el.push (i->first);
    el.push (i->second);
    var.push (el);
  }
  return var;
}

size_t
PropertiesSet::hash () const
{
  if (empty ()) {
    return 0;
  }

  if (! m_hash) {

    static tl::Mutex lock;
    tl::MutexLocker locker (&lock);

    if (! m_hash) {
      m_hash = tl::hfunc (to_map ());
      if (! m_hash) {
        //  avoid 0 value as this is reserved for "not computed yet"
        m_hash = size_t (1);
      }
    }

  }

  return m_hash;
}

// ----------------------------------------------------------------------------------
//  PropertiesRepository implementation

static PropertiesRepository s_instance;
static PropertiesRepository *sp_temp_instance = 0;

PropertiesRepository &
PropertiesRepository::instance ()
{
  return sp_temp_instance ? *sp_temp_instance : s_instance;
}

void
PropertiesRepository::replace_instance_temporarily (db::PropertiesRepository *temp)
{
  sp_temp_instance = temp;
}

PropertiesRepository::PropertiesRepository ()
{
  //  .. nothing yet ..
}

std::pair<bool, property_names_id_type>
PropertiesRepository::get_id_of_name (const tl::Variant &name) const
{
  tl::MutexLocker locker (&m_lock);

  std::set<const tl::Variant *>::const_iterator pi = m_propnames.find (&name);
  if (pi == m_propnames.end ()) {
    return std::make_pair (false, property_names_id_type (0));
  } else {
    return std::make_pair (true, property_names_id_type (*pi));
  }
}

std::pair<bool, property_values_id_type>
PropertiesRepository::get_id_of_value (const tl::Variant &value) const
{
  tl::MutexLocker locker (&m_lock);

  std::set<const tl::Variant *>::const_iterator pi = m_propvalues.find (&value);
  if (pi == m_propvalues.end ()) {
    return std::make_pair (false, property_values_id_type (0));
  } else {
    return std::make_pair (true, property_values_id_type (*pi));
  }
}

property_names_id_type
PropertiesRepository::prop_name_id (const tl::Variant &name)
{
  tl::MutexLocker locker (&m_lock);

  std::set<const tl::Variant *>::const_iterator pi = m_propnames.find (&name);
  if (pi == m_propnames.end ()) {
    m_property_names_heap.push_back (name);
    const tl::Variant &new_name = m_property_names_heap.back ();
    m_propnames.insert (&new_name);
    return property_names_id_type (&new_name);
  } else {
    return property_names_id_type (*pi);
  }
}

property_values_id_type
PropertiesRepository::prop_value_id (const tl::Variant &value)
{
  tl::MutexLocker locker (&m_lock);

  std::set<const tl::Variant *>::const_iterator pi = m_propvalues.find (&value);
  if (pi == m_propvalues.end ()) {
    m_property_values_heap.push_back (value);
    const tl::Variant &new_value = m_property_values_heap.back ();
    m_propvalues.insert (&new_value);
    return property_values_id_type (&new_value);
  } else {
    return property_values_id_type (*pi);
  }
}


properties_id_type
PropertiesRepository::properties_id (const PropertiesSet &props)
{
  if (props.empty ()) {
    return 0;
  }

  properties_id_type pid;

  {
    tl::MutexLocker locker (&m_lock);

    std::set <const PropertiesSet *>::const_iterator pi = m_properties.find (&props);
    if (pi == m_properties.end ()) {

      m_properties_heap.push_back (props);
      const PropertiesSet &new_props = m_properties_heap.back ();
      m_properties.insert (&new_props);

      pid = db::properties_id_type (&new_props);
      for (auto nv = props.begin (); nv != props.end (); ++nv) {
        m_properties_by_name_table [nv->first].insert (pid);
        m_properties_by_value_table [nv->second].insert (pid);
      }

    } else {
      pid = db::properties_id_type (*pi);
    }
  }

  return pid;
}

bool
PropertiesRepository::is_valid_properties_id (properties_id_type id) const
{
  if (id == 0) {
    return true;
  }

  tl::MutexLocker locker (&m_lock);
  for (auto i = m_properties.begin (); i != m_properties.end (); ++i) {
    if (properties_id_type (*i) == id) {
      return true;
    }
  }
  return false;
}

bool
PropertiesRepository::is_valid_property_names_id (property_names_id_type id) const
{
  tl::MutexLocker locker (&m_lock);
  for (auto i = m_propnames.begin (); i != m_propnames.end (); ++i) {
    if (property_names_id_type (*i) == id) {
      return true;
    }
  }
  return false;
}

bool
PropertiesRepository::is_valid_property_values_id (property_values_id_type id) const
{
  tl::MutexLocker locker (&m_lock);
  for (auto i = m_propvalues.begin (); i != m_propvalues.end (); ++i) {
    if (property_names_id_type (*i) == id) {
      return true;
    }
  }
  return false;
}

PropertiesRepository::properties_id_set
PropertiesRepository::properties_ids_by_name (db::property_names_id_type name_id) const
{
  tl::MutexLocker locker (&m_lock);

  auto ni = m_properties_by_name_table.find (name_id);
  if (ni == m_properties_by_name_table.end ()) {
    return properties_id_set ();
  } else {
    return ni->second;
  }
}

PropertiesRepository::properties_id_set
PropertiesRepository::properties_ids_by_value (db::property_values_id_type value_id) const
{
  tl::MutexLocker locker (&m_lock);

  auto vi = m_properties_by_value_table.find (value_id);
  if (vi == m_properties_by_value_table.end ()) {
    return properties_id_set ();
  } else {
    return vi->second;
  }
}

PropertiesRepository::properties_id_set
PropertiesRepository::properties_ids_by_name_value (db::property_names_id_type name_id, db::property_values_id_type value_id) const
{
  tl::MutexLocker locker (&m_lock);

  auto ni = m_properties_by_name_table.find (name_id);
  if (ni == m_properties_by_name_table.end ()) {
    return properties_id_set ();
  }

  auto vi = m_properties_by_value_table.find (value_id);
  if (vi == m_properties_by_value_table.end ()) {
    return properties_id_set ();
  }

  properties_id_set result;

  //  find the property sets in the intersection of ni->second and vi->second that contain the
  //  combination of name and value.

  for (auto n = ni->second.begin (); n != ni->second.end (); ++n) {

    auto vv = vi->second.find (*n);
    if (vv != vi->second.end ()) {

      const db::PropertiesSet &props = db::properties (*n);
      for (auto p = props.find (name_id); p != props.end () && p->first == name_id; ++p) {
        if (p->second == value_id) {
          result.insert (*n);
          break;
        }
      }

    }

  }

  return result;
}

// ----------------------------------------------------------------------------------
//  PropertiesTranslator implementation

PropertiesTranslator::PropertiesTranslator ()
  : m_pass (true), m_null (true)
{
  //  .. nothing yet ..
}

PropertiesTranslator::PropertiesTranslator (bool pass)
  : m_pass (pass), m_null (false)
{
  //  .. nothing yet ..
}

PropertiesTranslator::PropertiesTranslator (const std::map<db::properties_id_type, db::properties_id_type> &map)
  : m_map (map), m_pass (false), m_null (false)
{
  //  .. nothing yet ..
}

PropertiesTranslator
PropertiesTranslator::operator* (const PropertiesTranslator &other) const
{
  if (other.m_pass) {

    //  NOTE: by handling this first, "pass_all * null" will give "pass_all" which is desired
    //  for RecursiveShapeIterator::apply_property_translator.
    return *this;

  } else if (m_pass) {

    return other;

  } else {

    std::map<db::properties_id_type, db::properties_id_type> new_map;

    for (auto i = other.m_map.begin (); i != other.m_map.end (); ++i) {
      auto ii = m_map.find (i->second);
      if (ii != m_map.end ()) {
        new_map.insert (std::make_pair (i->first, ii->second));
      }
    }

    return PropertiesTranslator (new_map);

  }
}

db::properties_id_type
PropertiesTranslator::operator() (db::properties_id_type id) const
{
  if (m_pass || id == 0) {
    return id;
  } else {
    auto i = m_map.find (id);
    return i != m_map.end () ? i->second : 0;
  }
}

PropertiesTranslator
PropertiesTranslator::make_remove_all ()
{
  return PropertiesTranslator (false);
}

PropertiesTranslator
PropertiesTranslator::make_pass_all ()
{
  return PropertiesTranslator (true);
}

PropertiesTranslator
PropertiesTranslator::make_filter (const std::set<tl::Variant> &keys, db::PropertiesRepository &repo)
{
  db::PropertiesRepository::properties_id_set ids;
  std::set<db::property_names_id_type> names_selected;

  for (auto k = keys.begin (); k != keys.end (); ++k) {

    db::property_names_id_type nid = repo.prop_name_id (*k);
    names_selected.insert (nid);

    db::PropertiesRepository::properties_id_set ids_with_name = repo.properties_ids_by_name (nid);
    ids.insert (ids_with_name.begin (), ids_with_name.end ());

  }

  std::map<db::properties_id_type, db::properties_id_type> map;

  for (auto i = ids.begin (); i != ids.end (); ++i) {

    const db::PropertiesSet &props = db::properties (*i);
    db::PropertiesSet new_props;

    for (auto p = props.begin (); p != props.end (); ++p) {
      if (names_selected.find (p->first) != names_selected.end ()) {
        new_props.insert_by_id (p->first, p->second);
      }
    }

    if (! new_props.empty ()) {
      map.insert (std::make_pair (*i, new_props == props ? *i : repo.properties_id (new_props)));
    }

  }

  return PropertiesTranslator (map);
}

PropertiesTranslator
PropertiesTranslator::make_key_mapper (const std::map<tl::Variant, tl::Variant> &keys, db::PropertiesRepository &repo)
{
  db::PropertiesRepository::properties_id_set ids;
  std::map<db::property_names_id_type, db::property_names_id_type> name_map;

  for (auto k = keys.begin (); k != keys.end (); ++k) {

    db::property_names_id_type nid = repo.prop_name_id (k->first);
    name_map.insert (std::make_pair (nid, repo.prop_name_id (k->second)));

    db::PropertiesRepository::properties_id_set ids_with_name = repo.properties_ids_by_name (nid);
    ids.insert (ids_with_name.begin (), ids_with_name.end ());

  }

  std::map<db::properties_id_type, db::properties_id_type> map;

  for (auto i = ids.begin (); i != ids.end (); ++i) {

    const db::PropertiesSet &props = db::properties (*i);
    db::PropertiesSet new_props;

    for (auto p = props.begin (); p != props.end (); ++p) {
      auto nm = name_map.find (p->first);
      if (nm != name_map.end ()) {
        new_props.insert_by_id (nm->second, p->second);
      }
    }

    if (! new_props.empty ()) {
      map.insert (std::make_pair (*i, new_props == props ? *i : repo.properties_id (new_props)));
    }

  }

  return PropertiesTranslator (map);
}

} // namespace db

