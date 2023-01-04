
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


#ifndef HDR_tlSelect
#define HDR_tlSelect

#include "tlCommon.h"

namespace tl
{

template<class Q, class X>
struct try_assign
{
  bool operator() (Q &, const X &) { return false; }
};

template<class Q>
struct try_assign<Q, Q>
{
  bool operator() (Q &q, const Q &x) { q = x; return true; }
};

/**
 *  @brief Copies either value a or b into q, whichever type matches
 *
 *  Returns true, if type A or B match with Q and q has been assigned.
 */
template <class Q, class A, class B>
bool select (Q &q, const A &a, const B &b)
{
  return try_assign<Q, A> () (q, a) || try_assign<Q, B> () (q, b);
}

}

#endif

