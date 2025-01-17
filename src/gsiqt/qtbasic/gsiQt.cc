
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include "gsiQt.h"

#include <QGraphicsItem>
#include <QGraphicsObject>
#include <QVariant>

namespace qt_gsi
{

// ---------------------------------------------------------------------------
//  AbstractMethodCalledException implementation

AbstractMethodCalledException::AbstractMethodCalledException (const char *method_name)
  : tl::Exception (tl::to_string (QObject::tr ("Abstract method called (%s)")).c_str (), method_name)
{
  //  .. nothing yet ..
}

// ---------------------------------------------------------------------------
//  QtObjectBase implementation

void QtObjectBase::init(QObject *object)
{
  //  This method is called whenever a QtObjectBase object is created as a GSI 
  //  interface for a QObject. If there is a parent, we can make C++ hold the reference to that
  //  object, thus moving the ownership to the C++ parent.
  if (object->parent ()) {
    keep ();
  }
}

void QtObjectBase::init(QGraphicsItem *object)
{
  //  This method is called whenever a QtObjectBase object is created as a GSI 
  //  interface for a QGraphicsItem. If there is a parent, we can make C++ hold the reference to that
  //  object, thus moving the ownership to the C++ parent.
  if (object->parentItem ()) {
    keep ();
  }
}

void QtObjectBase::init(QGraphicsObject *object)
{
  //  This method is called whenever a QtObjectBase object is created as a GSI 
  //  interface for a QGraphicsObject. If there is a parent, we can make C++ hold the reference to that
  //  object, thus moving the ownership to the C++ parent.
  if (object->parentItem () || object->parent ()) {
    keep ();
  }
}

}

