
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

#include "dbPin.h"
#include "tlString.h"

namespace db
{

// --------------------------------------------------------------------------------
//  Pin class implementation

Pin::Pin ()
  : db::NetlistObject (), m_id (0)
{
  //  .. nothing yet ..
}

Pin::Pin (const std::string &name)
  : db::NetlistObject (), m_name (name), m_id (0)
{
  //  .. nothing yet ..
}

std::string Pin::expanded_name () const
{
  if (name ().empty ()) {
    return "$" + tl::to_string (id ());
  } else {
    return name ();
  }
}

}
