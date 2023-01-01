
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


#ifndef HDR_tlObject
#define HDR_tlObject

#include "tlCommon.h"

#include "tlAssert.h"
#include "tlThreads.h"

#include <iterator>
#include <vector>

namespace tl
{

class WeakOrSharedPtr;

/**
 *  @brief A basic object class
 *
 *  tl::Object enables a class to participate in weak and shared pointer
 *  formation. tl::Object is a neutral and empty class. It only provides
 *  the infrastructure for the shared pointer management.
 *
 *  The shared and weak pointer infrastructure contains of
 *
 *  1.) A shared pointer class (tl::shared_ptr<T>). All shared pointers
 *      share ownership of the contained object. When the last shared
 *      pointer owning one object is deleted, the object is deleted as well.
 *      If the object is deleted before the shared pointers by other
 *      means (i.e. direct delete on the object), the shared pointers
 *      are reset to null.
 *
 *  2.) A weak pointer class (tl::weak_ptr<T>). Weak pointers track an
 *      object's lifetime but they don't share ownership. When the object
 *      is deleted, weak pointers pointing to the object are reset to null.
 *      But when weak pointers are deleted, the object they point to will
 *      still persist.
 *
 *  3.) A shared collection (tl::shared_collection<T>). A shared collection
 *      is basically an array of shared pointers. It's behavior is the same
 *      than a std::vector<T *>, except that the ownership over the objects
 *      contained within the shared collection is shared among the array and
 *      potentially other shared pointers. The contained objects will be
 *      deleted when the array is deleted unless other shared pointers
 *      own an object inside contained in the collection.
 *      If an object in the collection is deleted by other means (i.e.
 *      direct delete), the corresponding entry is *removed* from the array
 *      instead of going to null ("following" behavior).
 *
 *  4.) A weak collection (tl::weak_collection<T>). A weak collection
 *      basically is an array of weak pointers. The collection will not
 *      own the objects contained in the collection and upon deletion,
 *      pointers inside the weak collection are removed from the collection.
 */
class TL_PUBLIC Object
{
public:
  /**
   *  @brief The constructor
   */
  Object ();

  /**
   *  @brief The destructor
   */
  virtual ~Object ();

  /**
   *  @brief Copy constructor
   *  The copy constructor's implementation will not copy the ownership
   *  and references.
   */
  Object (const Object &other);

  /**
   *  @brief Assignment
   */
  Object &operator= (const Object &other);

  /**
   *  @brief Marks the object as "to be kept"
   *  This method will mark the object as "not scheduled for deletion". Even if
   *  no strong pointer is having a reference to this object and longer, the
   *  object is not deleted.
   */
  void keep_object ();

  /**
   *  @brief Releases this object from being kept
   *  This method may delete the object if no strong pointer holds a
   *  reference to it.
   */
  void release_object ();

protected:
  /**
   *  @brief Detach this object from all events it was registered as listener
   */
  void detach_from_all_events ();

  /**
   *  @brief Resets all references to this object
   *  This method will release all references within weak and shared pointers.
   */
  void reset ();

private:
  friend class WeakOrSharedPtr;

  void register_ptr (WeakOrSharedPtr *p);
  void unregister_ptr (WeakOrSharedPtr *p);
  bool has_strong_references () const;

  WeakOrSharedPtr *mp_ptrs;  
};

/**
 *  @brief The base class for weak and shared pointers
 *  This class is used for implementation of the weak and shared pointers.
 */
class TL_PUBLIC WeakOrSharedPtr
{
public:
  /**
   *  @brief Constructor
   */
  WeakOrSharedPtr ();

  /**
   *  @brief Copy constructor
   */
  WeakOrSharedPtr (const WeakOrSharedPtr &o);

  /**
   *  @brief Constructor from an object pointer and a shared flag
   *  The shared flag indicates whether the pointer will behave shared or weak pointer like.
   */
  WeakOrSharedPtr (Object *t, bool shared, bool is_event = false);

  /**
   *  @brief Destructor
   */
  virtual ~WeakOrSharedPtr ();

  /**
   *  @brief Assignment
   */
  WeakOrSharedPtr &operator= (const WeakOrSharedPtr &o);

  /**
   *  @brief Gets the underlying basic object pointer
   */
  Object *get ();

  /**
   *  @brief Gets the underlying basic object pointer (const version)
   */
  const Object *get () const;

  /**
   *  @brief Gets a value indicating whether the object is non-null
   *  This conversion operator allows using the shared pointers inside if
   *  conditions.
   */
  operator bool () const
  {
    return get () != 0;
  }

  /**
   *  @brief Detaches this object from all events
   *  This method will remove this object from all events. After calling this method, the
   *  object will no longer receive events.
   */
  void detach_from_all_events ();

  /**
   *  @brief Indicates that this object is an event
   *  This property is intended for internal use only.
   */
  bool is_event () const
  {
    return m_is_event;
  }

  /**
   *  @brief Indicates that this
   *  @return
   */
  bool is_shared () const
  {
    return m_is_shared;
  }

protected:
  /**
   *  @brief This method is called when the pointer is reset because the object is deleted
   *  This method provides a way to detect whether the object gets detached or destroyed.
   *  It is used by the containers to automatically remove the object from the list.
   *  TODO: this tiny feature requires us to become virtual ...
   */
  virtual void reset_object ();

  /**
   *  @brief Resets the pointer
   */
  void reset (Object *t, bool is_shared, bool is_event);

private:
  friend class Object;

  WeakOrSharedPtr *mp_next, *mp_prev;
  Object *mp_t;
  bool m_is_shared : 1;
  bool m_is_event : 1;
  static tl::Mutex &lock ();
};

/**
 *  @brief A type specialization of a weak or shared pointer
 *  This class represents a weak or shared pointer for the given type T.
 */
template <class T, bool Shared>
class TL_PUBLIC_TEMPLATE weak_or_shared_ptr
  : public WeakOrSharedPtr
{
public:
  /**
   *  @brief Default constructor
   */
  weak_or_shared_ptr ()
    : WeakOrSharedPtr ()
  {
  }

  /**
   *  @brief Copy constructor
   */
  weak_or_shared_ptr (const weak_or_shared_ptr<T, Shared> &o)
    : WeakOrSharedPtr (o)
  {
  }

  /**
   *  @brief Constructor from a direct pointer
   */
  weak_or_shared_ptr (T *t, bool is_event = false)
    : WeakOrSharedPtr (t, Shared, is_event)
  {
  }

  /**
   *  @brief Assignment
   */
  weak_or_shared_ptr<T, Shared> &operator= (const weak_or_shared_ptr<T, Shared> &o)
  {
    WeakOrSharedPtr::operator= (o);
    return *this;
  }

  /**
   *  @brief Equality to pointer
   */
  bool operator== (const T *other) const
  {
    return get () == other;
  }

  /**
   *  @brief Equality to other pointer
   */
  bool operator== (const weak_or_shared_ptr<T, Shared> &other) const
  {
    return get () == other.get ();
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const T *other) const
  {
    return get () != other;
  }

  /**
   *  @brief Inequality from other pointer
   */
  bool operator!= (const weak_or_shared_ptr<T, Shared> &other) const
  {
    return get () != other.get ();
  }

  /**
   *  @brief Access operator
   */
  T *get ()
  {
    return dynamic_cast<T *> (WeakOrSharedPtr::get ());
  }

  /**
   *  @brief Access operator (const version)
   */
  const T *get () const
  {
    return dynamic_cast<const T *> (WeakOrSharedPtr::get ());
  }

  /**
   *  @brief Access operator
   */
  T *operator-> ()
  {
    return get ();
  }

  /**
   *  @brief Access operator (const version)
   */
  const T *operator-> () const
  {
    return get ();
  }

  /**
   *  @brief Dereferencing operator
   */
  T &operator* ()
  {
    T *t = get ();
    tl_assert (t != 0);
    return *t;
  }

  /**
   *  @brief Dereferencing operator (const version)
   */
  const T &operator* () const
  {
    const T *t = get ();
    tl_assert (t != 0);
    return *t;
  }

  /**
   *  @brief Resets the pointer to the given object
   *  0 can be passed to clear the pointer.
   */
  void reset (T *t, bool is_event = false)
  {
    WeakOrSharedPtr::reset (t, Shared, is_event);
  }
};

/**
 *  @brief The weak pointer class
 *  See description of tl::Object for details.
 */
template <class T>
class TL_PUBLIC_TEMPLATE weak_ptr
  : public weak_or_shared_ptr<T, false>
{
public:
  /**
   *  @brief Default constructor
   */
  weak_ptr ()
    : weak_or_shared_ptr<T, false> ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Constructor from a raw pointer
   */
  weak_ptr (T *t, bool is_event = false)
    : weak_or_shared_ptr<T, false> (t, is_event)
  {
    //  .. nothing yet ..
  }
};

/**
 *  @brief The shared pointer class
 *  See description of tl::Object for details.
 */
template <class T>
class TL_PUBLIC_TEMPLATE shared_ptr
  : public weak_or_shared_ptr<T, true>
{
public:
  /**
   *  @brief Default constructor
   */
  shared_ptr ()
    : weak_or_shared_ptr<T, true> ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Constructor from a raw pointer
   */
  shared_ptr (T *t)
    : weak_or_shared_ptr<T, true> (t)
  {
    //  .. nothing yet ..
  }
};

}

#endif

