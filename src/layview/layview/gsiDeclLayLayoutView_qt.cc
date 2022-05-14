
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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
#include "gsiEnums.h"
#include "rdb.h"
#include "layLayoutView.h"
#include "layLayerProperties.h"
#include "layDitherPattern.h"
#include "layLineStyles.h"
#include "dbSaveLayoutOptions.h"
#include "dbLayoutToNetlist.h"
#include "dbLayoutVsSchematic.h"
#include "tlStream.h"

namespace gsi
{

#if defined(HAVE_QTBINDINGS)
static lay::LayoutView *new_view (QWidget *parent, bool editable, db::Manager *manager, unsigned int options)
{
  lay::LayoutView *lv = new lay::LayoutView (manager, editable, 0 /*plugin parent*/, parent, "view", options);
  if (parent) {
    //  transfer ownership to the parent
    lv->keep ();
  }
  return lv;
}
#endif

static lay::LayoutView *new_view2 (bool editable, db::Manager *manager, unsigned int options)
{
  return new lay::LayoutView (manager, editable, 0 /*plugin parent*/, 0 /*parent*/, "view", options);
}

extern Class<lay::LayoutViewBase> decl_LayoutViewBase;

Class<lay::LayoutView> decl_LayoutView (decl_LayoutViewBase, "lay", "LayoutView",
#if defined(HAVE_QTBINDINGS)
  gsi::constructor ("new", &new_view, gsi::arg ("parent"), gsi::arg ("editable", false), gsi::arg ("manager", (db::Manager *) 0, "nil"), gsi::arg ("options", (unsigned int) 0),
    "@brief Creates a standalone view\n"
    "\n"
    "This constructor is for special purposes only. To create a view in the context of a main window, "
    "use \\MainWindow#create_view and related methods.\n"
    "\n"
    "@param parent The parent widget in which to embed the view\n"
    "@param editable True to make the view editable\n"
    "@param manager The \\Manager object to enable undo/redo\n"
    "@param options A combination of the values in the LV_... constants\n"
    "\n"
    "This constructor has been introduced in version 0.25.\n"
    "It has been enhanced with the arguments in version 0.27.\n"
  ) +
#endif
  gsi::constructor ("new", &new_view2, gsi::arg ("editable", false), gsi::arg ("manager", (db::Manager *) 0, "nil"), gsi::arg ("options", (unsigned int) 0),
    "@brief Creates a standalone view\n"
    "\n"
    "This constructor is for special purposes only. To create a view in the context of a main window, "
    "use \\MainWindow#create_view and related methods.\n"
    "\n"
    "@param editable True to make the view editable\n"
    "@param manager The \\Manager object to enable undo/redo\n"
    "@param options A combination of the values in the LV_... constants\n"
    "\n"
    "This constructor has been introduced in version 0.25.\n"
    "It has been enhanced with the arguments in version 0.27.\n"
  ) +
#if defined(HAVE_QTBINDINGS)
  gsi::method ("layer_control_frame", static_cast<QWidget *(lay::LayoutView::*) ()> (&lay::LayoutView::layer_control_frame),
    "@brief Gets the layer control side widget\n"
    "A 'side widget' is a widget attached to the view. It does not have a parent, so you can "
    "embed it into a different context. Please note that with embedding through 'setParent' it will be "
    "destroyed when your parent widget gets destroyed. It will be lost then to the view.\n"
    "\n"
    "The side widget can be configured through the views configuration interface.\n"
    "\n"
    "This method has been introduced in version 0.27\n"
  ) +
  gsi::method ("hierarchy_control_frame", static_cast<QWidget *(lay::LayoutView::*) ()> (&lay::LayoutView::hierarchy_control_frame),
    "@brief Gets the cell view (hierarchy view) side widget\n"
    "For details about side widgets see \\layer_control_frame.\n"
    "\n"
    "This method has been introduced in version 0.27\n"
  ) +
  gsi::method ("libraries_frame", static_cast<QWidget *(lay::LayoutView::*) ()> (&lay::LayoutView::libraries_frame),
    "@brief Gets the library view side widget\n"
    "For details about side widgets see \\layer_control_frame.\n"
    "\n"
    "This method has been introduced in version 0.27\n"
  ) +
  gsi::method ("bookmarks_frame", static_cast<QWidget *(lay::LayoutView::*) ()> (&lay::LayoutView::bookmarks_frame),
    "@brief Gets the bookmarks side widget\n"
    "For details about side widgets see \\layer_control_frame.\n"
    "\n"
    "This method has been introduced in version 0.27\n"
  ) +
#endif
  gsi::method ("current", &lay::LayoutView::current,
    "@brief Returns the current view\n"
    "The current view is the one that is shown in the current tab. Returns nil if no layout is loaded.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method ("bookmark_view", static_cast<void (lay::LayoutView::*) (const std::string &)> (&lay::LayoutView::bookmark_view), gsi::arg ("name"),
    "@brief Bookmarks the current view under the given name\n"
    "\n"
    "@param name The name under which to bookmark the current state"
  ) +
  gsi::event ("on_close", static_cast<tl::Event (lay::LayoutView::*)> (&lay::LayoutView::close_event),
    "@brief A event indicating that the view is about to close\n"
    "\n"
    "This event is triggered when the view is going to be closed entirely.\n"
    "\n"
    "It has been added in version 0.25."
  ) +
  gsi::event ("on_show", static_cast<tl::Event (lay::LayoutView::*)> (&lay::LayoutView::show_event),
    "@brief A event indicating that the view is going to become visible\n"
    "\n"
    "It has been added in version 0.25."
  ) +
  gsi::event ("on_hide", static_cast<tl::Event (lay::LayoutView::*)> (&lay::LayoutView::hide_event),
    "@brief A event indicating that the view is going to become invisible\n"
    "\n"
    "It has been added in version 0.25."
  ) +
  gsi::method ("show_rdb", static_cast<void (lay::LayoutView::*) (int, int)> (&lay::LayoutView::open_rdb_browser), gsi::arg ("rdb_index"), gsi::arg ("cv_index"),
    "@brief Shows a report database in the marker browser on a certain layout\n"
    "The marker browser is opened showing the report database with the index given by \"rdb_index\".\n"
    "It will be attached (i.e. navigate to) the layout with the given cellview index in \"cv_index\".\n"
  ) +
  gsi::method ("show_l2ndb", static_cast<void (lay::LayoutView::*) (int, int)> (&lay::LayoutView::open_l2ndb_browser), gsi::arg ("l2ndb_index"), gsi::arg ("cv_index"),
    "@brief Shows a netlist database in the marker browser on a certain layout\n"
    "The netlist browser is opened showing the netlist database with the index given by \"l2ndb_index\".\n"
    "It will be attached (i.e. navigate to) the layout with the given cellview index in \"cv_index\".\n"
    "\n"
    "This method has been added in version 0.26."
  ) +
  gsi::method ("show_lvsdb", static_cast<void (lay::LayoutView::*) (int, int)> (&lay::LayoutView::open_l2ndb_browser), gsi::arg ("lvsdb_index"), gsi::arg ("cv_index"),
    "@brief Shows a netlist database in the marker browser on a certain layout\n"
    "The netlist browser is opened showing the netlist database with the index given by \"lvsdb_index\".\n"
    "It will be attached (i.e. navigate to) the layout with the given cellview index in \"cv_index\".\n"
    "\n"
    "This method has been added in version 0.26."
  ),
  "@brief The view object presenting one or more layout objects\n"
  "\n"
  "The visual part of the view is the tab panel in the main window. The non-visual part "
  "are the redraw thread, the layout handles, cell lists, layer view lists etc. "
  "This object controls these aspects of the view and controls the appearance of the data. "
);

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
