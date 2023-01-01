
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

#include "gsiDecl.h"
#include "rdb.h"

namespace gsi
{

#if defined(HAVE_QTBINDINGS)

ClassExt<rdb::Item> decl_RdbItem (
  gsi::method ("image", &rdb::Item::image,
    "@brief Gets the attached image as a QImage object\n"
    "\n"
    "This method has been added in version 0.28."
  ) +
  gsi::method ("image=", static_cast<void (rdb::Item::*) (const QImage &)> (&rdb::Item::set_image),
    "@brief Sets the attached image from a QImage object\n"
    "\n"
    "This method has been added in version 0.28."
  )
);

#endif

}
