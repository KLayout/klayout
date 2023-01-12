
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
  explicit vector (const tl::vector<T> &d) : base (d) { }

  /**
   *  @brief Move constructor
   */
  explicit vector (const tl::vector<T> &&d) : base (d) { }

  /**
   *  @brief Assignment
   */
  vector &operator= (const tl::vector<T> &d)
  {
    if (&d != this) {
      base::operator= (d);
    }
    return *this;
  }

  /**
   *  @brief Assignment (Move)
   */
  vector &operator= (const tl::vector<T> &&d)
  {
    if (&d != this) {
      base::operator= (d);
    }
    return *this;
  }

  /**
   *  @brief Initialization with value and length
   */
  vector (const T &v, int s) : base (v, s) { }
};

} // namespace tl

#endif

