/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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


#ifndef HDR_tlOptional
#define HDR_tlOptional

#include "tlAssert.h"
#include "tlString.h"
#include "tlCommon.h"

#include <iostream>

namespace tl
{

struct nullopt_t {};

extern const nullopt_t nullopt;

/**
 *  @brief Poor man's partial implementation of C++17's std::optional
 */
template<typename T>
class TL_PUBLIC_TEMPLATE optional
{
public:
  optional () :
    m_value (),
    m_is_valid (false)
  {}

  optional (const nullopt_t &) :
    m_value (),
    m_is_valid (false)
  {}

  optional (const T &value) :
    m_value (value),
    m_is_valid (true)
  {}

  void reset ()
  {
    m_is_valid = false;
  }

  bool has_value() const { return m_is_valid; }

  T &value ()
  {
    tl_assert (m_is_valid);

    return m_value;
  }

  const T &value () const
  {
    tl_assert (m_is_valid);

    return m_value;
  }

  T& operator* ()
  {
    return value ();
  }

  const T& operator* () const
  {
    return value ();
  }

  T* operator-> ()
  {
    return m_is_valid ? &m_value : 0;
  }

  const T* operator-> () const
  {
    return m_is_valid ? &m_value : 0;
  }

private:
  T m_value;
  bool m_is_valid;
};

template<typename T>
optional<T> make_optional (const T &value)
{
  return optional<T> (value);
}

template<typename T>
bool operator== (const optional<T> &lhs, const optional<T> &rhs)
{
  if (lhs.has_value () != rhs.has_value ()) {
    return false;
  }
  if (!lhs.has_value ()) {
    return true;
  }

  return lhs.value() == rhs.value();
}

template<typename T>
bool operator!= (const optional<T> &lhs, const optional<T> &rhs)
{
  return !(lhs == rhs);
}

template<typename T>
std::ostream &operator<< (std::ostream &ostr, const optional<T> &rhs)
{
  if (rhs.has_value()) {
    ostr << rhs.value();
  } else {
    ostr << "<invalid>";
  }

  return ostr;
}

template <class T>
std::string to_string (const optional<T> &opt)
{
  if (opt.has_value ()) {
    return tl::to_string (*opt);
  } else {
    return std::string ();
  }
}

} // namespace tl

#endif /* HDR_tlOptional */
