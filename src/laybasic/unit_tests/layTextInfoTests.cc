
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

  //  Default font
  lay::TextInfo ti (&lv);
  EXPECT_EQ (ti.bbox (text, db::DCplxTrans ()).to_string (), "(12,22;36,37)");
  //  global transformation changes the dimension as the default font is not scaled or rotated
  EXPECT_EQ (ti.bbox (text, db::DCplxTrans (2.0)).to_string (), "(11,21;23,28.5)");
  EXPECT_EQ (ti.bbox (text, db::DCplxTrans (db::DFTrans (1))).to_string (), "(12,-6;27,18)");
  //  long text
  EXPECT_EQ (ti.bbox (text2, db::DCplxTrans ()).to_string (), "(12,22;60,52)");

  //  valign
  text3 = text2;
  text3.valign (db::VAlignCenter);
  EXPECT_EQ (ti.bbox (text3, db::DCplxTrans ()).to_string (), "(12,5;60,35)");
  text3.valign (db::VAlignTop);
  EXPECT_EQ (ti.bbox (text3, db::DCplxTrans ()).to_string (), "(12,-12;60,18)");

  //  halign
  text3 = text2;
  text3.halign (db::HAlignCenter);
  EXPECT_EQ (ti.bbox (text3, db::DCplxTrans ()).to_string (), "(-14,22;34,52)");
  text3.halign (db::HAlignRight);
  EXPECT_EQ (ti.bbox (text3, db::DCplxTrans ()).to_string (), "(-40,22;8,52)");

  //  Herschey font
  lv.text_font (db::Font::StickFont);
  ti = lay::TextInfo (&lv);

  EXPECT_EQ (ti.bbox (text, db::DCplxTrans ()).to_string (), "(12,15;72,47)");
  //  global trans only scales pixel-based border but does not modify the outline in
  //  "apply transformation" mode
  EXPECT_EQ (ti.bbox (text, db::DCplxTrans (2.0)).to_string (), "(11,14;71,46)");
  EXPECT_EQ (ti.bbox (text, db::DCplxTrans (db::DFTrans (1))).to_string (), "(12,15;72,47)");
  //  long text
  EXPECT_EQ (ti.bbox (text2, db::DCplxTrans ()).to_string (), "(12,15;134,83)");

  //  valign
  text3 = text2;
  text3.valign (db::VAlignCenter);
  EXPECT_EQ (ti.bbox (text3, db::DCplxTrans ()).to_string (), "(12,-17.5;134,50.5)");
  text3.valign (db::VAlignTop);
  EXPECT_EQ (ti.bbox (text3, db::DCplxTrans ()).to_string (), "(12,-50;134,18)");

  //  halign
  text3 = text2;
  text3.halign (db::HAlignCenter);
  EXPECT_EQ (ti.bbox (text3, db::DCplxTrans ()).to_string (), "(-51,15;71,83)");
  text3.halign (db::HAlignRight);
  EXPECT_EQ (ti.bbox (text3, db::DCplxTrans ()).to_string (), "(-114,15;8,83)");

  //  smaller size as default
  lv.default_text_size (4.2);
  ti = lay::TextInfo (&lv);

  EXPECT_EQ (ti.bbox (text, db::DCplxTrans ()).to_string (), "(12,20.6;24,27)");
  EXPECT_EQ (ti.bbox (text2, db::DCplxTrans ()).to_string (), "(12,20.6;36.4,34.2)");

  //  text with explicit size
  text3 = text2;
  text3.size (21);
  EXPECT_EQ (ti.bbox (text3, db::DCplxTrans ()).to_string (), "(12,15;134,83)");

  //  text with rotation
  text3.trans (db::DTrans (1, db::DVector (10.0, 20.0)));
  EXPECT_EQ (ti.bbox (text3, db::DCplxTrans ()).to_string (), "(-53,22;15,144)");

  //  text with rotation and default font (-> rotation ignored)
  text3.font (db::Font::DefaultFont);
  EXPECT_EQ (ti.bbox (text3, db::DCplxTrans ()).to_string (), "(12,22;60,52)");
  text3.font (db::Font::StickFont);

  //  apply_text_trans = false
  lv.apply_text_trans (false);
  ti = lay::TextInfo (&lv);
  EXPECT_EQ (ti.bbox (text3, db::DCplxTrans ()).to_string (), "(12,20.6;36.4,34.2)");
  //  with apply_text_trans false, the global transformation does change the text
  //  bounding box.
  EXPECT_EQ (ti.bbox (text, db::DCplxTrans ()).to_string (), "(12,20.6;24,27)");
  EXPECT_EQ (ti.bbox (text, db::DCplxTrans (2.0)).to_string (), "(11,19.6;23,26)");
  EXPECT_EQ (ti.bbox (text, db::DCplxTrans (db::DFTrans (1))).to_string (), "(10.6,6;17,18)");
}
