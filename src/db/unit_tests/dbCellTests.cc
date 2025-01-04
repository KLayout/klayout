
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



#include "dbLayout.h"
#include "tlString.h"
#include "tlUnitTest.h"

TEST(1) 
{
  db::Manager m (true);
  db::Layout g (&m);
  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));

  db::Box b (0, 100, 1000, 1200);
  c1.shapes (0).insert (b);
  EXPECT_EQ (c1.bbox (), b);

  db::Box bb (0, -100, 2000, 2200);
  c1.shapes (1).insert (bb);
  EXPECT_EQ (c1.bbox (), b + bb);
  EXPECT_EQ (c1.bbox (0), b);
  EXPECT_EQ (c1.bbox (1), bb);

  //  inserting instances ..
  db::FTrans f (1, true);
  db::Vector p (-10, 20);
  db::Trans t (f.rot (), p);
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  EXPECT_EQ (c2.bbox (), t * (b + bb));
  EXPECT_EQ (c2.bbox (0), t * b);
  EXPECT_EQ (c2.bbox (1), t * bb);
  EXPECT_EQ (c1.bbox (), (b + bb));

  //  some basic testing of the instance trees
  int n;
  n = 0;
  for (db::Cell::touching_iterator r = c2.begin_touching (t * db::Box (-100, 0, 0, 100));
       ! r.at_end (); ++r) ++n; 
  EXPECT_EQ (n, 1);
  n = 0;
  for (db::Cell::overlapping_iterator r = c2.begin_overlapping (t * db::Box (-100, 0, 0, 100));
       ! r.at_end (); ++r) ++n; 
  EXPECT_EQ (n, 0);
  n = 0;
  for (db::Cell::overlapping_iterator r = c2.begin_overlapping (t * db::Box (-100, 0, 1, 100));
       ! r.at_end (); ++r) ++n; 
  EXPECT_EQ (n, 1);
  n = 0;
  for (db::Cell::touching_iterator r = c2.begin_touching (t * db::Box (-100, 0, -1, 100));
       ! r.at_end (); ++r) ++n; 
  EXPECT_EQ (n, 0);

  //  try adding a new instance into c2
  db::FTrans ff (2, true);
  db::Vector pp (10, -20);
  db::Trans tt (ff.rot (), pp);
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), tt));
  EXPECT_EQ (c2.bbox (), t * (b + bb) + tt * (b + bb));
  EXPECT_EQ (c2.bbox (0), t * b + tt * b);
  EXPECT_EQ (c2.bbox (1), t * bb + tt * bb);

}

struct p2s_compare 
{  
  bool operator() (const db::Cell::parent_inst_iterator &p1, const db::Cell::parent_inst_iterator &p2) 
  {
    if (p1->inst ().object ().cell_index () != p2->inst ().object ().cell_index ()) {
      return p1->inst ().object ().cell_index () < p2->inst ().object ().cell_index ();
    }
    if (p1->inst ().complex_trans () != p2->inst ().complex_trans ()) {
      return p1->inst ().complex_trans () < p2->inst ().complex_trans ();
    }
    return false;
  }
};

std::string p2s (const db::Cell &c)
{
  std::vector<db::Cell::parent_inst_iterator> pp;
  for (db::Cell::parent_inst_iterator p = c.begin_parent_insts (); ! p.at_end (); ++p) {
    pp.push_back (p);
  }
  std::sort (pp.begin (), pp.end (), p2s_compare ());
  std::string r;
  for (std::vector<db::Cell::parent_inst_iterator>::const_iterator ppp = pp.begin (); ppp != pp.end (); ++ppp) {
    db::Cell::parent_inst_iterator p = *ppp;
    if (! r.empty ()) {
      r += ",";
    }
    r += tl::to_string (p->inst ().object ().cell_index ()) + "[" + p->inst ().complex_trans ().to_string () + "]" + "#" + db::properties (p->child_inst ().prop_id ()).to_dict_var ().to_string ();
  }
  return r;
}

struct pc2s_compare 
{  
  bool operator() (const db::Cell::parent_cell_iterator &p1, const db::Cell::parent_cell_iterator &p2) 
  {
    return *p1 < *p2;
  }
};

std::string pc2s (const db::Cell &c)
{
  std::vector<db::Cell::parent_cell_iterator> pp;
  for (db::Cell::parent_cell_iterator p = c.begin_parent_cells (); p != c.end_parent_cells (); ++p) {
    pp.push_back (p);
  }
  std::sort (pp.begin (), pp.end (), pc2s_compare ());
  std::string r;
  for (std::vector<db::Cell::parent_cell_iterator>::const_iterator ppp = pp.begin (); ppp != pp.end (); ++ppp) {
    db::Cell::parent_cell_iterator p = *ppp;
    if (! r.empty ()) {
      r += ",";
    }
    r += tl::to_string (*p);
  }
  return r;
}

struct c2s_compare 
{  
  bool operator() (const db::Cell::const_iterator &p1, const db::Cell::const_iterator &p2) 
  {
    if (p1->cell_index () != p2->cell_index ()) {
      return p1->cell_index () < p2->cell_index ();
    }
    if (p1->complex_trans () != p2->complex_trans ()) {
      return p1->complex_trans () < p2->complex_trans ();
    }
    if (p1->prop_id () != p2->prop_id ()) {
      return db::properties (p1->prop_id ()).to_map () < db::properties (p2->prop_id ()).to_map ();
    }
    return false;
  }
};

std::string c2s (const db::Cell &c)
{
  std::vector<db::Cell::const_iterator> pp;
  for (db::Cell::const_iterator p = c.begin (); ! p.at_end (); ++p) {
    pp.push_back (p);
  }
  std::sort (pp.begin (), pp.end (), c2s_compare ());
  std::string r;
  for (std::vector<db::Cell::const_iterator>::const_iterator ppp = pp.begin (); ppp != pp.end (); ++ppp) {
    db::Cell::const_iterator p = *ppp;
    if (! r.empty ()) {
      r += ",";
    }
    r += tl::to_string (p->cell_index ()) + "[" + p->complex_trans ().to_string () + "]" + "#" + db::properties (p->prop_id ()).to_dict_var ().to_string ();
  }
  return r;
}

std::string c2s_unsorted (const db::Cell &c)
{
  std::string r;
  for (db::Cell::const_iterator p = c.begin (); ! p.at_end (); ++p) {
    if (! r.empty ()) {
      r += ",";
    }
    r += tl::to_string (p->cell_index ()) + "[" + p->complex_trans ().to_string () + "]" + "#" + db::properties (p->prop_id ()).to_dict_var ().to_string ();
  }
  return r;
}

struct ct2s_compare 
{  
  bool operator() (const db::Cell::touching_iterator &p1, const db::Cell::touching_iterator &p2) 
  {
    if (p1->cell_index () != p2->cell_index ()) {
      return p1->cell_index () < p2->cell_index ();
    }
    if (p1->complex_trans () != p2->complex_trans ()) {
      return p1->complex_trans () < p2->complex_trans ();
    }
    if (p1->prop_id () != p2->prop_id ()) {
      return db::properties (p1->prop_id ()).to_map () < db::properties (p2->prop_id ()).to_map ();
    }
    return false;
  }
};

std::string ct2s (const db::Cell &c)
{
  std::vector<db::Cell::touching_iterator> pp;
  for (db::Cell::touching_iterator p = c.begin_touching (db::Box (-10000, -10000, 10000, 10000)); ! p.at_end (); ++p) {
    pp.push_back (p);
  }
  std::sort (pp.begin (), pp.end (), ct2s_compare ());
  std::string r;
  for (std::vector<db::Cell::touching_iterator>::const_iterator ppp = pp.begin (); ppp != pp.end (); ++ppp) {
    db::Cell::touching_iterator p = *ppp;
    if (! r.empty ()) {
      r += ",";
    }
    r += tl::to_string (p->cell_index ()) + "[" + p->complex_trans ().to_string () + "]" + "#" + db::properties (p->prop_id ()).to_dict_var ().to_string ();
  }
  return r;
}

struct cc2s_compare 
{  
  bool operator() (const db::Cell::child_cell_iterator &p1, const db::Cell::child_cell_iterator &p2) 
  {
    return *p1 < *p2;
  }
};

std::string cc2s (const db::Cell &c)
{
  std::vector<db::Cell::child_cell_iterator> pp;
  for (db::Cell::child_cell_iterator p = c.begin_child_cells (); ! p.at_end (); ++p) {
    pp.push_back (p);
  }
  std::sort (pp.begin (), pp.end (), cc2s_compare ());
  std::string r;
  for (std::vector<db::Cell::child_cell_iterator>::const_iterator ppp = pp.begin (); ppp != pp.end (); ++ppp) {
    db::Cell::child_cell_iterator p = *ppp;
    if (! r.empty ()) {
      r += ",";
    }
    r += tl::to_string (*p);
  }
  return r;
}

static unsigned int pi = 0;

void insert_ci (db::Cell &c, size_t ci, const db::Trans &t)
{
  db::PropertiesSet props;
  props.insert (tl::Variant ("id"), pi);
  db::properties_id_type pid = db::properties_id (props);
  if (pi == 0) {
    c.insert (db::CellInstArray (db::CellInst (db::cell_index_type (ci)), t));
  } else {
    c.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (db::cell_index_type (ci)), t), pid));
  }
  pi = (pi + 1) % 3;
}

TEST(2) 
{
  ::pi = 0;

  db::PropertiesSet props;
  props.insert (tl::Variant ("id"), 17);
  db::properties_id_type pid17 = db::properties_id (props);

  props.clear ();
  props.insert (tl::Variant ("id"), 11);
  db::properties_id_type pid11 = db::properties_id (props);

  props.clear ();
  props.insert (tl::Variant ("id"), 13);
  db::properties_id_type pid13 = db::properties_id (props);

  db::Manager m (true);
  db::Layout g (&m);
  db::Cell &c0 (g.cell (g.add_cell ()));
  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));
  db::Cell &c3 (g.cell (g.add_cell ()));
  db::Cell &c4 (g.cell (g.add_cell ()));
  db::Cell &c5 (g.cell (g.add_cell ()));

  db::Box b (0, 100, 1000, 1200);
  c0.shapes (0).insert (b);
  c1.shapes (1).insert (b);
  c2.shapes (2).insert (b);
  c3.shapes (3).insert (b);
  c4.shapes (4).insert (b);
  c5.shapes (5).insert (b);

  db::Trans t;
  insert_ci (c0, c1.cell_index (), t);
  insert_ci (c1, c2.cell_index (), t);
  insert_ci (c2, c3.cell_index (), t);
  insert_ci (c3, c4.cell_index (), t);
  insert_ci (c4, c5.cell_index (), t);

  EXPECT_EQ (p2s(c0), "");
  EXPECT_EQ (p2s(c1), "0[r0 *1 0,0]#{}");
  EXPECT_EQ (p2s(c2), "1[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (p2s(c3), "2[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (p2s(c4), "3[r0 *1 0,0]#{}");
  EXPECT_EQ (p2s(c5), "4[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (pc2s(c0), "");
  EXPECT_EQ (pc2s(c1), "0");
  EXPECT_EQ (pc2s(c2), "1");
  EXPECT_EQ (pc2s(c3), "2");
  EXPECT_EQ (pc2s(c4), "3");
  EXPECT_EQ (pc2s(c5), "4");
  EXPECT_EQ (c2s(c0), "1[r0 *1 0,0]#{}");
  EXPECT_EQ (c2s(c1), "2[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (c2s(c2), "3[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (c2s(c3), "4[r0 *1 0,0]#{}");
  EXPECT_EQ (c2s(c4), "5[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (c2s(c5), "");
  EXPECT_EQ (ct2s(c0), "1[r0 *1 0,0]#{}");
  EXPECT_EQ (ct2s(c1), "2[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (ct2s(c2), "3[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (ct2s(c3), "4[r0 *1 0,0]#{}");
  EXPECT_EQ (ct2s(c4), "5[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (ct2s(c5), "");
  EXPECT_EQ (cc2s(c0), "1");
  EXPECT_EQ (cc2s(c1), "2");
  EXPECT_EQ (cc2s(c2), "3");
  EXPECT_EQ (cc2s(c3), "4");
  EXPECT_EQ (cc2s(c4), "5");
  EXPECT_EQ (cc2s(c5), "");
   
  insert_ci (c0, c2.cell_index (), t);
  insert_ci (c1, c3.cell_index (), t);
  insert_ci (c2, c4.cell_index (), t);
  insert_ci (c3, c5.cell_index (), t);
  EXPECT_EQ (p2s(c0), "");
  EXPECT_EQ (p2s(c1), "0[r0 *1 0,0]#{}");
  EXPECT_EQ (p2s(c2), "0[r0 *1 0,0]#{id=>2},1[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (p2s(c3), "1[r0 *1 0,0]#{},2[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (p2s(c4), "2[r0 *1 0,0]#{id=>1},3[r0 *1 0,0]#{}");
  EXPECT_EQ (p2s(c5), "3[r0 *1 0,0]#{id=>2},4[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (pc2s(c0), "");
  EXPECT_EQ (pc2s(c1), "0");
  EXPECT_EQ (pc2s(c2), "0,1");
  EXPECT_EQ (pc2s(c3), "1,2");
  EXPECT_EQ (pc2s(c4), "2,3");
  EXPECT_EQ (pc2s(c5), "3,4");
  EXPECT_EQ (c2s(c0), "1[r0 *1 0,0]#{},2[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (c2s(c1), "2[r0 *1 0,0]#{id=>1},3[r0 *1 0,0]#{}");
  EXPECT_EQ (c2s(c2), "3[r0 *1 0,0]#{id=>2},4[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (c2s(c3), "4[r0 *1 0,0]#{},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (c2s(c4), "5[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (c2s(c5), "");
  EXPECT_EQ (ct2s(c0), "1[r0 *1 0,0]#{},2[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (ct2s(c1), "2[r0 *1 0,0]#{id=>1},3[r0 *1 0,0]#{}");
  EXPECT_EQ (ct2s(c2), "3[r0 *1 0,0]#{id=>2},4[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (ct2s(c3), "4[r0 *1 0,0]#{},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (ct2s(c4), "5[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (ct2s(c5), "");
  EXPECT_EQ (cc2s(c0), "1,2");
  EXPECT_EQ (cc2s(c1), "2,3");
  EXPECT_EQ (cc2s(c2), "3,4");
  EXPECT_EQ (cc2s(c3), "4,5");
  EXPECT_EQ (cc2s(c4), "5");
  EXPECT_EQ (cc2s(c5), "");

  insert_ci (c0, c3.cell_index (), t);
  insert_ci (c1, c4.cell_index (), t);
  insert_ci (c2, c5.cell_index (), t);
  EXPECT_EQ (p2s(c0), "");
  EXPECT_EQ (p2s(c1), "0[r0 *1 0,0]#{}");
  EXPECT_EQ (p2s(c2), "0[r0 *1 0,0]#{id=>2},1[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (p2s(c3), "0[r0 *1 0,0]#{},1[r0 *1 0,0]#{},2[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (p2s(c4), "1[r0 *1 0,0]#{id=>1},2[r0 *1 0,0]#{id=>1},3[r0 *1 0,0]#{}");
  EXPECT_EQ (p2s(c5), "2[r0 *1 0,0]#{id=>2},3[r0 *1 0,0]#{id=>2},4[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (pc2s(c0), "");
  EXPECT_EQ (pc2s(c1), "0");
  EXPECT_EQ (pc2s(c2), "0,1");
  EXPECT_EQ (pc2s(c3), "0,1,2");
  EXPECT_EQ (pc2s(c4), "1,2,3");
  EXPECT_EQ (pc2s(c5), "2,3,4");
  EXPECT_EQ (c2s(c0), "1[r0 *1 0,0]#{},2[r0 *1 0,0]#{id=>2},3[r0 *1 0,0]#{}");
  EXPECT_EQ (c2s(c1), "2[r0 *1 0,0]#{id=>1},3[r0 *1 0,0]#{},4[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (c2s(c2), "3[r0 *1 0,0]#{id=>2},4[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (c2s(c3), "4[r0 *1 0,0]#{},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (c2s(c4), "5[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (c2s(c5), "");
  EXPECT_EQ (ct2s(c0), "1[r0 *1 0,0]#{},2[r0 *1 0,0]#{id=>2},3[r0 *1 0,0]#{}");
  EXPECT_EQ (ct2s(c1), "2[r0 *1 0,0]#{id=>1},3[r0 *1 0,0]#{},4[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (ct2s(c2), "3[r0 *1 0,0]#{id=>2},4[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (ct2s(c3), "4[r0 *1 0,0]#{},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (ct2s(c4), "5[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (ct2s(c5), "");
  EXPECT_EQ (cc2s(c0), "1,2,3");
  EXPECT_EQ (cc2s(c1), "2,3,4");
  EXPECT_EQ (cc2s(c2), "3,4,5");
  EXPECT_EQ (cc2s(c3), "4,5");
  EXPECT_EQ (cc2s(c4), "5");
  EXPECT_EQ (cc2s(c5), "");

  insert_ci (c0, c4.cell_index (), t);
  insert_ci (c1, c5.cell_index (), t);
  EXPECT_EQ (p2s(c0), "");
  EXPECT_EQ (p2s(c1), "0[r0 *1 0,0]#{}");
  EXPECT_EQ (p2s(c2), "0[r0 *1 0,0]#{id=>2},1[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (p2s(c3), "0[r0 *1 0,0]#{},1[r0 *1 0,0]#{},2[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (p2s(c4), "0[r0 *1 0,0]#{},1[r0 *1 0,0]#{id=>1},2[r0 *1 0,0]#{id=>1},3[r0 *1 0,0]#{}");
  EXPECT_EQ (p2s(c5), "1[r0 *1 0,0]#{id=>1},2[r0 *1 0,0]#{id=>2},3[r0 *1 0,0]#{id=>2},4[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (pc2s(c0), "");
  EXPECT_EQ (pc2s(c1), "0");
  EXPECT_EQ (pc2s(c2), "0,1");
  EXPECT_EQ (pc2s(c3), "0,1,2");
  EXPECT_EQ (pc2s(c4), "0,1,2,3");
  EXPECT_EQ (pc2s(c5), "1,2,3,4");
  EXPECT_EQ (c2s(c0), "1[r0 *1 0,0]#{},2[r0 *1 0,0]#{id=>2},3[r0 *1 0,0]#{},4[r0 *1 0,0]#{}");
  EXPECT_EQ (c2s(c1), "2[r0 *1 0,0]#{id=>1},3[r0 *1 0,0]#{},4[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (c2s(c2), "3[r0 *1 0,0]#{id=>2},4[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (c2s(c3), "4[r0 *1 0,0]#{},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (c2s(c4), "5[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (c2s(c5), "");
  EXPECT_EQ (ct2s(c0), "1[r0 *1 0,0]#{},2[r0 *1 0,0]#{id=>2},3[r0 *1 0,0]#{},4[r0 *1 0,0]#{}");
  EXPECT_EQ (ct2s(c1), "2[r0 *1 0,0]#{id=>1},3[r0 *1 0,0]#{},4[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (ct2s(c2), "3[r0 *1 0,0]#{id=>2},4[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (ct2s(c3), "4[r0 *1 0,0]#{},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (ct2s(c4), "5[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (ct2s(c5), "");
  EXPECT_EQ (cc2s(c0), "1,2,3,4");
  EXPECT_EQ (cc2s(c1), "2,3,4,5");
  EXPECT_EQ (cc2s(c2), "3,4,5");
  EXPECT_EQ (cc2s(c3), "4,5");
  EXPECT_EQ (cc2s(c4), "5");
  EXPECT_EQ (cc2s(c5), "");

  insert_ci (c0, c5.cell_index (), t);
  EXPECT_EQ (p2s(c0), "");
  EXPECT_EQ (p2s(c1), "0[r0 *1 0,0]#{}");
  EXPECT_EQ (p2s(c2), "0[r0 *1 0,0]#{id=>2},1[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (p2s(c3), "0[r0 *1 0,0]#{},1[r0 *1 0,0]#{},2[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (p2s(c4), "0[r0 *1 0,0]#{},1[r0 *1 0,0]#{id=>1},2[r0 *1 0,0]#{id=>1},3[r0 *1 0,0]#{}");
  EXPECT_EQ (p2s(c5), "0[r0 *1 0,0]#{id=>2},1[r0 *1 0,0]#{id=>1},2[r0 *1 0,0]#{id=>2},3[r0 *1 0,0]#{id=>2},4[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (pc2s(c0), "");
  EXPECT_EQ (pc2s(c1), "0");
  EXPECT_EQ (pc2s(c2), "0,1");
  EXPECT_EQ (pc2s(c3), "0,1,2");
  EXPECT_EQ (pc2s(c4), "0,1,2,3");
  EXPECT_EQ (pc2s(c5), "0,1,2,3,4");
  EXPECT_EQ (c2s(c0), "1[r0 *1 0,0]#{},2[r0 *1 0,0]#{id=>2},3[r0 *1 0,0]#{},4[r0 *1 0,0]#{},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (c2s(c1), "2[r0 *1 0,0]#{id=>1},3[r0 *1 0,0]#{},4[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (c2s(c2), "3[r0 *1 0,0]#{id=>2},4[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (c2s(c3), "4[r0 *1 0,0]#{},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (c2s(c4), "5[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (c2s(c5), "");
  EXPECT_EQ (ct2s(c0), "1[r0 *1 0,0]#{},2[r0 *1 0,0]#{id=>2},3[r0 *1 0,0]#{},4[r0 *1 0,0]#{},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (ct2s(c1), "2[r0 *1 0,0]#{id=>1},3[r0 *1 0,0]#{},4[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (ct2s(c2), "3[r0 *1 0,0]#{id=>2},4[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (ct2s(c3), "4[r0 *1 0,0]#{},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (ct2s(c4), "5[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (ct2s(c5), "");
  EXPECT_EQ (cc2s(c0), "1,2,3,4,5");
  EXPECT_EQ (cc2s(c1), "2,3,4,5");
  EXPECT_EQ (cc2s(c2), "3,4,5");
  EXPECT_EQ (cc2s(c3), "4,5");
  EXPECT_EQ (cc2s(c4), "5");
  EXPECT_EQ (cc2s(c5), "");

  ::pi = 1;
  insert_ci (c0, c1.cell_index (), t);
  insert_ci (c1, c2.cell_index (), t);
  insert_ci (c2, c3.cell_index (), t);
  insert_ci (c3, c4.cell_index (), t);
  insert_ci (c4, c5.cell_index (), t);
  EXPECT_EQ (p2s(c0), "");
  EXPECT_EQ (p2s(c1), "0[r0 *1 0,0]#{},0[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (p2s(c2), "0[r0 *1 0,0]#{id=>2},1[r0 *1 0,0]#{id=>1},1[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (p2s(c3), "0[r0 *1 0,0]#{},1[r0 *1 0,0]#{},2[r0 *1 0,0]#{},2[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (p2s(c4), "0[r0 *1 0,0]#{},1[r0 *1 0,0]#{id=>1},2[r0 *1 0,0]#{id=>1},3[r0 *1 0,0]#{},3[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (p2s(c5), "0[r0 *1 0,0]#{id=>2},1[r0 *1 0,0]#{id=>1},2[r0 *1 0,0]#{id=>2},3[r0 *1 0,0]#{id=>2},4[r0 *1 0,0]#{id=>1},4[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (pc2s(c0), "");
  EXPECT_EQ (pc2s(c1), "0");
  EXPECT_EQ (pc2s(c2), "0,1");
  EXPECT_EQ (pc2s(c3), "0,1,2");
  EXPECT_EQ (pc2s(c4), "0,1,2,3");
  EXPECT_EQ (pc2s(c5), "0,1,2,3,4");
  EXPECT_EQ (c2s(c0), "1[r0 *1 0,0]#{},1[r0 *1 0,0]#{id=>1},2[r0 *1 0,0]#{id=>2},3[r0 *1 0,0]#{},4[r0 *1 0,0]#{},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (c2s(c1), "2[r0 *1 0,0]#{id=>1},2[r0 *1 0,0]#{id=>2},3[r0 *1 0,0]#{},4[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (c2s(c2), "3[r0 *1 0,0]#{},3[r0 *1 0,0]#{id=>2},4[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (c2s(c3), "4[r0 *1 0,0]#{},4[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (c2s(c4), "5[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (c2s(c5), "");
  EXPECT_EQ (c2s_unsorted(c0), "1[r0 *1 0,0]#{},3[r0 *1 0,0]#{},4[r0 *1 0,0]#{},2[r0 *1 0,0]#{id=>2},5[r0 *1 0,0]#{id=>2},1[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (c2s_unsorted(c1), "3[r0 *1 0,0]#{},2[r0 *1 0,0]#{id=>1},4[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>1},2[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (c2s_unsorted(c2), "3[r0 *1 0,0]#{},3[r0 *1 0,0]#{id=>2},4[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (c2s_unsorted(c3), "4[r0 *1 0,0]#{},5[r0 *1 0,0]#{id=>2},4[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (c2s_unsorted(c4), "5[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (c2s_unsorted(c5), "");
  EXPECT_EQ (ct2s(c0), "1[r0 *1 0,0]#{},1[r0 *1 0,0]#{id=>1},2[r0 *1 0,0]#{id=>2},3[r0 *1 0,0]#{},4[r0 *1 0,0]#{},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (ct2s(c1), "2[r0 *1 0,0]#{id=>1},2[r0 *1 0,0]#{id=>2},3[r0 *1 0,0]#{},4[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (ct2s(c2), "3[r0 *1 0,0]#{},3[r0 *1 0,0]#{id=>2},4[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (ct2s(c3), "4[r0 *1 0,0]#{},4[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (ct2s(c4), "5[r0 *1 0,0]#{id=>1},5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (ct2s(c5), "");
  EXPECT_EQ (cc2s(c0), "1,2,3,4,5");
  EXPECT_EQ (cc2s(c1), "2,3,4,5");
  EXPECT_EQ (cc2s(c2), "3,4,5");
  EXPECT_EQ (cc2s(c3), "4,5");
  EXPECT_EQ (cc2s(c4), "5");
  EXPECT_EQ (cc2s(c5), "");

  db::Cell::const_iterator inst = c0.begin ();
  EXPECT_EQ (c2s_unsorted(c0), "1[r0 *1 0,0]#{},3[r0 *1 0,0]#{},4[r0 *1 0,0]#{},2[r0 *1 0,0]#{id=>2},5[r0 *1 0,0]#{id=>2},1[r0 *1 0,0]#{id=>1}");
  db::Trans t1 (1, db::Vector (100, -200));
  c0.replace (*inst, db::CellInstArray (db::CellInst (c2.cell_index ()), t1));
  EXPECT_EQ (c2s_unsorted(c0), "2[r90 *1 100,-200]#{},3[r0 *1 0,0]#{},4[r0 *1 0,0]#{},2[r0 *1 0,0]#{id=>2},5[r0 *1 0,0]#{id=>2},1[r0 *1 0,0]#{id=>1}");
  ++inst;
  ++inst;
  ++inst;
  c0.replace (*inst, db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (c3.cell_index ()), t1), pid17));
  EXPECT_EQ (c2s_unsorted(c0), "2[r90 *1 100,-200]#{},3[r0 *1 0,0]#{},4[r0 *1 0,0]#{},3[r90 *1 100,-200]#{id=>17},5[r0 *1 0,0]#{id=>2},1[r0 *1 0,0]#{id=>1}");
  c0.replace_prop_id (*inst, pid11);
  EXPECT_EQ (c2s_unsorted(c0), "2[r90 *1 100,-200]#{},3[r0 *1 0,0]#{},4[r0 *1 0,0]#{},3[r90 *1 100,-200]#{id=>11},5[r0 *1 0,0]#{id=>2},1[r0 *1 0,0]#{id=>1}");
  inst = c0.begin ();

  //  replace a non-property array with one with properties:
  c0.replace (*inst, db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (c3.cell_index ()), db::Trans ()), pid13));
  EXPECT_EQ (c2s_unsorted(c0), "3[r0 *1 0,0]#{},4[r0 *1 0,0]#{},3[r90 *1 100,-200]#{id=>11},5[r0 *1 0,0]#{id=>2},1[r0 *1 0,0]#{id=>1},3[r0 *1 0,0]#{id=>13}");

  db::Cell &cx = g.cell (g.add_cell (g.cell_name (c0.cell_index ())));

  //  erase from iterator
  inst = c0.begin ();
  db::Instance i0 = *inst;
  c0.erase (inst);
  //  HINT: doing a c2s_unsorted on c0 would disturb the index order of c0, because it uses a flat
  //  iterator. Therefore we make a copy in order to prevent that problem. See bug #{id=>120}.
  cx = c0;
  EXPECT_EQ (c2s_unsorted(cx), "4[r0 *1 0,0]#{},3[r90 *1 100,-200]#{id=>11},5[r0 *1 0,0]#{id=>2},1[r0 *1 0,0]#{id=>1},3[r0 *1 0,0]#{id=>13}");
  EXPECT_EQ (c0.cell_instances (), size_t (5));
  // not yet: EXPECT_EQ (c0.empty (), false);
  inst = c0.begin ();
  db::Instance i1 = *inst;
  c0.erase (inst);
  cx = c0;
  EXPECT_EQ (c2s_unsorted(cx), "3[r90 *1 100,-200]#{id=>11},5[r0 *1 0,0]#{id=>2},1[r0 *1 0,0]#{id=>1},3[r0 *1 0,0]#{id=>13}");
  inst = c0.begin ();
  db::Instance i2 = *inst;
  c0.erase (inst);
  cx = c0;
  EXPECT_EQ (c2s_unsorted(cx), "5[r0 *1 0,0]#{id=>2},1[r0 *1 0,0]#{id=>1},3[r0 *1 0,0]#{id=>13}");
  inst = c0.begin ();
  db::Instance i3 = *inst;
  db::Instance i4 = *++inst;
  db::Instance i5 = *++inst;

  //  note: double delete is not supported in non-editable mode 
  if (db::default_editable_mode ()) {

    /* currently does not issue an exception:
    caught = false;
    bool caught;
    try {
      c0.erase (i0); //  already deleted
    } catch (...) {
      caught = true;
    }
    EXPECT_EQ (caught, true);
    */
    c0.erase (i0); //  already deleted

    /* currently does not issue an exception:
    caught = false;
    try {
      c0.erase (i1); //  already deleted
    } catch (...) {
      caught = true;
    }
    EXPECT_EQ (caught, true);
    */
    c0.erase (i1); //  already deleted

  }

  c0.erase (i5);
  EXPECT_EQ (c2s_unsorted(c0), "5[r0 *1 0,0]#{id=>2},1[r0 *1 0,0]#{id=>1}");
  EXPECT_EQ (c0.cell_instances (), size_t (2));
  c0.erase (i4);
  EXPECT_EQ (c2s_unsorted(c0), "5[r0 *1 0,0]#{id=>2}");
  EXPECT_EQ (c0.cell_instances (), size_t (1));
  //  Not yet: EXPECT_EQ (c0.empty (), false);

  //  note: double delete is not supported in non-editable mode 
  if (db::default_editable_mode ()) {

    /* currently does not issue an exception:
    caught = false;
    try {
      c0.erase (i2); //  already deleted
    } catch (...) {
      caught = true;
    }
    EXPECT_EQ (caught, true);
    */
    c0.erase (i2); //  already deleted

    /* currently does not issue an exception:
    caught = false;
    try {
      c0.erase (i5); //  already deleted
    } catch (...) {
      caught = true;
    }
    EXPECT_EQ (caught, true);
    */
    c0.erase (i5); //  already deleted

  }

  c0.erase (i3);
  EXPECT_EQ (c2s_unsorted(c0), "");
  EXPECT_EQ (c0.cell_instances (), size_t (0));
  //  Not yet: EXPECT_EQ (c0.empty (), true);
}

TEST(3) 
{
  ::pi = 0;

  db::PropertiesSet props;
  props.insert (tl::Variant ("id"), 17);
  db::properties_id_type pid17 = db::properties_id (props);

  props.clear ();
  props.insert (tl::Variant ("id"), 18);
  db::properties_id_type pid18 = db::properties_id (props);

  props.clear ();
  props.insert (tl::Variant ("id"), 21);
  db::properties_id_type pid21 = db::properties_id (props);

  props.clear ();
  props.insert (tl::Variant ("id"), 1);
  db::properties_id_type pid1 = db::properties_id (props);

  props.clear ();
  props.insert (tl::Variant ("id"), 10);
  db::properties_id_type pid10 = db::properties_id (props);

  db::Manager m (true);
  db::Layout g (&m);
  db::Cell &c0 (g.cell (g.add_cell ()));
  db::Cell &c1 (g.cell (g.add_cell ()));

  db::Box b (0, 100, 1000, 1200);
  c0.shapes (0).insert (b);
  c1.shapes (1).insert (b);

  db::Trans t1;
  db::Trans t2 (db::Vector (100, -100));
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), t1));
  c0.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (c1.cell_index ()), t1), pid1));
  c0.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (c1.cell_index ()), t2), pid10));

  EXPECT_EQ (c2s_unsorted(c0), "1[r0 *1 0,0]#{},1[r0 *1 0,0]#{id=>1},1[r0 *1 100,-100]#{id=>10}");

  db::Cell::const_iterator i = c0.begin ();
  ++i; ++i;
  db::Cell::const_iterator i2 = i;
  ++i;
  EXPECT_EQ (i.at_end (), true);

  c0.erase (i2);
  EXPECT_EQ (c2s_unsorted(c0), "1[r0 *1 0,0]#{},1[r0 *1 0,0]#{id=>1}");

  c0.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (c1.cell_index ()), t2), pid17));
  EXPECT_EQ (c2s_unsorted(c0), "1[r0 *1 0,0]#{},1[r0 *1 0,0]#{id=>1},1[r0 *1 100,-100]#{id=>17}");

  i = c0.begin ();
  ++i; ++i;
  db::Instance inst2 = *i;
  ++i;

  c0.erase (inst2);
  EXPECT_EQ (c2s_unsorted(c0), "1[r0 *1 0,0]#{},1[r0 *1 0,0]#{id=>1}");

  c0.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (c1.cell_index ()), t2), pid18));
  c0.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (c1.cell_index ()), t2), pid21));
  EXPECT_EQ (c2s_unsorted(c0), "1[r0 *1 0,0]#{},1[r0 *1 0,0]#{id=>1},1[r0 *1 100,-100]#{id=>18},1[r0 *1 100,-100]#{id=>21}");

  i = c0.begin ();
  std::vector<db::Instance> insts;
  insts.push_back (*i);
  ++i; ++i;
  insts.push_back (*i);
  std::swap (insts[0], insts[1]);
  std::sort (insts.begin (), insts.end ());

  c0.erase_insts (insts);
  EXPECT_EQ (c2s_unsorted(c0), "1[r0 *1 0,0]#{id=>1},1[r0 *1 100,-100]#{id=>21}");
}

TEST(3a) 
{
  ::pi = 0;

  db::Manager m (true);
  db::Layout g (&m);
  db::Cell &c0 (g.cell (g.add_cell ()));
  db::Cell &c1 (g.cell (g.add_cell ()));

  db::Trans t (db::Vector (100, -100));
  db::Instance inst = c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), t));
  EXPECT_EQ (inst.to_string (), "cell_index=1 r0 100,-100");

  inst = c0.transform (inst, db::Trans (5));
  EXPECT_EQ (inst.to_string (), "cell_index=1 m45 -100,100");

  inst = c0.transform (inst, db::ICplxTrans (2.5));
  EXPECT_EQ (inst.to_string (), "cell_index=1 m45 *2.5 -250,250");

  inst = c0.replace (inst, db::CellInstArray (db::CellInst (c1.cell_index ()), t));
  EXPECT_EQ (inst.to_string (), "cell_index=1 r0 100,-100");

  inst = c0.transform_into (inst, db::Trans (5));
  EXPECT_EQ (inst.to_string (), "cell_index=1 r0 -100,100");

  inst = c0.transform_into (inst, db::ICplxTrans (2.5));
  EXPECT_EQ (inst.to_string (), "cell_index=1 r0 -250,250");

  t = db::Trans (5, db::Vector (100, -100));
  inst = c0.replace (inst, db::CellInstArray (db::CellInst (c1.cell_index ()), t));
  EXPECT_EQ (inst.to_string (), "cell_index=1 m45 100,-100");

  inst = c0.transform_into (inst, db::Trans (5));
  EXPECT_EQ (inst.to_string (), "cell_index=1 m45 -100,100");

  db::CplxTrans ti (2.5, 45.0, false, db::DVector (10, 20));

  //  NOTE: even a ICPlxTrans carries a float displacement as accuracy reserve.
  EXPECT_EQ ((ti * inst.complex_trans () * ti.inverted ()).to_string (), "m90 *1 -333.553390593,0");

  inst = c0.transform_into (inst, db::ICplxTrans (ti));
  EXPECT_EQ (inst.to_string (), "cell_index=1 m90 -334,0");

  t = db::Trans (5, db::Vector (100, -100));
  inst = c0.replace (inst, db::CellInstArray (db::CellInst (c1.cell_index ()), t));
  EXPECT_EQ (inst.to_string (), "cell_index=1 m45 100,-100");

  c0.transform_into (db::Trans (5));
  inst = *c0.begin ();
  EXPECT_EQ (inst.to_string (), "cell_index=1 m45 -100,100");

  c0.transform_into (db::ICplxTrans (ti));
  inst = *c0.begin ();
  EXPECT_EQ (inst.to_string (), "cell_index=1 m90 -334,0");

  c0.transform (db::Trans (5));
  inst = *c0.begin ();
  EXPECT_EQ (inst.to_string (), "cell_index=1 r270 0,-334");

  c0.transform (db::ICplxTrans (ti));
  inst = *c0.begin ();
  EXPECT_EQ (inst.to_string (), "cell_index=1 r315 *2.5 600,-570");
}

TEST(3b) 
{
  ::pi = 0;

  db::PropertiesSet props;
  props.insert (tl::Variant ("id"), 5);
  db::properties_id_type pid5 = db::properties_id (props);

  props.clear ();
  props.insert (tl::Variant ("id"), 17);
  db::properties_id_type pid17 = db::properties_id (props);

  db::Manager m (true);
  db::Layout g (&m);
  db::Cell &c0 (g.cell (g.add_cell ()));
  db::Cell &c1 (g.cell (g.add_cell ()));

  db::Trans t (db::Vector (100, -100));
  c0.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (c1.cell_index ()), t), pid5));

  db::Box b (0, 100, 1000, 1200);
  c0.shapes (0).insert (db::BoxWithProperties (b, pid17));
  c1.shapes (1).insert (b);

  //  Note: this requires editable mode since db::Shapes::erase is permitted in editable mode only
  //  (erase is triggered by undo)
  if (db::default_editable_mode ()) {

    m.transaction ("t");
    g.transform (db::ICplxTrans (2.5));
    m.commit ();

    EXPECT_EQ (c1.cell_instances (), size_t (0));
    EXPECT_EQ (c0.cell_instances (), size_t (1));
    EXPECT_EQ (c0.begin ()->to_string (), "cell_index=1 r0 250,-250 props={id=>5}");

    EXPECT_EQ (c0.shapes (0).size (), size_t (1));
    EXPECT_EQ (c0.shapes (1).size (), size_t (0));
    EXPECT_EQ (c1.shapes (0).size (), size_t (0));
    EXPECT_EQ (c1.shapes (1).size (), size_t (1));
 
    EXPECT_EQ (c0.shapes (0).begin (db::ShapeIterator::All)->to_string (), "box (0,250;2500,3000) props={id=>17}");
    EXPECT_EQ (c1.shapes (1).begin (db::ShapeIterator::All)->to_string (), "box (0,250;2500,3000)"); 

    m.undo ();

    EXPECT_EQ (c1.cell_instances (), size_t (0));
    EXPECT_EQ (c0.cell_instances (), size_t (1));
    EXPECT_EQ (c0.begin ()->to_string (), "cell_index=1 r0 100,-100 props={id=>5}");

    EXPECT_EQ (c0.shapes (0).size (), size_t (1));
    EXPECT_EQ (c0.shapes (1).size (), size_t (0));
    EXPECT_EQ (c1.shapes (0).size (), size_t (0));
    EXPECT_EQ (c1.shapes (1).size (), size_t (1));
 
    EXPECT_EQ (c0.shapes (0).begin (db::ShapeIterator::All)->to_string (), "box (0,100;1000,1200) props={id=>17}");
    EXPECT_EQ (c1.shapes (1).begin (db::ShapeIterator::All)->to_string (), "box (0,100;1000,1200)"); 

    m.redo ();

    EXPECT_EQ (c1.cell_instances (), size_t (0));
    EXPECT_EQ (c0.cell_instances (), size_t (1));
    EXPECT_EQ (c0.begin ()->to_string (), "cell_index=1 r0 250,-250 props={id=>5}");

    EXPECT_EQ (c0.shapes (0).size (), size_t (1));
    EXPECT_EQ (c0.shapes (1).size (), size_t (0));
    EXPECT_EQ (c1.shapes (0).size (), size_t (0));
    EXPECT_EQ (c1.shapes (1).size (), size_t (1));
 
    EXPECT_EQ (c0.shapes (0).begin (db::ShapeIterator::All)->to_string (), "box (0,250;2500,3000) props={id=>17}");
    EXPECT_EQ (c1.shapes (1).begin (db::ShapeIterator::All)->to_string (), "box (0,250;2500,3000)"); 

  }
}

TEST(3c)
{
  ::pi = 0;

  db::PropertiesSet props;
  props.insert (tl::Variant ("id"), 5);
  db::properties_id_type pid5 = db::properties_id (props);

  props.clear ();
  props.insert (tl::Variant ("id"), 17);
  db::properties_id_type pid17 = db::properties_id (props);

  db::Manager m (true);
  db::Layout g (&m);
  db::Cell &c0 (g.cell (g.add_cell ()));
  db::Cell &c1 (g.cell (g.add_cell ()));

  db::Trans t (db::Vector (100, -100));
  c0.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (c1.cell_index ()), t), pid5));

  db::Box b (0, 100, 1000, 1200);
  c0.shapes (0).insert (db::BoxWithProperties (b, pid17));
  c1.shapes (1).insert (b);

  //  Note: this requires editable mode since db::Shapes::erase is permitted in editable mode only
  //  (erase is triggered by undo)
  if (db::default_editable_mode ()) {

    m.transaction ("t");
    c0.transform (db::ICplxTrans (2.5));
    m.commit ();

    EXPECT_EQ (c1.cell_instances (), size_t (0));
    EXPECT_EQ (c0.cell_instances (), size_t (1));
    EXPECT_EQ (c0.begin ()->to_string (), "cell_index=1 r0 *2.5 250,-250 props={id=>5}");

    EXPECT_EQ (c0.shapes (0).size (), size_t (1));
    EXPECT_EQ (c0.shapes (1).size (), size_t (0));
    EXPECT_EQ (c1.shapes (0).size (), size_t (0));
    EXPECT_EQ (c1.shapes (1).size (), size_t (1));

    EXPECT_EQ (c0.shapes (0).begin (db::ShapeIterator::All)->to_string (), "box (0,250;2500,3000) props={id=>17}");
    EXPECT_EQ (c1.shapes (1).begin (db::ShapeIterator::All)->to_string (), "box (0,100;1000,1200)");

    m.undo ();

    EXPECT_EQ (c1.cell_instances (), size_t (0));
    EXPECT_EQ (c0.cell_instances (), size_t (1));
    EXPECT_EQ (c0.begin ()->to_string (), "cell_index=1 r0 100,-100 props={id=>5}");

    EXPECT_EQ (c0.shapes (0).size (), size_t (1));
    EXPECT_EQ (c0.shapes (1).size (), size_t (0));
    EXPECT_EQ (c1.shapes (0).size (), size_t (0));
    EXPECT_EQ (c1.shapes (1).size (), size_t (1));

    EXPECT_EQ (c0.shapes (0).begin (db::ShapeIterator::All)->to_string (), "box (0,100;1000,1200) props={id=>17}");
    EXPECT_EQ (c1.shapes (1).begin (db::ShapeIterator::All)->to_string (), "box (0,100;1000,1200)");

    m.redo ();

    EXPECT_EQ (c1.cell_instances (), size_t (0));
    EXPECT_EQ (c0.cell_instances (), size_t (1));
    EXPECT_EQ (c0.begin ()->to_string (), "cell_index=1 r0 *2.5 250,-250 props={id=>5}");

    EXPECT_EQ (c0.shapes (0).size (), size_t (1));
    EXPECT_EQ (c0.shapes (1).size (), size_t (0));
    EXPECT_EQ (c1.shapes (0).size (), size_t (0));
    EXPECT_EQ (c1.shapes (1).size (), size_t (1));

    EXPECT_EQ (c0.shapes (0).begin (db::ShapeIterator::All)->to_string (), "box (0,250;2500,3000) props={id=>17}");
    EXPECT_EQ (c1.shapes (1).begin (db::ShapeIterator::All)->to_string (), "box (0,100;1000,1200)");

  }
}

struct map1
{
  db::cell_index_type operator() (db::cell_index_type i) const { return 3-i; }
};

TEST(4) 
{
  ::pi = 0;

  db::PropertiesSet props;
  props.insert (tl::Variant ("id"), 1);
  db::properties_id_type pid1 = db::properties_id (props);

  props.clear ();
  props.insert (tl::Variant ("id"), 10);
  db::properties_id_type pid10 = db::properties_id (props);

  db::Manager m (true);
  db::Layout g (&m);
  db::Cell &c0 (g.cell (g.add_cell ()));
  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));

  db::Box b (0, 100, 1000, 1200);
  c0.shapes (0).insert (b);
  c1.shapes (1).insert (b);
  c2.shapes (2).insert (b);

  db::Trans t1;
  db::Trans t2 (db::Vector (100, -100));
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), t1));
  c0.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (c1.cell_index ()), t1), pid1));
  c0.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (c2.cell_index ()), t2), pid10));

  EXPECT_EQ (c2s_unsorted(c0), "1[r0 *1 0,0]#{},1[r0 *1 0,0]#{id=>1},2[r0 *1 100,-100]#{id=>10}");

  db::Cell &c3 (g.cell (g.add_cell ()));
  for (db::Cell::const_iterator i = c0.begin (); ! i.at_end (); ++i) {
    c3.insert (*i);
  }
  EXPECT_EQ (c2s_unsorted(c3), "1[r0 *1 0,0]#{},1[r0 *1 0,0]#{id=>1},2[r0 *1 100,-100]#{id=>10}");

  db::Cell &c4 (g.cell (g.add_cell ()));
  for (db::Cell::const_iterator i = c0.begin (); ! i.at_end (); ++i) {
    map1 m1;
    c4.insert (*i, m1);
  }
  EXPECT_EQ (c2s_unsorted(c4), "2[r0 *1 0,0]#{},2[r0 *1 0,0]#{id=>1},1[r0 *1 100,-100]#{id=>10}");

}

TEST(5) 
{
  db::Manager m (true);
  db::Layout g (&m);
  db::Cell &c0 (g.cell (g.add_cell ()));
  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));
  db::Cell &c3 (g.cell (g.add_cell ()));
  db::Cell &c4 (g.cell (g.add_cell ()));

  db::Box b (0, 100, 1000, 1200);
  c0.shapes (0).insert (b);
  c1.shapes (0).insert (b);
  c2.shapes (0).insert (b);
  c3.shapes (0).insert (b);
  c4.shapes (0).insert (b);

  db::Cell *cells[] = { &c1, &c2, &c3, &c4 };

  db::Trans trans [] = { db::Trans (), 
                         db::Trans (1, db::Vector (100, -200)),
                         db::Trans (6, db::Vector (-20, 1000)) };

  db::Trans tt;
  for (unsigned int p = 0; p < 1000; ++p) {
    if ((p % 17) == 0) {
      c0.insert (db::CellInstArray (db::CellInst (cells[(p * 23) % (sizeof (cells) / sizeof (cells [0]))]->cell_index ()), tt));
    } else {
      //  NOTE: we don't use the properties ID, so they can be any number
      c0.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (cells[(p * 23) % (sizeof (cells) / sizeof (cells [0]))]->cell_index ()), tt), p % 17));
    }
    tt *= trans[p % (sizeof (trans) / sizeof (trans [0]))];
  }

  g.update ();

  std::string r;
  for (db::Cell::child_cell_iterator cc = c0.begin_child_cells (); ! cc.at_end (); ++cc) {
    if (r != "") {
      r += ",";
    }
    r += tl::to_string (*cc);
  }

  EXPECT_EQ (r, "1,2,3,4");
}

TEST(6) 
{
  db::PropertiesSet props;
  props.insert (tl::Variant ("id"), 1);
  db::properties_id_type pid1 = db::properties_id (props);

  props.clear ();
  props.insert (tl::Variant ("id"), 2);
  db::properties_id_type pid2 = db::properties_id (props);

  props.clear ();
  props.insert (tl::Variant ("id"), 3);
  db::properties_id_type pid3 = db::properties_id (props);

  props.clear ();
  props.insert (tl::Variant ("id"), 4);
  db::properties_id_type pid4 = db::properties_id (props);

  db::Manager m (true);
  db::Layout g (&m);
  db::Cell &c0 (g.cell (g.add_cell ()));
  db::Cell &c1 (g.cell (g.add_cell ()));
  db::Cell &c2 (g.cell (g.add_cell ()));
  db::Cell &c3 (g.cell (g.add_cell ()));

  db::Trans tt;
  c0.insert (db::CellInstArray (db::CellInst (c1.cell_index ()), tt));
  c0.insert (db::CellInstArray (db::CellInst (c2.cell_index ()), tt));
  c0.insert (db::CellInstArray (db::CellInst (c3.cell_index ()), tt));
  c0.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (c1.cell_index ()), tt), 0));
  c0.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (c2.cell_index ()), tt), pid1));
  c0.insert (db::CellInstArrayWithProperties (db::CellInstArray (db::CellInst (c3.cell_index ()), tt), pid2));

  g.update ();

  std::string r;
  for (db::Cell::child_cell_iterator cc = c0.begin_child_cells (); ! cc.at_end (); ++cc) {
    if (r != "") {
      r += ",";
    }
    r += tl::to_string (*cc);
  }

  EXPECT_EQ (c2s_unsorted(c0), "1[r0 *1 0,0]#{},2[r0 *1 0,0]#{},3[r0 *1 0,0]#{},1[r0 *1 0,0]#{},2[r0 *1 0,0]#{id=>1},3[r0 *1 0,0]#{id=>2}");

  db::Cell &cc (g.cell (g.add_cell ()));
  cc = c0;

  EXPECT_EQ (c2s_unsorted(cc), "1[r0 *1 0,0]#{},2[r0 *1 0,0]#{},3[r0 *1 0,0]#{},1[r0 *1 0,0]#{},2[r0 *1 0,0]#{id=>1},3[r0 *1 0,0]#{id=>2}");

  //  Note: iterating and replace does not work in non-editable mode
  if (db::default_editable_mode ()) {

    std::vector<db::Instance> insts;
    for (db::Cell::const_iterator i = cc.begin (); ! i.at_end (); ++i) {
      insts.push_back (*i);
    }
    for (db::Cell::const_iterator i = cc.begin (); ! i.at_end (); ++i) {
      cc.replace_prop_id (*i, pid3);
    }

    EXPECT_EQ (c2s_unsorted(cc), "1[r0 *1 0,0]#{id=>3},2[r0 *1 0,0]#{id=>3},3[r0 *1 0,0]#{id=>3},1[r0 *1 0,0]#{id=>3},2[r0 *1 0,0]#{id=>3},3[r0 *1 0,0]#{id=>3}");

    for (db::Cell::const_iterator i = c0.begin (); ! i.at_end (); ++i) {
      c0.replace (*i, db::CellInstArrayWithProperties (i->cell_inst (), pid4));
    }

    EXPECT_EQ (c2s_unsorted(c0), "1[r0 *1 0,0]#{id=>4},2[r0 *1 0,0]#{id=>4},3[r0 *1 0,0]#{id=>4},1[r0 *1 0,0]#{id=>4},2[r0 *1 0,0]#{id=>4},3[r0 *1 0,0]#{id=>4}");

  }

}

TEST(10_HasShapesTouching)
{
  db::Layout ly;
  unsigned int l1 = ly.insert_layer (db::LayerProperties (1, 0));

  db::Cell &a = ly.cell (ly.add_cell ("A"));

  EXPECT_EQ (a.has_shapes_touching (l1, db::Box ()), false);

  a.shapes (l1).insert (db::Box (-100, -100, 0, 0));

  EXPECT_EQ (a.has_shapes_touching (l1, db::Box ()), false);
  EXPECT_EQ (a.has_shapes_touching (l1, db::Box (0, 0, 100, 100)), true);
  EXPECT_EQ (a.has_shapes_touching (l1, db::Box (0, 1, 100, 100)), false);
  EXPECT_EQ (a.has_shapes_touching (l1, db::Box (0, -1, 100, 100)), true);
  EXPECT_EQ (a.has_shapes_touching (l1, db::Box (-1, -1, -1, -1)), true);
  EXPECT_EQ (a.has_shapes_touching (l1, db::Box (1, 1, 1, 1)), false);
}

TEST(11_HasShapesTouchingWithHier)
{
  db::Layout ly;
  unsigned int l1 = ly.insert_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.insert_layer (db::LayerProperties (2, 0));

  db::Cell &a = ly.cell (ly.add_cell ("A"));
  db::Cell &b = ly.cell (ly.add_cell ("B"));

  a.insert (db::CellInstArray (b.cell_index (), db::Trans (db::Vector (100, 100)), db::Vector (0, 200), db::Vector (200, 0), 2, 2));

  EXPECT_EQ (a.has_shapes_touching (l1, db::Box ()), false);
  EXPECT_EQ (a.has_shapes_touching (l2, db::Box ()), false);

  b.shapes (l1).insert (db::Box (0, 0, 10, 10));

  EXPECT_EQ (a.has_shapes_touching (l1, db::Box ()), false);
  EXPECT_EQ (a.has_shapes_touching (l1, db::Box (0, 0, 100, 100)), true);
  EXPECT_EQ (a.has_shapes_touching (l2, db::Box (0, 0, 100, 100)), false);
  EXPECT_EQ (a.has_shapes_touching (l1, db::Box (0, 0, 99, 100)), false);
  EXPECT_EQ (a.has_shapes_touching (l1, db::Box (0, 0, 100, 99)), false);
  EXPECT_EQ (a.has_shapes_touching (l1, db::Box (100, 100, 110, 110)), true);
  EXPECT_EQ (a.has_shapes_touching (l1, db::Box (150, 150, 160, 160)), false);
  EXPECT_EQ (a.has_shapes_touching (l1, db::Box (300, 300, 310, 310)), true);
  EXPECT_EQ (a.has_shapes_touching (l1, db::Box (300, 100, 310, 110)), true);
  EXPECT_EQ (a.has_shapes_touching (l1, db::Box (300, 400, 310, 410)), false);
}
