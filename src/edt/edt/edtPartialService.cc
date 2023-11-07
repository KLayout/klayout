
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


#include "dbVector.h"
#include "layLayoutViewBase.h"
#include "laySnap.h"
#include "layFinder.h"
#include "tlProgress.h"
#include "edtPartialService.h"
#include "edtService.h"
#include "edtConfig.h"
#include "edtPlugin.h"

#if defined(HAVE_QT)
#  include "edtDialogs.h"
#  include "edtEditorOptionsPages.h"
#endif

#include <cmath>

namespace tl
{
  class Progress;
}

namespace edt
{

//  max. number of tries in single-click selection before giving up
static int point_sel_tests = 10000; // TODO: is this a reasonable value?

// -------------------------------------------------------------

/**
 *  @brief A move constraint 
 *
 *  A move constraint describes the degrees of freedom for a single point.
 *  Such a constraint can be: fixed (no freedom), unconstrained (point can move
 *  both in x and y direction) and freedom along an axis.
 */
class Constraint
{
public:
  typedef enum { Free, Fixed, OneDim } constraint_mode;

  /**
   *  @brief Construct an unconstrained constraint
   */
  Constraint ()
    : m_mode (Free)
  { }

  /**
   *  @brief Construct an "fixed" constraint
   */
  Constraint (int)
    : m_mode (Fixed)
  { }

  /**
   *  @brief Construct an "1-dimensional" constraint
   *
   *  @param axis Gives the direction in which the point can move, if 0 same as fixed constraint
   */
  Constraint (db::Vector axis)
    : m_axis (axis)
  { 
    if (axis == db::Vector ()) {
      m_mode = Fixed;
    } else {
      m_mode = OneDim;
    } 
  }

  /**
   *  @brief Type accessor
   */
  constraint_mode mode () const
  {
    return m_mode;
  }

  /**
   *  @brief Add constraints
   *
   *  Merging of constraints means to allow movement additionally in the same directions
   *  than given by the second constraint.
   */
  Constraint &operator+= (const Constraint &b)
  {
    if (m_mode == Fixed || b.mode () == Free) {
      *this = b;
    } else if (m_mode == Free || b.mode () == Fixed) {
      //  nothing to do.
    } else {
      //  must both be OneDim here.
      if (db::vprod_sign (m_axis, b.m_axis) != 0) {
        m_mode = Free;
      }
    }
    return *this;
  }

  /**
   *  @brief Unite constraints
   *
   *  Additionally impose a constraint on this movement.
   */
  Constraint &operator*= (const Constraint &b)
  {
    if (m_mode == Free || b.mode () == Fixed) {
      *this = b;
    } else if (m_mode == Fixed || b.mode () == Free) {
      //  nothing to do.
    } else {
      //  must both be OneDim here.
      if (db::vprod_sign (m_axis, b.m_axis) != 0) {
        m_mode = Fixed;
      }
    }
    return *this;
  }

  /**
   *  @brief Move a point by the given vector, given the imposed constraints
   *
   *  The movement is performed "as far as possible", i.e. projecting the axis to the
   *  move vector, not vice versa.
   *
   *  @param p The point to move
   *  @param v The vector by which to move (desired direction)
   *  @return first: true, if the move was successful, second: the resulting point
   */
  std::pair<bool, db::Point> move (const db::Point &p, const db::DVector &v)
  {
    if (v == db::DVector ()) {
      return std::make_pair (true, p);
    } else if (m_mode == Free) {
      return std::make_pair (true, p + db::Vector (v));
    } else if (m_mode == Fixed) {
      return std::make_pair (true, p);
    } else {
      double proj = db::sprod (db::DVector (m_axis), v);
#if 0
      //  HINT: disallowing movements when constraint axis and edge form a small angle
      //  leads to strange results when moving rounded corner segments as a whole for example.
      //  Therefore the creation of artefacts in this case is not recommended
      //  There is other code that takes care of the case that edges get very long
      //  beyond a certain threshold.

      //  do not allow movement for small angles to avoid creation of nasty spikes
      if (fabs (proj) < m_axis.double_length () * v.double_length () * 0.1) {
        return std::make_pair (false, p);
      } else {
        //  check for overflow and return false if that happens
        db::DPoint dp = db::DPoint (p) + db::DVector (m_axis) * (v.sq_double_length () / proj);
        if (dp.x () <= double (std::numeric_limits<db::Point::coord_type>::min ()) || dp.x () >= double (std::numeric_limits<db::Point::coord_type>::max ()) ||
            dp.y () <= double (std::numeric_limits<db::Point::coord_type>::min ()) || dp.y () >= double (std::numeric_limits<db::Point::coord_type>::max ())) {
          return std::make_pair (false, p);
        } else {
          return std::make_pair (true, db::Point::from_double (dp));
        }
      }
#else
      //  check for overflow and return false if that happens
      db::DPoint dp = db::DPoint (p) + db::DVector (m_axis) * (v.sq_double_length () / proj);
      if (dp.x () <= double (std::numeric_limits<db::Point::coord_type>::min ()) || dp.x () >= double (std::numeric_limits<db::Point::coord_type>::max ()) ||
          dp.y () <= double (std::numeric_limits<db::Point::coord_type>::min ()) || dp.y () >= double (std::numeric_limits<db::Point::coord_type>::max ())) {
        return std::make_pair (false, p);
      } else {
        return std::make_pair (true, db::Point (dp));
      }
#endif
    }
  }

  /**
   *  @brief Transform by a given transformation
   */
  template <class T>
  Constraint &transform (const T &t)
  {
    m_axis.transform (t);
    return *this;
  }

  /**
   *  @brief Return the transformed version
   */
  template <class T>
  Constraint transformed (const T &t) const
  {
    Constraint c (*this);
    return c.transform (t);
  }

private:
  constraint_mode m_mode;
  db::Vector m_axis;
};

// -------------------------------------------------------------
//  Some utilities

static bool
insert_point_path (const db::Path &p, const std::set<EdgeWithIndex> &sel, db::Point &ins, db::Path &new_path)
{
  new_path.width (p.width ());
  new_path.round (p.round ());
  new_path.extensions (p.bgn_ext (), p.end_ext ());

  std::vector<db::Point> ctr;
  ctr.reserve (p.points () + 1);

  bool found = false;

  unsigned int n = 0;
  for (db::Path::iterator pt = p.begin (); pt != p.end (); ++n) {
    db::Point p1 = *pt;
    ++pt;
    if (pt != p.end ()) {
      db::Point p2 = *pt;
      ctr.push_back (p1);
      if (! found && sel.find (EdgeWithIndex (db::Edge (p1, p2), n, n + 1, 0)) != sel.end ()) {
        //  project the point onto the edge
        db::Edge e (p1, p2);
        std::pair <bool, db::Point> projected = e.projected (ins);
        if (projected.first) {
          if (e.is_ortho ()) {
            //  NOTE: for skew edges we use the original point as the projected one usually
            //  is off-grid.
            ins = projected.second;
          }
          ctr.push_back (ins);
          found = true;
        }
      }
    } else {
      ctr.push_back (p1);
    }
  }

  if (found) {
    new_path.assign (ctr.begin (), ctr.end ());
  } 
  return found;
}

static void 
remove_redundant_points (std::vector <db::Point> &ctr, bool cyclic)
{
  //  compress contour (remove redundant points)
  //  and assign to path

  std::vector<db::Point>::iterator wp = ctr.begin ();
  std::vector<db::Point>::const_iterator rp = ctr.begin ();
  db::Point pm1;
  if (rp != ctr.end ()) {
    if (cyclic) {
      pm1 = ctr.back ();
    } else {
      pm1 = ctr.front ();
      ++wp;
      ++rp;
    }
    while (rp != ctr.end ()) {
      db::Point p0 = *rp;
      if (p0 != pm1) {
        *wp++ = p0;
      }
      pm1 = p0;
      ++rp;
    }
  }

  ctr.erase (wp, ctr.end ());
}

static db::Path
del_points_path (const db::Path &p, const std::set<EdgeWithIndex> &sel)
{
  db::Path new_path;
  new_path.width (p.width ());
  new_path.round (p.round ());
  new_path.extensions (p.bgn_ext (), p.end_ext ());

  std::vector<db::Point> ctr;
  ctr.reserve (p.points ());

  unsigned int n = 0;
  for (db::Path::iterator pt = p.begin (); pt != p.end (); ++pt, ++n) {
    db::Point p1 = *pt;
    if (sel.find (EdgeWithIndex (db::Edge (p1, p1), n, n, 0)) == sel.end ()) {
      ctr.push_back (p1);
    }
  }

  remove_redundant_points (ctr, false);
  new_path.assign (ctr.begin (), ctr.end ());

  return new_path;
}

static void 
modify_path (db::Path &p, const std::map <PointWithIndex, db::Point> &new_points, const std::map <EdgeWithIndex, db::Edge> &new_edges, bool compress = false)
{
  std::vector<db::Point> ctr;
  ctr.reserve (p.points ());

  std::map <PointWithIndex, db::Point>::const_iterator np;
  std::map <EdgeWithIndex, db::Edge>::const_iterator ne;

  unsigned int n = 0;
  for (db::Path::iterator pt = p.begin (); pt != p.end (); ++n) {

    db::Point p1 = *pt;
    db::Point p1org = p1;
    np = new_points.find (PointWithIndex (p1, n, 0));
    if (np != new_points.end ()) {
      p1 = np->second;
    }

    ++pt;
    if (pt != p.end ()) {

      db::Point p2 = *pt;
      db::Point p2org = p2;
      np = new_points.find (PointWithIndex (p2, n + 1, 0));
      if (np != new_points.end ()) {
        p2 = np->second;
      }

      ne = new_edges.find (EdgeWithIndex (db::Edge (p1org, p2org), n, n + 1, 0));

      ctr.push_back (p1);

      if (ne != new_edges.end () && ne->second.p1 () != p1) {
        ctr.push_back (ne->second.p1 ());
      }
      if (ne != new_edges.end () && ne->second.p2 () != p2) {
        ctr.push_back (ne->second.p2 ());
      }

    } else {
      ctr.push_back (p1);
    }

  }

  if (compress) {
    remove_redundant_points (ctr, false);
  }

  p.assign (ctr.begin (), ctr.end ());
}

bool
insert_point_poly (const db::Polygon &p, const std::set<EdgeWithIndex> &sel, db::Point &ins, db::Polygon &new_poly)
{
  for (unsigned int c = 0; c < p.holes () + 1; ++c) {

    bool found = false;

    std::vector<db::Point> ctr;
    size_t points = p.contour (c).size ();
    ctr.reserve (points + 1);

    unsigned int n = 0;
    db::Shape::polygon_edge_iterator ee;
    for (db::Shape::polygon_edge_iterator e = p.begin_edge (c); ! e.at_end (); e = ee, ++n) {

      ee = e;
      ++ee;
      unsigned int nn = ee.at_end () ? 0 : n + 1;

      ctr.push_back ((*e).p1 ());
      if (! found && sel.find (EdgeWithIndex (*e, n, nn, c)) != sel.end ()) {
        //  project the point onto the edge - use the first edge the point projects to
        std::pair <bool, db::Point> projected = (*e).projected (ins);
        if (projected.first) {
          if ((*e).is_ortho ()) {
            //  NOTE: for skew edges we use the original point as the projected one usually
            //  is off-grid.
            ins = projected.second;
          }
          ctr.push_back (ins);
          found = true;
        }
      }

    }

    if (found) {

      remove_redundant_points (ctr, true);

      new_poly = p;
      if (c == 0) {
        new_poly.assign_hull (ctr.begin (), ctr.end (), false /*don't compress*/);
      } else {
        new_poly.assign_hole (c - 1, ctr.begin (), ctr.end (), false /*don't compress*/);
      }
      return true;

    }

  }

  return false;
}

static db::Polygon
del_points_poly (const db::Polygon &p, const std::set<EdgeWithIndex> &sel)
{
  db::Polygon new_poly = p;

  for (unsigned int c = 0; c < p.holes () + 1; ++c) {

    std::vector<db::Point> ctr;
    size_t points = p.contour (c).size ();
    ctr.reserve (points);

    unsigned int n = 0;
    for (db::Shape::polygon_edge_iterator e = p.begin_edge (c); ! e.at_end (); ++e, ++n) {
      db::Point p1 = (*e).p1 ();
      if (sel.find (EdgeWithIndex (db::Edge (p1, p1), n, n, c)) == sel.end ()) {
        ctr.push_back (p1);
      }
    }

    remove_redundant_points (ctr, true);

    if (c == 0) {
      new_poly.assign_hull (ctr.begin (), ctr.end (), false /*compress*/);
    } else {
      new_poly.assign_hole (c - 1, ctr.begin (), ctr.end (), false /*compress*/);
    }

  }

  return new_poly;
}

static void
modify_polygon (db::Polygon &p, 
                const std::map <PointWithIndex, db::Point> &new_points, 
                const std::map <EdgeWithIndex, db::Edge> &new_edges,
                bool compress = false)
{
  for (unsigned int c = 0; c < p.holes () + 1; ++c) {

    std::vector<db::Point> ctr;
    size_t points = p.contour (c).size ();
    ctr.reserve (points);

    std::map <PointWithIndex, db::Point>::const_iterator np;
    std::map <EdgeWithIndex, db::Edge>::const_iterator ne;

    unsigned int n = 0;
    db::Shape::polygon_edge_iterator ee;
    for (db::Shape::polygon_edge_iterator e = p.begin_edge (c); ! e.at_end (); e = ee, ++n) {

      ee = e;
      ++ee;
      unsigned int nn = ee.at_end () ? 0 : n + 1;

      db::Point p1 = (*e).p1 ();
      np = new_points.find (PointWithIndex (p1, n, c));
      if (np != new_points.end ()) {
        p1 = np->second;
      }

      db::Point p2 = (*e).p2 ();
      np = new_points.find (PointWithIndex (p2, nn, c));
      if (np != new_points.end ()) {
        p2 = np->second;
      }

      ne = new_edges.find (EdgeWithIndex (*e, n, nn, c));

      ctr.push_back (p1);

      if (ne != new_edges.end () && ne->second.p1 () != p1) {
        ctr.push_back (ne->second.p1 ());
      }
      if (ne != new_edges.end () && ne->second.p2 () != p2) {
        ctr.push_back (ne->second.p2 ());
      }

    }

    if (compress) {
      remove_redundant_points (ctr, true);
    }

    if (c == 0) {
      p.assign_hull (ctr.begin (), ctr.end (), false /*compress*/);
    } else {
      p.assign_hole (c - 1, ctr.begin (), ctr.end (), false /*compress*/);
    }

  }
}

static void 
constrain (std::map <PointWithIndex, Constraint> &constr, const EdgeWithIndex &edge)
{
  constr.insert (std::make_pair (edge.pi1 (), Constraint ())).first->second *= Constraint (edge.d ());
  constr.insert (std::make_pair (edge.pi2 (), Constraint ())).first->second *= Constraint (edge.d ());
}

static void
create_shift_sets (const db::Shape &shape, const std::set <EdgeWithIndex> &sel, std::map <PointWithIndex, db::Point> &new_points, std::map <EdgeWithIndex, db::Edge> &new_edges, db::Vector mv)
{
  //  Set up a map of new edges and new points
  for (std::set <EdgeWithIndex>::const_iterator e = sel.begin (); e != sel.end (); ++e) {
    if (e->p1 () != e->p2 ()) {
      new_edges.insert (std::make_pair (*e, db::Edge (*e)));
    } else {
      new_points.insert (std::make_pair (PointWithIndex (e->p1 (), e->n, e->c), e->p1 ()));
    }
  }

  //  new_points should only contain the selected points, not the start and end points of selected edges
  for (std::set <EdgeWithIndex>::const_iterator e = sel.begin (); e != sel.end (); ++e) {
    if (e->p1 () != e->p2 ()) {
      new_points.erase (e->pi1 ());
      new_points.erase (e->pi2 ());
    }
  }

  std::map <PointWithIndex, Constraint> point_constr;

  if (shape.is_polygon ()) {

    for (unsigned int c = 0; c < shape.holes () + 1; ++c) {

      unsigned int n = 0;
      db::Shape::polygon_edge_iterator ee;
      for (db::Shape::polygon_edge_iterator e = shape.begin_edge (c); ! e.at_end (); e = ee, ++n) {
        ee = e;
        ++ee;
        unsigned int nn = ee.at_end () ? 0 : n + 1;
        if ((*e).p1 () != (*e).p2 () && sel.find (EdgeWithIndex (*e, n, nn, c)) == sel.end ()) {
          constrain (point_constr, EdgeWithIndex (*e, n, nn, c));
        }
      }

    }

  } else if (shape.is_path ()) {

    if (shape.begin_point () != shape.end_point ()) {

      db::Shape::point_iterator pt = shape.begin_point ();
      db::Point p1 = *pt;

      unsigned int n = 0;
      while (true) {

        ++pt;
        if (pt == shape.end_point ()) {
          break;
        }

        EdgeWithIndex e (db::Edge (p1, *pt), n, n + 1, 0);
        if (e.p1 () != e.p2 () && sel.find (e) == sel.end ()) {
          constrain (point_constr, e);
        }

        p1 = *pt;
        ++n;

      }

    }

  } else if (shape.is_box ()) {

    //  convert to polygon and test those edges
    db::Polygon poly (shape.box ());
    unsigned int n = 0;
    db::Shape::polygon_edge_iterator ee;
    for (db::Shape::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end (); e = ee, ++n) {
      ee = e;
      ++ee;
      unsigned int nn = ee.at_end () ? 0 : n + 1;
      EdgeWithIndex ewi (*e, n, nn, 0);
      if ((*e).p1 () != (*e).p2 () && sel.find (ewi) == sel.end ()) {
        //  add some moveable edges to impose manhattan constraints
        if (new_points.find (ewi.pi1 ()) != new_points.end () || 
            new_points.find (ewi.pi2 ()) != new_points.end ()) {
          new_edges.insert (std::make_pair (ewi, db::Edge ((*e))));
        } else {
          constrain (point_constr, ewi);
        }
      }
    }

  } 

  //  Simply move the points
  for (std::map <PointWithIndex, db::Point>::iterator np = new_points.begin (); np != new_points.end (); ++np) {
    np->second += mv;
  }

  //  The edges are treated somewhat more elaborate:
  for (std::map <EdgeWithIndex, db::Edge>::iterator ne = new_edges.begin (); ne != new_edges.end (); ++ne) {

    std::map <PointWithIndex, Constraint>::iterator c1, c2;

    //  compute normal of move vector
    db::DVector nmv;
    if (ne->first.d () != db::Vector ()) {
      nmv = db::DVector (mv) - db::DVector (ne->first.d ()) * (double (db::sprod (mv, ne->first.d ())) / ne->first.d ().sq_double_length ());
    }

    db::Point p1 = ne->second.p1 (), p2 = ne->second.p2 ();
    db::Point p1e = p1, p2e = p2;

    c1 = point_constr.find (ne->first.pi1 ());
    if (c1 != point_constr.end ()) {
      std::pair <bool, db::Point> pm = c1->second.move (p1, nmv);
      if (pm.first) {
        p1e = p1 = pm.second;
      } else {
        //  if the movement was not possible, create a new "detached" edge 
        p1e = p1 + db::Vector (nmv);
      }
    } else {
      p1 += mv;
      p1e = p1;
    }

    c2 = point_constr.find (ne->first.pi2 ());
    if (c2 != point_constr.end ()) {
      std::pair <bool, db::Point> pm = c2->second.move (p2, nmv);
      if (pm.first) {
        p2e = p2 = pm.second;
      } else {
        //  if the movement was not possible, create a new "detached" edge 
        p2e = p2 + db::Vector (nmv);
      }
    } else {
      p2 += mv;
      p2e = p2;
    }

    //  if the moved edge is 
    //    1. result of two constraints
    //    ( commented out: 2. inverted (the direction has changed) or the length grows 4x larger than the move distance )
    //  then create a "detached edge" as well

    db::Vector ve (p2e - p1e);
    db::Vector vo (ne->second.p2 () - ne->second.p1 ());

    if (c1 != point_constr.end () && c2 != point_constr.end () 
        && (/* db::sprod_sign (ve, vo) <= 0 || */ (ve - vo).double_length () > 4.0 * nmv.double_length ())) {

#if 0
      //  first try to detach just one (the one with the largest movement)
      if (p2e.sq_double_distance (ne->second.p2 ()) > p1e.sq_double_distance (ne->second.p1 ())) {
        p2 = ne->second.p2 ();
        p2e = p2 + db::Vector (nmv);
      } else {
        p1 = ne->second.p1 ();
        p1e = p1 + db::Vector (nmv);
      }

      //  if that still is not sufficient to avoid inversion, do both
      if (db::sprod_sign (db::Vector (p2e - p1e), db::Vector (ne->second.p2 () - ne->second.p1 ())) <= 0) {
        p2 = ne->second.p2 ();
        p1 = ne->second.p1 ();
        p2e = p2 + db::Vector (nmv);
        p1e = p1 + db::Vector (nmv);
      }
#else
      //  this approach is more simple: just create the detached edge ..
      p2 = ne->second.p2 ();
      p1 = ne->second.p1 ();
      p2e = p2 + db::Vector (nmv);
      p1e = p1 + db::Vector (nmv);
#endif

    }

    ne->second = db::Edge (p1e, p2e);

    //  insert the end points into the point list in order to find them by looking up a point alone
    new_points.insert (std::make_pair (ne->first.pi1 (), db::Point ())).first->second = p1;
    new_points.insert (std::make_pair (ne->first.pi2 (), db::Point ())).first->second = p2;

  }

}

// -------------------------------------------------------------
//  PartialShapeFinder declaration

/**
 *  @brief Partial shape finder utility class
 *
 *  This class specializes the finder to finding vertices or edges of shapes.
 */
class PartialShapeFinder
  : public lay::ShapeFinder
{
public:
  typedef std::vector<std::pair <lay::ObjectInstPath, std::vector <EdgeWithIndex> > > founds_vector_type;
  typedef founds_vector_type::const_iterator iterator;

  PartialShapeFinder (bool point_mode, bool top_level_sel, db::ShapeIterator::flags_type flags);

  iterator begin () const
  {
    return m_founds.begin ();
  }

  iterator end () const
  {
    return m_founds.end ();
  }

private:
  virtual void visit_cell (const db::Cell &cell, const db::Box &hit_box, const db::Box &scan_box, const db::DCplxTrans &vp, const db::ICplxTrans &t, int level);

  founds_vector_type m_founds;
};

// -------------------------------------------------------------
//  PartialShapeFinder implementation

PartialShapeFinder::PartialShapeFinder (bool point_mode, bool top_level_sel, db::ShapeIterator::flags_type flags)
  : lay::ShapeFinder (point_mode, top_level_sel, flags, 0)
{
  set_test_count (point_sel_tests);
}

void 
PartialShapeFinder::visit_cell (const db::Cell &cell, const db::Box &hit_box, const db::Box &scan_box, const db::DCplxTrans &vp, const db::ICplxTrans &t, int /*level*/)
{
  if (! point_mode ()) {

    for (std::vector<int>::const_iterator l = layers ().begin (); l != layers ().end (); ++l) {

      if (layers ().size () == 1 || (layers ().size () > 1 && cell.bbox ((unsigned int) *l).touches (scan_box))) {

        checkpoint ();

        const db::Shapes &shapes = cell.shapes (*l);

        db::ShapeIterator shape = shapes.begin_touching (scan_box, flags (), prop_sel (), inv_prop_sel ());
        while (! shape.at_end ()) {

          checkpoint ();

          m_founds.push_back (founds_vector_type::value_type ());

          lay::ObjectInstPath &inst_path = m_founds.back ().first;
          std::vector<EdgeWithIndex> &edges = m_founds.back ().second;

          inst_path.set_cv_index (cv_index ());
          inst_path.set_topcell (topcell ());
          inst_path.assign_path (path ().begin (), path ().end ());
          inst_path.set_layer (*l);
          inst_path.set_shape (*shape);

          //  in box mode, select the edges depending on whether an endpoint is inside the
          //  box or not
          if (shape->is_polygon ()) {

            for (unsigned int c = 0; c < shape->holes () + 1; ++c) {

              unsigned int n = 0; 
              db::Shape::polygon_edge_iterator ee;
              for (db::Shape::polygon_edge_iterator e = shape->begin_edge (c); ! e.at_end (); e = ee, ++n) {

                ee = e;
                ++ee;
                unsigned int nn = ee.at_end () ? 0 : n + 1;

                if (hit_box.contains ((*e).p1 ())) {
                  edges.push_back (EdgeWithIndex (db::Edge ((*e).p1 (), (*e).p1 ()), n, n, c));
                  if (hit_box.contains ((*e).p2 ())) {
                    edges.push_back (EdgeWithIndex (*e, n, nn, c));
                  } 
                } 

              }

            }

          } else if (shape->is_path ()) {

            bool pl_set = false;
            db::Point pl;
            unsigned int n = 0;
            for (db::Shape::point_iterator pt = shape->begin_point (); pt != shape->end_point (); ++pt, ++n) {
              if (hit_box.contains (*pt)) {
                edges.push_back (EdgeWithIndex (db::Edge (*pt, *pt), n, n, 0));
                if (pl_set && hit_box.contains (pl)) {
                  edges.push_back (EdgeWithIndex (db::Edge (pl, *pt), n - 1, n, 0));
                }
              }
              pl = *pt;
              pl_set = true;
            }

          } else if (shape->is_box ()) {

            const db::Box &box = shape->box ();

            //  convert to polygon and test those edges
            db::Polygon poly (box);
            unsigned int n = 0; 
            db::Shape::polygon_edge_iterator ee;
            for (db::Shape::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end (); e = ee, ++n) {

              ee = e;
              ++ee;
              unsigned int nn = ee.at_end () ? 0 : n + 1;

              if (hit_box.contains ((*e).p1 ())) {
                edges.push_back (EdgeWithIndex (db::Edge ((*e).p1 (), (*e).p1 ()), n, n, 0));
                if (hit_box.contains ((*e).p2 ())) {
                  edges.push_back (EdgeWithIndex (*e, n, nn, 0));
                } 
              } 

            }

          } else if (shape->is_point ()) {

            db::Point tp (shape->point ());

            if (hit_box.contains (tp)) {
              edges.push_back (EdgeWithIndex (db::Edge (tp, tp), 0, 0, 0));
            }

          } else if (shape->is_text ()) {

            db::Point tp (shape->text_trans () * db::Point ());

            if (text_info () && ! text_info ()->point_mode ()) {

              db::CplxTrans t_dbu = db::CplxTrans (layout ().dbu ()) * t;
              db::Text text;
              shape->text (text);
              db::Box tb = t_dbu.inverted () * text_info ()->bbox (t_dbu * text, vp);
              if (tb.inside (hit_box)) {
                edges.push_back (EdgeWithIndex (db::Edge (tp, tp), 0, 0, 0));
              }

            } else {

              if (hit_box.contains (tp)) {
                edges.push_back (EdgeWithIndex (db::Edge (tp, tp), 0, 0, 0));
              }

            }

          }

          //  do not select shapes that do not have at least one edge selected
          if (edges.empty ()) {
            m_founds.pop_back ();
          }

          ++shape;

        } 

      }

    }

  } else {

    for (std::vector<int>::const_iterator l = layers ().begin (); l != layers ().end (); ++l) {

      if (layers ().size () == 1 || (layers ().size () > 1 && cell.bbox ((unsigned int) *l).touches (hit_box))) {

        checkpoint ();

        const db::Shapes &shapes = cell.shapes (*l);
        std::vector <EdgeWithIndex> edge_sel;

        //  two passes - one with points, second with edges

        bool any = false;
        for (int pass = 0; pass < 2 && ! any; ++pass) {

          db::ShapeIterator shape = shapes.begin_touching (scan_box, flags (), prop_sel (), inv_prop_sel ());
          while (! shape.at_end ()) {

            bool match = false;
            double d = std::numeric_limits<double>::max ();

            edge_sel.clear ();

            checkpoint ();

            //  in point mode, test the edges and use a "closest" criterion
            if (shape->is_polygon ()) {

              for (unsigned int c = 0; c < shape->holes () + 1; ++c) {

                unsigned int n = 0;
                db::Shape::polygon_edge_iterator ee;
                for (db::Shape::polygon_edge_iterator e = shape->begin_edge (c); ! e.at_end (); e = ee, ++n) {

                  ee = e;
                  ++ee;
                  unsigned int nn = ee.at_end () ? 0 : n + 1;

                  unsigned int r = test_edge (t, *e, pass == 0, d, match);
                  if (r) {
                    edge_sel.clear ();
                    if ((r & 1) != 0) {
                      edge_sel.push_back (EdgeWithIndex (db::Edge ((*e).p1 (), (*e).p1 ()), n, n, c));
                    }
                    if ((r & 2) != 0) {
                      edge_sel.push_back (EdgeWithIndex (db::Edge ((*e).p2 (), (*e).p2 ()), nn, nn, c));
                    }
                    if (r == 3) {
                      edge_sel.push_back (EdgeWithIndex (*e, n, nn, c));
                    }
                  }

                }

              }

            } else if (shape->is_path ()) {

              //  test the "spine"
              db::Shape::point_iterator pt = shape->begin_point ();
              if (pt != shape->end_point ()) {
                db::Point p (*pt);
                ++pt;
                unsigned int n = 0;
                for (; pt != shape->end_point (); ++pt, ++n) {
                  unsigned int r = test_edge (t, db::Edge (p, *pt), pass == 0, d, match);
                  if (r) {
                    edge_sel.clear ();
                    if ((r & 1) != 0) {
                      edge_sel.push_back (EdgeWithIndex (db::Edge (p, p), n, n, 0));
                    }
                    if ((r & 2) != 0) {
                      edge_sel.push_back (EdgeWithIndex (db::Edge (*pt, *pt), n + 1, n + 1, 0));
                    }
                    if (r == 3) {
                      edge_sel.push_back (EdgeWithIndex (db::Edge (p, *pt), n, n + 1, 0));
                    }
                  }
                  p = *pt;
                }
              }

            } else if (shape->is_box ()) {

              const db::Box &box = shape->box ();

              //  convert to polygon and test those edges
              db::Polygon poly (box);

              unsigned int n = 0;
              db::Shape::polygon_edge_iterator ee;
              for (db::Shape::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end (); e = ee, ++n) {

                ee = e;
                ++ee;
                unsigned int nn = ee.at_end () ? 0 : n + 1;

                unsigned int r = test_edge (t, *e, pass == 0, d, match);
                if (r) {
                  edge_sel.clear ();
                  if ((r & 1) != 0) {
                    edge_sel.push_back (EdgeWithIndex (db::Edge ((*e).p1 (), (*e).p1 ()), n, n, 0));
                  }
                  if ((r & 2) != 0) {
                    edge_sel.push_back (EdgeWithIndex (db::Edge ((*e).p2 (), (*e).p2 ()), nn, nn, 0));
                  }
                  if (r == 3) {
                    edge_sel.push_back (EdgeWithIndex (*e, n, nn, 0));
                  }
                }

              }

            } else if (shape->is_point ()) {

              db::Point tp (shape->point ());

              if (hit_box.contains (tp)) {
                d = tp.distance (hit_box.center ());
                edge_sel.clear ();
                edge_sel.push_back (EdgeWithIndex (db::Edge (tp, tp), 0, 0, 0));
                match = true;
              }

            } else if (shape->is_text ()) {

              db::Point tp (shape->text_trans () * db::Point ());

              if (text_info () && ! text_info ()->point_mode ()) {

                db::CplxTrans t_dbu = db::CplxTrans (layout ().dbu ()) * t;
                db::Text text;
                shape->text (text);
                db::Box tb (t_dbu.inverted () * text_info ()->bbox (t_dbu * text, vp));
                if (tb.contains (hit_box.center ())) {
                  d = tp.distance (hit_box.center ());
                  edge_sel.clear ();
                  edge_sel.push_back (EdgeWithIndex (db::Edge (tp, tp), 0, 0, 0));
                  match = true;
                }

              } else {

                if (hit_box.contains (tp)) {
                  d = tp.distance (hit_box.center ());
                  edge_sel.clear ();
                  edge_sel.push_back (EdgeWithIndex (db::Edge (tp, tp), 0, 0, 0));
                  match = true;
                }

              }

            }

            if (match && closer (d)) {

              //  in point mode just store that found that has the least "distance"
              if (m_founds.empty ()) {
                m_founds.push_back (founds_vector_type::value_type ());
              }

              lay::ObjectInstPath &inst_path = m_founds.back ().first;

              inst_path.set_cv_index (cv_index ());
              inst_path.set_topcell (topcell ());
              inst_path.assign_path (path ().begin (), path ().end ());
              inst_path.set_layer (*l);
              inst_path.set_shape (*shape);

              m_founds.back ().second = edge_sel;

              any = true;

            }

            ++shape;

          }

        }

      }

    }

  }

}

// -----------------------------------------------------------------------------
//  Main Service implementation

PartialService::PartialService (db::Manager *manager, lay::LayoutViewBase *view, lay::Dispatcher *root) :
#if defined(HAVE_QT)
    QObject (),
#endif
    lay::EditorServiceBase (view),
    db::Object (manager),
    mp_view (view),
    mp_root (root),
    m_dragging (false),
    m_keep_selection (true),
    mp_box (0),
    m_color (0),
    m_buttons (0),
    m_connect_ac (lay::AC_Any), m_move_ac (lay::AC_Any), m_alt_ac (lay::AC_Global),
    m_snap_to_objects (true),
    m_snap_objects_to_grid (true),
    m_top_level_sel (false),
    m_hover (false),
    m_hover_wait (false),
    dm_selection_to_view (this, &edt::PartialService::do_selection_to_view)
{
#if defined(HAVE_QT)
  m_timer.setInterval (100 /*hover time*/);
  m_timer.setSingleShot (true);
  connect (&m_timer, SIGNAL (timeout ()), this, SLOT (timeout ()));
#endif

  mp_view->geom_changed_event.add (this, &edt::PartialService::selection_to_view);
}

PartialService::~PartialService ()
{
  resize_markers (0, true);
  resize_markers (0, false);
  resize_inst_markers (0, true);
  resize_inst_markers (0, false);

  if (mp_box) {
    delete mp_box;
    mp_box = 0;
  }
}

lay::angle_constraint_type 
PartialService::connect_ac () const
{
  //  m_alt_ac (which is set from mouse buttons) can override the specified connect angle constraint
  return m_alt_ac != lay::AC_Global ? m_alt_ac : m_connect_ac;
}

lay::angle_constraint_type 
PartialService::move_ac () const
{
  //  m_alt_ac (which is set from mouse buttons) can override the specified move angle constraint
  return m_alt_ac != lay::AC_Global ? m_alt_ac : m_move_ac;
}

void  
PartialService::deactivated ()
{
  clear_partial_transient_selection ();
}

void  
PartialService::activated ()
{
  //  .. nothing yet ..
}

void 
PartialService::hover_reset ()
{
  if (m_hover_wait) {
#if defined(HAVE_QT)
    m_timer.stop ();
#endif
    m_hover_wait = false;
  }
  if (m_hover) {
    clear_partial_transient_selection ();
    m_hover = false;
  }
}

//  TODO: should receive timer calls from regular timer update
#if defined(HAVE_QT)
void
PartialService::timeout ()
{
  m_hover_wait = false;
  m_hover = true;
  
  mp_view->clear_transient_selection ();
  clear_mouse_cursors ();

  //  compute search box
  double l = catch_distance ();
  db::DBox search_box = db::DBox (m_hover_point, m_hover_point).enlarged (db::DVector (l, l));

  PartialShapeFinder finder (true, m_top_level_sel, db::ShapeIterator::All);
  finder.find (view (), search_box);

  size_t n_marker = 0;
  size_t n_inst_marker = 0;

  if (finder.begin () != finder.end ()) {

    partial_objects transient_selection;
    transient_selection.insert (std::make_pair (finder.begin ()->first, std::set <EdgeWithIndex> (finder.begin ()->second.begin (), finder.begin ()->second.end ())));

    partial_objects::const_iterator r = transient_selection.begin (); 

    //  build the transformation variants cache
    TransformationVariants tv (view ());

    const lay::CellView &cv = view ()->cellview (r->first.cv_index ());

    //  compute the global transformation including context and explicit transformation
    db::ICplxTrans gt = (cv.context_trans () * r->first.trans ());

    if (! r->first.is_cell_inst ()) {

      const std::vector<db::DCplxTrans> *tv_list = tv.per_cv_and_layer (r->first.cv_index (), r->first.layer ());
      if (tv_list && !tv_list->empty ()) {

        //  dummy shift set
        std::map <EdgeWithIndex, db::Edge> new_edges;
        std::map <PointWithIndex, db::Point> new_points;

        //  create the markers to represent vertices and edges
        enter_vertices (n_marker, r, new_points, new_edges, gt, *tv_list, true);

        if (r->first.shape ().is_polygon ()) {

          for (unsigned int c = 0; c < r->first.shape ().holes () + 1; ++c) {

            unsigned int n = 0;
            db::Shape::polygon_edge_iterator ee;
            for (db::Shape::polygon_edge_iterator e = r->first.shape ().begin_edge (c); ! e.at_end (); e = ee, ++n) {
              ee = e;
              ++ee;
              unsigned int nn = ee.at_end () ? 0 : n + 1;
              enter_edge (EdgeWithIndex (*e, n, nn, c), n_marker, r, new_points, new_edges, gt, *tv_list, true);
            }

          }

        } else if (r->first.shape ().is_path ()) {

          if (r->first.shape ().begin_point () != r->first.shape ().end_point ()) {

            db::Shape::point_iterator pt = r->first.shape ().begin_point ();
            db::Point p1 = *pt;

            unsigned int n = 0;
            while (true) {

              ++pt;
              if (pt == r->first.shape ().end_point ()) {
                break;
              }
              
              enter_edge (EdgeWithIndex (db::Edge (p1, *pt), n, n + 1, 0), n_marker, r, new_points, new_edges, gt, *tv_list, true);

              p1 = *pt;
              ++n;

            }

          }

        } else if (r->first.shape ().is_box ()) {

          //  convert to polygon and test those edges
          db::Polygon poly (r->first.shape ().box ());
          unsigned int n = 0;
          db::Shape::polygon_edge_iterator ee;
          for (db::Shape::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end (); e = ee, ++n) {
            ee = e;
            ++ee;
            unsigned int nn = ee.at_end () ? 0 : n + 1;
            enter_edge (EdgeWithIndex (*e, n, nn, 0), n_marker, r, new_points, new_edges, gt, *tv_list, true);
          }

        } else if (r->first.shape ().is_text ()) {

          db::Point tp (r->first.shape ().text_trans () * db::Point ());
          enter_edge (EdgeWithIndex (db::Edge (tp, tp), 0, 0, 0), n_marker, r, new_points, new_edges, gt, *tv_list, true);

        } else if (r->first.shape ().is_point ()) {

          db::Point tp (r->first.shape ().point ());
          enter_edge (EdgeWithIndex (db::Edge (tp, tp), 0, 0, 0), n_marker, r, new_points, new_edges, gt, *tv_list, true);

        }

      }

    } else {

      const std::vector<db::DCplxTrans> *tv_list = tv.per_cv (r->first.cv_index ());
      if (tv_list  && ! tv_list->empty ()) {
        lay::InstanceMarker *marker = new_inst_marker (n_inst_marker, r->first.cv_index (), true);
        marker->set (r->first.back ().inst_ptr, gt, *tv_list);
      }

    }

  }

  //  delete superfluous markers
  resize_markers (n_marker, true);
  resize_inst_markers (n_inst_marker, true);

}
#endif

void
PartialService::clear_partial_transient_selection ()
{
  mp_view->clear_transient_selection ();

  resize_markers (0, true);
  resize_inst_markers (0, true);
}

void 
PartialService::set_colors (tl::Color /*background*/, tl::Color color)
{
  m_color = color.rgb ();
  if (mp_box) {
    mp_box->set_color (m_color);
  }
}

void 
PartialService::menu_activated (const std::string & /*symbol*/)
{
  // ...
}

bool 
PartialService::configure (const std::string &name, const std::string &value)
{
  edt::EditGridConverter egc;
  edt::ACConverter acc;

  if (name == cfg_edit_global_grid) {
    egc.from_string (value, m_global_grid);
  } else if (name == cfg_edit_grid) {
    egc.from_string (value, m_edit_grid);
    return true;  //  taken
  } else if (name == cfg_edit_snap_to_objects) {
    tl::from_string (value, m_snap_to_objects);
    return true;  //  taken
  } else if (name == cfg_edit_snap_objects_to_grid) {
    tl::from_string (value, m_snap_objects_to_grid);
    return true;  //  taken
  } else if (name == cfg_edit_move_angle_mode) {
    acc.from_string (value, m_move_ac);
    return true;  //  taken
  } else if (name == cfg_edit_connect_angle_mode) {
    acc.from_string (value, m_connect_ac);
    return true;  //  taken
  } else if (name == cfg_edit_top_level_selection) {
    tl::from_string (value, m_top_level_sel);
  }

  return false;  //  not taken
}

void 
PartialService::config_finalize ()
{
  // ...
}

db::DPoint 
PartialService::snap (const db::DPoint &p) const
{
  //  snap according to the grid
  if (m_edit_grid == db::DVector ()) {
    return lay::snap_xy (p, m_global_grid);
  } else if (m_edit_grid.x () >= 1e-6) {
    return lay::snap_xy (p, m_edit_grid);
  } else {
    return p;
  }
}

db::DVector
PartialService::snap (const db::DVector &v_org) const
{
  db::DVector v = lay::snap_angle (v_org, move_ac ());

  //  snap according to the grid
  if (m_edit_grid == db::DVector ()) {
    return lay::snap_xy (db::DPoint () + v, m_global_grid) - db::DPoint ();
  } else if (m_edit_grid.x () >= 1e-6) {
    return lay::snap_xy (db::DPoint () + v, m_edit_grid) - db::DPoint ();
  } else {
    return v;
  }
}

const int sr_pixels = 8; //  TODO: make variable

lay::PointSnapToObjectResult
PartialService::snap2 (const db::DPoint &p) const
{
  double snap_range = ui ()->mouse_event_trans ().inverted ().ctrans (sr_pixels);
  return lay::obj_snap (m_snap_to_objects ? view () : 0, m_start, p, m_edit_grid == db::DVector () ? m_global_grid : m_edit_grid, move_ac (), snap_range);
}

void
PartialService::transform (const db::DCplxTrans &tr)
{
  //  ignore this function is non-editable mode
  if (! view ()->is_editable ()) {
    return;
  }

  //  just allow displacements
  db::DTrans move_trans (tr.disp ());
  transform_selection (move_trans);

  selection_to_view ();
}

void  
PartialService::transform_selection (const db::DTrans &move_trans)
{
  //  build the transformation variants cache
  TransformationVariants tv (view ());

  //  since a shape reference may become invalid while moving it and
  //  because it creates ambiguities, we treat each shape separately:
  //  collect the valid selected items in a selection-per-shape map.
  std::map <db::Shape, std::vector<partial_objects::iterator> > sel_per_shape;

  for (partial_objects::iterator r = m_selection.begin (); r != m_selection.end (); ++r) {
    if (! r->first.is_cell_inst ()) {
      const std::vector<db::DCplxTrans> *tv_list = tv.per_cv_and_layer (r->first.cv_index (), r->first.layer ());
      if (tv_list && ! tv_list->empty ()) {
        sel_per_shape.insert (std::make_pair (r->first.shape (), std::vector<partial_objects::iterator> ())).first->second.push_back (r);
      }
    }
  }

  for (std::map <db::Shape, std::vector<partial_objects::iterator> >::iterator sps = sel_per_shape.begin (); sps != sel_per_shape.end (); ++sps) {

    db::Shape shape = sps->first;

    for (std::vector<partial_objects::iterator>::const_iterator rr = sps->second.begin (); rr != sps->second.end (); ++rr) {

      partial_objects::iterator r = *rr;

      const lay::CellView &cv = view ()->cellview (r->first.cv_index ());

      //  use only the first one of the explicit transformations 
      //  TODO: clarify how this can be implemented in a more generic form or leave it thus.

      db::ICplxTrans gt (cv.context_trans () * r->first.trans ());
      const std::vector<db::DCplxTrans> *tv_list = tv.per_cv_and_layer (r->first.cv_index (), r->first.layer ());
      db::CplxTrans tt = (*tv_list) [0] * db::CplxTrans (cv->layout ().dbu ()) * gt;
      db::Vector move_vector = db::Vector ((tt.inverted () * db::DCplxTrans (move_trans) * tt).disp ());

      std::map <EdgeWithIndex, db::Edge> new_edges;
      std::map <PointWithIndex, db::Point> new_points;
      create_shift_sets (shape, r->second, new_points, new_edges, move_vector);

      //  modify the shapes and insert

      db::Shapes &shapes = cv->layout ().cell (r->first.cell_index ()).shapes (r->first.layer ());

      if (shape.is_polygon ()) {

        db::Polygon poly;
        shape.polygon (poly);

        //  warning: poly is modified:
        modify_polygon (poly, new_points, new_edges, true /*compress*/);

        shape = shapes.replace (shape, poly);

      } else if (shape.is_path ()) {

        db::Path path;
        shape.path (path);

        //  warning: path is modified:
        modify_path (path, new_points, new_edges, true /*compress*/);

        shape = shapes.replace (shape, path);

      } else if (shape.is_box ()) {

        db::Polygon poly;
        shape.polygon (poly);

        //  warning: poly is modified:
        modify_polygon (poly, new_points, new_edges, true /*compress*/);

        shape = shapes.replace (shape, poly.box ());

      } else if (shape.is_text ()) {

        db::Text t;
        shape.text (t);

        db::Point tp (shape.text_trans () * db::Point ());
        std::map <PointWithIndex, db::Point>::const_iterator np = new_points.find (PointWithIndex (tp, 0, 0));

        if (np != new_points.end ()) {
          t.transform (db::Trans (np->second - tp));
          shape = shapes.replace (shape, t);
        }

      } else if (shape.is_point ()) {

        db::Point p;
        shape.point (p);

        std::map <PointWithIndex, db::Point>::const_iterator np = new_points.find (PointWithIndex (p, 0, 0));

        if (np != new_points.end ()) {
          shape = shapes.replace (shape, np->second);
        }

      }

      //  transform the selection 
      
      std::set <EdgeWithIndex> new_sel;
      new_sel.swap (r->second);

      for (std::set <EdgeWithIndex>::const_iterator s = new_sel.begin (); s != new_sel.end () && m_keep_selection; ++s) {
        if (s->p1 () == s->p2 ()) {
          std::map <PointWithIndex, db::Point>::const_iterator np = new_points.find (s->pi1 ());
          if (np != new_points.end ()) {
            r->second.insert (EdgeWithIndex (db::Edge (np->second, np->second), s->n, s->n, s->c));
          } else {
            r->second.insert (*s);
          }
        } else {
          std::map <EdgeWithIndex, db::Edge>::const_iterator ne = new_edges.find (*s);
          if (ne != new_edges.end ()) {
            r->second.insert (EdgeWithIndex (ne->second, s->n, s->nn, s->c));
          } else {
            r->second.insert (*s);
          }
        }
      }

    }

    //  change the shape references if required
      
    if (shape != sps->first) {

      for (std::vector<partial_objects::iterator>::const_iterator rr = sps->second.begin (); rr != sps->second.end (); ++rr) {

        std::set <EdgeWithIndex> sel;
        sel.swap ((*rr)->second);

        lay::ObjectInstPath inst_path ((*rr)->first);
        inst_path.set_shape (shape);

        m_selection.erase ((*rr)->first);
        m_selection.insert (std::make_pair (inst_path, std::set <EdgeWithIndex> ())).first->second.swap (sel);

      }

    }

  }

  //  then move all instances.
  
  //  sort the selected objects (the instances) by the cell they are in
  //  The key is a pair: cell_index, cv_index
  std::map <std::pair <db::cell_index_type, unsigned int>, std::vector <partial_objects::const_iterator> > insts_by_cell;
  for (partial_objects::const_iterator r = m_selection.begin (); r != m_selection.end (); ++r) {
    if (r->first.is_cell_inst ()) {
      insts_by_cell.insert (std::make_pair (std::make_pair (r->first.cell_index (), r->first.cv_index ()), std::vector <partial_objects::const_iterator> ())).first->second.push_back (r);
    }
  }

  std::vector <std::pair <db::Instance, db::ICplxTrans> > insts_to_transform;
  for (std::map <std::pair <db::cell_index_type, unsigned int>, std::vector <partial_objects::const_iterator> >::const_iterator ibc = insts_by_cell.begin (); ibc != insts_by_cell.end (); ++ibc) {

    insts_to_transform.clear ();
    insts_to_transform.reserve (ibc->second.size ());
    for (std::vector <partial_objects::const_iterator>::const_iterator i = ibc->second.begin (); i != ibc->second.end (); ++i) {
      insts_to_transform.push_back (std::make_pair ((*i)->first.back ().inst_ptr, (*i)->first.trans ()));
    }

    const lay::CellView &cv = view ()->cellview (ibc->first.second);
    if (cv.is_valid ()) {

      const std::vector<db::DCplxTrans> *tv_list = tv.per_cv (ibc->first.second);
      if (tv_list && ! tv_list->empty ()) {

        db::CplxTrans tt = (*tv_list) [0] * db::CplxTrans (cv->layout ().dbu ()) * cv.context_trans ();
        db::ICplxTrans move_trans_dbu (tt.inverted () * db::DCplxTrans (move_trans) * tt);

        std::sort (insts_to_transform.begin (), insts_to_transform.end ());
        std::vector <std::pair <db::Instance, db::ICplxTrans> >::const_iterator unique_end = std::unique (insts_to_transform.begin (), insts_to_transform.end ());
        db::Cell &cell = cv->layout ().cell (ibc->first.first);
        for (std::vector <std::pair <db::Instance, db::ICplxTrans> >::const_iterator inst = insts_to_transform.begin (); inst != unique_end; ++inst) {

          db::ICplxTrans mt (inst->second.inverted () * move_trans_dbu * inst->second);
          cell.transform (inst->first, mt);

        }

      }

    }

  }

  handle_guiding_shape_changes ();
}

void 
PartialService::edit_cancel ()
{
  //  stop dragging, clear selection
  m_dragging = false;

  if (mp_box) {
    delete mp_box;
    mp_box = 0;
  }

  ui ()->ungrab_mouse (this);

  selection_to_view ();
}

bool 
PartialService::wheel_event (int /*delta*/, bool /*horizontal*/, const db::DPoint & /*p*/, unsigned int /*buttons*/, bool /*prio*/)
{
  hover_reset ();
  return false;
}

bool  
PartialService::mouse_move_event (const db::DPoint &p, unsigned int buttons, bool prio)
{
  clear_mouse_cursors ();

  if (m_dragging) {

    set_cursor (lay::Cursor::size_all);

    m_alt_ac = ac_from_buttons (buttons);

    //  drag the vertex or edge/segment
    if (is_single_point_selection () || is_single_edge_selection ()) {

      lay::PointSnapToObjectResult snap_details;

      //  for a single selected point or edge, m_start is the original position and we snap the target -
      //  thus, we can bring the point on grid or to an object's edge or vertex
      snap_details = snap2 (p);
      if (snap_details.object_snap == lay::PointSnapToObjectResult::NoObject) {
        m_current = m_start + snap_move (p - m_start);
      } else {
        m_current = snap_details.snapped_point;
        mouse_cursor_from_snap_details (snap_details);
      }

    } else {

      //  snap movement to angle and grid without object
      m_current = m_start + snap_move (p - m_start);
      clear_mouse_cursors ();

    }

    selection_to_view ();

    m_alt_ac = lay::AC_Global;

  } else if (prio) {
    
    if (mp_box) {

      m_alt_ac = ac_from_buttons (buttons);

      m_p2 = p;
      mp_box->set_points (m_p1, m_p2);

      m_alt_ac = lay::AC_Global;

    } else if (view ()->transient_selection_mode ()) {

      m_hover_wait = true;
#if defined(HAVE_QT)
      m_timer.start ();
#endif
      m_hover_point = p;

    }

  } 

  //  pass on this event to other handlers
  return false;
}

static db::DPoint
projected_to_edge (const db::DEdge &edge, const db::DPoint &p)
{
  if (edge.is_degenerate ()) {
    return edge.p1 ();
  } else {
    db::DVector v = edge.d () * (1.0 / edge.length ());
    return edge.p1 () + v * db::sprod (p - edge.p1 (), v);
  }
}

bool  
PartialService::mouse_press_event (const db::DPoint &p, unsigned int buttons, bool prio)
{
  hover_reset ();

  if (! view ()->is_editable ()) {
    return false;
  }

  //  only respond to left button clicks
  if ((buttons & lay::LeftButton) == 0) {
    return false;
  }

  //  only respond to first order events
  if (! prio) {
    return false;
  }

  if (m_dragging) {

    //  eat events if already dragging
    return true;

  } else if (! mp_box) {

    m_alt_ac = ac_from_buttons (buttons);

    if (m_selection.empty ()) {

      //  clear other selection when this mode gets active
      view ()->clear_selection ();

      //  nothing is selected yet: 
      //  try to select something here.
      //  (select is allowed to throw an exception)
      
      try {
        partial_select (db::DBox (p, p), lay::Editable::Replace);
      } catch (tl::Exception &ex) {
        show_error (ex);
        //  clear selection
        partial_select (db::DBox (), lay::Editable::Reset);
      }

    }

    if (m_selection.empty () || ((buttons & lay::ShiftButton) != 0) || ((buttons & lay::ControlButton) != 0)) {

      //  if nothing was selected by this point or Ctrl or Shift was pressed, start dragging a box

      view ()->stop_redraw (); // TODO: how to restart if selection is aborted?
      m_buttons = buttons;

      if (mp_box) {
        delete mp_box;
      }

      m_p1 = p;
      m_p2 = p;
      mp_box = new lay::RubberBox (ui (), m_color, p, p);
      mp_box->set_stipple (6); // coarse hatched

      ui ()->grab_mouse (this, true);

    } else {

      //  something was selected: start dragging this ..
      m_dragging = true;
      m_keep_selection = true;

      if (is_single_point_selection ()) {
        //  for a single selected point we use the original point as the start location which 
        //  allows bringing it to grid
        m_current = m_start = single_selected_point ();
      } else if (is_single_edge_selection ()) {
        //  for an edge selection use the point projected to edge as the start location which
        //  allows bringing it to grid
        m_current = m_start = projected_to_edge (single_selected_edge (), p);
      } else {
        m_current = m_start = p;
      }

      ui ()->grab_mouse (this, true);

    }

    m_alt_ac = lay::AC_Global;

    return true;

  }

  return false;
}

bool  
PartialService::mouse_click_event (const db::DPoint &p, unsigned int buttons, bool prio)
{
  hover_reset ();

  if (! view ()->is_editable ()) {
    return false;
  }

  //  only respond to left button clicks
  if ((buttons & lay::LeftButton) == 0) {
    return false;
  }

  //  only respond to first order events
  if (! prio) {
    return false;
  }

  if (m_dragging) {

    m_alt_ac = ac_from_buttons (buttons);

    if (m_current != m_start) {

      //  stop dragging
      ui ()->ungrab_mouse (this);
      
      if (manager ()) {
        manager ()->transaction (tl::to_string (tr ("Partial move")));
      }

      //  heuristically, if there is just one edge selected: do not confine to the movement
      //  angle constraint - the edge usually is confined enough
      db::DTrans move_trans = db::DTrans (m_current - m_start);

      transform_selection (move_trans);

      if (manager ()) {
        manager ()->commit ();
      }

    }

    if (! m_keep_selection) {
      m_selection.clear ();
    }

    m_dragging = false;
    selection_to_view ();

    m_alt_ac = lay::AC_Global;

    return true;

  } else if (ui ()->mouse_event_viewport ().contains (p)) { 

    //  clear other selection when this mode gets active
    //  (save the selection so our own selection does not get cleared)
    partial_objects selection = m_selection;
    view ()->clear_selection ();
    m_selection = selection;

    m_alt_ac = ac_from_buttons (buttons);

    lay::Editable::SelectionMode mode = lay::Editable::Replace;
    bool shift = ((buttons & lay::ShiftButton) != 0);
    bool ctrl = ((buttons & lay::ControlButton) != 0);
    if (shift && ctrl) {
      mode = lay::Editable::Invert;
    } else if (shift) {
      mode = lay::Editable::Add;
    } else if (ctrl) {
      mode = lay::Editable::Reset;
    } 

    //  select is allowed to throw an exception 
    try {

      //  compute search box
      double l = catch_distance ();
      db::DBox search_box = db::DBox (p, p).enlarged (db::DVector (l, l));

      //  check, if there is a selected shape under the mouse - in this case, we do not do a new selection
      PartialShapeFinder finder (true /*point mode*/, m_top_level_sel, db::ShapeIterator::All);
      finder.find (view (), search_box);

      //  check, if there is a selected shape under the mouse - in this case, we do not do a new selection
      lay::InstFinder inst_finder (true /*point mode*/, m_top_level_sel, true /*full arrays*/, true /*enclose*/, 0 /*no excludes*/, true /*visible layers*/);
      inst_finder.find (view (), search_box);

      //  collect the founds from the finder
      //  consider a new selection if new objects are selected or the current selection is shape-only 
      //  (this may happen if points have been inserted)
      bool new_selection = ((finder.begin () == finder.end () && inst_finder.begin () == inst_finder.end ()) 
                             || mode != lay::Editable::Replace);

      for (PartialShapeFinder::iterator f = finder.begin (); f != finder.end () && ! new_selection; ++f) {
        partial_objects::const_iterator sel = m_selection.find (f->first);
        new_selection = true;
        if (sel != m_selection.end ()) {
          for (std::vector<edt::EdgeWithIndex>::const_iterator e = f->second.begin (); e != f->second.end () && new_selection; ++e) {
            if (sel->second.find (*e) != sel->second.end ()) {
              new_selection = false;
            }
          }
        }
      }

      if (finder.begin () == finder.end ()) {

        for (lay::InstFinder::iterator f = inst_finder.begin (); f != inst_finder.end () && ! new_selection; ++f) {
          partial_objects::const_iterator sel = m_selection.find (*f);
          if (sel == m_selection.end ()) {
            new_selection = true;
          }
        }

      }

      if (new_selection) {

        if (mode == lay::Editable::Replace) {
          m_selection.clear ();
        }

        //  clear the selection if we now select a guiding shape or if it was consisting of a guiding shape before
        //  (that way we ensure there is only a guiding shape selected)
        PartialShapeFinder::iterator f0 = finder.begin ();
        if (f0 != finder.end () && f0->first.layer () == view ()->cellview (f0->first.cv_index ())->layout ().guiding_shape_layer ()) {
          m_selection.clear ();
        } else {
          partial_objects::const_iterator s0 = m_selection.begin (); 
          if (s0 != m_selection.end () && s0->first.layer () == view ()->cellview (s0->first.cv_index ())->layout ().guiding_shape_layer ()) {
            m_selection.clear ();
          }
        }

        //  collect the founds from the finder
        for (PartialShapeFinder::iterator f = finder.begin (); f != finder.end (); ++f) {

          if (mode == lay::Editable::Replace || mode == lay::Editable::Add) {
            //  select
            partial_objects::iterator sel = m_selection.find (f->first);
            if (sel == m_selection.end ()) {
              sel = m_selection.insert (std::make_pair (f->first, std::set <EdgeWithIndex> ())).first;
            } 
            sel->second.insert (f->second.begin (), f->second.end ());
          } else if (mode == lay::Editable::Reset) {
            //  unselect
            if (m_selection.find (f->first) != m_selection.end ()) {
              m_selection.erase (f->first);
            }
          } else {
            //  invert selection
            if (m_selection.find (f->first) != m_selection.end ()) {
              m_selection.erase (f->first);
            } else {
              m_selection.insert (std::make_pair (f->first, std::set<EdgeWithIndex> ())).first->second.insert (f->second.begin (), f->second.end ());
            }
          }

        }

      }
      
      //  start dragging with that single selection
      if (mode == lay::Editable::Replace && ! m_selection.empty ()) {

        m_dragging = true;
        m_keep_selection = ! new_selection;

        if (is_single_point_selection ()) {
          //  for a single selected point we use the original point as the start location which 
          //  allows bringing it to grid
          m_current = m_start = single_selected_point ();
        } else if (is_single_edge_selection ()) {
          //  for an edge selection use the point projected to edge as the start location which
          //  allows bringing it to grid
          m_current = m_start = projected_to_edge (single_selected_edge (), p);
        } else {
          m_current = m_start = p;
        }

      }

      selection_to_view ();

    } catch (tl::Exception &ex) {
      show_error (ex);
      //  clear selection
      partial_select (db::DBox (), lay::Editable::Reset);
    }

    m_alt_ac = lay::AC_Global;

    return true;

  }

  return false;
}

bool  
PartialService::mouse_double_click_event (const db::DPoint &p, unsigned int buttons, bool prio)
{
  hover_reset ();

  if (! view ()->is_editable ()) {
    return false;
  }

  if ((buttons & lay::LeftButton) != 0 && prio) {

    m_alt_ac = ac_from_buttons (buttons);

    //  stop dragging
    ui ()->ungrab_mouse (this);
    m_dragging = false;
      
    partial_select (db::DBox (p, p), lay::Editable::Replace);

    if (! m_selection.empty ()) {

      partial_objects::iterator r = m_selection.begin (); // we assert above that we have at least one selected element
      if (! r->first.is_cell_inst ()) {

        if (manager ()) {
          manager ()->transaction (tl::to_string (tr ("Insert point")));
        }

        //  snap the point
        db::DPoint new_point_d = snap (p);

        //  build the transformation variants cache
        TransformationVariants tv (view (), true /*per cv and layer*/, false /*per cv*/); 

        const std::vector<db::DCplxTrans> *tv_list = tv.per_cv_and_layer (r->first.cv_index (), r->first.layer ());
        if (tv_list && ! tv_list->empty ()) {

          const lay::CellView &cv = view ()->cellview (r->first.cv_index ());

          db::CplxTrans tt = (*tv_list) [0] * db::CplxTrans (cv->layout ().dbu ()) * (cv.context_trans () * r->first.trans ());
          db::Point new_point = db::Point (tt.inverted () * new_point_d);

          //  modify the shapes and replace

          db::Shapes &shapes = cv->layout ().cell (r->first.cell_index ()).shapes (r->first.layer ());
          db::Shape shape = r->first.shape ();

          if (shape.is_polygon ()) {

            db::Polygon poly;
            shape.polygon (poly);

            db::Polygon new_poly;
            if (insert_point_poly (poly, r->second, new_point, new_poly)) {
              shape = shapes.replace (shape, new_poly);
            }

          } else if (shape.is_path ()) {

            db::Path path;
            shape.path (path);

            db::Path new_path;
            if (insert_point_path (path, r->second, new_point, new_path)) {
              shape = shapes.replace (shape, new_path);
            }

          } else if (shape.is_box ()) {

            //  convert the box into a polygon unless the shape is on a guiding shape layer
            //  (if it's a guiding shape we must not change it's nature)

            if (r->first.layer () != view ()->cellview (r->first.cv_index ())->layout ().guiding_shape_layer ()) {

              db::Polygon poly (shape.box ());

              db::Polygon new_poly;
              if (insert_point_poly (poly, r->second, new_point, new_poly)) {
                shape = shapes.replace (shape, new_poly);
              }

            }

          }

          lay::ObjectInstPath obj = r->first;
          obj.set_shape (shape);

          m_selection.clear ();
          m_selection.insert (std::make_pair (obj, std::set<EdgeWithIndex> ()));

          handle_guiding_shape_changes ();

          if (manager ()) {
            manager ()->commit ();
          }

          selection_to_view ();

        }

      }

    }

    m_alt_ac = lay::AC_Global;

    return true;

  } else {
    return false;
  }
}

bool  
PartialService::mouse_release_event (const db::DPoint &p, unsigned int buttons, bool prio)
{
  hover_reset ();

  if (prio && mp_box) {

    m_alt_ac = ac_from_buttons (buttons);

    ui ()->ungrab_mouse (this);

    delete mp_box;
    mp_box = 0;

    if (ui ()->mouse_event_viewport ().contains (p)) { 

      lay::Editable::SelectionMode mode = lay::Editable::Replace;
      bool shift = ((m_buttons & lay::ShiftButton) != 0);
      bool ctrl = ((m_buttons & lay::ControlButton) != 0);
      if (shift && ctrl) {
        mode = lay::Editable::Invert;
      } else if (shift) {
        mode = lay::Editable::Add;
      } else if (ctrl) {
        mode = lay::Editable::Reset;
      } 

      //  select is allowed to throw an exception 
      try {
        partial_select (db::DBox (m_p1, m_p2), mode);
      } catch (tl::Exception &ex) {
        show_error (ex);
        //  clear selection
        partial_select (db::DBox (), lay::Editable::Reset);
      }

    }

    m_alt_ac = lay::AC_Global;

    return true;

  }

  return false;
}

bool
PartialService::begin_move (MoveMode mode, const db::DPoint &p, lay::angle_constraint_type ac)
{
  if (has_selection () && mode == lay::Editable::Selected) {

    m_alt_ac = ac;

    m_dragging = true;
    m_keep_selection = true;

    if (is_single_point_selection ()) {
      //  for a single selected point we use the original point as the start location which
      //  allows bringing it to grid
      m_current = m_start = single_selected_point ();
    } else if (is_single_edge_selection ()) {
      //  for an edge selection use the point projected to edge as the start location which
      //  allows bringing it to grid
      m_current = m_start = projected_to_edge (single_selected_edge (), p);
    } else {
      m_current = m_start = p;
    }

    m_alt_ac = lay::AC_Global;

    return true;

  } else {
    return false;
  }
}

void
PartialService::update_vector_snapped_point (const db::DPoint &pt, db::DVector &vr, bool &result_set) const
{
  db::DVector v = snap (pt) - pt;

  if (! result_set || v.length () < vr.length ()) {
    result_set = true;
    vr = v;
  }
}

db::DVector
PartialService::snap_marker_to_grid (const db::DVector &v, bool &snapped) const
{
  if (! m_snap_objects_to_grid) {
    return v;
  }

  snapped = false;
  db::DVector vr;

  //  max. 10000 checks
  size_t count = 10000;

  db::DVector snapped_to (1.0, 1.0);
  db::DVector vv = lay::snap_angle (v, move_ac (), &snapped_to);

  TransformationVariants tv (view ());

  for (auto r = m_selection.begin (); r != m_selection.end (); ++r) {

    if (! r->first.is_valid (view ()) || r->first.is_cell_inst ()) {
      continue;
    }

    const lay::CellView &cv = view ()->cellview (r->first.cv_index ());
    const std::vector<db::DCplxTrans> *tv_list = tv.per_cv_and_layer (r->first.cv_index (), r->first.layer ());
    if (!tv_list || tv_list->empty ()) {
      continue;
    }

    db::CplxTrans tr = db::DCplxTrans (vv) * tv_list->front () * db::CplxTrans (cv->layout ().dbu ()) * cv.context_trans () * r->first.trans ();

    for (auto e = r->second.begin (); e != r->second.end () && count > 0; ++e) {
      update_vector_snapped_point (tr * e->p1 (), vr, snapped);
      --count;
      if (count > 0) {
        update_vector_snapped_point (tr * e->p2 (), vr, snapped);
        --count;
      }
    }

  }

  if (snapped) {
    vr += vv;
    return db::DVector (vr.x () * snapped_to.x (), vr.y () * snapped_to.y ());
  } else {
    return db::DVector ();
  }
}

db::DVector
PartialService::snap_move (const db::DVector &v) const
{
  bool snapped = false;
  db::DVector vs = snap_marker_to_grid (v, snapped);
  if (! snapped) {
    vs = snap (v);
  }
  return vs;
}

void
PartialService::move (const db::DPoint &p, lay::angle_constraint_type ac)
{
  if (! m_dragging) {
    return;
  }

  m_alt_ac = ac;

  set_cursor (lay::Cursor::size_all);

  //  drag the vertex or edge/segment
  if (is_single_point_selection () || is_single_edge_selection ()) {

    lay::PointSnapToObjectResult snap_details;

    //  for a single selected point or edge, m_start is the original position and we snap the target -
    //  thus, we can bring the point on grid or to an object's edge or vertex
    snap_details = snap2 (p);
    if (snap_details.object_snap == lay::PointSnapToObjectResult::NoObject) {
      m_current = m_start + snap_move (p - m_start);
    } else {
      m_current = snap_details.snapped_point;
      mouse_cursor_from_snap_details (snap_details);
    }

  } else {

    //  snap movement to angle and grid without object
    m_current = m_start + snap_move (p - m_start);
    clear_mouse_cursors ();

  }

  selection_to_view ();

  m_alt_ac = lay::AC_Global;
}

void
PartialService::end_move (const db::DPoint & /*p*/, lay::angle_constraint_type ac)
{
  if (! m_dragging) {
    return;
  }

  m_alt_ac = ac;

  if (m_current != m_start) {

    //  stop dragging
    ui ()->ungrab_mouse (this);

    if (manager ()) {
      manager ()->transaction (tl::to_string (tr ("Partial move")));
    }

    //  heuristically, if there is just one edge selected: do not confine to the movement
    //  angle constraint - the edge usually is confined enough
    db::DTrans move_trans = db::DTrans (m_current - m_start);

    transform_selection (move_trans);

    if (manager ()) {
      manager ()->commit ();
    }

  }

  if (! m_keep_selection) {
    m_selection.clear ();
  }

  m_dragging = false;
  selection_to_view ();

  clear_mouse_cursors ();

  m_alt_ac = lay::AC_Global;
}

bool
PartialService::has_selection ()
{
  return ! m_selection.empty ();
}

size_t
PartialService::selection_size ()
{
  return m_selection.size ();
}

db::DBox
PartialService::selection_bbox ()
{
  //  build the transformation variants cache
  //  TODO: this is done multiple times - once for each service!
  TransformationVariants tv (view ());
  const db::DCplxTrans &vp = view ()->viewport ().trans ();

  lay::TextInfo text_info (view ());

  db::DBox box;
  for (partial_objects::const_iterator r = m_selection.begin (); r != m_selection.end (); ++r) {

    const lay::CellView &cv = view ()->cellview (r->first.cv_index ());
    const db::Layout &layout = cv->layout ();

    db::CplxTrans ctx_trans = db::CplxTrans (layout.dbu ()) * cv.context_trans () * r->first.trans ();

    db::box_convert<db::CellInst> bc (layout);
    if (! r->first.is_cell_inst ()) {

      const std::vector<db::DCplxTrans> *tv_list = tv.per_cv_and_layer (r->first.cv_index (), r->first.layer ());
      if (tv_list != 0) {
        for (std::vector<db::DCplxTrans>::const_iterator t = tv_list->begin (); t != tv_list->end (); ++t) {
          if (r->first.shape ().is_text ()) {
            db::Text text;
            r->first.shape ().text (text);
            box += *t * text_info.bbox (ctx_trans * text, vp * *t);
          } else {
            for (auto e = r->second.begin (); e != r->second.end (); ++e) {
              box += *t * (ctx_trans * e->bbox ());
            }
          }
        }
      }

    } else {

      const std::vector<db::DCplxTrans> *tv_list = tv.per_cv (r->first.cv_index ());
      if (tv_list != 0) {
        for (std::vector<db::DCplxTrans>::const_iterator t = tv_list->begin (); t != tv_list->end (); ++t) {
          box += *t * (ctx_trans * r->first.back ().bbox (bc));
        }
      }

    }

  }

  return box;
}

bool
PartialService::has_transient_selection ()
{
  //  there is no specific transient selection for the partial editor
  return false;
}

void
PartialService::del ()
{
  std::set<db::Layout *> needs_cleanup;

  //  stop dragging
  ui ()->ungrab_mouse (this);
  
  std::map <std::pair <db::cell_index_type, std::pair <unsigned int, unsigned int> >, std::vector <partial_objects::const_iterator> > shapes_to_delete_by_cell;

  for (partial_objects::iterator r = m_selection.begin (); r != m_selection.end (); ++r) {

    if (! r->first.is_cell_inst ()) {

      const lay::CellView &cv = view ()->cellview (r->first.cv_index ());

      //  modify the shapes and replace

      db::Shapes &shapes = cv->layout ().cell (r->first.cell_index ()).shapes (r->first.layer ());
      db::Shape shape = r->first.shape ();

      if (shape.is_polygon ()) {

        db::Polygon poly;
        shape.polygon (poly);

        db::Polygon new_poly = del_points_poly (poly, r->second);
        if (new_poly.hull ().size () < 3) {
          shapes_to_delete_by_cell.insert (std::make_pair (std::make_pair (r->first.cell_index (), std::make_pair (r->first.cv_index (), r->first.layer ())), std::vector <partial_objects::const_iterator> ())).first->second.push_back (r);
        } else {
          shapes.replace (shape, new_poly);
        }

      } else if (shape.is_path ()) {

        db::Path path;
        shape.path (path);

        db::Path new_path = del_points_path (path, r->second);
        if (new_path.points () < 2) {
          shapes_to_delete_by_cell.insert (std::make_pair (std::make_pair (r->first.cell_index (), std::make_pair (r->first.cv_index (), r->first.layer ())), std::vector <partial_objects::const_iterator> ())).first->second.push_back (r);
        } else {
          shapes.replace (shape, new_path);
        }

      } else if (shape.is_box ()) {

        //  if more than one point is deleted, the box basically collapses, if one point is deleted
        //  nothing changes on the box.
        if (r->second.size () > 1) {
          shapes_to_delete_by_cell.insert (std::make_pair (std::make_pair (r->first.cell_index (), std::make_pair (r->first.cv_index (), r->first.layer ())), std::vector <partial_objects::const_iterator> ())).first->second.push_back (r);
        }

      } else if (shape.is_text () || shape.is_point ()) {

        shapes_to_delete_by_cell.insert (std::make_pair (std::make_pair (r->first.cell_index (), std::make_pair (r->first.cv_index (), r->first.layer ())), std::vector <partial_objects::const_iterator> ())).first->second.push_back (r);

      }

    }

  }

  //  delete all shapes that are really lost
  std::vector <db::Shape> shapes_to_delete;
  for (std::map <std::pair <db::cell_index_type, std::pair <unsigned int, unsigned int> >, std::vector <partial_objects::const_iterator> >::const_iterator sbc = shapes_to_delete_by_cell.begin (); sbc != shapes_to_delete_by_cell.end (); ++sbc) {
    const lay::CellView &cv = view ()->cellview (sbc->first.second.first);
    if (cv.is_valid ()) {
      //  don't delete guiding shapes
      if (sbc->first.second.second != cv->layout ().guiding_shape_layer ()) {
        for (std::vector <partial_objects::const_iterator>::const_iterator s = sbc->second.begin (); s != sbc->second.end (); ++s) {
          cv->layout ().cell (sbc->first.first).shapes (sbc->first.second.second).erase_shape ((*s)->first.shape ());
        }
      }
    }
  }
  
  //  then delete all instances.
  
  //  sort the selected objects (the instances) by the cell they are in
  //  The key is a pair: cell_index, cv_index
  std::map <std::pair <db::cell_index_type, unsigned int>, std::vector <partial_objects::const_iterator> > insts_by_cell;
  for (partial_objects::const_iterator r = m_selection.begin (); r != m_selection.end (); ++r) {
    if (r->first.is_cell_inst ()) {
      const lay::CellView &cv = view ()->cellview (r->first.cv_index ());
      if (cv.is_valid ()) {
        if (cv->layout ().cell (r->first.back ().inst_ptr.cell_index ()).is_proxy ()) {
          needs_cleanup.insert (& cv->layout ());
        }
        cv->layout ().cell (r->first.cell_index ()).erase (r->first.back ().inst_ptr);
      }
    }
  }

  //  Hint: calling this method is somewhat dangerous since the selection is not necessarily valid (the shapes
  //  may have been deleted). However, since we did not delete guiding shapes before and this method in particular
  //  handles guiding shapes, this should be fairly safe.
  handle_guiding_shape_changes ();

  m_selection.clear ();
  m_dragging = false;
  selection_to_view ();

  //  clean up the layouts that need to do so.
  for (std::set<db::Layout *>::const_iterator l = needs_cleanup.begin (); l != needs_cleanup.end (); ++l) {
    (*l)->cleanup ();
  }
}

lay::InstanceMarker *
PartialService::new_inst_marker (size_t &nmarker, unsigned int cv_index, bool transient)
{
  lay::InstanceMarker *marker;

  if (transient) {
    if (nmarker >= m_transient_inst_markers.size ()) {
      marker = new lay::InstanceMarker (view (), cv_index);
      m_transient_inst_markers.push_back (marker);
    } else {
      marker = m_transient_inst_markers [nmarker];
    }
  } else {
    if (nmarker >= m_inst_markers.size ()) {
      marker = new lay::InstanceMarker (view (), cv_index);
      m_inst_markers.push_back (marker);
    } else {
      marker = m_inst_markers [nmarker];
    }
  }

  ++nmarker;

  return marker;
}

lay::Marker *
PartialService::new_marker (size_t &nmarker, unsigned int cv_index, bool transient)
{
  lay::Marker *marker;

  if (transient) {
    if (nmarker >= m_transient_markers.size ()) {
      marker = new lay::Marker (view (), cv_index);
      m_transient_markers.push_back (marker);
    } else {
      marker = m_transient_markers [nmarker];
    }
  } else {
    if (nmarker >= m_markers.size ()) {
      marker = new lay::Marker (view (), cv_index);
      m_markers.push_back (marker);
    } else {
      marker = m_markers [nmarker];
    }
  }

  ++nmarker;

  if (transient) {
    marker->set_vertex_size (0);
    marker->set_dither_pattern (1 /*hollow*/);
    marker->set_frame_pattern (0 /*solid*/);
    marker->set_line_width (1);
    marker->set_halo (false);
  } else {
    marker->set_vertex_size (-1 /*default*/);
    marker->set_dither_pattern (1 /*hollow*/);
    marker->set_frame_pattern (0 /*solid*/);
    marker->set_line_width (-1 /*default*/);
    marker->set_halo (-1 /*default*/);
  }

  return marker;
}

void 
PartialService::enter_path (db::Path &p, size_t &nmarker, partial_objects::const_iterator sel, const std::map <PointWithIndex, db::Point> &new_points, const std::map <EdgeWithIndex, db::Edge> &new_edges, const db::ICplxTrans &gt, const std::vector<db::DCplxTrans> &tv, bool transient)
{
  lay::Marker *marker = new_marker (nmarker, sel->first.cv_index (), transient);

  marker->set_dither_pattern (3 /*dotted*/);
  marker->set_frame_pattern (2 /*dotted*/);
  marker->set_line_width (1);
  marker->set_halo (0);
  modify_path (p, new_points, new_edges);
  marker->set (p, gt, tv);
}

void 
PartialService::enter_polygon (db::Polygon &p, size_t &nmarker, partial_objects::const_iterator sel, const std::map <PointWithIndex, db::Point> &new_points, const std::map <EdgeWithIndex, db::Edge> &new_edges, const db::ICplxTrans &gt, const std::vector<db::DCplxTrans> &tv, bool transient)
{
  lay::Marker *marker = new_marker (nmarker, sel->first.cv_index (), transient);

  marker->set_dither_pattern (3 /*dotted*/);
  marker->set_frame_pattern (2 /*dotted*/);
  marker->set_line_width (1);
  marker->set_halo (0);
  modify_polygon (p, new_points, new_edges);
  marker->set (p, gt, tv);
}

void 
PartialService::enter_vertices (size_t &nmarker, partial_objects::const_iterator sel, const std::map <PointWithIndex, db::Point> &new_points, const std::map <EdgeWithIndex, db::Edge> & /*new_edges*/, const db::ICplxTrans &gt, const std::vector<db::DCplxTrans> &tv, bool transient)
{
  //  TODO: create vertex markers only for vertices that are not for an edge 
  //  and use "fat" vertices on the edge markers.

  for (std::set <EdgeWithIndex>::const_iterator e = sel->second.begin (); e != sel->second.end (); ++e) {

    if (e->p1 () == e->p2 ()) {
  
      lay::Marker *marker = new_marker (nmarker, sel->first.cv_index (), transient);

      db::Point pnew = e->p1 ();
      std::map <PointWithIndex, db::Point>::const_iterator np = new_points.find (PointWithIndex (pnew, e->n, e->c));
      if (np != new_points.end ()) {
        pnew = np->second;
      }

      marker->set (db::Edge (pnew, pnew), gt, tv);

    }

  }

}

void 
PartialService::enter_edge (const EdgeWithIndex &e, size_t &nmarker, partial_objects::const_iterator sel, const std::map <PointWithIndex, db::Point> &new_points, const std::map <EdgeWithIndex, db::Edge> &new_edges, const db::ICplxTrans &gt, const std::vector<db::DCplxTrans> &tv, bool transient)
{
  db::Point ep1 (e.p1 ());
  db::Point ep2 (e.p2 ());

  bool p1_sel = sel->second.find (EdgeWithIndex (db::Edge (ep1, ep1), e.n, e.n, e.c)) != sel->second.end ();
  bool p2_sel = sel->second.find (EdgeWithIndex (db::Edge (ep2, ep2), e.nn, e.nn, e.c)) != sel->second.end ();
  bool p12_sel = sel->second.find (e) != sel->second.end ();
  
  if (p1_sel || p2_sel || p12_sel) {

    //  map points to moved ones
    std::map <PointWithIndex, db::Point>::const_iterator np;
    np = new_points.find (e.pi1 ());
    if (np != new_points.end ()) {
      ep1 = np->second;
    }
    np = new_points.find (e.pi2 ());
    if (np != new_points.end ()) {
      ep2 = np->second;
    }

    db::Edge enew (ep1, ep2);

    std::map <EdgeWithIndex, db::Edge>::const_iterator ne;
    ne = new_edges.find (e);
    if (ne != new_edges.end ()) {

      enew = ne->second;

      if (enew.p1 () != ep1) {
        lay::Marker *marker = new_marker (nmarker, sel->first.cv_index (), transient);
        marker->set_vertex_size (0);
        marker->set (db::Edge (ep1, enew.p1 ()), gt, tv);
      }

      if (enew.p2 () != ep2) {
        lay::Marker *marker = new_marker (nmarker, sel->first.cv_index (), transient);
        marker->set_vertex_size (0);
        marker->set (db::Edge (enew.p2 (), ep2), gt, tv);
      }

    }

    if (p2_sel && !p12_sel) {

      lay::Marker *marker = new_marker (nmarker, sel->first.cv_index (), transient);
      marker->set_vertex_size (0);

      db::DEdge ee = db::DEdge (db::DPoint (ep2) + ((db::DPoint (ep1) - db::DPoint (ep2)) * 0.25), db::DPoint (ep2));
      marker->set (ee, db::DCplxTrans (gt), tv);

      if (transient && sel->second.size () == 1) {
        add_mouse_cursor (ep2, sel->first.cv_index (), gt, tv, true);
      }

    } 

    if (p1_sel && !p12_sel) {

      lay::Marker *marker = new_marker (nmarker, sel->first.cv_index (), transient);
      marker->set_vertex_size (0);

      db::DEdge ee = db::DEdge (db::DPoint (ep1), db::DPoint (ep1) + ((db::DPoint (ep2) - db::DPoint (ep1)) * 0.25));
      marker->set (ee, db::DCplxTrans (gt), tv);

      if (transient && sel->second.size () == 1) {
        add_mouse_cursor (ep1, sel->first.cv_index (), gt, tv, true);
      }

    }

    if (p12_sel) {

      lay::Marker *marker = new_marker (nmarker, sel->first.cv_index (), transient);
      marker->set_vertex_size (0);
      marker->set (enew, gt, tv);

      if (transient) {
        add_edge_marker (enew, sel->first.cv_index (), gt, tv, true);
      }

    }

  }

}

double
PartialService::catch_distance ()
{
  return double (view ()->search_range ()) / ui ()->mouse_event_trans ().mag ();
}

double
PartialService::catch_distance_box ()
{
  return double (view ()->search_range_box ()) / ui ()->mouse_event_trans ().mag ();
}

db::DPoint
PartialService::single_selected_point () const
{
  //  build the transformation variants cache and 
  //  use only the first one of the explicit transformations 
  //  TODO: clarify how this can be implemented in a more generic form or leave it thus.
  TransformationVariants tv (view ());
  const std::vector<db::DCplxTrans> *tv_list = tv.per_cv_and_layer (m_selection.begin ()->first.cv_index (), m_selection.begin ()->first.layer ());

  const lay::CellView &cv = view ()->cellview (m_selection.begin ()->first.cv_index ());
  db::ICplxTrans gt (cv.context_trans () * m_selection.begin ()->first.trans ());
  db::CplxTrans tt = (*tv_list)[0] * db::CplxTrans (cv->layout ().dbu ()) * gt;

  return tt * m_selection.begin ()->second.begin ()->p1 ();
}

bool 
PartialService::is_single_point_selection () const
{
  return (m_selection.size () == 1 && ! m_selection.begin ()->first.is_cell_inst () && m_selection.begin ()->second.size () == 1 /*p*/);
}

db::DEdge
PartialService::single_selected_edge () const
{
  //  build the transformation variants cache and
  //  use only the first one of the explicit transformations
  //  TODO: clarify how this can be implemented in a more generic form or leave it thus.
  TransformationVariants tv (view ());
  const std::vector<db::DCplxTrans> *tv_list = tv.per_cv_and_layer (m_selection.begin ()->first.cv_index (), m_selection.begin ()->first.layer ());

  const lay::CellView &cv = view ()->cellview (m_selection.begin ()->first.cv_index ());
  db::ICplxTrans gt (cv.context_trans () * m_selection.begin ()->first.trans ());
  db::CplxTrans tt = (*tv_list)[0] * db::CplxTrans (cv->layout ().dbu ()) * gt;

  //  pick the edge from the selection (there is: p1, p2 and the edge between them)
  for (auto s = m_selection.begin ()->second.begin (); s != m_selection.begin ()->second.end (); ++s) {
    if (s->n != s->nn) {
      return tt * *s;
    }
  }

  //  fallback: should not happen
  return tt * *m_selection.begin ()->second.begin ();
}

bool
PartialService::is_single_edge_selection () const
{
  return (m_selection.size () == 1 && ! m_selection.begin ()->first.is_cell_inst () && m_selection.begin ()->second.size () == 3 /*p1,p2,edge*/);
}

bool
PartialService::select (const db::DBox &box, SelectionMode mode)
{
  if (box.empty () && mode == lay::Editable::Reset) {
    //  clear selection
    m_selection.clear ();
    selection_to_view ();
  }

  return false;
}

void
PartialService::selection_to_view ()
{
  dm_selection_to_view ();
}

void
PartialService::do_selection_to_view ()
{
  //  if dragging, establish the current displacement
  db::DTrans move_trans;
  if (m_dragging) {

    //  heuristically, if there is just one edge selected: do not confine to the movement
    //  angle constraint - the edge usually is confined enough
    if (m_selection.size () == 1 && ! m_selection.begin ()->first.is_cell_inst () && m_selection.begin ()->second.size () == 3 /*p1,p2,edge*/) {
      move_trans = db::DTrans (m_current - m_start);
    } else {
      //  TODO: DTrans should have a ctor that takes a vector
      move_trans = db::DTrans (lay::snap_angle (m_current - m_start, move_ac ()));
    }

    //  display vector
    view ()->message (std::string ("dx: ") + tl::micron_to_string (move_trans.disp ().x ()) + 
                      std::string ("  dy: ") + tl::micron_to_string (move_trans.disp ().y ()) + 
                      std::string ("  d: ") + tl::micron_to_string (move_trans.disp ().length ()));

  }

  size_t n_marker = 0;
  size_t n_inst_marker = 0;

  //  Reduce the selection to valid paths (issue-1145)
  std::vector<partial_objects::iterator> invalid_objects;
  for (partial_objects::iterator r = m_selection.begin (); r != m_selection.end (); ++r) {
    if (! r->first.is_valid (view ())) {
      invalid_objects.push_back (r);
    }
  }
  for (auto i = invalid_objects.begin (); i != invalid_objects.end (); ++i) {
    m_selection.erase (*i);
  }

  if (! m_selection.empty ()) {

    //  build the transformation variants cache
    TransformationVariants tv (view ());

    for (partial_objects::const_iterator r = m_selection.begin (); r != m_selection.end (); ++r) {

      const lay::CellView &cv = view ()->cellview (r->first.cv_index ());

      if (! r->first.is_cell_inst ()) {

        const std::vector<db::DCplxTrans> *tv_list = tv.per_cv_and_layer (r->first.cv_index (), r->first.layer ());
        if (tv_list && !tv_list->empty ()) {

          //  use only the first one of the explicit transformations
          //  TODO: clarify how this can be implemented in a more generic form or leave it thus.
          db::ICplxTrans gt (cv.context_trans () * r->first.trans ());
          db::CplxTrans tt = (*tv_list) [0] * db::CplxTrans (cv->layout ().dbu ()) * gt;
          db::Vector move_vector (tt.inverted () * (move_trans * (tt * db::Point ())));

          //  create the shift sets describing how points and edges are being moved

          std::map <EdgeWithIndex, db::Edge> new_edges;
          std::map <PointWithIndex, db::Point> new_points;

          if (m_dragging) {
            create_shift_sets (r->first.shape (), r->second, new_points, new_edges, move_vector);
          }

          //  create the markers to represent vertices and edges

          enter_vertices (n_marker, r, new_points, new_edges, gt, *tv_list, false);

          if (r->first.shape ().is_polygon ()) {

            for (unsigned int c = 0; c < r->first.shape ().holes () + 1; ++c) {

              unsigned int n = 0;
              db::Shape::polygon_edge_iterator ee;
              for (db::Shape::polygon_edge_iterator e = r->first.shape ().begin_edge (c); ! e.at_end (); e = ee, ++n) {
                ee = e;
                ++ee;
                unsigned int nn = ee.at_end () ? 0 : n + 1;
                enter_edge (EdgeWithIndex (*e, n, nn, c), n_marker, r, new_points, new_edges, gt, *tv_list, false);
              }

            }

            db::Polygon poly;
            r->first.shape ().polygon (poly);

            //  warning: poly is modified:
            enter_polygon (poly, n_marker, r, new_points, new_edges, gt, *tv_list, false);

          } else if (r->first.shape ().is_path ()) {

            if (r->first.shape ().begin_point () != r->first.shape ().end_point ()) {

              db::Shape::point_iterator pt = r->first.shape ().begin_point ();
              db::Point p1 = *pt;

              unsigned int n = 0;
              while (true) {

                ++pt;
                if (pt == r->first.shape ().end_point ()) {
                  break;
                }

                enter_edge (EdgeWithIndex (db::Edge (p1, *pt), n, n + 1, 0), n_marker, r, new_points, new_edges, gt, *tv_list, false);

                p1 = *pt;
                ++n;

              }

              //  TODO: ... put this somewhere else:
              db::Path path;
              r->first.shape ().path (path);

              //  warning: path is modified:
              enter_path (path, n_marker, r, new_points, new_edges, gt, *tv_list, false);

            }

          } else if (r->first.shape ().is_box ()) {

            //  convert to polygon and test those edges
            db::Polygon poly (r->first.shape ().box ());
            unsigned int n = 0;
            db::Shape::polygon_edge_iterator ee;
            for (db::Shape::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end (); e = ee, ++n) {
              ee = e;
              ++ee;
              unsigned int nn = ee.at_end () ? 0 : n + 1;
              enter_edge (EdgeWithIndex (*e, n, nn, 0), n_marker, r, new_points, new_edges, gt, *tv_list, false);
            }

            //  warning: poly is modified:
            enter_polygon (poly, n_marker, r, new_points, new_edges, gt, *tv_list, false);

          } else if (r->first.shape ().is_text ()) {

            db::Point tp (r->first.shape ().text_trans () * db::Point ());
            enter_edge (EdgeWithIndex (db::Edge (tp, tp), 0, 0, 0), n_marker, r, new_points, new_edges, gt, *tv_list, false);

          } else if (r->first.shape ().is_point ()) {

            db::Point tp (r->first.shape ().point ());
            enter_edge (EdgeWithIndex (db::Edge (tp, tp), 0, 0, 0), n_marker, r, new_points, new_edges, gt, *tv_list, false);

          }

        }

      } else {

        //  compute the global transformation including movement, context and explicit transformation
        db::ICplxTrans gt = db::VCplxTrans (1.0 / cv->layout ().dbu ()) * db::DCplxTrans (move_trans) * db::CplxTrans (cv->layout ().dbu ());
        gt *= (cv.context_trans () * r->first.trans ());

        const std::vector<db::DCplxTrans> *tv_list = tv.per_cv (r->first.cv_index ());
        if (tv_list  && ! tv_list->empty ()) {
          lay::InstanceMarker *marker = new_inst_marker (n_inst_marker, r->first.cv_index (), false);
          marker->set (r->first.back ().inst_ptr, gt, *tv_list);
        }

      }

    }

  }

  //  delete superfluous markers
  resize_markers (n_marker, false);
  resize_inst_markers (n_inst_marker, false);
}

void 
PartialService::resize_markers (size_t n, bool transient)
{
  if (transient) {
    for (std::vector<lay::Marker *>::iterator r = m_transient_markers.begin () + n; r != m_transient_markers.end (); ++r) {
      delete *r;
    }
    m_transient_markers.erase (m_transient_markers.begin () + n, m_transient_markers.end ());
  } else {
    for (std::vector<lay::Marker *>::iterator r = m_markers.begin () + n; r != m_markers.end (); ++r) {
      delete *r;
    }
    m_markers.erase (m_markers.begin () + n, m_markers.end ());
  }
}

void 
PartialService::resize_inst_markers (size_t n, bool transient)
{
  if (transient) {
    for (std::vector<lay::InstanceMarker *>::iterator r = m_transient_inst_markers.begin () + n; r != m_transient_inst_markers.end (); ++r) {
      delete *r;
    }
    m_transient_inst_markers.erase (m_transient_inst_markers.begin () + n, m_transient_inst_markers.end ());
  } else {
    for (std::vector<lay::InstanceMarker *>::iterator r = m_inst_markers.begin () + n; r != m_inst_markers.end (); ++r) {
      delete *r;
    }
    m_inst_markers.erase (m_inst_markers.begin () + n, m_inst_markers.end ());
  }
}

bool 
PartialService::partial_select (const db::DBox &box, lay::Editable::SelectionMode mode)
{
  clear_partial_transient_selection ();

  //  compute search box
  double l = box.is_point () ? catch_distance () : catch_distance_box ();
  db::DBox search_box = box.enlarged (db::DVector (l, l));

  bool needs_update = false;
  bool any_selected = false;

  //  clear before unless "add" is selected
  if (mode == lay::Editable::Replace) {
    if (! m_selection.empty ()) {
      m_selection.clear ();
      needs_update = true;
    }
  }

  if (box.empty ()) {

    //  unconditional selection
    if (mode == lay::Editable::Reset) {
      if (! m_selection.empty ()) {
        m_selection.clear ();
        needs_update = true;
      }
    } else {

      //  extract all shapes
      //  ... not implemented yet ...

    }

  } else {

    int shape_flags = 0;
    if (edt::polygons_enabled ()) {
      shape_flags |= db::ShapeIterator::Polygons;
    }
    if (edt::paths_enabled ()) {
      //  Note: points, edges and edge pairs don't have separate entires, so
      //  we count them as paths here
      shape_flags |= db::ShapeIterator::Paths;
      shape_flags |= db::ShapeIterator::Edges;
      shape_flags |= db::ShapeIterator::EdgePairs;
      shape_flags |= db::ShapeIterator::Points;
    }
    if (edt::boxes_enabled ()) {
      shape_flags |= db::ShapeIterator::Boxes;
    }
    if (edt::points_enabled ()) {
      shape_flags |= db::ShapeIterator::Points;
    }
    if (edt::texts_enabled ()) {
      shape_flags |= db::ShapeIterator::Texts;
    }

    PartialShapeFinder finder (box.is_point (), m_top_level_sel, db::ShapeIterator::flags_type (shape_flags));
    finder.find (view (), search_box);

    //  We must make sure that guiding shapes are only selected alone. The first selected object will 
    //  determine whether we take guiding shapes into account or not.
    bool gs_mode = (finder.begin () != finder.end () && finder.begin ()->first.layer () == view ()->cellview (finder.begin ()->first.cv_index ())->layout ().guiding_shape_layer ());

    //  Clear the selection if it was consisting of a guiding shape or non-guiding shape before (depending on mode). 
    //  This way we make sure there is not mixture between guiding shapes and others.
    if (m_selection.begin () != m_selection.end ()) {
      if (gs_mode != (m_selection.begin ()->first.layer () == view ()->cellview (m_selection.begin ()->first.cv_index ())->layout ().guiding_shape_layer ())) {
        m_selection.clear ();
        needs_update = true;
      }
    }

    //  collect the founds from the finder
    for (PartialShapeFinder::iterator f = finder.begin (); f != finder.end (); ++f) {

      if (gs_mode == (f->first.layer () == view ()->cellview (f->first.cv_index ())->layout ().guiding_shape_layer ())) {

        if (mode == lay::Editable::Replace || mode == lay::Editable::Add) {
          //  select
          partial_objects::iterator sel = m_selection.find (f->first);
          if (sel == m_selection.end ()) {
            sel = m_selection.insert (std::make_pair (f->first, std::set <EdgeWithIndex> ())).first;
          } 
          sel->second.insert (f->second.begin (), f->second.end ());
        } else if (mode == lay::Editable::Reset) {
          //  unselect
          if (m_selection.find (f->first) != m_selection.end ()) {
            m_selection.erase (f->first);
          }
        } else {
          //  invert selection
          if (m_selection.find (f->first) != m_selection.end ()) {
            m_selection.erase (f->first);
          } else {
            m_selection.insert (std::make_pair (f->first, std::set <EdgeWithIndex> ())).first->second.insert (f->second.begin (), f->second.end ());
          }
        }

        needs_update = true;
        any_selected = true;

      }

    }

    //  check, if there is a selected instance inside the box - in this case, we do not do a new selection
    if (! box.is_point () && edt::instances_enabled ()) {

      lay::InstFinder inst_finder (box.is_point (), m_top_level_sel, true /*full arrays*/, true /*enclose*/, 0 /*no excludes*/, true /*visible layers*/);
      inst_finder.find (view (), search_box);

      //  collect the founds from the finder
      for (lay::InstFinder::iterator f = inst_finder.begin (); f != inst_finder.end (); ++f) {

        if (mode == lay::Editable::Replace || mode == lay::Editable::Add) {
          //  select
          m_selection.insert (std::make_pair (*f, std::set <EdgeWithIndex> ()));
        } else if (mode == lay::Editable::Reset) {
          //  unselect
          if (m_selection.find (*f) != m_selection.end ()) {
            m_selection.erase (*f);
          }
        } else {
          //  invert selection
          if (m_selection.find (*f) != m_selection.end ()) {
            m_selection.erase (*f);
          } else {
            m_selection.insert (std::make_pair (*f, std::set <EdgeWithIndex> ()));
          }
        }

        needs_update = true;
        any_selected = true;

      }

    }
      
  }

  //  if required, update the list of objects to display the selection
  if (needs_update) {
    selection_to_view ();
  }

  return any_selected;
}

bool 
PartialService::handle_guiding_shape_changes ()
{
  //  just allow one guiding shape to be selected
  if (m_selection.empty ()) {
    return false;
  }

  partial_objects::const_iterator s = m_selection.begin ();

  unsigned int cv_index = s->first.cv_index ();
  lay::CellView cv = view ()->cellview (cv_index);
  db::Layout *layout = &cv->layout ();

  if (s->first.is_cell_inst () || s->first.layer () != layout->guiding_shape_layer ()) {
    return false;
  }

  if (! s->first.shape ().has_prop_id ()) {
    return false;
  }

  if (! layout->is_pcell_instance (s->first.cell_index ()).first) {
    return false;
  }

  db::cell_index_type top_cell = std::numeric_limits<db::cell_index_type>::max ();
  db::cell_index_type parent_cell = std::numeric_limits<db::cell_index_type>::max ();
  db::Instance parent_inst;
  db::pcell_parameters_type parameters_for_pcell;

  //  determine parent cell and instance if required
  lay::ObjectInstPath::iterator e = s->first.end ();
  if (e == s->first.begin ()) {
    top_cell = s->first.cell_index ();
  } else {
    --e;
    db::cell_index_type pc = s->first.topcell ();
    if (e != s->first.begin ()) {
      --e;
      pc = e->inst_ptr.cell_index ();
    }
    parent_cell = pc;
    parent_inst = s->first.back ().inst_ptr;
  }

  db::property_names_id_type pn = layout->properties_repository ().prop_name_id ("name");

  const db::PropertiesRepository::properties_set &input_props = layout->properties_repository ().properties (s->first.shape ().prop_id ());
  db::PropertiesRepository::properties_set::const_iterator input_pv = input_props.find (pn);
  if (input_pv == input_props.end ()) {
    return false;
  }

  std::string shape_name = input_pv->second.to_string ();

  //  Hint: get_parameters_from_pcell_and_guiding_shapes invalidates the shapes because it resets the changed
  //  guiding shapes. We must not access s->shape after that.
  if (! get_parameters_from_pcell_and_guiding_shapes (layout, s->first.cell_index (), parameters_for_pcell)) {
    return false;
  }

  partial_objects new_sel;

  if (parent_cell != std::numeric_limits <db::cell_index_type>::max ()) {

    db::Instance new_inst = layout->cell (parent_cell).change_pcell_parameters (parent_inst, parameters_for_pcell);

    //  try to identify the selected shape in the new shapes and select this one
    db::Shapes::shape_iterator sh = layout->cell (new_inst.cell_index ()).shapes (layout->guiding_shape_layer ()).begin (db::ShapeIterator::All);
    while (! sh.at_end ()) {
      const db::PropertiesRepository::properties_set &props = layout->properties_repository ().properties (sh->prop_id ());
      db::PropertiesRepository::properties_set::const_iterator pv = props.find (pn);
      if (pv != props.end ()) {
        if (pv->second.to_string () == shape_name) {
          lay::ObjectInstPath inst_path = s->first;
          inst_path.back ().inst_ptr = new_inst;
          inst_path.back ().array_inst = new_inst.begin ();
          inst_path.set_shape (*sh);
          new_sel.insert (std::make_pair (inst_path, s->second));
          break;
        }
      }
      ++sh;
    }
    
  }

  if (top_cell != std::numeric_limits <db::cell_index_type>::max ()) {
    // TODO: implement the case of a PCell variant being a top cell
    // Currently there is not way to create such a configuration ...
  }

  //  remove superfluous proxies
  layout->cleanup ();

  m_selection = new_sel;
  selection_to_view ();

  return true;
}

} // namespace edt


