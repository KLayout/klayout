
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


#include "dbShapeRepository.h"
#include "dbShapes.h"
#include "dbLayout.h"
#include "dbShape.h"
#include "dbManager.h"
#include "dbPolygon.h"
#include "dbPath.h"
#include "dbText.h"
#include "dbEdge.h"
#include "dbUserObject.h"
#include "tlUnitTest.h"


TEST(1) 
{
  db::GenericRepository rep;

  db::Polygon p1;
  std::vector <db::Point> c1;
  c1.push_back (db::Point (100, 0));
  c1.push_back (db::Point (100, 1000));
  c1.push_back (db::Point (200, 1000));
  c1.push_back (db::Point (200, 0));
  p1.assign_hull (c1.begin (), c1.end ());

  //  insert two references to identical but shifted polygons
  db::PolygonRef pref1 (p1, rep);

  db::Polygon p2;
  p2 = p1;
  p2.move (db::Vector (-100, 100));

  db::PolygonRef pref2 (p2, rep);

  EXPECT_EQ (rep.repository (db::Polygon::tag ()).size (), size_t (1));

  EXPECT_EQ (pref1.trans (), db::Disp (db::Vector (100, 0)));
  EXPECT_EQ (pref1.instantiate (), p1); 
  EXPECT_EQ (pref2.trans (), db::Disp (db::Vector (0, 100)));
  EXPECT_EQ (pref2.instantiate (), p2); 

  //  transform the polygon reference and original
  db::Disp t (db::Vector (1234, -789));

  pref1 = t * pref1;
  pref2.transform (t);

  p1 = p1.transformed (t);
  p2 = t * p2;

  EXPECT_EQ (pref1.instantiate (), p1); 
  EXPECT_EQ (pref2.instantiate (), p2); 

  //  Test, if the edge iterators deliver the right sequence
  db::PolygonRef::polygon_edge_iterator e1;
  db::Polygon::polygon_edge_iterator e2;
  std::set <db::Edge> s1, s2;

  e1 = pref1.begin_edge ();
  e2 = p1.begin_edge ();

  while (! e1.at_end () && ! e2.at_end  ()) {
    s1.insert (*e1);
    s2.insert (*e2);
    ++e1; ++e2;
  }

  EXPECT_EQ (s1 == s2, true);

  e1 = pref2.begin_edge ();
  e2 = p2.begin_edge ();
  s1.clear (); s2.clear ();

  while (! e1.at_end () && ! e2.at_end ()) {
    s1.insert (*e1);
    s2.insert (*e2);
    ++e1; ++e2;
  }

  EXPECT_EQ (s1 == s2, true);
}

TEST(2) 
{
  db::GenericRepository *rep = new db::GenericRepository ();

  db::Polygon p1;
  std::vector <db::Point> c1;
  c1.push_back (db::Point (100, 0));
  c1.push_back (db::Point (100, 1000));
  c1.push_back (db::Point (200, 1000));
  c1.push_back (db::Point (200, 0));
  p1.assign_hull (c1.begin (), c1.end ());

  //  insert two references to identical but shifted polygons
  db::PolygonRef pr1 (p1, *rep);

  db::Polygon p2;
  p2 = p1;
  p2.move (db::Vector (-100, 100));

  db::PolygonRef pr2 (p2, *rep);

  EXPECT_EQ (rep->repository (db::Polygon::tag ()).size (), size_t (1));

  //  copy everything into a new repository
  db::GenericRepository rep2;

  db::PolygonRef pref1 (pr1, rep2);
  db::PolygonRef pref2 (pr2, rep2);

  delete rep;
  rep = 0;

  EXPECT_EQ (rep2.repository (db::Polygon::tag ()).size (), size_t (1));

  EXPECT_EQ (pref1.trans (), db::Disp (db::Vector (100, 0)));
  EXPECT_EQ (pref1.instantiate (), p1); 
  EXPECT_EQ (pref2.trans (), db::Disp (db::Vector (0, 100)));
  EXPECT_EQ (pref2.instantiate (), p2); 
}


TEST(1SIMPLE) 
{
  db::GenericRepository rep;

  db::SimplePolygon p1;
  std::vector <db::Point> c1;
  c1.push_back (db::Point (100, 0));
  c1.push_back (db::Point (100, 1000));
  c1.push_back (db::Point (200, 1000));
  c1.push_back (db::Point (200, 0));
  p1.assign_hull (c1.begin (), c1.end ());

  //  insert two references to identical but shifted polygons
  db::SimplePolygonRef pref1 (p1, rep);

  db::SimplePolygon p2;
  p2 = p1;
  p2.move (db::Vector (-100, 100));

  db::SimplePolygonRef pref2 (p2, rep);

  EXPECT_EQ (rep.repository (db::SimplePolygon::tag ()).size (), size_t (1));

  EXPECT_EQ (pref1.trans (), db::Disp (db::Vector (100, 0)));
  EXPECT_EQ (pref1.instantiate (), p1); 
  EXPECT_EQ (pref2.trans (), db::Disp (db::Vector (0, 100)));
  EXPECT_EQ (pref2.instantiate (), p2); 

  //  transform the polygon reference and original
  db::Disp t (db::Vector (1234, -789));

  pref1 = t * pref1;
  pref2.transform (t);

  p1 = p1.transformed (t);
  p2 = t * p2;

  EXPECT_EQ (pref1.instantiate (), p1); 
  EXPECT_EQ (pref2.instantiate (), p2); 

  //  Test, if the edge iterators deliver the right sequence
  db::SimplePolygonRef::polygon_edge_iterator e1;
  db::SimplePolygon::polygon_edge_iterator e2;
  std::set <db::Edge> s1, s2;

  e1 = pref1.begin_edge ();
  e2 = p1.begin_edge ();

  while (! e1.at_end () && ! e2.at_end ()) {
    s1.insert (*e1);
    s2.insert (*e2);
    ++e1; ++e2;
  }

  EXPECT_EQ (s1 == s2, true);

  e1 = pref2.begin_edge ();
  e2 = p2.begin_edge ();
  s1.clear (); s2.clear ();

  while (! e1.at_end () && ! e2.at_end ()) {
    s1.insert (*e1);
    s2.insert (*e2);
    ++e1; ++e2;
  }

  EXPECT_EQ (s1 == s2, true);
}

TEST(2SIMPLE) 
{
  db::GenericRepository *rep = new db::GenericRepository ();

  db::SimplePolygon p1;
  std::vector <db::Point> c1;
  c1.push_back (db::Point (100, 0));
  c1.push_back (db::Point (100, 1000));
  c1.push_back (db::Point (200, 1000));
  c1.push_back (db::Point (200, 0));
  p1.assign_hull (c1.begin (), c1.end ());

  //  insert two references to identical but shifted polygons
  db::SimplePolygonRef pr1 (p1, *rep);

  db::SimplePolygon p2;
  p2 = p1;
  p2.move (db::Vector (-100, 100));

  db::SimplePolygonRef pr2 (p2, *rep);

  EXPECT_EQ (rep->repository (db::SimplePolygon::tag ()).size (), size_t (1));

  //  copy everything into a new repository
  db::GenericRepository rep2;

  db::SimplePolygonRef pref1 (pr1, rep2);
  db::SimplePolygonRef pref2 (pr2, rep2);

  delete rep;
  rep = 0;

  EXPECT_EQ (rep2.repository (db::SimplePolygon::tag ()).size (), size_t (1));

  EXPECT_EQ (pref1.trans (), db::Disp (db::Vector (100, 0)));
  EXPECT_EQ (pref1.instantiate (), p1); 
  EXPECT_EQ (pref2.trans (), db::Disp (db::Vector (0, 100)));
  EXPECT_EQ (pref2.instantiate (), p2); 
}

TEST(3) 
{
  db::GenericRepository *rep = new db::GenericRepository ();
  db::Manager m (true);

  std::vector <db::Point> c1;
  c1.push_back (db::Point (100, 0));
  c1.push_back (db::Point (100, 1000));
  c1.push_back (db::Point (200, 1000));
  c1.push_back (db::Point (200, 0));

  db::Polygon p1;
  p1.assign_hull (c1.begin (), c1.end ());

  db::SimplePolygon p2;
  p2.assign_hull (c1.begin (), c1.end ());

  db::Shapes shapes (db::default_editable_mode ());
  shapes.insert (p1);
  shapes.insert (db::PolygonRef (p1, *rep));
  shapes.insert (p2);
  shapes.insert (db::SimplePolygonRef (p2, *rep));

  EXPECT_EQ (rep->repository (db::SimplePolygon::tag ()).size (), size_t (1));

  db::Shapes::shape_iterator s = shapes.begin (db::ShapeIterator::All);
  unsigned int n = 0;
  while (! s.at_end ()) {
    
    db::Polygon px;
    s->polygon (px);

    EXPECT_EQ (px, p1);
    
    ++n; 

    db::Shapes::polygon_edge_iterator pe = s->begin_edge ();
    db::Polygon::polygon_edge_iterator pe2 = p1.begin_edge ();
    while (! pe.at_end () && ! pe2.at_end ()) {
      EXPECT_EQ (*pe, *pe2);
      ++pe;
      ++pe2;
    }

    EXPECT_EQ (pe.at_end (), true);
    EXPECT_EQ (pe2.at_end (), true);

    ++s;

  }

  EXPECT_EQ (n, (unsigned int) 4);

  db::Layout rep2;
  db::Cell &rep2_cell = rep2.cell (rep2.add_cell ());

  //  create a new repository and shapes list
  db::Shapes shapes2 (0, &rep2_cell, db::default_editable_mode ());
  shapes2 = shapes;

  delete rep;
  rep = 0;

  EXPECT_EQ (rep2.shape_repository ().repository (db::SimplePolygon::tag ()).size (), size_t (1));

  s = shapes2.begin (db::ShapeIterator::All);
  n = 0;
  while (! s.at_end ()) {
    db::Polygon px;
    s->polygon (px);
    EXPECT_EQ (px, p1);
    ++s;
    ++n;
  }

  EXPECT_EQ (n, size_t (4));

}

TEST(4) 
{
  db::GenericRepository rep;

  std::vector <db::Point> c1;
  c1.push_back (db::Point (100, 0));
  c1.push_back (db::Point (100, 1000));
  c1.push_back (db::Point (200, 1000));
  c1.push_back (db::Point (200, 0));

  db::Polygon p1;
  p1.assign_hull (c1.begin (), c1.end ());

  db::SimplePolygon p2;
  p2.assign_hull (c1.begin (), c1.end ());

  db::Path pt;
  pt.assign (c1.begin (), c1.end ());
  pt.width (21);

  db::Text tt ("Text", db::Trans (5, db::Vector (100, 200)), 15);

  db::Shapes shapes (db::default_editable_mode ());
  shapes.insert (p1);
  shapes.insert (db::PolygonRef (p1, rep));
  shapes.insert (p2);
  shapes.insert (db::SimplePolygonRef (p2, rep));
  shapes.insert (pt);
  shapes.insert (db::PathRef (pt, rep));
  shapes.insert (tt);
  shapes.insert (db::TextRef (tt, rep));

  EXPECT_EQ (rep.repository (db::SimplePolygon::tag ()).size (), size_t (1));
  EXPECT_EQ (rep.repository (db::Polygon::tag ()).size (), size_t (1));
  EXPECT_EQ (rep.repository (db::Path::tag ()).size (), size_t (1));
  EXPECT_EQ (rep.repository (db::Text::tag ()).size (), size_t (1));

  db::Shapes::shape_iterator s = shapes.begin (db::ShapeIterator::All);
  unsigned int n = 0;
  while (! s.at_end ()) {

    if (s->is_text ()) {
      db::Text t;
      s->text (t);
      EXPECT_EQ (t, tt);
      EXPECT_EQ (std::string (s->text_string ()), "Text");
    } else if (s->is_path ()) {
      db::Path pp;
      s->path (pp);
      EXPECT_EQ (pp, pt);
      std::vector<db::Point>::const_iterator r = c1.begin ();
      for (db::Shape::point_iterator pt = s->begin_point (); pt != s->end_point (); ++pt) {
        EXPECT_EQ (*pt, *r);
        ++r;
      }
      EXPECT_EQ (r == c1.end (), true);
      EXPECT_EQ (s->path_width (), 21);
    } else {
      EXPECT_EQ (s->holes (), size_t (0));
      std::vector<db::Point>::const_iterator r = c1.begin ();
      for (db::Shape::point_iterator pt = s->begin_hull (); pt != s->end_hull (); ++pt) {
        EXPECT_EQ (*pt, *r);
        ++r;
      }
      EXPECT_EQ (r == c1.end (), true);
    }

    ++n; 
    ++s;

  }

  EXPECT_EQ (n, size_t (8));

}

