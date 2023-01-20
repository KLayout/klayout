
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

#ifndef HDR_dbEdgeBoolean
#define HDR_dbEdgeBoolean

#include "dbEdge.h"
#include "dbHash.h"
#include "dbBoxScanner.h"
#include "dbShapes.h"

#include "tlIntervalMap.h"

namespace db
{

/**
 *  @brief A common definition for the boolean operations available on edges
 */
enum EdgeBoolOp { EdgeOr, EdgeNot, EdgeXor, EdgeAnd, EdgeIntersections, EdgeAndNot /*not always supported*/ };

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

template <class OutputContainer>
struct EdgeBooleanCluster 
  : public db::cluster<db::Edge, size_t>
{
  typedef db::Edge::coord_type coord_type;

  EdgeBooleanCluster (OutputContainer *output, EdgeBoolOp op)
    : mp_output (output), mp_output2 (0), m_op (op)
  {
    //  .. nothing yet ..
  }

  EdgeBooleanCluster (OutputContainer *output, OutputContainer *output2, EdgeBoolOp op)
    : mp_output (output), mp_output2 (output2), m_op (op)
  {
    //  .. nothing yet ..
  }

  void finish ()
  {
    //  determine base edge (longest overall edge)

    //  shortcut for single edge
    if (begin () + 1 == end ()) {
      if (begin ()->second == 0) {
        if (m_op == EdgeAndNot) {
          mp_output2->insert (*(begin ()->first));
        } else if (m_op != EdgeAnd) {
          mp_output->insert (*(begin ()->first));
        }
      } else {
        if (m_op != EdgeAnd && m_op != EdgeNot && m_op != EdgeAndNot) {
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
      if (o->second == 0 || m_op == EdgeOr) {
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
      OutputContainer *oc = 0;
      if (m_op == EdgeAndNot) {
        oc = mp_output;
      } else if (m_op != EdgeAnd) {
        oc = mp_output;
      }

      if (oc) {
        for (tl::interval_map<db::Coord, int>::const_iterator ib = b.begin (); ib != b.end (); ++ib) {
          if (ib->second > 0) {
            oc->insert (db::Edge (p1 + db::Vector (d * (ib->first.first * n)), p1 + db::Vector (d * (ib->first.second * n))));
          } else if (ib->second < 0) {
            oc->insert (db::Edge (p1 + db::Vector (d * (ib->first.second * n)), p1 + db::Vector (d * (ib->first.first * n))));
          }
        }
      }

    } else {

      tl::interval_map<db::Coord, int> q2;

      if (m_op == EdgeAnd || m_op == EdgeAndNot) {
        if (m_op == EdgeAndNot) {
          q2 = q;
          for (tl::interval_map<db::Coord, int>::const_iterator ib = b.begin (); ib != b.end (); ++ib) {
            q2.add (ib->first.first, ib->first.second, ib->second, not_jop);
          }
        }
        for (tl::interval_map<db::Coord, int>::const_iterator ib = b.begin (); ib != b.end (); ++ib) {
          q.add (ib->first.first, ib->first.second, ib->second, and_jop);
        }
      } else if (m_op == EdgeNot) {
        for (tl::interval_map<db::Coord, int>::const_iterator ib = b.begin (); ib != b.end (); ++ib) {
          q.add (ib->first.first, ib->first.second, ib->second, not_jop);
        }
      } else if (m_op == EdgeXor) {
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

      for (tl::interval_map<db::Coord, int>::const_iterator iq = q2.begin (); iq != q2.end (); ++iq) {
        if (iq->second > 0) {
          mp_output2->insert (db::Edge (p1 + db::Vector (d * (iq->first.first * n)), p1 + db::Vector (d * (iq->first.second * n))));
        } else if (iq->second < 0) {
          mp_output2->insert (db::Edge (p1 + db::Vector (d * (iq->first.second * n)), p1 + db::Vector (d * (iq->first.first * n))));
        }
      }

    }

  }

private:
  OutputContainer *mp_output, *mp_output2;
  db::EdgeBoolOp m_op;
};

template <class OutputContainer>
struct EdgeBooleanClusterCollector
  : public db::cluster_collector<db::Edge, size_t, EdgeBooleanCluster<OutputContainer> >
{
  EdgeBooleanClusterCollector (OutputContainer *output, EdgeBoolOp op, OutputContainer *output2 = 0)
    : db::cluster_collector<db::Edge, size_t, EdgeBooleanCluster<OutputContainer> > (EdgeBooleanCluster<OutputContainer> (output, output2, op == EdgeIntersections ? EdgeAnd : op), op != EdgeAnd && op != EdgeIntersections /*report single*/),
      mp_output (output), mp_intersections (op == EdgeIntersections ? output : 0)
  {
    //  .. nothing yet ..
  }

  EdgeBooleanClusterCollector (OutputContainer *output, OutputContainer *intersections, EdgeBoolOp op)
    : db::cluster_collector<db::Edge, size_t, EdgeBooleanCluster<OutputContainer> > (EdgeBooleanCluster<OutputContainer> (output, op), op != EdgeAnd /*report single*/),
      mp_output (output), mp_intersections (intersections)
  {
    //  .. nothing yet ..
  }

  void add (const db::Edge *o1, size_t p1, const db::Edge *o2, size_t p2)
  {
    //  Select edges which are:
    //  1.) not degenerate
    //  2.) parallel with some tolerance of roughly 1 dbu
    //  3.) connected
    //  In intersection-detection mode, identify intersection points otherwise
    //  and insert into the intersections container as degenerated edges.

    if (! o1->is_degenerate () && ! o2->is_degenerate () 
        && fabs ((double) db::vprod (*o1, *o2)) < db::coord_traits<db::Coord>::prec_distance () * std::min (o1->double_length (), o2->double_length ())
        && (o1->p1 () == o2->p1 () || o1->p1 () == o2->p2 () || o1->p2 () == o2->p1 () || o1->p2 () == o2->p2 () || o1->coincident (*o2))) {

      db::cluster_collector<db::Edge, size_t, EdgeBooleanCluster<OutputContainer> >::add (o1, p1, o2, p2);

    } else if (mp_intersections && p1 != p2) {

      std::pair<bool, db::Point> ip = o1->intersect_point (*o2);
      if (ip.first) {
        m_intersections.insert (ip.second);
      }

    }
  }

  /**
   *  @brief A receiver for the reducer which removes points that are on the edges
   */
  struct RemovePointsOnEdges
    : public db::box_scanner_receiver2<db::Edge, size_t, db::Point, size_t>
  {
  public:
    RemovePointsOnEdges (std::set<db::Point> &points_to_remove)
      : mp_points_to_remove (&points_to_remove)
    { }

    void add (const db::Edge *e, const size_t &, const db::Point *pt, const size_t &)
    {
      if (e->contains (*pt)) {
        mp_points_to_remove->insert (*pt);
      }
    }

  private:
    std::set<db::Point> *mp_points_to_remove;
  };

  /**
   *  @brief An inserter to produce degenerated edges from points
   */
  struct PointInserter
    : public std::iterator<std::output_iterator_tag, void, void, void, void>
  {
    typedef db::Point value_type;

    PointInserter (OutputContainer *output)
      : mp_output (output)
    { }

    PointInserter &operator= (const db::Point &pt)
    {
      mp_output->insert (db::Edge (pt, pt));
      return *this;
    }

    PointInserter &operator* () { return *this; }
    PointInserter &operator++ () { return *this; }
    PointInserter &operator++ (int) { return *this; }

  private:
    OutputContainer *mp_output;
  };

  /**
   *  @brief Finalizes the implementation for "EdgeIntersections"
   *  This method pushes those points which don't interact with the edges to the output container
   *  as degenerate edges. It needs to be called after the pass has been made.
   */
  void finalize (bool)
  {
    if (m_intersections.empty ()) {
      return;
    }

    db::box_scanner2<db::Edge, size_t, db::Point, size_t> intersections_to_edge_scanner;
    for (typename OutputContainer::const_iterator e = mp_output->begin (); e != mp_output->end (); ++e) {
      intersections_to_edge_scanner.insert1 (e.operator-> (), 0);
    }
    for (std::set<db::Point>::const_iterator p = m_intersections.begin (); p != m_intersections.end (); ++p) {
      intersections_to_edge_scanner.insert2 (p.operator-> (), 0);
    }

    std::set<db::Point> points_to_remove;
    RemovePointsOnEdges rpoe (points_to_remove);
    intersections_to_edge_scanner.process (rpoe, 1, db::box_convert<db::Edge> (), db::box_convert<db::Point> ());

    std::set_difference (m_intersections.begin (), m_intersections.end (), points_to_remove.begin (), points_to_remove.end (), PointInserter (mp_intersections));
  }

private:
  OutputContainer *mp_output;
  OutputContainer *mp_intersections;
  std::set<db::Point> m_intersections;
};

/**
 *  @brief A helper class to use db::Shapes as container for EdgeBooleanClusterCollector
 */
struct DB_PUBLIC ShapesToOutputContainerAdaptor
{
public:
  struct Iterator
    : public db::Shapes::shape_iterator
  {
    Iterator (const db::Shapes::shape_iterator &iter)
      : db::Shapes::shape_iterator (iter)
    { }

    Iterator ()
      : db::Shapes::shape_iterator ()
    { }

    const db::Edge *operator-> () const
    {
      return (db::Shapes::shape_iterator::operator* ()).basic_ptr (db::Edge::tag ());
    }

    bool operator!= (const Iterator &other) const
    {
      //  only for testing whether at end:
      return at_end () != other.at_end ();
    }

    bool operator== (const Iterator &other) const
    {
      //  only for testing whether at end:
      return at_end () == other.at_end ();
    }
  };

  typedef Iterator const_iterator;

  ShapesToOutputContainerAdaptor ()
    : mp_shapes (0), m_prop_id (0)
  {
    //  .. nothing yet ..
  }

  ShapesToOutputContainerAdaptor (db::Shapes &shapes, db::properties_id_type prop_id = 0)
    : mp_shapes (&shapes), m_prop_id (prop_id)
  {
    //  .. nothing yet ..
  }

  const_iterator begin ()
  {
    return Iterator (mp_shapes->begin (db::ShapeIterator::Edges));
  }

  const_iterator end ()
  {
    return Iterator ();
  }

  void insert (const db::Edge &edge)
  {
    if (m_prop_id != 0) {
      mp_shapes->insert (db::EdgeWithProperties (edge, m_prop_id));
    } else {
      mp_shapes->insert (edge);
    }
  }

private:
  db::Shapes *mp_shapes;
  db::properties_id_type m_prop_id;
};

/**
 *  @brief A specialization of the EdgeBooleanClusterCollector for a Shapes output container
 */
struct DB_PUBLIC EdgeBooleanClusterCollectorToShapes
  : EdgeBooleanClusterCollector<ShapesToOutputContainerAdaptor>
{
  EdgeBooleanClusterCollectorToShapes (db::Shapes *output, EdgeBoolOp op, db::properties_id_type prop_id = 0)
    : EdgeBooleanClusterCollector<ShapesToOutputContainerAdaptor> (&m_adaptor, op), m_adaptor (*output, prop_id)
  {
  }

  EdgeBooleanClusterCollectorToShapes (db::Shapes *output, EdgeBoolOp op, db::Shapes *output2, db::properties_id_type prop_id = 0)
    : EdgeBooleanClusterCollector<ShapesToOutputContainerAdaptor> (&m_adaptor, op, &m_adaptor2), m_adaptor (*output, prop_id), m_adaptor2 (*output2, prop_id)
  {
  }

private:
  ShapesToOutputContainerAdaptor m_adaptor;
  ShapesToOutputContainerAdaptor m_adaptor2;
};

}

#endif

