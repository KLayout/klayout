
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


#ifndef HDR_layEditable
#define HDR_layEditable

#include "laybasicCommon.h"

#include "tlObjectCollection.h"
#include "laySnap.h"
#include "dbTrans.h"
#include "dbPoint.h"
#include "dbBox.h"
#include "dbObject.h"
#include "dbManager.h"

#include <set>
#include <limits>

#if defined(HAVE_QT)
class QWidget;
#endif

namespace lay
{

class Editables;
#if defined(HAVE_QT)
class PropertiesPage;
#endif

/**
 *  @brief The "editable" interface
 *
 *  An "editable" object is providing an interface with the 
 *  common editing operations like "delete", "copy", "select" etc.
 *  The "editable" objects are maintained in an lay::Editables collection.
 */
class LAYBASIC_PUBLIC Editable
  : virtual public tl::Object
{
public:
  enum SelectionMode { Replace = 0, Reset, Add, Invert };
  enum MoveMode { Any = 0, Selected, Partial };

  /**
   *  @brief The constructor
   *
   *  @param editables The collection in which to insert the 
   *                   object. Can be 0 for not inserting it somewhere.
   */
  Editable (Editables *editables = 0);

  /**
   *  @brief The constructor
   */
  virtual ~Editable ();

  /**
   *  @brief "delete" operation
   */
  virtual void del ()
  {
    //  .. by default, nothing is implemented ..
  }

  /**
   *  @brief "cut" operation
   */
  virtual void cut ()
  {
    //  .. by default, nothing is implemented ..
  }

  /**
   *  @brief "copy" operation
   */
  virtual void copy ()
  {
    //  .. by default, nothing is implemented ..
  }

  /**
   *  @brief Deliver the bbox of the selection
   *
   *  This bounding box is used to compute the center of the 
   *  selection for transformation by an angle (it is better
   *  to do this around der center of the selection bbox).
   *  The bbox must either be empty (if no selection is present)
   *  or must be given in micron units.
   */
  virtual db::DBox selection_bbox ()
  {
    return db::DBox ();
  }
  
  /**
   *  @brief transform the selection
   *
   *  The transformation is given in micron units.
   */
  virtual void transform (const db::DCplxTrans & /*tr*/)
  {
    //  .. by default, nothing is implemented ..
  }

  /**
   *  @brief "paste" operation
   */
  virtual void paste ()
  {
    //  .. by default, nothing is implemented ..
  }

  /**
   *  @brief "point selection proximity" predicate
   *
   *  The point selection proximity is a method to determine which
   *  object should be selected by point selection. The point selection
   *  proximity is a typical distance value of the point to the closest
   *  object in micron. The plugin with the least point selection proximity
   *  is selected first for the actual "select" operation.
   *  If a plugin definitely wants to get the selection, it should return
   *  a negative value. If the plugin is only weakly interested in a selection,
   *  it should return the value provided by the default implementation.
   *  The click_proximity method can be used to implement cycling through
   *  several objects on the same location by delivering the proximity for 
   *  "new" objects only by remembering the objects already selected.
   *  If the client of the plugin finds no plugin that has anything to select,
   *  it will try to reset the selected and do a new scan over all plugins.
   *
   *  @param pos The point at which to select in micron space.
   *  @param mode The mode of the selection for which the closest object is looked for.
   *  @return The distance in micron
   */
  virtual double click_proximity (const db::DPoint & /*pos*/, SelectionMode /*mode*/)
  {
    return std::numeric_limits<double>::max ();
  }

  /**
   *  @brief The catch distance (for single click)
   *
   *  The catch distance is a typical value for the "fuzzyness" of a mouse click.
   *  It is given in micron.
   */
  virtual double catch_distance ()
  {
    return 0.0;
  }

  /**
   *  @brief The catch distance (for box)
   *
   *  The catch distance is a typical value for the "fuzzyness" of a box selection.
   *  It is given in micron.
   */
  virtual double catch_distance_box ()
  {
    return 0.0;
  }

  /**
   *  @brief transient selection
   *
   *  The transient selection is triggered when the mouse does not move for
   *  some time interval ("hover mode"). 
   *  This method is supposed to create a transient selection on a single object
   *  at the given point.
   *
   *  @param point The point at which to select in micron space.
   *  @return Should return true if anything was selected
   */
  virtual bool transient_select (const db::DPoint & /*pos*/)
  {
    return false;
  }

  /**
   *  @brief Turns the transient selection to the selection
   */
  virtual void transient_to_selection ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Clear the transient selection
   *
   *  This method is supposed to reset any transient selection
   */
  virtual void clear_transient_selection ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Clears the previous selection state
   *
   *  This method is used by the single-point selection cycling protocol to clear the 
   *  plugin's single-point selection state. The cycling protocol is used when a certain
   *  point is clicked at multiple times. A plugin is supposed to remember such selections and
   *  exclude them from further checks. If all objects in question are selected, no further 
   *  object would be selected. clear_previous_selection is called in that case to indicate that
   *  the previous selection should be cleared and a new cycle is about to begin
   */
  virtual void clear_previous_selection ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief "select" operation
   *
   *  This is geometrical selection of objects. Selection can be
   *  either by a box or by a point. In the latter case, a degenerated
   *  box must be provided which is a single-point box. 
   *  An empty box equals "all".
   *  The return value is basically used by the point selection mode 
   *  to determine the first editable that gets the selection.
   *
   *  @param box The box within which to select in micron space.
   *  @param mode The selection mode
   *  @return Should return true if anything was selected, in particular in point mode
   */
  virtual bool select (const db::DBox & /*box*/, SelectionMode /*mode*/)
  {
    return false;
  }

  /**
   *  @brief Start a "move" operation
   *
   *  The move operation can be requested in three modes: In "Any" mode, the plugin
   *  can decide whether it wants to catch a move request and return true.
   *  In "Selected" mode, the plugin is supposed to move all selected objects.
   *  In "Partial" mode, the plugin is supposed to pick an object from the selected
   *  ones and perform a move that modifies that object. In "Any" or "Selected" mode, 
   *  the plugin must return true to catch the selection. Only one plugin will receive
   *  the selection.
   *
   *  All move operations start from point p.
   *    
   *  A move operation is continued by "move" events and terminated
   *  by either a "cancel" or a "end_move" event.
   *
   *  The basic algorithm used for the move implementation is this:
   *    1.) if anything is selected, call begin_move on every Editable with sel=Partial.
   *        If a plugin returns true, the method returns.
   *    2.) else if anything is selected, call begin_move on every Editable with sel=Selected. 
   *    3.) if nothing is selected, call begin_move on every Editable with sel=Any. Stop if one of
   *        these returns true.
   *    4.) if none returned true, select pointwise around "p" and proceed as in 2.)
   *  
   *  @param sel See above.
   *  @param p The point at which the mouse was clicked
   *  @param ac The angle constraint imposed (lay::AC_Global if no specific is requested)
   *  @return See above.
   */
  virtual bool begin_move (MoveMode /*sel*/, const db::DPoint & /*p*/, lay::angle_constraint_type /*ac*/)
  {
    return false;
  }

  /**
   *  @brief Continue a "move" operation
   *
   *  These events are sent whenever the mouse is moved.
   *  
   *  @param p The current mouse location
   *  @param ac The angle constraint imposed (lay::AC_Global if no specific is requested)
   */
  virtual void move (const db::DPoint & /*p*/, lay::angle_constraint_type /*ac*/)
  {
    //  .. by default, nothing is implemented ..
  }

  /**
   *  @brief Transform the moved object/set while in a move operation
   *
   *  These events are sent whenever the right mouse button is clicked in order to request a 
   *  rotation of the content currently moved.
   *
   *  @param p The current point
   *  @param tr The transformation to apply
   *  @param ac The angle constraint imposed (lay::AC_Global if no specific is requested)
   */
  virtual void move_transform (const db::DPoint & /*p*/, db::DFTrans /*tr*/, lay::angle_constraint_type /*ac*/)
  {
    //  .. by default, nothing is implemented ..
  }

  /**
   *  @brief Terminate a "move" operation
   *
   *  @param p The last mouse location
   *  @param ac The angle constraint imposed (lay::AC_Global if no specific is requested)
   */
  virtual void end_move (const db::DPoint & /*p*/, lay::angle_constraint_type  /*ac*/)
  {
    //  .. by default, nothing is implemented ..
  }

  /**
   *  @brief Cancel any pending operations
   *
   *  This event is sent whenever a pending operation such as 
   *  a move operation should be canceled.
   */
  virtual void edit_cancel ()
  {
    //  .. by default, nothing is implemented ..
  }

  /**
   *  @brief Indicates if any objects are selected
   */
  virtual bool has_selection ()
  {
    return false;
  }

  /**
   *  @brief Indicates how many objects are selected
   */
  virtual size_t selection_size ()
  {
    return 0;
  }

  /**
   *  @brief Indicates if any objects are selected in the transient selection
   */
  virtual bool has_transient_selection ()
  {
    return false;
  }

#if defined(HAVE_QT)
  /**
   *  @brief Create a "properties page" object
   *
   *  A "properties page" object is first a Qt widget (a QFrame)
   *  that is acting as a proxy to a certain selected editable object.
   *  It acts as the communication link between the Editable object and
   *  the properties dialog by displaying and applying properties.
   *  The object returned by this method is newed and must be deleted 
   *  by the caller. The return value is 0 if the Editable object does
   *  not support a properties page.
   */
  virtual std::vector<lay::PropertiesPage *> properties_pages (db::Manager * /*manager*/, QWidget * /*parent*/)
  {
    return std::vector<lay::PropertiesPage *> ();
  }
#endif

  /**
   *  @brief Destruction callback by the properties page
   *
   *  The properties page object calls this method if it is destroyed
   *  on the Editable object that it was issued from.
   */
  virtual void properties_page_deleted () 
  {
    //  .. nothing yet.
  }

protected:
  Editables *editables ()
  {
    return mp_editables;
  }

private:
  Editables *mp_editables;
};

/**
 *  @brief The "editable" collection
 *
 *  This class provides a common interface to a collection of "editables".
 *  If a pointer to this object is passed to the constructor of the
 *  editable object, the latter is automatically inserted into this
 *  collection.
 *  In addition to managing the editable objects, a selection mechanism
 *  is provided: by enabling or disabling certain editable objects, the
 *  "select" requests are forwarded to a subset of the editable objects.
 */
class LAYBASIC_PUBLIC Editables
  : public db::Object
{
public:
  typedef tl::shared_collection<lay::Editable>::iterator iterator;

  /**
   *  @brief The constructor
   */
  Editables (db::Manager *manager = 0);

  /**
   *  @brief The destructor
   */
  virtual ~Editables ();

  /**
   *  @brief The delete operation
   *
   *  If a transaction is given, the operation will be appended to this pending transaction
   *  The Editables object takes ownership over the Transaction object.
   */
  void del (db::Transaction *transaction = 0);

  /**
   *  @brief "cut" operation
   */
  void cut ();

  /**
   *  @brief "copy" operation
   */
  void copy ();

  /**
   *  @brief "paste" operation
   */
  void paste ();

  /**
   *  @brief Deliver the bbox of the selection
   *
   *  This bounding box is used to compute the center of the 
   *  selection for transformation by an angle (it is better
   *  to do this around der center of the selection bbox).
   *  The bbox must either be empty (if no selection is present)
   *  or must be given in micron units.
   */
  db::DBox selection_bbox ();

  /**
   *  @brief transform the selection
   *
   *  The transformation is given in micron units.
   *
   *  If a transaction is given, the operation will be appended to this pending transaction.
   *  The Editables object takes ownership over the Transaction object.
   */
  void transform (const db::DCplxTrans &tr, db::Transaction *transaction = 0);

  /**
   *  @brief Enable or disable a certain editable
   */
  void enable (lay::Editable *, bool en);

  /**
   *  @brief Clear a selection
   *
   *  The selection is cleared irregardless of whether the editable
   *  is enabled or not.
   */
  void clear_selection ();

  /**
   *  @brief Establish a transient selection
   *
   *  The transient selection is supposed to highlight the object which would be scope of the next
   *  select or move operation. Since move is usually defined to move the current selection, the 
   *  transient selection should not be done if the view is in move mode and something is selected.
   *  Exceptions are: highlighting an object within the current selection.
   */
  void transient_select (const db::DPoint &pt);

  /**
   *  @brief Clear the transient selection
   */
  void clear_transient_selection ();

  /**
   *  @brief Turns the transient selection to the selection
   */
  void transient_to_selection ();

  /**
   *  @brief Clear the previous selection
   *
   *  The previous selection is used to implement the cycling protocol for single-point 
   *  selections.
   */
  void clear_previous_selection ();

  /**
   *  @brief Select "all"
   */
  void select ();

  /**
   *  @brief Select geometrically by a rectangle
   */
  void select (const db::DBox &box, Editable::SelectionMode mode);

  /**
   *  @brief Select geometrically by a point
   */
  void select (const db::DPoint &pt, Editable::SelectionMode mode);

  /**
   *  @brief Repeat the previous selection
   *
   *  This method will not do anything if there is no previous, click-at selection.
   */
  void repeat_selection (Editable::SelectionMode mode);

  /**
   *  @brief Start "move" operation
   *
   *  @param sel Should be true if the selection is to be dragged.
   *  @param ac The angle constraint imposed (lay::AC_Global if no specific is requested)
   *  @return true, if anything is dragged
   */
  bool begin_move (const db::DPoint &p, lay::angle_constraint_type ac);

  /**
   *  @brief Continue "move" operation
   */
  void move (const db::DPoint &p, lay::angle_constraint_type ac);

  /**
   *  @brief Transform during a move operation
   */
  void move_transform (const db::DPoint &p, db::DFTrans tr, lay::angle_constraint_type ac);

  /**
   *  @brief End "move" operation
   *
   *  If a transaction is given, the operation will be appended to this pending transaction
   *  The Editables object takes ownership over the Transaction object.
   */
  void end_move (const db::DPoint &p, lay::angle_constraint_type ac, db::Transaction *transaction = 0);

  /**
   *  @brief Indicates how many objects are selected.
   *
   *  This method will return the number of selected objects.
   */
  size_t selection_size ();
  
  /**
   *  @brief Indicates whether any object is selected.
   */
  bool has_selection ();

  /**
   *  @brief Cancel any pending operations
   */
  void edit_cancel ();

  /**
   *  @brief Editable iterator: begin
   */
  iterator begin () 
  {
    return m_editables.begin ();
  }

  /**
   *  @brief Editable iterator: end
   */
  iterator end () 
  {
    return m_editables.end ();
  }

  /**
   *  @brief The "show properties" operation
   */
  virtual void show_properties ();

  /**
   *  @brief An event triggered if the selection changed
   *  After the selection changed, this event is fired.
   */
  tl::Event selection_changed_event;

  /**
   *  @brief Send a selection changed signal to all observers
   */
  virtual void signal_selection_changed ()
  {
    selection_changed_event ();
  }

  /**
   *  @brief An event indicating that the transient selection has changed
   */
  tl::Event transient_selection_changed_event;

  /**
   *  @brief Send a transient selection changed signal to all observers
   */
  virtual void signal_transient_selection_changed ()
  {
    transient_selection_changed_event ();
  }

  /**
   *  @brief Enable or disable edit operations
   *
   *  This method is called when the client code wishes to (temporarily) enable to disable 
   *  user functions that may cause an edit operation. This method can be reimplemented to 
   *  disable menu entries etc.
   */
  virtual void enable_edits (bool /*enable*/)
  {
    //  .. nothing yet ..
  }

protected:
  /**
   *  @brief Cancel all edit operations
   *
   *  This method can be overridden in order to implement special behaviour on cancel
   *  of edits (i.e. release the mouse).
   *  Make sure, the base implementation is called as well.
   */
  virtual void cancel_edits ();

private:
  friend class Editable;

  tl::shared_collection<lay::Editable> m_editables;
  std::set<lay::Editable *> m_enabled;
  bool m_move_selection;
  bool m_any_move_operation;
  db::DBox m_last_selected_point;

  db::DBox selection_catch_bbox ();
};

}

#endif

