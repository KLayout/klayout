
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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


#include "layColor.h"
#include "tlString.h"

#include <ctype.h>

namespace lay
{

Color::Color ()
  : m_color (0)
{
  //  .. nothing yet ..
}

Color::Color (color_t color)
  : m_color (color | 0xff000000)
{
  //  .. nothing yet ..
}

Color::Color (unsigned int r, unsigned int g, unsigned int b, unsigned int alpha)
  : m_color ((b & 0xff) | ((g & 0xff) << 8) | ((r & 0xff) << 16) | ((alpha & 0xff) << 24))
{
  //  .. nothing yet ..
}

Color::Color (const std::string &name)
{
  m_color = 0;

  tl::Extractor ex (name.c_str ());

  unsigned int n = 0;

  ex.test ("#");
  while (! ex.at_end ()) {
    char c = tolower (*ex.get ());
    if (c >= '0' && c <= '9') {
      m_color <<= 4;
      m_color |= (c - '0');
      ++n;
    } else if (c >= 'a' && c <= 'f') {
      m_color <<= 4;
      m_color |= (c - 'a') + 10;
      ++n;
    }
    ++ex;
  }

  if (n == 0) {
    m_color = 0;
  } else if (n <= 3) {
    m_color = ((m_color & 0xf) * 0x11) | ((m_color & 0xf0) * 0x110) | ((m_color & 0xf00) * 0x1100) | 0xff000000;
  } else if (n <= 4) {
    m_color = ((m_color & 0xf) * 0x11) | ((m_color & 0xf0) * 0x110) | ((m_color & 0xf00) * 0x1100) | ((m_color & 0xf000) * 0x11000);
  } else if (n <= 6) {
    m_color |= 0xff000000;
  }
}

std::string
Color::to_string () const
{
  if (! is_valid ()) {

    return std::string ();

  } else {

    unsigned int n = 8;
    if ((m_color & 0xff000000) == 0xff000000) {
      n = 6;
    }

    uint32_t c = m_color;
    char s [10];
    s[n + 1] = 0;
    s[0] = '#';
    while (n > 0) {
      s [n] = "0123456789abcdef" [c & 0xf];
      c >>= 4;
      --n;
    }

    return std::string (s);

  }
}

bool
Color::is_valid () const
{
  return (m_color & 0xff000000) != 0;
}

}
