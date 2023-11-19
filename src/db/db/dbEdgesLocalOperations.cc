
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

}

