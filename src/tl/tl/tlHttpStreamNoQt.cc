
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

#include "tlHttpStream.h"
#include "tlEvents.h"

namespace tl
{

// ---------------------------------------------------------------
//  Dummy InputHttpStream implementation

InputHttpStream::InputHttpStream (const std::string &)
{
  tl_assert (false);
}

InputHttpStream::~InputHttpStream ()
{
  //  .. nothing yet ..
}

void
InputHttpStream::set_credential_provider (HttpCredentialProvider *)
{
  //  .. nothing yet ..
}

void
InputHttpStream::send ()
{
  //  .. nothing yet ..
}

void
InputHttpStream::close ()
{
  //  .. nothing yet ..
}

void
InputHttpStream::set_request (const char *)
{
  //  .. nothing yet ..
}

void
InputHttpStream::set_data (const char *)
{
  //  .. nothing yet ..
}

void
InputHttpStream::set_data (const char *, size_t)
{
  //  .. nothing yet ..
}

void
InputHttpStream::add_header (const std::string &, const std::string &)
{
  //  .. nothing yet ..
}

tl::Event &
InputHttpStream::ready ()
{
  //  .. nothing yet ..
  static tl::Event dummy;
  return dummy;
}

bool
InputHttpStream::data_available ()
{
  //  .. nothing yet ..
  return false;
}

size_t
InputHttpStream::read (char *, size_t)
{
  //  .. nothing yet ..
  return 0;
}

void
InputHttpStream::reset ()
{
  //  .. nothing yet ..
}

std::string
InputHttpStream::source () const
{
  //  .. nothing yet ..
  return std::string ();
}

std::string
InputHttpStream::absolute_path () const
{
  //  .. nothing yet ..
  return std::string ();
}

std::string
InputHttpStream::filename () const
{
  //  .. nothing yet ..
  return std::string ();
}

bool
InputHttpStream::is_available ()
{
  return false;
}

void
InputHttpStream::tick ()
{
  //  .. nothing yet ..
}

void
InputHttpStream::set_timeout (double)
{
  //  .. nothing yet ..
}

double
InputHttpStream::timeout () const
{
  return 0.0;
}


}
