
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

#include <algorithm>

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


//  Byte-wise reads through InputStream with a get/unget pair per byte, as the
//  OASIS reader does inside CBLOCKs. The data is larger than the 64k ring buffer,
//  so bytes are taken from every ring position including the last one.
TEST(4)
{
  std::string data;
  size_t r = 1;
  for (size_t i = 0; i < 300000; ++i) {
    r *= 12361;
    r ^= (r >> 8);
    data += "abcd" [r % 4];
  }

  tl::OutputStringStream oss;
  {
    tl::OutputStream os (oss);
    tl::DeflateFilter fg (os);
    fg.put (data.c_str (), data.size ());
    fg.flush ();
  }

  std::string deflated = oss.string ();
  tl::InputMemoryStream ims (deflated.c_str (), deflated.size ());
  tl::InputStream is (ims);
  is.inflate (true);

  std::string out;
  const char *c;
  while ((c = is.get (1)) != 0) {
    char ch = *c;
    is.unget (1);
    c = is.get (1);
    EXPECT_EQ (c != 0 && *c == ch, true);
    out += ch;
  }

  EXPECT_EQ (out.size (), data.size ());
  EXPECT_EQ (out == data, true);
}

static std::string
random_data (size_t n)
{
  std::string data;
  size_t r = 1;
  for (size_t i = 0; i < n; ++i) {
    r *= 12361;
    r ^= (r >> 8);
    data += "abcd" [r % 4];
  }
  return data;
}

static std::string
deflate_string (const std::string &data)
{
  tl::OutputStringStream oss;
  {
    tl::OutputStream os (oss);
    tl::DeflateFilter fg (os);
    fg.put (data.c_str (), data.size ());
    fg.flush ();
  }
  return oss.string ();
}

//  Raw data behind the DEFLATE stream, as in OASIS CBLOCKs: reading continues
//  exactly behind the compressed block
TEST(5)
{
  std::string data = random_data (300000);
  std::string deflated = deflate_string (data);
  std::string tail = "raw tail";
  std::string all = deflated + tail;

  {
    tl::InputMemoryStream ims (all.c_str (), all.size ());
    tl::InputStream is (ims);
    tl::InflateFilter f (is);

    std::string out;
    while (! f.at_end ()) {
      out += f.get (1) [0];
    }

    EXPECT_EQ (out == data, true);
    EXPECT_EQ (is.pos (), deflated.size ());

    const char *t = is.get (tail.size ());
    EXPECT_EQ (t != 0 && std::string (t, tail.size ()) == tail, true);
    EXPECT_EQ (is.get (1) == 0, true);
  }

  //  same through InputStream in inflate mode with mixed read sizes
  {
    tl::InputMemoryStream ims (all.c_str (), all.size ());
    tl::InputStream is (ims);
    is.inflate ();

    std::string out;
    size_t n = 1;
    size_t max_pos = 0;
    while (out.size () < data.size ()) {
      n = std::min (n % 1000 + 1, data.size () - out.size ());
      const char *b = is.get (n);
      EXPECT_EQ (b != 0, true);
      if (! b) {
        break;
      }
      out += std::string (b, n);
      max_pos = std::max (max_pos, is.pos ());
      if (out.size () == n) {
        //  pos () follows the compressed bytes actually consumed
        EXPECT_EQ (is.pos () < deflated.size () / 2, true);
      }
    }

    EXPECT_EQ (max_pos <= deflated.size (), true);

    EXPECT_EQ (out == data, true);

    const char *t = is.get (tail.size ());
    EXPECT_EQ (t != 0 && std::string (t, tail.size ()) == tail, true);
    EXPECT_EQ (is.pos (), all.size ());
    EXPECT_EQ (is.get (1) == 0, true);
  }
}

//  Truncated DEFLATE stream
TEST(6)
{
  std::string deflated = deflate_string (random_data (100000));
  deflated.resize (deflated.size () / 2);

  tl::InputMemoryStream ims (deflated.c_str (), deflated.size ());
  tl::InputStream is (ims);
  tl::InflateFilter f (is);

  bool error = false;
  try {
    while (! f.at_end ()) {
      f.get (1);
    }
  } catch (tl::Exception &) {
    error = true;
  }

  EXPECT_EQ (error, true);
}
