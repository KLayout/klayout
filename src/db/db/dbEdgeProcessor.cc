
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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
 *  @brief A compare operator for edged
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

EdgePolygonOp::EdgePolygonOp (bool outside, bool include_touching, int polygon_mode) 
  : m_outside (outside), m_include_touching (include_touching),
    m_function (polygon_mode),
    m_wcp_n (0), m_wcp_s (0)
{
}

void EdgePolygonOp::reset () 
{ 
  m_wcp_n = m_wcp_s = 0;
}

bool EdgePolygonOp::select_edge (bool horizontal, property_type p) 
{
  if (p == 0) {

    return false;

  } else if (horizontal) {

    bool res;
    if (m_include_touching) {
      res = (m_function (m_wcp_n) || m_function (m_wcp_s));
    } else {
      res = (m_function (m_wcp_n) && m_function (m_wcp_s));
    }

    return m_outside ? !res : res;

  } else {

    return m_outside ? !m_function (m_wcp_n) : m_function (m_wcp_n);

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

InteractionDetector::InteractionDetector (int mode, property_type container_id)
  : m_mode (mode), m_include_touching (true), m_container_id (container_id)
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

  //  In "interacting" mode we need to handle both north and south events because
  //  we have to catch interactions between objects north and south to the scanline
  if (north || (m_mode == 0 && m_include_touching)) {

    std::set <property_type> *inside = north ? &m_inside_n : &m_inside_s;

    if (inside_after < inside_before) {

      inside->erase (p);

      if (m_mode != 0) {

        //  the container objects are delivered last of all coincident edges
        //  (due to prefer_touch == true and the sorting of coincident edges by property id)
        //  hence every remaining parts count as non-interacting (outside)
        if (p == m_container_id) {
          for (std::set <property_type>::const_iterator i = inside->begin (); i != inside->end (); ++i) {
            if (*i != m_container_id) {
              m_non_interactions.insert (*i);
            }
          }
        }

      }

    } else if (inside_after > inside_before) {

      if (m_mode != 0) {

        //  in inside/outside mode we are only interested in interactions with the container_id property
        if (p != m_container_id) {
          //  note that the container parts will be delivered first of all coincident 
          //  edges hence we can check whether the container is present even for coincident
          //  edges
          if (inside->find (m_container_id) != inside->end ()) {
            m_interactions.insert (std::make_pair (m_container_id, p));
          } else {
            m_non_interactions.insert (p);
          }
        } else {
          for (std::set <property_type>::const_iterator i = inside->begin (); i != inside->end (); ++i) {
            if (*i != m_container_id) {
              m_interactions.insert (std::make_pair (m_container_id, *i));
            }
          }
        }

      } else {

        for (std::set <property_type>::const_iterator i = m_inside_n.begin (); i != m_inside_n.end (); ++i) {
          if (*i < p) {
            m_interactions.insert (std::make_pair (*i, p));
          } else if (*i > p) {
            m_interactions.insert (std::make_pair (p, *i));
          }
        }

        for (std::set <property_type>::const_iterator i = m_inside_s.begin (); i != m_inside_s.end (); ++i) {
          if (*i < p) {
            m_interactions.insert (std::make_pair (*i, p));
          } else if (*i > p) {
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
  if (m_mode < 0) {

    //  In inside mode remove those objects which have a non-interaction with the container_id property
    for (std::set<property_type>::const_iterator p = m_non_interactions.begin (); p != m_non_interactions.end (); ++p) {
      m_interactions.erase (std::make_pair (m_container_id, *p));
    }

    m_non_interactions.clear ();

  } else if (m_mode > 0) {

    //  In outside mode leave those objects which don't participate in an interaction
    for (iterator pp = begin (); pp != end (); ++pp) {
      m_non_interactions.erase (pp->second);
    }

    m_interactions.clear ();
    for (std::set<property_type>::const_iterator p = m_non_interactions.begin (); p != m_non_interactions.end (); ++p) {
      m_interactions.insert (m_interactions.end (), std::make_pair (m_container_id, *p));
    }

    m_non_interactions.clear ();

  }

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
EdgeProcessor::insert (const db::Polygon &q, EdgeProcessor::property_type p)
{
  for (db::Polygon::polygon_edge_iterator e = q.begin_edge (); ! e.at_end (); ++e) {
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
add_hparallel_cutpoints (WorkEdge &e1, WorkEdge &e2, std::vector <CutPoints> &cutpoints)
{
  db::Coord e1_xmin = std::min (e1.x1 (), e1.x2 ());
  db::Coord e1_xmax = std::max (e1.x1 (), e1.x2 ());
  if (e2.x1 () > e1_xmin && e2.x1 () < e1_xmax) {
    e1.make_cutpoints (cutpoints)->add (e2.p1 (), &cutpoints, false);
  }
  if (e2.x2 () > e1_xmin && e2.x2 () < e1_xmax) {
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

    while (f != future && edge_xmin (*f) <= x) {
      ++f;
    }

    db::Coord xx = std::numeric_limits <db::Coord>::max ();
    if (f != future) {
      xx = edge_xmin (*f);
    }

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
                  add_hparallel_cutpoints (*c1, *c2, cutpoints);
                  add_hparallel_cutpoints (*c2, *c1, cutpoints);
                }

              } else if (c1->p1 () != c2->p1 () && c1->p2 () != c2->p1 () &&
                         c1->p1 () != c2->p2 () && c1->p2 () != c2->p2 ()) {

                std::pair <bool, db::Point> cp = c1->intersect_point (*c2);
                if (cp.first) {

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
              if (cp.first) {
                
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
 *  This function is intended for use in scanline scenarious to determine what edges are 
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
  std::vector <WorkEdge *> p1_weak; // holds weak interactions of edge endpoints with other edges
  std::vector <WorkEdge *> ip_weak;
  double dy = y - 0.5;
  double dyy = yy + 0.5;

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

    while (f != future && edge_xmin_at_yinterval_double (*f, dy, dyy) <= x) {
      ++f;
    }

    db::Coord xx = std::numeric_limits <db::Coord>::max ();
    if (f != future) {
      xx = edge_xmin_at_yinterval_double (*f, dy, dyy);
    }

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

        p1_weak.clear (); 

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
                  add_hparallel_cutpoints (*c1, *c2, cutpoints);
                  add_hparallel_cutpoints (*c2, *c1, cutpoints);
                }

              } else if (c1->p1 () != c2->p1 () && c1->p2 () != c2->p1 () &&
                         c1->p1 () != c2->p2 () && c1->p2 () != c2->p2 ()) {

                std::pair <bool, db::Point> cp = safe_intersect_point (*c1, *c2);
                if (cp.first) {

                  bool on_edge1 = is_point_on_exact (*c1, cp.second);

                  //  add a cut point to c1 and c2 (points not on the edge give strong attractors)
                  c1->make_cutpoints (cutpoints)->add (cp.second, &cutpoints, !on_edge1);
                  if (with_h) {
                    c2->make_cutpoints (cutpoints)->add (cp.second, &cutpoints, false);
                  }

#ifdef DEBUG_EDGE_PROCESSOR
                  if (on_edge1) {
                    printf ("weak intersection point %s between %s and %s.\n", cp.second.to_string ().c_str (), c1->to_string ().c_str (), c2->to_string ().c_str ());
                  } else {
                    printf ("intersection point %s between %s and %s.\n", cp.second.to_string ().c_str (), c1->to_string ().c_str (), c2->to_string ().c_str ());
                  }
#endif

                  //  The new cutpoint must be inserted into other edges as well.
                  ip_weak.clear ();
                  for (std::vector <WorkEdge>::iterator cc = c; cc != f; ++cc) {
                    if ((with_h || cc->dy () != 0) && cc != c1 && cc != c2 && is_point_on_fuzzy (*cc, cp.second)) {
                      ip_weak.push_back (&*cc);
                    }
                  }
                  for (std::vector <WorkEdge *>::iterator icc = ip_weak.begin (); icc != ip_weak.end (); ++icc) {
                    (*icc)->make_cutpoints (cutpoints)->add (cp.second, &cutpoints, true);
#ifdef DEBUG_EDGE_PROCESSOR
                    printf ("intersection point %s gives cutpoint in %s.\n", cp.second.to_string ().c_str (), (*icc)->to_string ().c_str ());
#endif
                  }

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
              if (cp.first) {
                
                bool on_edge1 = true;
                bool on_edge2 = is_point_on_exact (*c2, cp.second);

                //  add a cut point to c1 and c2
                if (with_h || c1->dy () != 0) {
                  on_edge1 = is_point_on_exact (*c1, cp.second);
                  c1->make_cutpoints (cutpoints)->add (cp.second, &cutpoints, !on_edge1);
                }

                c2->make_cutpoints (cutpoints)->add (cp.second, &cutpoints, !on_edge2);

#ifdef DEBUG_EDGE_PROCESSOR
                if (!on_edge1 || !on_edge2) {
                  printf ("intersection point %s between %s and %s.\n", cp.second.to_string ().c_str (), c1->to_string ().c_str (), c2->to_string ().c_str ()); 
                } else {
                  printf ("weak intersection point %s between %s and %s.\n", cp.second.to_string ().c_str (), c1->to_string ().c_str (), c2->to_string ().c_str ()); 
                }
#endif

                //  The new cutpoint must be inserted into other edges as well.
                ip_weak.clear ();
                for (std::vector <WorkEdge>::iterator cc = c; cc != f; ++cc) {
                  if ((with_h || cc->dy () != 0) && cc != c1 && cc != c2 && is_point_on_fuzzy (*cc, cp.second)) {
                    ip_weak.push_back (&*cc);
                  }
                }
                for (std::vector <WorkEdge *>::iterator icc = ip_weak.begin (); icc != ip_weak.end (); ++icc) {
                  (*icc)->make_cutpoints (cutpoints)->add (cp.second, &cutpoints, true);
#ifdef DEBUG_EDGE_PROCESSOR
                  printf ("intersection point %s gives cutpoint in %s.\n", cp.second.to_string ().c_str (), (*icc)->to_string ().c_str ());
#endif
                }

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
                p1_weak.push_back (&*c2);
              }
            }

          }

        }

        if (! p1_weak.empty ()) {

          bool strong = false;
          for (std::vector<WorkEdge *>::const_iterator cp = p1_weak.begin (); cp != p1_weak.end () && ! strong; ++cp) {
            if ((*cp)->data > 0 && cutpoints [(*cp)->data - 1].strong_cutpoints) {
              strong = true;
            }
          }

          p1_weak.back ()->make_cutpoints (cutpoints);
          size_t n = p1_weak.back ()->data - 1;
          for (std::vector<WorkEdge *>::const_iterator cp = p1_weak.begin (); cp != p1_weak.end (); ++cp) {

            (*cp)->make_cutpoints (cutpoints);
            size_t nn = (*cp)->data - 1;
            if (strong) {
              cutpoints [nn].add (c1->p1 (), &cutpoints);
#ifdef DEBUG_EDGE_PROCESSOR
              printf ("Insert strong attractor %s in %s.\n", c1->p1 ().to_string ().c_str (), (*cp)->to_string ().c_str ()); 
#endif
            } else {
              cutpoints [nn].add_attractor (c1->p1 (), n);
#ifdef DEBUG_EDGE_PROCESSOR
              printf ("Insert weak attractor %s in %s.\n", c1->p1 ().to_string ().c_str (), (*cp)->to_string ().c_str ()); 
#endif
            }

            n = nn;

          }

        }

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
  tl::SelfTimer timer (tl::verbosity () >= m_base_verbosity, "EdgeProcessor: process");

  bool prefer_touch = op.prefer_touch (); 
  bool selects_edges = op.selects_edges (); 
  
  db::Coord y;
  std::vector <WorkEdge>::iterator future;

  //  step 1: preparation

  if (mp_work_edges->empty ()) {
    es.start ();
    es.flush ();
    return;
  }

  mp_cpvector->clear ();

  property_type n_props = 0;
  for (std::vector <WorkEdge>::iterator e = mp_work_edges->begin (); e != mp_work_edges->end (); ++e) {
    if (e->prop > n_props) {
      n_props = e->prop;
    }
  }
  ++n_props;

  size_t todo_max = 1000000;

  std::auto_ptr<tl::AbsoluteProgress> progress (0);
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


  //  step 2: find intersections
  std::sort (mp_work_edges->begin (), mp_work_edges->end (), edge_ymin_compare<db::Coord> ());

  y = edge_ymin ((*mp_work_edges) [0]);
  future = mp_work_edges->begin ();

  for (std::vector <WorkEdge>::iterator current = mp_work_edges->begin (); current != mp_work_edges->end (); ) {

    if (m_report_progress) {
      double p = double (std::distance (mp_work_edges->begin (), current)) / double (mp_work_edges->size ());
      progress->set (size_t (double (todo_next - todo) * p) + todo);
    }

    size_t n = std::distance (current, future);
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

    } while (future != mp_work_edges->end () && std::distance (current, future) < long (n + n / 2));

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


  tl::SelfTimer timer2 (tl::verbosity () >= m_base_verbosity + 10, "EdgeProcessor: production");

  //  step 4: compute the result edges 
  
  es.start (); // call this as late as possible. This way, input containers can be identical with output containers ("clear" is done after the input is read)

  op.reset ();
  op.reserve (n_props);

  std::sort (mp_work_edges->begin (), mp_work_edges->end (), edge_ymin_compare<db::Coord> ());

  y = edge_ymin ((*mp_work_edges) [0]);
  size_t skip_unit = 1;

  future = mp_work_edges->begin ();
  for (std::vector <WorkEdge>::iterator current = mp_work_edges->begin (); current != mp_work_edges->end (); ) {

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
    es.begin_scanline (y);

    tl_assert (op.is_reset ()); // HINT: for development

    if (current != future) {

      std::inplace_merge (current, f0, future, EdgeXAtYCompare2 (y));
#ifdef DEBUG_EDGE_PROCESSOR
      printf ("y=%d ", y);
      for (std::vector <WorkEdge>::iterator c = current; c != future; ++c) { 
        printf ("%ld-", long (c->data)); 
      } 
      printf ("\n");
#endif

      db::Coord hx = 0;
      int ho = 0;

      size_t new_skip_unit = std::distance (current, future);

      for (std::vector <WorkEdge>::iterator c = current; c != future; ) {

        size_t skip = c->data % skip_unit;
        size_t skip_res = c->data / skip_unit;
#ifdef DEBUG_EDGE_PROCESSOR
        printf ("X %ld->%d,%d\n", long (c->data), int (skip), int (skip_res));
#endif

        if (skip != 0 && (c + skip >= future || (c + skip)->data != 0)) {

          tl_assert (c + skip <= future);

          es.skip_n (skip_res);

          c->data = skip + new_skip_unit * skip_res;

          //  skip this interval - has not changed
          c += skip;

        } else {

          std::vector <WorkEdge>::iterator c0 = c;
          size_t n_res = 0;

          do {

            c->data = 0;
            std::vector <WorkEdge>::iterator f = c + 1;

            //  HINT: "volatile" forces x and xx into memory and disables FPU register optimisation.
            //  That way, we can exactly compare doubles afterwards.
            volatile double x = edge_xaty (*c, y);

            while (f != future) {
              volatile double xx = edge_xaty (*f, y);
              if (xx != x) {
                break;
              }
              f->data = 0;
              ++f;
            }

            //  compute edges that occure at this vertex
            
            bool vertex = false;
            
            //  treat all edges crossing the scanline in a certain point
            for (std::vector <WorkEdge>::iterator cc = c; cc != f; ) {

              std::vector <WorkEdge>::iterator e = mp_work_edges->end ();

              int pn = 0, ps = 0;

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
                      pn += op.edge (true, prefer_touch, cc->prop);
                    }
                    if (edge_ymin (*cc) < y) {
                      ps += op.edge (false, prefer_touch, cc->prop);
                    }
                  }

                }

                ++cc;

              } while (cc != fc);

              //  Give the edge selection operator a chance to select edges now
              if (selects_edges) {

                for (std::vector <WorkEdge>::iterator sc = cc0; sc != fc; ++sc) {
                  if (edge_ymin (*sc) == y && op.select_edge (sc->dy () == 0, sc->prop)) {
                    es.put (*sc);
#ifdef DEBUG_EDGE_PROCESSOR
                    printf ("put(%s)\n", sc->to_string().c_str());
#endif
                  }
                }

              }

              //  report the closing or opening edges in the opposite order 
              //  than the other ones (see previous loop). Hence we have some
              //  symmetry of events which simplify implementatin of the 
              //  InteractionDetector for example.
              do {

                --fc;

                if (fc->dy () != 0 && (fc->dy () > 0) != prefer_touch) {
                  if (edge_ymax (*fc) > y) {
                    pn += op.edge (true, ! prefer_touch, fc->prop);
                  }
                  if (edge_ymin (*fc) < y) {
                    ps += op.edge (false, ! prefer_touch, fc->prop);
                  }
                }

              } while (fc != cc0);

              if (! vertex && (ps != 0 || pn != 0)) {

                if (ho != 0) {
                  db::Edge he (db::Point (hx, y), db::Point (db::coord_traits<db::Coord>::rounded (x), y));
                  if (ho > 0) {
                    he.swap_points ();
                  }
                  es.put (he);
#ifdef DEBUG_EDGE_PROCESSOR
                  printf ("put(%s)\n", he.to_string().c_str());
#endif
                } 

                vertex = true;

              }

              if (e != mp_work_edges->end ()) {

                db::Edge edge (*e);

                if ((pn > 0 && edge.dy () < 0) || (pn < 0 && edge.dy () > 0)) {
                  edge.swap_points ();
                }

                if (pn != 0) {
                  ++n_res;
                  if (edge_ymin (edge) == y) {
                    es.put (edge);
#ifdef DEBUG_EDGE_PROCESSOR
                    printf ("put(%s)\n", edge.to_string().c_str());
#endif
                  } else {
                    es.crossing_edge (edge);
#ifdef DEBUG_EDGE_PROCESSOR
                    printf ("xing(%s)\n", edge.to_string().c_str());
#endif
                  }
                }

              }

            }

            if (vertex) {
              hx = db::coord_traits<db::Coord>::rounded (x);
              ho = op.compare_ns ();
            }

            c = f;

          } while (c != future && ! op.is_reset ());

          //  TODO: assert that there is no overflow here:
          c0->data = size_t (std::distance (c0, c) + new_skip_unit * n_res);

        }

      }

      skip_unit = new_skip_unit;

      y = yy;

#ifdef DEBUG_EDGE_PROCESSOR
      for (std::vector <WorkEdge>::iterator c = current; c != future; ++c) { 
        printf ("%ld-", long (c->data)); 
      } 
      printf ("\n");
#endif
      std::vector <WorkEdge>::iterator c0 = current;
      std::vector <WorkEdge>::iterator last_interval = future;
      current = future;

      bool valid = true;

      for (std::vector <WorkEdge>::iterator c = future; c != c0; ) {

        --c;

        bool start_interval = (c->data != 0);
        size_t data = c->data;
        c->data = 0;

        db::Coord ymax = edge_ymax (*c);
        if (ymax >= y) {
          --current;
          if (current != c) {
            *current = *c;
          }
        }
        if (ymax <= y) {
          //  an edge ends now. The interval is not valid, i.e. cannot be skipped easily.
          valid = false;
        }

        if (start_interval && current != future) {
          current->data = valid ? data : 0;
          last_interval = current;
          valid = true;
        }

      }
#ifdef DEBUG_EDGE_PROCESSOR
      for (std::vector <WorkEdge>::iterator c = current; c != future; ++c) { 
        printf ("%ld-", long (c->data)); 
      } 
      printf ("\n");
#endif
    
    }

    tl_assert (op.is_reset ()); // HINT: for development (second)

    es.end_scanline (ysl);

  }

  es.flush ();

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

