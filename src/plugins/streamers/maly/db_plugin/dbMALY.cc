
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
//  MALYData implementation

std::string
MALYTitle::to_string () const
{
  std::string res;
  res += "\"" + string + "\" " + transformation.to_string ();
  res += tl::sprintf (" %g,%g,%g", width, height, pitch);
  if (font == Standard) {
    res += " [Standard]";
  } else if (font == Native) {
    res += " [Native]";
  }
  return res;
}

std::string
MALYStructure::to_string () const
{
  std::string res;
  res += path + "{" + topcell + "}";
  if (layer < 0) {
    res += "(*)";
  } else {
    res += tl::sprintf ("(%d)", layer);
  }

  if (! mname.empty ()) {
    res += " mname(" + mname + ")";
  }
  if (! ename.empty ()) {
    res += " ename(" + ename + ")";
  }
  if (! dname.empty ()) {
    res += " dname(" + dname + ")";
  }

  res += " ";
  res += size.to_string ();

  res += " ";
  res += transformation.to_string ();

  if (nx > 1 || ny > 1) {
    res += tl::sprintf (" [%.12gx%d,%.12gx%d]", dx, nx, dy, ny);
  }

  return res;
}

std::string
MALYMask::to_string () const
{
  std::string res;
  res += "Mask " + name + "\n";
  res += "  Size " + tl::to_string (size_um);

  for (auto t = titles.begin (); t != titles.end (); ++t) {
    res += "\n    Title " + t->to_string ();
  }
  for (auto s = structures.begin (); s != structures.end (); ++s) {
    res += "\n    Ref " + s->to_string ();
  }
  return res;
}

std::string
MALYData::to_string () const
{
  std::string res;
  for (auto m = masks.begin (); m != masks.end (); ++m) {
    if (m != masks.begin ()) {
      res += "\n";
    }
    res += m->to_string ();
  }
  return res;
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
  virtual std::string file_format () const { return "MALY jobdeck files (*.maly *.MALY *.mly *.MLY)"; }

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
    return new db::ReaderOptionsXMLElement<db::MALYReaderOptions> ("maly",
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


