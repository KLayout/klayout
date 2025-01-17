
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


#include "dbMatrix.h"
#include "dbTrans.h"
#include "tlUnitTest.h"

TEST(1) 
{
  db::Matrix2d m1;
  EXPECT_EQ (m1.to_string (), "(0,0) (0,0)");
  EXPECT_EQ (tl::to_string (m1.is_ortho ()), "true");

  m1 = db::Matrix2d (1, 2, 3, 4);
  EXPECT_EQ (m1.to_string (), "(1,2) (3,4)");
  EXPECT_EQ (tl::to_string (m1.is_ortho ()), "false");
  EXPECT_EQ (m1.det (), -2);
  EXPECT_EQ (m1.m11 (), 1);
  EXPECT_EQ (m1.m12 (), 2);
  EXPECT_EQ (m1.m21 (), 3);
  EXPECT_EQ (m1.m22 (), 4);

  db::Matrix2d m2 (5.0);
  EXPECT_EQ (m2.to_string (), "(5,0) (0,5)");

  EXPECT_EQ ((m2 + m1).to_string (), "(6,2) (3,9)");

  m2 += m1;
  EXPECT_EQ (m2.to_string (), "(6,2) (3,9)");

  EXPECT_EQ ((m1 * m2).to_string (), "(12,20) (30,42)");

  m1 *= m2;
  EXPECT_EQ (m1.to_string (), "(12,20) (30,42)");

  EXPECT_EQ ((m1 * 0.5).to_string (), "(6,10) (15,21)");

  m1 *= 0.5;
  EXPECT_EQ (m1.to_string (), "(6,10) (15,21)");

  EXPECT_EQ ((m1 * db::DVector (1, 2)).to_string (), "26,57");

  EXPECT_EQ ((m1.transposed ()).to_string (), "(6,15) (10,21)");
  m1.transpose ();
  EXPECT_EQ (m1.to_string (), "(6,15) (10,21)");

  EXPECT_EQ (fabs ((m1 * m1.inverted ()).m11 () - 1.0) < 1e-15, true);
  EXPECT_EQ (fabs ((m1 * m1.inverted ()).m12 ()) < 1e-15, true);
  EXPECT_EQ (fabs ((m1 * m1.inverted ()).m21 ()) < 1e-15, true);
  EXPECT_EQ (fabs ((m1 * m1.inverted ()).m22 () - 1.0) < 1e-15, true);

  db::Matrix2d m1s = m1;
  m1.invert ();
  EXPECT_EQ (fabs ((m1 * m1s).m11 () - 1.0) < 1e-15, true);
  EXPECT_EQ (fabs ((m1 * m1s).m12 ()) < 1e-15, true);
  EXPECT_EQ (fabs ((m1 * m1s).m21 ()) < 1e-15, true);
  EXPECT_EQ (fabs ((m1 * m1s).m22 () - 1.0) < 1e-15, true);
  EXPECT_EQ (fabs ((m1s * m1).m11 () - 1.0) < 1e-15, true);
  EXPECT_EQ (fabs ((m1s * m1).m12 ()) < 1e-15, true);
  EXPECT_EQ (fabs ((m1s * m1).m21 ()) < 1e-15, true);
  EXPECT_EQ (fabs ((m1s * m1).m22 () - 1.0) < 1e-15, true);

  db::CplxTrans t (2.0, 90.0, false, db::DVector ());
  EXPECT_EQ (fabs (db::Matrix2d (t).m11 ()) < 1e-15, true);
  EXPECT_EQ (fabs (db::Matrix2d (t).m12 () + 2.0) < 1e-15, true);
  EXPECT_EQ (fabs (db::Matrix2d (t).m21 () - 2.0) < 1e-15, true);
  EXPECT_EQ (fabs (db::Matrix2d (t).m22 ()) < 1e-15, true);

  //  Base transformations and decomposition
  EXPECT_EQ (tl::to_string (db::Matrix2d (1.0).mag_x ()), "1");
  EXPECT_EQ (tl::to_string (db::Matrix2d (1.0).mag_y ()), "1");
  EXPECT_EQ (tl::to_string (db::Matrix2d (1.0).is_mirror ()), "false");
  EXPECT_EQ (tl::to_string (db::Matrix2d (1.0).angle ()), "0");
  EXPECT_EQ (tl::to_string (db::Matrix2d (1.0).shear_angle ()), "0");
  EXPECT_EQ (tl::to_string (db::Matrix2d (1.0).has_shear ()), "false");
  EXPECT_EQ (tl::to_string (db::Matrix2d::mag (17.5).mag_x ()), "17.5");
  EXPECT_EQ (tl::to_string (db::Matrix2d::mag (17.5).mag_x ()), "17.5");
  EXPECT_EQ (tl::to_string (db::Matrix2d::mag (17.5).shear_angle ()), "0");
  EXPECT_EQ (tl::to_string (db::Matrix2d::mag (17.5).angle ()), "0");
  EXPECT_EQ (tl::to_string (db::Matrix2d::mag (17.5).is_mirror ()), "false");
  EXPECT_EQ (tl::to_string (db::Matrix2d::mirror (true).is_mirror ()), "true");
  EXPECT_EQ (tl::to_string (db::Matrix2d::mirror (false).is_mirror ()), "false");
  EXPECT_EQ (tl::to_string (db::Matrix2d::mirror (true).shear_angle ()), "0");
  EXPECT_EQ (tl::to_string (db::Matrix2d::mirror (true).angle ()), "0");
  EXPECT_EQ (tl::to_string (db::Matrix2d::rotation (25).angle ()), "25");
  EXPECT_EQ (tl::to_string (db::Matrix2d::rotation (-25).angle ()), "-25");
  EXPECT_EQ (tl::to_string (db::Matrix2d::rotation (115).angle ()), "115");
  EXPECT_EQ (tl::to_string (db::Matrix2d::rotation (-115).angle ()), "-115");
  EXPECT_EQ (tl::to_string (1e-6 * floor (0.5 + 1e6 * db::Matrix2d::rotation (-115).shear_angle ())), "0");
  EXPECT_EQ (tl::to_string (db::Matrix2d::shear (17).shear_angle ()), "17");
  EXPECT_EQ (tl::to_string (1e-6 * floor (0.5 + 1e6 * db::Matrix2d::shear (17).angle ())), "0");
  EXPECT_EQ (tl::to_string (db::Matrix2d::shear (17).mag_x ()), "1");
  EXPECT_EQ (tl::to_string (db::Matrix2d::shear (17).mag_y ()), "1");
  EXPECT_EQ (tl::to_string (db::Matrix2d::shear (17).has_shear ()), "true");
  EXPECT_EQ (tl::to_string (db::Matrix2d::shear (40).shear_angle ()), "40");
  EXPECT_EQ (tl::to_string (db::Matrix2d::shear (-40).shear_angle ()), "-40");
  EXPECT_EQ (tl::to_string (1.0 / db::Matrix2d::mag (17.5).inverted ().mag2 ().first), "17.5");
  EXPECT_EQ (tl::to_string (1.0 / db::Matrix2d::mag (17.5).inverted ().mag2 ().second), "17.5");
  EXPECT_EQ (tl::to_string (1.0 / db::Matrix2d::mag (27.5, 7.5).inverted ().mag2 ().first), "27.5");
  EXPECT_EQ (tl::to_string (1.0 / db::Matrix2d::mag (27.5, 7.5).inverted ().mag2 ().second), "7.5");
  EXPECT_EQ (tl::to_string (db::Matrix2d::mirror (true).inverted ().is_mirror ()), "true");
  EXPECT_EQ (tl::to_string (db::Matrix2d::mirror (false).inverted ().is_mirror ()), "false");
  EXPECT_EQ (tl::to_string (db::Matrix2d::rotation (25).inverted ().angle ()), "-25");
  EXPECT_EQ (tl::to_string (db::Matrix2d::shear (17).inverted ().shear_angle ()), "-17");

  db::Matrix2d m = db::Matrix2d::rotation (25) * (db::Matrix2d::shear (17) * (db::Matrix2d::mirror (true) * db::Matrix2d::mag (7.5, 27.5)));
  EXPECT_EQ (tl::to_string (m.mag_x ()), "7.5");
  EXPECT_EQ (tl::to_string (m.mag_y ()), "27.5");
  EXPECT_EQ (tl::to_string (m.is_mirror ()), "true");
  EXPECT_EQ (tl::to_string (m.angle ()), "25");
  EXPECT_EQ (tl::to_string (m.shear_angle ()), "17");

  m = db::Matrix2d::rotation (-25) * (db::Matrix2d::shear (-17) * (db::Matrix2d::mirror (true) * db::Matrix2d::mag (27.5, 7.5)));
  EXPECT_EQ (tl::to_string (m.mag_x ()), "27.5");
  EXPECT_EQ (tl::to_string (m.mag_y ()), "7.5");
  EXPECT_EQ (tl::to_string (m.is_mirror ()), "true");
  EXPECT_EQ (tl::to_string (m.angle ()), "-25");
  EXPECT_EQ (tl::to_string (m.shear_angle ()), "-17");
}

TEST(2) 
{
  db::Matrix2d m (1.0, 0.0, 0.0, 1.0);
  db::DVector d;
  std::vector<db::DPoint> p, q;
  p.push_back (db::DPoint (1, 2));
  q.push_back (db::DPoint (2, 4));
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::Displacement);
  EXPECT_EQ (m.to_string (), "(1,0) (0,1)");
  EXPECT_EQ (d.to_string (), "1,2");
  //  once again with the previous transformation as the initial one
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::Displacement);
  EXPECT_EQ (m.to_string (), "(1,0) (0,1)");
  EXPECT_EQ (d.to_string (), "1,2");

  // one more point
  p.clear (); q.clear ();
  p.push_back (db::DPoint (1, 2));
  p.push_back (db::DPoint (2, 3));
  q.push_back (db::DPoint (2, 4));
  q.push_back (db::DPoint (4, 6));
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::Displacement);
  EXPECT_EQ (m.to_string (), "(1,0) (0,1)");
  EXPECT_EQ (d.to_string (), "1.5,2.5");
  //  once again with the previous transformation as the initial one
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::Displacement);
  EXPECT_EQ (m.to_string (), "(1,0) (0,1)");
  EXPECT_EQ (d.to_string (), "1.5,2.5");
  //  once again with focus on the first points
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::Displacement, 0);
  EXPECT_EQ (m.to_string (), "(1,0) (0,1)");
  EXPECT_EQ (d.to_string (), "1,2");
  //  .. and on the second
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::Displacement, 1);
  EXPECT_EQ (m.to_string (), "(1,0) (0,1)");
  EXPECT_EQ (d.to_string (), "2,3");
}

TEST(3) 
{
  db::Matrix2d m (1.0, 0.0, 0.0, 1.0);
  db::DVector d;
  std::vector<db::DPoint> p, q;
  p.push_back (db::DPoint (1, 2));
  p.push_back (db::DPoint (2, 2));
  q.push_back (db::DPoint (2, 4));
  q.push_back (db::DPoint (2, 6));
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::Rotation);
  EXPECT_EQ (m.to_string (), "(0,-1) (1,0)");
  EXPECT_EQ (tl::to_string (m.is_ortho ()), "true");
  EXPECT_EQ (d.to_string (), "4,3.5");
  //  once again with the previous transformation as the initial one
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::Rotation);
  EXPECT_EQ (m.to_string (), "(0,-1) (1,0)");
  EXPECT_EQ (d.to_string (), "4,3.5");
  //  once again with focus on the first point
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::Rotation, 0);
  EXPECT_EQ (m.to_string (), "(0,-1) (1,0)");
  EXPECT_EQ (d.to_string (), "4,3");
  //  .. and on the second
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::Rotation, 1);
  EXPECT_EQ (m.to_string (), "(0,-1) (1,0)");
  EXPECT_EQ (d.to_string (), "4,4");

  //  Degenerated
  m = db::Matrix2d (1.0, 0.0, 0.0, 1.0);
  d = db::DVector ();
  p.clear (); q.clear ();
  p.push_back (db::DPoint (1, 2));
  p.push_back (db::DPoint (1, 2));
  q.push_back (db::DPoint (2, 4));
  q.push_back (db::DPoint (2, 4));
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::Rotation);
  EXPECT_EQ (m.to_string (), "(1,0) (0,1)");
  EXPECT_EQ (d.to_string (), "1,2");
  //  once again with the previous transformation as the initial one
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::Rotation);
  EXPECT_EQ (m.to_string (), "(1,0) (0,1)");
  EXPECT_EQ (d.to_string (), "1,2");

  p.clear (); q.clear ();
  p.push_back (db::DPoint (1, 2));
  p.push_back (db::DPoint (1, 2));
  q.push_back (db::DPoint (2, 4));
  q.push_back (db::DPoint (2, 5));
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::Rotation);
  EXPECT_EQ (m.to_string (), "(1,0) (0,1)");
  EXPECT_EQ (d.to_string (), "1,2.5");
  //  once again with the previous transformation as the initial one
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::Rotation);
  EXPECT_EQ (m.to_string (), "(1,0) (0,1)");
  EXPECT_EQ (d.to_string (), "1,2.5");

  p.clear (); q.clear ();
  p.push_back (db::DPoint (1, 2));
  p.push_back (db::DPoint (1, 3));
  q.push_back (db::DPoint (2, 4));
  q.push_back (db::DPoint (2, 4));
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::Rotation);
  EXPECT_EQ (m.to_string (), "(1,0) (0,1)");
  EXPECT_EQ (d.to_string (), "1,1.5");
  //  once again with the previous transformation as the initial one
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::Rotation);
  EXPECT_EQ (m.to_string (), "(1,0) (0,1)");
  EXPECT_EQ (d.to_string (), "1,1.5");
}

TEST(4) 
{
  db::Matrix2d m (1.0, 0.0, 0.0, 1.0);
  db::DVector d;
  std::vector<db::DPoint> p, q;
  p.push_back (db::DPoint (3, 6));
  p.push_back (db::DPoint (6, 6));
  p.push_back (db::DPoint (3, 9));
  q.push_back (db::DPoint (6, 12));
  q.push_back (db::DPoint (6, 18));
  q.push_back (db::DPoint (0, 12));
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::RotationMirror);
  EXPECT_EQ (m.to_string (), "(0,-1) (1,0)");
  EXPECT_EQ (d.to_string (), "11,10");
  //  once again with the previous transformation as the initial one
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::RotationMirror);
  EXPECT_EQ (m.to_string (), "(0,-1) (1,0)");
  EXPECT_EQ (d.to_string (), "11,10");
  //  once again with focus on the first point
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::RotationMirror, 0);
  EXPECT_EQ (m.to_string (), "(0,-1) (1,0)");
  EXPECT_EQ (d.to_string (), "12,9");
  //  .. and on the second
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::RotationMirror, 2);
  EXPECT_EQ (m.to_string (), "(0,-1) (1,0)");
  EXPECT_EQ (d.to_string (), "9,9");

  m = db::Matrix2d (1.0, 0.0, 0.0, 1.0);
  d = db::DVector ();
  p.clear (); q.clear ();

  p.push_back (db::DPoint (3, 6));
  p.push_back (db::DPoint (6, 6));
  p.push_back (db::DPoint (3, 3));
  q.push_back (db::DPoint (6, 12));
  q.push_back (db::DPoint (6, 18));
  q.push_back (db::DPoint (0, 12));
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::RotationMirror);
  EXPECT_EQ (m.to_string (), "(0,1) (1,0)");
  EXPECT_EQ (d.to_string (), "-1,10");
  //  once again with the previous transformation as the initial one
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::RotationMirror);
  EXPECT_EQ (m.to_string (), "(0,1) (1,0)");
  EXPECT_EQ (d.to_string (), "-1,10");
  //  once again with focus on the first point
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::RotationMirror, 0);
  EXPECT_EQ (m.to_string (), "(0,1) (1,0)");
  EXPECT_EQ (d.to_string (), "0,9");
  //  .. and on the second
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::RotationMirror, 2);
  EXPECT_EQ (m.to_string (), "(0,1) (1,0)");
  EXPECT_EQ (d.to_string (), "-3,9");

  //  special case of axis normalization:

  m = db::Matrix2d (1.0, 0.0, 0.0, 1.0);
  d = db::DVector ();
  p.clear (); q.clear ();

  p.push_back (db::DPoint (3, 6));
  p.push_back (db::DPoint (7, 6));
  p.push_back (db::DPoint (3, 7));
  q.push_back (db::DPoint (6, 12));
  q.push_back (db::DPoint (6, 17));
  q.push_back (db::DPoint (1, 12));
  //  once again with focus on the first point
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::RotationMirror, 0);
  EXPECT_EQ (m.to_string (), "(0,-1) (1,0)");
  EXPECT_EQ (d.to_string (), "12,9");
}

TEST(5) 
{
  db::Matrix2d m (1.0, 0.0, 0.0, 1.0);
  db::DVector d;
  std::vector<db::DPoint> p, q;
  p.push_back (db::DPoint (3, 6));
  p.push_back (db::DPoint (6, 6));
  p.push_back (db::DPoint (3, 9));
  q.push_back (db::DPoint (6, 12));
  q.push_back (db::DPoint (6, 18));
  q.push_back (db::DPoint (0, 12));
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::Magnification);
  EXPECT_EQ (m.to_string (), "(0,-2) (2,0)");
  EXPECT_EQ (d.to_string (), "18,6");
  //  once again with the previous transformation as the initial one
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::Magnification);
  EXPECT_EQ (m.to_string (), "(0,-2) (2,0)");
  EXPECT_EQ (d.to_string (), "18,6");
  //  once again with focus on the first point
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::Magnification, 0);
  EXPECT_EQ (m.to_string (), "(0,-2) (2,0)");
  EXPECT_EQ (d.to_string (), "18,6");
  //  .. and on the second
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::Magnification, 2);
  EXPECT_EQ (m.to_string (), "(0,-2) (2,0)");
  EXPECT_EQ (d.to_string (), "18,6");
}

TEST(6) 
{
  db::Matrix2d m (1.0, 0.0, 0.0, 1.0);
  db::DVector d;
  std::vector<db::DPoint> p, q;
  p.push_back (db::DPoint (3, 6));
  p.push_back (db::DPoint (6, 6));
  p.push_back (db::DPoint (3, 9));
  p.push_back (db::DPoint (6, 9));
  q.push_back (db::DPoint (6, 12));
  q.push_back (db::DPoint (6, 18));
  q.push_back (db::DPoint (0, 12));
  q.push_back (db::DPoint (0, 18));
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::All);
  EXPECT_EQ (m.to_string (), "(0,-2) (2,0)");
  EXPECT_EQ (d.to_string (), "18,6");
  //  once again with the previous transformation as the initial one
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::All);
  EXPECT_EQ (m.to_string (), "(0,-2) (2,0)");
  EXPECT_EQ (d.to_string (), "18,6");
  //  once again with focus on the first point
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::All, 0);
  EXPECT_EQ (m.to_string (), "(0,-2) (2,0)");
  EXPECT_EQ (d.to_string (), "18,6");
  //  .. and on the second
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::All, 2);
  EXPECT_EQ (m.to_string (), "(0,-2) (2,0)");
  EXPECT_EQ (d.to_string (), "18,6");

  db::Matrix2d n = db::Matrix2d::rotation (-25) * (db::Matrix2d::shear (-17) * (db::Matrix2d::mirror (true) * db::Matrix2d::mag (17.5, 7.5)));

  db::DVector dd (0.5, -1);
  p.clear (); q.clear ();
  p.push_back (db::DPoint (0, 0));
  p.push_back (db::DPoint (1, 0));
  p.push_back (db::DPoint (0, 1));
  p.push_back (db::DPoint (1, 1));
  q.push_back (n * p[0] + dd);
  q.push_back (n * p[1] + dd);
  q.push_back (n * p[2] + dd);
  q.push_back (n * p[3] + dd);

  m = db::Matrix2d (1.0, 0.0, 0.0, 1.0);
  d = db::DVector ();
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::All);
  EXPECT_EQ (tl::to_string (m.is_ortho ()), "false");
  EXPECT_EQ (tl::to_string (m.shear_angle ()), "-17");
  EXPECT_EQ (tl::to_string (m.has_shear ()), "true");
  EXPECT_EQ (tl::to_string (m.angle ()), "-25");
  EXPECT_EQ (tl::to_string (m.is_mirror ()), "true");
  EXPECT_EQ (tl::to_string (m.mag_x ()), "17.5");
  EXPECT_EQ (tl::to_string (m.mag_y ()), "7.5");
  EXPECT_EQ (d.to_string (), "0.5,-1");
  //  once again with the previous transformation as the initial one
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::All);
  EXPECT_EQ (tl::to_string (m.shear_angle ()), "-17");
  EXPECT_EQ (tl::to_string (m.angle ()), "-25");
  EXPECT_EQ (tl::to_string (m.is_mirror ()), "true");
  EXPECT_EQ (tl::to_string (m.mag_x ()), "17.5");
  EXPECT_EQ (tl::to_string (m.mag_y ()), "7.5");
  EXPECT_EQ (d.to_string (), "0.5,-1");
  //  once again with focus on the first point
  adjust_matrix (m, d, p, q, db::MatrixAdjustFlags::All, 0);
  EXPECT_EQ (tl::to_string (m.shear_angle ()), "-17");
  EXPECT_EQ (tl::to_string (m.angle ()), "-25");
  EXPECT_EQ (tl::to_string (m.is_mirror ()), "true");
  EXPECT_EQ (tl::to_string (m.mag_x ()), "17.5");
  EXPECT_EQ (tl::to_string (m.mag_y ()), "7.5");
  EXPECT_EQ (d.to_string (), "0.5,-1");

  m = db::Matrix2d (1, 5, 3, 9);
  db::Matrix2d mm = db::Matrix2d::rotation (m.angle ()) * db::Matrix2d::shear (m.shear_angle ()) * db::Matrix2d::mag (m.mag_x (), m.mag_y ()) * db::Matrix2d::mirror (m.is_mirror ());
  EXPECT_EQ (mm.to_string (), "(1,5) (3,9)");
}

TEST(7) 
{
  db::CplxTrans t (1.5, 45.0, true, db::DVector (10, -20));
  EXPECT_EQ (tl::to_string (db::Matrix3d (t).angle ()), "45");
  EXPECT_EQ (tl::to_string (db::Matrix3d (t).is_mirror ()), "true");
  EXPECT_EQ (tl::to_string (db::Matrix3d (t).mag_x ()), "1.5");
  EXPECT_EQ (tl::to_string (db::Matrix3d (t).mag_y ()), "1.5");
  EXPECT_EQ (tl::to_string (db::Matrix3d (t).disp ().x ()), "10");
  EXPECT_EQ (tl::to_string (db::Matrix3d (t).disp ().y ()), "-20");

  db::Matrix3d m;
  m = db::Matrix3d (2.0);
  EXPECT_EQ (tl::to_string (m.is_ortho ()), "true");
  EXPECT_EQ (m.to_string (), "(2,0,0) (0,2,0) (0,0,1)");
  EXPECT_EQ (m.inverted ().to_string (), "(0.5,0,0) (0,0.5,0) (0,0,1)");
  m = db::Matrix3d ();
  EXPECT_EQ (m.to_string (), "(0,0,0) (0,0,0) (0,0,0)");
  m = db::Matrix3d (2.0, 0.0, 0.0, 3.0);
  EXPECT_EQ (m.to_string (), "(2,0,0) (0,3,0) (0,0,1)");
  m = db::Matrix3d::rotation (90.0);
  EXPECT_EQ (m.to_string (), "(0,-1,0) (1,0,0) (0,0,1)");
  EXPECT_EQ (tl::to_string (m.is_ortho ()), "true");
  m = db::Matrix3d::perspective (45.0, 0.0, 1.0);
  EXPECT_EQ (m.to_string (), "(1,0,0) (0,1,0) (1,0,1)");
  m = db::Matrix3d (1, 2, 3, 4, 2, 1, 1, 2, 5);
  EXPECT_EQ ((m * (m.inverted () * m)).to_string (), "(1,2,3) (4,2,1) (1,2,5)");

  m = db::Matrix3d::perspective (18, -5, 1);
  EXPECT_EQ (tl::to_string (m.is_ortho ()), "false");
  EXPECT_EQ (tl::to_string (m.perspective_tilt_x (1)), "18");
  EXPECT_EQ (tl::to_string (m.perspective_tilt_y (1)), "-5");

  m = db::Matrix3d::disp (db::DVector (-5, 3)) * db::Matrix3d::perspective (18, -5, 1);
  EXPECT_EQ (tl::to_string (m.is_ortho ()), "false");
  EXPECT_EQ (tl::to_string (m.perspective_tilt_x (1)), "18");
  EXPECT_EQ (tl::to_string (m.perspective_tilt_y (1)), "-5");

  m = db::Matrix3d::disp (db::DVector (-5, 3)) * db::Matrix3d::perspective (18, -5, 1.5);
  EXPECT_EQ (tl::to_string (m.is_ortho ()), "false");
  EXPECT_EQ (tl::to_string (m.perspective_tilt_x (1.5)), "18");
  EXPECT_EQ (tl::to_string (m.perspective_tilt_y (1.5)), "-5");

  m = db::Matrix3d::disp (db::DVector (-5, 3)) * db::Matrix3d::perspective (18, -5, 1) * db::Matrix3d::rotation (33) * db::Matrix3d::shear (21) * db::Matrix3d::mag (2.5)  * db::Matrix3d::mirror (true);
  EXPECT_EQ (tl::to_string (m.is_ortho ()), "false");
  EXPECT_EQ (tl::to_string (m.perspective_tilt_x (1)), "18");
  EXPECT_EQ (tl::to_string (m.perspective_tilt_y (1)), "-5");
  EXPECT_EQ (m.disp ().to_string (), "-5,3");
  EXPECT_EQ (tl::to_string (m.shear_angle ()), "21");
  EXPECT_EQ (tl::to_string (m.has_shear ()), "true");
  EXPECT_EQ (tl::to_string (m.mag_x ()), "2.5");
  EXPECT_EQ (tl::to_string (m.mag_y ()), "2.5");
  EXPECT_EQ (tl::to_string (m.is_mirror ()), "true");
  EXPECT_EQ (tl::to_string (m.angle ()), "33");

  m = db::Matrix3d (1, 5, 0, 3, 9, 0, 0, 0, 1);
  db::Matrix3d mm = db::Matrix3d::rotation (m.angle ()) * db::Matrix3d::shear (m.shear_angle ()) * db::Matrix3d::mag (m.mag_x (), m.mag_y ()) * db::Matrix3d::mirror (m.is_mirror ());
  EXPECT_EQ (mm.to_string (), "(1,5,0) (3,9,0) (0,0,1)");

  m = db::Matrix3d (1, 5, 3, 3, 9, 4, 6, 1, 1);
  mm = db::Matrix3d::disp (m.disp ()) * db::Matrix3d::perspective (m.perspective_tilt_x (1), m.perspective_tilt_y (1), 1) * db::Matrix3d::rotation (m.angle ()) * db::Matrix3d::shear (m.shear_angle ()) * db::Matrix3d::mag (m.mag_x (), m.mag_y ()) * db::Matrix3d::mirror (m.is_mirror ());
  //  TODO: why does it need normalization?
  mm *= 1.0 / mm.m ()[2][2];
  EXPECT_EQ (mm.to_string (), "(1,5,3) (3,9,4) (6,1,1)");
}

TEST(7a) 
{
  db::Matrix3d m (0.0, 1.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0);
  EXPECT_EQ (m.inverted ().to_string (), "(0,1,0) (1,0,0) (0,0,1)");
}

TEST(7b) 
{
  db::Matrix3d m (0.0, 1.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 1.0);
  EXPECT_EQ (m.inverted ().to_string (), "(0,-1,0) (1,0,0) (0,0,1)");
}

TEST(7c) 
{
  db::Matrix3d m (0.5, 1.0, 0.75, -1.0, 0.25, -0.25, -0.5, 0.0, 2.0);
  db::DPoint p0 (1.0, -1.75);
  db::DPoint p1 (-0.5, 1);
  EXPECT_EQ (m.can_transform (db::DPoint (4.0, 0.0)), false);
  EXPECT_EQ (m.can_transform (db::DPoint (4.0, 1.0)), false);
  EXPECT_EQ (m.can_transform (db::DPoint (3.9, 0.0)), true);
  EXPECT_EQ (m.can_transform (db::DPoint (3.9, 1.0)), true);
  EXPECT_EQ (m.can_transform (db::DPoint (4.1, 0.0)), false);
  EXPECT_EQ (m.can_transform (db::DPoint (4.1, 1.0)), false);
  db::DVector v1 (m * p1 - m * p0);
  v1 *= 1.0 / v1.double_length ();
  db::DVector v2 (m.trans (p0, db::DVector (p1 - p0)));
  v2 *= 1.0 / v2.double_length ();
  EXPECT_EQ (v1.to_string (), v2.to_string ());
}

TEST(8) 
{
  db::Matrix3d n = db::Matrix3d::disp (db::DVector (-5, 3)) * db::Matrix3d::perspective (18, -5, 1) * db::Matrix3d::rotation (33) * db::Matrix3d::shear (21) * db::Matrix3d::mag (2.5) * db::Matrix3d::mirror (true);

  std::vector<db::DPoint> p, q;
  p.push_back (db::DPoint (0, 0));
  p.push_back (db::DPoint (1, 0));
  p.push_back (db::DPoint (0, 1));
  p.push_back (db::DPoint (1, 1));
  p.push_back (db::DPoint (1, 2));
  p.push_back (db::DPoint (2, 1));
  q.push_back (n * p[0]);
  q.push_back (n * p[1]);
  q.push_back (n * p[2]);
  q.push_back (n * p[3]);
  q.push_back (n * p[4]);
  q.push_back (n * p[5]);

  db::Matrix3d m (1.0);
  EXPECT_EQ (tl::to_string (m.shear_angle ()), "0");
  EXPECT_EQ (tl::to_string (m.has_shear ()), "false");
  adjust_matrix (m, p, q, db::MatrixAdjustFlags::All);
  EXPECT_EQ (tl::to_string (m.perspective_tilt_x (1)), "18");
  EXPECT_EQ (tl::to_string (m.perspective_tilt_y (1)), "-5");
  EXPECT_EQ (tl::to_string (m.shear_angle ()), "21");
  EXPECT_EQ (tl::to_string (m.angle ()), "33");
  EXPECT_EQ (tl::to_string (m.is_mirror ()), "true");
  EXPECT_EQ (tl::to_string (m.mag_x ()), "2.5");
  EXPECT_EQ (tl::to_string (m.mag_y ()), "2.5");
  EXPECT_EQ (m.disp ().to_string (), "-5,3");
  //  once again with the previous transformation as the initial one
  m = db::Matrix3d (1.0);
  adjust_matrix (m, p, q, db::MatrixAdjustFlags::All);
  EXPECT_EQ (tl::to_string (m.perspective_tilt_x (1)), "18");
  EXPECT_EQ (tl::to_string (m.perspective_tilt_y (1)), "-5");
  EXPECT_EQ (tl::to_string (m.shear_angle ()), "21");
  EXPECT_EQ (tl::to_string (m.angle ()), "33");
  EXPECT_EQ (tl::to_string (m.is_mirror ()), "true");
  EXPECT_EQ (tl::to_string (m.mag_x ()), "2.5");
  EXPECT_EQ (tl::to_string (m.mag_y ()), "2.5");
  EXPECT_EQ (m.disp ().to_string (), "-5,3");
  //  once again with focus on the first point
  m = db::Matrix3d (1.0);
  adjust_matrix (m, p, q, db::MatrixAdjustFlags::All, 1);
  EXPECT_EQ (tl::to_string (m.perspective_tilt_x (1)), "18");
  EXPECT_EQ (tl::to_string (m.perspective_tilt_y (1)), "-5");
  EXPECT_EQ (tl::to_string (m.shear_angle ()), "21");
  EXPECT_EQ (tl::to_string (m.angle ()), "33");
  EXPECT_EQ (tl::to_string (m.is_mirror ()), "true");
  EXPECT_EQ (tl::to_string (m.mag_x ()), "2.5");
  EXPECT_EQ (tl::to_string (m.mag_y ()), "2.5");
  EXPECT_EQ (m.disp ().to_string (), "-5,3");
}

TEST(9) 
{
  db::Matrix3d n = db::Matrix3d::disp (db::DVector (-5, 3)) * db::Matrix3d::perspective (18, -5, 1) * db::Matrix3d::rotation (33) * db::Matrix3d::shear (21) * db::Matrix3d::mag (2.5, 1.5) * db::Matrix3d::mirror (true);

  std::string ns = n.to_string ();
  tl::Extractor ex (ns.c_str ());

  db::Matrix3d m;
  ex.read (m);
  EXPECT_EQ (m.equal (n), true);
  EXPECT_EQ (m.less (n), false);
  EXPECT_EQ (tl::to_string (m.perspective_tilt_x (1), 8), "18");
  EXPECT_EQ (tl::to_string (m.perspective_tilt_y (1), 8), "-5");
  EXPECT_EQ (tl::to_string (m.shear_angle ()), "21");
  EXPECT_EQ (tl::to_string (m.angle ()), "33");
  EXPECT_EQ (tl::to_string (m.is_mirror ()), "true");
  EXPECT_EQ (tl::to_string (m.mag_x (), 8), "2.5");
  EXPECT_EQ (tl::to_string (m.mag_y (), 8), "1.5");
  EXPECT_EQ (tl::to_string (m.disp ().x (), 8), "-5");
  EXPECT_EQ (tl::to_string (m.disp ().y (), 8), "3");

  db::Matrix2d n2 = db::Matrix2d::rotation (-25) * (db::Matrix2d::shear (-17) * (db::Matrix2d::mirror (true) * db::Matrix2d::mag (17.5)));

  ns = n2.to_string ();
  ex = tl::Extractor (ns.c_str ());

  db::Matrix2d m2;
  ex.read (m2);

  EXPECT_EQ (m2.equal (n2), true);
  EXPECT_EQ (m2.less (n2), false);
  EXPECT_EQ (tl::to_string (m2.mag_x (), 8), "17.5");
  EXPECT_EQ (tl::to_string (m2.mag_y (), 8), "17.5");
  EXPECT_EQ (tl::to_string (m2.is_mirror ()), "true");
  EXPECT_EQ (tl::to_string (m2.angle (), 8), "-25"); // some roundoff happens here .. 
  EXPECT_EQ (tl::to_string (m2.shear_angle (), 8), "-17"); // some roundoff happens here .. 
}

TEST(10) 
{
  db::Matrix3d m (1.0);

  std::vector<db::DPoint> p, q;
  p.push_back (db::DPoint (1, 1));
  p.push_back (db::DPoint (2, 1));
  p.push_back (db::DPoint (2, 2));
  q.push_back (db::DPoint (1, 1));
  q.push_back (db::DPoint (2, 1));
  q.push_back (db::DPoint (2, 3));

  adjust_matrix (m, p, q, db::MatrixAdjustFlags::All);
  EXPECT_EQ ((m * p[0]).to_string (), "1,1");
  EXPECT_EQ ((m * p[1]).to_string (), "2,1");
  EXPECT_EQ ((m * p[2]).to_string (), "2,3");

  m = db::Matrix3d (1.0);
  adjust_matrix (m, p, q, db::MatrixAdjustFlags::All, 2);
  EXPECT_EQ ((m * p[0]).to_string (), "1,1");
  EXPECT_EQ ((m * p[1]).to_string (), "2,1");
  EXPECT_EQ ((m * p[2]).to_string (), "2,3");
}

TEST(11)
{
  //  double and integer versions basic functionality
  EXPECT_EQ ((db::Matrix2d (1.0, 0.5, -0.5, 2.0) * db::DPoint (1, 2)).to_string (), "2,3.5");
  EXPECT_EQ ((db::IMatrix2d (1.0, 0.5, -0.5, 2.0) * db::Point (10, 20)).to_string (), "20,35");
  EXPECT_EQ ((db::Matrix3d (1.0, 0.5, 0.0, -0.5, 2.0, 1.0, 0.0, 0.0, 1.0) * db::DPoint (1, 2)).to_string (), "2,4.5");
  EXPECT_EQ ((db::IMatrix3d (1.0, 0.5, 0.0, -0.5, 2.0, 1.0, 0.0, 0.0, 1.0) * db::DPoint (10, 20)).to_string (), "20,36");
}

