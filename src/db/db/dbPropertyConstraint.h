
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



#ifndef HDR_dbPropertyConstraint
#define HDR_dbPropertyConstraint

#include "dbCommon.h"
#include "dbTypes.h"

namespace db
{

/**
 *  @brief Specifies a property constraint for some operations
 */
enum PropertyConstraint
{
  /**
   *  @brief Ignore properties
   *
   *  In this mode, properties are not considered and erased.
   */
  IgnoreProperties = 0,

  /**
   *  @brief No constraint, shapes are processed regardless of their properties
   *
   *  Properties are attached to the outputs where applicable.
   */
  NoPropertyConstraint = 1,

  /**
   *  @brief Shapes are processed if their properties are the same
   *
   *  Properties are attached to the outputs where applicable.
   */
  SamePropertiesConstraint = 2,

  /**
   *  @brief Shapes are processed if their properties are the same
   *
   *  No properties are attached to the output.
   */
  SamePropertiesConstraintDrop = 3,

  /**
   *  @brief Shapes are processed if their properties are different
   *
   *  Properties are attached to the outputs where applicable.
   */
  DifferentPropertiesConstraint = 4,

  /**
   *  @brief Shapes are processed if their properties are different
   *
   *  No properties are attached to the output.
   */
  DifferentPropertiesConstraintDrop = 5
};

/**
 *  @brief Returns a predicate indicating whether properties need to be considered
 */
bool inline pc_skip (PropertyConstraint pc)
{
  return pc == IgnoreProperties;
}

/**
 *  @brief Returns a predicate indicating whether properties are always different
 */
bool inline pc_always_different (PropertyConstraint pc)
{
  return pc == DifferentPropertiesConstraint || pc == DifferentPropertiesConstraintDrop;
}

/**
 *  @brief Returns a value indicating whether two properties satisfy the condition
 */
bool inline pc_match (PropertyConstraint pc, db::properties_id_type a, db::properties_id_type b)
{
  if (pc == SamePropertiesConstraint || pc == SamePropertiesConstraintDrop) {
    return a == b;
  } else if (pc == DifferentPropertiesConstraint || pc == DifferentPropertiesConstraintDrop) {
    return a != b;
  } else {
    return true;
  }
}

/**
 *  @brief Returns a value indicating whether the property can be removed on output
 */
bool inline pc_remove (PropertyConstraint pc)
{
  return pc == IgnoreProperties || pc == SamePropertiesConstraintDrop || pc == DifferentPropertiesConstraintDrop;
}

/**
 *  @brief Returns a normalized property for output
 */
db::properties_id_type inline pc_norm (PropertyConstraint pc, db::properties_id_type prop_id)
{
  return pc_remove (pc) ? 0 : prop_id;
}

}

#endif

