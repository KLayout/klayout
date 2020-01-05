
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#include "gsiDeclBasic.h"
#include "gsiDecl.h"

namespace gsi
{


// ---------------------------------------------------------------------------------
//  A generic value wrapper that allows wrapping a plain data type into an object

static gsi::Value *new_vv (const tl::Variant &v) 
{
  return new gsi::Value (v);
}

static gsi::Value *new_vv0 () 
{
  return new gsi::Value ();
}

Class<Value> decl_Value ("tl", "Value",
  gsi::constructor ("new", &new_vv0, 
    "@brief Constructs a nil object.\n"
  ) +
  gsi::constructor ("new", &new_vv, 
    "@brief Constructs a non-nil object with the given value.\n"
    "@args value\n"
    "This constructor has been introduced in version 0.22.\n"
  ) +
  gsi::method ("to_s", &Value::to_string,
    "@brief Convert this object to a string\n"
  ) +
  gsi::method ("value=", &Value::set_value,
    "@brief Set the actual value.\n"
    "@args value\n"
  ) +
  gsi::method ("value", (const tl::Variant &(Value::*)() const) &Value::value,
    "@brief Gets the actual value.\n"
  ),
  "@brief Encapsulates a value (preferably a plain data type) in an object\n"
  "This class is provided to 'box' a value (encapsulate the value in an object). This class is required to interface "
  "to pointer or reference types in a method call. By using that class, the method can alter the value and thus implement "
  "'out parameter' semantics. The value may be 'nil' which acts as a null pointer in pointer type arguments."
  "\n"
  "This class has been introduced in version 0.22."
);

}

