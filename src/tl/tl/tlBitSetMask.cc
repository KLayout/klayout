
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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


#include "tlCommon.h"

#include "tlBitSetMask.h"
#include "tlBitSet.h"

namespace tl
{

static inline unsigned int nwords (BitSetMask::size_type size)
{
  return (size + (sizeof (BitSetMask::data_type) * 8 - 1)) / (sizeof (BitSetMask::data_type) * 8);
}

static inline unsigned int word (BitSetMask::size_type index)
{
  return index / (sizeof (BitSetMask::data_type) * 8);
}

static inline unsigned int bit (BitSetMask::size_type index)
{
  //  first bit is the highest bit, so that comparing the uint's is good enough
  //  for lexical order.
  return 31 - (index % (sizeof (BitSetMask::data_type) * 8));
}

BitSetMask::BitSetMask ()
  : mp_data0 (0), mp_data1 (0), m_size (0)
{
  //  .. nothing yet ..
}

BitSetMask::BitSetMask (const std::string &s)
  : mp_data0 (0), mp_data1 (0), m_size (0)
{
  index_type bit = 0;
  for (const char *cp = s.c_str (); *cp; ++cp, ++bit) {
    mask_type m = Any;
    if (*cp == '0') {
      m = False;
    } else if (*cp == '1') {
      m = True;
    } else if (*cp == '-') {
      m = Never;
    }
    set (bit, m);
  }
}

BitSetMask::BitSetMask (const BitSetMask &other)
  : mp_data0 (0), mp_data1 (0), m_size (0)
{
  operator= (other);
}

BitSetMask::BitSetMask (BitSetMask &&other)
  : mp_data0 (0), mp_data1 (0), m_size (0)
{
  operator= (std::move (other));
}

BitSetMask::~BitSetMask ()
{
  clear ();
}

std::string
BitSetMask::to_string () const
{
  std::string r;
  r.reserve (m_size);

  for (index_type i = 0; i < m_size; ++i) {
    switch (operator[] (i)) {
    case False:
      r += '0';
      break;
    case True:
      r += '1';
      break;
    case Never:
      r += '-';
      break;
    case Any:
    default:
      r += 'X';
      break;
    }
  }

  return r;
}

BitSetMask &
BitSetMask::operator= (const BitSetMask &other)
{
  if (&other != this) {

    clear ();

    //  reallocate
    m_size = other.m_size;
    unsigned int words = nwords (m_size);
    mp_data0 = new data_type[words];
    mp_data1 = new data_type[words];
    data_type *t0 = mp_data0;
    data_type *s0 = other.mp_data0;
    data_type *t1 = mp_data1;
    data_type *s1 = other.mp_data1;
    for (unsigned int i = 0; i < words; ++i) {
      *t0++ = *s0++;
      *t1++ = *s1++;
    }

  }
  return *this;
}

BitSetMask &
BitSetMask::operator= (BitSetMask &&other)
{
  if (&other != this) {
    swap (other);
  }
  return *this;
}

void
BitSetMask::clear ()
{
  if (mp_data0) {
    delete [] mp_data0;
  }
  mp_data0 = 0;
  if (mp_data1) {
    delete [] mp_data1;
  }
  mp_data1 = 0;
  m_size = 0;
}

void
BitSetMask::resize (size_type size)
{
  if (size > m_size) {

    unsigned int words = nwords (m_size);
    unsigned int new_words = nwords (size);

    if (new_words > words) {

      //  reallocate
      data_type *new_data0 = new data_type[new_words];
      data_type *new_data1 = new data_type[new_words];
      data_type *t0 = new_data0;
      data_type *s0 = mp_data0;
      data_type *t1 = new_data1;
      data_type *s1 = mp_data1;
      unsigned int i;
      for (i = 0; i < words; ++i) {
        *t0++ = *s0++;
        *t1++ = *s1++;
      }
      for (; i < new_words; ++i) {
        //  corresponds to "Any"
        *t0++ = 0;
        *t1++ = 0;
      }
      delete mp_data0;
      mp_data0 = new_data0;
      delete mp_data1;
      mp_data1 = new_data1;
      m_size = size;

    }

  }
}

bool
BitSetMask::operator== (const BitSetMask &other) const
{
  unsigned int words = nwords (m_size);
  unsigned int other_words = nwords (other.m_size);

  const data_type *p0 = mp_data0;
  const data_type *p1 = mp_data1;
  const data_type *op0 = other.mp_data0;
  const data_type *op1 = other.mp_data1;
  unsigned int i;
  for (i = 0; i < words && i < other_words; ++i) {
    if (*p0++ != *op0++) {
      return false;
    }
    if (*p1++ != *op1++) {
      return false;
    }
  }
  for (; i < words; ++i) {
    if (*p0++ != 0) {
      return false;
    }
    if (*p1++ != 0) {
      return false;
    }
  }
  for (; i < other_words; ++i) {
    if (0 != *op0++) {
      return false;
    }
    if (0 != *op1++) {
      return false;
    }
  }
  return true;
}

/**
 *  @brief Gets the most significant bit of a bit set
 *
 *  For example b:00101101 will give b:00100000.
 */
static inline BitSetMask::data_type msb_only (BitSetMask::data_type value)
{
  const unsigned int smax = sizeof (BitSetMask::data_type) * 8;

  BitSetMask::data_type m = value;
  for (unsigned int s = 1; s < smax; s *= 2) {
    m |= (m >> s);
  }
  return value & ~(m >> 1);
}

bool
BitSetMask::operator< (const BitSetMask &other) const
{
  unsigned int words = nwords (m_size);
  unsigned int other_words = nwords (other.m_size);

  const data_type *p0 = mp_data0;
  const data_type *p1 = mp_data1;
  const data_type *op0 = other.mp_data0;
  const data_type *op1 = other.mp_data1;

  unsigned int i;
  for (i = 0; i < words && i < other_words; ++i, ++p0, ++p1, ++op0, ++op1) {
    data_type diff = (*p0 ^ *op0) | (*p1 ^ *op1);
    if (diff) {
      //  compare the most significant position of the differences by value
      data_type mb = msb_only (diff);
      unsigned int m = ((*p0 & mb) != 0 ? 1 : 0) + ((*p1 & mb) != 0 ? 2 : 0);
      unsigned int om = ((*op0 & mb) != 0 ? 1 : 0) + ((*op1 & mb) != 0 ? 2 : 0);
      return m < om;
    }
  }

  //  the remaining part of other is simply checked for
  //  not being zero
  for (; i < other_words; ++i, ++op0, ++op1) {
    if (0 != *op0 || 0 != *op1) {
      return true;
    }
  }

  return false;

}

void
BitSetMask::set (index_type index, mask_type mask)
{
  if (index >= m_size && mask == Any) {
    return;
  }

  unsigned int wi = word (index);
  if (wi >= nwords (m_size)) {
    resize (index + 1);
  } else if (index >= m_size) {
    m_size = index + 1;
  }

  unsigned int mi = (unsigned int) mask;
  data_type bm = (1 << bit (index));
  if (mi & 1) {
    mp_data0 [wi] |= bm;
  } else {
    mp_data0 [wi] &= ~bm;
  }
  if (mi & 2) {
    mp_data1 [wi] |= bm;
  } else {
    mp_data1 [wi] &= ~bm;
  }
}

BitSetMask::mask_type
BitSetMask::operator[] (index_type index) const
{
  if (index < m_size) {
    unsigned int wi = word (index);
    data_type bm = (1 << bit (index));
    unsigned int mi = ((mp_data0 [wi] & bm) != 0 ? 1 : 0) | ((mp_data1 [wi] & bm) != 0 ? 2 : 0);
    return mask_type (mi);
  } else {
    return Any;
  }
}

bool
BitSetMask::match (const tl::BitSet &bs) const
{
  unsigned int nw_bs = nwords (bs.m_size);
  unsigned int nw = nwords (m_size);

  const tl::BitSet::data_type *d0 = mp_data0, *d1 = mp_data1;
  const tl::BitSet::data_type *s = bs.mp_data;

  unsigned int i;
  for (i = 0; i < nw; ++i, ++d0, ++d1) {

    tl::BitSet::data_type d = i < nw_bs ? *s++ : 0;

    tl::BitSet::data_type invalid = 0;
    if (i >= nw_bs) {
      invalid = ~invalid;
    } else if (bs.m_size < (i + 1) * (sizeof (tl::BitSet::data_type) * 8)) {
      invalid = (1 << ((i + 1) * (sizeof (tl::BitSet::data_type) * 8) - bs.m_size)) - 1;
    }

    //  "never" matches no valid bit ("never" is: d0 and d1 bits are ones)
    if (((*d0 & *d1) & ~invalid) != 0) {
      return false;
    }

    //  A "true" in place of "false expected" gives "no match"
    if ((*d0 & ~*d1 & d) != 0) {
      return false;
    }
    //  A "false" in place of "true expected" gives "no match"
    if ((*d1 & ~*d0 & ~d) != 0) {
      return false;
    }

  }

  //  as "not set" corresponds to "Any", we can stop here and have a match.
  return true;
}

}
