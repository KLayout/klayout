
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

#include "layMargin.h"

#include "tlUnitTest.h"

TEST(1)
{
  lay::Margin m;

  EXPECT_EQ (m.relative_mode (), false);
  EXPECT_EQ (m.to_string (), "0");
  EXPECT_EQ (m.get (1.0), 0.0);
  EXPECT_EQ (lay::Margin::from_string (m.to_string ()).to_string (), m.to_string ());

  m.set_relative_mode (true);
  EXPECT_EQ (m.get (1.0), 0.0);
  EXPECT_EQ (m.relative_mode (), true);
  EXPECT_EQ (m.to_string (), "*0");
  EXPECT_EQ (lay::Margin::from_string (m.to_string ()).to_string (), m.to_string ());

  m = lay::Margin (1.0);
  EXPECT_EQ (m.get (2.0), 1.0);
  EXPECT_EQ (m.relative_mode (), false);
  EXPECT_EQ (m.absolute_value (), 1.0);
  EXPECT_EQ (m.to_string (), "1");
  EXPECT_EQ (lay::Margin::from_string (m.to_string ()).to_string (), m.to_string ());

  m.set_absolute_value (2.0);
  EXPECT_EQ (m.get (1.0), 2.0);
  EXPECT_EQ (m.absolute_value (), 2.0);
  EXPECT_EQ (m.to_string (), "2");
  EXPECT_EQ (lay::Margin::from_string (m.to_string ()).to_string (), m.to_string ());

  m = lay::Margin (1.5, true);
  EXPECT_EQ (m.get (1.0), 1.5);
  EXPECT_EQ (m.get (db::DBox (0.0, 0.0, 1.0, 0.5)), 1.5);
  EXPECT_EQ (m.get (db::DBox (0.0, 0.0, 1.0, 2.0)), 3.0);
  EXPECT_EQ (m.relative_mode (), true);
  EXPECT_EQ (m.relative_value (), 1.5);
  EXPECT_EQ (m.to_string (), "*1.5");
  EXPECT_EQ (lay::Margin::from_string (m.to_string ()).to_string (), m.to_string ());

  m.set_absolute_value (2.5);
  EXPECT_EQ (m.get (1.0), 1.5);
  EXPECT_EQ (m.to_string (), "*1.5 2.5");
  EXPECT_EQ (lay::Margin::from_string (m.to_string ()).to_string (), m.to_string ());
  EXPECT_EQ (m.absolute_value (), 2.5);

  m.set_relative_value (2.0);
  EXPECT_EQ (m.get (1.0), 2.0);
  EXPECT_EQ (m.to_string (), "*2 2.5");
  EXPECT_EQ (lay::Margin::from_string (m.to_string ()).to_string (), m.to_string ());
  EXPECT_EQ (m.relative_value (), 2.0);

  m.set_relative_mode (false);
  EXPECT_EQ (m.get (1.0), 2.5);
  EXPECT_EQ (m.absolute_value (), 2.5);
  EXPECT_EQ (m.to_string (), "2.5 *2");
  EXPECT_EQ (lay::Margin::from_string (m.to_string ()).to_string (), m.to_string ());
  EXPECT_EQ (m.relative_value (), 2.0);
}
