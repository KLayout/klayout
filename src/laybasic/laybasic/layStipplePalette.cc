
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



#include "layStipplePalette.h"
#include "tlString.h"
#include "tlException.h"
#include "tlInternational.h"

namespace lay
{

static const char *def_palette = 
  "0 1 2 3 "
  "4 5[1] 6 7 "
  "8 9[0] 10 11 "
  "12 13 14 15 ";

static StipplePalette def_palette_object = StipplePalette::default_palette ();

StipplePalette 
StipplePalette::default_palette ()
{
  StipplePalette p;
  p.from_string (def_palette);
  return p;
}

StipplePalette::StipplePalette ()
{
  // .. nothing yet ..
}

StipplePalette::StipplePalette (const std::vector<unsigned int> &stipples, const std::vector<unsigned int> &standard)
  : m_stipples (stipples), m_standard (standard)
{
  // .. nothing yet ..
}

StipplePalette::StipplePalette (const StipplePalette &d)
  : m_stipples (d.m_stipples), m_standard (d.m_standard)
{
  // .. nothing yet ..
}

StipplePalette 
StipplePalette::operator= (const StipplePalette &d)
{
  if (&d != this) {
    m_stipples = d.m_stipples;
    m_standard = d.m_standard;
  }
  return *this;
}

bool 
StipplePalette::operator== (const StipplePalette &d) const
{
  return m_stipples == d.m_stipples && m_standard == d.m_standard;
}

unsigned int
StipplePalette::stipple_by_index (unsigned int n) const
{
  if (stipples () == 0) {
    //  fallback for corrupt palette
    return def_palette_object.stipple_by_index (n);
  } else {
    return m_stipples [n % stipples ()];
  }
}

unsigned int 
StipplePalette::stipples () const
{
  return (unsigned int) m_stipples.size ();
}

unsigned int
StipplePalette::standard_stipple_index_by_index (unsigned int n) const
{
  if (standard_stipples () == 0) {
    //  fallback for corrupt palette
    return def_palette_object.standard_stipple_index_by_index (n);
  } else {
    return m_standard [n % standard_stipples ()];
  }
}

unsigned int 
StipplePalette::standard_stipples () const
{
  return (unsigned int) m_standard.size ();
}

void 
StipplePalette::set_stipple (unsigned int n, unsigned int s)
{
  while (m_stipples.size () <= n) {
    m_stipples.push_back (0);
  }
  m_stipples [n] = s;
}

void
StipplePalette::clear_stipples () 
{
  m_stipples.clear ();
}

void 
StipplePalette::set_standard_stipple_index (unsigned int n, unsigned int si)
{
  while (m_standard.size () <= n) {
    m_standard.push_back (0);
  }
  m_standard [n] = si;
}

void 
StipplePalette::clear_standard_stipples ()
{
  m_standard.clear ();
}

std::string 
StipplePalette::to_string () const
{
  std::string res;

  for (unsigned int i = 0; i < m_stipples.size (); ++i) {

    if (i > 0) {
      res += " ";
    }

    unsigned int s = m_stipples [i];
    res += tl::sprintf ("%d", s);

    for (unsigned int j = 0; j < m_standard.size (); ++j) {
      if (m_standard [j] == i) {
        res += tl::sprintf ("[%d]", j);
        break;
      }
    }

  }

  return res;
}

void 
StipplePalette::from_string (const std::string &s)
{
  try {

    m_stipples.clear ();
    m_standard.clear ();

    tl::Extractor x (s.c_str ());

    unsigned int i = 0;

    while (true) {

      unsigned int s = 0;
      unsigned int st = 0;

      if (! x.try_read (s)) {
        break;
      }

      m_stipples.push_back (s);

      if (x.test ("[")) {
        x.read (st).expect ("]");
        while (m_standard.size () <= st) {
          m_standard.push_back (0);
        } 
        m_standard [st] = i;
      }

      ++i;

    }

    if (! x.at_end ()) {
      throw tl::Exception (tl::sprintf (tl::to_string (tr ("unexpected characters: %s")), x.skip ()));
    }

    if (stipples () == 0 || standard_stipples () == 0) {
      throw tl::Exception (tl::to_string (tr ("invalid palette - no stipples and/or standard stipples")));
    }

  } catch (std::exception &ex) {
    //  reformat error message
    throw tl::Exception (tl::sprintf (tl::to_string (tr ("Stipple palette string format error: %s")), ex.what ()));
  }
}

}

