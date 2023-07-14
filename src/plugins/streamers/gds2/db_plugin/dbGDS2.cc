
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


#include "dbGDS2Reader.h"
#include "dbGDS2Writer.h"
#include "dbStream.h"
#include "tlClassRegistry.h"

namespace db
{

// ---------------------------------------------------------------
//  GDS2 format declaration

class GDS2FormatDeclaration
  : public db::StreamFormatDeclaration
{
  virtual std::string format_name () const { return "GDS2"; }
  virtual std::string format_desc () const { return "GDS2"; }
  virtual std::string format_title () const { return "GDS2"; }
  virtual std::string file_format () const { return "GDS2 files (*.gds *.GDS *.gds.gz *.GDS.gz *.GDS2 *.gds2 *.gds2.gz *.GDS2.gz)"; }

  virtual bool detect (tl::InputStream &stream) const 
  {
    const char *hdr = stream.get (4);
    return (hdr && hdr[0] == 0x00 && hdr[1] == 0x06 && hdr[2] == 0x00 && hdr[3] == 0x02);
  }

  virtual ReaderBase *create_reader (tl::InputStream &s) const 
  {
    return new db::GDS2Reader (s);
  }

  virtual WriterBase *create_writer () const 
  {
    return new db::GDS2Writer ();
  }

  virtual bool can_read () const
  {
    return true;
  }

  virtual bool can_write () const
  {
    return true;
  }

  virtual tl::XMLElementBase *xml_writer_options_element () const
  {
    return new db::WriterOptionsXMLElement<db::GDS2WriterOptions> ("gds2",
      tl::make_member (&db::GDS2WriterOptions::write_timestamps, "write-timestamps") +
      tl::make_member (&db::GDS2WriterOptions::write_cell_properties, "write-cell-properties") +
      tl::make_member (&db::GDS2WriterOptions::write_file_properties, "write-file-properties") +
      tl::make_member (&db::GDS2WriterOptions::no_zero_length_paths, "no-zero-length-paths") +
      tl::make_member (&db::GDS2WriterOptions::multi_xy_records, "multi-xy-records") +
      tl::make_member (&db::GDS2WriterOptions::resolve_skew_arrays, "resolve-skew-arrays") +
      tl::make_member (&db::GDS2WriterOptions::max_vertex_count, "max-vertex-count") +
      tl::make_member (&db::GDS2WriterOptions::max_cellname_length, "max-cellname-length") +
      tl::make_member (&db::GDS2WriterOptions::libname, "libname")
    );
  }

  virtual tl::XMLElementBase *xml_reader_options_element () const
  {
    return new db::ReaderOptionsXMLElement<db::GDS2ReaderOptions> ("gds2",
      tl::make_member (&db::GDS2ReaderOptions::box_mode, "box-mode") +
      tl::make_member (&db::GDS2ReaderOptions::allow_big_records, "allow-big-records") +
      tl::make_member (&db::GDS2ReaderOptions::allow_multi_xy_records, "allow-multi-xy-records")
    );
  }
};

static tl::RegisteredClass<db::StreamFormatDeclaration> format_decl (new GDS2FormatDeclaration (), 0, "GDS2");

}

