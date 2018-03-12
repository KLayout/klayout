
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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


#include "dbEdges.h"
#include "dbEdgePairs.h"
#include "dbRegion.h"
#include "dbLayoutUtils.h"
#include "dbBoxConvert.h"
#include "dbBoxScanner.h"
#include "dbPolygonTools.h"

#include "tlIntervalMap.h"
#include "tlVariant.h"

#include <sstream>

namespace db
{

Edges::Edges (const RecursiveShapeIterator &si, bool as_edges)
  : m_edges (false), m_merged_edges (false)
{
  init ();
  if (! as_edges) {
    m_iter = si;
  } else {
    for (RecursiveShapeIterator s = si; ! s.at_end (); ++s) {
      insert (s.shape (), s.trans ());
    }
  }
  m_bbox_valid = false;
  m_is_merged = false;
}

Edges::Edges (const RecursiveShapeIterator &si, const db::ICplxTrans &trans, bool as_edges, bool merged_semantics)
  : m_edges (false), m_merged_edges (false)
{
  init ();
  if (! as_edges) {
    m_iter = si;
    m_iter_trans = trans;
  } else {
    for (RecursiveShapeIterator s = si; ! s.at_end (); ++s) {
      insert (s.shape (), trans * s.trans ());
    }
  }
  m_bbox_valid = false;
  m_is_merged = false;
  m_merged_semantics = merged_semantics;
}

bool  
Edges::operator== (const db::Edges &other) const
{
  if (empty () != other.empty ()) {
    return false;
  }
  if (size () != other.size ()) {
    return false;
  }
  db::Edges::const_iterator o1 = begin ();
  db::Edges::const_iterator o2 = other.begin ();
  while (! o1.at_end () && ! o2.at_end ()) {
    if (*o1 != *o2) {
      return false;
    }
    ++o1;
    ++o2;
  }
  return true;
}

bool  
Edges::operator< (const db::Edges &other) const
{
  if (empty () != other.empty ()) {
    return empty () < other.empty ();
  }
  if (size () != other.size ()) {
    return (size () < other.size ());
  }
  db::Edges::const_iterator o1 = begin ();
  db::Edges::const_iterator o2 = other.begin ();
  while (! o1.at_end () && ! o2.at_end ()) {
    if (*o1 != *o2) {
      return *o1 < *o2;
    }
    ++o1;
    ++o2;
  }
  return false;
}

std::string 
Edges::to_string (size_t nmax) const
{
  std::ostringstream os;
  const_iterator e = begin ();
  bool first = true;
  for ( ; ! e.at_end () && nmax != 0; ++e, --nmax) {
    if (! first) {
      os << ";";
    }
    first = false;
    os << e->to_string ();
  }
  if (! e.at_end ()) {
    os << "...";
  }
  return os.str ();
}

void 
Edges::swap (Edges &other)
{
  std::swap (m_is_merged, other.m_is_merged);
  std::swap (m_merged_semantics, other.m_merged_semantics);
  m_edges.swap (other.m_edges);
  m_merged_edges.swap (other.m_merged_edges);
  std::swap (m_bbox, other.m_bbox);
  std::swap (m_bbox_valid, other.m_bbox_valid);
  std::swap (m_merged_edges_valid, other.m_merged_edges_valid);
  std::swap (m_iter, other.m_iter);
  std::swap (m_iter_trans, other.m_iter_trans);
}

void
Edges::invalidate_cache ()
{
  m_bbox_valid = false;
  m_merged_edges.clear ();
  m_merged_edges_valid = false;
}

void 
Edges::disable_progress ()
{
  m_report_progress = false;
}

void 
Edges::enable_progress (const std::string &progress_desc)
{
  m_report_progress = true;
  m_progress_desc = progress_desc;
}

Edges::distance_type 
Edges::length (const db::Box &box) const
{
  distance_type l = 0;

  for (const_iterator e = begin_merged (); ! e.at_end (); ++e) {

    if (box.empty () || (box.contains (e->p1 ()) && box.contains (e->p2 ()))) {
      l += e->length ();
    } else {

      std::pair<bool, db::Edge> ce = e->clipped (box);
      if (ce.first) {

        db::Coord dx = ce.second.dx ();
        db::Coord dy = ce.second.dy ();
        db::Coord x = ce.second.p1 ().x ();
        db::Coord y = ce.second.p1 ().y ();
        if ((dx == 0 && x == box.left ()   && dy < 0) ||
            (dx == 0 && x == box.right ()  && dy > 0) ||
            (dy == 0 && y == box.top ()    && dx < 0) ||
            (dy == 0 && y == box.bottom () && dx > 0)) {
          //  not counted -> box is at outside side of the edge
        } else {
          l += ce.second.length ();
        }

      }

    }

  }

  return l;
}

Edges 
Edges::start_segments (db::Edges::length_type length, double fraction) const
{
  Edges edges;
  edges.reserve (m_edges.size ());

  //  zero-length edges would vanish in merged sematics, so we don't set it now
  if (length == 0) {
    edges.set_merged_semantics (false);
  }

  for (const_iterator e = begin_merged (); ! e.at_end (); ++e) {
    double l = std::max (e->double_length () * fraction, double (length));
    edges.insert (db::Edge (e->p1 (), db::Point (db::DPoint (e->p1 ()) + db::DVector (e->d()) * (l / e->double_length ()))));
  }

  return edges;
}

Edges 
Edges::end_segments (db::Edges::length_type length, double fraction) const
{
  Edges edges;
  edges.reserve (m_edges.size ());

  //  zero-length edges would vanish in merged sematics, so we don't set it now
  if (length == 0) {
    edges.set_merged_semantics (false);
  }

  for (const_iterator e = begin_merged (); ! e.at_end (); ++e) {
    double l = std::max (e->double_length () * fraction, double (length));
    edges.insert (db::Edge (db::Point (db::DPoint (e->p2 ()) - db::DVector (e->d()) * (l / e->double_length ())), e->p2 ()));
  }

  return edges;
}

Edges 
Edges::centers (db::Edges::length_type length, double fraction) const
{
  Edges edges;
  edges.reserve (m_edges.size ());

  //  zero-length edges would vanish in merged sematics, so we don't set it now
  if (length == 0) {
    edges.set_merged_semantics (false);
  }

  for (const_iterator e = begin_merged (); ! e.at_end (); ++e) {
    double l = std::max (e->double_length () * fraction, double (length));
    db::DVector dl = db::DVector (e->d()) * (0.5 * l / e->double_length ());
    db::DPoint center = db::DPoint (e->p1 ()) + db::DVector (e->p2 () - e->p1 ()) * 0.5;
    edges.insert (db::Edge (db::Point (center - dl), db::Point (center + dl)));
  }

  return edges;
}

void 
Edges::edge_region_op (const Region &other, bool outside, bool include_borders)
{
  //  shortcuts
  if (other.empty ()) {
    if (! outside) {
      clear ();
    }
    return;
  } else if (empty ()) {
    return;
  }

  db::EdgeProcessor ep (m_report_progress, m_progress_desc);

  for (db::Region::const_iterator p = other.begin (); ! p.at_end (); ++p) {
    if (p->box ().touches (bbox ())) {
      ep.insert (*p, 0);
    }
  }

  for (const_iterator e = begin (); ! e.at_end (); ++e) {
    ep.insert (*e, 1);
  }

  invalidate_cache ();

  db::EdgeShapeGenerator cc (m_edges, true /*clear*/);
  db::EdgePolygonOp op (outside, include_borders);
  ep.process (cc, op);

  set_valid_edges ();

  m_is_merged = false;
}

namespace
{

struct OrJoinOp
{
  void operator() (int &v, int n)
  {
    v += n;
  }
};

struct AndJoinOp
{
  void operator() (int &v, int n)
  {
    if (n == 0) {
      v = 0;
    }
  }
};

struct NotJoinOp
{
  void operator() (int &v, int n)
  {
    if (n != 0) {
      v = 0;
    }
  }
};

struct XorJoinOp
{
  void operator() (int &v, int n)
  {
    if (n != 0) {
      if (v == 0) {
        v = (n > 0 ? 1 : -1);
      } else {
        v = 0;
      }
    }
  }
};

struct EdgeBooleanCluster 
  : public db::cluster<db::Edge, size_t>
{
  typedef db::Edge::coord_type coord_type;

  EdgeBooleanCluster (db::Edges *output, db::Edges::BoolOp op)
    : mp_output (output), m_op (op)
  {
    //  .. nothing yet ..
  }

  void finish ()
  {
    //  determine base edge (longest overall edge)

    //  shortcut for single edge
    if (begin () + 1 == end ()) {
      if (begin ()->second == 0) {
        if (m_op != db::Edges::And) {
          mp_output->insert (*(begin ()->first));
        }
      } else {
        if (m_op != db::Edges::And && m_op != db::Edges::Not) {
          mp_output->insert (*(begin ()->first));
        }
      }
      return;
    }

    db::Edge r = *begin ()->first;
    double l1 = 0.0, l2 = r.double_length ();
    double n = 1.0 / l2;
    db::Point p1 = r.p1 (), p2 = r.p2 ();

    for (iterator o = begin () + 1; o != end (); ++o) {
      double ll1 = db::sprod (db::Vector (o->first->p1 () - r.p1 ()), r.d ()) * n;
      double ll2 = db::sprod (db::Vector (o->first->p2 () - r.p1 ()), r.d ()) * n;
      if (ll1 < l1) {
        p1 = o->first->p1 ();
        l1 = ll1;
      }
      if (ll2 < l1) {
        p1 = o->first->p2 ();
        l1 = ll2;
      }
      if (ll1 > l2) {
        p2 = o->first->p1 ();
        l2 = ll1;
      }
      if (ll2 > l2) {
        p2 = o->first->p2 ();
        l2 = ll2;
      }
    }

    db::Vector d = db::Vector (p2 - p1);
    n = 1.0 / d.double_length ();

    OrJoinOp or_jop;
    AndJoinOp and_jop;
    NotJoinOp not_jop;
    XorJoinOp xor_jop;

    tl::interval_map<db::Coord, int> a, b;
    a.add (0, db::coord_traits<db::Coord>::rounded (d.double_length ()), 0, or_jop);
    b.add (0, db::coord_traits<db::Coord>::rounded (d.double_length ()), 0, or_jop);

    for (iterator o = begin (); o != end (); ++o) {
      db::Coord l1 = db::coord_traits<db::Coord>::rounded (db::sprod (db::Vector (o->first->p1 () - p1), d) * n);
      db::Coord l2 = db::coord_traits<db::Coord>::rounded (db::sprod (db::Vector (o->first->p2 () - p1), d) * n);
      if (o->second == 0 || m_op == db::Edges::Or) {
        if (l1 < l2) {
          a.add (l1, l2, 1, or_jop);
        } else if (l1 > l2) {
          a.add (l2, l1, -1, or_jop);
        }
      } else {
        if (l1 < l2) {
          b.add (l1, l2, 1, or_jop);
        } else {
          b.add (l2, l1, -1, or_jop);
        }
      }
    }

    tl::interval_map<db::Coord, int> q;
    for (tl::interval_map<db::Coord, int>::const_iterator ia = a.begin (); ia != a.end (); ++ia) {
      q.add (ia->first.first, ia->first.second, ia->second > 0 ? 1 : (ia->second < 0 ? -1 : 0), or_jop);
    }

    if (b.begin () == b.end ()) {

      //  optimize for empty b
      if (m_op != db::Edges::And) {
        for (tl::interval_map<db::Coord, int>::const_iterator ib = b.begin (); ib != b.end (); ++ib) {
          if (ib->second > 0) {
            mp_output->insert (db::Edge (p1 + db::Vector (d * (ib->first.first * n)), p1 + db::Vector (d * (ib->first.second * n))));
          } else if (ib->second < 0) {
            mp_output->insert (db::Edge (p1 + db::Vector (d * (ib->first.second * n)), p1 + db::Vector (d * (ib->first.first * n))));
          }
        }
      }

    } else {

      if (m_op == db::Edges::And) {
        for (tl::interval_map<db::Coord, int>::const_iterator ib = b.begin (); ib != b.end (); ++ib) {
          q.add (ib->first.first, ib->first.second, ib->second, and_jop);
        }
      } else if (m_op == db::Edges::Not) {
        for (tl::interval_map<db::Coord, int>::const_iterator ib = b.begin (); ib != b.end (); ++ib) {
          q.add (ib->first.first, ib->first.second, ib->second, not_jop);
        }
      } else if (m_op == db::Edges::Xor) {
        for (tl::interval_map<db::Coord, int>::const_iterator ib = b.begin (); ib != b.end (); ++ib) {
          q.add (ib->first.first, ib->first.second, ib->second, xor_jop);
        }
      }

      for (tl::interval_map<db::Coord, int>::const_iterator iq = q.begin (); iq != q.end (); ++iq) {
        if (iq->second > 0) {
          mp_output->insert (db::Edge (p1 + db::Vector (d * (iq->first.first * n)), p1 + db::Vector (d * (iq->first.second * n))));
        } else if (iq->second < 0) {
          mp_output->insert (db::Edge (p1 + db::Vector (d * (iq->first.second * n)), p1 + db::Vector (d * (iq->first.first * n))));
        }
      }

    }

  }

private:
  db::Edges *mp_output;
  db::Edges::BoolOp m_op;
};

struct EdgeBooleanClusterCollector
  : public db::cluster_collector<db::Edge, size_t, EdgeBooleanCluster>
{
  EdgeBooleanClusterCollector (db::Edges *output, Edges::BoolOp op)
    : db::cluster_collector<db::Edge, size_t, EdgeBooleanCluster> (EdgeBooleanCluster (output, op), op != Edges::And /*report single*/)
  {
    //  .. nothing yet ..
  }

  void add (const db::Edge *o1, size_t p1, const db::Edge *o2, size_t p2)
  {
    //  Select edges which are:
    //  1.) not degenerate
    //  2.) parallel with some tolerance of roughly 1 dbu
    //  3.) connected
    if (! o1->is_degenerate () && ! o2->is_degenerate () 
        && fabs ((double) db::vprod (*o1, *o2)) < db::coord_traits<db::Coord>::prec_distance () * std::min (o1->double_length (), o2->double_length ())
        && (o1->p1 () == o2->p1 () || o1->p1 () == o2->p2 () || o1->p2 () == o2->p1 () || o1->p2 () == o2->p2 () || o1->coincident (*o2))) {
      db::cluster_collector<db::Edge, size_t, EdgeBooleanCluster>::add (o1, p1, o2, p2);
    }
  }
};

}

void
Edges::inplace_boolean (const Edges *other, Edges::BoolOp op)
{
  Edges out = boolean (other, op);
  swap (out);
}

Edges
Edges::boolean (const Edges *other, Edges::BoolOp op) const
{
  Edges output;
  EdgeBooleanClusterCollector cluster_collector (&output, op);

  db::box_scanner<db::Edge, size_t> scanner (m_report_progress, m_progress_desc);
  scanner.reserve (m_edges.size () + (other ? other->size () : 0));

  ensure_valid_edges ();
  for (const_iterator e = begin (); ! e.at_end (); ++e) {
    if (! e->is_degenerate ()) {
      scanner.insert (&*e, 0); 
    }
  }
  if (other) {
    other->ensure_valid_edges ();
    for (const_iterator e = other->begin (); ! e.at_end (); ++e) {
      if (! e->is_degenerate ()) {
        scanner.insert (&*e, 1); 
      }
    }
  }

  scanner.process (cluster_collector, 1, db::box_convert<db::Edge> ());

  output.m_is_merged = true;
  return output;
}

void
Edges::ensure_valid_merged_edges () const
{
  //  If no merged semantics applies or we will deliver the original
  //  edges as merged ones, we need to make sure those are valid
  //  ones (with a unique memory address)
  if (! m_merged_semantics || m_is_merged) {
    ensure_valid_edges ();
  } else {
    ensure_merged_edges_valid ();
  }
}

void
Edges::ensure_merged_edges_valid () const
{
  if (! m_merged_edges_valid) {

    m_merged_edges.clear ();

    Edges tmp;
    EdgeBooleanClusterCollector cluster_collector (&tmp, Or);

    db::box_scanner<db::Edge, size_t> scanner (m_report_progress, m_progress_desc);
    scanner.reserve (m_edges.size ());

    ensure_valid_edges ();
    for (const_iterator e = begin (); ! e.at_end (); ++e) {
      if (! e->is_degenerate ()) {
        scanner.insert (&*e, 0); 
      }
    }

    scanner.process (cluster_collector, 1, db::box_convert<db::Edge> ());

    m_merged_edges.swap (tmp.m_edges);
    m_merged_edges_valid = true;

  }
}

Edges &
Edges::operator+= (const Edges &other)
{
  invalidate_cache ();

  if (! has_valid_edges ()) {

    m_edges.clear ();

    size_t n = 0;
    for (const_iterator e = begin (); ! e.at_end (); ++e) {
      ++n;
    }
    for (const_iterator e = other.begin (); ! e.at_end (); ++e) {
      ++n;
    }

    m_edges.reserve (db::Edge::tag (), n);

    for (const_iterator e = begin (); ! e.at_end (); ++e) {
      m_edges.insert (*e);
    }
    for (const_iterator e = other.begin (); ! e.at_end (); ++e) {
      m_edges.insert (*e);
    }

    set_valid_edges ();

  } else if (! other.has_valid_edges ()) {

    size_t n = m_edges.size ();
    for (const_iterator e = other.begin (); ! e.at_end (); ++e) {
      ++n;
    }

    m_edges.reserve (db::Edge::tag (), n);

    for (const_iterator e = other.begin (); ! e.at_end (); ++e) {
      m_edges.insert (*e);
    }

  } else {
    m_edges.insert (other.m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().begin (), other.m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().end ());
  }

  m_is_merged = false;
  return *this;
}

namespace
{

/**
 *  @brief A helper class for the edge to region interaction functionality which acts as an edge pair receiver
 *
 *  Note: This special scanner uses pointers to two different objects: edges and polygons. 
 *  It uses odd value pointers to indicate pointers to polygons and even value pointers to indicate 
 *  pointers to edges.
 *
 *  There is a special box converter which is able to sort that out as well.
 */
template <class OutputContainer>
class edge_to_region_interaction_filter
  : public db::box_scanner_receiver<char, size_t>
{
public:
  edge_to_region_interaction_filter (OutputContainer &output)
    : mp_output (&output)
  {
    //  .. nothing yet ..
  }
  
  void add (const char *o1, size_t p1, const char *o2, size_t p2)
  { 
    const db::Edge *e = 0;
    const db::Polygon *p = 0;

    //  Note: edges have property 0 and have even-valued pointers.
    //  Polygons have property 1 and odd-valued pointers.
    if (p1 == 0 && p2 == 1) {
      e = reinterpret_cast<const db::Edge *> (o1);
      p = reinterpret_cast<const db::Polygon *> (o2 - 1);
    } else if (p1 == 1 && p2 == 0) {
      e = reinterpret_cast<const db::Edge *> (o2);
      p = reinterpret_cast<const db::Polygon *> (o1 - 1);
    }

    if (e && p && m_seen.find (e) == m_seen.end ()) {
      if (db::interact (*p, *e)) {
        m_seen.insert (e);
        mp_output->insert (*e);
      }
    }
  }

private:
  OutputContainer *mp_output;
  std::set<const db::Edge *> m_seen;
};

/**
 *  @brief A special box converter that splits the pointers into polygon and edge pointers
 */
struct EdgeOrRegionBoxConverter
{
  typedef db::Box box_type;

  db::Box operator() (const char &c) const
  {
    //  Note: edges have property 0 and have even-valued pointers.
    //  Polygons have property 1 and odd-valued pointers.
    const char *cp = &c;
    if ((size_t (cp) & 1) == 1) {
      //  it's a polygon
      return (reinterpret_cast<const db::Polygon *> (cp - 1))->box ();
    } else {
      //  it's an edge
      const db::Edge *e = reinterpret_cast<const db::Edge *> (cp);
      return db::Box (e->p1 (), e->p2 ());
    }
  }
};

}

Edges &
Edges::select_interacting (const Region &other)
{
  //  shortcuts
  if (other.empty ()) {
    clear ();
    return *this;
  } else if (empty ()) {
    return *this;
  }

  db::box_scanner<char, size_t> scanner (m_report_progress, m_progress_desc);
  scanner.reserve (size () + other.size ());

  ensure_valid_merged_edges ();
  for (const_iterator e = begin_merged (); ! e.at_end (); ++e) {
    scanner.insert ((char *) &*e, 0);
  }

  other.ensure_valid_polygons ();
  for (Region::const_iterator p = other.begin (); ! p.at_end (); ++p) {
    scanner.insert ((char *) &*p + 1, 1);
  }

  Edges output;
  edge_to_region_interaction_filter<Edges> filter (output);
  EdgeOrRegionBoxConverter bc;
  scanner.process (filter, 1, bc);

  swap (output);
  return *this;
}

Edges &
Edges::select_not_interacting (const Region &other)
{
  //  shortcuts
  if (other.empty () || empty ()) {
    return *this;
  }

  db::box_scanner<char, size_t> scanner (m_report_progress, m_progress_desc);
  scanner.reserve (size () + other.size ());

  ensure_valid_merged_edges ();
  for (const_iterator e = begin_merged (); ! e.at_end (); ++e) {
    scanner.insert ((char *) &*e, 0);
  }

  other.ensure_valid_polygons ();
  for (Region::const_iterator p = other.begin (); ! p.at_end (); ++p) {
    scanner.insert ((char *) &*p + 1, 1);
  }

  std::set<db::Edge> interacting;
  edge_to_region_interaction_filter<std::set<db::Edge> > filter (interacting);
  EdgeOrRegionBoxConverter bc;
  scanner.process (filter, 1, bc);

  Edges output;
  for (const_iterator o = begin_merged (); ! o.at_end (); ++o) {
    if (interacting.find (*o) == interacting.end ()) {
      output.insert (*o);
    }
  }

  swap (output);
  return *this; 
}

namespace
{

/**
 *  @brief A helper class for the edge interaction functionality which acts as an edge pair receiver
 */
template <class OutputContainer>
class edge_interaction_filter
  : public db::box_scanner_receiver<db::Edge, size_t>
{
public:
  edge_interaction_filter (OutputContainer &output)
    : mp_output (&output)
  {
    //  .. nothing yet ..
  }
  
  void add (const db::Edge *o1, size_t p1, const db::Edge *o2, size_t p2)
  { 
    //  Select the edges which intersect
    if (p1 != p2) {
      const db::Edge *o = p1 > p2 ? o2 : o1;
      const db::Edge *oo = p1 > p2 ? o1 : o2;
      if (o->intersect (*oo)) {
        if (m_seen.insert (o).second) {
          mp_output->insert (*o);
        }
      }
    }
  }

private:
  OutputContainer *mp_output;
  std::set<const db::Edge *> m_seen;
};

}

Edges &
Edges::select_interacting (const Edges &other)
{
  db::box_scanner<db::Edge, size_t> scanner (m_report_progress, m_progress_desc);
  scanner.reserve (size () + other.size ());

  ensure_valid_merged_edges ();
  for (const_iterator e = begin_merged (); ! e.at_end (); ++e) {
    scanner.insert (&*e, 0);
  }
  other.ensure_valid_edges ();
  for (const_iterator e = other.begin (); ! e.at_end (); ++e) {
    scanner.insert (&*e, 1);
  }

  Edges output;
  edge_interaction_filter<Edges> filter (output);
  scanner.process (filter, 1, db::box_convert<db::Edge> ());

  swap (output);
  return *this;
}

Edges &
Edges::select_not_interacting (const Edges &other)
{
  db::box_scanner<db::Edge, size_t> scanner (m_report_progress, m_progress_desc);
  scanner.reserve (size () + other.size ());

  ensure_valid_merged_edges ();
  for (const_iterator e = begin_merged (); ! e.at_end (); ++e) {
    scanner.insert (&*e, 0);
  }
  other.ensure_valid_edges ();
  for (const_iterator e = other.begin (); ! e.at_end (); ++e) {
    scanner.insert (&*e, 1);
  }

  std::set<db::Edge> interacting;
  edge_interaction_filter<std::set<db::Edge> > filter (interacting);
  scanner.process (filter, 1, db::box_convert<db::Edge> ());

  Edges output;
  for (const_iterator o = begin_merged (); ! o.at_end (); ++o) {
    if (interacting.find (*o) == interacting.end ()) {
      output.insert (*o);
    }
  }

  swap (output);
  return *this; 
}

namespace
{

struct JoinEdgesCluster 
  : public db::cluster<db::Edge, size_t>
{
  typedef db::Edge::coord_type coord_type;

  JoinEdgesCluster (db::Region *output, coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i)
    : mp_output (output), m_ext_b (ext_b), m_ext_e (ext_e), m_ext_o (ext_o), m_ext_i (ext_i) 
  {
    //  .. nothing yet ..
  }

  void finish ()
  {
    std::multimap<db::Point, iterator> objects_by_p1;
    std::multimap<db::Point, iterator> objects_by_p2;
    for (iterator o = begin (); o != end (); ++o) {
      if (o->first->p1 () != o->first->p2 ()) {
        objects_by_p1.insert (std::make_pair (o->first->p1 (), o));
        objects_by_p2.insert (std::make_pair (o->first->p2 (), o));
      }
    }

    while (! objects_by_p2.empty ()) {

      tl_assert (! objects_by_p1.empty ());

      //  Find the beginning of a new sequence
      std::multimap<db::Point, iterator>::iterator j0 = objects_by_p1.begin ();
      std::multimap<db::Point, iterator>::iterator j = j0;
      do {
        std::multimap<db::Point, iterator>::iterator jj = objects_by_p2.find (j->first);
        if (jj == objects_by_p2.end ()) {
          break;
        } else {
          j = objects_by_p1.find (jj->second->first->p1 ());
          tl_assert (j != objects_by_p1.end ());
        }
      } while (j != j0);

      iterator i = j->second;

      //  determine a sequence 
      //  TODO: this chooses any solution in case of forks. Choose a specific one?
      std::vector<db::Point> pts;
      pts.push_back (i->first->p1 ());

      do {

        //  record the next point
        pts.push_back (i->first->p2 ());

        //  remove the edge as it's taken
        std::multimap<db::Point, iterator>::iterator jj;
        for (jj = objects_by_p2.find (i->first->p2 ()); jj != objects_by_p2.end () && jj->first == i->first->p2 (); ++jj) {
          if (jj->second == i) {
            break;
          }
        }
        tl_assert (jj != objects_by_p2.end () && jj->second == i);
        objects_by_p2.erase (jj);
        objects_by_p1.erase (j);

        //  process along the edge to the next one
        //  TODO: this chooses any solution in case of forks. Choose a specific one?
        j = objects_by_p1.find (i->first->p2 ());
        if (j != objects_by_p1.end ()) {
          i = j->second;
        } else {
          break;
        }

      } while (true);

      bool cyclic = (pts.back () == pts.front ());

      if (! cyclic) {

        //  non-cyclic sequence
        db::Path path (pts.begin (), pts.end (), 0, m_ext_b, m_ext_e, false);
        std::vector<db::Point> hull;
        path.hull (hull, m_ext_o, m_ext_i);
        db::Polygon poly;
        poly.assign_hull (hull.begin (), hull.end ());
        mp_output->insert (poly);

      } else {

        //  we have a loop: form a contour by using the polygon size functions and a "Not" to form the hole
        db::Polygon poly;
        poly.assign_hull (pts.begin (), pts.end ());

        db::EdgeProcessor ep;
        db::RegionPolygonSink ps (*mp_output, false);
        db::PolygonGenerator pg (ps, false, true);

        int mode_a = -1, mode_b = -1;

        if (m_ext_o == 0) {
          ep.insert (poly, 0);
        } else {
          db::Polygon sized_poly (poly);
          sized_poly.size (m_ext_o, m_ext_o, 2 /*sizing mode*/);
          ep.insert (sized_poly, 0);
          mode_a = 1;
        }

        if (m_ext_i == 0) {
          ep.insert (poly, 1);
        } else {
          db::Polygon sized_poly (poly);
          sized_poly.size (-m_ext_i, -m_ext_i, 2 /*sizing mode*/);
          ep.insert (sized_poly, 1);
          mode_b = 1;
        }

        db::BooleanOp2 op (db::BooleanOp::ANotB, mode_a, mode_b);
        ep.process (pg, op);

      }

    }
  }

private:
  db::Region *mp_output;
  coord_type m_ext_b, m_ext_e, m_ext_o, m_ext_i;
};

struct JoinEdgesClusterCollector
  : public db::cluster_collector<db::Edge, size_t, JoinEdgesCluster>
{
  typedef db::Edge::coord_type coord_type;

  JoinEdgesClusterCollector (db::Region *output, coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i)
    : db::cluster_collector<db::Edge, size_t, JoinEdgesCluster> (JoinEdgesCluster (output, ext_b, ext_e, ext_o, ext_i), true)
  {
    //  .. nothing yet ..
  }

  void add (const db::Edge *o1, size_t p1, const db::Edge *o2, size_t p2)
  {
    if (o1->p2 () == o2->p1 () || o1->p1 () == o2->p2 ()) {
      db::cluster_collector<db::Edge, size_t, JoinEdgesCluster>::add (o1, p1, o2, p2);
    }
  }
};

}

void 
Edges::extended (Region &output, coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i, bool join) const
{
  if (join) {

    JoinEdgesClusterCollector cluster_collector (&output, ext_b, ext_e, ext_o, ext_i);

    db::box_scanner<db::Edge, size_t> scanner (m_report_progress, m_progress_desc);
    scanner.reserve (size ());

    ensure_valid_edges ();
    size_t n = 0;
    for (const_iterator e = begin (); ! e.at_end (); ++e) {
      scanner.insert (&*e, n); 
      ++n;
    }

    scanner.process (cluster_collector, 1, db::box_convert<db::Edge> ());

  } else {

    for (const_iterator e = begin_merged (); ! e.at_end (); ++e) {

      db::DVector d;
      if (e->is_degenerate ()) {
        d = db::DVector (1.0, 0.0);
      } else {
        d = db::DVector (e->d ()) * (1.0 / e->double_length ());
      }

      db::DVector n (-d.y (), d.x ());

      db::Point pts[4] = {
        db::Point (db::DPoint (e->p1 ()) - d * double (ext_b) + n * double (ext_o)),
        db::Point (db::DPoint (e->p2 ()) + d * double (ext_e) + n * double (ext_o)),
        db::Point (db::DPoint (e->p2 ()) + d * double (ext_e) - n * double (ext_i)),
        db::Point (db::DPoint (e->p1 ()) - d * double (ext_b) - n * double (ext_i)),
      };
      
      db::Polygon poly;
      poly.assign_hull (pts + 0, pts + 4);
      output.insert (poly);

    }

  }
}

Edges 
Edges::in (const Edges &other, bool invert) const
{
  std::set <db::Edge> op;
  for (const_iterator o = other.begin_merged (); ! o.at_end (); ++o) {
    op.insert (*o);
  }

  Edges r;
  for (const_iterator o = begin_merged (); ! o.at_end (); ++o) {
    if ((op.find (*o) == op.end ()) == invert) {
      r.insert (*o);
    }
  }

  return r;
}

void 
Edges::init ()
{
  m_report_progress = false;
  m_bbox_valid = true;
  m_is_merged = true;
  m_merged_semantics = true;
  m_merged_edges_valid = false;
}

void 
Edges::ensure_bbox_valid () const
{
  if (! m_bbox_valid) {
    m_bbox = db::Box ();
    for (const_iterator e = begin (); ! e.at_end (); ++e) {
      m_bbox += db::Box (e->p1 (), e->p2 ());
    }
    m_bbox_valid = true;
  }
}

Edges::const_iterator 
Edges::begin_merged () const
{
  if (! m_merged_semantics || m_is_merged) {
    return begin ();
  } else {
    ensure_merged_edges_valid ();
    return db::EdgesIterator (m_merged_edges.get_layer<db::Edge, db::unstable_layer_tag> ().begin (), m_merged_edges.get_layer<db::Edge, db::unstable_layer_tag> ().end ());
  }
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans>
Edges::begin_iter () const
{
  if (has_valid_edges ()) {
    return std::make_pair (db::RecursiveShapeIterator (m_edges), db::ICplxTrans ());
  } else {
    return std::make_pair (m_iter, m_iter_trans);
  }
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans>
Edges::begin_merged_iter () const
{
  if (! m_merged_semantics || m_is_merged) {
    return begin_iter ();
  } else {
    ensure_merged_edges_valid ();
    return std::make_pair (db::RecursiveShapeIterator (m_merged_edges), db::ICplxTrans ());
  }
}

void 
Edges::insert (const db::Edge &edge)
{
  ensure_valid_edges ();
  m_edges.insert (edge);
  m_is_merged = false;
  invalidate_cache ();
}

void 
Edges::insert (const db::Box &box)
{
  if (! box.empty ()) {
    ensure_valid_edges ();
    m_edges.insert (db::Edge (box.lower_left (), box.upper_left ()));
    m_edges.insert (db::Edge (box.upper_left (), box.upper_right ()));
    m_edges.insert (db::Edge (box.upper_right (), box.lower_right ()));
    m_edges.insert (db::Edge (box.lower_right (), box.lower_left ()));
    m_is_merged = false;
    invalidate_cache ();
  }
}

void 
Edges::insert (const db::Path &path)
{
  if (path.points () > 0) {
    insert (path.polygon ());
  }
}

void 
Edges::insert (const db::Polygon &polygon)
{
  ensure_valid_edges ();
  for (db::Polygon::polygon_edge_iterator e = polygon.begin_edge (); ! e.at_end (); ++e) {
    m_edges.insert (*e);
  }
  m_is_merged = false;
  invalidate_cache ();
}

void 
Edges::insert (const db::SimplePolygon &polygon)
{
  ensure_valid_edges ();
  for (db::SimplePolygon::polygon_edge_iterator e = polygon.begin_edge (); ! e.at_end (); ++e) {
    m_edges.insert (*e);
  }
  m_is_merged = false;
  invalidate_cache ();
}

void 
Edges::clear ()
{
  m_edges.clear ();
  m_bbox = db::Box ();
  m_bbox_valid = true;
  m_is_merged = true;
  m_merged_edges.clear ();
  m_merged_edges_valid = true;
  m_iter = db::RecursiveShapeIterator ();
  m_iter_trans = db::ICplxTrans ();
}

namespace
{

/**
 *  @brief A helper class for the DRC functionality which acts as an edge pair receiver
 *
 *  If will perform a edge by edge check using the provided EdgeRelationFilter
 */
class Edge2EdgeCheck
  : public db::box_scanner_receiver<db::Edge, size_t>
{
public:
  Edge2EdgeCheck (const EdgeRelationFilter &check, EdgePairs &output, bool requires_different_layers)
    : mp_check (&check), mp_output (&output)
  {
    m_requires_different_layers = requires_different_layers;
  }
  
  void add (const db::Edge *o1, size_t p1, const db::Edge *o2, size_t p2)
  { 
    //  Overlap or inside checks require input from different layers
    if (! m_requires_different_layers || ((p1 ^ p2) & 1) != 0) {

      //  ensure that the first check argument is of layer 1 and the second of
      //  layer 2 (unless both are of the same layer)
      int l1 = int (p1 & size_t (1));
      int l2 = int (p2 & size_t (1));

      db::EdgePair ep;
      if (mp_check->check (l1 <= l2 ? *o1 : *o2, l1 <= l2 ? *o2 : *o1, &ep)) {
        mp_output->insert (ep);
      }

    }
  }

private:
  const EdgeRelationFilter *mp_check;
  EdgePairs *mp_output;
  bool m_requires_different_layers;
};

}

EdgePairs 
Edges::run_check (db::edge_relation_type rel, const Edges *other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
{
  EdgePairs result;

  db::box_scanner<db::Edge, size_t> scanner (m_report_progress, m_progress_desc);
  scanner.reserve (size () + (other ? other->size () : 0));

  ensure_valid_merged_edges ();
  size_t n = 0;
  for (const_iterator e = begin_merged (); ! e.at_end (); ++e) {
    scanner.insert (&*e, n); 
    n += 2;
  }

  if (other) {
    other->ensure_valid_merged_edges ();
    n = 1;
    for (const_iterator e = other->begin_merged (); ! e.at_end (); ++e) {
      scanner.insert (&*e, n); 
      n += 2;
    }
  }

  EdgeRelationFilter check (rel, d, metrics);
  check.set_include_zero (other != 0);
  check.set_whole_edges (whole_edges);
  check.set_ignore_angle (ignore_angle);
  check.set_min_projection (min_projection);
  check.set_max_projection (max_projection);

  Edge2EdgeCheck edge_check (check, result, other != 0);
  scanner.process (edge_check, d, db::box_convert<db::Edge> ());

  return result;
}

size_t 
Edges::size () const
{
  if (! has_valid_edges ()) {
    //  If we have an iterator, we have to do it the hard way ..
    size_t n = 0;
    for (const_iterator e = begin (); ! e.at_end (); ++e) {
      ++n;
    }
    return n;
  } else {
    return m_edges.size ();
  }
}

void 
Edges::set_merged_semantics (bool f)
{
  if (f != m_merged_semantics) {
    m_merged_semantics = f;
    m_merged_edges.clear ();
    m_merged_edges_valid = false;
  }
}

void
Edges::set_valid_edges ()
{
  m_iter = db::RecursiveShapeIterator ();
}

void
Edges::ensure_valid_edges () const
{
  if (! has_valid_edges ()) {

    m_edges.clear ();

    size_t n = 0;
    for (const_iterator e = begin (); ! e.at_end (); ++e) {
      ++n;
    }
    m_edges.reserve (db::Edge::tag (), n);

    for (const_iterator e = begin (); ! e.at_end (); ++e) {
      m_edges.insert (*e);
    }

    m_iter = db::RecursiveShapeIterator ();

  }
}

} // namespace db

namespace tl
{
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::Edges &b)
  {
    db::Edge e;

    if (! ex.try_read (e)) {
      return false;
    }
    b.insert (e);

    while (ex.test (";")) {
      ex.read (e);
      b.insert (e);
    } 

    return true;
  }

  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::Edges &b)
  {
    if (! test_extractor_impl (ex, b)) {
      ex.error (tl::to_string (QObject::tr ("Expected an edge collection specification")));
    }
  }
}


