
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

#include "bdReaderOptions.h"
#include "dbLoadLayoutOptions.h"
#include "tlCommandLineParser.h"

namespace bd
{

GenericReaderOptions::GenericReaderOptions ()
  : m_prefix ("i"), m_group_prefix ("Input"), m_create_other_layers (true)
{
  //  .. nothing yet ..
}

void
GenericReaderOptions::add_options (tl::CommandLineOptions &cmd)
{
  {
    std::string group ("[" + m_group_prefix + " options - General]");

    cmd << tl::arg (group +
                    "!-" + m_prefix + "s|--" + m_long_prefix + "skip-unknown-layers", &m_create_other_layers, "Skips unknown layers",
                    "This option is effective with the the --layer-map option. If combined with "
                    "--skip-unknown-layers, layers not listed in the layer map will not be read. "
                    "By default, corresponding entries are created also for unknown layers."
                   )
        << tl::arg (group +
                    "-" + m_prefix + "m|--" + m_long_prefix + "layer-map=map", this, &GenericReaderOptions::set_layer_map, "Specifies the layer mapping for the input",
                    "This option specifies a layer selection or mapping. The selection or mapping is a sequence of source and optional "
                    "target specifications. The specifications are separated by blanks or double-slash sequences (//).\n"
                    "\n"
                    "A source specification can apply to a single or many source layers. If many source layers are "
                    "selected, they are combined into a single target layer. A source specification is:\n"
                    "\n"
                    "* A list of source specs, separated by semicolon characters (;)\n"
                    "* A layer name (in double or single quotes if necessary)\n"
                    "* A layer/datatype pair or range separated with a slash\n"
                    "* Layer and datatype can be simple positive integer numbers\n"
                    "* Layer and datatype numbers can be enumerated (numbers separated with a comma)\n"
                    "* Layer and datatype numbers can be ranges formed with a dash separator\n"
                    "\n"
                    "Target specifications are added to source specifications with a colon (:). If a target "
                    "layer is specified, all source layers addressed with the source specification are "
                    "combined into this target layer.\n"
                    "\n"
                    "Examples:\n"
                    "\n"
                    "* 1/0 2/0 3/0-255:17/0\n"
                    "  Selects 1/0, 2/0 and maps layer 3, datatype 0 to 255 to layer 17, datatype 0\n"
                    "\n"
                    "* A:1/0 B:2/0\n"
                    "  Maps named layer A to 1/0 and named layer B to 2/0"
                   )
      ;
  }

  {
    std::string group ("[" + m_group_prefix + " options - GDS2 and OASIS specific]");

    cmd << tl::arg (group +
                    "#!--" + m_long_prefix + "disable-texts", &m_common_reader_options.enable_text_objects, "Skips text objects",
                    "With this option set, text objects won't be read."
                   )
        << tl::arg (group +
                    "#!--" + m_long_prefix + "disable-properties", &m_common_reader_options.enable_properties, "Skips properties",
                    "With this option set, properties won't be read."
                   )
      ;
  }

  {
    std::string group ("[" + m_group_prefix + " options - GDS2 specific]");

    cmd << tl::arg (group +
                    "#!--" + m_long_prefix + "no-multi-xy-records", &m_gds2_reader_options.allow_multi_xy_records, "Gives an error on multi-XY records",
                    "This option disables an advanced interpretation of GDS2 which allows unlimited polygon and path "
                    "complexity. For compatibility with other readers, this option restores the standard behavior and "
                    "disables this feature."
                   )
        << tl::arg (group +
                    "#!--" + m_long_prefix + "no-big-records", &m_gds2_reader_options.allow_big_records, "Gives an error on big (>32767 bytes) records",
                    "The GDS2 specification claims the record length to be a signed 16 bit value. So a record "
                    "can be 32767 bytes max. To allow bigger records (i.e. bigger polygons), the usual approach "
                    "is to take the length as a unsigned 16 bit value, so the length is up to 65535 bytes. "
                    "This option restores the original behavior and reports big (>32767 bytes) records are errors."
                   )
        << tl::arg (group +
                    "-" + m_prefix + "b|--" + m_long_prefix + "box-mode=mode", &m_gds2_reader_options.box_mode, "Specifies how BOX records are read",
                    "This an option provided for compatibility with other readers. The mode value specifies how "
                    "BOX records are read:\n"
                    "\n"
                    "* 0: ignore BOX records\n"
                    "* 1: treat as rectangles (the default)\n"
                    "* 2: treat as boundaries\n"
                    "* 3: treat as errors"
                   )
      ;
  }

  {
    std::string group ("[" + m_group_prefix + " options - OASIS specific]");

    cmd << tl::arg (group +
                    "#--" + m_long_prefix + "expect-strict-mode=mode", &m_oasis_reader_options.expect_strict_mode, "Makes the reader expect strict or non-strict mode",
                    "With this option, the OASIS reader will expect strict mode (mode is 1) or expect non-strict mode "
                    "(mode is 0). By default, both modes are allowed. This is a diagnostic feature and does not "
                    "have any other effect than checking the mode."
                   )
      ;
  }

  {
    std::string group ("[" + m_group_prefix + " options - CIF and DXF specific]");

    cmd << tl::arg (group +
                    "-" + m_prefix + "d|--" + m_long_prefix + "dbu-in=dbu", this, &GenericReaderOptions::set_dbu, "Specifies the database unit to use",
                    "This option specifies the database unit the resulting layer will have. "
                    "The value is given in micrometer units. The default value is 1nm (0.001)."
                   )
      ;
  }

  {
    std::string group ("[" + m_group_prefix + " options - CIF specific]");

    cmd << tl::arg (group +
                    "-" + m_prefix + "w|--" + m_long_prefix + "wire-mode=mode", &m_cif_reader_options.wire_mode, "Specifies how wires (W) are read",
                    "This option specifies how wire objects (W) are read:\n"
                    "\n"
                    "* 0: as square ended paths (the default)\n"
                    "* 1: as flush ended paths\n"
                    "* 2: as round paths"
                   )
      ;
  }

  {
    std::string group ("[" + m_group_prefix + " options - DXF specific]");

    cmd << tl::arg (group +
                    "-" + m_prefix + "u|--" + m_long_prefix + "dxf-unit=unit", &m_dxf_reader_options.unit, "Specifies the DXF drawing units",
                    "Since DXF is unitless, this value needs to be given to specify the drawing units. "
                    "By default, a drawing unit of micrometers is assumed."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "dxf-text-scaling=factor", &m_dxf_reader_options.text_scaling, "Specifies text scaling",
                    "This value specifies text scaling in percent. A value of 100 roughly means that the letter "
                    "pitch of the font will be 92% of the specified text height. That value applies for ROMANS fonts. "
                    "When generating GDS texts, a value of 100 generates TEXT objects with "
                    "the specified size. Smaller values generate smaller sizes."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "dxf-polyline-mode=mode", &m_dxf_reader_options.polyline_mode, "Specifies how POLYLINE records are handled",
                    "This value specifies how POLYLINE records are handled:\n"
                    "\n"
                    "* 0: automatic mode\n"
                    "* 1: keep lines\n"
                    "* 2: create polygons from closed POLYLINE/LWPOLYLINE with width == 0\n"
                    "* 3: merge all lines (width width 0)\n"
                    "* 4: as 3 and auto-close contours"
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "dxf-circle-points=points", &m_dxf_reader_options.circle_points, "Specifies the number of points for a full circle for arc interpolation",
                    "See --" + m_long_prefix + "dxf-circle-accuracy for another way of specifying the number of points per circle."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "dxf-circle-accuracy=value", &m_dxf_reader_options.circle_accuracy, "Specifies the accuracy of circle approximation",
                    "This value specifies the approximation accuracy of the circle and other\n"
                    "\"round\" structures. If this value is a positive number bigger than the\n"
                    "database unit (see dbu), it will control the number of points the\n"
                    "circle is resolved into. The number of points will be chosen such that\n"
                    "the deviation from the ideal curve is less than this value.\n"
                    "\n"
                    "The actual number of points used for the circle approximation is\n"
                    "not larger than circle_points.\n"
                    "\n"
                    "The value is given in the units of the DXF file."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "dxf-render-texts-as-polygons", &m_dxf_reader_options.render_texts_as_polygons, "Renders texts as polygons",
                    "If this option is used, texts are converted to polygons instead of being converted to labels."
                   )
        << tl::arg (group +
                    "#--" + m_long_prefix + "dxf-keep-other-cells", &m_dxf_reader_options.keep_other_cells, "Keeps cells which are not instantiated by the top cell",
                    "With this option, all cells not found to be instantiated are kept as additional top cells. "
                    "By default, such cells are removed."
                   )
      ;
  }
}

void GenericReaderOptions::set_layer_map (const std::string &lm)
{
  tl::Extractor ex (lm.c_str ());

  int l = 0;
  while (! ex.at_end ()) {
    m_layer_map.map_expr (ex, l);
    ex.test ("//");
    ++l;
  }
}

void GenericReaderOptions::set_dbu (double dbu)
{
  m_dxf_reader_options.dbu = dbu;
  m_cif_reader_options.dbu = dbu;
}

void
GenericReaderOptions::configure (db::LoadLayoutOptions &load_options) const
{
  db::CommonReaderOptions common_reader_options = m_common_reader_options;
  common_reader_options.layer_map = m_layer_map;
  common_reader_options.create_other_layers = m_create_other_layers;

  db::DXFReaderOptions dxf_reader_options = m_dxf_reader_options;
  dxf_reader_options.layer_map = m_layer_map;
  dxf_reader_options.create_other_layers = m_create_other_layers;

  db::CIFReaderOptions cif_reader_options = m_cif_reader_options;
  cif_reader_options.layer_map = m_layer_map;
  cif_reader_options.create_other_layers = m_create_other_layers;

  load_options.set_options (common_reader_options);
  load_options.set_options (m_gds2_reader_options);
  load_options.set_options (m_oasis_reader_options);
  load_options.set_options (cif_reader_options);
  load_options.set_options (dxf_reader_options);
}

}
