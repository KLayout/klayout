
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

#include "layBookmarkList.h"
#include "tlXMLParser.h"

#include <fstream>
#include <cstdlib>
#include <cctype>

namespace lay
{

//  helper typedefs to make the templates more readable
typedef std::vector<std::string> string_v;
typedef std::vector<SpecificInst> specific_inst_v;
typedef std::list<CellPath> cell_path_v;

// -------------------------------------------------------------
//  BookmarkListElement implementation

const tl::XMLElementList *
BookmarkListElement::xml_format ()
{
  static tl::XMLElementList format (
    tl::make_member<std::string, BookmarkListElement> (&BookmarkListElement::name, &BookmarkListElement::set_name, "name") +
    //  Hint: the following is a copy of the DisplayState attributes:
    tl::make_member<double, BookmarkListElement> (&BookmarkListElement::xleft, &BookmarkListElement::set_xleft, "x-left") + 
    tl::make_member<double, BookmarkListElement> (&BookmarkListElement::xright, &BookmarkListElement::set_xright, "x-right") + 
    tl::make_member<double, BookmarkListElement> (&BookmarkListElement::ybottom, &BookmarkListElement::set_ybottom, "y-bottom") + 
    tl::make_member<double, BookmarkListElement> (&BookmarkListElement::ytop, &BookmarkListElement::set_ytop, "y-top") + 
    tl::make_member<int, BookmarkListElement> (&BookmarkListElement::min_hier, &BookmarkListElement::set_min_hier, "min-hier") + 
    tl::make_member<int, BookmarkListElement> (&BookmarkListElement::max_hier, &BookmarkListElement::set_max_hier, "max-hier") + 
    tl::make_element<cell_path_v, BookmarkListElement> (&BookmarkListElement::paths, &BookmarkListElement::set_paths, "cellpaths", 
      tl::make_element<CellPath, cell_path_v::const_iterator, cell_path_v> (&cell_path_v::begin, &cell_path_v::end, &cell_path_v::push_back, "cellpath", CellPath::xml_format ())
    )
  );

  return &format;
}

// -------------------------------------------------------------
//  BookmarkList implementation

//  declaration of the bookmarks file XML structure
static const tl::XMLStruct <BookmarkList::bookmark_list_type>
bookmarks_structure ("bookmarks", 
  tl::make_element<BookmarkListElement, BookmarkList::bookmark_list_type::const_iterator, BookmarkList::bookmark_list_type> (&BookmarkList::bookmark_list_type::begin, &BookmarkList::bookmark_list_type::end, &BookmarkList::bookmark_list_type::push_back, "bookmark", BookmarkListElement::xml_format())
);

void 
BookmarkList::load (const std::string &fn) 
{
  tl::XMLFileSource in (fn);

  m_list.clear ();
  bookmarks_structure.parse (in, m_list); 

  tl::log << "Loaded bookmarks from " << fn;
}

void 
BookmarkList::save (const std::string &fn) const 
{
  tl::OutputStream os (fn, tl::OutputStream::OM_Plain);
  bookmarks_structure.write (os, m_list); 

  tl::log << "Saved bookmarks to " << fn;
}

std::string
BookmarkList::propose_new_bookmark_name () const
{
  int n = 0;

  for (const_iterator b = begin (); b != end (); ++b) {

    const std::string &name = b->name ();
    if (! name.empty ()) {

      const char *cp = name.c_str () + name.size ();
      while (cp != name.c_str ()) {
        if (! isdigit (cp [-1])) {
          break;
        }
        --cp;
      }

      int nn = atoi (cp);
      n = std::max (nn, n);

    }

  }

  return "B" + tl::to_string (n + 1);
}

}
