
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


#ifndef HDR_tlFixedVector
#define HDR_tlFixedVector

#include "tlCommon.h"

#include "tlAssert.h"

namespace tl
{

/**
 *  @brief A fixed-capacity vector class
 *
 *  This vector class allocates the given number of items.
 *  It allows one to push and pop, but not to exceed the given length.
 */
template <class T, size_t N>
class fixed_vector 
{
public:
  typedef T value_type;
  typedef T *iterator;
  typedef const T *const_iterator;

  /**
   *  @brief Default ctor - creates an empty fixed-capacity vector
   */
  fixed_vector ()
    : m_size (0)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Single-element ctor - assigns one element to the vector
   */
  fixed_vector (const T &l)
    : m_size (1)
  {
    m_values [0] = l;
  }

  /**
   *  @brief Assignment of a several items 
   */
  fixed_vector (const T &l, size_t n)
    : m_size (n)
  {
    tl_assert (m_size < N);
    for (size_t i = 0; i < n; ++i) {
      m_values [i] = l;
    }
  }

  /**
   *  @brief Clears the vector
   */
  void clear () 
  {
    m_size = 0;
  }

  /**
   *  @brief The last element
   */
  T &back ()
  {
    tl_assert (m_size > 0);
    return m_values [m_size - 1];
  }

  /**
   *  @brief The last element (const)
   */
  const T &back () const
  {
    tl_assert (m_size > 0);
    return m_values [m_size - 1];
  }

  /**
   *  @brief Random access operator
   */
  T &operator[] (size_t i) 
  {
    return m_values [i];
  }

  /**
   *  @brief Random access operator (const)
   */
  const T &operator[] (size_t i) const
  {
    return m_values [i];
  }

  /**
   *  @brief Insert at end
   */
  void push_back (const T &l)
  {
    tl_assert (m_size < N);
    m_values [m_size++] = l;
  }

  /**
   *  @brief Pop element from back
   */
  void pop_back ()
  {
    --m_size;
  }

  /**
   *  @brief Size of the vector
   */
  size_t size () const
  {
    return m_size;
  }

  /**
   *  @brief begin iterator 
   */
  T *begin ()
  {
    return m_values;
  }

  /**
   *  @brief end iterator 
   */
  T *end ()
  {
    return m_values + m_size;
  }

  /**
   *  @brief begin iterator (const)
   */
  const T *begin () const
  {
    return m_values;
  }

  /**
   *  @brief end iterator (const)
   */
  const T *end () const
  {
    return m_values + m_size;
  }

  /**
   *  @brief empty predicate
   */
  bool empty () const
  {
    return m_size == 0;
  }

  /**
   *  @brief equality
   */
  bool operator== (const fixed_vector &other) const
  {
    if (m_size != other.m_size) {
      return false;
    }

    for (size_t i = 0; i < m_size; ++i) {
      if (m_values [i] != other.m_values [i]) {
        return false;
      }
    }

    return true;
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const fixed_vector &other) const
  {
    return ! operator== (other);
  }

  /**
   *  @brief less operator
   */
  bool operator< (const fixed_vector &other) const
  {
    if (m_size != other.m_size) {
      return m_size < other.m_size;
    }

    for (size_t i = 0; i < m_size; ++i) {
      if (m_values [i] != other.m_values [i]) {
        return m_values [i] < other.m_values [i];
      }
    }

    return false;
  }

private:
  size_t m_size;
  T m_values [N];
};

}

#endif

