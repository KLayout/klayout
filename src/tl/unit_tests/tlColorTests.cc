
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

#include "tlColor.h"

#include "tlUnitTest.h"

#if defined(HAVE_QT)
#include <QColor>
#endif

TEST(1)
{
  EXPECT_EQ (tl::Color ().is_valid (), false);
  EXPECT_EQ (tl::Color ().to_string (), "");
  EXPECT_EQ (tl::Color ().rgb (), 0x00000000u);

#if defined(HAVE_QT)
  EXPECT_EQ (tl::Color (QColor ()).is_valid (), false);
  EXPECT_EQ (tl::Color (QColor ()).to_string (), "");
  EXPECT_EQ (tl::Color (QColor ()).rgb (), 0x00000000u);
  EXPECT_EQ (tl::Color (QColor ()).to_qc ().isValid (), false);
  EXPECT_EQ (QColor ().isValid (), false);
  EXPECT_EQ (tl::to_string (QColor ().name ()), "#000000");  // why?
  EXPECT_EQ (QColor ().rgb (), 0xff000000u);
#endif
}

TEST(2)
{
  EXPECT_EQ (tl::Color (0x102030).is_valid (), true);
  EXPECT_EQ (tl::Color (0x102030).to_string (), "#102030");
  EXPECT_EQ (tl::Color (0x102030).rgb (), 0xff102030u);

#if defined(HAVE_QT)
  EXPECT_EQ (tl::Color (QColor (0x102030)).is_valid (), true);
  EXPECT_EQ (tl::Color (QColor (0x102030)).to_string (), "#102030");
  EXPECT_EQ (tl::Color (QColor (0x102030)).rgb (), 0xff102030u);
  EXPECT_EQ (tl::Color (QColor (0x102030)).to_qc ().isValid (), true);
  EXPECT_EQ (tl::to_string (tl::Color (QColor (0x102030)).to_qc ().name ()), "#102030");
  EXPECT_EQ (QColor (0x102030).isValid (), true);
  EXPECT_EQ (tl::to_string (QColor (0x102030).name ()), "#102030");
  EXPECT_EQ (QColor (0x102030).rgb (), 0xff102030u);
#endif
}

TEST(3)
{
  EXPECT_EQ (tl::Color (std::string ()).is_valid (), false);
  EXPECT_EQ (tl::Color ("#102030").is_valid (), true);
  EXPECT_EQ (tl::Color ("#102030").to_string (), "#102030");
  EXPECT_EQ (tl::Color ("#102030").rgb (), 0xff102030u);
  EXPECT_EQ (tl::Color ("102030").is_valid (), true);
  EXPECT_EQ (tl::Color ("102030").to_string (), "#102030");
  EXPECT_EQ (tl::Color ("102030").rgb (), 0xff102030u);

#if defined(HAVE_QT)
  EXPECT_EQ (QColor (tl::to_qstring ("#102030")).isValid (), true);
  EXPECT_EQ (tl::to_string (QColor (tl::to_qstring ("#102030")).name ()), "#102030");
  EXPECT_EQ (QColor (tl::to_qstring ("#102030")).rgb (), 0xff102030u);
#endif
}

TEST(4)
{
  EXPECT_EQ (tl::Color ("#123").is_valid (), true);
  EXPECT_EQ (tl::Color ("#123").to_string (), "#112233");
  EXPECT_EQ (tl::Color ("#123").rgb (), 0xff112233u);
}

TEST(5)
{
  EXPECT_EQ (tl::Color ("#80102030").is_valid (), true);
  EXPECT_EQ (tl::Color ("#80102030").alpha (), 128u);
  EXPECT_EQ (tl::Color ("#80102030").red (), 16u);
  EXPECT_EQ (tl::Color ("#80102030").green (), 32u);
  EXPECT_EQ (tl::Color ("#80102030").blue (), 48u);
  EXPECT_EQ (tl::Color ("#80102030").to_string (), "#80102030");
  EXPECT_EQ (tl::Color ("#80102030").rgb (), 0x80102030u);

#if defined(HAVE_QT) && QT_VERSION >= 0x50000
  //  no alpha support in Qt
  EXPECT_EQ (QColor (tl::to_qstring ("#80102030")).isValid (), true);
  EXPECT_EQ (tl::to_string (QColor (tl::to_qstring ("#80102030")).name ()), "#102030");
  EXPECT_EQ (QColor (tl::to_qstring ("#80102030")).rgb (), 0xff102030u);
#endif
}

TEST(6)
{
  EXPECT_EQ (tl::Color ("#8123").is_valid (), true);
  EXPECT_EQ (tl::Color ("#8123").to_string (), "#88112233");
  EXPECT_EQ (tl::Color ("#8123").rgb (), 0x88112233u);
}

TEST(7)
{
  EXPECT_EQ (tl::Color (16, 32, 48, 128).is_valid (), true);
  EXPECT_EQ (tl::Color (16, 32, 48, 128).to_string (), "#80102030");
  EXPECT_EQ (tl::Color (16, 32, 48, 128).rgb (), 0x80102030u);

#if defined(HAVE_QT)
  //  no alpha support in Qt
  EXPECT_EQ (QColor (16, 32, 48, 128).isValid (), true);
  EXPECT_EQ (tl::to_string (QColor (16, 32, 48, 128).name ()), "#102030");
  EXPECT_EQ (QColor (16, 32, 48, 128).rgb (), 0xff102030u);
#endif
}

TEST(8)
{
  unsigned int h, s, v;
  int ih, is, iv;
  tl::Color c = tl::Color (16, 32, 48);
  c.get_hsv (h, s, v);
  EXPECT_EQ (h, 210u);
  EXPECT_EQ (s, 170u);
  EXPECT_EQ (v, 48u);

  EXPECT_EQ (tl::Color::from_hsv (h, s, v).to_string (), "#102030");

#if defined(HAVE_QT)
  QColor qc = QColor (16, 32, 48);
  qc.getHsv (&ih, &is, &iv);
  EXPECT_EQ (ih, 210);
  EXPECT_EQ (is, 170);
  EXPECT_EQ (iv, 48);
#endif

  c = tl::Color (32, 16, 48);
  c.get_hsv (h, s, v);
  EXPECT_EQ (h, 270u);
  EXPECT_EQ (s, 170u);
  EXPECT_EQ (v, 48u);

  EXPECT_EQ (tl::Color::from_hsv (h, s, v).to_string (), "#201030");

#if defined(HAVE_QT)
  qc = QColor (32, 16, 48);
  qc.getHsv (&ih, &is, &iv);
  EXPECT_EQ (ih, 270);
  EXPECT_EQ (is, 170);
  EXPECT_EQ (iv, 48);
#endif

  c = tl::Color (32, 48, 16);
  c.get_hsv (h, s, v);
  EXPECT_EQ (h, 90u);
  EXPECT_EQ (s, 170u);
  EXPECT_EQ (v, 48u);

  EXPECT_EQ (tl::Color::from_hsv (h, s, v).to_string (), "#203010");

#if defined(HAVE_QT)
  qc = QColor (32, 48, 16);
  qc.getHsv (&ih, &is, &iv);
  EXPECT_EQ (ih, 90);
  EXPECT_EQ (is, 170);
  EXPECT_EQ (iv, 48);
#endif

  c = tl::Color (48, 32, 16);
  c.get_hsv (h, s, v);
  EXPECT_EQ (h, 30u);
  EXPECT_EQ (s, 170u);
  EXPECT_EQ (v, 48u);

  EXPECT_EQ (tl::Color::from_hsv (h, s, v).to_string (), "#302010");

#if defined(HAVE_QT)
  qc = QColor (48, 32, 16);
  qc.getHsv (&ih, &is, &iv);
  EXPECT_EQ (ih, 30);
  EXPECT_EQ (is, 170);
  EXPECT_EQ (iv, 48);
#endif

  c = tl::Color (48, 16, 32);
  c.get_hsv (h, s, v);
  EXPECT_EQ (h, 330u);
  EXPECT_EQ (s, 170u);
  EXPECT_EQ (v, 48u);

  EXPECT_EQ (tl::Color::from_hsv (h, s, v).to_string (), "#301020");

#if defined(HAVE_QT)
  qc = QColor (48, 16, 32);
  qc.getHsv (&ih, &is, &iv);
  EXPECT_EQ (ih, 330);
  EXPECT_EQ (is, 170);
  EXPECT_EQ (iv, 48);
#endif

  c = tl::Color (16, 48, 32);
  c.get_hsv (h, s, v);
  EXPECT_EQ (h, 150u);
  EXPECT_EQ (s, 170u);
  EXPECT_EQ (v, 48u);

  EXPECT_EQ (tl::Color::from_hsv (h, s, v).to_string (), "#103020");

#if defined(HAVE_QT)
  qc = QColor (16, 48, 32);
  qc.getHsv (&ih, &is, &iv);
  EXPECT_EQ (ih, 150);
  EXPECT_EQ (is, 170);
  EXPECT_EQ (iv, 48);
#endif
}
