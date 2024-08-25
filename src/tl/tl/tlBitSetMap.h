
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


#ifndef HDR_tlBitSetMap
#define HDR_tlBitSetMap

#include "tlCommon.h"
#include "tlBitSetMask.h"
#include "tlBitSet.h"
#include "tlAssert.h"

#include <algorithm>

namespace tl
{

template <class Value>
struct TL_PUBLIC bit_set_mask_node
{
  bit_set_mask_node ()
    : mask (), next (0), value ()
  {
  }

  tl::BitSetMask mask;
  size_t next;
  Value value;

  void swap (bit_set_mask_node<Value> &other)
  {
    std::swap (mask, other.mask);
    std::swap (next, other.next);
    std::swap (value, other.value);
  }
};

template <class Value>
class TL_PUBLIC bit_set_mask_compare
{
public:
  bit_set_mask_compare (tl::BitSetMask::index_type bit, tl::BitSetMask::mask_type mask)
    : m_bit (bit), m_mask (mask)
  {
  }

  bool operator() (const bit_set_mask_node<Value> &node) const
  {
    return node.mask [m_bit] < m_mask;
  }

private:
  tl::BitSetMask::index_type m_bit;
  tl::BitSetMask::mask_type m_mask;
};

}

namespace std
{

template <class Value>
inline void
swap (tl::bit_set_mask_node<Value> &a, tl::bit_set_mask_node<Value> &b)
{
  a.swap (b);
}

}

namespace tl
{

/**
 *  @brief A bit set map
 *
 *  This specialized map stores tl::BitSetMask keys and corresponding values.
 *  tl::BitSet objects can be used to retrieve values. Masks may overlap, hence
 *  multiple matches are possible. The "lookup" method employs a visitor
 *  pattern to deliver these multiple matches.
 *
 *  In order to use the map, it first has to be sorted. Insert masks using
 *  "insert" and do a "sort" before using "lookup".
 */
template <class Value>
class TL_PUBLIC bit_set_map
{
public:
  typedef std::vector<bit_set_mask_node<Value> > node_list;
  typedef typename node_list::const_iterator const_iterator;
  typedef typename node_list::iterator iterator;

  /**
   *  @brief Default constructor: creates an empty bit set
   */
  bit_set_map ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Copy constructor
   */
  bit_set_map (const bit_set_map &other)
  {
    operator= (other);
  }

  /**
   *  @brief Move constructor
   */
  bit_set_map (bit_set_map &&other)
  {
    operator= (std::move (other));
  }

  /**
   *  @brief Assignment
   */
  bit_set_map &operator= (const bit_set_map &other)
  {
    if (this != &other) {
      m_nodes = other.m_nodes;
      m_sorted = other.m_sorted;
    }
    return *this;
  }

  /**
   *  @brief Move assignment
   */
  bit_set_map &operator= (bit_set_map &&other)
  {
    if (this != &other) {
      swap (other);
    }
    return *this;
  }

  /**
   *  @brief Swaps the contents of this bit set with the other
   */
  void swap (bit_set_map &other)
  {
    if (this != &other) {
      m_nodes.swap (other.m_nodes);
      std::swap (m_sorted, other.m_sorted);
    }
  }

  /**
   *  @brief Clears this map
   */
  void clear ()
  {
    m_nodes.clear ();
    m_sorted = true;
  }

  /**
   *  @brief Reserves "size" entries
   *
   *  Use this method to specify the intended size of the map.
   *  This optimizes performance and memory allocation.
   */
  void reserve (size_t n)
  {
    m_nodes.reserve (n);
  }

  /**
   *  @brief Inserts an item into the map
   */
  void insert (const tl::BitSetMask &mask, const Value &value)
  {
    m_nodes.push_back (tl::bit_set_mask_node<Value> ());
    m_nodes.back ().mask = mask;
    m_nodes.back ().next = 0;
    m_nodes.back ().value = value;

    m_sorted = false;
  }

  /**
   *  @brief Sorts the map
   *
   *  "sort" needs to be called before "lookup" can be used.
   */
  void sort ()
  {
    if (! m_sorted) {
      sort_range (0, m_nodes.begin (), m_nodes.end ());
      m_sorted = true;
    }
  }

  /**
   *  @brief Gets a value indicating whether the set is empty
   *
   *  "empty" means, no bits have been written yet. "empty" does NOT mean
   *  all masks are of some specific value.
   */
  bool is_empty () const
  {
    return m_nodes.empty ();
  }

  /**
   *  @brief Gets the number of bits for the mask stored
   *
   *  The number of bits is the highest bit written so far.
   */
  size_t size () const
  {
    return m_nodes.size ();
  }

  /**
   *  @brief Looks up items by bit set
   *
   *  For each item found, the value is delivered through the
   *  Inserter provided.
   *
   *  The return value is true, if any value has been found.
   */
  template <class Inserter>
  bool lookup (const tl::BitSet &bit_set, Inserter inserter)
  {
    tl_assert (m_sorted);
    return partial_lookup (0, m_nodes.begin (), m_nodes.end (), bit_set, inserter);
  }

private:
  node_list m_nodes;
  bool m_sorted;

  void sort_range (tl::BitSetMask::index_type bit, iterator from, iterator to)
  {
    if (from == to) {
      return;
    }

    //  special case of identical entries which creates a sequence of
    //  single entries
    bool all_same = true;
    for (auto i = from + 1; i != to && all_same; ++i) {
      if (*i != *from) {
        all_same = false;
      }
    }
    if (all_same) {
      //  this is also the case for a single element
      for (auto i = from + 1; i != to; ++i) {
        i->next = 1;
      }
      return;
    }

    //  we have at least one element. The first one is taken for the previous level node, so we start partitioning
    //  at the second node
    ++from;

    auto middle_false = std::partition (from, to, tl::bit_set_mask_compare (bit, tl::BitSetMask::False));
    auto middle_true = std::partition (middle_false, to, tl::bit_set_mask_compare (bit, tl::BitSetMask::True));
    auto middle_never = std::partition (middle_true, to, tl::bit_set_mask_compare (bit, tl::BitSetMask::Never));

    from->next = middle_false - from;
    if (middle_false != to) {
      middle_false->next = middle_true - middle_false;
    }
    if (middle_true != to) {
      middle_true->next = middle_never - middle_true;
    }
    if (middle_never != to) {
      middle_never->next = to - middle_never;
    }

    sort_range (bit + 1, from, middle_false);
    sort_range (bit + 1, middle_false, middle_true);
    sort_range (bit + 1, middle_true, middle_never);
    sort_range (bit + 1, middle_never, to);
  }

  template <class Inserter>
  bool partial_lookup (tl::BitSetMask::index_type bit, const_iterator from, const_iterator to, const tl::BitSet &bit_set, Inserter inserter)
  {
    if (from == to) {
      return false;
    }

    bool any = false;
    if (from->mask.match (bit_set)) {
      *inserter++ = from->value;
      any = true;
    }

    bool b = bit_set [bit];

    auto i = ++from;
    while (i != to) {
      auto m = i->mask [bit];
      if (m == tl::BitSetMask::Any || (m == tl::BitSetMask::True && b) || (m == tl::BitSetMask::False && !b)) {
        if (partial_lookup (bit + 1, i, i + i->next, bit_set, inserter)) {
          any = true;
        }
      }
      if (i->next) {
        break;
      }
      i += i->next;
    }

    return any;
  }
};

}

#endif
