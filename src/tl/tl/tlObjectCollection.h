
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


#ifndef HDR_tlObjectCollection
#define HDR_tlObjectCollection

#include "tlObject.h"
#include "tlEvents.h"
#include "tlThreads.h"

#include <iterator>
#include <vector>

namespace tl
{

/**
 *  @brief The iterator class for the weak or shared collection
 *  This class is used to implement tl::shared_collection::iterator etc.
 *  It is based on a opaque internal iterator (Iter) which provides the standard
 *  compare and assignment methods.
 */
template <class T, class Holder, bool Shared>
class weak_or_shared_collection_iterator
{
public:
  typedef T value_type;
  typedef T &reference;
  typedef T *pointer;
  typedef std::bidirectional_iterator_tag iterator_category;
  typedef std::ptrdiff_t difference_type;

  /**
   *  @brief Default constructor
   */
  weak_or_shared_collection_iterator ()
    : mp_holder (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Constructor from the internal iterator type
   */
  weak_or_shared_collection_iterator (Holder *holder)
    : mp_holder (holder)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Equality of iterators
   */
  bool operator== (const weak_or_shared_collection_iterator &other) const
  {
    return mp_holder == other.mp_holder;
  }

  /**
   *  @brief Inequality of iterators
   */
  bool operator!= (const weak_or_shared_collection_iterator &other) const
  {
    return mp_holder != other.mp_holder;
  }

  /**
   *  @brief Dereferencing
   *  Dereferencing delivers a reference to the stored object (T &).
   */
  reference operator* () const
  {
    tl_assert (mp_holder != 0);
    return mp_holder->operator* ();
  }

  /**
   *  @brief Access operator
   */
  pointer operator-> () const
  {
    tl_assert (mp_holder != 0);
    return mp_holder->operator-> ();
  }

  /**
   *  @brief Pre-decrement
   */
  weak_or_shared_collection_iterator<T, Holder, Shared> &operator-- ()
  {
    tl_assert (mp_holder);
    mp_holder = mp_holder->prev;
    return *this;
  }

  /**
   *  @brief Post-decrement
   */
  weak_or_shared_collection_iterator<T, Holder, Shared> operator-- (int n)
  {
    weak_or_shared_collection_iterator<T, Holder, Shared> ret = *this;
    while (n-- > 0) {
      operator-- ();
    }
    return ret;
  }

  /**
   *  @brief Pre-increment
   */
  weak_or_shared_collection_iterator<T, Holder, Shared> &operator++ ()
  {
    tl_assert (mp_holder);
    mp_holder = mp_holder->next;
    return *this;
  }

  /**
   *  @brief Post-increment
   */
  weak_or_shared_collection_iterator<T, Holder, Shared> operator++ (int n)
  {
    weak_or_shared_collection_iterator<T, Holder, Shared> ret = *this;
    while (n-- > 0) {
      operator++ ();
    }
    return ret;
  }

  /**
   *  @brief Internal: access to the holder object
   */
  Holder *holder () const
  {
    return mp_holder;
  }

public:
  Holder *mp_holder;
};

/**
 *  @brief The weak or shared collection
 *
 *  The actual implementation is provided through tl::shared_collection<T> and tl::weak_collection<T> which
 *  are basically aliases for this class.
 *
 *  Collections basically behave like std::vector<T *> except for the iterator which dereferences the
 *  pointer (i.e. iterator::operator* delivers T & and iterator::operator-> delivers T *).
 */
template <class T, bool Shared>
class weak_or_shared_collection
{
public:
  class holder_type
    : public weak_or_shared_ptr<T, Shared>
  {
  public:
    holder_type (weak_or_shared_collection<T, Shared> *collection)
      : weak_or_shared_ptr<T, Shared> (), next (0), prev (0), mp_collection (collection)
    {
      //  .. nothing yet ..
    }

    holder_type (weak_or_shared_collection<T, Shared> *collection, T *t)
      : weak_or_shared_ptr<T, Shared> (t), next (0), prev (0), mp_collection (collection)
    {
      //  .. nothing yet ..
    }

    holder_type (weak_or_shared_collection<T, Shared> *collection, const weak_or_shared_ptr<T, Shared> &d)
      : weak_or_shared_ptr<T, Shared> (d), next (0), prev (0), mp_collection (collection)
    {
      //  .. nothing yet ..
    }

    holder_type *next, *prev;

  protected:
    virtual void reset_object ()
    {
      weak_or_shared_ptr<T, Shared>::reset_object ();
      if (mp_collection) {
        //  Caution: this will probably delete "this"!
        mp_collection->remove_element (this);
      }
    }

  private:
    weak_or_shared_collection<T, Shared> *mp_collection;
  };

  typedef weak_or_shared_collection_iterator<T, holder_type, Shared> iterator;
  typedef weak_or_shared_collection_iterator<const T, holder_type, Shared> const_iterator;
  typedef T value_type;
  typedef T &reference;
  typedef T *pointer;

  /**
   *  @brief The default constructor
   */
  weak_or_shared_collection ()
    : mp_first (0), mp_last (0), m_size (0)
  {
  }

  /**
   *  @brief Destructor
   */
  ~weak_or_shared_collection ()
  {
    while (! empty ()) {
      erase (mp_first);
    }
  }

  /**
   *  @brief Returns a value indicating whether the collection is empty
   */
  bool empty () const
  {
    return mp_first == 0;
  }

  /**
   *  @brief Returns the size of the collection
   */
  size_t size () const
  {
    return m_size;
  }

  /**
   *  @brief Clears the collection
   */
  void clear ()
  {
    m_about_to_change ();
    while (! empty ()) {
      erase (mp_first);
    }
    tl_assert (m_size == 0);
    m_changed ();
  }

  /**
   *  @brief Erases the element with the given value
   *  This will remove the given object from the collection. For a shared collection this may delete
   *  the object unless another shared pointer or shared collection owns that object.
   */
  void erase (T *t)
  {
    holder_type *h = mp_first;
    while (h && h->operator-> () != t) {
      h = h->next;
    }

    if (h) {
      m_about_to_change ();
      erase (h);
      m_changed ();
    }
  }

  /**
   *  @brief Erases the element given by the iterator i
   *  This will remove the object from the collection. For a shared collection this may delete
   *  the object unless another shared pointer or shared collection owns that object.
   */
  void erase (iterator i)
  {
    m_about_to_change ();
    erase (i.holder ());
    m_changed ();
  }

  /**
   *  @brief Inserts the object before the given position
   *  For shared collections, this will make the object owned by the collection.
   */
  void insert (iterator before, T *object)
  {
    m_about_to_change ();
    insert (before.holder (), new holder_type (this, object));
    m_changed ();
  }

  /**
   *  @brief Inserts the object from a weak or shared pointer before the given position
   *  For shared collections, this will make the object owned by the collection.
   */
  void insert (iterator before, const weak_or_shared_ptr<T, Shared> &object)
  {
    m_about_to_change ();
    insert (before.holder (), new holder_type (this, object));
    m_changed ();
  }

  /**
   *  @brief Appends the object to the end of the collection
   *  For shared collections, this will make the object owned by the collection.
   */
  void push_back (T *object)
  {
    m_about_to_change ();
    insert (0, new holder_type (this, object));
    m_changed ();
  }

  /**
   *  @brief Appends the object from a weak or shared pointer to the end of the collection
   *  For shared collections, this will make the object owned by the collection.
   */
  void push_back (const weak_or_shared_ptr<T, Shared> &object)
  {
    m_about_to_change ();
    insert (0, new holder_type (this, object));
    m_changed ();
  }

  /**
   *  @brief Removes the object from the end of the collection
   *  For a shared collection, this will release the object unless another shared pointer
   *  refers to it.
   */
  void pop_back ()
  {
    if (! empty ()) {
      m_about_to_change ();
      erase (mp_last);
      m_changed ();
    }
  }

  /**
   *  @brief Gets a pointer to the first object in the collection
   */
  typename iterator::pointer front ()
  {
    return mp_first->operator-> ();
  }

  /**
   *  @brief Gets a pointer to the last object in the collection
   */
  typename iterator::pointer back ()
  {
    return mp_last->operator-> ();
  }

  /**
   *  @brief Gets a pointer to the first object in the collection (const version)
   */
  typename const_iterator::pointer front () const
  {
    return mp_first->operator-> ();
  }

  /**
   *  @brief Gets a pointer to the last object in the collection (const version)
   */
  typename const_iterator::pointer back () const
  {
    return mp_last->operator-> ();
  }

  /**
   *  @brief Gets the begin iterator
   */
  iterator begin ()
  {
    return iterator (mp_first);
  }
  
  /**
   *  @brief Gets the end iterator
   */
  iterator end ()
  {
    return iterator (0);
  }
  
  /**
   *  @brief Gets the begin iterator (const version)
   */
  const_iterator begin () const
  {
    return const_iterator (mp_first);
  }
  
  /**
   *  @brief Gets the end iterator (const version)
   */
  const_iterator end () const
  {
    return const_iterator (0);
  }

  /**
   *  @brief Exposes a signal that is issued before a change is made
   */
  tl::Event &about_to_change ()
  {
    return m_about_to_change;
  }

  /**
   *  @brief Exposes a signal that is issued after a change is made
   */
  tl::Event &changed ()
  {
    return m_changed;
  }

private:
  friend class holder_type;
  tl::Mutex m_lock;

  void remove_element (holder_type *h)
  {
    tl::MutexLocker locker (&m_lock);
    tl_assert (! empty ());
    m_about_to_change ();
    erase (h);
    m_changed ();
  }

  void erase (holder_type *h)
  {
    if (h == mp_first) {
      mp_first = h->next;
    }
    if (h == mp_last) {
      mp_last = h->prev;
    }
    if (h->next) {
      h->next->prev = h->prev;
    }
    if (h->prev) {
      h->prev->next = h->next;
    }

    delete h;

    --m_size;
  }

  void insert (holder_type *before, holder_type *h)
  {
    if (! before) {

      h->prev = mp_last;
      h->next = 0;
      if (mp_last) {
        mp_last->next = h;
      }

      mp_last = h;
      if (! mp_first) {
        mp_first = h;
      }

    } else {

      h->prev = before->prev;
      h->next = before;
      before->prev = h;

      if (before == mp_first) {
        mp_first = h;
      }

    }

    ++m_size;
  }

  tl::Event m_about_to_change, m_changed;
  holder_type *mp_first, *mp_last;
  size_t m_size;
};

/**
 *  @brief The alias for the weak collection
 */
template <class T>
class weak_collection
  : public weak_or_shared_collection<T, false>
{
public:
  weak_collection ()
    : weak_or_shared_collection<T, false> ()
  { }
};

/**
 *  @brief The alias for the shared collection
 */
template <class T>
class shared_collection
  : public weak_or_shared_collection<T, true>
{
public:
  shared_collection ()
    : weak_or_shared_collection<T, true> ()
  { }
};

}

#endif

