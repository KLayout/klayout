
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#include "dbEdgesLocalOperations.h"
#include "dbHierProcessor.h"
#include "dbLocalOperationUtils.h"

namespace db
{

// ---------------------------------------------------------------------------------------------
//  EdgeBoolAndOrNotLocalOperation implementation

EdgeBoolAndOrNotLocalOperation::EdgeBoolAndOrNotLocalOperation (EdgeBoolOp op)
  : m_op (op)
{
  //  .. nothing yet ..
}

OnEmptyIntruderHint
EdgeBoolAndOrNotLocalOperation::on_empty_intruder_hint () const
{
  return (m_op == EdgeAnd || m_op == EdgeIntersections) ? Drop : Copy;
}

std::string
EdgeBoolAndOrNotLocalOperation::description () const
{
  if (m_op == EdgeIntersections) {
    return tl::to_string (tr ("Edge INTERSECTION operation"));
  } else if (m_op == EdgeAnd) {
    return tl::to_string (tr ("Edge AND operation"));
  } else if (m_op == EdgeNot) {
    return tl::to_string (tr ("Edge NOT operation"));
  } else {
    return std::string ();
  }
}

void
EdgeBoolAndOrNotLocalOperation::do_compute_local (db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Edge, db::Edge> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase * /*proc*/) const
{
  tl_assert (results.size () == size_t (m_op == EdgeAndNot ? 2 : 1));

  std::unordered_set<db::Edge> &result = results.front ();

  std::unordered_set<db::Edge> *result2 = 0;
  if (results.size () > 1) {
    result2 = &results[1];
  }

  EdgeBooleanClusterCollector<std::unordered_set<db::Edge> > cluster_collector (&result, m_op, result2);

  db::box_scanner<db::Edge, size_t> scanner;

  std::set<db::Edge> others;
  for (shape_interactions<db::Edge, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (shape_interactions<db::Edge, db::Edge>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  bool any_subject = false;
  bool is_and = (m_op == EdgeAnd || m_op == EdgeAndNot || m_op == EdgeIntersections);

  for (shape_interactions<db::Edge, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

    const db::Edge &subject = interactions.subject_shape (i->first);
    if (others.find (subject) != others.end ()) {
      if (is_and) {
        result.insert (subject);
      }
    } else if (i->second.empty ()) {
      //  shortcut (not: keep, and: drop)
      if (! is_and) {
        result.insert (subject);
      }
      if (result2) {
        result2->insert (subject);
      }
    } else {
      scanner.insert (&subject, 0);
      any_subject = true;
    }

  }

  if (! others.empty () || any_subject) {

    for (std::set<db::Edge>::const_iterator o = others.begin (); o != others.end (); ++o) {
      scanner.insert (o.operator-> (), 1);
    }

    scanner.process (cluster_collector, 1, db::box_convert<db::Edge> ());

  }
}

// ---------------------------------------------------------------------------------------------
//  EdgeToPolygonLocalOperation implementation

EdgeToPolygonLocalOperation::EdgeToPolygonLocalOperation (EdgePolygonOp::mode_t op, bool include_borders)
  : m_op (op), m_include_borders (include_borders)
{
  //  .. nothing yet ..
}

OnEmptyIntruderHint
EdgeToPolygonLocalOperation::on_empty_intruder_hint () const
{
  return m_op == EdgePolygonOp::Inside ? Drop : (m_op == EdgePolygonOp::Outside ? Copy : CopyToSecond);
}

std::string
EdgeToPolygonLocalOperation::description () const
{
  if (m_op == EdgePolygonOp::Inside) {
    return tl::to_string (tr ("Edge to polygon AND/INSIDE"));
  } else if (m_op == EdgePolygonOp::Outside) {
    return tl::to_string (tr ("Edge to polygon NOT/OUTSIDE"));
  } else {
    return tl::to_string (tr ("Edge to polygon ANDNOT/INOUTSIDE"));
  }
}

void
EdgeToPolygonLocalOperation::do_compute_local (db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Edge, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase * /*proc*/) const
{
  tl_assert (results.size () == size_t (m_op == EdgePolygonOp::Both ? 2 : 1));

  std::unordered_set<db::Edge> &result = results.front ();

  std::unordered_set<db::Edge> *result2 = 0;
  if (results.size () > 1) {
    result2 = &results[1];
  }

  db::EdgeProcessor ep;

  std::set<db::PolygonRef> others;
  for (shape_interactions<db::Edge, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (shape_interactions<db::Edge, db::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  bool any_subject = false;

  for (shape_interactions<db::Edge, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

    const db::Edge &subject = interactions.subject_shape (i->first);
    if (i->second.empty ()) {
      //  shortcut (outside: keep, otherwise: drop)
      if (m_op == db::EdgePolygonOp::Outside) {
        result.insert (subject);
      } else if (m_op == db::EdgePolygonOp::Both) {
        result2->insert (subject);
      }
    } else {
      ep.insert (subject, 1);
      any_subject = true;
    }

  }

  if (! others.empty () || any_subject) {

    for (std::set<db::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o) {
      for (db::PolygonRef::polygon_edge_iterator e = o->begin_edge (); ! e.at_end (); ++e) {
        ep.insert (*e, 0);
      }
    }

    std::unique_ptr<db::EdgeToEdgeSetGenerator> cc_second;
    if (result2) {
      cc_second.reset (new db::EdgeToEdgeSetGenerator (*result2, 2 /*second tag*/));
    }

    db::EdgeToEdgeSetGenerator cc (result, 1 /*first tag*/, cc_second.get ());
    db::EdgePolygonOp op (m_op, m_include_borders);
    ep.process (cc, op);

  }
}

// ---------------------------------------------------------------------------------------------
//  Edge2EdgeInteractingLocalOperation implementation

Edge2EdgeInteractingLocalOperation::Edge2EdgeInteractingLocalOperation (EdgeInteractionMode mode, output_mode_t output_mode, size_t min_count, size_t max_count)
  : m_mode (mode), m_output_mode (output_mode), m_min_count (min_count), m_max_count (max_count)
{
  //  .. nothing yet ..
}

db::Coord Edge2EdgeInteractingLocalOperation::dist () const
{
  //  touching is sufficient
  return 1;
}

void Edge2EdgeInteractingLocalOperation::do_compute_local (db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Edge, db::Edge> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase * /*proc*/) const
{
  tl_assert (results.size () == (m_output_mode == Both ? 2 : 1));

  std::unordered_set<db::Edge> &result = results.front ();

  std::unordered_set<db::Edge> *result2 = 0;
  if (m_output_mode == Both) {
    result2 = &results[1];
  }

  db::box_scanner<db::Edge, size_t> scanner;

  std::set<db::Edge> others;
  for (shape_interactions<db::Edge, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (shape_interactions<db::Edge, db::Edge>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  for (shape_interactions<db::Edge, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    const db::Edge &subject = interactions.subject_shape (i->first);
    scanner.insert (&subject, 0);
  }

  for (std::set<db::Edge>::const_iterator o = others.begin (); o != others.end (); ++o) {
    scanner.insert (o.operator-> (), 1);
  }

  if (m_output_mode == Inverse || m_output_mode == Both) {

    std::unordered_set<db::Edge> interacting;
    edge_interaction_filter<std::unordered_set<db::Edge> > filter (interacting, m_mode, m_min_count, m_max_count);
    scanner.process (filter, 1, db::box_convert<db::Edge> ());

    for (shape_interactions<db::Edge, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

      const db::Edge &subject = interactions.subject_shape (i->first);
      if (interacting.find (subject) == interacting.end ()) {
        if (m_output_mode != Both) {
          result.insert (subject);
        } else {
          result2->insert (subject);
        }
      } else if (m_output_mode == Both) {
        result.insert (subject);
      }

    }

  } else {

    edge_interaction_filter<std::unordered_set<db::Edge> > filter (result, m_mode, m_min_count, m_max_count);
    scanner.process (filter, 1, db::box_convert<db::Edge> ());

  }

}

OnEmptyIntruderHint Edge2EdgeInteractingLocalOperation::on_empty_intruder_hint () const
{
  if (m_mode == EdgesOutside) {
    return m_output_mode == Both ? Copy : (m_output_mode == Inverse ? Drop : Copy);
  } else {
    return m_output_mode == Both ? CopyToSecond : (m_output_mode == Inverse ? Copy : Drop);
  }
}

std::string Edge2EdgeInteractingLocalOperation::description () const
{
  return tl::to_string (tr ("Select interacting edges"));
}

// ---------------------------------------------------------------------------------------------
//  Edge2EdgePullLocalOperation implementation

Edge2EdgePullLocalOperation::Edge2EdgePullLocalOperation ()
{
  //  .. nothing yet ..
}

db::Coord Edge2EdgePullLocalOperation::dist () const
{
  //  touching is sufficient
  return 1;
}

void Edge2EdgePullLocalOperation::do_compute_local (db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Edge, db::Edge> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase * /*proc*/) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<db::Edge> &result = results.front ();

  db::box_scanner<db::Edge, size_t> scanner;

  std::set<db::Edge> others;
  for (shape_interactions<db::Edge, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (shape_interactions<db::Edge, db::Edge>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  for (shape_interactions<db::Edge, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    const db::Edge &subject = interactions.subject_shape (i->first);
    scanner.insert (&subject, 1);
  }

  for (std::set<db::Edge>::const_iterator o = others.begin (); o != others.end (); ++o) {
    scanner.insert (o.operator-> (), 0);
  }

  edge_interaction_filter<std::unordered_set<db::Edge> > filter (result, EdgesInteract, size_t (1), std::numeric_limits<size_t>::max ());
  scanner.process (filter, 1, db::box_convert<db::Edge> ());

}

OnEmptyIntruderHint Edge2EdgePullLocalOperation::on_empty_intruder_hint () const
{
  return Drop;
}

std::string Edge2EdgePullLocalOperation::description () const
{
  return tl::to_string (tr ("Select interacting edges from other"));
}


// ---------------------------------------------------------------------------------------------
//  edge_to_polygon_interacting_local_operation implementation

template <class TI>
edge_to_polygon_interacting_local_operation<TI>::edge_to_polygon_interacting_local_operation (EdgeInteractionMode mode, output_mode_t output_mode, size_t min_count, size_t max_count)
  : m_mode (mode), m_output_mode (output_mode), m_min_count (min_count), m_max_count (max_count)
{
  //  .. nothing yet ..
}

template <class TI>
db::Coord edge_to_polygon_interacting_local_operation<TI>::dist () const
{
  //  touching is sufficient
  return 1;
}

static const db::Polygon *deref (const db::Polygon &poly, std::list<db::Polygon> &)
{
  return &poly;
}

static const db::Polygon *deref (const db::PolygonRef &pref, std::list<db::Polygon> &heap)
{
  heap.push_back (pref.obj ().transformed (pref.trans ()));
  return & heap.back ();
}

template <class TI>
void edge_to_polygon_interacting_local_operation<TI>::do_compute_local (db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::Edge, TI> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase * /*proc*/) const
{
  tl_assert (results.size () == size_t (m_output_mode == Both ? 2 : 1));

  std::unordered_set<db::Edge> &result = results.front ();

  std::unordered_set<db::Edge> *result2 = 0;
  if (m_output_mode == Both) {
    result2 = &results[1];
  }

  db::box_scanner2<db::Edge, size_t, db::Polygon, size_t> scanner;

  std::set<TI> others;
  for (typename shape_interactions<db::Edge, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (typename shape_interactions<db::Edge, TI>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  for (typename shape_interactions<db::Edge, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    const db::Edge &subject = interactions.subject_shape (i->first);
    scanner.insert1 (&subject, 0);
  }

  std::list<db::Polygon> heap;
  for (typename std::set<TI>::const_iterator o = others.begin (); o != others.end (); ++o) {
    scanner.insert2 (deref (*o, heap), 1);
  }

  if (m_output_mode == Inverse || m_output_mode == Both) {

    std::unordered_set<db::Edge> interacting;
    edge_to_polygon_interaction_filter<std::unordered_set<db::Edge> > filter (&interacting, m_mode, m_min_count, m_max_count);
    scanner.process (filter, 1, db::box_convert<db::Edge> (), db::box_convert<db::Polygon> ());

    for (typename shape_interactions<db::Edge, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

      const db::Edge &subject = interactions.subject_shape (i->first);

      if (interacting.find (subject) == interacting.end ()) {
        if (m_output_mode != Both) {
          result.insert (subject);
        } else {
          result2->insert (subject);
        }
      } else if (m_output_mode == Both) {
        result.insert (subject);
      }

    }

  } else {

    edge_to_polygon_interaction_filter<std::unordered_set<db::Edge> > filter (&result, m_mode, m_min_count, m_max_count);
    scanner.process (filter, 1, db::box_convert<db::Edge> (), db::box_convert<db::Polygon> ());

  }
}

template <class TI>
OnEmptyIntruderHint edge_to_polygon_interacting_local_operation<TI>::on_empty_intruder_hint () const
{
  if (m_mode == EdgesOutside) {
    return m_output_mode == Both ? Copy : (m_output_mode == Inverse ? Drop : Copy);
  } else {
    return m_output_mode == Both ? CopyToSecond : (m_output_mode == Inverse ? Copy : Drop);
  }
}

template <class TI>
std::string edge_to_polygon_interacting_local_operation<TI>::description () const
{
  if (m_mode == EdgesInteract) {
    if (m_output_mode == Inverse) {
      return tl::to_string (tr ("Select non-interacting edges"));
    } else if (m_output_mode == Normal) {
      return tl::to_string (tr ("Select interacting edges"));
    } else {
      return tl::to_string (tr ("Select interacting and non-interacting edges"));
    }
  } else if (m_mode == EdgesInside) {
    if (m_output_mode == Inverse) {
      return tl::to_string (tr ("Select non-inside edges"));
    } else if (m_output_mode == Normal) {
      return tl::to_string (tr ("Select inside edges"));
    } else {
      return tl::to_string (tr ("Select inside and non-inside edges"));
    }
  } else if (m_mode == EdgesOutside) {
    if (m_output_mode == Inverse) {
      return tl::to_string (tr ("Select non-outside edges"));
    } else if (m_output_mode == Normal) {
      return tl::to_string (tr ("Select outside edges"));
    } else {
      return tl::to_string (tr ("Select outside and non-outside edges"));
    }
  }
  return std::string ();
}

template class edge_to_polygon_interacting_local_operation<db::Polygon>;
template class edge_to_polygon_interacting_local_operation<db::PolygonRef>;

// ---------------------------------------------------------------------------------------------
//  Edge2EdgePullLocalOperation implementation

namespace {

struct ResultInserter
{
  typedef db::Polygon value_type;

  ResultInserter (db::Layout *layout, std::unordered_set<db::PolygonRef> &result)
    : mp_layout (layout), mp_result (&result)
  {
    //  .. nothing yet ..
  }

  void insert (const db::Polygon &p)
  {
    (*mp_result).insert (db::PolygonRef (p, mp_layout->shape_repository ()));
  }

private:
  db::Layout *mp_layout;
  std::unordered_set<db::PolygonRef> *mp_result;
};

}

Edge2PolygonPullLocalOperation::Edge2PolygonPullLocalOperation ()
{
  //  .. nothing yet ..
}

db::Coord Edge2PolygonPullLocalOperation::dist () const
{
  //  touching is sufficient
  return 1;
}

void Edge2PolygonPullLocalOperation::do_compute_local (db::Layout *layout, db::Cell * /*cell*/, const shape_interactions<db::Edge, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase * /*proc*/) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<db::PolygonRef> &result = results.front ();

  db::box_scanner2<db::Edge, size_t, db::Polygon, size_t> scanner;

  std::set<db::PolygonRef> others;
  for (shape_interactions<db::Edge, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (shape_interactions<db::Edge, db::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  for (shape_interactions<db::Edge, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    const db::Edge &subject = interactions.subject_shape (i->first);
    scanner.insert1 (&subject, 1);
  }

  std::list<db::Polygon> heap;
  for (std::set<db::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o) {
    heap.push_back (o->obj ().transformed (o->trans ()));
    scanner.insert2 (& heap.back (), 0);
  }

  ResultInserter inserter (layout, result);
  edge_to_polygon_interaction_filter<ResultInserter> filter (&inserter, EdgesInteract, size_t (1), std::numeric_limits<size_t>::max ());
  scanner.process (filter, 1, db::box_convert<db::Edge> (), db::box_convert<db::Polygon> ());
}

OnEmptyIntruderHint Edge2PolygonPullLocalOperation::on_empty_intruder_hint () const
{
  return Drop;
}

std::string Edge2PolygonPullLocalOperation::description () const
{
  return tl::to_string (tr ("Select interacting regions"));
}

}

