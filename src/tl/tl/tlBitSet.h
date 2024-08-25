
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


#ifndef HDR_tlBitSet
#define HDR_tlBitSet

#include "tlCommon.h"

#include <cstdint>
#include <algorithm>

namespace tl
{

/**
 *  @brief A bit set
 *
 *  This object can store a set of n bits, each being true or false.
 *  Essentially is it like a vector<bool>, but optimized to cooperate
 *  with tl::BitSetMap and tl::BitSetMatch.
 *
 *  Allocation is dynamic when a bit is accessed for write. Bits beyond the
 *  allocated size are treated as "false" or zero.
 */
class TL_PUBLIC BitSet
{
public:
  typedef unsigned int index_type;
  typedef unsigned int size_type;
  typedef uint32_t data_type;

  /**
   *  @brief Default constructor: creates an empty bit set
   */
  BitSet ()
    : mp_data (0), m_size (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Creates and initializes a bit set from a range of indexes
   *  Every bit given by an index from the range is set.
   */
  template <class Iter>
  BitSet (Iter from, Iter to)
    : mp_data (0), m_size (0)
  {
    set (from, to);
  }

  /**
   *  @brief Copy constructor
   */
  BitSet (const BitSet &other)
    : mp_data (0), m_size (0)
  {
    operator= (other);
  }

  /**
   *  @brief Move constructor
   */
  BitSet (BitSet &&other)
    : mp_data (0), m_size (0)
  {
    operator= (std::move (other));
  }

  /**
   *  @brief Destructor
   */
  ~BitSet ()
  {
    clear ();
  }

  /**
   *  @brief Assignment
   */
  BitSet &operator= (const BitSet &other);

  /**
   *  @brief Move assignment
   */
  BitSet &operator= (BitSet &&other);

  /**
   *  @brief Swaps the contents of this bit set with the other
   */
  void swap (BitSet &other)
  {
    std::swap (mp_data, other.mp_data);
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
  bool operator== (const BitSet &other) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const BitSet &other) const
  {
    return !operator== (other);
  }

  /**
   *  @brief Less operator
   *
   *  The bits are compared in lexical order, first bit first.
   */
  bool operator< (const BitSet &other) const;

  /**
   *  @brief Sets the given bit
   */
  void set (index_type index);

  /**
   *  @brief Sets a range of bits
   *  The indexes are taken from the sequence delivered by the iterator.
   */
  template <class Iter>
  void set (Iter from, Iter to)
  {
    for (Iter i = from; i != to; ++i) {
      set (*i);
    }
  }

  /**
   *  @brief Resets the given bit
   */
  void reset (index_type index);

  /**
   *  @brief Resets a range of bits
   *  The indexes are taken from the sequence delivered by the iterator.
   */
  template <class Iter>
  void reset (Iter from, Iter to)
  {
    for (Iter i = from; i != to; ++i) {
      reset (*i);
    }
  }

  /**
   *  @brief Sets the values for a given bit
   */
  void set_value (index_type index, bool f)
  {
    if (f) {
      set (index);
    } else {
      reset (index);
    }
  }

  /**
   *  @brief Sets the values for a range of bits
   *  The indexes are taken from the sequence delivered by the iterator.
   */
  template <class Iter>
  void set_value (Iter from, Iter to, bool f)
  {
    for (Iter i = from; i != to; ++i) {
      set_value (*i, f);
    }
  }

  /**
   *  @brief Gets a bit from the given index
   */
  bool operator[] (index_type index) const;

  /**
   *  @brief Gets a value indicating whether the set is empty
   *
   *  "empty" means, no bits have been written yet. "empty" does NOT mean
   *  all bits are zero.
   */
  bool is_empty () const
  {
    return m_size == 0;
  }

  /**
   *  @brief Gets the number of bits stored
   *
   *  The number of bits is the highest bit written so far.
   */
  size_type size () const
  {
    return m_size;
  }

private:
  friend class BitSetMask;

  data_type *mp_data;
  size_type m_size;
};

}

namespace std
{

inline void
swap (tl::BitSet &a, tl::BitSet &b)
{
  a.swap (b);
}

}

#endif
