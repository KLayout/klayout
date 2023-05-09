
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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

#include "layTextInfo.h"
#include "layLayoutViewBase.h"

#include "tlUnitTest.h"

TEST(1)
{
  lay::LayoutViewBase lv (0, false, 0);
  lv.resize (200, 100);
  lv.zoom_box (db::DBox (0, 0, 200, 100));

  lv.default_text_size (21);
  lv.text_font (db::Font::DefaultFont);

  db::DText text;
  text.string ("ABC");
  text.trans (db::DTrans (db::DVector (10.0, 20.0)));

  db::DText text2;
  text2.string ("ABC\nCDEFGH");
  text2.trans (db::DTrans (db::DVector (10.0, 20.0)));

  db::DText text3;

  lay::TextInfo ti (&lv);
  EXPECT_EQ (ti.bbox (text, db::DCplxTrans ()).to_string (), "(12,22;36,37)");
  EXPECT_EQ (ti.bbox (text, db::DCplxTrans (2.0)).to_string (), "(6,11;18,18.5)");
  EXPECT_EQ (ti.bbox (text2, db::DCplxTrans ()).to_string (), "(12,22;60,52)");

  text3 = text2;
  text3.valign (db::VAlignCenter);
  EXPECT_EQ (ti.bbox (text3, db::DCplxTrans ()).to_string (), "(12,5;60,35)");
  text3.valign (db::VAlignTop);
  EXPECT_EQ (ti.bbox (text3, db::DCplxTrans ()).to_string (), "(12,-12;60,18)");

  text3 = text2;
  text3.halign (db::HAlignCenter);
  EXPECT_EQ (ti.bbox (text3, db::DCplxTrans ()).to_string (), "(-14,22;34,52)");
  text3.halign (db::HAlignRight);
  EXPECT_EQ (ti.bbox (text3, db::DCplxTrans ()).to_string (), "(-40,22;8,52)");

  lv.text_font (db::Font::StickFont);
  ti = lay::TextInfo (&lv);

  EXPECT_EQ (ti.bbox (text, db::DCplxTrans ()).to_string (), "(12,22;72,47)");
  EXPECT_EQ (ti.bbox (text2, db::DCplxTrans ()).to_string (), "(12,22;134,83)");

  text3 = text2;
  text3.valign (db::VAlignCenter);
  EXPECT_EQ (ti.bbox (text3, db::DCplxTrans ()).to_string (), "(12,-10.5;134,50.5)");
  text3.valign (db::VAlignTop);
  EXPECT_EQ (ti.bbox (text3, db::DCplxTrans ()).to_string (), "(12,-43;134,18)");

  text3 = text2;
  text3.halign (db::HAlignCenter);
  EXPECT_EQ (ti.bbox (text3, db::DCplxTrans ()).to_string (), "(-51,22;71,83)");
  text3.halign (db::HAlignRight);
  EXPECT_EQ (ti.bbox (text3, db::DCplxTrans ()).to_string (), "(-114,22;8,83)");

  lv.default_text_size (4.2);
  ti = lay::TextInfo (&lv);

  EXPECT_EQ (ti.bbox (text, db::DCplxTrans ()).to_string (), "(12,22;24,27)");
  EXPECT_EQ (ti.bbox (text2, db::DCplxTrans ()).to_string (), "(12,22;36.4,34.2)");


}
