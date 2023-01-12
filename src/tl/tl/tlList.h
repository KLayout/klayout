
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


#ifndef HDR_tlList
#define HDR_tlList

#include "tlAssert.h"
#include "tlTypeTraits.h"

#include <iterator>
#include <algorithm>
#include <type_traits>

namespace tl
{

template <class C, bool copy_constructible> class list_impl;

/**
 *  @brief A base class for objects that can be kept in the linked list
 */
template <class C>
class list_node
{
public:
  list_node () : mp_next (0), mp_prev (0), m_owned (true) { }
  list_node (const list_node &) : mp_next (0), mp_prev (0), m_owned (true) { }
  list_node &operator= (const list_node &) { return *this; }

  ~list_node ()
  {
    unlink ();
  }

  C *next ()
  {
    tl_assert (mp_next);
    return static_cast<C *> (mp_next->mp_next == 0 ? 0 : mp_next);
  }

  const C *next () const
  {
    tl_assert (mp_next);
    return static_cast<const C *> (mp_next->mp_next == 0 ? 0 : mp_next);
  }

  C *prev ()
  {
    tl_assert (mp_prev);
    return static_cast<C *> (mp_prev->mp_prev == 0 ? 0 : mp_prev);
  }

  const C *prev () const
  {
    tl_assert (mp_prev);
    return static_cast<const C *> (mp_prev->mp_prev == 0 ? 0 : mp_prev);
  }

  C *self ()
  {
    return static_cast<C *> (this);
  }

  const C *self () const
  {
    return static_cast<const C *> (this);
  }

  void unlink ()
  {
    if (mp_prev) {
      tl_assert (mp_prev->mp_next == this);
      mp_prev->mp_next = mp_next;
    }
    if (mp_next) {
      tl_assert (mp_next->mp_prev == this);
      mp_next->mp_prev = mp_prev;
    }
    mp_prev = mp_next = 0;
  }

private:
  template <class Type, bool copy_constructible> friend class list_impl;
  template <class Type, bool CanCopy> friend class list;
  template <class Type> friend class list_iterator;
  template <class Type> friend class reverse_list_iterator;

  list_node *mp_next, *mp_prev;
  bool m_owned;
};

template <class C>
class list_impl<C, false>
{
public:
  list_impl () : m_head (), m_back ()
  {
    m_head.mp_next = &m_back;
    m_back.mp_prev = &m_head;
  }

  list_impl (const list_impl &) { tl_assert (false); }
  list_impl &operator= (const list_impl &) { tl_assert (false); return *this; }

  ~list_impl ()
  {
    clear ();
  }

  void clear ()
  {
    while (! empty ()) {
      erase (first ());
    }
  }

  void erase (C *c)
  {
    if (c->m_owned) {
      delete c;
    } else {
      c->unlink ();
    }
  }

  void swap (list_impl<C, false> &other)
  {
    std::swap (m_head.mp_next, other.m_head.mp_next);
    if (m_head.mp_next) {
      m_head.mp_next->mp_prev = &m_head;
    }
    if (other.m_head.mp_next) {
      other.m_head.mp_next->mp_prev = &other.m_head;
    }
    std::swap (m_back.mp_prev, other.m_back.mp_prev);
    if (m_back.mp_prev) {
      m_back.mp_prev->mp_next = &m_back;
    }
    if (other.m_back.mp_prev) {
      other.m_back.mp_prev->mp_next = &other.m_back;
    }
  }

  bool empty () const
  {
    return m_head.mp_next == &m_back;
  }

  C *first ()
  {
    return ! empty () ? static_cast<C *> (m_head.mp_next) : 0;
  }

  const C *first () const
  {
    return ! empty () ? static_cast<C *> (m_head.mp_next) : 0;
  }

  C *last ()
  {
    return ! empty () ? static_cast<C *> (m_back.mp_prev) : 0;
  }

  const C *last () const
  {
    return ! empty () ? static_cast<C *> (m_back.mp_prev) : 0;
  }

  void pop_back ()
  {
    delete last ();
  }

  void pop_front ()
  {
    delete first ();
  }

  void insert (C *after, C *new_obj)
  {
    insert_impl (after, new_obj, true);
  }

  void insert_before (C *before, C *new_obj)
  {
    insert_before_impl (before, new_obj, true);
  }

  void push_back (C *new_obj)
  {
    push_back_impl (new_obj, true);
  }

  void push_front (C *new_obj)
  {
    push_front_impl (new_obj, true);
  }

  void insert (C *after, C &new_obj)
  {
    insert_impl (after, &new_obj, false);
  }

  void insert_before (C *before, C &new_obj)
  {
    insert_before_impl (before, &new_obj, false);
  }

  void push_back (C &new_obj)
  {
    push_back_impl (&new_obj, false);
  }

  void push_front (C &new_obj)
  {
    push_front_impl (&new_obj, false);
  }

  size_t size () const
  {
    size_t n = 0;
    for (const C *p = first (); p; p = p->next ()) {
      ++n;
    }
    return n;
  }

protected:
  list_node<C> &head ()
  {
    return m_head;
  }

  const list_node<C> &head () const
  {
    return m_head;
  }

  list_node<C> &back ()
  {
    return m_back;
  }

  const list_node<C> &back () const
  {
    return m_back;
  }

private:
  list_node<C> m_head, m_back;

  void insert_impl (C *after, C *new_obj, bool owned)
  {
    list_node<C> *after_node = after;
    if (! after) {
      after_node = &m_head;
    }

    new_obj->m_owned = owned;
    new_obj->mp_next = after_node->mp_next;
    after_node->mp_next = new_obj;
    new_obj->mp_prev = after_node;
    new_obj->mp_next->mp_prev = new_obj;
  }

  void insert_before_impl (C *before, C *new_obj, bool owned)
  {
    list_node<C> *before_node = before;
    if (! before) {
      before_node = &m_back;
    }

    new_obj->m_owned = owned;
    new_obj->mp_prev = before_node->mp_prev;
    before_node->mp_prev = new_obj;
    new_obj->mp_next = before_node;
    new_obj->mp_prev->mp_next = new_obj;
  }

  void push_back_impl (C *new_obj, bool owned)
  {
    insert_before_impl (0, new_obj, owned);
  }

  void push_front_impl (C *new_obj, bool owned)
  {
    insert_impl (0, new_obj, owned);
  }
};

template <class C>
class list_impl<C, true>
  : public list_impl<C, false>
{
public:
  using list_impl<C, false>::insert;
  using list_impl<C, false>::push_back;
  using list_impl<C, false>::pop_back;
  using list_impl<C, false>::insert_before;
  using list_impl<C, false>::push_front;
  using list_impl<C, false>::pop_front;

  list_impl () { }

  list_impl (const list_impl &other)
    : list_impl<C, false> ()
  {
    operator= (other);
  }

  list_impl &operator= (const list_impl &other)
  {
    if (this != &other) {
      list_impl<C, false>::clear ();
      for (const C *p = other.first (); p; p = p->next ()) {
        push_back (*p);
      }
    }
    return *this;
  }

  void insert (C *after, const C &obj)
  {
    insert (after, new C (obj));
  }

  void insert_before (C *before, const C &obj)
  {
    insert_before (before, new C (obj));
  }

  void push_back (const C &obj)
  {
    insert_before (0, new C (obj));
  }

  void push_front (const C &obj)
  {
    insert (0, new C (obj));
  }
};

/**
 *  @brief An iterator for the linked list
 */
template <class C>
class list_iterator
{
public:
  typedef std::bidirectional_iterator_tag category;
  typedef C value_type;
  typedef C &reference;
  typedef C *pointer;

  list_iterator (C *p = 0) : mp_p (p) { }
  list_iterator operator++ () { mp_p = static_cast<C *> (mp_p->mp_next); return *this; }
  list_iterator operator-- () { mp_p = static_cast<C *> (mp_p->mp_prev); return *this; }

  C *operator-> () const
  {
    return mp_p;
  }

  C &operator* () const
  {
    return *mp_p;
  }

  bool operator== (list_iterator other) const { return mp_p == other.mp_p; }
  bool operator!= (list_iterator other) const { return mp_p != other.mp_p; }

private:
   C *mp_p;
};

/**
 *  @brief A reverse iterator for the linked list
 */
template <class C>
class reverse_list_iterator
{
public:
  typedef std::bidirectional_iterator_tag category;
  typedef C value_type;
  typedef C &reference;
  typedef C *pointer;

  reverse_list_iterator (C *p = 0) : mp_p (p) { }
  reverse_list_iterator operator++ () { mp_p = static_cast<C *> (mp_p->mp_prev); return *this; }
  reverse_list_iterator operator-- () { mp_p = static_cast<C *> (mp_p->mp_next); return *this; }

  C *operator-> () const
  {
    return mp_p;
  }

  C &operator* () const
  {
    return *mp_p;
  }

  bool operator== (reverse_list_iterator other) const { return mp_p == other.mp_p; }
  bool operator!= (reverse_list_iterator other) const { return mp_p != other.mp_p; }

private:
   C *mp_p;
};

/**
 *  @brief A linked list
 *
 *  In contrast to std::list this implementation is based on derivation from
 *  a common base class (list_node<C> where C is the type that needs to be
 *  put into the list.
 *
 *  The advantage of this approach is that the elements can unregister them
 *  selves upon delete, there are no iterators involved in insert and delete
 *  operations and each object knows it's followers and predecessors.
 *
 *  @code
 *  class MyClass : public tl::list_node<MyClass> { ... };
 *
 *  tl::list<MyClass> list;
 *  list.push_back (new MyClass ());
 */
template <class C, bool CanCopy = std::is_copy_constructible<C>::value>
class list
  : public list_impl<C, CanCopy>
{
public:
  typedef list_iterator<C> iterator;
  typedef list_iterator<const C> const_iterator;
  typedef reverse_list_iterator<C> reverse_iterator;
  typedef reverse_list_iterator<const C> const_reverse_iterator;

  typedef C value_type;

  using list_impl<C, CanCopy>::first;
  using list_impl<C, CanCopy>::last;
  using list_impl<C, CanCopy>::head;
  using list_impl<C, CanCopy>::back;

  list () { }
  list (const list &other) : list_impl<C, CanCopy> (other) { }

  list &operator= (const list &other)
  {
    list_impl<C, CanCopy>::operator= (other);
    return *this;
  }

  iterator begin ()
  {
    return iterator (static_cast <C *> (head ().mp_next));
  }

  iterator end ()
  {
    return iterator (static_cast <C *> (&back ()));
  }

  const_iterator begin () const
  {
    return const_iterator (static_cast <const C *> (head ().mp_next));
  }

  const_iterator end () const
  {
    return const_iterator (static_cast <const C *> (&back ()));
  }

  reverse_iterator rbegin ()
  {
    return reverse_iterator (static_cast <C *> (back ().mp_prev));
  }

  reverse_iterator rend ()
  {
    return reverse_iterator (static_cast <C *> (&head ()));
  }

  const_reverse_iterator rbegin () const
  {
    return const_reverse_iterator (static_cast <const C *> (back ().mp_prev));
  }

  const_reverse_iterator rend () const
  {
    return const_reverse_iterator (static_cast <const C *> (&head ()));
  }

  bool operator== (const list &other) const
  {
    const C *i1 = first ();
    const C *i2 = other.first ();
    while (i1 && i2) {
      if (! (*i1 == *i2)) {
        return false;
      }
      i1 = i1->next ();
      i2 = i2->next ();
    }
    return (i1 == 0 && i2 == 0);
  }

  bool operator!= (const list &other) const
  {
    return !operator== (other);
  }

  bool operator< (const list &other) const
  {
    const C *i1 = first ();
    const C *i2 = other.first ();
    while (i1 && i2) {
      if (! (*i1 == *i2)) {
        return *i1 < *i2;
      }
      i1 = i1->next ();
      i2 = i2->next ();
    }
    return ((i1 == 0) > (i2 == 0));
  }
};

}

#endif
