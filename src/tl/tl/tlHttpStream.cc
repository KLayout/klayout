
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

namespace tl
{

// ---------------------------------------------------------------
//  HttpErrorException implementation

std::string
HttpErrorException::format_error (const std::string &em, int ec, const std::string &url, const std::string &body)
{
  std::string msg = tl::sprintf (tl::to_string (tr ("Error %d: %s, fetching %s")), ec, em, url);

  if (! body.empty ()) {

    msg += "\n\n";
    msg += tl::to_string (tr ("Reply body:"));
    msg += "\n";

    if (body.size () > 1000) {
      msg += std::string (body.c_str (), 1000);
      msg += "...";
    } else {
      msg += body;
    }

  }

  return msg;
}

}
