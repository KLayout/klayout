
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


#ifndef HDR_tlBitSetMask
#define HDR_tlBitSetMask

#include "tlCommon.h"
#include "tlBitSet.h"

#include <cstdint>
#include <algorithm>
#include <string>

namespace tl
{

/**
 *  @brief A bit set
 *
 *  This object can store a mask for a bit set.
 *  Each element of the mask corresponds to one bit. Each element can be "True"
 *  (matching to true), "False" (matching to false), "Any" matching to true
 *  or false and "Never" matches neither to true nor false.
 *
 *  A bit set match can be matched against a bit set and will return true if
 *  the bit set corresponds to the mask.
 *
 *  Allocation is dynamic when a mask element is accessed for write. Bits beyond the
 *  allocated size are treated as "Any".
 */
class TL_PUBLIC BitSetMask
{
public:
  typedef enum { Any = 0, False = 1, True = 2, Never = 3 } mask_type;

  typedef tl::BitSet::index_type index_type;
  typedef tl::BitSet::size_type size_type;
  typedef tl::BitSet::data_type data_type;

  /**
   *  @brief Default constructor: creates an empty bit set
   */
  BitSetMask ();

  /**
   *  @brief Creates a bit set mask from a string
   *
   *  In the string, a '0' character is for False, '1' for True, 'X' for Any and '-' for Never.
   */
  BitSetMask (const std::string &s);

  /**
   *  @brief Copy constructor
   */
  BitSetMask (const BitSetMask &other);

  /**
   *  @brief Move constructor
   */
  BitSetMask (BitSetMask &&other);

  /**
   *  @brief Destructor
   */
  ~BitSetMask ();

  /**
   *  @brief Converts the mask to a string
   */
  std::string to_string () const;

  /**
   *  @brief Assignment
   */
  BitSetMask &operator= (const BitSetMask &other);

  /**
   *  @brief Move assignment
   */
  BitSetMask &operator= (BitSetMask &&other);

  /**
   *  @brief Swaps the contents of this bit set with the other
   */
  void swap (BitSetMask &other)
  {
    std::swap (mp_data0, other.mp_data0);
    std::swap (mp_data1, other.mp_data1);
    std::swap (m_size, other.m_size);
  }

  /**
   *  @brief Clears this bit set
   */
  void clear ();

  /**
   *  @brief Sizes the bit set to "size" bits
   *
   *  New bits are set to false.
   */
  void resize (size_type size);

  /**
   *  @brief Equality
   */
  bool operator== (const BitSetMask &other) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const BitSetMask &other) const
  {
    return !operator== (other);
  }

  /**
   *  @brief Less operator
   *
   *  The bits are compared in lexical order, first bit first.
   */
  bool operator< (const BitSetMask &other) const;

  /**
   *  @brief Sets the mask for the given bit
   */
  void set (index_type index, mask_type mask);

  /**
   *  @brief Gets a mask from the given bit
   */
  mask_type operator[] (index_type index) const;

  /**
   *  @brief Gets a value indicating whether the set is empty
   *
   *  "empty" means, no bits have been written yet. "empty" does NOT mean
   *  all masks are of some specific value.
   */
  bool is_empty () const
  {
    return m_size == 0;
  }

  /**
   *  @brief Gets the number of bits for the mask stored
   *
   *  The number of bits is the highest bit written so far.
   */
  size_type size () const
  {
    return m_size;
  }

  /**
   *  @brief Matches the given bit set against this mask
   */
  bool match (const tl::BitSet &) const;

private:
  data_type *mp_data0, *mp_data1;
  size_type m_size;
};

}

namespace std
{

inline void
swap (tl::BitSetMask &a, tl::BitSetMask &b)
{
  a.swap (b);
}

}

#endif
