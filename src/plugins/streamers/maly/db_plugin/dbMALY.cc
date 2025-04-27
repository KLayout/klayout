
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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


#include "dbMALY.h"
#include "dbMALYReader.h"
#include "dbStream.h"

#include "tlClassRegistry.h"

namespace db
{

// ---------------------------------------------------------------
//  MALYDiagnostics implementation

MALYDiagnostics::~MALYDiagnostics ()
{
  //  .. nothing yet ..
}

// ---------------------------------------------------------------
//  MALY format declaration

class MALYFormatDeclaration
  : public db::StreamFormatDeclaration
{
public:
  MALYFormatDeclaration ()
  {
    //  .. nothing yet ..
  }

  virtual std::string format_name () const { return "MALY"; }
  virtual std::string format_desc () const { return "MALY jobdeck"; }
  virtual std::string format_title () const { return "MALY (MALY jobdeck format)"; }
  virtual std::string file_format () const { return "MALY jobdeck files (*.maly *.MALY)"; }

  virtual bool detect (tl::InputStream &s) const 
  {
    db::MALYReader reader (s);
    return reader.test ();
  }

  virtual ReaderBase *create_reader (tl::InputStream &s) const 
  {
    return new db::MALYReader (s);
  }

  virtual WriterBase *create_writer () const
  {
    return 0;
  }

  virtual bool can_read () const
  {
    return true;
  }

  virtual bool can_write () const
  {
    return false;
  }

  virtual tl::XMLElementBase *xml_reader_options_element () const
  {
    return new db::ReaderOptionsXMLElement<db::MALYReaderOptions> ("mag",
      tl::make_member (&db::MALYReaderOptions::dbu, "dbu") +
      tl::make_member (&db::MALYReaderOptions::layer_map, "layer-map") +
      tl::make_member (&db::MALYReaderOptions::create_other_layers, "create-other-layers")
    );
  }
};

//  NOTE: Because MALY has such a high degree of syntactic freedom, the detection is somewhat
//  fuzzy: do MALY at the very end of the detection chain
static tl::RegisteredClass<db::StreamFormatDeclaration> reader_decl (new MALYFormatDeclaration (), 2300, "MALY");

//  provide a symbol to force linking against
int force_link_MALY = 0;

}


