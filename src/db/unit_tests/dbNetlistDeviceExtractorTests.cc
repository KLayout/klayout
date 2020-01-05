
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

#include "dbNetlistDeviceExtractor.h"
#include "dbLoadLayoutOptions.h"
#include "dbReader.h"
#include "dbRecursiveShapeIterator.h"
#include "dbNetlistDeviceExtractorClasses.h"

#include "tlUnitTest.h"
#include "tlFileUtils.h"
#include "tlStream.h"

TEST(1_NetlistDeviceExtractorErrorBasic)
{
  db::NetlistDeviceExtractorError error;

  EXPECT_EQ (error.message (), "");
  error.set_message ("x");
  EXPECT_EQ (error.message (), "x");
  error.set_category_name ("cat");
  EXPECT_EQ (error.category_name (), "cat");
  error.set_category_description ("cdesc");
  EXPECT_EQ (error.category_description (), "cdesc");
  error.set_cell_name ("cell");
  EXPECT_EQ (error.cell_name (), "cell");
  error.set_geometry (db::DPolygon (db::DBox (0, 1, 2, 3)));
  EXPECT_EQ (error.geometry ().to_string (), "(0,1;0,3;2,3;2,1)");

  error = db::NetlistDeviceExtractorError ("cell2", "msg2");
  EXPECT_EQ (error.cell_name (), "cell2");
  EXPECT_EQ (error.message (), "msg2");
  EXPECT_EQ (error.category_name (), "");
  EXPECT_EQ (error.category_description (), "");
  EXPECT_EQ (error.geometry ().to_string (), "()");
}

namespace {
  class DummyDeviceExtractor
    : public db::NetlistDeviceExtractor
  {
  public:
    DummyDeviceExtractor ()
      : db::NetlistDeviceExtractor (std::string ("DUMMY"))
    {
      error ("msg1");
      error ("msg2", db::DPolygon (db::DBox (0, 1, 2, 3)));
      error ("cat1", "desc1", "msg1");
      error ("cat1", "desc1", "msg3", db::DPolygon (db::DBox (10, 11, 12, 13)));
    }
  };
}

static std::string error2string (const db::NetlistDeviceExtractorError &e)
{
  return e.cell_name() + ":" + e.category_name () + ":" + e.category_description () + ":" +
         e.geometry ().to_string () + ":" + e.message ();
}

TEST(2_NetlistDeviceExtractorErrors)
{
  DummyDeviceExtractor dummy_ex;

  EXPECT_EQ (dummy_ex.has_errors (), true);

  std::vector<db::NetlistDeviceExtractorError> errors (dummy_ex.begin_errors (), dummy_ex.end_errors ());
  EXPECT_EQ (int (errors.size ()), 4);
  EXPECT_EQ (error2string (errors [0]), ":::():msg1");
  EXPECT_EQ (error2string (errors [1]), ":::(0,1;0,3;2,3;2,1):msg2");
  EXPECT_EQ (error2string (errors [2]), ":cat1:desc1:():msg1");
  EXPECT_EQ (error2string (errors [3]), ":cat1:desc1:(10,11;10,13;12,13;12,11):msg3");
}

TEST(10_MOS3DeviceExtractorTest)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testsrc ());
    fn = tl::combine_path (fn, "testdata");
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "mos3_1.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::PolygonRef> cl;

  db::NetlistDeviceExtractorMOS3Transistor ex ("MOS3");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &l1;
  dl["G"] = &l2;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device MOS3 $1 (S=(null),G=(null),D=(null)) (L=0.3,W=0.8,AS=0.4,AD=0.16,PS=2.6,PD=2);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(-600,-200;-600,600;-100,600;-100,-200)");
  EXPECT_EQ (o2.to_string (), "(200,-200;200,600;400,600;400,-200)");
  EXPECT_EQ (o3.to_string (), "(-100,-200;-100,600;200,600;200,-200)");
}

TEST(11_MOS3DeviceExtractorTestNotRectangularGate)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testsrc ());
    fn = tl::combine_path (fn, "testdata");
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "mos3_2.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::PolygonRef> cl;

  db::NetlistDeviceExtractorMOS3Transistor ex ("MOS3");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &l1;
  dl["G"] = &l2;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device MOS3 $1 (S=(null),G=(null),D=(null)) (L=0.3,W=1,AS=0.32,AD=0.18,PS=2.6,PD=2.4);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(-600,-200;-600,600;-300,600;-300,200;-100,200;-100,-200)");
  EXPECT_EQ (o2.to_string (), "(200,-200;200,500;0,500;0,600;400,600;400,-200)");
  EXPECT_EQ (o3.to_string (), "(-100,-200;-100,200;-300,200;-300,600;0,600;0,500;200,500;200,-200)");
}

TEST(12_MOS3DeviceExtractorTestCircular)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testsrc ());
    fn = tl::combine_path (fn, "testdata");
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "mos3_3.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::PolygonRef> cl;

  db::NetlistDeviceExtractorMOS3Transistor ex ("MOS3");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &l1;
  dl["G"] = &l2;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device MOS3 $1 (S=(null),G=(null),D=(null)) (L=0.3,W=3.8,AS=0.4,AD=4.18,PS=2.6,PD=14.6);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(200,-200;200,600;700,600;700,-200)");
  EXPECT_EQ (o2.to_string (), "(-600,-1200;-600,1400;1600,1400;1600,-1200/-100,-500;1000,-500;1000,900;-100,900)");
  EXPECT_EQ (o3.to_string (), "(-100,-500;-100,900;1000,900;1000,-500/200,-200;700,-200;700,600;200,600)");
}

TEST(20_MOS4DeviceExtractorTest)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testsrc ());
    fn = tl::combine_path (fn, "testdata");
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "mos4_1.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region l3 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(3, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);
  db::Region o4 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::PolygonRef> cl;

  db::NetlistDeviceExtractorMOS4Transistor ex ("MOS4");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &l1;
  dl["G"] = &l2;
  dl["W"] = &l3;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  dl["tB"] = &o4;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device MOS4 $1 (S=(null),G=(null),D=(null),B=(null)) (L=0.3,W=0.8,AS=0.4,AD=0.16,PS=2.6,PD=2);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(-600,-200;-600,600;-100,600;-100,-200)");
  EXPECT_EQ (o2.to_string (), "(200,-200;200,600;400,600;400,-200)");
  EXPECT_EQ (o3.to_string (), "(-100,-200;-100,600;200,600;200,-200)");
  EXPECT_EQ (o4.to_string (), "(-100,-200;-100,600;200,600;200,-200)");
}

TEST(21_MOS4DeviceExtractorTestNotRectangularGate)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testsrc ());
    fn = tl::combine_path (fn, "testdata");
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "mos4_2.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region l3 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(3, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);
  db::Region o4 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::PolygonRef> cl;

  db::NetlistDeviceExtractorMOS4Transistor ex ("MOS4");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &l1;
  dl["G"] = &l2;
  dl["W"] = &l3;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  dl["tB"] = &o4;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device MOS4 $1 (S=(null),G=(null),D=(null),B=(null)) (L=0.3,W=1,AS=0.32,AD=0.18,PS=2.6,PD=2.4);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(-600,-200;-600,600;-300,600;-300,200;-100,200;-100,-200)");
  EXPECT_EQ (o2.to_string (), "(200,-200;200,500;0,500;0,600;400,600;400,-200)");
  EXPECT_EQ (o3.to_string (), "(-100,-200;-100,200;-300,200;-300,600;0,600;0,500;200,500;200,-200)");
  EXPECT_EQ (o4.to_string (), "(-100,-200;-100,200;-300,200;-300,600;0,600;0,500;200,500;200,-200)");
}

TEST(22_MOS4DeviceExtractorTestCircular)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testsrc ());
    fn = tl::combine_path (fn, "testdata");
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "mos4_3.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region l3 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(3, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);
  db::Region o4 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::PolygonRef> cl;

  db::NetlistDeviceExtractorMOS4Transistor ex ("MOS4");

  db::NetlistDeviceExtractor::input_layers dl;

  dl["SD"] = &l1;
  dl["G"] = &l2;
  dl["W"] = &l3;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  dl["tB"] = &o4;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device MOS4 $1 (S=(null),G=(null),D=(null),B=(null)) (L=0.3,W=3.8,AS=0.4,AD=4.18,PS=2.6,PD=14.6);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(200,-200;200,600;700,600;700,-200)");
  EXPECT_EQ (o2.to_string (), "(-600,-1200;-600,1400;1600,1400;1600,-1200/-100,-500;1000,-500;1000,900;-100,900)");
  EXPECT_EQ (o3.to_string (), "(-100,-500;-100,900;1000,900;1000,-500/200,-200;700,-200;700,600;200,600)");
  EXPECT_EQ (o4.to_string (), "(-100,-500;-100,900;1000,900;1000,-500/200,-200;700,-200;700,600;200,600)");
}

TEST(30_DMOS3DeviceExtractorTest)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testsrc ());
    fn = tl::combine_path (fn, "testdata");
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "dmos3_1.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l0 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(0, 0))), dss);
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::PolygonRef> cl;

  db::NetlistDeviceExtractorMOS3Transistor ex ("DMOS3", true);

  db::NetlistDeviceExtractor::input_layers dl;

  dl["S"] = &l0;
  dl["D"] = &l1;
  dl["G"] = &l2;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device DMOS3 $1 (S=(null),G=(null),D=(null)) (L=0.3,W=0.8,AS=0.4,AD=0.16,PS=2.6,PD=2);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(-600,-200;-600,600;-100,600;-100,-200)");
  EXPECT_EQ (o2.to_string (), "(200,-200;200,600;400,600;400,-200)");
  EXPECT_EQ (o3.to_string (), "(-100,-200;-100,600;200,600;200,-200)");
}

TEST(31_DMOS3DeviceExtractorTestNotRectangularGate)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testsrc ());
    fn = tl::combine_path (fn, "testdata");
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "dmos3_2.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l0 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(0, 0))), dss);
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::PolygonRef> cl;

  db::NetlistDeviceExtractorMOS3Transistor ex ("DMOS3", true);

  db::NetlistDeviceExtractor::input_layers dl;

  dl["S"] = &l0;
  dl["D"] = &l1;
  dl["G"] = &l2;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device DMOS3 $1 (S=(null),G=(null),D=(null)) (L=0.3,W=1,AS=0.32,AD=0.18,PS=2.6,PD=2.4);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(-600,-200;-600,600;-300,600;-300,200;-100,200;-100,-200)");
  EXPECT_EQ (o2.to_string (), "(200,-200;200,500;0,500;0,600;400,600;400,-200)");
  EXPECT_EQ (o3.to_string (), "(-100,-200;-100,200;-300,200;-300,600;0,600;0,500;200,500;200,-200)");
}

TEST(32_DMOS3DeviceExtractorTestCircular)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testsrc ());
    fn = tl::combine_path (fn, "testdata");
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "dmos3_3.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l0 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(0, 0))), dss);
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::PolygonRef> cl;

  db::NetlistDeviceExtractorMOS3Transistor ex ("DMOS3", true);

  db::NetlistDeviceExtractor::input_layers dl;

  dl["S"] = &l0;
  dl["D"] = &l1;
  dl["G"] = &l2;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device DMOS3 $1 (S=(null),G=(null),D=(null)) (L=0.3,W=3.8,AS=0.4,AD=4.18,PS=2.6,PD=14.6);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(200,-200;200,600;700,600;700,-200)");
  EXPECT_EQ (o2.to_string (), "(-600,-1200;-600,1400;1600,1400;1600,-1200/-100,-500;1000,-500;1000,900;-100,900)");
  EXPECT_EQ (o3.to_string (), "(-100,-500;-100,900;1000,900;1000,-500/200,-200;700,-200;700,600;200,600)");
}

TEST(40_DMOS4DeviceExtractorTest)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testsrc ());
    fn = tl::combine_path (fn, "testdata");
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "dmos4_1.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l0 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(0, 0))), dss);
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region l3 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(3, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);
  db::Region o4 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::PolygonRef> cl;

  db::NetlistDeviceExtractorMOS4Transistor ex ("DMOS4", true);

  db::NetlistDeviceExtractor::input_layers dl;

  dl["S"] = &l0;
  dl["D"] = &l1;
  dl["G"] = &l2;
  dl["W"] = &l3;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  dl["tB"] = &o4;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device DMOS4 $1 (S=(null),G=(null),D=(null),B=(null)) (L=0.3,W=0.8,AS=0.4,AD=0.16,PS=2.6,PD=2);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(-600,-200;-600,600;-100,600;-100,-200)");
  EXPECT_EQ (o2.to_string (), "(200,-200;200,600;400,600;400,-200)");
  EXPECT_EQ (o3.to_string (), "(-100,-200;-100,600;200,600;200,-200)");
  EXPECT_EQ (o4.to_string (), "(-100,-200;-100,600;200,600;200,-200)");
}

TEST(41_DMOS4DeviceExtractorTestNotRectangularGate)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testsrc ());
    fn = tl::combine_path (fn, "testdata");
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "dmos4_2.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l0 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(0, 0))), dss);
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region l3 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(3, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);
  db::Region o4 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::PolygonRef> cl;

  db::NetlistDeviceExtractorMOS4Transistor ex ("DMOS4", true);

  db::NetlistDeviceExtractor::input_layers dl;

  dl["S"] = &l0;
  dl["D"] = &l1;
  dl["G"] = &l2;
  dl["W"] = &l3;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  dl["tB"] = &o4;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device DMOS4 $1 (S=(null),G=(null),D=(null),B=(null)) (L=0.3,W=1,AS=0.32,AD=0.18,PS=2.6,PD=2.4);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(-600,-200;-600,600;-300,600;-300,200;-100,200;-100,-200)");
  EXPECT_EQ (o2.to_string (), "(200,-200;200,500;0,500;0,600;400,600;400,-200)");
  EXPECT_EQ (o3.to_string (), "(-100,-200;-100,200;-300,200;-300,600;0,600;0,500;200,500;200,-200)");
  EXPECT_EQ (o4.to_string (), "(-100,-200;-100,200;-300,200;-300,600;0,600;0,500;200,500;200,-200)");
}

TEST(42_DMOS4DeviceExtractorTestCircular)
{
  db::Layout ly;

  {
    db::LoadLayoutOptions options;

    std::string fn (tl::testsrc ());
    fn = tl::combine_path (fn, "testdata");
    fn = tl::combine_path (fn, "algo");
    fn = tl::combine_path (fn, "dmos4_3.gds");

    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly, options);
  }

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;
  dss.set_text_enlargement (1);
  dss.set_text_property_name (tl::Variant ("LABEL"));

  //  original layers
  db::Region l0 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(0, 0))), dss);
  db::Region l1 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(1, 0))), dss);
  db::Region l2 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(2, 0))), dss);
  db::Region l3 (db::RecursiveShapeIterator (ly, tc, ly.get_layer (db::LayerProperties(3, 0))), dss);
  db::Region o1 (dss);
  db::Region o2 (dss);
  db::Region o3 (dss);
  db::Region o4 (dss);

  //  perform the extraction

  db::Netlist nl;
  db::hier_clusters<db::PolygonRef> cl;

  db::NetlistDeviceExtractorMOS4Transistor ex ("DMOS4", true);

  db::NetlistDeviceExtractor::input_layers dl;

  dl["S"] = &l0;
  dl["D"] = &l1;
  dl["G"] = &l2;
  dl["W"] = &l3;
  dl["tS"] = &o1;
  dl["tD"] = &o2;
  dl["tG"] = &o3;
  dl["tB"] = &o4;
  ex.extract (dss, 0, dl, nl, cl);

  EXPECT_EQ (nl.to_string (),
    "circuit TOP ();\n"
    "  device DMOS4 $1 (S=(null),G=(null),D=(null),B=(null)) (L=0.3,W=3.8,AS=0.4,AD=4.18,PS=2.6,PD=14.6);\n"
    "end;\n"
  );
  EXPECT_EQ (o1.to_string (), "(200,-200;200,600;700,600;700,-200)");
  EXPECT_EQ (o2.to_string (), "(-600,-1200;-600,1400;1600,1400;1600,-1200/-100,-500;1000,-500;1000,900;-100,900)");
  EXPECT_EQ (o3.to_string (), "(-100,-500;-100,900;1000,900;1000,-500/200,-200;700,-200;700,600;200,600)");
  EXPECT_EQ (o4.to_string (), "(-100,-500;-100,900;1000,900;1000,-500/200,-200;700,-200;700,600;200,600)");
}
