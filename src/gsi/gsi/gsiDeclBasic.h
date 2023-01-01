
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


#ifndef _HDR_gsiDeclBasic
#define _HDR_gsiDeclBasic

#include <string>
#include <vector>
#include <sstream>
#include <iostream>

#include "tlVariant.h"
#include "gsiObject.h"

namespace gsi
{

/**
 *  @brief Provides a basic implementation for a "boxed" plain value using a Variant as the basic type
 */
class GSI_PUBLIC Value
  : public gsi::ObjectBase
{
public:
  /**
   *  @brief Constructor: create a "nil" object
   */
  Value () { }

  /**
   *  @brief Constructor: create an object with a value
   */
  Value (const tl::Variant &v) : m_v (v) { }

  /**
   *  @brief Obtain the value 
   */
  const tl::Variant &value () const 
  { 
    return m_v;
  }

  /**
   *  @brief Obtain the value (non-const)
   */
  tl::Variant &value () 
  { 
    return m_v;
  }

  /**
   *  @brief Set the value 
   */
  void set_value (const tl::Variant &v)
  { 
    m_v = v;
  }

  /**
   *   @brief Converts the value to a string
   */
  std::string to_string () const 
  { 
    return m_v.to_string ();
  }

protected:
  tl::Variant m_v;
};

}

#endif


