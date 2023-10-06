
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



#ifndef HDR_laySnap
#define HDR_laySnap

#include "laybasicCommon.h"

#if defined(HAVE_QT)
#include <QPoint>
#endif

#include <utility>
#include <vector>

#include "dbPoint.h"
#include "dbVector.h"
#include "dbEdge.h"
#include "dbTypes.h"

namespace lay
{
  class LayoutViewBase;

  /**
   *  @brief An angle constraint type
   *
   *  Any: no angle constraint
   *  Diagonal: vertical, horizontal and 45 degree diagonals
   *  Ortho: vertical and horizontal
   *  Horizontal: horizontal only
   *  Vertical: vertical only
   *  Global: use global setting (templates and ruler specific setting only)
   */
  enum angle_constraint_type { AC_Any = 0, AC_Diagonal, AC_Ortho, AC_Horizontal, AC_Vertical, AC_Global, AC_NumModes };

  /**
   *  @brief snap a coordinate value to a unit grid
   */
  inline db::DCoord snap (db::DCoord c)
  {
    return floor (c + 0.5 + db::coord_traits<double>::prec ());
  }

  /**
   *  @brief snap a coordinate value to a unit grid
   */
  inline db::DPoint snap (const db::DPoint &p)
  {
    return db::DPoint (snap (p.x ()), snap (p.y ()));
  }

  /**
   *  @brief snap a coordinate value to the given grid
   */
  LAYBASIC_PUBLIC db::DCoord snap (db::DCoord c, db::DCoord grid);

  /**
   *  @brief snap a point value to the given grid
   */
  LAYBASIC_PUBLIC db::DPoint snap (const db::DPoint &p, db::DCoord grid);

  /**
   *  @brief snap a point value to the given (anisotropic) grid
   */
  LAYBASIC_PUBLIC db::DPoint snap_xy (const db::DPoint &p, const db::DPoint &grid);

  /**
   *  @brief snap a point value to the given (anisotropic) grid
   */
  inline db::DPoint snap_xy (const db::DPoint &p, const db::DVector &grid)
  {
    return snap_xy (p, db::DPoint (grid.x (), grid.y ()));
  }

  /**
   *  @brief snapping of a two-point vector 
   *
   *  The snapping algorithm used ensures maximum fidelity of the
   *  resulting vector with respect to the original (angle-wise and length-wise).
   */
  LAYBASIC_PUBLIC std::pair<db::DPoint, db::DPoint> snap (const db::DPoint &p1, const db::DPoint &p2);

  /**
   *  @brief snapping of a two-point vector to a grid
   *
   *  The snapping algorithm used ensures maximum fidelity of the
   *  resulting vector with respect to the original (angle-wise and length-wise).
   */
  LAYBASIC_PUBLIC std::pair<db::DPoint, db::DPoint> snap (const db::DPoint &p1, const db::DPoint &p2, db::DCoord grid);

  /**
   *  @brief A structure describing the snap result for a point-wise snap
   */
  struct LAYBASIC_PUBLIC PointSnapToObjectResult
  {
    enum ObjectSnap {
      NoObject = 0,
      ObjectVertex,
      ObjectEdge,
      ObjectUnspecific
    };

    PointSnapToObjectResult () : object_snap (NoObject) { }

    /**
     *  @brief The result of the snap
     */
    db::DPoint snapped_point;

    /**
     *  @brief Indicates whether and how the point was snapped to an object
     */
    ObjectSnap object_snap;

    /**
     *  @brief Indicates the edge the point was snapped against unless in NoObject mode
     */
    db::DEdge object_ref;
  };

  /**
   *  @brief combined grid-, projection- and object snapping provided to implementing "magnetic features"
   *
   *  This method will snap a point pt to the grid, project it to one of the cutlines or try to snap it 
   *  to an object edge or point. It will use a heuristics to determine
   *  what criterion is applied if a conflict is detected.
   *
   *  The function will return a PointSnapToObjectResult object.
   *
   *  @param view The layout view used for object snapping. Can be 0 to disable object snapping
   *  @param pt The point to snap
   *  @param grid Either (0,0) to disable grid snapping or a (gx,gy) value for the (potentially anisotropic grid)
   *  @param snap_range The search range for objects 
   */
  LAYBASIC_PUBLIC PointSnapToObjectResult obj_snap (lay::LayoutViewBase *view, const db::DPoint &pt, const db::DVector &grid, double snap_range);

  /**
   *  @brief combined grid-, projection- and object snapping provided to implementing "magnetic features"
   *
   *  This is a convenience method that creates the projection axes from a reference point and an angle mode.
   *  "pr" is the reference point, "pt" is the point to snap.
   */
  LAYBASIC_PUBLIC PointSnapToObjectResult obj_snap (lay::LayoutViewBase *view, const db::DPoint &pr, const db::DPoint &pt, const db::DVector &grid, lay::angle_constraint_type ac, double snap_range);

  /**
   *  @brief A structure describing the snap result for a two-sided object snap (distance measurement)
   */
  struct LAYBASIC_PUBLIC TwoPointSnapToObjectResult
  {
    enum ObjectSnap {
      NoObject = 0,
      ObjectVertex,
      ObjectEdge,
      ObjectUnspecific
    };

    TwoPointSnapToObjectResult () : any (false), object_snap_first (NoObject), object_snap_second (NoObject) { }

    /**
     *  @brief Indicates whether the two-sided snap was successful
     */
    bool any;

    /**
     *  @brief The result of the snap
     *  Two values are provided for the first and second snap.
     */
    db::DPoint first, second;

    /**
     *  @brief Indicates whether and how the point was snapped to an object
     *  Two values are provided for the first and second snap.
     */
    ObjectSnap object_snap_first, object_snap_second;

    /**
     *  @brief Indicates the edge the point was snapped against unless in NoObject mode
     *  Two values are provided for the first and second snap.
     */
    db::DEdge object_ref_first, object_ref_second;
  };

  /**
   *  @brief Same than obj_snap, but delivers two points on two opposite sides of the initial point
   *
   *  This method basically implements "auto measure". The first value of the returned pair
   *  is true if such an edge could be found. Otherwise it's false.
   */
  LAYBASIC_PUBLIC TwoPointSnapToObjectResult obj_snap2 (LayoutViewBase *view, const db::DPoint &pt, const db::DVector &grid, double min_search_range, double max_search_range);

  /**
   *  @brief Same than obj_snap, but delivers two points on two opposite sides of the initial points
   *
   *  This method basically implements "auto measure". The first value of the returned pair
   *  is true if such an edge could be found. Otherwise it's false.
   *
   *  This version accepts two points defining different search regions for first and second edge.
   */
  LAYBASIC_PUBLIC TwoPointSnapToObjectResult obj_snap2 (lay::LayoutViewBase *view, const db::DPoint &pt1, const db::DPoint &pt2, const db::DVector &grid, double min_search_range, double max_search_range);

  /**
   *  @brief Same than the previous obj_snap2, but allows specification of an angle constraint
   *
   *  Measurements will be confined to the direction specified.
   */
  LAYBASIC_PUBLIC TwoPointSnapToObjectResult obj_snap2 (lay::LayoutViewBase *view, const db::DPoint &pt, const db::DVector &grid, lay::angle_constraint_type ac, double min_search_range, double max_search_range);

  /**
   *  @brief Same than the previous obj_snap2, but allows specification of an angle constraint
   *
   *  Measurements will be confined to the direction specified.
   *
   *  This version accepts two points defining different search regions for first and second edge.
   */
  LAYBASIC_PUBLIC TwoPointSnapToObjectResult obj_snap2 (lay::LayoutViewBase *view, const db::DPoint &pt1, const db::DPoint &pt2, const db::DVector &grid, lay::angle_constraint_type ac, double min_search_range, double max_search_range);

  /**
   *  @brief Reduce a given vector according to the angle constraint
   *
   *  If the "snapped_to" pointer is non-null, it will receive the snap target
   *  vector (e.g. (1, 0) for snapping to horizontal axis. If no snapping happens,
   *  the value of this vector is not changed.
   */
  LAYBASIC_PUBLIC db::DVector snap_angle (const db::DVector &in, lay::angle_constraint_type ac, db::DVector *snapped_to = 0);

  /**
   *  @brief rounding of a double value for drawing purposes
   */
  LAYBASIC_PUBLIC int draw_round (double x);

  /**
   *  @brief rounding (and height-transformation) of a double point 
   */
  LAYBASIC_PUBLIC db::Point draw_round (db::DPoint p, int h);

  /**
   *  @brief rounding (and height-transformation) of a two-point vector
   */
  LAYBASIC_PUBLIC std::pair<db::Point, db::Point> draw_round (const db::DPoint &p1, const db::DPoint &p2, int h);

  /**
   *  @brief rounding (and height-transformation) of a two-point vector
   */
  LAYBASIC_PUBLIC std::pair<db::DPoint, db::DPoint> draw_round_dbl (const db::DPoint &p1, const db::DPoint &p2, int h);
}

#endif


