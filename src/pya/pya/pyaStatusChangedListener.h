
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


#ifndef _HDR_pyaStatusChangedListener
#define _HDR_pyaStatusChangedListener

#include <Python.h>

#include "pyaRefs.h"

#include "gsiSignals.h"
#include "gsiObject.h"

namespace pya
{

class PYAObjectBase;

/**
 *  @brief A helper object to forward status changed events to a Python object
 *  This object is used to connect the events to the Python object. Unfortunately,
 *  PYAObjectBase cannot be derived from tl::Object directly since in that case,
 *  tl::Object will be placed before PyObject in the memory layout.
 */
class StatusChangedListener
  : public tl::Object
{
public:
  StatusChangedListener (PYAObjectBase *pya_object);

  void object_status_changed (gsi::ObjectBase::StatusEventType type);

  PYAObjectBase *pya_object () const
  {
    return mp_pya_object;
  }

private:
  PYAObjectBase *mp_pya_object;
};

}

#endif
