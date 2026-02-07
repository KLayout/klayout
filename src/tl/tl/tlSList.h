
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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



#ifndef HDR_tlSList
#define HDR_tlSList

#include "tlCommon.h"

#include <algorithm>
#include <iterator>

namespace tl
{

/**
 *  @brief A simple single-linked list implementation
 *
 *  This implementation supports:
 *  - fast size
 *  - push_back, push_front
 *  - forward iterator, const_iterator
 *  - splice
 *  - pop_front
 *  - clear
 *  - empty
 */

template <class T> class slist;

template <class T>
struct slist_node_type
{
  slist_node_type (const T &_t) : next (0), t (_t) { }
  slist_node_type (T &&_t) : next (0), t (_t) { }
  slist_node_type *next;
  T t;
};

template <class T>
class slist_const_iterator;

template <class T>
class slist_iterator
{
public:
  typedef slist_node_type<T> node_type;
  typedef std::forward_iterator_tag iterator_category;
  typedef T value_type;
  typedef T &reference;
  typedef T *pointer;
  typedef void difference_type;

  slist_iterator (node_type *p = 0) : mp_p (p) { }
  slist_iterator operator++ () { mp_p = mp_p->next; return *this; }

  T *operator-> () const
  {
    return &mp_p->t;
  }

  T &operator* () const
  {
    return mp_p->t;
  }

  bool operator== (slist_iterator<T> other) const { return mp_p == other.mp_p; }
  bool operator!= (slist_iterator<T> other) const { return mp_p != other.mp_p; }

private:
  friend class slist_const_iterator<T>;
  friend class slist<T>;
  node_type *mp_p;
};

template <class T>
class slist_const_iterator
{
public:
  typedef slist_node_type<T> node_type;
  typedef std::forward_iterator_tag iterator_category;
  typedef const T value_type;
  typedef const T &reference;
  typedef const T *pointer;
  typedef void difference_type;

  slist_const_iterator (slist_iterator<T> i) : mp_p (i.mp_p) { }
  slist_const_iterator (const node_type *p = 0) : mp_p (p) { }
  slist_const_iterator operator++ () { mp_p = mp_p->next; return *this; }

  const T *operator-> () const
  {
    return &mp_p->t;
  }

  const T &operator* () const
  {
    return mp_p->t;
  }

  bool operator== (slist_const_iterator<T> other) const { return mp_p == other.mp_p; }
  bool operator!= (slist_const_iterator<T> other) const { return mp_p != other.mp_p; }

private:
  friend class slist<T>;
  const node_type *mp_p;
};

template <class T>
class slist
{
public:
  typedef slist_node_type<T> node_type;
  typedef T value_type;
  typedef slist_const_iterator<T> const_iterator;
  typedef slist_iterator<T> iterator;

  slist ()
    : mp_first (0), mp_last (0), m_size (0)
  {
    //  .. nothing yet ..
  }

  template <class Iter>
  slist (Iter from, Iter to)
    : mp_first (0), mp_last (0), m_size (0)
  {
    for (Iter i = from; i != to; ++i) {
      push_back (*i);
    }
  }

  slist (const slist<T> &other)
    : mp_first (0), mp_last (0), m_size (0)
  {
    for (auto i = other.begin (); i != other.end (); ++i) {
      push_back (*i);
    }
  }

  slist (slist<T> &&other)
    : mp_first (0), mp_last (0), m_size (0)
  {
    std::swap (mp_first, other.mp_first);
    std::swap (mp_last, other.mp_last);
    std::swap (m_size, other.m_size);
  }

  slist<T> &operator= (const slist<T> &other)
  {
    if (this != &other) {
      clear ();
      for (const_iterator i = other.begin (); i != other.end (); ++i) {
        push_back (*i);
      }
    }
    return *this;
  }

  slist<T> &operator= (slist<T> &&other)
  {
    clear ();
    std::swap (mp_first, other.mp_first);
    std::swap (mp_last, other.mp_last);
    std::swap (m_size, other.m_size);
    return *this;
  }

  ~slist ()
  {
    clear ();
  }

  iterator begin ()
  {
    return iterator (mp_first);
  }

  iterator end ()
  {
    return iterator (0);
  }

  const_iterator begin () const
  {
    return const_iterator (mp_first);
  }

  const_iterator end () const
  {
    return const_iterator (0);
  }

  size_t size () const
  {
    return m_size;
  }

  bool empty () const
  {
    return mp_first == 0;
  }

  void clear ()
  {
    while (! empty ()) {
      pop_front ();
    }
  }

  void swap (slist<T> &other)
  {
    std::swap (mp_first, other.mp_first);
    std::swap (mp_last, other.mp_last);
    std::swap (m_size, other.m_size);
  }

  void pop_front ()
  {
    if (mp_first) {
      node_type *n = mp_first;
      if (n == mp_last) {
        mp_first = mp_last = 0;
      } else {
        mp_first = mp_first->next;
      }
      delete n;
      --m_size;
    }
  }

  T &front ()
  {
    return mp_first->t;
  }

  const T &front () const
  {
    return mp_first->t;
  }

  T &back ()
  {
    return mp_last->t;
  }

  const T &back () const
  {
    return mp_last->t;
  }

  void push_front (const T &t)
  {
    push_front_impl (new node_type (t));
  }

  void push_front (T &&t)
  {
    push_front_impl (new node_type (t));
  }

  void push_back (const T &t)
  {
    push_back_impl (new node_type (t));
  }

  void push_back (T &&t)
  {
    push_back_impl (new node_type (t));
  }

  void splice (slist<T> &other)
  {
    if (! other.mp_first) {
      return;
    }

    if (! mp_first) {
      mp_first = other.mp_first;
    } else {
      mp_last->next = other.mp_first;
    }

    mp_last = other.mp_last;
    m_size += other.m_size;

    other.mp_first = other.mp_last = 0;
    other.m_size = 0;
  }

  void erase_after (iterator at)
  {
    node_type *n = at.mp_p->next;
    if (n) {
      at.mp_p->next = n->next;
      delete n;
    }
  }

private:
  node_type *mp_first, *mp_last;
  size_t m_size;

  void push_front_impl (node_type *n)
  {
    if (mp_first) {
      n->next = mp_first;
      mp_first = n;
    } else {
      mp_first = mp_last = n;
    }
    ++m_size;
  }

  void push_back_impl (node_type *n)
  {
    if (mp_last) {
      mp_last->next = n;
      mp_last = n;
    } else {
      mp_first = mp_last = n;
    }
    ++m_size;
  }
};

}

#endif
