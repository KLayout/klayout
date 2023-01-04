
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


#ifndef _HDR_pyaMarshal
#define _HDR_pyaMarshal

#include "Python.h"

#include "gsiDecl.h"
#include "gsiDeclBasic.h"
#include "pyaRefs.h"

namespace pya
{

class PYAObjectBase;

/**
 *  @brief Serializes the given argument using the given type.
 *
 *  @param atype Serialized type
 *  @param aserial The stream to which the type is serialized
 *  @param arg The argument to serialize (a Python object)
 *  @param heap A heap for temporary objects
 *
 *  The heap collects objects created while filling the buffer.
 *  The stack must persist as long as the serial buffer is used.
 */
void
push_arg (const gsi::ArgType &atype, gsi::SerialArgs &aserial, PyObject *arg, tl::Heap &heap);

/**
 *  @brief Reads a value from the serial stream (deserialize)
 *
 *  @param A type of the object to take from the stream
 *  @param aserial The stream from which to deserialize
 *  @param heap The heap for temporary objects
 *  @param self The self object of the method call (for shortcut evaluation to return self if possible)
 *  @return The deserialized object (a new reference)
 */
PythonRef pop_arg (const gsi::ArgType &atype, gsi::SerialArgs &aserial, PYAObjectBase *self, tl::Heap &heap);

/**
 *  @brief Tests whether the given object is compatible with the given type
 *
 *  @param atype The requested type
 *  @param arg The object to test
 *  @param loose If true, the test is performed in a loose fashion (used for second-pass matching)
 *  @return True, if the type match
 */
bool
test_arg (const gsi::ArgType &atype, PyObject *arg, bool loose);

/**
 *  @brief Correct constness if a reference is const and a non-const reference is required
 *  HINT: this is a workaround for the fact that unlike C++, Python does not have const or non-const
 *  references. Since a reference is identical with the object it points to, there are only const or non-const
 *  objects. We deliver const objects first, but if a non-const version is requested, the
 *  object turns into a non-const one. This may be confusing but provides a certain level
 *  of "constness", at least until there is another non-const reference for that object.
 */
void correct_constness (PyObject *obj, bool const_required);

}

#endif
