
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

#ifndef _HDR_dbNetlistObject
#define _HDR_dbNetlistObject

#include "dbCommon.h"
#include "tlObject.h"
#include "tlVariant.h"

#include <list>
#include <string>

namespace db
{

/**
 *  @brief A base class for a objects in the netlist
 */
class DB_PUBLIC NetlistObject
  : public tl::Object
{
public:
  typedef std::map<tl::Variant, tl::Variant> property_table;
  typedef property_table::const_iterator property_iterator;

  /**
   *  @brief Default constructor
   */
  NetlistObject ();

  /**
   *  @brief Copy constructor
   */
  NetlistObject (const db::NetlistObject &object);

  /**
   *  @brief Destructor
   */
  ~NetlistObject ();

  /**
   *  @brief Assignment
   */
  NetlistObject &operator= (const NetlistObject &other);

  /**
   *  @brief Gets the property value for a given key
   *  Returns nil if there is no property for the given key.
   */
  tl::Variant property (const tl::Variant &key) const;

  /**
   *  @brief Sets the property value for a given key
   *  Set the value to nil to clear a specific key
   */
  void set_property (const tl::Variant &key, const tl::Variant &value);

  /**
   *  @brief Iterator for the netlist properties (begin)
   */
  property_iterator begin_properties () const;

  /**
   *  @brief Iterator for the netlist properties (end)
   */
  property_iterator end_properties () const;

private:
  property_table *mp_properties;
};

}

#endif
