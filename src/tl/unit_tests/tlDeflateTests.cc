
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


#include "tlStream.h"
#include "tlDeflate.h"
#include "tlUnitTest.h"

#include "zlib.h"

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

