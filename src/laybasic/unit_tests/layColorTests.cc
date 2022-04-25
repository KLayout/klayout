
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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

#include "layColor.h"

#include "tlUnitTest.h"

#if defined(HAVE_QT)
#include <QColor>
#endif

TEST(1)
{
  EXPECT_EQ (lay::Color ().is_valid (), false);
  EXPECT_EQ (lay::Color ().to_string (), "");
  EXPECT_EQ (lay::Color ().rgb (), 0x00000000);

#if defined(HAVE_QT)
  EXPECT_EQ (QColor ().isValid (), false);
  EXPECT_EQ (tl::to_string (QColor ().name ()), "#000000");  // why?
  EXPECT_EQ (QColor ().rgb (), 0xff000000);
#endif
}

TEST(2)
{
  EXPECT_EQ (lay::Color (0x102030).is_valid (), true);
  EXPECT_EQ (lay::Color (0x102030).to_string (), "#102030");
  EXPECT_EQ (lay::Color (0x102030).rgb (), 0xff102030);

#if defined(HAVE_QT)
  EXPECT_EQ (QColor (0x102030).isValid (), true);
  EXPECT_EQ (tl::to_string (QColor (0x102030).name ()), "#102030");
  EXPECT_EQ (QColor (0x102030).rgb (), 0xff102030);
#endif
}

TEST(3)
{
  EXPECT_EQ (lay::Color (std::string ()).is_valid (), false);
  EXPECT_EQ (lay::Color ("#102030").is_valid (), true);
  EXPECT_EQ (lay::Color ("#102030").to_string (), "#102030");
  EXPECT_EQ (lay::Color ("#102030").rgb (), 0xff102030);
  EXPECT_EQ (lay::Color ("102030").is_valid (), true);
  EXPECT_EQ (lay::Color ("102030").to_string (), "#102030");
  EXPECT_EQ (lay::Color ("102030").rgb (), 0xff102030);

#if defined(HAVE_QT)
  EXPECT_EQ (QColor (tl::to_qstring ("#102030")).isValid (), true);
  EXPECT_EQ (tl::to_string (QColor (tl::to_qstring ("#102030")).name ()), "#102030");
  EXPECT_EQ (QColor (tl::to_qstring ("#102030")).rgb (), 0xff102030);
#endif
}

TEST(4)
{
  EXPECT_EQ (lay::Color ("#123").is_valid (), true);
  EXPECT_EQ (lay::Color ("#123").to_string (), "#112233");
  EXPECT_EQ (lay::Color ("#123").rgb (), 0xff112233);
}

TEST(5)
{
  EXPECT_EQ (lay::Color ("#80102030").is_valid (), true);
  EXPECT_EQ (lay::Color ("#80102030").to_string (), "#80102030");
  EXPECT_EQ (lay::Color ("#80102030").rgb (), 0x80102030);

#if defined(HAVE_QT)
  //  no alpha support in Qt
  EXPECT_EQ (QColor (tl::to_qstring ("#80102030")).isValid (), true);
  EXPECT_EQ (tl::to_string (QColor (tl::to_qstring ("#80102030")).name ()), "#102030");
  EXPECT_EQ (QColor (tl::to_qstring ("#80102030")).rgb (), 0xff102030);
#endif
}

TEST(6)
{
  EXPECT_EQ (lay::Color ("#8123").is_valid (), true);
  EXPECT_EQ (lay::Color ("#8123").to_string (), "#88112233");
  EXPECT_EQ (lay::Color ("#8123").rgb (), 0x88112233);
}

TEST(7)
{
  EXPECT_EQ (lay::Color (16, 32, 48, 128).is_valid (), true);
  EXPECT_EQ (lay::Color (16, 32, 48, 128).to_string (), "#80102030");
  EXPECT_EQ (lay::Color (16, 32, 48, 128).rgb (), 0x80102030);

#if defined(HAVE_QT)
  //  no alpha support in Qt
  EXPECT_EQ (QColor (16, 32, 48, 128).isValid (), true);
  EXPECT_EQ (tl::to_string (QColor (16, 32, 48, 128).name ()), "#102030");
  EXPECT_EQ (QColor (16, 32, 48, 128).rgb (), 0xff102030);
#endif
}
