
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
#include "dbLayoutDiff.h"
#include "dbLayerProperties.h"
#include "dbLayout.h"

#include <sstream>

class TestDifferenceReceiver
  : public db::DifferenceReceiver
{
public:
  TestDifferenceReceiver ();
  virtual ~TestDifferenceReceiver () { }

  std::string text () const { return m_os.str (); }
  void clear () { m_os.str (std::string ()); }
  void dbu_differs (double dbu_a, double dbu_b);
  void layout_meta_info_differs (const std::string &, const tl::Variant &, const tl::Variant &);
  void layer_in_a_only (const db::LayerProperties &la);
  void layer_in_b_only (const db::LayerProperties &lb);
  void layer_name_differs (const db::LayerProperties &la, const db::LayerProperties &lb);
  void cell_in_a_only (const std::string &cellname, db::cell_index_type ci);
  void cell_in_b_only (const std::string &cellname, db::cell_index_type ci);
  void cell_name_differs (const std::string &cellname_a, db::cell_index_type cia, const std::string &cellname_b, db::cell_index_type cib);
  void begin_cell (const std::string &cellname, db::cell_index_type cia, db::cell_index_type cib);
  void cell_meta_info_differs (const std::string &, const tl::Variant &, const tl::Variant &);
  void bbox_differs (const db::Box &ba, const db::Box &bb);
  void begin_inst_differences ();
  void instances_in_a (const std::vector <db::CellInstArrayWithProperties> &insts_a, const std::vector <std::string> &cell_names);
  void instances_in_b (const std::vector <db::CellInstArrayWithProperties> &insts_b, const std::vector <std::string> &cell_names);
  void instances_in_a_only (const std::vector <db::CellInstArrayWithProperties> &anotb, const db::Layout &a);
  void instances_in_b_only (const std::vector <db::CellInstArrayWithProperties> &bnota, const db::Layout &b);
  void end_inst_differences ();
  void begin_layer (const db::LayerProperties &layer, unsigned int layer_index_a, bool is_valid_a, unsigned int layer_index_b, bool is_valid_b);
  void per_layer_bbox_differs (const db::Box &ba, const db::Box &bb);
  void begin_polygon_differences ();
  void detailed_diff (const std::vector <std::pair <db::Polygon, db::properties_id_type> > &a, const std::vector <std::pair <db::Polygon, db::properties_id_type> > &b);
  void end_polygon_differences ();
  void begin_path_differences ();
  void detailed_diff (const std::vector <std::pair <db::Path, db::properties_id_type> > &a, const std::vector <std::pair <db::Path, db::properties_id_type> > &b);
  void end_path_differences ();
  void begin_box_differences ();
  void detailed_diff (const std::vector <std::pair <db::Box, db::properties_id_type> > &a, const std::vector <std::pair <db::Box, db::properties_id_type> > &b);
  void end_box_differences ();
  void begin_edge_differences ();
  void detailed_diff (const std::vector <std::pair <db::Edge, db::properties_id_type> > &a, const std::vector <std::pair <db::Edge, db::properties_id_type> > &b);
  void end_edge_differences ();
  void begin_edge_pair_differences ();
  void detailed_diff (const std::vector <std::pair <db::EdgePair, db::properties_id_type> > &a, const std::vector <std::pair <db::EdgePair, db::properties_id_type> > &b);
  void end_edge_pair_differences ();
  void begin_text_differences ();
  void detailed_diff (const std::vector <std::pair <db::Text, db::properties_id_type> > &a, const std::vector <std::pair <db::Text, db::properties_id_type> > &b);
  void end_text_differences ();
  void end_layer ();
  void end_cell ();

private:
  std::string m_cellname;
  std::ostringstream m_os;
  db::LayerProperties m_layer;

  void print_cell_inst (const db::CellInstArrayWithProperties &ci, const std::vector <std::string> &cell_names);
  void print_cell_inst (const db::CellInstArrayWithProperties &ci, const db::Layout &l);
  template <class SH> void print_diffs (const std::vector <std::pair <SH, db::properties_id_type> > &a, const std::vector <std::pair <SH, db::properties_id_type> > &b);
};

TestDifferenceReceiver::TestDifferenceReceiver ()
  : m_cellname (), m_layer ()
{
  // .. nothing yet ..
}

void 
TestDifferenceReceiver::print_cell_inst (const db::CellInstArrayWithProperties &ci, const db::Layout &l)
{
  m_os << "  " << l.cell_name (ci.object ().cell_index ()) << " " << ci.complex_trans ().to_string ();

  db::Vector a, b;
  unsigned long amax, bmax;
  if (ci.is_regular_array (a, b, amax, bmax)) {
    m_os << "[a=" << a.to_string () << ", b=" << b.to_string () << ", na=" << amax << ", nb=" << bmax << "]";
  } else if (ci.size () > 1) {
    m_os << " (+" << (ci.size () - 1) << " irregular placements)";
  }
  if (ci.properties_id () != 0) {
    m_os << " [" << ci.properties_id () << "]" << std::endl;
  } else {
    m_os << "" << std::endl;
  }
}

void 
TestDifferenceReceiver::print_cell_inst (const db::CellInstArrayWithProperties &ci, const std::vector <std::string> &cell_names)
{
  m_os << "  " << cell_names [ci.object ().cell_index ()] << " " << ci.complex_trans ().to_string ();

  db::Vector a, b;
  unsigned long amax, bmax;
  if (ci.is_regular_array (a, b, amax, bmax)) {
    m_os << "[a=" << a.to_string () << ", b=" << b.to_string () << ", na=" << amax << ", nb=" << bmax << "]";
  } else if (ci.size () > 1) {
    m_os << " (+" << (ci.size () - 1) << " irregular placements)";
  }
  if (ci.properties_id () != 0) {
    m_os << " [" << ci.properties_id () << "]" << std::endl;
  } else {
    m_os << "" << std::endl;
  }
}

template <class SH>
void
TestDifferenceReceiver::print_diffs (const std::vector <std::pair <SH, db::properties_id_type> > &_a, const std::vector <std::pair <SH, db::properties_id_type> > &_b)
{
  std::vector <std::pair <SH, db::properties_id_type> > a = _a;
  std::sort (a.begin (), a.end ());
  std::vector <std::pair <SH, db::properties_id_type> > b = _b;
  std::sort (b.begin (), b.end ());
  std::vector <std::pair <SH, db::properties_id_type> > anotb;
  std::set_difference (a.begin (), a.end (), b.begin (), b.end (), std::back_inserter (anotb));
  for (typename std::vector <std::pair <SH, db::properties_id_type> >::const_iterator s = anotb.begin (); s != anotb.end (); ++s) {
    m_os << "  " << s->first.to_string ();
    if (s->second != 0) {
      m_os << " [" << s->second << "]" << std::endl;
    } else {
      m_os << "" << std::endl;
    }
  }
}

void
TestDifferenceReceiver::dbu_differs (double dbu_a, double dbu_b) 
{
  m_os << "layout_diff: database units differ " << dbu_a << " vs. " << dbu_b << std::endl;
}

void
TestDifferenceReceiver::layout_meta_info_differs (const std::string &name, const tl::Variant &va, const tl::Variant &vb)
{
  m_os << "layout_diff: global meta info differs " << name << ": " << va.to_string () << " vs. " << vb.to_string () << std::endl;
}

void 
TestDifferenceReceiver::layer_in_a_only (const db::LayerProperties &la)
{
  m_os << "layout_diff: layer " << la.to_string () << " is not present in layout b, but in a" << std::endl;
}

void 
TestDifferenceReceiver::layer_in_b_only (const db::LayerProperties &lb)
{
  m_os << "layout_diff: layer " << lb.to_string () << " is not present in layout a, but in b" << std::endl;
}

void
TestDifferenceReceiver::layer_name_differs (const db::LayerProperties &la, const db::LayerProperties &lb)
{
  m_os << "layout_diff: layer names differ between layout a and b for layer " << la.layer << "/" << la.datatype << ": " 
       << la.name << " vs. " << lb.name << std::endl;
}

void 
TestDifferenceReceiver::cell_in_a_only (const std::string &cellname, db::cell_index_type /*ci*/)
{
  m_os << "layout_diff: cell " << cellname << " is not present in layout b, but in a" << std::endl;
}

void 
TestDifferenceReceiver::cell_in_b_only (const std::string &cellname, db::cell_index_type /*ci*/)
{
  m_os << "layout_diff: cell " << cellname << " is not present in layout a, but in b" << std::endl;
}

void 
TestDifferenceReceiver::cell_name_differs (const std::string &cellname_a, db::cell_index_type /*cia*/, const std::string &cellname_b, db::cell_index_type /*cib*/)
{
  m_os << "layout_diff: cell " << cellname_a << " in a is renamed to " << cellname_b << " in b" << std::endl;
}

void 
TestDifferenceReceiver::bbox_differs (const db::Box &ba, const db::Box &bb)
{
  m_os << "layout_diff: bounding boxes differ for cell " << m_cellname << ", " << ba.to_string () << " vs. " << bb.to_string () << std::endl;
}

void 
TestDifferenceReceiver::begin_cell (const std::string &cellname, db::cell_index_type /*cia*/, db::cell_index_type /*cib*/)
{
  m_cellname = cellname;
}

void
TestDifferenceReceiver::cell_meta_info_differs (const std::string &name, const tl::Variant &va, const tl::Variant &vb)
{
  m_os << "layout_diff: cell meta info differs for cell " << m_cellname << " - " << name << ": " << va.to_string () << " vs. " << vb.to_string () << std::endl;
}

void
TestDifferenceReceiver::begin_inst_differences ()
{
  m_os << "layout_diff: instances differ in cell " << m_cellname << std::endl;
}

void
TestDifferenceReceiver::instances_in_a (const std::vector <db::CellInstArrayWithProperties> &insts_a, const std::vector <std::string> &cell_names)
{
  m_os << "list for a:" << std::endl;
  for (std::vector <db::CellInstArrayWithProperties>::const_iterator s = insts_a.begin (); s != insts_a.end (); ++s) {
    print_cell_inst (*s, cell_names);
  }
}

void
TestDifferenceReceiver::instances_in_b (const std::vector <db::CellInstArrayWithProperties> &insts_b, const std::vector <std::string> &cell_names)
{
  m_os << "list for b:" << std::endl;
  for (std::vector <db::CellInstArrayWithProperties>::const_iterator s = insts_b.begin (); s != insts_b.end (); ++s) {
    print_cell_inst (*s, cell_names);
  }
}

void
TestDifferenceReceiver::instances_in_a_only (const std::vector <db::CellInstArrayWithProperties> &anotb, const db::Layout &a)
{
  m_os << "Not in b but in a:" << std::endl;
  for (std::vector <db::CellInstArrayWithProperties>::const_iterator s = anotb.begin (); s != anotb.end (); ++s) {
    print_cell_inst (*s, a);
  }
}

void
TestDifferenceReceiver::instances_in_b_only (const std::vector <db::CellInstArrayWithProperties> &bnota, const db::Layout &b)
{
  m_os << "Not in a but in b:" << std::endl;
  for (std::vector <db::CellInstArrayWithProperties>::const_iterator s = bnota.begin (); s != bnota.end (); ++s) {
    print_cell_inst (*s, b);
  }
}

void
TestDifferenceReceiver::end_inst_differences ()
{
}

void
TestDifferenceReceiver::begin_layer (const db::LayerProperties &layer, unsigned int /*layer_index_a*/, bool /*is_valid_a*/, unsigned int /*layer_index_b*/, bool /*is_valid_b*/)
{
  m_layer = layer;
}

void 
TestDifferenceReceiver::per_layer_bbox_differs (const db::Box &ba, const db::Box &bb)
{
  m_os << "layout_diff: per-layer bounding boxes differ for cell " << m_cellname << ", layer (" << m_layer.to_string () << "), " 
       << ba.to_string () << " vs. " << bb.to_string () << std::endl;
}

void
TestDifferenceReceiver::begin_polygon_differences ()
{
  m_os << "layout_diff: polygons differ for layer " << m_layer.to_string () << " in cell " << m_cellname << std::endl;
}

void
TestDifferenceReceiver::detailed_diff (const std::vector <std::pair <db::Polygon, db::properties_id_type> > &a, const std::vector <std::pair <db::Polygon, db::properties_id_type> > &b)
{
  m_os << "Not in b but in a:" << std::endl;
  print_diffs (a, b);
  m_os << "Not in a but in b:" << std::endl;
  print_diffs (b, a);
}

void
TestDifferenceReceiver::end_polygon_differences ()
{
}

void
TestDifferenceReceiver::begin_path_differences ()
{
  m_os << "layout_diff: paths differ for layer " << m_layer.to_string () << " in cell " << m_cellname << std::endl;
}

void
TestDifferenceReceiver::detailed_diff (const std::vector <std::pair <db::Path, db::properties_id_type> > &a, const std::vector <std::pair <db::Path, db::properties_id_type> > &b)
{
  m_os << "Not in b but in a:" << std::endl;
  print_diffs (a, b);
  m_os << "Not in a but in b:" << std::endl;
  print_diffs (b, a);
}

void
TestDifferenceReceiver::end_path_differences ()
{
}

void
TestDifferenceReceiver::begin_box_differences ()
{
  m_os << "layout_diff: boxes differ for layer " << m_layer.to_string () << " in cell " << m_cellname << std::endl;
}

void
TestDifferenceReceiver::detailed_diff (const std::vector <std::pair <db::Box, db::properties_id_type> > &a, const std::vector <std::pair <db::Box, db::properties_id_type> > &b)
{
  m_os << "Not in b but in a:" << std::endl;
  print_diffs (a, b);
  m_os << "Not in a but in b:" << std::endl;
  print_diffs (b, a);
}

void
TestDifferenceReceiver::end_box_differences ()
{
}

void
TestDifferenceReceiver::begin_edge_differences ()
{
  m_os << "layout_diff: edges differ for layer " << m_layer.to_string () << " in cell " << m_cellname << std::endl;
}

void
TestDifferenceReceiver::detailed_diff (const std::vector <std::pair <db::Edge, db::properties_id_type> > &a, const std::vector <std::pair <db::Edge, db::properties_id_type> > &b)
{
  m_os << "Not in b but in a:" << std::endl;
  print_diffs (a, b);
  m_os << "Not in a but in b:" << std::endl;
  print_diffs (b, a);
}

void
TestDifferenceReceiver::end_edge_differences ()
{
}

void
TestDifferenceReceiver::begin_edge_pair_differences ()
{
  m_os << "layout_diff: edge pairs differ for layer " << m_layer.to_string () << " in cell " << m_cellname << std::endl;
}

void
TestDifferenceReceiver::detailed_diff (const std::vector <std::pair <db::EdgePair, db::properties_id_type> > &a, const std::vector <std::pair <db::EdgePair, db::properties_id_type> > &b)
{
  m_os << "Not in b but in a:" << std::endl;
  print_diffs (a, b);
  m_os << "Not in a but in b:" << std::endl;
  print_diffs (b, a);
}

void
TestDifferenceReceiver::end_edge_pair_differences ()
{
}

void
TestDifferenceReceiver::begin_text_differences ()
{
  m_os << "layout_diff: texts differ for layer " << m_layer.to_string () << " in cell " << m_cellname << std::endl;
}

void
TestDifferenceReceiver::detailed_diff (const std::vector <std::pair <db::Text, db::properties_id_type> > &a, const std::vector <std::pair <db::Text, db::properties_id_type> > &b)
{
  m_os << "Not in b but in a:" << std::endl;
  print_diffs (a, b);
  m_os << "Not in a but in b:" << std::endl;
  print_diffs (b, a);
}

void
TestDifferenceReceiver::end_text_differences ()
{
}

void
TestDifferenceReceiver::end_layer ()
{
}

void 
TestDifferenceReceiver::end_cell ()
{
}

TEST(1) 
{
  db::Layout g;
  g.insert_layer (0);
  g.set_properties (0, db::LayerProperties (17, 0));
  g.insert_layer (1);
  g.set_properties (1, db::LayerProperties (42, 1));

  db::cell_index_type c1i = g.add_cell ("c1");
  db::cell_index_type c2i = g.add_cell ("c2x");
  db::cell_index_type c3i = g.add_cell ("c3");
  db::cell_index_type c4i = g.add_cell ("c4");
  db::cell_index_type c5i = g.add_cell ("c5x");

  {

    db::Cell &c1 (g.cell (c1i));
    db::Cell &c2 (g.cell (c2i));
    db::Cell &c3 (g.cell (c3i));
    db::Cell &c4 (g.cell (c4i));
    db::Cell &c5 (g.cell (c5i));
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

  }

  db::Layout h = g;

  TestDifferenceReceiver r;
  bool eq;
  
  r.clear ();
  eq = db::compare_layouts (g, h, 0, 0, r); 

  EXPECT_EQ (eq, true);
  EXPECT_EQ (r.text (), "");

  {
    //  c2->c5 (2x)
    db::FTrans f (1, true);
    db::Vector p (-10, 20);
    db::Trans t (f.rot (), p);
    db::Vector pp (10, -20);
    db::Trans tt (0, pp);
    g.cell (c2i).insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5i), t));
    g.cell (c2i).insert (db::array <db::CellInst, db::Trans> (db::CellInst (c5i), tt));
  }

  r.clear ();
  eq = db::compare_layouts (g, h, 0, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), "layout_diff: instances differ in cell c2x\n");

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: instances differ in cell c2x\n"
    "list for a:\n"
    "  c1 r0 *1 10,-20\n"
    "  c1 m45 *1 -10,20\n"
    "  c4 m45 *1 -10,20\n"
    "  c4 m45 *1 -10,20\n"
    "  c5x r0 *1 10,-20\n"
    "  c5x m45 *1 -10,20\n"
    "list for b:\n"
    "  c1 r0 *1 10,-20\n"
    "  c1 m45 *1 -10,20\n"
    "  c4 m45 *1 -10,20\n"
    "  c4 m45 *1 -10,20\n"
    "Not in b but in a:\n"
    "  c5x r0 *1 10,-20\n"
    "  c5x m45 *1 -10,20\n"
    "Not in a but in b:\n"
  );

  g = h;
  g.set_properties (1, db::LayerProperties (42, 2));

  r.clear ();
  eq = db::compare_layouts (g, h, 0, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: layer 42/2 is not present in layout b, but in a\n"
    "layout_diff: layer 42/1 is not present in layout a, but in b\n"
  );

  g = h;
  g.rename_cell (c2i, "c2");

  r.clear ();
  eq = db::compare_layouts (g, h, 0, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: cell c2 is not present in layout b, but in a\n"
    "layout_diff: cell c2x is not present in layout a, but in b\n"
  );
}

TEST(2) 
{
  db::Layout g;
  g.insert_layer (0);
  g.set_properties (0, db::LayerProperties (17, 0));
  g.insert_layer (1);
  g.set_properties (1, db::LayerProperties (42, 1));

  db::cell_index_type c1i = g.add_cell ("c1");
  db::cell_index_type c2i = g.add_cell ("c2x");
  db::cell_index_type c3i = g.add_cell ("c3");
  db::cell_index_type c4i = g.add_cell ("c4");
  db::cell_index_type c5i = g.add_cell ("c5x");

  {

    db::Cell &c1 (g.cell (c1i));
    db::Cell &c2 (g.cell (c2i));
    db::Cell &c3 (g.cell (c3i));
    db::Cell &c4 (g.cell (c4i));
    db::Cell &c5 (g.cell (c5i));
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

  }

  db::Layout h = g;

  TestDifferenceReceiver r;
  bool eq;
  
  g.cell (c2i).shapes (0).insert (db::Box (1, 2, 1003, 1004));

  r.clear ();
  eq = db::compare_layouts (g, h, 0, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: bounding boxes differ for cell c2x, (0,1;1003,1004) vs. (0,1;2,3)\n"
    "layout_diff: per-layer bounding boxes differ for cell c2x, layer (17/0), (0,1;1003,1004) vs. (0,1;2,3)\n"
    "layout_diff: boxes differ for layer 17/0 in cell c2x\n"
  );

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (),  
    "layout_diff: boxes differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  (1,2;1003,1004)\n"
    "Not in a but in b:\n"
  );

  db::Layout hh = h;

  db::Cell &c2h = h.cell (c2i);
  c2h.shapes (0).insert (db::Box (1, 2, 1003, 1005));
  c2h.shapes (0).insert (db::Box (2, 2, 1003, 1004));
  c2h.shapes (0).insert (db::Box (1, 2, 1003, 1006));

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: boxes differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  (1,2;1003,1004)\n"
    "Not in a but in b:\n"
    "  (1,2;1003,1005)\n"
    "  (1,2;1003,1006)\n"
    "  (2,2;1003,1004)\n"
  );

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 1, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: boxes differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "Not in a but in b:\n"
    "  (1,2;1003,1006)\n"
    "  (2,2;1003,1004)\n"
  );
}

std::string ps2string (db::properties_id_type pi)
{
  return db::properties (pi).to_dict_var ().to_string ();
}

TEST(2P) 
{
  db::Layout g;
  g.insert_layer (0);
  g.set_properties (0, db::LayerProperties (17, 0));
  g.insert_layer (1);
  g.set_properties (1, db::LayerProperties (42, 1));

  db::cell_index_type c1i = g.add_cell ("c1");
  db::cell_index_type c2i = g.add_cell ("c2x");
  db::cell_index_type c3i = g.add_cell ("c3");
  db::cell_index_type c4i = g.add_cell ("c4");
  db::cell_index_type c5i = g.add_cell ("c5x");

  {

    db::Cell &c1 (g.cell (c1i));
    db::Cell &c2 (g.cell (c2i));
    db::Cell &c3 (g.cell (c3i));
    db::Cell &c4 (g.cell (c4i));
    db::Cell &c5 (g.cell (c5i));
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

  }

  {
    db::properties_id_type pi1, pi2, pi3;

    db::PropertiesSet ps;
    ps.insert (tl::Variant ("A"), tl::Variant (1));
    pi1 = db::properties_id (ps);

    ps.clear ();
    ps.insert (tl::Variant ("B"), tl::Variant (2));
    pi2 = db::properties_id (ps);

    ps.insert (tl::Variant ("C"), tl::Variant ("c"));
    pi3 = db::properties_id (ps);

    EXPECT_EQ (ps2string (pi1), "{A=>1}");
    EXPECT_EQ (ps2string (pi2), "{B=>2}");
    EXPECT_EQ (ps2string (pi3), "{B=>2,C=>c}");
  }

  db::Layout h = g;

  TestDifferenceReceiver r;
  bool eq;
  
  g.cell (c2i).shapes (0).insert (db::BoxWithProperties (db::Box (1, 2, 1003, 1004), 1));

  r.clear ();
  eq = db::compare_layouts (g, h, 0, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: bounding boxes differ for cell c2x, (0,1;1003,1004) vs. (0,1;2,3)\n"
    "layout_diff: per-layer bounding boxes differ for cell c2x, layer (17/0), (0,1;1003,1004) vs. (0,1;2,3)\n"
    "layout_diff: boxes differ for layer 17/0 in cell c2x\n"
  );

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (),  
    "layout_diff: boxes differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  (1,2;1003,1004) [1]\n"
    "Not in a but in b:\n"
  );

  db::Layout hh = h;

  //  Note: properties are "normalized" (mapped to a common layout). In order to maintain
  //  their meaning later, keep these inserts sorted by property ID:
  h.cell (c2i).shapes (0).insert (db::BoxWithProperties (db::Box (1, 2, 1003, 1006), 1));
  h.cell (c2i).shapes (0).insert (db::BoxWithProperties (db::Box (1, 2, 1003, 1005), 2));
  h.cell (c2i).shapes (0).insert (db::BoxWithProperties (db::Box (2, 2, 1003, 1004), 3));

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: boxes differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  (1,2;1003,1004) [1]\n"
    "Not in a but in b:\n"
    "  (1,2;1003,1005) [2]\n"
    "  (1,2;1003,1006) [1]\n"
    "  (2,2;1003,1004) [3]\n"
  );

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 1, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: boxes differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  (1,2;1003,1004) [1]\n"
    "Not in a but in b:\n"
    "  (1,2;1003,1005) [2]\n"
    "  (1,2;1003,1006) [1]\n"
    "  (2,2;1003,1004) [3]\n"
  );

  h = hh;

  //  Note: properties are "normalized" (mapped to a common layout). In order to maintain
  //  their meaning later, keep these inserts sorted by property ID:
  h.cell (c2i).shapes (0).insert (db::BoxWithProperties (db::Box (1, 2, 1003, 1005), 1));
  h.cell (c2i).shapes (0).insert (db::BoxWithProperties (db::Box (1, 2, 1003, 1006), 2));
  h.cell (c2i).shapes (0).insert (db::BoxWithProperties (db::Box (2, 2, 1003, 1004), 3));

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: boxes differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  (1,2;1003,1004) [1]\n"
    "Not in a but in b:\n"
    "  (1,2;1003,1005) [1]\n"
    "  (1,2;1003,1006) [2]\n"
    "  (2,2;1003,1004) [3]\n"
  );

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 1, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: boxes differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "Not in a but in b:\n"
    "  (1,2;1003,1006) [2]\n"
    "  (2,2;1003,1004) [3]\n"
  );

  h = hh;

  h.cell (c2i).shapes (0).insert (db::BoxWithProperties (db::Box (2, 2, 1003, 1004), 1));
  h.cell (c2i).shapes (0).insert (db::BoxWithProperties (db::Box (1, 2, 1003, 1006), 2));
  h.cell (c2i).shapes (0).insert (db::BoxWithProperties (db::Box (1, 2, 1003, 1005), 3));

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 1, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: boxes differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "Not in a but in b:\n"
    "  (1,2;1003,1005) [3]\n"
    "  (1,2;1003,1006) [2]\n"
  );

  h = hh;

  h.cell (c2i).shapes (0).insert (db::Box (2, 2, 1003, 1004));
  h.cell (c2i).shapes (0).insert (db::BoxWithProperties (db::Box (1, 2, 1003, 1006), 1));
  h.cell (c2i).shapes (0).insert (db::BoxWithProperties (db::Box (1, 2, 1003, 1005), 1));

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 1, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: boxes differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  (1,2;1003,1004) [1]\n"
    "Not in a but in b:\n"
    "  (1,2;1003,1005) [1]\n"
    "  (1,2;1003,1006) [1]\n"
    "  (2,2;1003,1004)\n"
  );
}

TEST(3) 
{
  db::Layout g;
  g.insert_layer (0);
  g.set_properties (0, db::LayerProperties (17, 0));
  g.insert_layer (1);
  g.set_properties (1, db::LayerProperties (42, 1));

  db::cell_index_type c1i = g.add_cell ("c1");
  db::cell_index_type c2i = g.add_cell ("c2x");
  db::cell_index_type c3i = g.add_cell ("c3");
  db::cell_index_type c4i = g.add_cell ("c4");
  db::cell_index_type c5i = g.add_cell ("c5x");

  {

    db::Cell &c1 (g.cell (c1i));
    db::Cell &c2 (g.cell (c2i));
    db::Cell &c3 (g.cell (c3i));
    db::Cell &c4 (g.cell (c4i));
    db::Cell &c5 (g.cell (c5i));
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

  }

  db::Layout h = g;

  TestDifferenceReceiver r;
  bool eq;
  
  g.cell (c2i).shapes (0).insert (db::Polygon (db::Box (1, 2, 1003, 1004)));

  r.clear ();
  eq = db::compare_layouts (g, h, 0, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: bounding boxes differ for cell c2x, (0,1;1003,1004) vs. (0,1;2,3)\n"
    "layout_diff: per-layer bounding boxes differ for cell c2x, layer (17/0), (0,1;1003,1004) vs. (0,1;2,3)\n"
    "layout_diff: polygons differ for layer 17/0 in cell c2x\n"
  );

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (),  
    "layout_diff: polygons differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  (1,2;1,1004;1003,1004;1003,2)\n"
    "Not in a but in b:\n"
  );

  db::Layout hh = h;

  db::Cell &c2h = h.cell (c2i);
  c2h.shapes (0).insert (db::Polygon (db::Box (1, 2, 1003, 1005)));
  c2h.shapes (0).insert (db::Polygon (db::Box (2, 2, 1003, 1004)));
  c2h.shapes (0).insert (db::Polygon (db::Box (1, 2, 1003, 1006)));

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 0, r);

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: polygons differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  (1,2;1,1004;1003,1004;1003,2)\n"
    "Not in a but in b:\n"
    "  (1,2;1,1005;1003,1005;1003,2)\n"
    "  (1,2;1,1006;1003,1006;1003,2)\n"
    "  (2,2;2,1004;1003,1004;1003,2)\n"
  );

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 1, r);

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: polygons differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "Not in a but in b:\n"
    "  (1,2;1,1006;1003,1006;1003,2)\n"
    "  (2,2;2,1004;1003,1004;1003,2)\n"
  );
}

TEST(4) 
{
  db::Layout g;
  g.insert_layer (0);
  g.set_properties (0, db::LayerProperties (17, 0));
  g.insert_layer (1);
  g.set_properties (1, db::LayerProperties (42, 1));

  db::cell_index_type c1i = g.add_cell ("c1");
  db::cell_index_type c2i = g.add_cell ("c2x");
  db::cell_index_type c3i = g.add_cell ("c3");
  db::cell_index_type c4i = g.add_cell ("c4");
  db::cell_index_type c5i = g.add_cell ("c5x");

  {

    db::Cell &c1 (g.cell (c1i));
    db::Cell &c2 (g.cell (c2i));
    db::Cell &c3 (g.cell (c3i));
    db::Cell &c4 (g.cell (c4i));
    db::Cell &c5 (g.cell (c5i));
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

  }

  db::Layout h = g;

  TestDifferenceReceiver r;
  bool eq;
  
  g.cell (c2i).shapes (0).insert (db::Edge (1, 2, 1003, 1004));

  r.clear ();
  eq = db::compare_layouts (g, h, 0, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: bounding boxes differ for cell c2x, (0,1;1003,1004) vs. (0,1;2,3)\n"
    "layout_diff: per-layer bounding boxes differ for cell c2x, layer (17/0), (0,1;1003,1004) vs. (0,1;2,3)\n"
    "layout_diff: edges differ for layer 17/0 in cell c2x\n"
  );

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (),  
    "layout_diff: edges differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  (1,2;1003,1004)\n"
    "Not in a but in b:\n"
  );

  db::Layout hh = h;

  db::Cell &c2h = h.cell (c2i);
  c2h.shapes (0).insert (db::Edge (1, 2, 1003, 1005));
  c2h.shapes (0).insert (db::Edge (2, 2, 1003, 1004));
  c2h.shapes (0).insert (db::Edge (1, 2, 1003, 1006));

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: edges differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  (1,2;1003,1004)\n"
    "Not in a but in b:\n"
    "  (1,2;1003,1005)\n"
    "  (1,2;1003,1006)\n"
    "  (2,2;1003,1004)\n"
  );

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 1, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: edges differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "Not in a but in b:\n"
    "  (1,2;1003,1006)\n"
    "  (2,2;1003,1004)\n"
  );
}

TEST(5) 
{
  db::Layout g;
  g.insert_layer (0);
  g.set_properties (0, db::LayerProperties (17, 0));
  g.insert_layer (1);
  g.set_properties (1, db::LayerProperties (42, 1));

  db::cell_index_type c1i = g.add_cell ("c1");
  db::cell_index_type c2i = g.add_cell ("c2x");
  db::cell_index_type c3i = g.add_cell ("c3");
  db::cell_index_type c4i = g.add_cell ("c4");
  db::cell_index_type c5i = g.add_cell ("c5x");

  {

    db::Cell &c1 (g.cell (c1i));
    db::Cell &c2 (g.cell (c2i));
    db::Cell &c3 (g.cell (c3i));
    db::Cell &c4 (g.cell (c4i));
    db::Cell &c5 (g.cell (c5i));
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

  }

  db::Layout h = g;

  TestDifferenceReceiver r;
  bool eq;
  
  db::Text t;
  t = db::Text ("X", db::Trans (1, db::Vector (2, 3)), 17);
  g.cell (c2i).shapes (0).insert (t);

  r.clear ();
  eq = db::compare_layouts (g, h, 0, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: texts differ for layer 17/0 in cell c2x\n"
  );

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (),  
    "layout_diff: texts differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  ('X',r90 2,3) s=17\n"
    "Not in a but in b:\n"
  );

  db::Layout hh = h;
  hh.cell (c2i).shapes (0).insert (t);

  r.clear ();
  eq = db::compare_layouts (g, hh, db::layout_diff::f_verbose, 0, r); 

  EXPECT_EQ (eq, true);
  EXPECT_EQ (r.text (), "");

  hh = h;

  db::Cell &c2h = h.cell (c2i);
  c2h.shapes (0).insert (db::Text ("Y", db::Trans (1, db::Vector (2, 3)), 17));
  c2h.shapes (0).insert (db::Text ("X", db::Trans (2, db::Vector (2, 3)), 17));
  c2h.shapes (0).insert (db::Text ("X", db::Trans (1, db::Vector (3, 4)), 17));
  c2h.shapes (0).insert (db::Text ("X", db::Trans (1, db::Vector (2, 3)), 18));
  //  Text attributes like font and alignment are not compared, hence this text matches the one of g:
  c2h.shapes (0).insert (db::Text ("X", db::Trans (1, db::Vector (2, 3)), 17, db::DefaultFont, db::HAlignCenter, db::VAlignCenter));

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: texts differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "Not in a but in b:\n"
    "  ('X',r90 2,3) s=18\n"
    "  ('Y',r90 2,3) s=17\n"
    "  ('X',r90 3,4) s=17\n"
    "  ('X',r180 2,3) s=17\n"
  );

  //  two more to match more of h:
  g.cell (c2i).shapes (0).insert (t);
  g.cell (c2i).shapes (0).insert (t);

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 1, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: texts differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "Not in a but in b:\n"
    "  ('Y',r90 2,3) s=17\n"
    "  ('X',r180 2,3) s=17\n"
  );
}

TEST(6) 
{
  db::Layout g;
  g.insert_layer (0);
  g.set_properties (0, db::LayerProperties (17, 0));
  g.insert_layer (1);
  g.set_properties (1, db::LayerProperties (42, 1));

  db::cell_index_type c1i = g.add_cell ("c1");
  db::cell_index_type c2i = g.add_cell ("c2x");
  db::cell_index_type c3i = g.add_cell ("c3");
  db::cell_index_type c4i = g.add_cell ("c4");
  db::cell_index_type c5i = g.add_cell ("c5x");

  {

    db::Cell &c1 (g.cell (c1i));
    db::Cell &c2 (g.cell (c2i));
    db::Cell &c3 (g.cell (c3i));
    db::Cell &c4 (g.cell (c4i));
    db::Cell &c5 (g.cell (c5i));
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

  }

  db::Layout h = g;

  TestDifferenceReceiver r;
  bool eq;
  
  db::Point pts1[] = { db::Point (1, 2), db::Point (11, 12) };
  db::Point pts2[] = { db::Point (1, 3), db::Point (11, 12) };
  db::Point pts3[] = { db::Point (1, 3), db::Point (11, 11) };

  db::Path p;
  p = db::Path (&pts1[0], &pts1[sizeof (pts1) / sizeof (pts1[0])], 17, 0, 0, false);
  g.cell (c2i).shapes (0).insert (p);

  r.clear ();
  eq = db::compare_layouts (g, h, 0, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: bounding boxes differ for cell c2x, (-5,-4;17,18) vs. (0,1;2,3)\n"
    "layout_diff: per-layer bounding boxes differ for cell c2x, layer (17/0), (-5,-4;17,18) vs. (0,1;2,3)\n"
    "layout_diff: paths differ for layer 17/0 in cell c2x\n"
  );

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (),  
    "layout_diff: paths differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  (1,2;11,12) w=17 bx=0 ex=0 r=false\n"
    "Not in a but in b:\n"
  );

  db::Layout hh = h;
  hh.cell (c2i).shapes (0).insert (p);

  r.clear ();
  eq = db::compare_layouts (g, hh, db::layout_diff::f_verbose, 0, r); 

  EXPECT_EQ (eq, true);
  EXPECT_EQ (r.text (), "");

  hh = h;

  db::Cell &c2h = h.cell (c2i);
  c2h.shapes (0).insert (db::Path (&pts1[0], &pts1[sizeof (pts1) / sizeof (pts1[0])], 18, 0, 0, false));
  c2h.shapes (0).insert (db::Path (&pts1[0], &pts1[sizeof (pts1) / sizeof (pts1[0])], 17, 1, 0, false));
  c2h.shapes (0).insert (db::Path (&pts1[0], &pts1[sizeof (pts1) / sizeof (pts1[0])], 17, 0, -1, false));
  c2h.shapes (0).insert (db::Path (&pts1[0], &pts1[sizeof (pts1) / sizeof (pts1[0])], 17, 0, 0, true));
  c2h.shapes (0).insert (db::Path (&pts2[0], &pts2[sizeof (pts2) / sizeof (pts2[0])], 17, 0, 0, false));
  c2h.shapes (0).insert (db::Path (&pts3[0], &pts3[sizeof (pts3) / sizeof (pts3[0])], 17, 0, 0, false));

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: paths differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  (1,2;11,12) w=17 bx=0 ex=0 r=false\n"
    "Not in a but in b:\n"
    "  (1,2;11,12) w=17 bx=0 ex=0 r=true\n"
    "  (1,2;11,12) w=17 bx=0 ex=-1 r=false\n"
    "  (1,3;11,11) w=17 bx=0 ex=0 r=false\n"
    "  (1,3;11,12) w=17 bx=0 ex=0 r=false\n"
    "  (1,2;11,12) w=17 bx=1 ex=0 r=false\n"
    "  (1,2;11,12) w=18 bx=0 ex=0 r=false\n"
  );

  //  some more to match more of h:
  g.cell (c2i).shapes (0).insert (p);
  g.cell (c2i).shapes (0).insert (p);
  g.cell (c2i).shapes (0).insert (p);
  g.cell (c2i).shapes (0).insert (p);

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 1, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: paths differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "Not in a but in b:\n"
    "  (1,2;11,12) w=17 bx=0 ex=0 r=true\n"
  );
}

TEST(7) 
{
  db::Layout g;
  g.insert_layer (0);
  g.set_properties (0, db::LayerProperties (17, 0));
  g.insert_layer (1);
  g.set_properties (1, db::LayerProperties (42, 1));

  db::cell_index_type c1i = g.add_cell ("c1");
  db::cell_index_type c2i = g.add_cell ("c2x");
  db::cell_index_type c3i = g.add_cell ("c3");
  db::cell_index_type c4i = g.add_cell ("c4");
  db::cell_index_type c5i = g.add_cell ("c5x");

  {

    db::Cell &c1 (g.cell (c1i));
    db::Cell &c2 (g.cell (c2i));
    db::Cell &c3 (g.cell (c3i));
    db::Cell &c4 (g.cell (c4i));
    db::Cell &c5 (g.cell (c5i));
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

  }

  db::Layout h = g;
  db::Layout hh = g;
  db::Layout hhh = g;

  TestDifferenceReceiver r;
  bool eq;
  
  g.cell (c2i).shapes (0).insert (db::Box (1, 2, 1003, 1004));
  g.cell (c2i).shapes (0).insert (db::Box (2, 3, 1004, 1005));
  g.cell (c2i).shapes (0).insert (db::Box (3, 4, 1005, 1006));
  g.cell (c2i).shapes (0).insert (db::Box (4, 5, 1006, 1007));
  g.cell (c2i).shapes (0).insert (db::Box (5, 6, 1007, 1008));
  g.cell (c2i).shapes (0).insert (db::Box (3, 7, 1008, 1009));
  g.cell (c2i).shapes (0).insert (db::Box (3, 8, 1009, 1010));

  h.cell (c2i).shapes (0).insert (db::Box (3, 8, 1009, 1010));
  h.cell (c2i).shapes (0).insert (db::Box (3, 7, 1008, 1009));
  h.cell (c2i).shapes (0).insert (db::Box (5, 6, 1007, 1008));
  h.cell (c2i).shapes (0).insert (db::Box (4, 5, 1006, 1009));
  h.cell (c2i).shapes (0).insert (db::Box (3, 4, 1005, 1006));
  h.cell (c2i).shapes (0).insert (db::Box (2, 3, 1004, 1005));
  h.cell (c2i).shapes (0).insert (db::Box (1, 2, 1003, 1004));

  hh.cell (c2i).shapes (0).insert (db::Box (3, 8, 1009, 1010));
  hh.cell (c2i).shapes (0).insert (db::Box (3, 7, 1008, 1009));
  hh.cell (c2i).shapes (0).insert (db::Box (5, 6, 1007, 1008));
  hh.cell (c2i).shapes (0).insert (db::Box (4, 5, 1006, 1007));
  hh.cell (c2i).shapes (0).insert (db::Box (3, 4, 1005, 1006));
  hh.cell (c2i).shapes (0).insert (db::Box (2, 3, 1004, 1005));
  hh.cell (c2i).shapes (0).insert (db::Box (1, 2, 1003, 1004));

  hhh.cell (c2i).shapes (0).insert (db::Box (3, 8, 1009, 1010));
  hhh.cell (c2i).shapes (0).insert (db::Box (3, 7, 1008, 1009));
  hhh.cell (c2i).shapes (0).insert (db::Box (5, 6, 1007, 1008));
  hhh.cell (c2i).shapes (0).insert (db::Box (4, 5, 1006, 1008));
  hhh.cell (c2i).shapes (0).insert (db::Box (3, 4, 1005, 1006));
  hhh.cell (c2i).shapes (0).insert (db::Box (2, 3, 1004, 1005));
  hhh.cell (c2i).shapes (0).insert (db::Box (1, 2, 1003, 1004));

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (),  
    "layout_diff: boxes differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  (4,5;1006,1007)\n"
    "Not in a but in b:\n"
    "  (4,5;1006,1009)\n"
  );

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 1, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (),  
    "layout_diff: boxes differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  (4,5;1006,1007)\n"
    "Not in a but in b:\n"
    "  (4,5;1006,1009)\n"
  );

  r.clear ();
  eq = db::compare_layouts (g, hh, db::layout_diff::f_verbose, 0, r); 

  EXPECT_EQ (eq, true);
  EXPECT_EQ (r.text (), "");

  r.clear ();
  eq = db::compare_layouts (g, hh, db::layout_diff::f_verbose, 1, r); 

  EXPECT_EQ (eq, true);
  EXPECT_EQ (r.text (), "");

  r.clear ();
  eq = db::compare_layouts (g, hhh, db::layout_diff::f_verbose, 0, r); 

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (), 
    "layout_diff: boxes differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  (4,5;1006,1007)\n"
    "Not in a but in b:\n"
    "  (4,5;1006,1008)\n"
  );

  r.clear ();
  eq = db::compare_layouts (g, hhh, db::layout_diff::f_verbose, 1, r); 

  EXPECT_EQ (eq, true);
  EXPECT_EQ (r.text (), "");
}

TEST(8)
{
  db::Layout g;
  g.insert_layer (0);
  g.set_properties (0, db::LayerProperties (17, 0));
  g.insert_layer (1);
  g.set_properties (1, db::LayerProperties (42, 1));

  db::cell_index_type c1i = g.add_cell ("c1");
  db::cell_index_type c2i = g.add_cell ("c2x");
  db::cell_index_type c3i = g.add_cell ("c3");
  db::cell_index_type c4i = g.add_cell ("c4");
  db::cell_index_type c5i = g.add_cell ("c5x");

  {

    db::Cell &c1 (g.cell (c1i));
    db::Cell &c2 (g.cell (c2i));
    db::Cell &c3 (g.cell (c3i));
    db::Cell &c4 (g.cell (c4i));
    db::Cell &c5 (g.cell (c5i));
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

  }

  db::Layout h = g;

  TestDifferenceReceiver r;
  bool eq;

  g.cell (c2i).shapes (0).insert (db::Box (1, 2, 1001, 1002));
  g.cell (c2i).shapes (0).insert (db::Box (2, 3, 1002, 1003));
  g.cell (c2i).shapes (0).insert (db::Box (2, 3, 1002, 1003));
  g.cell (c2i).shapes (0).insert (db::Box (3, 4, 1003, 1004));
  g.cell (c2i).shapes (0).insert (db::Box (3, 4, 1003, 1004));

  h.cell (c2i).shapes (0).insert (db::Box (1, 2, 1001, 1002));
  h.cell (c2i).shapes (0).insert (db::Box (1, 2, 1001, 1002));
  h.cell (c2i).shapes (0).insert (db::Box (2, 3, 1002, 1003));
  h.cell (c2i).shapes (0).insert (db::Box (4, 5, 1004, 1005));
  h.cell (c2i).shapes (0).insert (db::Box (4, 5, 1004, 1005));

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 0, r);

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (),
    "layout_diff: boxes differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  (2,3;1002,1003)\n"
    "  (3,4;1003,1004)\n"
    "  (3,4;1003,1004)\n"
    "Not in a but in b:\n"
    "  (1,2;1001,1002)\n"
    "  (4,5;1004,1005)\n"
    "  (4,5;1004,1005)\n"
  );

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose + db::layout_diff::f_ignore_duplicates, 0, r);

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (),
    "layout_diff: boxes differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  (3,4;1003,1004)\n"
    "Not in a but in b:\n"
    "  (4,5;1004,1005)\n"
  );

  //  duplicate instances
  {
    db::FTrans f (1, true);
    db::Vector p (-10, 20);
    db::Trans t (f.rot (), p);

    h.cell(c4i).insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1i), t, db::Vector(1, 1), db::Vector (0, 2), 2, 3));
    h.cell(c4i).insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1i), t));
    h.cell(c4i).insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1i), t));

    g.cell(c5i).insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1i), t));
    g.cell(c5i).insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1i), t, db::Vector(1, 1), db::Vector (0, 2), 2, 3));
    g.cell(c5i).insert (db::array <db::CellInst, db::Trans> (db::CellInst (c1i), t, db::Vector(1, 1), db::Vector (0, 2), 2, 3));

    db::cell_index_type c6i = g.add_cell ("c6");
    g.cell(c5i).insert (db::array <db::CellInst, db::Trans> (db::CellInst (c6i), t));
    g.cell(c5i).insert (db::array <db::CellInst, db::Trans> (db::CellInst (c6i), t));

  }

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose, 0, r);

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (),
    "layout_diff: cell c6 is not present in layout b, but in a\n"
    "layout_diff: boxes differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  (2,3;1002,1003)\n"
    "  (3,4;1003,1004)\n"
    "  (3,4;1003,1004)\n"
    "Not in a but in b:\n"
    "  (1,2;1001,1002)\n"
    "  (4,5;1004,1005)\n"
    "  (4,5;1004,1005)\n"
    "layout_diff: instances differ in cell c4\n"
    "list for a:\n"
    "  c1 r0 *1 10,-20\n"
    "  c1 m45 *1 -10,20[a=1,1, b=0,2, na=2, nb=3]\n"
    "  c3 m45 *1 -10,20\n"
    "list for b:\n"
    "  c1 r0 *1 10,-20\n"
    "  c1 m45 *1 -10,20\n"
    "  c1 m45 *1 -10,20\n"
    "  c1 m45 *1 -10,20[a=1,1, b=0,2, na=2, nb=3]\n"
    "  c1 m45 *1 -10,20[a=1,1, b=0,2, na=2, nb=3]\n"
    "  c3 m45 *1 -10,20\n"
    "Not in b but in a:\n"
    "Not in a but in b:\n"
    "  c1 m45 *1 -10,20\n"
    "  c1 m45 *1 -10,20\n"
    "  c1 m45 *1 -10,20[a=1,1, b=0,2, na=2, nb=3]\n"
    "layout_diff: instances differ in cell c5x\n"
    "list for a:\n"
    "  c1 m45 *1 -10,20\n"
    "  c1 m45 *1 -10,20\n"
    "  c1 m45 *1 -10,20[a=1,1, b=0,2, na=2, nb=3]\n"
    "  c1 m45 *1 -10,20[a=1,1, b=0,2, na=2, nb=3]\n"
    "list for b:\n"
    "  c1 m45 *1 -10,20\n"
    "Not in b but in a:\n"
    "  c1 m45 *1 -10,20\n"
    "  c1 m45 *1 -10,20[a=1,1, b=0,2, na=2, nb=3]\n"
    "  c1 m45 *1 -10,20[a=1,1, b=0,2, na=2, nb=3]\n"
    "  c6 m45 *1 -10,20\n"
    "  c6 m45 *1 -10,20\n"
    "Not in a but in b:\n"
  );

  r.clear ();
  eq = db::compare_layouts (g, h, db::layout_diff::f_verbose + db::layout_diff::f_ignore_duplicates, 0, r);

  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (),
    "layout_diff: cell c6 is not present in layout b, but in a\n"
    "layout_diff: boxes differ for layer 17/0 in cell c2x\n"
    "Not in b but in a:\n"
    "  (3,4;1003,1004)\n"
    "Not in a but in b:\n"
    "  (4,5;1004,1005)\n"
    "layout_diff: instances differ in cell c4\n"
    "list for a:\n"
    "  c1 r0 *1 10,-20\n"
    "  c1 m45 *1 -10,20[a=1,1, b=0,2, na=2, nb=3]\n"
    "  c3 m45 *1 -10,20\n"
    "list for b:\n"
    "  c1 r0 *1 10,-20\n"
    "  c1 m45 *1 -10,20\n"
    "  c1 m45 *1 -10,20[a=1,1, b=0,2, na=2, nb=3]\n"
    "  c3 m45 *1 -10,20\n"
    "Not in b but in a:\n"
    "Not in a but in b:\n"
    "  c1 m45 *1 -10,20\n"
    "layout_diff: instances differ in cell c5x\n"
    "list for a:\n"
    "  c1 m45 *1 -10,20\n"
    "  c1 m45 *1 -10,20[a=1,1, b=0,2, na=2, nb=3]\n"
    "list for b:\n"
    "  c1 m45 *1 -10,20\n"
    "Not in b but in a:\n"
    "  c1 m45 *1 -10,20[a=1,1, b=0,2, na=2, nb=3]\n"
    "  c6 m45 *1 -10,20\n"
    "Not in a but in b:\n"
  );
}

//  meta info
TEST(9)
{
  db::Layout a;
  db::cell_index_type caa = a.add_cell ("A");
  db::cell_index_type cab = a.add_cell ("B");

  db::Layout b;
  db::cell_index_type cba = b.add_cell ("A");
  db::cell_index_type cbb = b.add_cell ("B");

  a.add_meta_info ("x", db::MetaInfo ("", 17.0, true));
  a.add_meta_info ("y", db::MetaInfo ("", -1.0, false));  // not persisted
  a.add_meta_info ("z", db::MetaInfo ("", -1.0, true));
  a.add_meta_info (caa, "a1", db::MetaInfo ("", "a", true));
  a.add_meta_info (caa, "a2", db::MetaInfo ("", 42, false));  // not persisted
  a.add_meta_info (caa, "a3", db::MetaInfo ("", 41, true));
  a.add_meta_info (cab, "b1", db::MetaInfo ("", "b", true));
  a.add_meta_info (cab, "b2", db::MetaInfo ("", 3, false));  // not persisted
  a.add_meta_info (cab, "b3", db::MetaInfo ("", "q", true));

  b.add_meta_info ("x", db::MetaInfo ("", 21.0, true));
  b.add_meta_info ("y", db::MetaInfo ("", -1.0, true));
  b.add_meta_info (cba, "a1", db::MetaInfo ("", "aa", true));
  b.add_meta_info (cba, "a2", db::MetaInfo ("", 42, true));
  b.add_meta_info (cbb, "b1", db::MetaInfo ("", "bb", true));
  b.add_meta_info (cbb, "b2", db::MetaInfo ("", 3, true));

  TestDifferenceReceiver r;
  bool eq = db::compare_layouts (a, b, db::layout_diff::f_verbose | db::layout_diff::f_with_meta, 0, r);
  EXPECT_EQ (eq, false);
  EXPECT_EQ (r.text (),
    "layout_diff: global meta info differs x: 17 vs. 21\n"
    "layout_diff: global meta info differs y: nil vs. -1\n"
    "layout_diff: global meta info differs z: -1 vs. nil\n"
    "layout_diff: cell meta info differs for cell A - a1: a vs. aa\n"
    "layout_diff: cell meta info differs for cell A - a2: nil vs. 42\n"
    "layout_diff: cell meta info differs for cell A - a3: 41 vs. nil\n"
    "layout_diff: cell meta info differs for cell B - b1: b vs. bb\n"
    "layout_diff: cell meta info differs for cell B - b2: nil vs. 3\n"
    "layout_diff: cell meta info differs for cell B - b3: q vs. nil\n"
  );
}
