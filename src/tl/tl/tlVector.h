
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


#ifndef HDR_tlVector
#define HDR_tlVector

#include <vector>
#include "tlTypeTraits.h"

namespace tl 
{

/**
 *  @brief a specialized vector class that is (once) supposed to support garbage collection mechanisms
 *
 *  The general idea to use vectors as containers as far as possible. 
 *  This special incarnation of a vector is supposed to: 
 *  1.) provide fast allocation schemes using thread-local storage
 *      and per-object size freelists.
 *  2.) by use of a special allocator provide a garbage collection 
 *      mechanism that may move and compact the blocks allocated by
 *      the vectors.
 *  3.) Avoid memory fragmentation by using blocks with a maximum size
 */

template <class T>
class vector 
  : public std::vector<T>
{
public:
  typedef std::vector<T> base;

  /**
   *  @brief Default constructor: creates an empty vector
   */
  vector () : std::vector<T> () { }

  /**
   *  @brief Copy constructor
   */
  explicit vector (const tl::vector<T> &d) : std::vector<T> (d) { }

  /**
   *  @brief Initialization with value and length
   */
  vector (const T &v, int s) : std::vector<T> (v, s) { }
};

/**
 *  @brief The type traits for the vector type
 */
template <class C>
struct type_traits <tl::vector<C> > : public type_traits<void>
{
#if defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL != 0
  //  With iterator debugging on, the vector carries additional
  //  information which cannot be copied trivially
  typedef complex_relocate_required relocate_requirements;
#else
  typedef trivial_relocate_required relocate_requirements;
#endif
  typedef true_tag has_efficient_swap;
  typedef false_tag supports_extractor;
  typedef false_tag supports_to_string;
  typedef true_tag has_less_operator;
  typedef true_tag has_equal_operator;
};

} // namespace tl

#endif

