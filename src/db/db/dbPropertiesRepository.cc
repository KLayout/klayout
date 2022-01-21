
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


#include "dbPropertiesRepository.h"
#include "dbLayoutStateModel.h"
#include "tlException.h"
#include "tlString.h"
#include "tlAssert.h"

namespace db
{

// ----------------------------------------------------------------------------------
//  PropertiesRepository implementation

PropertiesRepository::PropertiesRepository (db::LayoutStateModel *state_model)
  : mp_state_model (state_model)
{
  //  install empty property set
  properties_set empty_set;
  properties_id_type id = properties_id (empty_set);
  tl_assert (id == 0);
}

PropertiesRepository &
PropertiesRepository::operator= (const PropertiesRepository &d)
{
  if (&d != this) {
    m_propnames_by_id            = d.m_propnames_by_id;
    m_propname_ids_by_name       = d.m_propname_ids_by_name;
    m_properties_by_id           = d.m_properties_by_id;
    m_properties_ids_by_set      = d.m_properties_ids_by_set;
    m_properties_component_table = d.m_properties_component_table;
  }
  return *this;
}

std::pair<bool, property_names_id_type>
PropertiesRepository::get_id_of_name (const tl::Variant &name) const
{
  std::map <tl::Variant, property_names_id_type>::const_iterator pi = m_propname_ids_by_name.find (name);
  if (pi == m_propname_ids_by_name.end ()) {
    return std::make_pair (false, property_names_id_type (0));
  } else {
    return std::make_pair (true, pi->second);
  }
}

property_names_id_type 
PropertiesRepository::prop_name_id (const tl::Variant &name)
{
  std::map <tl::Variant, property_names_id_type>::const_iterator pi = m_propname_ids_by_name.find (name);
  if (pi == m_propname_ids_by_name.end ()) {
    property_names_id_type id = m_propnames_by_id.size ();
    m_propnames_by_id.insert (std::make_pair (id, name));
    m_propname_ids_by_name.insert (std::make_pair (name, id));
    return id;
  } else {
    return pi->second;
  }
}

void 
PropertiesRepository::change_properties (property_names_id_type id, const properties_set &new_props)
{
  const properties_set &old_props = properties (id);

  std::map <properties_set, properties_id_type>::const_iterator pi = m_properties_ids_by_set.find (old_props);
  if (pi != m_properties_ids_by_set.end ()) {

    //  erase the id from the component table
    for (properties_set::const_iterator nv = old_props.begin (); nv != old_props.end (); ++nv) {
      if (m_properties_component_table.find (*nv) != m_properties_component_table.end ()) {
        properties_id_vector &v = m_properties_component_table [*nv];
        for (size_t i = 0; i < v.size (); ) {
          if (v[i] == id) {
            v.erase (v.begin () + i);
          } else {
            ++i;
          }
        }
      }
    }

    //  and insert again
    m_properties_ids_by_set.erase (old_props);
    m_properties_ids_by_set.insert (std::make_pair (new_props, id));

    m_properties_by_id [id] = new_props;

    for (properties_set::const_iterator nv = new_props.begin (); nv != new_props.end (); ++nv) {
      m_properties_component_table.insert (std::make_pair (*nv, properties_id_vector ())).first->second.push_back (id);
    }

    //  signal the change of the properties ID's. This way for example, the layer views
    //  can recompute the property selectors
    if (mp_state_model) {
      mp_state_model->prop_ids_changed ();
    }

  }
}

void 
PropertiesRepository::change_name (property_names_id_type id, const tl::Variant &new_name)
{
  std::map <property_names_id_type, tl::Variant>::iterator pi = m_propnames_by_id.find (id);
  tl_assert (pi != m_propnames_by_id.end ());
  pi->second = new_name;

  m_propname_ids_by_name.insert (std::make_pair (new_name, id));
}

const tl::Variant &
PropertiesRepository::prop_name (property_names_id_type id) const
{
  return m_propnames_by_id.find (id)->second;
}

properties_id_type 
PropertiesRepository::properties_id (const properties_set &props)
{
  std::map <properties_set, properties_id_type>::const_iterator pi = m_properties_ids_by_set.find (props);
  if (pi == m_properties_ids_by_set.end ()) {

    properties_id_type id = m_properties_ids_by_set.size ();
    m_properties_ids_by_set.insert (std::make_pair (props, id));
    m_properties_by_id.insert (std::make_pair (id, props));
    for (properties_set::const_iterator nv = props.begin (); nv != props.end (); ++nv) {
      m_properties_component_table.insert (std::make_pair (*nv, properties_id_vector ())).first->second.push_back (id);
    }

    //  signal the change of the properties ID's. This way for example, the layer views
    //  can recompute the property selectors
    if (mp_state_model) {
      mp_state_model->prop_ids_changed ();
    }

    return id;

  } else {
    return pi->second;
  }
}

const PropertiesRepository::properties_set &
PropertiesRepository::properties (properties_id_type id) const
{
  iterator p = m_properties_by_id.find (id);
  if (p != m_properties_by_id.end ()) {
    return p->second;
  } else {
    static PropertiesRepository::properties_set empty_set;
    return empty_set;
  }
}

bool
PropertiesRepository::is_valid_properties_id (properties_id_type id) const
{
  return m_properties_by_id.find (id) != m_properties_by_id.end ();
}

const PropertiesRepository::properties_id_vector &
PropertiesRepository::properties_ids_by_name_value (const name_value_pair &nv) const
{
  std::map <name_value_pair, properties_id_vector>::const_iterator idv = m_properties_component_table.find (nv);
  if (idv == m_properties_component_table.end ()) {
    static properties_id_vector empty;
    return empty;
  } else {
    return idv->second;
  }
}

properties_id_type 
PropertiesRepository::translate (const PropertiesRepository &rep, properties_id_type id)
{
  const properties_set &pset = rep.properties (id);

  //  create a new set by mapping the names
  properties_set new_pset;
  for (properties_set::const_iterator pp = pset.begin (); pp != pset.end (); ++pp) {
    new_pset.insert (std::make_pair (prop_name_id (rep.prop_name (pp->first)), pp->second));
  }

  return properties_id (new_pset);
}

} // namespace db

