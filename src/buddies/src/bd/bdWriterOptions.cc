
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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
#include "dbLayout.h"
#include "dbSaveLayoutOptions.h"
#include "tlCommandLineParser.h"
#include "tlGlobPattern.h"

namespace bd
{

GenericWriterOptions::GenericWriterOptions ()
{
  db::SaveLayoutOptions options;
  init_from_options (options);
}

GenericWriterOptions::GenericWriterOptions (const db::SaveLayoutOptions &options)
{
  init_from_options (options);
}

void
GenericWriterOptions::init_from_options (const db::SaveLayoutOptions &save_options_nc)
{
  //  const_cast needed because "get_option_by_name" is not const as it should be
  db::SaveLayoutOptions &save_options = const_cast<db::SaveLayoutOptions &> (save_options_nc);

  m_scale_factor = 1.0;

  m_dbu = save_options.get_option_by_name ("dbu").to_double ();
  m_libname = save_options.get_option_by_name ("libname").to_string ();

  m_dont_write_empty_cells = save_options.get_option_by_name ("no_empty_cells").to_bool ();
  m_keep_instances = save_options.get_option_by_name ("keep_instances").to_bool ();
  m_write_context_info = save_options.get_option_by_name ("write_context_info").to_bool ();

  m_gds2_max_vertex_count = save_options.get_option_by_name ("gds2_max_vertex_count").to_uint ();
  m_gds2_no_zero_length_paths = save_options.get_option_by_name ("gds2_no_zero_length_paths").to_bool ();
  m_gds2_multi_xy_records = save_options.get_option_by_name ("gds2_multi_xy_records").to_bool ();
  m_gds2_resolve_skew_arrays = save_options.get_option_by_name ("gds2_resolve_skew_arrays").to_bool ();
  m_gds2_max_cellname_length = save_options.get_option_by_name ("gds2_max_cellname_length").to_uint ();
  m_gds2_user_units = save_options.get_option_by_name ("gds2_user_units").to_double ();
  m_gds2_write_timestamps = save_options.get_option_by_name ("gds2_write_timestamps").to_bool ();
  m_gds2_write_cell_properties = save_options.get_option_by_name ("gds2_write_cell_properties").to_bool ();
  m_gds2_write_file_properties = save_options.get_option_by_name ("gds2_write_file_properties").to_bool ();
  tl::Variant def_text_size = save_options.get_option_by_name ("gds2_default_text_size");
  m_gds2_default_text_size = def_text_size.is_nil () ? -1.0 : def_text_size.to_double ();

  m_oasis_compression_level = save_options.get_option_by_name ("oasis_compression_level").to_int ();
  m_oasis_write_cblocks = save_options.get_option_by_name ("oasis_write_cblocks").to_bool ();
  m_oasis_strict_mode = save_options.get_option_by_name ("oasis_strict_mode").to_bool ();
  m_oasis_recompress = save_options.get_option_by_name ("oasis_recompress").to_bool ();
  m_oasis_permissive = save_options.get_option_by_name ("oasis_permissive").to_bool ();
  m_oasis_write_std_properties = save_options.get_option_by_name ("oasis_write_std_properties").to_int ();
  //  No substitution by default (issue #1885), so skip this:
  //  m_oasis_subst_char = save_options.get_option_by_name ("oasis_substitution_char").to_string ();

  m_cif_dummy_calls = save_options.get_option_by_name ("cif_dummy_calls").to_bool ();
  m_cif_blank_separator = save_options.get_option_by_name ("cif_blank_separator").to_bool ();

  //  The default options do not specify a lambda, but we prefer having a default here:
  //  m_magic_lambda = save_options.get_option_by_name ("mag_lambda").to_double ();
  m_magic_lambda = 1.0;

  m_dxf_polygon_mode = save_options.get_option_by_name ("dxf_polygon_mode").to_int ();

  m_lstream_compression_level = save_options.get_option_by_name ("lstream_compression_level").to_int ();
  m_lstream_recompress = save_options.get_option_by_name ("lstream_recompress").to_bool ();
  m_lstream_permissive = save_options.get_option_by_name ("lstream_permissive").to_bool ();
}

const std::string GenericWriterOptions::gds2_format_name      = "GDS2";
const std::string GenericWriterOptions::gds2text_format_name  = "GDS2Text";  //  no special options
const std::string GenericWriterOptions::oasis_format_name     = "OASIS";
const std::string GenericWriterOptions::lstream_format_name   = "LStream";
const std::string GenericWriterOptions::dxf_format_name       = "DXF";
const std::string GenericWriterOptions::cif_format_name       = "CIF";
const std::string GenericWriterOptions::mag_format_name       = "MAG";

void
GenericWriterOptions::add_options (tl::CommandLineOptions &cmd, const std::string &format)
{
  std::string group ("[Output options - General]");

  cmd << tl::arg (group +
                  "-os|--scale-factor=factor", &m_scale_factor, "Scales the layout upon writing",
                  "Specifies layout scaling. If given, the saved layout will be scaled by the "
                  "given factor."
                 );

  if (format.empty () || format == gds2_format_name || format == gds2text_format_name || format == oasis_format_name) {
    cmd << tl::arg (group +
                    "-od|--dbu-out=dbu",    &m_dbu, "Uses the specified database unit",
                    "Specifies the database unit to save the layout in. The database unit is given "
                    "in micron units. By default, the original unit is used. The layout will not "
                    "change physically because internally, the coordinates are scaled to match the "
                    "new database unit."
                   );
    cmd << tl::arg (group +
                    "-ol|--libname=libname", &m_libname, "Uses the given library name",
                    "This option can specify the LIBNAME for the output file. By default, the original LIBNAME is "
                    "written. This option is generic, but currently only supported by GDS2."
                   );
  }

  cmd << tl::arg (group +
                  "#--drop-empty-cells",    &m_dont_write_empty_cells, "Drops empty cells",
                  "If given, empty cells won't be written. See --keep-instances for more options."
                 );

  if (format.empty () || format == gds2_format_name || format == gds2text_format_name) {
    cmd << tl::arg (group +
                    "#--keep-instances",      &m_keep_instances, "Keeps instances of dropped cells",
                    "If given, instances of dropped cells won't be removed. Hence, ghost cells are "
                    "produced. The resulting layout may not be readable by consumers that require "
                    "all instantiated cells to be present as actual cells.\n"
                    "Dropped cells are those which are removed by a negative cell selection (see "
                    "--write-cells) "
                   );
  }

  if (format.empty () || format == gds2_format_name || format == gds2text_format_name || format == oasis_format_name) {
    cmd << tl::arg (group +
                    "!#--no-context-info",    &m_write_context_info, "Does not write context information",
                    "Context information is included to maintain PCell parameters and library connections. "
                    "This information is kept inside the layout files in a proprietary way. This option disables "
                    "this feature to maintain compatibility with other consumers of the file. If this option is "
                    "used, PCell parameters and library links are lost."
                   );
  }

  cmd << tl::arg (group +
                  "#--write-cells=sel",       &m_cell_selection, "Specifies cells to write",
                  "This option specifies the cells to write. The value of this option is a sequence of "
                  "positive and negative cell select operations. "
                  "A select operation is an optional plus (+) or minus sign (-), followed by "
                  "a cell filter. By default a select operation is positive, with a minus sign, the "
                  "select operation is negative and will unselect the matching cells."
                  "A cell filter is a plain cell name or a glob pattern (using '*' and '?' for placeholders). "
                  "If a cell filter is enclosed in round brackets, it will apply only to the matching cells. "
                  "Otherwise it will apply to these cells plus their children.\n"
                  "\n"
                  "Multiple operations can be specified by combining them with a comma. "
                  "Positive and negative selection happens in the order given. Hence it's possible "
                  "to select a cell with its children and then unselect some children of this cell.\n"
                  "\n"
                  "Examples:\n\n"
                  "* \"TOP1,TOP2\" - Select cells TOP1 and TOP2 with all of their children\n"
                  "* \"(TOP)\" - Select only cell TOP, but none of its child cells\n"
                  "* \"TOP,-A\" - Select cell TOP (plus children), then remove A (with children)"
                 );

  if (format.empty () || format == gds2_format_name || format == gds2text_format_name) {

    //  Add GDS2 and GDS2Text format options
    std::string group = "[Output options - GDS2 specific]";

    cmd << tl::arg (group +
                    "-ov|--max-vertex-count=count", &m_gds2_max_vertex_count, "Specifies the maximum number of points per polygon",
                    "If this number is given, polygons are cut into smaller parts if they have more "
                    "than the specified number of points. If not given, the maximum number of points will be used. "
                    "This is 8190 unless --multi-xy-records is given."
                   )
        << tl::arg (group +
                    "#--multi-xy-records", &m_gds2_multi_xy_records, "Allows unlimited number of points",
                    "If this option is given, multiple XY records will be written to accommodate an unlimited number "
                    "of points per polygon or path. However, such files may not be compatible with some consumers."
                   )
        << tl::arg (group +
                    "-ow|--resolve-skew-arrays", &m_gds2_resolve_skew_arrays, "Resolve skew (non-orthogonal) arrays",
                    "If this option is given, skew arrays are resolved into single instances. Skew arrays "
                    "are ones where the row or column vectors are not horizontal or vertical. Such arrays can cause problems "
                    "in legacy software. This option will eliminate them at the expense of bigger files and loss of the array instance property."
                   )
        << tl::arg (group +
                    "#--no-zero-length-paths", &m_gds2_no_zero_length_paths, "Converts zero-length paths to polygons",
                    "If this option is given, zero-length paths (such with one point) are not written as paths "
                    "but converted to polygons. This avoids compatibility issues with consumers of this layout file."
                   )
        << tl::arg (group +
                    "-on|--cellname-length=length", &m_gds2_max_cellname_length, "Limits cell names to the given length",
                    "If this option is given, long cell names will truncated if their length exceeds the given length."
                   )
        << tl::arg (group +
                    "#--user-units=unit", &m_gds2_user_units, "Specifies the user unit to use",
                    "Specifies the GDS2 user unit. By default micrometers are used for the user unit."
                   )
        << tl::arg (group +
                    "#!--no-timestamps", &m_gds2_write_timestamps, "Don't write timestamps",
                    "Writes a dummy time stamp instead of the actual time. With this option, GDS2 files become "
                    "bytewise identical even if written at different times. This option is useful if binary "
                    "identity is important (i.e. in regression scenarios)."
                   )
        << tl::arg (group +
                    "#--write-cell-properties", &m_gds2_write_cell_properties, "Write cell properties",
                    "This option enables a GDS2 extension that allows writing of cell properties to GDS2 files. "
                    "Consumers that don't support this feature, may not be able to read such a GDS2 files."
                   )
        << tl::arg (group +
                    "#--write-file-properties", &m_gds2_write_file_properties, "Write file properties",
                    "This option enables a GDS2 extension that allows writing of file properties to GDS2 files. "
                    "Consumers that don't support this feature, may not be able to read such a GDS2 files."
                   )
        << tl::arg (group +
                    "#--default-text-size", &m_gds2_default_text_size, "Default text size",
                    "This text size (given in micrometers) is applied to text objects not coming with their "
                    "own text size (technically: with a zero text size). It can be set to 0 to preserve an original "
                    "text size of zero. This option is also handy to give text objects from OASIS files a "
                    "specific size. By default, text objects without a size (i.e. with a zero size) do not receive one."
                   )
      ;

  }

  if (format.empty () || format == oasis_format_name) {

    //  Add OASIS format options
    std::string group = "[Output options - OASIS specific]";

    cmd << tl::arg (group +
                    "-ok|--compression-level=level", &m_oasis_compression_level, "Specifies the OASIS compression level",
                    "This level describes how hard the OASIS writer will try to compress the shapes "
                    "using shape arrays. Building shape arrays may take some time and requires some memory. "
                    "The default compression level is 2.\n"
                    "* 0 - no shape array building\n"
                    "* 1 - nearest neighbor shape array formation\n"
                    "* 2++ - enhanced shape array search algorithm using 2nd and further neighbor distances as well\n"
                   )
        << tl::arg (group +
                    "-ob|--cblocks", &m_oasis_write_cblocks, "Uses CBLOCK compression",
                    "Please note that since version 0.27.12, CBLOCK compression is enabled by default. If you do not want "
                    "CBLOCK compression, use '--cblocks=false'."
                   )
        << tl::arg (group +
                    "-ot|--strict-mode", &m_oasis_strict_mode, "Uses strict mode",
                    "Please note that since version 0.27.12, strict mode is enabled by default. If you do not want "
                    "strict mode, use '--strict-mode=false'."
                   )
        << tl::arg (group +
                    "#--recompress", &m_oasis_recompress, "Compresses shape arrays again",
                    "With this option, shape arrays will be expanded and recompressed. This may result in a better "
                    "compression ratio, but at the cost of slower execution."
                   )
        << tl::arg (group +
                    "#--permissive", &m_oasis_permissive, "Permissive mode",
                    "In permissive mode, certain forbidden objects are reported as warnings, not as errors: "
                    "paths with odd width, polygons with less than three points etc."
                   )
        << tl::arg (group +
                    "#--write-std-properties", &m_oasis_write_std_properties, "Writes some global standard properties",
                    "This is an integer describing what standard properties shall be written. 0 is \"none\", "
                    "1 means \"global standard properties such as S_TOP_CELL\" are produced (the default). With 2 also per-cell bounding "
                    "boxes are produced."
                   )
        << tl::arg (group +
                    "#--subst-char=char", this, &GenericWriterOptions::set_oasis_substitution_char, "Specifies the substitution character for non-standard characters",
                    "The first character of the string specified with this option will be used in placed of illegal "
                    "characters in n-strings and a-strings."
                   )
      ;

  }

  if (format.empty () || format == lstream_format_name) {

    //  Add LStream format options
    std::string group = "[Output options - LStream specific]";

    cmd << tl::arg (group +
                    "-oc|--lstr-compression-level=level", &m_lstream_compression_level, "Specifies the LStream compression level",
                    "This level describes how hard the LStream writer will try to compress the shapes "
                    "using shape arrays. Building shape arrays may take some time and requires some memory. "
                    "The default compression level is 2.\n"
                    "* 0 - no shape array building\n"
                    "* 1 - nearest neighbor shape array formation\n"
                    "* 2++ - enhanced shape array search algorithm using 2nd and further neighbor distances as well\n"
                   )
        << tl::arg (group +
                    "#--lstr-recompress", &m_lstream_recompress, "Compresses shape arrays again",
                    "With this option, shape arrays will be expanded and recompressed. This may result in a better "
                    "compression ratio, but at the cost of slower execution."
                   )
        << tl::arg (group +
                    "#--lstr-permissive", &m_lstream_permissive, "Permissive mode",
                    "In permissive mode, certain forbidden objects are reported as warnings, not as errors: "
                    "paths with odd width, polygons with less than three points etc."
                   )
      ;

  }

  if (format.empty () || format == dxf_format_name) {

    //  Add DXF format options
    std::string group = "[Output options - DXF specific]";

    cmd << tl::arg (group +
                    "-op|--polygon-mode=mode", &m_dxf_polygon_mode, "Specifies how to write polygons",
                    "This option specifies how to write polygons:\n"
                    "* 0: create POLYLINE (default)\n"
                    "* 1: create LWPOLYLINE\n"
                    "* 2: decompose into SOLID\n"
                    "* 3: create HATCH\n"
                    "* 4: create LINE"
                   )
      ;

  }

  if (format.empty () || format == cif_format_name) {

    //  Add CIF format options
    std::string group = "[Output options - CIF specific]";

    cmd << tl::arg (group +
                    "#--dummy-calls",         &m_cif_dummy_calls,       "Produces dummy calls",
                    "If this option is given, the writer will produce dummy cell calls on global level for all top cells"
                   )
        << tl::arg (group +
                    "#--blank-separator",     &m_cif_blank_separator,   "Uses blanks as x/y separators",
                    "If this option is given, blank characters will be used to separate x and y values. "
                    "Otherwise comma characters will be used.\n"
                    "Use this option if your CIF consumer cannot read comma characters as x/y separators."
                   )
      ;

  }

  if (format.empty () || format == mag_format_name) {

    //  Add MAG format options
    std::string group = "[Output options - MAG (Magic) specific]";

    cmd << tl::arg (group +
                    "--magic-lambda-out=lambda", &m_magic_lambda, "Specifies the lambda value when writing Magic files (which are unitless)"
                   )
        << tl::arg (group +
                    "--magic-tech",           &m_magic_tech, "Specifies the technology to include in the Magic files"
                   )
      ;

  }
}

void GenericWriterOptions::set_oasis_substitution_char (const std::string &text)
{
  if (! text.empty ()) {
    m_oasis_subst_char = text[0];
  } else {
    m_oasis_subst_char = std::string ();
  }
}

static void get_selected_cells (tl::Extractor &ex, const db::Layout &layout, std::set<db::cell_index_type> &selected)
{
  while (! ex.at_end ()) {

    bool remove = ex.test ("-");
    ex.test ("+");

    bool without_children = ex.test ("(");
    std::string filter;
    ex.read_word_or_quoted (filter, "_-.*?{}$[]");
    if (without_children) {
      ex.expect (")");
    }

    if (! ex.at_end ()) {
      ex.expect (",");
    }

    tl::GlobPattern pat (filter);
    for (db::Layout::const_iterator c = layout.begin (); c != layout.end (); ++c) {

      if (pat.match (layout.cell_name (c->cell_index ()))) {

        std::set<db::cell_index_type> cells;
        if (! without_children) {
          c->collect_called_cells (cells);
        }
        cells.insert (c->cell_index ());

        if (! remove) {
          selected.insert (cells.begin (), cells.end ());
        } else {
          for (std::set<db::cell_index_type>::const_iterator c = cells.begin (); c != cells.end (); ++c) {
            selected.erase (*c);
          }
        }

      }

    }

  }
}

void
GenericWriterOptions::configure (db::SaveLayoutOptions &save_options, const db::Layout &layout) const
{
  save_options.set_scale_factor (m_scale_factor);
  save_options.set_dbu (m_dbu);
  save_options.set_libname (m_libname);
  save_options.set_dont_write_empty_cells (m_dont_write_empty_cells);
  save_options.set_keep_instances (m_keep_instances);
  save_options.set_write_context_info (m_write_context_info);

  save_options.set_option_by_name ("gds2_max_vertex_count", m_gds2_max_vertex_count);
  save_options.set_option_by_name ("gds2_no_zero_length_paths", m_gds2_no_zero_length_paths);
  save_options.set_option_by_name ("gds2_multi_xy_records", m_gds2_multi_xy_records);
  save_options.set_option_by_name ("gds2_resolve_skew_arrays", m_gds2_resolve_skew_arrays);
  save_options.set_option_by_name ("gds2_max_cellname_length", m_gds2_max_cellname_length);
  save_options.set_option_by_name ("gds2_user_units", m_gds2_user_units);
  save_options.set_option_by_name ("gds2_write_timestamps", m_gds2_write_timestamps);
  save_options.set_option_by_name ("gds2_write_cell_properties", m_gds2_write_cell_properties);
  save_options.set_option_by_name ("gds2_write_file_properties", m_gds2_write_file_properties);
  save_options.set_option_by_name ("gds2_default_text_size", m_gds2_default_text_size < 0.0 ? tl::Variant () : tl::Variant (m_gds2_default_text_size));

  save_options.set_option_by_name ("oasis_compression_level", m_oasis_compression_level);
  save_options.set_option_by_name ("oasis_write_cblocks", m_oasis_write_cblocks);
  save_options.set_option_by_name ("oasis_strict_mode", m_oasis_strict_mode);
  save_options.set_option_by_name ("oasis_recompress", m_oasis_recompress);
  save_options.set_option_by_name ("oasis_permissive", m_oasis_permissive);
  //  Note: "..._ext" is a version taking the real value (not just a boolean)
  save_options.set_option_by_name ("oasis_write_std_properties_ext", m_oasis_write_std_properties);
  save_options.set_option_by_name ("oasis_substitution_char", m_oasis_subst_char);

  save_options.set_option_by_name ("cif_dummy_calls", m_cif_dummy_calls);
  save_options.set_option_by_name ("cif_blank_separator", m_cif_blank_separator);

  save_options.set_option_by_name ("dxf_polygon_mode", m_dxf_polygon_mode);

  save_options.set_option_by_name ("lstream_compression_level", m_lstream_compression_level);
  save_options.set_option_by_name ("lstream_recompress", m_lstream_recompress);
  save_options.set_option_by_name ("lstream_permissive", m_lstream_permissive);

  save_options.set_option_by_name ("mag_lambda", m_magic_lambda);
  save_options.set_option_by_name ("mag_tech", m_magic_tech);

  if (!m_cell_selection.empty ()) {

    std::set<db::cell_index_type> selected;
    tl::Extractor ex (m_cell_selection.c_str ());
    get_selected_cells (ex, layout, selected);

    save_options.clear_cells ();
    for (std::set<db::cell_index_type>::const_iterator s = selected.begin (); s != selected.end (); ++s) {
      save_options.add_this_cell (*s);
    }

  }
}

}
