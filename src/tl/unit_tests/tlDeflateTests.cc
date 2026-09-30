
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


#include "tlStream.h"
#include "tlDeflate.h"
#include "tlUnitTest.h"

#include "zlib.h"

#include <string>
#include <vector>

static std::string
make_random (size_t n)
{
  std::string d;
  d.reserve (n);
  unsigned int r = 12345;
  for (size_t i = 0; i < n; ++i) {
    r = r * 1103515245u + 12345u;
    d += char ((r >> 16) & 0xff);
  }
  return d;
}

static std::string
deflate_data (const std::string &data)
{
  tl::OutputStringStream oss;
  tl::OutputStream os (oss);
  tl::DeflateFilter fg (os);
  fg.put (data.c_str (), data.size ());
  fg.flush ();
  return oss.string ();
}

TEST(1) 
{
  unsigned char data[] = {
    // gzip header:
    // 0x1f, 0x8b, 0x08, 0x08, 
    // 0xed, 0x11, 0x07, 0x50, 
    // 0x00, 0x03, 
    // 0x78, 0x00, 
    0x0b, 0xc9, 0xc8, 0x2c,
    0x56, 0x00, 0xa2, 0x44, 
    0x85, 0x92, 0xd4, 0xe2, 
    0x12, 0x85, 0x18, 0x45, 
    0x2e, 0x00,
    // gzip tail (8 bytes):
    // 0x20, 0xc7, 0x43, 0x6a,  CRC32
    // 0x12, 0x00, 0x00, 0x00   uncompressed file size
  };

  tl::InputMemoryStream ims ((const char *) data, sizeof (data));
  tl::InputStream is (ims);
  
  std::string out;
  tl::InflateFilter f (is);
  while (! f.at_end ()) {
    out += f.get (1) [0];
  }

  EXPECT_EQ (out, "This is a test \\!\n");
}

TEST(2)
{
  const char hello[] = "This is a test \\!";

  tl::OutputStringStream oss;
  tl::OutputStream os (oss);
  tl::DeflateFilter fg (os);
  fg.put (hello, sizeof (hello) - 1);
  fg.flush ();

  std::string deflated = oss.string ();
  for (size_t i = 0; i < deflated.size(); ++i) {
  }
  tl::InputMemoryStream ims ((const char *) deflated.c_str (), deflated.size ());
  tl::InputStream is (ims);
  
  std::string out;
  tl::InflateFilter f (is);
  while (! f.at_end ()) {
    out += f.get (1) [0];
  }

  EXPECT_EQ (out, "This is a test \\!");
}

//  Big deflate:
TEST(3)
{
  size_t n_hello = 1024*1024;
  char *hello = new char[n_hello + 1];
  hello[n_hello] = 0;
  size_t r = 1;
  for (size_t i = 0; i < n_hello; ++i) {
    r *= 12361;
    r ^= (r >> 8); 
    hello [i] = "abc" [r % 3];
  }

  tl::OutputStringStream oss;
  tl::OutputStream os (oss);
  tl::DeflateFilter fg (os);
  fg.put (hello, n_hello);
  fg.flush ();

  std::string deflated = oss.string ();
  EXPECT_EQ (deflated.size () < 300000 && deflated.size () > 200000, true);
  EXPECT_EQ (deflated.size (), fg.compressed ());
  EXPECT_EQ (n_hello, fg.uncompressed ());
  tl::InputMemoryStream ims ((const char *) deflated.c_str (), deflated.size ());
  tl::InputStream is (ims);
  
  std::string out;
  tl::InflateFilter f (is);
  while (! f.at_end ()) {
    out += f.get (1) [0];
  }

  EXPECT_EQ (out, hello);

  delete[] hello;
}

//  inflate_block: round trip over several sizes, with trailing raw bytes
TEST(4)
{
  const size_t sizes[] = { 1, 1000, 100000, 1000000 };

  for (size_t i = 0; i < sizeof (sizes) / sizeof (sizes[0]); ++i) {

    size_t n = sizes [i];

    std::string data = make_random (n);
    std::string deflated = deflate_data (data);

    //  bytes which must be served raw after the decompressed block
    std::string trailing = "some bytes after the compressed block";
    std::string all = deflated + trailing;

    tl::InputMemoryStream ims (all.c_str (), all.size ());
    tl::InputStream is (ims);

    is.inflate_block (deflated.size (), data.size ());

    //  the position is right after the compressed block
    EXPECT_EQ (is.pos (), deflated.size ());

    //  get/unget works inside the decompressed block
    const char *b0 = is.get (1);
    EXPECT_EQ (*b0, data [0]);
    is.unget (1);
    const char *b1 = is.get (1);
    EXPECT_EQ (*b1, data [0]);

    std::string out;
    out += *b1;
    while (out.size () < data.size ()) {
      size_t nn = std::min (size_t (65536), data.size () - out.size ());
      const char *b = is.get (nn);
      EXPECT_EQ (b != 0, true);
      out.append (b, nn);
    }

    EXPECT_EQ (out, data);

    //  the last block byte can be ungotten even though the block is exhausted
    is.unget (1);
    const char *lb = is.get (1);
    EXPECT_EQ (*lb, data [data.size () - 1]);

    //  peek does not end the block mode: a pending unget stays valid
    size_t navail = 0;
    EXPECT_EQ (is.peek (navail) != 0, true);
    is.unget (1);
    const char *lb2 = is.get (1);
    EXPECT_EQ (*lb2, data [data.size () - 1]);

    //  after the block, the raw bytes are served
    const char *t = is.get (trailing.size ());
    EXPECT_EQ (t != 0, true);
    EXPECT_EQ (std::string (t, trailing.size ()), trailing);
    EXPECT_EQ (is.pos (), all.size ());

  }
}

//  inflate_block: error cases
TEST(5)
{
  std::string data = make_random (10000);
  std::string deflated = deflate_data (data);
  std::string trailing = "trailing";

  //  truncated compressed data is an error
  {
    std::string half = deflated.substr (0, deflated.size () / 2);
    tl::InputMemoryStream ims (half.c_str (), half.size ());
    tl::InputStream is (ims);

    bool error = false;
    try {
      is.inflate_block (deflated.size (), data.size ());
    } catch (tl::Exception &) {
      error = true;
    }
    EXPECT_EQ (error, true);
  }

  //  reading over the end of the decompressed block is an error
  //  (the streaming inflate filter behaves the same way)
  {
    std::string all = deflated + trailing;
    tl::InputMemoryStream ims (all.c_str (), all.size ());
    tl::InputStream is (ims);
    is.inflate_block (deflated.size (), data.size ());

    bool error = false;
    try {
      is.get (data.size () + 1);
    } catch (tl::Exception &) {
      error = true;
    }
    EXPECT_EQ (error, true);
  }

  //  a comp-byte-count of 0 falls back to the streaming decoder
  {
    std::string all = deflated + trailing;
    tl::InputMemoryStream ims (all.c_str (), all.size ());
    tl::InputStream is (ims);
    is.inflate_block (0, data.size ());

    std::string out = is.read_all (data.size ());
    EXPECT_EQ (out, data);

    const char *t = is.get (trailing.size ());
    EXPECT_EQ (t != 0, true);
    EXPECT_EQ (std::string (t, trailing.size ()), trailing);
  }
}

//  inflate_block: lenient byte counts
TEST(7)
{
  std::string data = make_random (10000);
  std::string deflated = deflate_data (data);
  std::string trailing = "trailing";

  //  comp-byte-count is authoritative: padding between the DEFLATE stream
  //  and the end of the block is skipped
  {
    std::string all = deflated + std::string (4, '\0') + trailing;
    tl::InputMemoryStream ims (all.c_str (), all.size ());
    tl::InputStream is (ims);

    is.inflate_block (deflated.size () + 4, data.size ());

    EXPECT_EQ (is.read_all (data.size ()), data);
    //  the next record starts right after the padding
    EXPECT_EQ (is.pos (), deflated.size () + 4);

    const char *t = is.get (trailing.size ());
    EXPECT_EQ (t != 0, true);
    EXPECT_EQ (std::string (t, trailing.size ()), trailing);
  }

  //  uncomp-byte-count too small: what the DEFLATE stream produces wins
  {
    std::string all = deflated + trailing;
    tl::InputMemoryStream ims (all.c_str (), all.size ());
    tl::InputStream is (ims);
    is.inflate_block (deflated.size (), data.size () - 1);

    EXPECT_EQ (is.read_all (data.size ()), data);

    const char *t = is.get (trailing.size ());
    EXPECT_EQ (std::string (t, trailing.size ()), trailing);
  }

  //  uncomp-byte-count too large
  {
    std::string all = deflated + trailing;
    tl::InputMemoryStream ims (all.c_str (), all.size ());
    tl::InputStream is (ims);
    is.inflate_block (deflated.size (), data.size () + 1000);

    EXPECT_EQ (is.read_all (data.size ()), data);

    const char *t = is.get (trailing.size ());
    EXPECT_EQ (std::string (t, trailing.size ()), trailing);
  }

  //  a huge uncomp-byte-count is a hint only: no huge allocation, and the
  //  data is still read correctly
  {
    std::string all = deflated + trailing;
    tl::InputMemoryStream ims (all.c_str (), all.size ());
    tl::InputStream is (ims);
    is.inflate_block (deflated.size (), 0x40000000);

    EXPECT_EQ (is.read_all (data.size ()), data);

    const char *t = is.get (trailing.size ());
    EXPECT_EQ (std::string (t, trailing.size ()), trailing);
  }
}

//  inflate_block: read_all over the block boundary
TEST(6)
{
  std::string data = make_random (50000);
  std::string deflated = deflate_data (data);
  std::string trailing = make_random (3000);

  std::string all = deflated + trailing;
  tl::InputMemoryStream ims (all.c_str (), all.size ());
  tl::InputStream is (ims);

  is.inflate_block (deflated.size (), data.size ());

  //  a partial read from the decompressed block
  EXPECT_EQ (is.read_all (12345), data.substr (0, 12345));

  //  the block remainder plus the trailing raw bytes
  EXPECT_EQ (is.read_all (), data.substr (12345) + trailing);
}

