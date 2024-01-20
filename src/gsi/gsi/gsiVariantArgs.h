
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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


#ifndef HDR_gsiVariantArgs
#define HDR_gsiVariantArgs

#include "gsiCommon.h"
#include "gsiSerialisation.h"
#include "gsiTypes.h"

namespace tl
{
  class Heap;
  class Variant;
}

namespace gsi
{

class SerialArgs;
class ArgType;

/**
 *  @brief Pushes a variant on the serialization stack
 *
 *  This also involves expanding of arrays into objects by calling the constructor.
 *
 *  @param arglist The serialization stack to push the argument to
 *  @param atype The argument type
 *  @param arg The argument to push (may be modified if 'out' parameter)
 *  @param heap The heap
 */
GSI_PUBLIC void push_arg (gsi::SerialArgs &arglist, const gsi::ArgType &atype, tl::Variant &arg, tl::Heap *heap);

/**
 *  @brief Pulls a variant from the serialization stack
 *
 *  This function will pull the next argument from the bottom of the serialization stack
 *  and remove it from the stack.
 *
 *  @param retlist The serialization stack
 *  @param atype The argument type
 *  @param arg_out Receives the value
 *  @param heap The heap
 */
GSI_PUBLIC void pull_arg (gsi::SerialArgs &retlist, const gsi::ArgType &atype, tl::Variant &arg_out, tl::Heap *heap);

/**
 *  @brief Tests if the argument can be passed to a specific type
 *  @param atype The argument type
 *  @param arg The value to pass to it
 *  @param loose true for loose checking
 *
 *  @return True, if the argument can be passed
 */
GSI_PUBLIC bool test_arg (const gsi::ArgType &atype, const tl::Variant &arg, bool loose);

}

#endif

