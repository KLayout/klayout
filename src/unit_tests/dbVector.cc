
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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



#include "dbVector.h"
#include "dbPoint.h"
#include "dbTrans.h"
#include "tlString.h"
#include "utHead.h"

TEST(1) 
{
  db::Vector p;
  EXPECT_EQ (p.x (), 0);
  EXPECT_EQ (p.y (), 0);
  db::Vector pp (100, 200);
  EXPECT_EQ ((p + pp).x (), 100);
  EXPECT_EQ ((p + pp).y (), 200);
  p += pp;
  p += p + pp;
  EXPECT_EQ ((p - pp).x (), 200);
  EXPECT_EQ ((p - pp).y (), 400);
  EXPECT_EQ ((db::Vector () - p).to_string (), "-300,-600");
}

TEST(2)
{
  db::DVector p (12.5, -17.1);
  EXPECT_EQ (p.to_string (), "12.5,-17.1");

  db::DVector pp;
  tl::Extractor ex ("a");
  EXPECT_EQ (ex.try_read (pp), false);
  ex = tl::Extractor ("12.500, -171e-1   a");
  EXPECT_EQ (ex.try_read (pp), true);
  EXPECT_EQ (pp.to_string (), p.to_string ());
  EXPECT_EQ (ex.test ("a"), true);
}

TEST(3)
{
  db::Vector p (125, -171);
  EXPECT_EQ (p.to_string (), "125,-171");

  db::Vector pp;
  tl::Extractor ex ("a");
  EXPECT_EQ (ex.try_read (pp), false);
  ex = tl::Extractor (" 125, -171 a");
  EXPECT_EQ (ex.try_read (pp), true);
  EXPECT_EQ (pp == p, true);
  EXPECT_EQ (ex.test ("a"), true);
}

TEST(4)
{
  db::Vector p1 (100, -100), p2 (200, 200);

  EXPECT_EQ (db::vprod (p1, p2), 40000);
  EXPECT_EQ (db::vprod_sign (p1, p2), 1);
  EXPECT_EQ (db::sprod (p1, p2), 0);
  EXPECT_EQ (db::sprod_sign (p1, p2), 0);

  EXPECT_EQ ((db::Point (100, 100) + p1).to_string (), "200,0")
}

TEST(5)
{
  db::Vector p1 (100, -100), p2 (200, 200);

  EXPECT_EQ (p1.transformed (db::Disp (db::Vector (50, -150))).to_string (), "100,-100")
  EXPECT_EQ (p1.transformed (db::FTrans (db::FTrans::r90)).to_string (), "100,100")
  EXPECT_EQ (p1.transformed (db::Trans (db::FTrans::r90, p2)).to_string (), "100,100")
  EXPECT_EQ (p1.transformed (db::DCplxTrans (db::DTrans (db::DFTrans::r90, db::DVector (p2)))).to_string (), "100,100")
  EXPECT_EQ (p1.transformed (db::DCplxTrans (1.5)).to_string (), "150,-150")

  EXPECT_EQ ((db::Disp (db::Vector (50, -150)) * p1).to_string (), "100,-100")
  EXPECT_EQ ((db::FTrans (db::FTrans::r90) * p1).to_string (), "100,100")
  EXPECT_EQ ((db::Trans (db::FTrans::r90, p2) * p1).to_string (), "100,100")
  EXPECT_EQ ((db::DCplxTrans (db::DTrans (db::DFTrans::r90, db::DVector (p2))) * db::DVector (p1)).to_string (), "100,100")
  EXPECT_EQ ((db::DCplxTrans (1.5) * p1).to_string (), "150,-150")

  EXPECT_EQ (p1.transform (db::ICplxTrans (1.5)).to_string (), "150,-150")
  EXPECT_EQ (p1.to_string (), "150,-150")
}


