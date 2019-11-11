
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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
  : tl::Object ()
{
  //  .. nothing yet ..
}

NetlistObject::NetlistObject (const db::NetlistObject &other)
  : tl::Object (other), m_properties (other.m_properties)
{
  //  .. nothing yet ..
}

NetlistObject &NetlistObject::operator= (const NetlistObject &other)
{
  if (this != &other) {
    tl::Object::operator= (other);
    m_properties = other.m_properties;
  }
  return *this;
}

tl::Variant NetlistObject::property (const tl::Variant &key) const
{
  std::map<tl::Variant, tl::Variant>::const_iterator i = m_properties.find (key);
  if (i == m_properties.end ()) {
    return tl::Variant ();
  } else {
    return i->second;
  }
}

void
NetlistObject::set_property (const tl::Variant &key, const tl::Variant &value)
{
  if (value.is_nil ()) {
    m_properties.erase (key);
  } else {
    m_properties [key] = value;
  }
}

}
