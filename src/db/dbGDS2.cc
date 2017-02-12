
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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
  virtual std::string file_format () const { return "GDS2 files (*.GDS *.gds *.gds.gz *.GDS.gz *.GDS2 *.gds2 *.gds2.gz *.GDS2.gz)"; }

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
};

static tl::RegisteredClass<db::StreamFormatDeclaration> format_decl (new GDS2FormatDeclaration (), 0, "GDS2");

//  provide a symbol to force linking against
int force_link_GDS2 = 0;

}

