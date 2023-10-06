
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



#ifndef HDR_edtService
#define HDR_edtService

#include "edtCommon.h"

#include "layEditorServiceBase.h"
#include "layPlugin.h"
#include "layMarker.h"
#include "laySnap.h"
#include "layObjectInstPath.h"
#include "layTextInfo.h"
#include "tlColor.h"
#include "dbLayout.h"
#include "dbShape.h"
#include "edtUtils.h"
#include "edtConfig.h"
#include "tlAssert.h"
#include "tlException.h"

#include <set>
#include <vector>

namespace lay {
  class LayerPropertiesConstIterator;
}

namespace edt {

class Service;
class PluginDeclarationBase;

// -------------------------------------------------------------

extern lay::angle_constraint_type ac_from_buttons (unsigned int buttons);

// -------------------------------------------------------------

/**
 *  @brief Utility function: serialize PCell parameters into a string
 */
std::string pcell_parameters_to_string (const std::map<std::string, tl::Variant> &parameters);

/**
 *  @brief Utility: deserialize PCell parameters from a string
 */
std::map<std::string, tl::Variant> pcell_parameters_from_string (const std::string &s);

// -------------------------------------------------------------

class EDT_PUBLIC Service
  : public lay::EditorServiceBase,
    public db::Object
{
public: 
  enum SnapMode { Any = 0, Diagonal, Ortho, Horizontal, Vertical, NumSnapModes };

  typedef std::set<lay::ObjectInstPath> objects;
  typedef objects::const_iterator obj_iterator;

  /**
   *  @brief The constructor for an service selecting shapes
   */
  Service (db::Manager *manager, lay::LayoutViewBase *view, db::ShapeIterator::flags_type shape_types);

  /**
   *  @brief The constructor for an service selecting instances
   */
  Service (db::Manager *manager, lay::LayoutViewBase *view);

  /**
   *  @brief The destructor
   */
  ~Service ();

  /** 
   *  @brief Clear all highlights (for current object highlighting)
   */
  void clear_highlights ();

  /** 
   *  @brief Restore all highlights (for current object highlighting)
   */
  void restore_highlights ();

  /**
   *  @brief Highlights a group of objects
   */
  void highlight (const std::vector<size_t> &n);

  /** 
   *  @brief "delete" operation
   */
  virtual void del ();

  /**
   *  @brief Deliver the selection's bbox (reimplementation of lay::Editable interface)
   */
  virtual db::DBox selection_bbox ();
 
  /** 
   *  @brief "transform" operation
   */
  virtual void transform (const db::DCplxTrans &tr)
  {
    transform (tr, 0);
  }

  /** 
   *  @brief "cut" operation
   */
  virtual void cut ();

  /** 
   *  @brief "copy" operation
   */
  virtual void copy ();

  /**
   *  @brief Begin a "move" operation
   */
  virtual bool begin_move (lay::Editable::MoveMode mode, const db::DPoint &p, lay::angle_constraint_type ac);

  /**
   *  @brief Continue a "move" operation
   */
  virtual void move (const db::DPoint &p, lay::angle_constraint_type ac);

  /**
   *  @brief Transform during a move operation
   */
  virtual void move_transform (const db::DPoint &p, db::DFTrans tr, lay::angle_constraint_type ac);

  /**
   *  @brief Terminate a "move" operation
   */
  virtual void end_move (const db::DPoint &p, lay::angle_constraint_type ac);

  /**
   *  @brief Indicates whether objects are selected
   */
  virtual bool has_selection ();

  /**
   *  @brief Indicates how many objects are selected
   */
  virtual size_t selection_size ();

  /**
   *  @brief Indicates whether objects are selected in transient mode
   */
  virtual bool has_transient_selection ();

  /** 
   *  @brief "select" operation
   */
  virtual double click_proximity (const db::DPoint &pos, lay::Editable::SelectionMode mode);

  /**
   *  @brief Gets the catch distance (for single click)
   */
  virtual double catch_distance ();

  /**
   *  @brief Gets the catch distance (for box)
   */
  virtual double catch_distance_box ();

  /**
   *  @brief "select" operation
   */
  virtual bool select (const db::DBox &box, lay::Editable::SelectionMode mode);

  /** 
   *  @brief Returns true, if the given selected object is handled by this service
   */
  virtual bool selection_applies (const lay::ObjectInstPath &sel) const;

  /**
   *  @brief Get the selection for the properties page
   */
  void get_selection (std::vector <lay::ObjectInstPath> &selection) const;

  /** 
   *  @brief "transform" operation with a transformation vector
   *
   *  This version of the transformation operation allows one to specify a transformation per selected object.
   *  The transformations in the vector must be ordered in the order the selection is delivered by 
   *  the selection iterator.
   *  Either global or per-object transformations can be specified. If a per-object transformation is
   *  specified, the global transformation is ignored.
   *
   *  @param tr The global transformation (ignored if p_trv != 0)
   *  @param p_trv The per-object transformation (can be 0 for one global transformation)
   */
  void transform (const db::DCplxTrans &tr, const std::vector<db::DCplxTrans> *p_trv);

  /**
   *  @brief Color accessor
   */
  tl::Color color () const
  {
    return m_color;
  }

  /**
   *  @brief Get the selection container
   */
  const objects &selection () const
  {
    return m_selection;
  }

  /**
   *  @brief Get the transient selection container
   */
  const objects &transient_selection () const
  {
    return m_transient_selection;
  }

  /**
   *  @brief Access to the view object
   */
  lay::LayoutViewBase *view () const
  {
    tl_assert (mp_view != 0);
    return mp_view;
  }

  /**
   *  @brief Select a certain object
   *
   *  @return true, if the selection has changed
   */
  bool select (const lay::ObjectInstPath &obj, lay::Editable::SelectionMode mode);

  /**
   *  @brief Clears the previous selection
   */
  void clear_previous_selection ();

  /**
   *  @brief Establish a transient selection
   */
  bool transient_select (const db::DPoint &pos);

  /**
   *  @brief Turns the transient selection to the selection
   */
  virtual void transient_to_selection ();

  /**
   *  @brief Clear the transient selection
   */
  void clear_transient_selection ();

  /**
   *  @brief Clear the selection
   *
   *  This is a convenience method.
   */
  void clear_selection ()
  {
    select (db::DBox (), lay::Editable::Reset);
  }

  /**
   *  @brief Set the selection
   */
  void set_selection (std::vector <lay::ObjectInstPath>::const_iterator s1, std::vector <lay::ObjectInstPath>::const_iterator s2);

  /** 
   *  @brief Add one element to the selection
   */
  void add_selection (const lay::ObjectInstPath &sel);

  /** 
   *  @brief Removes one element from the selection
   */
  void remove_selection (const lay::ObjectInstPath &sel);

  /**
   *  @brief Implement the mouse mode: move event
   */
  virtual bool mouse_move_event (const db::DPoint &p, unsigned int buttons, bool prio);

  /**
   *  @brief Implement the mouse mode: button press event
   */
  virtual bool mouse_press_event (const db::DPoint &p, unsigned int buttons, bool prio);

  /**
   *  @brief Implement the mouse mode: button clicked (pressed and released)
   */
  virtual bool mouse_click_event (const db::DPoint &p, unsigned int buttons, bool prio);

  /**
   *  @brief Implement the mouse mode: button double clicked 
   */
  virtual bool mouse_double_click_event (const db::DPoint &p, unsigned int buttons, bool prio);

  /**
   *  @brief Implements the key handler
   */
  virtual bool key_event (unsigned int /*key*/, unsigned int /*buttons*/);

  /**
   *  @brief Implement the mouse mode: deactivate mouse mode
   */
  virtual void deactivated ();

  /**
   *  @brief Implement the mouse mode: mode activated
   */
  virtual void activated ();

  /**
   *  @brief Cancel any edit operations (such as move)
   */
  virtual void edit_cancel ();

  /**
   *  @brief Triggered by tap - gives the new layer and if required the initial point
   */
  virtual void tap (const db::DPoint &initial);

  /**
   *  @brief Delete the selected rulers
   *
   *  Used as implementation for "del" and "cut"
   *  Warning: this is a low-level method that does not provide undo etc.
   */
  void del_selected ();

  /**
   *  @brief Get the type of objects managed by this service
   */
  db::ShapeIterator::flags_type flags () const
  {
    return m_flags;
  }

  /**
   *  @brief Handle changes in the guiding shapes, i.e. create PCell variants
   *
   *  @return true, if PCells have been updated, indicating that our selection is no longer valid
   *
   *  This version assumes there is only one guiding shape selected and will update the selection.
   *  It will also call layout.cleanup() if required.
   */
  bool handle_guiding_shape_changes ();

  /**
   *  @brief Handle changes in a specific guiding shape, i.e. create new PCell variants if required
   *
   *  @return A pair of bool (indicating that the object path has changed) and the new guiding shape path
   */
  std::pair<bool, lay::ObjectInstPath> handle_guiding_shape_changes (const lay::ObjectInstPath &obj) const;

protected:
  /**
   *  @brief Update m_markers to reflect the selection
   */
  void selection_to_view ();

  /**
   *  @brief starts editing at the given point.
   */
  void begin_edit (const db::DPoint &p);

  /**
   *  @brief Reimplemented by the specific implementation of the shape editors
   *
   *  This method is called when the mouse mode is activated.
   *  If the edit operation shall begin immediately, this method is supposed to return true.
   */
  virtual bool do_activated () { return false; }

  /**
   *  @brief Reimplemented by the specific implementation of the shape editors
   *
   *  This method is called when the object is started to be edited
   */
  virtual void do_begin_edit (const db::DPoint & /*p*/) { }

  /**
   *  @brief Reimplemented by the specific implementation of the shape editors
   *
   *  This method is called when the mouse is moved
   */
  virtual void do_mouse_move (const db::DPoint & /*p*/) { }

  /**
   *  @brief Reimplemented by the specific implementation of the shape editors
   *
   *  This method is called when the mouse is moved but editing has not started yet
   */
  virtual void do_mouse_move_inactive (const db::DPoint & /*p*/) { }

  /**
   *  @brief Reimplemented by the specific implementation of the shape editors
   *
   *  Transform the current edited object by the given fixpoint transformation
   */
  virtual void do_mouse_transform (const db::DPoint & /*p*/, db::DFTrans /*trans*/) { }

  /**
   *  @brief Reimplemented by the specific implementation of the shape editors
   *
   *  This method is called when the mouse button is clicked
   *  This method is supposed to return true, if the editing should be finished.
   */
  virtual bool do_mouse_click (const db::DPoint & /*p*/) { return false; }

  /**
   *  @brief Reimplemented by the specific implementation of the shape editors
   *
   *  This method is called when the backspace button is pressed
   */
  virtual void do_delete () { }

  /**
   *  @brief Reimplemented by the specific implementation of the shape editors
   *
   *  This method is called when the object is finished
   */
  virtual void do_finish_edit () { }

  /**
   *  @brief Reimplemented by the specific implementation of the shape editors
   *
   *  This method is called when the edit operation should be cancelled
   */
  virtual void do_cancel_edit () { }

  /**
   *  @brief Called when a configuration parameter provided by the service base class has changed
   */
  virtual void service_configuration_changed ();

  /**
   *  @brief Install a marker for representing the edited object
   *
   *  The ownership over the marker is transferred to the service object.
   *  Setting the edit marker to 0 clears all edit markers.
   */
  void set_edit_marker (lay::ViewObject *edit_marker);

  /**
   *  @brief Add a new edit marker
   */
  void add_edit_marker (lay::ViewObject *edit_marker);

  /**
   *  @brief Return the marker object for representing the edited object
   *
   *  Returns the first marker of 0 if no marker is present
   */
  lay::ViewObject *edit_marker ();

  /**
   *  @brief Receive configuration parameters
   */
  bool configure (const std::string &name, const std::string &value);

  /**
   *  @brief Snap a point to the edit grid
   *
   *  @param p The point to snap
   */
  db::DPoint snap (db::DPoint p) const;

  /**
   *  @brief Snap a vector to the edit grid
   *
   *  @param v The vector to snap
   */
  db::DVector snap (db::DVector v) const;

  /**
   *  @brief Snap a point to the edit grid with an angle constraint 
   *
   *  @param p The point to snap
   *  @param plast The last point of the connection/move vector
   *  @param connect true, if the point is an connection vertex, false if it is a move target point
   */
  db::DPoint snap (const db::DPoint &p, const db::DPoint &plast, bool connect) const;

  /**
   *  @brief Snap a move vector to the edit grid with an angle constraint
   *
   *  @param v The vector to snap
   *  @param connect true, if the point is an connection vertex, false if it is a move target point
   */
  db::DVector snap (const db::DVector &v, bool connect) const;

  /**
   *  @brief Proposes a grid-snapped displacement vector
   *
   *  @param v The input displacement
   *  @return A displacement that pushes the (current) markers on-grid, definition depending on marker
   */
  db::DVector snap_marker_to_grid (const db::DVector &v, bool &snapped) const;

  /**
   *  @brief Snap a point to the edit grid with advanced snapping (including object snapping)
   *
   *  @param p The point to snap
   *  @param connect true, if the point is an connection vertex, false if it is a move target point
   */
  db::DPoint snap2 (const db::DPoint &p) const;

  /**
   *  @brief Snap a point to the edit grid with an angle constraint  with advanced snapping (including object snapping)
   *
   *  @param p The point to snap
   *  @param plast The last point of the connection/move vector
   *  @param connect true, if the point is an connection vertex, false if it is a move target point
   */
  db::DPoint snap2 (const db::DPoint &p, const db::DPoint &plast, bool connect = true) const;

protected:
  lay::angle_constraint_type connect_ac () const;
  lay::angle_constraint_type move_ac () const;

  bool show_shapes_of_instances () const
  {
    return m_show_shapes_of_instances;
  }

  unsigned int max_shapes_of_instances () const
  {
    return m_max_shapes_of_instances;
  }

  bool editing () const
  {
    return m_editing;
  }

  /**
   *  @brief Point snapping with detailed return value
   */
  lay::PointSnapToObjectResult snap2_details (const db::DPoint &p) const;

private:
  //  The layout view that the editor service is attached to
  lay::LayoutViewBase *mp_view;

  //  The marker objects representing the selection
  std::vector<lay::ViewObject *> m_markers;

  //  Marker for the transient selection
  lay::ViewObject *mp_transient_marker;

  //  The marker representing the object to be edited
  std::vector<lay::ViewObject *> m_edit_markers;

  //  True, if editing is in progress.
  bool m_editing;

  //  True, if on the first mouse move an immediate do_begin_edit should be issued.
  bool m_immediate;

  //  The selection
  objects m_selection;

  //  The previous selection (used for cycling through different selections for single clicks)
  objects m_previous_selection;

  //  The transient selection
  objects m_transient_selection;

  //  True, if this service deals with cell instances
  bool m_cell_inst_service;

  //  The shape types to select
  db::ShapeIterator::flags_type m_flags;

  //  The look of the markers
  tl::Color m_color;

  //  The current transformation on movement
  db::DTrans m_move_trans;
  db::DPoint m_move_start;
  bool m_move_sel, m_moving;

  //  Angle constraints and grids
  lay::angle_constraint_type m_connect_ac, m_move_ac, m_alt_ac;
  db::DVector m_edit_grid;
  bool m_snap_to_objects;
  bool m_snap_objects_to_grid;
  db::DVector m_global_grid;
  bool m_top_level_sel;
  bool m_show_shapes_of_instances;
  unsigned int m_max_shapes_of_instances;

  //  Hierarchical copy mode (-1: ask, 0: shallow, 1: deep)
  int m_hier_copy_mode;

  //  Sequence number of selection
  bool m_indicate_secondary_selection;
  unsigned long m_seq;

  //  selective highlights
  bool m_highlights_selected;
  std::set<size_t> m_selected_highlights;

  //  Deferred method to update the selection
  tl::DeferredMethod<edt::Service> dm_selection_to_view;

  /**
   *  @brief Update m_markers to reflect the selection (called through deferred method)
   */
  void do_selection_to_view ();

  /**
   *  @brief Update m_markers with the new transformation to reflect the movement
   */
  void move_markers (const db::DTrans &t);

  /**
   *  @brief Copy the selected rulers to the clipboard
   *
   *  Used as implementation for "copy" and "cut"
   */
  void copy_selected ();

  /**
   *  @brief Cancel the move operation
   */
  void move_cancel ();

  /**
   *  @brief Display the status bar message for the given selection
   */
  void display_status (bool transient);

  /**
   *  @brief Apply highlight selection
   */
  void apply_highlights ();

private:
  void copy_selected (unsigned int inst_mode);
  void update_vector_snapped_point (const db::DPoint &pt, db::DVector &vr, bool &result_set) const;
  void update_vector_snapped_marker (const lay::ShapeMarker *sm, const db::DTrans &trans, db::DVector &vr, bool &result_set, size_t &count) const;
  void update_vector_snapped_marker (const lay::InstanceMarker *sm, const db::DTrans &trans, db::DVector &vr, bool &result_set, size_t &count) const;
};

}

#endif

