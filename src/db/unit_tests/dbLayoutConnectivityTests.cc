
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


#include "dbLayoutConnectivity.h"
#include "tlUnitTest.h"

namespace
{

TEST(1_LayoutPinBasic)
{
  db::LayoutPin pin;

  EXPECT_EQ (pin.name (), "");
  EXPECT_EQ (pin.must_connect (), false);

  pin = db::LayoutPin ("XYZ", true);

  EXPECT_EQ (pin.name (), "XYZ");
  EXPECT_EQ (pin.must_connect (), true);

  pin.set_must_connect (false);
  EXPECT_EQ (pin.must_connect (), false);

  pin.set_name ("ABC");
  EXPECT_EQ (pin.name (), "ABC");
}

TEST(2_LayoutPinVariantCompatibility)
{
  db::LayoutPin pin ("ABC", true);

  std::string s = tl::Variant (pin).to_parsable_string ();
  EXPECT_EQ (s, "[layoutpin:{'must_connect'=>true,'name'=>'ABC'}]");

  tl::Variant v;
  tl::Extractor ex (s.c_str ());

  ex.read (v);

  EXPECT_EQ (v.is_user (), true);
  db::LayoutPin pin2 = v.to_user<db::LayoutPin> ();

  EXPECT_EQ (pin2.name (), "ABC");
  EXPECT_EQ (pin2.must_connect (), true);

  s = tl::Variant (db::LayoutPin ()).to_parsable_string ();
  EXPECT_EQ (s, "[layoutpin:{'name'=>''}]");

  ex = tl::Extractor (s.c_str ());
  ex.read (v);
  EXPECT_EQ (v.is_user (), true);
  pin2 = v.to_user<db::LayoutPin> ();

  EXPECT_EQ (pin2.name (), "");
  EXPECT_EQ (pin2.must_connect (), false);
}

}
