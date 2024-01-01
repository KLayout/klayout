
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


#include "tlUnitTest.h"
#include "tlStream.h"
#include "tlFileUtils.h"
#include "dbLayoutToNetlist.h"
#include "dbTestSupport.h"
#include "dbReader.h"
#include "dbWriter.h"
#include "dbCommonReader.h"

static unsigned int define_layer (db::Layout &ly, db::LayerMap &lmap, int gds_layer, int gds_datatype = 0)
{
  unsigned int lid = ly.insert_layer (db::LayerProperties (gds_layer, gds_datatype));
  lmap.map (ly.get_properties (lid), lid);
  return lid;
}

db::Region make_region (const db::RecursiveShapeIterator &si, db::DeepShapeStore &dss, bool flat)
{
  return flat ? db::Region (si) : db::Region (si, dss);
}

void run_test (tl::TestBase *_this, bool flat, bool flat_nets, const std::string &au_fn)
{
  db::Layout ly;
  db::DeepShapeStore dss;
  if (! flat_nets) {
    dss.set_subcircuit_hierarchy_for_nets (true);
  }

  db::LayerMap lmap;

  unsigned int poly       = define_layer (ly, lmap, 1);
  unsigned int cont       = define_layer (ly, lmap, 2);
  unsigned int metal1     = define_layer (ly, lmap, 3);
  unsigned int via1       = define_layer (ly, lmap, 4);
  unsigned int metal2     = define_layer (ly, lmap, 5);
  unsigned int via2       = define_layer (ly, lmap, 6);
  unsigned int metal3     = define_layer (ly, lmap, 7);
  unsigned int via3       = define_layer (ly, lmap, 8);
  unsigned int metal4     = define_layer (ly, lmap, 9);

  {
    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

    std::string fn = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), "nets_proc_1.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::Region rpoly    = make_region (db::RecursiveShapeIterator (ly, tc, poly),   dss, flat);
  db::Region rcont    = make_region (db::RecursiveShapeIterator (ly, tc, cont),   dss, flat);
  db::Region rmetal1  = make_region (db::RecursiveShapeIterator (ly, tc, metal1), dss, flat);
  db::Region rvia1    = make_region (db::RecursiveShapeIterator (ly, tc, via1),   dss, flat);
  db::Region rmetal2  = make_region (db::RecursiveShapeIterator (ly, tc, metal2), dss, flat);
  db::Region rvia2    = make_region (db::RecursiveShapeIterator (ly, tc, via2),   dss, flat);
  db::Region rmetal3  = make_region (db::RecursiveShapeIterator (ly, tc, metal3), dss, flat);
  db::Region rvia3    = make_region (db::RecursiveShapeIterator (ly, tc, via3),   dss, flat);
  db::Region rmetal4  = make_region (db::RecursiveShapeIterator (ly, tc, metal4), dss, flat);

  std::unique_ptr<db::LayoutToNetlist> l2n;
  if (! flat) {
    l2n.reset (new db::LayoutToNetlist (&dss));
    EXPECT_EQ (dss.has_net_builder_for (0, l2n.get ()), false);
  } else {
    l2n.reset (new db::LayoutToNetlist (ly.cell_name (tc.cell_index ()), ly.dbu ()));
  }

  //  net extraction

  if (flat) {
    //  flat or original layers need to be registered
    l2n->register_layer (rpoly);
    l2n->register_layer (rcont);
    l2n->register_layer (rmetal1);
    l2n->register_layer (rvia1);
    l2n->register_layer (rmetal2);
    l2n->register_layer (rvia2);
    l2n->register_layer (rmetal3);
    l2n->register_layer (rvia3);
    l2n->register_layer (rmetal4);
  }

  //  Intra-layer
  l2n->connect (rpoly);
  l2n->connect (rcont);
  l2n->connect (rmetal1);
  l2n->connect (rvia1);
  l2n->connect (rmetal2);
  l2n->connect (rvia2);
  l2n->connect (rmetal3);
  l2n->connect (rvia3);
  l2n->connect (rmetal4);
  //  Inter-layer
  l2n->connect (rpoly,      rcont);
  l2n->connect (rcont,      rmetal1);
  l2n->connect (rmetal1,    rvia1);
  l2n->connect (rvia1,      rmetal2);
  l2n->connect (rmetal2,    rvia2);
  l2n->connect (rvia2,      rmetal3);
  l2n->connect (rmetal3,    rvia3);
  l2n->connect (rvia3,      rmetal4);

  l2n->extract_netlist ();

  db::Region rmetal1_nets = rmetal1.nets (*l2n, db::NPM_NetQualifiedNameOnly, tl::Variant (1));
  if (! flat) {
    EXPECT_EQ (dss.has_net_builder_for (0, l2n.get ()), true);
  }
  db::Region rmetal2_nets = rmetal2.nets (*l2n, db::NPM_NetQualifiedNameOnly, tl::Variant (1));

  db::Region res1 = rmetal1_nets.bool_and (rmetal2_nets, db::SamePropertiesConstraint);
  db::Region res2 = rmetal1_nets.bool_and (rmetal2_nets, db::DifferentPropertiesConstraint);
  db::Region res3 = rmetal1_nets.bool_and (rmetal2_nets, db::NoPropertyConstraint);

  rmetal1_nets.insert_into (&ly, tc.cell_index (), ly.insert_layer (db::LayerProperties (100, 0)));
  rmetal2_nets.insert_into (&ly, tc.cell_index (), ly.insert_layer (db::LayerProperties (101, 0)));

  res1.insert_into (&ly, tc.cell_index (), ly.insert_layer (db::LayerProperties (1000, 0)));
  res2.insert_into (&ly, tc.cell_index (), ly.insert_layer (db::LayerProperties (1001, 0)));
  res3.insert_into (&ly, tc.cell_index (), ly.insert_layer (db::LayerProperties (1002, 0)));

  //  Test auto-unregistration
  l2n.reset (0);
  if (! flat) {
    EXPECT_EQ (dss.has_net_builder_for (0, l2n.get ()), false);
  }

  std::string au_path = tl::combine_path (tl::combine_path (tl::testdata (), "algo"), au_fn);
  db::compare_layouts (_this, ly, au_path);
}

TEST(1_NetSpecificBoolFlat)
{
  run_test (_this, false, true, "net_proc_au1.gds");
}

TEST(2_NetSpecificBoolFlatNets)
{
  run_test (_this, false, true, "net_proc_au2.gds");
}

TEST(3_NetSpecificBoolFullyHier)
{
  run_test (_this, false, false, "net_proc_au3.gds");
}
