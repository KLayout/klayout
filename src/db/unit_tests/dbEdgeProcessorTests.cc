
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

#include "dbShapeProcessor.h"
#include "dbPolygon.h"
#include "dbPolygonGenerators.h"
#include "dbLayout.h"
#include "dbReader.h"
#include "dbCommonReader.h"
#include "dbLayoutDiff.h"
#include "dbTestSupport.h"
#include "dbSaveLayoutOptions.h"
#include "dbWriter.h"
#include "dbRecursiveShapeIterator.h"
#include "tlStream.h"
#include "tlTimer.h"

#include <vector>
#include <algorithm>

#include <stdlib.h>

void run_test_bool (tl::TestBase *_this, const char *file, int mode, bool min_coherence = true, const char *au_file1 = 0, const char *au_file2 = 0, const char *au_file3 = 0)
{
  db::Layout layout_org;

  {
    std::string fn (tl::testdata ());
    fn += "/bool/";
    fn += file;
    tl::InputStream stream (fn);
    db::Reader reader (stream);

    db::LayerMap lmap;
    unsigned int index;
    db::LayerProperties p;

    p.layer = 1;
    p.datatype = 0;
    lmap.map (db::LDPair (1, 0), index = layout_org.insert_layer ());
    layout_org.set_properties (index, p);

    p.layer = 2;
    p.datatype = 0;
    lmap.map (db::LDPair (2, 0), index = layout_org.insert_layer ());
    layout_org.set_properties (index, p);

    p.layer = 100;
    p.datatype = 0;
    lmap.map (db::LDPair (100, 0), index = layout_org.insert_layer ());
    layout_org.set_properties (index, p);

    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;
    reader.read (layout_org, options);
  }

  std::string au_fn1 (tl::testdata ());
  au_fn1 += "/bool/";
  au_fn1 += au_file1 ? au_file1 : file;

  std::string au_fn2 (tl::testdata ());
  au_fn2 += "/bool/";
  au_fn2 += au_file2 ? au_file2 : (au_file1 ? au_file1 : file);

  int la = -1;
  for (unsigned int l = 0; l < layout_org.layers (); ++l) {
    if (layout_org.is_valid_layer (l) && layout_org.get_properties (l).layer == 1 
                                      && layout_org.get_properties (l).datatype == 0) {
      la = l;
      break;
    }
  }
  if (la < 0) {
    la = layout_org.insert_layer ();
  }

  int lb = -1;
  for (unsigned int l = 0; l < layout_org.layers (); ++l) {
    if (layout_org.is_valid_layer (l) && layout_org.get_properties (l).layer == 2 
                                      && layout_org.get_properties (l).datatype == 0) {
      lb = l;
      break;
    }
  }
  if (lb < 0) {
    lb = layout_org.insert_layer ();
  }

  int lr = -1;
  for (unsigned int l = 0; l < layout_org.layers (); ++l) {
    if (layout_org.is_valid_layer (l) && layout_org.get_properties (l).layer == 100 
                                      && layout_org.get_properties (l).datatype == 0) {
      lr = l;
      break;
    }
  }
  if (lr >= 0) {
    layout_org.delete_layer (lr);
  }
  lr = layout_org.insert_layer ();

  db::LayerProperties p;

  p.layer = 1;
  p.datatype = 0;
  layout_org.set_properties (la, p);

  p.layer = 2;
  p.datatype = 0;
  layout_org.set_properties (lb, p);

  p.layer = 100;
  p.datatype = 0;
  layout_org.set_properties (lr, p);

  db::ShapeProcessor proc;
  proc.boolean (layout_org, layout_org.cell (*layout_org.begin_top_down ()), la, 
                layout_org, layout_org.cell (*layout_org.begin_top_down ()), lb, 
                layout_org.cell (*layout_org.begin_top_down ()).shapes (lr), mode, true /*hierarchical*/, true /*resolve holes*/, min_coherence);

  db::LayerMap lmap;
  //  Note: the logical layers have to be non-existing ones because we read to a layout that has been 
  //  configured with layers already
  lmap.map (db::LDPair (1, 0), 1000);
  lmap.map (db::LDPair (2, 0), 1001);
  lmap.map (db::LDPair (100, 0), 1002);

  db::compare_layouts (_this, layout_org, au_fn1, lmap, false /*skip other layers*/, db::WriteOAS);

  layout_org.cell (*layout_org.begin_top_down ()).shapes (lr).clear ();

  unsigned int lr2 = layout_org.insert_layer ();
  //  temporarily disable compression for the boolean step to acchieve identical results with the previous test
  db::PolygonGenerator::enable_compression_global (false);
  proc.boolean (layout_org, layout_org.cell (*layout_org.begin_top_down ()), la, 
                layout_org, layout_org.cell (*layout_org.begin_top_down ()), lb, 
                layout_org.cell (*layout_org.begin_top_down ()).shapes (lr2), mode, true /*hierarchical*/, false /*resolve holes*/);
  db::PolygonGenerator::enable_compression_global (true);

  proc.merge (layout_org, layout_org.cell (*layout_org.begin_top_down ()), lr2,
              layout_org.cell (*layout_org.begin_top_down ()).shapes (lr), true /*hierarchical*/, 0 /*all polygons*/, true /*resolve holes*/, min_coherence);

  db::compare_layouts (_this, layout_org, au_fn2, lmap, false /*skip other layers*/, db::WriteOAS);

  //  Use this opportunity to test trapezoid decomposition
  if (au_file3) {

    std::string au_fn3 (tl::testdata ());
    au_fn3 += "/bool/";
    au_fn3 += au_file3;

    db::EdgeProcessor ep;
    db::Shapes &shapes = layout_org.cell (*layout_org.begin_top_down ()).shapes (lr);
    for (db::ShapeIterator s = shapes.begin (db::ShapeIterator::All); !s.at_end (); ++s) {
      ep.insert (s->polygon ());
    }

    db::MergeOp op;
    db::ShapeGenerator sg (shapes, true /*clear shapes*/);
    db::TrapezoidGenerator out (sg);
    ep.process (out, op);

    db::compare_layouts (_this, layout_org, au_fn3, lmap, false /*skip other layers*/, db::WriteOAS);

  }
}

TEST(1and) 
{
  run_test_bool (_this, "and1.oas", db::BooleanOp::And, true, 0, 0, "and1_tz.oas");
}

TEST(2and) 
{
  run_test_bool (_this, "and2.oas", db::BooleanOp::And, true, 0, 0, "and2_tz.oas");
}

TEST(3and) 
{
  run_test_bool (_this, "and3.oas", db::BooleanOp::And, true, 0, 0, "and3_tz.oas");
}

TEST(4and) 
{
  run_test_bool (_this, "and4.oas", db::BooleanOp::And, true, 0, 0, "and4_tz.oas");
}

TEST(5and) 
{
  run_test_bool (_this, "and5.oas", db::BooleanOp::And, true, 0, 0, "and5_tz.oas");
}

TEST(6and) 
{
  run_test_bool (_this, "and6.oas", db::BooleanOp::And, true, 0, 0, "and6_tz.oas");
}


TEST(1xor) 
{
  run_test_bool (_this, "xor1.oas", db::BooleanOp::Xor, true, 0, 0, "xor1_tz.oas");
}

TEST(2xor) 
{
  run_test_bool (_this, "xor2.oas", db::BooleanOp::Xor, true, 0, 0, "xor2_tz.oas");
}

TEST(3xor) 
{
  run_test_bool (_this, "xor3.oas", db::BooleanOp::Xor, true, 0, 0, "xor3_tz.oas");
}

TEST(4xor) 
{
  run_test_bool (_this, "xor4.oas", db::BooleanOp::Xor, true, 0, 0, "xor4_tz.oas");
}

TEST(5xor) 
{
  run_test_bool (_this, "xor5.oas", db::BooleanOp::Xor, true, 0, 0, "xor5_tz.oas");
}

TEST(6xor) 
{
  run_test_bool (_this, "xor6.oas", db::BooleanOp::Xor, true, 0, 0, "xor6_tz.oas");
}

TEST(7xor) 
{
  run_test_bool (_this, "xor7.oas", db::BooleanOp::Xor, true, "xor7_au1.oas", "xor7_au2.oas", "xor7_au_tz.oas");
}

TEST(8xor) 
{
  run_test_bool (_this, "xor8.oas", db::BooleanOp::Xor, true, "xor8_au1.oas", "xor8_au2.oas", "xor8_au_tz.oas");
}


TEST(1xor_max) 
{
  run_test_bool (_this, "xor1_max.oas", db::BooleanOp::Xor, false);
}

TEST(2xor_max) 
{
  run_test_bool (_this, "xor2_max.oas", db::BooleanOp::Xor, false);
}

TEST(3xor_max) 
{
  run_test_bool (_this, "xor3_max.oas", db::BooleanOp::Xor, false);
}

TEST(4xor_max) 
{
  run_test_bool (_this, "xor4_max.oas", db::BooleanOp::Xor, false);
}

TEST(5xor_max) 
{
  run_test_bool (_this, "xor5_max.oas", db::BooleanOp::Xor, false);
}

TEST(6xor_max) 
{
  run_test_bool (_this, "xor6_max.oas", db::BooleanOp::Xor, false);
}

TEST(7xor_max) 
{
  run_test_bool (_this, "xor7_max.oas", db::BooleanOp::Xor, false, "xor7_max_au1.oas", "xor7_max_au2.oas");
}


TEST(1or) 
{
  run_test_bool (_this, "or1.oas", db::BooleanOp::Or);
}

TEST(2or) 
{
  run_test_bool (_this, "or2.oas", db::BooleanOp::Or);
}

TEST(3or) 
{
  run_test_bool (_this, "or3.oas", db::BooleanOp::Or);
}

TEST(4or) 
{
  run_test_bool (_this, "or4.oas", db::BooleanOp::Or);
}

TEST(5or) 
{
  run_test_bool (_this, "or5.oas", db::BooleanOp::Or);
}

TEST(6or) 
{
  run_test_bool (_this, "or6.oas", db::BooleanOp::Or);
}


TEST(1anotb) 
{
  run_test_bool (_this, "anotb1.oas", db::BooleanOp::ANotB);
}

TEST(2anotb) 
{
  run_test_bool (_this, "anotb2.oas", db::BooleanOp::ANotB);
}

TEST(3anotb) 
{
  run_test_bool (_this, "anotb3.oas", db::BooleanOp::ANotB);
}

TEST(4anotb) 
{
  run_test_bool (_this, "anotb4.oas", db::BooleanOp::ANotB);
}

TEST(5anotb) 
{
  run_test_bool (_this, "anotb5.oas", db::BooleanOp::ANotB);
}

TEST(6anotb) 
{
  run_test_bool (_this, "anotb6.oas", db::BooleanOp::ANotB);
}


TEST(1bnota) 
{
  run_test_bool (_this, "bnota1.oas", db::BooleanOp::BNotA);
}

TEST(2bnota) 
{
  run_test_bool (_this, "bnota2.oas", db::BooleanOp::BNotA);
}

TEST(3bnota) 
{
  run_test_bool (_this, "bnota3.oas", db::BooleanOp::BNotA);
}

TEST(4bnota) 
{
  run_test_bool (_this, "bnota4.oas", db::BooleanOp::BNotA);
}

TEST(5bnota) 
{
  run_test_bool (_this, "bnota5.oas", db::BooleanOp::BNotA);
}

TEST(6bnota) 
{
  run_test_bool (_this, "bnota6.oas", db::BooleanOp::BNotA);
}

TEST(1special) 
{
  run_test_bool (_this, "special1.oas", db::BooleanOp::Xor, true, 0, 0, "special1_tz.oas");
}

TEST(2special) 
{
  test_is_long_runner ();
  tl::SelfTimer timer ("special2 test");
  run_test_bool (_this, "special2.oas", db::BooleanOp::Xor, true, "special2_au1.oas", 0, "special2_au1_tz.oas");
  run_test_bool (_this, "special2.oas", db::BooleanOp::And, true, "special2_au2.oas", 0, "special2_au2_tz.oas");
  run_test_bool (_this, "special2.oas", db::BooleanOp::ANotB, true, "special2_au3.oas", 0, "special2_au3_tz.oas");
  run_test_bool (_this, "special2.oas", db::BooleanOp::BNotA, true, "special2_au4.oas", 0, "special2_au4_tz.oas");
  run_test_bool (_this, "special2.oas", db::BooleanOp::Or, true, "special2_au5.oas", 0, "special2_au5_tz.oas");
}

TEST(3special) 
{
  test_is_long_runner ();
  tl::SelfTimer timer ("special3 test");
  run_test_bool (_this, "special3.oas", db::BooleanOp::Xor, true, "special3_au1.oas");
  run_test_bool (_this, "special3.oas", db::BooleanOp::And, true, "special3_au2.oas");
  run_test_bool (_this, "special3.oas", db::BooleanOp::ANotB, true, "special3_au3.oas");
  run_test_bool (_this, "special3.oas", db::BooleanOp::BNotA, true, "special3_au4.oas");
  run_test_bool (_this, "special3.oas", db::BooleanOp::Or, true, "special3_au5.oas");
}

void run_test_size (tl::TestBase *_this, const char *file, const char *au_file, int mode, db::Coord dx, db::Coord dy, bool min_coherence = true, bool flat = true)
{
  db::Manager m (true);

  db::Layout layout_org (&m);
  db::Layout layout_au (&m);

  {
    std::string fn (tl::testdata ());
    fn += "/bool/";
    fn += file;
    tl::InputStream stream (fn);
    db::Reader reader (stream);

    db::LayerMap lmap;
    unsigned int index;
    db::LayerProperties p;

    p.layer = 1;
    p.datatype = 0;
    lmap.map (db::LDPair (1, 0), index = layout_org.insert_layer ());
    layout_org.set_properties (index, p);

    p.layer = 100;
    p.datatype = 0;
    lmap.map (db::LDPair (100, 0), index = layout_org.insert_layer ());
    layout_org.set_properties (index, p);

    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;
    reader.read (layout_org, options);
  }

  std::string au_fn (tl::testdata ());
  au_fn += "/bool/";
  au_fn += au_file;

  int la = -1;
  for (unsigned int l = 0; l < layout_org.layers (); ++l) {
    if (layout_org.is_valid_layer (l) && layout_org.get_properties (l).layer == 1 
                                      && layout_org.get_properties (l).datatype == 0) {
      la = l;
      break;
    }
  }
  if (la < 0) {
    la = layout_org.insert_layer ();
  }

  int lr = -1;
  for (unsigned int l = 0; l < layout_org.layers (); ++l) {
    if (layout_org.is_valid_layer (l) && layout_org.get_properties (l).layer == 100 
                                      && layout_org.get_properties (l).datatype == 0) {
      lr = l;
      break;
    }
  }
  if (lr >= 0) {
    layout_org.delete_layer (lr);
  }
  lr = layout_org.insert_layer ();

  db::LayerProperties p;

  p.layer = 1;
  p.datatype = 0;
  layout_org.set_properties (la, p);

  p.layer = 100;
  p.datatype = 0;
  layout_org.set_properties (lr, p);

  db::ShapeProcessor proc;
  proc.size (layout_org, layout_org.cell (*layout_org.begin_top_down ()), la, 
             layout_org.cell (*layout_org.begin_top_down ()).shapes (lr), dx, dy, mode, flat /*hierarchical*/, true /*resolve holes*/, min_coherence);

  db::LayerMap lmap;
  lmap.map (db::LDPair (1, 0), 1);
  lmap.map (db::LDPair (100, 0), 2);

  db::compare_layouts (_this, layout_org, au_fn, lmap, false /*skip other layers*/, db::WriteOAS);
}

void run_test_twobool (tl::TestBase *_this, const char *file, const char *au_file)
{
  db::Manager m (true);

  db::Layout layout_org (&m);
  db::Layout layout_au (&m);

  {
    std::string fn (tl::testdata ());
    fn += "/bool/";
    fn += file;
    tl::InputStream stream (fn);
    db::Reader reader (stream);

    db::LayerMap lmap;
    unsigned int index;
    db::LayerProperties p;

    p.layer = 1;
    p.datatype = 0;
    lmap.map (db::LDPair (1, 0), index = layout_org.insert_layer ());
    layout_org.set_properties (index, p);

    p.layer = 2;
    p.datatype = 0;
    lmap.map (db::LDPair (2, 0), index = layout_org.insert_layer ());
    layout_org.set_properties (index, p);

    p.layer = 100;
    p.datatype = 0;
    lmap.map (db::LDPair (100, 0), index = layout_org.insert_layer ());
    layout_org.set_properties (index, p);

    p.layer = 101;
    p.datatype = 0;
    lmap.map (db::LDPair (101, 0), index = layout_org.insert_layer ());
    layout_org.set_properties (index, p);

    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;
    reader.read (layout_org, options);
  }

  std::string au_fn (tl::testdata ());
  au_fn += "/bool/";
  au_fn += au_file;

  int la = -1;
  for (unsigned int l = 0; l < layout_org.layers (); ++l) {
    if (layout_org.is_valid_layer (l) && layout_org.get_properties (l).layer == 1
                                      && layout_org.get_properties (l).datatype == 0) {
      la = l;
      break;
    }
  }
  if (la < 0) {
    la = layout_org.insert_layer ();
  }

  int lb = -1;
  for (unsigned int l = 0; l < layout_org.layers (); ++l) {
    if (layout_org.is_valid_layer (l) && layout_org.get_properties (l).layer == 2
                                      && layout_org.get_properties (l).datatype == 0) {
      lb = l;
      break;
    }
  }
  if (lb < 0) {
    lb = layout_org.insert_layer ();
  }

  int lra = -1;
  for (unsigned int l = 0; l < layout_org.layers (); ++l) {
    if (layout_org.is_valid_layer (l) && layout_org.get_properties (l).layer == 100
                                      && layout_org.get_properties (l).datatype == 0) {
      lra = l;
      break;
    }
  }
  if (lra >= 0) {
    layout_org.delete_layer (lra);
  }
  lra = layout_org.insert_layer ();

  int lrb = -1;
  for (unsigned int l = 0; l < layout_org.layers (); ++l) {
    if (layout_org.is_valid_layer (l) && layout_org.get_properties (l).layer == 101
                                      && layout_org.get_properties (l).datatype == 0) {
      lrb = l;
      break;
    }
  }
  if (lrb >= 0) {
    layout_org.delete_layer (lrb);
  }
  lrb = layout_org.insert_layer ();

  db::LayerProperties p;

  p.layer = 1;
  p.datatype = 0;
  layout_org.set_properties (la, p);

  p.layer = 2;
  p.datatype = 0;
  layout_org.set_properties (lb, p);

  p.layer = 100;
  p.datatype = 0;
  layout_org.set_properties (lra, p);

  p.layer = 101;
  p.datatype = 0;
  layout_org.set_properties (lrb, p);

  db::EdgeProcessor ep;

  size_t pn = 0;

  for (db::RecursiveShapeIterator iter (layout_org, layout_org.cell (*layout_org.begin_top_down ()), la); ! iter.at_end (); ++iter) {
    db::Polygon p;
    iter.shape ().polygon (p);
    p.transform (iter.trans ());
    ep.insert (p, pn);
    pn += 2;
  }

  pn = 1;

  for (db::RecursiveShapeIterator iter (layout_org, layout_org.cell (*layout_org.begin_top_down ()), lb); ! iter.at_end (); ++iter) {
    db::Polygon p;
    iter.shape ().polygon (p);
    p.transform (iter.trans ());
    ep.insert (p, pn);
    pn += 2;
  }

  db::ShapeGenerator sg1 (layout_org.cell (*layout_org.begin_top_down ()).shapes (lra), true /*clear shapes*/);
  db::PolygonGenerator pg1 (sg1, true /*resolve holes*/, false /*min. coherence*/);
  db::BooleanOp op1 (db::BooleanOp::And);

  db::ShapeGenerator sg2 (layout_org.cell (*layout_org.begin_top_down ()).shapes (lrb), true /*clear shapes*/);
  db::PolygonGenerator pg2 (sg2, true /*resolve holes*/, false /*min. coherence*/);
  db::BooleanOp op2 (db::BooleanOp::ANotB);

  std::vector<std::pair<db::EdgeSink *, db::EdgeEvaluatorBase *> > procs;
  procs.push_back (std::make_pair (&pg1, &op1));
  procs.push_back (std::make_pair (&pg2, &op2));
  ep.process (procs);

  db::LayerMap lmap;
  lmap.map (db::LDPair (1, 0), la);
  lmap.map (db::LDPair (2, 0), lb);
  lmap.map (db::LDPair (100, 0), lra);
  lmap.map (db::LDPair (101, 0), lrb);

  db::compare_layouts (_this, layout_org, au_fn, lmap, false /*skip other layers*/, db::WriteOAS);
}

TEST(1size)
{
  run_test_size (_this, "size1.oas", "size1_au.oas", 2, -1, -1);
}

TEST(2size)
{
  run_test_size (_this, "size2.oas", "size2_au.oas", 2, 1, 1);
}
//size5: 100: 0.002 (mode 0), 101: mode 1, 102: mode 2, .. 105; 200: -0.002 (mode 0), 201: mode 1, 202: mode2
//size6: 100: 0.002 (flat), 101: 0.002 (top cell), 102: 0.002 (cell by cell)

TEST(3size)
{
  run_test_size (_this, "size3.oas", "size3_au1.oas", 2, 10, 0);
  run_test_size (_this, "size3.oas", "size3_au2.oas", 2, -10, -50);
}

TEST(4size)
{
  run_test_size (_this, "size4.oas", "size4_au1.oas", 2, -10, -50);
  run_test_size (_this, "size4.oas", "size4_au2.oas", 2, 50, 10);
}

TEST(5size)
{
  run_test_size (_this, "size5.oas", "size5_au1.oas", 0, 2, 2);
  run_test_size (_this, "size5.oas", "size5_au2.oas", 1, 2, 2);
  run_test_size (_this, "size5.oas", "size5_au3.oas", 2, 2, 2);
  run_test_size (_this, "size5.oas", "size5_au4.oas", 3, 2, 2);
  run_test_size (_this, "size5.oas", "size5_au5.oas", 4, 2, 2);
  run_test_size (_this, "size5.oas", "size5_au6.oas", 5, 2, 2);
  run_test_size (_this, "size5.oas", "size5_au10.oas", 0, -2, -2);
  run_test_size (_this, "size5.oas", "size5_au11.oas", 1, -2, -2);
  run_test_size (_this, "size5.oas", "size5_au12.oas", 2, -2, -2);
}

TEST(6size)
{
  run_test_size (_this, "size6.oas", "size6_au1.oas", 2, 2, 2);
  run_test_size (_this, "size6.oas", "size6_au2.oas", 2, 2, 2, true, false/*top cell only*/);
  //  not tested: layer 102 (cell by cell)
}

TEST(7size)
{
  run_test_size (_this, "size7.oas", "size7_au1.oas", 2, -40, -40);
  run_test_size (_this, "size7.oas", "size7_au2.oas", 2, -50, -50);
  run_test_size (_this, "size7.oas", "size7_au3.oas", 2, -60, -60);
  run_test_size (_this, "size7.oas", "size7_au4.oas", 2, -80, -80);
  run_test_size (_this, "size7.oas", "size7_au5.oas", 2, -100, -100);
  run_test_size (_this, "size7.oas", "size7_au6.oas", 2, 0, -100);
}

TEST(8size)
{
  run_test_size (_this, "size8.oas", "size8_au1.oas", 2, 0, 100);
  run_test_size (_this, "size8.oas", "size8_au2.oas", 2, 100, 50);
  run_test_size (_this, "size8.oas", "size8_au3.oas", 2, -100, -100);
  run_test_size (_this, "size8.oas", "size8_au4.oas", 2, 100, 100);
}

TEST(9twobool)
{
  run_test_twobool (_this, "twobool9.oas", "twobool9_au1.oas");
}

void write (const std::vector<db::Polygon> &q1, const std::vector<db::Polygon> &q2,
            const std::vector<db::Edge> &e1, const std::vector<db::Edge> &e2,
            const std::string &fn)
{
  db::Layout out;
  db::Cell *cell = &out.cell (out.add_cell ("TOP"));
  unsigned int l1 = out.insert_layer ();
  db::LayerProperties lp1;
  lp1.layer = 1;
  lp1.datatype = 0;
  out.set_properties (l1, lp1);
  unsigned int l2 = out.insert_layer ();
  db::LayerProperties lp2;
  lp2.layer = 2;
  lp2.datatype = 0;
  out.set_properties (l2, lp2);
  unsigned int la = out.insert_layer ();
  db::LayerProperties lpa;
  lpa.layer = 100;
  lpa.datatype = 0;
  out.set_properties (la, lpa);
  unsigned int lb = out.insert_layer ();
  db::LayerProperties lpb;
  lpb.layer = 101;
  lpb.datatype = 0;
  out.set_properties (lb, lpb);

  for (std::vector <db::Polygon>::const_iterator p = q1.begin (); p != q1.end (); ++p) {
    cell->shapes (l1).insert (*p);
  }
  for (std::vector <db::Polygon>::const_iterator p = q2.begin (); p != q2.end (); ++p) {
    cell->shapes (l2).insert (*p);
  }

  db::SimpleMerge sm_op;
  db::EdgeProcessor ep;

  db::PolygonContainer p1;
  db::PolygonGenerator pp1 (p1, false);
  ep.clear ();
  ep.insert_sequence (e1.begin (), e1.end (), 0);
  ep.process (pp1, sm_op);
  for (std::vector <db::Polygon>::const_iterator p = p1.polygons ().begin (); p != p1.polygons ().end (); ++p) {
    cell->shapes (la).insert (*p);
  }

  db::PolygonContainer p2;
  db::PolygonGenerator pp2 (p2, false);
  ep.clear ();
  ep.insert_sequence (e2.begin (), e2.end (), 0);
  ep.process (pp2, sm_op);
  for (std::vector <db::Polygon>::const_iterator p = p2.polygons ().begin (); p != p2.polygons ().end (); ++p) {
    cell->shapes (lb).insert (*p);
  }

  db::SaveLayoutOptions options;
  options.set_format ("GDS2");
  db::Writer writer (options);
  tl::OutputStream stream (fn); 
  writer.write (out, stream);
  printf ("%s written.\n", fn.c_str ());
}

TEST(10)
{
  std::vector<db::Edge> edges;

  db::Point plast (rand () / 2 - RAND_MAX / 4, rand () / 2 - RAND_MAX / 4);
  for (unsigned int i = 0; i < 100; ++i) {
    db::Point pnext (rand () / 2 - RAND_MAX / 4, rand () / 2 - RAND_MAX / 4);
    edges.push_back (db::Edge (plast, pnext));
    plast = pnext;
  }
  edges.push_back (db::Edge (plast, edges.front ().p1 ()));

  std::vector<db::Edge> edges2;
  db::Trans t (db::Vector (100, -200));
  for (std::vector<db::Edge>::const_iterator e = edges.begin (); e != edges.end (); ++e) {
    edges2.push_back (e->transformed (t));
  }

  db::EdgeContainer anotb;
  db::EdgeContainer bnota;
  db::EdgeContainer xor_tmp;

  db::PolygonContainer xor_res;
  db::PolygonContainer anotb_or_bnota;

  db::EdgeProcessor ep;

  ep.clear ();
  db::BooleanOp anotb_op (db::BooleanOp::ANotB);
  ep.insert_sequence (edges.begin (), edges.end (), 0);
  ep.insert_sequence (edges2.begin (), edges2.end (), 1);
  ep.process (anotb, anotb_op);
  
  ep.clear ();
  db::BooleanOp bnota_op (db::BooleanOp::BNotA);
  ep.insert_sequence (edges.begin (), edges.end (), 0);
  ep.insert_sequence (edges2.begin (), edges2.end (), 1);
  ep.process (bnota, bnota_op);

  ep.clear ();
  ep.insert_sequence (anotb.edges ().begin (), anotb.edges ().end ());
  ep.insert_sequence (bnota.edges ().begin (), bnota.edges ().end ());
  db::SimpleMerge sm_op;
  db::PolygonGenerator pg1 (anotb_or_bnota, false);
  ep.process (pg1, sm_op);
  
  ep.clear ();
  db::BooleanOp xor_op (db::BooleanOp::Xor);
  ep.insert_sequence (edges.begin (), edges.end (), 0);
  ep.insert_sequence (edges2.begin (), edges2.end (), 1);
  db::PolygonGenerator pg2 (xor_res, false);
  ep.process (pg2, xor_op);

  std::sort (anotb_or_bnota.polygons ().begin (), anotb_or_bnota.polygons ().end ());
  std::sort (xor_res.polygons ().begin (), xor_res.polygons ().end ());

  std::vector <db::Polygon> diff1, diff2;

  std::set_difference (anotb_or_bnota.polygons ().begin (), anotb_or_bnota.polygons ().end (),
                       xor_res.polygons ().begin (), xor_res.polygons ().end (), 
                       std::back_inserter (diff1));

  std::set_difference (xor_res.polygons ().begin (), xor_res.polygons ().end (), 
                       anotb_or_bnota.polygons ().begin (), anotb_or_bnota.polygons ().end (),
                       std::back_inserter (diff2));

#if 1
  EXPECT_EQ (diff1.empty (), true);
  EXPECT_EQ (diff2.empty (), true);
#else
  //  for debugging
  printf ("pg1.size () == %d\n", anotb_or_bnota.polygons ().size ());
  printf ("pg2.size () == %d\n", xor_res.polygons ().size ());

  printf ("diff1:\n");
  for (std::vector<db::Polygon>::const_iterator d = diff1.begin (); d != diff1.end (); ++d) {
    printf ("%s\n", d->to_string().c_str ());
  }

  printf ("diff2:\n");
  for (std::vector<db::Polygon>::const_iterator d = diff2.begin (); d != diff2.end (); ++d) {
    printf ("%s\n", d->to_string().c_str ());
  }

  write (anotb_or_bnota.polygons (), xor_res.polygons (), edges, edges2, "x10.gds");
#endif
}

TEST(11)
{
  std::vector<db::Edge> edges;

  db::Point plast ((rand () % 20) * 100 - 1000, (rand () % 20) * 100 - 1000);
  for (unsigned int i = 0; i < 100; ++i) {
    db::Point pnext ((rand () % 20) * 100 - 1000, (rand () % 20) * 100 - 1000);
    edges.push_back (db::Edge (plast, pnext));
    plast = pnext;
  }
  edges.push_back (db::Edge (plast, edges.front ().p1 ()));

  std::vector<db::Edge> edges2;
  db::Trans t (db::Vector (100, -200));
  for (std::vector<db::Edge>::const_iterator e = edges.begin (); e != edges.end (); ++e) {
    edges2.push_back (e->transformed (t));
  }

  db::EdgeContainer anotb;
  db::EdgeContainer bnota;
  db::EdgeContainer xor_tmp;

  db::PolygonContainer xor_res;
  db::PolygonContainer anotb_or_bnota;

  db::EdgeProcessor ep;

  ep.clear ();
  db::BooleanOp anotb_op (db::BooleanOp::ANotB);
  ep.insert_sequence (edges.begin (), edges.end (), 0);
  ep.insert_sequence (edges2.begin (), edges2.end (), 1);
  ep.process (anotb, anotb_op);
  
  ep.clear ();
  db::BooleanOp bnota_op (db::BooleanOp::BNotA);
  ep.insert_sequence (edges.begin (), edges.end (), 0);
  ep.insert_sequence (edges2.begin (), edges2.end (), 1);
  ep.process (bnota, bnota_op);

  ep.clear ();
  ep.insert_sequence (anotb.edges ().begin (), anotb.edges ().end ());
  ep.insert_sequence (bnota.edges ().begin (), bnota.edges ().end ());
  db::SimpleMerge sm_op;
  db::PolygonGenerator pg1 (anotb_or_bnota, false);
  ep.process (pg1, sm_op);
  
  ep.clear ();
  db::BooleanOp xor_op (db::BooleanOp::Xor);
  ep.insert_sequence (edges.begin (), edges.end (), 0);
  ep.insert_sequence (edges2.begin (), edges2.end (), 1);
  db::PolygonGenerator pg2 (xor_res, false);
  ep.process (pg2, xor_op);

  std::sort (anotb_or_bnota.polygons ().begin (), anotb_or_bnota.polygons ().end ());
  std::sort (xor_res.polygons ().begin (), xor_res.polygons ().end ());

  std::vector <db::Polygon> diff1, diff2;

  std::set_difference (anotb_or_bnota.polygons ().begin (), anotb_or_bnota.polygons ().end (),
                       xor_res.polygons ().begin (), xor_res.polygons ().end (), 
                       std::back_inserter (diff1));

  std::set_difference (xor_res.polygons ().begin (), xor_res.polygons ().end (), 
                       anotb_or_bnota.polygons ().begin (), anotb_or_bnota.polygons ().end (),
                       std::back_inserter (diff2));

#if 1
  EXPECT_EQ (diff1.empty (), true);
  EXPECT_EQ (diff2.empty (), true);
#else
  //  for debugging
  printf ("pg1.size () == %d\n", anotb_or_bnota.polygons ().size ());
  printf ("pg2.size () == %d\n", xor_res.polygons ().size ());

  printf ("diff1:\n");
  for (std::vector<db::Polygon>::const_iterator d = diff1.begin (); d != diff1.end (); ++d) {
    printf ("%s\n", d->to_string().c_str ());
  }

  printf ("diff2:\n");
  for (std::vector<db::Polygon>::const_iterator d = diff2.begin (); d != diff2.end (); ++d) {
    printf ("%s\n", d->to_string().c_str ());
  }

  write (anotb_or_bnota.polygons (), xor_res.polygons (), edges, edges2, "x11.gds");
#endif
}

TEST(12)
{
  std::vector<db::Edge> edges;

  //  manhattan test case
  db::Point plast ((rand () % 20) * 100 - 1000, (rand () % 20) * 100 - 1000);
  for (unsigned int i = 0; i < 100; ++i) {
    {
      db::Point pnext ((rand () % 20) * 100 - 1000, plast.y ());
      edges.push_back (db::Edge (plast, pnext));
      plast = pnext;
    }
    {
      db::Point pnext (plast.x (), (rand () % 20) * 100 - 1000);
      edges.push_back (db::Edge (plast, pnext));
      plast = pnext;
    }
  }
  edges.push_back (db::Edge (plast, db::Point (edges.front ().p1 ().x (), plast.y ())));
  edges.push_back (db::Edge (db::Point (edges.front ().p1 ().x (), plast.y ()), edges.front ().p1 ()));

  std::vector<db::Edge> edges2;
  db::Trans t (db::Vector (100, -200));
  for (std::vector<db::Edge>::const_iterator e = edges.begin (); e != edges.end (); ++e) {
    edges2.push_back (e->transformed (t));
  }

  db::EdgeContainer anotb;
  db::EdgeContainer bnota;
  db::EdgeContainer xor_tmp;

  db::PolygonContainer xor_res;
  db::PolygonContainer anotb_or_bnota;

  db::EdgeProcessor ep;

  ep.clear ();
  db::BooleanOp anotb_op (db::BooleanOp::ANotB);
  ep.insert_sequence (edges.begin (), edges.end (), 0);
  ep.insert_sequence (edges2.begin (), edges2.end (), 1);
  ep.process (anotb, anotb_op);
  
  ep.clear ();
  db::BooleanOp bnota_op (db::BooleanOp::BNotA);
  ep.insert_sequence (edges.begin (), edges.end (), 0);
  ep.insert_sequence (edges2.begin (), edges2.end (), 1);
  ep.process (bnota, bnota_op);

  ep.clear ();
  ep.insert_sequence (anotb.edges ().begin (), anotb.edges ().end ());
  ep.insert_sequence (bnota.edges ().begin (), bnota.edges ().end ());
  db::SimpleMerge sm_op;
  db::PolygonGenerator pg1 (anotb_or_bnota, false);
  ep.process (pg1, sm_op);
  
  ep.clear ();
  db::BooleanOp xor_op (db::BooleanOp::Xor);
  ep.insert_sequence (edges.begin (), edges.end (), 0);
  ep.insert_sequence (edges2.begin (), edges2.end (), 1);
  db::PolygonGenerator pg2 (xor_res, false);
  ep.process (pg2, xor_op);

  std::sort (anotb_or_bnota.polygons ().begin (), anotb_or_bnota.polygons ().end ());
  std::sort (xor_res.polygons ().begin (), xor_res.polygons ().end ());

  std::vector <db::Polygon> diff1, diff2;

  std::set_difference (anotb_or_bnota.polygons ().begin (), anotb_or_bnota.polygons ().end (),
                       xor_res.polygons ().begin (), xor_res.polygons ().end (), 
                       std::back_inserter (diff1));

  std::set_difference (xor_res.polygons ().begin (), xor_res.polygons ().end (), 
                       anotb_or_bnota.polygons ().begin (), anotb_or_bnota.polygons ().end (),
                       std::back_inserter (diff2));

#if 1
  EXPECT_EQ (diff1.empty (), true);
  EXPECT_EQ (diff2.empty (), true);
#else
  //  for debugging
  printf ("pg1.size () == %d\n", anotb_or_bnota.polygons ().size ());
  printf ("pg2.size () == %d\n", xor_res.polygons ().size ());

  printf ("diff1:\n");
  for (std::vector<db::Polygon>::const_iterator d = diff1.begin (); d != diff1.end (); ++d) {
    printf ("%s\n", d->to_string().c_str ());
  }

  printf ("diff2:\n");
  for (std::vector<db::Polygon>::const_iterator d = diff2.begin (); d != diff2.end (); ++d) {
    printf ("%s\n", d->to_string().c_str ());
  }

  write (anotb_or_bnota.polygons (), xor_res.polygons (), edges, edges2, "x12.gds");
#endif
}

TEST(13)
{
  std::vector<db::Polygon> in;
  std::vector<db::Polygon> out;

  db::Box box (0, 0, 100, 100);
  in.push_back (db::Polygon (box));

  db::EdgeProcessor ep;
  ep.size (in, -75, out);
  EXPECT_EQ (out.size (), size_t (0));

  out.clear();
  ep.size (in, -25, out);
  EXPECT_EQ (out.size (), size_t (1));
  EXPECT_EQ (out[0].to_string (), "(25,25;25,75;75,75;75,25)");
}

TEST(14)
{
  //  This has been a problem which was solved with R228 - it was taking too many iterations
  std::vector<db::Polygon> in1;
  std::vector<db::Polygon> in2;

  db::Point p1[] = {
    db::Point (71012503, 113882497),
    db::Point (70124103, 114770787),
    db::Point (70124097, 114875997),
    db::Point (70198487, 114950397),
    db::Point (70303697, 114950403),
    db::Point (71192097, 114062113),
    db::Point (71192103, 113956903),
    db::Point (71117713, 113882503)
  };

  db::Point p2[] = {
    db::Point (71060295, 113834700),
    db::Point (70938100, 113956895),
    db::Point (70938100, 114062105),
    db::Point (71012495, 114136500),
    db::Point (71117705, 114136500),
    db::Point (71239900, 114014305),
    db::Point (71239900, 113909095),
    db::Point (71165505, 113834700)
  };

  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p1 + 0, p1 + sizeof (p1) / sizeof (p1[0]));
  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p2 + 0, p2 + sizeof (p2) / sizeof (p2[0]));

  db::Point p3[] = {
    db::Point (71060295, 113834700),
    db::Point (70972121, 113922874),
    db::Point (70198499, 114696400),
    db::Point (70198495, 114696400),
    db::Point (70164485, 114730410),
    db::Point (70124103, 114770787),
    db::Point (70124103, 114770792),
    db::Point (70124100, 114770795),
    db::Point (70124100, 114823392),
    db::Point (70124097, 114875997),
    db::Point (70124100, 114876000),
    db::Point (70124100, 115107255),
    db::Point (71239900, 115107255),
    db::Point (71239900, 113909095),
    db::Point (71165505, 113834700)
  };

  in2.push_back (db::Polygon ());
  in2.back ().assign_hull (p3, p3 + sizeof (p3) / sizeof (p3[0]));

  std::vector<db::Polygon> out;

  db::EdgeProcessor ep;
  ep.boolean (in1, in2, out, db::BooleanOp::Xor, false, false);

  EXPECT_EQ (out.size (), size_t (3));
  std::sort (out.begin (), out.end ());
  EXPECT_EQ (out[0].to_string (), "(71239900,114014305;71192097,114062108;71192097,114062113;71151715,114102490;71117705,114136500;71117701,114136500;70303697,114950403;70198487,114950397;70124100,114876000;70124100,115107255;71239900,115107255)");
  EXPECT_EQ (out[1].to_string (), "(70198495,114696400;70164485,114730410;70198499,114696400)");
  // before R787 was:
  // EXPECT_EQ (out[1].to_string (), "(70972040,113922955;70198499,114696400;70198495,114696400;70164485,114730410;70124103,114770787)");
  EXPECT_EQ (out[2].to_string (), "(70124103,114770792;70124100,114770795;70124100,114823392)");
}

TEST(15)
{
  // large coordinate handling 

  //  This has been a problem which was solved with R228 - it was taking too many iterations
  std::vector<db::Polygon> in1;
  std::vector<db::Polygon> in2;

  db::Point p1[] = {
    db::Point (1514900, 9767),
    db::Point (9080, 17031),
    db::Point (1467712, 245710),
  };

  db::Point p2[] = {
    db::Point (0, 22388),
    db::Point (1467712, 245710),
    db::Point (1510912, 29731),
    db::Point (1511726, 25637),
    db::Point (1512360, 22467),
  };

  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p1 + 0, p1 + sizeof (p1) / sizeof (p1[0]));
  in2.push_back (db::Polygon ());
  in2.back ().assign_hull (p2 + 0, p2 + sizeof (p2) / sizeof (p2[0]));

  std::vector<db::Polygon> out;

  db::EdgeProcessor ep;
  ep.boolean (in1, in2, out, db::BooleanOp::And, false, false);

  EXPECT_EQ (out.size (), size_t (1));
  std::sort (out.begin (), out.end ());
  EXPECT_EQ (out[0].to_string (), "(43264,22390;1467712,245710;1511719,25671;1511726,25637;1512360,22467)");
}

TEST(16)
{
  // large coordinate handling 

  //  This has been a problem which was solved with R228 - it was taking too many iterations
  std::vector<db::Polygon> in1;
  std::vector<db::Polygon> in2;

  db::Point p1[] = {
    db::Point (23, 2345),
    db::Point (4, 9832),
    db::Point (10592, 2485)
  };

  db::Point p2[] = {
    db::Point (13, 0),
    db::Point (13, 7486),
    db::Point (10873, 6)
  };

  db::Point p3[] = {
    db::Point (0, 2818),
    db::Point (27, 10304),
    db::Point (8643, 2823)
  };

  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p1 + 0, p1 + sizeof (p1) / sizeof (p1[0]));
  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p2 + 0, p2 + sizeof (p2) / sizeof (p2[0]));
  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p3 + 0, p3 + sizeof (p3) / sizeof (p3[0]));

  std::vector<db::Polygon> out;

  db::EdgeProcessor ep;
  ep.simple_merge (in1, out, false, false);

  EXPECT_EQ (out.size (), size_t (1));
  std::sort (out.begin (), out.end ());
  EXPECT_EQ (out[0].to_string (), "(13,0;13,2818;0,2818;13,6286;13,6422;4,9832;25,9817;27,10304;2825,7874;10592,2485;7336,2442;10873,6)");
}

TEST(17)
{
  // large coordinate handling 

  //  This has been a problem which was solved with R228 - it was taking too many iterations
  std::vector<db::Polygon> in1;
  std::vector<db::Polygon> in2;

  db::Point p1[] = {
    db::Point (113, 64),
    db::Point (1293, 469),
    db::Point (1293, 64)
  };

  db::Point p2[] = {
    db::Point (204, 100),
    db::Point (1388, 495),
    db::Point (1387, 101)
  };

  db::Point p3[] = {
    db::Point (0, 18),
    db::Point (1177, 434),
    db::Point (1178, 18)
  };

  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p1 + 0, p1 + sizeof (p1) / sizeof (p1[0]));
  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p2 + 0, p2 + sizeof (p2) / sizeof (p2[0]));
  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p3 + 0, p3 + sizeof (p3) / sizeof (p3[0]));

  std::vector<db::Polygon> out;

  db::EdgeProcessor ep;
  ep.simple_merge (in1, out, false, false);

  EXPECT_EQ (out.size (), size_t (1));
  std::sort (out.begin (), out.end ());
  EXPECT_EQ (out[0].to_string (), "(0,18;130,64;113,64;218,100;204,100;700,266;706,268;1177,434;1177,429;1293,469;1293,463;1388,495;1387,101;1293,101;1293,64;1178,64;1178,18)");
}

TEST(18)
{
  // large coordinate handling 

  //  This has been a problem which was solved with R228 - it was taking too many iterations
  std::vector<db::Polygon> in1;
  std::vector<db::Polygon> in2;

  db::Point p1[] = {
    db::Point (419, 1400.00),
    db::Point (2281, 1589.00),
    db::Point (2281, 1400.00)
  };

  db::Point p2[] = {
    db::Point (419, 1400.00),
    db::Point (2284, 1589.00),
    db::Point (2284, 1400.00)
  };

  db::Point p3[] = {
    db::Point (453, 1405.00),
    db::Point (2316, 1588.00),
    db::Point (2316, 1405.00)
  };

  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p1 + 0, p1 + sizeof (p1) / sizeof (p1[0]));
  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p2 + 0, p2 + sizeof (p2) / sizeof (p2[0]));
  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p3 + 0, p3 + sizeof (p3) / sizeof (p3[0]));

  std::vector<db::Polygon> out;

  db::EdgeProcessor ep;
  ep.simple_merge (in1, out, false, false);

  EXPECT_EQ (out.size (), size_t (1));
  std::sort (out.begin (), out.end ());
  EXPECT_EQ (out[0].to_string (), "(419,1400;468,1405;453,1405;926,1451;953,1454;2281,1589;2284,1589;2284,1585;2316,1588;2316,1405;2284,1405;2284,1400)");
}

TEST(19)
{
  // large coordinate handling 

  //  This has been a problem which was solved with R228 - it was taking too many iterations
  std::vector<db::Polygon> in1;
  std::vector<db::Polygon> in2;

  db::Point p1[] = {
    db::Point (26029700, 19931900),
    db::Point (11944600, 24988200),
    db::Point (16663400, 48582400),
    db::Point (31607400, 45593600)
  };

  db::Point p2[] = {
    db::Point (12654800, 0),
    db::Point (0, 8492100),
    db::Point (13371200, 28417800),
    db::Point (27109200, 23471400)
  };

  db::Point p3[] = {
    db::Point (145086300, 20050900),
    db::Point (12705400, 28790300),
    db::Point (12851400, 29524600),
    db::Point (145086300, 35290900)
  };

  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p1 + 0, p1 + sizeof (p1) / sizeof (p1[0]));
  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p2 + 0, p2 + sizeof (p2) / sizeof (p2[0]));
  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p3 + 0, p3 + sizeof (p3) / sizeof (p3[0]));

  std::vector<db::Polygon> out;

  db::EdgeProcessor ep;
  ep.simple_merge (in1, out, false, false);

  EXPECT_EQ (out.size (), size_t (1));
  std::sort (out.begin (), out.end ());
  EXPECT_EQ (out[0].to_string (), "(12654800,0;0,8492100;12316037,26845401;12771101,29120740;12851400,29524600;12851876,29524621;16663400,48582400;31607400,45593600;28260773,30196549;145086300,35290900;145086300,20050900;27739393,27797800;26821540,23574973;27109200,23471400;26629840,22693003;26029700,19931900;25128663,20255356)");
}

TEST(20)
{
  // TOPIC: catching of an edge by a close one.

  //  This has been a problem which was solved with R228 - it was taking too many iterations
  std::vector<db::Polygon> in1;
  std::vector<db::Polygon> in2;

  db::Point p1[] = {
    db::Point (7394, 2768),
    db::Point (7533, 2826),
    db::Point (7404, 2768)
  };

  db::Point p2[] = {
    db::Point (7427, 2768),
    db::Point (7533, 2826),
    db::Point (7434, 2768)
  };

  db::Point p3[] = {
    db::Point (7362, 2768),
    db::Point (7532, 2826),
    db::Point (7374, 2768)
  };

  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p1 + 0, p1 + sizeof (p1) / sizeof (p1[0]));
  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p2 + 0, p2 + sizeof (p2) / sizeof (p2[0]));
  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p3 + 0, p3 + sizeof (p3) / sizeof (p3[0]));

  std::vector<db::Polygon> out;

  db::EdgeProcessor ep;
  ep.simple_merge (in1, out, false, false);

  EXPECT_EQ (out.size (), size_t (2));
  std::sort (out.begin (), out.end ());
  EXPECT_EQ (out[0].to_string (), "(7362,2768;7532,2826;7374,2768)");
  EXPECT_EQ (out[1].to_string (), "(7394,2768;7533,2826;7434,2768;7427,2768;7533,2826;7404,2768)");

}

TEST(21)
{
  //  Recurring edge and similar other edge problem
  std::vector<db::Polygon> in1;
  std::vector<db::Polygon> in2;

  db::Point p1[] = {
    db::Point (2696, 0), 
    db::Point (5297, 13339), 
    db::Point (6592, 2603), 
    db::Point (4217, 5014)
  };

  db::Point p2[] = {
    db::Point (2696, 0), 
    db::Point (4217, 5015), 
    db::Point (890, 1381), 
    db::Point (0, 11887), 
    db::Point (4217, 5015)
  };

  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p1 + 0, p1 + sizeof (p1) / sizeof (p1[0]));
  in2.push_back (db::Polygon ());
  in2.back ().assign_hull (p2 + 0, p2 + sizeof (p2) / sizeof (p2[0]));

  std::vector<db::Polygon> out;

  db::EdgeProcessor ep;
  ep.boolean (in1, in2, out, db::BooleanOp::And, false, false);

  EXPECT_EQ (out.size (), size_t (1));
  std::sort (out.begin (), out.end ());
  EXPECT_EQ (out[0].to_string (), "(3527,4261;3805,5687;4217,5015;4217,5014)");
}

TEST(22)
{
  //  Recurring edge and similar other edge problem
  std::vector<db::Polygon> in1;
  std::vector<db::Polygon> in2;

  db::Point p1[] = {
    db::Point (9985, 0),
    db::Point (0, 2236),
    db::Point (13710, 3746),
    db::Point (12442, 2457),
  };

  db::Point p2[] = {
    db::Point (9985, 0),
    db::Point (0, 2236),
    db::Point (13710, 3747),
    db::Point (12443, 2458),
  };

  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p1 + 0, p1 + sizeof (p1) / sizeof (p1[0]));
  in2.push_back (db::Polygon ());
  in2.back ().assign_hull (p2 + 0, p2 + sizeof (p2) / sizeof (p2[0]));

  std::vector<db::Polygon> out;

  db::EdgeProcessor ep;
  ep.boolean (in1, in2, out, db::BooleanOp::And, false, false);

  EXPECT_EQ (out.size (), size_t (1));
  std::sort (out.begin (), out.end ());
  EXPECT_EQ (out[0].to_string (), "(9985,0;0,2236;13709,3746;12464,2479)");
}

TEST(23)
{
  std::vector<db::Polygon> in1;

  db::Point p1[] = {
    db::Point (0, 0),
    db::Point (1, 1),
    db::Point (0, 1),
    db::Point (1, 0),
  };

  in1.push_back (db::Polygon ());
  in1.back ().assign_hull (p1 + 0, p1 + sizeof (p1) / sizeof (p1[0]));

  std::vector<db::Polygon> out;

  db::EdgeProcessor ep;
  ep.simple_merge (in1, out, false, false);

  EXPECT_EQ (out.size (), size_t (1));
  std::sort (out.begin (), out.end ());
  EXPECT_EQ (out[0].to_string (), "(0,0;0,1;1,1)");
}

TEST(24)
{
  std::vector<db::Polygon> in1;

  {
    db::Point p1[] = {
      db::Point (0, -9),
      db::Point (1, 10),
      db::Point (0, 10),
      db::Point (1, -9)
    };
    in1.push_back (db::Polygon ());
    in1.back ().assign_hull (p1 + 0, p1 + sizeof (p1) / sizeof (p1[0]));
  }

  {
    db::Point p1[] = {
      db::Point (1, 1),
      db::Point (-2, 2),
      db::Point (-2, 3)
    };
    in1.push_back (db::Polygon ());
    in1.back ().assign_hull (p1 + 0, p1 + sizeof (p1) / sizeof (p1[0]));
  }

  {
    db::Point p1[] = {
      db::Point (3, -1),
      db::Point (0, 1),
      db::Point (3, 0)
    };
    in1.push_back (db::Polygon ());
    in1.back ().assign_hull (p1 + 0, p1 + sizeof (p1) / sizeof (p1[0]));
  }

  {
    db::Point p1[] = {
      db::Point (1, 0),
      db::Point (-2, 1),
      db::Point (-2, 2)
    };
    in1.push_back (db::Polygon ());
    in1.back ().assign_hull (p1 + 0, p1 + sizeof (p1) / sizeof (p1[0]));
  }

  {
    db::Point p1[] = {
      db::Point (3, -2),
      db::Point (0, 0),
      db::Point (3, -1)
    };
    in1.push_back (db::Polygon ());
    in1.back ().assign_hull (p1 + 0, p1 + sizeof (p1) / sizeof (p1[0]));
  }

  {
    std::vector<db::Polygon> out;

    db::EdgeProcessor ep;
    ep.simple_merge (in1, out, false, false);

    EXPECT_EQ (out.size (), size_t (1));
    std::sort (out.begin (), out.end ());
    EXPECT_EQ (out[0].to_string (), "(0,-9;0,0;-2,1;-2,3;0,1;0,10;1,10;1,1;0,0;3,0;3,-2;1,0;1,-9)");
  }

  {
    std::vector<db::Polygon> out;

    db::EdgeProcessor ep;
    ep.simple_merge (in1, out, false, true);

    EXPECT_EQ (out.size (), size_t (3));
    std::sort (out.begin (), out.end ());
    EXPECT_EQ (out[0].to_string (), "(0,-9;0,0;1,0;1,-9)");
    EXPECT_EQ (out[1].to_string (), "(3,-2;1,0;3,0)");
    EXPECT_EQ (out[2].to_string (), "(0,0;-2,1;-2,3;0,1;0,10;1,10;1,1)");
  }
}

TEST(25)
{
  std::vector<db::Polygon> in1;

  {
    db::Point p1[] = {
      db::Point (-471, 2264),
      db::Point (-471, 2367),
      db::Point (-345, 2367),
      db::Point (-333, 2391),
      db::Point (-327, 2402),
      db::Point (-329, 2400),
      db::Point (-327, 2399),
      db::Point (-329, 2400),
      db::Point (-323, 2407),
      db::Point (-332, 2407),
      db::Point (-332, 2391),
      db::Point (-318, 2393),
      db::Point (-328, 2397),
      db::Point (-323, 2390),
      db::Point (-332, 2394),
      db::Point (-330, 2387),
      db::Point (-326, 2387),
      db::Point (-333, 2394),
      db::Point (-333, 2388),
      db::Point (-328, 2402),
      db::Point (-339, 2402),
      db::Point (-353, 2367),
      db::Point (-353, 2264)
    };
    in1.push_back (db::Polygon ());
    in1.back ().assign_hull (p1 + 0, p1 + sizeof (p1) / sizeof (p1[0]));
  }

  std::vector<db::Polygon> out;

  db::EdgeProcessor ep;
  ep.simple_merge (in1, out, false, false, 1);

  EXPECT_EQ (out.size (), size_t (2));
  std::sort (out.begin (), out.end ());
  EXPECT_EQ (out[0].to_string (), "(-471,2264;-471,2367;-353,2367;-353,2264)");
  EXPECT_EQ (out[1].to_string (), "(-323,2390;-327,2392;-324,2392)");
}

TEST(26a)
{
  db::EdgeProcessor ep;
  ep.insert (db::Polygon (db::Box (db::Point (0, 0), db::Point (100, 100))), 0);
  ep.insert (db::Polygon (db::Box (db::Point (40, 0), db::Point (140, 100))), 1);
  ep.insert (db::Polygon (db::Box (db::Point (60, 20), db::Point (160, 120))), 2);
  ep.insert (db::Polygon (db::Box (db::Point (110, 50), db::Point (210, 150))), 3);
  ep.insert (db::Polygon (db::Box (db::Point (-100, -100), db::Point (1000, 1000))), 4);

  db::InteractionDetector id;
  db::EdgeSink es;
  ep.process (es, id);
  id.finish ();

  std::string s;
  for (db::InteractionDetector::iterator i = id.begin (); i != id.end (); ++i) {
    if (! s.empty ()) {
      s += ",";
    }
    s += tl::to_string (i->first) + ":" + tl::to_string (i->second);
  }

  EXPECT_EQ (s, "0:1,0:2,0:4,1:2,1:3,1:4,2:3,2:4,3:4");
}

TEST(26b)
{
  db::EdgeProcessor ep;
  ep.insert (db::Polygon (db::Box (db::Point (0, 0), db::Point (100, 100))), 0);
  ep.insert (db::Polygon (db::Box (db::Point (40, 0), db::Point (140, 100))), 1);
  ep.insert (db::Polygon (db::Box (db::Point (60, 20), db::Point (160, 120))), 2);
  ep.insert (db::Polygon (db::Box (db::Point (110, 50), db::Point (210, 150))), 3);
  ep.insert (db::Polygon (db::Box (db::Point (-100, -100), db::Point (1000, 1000))), 4);
  ep.insert (db::Polygon (db::Box (db::Point (1000, 1100), db::Point (1100, 1200))), 5);

  db::InteractionDetector id (1, 4); // outside with background #4
  db::EdgeSink es;
  ep.process (es, id);
  id.finish ();

  std::string s;
  for (db::InteractionDetector::iterator i = id.begin (); i != id.end (); ++i) {
    if (! s.empty ()) {
      s += ",";
    }
    s += tl::to_string (i->first) + ":" + tl::to_string (i->second);
  }

  EXPECT_EQ (s, "4:5");
}

TEST(26c)
{
  db::EdgeProcessor ep;
  ep.insert (db::Polygon (db::Box (db::Point (-100, -100), db::Point (1000, 1000))), 0);
  ep.insert (db::Polygon (db::Box (db::Point (1000, 1100), db::Point (1100, 1200))), 1);
  ep.insert (db::Polygon (db::Box (db::Point (0, 0), db::Point (100, 100))), 2);
  ep.insert (db::Polygon (db::Box (db::Point (40, 0), db::Point (140, 100))), 3);
  ep.insert (db::Polygon (db::Box (db::Point (60, 20), db::Point (160, 120))), 4);
  ep.insert (db::Polygon (db::Box (db::Point (110, 50), db::Point (210, 150))), 5);
  ep.insert (db::Polygon (db::Box (db::Point (1000, 1100), db::Point (1010, 1110))), 6);

  db::InteractionDetector id (-1, 0); // inside with background #0
  db::EdgeSink es;
  ep.process (es, id);
  id.finish ();

  std::string s;
  for (db::InteractionDetector::iterator i = id.begin (); i != id.end (); ++i) {
    if (! s.empty ()) {
      s += ",";
    }
    s += tl::to_string (i->first) + ":" + tl::to_string (i->second);
  }

  //  does not work yet!
  EXPECT_EQ (s, "0:2,0:3,0:4,0:5");
}

TEST(26d)
{
  db::EdgeProcessor ep;
  ep.insert (db::Polygon (db::Box (db::Point (-100, -100), db::Point (1000, 1000))), 0);
  ep.insert (db::Polygon (db::Box (db::Point (1000, 1100), db::Point (1100, 1200))), 1);
  ep.insert (db::Polygon (db::Box (db::Point (0, 0), db::Point (100, 100))), 2);
  ep.insert (db::Polygon (db::Box (db::Point (40, 0), db::Point (140, 100))), 3);
  ep.insert (db::Polygon (db::Box (db::Point (60, 20), db::Point (160, 120))), 4);
  ep.insert (db::Polygon (db::Box (db::Point (110, 50), db::Point (210, 150))), 5);
  ep.insert (db::Polygon (db::Box (db::Point (1000, 1100), db::Point (1010, 1110))), 6);

  db::InteractionDetector id (-1, 1); // inside with background #0 and #1
  db::EdgeSink es;
  ep.process (es, id);
  id.finish ();

  std::string s;
  for (db::InteractionDetector::iterator i = id.begin (); i != id.end (); ++i) {
    if (! s.empty ()) {
      s += ",";
    }
    s += tl::to_string (i->first) + ":" + tl::to_string (i->second);
  }

  //  does not work yet!
  EXPECT_EQ (s, "0:2,0:3,0:4,0:5,1:6");
}

TEST(27)
{
  db::Polygon poly (db::Box (db::Point (0, 0), db::Point (1000, 1000)));
  db::Polygon p2 (poly);
  p2.size (-100, -100, 2);

  //  because we don't use mode 1 merging for p2, we get loops at the corners of p2
  {
    db::EdgeProcessor ep;
    ep.insert (poly, 0);
    ep.insert (p2, 1);

    std::vector<db::Polygon> out;
    db::PolygonContainer pc (out);
    db::PolygonGenerator pg (pc, false, true);
    db::BooleanOp op (db::BooleanOp::Xor);

    ep.process (pg, op);

    EXPECT_EQ (out.size (), size_t (4));
    EXPECT_EQ (out[0].to_string (), "(100,0;100,100;900,100;900,0)");
    EXPECT_EQ (out[1].to_string (), "(0,100;0,900;100,900;100,100)");
    EXPECT_EQ (out[2].to_string (), "(900,100;900,900;1000,900;1000,100)");
    EXPECT_EQ (out[3].to_string (), "(100,900;100,1000;900,1000;900,900)");
  }

  //  BooleanOp2 behaves the same with modes -1
  {
    db::EdgeProcessor ep;
    ep.insert (poly, 0);
    ep.insert (p2, 1);

    std::vector<db::Polygon> out;
    db::PolygonContainer pc (out);
    db::PolygonGenerator pg (pc, false, false);
    db::BooleanOp2 op (db::BooleanOp::Xor, -1, -1);

    ep.process (pg, op);

    EXPECT_EQ (out.size (), size_t (1));
    EXPECT_EQ (out[0].to_string (), "(100,0;100,100;0,100;0,900;100,900;100,1000;900,1000;900,900;1000,900;1000,100;900,100;900,0/100,100;900,100;900,900;100,900)");
  }

  //  with BooleanOp2 we can solve this issue
  {
    db::EdgeProcessor ep;
    ep.insert (poly, 0);
    ep.insert (p2, 1);

    std::vector<db::Polygon> out;
    db::PolygonContainer pc (out);
    db::PolygonGenerator pg (pc, false, true);
    db::BooleanOp2 op (db::BooleanOp::Xor, -1, 1);

    ep.process (pg, op);

    EXPECT_EQ (out.size (), size_t (1));
    EXPECT_EQ (out[0].to_string (), "(0,0;0,1000;1000,1000;1000,0/100,100;900,100;900,900;100,900)");
  }

  {
    db::EdgeProcessor ep;
    ep.insert (poly, 1);
    ep.insert (p2, 0);

    std::vector<db::Polygon> out;
    db::PolygonContainer pc (out);
    db::PolygonGenerator pg (pc, false, false);
    db::BooleanOp2 op (db::BooleanOp::Xor, -1, 1);

    ep.process (pg, op);

    EXPECT_EQ (out.size (), size_t (1));
    EXPECT_EQ (out[0].to_string (), "(100,0;100,100;0,100;0,900;100,900;100,1000;900,1000;900,900;1000,900;1000,100;900,100;900,0/100,100;900,100;900,900;100,900)");
  }

  {
    db::EdgeProcessor ep;
    ep.insert (poly, 1);
    ep.insert (p2, 0);

    std::vector<db::Polygon> out;
    db::PolygonContainer pc (out);
    db::PolygonGenerator pg (pc, false, true);
    db::BooleanOp2 op (db::BooleanOp::Xor, 1, -1);

    ep.process (pg, op);

    EXPECT_EQ (out.size (), size_t (1));
    EXPECT_EQ (out[0].to_string (), "(0,0;0,1000;1000,1000;1000,0/100,100;900,100;900,900;100,900)");
  }
}

// #594
TEST(28)
{
  std::vector<db::Polygon> b;
  db::Point b1[] = {
    db::Point (-518003,-792684),
    db::Point (-489451,-724867),
    db::Point (-487680,-724734),
    db::Point (-485180,-757934),
    db::Point (-501151,-775469)
  };
  b.push_back (db::Polygon ());
  b.back ().assign_hull (b1 + 0, b1 + sizeof (b1) / sizeof (b1[0]));

  std::vector<db::Polygon> a;
  db::Point a1[] = {
    db::Point (-488720,-758200),
    db::Point (-491220,-725000),
    db::Point (-487680,-724734),
    db::Point (-485180,-757934)
  };
  a.push_back (db::Polygon ());
  a.back ().assign_hull (a1 + 0, a1 + sizeof (a1) / sizeof (a1[0]));
  db::Point a2[] = {
    db::Point (-490953,-726224),
    db::Point (-505953,-709625),
    db::Point (-502948,-706909),
    db::Point (-487948,-723509)
  };
  a.push_back (db::Polygon ());
  a.back ().assign_hull (a2 + 0, a2 + sizeof (a2) / sizeof (a2[0]));
  db::Point a3[] = {
    db::Point (-491225,-724867),
    db::Point (-491225,-691667),
    db::Point (-487675,-691667),
    db::Point (-487675,-724867)
  };
  a.push_back (db::Polygon ());
  a.back ().assign_hull (a3 + 0, a3 + sizeof (a3) / sizeof (a3[0]));

  db::EdgeProcessor ep;
  for (std::vector<db::Polygon>::const_iterator i = a.begin (); i != a.end (); ++i) {
    ep.insert (*i, 0);
  }
  for (std::vector<db::Polygon>::const_iterator i = b.begin (); i != b.end (); ++i) {
    ep.insert (*i, 1);
  }

  std::vector<db::Polygon> out;
  db::PolygonContainer pc (out);
  db::PolygonGenerator pg (pc, false, true);
  db::BooleanOp op (db::BooleanOp::And);

  ep.process (pg, op);

  EXPECT_EQ (out.size (), size_t (1));
  EXPECT_EQ (out[0].to_string (), "(-488720,-758200;-490960,-728451;-489451,-724867;-489450,-724867;-487680,-724734;-487675,-724800;-485180,-757934)");
}

// #644
TEST(29)
{
  std::vector<db::Polygon> b;
  db::Point b1[] = {
    db::Point (0, 0),
    db::Point (0, 608),
    db::Point (172, 602),
    db::Point (572, 588),
    db::Point (573, 588),
    db::Point (710, 583),
    db::Point (710, 0)
  };
  b.push_back (db::Polygon ());
  b.back ().assign_hull (b1 + 0, b1 + sizeof (b1) / sizeof (b1[0]));

  std::vector<db::Polygon> a;
  db::Point a1[] = {
    db::Point (140, 140),
    db::Point (140, 603),
    db::Point (167, 602),
    db::Point (372, 595),
    db::Point (580, 588),
    db::Point (580, 140)
  };
  a.push_back (db::Polygon ());
  a.back ().assign_hull (a1 + 0, a1 + sizeof (a1) / sizeof (a1[0]));

  db::EdgeProcessor ep;
  for (std::vector<db::Polygon>::const_iterator i = a.begin (); i != a.end (); ++i) {
    ep.insert (*i, 0);
  }
  for (std::vector<db::Polygon>::const_iterator i = b.begin (); i != b.end (); ++i) {
    ep.insert (*i, 1);
  }

  std::vector<db::Polygon> out;
  db::PolygonContainer pc (out);
  db::PolygonGenerator pg (pc, false, true);
  db::BooleanOp op (db::BooleanOp::Or);

  ep.process (pg, op);

  EXPECT_EQ (out.size (), size_t (1));
  EXPECT_EQ (out[0].to_string (), "(0,0;0,608;172,602;572,588;580,588;710,583;710,0)");
}

TEST(30)
{
  std::vector<db::Polygon> a;
  db::Point a1[] = {
    db::Point (0, 0),
    db::Point (0, 500),
    db::Point (300, 500),
    db::Point (500, 300),
    db::Point (500, 0)
  };
  a.push_back (db::Polygon ());
  a.back ().assign_hull (a1 + 0, a1 + sizeof (a1) / sizeof (a1[0]));

  db::EdgeProcessor ep;
  for (std::vector<db::Polygon>::const_iterator i = a.begin (); i != a.end (); ++i) {
    ep.insert (*i, 0);
  }
  ep.insert (db::Edge (-100, 400, 600, 420), 1);
  ep.insert (db::Edge (-100, 400, 600, 400), 1);

  std::vector<db::Edge> out;
  db::EdgeContainer ec (out);
  db::EdgePolygonOp op;

  ep.process (ec, op);

  EXPECT_EQ (out.size (), size_t (2));
  EXPECT_EQ (out[0].to_string (), "(0,400;400,400)");
  EXPECT_EQ (out[1].to_string (), "(0,403;386,414)");
}

TEST(31)
{
  std::vector<db::Polygon> a;
  db::Point a1[] = {
    db::Point (0, 0),
    db::Point (0, 500),
    db::Point (300, 500),
    db::Point (500, 300),
    db::Point (500, 0)
  };
  a.push_back (db::Polygon ());
  a.back ().assign_hull (a1 + 0, a1 + sizeof (a1) / sizeof (a1[0]));

  db::EdgeProcessor ep;
  for (std::vector<db::Polygon>::const_iterator i = a.begin (); i != a.end (); ++i) {
    ep.insert (*i, 0);
  }
  ep.insert (db::Edge (600, 400, -100, 420), 1);
  ep.insert (db::Edge (600, 400, -100, 400), 1);
  ep.insert (db::Edge (-100, 0, 600, 0), 1);
  ep.insert (db::Edge (0, -100, 0, 600), 1);
  ep.insert (db::Edge (500, -100, 500, 600), 1);

  std::vector<db::Edge> out;
  db::EdgeContainer ec (out);
  db::EdgePolygonOp op;

  ep.process (ec, op);

  std::string s = tl::join (out.begin (), out.end (), ";");
  EXPECT_EQ (s, "(0,0;0,400);(0,0;500,0);(500,0;500,300);(0,400;0,417);(400,400;0,400);(394,406;0,417);(0,417;0,500)");

  ep.clear ();
  out.clear ();

  for (std::vector<db::Polygon>::const_iterator i = a.begin (); i != a.end (); ++i) {
    ep.insert (*i, 0);
  }
  ep.insert (db::Edge (-100, 500, 600, 500), 1);
  ep.insert (db::Edge (400, -100, 400, 600), 1);
  ep.insert (db::Edge (-100, -100, -100, 600), 1);
  ep.insert (db::Edge (600, -100, 600, 600), 1);
  ep.insert (db::Edge (-100, -100, 600, -100), 1);

  ep.process (ec, op);

  s = tl::join (out.begin (), out.end (), ";");
  EXPECT_EQ (s, "(400,0;400,400);(0,500;300,500)");
}

TEST(32)
{
  std::vector<db::Polygon> a;
  db::Point a1[] = {
    db::Point (0, 0),
    db::Point (0, 500),
    db::Point (300, 500),
    db::Point (500, 300),
    db::Point (500, 0)
  };
  a.push_back (db::Polygon ());
  a.back ().assign_hull (a1 + 0, a1 + sizeof (a1) / sizeof (a1[0]));

  db::EdgeProcessor ep;
  for (std::vector<db::Polygon>::const_iterator i = a.begin (); i != a.end (); ++i) {
    ep.insert (*i, 0);
  }
  ep.insert (db::Edge (600, 400, -100, 420), 1);
  ep.insert (db::Edge (600, 400, -100, 400), 1);
  ep.insert (db::Edge (-100, 0, 600, 0), 1);
  ep.insert (db::Edge (0, -100, 0, 600), 1);
  ep.insert (db::Edge (500, -100, 500, 600), 1);

  std::vector<db::Edge> out;
  db::EdgeContainer ec (out);
  db::EdgePolygonOp op (db::EdgePolygonOp::Inside, false /*not including touch*/);

  ep.process (ec, op);

  std::string s = tl::join (out.begin (), out.end (), ";");
  EXPECT_EQ (s, "(400,400;0,400);(394,406;0,417)");

  ep.clear ();
  out.clear ();

  for (std::vector<db::Polygon>::const_iterator i = a.begin (); i != a.end (); ++i) {
    ep.insert (*i, 0);
  }
  ep.insert (db::Edge (-100, 500, 600, 500), 1);
  ep.insert (db::Edge (400, -100, 400, 600), 1);
  ep.insert (db::Edge (-100, -100, -100, 600), 1);
  ep.insert (db::Edge (600, -100, 600, 600), 1);
  ep.insert (db::Edge (-100, -100, 600, -100), 1);

  ep.process (ec, op);

  s = tl::join (out.begin (), out.end (), ";");
  EXPECT_EQ (s, "(400,0;400,400)");
}

TEST(33)
{
  std::vector<db::Polygon> a;
  db::Point a1[] = {
    db::Point (0, 0),
    db::Point (0, 500),
    db::Point (300, 500),
    db::Point (500, 300),
    db::Point (500, 0)
  };
  a.push_back (db::Polygon ());
  a.back ().assign_hull (a1 + 0, a1 + sizeof (a1) / sizeof (a1[0]));

  db::EdgeProcessor ep;
  for (std::vector<db::Polygon>::const_iterator i = a.begin (); i != a.end (); ++i) {
    ep.insert (*i, 0);
  }
  ep.insert (db::Edge (600, 400, -100, 420), 1);
  ep.insert (db::Edge (600, 400, -100, 400), 1);
  ep.insert (db::Edge (-100, 0, 600, 0), 1);
  ep.insert (db::Edge (0, -100, 0, 600), 1);
  ep.insert (db::Edge (500, -100, 500, 600), 1);

  std::vector<db::Edge> out;
  db::EdgeContainer ec (out);
  db::EdgePolygonOp op (db::EdgePolygonOp::Outside, true /*including touch*/);

  ep.process (ec, op);

  std::string s = tl::join (out.begin (), out.end (), ";");
  EXPECT_EQ (s, "(0,-100;0,0);(500,-100;500,0);(-100,0;0,0);(500,0;600,0);(500,300;500,400);"
                "(0,400;-100,400);(500,400;400,400);(500,400;500,403);(600,400;500,400);"
                "(600,400;500,403);(500,403;394,406);(500,403;500,600);(0,417;-100,420);"
                "(0,500;0,600)");

  ep.clear ();
  out.clear ();

  for (std::vector<db::Polygon>::const_iterator i = a.begin (); i != a.end (); ++i) {
    ep.insert (*i, 0);
  }
  ep.insert (db::Edge (-100, 500, 600, 500), 1);
  ep.insert (db::Edge (400, -100, 400, 600), 1);
  ep.insert (db::Edge (-100, -100, -100, 600), 1);
  ep.insert (db::Edge (600, -100, 600, 600), 1);
  ep.insert (db::Edge (-100, -100, 600, -100), 1);

  ep.process (ec, op);

  s = tl::join (out.begin (), out.end (), ";");
  EXPECT_EQ (s, "(-100,-100;-100,500);(-100,-100;400,-100);(400,-100;400,0);(400,-100;600,-100);"
                "(600,-100;600,500);(400,400;400,500);(-100,500;-100,600);(-100,500;0,500);"
                "(300,500;400,500);(400,500;400,600);(400,500;600,500);(600,500;600,600)");
}

TEST(34)
{
  std::vector<db::Polygon> a;
  db::Point a1[] = {
    db::Point (0, 0),
    db::Point (0, 500),
    db::Point (300, 500),
    db::Point (500, 300),
    db::Point (500, 0)
  };
  a.push_back (db::Polygon ());
  a.back ().assign_hull (a1 + 0, a1 + sizeof (a1) / sizeof (a1[0]));

  db::EdgeProcessor ep;
  for (std::vector<db::Polygon>::const_iterator i = a.begin (); i != a.end (); ++i) {
    ep.insert (*i, 0);
  }
  ep.insert (db::Edge (600, 400, -100, 420), 1);
  ep.insert (db::Edge (600, 400, -100, 400), 1);
  ep.insert (db::Edge (-100, 0, 600, 0), 1);
  ep.insert (db::Edge (0, -100, 0, 600), 1);
  ep.insert (db::Edge (500, -100, 500, 600), 1);

  std::vector<db::Edge> out2;
  db::EdgeContainer ec2 (out2, false, 2);
  std::vector<db::Edge> out;
  db::EdgeContainer ec (out, false, 1, &ec2);
  db::EdgePolygonOp op (db::EdgePolygonOp::Both, true /*not including touch*/);

  ep.process (ec, op);

  std::string s = tl::join (out2.begin (), out2.end (), ";");
  EXPECT_EQ (s, "(0,-100;0,0);(500,-100;500,0);(-100,0;0,0);(500,0;600,0);(500,300;500,400);"
                "(0,400;-100,400);(500,400;400,400);(500,400;500,403);(600,400;500,400);"
                "(600,400;500,403);(500,403;394,406);(500,403;500,600);(0,417;-100,420);"
                "(0,500;0,600)");

  s = tl::join (out.begin (), out.end (), ";");
  EXPECT_EQ (s, "(0,0;0,400);(0,0;500,0);(500,0;500,300);(0,400;0,417);(400,400;0,400);(394,406;0,417);(0,417;0,500)");

  ep.clear ();
  out.clear ();
  out2.clear ();

  for (std::vector<db::Polygon>::const_iterator i = a.begin (); i != a.end (); ++i) {
    ep.insert (*i, 0);
  }
  ep.insert (db::Edge (-100, 500, 600, 500), 1);
  ep.insert (db::Edge (400, -100, 400, 600), 1);
  ep.insert (db::Edge (-100, -100, -100, 600), 1);
  ep.insert (db::Edge (600, -100, 600, 600), 1);
  ep.insert (db::Edge (-100, -100, 600, -100), 1);

  ep.process (ec, op);

  s = tl::join (out2.begin (), out2.end (), ";");
  EXPECT_EQ (s, "(-100,-100;-100,500);(-100,-100;400,-100);(400,-100;400,0);(400,-100;600,-100);"
                "(600,-100;600,500);(400,400;400,500);(-100,500;-100,600);(-100,500;0,500);"
                "(300,500;400,500);(400,500;400,600);(400,500;600,500);(600,500;600,600)");

  s = tl::join (out.begin (), out.end (), ";");
  EXPECT_EQ (s, "(400,0;400,400);(0,500;300,500)");
}

//  TrapezoidGenerator

//  Basic
TEST(40)
{
  db::EdgeProcessor ep;
  ep.insert (db::Polygon (db::Box (0, 0, 1000, 1000)), 0);
  ep.insert (db::Polygon (db::Box (100, 100, 800, 800)), 1);

  std::vector<db::Polygon> out;
  db::PolygonContainer pc (out);
  db::TrapezoidGenerator pg (pc);
  db::BooleanOp2 op (db::BooleanOp::Xor, 1, -1);

  ep.process (pg, op);

  EXPECT_EQ (out.size (), size_t (4));
  EXPECT_EQ (out[0].to_string (), "(0,0;0,100;1000,100;1000,0)");
  EXPECT_EQ (out[1].to_string (), "(0,100;0,800;100,800;100,100)");
  EXPECT_EQ (out[2].to_string (), "(800,100;800,800;1000,800;1000,100)");
  EXPECT_EQ (out[3].to_string (), "(0,800;0,1000;1000,1000;1000,800)");
}

TEST(41)
{
  db::EdgeProcessor ep;
  ep.insert (db::Polygon (db::Box (0, 0, 1000, 1000)), 0);
  ep.insert (db::Polygon (db::Box (100, 100, 400, 400)), 1);
  ep.insert (db::Polygon (db::Box (400, 400, 800, 800)), 1);

  std::vector<db::Polygon> out;
  db::PolygonContainer pc (out);
  db::TrapezoidGenerator pg (pc);
  db::BooleanOp2 op (db::BooleanOp::Xor, 1, -1);

  ep.process (pg, op);

  EXPECT_EQ (out.size (), size_t (6));
  EXPECT_EQ (out[0].to_string (), "(0,0;0,100;1000,100;1000,0)");
  EXPECT_EQ (out[1].to_string (), "(0,100;0,400;100,400;100,100)");
  EXPECT_EQ (out[2].to_string (), "(400,100;400,400;1000,400;1000,100)");
  EXPECT_EQ (out[3].to_string (), "(0,400;0,800;400,800;400,400)");
  EXPECT_EQ (out[4].to_string (), "(800,400;800,800;1000,800;1000,400)");
  EXPECT_EQ (out[5].to_string (), "(0,800;0,1000;1000,1000;1000,800)");
}

TEST(42)
{
  db::EdgeProcessor ep;
  ep.insert (db::Polygon (db::Box (400, 0, 1000, 600)), 0);
  ep.insert (db::Polygon (db::Box (0, 400, 600, 1000)), 1);

  std::vector<db::Polygon> out;
  db::PolygonContainer pc (out);
  db::TrapezoidGenerator pg (pc);
  db::BooleanOp2 op (db::BooleanOp::Xor, 1, -1);

  ep.process (pg, op);

  EXPECT_EQ (out.size (), size_t (4));
  EXPECT_EQ (out[0].to_string (), "(400,0;400,400;1000,400;1000,0)");
  EXPECT_EQ (out[1].to_string (), "(0,400;0,600;400,600;400,400)");
  EXPECT_EQ (out[2].to_string (), "(600,400;600,600;1000,600;1000,400)");
  EXPECT_EQ (out[3].to_string (), "(0,600;0,1000;600,1000;600,600)");
}

TEST(43)
{
  db::EdgeProcessor ep;
  ep.insert (db::Edge (0, 0, 500, 1000), 0);
  ep.insert (db::Edge (500, 1000, 1000, 500), 0);
  ep.insert (db::Edge (1000, 500, 1000, 0), 0);
  ep.insert (db::Edge (1000, 0, 0, 0), 0);

  std::vector<db::Polygon> out;
  db::PolygonContainer pc (out);
  db::TrapezoidGenerator pg (pc);
  db::BooleanOp2 op (db::BooleanOp::Xor, 1, -1);

  ep.process (pg, op);

  EXPECT_EQ (out.size (), size_t (2));
  EXPECT_EQ (out[0].to_string (), "(0,0;250,500;1000,500;1000,0)");
  EXPECT_EQ (out[1].to_string (), "(250,500;500,1000;1000,500)");
}

TEST(44)
{
  db::EdgeProcessor ep;
  db::Point pts[] = {
    db::Point (0, 0),
    db::Point (0, 4000),
    db::Point (2000, 4000),
    db::Point (2500, 3000),
    db::Point (3000, 2500),
    db::Point (6500, 2000),
    db::Point (8000, 4000),
    db::Point (9000, 4000),
    db::Point (9000, 0)
  };
  db::Polygon p;
  p.assign_hull (&pts[0], &pts[sizeof(pts) / sizeof(pts[0])]);
  ep.insert (p);

  std::vector<db::Polygon> out;
  db::PolygonContainer pc (out);
  db::TrapezoidGenerator pg (pc);
  db::BooleanOp2 op (db::BooleanOp::Xor, 1, -1);

  ep.process (pg, op);

  EXPECT_EQ (out.size (), size_t (5));
  EXPECT_EQ (out[0].to_string (), "(0,0;0,2000;9000,2000;9000,0)");
  EXPECT_EQ (out[1].to_string (), "(0,2000;0,2500;3000,2500;6500,2000)");
  EXPECT_EQ (out[2].to_string (), "(0,2500;0,3000;2500,3000;3000,2500)");
  EXPECT_EQ (out[3].to_string (), "(0,3000;0,4000;2000,4000;2500,3000)");
  EXPECT_EQ (out[4].to_string (), "(6500,2000;8000,4000;9000,4000;9000,2000)");
}

TEST(45)
{
  db::EdgeProcessor ep;
  db::Point pts[] = {
    db::Point (0, 0),
    db::Point (0, 200),
    db::Point (200, 150),
    db::Point (250, 150),
    db::Point (300, 100),
    db::Point (800, 50),
    db::Point (900, 200),
    db::Point (1000, 200),
    db::Point (1000, 0)
  };
  db::Polygon p;
  p.assign_hull (&pts[0], &pts[sizeof(pts) / sizeof(pts[0])]);
  ep.insert (p);

  std::vector<db::Polygon> out;
  db::PolygonContainer pc (out);
  db::TrapezoidGenerator pg (pc);
  db::BooleanOp2 op (db::BooleanOp::Xor, 1, -1);

  ep.process (pg, op);

  EXPECT_EQ (out.size (), size_t (5));
  EXPECT_EQ (out[0].to_string (), "(0,0;0,50;1000,50;1000,0)");
  EXPECT_EQ (out[1].to_string (), "(0,50;0,100;300,100;800,50)");
  EXPECT_EQ (out[2].to_string (), "(0,100;0,150;250,150;300,100)");
  EXPECT_EQ (out[3].to_string (), "(0,150;0,200;200,150)");
  EXPECT_EQ (out[4].to_string (), "(800,50;900,200;1000,200;1000,50)");
}

TEST(46)
{
  db::EdgeProcessor ep;
  db::Point pts[] = {
    db::Point (0, 0),
    db::Point (300, 500),
    db::Point (800, 100),
    db::Point (300, 250),
    db::Point (350, 0),
    db::Point (400, 150)
  };
  db::Polygon p;
  p.assign_hull (&pts[0], &pts[sizeof(pts) / sizeof(pts[0])]);
  ep.insert (p);

  std::vector<db::Polygon> out;
  db::PolygonContainer pc (out);
  db::TrapezoidGenerator pg (pc);
  db::BooleanOp2 op (db::BooleanOp::Xor, -1, -1);

  ep.process (pg, op);

  EXPECT_EQ (out.size (), size_t (6));
  EXPECT_EQ (out[0].to_string (), "(0,0;73,122;326,122)");
  EXPECT_EQ (out[1].to_string (), "(350,0;326,122;391,122)");
  EXPECT_EQ (out[2].to_string (), "(326,122;400,150;391,122)");
  EXPECT_EQ (out[3].to_string (), "(73,122;150,250;300,250;326,122)");
  EXPECT_EQ (out[4].to_string (), "(800,100;300,250;613,250)");
  EXPECT_EQ (out[5].to_string (), "(150,250;300,500;613,250)");
}

// # 880
TEST(100)
{
  db::Layout layout_1, layout_2;
  db::Layout layout_au;
  unsigned int l1_l1d0 = 0, l1_l2d0 = 0;
  unsigned int l2_l1d0 = 0, l2_l2d0 = 0;

  {
    std::string fn (tl::testdata ());
    fn += "/bool/";
    fn += "sp1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);

    db::LayerMap lmap;
    db::LayerProperties p;

    p.layer = 1;
    p.datatype = 0;
    lmap.map (db::LDPair (1, 0), l1_l1d0 = layout_1.insert_layer ());
    layout_1.set_properties (l1_l1d0, p);

    p.layer = 2;
    p.datatype = 0;
    lmap.map (db::LDPair (2, 0), l1_l2d0 = layout_1.insert_layer ());
    layout_1.set_properties (l1_l2d0, p);

    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;
    reader.read (layout_1, options);
  }

  {
    std::string fn (tl::testdata ());
    fn += "/bool/";
    fn += "sp2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);

    db::LayerMap lmap;
    db::LayerProperties p;

    p.layer = 1;
    p.datatype = 0;
    lmap.map (db::LDPair (1, 0), l2_l1d0 = layout_2.insert_layer ());
    layout_2.set_properties (l2_l1d0, p);

    p.layer = 2;
    p.datatype = 0;
    lmap.map (db::LDPair (2, 0), l2_l2d0 = layout_2.insert_layer ());
    layout_2.set_properties (l2_l2d0, p);

    db::LoadLayoutOptions options;
    options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
    options.get_options<db::CommonReaderOptions> ().create_other_layers = false;
    reader.read (layout_2, options);
  }

  db::ShapeProcessor proc;

  db::Layout lr;
  lr.dbu (0.0001);
  db::Cell *lr_top = &lr.cell (lr.add_cell ("TOP"));

  unsigned int lr_l100d0 = lr.insert_layer (db::LayerProperties (100, 0));

  proc.boolean (layout_1, layout_1.cell (*layout_1.begin_top_down ()), l1_l1d0, 
                layout_2, layout_2.cell (*layout_2.begin_top_down ()), l2_l1d0, 
                lr_top->shapes (lr_l100d0), db::BooleanOp::Xor, true /*hierarchical*/, true /*resolve holes*/, true /*min coherence*/);

  unsigned int lr_l101d0 = lr.insert_layer (db::LayerProperties (101, 0));

  proc.boolean (layout_1, layout_1.cell (*layout_1.begin_top_down ()), l1_l1d0, 
                layout_2, layout_2.cell (*layout_2.begin_top_down ()), l2_l1d0, 
                lr_top->shapes (lr_l101d0), db::BooleanOp::Xor, false /*hierarchical*/, true /*resolve holes*/, true /*min coherence*/);

  unsigned int lr_l110d0 = lr.insert_layer (db::LayerProperties (110, 0));

  proc.size (layout_1, layout_1.cell (*layout_1.begin_top_down ()), l1_l1d0, 
             lr_top->shapes (lr_l110d0), 100, 200, 2, true /*hierarchical*/, true /*resolve holes*/, true /*min coherence*/);

  unsigned int lr_l111d0 = lr.insert_layer (db::LayerProperties (111, 0));

  proc.size (layout_1, layout_1.cell (*layout_1.begin_top_down ()), l1_l1d0, 
             lr_top->shapes (lr_l111d0), 100, 200, 2, false /*hierarchical*/, true /*resolve holes*/, true /*min coherence*/);

  unsigned int lr_l120d0 = lr.insert_layer (db::LayerProperties (120, 0));

  proc.merge (layout_1, layout_1.cell (*layout_1.begin_top_down ()), l1_l1d0, 
              lr_top->shapes (lr_l120d0), true /*hierarchical*/, 0, true /*resolve holes*/, true /*min coherence*/);

  unsigned int lr_l121d0 = lr.insert_layer (db::LayerProperties (121, 0));

  proc.merge (layout_1, layout_1.cell (*layout_1.begin_top_down ()), l1_l1d0, 
              lr_top->shapes (lr_l121d0), false /*hierarchical*/, 0, true /*resolve holes*/, true /*min coherence*/);

  unsigned int lr_l122d0 = lr.insert_layer (db::LayerProperties (122, 0));

  proc.merge (layout_1, layout_1.cell (*layout_1.begin_top_down ()), l1_l1d0, 
              lr_top->shapes (lr_l122d0), true /*hierarchical*/, 1, true /*resolve holes*/, true /*min coherence*/);

  unsigned int lr_l123d0 = lr.insert_layer (db::LayerProperties (123, 0));

  proc.merge (layout_1, layout_1.cell (*layout_1.begin_top_down ()), l1_l1d0, 
              lr_top->shapes (lr_l123d0), false /*hierarchical*/, 1, true /*resolve holes*/, true /*min coherence*/);

  std::string au_fn (tl::testdata ());
  au_fn += "/bool/";
  au_fn += "sp_au.gds";

  db::compare_layouts (_this, lr, au_fn);
}

//  #74 (GitHub)
std::string run_test101 (tl::TestBase *_this, const db::Trans &t)
{
  db::EdgeProcessor ep;

  {
    db::Point pts[] = {
      db::Point (0, 0),
      db::Point (0, 10),
      db::Point (10, 10),
      db::Point (10, 0)
    };
    db::Polygon p;
    p.assign_hull (&pts[0], &pts[sizeof(pts) / sizeof(pts[0])]);
    p.transform (t);
    ep.insert (p, 0);
  }

  {
    db::Point pts[] = {
      db::Point (-1, -1),
      db::Point (-1, 8),
      db::Point (2, 11),
      db::Point (2, -1)
    };
    db::Polygon p;
    p.assign_hull (&pts[0], &pts[sizeof(pts) / sizeof(pts[0])]);
    p.transform (t);
    ep.insert (p, 1);
  }

  {
    db::Point pts[] = {
      db::Point (2, -1),
      db::Point (2, 11),
      db::Point (11, 11),
      db::Point (11, -1)
    };
    db::Polygon p;
    p.assign_hull (&pts[0], &pts[sizeof(pts) / sizeof(pts[0])]);
    p.transform (t);
    ep.insert (p, 1);
  }

  std::vector<db::Polygon> out;
  db::PolygonContainer pc (out);
  db::PolygonGenerator pg (pc, false, true);
  db::BooleanOp op (db::BooleanOp::And);

  ep.process (pg, op);

  EXPECT_EQ (out.size (), size_t (1));

  return out.empty () ? std::string () : out.front ().to_string ();
}

TEST(101)
{
  EXPECT_EQ (run_test101 (_this, db::Trans (db::Trans::r0)), "(0,0;0,9;1,10;10,10;10,0)");
  EXPECT_EQ (run_test101 (_this, db::Trans (db::Trans::r90)), "(-9,0;-10,1;-10,10;0,10;0,0)");
  EXPECT_EQ (run_test101 (_this, db::Trans (db::Trans::r180)), "(-10,-10;-10,0;0,0;0,-9;-1,-10)");
  EXPECT_EQ (run_test101 (_this, db::Trans (db::Trans::r270)), "(0,-10;0,0;9,0;10,-1;10,-10)");
}

TEST(102)
{
  db::EdgeProcessor ep;

  {
    db::Point pts[] = {
      db::Point (0, 0),
      db::Point (0, 1000),
      db::Point (1000, 1000),
      db::Point (1000, 0)
    };
    db::Polygon p;
    p.assign_hull (&pts[0], &pts[sizeof(pts) / sizeof(pts[0])]);
    ep.insert (p, 0);
  }

  {
    db::Point pts[] = {
      db::Point (100, 100),
      db::Point (100, 200),
      db::Point (200, 200),
      db::Point (200, 100)
    };
    db::Polygon p;
    p.assign_hull (&pts[0], &pts[sizeof(pts) / sizeof(pts[0])]);
    ep.insert (p, 1);
  }

  {
    db::Point pts[] = {
      db::Point (500, 100),
      db::Point (500, 200),
      db::Point (600, 200),
      db::Point (600, 100)
    };
    db::Polygon p;
    p.assign_hull (&pts[0], &pts[sizeof(pts) / sizeof(pts[0])]);
    ep.insert (p, 1);
  }

  std::vector<db::Polygon> out;
  db::PolygonContainer pc (out);
  db::PolygonGenerator pg (pc, true, true);
  db::BooleanOp op (db::BooleanOp::ANotB);

  ep.process (pg, op);

  EXPECT_EQ (out.size (), size_t (1));
  EXPECT_EQ (out[0].to_string (), "(0,0;0,200;100,200;100,100;200,100;200,200;500,200;500,100;600,100;600,200;0,200;0,1000;1000,1000;1000,0)");
}

TEST(103)
{
  db::EdgeProcessor ep;

  {
    db::Point pts[] = {
      db::Point (0, 0),
      db::Point (0, 500),
      db::Point (1500, 500),
      db::Point (1500, 0),
      db::Point (1000, 0),
      db::Point (1000, 400),
      db::Point (500, 400),
      db::Point (500, 0)
    };
    db::Polygon p;
    p.assign_hull (&pts[0], &pts[sizeof(pts) / sizeof(pts[0])]);
    ep.insert (p, 0);
  }

  {
    db::Point pts[] = {
      db::Point (100, 100),
      db::Point (100, 400),
      db::Point (400, 400),
      db::Point (400, 100)
    };
    db::Polygon p;
    p.assign_hull (&pts[0], &pts[sizeof(pts) / sizeof(pts[0])]);
    ep.insert (p, 1);
  }

  {
    db::Point pts[] = {
      db::Point (1100, 100),
      db::Point (1100, 400),
      db::Point (1400, 400),
      db::Point (1400, 100)
    };
    db::Polygon p;
    p.assign_hull (&pts[0], &pts[sizeof(pts) / sizeof(pts[0])]);
    ep.insert (p, 1);
  }

  std::vector<db::Polygon> out;
  db::PolygonContainer pc (out);
  db::PolygonGenerator pg (pc, true, true);
  db::BooleanOp op (db::BooleanOp::ANotB);

  ep.process (pg, op);

  EXPECT_EQ (out.size (), size_t (1));
#if 1
  //  fast hole treatment
  EXPECT_EQ (out[0].to_string (), "(0,0;0,400;100,400;100,100;400,100;400,400;1100,400;1100,100;1400,100;1400,400;0,400;0,500;1500,500;1500,0;1000,0;1000,400;500,400;500,0)");
#else
  //  elaborate hole treatment
  EXPECT_EQ (out[0].to_string (), "(0,0;0,400;100,400;100,100;400,100;400,400;0,400;0,500;1500,500;1500,0;1000,0;1000,400;1100,400;1100,100;1400,100;1400,400;500,400;500,0)");
#endif

  //  test "redo" on this occasion

  db::PolygonContainer pc2 (out);
  db::PolygonGenerator pg2 (pc2, true, true);
  db::BooleanOp op2 (db::BooleanOp::ANotB);

  out.clear ();
  ep.redo (pg2, op2);

  EXPECT_EQ (out.size (), size_t (1));
#if 1
  //  fast hole treatment
  EXPECT_EQ (out[0].to_string (), "(0,0;0,400;100,400;100,100;400,100;400,400;1100,400;1100,100;1400,100;1400,400;0,400;0,500;1500,500;1500,0;1000,0;1000,400;500,400;500,0)");
#else
  //  elaborate hole treatment
  EXPECT_EQ (out[0].to_string (), "(0,0;0,400;100,400;100,100;400,100;400,400;0,400;0,500;1500,500;1500,0;1000,0;1000,400;1100,400;1100,100;1400,100;1400,400;500,400;500,0)");
#endif
}

//  Bug 134
TEST(134)
{
  const char *pd = "(30,-7957;0,0;56,-4102;30,-7921)";

  db::Coord dx = 0;
  db::Coord dy = -3999;
  unsigned int mode = 3;

  db::Polygon p;
  tl::from_string (pd, p);

  db::EdgeProcessor ep;
  db::Polygon ps (p.sized (dx, dy, mode));
  ep.insert (ps);

  db::SimpleMerge op (1 /*wc>0*/);
  std::vector<db::Polygon> out;
  db::PolygonContainer pc (out);
  db::PolygonGenerator pg (pc);
  ep.process (pg, op);

  EXPECT_EQ (out.size (), size_t (0));
}

void run_test135a (tl::TestBase *_this, const db::Trans &t)
{
  db::EdgeProcessor ep;

  db::Point pts[] = {
    db::Point (0, 0),
    db::Point (19, 19),
    db::Point (19, 18),
    db::Point (43, 32),
    db::Point (37, 27)
  };

  db::Polygon p;
  p.assign_hull (&pts[0], &pts[sizeof(pts) / sizeof(pts[0])]);
  p.transform (t);
  p.size (-2, -2, 2);

  ep.insert (p);

  //  this is just supposed to work and not fail with internal error "m_open.empty()"
  std::vector<db::Polygon> out;
  db::PolygonContainer pc (out);
  db::PolygonGenerator pg2 (pc, false /*don't resolve holes*/, true /*min. coherence*/);
  db::SimpleMerge op (1 /*wc>0*/);
  ep.process (pg2, op);

  EXPECT_EQ (out.size (), size_t (0));
}

TEST(135a)
{
  run_test135a (_this, db::Trans (db::Trans::r0));
  run_test135a (_this, db::Trans (db::Trans::r90));
  run_test135a (_this, db::Trans (db::Trans::r180));
  run_test135a (_this, db::Trans (db::Trans::r270));
  run_test135a (_this, db::Trans (db::Trans::m0));
  run_test135a (_this, db::Trans (db::Trans::m45));
  run_test135a (_this, db::Trans (db::Trans::m90));
  run_test135a (_this, db::Trans (db::Trans::m135));
}

std::string run_test135b (tl::TestBase *_this, const db::Trans &t)
{
  db::EdgeProcessor ep;

  db::Point pts[] = {
    db::Point (215, 0),
    db::Point (145, 11),
    db::Point (37, 31),
    db::Point (36, 31),
    db::Point (0, 43)
  };

  db::Polygon p;
  p.assign_hull (&pts[0], &pts[sizeof(pts) / sizeof(pts[0])]);
  p.transform (t);
  p.size (-2, -2, 2);

  ep.insert (p);

  //  this is just supposed to work and not fail with internal error "m_open.empty()"
  std::vector<db::Polygon> out;
  db::PolygonContainer pc (out);
  db::PolygonGenerator pg2 (pc, false /*don't resolve holes*/, true /*min. coherence*/);
  db::SimpleMerge op (1 /*wc>0*/);
  ep.process (pg2, op);

  EXPECT_EQ (out.size (), size_t (1));
  return out.empty () ? std::string () : out.front ().to_string ();
}

TEST(135b)
{
  EXPECT_EQ (run_test135b (_this, db::Trans (db::Trans::r0)), "(36,33;32,34;37,33)");
  EXPECT_EQ (run_test135b (_this, db::Trans (db::Trans::r90)), "(-35,32;-26,77;-33,37;-33,36)");
  EXPECT_EQ (run_test135b (_this, db::Trans (db::Trans::r180)), "(-33,-35;-78,-26;-37,-33;-36,-33)");
  EXPECT_EQ (run_test135b (_this, db::Trans (db::Trans::r270)), "(25,-78;33,-37;33,-36;34,-33)");
  EXPECT_EQ (run_test135b (_this, db::Trans (db::Trans::m0)), "(32,-35;36,-33;37,-33;77,-26)");
  EXPECT_EQ (run_test135b (_this, db::Trans (db::Trans::m45)), "(34,32;33,36;33,37)");
  EXPECT_EQ (run_test135b (_this, db::Trans (db::Trans::m90)), "(-78,25;-33,34;-36,33;-37,33)");
  EXPECT_EQ (run_test135b (_this, db::Trans (db::Trans::m135)), "(-26,-78;-35,-33;-33,-36;-33,-37)");
}

//  issue #1366
TEST(136)
{
  db::Layout layout_1;
  unsigned int l_l20000d0;

  {
    std::string fn (tl::testdata ());
    fn += "/bool/";
    fn += "issue_1366.oas";
    tl::InputStream stream (fn);
    db::Reader reader (stream);

    db::LoadLayoutOptions options;
    reader.read (layout_1, options);

    l_l20000d0 = layout_1.get_layer (db::LayerProperties (20000, 0));
  }

  db::ShapeProcessor proc;

  db::Layout lr;
  lr.dbu (0.0001);
  db::Cell *lr_top = &lr.cell (lr.add_cell ("TOP"));

  unsigned int lr_l100d0 = lr.insert_layer (db::LayerProperties (100, 0));

  proc.merge (layout_1, layout_1.cell (*layout_1.begin_top_down ()), l_l20000d0,
              lr_top->shapes (lr_l100d0), false /*hierarchical*/, 0, true /*resolve holes*/, true /*min coherence*/);

  std::string au_fn (tl::testdata ());
  au_fn += "/bool/";
  au_fn += "issue_1366_au.gds";

  db::compare_layouts (_this, lr, au_fn);
}
