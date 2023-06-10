
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



#include "dbGDS2Reader.h"
#include "dbGDS2.h"
#include "dbArray.h"

#include "tlException.h"
#include "tlString.h"
#include "tlClassRegistry.h"

namespace db
{

// ---------------------------------------------------------------
//  GDS2Reader

GDS2Reader::GDS2Reader (tl::InputStream &s)
  : m_stream (s), 
    m_recnum (0),
    m_reclen (0),
    m_recptr (0),
    mp_rec_buf (0),
    m_stored_rec (0),
    m_allow_big_records (true),
    m_progress (tl::to_string (tr ("Reading GDS2 file")), 10000)
{
  m_progress.set_format (tl::to_string (tr ("%.0f MB")));
  m_progress.set_unit (1024 * 1024);
}

GDS2Reader::~GDS2Reader ()
{
  //  .. nothing yet ..
}

void
GDS2Reader::init (const db::LoadLayoutOptions &options)
{
  GDS2ReaderBase::init (options);

  m_allow_big_records = options.get_options<db::GDS2ReaderOptions> ().allow_big_records;

  m_recnum = 0;
  --m_recnum;
  m_reclen = 0;
}

void 
GDS2Reader::unget_record (short rec_id)
{  
  m_stored_rec = rec_id;
  m_recptr = 0; 
}

short 
GDS2Reader::get_record ()
{  
  if (m_stored_rec) {
    short ret = m_stored_rec;
    m_stored_rec = 0;
    return ret;
  }

  unsigned char *b = (unsigned char *) m_stream.get (4);
  if (! b) {
    error (tl::to_string (tr ("Unexpected end-of-file")));
    return 0;
  }

  m_recnum++;

  uint16_t l = *((uint16_t *)b);
  gds2h ((int16_t &) l);
  m_reclen = size_t (l);

  uint16_t rec_id = ((uint16_t *)b) [1];
  gds2h ((int16_t &) rec_id);

  if (m_reclen < 4) {
    error (tl::to_string (tr ("Invalid record length (less than 4)")));
  }
  if (m_reclen >= 0x8000) {
    if (m_allow_big_records) {
      warn (tl::to_string (tr ("Record length larger than 0x8000 encountered: interpreting as unsigned")));
    } else {
      error (tl::to_string (tr ("Record length larger than 0x8000 encountered (reader is configured not to allow such records)")));
    }
  }
  if (m_reclen % 2 == 1) {
    warn (tl::to_string (tr ("Odd record length")));
  }

  m_reclen -= 4;

  if (m_reclen > 0) {
    mp_rec_buf = (unsigned char *) m_stream.get (m_reclen);
    if (! mp_rec_buf) {
      error (tl::to_string (tr ("Unexpected end-of-file")));
    }
  } else {
    mp_rec_buf = 0;
  }
   
  m_recptr = 0; 
  return rec_id;
}

void
GDS2Reader::record_underflow_error ()
{
  error (tl::to_string (tr ("Record too short")));
}

inline int 
GDS2Reader::get_int ()
{
  unsigned char *b = mp_rec_buf + m_recptr;
  if ((m_recptr += 4) > m_reclen) {
    record_underflow_error ();
  }

  int32_t l = *((int32_t *)b);
  gds2h (l);
  return l;
}

inline short 
GDS2Reader::get_short ()
{
  unsigned char *b = mp_rec_buf + m_recptr;
  if ((m_recptr += 2) > m_reclen) {
    record_underflow_error ();
  }

  int16_t s = *((int16_t *)b);
  gds2h (s);
  return s;
}

inline unsigned short 
GDS2Reader::get_ushort ()
{
  unsigned char *b = mp_rec_buf + m_recptr;
  if ((m_recptr += 2) > m_reclen) {
    record_underflow_error ();
  }

  uint16_t s = *((uint16_t *)b);
  gds2h ((int16_t &) s);
  return s;
}

inline double 
GDS2Reader::get_double ()
{
  unsigned char *b = mp_rec_buf + m_recptr;
  if ((m_recptr += 8) > m_reclen) {
    record_underflow_error ();
  }

  uint32_t l0 = ((uint32_t *)b) [0];
  gds2h ((int32_t &) l0);
  l0 &= 0xffffff;
  uint32_t l1 = ((uint32_t *)b) [1];
  gds2h ((int32_t &) l1);

  double x = 4294967296.0 * double (l0) + double (l1);

  if (b[0] & 0x80) {
    x = -x;
  }
  
  int e = int (b[0] & 0x7f) - (64 + 14);
  if (e != 0) {
    x *= pow (16.0, double (e));
  }

  return x;
}

const char *
GDS2Reader::get_string ()
{
  if (m_reclen == 0) {
    return "";
  }

  if (mp_rec_buf [m_reclen - 1] == 0) {
    //  we already have a terminating '\0': just return the string's location
    return (const char *) mp_rec_buf;
  } else {
    //  use the temporary buffer to create a zero-terminated string
    m_string_buf.assign ((const char *) mp_rec_buf, 0, m_reclen);
    return m_string_buf.c_str ();
  }
}

void
GDS2Reader::get_string (std::string &s) const
{
  if (m_reclen == 0) {
    s.clear ();
  } else {
    //  strip padding 0 characters
    unsigned long n = m_reclen;
    while (n > 0 && mp_rec_buf [n - 1] == 0) {
      --n;
    }
    s.assign ((const char *) mp_rec_buf, n);
  }
}

void
GDS2Reader::get_time (unsigned int *mod_time, unsigned int *access_time)
{
  unsigned int length = (unsigned int) (m_reclen / sizeof (uint16_t));
  for (unsigned int l = 0; l < length && l < 6; ++l) {
    mod_time [l] = get_ushort ();
  }
  for (unsigned int l = 0; l + 6 < length && l < 6; ++l) {
    access_time [l] = get_ushort ();
  }

  //  correct year if required
  if (mod_time [0] == 0 && mod_time [1] == 0 && mod_time [2] == 0) {
    //  leave it
  } else if (mod_time [0] < 50) {
    mod_time [0] += 2000;
  } else if (mod_time [0] < 1900) {
    mod_time [0] += 1900;
  }
  if (access_time [0] == 0 && access_time [1] == 0 && access_time [2] == 0) {
    //  leave it
  } else if (access_time [0] < 50) {
    access_time [0] += 2000;
  } else if (access_time [0] < 1900) {
    access_time [0] += 1900;
  }
}

GDS2XY *
GDS2Reader::get_xy_data (unsigned int &length)
{
  length = (unsigned int) (m_reclen / sizeof (GDS2XY));
  return (GDS2XY *) mp_rec_buf;
}

void  
GDS2Reader::progress_checkpoint () 
{
  m_progress.set (m_stream.pos ());
}

std::string
GDS2Reader::path () const
{
  return m_stream.source ();
}

void 
GDS2Reader::error (const std::string &msg)
{
  throw GDS2ReaderException (msg, m_stream.pos (), m_recnum, cellname ().c_str ());
}

void 
GDS2Reader::warn (const std::string &msg, int wl)
{
  if (warn_level () < wl) {
    return;
  }

  // TODO: compress
  tl::warn << msg 
           << tl::to_string (tr (" (position=")) << m_stream.pos ()
           << tl::to_string (tr (", record number=")) << m_recnum
           << tl::to_string (tr (", cell=")) << cellname ().c_str ()
           << ")";
}

}

