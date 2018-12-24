
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
//  NetNameProperty Implementation

NetNameProperty::NetNameProperty ()
  : NetlistProperty ()
{
  //  .. nothing yet ..
}

NetNameProperty::NetNameProperty (const NetNameProperty &other)
  : NetlistProperty (other), m_name (other.m_name)
{
  //  .. nothing yet ..
}

NetNameProperty::NetNameProperty (const std::string &n)
  : NetlistProperty (), m_name (n)
{
  //  .. nothing yet ..
}

NetNameProperty &NetNameProperty::operator= (const NetNameProperty &other)
{
  NetlistProperty::operator= (other);
  if (this != &other) {
    m_name = other.m_name;
  }
  return *this;
}

void NetNameProperty::set_name (const std::string &n)
{
  m_name = n;
}

bool NetNameProperty::equals (const NetlistProperty *p) const
{
  const NetNameProperty *pp = static_cast<const NetNameProperty *> (p);
  return NetlistProperty::equals (p) && m_name == pp->m_name;
}

bool NetNameProperty::less (const NetlistProperty *p) const
{
  if (! NetlistProperty::equals (p)) {
    return NetlistProperty::less (p);
  } else {
    const NetNameProperty *pp = static_cast<const NetNameProperty *> (p);
    return m_name < pp->m_name;
  }
}

void NetNameProperty::assign (const NetlistProperty *p)
{
  NetlistProperty::assign (p);
  const NetNameProperty *pp = static_cast<const NetNameProperty *> (p);
  m_name = pp->m_name;
}


static const char *valid_netname_chars = "_.$[]():-,";

std::string NetNameProperty::to_string () const
{
  return "name:" + tl::to_word_or_quoted_string (m_name, valid_netname_chars);
}

// --------------------------------------------------------------------------------------------
//  DevicePortProperty Implementation

DevicePortProperty::DevicePortProperty ()
  : NetlistProperty ()
{
  //  .. nothing yet ..
}

DevicePortProperty::DevicePortProperty (const DevicePortProperty &other)
  : NetlistProperty (other), m_port_ref (other.m_port_ref)
{
  //  .. nothing yet ..
}

DevicePortProperty::DevicePortProperty (const db::NetPortRef &p)
  : NetlistProperty (), m_port_ref (p)
{
  //  .. nothing yet ..
}

DevicePortProperty &DevicePortProperty::operator= (const DevicePortProperty &other)
{
  NetlistProperty::operator= (other);
  if (this != &other) {
    m_port_ref = other.m_port_ref;
  }
  return *this;
}

void DevicePortProperty::set_port_ref (const db::NetPortRef &p)
{
  m_port_ref = p;
}

bool DevicePortProperty::equals (const NetlistProperty *p) const
{
  const DevicePortProperty *pp = static_cast<const DevicePortProperty *> (p);
  return NetlistProperty::equals (p) && m_port_ref == pp->m_port_ref;
}

bool DevicePortProperty::less (const NetlistProperty *p) const
{
  if (! NetlistProperty::equals (p)) {
    return NetlistProperty::less (p);
  } else {
    const DevicePortProperty *pp = static_cast<const DevicePortProperty *> (p);
    return m_port_ref < pp->m_port_ref;
  }
}

void DevicePortProperty::assign (const NetlistProperty *p)
{
  NetlistProperty::assign (p);
  const DevicePortProperty *pp = static_cast<const DevicePortProperty *> (p);
  m_port_ref = pp->m_port_ref;
}


std::string DevicePortProperty::to_string () const
{
  if (m_port_ref.device () && m_port_ref.port_def ()) {
    return "port:" + tl::to_word_or_quoted_string (m_port_ref.device ()->name ()) + ":" + tl::to_word_or_quoted_string (m_port_ref.port_def ()->name ());
  } else {
    return "port";
  }
}

}
