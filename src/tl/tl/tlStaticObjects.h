
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


#ifndef HDR_tlStaticObjects
#define HDR_tlStaticObjects

#include "tlCommon.h"

#include <vector>

namespace tl
{

/**
 *  @brief A helper class for registered objects
 */
class StaticObjectReferenceBase
{
public:
  virtual ~StaticObjectReferenceBase () { };

  virtual void clear() = 0;
};

/**
 *  @brief A helper template for a specific class
 */
template <class X>
class StaticObjectReference : public StaticObjectReferenceBase
{
public:
  StaticObjectReference (X **x)
    : mp_x (x)
  {
    //  .. nothing yet ..
  }

  ~StaticObjectReference ()
  {
    clear ();
  }

  void clear()
  {
    if (mp_x) {
      delete *mp_x;
      *mp_x = 0;
    }
    mp_x = 0;
  }

private:
  X **mp_x;
};

/**
 *  @brief Provides a registration facility for late-created static objects
 *
 *  The basic purpose of this facility is to provide a way to clean up those
 *  objects without having to rely on some destructors being called from the
 *  exit handler. It registers locations where objects are stored and will 
 *  release the stored objects and reset their pointer to 0.
 */
struct TL_PUBLIC StaticObjects
{
  /**
   *  @brief Destructor
   */
  ~StaticObjects ();

  /**
   *  @brief Register a static object 
   *
   *  After registration, the object behind the location is automatically deleted on the static destructor.
   */
  template <class X>
  static void reg (X **x)
  {
    ms_instance.register_object_base (new StaticObjectReference<X> (x));
  }

  /**
   *  @brief Clean up all registered static objects
   *
   *  Caution: if cleanup is not called, the stored objects are never deleted!
   */
  static void cleanup ()
  {
    ms_instance.do_cleanup ();
  }

protected:
  std::vector<StaticObjectReferenceBase *> m_objects;

  static StaticObjects ms_instance;

  void do_cleanup ();
  void register_object_base (StaticObjectReferenceBase *o);
};

}

#endif

