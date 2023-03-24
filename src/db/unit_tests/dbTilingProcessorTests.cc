
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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
#include "tlThreads.h"

#include "dbTilingProcessor.h"
#include "dbTextWriter.h"
#include "gsiExpression.h"
#include "gsiDecl.h"
#include "dbWriter.h"
#include "dbSaveLayoutOptions.h"
#include "dbShapeProcessor.h"

#include <cstdlib>

unsigned int get_rand()
{
  //  provide a 32bit random number also for MSVC's 16bit rand():
  return (((unsigned int) rand ()) << 16) ^ ((unsigned int) rand ());
}

TEST(1a) 
{
  db::Layout out;
  unsigned int o1 = out.insert_layer ();
  db::Cell *otop = &out.cell (out.add_cell ("TOP"));

  db::TilingProcessor tp;
  tp.output ("o", out, otop->cell_index (), o1);
  tp.queue ("_output(o, Box.new(0, 0, 1000, 2000))");
  tp.execute ("test");

  tl::OutputStringStream sstream;
  tl::OutputStream stream (sstream);
  db::TextWriter writer (stream);
  writer.write (out);
  //  tiles not specified, bbox is empty -> no execution
  EXPECT_EQ (sstream.string (), "begin_lib 0.001\nbegin_cell {TOP}\nend_cell\nend_lib\n");
}

TEST(1b) 
{
  db::Layout out;
  unsigned int o1 = out.insert_layer ();
  db::Cell *otop = &out.cell (out.add_cell ("TOP"));

  db::TilingProcessor tp;
  tp.output ("o", out, otop->cell_index (), o1);
  tp.queue ("_output(o, Box.new(0, 0, 1000, 2000))");
  tp.execute ("test");

  tp.tile_size (1.0, 1.0);
  tp.tiles (1, 1);
  tp.tile_origin (0.0, 0.0);
  tp.execute ("test");

  tl::OutputStringStream sstream;
  tl::OutputStream stream (sstream);
  db::TextWriter writer (stream);
  writer.write (out);
  EXPECT_EQ (sstream.string (), "begin_lib 0.001\nbegin_cell {TOP}\nbox -1 -1 {0 0} {1000 2000}\nend_cell\nend_lib\n");
}

TEST(1c) 
{
  db::Layout out;
  unsigned int o1 = out.insert_layer ();
  db::Cell *otop = &out.cell (out.add_cell ("TOP"));

  db::TilingProcessor tp;
  tp.output ("o", out, otop->cell_index (), o1);
  tp.var ("bx", tl::Variant (db::Box (0, 0, 1000, 2000)));
  tp.queue ("_output(o, bx)");

  tp.tile_size (1.0, 1.0);
  tp.tiles (1, 1);
  tp.tile_origin (0.0, 0.0);
  tp.execute ("test");

  tl::OutputStringStream sstream;
  tl::OutputStream stream (sstream);
  db::TextWriter writer (stream);
  writer.write (out);
  EXPECT_EQ (sstream.string (), "begin_lib 0.001\nbegin_cell {TOP}\nbox -1 -1 {0 0} {1000 2000}\nend_cell\nend_lib\n");
}

static std::string to_s (const db::Layout &ly, db::cell_index_type top, unsigned int layer)
{
  std::string r;
  for (db::RecursiveShapeIterator i (ly, ly.cell (top), layer); ! i.at_end (); ++i) {
    if (! r.empty ()) {
      r += ";";
    }
    r += i.shape ().to_string ();
  }
  return r;
}

TEST(2)
{
  db::Layout ly;
  unsigned int l1 = ly.insert_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.insert_layer (db::LayerProperties (2, 0));
  unsigned int o1 = ly.insert_layer (db::LayerProperties (10, 0));
  unsigned int o2 = ly.insert_layer (db::LayerProperties (11, 0));
  unsigned int o3 = ly.insert_layer (db::LayerProperties (12, 0));
  db::cell_index_type top = ly.add_cell ("TOP");
  db::cell_index_type c1 = ly.add_cell ("C1");
  db::cell_index_type c2 = ly.add_cell ("C2");
  ly.cell (c1).shapes (l1).insert (db::Box (0, 0, 30, 30));
  ly.cell (c2).shapes (l2).insert (db::Box (0, 0, 30, 30));
  ly.cell (top).insert (db::CellInstArray (c1, db::Trans (db::Vector (0, 0))));
  ly.cell (top).insert (db::CellInstArray (c1, db::Trans (db::Vector (50, 0))));
  ly.cell (top).insert (db::CellInstArray (c1, db::Trans (db::Vector (50, 40))));
  ly.cell (top).insert (db::CellInstArray (c2, db::Trans (db::Vector (10, 10))));
  ly.cell (top).insert (db::CellInstArray (c2, db::Trans (db::Vector (80, 40))));
  ly.cell (top).insert (db::CellInstArray (c2, db::Trans (db::Vector (110, 40))));
  ly.cell (top).shapes (l2).insert (db::Box (60, 10, 70, 20));

  {
    db::TilingProcessor tp;
    tp.input ("i1", db::RecursiveShapeIterator (ly, ly.cell (top), l1));
    tp.input ("i2", db::RecursiveShapeIterator (ly, ly.cell (top), l2));
    tp.output ("o1", ly, top, o1);
    tp.output ("o2", ly, top, o2);
    tp.output ("o3", ly, top, o3);
    tp.queue ("_output(o1, _tile ? (i1 & i2 & _tile) : (i1 & i2), false)");
    tp.queue ("!_tile && _output(o2, i1.outside(i2), false)");
    tp.queue ("_tile && _output(o3, _tile, false)");
    tp.execute ("test");

    EXPECT_EQ (to_s (ly, top, o1), "box (60,10;70,20);box (10,10;30,30)");
    EXPECT_EQ (to_s (ly, top, o2), "box (50,40;80,70)");
    EXPECT_EQ (to_s (ly, top, o3), "");

    ly.clear_layer (o1);
    ly.clear_layer (o2);

    EXPECT_EQ (to_s (ly, top, o1), "");
    EXPECT_EQ (to_s (ly, top, o2), "");

    tp.tile_size (0.025, 0.025);

    tp.execute ("test");

    EXPECT_EQ (to_s (ly, top, o1), "box (10,10;20,23);box (10,23;20,30);box (20,10;30,23);box (20,23;30,30);box (60,10;70,20)");
    EXPECT_EQ (to_s (ly, top, o2), "");
    EXPECT_EQ (to_s (ly, top, o3), "box (-5,-2;20,23);box (-5,23;20,48);box (-5,48;20,73);box (20,-2;45,23);box (20,23;45,48);box (20,48;45,73);box (45,-2;70,23);box (45,23;70,48);box (45,48;70,73);box (70,-2;95,23);box (70,23;95,48);box (70,48;95,73);box (95,-2;120,23);box (95,23;120,48);box (95,48;120,73);box (120,-2;145,23);box (120,23;145,48);box (120,48;145,73)");
  }

  {
    ly.clear_layer (o1);
    ly.clear_layer (o2);
    ly.clear_layer (o3);

    db::TilingProcessor tp;
    tp.input ("i1", db::RecursiveShapeIterator (ly, ly.cell (top), l1));
    tp.input ("i2", db::RecursiveShapeIterator (ly, ly.cell (top), l2));
    tp.output ("o1", ly, top, o1);
    tp.output ("o2", ly, top, o2);
    tp.output ("o3", ly, top, o3);
    tp.queue ("_output(o1, i1 & i2)");
    tp.queue ("!_tile && _output(o2, i1.outside(i2))");
    tp.queue ("_output(o3, _tile)");
    tp.execute ("test");

    EXPECT_EQ (to_s (ly, top, o1), "box (60,10;70,20);box (10,10;30,30)");
    EXPECT_EQ (to_s (ly, top, o2), "box (50,40;80,70)");
    EXPECT_EQ (to_s (ly, top, o3), "");

    ly.clear_layer (o1);
    ly.clear_layer (o2);

    EXPECT_EQ (to_s (ly, top, o1), "");
    EXPECT_EQ (to_s (ly, top, o2), "");

    tp.tile_size (0.025, 0.025);

    tp.execute ("test");

    EXPECT_EQ (to_s (ly, top, o1), "box (10,10;20,23);box (10,23;20,30);box (20,10;30,23);box (20,23;30,30);box (60,10;70,20)");
    EXPECT_EQ (to_s (ly, top, o2), "");
    EXPECT_EQ (to_s (ly, top, o3), "box (-5,-2;20,23);box (-5,23;20,48);box (-5,48;20,73);box (20,-2;45,23);box (20,23;45,48);box (20,48;45,73);box (45,-2;70,23);box (45,23;70,48);box (45,48;70,73);box (70,-2;95,23);box (70,23;95,48);box (70,48;95,73);box (95,-2;120,23);box (95,23;120,48);box (95,48;120,73);box (120,-2;145,23);box (120,23;145,48);box (120,48;145,73)");
  }
}

TEST(3)
{
  db::Layout ly;
  unsigned int l1 = ly.insert_layer (db::LayerProperties (1, 0));
  unsigned int l2 = ly.insert_layer (db::LayerProperties (2, 0));
  unsigned int l3 = ly.insert_layer (db::LayerProperties (3, 0));
  unsigned int o1 = ly.insert_layer (db::LayerProperties (10, 0));
  unsigned int o2 = ly.insert_layer (db::LayerProperties (11, 0));
  unsigned int o3 = ly.insert_layer (db::LayerProperties (12, 0));
  unsigned int q1 = ly.insert_layer (db::LayerProperties (20, 0));
  unsigned int q2 = ly.insert_layer (db::LayerProperties (21, 0));
  unsigned int q3 = ly.insert_layer (db::LayerProperties (22, 0));
  db::cell_index_type top = ly.add_cell ("TOP");

  for (size_t i = 0; i < 50000; ++i) {
    db::Coord x = get_rand () % 10000000;
    db::Coord y = get_rand () % 10000000;
    ly.cell (top).shapes (l1).insert (db::Box (x, y, x + 10000, y + 10000));
    x = get_rand () % 10000000;
    y = get_rand () % 10000000;
    ly.cell (top).shapes (l2).insert (db::Box (x, y, x + 10000, y + 10000));
    x = get_rand () % 10000000;
    y = get_rand () % 10000000;
    ly.cell (top).shapes (l3).insert (db::Box (x, y, x + 10000, y + 10000));
  }

  db::TilingProcessor tp;
  tp.input ("i1", db::RecursiveShapeIterator (ly, ly.cell (top), l1));
  db::Region ir2 (db::RecursiveShapeIterator (ly, ly.cell (top), l2));
  tp.input ("i2", ir2.begin_iter ().first, ir2.begin_iter ().second);
  EXPECT_EQ (ir2.has_valid_polygons (), false);
  db::Region ir3 (db::RecursiveShapeIterator (ly, ly.cell (top), l3));
  ir3.flatten ();
  tp.input ("i3", ir3.begin_iter ().first, ir3.begin_iter ().second);
  EXPECT_EQ (ir3.has_valid_polygons (), true);
  tp.output ("o1", ly, top, o1);
  db::Region or2;
  tp.output ("o2", or2);
  tp.output ("o3", ly, top, o3);
  tp.queue ("_output(o1, i1 ^ i2)");
  tp.queue ("_output(o2, i1 ^ i3)");
  tp.queue ("_output(o3, i2 ^ i3)");

  int v = tl::verbosity ();
  tl::verbosity (21);
  tp.execute ("test");
  tl::verbosity (v);

  for (db::Region::const_iterator o = or2.begin (); ! o.at_end (); ++o) {
    ly.cell (top).shapes (o2).insert (*o);
  }

  ly.swap_layers (o1, q1);
  ly.swap_layers (o2, q2);
  ly.swap_layers (o3, q3);

  tp.tile_size (100, 100);
  tp.set_threads (2);

  db::Region or2_copy;
  or2.swap (or2_copy);

  v = tl::verbosity ();
  tl::verbosity (11);
  tp.execute ("test");
  tl::verbosity (v);

  for (db::Region::const_iterator o = or2.begin (); ! o.at_end (); ++o) {
    ly.cell (top).shapes (o2).insert (*o);
  }

  EXPECT_EQ (or2.has_valid_polygons (), true);
  EXPECT_EQ (or2.count () / 2000, size_t (50));  //  because we use rand () the value may vary - it's only accurate to 2%
  EXPECT_EQ (or2_copy.has_valid_polygons (), true);
  EXPECT_EQ (or2_copy.count () / 2000, size_t (40));  //  because we use rand () the value may vary - it's only accurate to 2%
  EXPECT_EQ ((or2 ^ or2_copy).empty (), true);

#if 0
  {
    db::SaveLayoutOptions options;
    db::Writer writer (options);
    tl::OutputFile file ("x.gds");
    writer.write (ly, file);
  }
#endif

  db::ShapeProcessor sp;

  EXPECT_EQ (ly.cell (top).shapes (o1).empty (), false);
  EXPECT_EQ (ly.cell (top).shapes (q1).empty (), false);
  db::Shapes x1;
  sp.boolean (ly, ly.cell (top), o1, ly, ly.cell (top), q1, x1, db::BooleanOp::Xor, true);
  EXPECT_EQ (x1.empty (), true);

  EXPECT_EQ (ly.cell (top).shapes (o2).empty (), false);
  EXPECT_EQ (ly.cell (top).shapes (q2).empty (), false);
  db::Shapes x2;
  sp.boolean (ly, ly.cell (top), o2, ly, ly.cell (top), q2, x2, db::BooleanOp::Xor, true);
  EXPECT_EQ (x2.empty (), true);

  EXPECT_EQ (ly.cell (top).shapes (o3).empty (), false);
  EXPECT_EQ (ly.cell (top).shapes (q3).empty (), false);
  db::Shapes x3;
  sp.boolean (ly, ly.cell (top), o3, ly, ly.cell (top), q3, x3, db::BooleanOp::Xor, true);
  EXPECT_EQ (x3.empty (), true);
}

TEST(4)
{
  //  different DBU's

  db::Layout ly1;
  ly1.dbu (0.01);
  unsigned int l11 = ly1.insert_layer (db::LayerProperties (1, 0));
  db::cell_index_type top1 = ly1.add_cell ("TOP");
  ly1.cell (top1).shapes (l11).insert (db::Box (10, 20, 30, 40));

  db::Layout ly2;
  ly2.dbu (0.001);
  unsigned int l12 = ly2.insert_layer (db::LayerProperties (1, 0));
  db::cell_index_type top2 = ly2.add_cell ("TOP");
  ly2.cell (top2).shapes (l12).insert (db::Box (100, 200, 301, 401));

  db::Layout o;
  o.dbu (0.0001);
  unsigned int l1o = o.insert_layer (db::LayerProperties (1, 0));
  unsigned int l2o = o.insert_layer (db::LayerProperties (2, 0));
  unsigned int l3o = o.insert_layer (db::LayerProperties (3, 0));
  db::cell_index_type topo = o.add_cell ("TOP");

  {
    db::TilingProcessor tp;
    tp.input ("i1", db::RecursiveShapeIterator (ly1, ly1.cell (top1), l11));
    tp.input ("i2", db::RecursiveShapeIterator (ly2, ly2.cell (top2), l12));
    tp.output ("o1", o, topo, l1o);
    tp.output ("o2", o, topo, l2o);
    tp.output ("o3", o, topo, l3o);
    tp.queue ("_output(o1, _tile ? ((i1 ^ i2) & _tile) : (i1 ^ i2), false)");
    tp.queue ("_output(o2, i1, true)");
    tp.queue ("_output(o3, i2, true)");
    tp.execute ("test");

    EXPECT_EQ (to_s (o, topo, l1o), "");
    EXPECT_EQ (to_s (o, topo, l2o), "box (1000,2000;3000,4000)");
    EXPECT_EQ (to_s (o, topo, l3o), "box (1000,2000;3000,4000)");

    o.clear_layer (l1o);
    o.clear_layer (l2o);
    o.clear_layer (l3o);

    tp.set_dbu (0.001);
    tp.execute ("test");

    EXPECT_EQ (to_s (o, topo, l1o), "polygon (3000,2000;3000,4000;1000,4000;1000,4010;3010,4010;3010,2000)");
    EXPECT_EQ (to_s (o, topo, l2o), "box (1000,2000;3000,4000)");
    EXPECT_EQ (to_s (o, topo, l3o), "box (1000,2000;3010,4010)");

    o.clear_layer (l1o);
    o.clear_layer (l2o);
    o.clear_layer (l3o);

    tp.tile_size (0.1, 0.1);

    tp.execute ("test");

    EXPECT_EQ (to_s (o, topo, l1o), "polygon (3000,3510;3000,4000;2510,4000;2510,4010;3010,4010;3010,3510);box (1000,4000;1510,4010);box (1510,4000;2510,4010);box (3000,2000;3010,2510);box (3000,2510;3010,3510)");

    o.clear_layer (l1o);
    o.clear_layer (l2o);
    o.clear_layer (l3o);

    tp.tile_size (1000, 1000);
    tp.set_scale_to_dbu (false);
    tp.execute ("test");

    EXPECT_EQ (to_s (o, topo, l1o), "box (100,200;300,400);box (1000,2000;3010,4010)");
    EXPECT_EQ (to_s (o, topo, l2o), "box (100,200;300,400)");
    EXPECT_EQ (to_s (o, topo, l3o), "box (1000,2000;3010,4010)");

  }

}

class MyTilingOutputReceiver
  : public db::TileOutputReceiver
{
public:
  MyTilingOutputReceiver (double *sum, int *n)
    : mp_sum (sum), mp_n (n)
  { }

  MyTilingOutputReceiver ()
    : mp_sum (0), mp_n (0)
  { }

  void add (double x) const
  {
    static tl::Mutex lock;
    tl::MutexLocker locker (&lock);
    *mp_sum += x;
    *mp_n += 1;
  }

private:
  double *mp_sum;
  int *mp_n;
};

namespace gsi
{
DB_PUBLIC gsi::Class<db::TileOutputReceiver> &dbdecl_TileOutputReceiverBase ();
}

gsi::Class<MyTilingOutputReceiver> decl_MyTilingOutputReceiver (gsi::dbdecl_TileOutputReceiverBase (), "db", "MyTileOutputReceiver",
  gsi::method ("add", &MyTilingOutputReceiver::add)
);

//  Multithreaded, access to _rec()
//  This will mainly test the ability of gsi::Proxy to manage references
//  in a multithreaded case.
TEST(5)
{
  db::Layout ly1;
  ly1.dbu (0.001);
  unsigned int l11 = ly1.insert_layer (db::LayerProperties (1, 0));
  db::cell_index_type top1 = ly1.add_cell ("TOP");
  ly1.cell (top1).shapes (l11).insert (db::Box (0, 0, 50000, 50000));

  double sum = 0.0;
  int num = 0;
  MyTilingOutputReceiver *rec = new MyTilingOutputReceiver (&sum, &num);

  db::TilingProcessor tp;
  tp.set_threads (4);
  tp.tile_size (0.11, 0.17);
  tp.input ("i1", db::RecursiveShapeIterator (ly1, ly1.cell (top1), l11));
  tp.output ("o1", 0, rec, db::ICplxTrans ());
  tp.queue ("_rec(o1).add((i1 & _tile).area)");
  tp.execute ("test");

  EXPECT_EQ (sum, 2500000000);
  EXPECT_EQ (num, 134225);
}
