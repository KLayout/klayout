
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

#ifndef _HDR_rbaMarshal
#define _HDR_rbaMarshal

#ifdef HAVE_RUBY

#include "gsiSerialisation.h"

#include <ruby.h>

#include "tlHeap.h"

namespace rba
{

class Proxy;

/**
 *  @brief Pops an argument from the call or return stack
 *
 *  "self" is a reference to the object that the method is called on or 0 if there is no such object.
 */
VALUE pop_arg (const gsi::ArgType &atype, Proxy *self, gsi::SerialArgs &aserial, tl::Heap &heap);

/**
 *  @brief Pushes an argument on the call or return stack
 */
void push_arg (const gsi::ArgType &atype, gsi::SerialArgs &aserial, VALUE arg, tl::Heap &heap);

/**
 *  @brief Tests if an argument can be converted to the given type
 *
 *  if atype is a vector:
 *      argument must be an array of the given type
 *  if atype is a ref:
 *      argument must be a boxed type of the required type or an object of the requested class
 *  if atype is a ptr:
 *      argument must be a boxed type of the required type or an object of the requested class or nil
 *  if atype is a cptr:
 *      argument must be of requested type or nil
 *  otherwise:
 *      argument must be of the requested type
 */
bool test_arg (const gsi::ArgType &atype, VALUE arg, bool loose);

}

#endif

#endif

