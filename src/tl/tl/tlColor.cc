
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


#include "tlColor.h"
#include "tlString.h"
#include "tlMath.h"

#include <ctype.h>
#include <algorithm>

namespace tl
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

#if defined(HAVE_QT)
Color::Color (const QColor &qc)
  : m_color (0)
{
  if (qc.isValid ()) {
    m_color = qc.rgba ();
  }
}
#endif

Color::Color (unsigned int r, unsigned int g, unsigned int b, unsigned int alpha)
  : m_color ((b & 0xff) | ((g & 0xff) << 8) | ((r & 0xff) << 16) | ((alpha & 0xff) << 24))
{
  //  .. nothing yet ..
}

Color::Color (const std::string &name)
{
  m_color = 0;
  init_from_string (name.c_str ());
}

Color::Color (const char *name)
{
  m_color = 0;
  init_from_string (name);
}

void
Color::init_from_string (const char *s)
{
  tl::Extractor ex (s);

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

#if defined(HAVE_QT)
QColor
Color::to_qc () const
{
  return is_valid () ? QColor (rgb ()) : QColor ();
}
#endif

bool
Color::is_valid () const
{
  return (m_color & 0xff000000) != 0;
}

void
Color::get_hsv (unsigned int &hue, unsigned int &saturation, unsigned int &value) const
{
  double r = double (red ()) / 255.0;
  double g = double (green ()) / 255.0;
  double b = double (blue ()) / 255.0;

  double max = std::max (r, std::max (g, b));
  double min = std::min (r, std::min (g, b));
  double delta = max - min;

  value = (unsigned int) tl::round (255.0 * max, 1);
  hue = 0;
  saturation = 0;

  if (! tl::equal (delta, 0.0)) {

    saturation = (unsigned int) tl::round (255.0 * delta / max, 1);
    double h = 0.0;
    if (tl::equal (r, max)) {
      h = (g - b) / delta;
    } else if (tl::equal (g, max)) {
      h = 2.0f + (b - r) / delta;
    } else if (tl::equal (b, max)) {
      h = 4.0f + (r - g) / delta;
    }
    h *= 60.0;
    if (tl::less (h, 0.0)) {
      h += 360.0;
    }

    hue = (unsigned int) tl::round (h, 1);

  }
}

static tl::Color color_d (double r, double g, double b)
{
  return tl::Color (tl::round (r * 255.0, 1), tl::round (g * 255.0, 1), tl::round (b * 255.0, 1));
}

tl::Color
Color::from_hsv (unsigned int hue, unsigned int saturation, unsigned int value)
{
  if (saturation == 0) {
    return tl::Color (value, value, value);
  }

  hue = (hue + 360) % 360;

  double h = double (hue) / 60.0;
  double s = double (saturation) / 255.0;
  double v = double (value) / 255.0;

  int i = int (tl::round_down (h, 1));
  double f = (i & 1) != 0 ? h - i : 1.0 - h + i;
  double p = v * (1.0 - s);
  double q = v * (1.0 - s * f);

  switch (i) {
  case 0:
    return color_d (v, q, p);
  case 1:
    return color_d (q, v, p);
  case 2:
    return color_d (p, v, q);
  case 3:
    return color_d (p, q, v);
  case 4:
    return color_d (q, p, v);
  case 5:
    return color_d (v, p, q);
  default:
    return tl::Color ();
  }

}

}
