
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


#include "dbEdgesToContours.h"
#include "tlUnitTest.h"

static std::string c2s (const std::vector <db::Point> &c) 
{
  std::string s;
  for (std::vector <db::Point>::const_iterator p = c.begin (); p != c.end (); ++p) {
    if (! s.empty ()) {
      s += ";";
    }
    s += p->to_string ();
  }
  return s;
}

TEST(1) 
{
  db::Edge edges[] = {
    db::Edge (db::Point (0, 0), db::Point (100, 0)),
    db::Edge (db::Point (100, 0), db::Point (100, 100)),
    db::Edge (db::Point (100, 100), db::Point (0, 100)),
    db::Edge (db::Point (0, 100), db::Point (0, 0))
  };

  db::EdgesToContours e2c;
  e2c.fill (&edges[0], &edges[0] + (sizeof (edges) / sizeof (edges [0])), false);

  EXPECT_EQ (e2c.contours (), size_t (1));
  EXPECT_EQ (c2s (e2c.contour (0)), "100,0;100,100;0,100;0,0");
  EXPECT_EQ (e2c.contour_closed (0), true);

  edges [0].swap_points ();
  e2c.fill (&edges[0], &edges[0] + (sizeof (edges) / sizeof (edges [0])), false);

  EXPECT_EQ (e2c.contours (), size_t (2));
  EXPECT_EQ (c2s (e2c.contour (0)), "100,0;0,0");
  EXPECT_EQ (e2c.contour_closed (0), false);
  EXPECT_EQ (c2s (e2c.contour (1)), "100,0;100,100;0,100;0,0");
  EXPECT_EQ (e2c.contour_closed (1), false);

  e2c.fill (&edges[0], &edges[0] + (sizeof (edges) / sizeof (edges [0])), true);
  EXPECT_EQ (e2c.contours (), size_t (1));
  EXPECT_EQ (c2s (e2c.contour (0)), "0,0;0,100;100,100;100,0");
  EXPECT_EQ (e2c.contour_closed (0), true);

  edges [2].swap_points ();

  e2c.fill (&edges[0], &edges[0] + (sizeof (edges) / sizeof (edges [0])), true);
  EXPECT_EQ (e2c.contours (), size_t (1));
  EXPECT_EQ (c2s (e2c.contour (0)), "0,0;0,100;100,100;100,0");
  EXPECT_EQ (e2c.contour_closed (0), true);
}

TEST(2) 
{
  db::Edge edges[] = {
    db::Edge (db::Point (-100, -100), db::Point (100, -100)),
    db::Edge (db::Point (100, -100), db::Point (0, 0)),
    db::Edge (db::Point (200, -50), db::Point (0, 0)),
    db::Edge (db::Point (200, -50), db::Point (0, 100)),
    db::Edge (db::Point (-200, -50), db::Point (0, 100)),
    db::Edge (db::Point (-200, -50), db::Point (0, 0)),
    db::Edge (db::Point (-100, -100), db::Point (0, 0))
  };

  db::EdgesToContours e2c;
  e2c.fill (&edges[0], &edges[0] + (sizeof (edges) / sizeof (edges [0])), true);

  EXPECT_EQ (e2c.contours (), size_t (1));
  EXPECT_EQ (c2s (e2c.contour (0)), "100,-100;0,0;200,-50;0,100;-200,-50;0,0;-100,-100");
  EXPECT_EQ (e2c.contour_closed (0), true);

  std::swap (edges [0], edges [3]);

  e2c.fill (&edges[0], &edges[0] + (sizeof (edges) / sizeof (edges [0])), true);

  EXPECT_EQ (e2c.contours (), size_t (1));
  EXPECT_EQ (c2s (e2c.contour (0)), "0,100;-200,-50;0,0;100,-100;-100,-100;0,0;200,-50");
  EXPECT_EQ (e2c.contour_closed (0), true);

}

TEST(3) 
{
  db::Edge edges[] = {
    db::Edge (db::Point (-100, -100), db::Point (100, -100)),
    db::Edge (db::Point (100, -100), db::Point (0, 0)),
    db::Edge (db::Point (0, 0), db::Point (200, -50)),
    db::Edge (db::Point (200, -50), db::Point (0, 100)),
    db::Edge (db::Point (0, 100), db::Point (-200, -50)),
    db::Edge (db::Point (-200, -50), db::Point (0, 0)),
    db::Edge (db::Point (0, 0), db::Point (-100, -100))
  };

  db::EdgesToContours e2c;
  e2c.fill (&edges[0], &edges[0] + (sizeof (edges) / sizeof (edges [0])), false);

  EXPECT_EQ (e2c.contours (), size_t (2));
  EXPECT_EQ (c2s (e2c.contour (0)), "100,-100;0,0;-100,-100");
  EXPECT_EQ (e2c.contour_closed (0), true);
  EXPECT_EQ (c2s (e2c.contour (1)), "200,-50;0,100;-200,-50;0,0");
  EXPECT_EQ (e2c.contour_closed (1), true);

  std::swap (edges [0], edges [3]);

  e2c.fill (&edges[0], &edges[0] + (sizeof (edges) / sizeof (edges [0])), false);

  EXPECT_EQ (e2c.contours (), size_t (2));
  EXPECT_EQ (c2s (e2c.contour (0)), "0,100;-200,-50;0,0;200,-50");
  EXPECT_EQ (e2c.contour_closed (1), true);
  EXPECT_EQ (c2s (e2c.contour (1)), "0,0;-100,-100;100,-100");
  EXPECT_EQ (e2c.contour_closed (0), true);

}

TEST(4) 
{
  db::Edge edges[] = {
    db::Edge (db::Point (0, 0), db::Point (0, 100)),
    db::Edge (db::Point (0, 100), db::Point (-100, 100)),
    db::Edge (db::Point (-100, 100), db::Point (-100, 200)),
    db::Edge (db::Point (-100, 200), db::Point (200, 200)),
    db::Edge (db::Point (200, 200), db::Point (200, 100)),
    db::Edge (db::Point (200, 100), db::Point (0, 100)),
    db::Edge (db::Point (0, 100), db::Point (0, 200)),
    db::Edge (db::Point (0, 200), db::Point (100, 200)),
    db::Edge (db::Point (100, 200), db::Point (100, 0)),
    db::Edge (db::Point (100, 0), db::Point (0, 0))
  };

  db::EdgesToContours e2c;
  e2c.fill (&edges[0], &edges[0] + (sizeof (edges) / sizeof (edges [0])), false);

  EXPECT_EQ (e2c.contours (), size_t (1));
  EXPECT_EQ (c2s (e2c.contour (0)), "0,100;-100,100;-100,200;200,200;200,100;0,100;0,200;100,200;100,0;0,0");
  EXPECT_EQ (e2c.contour_closed (0), true);
}

TEST(5) 
{
  db::Edge edges[] = {
    db::Edge (db::Point (0, 0), db::Point (0, 100)),
    db::Edge (db::Point (0, 100), db::Point (-100, 100)),
    db::Edge (db::Point (200, 100), db::Point (0, 100)),
    db::Edge (db::Point (-100, 100), db::Point (-100, 200)),
    db::Edge (db::Point (0, 100), db::Point (0, 200)),
    db::Edge (db::Point (200, 200), db::Point (200, 100)),
    db::Edge (db::Point (100, 200), db::Point (100, 0)),
    db::Edge (db::Point (-100, 200), db::Point (200, 200)),
    db::Edge (db::Point (0, 200), db::Point (100, 200)),
  };

  {
    db::EdgesToContours e2c;
    e2c.fill (&edges[0], &edges[0] + (sizeof (edges) / sizeof (edges [0])), false);

    EXPECT_EQ (e2c.contours (), size_t (1));
    EXPECT_EQ (c2s (e2c.contour (0)), "0,0;0,100;-100,100;-100,200;200,200;200,100;0,100;0,200;100,200;100,0");
    EXPECT_EQ (e2c.contour_closed (0), false);
  }

  for (size_t i = 0 ; i < sizeof (edges) / sizeof (edges[0]); ++i) {
    edges[i].set_p1 (edges[i].p1 () + db::Vector (1, 1));
  }

  {
    db::EdgesToContours e2c;
    e2c.fill (&edges[0], &edges[0] + (sizeof (edges) / sizeof (edges [0])), false, 10);

    EXPECT_EQ (e2c.contours (), size_t (1));
    EXPECT_EQ (c2s (e2c.contour (0)), "1,1;0,100;-100,100;-100,200;200,200;200,100;0,100;0,200;100,200;100,0");
    EXPECT_EQ (e2c.contour_closed (0), false);
  }
}

TEST(6) 
{
  db::Edge edges[] = {
    db::Edge (db::Point (0, 0), db::Point (100, 0)),
    db::Edge (db::Point (100, 0), db::Point (100, 100)),
    db::Edge (db::Point (100, 100), db::Point (0, 100)),
    db::Edge (db::Point (0, 100), db::Point (0, 0)),
    db::Edge (db::Point (1000, 0), db::Point (1100, 0)),
    db::Edge (db::Point (1100, 0), db::Point (1100, 100)),
    db::Edge (db::Point (1100, 100), db::Point (1000, 100)),
    db::Edge (db::Point (1000, 100), db::Point (1000, 0))
  };

  {
    db::EdgesToContours e2c;
    e2c.fill (&edges[0], &edges[0] + (sizeof (edges) / sizeof (edges [0])), false);

    EXPECT_EQ (e2c.contours (), size_t (2));
    EXPECT_EQ (c2s (e2c.contour (0)), "100,0;100,100;0,100;0,0");
    EXPECT_EQ (e2c.contour_closed (0), true);
    EXPECT_EQ (c2s (e2c.contour (1)), "1100,0;1100,100;1000,100;1000,0");
    EXPECT_EQ (e2c.contour_closed (1), true);
  }

  for (size_t i = 0 ; i < sizeof (edges) / sizeof (edges[0]); ++i) {
    edges[i].set_p1 (edges[i].p1 () + db::Vector (1, 1));
  }

  {
    db::EdgesToContours e2c;
    e2c.fill (&edges[0], &edges[0] + (sizeof (edges) / sizeof (edges [0])), false, 10);

    EXPECT_EQ (e2c.contours (), size_t (2));
    EXPECT_EQ (c2s (e2c.contour (0)), "100,0;100,100;0,100;0,0");
    EXPECT_EQ (e2c.contour_closed (0), true);
    EXPECT_EQ (c2s (e2c.contour (1)), "1100,0;1100,100;1000,100;1000,0");
    EXPECT_EQ (e2c.contour_closed (1), true);
  }
}

TEST(7) 
{
  db::Edge edges[] = {
    db::Edge (db::Point (0, 0), db::Point (0, 100)),
    db::Edge (db::Point (0, 100), db::Point (200, 100)),
    db::Edge (db::Point (200, 100), db::Point (400, 100)),
    db::Edge (db::Point (400, 100), db::Point (400, 0)),
    db::Edge (db::Point (400, 0), db::Point (300, 0)),
    db::Edge (db::Point (300, 0), db::Point (300, 100)),
    db::Edge (db::Point (300, 100), db::Point (200, 100)),
    db::Edge (db::Point (200, 100), db::Point (200, 0)),
    db::Edge (db::Point (200, 0), db::Point (200, 100)),
    db::Edge (db::Point (200, 100), db::Point (100, 100)),
    db::Edge (db::Point (100, 100), db::Point (100, 0)),
    db::Edge (db::Point (100, 0), db::Point (0, 0))
  };

  {
    db::EdgesToContours e2c;
    e2c.fill (&edges[0], &edges[0] + (sizeof (edges) / sizeof (edges [0])), false);

    EXPECT_EQ (e2c.contours (), size_t (1));
    EXPECT_EQ (c2s (e2c.contour (0)), "0,100;200,100;400,100;400,0;300,0;300,100;200,100;200,0;200,100;100,100;100,0;0,0");
    EXPECT_EQ (e2c.contour_closed (0), true);
  }

  {
    db::EdgesToContours e2c;
    e2c.fill (&edges[0], &edges[0] + (sizeof (edges) / sizeof (edges [0])), false, 10);

    EXPECT_EQ (e2c.contours (), size_t (1));
    EXPECT_EQ (c2s (e2c.contour (0)), "0,100;200,100;400,100;400,0;300,0;300,100;200,100;200,0;200,100;100,100;100,0;0,0");
    EXPECT_EQ (e2c.contour_closed (0), true);
  }

  for (size_t i = 0 ; i < sizeof (edges) / sizeof (edges[0]); ++i) {
    edges[i].set_p1 (edges[i].p1 () + db::Vector (1, 1));
  }

  {
    db::EdgesToContours e2c;
    e2c.fill (&edges[0], &edges[0] + (sizeof (edges) / sizeof (edges [0])), false, 10);

    EXPECT_EQ (e2c.contours (), size_t (1));
    EXPECT_EQ (c2s (e2c.contour (0)), "0,100;200,100;400,100;400,0;300,0;300,100;200,100;200,0;200,100;100,100;100,0;0,0");
    EXPECT_EQ (e2c.contour_closed (0), true);
  }

  for (size_t i = 0 ; i < sizeof (edges) / sizeof (edges[0]); ++i) {
    edges[i].set_p1 (edges[i].p1 () + db::Vector (10, 10));
  }

  {
    db::EdgesToContours e2c;
    e2c.fill (&edges[0], &edges[0] + (sizeof (edges) / sizeof (edges [0])), false, 10);

    EXPECT_EQ (e2c.contours (), size_t (12));
    EXPECT_EQ (c2s (e2c.contour (0)), "11,11;0,100");
    EXPECT_EQ (e2c.contour_closed (0), false);
  }
}


TEST(8)
{
  db::Edge edges[] = {
    db::Edge (db::Point (100, 100), db::Point (200, 100)),
    db::Edge (db::Point (100, 100), db::Point (100, 200)),
    db::Edge (db::Point (0, 0), db::Point (0, 1000)),
    db::Edge (db::Point (0, 1000), db::Point (1000, 1000)),
    db::Edge (db::Point (1000, 1000), db::Point (1000, 0)),
    db::Edge (db::Point (1000, 0), db::Point (0, 0))
  };

  {
    db::EdgesToContours e2c;
    e2c.fill (&edges[0], &edges[0] + (sizeof (edges) / sizeof (edges [0])), false);

    EXPECT_EQ (e2c.contours (), size_t (3));
    EXPECT_EQ (c2s (e2c.contour (0)), "100,100;200,100");
    EXPECT_EQ (e2c.contour_closed (0), false);
    EXPECT_EQ (c2s (e2c.contour (1)), "100,100;100,200");
    EXPECT_EQ (e2c.contour_closed (1), false);
    EXPECT_EQ (c2s (e2c.contour (2)), "0,1000;1000,1000;1000,0;0,0");
    EXPECT_EQ (e2c.contour_closed (2), true);
  }

  {
    db::EdgesToContours e2c;
    e2c.fill (&edges[0], &edges[0] + (sizeof (edges) / sizeof (edges [0])), true);

    EXPECT_EQ (e2c.contours (), size_t (2));
    EXPECT_EQ (c2s (e2c.contour (0)), "200,100;100,100;100,200");
    EXPECT_EQ (e2c.contour_closed (0), false);
    EXPECT_EQ (c2s (e2c.contour (1)), "0,1000;1000,1000;1000,0;0,0");
    EXPECT_EQ (e2c.contour_closed (1), true);
  }
}
