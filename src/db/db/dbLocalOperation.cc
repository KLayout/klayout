
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

#include "dbLocalOperation.h"
#include "dbHierProcessor.h"
#include "dbBoxScanner.h"
#include "dbRecursiveShapeIterator.h"
#include "dbBoxConvert.h"
#include "dbPolygonGenerators.h"
#include "dbPolygonTools.h"
#include "dbLocalOperationUtils.h"
#include "dbEdgeBoolean.h"
#include "dbLayoutUtils.h"
#include "tlLog.h"
#include "tlTimer.h"
#include "tlInternational.h"
#include "tlProgress.h"
#include "tlSList.h"

namespace db
{

// ---------------------------------------------------------------------------------------------
//  local_operations implementation

template <class TS, class TI, class TR>
void local_operation<TS, TI, TR>::compute_local (db::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio, bool report_progress, const std::string &progress_desc) const
{
  if (interactions.num_subjects () <= 1 || ! requests_single_subjects ()) {

    do_compute_local (layout, interactions, results, max_vertex_count, area_ratio);

  } else {

    std::unique_ptr<tl::RelativeProgress> progress;
    if (report_progress) {
      progress.reset (new tl::RelativeProgress (progress_desc, interactions.size ()));
    }

    for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      const TS &subject_shape = interactions.subject_shape (i->first);

      shape_interactions<TS, TI> single_interactions;

      if (on_empty_intruder_hint () == OnEmptyIntruderHint::Drop) {
        single_interactions.add_subject_shape (i->first, subject_shape);
      } else {
        //  this includes the subject-without-intruder "interaction"
        single_interactions.add_subject (i->first, subject_shape);
      }

      const std::vector<unsigned int> &intruders = interactions.intruders_for (i->first);
      for (typename std::vector<unsigned int>::const_iterator ii = intruders.begin (); ii != intruders.end (); ++ii) {
        const std::pair<unsigned int, TI> &is = interactions.intruder_shape (*ii);
        single_interactions.add_intruder_shape (*ii, is.first, is.second);
        single_interactions.add_interaction (i->first, *ii);
      }

      do_compute_local (layout, single_interactions, results, max_vertex_count, area_ratio);

      if (progress.get ()) {
        ++*progress;
      }

    }

  }
}


//  explicit instantiations
template class DB_PUBLIC local_operation<db::Polygon, db::Polygon, db::Polygon>;
template class DB_PUBLIC local_operation<db::Polygon, db::Polygon, db::Edge>;
template class DB_PUBLIC local_operation<db::Polygon, db::Text, db::Polygon>;
template class DB_PUBLIC local_operation<db::Polygon, db::Text, db::Text>;
template class DB_PUBLIC local_operation<db::Polygon, db::Edge, db::Polygon>;
template class DB_PUBLIC local_operation<db::Polygon, db::Edge, db::Edge>;
template class DB_PUBLIC local_operation<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::PolygonRefWithProperties>;
template class DB_PUBLIC local_operation<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgeWithProperties>;
template class DB_PUBLIC local_operation<db::PolygonWithProperties, db::PolygonWithProperties, db::PolygonWithProperties>;
template class DB_PUBLIC local_operation<db::PolygonWithProperties, db::PolygonWithProperties, db::EdgeWithProperties>;
template class DB_PUBLIC local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_operation<db::PolygonRef, db::Text, db::PolygonRef>;
template class DB_PUBLIC local_operation<db::PolygonRef, db::TextRef, db::PolygonRef>;
template class DB_PUBLIC local_operation<db::PolygonRef, db::TextRef, db::TextRef>;
template class DB_PUBLIC local_operation<db::PolygonRef, db::Edge, db::PolygonRef>;
template class DB_PUBLIC local_operation<db::PolygonRef, db::Edge, db::Edge>;
template class DB_PUBLIC local_operation<db::PolygonRef, db::PolygonRef, db::EdgePair>;
template class DB_PUBLIC local_operation<db::PolygonRef, db::PolygonRef, db::Edge>;
template class DB_PUBLIC local_operation<db::Polygon, db::Polygon, db::EdgePair>;
template class DB_PUBLIC local_operation<db::Polygon, db::TextRef, db::TextRef>;
template class DB_PUBLIC local_operation<db::Edge, db::Edge, db::Edge>;
template class DB_PUBLIC local_operation<db::Edge, db::PolygonRef, db::Edge>;
template class DB_PUBLIC local_operation<db::Edge, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_operation<db::Edge, db::Edge, db::EdgePair>;
template class DB_PUBLIC local_operation<db::TextRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC local_operation<db::TextRef, db::PolygonRef, db::TextRef>;

// ---------------------------------------------------------------------------------------------
//  BoolAndOrNotLocalOperation implementation

template <class TS, class TI, class TR>
bool_and_or_not_local_operation<TS, TI, TR>::bool_and_or_not_local_operation (bool is_and)
  : m_is_and (is_and)
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
OnEmptyIntruderHint
bool_and_or_not_local_operation<TS, TI, TR>::on_empty_intruder_hint () const
{
  return m_is_and ? Drop : Copy;
}

template <class TS, class TI, class TR>
std::string
bool_and_or_not_local_operation<TS, TI, TR>::description () const
{
  return m_is_and ? tl::to_string (tr ("AND operation")) : tl::to_string (tr ("NOT operation"));
}

template <class TS, class TI, class TR>
void
bool_and_or_not_local_operation<TS, TI, TR>::do_compute_local (db::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<TR> &result = results.front ();

  db::EdgeProcessor ep;

  size_t p1 = 0, p2 = 1;

  std::set<TI> others;
  for (auto i = interactions.begin (); i != interactions.end (); ++i) {
    for (auto j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  for (auto i = interactions.begin (); i != interactions.end (); ++i) {

    const TR &subject = interactions.subject_shape (i->first);
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
      for (auto e = subject.begin_edge (); ! e.at_end(); ++e) {
        ep.insert (*e, p1);
      }
      p1 += 2;
    }

  }

  if (! others.empty () && p1 > 0) {

    for (auto o = others.begin (); o != others.end (); ++o) {
      for (auto e = o->begin_edge (); ! e.at_end(); ++e) {
        ep.insert (*e, p2);
      }
      p2 += 2;
    }

    db::BooleanOp op (m_is_and ? db::BooleanOp::And : db::BooleanOp::ANotB);
    db::polygon_ref_generator<TR> pr (layout, result);
    db::PolygonSplitter splitter (pr, area_ratio, max_vertex_count);
    db::PolygonGenerator pg (splitter, true, true);
    ep.set_base_verbosity (50);
    ep.process (pg, op);

  }
}

template class DB_PUBLIC bool_and_or_not_local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC bool_and_or_not_local_operation<db::Polygon, db::Polygon, db::Polygon>;

// ---------------------------------------------------------------------------------------------
//  BoolAndOrNotLocalOperationWithProperties implementation

template <class TS, class TI, class TR>
bool_and_or_not_local_operation_with_properties<TS, TI, TR>::bool_and_or_not_local_operation_with_properties (bool is_and, db::PropertiesRepository *target_pr, const db::PropertiesRepository *subject_pr, const db::PropertiesRepository *intruder_pr, db::PropertyConstraint property_constraint)
  : m_is_and (is_and), m_property_constraint (property_constraint), m_pms (target_pr, subject_pr), m_pmi (target_pr, intruder_pr)
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
OnEmptyIntruderHint
bool_and_or_not_local_operation_with_properties<TS, TI, TR>::on_empty_intruder_hint () const
{
  return m_is_and ? Drop : Copy;
}

template <class TS, class TI, class TR>
std::string
bool_and_or_not_local_operation_with_properties<TS, TI, TR>::description () const
{
  return m_is_and ? tl::to_string (tr ("AND operation")) : tl::to_string (tr ("NOT operation"));
}

template <class TS, class TI, class TR>
void
bool_and_or_not_local_operation_with_properties<TS, TI, TR>::do_compute_local (db::Layout *layout, const shape_interactions<db::object_with_properties<TS>, db::object_with_properties<TI> > &interactions, std::vector<std::unordered_set<db::object_with_properties<TR> > > &results, size_t max_vertex_count, double area_ratio) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<db::object_with_properties<TR> > &result = results.front ();

  db::EdgeProcessor ep;

  std::map<db::properties_id_type, std::pair<tl::slist<TS>, std::set<TI> > > by_prop_id;

  for (auto i = interactions.begin (); i != interactions.end (); ++i) {

    const db::object_with_properties<TS> &subject = interactions.subject_shape (i->first);

    if (i->second.empty ()) {

      if (! m_is_and) {
        result.insert (db::object_with_properties<TR> (subject, m_pms (subject.properties_id ())));
      }

    } else {

      db::properties_id_type prop_id_s = m_pms (subject.properties_id ());

      auto &shapes_by_prop = by_prop_id [prop_id_s];
      shapes_by_prop.first.push_front (subject);

      for (auto j = i->second.begin (); j != i->second.end (); ++j) {
        const db::object_with_properties<TI> &intruder = interactions.intruder_shape (*j).second;
        db::properties_id_type prop_id_i = (m_property_constraint != db::NoPropertyConstraint ? m_pmi (intruder.properties_id ()) : prop_id_s);
        if ((prop_id_i != prop_id_s) == (m_property_constraint == db::DifferentPropertiesConstraint)) {
          shapes_by_prop.second.insert (intruder);
        }
      }

    }

  }

  for (auto p2s = by_prop_id.begin (); p2s != by_prop_id.end (); ++p2s) {

    ep.clear ();
    size_t p1 = 0, p2 = 1;

    const std::set<TI> &others = p2s->second.second;
    db::properties_id_type prop_id = p2s->first;

    for (auto s = p2s->second.first.begin (); s != p2s->second.first.end (); ++s) {

      const TS &subject = *s;
      if (others.find (subject) != others.end ()) {
        if (m_is_and) {
          result.insert (db::object_with_properties<TR> (subject, prop_id));
        }
      } else if (others.empty ()) {
        //  shortcut (not: keep, and: drop)
        if (! m_is_and) {
          result.insert (db::object_with_properties<TR> (subject, prop_id));
        }
      } else {
        for (auto e = subject.begin_edge (); ! e.at_end(); ++e) {
          ep.insert (*e, p1);
        }
        p1 += 2;
      }

    }

    if (! others.empty () && p1 > 0) {

      for (auto o = others.begin (); o != others.end (); ++o) {
        for (auto e = o->begin_edge (); ! e.at_end(); ++e) {
          ep.insert (*e, p2);
        }
        p2 += 2;
      }

      db::BooleanOp op (m_is_and ? db::BooleanOp::And : db::BooleanOp::ANotB);
      db::polygon_ref_generator_with_properties<db::object_with_properties<TR> > pr (layout, result, prop_id);
      db::PolygonSplitter splitter (pr, area_ratio, max_vertex_count);
      db::PolygonGenerator pg (splitter, true, true);
      ep.set_base_verbosity (50);
      ep.process (pg, op);

    }

  }
}

template class DB_PUBLIC bool_and_or_not_local_operation_with_properties<db::PolygonRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC bool_and_or_not_local_operation_with_properties<db::Polygon, db::Polygon, db::Polygon>;

// ---------------------------------------------------------------------------------------------
//  TwoBoolAndNotLocalOperation implementation

template <class TS, class TI, class TR>
two_bool_and_not_local_operation<TS, TI, TR>::two_bool_and_not_local_operation ()
  : db::local_operation<TS, TI, TR> ()
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
void
two_bool_and_not_local_operation<TS, TI, TR>::do_compute_local (db::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t max_vertex_count, double area_ratio) const
{
  tl_assert (results.size () == 2);

  db::EdgeProcessor ep;

  std::unordered_set<TR> &result0 = results [0];
  std::unordered_set<TR> &result1 = results [1];

  size_t p1 = 0, p2 = 1;

  std::set<TI> others;
  for (auto i = interactions.begin (); i != interactions.end (); ++i) {
    for (auto j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  for (auto i = interactions.begin (); i != interactions.end (); ++i) {

    const TS &subject = interactions.subject_shape (i->first);
    if (others.find (subject) != others.end ()) {
      result0.insert (subject);
    } else if (i->second.empty ()) {
      //  shortcut (not: keep, and: drop)
      result1.insert (subject);
    } else {
      for (auto e = subject.begin_edge (); ! e.at_end(); ++e) {
        ep.insert (*e, p1);
      }
      p1 += 2;
    }

  }

  if (! others.empty () && p1 > 0) {

    for (auto o = others.begin (); o != others.end (); ++o) {
      for (auto e = o->begin_edge (); ! e.at_end(); ++e) {
        ep.insert (*e, p2);
      }
      p2 += 2;
    }

    db::BooleanOp op0 (db::BooleanOp::And);
    db::polygon_ref_generator<TR> pr0 (layout, result0);
    db::PolygonSplitter splitter0 (pr0, area_ratio, max_vertex_count);
    db::PolygonGenerator pg0 (splitter0, true, true);

    db::BooleanOp op1 (db::BooleanOp::ANotB);
    db::polygon_ref_generator<TR> pr1 (layout, result1);
    db::PolygonSplitter splitter1 (pr1, area_ratio, max_vertex_count);
    db::PolygonGenerator pg1 (splitter1, true, true);

    ep.set_base_verbosity (50);

    std::vector<std::pair<db::EdgeSink *, db::EdgeEvaluatorBase *> > procs;
    procs.push_back (std::make_pair (&pg0, &op0));
    procs.push_back (std::make_pair (&pg1, &op1));
    ep.process (procs);

  }

}

template <class TS, class TI, class TR>
std::string two_bool_and_not_local_operation<TS, TI, TR>::description () const
{
  return tl::to_string (tr ("ANDNOT operation"));
}

template class DB_PUBLIC two_bool_and_not_local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC two_bool_and_not_local_operation<db::Polygon, db::Polygon, db::Polygon>;

// ---------------------------------------------------------------------------------------------
//  TwoBoolAndNotLocalOperationWithProperties implementation

template <class TS, class TI, class TR>
two_bool_and_not_local_operation_with_properties<TS, TI, TR>::two_bool_and_not_local_operation_with_properties (db::PropertiesRepository *target1_pr, db::PropertiesRepository *target2_pr, const db::PropertiesRepository *subject_pr, const db::PropertiesRepository *intruder_pr, db::PropertyConstraint property_constraint)
  : db::local_operation<db::object_with_properties<TS>, db::object_with_properties<TI>, db::object_with_properties<TR> > (),
    m_property_constraint (property_constraint), m_pms (target1_pr, subject_pr), m_pmi (target1_pr, intruder_pr), m_pm12 (target2_pr, target1_pr)
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
void
two_bool_and_not_local_operation_with_properties<TS, TI, TR>::do_compute_local (db::Layout *layout, const shape_interactions<db::object_with_properties<TS>, db::object_with_properties<TI> > &interactions, std::vector<std::unordered_set<db::object_with_properties<TR> > > &results, size_t max_vertex_count, double area_ratio) const
{
  tl_assert (results.size () == 2);
  std::unordered_set<db::object_with_properties<TR> > &result0 = results [0];
  std::unordered_set<db::object_with_properties<TR> > &result1 = results [1];

  db::EdgeProcessor ep;

  std::map<db::properties_id_type, std::pair<tl::slist<TS>, std::set<TI> > > by_prop_id;

  for (auto i = interactions.begin (); i != interactions.end (); ++i) {

    const db::object_with_properties<TS> &subject = interactions.subject_shape (i->first);

    if (i->second.empty ()) {

      result1.insert (db::object_with_properties<TR> (subject, m_pms (subject.properties_id ())));

    } else {

      db::properties_id_type prop_id_s = m_pms (subject.properties_id ());

      auto &shapes_by_prop = by_prop_id [prop_id_s];
      shapes_by_prop.first.push_front (subject);

      for (auto j = i->second.begin (); j != i->second.end (); ++j) {
        const db::object_with_properties<TI> &intruder = interactions.intruder_shape (*j).second;
        db::properties_id_type prop_id_i = (m_property_constraint != db::NoPropertyConstraint ? m_pmi (intruder.properties_id ()) : prop_id_s);
        if ((prop_id_i != prop_id_s) == (m_property_constraint == db::DifferentPropertiesConstraint)) {
          shapes_by_prop.second.insert (intruder);
        }
      }

    }

  }

  for (auto p2s = by_prop_id.begin (); p2s != by_prop_id.end (); ++p2s) {

    ep.clear ();
    size_t p1 = 0, p2 = 1;

    const std::set<TR> &others = p2s->second.second;
    db::properties_id_type prop_id = p2s->first;

    for (auto s = p2s->second.first.begin (); s != p2s->second.first.end (); ++s) {

      const TR &subject = *s;
      if (others.find (subject) != others.end ()) {
        result0.insert (db::object_with_properties<TR> (subject, prop_id));
      } else if (others.empty ()) {
        //  shortcut (not: keep, and: drop)
        result1.insert (db::object_with_properties<TR> (subject, m_pm12 (prop_id)));
      } else {
        for (auto e = subject.begin_edge (); ! e.at_end(); ++e) {
          ep.insert (*e, p1);
        }
        p1 += 2;
      }

    }

    if (! others.empty () && p1 > 0) {

      for (auto o = others.begin (); o != others.end (); ++o) {
        for (auto e = o->begin_edge (); ! e.at_end(); ++e) {
          ep.insert (*e, p2);
        }
        p2 += 2;
      }

      std::unordered_set<TR> result0_wo_props;
      std::unordered_set<TR> result1_wo_props;

      db::BooleanOp op0 (db::BooleanOp::And);
      db::polygon_ref_generator<TR> pr0 (layout, result0_wo_props);
      db::PolygonSplitter splitter0 (pr0, area_ratio, max_vertex_count);
      db::PolygonGenerator pg0 (splitter0, true, true);

      db::BooleanOp op1 (db::BooleanOp::ANotB);
      db::polygon_ref_generator<TR> pr1 (layout, result1_wo_props);
      db::PolygonSplitter splitter1 (pr1, area_ratio, max_vertex_count);
      db::PolygonGenerator pg1 (splitter1, true, true);

      ep.set_base_verbosity (50);

      std::vector<std::pair<db::EdgeSink *, db::EdgeEvaluatorBase *> > procs;
      procs.push_back (std::make_pair (&pg0, &op0));
      procs.push_back (std::make_pair (&pg1, &op1));
      ep.process (procs);

      for (auto r = result0_wo_props.begin (); r != result0_wo_props.end (); ++r) {
        result0.insert (db::object_with_properties<TR> (*r, prop_id));
      }
      for (auto r = result1_wo_props.begin (); r != result1_wo_props.end (); ++r) {
        result1.insert (db::object_with_properties<TR> (*r, m_pm12 (prop_id)));
      }

    }

  }
}

template <class TS, class TI, class TR>
std::string two_bool_and_not_local_operation_with_properties<TS, TI, TR>::description () const
{
  return tl::to_string (tr ("ANDNOT operation"));
}

template class DB_PUBLIC two_bool_and_not_local_operation_with_properties<db::PolygonRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC two_bool_and_not_local_operation_with_properties<db::Polygon, db::Polygon, db::Polygon>;

// ---------------------------------------------------------------------------------------------

SelfOverlapMergeLocalOperation::SelfOverlapMergeLocalOperation (unsigned int wrap_count)
  : m_wrap_count (wrap_count)
{
  //  .. nothing yet ..
}

void
SelfOverlapMergeLocalOperation::do_compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<db::PolygonRef> &result = results.front ();

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
        const db::PolygonRef &intruder = interactions.intruder_shape (*o).second;
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

OnEmptyIntruderHint SelfOverlapMergeLocalOperation::on_empty_intruder_hint () const
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
EdgeBoolAndOrNotLocalOperation::do_compute_local (db::Layout * /*layout*/, const shape_interactions<db::Edge, db::Edge> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
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
EdgeToPolygonLocalOperation::do_compute_local (db::Layout * /*layout*/, const shape_interactions<db::Edge, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
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

