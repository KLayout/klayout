
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


#include "gsiInspector.h"

namespace gsi
{

Inspector::Inspector ()
{
  //  .. the base class implementation does nothing ..
}

Inspector::~Inspector ()
{
  //  .. the base class implementation does nothing ..
}

std::string Inspector::description () const
{
  return std::string ();
}

bool Inspector::has_keys () const
{
  return true;
}

std::string Inspector::key (size_t /*index*/) const
{
  return std::string ();
}
  
tl::Variant Inspector::keyv (size_t /*index*/) const
{
  return tl::Variant ();
}

std::string Inspector::type (size_t /*index*/) const
{
  return std::string ();
}

Inspector::Visibility Inspector::visibility (size_t /*index*/) const
{
  return Always;
}

tl::Variant Inspector::value (size_t /*index*/) const
{
  return tl::Variant ();
}

size_t Inspector::count () const
{
  return 0;
}

bool Inspector::has_children (size_t /*index*/) const
{
  return false;
}

Inspector *Inspector::child_inspector (size_t /*index*/) const
{
  return 0;
}

bool Inspector::equiv (const gsi::Inspector * /*other*/) const
{
  return false;
}

}

