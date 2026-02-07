
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

#include "lstrReader.h"
#include "lstrWriter.h"
#include "lstrPlugin.h"
#include "lstrFormat.h"

#include "dbStream.h"
#include "version.h"

#include "tlClassRegistry.h"

namespace lstr
{

// ---------------------------------------------------------------
//  Signature string and generator

const char *LStream_sig = "LStream_1.0";

const char *LStream_generator = "klayout " STRINGIFY(KLAYOUT_VERSION);

// ---------------------------------------------------------------

/**
 *  @brief The LStream plugin
 * 
 *  Providing a class a regiserting it will enable this file 
 *  format inside KLayout.
 * 
 *  It implements the "db::StreamFormatDeclaration" interface
 *  and provides KLayout with the necessary information to 
 *  implement the format.
 */
class LStreamFormatDeclaration
  : public db::StreamFormatDeclaration
{
  virtual std::string format_name () const { return "LStream"; }
  virtual std::string format_desc () const { return "LStream"; }
  virtual std::string format_title () const { return "LStream"; }
  virtual std::string file_format () const { return "LStream files (*.lstr *.lstr.gz)"; }

  /**
   *  @brief Returns a value indicating whether the given stream represents the particular format
   * 
   *  KLayout will use this method to identify a file by content, rather than 
   *  suffix. In the LStream case, the format is detected by the magic bytes
   *  at the front of the stream.
   */
  virtual bool detect (tl::InputStream &stream) const 
  {
    const char *hdr = stream.get (strlen (LStream_sig) + 1);
    return (hdr && strcmp (hdr, LStream_sig) == 0);
  }

  /**
   *  @brief Creates a reader object that does the actual reading
   */
  virtual db::ReaderBase *create_reader (tl::InputStream &s) const 
  {
    return new Reader (s);
  }

  /**
   *  @brief Creates a writer object that does the actual reading
   */
  virtual db::WriterBase *create_writer () const 
  {
    return new Writer ();
  }

  /**
   *  @brief Returns a value indicating whether reading is supported
   */
  virtual bool can_read () const
  {
    return true;
  }

  /**
   *  @brief Returns a value indicating whether writing is supported
   */
  virtual bool can_write () const
  {
    return true;
  }

  virtual tl::XMLElementBase *xml_writer_options_element () const
  {
    return new db::WriterOptionsXMLElement<lstr::WriterOptions> ("lstream",
      tl::make_member (&lstr::WriterOptions::compression_level, "compression-level") +
      tl::make_member (&lstr::WriterOptions::recompress, "recompress") +
      tl::make_member (&lstr::WriterOptions::permissive, "permissive")
    );
  }
};

static tl::RegisteredClass<db::StreamFormatDeclaration> format_decl (new LStreamFormatDeclaration (), 2050, "LStream");

}

