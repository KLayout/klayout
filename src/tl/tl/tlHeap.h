
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


#ifndef HDR_tlHeap
#define HDR_tlHeap

#include "tlCommon.h"

#include "tlAssert.h"

#include <list>

namespace tl
{

/**
 *  @brief A helper class that provides a base class for wrapped pointers for HeapObject
 */
class HeapObjectBase 
{
public:
  virtual ~HeapObjectBase () { }
};

/**
 *  @brief A helper class that provides a wrapper to a custom type for HeapObject
 */
template <class X>
class HeapObjectCont : public HeapObjectBase 
{
public:
  HeapObjectCont (X *x) : mp_x (x) { }
  ~HeapObjectCont () { delete mp_x; }
private:
  X *mp_x;
};

/**
 *  @brief A simple autopointer class to hold references to temporary objects
 */
class TL_PUBLIC HeapObject
{
public:
  HeapObject ();

  ~HeapObject ();

  template <class X>
  void set (X *x) 
  { 
    tl_assert (mp_b == 0);
    mp_b = new HeapObjectCont<X> (x); 
  }

private:
  HeapObjectBase *mp_b;
};

/**
 *  @brief A heap holding objects of an arbitrary type
 *
 *  The heap is basically a storage for temporary objects. 
 *  Such objects are created on the heap and are destroyed automatically
 *  when the heap goes out of scope.
 *
 *  It is guaranteed that objects created on the heap will be 
 *  destroyed in the reverse order they are created.
 */
class TL_PUBLIC Heap
{
public:
  /**
   *  @brief Constructor
   */
  Heap ();

  /**
   *  @brief Destructor
   */
  ~Heap ();

  /**
   *  @brief Register a new object
   */
  template <class X>
  void push (X *x)
  {
    m_objects.push_back (HeapObject ());
    m_objects.back ().set (x);
  }

  /**
   *  @brief Convenience method: create and register an object of type X
   */
  template <class X>
  X *create () 
  {
    X *x = new X ();
    push (x);
    return x;
  }

  /**
   *  @brief Returns a value indicating whether the heap is empty
   */
  bool empty () const
  {
    return m_objects.empty ();
  }

private:
  std::list<HeapObject> m_objects;
};

}

#endif

