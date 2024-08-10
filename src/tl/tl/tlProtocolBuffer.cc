
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

#include "tlProtocolBuffer.h"

namespace tl
{

// ----------------------------------------------------------------------------------

ProtocolBufferReader::ProtocolBufferReader (tl::InputStream &input)
  : mp_stream (&input), m_type (PBWireType (0)), m_pos (0), m_pos_before (0)
{
  //  .. nothing yet ..
}

int
ProtocolBufferReader::read_tag ()
{
  uint32_t value = 0;
  read (value);
  m_type = PBWireType (value & 7);
  return int (value >> 3);
}

void
ProtocolBufferReader::skip ()
{
  if (m_type == PB_VARINT) {

    while ((*get (1) & 0x80) != 0) {
      //  continue reading until the byte's MSB is 0
    }

  } else if (m_type == PB_I64) {

    get (8);

  } else if (m_type == PB_I32) {

    get (4);

  } else if (m_type == PB_LEN) {

    size_t value = 0;
    read (value);
    skip_bytes (value);

  }
}

void
ProtocolBufferReader::read (double &d)
{
  uint64_t value = 0;
  read (value);
  d = *reinterpret_cast<const double *> (&value);
}

void
ProtocolBufferReader::read (float &f)
{
  uint32_t value = 0;
  read (value);
  f = *reinterpret_cast<const float *> (&value);
}

void
ProtocolBufferReader::read (std::string &s)
{
  size_t value = 0;
  read (value);

  s = std::string ();
  s.reserve (value);

  const size_t chunk_size = 1024;
  while (value > 0) {
    size_t l = std::min (chunk_size, value);
    s += std::string (get (l), l);
    value -= l;
  }
}

void
ProtocolBufferReader::read (uint32_t &ui32, bool fixed)
{
  if (! fixed) {

    pb_varint ui64 = read_varint ();
    if (ui64 > std::numeric_limits<uint32_t>::max ()) {
      error (tl::to_string (tr ("32 bit value overflow")));
    }

    ui32 = uint32_t (ui64);

  } else {

    ui32 = 0;
    for (unsigned int i = 0; i < sizeof (ui32) && ! at_end (); ++i) {
      ui32 <<= 8;
      ui32 |= (unsigned char) *get (1);
    }

  }
}

void
ProtocolBufferReader::read (int32_t &i32, bool fixed)
{
  uint32_t ui32;
  read (ui32, fixed);

  if (! fixed) {
    if (ui32 & 1) {
      i32 = -(int32_t (ui32 >> 1) + 1);
    } else {
      i32 = int32_t (ui32 >> 1);
    }
  } else {
    i32 = int32_t (ui32);
  }
}

void
ProtocolBufferReader::read (uint64_t &ui64, bool fixed)
{
  if (! fixed) {

    ui64 = read_varint ();

  } else {

    ui64 = 0;
    for (unsigned int i = 0; i < sizeof (ui64) && ! at_end (); ++i) {
      ui64 <<= 8;
      ui64 |= (unsigned char) *get (1);
    }

  }
}

void
ProtocolBufferReader::read (int64_t &i64, bool fixed)
{
  uint64_t ui64;
  read (ui64, fixed);

  if (! fixed) {
    if (ui64 & 1) {
      i64 = -(int64_t (ui64 >> 1) + 1);
    } else {
      i64 = int64_t (ui64 >> 1);
    }
  } else {
    i64 = int64_t (ui64);
  }
}

void
ProtocolBufferReader::read (bool &b)
{
  uint32_t ui32;
  read (ui32);
  b = (ui32 != 0);
}

void
ProtocolBufferReader::open ()
{
  size_t value = 0;
  read (value);
  m_seq_counts.push_back (value);
}

void
ProtocolBufferReader::close ()
{
  if (! m_seq_counts.empty ()) {
    skip_bytes (m_seq_counts.back ());
    m_seq_counts.pop_back ();
  }
}

bool
ProtocolBufferReader::at_end () const
{
  if (m_seq_counts.empty ()) {
    const char *cp = mp_stream->get (1);
    if (cp) {
      mp_stream->unget (1);
      return false;
    } else {
      return true;
    }
  } else {
    return m_seq_counts.back () == 0;
  }
}

const char *
ProtocolBufferReader::get (size_t n)
{
  m_pos_before = m_pos;
  m_pos += n;
  if (! m_seq_counts.empty ()) {
    if (m_seq_counts.back () < n) {
      error (tl::to_string (tr ("sequence underflow")));
    }
    m_seq_counts.back () -= n;
  }

  const char *cp = mp_stream->get (n);
  if (! cp) {
    error (tl::to_string (tr ("unexpected end of file")));
  }

  return cp;
}

pb_varint
ProtocolBufferReader::read_varint ()
{
  pb_varint v = 0;

  while (true) {
    const char *cp = get (1);
    if (! cp) {
      error (tl::to_string (tr ("unexpected end of file")));
    }
    if ((v & 0xfe00000000000000l) != 0) {
      error (tl::to_string (tr ("64 bit integer overflow")));
    }
    v <<= 7;
    v |= (unsigned char) (*cp & 0x7f);
    if ((*cp & 0x80) == 0) {
      break;
    }
  }

  return v;
}

void
ProtocolBufferReader::skip_bytes (size_t n)
{
  const size_t chunk_size = 1024;
  while (n > 0) {
    size_t l = std::min (chunk_size, n);
    if (! mp_stream->get (l)) {
      error (tl::to_string (tr ("unexpected end of file")));
    }
    n -= l;
  }
}

void
ProtocolBufferReader::error (const std::string &msg)
{
  throw ProtocolBufferReaderError (msg + tl::to_string (tr (", in: ")) + mp_stream->source (), m_pos_before);
}

// ----------------------------------------------------------------------------------

ProtocolBufferWriter::ProtocolBufferWriter (tl::OutputStream &stream)
  : mp_stream (&stream), m_bytes_counted (0)
{
  //  .. nothing yet ..
}

void ProtocolBufferWriter::write (int tag, float v)
{
  write (tag, *reinterpret_cast<uint32_t *> (&v));
}

void ProtocolBufferWriter::write (int tag, double v)
{
  write (tag, *reinterpret_cast<uint64_t *> (&v));
}

void ProtocolBufferWriter::write (int tag, uint32_t v, bool fixed)
{
  if (fixed) {

    write_varint (pb_varint ((tag << 3) + PB_I32));

    if (is_counting ()) {

      m_byte_counter_stack.back () += sizeof (v);

    } else {

      char b[sizeof (v)];
      for (unsigned int i = 0; i < sizeof (v); ++i) {
        b[sizeof (v) - 1 - i] = (char) v;
        v >>= 8;
      }

      mp_stream->put (b, sizeof (v));

    }

  } else {

    write_varint (pb_varint ((tag << 3) + PB_VARINT));
    write_varint (pb_varint (v));

  }
}

void ProtocolBufferWriter::write (int tag, int32_t v, bool fixed)
{
  if (fixed) {
    write (tag, uint32_t (v), true);
  } else {
    if (v < 0) {
      write (tag, ((uint32_t (-v) - 1) << 1) + 1, false);
    } else {
      write (tag, uint32_t (v), false);
    }
  }
}

void ProtocolBufferWriter::write (int tag, uint64_t v, bool fixed)
{
  if (fixed) {

    write_varint (pb_varint ((tag << 3) + PB_I64));

    if (is_counting ()) {

      m_byte_counter_stack.back () += sizeof (v);

    } else {

      char b[sizeof (v)];
      for (unsigned int i = 0; i < sizeof (v); ++i) {
        b[sizeof (v) - 1 - i] = (char) v;
        v >>= 8;
      }

      mp_stream->put (b, sizeof (v));

    }

  } else {

    write_varint (pb_varint ((tag << 3) + PB_VARINT));
    write_varint (pb_varint (v));

  }
}

void ProtocolBufferWriter::write (int tag, int64_t v, bool fixed)
{
  if (fixed) {
    write (tag, uint64_t (v), true);
  } else {
    if (v < 0) {
      write (tag, ((uint64_t (-v) - 1) << 1) + 1, false);
    } else {
      write (tag, uint64_t (v), false);
    }
  }
}

void ProtocolBufferWriter::write (int tag, bool b)
{
  write (tag, uint32_t (b ? 1 : 0));
}

void ProtocolBufferWriter::write (int tag, const std::string &s)
{
  write_varint (pb_varint ((tag << 3) + PB_LEN));
  write_varint (s.size ());

  if (is_counting ()) {
    m_byte_counter_stack.back () += s.size ();
  } else {
    mp_stream->put (s.c_str (), s.size ());
  }
}

bool ProtocolBufferWriter::is_counting () const
{
  return ! m_byte_counter_stack.empty ();
}

void ProtocolBufferWriter::begin_seq (int tag, bool counting)
{
  if (counting) {

    //  header only
    m_byte_counter_stack.push_back (0);
    write_varint (pb_varint ((tag << 3) + PB_LEN));

    //  body only
    m_byte_counter_stack.push_back (0);

  } else {

    write_varint (pb_varint ((tag << 3) + PB_LEN));
    write_varint (m_bytes_counted);

  }
}

void ProtocolBufferWriter::end_seq ()
{
  if (is_counting ()) {

    m_bytes_counted = m_byte_counter_stack.back ();
    m_byte_counter_stack.pop_back ();

    //  just for adding the required bytes
    write_varint (m_bytes_counted);

    //  now add header bytes
    m_bytes_counted += m_byte_counter_stack.back ();
    m_byte_counter_stack.pop_back ();

  }
}

void
ProtocolBufferWriter::write_varint (pb_varint v)
{
  if (is_counting ()) {

    size_t n = 0;
    while (true) {
      ++n;
      if (v < 0x80) {
        break;
      }
      v >>= 7;
    }

    m_byte_counter_stack.back () += n;

  } else {

    char b[16];
    char *cp = b;
    while (true) {
      if (v < 0x80) {
        *cp++ = char (v);
        break;
      } else {
        *cp++ = (char (v) | 0x80);
      }
      v >>= 7;
    }

    m_byte_counter_stack.back () += (cp - b);

  }
}

}
