
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


#include "tlStream.h"
#include "tlAssert.h"
#include "tlException.h"
#include "dbGDS2Writer.h"
#include "dbGDS2.h"

#include <stdio.h>
#include <errno.h>

#include <limits>

namespace db
{

// ------------------------------------------------------------------
//  GDS2Writer implementation

GDS2Writer::GDS2Writer ()
  : mp_stream (0), m_progress (tl::to_string (tr ("Writing GDS2 file")), 10000)
{
  m_progress.set_format (tl::to_string (tr ("%.0f MB")));
  m_progress.set_unit (1024 * 1024);
}

void 
GDS2Writer::write_byte (unsigned char b)
{
  mp_stream->put ((const char *) &b, 1);
}

void 
GDS2Writer::write_record_size (int16_t i)
{
  gds2h (i);
  mp_stream->put ( (char*)(&i), sizeof (i));
}

void 
GDS2Writer::write_record (int16_t i)
{
  gds2h (i);
  mp_stream->put ( (char*)(&i), sizeof (i));
}

void 
GDS2Writer::write_short (int16_t i)
{
  gds2h (i);
  mp_stream->put ( (char*)(&i), sizeof (i));
}

void 
GDS2Writer::write_int (int32_t l)
{
  gds2h (l);
  mp_stream->put ( (char*)(&l), sizeof (l));
}

void 
GDS2Writer::write_double (double d)
{
  char b[8];
  
  b[0] = 0;
  if (d < 0) {
    b[0] = char (0x80);
    d = -d;
  }

  //  compute the next power of 16 that that value will fit in
  int e = 0;
  if (d < 1e-77 /*~16^-64*/) {
    d = 0;
  } else {
    double lg16 = log (d) / log (16.0);
    e = int (ceil (log (d) / log (16.0)));
    if (e == lg16) {
      ++e;
    }
  }

  d /= pow (16.0, e - 14);

  tl_assert (e >= -64 && e < 64);
  b[0] |= ((e + 64) & 0x7f);

  uint64_t m = uint64_t (d + 0.5);
  for (int i = 7; i > 0; --i) {
    b[i] = (m & 0xff);
    m >>= 8;
  }

  mp_stream->put (b, sizeof (b));
}

void 
GDS2Writer::write_time (const short *t)
{
  for (unsigned int i = 0; i < 6; ++i) {
    write_short (t [i]);
  }
}

void 
GDS2Writer::write_string (const char *t)
{
  size_t l = strlen (t);
  mp_stream->put (t, l);
  if ((l & 1) != 0) {
    write_byte (0);
  }
}

void 
GDS2Writer::write_string (const std::string &t)
{
  size_t l = t.size ();
  mp_stream->put (t.c_str (), l);
  if ((l & 1) != 0) {
    write_byte (0);
  }
}

void 
GDS2Writer::progress_checkpoint ()
{
  m_progress.set (mp_stream->pos ());
}

} // namespace db

