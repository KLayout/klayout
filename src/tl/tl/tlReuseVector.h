
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



#ifndef HDR_tlReuseVector
#define HDR_tlReuseVector

#include <stdlib.h>
#include <iterator>
#include <type_traits>
#include <vector>
#include <cstring>

#include "tlAssert.h"
#include "tlTypeTraits.h"

namespace tl
{

template <class Value, bool trivial_relocate> class reuse_vector;
template <class Value, bool trivial_relocate> class reuse_vector_const_iterator;

/**
 *  @brief The iterator for a reuse_vector
 */

template <class Value, bool trivial_relocate>
class reuse_vector_iterator
{
public:
  typedef size_t size_type;
  typedef Value value_type;
  typedef value_type *pointer; 
  typedef value_type &reference;   //  operator* returns a value
  typedef std::forward_iterator_tag iterator_category;
  typedef size_type difference_type;

  /**
   *  @brief The default constructor
   */
  reuse_vector_iterator ()
    : mp_v (0), m_n (0)
  { }

  /**
   *  @brief The constructor
   */
  reuse_vector_iterator (reuse_vector<Value, trivial_relocate> *v, size_type n)
    : mp_v (v), m_n (n)
  { }

  /**
   *  @brief Equality with const iterator
   */
  bool operator== (const reuse_vector_const_iterator<Value, trivial_relocate> &d) const
  {
    return mp_v == d.mp_v && m_n == d.m_n;
  }

  /**
   *  @brief Inequality with const iterator
   */
  bool operator!= (const reuse_vector_const_iterator<Value, trivial_relocate> &d) const
  {
    return ! operator== (d);
  }

  /**
   *  @brief Equality
   */
  bool operator== (const reuse_vector_iterator &d) const
  {
    return mp_v == d.mp_v && m_n == d.m_n;
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const reuse_vector_iterator &d) const
  {
    return ! operator== (d);
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const reuse_vector_iterator &d) const
  {
    if (mp_v != d.mp_v) {
      return mp_v < d.mp_v;
    }
    return m_n < d.m_n;
  }

  /**
   *  @brief Dereference operator
   */
  value_type &operator* () const
  {
    tl_assert (mp_v->is_used (m_n));
    return mp_v->item (m_n);
  }

  /**
   *  @brief Access operator
   */
  value_type *operator-> () const
  {
    tl_assert (mp_v->is_used (m_n));
    return &mp_v->item (m_n);
  }

  /**
   *  @brief Validity 
   *
   *  An iterator is valid if the object is still available
   *  This does not take care about the container, but just about elements inside it.
   *  If the container is deleted, the iterators become invalid without further notice.
   */
  bool is_valid () const
  {
    return mp_v->is_used (m_n);
  }

  /**
   *  @brief Increment operator
   */
  reuse_vector_iterator &operator++ ()
  {
    do {
      ++m_n;
    } while (! at_end () && ! mp_v->is_used (m_n));
    return *this;
  }

  /**
   *  @brief "at end" predicate
   */
  bool at_end () const
  {
    return index () >= mp_v->last ();
  }

  /** 
   *  @brief The index of the element pointed to
   */
  size_t index () const
  {
    return m_n;
  }

  /** 
   *  @brief The pointer to the vector that this iterator points into
   */
  reuse_vector<Value, trivial_relocate> *vector () const
  {
    return mp_v;
  }

  /**
   *  @brief A distance between two vectors
   */
  difference_type operator- (reuse_vector_iterator d) const
  {
    //  KLUDGE: this is slow and can be optimized if the vector does not have deleted items
    difference_type n = 0;
    while (d != *this) {
      ++d;
      ++n;
    }
    return n;
  }

private:
  template <class, bool> friend class reuse_vector_const_iterator;

  reuse_vector<Value, trivial_relocate> *mp_v;
  size_type m_n;
};


/**
 *  @brief The const_iterator for a reuse_vector
 */

template <class Value, bool trivial_relocate>
class reuse_vector_const_iterator
{
public:
  typedef size_t size_type;
  typedef Value value_type;
  typedef const value_type *pointer; 
  typedef const value_type &reference;   //  operator* returns a value
  typedef std::forward_iterator_tag iterator_category;
  typedef size_type difference_type;

  /**
   *  @brief The default constructor
   */
  reuse_vector_const_iterator ()
    : mp_v (0), m_n (0)
  { }

  /**
   *  @brief The constructor
   */
  reuse_vector_const_iterator (const reuse_vector<Value, trivial_relocate> *v, size_type n)
    : mp_v (v), m_n (n)
  { }

  /**
   *  @brief The conversion of a non-const iterator to a const iterator
   */
  reuse_vector_const_iterator (const reuse_vector_iterator<Value, trivial_relocate> &d)
    : mp_v (d.mp_v), m_n (d.m_n)
  { }

  /**
   *  @brief cast to non-const iterator
   */
  reuse_vector_iterator<Value, trivial_relocate> to_non_const () const
  {
    return reuse_vector_iterator<Value, trivial_relocate> (const_cast<reuse_vector<Value, trivial_relocate> *> (mp_v), m_n);
  }

  /**
   *  @brief Equality
   */
  bool operator== (const reuse_vector_const_iterator &d) const
  {
    return mp_v == d.mp_v && m_n == d.m_n;
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const reuse_vector_const_iterator &d) const
  {
    return ! operator== (d);
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const reuse_vector_const_iterator &d) const
  {
    if (mp_v != d.mp_v) {
      return mp_v < d.mp_v;
    }
    return m_n < d.m_n;
  }

  /**
   *  @brief Dereference operator
   */
  const value_type &operator* () const
  {
    tl_assert (mp_v->is_used (m_n));
    return mp_v->item (m_n);
  }

  /**
   *  @brief Access operator
   */
  const value_type *operator-> () const
  {
    tl_assert (mp_v->is_used (m_n));
    return &mp_v->item (m_n);
  }

  /**
   *  @brief Unsafe access to the target address
   *
   *  This method is intended for special use cases such as the cached box picker in 
   *  the box tree
   */
  const value_type *unsafe_target_addr () const
  {
    return &mp_v->item (m_n);
  }

  /**
   *  @brief Validity 
   *
   *  An iterator is valid if the object is still available
   *  This does not take care about the container, but just about elements inside it.
   *  If the container is deleted, the iterators become invalid without further notice.
   */
  bool is_valid () const
  {
    return mp_v->is_used (m_n);
  }

  /**
   *  @brief Increment operator
   */
  reuse_vector_const_iterator &operator++ ()
  {
    do {
      ++m_n;
    } while (! at_end () && ! mp_v->is_used (m_n));
    return *this;
  }

  /**
   *  @brief "at end" predicate
   */
  bool at_end () const
  {
    return index () >= mp_v->last ();
  }

  /** 
   *  @brief The index of the element pointed to
   */
  size_t index () const
  {
    return m_n;
  }

  /** 
   *  @brief The pointer to the vector that this iterator points into
   */
  const reuse_vector<Value, trivial_relocate> *vector () const
  {
    return mp_v;
  }

  /**
   *  @brief A distance between two vectors
   */
  difference_type operator- (reuse_vector_const_iterator d) const
  {
    //  KLUDGE: this is slow and can be optimized if the vector does not have deleted items
    difference_type n = 0;
    while (d != *this) {
      ++d;
      ++n;
    }
    return n;
  }

private:
  template <class, bool> friend class reuse_vector_iterator;

  const reuse_vector<Value, trivial_relocate> *mp_v;
  size_type m_n;
};


/**
 *  @brief A helper class describing the "unused" entries of a reuse_vector
 */

class ReuseData
{
public:
  typedef size_t size_type;

  ReuseData ()
    : m_first_used (0), m_last_used (0), m_next_free (0), m_size (0)
  { }

  ReuseData (size_type n)
    : m_first_used (0), m_last_used (n), m_next_free (n), m_size (n)
  { 
    m_used.resize (n, true);
  }

  size_type first () const
  {
    return m_first_used;
  }

  size_type last () const
  {
    return m_last_used;
  }

  size_type size () const
  {
    return m_size;
  }

  size_type allocate () 
  {
    tl_assert (can_allocate ());

    size_type r = m_next_free;
    m_used [r] = true;

    if (r >= m_last_used) {
      m_last_used = r + 1;
    }
    if (r < m_first_used) {
      m_first_used = r;
    }

    while (m_next_free != m_used.size () && m_used [m_next_free]) {
      ++m_next_free;
    }

    ++m_size;

    return r;
  }

  bool can_allocate () const
  {
    return (m_next_free < m_used.size ());
  }

  void deallocate (size_type n) 
  {
    m_used [n] = false;

    if (n == m_first_used) {
      while (m_first_used < m_last_used && ! m_used [m_first_used]) {
        ++m_first_used;
      }
    }

    if (n == m_last_used - 1) {
      while (m_last_used > m_first_used && ! m_used [m_last_used - 1]) {
        --m_last_used;
      }
    }

    if (n < m_next_free) {
      m_next_free = n;
    }

    --m_size;
  }

  void reserve (size_type n) 
  {
    m_used.reserve (n);
  }

  bool is_used (size_type n) const
  {
    return m_used [n];
  }

  size_t mem_reqd () const
  {
    return (m_used.size () + 7) / 8 + sizeof (*this);
  }

  size_t mem_used () const
  {
    return m_used.capacity () / 8 + sizeof (*this);
  }

private:
  std::vector<bool> m_used;
  size_type m_first_used, m_last_used, m_next_free, m_size;
};
   
/**
 *  @brief A vector that maintains the order of elements but allows one to reference elements in a stable way
 *
 *  This container allows inserting and deleting of elements while references to them (through iterators)
 *  remain stable. The insert does not necessarily happen at a certain position. Instead, the vector
 *  keeps a reuseable member list (hence reuse_vector). In addition, the iterators deliver stable references
 *  by using indices and a container pointer. This way, the iterators point to the same element
 *  even after delete and insert (and potentially reallocation) actions.
 *
 *  The memory requirements of this container are the same than that of a std::vector.
 *  
 *  One requirement is that sizeof(C) >= sizeof(void *).
 */

#if __GNUC__ >= 5
template <class Value, bool trivial_relocate = std::is_trivially_copy_constructible<Value>::value>
#else
//  no support for extended type traits in gcc 4.x
template <class Value, bool trivial_relocate = false>
#endif
class reuse_vector
{
public:
  typedef Value value_type;
  typedef size_t size_type;
  typedef reuse_vector_iterator<value_type, trivial_relocate> iterator;
  typedef reuse_vector_const_iterator<value_type, trivial_relocate> const_iterator;

  /**
   *  @brief Default constructor
   */
  reuse_vector ()
  {
    init ();
  }

  /**
   *  @brief Assignment constructor
   *
   *  Assign the sequence of [from,to) to the vector.
   */
  template <class Iter>
  reuse_vector (Iter from, Iter to)
  {
    init ();
    reserve (std::distance (from, to));
    insert (from, to);
  }

  /**
   *  @brief Copy constructor
   *
   *  See operator= for a description of the copy operation.
   */
  reuse_vector (const reuse_vector &d)
  {
    init ();
    reserve (d.size ());
    for (const_iterator i = d.begin (); i != d.end (); ++i) {
      insert (*i);
    }
  }

  /**
   *  @brief Move constructor
   *
   *  See operator= for a description of the copy operation.
   */
  reuse_vector (reuse_vector &&d)
  {
    mp_start = d.mp_start; d.mp_start = 0;
    mp_finish = d.mp_finish; d.mp_finish = 0;
    mp_capacity = d.mp_capacity; d.mp_capacity = 0;
    mp_rdata = d.mp_rdata; d.mp_rdata = 0;
  }

  /**
   *  @brief Destructor
   */
  ~reuse_vector ()
  {
    release ();
  }

  /**
   *  @brief Assignment
   *
   *  The assignment will not only copy the items but also compact the vector, i.e.
   *  create a linear chain of elements without holes for unused ones and a capacity
   *  exactly matching the required count.
   */
  reuse_vector &operator= (const reuse_vector &d)
  {
    if (&d != this) {
      release ();
      reserve (d.size ());
      for (const_iterator i = d.begin (); i != d.end (); ++i) {
        insert (*i);
      }
    }
    return *this;
  }

  /**
   *  @brief Assignment (move)
   */
  reuse_vector &operator= (reuse_vector &&d)
  {
    if (&d != this) {
      mp_start = d.mp_start; d.mp_start = 0;
      mp_finish = d.mp_finish; d.mp_finish = 0;
      mp_capacity = d.mp_capacity; d.mp_capacity = 0;
      mp_rdata = d.mp_rdata; d.mp_rdata = 0;
    }
    return *this;
  }

  /**
   *  @brief Assignment
   *
   *  Assign the sequence of [from,to) to the vector.
   *  See operator= for a description of the assignment semantics.
   */
  template <class Iter>
  void assign (Iter from, Iter to)
  {
    release ();
    reserve (std::distance (from, to));
    insert (from, to);
  }

  /**
   *  @brief equality operator
   */
  bool operator== (const reuse_vector &d) const
  {
    if (size () != d.size ()) {
      return false;
    }

    const_iterator i = begin ();
    const_iterator ii = d.begin ();
    while (i != end ()) {
      if (*i != *ii) {
        return false;
      }
      ++i;
      ++ii;
    }

    return true;
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const reuse_vector &d) const
  {
    return ! operator== (d);
  }

  /**
   *  @brief less operator
   */
  bool operator< (const reuse_vector &d) const
  {
    if (size () != d.size ()) {
      return size () < d.size ();
    }

    const_iterator i = begin ();
    const_iterator ii = d.begin ();
    while (i != end ()) {
      if (*i != *ii) {
        return *i < *ii;
      }
      ++i;
      ++ii;
    }

    return false;
  }

  /**
   *  @brief For a given pointer, tell if the element is a member of this container
   */
  template <class V>
  bool is_member_of (const V *ptr) const
  {
    return (ptr >= mp_start && ptr < mp_finish);
  }

  /**
   *  @brief Deliver the iterator for a certain element (given by pointer)
   */
  iterator iterator_from_pointer (Value *ptr) 
  {
    return iterator (this, ptr - mp_start);
  }

  /**
   *  @brief "begin" iterator
   */
  iterator begin ()
  {
    return iterator (this, first ());
  }

  /**
   *  @brief "end" iterator
   */
  iterator end ()
  {
    return iterator (this, last ());
  }
  
  /**
   *  @brief Deliver the iterator for a certain element (given by pointer)
   */
  const_iterator iterator_from_pointer (const Value *ptr) const
  {
    return const_iterator (this, ptr - mp_start);
  }

  /**
   *  @brief "begin" const iterator
   */
  const_iterator begin () const
  {
    return const_iterator (this, first ());
  }

  /**
   *  @brief "end" const iterator
   */
  const_iterator end () const
  {
    return const_iterator (this, last ());
  }

  /** 
   *  @brief Access by the basic index
   *
   *  The basic index is the internal index reported from the iterator's index() method
   */
  value_type &item (size_type n) 
  {
    return mp_start [n];
  }
  
  /** 
   *  @brief Access by the basic index
   *
   *  The basic index is the internal index reported from the iterator's index() method
   */
  const value_type &item (size_type n) const
  {
    return mp_start [n];
  }
  
  /**
   *  @brief Insert one element into the container
   */
  iterator insert (const value_type &item)
  {
    size_type n = 0;

    if (mp_rdata) {
      n = mp_rdata->allocate ();
      //  when the last unused member is allocated, we can remove the 
      //  ReuseData pointer and add to the end
      if (! mp_rdata->can_allocate ()) {
        delete mp_rdata;
        mp_rdata = 0;
      }
    } else {
      if (mp_finish == mp_capacity) {
        //  Special case: we are inserting an element from our own space - since reserve will first 
        //  release the element we have to create a copy before we insert it.
        if (&item >= mp_start && &item < mp_finish) {
          value_type copy (item);
          return insert (copy);
        }
        reserve (size () == 0 ? 4 : size () * 2);
      }
      n = size_t (mp_finish - mp_start);
      ++mp_finish;
    }

    new (mp_start + n) value_type (item);

    return iterator (this, n);
  }

  /**
   *  @brief Insert a sequence of elements [from,to) into the container
   */
  template <class Iter>
  void insert (const Iter &from, const Iter &to)
  {
    if (! (from == to)) {
      //  don't reserve if the first element of the sequence comes from ourself.
      //  This implies that we try to duplicate our own elements which would invalidate
      //  the source upon reservation.
      if (! (&*from >= mp_start && &*from < mp_finish)) {
        reserve (size () + std::distance (from, to));
      }
      for (Iter i = from; i != to; ++i) {
        insert (*i);
      }
    }
  }

  /**
   *  @brief Erase the given element from the container
   *
   *  If the element was erased already, nothing will happen.
   */
  void erase (const iterator &i)
  {
    if (! mp_rdata) {
      mp_rdata = new ReuseData (size ());
    }

    if (mp_rdata->is_used (i.index ())) {
      item (i.index ()).~value_type ();
      mp_rdata->deallocate (i.index ());
    }
  }

  /**
   *  @brief Erase the given sequence from the container
   */
  void erase (const iterator &from, const iterator &to)
  {
    //  trivial shortcut
    if (from == to) {
      return;
    }

    if (! mp_rdata) {
      mp_rdata = new ReuseData (size ());
    }

    for (size_type i = from.index (); i != to.index (); ++i) {
      if (mp_rdata->is_used (i)) {
        item (i).~value_type ();
        mp_rdata->deallocate (i);
      }
    }
  }

  /**
   *  @brief Clear the container
   *
   *  This does not release the memory allocated for this container,
   *  similar to that what std::vector does.
   */
  void clear ()
  {
    if (mp_start) {
      //  call destructor
      for (size_type i = first (); i < last (); ++i) {
        if (is_used (i)) {
          item (i).~value_type ();
        }
      }
    }
    if (mp_rdata) {
      delete mp_rdata;
      mp_rdata = 0;
    }

    mp_finish = mp_start;
  }

  /**
   *  @brief Return the size
   *
   *  The size is the number of elements that are actually stored.
   *  The size is computed in linear time.
   */
  size_type size () const
  {
    if (mp_rdata) {
      return mp_rdata->size ();
    } else {
      return size_type (mp_finish - mp_start);
    }
  }

  /**
   *  @brief Empty predicate
   */
  bool empty () const
  {
    return size () == 0;
  }

  /**
   *  @brief Return the capacity
   *
   *  The capacity is the number of elements that can be stored without reallocation to happen.
   */
  size_type capacity () const
  {
    return size_type (mp_capacity - mp_start);
  }

  /**
   *  @brief Reserve space for a given number of elements
   *
   *  This guarantees that for the next n-size() inserts no reallocation will occur
   *  No resizing will happen, if n is less than the current capacity.
   */
  void reserve (size_type n)
  {
    if (trivial_relocate) {
      internal_reserve_trivial (n);
    } else {
      internal_reserve_complex (n);
    }
  }

  /**
   *  @brief Release the memory allocated and clear the container
   */
  void release ()
  {
    if (mp_start) {
      //  call destructor
      for (size_type i = first (); i < last (); ++i) {
        if (is_used (i)) {
          item (i).~value_type ();
        }
      }
      delete [] ((char *) mp_start);
    }
    if (mp_rdata) {
      delete mp_rdata;
      mp_rdata = 0;
    }
    init ();
  }

  /**
   *  @brief Returns a value indicating whether the given index is valid
   */
  bool is_used (size_type n) const
  {
    if (n >= first () && n < last ()) {
      if (mp_rdata) {
        return mp_rdata->is_used (n);
      } else {
        return true;
      }
    }
    return false;
  }

  /**
   *  @brief For diagnostics purposes only
   */
  ReuseData *reuse_data () const
  {
    return mp_rdata;
  }

  /**
   *  @brief Swaps the vector with another one
   */
  void swap (reuse_vector &other)
  {
    std::swap (mp_start, other.mp_start);
    std::swap (mp_finish, other.mp_finish);
    std::swap (mp_capacity, other.mp_capacity);
    std::swap (mp_rdata, other.mp_rdata);
  }

private:
  value_type *mp_start, *mp_finish, *mp_capacity;
  ReuseData *mp_rdata;

  template<class, bool> friend class reuse_vector_iterator;
  template<class, bool> friend class reuse_vector_const_iterator;

  void init ()
  {
    mp_start = mp_finish = mp_capacity = 0;
    mp_rdata = 0;
  }

  size_type first () const
  {
    if (mp_rdata) {
      return mp_rdata->first ();
    } else {
      return 0;
    }
  }

  size_type last () const
  {
    if (mp_rdata) {
      return mp_rdata->last ();
    } else {
      return size_type (mp_finish - mp_start);
    }
  }

  void internal_reserve_complex (size_type n)
  {
    if (n > capacity ()) {

      value_type *new_start = (value_type *) (new char [sizeof (value_type) * n]);

      size_type l = last ();
      for (size_type i = first (); i < l; ++i) {
        if (is_used (i)) {
          new (new_start + i) value_type (item (i));
          item (i).~value_type ();
        }
      }

      size_type e = size_type (mp_finish - mp_start);

      if (mp_rdata) {
        mp_rdata->reserve (n);
      }

      if (mp_start) {
        delete [] ((char *) mp_start);
      }

      mp_start = new_start;
      mp_finish = mp_start + e;
      mp_capacity = mp_start + n;

    }
  }

  void internal_reserve_trivial (size_type n)
  {
    if (n > capacity ()) {

      value_type *new_start = (value_type *) (new char [sizeof (value_type) * n]);

      size_type e = 0;

      if (mp_start) {

        e = size_type (mp_finish - mp_start);

        size_type l = last ();
        size_type i = first ();
        memcpy ((void *)(new_start + i), (void *)(mp_start + i), (l - i) * sizeof (Value));

        delete [] ((char *) mp_start);

      }

      if (mp_rdata) {
        mp_rdata->reserve (n);
      }

      mp_start = new_start;
      mp_finish = mp_start + e;
      mp_capacity = mp_start + n;

    }
  }
};

}

#endif

