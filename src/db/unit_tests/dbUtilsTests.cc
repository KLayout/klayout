
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


#include "dbUtils.h"
#include "tlUnitTest.h"
#include "tlStringEx.h"

#include <cmath>

TEST(1)
{
  std::list<db::DPoint> pts;

  std::vector<std::pair<db::DPoint, double> > cp;
  cp.push_back (std::make_pair (db::DPoint (-1, 0), 1));
  cp.push_back (std::make_pair (db::DPoint (-1, 1), sqrt (0.5)));
  cp.push_back (std::make_pair (db::DPoint (0, 1), 1));

  std::vector<double> k;
  k.push_back (0.0);
  k.push_back (0.0);
  k.push_back (0.0);
  k.push_back (1.0);
  k.push_back (1.0);
  k.push_back (1.0);

  pts = db::spline_interpolation (cp, 2, k, 0.01, 0.01);

  EXPECT_EQ (tl::to_string (pts),
    "-1,0,"
    "-0.983305368417,0.181963052412,"
    "-0.929788301062,0.368094709562,"
    "-0.836995511219,0.547209753385,"
    "-0.707106781187,0.707106781187,"
    "-0.547209753385,0.836995511219,"
    "-0.368094709562,0.929788301062,"
    "-0.181963052412,0.983305368417,"
    "0,1"
  );

  for (std::list<db::DPoint>::iterator i = pts.begin (); i != pts.end (); ++i) {
    EXPECT_EQ (fabs (i->double_distance () - 1.0) < 1e-10, true);
  }

  pts = db::spline_interpolation (cp, 2, k, 0.001, 0.001);

  EXPECT_EQ (pts.size (), size_t (33));

  for (std::list<db::DPoint>::iterator i = pts.begin (); i != pts.end (); ++i) {
    EXPECT_EQ (fabs (i->double_distance () - 1.0) < 1e-10, true);
  }
}

TEST(2)
{
  std::list<db::DPoint> pts;

  std::vector<db::DPoint> cp;
  cp.push_back (db::DPoint (-1, 0));
  cp.push_back (db::DPoint (-1, 1));
  cp.push_back (db::DPoint (0, 1));

  std::vector<double> w;
  w.push_back (1);
  w.push_back (sqrt (0.5));
  w.push_back (1);

  std::vector<double> k;
  k.push_back (0.0);
  k.push_back (0.0);
  k.push_back (0.0);
  k.push_back (1.0);
  k.push_back (1.0);
  k.push_back (1.0);

  pts = db::spline_interpolation (cp, w, 2, k, 0.01, 0.01);

  EXPECT_EQ (tl::to_string (pts),
    "-1,0,"
    "-0.983305368417,0.181963052412,"
    "-0.929788301062,0.368094709562,"
    "-0.836995511219,0.547209753385,"
    "-0.707106781187,0.707106781187,"
    "-0.547209753385,0.836995511219,"
    "-0.368094709562,0.929788301062,"
    "-0.181963052412,0.983305368417,"
    "0,1"
  );
}

TEST(3)
{
  std::list<db::DPoint> pts;

  std::vector<db::DPoint> cp;
  cp.push_back (db::DPoint (-1, 0));
  cp.push_back (db::DPoint (-1, 1));
  cp.push_back (db::DPoint (0, 1));

  std::vector<double> k;
  k.push_back (0.0);
  k.push_back (0.0);
  k.push_back (0.0);
  k.push_back (1.0);
  k.push_back (1.0);
  k.push_back (1.0);

  //  non-rational spline
  pts = db::spline_interpolation (cp, 2, k, 0.01, 0.01);

  EXPECT_EQ (tl::to_string (pts),
    "-1,0,"
    "-0.984375,0.234375,"
    "-0.9375,0.4375,"
    "-0.859375,0.609375,"
    "-0.75,0.75,"
    "-0.609375,0.859375,"
    "-0.4375,0.9375,"
    "-0.234375,0.984375,"
    "0,1"
  );
}

