
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


#ifndef HDR_tlCopyOnWrite
#define HDR_tlCopyOnWrite

#include "tlCommon.h"
#include "tlThreads.h"
#include <algorithm>

namespace tl
{

/**
 *  @brief A copy duplicator (see copy_on_write_ptr below)
 */
template <class X>
struct copy_duplicator
{
  X *operator() (const X &o)
  {
    return new X (o);
  }
};

/**
 *  @brief A clone duplicator (see copy_on_write_ptr below)
 */
template <class X>
struct clone_duplicator
{
  X *operator() (const X &o)
  {
    return o.clone ();
  }
};

/**
 *  @brief A base class for the copy-on-write shared pointers
 */
class TL_PUBLIC CopyOnWritePtrBase
{
protected:
  static tl::Mutex ms_lock;
};

/**
 *  @brief The holder object: keeps the actual reference of the object
 */
template <class X>
class copy_on_write_holder
{
public:
  copy_on_write_holder (X *x)
    : m_ref_count (1), mp_x (x)
  { }

  ~copy_on_write_holder ()
  {
    delete mp_x;
    mp_x = 0;
  }

  X *x ()
  {
    return mp_x;
  }

  int ref_count () const { return m_ref_count; };
  int dec_ref () { return --m_ref_count; }
  void inc_ref () { ++m_ref_count; }

private:
  int m_ref_count;
  X *mp_x;
};

/**
 *  @brief Supplies a copy-on-write shared pointer scheme
 *
 *  The idea is to provide a smart, "unique" pointer providing a copy constructor and assignment, but
 *  sharing the object as long as the object is not written.
 *
 *  Write access is assumed as soon as the non-const pointer is retrieved.
 *
 *  In order to duplicate the object, a "duplicator" needs to be defined. By default, the
 *  object is duplicated using the copy constructor ("copy_duplicator"). An alternative implementation
 *  is provided through the "clone_duplicator", which assumes a "clone" method to supply the duplicated
 *  object.
 */
template <class X, class Dup = copy_duplicator<X> >
class copy_on_write_ptr
  : public CopyOnWritePtrBase
{
public:
  typedef X value_type;

  copy_on_write_ptr ()
    : mp_holder (0)
  { }

  copy_on_write_ptr (X *x)
    : mp_holder (x ? new copy_on_write_holder<X> (x) : 0)
  { }

  explicit copy_on_write_ptr (const copy_on_write_ptr<X, Dup> &other)
    : mp_holder (other.mp_holder)
  {
    acquire ();
  }

  copy_on_write_ptr &operator= (const copy_on_write_ptr<X, Dup> &other)
  {
    if (this != &other) {
      release ();
      mp_holder = other.mp_holder;
      acquire ();
    }
    return *this;
  }

  ~copy_on_write_ptr ()
  {
    release ();
  }

  /**
   *  @brief Swaps two pointers
   */
  void swap (copy_on_write_ptr<X, Dup> &other)
  {
    if (this == &other) {
      return;
    }

    tl::MutexLocker locker (&ms_lock);
    std::swap (mp_holder, other.mp_holder);
  }

  /**
   *  @brief Gets a writable object
   *  This is when we will create a new copy if the object is shared.
   */
  X *get_non_const ()
  {
    if (mp_holder) {
      tl::MutexLocker locker (&ms_lock);
      if (mp_holder->ref_count () > 1) {
        X *x = mp_holder->x ();
        mp_holder->dec_ref ();
        mp_holder = new copy_on_write_holder<X> (Dup () (*x));
      }
      return mp_holder->x ();
    } else {
      return 0;
    }
  }

  /**
   *  @brief Gets the const pointer
   *  No copy will be created.
   */
  const X *get_const () const
  {
    if (mp_holder) {
      return mp_holder->x ();
    } else {
      return 0;
    }
  }

  /**
   *  @brief Sets the pointer
   */
  void reset (X *x)
  {
    release ();
    if (x) {
      mp_holder = new copy_on_write_holder<X> (x);
    }
  }

  /**
   *  @brief Gets the non-const pointer
   *  This is when we will create a new copy if the object is shared.
   */
  X *operator-> ()
  {
    return get_non_const ();
  }

  /**
   *  @brief Gets the const pointer
   *  No copy will be created.
   */
  const X *operator-> () const
  {
    return get_const ();
  }

  /**
   *  @brief Gets the non-const reference
   *  This is when we will create a new copy if the object is shared.
   */
  X &operator* ()
  {
    return *get_non_const ();
  }

  /**
   *  @brief Gets the const reference
   *  No copy will be created.
   */
  const X &operator* () const
  {
    return *get_const ();
  }

  /**
   *  @brief Debugging and testing: gets the reference count
   */
  int ref_count () const
  {
    int rc = 0;
    if (mp_holder) {
      tl::MutexLocker locker (&ms_lock);
      rc = mp_holder->ref_count ();
    }
    return rc;
  }

private:
  copy_on_write_holder<X> *mp_holder;

  void release ()
  {
    if (mp_holder) {
      tl::MutexLocker locker (&ms_lock);
      if (mp_holder->dec_ref () <= 0) {
        delete mp_holder;
      }
      mp_holder = 0;
    }
  }

  void acquire ()
  {
    if (mp_holder) {
      tl::MutexLocker locker (&ms_lock);
      mp_holder->inc_ref ();
    }
  }
};

}

#endif
