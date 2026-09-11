
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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


#ifndef _HDR_tlUnknownUserClass
#define _HDR_tlUnknownUserClass

#include "tlCommon.h"
#include <string>

namespace tl
{

/**
 *  @brief A placeholder class for an object with unknown class
 *
 *  This placeholder class is used to represent serialized versions
 *  of objects which are unknown to this version of KLayout.
 *  It stores the class name and payload string, so string
 *  serialization can be maintained for objects without having the
 *  actual class available.
 */

class TL_PUBLIC UnknownUserClass
{
public:
  UnknownUserClass (const std::string &cls_name, const std::string &payload)
    : m_cls_name (cls_name), m_payload (payload)
  {
    //  .. nothing yet ..
  }

  const std::string class_name () const
  {
    return m_cls_name;
  }

  const std::string &payload () const
  {
    return m_payload;
  }

private:
  std::string m_cls_name, m_payload;
};

}

#endif
