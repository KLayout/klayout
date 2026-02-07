
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

#if defined(HAVE_QT)

#include "gsiDecl.h"
#include "dbReader.h"
#include "layTechnology.h"

namespace gsi
{

static db::LoadLayoutOptions get_options_from_technology (const std::string &technology)
{
  return db::Technologies::instance ()->technology_by_name (technology)->load_layout_options ();
}

//  Extend "LoadLayoutOptions" by contributions from lay
gsi::ClassExt<db::LoadLayoutOptions> layout_reader_decl (
  gsi::method ("from_technology", &gsi::get_options_from_technology, gsi::arg ("technology"),
    "@brief Gets the reader options of a given technology\n"
    "@param technology The name of the technology to apply\n"
    "Returns the reader options of a specific technology. If the technology name is not valid or an empty string, "
    "the reader options of the default technology are returned.\n"
    "\n"
    "This method has been introduced in version 0.25\n"
  )
);

}

#endif

