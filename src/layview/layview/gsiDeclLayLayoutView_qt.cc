
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

#if defined(HAVE_QT)

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

#include "gsiQtGuiExternals.h"
#include "gsiQtWidgetsExternals.h"  //  for Qt5

#include <QFrame>

namespace gsi
{

#if defined(HAVE_QTBINDINGS)

static lay::LayoutViewWidget *new_view_widget (QWidget *parent, bool editable, db::Manager *manager, unsigned int options)
{
  lay::LayoutViewWidget *lv = new lay::LayoutViewWidget (manager, editable, 0 /*plugin parent*/, parent, options);
  if (parent) {
    //  transfer ownership to the parent
    lv->keep ();
  }
  return lv;
}

static lay::LayoutView *get_view (lay::LayoutViewWidget *lv)
{
  return lv->view ();
}

static QWidget *layer_control_frame (lay::LayoutViewWidget *lv)
{
  return lv->layer_control_frame ();
}

static QWidget *layer_toolbox_frame (lay::LayoutViewWidget *lv)
{
  return lv->layer_toolbox_frame ();
}

static QWidget *hierarchy_control_frame (lay::LayoutViewWidget *lv)
{
  return lv->hierarchy_control_frame ();
}

static QWidget *libraries_frame (lay::LayoutViewWidget *lv)
{
  return lv->libraries_frame ();
}

static QWidget *bookmarks_frame (lay::LayoutViewWidget *lv)
{
  return lv->bookmarks_frame ();
}

Class<lay::LayoutViewWidget> decl_LayoutViewWidget (QT_EXTERNAL_BASE (QFrame) "lay", "LayoutViewWidget",
  gsi::constructor ("new", &new_view_widget, gsi::arg ("parent"), gsi::arg ("editable", false), gsi::arg ("manager", (db::Manager *) 0, "nil"), gsi::arg ("options", (unsigned int) 0),
    "@brief Creates a standalone view widget\n"
    "\n"
    "@param parent The parent widget in which to embed the view\n"
    "@param editable True to make the view editable\n"
    "@param manager The \\Manager object to enable undo/redo\n"
    "@param options A combination of the values in the LV_... constants from \\LayoutViewBase\n"
    "\n"
    "This constructor has been introduced in version 0.25.\n"
    "It has been enhanced with the arguments in version 0.27.\n"
  ) +
  gsi::method_ext ("layer_control_frame", &layer_control_frame,
    "@brief Gets the layer control side widget\n"
    "A 'side widget' is a widget attached to the view. It does not have a parent, so you can "
    "embed it into a different context. Please note that with embedding through 'setParent' it will be "
    "destroyed when your parent widget gets destroyed. It will be lost then to the view.\n"
    "\n"
    "The side widget can be configured through the views configuration interface.\n"
    "\n"
    "This method has been introduced in version 0.27\n"
  ) +
  gsi::method_ext ("layer_toolbox_frame", &layer_toolbox_frame,
    "@brief Gets the layer toolbox side widget\n"
    "A 'side widget' is a widget attached to the view. It does not have a parent, so you can "
    "embed it into a different context. Please note that with embedding through 'setParent' it will be "
    "destroyed when your parent widget gets destroyed. It will be lost then to the view.\n"
    "\n"
    "The side widget can be configured through the views configuration interface.\n"
    "\n"
    "This method has been introduced in version 0.28\n"
  ) +
  gsi::method_ext ("hierarchy_control_frame", &hierarchy_control_frame,
    "@brief Gets the cell view (hierarchy view) side widget\n"
    "For details about side widgets see \\layer_control_frame.\n"
    "\n"
    "This method has been introduced in version 0.27\n"
  ) +
  gsi::method_ext ("libraries_frame", &libraries_frame,
    "@brief Gets the library view side widget\n"
    "For details about side widgets see \\layer_control_frame.\n"
    "\n"
    "This method has been introduced in version 0.27\n"
  ) +
  gsi::method_ext ("bookmarks_frame", &bookmarks_frame,
    "@brief Gets the bookmarks side widget\n"
    "For details about side widgets see \\layer_control_frame.\n"
    "\n"
    "This method has been introduced in version 0.27\n"
  ) +
  gsi::method_ext ("view", &get_view,
    "@brief Gets the embedded view object.\n"
  ),
  "This object produces a widget which embeds a LayoutView. This widget can be used inside Qt widget hierarchies.\n"
  "To access the \\LayoutView object within, use \\view.\n"
  "\n"
  "This class has been introduced in version 0.28."
);
#endif

static lay::LayoutView *new_view (bool editable, db::Manager *manager, unsigned int options)
{
  return new lay::LayoutView (manager, editable, 0 /*plugin parent*/, options);
}

extern LAYBASIC_PUBLIC Class<lay::LayoutViewBase> decl_LayoutViewBase;

Class<lay::LayoutView> decl_LayoutView (decl_LayoutViewBase, "lay", "LayoutView",
  gsi::constructor ("new", &new_view, gsi::arg ("editable", false), gsi::arg ("manager", (db::Manager *) 0, "nil"), gsi::arg ("options", (unsigned int) 0),
    "@brief Creates a standalone view\n"
    "\n"
    "This constructor is for special purposes only. To create a view in the context of a main window, "
    "use \\MainWindow#create_view and related methods.\n"
    "\n"
    "@param editable True to make the view editable\n"
    "@param manager The \\Manager object to enable undo/redo\n"
    "@param options A combination of the values in the LV_... constants from \\LayoutViewBase\n"
    "\n"
    "This constructor has been introduced in version 0.25.\n"
    "It has been enhanced with the arguments in version 0.27.\n"
  ) +
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

}

#endif
