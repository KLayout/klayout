
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#include "dbEdgePairsLocalOperations.h"
#include "dbHierProcessor.h"
#include "dbLocalOperationUtils.h"

namespace db
{

// ---------------------------------------------------------------------------------------------
//  EdgePair2EdgeInteractingLocalOperation implementation

EdgePair2EdgeInteractingLocalOperation::EdgePair2EdgeInteractingLocalOperation (output_mode_t output_mode, size_t min_count, size_t max_count)
  : m_output_mode (output_mode), m_min_count (min_count), m_max_count (max_count)
{
  //  .. nothing yet ..
}

db::Coord EdgePair2EdgeInteractingLocalOperation::dist () const
{
  //  touching is sufficient
  return 1;
}

void EdgePair2EdgeInteractingLocalOperation::do_compute_local (db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::EdgePair, db::Edge> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase * /*proc*/) const
{
  tl_assert (results.size () == size_t (m_output_mode == Both ? 2 : 1));

  std::unordered_set<db::EdgePair> &result = results.front ();

  std::unordered_set<db::EdgePair> *result2 = 0;
  if (m_output_mode == Both) {
    result2 = &results[1];
  }

  db::box_scanner2<db::EdgePair, size_t, db::Edge, size_t> scanner;

  std::set<db::Edge> others;
  for (typename shape_interactions<db::EdgePair, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (typename shape_interactions<db::EdgePair, db::Edge>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  for (typename shape_interactions<db::EdgePair, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    const db::EdgePair &subject = interactions.subject_shape (i->first);
    scanner.insert1 (&subject, 0);
  }

  for (typename std::set<db::Edge>::const_iterator o = others.begin (); o != others.end (); ++o) {
    scanner.insert2 (o.operator-> (), 1);
  }

  if (m_output_mode == Inverse || m_output_mode == Both) {

    std::unordered_set<db::EdgePair> interacting;
    edge_pair_to_edge_interaction_filter<std::unordered_set<db::EdgePair> > filter (&interacting, m_min_count, m_max_count);
    scanner.process (filter, 1, db::box_convert<db::EdgePair> (), db::box_convert<db::Edge> ());

    for (typename shape_interactions<db::EdgePair, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

      const db::EdgePair &subject = interactions.subject_shape (i->first);

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

    edge_pair_to_edge_interaction_filter<std::unordered_set<db::EdgePair> > filter (&result, m_min_count, m_max_count);
    scanner.process (filter, 1, db::box_convert<db::EdgePair> (), db::box_convert<db::Edge> ());

  }
}

OnEmptyIntruderHint EdgePair2EdgeInteractingLocalOperation::on_empty_intruder_hint () const
{
  return m_output_mode == Both ? CopyToSecond : (m_output_mode == Inverse ? Copy : Drop);
}

std::string EdgePair2EdgeInteractingLocalOperation::description () const
{
  return tl::to_string (tr ("Select edge pairs interacting edges"));
}

// ---------------------------------------------------------------------------------------------
//  Edge2EdgePullLocalOperation implementation

EdgePair2EdgePullLocalOperation::EdgePair2EdgePullLocalOperation ()
{
  //  .. nothing yet ..
}

db::Coord EdgePair2EdgePullLocalOperation::dist () const
{
  //  touching is sufficient
  return 1;
}

void EdgePair2EdgePullLocalOperation::do_compute_local (db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::EdgePair, db::Edge> &interactions, std::vector<std::unordered_set<db::Edge> > &results, const db::LocalProcessorBase * /*proc*/) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<db::Edge> &result = results.front ();

  db::box_scanner2<db::EdgePair, size_t, db::Edge, size_t> scanner;

  std::set<db::Edge> others;
  for (shape_interactions<db::EdgePair, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (shape_interactions<db::EdgePair, db::Edge>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  for (shape_interactions<db::EdgePair, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    const db::EdgePair &subject = interactions.subject_shape (i->first);
    scanner.insert1 (&subject, 1);
  }

  for (std::set<db::Edge>::const_iterator o = others.begin (); o != others.end (); ++o) {
    scanner.insert2 (o.operator-> (), 0);
  }

  edge_pair_to_edge_interaction_filter<std::unordered_set<db::Edge> > filter (&result, size_t (1), std::numeric_limits<size_t>::max ());
  scanner.process (filter, 1, db::box_convert<db::EdgePair> (), db::box_convert<db::Edge> ());
}

OnEmptyIntruderHint EdgePair2EdgePullLocalOperation::on_empty_intruder_hint () const
{
  return Drop;
}

std::string EdgePair2EdgePullLocalOperation::description () const
{
  return tl::to_string (tr ("Select interacting edges from other"));
}

// ---------------------------------------------------------------------------------------------
//  edge_to_polygon_interacting_local_operation implementation

template <class TI>
edge_pair_to_polygon_interacting_local_operation<TI>::edge_pair_to_polygon_interacting_local_operation (EdgePairInteractionMode mode, output_mode_t output_mode, size_t min_count, size_t max_count)
  : m_mode (mode), m_output_mode (output_mode), m_min_count (min_count), m_max_count (max_count)
{
  //  .. nothing yet ..
}

template <class TI>
db::Coord edge_pair_to_polygon_interacting_local_operation<TI>::dist () const
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
void edge_pair_to_polygon_interacting_local_operation<TI>::do_compute_local (db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::EdgePair, TI> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase * /*proc*/) const
{
  tl_assert (results.size () == size_t (m_output_mode == Both ? 2 : 1));

  std::unordered_set<db::EdgePair> &result = results.front ();

  std::unordered_set<db::EdgePair> *result2 = 0;
  if (m_output_mode == Both) {
    result2 = &results[1];
  }

  db::box_scanner2<db::EdgePair, size_t, db::Polygon, size_t> scanner;

  std::set<TI> others;
  for (typename shape_interactions<db::EdgePair, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (typename shape_interactions<db::EdgePair, TI>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  for (typename shape_interactions<db::EdgePair, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    const db::EdgePair &subject = interactions.subject_shape (i->first);
    scanner.insert1 (&subject, 0);
  }

  std::list<db::Polygon> heap;
  for (typename std::set<TI>::const_iterator o = others.begin (); o != others.end (); ++o) {
    scanner.insert2 (deref (*o, heap), 1);
  }

  if (m_output_mode == Inverse || m_output_mode == Both) {

    std::unordered_set<db::EdgePair> interacting;
    edge_pair_to_polygon_interaction_filter<std::unordered_set<db::EdgePair> > filter (&interacting, m_mode, m_min_count, m_max_count);
    scanner.process (filter, 1, db::box_convert<db::EdgePair> (), db::box_convert<db::Polygon> ());

    for (typename shape_interactions<db::EdgePair, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

      const db::EdgePair &subject = interactions.subject_shape (i->first);

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

    edge_pair_to_polygon_interaction_filter<std::unordered_set<db::EdgePair> > filter (&result, m_mode, m_min_count, m_max_count);
    scanner.process (filter, 1, db::box_convert<db::EdgePair> (), db::box_convert<db::Polygon> ());

  }
}

template <class TI>
OnEmptyIntruderHint edge_pair_to_polygon_interacting_local_operation<TI>::on_empty_intruder_hint () const
{
  if (m_mode == EdgePairsOutside) {
    return m_output_mode == Both ? Copy : (m_output_mode == Inverse ? Drop : Copy);
  } else {
    return m_output_mode == Both ? CopyToSecond : (m_output_mode == Inverse ? Copy : Drop);
  }
}

template <class TI>
std::string edge_pair_to_polygon_interacting_local_operation<TI>::description () const
{
  if (m_mode == EdgePairsInteract) {
    if (m_output_mode == Inverse) {
      return tl::to_string (tr ("Select non-interacting edge pairs"));
    } else if (m_output_mode == Normal) {
      return tl::to_string (tr ("Select interacting edge pairs"));
    } else {
      return tl::to_string (tr ("Select interacting and non-interacting edge pairs"));
    }
  } else if (m_mode == EdgePairsInside) {
    if (m_output_mode == Inverse) {
      return tl::to_string (tr ("Select non-inside edge pairs"));
    } else if (m_output_mode == Normal) {
      return tl::to_string (tr ("Select inside edge pairs"));
    } else {
      return tl::to_string (tr ("Select inside and non-inside edge pairs"));
    }
  } else if (m_mode == EdgePairsOutside) {
    if (m_output_mode == Inverse) {
      return tl::to_string (tr ("Select non-outside edge pairs"));
    } else if (m_output_mode == Normal) {
      return tl::to_string (tr ("Select outside edge pairs"));
    } else {
      return tl::to_string (tr ("Select outside and non-outside edge pairs"));
    }
  }
  return std::string ();
}

template class edge_pair_to_polygon_interacting_local_operation<db::Polygon>;
template class edge_pair_to_polygon_interacting_local_operation<db::PolygonRef>;

// ---------------------------------------------------------------------------------------------
//  EdgePair2PolygonPullLocalOperation implementation

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

EdgePair2PolygonPullLocalOperation::EdgePair2PolygonPullLocalOperation ()
{
  //  .. nothing yet ..
}

db::Coord EdgePair2PolygonPullLocalOperation::dist () const
{
  //  touching is sufficient
  return 1;
}

void EdgePair2PolygonPullLocalOperation::do_compute_local (db::Layout *layout, db::Cell * /*cell*/, const shape_interactions<db::EdgePair, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase * /*proc*/) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<db::PolygonRef> &result = results.front ();

  db::box_scanner2<db::EdgePair, size_t, db::Polygon, size_t> scanner;

  std::set<db::PolygonRef> others;
  for (shape_interactions<db::EdgePair, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (shape_interactions<db::EdgePair, db::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  for (shape_interactions<db::EdgePair, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    const db::EdgePair &subject = interactions.subject_shape (i->first);
    scanner.insert1 (&subject, 1);
  }

  std::list<db::Polygon> heap;
  for (std::set<db::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o) {
    heap.push_back (o->obj ().transformed (o->trans ()));
    scanner.insert2 (& heap.back (), 0);
  }

  ResultInserter inserter (layout, result);
  edge_pair_to_polygon_interaction_filter<ResultInserter> filter (&inserter, EdgePairsInteract, size_t (1), std::numeric_limits<size_t>::max ());
  scanner.process (filter, 1, db::box_convert<db::EdgePair> (), db::box_convert<db::Polygon> ());
}

OnEmptyIntruderHint EdgePair2PolygonPullLocalOperation::on_empty_intruder_hint () const
{
  return Drop;
}

std::string EdgePair2PolygonPullLocalOperation::description () const
{
  return tl::to_string (tr ("Select interacting polygons"));
}

}

