
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


#include "dbCIF.h"
#include "dbCIFReader.h"
#include "dbCIFWriter.h"
#include "dbStream.h"

#include "tlClassRegistry.h"

namespace db
{

// ---------------------------------------------------------------
//  CIFDiagnostics implementation

CIFDiagnostics::~CIFDiagnostics ()
{
  //  .. nothing yet ..
}

// ---------------------------------------------------------------
//  CIF format declaration

class CIFFormatDeclaration
  : public db::StreamFormatDeclaration
{
public:
  CIFFormatDeclaration ()
  {
    //  .. nothing yet ..
  }

  virtual std::string format_name () const { return "CIF"; }
  virtual std::string format_desc () const { return "CIF"; }
  virtual std::string format_title () const { return "CIF (Caltech interchange format)"; }
  virtual std::string file_format () const { return "CIF files (*.cif *.CIF *.cif.gz *.CIF.gz)"; }

  static tl::Extractor &skip_blanks (tl::Extractor &ex)
  {
    while (! ex.at_end () && *ex != ';' && *ex != '-' && *ex != '(' && *ex != ')' && !isalnum(*ex)) {
      ++ex;
    }
    return ex;
  }

  virtual bool detect (tl::InputStream &s) const 
  {
    try {

      //  analyze the first 4000 characters - this is within the initial block read
      //  by the stream and won't trigger a reset of the stream which is not available
      //  on some sources.
      std::string head = s.read_all (4000);
      int n = 0;

      tl::Extractor ex (head.c_str ());
      while (! skip_blanks (ex).at_end ()) {

        if (*ex == '(') {

          //  read over comments
          ++ex;
          int bl = 0;
          while (! ex.at_end () && (*ex != ')' || bl > 0)) {
            //  check for nested comments (bl is the nesting level)
            if (*ex == '(') {
              ++bl;
            } else if (*ex == ')') {
              --bl;
            }
            ++ex;
          }
          if (! ex.at_end ()) {
            ++ex;
          }

        } else if (*ex == ';') {

          //  ignore ;
          ++ex;

        } else if (*ex == 'L') {

          //  first command must be "DS num", or "L shortname"
          ++ex;
          skip_blanks (ex);
          return ! ex.at_end () && isalnum (*ex);

        } else if (*ex == 'D') {

          //  first command must be "DS num", or "L"
          ++ex;
          skip_blanks (ex);
          if (ex.at_end () || *ex != 'S') {
            break;   // not "D<sep>S"
          }
          ++ex;
          skip_blanks (ex);
          if (ex.try_read (n)) {
            return true;
          }

        } else if (*ex == '9') {

          //  read over 9...; commands
          ++ex;
          while (! ex.at_end () && *ex != ';') {
            ++ex;
          }
          if (! ex.at_end ()) {
            ++ex;
          }

        } else {
          break;
        }

      }

    } catch (...) {
      //  ignore errors
    }

    return false;
  }

  virtual ReaderBase *create_reader (tl::InputStream &s) const 
  {
    return new db::CIFReader (s);
  }

  virtual WriterBase *create_writer () const
  {
    return new db::CIFWriter ();
  }

  virtual bool can_read () const
  {
    return true;
  }

  virtual bool can_write () const
  {
    return true;
  }

  virtual tl::XMLElementBase *xml_reader_options_element () const
  {
    return new db::ReaderOptionsXMLElement<db::CIFReaderOptions> ("cif",
      tl::make_member (&db::CIFReaderOptions::wire_mode, "wire-mode") +
      tl::make_member (&db::CIFReaderOptions::dbu, "dbu") +
      tl::make_member (&db::CIFReaderOptions::layer_map, "layer-map") +
      tl::make_member (&db::CIFReaderOptions::create_other_layers, "create-other-layers") +
      tl::make_member (&db::CIFReaderOptions::keep_layer_names, "keep-layer-names")
    );
  }

  virtual tl::XMLElementBase *xml_writer_options_element () const
  {
    return new db::WriterOptionsXMLElement<db::CIFWriterOptions> ("cif",
      tl::make_member (&db::CIFWriterOptions::dummy_calls, "dummy-calls") +
      tl::make_member (&db::CIFWriterOptions::blank_separator, "blank-separator")
    );
  }
};

//  NOTE: Because CIF has such a high degree of syntactic freedom, the detection is somewhat
//  fuzzy: do CIF at the very end of the detection chain
static tl::RegisteredClass<db::StreamFormatDeclaration> reader_decl (new CIFFormatDeclaration (), 2100, "CIF");

//  provide a symbol to force linking against
int force_link_CIF = 0;

}


