
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


#ifndef HDR_tlDeflate
#define HDR_tlDeflate

#include "tlCommon.h"

#include "tlStream.h"
#include "tlException.h"

//  forware definition of the zlib stream structure - we can omit the zlib header here
struct z_stream_s;

namespace tl
{

class HuffmannDecoder;

/**
 *  @brief A bit stream reader according to the DEFLATE specification
 *
 *  This filter reads bytes from a tl::Stream and delivers bits, taken from
 *  these bytes. The bits are delivered in the order specified by the DEFLATE
 *  format specification (least significant bit first).
 */
class TL_PUBLIC BitStream
{
public:
  /**
   *  @brief Constructor
   *
   *  Constructs a bit stream filter attached to the given stream
   */
  BitStream (tl::InputStream &input)
    : mp_input (&input),
      m_mask (0), m_byte (0)
  {
    // ...
  }

  /**
   *  @brief Get a byte
   *
   *  This method simply passes the byte.
   *  The method expects the next byte to be available.
   */
  unsigned char get_byte ()
  {
    m_mask = 0;
    const char *c = mp_input->get (1, true /*bypass_deflate*/);
    if (c == 0) {
      throw tl::Exception (tl::to_string (tr ("Unexpected end of file (DEFLATE implementation)")));
    }
    return *c;
  }

  /**
   *  @brief Get a single bit
   *
   *  This method gets the next bit available.
   *  The method expects the next byte to be available, if one needs to be read.
   */
  bool get_bit ()
  {
    if (m_mask == 0) {
      m_byte = get_byte ();
      m_mask = 0x01;
    } 
    bool b = ((m_byte & m_mask) != 0);
    m_mask <<= 1;
    return b;
  }

  /**
   *  @brief Get a sequence of bits
   *
   *  This method gets the next n bits and delivers them as a single unsigned int,
   *  packing the first bit into the least signification bit. This is the specification
   *  for reading multiple bit values except Huffmann codes.
   */
  unsigned int get_bits (unsigned int n)
  {
    //  KLUDGE: take directly from the byte for performance.
    unsigned int r = 0;
    unsigned int m = 1;
    while (n-- > 0) {
      r |= get_bit () ? m : 0;
      m <<= 1;
    }
    return r;
  }

  /**
   *  @brief Skip the next bits up to the next byte boundary
   */
  void skip_to_byte ()
  {
    m_mask = 0;
  }

private:
  tl::InputStream *mp_input;
  unsigned char m_mask;
  unsigned char m_byte;
};


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
   *  The maximum size of the block available is half the buffer size.
   */
  const char *get (size_t n);

  /**
   *  @brief Undo the last "get" operation
   *
   *  This method ungets the last bytes obtained with the "get" method.
   *  the size to unget must match the last get's size.
   */
  void unget (size_t n);
  

  /**
   *  @brief Report true, if no more bytes can be delivered
   */
  bool at_end ();

private:
  BitStream m_input;

  char m_buffer[65536];
  unsigned int m_b_insert;
  unsigned int m_b_read;
  bool m_at_end;

  //  processor state
  bool m_last_block;
  int m_uncompressed_length;
  HuffmannDecoder *mp_lit_decoder, *mp_dist_decoder;

  void put_byte (char b);
  void put_byte_dist (unsigned int d);
  bool process ();

};

}

#endif

