
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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


#include "layFixedFont.h"
#include "fixedFont.h"

#include <algorithm>

namespace lay
{

int FixedFont::ms_default_font_size = 0;

FixedFont::FixedFont (unsigned int h, unsigned int lh, unsigned int w, unsigned char c0, unsigned char nc, uint32_t *d)
  : m_height (h), m_line_height (lh), m_width (w), m_first_char (c0), m_n_chars (nc), mp_data (d)
{
  // .. nothing yet ..
}

void 
FixedFont::set_default_font_size (int fs)
{
  ms_default_font_size = std::min (2, std::max (0, fs));
}

const FixedFont &
FixedFont::get_font (double resolution)
{
  int fs = ms_default_font_size;
  int od = std::max (1, std::min (int (sizeof (fonts) / sizeof (fonts [0])), int (1.0 / resolution + 0.5))) - 1;
  return fonts [od * 3 + fs];
}

}

