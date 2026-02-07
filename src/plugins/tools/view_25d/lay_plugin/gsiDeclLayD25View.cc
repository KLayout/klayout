
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

#include "gsiDecl.h"
#include "gsiDeclBasic.h"

#include "layD25View.h"
#include "layLayoutView.h"

#include "dbLayerProperties.h"
#include "dbRegion.h"

#include <limits>

#if defined(HAVE_QTBINDINGS)

#  include "gsiQtGuiExternals.h"
#  include "gsiQtWidgetsExternals.h"

FORCE_LINK_GSI_QTGUI
FORCE_LINK_GSI_QTWIDGETS // for Qt5

#else
# define QT_EXTERNAL_BASE(x)
#endif

namespace gsi
{

static lay::D25View *open_d25_view (lay::LayoutView *view)
{
  return lay::D25View::open (view);
}

ClassExt<lay::LayoutView> decl_LayoutViewExt (
  gsi::method_ext ("open_d25_view", &open_d25_view,
    "@brief Opens the 2.5d view window and returns a reference to the D25View object.\n"
    "This method has been introduced in version 0.28.\n"
  )
);

Class<lay::D25View> decl_D25View (QT_EXTERNAL_BASE (QDialog) "lay", "D25View",
  gsi::method ("clear", &lay::D25View::clear,
    "@brief Clears all display entries in the view"
  ) +
  gsi::method ("begin", &lay::D25View::begin, gsi::arg ("generator"),
    "@brief Initiates delivery of display groups"
  ) +
  gsi::method ("open_display", &lay::D25View::open_display, gsi::arg ("frame_color"), gsi::arg ("fill_color"), gsi::arg ("like"), gsi::arg ("name"),
    "@brief Creates a new display group"
  ) +
  gsi::method ("entry", &lay::D25View::entry, gsi::arg ("data"), gsi::arg ("dbu"), gsi::arg ("zstart"), gsi::arg ("zstop"),
    "@brief Creates a new display entry in the group opened with \\open_display"
  ) +
  gsi::method ("entry", &lay::D25View::entry_edge, gsi::arg ("data"), gsi::arg ("dbu"), gsi::arg ("zstart"), gsi::arg ("zstop"),
    "@brief Creates a new display entry in the group opened with \\open_display"
  ) +
  gsi::method ("entry", &lay::D25View::entry_edge_pair, gsi::arg ("data"), gsi::arg ("dbu"), gsi::arg ("zstart"), gsi::arg ("zstop"),
    "@brief Creates a new display entry in the group opened with \\open_display"
  ) +
  gsi::method ("close_display", &lay::D25View::close_display,
    "@brief Finishes the display group"
  ) +
  gsi::method ("finish", &lay::D25View::finish,
    "@brief Finishes the view - call this after the display groups have been created"
  ) +
  gsi::method ("close", &lay::D25View::close,
    "@brief Closes the view"
  ),
  "@brief The 2.5d View Dialog\n"
  "\n"
  "This class is used internally to implement the 2.5d feature.\n"
  "\n"
  "This class has been introduced in version 0.28."
);

}
