
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
#include "edtService.h"
#include "dbRecursiveShapeIterator.h"
#include "layObjectInstPath.h"
#include "layLayoutViewBase.h"

namespace gsi
{

static const db::Instance &inst (const lay::ObjectInstPath *p) 
{
  if (! p->is_cell_inst ()) {
    throw tl::Exception ("Selection does not represent an instance in 'inst' method");
  }
  return p->back ().inst_ptr;
}

static unsigned int path_length (const lay::ObjectInstPath *p)
{
  //  HINT: this method is not quite fast - the path is a list
  return std::distance (p->begin (), p->end ());
}

static const db::InstElement &path_nth (const lay::ObjectInstPath *p, unsigned int n) 
{
  //  HINT: this method is not quite fast - the path is a list
  lay::ObjectInstPath::iterator e;
  for (e = p->begin (); e != p->end () && n-- > 0; ++e) 
    ;
  tl_assert (e != p->end ());
  return *e;
}

static db::Layout *layout_from_inst_path (const lay::ObjectInstPath *p)
{
  db::Cell *cell = 0;

  if (p->is_cell_inst ()) {
    db::Instances *instances = p->back ().inst_ptr.instances ();
    if (instances) {
      cell = instances->cell ();
    }
  } else {
    const db::Shapes *shapes = p->shape ().shapes ();
    if (shapes) {
      cell = shapes->cell ();
    }
  }

  return cell ? cell->layout () : 0;
}

static db::DCplxTrans source_dtrans (const lay::ObjectInstPath *p)
{
  const db::Layout *layout = layout_from_inst_path (p);
  if (layout) {
    double dbu = layout->dbu ();
    return db::CplxTrans (dbu) * p->trans () * db::VCplxTrans (1.0 / dbu);
  } else {
    return db::DCplxTrans ();
  }
}

static db::DCplxTrans dtrans (const lay::ObjectInstPath *p)
{
  const db::Layout *layout = layout_from_inst_path (p);
  if (layout) {
    double dbu = layout->dbu ();
    return db::CplxTrans (dbu) * p->trans_tot () * db::VCplxTrans (1.0 / dbu);
  } else {
    return db::DCplxTrans ();
  }
}

static std::vector<db::InstElement> get_path (const lay::ObjectInstPath *p)
{
  std::vector<db::InstElement> pe;
  pe.insert (pe.end (), p->begin (), p->end ());
  return pe;
}

static void set_path (lay::ObjectInstPath *p, const std::vector<db::InstElement> &pe)
{
  p->assign_path (pe.begin (), pe.end ());
}

static lay::ObjectInstPath *from_si (const db::RecursiveShapeIterator &si, int cv_index)
{
  lay::ObjectInstPath *ip = new lay::ObjectInstPath ();

  if (! si.at_end ()) {

    ip->set_cv_index (cv_index);
    ip->set_layer (si.layer ());
    ip->set_shape (si.shape ());
    ip->set_topcell (si.top_cell ()->cell_index ());
    std::vector<db::InstElement> path (si.path ());
    ip->assign_path (path.begin (), path.end ());

  }

  return ip;
}

static tl::Variant ip_layer (const lay::ObjectInstPath *ip)
{
  if (ip->is_cell_inst ()) {
    return tl::Variant ();
  } else {
    return tl::Variant (ip->layer ());
  }
}

static tl::Variant ip_shape (const lay::ObjectInstPath *ip)
{
  if (ip->is_cell_inst ()) {
    return tl::Variant ();
  } else {
    return tl::Variant (ip->shape ());
  }
}

gsi::Class<lay::ObjectInstPath> decl_ObjectInstPath ("lay", "ObjectInstPath",
  gsi::constructor ("new", &from_si, gsi::arg ("si"), gsi::arg ("cv_index"),
    "@brief Creates a new path object from a \\RecursiveShapeIterator\n"
    "Use this constructor to quickly turn a recursive shape iterator delivery "
    "into a shape selection."
  ) +
  gsi::method ("<", &lay::ObjectInstPath::operator<, gsi::arg ("b"),
    "@brief Provides an order criterion for two ObjectInstPath objects\n"
    "Note: this operator is just provided to establish any order, not a particular one.\n"
    "\n"
    "This method has been introduced with version 0.24.\n"
  ) +
  gsi::method ("!=", &lay::ObjectInstPath::operator!=, gsi::arg ("b"),
    "@brief Inequality of two ObjectInstPath objects\n"
    "See the comments on the == operator.\n"
    "\n"
    "This method has been introduced with version 0.24.\n"
  ) +
  gsi::method ("==", &lay::ObjectInstPath::operator==, gsi::arg ("b"),
    "@brief Equality of two ObjectInstPath objects\n"
    "Note: this operator returns true if both instance paths refer to the same object, not just identical ones.\n"
    "\n"
    "This method has been introduced with version 0.24.\n"
  ) +
  gsi::method ("is_valid?", &lay::ObjectInstPath::is_valid, gsi::arg ("view"),
    "@brief Gets a value indicating whether the instance path refers to a valid object in the context of the given view\n"
    "\n"
    "This predicate has been introduced in version 0.27.12.\n"
  ) +
  gsi::method ("cv_index", &lay::ObjectInstPath::cv_index,
    "@brief Gets the cellview index that describes which cell view the shape or instance is located in\n"
  ) + 
  gsi::method ("cv_index=", &lay::ObjectInstPath::set_cv_index, gsi::arg ("index"),
    "@brief Sets the cellview index that describes which cell view the shape or instance is located in\n"
    "\n"
    "This method has been introduced in version 0.24."
  ) + 
  gsi::method ("top", &lay::ObjectInstPath::topcell, 
    "@brief Gets the cell index of the top cell the selection applies to\n"
    "\n"
    "The top cell is identical to the current cell provided by the cell view.\n"
    "It is the cell from which is instantiation path originates and the container cell "
    "if not instantiation path is set.\n"
    "\n"
    "This method has been introduced in version 0.24."
  ) + 
  gsi::method ("top=", &lay::ObjectInstPath::set_topcell, gsi::arg ("cell_index"),
    "@brief Sets the cell index of the top cell the selection applies to\n"
    "\n"
    "See \\top_cell for a description of this property.\n"
    "\n"
    "This method has been introduced in version 0.24."
  ) + 
  gsi::method ("cell_index", &lay::ObjectInstPath::cell_index_tot, 
    "@brief Gets the cell index of the cell that the selection applies to.\n"
    "This method returns the cell index that describes which cell the selected shape is located in or the cell whose instance is selected if \\is_cell_inst? is true."
    "\n"
    "This property is set implicitly by setting the top cell and adding elements to the instantiation path.\n"
    "To obtain the index of the container cell, use \\source.\n"
  ) + 
  gsi::method_ext ("layout", &layout_from_inst_path,
    "@brief Gets the Layout object the selected object lives in.\n"
    "\n"
    "This method returns the \\Layout object that the selected object lives in. This method may return nil, if "
    "the selection does not point to a valid object.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method ("source", &lay::ObjectInstPath::cell_index, 
    "@brief Returns to the cell index of the cell that the selected element resides inside.\n"
    "\n"
    "If this reference represents a cell instance, this method delivers the index of the cell in which the "
    "cell instance resides. Otherwise, this method returns the same value than \\cell_index.\n"
    "\n"
    "This property is set implicitly by setting the top cell and adding elements to the instantiation path.\n"
    "\n"
    "This method has been added in version 0.16."
  ) + 
  gsi::method ("trans", &lay::ObjectInstPath::trans_tot,
    "@brief Gets the transformation applicable for the shape.\n"
    "\n"
    "If this object represents a shape, this transformation describes how the selected shape is transformed into the current cell of the cell view.\n"
    "Basically, this transformation is the accumulated transformation over the instantiation path. If the ObjectInstPath represents a cell instance, this includes the transformation of the selected instance as well.\n"
    "\n"
    "This property is set implicitly by setting the top cell and adding elements to the instantiation path.\n"
    "This method is not applicable for instance selections. A more generic attribute is \\source_trans.\n"
  ) + 
  gsi::method_ext ("dtrans", &dtrans,
    "@brief Gets the transformation applicable for the shape in micron space.\n"
    "\n"
    "This method returns the same transformation than \\trans, but applicable to objects in micrometer units:\n"
    "\n"
    "@code\n"
    "# renders the micrometer-unit polygon in top cell coordinates:\n"
    "dpolygon_in_top = sel.dtrans * sel.shape.dpolygon\n"
    "@/code\n"
    "\n"
    "This method is not applicable to instance selections. A more generic attribute is \\source_dtrans.\n"
    "\n"
    "The method has been introduced in version 0.25.\n"
  ) +
  gsi::method ("source_trans", &lay::ObjectInstPath::trans,
    "@brief Gets the transformation applicable for an instance and shape.\n"
    "\n"
    "If this object represents a shape, this transformation describes how the selected shape is transformed into the current cell of the cell view.\n"
    "If this object represents an instance, this transformation describes how the selected instance is transformed into the current cell of the cell view.\n"
    "This method is similar to \\trans, except that the resulting transformation does not include the instance "
    "transformation if the object represents an instance.\n"
    "\n"
    "This property is set implicitly by setting the top cell and adding elements to the instantiation path.\n"
    "\n"
    "This method has been added in version 0.16."
  ) + 
  gsi::method_ext ("source_dtrans", &source_dtrans,
    "@brief Gets the transformation applicable for an instance and shape in micron space.\n"
    "\n"
    "This method returns the same transformation than \\source_trans, but applicable to objects in micrometer units:\n"
    "\n"
    "@code\n"
    "# renders the cell instance as seen from top level:\n"
    "dcell_inst_in_top = sel.source_dtrans * sel.inst.dcell_inst\n"
    "@/code\n"
    "\n"
    "The method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("layer", &ip_layer,
    "@brief Gets the layer index that describes which layer the selected shape is on\n"
    "\n"
    "Starting with version 0.27, this method returns nil for this property if \\is_cell_inst? is false - "
    "i.e. the selection does not represent a shape."
  ) + 
  gsi::method ("layer=", &lay::ObjectInstPath::set_layer, gsi::arg ("layer_index"),
    "@brief Sets to the layer index that describes which layer the selected shape is on\n"
    "\n"
    "Setting the layer property to a valid layer index makes the path a shape selection path.\n"
    "Setting the layer property to a negative layer index makes the selection an instance selection.\n"
    "\n"
    "This method has been introduced in version 0.24."
  ) + 
  gsi::method_ext ("shape", &ip_shape,
    "@brief Gets the selected shape\n"
    "\n"
    "The shape object may be modified. This does not have an immediate effect on the selection. Instead, "
    "the selection must be set in the view using \\LayoutView#object_selection= or \\LayoutView#select_object.\n"
    "\n"
    "This method delivers valid results only for object selections that represent shapes. "
    "Starting with version 0.27, this method returns nil for this property if \\is_cell_inst? is false."
  ) +
  gsi::method ("shape=", &lay::ObjectInstPath::set_shape, gsi::arg ("shape"),
    "@brief Sets the shape object that describes the selected shape geometrically\n"
    "\n"
    "When using this setter, the layer index must be set to a valid layout layer (see \\layer=).\n"
    "Setting both properties makes the selection a shape selection.\n"
    "\n"
    "This method has been introduced in version 0.24."
  ) + 
  gsi::method_ext ("inst", &gsi::inst, 
    "@brief Deliver the instance represented by this selection\n"
    "\n"
    "This method delivers valid results only if \\is_cell_inst? is true.\n"
    "It returns the instance reference (an \\Instance object) that this selection represents.\n"
    "\n"
    "This property is set implicitly by adding instance elements to the instantiation path.\n"
    "\n"
    "This method has been added in version 0.16."
  ) +
  gsi::method ("is_cell_inst?", &lay::ObjectInstPath::is_cell_inst, 
    "@brief True, if this selection represents a cell instance\n"
    "\n"
    "If this attribute is true, the shape reference and layer are not valid.\n"
  ) + 
  gsi::method ("seq", &lay::ObjectInstPath::seq, 
    "@brief Gets the sequence number\n"
    "\n"
    "The sequence number describes when the item was selected.\n"
    "A sequence number of 0 indicates that the item was selected in the first selection action (without 'Shift' pressed).\n"
  ) + 
  gsi::method ("seq=", &lay::ObjectInstPath::set_seq, gsi::arg ("n"),
    "@brief Sets the sequence number\n"
    "\n"
    "See \\seq for a description of this property.\n"
    "\n"
    "This method was introduced in version 0.24.\n"
  ) + 
  gsi::method ("clear_path", &lay::ObjectInstPath::clear_path, 
    "@brief Clears the instantiation path\n"
    "\n"
    "This method was introduced in version 0.24.\n"
  ) +
  gsi::method_ext ("path", &get_path,
    "@brief Gets the instantiation path\n"
    "The path is a sequence of \\InstElement objects leading to the target object.\n"
    "\n"
    "This method was introduced in version 0.26.\n"
  ) +
  gsi::method_ext ("path=", &set_path, gsi::arg ("p"),
    "@brief Sets the instantiation path\n"
    "\n"
    "This method was introduced in version 0.26.\n"
  ) +
  gsi::method ("append_path", (void (lay::ObjectInstPath::*) (const db::InstElement &)) &lay::ObjectInstPath::add_path, gsi::arg ("element"),
    "@brief Appends an element to the instantiation path\n"
    "\n"
    "This method allows building of an instantiation path pointing to the selected object.\n"
    "For an instance selection, the last component added is the instance which is selected.\n"
    "For a shape selection, the path points to the cell containing the selected shape.\n"
    "\n"
    "This method was introduced in version 0.24.\n"
  ) +
  gsi::method_ext ("path_length", &gsi::path_length, 
    "@brief Returns the length of the path (number of elements delivered by \\each_inst)\n"
    "\n"
    "This method has been added in version 0.16."
  ) + 
  gsi::method_ext ("path_nth", &gsi::path_nth, gsi::arg ("n"),
    "@brief Returns the nth element of the path (similar to \\each_inst but with direct access through the index)\n"
    "\n"
    "@param n The index of the element to retrieve (0..\\path_length-1)\n"
    "This method has been added in version 0.16."
  ) + 
  gsi::iterator ("each_inst", (lay::ObjectInstPath::iterator (lay::ObjectInstPath::*) () const) &lay::ObjectInstPath::begin, 
                              (lay::ObjectInstPath::iterator (lay::ObjectInstPath::*) () const) &lay::ObjectInstPath::end, 
    "@brief Yields the instantiation path\n"
    "\n"
    "The instantiation path describes by an sequence of \\InstElement objects the path by which the cell containing the selected shape is found from the cell view's current cell.\n"
    "If this object represents an instance, the path will contain the selected instance as the last element.\n"
    "The elements are delivered top down."
  ),
  "@brief A class describing a selected shape or instance\n"
  "\n"
  "A shape or instance is addressed by a path which describes all instances leading to the specified\n"
  "object. These instances are described through \\InstElement objects, which specify the instance and, in case of array instances, the specific array member.\n"
  "For shapes, additionally the layer and the shape itself is specified. The ObjectInstPath objects\n"
  "encapsulates both forms, which can be distinguished with the \\is_cell_inst? predicate.\n"
  "\n"
  "An instantiation path leads from a top cell down to the container cell which either holds the shape or the instance.\n"
  "The top cell can be obtained through the \\top attribute, the container cell through the \\source attribute. Both are cell indexes which can be "
  "converted to \\Cell objects through the \\Layout#cell. In case of objects located in the top cell, \\top and \\source refer to the same cell.\n"
  "The first element of the instantiation path is the instance located within the top cell leading to the first child cell. The second element leads to the "
  "next child cell and so forth. \\path_nth can be used to obtain a specific element of the path.\n"
  "\n"
  "The \\cv_index attribute specifies the cellview the selection applies to. Use "
  "\\LayoutView#cellview to obtain the \\CellView object from the index.\n"
  "\n"
  "The shape or instance the selection refers to can be obtained with \\shape and \\inst respectively. Use "
  "\\is_cell_inst? to decide whether the selection refers to an instance or not.\n"
  "\n"
  "The ObjectInstPath class plays a role when retrieving and modifying the selection of shapes and instances through \\LayoutView#object_selection, \\LayoutView#object_selection=, \\LayoutView#select_object and \\LayoutView#unselect_object. \\ObjectInstPath objects "
  "can be modified to reflect a new selection, but the new selection becomes active only after it is installed "
  "in the view. The following sample demonstrates that. It implements a function to convert all shapes "
  "to polygons:\n"
  "\n"
  "@code\n"
  "mw = RBA::Application::instance::main_window\n"
  "view = mw.current_view\n"
  "\n"
  "begin\n"
  "\n"
  "  view.transaction(\"Convert selected shapes to polygons\")\n"
  "\n"
  "  sel = view.object_selection\n"
  "\n"
  "  sel.each do |s|\n"
  "    if !s.is_cell_inst? && !s.shape.is_text?\n"
  "      ly = view.cellview(s.cv_index).layout\n"
  "      # convert to polygon\n"
  "      s.shape.polygon = s.shape.polygon\n"
  "    end\n"
  "  end\n"
  "  \n"
  "  view.object_selection = sel\n"
  "\n"
  "ensure\n"
  "  view.commit\n"
  "end\n"
  "@/code\n"
  "\n"
  "Note, that without resetting the selection in the above example, the application might raise errors because "
  "after modifying the selected objects, the current selection will no longer be valid. Establishing a new valid selection "
  "in the way shown above will help avoiding this issue.\n"
);

class EditableSelectionIterator 
{
public:
  typedef edt::Service::objects::value_type value_type;
  typedef edt::Service::objects::const_iterator iterator_type;
  typedef void pointer; 
  typedef const value_type &reference;
  typedef std::forward_iterator_tag iterator_category;
  typedef void difference_type;

  EditableSelectionIterator (const std::vector<edt::Service *> &services, bool transient) 
    : m_services (services), m_service (0), m_transient_selection (transient)
  {
    if (! m_services.empty ()) {
      if (m_transient_selection) {
        m_iter = m_services [m_service]->transient_selection ().begin ();
        m_end = m_services [m_service]->transient_selection ().end ();
      } else {
        m_iter = m_services [m_service]->selection ().begin ();
        m_end = m_services [m_service]->selection ().end ();
      }
      next ();
    }
  }

  bool at_end () const
  {
    return (m_service >= m_services.size ());
  }

  EditableSelectionIterator &operator++ ()
  {
    ++m_iter;
    next ();
    return *this;
  }

  const value_type &operator* () const
  {
    return *m_iter;
  }

private:
  std::vector<edt::Service *> m_services;
  unsigned int m_service;
  bool m_transient_selection;
  iterator_type m_iter, m_end;

  void next ()
  {
    while (m_iter == m_end) {
      ++m_service;
      if (m_service < m_services.size ()) {
        if (m_transient_selection) {
          m_iter = m_services [m_service]->transient_selection ().begin ();
          m_end = m_services [m_service]->transient_selection ().end ();
        } else {
          m_iter = m_services [m_service]->selection ().begin ();
          m_end = m_services [m_service]->selection ().end ();
        }
      } else {
        break;
      }
    }
  }
};

//  extend the layout view by "edtService" specific methods 

static std::vector<edt::Service::objects::value_type> object_selection (const lay::LayoutViewBase *view)
{
  std::vector<edt::Service::objects::value_type> result;
  std::vector<edt::Service *> edt_services = view->get_plugins <edt::Service> ();
  for (std::vector<edt::Service *>::const_iterator s = edt_services.begin (); s != edt_services.end (); ++s) {
    std::vector<edt::Service::objects::value_type> sel;
    (*s)->get_selection (sel);
    result.insert (result.end (), sel.begin (), sel.end ());
  }
  return result;
}

static void set_object_selection (const lay::LayoutViewBase *view, const std::vector<edt::Service::objects::value_type> &all_selected)
{
  std::vector<edt::Service::objects::value_type> sel;

  std::vector<edt::Service *> edt_services = view->get_plugins <edt::Service> ();
  for (std::vector<edt::Service *>::const_iterator s = edt_services.begin (); s != edt_services.end (); ++s) {

    sel.clear ();

    for (std::vector<edt::Service::objects::value_type>::const_iterator o = all_selected.begin (); o != all_selected.end (); ++o) {
      if ((*s)->selection_applies (*o)) {
        sel.push_back (*o);
      }
    }

    (*s)->set_selection (sel.begin (), sel.end ());

  }
}

static bool has_object_selection (const lay::LayoutViewBase *view)
{
  std::vector<edt::Service *> edt_services = view->get_plugins <edt::Service> ();
  for (std::vector<edt::Service *>::const_iterator s = edt_services.begin (); s != edt_services.end (); ++s) {
    if ((*s)->has_selection ()) {
      return true;
    }
  }
  return false;
}

static void clear_object_selection (const lay::LayoutViewBase *view)
{
  std::vector<edt::Service *> edt_services = view->get_plugins <edt::Service> ();
  for (std::vector<edt::Service *>::const_iterator s = edt_services.begin (); s != edt_services.end (); ++s) {
    (*s)->clear_selection ();
  }
}

static void select_object (const lay::LayoutViewBase *view, const edt::Service::objects::value_type &object)
{
  std::vector<edt::Service *> edt_services = view->get_plugins <edt::Service> ();
  for (std::vector<edt::Service *>::const_iterator s = edt_services.begin (); s != edt_services.end (); ++s) {
    if ((*s)->selection_applies (object)) {
      (*s)->add_selection (object);
      break;
    }
  }
}

static void unselect_object (const lay::LayoutViewBase *view, const edt::Service::objects::value_type &object)
{
  std::vector<edt::Service *> edt_services = view->get_plugins <edt::Service> ();
  for (std::vector<edt::Service *>::const_iterator s = edt_services.begin (); s != edt_services.end (); ++s) {
    if ((*s)->selection_applies (object)) {
      (*s)->remove_selection (object);
      break;
    }
  }
}

static bool has_transient_object_selection (const lay::LayoutViewBase *view)
{
  std::vector<edt::Service *> edt_services = view->get_plugins <edt::Service> ();
  for (std::vector<edt::Service *>::const_iterator s = edt_services.begin (); s != edt_services.end (); ++s) {
    if ((*s)->has_transient_selection ()) {
      return true;
    }
  }
  return false;
}

static EditableSelectionIterator begin_objects_selected (const lay::LayoutViewBase *view)
{
  return EditableSelectionIterator (view->get_plugins <edt::Service> (), false);
}

static EditableSelectionIterator begin_objects_selected_transient (const lay::LayoutViewBase *view)
{
  return EditableSelectionIterator (view->get_plugins <edt::Service> (), true);
}

static
gsi::ClassExt<lay::LayoutViewBase> layout_view_decl (
  gsi::method_ext ("has_object_selection?", &has_object_selection, 
    "@brief Returns true, if geometrical objects (shapes or cell instances) are selected in this view"
  ) +
  gsi::method_ext ("object_selection", &object_selection, 
    "@brief Returns a list of selected objects\n"
    "This method will deliver an array of \\ObjectInstPath objects listing the selected geometrical "
    "objects. Other selected objects such as annotations and images will not be contained in that "
    "list.\n"
    "\n"
    "The list returned is an array of copies of \\ObjectInstPath objects. They can be modified, but "
    "they will become a new selection only after re-introducing them into the view through \\object_selection= or "
    "\\select_object.\n"
    "\n"
    "Another way of obtaining the selected objects is \\each_object_selected.\n"
    "\n"
    "This method has been introduced in version 0.24.\n"
  ) +
  gsi::method_ext ("object_selection=", &set_object_selection, gsi::arg ("sel"),
    "@brief Sets the list of selected objects\n"
    "\n"
    "This method will set the selection of geometrical objects such as shapes and instances. "
    "It is the setter which complements the \\object_selection method.\n"
    "\n"
    "Another way of setting the selection is through \\clear_object_selection and \\select_object.\n"
    "\n"
    "This method has been introduced in version 0.24.\n"
  ) +
  gsi::method_ext ("clear_object_selection", &clear_object_selection,
    "@brief Clears the selection of geometrical objects (shapes or cell instances)\n"
    "The selection of other objects (such as annotations and images) will not be affected.\n"
    "\n"
    "This method has been introduced in version 0.24\n"
  ) +
  gsi::method_ext ("select_object", &select_object, gsi::arg ("obj"),
    "@brief Adds the given selection to the list of selected objects\n"
    "\n"
    "The selection provided by the \\ObjectInstPath descriptor is added to the list of selected objects.\n"
    "To clear the previous selection, use \\clear_object_selection.\n"
    "\n"
    "The selection of other objects (such as annotations and images) will not be affected.\n"
    "\n"
    "Another way of selecting objects is \\object_selection=.\n"
    "\n"
    "This method has been introduced in version 0.24\n"
  ) +
  gsi::method_ext ("unselect_object", &unselect_object, gsi::arg ("obj"),
    "@brief Removes the given selection from the list of selected objects\n"
    "\n"
    "The selection provided by the \\ObjectInstPath descriptor is removed from the list of selected objects.\n"
    "If the given object was not part of the selection, nothing will be changed.\n"
    "The selection of other objects (such as annotations and images) will not be affected.\n"
    "\n"
    "This method has been introduced in version 0.24\n"
  ) +
  gsi::iterator_ext ("each_object_selected", &begin_objects_selected,
    "@brief Iterates over each selected geometrical object, yielding a \\ObjectInstPath object for each of them\n"
    "\n"
    "This iterator will deliver const objects - they cannot be modified. In order to modify the selection, "
    "create a copy of the \\ObjectInstPath objects, modify them and install the new selection using \\select_object or "
    "\\object_selection=.\n"
    "\n"
    "Another way of obtaining the selection is \\object_selection, which returns an array of \\ObjectInstPath objects.\n"
  ) +
  gsi::method_ext ("has_transient_object_selection?", &has_transient_object_selection,  
    "@brief Returns true, if geometrical objects (shapes or cell instances) are selected in this view in the transient selection\n"
    "\n"
    "The transient selection represents the objects selected when the mouse hovers over the "
    "layout windows. This selection is not used for operations but rather to indicate which object would "
    "be selected if the mouse is clicked.\n"
    "\n"
    "This method was introduced in version 0.18."
  ) +
  gsi::iterator_ext ("each_object_selected_transient", &begin_objects_selected_transient,
    "@brief Iterates over each geometrical objects in the transient selection, yielding a \\ObjectInstPath object for each of them\n"
    "\n"
    "This method was introduced in version 0.18."
  ),
  ""
);

}

