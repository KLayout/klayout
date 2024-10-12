
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

#include "bdWriterOptions.h"
#include "bdReaderOptions.h"
#include "tlCommandLineParser.h"
#include "tlUnitTest.h"
#include "dbLayout.h"
#include "dbCell.h"
#include "dbSaveLayoutOptions.h"

//  Testing writer options
TEST(1)
{
  bd::GenericWriterOptions opt;
  tl::CommandLineOptions cmd;

  opt.add_options (cmd);

  const char *argv[] = {
                   "x",
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
                   "-ob=false",
                   "-ok=9",
                   "-ot=false",
                   "--recompress",
                   "--subst-char=XY",
                   "--write-std-properties=2"
                 };

  cmd.parse (sizeof (argv) / sizeof (argv[0]), const_cast<char **> (argv));

  db::Layout layout;

  db::SaveLayoutOptions stream_opt;
  EXPECT_EQ (stream_opt.dont_write_empty_cells (), false);
  EXPECT_EQ (stream_opt.keep_instances (), false);
  EXPECT_EQ (stream_opt.write_context_info (), true);
  EXPECT_EQ (stream_opt.get_option_by_name ("cif_blank_separator").to_bool (), false);
  EXPECT_EQ (stream_opt.get_option_by_name ("cif_dummy_calls").to_bool (), false);
  EXPECT_EQ (stream_opt.get_option_by_name ("dxf_polygon_mode").to_int (), 0);
  EXPECT_EQ (stream_opt.get_option_by_name ("gds2_libname").to_string (), "LIB");
  EXPECT_EQ (stream_opt.get_option_by_name ("gds2_max_vertex_count").to_uint (), (unsigned int) 8000);
  EXPECT_EQ (stream_opt.get_option_by_name ("gds2_multi_xy_records").to_bool (), false);
  EXPECT_EQ (stream_opt.get_option_by_name ("gds2_write_timestamps").to_bool (), true);
  EXPECT_EQ (stream_opt.get_option_by_name ("gds2_no_zero_length_paths").to_bool (), false);
  EXPECT_EQ (tl::to_string (stream_opt.get_option_by_name ("gds2_user_units").to_double ()), "1");
  EXPECT_EQ (stream_opt.get_option_by_name ("gds2_write_cell_properties").to_bool (), false);
  EXPECT_EQ (stream_opt.get_option_by_name ("gds2_write_file_properties").to_bool (), false);
  EXPECT_EQ (stream_opt.get_option_by_name ("oasis_write_cblocks").to_bool (), true);
  EXPECT_EQ (stream_opt.get_option_by_name ("oasis_compression_level").to_int (), 2);
  EXPECT_EQ (stream_opt.get_option_by_name ("oasis_strict_mode").to_bool (), true);
  EXPECT_EQ (stream_opt.get_option_by_name ("oasis_recompress").to_bool (), false);
  EXPECT_EQ (stream_opt.get_option_by_name ("oasis_substitution_char").to_string (), "*");
  EXPECT_EQ (stream_opt.get_option_by_name ("oasis_write_std_properties_ext").to_int (), 1);

  opt.configure (stream_opt, layout);

  EXPECT_EQ (stream_opt.scale_factor (), 1.25);
  EXPECT_EQ (stream_opt.dbu (), 0.125);
  EXPECT_EQ (stream_opt.dont_write_empty_cells (), true);
  EXPECT_EQ (stream_opt.keep_instances (), true);
  EXPECT_EQ (stream_opt.write_context_info (), false);
  EXPECT_EQ (stream_opt.get_option_by_name ("cif_blank_separator").to_bool (), true);
  EXPECT_EQ (stream_opt.get_option_by_name ("cif_dummy_calls").to_bool (), true);
  EXPECT_EQ (stream_opt.get_option_by_name ("dxf_polygon_mode").to_int (), 2);
  EXPECT_EQ (stream_opt.get_option_by_name ("gds2_libname").to_string (), "MYLIBNAME");
  EXPECT_EQ (stream_opt.get_option_by_name ("gds2_max_vertex_count").to_uint (), (unsigned int) 250);
  EXPECT_EQ (stream_opt.get_option_by_name ("gds2_multi_xy_records").to_bool (), true);
  EXPECT_EQ (stream_opt.get_option_by_name ("gds2_write_timestamps").to_bool (), false);
  EXPECT_EQ (stream_opt.get_option_by_name ("gds2_no_zero_length_paths").to_bool (), true);
  EXPECT_EQ (tl::to_string (stream_opt.get_option_by_name ("gds2_user_units").to_double ()), "2.5");
  EXPECT_EQ (stream_opt.get_option_by_name ("gds2_write_cell_properties").to_bool (), true);
  EXPECT_EQ (stream_opt.get_option_by_name ("gds2_write_file_properties").to_bool (), true);
  EXPECT_EQ (stream_opt.get_option_by_name ("oasis_write_cblocks").to_bool (), false);
  EXPECT_EQ (stream_opt.get_option_by_name ("oasis_compression_level").to_int (), 9);
  EXPECT_EQ (stream_opt.get_option_by_name ("oasis_strict_mode").to_bool (), false);
  EXPECT_EQ (stream_opt.get_option_by_name ("oasis_recompress").to_bool (), true);
  EXPECT_EQ (stream_opt.get_option_by_name ("oasis_substitution_char").to_string (), "X");
  EXPECT_EQ (stream_opt.get_option_by_name ("oasis_write_std_properties_ext").to_int (), 2);
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
                         "--blend-mode=1",
                         //  OASIS
                         "--expect-strict-mode=1"
                       };

  cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);

  db::LoadLayoutOptions stream_opt;
  EXPECT_EQ (tl::to_string (stream_opt.get_option_by_name ("cif_dbu").to_double ()), "0.001");
  EXPECT_EQ (stream_opt.get_option_by_name ("cif_wire_mode").to_uint (), (unsigned int) 0);
  EXPECT_EQ (stream_opt.get_option_by_name ("cif_layer_map").to_user<db::LayerMap> ().to_string (), "layer_map()");
  EXPECT_EQ (stream_opt.get_option_by_name ("cif_create_other_layers").to_bool (), true);
  EXPECT_EQ (tl::to_string (stream_opt.get_option_by_name ("dxf_dbu").to_double ()), "0.001");
  EXPECT_EQ (stream_opt.get_option_by_name ("dxf_layer_map").to_user<db::LayerMap> ().to_string (), "layer_map()");
  EXPECT_EQ (stream_opt.get_option_by_name ("dxf_create_other_layers").to_bool (), true);
  EXPECT_EQ (stream_opt.get_option_by_name ("dxf_unit").to_double (), 1.0);
  EXPECT_EQ (tl::to_string (stream_opt.get_option_by_name ("dxf_circle_accuracy").to_double ()), "0");
  EXPECT_EQ (stream_opt.get_option_by_name ("dxf_circle_points").to_int (), 100);
  EXPECT_EQ (stream_opt.get_option_by_name ("dxf_keep_other_cells").to_bool (), false);
  EXPECT_EQ (stream_opt.get_option_by_name ("dxf_polyline_mode").to_int (), 0);
  EXPECT_EQ (stream_opt.get_option_by_name ("dxf_render_texts_as_polygons").to_bool (), false);
  EXPECT_EQ (stream_opt.get_option_by_name ("dxf_text_scaling").to_int (), 100);
  EXPECT_EQ (stream_opt.get_option_by_name ("layer_map").to_user<db::LayerMap> ().to_string (), "layer_map()");
  EXPECT_EQ (stream_opt.get_option_by_name ("create_other_layers").to_bool (), true);
  EXPECT_EQ (stream_opt.get_option_by_name ("cell_conflict_resolution").to_string (), "AddToCell");
  EXPECT_EQ (stream_opt.get_option_by_name ("properties_enabled").to_bool (), true);
  EXPECT_EQ (stream_opt.get_option_by_name ("text_enabled").to_bool (), true);
  EXPECT_EQ (stream_opt.get_option_by_name ("gds2_box_mode").to_uint (), (unsigned int) 1);
  EXPECT_EQ (stream_opt.get_option_by_name ("gds2_allow_big_records").to_bool (), true);
  EXPECT_EQ (stream_opt.get_option_by_name ("gds2_allow_multi_xy_records").to_bool (), true);
  EXPECT_EQ (stream_opt.get_option_by_name ("oasis_expect_strict_mode").to_int (), -1);

  opt.configure (stream_opt);

  EXPECT_EQ (tl::to_string (stream_opt.get_option_by_name ("cif_dbu").to_double ()), "0.125");
  EXPECT_EQ (stream_opt.get_option_by_name ("cif_wire_mode").to_uint (), (unsigned int) 1);
  EXPECT_EQ (stream_opt.get_option_by_name ("cif_layer_map").to_user<db::LayerMap> ().to_string (), "layer_map('1/0';'3-4/0-255';'A : 17/0')");
  EXPECT_EQ (stream_opt.get_option_by_name ("cif_create_other_layers").to_bool (), false);
  EXPECT_EQ (tl::to_string (stream_opt.get_option_by_name ("dxf_dbu").to_double ()), "0.125");
  EXPECT_EQ (stream_opt.get_option_by_name ("dxf_layer_map").to_user<db::LayerMap> ().to_string (), "layer_map('1/0';'3-4/0-255';'A : 17/0')");
  EXPECT_EQ (stream_opt.get_option_by_name ("dxf_create_other_layers").to_bool (), false);
  EXPECT_EQ (stream_opt.get_option_by_name ("dxf_unit").to_double (), 2.5);
  EXPECT_EQ (tl::to_string (stream_opt.get_option_by_name ("dxf_circle_accuracy").to_double ()), "0.5");
  EXPECT_EQ (stream_opt.get_option_by_name ("dxf_circle_points").to_int (), 1000);
  EXPECT_EQ (stream_opt.get_option_by_name ("dxf_keep_other_cells").to_bool (), true);
  EXPECT_EQ (stream_opt.get_option_by_name ("dxf_polyline_mode").to_int (), 3);
  EXPECT_EQ (stream_opt.get_option_by_name ("dxf_render_texts_as_polygons").to_bool (), true);
  EXPECT_EQ (stream_opt.get_option_by_name ("dxf_text_scaling").to_int (), 75);
  EXPECT_EQ (stream_opt.get_option_by_name ("layer_map").to_user<db::LayerMap> ().to_string (), "layer_map('1/0';'3-4/0-255';'A : 17/0')");
  EXPECT_EQ (stream_opt.get_option_by_name ("create_other_layers").to_bool (), false);
  EXPECT_EQ (stream_opt.get_option_by_name ("cell_conflict_resolution").to_string (), "OverwriteCell");
  EXPECT_EQ (stream_opt.get_option_by_name ("properties_enabled").to_bool (), false);
  EXPECT_EQ (stream_opt.get_option_by_name ("text_enabled").to_bool (), false);
  EXPECT_EQ (stream_opt.get_option_by_name ("gds2_box_mode").to_uint (), (unsigned int) 3);
  EXPECT_EQ (stream_opt.get_option_by_name ("gds2_allow_big_records").to_bool (), false);
  EXPECT_EQ (stream_opt.get_option_by_name ("gds2_allow_multi_xy_records").to_bool (), false);
  EXPECT_EQ (stream_opt.get_option_by_name ("oasis_expect_strict_mode").to_int (), 1);
}


//  Testing reader options - blend mode "Rename" is default
TEST(11)
{
  bd::GenericReaderOptions opt;
  tl::CommandLineOptions cmd;

  opt.add_options (cmd);

  const char *argv[] = { "x" };

  cmd.parse (sizeof (argv) / sizeof (argv[0]), (char **) argv);

  db::LoadLayoutOptions stream_opt;
  opt.configure (stream_opt);

  EXPECT_EQ (stream_opt.get_option_by_name ("cell_conflict_resolution").to_string (), "RenameCell");
}

//  Testing writer options
TEST(12_issue1885)
{
  bd::GenericWriterOptions opt;
  tl::CommandLineOptions cmd;

  opt.add_options (cmd);

  db::Layout layout;

  db::SaveLayoutOptions stream_opt;
  opt.configure (stream_opt, layout);

  EXPECT_EQ (stream_opt.get_option_by_name ("oasis_substitution_char").to_string (), "");

  const char *argv[] = {
                   "x",
                   "--subst-char=x",
                 };

  cmd.parse (sizeof (argv) / sizeof (argv[0]), const_cast<char **> (argv));

  opt.configure (stream_opt, layout);

  EXPECT_EQ (stream_opt.get_option_by_name ("oasis_substitution_char").to_string (), "x");

  const char *argv2[] = {
                   "x",
                   "--subst-char=",
                 };

  cmd.parse (sizeof (argv2) / sizeof (argv2[0]), const_cast<char **> (argv2));

  opt.configure (stream_opt, layout);

  EXPECT_EQ (stream_opt.get_option_by_name ("oasis_substitution_char").to_string (), "");
}

