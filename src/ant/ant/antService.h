
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



#ifndef HDR_antService
#define HDR_antService

#include "antCommon.h"

#include "layEditorServiceBase.h"
#include "layDrawing.h"
#include "laySnap.h"
#include "layAnnotationShapes.h"
#include "tlEvents.h"
#include "dbLayout.h"
#include "antObject.h"
#include "antTemplate.h"

#include <map>
#include <vector>

namespace ant {

class LayoutViewBase;
class LayoutCanvas;
class Service;

// -------------------------------------------------------------

class ANT_PUBLIC View
  : public lay::ViewObject
{
public: 
  /**
   *  @brief Constructor attaching to a certain object
   */
  View (ant::Service *rulers, const ant::Object *ruler, bool selected);

  /**
   *  @brief The destructor
   */
  ~View ();

  /**
   *  @brief Set a transformation
   *
   *  The transformation how the ruler is transformed before being painted.
   *  The transformation must be specified in database coordinates.
   */
  void transform_by (const db::DCplxTrans &p);

  /**
   *  @brief set the Ruler object
   */
  void ruler (const ant::Object *r);

  /**
   *  @brief Get the db::Ruler object that this view object is presenting
   */
  const ant::Object *ruler () const
  {
    return mp_ruler;
  }

private:
  ant::Service *mp_rulers;
  bool m_selected;
  const ant::Object *mp_ruler;
  db::DCplxTrans m_trans;

  virtual void render (const lay::Viewport &vp, lay::ViewObjectCanvas &canvas);

  //  no copying nor default construction
  View (const View &d);
  View &operator= (const View &d);
  View ();
};

// -------------------------------------------------------------

/**
 *  @brief An iterator for "annotation objects only"
 */
class ANT_PUBLIC AnnotationIterator
{
public:
  typedef const ant::Object value_type;
  typedef const value_type *pointer; 
  typedef const value_type &reference;
  typedef std::forward_iterator_tag iterator_category;
  typedef void difference_type;

  AnnotationIterator (lay::AnnotationShapes::iterator begin, lay::AnnotationShapes::iterator end)
    : m_current (begin), m_end (end)
  {
    next_valid ();
  }

  AnnotationIterator ()
    : m_current (), m_end ()
  {
    //  .. nothing yet ..
  }

  AnnotationIterator (const AnnotationIterator &d)
    : m_current (d.m_current), m_end (d.m_end)
  {
    //  .. nothing yet ..
  }

  AnnotationIterator &operator= (const AnnotationIterator &d)
  {
    if (this != &d) {
      m_current = d.m_current;
      m_end = d.m_end;
    }
    return *this;
  }

  const ant::Object &operator* () const
  {
    return *(dynamic_cast <const ant::Object *> (m_current->ptr ()));
  }

  const ant::Object *operator-> () const
  {
    return dynamic_cast <const ant::Object *> (m_current->ptr ());
  }

  AnnotationIterator &operator++ () 
  {
    ++m_current;
    next_valid ();
    return *this;
  }

  bool at_end () const
  {
    return m_current == m_end;
  }

  lay::AnnotationShapes::iterator current () const
  {
    return m_current;
  }

private:
  void next_valid ()
  {
    while (m_current != m_end && dynamic_cast<const ant::Object *> (m_current->ptr ()) == 0) {
      ++m_current;
    }
  }

  lay::AnnotationShapes::iterator m_current, m_end;
};

// -------------------------------------------------------------

class ANT_PUBLIC Service
  : public lay::EditorServiceBase,
    public lay::Drawing,
    public db::Object
{
public: 
  typedef lay::AnnotationShapes::iterator obj_iterator;

  /**
   *  The current move mode:
   *    MoveNone - not moving
   *    MoveP1 - dragging the first point
   *    MoveP2 - dragging the second point
   *    MoveP12 - dragging (P1.y,P2.x) (if box-like)
   *    MoveP21 - dragging (P1.x,P2.y) (if box-like)
   *    MoveP1X - dragging P1.x (if box-like)
   *    MoveP2X - dragging P2.x (if box-like)
   *    MoveP1Y - dragging P1.y (if box-like)
   *    MoveP2Y - dragging P2.y (if box-like)
   *    MoveRuler - dragging a whole ruler (one)
   *    MoveSelection - dragging a whole ruler (many)
   */
  enum MoveMode { MoveNone, MoveP1, MoveP2, MoveP12, MoveP21, MoveP1X, MoveP2X, MoveP1Y, MoveP2Y, MoveRuler, MoveSelected };

  Service (db::Manager *manager, lay::LayoutViewBase *view);

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
   *  @brief Highlight a certain object
   */
  void highlight (unsigned int n);

  /**
   *  @brief Cancel dragging the ruler. The ruler is erased.
   */
  void drag_cancel ();

  /**
   *  @brief Cancel any edit operations (such as move)
   */
  void edit_cancel ();

  /** 
   *  @brief Clear all rulers
   */
  void clear_rulers ();

  /** 
   *  @brief "delete" operation
   */
  virtual void del ();

  /** 
   *  @brief "cut" operation
   */
  virtual void cut ();

  /** 
   *  @brief "copy" operation
   */
  virtual void copy ();

  /** 
   *  @brief "paste" operation
   */
  virtual void paste ();

  /**
   *  @brief Indicates whether there are selection objects
   */
  virtual bool has_selection ();

  /**
   *  @brief Indicates how many objects are selected
   */
  virtual size_t selection_size ();

  /**
   *  @brief Indicates whether there are selection objects in transient mode
   */
  virtual bool has_transient_selection ();

  /**
   *  @brief point selection proximity predicate
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
   *  @brief Clears the previous selection
   */
  virtual void clear_previous_selection ();

  /**
   *  @brief Turns the transient selection to the selection
   */
  virtual void transient_to_selection ();

  /**
   *  @brief Establish a transient selection
   */
  virtual bool transient_select (const db::DPoint &pos);

  /**
   *  @brief Clear the transient selection
   */
  virtual void clear_transient_selection ();

  /**
   *  @brief Inserts a ruler
   *  The return value will be the ID of the new ruler.
   */
  int insert_ruler (const ant::Object &ruler, bool limit_number);

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
   *  @brief Return the bbox of the selection (reimplementation of lay::Editable interface)
   */
  virtual db::DBox selection_bbox ();

  /**
   *  @brief Transform the selection (reimplementation of lay::Editable interface)
   */
  virtual void transform (const db::DCplxTrans &trans);

#if defined(HAVE_QT)
  /**
   *  @brief Create the properties page
   */
  virtual std::vector<lay::PropertiesPage *> properties_pages (db::Manager *manager, QWidget *parent);
#endif

  /**
   *  @brief Get the selection for the properties page
   */
  void get_selection (std::vector <obj_iterator> &selection) const;

  /**
   *  @brief Direct access to the selection 
   */
  const std::map<obj_iterator, unsigned int> &selection () const
  {
    return m_selected;
  }

  /**
   *  @brief Change a specific ruler 
   */
  void change_ruler (obj_iterator pos, const ant::Object &to);

  /**
   *  @brief Delete a specific ruler 
   */
  void delete_ruler (obj_iterator pos);

  /**
   *  @brief Implementation of "Plugin" interface: configuration setup
   */
  bool configure (const std::string &name, const std::string &value);

  /**
   *  @brief Implementation of "Plugin" interface: configuration finalization
   */
  void config_finalize ();

  /**
   *  @brief Color accessor
   */
  tl::Color color () const
  {
    return m_color;
  }

  /**
   *  @brief Halo flag accessor
   */
  bool with_halo () const
  {
    return m_halo;
  }

  /**
   *  @brief Obtain the lay::ViewService interface
   */
  lay::ViewService *view_service_interface ()
  {
    return this;
  }

  /**
   *  @brief Obtain the lay::Drawing interface
   */
  lay::Drawing *drawing_interface ()
  {
    return this;
  }

  /**
   *  @brief Obtain the lay::Editable interface
   */
  lay::Editable *editable_interface ()
  {
    return this;
  }

  /**
   *  @brief Access to the view object
   */
  lay::LayoutViewBase *view () const
  {
    return mp_view;
  }

  /**
   *  @brief Gets the snap range
   */
  int snap_range () const
  {
    return m_snap_range;
  }

  /**
   *  @brief Gets the global snap mode
   */
  lay::angle_constraint_type snap_mode () const
  {
    return m_snap_mode;
  }

  /**
   *  @brief Gets the grid
   */
  double grid () const
  {
    return m_grid;
  }

  /**
   *  @brief Gets a value indicating whether to snap to grid
   */
  bool grid_snap () const
  {
    return m_grid_snap;
  }

  /**
   *  @brief Implement the menu response function
   */
  void menu_activated (const  std::string &symbol);

  /**
   *  @brief Return the annotation iterator that delivers the annotations (and only these)
   */
  AnnotationIterator begin_annotations () const;

  /**
   *  @brief Creates an auto-measure ruler at the given point with the given angle constraint
   */
  ant::Object create_measure_ruler(const db::DPoint &pt, lay::angle_constraint_type ac);

  /**
   *  @brief Gets the annotation templates
   */
  const std::vector<ant::Template> &ruler_templates () const
  {
    return m_ruler_templates;
  }

  /**
   *  @brief An event triggered when the annotations changed
   *  When an annotation is added or removed, this event is triggered.
   */
  tl::Event annotations_changed_event;

  /**
   *  @brief An event triggered when one annotation was modified
   *  The argument is the ID of the annotation that was modified.
   */
  tl::event<int> annotation_changed_event;

  /**
   *  @brief An event triggered when the selected annotations changed
   */
  tl::Event annotation_selection_changed_event;

private:
  //  Ruler display and snapping configuration
  tl::Color m_color;
  bool m_halo;
  lay::angle_constraint_type m_snap_mode;
  double m_grid;
  bool m_grid_snap;
  bool m_obj_snap;
  int m_snap_range;

  //  Configuration parameter: maximum number of rulers
  int m_max_number_of_rulers;

  //  The layout view that the ruler service is attached to
  lay::LayoutViewBase *mp_view;

  //  The ruler view objects representing the selection
  //  and the moved rules in move mode
  std::vector<ant::View *> m_rulers;
  //  The selection
  std::map<obj_iterator, unsigned int> m_selected;
  //  The previous selection
  std::map<obj_iterator, unsigned int> m_previous_selection;
  //  The reference point in move mode
  db::DPoint m_p1;
  //  The transformation in MoveSelection mode
  db::DTrans m_trans;
  //  The ruler representing the dragged ruler in "create ruler" mode
  ant::View *mp_active_ruler;
  //  The ruler representing the transient selection
  ant::View *mp_transient_ruler;
  //  True, if creating a ruler (dragging)
  bool m_drawing;
  //  The ruler object representing the ruler being created
  ant::Object m_current;
  //  The ruler object representing the original ruler when moving one
  ant::Object m_original;
  //  The current move mode
  MoveMode m_move_mode;
  //  The currently moving segment
  size_t m_seg_index;
  //  The ruler template
  std::vector<ant::Template> m_ruler_templates;
  unsigned int m_current_template;

  std::pair<bool, db::DPoint> snap1 (const db::DPoint &p, bool obj_snap);
  lay::PointSnapToObjectResult snap1_details (const db::DPoint &p, bool obj_snap);
  std::pair<bool, db::DPoint> snap2 (const db::DPoint &p1, const db::DPoint &p2, const ant::Object *obj, lay::angle_constraint_type ac);
  lay::PointSnapToObjectResult snap2_details (const db::DPoint &p1, const db::DPoint &p2, const ant::Object *obj, lay::angle_constraint_type ac);

  const ant::Template &current_template () const;

  void show_message ();

  /**
   *  @brief A handler for the shape container's changed event
   */
  void annotations_changed ();

  virtual bool mouse_move_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool mouse_press_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool mouse_click_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual bool mouse_double_click_event (const db::DPoint &p, unsigned int buttons, bool prio);
  virtual void deactivated ();

  /**
   *  @brief Select a certain ruler
   *
   *  @return true, if the selection has changed
   */
  bool select (obj_iterator obj, lay::Editable::SelectionMode mode);

  /**
   *  @brief Clears the selection
   */
  void clear_selection ();

  /**
   *  @brief Limit the number of rulers to this number
   */
  void reduce_rulers (int num);

  /**
   *  @brief Finishes drawing mode and creates the ruler
   */
  void finish_drawing ();

  /**
   *  @brief Delete the selected rulers
   *
   *  Used as implementation for "del" and "cut"
   */
  void del_selected ();

  /**
   *  @brief Copy the selected rulers to the clipboard
   *
   *  Used as implementation for "copy" and "cut"
   */
  void copy_selected ();

  /**
   *  @brief implementation of the "Drawing" interface: painting
   */
  void paint_on_planes (const db::DCplxTrans &trans,
                        const std::vector <lay::CanvasPlane *> &planes,
                        lay::Renderer &renderer);

  /**
   *  @brief implementation of the "Drawing" interface: configuration
   */
  std::vector <lay::ViewOp> get_view_ops (lay::RedrawThreadCanvas &canvas, tl::Color background, tl::Color foreground, tl::Color active) const;

  /**
   *  @brief Update m_rulers to reflect the selection
   */
  void selection_to_view ();

  /**
   *  @brief Display a message about the current selection
   */
  void display_status (bool transient);
};

}

#endif

