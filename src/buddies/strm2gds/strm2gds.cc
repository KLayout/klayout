
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

#include "dbLayout.h"
#include "dbReader.h"
#include "dbGDS2Writer.h"
#include "tlCommandLineParser.h"

int
main (int argc, char *argv [])
{
  db::SaveLayoutOptions save_options;
  db::GDS2WriterOptions gds2_options;
  std::string infile, outfile;

  tl::CommandLineOptions cmd;

  cmd << tl::arg ("-ov|--max-vertex-count=count", &gds2_options.max_vertex_count, "Specifies the maximum number of points per polygon",
                  "If this number is given, polygons are cut into smaller parts if they have more "
                  "than the specified number of points. If not given, the maximum number of points will be used. "
                  "This is 8190 unless --multi-xy-records is given."
                 )
      << tl::arg ("-om|--multi-xy-records",    &gds2_options.multi_xy_records, "Allows unlimited number of points",
                  "If this option is given, multiple XY records will be written to accomodate an unlimited number "
                  "of points per polygon or path. However, such files may not be compatible with some consumers."
                 )
      << tl::arg ("-oz|--no-zero-length-paths", &gds2_options.no_zero_length_paths, "Converts zero-length paths to polygons",
                  "If this option is given, zero-length paths (such with one point) are not written as paths "
                  "but converted to polygons. This avoids compatibility issues with consumers of this layout file."
                 )
      << tl::arg ("-on|--cellname-length=length", &gds2_options.max_cellname_length, "Limits cell names to the given length",
                  "If this option is given, long cell names will truncated if their length exceeds the given length."
                 )
      << tl::arg ("-ol|--libname=libname",     &gds2_options.libname, "Uses the given library name",
                  "This option can specify the GDS2 LIBNAME for the output file. By default, the original LIBNAME is "
                  "written."
                 )
      << tl::arg ("-om|--user-units=unit",     &gds2_options.user_units, "Specifies the user unit to use",
                  "Specifies the GDS2 user unit. By default micrometers are used for the user unit."
                 )
      << tl::arg ("!-ot|--no-timestamps",       &gds2_options.write_timestamps, "Don't write timestamps",
                  "Writes a dummy time stamp instead of the actual time. With this option, GDS2 files become "
                  "bytewise indentical even if written at different times. This option is useful if binary "
                  "identity is important (i.e. in regression scenarios)."
                 )
      << tl::arg ("-op|--write-cell-properties", &gds2_options.write_cell_properties, "Write cell properties",
                  "This option enables a GDS2 extension that allows writing of cell properties to GDS2 files. "
                  "Consumers that don't support this feature, may not be able to read such a GDS2 files."
                 )
      << tl::arg ("-oq|--write-file-properties", &gds2_options.write_file_properties, "Write file properties",
                  "This option enables a GDS2 extension that allows writing of file properties to GDS2 files. "
                  "Consumers that don't support this feature, may not be able to read such a GDS2 files."
                 )
      << tl::arg ("-os|--scale-factor=factor", &save_options,  &db::SaveLayoutOptions::set_scale_factor, "Scales the layout upon writing",
                  "Specifies layout scaling. If given, the saved layout will be scaled by the "
                  "given factor."
                 )
      << tl::arg ("-ou|--dbu=dbu",             &save_options,  &db::SaveLayoutOptions::set_dbu, "Uses the specified database unit",
                  "Specifies the database unit to save the layout in. The database unit is given "
                  "in micron units. By default, the original unit is used. The layout will not "
                  "change physically because internally, the coordinates are scaled to match the "
                  "new database unit."
                 )
      << tl::arg ("-ox|--drop-empty-cells",    &save_options,  &db::SaveLayoutOptions::set_dont_write_empty_cells, "Drops empty cells",
                  "If given, empty cells won't be written. See --keep-instances for more options."
                 )
      << tl::arg ("-ok|--keep-instances",      &save_options,  &db::SaveLayoutOptions::set_keep_instances, "Keeps instances of dropped cells",
                  "If given, instances of dropped cell's won't be removed. Hence, ghost cells are "
                  "produced. The resulting layout may not be readable by consumers that require "
                  "all instantiated cells to be present as actual cells."
                 )
      << tl::arg ("-oc|--write-context-info",  &save_options,  &db::SaveLayoutOptions::set_write_context_info, "Writes context information",
                  "Include context information for PCell instances and other information in a format-specific "
                  "way. The resulting layout may show unexpected features for other consumers."
                 )
      << tl::arg ("input",                     &infile,                        "The input file (any format, may be gzip compressed)")
      << tl::arg ("output",                    &outfile,                       "The output file")
    ;

  save_options.set_options (gds2_options);

  cmd.brief ("This program will convert the given file to a GDS2 file");

  try {

    cmd.parse (argc, argv);

    db::Manager m;
    db::Layout layout (&m);
    db::LayerMap map;

    {
      tl::InputStream stream (infile);
      db::Reader reader (stream);
      map = reader.read (layout);
    }

    {
      tl::OutputStream stream (outfile);
      db::GDS2Writer writer;
      writer.write (layout, stream, save_options);
    }

  } catch (tl::CancelException &ex) {
    return 1;
  } catch (std::exception &ex) {
    tl::error << ex.what ();
    return 1;
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    return 1;
  } catch (...) {
    tl::error << "ERROR: unspecific error";
  }

  return 0;
}


