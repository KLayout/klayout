
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

#include "tlBitSet.h"

namespace tl
{

static inline unsigned int nwords (BitSet::size_type size)
{
  return (size + (sizeof (BitSet::data_type) * 8 - 1)) / (sizeof (BitSet::data_type) * 8);
}

static inline unsigned int word (BitSet::size_type index)
{
  return index / (sizeof (BitSet::data_type) * 8);
}

static inline unsigned int bit (BitSet::size_type index)
{
  //  first bit is the highest bit, so that comparing the uint's is good enough
  //  for lexical order.
  return 31 - (index % (sizeof (BitSet::data_type) * 8));
}

BitSet &
BitSet::operator= (const BitSet &other)
{
  if (&other != this) {

    clear ();

    //  reallocate
    m_size = other.m_size;
    unsigned int words = nwords (m_size);
    mp_data = new data_type[words];
    data_type *t = mp_data;
    data_type *s = other.mp_data;
    for (unsigned int i = 0; i < words; ++i) {
      *t++ = *s++;
    }

  }
  return *this;
}

BitSet &
BitSet::operator= (BitSet &&other)
{
  if (&other != this) {
    swap (other);
  }
  return *this;
}

void
BitSet::clear ()
{
  if (mp_data) {
    delete [] mp_data;
  }
  mp_data = 0;
  m_size = 0;
}

void
BitSet::resize (size_type size)
{
  if (size > m_size) {

    unsigned int words = nwords (m_size);
    unsigned int new_words = nwords (size);

    if (new_words > words) {

      //  reallocate
      data_type *new_data = new data_type[new_words];
      data_type *t = new_data;
      data_type *s = mp_data;
      unsigned int i;
      for (i = 0; i < words; ++i) {
        *t++ = *s++;
      }
      for (; i < new_words; ++i) {
        *t++ = 0;
      }
      delete mp_data;
      mp_data = new_data;
      m_size = size;

    }

  }
}

bool
BitSet::operator== (const BitSet &other) const
{
  unsigned int words = nwords (m_size);
  unsigned int other_words = nwords (other.m_size);

  const data_type *p = mp_data;
  const data_type *op = other.mp_data;
  unsigned int i;
  for (i = 0; i < words && i < other_words; ++i) {
    if (*p++ != *op++) {
      return false;
    }
  }
  for (; i < words; ++i) {
    if (*p++ != 0) {
      return false;
    }
  }
  for (; i < other_words; ++i) {
    if (0 != *op++) {
      return false;
    }
  }
  return true;
}

bool
BitSet::operator< (const BitSet &other) const
{
  unsigned int words = nwords (m_size);
  unsigned int other_words = nwords (other.m_size);

  const data_type *p = mp_data;
  const data_type *op = other.mp_data;
  unsigned int i;
  for (i = 0; i < words && i < other_words; ++i, ++p, ++op) {
    if (*p != *op) {
      return *p < *op;
    }
  }
  for (; i < other_words; ++i, ++op) {
    if (0 != *op) {
      return true;
    }
  }
  return false;
}

void
BitSet::set (index_type index)
{
  unsigned int wi = word (index);
  if (wi >= nwords (m_size)) {
    resize (index + 1);
  } else if (index >= m_size) {
    m_size = index + 1;
  }
  mp_data [wi] |= (1 << bit (index));
}

void
BitSet::reset (index_type index)
{
  if (index < m_size) {
    unsigned int wi = word (index);
    mp_data [wi] &= ~(1 << bit (index));
  }
}

bool
BitSet::operator[] (index_type index) const
{
  if (index < m_size) {
    unsigned int wi = word (index);
    return (mp_data [wi] & (1 << bit (index))) != 0;
  } else {
    return false;
  }
}

}
