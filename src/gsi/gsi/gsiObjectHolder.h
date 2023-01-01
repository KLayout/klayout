
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



#ifndef _HDR_gsiObjectHolder
#define _HDR_gsiObjectHolder

#include "gsiCommon.h"

namespace gsi
{

class ClassBase;

/**
 *  @brief Implements an object holder
 *  This is some kind of auto pointer which acts on generic objects based on the 
 *  gsi::ClassBase scheme.
 */
class GSI_PUBLIC ObjectHolder
{
public:
  /**
   *  @brief Constructor
   *  Creates a smart pointer holding an object of class "cls".
   */
  ObjectHolder (const gsi::ClassBase *cls, void *obj);

  /**
   *  @brief Destructor
   */
  ~ObjectHolder ();

  /**
   *  @brief Resets the holder to a new pointer
   */
  void reset (const gsi::ClassBase *cls, void *obj);

  /**
   *  @brief Gets the object while releasing the ownership
   */
  void *release ();

  /**
   *  @brief Gets the object
   */
  void *obj () const
  {
    return mp_obj;
  }

  /**
   *  @brief Gets the class
   */
  const gsi::ClassBase *cls () const
  {
    return mp_cls;
  }

private:
  const gsi::ClassBase *mp_cls;
  void *mp_obj;
};

}

#endif

