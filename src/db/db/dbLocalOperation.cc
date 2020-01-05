
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


#include "dbHierProcessor.h"
#include "dbBoxScanner.h"
#include "dbRecursiveShapeIterator.h"
#include "dbBoxConvert.h"
#include "dbEdgeProcessor.h"
#include "dbPolygonGenerators.h"
#include "dbPolygonTools.h"
#include "dbLocalOperationUtils.h"
#include "dbEdgeBoolean.h"
#include "tlLog.h"
#include "tlTimer.h"
#include "tlInternational.h"

namespace db
{

// ---------------------------------------------------------------------------------------------
//  BoolAndOrNotLocalOperation implementation

BoolAndOrNotLocalOperation::BoolAndOrNotLocalOperation (bool is_and)
  : m_is_and (is_and)
{
  //  .. nothing yet ..
}

local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef>::on_empty_intruder_mode
BoolAndOrNotLocalOperation::on_empty_intruder_hint () const
{
  return m_is_and ? Drop : Copy;
}

std::string
BoolAndOrNotLocalOperation::description () const
{
  return m_is_and ? tl::to_string (tr ("AND operation")) : tl::to_string (tr ("NOT operation"));
}

void
BoolAndOrNotLocalOperation::compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::unordered_set<db::PolygonRef> &result, size_t max_vertex_count, double area_ratio) const
{
  db::EdgeProcessor ep;

  size_t p1 = 0, p2 = 1;

  std::set<db::PolygonRef> others;
  for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j));
    }
  }

  for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

    const db::PolygonRef &subject = interactions.subject_shape (i->first);
    if (others.find (subject) != others.end ()) {
      if (m_is_and) {
        result.insert (subject);
      }
    } else if (i->second.empty ()) {
      //  shortcut (not: keep, and: drop)
      if (! m_is_and) {
        result.insert (subject);
      }
    } else {
      for (db::PolygonRef::polygon_edge_iterator e = subject.begin_edge (); ! e.at_end(); ++e) {
        ep.insert (*e, p1);
      }
      p1 += 2;
    }

  }

  if (! others.empty () || p1 > 0) {

    for (std::set<db::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o) {
      for (db::PolygonRef::polygon_edge_iterator e = o->begin_edge (); ! e.at_end(); ++e) {
        ep.insert (*e, p2);
      }
      p2 += 2;
    }

    db::BooleanOp op (m_is_and ? db::BooleanOp::And : db::BooleanOp::ANotB);
    db::PolygonRefGenerator pr (layout, result);
    db::PolygonSplitter splitter (pr, area_ratio, max_vertex_count);
    db::PolygonGenerator pg (splitter, true, true);
    ep.set_base_verbosity (50);
    ep.process (pg, op);

  }
}

// ---------------------------------------------------------------------------------------------

SelfOverlapMergeLocalOperation::SelfOverlapMergeLocalOperation (unsigned int wrap_count)
  : m_wrap_count (wrap_count)
{
  //  .. nothing yet ..
}

void SelfOverlapMergeLocalOperation::compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::unordered_set<db::PolygonRef> &result, size_t /*max_vertex_count*/, double /*area_ratio*/) const
{
  if (m_wrap_count == 0) {
    return;
  }

  db::EdgeProcessor ep;

  size_t p1 = 0, p2 = 1;
  std::set<unsigned int> seen;

  for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

    if (seen.find (i->first) == seen.end ()) {
      seen.insert (i->first);
      const db::PolygonRef &subject = interactions.subject_shape (i->first);
      for (db::PolygonRef::polygon_edge_iterator e = subject.begin_edge (); ! e.at_end(); ++e) {
        ep.insert (*e, p1);
      }
      p1 += 2;
    }

    for (db::shape_interactions<db::PolygonRef, db::PolygonRef>::iterator2 o = i->second.begin (); o != i->second.end (); ++o) {
      //  don't take the same (really the same, not an identical one) shape twice - the interaction
      //  set does not take care to list just one copy of the same item on the intruder side.
      if (seen.find (*o) == seen.end ()) {
        seen.insert (*o);
        const db::PolygonRef &intruder = interactions.intruder_shape (*o);
        for (db::PolygonRef::polygon_edge_iterator e = intruder.begin_edge (); ! e.at_end(); ++e) {
          ep.insert (*e, p2);
        }
        p2 += 2;
      }
    }

  }

  db::MergeOp op (m_wrap_count - 1);
  db::PolygonRefGenerator pr (layout, result);
  db::PolygonGenerator pg (pr, true, true);
  ep.set_base_verbosity (50);
  ep.process (pg, op);
}

SelfOverlapMergeLocalOperation::on_empty_intruder_mode SelfOverlapMergeLocalOperation::on_empty_intruder_hint () const
{
  return m_wrap_count > 1 ? Drop : Copy;
}

std::string SelfOverlapMergeLocalOperation::description () const
{
  return tl::sprintf (tl::to_string (tr ("Self-overlap (wrap count %d)")), int (m_wrap_count));
}

// ---------------------------------------------------------------------------------------------
//  EdgeBoolAndOrNotLocalOperation implementation

EdgeBoolAndOrNotLocalOperation::EdgeBoolAndOrNotLocalOperation (EdgeBoolOp op)
  : m_op (op)
{
  //  .. nothing yet ..
}

local_operation<db::Edge, db::Edge, db::Edge>::on_empty_intruder_mode
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
EdgeBoolAndOrNotLocalOperation::compute_local (db::Layout * /*layout*/, const shape_interactions<db::Edge, db::Edge> &interactions, std::unordered_set<db::Edge> &result, size_t /*max_vertex_count*/, double /*area_ratio*/) const
{
  EdgeBooleanClusterCollector<std::unordered_set<db::Edge> > cluster_collector (&result, m_op);

  db::box_scanner<db::Edge, size_t> scanner;

  std::set<db::Edge> others;
  for (shape_interactions<db::Edge, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (shape_interactions<db::Edge, db::Edge>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j));
    }
  }

  bool any_subject = false;
  bool is_and = (m_op == EdgeAnd || m_op == EdgeIntersections);

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

EdgeToPolygonLocalOperation::EdgeToPolygonLocalOperation (bool outside, bool include_borders)
  : m_outside (outside), m_include_borders (include_borders)
{
  //  .. nothing yet ..
}

local_operation<db::Edge, db::PolygonRef, db::Edge>::on_empty_intruder_mode
EdgeToPolygonLocalOperation::on_empty_intruder_hint () const
{
  return m_outside ? Copy : Drop;
}

std::string
EdgeToPolygonLocalOperation::description () const
{
  return tl::to_string (m_outside ? tr ("Edge to polygon AND/INSIDE") : tr ("Edge to polygons NOT/OUTSIDE"));
}

void
EdgeToPolygonLocalOperation::compute_local (db::Layout * /*layout*/, const shape_interactions<db::Edge, db::PolygonRef> &interactions, std::unordered_set<db::Edge> &result, size_t /*max_vertex_count*/, double /*area_ratio*/) const
{
  db::EdgeProcessor ep;

  std::set<db::PolygonRef> others;
  for (shape_interactions<db::Edge, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (shape_interactions<db::Edge, db::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j));
    }
  }

  bool any_subject = false;

  for (shape_interactions<db::Edge, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

    const db::Edge &subject = interactions.subject_shape (i->first);
    if (i->second.empty ()) {
      //  shortcut (outside: keep, otherwise: drop)
      if (m_outside) {
        result.insert (subject);
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

    db::EdgeToEdgeSetGenerator cc (result);
    db::EdgePolygonOp op (m_outside, m_include_borders);
    ep.process (cc, op);

  }
}

}

