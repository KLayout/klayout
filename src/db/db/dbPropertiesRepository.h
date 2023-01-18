
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


#ifndef HDR_dbPropertiesRepository
#define HDR_dbPropertiesRepository

#include "dbCommon.h"
#include "dbTypes.h"
#include "dbMemStatistics.h"

#include "tlVariant.h"

#include <vector>
#include <string>
#include <map>

namespace db
{

class LayoutStateModel;

/**
 *  @brief The properties repository
 *
 *  This repository associates a set of property name/value pairs with 
 *  an unique Id which can be stored with a object_with_properties element.
 *  For performance reasons property names (which are strings) are not
 *  stored as such but as integers.
 */

class DB_PUBLIC PropertiesRepository
{
public:
  typedef std::multimap <property_names_id_type, tl::Variant> properties_set;
  typedef std::map <properties_id_type, properties_set> properties_map;
  typedef std::map <properties_id_type, properties_set>::const_iterator iterator;
  typedef std::map <properties_id_type, properties_set>::iterator non_const_iterator;
  typedef std::pair <property_names_id_type, tl::Variant> name_value_pair;
  typedef std::vector <properties_id_type> properties_id_vector;

  /**
   *  @brief Default constructor
   */
  PropertiesRepository (db::LayoutStateModel *state_model = 0);

  /**
   *  @brief Copy constructor
   */
  PropertiesRepository (const PropertiesRepository &d);

  /**
   *  @brief Assignment
   */
  PropertiesRepository &operator= (const PropertiesRepository &d);

  /**
   *  @brief Associate a name with a name Id
   * 
   *  A name of a property can be either a integer or a string as specified
   *  with the tl::Variant variant. In principle it could even be a list or void.
   *  This method will assign a new Id to the given name if required and
   *  return the Id associated with it.
   */
  property_names_id_type prop_name_id (const tl::Variant &name);
  
  /**
   *  @brief Change the name associated with a given name Id to another name
   * 
   *  All properties with the given name Id will get the new name. This method is
   *  particular useful for the OASIS reader, which may associate a name with an Id 
   *  at a late stage. The new name must not be associated with an Id already.
   *  The old name will stay associated with the Id.
   */
  void change_name (property_names_id_type id, const tl::Variant &new_name);
  
  /**
   *  @brief Change the properties for a given Id
   * 
   *  This method will change the properties for a given Id. The properties
   *  set for Id 0 cannot be changed.
   *
   *  This method is intended for special applications, i.e. the OASIS reader
   *  in forware references mode.
   */
  void change_properties (properties_id_type id, const properties_set &new_props);

  /**
   *  @brief Get the id for a name 
   * 
   *  This method checks whether the given name is present as a name and returns the 
   *  id in the second member of the pair. The first member is true, if the name is
   *  present.
   */
  std::pair<bool, property_names_id_type> get_id_of_name (const tl::Variant &name) const;
  
  /**
   *  @brief Associate a name with a name Id
   * 
   *  This method will return the name associated with the given Id.
   *  It will assert if the Id is not a valid one.
   */
  const tl::Variant &prop_name (property_names_id_type id) const;
  
  /**
   *  @brief Associate a properties set with a properties Id
   * 
   *  This method will assign a new Id to the given set if required and
   *  return the Id associated with it.
   *  An empty property set is associated with property Id 0.
   */
  properties_id_type properties_id (const properties_set &props);
  
  /**
   *  @brief Associate a properties set with a properties Id
   * 
   *  This method will return the properties set associated with the given Id.
   *  Id 0 always delivers an empty property set.
   */
  const properties_set &properties (properties_id_type id) const;

  /**
   *  @brief Determine if the given Id is a valid one
   */
  bool is_valid_properties_id (properties_id_type id) const;

  /**
   *  @brief Iterate over Id/Properties sets (non-const)
   */
  non_const_iterator begin_non_const () 
  {
    return m_properties_by_id.begin ();
  }

  /**
   *  @brief Iterate over Id/Properties sets: end iterator (non-const)
   */
  non_const_iterator end_non_const () 
  {
    return m_properties_by_id.end ();
  }

  /**
   *  @brief Iterate over Id/Properties sets
   */
  iterator begin () const
  {
    return m_properties_by_id.begin ();
  }

  /**
   *  @brief Iterate over Id/Properties sets: end iterator
   */
  iterator end () const
  {
    return m_properties_by_id.end ();
  }

  /**
   *  @brief Obtain the first properties id in the repository
   */
  properties_id_type begin_id () 
  {
    return 0;
  }
  
  /**
   *  @brief Obtain the last properties id in the repository plus 1
   */
  properties_id_type end_id () 
  {
    return m_properties_by_id.size ();
  }
  
  /**
   *  @brief Lookup a table of properties id's by a name value pair
   *
   *  For a given name/value pair, this method returns a vector of ids
   *  of property sets that contain the given name/value pair. This method
   *  is intended for use with the properties_id resolution algorithm.
   */
  const properties_id_vector &properties_ids_by_name_value (const name_value_pair &nv) const;

  /**
   *  @brief Translate a properties id from one repository to this one
   *
   *  Take the given properties set from one repository and map it to this one.
   *  Inserts the properties set into this repository if necessary.
   *
   *  @param rep The source repository
   *  @param id The source properties id
   *  @return id The id in *this
   */
  properties_id_type translate (const PropertiesRepository &rep, properties_id_type id);

  /**
   *  @brief Collect memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const
  {
    if (!no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }

    db::mem_stat (stat, purpose, cat, m_propnames_by_id, true, parent);
    db::mem_stat (stat, purpose, cat, m_propname_ids_by_name, true, parent);
    db::mem_stat (stat, purpose, cat, m_properties_by_id, true, parent);
    db::mem_stat (stat, purpose, cat, m_properties_ids_by_set, true, parent);
    db::mem_stat (stat, purpose, cat, m_properties_component_table, true, parent);
  }

private:
  std::map <property_names_id_type, tl::Variant> m_propnames_by_id;
  std::map <tl::Variant, property_names_id_type> m_propname_ids_by_name;

  std::map <properties_id_type, properties_set> m_properties_by_id;
  std::map <properties_set, properties_id_type> m_properties_ids_by_set;
  std::map <name_value_pair, properties_id_vector> m_properties_component_table;

  db::LayoutStateModel *mp_state_model;
};

/**
 *  @brief A map for selecting/translating properties
 *
 *  The following rules apply:
 *  - All non-mapped properties are mapped to 0 (removed)
 *  - 0 is always mapped to 0
 *  - Do not include key or value 0 in the map passed to the constructor
 *
 *  A "pass translator" will pass all IDs unchanged.
 *
 *  Note that a property translator - specifically the filters and
 *  mappers created by "make_filter" and "make_key_mapper" - are snapshots.
 *  As creating new filters will generate new property IDs for the mapping
 *  targets, property translators generated previously may become invalid.
 *  In general it is safe to concatenate new translators after old ones.
 *  The old ones will not map the property IDs understood by the new ones,
 *  but as such IDs cannot become input to the old translator, this should
 *  not matter.
 */

class DB_PUBLIC PropertiesTranslator
{
public:
  /**
   *  @brief Default constructor - this creates a null translator
   */
  PropertiesTranslator ();

  /**
   *  @brief Creates a "pass all" (pass = true) or "remove all" (pass = false) translator
   */
  PropertiesTranslator (bool pass);

  /**
   *  @brief Creates a property ID mapper from a table
   */
  PropertiesTranslator (const std::map<db::properties_id_type, db::properties_id_type> &map);

  /**
   *  @brief Gets a value indicating whether the translator is "pass"
   */
  bool is_pass () const
  {
    return m_pass;
  }

  /**
   *  @brief Gets a value indicating whether the translator is "empty" (remove all)
   */
  bool is_empty () const
  {
    return ! m_pass && m_map.empty ();
  }

  /**
   *  @brief Gets a value indicating whether the translator is "null" (default-constructed)
   */
  bool is_null () const
  {
    return m_null;
  }

  /**
   *  @brief Concatenates two translators (the right one first)
   */
  PropertiesTranslator operator* (const PropertiesTranslator &other) const;

  /**
   *  @brief Concatenates two translators (the right one first) - in place version
   */
  PropertiesTranslator &operator*= (const PropertiesTranslator &other)
  {
    *this = this->operator* (other);
    return *this;
  }

  /**
   *  @brief Translation of the property ID
   */
  db::properties_id_type operator() (db::properties_id_type id) const;

  /**
   *  @brief Factory: create a "remove all" translator
   */
  static PropertiesTranslator make_remove_all ();

  /**
   *  @brief Factory: create a "pass all" translator
   */
  static PropertiesTranslator make_pass_all ();

  /**
   *  @brief Factory: create a filter translator
   *
   *  The translator delivered by this function will leave only the given keys in the properties.
   */
  static PropertiesTranslator make_filter (db::PropertiesRepository &repo, const std::set<tl::Variant> &keys);

  /**
   *  @brief Factory: create a key mapper translator
   *
   *  The translator delivered by this function will translate the given keys to new ones
   *  and remove non-listed keys.
   */
  static PropertiesTranslator make_key_mapper (db::PropertiesRepository &repo, const std::map<tl::Variant, tl::Variant> &keys);

private:
  std::map<db::properties_id_type, db::properties_id_type> m_map;
  bool m_pass, m_null;
};

/**
 *  @brief Collect memory statistics
 */
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const PropertiesRepository &x, bool no_self = false, void *parent = 0)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

} // namespace db

#endif

