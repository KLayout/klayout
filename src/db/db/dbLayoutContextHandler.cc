
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


#include "dbLayoutContextHandler.h"
#include "dbLibraryManager.h"
#include "dbLibrary.h"

namespace db
{

LayoutContextHandler::LayoutContextHandler (const db::Layout *layout)
  : tl::ContextHandler (), mp_layout (layout), mp_layout_nc (0)
{
}

LayoutContextHandler::LayoutContextHandler (db::Layout *layout, bool can_modify)
  : tl::ContextHandler (), mp_layout (layout), mp_layout_nc (0)
{
  if (can_modify) {
    mp_layout_nc = layout;
  }
}

tl::Variant LayoutContextHandler::eval_bracket (const std::string &content) const
{
  tl::Extractor ex (content.c_str ());
  db::LayerProperties lp;
  lp.read (ex);

  if (! ex.at_end ()) {
    throw tl::Exception (tl::to_string (tr ("Not a valid layer source expression: ")) + content);
  }

  for (db::LayerIterator l = mp_layout->begin_layers (); l != mp_layout->end_layers (); ++l) {
    if ((*l).second->log_equal (lp)) {
      return tl::Variant ((*l).first);
    }
  }

  if (mp_layout_nc) {
    return tl::Variant (mp_layout_nc->insert_layer (lp));
  } else {
    throw tl::Exception (tl::to_string (tr ("Not a valid layer: ")) + lp.to_string ());
  }
}

tl::Variant LayoutContextHandler::eval_double_bracket (const std::string &s) const
{
  std::pair<bool, db::cell_index_type> ci = mp_layout->cell_by_name (s.c_str ());
  if (! ci.first) {

    if (mp_layout_nc) {

      std::string libname;
      const char *cp;
      for (cp = s.c_str (); *cp && *cp != '.'; ++cp) {
        libname += *cp;
      }
      if (*cp == '.') {

        std::string tail = cp + 1;

        db::Library *lib = db::LibraryManager::instance ().lib_ptr_by_name (libname, mp_layout->technology_name ());
        if (! lib) {
          throw tl::Exception (tl::to_string (tr ("Not a valid library name: ")) + libname);
        }

        db::LayoutContextHandler lib_context (&lib->layout (), true);
        tl::Variant r = lib_context.eval_double_bracket (tail);
        if (r.is_nil ()) {
          return r;
        } else {
          db::cell_index_type lib_cell_id = r.to<db::cell_index_type> ();
          return tl::Variant (mp_layout_nc->get_lib_proxy (lib, lib_cell_id));
        }

      } else {
        return tl::Variant (mp_layout_nc->add_cell (s.c_str ()));
      }


    } else {
      throw tl::Exception (tl::to_string (tr ("Not a valid cell name: ")) + s);
    }

  }

  return tl::Variant (ci.second);
}

}

