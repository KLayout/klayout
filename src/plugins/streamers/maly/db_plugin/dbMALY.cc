
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
    return false; // @@@
  }

  virtual ReaderBase *create_reader (tl::InputStream &s) const 
  {
    return new db::MALYReader (s);
  }

  virtual WriterBase *create_writer () const
  {
    return 0;
    // @@@ return new db::MALYWriter ();
  }

  virtual bool can_read () const
  {
    return true;
  }

  virtual bool can_write () const
  {
    return false;
    // @@@ return true;
  }

  virtual tl::XMLElementBase *xml_reader_options_element () const
  {
    return new db::ReaderOptionsXMLElement<db::MALYReaderOptions> ("mag",
      tl::make_member (&db::MALYReaderOptions::dbu, "dbu") 
        /* @@@
      tl::make_member (&db::MALYReaderOptions::lambda, "lambda") +
      tl::make_member (&db::MALYReaderOptions::layer_map, "layer-map") +
      tl::make_member (&db::MALYReaderOptions::create_other_layers, "create-other-layers") +
      tl::make_member (&db::MALYReaderOptions::keep_layer_names, "keep-layer-names") +
      tl::make_member (&db::MALYReaderOptions::merge, "merge") +
      tl::make_element<std::vector<std::string>, db::MALYReaderOptions> (&db::MALYReaderOptions::lib_paths, "lib-paths",
        tl::make_member<std::string, std::vector<std::string>::const_iterator, std::vector<std::string> > (&std::vector<std::string>::begin, &std::vector<std::string>::end, &std::vector<std::string>::push_back, "lib-path")
      )
      */
    );
  }

  /* @@@
  virtual tl::XMLElementBase *xml_writer_options_element () const
  {
    return new db::WriterOptionsXMLElement<db::MALYWriterOptions> ("mag",
      tl::make_member (&db::MALYWriterOptions::lambda, "lambda") +
      tl::make_member (&db::MALYWriterOptions::tech, "tech") +
      tl::make_member (&db::MALYWriterOptions::write_timestamp, "write-timestamp")
    );
  }
  @@@ */
};

//  NOTE: Because MALY has such a high degree of syntactic freedom, the detection is somewhat
//  fuzzy: do MALY at the very end of the detection chain
static tl::RegisteredClass<db::StreamFormatDeclaration> reader_decl (new MALYFormatDeclaration (), 2300, "MALY");

//  provide a symbol to force linking against
int force_link_MALY = 0;

}


