
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

#include "dbNetlistObject.h"

namespace db
{

NetlistObject::NetlistObject ()
  : tl::Object (), mp_properties (0)
{
  //  .. nothing yet ..
}

NetlistObject::NetlistObject (const db::NetlistObject &other)
  : tl::Object (other), mp_properties (0)
{
  if (other.mp_properties) {
    mp_properties = new std::map<tl::Variant, tl::Variant> (*other.mp_properties);
  }
}

NetlistObject::~NetlistObject ()
{
  delete mp_properties;
  mp_properties = 0;
}

NetlistObject &NetlistObject::operator= (const NetlistObject &other)
{
  if (this != &other) {

    tl::Object::operator= (other);

    delete mp_properties;
    mp_properties = 0;

    if (other.mp_properties) {
      mp_properties = new std::map<tl::Variant, tl::Variant> (*other.mp_properties);
    }

  }
  return *this;
}

tl::Variant NetlistObject::property (const tl::Variant &key) const
{
  if (! mp_properties) {
    return tl::Variant ();
  }

  std::map<tl::Variant, tl::Variant>::const_iterator i = mp_properties->find (key);
  if (i == mp_properties->end ()) {
    return tl::Variant ();
  } else {
    return i->second;
  }
}

void
NetlistObject::set_property (const tl::Variant &key, const tl::Variant &value)
{
  if (value.is_nil ()) {

    if (mp_properties) {
      mp_properties->erase (key);
      if (mp_properties->empty ()) {
        delete mp_properties;
        mp_properties = 0;
      }
    }

  } else {
    if (! mp_properties) {
      mp_properties = new std::map<tl::Variant, tl::Variant> ();
    }
    (*mp_properties) [key] = value;
  }
}

static NetlistObject::property_table empty_properties;

NetlistObject::property_iterator
NetlistObject::begin_properties () const
{
  return mp_properties ? mp_properties->begin () : empty_properties.begin ();
}

NetlistObject::property_iterator
NetlistObject::end_properties () const
{
  return mp_properties ? mp_properties->end () : empty_properties.end ();
}

}
