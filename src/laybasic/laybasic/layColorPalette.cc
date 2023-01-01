
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



#include "layColorPalette.h"
#include "tlString.h"
#include "tlException.h"
#include "tlInternational.h"

namespace lay
{

static const char *def_palette = 
  "255,157,157[0] "
  "255,128,168[1] "
  "192,128,255[2] "
  "149,128,255[3] "
  "128,134,255[4] "
  "128,168,255[5] "
  "255,0,0[6] "
  "255,0,128[7] "
  "255,0,255[8] "
  "128,0,255[9] "
  "0,0,255[10] "
  "0,128,255[11] "
  "128,0,0[12] "
  "128,0,87[13] "
  "128,0,128[14] "
  "80,0,128[15] "
  "0,0,128[16] "
  "0,64,128[17] "
  "128,255,251[18] "
  "128,255,141[19] "
  "175,255,128[20] "
  "243,255,128[21] "
  "255,194,128[22] "
  "255,160,128[23] "
  "0,255,255[24] "
  "1,255,107[25] "
  "145,255,0[26] "
  "221,255,0[27] "
  "255,174,0[28] "
  "255,128,0[29] "
  "0,128,128[30] "
  "0,128,80[31] "
  "0,128,0[32] "
  "80,128,0[33] "
  "128,128,0[34] "
  "128,80,0[35] "
  "255,255,255 "
  "192,192,192 "
  "128,128,128 "
  "96,96,96 "
  "64,64,64 "
  "0,0,0";

ColorPalette 
ColorPalette::default_palette ()
{
  ColorPalette p;
  p.from_string (def_palette);
  return p;
}

ColorPalette::ColorPalette ()
{
  // .. nothing yet ..
}

ColorPalette::ColorPalette (const std::vector<tl::color_t> &colors, const std::vector<unsigned int> &luminous_colors)
  : m_colors (colors), m_luminous_color_indices (luminous_colors)
{
  // .. nothing yet ..
}

ColorPalette::ColorPalette (const ColorPalette &d)
  : m_colors (d.m_colors), m_luminous_color_indices (d.m_luminous_color_indices)
{
  // .. nothing yet ..
}

ColorPalette 
ColorPalette::operator= (const ColorPalette &d)
{
  if (&d != this) {
    m_colors = d.m_colors;
    m_luminous_color_indices = d.m_luminous_color_indices;
  }
  return *this;
}

bool 
ColorPalette::operator== (const ColorPalette &d) const
{
  return m_colors == d.m_colors && m_luminous_color_indices == d.m_luminous_color_indices;
}

tl::color_t 
ColorPalette::color_by_index (unsigned int n) const
{
  return m_colors [n % colors ()];
}

unsigned int 
ColorPalette::colors () const
{
  return (unsigned int) m_colors.size ();
}

unsigned int
ColorPalette::luminous_color_index_by_index (unsigned int n) const
{
  return m_luminous_color_indices [n % luminous_colors ()];
}

unsigned int 
ColorPalette::luminous_colors () const
{
  return (unsigned int) m_luminous_color_indices.size ();
}

void 
ColorPalette::set_color (unsigned int n, tl::color_t c)
{
  while (m_colors.size () <= n) {
    m_colors.push_back (0);
  }
  m_colors [n] = c | 0xff000000;
}

void
ColorPalette::clear_colors () 
{
  m_colors.clear ();
}

void 
ColorPalette::set_luminous_color_index (unsigned int n, unsigned int ci)
{
  while (m_luminous_color_indices.size () <= n) {
    m_luminous_color_indices.push_back (0);
  }
  m_luminous_color_indices [n] = ci;
}

void 
ColorPalette::clear_luminous_colors ()
{
  m_luminous_color_indices.clear ();
}

std::string 
ColorPalette::to_string () const
{
  std::string res;

  for (unsigned int i = 0; i < m_colors.size (); ++i) {

    if (i > 0) {
      res += " ";
    }

    tl::color_t c = m_colors [i];
    res += tl::sprintf ("%d,%d,%d", (c >> 16) & 0xff, (c >> 8) & 0xff, c & 0xff);

    for (unsigned int j = 0; j < m_luminous_color_indices.size (); ++j) {
      if (m_luminous_color_indices [j] == i) {
        res += tl::sprintf ("[%d]", j);
        break;
      }
    }

  }

  return res;
}

void 
ColorPalette::from_string (const std::string &s, bool simple)
{
  try {

    m_colors.clear ();
    m_luminous_color_indices.clear ();

    tl::Extractor x (s.c_str ());

    unsigned int i = 0;

    while (true) {

      unsigned int r = 0, g = 0, b = 0;
      unsigned int lc = 0;

      if (! x.try_read (r)) {
        break;
      }
      x.expect (",").read (g).expect (",").read (b);

      m_colors.push_back (0xff000000 | (tl::color_t (r & 0xff) << 16) | (tl::color_t (g & 0xff) << 8) | tl::color_t (b & 0xff));

      if (x.test ("[")) {
        x.read (lc).expect ("]");
        while (m_luminous_color_indices.size () <= lc) {
          m_luminous_color_indices.push_back (0);
        } 
        m_luminous_color_indices [lc] = i;
      }

      ++i;

    }

    if (! x.at_end ()) {
      throw tl::Exception (tl::sprintf (tl::to_string (tr ("unexpected characters: %s")), x.skip ()));
    }

    if (! simple && (colors () == 0 || luminous_colors () == 0)) {
      throw tl::Exception (tl::to_string (tr ("invalid palette - no colors and/or default colors")));
    }

  } catch (std::exception &ex) {
    //  reformat error message
    throw tl::Exception (tl::sprintf (tl::to_string (tr ("Color palette string format error: %s")), ex.what ()));
  }
}

}

