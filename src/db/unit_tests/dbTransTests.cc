
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


#include "dbTrans.h"
#include "tlUnitTest.h"

TEST(1) 
{
  db::Trans unity;
  db::Trans t;
  db::Point p(100,200);
  EXPECT_EQ (unity.is_unity (), true);
  EXPECT_EQ (t.is_unity (), true);
  EXPECT_EQ (t * p, db::Point(100, 200));
  t = db::Trans (0, false, db::Vector (-100, -200));
  EXPECT_EQ (t.is_unity (), false);
  EXPECT_EQ (t * p, db::Point(0, 0));
  db::Trans tt = t.inverted ();
  EXPECT_EQ (tt * t, unity);
  EXPECT_EQ ((tt * t).is_unity (), true);
}

TEST(2) 
{
  db::Trans unity;
  db::Point p;
  p = db::Point (100, 200);
  db::Trans t1 (1, false, db::Vector (0, 100));
  EXPECT_EQ (t1 * p, db::Point (-200, 200));
  db::Trans t2 (1, true, db::Vector (200, 100));
  EXPECT_EQ (t2 * p, db::Point (400, 200));
  EXPECT_EQ ((t1 * t2) * p, t1 * (t2 * p));
  EXPECT_EQ ((t2 * t2) * p, t2 * (t2 * p));
  EXPECT_EQ ((t1 * t1) * p, t1 * (t1 * p));
  EXPECT_EQ (t1 * t1.inverted (), unity);
  EXPECT_EQ (t1.inverted () * t1, unity);
  EXPECT_EQ (t2 * t2.inverted (), unity);
  EXPECT_EQ (t2.inverted () * t2, unity);
  EXPECT_EQ ((t1 * t2).inverted () * (t1 * t2), unity);
  EXPECT_EQ ((t1 * t2) * (t1 * t2).inverted (), unity);
}

TEST(5) 
{
  db::Point p (100, 200);
  db::Trans t1 (1, false, db::Vector (0, 100));
  db::Trans t2 (2, true, db::Vector (200, 100));
  EXPECT_EQ ((t1 * t2) * p, t1 * (t2 * p));
  EXPECT_EQ ((t2 * t1) * p, t2 * (t1 * p));
}

TEST(6) 
{
  db::Trans t1 (1, false, db::Vector (0, 100));
  EXPECT_EQ (t1.to_string (), "r90 0,100");
  db::DTrans t2 (0, true, db::DVector (12.5, -17.1));
  EXPECT_EQ (t2.to_string (), "m0 12.5,-17.1");

  tl::Extractor x;
  db::Trans tt1;
  db::DTrans tt2;

  x = tl::Extractor ("a");
  EXPECT_EQ (x.try_read (tt1), false);
  x = tl::Extractor ("r90 0,100 a");
  EXPECT_EQ (x.try_read (tt1), true);
  EXPECT_EQ (x.test ("a"), true);
  EXPECT_EQ (tt1 == t1, true);

  x = tl::Extractor ("a");
  EXPECT_EQ (x.try_read (tt2), false);
  x = tl::Extractor ("m0 12.5,-17.1 a");
  EXPECT_EQ (x.try_read (tt2), true);
  EXPECT_EQ (x.test ("a"), true);
  EXPECT_EQ (tt2 == t2, true);

  db::Matrix3d tt3d = tt2.to_matrix3d ();
  EXPECT_EQ ((tt3d * db::DVector (1, 0)).to_string (), (tt2 * db::DVector (1, 0)).to_string ());
  EXPECT_EQ ((tt3d * db::DVector (0, 1)).to_string (), (tt2 * db::DVector (0, 1)).to_string ());
  EXPECT_EQ ((tt3d * db::DVector (0, 0)).to_string (), (tt2 * db::DVector (0, 0)).to_string ());
  EXPECT_EQ ((tt3d * db::DPoint (1, 0)).to_string (), (tt2 * db::DPoint (1, 0)).to_string ());
  EXPECT_EQ ((tt3d * db::DPoint (0, 1)).to_string (), (tt2 * db::DPoint (0, 1)).to_string ());
  EXPECT_EQ ((tt3d * db::DPoint (0, 0)).to_string (), (tt2 * db::DPoint (0, 0)).to_string ());

  db::Matrix2d tt2d = tt2.to_matrix2d ();
  EXPECT_EQ ((tt2d * db::DVector (1, 0)).to_string (), (tt2 * db::DVector (1, 0)).to_string ());
  EXPECT_EQ ((tt2d * db::DVector (0, 1)).to_string (), (tt2 * db::DVector (0, 1)).to_string ());
  EXPECT_EQ ((tt2d * db::DVector (0, 0)).to_string (), (tt2 * db::DVector (0, 0)).to_string ());
}

template <class T>
T recomposed (const T &t)
{
  typedef typename T::target_coord_type tt;
  T r (db::complex_trans<tt, tt> (db::simple_trans<tt> (db::complex_trans<tt, tt> (t)), t.rcos (), t.mag ()));
  return r;
}

//  complex_trans tests 
TEST(10)
{
  db::DCplxTrans t;
  db::CplxTrans tt;

  EXPECT_EQ (t.is_unity (), true);
  EXPECT_EQ (t.to_string (), "r0 *1 0,0");
  EXPECT_EQ (t.is_mirror (), false);
  EXPECT_EQ (t.is_ortho (), true);
  EXPECT_EQ (t.fp_trans () == db::DFTrans (db::DFTrans::r0), true);

  t = db::DCplxTrans (db::DFTrans::r90);
  EXPECT_EQ (t.is_unity (), false);
  EXPECT_EQ (t.to_string (), "r90 *1 0,0");
  EXPECT_EQ (t.is_mirror (), false);
  EXPECT_EQ (t.fp_trans () == db::DFTrans (db::DFTrans::r90), true);
  EXPECT_EQ (t.to_matrix2d ().to_string (), db::DFTrans (db::DFTrans::r90).to_matrix2d ().to_string ());
  EXPECT_EQ (t.to_matrix3d ().to_string (), db::DFTrans (db::DFTrans::r90).to_matrix3d ().to_string ());
  EXPECT_EQ (int (t.angle () + 0.5), 90);
  EXPECT_EQ (t.is_ortho (), true);
  EXPECT_EQ (t (db::point<double> (1.0, 0.0)).to_string (), "0,1");
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());
  t = db::DCplxTrans (db::DFTrans::r180);
  EXPECT_EQ (t.to_string (), "r180 *1 0,0");
  EXPECT_EQ (t.is_mirror (), false);
  EXPECT_EQ (int (t.angle () + 0.5), 180);
  EXPECT_EQ (t.fp_trans () == db::DFTrans (db::DFTrans::r180), true);
  EXPECT_EQ (t.is_ortho (), true);
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());
  t = db::DCplxTrans (db::DFTrans::r270);
  EXPECT_EQ (t.fp_trans () == db::DFTrans (db::DFTrans::r270), true);
  EXPECT_EQ (t.to_string (), "r270 *1 0,0");
  EXPECT_EQ (t.is_mirror (), false);
  EXPECT_EQ (int (t.angle () + 0.5), 270);
  EXPECT_EQ (t.is_ortho (), true);
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());
  t = db::DCplxTrans (db::DFTrans::m0);
  EXPECT_EQ (t.fp_trans () == db::DFTrans (db::DFTrans::m0), true);
  EXPECT_EQ (t.is_unity (), false);
  EXPECT_EQ (t.to_string (), "m0 *1 0,0");
  EXPECT_EQ (int (t.angle () + 0.5), 0);
  EXPECT_EQ (t.is_mirror (), true);
  EXPECT_EQ (t.is_ortho (), true);
  EXPECT_EQ (t (db::point<double> (1.0, 1.0)).to_string (), "1,-1");
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());
  t = db::DCplxTrans (db::DFTrans::m45);
  EXPECT_EQ (int (t.angle () + 0.5), 90);
  EXPECT_EQ (t.fp_trans () == db::DFTrans (db::DFTrans::m45), true);
  EXPECT_EQ (t.to_string (), "m45 *1 0,0");
  EXPECT_EQ (t.is_mirror (), true);
  EXPECT_EQ (t.is_ortho (), true);
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());
  t = db::DCplxTrans (db::DFTrans::m90);
  EXPECT_EQ (int (t.angle () + 0.5), 180);
  EXPECT_EQ (t.fp_trans () == db::DFTrans (db::DFTrans::m90), true);
  EXPECT_EQ (t.to_string (), "m90 *1 0,0");
  EXPECT_EQ (t.is_ortho (), true);
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());
  t = db::DCplxTrans (db::DFTrans::m135);
  EXPECT_EQ (int (t.angle () + 0.5), 270);
  EXPECT_EQ (t.is_mirror (), true);
  EXPECT_EQ (t.fp_trans () == db::DFTrans (db::DFTrans::m135), true);
  EXPECT_EQ (t.to_string (), "m135 *1 0,0");
  EXPECT_EQ (t.is_mirror (), true);
  EXPECT_EQ (t.is_ortho (), true);
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());

  t = db::DCplxTrans (db::ICplxTrans (db::disp_trans<db::Coord> (db::Vector (100, -256))));
  EXPECT_EQ (t.is_unity (), false);
  EXPECT_EQ (t.to_string (), "r0 *1 100,-256");
  EXPECT_EQ (t.is_ortho (), true);
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());
  t = db::DCplxTrans (db::disp_trans<double> (db::DVector (-0.5, 1.25)));
  EXPECT_EQ (t.to_string (), "r0 *1 -0.5,1.25");
  EXPECT_EQ (t.is_ortho (), true);
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());

  t = db::DCplxTrans (db::ICplxTrans (db::Vector (100, -256)));
  EXPECT_EQ (t.is_unity (), false);
  EXPECT_EQ (t.to_string (), "r0 *1 100,-256");
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());
  t = db::DCplxTrans (db::DVector (-0.5, 1.25));
  EXPECT_EQ (t.to_string (), "r0 *1 -0.5,1.25");
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());

  t = db::DCplxTrans (db::DTrans (db::simple_trans<db::Coord> (db::FTrans::m135, db::Vector (128, -256))));
  EXPECT_EQ (t.is_unity (), false);
  EXPECT_EQ (t.to_string (), "m135 *1 128,-256");
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());
  t = db::DCplxTrans (db::simple_trans<double> (db::DFTrans::r180, db::DVector (-0.25, 1.5)));
  EXPECT_EQ (t.to_string (), "r180 *1 -0.25,1.5");
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());

  t = db::DCplxTrans (db::ICplxTrans (1.5, 2.5, false, db::Vector (17, -34)));
  EXPECT_EQ (t.is_unity (), false);
  EXPECT_EQ (t.to_string (), "r2.5 *1.5 17,-34");
  EXPECT_EQ (t.is_ortho (), false);
  EXPECT_EQ (tl::to_string (t.angle ()), "2.5");
  EXPECT_EQ (tl::to_string (t.ctrans (10)), "15");
  EXPECT_EQ (tl::to_string (t.mag ()), "1.5");
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());
  t = db::DCplxTrans (0.75, 12.0, true, db::DVector (1.7, 3.4));
  EXPECT_EQ (t.to_string (), "m6 *0.75 1.7,3.4");
  EXPECT_EQ (t.is_ortho (), false);
  EXPECT_EQ (tl::to_string (t.angle ()), "12");
  EXPECT_EQ (tl::to_string (t.mag ()), "0.75");
  EXPECT_EQ (tl::to_string (t.ctrans (100)), "75");
  EXPECT_EQ (t (db::DPoint (0.0, 0.0)).to_string (), "1.7,3.4");
  EXPECT_EQ (t (db::DVector (0.0, 0.0)).to_string (), "0,0");
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());

  t.angle (24.0);
  EXPECT_EQ (t.to_string (), "m12 *0.75 1.7,3.4");
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());
  t.mag (2.0);
  EXPECT_EQ (t.to_string (), "m12 *2 1.7,3.4");
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());
  t.mirror (false);
  EXPECT_EQ (t.to_string (), "r24 *2 1.7,3.4");
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());
  t.disp (db::DVector (1.8, 3.3));
  EXPECT_EQ (t.to_string (), "r24 *2 1.8,3.3");
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());

  t = db::DCplxTrans (0.15, 0.2, false, db::DVector (0.17, -0.034));
  EXPECT_EQ (t.is_unity (), false);
  EXPECT_EQ (t.to_string (), db::DCplxTrans (0.15, 0.2, false, db::DVector (17 * 0.01, -34 * 0.001)).to_string ());
  EXPECT_NE (t.to_string (), db::DCplxTrans (0.15, 0.21, false, db::DVector (17 * 0.01, -34 * 0.001)).to_string ());
  EXPECT_EQ (t.to_string (), db::DCplxTrans (0.15, 0.2, false, db::DVector (17 * 0.01, -34 * 0.001)).to_string ());
  EXPECT_NE (t.to_string (), db::DCplxTrans (0.15, 0.21, false, db::DVector (17 * 0.01, -34 * 0.001)).to_string ());
  EXPECT_EQ (t < db::DCplxTrans (0.15, 0.21, false, db::DVector (17 * 0.01, -34 * 0.001)), true);
  EXPECT_EQ (db::DCplxTrans (0.15, 0.21, false, db::DVector (17 * 0.01, -34 * 0.001)) < t, false);
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());

  tt = db::CplxTrans (0.1, 90.0, false, db::DVector (0.2, -0.1));
  EXPECT_EQ (tt.to_string (), "r90 *0.1 0.2,-0.1");
  EXPECT_EQ (tt (db::Point (10, 0.0)).to_string (), "0.2,0.9");
  EXPECT_EQ (tt.to_string (), recomposed (tt).to_string ());

  t = db::DCplxTrans (tt);
  EXPECT_EQ (t.to_string (), "r90 *0.1 0.2,-0.1");
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());
  t = db::DCplxTrans (db::ICplxTrans (tt));
  EXPECT_EQ (t.to_string (), "r90 *0.1 0.2,-0.1");

  {
    db::DCplxTrans tt1 (tt);
    tt.invert ();
    EXPECT_EQ (tt.to_string (), "r270 *10 1,2");

    db::DCplxTrans t1 (t);
    t.invert ();
    EXPECT_EQ (t.to_string (), "r270 *10 1,2");
    EXPECT_EQ (t.to_string (), recomposed (t).to_string ());
    t = t1.inverted ();
    EXPECT_EQ (t.to_string (), "r270 *10 1,2");

    t *= t1;
    EXPECT_EQ (t.is_unity (), true);
    EXPECT_EQ (t.to_string (), recomposed (t).to_string ());
  }

  tt = db::CplxTrans (0.01, 45.0, true, db::DVector (0.02, -0.01));
  EXPECT_EQ (tt.to_string (), "m22.5 *0.01 0.02,-0.01");

  db::Matrix3d tt3d = tt.to_matrix3d ();
  EXPECT_EQ ((tt3d * db::Vector (1, 0)).to_string (), (tt * db::Vector (1, 0)).to_string ());
  EXPECT_EQ ((tt3d * db::Vector (0, 1)).to_string (), (tt * db::Vector (0, 1)).to_string ());
  EXPECT_EQ ((tt3d * db::Vector (0, 0)).to_string (), (tt * db::Vector (0, 0)).to_string ());
  EXPECT_EQ ((tt3d * db::DPoint (1, 0)).to_string (), (tt * db::DPoint (1, 0)).to_string ());
  EXPECT_EQ ((tt3d * db::DPoint (0, 1)).to_string (), (tt * db::DPoint (0, 1)).to_string ());
  EXPECT_EQ ((tt3d * db::DPoint (0, 0)).to_string (), (tt * db::DPoint (0, 0)).to_string ());

  db::Matrix2d tt2d = tt.to_matrix2d ();
  EXPECT_EQ ((tt2d * db::DVector (1, 0)).to_string (), (tt * db::DVector (1, 0)).to_string ());
  EXPECT_EQ ((tt2d * db::DVector (0, 1)).to_string (), (tt * db::DVector (0, 1)).to_string ());
  EXPECT_EQ ((tt2d * db::DVector (0, 0)).to_string (), (tt * db::DVector (0, 0)).to_string ());

  t = db::DCplxTrans (tt);
  EXPECT_EQ (t.to_string (), "m22.5 *0.01 0.02,-0.01");
  EXPECT_EQ (t.to_string (), recomposed (t).to_string ());
  t = db::DCplxTrans (db::ICplxTrans (tt));
  EXPECT_EQ (t.to_string (), "m22.5 *0.01 0.02,-0.01");

  {
    db::DCplxTrans t1 = t;
    t.invert ();
    EXPECT_EQ (t.to_string (), "m22.5 *100 -0.707106781187,-2.12132034356");
    EXPECT_EQ (t.to_string (), recomposed (t).to_string ());
    t = t1.inverted ();
    EXPECT_EQ (t.to_string (), "m22.5 *100 -0.707106781187,-2.12132034356");
    EXPECT_EQ (t.to_string (), recomposed (t).to_string ());

    t *= t1;
    EXPECT_EQ (t.is_unity (), true);
    EXPECT_EQ (t.to_string (), recomposed (t).to_string ());
  }

}

TEST(11) 
{
  db::CplxTrans t1 (db::Trans (1, false, db::Vector (0, 100)));
  t1.mag (1.2);
  EXPECT_EQ (t1.to_string (), "r90 *1.2 0,100");
  db::DCplxTrans t2 (db::DTrans (0, true, db::DVector (12.5, -17.1)));
  t2.mag (0.45);
  EXPECT_EQ (t2.to_string (), "m0 *0.45 12.5,-17.1");
  db::DCplxTrans t3 (db::DTrans (0, true, db::DVector (12.4, -17.0)), cos (45 * M_PI / 180.0), 0.55);
  EXPECT_EQ (t3.to_string (), "m22.5 *0.55 12.4,-17");

  tl::Extractor x;
  db::CplxTrans tt1;
  db::DCplxTrans tt2;

  x = tl::Extractor ("a");
  EXPECT_EQ (x.try_read (tt1), false);
  x = tl::Extractor ("   r90  0, 100  * 1.2  a");
  EXPECT_EQ (x.try_read (tt1), true);
  EXPECT_EQ (x.test ("a"), true);
  EXPECT_EQ (tt1.to_string (), t1.to_string ());

  x = tl::Extractor ("a");
  EXPECT_EQ (x.try_read (tt2), false);
  x = tl::Extractor ("m0 12.5,-17.1 *0.45 a");
  EXPECT_EQ (x.try_read (tt2), true);
  EXPECT_EQ (x.test ("a"), true);
  EXPECT_EQ (tt2.to_string (), t2.to_string ());
  x = tl::Extractor (" *0.45 m0 12.5,-17.100 a");
  EXPECT_EQ (x.try_read (tt2), true);
  EXPECT_EQ (x.test ("a"), true);
  EXPECT_EQ (tt2.to_string (), t2.to_string ());
  x = tl::Extractor (" *0.45 m0 12.5,-17.100 a");
  EXPECT_EQ (x.try_read (tt2), true);
  EXPECT_EQ (x.test ("a"), true);
  EXPECT_EQ (tt2.to_string (), t2.to_string ());
  x = tl::Extractor ("m22.5 *0.55 12.4,-17 ##");
  EXPECT_EQ (x.try_read (tt2), true);
  EXPECT_EQ (x.test ("##"), true);
  EXPECT_EQ (tt2.to_string (), "m22.5 *0.55 12.4,-17");
  EXPECT_EQ (tt2.to_string (), t3.to_string ());
}

TEST(12) 
{
  db::CplxTrans t1;
  t1 = db::CplxTrans (db::Trans (1, false, db::Vector (0, 100)));
  t1.mag (1.2);
  EXPECT_EQ (t1.to_string (), "r90 *1.2 0,100");

  t1 = db::CplxTrans (db::Trans (1, false, db::Vector (0, 100)), cos (7.5 * (M_PI / 180.0)), 1.2);
  EXPECT_EQ (t1.to_string (), "r97.5 *1.2 0,100");

  t1 = db::CplxTrans (db::Trans (1, true, db::Vector (0, 100)));
  t1.mag (1.2);
  EXPECT_EQ (t1.to_string (), "m45 *1.2 0,100");

  t1 = db::CplxTrans (db::Trans (1, true, db::Vector (0, 100)), cos (7.5 * (M_PI / 180.0)), 1.2);
  EXPECT_EQ (t1.to_string (), "m48.75 *1.2 0,100");
}

TEST(13) 
{
  db::Disp t;
  EXPECT_EQ (t.to_string (), "0,0");
  EXPECT_EQ (db::Trans (t).to_string (), "r0 0,0");
  EXPECT_EQ (t.is_unity (), true);
  t = db::Disp (db::Vector (0, 100));
  EXPECT_EQ (t.to_string (), "0,100");
  EXPECT_EQ (t.to_matrix2d ().to_string (), "(1,0) (0,1)");
  EXPECT_EQ (t.to_matrix3d ().to_string (), "(1,0,0) (0,1,100) (0,0,1)");
  EXPECT_EQ (db::Trans (t).to_string (), "r0 0,100");
  EXPECT_EQ (t.inverted ().to_string (), "0,-100");
  EXPECT_EQ (t.is_unity (), false);
}

TEST(14) 
{
  db::FTrans t;
  EXPECT_EQ (db::Trans (t).to_string (), "r0 0,0");
  EXPECT_EQ (t.to_string (), "r0");
  EXPECT_EQ (t.is_unity (), true);
  t = db::FTrans (4);
  EXPECT_EQ (t.to_string (), "m0");
  EXPECT_EQ (db::Trans (t).to_string (), "m0 0,0");
  EXPECT_EQ (t.inverted ().to_string (), "m0");
  EXPECT_EQ (t.is_unity (), false);
  t = db::FTrans (1);
  EXPECT_EQ (t.to_string (), "r90");
  EXPECT_EQ (t.inverted ().to_string (), "r270");
  EXPECT_EQ (t.is_unity (), false);
}

TEST(15) 
{
  db::UnitTrans t;
  EXPECT_EQ (t.to_string (), "");
  EXPECT_EQ (t.is_unity (), true);
  EXPECT_EQ (t.to_matrix2d ().to_string (), db::Matrix2d (1.0).to_string ());
  EXPECT_EQ (t.to_matrix3d ().to_string (), db::Matrix3d (1.0).to_string ());
  EXPECT_EQ (db::Trans (t).to_string (), "r0 0,0");
}


