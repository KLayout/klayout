
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


#include <string>
#include <map>

#include "dbGDS2Converter.h"
#include "dbGDS2TextReader.h"
#include "dbGDS2TextWriter.h"
#include "dbGDS2.h"
#include "dbStream.h"

#include "tlClassRegistry.h"

namespace db
{

// ---------------------------------------------------------------
//  GDS2 Text format implementation

class GDS2TextFormatDeclaration
  : public db::StreamFormatDeclaration
{
  virtual std::string format_name () const { return "GDS2Text"; }
  virtual std::string format_desc () const { return "GDS2 Text"; }
  virtual std::string format_title () const { return "GDS2 (ASCII text representation)"; }
  virtual std::string file_format () const { return "GDS2 Text files (*.txt *.TXT )"; }

  virtual bool detect (tl::InputStream &s) const 
  {
    try {

      tl::TextInputStream stream (s);

      while (! stream.at_end ()) {

        std::string line = stream.get_line ();
        tl::Extractor ex (line.c_str ());
        if (ex.test ("#") || ex.at_end ()) {
          //  ignore comment or empty lines
        } else {
          return (ex.test ("HEADER") || ex.test ("BGNLIB") || ex.test ("UNITS"));
        } 

      }

    } catch (...) {
    }

    return false;
  }

  virtual ReaderBase *create_reader (tl::InputStream &s) const 
  {
    return new db::GDS2ReaderText(s);
  }

  virtual WriterBase *create_writer () const 
  {
    return new db::GDS2WriterText();
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

static tl::RegisteredClass<db::StreamFormatDeclaration> format_txt_decl (new GDS2TextFormatDeclaration(), 1, "GDS2Text");

}

