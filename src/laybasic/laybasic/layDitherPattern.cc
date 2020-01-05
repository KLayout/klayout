
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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



#include "layDitherPattern.h"
#include "tlAssert.h"

#include <ctype.h>
#include <algorithm>

namespace lay
{

// ---------------------------------------------------------------------
//  The standard dither pattern

static const char *dither_strings [] = {

  // 0: solid
  "solid",
  "*",

  // 1: hollow
  "hollow",
  ".", 

  // 2: dotted
  "dotted",
  "*.\n"
  ".*",

  // 3: coarsly dotted
  "coarsly dotted",
  "*...\n"
  "....\n"
  "..*.\n"
  "....",

  // 4: left-hatched
  "left-hatched",
  "*...\n"
  ".*..\n"
  "..*.\n"
  "...*",

  // 5: lightly left-hatched
  "lightly left-hatched",
  "*.......\n"
  ".*......\n"
  "..*.....\n"
  "...*....\n"
  "....*...\n"
  ".....*..\n"
  "......*.\n"
  ".......*",

  // 6: strongly left-hatched dense
  "strongly left-hatched dense",
  "**..\n"
  ".**.\n"
  "..**\n"
  "*..*",

  // 7: strongly left-hatched sparse
  "strongly left-hatched sparse",
  "**......\n"
  ".**.....\n"
  "..**....\n"
  "...**...\n"
  "....**..\n"
  ".....**.\n"
  "......**\n"
  "*......*",

  // 8: right-hatched
  "right-hatched",
  "*...\n"
  "...*\n"
  "..*.\n"
  ".*..",

  // 9: lightly right-hatched
  "lightly right-hatched",
  "*.......\n"
  ".......*\n"
  "......*.\n"
  ".....*..\n"
  "....*...\n"
  "...*....\n"
  "..*.....\n"
  ".*......",

  // 10: strongly right-hatched dense
  "strongly right-hatched dense",
  "**..\n"
  "*..*\n"
  "..**\n"
  ".**.",

  // 11: strongly right-hatched sparse
  "strongly right-hatched sparse",
  "**......\n"
  "*......*\n"
  "......**\n"
  ".....**.\n"
  "....**..\n"
  "...**...\n"
  "..**....\n"
  ".**.....",

  // 12: cross-hatched
  "cross-hatched",
  "*...\n"
  ".*.*\n"
  "..*.\n"
  ".*.*",

  // 13: lightly cross-hatched
  "lightly cross-hatched",
  "*.......\n"
  ".*.....*\n"
  "..*...*.\n"
  "...*.*..\n"
  "....*...\n"
  "...*.*..\n"
  "..*...*.\n"
  ".*.....*",

  // 14: checkerboard 2px
  "checkerboard 2px",
  "**..\n"
  "**..\n"
  "..**\n"
  "..**",

  // 15: strongly cross-hatched sparse
  "strongly cross-hatched sparse",
  "**......\n"
  "***....*\n"
  "..**..**\n"
  "...****.\n"
  "....**..\n"
  "...****.\n"
  "..**..**\n"
  "***....*",

  // 16: heavy checkerboard
  "heavy checkerboard",
  "****....\n"
  "****....\n"
  "****....\n"
  "****....\n"
  "....****\n"
  "....****\n"
  "....****\n"
  "....****",

  // 17: hollow bubbles
  "hollow bubbles",
  ".*...*..\n"
  "*.*.....\n"
  ".*...*..\n"
  "....*.*.\n"
  ".*...*..\n"
  "*.*.....\n"
  ".*...*..\n"
  "....*.*.",

  // 18: solid bubbles
  "solid bubbles",
  ".*...*..\n"
  "***.....\n"
  ".*...*..\n"
  "....***.\n"
  ".*...*..\n"
  "***.....\n"
  ".*...*..\n"
  "....***.",

  // 19: pyramids
  "pyramids",
  ".*......\n"
  "*.*.....\n"
  "****...*\n"
  "........\n"
  "....*...\n"
  "...*.*..\n"
  "..*****.\n"
  "........",

  // 20: turned pyramids
  "turned pyramids",
  "****...*\n"
  "*.*.....\n"
  ".*......\n"
  "........\n"
  "..*****.\n"
  "...*.*..\n"
  "....*...\n"
  "........",

  // 21: plus
  "plus",
  "..*...*.\n"
  "..*.....\n"
  "*****...\n"
  "..*.....\n"
  "..*...*.\n"
  "......*.\n"
  "*...****\n"
  "......*.",

  // 22: minus
  "minus",
  "........\n"
  "........\n"
  "*****...\n"
  "........\n"
  "........\n"
  "........\n"
  "*...****\n"
  "........",

  // 23: 22.5 degree down
  "22.5 degree down",
  "*......*\n"
  ".**.....\n"
  "...**...\n"
  ".....**.\n"
  "*......*\n"
  ".**.....\n"
  "...**...\n"
  ".....**.",

  // 24: 22.5 degree up
  "22.5 degree up",
  "*......*\n"
  ".....**.\n"
  "...**...\n"
  ".**.....\n"
  "*......*\n"
  ".....**.\n"
  "...**...\n"
  ".**.....",

  // 25: 67.5 degree down
  "67.5 degree down",
  "*...*...\n"
  ".*...*..\n"
  ".*...*..\n"
  "..*...*.\n"
  "..*...*.\n"
  "...*...*\n"
  "...*...*\n"
  "*...*...",

  // 26: 67.5 degree up
  "67.5 degree up",
  "...*...*\n"
  "..*...*.\n"
  "..*...*.\n"
  ".*...*..\n"
  ".*...*..\n"
  "*...*...\n"
  "*...*...\n"
  "...*...*",

  // 27: 22.5 cross hatched
  "22.5 degree cross hatched",
  "*......*\n"
  ".**..**.\n"
  "...**...\n"
  ".**..**.\n"
  "*......*\n"
  ".**..**.\n"
  "...**...\n"
  ".**..**.",

  // 28: zig zag
  "zig zag",
  "..*...*.\n"
  ".*.*.*.*\n"
  "*...*...\n"
  "........\n"
  "..*...*.\n"
  ".*.*.*.*\n"
  "*...*...\n"
  "........",

  // 29: sine 
  "sine",
  "..***...\n"
  ".*...*..\n"
  "*.....**\n"
  "........\n"
  "..***...\n"
  ".*...*..\n"
  "*.....**\n"
  "........",

  // 30: special pattern for light heavy dithering
  "heavy unordered",
  "****.*.*\n"
  "**.****.\n"
  "*.**.***\n"
  "*****.*.\n"
  ".**.****\n"
  "**.***.*\n"
  ".****.**\n"
  "*.*.****",

  // 31: special pattern for light frame dithering
  "light unordered",
  "....*.*.\n"
  "..*....*\n"
  ".*..*...\n"
  ".....*.*\n"
  "*..*....\n"
  "..*...*.\n"
  "*....*..\n"
  ".*.*....",

  // 32: vertical dense
  "vertical dense",
  "*.\n"
  "*.\n",

  // 33: vertical 
  "vertical",
  ".*..\n"
  ".*..\n"
  ".*..\n"
  ".*..\n",

  // 34: vertical thick
  "vertical thick",
  ".**.\n"
  ".**.\n"
  ".**.\n"
  ".**.\n",

  // 35: vertical sparse
  "vertical sparse",
  "...*....\n"
  "...*....\n"
  "...*....\n"
  "...*....\n",

  // 36: vertical sparse, thick
  "vertical sparse, thick",
  "...**...\n"
  "...**...\n"
  "...**...\n"
  "...**...\n",

  // 37: horizontal dense
  "horizontal dense",
  "**\n"
  "..\n",

  // 38: horizontal 
  "horizontal",
  "....\n"
  "****\n"
  "....\n"
  "....\n",

  // 39: horizontal thick
  "horizontal thick",
  "....\n"
  "****\n"
  "****\n"
  "....\n",

  // 40: horizontal 
  "horizontal sparse",
  "........\n"
  "........\n"
  "........\n"
  "********\n"
  "........\n"
  "........\n"
  "........\n"
  "........\n",

  // 41: horizontal 
  "horizontal sparse, thick",
  "........\n"
  "........\n"
  "........\n"
  "********\n"
  "********\n"
  "........\n"
  "........\n"
  "........\n",

  // 42: grid dense
  "grid dense",
  "**\n"
  "*.\n",

  // 43: grid 
  "grid",
  ".*..\n"
  "****\n"
  ".*..\n"
  ".*..\n",

  // 44: grid thick
  "grid thick",
  ".**.\n"
  "****\n"
  "****\n"
  ".**.\n",

  // 45: grid sparse
  "grid sparse",
  "...*....\n"
  "...*....\n"
  "...*....\n"
  "********\n"
  "...*....\n"
  "...*....\n"
  "...*....\n"
  "...*....\n",

  // 46: grid sparse, thick
  "grid sparse, thick",
  "...**...\n"
  "...**...\n"
  "...**...\n"
  "********\n"
  "********\n"
  "...**...\n"
  "...**...\n"
  "...**...\n",
};


// ---------------------------------------------------------------------
//  DitherPatternInfo implementation

DitherPatternInfo::DitherPatternInfo ()
  : m_width (1), m_height (1), m_order_index (0)
{
  m_pattern_stride = 1;
  for (size_t i = 0; i < sizeof (m_pattern) / sizeof (m_pattern [0]); ++i) {
    m_pattern [i] = &m_buffer [0];
  }
  memset (m_buffer, 0xff, sizeof (m_buffer));
}
  
DitherPatternInfo::DitherPatternInfo (const DitherPatternInfo &d)
  : m_width (d.m_width), m_height (d.m_height), m_order_index (d.m_order_index), m_name (d.m_name)
{
  operator= (d);
}
  
DitherPatternInfo &
DitherPatternInfo::operator= (const DitherPatternInfo &d)
{
  if (&d != this) {

    m_order_index = d.m_order_index;
    m_name = d.m_name;
    m_width = d.m_width;
    m_pattern_stride = d.m_pattern_stride;
    m_height = d.m_height;

    for (size_t i = 0; i < sizeof (m_pattern) / sizeof (m_pattern [0]); ++i) {
      m_pattern [i] = &m_buffer [0] + (d.m_pattern [i] - d.m_buffer);
    }
    memcpy (m_buffer, d.m_buffer, sizeof (m_buffer));
  }

  return *this;
}

bool 
DitherPatternInfo::same_bitmap (const DitherPatternInfo &d) const
{
  if (m_width != d.m_width || m_height != d.m_height) {
    return false;
  }

  tl_assert (m_pattern_stride == d.m_pattern_stride);

  unsigned int n = m_pattern_stride * (sizeof (m_pattern) / sizeof (m_pattern [0]));
  for (unsigned int i = 0; i < n; ++i) {
    if (m_buffer [i] != d.m_buffer [i]) {
      return false;
    }
  }
  return true;
}

bool 
DitherPatternInfo::less_bitmap (const DitherPatternInfo &d) const
{
  if (m_width != d.m_width) {
    return m_width < d.m_width;
  }
  if (m_height != d.m_height) {
    return m_height < d.m_height;
  }

  tl_assert (m_pattern_stride == d.m_pattern_stride);

  unsigned int n = m_pattern_stride * (sizeof (m_pattern) / sizeof (m_pattern [0]));
  for (unsigned int i = 0; i < n; ++i) {
    if (m_buffer [i] < d.m_buffer [i]) {
      return true;
    } else if (m_buffer [i] > d.m_buffer [i]) {
      return false;
    }
  }
  return false;
}

bool 
DitherPatternInfo::operator== (const DitherPatternInfo &d) const
{
  return same_bitmap (d) && m_name == d.m_name && m_order_index == d.m_order_index;
}

bool 
DitherPatternInfo::operator< (const DitherPatternInfo &d) const
{
  if (! same_bitmap (d)) {
    return less_bitmap (d);
  } 
  if (m_name != d.m_name) {
    return m_name < d.m_name;
  }
  return m_order_index < d.m_order_index;
}

// TODO including a scaling algorithm in this formula, or give more resolution to the dither
QBitmap
DitherPatternInfo::get_bitmap (int width, int height) const
{
  if (height < 0) {
    height = 36;
  }
  if (width < 0) {
    width = 34;
  }

  const uint32_t * const *p = pattern ();
  unsigned int stride = (width + 7) / 8;

  unsigned char *data = new unsigned char[stride * height];
  memset (data, 0x00, size_t (stride * height));

  for (unsigned int i = 1; i < (unsigned int)(height - 1); ++i) {
    for (unsigned int j = 0; j < stride; ++j) {
      data [i * stride + j] = 0xff;
    }
  }

  for (unsigned int i = 0; i < (unsigned int)(height - 4); ++i) {
    uint32_t w = *(p [(height - 5 - i) % m_height]);
    for (unsigned int j = 0; j < (unsigned int)(width - 2); ++j) {
      if (! (w & (1 << (j % m_width)))) {
        data [stride * (i + 2) + (j + 1) / 8] &= ~(1 << ((j + 1) % 8));
      }
    }
  }

  QBitmap bitmap (QBitmap::fromData (QSize (width, height), data, QImage::Format_MonoLSB));
  delete[] data;

  return bitmap;
}

void 
DitherPatternInfo::set_pattern (const uint32_t *pt, unsigned int w, unsigned int h) 
{
  //  pattern size must be 1x1 at least
  if (w == 0 || h == 0) {
    uint32_t zero = 0;
    set_pattern (&zero, 1, 1);
    return;
  }

  memset (m_buffer, 0, sizeof (m_buffer));

  if (w >= 32) {
    w = 32;
  }
  m_width = w;

  if (h >= 32) {
    h = 32;
  } 
  m_height = h;

  //  compute pattern stride
  m_pattern_stride = 1;
  while ((m_pattern_stride * 32) % w != 0) {
    ++m_pattern_stride;
  }

  uint32_t *pp = &m_buffer[0];

  for (unsigned int j = 0; j < sizeof (m_pattern) / sizeof (m_pattern [0]); ++j) {

    m_pattern [j] = pp;

    uint32_t din = pt[j % h];
    uint32_t dd = din;

    unsigned int b = 0;
    for (unsigned int i = 0; i < m_pattern_stride; ++i) {
      uint32_t dout = 0;
      for (uint32_t m = 1; m != 0; m <<= 1) {
        if ((dd & 1) != 0) {
          dout |= m;
        }
        dd >>= 1;
        if (++b == w) {
          dd = din;
          b = 0;
        }
      }
      *pp++ = dout;
    }

  }
}

std::string
DitherPatternInfo::to_string () const
{
  std::string res;

  for (unsigned int i = 0; i < m_height; ++i) {
    for (unsigned int j = 0; j < m_width; ++j) {
      if ((*(m_pattern [m_height - 1 - i]) & (1 << j)) != 0) {
        res += "*";
      } else {
        res += ".";
      }
    }
    res += "\n";
  }

  return res;
}

std::vector <std::string>
DitherPatternInfo::to_strings () const
{
  std::vector <std::string> res;
  
  for (unsigned int i = 0; i < m_height; ++i) {
    std::string r;
    for (unsigned int j = 0; j < m_width; ++j) {
      if ((*(m_pattern [m_height - 1 - i]) & (1 << j)) != 0) {
        r += "*";
      } else {
        r += ".";
      }
    }
    res.push_back (r);
  }

  return res;
}

static const char *uint_from_string (const char *s, uint32_t &w, unsigned int &width)
{
  while (*s && isspace (*s)) {
    ++s;
  }

  w = 0;

  unsigned int b = 1;
  unsigned int n = 0;

  while (*s && ! isspace (*s)) {
    if (*s++ == '*') {
      w |= b;
    }
    ++n;
    b <<= 1;
  }

  width = std::max (width, n);
  return s;
}

void
DitherPatternInfo::from_strings (const std::vector<std::string> &strv) 
{
  unsigned int h = std::min ((unsigned int) 32, (unsigned int) strv.size ());
  unsigned int w = 0;

  uint32_t data [32];
  for (unsigned int l = 0; l < 32; ++l) {
    data[l] = 0;
  }

  for (size_t i = 0; i < h; ++i) {
    uint_from_string (strv [h - 1 - i].c_str (), data [i], w);
  }

  set_pattern (data, w, h);
}

void
DitherPatternInfo::from_string (const std::string &cstr)
{
  unsigned int h = 0;
  unsigned int w = 0;

  uint32_t data [32];
  for (unsigned int l = 0; l < 32; ++l) {
    data[l] = 0;
  }

  const char *s = cstr.c_str ();

  while (*s && h < 32) {
    while (*s && isspace (*s)) {
      ++s;
    }
    if (*s) {
      s = uint_from_string (s, data [h], w);
      ++h;
    }
  }

  std::reverse (&data[0], &data[h]);

  set_pattern (data, w, h);
}

// ---------------------------------------------------------------------
//  DitherPattern implementation

struct ReplaceDitherPatternOp
  : public db::Op
{
  ReplaceDitherPatternOp (unsigned int i, const DitherPatternInfo &o, const DitherPatternInfo &n) 
    : db::Op (), index (i), m_old (o), m_new (n) 
  { }

  unsigned int index;
  DitherPatternInfo m_old, m_new;
};

DitherPattern::DitherPattern ()
  : QObject (), db::Object (0)
{
  for (unsigned int d = 0; d < sizeof (dither_strings) / sizeof (dither_strings [0]); d += 2) {
    m_pattern.push_back (DitherPatternInfo ());
    m_pattern.back ().set_name (dither_strings [d]);
    m_pattern.back ().from_string (dither_strings [d + 1]);
  }
}

DitherPattern::~DitherPattern ()
{
  //  .. nothing yet ..
}

DitherPattern::DitherPattern (const DitherPattern &p)
  : QObject (), db::Object (0)
{
  m_pattern = p.m_pattern;
}

DitherPattern & 
DitherPattern::operator= (const DitherPattern &p)
{
  if (this != &p) {
    unsigned int i;
    for (i = 0; i < p.count (); ++i) {
      replace_pattern (i, p.begin () [i]);
    }
    for ( ; i < count (); ++i) {
      replace_pattern (i, DitherPatternInfo ());
    }
  }
  return *this;
}

QBitmap
DitherPattern::get_bitmap (unsigned int i, int width, int height) const
{
  if (i < count ()) {
    return m_pattern [i].get_bitmap (width, height);
  } else {
    return m_pattern [1].get_bitmap (width, height);
  }
}

const DitherPatternInfo &
DitherPattern::pattern (unsigned int i) const
{
  if (i < count ()) {
    return m_pattern [i];
  } else {
    static DitherPatternInfo empty;
    return empty;
  }
}

void 
DitherPattern::replace_pattern (unsigned int i, const DitherPatternInfo &p)
{
  bool chg = false;

  while (i >= count ()) {
    m_pattern.push_back (DitherPatternInfo ());
    chg = true;
  }

  if (m_pattern [i] != p) {
    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new ReplaceDitherPatternOp (i, m_pattern [i], p));
    }
    m_pattern [i] = p;
    chg = true;
  }

  //  if something has changed emit the signal
  if (chg) {
    emit changed ();
  }
}

unsigned int 
DitherPattern::add_pattern (const DitherPatternInfo &p)
{
  unsigned int oi = 0;
  lay::DitherPattern::iterator iempty = end ();
  for (lay::DitherPattern::iterator i = begin_custom (); i != end (); ++i) {
    if (i->order_index () == 0) {
      iempty = i;
    } else if (i->order_index () > oi) {
      oi = i->order_index ();
    } 
  }

  unsigned int index = std::distance (begin (), iempty);

  //  NOTE: doing it this way will enable undo/redo because replace_pattern
  //  is undo/redo enabled.
  DitherPatternInfo pdup = p;
  pdup.set_order_index (oi + 1);
  replace_pattern (index, pdup);

  return index;
}

namespace {
  struct display_order
  {
    bool operator () (lay::DitherPattern::iterator a, lay::DitherPattern::iterator b)
    {
      return a->order_index () < b->order_index ();
    }
  };
}

void 
DitherPattern::renumber ()
{
  //  renumber the order indices
  std::vector <lay::DitherPattern::iterator> iters; 
  for (lay::DitherPattern::iterator i = begin_custom (); i != end (); ++i) {
    iters.push_back (i);
  }
  std::sort (iters.begin (), iters.end (), display_order ());

  unsigned int oi = 1;
  for (std::vector <lay::DitherPattern::iterator>::const_iterator i = iters.begin (); i != iters.end (); ++i) {
    if ((*i)->order_index () > 0) {
      lay::DitherPatternInfo p (**i);
      p.set_order_index (oi++);
      replace_pattern (std::distance (begin (), *i), p);
    }
  }
}

DitherPattern::iterator 
DitherPattern::begin_custom () const 
{
  return m_pattern.begin () + sizeof (dither_strings) / sizeof (dither_strings [0]) / 2;
}

const DitherPattern &
DitherPattern::default_pattern () 
{
  static DitherPattern empty;
  return empty;
}

void 
DitherPattern::undo (db::Op *op)
{
  const ReplaceDitherPatternOp *rop = dynamic_cast <const ReplaceDitherPatternOp *> (op);
  if (rop) {
    replace_pattern (rop->index, rop->m_old);
  }
}

void 
DitherPattern::redo (db::Op *op)
{
  const ReplaceDitherPatternOp *rop = dynamic_cast <const ReplaceDitherPatternOp *> (op);
  if (rop) {
    replace_pattern (rop->index, rop->m_new);
  }
}

struct pattern_less_f
{
  bool operator() (const DitherPatternInfo &a, const DitherPatternInfo &b) const
  {
    return a.less_bitmap (b);
  }
};

void 
DitherPattern::merge (const DitherPattern &other, std::map<unsigned int, unsigned int> &index_map)
{
  //  insert the standard pattern into the map (for completeness)
  for (iterator c = begin (); c != begin_custom (); ++c) {
    index_map.insert (std::make_pair ((unsigned int) std::distance (begin (), c), (unsigned int) std::distance (begin (), c)));
  }

  //  build an index of present pattern
  std::map <DitherPatternInfo, unsigned int, pattern_less_f> patterns;
  for (iterator c = begin_custom (); c != end (); ++c) {
    patterns.insert (std::make_pair (*c, (unsigned int) std::distance (begin (), c)));
  }

  //  map the pattern of other into *this, possibly creating new ones
  for (iterator c = other.begin_custom (); c != other.end (); ++c) {
    std::map <DitherPatternInfo, unsigned int, pattern_less_f>::const_iterator p = patterns.find (*c);
    unsigned int new_index;
    if (p == patterns.end ()) {
      new_index = add_pattern (*c);
      patterns.insert (std::make_pair (*c, new_index));
    } else {
      new_index = p->second;
    }
    index_map.insert (std::make_pair ((unsigned int) std::distance (other.begin (), c), new_index));
  }
}

}

