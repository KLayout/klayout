
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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

#include "dbNetlistProperty.h"
#include "tlString.h"

#include <typeinfo>

namespace tl
{

void VariantUserClass<db::NetlistProperty>::destroy (void *p) const
{
  delete (db::NetlistProperty *) p;
}

bool VariantUserClass<db::NetlistProperty>::equal (const void *a, const void *b) const
{
  const db::NetlistProperty *pa = (db::NetlistProperty *) a;
  const db::NetlistProperty *pb = (db::NetlistProperty *) b;
  if (typeid (*pa) == typeid (*pb)) {
    return pa->equals (pb);
  } else {
    return false;
  }
}

bool VariantUserClass<db::NetlistProperty>::less (const void *a, const void *b) const
{
  const db::NetlistProperty *pa = (db::NetlistProperty *) a;
  const db::NetlistProperty *pb = (db::NetlistProperty *) b;
  if (typeid (*pa) == typeid (*pb)) {
    return pa->less (pb);
  } else {
    return typeid (*pa).before (typeid (*pb));
  }
}

void *VariantUserClass<db::NetlistProperty>::clone (const void *p) const
{
  return ((const db::NetlistProperty *) p)->clone ();
}

std::string VariantUserClass<db::NetlistProperty>::to_string (const void *p) const
{
  return ((const db::NetlistProperty *) p)->to_string ();
}

void VariantUserClass<db::NetlistProperty>::read (void * /*p*/, tl::Extractor & /*ex*/) const
{
  //  .. nothing yet ..
  return;
}

void VariantUserClass<db::NetlistProperty>::assign (void *self, const void *other) const
{
  db::NetlistProperty *pself = (db::NetlistProperty *) self;
  const db::NetlistProperty *pother = (const db::NetlistProperty *) other;
  tl_assert (typeid (*pself) == typeid (*pother));
  pself->assign (pother);
}

void *VariantUserClass<db::NetlistProperty>::deref_proxy (tl::Object *proxy) const
{
  return proxy;
}

void VariantUserClass<db::NetlistProperty>::register_instance (const tl::VariantUserClassBase *inst, bool is_const)
{
  VariantUserClassBase::register_instance (inst, typeid (db::NetlistProperty), is_const);
}

void VariantUserClass<db::NetlistProperty>::unregister_instance (const tl::VariantUserClassBase *inst, bool is_const)
{
  VariantUserClassBase::unregister_instance (inst, typeid (db::NetlistProperty), is_const);
}

}

namespace db
{

// --------------------------------------------------------------------------------------------
//  NetlistProperty Implementation

NetlistProperty::NetlistProperty ()
{
  //  .. nothing yet ..
}

NetlistProperty::NetlistProperty (const NetlistProperty &)
{
  //  .. nothing yet ..
}

NetlistProperty::~NetlistProperty ()
{
  //  .. nothing yet ..
}

const tl::VariantUserClass<db::NetlistProperty> *NetlistProperty::variant_class ()
{
  static tl::VariantUserClass<db::NetlistProperty> s_cls;
  return &s_cls;
}

// --------------------------------------------------------------------------------------------
//  DeviceTerminalProperty Implementation

DeviceTerminalProperty::DeviceTerminalProperty ()
  : NetlistProperty (), m_terminal_id (0), m_device_id (0)
{
  //  .. nothing yet ..
}

DeviceTerminalProperty::DeviceTerminalProperty (const DeviceTerminalProperty &other)
  : NetlistProperty (other), m_terminal_id (other.m_terminal_id), m_device_id (other.m_device_id)
{
  //  .. nothing yet ..
}

DeviceTerminalProperty::DeviceTerminalProperty (size_t device_id, size_t terminal_id)
  : NetlistProperty (), m_terminal_id (terminal_id), m_device_id (device_id)
{
  //  .. nothing yet ..
}

void DeviceTerminalProperty::set_terminal_ref (size_t device_id, size_t terminal_id)
{
  m_device_id = device_id;
  m_terminal_id = terminal_id;
}

DeviceTerminalProperty &DeviceTerminalProperty::operator= (const DeviceTerminalProperty &other)
{
  NetlistProperty::operator= (other);
  if (this != &other) {
    m_terminal_id = other.m_terminal_id;
    m_device_id = other.m_device_id;
  }
  return *this;
}

bool DeviceTerminalProperty::equals (const NetlistProperty *p) const
{
  const DeviceTerminalProperty *pp = static_cast<const DeviceTerminalProperty *> (p);
  return NetlistProperty::equals (p) && m_terminal_id == pp->m_terminal_id && m_device_id == pp->m_device_id;
}

bool DeviceTerminalProperty::less (const NetlistProperty *p) const
{
  if (! NetlistProperty::equals (p)) {
    return NetlistProperty::less (p);
  } else {
    const DeviceTerminalProperty *pp = static_cast<const DeviceTerminalProperty *> (p);
    if (m_terminal_id != pp->m_terminal_id) {
      return m_terminal_id < pp->m_terminal_id;
    } else {
      return m_device_id < pp->m_device_id;
    }
  }
}

void DeviceTerminalProperty::assign (const NetlistProperty *p)
{
  NetlistProperty::assign (p);
  const DeviceTerminalProperty *pp = static_cast<const DeviceTerminalProperty *> (p);
  m_terminal_id = pp->m_terminal_id;
  m_device_id = pp->m_device_id;
}


std::string DeviceTerminalProperty::to_string () const
{
  return tl::to_string (m_device_id) + ":" + tl::to_string (m_terminal_id);
}

}
