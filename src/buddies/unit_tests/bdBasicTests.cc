
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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

#include "bdWriterOptions.h"
#include "bdReaderOptions.h"
#include "tlCommandLineParser.h"
#include "tlUnitTest.h"
#include "dbLayout.h"
#include "dbCell.h"

//  Testing writer options
TEST(1)
{
  bd::GenericWriterOptions opt;
  tl::CommandLineOptions cmd;

  opt.add_options (cmd);

  char *argv[] = { "x",
                   "-os=1.25",
                   "-od=0.125",
                   "--drop-empty-cells",
                   "--keep-instances",
                   "--no-context-info",
                   //  CIF
                   "--blank-separator",
                   "--dummy-calls",
                   //  DXF
                   "-op=2",
                   //  GDS2
                   "-ol=MYLIBNAME",
                   "-ov=250",
                   "--multi-xy-records",
                   "--no-timestamps",
                   "--no-zero-length-paths",
                   "--user-units=2.5",
                   "--write-cell-properties",
                   "--write-file-properties",
                   //  OASIS
                   "-ob",
                   "-ok=9",
                   "-ot",
                   "--recompress",
                   "--subst-char=XY",
                   "--write-std-properties=2"
                 };

  cmd.parse (sizeof (argv) / sizeof (argv[0]), argv);

  db::Layout layout;

  db::SaveLayoutOptions stream_opt;
  EXPECT_EQ (stream_opt.dont_write_empty_cells (), false);
  EXPECT_EQ (stream_opt.keep_instances (), false);
  EXPECT_EQ (stream_opt.write_context_info (), true);
  EXPECT_EQ (stream_opt.get_options<db::CIFWriterOptions> ().blank_separator, false);
  EXPECT_EQ (stream_opt.get_options<db::CIFWriterOptions> ().dummy_calls, false);
  EXPECT_EQ (stream_opt.get_options<db::DXFWriterOptions> ().polygon_mode, 0);
  EXPECT_EQ (stream_opt.get_options<db::GDS2WriterOptions> ().libname, "LIB");
  EXPECT_EQ (stream_opt.get_options<db::GDS2WriterOptions> ().max_vertex_count, (unsigned int) 8000);
  EXPECT_EQ (stream_opt.get_options<db::GDS2WriterOptions> ().multi_xy_records, false);
  EXPECT_EQ (stream_opt.get_options<db::GDS2WriterOptions> ().write_timestamps, true);
  EXPECT_EQ (stream_opt.get_options<db::GDS2WriterOptions> ().no_zero_length_paths, false);
  EXPECT_EQ (tl::to_string (stream_opt.get_options<db::GDS2WriterOptions> ().user_units), "1");
  EXPECT_EQ (stream_opt.get_options<db::GDS2WriterOptions> ().write_cell_properties, false);
  EXPECT_EQ (stream_opt.get_options<db::GDS2WriterOptions> ().write_file_properties, false);
  EXPECT_EQ (stream_opt.get_options<db::OASISWriterOptions> ().write_cblocks, false);
  EXPECT_EQ (stream_opt.get_options<db::OASISWriterOptions> ().compression_level, 2);
  EXPECT_EQ (stream_opt.get_options<db::OASISWriterOptions> ().strict_mode, false);
  EXPECT_EQ (stream_opt.get_options<db::OASISWriterOptions> ().recompress, false);
  EXPECT_EQ (stream_opt.get_options<db::OASISWriterOptions> ().subst_char, "*");
  EXPECT_EQ (stream_opt.get_options<db::OASISWriterOptions> ().write_std_properties, 1);

  opt.configure (stream_opt, layout);

  EXPECT_EQ (stream_opt.scale_factor (), 1.25);
  EXPECT_EQ (stream_opt.dbu (), 0.125);
  EXPECT_EQ (stream_opt.dont_write_empty_cells (), true);
  EXPECT_EQ (stream_opt.keep_instances (), true);
  EXPECT_EQ (stream_opt.write_context_info (), false);
  EXPECT_EQ (stream_opt.get_options<db::CIFWriterOptions> ().blank_separator, true);
  EXPECT_EQ (stream_opt.get_options<db::CIFWriterOptions> ().dummy_calls, true);
  EXPECT_EQ (stream_opt.get_options<db::DXFWriterOptions> ().polygon_mode, 2);
  EXPECT_EQ (stream_opt.get_options<db::GDS2WriterOptions> ().libname, "MYLIBNAME");
  EXPECT_EQ (stream_opt.get_options<db::GDS2WriterOptions> ().max_vertex_count, (unsigned int) 250);
  EXPECT_EQ (stream_opt.get_options<db::GDS2WriterOptions> ().multi_xy_records, true);
  EXPECT_EQ (stream_opt.get_options<db::GDS2WriterOptions> ().write_timestamps, false);
  EXPECT_EQ (stream_opt.get_options<db::GDS2WriterOptions> ().no_zero_length_paths, true);
  EXPECT_EQ (tl::to_string (stream_opt.get_options<db::GDS2WriterOptions> ().user_units), "2.5");
  EXPECT_EQ (stream_opt.get_options<db::GDS2WriterOptions> ().write_cell_properties, true);
  EXPECT_EQ (stream_opt.get_options<db::GDS2WriterOptions> ().write_file_properties, true);
  EXPECT_EQ (stream_opt.get_options<db::OASISWriterOptions> ().write_cblocks, true);
  EXPECT_EQ (stream_opt.get_options<db::OASISWriterOptions> ().compression_level, 9);
  EXPECT_EQ (stream_opt.get_options<db::OASISWriterOptions> ().strict_mode, true);
  EXPECT_EQ (stream_opt.get_options<db::OASISWriterOptions> ().recompress, true);
  EXPECT_EQ (stream_opt.get_options<db::OASISWriterOptions> ().subst_char, "X");
  EXPECT_EQ (stream_opt.get_options<db::OASISWriterOptions> ().write_std_properties, 2);
}

static std::string cells2string (const db::Layout &layout, const std::set<db::cell_index_type> &cells)
{
  std::string res;
  for (std::set<db::cell_index_type>::const_iterator c = cells.begin (); c != cells.end (); ++c) {
    if (c != cells.begin ()) {
      res += ",";
    }
    res += layout.cell_name (*c);
  }
  return res;
}

//  Testing writer options: cell resolution
TEST(2)
{
  //  Build a layout with the hierarchy
  //    TOP -> A, B
  //    A -> B
  //    B -> C
  //    C -> D
  db::Layout layout;
  db::cell_index_type itop = layout.add_cell ("TOP");
  db::cell_index_type ia = layout.add_cell ("A");
  db::cell_index_type ib = layout.add_cell ("B");
  db::cell_index_type ic = layout.add_cell ("C");
  db::cell_index_type id = layout.add_cell ("D");
  layout.cell (itop).insert (db::CellInstArray (ia, db::Trans ()));
  layout.cell (itop).insert (db::CellInstArray (ib, db::Trans ()));
  layout.cell (ia).insert (db::CellInstArray (ib, db::Trans ()));
  layout.cell (ib).insert (db::CellInstArray (ic, db::Trans ()));
  layout.cell (ic).insert (db::CellInstArray (id, db::Trans ()));

  bd::GenericWriterOptions opt;
  tl::CommandLineOptions cmd;
  opt.add_options (cmd);

  std::set <db::cell_index_type> cells;
  std::vector <std::pair <unsigned int, db::LayerProperties> > valid_layers;
  db::SaveLayoutOptions stream_opt;

  {
    const char *argv[] = { "x",
                           "--write-cells=A,-C,(C)",
                         };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }

  opt.configure (stream_opt, layout);

  cells.clear ();
  valid_layers.clear ();
  stream_opt.get_cells (layout, cells, valid_layers);

  EXPECT_EQ (cells2string (layout, cells), "A,B,C");

  {
    const char *argv[] = { "x",
                           "--write-cells=(C),(TOP)",
                         };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }

  opt.configure (stream_opt, layout);

  cells.clear ();
  valid_layers.clear ();
  stream_opt.get_cells (layout, cells, valid_layers);

  EXPECT_EQ (cells2string (layout, cells), "TOP,C");

  {
    const char *argv[] = { "x",
                           "--write-cells=(TOP),+B",
                         };
    cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);
  }

  opt.configure (stream_opt, layout);

  cells.clear ();
  valid_layers.clear ();
  stream_opt.get_cells (layout, cells, valid_layers);

  EXPECT_EQ (cells2string (layout, cells), "TOP,B,C,D");
}

//  Testing reader options
TEST(10)
{
  bd::GenericReaderOptions opt;
  tl::CommandLineOptions cmd;

  opt.add_options (cmd);

  const char *argv[] = { "x",
                         //  CIF and DXF
                         "-id=0.125",
                         //  CIF
                         "-iw=1",
                         //  DXF
                         "-iu=2.5",
                         "--dxf-circle-accuracy=0.5",
                         "--dxf-circle-points=1000",
                         "--dxf-keep-other-cells",
                         "--dxf-polyline-mode=3",
                         "--dxf-render-texts-as-polygons",
                         "--dxf-text-scaling=75",
                         //  GDS2 and OASIS
                         "--no-properties",
                         "--no-texts",
                         //  GDS2
                         "-ib=3",
                         "--no-big-records",
                         "--no-multi-xy-records",
                         //  General
                         "-im=1/0 3,4/0-255 A:17/0",
                         "-is",
                         //  OASIS
                         "--expect-strict-mode=1"
                       };

  cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);

  db::LoadLayoutOptions stream_opt;
  EXPECT_EQ (tl::to_string (stream_opt.get_options<db::CIFReaderOptions> ().dbu), "0.001");
  EXPECT_EQ (stream_opt.get_options<db::CIFReaderOptions> ().wire_mode, (unsigned int) 0);
  EXPECT_EQ (stream_opt.get_options<db::CIFReaderOptions> ().layer_map.to_string (), "layer_map()");
  EXPECT_EQ (stream_opt.get_options<db::CIFReaderOptions> ().create_other_layers, true);
  EXPECT_EQ (tl::to_string (stream_opt.get_options<db::DXFReaderOptions> ().dbu), "0.001");
  EXPECT_EQ (stream_opt.get_options<db::DXFReaderOptions> ().layer_map.to_string (), "layer_map()");
  EXPECT_EQ (stream_opt.get_options<db::DXFReaderOptions> ().create_other_layers, true);
  EXPECT_EQ (stream_opt.get_options<db::DXFReaderOptions> ().unit, 1.0);
  EXPECT_EQ (tl::to_string (stream_opt.get_options<db::DXFReaderOptions> ().circle_accuracy), "0");
  EXPECT_EQ (stream_opt.get_options<db::DXFReaderOptions> ().circle_points, 100);
  EXPECT_EQ (stream_opt.get_options<db::DXFReaderOptions> ().keep_other_cells, false);
  EXPECT_EQ (stream_opt.get_options<db::DXFReaderOptions> ().polyline_mode, 0);
  EXPECT_EQ (stream_opt.get_options<db::DXFReaderOptions> ().render_texts_as_polygons, false);
  EXPECT_EQ (stream_opt.get_options<db::DXFReaderOptions> ().text_scaling, 100);
  EXPECT_EQ (stream_opt.get_options<db::CommonReaderOptions> ().layer_map.to_string (), "layer_map()");
  EXPECT_EQ (stream_opt.get_options<db::CommonReaderOptions> ().create_other_layers, true);
  EXPECT_EQ (stream_opt.get_options<db::CommonReaderOptions> ().enable_properties, true);
  EXPECT_EQ (stream_opt.get_options<db::CommonReaderOptions> ().enable_text_objects, true);
  EXPECT_EQ (stream_opt.get_options<db::GDS2ReaderOptions> ().box_mode, (unsigned int) 1);
  EXPECT_EQ (stream_opt.get_options<db::GDS2ReaderOptions> ().allow_big_records, true);
  EXPECT_EQ (stream_opt.get_options<db::GDS2ReaderOptions> ().allow_multi_xy_records, true);
  EXPECT_EQ (stream_opt.get_options<db::OASISReaderOptions> ().expect_strict_mode, -1);

  opt.configure (stream_opt);

  EXPECT_EQ (tl::to_string (stream_opt.get_options<db::CIFReaderOptions> ().dbu), "0.125");
  EXPECT_EQ (stream_opt.get_options<db::CIFReaderOptions> ().wire_mode, (unsigned int) 1);
  EXPECT_EQ (stream_opt.get_options<db::CIFReaderOptions> ().layer_map.to_string (), "layer_map('1/0';'3-4/0-255';'A : 17/0')");
  EXPECT_EQ (stream_opt.get_options<db::CIFReaderOptions> ().create_other_layers, false);
  EXPECT_EQ (tl::to_string (stream_opt.get_options<db::DXFReaderOptions> ().dbu), "0.125");
  EXPECT_EQ (stream_opt.get_options<db::DXFReaderOptions> ().layer_map.to_string (), "layer_map('1/0';'3-4/0-255';'A : 17/0')");
  EXPECT_EQ (stream_opt.get_options<db::DXFReaderOptions> ().create_other_layers, false);
  EXPECT_EQ (stream_opt.get_options<db::DXFReaderOptions> ().unit, 2.5);
  EXPECT_EQ (tl::to_string (stream_opt.get_options<db::DXFReaderOptions> ().circle_accuracy), "0.5");
  EXPECT_EQ (stream_opt.get_options<db::DXFReaderOptions> ().circle_points, 1000);
  EXPECT_EQ (stream_opt.get_options<db::DXFReaderOptions> ().keep_other_cells, true);
  EXPECT_EQ (stream_opt.get_options<db::DXFReaderOptions> ().polyline_mode, 3);
  EXPECT_EQ (stream_opt.get_options<db::DXFReaderOptions> ().render_texts_as_polygons, true);
  EXPECT_EQ (stream_opt.get_options<db::DXFReaderOptions> ().text_scaling, 75);
  EXPECT_EQ (stream_opt.get_options<db::CommonReaderOptions> ().layer_map.to_string (), "layer_map('1/0';'3-4/0-255';'A : 17/0')");
  EXPECT_EQ (stream_opt.get_options<db::CommonReaderOptions> ().create_other_layers, false);
  EXPECT_EQ (stream_opt.get_options<db::CommonReaderOptions> ().enable_properties, false);
  EXPECT_EQ (stream_opt.get_options<db::CommonReaderOptions> ().enable_text_objects, false);
  EXPECT_EQ (stream_opt.get_options<db::GDS2ReaderOptions> ().box_mode, (unsigned int) 3);
  EXPECT_EQ (stream_opt.get_options<db::GDS2ReaderOptions> ().allow_big_records, false);
  EXPECT_EQ (stream_opt.get_options<db::GDS2ReaderOptions> ().allow_multi_xy_records, false);
  EXPECT_EQ (stream_opt.get_options<db::OASISReaderOptions> ().expect_strict_mode, 1);
}

