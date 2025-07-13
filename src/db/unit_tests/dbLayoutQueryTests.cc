
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


#include "tlUnitTest.h"
#include "dbLayoutQuery.h"
#include "gsiExpression.h"

#include "dbCell.h"
#include "dbLayout.h"
#include "dbPCellDeclaration.h"
#include "dbLibrary.h"
#include "dbLibraryManager.h"
#include "dbReader.h"
#include "tlStream.h"


static std::string q2s_var (db::Layout &g, const std::string &query, const std::string &pname, const char *sep = ",")
{
  db::LayoutQuery q (query);
  db::LayoutQueryIterator iq (q, &g);
  std::string res;
  while (! iq.at_end ()) {
    if (!res.empty ()) {
      res += sep;
    }
    tl::Variant v;
    iq.get (pname, v);
    res += v.to_string ();
    ++iq;
  }
  return res;
}

static std::string q2s_var_skip (db::LayoutQueryIterator &iq, const std::string &pname, const char *sep = ",")
{
  iq.reset ();
  std::string res;
  while (! iq.at_end ()) {
    if (!res.empty ()) {
      res += sep;
    }
    tl::Variant v;
    iq.get (pname, v);
    res += v.to_string ();
    iq.next (true);
  }
  return res;
}

static std::string q2s_var (db::LayoutQueryIterator &iq, const std::string &pname, const char *sep = ",")
{
  iq.reset ();
  std::string res;
  while (! iq.at_end ()) {
    if (!res.empty ()) {
      res += sep;
    }
    tl::Variant v;
    iq.get (pname, v);
    res += v.to_string ();
    ++iq;
  }
  return res;
}

static std::string q2s_expr (db::LayoutQueryIterator &iq, const std::string &es)
{
  std::string res;
  iq.reset ();
  tl::Expression ex;
  iq.eval ().parse (ex, es, true);
  while (! iq.at_end ()) {
    if (!res.empty ()) {
      res += ",";
    }
    res += ex.execute().to_string ();
    ++iq;
  }
  return res;
}

static std::string q2s_cell (db::LayoutQueryIterator &iq, const std::string &pname)
{
  iq.reset ();
  std::string res;
  while (! iq.at_end ()) {
    if (!res.empty ()) {
      res += ",";
    }
    tl::Variant v;
    iq.get (pname, v);
    if (v.is_nil ()) {
      res += v.to_string ();
    } else {
      res += iq.layout ()->cell_name (v.to_ulong ());
    }
    ++iq;
  }
  return res;
}

TEST(0)
{
  //  FilterStateObjectives tests
  db::FilterStateObjectives o1;

  EXPECT_EQ (o1.wants_all_cells (), false);
  o1.set_wants_all_cells (true);
  EXPECT_EQ (o1.wants_cell (db::cell_index_type (17)), true);
  EXPECT_EQ (o1.wants_all_cells (), true);

  o1.set_wants_all_cells (false);
  o1.request_cell (db::cell_index_type (17));
  EXPECT_EQ (o1.wants_all_cells (), false);
  EXPECT_EQ (o1.wants_cell (db::cell_index_type (17)), true);
  EXPECT_EQ (o1.wants_cell (db::cell_index_type (16)), false);

  db::FilterStateObjectives o2 = o1;

  o1.set_wants_all_cells (false);
  EXPECT_EQ (o1.wants_cell (db::cell_index_type (17)), false);

  EXPECT_EQ (o2.wants_cell (db::cell_index_type (17)), true);

  db::FilterStateObjectives o3 = o2;

  EXPECT_EQ (o3.wants_cell (db::cell_index_type (17)), true);
  o3 += db::FilterStateObjectives::everything ();
  EXPECT_EQ (o3.wants_all_cells (), true);

  o3 = db::FilterStateObjectives::everything ();
  EXPECT_EQ (o3.wants_all_cells (), true);
  o3 += o2;
  EXPECT_EQ (o3.wants_all_cells (), true);

  o3 = db::FilterStateObjectives ();
  EXPECT_EQ (o3.wants_all_cells (), false);
  o3.request_cell (db::cell_index_type (16));
  EXPECT_EQ (o3.wants_cell (db::cell_index_type (17)), false);
  EXPECT_EQ (o3.wants_cell (db::cell_index_type (16)), true);
  o3 += o2;
  EXPECT_EQ (o3.wants_all_cells (), false);
  EXPECT_EQ (o3.wants_cell (db::cell_index_type (17)), true);
  EXPECT_EQ (o3.wants_cell (db::cell_index_type (16)), true);
}

TEST(1)
{
  db::Layout g;
  g.insert_layer (0);
  g.insert_layer (1);
  db::Cell &c1 (g.cell (g.add_cell ("c1")));
  db::Cell &c2 (g.cell (g.add_cell ("c2x")));
  db::Cell &c3 (g.cell (g.add_cell ("c3")));
  db::Cell &c4 (g.cell (g.add_cell ("c4")));
  db::Cell &c5 (g.cell (g.add_cell ("c5x")));
  c2.shapes (0).insert (db::Box (0, 1, 2, 3));

  db::FTrans f (1, true);
  db::Vector p (-10, 20);
  db::Trans t (f.rot (), p);
  db::Vector pp (10, -20);
  db::Trans tt (0, pp);

  //  c4->c1 (aref)
  c4.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t, db::Vector(1, 1), db::Vector (0, 2), 2, 3));
  //  c5->c1
  c5.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  //  c3->c5 (3x)
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), tt));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), t));
  //  c4->c3
  c4.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c3.cell_index ()), t));
  //  c4->c1
  c4.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), tt));
  //  c2->c1 (2x)
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), tt));
  //  c2->c4 (2x)
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c4.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c4.cell_index ()), t));
  //  c2->c5 (2x)
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), tt));

  {
    db::LayoutQuery q ("*");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c2x,c4,c3,c5x,c1");
    s = q2s_cell (iq, "cell_index");
    EXPECT_EQ (s, "c2x,c4,c3,c5x,c1");
    s = q2s_expr (iq, "cell.name");
    EXPECT_EQ (s, "c2x,c4,c3,c5x,c1");
    s = q2s_var (iq, "initial_cell_name");
    EXPECT_EQ (s, "c2x,c4,c3,c5x,c1");
    s = q2s_expr (iq, "initial_cell.name");
    EXPECT_EQ (s, "c2x,c4,c3,c5x,c1");
    s = q2s_cell (iq, "initial_cell_index");
    EXPECT_EQ (s, "c2x,c4,c3,c5x,c1");
  }

  {
    db::LayoutQuery q ("*x");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c2x,c5x");
    s = q2s_cell (iq, "cell_index");
    EXPECT_EQ (s, "c2x,c5x");
    s = q2s_cell (iq, "initial_cell_index");
    EXPECT_EQ (s, "c2x,c5x");
    s = q2s_var (iq, "instances");
    EXPECT_EQ (s, "1,8");
    s = q2s_var (iq, "bbox");
    EXPECT_EQ (s, "(0,1;2,3),()");
    s = q2s_var (iq, "dbbox");
    EXPECT_EQ (s, "(0,0.001;0.002,0.003),()");
  }

  {
    db::LayoutQuery q (".*");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c2x");
    s = q2s_cell (iq, "cell_index");
    EXPECT_EQ (s, "c2x");
    s = q2s_cell (iq, "initial_cell_index");
    EXPECT_EQ (s, "c2x");
    s = q2s_expr (iq, "initial_cell.name");
    EXPECT_EQ (s, "c2x");
    s = q2s_var (iq, "initial_cell_name");
    EXPECT_EQ (s, "c2x");
    s = q2s_cell (iq, "parent_cell_index");
    EXPECT_EQ (s, "nil");
    s = q2s_var (iq, "parent_cell_name");
    EXPECT_EQ (s, "nil");
    s = q2s_var (iq, "path_names");
    EXPECT_EQ (s, "(c2x)");
    s = q2s_var (iq, "path");
    EXPECT_EQ (s, "(1)");
    s = q2s_var (iq, "hier_levels");
    EXPECT_EQ (s, "0");
  }

  {
    //  all cells one level below the top cell
    db::LayoutQuery q (".*.*");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c4,c5x");
    s = q2s_expr (iq, "cell.name");
    EXPECT_EQ (s, "c1,c4,c5x");
    bool error = false;
    try {
      //  errors: cannot call non-const method on const reference
      s = q2s_expr (iq, "cell.name='hallo'");
    } catch (...) {
      error = true;
    }
    EXPECT_EQ (error, true);
    s = q2s_expr (iq, "cell.name");
    EXPECT_EQ (s, "c1,c4,c5x");
    s = q2s_cell (iq, "cell_index");
    EXPECT_EQ (s, "c1,c4,c5x");
    s = q2s_cell (iq, "initial_cell_index");
    EXPECT_EQ (s, "c2x,c2x,c2x");
    s = q2s_var (iq, "initial_cell_name");
    EXPECT_EQ (s, "c2x,c2x,c2x");
    s = q2s_cell (iq, "parent_cell_index");
    EXPECT_EQ (s, "c2x,c2x,c2x");
    s = q2s_var (iq, "parent_cell_name");
    EXPECT_EQ (s, "c2x,c2x,c2x");
    s = q2s_expr (iq, "parent_cell.name");
    EXPECT_EQ (s, "c2x,c2x,c2x");
    s = q2s_var (iq, "path_names");
    EXPECT_EQ (s, "(c2x,c1),(c2x,c4),(c2x,c5x)");
    s = q2s_var (iq, "hier_levels");
    EXPECT_EQ (s, "1,1,1");
  }

  {
    //  all cells one level below the top cell
    db::LayoutQuery q (".c2x.*");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c4,c5x");
  }

  {
    //  invalid top cell
    db::LayoutQuery q (".x.*");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "");
  }

  {
    //  all cells two levels below the top cell
    db::LayoutQuery q (".*.*.*");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c3,c1");
  }

  {
    //  all cells two levels below the top cell
    db::LayoutQuery q (".*.c4.*");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c3");
  }

  {
    db::LayoutQuery q ("cell (.*)[3]");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c3,c1");
  }

  {
    db::LayoutQuery q ("cell (.*)[1..2]");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c2x,c1,c4,c5x");
    s = q2s_cell (iq, "cell_index");
    EXPECT_EQ (s, "c2x,c1,c4,c5x");
    s = q2s_cell (iq, "initial_cell_index");
    EXPECT_EQ (s, "c2x,c2x,c2x,c2x");
    s = q2s_var (iq, "initial_cell_name");
    EXPECT_EQ (s, "c2x,c2x,c2x,c2x");
    s = q2s_cell (iq, "parent_cell_index");
    EXPECT_EQ (s, "nil,c2x,c2x,c2x");
    s = q2s_var (iq, "parent_cell_name");
    EXPECT_EQ (s, "nil,c2x,c2x,c2x");
    s = q2s_var (iq, "path_names");
    EXPECT_EQ (s, "(c2x),(c2x,c1),(c2x,c4),(c2x,c5x)");
    s = q2s_var (iq, "hier_levels");
    EXPECT_EQ (s, "0,1,1,1");
  }

  {
    db::LayoutQuery q ("cell (.*)(.*)?");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c2x,c1,c4,c5x");
    s = q2s_var (iq, "bbox");
    EXPECT_EQ (s, "(0,1;2,3),(),(),()");
  }

  {
    db::LayoutQuery q ("cell (.*)[0..5]");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "nil,c2x,c1,c4,c1,c3,c5x,c1,c5x,c1");
    s = q2s_var (iq, "references");
    EXPECT_EQ (s, "nil,0,2,2,2,1,3,1,2,1");
    s = q2s_var (iq, "weight");
    EXPECT_EQ (s, "nil,0,2,2,7,1,3,1,2,1");
    s = q2s_var (iq, "tot_weight");
    EXPECT_EQ (s, "nil,0,2,2,14,2,6,6,2,2");
  }

  {
    db::LayoutQuery q ("cell (.*)[0..5] where weight==7");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1");
    s = q2s_var (iq, "references");
    EXPECT_EQ (s, "2");
    s = q2s_var (iq, "weight");
    EXPECT_EQ (s, "7");
    s = q2s_var (iq, "tot_weight");
    EXPECT_EQ (s, "14");
  }

  {
    db::LayoutQuery q ("cell (.*)*");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "nil,c2x,c1,c4,c1,c3,c5x,c1,c5x,c1");
    s = q2s_var (iq, "hier_levels");
    EXPECT_EQ (s, "nil,0,1,1,2,2,3,4,1,2");
  }

  {
    db::LayoutQuery q ("cell (.*)*.c5x");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c5x,c5x");
    s = q2s_cell (iq, "cell_index");
    EXPECT_EQ (s, "c5x,c5x");
    s = q2s_cell (iq, "initial_cell_index");
    EXPECT_EQ (s, "c2x,c2x");
    s = q2s_var (iq, "initial_cell_name");
    EXPECT_EQ (s, "c2x,c2x");
    s = q2s_cell (iq, "parent_cell_index");
    EXPECT_EQ (s, "c2x,c3");
    s = q2s_var (iq, "parent_cell_name");
    EXPECT_EQ (s, "c2x,c3");
    s = q2s_var (iq, "path_names");
    EXPECT_EQ (s, "(c2x,c5x),(c2x,c4,c3,c5x)");
    s = q2s_var (iq, "hier_levels");
    EXPECT_EQ (s, "1,3");
  }

  {
    db::LayoutQuery q ("c2x..c1");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c1,c1,c1");
    s = q2s_cell (iq, "cell_index");
    EXPECT_EQ (s, "c1,c1,c1,c1");
    s = q2s_cell (iq, "initial_cell_index");
    EXPECT_EQ (s, "c2x,c2x,c2x,c2x");
    s = q2s_var (iq, "initial_cell_name");
    EXPECT_EQ (s, "c2x,c2x,c2x,c2x");
    s = q2s_cell (iq, "parent_cell_index");
    EXPECT_EQ (s, "c2x,c4,c5x,c5x");
    s = q2s_var (iq, "parent_cell_name");
    EXPECT_EQ (s, "c2x,c4,c5x,c5x");
    s = q2s_var (iq, "path_names");
    EXPECT_EQ (s, "(c2x,c1),(c2x,c4,c1),(c2x,c4,c3,c5x,c1),(c2x,c5x,c1)");
    s = q2s_var (iq, "hier_levels");
    EXPECT_EQ (s, "1,2,4,2");
    s = q2s_var (iq, "references");
    EXPECT_EQ (s, "2,2,1,1");
    s = q2s_var (iq, "weight");
    EXPECT_EQ (s, "2,7,1,1");
    s = q2s_var (iq, "tot_weight");
    EXPECT_EQ (s, "2,14,6,2");
  }

  {
    db::LayoutQuery q ("c2x...c1");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c1,c1,c1");
    s = q2s_cell (iq, "cell_index");
    EXPECT_EQ (s, "c1,c1,c1,c1");
    s = q2s_cell (iq, "initial_cell_index");
    EXPECT_EQ (s, "c2x,c2x,c2x,c2x");
    s = q2s_var (iq, "initial_cell_name");
    EXPECT_EQ (s, "c2x,c2x,c2x,c2x");
    s = q2s_cell (iq, "parent_cell_index");
    EXPECT_EQ (s, "c2x,c4,c5x,c5x");
    s = q2s_var (iq, "parent_cell_name");
    EXPECT_EQ (s, "c2x,c4,c5x,c5x");
    s = q2s_var (iq, "path_names");
    EXPECT_EQ (s, "(c2x,c1),(c2x,c4,c1),(c2x,c4,c3,c5x,c1),(c2x,c5x,c1)");
    s = q2s_var (iq, "hier_levels");
    EXPECT_EQ (s, "1,2,4,2");
    s = q2s_var (iq, "references");
    EXPECT_EQ (s, "2,2,1,1");
    s = q2s_var (iq, "weight");
    EXPECT_EQ (s, "2,7,1,1");
    s = q2s_var (iq, "tot_weight");
    EXPECT_EQ (s, "2,14,6,2");
  }

  {
    db::LayoutQuery q ("c2x c1");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1");
    s = q2s_cell (iq, "cell_index");
    EXPECT_EQ (s, "c1");
    s = q2s_cell (iq, "initial_cell_index");
    EXPECT_EQ (s, "c1");
    s = q2s_var (iq, "initial_cell_name");
    EXPECT_EQ (s, "c1");
    s = q2s_cell (iq, "parent_cell_index");
    EXPECT_EQ (s, "nil");
    s = q2s_var (iq, "parent_cell_name");
    EXPECT_EQ (s, "nil");
    s = q2s_var (iq, "path_names");
    EXPECT_EQ (s, "(c1)");
    s = q2s_var (iq, "hier_levels");
    EXPECT_EQ (s, "0");
    s = q2s_var (iq, "references");
    EXPECT_EQ (s, "0");
    s = q2s_var (iq, "weight");
    EXPECT_EQ (s, "0");
    s = q2s_var (iq, "tot_weight");
    EXPECT_EQ (s, "0");
    s = q2s_var (iq, "instances");
    EXPECT_EQ (s, "24");
  }

  {
    //  all cells one level below the top cell with an expression for the top cell
    db::LayoutQuery q (".$('c2'+'x').*");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c4,c5x");
  }

  {
    //  $_ is a placeholder for the current cell
    db::LayoutQuery q ("$_.*");
    db::LayoutQueryIterator iq (q, &g, &g.cell (g.cell_by_name ("c4").second));
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c3"); // child cells of "c4"
  }

  {
    //  Another way of saying "c2x.*"
    db::LayoutQuery q ("*.$(cell_name=='c2x'?'*':'')");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c4,c5x");
  }
}

TEST(2) 
{
  db::Layout g;
  g.insert_layer (0);
  g.insert_layer (1);
  db::Cell &c1 (g.cell (g.add_cell ("c1")));
  db::Cell &c2 (g.cell (g.add_cell ("c2x")));
  db::Cell &c3 (g.cell (g.add_cell ("c3")));
  db::Cell &c4 (g.cell (g.add_cell ("c4")));
  db::Cell &c5 (g.cell (g.add_cell ("c5x")));
  c2.shapes (0).insert (db::Box (0, 1, 2, 3));
  c1.shapes (1).insert (db::Box (0, 10, 10, 30));

  db::FTrans f (1, true);
  db::Vector p (-10, 20);
  db::Trans t (f.rot (), p);
  db::Vector pp (10, -20);
  db::Trans tt (0, pp);

  //  c4->c1 (aref)
  c4.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t, db::Vector(1, 1), db::Vector (0, 2), 2, 3));
  //  c5->c1
  c5.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  //  c3->c5 (3x)
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), tt));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), t));
  //  c4->c3
  c4.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c3.cell_index ()), t));
  //  c4->c1
  c4.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), tt));
  //  c2->c1 (2x)
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), tt));
  //  c2->c4 (2x)
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c4.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c4.cell_index ()), t));
  //  c2->c5 (2x)
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), tt));

  {
    db::LayoutQuery q ("instances of c2x.c1");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c1");
    s = q2s_var (iq, "trans");
    EXPECT_EQ (s, "r0 *1 10,-20,m45 *1 -10,20");
    s = q2s_var (iq, "dtrans");
    EXPECT_EQ (s, "r0 *1 0.01,-0.02,m45 *1 -0.01,0.02");
    s = q2s_var (iq, "path_trans");
    EXPECT_EQ (s, "r0 *1 10,-20,m45 *1 -10,20");
    s = q2s_var (iq, "path_dtrans");
    EXPECT_EQ (s, "r0 *1 0.01,-0.02,m45 *1 -0.01,0.02");
    s = q2s_var (iq, "inst_bbox");
    EXPECT_EQ (s, "(10,-10;20,10),(0,20;20,30)");
    s = q2s_var (iq, "inst_dbbox");
    EXPECT_EQ (s, "(0.01,-0.01;0.02,0.01),(0,0.02;0.02,0.03)");
    s = q2s_var (iq, "inst");
    EXPECT_EQ (s, "cell_index=0 r0 10,-20,cell_index=0 m45 -10,20");
    s = q2s_var (iq, "array_a");
    EXPECT_EQ (s, "nil,nil");
    s = q2s_var (iq, "array_da");
    EXPECT_EQ (s, "nil,nil");
    s = q2s_var (iq, "array_b");
    EXPECT_EQ (s, "nil,nil");
    s = q2s_var (iq, "array_db");
    EXPECT_EQ (s, "nil,nil");
    s = q2s_var (iq, "array_na");
    EXPECT_EQ (s, "nil,nil");
    s = q2s_var (iq, "array_nb");
    EXPECT_EQ (s, "nil,nil");
    s = q2s_var (iq, "array_ia");
    EXPECT_EQ (s, "-1,-1");
    s = q2s_var (iq, "array_ia");
    EXPECT_EQ (s, "-1,-1");
  }

  {
    db::LayoutQuery q ("instances of c4.c1");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c1,c1,c1,c1,c1,c1");
    s = q2s_var (iq, "trans");
    EXPECT_EQ (s, "r0 *1 10,-20,m45 *1 -10,20,m45 *1 -9,21,m45 *1 -10,22,m45 *1 -9,23,m45 *1 -10,24,m45 *1 -9,25");
    s = q2s_var (iq, "path_trans");
    EXPECT_EQ (s, "r0 *1 10,-20,m45 *1 -10,20,m45 *1 -9,21,m45 *1 -10,22,m45 *1 -9,23,m45 *1 -10,24,m45 *1 -9,25");
    s = q2s_var (iq, "inst_bbox");
    EXPECT_EQ (s, "(10,-10;20,10),(0,20;20,30),(1,21;21,31),(0,22;20,32),(1,23;21,33),(0,24;20,34),(1,25;21,35)");
    s = q2s_var (iq, "inst");
    EXPECT_EQ (s, "cell_index=0 r0 10,-20,cell_index=0 m45 -10,20 array=(1,1,0,2 2x3),cell_index=0 m45 -10,20 array=(1,1,0,2 2x3),cell_index=0 m45 -10,20 array=(1,1,0,2 2x3),cell_index=0 m45 -10,20 array=(1,1,0,2 2x3),cell_index=0 m45 -10,20 array=(1,1,0,2 2x3),cell_index=0 m45 -10,20 array=(1,1,0,2 2x3)");
    s = q2s_var (iq, "array_a");
    EXPECT_EQ (s, "nil,1,1,1,1,1,1,1,1,1,1,1,1");
    s = q2s_var (iq, "array_da");
    EXPECT_EQ (s, "nil,0.001,0.001,0.001,0.001,0.001,0.001,0.001,0.001,0.001,0.001,0.001,0.001");
    s = q2s_var (iq, "array_b");
    EXPECT_EQ (s, "nil,0,2,0,2,0,2,0,2,0,2,0,2");
    s = q2s_var (iq, "array_db");
    EXPECT_EQ (s, "nil,0,0.002,0,0.002,0,0.002,0,0.002,0,0.002,0,0.002");
    s = q2s_var (iq, "array_na");
    EXPECT_EQ (s, "nil,2,2,2,2,2,2");
    s = q2s_var (iq, "array_nb");
    EXPECT_EQ (s, "nil,3,3,3,3,3,3");
    s = q2s_var (iq, "array_ia");
    EXPECT_EQ (s, "-1,0,1,0,1,0,1");
    s = q2s_var (iq, "array_ib");
    EXPECT_EQ (s, "-1,0,0,1,1,2,2");
  }

  {
    db::LayoutQuery q ("instances of c4.c3");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c3");
    s = q2s_var (iq, "trans");
    EXPECT_EQ (s, "m45 *1 -10,20");
    s = q2s_var (iq, "path_trans");
    EXPECT_EQ (s, "m45 *1 -10,20");
  }

  {
    db::LayoutQuery q ("instances of ...*");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_expr (iq, "inst&&inst.cell.qname");
    EXPECT_EQ (s, "nil,c1,c1,c4,c4,c5x,c5x,c1,c1,c1,c1,c1,c1,c1,c3,c5x,c5x,c5x,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c3,c5x,c5x,c5x,c1,c1,c1,c1,c1");
  }

  {
    db::LayoutQuery q ("arrays of c4.c1");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c1");
    s = q2s_var (iq, "trans");
    EXPECT_EQ (s, "r0 *1 10,-20,m45 *1 -10,20");
    s = q2s_var (iq, "path_trans");
    EXPECT_EQ (s, "r0 *1 10,-20,m45 *1 -10,20");
    s = q2s_var (iq, "inst_bbox");
    EXPECT_EQ (s, "(20,-30;30,-10),(10,20;25,41)");
    s = q2s_var (iq, "inst");
    EXPECT_EQ (s, "cell_index=0 r0 10,-20,cell_index=0 m45 -10,20 array=(1,1,0,2 2x3)");
    s = q2s_var (iq, "array_a");
    EXPECT_EQ (s, "nil,1,1");
    s = q2s_var (iq, "array_b");
    EXPECT_EQ (s, "nil,0,2");
    s = q2s_var (iq, "array_na");
    EXPECT_EQ (s, "nil,2");
    s = q2s_var (iq, "array_nb");
    EXPECT_EQ (s, "nil,3");
    s = q2s_var (iq, "array_ia");
    EXPECT_EQ (s, "nil,nil");
    s = q2s_var (iq, "array_ib");
    EXPECT_EQ (s, "nil,nil");
  }

  {
    db::LayoutQuery q ("arrays of (.*)*.c1");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1");
    s = q2s_var (iq, "parent_cell_name");
    EXPECT_EQ (s, "c2x,c2x,c4,c4,c5x,c5x,c5x,c4,c4,c5x,c5x,c5x,c5x,c5x");
    s = q2s_var (iq, "trans");
    EXPECT_EQ (s, "r0 *1 10,-20,m45 *1 -10,20,r0 *1 10,-20,m45 *1 -10,20,m45 *1 -10,20,m45 *1 -10,20,m45 *1 -10,20,r0 *1 10,-20,m45 *1 -10,20,m45 *1 -10,20,m45 *1 -10,20,m45 *1 -10,20,m45 *1 -10,20,m45 *1 -10,20");
    s = q2s_var (iq, "path_trans");
    EXPECT_EQ (s, "r0 *1 10,-20,m45 *1 -10,20,m45 *1 -30,30,r0 *1 10,10,m45 *1 10,10,r0 *1 20,20,r0 *1 20,20,m45 *1 -30,30,r0 *1 10,10,m45 *1 10,10,r0 *1 20,20,r0 *1 20,20,m45 *1 0,0,r0 *1 10,10");
  }

  {
    db::LayoutQuery q ("arrays of (.*)*.c1 where trans.rot==0");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c1,c1");
    s = q2s_var (iq, "parent_cell_name");
    EXPECT_EQ (s, "c2x,c4,c4");
    s = q2s_var (iq, "trans");
    EXPECT_EQ (s, "r0 *1 10,-20,r0 *1 10,-20,r0 *1 10,-20");
    s = q2s_var (iq, "path_trans");
    EXPECT_EQ (s, "r0 *1 10,-20,m45 *1 -30,30,m45 *1 -30,30");
  }

  {
    db::LayoutQuery q ("arrays of ..'c1' where parent_cell_name=='c4' && trans.rot==0");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c1");
    s = q2s_var (iq, "parent_cell_name");
    EXPECT_EQ (s, "c4,c4");
    s = q2s_var (iq, "trans");
    EXPECT_EQ (s, "r0 *1 10,-20,r0 *1 10,-20");
    s = q2s_var (iq, "path_trans");
    EXPECT_EQ (s, "m45 *1 -30,30,m45 *1 -30,30");
  }
}

TEST(3) 
{
  db::Layout g;
  g.insert_layer (0, db::LayerProperties ("l0"));
  g.insert_layer (1, db::LayerProperties ("l1"));
  g.insert_layer (2, db::LayerProperties ("l2"));
  db::Cell &c1 (g.cell (g.add_cell ("c1")));
  db::Cell &c2 (g.cell (g.add_cell ("c2x")));
  db::Cell &c3 (g.cell (g.add_cell ("c3")));
  db::Cell &c4 (g.cell (g.add_cell ("c4")));
  db::Cell &c5 (g.cell (g.add_cell ("c5x")));
  c2.shapes (0).insert (db::Box (0, 1, 2, 3));
  c2.shapes (1).insert (db::Polygon (db::Box (0, 1, 2, 3)));
  c2.shapes (1).insert (db::Edge (db::Point (0, 1), db::Point (2, 3)));
  c2.shapes (2).insert (db::Text ("hallo", db::Trans (db::Vector (10, 11))));
  c1.shapes (1).insert (db::Box (0, 10, 10, 30));

  db::FTrans f (1, true);
  db::Vector p (-10, 20);
  db::Trans t (f.rot (), p);
  db::Vector pp (10, -20);
  db::Trans tt (0, pp);

  //  c4->c1 (aref)
  c4.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t, db::Vector(1, 1), db::Vector (0, 2), 2, 3));
  //  c5->c1
  c5.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  //  c3->c5 (3x)
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), tt));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), t));
  //  c4->c3
  c4.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c3.cell_index ()), t));
  //  c4->c1
  c4.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), tt));
  //  c2->c1 (2x)
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), tt));
  //  c2->c4 (2x)
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c4.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c4.cell_index ()), t));
  //  c2->c5 (2x)
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), tt));

  {
    db::LayoutQuery q ("shapes of c1");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1");
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,10;10,30)");
  }

  {
    db::LayoutQuery q ("boxes of *");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c2x,c1");
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,1;2,3),box (0,10;10,30)");
    s = q2s_var (iq, "layer_info");
    EXPECT_EQ (s, "l0,l1");
  }

  {
    db::LayoutQuery q ("boxes of * where shape.area > 10");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1");
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,10;10,30)");
    s = q2s_var (iq, "layer_info");
    EXPECT_EQ (s, "l1");
  }

  {
    db::LayoutQuery q ("shapes of c2x");
    db::LayoutQueryIterator iq (q, &g);
    std::string s;
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,1;2,3),polygon (0,1;0,3;2,3;2,1),edge (0,1;2,3),text ('hallo',r0 10,11)");
    s = q2s_var (iq, "layer_info");
    EXPECT_EQ (s, "l0,l1,l1,l2");
    s = q2s_var (iq, "layer_index");
    EXPECT_EQ (s, "0,1,1,2");
    s = q2s_var (iq, "bbox");
    EXPECT_EQ (s, "(0,1;2,3),(0,1;2,3),(0,1;2,3),(10,11;10,11)");
    s = q2s_var (iq, "dbbox");
    EXPECT_EQ (s, "(0,0.001;0.002,0.003),(0,0.001;0.002,0.003),(0,0.001;0.002,0.003),(0.01,0.011;0.01,0.011)");
    s = q2s_var (iq, "shape_bbox");
    EXPECT_EQ (s, "(0,1;2,3),(0,1;2,3),(0,1;2,3),(10,11;10,11)");
    s = q2s_var (iq, "shape_dbbox");
    EXPECT_EQ (s, "(0,0.001;0.002,0.003),(0,0.001;0.002,0.003),(0,0.001;0.002,0.003),(0.01,0.011;0.01,0.011)");
  }

  {
    db::LayoutQuery q ("polygons of c2x");
    db::LayoutQueryIterator iq (q, &g);
    std::string s;
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "polygon (0,1;0,3;2,3;2,1)");
    s = q2s_var (iq, "layer_info");
    EXPECT_EQ (s, "l1");
    s = q2s_var (iq, "layer_index");
    EXPECT_EQ (s, "1");
    s = q2s_var (iq, "bbox");
    EXPECT_EQ (s, "(0,1;2,3)");
  }

  {
    db::LayoutQuery q ("boxes of c2x");
    db::LayoutQueryIterator iq (q, &g);
    std::string s;
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,1;2,3)");
    s = q2s_var (iq, "layer_info");
    EXPECT_EQ (s, "l0");
    s = q2s_var (iq, "layer_index");
    EXPECT_EQ (s, "0");
    s = q2s_var (iq, "bbox");
    EXPECT_EQ (s, "(0,1;2,3)");
  }

  {
    db::LayoutQuery q ("boxes, polygons of c2x");
    db::LayoutQueryIterator iq (q, &g);
    std::string s;
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,1;2,3),polygon (0,1;0,3;2,3;2,1)");
    s = q2s_var (iq, "layer_info");
    EXPECT_EQ (s, "l0,l1");
    s = q2s_var (iq, "layer_index");
    EXPECT_EQ (s, "0,1");
    s = q2s_var (iq, "bbox");
    EXPECT_EQ (s, "(0,1;2,3),(0,1;2,3)");
  }

  {
    db::LayoutQuery q ("paths of c2x");
    db::LayoutQueryIterator iq (q, &g);
    std::string s;
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "");
    s = q2s_var (iq, "layer_info");
    EXPECT_EQ (s, "");
    s = q2s_var (iq, "layer_index");
    EXPECT_EQ (s, "");
    s = q2s_var (iq, "bbox");
    EXPECT_EQ (s, "");
  }

  c4.shapes (2).insert (db::Box (0, -1, 2, 1));

  {
    db::LayoutQuery q ("boxes of c2x.*");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c4");
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,10;10,30),box (0,-1;2,1)");
    s = q2s_var (iq, "layer_info");
    EXPECT_EQ (s, "l1,l2");
  }

  {
    db::LayoutQuery q ("boxes of instances of c2x.*");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c1,c4,c4");
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,10;10,30),box (0,10;10,30),box (0,-1;2,1),box (0,-1;2,1)");
    s = q2s_expr (iq, "bbox.transformed(path_trans)");
    EXPECT_EQ (s, "(10,-10;20,10),(0,20;20,30),(-11,20;-9,22),(-11,20;-9,22)");
    s = q2s_var (iq, "layer_info");
    EXPECT_EQ (s, "l1,l1,l2,l2");
  }

  {
    db::LayoutQuery q ("boxes on 'l1';'l2' of instances of c2x.*");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c1,c4,c4");
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,10;10,30),box (0,10;10,30),box (0,-1;2,1),box (0,-1;2,1)");
    s = q2s_var (iq, "layer_info");
    EXPECT_EQ (s, "l1,l1,l2,l2");
  }

  {
    db::LayoutQuery q ("boxes on 'l2' of instances of c2x.*");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c4,c4");
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,-1;2,1),box (0,-1;2,1)");
    s = q2s_var (iq, "layer_info");
    EXPECT_EQ (s, "l2,l2");
  }

  {
    db::LayoutQuery q ("boxes on 'l0';'l2' of instances of c2x.*");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c4,c4");
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,-1;2,1),box (0,-1;2,1)");
    s = q2s_var (iq, "layer_info");
    EXPECT_EQ (s, "l2,l2");
  }

  {
    db::LayoutQuery q ("boxes of instances of c2x..*");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c1,c4,c4,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1");
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,10;10,30),box (0,10;10,30),box (0,-1;2,1),box (0,-1;2,1),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30)");
  }

  {
    db::LayoutQuery q ("boxes of instances of c2x..* where true");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c1,c4,c4,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1");
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,10;10,30),box (0,10;10,30),box (0,-1;2,1),box (0,-1;2,1),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30)");
  }

  {
    db::LayoutQuery q ("boxes of instances of c2x..");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c2x,c1,c1,c4,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c4,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1");
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,1;2,3),box (0,10;10,30),box (0,10;10,30),box (0,-1;2,1),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,-1;2,1),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30)");
  }

  {
    db::LayoutQuery q ("boxes of instances of c2x.. where true");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c2x,c1,c1,c4,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c4,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1,c1");
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,1;2,3),box (0,10;10,30),box (0,10;10,30),box (0,-1;2,1),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,-1;2,1),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30),box (0,10;10,30)");
  }
}

void init_layout (db::Layout &g)
{
  g = db::Layout ();

  g.insert_layer (0, db::LayerProperties ("l0"));
  g.insert_layer (1, db::LayerProperties ("l1"));
  g.insert_layer (2, db::LayerProperties ("l2"));
  db::Cell &c1 (g.cell (g.add_cell ("c1")));
  db::Cell &c2 (g.cell (g.add_cell ("c2x")));
  db::Cell &c3 (g.cell (g.add_cell ("c3")));
  db::Cell &c4 (g.cell (g.add_cell ("c4")));
  db::Cell &c5 (g.cell (g.add_cell ("c5x")));
  c2.shapes (0).insert (db::Box (0, 1, 2, 3));
  c2.shapes (1).insert (db::Polygon (db::Box (0, 1, 2, 3)));
  c2.shapes (1).insert (db::Edge (db::Point (0, 1), db::Point (2, 3)));
  c2.shapes (2).insert (db::Text ("hallo", db::Trans (db::Vector (10, 11))));
  c1.shapes (1).insert (db::Box (0, 10, 10, 30));

  db::FTrans f (1, true);
  db::Vector p (-10, 20);
  db::Trans t (f.rot (), p);
  db::Vector pp (10, -20);
  db::Trans tt (0, pp);

  //  c4->c1 (aref)
  c4.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t, db::Vector(1, 1), db::Vector (0, 2), 2, 3));
  //  c5->c1
  c5.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  //  c3->c5 (3x)
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), tt));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), t));
  //  c4->c3
  c4.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c3.cell_index ()), t));
  //  c4->c1
  c4.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), tt));
  //  c2->c1 (2x)
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), tt));
  //  c2->c4 (2x)
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c4.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c4.cell_index ()), t));
  //  c2->c5 (2x)
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5.cell_index ()), tt));
}

void init_layout2 (db::Layout &g)
{
  g = db::Layout ();

  tl::InputStream stream (tl::testdata () + "/gds/issue-1671.gds");
  db::Reader reader (stream);
  reader.read (g, db::LoadLayoutOptions ());

  g.insert_layer (0, db::LayerProperties ("l0"));
  g.insert_layer (1, db::LayerProperties ("l1"));
  g.insert_layer (2, db::LayerProperties ("l2"));
  db::Cell &c1 (g.cell (g.add_cell ("c1")));
  db::Cell &c2 (g.cell (g.add_cell ("c2")));
  db::Cell &c3 (g.cell (g.add_cell ("c3")));
  c2.shapes (0).insert (db::Box (0, 1, 2, 3));
  c2.shapes (1).insert (db::Polygon (db::Box (0, 1, 2, 3)));
  c2.shapes (1).insert (db::Edge (db::Point (0, 1), db::Point (2, 3)));
  c2.shapes (2).insert (db::Text ("hallo", db::Trans (db::Vector (10, 11))));
  c1.shapes (1).insert (db::Box (0, 10, 10, 30));

  db::FTrans f (1, true);
  db::Vector p (-10, 20);
  db::Trans t (f.rot (), p);
  db::Vector pp (10, -20);
  db::Trans tt (0, pp);

  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c2.cell_index ()), t));
  c3.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c2.cell_index ()), tt));

  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
}

TEST(4)
{
  db::Layout g;
  init_layout (g);

  {
    db::LayoutQuery q ("select cell_name+'#'+cell_index from *");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "data");
    EXPECT_EQ (s, "(c2x#1),(c4#3),(c3#2),(c5x#4),(c1#0)");
  }
 
  {
    db::LayoutQuery q ("select $1 from 'c(*)'");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "data");
    EXPECT_EQ (s, "(2x),(4),(3),(5x),(1)");
  }
 
  {
    db::LayoutQuery q ("select cell_index+'#'+cell_name from * sorted by cell_name");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "data");
    EXPECT_EQ (s, "(0#c1),(1#c2x),(2#c3),(3#c4),(4#c5x)");
  }
 
  {
    db::LayoutQuery q ("select cell_index+'#'+cell_name from ..* sorted by cell_name");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "data");
    EXPECT_EQ (s, "(0#c1),(0#c1),(0#c1),(0#c1),(1#c2x),(2#c3),(3#c4),(4#c5x),(4#c5x)");
  }
 
  {
    db::LayoutQuery q ("select cell_index+'#'+cell_name from ..* sorted by cell_name unique");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "data");
    EXPECT_EQ (s, "(0#c1),(1#c2x),(2#c3),(3#c4),(4#c5x)");
  }
 
  {
    db::LayoutQuery q ("select cell_index+'#'+cell_name from instances of ..* sorted by cell_name");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "data");
    EXPECT_EQ (s, "(0#c1),(0#c1),(0#c1),(0#c1),(0#c1),(0#c1),(0#c1),(0#c1),(0#c1),(0#c1),(0#c1),(0#c1),(0#c1),(0#c1),(0#c1),(0#c1),(0#c1),(0#c1),(0#c1),(0#c1),(0#c1),(0#c1),(0#c1),(0#c1),(1#c2x),(2#c3),(2#c3),(3#c4),(3#c4),(4#c5x),(4#c5x),(4#c5x),(4#c5x),(4#c5x),(4#c5x),(4#c5x),(4#c5x)");
  }
 
  {
    db::LayoutQuery q ("select cell_index+'#'+cell_name from instances of ..* sorted by cell_name unique");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "data");
    EXPECT_EQ (s, "(0#c1),(1#c2x),(2#c3),(3#c4),(4#c5x)");
  }
}

TEST(51a) 
{
  if (! db::default_editable_mode ()) { return; }

  db::Layout g;
  init_layout (g);

  {
    db::LayoutQuery ("delete cell *x").execute (g);
    db::LayoutQuery q ("select cell_name+'#'+cell_index from *");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "data");
    EXPECT_EQ (s, "(c4#3),(c1#0),(c3#2)");
  }

  {
    db::LayoutQuery ("delete cell *").execute (g);
    db::LayoutQuery q ("select cell_name+'#'+cell_index from *");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "data");
    EXPECT_EQ (s, "");
  }
}

TEST(51b) 
{
  if (! db::default_editable_mode ()) { return; }

  db::Layout g;
  init_layout (g);

  {
    db::LayoutQuery q ("delete cell *x");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "");
  }
}

TEST(51c) 
{
  if (! db::default_editable_mode ()) { return; }

  db::Layout g;
  init_layout (g);

  {
    db::LayoutQuery q ("delete cell *x pass");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var_skip (iq, "cell_name");
    EXPECT_EQ (s, "c2x,c5x");
  }

  {
    db::LayoutQuery q ("delete cell *x pass");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c2x,c5x");
  }

  {
    db::LayoutQuery q ("delete cell *x pass");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "");
  }
}

TEST(52a) 
{
  if (! db::default_editable_mode ()) { return; }

  db::Layout g;
  init_layout (g);

  {
    db::LayoutQuery q ("shapes of c2x");
    db::LayoutQueryIterator iq (q, &g);
    std::string s;
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,1;2,3),polygon (0,1;0,3;2,3;2,1),edge (0,1;2,3),text ('hallo',r0 10,11)");
    s = q2s_var (iq, "layer_info");
    EXPECT_EQ (s, "l0,l1,l1,l2");
  }

  {
    db::LayoutQuery q ("shapes on layer l0,l1 from c2x");
    db::LayoutQueryIterator iq (q, &g);
    std::string s;
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,1;2,3),polygon (0,1;0,3;2,3;2,1),edge (0,1;2,3)");
  }

  {
    db::LayoutQuery ("delete shapes on layer l1 from *").execute (g);
    db::LayoutQuery q ("shapes of c2x");
    db::LayoutQueryIterator iq (q, &g);
    std::string s;
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,1;2,3),text ('hallo',r0 10,11)");
  }

  {
    init_layout (g);
    db::LayoutQuery ("delete shapes on layer l1 from c4").execute (g);
    db::LayoutQuery q ("shapes of c2x");
    db::LayoutQueryIterator iq (q, &g);
    std::string s;
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,1;2,3),polygon (0,1;0,3;2,3;2,1),edge (0,1;2,3),text ('hallo',r0 10,11)");
    db::LayoutQuery ("delete polygons from *").execute (g);
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,1;2,3),edge (0,1;2,3),text ('hallo',r0 10,11)");
  }

}

TEST(52b) 
{
  if (! db::default_editable_mode ()) { return; }

  db::Layout g;
  init_layout (g);

  {
    db::LayoutQuery q ("delete shapes on layer l1 from * pass");
    db::LayoutQueryIterator iq (q, &g);
    std::string s;
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "polygon (0,1;0,3;2,3;2,1),edge (0,1;2,3),box (0,10;10,30)");
  }
}

TEST(53) 
{
  if (! db::default_editable_mode ()) { return; }

  db::Manager m;
  db::Layout g (&m);
  init_layout (g);

  {
    db::LayoutQuery q ("cell ..*");
    std::string s;
    db::LayoutQueryIterator iq (q, &g);
    s = q2s_var (iq, "path_names", ";");
    EXPECT_EQ (s, "(c2x);(c2x,c1);(c2x,c4);(c2x,c5x);(c2x,c4,c1);(c2x,c4,c3);(c2x,c4,c3,c5x);(c2x,c4,c3,c5x,c1);(c2x,c5x,c1)");
    db::LayoutQuery ("delete instances of *.c1").execute (g);
    s = q2s_var (iq, "path_names", ";");
    EXPECT_EQ (s, "(c1);(c2x);(c2x,c4);(c2x,c5x);(c2x,c4,c3);(c2x,c4,c3,c5x)");
  }

  init_layout (g);

  {
    std::string s;
    db::LayoutQuery q ("delete instances of *.c1 pass");
    db::LayoutQueryIterator iq (q, &g);
    s = q2s_var (iq, "path_names", ";");
    EXPECT_EQ (s, "(c2x,c1);(c2x,c1);(c4,c1);(c4,c1);(c5x,c1)");
  }

  init_layout (g);

  {
    db::LayoutQuery q ("cell ..*");
    std::string s;
    db::LayoutQueryIterator iq (q, &g);
    s = q2s_var (iq, "path_names", ";");
    EXPECT_EQ (s, "(c2x);(c2x,c1);(c2x,c4);(c2x,c5x);(c2x,c4,c1);(c2x,c4,c3);(c2x,c4,c3,c5x);(c2x,c4,c3,c5x,c1);(c2x,c5x,c1)");
    db::LayoutQuery ("delete instances of c1").execute (g);
    s = q2s_var (iq, "path_names", ";");
    EXPECT_EQ (s, "(c1);(c2x);(c2x,c4);(c2x,c5x);(c2x,c4,c3);(c2x,c4,c3,c5x)");
  }
 
  init_layout (g);

  {
    db::LayoutQuery q ("cell ..*");
    std::string s;
    db::LayoutQueryIterator iq (q, &g);
    s = q2s_var (iq, "path_names", ";");
    EXPECT_EQ (s, "(c2x);(c2x,c1);(c2x,c4);(c2x,c5x);(c2x,c4,c1);(c2x,c4,c3);(c2x,c4,c3,c5x);(c2x,c4,c3,c5x,c1);(c2x,c5x,c1)");
    db::LayoutQuery ("delete instances of *").execute (g);
    s = q2s_var (iq, "path_names", ";");
    EXPECT_EQ (s, "(c1);(c2x);(c3);(c4);(c5x)");
  }

  init_layout (g);

  {
    //  triggers issue-1671 (with transaction)
    db::Transaction trans (&m, "test 53");
    std::string s;
    db::LayoutQuery q ("delete instances of ...c1 pass");
    db::LayoutQueryIterator iq (q, &g);
    s = q2s_var (iq, "path_names", ";");
    EXPECT_EQ (s, "(c2x,c1);(c2x,c1);(c2x,c4,c1);(c2x,c4,c1);(c2x,c4,c3,c5x,c1)");
  }
}

TEST(61)
{
  if (! db::default_editable_mode ()) { return; }

  db::Layout g;
  init_layout (g);

  {
    db::LayoutQuery q ("shapes of c2x");
    db::LayoutQueryIterator iq (q, &g);
    std::string s;
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,1;2,3),polygon (0,1;0,3;2,3;2,1),edge (0,1;2,3),text ('hallo',r0 10,11)");
    s = q2s_var (iq, "layer_info");
    EXPECT_EQ (s, "l0,l1,l1,l2");
    db::LayoutQuery ("with boxes from * do shape.polygon = Polygon.new(shape.bbox)").execute (g);
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "polygon (0,1;0,3;2,3;2,1),polygon (0,1;0,3;2,3;2,1),edge (0,1;2,3),text ('hallo',r0 10,11)");
    db::LayoutQuery ("with polygons from * do shape.box = shape.bbox").execute (g);
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,1;2,3),edge (0,1;2,3),box (0,1;2,3),text ('hallo',r0 10,11)");
    s = q2s_var (iq, "layer_info");
    EXPECT_EQ (s, "l0,l1,l1,l2");
    db::LayoutQuery ("with texts from * do shape.text_string = shape.text_string + 'xx'").execute (g);
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,1;2,3),edge (0,1;2,3),box (0,1;2,3),text ('halloxx',r0 10,11)");
    db::LayoutQuery ("with texts from * where shape.text_string ~ '(*)all(*)' do shape.text_string = $1 + $2").execute (g);
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "box (0,1;2,3),edge (0,1;2,3),box (0,1;2,3),text ('hoxx',r0 10,11)");
  }

  init_layout (g);

  {
    std::string s;

    db::LayoutQuery qq ("shapes from *");
    db::LayoutQueryIterator iqq (qq, &g);
    s = q2s_var (iqq, "shape");
    EXPECT_EQ (s, "box (0,1;2,3),polygon (0,1;0,3;2,3;2,1),edge (0,1;2,3),text ('hallo',r0 10,11),box (0,10;10,30)");
    db::LayoutQuery ("with boxes from * do shape.polygon = Polygon.new(shape.bbox)").execute (g);
    s = q2s_var (iqq, "shape");
    EXPECT_EQ (s, "polygon (0,1;0,3;2,3;2,1),polygon (0,1;0,3;2,3;2,1),edge (0,1;2,3),text ('hallo',r0 10,11),polygon (0,10;0,30;10,30;10,10)");
    db::LayoutQuery q ("with polygons from * do shape.box = shape.bbox pass");
    db::LayoutQueryIterator iq (q, &g);
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "polygon (0,1;0,3;2,3;2,1),polygon (0,1;0,3;2,3;2,1),polygon (0,10;0,30;10,30;10,10)");
    s = q2s_var (iqq, "shape");
    EXPECT_EQ (s, "box (0,1;2,3),edge (0,1;2,3),box (0,1;2,3),text ('hallo',r0 10,11),box (0,10;10,30)");
  }

  init_layout (g);

  {
    std::string s;

    db::LayoutQuery qq ("shapes from *");
    db::LayoutQueryIterator iqq (qq, &g);
    s = q2s_var (iqq, "shape");
    EXPECT_EQ (s, "box (0,1;2,3),polygon (0,1;0,3;2,3;2,1),edge (0,1;2,3),text ('hallo',r0 10,11),box (0,10;10,30)");
    db::LayoutQuery ("with boxes from * do shape.polygon = Polygon.new(shape.bbox)").execute (g);
    s = q2s_var (iqq, "shape");
    EXPECT_EQ (s, "polygon (0,1;0,3;2,3;2,1),polygon (0,1;0,3;2,3;2,1),edge (0,1;2,3),text ('hallo',r0 10,11),polygon (0,10;0,30;10,30;10,10)");
    db::LayoutQuery q ("with polygons from * do shape.box = shape.bbox");
    db::LayoutQueryIterator iq (q, &g);
    s = q2s_var (iq, "shape");
    EXPECT_EQ (s, "");
    s = q2s_var (iqq, "shape");
    EXPECT_EQ (s, "box (0,1;2,3),edge (0,1;2,3),box (0,1;2,3),text ('hallo',r0 10,11),box (0,10;10,30)");
  }

  init_layout (g);

  {
    db::LayoutQuery q ("*");
    db::LayoutQueryIterator iq (q, &g);
    std::string s;
    s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c2x,c4,c3,c5x,c1");
    db::LayoutQuery ("with * do cell.name = 'i' + cell_index").execute (g);
    s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "i1,i3,i2,i4,i0");
  }

  init_layout (g);

  {
    EXPECT_EQ (q2s_var (g, "shapes on l1 from c2x", "shape"), "polygon (0,1;0,3;2,3;2,1),edge (0,1;2,3)");
    EXPECT_EQ (q2s_var (g, "shapes on l1 from c1", "shape"), "box (0,10;10,30)");
    db::LayoutQuery ("with shapes from instances of c2x..* do initial_cell.shapes(1).insert(shape).transform(path_trans)").execute (g);
    db::LayoutQuery ("with shapes from c2x..* do shape.delete").execute (g);
    EXPECT_EQ (q2s_var (g, "shapes on l1 from c2x", "shape"), "polygon (0,1;0,3;2,3;2,1),edge (0,1;2,3),box (10,-10;20,10),box (0,20;20,30),box (-20,30;0,40),box (10,20;20,40),box (20,10;40,20),box (20,30;30,50),box (20,30;30,50),box (-20,30;0,40),box (10,20;20,40),box (20,10;40,20),box (20,30;30,50),box (20,30;30,50),box (10,0;30,10),box (10,20;20,40)");
    EXPECT_EQ (q2s_var (g, "shapes on l1 from c1", "shape"), "");
  }

  init_layout (g);

  {
    g.add_cell ("cx");
    EXPECT_EQ (q2s_var (g, "instances of ..c4", "inst"), "cell_index=3 m45 -10,20,cell_index=3 m45 -10,20");
    db::LayoutQuery ("with instances of ..c4 do inst.cell_index = layout.cell_by_name('cx')").execute (g);
    EXPECT_EQ (q2s_var (g, "instances of ..c4", "inst"), "nil");
    EXPECT_EQ (q2s_var (g, "instances of .*..c4", "inst"), "");
    EXPECT_EQ (q2s_var (g, "instances of ..cx", "inst"), "cell_index=5 m45 -10,20,cell_index=5 m45 -10,20");
    db::LayoutQuery ("delete instances of .*..c4").execute (g);
    db::LayoutQuery ("delete instances of .*..cx").execute (g);
    EXPECT_EQ (q2s_var (g, "instances of .*..c4", "inst"), "");
    EXPECT_EQ (q2s_var (g, "instances of .*..cx", "inst"), "");
  }

  init_layout (g);

  {
    EXPECT_EQ (q2s_var (g, "instances of ..c4", "inst"), "cell_index=3 m45 -10,20,cell_index=3 m45 -10,20");
    db::LayoutQuery ("with instances of ..c4 do inst.cell_index = <<cy>>").execute (g);
    EXPECT_EQ (q2s_var (g, "instances of ..c4", "inst"), "nil");
    EXPECT_EQ (q2s_var (g, "instances of .*..c4", "inst"), "");
    EXPECT_EQ (q2s_var (g, "instances of ..cy", "inst"), "cell_index=5 m45 -10,20,cell_index=5 m45 -10,20");
  }

  init_layout (g);

  {
    EXPECT_EQ (q2s_var (g, "instances of ..c4", "inst"), "cell_index=3 m45 -10,20,cell_index=3 m45 -10,20");
    db::LayoutQuery ("with instances of ..c4 do inst.cell_index = <<\"Basic\" + \".\" + \"TEXT\">>").execute (g);
    EXPECT_EQ (q2s_var (g, "instances of ..c4", "inst"), "nil");
    EXPECT_EQ (q2s_var (g, "instances of .*..c4", "inst"), "");
    EXPECT_EQ (q2s_var (g, "instances of ..\"Basic.*\"", "inst"), "cell_index=5 m45 -10,20,cell_index=5 m45 -10,20");
  }
}

TEST(62)
{
  if (! db::default_editable_mode ()) { return; }

  db::Library *basic_lib = db::LibraryManager::instance ().lib_ptr_by_name ("Basic");
  EXPECT_EQ (basic_lib != 0, true);
  if (!basic_lib) {
    return;
  }

  db::pcell_id_type text_id = basic_lib->layout ().pcell_by_name ("TEXT").second;
  const db::PCellDeclaration *text_decl = basic_lib->layout ().pcell_declaration (text_id);

  std::vector<tl::Variant> values;
  const std::vector<db::PCellParameterDeclaration> &pd = text_decl->get_parameter_declarations ();

  for (std::vector<db::PCellParameterDeclaration>::const_iterator p = pd.begin (); p != pd.end (); ++p) {
    if (p->get_name () == "text") {
      values.push_back (tl::Variant ("T1"));
    } else if (p->get_name () == "layer") {
      values.push_back (tl::Variant (db::LayerProperties (1, 0)));
    } else {
      values.push_back (p->get_default ());
    }
  }

  db::cell_index_type v1t1 = basic_lib->layout ().get_pcell_variant (text_id, values);

  values.clear ();
  for (std::vector<db::PCellParameterDeclaration>::const_iterator p = pd.begin (); p != pd.end (); ++p) {
    if (p->get_name () == "text") {
      values.push_back (tl::Variant ("T2"));
    } else if (p->get_name () == "layer") {
      values.push_back (tl::Variant (db::LayerProperties (2, 0)));
    } else {
      values.push_back (p->get_default ());
    }
  }

  db::cell_index_type v1t2 = basic_lib->layout ().get_pcell_variant (text_id, values);

  db::Layout g;
  init_layout (g);

  size_t c3index = g.get_lib_proxy (basic_lib, v1t1);
  size_t c4index = g.get_lib_proxy (basic_lib, v1t2);

  db::Cell &c1 (g.cell (g.add_cell ("c1")));
  db::Cell &c2 (g.cell (g.add_cell ("c2")));
  db::Cell &c3 (g.cell (db::cell_index_type (c3index)));
  db::Cell &c4 (g.cell (db::cell_index_type (c4index)));

  db::FTrans f (1, true);
  db::Vector p (-10, 20);
  db::Trans t (f.rot (), p);
  db::Vector pp (10, -20);
  db::Trans tt (0, pp);

  //  c1->c3
  c1.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c3.cell_index ()), t));
  //  c2->c4
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c4.cell_index ()), t));
  //  c2->c1 (2x)
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), t));
  c2.insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1.cell_index ()), tt));

  {
    db::LayoutQuery q ("\"Basic.*\"");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "Basic.TEXT,Basic.TEXT");
  }

  {
    db::LayoutQuery q ("instances of ...\"Basic.*\"");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_expr (iq, "inst.cell.display_title");
    EXPECT_EQ (s, "Basic.TEXT(l=2/0,'T2'),Basic.TEXT(l=1/0,'T1'),Basic.TEXT(l=1/0,'T1')");
  }

  {
    db::LayoutQuery q ("select inst.pcell_parameters_by_name[\"text\"] from instances of ...* where cell_name ~ \"Basic.*\"");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "data");
    EXPECT_EQ (s, "(T2),(T1),(T1)");
  }

  {
    db::LayoutQuery q ("select inst.pcell_parameter(\"text\") from instances of ...* where cell_name ~ \"Basic.*\"");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "data");
    EXPECT_EQ (s, "(T2),(T1),(T1)");
  }

  {
    db::LayoutQuery q ("select inst[\"text\"] from instances of ...* where cell_name ~ \"Basic.*\"");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "data");
    EXPECT_EQ (s, "(T2),(T1),(T1)");
  }

  {
    //  A non-executed query
    db::LayoutQuery q ("select inst[\"text\"] from instances of ...* where cell_name ~ \"Basic.*\"");
    db::LayoutQueryIterator iq (q, &g);
    EXPECT_EQ (true, true);
  }
}

TEST(63)
{
  db::Layout g;

  //  A failing query must not leave a layout under construction
  try {
    db::LayoutQuery q ("!not a valid query");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "data");
    EXPECT_EQ (true, false);
  } catch (tl::Exception &ex) {
    EXPECT_EQ (ex.msg (), "Expected a word or quoted string here: !not a val ..");
  }

  EXPECT_EQ (g.under_construction (), false);
}

//  issue-787
TEST(64)
{
  db::Layout g;
  init_layout (g);

  {
    db::LayoutQuery q ("select inst.dtrans from instances of .*.*");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "data");
    EXPECT_EQ (s, "(r0 0.01,-0.02),(m45 -0.01,0.02),(m45 -0.01,0.02),(m45 -0.01,0.02),(r0 0.01,-0.02),(m45 -0.01,0.02)");
  }

  {
    db::LayoutQuery q ("select inst.dtrans.disp.x,inst.dtrans.disp.y from instances of .*.*");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "data");
    EXPECT_EQ (s, "(0.01,-0.02),(-0.01,0.02),(-0.01,0.02),(-0.01,0.02),(0.01,-0.02),(-0.01,0.02)");
  }
}

TEST(65)
{
  db::Layout g;
  init_layout (g);

  {
    db::LayoutQuery q ("instances of cell .*.* where inst.trans.rot == 0");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "data");
    EXPECT_EQ (s, "nil,nil");
    s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c5x");
    s = q2s_var (iq, "inst_elements");
    EXPECT_EQ (s, "(cell_index=0 r0 *1 10,-20),(cell_index=4 r0 *1 10,-20)");
  }
}

TEST(66)
{
  db::Layout g;
  init_layout (g);

  {
    db::LayoutQuery q ("instances of cell .*.* where inst.trans.rot == 0");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "data");
    EXPECT_EQ (s, "nil,nil");
    s = q2s_var (iq, "cell_name");
    EXPECT_EQ (s, "c1,c5x");
    s = q2s_var (iq, "inst_elements");
    EXPECT_EQ (s, "(cell_index=0 r0 *1 10,-20),(cell_index=4 r0 *1 10,-20)");
  }
}

//  Bug: path_dtrans was ICplxTrans on top level
TEST(67)
{
  db::Layout g;
  init_layout (g);

  {
    db::LayoutQuery q ("select path_dtrans*shape.dbbox from shapes on layer l1 from instances of .*");
    db::LayoutQueryIterator iq (q, &g);
    std::string s = q2s_var (iq, "data");
    EXPECT_EQ (s, "((0,0.001;0.002,0.003)),((0,0.001;0.002,0.003))");
  }
}
