
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


#ifndef HDR_tlUniqueId
#define HDR_tlUniqueId

#include "tlCommon.h"
#include <cstdint>

namespace tl
{

typedef uint64_t id_type;

/**
 *  @brief An object delivering a unique ID per object
 *
 *  This class is supposed to provide a solution for the non-uniqueness
 *  issue of addresses. Instead of using addresses as keys, use the unique
 *  ID. The advantage is to be reproducible and guaranteed to be different
 *  for each allocation of an object (apart from overflow wrapping at 64bit -
 *  this this is pretty unlikely event).
 *
 *  To supply an ID, derive your class from tl::UniqueId. Use
 *  "tl::id_of (object *)" the get the ID from that object.
 *  A null-pointer will give an ID of 0 which is reserved for
 *  "nothing".
 *
 *  The type of the ID is tl::id_type.
 */
class TL_PUBLIC UniqueId
{
public:
  /**
   *  @brief @brief Creates a new object with a new ID
   */
  UniqueId ();

  UniqueId (const UniqueId &) { }
  UniqueId &operator= (const UniqueId &) { return *this; }

private:
  friend id_type id_of (const UniqueId *);

  id_type m_id;
};

/**
 *  @brief Gets the ID of the object pointed to by o
 *
 *  Returns 0 in a null pointer and only then.
 */
inline id_type id_of (const UniqueId *o)
{
  return o ? o->m_id : 0;
}

} // namespace tl

#endif

