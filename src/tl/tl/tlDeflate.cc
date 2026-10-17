
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


#include "tlDeflate.h"
#include "tlStream.h"
#include "tlException.h"
#include "tlAssert.h"

#include <cstring>

#include <zlib.h>

namespace tl
{

// ------------------------------------------------------------------------
//  InflateFilter implementation
//  This implementation is based on the zlib, like the DeflateFilter below.
//  zlib keeps its own 32k window, so the output buffer is a plain linear
//  buffer of undelivered bytes - no ring and no lookback are needed.

InflateFilter::InflateFilter (tl::InputStream &input)
  : mp_input (&input),
    mp_zs (new z_stream ()),
    mp_in (0), m_in_avail (0),
    m_b_insert (0), m_b_read (0), m_at_end (false),
    m_z_end (false)
{
  mp_zs->zalloc = (alloc_func) 0;
  mp_zs->zfree = (free_func) 0;
  mp_zs->opaque = (voidpf) 0;

  //  -15 selects raw DEFLATE data without the zlib header
  if (inflateInit2 (mp_zs, -15) != Z_OK) {
    delete mp_zs;
    mp_zs = 0;
    throw tl::Exception (tl::to_string (tr ("Unable to initialize DEFLATE decompression")));
  }

  m_blen = 65536;
  m_buffer = new char [m_blen];
  std::memset (m_buffer, 0, m_blen);
}

InflateFilter::~InflateFilter ()
{
  if (mp_zs) {
    inflateEnd (mp_zs);
    delete mp_zs;
    mp_zs = 0;
  }

  delete[] m_buffer;
  m_buffer = 0;
  m_blen = 0;
}

const char *
InflateFilter::get (size_t n)
{
  size_t blen = m_blen;
  while (n >= blen / 2)  {
    blen *= 2;
  }

  //  buffer needs to be enlarged - reallocate and move the undelivered bytes
  if (blen != m_blen) {

    char *new_buffer = new char[blen];
    std::memcpy (new_buffer, m_buffer + m_b_read, m_b_insert - m_b_read);

    delete[] m_buffer;
    m_buffer = new_buffer;
    m_blen = blen;
    m_b_insert -= m_b_read;
    m_b_read = 0;

  }

  while (size_t (m_b_insert - m_b_read) < n) {
    if (! process ()) {
      throw tl::Exception (tl::to_string (tr ("Unexpected end of file (DEFLATE implementation)")));
    }
  }

  const char *r = m_buffer + m_b_read;
  m_b_read += n;
  return r;
}

void
InflateFilter::unget (size_t n)
{
  tl_assert (m_b_read >= n);
  m_b_read -= n;
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

bool
InflateFilter::process ()
{
  //  Produces new output into the buffer. Returns true if something was
  //  produced, false if the DEFLATE stream has ended and no output is left.

  if (m_z_end) {
    return false;
  }

  while (true) {

    //  keep a large enough tail space for the next inflate call
    if (m_blen - m_b_insert < m_blen / 2 && m_b_read > 0) {
      std::memmove (m_buffer, m_buffer + m_b_read, m_b_insert - m_b_read);
      m_b_insert -= m_b_read;
      m_b_read = 0;
    }

    if (m_in_avail == 0) {

      //  fetch the next block of compressed input. get() refuses to deliver
      //  less than asked for, so step the request down until the tail of the
      //  raw stream is covered too.
      for (size_t n = 65536; n >= 1; n /= 2) {
        mp_in = mp_input->get (n, true /*bypass_inflate*/);
        if (mp_in) {
          m_in_avail = n;
          break;
        }
      }

      if (m_in_avail == 0) {
        throw tl::Exception (tl::to_string (tr ("Unexpected end of file (DEFLATE implementation)")));
      }

    }

    mp_zs->next_in = (Bytef *) mp_in;
    mp_zs->avail_in = (uInt) m_in_avail;
    mp_zs->next_out = (Bytef *) (m_buffer + m_b_insert);
    mp_zs->avail_out = (uInt) (m_blen - m_b_insert);

    int r = inflate (mp_zs, Z_NO_FLUSH);

    m_in_avail = mp_zs->avail_in;
    mp_in = (const char *) mp_zs->next_in;

    size_t produced = (m_blen - m_b_insert) - mp_zs->avail_out;
    m_b_insert += produced;

    if (r == Z_STREAM_END) {

      //  push the input zlib did not consume back into the raw stream, so
      //  reading continues right behind the DEFLATE block
      if (m_in_avail > 0) {
        mp_input->unget (m_in_avail, true /*bypass_inflate*/);
        m_in_avail = 0;
        mp_in = 0;
      }

      m_z_end = true;
      return produced > 0;

    } else if (r != Z_OK) {
      throw tl::Exception (tl::to_string (tr ("DEFLATE decompression error")) + (mp_zs->msg ? std::string (": ") + mp_zs->msg : std::string ()));
    }

    if (produced > 0) {
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

