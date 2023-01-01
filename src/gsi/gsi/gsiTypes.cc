
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


#include "gsi.h"
#include "gsiDecl.h"

namespace gsi
{

// --------------------------------------------------------------------------------
//  Implementation of ArgType

std::string
ArgType::to_string () const
{
  std::string s;
  if (m_is_cref || m_is_cptr) {
    s = "const ";
  }
  switch (m_type) {
  case T_void:
    s += "void"; break;
  case T_void_ptr:
    s += "void *"; break;
  case T_bool:
    s += "bool"; break;
  case T_char:
    s += "char"; break;
  case T_schar:
    s += "signed char"; break;
  case T_uchar:
    s += "unsigned char"; break;
  case T_short:
    s += "short"; break;
  case T_ushort:
    s += "unsigned short"; break;
  case T_int:
    s += "int"; break;
  case T_uint:
    s += "unsigned int"; break;
  case T_long:
    s += "long"; break;
  case T_ulong:
    s += "unsigned long"; break;
  case T_longlong:
    s += "long long"; break;
  case T_ulonglong:
    s += "unsigned long long"; break;
#if defined(HAVE_64BIT_COORD)
  case T_int128:
    s += "int128"; break;
#endif
  case T_double:
    s += "double"; break;
  case T_float:
    s += "float"; break;
  case T_string:
    s += "string"; break;
  case T_byte_array:
    s += "byte array"; break;
  case T_var:
    s += "variant"; break;
  case T_object:
    if (m_pass_obj) {
      s += "new ";
    }
    s += mp_cls->name (); break;
  case T_map:
    s += "map<";
    if (mp_inner_k) {
      s += mp_inner_k->to_string ();
    } 
    s += ",";
    if (mp_inner) {
      s += mp_inner->to_string ();
    } 
    s += ">";
    break;
  case T_vector:
    if (mp_inner) {
      s += mp_inner->to_string ();
    } 
    s += "[]";
    break;
  }
  if (m_is_cref || m_is_ref) {
    s += " &";
  } else if (m_is_cptr || m_is_ptr) {
    s += " *";
  }
  return s;
}

ArgType::ArgType ()
  : m_type (T_void), mp_spec (0), mp_inner (0), mp_inner_k (0),
    m_is_ref (false), m_is_ptr (false), m_is_cref (false), m_is_cptr (false), m_is_iter (false), 
    m_owns_spec (false), m_pass_obj (false), m_prefer_copy (false),
    mp_cls (0), m_size (0)
{ }

ArgType::~ArgType ()
{
  if (mp_inner) {
    delete mp_inner;
    mp_inner = 0;
  }
  if (mp_inner_k) {
    delete mp_inner_k;
    mp_inner_k = 0;
  }
  release_spec ();
}

ArgType::ArgType (const ArgType &other)
  : m_type (T_void), mp_spec (0), mp_inner (0), mp_inner_k (0),
    m_is_ref (false), m_is_ptr (false), m_is_cref (false), m_is_cptr (false), m_is_iter (false), 
    m_owns_spec (false), m_pass_obj (false), m_prefer_copy (false),
    mp_cls (0), m_size (0)
{
  operator= (other);
}

ArgType &
ArgType::operator= (const ArgType &other)
{
  if (this != &other) {

    release_spec ();

    if (other.mp_spec) {
      if (other.m_owns_spec) {
        mp_spec = other.mp_spec->clone ();
      } else {
        mp_spec = other.mp_spec;
      }
      m_owns_spec = other.m_owns_spec;
    }

    m_type = other.m_type;
    m_pass_obj = other.m_pass_obj;
    m_prefer_copy = other.m_prefer_copy;
    m_is_ref = other.m_is_ref;
    m_is_cref = other.m_is_cref;
    m_is_ptr = other.m_is_ptr;
    m_is_cptr = other.m_is_cptr;
    m_is_iter = other.m_is_iter;
    mp_cls = other.mp_cls;
    m_size = other.m_size;

    if (mp_inner) {
      delete mp_inner;
      mp_inner = 0;
    }

    if (other.mp_inner) {
      mp_inner = new ArgType (*other.mp_inner);
    }

    if (mp_inner_k) {
      delete mp_inner_k;
      mp_inner_k = 0;
    }

    if (other.mp_inner_k) {
      mp_inner_k = new ArgType (*other.mp_inner_k);
    }

  }

  return *this;
}

bool 
ArgType::operator== (const ArgType &b) const
{
  if ((mp_inner == 0) != (b.mp_inner == 0)) {
    return false;
  }
  if (mp_inner && *mp_inner != *b.mp_inner) {
    return false;
  }
  if ((mp_inner_k == 0) != (b.mp_inner_k == 0)) {
    return false;
  }
  if (mp_inner_k && *mp_inner_k != *b.mp_inner_k) {
    return false;
  }
  return m_type == b.m_type && m_is_iter == b.m_is_iter && 
         m_is_ref == b.m_is_ref && m_is_cref == b.m_is_cref && m_is_ptr == b.m_is_ptr && m_is_cptr == b.m_is_cptr && 
         mp_cls == b.mp_cls && m_pass_obj == b.m_pass_obj && m_prefer_copy == b.m_prefer_copy;
}

void 
ArgType::release_spec ()
{
  if (mp_spec && m_owns_spec) {
    delete mp_spec;
  }
  mp_spec = 0;
  m_owns_spec = false;
}

}

