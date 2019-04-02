
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


#ifndef HDR_dbLayerProperties
#define HDR_dbLayerProperties

#include "dbCommon.h"

#include "tlString.h"
#include "tlTypeTraits.h"

#include <string>

namespace db
{

/**
 *  @brief A layer property
 *
 *  The layer properties are basically to be used for storing of layer name and 
 *  layer/datatype information.
 */
struct DB_PUBLIC LayerProperties
{
  /**
   *  @brief Default constructor
   */
  LayerProperties ();
  
  /**
   *  @brief Constructor with layer and datatype
   */
  LayerProperties (int l, int d);
  
  /**
   *  @brief Constructor with name
   */
  LayerProperties (const std::string &n);
  
  /**
   *  @brief Constructor with layer and datatype and name
   */
  LayerProperties (int l, int d, const std::string &n);

  /**
   *  @brief Returns true, if the layer specification is not a null specification
   *
   *  A null specification is one created by the default constructor. It does not have
   *  a layer, datatype or name assigned.
   */
  bool is_null () const
  {
    return layer < 0 && datatype < 0 && name.empty ();
  }
  
  /**
   *  @brief Return true, if the layer is specified by name only
   */
  bool is_named () const;

  /**
   *  @brief Convert to a string
   */
  std::string to_string () const;

  /**
   *  @brief Extract from a tl::Extractor
   */
  void read (tl::Extractor &ex);

  /**
   *  @brief "Logical" equality
   *
   *  This currently reflects only equality of layers and datatypes, name is of second order
   *  and used only if no layer or datatype is given
   */
  bool log_equal (const LayerProperties &b) const;

  /**
   *  @brief "Logical" less operator
   *
   *  This currently reflects only order of layer and datatype, name is of second order
   */
  bool log_less (const LayerProperties &b) const;

  /**
   *  @brief Exact equality
   */
  bool operator== (const LayerProperties &b) const;

  /**
   *  @brief Exact inequality
   */
  bool operator!= (const LayerProperties &b) const;

  /**
   *  @brief Exact less operator
   */
  bool operator< (const LayerProperties &b) const;

  std::string name;
  int layer;
  int datatype;
};

/**
 *  @brief "Logical less" functor for LayerProperties
 */
struct LPLogicalLessFunc 
{
  bool operator () (const LayerProperties &a, const LayerProperties &b) const
  {
    return a.log_less (b);
  }
};

/**
 *  @brief A layer offset 
 *
 *  This struct defines a layer offset which can be "added" to a LayerProperties object
 *  If the layer offset is defined with a name, any occurrence of '*' in the string
 *  is replaced with the original name. This way, applying "*_A" with "+" yields a 
 *  postfix "_A" to the original layer name (if it is named).
 */
struct DB_PUBLIC LayerOffset
{
  /**
   *  @brief Default constructor
   */
  LayerOffset ();
  
  /**
   *  @brief Constructor with layer and datatype
   */
  LayerOffset (int l, int d);
  
  /**
   *  @brief Constructor with name
   */
  LayerOffset (const std::string &n);
  
  /**
   *  @brief Constructor with layer and datatype and name
   */
  LayerOffset (int l, int d, const std::string &n);
  
  /**
   *  @brief Return true, if the layer is specified by name only
   */
  bool is_named () const;

  /**
   *  @brief Convert to a string
   */
  std::string to_string () const;

  /**
   *  @brief Extract from a tl::Extractor
   */
  void read (tl::Extractor &ex);

  /**
   *  @brief Exact equality
   */
  bool operator== (const LayerOffset &b) const;

  /**
   *  @brief Exact inequality
   */
  bool operator!= (const LayerOffset &b) const;

  /**
   *  @brief Exact less operator
   */
  bool operator< (const LayerOffset &b) const;

  /**
   *  @brief Apply to a LayerProperties object
   */
  LayerProperties apply (const LayerProperties &props) const;

  std::string name;
  int layer;
  int datatype;
};

/**
 *  @brief Apply a LayerOffset to a LayerProperties object
 */
inline LayerProperties operator+ (const LayerProperties &props, const LayerOffset &offset)
{
  return offset.apply (props);
}

/**
 *  @brief Apply a LayerOffset to a LayerProperties itself
 */
inline LayerProperties &operator+= (LayerProperties &props, const LayerOffset &offset)
{
  props = offset.apply (props);
  return props;
}

}

//  tl namespace support for db::LayerProperties
namespace tl
{
  template <>
  struct type_traits <db::LayerProperties> : public type_traits<void> 
  {
    typedef true_tag supports_extractor;
    typedef true_tag supports_to_string;
    typedef true_tag has_less_operator;
    typedef true_tag has_equal_operator;
  };

  template <>
  struct type_traits <db::LayerOffset> : public type_traits<void> 
  {
    typedef true_tag supports_extractor;
    typedef true_tag supports_to_string;
    typedef true_tag has_less_operator;
    typedef true_tag has_equal_operator;
  };
}

#endif

