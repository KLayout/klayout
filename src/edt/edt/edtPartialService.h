
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



#ifndef HDR_edtPartialService
#define HDR_edtPartialService

#include "layEditorServiceBase.h"
#include "layObjectInstPath.h"
#include "layViewObject.h"
#include "layRubberBox.h"
#include "laySnap.h"
#include "tlAssert.h"
#include "tlDeferredExecution.h"
#include "edtUtils.h"
#include "edtConfig.h"

#if defined(HAVE_QT)
#  include <QObject>
#  include <QTimer>
#endif

namespace db {
  class Manager;
}

namespace lay {
  class Dispatcher;
  class Marker;
  class InstanceMarker;
}

namespace edt {

// -------------------------------------------------------------
//  A point with an unsigned index describing a certain point of a polygon or path

struct PointWithIndex 
  : public db::Point
{
  PointWithIndex ()
    : db::Point (), n (0), c (0)
  { }

  PointWithIndex (const db::Point &p, unsigned int _n, unsigned int _c)
    : db::Point (p), n (_n), c (_c)
  { }

  unsigned int n, c;

  bool operator== (const PointWithIndex &d) const
  {
    if (n != d.n) {
      return false;
    }
    return db::Point::operator== (d);
  }

  bool operator< (const PointWithIndex &d) const
  {
    if (n != d.n) {
      return n < d.n;
    }
    return db::Point::operator< (d);
  }
};

// -------------------------------------------------------------
//  An edge with two indices describing an edge of a polygon or segment of a path

struct EdgeWithIndex 
  : public db::Edge
{
  EdgeWithIndex ()
    : db::Edge (), n (0), nn (0), c (0) 
  { }

  EdgeWithIndex (const db::Edge &e, unsigned int _n, unsigned int _nn, unsigned int _c)
    : db::Edge (e), n (_n), nn (_nn), c (_c) 
  { }

  unsigned int n, nn, c;

  PointWithIndex pi1 () const
  {
    return PointWithIndex (p1 (), n, c);
  }

  PointWithIndex pi2 () const
  {
    return PointWithIndex (p2 (), nn, c);
  }

  bool operator== (const EdgeWithIndex &d) const
  {
    if (n != d.n || nn != d.nn || c != d.c) {
      return false;
    }
    return db::Edge::operator== (d);
  }

  bool operator< (const EdgeWithIndex &d) const
  {
    if (n != d.n) {
      return n < d.n;
    }
    if (nn != d.nn) {
      return nn < d.nn;
    }
    if (c != d.c) {
      return c < d.c;
    }
    return db::Edge::operator< (d);
  }
};

// -------------------------------------------------------------

/**
 *  @brief The partial selection and manipulation service
 */
class PartialService :
#if defined(HAVE_QT)
    public QObject,
#endif
    public lay::EditorServiceBase,
    public db::Object
{
#if defined(HAVE_QT)
Q_OBJECT
#endif

public: 
  typedef std::map<lay::ObjectInstPath, std::set<EdgeWithIndex> > partial_objects;

  /**
   *  @brief The constructor
   */
  PartialService (db::Manager *manager, lay::LayoutViewBase *view, lay::Dispatcher *root);

  /**
   *  @brief The destructor
   */
  ~PartialService ();

  /**
   *  @brief Access to the view object
   */
  lay::LayoutViewBase *view () const
  {
    tl_assert (mp_view != 0);
    return mp_view;
  }

  /**
   *  @brief Obtain the lay::ViewService interface
   */
  lay::ViewService *view_service_interface ()
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
   *  @brief Implementation of the menu functions
   */
  virtual void menu_activated (const std::string &symbol);

  /**
   *  @brief Implementation of "Plugin" interface: configuration setup
   */
  bool configure (const std::string &name, const std::string &value);

  /**
   *  @brief Implementation of "Plugin" interface: configuration finalization
   */
  void config_finalize ();

  /**
   *  @brief Implement the wheel event (for resetting hove state)
   */
  virtual bool wheel_event (int delta, bool horizontal, const db::DPoint &p, unsigned int buttons, bool prio);

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
   *  @brief Implement the mouse mode: button release
   */
  virtual bool mouse_release_event (const db::DPoint &p, unsigned int buttons, bool prio);

  /**
   *  @brief Transforms the selection
   *
   *  Currently only displacements are allowed which basically moves the partial selection 
   *  by the given distance
   */
  virtual void transform (const db::DCplxTrans &tr);

  /**
   *  @brief Gets the catch distance (for single click)
   */
  virtual double catch_distance ();

  /**
   *  @brief Gets the catch distance (for box)
   */
  virtual double catch_distance_box ();

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
   *  @brief Gets the selection bounding box
   */
  virtual db::DBox selection_bbox ();

  /**
   *  @brief Start a "move" operation
   */
  virtual bool begin_move (MoveMode sel, const db::DPoint &p, lay::angle_constraint_type ac);

  /**
   *  @brief Continue a "move" operation
   */
  virtual void move (const db::DPoint &p, lay::angle_constraint_type ac);

  /**
   *  @brief Terminate a "move" operation
   */
  virtual void end_move (const db::DPoint &p, lay::angle_constraint_type ac);

  /**
   *  @brief Implement the "select" method at least to clear the selection
   */
  virtual bool select (const db::DBox &box, SelectionMode mode);

  /** 
   *  @brief "delete" operation
   */
  virtual void del ();

  /**
   *  @brief Implement the mouse mode: deactivate mouse mode
   */
  virtual void deactivated ();

  /**
   *  @brief Implement the mouse mode: mode activated
   */
  virtual void activated ();

  /**
   *  @brief Reimplementation of the ViewService interface: set the colors
   */
  virtual void set_colors (tl::Color background, tl::Color text);

  /**
   *  @brief Cancel any edit operations (in this case, unselect all & cancel any drag operation)
   */
  virtual void edit_cancel ();

#if defined(HAVE_QT)
public slots:
  void timeout ();
#endif

protected:
  lay::angle_constraint_type connect_ac () const;
  lay::angle_constraint_type move_ac () const;

private:
  //  The layout view that this service is attached to
  lay::LayoutViewBase *mp_view;
  lay::Dispatcher *mp_root;
  bool m_dragging;
  bool m_keep_selection;
  db::DPoint m_start, m_current;
  db::DPoint m_p1, m_p2;
  lay::RubberBox *mp_box;
  unsigned int m_color;
  unsigned int m_buttons;

  //  Angle constraints and grids
  lay::angle_constraint_type m_connect_ac, m_move_ac, m_alt_ac;
  db::DVector m_edit_grid;
  bool m_snap_to_objects;
  bool m_snap_objects_to_grid;
  db::DVector m_global_grid;
  bool m_top_level_sel;

  //  The selection
  partial_objects m_selection;

  //  The marker objects representing the selection
  std::vector<lay::Marker *> m_markers;
  std::vector<lay::Marker *> m_transient_markers;
  std::vector<lay::InstanceMarker *> m_inst_markers;
  std::vector<lay::InstanceMarker *> m_transient_inst_markers;

#if defined(HAVE_QT)
  QTimer m_timer;
#endif
  bool m_hover;
  bool m_hover_wait;
  db::DPoint m_hover_point;

  //  Deferred method to update the selection
  tl::DeferredMethod<edt::PartialService> dm_selection_to_view;

  void hover_reset ();

  void clear_partial_transient_selection ();
  bool partial_select (const db::DBox &box, lay::Editable::SelectionMode mode);
  void selection_to_view ();
  void do_selection_to_view ();

  db::DPoint snap (const db::DPoint &p) const;
  db::DVector snap (const db::DVector &p) const;
  lay::PointSnapToObjectResult snap2 (const db::DPoint &p) const;
  void update_vector_snapped_point (const db::DPoint &pt, db::DVector &vr, bool &result_set) const;
  db::DVector snap_marker_to_grid (const db::DVector &v, bool &snapped) const;
  db::DVector snap_move(const db::DVector &p) const;

  void enter_edge (const EdgeWithIndex &e, size_t &nmarker, partial_objects::const_iterator sel, const std::map <PointWithIndex, db::Point> &new_points, const std::map <EdgeWithIndex, db::Edge> &new_edges, const db::ICplxTrans &gt, const std::vector<db::DCplxTrans> &tv, bool transient);
  void enter_vertices (size_t &nmarker, partial_objects::const_iterator sel, const std::map <PointWithIndex, db::Point> &new_points, const std::map <EdgeWithIndex, db::Edge> &new_edges, const db::ICplxTrans &gt, const std::vector<db::DCplxTrans> &tv, bool transient);
  void enter_polygon (db::Polygon &p, size_t &nmarker, partial_objects::const_iterator sel, const std::map <PointWithIndex, db::Point> &new_points, const std::map <EdgeWithIndex, db::Edge> &new_edges, const db::ICplxTrans &gt, const std::vector<db::DCplxTrans> &tv, bool transient);
  void enter_path (db::Path &p, size_t &nmarker, partial_objects::const_iterator sel, const std::map <PointWithIndex, db::Point> &new_points, const std::map <EdgeWithIndex, db::Edge> &new_edges, const db::ICplxTrans &gt, const std::vector<db::DCplxTrans> &tv, bool transient);
  lay::Marker *new_marker (size_t &nmarker, unsigned int cv_index, bool transient);
  lay::InstanceMarker *new_inst_marker (size_t &nmarker, unsigned int cv_index, bool transient);
  void resize_markers (size_t n, bool transient);
  void resize_inst_markers (size_t n, bool transient);
  bool is_single_point_selection () const;
  bool is_single_edge_selection () const;
  db::DPoint single_selected_point () const;
  db::DEdge single_selected_edge () const;
  bool handle_guiding_shape_changes ();
  void transform_selection (const db::DTrans &move_trans);
};

}

#endif

