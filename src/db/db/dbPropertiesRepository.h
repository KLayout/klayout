
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


#ifndef HDR_dbPropertiesRepository
#define HDR_dbPropertiesRepository

#include "dbCommon.h"
#include "dbTypes.h"
#include "dbMemStatistics.h"

#include "tlVariant.h"
#include "tlThreads.h"
#include "tlEvents.h"

#include <vector>
#include <string>
#include <map>

namespace db
{

/**
 *  @brief Gets a name entry from the property name ID
 */
DB_PUBLIC const tl::Variant &property_name (db::property_names_id_type id);

/**
 *  @brief Gets the property name ID from a property name
 */
DB_PUBLIC db::property_names_id_type property_names_id (const tl::Variant &pn);

/**
 *  @brief Gets a value entry from the property value ID
 */
DB_PUBLIC const tl::Variant &property_value (db::property_values_id_type id);

/**
 *  @brief Gets the property value ID from a property value
 */
DB_PUBLIC db::property_values_id_type property_values_id (const tl::Variant &pv);

/**
 *  @brief Computes the hash value for a properties_id
 */
DB_PUBLIC size_t hash_for_properties_id (properties_id_type id);

/**
 *  @brief A less compare function implementation that compares the properties IDs by value
 */
DB_PUBLIC bool properties_id_less (properties_id_type a, properties_id_type b);

/**
 *  @brief A compare function for property IDs
 */
struct ComparePropertiesIds
{
  bool operator() (properties_id_type a, properties_id_type b) const
  {
    return properties_id_less (a, b);
  }
};

/**
 *  @brief A properties set
 *
 *  The properties set is
 */

class DB_PUBLIC PropertiesSet
{
public:
  typedef std::multimap<db::property_names_id_type, db::property_values_id_type> map_type;
  typedef map_type::const_iterator iterator;
  typedef map_type::iterator non_const_iterator;

  /**
   *  @brief The default constructor
   *
   *  This will create an empty properties set.
   */
  PropertiesSet ();

  /**
   *  @brief Copy constructor
   */
  PropertiesSet (const PropertiesSet &other);

  /**
   *  @brief Move constructor
   */
  PropertiesSet (const PropertiesSet &&other);

  /**
   *  @brief Constructor from tl::Variant pair iterator
   */
  template <class Iter>
  PropertiesSet (const Iter &from, const Iter &to)
    : m_map (), m_hash (0)
  {
    for (auto i = from; i != to; ++i) {
      insert (i->first, i->second);
    }
  }

  /**
   *  @brief Assignment
   */
  PropertiesSet &operator= (const PropertiesSet &other);

  /**
   *  @brief Move assignment
   */
  PropertiesSet &operator= (const PropertiesSet &&other);

  /**
   *  @brief Equality
   */
  bool operator== (const PropertiesSet &other) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const PropertiesSet &other) const
  {
    return ! operator== (other);
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const PropertiesSet &other) const;

  /**
   *  @brief Swaps with another properties set
   */
  void swap (PropertiesSet &other)
  {
    m_map.swap (other.m_map);
  }

  /**
   *  @brief Gets a value indicating whether the properties set is empty
   */
  bool empty () const
  {
    return m_map.empty ();
  }

  /**
   *  @brief Gets the size of the properties set
   */
  size_t size () const
  {
    return m_map.size ();
  }

  /**
   *  @brief Gets a value indicating whether the given name is contained in the set
   */
  bool has_value (const tl::Variant &name) const;

  /**
   *  @brief Gets a value indicating whether the given name is contained in the set
   */
  bool has_value (db::property_names_id_type name_id) const;

  /**
   *  @brief Gets the value for the given name or a nil variant if there is no value for this name
   */
  const tl::Variant &value (const tl::Variant &name) const;

  /**
   *  @brief Gets the value for the given name or a nil variant if there is no value for this name
   */
  const tl::Variant &value (db::property_names_id_type name_id) const;

  /**
   *  @brief operator[] as alias for "value"
   */
  const tl::Variant &operator[] (const tl::Variant &name) const
  {
    return value (name);
  }

  /**
   *  @brief operator[] as alias for "value"
   */
  const tl::Variant &operator[] (db::property_names_id_type name_id) const
  {
    return value (name_id);
  }

  /**
   *  @brief Clears the properties set
   */
  void clear ();

  /**
   *  @brief Deletes a value for the given name
   */
  void erase (const tl::Variant &name);

  /**
   *  @brief Deletes a value for the given name
   */
  void erase (db::property_names_id_type name_id);

  /**
   *  @brief Inserts a value for the given name
   */
  void insert (const tl::Variant &name, const tl::Variant &value);

  /**
   *  @brief Inserts a value for the given name ID
   */
  void insert (db::property_names_id_type name_id, const tl::Variant &value);

  /**
   *  @brief Inserts a value by ID for the given name ID
   */
  void insert_by_id (db::property_names_id_type name_id, db::property_values_id_type value_id);

  /**
   *  @brief Merge another properties set into self
   */
  void merge (const db::PropertiesSet &other);

  /**
   *  @brief Gets the properties as a map
   */
  std::multimap<tl::Variant, tl::Variant> to_map () const;

  /**
   *  @brief Gets the properties as a dict variant
   */
  tl::Variant to_dict_var () const;

  /**
   *  @brief Gets the properties as an array of pairs
   *
   *  In contrast to the dict version, this variant allows delivery of
   *  property set with multiple values for the same name.
   */
  tl::Variant to_list_var () const;

  /**
   *  @brief Iterator (begin)
   *
   *  This iterator delivers key/value pairs in the ID form.
   *  The order is basically undefined.
   */
  iterator begin () const
  {
    return m_map.begin ();
  }

  /**
   *  @brief Iterator (end)
   */
  iterator end () const
  {
    return m_map.end ();
  }

  /**
   *  @brief Finds and entry with the given name ID
   */
  iterator find (db::property_names_id_type name_id) const
  {
    return m_map.find (name_id);
  }

  /**
   * @brief Gets the hash value for the properties ID set
   */
  size_t hash () const;

private:
  map_type m_map;
  mutable size_t m_hash;
};

/**
 *  @brief Gets the properties set from a properties set ID
 */
DB_PUBLIC const PropertiesSet &properties (db::properties_id_type id);

/**
 *  @brief Gets the properties ID from a properties set
 */
DB_PUBLIC db::properties_id_type properties_id (const PropertiesSet &ps);

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
  typedef std::set <properties_id_type> properties_id_set;

  /**
   *  @brief Default constructor
   *
   *  This constructor is mainly provided for test purposes.
   */
  PropertiesRepository ();

  /**
   *  @brief Gets the singleton instance of the properties repository
   */
  static PropertiesRepository &instance ();

  /**
   *  @brief Temporarily replace the singleton instance
   *
   *  This method is intended for testing purposes only. Passing 0 for the
   *  repository argument resets back to the global singleton.
   */
  static void replace_instance_temporarily (db::PropertiesRepository *temp);

  /**
   *  @brief Gets the name ID for a property name
   * 
   *  This method will assign a new ID to the given name if required and
   *  return the ID associated with it.
   */
  property_names_id_type prop_name_id (const tl::Variant &name);
  
  /**
   *  @brief Gets the value ID for a property value
   *
   *  This method will assign a new ID to the given value if required and
   *  return the ID associated with it.
   */
  property_names_id_type prop_value_id (const tl::Variant &name);

  /**
   *  @brief Get the ID for a name
   * 
   *  This method checks whether the given name is present as a name and returns the 
   *  ID in the second member of the pair. The first member is true, if the name is
   *  present.
   */
  std::pair<bool, property_names_id_type> get_id_of_name (const tl::Variant &name) const;
  
  /**
   *  @brief Get the ID for a value
   *
   *  This method checks whether the given name is present as a name and returns the
   *  ID in the second member of the pair. The first member is true, if the name is
   *  present.
   */
  std::pair<bool, property_values_id_type> get_id_of_value (const tl::Variant &value) const;

  /**
   *  @brief Associate a properties set with a properties Id
   * 
   *  This method will assign a new Id to the given set if required and
   *  return the Id associated with it.
   *  An empty property set is associated with property Id 0.
   */
  properties_id_type properties_id (const PropertiesSet &props);
  
  /**
   *  @brief Determine if the given ID is a valid properties ID
   *
   *  Caution: this operation is slow!
   */
  bool is_valid_properties_id (properties_id_type id) const;

  /**
   *  @brief Determine if the given ID is a valid name ID
   *
   *  Caution: this operation is slow!
   */
  bool is_valid_property_names_id (property_names_id_type id) const;

  /**
   *  @brief Determine if the given ID is a valid value ID
   *
   *  Caution: this operation is slow!
   */
  bool is_valid_property_values_id (property_values_id_type id) const;

  /**
   *  @brief Lookup a table of properties id's by a name value pair
   *
   *  For a given name/value pair, this method returns a set of property IDs
   *  of property sets that contain the given name/value pair.
   */
  properties_id_set properties_ids_by_name_value (db::property_names_id_type name, db::property_values_id_type value) const;

  /**
   *  @brief Lookup a table of properties id's by a name
   *
   *  For a given name, this method returns a set of property IDs
   *  of property sets that contain the given name.
   */
  properties_id_set properties_ids_by_name (db::property_names_id_type name) const;

  /**
   *  @brief Lookup a table of properties id's by a value
   *
   *  For a given name, this method returns a set of property IDs
   *  of property sets that contain the given value.
   */
  properties_id_set properties_ids_by_value (db::property_values_id_type value) const;

  /**
   *  @brief Collect memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const
  {
    if (!no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }

    db::mem_stat (stat, purpose, cat, m_propnames, true, parent);
    db::mem_stat (stat, purpose, cat, m_property_names_heap, true, parent);
    db::mem_stat (stat, purpose, cat, m_propvalues, true, parent);
    db::mem_stat (stat, purpose, cat, m_property_values_heap, true, parent);
    db::mem_stat (stat, purpose, cat, m_properties, true, parent);
    db::mem_stat (stat, purpose, cat, m_properties_heap, true, parent);
    db::mem_stat (stat, purpose, cat, m_properties_by_name_table, true, parent);
    db::mem_stat (stat, purpose, cat, m_properties_by_value_table, true, parent);
  }

private:
  struct CompareNamePtrByValueForValues
  {
    bool operator() (const tl::Variant *a, const tl::Variant *b) const
    {
      //  NOTE: for values, the type should matter, so 2.0 is different from 2 (integer).
      //  Hence we use "less" here.
      return a->less (*b);
    }
  };

  struct CompareNamePtrByValueForNames
  {
    bool operator() (const tl::Variant *a, const tl::Variant *b) const
    {
      return *a < *b;
    }
  };

  struct ComparePropertiesPtrByValue
  {
    bool operator() (const PropertiesSet *a, const PropertiesSet *b) const
    {
      return *a < *b;
    }
  };

  std::set <const tl::Variant *, CompareNamePtrByValueForNames> m_propnames;
  std::list <tl::Variant> m_property_names_heap;
  std::set <const tl::Variant *, CompareNamePtrByValueForValues> m_propvalues;
  std::list <tl::Variant> m_property_values_heap;
  std::set <const PropertiesSet *, ComparePropertiesPtrByValue> m_properties;
  std::list <PropertiesSet> m_properties_heap;

  std::map <property_names_id_type, properties_id_set> m_properties_by_name_table;
  std::map <property_values_id_type, properties_id_set> m_properties_by_value_table;

  mutable tl::Mutex m_lock;
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
   *
   *  If no repository is given, the translator acts on the singleton instance.
   */
  static PropertiesTranslator make_filter (const std::set<tl::Variant> &keys, db::PropertiesRepository &repo = db::PropertiesRepository::instance ());

  /**
   *  @brief Factory: create a key mapper translator
   *
   *  The translator delivered by this function will translate the given keys to new ones
   *  and remove non-listed keys.
   *
   *  If no repository is given, the translator acts on the singleton instance.
   */
  static PropertiesTranslator make_key_mapper (const std::map<tl::Variant, tl::Variant> &keys, db::PropertiesRepository &repo = db::PropertiesRepository::instance ());

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

