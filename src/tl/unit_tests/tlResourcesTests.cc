
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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


#include "tlUnitTest.h"
#include "tlResources.h"

#include <memory>

//  uncompressed resources

TEST(1)
{
  unsigned char hw[] = "hello, world!\n";

  const char *name;
  std::unique_ptr<tl::InputStream> s;

  name = "__test_resource1";
  tl::resource_id_type id = tl::register_resource (name, false, hw, sizeof (hw));

  s.reset (tl::get_resource ("__doesnotexist"));
  EXPECT_EQ (s.get () == 0, true);

  s.reset (tl::get_resource (name));
  EXPECT_EQ (s.get () == 0, false);
  if (s) {
    std::string data = s->read_all ();
    EXPECT_EQ (data.size (), strlen ((const char *) hw) + 1);
    EXPECT_EQ (data, std::string ((const char *) hw, sizeof (hw)));
  }

  tl::unregister_resource (id);
  s.reset (tl::get_resource (name));
  EXPECT_EQ (s.get () == 0, true);
}

//  compressed resources

TEST(2)
{
  const unsigned char hw[] = {
    0x78,0x9c, //  zlib header
    //  data:
    0xcb,0x48,0xcd,0xc9,0xc9,0xd7,0x51,0x28,0xcf,0x2f,0xca,0x49,0x51,0xe4,
    0x02,0x00,
    0x26,0xb2,0x04,0xb4,  //  zlib CRC
  };
  unsigned char hw_decoded[] = "hello, world!\n";

  const char *name;
  std::unique_ptr<tl::InputStream> s;

  name = "__test_resource2";
  tl::resource_id_type id = tl::register_resource (name, true, hw, sizeof (hw));

  s.reset (tl::get_resource ("__doesnotexist"));
  EXPECT_EQ (s.get () == 0, true);

  s.reset (tl::get_resource (name));
  EXPECT_EQ (s.get () == 0, false);
  if (s) {
    std::string data = s->read_all ();
    EXPECT_EQ (data.size (), strlen ((const char *) hw_decoded));
    EXPECT_EQ (data, std::string ((const char *) hw_decoded, sizeof (hw_decoded) - 1));
  }

  tl::unregister_resource (id);
  s.reset (tl::get_resource (name));
  EXPECT_EQ (s.get () == 0, true);
}

