
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
#include "gsiSignals.h"
#include "layLayoutView.h"
#include "laybasicCommon.h"

namespace gsi
{

static lay::CellViewRef get_active_cellview_ref ()
{
  lay::LayoutView *view = lay::LayoutView::current ();
  if (! view) {
    return lay::CellViewRef ();
  }
  if (view->active_cellview_index () >= 0) {
    return view->active_cellview_ref ();
  } else {
    return lay::CellViewRef ();
  }
}

static lay::LayoutView *get_view (lay::CellViewRef *cv)
{
  return cv->view ()->ui ();
}

static ClassExt<lay::CellViewRef> extdecl_CellView (
  method ("active", &get_active_cellview_ref,
    "@brief Gets the active CellView\n"
    "The active CellView is the one that is selected in the current layout view. This method is "
    "equivalent to\n"
    "@code\n"
    "RBA::LayoutView::current.active_cellview\n"
    "@/code\n"
    "If no CellView is active, this method returns nil.\n"
    "\n"
    "This method has been introduced in version 0.23."
  ) +
  method_ext ("view", &get_view,
    "@brief Gets the view the cellview resides in\n"
    "This reference will be nil if the cellview is not a valid one.\n"
    "This method has been added in version 0.25.\n"
  )
);

static lay::LayoutView *get_view_from_lp (lay::LayerPropertiesNode *node)
{
  return node->view ()->ui ();
}

static ClassExt<lay::LayerPropertiesNode> extdecl_LayerPropertiesNode (
  method_ext ("view", &get_view_from_lp,
    "@brief Gets the view this node lives in\n"
    "\n"
    "This reference can be nil if the node is a orphan node that lives outside a view."
  )
);

}
