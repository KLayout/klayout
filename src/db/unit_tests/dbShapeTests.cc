
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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



#include "dbShapes.h"
#include "dbShape.h"
#include "dbObjectWithProperties.h"
#include "dbPropertiesRepository.h"
#include "dbLayout.h"
#include "tlUnitTest.h"


TEST(1) 
{
  db::Manager m (true);
  db::Shapes s (&m, 0, db::default_editable_mode ());

  db::Polygon p1;
  std::vector <db::Point> c1;
  c1.push_back (db::Point (0, 0));
  c1.push_back (db::Point (0, 1000));
  c1.push_back (db::Point (100, 1000));
  c1.push_back (db::Point (100, 0));
  p1.assign_hull (c1.begin (), c1.end ());

  db::Polygon p2;
  std::vector <db::Point> c2;
  c2.push_back (db::Point (2000, 0));
  c2.push_back (db::Point (2000, 1000));
  c2.push_back (db::Point (2100, 1000));
  c2.push_back (db::Point (2100, 0));
  p2.assign_hull (c2.begin (), c2.end ());

  db::Box b1 (db::Point (-100, -100), db::Point (100, 100));
  db::Box b2 (db::Point (1900, -100), db::Point (2100, 100));

  db::ShapeIterator si;

  si = db::ShapeIterator (s);
  EXPECT_EQ (si.at_end (), true);

  s.insert (p1);
  s.insert (p2);
  s.insert (b1);
  s.insert (b2);

  si = s.begin (db::ShapeIterator::All);

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), true);
  EXPECT_EQ (si->polygon () == p1, true);
  EXPECT_EQ (si->is_box (), false);

  db::ShapeIterator si_saved (si);
  ++si;

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), true);
  EXPECT_EQ (si->polygon () == p2, true);
  EXPECT_EQ (si->is_box (), false);

  ++si;

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b1, true);

  ++si;

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b2, true);

  si = si_saved;

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), true);
  EXPECT_EQ (si->polygon () == p1, true);
  EXPECT_EQ (si->is_box (), false);

  ++si;

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), true);
  EXPECT_EQ (si->polygon () == p2, true);
  EXPECT_EQ (si->is_box (), false);

  ++si;

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b1, true);

  ++si;

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b2, true);

  ++si;

  EXPECT_EQ (si.at_end (), true);

}

TEST(2) 
{
  db::Manager m (true);
  db::Shapes s (&m, 0, db::default_editable_mode ());

  db::Polygon p1;
  std::vector <db::Point> c1;
  c1.push_back (db::Point (0, 0));
  c1.push_back (db::Point (0, 1000));
  c1.push_back (db::Point (100, 1000));
  c1.push_back (db::Point (100, 0));
  p1.assign_hull (c1.begin (), c1.end ());

  db::Polygon p2;
  std::vector <db::Point> c2;
  c2.push_back (db::Point (2000, 0));
  c2.push_back (db::Point (2000, 1000));
  c2.push_back (db::Point (2100, 1000));
  c2.push_back (db::Point (2100, 0));
  p2.assign_hull (c2.begin (), c2.end ());

  db::Box b1 (db::Point (-100, -100), db::Point (100, 100));
  db::Box b2 (db::Point (1900, -100), db::Point (2100, 100));

  db::Box r (db::Point (-100, -100), db::Point (100, 100));

  db::ShapeIterator si;

  si = db::ShapeIterator (s, r, db::ShapeIterator::Touching);
  EXPECT_EQ (si.at_end (), true);

  s.insert (p1);
  s.insert (p2);
  s.insert (b1);
  s.insert (b2);
  s.sort ();

  si = s.begin_touching (r, db::ShapeIterator::All);

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), true);
  EXPECT_EQ (si->polygon () == p1, true);
  EXPECT_EQ (si->area (), p1.area ());
  EXPECT_EQ (si->perimeter (), p1.perimeter ());
  EXPECT_EQ (si->is_box (), false);

  db::ShapeIterator si_saved (si);
  ++si;

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b1, true);
  EXPECT_EQ (si->area (), b1.area ());
  EXPECT_EQ (si->perimeter (), b1.perimeter ());

  ++si;

  EXPECT_EQ (si.at_end (), true);

  si = si_saved;

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), true);
  EXPECT_EQ (si->polygon () == p1, true);
  EXPECT_EQ (si->is_box (), false);

  ++si;

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b1, true);

  ++si;

  EXPECT_EQ (si.at_end (), true);

}

TEST(3) 
{
  db::Manager m (true);
  db::Shapes s (&m, 0, db::default_editable_mode ());

  db::Polygon p1;
  std::vector <db::Point> c1;
  c1.push_back (db::Point (0, 0));
  c1.push_back (db::Point (0, 1000));
  c1.push_back (db::Point (100, 1000));
  c1.push_back (db::Point (100, 0));
  p1.assign_hull (c1.begin (), c1.end ());

  db::Polygon p2;
  std::vector <db::Point> c2;
  c2.push_back (db::Point (2000, 0));
  c2.push_back (db::Point (2000, 1000));
  c2.push_back (db::Point (2100, 1000));
  c2.push_back (db::Point (2100, 0));
  p2.assign_hull (c2.begin (), c2.end ());

  db::Box b1 (db::Point (-100, -100), db::Point (100, 100));
  db::Box b2 (db::Point (1900, -100), db::Point (2100, 100));

  db::Box r (db::Point (1900, -100), db::Point (2000, 100));

  db::ShapeIterator si;

  si = db::ShapeIterator (s, r, db::ShapeIterator::Overlapping);
  EXPECT_EQ (si.at_end (), true);

  s.insert (p1);
  s.insert (p2);
  s.insert (b1);
  s.insert (b2);
  s.sort ();

  si = db::ShapeIterator (s, r, db::ShapeIterator::Overlapping);

  db::ShapeIterator si_saved (si);

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b2, true);

  ++si;

  EXPECT_EQ (si.at_end (), true);

  si = si_saved;

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b2, true);

  ++si;

  EXPECT_EQ (si.at_end (), true);

}

TEST(1BOX) 
{
  db::Manager m (true);
  db::Shapes s (&m, 0, db::default_editable_mode ());

  db::Polygon p1;
  std::vector <db::Point> c1;
  c1.push_back (db::Point (0, 0));
  c1.push_back (db::Point (0, 1000));
  c1.push_back (db::Point (100, 1000));
  c1.push_back (db::Point (100, 0));
  p1.assign_hull (c1.begin (), c1.end ());

  db::Polygon p2;
  std::vector <db::Point> c2;
  c2.push_back (db::Point (2000, 0));
  c2.push_back (db::Point (2000, 1000));
  c2.push_back (db::Point (2100, 1000));
  c2.push_back (db::Point (2100, 0));
  p2.assign_hull (c2.begin (), c2.end ());

  db::Box b1 (db::Point (-100, -100), db::Point (100, 100));
  db::Box b2 (db::Point (1900, -100), db::Point (2100, 100));

  db::ShapeIterator si;

  si = db::ShapeIterator (s, db::ShapeIterator::Boxes);
  EXPECT_EQ (si.at_end (), true);

  s.insert (p1);
  s.insert (p2);
  s.insert (b1);
  s.insert (b2);

  si = s.begin (db::ShapeIterator::Boxes);

  db::ShapeIterator si_saved (si);

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b1, true);

  ++si;

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b2, true);

  si = si_saved;

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b1, true);

  ++si;

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b2, true);

  ++si;

  EXPECT_EQ (si.at_end (), true);

}

TEST(2BOX) 
{
  db::Manager m (true);
  db::Shapes s (&m, 0, db::default_editable_mode ());

  db::Polygon p1;
  std::vector <db::Point> c1;
  c1.push_back (db::Point (0, 0));
  c1.push_back (db::Point (0, 1000));
  c1.push_back (db::Point (100, 1000));
  c1.push_back (db::Point (100, 0));
  p1.assign_hull (c1.begin (), c1.end ());

  db::Polygon p2;
  std::vector <db::Point> c2;
  c2.push_back (db::Point (2000, 0));
  c2.push_back (db::Point (2000, 1000));
  c2.push_back (db::Point (2100, 1000));
  c2.push_back (db::Point (2100, 0));
  p2.assign_hull (c2.begin (), c2.end ());

  db::Box b1 (db::Point (-100, -100), db::Point (100, 100));
  db::Box b2 (db::Point (1900, -100), db::Point (2100, 100));

  db::Box r (db::Point (-100, -100), db::Point (100, 100));

  db::ShapeIterator si;

  si = db::ShapeIterator (s, r, db::ShapeIterator::Touching, db::ShapeIterator::Boxes);
  EXPECT_EQ (si.at_end (), true);

  s.insert (p1);
  s.insert (p2);
  s.insert (b1);
  s.insert (b2);
  s.sort ();

  si = s.begin_touching (r, db::ShapeIterator::Boxes);

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b1, true);

  db::ShapeIterator si_saved (si);

  ++si;

  EXPECT_EQ (si.at_end (), true);

  si = si_saved;

  ++si;

  EXPECT_EQ (si.at_end (), true);

}

TEST(3BOX) 
{
  db::Manager m (true);
  db::Shapes s (&m, 0, db::default_editable_mode ());

  db::Polygon p1;
  std::vector <db::Point> c1;
  c1.push_back (db::Point (0, 0));
  c1.push_back (db::Point (0, 1000));
  c1.push_back (db::Point (100, 1000));
  c1.push_back (db::Point (100, 0));
  p1.assign_hull (c1.begin (), c1.end ());

  db::Polygon p2;
  std::vector <db::Point> c2;
  c2.push_back (db::Point (2000, 0));
  c2.push_back (db::Point (2000, 1000));
  c2.push_back (db::Point (2100, 1000));
  c2.push_back (db::Point (2100, 0));
  p2.assign_hull (c2.begin (), c2.end ());

  db::Box b1 (db::Point (-100, -100), db::Point (100, 100));
  db::Box b2 (db::Point (1900, -100), db::Point (2100, 100));

  db::Box r (db::Point (1900, -100), db::Point (2000, 100));

  db::ShapeIterator si;

  si = db::ShapeIterator (s, r, db::ShapeIterator::Overlapping, db::ShapeIterator::Boxes);
  EXPECT_EQ (si.at_end (), true);

  s.insert (p1);
  s.insert (p2);
  s.insert (b1);
  s.insert (b2);
  s.sort ();

  si = db::ShapeIterator (s, r, db::ShapeIterator::Overlapping, db::ShapeIterator::Boxes);

  db::ShapeIterator si_saved (si);

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b2, true);

  ++si;

  EXPECT_EQ (si.at_end (), true);

  si = si_saved;

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b2, true);

  ++si;

  EXPECT_EQ (si.at_end (), true);

}

TEST(1SBOX) 
{
  db::Manager m (true);
  db::Shapes s (&m, 0, db::default_editable_mode ());

  db::Polygon p1;
  std::vector <db::Point> c1;
  c1.push_back (db::Point (0, 0));
  c1.push_back (db::Point (0, 1000));
  c1.push_back (db::Point (100, 1000));
  c1.push_back (db::Point (100, 0));
  p1.assign_hull (c1.begin (), c1.end ());

  db::Polygon p2;
  std::vector <db::Point> c2;
  c2.push_back (db::Point (2000, 0));
  c2.push_back (db::Point (2000, 1000));
  c2.push_back (db::Point (2100, 1000));
  c2.push_back (db::Point (2100, 0));
  p2.assign_hull (c2.begin (), c2.end ());

  db::Box b1 (db::Point (-100, -100), db::Point (100, 100));
  db::Box b2 (db::Point (1900, -100), db::Point (2100, 100));

  db::ShapeIterator si;

  si = db::ShapeIterator (s, db::ShapeIterator::Boxes);
  EXPECT_EQ (si.at_end (), true);

  s.insert (p1);
  s.insert (p2);
  s.insert (b1);
  s.insert (b2);

  si = s.begin (db::ShapeIterator::Boxes);

  db::ShapeIterator si_saved (si);

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b1, true);

  ++si;

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b2, true);

  si = si_saved;

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b1, true);

  ++si;

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b2, true);

  ++si;

  EXPECT_EQ (si.at_end (), true);

}

TEST(2SBOX) 
{
  db::Manager m (true);
  db::Shapes s (&m, 0, db::default_editable_mode ());

  db::Polygon p1;
  std::vector <db::Point> c1;
  c1.push_back (db::Point (0, 0));
  c1.push_back (db::Point (0, 1000));
  c1.push_back (db::Point (100, 1000));
  c1.push_back (db::Point (100, 0));
  p1.assign_hull (c1.begin (), c1.end ());

  db::Polygon p2;
  std::vector <db::Point> c2;
  c2.push_back (db::Point (2000, 0));
  c2.push_back (db::Point (2000, 1000));
  c2.push_back (db::Point (2100, 1000));
  c2.push_back (db::Point (2100, 0));
  p2.assign_hull (c2.begin (), c2.end ());

  db::Box b1 (db::Point (-100, -100), db::Point (100, 100));
  db::Box b2 (db::Point (1900, -100), db::Point (2100, 100));

  db::Box r (db::Point (-100, -100), db::Point (100, 100));

  db::ShapeIterator si;

  si = db::ShapeIterator (s, r, db::ShapeIterator::Touching, db::ShapeIterator::Boxes);
  EXPECT_EQ (si.at_end (), true);

  s.insert (p1);
  s.insert (p2);
  s.insert (b1);
  s.insert (b2);
  s.sort ();

  si = s.begin_touching (r, db::ShapeIterator::Boxes);

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b1, true);

  db::ShapeIterator si_saved (si);

  ++si;

  EXPECT_EQ (si.at_end (), true);

  si = si_saved;

  ++si;

  EXPECT_EQ (si.at_end (), true);

}

TEST(3SBOX) 
{
  db::Manager m (true);
  db::Shapes s (&m, 0, db::default_editable_mode ());

  db::Polygon p1;
  std::vector <db::Point> c1;
  c1.push_back (db::Point (0, 0));
  c1.push_back (db::Point (0, 1000));
  c1.push_back (db::Point (100, 1000));
  c1.push_back (db::Point (100, 0));
  p1.assign_hull (c1.begin (), c1.end ());

  db::Polygon p2;
  std::vector <db::Point> c2;
  c2.push_back (db::Point (2000, 0));
  c2.push_back (db::Point (2000, 1000));
  c2.push_back (db::Point (2100, 1000));
  c2.push_back (db::Point (2100, 0));
  p2.assign_hull (c2.begin (), c2.end ());

  db::Box b1 (db::Point (-100, -100), db::Point (100, 100));
  db::Box b2 (db::Point (1900, -100), db::Point (2100, 100));

  db::Box r (db::Point (1900, -100), db::Point (2000, 100));

  db::ShapeIterator si;

  si = db::ShapeIterator (s, r, db::ShapeIterator::Overlapping, db::ShapeIterator::Boxes);
  EXPECT_EQ (si.at_end (), true);

  s.insert (p1);
  s.insert (p2);
  s.insert (b1);
  s.insert (b2);
  s.sort ();

  si = db::ShapeIterator (s, r, db::ShapeIterator::Overlapping, db::ShapeIterator::Boxes);

  db::ShapeIterator si_saved (si);

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b2, true);

  ++si;

  EXPECT_EQ (si.at_end (), true);

  si = si_saved;

  EXPECT_EQ (si.at_end (), false);
  EXPECT_EQ (si->is_polygon (), false);
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box () == b2, true);

  ++si;

  EXPECT_EQ (si.at_end (), true);

}

TEST(4) 
{
  db::Manager m (true);
  db::Shapes s (&m, 0, db::default_editable_mode ());

  db::Box bx1 (db::Point (0, 0), db::Point (1000, 100));
  db::Box bx2 (db::Point (0, 1000), db::Point (100, 2000));
  s.insert (bx1);
  s.insert (bx2);

  db::ShapeIterator si (s.begin (db::ShapeIterator::Polygons | db::ShapeIterator::Boxes));

  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box (), bx1);
  EXPECT_EQ (si->area (), bx1.area ());
  EXPECT_EQ (si->perimeter (), bx1.perimeter ());

  ++si;
  
  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box (), bx2);
  EXPECT_EQ (si->area (), bx2.area ());
  EXPECT_EQ (si->perimeter (), bx2.perimeter ());

  ++si;
  
  EXPECT_EQ (si.at_end (), true);
}

TEST(5) 
{
  if (db::default_editable_mode ()) { return; } //  currently Boxes are treated as ones with properties
  db::Manager m (true);
  db::Shapes s (&m, 0, db::default_editable_mode ());

  db::Box bx1 (db::Point (0, 0), db::Point (1000, 100));
  db::Box bx2 (db::Point (0, 1000), db::Point (100, 2000));
  s.insert (db::object_with_properties <db::Box> (bx1, 17));
  s.insert (bx2);

  db::ShapeIterator si (s.begin (db::ShapeIterator::Polygons | db::ShapeIterator::Boxes));
  
  EXPECT_EQ (si.at_end (), false);

  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box (), bx2);
  EXPECT_EQ (si->area (), bx2.area());
  EXPECT_EQ (si->perimeter (), bx2.perimeter());
  EXPECT_EQ (si->has_prop_id (), false);

  ++si;

  EXPECT_EQ (si.at_end (), false);

  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box (), bx1);
  EXPECT_EQ (si->area (), bx1.area());
  EXPECT_EQ (si->perimeter (), bx1.perimeter());
  EXPECT_EQ (si->has_prop_id (), true);
  EXPECT_EQ (si->prop_id (), size_t (17));

  ++si;
  
  EXPECT_EQ (si.at_end (), true);
}

TEST(6) 
{
  db::Manager m (true);
  db::Shapes s (&m, 0, db::default_editable_mode ());

  db::Box bx1 (db::Point (0, 0), db::Point (1000, 100));
  s.insert (db::object_with_properties <db::Box> (bx1, 17));

  db::ShapeIterator si (s.begin (db::ShapeIterator::Polygons | db::ShapeIterator::Boxes));

  EXPECT_EQ (si.at_end (), false);

  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box (), bx1);
  EXPECT_EQ (si->area (), bx1.area ());
  EXPECT_EQ (si->perimeter (), bx1.perimeter ());
  EXPECT_EQ (si->has_prop_id (), true);
  EXPECT_EQ (si->prop_id (), size_t (17));

  ++si;
  
  EXPECT_EQ (si.at_end (), true);
}

TEST(7) 
{
  if (db::default_editable_mode ()) { return; } //  currently Boxes are treated as ones with properties
  db::Manager m (true);
  db::Shapes s (&m, 0, db::default_editable_mode ());

  db::Box bx2 (db::Point (0, 1000), db::Point (100, 2000));
  s.insert (bx2);

  db::ShapeIterator si (s.begin (db::ShapeIterator::Polygons | db::ShapeIterator::Boxes));

  EXPECT_EQ (si.at_end (), false);

  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box (), bx2);
  EXPECT_EQ (si->has_prop_id (), false);

  ++si;
  
  EXPECT_EQ (si.at_end (), true);
}

TEST(8) 
{
  db::Manager m (true);
  db::Shapes s (&m, 0, db::default_editable_mode ());
  db::PropertiesRepository proprep;

  db::PropertiesRepository::properties_set set1;
  set1.insert (std::make_pair (0, tl::Variant (15l)));
  db::PropertiesRepository rep;
  size_t id1 = rep.properties_id (set1);

  db::BoxWithProperties bx2 = db::BoxWithProperties (db::Box (db::Point (0, 1000), db::Point (100, 2000)), id1);
  s.insert (bx2);

  db::ShapeIterator si (s.begin (db::ShapeIterator::Polygons | db::ShapeIterator::Boxes));

  EXPECT_EQ (si.at_end (), false); 

  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box (), bx2);
  EXPECT_EQ (si->has_prop_id (), true);

  ++si;
  
  EXPECT_EQ (si.at_end (), true);

  std::set<size_t> prop_sel;
  si = db::ShapeIterator (s.begin (db::ShapeIterator::Polygons | db::ShapeIterator::Boxes, &prop_sel, false));

  EXPECT_EQ (si.at_end (), true); 

  prop_sel.insert (22);
  si = db::ShapeIterator (s.begin (db::ShapeIterator::Polygons | db::ShapeIterator::Boxes, &prop_sel, false));

  EXPECT_EQ (si.at_end (), true); 

  prop_sel.insert (id1);
  si = db::ShapeIterator (s.begin (db::ShapeIterator::Polygons | db::ShapeIterator::Boxes, &prop_sel, false));

  EXPECT_EQ (si.at_end (), false); 

  EXPECT_EQ (si->is_box (), true);
  EXPECT_EQ (si->box (), bx2);
  EXPECT_EQ (si->has_prop_id (), true);

  ++si;
  
  EXPECT_EQ (si.at_end (), true);

}

