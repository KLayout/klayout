
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
#include "tlUnitTest.h"

std::string s2string (tl::OutputMemoryStream &osm)
{
  std::string res;
  size_t n = osm.size ();
  const char *d = osm.data ();
  for (size_t i = 0; i < n; ++i, ++d) {
    if (i > 0) {
      res += ",";
    }
    res += tl::sprintf ("0x%02x", int ((unsigned char) *d));
  }
  return res;
}

TEST(BinaryStreams1)
{
  tl::OutputMemoryStream osm;
  tl::BinaryOutputStream os (osm);

  os << (double) 0.17;

  os.flush ();
  EXPECT_EQ (s2string (osm), "0xc3,0xf5,0x28,0x5c,0x8f,0xc2,0xc5,0x3f");

  tl::InputMemoryStream ism (osm.data (), osm.size ());
  tl::InputStream is (ism);
  tl::BinaryInputStream bis (is);

  double x = 0.0;
  bis >> x;

  EXPECT_EQ (tl::to_string (x), "0.17");
}

TEST(BinaryStreams2)
{
  tl::OutputMemoryStream osm;
  tl::BinaryOutputStream os (osm);

  os << (float) 0.17;

  os.flush ();
  EXPECT_EQ (s2string (osm), "0x7b,0x14,0x2e,0x3e");

  tl::InputMemoryStream ism (osm.data (), osm.size ());
  tl::InputStream is (ism);
  tl::BinaryInputStream bis (is);

  float x = 0.0;
  bis >> x;

  EXPECT_EQ (tl::to_string (x), "0.17");
}

TEST(BinaryStreams3)
{
  tl::OutputMemoryStream osm;
  tl::BinaryOutputStream os (osm);

  os << std::string ("ABC");

  os.flush ();
  EXPECT_EQ (s2string (osm), "0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x41,0x42,0x43");

  tl::InputMemoryStream ism (osm.data (), osm.size ());
  tl::InputStream is (ism);
  tl::BinaryInputStream bis (is);

  std::string x;
  bis >> x;

  EXPECT_EQ (x, "ABC");
}

TEST(BinaryStreams4)
{
  tl::OutputMemoryStream osm;
  tl::BinaryOutputStream os (osm);

  os << "ABC";

  os.flush ();
  EXPECT_EQ (s2string (osm), "0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x41,0x42,0x43");

  tl::InputMemoryStream ism (osm.data (), osm.size ());
  tl::InputStream is (ism);
  tl::BinaryInputStream bis (is);

  std::string x;
  bis >> x;

  EXPECT_EQ (x, "ABC");
}

TEST(BinaryStreams5)
{
  tl::OutputMemoryStream osm;
  tl::BinaryOutputStream os (osm);

  os << uint8_t (17);

  os.flush ();
  EXPECT_EQ (s2string (osm), "0x11");

  tl::InputMemoryStream ism (osm.data (), osm.size ());
  tl::InputStream is (ism);
  tl::BinaryInputStream bis (is);

  uint8_t x;
  bis >> x;

  EXPECT_EQ (x, 17);
}

TEST(BinaryStreams6)
{
  tl::OutputMemoryStream osm;
  tl::BinaryOutputStream os (osm);

  os << int8_t (17);

  os.flush ();
  EXPECT_EQ (s2string (osm), "0x11");

  tl::InputMemoryStream ism (osm.data (), osm.size ());
  tl::InputStream is (ism);
  tl::BinaryInputStream bis (is);

  int8_t x;
  bis >> x;

  EXPECT_EQ (x, 17);
}

TEST(BinaryStreams7)
{
  tl::OutputMemoryStream osm;
  tl::BinaryOutputStream os (osm);

  os << uint16_t (1742);

  os.flush ();
  EXPECT_EQ (s2string (osm), "0xce,0x06");

  tl::InputMemoryStream ism (osm.data (), osm.size ());
  tl::InputStream is (ism);
  tl::BinaryInputStream bis (is);

  uint16_t x;
  bis >> x;

  EXPECT_EQ (x, 1742);
}

TEST(BinaryStreams8)
{
  tl::OutputMemoryStream osm;
  tl::BinaryOutputStream os (osm);

  os << int16_t (1742);

  os.flush ();
  EXPECT_EQ (s2string (osm), "0xce,0x06");

  tl::InputMemoryStream ism (osm.data (), osm.size ());
  tl::InputStream is (ism);
  tl::BinaryInputStream bis (is);

  int16_t x;
  bis >> x;

  EXPECT_EQ (x, 1742);
}

TEST(BinaryStreams9)
{
  tl::OutputMemoryStream osm;
  tl::BinaryOutputStream os (osm);

  os << uint32_t (17420000);

  os.flush ();
  EXPECT_EQ (s2string (osm), "0xe0,0xce,0x09,0x01");

  tl::InputMemoryStream ism (osm.data (), osm.size ());
  tl::InputStream is (ism);
  tl::BinaryInputStream bis (is);

  uint32_t x;
  bis >> x;

  EXPECT_EQ (x, 17420000u);
}

TEST(BinaryStreams10)
{
  tl::OutputMemoryStream osm;
  tl::BinaryOutputStream os (osm);

  os << int32_t (17420000);

  os.flush ();
  EXPECT_EQ (s2string (osm), "0xe0,0xce,0x09,0x01");

  tl::InputMemoryStream ism (osm.data (), osm.size ());
  tl::InputStream is (ism);
  tl::BinaryInputStream bis (is);

  int32_t x;
  bis >> x;

  EXPECT_EQ (x, 17420000);
}

TEST(BinaryStreams11)
{
  tl::OutputMemoryStream osm;
  tl::BinaryOutputStream os (osm);

  os << uint64_t (174200000000l);

  os.flush ();
  EXPECT_EQ (s2string (osm), "0x00,0x0e,0x21,0x8f,0x28,0x00,0x00,0x00");

  tl::InputMemoryStream ism (osm.data (), osm.size ());
  tl::InputStream is (ism);
  tl::BinaryInputStream bis (is);

  uint64_t x;
  bis >> x;

  EXPECT_EQ (x, 174200000000lu);
}

TEST(BinaryStreams12)
{
  tl::OutputMemoryStream osm;
  tl::BinaryOutputStream os (osm);

  os << int64_t (174200000000l);

  os.flush ();
  EXPECT_EQ (s2string (osm), "0x00,0x0e,0x21,0x8f,0x28,0x00,0x00,0x00");

  tl::InputMemoryStream ism (osm.data (), osm.size ());
  tl::InputStream is (ism);
  tl::BinaryInputStream bis (is);

  int64_t x;
  bis >> x;

  EXPECT_EQ (x, 174200000000l);
}

TEST(BinaryStreams13)
{
  tl::OutputMemoryStream osm;
  tl::BinaryOutputStream os (osm);

  os << true;
  os << false;

  os.flush ();
  EXPECT_EQ (s2string (osm), "0x01,0x00");

  tl::InputMemoryStream ism (osm.data (), osm.size ());
  tl::InputStream is (ism);
  tl::BinaryInputStream bis (is);

  bool x = false, y = false;
  bis >> x >> y;

  EXPECT_EQ (x, true);
  EXPECT_EQ (y, false);
}

TEST(BinaryStreamsCombined)
{
  tl::OutputMemoryStream osm;
  tl::BinaryOutputStream os (osm);

  os << "ABC" << 17.0 << "XUV" << (int32_t) 42;

  os.flush ();

  tl::InputMemoryStream ism (osm.data (), osm.size ());
  tl::InputStream is (ism);
  tl::BinaryInputStream bis (is);

  std::string a, c;
  double b = 0.0;
  int32_t d = 0;

  bis >> a >> b >> c >> d;

  EXPECT_EQ (a, "ABC");
  EXPECT_EQ (b, 17.0);
  EXPECT_EQ (c, "XUV");
  EXPECT_EQ (d, 42);
}

TEST(BinaryStreamsVariants)
{
  tl::OutputMemoryStream osm;
  tl::BinaryOutputStream os (osm);

  std::vector<char> bytes = { 'X', 'U', 'V' };

  os << tl::Variant ("ABC")
     << tl::Variant ()
     << tl::Variant (bytes)
     << tl::Variant ((float) 42.5)
     << tl::Variant ((double) -17.5)
     << tl::Variant ((char) 'x')
     << tl::Variant ((unsigned char) 'u')
     << tl::Variant ((signed char) 'v')
     << tl::Variant ((short) 17)
     << tl::Variant ((unsigned short) 42)
     << tl::Variant ((int) 18)
     << tl::Variant ((unsigned int) 43)
     << tl::Variant ((long) 19)
     << tl::Variant ((unsigned long) 44)
     << tl::Variant ((long long) 20)
     << tl::Variant ((unsigned long long) 45)
     << tl::Variant ((size_t) 202, true /*id*/);

  os.flush ();

  tl::InputMemoryStream ism (osm.data (), osm.size ());
  tl::InputStream is (ism);
  tl::BinaryInputStream bis (is);

  tl::Variant v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15, v16, v17;

  bis >> v1 >> v2 >> v3 >> v4 >> v5 >> v6 >> v7 >> v8 >> v9 >> v10 >> v11 >> v12 >> v13 >> v14 >> v15 >> v16 >> v17;

  EXPECT_EQ (v1.to_parsable_string (), "'ABC'");
  EXPECT_EQ (v2.to_parsable_string (), "nil");
  EXPECT_EQ (v3.to_parsable_string (), "'XUV'b");
  EXPECT_EQ (v3.type_code (), tl::Variant::t_bytearray);
  EXPECT_EQ (v4.to_parsable_string (), "##42.5");
  EXPECT_EQ (v5.to_parsable_string (), "##-17.5");
  EXPECT_EQ (v6.to_parsable_string (), "'x'c");
  EXPECT_EQ (v7.to_parsable_string (), "#u117");
  EXPECT_EQ (v8.to_parsable_string (), "#118");
  EXPECT_EQ (v9.to_parsable_string (), "#17");
  EXPECT_EQ (v10.to_parsable_string (), "#u42");
  EXPECT_EQ (v11.to_parsable_string (), "#18");
  EXPECT_EQ (v12.to_parsable_string (), "#u43");
  EXPECT_EQ (v13.to_parsable_string (), "#19");
  EXPECT_EQ (v14.to_parsable_string (), "#u44");
  EXPECT_EQ (v15.to_parsable_string (), "#l20");
  EXPECT_EQ (v16.to_parsable_string (), "#lu45");
  EXPECT_EQ (v17.to_parsable_string (), "[id202]");
}

TEST(BinaryStreamsVariantList)
{
  tl::OutputMemoryStream osm;
  tl::BinaryOutputStream os (osm);

  std::vector<tl::Variant> list = { 5.5, -17, "ABC" };

  os << tl::Variant (list);

  os.flush ();

  tl::InputMemoryStream ism (osm.data (), osm.size ());
  tl::InputStream is (ism);
  tl::BinaryInputStream bis (is);

  tl::Variant v1;

  bis >> v1;

  EXPECT_EQ (v1.to_parsable_string (), "(##5.5,#-17,'ABC')");
}

TEST(BinaryStreamsVariantArray)
{
  tl::OutputMemoryStream osm;
  tl::BinaryOutputStream os (osm);

  std::vector<std::pair<tl::Variant, tl::Variant> > a = { { 1, 5.5 }, { 2, -17 }, { "id", "ABC" } };

  std::map<tl::Variant, tl::Variant> array (a.begin (), a.end ());
  os << tl::Variant (array);

  os.flush ();

  tl::InputMemoryStream ism (osm.data (), osm.size ());
  tl::InputStream is (ism);
  tl::BinaryInputStream bis (is);

  tl::Variant v1;

  bis >> v1;

  EXPECT_EQ (v1.to_parsable_string (), "{#1=>##5.5,#2=>#-17,'id'=>'ABC'}");
}
