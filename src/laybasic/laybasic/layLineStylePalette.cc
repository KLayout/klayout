
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



#include "layLineStylePalette.h"
#include "tlString.h"
#include "tlException.h"
#include "tlInternational.h"

namespace lay
{

static const char *def_palette = 
  "0 1 2 3";

static LineStylePalette def_palette_object = LineStylePalette::default_palette ();

LineStylePalette 
LineStylePalette::default_palette ()
{
  LineStylePalette p;
  p.from_string (def_palette);
  return p;
}

LineStylePalette::LineStylePalette ()
{
  // .. nothing yet ..
}

LineStylePalette::LineStylePalette (const std::vector<unsigned int> &styles)
  : m_styles (styles)
{
  // .. nothing yet ..
}

LineStylePalette::LineStylePalette (const LineStylePalette &d)
  : m_styles (d.m_styles)
{
  // .. nothing yet ..
}

LineStylePalette 
LineStylePalette::operator= (const LineStylePalette &d)
{
  if (&d != this) {
    m_styles = d.m_styles;
  }
  return *this;
}

bool 
LineStylePalette::operator== (const LineStylePalette &d) const
{
  return m_styles == d.m_styles;
}

unsigned int
LineStylePalette::style_by_index (unsigned int n) const
{
  if (styles () == 0) {
    //  fallback for corrupt palette
    return def_palette_object.style_by_index (n);
  } else {
    return m_styles [n % styles ()];
  }
}

unsigned int 
LineStylePalette::styles () const
{
  return (unsigned int) m_styles.size ();
}

void 
LineStylePalette::set_style (unsigned int n, unsigned int s)
{
  while (m_styles.size () <= n) {
    m_styles.push_back (0);
  }
  m_styles [n] = s;
}

void
LineStylePalette::clear_styles () 
{
  m_styles.clear ();
}

std::string 
LineStylePalette::to_string () const
{
  std::string res;

  for (unsigned int i = 0; i < m_styles.size (); ++i) {
    if (i > 0) {
      res += " ";
    }
    unsigned int s = m_styles [i];
    res += tl::sprintf ("%d", s);
  }

  return res;
}

void 
LineStylePalette::from_string (const std::string &s)
{
  try {

    m_styles.clear ();

    tl::Extractor x (s.c_str ());

    unsigned int i = 0;

    while (true) {

      unsigned int s = 0;

      if (! x.try_read (s)) {
        break;
      }

      m_styles.push_back (s);

      ++i;

    }

    if (! x.at_end ()) {
      throw tl::Exception (tl::sprintf (tl::to_string (tr ("unexpected characters: %s")), x.skip ()));
    }

    if (styles () == 0) {
      throw tl::Exception (tl::to_string (tr ("invalid line style palette - no styles")));
    }

  } catch (std::exception &ex) {
    //  reformat error message
    throw tl::Exception (tl::sprintf (tl::to_string (tr ("Line style palette string format error: %s")), ex.what ()));
  }
}

}

