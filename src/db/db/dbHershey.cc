
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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



#include "dbHershey.h"
#include "tlInternational.h"


namespace db
{

//  this is the character to replace invalid characters (>end_char)
const char invalid_char = '?';

struct HersheyCharInfo 
{
  HersheyCharInfo (unsigned int e1, unsigned int e2, int w, int y1, int y2)
    : edge_start (e1), edge_end (e2), width (w), ymin (y1), ymax (y2)
  { }

  unsigned int edge_start, edge_end;
  int width;
  int ymin, ymax;
};

struct HersheyFont
{
  HersheyFont (const short (*e) [4], const HersheyCharInfo *c, 
               unsigned char c1, unsigned char c2, int y2, int y1)
    : edges (e), chars (c), start_char (c1), end_char (c2), ymin (y1), ymax (y2)
  { 
    width = ymax;
    height = ymax;

    //  use the "M" characters as measure for the width and height of the font
    char refc = 'M';
    if (start_char <= refc && end_char >= refc) {
      width = chars [refc - start_char].width;
      height = chars [refc - start_char].ymax;
    }
  }

  const short (*edges) [4];
  const HersheyCharInfo *chars;
  unsigned char start_char, end_char;
  int ymin, ymax;
  int width, height;
};

#include "fonts.cc_gen"

const int line_spacing = 4;

static HersheyFont *fonts [] = {
  &futural,     // Default
  &gothiceng,   // Gothic
  &futuram,     // Sans thick
  &futural,     // Stick
  &timesi,      // Times italic
  &timesr,      // Times thin
  &rowmant,     // Times thick
};

std::vector<std::string>
hershey_font_names ()
{
  std::vector<std::string> ff;
  ff.push_back (tl::to_string (tr ("Default")));
  ff.push_back (tl::to_string (tr ("Gothic")));
  ff.push_back (tl::to_string (tr ("Sans Serif")));
  ff.push_back (tl::to_string (tr ("Stick")));
  ff.push_back (tl::to_string (tr ("Times Italic")));
  ff.push_back (tl::to_string (tr ("Times Thin")));
  ff.push_back (tl::to_string (tr ("Times")));
  return ff;
}

size_t
hershey_count_edges (const std::string &s, unsigned int f)
{
  HersheyFont *fp = fonts [f];
  size_t n = 0;

  for (const char *cp = s.c_str (); *cp; ++cp) {

    unsigned char c = (unsigned char) *cp;
    if (c == '\012' || c == '\015') {
      if (c == '\015' && cp[1] == '\012') {
        ++cp;
      }
    } else if (c < fp->end_char && c >= fp->start_char) {
      n += fp->chars [c - fp->start_char].edge_end - fp->chars [c - fp->start_char].edge_start;
    } else if (invalid_char < fp->end_char && invalid_char >= fp->start_char) {
      n += fp->chars [invalid_char - fp->start_char].edge_end - fp->chars [invalid_char - fp->start_char].edge_start;
    }

  }

  return n;
}

int
hershey_font_width (unsigned int f)
{
  return fonts [f]->width;
}

int
hershey_font_height (unsigned int f)
{
  return fonts [f]->height;
}

db::DBox 
hershey_text_box (const std::string &s, unsigned int f)
{
  HersheyFont *fp = fonts [f];

  int wl = 0;
  int hl = 0;

  int w = 0;
  int h = fp->ymax;

  for (const char *cp = s.c_str (); *cp; ++cp) {

    unsigned char c = (unsigned char) *cp;
    if (c == '\012' || c == '\015') {
      if (c == '\015' && cp[1] == '\012') {
        ++cp;
      }
      if (w > wl) {
        wl = w;
      }
      hl += line_spacing + h - fp->ymin;
      w = 0;
    } else if (c < fp->end_char && c >= fp->start_char) {
      w += fp->chars [c - fp->start_char].width;
    } else if (invalid_char < fp->end_char && invalid_char >= fp->start_char) {
      w += fp->chars [invalid_char - fp->start_char].width;
    }

  }

  if (w > wl) {
    wl = w;
  }
  hl += h;

  return db::DBox (0, 0, wl, hl);
}

void 
hershey_justify (const std::string &s, unsigned int f, db::DBox bx, HAlign halign, VAlign valign, std::vector<db::DPoint> &linestarts)
{
  HersheyFont *fp = fonts [f];

  int hl = 0;
  int w = 0;
  int h = fp->ymax;

  for (const char *cp = s.c_str (); *cp; ++cp) {

    unsigned char c = (unsigned char) *cp;
    if (c == '\012' || c == '\015') {
      if (c == '\015' && cp[1] == '\012') {
        ++cp;
      }
      linestarts.push_back (db::DPoint (w, -hl));
      hl += line_spacing + h - fp->ymin;
      w = 0;
    } else if (c < fp->end_char && c >= fp->start_char) {
      w += fp->chars [c - fp->start_char].width;
    } else if (invalid_char < fp->end_char && invalid_char >= fp->start_char) {
      w += fp->chars [invalid_char - fp->start_char].width;
    }

  }

  linestarts.push_back (db::DPoint (w, -hl));
  hl += h;

  db::DVector delta;

  if (valign == VAlignCenter) {
    delta = db::DVector (0, (bx.height () + hl) / 2 - fp->ymax);
  } else if (valign == VAlignTop) {
    delta = db::DVector (0, bx.height () - fp->ymax);
  } else if (valign == VAlignBottom || valign == NoVAlign) {
    delta = db::DVector (0, hl - fp->ymax);
  }    

  for (std::vector<db::DPoint>::iterator l = linestarts.begin (); l != linestarts.end (); ++l) {
    db::DPoint p (bx.p1 () + delta);
    if (halign == HAlignCenter) {
      p += db::DVector ((bx.width () - l->x ()) / 2, l->y ());
    } else if (halign == HAlignRight) {
      p += db::DVector (bx.width () - l->x (), l->y ());
    } else if (halign == HAlignLeft || halign == NoHAlign) {
      p += db::DVector (0, l->y ());
    }
    *l = p;
  }

}

// ----------------------------------------------------------------------------
//  basic_hershey_edge_iterator implementation

basic_hershey_edge_iterator::basic_hershey_edge_iterator (const std::string &s, unsigned int f, const std::vector<db::DPoint> &line_starts)
  : m_line (0), m_string (s), m_edge (0), m_edge_end (0), m_index (0), m_linestarts (line_starts)
{
  m_fp = fonts [f];
  m_end = (unsigned int) m_string.size ();
  m_new_char = true;

  if (m_linestarts.size () == 0) {
    m_linestarts.push_back (db::DPoint (0.0, 0.0));
  }
  m_pos = m_linestarts [0];
}

bool
basic_hershey_edge_iterator::at_end () const
{
  return m_index >= m_end;
}

db::DEdge
basic_hershey_edge_iterator::get () 
{
  while (m_new_char && !at_end ()) {

    unsigned char c = (unsigned char) (m_index < m_end ? m_string [m_index] : ' ');

    m_pos += m_delta;

    if (c < m_fp->end_char && c >= m_fp->start_char) {

      m_edge = m_fp->chars [c - m_fp->start_char].edge_start;
      m_edge_end = m_fp->chars [c - m_fp->start_char].edge_end;
      m_delta = db::DVector (m_fp->chars [c - m_fp->start_char].width, 0);

    } else if (c != '\012' && c != '\015' 
               && invalid_char < m_fp->end_char 
               && invalid_char >= m_fp->start_char) {

      m_edge = m_fp->chars [invalid_char - m_fp->start_char].edge_start;
      m_edge_end = m_fp->chars [invalid_char - m_fp->start_char].edge_end;
      m_delta = db::DVector (m_fp->chars [invalid_char - m_fp->start_char].width, 0);

    } else {

      m_edge = m_edge_end = 0;
      m_delta = db::DVector ();

    }

    if (m_edge == m_edge_end) {

      if (c == '\012' || c == '\015') {
        if (c == '\015' && m_string.size () > m_index + 1 && m_string [m_index] == '\012') {
          ++m_index;
        }
        ++m_line;
        if (m_line >= m_linestarts.size ()) {
          m_linestarts.push_back (m_linestarts.back () + db::DVector (0.0, -(m_fp->ymax - m_fp->ymin + line_spacing)));
        }
        m_pos = m_linestarts [m_line];
      }

      ++m_index;

    } else {
      m_new_char = false;
    }
  }

  while (m_line >= m_linestarts.size ()) {
    db::DPoint last;
    if (m_linestarts.size () > 0) {
      last = m_linestarts.back ();
      last += db::DVector (0, -(m_fp->ymax - m_fp->ymin + line_spacing));
    } 
    m_linestarts.push_back (last);
  }

  if (!at_end ()) {
    const short *ep = m_fp->edges [m_edge];
    return db::DEdge (m_pos + db::DVector (ep[0], ep[1]), m_pos + db::DVector (ep[2], ep[3]));
  } else {
    return db::DEdge ();
  }
}

void
basic_hershey_edge_iterator::inc ()
{
  if (m_new_char) {
    get ();
  }

  if (! at_end ()) {
    ++m_edge;
    if (m_edge == m_edge_end) {
      ++m_index;
      m_new_char = true;
    }
  }
}


} // namespace db

