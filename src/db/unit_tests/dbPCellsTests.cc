
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
#include "dbPCellHeader.h"
#include "dbPCellDeclaration.h"
#include "dbPCellVariant.h"
#include "dbWriter.h"
#include "dbReader.h"
#include "dbLayoutDiff.h"
#include "dbTestSupport.h"
#include "tlStream.h"
#include "tlUnitTest.h"

class PD 
  : public db::PCellDeclaration
{
  virtual std::vector<db::PCellLayerDeclaration> get_layer_declarations (const db::pcell_parameters_type &) const
  {
    std::vector<db::PCellLayerDeclaration> layers;

    layers.push_back(db::PCellLayerDeclaration ());
    layers.back ().symbolic = "gate";
    layers.back ().layer = 16;
    layers.back ().datatype = 0;

    layers.push_back(db::PCellLayerDeclaration ());
    layers.back ().symbolic = "metal0";
    layers.back ().layer = 24;
    layers.back ().datatype = 0;

    layers.push_back(db::PCellLayerDeclaration ());
    layers.back ().symbolic = "cont";
    layers.back ().layer = 23;
    layers.back ().datatype = 0;

    return layers;
  }

  virtual std::vector<db::PCellParameterDeclaration> get_parameter_declarations () const
  {
    std::vector<db::PCellParameterDeclaration> parameters;

    parameters.push_back (db::PCellParameterDeclaration ("length"));
    parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
    parameters.push_back (db::PCellParameterDeclaration ("width"));
    parameters.back ().set_type (db::PCellParameterDeclaration::t_double);
    parameters.push_back (db::PCellParameterDeclaration ("orientation"));
    parameters.back ().set_type (db::PCellParameterDeclaration::t_int);

    return parameters;
  }

  virtual void produce (const db::Layout &layout, const std::vector<unsigned int> &layer_ids, const db::pcell_parameters_type &parameters, db::Cell &cell) const
  {
    db::Coord width = db::coord_traits<db::Coord>::rounded (parameters[0].to_double () / layout.dbu ());
    db::Coord height = db::coord_traits<db::Coord>::rounded (parameters[1].to_double () / layout.dbu ());

    int orientation = parameters[2].to_long ();

    //unsigned int l_gate = layer_ids[0];
    unsigned int l_metal0 = layer_ids[1];
    //unsigned int l_cont = layer_ids[2];

    const db::Cell &cell_a = layout.cell (layout.cell_by_name ("A").second);

    cell.insert (db::CellInstArray (db::CellInst (cell_a.cell_index ()), db::Trans (orientation, db::Vector (width / 2 - 50, height / 2 - 100))));

    cell.shapes (l_metal0).insert (db::Box (0, 0, width, height));
  }
};

TEST(0) 
{
  db::Manager m (true);
  db::Layout layout(&m);
  layout.dbu (0.001);
  db::Layout layout_au(&m);
  layout_au.dbu (0.001);

  //  Note: this sample requires the BASIC lib

  {
    std::string fn (tl::testdata ());
    fn += "/gds/pcell_test_0.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout);
  }

  CHECKPOINT ();
  db::compare_layouts (_this, layout, tl::testdata () + "/gds/pcell_test_0_au.gds", db::NoNormalization);
}

TEST(1) 
{
  db::Manager m (true);
  db::Layout layout(&m);
  layout.dbu (0.001);
  
  db::LayerProperties p;

  p.layer = 23;
  p.datatype = 0;
  unsigned int l_cont = layout.insert_layer (p);

  p.layer = 16;
  p.datatype = 0;
  unsigned int l_gate = layout.insert_layer (p);

  db::Cell &cell_a = layout.cell (layout.add_cell ("A"));
  cell_a.shapes(l_cont).insert(db::Box (50, 50, 150, 150));
  cell_a.shapes(l_gate).insert(db::Box (0, 0, 200, 1000));

  db::Cell &top = layout.cell (layout.add_cell ("TOP"));

  db::pcell_id_type pd = layout.register_pcell ("PD", new PD ());

  std::vector<tl::Variant> parameters;
  parameters.push_back (tl::Variant ());
  parameters.push_back (tl::Variant ());
  parameters.push_back (tl::Variant ());
  tl::Variant &width = parameters[0];
  tl::Variant &height = parameters[1];
  tl::Variant &orientation = parameters[2];

  width = 0.5;
  height = 1.0;
  orientation = long (0);

  db::cell_index_type pd1 = layout.get_pcell_variant (pd, parameters);
  db::Instance i1 = top.insert (db::CellInstArray (db::CellInst (pd1), db::Trans (db::Vector (0, 0))));

  width = width.to_double () * 0.1;
  width = width.to_double () * 10.0;

  db::cell_index_type pd2 = layout.get_pcell_variant (pd, parameters);
  db::Instance i2 = top.insert (db::CellInstArray (db::CellInst (pd2), db::Trans (db::Vector (0, 2000))));

  EXPECT_EQ (pd1, pd2);

  width = 0.4;
  height = 0.8;
  orientation = long (1);

  db::cell_index_type pd3 = layout.get_pcell_variant (pd, parameters);
  db::Instance i3 = top.insert (db::CellInstArray (db::CellInst (pd3), db::Trans (db::Vector (2000, 0))));

  EXPECT_NE (pd2, pd3);

  EXPECT_EQ (layout.get_properties(0).to_string (), "23/0");
  EXPECT_EQ (layout.get_properties(1).to_string (), "16/0");
  EXPECT_EQ (layout.get_properties(2).to_string (), "24/0");

  CHECKPOINT ();
  db::compare_layouts (_this, layout, tl::testdata () + "/gds/pcell_test.gds", db::NoNormalization);

  // if not in editable mode, we could have lost the reference to the second instance
  if (db::default_editable_mode ()) {

    m.transaction ("x");

    i2 = top.change_pcell_parameters (i2, parameters);
    EXPECT_EQ (i2.cell_index (), pd3);
    EXPECT_NE (i2.cell_index (), pd1);
    
    CHECKPOINT ();
    db::compare_layouts (_this, layout, tl::testdata () + "/gds/pcell_test2.gds", db::NoNormalization);

    width = 1.0; 
    i1 = top.change_pcell_parameters (i1, parameters);
    EXPECT_NE (i1.cell_index (), pd3);
    EXPECT_NE (i1.cell_index (), pd1);
    
    CHECKPOINT ();
    db::compare_layouts (_this, layout, tl::testdata () + "/gds/pcell_test3.gds", db::WriteGDS2);
    CHECKPOINT ();
    db::compare_layouts (_this, layout, tl::testdata () + "/gds/pcell_test3.gds", db::WriteOAS);

    m.commit ();

    m.undo ();

    CHECKPOINT ();
    db::compare_layouts (_this, layout, tl::testdata () + "/gds/pcell_test.gds", db::NoNormalization);

    m.redo ();

    CHECKPOINT ();
    db::compare_layouts (_this, layout, tl::testdata () + "/gds/pcell_test3.gds", db::WriteGDS2);
    CHECKPOINT ();
    db::compare_layouts (_this, layout, tl::testdata () + "/gds/pcell_test3.gds", db::WriteOAS);

    m.undo (); // test the ability to destroy things stored in the transaction

    CHECKPOINT ();
    db::compare_layouts (_this, layout, tl::testdata () + "/gds/pcell_test.gds", db::NoNormalization);

    //  Test the ability to copy things and change PCell parameters then
    db::Layout copy (layout);

    CHECKPOINT ();
    db::compare_layouts (_this, copy, tl::testdata () + "/gds/pcell_test.gds", db::NoNormalization);

    db::Cell &copy_top = copy.cell (top.cell_index ());

    db::Instance i1_copy = *(copy_top.begin ());
    const db::PCellVariant *pcv = dynamic_cast<const db::PCellVariant *> (&copy.cell (i1_copy.cell_index()));

    EXPECT_EQ (pcv != 0, true);
    EXPECT_EQ (copy_top.is_pcell_instance (i1_copy).first, true);

    EXPECT_EQ (copy.pcell_by_name ("PD").first, true);
    db::pcell_id_type pd_id_copy = copy.pcell_by_name ("PD").second;

    EXPECT_EQ (copy_top.is_pcell_instance (i1_copy).second, pd_id_copy);

    std::vector<tl::Variant> parameters = copy_top.get_pcell_parameters (i1_copy);
    EXPECT_EQ (parameters.size (), size_t (3));
    EXPECT_EQ (std::string (parameters[0].to_string()), "0.4");
    EXPECT_EQ (std::string (parameters[1].to_string()), "0.8");
    EXPECT_EQ (std::string (parameters[2].to_string()), "1");

    parameters[0] = 1.5;
    i1_copy = copy_top.change_pcell_parameters (i1_copy, parameters);
    
    CHECKPOINT ();
    db::compare_layouts (_this, copy, tl::testdata () + "/gds/pcell_test4.gds", db::WriteGDS2);
    CHECKPOINT ();
    db::compare_layouts (_this, copy, tl::testdata () + "/gds/pcell_test4.gds", db::WriteOAS);

  }
}

