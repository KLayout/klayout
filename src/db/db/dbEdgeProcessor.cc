
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



#include "dbEdgeProcessor.h"
#include "dbPolygonGenerators.h"
#include "dbLayout.h"
#include "tlTimer.h"
#include "tlProgress.h"
#include "gsi.h"

#include <vector>
#include <deque>
#include <memory>

#if 0
#define DEBUG_MERGEOP
#define DEBUG_BOOLEAN
#define DEBUG_EDGE_PROCESSOR
#endif

// #define DEBUG_SIZE_INTERMEDIATE

namespace db
{

const double fill_factor = 1.5;

// -------------------------------------------------------------------------------
//  Some utilities ..

struct NonZeroInsideFunc 
{
  inline bool operator() (int wc) const
  {
    return wc != 0;
  }
};

struct ProjectionCompare
{
  ProjectionCompare (const db::Edge &e)
    : m_e (e) { }

  bool operator () (const db::Point &a, const db::Point &b) const
  {
    db::coord_traits<db::Coord>::area_type sp1 = db::sprod (m_e, db::Edge (m_e.p1 (), a));
    db::coord_traits<db::Coord>::area_type sp2 = db::sprod (m_e, db::Edge (m_e.p1 (), b));
    if (sp1 != sp2) {
      return sp1 < sp2;
    } else {
      return a < b;
    }
  }

private:
  db::Edge m_e;
};

struct PolyMapCompare
{
  PolyMapCompare (db::Coord y)
    : m_y (y) { }

  bool operator() (const std::pair<db::Edge, size_t> &a, const std::pair<db::Edge, size_t> &b) const
  {
    //  simple cases ..
    if (a.first.dx () == 0 && b.first.dx () == 0) {
      return a.first.p1 ().x () < b.first.p1 ().x ();
    } else if (edge_xmax (a.first) < edge_xmin (b.first)) {
      return true;
    } else if (edge_xmin (a.first) > edge_xmax (b.first)) {
      return false;
    } else {

      //  complex case:
      double xa = edge_xaty (a.first, m_y);
      double xb = edge_xaty (b.first, m_y);

      if (xa != xb) {
        return xa < xb;
      } else {

        //  compare by angle of normalized edges

        db::Edge ea (a.first);
        db::Edge eb (b.first);

        if (ea.dy () < 0) {
          ea.swap_points ();
        }
        if (eb.dy () < 0) {
          eb.swap_points ();
        }

        return db::vprod_sign (ea, eb) < 0;

      }
    }
  }

private:
  db::Coord m_y;
};

inline bool
is_point_on_exact (const db::Edge &e, const db::Point &pt)
{
  if (pt.x () < db::edge_xmin (e) || pt.x () > db::edge_xmax (e) ||
      pt.y () < db::edge_ymin (e) || pt.y () > db::edge_ymax (e)) {

    return false;

  } else if (e.dy () == 0 || e.dx () == 0) {

    //  shortcut for orthogonal edges
    return true;

  } else {

    return db::vprod_sign (pt - e.p1 (), e.p2 () - e.p1 ()) == 0;

  }
}

inline bool
is_point_on_fuzzy (const db::Edge &e, const db::Point &pt)
{
  //  exclude the start and end point
  if (pt == e.p1 () || pt == e.p2 ()) {

    return false;

  } else if (pt.x () < db::edge_xmin (e) || pt.x () > db::edge_xmax (e) ||
             pt.y () < db::edge_ymin (e) || pt.y () > db::edge_ymax (e)) {

    return false;

  } else if (e.dy () == 0 || e.dx () == 0) {

    //  shortcut for orthogonal edges
    return true;

  } else {

    bool with_equal = false;

    db::Vector offset;
    if ((e.dx () < 0 && e.dy () > 0) || (e.dx () > 0 && e.dy () < 0)) {
      offset = db::Vector (1, 1);
      with_equal = true;
    } else {
      offset = db::Vector (-1, 1);
    }

    db::Vector pp1 = pt - e.p1 ();

    typedef db::coord_traits<db::Point::coord_type>::area_type area_type;
    area_type a1 = 2 * db::vprod (pp1, e.d ());
    area_type a2 = db::vprod (offset, e.d ());

    if ((a1 < 0) == (a2 < 0)) {
      with_equal = false;
    }

    if (a1 < 0) { a1 = -a1; }
    if (a2 < 0) { a2 = -a2; }

    return a1 < a2 || (a1 == a2 && with_equal);

  }
}

//  A intersection test that is more robust numerically.
//  In some cases (i.e. (3,-3;-8,-1) and (-4,-2;13,-4)), the intersection test gives different results
//  for the intersection point if the edges are swapped. This test is robust since it operates on ordered edges.
inline std::pair<bool, db::Point> safe_intersect_point (const db::Edge &e1, const db::Edge &e2)
{
  if (e1 < e2) {
    return e1.intersect_point (e2);
  } else {
    return e2.intersect_point (e1);
  }
}

/**
 *  @brief A structure for storing data in the first phase of the scanline algorithm
 *
 *  This data are the cut points (intersection points with other edges). The has_cutpoints
 *  flag indicates that the edge has cutpoints which the edge must follow. If the edge has strong
 *  cutpoints (strong_cutpoints), they will change the edge, hence non-cutpoints (attractors) must be
 *  included. 
 */
struct CutPoints
{
  std::vector <db::Point> cut_points;
  std::vector <std::pair<db::Point, size_t> > attractors;
  bool has_cutpoints : 8;
  bool strong_cutpoints : 8;

  CutPoints ()
    : has_cutpoints (false), strong_cutpoints (false)
  { }

  void add_attractor (const db::Point &p, size_t next)
  {
    if (strong_cutpoints) {
      cut_points.push_back (p);
    } else {
      attractors.push_back (std::make_pair (p, next));
    }
  }

  void add (const db::Point &p, std::vector <CutPoints> *cpvector, bool strong = true)
  {
    has_cutpoints = true;
    if (strong && !strong_cutpoints) {

      strong_cutpoints = true;
      if (! attractors.empty ()) {

        std::vector <std::pair<db::Point, size_t> > attr;
        attractors.swap (attr);

        cut_points.reserve (cut_points.size () + attr.size ());
        for (std::vector <std::pair<db::Point, size_t> >::const_iterator a = attr.begin (); a != attr.end (); ++a) {
          (*cpvector) [a->second].add (a->first, cpvector, true);
        }

      }

    } 

    cut_points.push_back (p);

  }

};

/**
 *  @brief A data object for the scanline algorithm
 */
struct WorkEdge
  : public db::Edge
{
  WorkEdge () 
    : db::Edge (), data (0), prop (0)
  { }

  WorkEdge (const db::Edge &e, EdgeProcessor::property_type p = 0, size_t d = 0) 
    : db::Edge (e), data (d), prop (p)
  { }

  WorkEdge (const WorkEdge &d)
    : db::Edge (d), data (d.data), prop (d.prop)
  { }

  WorkEdge &operator= (const WorkEdge &d)
  { 
    if (this != &d) {
      db::Edge::operator= (d);
      data = d.data;
      prop = d.prop;
    }
    return *this;
  }

  WorkEdge &operator= (const db::Edge &d)
  { 
    db::Edge::operator= (d);
    return *this;
  }

  CutPoints *make_cutpoints (std::vector <CutPoints> &cutpoints)
  {
    if (! data) {
      cutpoints.push_back (CutPoints ());
      data = cutpoints.size ();
    }
    return &cutpoints [data - 1];
  }

  size_t data;
  db::EdgeProcessor::property_type prop;
};

/**
 *  @brief Compare edges by property ID
 */
struct EdgePropCompare
{
  bool operator() (const db::WorkEdge &a, const db::WorkEdge &b) const
  {
    return a.prop < b.prop;
  }
};

/**
 *  @brief Compare edges by property ID (reverse)
 */
struct EdgePropCompareReverse
{
  bool operator() (const db::WorkEdge &a, const db::WorkEdge &b) const
  {
    return b.prop < a.prop;
  }
};

/**
 *  @brief A compare operator for edged
 *  This operator will compare edges by their x position on the scanline
 */
struct EdgeXAtYCompare
{
  EdgeXAtYCompare (db::Coord y)
    : m_y (y) { }

  bool operator() (const db::Edge &a, const db::Edge &b) const
  {
    //  simple cases ..
    if (a.dx () == 0 && b.dx () == 0) {
      return a.p1 ().x () < b.p1 ().x ();
    } else if (edge_xmax (a) < edge_xmin (b)) {
      return true;
    } else if (edge_xmin (a) > edge_xmax (b)) {
      return false;
    } else {

      //  complex case:
      //  HINT: "volatile" forces xa and xb into memory and disables FPU register optimisation.
      //  That way, we can exactly compare doubles afterwards.
      volatile double xa = edge_xaty (a, m_y);
      volatile double xb = edge_xaty (b, m_y);

      if (xa != xb) {
        return xa < xb;
      } else if (a.dy () == 0) {
        return false;
      } else if (b.dy () == 0) {
        return true;
      } else {

        //  compare by angle of normalized edges

        db::Edge ea (a);
        db::Edge eb (b);

        if (ea.dy () < 0) {
          ea.swap_points ();
        }
        if (eb.dy () < 0) {
          eb.swap_points ();
        }

        return db::vprod_sign (ea, eb) < 0;

      }
    }
  }

  bool equal (const db::Edge &a, const db::Edge &b) const
  {
    //  simple cases ..
    if (a.dx () == 0 && b.dx () == 0) {
      return a.p1 ().x () == b.p1 ().x ();
    } else if (edge_xmax (a) < edge_xmin (b)) {
      return false;
    } else if (edge_xmin (a) > edge_xmax (b)) {
      return false;
    } else {

      //  complex case:
      //  HINT: "volatile" forces xa and xb into memory and disables FPU register optimisation.
      //  That way, we can exactly compare doubles afterwards.
      volatile double xa = edge_xaty (a, m_y);
      volatile double xb = edge_xaty (b, m_y);

      if (xa != xb) {
        return false;
      } else if (a.dy () == 0 || b.dy () == 0) {
        return (a.dy () == 0) == (b.dy () == 0);
      } else {

        //  compare by angle of normalized edges

        db::Edge ea (a);
        db::Edge eb (b);

        if (ea.dy () < 0) {
          ea.swap_points ();
        }
        if (eb.dy () < 0) {
          eb.swap_points ();
        }

        return db::vprod_sign (ea, eb) == 0;

      }
    }
  }

private:
  db::Coord m_y;
};

/**
 *  @brief A compare operator for edges used within EdgeXAtYCompare2
 *  This operator is an extension of db::edge_xaty and will deliver
 *  the minimum x if the edge is horizontal.
 */
static inline double edge_xaty2 (db::Edge e, db::Coord y)
{
  if (e.p1 ().y () > e.p2 ().y ()) {
    e.swap_points ();
  }

  if (y <= e.p1 ().y ()) {
    if (y == e.p2 ().y ()) {
      return std::min (e.p1 ().x (), e.p2 ().x ());
    } else {
      return e.p1 ().x ();
    }
  } else if (y >= e.p2 ().y ()) {
    return e.p2 ().x ();
  } else {
    return double (e.p1 ().x ()) + double (e.dx ()) * double (y - e.p1 ().y ()) / double (e.dy ());
  }
}

/**
 *  @brief A compare operator for edges
 *  This operator will compare edges by their x position on the scanline
 *  In Addition to EdgeXAtYCompare, this operator will also compare the 
 *  direction of the edges. Edges are equal if the position at which they cross
 *  the scanline and their direction is identical.
 */
struct EdgeXAtYCompare2
{
  EdgeXAtYCompare2 (db::Coord y)
    : m_y (y) { }

  bool operator() (const db::Edge &a, const db::Edge &b) const
  {
    //  simple cases ..
    if (a.dx () == 0 && b.dx () == 0) {
      return a.p1 ().x () < b.p1 ().x ();
    } else if (edge_xmax (a) < edge_xmin (b)) {
      return true;
    } else if (edge_xmin (a) > edge_xmax (b)) {
      return false;
    } else {

      //  complex case:
      //  HINT: "volatile" forces xa and xb into memory and disables FPU register optimisation.
      //  That way, we can exactly compare doubles afterwards.
      volatile double xa = edge_xaty2 (a, m_y);
      volatile double xb = edge_xaty2 (b, m_y);

      if (xa != xb) {
        return xa < xb;
      } else if (a.dy () == 0) {
        return false;
      } else if (b.dy () == 0) {
        return true;
      } else {

        //  In that case the edges will not intersect but rather touch in one point. This defines
        //  a sorting which preserves the scanline order of the edges when y advances.

        db::Edge ea (a);
        db::Edge eb (b);

        if (ea.dy () < 0) {
          ea.swap_points ();
        }
        if (eb.dy () < 0) {
          eb.swap_points ();
        }

        bool fa = ea.p2 ().y () > m_y;
        bool fb = eb.p2 ().y () > m_y;

        if (fa && fb) {
          //  Both edges advance
          return db::vprod_sign (ea, eb) < 0;
        } else if (fa || fb) {
          //  Only one edge advances - equality
          return false;
        } else {
          return db::vprod_sign (ea, eb) > 0;
        }

      }
    }
  }

  bool equal (const db::Edge &a, const db::Edge &b) const
  {
    //  simple cases ..
    if (a.dx () == 0 && b.dx () == 0) {
      return a.p1 ().x () == b.p1 ().x ();
    } else if (edge_xmax (a) < edge_xmin (b)) {
      return false;
    } else if (edge_xmin (a) > edge_xmax (b)) {
      return false;
    } else {

      //  complex case:
      //  HINT: "volatile" forces xa and xb into memory and disables FPU register optimisation.
      //  That way, we can exactly compare doubles afterwards.
      volatile double xa = edge_xaty2 (a, m_y);
      volatile double xb = edge_xaty2 (b, m_y);

      if (xa != xb) {
        return false;
      } else if (a.dy () == 0 || b.dy () == 0) {
        return (a.dy () == 0) == (b.dy () == 0);
      } else {

        //  compare by angle of normalized edges

        db::Edge ea (a);
        db::Edge eb (b);

        if (ea.dy () < 0) {
          ea.swap_points ();
        }
        if (eb.dy () < 0) {
          eb.swap_points ();
        }

        return db::vprod_sign (ea, eb) == 0;

      }
    }
  }

private:
  db::Coord m_y;
};

// -------------------------------------------------------------------------------
//  EdgePolygonOp implementation

EdgePolygonOp::EdgePolygonOp (EdgePolygonOp::mode_t mode, bool include_touching, int polygon_mode)
  : m_mode (mode), m_include_touching (include_touching),
    m_function (polygon_mode),
    m_wcp_n (0), m_wcp_s (0)
{
}

void EdgePolygonOp::reset () 
{ 
  m_wcp_n = m_wcp_s = 0;
}

int EdgePolygonOp::select_edge (bool horizontal, property_type p)
{
  if (p == 0) {
    return 0;
  }

  bool inside;

  if (horizontal) {
    if (m_include_touching) {
      inside = (m_function (m_wcp_n) || m_function (m_wcp_s));
    } else {
      inside = (m_function (m_wcp_n) && m_function (m_wcp_s));
    }
  } else {
    inside = m_function (m_wcp_n);
  }

  if (m_mode == Inside) {
    return inside ? 1 : 0;
  } else if (m_mode == Outside) {
    return inside ? 0 : 1;
  } else {
    return inside ? 1 : 2;
  }
}

int EdgePolygonOp::edge (bool north, bool enter, property_type p) 
{ 
  if (p == 0) {
    int *wc = north ? &m_wcp_n : &m_wcp_s;
    if (enter) {
      ++*wc;
    } else {
      --*wc;
    }
  }

  return 0; 
}

bool EdgePolygonOp::is_reset () const 
{ 
  return (m_wcp_n == 0 && m_wcp_s == 0);
}

bool EdgePolygonOp::prefer_touch () const 
{ 
  return m_include_touching; 
}

bool EdgePolygonOp::selects_edges () const 
{ 
  return true; 
}

// -------------------------------------------------------------------------------
//  InteractionDetector implementation

InteractionDetector::InteractionDetector (int mode, property_type primary_id)
  : m_mode (mode), m_include_touching (true), m_last_primary_id (primary_id)
{
  // .. nothing yet ..
}

void
InteractionDetector::reset ()
{
  m_wcv_n.clear ();
  m_wcv_s.clear ();
  m_inside_n.clear ();
  m_inside_s.clear ();
}

void 
InteractionDetector::reserve (size_t n)
{
  m_wcv_n.clear ();
  m_wcv_s.clear ();
  m_wcv_n.resize (n, 0);
  m_wcv_s.resize (n, 0);
  m_inside_n.clear ();
  m_inside_s.clear ();
}

int 
InteractionDetector::edge (bool north, bool enter, property_type p)
{
  tl_assert (p < m_wcv_n.size () && p < m_wcv_s.size ());

  int *wcv = north ? &m_wcv_n [p] : &m_wcv_s [p];

  bool inside_before = (*wcv != 0);
  *wcv += (enter ? 1 : -1);
  bool inside_after = (*wcv != 0);

  //  In "interacting" and "enclosing" mode we need to handle both north and south events because
  //  we have to catch interactions between objects north and south to the scanline
  if (north || (m_mode == 0 && m_include_touching) || (m_mode < -1 && m_include_touching)) {

    std::set <property_type> *inside = north ? &m_inside_n : &m_inside_s;

    if (inside_after < inside_before) {

      inside->erase (p);

      //  the primary objects are delivered last of all coincident edges
      //  (due to prefer_touch == true and the sorting of coincident edges by property id)
      //  hence every remaining parts count as non-interacting (outside)
      if (p <= m_last_primary_id) {
        for (std::set <property_type>::const_iterator i = inside->begin (); i != inside->end (); ++i) {
          if (*i > m_last_primary_id) {
            m_non_interactions.insert (*i);
          }
        }
      }

    } else if (inside_after > inside_before) {

      if (m_mode != 0) {

        //  enclosing/inside/outside mode
        if (p > m_last_primary_id) {

          //  note that the primary parts will be delivered first of all coincident
          //  edges hence we can check whether the primary is present even for coincident
          //  edges
          bool any = false;
          for (std::set <property_type>::const_iterator i = inside->begin (); i != inside->end (); ++i) {
            if (*i <= m_last_primary_id) {
              any = true;
              m_interactions.insert (std::make_pair (*i, p));
            }
          }
          if (! any) {
            m_non_interactions.insert (p);
          }

        } else {

          for (std::set <property_type>::const_iterator i = inside->begin (); i != inside->end (); ++i) {
            if (*i > m_last_primary_id) {
              if (m_mode < -1) {
                //  enclosing mode: an opening primary (= enclosing one) with open secondaries means the secondary
                //  has been opened before and did not close. Because we sort by property ID this must have happened
                //  before, hence the secondary is overlapping. Make them non-interactions. We still have to record them
                //  as interactions because this is how we skip the primaries later.
                m_non_interactions.insert (*i);
              }
              m_interactions.insert (std::make_pair (p, *i));
            }
          }

        }

      } else {

        for (std::set <property_type>::const_iterator i = m_inside_n.begin (); i != m_inside_n.end (); ++i) {
          if (*i < p) {
            m_interactions.insert (std::make_pair (*i, p));
          } else if (p < *i) {
            m_interactions.insert (std::make_pair (p, *i));
          }
        }

        for (std::set <property_type>::const_iterator i = m_inside_s.begin (); i != m_inside_s.end (); ++i) {
          if (*i < p) {
            m_interactions.insert (std::make_pair (*i, p));
          } else if (p < *i) {
            m_interactions.insert (std::make_pair (p, *i));
          }
        }

      }

      inside->insert (p);

    }

  }

  return 0;
}

int 
InteractionDetector::compare_ns () const
{
  return 0;
}

void
InteractionDetector::finish ()
{
  if (m_mode < -1) {

    //  In enclosing mode remove those objects which have an interaction with a secondary having a non-interaction:
    //  these are the ones where secondaries overlap and stick to the outside.
    std::set<property_type> primaries_to_delete;
    for (std::set<std::pair<property_type, property_type> >::iterator i = m_interactions.begin (); i != m_interactions.end (); ++i) {
      if (m_non_interactions.find (i->second) != m_non_interactions.end ()) {
        primaries_to_delete.insert (i->first);
      }
    }

    for (std::set<std::pair<property_type, property_type> >::iterator i = m_interactions.begin (); i != m_interactions.end (); ) {
      std::set<std::pair<property_type, property_type> >::iterator ii = i;
      ++ii;
      if (primaries_to_delete.find (i->first) != primaries_to_delete.end ()) {
        m_interactions.erase (i);
      }
      i = ii;
    }

  } else if (m_mode == -1) {

    //  In inside mode remove those objects which have a non-interaction with a primary
    for (std::set<std::pair<property_type, property_type> >::iterator i = m_interactions.begin (); i != m_interactions.end (); ) {
      std::set<std::pair<property_type, property_type> >::iterator ii = i;
      ++ii;
      if (m_non_interactions.find (i->second) != m_non_interactions.end ()) {
        m_interactions.erase (i);
      }
      i = ii;
    }

  } else if (m_mode > 0) {

    //  In outside mode leave those objects which don't participate in an interaction
    for (iterator pp = begin (); pp != end (); ++pp) {
      m_non_interactions.erase (pp->second);
    }

    m_interactions.clear ();
    for (std::set<property_type>::const_iterator p = m_non_interactions.begin (); p != m_non_interactions.end (); ++p) {
      m_interactions.insert (m_interactions.end (), std::make_pair (m_last_primary_id, *p));
    }

  }

  m_non_interactions.clear ();
}

// -------------------------------------------------------------------------------
//  MergeOp implementation

MergeOp::MergeOp (unsigned int min_wc)
  : m_wc_n (0), m_wc_s (0), m_min_wc (min_wc), m_zeroes (0)
{
  //  .. nothing yet ..
}

void  
MergeOp::reset ()
{
  m_wcv_n.clear ();
  m_wcv_s.clear ();
  m_wc_n = 0;
  m_wc_n = 0;
  m_zeroes = 0;
}

void 
MergeOp::reserve (size_t n)
{
  m_wcv_n.clear ();
  m_wcv_s.clear ();
  m_wcv_n.resize (n, 0);
  m_wcv_s.resize (n, 0);
  m_zeroes = 2 * n;
}

static inline 
bool result_by_mode (int wc, unsigned int min_wc)
{
  return wc > int (min_wc);
}

int 
MergeOp::edge (bool north, bool enter, property_type p)
{
  tl_assert (p < m_wcv_n.size () && p < m_wcv_s.size ());

  int *wcv = north ? &m_wcv_n [p] : &m_wcv_s [p];
  int *wc = north ? &m_wc_n : &m_wc_s;

  bool inside_before = (*wcv != 0);
  *wcv += (enter ? 1 : -1);
  bool inside_after = (*wcv != 0);
  m_zeroes += (!inside_after) - (!inside_before);
#ifdef DEBUG_MERGEOP
  printf ("north=%d, enter=%d, prop=%d -> %d\n", north, enter, int (p), int (m_zeroes));
#endif
  tl_assert (long (m_zeroes) >= 0);

  bool res_before = result_by_mode (*wc, m_min_wc);
  if (inside_before != inside_after) {
    *wc += (inside_after - inside_before);
  }
  bool res_after = result_by_mode (*wc, m_min_wc);

  return res_after - res_before;
}

int 
MergeOp::compare_ns () const
{
  return result_by_mode (m_wc_n, m_min_wc) - result_by_mode (m_wc_s, m_min_wc);
}

// -------------------------------------------------------------------------------
//  BooleanOp implementation

BooleanOp::BooleanOp (BoolOp mode)
  : m_wc_na (0), m_wc_nb (0), m_wc_sa (0), m_wc_sb (0), m_mode (mode), m_zeroes (0)
{
  //  .. nothing yet ..
}

void  
BooleanOp::reset ()
{
  m_wcv_n.clear ();
  m_wcv_s.clear ();
  m_wc_na = m_wc_sa = 0;
  m_wc_nb = m_wc_sb = 0;
  m_zeroes = 0;
}

void 
BooleanOp::reserve (size_t n)
{
  m_wcv_n.clear ();
  m_wcv_s.clear ();
  m_wcv_n.resize (n, 0);
  m_wcv_s.resize (n, 0);
  m_zeroes = 2 * n;
}

template <class InsideFunc>
inline bool 
BooleanOp::result (int wca, int wcb, const InsideFunc &inside_a, const InsideFunc &inside_b) const
{
  switch (m_mode) {
  case BooleanOp::And:
    return inside_a (wca) && inside_b (wcb);
  case BooleanOp::ANotB:
    return inside_a (wca) && ! inside_b (wcb);
  case BooleanOp::BNotA:
    return ! inside_a (wca) && inside_b (wcb);
  case BooleanOp::Xor:
    return (inside_a (wca) && ! inside_b (wcb)) || (! inside_a (wca) && inside_b (wcb));
  case BooleanOp::Or:
    return inside_a (wca) || inside_b (wcb);
  default:
    return false;
  }
}

template <class InsideFunc>
inline int 
BooleanOp::edge_impl (bool north, bool enter, property_type p, const InsideFunc &inside_a, const InsideFunc &inside_b) 
{
  tl_assert (p < m_wcv_n.size () && p < m_wcv_s.size ());

  int *wcv = north ? &m_wcv_n [p] : &m_wcv_s [p];
  int *wca = north ? &m_wc_na : &m_wc_sa;
  int *wcb = north ? &m_wc_nb : &m_wc_sb;

  bool inside_before = ((p % 2) == 0 ? inside_a (*wcv) : inside_b (*wcv));
  *wcv += (enter ? 1 : -1);
  bool inside_after = ((p % 2) == 0 ? inside_a (*wcv) : inside_b (*wcv));
  m_zeroes += (!inside_after) - (!inside_before);
#ifdef DEBUG_BOOLEAN
  printf ("north=%d, enter=%d, prop=%d -> %d\n", north, enter, int (p), int (m_zeroes));
#endif
  tl_assert (long (m_zeroes) >= 0);

  bool res_before = result (*wca, *wcb, inside_a, inside_b);
  if (inside_before != inside_after) {
    if ((p % 2) == 0) {
      *wca += (inside_after - inside_before);
    } else {
      *wcb += (inside_after - inside_before);
    }
  }
  bool res_after = result (*wca, *wcb, inside_a, inside_b);

  return res_after - res_before;
}

template <class InsideFunc> 
inline int 
BooleanOp::compare_ns_impl (const InsideFunc &inside_a, const InsideFunc &inside_b) const
{
  return result (m_wc_na, m_wc_nb, inside_a, inside_b) - result (m_wc_sa, m_wc_sb, inside_a, inside_b);
}

int 
BooleanOp::edge (bool north, bool enter, property_type p)
{
  NonZeroInsideFunc inside;
  return edge_impl (north, enter, p, inside, inside);
}

int 
BooleanOp::compare_ns () const
{
  NonZeroInsideFunc inside;
  return compare_ns_impl (inside, inside);
}

// -------------------------------------------------------------------------------
//  BooleanOp2 implementation

BooleanOp2::BooleanOp2 (BoolOp op, int wc_mode_a, int wc_mode_b)
  : BooleanOp (op), m_wc_mode_a (wc_mode_a), m_wc_mode_b (wc_mode_b)
{
  //  .. nothing yet ..
}

int 
BooleanOp2::edge (bool north, bool enter, property_type p)
{
  ParametrizedInsideFunc inside_a (m_wc_mode_a);
  ParametrizedInsideFunc inside_b (m_wc_mode_b);
  return edge_impl (north, enter, p, inside_a, inside_b);
}

int 
BooleanOp2::compare_ns () const
{
  ParametrizedInsideFunc inside_a (m_wc_mode_a);
  ParametrizedInsideFunc inside_b (m_wc_mode_b);
  return compare_ns_impl (inside_a, inside_b);
}

// -------------------------------------------------------------------------------
//  EdgeProcessor implementation

EdgeProcessor::EdgeProcessor (bool report_progress, const std::string &progress_desc)
  : m_report_progress (report_progress), m_progress_desc (progress_desc), m_base_verbosity (30)
{
  mp_work_edges = new std::vector <WorkEdge> ();
  mp_cpvector = new std::vector <CutPoints> ();
}

EdgeProcessor::~EdgeProcessor ()
{
  if (mp_work_edges) {
    delete mp_work_edges;
    mp_work_edges = 0;
  }
  if (mp_cpvector) {
    delete mp_cpvector;
    mp_cpvector = 0;
  }
}

void 
EdgeProcessor::disable_progress ()
{
  m_report_progress = false;
}

void 
EdgeProcessor::enable_progress (const std::string &progress_desc)
{
  m_report_progress = true;
  m_progress_desc = progress_desc;
}

void
EdgeProcessor::set_base_verbosity (int bv)
{
  m_base_verbosity = bv;
}

void 
EdgeProcessor::reserve (size_t n)
{
  mp_work_edges->reserve (n);
}

void 
EdgeProcessor::insert (const db::Edge &e, EdgeProcessor::property_type p)
{
  if (e.p1 () != e.p2 ()) {
    mp_work_edges->push_back (WorkEdge (e, p));
  }
}

void
EdgeProcessor::insert (const db::SimplePolygon &q, EdgeProcessor::property_type p)
{
  for (db::SimplePolygon::polygon_edge_iterator e = q.begin_edge (); ! e.at_end (); ++e) {
    insert (*e, p);
  }
}

void
EdgeProcessor::insert (const db::Polygon &q, EdgeProcessor::property_type p)
{
  for (db::Polygon::polygon_edge_iterator e = q.begin_edge (); ! e.at_end (); ++e) {
    insert (*e, p);
  }
}

void
EdgeProcessor::insert (const db::PolygonRef &q, EdgeProcessor::property_type p)
{
  for (db::PolygonRef::polygon_edge_iterator e = q.begin_edge (); ! e.at_end (); ++e) {
    insert (*e, p);
  }
}

void
EdgeProcessor::clear ()
{
  mp_work_edges->clear ();
  mp_cpvector->clear ();
}

static void
add_hparallel_cutpoints (WorkEdge &e1, WorkEdge &e2, const db::Box &cell, std::vector <CutPoints> &cutpoints)
{
  db::Coord e1_xmin = std::min (e1.x1 (), e1.x2 ());
  db::Coord e1_xmax = std::max (e1.x1 (), e1.x2 ());
  if (e2.x1 () > e1_xmin && e2.x1 () < e1_xmax && cell.contains (e2.p1 ())) {
    e1.make_cutpoints (cutpoints)->add (e2.p1 (), &cutpoints, false);
  }
  if (e2.x2 () > e1_xmin && e2.x2 () < e1_xmax && cell.contains (e2.p2 ())) {
    e1.make_cutpoints (cutpoints)->add (e2.p2 (), &cutpoints, false);
  }
}

static void
get_intersections_per_band_90 (std::vector <CutPoints> &cutpoints, std::vector <WorkEdge>::iterator current, std::vector <WorkEdge>::iterator future, db::Coord y, db::Coord yy, bool with_h)
{
  std::sort (current, future, edge_xmin_compare<db::Coord> ());

#ifdef DEBUG_EDGE_PROCESSOR
  printf ("y=%d..%d (90 degree)\n", y, yy);
  printf ("edges:"); 
  for (std::vector <WorkEdge>::iterator c1 = current; c1 != future; ++c1) { 
    printf (" %s", c1->to_string().c_str ()); 
  } 
  printf ("\n");
#endif
  db::Coord x = edge_xmin (*current);

  std::vector <WorkEdge>::iterator f = current;
  for (std::vector <WorkEdge>::iterator c = current; c != future; ) {

    size_t n = 0;
    db::Coord xx = x;

    //  fetch as many cells as to fill in roughly 50% more
    //  (this is an empirical performance improvement factor)
    do {

      while (f != future && edge_xmin (*f) <= xx) {
        ++f;
      }

      if (f != future) {
        xx = edge_xmin (*f);
      } else {
        xx = std::numeric_limits <db::Coord>::max ();
      }

      if (n == 0) {
        n = std::distance (c, f);
      }

    } while (f != future && std::distance (c, f) < long (n * fill_factor));

#ifdef DEBUG_EDGE_PROCESSOR
    printf ("edges %d..%d:", x, xx); 
    for (std::vector <WorkEdge>::iterator c1 = c; c1 != f; ++c1) { 
      printf (" %s", c1->to_string().c_str ()); 
    } 
    printf ("\n");
#endif

    if (std::distance (c, f) > 1) {

      db::Box cell (x, y, xx, yy);

      for (std::vector <WorkEdge>::iterator c1 = c; c1 != f; ++c1) {

        bool c1p1_in_cell = cell.contains (c1->p1 ());
        bool c1p2_in_cell = cell.contains (c1->p2 ());

        for (std::vector <WorkEdge>::iterator c2 = c; c2 != f; ++c2) {

          if (c1 == c2) {
            continue;
          }

          if (c2->dy () == 0) {

            if ((with_h || c1->dy () != 0) && c1 < c2) {

              if (c1->dy () == 0) {

                //  parallel horizontal edges: produce the end points of each other edge as cutpoints
                if (c1->p1 ().y () == c2->p1 ().y ()) {
                  add_hparallel_cutpoints (*c1, *c2, cell, cutpoints);
                  add_hparallel_cutpoints (*c2, *c1, cell, cutpoints);
                }

              } else if (c1->p1 () != c2->p1 () && c1->p2 () != c2->p1 () &&
                         c1->p1 () != c2->p2 () && c1->p2 () != c2->p2 ()) {

                std::pair <bool, db::Point> cp = c1->intersect_point (*c2);
                if (cp.first && cell.contains (cp.second)) {

                  //  add a cut point to c1 and c2 (c2 only if necessary)
                  c1->make_cutpoints (cutpoints)->add (cp.second, &cutpoints, true);
                  if (with_h) {
                    c2->make_cutpoints (cutpoints)->add (cp.second, &cutpoints, true);
                  }

#ifdef DEBUG_EDGE_PROCESSOR
                  printf ("intersection point %s between %s and %s (1).\n", cp.second.to_string ().c_str (), c1->to_string ().c_str (), c2->to_string ().c_str ());
#endif

                }

              }

            } 
          
          } else if (c1->dy () == 0) {
            
            if (c1 < c2 && c1->p1 () != c2->p1 () && c1->p2 () != c2->p1 () &&
                           c1->p1 () != c2->p2 () && c1->p2 () != c2->p2 ()) {

              std::pair <bool, db::Point> cp = c1->intersect_point (*c2);
              if (cp.first && cell.contains (cp.second)) {
                
                //  add a cut point to c1 and c2
                c2->make_cutpoints (cutpoints)->add (cp.second, &cutpoints, true);
                if (with_h) {
                  c1->make_cutpoints (cutpoints)->add (cp.second, &cutpoints, true);
                }

#ifdef DEBUG_EDGE_PROCESSOR
                printf ("intersection point %s between %s and %s (2).\n", cp.second.to_string ().c_str (), c1->to_string ().c_str (), c2->to_string ().c_str ()); 
#endif

              }

            }

          } else if (c1->p1 ().x () == c2->p1 ().x ()) {

            //  both edges are coincident - produce the ends of the edges involved as cut points
            if (c1p1_in_cell && c1->p1 ().y () > db::edge_ymin (*c2) && c1->p1 ().y () < db::edge_ymax (*c2)) {
              c2->make_cutpoints (cutpoints)->add (c1->p1 (), &cutpoints, true);
            }
            if (c1p2_in_cell && c1->p2 ().y () > db::edge_ymin (*c2) && c1->p2 ().y () < db::edge_ymax (*c2)) {
              c2->make_cutpoints (cutpoints)->add (c1->p2 (), &cutpoints, true);
            }

          }

        }

      }

    }

    x = xx;
    for (std::vector <WorkEdge>::iterator cc = c; cc != f; ++cc) {
      if (edge_xmax (*cc) < x) {
        if (c != cc) {
          std::swap (*cc, *c);
        }
        ++c;
      }
    }

  }
}

/**
 *  @brief Computes the x value of an edge at the given y value
 *
 *  HINT: for application in the scanline algorithm 
 *  it is important that this method delivers exactly (!) the same x for the same edge 
 *  (after normalization to dy()>0) and same y!
 */
template <class C>
inline double edge_xaty_double (db::edge<C> e, double y)
{
  if (e.p1 ().y () > e.p2 ().y ()) {
    e.swap_points ();
  }

  if (y <= e.p1 ().y ()) {
    return e.p1 ().x ();
  } else if (y >= e.p2 ().y ()) {
    return e.p2 ().x ();
  } else {
    return double (e.p1 ().x ()) + double (e.dx ()) * (y - double (e.p1 ().y ())) / double (e.dy ());
  }
}

/**
 *  @brief Computes the left bound of the edge geometry for a given band [y1..y2].
 */
template <class C>
inline C edge_xmin_at_yinterval_double (const db::edge<C> &e, double y1, double y2) 
{
  if (e.dx () == 0) {
    return e.p1 ().x ();
  } else if (e.dy () == 0) {
    return std::min (e.p1 ().x (), e.p2 ().x ());
  } else {
    return C (floor (edge_xaty_double (e, ((e.dy () < 0) ^ (e.dx () < 0)) == 0 ? y1 : y2)));
  }
}

/**
 *  @brief Computes the right bound of the edge geometry for a given band [y1..y2].
 */
template <class C>
inline C edge_xmax_at_yinterval_double (const db::edge<C> &e, double y1, double y2) 
{
  if (e.dx () == 0) {
    return e.p1 ().x ();
  } else if (e.dy () == 0) {
    return std::max (e.p1 ().x (), e.p2 ().x ());
  } else {
    return C (ceil (edge_xaty_double (e, ((e.dy () < 0) ^ (e.dx () < 0)) != 0 ? y1 : y2)));
  }
}

/**
 *  @brief Functor that compares two edges by their left bound for a given interval [y1..y2].
 *
 *  This function is intended for use in scanline scenarios to determine what edges are 
 *  interacting in a certain y interval.
 */
template <class C>
struct edge_xmin_at_yinterval_double_compare
{
  edge_xmin_at_yinterval_double_compare (double y1, double y2)
    : m_y1 (y1), m_y2 (y2)
  {
    // .. nothing yet ..
  }

  bool operator() (const db::edge<C> &a, const db::edge<C> &b) const
  {
    if (edge_xmax (a) < edge_xmin (b)) {
      return true;
    } else if (edge_xmin (a) >= edge_xmax (b)) {
      return false;
    } else {
      C xa = edge_xmin_at_yinterval_double (a, m_y1, m_y2);
      C xb = edge_xmin_at_yinterval_double (b, m_y1, m_y2);
      if (xa != xb) {
        return xa < xb;
      } else {
        return a < b;
      }
    }
  }

public:
  double m_y1, m_y2;
};

static void 
get_intersections_per_band_any (std::vector <CutPoints> &cutpoints, std::vector <WorkEdge>::iterator current, std::vector <WorkEdge>::iterator future, db::Coord y, db::Coord yy, bool with_h)
{
  double dy = y - 0.5;
  double dyy = yy + 0.5;
  std::vector <std::pair<const WorkEdge *, WorkEdge *> > p1_weak;   // holds weak interactions of edge endpoints with other edges

  std::sort (current, future, edge_xmin_at_yinterval_double_compare<db::Coord> (dy, dyy));

#ifdef DEBUG_EDGE_PROCESSOR
  printf ("y=%d..%d\n", y, yy);
  printf ("edges:"); 
  for (std::vector <WorkEdge>::iterator c1 = current; c1 != future; ++c1) { 
    printf (" %s", c1->to_string().c_str ()); 
  } 
  printf ("\n");
#endif
  db::Coord x = edge_xmin_at_yinterval_double (*current, dy, dyy);

  std::vector <WorkEdge>::iterator f = current;
  for (std::vector <WorkEdge>::iterator c = current; c != future; ) {

    size_t n = 0;
    db::Coord xx = x;

    //  fetch as many cells as to fill in roughly 50% more
    //  (this is an empirical performance improvement factor)
    do {

      while (f != future && edge_xmin_at_yinterval_double (*f, dy, dyy) <= xx) {
        ++f;
      }

      if (f != future) {
        xx = edge_xmin_at_yinterval_double (*f, dy, dyy);
      } else {
        xx = std::numeric_limits <db::Coord>::max ();
      }

      if (n == 0) {
        n = std::distance (c, f);
      }

    } while (f != future && std::distance (c, f) < long (n * fill_factor));

#ifdef DEBUG_EDGE_PROCESSOR
    printf ("edges %d..%d:", x, xx); 
    for (std::vector <WorkEdge>::iterator c1 = c; c1 != f; ++c1) { 
      printf (" %s", c1->to_string().c_str ()); 
    } 
    printf ("\n");
#endif

    if (std::distance (c, f) > 1) {

      db::Box cell (x, y, xx, yy);

      std::set<db::Point> weak_points;    // holds points that need to go in all other edges
      p1_weak.clear ();

      for (std::vector <WorkEdge>::iterator c1 = c; c1 != f; ++c1) {

        bool c1p1_in_cell = cell.contains (c1->p1 ());
        bool c1p2_in_cell = cell.contains (c1->p2 ());

        for (std::vector <WorkEdge>::iterator c2 = c; c2 != f; ++c2) {

          if (c1 == c2) {
            continue;
          }

          if (c2->dy () == 0) {

            if ((with_h || c1->dy () != 0) && c1 < c2) {

              if (c1->dy () == 0) {

                //  parallel horizontal edges: produce the end points of each other edge as cutpoints
                if (c1->p1 ().y () == c2->p1 ().y ()) {
                  add_hparallel_cutpoints (*c1, *c2, cell, cutpoints);
                  add_hparallel_cutpoints (*c2, *c1, cell, cutpoints);
                }

              } else if (c1->p1 () != c2->p1 () && c1->p2 () != c2->p1 () &&
                         c1->p1 () != c2->p2 () && c1->p2 () != c2->p2 ()) {

                std::pair <bool, db::Point> cp = safe_intersect_point (*c1, *c2);
                if (cp.first && cell.contains (cp.second)) {
                  //  Stash the cutpoint as it must be inserted into other edges as well.
                  weak_points.insert (cp.second);
                }

              }

            } 
          
          } else if (c1->parallel (*c2) && c1->side_of (c2->p1 ()) == 0) {

#ifdef DEBUG_EDGE_PROCESSOR
            printf ("%s and %s are parallel.\n", c1->to_string ().c_str (), c2->to_string ().c_str ()); 
#endif

            //  both edges are coincident - produce the ends of the edges involved as cut points
            if (c1p1_in_cell && c2->contains (c1->p1 ()) && c2->p1 () != c1->p1 () && c2->p2 () != c1->p1 ()) {
              c2->make_cutpoints (cutpoints)->add (c1->p1 (), &cutpoints, !is_point_on_exact(*c2, c1->p1 ()));
#ifdef DEBUG_EDGE_PROCESSOR
              if (! is_point_on_exact(*c2, c1->p1 ())) {
                printf ("intersection point %s between %s and %s.\n", c1->p1 ().to_string ().c_str (), c1->to_string ().c_str (), c2->to_string ().c_str ()); 
              } else {
                printf ("weak intersection point %s between %s and %s.\n", c1->p1 ().to_string ().c_str (), c1->to_string ().c_str (), c2->to_string ().c_str ()); 
              }
#endif
            }
            if (c1p2_in_cell && c2->contains (c1->p2 ()) && c2->p1 () != c1->p2 () && c2->p2 () != c1->p2 ()) {
              c2->make_cutpoints (cutpoints)->add (c1->p2 (), &cutpoints, !is_point_on_exact(*c2, c1->p2 ()));
#ifdef DEBUG_EDGE_PROCESSOR
              if (! is_point_on_exact(*c2, c1->p2 ())) {
                printf ("intersection point %s between %s and %s.\n", c1->p2 ().to_string ().c_str (), c1->to_string ().c_str (), c2->to_string ().c_str ()); 
              } else {
                printf ("weak intersection point %s between %s and %s.\n", c1->p2 ().to_string ().c_str (), c1->to_string ().c_str (), c2->to_string ().c_str ()); 
              }
#endif
            }

          } else {

            if (c1 < c2 && c1->p1 () != c2->p1 () && c1->p2 () != c2->p1 () &&
                           c1->p1 () != c2->p2 () && c1->p2 () != c2->p2 ()) {

              std::pair <bool, db::Point> cp = safe_intersect_point (*c1, *c2);
              if (cp.first && cell.contains (cp.second)) {
                //  Stash the cutpoint as it must be inserted into other edges as well.
                weak_points.insert (cp.second);
              }

            } 

            //  The endpoints of the other edge must be inserted into the edge 
            //  if they are within the modification range (but only then).
            //  We first collect these endpoints because we have to decide whether that can be 
            //  a weak attractor or, if it affects two or more edges in which case it will become a strong attractor. 
            //  It's sufficient to do this for p1 only because we made sure we caught all edges
            //  in the +-0.5DBU vicinity by choosing the cell large enough (.._double operators).
            //  For end points exactly on the line we insert a cutpoint to ensure we use the
            //  endpoints as cutpoints in any case.
            if (c1p1_in_cell && is_point_on_fuzzy (*c2, c1->p1 ())) {
              if (is_point_on_exact (*c2, c1->p1 ())) {
#ifdef DEBUG_EDGE_PROCESSOR
                printf ("end point %s gives intersection point between %s and %s.\n", c1->p1 ().to_string ().c_str (), c1->to_string ().c_str (), c2->to_string ().c_str ()); 
#endif
                c2->make_cutpoints (cutpoints)->add (c1->p1 (), &cutpoints, true);
              } else {
                p1_weak.push_back (std::make_pair (c1.operator-> (), c2.operator-> ()));
              }
            }

          }

        }

      }

      //  insert weak intersection points into all relevant edges - weak into edges
      //  where the point is on and strong into edges where the point is on in a fuzzy way.

      for (auto wp = weak_points.begin (); wp != weak_points.end (); ++wp) {

        for (std::vector <WorkEdge>::iterator cc = c; cc != f; ++cc) {
          if ((with_h || cc->dy () != 0) && is_point_on_fuzzy (*cc, *wp)) {
            bool on_edge = is_point_on_exact (*cc, *wp);
            cc->make_cutpoints (cutpoints)->add (*wp, &cutpoints, !on_edge);
#ifdef DEBUG_EDGE_PROCESSOR
            if (!on_edge) {
              printf ("intersection point %s gives strong cutpoint in %s.\n", wp->to_string ().c_str (), cc->to_string ().c_str ());
            } else {
              printf ("intersection point %s gives weak cutpoint in %s.\n", wp->to_string ().c_str (), cc->to_string ().c_str ());
            }
#endif
          }
        }

      }

      //  go through the list of "p1 to other edges" and insert p1 either as cutpoint
      //  (if there are other strong cutpoints already) or weak attractor.

      auto p1w_from = p1_weak.begin ();
      while (p1w_from != p1_weak.end ()) {

        bool strong = false;
        auto p1w_to = p1w_from;
        while (p1w_to != p1_weak.end () && p1w_to->first == p1w_from->first) {
          if (p1w_to->second->data > 0 && cutpoints [p1w_to->second->data - 1].strong_cutpoints) {
            strong = true;
          }
          ++p1w_to;
        }

        db::Point p1 = p1w_from->first->p1 ();

        p1w_to [-1].second->make_cutpoints (cutpoints);
        size_t n = p1w_to [-1].second->data - 1;

        for (auto cp = p1w_from; cp != p1w_to; ++cp) {

          cp->second->make_cutpoints (cutpoints);
          size_t nn = cp->second->data - 1;
          if (strong) {
            cutpoints [nn].add (p1, &cutpoints);
#ifdef DEBUG_EDGE_PROCESSOR
            printf ("Insert strong attractor %s in %s.\n", cp->first->p1 ().to_string ().c_str (), cp->second->to_string ().c_str ());
#endif
          } else {
            cutpoints [nn].add_attractor (p1, n);
#ifdef DEBUG_EDGE_PROCESSOR
            printf ("Insert weak attractor %s in %s.\n", cp->first->p1 ().to_string ().c_str (), cp->second->to_string ().c_str ());
#endif
          }

          n = nn;

        }

        p1w_from = p1w_to;

      }

    }

    x = xx;
    for (std::vector <WorkEdge>::iterator cc = c; cc != f; ++cc) {
      if (edge_xmax (*cc) < x || edge_xmax_at_yinterval_double (*cc, dy, dyy) < x) {
        if (c != cc) {
          std::swap (*cc, *c);
        }
        ++c;
      }
    }

  }
}

void 
EdgeProcessor::process (db::EdgeSink &es, EdgeEvaluatorBase &op)
{
  std::vector<std::pair<db::EdgeSink *, db::EdgeEvaluatorBase *> > procs;
  procs.push_back (std::make_pair (&es, &op));
  process (procs);
}

void
EdgeProcessor::redo (db::EdgeSink &es, EdgeEvaluatorBase &op)
{
  std::vector<std::pair<db::EdgeSink *, db::EdgeEvaluatorBase *> > procs;
  procs.push_back (std::make_pair (&es, &op));
  redo (procs);
}

namespace
{

class EdgeProcessorState
{
public:
  EdgeProcessorState (db::EdgeSink *es, db::EdgeEvaluatorBase *op)
    : mp_es (es), mp_op (op), m_vertex (false),
      m_x (0), m_y (0), m_hx (0), m_ho (0), m_pn (0), m_ps (0)
  { }

  void start ()
  {
    mp_es->start ();
  }

  void flush ()
  {
    mp_es->flush ();
  }

  void reset ()
  {
    mp_es->reset_stop ();
    mp_op->reset ();
  }

  bool is_reset ()
  {
    return mp_op->is_reset ();
  }

  bool can_stop ()
  {
    return mp_es->can_stop ();
  }

  void reserve (size_t n)
  {
    mp_op->reserve (n);
  }

  void begin_scanline (db::Coord y)
  {
    m_y = y;
    m_x = 0;
    m_hx = 0;
    m_ho = 0;
    m_vertex = false;
    mp_es->begin_scanline (y);
  }

  void end_scanline (db::Coord y)
  {
    mp_es->end_scanline (y);
  }

  void next_vertex (double x)
  {
    m_x = db::coord_traits<db::Coord>::rounded (x);
    m_vertex = false;
  }

  void end_vertex ()
  {
    if (m_vertex) {
      m_hx = m_x;
      m_ho = mp_op->compare_ns ();
    }
  }

  void next_coincident ()
  {
    m_pn = m_ps = 0;
  }

  void end_coincident ()
  {
    if (! m_vertex && (m_ps != 0 || m_pn != 0)) {

      if (m_ho != 0) {
        db::Edge he (db::Point (m_hx, m_y), db::Point (db::coord_traits<db::Coord>::rounded (m_x), m_y));
        if (m_ho > 0) {
          he.swap_points ();
        }
        mp_es->put (he);
#ifdef DEBUG_EDGE_PROCESSOR
        printf ("put(%s)\n", he.to_string ().c_str ());
#endif
      }

      m_vertex = true;

    }
  }

  void north_edge (bool prefer_touch, EdgeEvaluatorBase::property_type prop)
  {
    m_pn += mp_op->edge (true, prefer_touch, prop);
  }

  void south_edge (bool prefer_touch, EdgeEvaluatorBase::property_type prop)
  {
    m_ps += mp_op->edge (false, prefer_touch, prop);
  }

  void select_edge (const WorkEdge &e)
  {
    int tag = mp_op->select_edge (e.dy () == 0, e.prop);
    if (tag > 0) {
      mp_es->put (e, (unsigned int) tag);
#ifdef DEBUG_EDGE_PROCESSOR
      printf ("put(%s, %d)\n", e.to_string().c_str(), tag);
#endif
    }
  }

  bool push_edge (const db::Edge &e)
  {
    if (m_pn != 0) {

      db::Edge edge (e);
      if ((m_pn > 0 && edge.dy () < 0) || (m_pn < 0 && edge.dy () > 0)) {
        edge.swap_points ();
      }

      if (edge_ymin (edge) == m_y) {
        mp_es->put (edge);
#ifdef DEBUG_EDGE_PROCESSOR
        printf ("put(%s)\n", edge.to_string().c_str());
#endif
      } else {
        mp_es->crossing_edge (edge);
#ifdef DEBUG_EDGE_PROCESSOR
        printf ("xing(%s)\n", edge.to_string().c_str());
#endif
      }

      return true;

    } else {
      return false;
    }
  }

  void skip_n (size_t n)
  {
    mp_es->skip_n (n);
  }

private:
  db::EdgeSink *mp_es;
  db::EdgeEvaluatorBase *mp_op;

  bool m_vertex;
  db::Coord m_x, m_y, m_hx;
  int m_ho;
  int m_pn, m_ps;
};

//  NOTE: set this to 0 to force memory-allocation storage for SkipInfo always (testing)
const size_t skip_info_storage_threshold = 1;

/**
 *  @brief Encapsulates the state of the edge processor's generation stage
 *
 *  The generation state may involve multiple generators and output sinks. This
 *  class provides a single interface to handle the case of single and multiple
 *  receivers in a uniform way.
 */
class EdgeProcessorStates
{
public:
  /**
   *  @brief A structure holding the "skip information"
   *
   *  Skipping intervals with a known behavior is an optimization to improve
   *  the scanner's performance. This object keeps the information required to
   *  properly implement the skipping. It keeps both the edge skip count per
   *  interval ("skip") as well as the corresponding skip count for the
   *  generated edges. As multiple edge receivers can be supplied, the result
   *  skip count is a individual one per generator and edge receiver.
   */
  struct SkipInfo
  {
    SkipInfo (size_t _skip, const std::vector<size_t> &_skip_res)
      : skip (_skip), m_skip_res_n (0), m_skip_res (0)
    {
      set_skip_res (_skip_res.begin (), _skip_res.end ());
    }

    SkipInfo ()
      : skip (0), m_skip_res_n (0), m_skip_res (0)
    { }

    SkipInfo (const SkipInfo &si)
      : skip (0), m_skip_res_n (0), m_skip_res (0)
    {
      operator= (si);
    }

    ~SkipInfo ()
    {
      if (m_skip_res_n > skip_info_storage_threshold) {
        delete[] skip_res ();
      }
    }

    SkipInfo &operator= (const SkipInfo &si)
    {
      if (&si != this) {
        skip = si.skip;
        const size_t *n = si.skip_res ();
        set_skip_res (n, n + si.m_skip_res_n);
      }
      return *this;
    }

    template <class Iter>
    void set_skip_res (Iter b, Iter e)
    {
      if (m_skip_res_n > skip_info_storage_threshold) {
        delete[] (reinterpret_cast<size_t *> (m_skip_res));
      }

      m_skip_res_n = e - b;
      if (m_skip_res_n <= skip_info_storage_threshold) {
        if (b == e) {
          m_skip_res = 0;
        } else {
          m_skip_res = *b;
        }
      } else {
        size_t *t = new size_t[m_skip_res_n];
        m_skip_res = reinterpret_cast<size_t> (t);
        for (Iter i = b; i != e; ++i) {
          *t++ = *i;
        }
      }
    }

    const size_t *skip_res () const
    {
      if (m_skip_res_n <= skip_info_storage_threshold) {
        return &m_skip_res;
      } else {
        return reinterpret_cast<const size_t *> (m_skip_res);
      }
    }

    size_t skip;

  private:
    size_t m_skip_res_n;
    size_t m_skip_res;
  };

  /**
   *  @brief Creates a generator stage state object from the given sinks and operators
   */
  EdgeProcessorStates (const std::vector<std::pair<db::EdgeSink *, db::EdgeEvaluatorBase *> > &procs)
    : m_selects_edges (false), m_prefer_touch (false)
  {
    m_states.reserve (procs.size ());
    for (std::vector<std::pair<db::EdgeSink *, db::EdgeEvaluatorBase *> >::const_iterator p = procs.begin (); p != procs.end (); ++p) {

      m_states.push_back (EdgeProcessorState (p->first, p->second));

      if (p->second->selects_edges ()) {
        m_selects_edges = true;
      }

      if (p->second->prefer_touch ()) {
        m_prefer_touch = true;
      }

    }
  }

  /**
   *  @brief Returns true if the processors want to select edges
   */
  bool selects_edges () const
  {
    return m_selects_edges;
  }

  /**
   *  @brief Returns true if the processors prefer touching mode
   */
  bool prefer_touch () const
  {
    return m_prefer_touch;
  }

  /**
   *  @brief Initial event
   *  This method is called when the scan is initiated
   */
  void start ()
  {
    for (std::vector<EdgeProcessorState>::iterator s = m_states.begin (); s != m_states.end (); ++s) {
      s->start ();
    }
  }

  /**
   *  @brief Final event
   *  This method is called after the scan terminated
   */
  void flush ()
  {
    for (std::vector<EdgeProcessorState>::iterator s = m_states.begin (); s != m_states.end (); ++s) {
      s->flush ();
    }
  }

  /**
   *  @brief Reset status event
   *  This method is to ensure the state of the operator is reset.
   */
  void reset ()
  {
    for (std::vector<EdgeProcessorState>::iterator s = m_states.begin (); s != m_states.end (); ++s) {
      s->reset ();
    }
  }

  /**
   *  @brief Gets a value indicating whether all operators are reset
   */
  bool is_reset ()
  {
    for (std::vector<EdgeProcessorState>::iterator s = m_states.begin (); s != m_states.end (); ++s) {
      if (! s->is_reset ()) {
        return false;
      }
    }
    return true;
  }

  /**
   *  @brief Gets a value indicating whether the generator wants to stop
   */
  bool can_stop ()
  {
    for (std::vector<EdgeProcessorState>::iterator s = m_states.begin (); s != m_states.end (); ++s) {
      if (s->can_stop ()) {
        return true;
      }
    }
    return false;
  }

  /**
   *  @brief Reserve memory n edges
   */
  void reserve (size_t n)
  {
    for (std::vector<EdgeProcessorState>::iterator s = m_states.begin (); s != m_states.end (); ++s) {
      s->reserve (n);
    }
  }

  /**
   *  @brief Begin scanline event
   *  This method is called at the beginning of a new scanline
   */
  void begin_scanline (db::Coord y)
  {
    for (std::vector<EdgeProcessorState>::iterator s = m_states.begin (); s != m_states.end (); ++s) {
      s->begin_scanline (y);
    }
  }

  /**
   *  @brief End scanline event
   *  This method is called at the end of a scanline
   */
  void end_scanline (db::Coord y)
  {
    for (std::vector<EdgeProcessorState>::iterator s = m_states.begin (); s != m_states.end (); ++s) {
      s->end_scanline (y);
    }
  }

  /**
   *  @brief Announces a batch of edges crossing the same point
   */
  void next_vertex (double x)
  {
    for (std::vector<EdgeProcessorState>::iterator s = m_states.begin (); s != m_states.end (); ++s) {
      s->next_vertex (x);
    }
  }

  /**
   *  @brief Finishes the vertex
   */
  void end_vertex ()
  {
    for (std::vector<EdgeProcessorState>::iterator s = m_states.begin (); s != m_states.end (); ++s) {
      s->end_vertex ();
    }
  }

  /**
   *  @brief Announces a batch of edges crossing the same point and begin coincident
   *  This event is a sub-event of "next_vertex".
   */
  void next_coincident ()
  {
    for (std::vector<EdgeProcessorState>::iterator s = m_states.begin (); s != m_states.end (); ++s) {
      s->next_coincident ();
    }
  }

  /**
   *  @brief Announces a batch of edges crossing the same point and begin coincident
   *  This event is a sub-event of "next_vertex".
   */
  void end_coincident ()
  {
    for (std::vector<EdgeProcessorState>::iterator s = m_states.begin (); s != m_states.end (); ++s) {
      s->end_coincident ();
    }
  }

  /**
   *  @brief Announces an edge north to the scanline
   */
  void north_edge (bool prefer_touch, EdgeEvaluatorBase::property_type prop)
  {
    for (std::vector<EdgeProcessorState>::iterator s = m_states.begin (); s != m_states.end (); ++s) {
      s->north_edge (prefer_touch, prop);
    }
  }

  /**
   *  @brief Announces an edge south of the scanline
   */
  void south_edge (bool prefer_touch, EdgeEvaluatorBase::property_type prop)
  {
    for (std::vector<EdgeProcessorState>::iterator s = m_states.begin (); s != m_states.end (); ++s) {
      s->south_edge (prefer_touch, prop);
    }
  }

  /**
   *  @brief Gives the generators an opportunity to select the given edge
   */
  void select_edge (const WorkEdge &e)
  {
    for (std::vector<EdgeProcessorState>::iterator s = m_states.begin (); s != m_states.end (); ++s) {
      s->select_edge (e);
    }
  }

  /**
   *  @brief Delivers an edge to the edge sink if present
   *
   *  This method will return true if at least one of the edge sinks received the edge
   */
  void push_edge (const db::Edge &e)
  {
    size_t i = 0;
    for (std::vector<EdgeProcessorState>::iterator s = m_states.begin (); s != m_states.end (); ++s, ++i) {
      if (s->push_edge (e)) {
        ++m_nres [i];
      }
    }
  }

  /**
   *  @brief Skips n edges on the edge sink
   *
   *  This is for optimization of the polygon generation. Stitching of edges does not happen if
   *  there are no news.
   */
  void skip_n (const SkipInfo &si)
  {
    const size_t *n = si.skip_res ();
    for (std::vector<EdgeProcessorState>::iterator s = m_states.begin (); s != m_states.end (); ++s) {
      s->skip_n (*n++);
    }
  }

  /**
   *  @brief Gets the SkipInfo for a given index
   */
  const SkipInfo &skip_info (size_t n)
  {
    if (n == 0) {
      static SkipInfo empty;
      return empty;
    } else {
      return m_skip_info [n - 1];
    }
  }

  /**
   *  @brief Releases a SkipInfo entry
   */
  void release_skip_entry (size_t n)
  {
    m_skip_queue.push_front (n - 1);
  }

  /**
   *  @brief Resets a SkipInfo entry
   *
   *  A convenience function to reset and release a SkipInfo entry
   */
  void reset_skip_entry (size_t &n)
  {
    if (n != 0) {
      release_skip_entry (n);
      n = 0;
    }
  }

  /**
   *  @brief Begins an interval that can potentially be skipped
   */
  void begin_skip_interval ()
  {
    m_nres.clear ();
    m_nres.resize (m_states.size (), size_t (0));
  }

  /**
   *  @brief Finishes an interval that can potentially be skipped
   *
   *  Returns the index of a new skip interval entry containing the skip information.
   */
  size_t end_skip_interval (size_t skip)
  {
    size_t n = 0;

    if (! m_skip_queue.empty ()) {
      n = m_skip_queue.front ();
      m_skip_queue.pop_front ();
    } else {
      n = m_skip_info.size ();
      m_skip_info.push_back (SkipInfo ());
    }

    m_skip_info[n].skip = skip;
    m_skip_info[n].set_skip_res (m_nres.begin (), m_nres.end ());
    return n + 1;
  }

private:
  std::vector<EdgeProcessorState> m_states;
  bool m_selects_edges, m_prefer_touch;
  std::vector<SkipInfo> m_skip_info;
  std::list<size_t> m_skip_queue;
  std::vector<size_t> m_nres;
};

}

void
EdgeProcessor::redo (const std::vector<std::pair<db::EdgeSink *, db::EdgeEvaluatorBase *> > &gen)
{
  redo_or_process (gen, true);
}

void
EdgeProcessor::process (const std::vector<std::pair<db::EdgeSink *, db::EdgeEvaluatorBase *> > &gen)
{
  redo_or_process (gen, false);
}

void
EdgeProcessor::redo_or_process (const std::vector<std::pair<db::EdgeSink *, db::EdgeEvaluatorBase *> > &gen, bool redo)
{
  tl::SelfTimer timer (tl::verbosity () >= m_base_verbosity, "EdgeProcessor: process");

  EdgeProcessorStates gs (gen);

  bool prefer_touch = gs.prefer_touch ();
  bool selects_edges = gs.selects_edges ();
  
  db::Coord y;
  std::vector <WorkEdge>::iterator future;

  //  step 1: preparation

  if (mp_work_edges->empty ()) {
    gs.start ();
    gs.flush ();
    return;
  }

  mp_cpvector->clear ();

  //  count the properties

  property_type n_props = 0;
  for (std::vector <WorkEdge>::iterator e = mp_work_edges->begin (); e != mp_work_edges->end (); ++e) {
    if (e->prop > n_props) {
      n_props = e->prop;
    }
  }
  ++n_props;

  //  prepare progress

  size_t todo_max = 1000000;

  std::unique_ptr<tl::AbsoluteProgress> progress;
  if (m_report_progress) {
    if (m_progress_desc.empty ()) {
      progress.reset (new tl::AbsoluteProgress (tl::to_string (tr ("Processing")), 1000));
    } else {
      progress.reset (new tl::AbsoluteProgress (m_progress_desc, 1000));
    }
    progress->set_format (tl::to_string (tr ("%.0f%%")));
    progress->set_unit (todo_max / 100);
  }

  size_t todo_next = 0;
  size_t todo = todo_next;
  todo_next += (todo_max - todo) / 5;


  if (redo) {

    //  redo mode: skip the intersection detection step and clear the data

    for (std::vector <WorkEdge>::iterator c = mp_work_edges->begin (); c != mp_work_edges->end (); ++c) {
      c->data = 0;
    }

    todo = todo_next;
    todo_next += (todo_max - todo) / 5;

  } else {

    //  step 2: find intersections
    std::sort (mp_work_edges->begin (), mp_work_edges->end (), edge_ymin_compare<db::Coord> ());

    y = edge_ymin ((*mp_work_edges) [0]);
    future = mp_work_edges->begin ();

    for (std::vector <WorkEdge>::iterator current = mp_work_edges->begin (); current != mp_work_edges->end (); ) {

      if (m_report_progress) {
        double p = double (std::distance (mp_work_edges->begin (), current)) / double (mp_work_edges->size ());
        progress->set (size_t (double (todo_next - todo) * p) + todo);
      }

      size_t n = 0;
      db::Coord yy = y;

      //  Use as many scanlines as to fetch approx. 50% new edges into the scanline (this
      //  is an empirically determined factor)
      do {

        while (future != mp_work_edges->end () && edge_ymin (*future) <= yy) {
          ++future;
        }

        if (future != mp_work_edges->end ()) {
          yy = edge_ymin (*future);
        } else {
          yy = std::numeric_limits <db::Coord>::max ();
        }

        if (n == 0) {
          n = std::distance (current, future);
        }

      } while (future != mp_work_edges->end () && std::distance (current, future) < long (n * fill_factor));

      bool is90 = true;

      if (current != future) {

        for (std::vector <WorkEdge>::iterator c = current; c != future && is90; ++c) {
          if (c->dx () != 0 && c->dy () != 0) {
            is90 = false;
          }
        }

        if (is90) {
          get_intersections_per_band_90 (*mp_cpvector, current, future, y, yy, selects_edges);
        } else {
          get_intersections_per_band_any (*mp_cpvector, current, future, y, yy, selects_edges);
        }

      }

      y = yy;
      for (std::vector <WorkEdge>::iterator c = current; c != future; ++c) {
        //  Hint: we have to keep the edges ending a y (the new lower band limit) in the all angle case because these edges
        //  may receive cutpoints because the enter the -0.5DBU region below the band
        if ((!is90 && edge_ymax (*c) < y) || (is90 && edge_ymax (*c) <= y)) {
          if (current != c) {
            std::swap (*current, *c);
          }
          ++current;
        }
      }

    }

    //  step 3: create new edges from the ones with cutpoints
    //
    //  Hint: when we create the edges from the cutpoints we use the projection to sort the cutpoints along the
    //  edge. However, we have some freedom to connect the points which we use to avoid "z" configurations which could
    //  create new intersections in a 1x1 pixel box.

    todo = todo_next;
    todo_next += (todo_max - todo) / 5;

    size_t n_work = mp_work_edges->size ();
    size_t nw = 0;
    for (size_t n = 0; n < n_work; ++n) {

      if (m_report_progress) {
        double p = double (n) / double (n_work);
        progress->set (size_t (double (todo_next - todo) * p) + todo);
      }

      WorkEdge &ew = (*mp_work_edges) [n];

      CutPoints *cut_points = ew.data ? & ((*mp_cpvector) [ew.data - 1]) : 0;
      ew.data = 0;

      if (ew.dy () == 0 && ! selects_edges) {

        //  don't care about horizontal edges

      } else if (cut_points) {

        if (cut_points->has_cutpoints && ! cut_points->cut_points.empty ()) {

          db::Edge e = ew;
          property_type p = ew.prop;
          std::sort (cut_points->cut_points.begin (), cut_points->cut_points.end (), ProjectionCompare (e));

          db::Point pll = e.p1 ();
          db::Point pl = e.p1 ();

          for (std::vector <db::Point>::iterator cp = cut_points->cut_points.begin (); cp != cut_points->cut_points.end (); ++cp) {
            if (*cp != pl) {
              WorkEdge ne = WorkEdge (db::Edge (pl, *cp), p);
              if (pl.y () == pll.y () && ne.p2 ().x () != pl.x () && ne.p2 ().x () == pll.x ()) {
                ne = db::Edge (pll, ne.p2 ());
              } else if (pl.x () == pll.x () && ne.p2 ().y () != pl.y () && ne.p2 ().y () == pll.y ()) {
                ne = db::Edge (ne.p1 (), pll);
              } else {
                pll = pl;
              }
              pl = *cp;
              if (selects_edges || ne.dy () != 0) {
                if (nw <= n) {
                  (*mp_work_edges) [nw++] = ne;
                } else {
                  mp_work_edges->push_back (ne);
                }
              }
            }
          }

          if (cut_points->cut_points.back () != e.p2 ()) {
            WorkEdge ne = WorkEdge (db::Edge (pl, e.p2 ()), p);
            if (pl.y () == pll.y () && ne.p2 ().x () != pl.x () && ne.p2 ().x () == pll.x ()) {
              ne = db::Edge (pll, ne.p2 ());
            } else if (pl.x () == pll.x () && ne.p2 ().y () != pl.y () && ne.p2 ().y () == pll.y ()) {
              ne = db::Edge (ne.p1 (), pll);
            }
            if (selects_edges || ne.dy () != 0) {
              if (nw <= n) {
                (*mp_work_edges) [nw++] = ne;
              } else {
                mp_work_edges->push_back (ne);
              }
            }
          }

        } else {

          if (nw < n) {
            (*mp_work_edges) [nw] = (*mp_work_edges) [n];
          }
          ++nw;

        }

      } else {

        if (nw < n) {
          (*mp_work_edges) [nw] = (*mp_work_edges) [n];
        }
        ++nw;

      }

    }

    if (nw != n_work) {
      mp_work_edges->erase (mp_work_edges->begin () + nw, mp_work_edges->begin () + n_work);
    }

#ifdef DEBUG_EDGE_PROCESSOR
    printf ("Output edges:\n");
    for (std::vector <WorkEdge>::iterator c1 = mp_work_edges->begin (); c1 != mp_work_edges->end (); ++c1) {
      printf ("%s\n", c1->to_string().c_str ());
    }
#endif

  }


  tl::SelfTimer timer2 (tl::verbosity () >= m_base_verbosity + 10, "EdgeProcessor: production");

  //  step 4: compute the result edges 
  
  gs.start (); // call this as late as possible. This way, input containers can be identical with output containers ("clear" is done after the input is read)

  gs.reset ();
  gs.reserve (n_props);

  std::sort (mp_work_edges->begin (), mp_work_edges->end (), edge_ymin_compare<db::Coord> ());

  y = edge_ymin ((*mp_work_edges) [0]);

  future = mp_work_edges->begin ();
  for (std::vector <WorkEdge>::iterator current = mp_work_edges->begin (); current != mp_work_edges->end () && ! gs.can_stop (); ) {

    if (m_report_progress) {
      double p = double (std::distance (mp_work_edges->begin (), current)) / double (mp_work_edges->size ());
      progress->set (size_t (double (todo_max - todo_next) * p) + todo_next);
    }

    std::vector <WorkEdge>::iterator f0 = future;
    while (future != mp_work_edges->end () && edge_ymin (*future) <= y) {
      tl_assert (future->data == 0); // HINT: for development
      ++future;
    }
    std::sort (f0, future, EdgeXAtYCompare2 (y));

    db::Coord yy = std::numeric_limits <db::Coord>::max ();
    if (future != mp_work_edges->end ()) {
      yy = edge_ymin (*future);
    }
    for (std::vector <WorkEdge>::const_iterator c = current; c != future; ++c) {
      if (edge_ymax (*c) > y) {
        yy = std::min (yy, edge_ymax (*c));
      }
    }

    db::Coord ysl = y;
    gs.begin_scanline (y);

    tl_assert (gs.is_reset ()); // HINT: for development

    if (current != future) {

      std::inplace_merge (current, f0, future, EdgeXAtYCompare2 (y));
#ifdef DEBUG_EDGE_PROCESSOR
      printf ("y=%d ", y);
      for (std::vector <WorkEdge>::iterator c = current; c != future; ++c) { 
        printf ("%ld-", long (c->data)); 
      } 
      printf ("\n");
#endif

      for (std::vector <WorkEdge>::iterator c = current; c != future; ) {

        const EdgeProcessorStates::SkipInfo &skip_info = gs.skip_info (c->data);
#ifdef DEBUG_EDGE_PROCESSOR
        printf ("X %ld->%d\n", long (c->data), int (skip_info.skip));
#endif

        if (skip_info.skip != 0 && (c + skip_info.skip >= future || (c + skip_info.skip)->data != 0)) {

          tl_assert (c + skip_info.skip <= future);

          gs.skip_n (skip_info);

          //  skip this interval - has not changed
          c += skip_info.skip;

        } else {

          std::vector <WorkEdge>::iterator c0 = c;
          gs.begin_skip_interval ();

          do {

            gs.reset_skip_entry (c->data);

            std::vector <WorkEdge>::iterator f = c + 1;

            //  HINT: "volatile" forces x and xx into memory and disables FPU register optimisation.
            //  That way, we can exactly compare doubles afterwards.
            volatile double x = edge_xaty (*c, y);

            while (f != future) {
              volatile double xx = edge_xaty (*f, y);
              if (xx != x) {
                break;
              }
              gs.reset_skip_entry (f->data);
              ++f;
            }

            //  compute edges that occur at this vertex
            
            gs.next_vertex (x);
            
            //  treat all edges crossing the scanline in a certain point
            for (std::vector <WorkEdge>::iterator cc = c; cc != f; ) {

              gs.next_coincident ();

              std::vector <WorkEdge>::iterator e = mp_work_edges->end ();

              std::vector <WorkEdge>::iterator cc0 = cc;

              std::vector <WorkEdge>::iterator fc = cc;
              do {
                ++fc;
              } while (fc != f && EdgeXAtYCompare2 (y).equal (*fc, *cc));

              //  sort the coincident edges by property ID - that will
              //  simplify algorithms like "inside" and "outside".
              if (fc - cc > 1) {
                //  for prefer_touch we first deliver the opening edges in ascending
                //  order, in the other case we the other way round so that the opening
                //  edges are always delivered with ascending property ID order.
                if (prefer_touch) {
                  std::sort (cc, fc, EdgePropCompare ());
                } else {
                  std::sort (cc, fc, EdgePropCompareReverse ());
                }
              }

              //  treat all coincident edges
              do {

                if (cc->dy () != 0) {

                  if (e == mp_work_edges->end () && edge_ymax (*cc) > y) {
                    e = cc;
                  }
                  
                  if ((cc->dy () > 0) == prefer_touch) {
                    if (edge_ymax (*cc) > y) {
                      gs.north_edge (prefer_touch, cc->prop);
                    }
                    if (edge_ymin (*cc) < y) {
                      gs.south_edge (prefer_touch, cc->prop);
                    }
                  }

                }

                ++cc;

              } while (cc != fc);

              //  Give the edge selection operator a chance to select edges now
              if (selects_edges) {
                for (std::vector <WorkEdge>::iterator sc = cc0; sc != fc; ++sc) {
                  if (edge_ymin (*sc) == y) {
                    gs.select_edge (*sc);
                  }
                }
              }

              //  report the closing or opening edges in the opposite order 
              //  than the other ones (see previous loop). Hence we have some
              //  symmetry of events which simplify implementation of the 
              //  InteractionDetector for example.
              do {

                --fc;

                if (fc->dy () != 0 && (fc->dy () > 0) != prefer_touch) {
                  if (edge_ymax (*fc) > y) {
                    gs.north_edge (! prefer_touch, fc->prop);
                  }
                  if (edge_ymin (*fc) < y) {
                    gs.south_edge (! prefer_touch, fc->prop);
                  }
                }

              } while (fc != cc0);

              gs.end_coincident ();

              if (e != mp_work_edges->end ()) {
                gs.push_edge (*e);
              }

            }

            gs.end_vertex ();

            c = f;

          } while (c != future && ! gs.is_reset ());

          //  TODO: assert that there is no overflow here:
          c0->data = gs.end_skip_interval (std::distance (c0, c));

        }

      }

      y = yy;

#ifdef DEBUG_EDGE_PROCESSOR
      for (std::vector <WorkEdge>::iterator c = current; c != future; ++c) {
        printf ("%ld-", long (c->data)); 
      } 
      printf ("\n");
#endif
      std::vector <WorkEdge>::iterator c0 = current;
      current = future;

      bool valid = true;

      for (std::vector <WorkEdge>::iterator c = future; c != c0; ) {

        --c;

        size_t data = c->data;
        c->data = 0;

        db::Coord ymax = edge_ymax (*c);
        if (ymax >= y) {
          --current;
          if (current != c) {
            std::swap (*current, *c);
          }
        }
        if (ymax <= y) {
          //  an edge ends now. The interval is not valid, i.e. cannot be skipped easily.
          valid = false;
        }

        if (data != 0 && current != future) {
          if (valid) {
            current->data = data;
            data = 0;
          } else {
            current->data = 0;
          }
          valid = true;
        }

        if (data) {
          gs.release_skip_entry (data);
        }

      }
#ifdef DEBUG_EDGE_PROCESSOR
      for (std::vector <WorkEdge>::iterator c = current; c != future; ++c) {
        printf ("%ld-", long (c->data)); 
      } 
      printf ("\n");
#endif
    
    }

    tl_assert (gs.is_reset ()); // HINT: for development (second)

    gs.end_scanline (ysl);

  }

  gs.flush ();

}

void
EdgeProcessor::simple_merge (const std::vector<db::Edge> &in, std::vector <db::Edge> &edges, int mode)
{
  clear ();
  reserve (in.size ());
  insert_sequence (in.begin (), in.end ());

  db::SimpleMerge op (mode);
  db::EdgeContainer out (edges);
  process (out, op);
}

void
EdgeProcessor::simple_merge (const std::vector<db::Edge> &in, std::vector <db::Polygon> &polygons, bool resolve_holes, bool min_coherence, int mode)
{
  clear ();
  reserve (in.size ());
  insert_sequence (in.begin (), in.end ());

  db::SimpleMerge op (mode);
  db::PolygonContainer pc (polygons);
  db::PolygonGenerator out (pc, resolve_holes, min_coherence);
  process (out, op);
}

void
EdgeProcessor::simple_merge (const std::vector<db::Polygon> &in, std::vector <db::Edge> &edges, int mode)
{
  clear ();
  reserve (count_edges (in));
  for (std::vector<db::Polygon>::const_iterator q = in.begin (); q != in.end (); ++q) {
    insert (*q);
  }

  db::SimpleMerge op (mode);
  db::EdgeContainer out (edges);
  process (out, op);
}

void
EdgeProcessor::simple_merge (const std::vector<db::Polygon> &in, std::vector <db::Polygon> &out, bool resolve_holes, bool min_coherence, int mode)
{
  clear ();
  reserve (count_edges (in));

  if (&in == &out) {
    while (! out.empty ()) {
      insert (out.back ());
      out.pop_back ();
    }
  } else {
    for (std::vector<db::Polygon>::const_iterator q = in.begin (); q != in.end (); ++q) {
      insert (*q);
    }
  }

  db::SimpleMerge op (mode);
  db::PolygonContainer pc (out);
  db::PolygonGenerator pg (pc, resolve_holes, min_coherence);
  process (pg, op);
}

void
EdgeProcessor::merge (const std::vector<db::Polygon> &in, std::vector <db::Edge> &edges, unsigned int min_wc)
{
  clear ();
  reserve (count_edges (in));

  size_t n = 0;
  for (std::vector<db::Polygon>::const_iterator q = in.begin (); q != in.end (); ++q, ++n) {
    insert (*q, n);
  }

  db::MergeOp op (min_wc);
  db::EdgeContainer out (edges);
  process (out, op);
}

void
EdgeProcessor::merge (const std::vector<db::Polygon> &in, std::vector <db::Polygon> &out, unsigned int min_wc, bool resolve_holes, bool min_coherence)
{
  clear ();
  reserve (count_edges (in));

  if (&in == &out) {
    size_t n = 0;
    while (! out.empty ()) {
      insert (out.back (), n);
      out.pop_back ();
      ++n;
    }
  } else {
    size_t n = 0;
    for (std::vector<db::Polygon>::const_iterator q = in.begin (); q != in.end (); ++q, ++n) {
      insert (*q, n);
    }
  }

  db::MergeOp op (min_wc);
  db::PolygonContainer pc (out);
  db::PolygonGenerator pg (pc, resolve_holes, min_coherence);
  process (pg, op);
}

void
EdgeProcessor::size (const std::vector<db::Polygon> &in, db::Coord dx, db::Coord dy, std::vector <db::Edge> &out, unsigned int mode)
{
  clear ();
  reserve (count_edges (in));

  size_t n = 0;
  for (std::vector<db::Polygon>::const_iterator q = in.begin (); q != in.end (); ++q, n += 2) {
    insert (*q, n);
  }

  //  Merge the polygons and feed them into the sizing filter
  db::EdgeContainer ec (out);
  db::SizingPolygonFilter siz (ec, dx, dy, mode);
  db::PolygonGenerator pg (siz, false /*don't resolve holes*/, false /*min. coherence*/);
  db::BooleanOp op (db::BooleanOp::Or);
  process (pg, op);
}

void
EdgeProcessor::size (const std::vector<db::Polygon> &in, db::Coord dx, db::Coord dy, std::vector <db::Polygon> &out, unsigned int mode, bool resolve_holes, bool min_coherence)
{
  clear ();
  reserve (count_edges (in));

  if (&in == &out) {
    size_t n = 0;
    while (! out.empty ()) {
      insert (out.back (), n);
      out.pop_back ();
      n += 2;
    }
  } else {
    size_t n = 0;
    for (std::vector<db::Polygon>::const_iterator q = in.begin (); q != in.end (); ++q, n += 2) {
      insert (*q, n);
    }
  }

  //  Merge the polygons and feed them into the sizing filter
#if ! defined(DEBUG_SIZE_INTERMEDIATE)
  db::PolygonContainer pc (out);
  db::PolygonGenerator pg2 (pc, resolve_holes, min_coherence);
  db::SizingPolygonFilter siz (pg2, dx, dy, mode);
  db::PolygonGenerator pg (siz, false /*don't resolve holes*/, false /*min. coherence*/);
  db::BooleanOp op (db::BooleanOp::Or);
  process (pg, op);
#else
  //  Intermediate output for debugging 
  db::PolygonContainer pc (out);
  db::PolygonGenerator pg2 (pc, false, false);
  db::BooleanOp op (db::BooleanOp::Or);
  process (pg2, op);
  for (std::vector <db::Polygon>::iterator p = out.begin (); p != out.end (); ++p) {
    *p = p->sized (dx, dy, mode);
  }
#endif
}

void 
EdgeProcessor::boolean (const std::vector<db::Polygon> &a, const std::vector<db::Polygon> &b, std::vector <db::Edge> &out, int mode)
{
  clear ();
  reserve (count_edges (a) + count_edges (b));

  size_t n;
  
  n = 0;
  for (std::vector<db::Polygon>::const_iterator q = a.begin (); q != a.end (); ++q, n += 2) {
    insert (*q, n);
  }

  n = 1;
  for (std::vector<db::Polygon>::const_iterator q = b.begin (); q != b.end (); ++q, n += 2) {
    insert (*q, n);
  }

  db::BooleanOp op ((db::BooleanOp::BoolOp) mode);
  db::EdgeContainer ec (out);
  process (ec, op);
}

void 
EdgeProcessor::boolean (const std::vector<db::Polygon> &a, const std::vector<db::Polygon> &b, std::vector <db::Polygon> &out, int mode, bool resolve_holes, bool min_coherence)
{
  clear ();
  reserve (count_edges (a) + count_edges (b));

  size_t n;
  
  n = 0;
  if (&a == &out && &b != &out) {
    while (! out.empty ()) {
      insert (out.back (), n);
      out.pop_back ();
      n += 2;
    }
  } else {
    for (std::vector<db::Polygon>::const_iterator q = a.begin (); q != a.end (); ++q, n += 2) {
      insert (*q, n);
    }
  }

  n = 1;
  if (&b == &out) {
    while (! out.empty ()) {
      insert (out.back (), n);
      out.pop_back ();
      n += 2;
    }
  } else {
    for (std::vector<db::Polygon>::const_iterator q = b.begin (); q != b.end (); ++q, n += 2) {
      insert (*q, n);
    }
  }

  db::BooleanOp op ((db::BooleanOp::BoolOp) mode);
  db::PolygonContainer pc (out);
  db::PolygonGenerator pg (pc, resolve_holes, min_coherence);
  process (pg, op);
}

void 
EdgeProcessor::boolean (const std::vector<db::Edge> &a, const std::vector<db::Edge> &b, std::vector <db::Edge> &out, int mode)
{
  clear ();
  reserve (a.size () + b.size ());

  insert_sequence (a.begin (), a.end (), 0);
  insert_sequence (b.begin (), b.end (), 1);

  db::BooleanOp op ((db::BooleanOp::BoolOp) mode);
  db::EdgeContainer ec (out);
  process (ec, op);
}

void 
EdgeProcessor::boolean (const std::vector<db::Edge> &a, const std::vector<db::Edge> &b, std::vector <db::Polygon> &out, int mode, bool resolve_holes, bool min_coherence)
{
  clear ();
  reserve (a.size () + b.size ());

  insert_sequence (a.begin (), a.end (), 0);
  insert_sequence (b.begin (), b.end (), 1);

  db::BooleanOp op ((db::BooleanOp::BoolOp) mode);
  db::PolygonContainer pc (out);
  db::PolygonGenerator pg (pc, resolve_holes, min_coherence);
  process (pg, op);
}

} // namespace db

