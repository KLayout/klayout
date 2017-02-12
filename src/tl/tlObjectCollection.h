
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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

#include <QMutex>

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
template <class T, class Iter, bool Shared>
class weak_or_shared_collection_iterator
  : public Iter
{
public:
  typedef T value_type;
  typedef T &reference;
  typedef T *pointer;

  /**
   *  @brief Default constructor
   */
  weak_or_shared_collection_iterator ()
    : Iter ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Constructor from the internal iterator type
   */
  weak_or_shared_collection_iterator (Iter i)
    : Iter (i)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Dereferencing
   *  Dereferencing delivers a reference to the stored object (T &).
   */
  reference operator* () const
  {
    T *t = dynamic_cast<T *> (Iter::operator* ().operator-> ());
    tl_assert (t != 0);
    return *t;
  }

  /**
   *  @brief Access operator
   */
  pointer operator-> () const
  {
    return dynamic_cast<T *> (Iter::operator* ().operator-> ());
  }

  /**
   *  @brief Returns the iterator offset by the distance d.
   */
  weak_or_shared_collection_iterator<T, Iter, Shared> operator+ (typename Iter::difference_type d) const
  {
    return weak_or_shared_collection_iterator<T, Iter, Shared> (Iter::operator+ (d));
  }

  /**
   *  @brief Offsets the iterator by the distance d.
   */
  weak_or_shared_collection_iterator<T, Iter, Shared> operator+= (typename Iter::difference_type d) const
  {
    Iter::operator+= (d);
    return *this;
  }

  /**
   *  @brief Pre-decrement
   */
  weak_or_shared_collection_iterator<T, Iter, Shared> &operator-- ()
  {
    Iter::operator-- ();
    return *this;
  }

  /**
   *  @brief Post-decrement
   */
  weak_or_shared_collection_iterator<T, Iter, Shared> operator-- (int n)
  {
    weak_or_shared_collection_iterator<T, Iter, Shared> ret = *this;
    Iter::operator-- (n);
    return ret;
  }

  /**
   *  @brief Pre-increment
   */
  weak_or_shared_collection_iterator<T, Iter, Shared> &operator++ ()
  {
    Iter::operator++ ();
    return *this;
  }

  /**
   *  @brief Post-increment
   */
  weak_or_shared_collection_iterator<T, Iter, Shared> operator++ (int n)
  {
    weak_or_shared_collection_iterator<T, Iter, Shared> ret = *this;
    Iter::operator++ (n);
    return ret;
  }
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
private:
  class holder_type
    : public weak_or_shared_ptr<T, Shared>
  {
  public:
    holder_type (weak_or_shared_collection<T, Shared> *collection)
      : weak_or_shared_ptr<T, Shared> (), mp_collection (collection)
    {
      //  .. nothing yet ..
    }

    holder_type (weak_or_shared_collection<T, Shared> *collection, T *t)
      : weak_or_shared_ptr<T, Shared> (t), mp_collection (collection)
    {
      //  .. nothing yet ..
    }

    holder_type (weak_or_shared_collection<T, Shared> *collection, const weak_or_shared_ptr<T, Shared> &d)
      : weak_or_shared_ptr<T, Shared> (d), mp_collection (collection)
    {
      //  .. nothing yet ..
    }

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

  typedef std::vector<holder_type> basic_vector_type;

public:
  typedef weak_or_shared_collection_iterator<T, typename basic_vector_type::iterator, Shared> iterator;
  typedef weak_or_shared_collection_iterator<const T, typename basic_vector_type::const_iterator, Shared> const_iterator;
  typedef T value_type;
  typedef T &reference;
  typedef T *pointer;

  /**
   *  @brief The default constructor
   */
  weak_or_shared_collection ()
    : m_c ()
  {
  }

  /**
   *  @brief Returns a value indicating whether the collection is empty
   */
  bool empty () const
  {
    return m_c.empty ();
  }

  /**
   *  @brief Returns the size of the collection
   */
  typename basic_vector_type::size_type size () const
  {
    return m_c.size ();
  }

  /**
   *  @brief Clears the collection
   */
  void clear ()
  {
    if (! m_c.empty ()) {
      m_about_to_change ();
      m_c.clear ();
      m_changed ();
    }
  }

  /**
   *  @brief Erases the element with the given value
   *  This will remove the given object from the collection. For a shared collection this may delete
   *  the object unless another shared pointer or shared collection owns that object.
   */
  void erase (T *t)
  {
    for (iterator i = begin (); i != end (); ++i) {
      if (i.operator->() == t) {
        erase (i);
        break;
      }
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
    m_c.erase (i);
    m_changed ();
  }

  /**
   *  @brief Inserts the object before the given position
   *  For shared collections, this will make the object owned by the collection.
   */
  void insert (iterator before, T *object)
  {
    m_about_to_change ();
    m_c.insert (before, holder_type (this, object));
    m_changed ();
  }

  /**
   *  @brief Inserts the object from a weak or shared pointer before the given position
   *  For shared collections, this will make the object owned by the collection.
   */
  void insert (iterator before, const weak_or_shared_ptr<T, Shared> &object)
  {
    m_about_to_change ();
    m_c.insert (before, holder_type (this, object));
    m_changed ();
  }

  /**
   *  @brief Appends the object to the end of the collection
   *  For shared collections, this will make the object owned by the collection.
   */
  void push_back (T *object)
  {
    m_about_to_change ();
    m_c.insert (m_c.end (), holder_type (this, object));
    m_changed ();
  }

  /**
   *  @brief Appends the object from a weak or shared pointer to the end of the collection
   *  For shared collections, this will make the object owned by the collection.
   */
  void push_back (const weak_or_shared_ptr<T, Shared> &object)
  {
    m_about_to_change ();
    m_c.insert (m_c.end (), holder_type (this, object));
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
      m_c.pop_back ();
      m_changed ();
    }
  }

  /**
   *  @brief Gets a pointer to the first object in the collection
   */
  typename iterator::pointer front ()
  {
    return begin ().operator-> ();
  }

  /**
   *  @brief Gets a pointer to the last object in the collection
   */
  typename iterator::pointer back ()
  {
    return (--end ()).operator-> ();
  }

  /**
   *  @brief Gets a pointer to the first object in the collection (const version)
   */
  typename const_iterator::pointer front () const
  {
    return begin ().operator-> ();
  }

  /**
   *  @brief Gets a pointer to the last object in the collection (const version)
   */
  typename const_iterator::pointer back () const
  {
    return (--end ()).operator-> ();
  }

  /**
   *  @brief Gets a pointer to the nth object in the collection
   */
  typename iterator::pointer operator[] (typename iterator::difference_type i)
  {
    return (begin () + i).operator-> ();
  }

  /**
   *  @brief Gets a pointer to the nth object in the collection (const version)
   */
  typename const_iterator::pointer operator[] (typename iterator::difference_type i) const
  {
    return (begin () + i).operator-> ();
  }

  /**
   *  @brief Gets the begin iterator
   */
  iterator begin ()
  {
    return iterator (m_c.begin ());
  }
  
  /**
   *  @brief Gets the end iterator
   */
  iterator end ()
  {
    return iterator (m_c.end ());
  }
  
  /**
   *  @brief Gets the begin iterator (const version)
   */
  const_iterator begin () const
  {
    return const_iterator (m_c.begin ());
  }
  
  /**
   *  @brief Gets the end iterator (const version)
   */
  const_iterator end () const
  {
    return const_iterator (m_c.end ());
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
  basic_vector_type m_c;
  QMutex m_lock;

  void remove_element (holder_type *h)
  {
    QMutexLocker locker (&m_lock);
    tl_assert (! empty ());
    //  NOTE: this is a quick but somewhat dirty hack to obtain the index of an element.
    //  It is based on the assumption that a vector's elements are stored inside a
    //  contiguous memory array.
    size_t index = h - &(m_c.front ());
    tl_assert (index < m_c.size ());
    m_about_to_change ();
    m_c.erase (m_c.begin () + index);
    m_changed ();
  }

  tl::Event m_about_to_change, m_changed;
};

/**
 *  @brief The alias for the weak collection
 */
template <class T>
class weak_collection
  : public weak_or_shared_collection<T, false>
{
};

/**
 *  @brief The alias for the shared collection
 */
template <class T>
class shared_collection
  : public weak_or_shared_collection<T, true>
{
};

}

#endif

