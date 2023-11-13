
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



#include "layLineStyles.h"
#include "tlAssert.h"
#include "tlThreads.h"

#include <ctype.h>
#include <algorithm>
#include <string.h>

namespace lay
{

// ---------------------------------------------------------------------
//  The standard dither pattern

static const char *style_strings [] = {

  // 0: solid
  "solid",
  "",

  // 1: dotted
  "dotted",
  "*.",

  // 2: dashed
  "dashed",
  "**..**",

  // 3: dash-dotted
  "dash-dotted",
  "***..**..***",

  // 4: short dashed
  "short dashed",
  "*..*",

  // 5: short dash-dotted
  "short dash-dotted",
  "**.*.*",

  // 6: long dashed
  "long dashed",
  "*****..*****",

  // 7: dash-double-dotted
  "dash-double-dotted",
  "***..*.*..**"
};


// ---------------------------------------------------------------------
//  LineStyleInfo implementation

static tl::Mutex s_mutex;

LineStyleInfo::LineStyleInfo ()
  : m_width (0), m_order_index (0)
{
  m_pattern_stride = 1;
  memset (m_pattern, 0xff, sizeof (m_pattern));
}
  
LineStyleInfo::LineStyleInfo (const LineStyleInfo &d)
  : m_width (d.m_width), m_order_index (d.m_order_index), m_name (d.m_name)
{
  operator= (d);
}
  
LineStyleInfo &
LineStyleInfo::operator= (const LineStyleInfo &d)
{
  if (&d != this) {
    tl::MutexLocker locker (& s_mutex);
    assign_no_lock (d);
  }
  return *this;
}

void
LineStyleInfo::assign_no_lock (const LineStyleInfo &d)
{
  m_scaled_pattern.reset (0);

  m_order_index = d.m_order_index;
  m_name = d.m_name;
  m_width = d.m_width;
  m_pattern_stride = d.m_pattern_stride;

  memcpy (m_pattern, d.m_pattern, sizeof (m_pattern));
}

bool
LineStyleInfo::same_bits (const LineStyleInfo &d) const
{
  if (m_width != d.m_width) {
    return false;
  }

  tl_assert (m_pattern_stride == d.m_pattern_stride);

  for (unsigned int i = 0; i < m_pattern_stride; ++i) {
    if (m_pattern [i] != d.m_pattern [i]) {
      return false;
    }
  }
  return true;
}

bool 
LineStyleInfo::less_bits (const LineStyleInfo &d) const
{
  if (m_width != d.m_width) {
    return m_width < d.m_width;
  }

  tl_assert (m_pattern_stride == d.m_pattern_stride);

  for (unsigned int i = 0; i < m_pattern_stride; ++i) {
    if (m_pattern [i] < d.m_pattern [i]) {
      return true;
    } else if (m_pattern [i] > d.m_pattern [i]) {
      return false;
    }
  }
  return false;
}

bool 
LineStyleInfo::operator== (const LineStyleInfo &d) const
{
  return same_bits (d) && m_name == d.m_name && m_order_index == d.m_order_index;
}

bool 
LineStyleInfo::operator< (const LineStyleInfo &d) const
{
  if (! same_bits (d)) {
    return less_bits (d);
  } 
  if (m_name != d.m_name) {
    return m_name < d.m_name;
  }
  return m_order_index < d.m_order_index;
}

bool
LineStyleInfo::is_bit_set (unsigned int n) const
{
  return (pattern () [(n / 32) % pattern_stride ()] & (1 << (n % 32))) != 0;
}

#if defined(HAVE_QT)
QBitmap
LineStyleInfo::get_bitmap (int w, int h, int fw) const
{
  unsigned int height = h < 0 ? 5 : (unsigned int) h;
  unsigned int width = w < 0 ? 34 : (unsigned int) w;
  unsigned int frame_width = fw <= 0 ? 1 : (unsigned int) fw;
  unsigned int stride = (width + 7) / 8;

  unsigned char *data = new unsigned char[stride * height];
  memset (data, 0x00, size_t (stride * height));

  unsigned int hv = height - 2 * frame_width;

  for (unsigned int i = 0; i < hv; ++i) {
    if (is_bit_set (i / frame_width + 1)) {
      unsigned int y = height - 1 - frame_width - i;
      for (unsigned int x = 0; x < frame_width; ++x) {
        data [y * stride + x / 8] |= (1 << (x % 8));
      }
      for (unsigned int x = width - frame_width; x < width; ++x) {
        data [y * stride + x / 8] |= (1 << (x % 8));
      }
    }
  }

  for (unsigned int i = 0; i < width; ++i) {
    if (is_bit_set (i / frame_width)) {
      for (unsigned int y = 0; y < frame_width; ++y) {
        data [y * stride + i / 8] |= (1 << (i % 8));
      }
      for (unsigned int y = height - frame_width; y < height; ++y) {
        data [y * stride + i / 8] |= (1 << (i % 8));
      }
    }
  }

  QBitmap bitmap (QBitmap::fromData (QSize (width, height), data, QImage::Format_MonoLSB));
  delete[] data;

  return bitmap;
}
#endif

void
LineStyleInfo::set_pattern (uint32_t pt, unsigned int w) 
{
  tl::MutexLocker locker (& s_mutex);
  m_scaled_pattern.reset (0);

  memset (m_pattern, 0, sizeof (m_pattern));

  if (w >= 32) {
    w = 32;
  }
  m_width = w;

  //  w == 0 means solid pattern
  if (w == 0) {
    m_pattern[0] = 0xffffffff;
    m_pattern_stride = 1;
    return;
  }


  //  compute pattern stride
  m_pattern_stride = 1;
  while ((m_pattern_stride * 32) % w != 0) {
    ++m_pattern_stride;
  }

  uint32_t *pp = m_pattern;
  uint32_t dd = pt;

  unsigned int b = 0;
  for (unsigned int i = 0; i < m_pattern_stride; ++i) {
    uint32_t dout = 0;
    for (uint32_t m = 1; m != 0; m <<= 1) {
      if ((dd & 1) != 0) {
        dout |= m;
      }
      dd >>= 1;
      if (++b == w) {
        dd = pt;
        b = 0;
      }
    }
    *pp++ = dout;
  }
}

const LineStyleInfo &
LineStyleInfo::scaled (unsigned int n) const
{
  if (n <= 1) {
    return *this;
  }

  tl::MutexLocker locker (& s_mutex);

  if (! m_scaled_pattern.get ()) {
    m_scaled_pattern.reset (new std::map<unsigned int, LineStyleInfo> ());
  }

  auto i = m_scaled_pattern->find (n);
  if (i != m_scaled_pattern->end ()) {
    return i->second;
  }

  LineStyleInfo &sp = (*m_scaled_pattern) [n];
  sp.assign_no_lock (*this);
  sp.scale_pattern (n);
  return sp;
}

void
LineStyleInfo::scale_pattern (unsigned int n)
{
  if (m_width == 0 || n <= 1) {
    return;
  }

  unsigned int w = m_width * n;

  //  compute new pattern stride (we take care that it does not get too big)
  unsigned int max_words = sizeof (m_pattern) / sizeof (m_pattern [0]);
  m_pattern_stride = 1;
  while ((m_pattern_stride * 32) % w != 0 && m_pattern_stride < max_words) {
    ++m_pattern_stride;
  }

  uint32_t *pp = m_pattern;
  uint32_t pt = m_pattern [0];
  uint32_t ptr = pt >> 1; // right-rotated by 1
  if (pt & 1) {
    ptr |= (1 << (m_width - 1));
  }
  uint32_t dd = pt;
  uint32_t ddr = ptr;

  memset (m_pattern, 0, sizeof (m_pattern));

  unsigned int b = 0;
  unsigned int bi = 0;
  for (unsigned int i = 0; i < m_pattern_stride; ++i) {
    uint32_t dout = 0;
    for (uint32_t m = 1; m != 0; m <<= 1) {
      //  NOTE: we do not fully expand "1" fields with a following "0" as pixel expansion
      //  will take care of this.
      if ((dd & 1) != 0 && ((ddr & 1) != 0 || bi == 0)) {
        dout |= m;
      }
      if (++bi == n) {
        bi = 0;
        dd >>= 1;
        ddr >>= 1;
        if (++b == m_width) {
          dd = pt;
          ddr = ptr;
          b = 0;
        }
      }
    }
    *pp++ = dout;
  }

  m_width = w;
}



std::string
LineStyleInfo::to_string () const
{
  std::string res;

  for (unsigned int j = 0; j < m_width; ++j) {
    if ((m_pattern [0] & (1 << j)) != 0) {
      res += "*";
    } else {
      res += ".";
    }
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
LineStyleInfo::from_string (const std::string &cstr)
{
  unsigned int w = 0;
  uint32_t data = 0;

  const char *s = cstr.c_str ();
  uint_from_string (s, data, w);

  set_pattern (data, w);
}

// ---------------------------------------------------------------------
//  LineStyles implementation

struct ReplaceLineStyleOp
  : public db::Op
{
  ReplaceLineStyleOp (unsigned int i, const LineStyleInfo &o, const LineStyleInfo &n) 
    : db::Op (), index (i), m_old (o), m_new (n) 
  { }

  unsigned int index;
  LineStyleInfo m_old, m_new;
};

LineStyles::LineStyles () :
    db::Object (0)
{
  for (unsigned int d = 0; d < sizeof (style_strings) / sizeof (style_strings [0]); d += 2) {
    m_styles.push_back (LineStyleInfo ());
    m_styles.back ().set_name (style_strings [d]);
    m_styles.back ().from_string (style_strings [d + 1]);
  }
}

LineStyles::LineStyles (const LineStyles &p) :
  db::Object (0)
{
  m_styles = p.m_styles;
}

LineStyles::~LineStyles ()
{
  //  .. nothing yet ..
}

LineStyles &
LineStyles::operator= (const LineStyles &p)
{
  if (this != &p) {
    unsigned int i;
    for (i = 0; i < p.count (); ++i) {
      replace_style (i, p.begin () [i]);
    }
    for ( ; i < count (); ++i) {
      replace_style (i, LineStyleInfo ());
    }
  }
  return *this;
}

const LineStyleInfo &
LineStyles::style (unsigned int i) const
{
  if (i < count ()) {
    return m_styles [i];
  } else {
    static LineStyleInfo empty;
    return empty;
  }
}

void 
LineStyles::replace_style (unsigned int i, const LineStyleInfo &p)
{
  while (i >= count ()) {
    m_styles.push_back (LineStyleInfo ());
  }

  if (m_styles [i] != p) {
    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new ReplaceLineStyleOp (i, m_styles [i], p));
    }
    m_styles [i] = p;
  }
}

unsigned int 
LineStyles::add_style (const LineStyleInfo &p)
{
  unsigned int oi = 0;
  lay::LineStyles::iterator iempty = end ();
  for (lay::LineStyles::iterator i = begin_custom (); i != end (); ++i) {
    if (i->order_index () == 0) {
      iempty = i;
    } else if (i->order_index () > oi) {
      oi = i->order_index ();
    } 
  }

  unsigned int index = std::distance (begin (), iempty);

  //  NOTE: doing it this way will enable undo/redo because replace_pattern
  //  is undo/redo enabled.
  LineStyleInfo pdup = p;
  pdup.set_order_index (oi + 1);
  replace_style (index, pdup);

  return index;
}

namespace {
  struct display_order
  {
    bool operator () (lay::LineStyles::iterator a, lay::LineStyles::iterator b)
    {
      return a->order_index () < b->order_index ();
    }
  };
}

void 
LineStyles::renumber ()
{
  //  renumber the order indices
  std::vector <lay::LineStyles::iterator> iters;
  for (lay::LineStyles::iterator i = begin_custom (); i != end (); ++i) {
    iters.push_back (i);
  }
  std::sort (iters.begin (), iters.end (), display_order ());

  unsigned int oi = 1;
  for (std::vector <lay::LineStyles::iterator>::const_iterator i = iters.begin (); i != iters.end (); ++i) {
    if ((*i)->order_index () > 0) {
      lay::LineStyleInfo p (**i);
      p.set_order_index (oi++);
      replace_style (std::distance (begin (), *i), p);
    }
  }
}

LineStyles::iterator
LineStyles::begin_custom () const
{
  return m_styles.begin () + sizeof (style_strings) / sizeof (style_strings [0]) / 2;
}

const LineStyles &
LineStyles::default_style ()
{
  static LineStyles empty;
  return empty;
}

void 
LineStyles::undo (db::Op *op)
{
  const ReplaceLineStyleOp *rop = dynamic_cast <const ReplaceLineStyleOp *> (op);
  if (rop) {
    replace_style (rop->index, rop->m_old);
  }
}

void 
LineStyles::redo (db::Op *op)
{
  const ReplaceLineStyleOp *rop = dynamic_cast <const ReplaceLineStyleOp *> (op);
  if (rop) {
    replace_style (rop->index, rop->m_new);
  }
}

struct style_less_f
{
  bool operator() (const LineStyleInfo &a, const LineStyleInfo &b) const
  {
    return a.less_bits (b);
  }
};

void 
LineStyles::merge (const LineStyles &other, std::map<unsigned int, unsigned int> &index_map)
{
  //  insert the standard pattern into the map (for completeness)
  for (iterator c = begin (); c != begin_custom (); ++c) {
    index_map.insert (std::make_pair ((unsigned int) std::distance (begin (), c), (unsigned int) std::distance (begin (), c)));
  }

  //  build an index of present pattern
  std::map <LineStyleInfo, unsigned int, style_less_f> styles;
  for (iterator c = begin_custom (); c != end (); ++c) {
    styles.insert (std::make_pair (*c, (unsigned int) std::distance (begin (), c)));
  }

  //  map the pattern of other into *this, possibly creating new ones
  for (iterator c = other.begin_custom (); c != other.end (); ++c) {
    std::map <LineStyleInfo, unsigned int, style_less_f>::const_iterator p = styles.find (*c);
    unsigned int new_index;
    if (p == styles.end ()) {
      new_index = add_style (*c);
      styles.insert (std::make_pair (*c, new_index));
    } else {
      new_index = p->second;
    }
    index_map.insert (std::make_pair ((unsigned int) std::distance (other.begin (), c), new_index));
  }
}

}

