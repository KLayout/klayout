
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
#include "gsiSignals.h"

#include "layCellView.h"
#include "layLayoutViewBase.h"
#include "layLayoutHandle.h"

namespace gsi
{

static db::Layout *get_layout (const lay::CellViewRef *cv)
{
  if (cv->handle ()) {
    return &cv->handle ()->layout ();
  } else {
    return 0;
  }
}

static lay::LayoutHandleRef *get_layout_handle (const lay::CellViewRef *cv)
{
  if (cv->handle ()) {
    return new lay::LayoutHandleRef (cv->handle ());
  } else {
    return 0;
  }
}

static std::string name (const lay::CellViewRef *cv)
{
  if (cv->handle ()) {
    return cv->handle ()->name ();
  } else {
    return std::string ();
  }
}

static void set_name (lay::CellViewRef *cv, const std::string &name)
{
  cv->set_name (name);
}

static std::string filename (const lay::CellViewRef *cv)
{
  if (cv->handle ()) {
    return cv->handle ()->filename ();
  } else {
    return std::string ();
  }
}

static bool is_dirty (const lay::CellViewRef *cv)
{
  if (cv->handle ()) {
    return cv->handle ()->is_dirty ();
  } else {
    return false;
  }
}

static void apply_technology (const lay::CellViewRef *cv, const std::string &tech)
{
  if (cv->handle ()) {
    cv->handle ()->apply_technology (tech);
  }
}

static std::string get_technology (const lay::CellViewRef *cv)
{
  if (cv->handle ()) {
    return cv->handle ()->tech_name ();
  } else {
    return std::string ();
  }
}

static tl::Event &get_technology_changed_event (lay::CellViewRef *cv)
{
  if (! cv->is_valid ()) {
    throw tl::Exception (tl::to_string (tr ("Not a valid cellview")));
  }
  return (*cv)->technology_changed_event;
}

static void set_cell (lay::CellViewRef *cv, db::Cell *cell)
{
  if (! cell) {
    cv->reset_cell ();
  } else {
    cv->set_cell (cell->cell_index ());
  }
}

static void close_cellview (lay::CellViewRef *cv)
{
  if (cv->is_valid ()) {
    cv->view ()->erase_cellview (cv->index ());
  }
}

static std::string get_cell_name (const lay::CellViewRef *cv)
{
  if (cv->cell () == 0) {
    return std::string ();
  } else {
    return (*cv)->layout ().cell_name (cv->cell_index ());
  }
}

static void cv_descend (lay::CellViewRef *cv, const std::vector<db::InstElement> &path)
{
  if (cv->is_valid ()) {
    cv->view ()->descend (path, cv->index ());
  }
}

static void cv_ascend (lay::CellViewRef *cv)
{
  if (cv->is_valid ()) {
    cv->view ()->ascend (cv->index ());
  }
}

static bool cv_is_cell_hidden (lay::CellViewRef *cv, const db::Cell *cell)
{
  if (cv->is_valid () && cell) {
    if (cell->layout () != &(*cv)->layout ()) {
      throw tl::Exception (tl::to_string (tr ("The cell is not a cell of the view's layout")));
    }
    return cv->view ()->is_cell_hidden (cell->cell_index (), cv->index ());
  } else {
    return false;
  }
}

static void cv_hide_cell (lay::CellViewRef *cv, const db::Cell *cell)
{
  if (cv->is_valid () && cell) {
    if (cell->layout () != &(*cv)->layout ()) {
      throw tl::Exception (tl::to_string (tr ("The cell is not a cell of the view's layout")));
    }
    cv->view ()->hide_cell (cell->cell_index (), cv->index ());
  }
}

static void cv_show_cell (lay::CellViewRef *cv, const db::Cell *cell)
{
  if (cv->is_valid () && cell) {
    if (cell->layout () != &(*cv)->layout ()) {
      throw tl::Exception (tl::to_string (tr ("The cell is not a cell of the view's layout")));
    }
    cv->view ()->show_cell (cell->cell_index (), cv->index ());
  }
}

static void cv_show_all_cells (lay::CellViewRef *cv)
{
  if (cv->is_valid ()) {
    cv->view ()->show_all_cells (cv->index ());
  }
}

Class<lay::CellViewRef> decl_CellView ("lay", "CellView",
  method ("==", static_cast<bool (lay::CellViewRef::*) (const lay::CellViewRef &) const> (&lay::CellViewRef::operator==), gsi::arg ("other"),
    "@brief Equality: indicates whether the cellviews refer to the same one\n"
    "In version 0.25, the definition of the equality operator has been changed to reflect identity of the "
    "cellview. Before that version, identity of the cell shown was implied."
  ) +
  method ("index", &lay::CellViewRef::index,
    "@brief Gets the index of this cellview in the layout view\n"
    "The index will be negative if the cellview is not a valid one.\n"
    "This method has been added in version 0.25.\n"
  ) +
  method ("is_valid?", &lay::CellViewRef::is_valid,
    "@brief Returns true, if the cellview is valid\n"
    "A cellview may become invalid if the corresponding tab is closed for example."
  ) +
  method ("path=|set_path", &lay::CellViewRef::set_unspecific_path, gsi::arg ("path"),
    "@brief Sets the unspecific part of the path explicitly\n"
    "\n"
    "Setting the unspecific part of the path will clear the context path component and\n"
    "update the context and target cell.\n"
  ) +
  method ("context_path=|set_context_path", &lay::CellViewRef::set_specific_path, gsi::arg ("path"),
    "@brief Sets the context path explicitly\n"
    "\n"
    "This method assumes that the unspecific part of the path \n"
    "is established already and that the context path starts\n"
    "from the context cell.\n"
  ) +
  method ("cell_index=|set_cell", (void (lay::CellViewRef::*) (lay::CellViewRef::cell_index_type)) &lay::CellViewRef::set_cell, gsi::arg ("cell_index"),
    "@brief Sets the path to the given cell\n"
    "\n"
    "This method will construct any path to this cell, not a \n"
    "particular one. It will clear the context path\n"
    "and update the context and target cell. Note that the cell is specified by its index.\n"
  ) +
  method ("cell_name=|set_cell_name", (void (lay::CellViewRef::*) (const std::string &)) &lay::CellViewRef::set_cell, gsi::arg ("cell_name"),
    "@brief Sets the cell by name\n"
    "\n"
    "If the name is not a valid one, the cellview will become\n"
    "invalid.\n"
    "This method will construct any path to this cell, not a \n"
    "particular one. It will clear the context path\n"
    "and update the context and target cell.\n"
  ) +
  method_ext ("cell=", set_cell, gsi::arg ("cell"),
    "@brief Sets the cell by reference to a Cell object\n"
    "Setting the cell reference to nil invalidates the cellview. "
    "This method will construct any path to this cell, not a \n"
    "particular one. It will clear the context path\n"
    "and update the context and target cell.\n"
  ) + 
  method ("reset_cell", &lay::CellViewRef::reset_cell,
    "@brief Resets the cell \n"
    "\n"
    "The cellview will become invalid. The layout object will\n"
    "still be attached to the cellview, but no cell will be selected.\n"
  ) +
  method ("ctx_cell_index", &lay::CellViewRef::ctx_cell_index,
    "@brief Gets the context cell's index\n"
  ) +
  method ("ctx_cell", &lay::CellViewRef::ctx_cell,
    "@brief Gets the reference to the context cell currently addressed\n"
  ) +
  method ("cell_index", &lay::CellViewRef::cell_index,
    "@brief Gets the target cell's index\n"
  ) +
  method ("cell", &lay::CellViewRef::cell,
    "@brief Gets the reference to the target cell currently addressed\n"
  ) +
  method_ext ("cell_name", &get_cell_name,
    "@brief Gets the name of the target cell currently addressed\n"
  ) +
  method_ext ("filename", &gsi::filename,
    "@brief Gets filename associated with the layout behind the cellview\n"
  ) +
  method_ext ("is_dirty?", &gsi::is_dirty,
    "@brief Gets a flag indicating whether the layout needs saving\n"
    "A layout is 'dirty' if it is modified and needs saving. This method returns "
    "true in this case.\n"
    "\n"
    "This method has been introduced in version 0.24.10.\n"
  ) +
  method_ext ("name", &gsi::name,
    "@brief Gets the unique name associated with the layout behind the cellview\n"
  ) +
  method_ext ("name=", &gsi::set_name, gsi::arg("name"),
    "@brief sets the unique name associated with the layout behind the cellview\n"
    "\n"
    "this method has been introduced in version 0.25."
  ) +
  method ("path", &lay::CellViewRef::unspecific_path,
    "@brief Gets the cell's unspecific part of the path leading to the context cell\n"
  ) +
  method ("context_path", &lay::CellViewRef::specific_path,
    "@brief Gets the cell's context path\n"
    "The context path leads from the context cell to the target cell in a specific "
    "fashion, i.e. describing each instance in detail, not just by cell indexes. If "
    "the context and target cell are identical, the context path is empty."
  ) +
  method ("context_trans", &lay::CellViewRef::context_trans,
    "@brief Gets the accumulated transformation of the context path\n"
    "This is the transformation applied to the target cell before it is shown in the context cell\n"
    "Technically this is the product of all transformations over the context path.\n"
    "See \\context_dtrans for a version delivering a micron-unit space transformation.\n"
    "\n"
    "This method has been introduced in version 0.27.3.\n"
  ) +
  method ("context_dtrans", &lay::CellViewRef::context_dtrans,
    "@brief Gets the accumulated transformation of the context path in micron unit space\n"
    "This is the transformation applied to the target cell before it is shown in the context cell\n"
    "Technically this is the product of all transformations over the context path.\n"
    "See \\context_trans for a version delivering an integer-unit space transformation.\n"
    "\n"
    "This method has been introduced in version 0.27.3.\n"
  ) +
  event_ext ("on_technology_changed", &get_technology_changed_event,
    "@brief An event indicating that the technology has changed\n"
    "This event is triggered when the CellView is attached to a different technology.\n"
    "\n"
    "This event has been introduced in version 0.27.\n"
  ) +
  method_ext ("technology", &get_technology,
    "@brief Returns the technology name for the layout behind the given cell view\n"
    "This method has been added in version 0.23.\n"
  ) + 
  method_ext ("technology=", &apply_technology, gsi::arg ("tech_name"),
    "@brief Sets the technology for the layout behind the given cell view\n"
    "According to the specification of the technology, new layer properties may be loaded "
    "or the net tracer may be reconfigured. If the layout is shown in multiple views, the "
    "technology is updated for all views.\n"
    "This method has been added in version 0.22.\n"
  ) + 
  method_ext ("layout", &get_layout,
    "@brief Gets the reference to the layout object addressed by this view\n"
  ) +
  factory_ext ("layout_handle", &get_layout_handle,
    "@brief Gets the handle to the layout object addressed by this cell view\n"
    "A layout handle is a reference-counted pointer to a layout object and allows sharing this "
    "resource among different views or objects. By holding a handle, the layout object is not "
    "destroyed when the view closes for example.\n"
    "\n"
    "This method has been added in version 0.30.7.\n"
  ) +
  method_ext ("descend", &cv_descend, gsi::arg ("path"),
    "@brief Descends further into the hierarchy.\n"
    "Adds the given path (given as an array of InstElement objects) to the specific path of the "
    "cellview with the given index. In effect, the cell addressed by the terminal of the new path "
    "components can be shown in the context of the upper cells, if the minimum hierarchy level is "
    "set to a negative value.\n"
    "The path is assumed to originate from the current cell and contain specific instances sorted from "
    "top to bottom."
    "\n"
    "This method has been added in version 0.25."
  ) +
  method_ext ("ascend", &cv_ascend,
    "@brief Ascends upwards in the hierarchy.\n"
    "Removes one element from the specific path of the cellview with the given index. Returns the element "
    "removed."
    "\n"
    "This method has been added in version 0.25."
  ) +
  method_ext ("is_cell_hidden?", &cv_is_cell_hidden, gsi::arg ("cell"),
    "@brief Returns true, if the given cell is hidden\n"
    "\n"
    "This method has been added in version 0.25."
  ) +
  method_ext ("hide_cell", &cv_hide_cell, gsi::arg ("cell"),
    "@brief Hides the given cell\n"
    "\n"
    "This method has been added in version 0.25."
  ) +
  method_ext ("show_cell", &cv_show_cell, gsi::arg ("cell"),
    "@brief Shows the given cell (cancels the effect of \\hide_cell)\n"
    "\n"
    "This method has been added in version 0.25."
  ) +
  method_ext ("show_all_cells", &cv_show_all_cells,
    "@brief Makes all cells shown (cancel effects of \\hide_cell) for the specified cell view\n"
    "\n"
    "This method has been added in version 0.25."
  ) +
  method_ext ("close", &close_cellview,
    "@brief Closes this cell view\n"
    "\n"
    "This method will close the cellview - remove it from the layout view. After this method was called, the "
    "cellview will become invalid (see \\is_valid?).\n"
    "\n"
    "This method was introduced in version 0.25."
  ),
  "@brief A class describing what is shown inside a layout view\n"
  "\n"
  "The cell view points to a specific cell within a certain layout and a hierarchical context.\n"
  "For that, first of all a layout pointer is provided. The cell itself\n"
  "is addressed by an cell_index or a cell object reference.\n"
  "The layout pointer can be nil, indicating that the cell view is invalid.\n"
  "\n"
  "The cell is not only identified by its index or object but also \n"
  "by the path leading to that cell. This path indicates how to find the\n"
  "cell in the hierarchical context of its parent cells. \n"
  "\n"
  "The path is in fact composed of two parts: first in an unspecific fashion,\n"
  "just describing which parent cells are used. The target of this path\n"
  "is called the \"context cell\". It is accessible by the \\ctx_cell_index\n"
  "or \\ctx_cell methods. In the viewer, the unspecific part of the path is\n"
  "the location of the cell in the cell tree.\n"
  "\n"
  "Additionally the path's second part may further identify a specific instance of a certain\n"
  "subcell in the context cell. This is done through a set of \\InstElement\n"
  "objects. The target of this specific path is the actual cell addressed by the\n"
  "cellview. This target cell is accessible by the \\cell_index or \\cell methods.\n"
  "In the viewer, the target cell is shown in the context of the context cell.\n"
  "The hierarchy levels are counted from the context cell, which is on level 0.\n"
  "If the context path is empty, the context cell is identical with the target cell.\n"
  "\n"
  "Starting with version 0.25, the cellview can be modified directly. This will have an immediate "
  "effect on the display. For example, the following code will select a different cell:\n"
  "\n"
  "@code\n"
  "cv = RBA::CellView::active\n"
  "cv.cell_name = \"TOP2\"\n"
  "@/code\n"
  "\n"
  "See @<a href=\"/programming/application_api.xml\">The Application API@</a> for more details about the "
  "cellview objects."
); 

}

