
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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


#include "dbShape.h"
#include "dbShapes.h"
#include "dbObjectWithProperties.h"
#include "dbPropertiesRepository.h"
#include "dbLayout.h"
#include "tlUnitTest.h"

TEST(1)
{
  if (db::default_editable_mode ()) { return; } // does not work in editable mode because polygon arrays are expanded

  db::GenericRepository *rep = new db::GenericRepository ();

  std::vector <db::Point> c1;
  c1.push_back (db::Point (100, 0));
  c1.push_back (db::Point (100, 1000));
  c1.push_back (db::Point (200, 2000));
  c1.push_back (db::Point (200, 0));

  db::Polygon p1;
  p1.assign_hull (c1.begin (), c1.end ());

  db::SimplePolygon p2;
  p2.assign_hull (c1.begin (), c1.end ());

  db::Shapes shapes (db::default_editable_mode ());
  shapes.insert (p1);
  shapes.insert (db::array<db::SimplePolygonPtr, db::Disp> (db::SimplePolygonPtr (p2, *rep), db::Disp (db::Vector (100, -200)),
                                                            db::Vector (2000, 0), db::Vector (0, -2500), 2, 3));
  shapes.insert (p2);
  shapes.insert (db::SimplePolygonRef (p2, *rep));

  db::Shapes::shape_iterator s = shapes.begin (db::ShapeIterator::All);

  db::Polygon pref [9] = {
    p1, p1, p1, 
    db::Trans (0, false, db::Vector (0, 0) + db::Vector (100, -200)) * p1,
    db::Trans (0, false, db::Vector (2000, 0) + db::Vector (100, -200)) * p1,
    db::Trans (0, false, db::Vector (0, -2500) + db::Vector (100, -200)) * p1,
    db::Trans (0, false, db::Vector (2000, -2500) + db::Vector (100, -200)) * p1,
    db::Trans (0, false, db::Vector (0, -5000) + db::Vector (100, -200)) * p1,
    db::Trans (0, false, db::Vector (2000, -5000) + db::Vector (100, -200)) * p1,
  };

  db::Shapes::shape_iterator scopy [9];

  unsigned int n = 0;
  while (! s.at_end ()) {

    EXPECT_EQ (n < 9, true);
    
    scopy[n] = s;

    db::Polygon px;
    s->polygon (px);
    EXPECT_EQ (px == pref[n], true);

    ++s;
    ++n;
  }

  EXPECT_EQ (n, (unsigned int) 9);
  EXPECT_EQ (s.at_end (), true);

  for (unsigned int i = 0; i < n; ++i) {
    db::Polygon px;
    scopy[i]->polygon (px);
    EXPECT_EQ (px == pref[i], true);
  }

  delete rep;
  rep = 0;
}


TEST(2)
{
  db::GenericRepository *rep = new db::GenericRepository ();

  std::vector <db::Point> c1;
  c1.push_back (db::Point (100, 0));
  c1.push_back (db::Point (100, 1000));
  c1.push_back (db::Point (200, 2000));
  c1.push_back (db::Point (200, 0));

  db::Polygon p1;
  p1.assign_hull (c1.begin (), c1.end ());

  db::SimplePolygon p2;
  p2.assign_hull (c1.begin (), c1.end ());

  db::Shapes shapes (db::default_editable_mode ());
  shapes.insert (p1);
  shapes.insert (db::array<db::SimplePolygonPtr, db::Disp> (db::SimplePolygonPtr (p2, *rep), db::Disp (db::Vector (100, -200)),
                                                            db::Vector (2000, 0), db::Vector (0, -2500), 2, 3));
  shapes.insert (p2);
  shapes.insert (db::SimplePolygonRef (p2, *rep));
  shapes.sort ();

  EXPECT_EQ (shapes.bbox () == db::Box (100,-5200,2300,2000), true);

  db::Box box (0, -3000, 100, 3000);

  for (unsigned int k = 0; k < 2; ++k) {

    db::Shapes::shape_iterator s;
    if (k == 0) {
      s = shapes.begin_touching (box, db::ShapeIterator::Polygons);
    } else {
      s = shapes.begin_overlapping (box, db::ShapeIterator::Polygons);
    }

    db::Polygon pref_org [9] = {
      p1, p1, p1, 
      db::Trans (0, false, db::Vector (0, 0) + db::Vector (100, -200)) * p1,
      db::Trans (0, false, db::Vector (2000, 0) + db::Vector (100, -200)) * p1,
      db::Trans (0, false, db::Vector (0, -2500) + db::Vector (100, -200)) * p1,
      db::Trans (0, false, db::Vector (2000, -2500) + db::Vector (100, -200)) * p1,
      db::Trans (0, false, db::Vector (0, -5000) + db::Vector (100, -200)) * p1,
      db::Trans (0, false, db::Vector (2000, -5000) + db::Vector (100, -200)) * p1,
    };

    db::Polygon pref [9];
    unsigned int nref = 0;
    for (unsigned int n = 0; n < 9; ++n) {
      if (k == 0 && pref_org [n].box ().touches (box)) {
        pref [nref++] = pref_org [n];
      } else if (k == 1 && pref_org [n].box ().overlaps (box)) {
        pref [nref++] = pref_org [n];
      }
    }

    db::Shapes::shape_iterator scopy [9];

    unsigned int n = 0;
    while (! s.at_end ()) {

      EXPECT_EQ (n < nref, true);
      
      scopy[n] = s;

      db::Polygon px;
      s->polygon (px);
      EXPECT_EQ (px == pref[n], true);

      ++s;
      ++n;
    }

    EXPECT_EQ (n, nref);
    EXPECT_EQ (s.at_end (), true);

    for (unsigned int i = 0; i < n; ++i) {
      db::Polygon px;
      scopy[i]->polygon (px);
      EXPECT_EQ (px == pref[i], true);
    }

  }

  delete rep;
  rep = 0;

}


