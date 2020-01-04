
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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



#include "dbCommonReader.h"
#include "dbStream.h"
#include "tlXMLParser.h"

namespace db
{

// ---------------------------------------------------------------
//  Common format declaration

/**
 *  @brief A declaration for the common reader options
 *  This is a dummy declaration that provides common specifications for both GDS and OASIS readers.
 */
class CommonFormatDeclaration
  : public db::StreamFormatDeclaration
{
public:
  CommonFormatDeclaration ()
  {
    //  .. nothing yet ..
  }

  virtual std::string format_name () const { return "Common"; }
  virtual std::string format_desc () const { return "GDS2+OASIS"; }
  virtual std::string format_title () const { return "Common GDS2+OASIS"; }
  virtual std::string file_format () const { return std::string (); }

  virtual bool detect (tl::InputStream & /*s*/) const
  {
    return false;
  }

  virtual ReaderBase *create_reader (tl::InputStream & /*s*/) const
  {
    return 0;
  }

  virtual WriterBase *create_writer () const
  {
    return 0;
  }

  virtual bool can_read () const
  {
    return false;
  }

  virtual bool can_write () const
  {
    return false;
  }

  virtual tl::XMLElementBase *xml_reader_options_element () const
  {
    return new db::ReaderOptionsXMLElement<db::CommonReaderOptions> ("common",
      tl::make_member (&db::CommonReaderOptions::create_other_layers, "create-other-layers") +
      tl::make_member (&db::CommonReaderOptions::layer_map, "layer-map") +
      tl::make_member (&db::CommonReaderOptions::enable_properties, "enable-properties") +
      tl::make_member (&db::CommonReaderOptions::enable_text_objects, "enable-text-objects")
    );
  }
};

static tl::RegisteredClass<db::StreamFormatDeclaration> reader_decl (new CommonFormatDeclaration (), 20, "Common");

}

