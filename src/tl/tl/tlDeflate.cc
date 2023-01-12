
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


#include "tlDeflate.h"
#include "tlException.h"
#include "tlAssert.h"

#include <algorithm>

#include <zlib.h>

namespace tl
{

// ------------------------------------------------------------------------
//  The Huffmann decoder core

/**
 *  @brief The decoder for Huffmann codes
 *
 *  The decoder keeps a Huffmann code tree and decodes a value from a bit stream
 *  using this tree. 
 *  As specified by RFC1951, the code tree is constructed from a list of code lengths
 *  vs. value alone.
 */
class HuffmannDecoder
{
public:
  /**
   *  @brief Constructor
   *  
   *  Creates an empty code tree.
   */
  HuffmannDecoder ()
  {
    mp_codes = 0;
    mp_bitmasks = 0;
    m_max_bits = 0;
    m_num_codes = 0;
  }

  /**
   *  @brief Destructor
   */
  ~HuffmannDecoder ()
  {
    if (mp_codes) {
      delete [] mp_codes;
    }
    mp_codes = 0;
    if (mp_bitmasks) {
      delete [] mp_bitmasks;
    }
    mp_bitmasks = 0;
  }

  /**
   *  @brief Initialize the code tree with the fixed Huffmann code table for literals/lengths
   *
   *  This table is used by compression mode 1.
   *  It is specified in RFC1951.
   */
  void fill_fixed_table_length ()
  {
    reserve (9);

    unsigned short lengths [288];
    for (unsigned int i = 0; i < 144; ++i) {
      lengths[i] = 8;
    }
    for (unsigned int i = 144; i < 256; ++i) {
      lengths[i] = 9;
    }
    for (unsigned int i = 256; i < 280; ++i) {
      lengths[i] = 7;
    }
    for (unsigned int i = 280; i < 288; ++i) {
      lengths[i] = 8;
    }

    init_codes (lengths, lengths + sizeof (lengths) / sizeof (lengths [0]));
  }

  /**
   *  @brief Initialize the code tree with the fixed Huffmann code table for distances
   *
   *  This table is used by compression mode 1.
   *  It is specified in RFC1951.
   */
  void fill_fixed_table_dist ()
  {
    reserve (5);

    unsigned short lengths [32];
    for (unsigned int i = 0; i < 32; ++i) {
      lengths[i] = 5;
    }
    init_codes (lengths, lengths + sizeof (lengths) / sizeof (lengths [0]));
  }

  /**
   *  @brief Initialize the code tree from a list of lengths
   *
   *  This method initializes the code tree from a list of lengths, given 
   *  by the sequence [begin_lengths, end_lengths). The codes are assumed to 
   *  range from 0 to distance(begin_lengths, end_lengths).
   *  See RFC1951 for a description about the procedure.
   */
  template <class Iter>
  void init_codes (Iter begin_lengths, Iter end_lengths)
  {
    const unsigned int MAX_BITS = 16;
    unsigned short bl_count[MAX_BITS + 1];
    unsigned short bitmasks[MAX_BITS + 1];
    unsigned short next_code[MAX_BITS + 1];
    unsigned int max_bits = 0;

    for (unsigned int bits = 0; bits <= MAX_BITS; bits++) {
      bl_count[bits] = 0;
    }

    for (Iter l = begin_lengths; l != end_lengths; ++l) {
      tl_assert (*l < MAX_BITS);
      if (*l > 0) {
        ++bl_count [*l];
      }
    }

    unsigned int code = 0;
    for (unsigned int bits = 1; bits <= MAX_BITS; bits++) {
      if (bl_count[bits - 1] > 0) {
        max_bits = bits - 1;
      }
      code = (code + bl_count[bits - 1]) << 1;
      next_code[bits] = code;
    }

    for (unsigned int bits = 0; bits <= max_bits; bits++) {
      bitmasks [bits] = ((1 << bits) - 1) << (max_bits - bits);
    }

    reserve (max_bits);

    unsigned short symbol = 0;
    for (Iter l = begin_lengths; l != end_lengths; ++l, ++symbol) {
      if (*l > 0) {
        unsigned int code = next_code [*l]++;
        code <<= (max_bits - *l);
        mp_codes [code] = symbol;
        mp_bitmasks [code] = bitmasks [*l];
      } 
    }
  }

  /**
   *  @brief Decode the next value from a bit stream
   *
   *  This method takes the next value from the bit stream decoding the bits with
   *  the code tree currently loaded.
   */
  unsigned short decode (BitStream &s) const
  {
    tl_assert (mp_codes != 0);

    unsigned int m = m_num_codes / 2;
    
    unsigned int c = 0;
    do {
      if (s.get_bit ()) {
        c |= m;
      }
      m >>= 1;
    } while ((mp_bitmasks [c] & m) != 0);

    return mp_codes [c];
  }

private:
  unsigned short *mp_codes, *mp_bitmasks;
  unsigned int m_num_codes, m_max_bits;

  void reserve (unsigned int max_bits)
  {
    m_num_codes = 1 << max_bits;
    if (max_bits > m_max_bits) {
      m_max_bits = max_bits;
      if (mp_codes) {
        delete [] mp_codes;
      }
      mp_codes = new unsigned short [m_num_codes];
      if (mp_bitmasks) {
        delete [] mp_bitmasks;
      }
      mp_bitmasks = new unsigned short [m_num_codes];
    }
  }
};


// ------------------------------------------------------------------------
//  InflateFilter implementation

InflateFilter::InflateFilter (tl::InputStream &input)
  : m_input (input), 
    m_b_insert (0), m_b_read (0), m_at_end (false),
    m_last_block (false), 
    m_uncompressed_length (0)  //  this forces a new block on "process()"
{
  for (size_t i = 0; i < sizeof (m_buffer) / sizeof (m_buffer [0]); ++i) {
    m_buffer[i] = 0;
  }

  mp_dist_decoder = new HuffmannDecoder ();
  mp_lit_decoder = new HuffmannDecoder ();
}

InflateFilter::~InflateFilter ()
{
  delete mp_dist_decoder;
  mp_dist_decoder = 0;
  delete mp_lit_decoder;
  mp_lit_decoder = 0;
}

const char * 
InflateFilter::get (size_t n)
{
  tl_assert (n < sizeof (m_buffer) / 2);

  while ((m_b_insert + sizeof (m_buffer) - m_b_read) % sizeof (m_buffer) < n) {
    if (! process ()) {
      throw tl::Exception (tl::to_string (tr ("Unexpected end of file (DEFLATE implementation)")));
    }
  }

  tl_assert (m_b_read != m_b_insert);

  //  ensure the block is accessible as a coherent chunk:
  if (m_b_read + n >= sizeof (m_buffer)) {
    std::rotate (m_buffer, m_buffer + m_b_read, m_buffer + sizeof (m_buffer));
    m_b_insert = (m_b_insert - m_b_read + sizeof (m_buffer)) % sizeof (m_buffer);
    m_b_read = 0;
  }

  const char *r = m_buffer + m_b_read;
  m_b_read = (m_b_read + n) % sizeof (m_buffer);
  return r;
}

void
InflateFilter::unget (size_t n)
{
  tl_assert (m_b_read >= n);
  m_b_read -= (unsigned int) n;
}

bool 
InflateFilter::at_end () 
{
  if (! m_at_end && m_b_read == m_b_insert) {
    if (! process ()) {
      m_at_end = true;
    }
  }
  return m_at_end;
}

void 
InflateFilter::put_byte (char b) 
{
  m_buffer [m_b_insert] = b;
  m_b_insert = (m_b_insert + 1) % sizeof (m_buffer);
}

void 
InflateFilter::put_byte_dist (unsigned int d) 
{
  put_byte (m_buffer [(m_b_insert - d) % sizeof (m_buffer)]);
}

bool 
InflateFilter::process ()
{
  while (true) {

    bool new_block = false;

    if (m_uncompressed_length == 0) {

      m_uncompressed_length = -1;
      new_block = true;

    } else if (m_uncompressed_length > 0) {

      put_byte (m_input.get_byte ());
      --m_uncompressed_length;

    } else {

      unsigned int l = mp_lit_decoder->decode (m_input);
      if (l < 256) {

        put_byte (char (l));

      } else if (l == 256) {

        new_block = true;

      } else {

        unsigned int length = 0;
        if (l < 265) {
          length = l - 254;
        } else if (l < 269) {
          length = (l - 265) * 2 + 11 + m_input.get_bits (1);
        } else if (l < 273) {
          length = (l - 269) * 4 + 19 + m_input.get_bits (2);
        } else if (l < 277) {
          length = (l - 273) * 8 + 35 + m_input.get_bits (3);
        } else if (l < 281) {
          length = (l - 277) * 16 + 67 + m_input.get_bits (4);
        } else if (l < 285) {
          length = (l - 281) * 32 + 131 + m_input.get_bits (5);
        } else {
          length = 258;
        }

        unsigned int d = mp_dist_decoder->decode (m_input);
        unsigned int dist = 0;
        if (d < 4) {
          dist = d + 1;
        } else if (d < 6) {
          dist = (d - 4) * 2 + 5 + m_input.get_bits (1);
        } else if (d < 8) {
          dist = (d - 6) * 4 + 9 + m_input.get_bits (2);
        } else if (d < 10) {
          dist = (d - 8) * 8 + 17 + m_input.get_bits (3);
        } else if (d < 12) {
          dist = (d - 10) * 16 + 33 + m_input.get_bits (4);
        } else if (d < 14) {
          dist = (d - 12) * 32 + 65 + m_input.get_bits (5);
        } else if (d < 16) {
          dist = (d - 14) * 64 + 129 + m_input.get_bits (6);
        } else if (d < 18) {
          dist = (d - 16) * 128 + 257 + m_input.get_bits (7);
        } else if (d < 20) {
          dist = (d - 18) * 256 + 513 + m_input.get_bits (8);
        } else if (d < 22) {
          dist = (d - 20) * 512 + 1025 + m_input.get_bits (9);
        } else if (d < 24) {
          dist = (d - 22) * 1024 + 2049 + m_input.get_bits (10);
        } else if (d < 26) {
          dist = (d - 24) * 2048 + 4097 + m_input.get_bits (11);
        } else if (d < 28) {
          dist = (d - 26) * 4096 + 8193 + m_input.get_bits (12);
        } else {
          dist = (d - 28) * 8192 + 16385 + m_input.get_bits (13);
        }

        while (length-- > 0) {
          put_byte_dist (dist);
        }

      }

    }

    if (new_block) {

      if (m_last_block) {
        return false;
      }

      //  read new block header
      m_last_block = m_input.get_bit ();
      unsigned int t = m_input.get_bits (2);

      if (t == 0) {

        //  uncompressed data
        m_input.skip_to_byte ();
        m_uncompressed_length = m_input.get_bits (16);
        m_input.get_bits (16);

      } else if (t == 1 || t == 2) {
        
        if (t == 1) {

          //  KLUDGE: should use a different decoder object, so we save time to do this:
          mp_lit_decoder->fill_fixed_table_length ();
          mp_dist_decoder->fill_fixed_table_dist ();

        } else {

          unsigned int hlit = m_input.get_bits (5) + 257;
          unsigned int hdist = m_input.get_bits (5) + 1;
          unsigned int hclen = m_input.get_bits (4) + 4;

          unsigned int hclengths [19];
          for (unsigned int i = 0; i < sizeof (hclengths) / sizeof (hclengths [0]); ++i) {
            hclengths [i] = 0;
          }

          static unsigned int hclen_order [] = {
            16, 17, 18, 0,   8,  7,  9,  6,  10,  5, 11,  4,  12,  3, 13,  2, 
            14,  1, 15
          };
          for (unsigned int i = 0; i < hclen; ++i) {
            hclengths [hclen_order [i]] = m_input.get_bits (3);
          }

          HuffmannDecoder ldecoder;
          ldecoder.init_codes (hclengths, hclengths + sizeof (hclengths) / sizeof (hclengths[0]));

          unsigned int lengths [286 + 32];
          unsigned int nlengths = hlit + hdist;

          for (unsigned int i = 0; i < nlengths; ) {

            unsigned short l = ldecoder.decode (m_input);
            if (l < 16) {
              lengths [i++] = l;
            } else if (l == 16) {
              unsigned int n = m_input.get_bits (2) + 3;
              tl_assert (i > 0);
              l = lengths [i - 1];
              while (n-- > 0) {
                tl_assert (i < nlengths);
                lengths [i++] = l;
              }
            } else if (l == 17) {
              unsigned int n = m_input.get_bits (3) + 3;
              while (n-- > 0) {
                tl_assert (i < nlengths);
                lengths [i++] = 0;
              }
            } else if (l == 18) {
              unsigned int n = m_input.get_bits (7) + 11;
              while (n-- > 0) {
                tl_assert (i < nlengths);
                lengths [i++] = 0;
              }
            } else {
              tl_assert (false);
            }

          }

          mp_lit_decoder->init_codes (lengths, lengths + hlit);
          mp_dist_decoder->init_codes (lengths + hlit, lengths + nlengths);

        }

      } else {
        throw tl::Exception (tl::to_string (tr ("Invalid compression type: %d")), t);
      }

    } else {
      return true;
    }

  }
}

// ------------------------------------------------------------------------
//  DeflateFilter implementation
//  This implementation is based on the zlib

DeflateFilter::DeflateFilter (tl::OutputStream &output)
  : m_finished (false), mp_output (&output), m_uc (0), m_cc (0)
{
  mp_stream = new z_stream ();
  mp_stream->zalloc = (alloc_func)0;
  mp_stream->zfree = (free_func)0;
  mp_stream->opaque = (voidpf)0;
  mp_stream->next_in = (Byte *)0;
  mp_stream->avail_in = 0;
  mp_stream->next_out = (Byte *)m_buffer;
  mp_stream->avail_out = sizeof (m_buffer);

  int err = deflateInit2 (mp_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15 /* == raw deflate data*/, 8 /* == default memory level */, Z_DEFAULT_STRATEGY);
  tl_assert (err == Z_OK);
}

DeflateFilter::~DeflateFilter ()
{
  delete mp_stream;
}

void
DeflateFilter::put (const char *b, size_t n)
{
  m_uc += n;

  mp_stream->next_in = (Byte *)b;
  mp_stream->avail_in = (unsigned int) n;

  while (mp_stream->avail_in > 0) {

    int err = deflate (mp_stream, Z_NO_FLUSH);
    tl_assert (err == Z_OK);

    if (mp_stream->avail_out == 0) {
      m_cc += sizeof (m_buffer);
      mp_output->put (m_buffer, sizeof (m_buffer));
      mp_stream->next_out = (Byte *)m_buffer;
      mp_stream->avail_out = sizeof (m_buffer);
    }

  }
}

void
DeflateFilter::flush ()
{
  while (true) {

    int err = deflate (mp_stream, Z_FINISH);
    tl_assert (err == Z_OK || err == Z_STREAM_END);

    m_cc += sizeof (m_buffer) - mp_stream->avail_out;
    mp_output->put (m_buffer, sizeof (m_buffer) - mp_stream->avail_out);
    mp_stream->next_out = (Byte *)m_buffer;
    mp_stream->avail_out = sizeof (m_buffer);

    if (err == Z_STREAM_END) {
      break;
    } 

  }

  int err = deflateEnd (mp_stream);
  tl_assert (err == Z_OK);

  mp_output->flush ();
  m_finished = true;
}

}

