
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


#ifndef HDR_tlDeflate
#define HDR_tlDeflate

#include "tlCommon.h"

#include "tlException.h"

//  forward declarations - tlStream.h includes this header for the inline
//  byte-delivery fast path, so we must not include it here
namespace tl
{

class InputStream;
class OutputStream;

}

//  forware definition of the zlib stream structure - we can omit the zlib header here
struct z_stream_s;

namespace tl
{

/**
 *  @brief A Deflating filter, similar to InflateFilter
 *
 *  This filter can be used to produce a DEFLATE-compressed stream on an
 *  output stream. Similar to InflateFilter it is put between the source
 *  and an output stream.
 */
class TL_PUBLIC DeflateFilter
{
public:
  /**
   *  @brief Constructor: creates a filter in front of the output stream
   */
  DeflateFilter (tl::OutputStream &output);

  /**
   *  @brief Destructor
   *
   *  Note that this method will not flush the stream since that may 
   *  throw an exception. flush() has to be called explicitly.
   */
  ~DeflateFilter ();

  /**
   *  @brief Outputs a series of bytes into the deflated stream
   */
  void put (const char *b, size_t n);

  /**
   *  @brief Flushes the buffer and writes all remaining bytes
   *
   *  Note: this method must be called always before the stream 
   *  is closed and the filter is destroyed. Otherwise, the last bytes may be lost.
   */
  void flush ();

  /**
   *  @brief Get the uncompressed count collected so far
   */
  size_t uncompressed () const
  {
    return m_uc;
  }

  /**
   *  @brief Get the compressed count collected so far
   */
  size_t compressed () const
  {
    return m_cc;
  }

private:
  bool m_finished;
  char m_buffer[65536];
  tl::OutputStream *mp_output;
  z_stream_s *mp_stream;
  size_t m_uc, m_cc;
};

/**
 *  @brief The DEFLATE decompression (inflating) filter
 *
 *  This class is the main DEFLATE decoder. It is called "filter", since it takes bytes from
 *  the input (from the "input" stream) and delivers bytes on the output ("get" method). 
 */
class TL_PUBLIC InflateFilter
{
public:
  /**
   *  @brief Constructor
   *
   *  Constructs a filter attached to the given Stream object.
   */
  InflateFilter (tl::InputStream &input);

  /**
   *  @brief Destructor
   */
  ~InflateFilter ();

  /**
   *  @brief Get the next byte(s)
   *  
   *  This method returns a contiguous block of decoded bytes with the given length.
   *  The buffer grows as needed, so any length is available.
   */
  const char *get (size_t n);

  /**
   *  @brief Tell whether the buffer holds undelivered bytes
   */
  bool can_get_byte () const
  {
    return m_b_read != m_b_insert && ! m_at_end;
  }

  /**
   *  @brief Get a single byte through the fast path
   *
   *  Call this only when "can_get_byte" is true, otherwise use "get".
   */
  const char *get_byte ()
  {
    return m_buffer + m_b_read++;
  }

  /**
   *  @brief Undo the last "get" operation
   *
   *  This method ungets the last bytes obtained with the "get" method.
   *  the size to unget must match the last get's size.
   */
  void unget (size_t n);
  

  /**
   *  @brief The number of compressed bytes fetched from the input, but not decoded yet
   */
  size_t input_pending () const
  {
    return m_in_avail;
  }

  /**
   *  @brief Report true, if no more bytes can be delivered
   */
  bool at_end ();

private:
  tl::InputStream *mp_input;
  z_stream_s *mp_zs;

  //  compressed input fetched from the raw stream, not yet fed to zlib
  const char *mp_in;
  size_t m_in_avail;

  char *m_buffer;
  size_t m_blen;
  size_t m_b_insert;
  size_t m_b_read;
  bool m_at_end;

  //  zlib has reported the end of the DEFLATE stream
  bool m_z_end;

  bool process ();

};

}

#endif

