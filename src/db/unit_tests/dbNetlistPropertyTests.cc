
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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


#include "dbNetlistProperty.h"

#include "tlUnitTest.h"
#include "tlVariant.h"

#include <memory>

TEST(1_TerminalRefBasic)
{
  db::DeviceTerminalProperty dp (42, 17);
  EXPECT_EQ (dp.to_string (), "42:17");
  EXPECT_EQ (dp.device_id () == 42, true);
  EXPECT_EQ (dp.terminal_id () == 17, true);

  dp.set_terminal_ref (2, 1);
  EXPECT_EQ (dp.to_string (), "2:1");
  EXPECT_EQ (dp.device_id () == 2, true);
  EXPECT_EQ (dp.terminal_id () == 1, true);

  db::DeviceTerminalProperty dp2 = dp;
  EXPECT_EQ (dp2.to_string (), "2:1");
}

TEST(2_Variants)
{
  std::auto_ptr<db::DeviceTerminalProperty> dp (new db::DeviceTerminalProperty ());
  dp->set_terminal_ref (42, 17);

  tl::Variant v (dp.release (), db::NetlistProperty::variant_class (), true);
  EXPECT_EQ (v.is_user<db::NetlistProperty> (), true);
  EXPECT_EQ (dynamic_cast<db::DeviceTerminalProperty &>(v.to_user<db::NetlistProperty> ()).to_string (), "42:17");
  EXPECT_EQ (v.to_string (), "42:17");

  tl::Variant vv = v;
  v = tl::Variant ();
  EXPECT_EQ (v.is_user<db::NetlistProperty> (), false);
  EXPECT_EQ (vv.is_user<db::NetlistProperty> (), true);
  EXPECT_EQ (dynamic_cast<db::DeviceTerminalProperty &>(vv.to_user<db::NetlistProperty> ()).to_string (), "42:17");
}

