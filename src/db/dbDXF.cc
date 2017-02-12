
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


#include "dbDXF.h"
#include "dbDXFReader.h"
#include "dbDXFWriter.h"
#include "dbStream.h"

#include "tlClassRegistry.h"

namespace db
{

// ---------------------------------------------------------------
//  DXFDiagnostics implementation

DXFDiagnostics::~DXFDiagnostics ()
{
  //  .. nothing yet ..
}

// ---------------------------------------------------------------
//  DXF format declaration

class DXFFormatDeclaration
  : public db::StreamFormatDeclaration
{
public:
  DXFFormatDeclaration ()
  {
    //  .. nothing yet ..
  }

  virtual std::string format_name () const { return "DXF"; }
  virtual std::string format_desc () const { return "DXF"; }
  virtual std::string format_title () const { return "DXF (AutoCAD)"; }
  virtual std::string file_format () const { return "DXF files (*.DXF *.dxf *.dxf.gz *.DXF.gz)"; }

  virtual bool detect (tl::InputStream &s) const 
  {
    std::string l;
    tl::Extractor ex;
    tl::TextInputStream stream (s);

    if (stream.at_end ()) {
      return false;
    }

    l = stream.get_line();

    if (l == "AutoCAD Binary DXF") {
      //  binary DXF file - no need to go further
      return true;
    }

    //  ASCII DXF: some lines with 999 plus comment may 
    //  appear, then next four lines must be "0", "SECTION", "2", "HEADER" ..
    ex = l.c_str ();

    while (ex.test ("999")) {
      stream.get_line();
      l = stream.get_line();
      ex = l.c_str ();
    }

    if (! ex.test("0") || ! ex.at_end ()) {
      return false;
    }

    if (stream.at_end ()) {
      return false;
    }

    l = stream.get_line();
    ex = l.c_str ();
    if (! ex.test("SECTION") || ! ex.at_end ()) {
      return false;
    }

    if (stream.at_end ()) {
      return false;
    }

    l = stream.get_line();
    ex = l.c_str ();
    if (! ex.test("2") || ! ex.at_end ()) {
      return false;
    }

    if (stream.at_end ()) {
      return false;
    }

    l = stream.get_line();
    ex = l.c_str ();
    if (! ex.test("HEADER") || ! ex.at_end ()) {
      return false;
    }

    return ! stream.at_end ();
  }

  virtual ReaderBase *create_reader (tl::InputStream &s) const 
  {
    return new db::DXFReader (s);
  }

  virtual WriterBase *create_writer () const
  {
    return new db::DXFWriter ();
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

static tl::RegisteredClass<db::StreamFormatDeclaration> reader_decl (new DXFFormatDeclaration (), 100, "DXF");

//  provide a symbol to force linking against
int force_link_DXF = 0;

}


