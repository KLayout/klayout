
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


#ifndef HDR_tlIteratorUtils
#define HDR_tlIteratorUtils

namespace tl
{

/**
 *  @brief Checks an iterator against the default-constructed value
 *
 *  This function takes care of using the right way of comparing iterators
 *  suitable for MSVC with iterator debugging on.
 */
template <class Iter>
static inline bool is_null_iterator (const Iter &iter)
{
#if defined(_MSC_VER) && defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL >= 2
  return iter._Unwrapped () == Iter ()._Unwrapped ();
#else
  return iter == Iter ();
#endif
}

/**
 *  @brief Checks an iterator against another one without iterator checking
 *
 *  This function takes care of using the right way of comparing iterators
 *  suitable for MSVC with iterator debugging on.
 */
template <class Iter>
static inline bool is_equal_iterator_unchecked (const Iter &a, const Iter &b)
{
#if defined(_MSC_VER) && defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL >= 2
  return a._Unwrapped () == b._Unwrapped ();
#else
  return a == b;
#endif
}

}

#endif
