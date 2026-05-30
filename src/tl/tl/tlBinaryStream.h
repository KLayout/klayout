
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


#ifndef HDR_tlBinaryStream
#define HDR_tlBinaryStream

#include "tlCommon.h"
#include "tlStream.h"
#include "tlVariant.h"

namespace tl
{

// ---------------------------------------------------------------------------------

/**
 *  @brief An output stream specialized on binary representation
 */

class TL_PUBLIC BinaryOutputStream
  : public OutputStream
{
public:
  /**
   *  @brief Default constructor
   *
   *  This constructor takes a delegate object.
   */
  BinaryOutputStream (OutputStreamBase &delegate);

  /**
   *  @brief Default constructor
   *
   *  This constructor takes a delegate object. The stream will own the delegate.
   */
  BinaryOutputStream (OutputStreamBase *delegate);

  /**
   *  @brief Open an output stream with the given path and stream mode
   *
   *  This will automatically create a delegate object and delete it later.
   */
  BinaryOutputStream (const std::string &abstract_path, OutputStreamMode om = OM_Auto, int keep_backups = 0);

  /**
   *  @brief << operator: inserts character
   */
  OutputStream &operator<< (char s)
  {
    put (&s, 1);
    return *this;
  }

  /**
   *  @brief << operator: inserts a character
   */
  OutputStream &operator<< (unsigned char s)
  {
    put ((const char *) &s, 1);
    return *this;
  }

  /**
   *  @brief << operator: inserts a string
   *
   *  In binary mode, the string is inserted as a length/data
   *  combination. That matches the extraction in BinaryInputStream.
   */
  BinaryOutputStream &operator<< (const char *s)
  {
    put_native (s, strlen (s));
    return *this;
  }

  /**
   *  @brief << operator: inserts a string
   *
   *  In binary mode, the string is inserted as a length/data
   *  combination. That matches the extraction in BinaryInputStream.
   */
  BinaryOutputStream &operator<< (const std::string &s)
  {
    put_native (s);
    return *this;
  }

  /**
   *  @brief << operator: inserts an object supported by "put_native".
   */
  template <class T>
  BinaryOutputStream &operator<< (const T &t)
  {
    put_native (t);
    return *this;
  }

private:
  void put_native (const std::string &s);
  void put_native (const char *s, size_t n);
  void put_native (double v);
  void put_native (float v);
  void put_native (bool v);
  void put_native (uint8_t v);
  void put_native (int8_t v);
  void put_native (uint16_t v);
  void put_native (int16_t v);
  void put_native (uint32_t v);
  void put_native (int32_t v);
  void put_native (uint64_t v);
  void put_native (int64_t v);
  void put_native (const tl::Variant &v);

  void set_as_text (bool f);

  //  No copying currently
  BinaryOutputStream (const BinaryOutputStream &);
  BinaryOutputStream &operator= (const BinaryOutputStream &);
};

// ---------------------------------------------------------------------------------

/**
 *  @brief A binary input stream
 *
 *  This class is put in front of a InputStream to retrieve binary primitives from the stream.
 *  The binary format corresponds to binary mode of OutputStream.
 */
class TL_PUBLIC BinaryInputStream
{
public:
  /**
   *  @brief Default constructor
   *
   *  This constructor takes a delegate object.
   */
  BinaryInputStream (InputStream &stream);

  /**
   *  @brief Gets the raw stream
   */
  InputStream &raw_stream ()
  {
    return m_stream;
  }

  /**
   *  @brief Get the source specification
   */
  std::string source () const
  {
    return m_stream.source ();
  }

  /**
   *  @brief Gets a value of a specific type
   */
  BinaryInputStream &operator>> (std::string &v);
  BinaryInputStream &operator>> (std::vector<char> &v);
  BinaryInputStream &operator>> (double &v);
  BinaryInputStream &operator>> (float &v);
  BinaryInputStream &operator>> (bool &v);
  BinaryInputStream &operator>> (uint8_t &v);
  BinaryInputStream &operator>> (int8_t &v);
  BinaryInputStream &operator>> (uint16_t &v);
  BinaryInputStream &operator>> (int16_t &v);
  BinaryInputStream &operator>> (uint32_t &v);
  BinaryInputStream &operator>> (int32_t &v);
  BinaryInputStream &operator>> (uint64_t &v);
  BinaryInputStream &operator>> (int64_t &v);
  BinaryInputStream &operator>> (tl::Variant &v);

  /**
   *  @brief Resets to the initial position
   */
  void reset ();

private:
  InputStream &m_stream;

  //  no copying
  BinaryInputStream (const BinaryInputStream &);
  BinaryInputStream &operator= (const BinaryInputStream &);
};

}

#endif
