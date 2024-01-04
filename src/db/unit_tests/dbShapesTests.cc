
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



#include "dbLayout.h"
#include "tlTimer.h"
#include "tlUnitTest.h"
#include "dbStatic.h"


TEST(1) 
{
  db::Manager m (true);
  db::Shapes s (&m, 0, db::default_editable_mode ());
  db::Box b_empty;

  EXPECT_EQ (s.bbox (), b_empty);

  db::Box b (0, 100, 1000, 1200);
  s.insert (b);
  EXPECT_EQ (s.bbox (), b);

  db::Edge e (-100, -200, 0, 0);
  s.insert (e);
  EXPECT_EQ (s.bbox (), db::Box (-100, -200, 1000, 1200));
  
  db::Shapes s2 (s);
  EXPECT_EQ (s2.bbox (), db::Box (-100, -200, 1000, 1200));

  if (db::default_editable_mode ()) {
    s2.erase (db::Box::tag (), db::stable_layer_tag (), s2.begin (db::Box::tag (), db::stable_layer_tag ()));
    EXPECT_EQ (s2.bbox (), db::Box (-100, -200, 0, 0));
  }
}

TEST(1a) 
{
  db::Manager m (true);
  db::Shapes s (&m, 0, true);
  db::Box b_empty;

  EXPECT_EQ (s.bbox (), b_empty);

  db::Box b (0, 100, 1000, 1200);
  s.insert (b);
  EXPECT_EQ (s.bbox (), b);

  db::Edge e (-100, -200, 0, 0);
  s.insert (e);
  EXPECT_EQ (s.bbox (), db::Box (-100, -200, 1000, 1200));
  
  db::Shapes s2 (s);
  EXPECT_EQ (s2.bbox (), db::Box (-100, -200, 1000, 1200));

  s2.erase (db::Box::tag (), db::stable_layer_tag (), s2.begin (db::Box::tag (), db::stable_layer_tag ()));
  EXPECT_EQ (s2.bbox (), db::Box (-100, -200, 0, 0));
}

TEST(1b) 
{
  db::Manager m (true);
  db::Shapes s (&m, 0, false);
  db::Box b_empty;

  EXPECT_EQ (s.bbox (), b_empty);

  db::Box b (0, 100, 1000, 1200);
  s.insert (b);
  EXPECT_EQ (s.bbox (), b);

  db::Edge e (-100, -200, 0, 0);
  s.insert (e);
  EXPECT_EQ (s.bbox (), db::Box (-100, -200, 1000, 1200));
  
  db::Shapes s2 (s);
  EXPECT_EQ (s2.bbox (), db::Box (-100, -200, 1000, 1200));
}

std::string shapes_to_string_norm (tl::TestBase *_this, const db::Shapes &shapes, const db::ICplxTrans &trans)
{
  std::vector<std::string> strings;
  for (db::Shapes::shape_iterator shape = shapes.begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
    std::string r;
    if (shape->is_polygon ()) {
      db::Shape::polygon_type p;
      shape->polygon (p);
      r += "polygon " + p.transformed (trans).to_string ();
      //  check the area and bbox while we are here.
      EXPECT_EQ (p.box ().to_string (), shape->bbox ().to_string ());
      EXPECT_EQ (p.area (), shape->area ());
    } else if (shape->is_path ()) {
      db::Shape::path_type p;
      shape->path (p);
      r += "path " + p.transformed (trans).to_string ();
      EXPECT_EQ (p.box ().to_string (), shape->bbox ().to_string ());
      EXPECT_EQ (p.area (), shape->area ());
    } else if (shape->is_text ()) {
      db::Shape::text_type p;
      shape->text (p);
      r += "text " + p.transformed (trans).to_string ();
      EXPECT_EQ (p.box ().to_string (), shape->bbox ().to_string ());
      EXPECT_EQ (0.0, shape->area ());
    } else if (shape->is_box ()) {
      db::Shape::box_type p;
      shape->box (p);
      r += "box " + p.transformed (trans).to_string ();
      EXPECT_EQ (p.to_string (), shape->bbox ().to_string ());
      EXPECT_EQ (p.area (), shape->area ());
    } else {
      r += "*unknown type*";
    }
    r += " #" + tl::to_string (shape->prop_id ()) + "\n";
    strings.push_back (r);
  }
  std::sort (strings.begin (), strings.end ());
  std::string r;
  for (std::vector<std::string>::const_iterator s = strings.begin (); s != strings.end (); ++s) {
    r += *s;
  }
  return r;
}

std::string shapes_to_string_norm (tl::TestBase *_this, const db::Shapes &shapes)
{
  std::vector<std::string> strings;
  for (db::Shapes::shape_iterator shape = shapes.begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
    std::string r;
    if (shape->is_polygon ()) {
      db::Shape::polygon_type p;
      shape->polygon (p);
      r += "polygon " + p.to_string ();
      //  check the area and bbox while we are here.
      EXPECT_EQ (p.box ().to_string (), shape->bbox ().to_string ());
      EXPECT_EQ (p.area (), shape->area ());
    } else if (shape->is_path ()) {
      db::Shape::path_type p;
      shape->path (p);
      r += "path " + p.to_string ();
      EXPECT_EQ (p.box ().to_string (), shape->bbox ().to_string ());
      EXPECT_EQ (p.area (), shape->area ());
    } else if (shape->is_edge ()) {
      db::Shape::edge_type p;
      shape->edge (p);
      r += "edge " + p.to_string ();
      EXPECT_EQ (p.bbox ().to_string (), shape->bbox ().to_string ());
    } else if (shape->is_edge_pair ()) {
      db::Shape::edge_pair_type p;
      shape->edge_pair (p);
      r += "edge_pair " + p.to_string ();
      EXPECT_EQ (p.bbox ().to_string (), shape->bbox ().to_string ());
    } else if (shape->is_text ()) {
      db::Shape::text_type p;
      shape->text (p);
      r += "text " + p.to_string ();
      EXPECT_EQ (p.box ().to_string (), shape->bbox ().to_string ());
      EXPECT_EQ (0.0, shape->area ());
    } else if (shape->is_box ()) {
      db::Shape::box_type p;
      shape->box (p);
      r += "box " + p.to_string ();
      EXPECT_EQ (p.to_string (), shape->bbox ().to_string ());
      EXPECT_EQ (p.area (), shape->area ());
    } else {
      r += "*unknown type*";
    }
    r += " #" + tl::to_string (shape->prop_id ()) + "\n";
    strings.push_back (r);
  }
  std::sort (strings.begin (), strings.end ());
  std::string r;
  for (std::vector<std::string>::const_iterator s = strings.begin (); s != strings.end (); ++s) {
    r += *s;
  }
  return r;
}

std::string shapes_to_string (tl::TestBase *_this, const db::Shapes &shapes)
{
  std::string r;
  for (db::Shapes::shape_iterator shape = shapes.begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
    if (shape->is_polygon ()) {
      db::Shape::polygon_type p;
      shape->polygon (p);
      r += "polygon " + p.to_string ();
      //  check the area and bbox while we are here.
      EXPECT_EQ (p.box ().to_string (), shape->bbox ().to_string ());
      EXPECT_EQ (p.area (), shape->area ());
    } else if (shape->is_path ()) {
      db::Shape::path_type p;
      shape->path (p);
      r += "path " + p.to_string ();
      EXPECT_EQ (p.box ().to_string (), shape->bbox ().to_string ());
      EXPECT_EQ (p.area (), shape->area ());
    } else if (shape->is_text ()) {
      db::Shape::text_type p;
      shape->text (p);
      r += "text " + p.to_string ();
      EXPECT_EQ (p.box ().to_string (), shape->bbox ().to_string ());
      EXPECT_EQ (0.0, shape->area ());
    } else if (shape->is_box ()) {
      db::Shape::box_type p;
      shape->box (p);
      r += "box " + p.to_string ();
      EXPECT_EQ (p.to_string (), shape->bbox ().to_string ());
      EXPECT_EQ (p.area (), shape->area ());
    } else {
      r += "*unknown type*";
    }
    if (shape->is_array_member ()) {
      r += " [" + shape->array_trans ().to_string () + "]";
    }
    r += " #" + tl::to_string (shape->prop_id ()) + "\n";
  }
  return r;
}

unsigned int read_testdata (db::Layout &layout, unsigned int what = 0xff)
{
  db::cell_index_type top = layout.add_cell ("TOP");
  db::Cell &top_cell = layout.cell (top);
  unsigned int layer_id = layout.insert_layer ();
  db::Shapes &shapes = top_cell.shapes (layer_id);

  bool with_arrays = ((what & 0x80) == 0);

  if ((what & 0x1) != 0) {
    static db::SimplePolygon p1 (db::Box (0, 100, 1000, 2000));
    static db::SimplePolygon p2 (db::Box (100, 200, 1100, 2100));
    static db::SimplePolygon p3 (db::Box (150, 150, 1150, 2050));

    shapes.insert (p1);
    shapes.insert (p2);
    shapes.insert (p3);
    shapes.insert (db::Shape::simple_polygon_ref_type (&p1, db::Trans (db::Vector (-10, 15))));
    shapes.insert (db::Shape::simple_polygon_ref_type (&p2, db::Trans (db::Vector (-110, 115))));
    shapes.insert (db::Shape::simple_polygon_ref_type (&p3, db::Trans (db::Vector (-210, 215))));
    if (with_arrays) shapes.insert (db::Shape::simple_polygon_ptr_array_type (db::Shape::simple_polygon_ptr_type (&p1, db::UnitTrans ()), db::Disp (db::Vector (0, 5)), db::Vector (0, 10000), db::Vector (11000, 0), 3, 4));
    
    shapes.insert (db::object_with_properties<db::SimplePolygon> (p1, 1));
    shapes.insert (db::object_with_properties<db::SimplePolygon> (p2, 2));
    shapes.insert (db::object_with_properties<db::SimplePolygon> (p3, 3));
    shapes.insert (db::object_with_properties<db::Shape::simple_polygon_ref_type> (db::Shape::simple_polygon_ref_type (&p1, db::Trans (db::Vector (-10, 15))), 5));
    shapes.insert (db::object_with_properties<db::Shape::simple_polygon_ref_type> (db::Shape::simple_polygon_ref_type (&p2, db::Trans (db::Vector (-110, 115))), 6));
    shapes.insert (db::object_with_properties<db::Shape::simple_polygon_ref_type> (db::Shape::simple_polygon_ref_type (&p3, db::Trans (db::Vector (-210, 215))), 7));
    if (with_arrays) shapes.insert (db::object_with_properties<db::Shape::simple_polygon_ptr_array_type> (db::Shape::simple_polygon_ptr_array_type (db::Shape::simple_polygon_ptr_type (&p1, db::UnitTrans ()), db::Disp (db::Vector (0, 5)), db::Vector (0, 10000), db::Vector (11000, 0), 3, 4), 8));
  }

  if ((what & 0x2) != 0) {
    static db::Polygon q1 (db::Box (0, 100, 2000, 1000));
    static db::Polygon q2 (db::Box (100, 200, 2100, 1100));
    static db::Polygon q3 (db::Box (150, 150, 2150, 1050));

    shapes.insert (q1);
    shapes.insert (q2);
    shapes.insert (q3);
    shapes.insert (db::Shape::polygon_ref_type (&q1, db::Trans (db::Vector (-10, 15))));
    shapes.insert (db::Shape::polygon_ref_type (&q2, db::Trans (db::Vector (-110, 115))));
    shapes.insert (db::Shape::polygon_ref_type (&q3, db::Trans (db::Vector (-210, 215))));
    if (with_arrays) shapes.insert (db::Shape::polygon_ptr_array_type (db::Shape::polygon_ptr_type (&q1, db::UnitTrans ()), db::Disp (db::Vector (0, 5)), db::Vector (0, 10000), db::Vector (11000, 0), 3, 4));
    
    shapes.insert (db::object_with_properties<db::Polygon> (q1, 1));
    shapes.insert (db::object_with_properties<db::Polygon> (q2, 2));
    shapes.insert (db::object_with_properties<db::Polygon> (q3, 3));
    shapes.insert (db::object_with_properties<db::Shape::polygon_ref_type> (db::Shape::polygon_ref_type (&q1, db::Trans (db::Vector (-10, 15))), 5));
    shapes.insert (db::object_with_properties<db::Shape::polygon_ref_type> (db::Shape::polygon_ref_type (&q2, db::Trans (db::Vector (-110, 115))), 6));
    shapes.insert (db::object_with_properties<db::Shape::polygon_ref_type> (db::Shape::polygon_ref_type (&q3, db::Trans (db::Vector (-210, 215))), 7));
    if (with_arrays) shapes.insert (db::object_with_properties<db::Shape::polygon_ptr_array_type> (db::Shape::polygon_ptr_array_type (db::Shape::polygon_ptr_type (&q1, db::UnitTrans ()), db::Disp (db::Vector (0, 5)), db::Vector (0, 10000), db::Vector (11000, 0), 3, 4), 8));
  }

  if ((what & 0x4) != 0) {
    static db::Path r1;
    static db::Path r2;
    static db::Path r3;
    db::Point pts1 [] = { db::Point (0, 100), db::Point (0, 500), db::Point (200, 700) };
    db::Point pts2 [] = { db::Point (0, 1100), db::Point (0, 1500), db::Point (200, 1300) };
    db::Point pts3 [] = { db::Point (0, 2100), db::Point (0, 2500), db::Point (-200, 2700) };
    r1 = db::Path (pts1, pts1 + 3, 100);
    r2 = db::Path (pts2, pts2 + 3, 150);
    r3 = db::Path (pts3, pts3 + 3, 200);

    shapes.insert (r1);
    shapes.insert (r2);
    shapes.insert (r3);
    shapes.insert (db::Shape::path_ref_type (&r1, db::Trans (db::Vector (-10, 15))));
    shapes.insert (db::Shape::path_ref_type (&r2, db::Trans (db::Vector (-110, 115))));
    shapes.insert (db::Shape::path_ref_type (&r3, db::Trans (db::Vector (-210, 215))));
    if (with_arrays) shapes.insert (db::Shape::path_ptr_array_type (db::Shape::path_ptr_type (&r1, db::UnitTrans ()), db::Disp (db::Vector (0, 5)), db::Vector (0, 10000), db::Vector (11000, 0), 3, 4));
    
    shapes.insert (db::object_with_properties<db::Path> (r1, 1));
    shapes.insert (db::object_with_properties<db::Path> (r2, 2));
    shapes.insert (db::object_with_properties<db::Path> (r3, 3));
    shapes.insert (db::object_with_properties<db::Shape::path_ref_type> (db::Shape::path_ref_type (&r1, db::Trans (db::Vector (-10, 15))), 5));
    shapes.insert (db::object_with_properties<db::Shape::path_ref_type> (db::Shape::path_ref_type (&r2, db::Trans (db::Vector (-110, 115))), 6));
    shapes.insert (db::object_with_properties<db::Shape::path_ref_type> (db::Shape::path_ref_type (&r3, db::Trans (db::Vector (-210, 215))), 7));
    if (with_arrays) shapes.insert (db::object_with_properties<db::Shape::path_ptr_array_type> (db::Shape::path_ptr_array_type (db::Shape::path_ptr_type (&r1, db::UnitTrans ()), db::Disp (db::Vector (0, 5)), db::Vector (0, 10000), db::Vector (11000, 0), 3, 4), 8));
  }

  if ((what & 0x8) != 0) {
    static db::Text t1 ("A", db::Trans (0, db::Vector (10, 35)));
    static db::Text t2 ("B", db::Trans (1, db::Vector (20, 25)));
    static db::Text t3 ("C", db::Trans (6, db::Vector (30, 15)));

    shapes.insert (t1);
    shapes.insert (t2);
    shapes.insert (t3);
    shapes.insert (db::Shape::text_ref_type (&t1, db::Disp (db::Vector (-10, 15))));
    shapes.insert (db::Shape::text_ref_type (&t2, db::Disp (db::Vector (-110, 115))));
    shapes.insert (db::Shape::text_ref_type (&t3, db::Disp (db::Vector (-210, 215))));
    if (with_arrays) shapes.insert (db::Shape::text_ptr_array_type (db::Shape::text_ptr_type (&t1, db::UnitTrans ()), db::Disp (db::Vector (0, 5)), db::Vector (0, 10000), db::Vector (11000, 0), 3, 4));
    
    shapes.insert (db::object_with_properties<db::Text> (t1, 1));
    shapes.insert (db::object_with_properties<db::Text> (t2, 2));
    shapes.insert (db::object_with_properties<db::Text> (t3, 3));
    shapes.insert (db::object_with_properties<db::Shape::text_ref_type> (db::Shape::text_ref_type (&t1, db::Disp (db::Vector (-10, 15))), 5));
    shapes.insert (db::object_with_properties<db::Shape::text_ref_type> (db::Shape::text_ref_type (&t2, db::Disp (db::Vector (-110, 115))), 6));
    shapes.insert (db::object_with_properties<db::Shape::text_ref_type> (db::Shape::text_ref_type (&t3, db::Disp (db::Vector (-210, 215))), 7));
    if (with_arrays) shapes.insert (db::object_with_properties<db::Shape::text_ptr_array_type> (db::Shape::text_ptr_array_type (db::Shape::text_ptr_type (&t1, db::UnitTrans ()), db::Disp (db::Vector (0, 5)), db::Vector (0, 10000), db::Vector (11000, 0), 3, 4), 8));
  }

  if ((what & 0x10) != 0) {
    static db::Box b1 (0, 100, 2000, 1000);
    static db::Box b2 (100, 200, 2100, 1100);
    static db::Box b3 (150, 150, 2150, 1050);

    shapes.insert (b1);
    shapes.insert (b2);
    shapes.insert (b3);
    if (with_arrays) shapes.insert (db::Shape::box_array_type (db::Box (50, -50, 1050, -1050), db::UnitTrans (), db::Vector (0, 10000), db::Vector (11000, 0), 3, 4));
    
    shapes.insert (db::object_with_properties<db::Box> (b1, 10));
    shapes.insert (db::object_with_properties<db::Box> (b2, 11));
    shapes.insert (db::object_with_properties<db::Box> (b3, 12));
    if (with_arrays) shapes.insert (db::object_with_properties<db::Shape::box_array_type> (db::Shape::box_array_type (db::Box (50, -50, 1050, -1050), db::UnitTrans (), db::Vector (0, 10000), db::Vector (11000, 0), 3, 4), 20));
  }

  if ((what & 0x20) != 0) {
    static db::ShortBox s1 (0, 100, 2000, 1000);
    static db::ShortBox s2 (100, 200, 2100, 1100);
    static db::ShortBox s3 (150, 150, 2150, 1050);

    shapes.insert (s1);
    shapes.insert (s2);
    shapes.insert (s3);
    if (with_arrays) shapes.insert (db::Shape::short_box_array_type (db::ShortBox (50, -50, 1050, -1050), db::UnitTrans (), db::Vector (0, 10000), db::Vector (11000, 0), 3, 4));
    
    shapes.insert (db::object_with_properties<db::ShortBox> (s1, 10));
    shapes.insert (db::object_with_properties<db::ShortBox> (s2, 11));
    shapes.insert (db::object_with_properties<db::ShortBox> (s3, 12));
    if (with_arrays) shapes.insert (db::object_with_properties<db::Shape::short_box_array_type> (db::Shape::short_box_array_type (db::ShortBox (50, -50, 1050, -1050), db::UnitTrans (), db::Vector (0, 10000), db::Vector (11000, 0), 3, 4), 20));
  }
  
  return layer_id;
}

struct plus1 
{
  db::Layout::properties_id_type operator() (db::Layout::properties_id_type i) const { return i+1; }
};

TEST(2)
{
  db::Manager m (true);

  db::Layout other_layout (&m);
  db::Cell &other_topcell = other_layout.cell (other_layout.add_cell ());

  db::Layout layout (&m);
  unsigned int lindex = read_testdata (layout, 0x1);

  db::Cell &topcell = layout.cell (*layout.begin_top_down ());
  db::Shapes copy (&m, &topcell, db::default_editable_mode ());

  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)),
    "polygon (-10,115;-10,2015;990,2015;990,115) #0\n"
    "polygon (-10,115;-10,2015;990,2015;990,115) #5\n"
    "polygon (-10,315;-10,2215;990,2215;990,315) #0\n"
    "polygon (-10,315;-10,2215;990,2215;990,315) #6\n"
    "polygon (-60,365;-60,2265;940,2265;940,365) #0\n"
    "polygon (-60,365;-60,2265;940,2265;940,365) #7\n"
    "polygon (0,100;0,2000;1000,2000;1000,100) #0\n"
    "polygon (0,100;0,2000;1000,2000;1000,100) #1\n"
    "polygon (0,10105;0,12005;1000,12005;1000,10105) #0\n"
    "polygon (0,10105;0,12005;1000,12005;1000,10105) #8\n"
    "polygon (0,105;0,2005;1000,2005;1000,105) #0\n"
    "polygon (0,105;0,2005;1000,2005;1000,105) #8\n"
    "polygon (0,20105;0,22005;1000,22005;1000,20105) #0\n"
    "polygon (0,20105;0,22005;1000,22005;1000,20105) #8\n"
    "polygon (100,200;100,2100;1100,2100;1100,200) #0\n"
    "polygon (100,200;100,2100;1100,2100;1100,200) #2\n"
    "polygon (11000,10105;11000,12005;12000,12005;12000,10105) #0\n"
    "polygon (11000,10105;11000,12005;12000,12005;12000,10105) #8\n"
    "polygon (11000,105;11000,2005;12000,2005;12000,105) #0\n"
    "polygon (11000,105;11000,2005;12000,2005;12000,105) #8\n"
    "polygon (11000,20105;11000,22005;12000,22005;12000,20105) #0\n"
    "polygon (11000,20105;11000,22005;12000,22005;12000,20105) #8\n"
    "polygon (150,150;150,2050;1150,2050;1150,150) #0\n"
    "polygon (150,150;150,2050;1150,2050;1150,150) #3\n"
    "polygon (22000,10105;22000,12005;23000,12005;23000,10105) #0\n"
    "polygon (22000,10105;22000,12005;23000,12005;23000,10105) #8\n"
    "polygon (22000,105;22000,2005;23000,2005;23000,105) #0\n"
    "polygon (22000,105;22000,2005;23000,2005;23000,105) #8\n"
    "polygon (22000,20105;22000,22005;23000,22005;23000,20105) #0\n"
    "polygon (22000,20105;22000,22005;23000,22005;23000,20105) #8\n"
    "polygon (33000,10105;33000,12005;34000,12005;34000,10105) #0\n"
    "polygon (33000,10105;33000,12005;34000,12005;34000,10105) #8\n"
    "polygon (33000,105;33000,2005;34000,2005;34000,105) #0\n"
    "polygon (33000,105;33000,2005;34000,2005;34000,105) #8\n"
    "polygon (33000,20105;33000,22005;34000,22005;34000,20105) #0\n"
    "polygon (33000,20105;33000,22005;34000,22005;34000,20105) #8\n"
  );

  // tests simple copy
  copy = topcell.shapes(lindex);
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, copy));

  // tests translate
  db::Shapes other_copy (&m, &other_topcell, db::default_editable_mode ());
  other_copy = topcell.shapes(lindex);
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, other_copy));

  // tests translate plus transform
  other_copy.assign_transformed (topcell.shapes(lindex), db::ICplxTrans (2.0));
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, other_copy, db::ICplxTrans (0.5)));

  // tests deref
  db::Shapes sb_copy;
  sb_copy = topcell.shapes(lindex);
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, sb_copy));

  // tests deref plus transform
  sb_copy.assign_transformed (topcell.shapes(lindex), db::ICplxTrans (2.0));
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, sb_copy, db::ICplxTrans (0.5)));

  tl::ident_map<db::Layout::properties_id_type> pm;

  copy.clear ();
  for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
    copy.insert (*shape, pm);
  }
  EXPECT_EQ (shapes_to_string (_this, copy),
    "polygon (0,100;0,2000;1000,2000;1000,100) #0\n"
    "polygon (100,200;100,2100;1100,2100;1100,200) #0\n"
    "polygon (150,150;150,2050;1150,2050;1150,150) #0\n"
    "polygon (0,100;0,2000;1000,2000;1000,100) #1\n"
    "polygon (100,200;100,2100;1100,2100;1100,200) #2\n"
    "polygon (150,150;150,2050;1150,2050;1150,150) #3\n"
    "polygon (-10,115;-10,2015;990,2015;990,115) #0\n"
    "polygon (-10,315;-10,2215;990,2215;990,315) #0\n"
    "polygon (-60,365;-60,2265;940,2265;940,365) #0\n"
    "polygon (0,105;0,2005;1000,2005;1000,105) #0\n"
    "polygon (0,10105;0,12005;1000,12005;1000,10105) #0\n"
    "polygon (0,20105;0,22005;1000,22005;1000,20105) #0\n"
    "polygon (11000,105;11000,2005;12000,2005;12000,105) #0\n"
    "polygon (11000,10105;11000,12005;12000,12005;12000,10105) #0\n"
    "polygon (11000,20105;11000,22005;12000,22005;12000,20105) #0\n"
    "polygon (22000,105;22000,2005;23000,2005;23000,105) #0\n"
    "polygon (22000,10105;22000,12005;23000,12005;23000,10105) #0\n"
    "polygon (22000,20105;22000,22005;23000,22005;23000,20105) #0\n"
    "polygon (33000,105;33000,2005;34000,2005;34000,105) #0\n"
    "polygon (33000,10105;33000,12005;34000,12005;34000,10105) #0\n"
    "polygon (33000,20105;33000,22005;34000,22005;34000,20105) #0\n"
    "polygon (-10,115;-10,2015;990,2015;990,115) #5\n"
    "polygon (-10,315;-10,2215;990,2215;990,315) #6\n"
    "polygon (-60,365;-60,2265;940,2265;940,365) #7\n"
    "polygon (0,105;0,2005;1000,2005;1000,105) #8\n"
    "polygon (0,10105;0,12005;1000,12005;1000,10105) #8\n"
    "polygon (0,20105;0,22005;1000,22005;1000,20105) #8\n"
    "polygon (11000,105;11000,2005;12000,2005;12000,105) #8\n"
    "polygon (11000,10105;11000,12005;12000,12005;12000,10105) #8\n"
    "polygon (11000,20105;11000,22005;12000,22005;12000,20105) #8\n"
    "polygon (22000,105;22000,2005;23000,2005;23000,105) #8\n"
    "polygon (22000,10105;22000,12005;23000,12005;23000,10105) #8\n"
    "polygon (22000,20105;22000,22005;23000,22005;23000,20105) #8\n"
    "polygon (33000,105;33000,2005;34000,2005;34000,105) #8\n"
    "polygon (33000,10105;33000,12005;34000,12005;34000,10105) #8\n"
    "polygon (33000,20105;33000,22005;34000,22005;34000,20105) #8\n"
  );

  db::Shapes sa_copy;
  for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
    sa_copy.insert (*shape);
  }
  EXPECT_EQ (shapes_to_string (_this, sa_copy),
    "polygon (0,100;0,2000;1000,2000;1000,100) #0\n"
    "polygon (100,200;100,2100;1100,2100;1100,200) #0\n"
    "polygon (150,150;150,2050;1150,2050;1150,150) #0\n"
    "polygon (-10,115;-10,2015;990,2015;990,115) #0\n"
    "polygon (-10,315;-10,2215;990,2215;990,315) #0\n"
    "polygon (-60,365;-60,2265;940,2265;940,365) #0\n"
    "polygon (0,105;0,2005;1000,2005;1000,105) #0\n"
    "polygon (0,10105;0,12005;1000,12005;1000,10105) #0\n"
    "polygon (0,20105;0,22005;1000,22005;1000,20105) #0\n"
    "polygon (11000,105;11000,2005;12000,2005;12000,105) #0\n"
    "polygon (11000,10105;11000,12005;12000,12005;12000,10105) #0\n"
    "polygon (11000,20105;11000,22005;12000,22005;12000,20105) #0\n"
    "polygon (22000,105;22000,2005;23000,2005;23000,105) #0\n"
    "polygon (22000,10105;22000,12005;23000,12005;23000,10105) #0\n"
    "polygon (22000,20105;22000,22005;23000,22005;23000,20105) #0\n"
    "polygon (33000,105;33000,2005;34000,2005;34000,105) #0\n"
    "polygon (33000,10105;33000,12005;34000,12005;34000,10105) #0\n"
    "polygon (33000,20105;33000,22005;34000,22005;34000,20105) #0\n"
    "polygon (0,100;0,2000;1000,2000;1000,100) #1\n"
    "polygon (100,200;100,2100;1100,2100;1100,200) #2\n"
    "polygon (150,150;150,2050;1150,2050;1150,150) #3\n"
    "polygon (-10,115;-10,2015;990,2015;990,115) #5\n"
    "polygon (-10,315;-10,2215;990,2215;990,315) #6\n"
    "polygon (-60,365;-60,2265;940,2265;940,365) #7\n"
    "polygon (0,105;0,2005;1000,2005;1000,105) #8\n"
    "polygon (0,10105;0,12005;1000,12005;1000,10105) #8\n"
    "polygon (0,20105;0,22005;1000,22005;1000,20105) #8\n"
    "polygon (11000,105;11000,2005;12000,2005;12000,105) #8\n"
    "polygon (11000,10105;11000,12005;12000,12005;12000,10105) #8\n"
    "polygon (11000,20105;11000,22005;12000,22005;12000,20105) #8\n"
    "polygon (22000,105;22000,2005;23000,2005;23000,105) #8\n"
    "polygon (22000,10105;22000,12005;23000,12005;23000,10105) #8\n"
    "polygon (22000,20105;22000,22005;23000,22005;23000,20105) #8\n"
    "polygon (33000,105;33000,2005;34000,2005;34000,105) #8\n"
    "polygon (33000,10105;33000,12005;34000,12005;34000,10105) #8\n"
    "polygon (33000,20105;33000,22005;34000,22005;34000,20105) #8\n"
  );

  if (db::default_editable_mode ()) { 

    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    ++shape;
    db::Shape s1 = *shape;
    for (unsigned int i = 0; i < 3; ++i) { ++shape; }
    db::Shape s2 = *shape;
    for (unsigned int i = 0; i < 3; ++i) { ++shape; }
    db::Shape s3 = *shape;
    for (unsigned int i = 0; i < 3; ++i) { ++shape; }
    db::Shape s4 = *shape;
    for (unsigned int i = 0; i < 3; ++i) { ++shape; }
    db::Shape s5 = *shape;
    for (unsigned int i = 0; i < 12; ++i) { ++shape; }
    db::Shape s6 = *shape;

    topcell.shapes (lindex).erase_shape (s1);
    topcell.shapes (lindex).erase_shape (s2);
    topcell.shapes (lindex).erase_shape (s3);
    topcell.shapes (lindex).erase_shape (s4);
    topcell.shapes (lindex).erase_shape (s5);
    topcell.shapes (lindex).erase_shape (s6);

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)),
      "polygon (0,100;0,2000;1000,2000;1000,100) #0\n"
      "polygon (150,150;150,2050;1150,2050;1150,150) #0\n"
      "polygon (0,100;0,2000;1000,2000;1000,100) #1\n"
      "polygon (150,150;150,2050;1150,2050;1150,150) #3\n"
      "polygon (-10,115;-10,2015;990,2015;990,115) #0\n"
      "polygon (-60,365;-60,2265;940,2265;940,365) #0\n"
      "polygon (0,105;0,2005;1000,2005;1000,105) #0\n"
      "polygon (0,20105;0,22005;1000,22005;1000,20105) #0\n"
      "polygon (11000,105;11000,2005;12000,2005;12000,105) #0\n"
      "polygon (11000,20105;11000,22005;12000,22005;12000,20105) #0\n"
      "polygon (22000,105;22000,2005;23000,2005;23000,105) #0\n"
      "polygon (22000,10105;22000,12005;23000,12005;23000,10105) #0\n"
      "polygon (22000,20105;22000,22005;23000,22005;23000,20105) #0\n"
      "polygon (33000,105;33000,2005;34000,2005;34000,105) #0\n"
      "polygon (33000,10105;33000,12005;34000,12005;34000,10105) #0\n"
      "polygon (33000,20105;33000,22005;34000,22005;34000,20105) #0\n"
      "polygon (-10,115;-10,2015;990,2015;990,115) #5\n"
      "polygon (-10,315;-10,2215;990,2215;990,315) #6\n"
      "polygon (-60,365;-60,2265;940,2265;940,365) #7\n"
      "polygon (0,105;0,2005;1000,2005;1000,105) #8\n"
      "polygon (0,20105;0,22005;1000,22005;1000,20105) #8\n"
      "polygon (11000,105;11000,2005;12000,2005;12000,105) #8\n"
      "polygon (11000,10105;11000,12005;12000,12005;12000,10105) #8\n"
      "polygon (11000,20105;11000,22005;12000,22005;12000,20105) #8\n"
      "polygon (22000,105;22000,2005;23000,2005;23000,105) #8\n"
      "polygon (22000,10105;22000,12005;23000,12005;23000,10105) #8\n"
      "polygon (22000,20105;22000,22005;23000,22005;23000,20105) #8\n"
      "polygon (33000,105;33000,2005;34000,2005;34000,105) #8\n"
      "polygon (33000,10105;33000,12005;34000,12005;34000,10105) #8\n"
      "polygon (33000,20105;33000,22005;34000,22005;34000,20105) #8\n"
    );

    //  test shape insert from shape reference
    copy.clear ();
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      copy.insert (*shape, pm);
    }
    EXPECT_EQ (shapes_to_string (_this, copy), shapes_to_string (_this, topcell.shapes (lindex)));

    //  test shape insert from shape reference with transformation
    copy.clear ();
    db::ICplxTrans t (2.0, 90.0, false, db::Vector (100, -50));
    db::ICplxTrans ti = t.inverted ();
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      db::Shape s = copy.insert (*shape, t, pm);
      copy.transform (s, ti);
    }
    EXPECT_EQ (shapes_to_string_norm (_this, copy), shapes_to_string_norm (_this, topcell.shapes (lindex)));

    //  test shape insert from shape reference with property modification
    copy.clear ();
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      plus1 pm;
      copy.insert (*shape, pm);
    }
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      if (shape->has_prop_id ()) {
        topcell.shapes (lindex).replace_prop_id (*shape, shape->prop_id () + 1);
      }
    }
    EXPECT_EQ (shapes_to_string (_this, copy), shapes_to_string (_this, topcell.shapes (lindex)));

  }
}

TEST(2A)
{
  db::Manager m (true);
  db::Layout layout (&m);
  unsigned int lindex = read_testdata (layout, 0x1);

  db::Cell &topcell = layout.cell (*layout.begin_top_down ());
  db::Shapes copy (&m, &topcell, db::default_editable_mode ());

  if (db::default_editable_mode ()) { 

    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    ++shape;
    topcell.shapes (lindex).erase_shape (*shape);
    //  duplicate erase should not hurt:
    topcell.shapes (lindex).erase_shape (*shape);
    topcell.shapes (lindex).erase_shape (*shape);
    topcell.shapes (lindex).erase_shape (*shape);
    for (unsigned int i = 0; i < 3; ++i) { ++shape; }
    topcell.shapes (lindex).erase_shape (*shape);
    for (unsigned int i = 0; i < 3; ++i) { ++shape; }
    topcell.shapes (lindex).erase_shape (*shape);
    for (unsigned int i = 0; i < 3; ++i) { ++shape; }
    topcell.shapes (lindex).erase_shape (*shape);
    for (unsigned int i = 0; i < 3; ++i) { ++shape; }
    topcell.shapes (lindex).erase_shape (*shape);
    for (unsigned int i = 0; i < 12; ++i) { ++shape; }
    topcell.shapes (lindex).erase_shape (*shape);

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)),
      "polygon (0,100;0,2000;1000,2000;1000,100) #0\n"
      "polygon (150,150;150,2050;1150,2050;1150,150) #0\n"
      "polygon (0,100;0,2000;1000,2000;1000,100) #1\n"
      "polygon (150,150;150,2050;1150,2050;1150,150) #3\n"
      "polygon (-10,115;-10,2015;990,2015;990,115) #0\n"
      "polygon (-60,365;-60,2265;940,2265;940,365) #0\n"
      "polygon (0,105;0,2005;1000,2005;1000,105) #0\n"
      "polygon (0,20105;0,22005;1000,22005;1000,20105) #0\n"
      "polygon (11000,105;11000,2005;12000,2005;12000,105) #0\n"
      "polygon (11000,20105;11000,22005;12000,22005;12000,20105) #0\n"
      "polygon (22000,105;22000,2005;23000,2005;23000,105) #0\n"
      "polygon (22000,10105;22000,12005;23000,12005;23000,10105) #0\n"
      "polygon (22000,20105;22000,22005;23000,22005;23000,20105) #0\n"
      "polygon (33000,105;33000,2005;34000,2005;34000,105) #0\n"
      "polygon (33000,10105;33000,12005;34000,12005;34000,10105) #0\n"
      "polygon (33000,20105;33000,22005;34000,22005;34000,20105) #0\n"
      "polygon (-10,115;-10,2015;990,2015;990,115) #5\n"
      "polygon (-10,315;-10,2215;990,2215;990,315) #6\n"
      "polygon (-60,365;-60,2265;940,2265;940,365) #7\n"
      "polygon (0,105;0,2005;1000,2005;1000,105) #8\n"
      "polygon (0,20105;0,22005;1000,22005;1000,20105) #8\n"
      "polygon (11000,105;11000,2005;12000,2005;12000,105) #8\n"
      "polygon (11000,10105;11000,12005;12000,12005;12000,10105) #8\n"
      "polygon (11000,20105;11000,22005;12000,22005;12000,20105) #8\n"
      "polygon (22000,105;22000,2005;23000,2005;23000,105) #8\n"
      "polygon (22000,10105;22000,12005;23000,12005;23000,10105) #8\n"
      "polygon (22000,20105;22000,22005;23000,22005;23000,20105) #8\n"
      "polygon (33000,105;33000,2005;34000,2005;34000,105) #8\n"
      "polygon (33000,10105;33000,12005;34000,12005;34000,10105) #8\n"
      "polygon (33000,20105;33000,22005;34000,22005;34000,20105) #8\n"
    );

    //  test shape insert from shape reference
    copy.clear ();
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      copy.insert (*shape);
    }
    EXPECT_EQ (shapes_to_string (_this, copy), shapes_to_string (_this, topcell.shapes (lindex)));

    //  test shape insert from shape reference with property modification
    copy.clear ();
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      plus1 pm;
      copy.insert (*shape, pm);
    }
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      if (shape->has_prop_id ()) {
        topcell.shapes (lindex).replace_prop_id (*shape, shape->prop_id () + 1);
      }
    }
    EXPECT_EQ (shapes_to_string (_this, copy), shapes_to_string (_this, topcell.shapes (lindex)));

    shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      topcell.shapes (lindex).erase_shape (*shape);
      ++shape;
    }

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)), "");

  }
}

TEST(3)
{
  db::Manager m (true);

  db::Layout other_layout (&m);
  db::Cell &other_topcell = other_layout.cell (other_layout.add_cell ());

  db::Layout layout (&m);
  unsigned int lindex = read_testdata (layout, 0x2);

  db::Cell &topcell = layout.cell (*layout.begin_top_down ());
  db::Shapes copy (&m, &topcell, db::default_editable_mode ());

  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)),
    "polygon (-10,115;-10,1015;1990,1015;1990,115) #0\n"
    "polygon (-10,115;-10,1015;1990,1015;1990,115) #5\n"
    "polygon (-10,315;-10,1215;1990,1215;1990,315) #0\n"
    "polygon (-10,315;-10,1215;1990,1215;1990,315) #6\n"
    "polygon (-60,365;-60,1265;1940,1265;1940,365) #0\n"
    "polygon (-60,365;-60,1265;1940,1265;1940,365) #7\n"
    "polygon (0,100;0,1000;2000,1000;2000,100) #0\n"
    "polygon (0,100;0,1000;2000,1000;2000,100) #1\n"
    "polygon (0,10105;0,11005;2000,11005;2000,10105) #0\n"
    "polygon (0,10105;0,11005;2000,11005;2000,10105) #8\n"
    "polygon (0,105;0,1005;2000,1005;2000,105) #0\n"
    "polygon (0,105;0,1005;2000,1005;2000,105) #8\n"
    "polygon (0,20105;0,21005;2000,21005;2000,20105) #0\n"
    "polygon (0,20105;0,21005;2000,21005;2000,20105) #8\n"
    "polygon (100,200;100,1100;2100,1100;2100,200) #0\n"
    "polygon (100,200;100,1100;2100,1100;2100,200) #2\n"
    "polygon (11000,10105;11000,11005;13000,11005;13000,10105) #0\n"
    "polygon (11000,10105;11000,11005;13000,11005;13000,10105) #8\n"
    "polygon (11000,105;11000,1005;13000,1005;13000,105) #0\n"
    "polygon (11000,105;11000,1005;13000,1005;13000,105) #8\n"
    "polygon (11000,20105;11000,21005;13000,21005;13000,20105) #0\n"
    "polygon (11000,20105;11000,21005;13000,21005;13000,20105) #8\n"
    "polygon (150,150;150,1050;2150,1050;2150,150) #0\n"
    "polygon (150,150;150,1050;2150,1050;2150,150) #3\n"
    "polygon (22000,10105;22000,11005;24000,11005;24000,10105) #0\n"
    "polygon (22000,10105;22000,11005;24000,11005;24000,10105) #8\n"
    "polygon (22000,105;22000,1005;24000,1005;24000,105) #0\n"
    "polygon (22000,105;22000,1005;24000,1005;24000,105) #8\n"
    "polygon (22000,20105;22000,21005;24000,21005;24000,20105) #0\n"
    "polygon (22000,20105;22000,21005;24000,21005;24000,20105) #8\n"
    "polygon (33000,10105;33000,11005;35000,11005;35000,10105) #0\n"
    "polygon (33000,10105;33000,11005;35000,11005;35000,10105) #8\n"
    "polygon (33000,105;33000,1005;35000,1005;35000,105) #0\n"
    "polygon (33000,105;33000,1005;35000,1005;35000,105) #8\n"
    "polygon (33000,20105;33000,21005;35000,21005;35000,20105) #0\n"
    "polygon (33000,20105;33000,21005;35000,21005;35000,20105) #8\n"
  );

  // tests simple copy
  copy = topcell.shapes(lindex);
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, copy));

  // tests translate
  db::Shapes other_copy (&m, &other_topcell, db::default_editable_mode ());
  other_copy = topcell.shapes(lindex);
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, other_copy));

  // tests translate plus transform
  other_copy.assign_transformed (topcell.shapes(lindex), db::ICplxTrans (2.0));
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, other_copy, db::ICplxTrans (0.5)));

  // tests deref
  db::Shapes sb_copy;
  sb_copy = topcell.shapes(lindex);
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, sb_copy));

  // tests deref plus transform
  sb_copy.assign_transformed (topcell.shapes(lindex), db::ICplxTrans (2.0));
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, sb_copy, db::ICplxTrans (0.5)));

  copy.clear ();
  for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
    copy.insert (*shape);
  }
  EXPECT_EQ (shapes_to_string (_this, copy),
    "polygon (0,100;0,1000;2000,1000;2000,100) #0\n"
    "polygon (100,200;100,1100;2100,1100;2100,200) #0\n"
    "polygon (150,150;150,1050;2150,1050;2150,150) #0\n"
    "polygon (0,100;0,1000;2000,1000;2000,100) #1\n"
    "polygon (100,200;100,1100;2100,1100;2100,200) #2\n"
    "polygon (150,150;150,1050;2150,1050;2150,150) #3\n"
    "polygon (-10,115;-10,1015;1990,1015;1990,115) #0\n"
    "polygon (-10,315;-10,1215;1990,1215;1990,315) #0\n"
    "polygon (-60,365;-60,1265;1940,1265;1940,365) #0\n"
    "polygon (0,105;0,1005;2000,1005;2000,105) #0\n"
    "polygon (0,10105;0,11005;2000,11005;2000,10105) #0\n"
    "polygon (0,20105;0,21005;2000,21005;2000,20105) #0\n"
    "polygon (11000,105;11000,1005;13000,1005;13000,105) #0\n"
    "polygon (11000,10105;11000,11005;13000,11005;13000,10105) #0\n"
    "polygon (11000,20105;11000,21005;13000,21005;13000,20105) #0\n"
    "polygon (22000,105;22000,1005;24000,1005;24000,105) #0\n"
    "polygon (22000,10105;22000,11005;24000,11005;24000,10105) #0\n"
    "polygon (22000,20105;22000,21005;24000,21005;24000,20105) #0\n"
    "polygon (33000,105;33000,1005;35000,1005;35000,105) #0\n"
    "polygon (33000,10105;33000,11005;35000,11005;35000,10105) #0\n"
    "polygon (33000,20105;33000,21005;35000,21005;35000,20105) #0\n"
    "polygon (-10,115;-10,1015;1990,1015;1990,115) #5\n"
    "polygon (-10,315;-10,1215;1990,1215;1990,315) #6\n"
    "polygon (-60,365;-60,1265;1940,1265;1940,365) #7\n"
    "polygon (0,105;0,1005;2000,1005;2000,105) #8\n"
    "polygon (0,10105;0,11005;2000,11005;2000,10105) #8\n"
    "polygon (0,20105;0,21005;2000,21005;2000,20105) #8\n"
    "polygon (11000,105;11000,1005;13000,1005;13000,105) #8\n"
    "polygon (11000,10105;11000,11005;13000,11005;13000,10105) #8\n"
    "polygon (11000,20105;11000,21005;13000,21005;13000,20105) #8\n"
    "polygon (22000,105;22000,1005;24000,1005;24000,105) #8\n"
    "polygon (22000,10105;22000,11005;24000,11005;24000,10105) #8\n"
    "polygon (22000,20105;22000,21005;24000,21005;24000,20105) #8\n"
    "polygon (33000,105;33000,1005;35000,1005;35000,105) #8\n"
    "polygon (33000,10105;33000,11005;35000,11005;35000,10105) #8\n"
    "polygon (33000,20105;33000,21005;35000,21005;35000,20105) #8\n"
  );

  db::Shapes sa_copy;
  sa_copy.clear ();
  for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
    sa_copy.insert (*shape);
  }
  EXPECT_EQ (shapes_to_string (_this, sa_copy),
    "polygon (0,100;0,1000;2000,1000;2000,100) #0\n"
    "polygon (100,200;100,1100;2100,1100;2100,200) #0\n"
    "polygon (150,150;150,1050;2150,1050;2150,150) #0\n"
    "polygon (-10,115;-10,1015;1990,1015;1990,115) #0\n"
    "polygon (-10,315;-10,1215;1990,1215;1990,315) #0\n"
    "polygon (-60,365;-60,1265;1940,1265;1940,365) #0\n"
    "polygon (0,105;0,1005;2000,1005;2000,105) #0\n"
    "polygon (0,10105;0,11005;2000,11005;2000,10105) #0\n"
    "polygon (0,20105;0,21005;2000,21005;2000,20105) #0\n"
    "polygon (11000,105;11000,1005;13000,1005;13000,105) #0\n"
    "polygon (11000,10105;11000,11005;13000,11005;13000,10105) #0\n"
    "polygon (11000,20105;11000,21005;13000,21005;13000,20105) #0\n"
    "polygon (22000,105;22000,1005;24000,1005;24000,105) #0\n"
    "polygon (22000,10105;22000,11005;24000,11005;24000,10105) #0\n"
    "polygon (22000,20105;22000,21005;24000,21005;24000,20105) #0\n"
    "polygon (33000,105;33000,1005;35000,1005;35000,105) #0\n"
    "polygon (33000,10105;33000,11005;35000,11005;35000,10105) #0\n"
    "polygon (33000,20105;33000,21005;35000,21005;35000,20105) #0\n"
    "polygon (0,100;0,1000;2000,1000;2000,100) #1\n"
    "polygon (100,200;100,1100;2100,1100;2100,200) #2\n"
    "polygon (150,150;150,1050;2150,1050;2150,150) #3\n"
    "polygon (-10,115;-10,1015;1990,1015;1990,115) #5\n"
    "polygon (-10,315;-10,1215;1990,1215;1990,315) #6\n"
    "polygon (-60,365;-60,1265;1940,1265;1940,365) #7\n"
    "polygon (0,105;0,1005;2000,1005;2000,105) #8\n"
    "polygon (0,10105;0,11005;2000,11005;2000,10105) #8\n"
    "polygon (0,20105;0,21005;2000,21005;2000,20105) #8\n"
    "polygon (11000,105;11000,1005;13000,1005;13000,105) #8\n"
    "polygon (11000,10105;11000,11005;13000,11005;13000,10105) #8\n"
    "polygon (11000,20105;11000,21005;13000,21005;13000,20105) #8\n"
    "polygon (22000,105;22000,1005;24000,1005;24000,105) #8\n"
    "polygon (22000,10105;22000,11005;24000,11005;24000,10105) #8\n"
    "polygon (22000,20105;22000,21005;24000,21005;24000,20105) #8\n"
    "polygon (33000,105;33000,1005;35000,1005;35000,105) #8\n"
    "polygon (33000,10105;33000,11005;35000,11005;35000,10105) #8\n"
    "polygon (33000,20105;33000,21005;35000,21005;35000,20105) #8\n"
  );

  db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
  ++shape;
  db::Shape s1 = *shape;
  for (unsigned int i = 0; i < 3; ++i) { ++shape; }
  db::Shape s2 = *shape;
  for (unsigned int i = 0; i < 3; ++i) { ++shape; }
  db::Shape s3 = *shape;
  for (unsigned int i = 0; i < 3; ++i) { ++shape; }
  db::Shape s4 = *shape;
  for (unsigned int i = 0; i < 3; ++i) { ++shape; }
  db::Shape s5 = *shape;
  for (unsigned int i = 0; i < 12; ++i) { ++shape; }
  db::Shape s6 = *shape;

  if (db::default_editable_mode ()) { 

    topcell.shapes (lindex).erase_shape (s1);
    topcell.shapes (lindex).erase_shape (s2);
    topcell.shapes (lindex).erase_shape (s3);
    topcell.shapes (lindex).erase_shape (s4);
    topcell.shapes (lindex).erase_shape (s5);
    topcell.shapes (lindex).erase_shape (s6);

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)),
      "polygon (0,100;0,1000;2000,1000;2000,100) #0\n"
      "polygon (150,150;150,1050;2150,1050;2150,150) #0\n"
      "polygon (0,100;0,1000;2000,1000;2000,100) #1\n"
      "polygon (150,150;150,1050;2150,1050;2150,150) #3\n"
      "polygon (-10,115;-10,1015;1990,1015;1990,115) #0\n"
      "polygon (-60,365;-60,1265;1940,1265;1940,365) #0\n"
      "polygon (0,105;0,1005;2000,1005;2000,105) #0\n"
      "polygon (0,20105;0,21005;2000,21005;2000,20105) #0\n"
      "polygon (11000,105;11000,1005;13000,1005;13000,105) #0\n"
      "polygon (11000,20105;11000,21005;13000,21005;13000,20105) #0\n"
      "polygon (22000,105;22000,1005;24000,1005;24000,105) #0\n"
      "polygon (22000,10105;22000,11005;24000,11005;24000,10105) #0\n"
      "polygon (22000,20105;22000,21005;24000,21005;24000,20105) #0\n"
      "polygon (33000,105;33000,1005;35000,1005;35000,105) #0\n"
      "polygon (33000,10105;33000,11005;35000,11005;35000,10105) #0\n"
      "polygon (33000,20105;33000,21005;35000,21005;35000,20105) #0\n"
      "polygon (-10,115;-10,1015;1990,1015;1990,115) #5\n"
      "polygon (-10,315;-10,1215;1990,1215;1990,315) #6\n"
      "polygon (-60,365;-60,1265;1940,1265;1940,365) #7\n"
      "polygon (0,105;0,1005;2000,1005;2000,105) #8\n"
      "polygon (0,20105;0,21005;2000,21005;2000,20105) #8\n"
      "polygon (11000,105;11000,1005;13000,1005;13000,105) #8\n"
      "polygon (11000,10105;11000,11005;13000,11005;13000,10105) #8\n"
      "polygon (11000,20105;11000,21005;13000,21005;13000,20105) #8\n"
      "polygon (22000,105;22000,1005;24000,1005;24000,105) #8\n"
      "polygon (22000,10105;22000,11005;24000,11005;24000,10105) #8\n"
      "polygon (22000,20105;22000,21005;24000,21005;24000,20105) #8\n"
      "polygon (33000,105;33000,1005;35000,1005;35000,105) #8\n"
      "polygon (33000,10105;33000,11005;35000,11005;35000,10105) #8\n"
      "polygon (33000,20105;33000,21005;35000,21005;35000,20105) #8\n"
    );

    //  test shape insert from shape reference
    copy.clear ();
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      copy.insert (*shape);
    }
    EXPECT_EQ (shapes_to_string (_this, copy), shapes_to_string (_this, topcell.shapes (lindex)));

    //  test shape insert from shape reference with transformation
    copy.clear ();
    db::ICplxTrans t (2.0, 90.0, false, db::Vector (100, -50));
    db::ICplxTrans ti = t.inverted ();
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      tl::ident_map<db::Layout::properties_id_type> pm;
      db::Shape s = copy.insert (*shape, t, pm);
      copy.transform (s, ti);
    }
    EXPECT_EQ (shapes_to_string_norm (_this, copy), shapes_to_string_norm (_this, topcell.shapes (lindex)));

    //  test shape insert from shape reference with property modification
    copy.clear ();
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      plus1 pm;
      copy.insert (*shape, pm);
    }
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      if (shape->has_prop_id ()) {
        topcell.shapes (lindex).replace_prop_id (*shape, shape->prop_id () + 1);
      }
    }
    EXPECT_EQ (shapes_to_string (_this, copy), shapes_to_string (_this, topcell.shapes (lindex)));

  }
}

TEST(4)
{
  db::Manager m (true);

  db::Layout other_layout (&m);
  db::Cell &other_topcell = other_layout.cell (other_layout.add_cell ());

  db::Layout layout (&m);
  unsigned int lindex = read_testdata (layout, 0x4);

  db::Cell &topcell = layout.cell (*layout.begin_top_down ());
  db::Shapes copy (&m, &topcell, db::default_editable_mode ());

  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)),
    "path (-10,115;-10,515;190,715) w=100 bx=0 ex=0 r=false #0\n"
    "path (-10,115;-10,515;190,715) w=100 bx=0 ex=0 r=false #5\n"
    "path (-110,1215;-110,1615;90,1415) w=150 bx=0 ex=0 r=false #0\n"
    "path (-110,1215;-110,1615;90,1415) w=150 bx=0 ex=0 r=false #6\n"
    "path (-210,2315;-210,2715;-410,2915) w=200 bx=0 ex=0 r=false #0\n"
    "path (-210,2315;-210,2715;-410,2915) w=200 bx=0 ex=0 r=false #7\n"
    "path (0,100;0,500;200,700) w=100 bx=0 ex=0 r=false #0\n"
    "path (0,100;0,500;200,700) w=100 bx=0 ex=0 r=false #1\n"
    "path (0,10105;0,10505;200,10705) w=100 bx=0 ex=0 r=false #0\n"
    "path (0,10105;0,10505;200,10705) w=100 bx=0 ex=0 r=false #8\n"
    "path (0,105;0,505;200,705) w=100 bx=0 ex=0 r=false #0\n"
    "path (0,105;0,505;200,705) w=100 bx=0 ex=0 r=false #8\n"
    "path (0,1100;0,1500;200,1300) w=150 bx=0 ex=0 r=false #0\n"
    "path (0,1100;0,1500;200,1300) w=150 bx=0 ex=0 r=false #2\n"
    "path (0,20105;0,20505;200,20705) w=100 bx=0 ex=0 r=false #0\n"
    "path (0,20105;0,20505;200,20705) w=100 bx=0 ex=0 r=false #8\n"
    "path (0,2100;0,2500;-200,2700) w=200 bx=0 ex=0 r=false #0\n"
    "path (0,2100;0,2500;-200,2700) w=200 bx=0 ex=0 r=false #3\n"
    "path (11000,10105;11000,10505;11200,10705) w=100 bx=0 ex=0 r=false #0\n"
    "path (11000,10105;11000,10505;11200,10705) w=100 bx=0 ex=0 r=false #8\n"
    "path (11000,105;11000,505;11200,705) w=100 bx=0 ex=0 r=false #0\n"
    "path (11000,105;11000,505;11200,705) w=100 bx=0 ex=0 r=false #8\n"
    "path (11000,20105;11000,20505;11200,20705) w=100 bx=0 ex=0 r=false #0\n"
    "path (11000,20105;11000,20505;11200,20705) w=100 bx=0 ex=0 r=false #8\n"
    "path (22000,10105;22000,10505;22200,10705) w=100 bx=0 ex=0 r=false #0\n"
    "path (22000,10105;22000,10505;22200,10705) w=100 bx=0 ex=0 r=false #8\n"
    "path (22000,105;22000,505;22200,705) w=100 bx=0 ex=0 r=false #0\n"
    "path (22000,105;22000,505;22200,705) w=100 bx=0 ex=0 r=false #8\n"
    "path (22000,20105;22000,20505;22200,20705) w=100 bx=0 ex=0 r=false #0\n"
    "path (22000,20105;22000,20505;22200,20705) w=100 bx=0 ex=0 r=false #8\n"
    "path (33000,10105;33000,10505;33200,10705) w=100 bx=0 ex=0 r=false #0\n"
    "path (33000,10105;33000,10505;33200,10705) w=100 bx=0 ex=0 r=false #8\n"
    "path (33000,105;33000,505;33200,705) w=100 bx=0 ex=0 r=false #0\n"
    "path (33000,105;33000,505;33200,705) w=100 bx=0 ex=0 r=false #8\n"
    "path (33000,20105;33000,20505;33200,20705) w=100 bx=0 ex=0 r=false #0\n"
    "path (33000,20105;33000,20505;33200,20705) w=100 bx=0 ex=0 r=false #8\n"
  );

  // tests simple copy
  copy = topcell.shapes(lindex);
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, copy));

  // tests translate
  db::Shapes other_copy (&m, &other_topcell, db::default_editable_mode ());
  other_copy = topcell.shapes(lindex);
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, other_copy));

  // tests translate plus transform
  other_copy.assign_transformed (topcell.shapes(lindex), db::ICplxTrans (2.0));
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, other_copy, db::ICplxTrans (0.5)));

  // tests deref
  db::Shapes sb_copy;
  sb_copy = topcell.shapes(lindex);
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, sb_copy));

  // tests deref plus transform
  sb_copy.assign_transformed (topcell.shapes(lindex), db::ICplxTrans (2.0));
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, sb_copy, db::ICplxTrans (0.5)));

  copy.clear ();
  for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
    copy.insert (*shape);
  }
  EXPECT_EQ (shapes_to_string (_this, copy),
    "path (0,100;0,500;200,700) w=100 bx=0 ex=0 r=false #0\n"
    "path (0,1100;0,1500;200,1300) w=150 bx=0 ex=0 r=false #0\n"
    "path (0,2100;0,2500;-200,2700) w=200 bx=0 ex=0 r=false #0\n"
    "path (0,100;0,500;200,700) w=100 bx=0 ex=0 r=false #1\n"
    "path (0,1100;0,1500;200,1300) w=150 bx=0 ex=0 r=false #2\n"
    "path (0,2100;0,2500;-200,2700) w=200 bx=0 ex=0 r=false #3\n"
    "path (-10,115;-10,515;190,715) w=100 bx=0 ex=0 r=false #0\n"
    "path (-110,1215;-110,1615;90,1415) w=150 bx=0 ex=0 r=false #0\n"
    "path (-210,2315;-210,2715;-410,2915) w=200 bx=0 ex=0 r=false #0\n"
    "path (0,105;0,505;200,705) w=100 bx=0 ex=0 r=false #0\n"
    "path (0,10105;0,10505;200,10705) w=100 bx=0 ex=0 r=false #0\n"
    "path (0,20105;0,20505;200,20705) w=100 bx=0 ex=0 r=false #0\n"
    "path (11000,105;11000,505;11200,705) w=100 bx=0 ex=0 r=false #0\n"
    "path (11000,10105;11000,10505;11200,10705) w=100 bx=0 ex=0 r=false #0\n"
    "path (11000,20105;11000,20505;11200,20705) w=100 bx=0 ex=0 r=false #0\n"
    "path (22000,105;22000,505;22200,705) w=100 bx=0 ex=0 r=false #0\n"
    "path (22000,10105;22000,10505;22200,10705) w=100 bx=0 ex=0 r=false #0\n"
    "path (22000,20105;22000,20505;22200,20705) w=100 bx=0 ex=0 r=false #0\n"
    "path (33000,105;33000,505;33200,705) w=100 bx=0 ex=0 r=false #0\n"
    "path (33000,10105;33000,10505;33200,10705) w=100 bx=0 ex=0 r=false #0\n"
    "path (33000,20105;33000,20505;33200,20705) w=100 bx=0 ex=0 r=false #0\n"
    "path (-10,115;-10,515;190,715) w=100 bx=0 ex=0 r=false #5\n"
    "path (-110,1215;-110,1615;90,1415) w=150 bx=0 ex=0 r=false #6\n"
    "path (-210,2315;-210,2715;-410,2915) w=200 bx=0 ex=0 r=false #7\n"
    "path (0,105;0,505;200,705) w=100 bx=0 ex=0 r=false #8\n"
    "path (0,10105;0,10505;200,10705) w=100 bx=0 ex=0 r=false #8\n"
    "path (0,20105;0,20505;200,20705) w=100 bx=0 ex=0 r=false #8\n"
    "path (11000,105;11000,505;11200,705) w=100 bx=0 ex=0 r=false #8\n"
    "path (11000,10105;11000,10505;11200,10705) w=100 bx=0 ex=0 r=false #8\n"
    "path (11000,20105;11000,20505;11200,20705) w=100 bx=0 ex=0 r=false #8\n"
    "path (22000,105;22000,505;22200,705) w=100 bx=0 ex=0 r=false #8\n"
    "path (22000,10105;22000,10505;22200,10705) w=100 bx=0 ex=0 r=false #8\n"
    "path (22000,20105;22000,20505;22200,20705) w=100 bx=0 ex=0 r=false #8\n"
    "path (33000,105;33000,505;33200,705) w=100 bx=0 ex=0 r=false #8\n"
    "path (33000,10105;33000,10505;33200,10705) w=100 bx=0 ex=0 r=false #8\n"
    "path (33000,20105;33000,20505;33200,20705) w=100 bx=0 ex=0 r=false #8\n"
  );

  db::Shapes sa_copy;
  for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
    sa_copy.insert (*shape);
  }
  EXPECT_EQ (shapes_to_string (_this, sa_copy),
    "path (0,100;0,500;200,700) w=100 bx=0 ex=0 r=false #0\n"
    "path (0,1100;0,1500;200,1300) w=150 bx=0 ex=0 r=false #0\n"
    "path (0,2100;0,2500;-200,2700) w=200 bx=0 ex=0 r=false #0\n"
    "path (-10,115;-10,515;190,715) w=100 bx=0 ex=0 r=false #0\n"
    "path (-110,1215;-110,1615;90,1415) w=150 bx=0 ex=0 r=false #0\n"
    "path (-210,2315;-210,2715;-410,2915) w=200 bx=0 ex=0 r=false #0\n"
    "path (0,105;0,505;200,705) w=100 bx=0 ex=0 r=false #0\n"
    "path (0,10105;0,10505;200,10705) w=100 bx=0 ex=0 r=false #0\n"
    "path (0,20105;0,20505;200,20705) w=100 bx=0 ex=0 r=false #0\n"
    "path (11000,105;11000,505;11200,705) w=100 bx=0 ex=0 r=false #0\n"
    "path (11000,10105;11000,10505;11200,10705) w=100 bx=0 ex=0 r=false #0\n"
    "path (11000,20105;11000,20505;11200,20705) w=100 bx=0 ex=0 r=false #0\n"
    "path (22000,105;22000,505;22200,705) w=100 bx=0 ex=0 r=false #0\n"
    "path (22000,10105;22000,10505;22200,10705) w=100 bx=0 ex=0 r=false #0\n"
    "path (22000,20105;22000,20505;22200,20705) w=100 bx=0 ex=0 r=false #0\n"
    "path (33000,105;33000,505;33200,705) w=100 bx=0 ex=0 r=false #0\n"
    "path (33000,10105;33000,10505;33200,10705) w=100 bx=0 ex=0 r=false #0\n"
    "path (33000,20105;33000,20505;33200,20705) w=100 bx=0 ex=0 r=false #0\n"
    "path (0,100;0,500;200,700) w=100 bx=0 ex=0 r=false #1\n"
    "path (0,1100;0,1500;200,1300) w=150 bx=0 ex=0 r=false #2\n"
    "path (0,2100;0,2500;-200,2700) w=200 bx=0 ex=0 r=false #3\n"
    "path (-10,115;-10,515;190,715) w=100 bx=0 ex=0 r=false #5\n"
    "path (-110,1215;-110,1615;90,1415) w=150 bx=0 ex=0 r=false #6\n"
    "path (-210,2315;-210,2715;-410,2915) w=200 bx=0 ex=0 r=false #7\n"
    "path (0,105;0,505;200,705) w=100 bx=0 ex=0 r=false #8\n"
    "path (0,10105;0,10505;200,10705) w=100 bx=0 ex=0 r=false #8\n"
    "path (0,20105;0,20505;200,20705) w=100 bx=0 ex=0 r=false #8\n"
    "path (11000,105;11000,505;11200,705) w=100 bx=0 ex=0 r=false #8\n"
    "path (11000,10105;11000,10505;11200,10705) w=100 bx=0 ex=0 r=false #8\n"
    "path (11000,20105;11000,20505;11200,20705) w=100 bx=0 ex=0 r=false #8\n"
    "path (22000,105;22000,505;22200,705) w=100 bx=0 ex=0 r=false #8\n"
    "path (22000,10105;22000,10505;22200,10705) w=100 bx=0 ex=0 r=false #8\n"
    "path (22000,20105;22000,20505;22200,20705) w=100 bx=0 ex=0 r=false #8\n"
    "path (33000,105;33000,505;33200,705) w=100 bx=0 ex=0 r=false #8\n"
    "path (33000,10105;33000,10505;33200,10705) w=100 bx=0 ex=0 r=false #8\n"
    "path (33000,20105;33000,20505;33200,20705) w=100 bx=0 ex=0 r=false #8\n"
  );

  db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
  ++shape;
  db::Shape s1 = *shape;
  for (unsigned int i = 0; i < 3; ++i) { ++shape; }
  db::Shape s2 = *shape;
  for (unsigned int i = 0; i < 3; ++i) { ++shape; }
  db::Shape s3 = *shape;
  for (unsigned int i = 0; i < 3; ++i) { ++shape; }
  db::Shape s4 = *shape;
  for (unsigned int i = 0; i < 3; ++i) { ++shape; }
  db::Shape s5 = *shape;
  for (unsigned int i = 0; i < 12; ++i) { ++shape; }
  db::Shape s6 = *shape;

  if (db::default_editable_mode ()) { 

    topcell.shapes (lindex).erase_shape (s1);
    topcell.shapes (lindex).erase_shape (s2);
    topcell.shapes (lindex).erase_shape (s3);
    topcell.shapes (lindex).erase_shape (s4);
    topcell.shapes (lindex).erase_shape (s5);
    topcell.shapes (lindex).erase_shape (s6);

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)),
      "path (0,100;0,500;200,700) w=100 bx=0 ex=0 r=false #0\n"
      "path (0,2100;0,2500;-200,2700) w=200 bx=0 ex=0 r=false #0\n"
      "path (0,100;0,500;200,700) w=100 bx=0 ex=0 r=false #1\n"
      "path (0,2100;0,2500;-200,2700) w=200 bx=0 ex=0 r=false #3\n"
      "path (-10,115;-10,515;190,715) w=100 bx=0 ex=0 r=false #0\n"
      "path (-210,2315;-210,2715;-410,2915) w=200 bx=0 ex=0 r=false #0\n"
      "path (0,105;0,505;200,705) w=100 bx=0 ex=0 r=false #0\n"
      "path (0,20105;0,20505;200,20705) w=100 bx=0 ex=0 r=false #0\n"
      "path (11000,105;11000,505;11200,705) w=100 bx=0 ex=0 r=false #0\n"
      "path (11000,20105;11000,20505;11200,20705) w=100 bx=0 ex=0 r=false #0\n"
      "path (22000,105;22000,505;22200,705) w=100 bx=0 ex=0 r=false #0\n"
      "path (22000,10105;22000,10505;22200,10705) w=100 bx=0 ex=0 r=false #0\n"
      "path (22000,20105;22000,20505;22200,20705) w=100 bx=0 ex=0 r=false #0\n"
      "path (33000,105;33000,505;33200,705) w=100 bx=0 ex=0 r=false #0\n"
      "path (33000,10105;33000,10505;33200,10705) w=100 bx=0 ex=0 r=false #0\n"
      "path (33000,20105;33000,20505;33200,20705) w=100 bx=0 ex=0 r=false #0\n"
      "path (-10,115;-10,515;190,715) w=100 bx=0 ex=0 r=false #5\n"
      "path (-110,1215;-110,1615;90,1415) w=150 bx=0 ex=0 r=false #6\n"
      "path (-210,2315;-210,2715;-410,2915) w=200 bx=0 ex=0 r=false #7\n"
      "path (0,105;0,505;200,705) w=100 bx=0 ex=0 r=false #8\n"
      "path (0,20105;0,20505;200,20705) w=100 bx=0 ex=0 r=false #8\n"
      "path (11000,105;11000,505;11200,705) w=100 bx=0 ex=0 r=false #8\n"
      "path (11000,10105;11000,10505;11200,10705) w=100 bx=0 ex=0 r=false #8\n"
      "path (11000,20105;11000,20505;11200,20705) w=100 bx=0 ex=0 r=false #8\n"
      "path (22000,105;22000,505;22200,705) w=100 bx=0 ex=0 r=false #8\n"
      "path (22000,10105;22000,10505;22200,10705) w=100 bx=0 ex=0 r=false #8\n"
      "path (22000,20105;22000,20505;22200,20705) w=100 bx=0 ex=0 r=false #8\n"
      "path (33000,105;33000,505;33200,705) w=100 bx=0 ex=0 r=false #8\n"
      "path (33000,10105;33000,10505;33200,10705) w=100 bx=0 ex=0 r=false #8\n"
      "path (33000,20105;33000,20505;33200,20705) w=100 bx=0 ex=0 r=false #8\n"
    );

    //  test shape insert from shape reference
    copy.clear ();
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      copy.insert (*shape);
    }
    EXPECT_EQ (shapes_to_string (_this, copy), shapes_to_string (_this, topcell.shapes (lindex)));

    //  test shape insert from shape reference with transformation
    copy.clear ();
    db::ICplxTrans t (2.0, 90.0, false, db::Vector (100, -50));
    db::ICplxTrans ti = t.inverted ();
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      tl::ident_map<db::Layout::properties_id_type> pm;
      db::Shape s = copy.insert (*shape, t, pm);
      copy.transform (s, ti);
    }
    EXPECT_EQ (shapes_to_string_norm (_this, copy), shapes_to_string_norm (_this, topcell.shapes (lindex)));

    //  test shape insert from shape reference with property modification
    copy.clear ();
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      plus1 pm;
      copy.insert (*shape, pm);
    }
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      if (shape->has_prop_id ()) {
        topcell.shapes (lindex).replace_prop_id (*shape, shape->prop_id () + 1);
      }
    }
    EXPECT_EQ (shapes_to_string (_this, copy), shapes_to_string (_this, topcell.shapes (lindex)));

  }
}

TEST(5)
{
  db::Manager m (true);

  db::Layout other_layout (&m);
  db::Cell &other_topcell = other_layout.cell (other_layout.add_cell ());

  db::Layout layout (&m);
  unsigned int lindex = read_testdata (layout, 0x8);

  db::Cell &topcell = layout.cell (*layout.begin_top_down ());
  db::Shapes copy (&m, &topcell, db::default_editable_mode ());

  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)),
    "text ('A',r0 0,50) #0\n"
    "text ('A',r0 0,50) #5\n"
    "text ('A',r0 10,10040) #0\n"
    "text ('A',r0 10,10040) #8\n"
    "text ('A',r0 10,20040) #0\n"
    "text ('A',r0 10,20040) #8\n"
    "text ('A',r0 10,35) #0\n"
    "text ('A',r0 10,35) #1\n"
    "text ('A',r0 10,40) #0\n"
    "text ('A',r0 10,40) #8\n"
    "text ('A',r0 11010,10040) #0\n"
    "text ('A',r0 11010,10040) #8\n"
    "text ('A',r0 11010,20040) #0\n"
    "text ('A',r0 11010,20040) #8\n"
    "text ('A',r0 11010,40) #0\n"
    "text ('A',r0 11010,40) #8\n"
    "text ('A',r0 22010,10040) #0\n"
    "text ('A',r0 22010,10040) #8\n"
    "text ('A',r0 22010,20040) #0\n"
    "text ('A',r0 22010,20040) #8\n"
    "text ('A',r0 22010,40) #0\n"
    "text ('A',r0 22010,40) #8\n"
    "text ('A',r0 33010,10040) #0\n"
    "text ('A',r0 33010,10040) #8\n"
    "text ('A',r0 33010,20040) #0\n"
    "text ('A',r0 33010,20040) #8\n"
    "text ('A',r0 33010,40) #0\n"
    "text ('A',r0 33010,40) #8\n"
    "text ('B',r90 -90,140) #0\n"
    "text ('B',r90 -90,140) #6\n"
    "text ('B',r90 20,25) #0\n"
    "text ('B',r90 20,25) #2\n"
    "text ('C',m90 -180,230) #0\n"
    "text ('C',m90 -180,230) #7\n"
    "text ('C',m90 30,15) #0\n"
    "text ('C',m90 30,15) #3\n"
  );

  // tests simple copy
  copy = topcell.shapes(lindex);
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, copy));

  // tests translate
  db::Shapes other_copy (&m, &other_topcell, db::default_editable_mode ());
  other_copy = topcell.shapes(lindex);
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, other_copy));

  // tests translate plus transform
  other_copy.assign_transformed (topcell.shapes(lindex), db::ICplxTrans (2.0));
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, other_copy, db::ICplxTrans (0.5)));

  // tests deref
  db::Shapes sb_copy;
  sb_copy = topcell.shapes(lindex);
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, sb_copy));

  // tests deref plus transform
  sb_copy.assign_transformed (topcell.shapes(lindex), db::ICplxTrans (2.0));
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, sb_copy, db::ICplxTrans (0.5)));

  copy.clear ();
  for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
    copy.insert (*shape);
  }
  EXPECT_EQ (shapes_to_string_norm (_this, copy), shapes_to_string_norm (_this, topcell.shapes (lindex)));

  db::Shapes sa_copy;
  for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
    sa_copy.insert (*shape);
  }
  EXPECT_EQ (shapes_to_string_norm (_this, sa_copy), shapes_to_string_norm (_this, topcell.shapes (lindex)));

  db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
  ++shape;
  db::Shape s1 = *shape;
  for (unsigned int i = 0; i < 3; ++i) { ++shape; }
  db::Shape s2 = *shape;
  for (unsigned int i = 0; i < 3; ++i) { ++shape; }
  db::Shape s3 = *shape;
  for (unsigned int i = 0; i < 3; ++i) { ++shape; }
  db::Shape s4 = *shape;
  for (unsigned int i = 0; i < 3; ++i) { ++shape; }
  db::Shape s5 = *shape;
  for (unsigned int i = 0; i < 12; ++i) { ++shape; }
  db::Shape s6 = *shape;

  if (db::default_editable_mode ()) {

    topcell.shapes (lindex).erase_shape (s1);
    topcell.shapes (lindex).erase_shape (s2);
    topcell.shapes (lindex).erase_shape (s3);
    topcell.shapes (lindex).erase_shape (s4);
    topcell.shapes (lindex).erase_shape (s5);
    topcell.shapes (lindex).erase_shape (s6);

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)),
      "text ('A',r0 10,35) #0\n"
      "text ('C',m90 30,15) #0\n"
      "text ('A',r0 10,35) #1\n"
      "text ('C',m90 30,15) #3\n"
      "text ('A',r0 0,50) #0\n"
      "text ('C',m90 -180,230) #0\n"
      "text ('A',r0 10,40) #0\n"
      "text ('A',r0 10,20040) #0\n"
      "text ('A',r0 11010,40) #0\n"
      "text ('A',r0 11010,20040) #0\n"
      "text ('A',r0 22010,40) #0\n"
      "text ('A',r0 22010,10040) #0\n"
      "text ('A',r0 22010,20040) #0\n"
      "text ('A',r0 33010,40) #0\n"
      "text ('A',r0 33010,10040) #0\n"
      "text ('A',r0 33010,20040) #0\n"
      "text ('A',r0 0,50) #5\n"
      "text ('B',r90 -90,140) #6\n"
      "text ('C',m90 -180,230) #7\n"
      "text ('A',r0 10,40) #8\n"
      "text ('A',r0 10,20040) #8\n"
      "text ('A',r0 11010,40) #8\n"
      "text ('A',r0 11010,10040) #8\n"
      "text ('A',r0 11010,20040) #8\n"
      "text ('A',r0 22010,40) #8\n"
      "text ('A',r0 22010,10040) #8\n"
      "text ('A',r0 22010,20040) #8\n"
      "text ('A',r0 33010,40) #8\n"
      "text ('A',r0 33010,10040) #8\n"
      "text ('A',r0 33010,20040) #8\n"
    );

    //  test shape insert from shape reference
    copy.clear ();
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      copy.insert (*shape);
    }
    //  note: we need "norm" since shapes are subject to normalization because of potential StringRef's in the source
    EXPECT_EQ (shapes_to_string_norm (_this, copy), shapes_to_string_norm (_this, topcell.shapes (lindex)));

    //  test shape insert from shape reference with transformation
    copy.clear ();
    db::ICplxTrans t (2.0, 90.0, false, db::Vector (100, -50));
    db::ICplxTrans ti = t.inverted ();
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      tl::ident_map<db::Layout::properties_id_type> pm;
      db::Shape s = copy.insert (*shape, t, pm);
      copy.transform (s, ti);
    }
    EXPECT_EQ (shapes_to_string_norm (_this, copy), shapes_to_string_norm (_this, topcell.shapes (lindex)));

    //  test shape insert from shape reference with property modification
    copy.clear ();
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      plus1 pm;
      copy.insert (*shape, pm);
    }
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      if (shape->has_prop_id ()) {
        topcell.shapes (lindex).replace_prop_id (*shape, shape->prop_id () + 1);
      }
    }
    EXPECT_EQ (shapes_to_string_norm (_this, copy), shapes_to_string_norm (_this, topcell.shapes (lindex)));

  }
}

TEST(6)
{
  db::Manager m (true);

  db::Layout other_layout (&m);
  db::Cell &other_topcell = other_layout.cell (other_layout.add_cell ());

  db::Layout layout (&m);
  unsigned int lindex = read_testdata (layout, 0x10);

  db::Cell &topcell = layout.cell (*layout.begin_top_down ());
  db::Shapes copy (&m, &topcell, db::default_editable_mode ());

  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)),
    "box (0,100;2000,1000) #0\n"
    "box (0,100;2000,1000) #10\n"
    "box (100,200;2100,1100) #0\n"
    "box (100,200;2100,1100) #11\n"
    "box (11050,-1050;12050,-50) #0\n"
    "box (11050,-1050;12050,-50) #20\n"
    "box (11050,18950;12050,19950) #0\n"
    "box (11050,18950;12050,19950) #20\n"
    "box (11050,8950;12050,9950) #0\n"
    "box (11050,8950;12050,9950) #20\n"
    "box (150,150;2150,1050) #0\n"
    "box (150,150;2150,1050) #12\n"
    "box (22050,-1050;23050,-50) #0\n"
    "box (22050,-1050;23050,-50) #20\n"
    "box (22050,18950;23050,19950) #0\n"
    "box (22050,18950;23050,19950) #20\n"
    "box (22050,8950;23050,9950) #0\n"
    "box (22050,8950;23050,9950) #20\n"
    "box (33050,-1050;34050,-50) #0\n"
    "box (33050,-1050;34050,-50) #20\n"
    "box (33050,18950;34050,19950) #0\n"
    "box (33050,18950;34050,19950) #20\n"
    "box (33050,8950;34050,9950) #0\n"
    "box (33050,8950;34050,9950) #20\n"
    "box (50,-1050;1050,-50) #0\n"
    "box (50,-1050;1050,-50) #20\n"
    "box (50,18950;1050,19950) #0\n"
    "box (50,18950;1050,19950) #20\n"
    "box (50,8950;1050,9950) #0\n"
    "box (50,8950;1050,9950) #20\n"
  );

  // tests simple copy
  copy = topcell.shapes(lindex);
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, copy));

  // tests translate
  db::Shapes other_copy (&m, &other_topcell, db::default_editable_mode ());
  other_copy = topcell.shapes(lindex);
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, other_copy));

  // tests translate plus transform
  other_copy.assign_transformed (topcell.shapes(lindex), db::ICplxTrans (2.0));
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, other_copy, db::ICplxTrans (0.5)));

  // tests deref
  db::Shapes sb_copy;
  sb_copy = topcell.shapes(lindex);
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, sb_copy));

  // tests deref plus transform
  sb_copy.assign_transformed (topcell.shapes(lindex), db::ICplxTrans (2.0));
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, sb_copy, db::ICplxTrans (0.5)));

  copy.clear ();
  for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
    copy.insert (*shape);
  }
  EXPECT_EQ (shapes_to_string (_this, copy),
    "box (0,100;2000,1000) #0\n"
    "box (100,200;2100,1100) #0\n"
    "box (150,150;2150,1050) #0\n"
    "box (50,-1050;1050,-50) #0\n"
    "box (50,8950;1050,9950) #0\n"
    "box (50,18950;1050,19950) #0\n"
    "box (11050,-1050;12050,-50) #0\n"
    "box (11050,8950;12050,9950) #0\n"
    "box (11050,18950;12050,19950) #0\n"
    "box (22050,-1050;23050,-50) #0\n"
    "box (22050,8950;23050,9950) #0\n"
    "box (22050,18950;23050,19950) #0\n"
    "box (33050,-1050;34050,-50) #0\n"
    "box (33050,8950;34050,9950) #0\n"
    "box (33050,18950;34050,19950) #0\n"
    "box (0,100;2000,1000) #10\n"
    "box (100,200;2100,1100) #11\n"
    "box (150,150;2150,1050) #12\n"
    "box (50,-1050;1050,-50) #20\n"
    "box (50,8950;1050,9950) #20\n"
    "box (50,18950;1050,19950) #20\n"
    "box (11050,-1050;12050,-50) #20\n"
    "box (11050,8950;12050,9950) #20\n"
    "box (11050,18950;12050,19950) #20\n"
    "box (22050,-1050;23050,-50) #20\n"
    "box (22050,8950;23050,9950) #20\n"
    "box (22050,18950;23050,19950) #20\n"
    "box (33050,-1050;34050,-50) #20\n"
    "box (33050,8950;34050,9950) #20\n"
    "box (33050,18950;34050,19950) #20\n"
  );

  db::Shapes sa_copy;
  for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
    sa_copy.insert (*shape);
  }
  EXPECT_EQ (shapes_to_string (_this, sa_copy),
    "box (0,100;2000,1000) #0\n"
    "box (100,200;2100,1100) #0\n"
    "box (150,150;2150,1050) #0\n"
    "box (50,-1050;1050,-50) #0\n"
    "box (50,8950;1050,9950) #0\n"
    "box (50,18950;1050,19950) #0\n"
    "box (11050,-1050;12050,-50) #0\n"
    "box (11050,8950;12050,9950) #0\n"
    "box (11050,18950;12050,19950) #0\n"
    "box (22050,-1050;23050,-50) #0\n"
    "box (22050,8950;23050,9950) #0\n"
    "box (22050,18950;23050,19950) #0\n"
    "box (33050,-1050;34050,-50) #0\n"
    "box (33050,8950;34050,9950) #0\n"
    "box (33050,18950;34050,19950) #0\n"
    "box (0,100;2000,1000) #10\n"
    "box (100,200;2100,1100) #11\n"
    "box (150,150;2150,1050) #12\n"
    "box (50,-1050;1050,-50) #20\n"
    "box (50,8950;1050,9950) #20\n"
    "box (50,18950;1050,19950) #20\n"
    "box (11050,-1050;12050,-50) #20\n"
    "box (11050,8950;12050,9950) #20\n"
    "box (11050,18950;12050,19950) #20\n"
    "box (22050,-1050;23050,-50) #20\n"
    "box (22050,8950;23050,9950) #20\n"
    "box (22050,18950;23050,19950) #20\n"
    "box (33050,-1050;34050,-50) #20\n"
    "box (33050,8950;34050,9950) #20\n"
    "box (33050,18950;34050,19950) #20\n"
  );

  db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
  ++shape;
  db::Shape s1 = *shape;
  for (unsigned int i = 0; i < 3; ++i) { ++shape; }
  db::Shape s2 = *shape;
  for (unsigned int i = 0; i < 3; ++i) { ++shape; }
  db::Shape s3 = *shape;
  for (unsigned int i = 0; i < 12; ++i) { ++shape; }
  db::Shape s4 = *shape;

  if (db::default_editable_mode ()) {

    topcell.shapes (lindex).erase_shape (s1);
    topcell.shapes (lindex).erase_shape (s2);
    topcell.shapes (lindex).erase_shape (s3);
    topcell.shapes (lindex).erase_shape (s4);

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)),
      "box (0,100;2000,1000) #0\n"
      "box (150,150;2150,1050) #0\n"
      "box (50,-1050;1050,-50) #0\n"
      "box (50,18950;1050,19950) #0\n"
      "box (11050,-1050;12050,-50) #0\n"
      "box (11050,18950;12050,19950) #0\n"
      "box (22050,-1050;23050,-50) #0\n"
      "box (22050,8950;23050,9950) #0\n"
      "box (22050,18950;23050,19950) #0\n"
      "box (33050,-1050;34050,-50) #0\n"
      "box (33050,8950;34050,9950) #0\n"
      "box (33050,18950;34050,19950) #0\n"
      "box (0,100;2000,1000) #10\n"
      "box (100,200;2100,1100) #11\n"
      "box (150,150;2150,1050) #12\n"
      "box (50,-1050;1050,-50) #20\n"
      "box (50,18950;1050,19950) #20\n"
      "box (11050,-1050;12050,-50) #20\n"
      "box (11050,8950;12050,9950) #20\n"
      "box (11050,18950;12050,19950) #20\n"
      "box (22050,-1050;23050,-50) #20\n"
      "box (22050,8950;23050,9950) #20\n"
      "box (22050,18950;23050,19950) #20\n"
      "box (33050,-1050;34050,-50) #20\n"
      "box (33050,8950;34050,9950) #20\n"
      "box (33050,18950;34050,19950) #20\n"
    );

    //  test shape insert from shape reference
    copy.clear ();
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      copy.insert (*shape);
    }
    EXPECT_EQ (shapes_to_string (_this, copy), shapes_to_string (_this, topcell.shapes (lindex)));

    //  test shape insert from shape reference with transformation
    copy.clear ();
    db::ICplxTrans t (2.0, 90.0, false, db::Vector (100, -50));
    db::ICplxTrans ti = t.inverted ();
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      tl::ident_map<db::Layout::properties_id_type> pm;
      db::Shape s = copy.insert (*shape, t, pm);
      copy.transform (s, ti);
    }
    EXPECT_EQ (shapes_to_string (_this, copy), shapes_to_string (_this, topcell.shapes (lindex)));

    //  test shape insert from shape reference with property modification
    copy.clear ();
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      plus1 pm;
      copy.insert (*shape, pm);
    }
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      if (shape->has_prop_id ()) {
        topcell.shapes (lindex).replace_prop_id (*shape, shape->prop_id () + 1);
      }
    }
    EXPECT_EQ (shapes_to_string (_this, copy), shapes_to_string (_this, topcell.shapes (lindex)));

  }
}

TEST(7)
{
  db::Manager m (true);

  db::Layout other_layout (&m);
  db::Cell &other_topcell = other_layout.cell (other_layout.add_cell ());

  db::Layout layout (&m);
  unsigned int lindex = read_testdata (layout, 0x20);

  db::Cell &topcell = layout.cell (*layout.begin_top_down ());
  db::Shapes copy (&m, &topcell, db::default_editable_mode ());

  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)),
    "box (0,100;2000,1000) #0\n"
    "box (0,100;2000,1000) #10\n"
    "box (100,200;2100,1100) #0\n"
    "box (100,200;2100,1100) #11\n"
    "box (11050,-1050;12050,-50) #0\n"
    "box (11050,-1050;12050,-50) #20\n"
    "box (11050,18950;12050,19950) #0\n"
    "box (11050,18950;12050,19950) #20\n"
    "box (11050,8950;12050,9950) #0\n"
    "box (11050,8950;12050,9950) #20\n"
    "box (150,150;2150,1050) #0\n"
    "box (150,150;2150,1050) #12\n"
    "box (22050,-1050;23050,-50) #0\n"
    "box (22050,-1050;23050,-50) #20\n"
    "box (22050,18950;23050,19950) #0\n"
    "box (22050,18950;23050,19950) #20\n"
    "box (22050,8950;23050,9950) #0\n"
    "box (22050,8950;23050,9950) #20\n"
    "box (33050,-1050;34050,-50) #0\n"
    "box (33050,-1050;34050,-50) #20\n"
    "box (33050,18950;34050,19950) #0\n"
    "box (33050,18950;34050,19950) #20\n"
    "box (33050,8950;34050,9950) #0\n"
    "box (33050,8950;34050,9950) #20\n"
    "box (50,-1050;1050,-50) #0\n"
    "box (50,-1050;1050,-50) #20\n"
    "box (50,18950;1050,19950) #0\n"
    "box (50,18950;1050,19950) #20\n"
    "box (50,8950;1050,9950) #0\n"
    "box (50,8950;1050,9950) #20\n"
  );

  // tests simple copy
  copy = topcell.shapes(lindex);
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, copy));

  // tests translate
  db::Shapes other_copy (&m, &other_topcell, db::default_editable_mode ());
  other_copy = topcell.shapes(lindex);
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, other_copy));

  // tests translate plus transform
  other_copy.assign_transformed (topcell.shapes(lindex), db::ICplxTrans (2.0));
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, other_copy, db::ICplxTrans (0.5)));

  // tests deref
  db::Shapes sb_copy;
  sb_copy = topcell.shapes(lindex);
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, sb_copy));

  // tests deref plus transform
  sb_copy.assign_transformed (topcell.shapes(lindex), db::ICplxTrans (2.0));
  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, sb_copy, db::ICplxTrans (0.5)));

  copy.clear ();
  for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
    copy.insert (*shape);
  }

  if (db::default_editable_mode ()) {

    //  in editable mode, no arrays are stored, thus no expansion problems occure for short boxes
    EXPECT_EQ (shapes_to_string (_this, copy),
      "box (50,-1050;1050,-50) #0\n"
      "box (50,8950;1050,9950) #0\n"
      "box (50,18950;1050,19950) #0\n"
      "box (11050,-1050;12050,-50) #0\n"
      "box (11050,8950;12050,9950) #0\n"
      "box (11050,18950;12050,19950) #0\n"
      "box (22050,-1050;23050,-50) #0\n"
      "box (22050,8950;23050,9950) #0\n"
      "box (22050,18950;23050,19950) #0\n"
      "box (33050,-1050;34050,-50) #0\n"
      "box (33050,8950;34050,9950) #0\n"
      "box (33050,18950;34050,19950) #0\n"
      "box (50,-1050;1050,-50) #20\n"
      "box (50,8950;1050,9950) #20\n"
      "box (50,18950;1050,19950) #20\n"
      "box (11050,-1050;12050,-50) #20\n"
      "box (11050,8950;12050,9950) #20\n"
      "box (11050,18950;12050,19950) #20\n"
      "box (22050,-1050;23050,-50) #20\n"
      "box (22050,8950;23050,9950) #20\n"
      "box (22050,18950;23050,19950) #20\n"
      "box (33050,-1050;34050,-50) #20\n"
      "box (33050,8950;34050,9950) #20\n"
      "box (33050,18950;34050,19950) #20\n"
      "box (0,100;2000,1000) #0\n"
      "box (100,200;2100,1100) #0\n"
      "box (150,150;2150,1050) #0\n"
      "box (0,100;2000,1000) #10\n"
      "box (100,200;2100,1100) #11\n"
      "box (150,150;2150,1050) #12\n"
    );

  } else if (sizeof (db::ShortBox) > 8) {

    EXPECT_EQ (shapes_to_string (_this, copy),
      "box (0,100;2000,1000) #0\n"
      "box (100,200;2100,1100) #0\n"
      "box (150,150;2150,1050) #0\n"
      "box (50,-1050;1050,-50) #0\n"
      "box (50,8950;1050,9950) #0\n"
      "box (50,18950;1050,19950) #0\n"
      "box (11050,-1050;12050,-50) #0\n"
      "box (11050,8950;12050,9950) #0\n"
      "box (11050,18950;12050,19950) #0\n"
      "box (22050,-1050;23050,-50) #0\n"
      "box (22050,8950;23050,9950) #0\n"
      "box (22050,18950;23050,19950) #0\n"
      "box (33050,-1050;34050,-50) #0\n"
      "box (33050,8950;34050,9950) #0\n"
      "box (33050,18950;34050,19950) #0\n"
      "box (0,100;2000,1000) #10\n"
      "box (100,200;2100,1100) #11\n"
      "box (150,150;2150,1050) #12\n"
      "box (50,-1050;1050,-50) #20\n"
      "box (50,8950;1050,9950) #20\n"
      "box (50,18950;1050,19950) #20\n"
      "box (11050,-1050;12050,-50) #20\n"
      "box (11050,8950;12050,9950) #20\n"
      "box (11050,18950;12050,19950) #20\n"
      "box (22050,-1050;23050,-50) #20\n"
      "box (22050,8950;23050,9950) #20\n"
      "box (22050,18950;23050,19950) #20\n"
      "box (33050,-1050;34050,-50) #20\n"
      "box (33050,8950;34050,9950) #20\n"
      "box (33050,18950;34050,19950) #20\n"
    );

  } else {

    //  16 bit coordinate overflow happens during ShortBox array expansion
    EXPECT_EQ (shapes_to_string (_this, copy),
      "box (0,100;2000,1000) #0\n"
      "box (100,200;2100,1100) #0\n"
      "box (150,150;2150,1050) #0\n"
      "box (50,-1050;1050,-50) #0\n"
      "box (50,8950;1050,9950) #0\n"
      "box (50,18950;1050,19950) #0\n"
      "box (11050,-1050;12050,-50) #0\n"
      "box (11050,8950;12050,9950) #0\n"
      "box (11050,18950;12050,19950) #0\n"
      "box (22050,-1050;23050,-50) #0\n"
      "box (22050,8950;23050,9950) #0\n"
      "box (22050,18950;23050,19950) #0\n"
      "box (-32486,-1050;-31486,-50) #0\n"
      "box (-32486,8950;-31486,9950) #0\n"
      "box (-32486,18950;-31486,19950) #0\n"
      "box (0,100;2000,1000) #10\n"
      "box (100,200;2100,1100) #11\n"
      "box (150,150;2150,1050) #12\n"
      "box (50,-1050;1050,-50) #20\n"
      "box (50,8950;1050,9950) #20\n"
      "box (50,18950;1050,19950) #20\n"
      "box (11050,-1050;12050,-50) #20\n"
      "box (11050,8950;12050,9950) #20\n"
      "box (11050,18950;12050,19950) #20\n"
      "box (22050,-1050;23050,-50) #20\n"
      "box (22050,8950;23050,9950) #20\n"
      "box (22050,18950;23050,19950) #20\n"
      "box (-32486,-1050;-31486,-50) #20\n"
      "box (-32486,8950;-31486,9950) #20\n"
      "box (-32486,18950;-31486,19950) #20\n"
    );

  }

  db::Shapes sa_copy;
  for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
    sa_copy.insert (*shape);
  }

  if (db::default_editable_mode ()) {

    //  in editable mode, no arrays are stored, thus no expansion problems occure for short boxes
    EXPECT_EQ (shapes_to_string (_this, sa_copy),
      "box (50,-1050;1050,-50) #0\n"
      "box (50,8950;1050,9950) #0\n"
      "box (50,18950;1050,19950) #0\n"
      "box (11050,-1050;12050,-50) #0\n"
      "box (11050,8950;12050,9950) #0\n"
      "box (11050,18950;12050,19950) #0\n"
      "box (22050,-1050;23050,-50) #0\n"
      "box (22050,8950;23050,9950) #0\n"
      "box (22050,18950;23050,19950) #0\n"
      "box (33050,-1050;34050,-50) #0\n"
      "box (33050,8950;34050,9950) #0\n"
      "box (33050,18950;34050,19950) #0\n"
      "box (50,-1050;1050,-50) #20\n"
      "box (50,8950;1050,9950) #20\n"
      "box (50,18950;1050,19950) #20\n"
      "box (11050,-1050;12050,-50) #20\n"
      "box (11050,8950;12050,9950) #20\n"
      "box (11050,18950;12050,19950) #20\n"
      "box (22050,-1050;23050,-50) #20\n"
      "box (22050,8950;23050,9950) #20\n"
      "box (22050,18950;23050,19950) #20\n"
      "box (33050,-1050;34050,-50) #20\n"
      "box (33050,8950;34050,9950) #20\n"
      "box (33050,18950;34050,19950) #20\n"
      "box (0,100;2000,1000) #0\n"
      "box (100,200;2100,1100) #0\n"
      "box (150,150;2150,1050) #0\n"
      "box (0,100;2000,1000) #10\n"
      "box (100,200;2100,1100) #11\n"
      "box (150,150;2150,1050) #12\n"
    );

  } else if (sizeof (db::ShortBox) > 8) {

    EXPECT_EQ (shapes_to_string (_this, sa_copy),
      "box (0,100;2000,1000) #0\n"
      "box (100,200;2100,1100) #0\n"
      "box (150,150;2150,1050) #0\n"
      "box (50,-1050;1050,-50) #0\n"
      "box (50,8950;1050,9950) #0\n"
      "box (50,18950;1050,19950) #0\n"
      "box (11050,-1050;12050,-50) #0\n"
      "box (11050,8950;12050,9950) #0\n"
      "box (11050,18950;12050,19950) #0\n"
      "box (22050,-1050;23050,-50) #0\n"
      "box (22050,8950;23050,9950) #0\n"
      "box (22050,18950;23050,19950) #0\n"
      "box (33050,-1050;34050,-50) #0\n"
      "box (33050,8950;34050,9950) #0\n"
      "box (33050,18950;34050,19950) #0\n"
      "box (0,100;2000,1000) #10\n"
      "box (100,200;2100,1100) #11\n"
      "box (150,150;2150,1050) #12\n"
      "box (50,-1050;1050,-50) #20\n"
      "box (50,8950;1050,9950) #20\n"
      "box (50,18950;1050,19950) #20\n"
      "box (11050,-1050;12050,-50) #20\n"
      "box (11050,8950;12050,9950) #20\n"
      "box (11050,18950;12050,19950) #20\n"
      "box (22050,-1050;23050,-50) #20\n"
      "box (22050,8950;23050,9950) #20\n"
      "box (22050,18950;23050,19950) #20\n"
      "box (33050,-1050;34050,-50) #20\n"
      "box (33050,8950;34050,9950) #20\n"
      "box (33050,18950;34050,19950) #20\n"
    );

  } else {

    //  16 bit coordinate overflow happens during ShortBox array expansion
    EXPECT_EQ (shapes_to_string (_this, sa_copy),
      "box (0,100;2000,1000) #0\n"
      "box (100,200;2100,1100) #0\n"
      "box (150,150;2150,1050) #0\n"
      "box (50,-1050;1050,-50) #0\n"
      "box (50,8950;1050,9950) #0\n"
      "box (50,18950;1050,19950) #0\n"
      "box (11050,-1050;12050,-50) #0\n"
      "box (11050,8950;12050,9950) #0\n"
      "box (11050,18950;12050,19950) #0\n"
      "box (22050,-1050;23050,-50) #0\n"
      "box (22050,8950;23050,9950) #0\n"
      "box (22050,18950;23050,19950) #0\n"
      "box (-32486,-1050;-31486,-50) #0\n"
      "box (-32486,8950;-31486,9950) #0\n"
      "box (-32486,18950;-31486,19950) #0\n"
      "box (0,100;2000,1000) #10\n"
      "box (100,200;2100,1100) #11\n"
      "box (150,150;2150,1050) #12\n"
      "box (50,-1050;1050,-50) #20\n"
      "box (50,8950;1050,9950) #20\n"
      "box (50,18950;1050,19950) #20\n"
      "box (11050,-1050;12050,-50) #20\n"
      "box (11050,8950;12050,9950) #20\n"
      "box (11050,18950;12050,19950) #20\n"
      "box (22050,-1050;23050,-50) #20\n"
      "box (22050,8950;23050,9950) #20\n"
      "box (22050,18950;23050,19950) #20\n"
      "box (-32486,-1050;-31486,-50) #20\n"
      "box (-32486,8950;-31486,9950) #20\n"
      "box (-32486,18950;-31486,19950) #20\n"
    );

  }

  db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
  ++shape;
  db::Shape s1 = *shape;
  for (unsigned int i = 0; i < 3; ++i) { ++shape; }
  db::Shape s2 = *shape;
  for (unsigned int i = 0; i < 3; ++i) { ++shape; }
  db::Shape s3 = *shape;
  for (unsigned int i = 0; i < 12; ++i) { ++shape; }
  db::Shape s4 = *shape;

  if (db::default_editable_mode ()) { 

    topcell.shapes (lindex).erase_shape (s1);
    topcell.shapes (lindex).erase_shape (s2);
    topcell.shapes (lindex).erase_shape (s3);
    topcell.shapes (lindex).erase_shape (s4);

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)),
      "box (50,-1050;1050,-50) #0\n"
      "box (50,18950;1050,19950) #0\n"
      "box (11050,-1050;12050,-50) #0\n"
      "box (11050,18950;12050,19950) #0\n"
      "box (22050,-1050;23050,-50) #0\n"
      "box (22050,18950;23050,19950) #0\n"
      "box (33050,-1050;34050,-50) #0\n"
      "box (33050,8950;34050,9950) #0\n"
      "box (33050,18950;34050,19950) #0\n"
      "box (50,-1050;1050,-50) #20\n"
      "box (50,8950;1050,9950) #20\n"
      "box (50,18950;1050,19950) #20\n"
      "box (11050,-1050;12050,-50) #20\n"
      "box (11050,8950;12050,9950) #20\n"
      "box (11050,18950;12050,19950) #20\n"
      "box (22050,-1050;23050,-50) #20\n"
      "box (22050,18950;23050,19950) #20\n"
      "box (33050,-1050;34050,-50) #20\n"
      "box (33050,8950;34050,9950) #20\n"
      "box (33050,18950;34050,19950) #20\n"
      "box (0,100;2000,1000) #0\n"
      "box (100,200;2100,1100) #0\n"
      "box (150,150;2150,1050) #0\n"
      "box (0,100;2000,1000) #10\n"
      "box (100,200;2100,1100) #11\n"
      "box (150,150;2150,1050) #12\n"
    );

    //  test shape insert from shape reference
    copy.clear ();
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      copy.insert (*shape);
    }
    EXPECT_EQ (shapes_to_string (_this, copy), shapes_to_string (_this, topcell.shapes (lindex)));

    //  test shape insert from shape reference with transformation
    copy.clear ();
    db::ICplxTrans t (2.0, 90.0, false, db::Vector (100, -50));
    db::ICplxTrans ti = t.inverted ();
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      tl::ident_map<db::Layout::properties_id_type> pm;
      db::Shape s = copy.insert (*shape, t, pm);
      copy.transform (s, ti);
    }
    EXPECT_EQ (shapes_to_string_norm (_this, copy), shapes_to_string_norm (_this, topcell.shapes (lindex)));

    //  test shape insert from shape reference with property modification
    copy.clear ();
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      plus1 pm;
      copy.insert (*shape, pm);
    }
    for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
      if (shape->has_prop_id ()) {
        topcell.shapes (lindex).replace_prop_id (*shape, shape->prop_id () + 1);
      }
    }
    EXPECT_EQ (shapes_to_string (_this, copy), shapes_to_string (_this, topcell.shapes (lindex)));

  }
}

//  copy, move, clear with shape types
TEST(8)
{
  db::Manager m (true);

  db::Layout layout (true, &m);
  unsigned int lindex1 = layout.insert_layer ();
  unsigned int lindex2 = layout.insert_layer ();

  db::Cell &topcell = layout.cell (layout.add_cell ("TOP"));

  topcell.shapes (lindex1).insert (db::Box (1, 2, 3, 4));
  topcell.shapes (lindex1).insert (db::Polygon (db::Box (1, 2, 3, 4)));

  {
    db::Transaction trans (&m, "T1");
    topcell.shapes (lindex2).insert (topcell.shapes (lindex1));
    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex2)), "polygon (1,2;1,4;3,4;3,2) #0\nbox (1,2;3,4) #0\n");
  }

  m.undo ();
  EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex2)), "");

  {
    db::Transaction trans (&m, "T1");
    topcell.shapes (lindex2).insert (topcell.shapes (lindex1), db::ShapeIterator::Boxes);
    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex2)), "box (1,2;3,4) #0\n");
  }

  m.undo ();
  EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex2)), "");

  topcell.shapes (lindex2).insert (topcell.shapes (lindex1));
  EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex2)), "polygon (1,2;1,4;3,4;3,2) #0\nbox (1,2;3,4) #0\n");

  {
    db::Transaction trans (&m, "T1");
    topcell.shapes (lindex2).clear (db::ShapeIterator::Polygons);
    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex2)), "box (1,2;3,4) #0\n");
  }

  m.undo ();
  EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex2)), shapes_to_string (_this, topcell.shapes (lindex1)));
}

TEST(10A)
{
  if (db::default_editable_mode ()) { 

    db::Manager m (true);
    db::Layout layout (&m);
    unsigned int lindex = read_testdata (layout, 0x1);

    db::Cell &topcell = layout.cell (*layout.begin_top_down ());

    std::vector <db::Shape> to_delete;
    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      to_delete.push_back (*shape);
      ++shape;
      if (! shape.at_end ()) {
        ++shape;
      }
    }

    std::sort (to_delete.begin (), to_delete.end ());
    topcell.shapes (lindex).erase_shapes (to_delete);

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)),
      "polygon (100,200;100,2100;1100,2100;1100,200) #0\n"
      "polygon (0,100;0,2000;1000,2000;1000,100) #1\n"
      "polygon (150,150;150,2050;1150,2050;1150,150) #3\n"
      "polygon (-10,315;-10,2215;990,2215;990,315) #0\n"
      "polygon (0,105;0,2005;1000,2005;1000,105) #0\n"
      "polygon (0,20105;0,22005;1000,22005;1000,20105) #0\n"
      "polygon (11000,10105;11000,12005;12000,12005;12000,10105) #0\n"
      "polygon (22000,105;22000,2005;23000,2005;23000,105) #0\n"
      "polygon (22000,20105;22000,22005;23000,22005;23000,20105) #0\n"
      "polygon (33000,10105;33000,12005;34000,12005;34000,10105) #0\n"
      "polygon (-10,115;-10,2015;990,2015;990,115) #5\n"
      "polygon (-60,365;-60,2265;940,2265;940,365) #7\n"
      "polygon (0,10105;0,12005;1000,12005;1000,10105) #8\n"
      "polygon (11000,105;11000,2005;12000,2005;12000,105) #8\n"
      "polygon (11000,20105;11000,22005;12000,22005;12000,20105) #8\n"
      "polygon (22000,10105;22000,12005;23000,12005;23000,10105) #8\n"
      "polygon (33000,105;33000,2005;34000,2005;34000,105) #8\n"
      "polygon (33000,20105;33000,22005;34000,22005;34000,20105) #8\n"
    );

    to_delete.clear ();
    shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      to_delete.push_back (*shape);
      ++shape;
    }

    std::sort (to_delete.begin (), to_delete.end ());
    topcell.shapes (lindex).erase_shapes (to_delete);

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)), "");

  }
}

TEST(10C)
{
  if (db::default_editable_mode ()) { 

    db::Manager m (true);
    db::Layout layout (&m);
    unsigned int lindex = read_testdata (layout, 0x1);

    db::Cell &topcell = layout.cell (*layout.begin_top_down ());

    std::vector <db::Shape> to_delete;
    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      to_delete.push_back (*shape);
      ++shape;
    }

    std::sort (to_delete.begin (), to_delete.end ());
    topcell.shapes (lindex).erase_shapes (to_delete);

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)), "");

  }
}

TEST(10D)
{
  if (db::default_editable_mode ()) { 

    db::Manager m (true);
    db::Layout layout (&m);
    unsigned int lindex = read_testdata (layout, 0x1);

    db::Cell &topcell = layout.cell (*layout.begin_top_down ());

    std::vector <db::Shape> to_delete;
    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      to_delete.push_back (*shape);
      ++shape;
    }

    std::sort (to_delete.begin (), to_delete.end ());
    topcell.shapes (lindex).erase_shapes (to_delete);

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)), "");

  }
}

TEST(11A)
{
  if (db::default_editable_mode ()) { 

    db::Manager m (true);
    db::Layout layout (&m);
    unsigned int lindex = read_testdata (layout, 0x20);

    db::Cell &topcell = layout.cell (*layout.begin_top_down ());

    std::vector <db::Shape> to_delete;
    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      to_delete.push_back (*shape);
      ++shape;
      if (! shape.at_end ()) {
        ++shape;
      }
    }

    std::sort (to_delete.begin (), to_delete.end ());
    topcell.shapes (lindex).erase_shapes (to_delete);

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)),
      "box (50,8950;1050,9950) #0\n"
      "box (11050,-1050;12050,-50) #0\n"
      "box (11050,18950;12050,19950) #0\n"
      "box (22050,8950;23050,9950) #0\n"
      "box (33050,-1050;34050,-50) #0\n"
      "box (33050,18950;34050,19950) #0\n"
      "box (50,8950;1050,9950) #20\n"
      "box (11050,-1050;12050,-50) #20\n"
      "box (11050,18950;12050,19950) #20\n"
      "box (22050,8950;23050,9950) #20\n"
      "box (33050,-1050;34050,-50) #20\n"
      "box (33050,18950;34050,19950) #20\n"
      "box (100,200;2100,1100) #0\n"
      "box (0,100;2000,1000) #10\n"
      "box (150,150;2150,1050) #12\n"
    );

    to_delete.clear ();
    shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      to_delete.push_back (*shape);
      ++shape;
    }

    std::sort (to_delete.begin (), to_delete.end ());
    topcell.shapes (lindex).erase_shapes (to_delete);

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)), "");

  }
}

TEST(11C)
{
  if (db::default_editable_mode ()) {

    db::Manager m (true);
    db::Layout layout (&m);
    unsigned int lindex = read_testdata (layout, 0x20);

    db::Cell &topcell = layout.cell (*layout.begin_top_down ());

    std::vector <db::Shape> to_delete;
    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      to_delete.push_back (*shape);
      ++shape;
    }

    std::sort (to_delete.begin (), to_delete.end ());
    topcell.shapes (lindex).erase_shapes (to_delete);

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)), "");

  }
}

TEST(11D)
{
  if (db::default_editable_mode ()) { 

    db::Manager m (true);
    db::Layout layout (&m);
    unsigned int lindex = read_testdata (layout, 0x20);

    db::Cell &topcell = layout.cell (*layout.begin_top_down ());

    std::vector <db::Shape> to_delete;
    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      to_delete.push_back (*shape);
      ++shape;
    }

    std::sort (to_delete.begin (), to_delete.end ());
    topcell.shapes (lindex).erase_shapes (to_delete);

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)), "");

  }
}

TEST(11E)
{
  if (db::default_editable_mode ()) { 

    db::Manager m (true);
    db::Layout layout (&m);
    m.transaction ("y");
    unsigned int lindex = read_testdata (layout, 0x20);
    m.commit ();

    db::Cell &topcell = layout.cell (*layout.begin_top_down ());
    std::string ref_string = shapes_to_string (_this, topcell.shapes (lindex));

    std::vector <db::Shape> to_delete;
    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      to_delete.push_back (*shape);
      ++shape;
    }

    std::sort (to_delete.begin (), to_delete.end ());
    m.transaction ("x");
    topcell.shapes (lindex).erase_shapes (to_delete);
    m.commit ();

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)), "");

    m.undo ();
    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)), ref_string);

    m.undo ();
    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)), "");

  }
}

TEST(11F)
{
  if (db::default_editable_mode ()) {

    db::Manager m (true);
    db::Layout layout (&m);
    m.transaction ("y");
    unsigned int lindex = read_testdata (layout, 0x20);
    m.commit ();

    db::Cell &topcell = layout.cell (*layout.begin_top_down ());

    std::vector <db::Shape> to_delete;
    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      to_delete.push_back (*shape);
      ++shape;
      if (! shape.at_end ()) {
        ++shape;
      }
    }

    std::sort (to_delete.begin (), to_delete.end ());
    m.transaction ("x");
    topcell.shapes (lindex).erase_shapes (to_delete);
    m.commit ();

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)),
      "box (50,8950;1050,9950) #0\n"
      "box (11050,-1050;12050,-50) #0\n"
      "box (11050,18950;12050,19950) #0\n"
      "box (22050,8950;23050,9950) #0\n"
      "box (33050,-1050;34050,-50) #0\n"
      "box (33050,18950;34050,19950) #0\n"
      "box (50,8950;1050,9950) #20\n"
      "box (11050,-1050;12050,-50) #20\n"
      "box (11050,18950;12050,19950) #20\n"
      "box (22050,8950;23050,9950) #20\n"
      "box (33050,-1050;34050,-50) #20\n"
      "box (33050,18950;34050,19950) #20\n"
      "box (100,200;2100,1100) #0\n"
      "box (0,100;2000,1000) #10\n"
      "box (150,150;2150,1050) #12\n"
    );

    to_delete.clear ();
    shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      to_delete.push_back (*shape);
      ++shape;
    }

    std::sort (to_delete.begin (), to_delete.end ());
    m.transaction ("z");
    topcell.shapes (lindex).erase_shapes (to_delete);
    m.commit ();

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)), "");

    m.undo ();
    m.undo ();

    EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)),
      "box (0,100;2000,1000) #0\n"
      "box (0,100;2000,1000) #10\n"
      "box (100,200;2100,1100) #0\n"
      "box (100,200;2100,1100) #11\n"
      "box (11050,-1050;12050,-50) #0\n"
      "box (11050,-1050;12050,-50) #20\n"
      "box (11050,18950;12050,19950) #0\n"
      "box (11050,18950;12050,19950) #20\n"
      "box (11050,8950;12050,9950) #0\n"
      "box (11050,8950;12050,9950) #20\n"
      "box (150,150;2150,1050) #0\n"
      "box (150,150;2150,1050) #12\n"
      "box (22050,-1050;23050,-50) #0\n"
      "box (22050,-1050;23050,-50) #20\n"
      "box (22050,18950;23050,19950) #0\n"
      "box (22050,18950;23050,19950) #20\n"
      "box (22050,8950;23050,9950) #0\n"
      "box (22050,8950;23050,9950) #20\n"
      "box (33050,-1050;34050,-50) #0\n"
      "box (33050,-1050;34050,-50) #20\n"
      "box (33050,18950;34050,19950) #0\n"
      "box (33050,18950;34050,19950) #20\n"
      "box (33050,8950;34050,9950) #0\n"
      "box (33050,8950;34050,9950) #20\n"
      "box (50,-1050;1050,-50) #0\n"
      "box (50,-1050;1050,-50) #20\n"
      "box (50,18950;1050,19950) #0\n"
      "box (50,18950;1050,19950) #20\n"
      "box (50,8950;1050,9950) #0\n"
      "box (50,8950;1050,9950) #20\n"
    );

    m.undo ();
    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)), "");

  }
}

TEST(12A)
{
  if (db::default_editable_mode ()) { 

    db::Manager m (true);
    db::Layout layout (&m);
    unsigned int lindex = read_testdata (layout, 0x20 | 0x80);  // short box, no arrays

    db::Cell &topcell = layout.cell (*layout.begin_top_down ());

    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      topcell.shapes (lindex).replace (*shape, db::Box (shape->box ().transformed (db::Trans (1))));
      ++shape;
    }

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)),
      "box (-1000,0;-100,2000) #0\n"
      "box (-1100,100;-200,2100) #0\n"
      "box (-1050,150;-150,2150) #0\n"
      "box (-1000,0;-100,2000) #10\n"
      "box (-1100,100;-200,2100) #11\n"
      "box (-1050,150;-150,2150) #12\n"
    );

    shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      if (shape->with_props ()) {
        topcell.shapes (lindex).replace_prop_id (*shape, shape->prop_id () + 100);
      }
      ++shape;
    }

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)),
      "box (-1000,0;-100,2000) #0\n"
      "box (-1100,100;-200,2100) #0\n"
      "box (-1050,150;-150,2150) #0\n"
      "box (-1000,0;-100,2000) #110\n"
      "box (-1100,100;-200,2100) #111\n"
      "box (-1050,150;-150,2150) #112\n"
    );

  }
}

TEST(12B)
{
  if (db::default_editable_mode ()) { 

    db::Manager m (true);
    db::Layout layout (&m);
    unsigned int lindex = read_testdata (layout, 0x20 | 0x80);  // short box, no arrays

    db::Cell &topcell = layout.cell (*layout.begin_top_down ());

    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      topcell.shapes (lindex).replace (*shape, db::ShortBox (shape->box ().transformed (db::Trans (1))));
      ++shape;
    }

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)),
      "box (-1000,0;-100,2000) #0\n"
      "box (-1100,100;-200,2100) #0\n"
      "box (-1050,150;-150,2150) #0\n"
      "box (-1000,0;-100,2000) #10\n"
      "box (-1100,100;-200,2100) #11\n"
      "box (-1050,150;-150,2150) #12\n"
    );

  }
}

TEST(12C)
{
  if (db::default_editable_mode ()) { 

    db::Manager m (true);
    db::Layout layout (&m);
    unsigned int lindex = read_testdata (layout, 0x10 | 0x80);  // box, no arrays

    db::Cell &topcell = layout.cell (*layout.begin_top_down ());

    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      topcell.shapes (lindex).replace (*shape, db::Box (shape->box ().transformed (db::Trans (1))));
      ++shape;
    }

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)),
      "box (-1000,0;-100,2000) #0\n"
      "box (-1100,100;-200,2100) #0\n"
      "box (-1050,150;-150,2150) #0\n"
      "box (-1000,0;-100,2000) #10\n"
      "box (-1100,100;-200,2100) #11\n"
      "box (-1050,150;-150,2150) #12\n"
    );

    shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      if (shape->with_props ()) {
        topcell.shapes (lindex).replace_prop_id (*shape, shape->prop_id () + 100);
      }
      ++shape;
    }

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)),
      "box (-1000,0;-100,2000) #0\n"
      "box (-1100,100;-200,2100) #0\n"
      "box (-1050,150;-150,2150) #0\n"
      "box (-1000,0;-100,2000) #110\n"
      "box (-1100,100;-200,2100) #111\n"
      "box (-1050,150;-150,2150) #112\n"
    );

  }
}

TEST(12D)
{
  if (db::default_editable_mode ()) { 

    db::Manager m (true);
    db::Layout layout (&m);
    unsigned int lindex = read_testdata (layout, 0x10 | 0x80);  // box, no arrays

    db::Cell &topcell = layout.cell (*layout.begin_top_down ());

    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      topcell.shapes (lindex).replace (*shape, db::ShortBox (shape->box ().transformed (db::Trans (1))));
      ++shape;
    }

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)),
      "box (-1000,0;-100,2000) #0\n"
      "box (-1100,100;-200,2100) #0\n"
      "box (-1050,150;-150,2150) #0\n"
      "box (-1000,0;-100,2000) #10\n"
      "box (-1100,100;-200,2100) #11\n"
      "box (-1050,150;-150,2150) #12\n"
    );

  }
}

TEST(12E)
{
  if (db::default_editable_mode ()) { 

    db::Manager m (true);
    db::Layout layout (&m);
    unsigned int lindex = read_testdata (layout, 0x01 | 0x80);  // simple polygon, no arrays

    db::Cell &topcell = layout.cell (*layout.begin_top_down ());

    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      db::SimplePolygon sp;
      shape->simple_polygon (sp);
      sp.transform (db::Trans (1));
      db::Polygon pp;
      pp.assign_hull (sp.begin_hull (), sp.end_hull ());
      topcell.shapes (lindex).replace (*shape, pp);
      ++shape;
    }

    EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)),
      "polygon (-2000,0;-2000,1000;-100,1000;-100,0) #0\n"
      "polygon (-2000,0;-2000,1000;-100,1000;-100,0) #1\n"
      "polygon (-2015,-10;-2015,990;-115,990;-115,-10) #0\n"
      "polygon (-2015,-10;-2015,990;-115,990;-115,-10) #5\n"
      "polygon (-2050,150;-2050,1150;-150,1150;-150,150) #0\n"
      "polygon (-2050,150;-2050,1150;-150,1150;-150,150) #3\n"
      "polygon (-2100,100;-2100,1100;-200,1100;-200,100) #0\n"
      "polygon (-2100,100;-2100,1100;-200,1100;-200,100) #2\n"
      "polygon (-2215,-10;-2215,990;-315,990;-315,-10) #0\n"
      "polygon (-2215,-10;-2215,990;-315,990;-315,-10) #6\n"
      "polygon (-2265,-60;-2265,940;-365,940;-365,-60) #0\n"
      "polygon (-2265,-60;-2265,940;-365,940;-365,-60) #7\n"
    );

    shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      if (shape->with_props ()) {
        topcell.shapes (lindex).replace_prop_id (*shape, shape->prop_id () + 100);
      }
      ++shape;
    }

    EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)),
      "polygon (-2000,0;-2000,1000;-100,1000;-100,0) #0\n"
      "polygon (-2000,0;-2000,1000;-100,1000;-100,0) #101\n"
      "polygon (-2015,-10;-2015,990;-115,990;-115,-10) #0\n"
      "polygon (-2015,-10;-2015,990;-115,990;-115,-10) #105\n"
      "polygon (-2050,150;-2050,1150;-150,1150;-150,150) #0\n"
      "polygon (-2050,150;-2050,1150;-150,1150;-150,150) #103\n"
      "polygon (-2100,100;-2100,1100;-200,1100;-200,100) #0\n"
      "polygon (-2100,100;-2100,1100;-200,1100;-200,100) #102\n"
      "polygon (-2215,-10;-2215,990;-315,990;-315,-10) #0\n"
      "polygon (-2215,-10;-2215,990;-315,990;-315,-10) #106\n"
      "polygon (-2265,-60;-2265,940;-365,940;-365,-60) #0\n"
      "polygon (-2265,-60;-2265,940;-365,940;-365,-60) #107\n"
    );

  }
}

TEST(12F)
{
  if (db::default_editable_mode ()) { 

    db::Manager m (true);
    db::Layout layout (&m);
    unsigned int lindex = read_testdata (layout, 0x01 | 0x80);  // simple polygon, no arrays

    db::Cell &topcell = layout.cell (*layout.begin_top_down ());

    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      db::SimplePolygon sp;
      shape->simple_polygon (sp);
      sp.transform (db::Trans (1));
      topcell.shapes (lindex).replace (*shape, sp);
      ++shape;
    }

    EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)),
      "polygon (-2000,0;-2000,1000;-100,1000;-100,0) #0\n"
      "polygon (-2000,0;-2000,1000;-100,1000;-100,0) #1\n"
      "polygon (-2015,-10;-2015,990;-115,990;-115,-10) #0\n"
      "polygon (-2015,-10;-2015,990;-115,990;-115,-10) #5\n"
      "polygon (-2050,150;-2050,1150;-150,1150;-150,150) #0\n"
      "polygon (-2050,150;-2050,1150;-150,1150;-150,150) #3\n"
      "polygon (-2100,100;-2100,1100;-200,1100;-200,100) #0\n"
      "polygon (-2100,100;-2100,1100;-200,1100;-200,100) #2\n"
      "polygon (-2215,-10;-2215,990;-315,990;-315,-10) #0\n"
      "polygon (-2215,-10;-2215,990;-315,990;-315,-10) #6\n"
      "polygon (-2265,-60;-2265,940;-365,940;-365,-60) #0\n"
      "polygon (-2265,-60;-2265,940;-365,940;-365,-60) #7\n"
    );

  }
}

TEST(12G)
{
  if (db::default_editable_mode ()) { 

    db::Manager m (true);
    db::Layout layout (&m);
    unsigned int lindex = read_testdata (layout, 0x02 | 0x80);  // polygon, no arrays

    db::Cell &topcell = layout.cell (*layout.begin_top_down ());

    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      db::Polygon sp;
      shape->polygon (sp);
      sp.transform (db::Trans (1));
      db::SimplePolygon pp;
      pp.assign_hull (sp.begin_hull (), sp.end_hull ());
      topcell.shapes (lindex).replace (*shape, pp);
      ++shape;
    }

    EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)),
      "polygon (-1000,0;-1000,2000;-100,2000;-100,0) #0\n"
      "polygon (-1000,0;-1000,2000;-100,2000;-100,0) #1\n"
      "polygon (-1015,-10;-1015,1990;-115,1990;-115,-10) #0\n"
      "polygon (-1015,-10;-1015,1990;-115,1990;-115,-10) #5\n"
      "polygon (-1050,150;-1050,2150;-150,2150;-150,150) #0\n"
      "polygon (-1050,150;-1050,2150;-150,2150;-150,150) #3\n"
      "polygon (-1100,100;-1100,2100;-200,2100;-200,100) #0\n"
      "polygon (-1100,100;-1100,2100;-200,2100;-200,100) #2\n"
      "polygon (-1215,-10;-1215,1990;-315,1990;-315,-10) #0\n"
      "polygon (-1215,-10;-1215,1990;-315,1990;-315,-10) #6\n"
      "polygon (-1265,-60;-1265,1940;-365,1940;-365,-60) #0\n"
      "polygon (-1265,-60;-1265,1940;-365,1940;-365,-60) #7\n"
    );

    shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      if (shape->with_props ()) {
        topcell.shapes (lindex).replace_prop_id (*shape, shape->prop_id () + 100);
      }
      ++shape;
    }

    EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)),
      "polygon (-1000,0;-1000,2000;-100,2000;-100,0) #0\n"
      "polygon (-1000,0;-1000,2000;-100,2000;-100,0) #101\n"
      "polygon (-1015,-10;-1015,1990;-115,1990;-115,-10) #0\n"
      "polygon (-1015,-10;-1015,1990;-115,1990;-115,-10) #105\n"
      "polygon (-1050,150;-1050,2150;-150,2150;-150,150) #0\n"
      "polygon (-1050,150;-1050,2150;-150,2150;-150,150) #103\n"
      "polygon (-1100,100;-1100,2100;-200,2100;-200,100) #0\n"
      "polygon (-1100,100;-1100,2100;-200,2100;-200,100) #102\n"
      "polygon (-1215,-10;-1215,1990;-315,1990;-315,-10) #0\n"
      "polygon (-1215,-10;-1215,1990;-315,1990;-315,-10) #106\n"
      "polygon (-1265,-60;-1265,1940;-365,1940;-365,-60) #0\n"
      "polygon (-1265,-60;-1265,1940;-365,1940;-365,-60) #107\n"
    );

  }
}

TEST(12H)
{
  if (db::default_editable_mode ()) { 

    db::Manager m (true);
    db::Layout layout (&m);
    unsigned int lindex = read_testdata (layout, 0x02 | 0x80);  // polygon, no arrays

    db::Cell &topcell = layout.cell (*layout.begin_top_down ());

    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      db::Polygon sp;
      shape->polygon (sp);
      sp.transform (db::Trans (1));
      topcell.shapes (lindex).replace (*shape, sp);
      ++shape;
    }

    EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)),
      "polygon (-1000,0;-1000,2000;-100,2000;-100,0) #0\n"
      "polygon (-1000,0;-1000,2000;-100,2000;-100,0) #1\n"
      "polygon (-1015,-10;-1015,1990;-115,1990;-115,-10) #0\n"
      "polygon (-1015,-10;-1015,1990;-115,1990;-115,-10) #5\n"
      "polygon (-1050,150;-1050,2150;-150,2150;-150,150) #0\n"
      "polygon (-1050,150;-1050,2150;-150,2150;-150,150) #3\n"
      "polygon (-1100,100;-1100,2100;-200,2100;-200,100) #0\n"
      "polygon (-1100,100;-1100,2100;-200,2100;-200,100) #2\n"
      "polygon (-1215,-10;-1215,1990;-315,1990;-315,-10) #0\n"
      "polygon (-1215,-10;-1215,1990;-315,1990;-315,-10) #6\n"
      "polygon (-1265,-60;-1265,1940;-365,1940;-365,-60) #0\n"
      "polygon (-1265,-60;-1265,1940;-365,1940;-365,-60) #7\n"
    );

  }
}

TEST(12I)
{
  if (db::default_editable_mode ()) { 

    db::Manager m (true);
    db::Layout layout (&m);
    unsigned int lindex = read_testdata (layout, 0x04 | 0x80);  // path, no arrays

    db::Cell &topcell = layout.cell (*layout.begin_top_down ());

    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      db::Path sp;
      shape->path (sp);
      sp.transform (db::Trans (1));
      topcell.shapes (lindex).replace (*shape, sp);
      ++shape;
    }

    EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)),
      "path (-100,0;-500,0;-700,200) w=100 bx=0 ex=0 r=false #0\n"
      "path (-100,0;-500,0;-700,200) w=100 bx=0 ex=0 r=false #1\n"
      "path (-1100,0;-1500,0;-1300,200) w=150 bx=0 ex=0 r=false #0\n"
      "path (-1100,0;-1500,0;-1300,200) w=150 bx=0 ex=0 r=false #2\n"
      "path (-115,-10;-515,-10;-715,190) w=100 bx=0 ex=0 r=false #0\n"
      "path (-115,-10;-515,-10;-715,190) w=100 bx=0 ex=0 r=false #5\n"
      "path (-1215,-110;-1615,-110;-1415,90) w=150 bx=0 ex=0 r=false #0\n"
      "path (-1215,-110;-1615,-110;-1415,90) w=150 bx=0 ex=0 r=false #6\n"
      "path (-2100,0;-2500,0;-2700,-200) w=200 bx=0 ex=0 r=false #0\n"
      "path (-2100,0;-2500,0;-2700,-200) w=200 bx=0 ex=0 r=false #3\n"
      "path (-2315,-210;-2715,-210;-2915,-410) w=200 bx=0 ex=0 r=false #0\n"
      "path (-2315,-210;-2715,-210;-2915,-410) w=200 bx=0 ex=0 r=false #7\n"
    );

    shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      if (shape->with_props ()) {
        topcell.shapes (lindex).replace_prop_id (*shape, shape->prop_id () + 100);
      }
      ++shape;
    }

    EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)),
      "path (-100,0;-500,0;-700,200) w=100 bx=0 ex=0 r=false #0\n"
      "path (-100,0;-500,0;-700,200) w=100 bx=0 ex=0 r=false #101\n"
      "path (-1100,0;-1500,0;-1300,200) w=150 bx=0 ex=0 r=false #0\n"
      "path (-1100,0;-1500,0;-1300,200) w=150 bx=0 ex=0 r=false #102\n"
      "path (-115,-10;-515,-10;-715,190) w=100 bx=0 ex=0 r=false #0\n"
      "path (-115,-10;-515,-10;-715,190) w=100 bx=0 ex=0 r=false #105\n"
      "path (-1215,-110;-1615,-110;-1415,90) w=150 bx=0 ex=0 r=false #0\n"
      "path (-1215,-110;-1615,-110;-1415,90) w=150 bx=0 ex=0 r=false #106\n"
      "path (-2100,0;-2500,0;-2700,-200) w=200 bx=0 ex=0 r=false #0\n"
      "path (-2100,0;-2500,0;-2700,-200) w=200 bx=0 ex=0 r=false #103\n"
      "path (-2315,-210;-2715,-210;-2915,-410) w=200 bx=0 ex=0 r=false #0\n"
      "path (-2315,-210;-2715,-210;-2915,-410) w=200 bx=0 ex=0 r=false #107\n"
    );

  }
}

TEST(12J)
{
  if (db::default_editable_mode ()) { 

    db::Manager m (true);
    db::Layout layout (&m);
    unsigned int lindex = read_testdata (layout, 0x08 | 0x80);  // text, no arrays

    db::Cell &topcell = layout.cell (*layout.begin_top_down ());

    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      db::Text sp;
      shape->text (sp);
      sp.transform (db::Trans (1));
      topcell.shapes (lindex).replace (*shape, sp);
      ++shape;
    }

    EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)),
      "text ('A',r90 -35,10) #0\n"
      "text ('A',r90 -35,10) #1\n"
      "text ('A',r90 -50,0) #0\n"
      "text ('A',r90 -50,0) #5\n"
      "text ('B',r180 -140,-90) #0\n"
      "text ('B',r180 -140,-90) #6\n"
      "text ('B',r180 -25,20) #0\n"
      "text ('B',r180 -25,20) #2\n"
      "text ('C',m135 -15,30) #0\n"
      "text ('C',m135 -15,30) #3\n"
      "text ('C',m135 -230,-180) #0\n"
      "text ('C',m135 -230,-180) #7\n"
    );

    shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    while (! shape.at_end ()) {
      if (shape->with_props ()) {
        topcell.shapes (lindex).replace_prop_id (*shape, shape->prop_id () + 100);
      }
      ++shape;
    }

    EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)),
      "text ('A',r90 -35,10) #0\n"
      "text ('A',r90 -35,10) #101\n"
      "text ('A',r90 -50,0) #0\n"
      "text ('A',r90 -50,0) #105\n"
      "text ('B',r180 -140,-90) #0\n"
      "text ('B',r180 -140,-90) #106\n"
      "text ('B',r180 -25,20) #0\n"
      "text ('B',r180 -25,20) #102\n"
      "text ('C',m135 -15,30) #0\n"
      "text ('C',m135 -15,30) #103\n"
      "text ('C',m135 -230,-180) #0\n"
      "text ('C',m135 -230,-180) #107\n"
    );

  }
}

TEST(13)
{
  db::Manager m (true);
  db::Layout layout (&m);
  unsigned int lindex = read_testdata (layout, 0x3f); // all with arrays

  db::Cell &topcell = layout.cell (*layout.begin_top_down ());

  if (db::default_editable_mode ()) { 

    //  replace all with first one
    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    db::Shape shape0 = *shape;
    db::Shapes ref;
    while (! shape.at_end ()) {
      ref.insert (db::BoxWithProperties (shape0.bbox (), shape->prop_id ()));
      db::Shape new_shape = topcell.shapes (lindex).replace (*shape, shape0.bbox ());
      if (*shape == shape0) {
        shape0 = new_shape;
      }
      ++shape;
    }

    EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)), shapes_to_string_norm (_this, ref));

  }
}

TEST(14)
{
  db::Manager m (true);
  db::Layout layout (&m);
  unsigned int lindex = read_testdata (layout, 0x10); // boxes

  db::Cell &topcell = layout.cell (*layout.begin_top_down ());

  if (db::default_editable_mode ()) { 

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)),
      "box (0,100;2000,1000) #0\n"
      "box (100,200;2100,1100) #0\n"
      "box (150,150;2150,1050) #0\n"
      "box (50,-1050;1050,-50) #0\n"
      "box (50,8950;1050,9950) #0\n"
      "box (50,18950;1050,19950) #0\n"
      "box (11050,-1050;12050,-50) #0\n"
      "box (11050,8950;12050,9950) #0\n"
      "box (11050,18950;12050,19950) #0\n"
      "box (22050,-1050;23050,-50) #0\n"
      "box (22050,8950;23050,9950) #0\n"
      "box (22050,18950;23050,19950) #0\n"
      "box (33050,-1050;34050,-50) #0\n"
      "box (33050,8950;34050,9950) #0\n"
      "box (33050,18950;34050,19950) #0\n"
      "box (0,100;2000,1000) #10\n"
      "box (100,200;2100,1100) #11\n"
      "box (150,150;2150,1050) #12\n"
      "box (50,-1050;1050,-50) #20\n"
      "box (50,8950;1050,9950) #20\n"
      "box (50,18950;1050,19950) #20\n"
      "box (11050,-1050;12050,-50) #20\n"
      "box (11050,8950;12050,9950) #20\n"
      "box (11050,18950;12050,19950) #20\n"
      "box (22050,-1050;23050,-50) #20\n"
      "box (22050,8950;23050,9950) #20\n"
      "box (22050,18950;23050,19950) #20\n"
      "box (33050,-1050;34050,-50) #20\n"
      "box (33050,8950;34050,9950) #20\n"
      "box (33050,18950;34050,19950) #20\n"
    );

    //  replace all with first one
    db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All);
    ++shape;
    topcell.shapes (lindex).transform (*shape, db::ICplxTrans (2.5, 45.0, false, db::Vector ()));
    for (unsigned int i = 0; i < 15; ++i) {
      ++shape;
    }
    topcell.shapes (lindex).transform (*shape, db::ICplxTrans (2.5, 45.0, false, db::Vector ()));

    EXPECT_EQ (shapes_to_string (_this, topcell.shapes (lindex)),
      "polygon (-177,530;-1768,2121;1768,5657;3359,4066) #0\n"
      "polygon (-177,530;-1768,2121;1768,5657;3359,4066) #11\n"
      "box (0,100;2000,1000) #0\n"
      "box (150,150;2150,1050) #0\n"
      "box (50,-1050;1050,-50) #0\n"
      "box (50,8950;1050,9950) #0\n"
      "box (50,18950;1050,19950) #0\n"
      "box (11050,-1050;12050,-50) #0\n"
      "box (11050,8950;12050,9950) #0\n"
      "box (11050,18950;12050,19950) #0\n"
      "box (22050,-1050;23050,-50) #0\n"
      "box (22050,8950;23050,9950) #0\n"
      "box (22050,18950;23050,19950) #0\n"
      "box (33050,-1050;34050,-50) #0\n"
      "box (33050,8950;34050,9950) #0\n"
      "box (33050,18950;34050,19950) #0\n"
      "box (0,100;2000,1000) #10\n"
      "box (150,150;2150,1050) #12\n"
      "box (50,-1050;1050,-50) #20\n"
      "box (50,8950;1050,9950) #20\n"
      "box (50,18950;1050,19950) #20\n"
      "box (11050,-1050;12050,-50) #20\n"
      "box (11050,8950;12050,9950) #20\n"
      "box (11050,18950;12050,19950) #20\n"
      "box (22050,-1050;23050,-50) #20\n"
      "box (22050,8950;23050,9950) #20\n"
      "box (22050,18950;23050,19950) #20\n"
      "box (33050,-1050;34050,-50) #20\n"
      "box (33050,8950;34050,9950) #20\n"
      "box (33050,18950;34050,19950) #20\n"
    );

  }
}

TEST(15)
{
  db::Manager m (true);
  db::Layout layout (&m);
  unsigned int lindex = read_testdata (layout, 0x1);

  db::Cell &topcell = layout.cell (*layout.begin_top_down ());
  db::Shapes copy (&m, &topcell, db::default_editable_mode ());

  EXPECT_EQ (shapes_to_string_norm (_this, topcell.shapes (lindex)),
    "polygon (-10,115;-10,2015;990,2015;990,115) #0\n"
    "polygon (-10,115;-10,2015;990,2015;990,115) #5\n"
    "polygon (-10,315;-10,2215;990,2215;990,315) #0\n"
    "polygon (-10,315;-10,2215;990,2215;990,315) #6\n"
    "polygon (-60,365;-60,2265;940,2265;940,365) #0\n"
    "polygon (-60,365;-60,2265;940,2265;940,365) #7\n"
    "polygon (0,100;0,2000;1000,2000;1000,100) #0\n"
    "polygon (0,100;0,2000;1000,2000;1000,100) #1\n"
    "polygon (0,10105;0,12005;1000,12005;1000,10105) #0\n"
    "polygon (0,10105;0,12005;1000,12005;1000,10105) #8\n"
    "polygon (0,105;0,2005;1000,2005;1000,105) #0\n"
    "polygon (0,105;0,2005;1000,2005;1000,105) #8\n"
    "polygon (0,20105;0,22005;1000,22005;1000,20105) #0\n"
    "polygon (0,20105;0,22005;1000,22005;1000,20105) #8\n"
    "polygon (100,200;100,2100;1100,2100;1100,200) #0\n"
    "polygon (100,200;100,2100;1100,2100;1100,200) #2\n"
    "polygon (11000,10105;11000,12005;12000,12005;12000,10105) #0\n"
    "polygon (11000,10105;11000,12005;12000,12005;12000,10105) #8\n"
    "polygon (11000,105;11000,2005;12000,2005;12000,105) #0\n"
    "polygon (11000,105;11000,2005;12000,2005;12000,105) #8\n"
    "polygon (11000,20105;11000,22005;12000,22005;12000,20105) #0\n"
    "polygon (11000,20105;11000,22005;12000,22005;12000,20105) #8\n"
    "polygon (150,150;150,2050;1150,2050;1150,150) #0\n"
    "polygon (150,150;150,2050;1150,2050;1150,150) #3\n"
    "polygon (22000,10105;22000,12005;23000,12005;23000,10105) #0\n"
    "polygon (22000,10105;22000,12005;23000,12005;23000,10105) #8\n"
    "polygon (22000,105;22000,2005;23000,2005;23000,105) #0\n"
    "polygon (22000,105;22000,2005;23000,2005;23000,105) #8\n"
    "polygon (22000,20105;22000,22005;23000,22005;23000,20105) #0\n"
    "polygon (22000,20105;22000,22005;23000,22005;23000,20105) #8\n"
    "polygon (33000,10105;33000,12005;34000,12005;34000,10105) #0\n"
    "polygon (33000,10105;33000,12005;34000,12005;34000,10105) #8\n"
    "polygon (33000,105;33000,2005;34000,2005;34000,105) #0\n"
    "polygon (33000,105;33000,2005;34000,2005;34000,105) #8\n"
    "polygon (33000,20105;33000,22005;34000,22005;34000,20105) #0\n"
    "polygon (33000,20105;33000,22005;34000,22005;34000,20105) #8\n"
  );

  db::Shapes sa_copy;
  for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin_touching (db::Box (0, 0, 200, 200), db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
    sa_copy.insert (*shape);
  }

  EXPECT_EQ (shapes_to_string_norm (_this, sa_copy),
    "polygon (-10,115;-10,2015;990,2015;990,115) #0\n"
    "polygon (-10,115;-10,2015;990,2015;990,115) #5\n"
    "polygon (0,100;0,2000;1000,2000;1000,100) #0\n"
    "polygon (0,100;0,2000;1000,2000;1000,100) #1\n"
    "polygon (0,105;0,2005;1000,2005;1000,105) #0\n"
    "polygon (0,105;0,2005;1000,2005;1000,105) #8\n"
    "polygon (100,200;100,2100;1100,2100;1100,200) #0\n"
    "polygon (100,200;100,2100;1100,2100;1100,200) #2\n"
    "polygon (150,150;150,2050;1150,2050;1150,150) #0\n"
    "polygon (150,150;150,2050;1150,2050;1150,150) #3\n"
  );

  sa_copy.clear ();
  for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin_touching (db::Box::world (), db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
    sa_copy.insert (*shape);
  }

  EXPECT_EQ (shapes_to_string_norm (_this, sa_copy),
    "polygon (-10,115;-10,2015;990,2015;990,115) #0\n"
    "polygon (-10,115;-10,2015;990,2015;990,115) #5\n"
    "polygon (-10,315;-10,2215;990,2215;990,315) #0\n"
    "polygon (-10,315;-10,2215;990,2215;990,315) #6\n"
    "polygon (-60,365;-60,2265;940,2265;940,365) #0\n"
    "polygon (-60,365;-60,2265;940,2265;940,365) #7\n"
    "polygon (0,100;0,2000;1000,2000;1000,100) #0\n"
    "polygon (0,100;0,2000;1000,2000;1000,100) #1\n"
    "polygon (0,10105;0,12005;1000,12005;1000,10105) #0\n"
    "polygon (0,10105;0,12005;1000,12005;1000,10105) #8\n"
    "polygon (0,105;0,2005;1000,2005;1000,105) #0\n"
    "polygon (0,105;0,2005;1000,2005;1000,105) #8\n"
    "polygon (0,20105;0,22005;1000,22005;1000,20105) #0\n"
    "polygon (0,20105;0,22005;1000,22005;1000,20105) #8\n"
    "polygon (100,200;100,2100;1100,2100;1100,200) #0\n"
    "polygon (100,200;100,2100;1100,2100;1100,200) #2\n"
    "polygon (11000,10105;11000,12005;12000,12005;12000,10105) #0\n"
    "polygon (11000,10105;11000,12005;12000,12005;12000,10105) #8\n"
    "polygon (11000,105;11000,2005;12000,2005;12000,105) #0\n"
    "polygon (11000,105;11000,2005;12000,2005;12000,105) #8\n"
    "polygon (11000,20105;11000,22005;12000,22005;12000,20105) #0\n"
    "polygon (11000,20105;11000,22005;12000,22005;12000,20105) #8\n"
    "polygon (150,150;150,2050;1150,2050;1150,150) #0\n"
    "polygon (150,150;150,2050;1150,2050;1150,150) #3\n"
    "polygon (22000,10105;22000,12005;23000,12005;23000,10105) #0\n"
    "polygon (22000,10105;22000,12005;23000,12005;23000,10105) #8\n"
    "polygon (22000,105;22000,2005;23000,2005;23000,105) #0\n"
    "polygon (22000,105;22000,2005;23000,2005;23000,105) #8\n"
    "polygon (22000,20105;22000,22005;23000,22005;23000,20105) #0\n"
    "polygon (22000,20105;22000,22005;23000,22005;23000,20105) #8\n"
    "polygon (33000,10105;33000,12005;34000,12005;34000,10105) #0\n"
    "polygon (33000,10105;33000,12005;34000,12005;34000,10105) #8\n"
    "polygon (33000,105;33000,2005;34000,2005;34000,105) #0\n"
    "polygon (33000,105;33000,2005;34000,2005;34000,105) #8\n"
    "polygon (33000,20105;33000,22005;34000,22005;34000,20105) #0\n"
    "polygon (33000,20105;33000,22005;34000,22005;34000,20105) #8\n"
  );
}

TEST(16)
{
  db::Manager m (true);
  db::Layout layout (&m);
  unsigned int lindex = read_testdata (layout, 0x1);

  db::Cell &topcell = layout.cell (*layout.begin_top_down ());

  db::Layout layout2 (&m);
  unsigned int lindex2 = read_testdata (layout2, 0x100);

  db::Cell &topcell2 = layout2.cell (*layout2.begin_top_down ());

  for (db::Shapes::shape_iterator shape = topcell.shapes (lindex).begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
    topcell2.shapes (lindex2).insert (*shape);
  }

  std::string s = shapes_to_string_norm (_this, topcell.shapes (lindex));
  db::Layout empty_layout;
  layout = empty_layout;

  EXPECT_EQ (shapes_to_string_norm (_this, topcell2.shapes (lindex2)), s);
}

TEST(17)
{
  db::Shapes shapes;

  tl::SelfTimer timer ("insert/transform sequence");

  db::Point ar, br;

  //  test performance of insert/transform sequences
  for (unsigned int i = 0; i < 50000; ++i) {
    db::Box b (0, 0, 10, 10);
    db::Shape shape = shapes.insert (b);
    db::Trans t (db::Vector (i, 50));
    shape = shapes.transform(shape, t);
    b.transform (t);
    ar += b.lower_left () - db::Point ();
    br += b.upper_right () - db::Point ();
  }

  db::Point af, bf;

  for (db::Shapes::shape_iterator shape = shapes.begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
    af += shape->box ().lower_left () - db::Point ();
    bf += shape->box ().upper_right () - db::Point ();
  }

  EXPECT_EQ (af, ar);
  EXPECT_EQ (bf, br);
}

TEST(18)
{
  db::Shapes shapes;

  tl::SelfTimer timer ("insert/transform sequence");

  db::Point ar, br;

  //  test performance of insert/transform sequences
  for (unsigned int i = 0; i < 50000; ++i) {
    db::Box b (0, 0, 10, 10);
    db::Shape shape = shapes.insert (b);
    db::Trans t (db::Vector (i, 50));
    shape = shapes.transform(shape, t);
    b.transform (t);
    ar += b.lower_left () - db::Point ();
    br += b.upper_right () - db::Point ();
  }

  db::Point af, bf;

  for (db::Shapes::shape_iterator shape = shapes.begin (db::Shapes::shape_iterator::All); ! shape.at_end (); ++shape) {
    af += shape->box ().lower_left () - db::Point ();
    bf += shape->box ().upper_right () - db::Point ();
  }

  EXPECT_EQ (af, ar);
  EXPECT_EQ (bf, br);
}

TEST(19)
{
  db::Shapes shapes0;
  shapes0.insert (db::BoxWithProperties (db::Box (0, 0, 100, 100), 1));
  shapes0.insert (db::BoxWithProperties (db::Box (0, 0, 100, 100), 2));

  db::Shapes shapes;

  if (db::default_editable_mode ()) { 

    db::ShapeIterator s = shapes0.begin (db::ShapeIterator::All);
    plus1 pm;
    shapes.insert (*s, db::Trans (2), pm);
    ++s;
    shapes.insert (*s, db::ICplxTrans (1.5, 45.0, false, db::Vector ()), pm);
    ++s;

    EXPECT_EQ (shapes_to_string_norm (_this, shapes),
      "box (-100,-100;0,0) #2\n"
      "polygon (0,0;-106,106;0,212;106,106) #3\n"
    );

  }
}

TEST(20)
{
  db::Shapes shapes;
  db::ShapeIterator s = shapes.begin (db::ShapeIterator::All);
  EXPECT_EQ (s.quad_id (), size_t (0));
  EXPECT_EQ (s.quad_box ().to_string (), db::Box::world ().to_string ());

  s = shapes.begin_touching (db::Box (-500, -500, 500, 500), db::ShapeIterator::All);
  EXPECT_EQ (s.quad_id (), size_t (0));
  EXPECT_EQ (s.quad_box ().to_string (), "()");

  for (int i = 0; i < 200; ++i) {
    shapes.insert (db::Box (-200, -200, -110, -110));
    shapes.insert (db::Box (200, -200, 110, -110));
    shapes.insert (db::Box (-200, 200, -110, 110));
    shapes.insert (db::Box (200, 200, 110, 110));
  }

  s = shapes.begin_touching (db::Box (-500, -500, 500, 500), db::ShapeIterator::All);
  size_t qid = s.quad_id ();
  EXPECT_EQ (qid != 0, true);
  EXPECT_EQ (s.quad_box ().to_string (), "(100,100;200,200)");
  EXPECT_EQ (s->to_string (), "box (110,110;200,200)");
  ++s;
  EXPECT_EQ (qid == s.quad_id (), true);
  EXPECT_EQ (s.quad_box ().to_string (), "(100,100;200,200)");
  EXPECT_EQ (s->to_string (), "box (110,110;200,200)");
  s.skip_quad ();
  EXPECT_EQ (qid != s.quad_id (), true);
  EXPECT_EQ (s.quad_box ().to_string (), "(-200,100;-100,200)");
  EXPECT_EQ (s->to_string (), "box (-200,110;-110,200)");
  s.skip_quad ();
  EXPECT_EQ (qid != s.quad_id (), true);
  EXPECT_EQ (s.quad_box ().to_string (), "(-200,-200;-100,-100)");
  EXPECT_EQ (s->to_string (), "box (-200,-200;-110,-110)");
  s.skip_quad ();
  EXPECT_EQ (qid != s.quad_id (), true);
  EXPECT_EQ (s.quad_box ().to_string (), "(100,-200;200,-100)");
  EXPECT_EQ (s->to_string (), "box (110,-200;200,-110)");
  s.skip_quad ();
  EXPECT_EQ (s.at_end (), true);
}

TEST(21)
{
  db::Shapes shapes;
  db::ShapeIterator s = shapes.begin (db::ShapeIterator::All);
  EXPECT_EQ (s.quad_id (), size_t (0));
  EXPECT_EQ (s.quad_box ().to_string (), db::Box::world ().to_string ());

  s = shapes.begin_touching (db::Box (-500, -500, 500, 500), db::ShapeIterator::All);
  EXPECT_EQ (s.quad_id (), size_t (0));
  EXPECT_EQ (s.quad_box ().to_string (), "()");

  for (int i = 0; i < 50; ++i) {
    shapes.insert (db::Box (200, -200, 100, -100));
    shapes.insert (db::Box (-200, 200, -100, 100));
    shapes.insert (db::Box (200, 200, 100, 100));
  }

  s = shapes.begin_touching (db::Box (-500, -500, 500, 500), db::ShapeIterator::All);
  size_t qid = s.quad_id ();
  EXPECT_EQ (qid != 0, true);
#if defined(HAVE_64BIT_COORD)
  EXPECT_EQ (s.quad_box ().to_string (), "(0,0;9007199254740992,9007199254740992)");
#else
  EXPECT_EQ (s.quad_box ().to_string (), "(0,0;2147483647,2147483647)");
#endif
  EXPECT_EQ (s->to_string (), "box (100,100;200,200)");
  ++s;
  EXPECT_EQ (qid == s.quad_id (), true);
#if defined(HAVE_64BIT_COORD)
  EXPECT_EQ (s.quad_box ().to_string (), "(0,0;9007199254740992,9007199254740992)");
#else
  EXPECT_EQ (s.quad_box ().to_string (), "(0,0;2147483647,2147483647)");
#endif
  EXPECT_EQ (s->to_string (), "box (100,100;200,200)");
  s.skip_quad ();
  EXPECT_EQ (qid != s.quad_id (), true);
#if defined(HAVE_64BIT_COORD)
  EXPECT_EQ (s.quad_box ().to_string (), "(-9007199254740992,0;0,9007199254740992)");
#else
  EXPECT_EQ (s.quad_box ().to_string (), "(-2147483648,0;0,2147483647)");
#endif
  EXPECT_EQ (s->to_string (), "box (-200,100;-100,200)");
  s.skip_quad ();
  EXPECT_EQ (qid != s.quad_id (), true);
#if defined(HAVE_64BIT_COORD)
  EXPECT_EQ (s.quad_box ().to_string (), "(0,-9007199254740992;9007199254740992,0)");
#else
  EXPECT_EQ (s.quad_box ().to_string (), "(0,-2147483648;2147483647,0)");
#endif
  EXPECT_EQ (s->to_string (), "box (100,-200;200,-100)");
  s.skip_quad ();
  EXPECT_EQ (s.at_end (), true);
}

TEST(22)
{
  db::Shapes shapes1;
  shapes1.insert (db::Box (200, -200, 100, -100));
  shapes1.insert (db::Box (-200, 200, -100, 100));
  shapes1.insert (db::Box (200, 200, 100, 100));

  db::Shapes shapes2;
  shapes2.insert (db::Box (200, -200, 100, -100));
  shapes2.insert (db::Box (-210, 200, -100, 100));

  db::Shapes shapes;
  shapes = shapes1;

  db::Shapes::shape_iterator s = shapes1.begin (db::ShapeIterator::All);
  EXPECT_EQ (shapes.find (*s).to_string (), s->to_string ());
  ++s;
  EXPECT_EQ (shapes.find (*s).to_string (), s->to_string ());
  ++s;
  EXPECT_EQ (shapes.find (*s).to_string (), s->to_string ());

  s = shapes2.begin (db::ShapeIterator::All);
  EXPECT_EQ (shapes.find (*s).to_string (), s->to_string ());
  ++s;
  EXPECT_EQ (shapes.find (*s).to_string (), "null");
}

//  Edge pairs
TEST(23)
{
  db::Manager m (true);
  db::Shapes s (&m, 0, db::default_editable_mode ());
  db::Box b_empty;

  EXPECT_EQ (s.bbox (), b_empty);

  db::EdgePair ep (db::Edge (-100, -200, 0, 0), db::Edge (0, -100, 100, 100));
  s.insert (ep);
  EXPECT_EQ (s.bbox (), db::Box (-100, -200, 100, 100));

  db::ShapeIterator si = s.begin (db::ShapeIterator::EdgePairs);
  EXPECT_EQ (!si.at_end (), true);
  EXPECT_EQ (si->edge_pair ().to_string (), "(-100,-200;0,0)/(0,-100;100,100)");
  EXPECT_EQ (si->is_edge_pair (), true);

  db::EdgePair ep2;
  si->instantiate (ep2);
  EXPECT_EQ (ep2.to_string (), "(-100,-200;0,0)/(0,-100;100,100)");

  ++si;
  EXPECT_EQ (si.at_end (), true);

  db::Shapes s2 = s;
  EXPECT_EQ (shapes_to_string_norm (_this, s2), "edge_pair (-100,-200;0,0)/(0,-100;100,100) #0\n");

  s2.clear ();
  s2.insert (db::EdgePairWithProperties (db::EdgePair (db::Edge (0, 0, 1, 1), db::Edge (10, 10, 11, 11)), 17));

  EXPECT_EQ (shapes_to_string_norm (_this, s2), "edge_pair (0,0;1,1)/(10,10;11,11) #17\n");
}

//  Shape insert and clear and undo/redo
TEST(24a)
{
  db::Manager m;
  db::Shapes s1 (&m, 0, true), s2;

  s2.insert (db::Edge (db::Point (0, 0), db::Point (100, 200)));
  s2.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));

  m.transaction ("test");
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "");
  s1.insert (s2);
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nedge (0,0;100,200) #0\n");
  m.commit ();

  m.undo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "");

  m.redo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nedge (0,0;100,200) #0\n");

  m.undo ();
  s1.insert (db::Box (db::Point (1, 1), db::Point (101, 201)));
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (1,1;101,201) #0\n");

  m.clear ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (1,1;101,201) #0\n");

  m.transaction ("test");
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (1,1;101,201) #0\n");
  s1.insert (s2);
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nbox (1,1;101,201) #0\nedge (0,0;100,200) #0\n");
  m.commit ();

  m.undo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (1,1;101,201) #0\n");

  m.redo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nbox (1,1;101,201) #0\nedge (0,0;100,200) #0\n");

  m.clear ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nbox (1,1;101,201) #0\nedge (0,0;100,200) #0\n");

  m.transaction ("test");
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nbox (1,1;101,201) #0\nedge (0,0;100,200) #0\n");
  s1.clear ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "");
  m.commit ();

  m.undo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nbox (1,1;101,201) #0\nedge (0,0;100,200) #0\n");

  m.redo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "");

  m.clear ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "");
}

//  Shape insert and clear and undo/redo - different layers, same layout
TEST(24b)
{
  db::Manager m;
  db::Layout l (true, &m);
  db::Cell &cell = l.cell (l.add_cell ("top"));
  l.insert_layer (1);
  l.insert_layer (2);
  db::Shapes &s1 = cell.shapes (1);
  db::Shapes &s2 = cell.shapes (2);

  s2.insert (db::Edge (db::Point (0, 0), db::Point (100, 200)));
  s2.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));

  m.transaction ("test");
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "");
  s1.insert (s2);
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nedge (0,0;100,200) #0\n");
  m.commit ();

  m.undo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "");

  m.redo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nedge (0,0;100,200) #0\n");

  m.undo ();
  s1.insert (db::Box (db::Point (1, 1), db::Point (101, 201)));
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (1,1;101,201) #0\n");

  m.clear ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (1,1;101,201) #0\n");

  m.transaction ("test");
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (1,1;101,201) #0\n");
  s1.insert (s2);
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nbox (1,1;101,201) #0\nedge (0,0;100,200) #0\n");
  m.commit ();

  m.undo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (1,1;101,201) #0\n");

  m.redo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nbox (1,1;101,201) #0\nedge (0,0;100,200) #0\n");

  m.clear ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nbox (1,1;101,201) #0\nedge (0,0;100,200) #0\n");

  m.transaction ("test");
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nbox (1,1;101,201) #0\nedge (0,0;100,200) #0\n");
  s1.clear ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "");
  m.commit ();

  m.undo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nbox (1,1;101,201) #0\nedge (0,0;100,200) #0\n");

  m.redo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "");

  m.clear ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "");
}

//  Shape insert and clear and undo/redo - no layout on target
TEST(24c)
{
  db::Manager m;
  db::Layout l;
  db::Cell &cell = l.cell (l.add_cell ("top"));
  l.insert_layer (1);
  l.insert_layer (2);
  db::Shapes s1 (&m, 0, true);
  db::Shapes &s2 = cell.shapes (2);

  s2.insert (db::Edge (db::Point (0, 0), db::Point (100, 200)));
  s2.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));

  m.transaction ("test");
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "");
  s1.insert (s2);
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nedge (0,0;100,200) #0\n");
  m.commit ();

  m.undo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "");

  m.redo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nedge (0,0;100,200) #0\n");

  m.undo ();
  s1.insert (db::Box (db::Point (1, 1), db::Point (101, 201)));
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (1,1;101,201) #0\n");

  m.clear ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (1,1;101,201) #0\n");

  m.transaction ("test");
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (1,1;101,201) #0\n");
  s1.insert (s2);
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nbox (1,1;101,201) #0\nedge (0,0;100,200) #0\n");
  m.commit ();

  m.undo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (1,1;101,201) #0\n");

  m.redo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nbox (1,1;101,201) #0\nedge (0,0;100,200) #0\n");

  m.clear ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nbox (1,1;101,201) #0\nedge (0,0;100,200) #0\n");

  m.transaction ("test");
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nbox (1,1;101,201) #0\nedge (0,0;100,200) #0\n");
  s1.clear ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "");
  m.commit ();

  m.undo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nbox (1,1;101,201) #0\nedge (0,0;100,200) #0\n");

  m.redo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "");

  m.clear ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "");
}

//  Shape insert and clear and undo/redo - different layouts
TEST(24d)
{
  db::Manager m;
  db::Layout l1 (true, &m);
  db::Cell &cell1 = l1.cell (l1.add_cell ("top"));
  l1.insert_layer (1);
  db::Layout l2 (true, &m);
  db::Cell &cell2 = l2.cell (l2.add_cell ("top"));
  l2.insert_layer (2);
  db::Shapes &s1 = cell1.shapes (1);
  db::Shapes &s2 = cell2.shapes (2);

  s2.insert (db::Edge (db::Point (0, 0), db::Point (100, 200)));
  s2.insert (db::Box (db::Point (0, 0), db::Point (100, 200)));

  m.transaction ("test");
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "");
  s1.insert (s2);
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nedge (0,0;100,200) #0\n");
  m.commit ();

  m.undo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "");

  m.redo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nedge (0,0;100,200) #0\n");

  m.undo ();
  s1.insert (db::Box (db::Point (1, 1), db::Point (101, 201)));
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (1,1;101,201) #0\n");

  m.clear ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (1,1;101,201) #0\n");

  m.transaction ("test");
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (1,1;101,201) #0\n");
  s1.insert (s2);
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nbox (1,1;101,201) #0\nedge (0,0;100,200) #0\n");
  m.commit ();

  m.undo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (1,1;101,201) #0\n");

  m.redo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nbox (1,1;101,201) #0\nedge (0,0;100,200) #0\n");

  m.clear ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nbox (1,1;101,201) #0\nedge (0,0;100,200) #0\n");

  m.transaction ("test");
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nbox (1,1;101,201) #0\nedge (0,0;100,200) #0\n");
  s1.clear ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "");
  m.commit ();

  m.undo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "box (0,0;100,200) #0\nbox (1,1;101,201) #0\nedge (0,0;100,200) #0\n");

  m.redo ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "");

  m.clear ();
  EXPECT_EQ (shapes_to_string_norm (_this, s1), "");
}

//  Bug #107
TEST(100)
{
  db::Manager m (true);
  db::Shapes shapes1 (&m, 0, true);

  m.transaction ("y");
  shapes1.insert (db::Box (200, -200, 100, -100));
  m.commit ();

  EXPECT_EQ (shapes_to_string_norm (_this, shapes1),
    "box (100,-200;200,-100) #0\n"
  );
  m.undo ();
  EXPECT_EQ (shapes_to_string_norm (_this, shapes1),
    ""
  );
  m.redo ();
  EXPECT_EQ (shapes_to_string_norm (_this, shapes1),
    "box (100,-200;200,-100) #0\n"
  );
  m.undo ();
  EXPECT_EQ (shapes_to_string_norm (_this, shapes1),
    ""
  );
}

//  Bug #835
TEST(101)
{
  db::Layout a, b;

  unsigned int la = a.insert_layer ();
  db::cell_index_type topa = a.add_cell ("TOP");
  db::Shapes &sa = a.cell (topa).shapes (la);

  unsigned int lb = b.insert_layer ();
  db::cell_index_type topb = b.add_cell ("TOP");
  db::Shapes &sb = b.cell (topb).shapes (lb);

  db::TextRef tr (db::Text ("TEXT", db::Trans ()), a.shape_repository ());

  db::PolygonRef pr (db::Polygon (db::Box (0, 0, 100, 200)), a.shape_repository ());

  db::Point pp[] = { db::Point (0, 0), db::Point (100, 200) };
  db::PathRef qr (db::Path (&pp[0], &pp[0] + 2, 20), a.shape_repository ());

  db::Shape st = sa.insert (tr);
  db::Shape sp = sa.insert (pr);
  db::Shape sq = sa.insert (qr);

  //  text sits in "a" shape repo now.
  db::TextRef tr1 = st.text_ref ();
  const db::Text &tr1_obj = *a.shape_repository ().repository (db::Text::tag ()).begin ();
  EXPECT_EQ (& tr1.obj () == &tr1_obj, true);

  //  polygon sits in "a" shape repo now.
  db::PolygonRef pr1 = sp.polygon_ref ();
  const db::Polygon &pr1_obj = *a.shape_repository ().repository (db::Polygon::tag ()).begin ();
  EXPECT_EQ (& pr1.obj () == &pr1_obj, true);

  //  path sits in "a" shape repo now.
  db::PathRef qr1 = sq.path_ref ();
  const db::Path &qr1_obj = *a.shape_repository ().repository (db::Path::tag ()).begin ();
  EXPECT_EQ (& qr1.obj () == &qr1_obj, true);

  //  Now insert into sb

  db::Shape st2 = sb.insert (st);
  db::Shape sp2 = sb.insert (sp);
  db::Shape sq2 = sb.insert (sq);

  //  text sits in "b" shape repo now.
  db::TextRef tr2 = st2.text_ref ();
  const db::Text &tr2_obj = *b.shape_repository ().repository (db::Text::tag ()).begin ();
  EXPECT_EQ (& tr2.obj () == &tr2_obj, true);

  //  polygon sits in "b" shape repo now.
  db::PolygonRef pr2 = sp2.polygon_ref ();
  const db::Polygon &pr2_obj = *b.shape_repository ().repository (db::Polygon::tag ()).begin ();
  EXPECT_EQ (& pr2.obj () == &pr2_obj, true);

  //  path sits in "b" shape repo now.
  db::PathRef qr2 = sq2.path_ref ();
  const db::Path &qr2_obj = *b.shape_repository ().repository (db::Path::tag ()).begin ();
  EXPECT_EQ (& qr2.obj () == &qr2_obj, true);
}

