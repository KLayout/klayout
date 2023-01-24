
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

#ifndef _HDR_dbLayoutToNetlistEnums
#define _HDR_dbLayoutToNetlistEnums

namespace db
{

/**
 *  @brief An enum describing the way how net information is attached to shapes as properties in "build_nets"
 */
enum NetPropertyMode
{
  /**
   *  @brief Do no generate properties
   */
  NPM_NoProperties,

  /**
   *  @brief Attach all net properties plus the net name (if a "netname_prop" is specified to "build_nets")
   */
  NPM_AllProperties,

  /**
   *  @brief Attach net name only (if a "netname_prop" is specified to "build_nets")
   */
  NPM_NetNameOnly,

  /**
   *  @brief Like NetNameOnly, but use a unique net ID (db::Net address actually) instead of name
   */
  NPM_NetIDOnly,

  /**
   *  @brief Like NetNameOnly, but use a tuple of net and circuit name
   */
  NPM_NetQualifiedNameOnly,
};

/**
 *  @brief An enum describing the way the net hierarchy is mapped
 */
enum BuildNetHierarchyMode
{
  /**
   *  @brief Flatten the net
   *  Collects all shapes of a net and puts that into the net cell or circuit cell
   */
  BNH_Flatten = 0,
  /**
   *  @brief Build a net hierarchy adding cells for each subcircuit on the net
   *  Uses the circuit_cell_prefix to build the subcircuit cell names
   */
  BNH_SubcircuitCells = 1,
  /**
   *  @brief No hierarchy
   *  Just output the shapes of the net belonging to the circuit cell.
   *  Connections are not indicated!
   */
  BNH_Disconnected = 2
};

}

#endif
