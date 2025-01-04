
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


#include "dbCellGraphUtils.h"
#include "dbMatrix.h"
#include "dbRecursiveShapeIterator.h"
#include "tlString.h"
#include "tlUnitTest.h"

TEST(1) 
{
  db::Manager m (true);
  db::Layout g (&m);
  db::Cell &c0 (g.cell (g.add_cell ()));
  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));
  db::Cell &c3 (g.cell (g.add_cell ()));
  db::Cell &c4 (g.cell (g.add_cell ()));

  db::Trans tt;
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), tt, db::Vector(), db::Vector(), 5, 2));
  c0.insert (db::CellInstArray (db::CellInst (c2.cell_index ()), tt));
  c4.insert (db::CellInstArray (db::CellInst (c2.cell_index ()), tt, db::Vector(), db::Vector(), 3, 4));
  c0.insert (db::CellInstArray (db::CellInst (c3.cell_index ()), tt));
  c2.insert (db::CellInstArray (db::CellInst (c3.cell_index ()), tt));
  c2.insert (db::CellInstArray (db::CellInst (c3.cell_index ()), tt));

  {
    db::CellCounter cc (&g);

    EXPECT_EQ (cc.weight (c0.cell_index ()), size_t (1));
    EXPECT_EQ (cc.weight (c1.cell_index ()), size_t (10));
    EXPECT_EQ (cc.weight (c2.cell_index ()), size_t (13));
    EXPECT_EQ (cc.weight (c3.cell_index ()), size_t (27));
    EXPECT_EQ (cc.weight (c4.cell_index ()), size_t (1));
  }

  {
    db::CellCounter cc (&g);

    EXPECT_EQ (cc.weight (c4.cell_index ()), size_t (1));
    EXPECT_EQ (cc.weight (c3.cell_index ()), size_t (27));
    EXPECT_EQ (cc.weight (c0.cell_index ()), size_t (1));
    EXPECT_EQ (cc.weight (c1.cell_index ()), size_t (10));
    EXPECT_EQ (cc.weight (c2.cell_index ()), size_t (13));
  }

  {
    db::CellCounter cc (&g);

    EXPECT_EQ (cc.weight (c4.cell_index ()), size_t (1));
    EXPECT_EQ (cc.weight (c3.cell_index ()), size_t (27));
    EXPECT_EQ (cc.weight (c2.cell_index ()), size_t (13));
    EXPECT_EQ (cc.weight (c0.cell_index ()), size_t (1));
    EXPECT_EQ (cc.weight (c1.cell_index ()), size_t (10));
  }

  {
    db::CellCounter cc (&g, c2.cell_index ());
    EXPECT_EQ (cc.weight (c3.cell_index ()), size_t (2));
    EXPECT_EQ (cc.weight (c2.cell_index ()), size_t (1));
    EXPECT_EQ (cc.weight (c4.cell_index ()), size_t (0));
    EXPECT_EQ (cc.weight (c0.cell_index ()), size_t (0));
    EXPECT_EQ (cc.weight (c1.cell_index ()), size_t (0));
  }

  {
    db::CellCounter cc (&g, c3.cell_index ());
    EXPECT_EQ (cc.weight (c3.cell_index ()), size_t (1));
    EXPECT_EQ (cc.weight (c4.cell_index ()), size_t (0));
    EXPECT_EQ (cc.weight (c0.cell_index ()), size_t (0));
    EXPECT_EQ (cc.weight (c1.cell_index ()), size_t (0));
    EXPECT_EQ (cc.weight (c2.cell_index ()), size_t (0));
  }

  {
    db::CellCounter cc (&g, c0.cell_index ());

    EXPECT_EQ (cc.weight (c4.cell_index ()), size_t (0));
    EXPECT_EQ (cc.weight (c3.cell_index ()), size_t (3));
    EXPECT_EQ (cc.weight (c2.cell_index ()), size_t (1));
    EXPECT_EQ (cc.weight (c0.cell_index ()), size_t (1));
    EXPECT_EQ (cc.weight (c1.cell_index ()), size_t (10));
  }

  {
    db::CellCounter cc (&g, c4.cell_index ());

    EXPECT_EQ (cc.weight (c4.cell_index ()), size_t (1));
    EXPECT_EQ (cc.weight (c3.cell_index ()), size_t (24));
    EXPECT_EQ (cc.weight (c2.cell_index ()), size_t (12));
    EXPECT_EQ (cc.weight (c0.cell_index ()), size_t (0));
    EXPECT_EQ (cc.weight (c1.cell_index ()), size_t (0));
  }
}

class InstanceReferenceSum
{
public:
  InstanceReferenceSum (const db::Layout &, const db::Cell &)
    : m_count (0)
  {
    // .. nothing yet ..
  }

  InstanceReferenceSum (size_t count, const db::Matrix2d &m, const db::DVector &p)
    : m_count (count), m_m (m), m_p (p)
  {
    // .. nothing yet ..
  }

  InstanceReferenceSum transformed (const db::CellInstArray &inst) const
  {
    db::Matrix2d m_res; 
    db::DVector p_res;

    m_res += db::Matrix2d (inst.complex_trans ()) * double (inst.size ());

    for (db::CellInstArray::iterator a = inst.begin (); ! a.at_end (); ++a) {
      p_res += db::DVector ((*a).disp ());
    }

    if (m_count == 0) {
      return InstanceReferenceSum (inst.size (), m_res, m_p + p_res);
    } else {
      return InstanceReferenceSum (m_count * inst.size (), m_m * m_res, m_p + m_m * p_res);
    }
  }

  void add (const InstanceReferenceSum &other)
  {
    m_count += other.m_count;
    m_p += db::DVector (other.m_p);
    m_m += other.m_m;
  }

  size_t n () const { return m_count; }
  const db::DVector &p () const { return m_p; }
  const db::Matrix2d &m () const { return m_m; }

private:
  size_t m_count;
  db::Matrix2d m_m;
  db::DVector m_p;
};

TEST(2) 
{
  for (int pass = 0; pass < 3; ++pass) {

    db::Layout g;
    db::Cell &a0 (g.cell (g.add_cell ("a0")));
    // db::Cell &a1 (g.cell (g.add_cell ("a1")));
    // db::Cell &a2 (g.cell (g.add_cell ("a2")));
    // db::Cell &a3 (g.cell (g.add_cell ("a3")));
    db::Cell &a4 (g.cell (g.add_cell ("a4")));
    g.insert_layer (0);
    a4.shapes (0).insert (db::Box (0, 0, 0, 0));

    if (pass == 0) {
      a0.insert (db::CellInstArray (db::CellInst (a4.cell_index ()), db::Trans (1/*r90*/), db::Vector(0, 10), db::Vector(10, 0), 5, 2));
    } else if (pass == 1) {
      a0.insert (db::CellInstArray (db::CellInst (a4.cell_index ()), db::ICplxTrans (1.0, 90.0, false, db::Vector(0, 0)), db::Vector(0, 10), db::Vector(10, 0), 5, 2));
    } else if (pass == 2) {
      a0.insert (db::CellInstArray (db::CellInst (a4.cell_index ()), db::ICplxTrans (0.1, 90.0, false, db::Vector(0, 0)), db::Vector(0, 10), db::Vector(10, 0), 5, 2));
    }

    db::Layout h;
    db::Cell &b0 (h.cell (h.add_cell ("b0")));
    db::Cell &b1 (h.cell (h.add_cell ("b1")));
    db::Cell &b2 (h.cell (h.add_cell ("b2")));
    db::Cell &b3 (h.cell (h.add_cell ("b3")));
    db::Cell &b4 (h.cell (h.add_cell ("b4")));
    h.insert_layer (0);
    b4.shapes (0).insert (db::Box (0, 0, 0, 0));

    if (pass < 2) {
      b0.insert (db::CellInstArray (db::CellInst (b4.cell_index ()), db::Trans (1/*r90*/), db::Vector(0, 10), db::Vector(10, 0), 5, 1));
      b0.insert (db::CellInstArray (db::CellInst (b1.cell_index ()), db::Trans (db::Vector (10, 0))));
      b1.insert (db::CellInstArray (db::CellInst (b2.cell_index ()), db::Trans ()));
      b1.insert (db::CellInstArray (db::CellInst (b2.cell_index ()), db::Trans (db::Vector (0, 20))));
      b1.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 40))));
      b2.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 0))));
      b2.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 10))));
      b3.insert (db::CellInstArray (db::CellInst (b4.cell_index ()), db::Trans (1/*r90*/)));
    } else {
      b0.insert (db::CellInstArray (db::CellInst (b4.cell_index ()), db::ICplxTrans (0.1, 90.0, false, db::Vector(0, 0)), db::Vector(0, 10), db::Vector(10, 0), 5, 1));
      b0.insert (db::CellInstArray (db::CellInst (b1.cell_index ()), db::ICplxTrans (0.1, 0.0, false, db::Vector(10, 0))));
      b1.insert (db::CellInstArray (db::CellInst (b2.cell_index ()), db::Trans ()));
      b1.insert (db::CellInstArray (db::CellInst (b2.cell_index ()), db::Trans (db::Vector (0, 200))));
      b1.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 400))));
      b2.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 0))));
      b2.insert (db::CellInstArray (db::CellInst (b3.cell_index ()), db::Trans (db::Vector (0, 100))));
      b3.insert (db::CellInstArray (db::CellInst (b4.cell_index ()), db::Trans (1/*r90*/)));
    }

    {

      db::InstanceStatistics<InstanceReferenceSum> rp (&g, a0.cell_index ());
      InstanceReferenceSum v = rp.value (a4.cell_index ());

      db::DPoint p;
      db::Matrix2d m;
      size_t n = 0; 
      for (db::RecursiveShapeIterator s (g, a0, 0); ! s.at_end (); ++s) {
        db::Point q = s.trans () * s.shape ().bbox ().center ();
        p += db::DVector (db::DPoint (q));
        m += db::Matrix2d (s.trans ());
        ++n;
      }

      EXPECT_EQ (v.n (), n);
      EXPECT_EQ (v.p ().to_string (), p.to_string ());
      EXPECT_EQ (v.m ().to_string (), m.to_string ());

    }

    {

      db::InstanceStatistics<InstanceReferenceSum> rp (&h, b0.cell_index ());
      InstanceReferenceSum v = rp.value (b4.cell_index ());

      db::DPoint p;
      db::Matrix2d m;
      size_t n = 0; 
      for (db::RecursiveShapeIterator s (h, b0, 0); ! s.at_end (); ++s) {
        db::Point q = s.trans () * s.shape ().bbox ().center ();
        p += db::DVector (db::DPoint (q));
        m += db::Matrix2d (s.trans ());
        ++n;
      }

      EXPECT_EQ (v.n (), n);
      EXPECT_EQ (v.p ().to_string (), p.to_string ());
      EXPECT_EQ (v.m ().to_string (), m.to_string ());

    }

  }


}

