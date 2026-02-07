
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



#include "tlUnitTest.h"
#include "layAnnotationShapes.h"
#include "dbBox.h"
#include "dbEdge.h"
#include "dbUserObject.h"
#include "dbBoxConvert.h"

template <class Sh>
class shape_as_user_object
  : public db::DUserObjectBase
{
public:
  shape_as_user_object<Sh> (const Sh &sh)
    : m_shape (sh)
  { }

  virtual bool equals (const db::DUserObjectBase *d) const 
  {
    return m_shape == (dynamic_cast<const shape_as_user_object<Sh> *> (d))->m_shape;
  }

  virtual bool less (const db::DUserObjectBase *d) const 
  {
    return m_shape < (dynamic_cast<const shape_as_user_object<Sh> *> (d))->m_shape;
  }

  virtual unsigned int class_id () const 
  {
    return (unsigned int) (size_t) &tag_func;
  }

  virtual db::DUserObjectBase *clone () const 
  {
    return new shape_as_user_object<Sh> (m_shape);
  }

  virtual db::DBox box () const 
  {
    db::box_convert<Sh> bc;
    return db::DBox (bc (m_shape));
  }

  virtual void transform (const db::simple_trans<db::DCoord> &t) 
  { 
    m_shape.transform (db::simple_trans<typename Sh::coord_type> (t));
  }

  virtual void transform (const db::DFTrans &t) 
  { 
    m_shape.transform (t);
  }

  virtual void transform (const db::complex_trans<db::DCoord, db::DCoord> &)
  { 
    tl_assert (false);
  }

  const Sh &shape () const { return m_shape; }

  virtual void mem_stat (db::MemStatistics *stat, db::MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
  {
    if (! no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }
    db::mem_stat (stat, purpose, cat, m_shape, true, (void *) this);
  }

private:
  Sh m_shape;

  static void tag_func () { }
};

template <class Sh>
db::DUserObject us (const Sh &sh)
{
  return db::DUserObject (new shape_as_user_object<Sh> (sh));
}


TEST(1) 
{
  db::Manager m (true);
  lay::AnnotationShapes s (&m);
  db::DBox b_empty;

  s.update_bbox ();
  EXPECT_EQ (s.bbox (), b_empty);

  db::DBox b (0, 100, 1000, 1200);
  s.insert (us (b));
  s.update_bbox ();
  EXPECT_EQ (s.bbox (), b);

  db::DEdge e (-100, -200, 0, 0);
  s.insert (us (e));
  s.update_bbox ();
  EXPECT_EQ (s.bbox (), db::DBox (-100, -200, 1000, 1200));
  
  lay::AnnotationShapes s2 (s);
  s2.update_bbox ();
  EXPECT_EQ (s2.bbox (), db::DBox (-100, -200, 1000, 1200));

  s2.erase (s2.begin ());
  s2.update_bbox ();
  EXPECT_EQ (s2.bbox (), db::DBox (-100, -200, 0, 0));
}

std::string shapes_to_string (const lay::AnnotationShapes &shapes)
{
  std::string r;
  for (lay::AnnotationShapes::iterator shape = shapes.begin (); shape != shapes.end (); ++shape) {
    if (dynamic_cast<const shape_as_user_object<db::DPolygon> *> (shape->ptr ())) {
      db::DPolygon p (dynamic_cast<const shape_as_user_object<db::DPolygon> *> (shape->ptr ())->shape ());
      r += "polygon " + p.to_string ();
    } else if (dynamic_cast<const shape_as_user_object<db::DPath> *> (shape->ptr ())) {
      db::DPath p (dynamic_cast<const shape_as_user_object<db::DPath> *> (shape->ptr ())->shape ());
      r += "path " + p.to_string ();
    } else if (dynamic_cast<const shape_as_user_object<db::DText> *> (shape->ptr ())) {
      db::DText p (dynamic_cast<const shape_as_user_object<db::DText> *> (shape->ptr ())->shape ());
      r += "text " + p.to_string ();
    } else if (dynamic_cast<const shape_as_user_object<db::DBox> *> (shape->ptr ())) {
      db::DBox p (dynamic_cast<const shape_as_user_object<db::DBox> *> (shape->ptr ())->shape ());
      r += "box " + p.to_string ();
    } else {
      r += "*unknown type*";
    }
    r += "\n";
  }
  return r;
}

void read_testdata (lay::AnnotationShapes &shapes, unsigned int what = 0xff)
{
  if ((what & 0x1) != 0) {
    static db::DPolygon p1 (db::DBox (0, 100, 1000, 2000));
    static db::DPolygon p2 (db::DBox (100, 200, 1100, 2100));
    static db::DPolygon p3 (db::DBox (150, 150, 1150, 2050));

    shapes.insert (us (p1));
    shapes.insert (us (p2));
    shapes.insert (us (p3));
  }

  if ((what & 0x4) != 0) {
    static db::DPath r1;
    static db::DPath r2;
    static db::DPath r3;
    db::DPoint pts1 [] = { db::DPoint (0, 100), db::DPoint (0, 500), db::DPoint (200, 700) };
    db::DPoint pts2 [] = { db::DPoint (0, 1100), db::DPoint (0, 1500), db::DPoint (200, 1300) };
    db::DPoint pts3 [] = { db::DPoint (0, 2100), db::DPoint (0, 2500), db::DPoint (-200, 2700) };
    r1 = db::DPath (pts1, pts1 + 3, 100);
    r2 = db::DPath (pts2, pts2 + 3, 150);
    r3 = db::DPath (pts3, pts3 + 3, 200);

    shapes.insert (us (r1));
    shapes.insert (us (r2));
    shapes.insert (us (r3));
  }

  if ((what & 0x8) != 0) {
    static db::DText t1 ("A", db::DTrans (0, db::DVector (10, 35)));
    static db::DText t2 ("B", db::DTrans (1, db::DVector (20, 25)));
    static db::DText t3 ("C", db::DTrans (6, db::DVector (30, 15)));

    shapes.insert (us (t1));
    shapes.insert (us (t2));
    shapes.insert (us (t3));
  }

  if ((what & 0x10) != 0) {
    static db::DBox b1 (0, 100, 2000, 1000);
    static db::DBox b2 (100, 200, 2100, 1100);
    static db::DBox b3 (150, 150, 2150, 1050);

    shapes.insert (us (b1));
    shapes.insert (us (b2));
    shapes.insert (us (b3));
  }
}

TEST(2)
{
  db::Manager m (true);
  lay::AnnotationShapes shapes (&m);
  read_testdata (shapes, 0x1);

  lay::AnnotationShapes copy (&m);

  EXPECT_EQ (shapes_to_string (shapes), 
    "polygon (0,100;0,2000;1000,2000;1000,100)\n"
    "polygon (100,200;100,2100;1100,2100;1100,200)\n"
    "polygon (150,150;150,2050;1150,2050;1150,150)\n"
  );

  copy.clear ();
  for (lay::AnnotationShapes::iterator shape = shapes.begin (); shape != shapes.end (); ++shape) {
    copy.insert (*shape);
  }
  EXPECT_EQ (shapes_to_string (copy), 
    "polygon (0,100;0,2000;1000,2000;1000,100)\n"
    "polygon (100,200;100,2100;1100,2100;1100,200)\n"
    "polygon (150,150;150,2050;1150,2050;1150,150)\n"
  );

  shapes.erase (shapes.begin ());
  EXPECT_EQ (shapes_to_string (shapes), 
    "polygon (100,200;100,2100;1100,2100;1100,200)\n"
    "polygon (150,150;150,2050;1150,2050;1150,150)\n"
  );

  shapes.erase (shapes.begin ());
  EXPECT_EQ (shapes_to_string (shapes), 
    "polygon (150,150;150,2050;1150,2050;1150,150)\n"
  );

  shapes.erase (shapes.begin ());
  EXPECT_EQ (shapes_to_string (shapes), "");
}

TEST(3)
{
  db::Manager m (true);
  lay::AnnotationShapes shapes (&m);
  read_testdata (shapes, 0x4);

  lay::AnnotationShapes copy (&m);

  EXPECT_EQ (shapes_to_string (shapes), 
    "path (0,100;0,500;200,700) w=100 bx=0 ex=0 r=false\n"
    "path (0,1100;0,1500;200,1300) w=150 bx=0 ex=0 r=false\n"
    "path (0,2100;0,2500;-200,2700) w=200 bx=0 ex=0 r=false\n"
  );

  copy.clear ();
  for (lay::AnnotationShapes::iterator shape = shapes.begin (); shape != shapes.end (); ++shape) {
    copy.insert (*shape);
  }
  EXPECT_EQ (shapes_to_string (copy), 
    "path (0,100;0,500;200,700) w=100 bx=0 ex=0 r=false\n"
    "path (0,1100;0,1500;200,1300) w=150 bx=0 ex=0 r=false\n"
    "path (0,2100;0,2500;-200,2700) w=200 bx=0 ex=0 r=false\n"
  );

  shapes.erase (shapes.begin ());
  EXPECT_EQ (shapes_to_string (shapes), 
    "path (0,1100;0,1500;200,1300) w=150 bx=0 ex=0 r=false\n"
    "path (0,2100;0,2500;-200,2700) w=200 bx=0 ex=0 r=false\n"
  );

  shapes.erase (shapes.begin ());
  EXPECT_EQ (shapes_to_string (shapes), 
    "path (0,2100;0,2500;-200,2700) w=200 bx=0 ex=0 r=false\n"
  );

  shapes.erase (shapes.begin ());
  EXPECT_EQ (shapes_to_string (shapes), "");
}

TEST(4)
{
  db::Manager m (true);
  lay::AnnotationShapes shapes (&m);
  read_testdata (shapes, 0x4);

  lay::AnnotationShapes copy (&m);

  EXPECT_EQ (shapes_to_string (shapes), 
    "path (0,100;0,500;200,700) w=100 bx=0 ex=0 r=false\n"
    "path (0,1100;0,1500;200,1300) w=150 bx=0 ex=0 r=false\n"
    "path (0,2100;0,2500;-200,2700) w=200 bx=0 ex=0 r=false\n"
  );

  copy.clear ();
  for (lay::AnnotationShapes::iterator shape = shapes.begin (); shape != shapes.end (); ++shape) {
    m.transaction ("x");
    copy.insert (*shape);
    m.commit ();
  }
  EXPECT_EQ (shapes_to_string (copy), 
    "path (0,100;0,500;200,700) w=100 bx=0 ex=0 r=false\n"
    "path (0,1100;0,1500;200,1300) w=150 bx=0 ex=0 r=false\n"
    "path (0,2100;0,2500;-200,2700) w=200 bx=0 ex=0 r=false\n"
  );

  m.undo ();
  EXPECT_EQ (shapes_to_string (copy), 
    "path (0,100;0,500;200,700) w=100 bx=0 ex=0 r=false\n"
    "path (0,1100;0,1500;200,1300) w=150 bx=0 ex=0 r=false\n"
  );

  m.undo ();
  EXPECT_EQ (shapes_to_string (copy), 
    "path (0,100;0,500;200,700) w=100 bx=0 ex=0 r=false\n"
  );

  m.undo ();
  EXPECT_EQ (shapes_to_string (copy), "");

  m.redo ();
  m.redo ();
  m.redo ();
  EXPECT_EQ (shapes_to_string (copy), 
    "path (0,100;0,500;200,700) w=100 bx=0 ex=0 r=false\n"
    "path (0,1100;0,1500;200,1300) w=150 bx=0 ex=0 r=false\n"
    "path (0,2100;0,2500;-200,2700) w=200 bx=0 ex=0 r=false\n"
  );
}

TEST(5)
{
  db::Manager m (true);
  lay::AnnotationShapes shapes (&m);
  read_testdata (shapes, 0x4);

  lay::AnnotationShapes copy (&m);

  EXPECT_EQ (shapes_to_string (shapes), 
    "path (0,100;0,500;200,700) w=100 bx=0 ex=0 r=false\n"
    "path (0,1100;0,1500;200,1300) w=150 bx=0 ex=0 r=false\n"
    "path (0,2100;0,2500;-200,2700) w=200 bx=0 ex=0 r=false\n"
  );

  copy.clear ();
  m.transaction ("x");
  copy = shapes;
  m.commit ();

  EXPECT_EQ (shapes_to_string (copy), 
    "path (0,100;0,500;200,700) w=100 bx=0 ex=0 r=false\n"
    "path (0,1100;0,1500;200,1300) w=150 bx=0 ex=0 r=false\n"
    "path (0,2100;0,2500;-200,2700) w=200 bx=0 ex=0 r=false\n"
  );

  m.undo ();
  EXPECT_EQ (shapes_to_string (copy), "");

  m.redo ();
  EXPECT_EQ (shapes_to_string (copy), 
    "path (0,100;0,500;200,700) w=100 bx=0 ex=0 r=false\n"
    "path (0,1100;0,1500;200,1300) w=150 bx=0 ex=0 r=false\n"
    "path (0,2100;0,2500;-200,2700) w=200 bx=0 ex=0 r=false\n"
  );
}

TEST(6)
{
  db::Manager m (true);
  lay::AnnotationShapes shapes (&m);
  read_testdata (shapes, 0x10);

  EXPECT_EQ (shapes_to_string (shapes), 
    "box (0,100;2000,1000)\n"
    "box (100,200;2100,1100)\n"
    "box (150,150;2150,1050)\n"
  );

  shapes.update ();

  lay::AnnotationShapes copy (&m);

  copy.clear ();
  for (lay::AnnotationShapes::touching_iterator shape = shapes.begin_touching (db::DBox (0, 100, 100, 200)); ! shape.at_end (); ++shape) {
    copy.insert (*shape);
  }

  EXPECT_EQ (shapes_to_string (copy), 
    "box (0,100;2000,1000)\n"
    "box (100,200;2100,1100)\n"
  );
}

TEST(7)
{
  db::Manager m (true);
  lay::AnnotationShapes shapes (&m);
  read_testdata (shapes, 0x10);

  EXPECT_EQ (shapes_to_string (shapes), 
    "box (0,100;2000,1000)\n"
    "box (100,200;2100,1100)\n"
    "box (150,150;2150,1050)\n"
  );

  lay::AnnotationShapes copy (&m);

  shapes.update ();

  copy.clear ();
  for (lay::AnnotationShapes::overlapping_iterator shape = shapes.begin_overlapping (db::DBox (0, 100, 100, 200)); ! shape.at_end (); ++shape) {
    copy.insert (*shape);
  }

  EXPECT_EQ (shapes_to_string (copy), 
    "box (0,100;2000,1000)\n"
  );
}

