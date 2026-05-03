
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#include "tlBinaryStream.h"

#include <limits>

namespace tl
{

// ---------------------------------------------------------------
//  Type codes for Variant serialization
//  These type codes are intentionally separated from tl::Variant's
//  type codes to allow a backward-compatible serialization scheme.

enum VariantTypeCode
{
  var_type_nil        = 0,
  var_type_bool       = 1,
  var_type_char       = 2,
  var_type_schar      = 3,
  var_type_uchar      = 4,
  var_type_short      = 5,
  var_type_ushort     = 6,
  var_type_int        = 7,
  var_type_uint       = 8,
  var_type_long       = 9,
  var_type_ulong      = 10,
  var_type_longlong   = 11,
  var_type_ulonglong  = 12,
  var_type_id         = 13,
  var_type_float      = 14,
  var_type_double     = 15,
  var_type_string     = 16,
  var_type_bytes      = 17,
  var_type_list       = 18,
  var_type_array      = 19,

  var_type_other      = -1
};

// ---------------------------------------------------------------

class UnexpectedEndOfFileException
  : public tl::Exception
{
public:
  UnexpectedEndOfFileException (const std::string &f)
    : tl::Exception (tl::to_string (tr ("Unexpected end of file in: %s")), f)
  { }
};

class InvalidVariantTypeCode
  : public tl::Exception
{
public:
  InvalidVariantTypeCode (const std::string &f, int tc)
    : tl::Exception (tl::to_string (tr ("Invalid variant type code %d in file: %s - maybe file is too new for this build?")), tc, f)
  { }
};

// ---------------------------------------------------------------

BinaryInputStream::BinaryInputStream (InputStream &stream)
  : m_stream (stream)
{
  //  .. nothing yet ..
}

void
BinaryInputStream::reset ()
{
  m_stream.reset ();
}

BinaryInputStream &
BinaryInputStream::operator>> (std::string &v)
{
  uint64_t l = 0;
  *this >> l;

  v.clear ();
  v.reserve (l);

  size_t chunk_size = 1024, n = l;
  while (n > 0) {
    size_t chunk_len = std::min (n, chunk_size);
    const char *chunk_data = m_stream.get (chunk_len);
    if (! chunk_data) {
      throw UnexpectedEndOfFileException (source ());
    }
    v.append (chunk_data, chunk_len);
    n -= chunk_len;
  }

  return *this;
}

BinaryInputStream &
BinaryInputStream::operator>> (std::vector<char> &v)
{
  uint64_t l = 0;
  *this >> l;

  v.clear ();
  v.reserve (l);

  size_t chunk_size = 1024, n = l;
  while (n > 0) {
    size_t chunk_len = std::min (n, chunk_size);
    const char *chunk_data = m_stream.get (chunk_len);
    if (! chunk_data) {
      throw UnexpectedEndOfFileException (source ());
    }
    v.insert (v.end (), chunk_data, chunk_data + chunk_len);
    n -= chunk_len;
  }

  return *this;
}

BinaryInputStream &
BinaryInputStream::operator>> (double &v)
{
  const char *chunk_data = m_stream.get (8);
  if (! chunk_data) {
    throw UnexpectedEndOfFileException (source ());
  }
  //  TODO: for now, we assume that file and memory layout are identical on all platforms
  v = *reinterpret_cast<const double *> (chunk_data);

  return *this;
}

BinaryInputStream &
BinaryInputStream::operator>> (float &v)
{
  const char *chunk_data = m_stream.get (4);
  if (! chunk_data) {
    throw UnexpectedEndOfFileException (source ());
  }
  //  TODO: for now, we assume that file and memory layout are identical on all platforms
  v = *reinterpret_cast<const float *> (chunk_data);

  return *this;
}

BinaryInputStream &
BinaryInputStream::operator>> (bool &v)
{
  const char *chunk_data = m_stream.get (1);
  if (! chunk_data) {
    throw UnexpectedEndOfFileException (source ());
  }
  v = *chunk_data != 0;

  return *this;
}

BinaryInputStream &
BinaryInputStream::operator>> (uint8_t &v)
{
  const char *chunk_data = m_stream.get (1);
  if (! chunk_data) {
    throw UnexpectedEndOfFileException (source ());
  }
  v = (uint8_t) *chunk_data;

  return *this;
}

BinaryInputStream &
BinaryInputStream::operator>> (int8_t &v)
{
  const char *chunk_data = m_stream.get (1);
  if (! chunk_data) {
    throw UnexpectedEndOfFileException (source ());
  }
  v = (int8_t) *chunk_data;

  return *this;
}

BinaryInputStream &
BinaryInputStream::operator>> (uint16_t &v)
{
  const char *chunk_data = m_stream.get (2);
  if (! chunk_data) {
    throw UnexpectedEndOfFileException (source ());
  }
  //  TODO: for now, we assume that file and memory layout are identical on all platforms
  v = *reinterpret_cast<const uint16_t *> (chunk_data);

  return *this;
}

BinaryInputStream &
BinaryInputStream::operator>> (int16_t &v)
{
  const char *chunk_data = m_stream.get (2);
  if (! chunk_data) {
    throw UnexpectedEndOfFileException (source ());
  }
  //  TODO: for now, we assume that file and memory layout are identical on all platforms
  v = *reinterpret_cast<const int16_t *> (chunk_data);

  return *this;
}

BinaryInputStream &
BinaryInputStream::operator>> (uint32_t &v)
{
  const char *chunk_data = m_stream.get (4);
  if (! chunk_data) {
    throw UnexpectedEndOfFileException (source ());
  }
  //  TODO: for now, we assume that file and memory layout are identical on all platforms
  v = *reinterpret_cast<const uint32_t *> (chunk_data);

  return *this;
}

BinaryInputStream &
BinaryInputStream::operator>> (int32_t &v)
{
  const char *chunk_data = m_stream.get (4);
  if (! chunk_data) {
    throw UnexpectedEndOfFileException (source ());
  }
  //  TODO: for now, we assume that file and memory layout are identical on all platforms
  v = *reinterpret_cast<const int32_t *> (chunk_data);

  return *this;
}

BinaryInputStream &
BinaryInputStream::operator>> (uint64_t &v)
{
  const char *chunk_data = m_stream.get (8);
  if (! chunk_data) {
    throw UnexpectedEndOfFileException (source ());
  }
  //  TODO: for now, we assume that file and memory layout are identical on all platforms
  v = *reinterpret_cast<const uint64_t *> (chunk_data);

  return *this;
}

BinaryInputStream &
BinaryInputStream::operator>> (int64_t &v)
{
  const char *chunk_data = m_stream.get (8);
  if (! chunk_data) {
    throw UnexpectedEndOfFileException (source ());
  }
  //  TODO: for now, we assume that file and memory layout are identical on all platforms
  v = *reinterpret_cast<const int64_t *> (chunk_data);

  return *this;
}

BinaryInputStream &
BinaryInputStream::operator>> (tl::Variant &v)
{
  int16_t var_type = -1;
  *this >> var_type;

  switch (var_type) {
  case VariantTypeCode::var_type_nil:
    v = tl::Variant ();
    break;
  case VariantTypeCode::var_type_bool:
    {
      bool f = false;
      *this >> f;
      v = tl::Variant (f);
    }
    break;
  case VariantTypeCode::var_type_char:
    {
      uint8_t vv = 0;
      *this >> vv;
      v = tl::Variant ((char) vv);
    }
    break;
  case VariantTypeCode::var_type_schar:
    {
      int8_t vv = 0;
      *this >> vv;
      v = tl::Variant ((signed char) vv);
    }
    break;
  case VariantTypeCode::var_type_uchar:
    {
      uint8_t vv = 0;
      *this >> vv;
      v = tl::Variant ((unsigned char) vv);
    }
    break;
  case VariantTypeCode::var_type_short:
    {
      int16_t vv = 0;
      *this >> vv;
      v = tl::Variant ((short) vv);
    }
    break;
  case VariantTypeCode::var_type_ushort:
    {
      uint16_t vv = 0;
      *this >> vv;
      v = tl::Variant ((unsigned short) vv);
    }
    break;
  case VariantTypeCode::var_type_int:
    {
      int32_t vv = 0;
      *this >> vv;
      v = tl::Variant ((int) vv);
    }
    break;
  case VariantTypeCode::var_type_uint:
    {
      uint32_t vv = 0;
      *this >> vv;
      v = tl::Variant ((unsigned int) vv);
    }
    break;
  case VariantTypeCode::var_type_long:
    {
      int64_t vv = 0;
      *this >> vv;
      if (vv >= (int64_t) std::numeric_limits<long>::min () && vv <= (int64_t) std::numeric_limits<long>::max ()) {
        v = tl::Variant ((long) vv);
      } else {
        //  Linux to Windows migration
        v = tl::Variant ((long long) vv);
      }
    }
    break;
  case VariantTypeCode::var_type_ulong:
    {
      uint64_t vv = 0;
      *this >> vv;
      if (vv >= (uint64_t) std::numeric_limits<unsigned long>::min () && vv <= (uint64_t) std::numeric_limits<unsigned long>::max ()) {
        v = tl::Variant ((unsigned long) vv);
      } else {
        //  Linux to Windows migration
        v = tl::Variant ((unsigned long long) vv);
      }
    }
    break;
    break;
  case VariantTypeCode::var_type_longlong:
    {
      int64_t vv = 0;
      *this >> vv;
      v = tl::Variant ((long long) vv);
    }
    break;
  case VariantTypeCode::var_type_ulonglong:
    {
      uint64_t vv = 0;
      *this >> vv;
      v = tl::Variant ((unsigned long long) vv);
    }
    break;
  case VariantTypeCode::var_type_id:
    {
      uint64_t vv = 0;
      *this >> vv;
      v = tl::Variant ((size_t) vv, true /*as id*/);
    }
    break;
  case VariantTypeCode::var_type_float:
    {
      float vv = 0;
      *this >> vv;
      v = tl::Variant (vv);
    }
    break;
  case VariantTypeCode::var_type_double:
    {
      double vv = 0;
      *this >> vv;
      v = tl::Variant (vv);
    }
    break;
  case VariantTypeCode::var_type_string:
    {
      std::string vv;
      *this >> vv;
      v = tl::Variant (std::move (vv));
    }
    break;
  case VariantTypeCode::var_type_bytes:
    {
      std::vector<char> vv;
      *this >> vv;
      v = tl::Variant (std::move (vv));
    }
    break;
  case VariantTypeCode::var_type_list:
    {
      uint64_t n = 0;
      *this >> n;
      v = tl::Variant::empty_list ();
      v.reserve (n);
      while (n-- > 0) {
        tl::Variant vv;
        *this >> vv;
        v.push (std::move (vv));
      }
    }
    break;
  case VariantTypeCode::var_type_array:
    {
      uint64_t n = 0;
      *this >> n;
      v = tl::Variant::empty_array ();
      while (n-- > 0) {
        tl::Variant vk, vv;
        *this >> vk;
        *this >> vv;
        v.insert (std::move (vk), std::move (vv));
      }
    }
    break;
  case VariantTypeCode::var_type_other:
    {
      std::string s;
      *this >> s;
      tl::Extractor ex (s.c_str ());
      v = tl::Variant ();
      ex.read (v);
    }
    break;
  default:
    throw InvalidVariantTypeCode (source (), int (var_type));
  }

  return *this;
}

// ---------------------------------------------------------------
//  BinaryOutputStream implementation

BinaryOutputStream::BinaryOutputStream (OutputStreamBase &delegate)
  : OutputStream (delegate, false)
{
  //  .. nothing yet ..
}

BinaryOutputStream::BinaryOutputStream (OutputStreamBase *delegate)
  : OutputStream (delegate, false)
{
  //  .. nothing yet ..
}

BinaryOutputStream::BinaryOutputStream (const std::string &abstract_path, OutputStreamMode om, int keep_backups)
  : OutputStream (abstract_path, om, false, keep_backups)
{
  //  .. nothing yet ..
}

void
BinaryOutputStream::put_native (const char *s, size_t n)
{
  //  the native format for a string is a length field (uint64_t) and the bytes
  //  TODO: for now we assume that the memory layout is the same for all platforms
  uint64_t len = uint64_t (n);
  put_raw ((const char *) &len, 8);
  put_raw (s, n);
}

void
BinaryOutputStream::put_native (const std::string &s)
{
  //  the native format for a string is a length field (uint64_t) and the bytes
  //  TODO: for now we assume that the memory layout is the same for all platforms
  uint64_t len = uint64_t (s.size ());
  put_raw ((const char *) &len, 8);
  put_raw (s.c_str (), s.size ());
}

void
BinaryOutputStream::put_native (double v)
{
  //  TODO: for now we assume that the memory layout is the same for all platforms
  put_raw ((const char *) &v, 8);
}

void
BinaryOutputStream::put_native (float v)
{
  //  TODO: for now we assume that the memory layout is the same for all platforms
  put_raw ((const char *) &v, 4);
}

void
BinaryOutputStream::put_native (bool v)
{
  char c = v ? 1 : 0;
  put_raw (&c, 1);
}

void
BinaryOutputStream::put_native (uint8_t v)
{
  put_raw ((const char *) &v, 1);
}

void
BinaryOutputStream::put_native (int8_t v)
{
  put_raw ((const char *) &v, 1);
}

void
BinaryOutputStream::put_native (uint16_t v)
{
  //  TODO: for now we assume that the memory layout is the same for all platforms
  put_raw ((const char *) &v, 2);
}

void
BinaryOutputStream::put_native (int16_t v)
{
  //  TODO: for now we assume that the memory layout is the same for all platforms
  put_raw ((const char *) &v, 2);
}

void
BinaryOutputStream::put_native (uint32_t v)
{
  //  TODO: for now we assume that the memory layout is the same for all platforms
  put_raw ((const char *) &v, 4);
}

void
BinaryOutputStream::put_native (int32_t v)
{
  //  TODO: for now we assume that the memory layout is the same for all platforms
  put_raw ((const char *) &v, 4);
}

void
BinaryOutputStream::put_native (uint64_t v)
{
  //  TODO: for now we assume that the memory layout is the same for all platforms
  put_raw ((const char *) &v, 8);
}

void
BinaryOutputStream::put_native (int64_t v)
{
  //  TODO: for now we assume that the memory layout is the same for all platforms
  put_raw ((const char *) &v, 8);
}

void
BinaryOutputStream::put_native (const tl::Variant &v)
{
  switch (v.type_code ()) {
  case tl::Variant::t_nil:
    put_native ((int16_t) VariantTypeCode::var_type_nil);
    break;
  case tl::Variant::t_bool:
    put_native ((int16_t) VariantTypeCode::var_type_bool);
    put_native (v.to_bool ());
    break;
  case tl::Variant::t_char:
    put_native ((int16_t) VariantTypeCode::var_type_char);
    put_native ((uint8_t) v.to_char ());
    break;
  case tl::Variant::t_schar:
    put_native ((int16_t) VariantTypeCode::var_type_schar);
    put_native ((int8_t) v.to_schar ());
    break;
  case tl::Variant::t_uchar:
    put_native ((int16_t) VariantTypeCode::var_type_uchar);
    put_native ((uint8_t) v.to_uchar ());
    break;
  case tl::Variant::t_short:
    put_native ((int16_t) VariantTypeCode::var_type_short);
    put_native ((int16_t) v.to_short ());
    break;
  case tl::Variant::t_ushort:
    put_native ((int16_t) VariantTypeCode::var_type_ushort);
    put_native ((uint16_t) v.to_ushort ());
    break;
  case tl::Variant::t_int:
    put_native ((int16_t) VariantTypeCode::var_type_int);
    put_native ((int32_t) v.to_int ());
    break;
  case tl::Variant::t_uint:
    put_native ((int16_t) VariantTypeCode::var_type_uint);
    put_native ((uint32_t) v.to_uint ());
    break;
  case tl::Variant::t_long:
    put_native ((int16_t) VariantTypeCode::var_type_long);
    //  NOTE: "long" is always encoded as 64 bit for compatibility of Windows + Linux
    put_native ((int64_t) v.to_long ());
    break;
  case tl::Variant::t_ulong:
    put_native ((int16_t) VariantTypeCode::var_type_ulong);
    //  NOTE: "ulong" is always encoded as 64 bit for compatibility of Windows + Linux
    put_native ((uint64_t) v.to_ulong ());
    break;
  case tl::Variant::t_longlong:
    put_native ((int16_t) VariantTypeCode::var_type_longlong);
    put_native ((int64_t) v.to_longlong ());
    break;
  case tl::Variant::t_ulonglong:
    put_native ((int16_t) VariantTypeCode::var_type_ulonglong);
    put_native ((uint64_t) v.to_ulonglong ());
    break;
  case tl::Variant::t_id:
    put_native ((int16_t) VariantTypeCode::var_type_id);
    //  NOTE: "id" is always encoded as 64 bit
    put_native ((uint64_t) v.to_id ());
    break;
  case tl::Variant::t_float:
    put_native ((int16_t) VariantTypeCode::var_type_float);
    put_native (v.to_float ());
    break;
  case tl::Variant::t_double:
    put_native ((int16_t) VariantTypeCode::var_type_double);
    put_native (v.to_double ());
    break;
  case tl::Variant::t_string:
  case tl::Variant::t_stdstring:
    put_native ((int16_t) VariantTypeCode::var_type_string);
    put_native (v.to_stdstring ());
    break;
  case tl::Variant::t_bytearray:
#if defined(HAVE_QT)
  case tl::Variant::t_qstring:
  case tl::Variant::t_qbytearray:
#endif
    {
      put_native ((int16_t) VariantTypeCode::var_type_bytes);
      std::vector<char> bytes = v.to_bytearray ();
      put_native (bytes.begin ().operator-> (), bytes.size ());
    }
    break;
  case tl::Variant::t_list:
    {
      put_native ((int16_t) VariantTypeCode::var_type_list);
      put_native ((uint64_t) v.size ());
      for (auto i = v.begin (); i != v.end (); ++i) {
        put_native (*i);
      }
    }
    break;
  case tl::Variant::t_array:
    {
      put_native ((int16_t) VariantTypeCode::var_type_array);
      put_native ((uint64_t) v.array_size ());
      for (auto i = v.begin_array (); i != v.end_array (); ++i) {
        put_native (i->first);
        put_native (i->second);
      }
    }
    break;
  default:
    put_native ((int16_t) VariantTypeCode::var_type_other);
    put_native (v.to_parsable_string ());
    break;
  }
}

}
