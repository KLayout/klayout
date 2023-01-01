
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


#ifndef HDR_gsiDeclDbHelpers
#define HDR_gsiDeclDbHelpers

#include "dbLayoutUtils.h"

namespace gsi
{
  /**
   *  @brief A safe iterator locking the layout while iterating a container within it
   */
  template <class I>
  class layout_locking_iterator2
    : private db::LayoutLocker
  {
  public:
    typedef typename I::value_type value_type;
    typedef typename I::reference reference;
    typedef typename I::pointer pointer;
    typedef typename I::difference_type difference_type;
    typedef typename I::iterator_category iterator_category;

    layout_locking_iterator2 (const db::Layout *layout, const I &b, const I &e) : db::LayoutLocker (const_cast<db::Layout *> (layout)), m_b (b), m_e (e) {}
    bool at_end () const { return m_b == m_e; }
    void operator++ () { ++m_b; }

    reference operator* () const { return *m_b; }
    pointer operator-> () const { return m_b.operator-> (); }

  private:
    I m_b, m_e;
  };

  /**
   *  @brief A safe iterator locking the layout while iterating a container within it
   */
  template <class I>
  class layout_locking_iterator1
    : private db::LayoutLocker
  {
  public:
    typedef typename I::value_type value_type;
    typedef typename I::reference reference;
    typedef typename I::pointer pointer;
    typedef typename I::difference_type difference_type;
    typedef typename I::iterator_category iterator_category;

    layout_locking_iterator1 (const db::Layout *layout, const I &i) : db::LayoutLocker (const_cast<db::Layout *> (layout)), m_i (i) { }
    bool at_end () const { return m_i.at_end (); }
    void operator++ () { ++m_i; }

    reference operator* () const { return *m_i; }
    pointer operator-> () const { return m_i.operator-> (); }

  private:
    I m_i;
  };

}

#endif
