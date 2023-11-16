
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


#include "dbRegionLocalOperations.h"
#include "dbRegionUtils.h"
#include "dbLocalOperationUtils.h"
#include "dbHierProcessor.h"
#include "dbEdgeBoolean.h"

namespace db
{

namespace
{

// ---------------------------------------------------------------------------------------------------------------

static inline const db::Polygon *push_polygon_to_heap (db::Layout *, const db::Polygon &p, std::list<db::Polygon> &)
{
  return &p;
}

static inline const db::PolygonRef *push_polygon_to_heap (db::Layout *layout, const db::PolygonRef &p, std::list<db::PolygonRef> &heap)
{
  db::PolygonRef ref = db::PolygonRef (p, layout->shape_repository ());
  heap.push_back (ref);
  return &heap.back ();
}

template <class TR>
struct result_counting_inserter
{
  typedef TR value_type;

  result_counting_inserter (std::unordered_map<TR, size_t> &result)
    : mp_result (&result)
  {
    //  .. nothing yet ..
  }

  void insert (const TR &p)
  {
    (*mp_result)[p] += 1;
  }

  void init (const TR &p)
  {
    (*mp_result)[p] = 0;
  }

private:
  std::unordered_map<TR, size_t> *mp_result;
};

template <class TR>
struct simple_result_inserter
{
  typedef TR value_type;

  simple_result_inserter<TR> (std::unordered_set<TR> &result)
    : mp_result (&result)
  {
    //  .. nothing yet ..
  }

  void insert (const TR &e)
  {
    (*mp_result).insert (e);
  }

private:
  std::unordered_set<TR> *mp_result;
};

}

// ---------------------------------------------------------------------------------------------------------------

namespace
{

static inline bool shields_interaction (const db::EdgePair &ep, const db::Edge &q)
{
  db::Edge pe1 (ep.first ().p1 (), ep.second ().p2 ());
  db::Edge pe2 (ep.second ().p1 (), ep.first ().p2 ());

  std::pair<bool, db::Point> ip1 = pe1.intersect_point (q);
  std::pair<bool, db::Point> ip2 = pe2.intersect_point (q);

  if (ip1.first && ip2.first && ip1.second != pe1.p1 () && ip1.second != pe1.p2 () && ip2.second != pe2.p1 () && ip2.second != pe2.p2 ()) {
    return ip1.second != ip2.second || (pe1.side_of (q.p1 ()) != 0 && pe2.side_of (q.p2 ()) != 0);
  } else {
    return false;
  }
}

template <class P>
static bool shields_interaction (const db::EdgePair &ep, const P &poly)
{
  for (typename P::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end (); ++e) {
    if (shields_interaction (ep, *e)) {
      return true;
    }
  }
  return false;
}

template <class T, class S>
void insert_into_hash (std::unordered_set<T> &, const S &)
{
  tl_assert (false);
}

template <class T>
void insert_into_hash (std::unordered_set<T> &hash, const T &shape)
{
  hash.insert (shape);
}

template <class TS>
static
uint32_t compute_error_pattern (const TS &subject, std::unordered_set<db::EdgePair> &result, std::map<db::Edge, uint32_t> &edges_with_errors)
{
  uint32_t p = 1;
  for (typename TS::polygon_edge_iterator e = subject.begin_edge (); ! e.at_end (); ++e) {
    edges_with_errors [*e] = p;
    p <<= 1;
  }

  uint32_t error_pattern = 0;
  for (std::unordered_set<db::EdgePair>::const_iterator ep = result.begin (); ep != result.end (); ++ep) {
    std::map<db::Edge, unsigned int>::iterator i = edges_with_errors.find (ep->first ());
    if (i != edges_with_errors.end ()) {
      if ((error_pattern & i->second) == 0) {
        error_pattern |= i->second;
      }
    }
  }

  return error_pattern;
}

static bool rect_filter_can_be_waived (uint32_t error_pattern, uint32_t rect_filter)
{
  if (! error_pattern) {
    return false;
  }

  bool can_be_waived = false;

  //  decode pattern: consider each group of 4 bits and match them against the error pattern in their four rotation variants
  uint32_t p32 = (uint32_t) rect_filter;
  while (p32 != 0 && ! can_be_waived) {

    uint32_t p4 = p32 & 0xf;
    p32 >>= 4;

    if (p4 > 0) {
      for (unsigned int r = 0; r < 4 && ! can_be_waived; ++r) {
        can_be_waived = (error_pattern == p4);
        p4 = ((p4 << 1) & 0xf) | ((p4 & 0x8) >> 3);
      }
    }

  }

  return can_be_waived;
}

}


template <class TS, class TI>
check_local_operation_base<TS, TI>::check_local_operation_base (const EdgeRelationFilter &check, bool different_polygons, bool is_merged, bool has_other, bool other_is_merged, const db::RegionCheckOptions &options)
  : m_check (check), m_different_polygons (different_polygons), m_is_merged (is_merged), m_has_other (has_other), m_other_is_merged (other_is_merged), m_options (options)
{
  //  .. nothing yet ..
}

template <class TS, class TI>
void
check_local_operation_base<TS, TI>::compute_results (db::Layout *layout, db::Cell *subject_cell, const std::vector<const TS *> &subjects, const std::set<const TI *> &intruders, std::unordered_set<db::EdgePair> &result, std::unordered_set<db::EdgePair> &intra_polygon_result, const db::LocalProcessorBase *proc) const
{
  //  NOTE: the rectangle and opposite filters are unsymmetric
  bool symmetric_edge_pairs = ! m_has_other && m_options.opposite_filter == db::NoOppositeFilter && m_options.rect_filter == RectFilter::NoRectFilter;

  //  modify the check to take into account scaled cells
  EdgeRelationFilter check = m_check;
  check.set_distance (proc->dist_for_cell (subject_cell, check.distance ()));

  edge2edge_check_negative_or_positive<std::unordered_set<db::EdgePair> > edge_check (check, result, intra_polygon_result, m_options.negative, m_different_polygons, m_has_other, m_options.shielded, symmetric_edge_pairs);
  poly2poly_check<TS> poly_check (edge_check);

  std::unordered_set<TI> polygons;
  std::unordered_set<TS> spolygons;

  db::EdgeProcessor ep;
  ep.set_base_verbosity (50);

  bool take_all = edge_check.has_negative_edge_output () || intruders.empty ();

  db::Box common_box;
  if (! take_all) {

    db::Vector e (edge_check.distance (), edge_check.distance ());

    db::Box subject_box;
    for (auto i = subjects.begin (); i != subjects.end (); ++i) {
      subject_box += db::box_convert<TS> () (**i);
    }

    if (edge_check.requires_different_layers ()) {
      db::Box intruder_box;
      for (auto i = intruders.begin (); i != intruders.end (); ++i) {
        intruder_box += db::box_convert<TI> () (**i);
      }
      common_box = subject_box.enlarged (e) & intruder_box.enlarged (e);
    } else {
      common_box = subject_box.enlarged (e);
    }

  }

  if (m_has_other) {

    size_t n = 0;

    if (m_is_merged || (subjects.size () == 1 && subjects.front ()->is_box ())) {

      for (auto i = subjects.begin (); i != subjects.end (); ++i) {
        if (! take_all) {
          poly_check.enter (**i, n, common_box);
        } else {
          poly_check.enter (**i, n);
        }
        n += 2;
      }

    } else {

      //  merge needed for the subject shapes

      ep.clear ();
      size_t nn = 0;

      for (auto i = subjects.begin (); i != subjects.end (); ++i) {
        for (auto e = (*i)->begin_edge (); ! e.at_end (); ++e) {
          ep.insert (*e, nn);
        }
        ++nn;
      }

      spolygons.clear ();

      db::polygon_ref_generator<TS> ps (layout, spolygons);
      db::PolygonGenerator pg (ps, false /*don't resolve holes*/, false);
      db::SimpleMerge op (1 /*wc>0*/);
      ep.process (pg, op);

      for (auto o = spolygons.begin (); o != spolygons.end (); ++o) {
        if (! take_all) {
          poly_check.enter (*o, n, common_box);
        } else {
          poly_check.enter (*o, n);
        }
        n += 2;
      }

    }

    //  merge the intruders to remove inner edges

    if (intruders.empty ()) {

      //  empty intruders

    } else if (! m_other_is_merged && (intruders.size () > 1 || ! (*intruders.begin ())->is_box ())) {

      //  NOTE: this local merge is not necessarily giving the same results than a global merge before running
      //  the processor. Reason: the search range is limited, hence not all necessary components may have been
      //  captured.

      ep.clear ();
      size_t nn = 0;

      for (auto i = intruders.begin (); i != intruders.end (); ++i) {
        for (auto e = (*i)->begin_edge (); ! e.at_end (); ++e) {
          ep.insert (*e, nn);
        }
        ++nn;
      }

      //  TODO: Use edges directly
      polygons.clear ();

      db::polygon_ref_generator<TI> ps (layout, polygons);
      db::PolygonGenerator pg (ps, false /*don't resolve holes*/, false);
      db::SimpleMerge op (1 /*wc>0*/);
      ep.process (pg, op);

      n = 1;
      for (auto o = polygons.begin (); o != polygons.end (); ++o) {
        if (! take_all) {
          poly_check.enter (*o, n, common_box);
        } else {
          poly_check.enter (*o, n);
        }
        n += 2;
      }

    } else {

      n = 1;
      for (auto i = intruders.begin (); i != intruders.end (); ++i) {
        if (! take_all) {
          poly_check.enter (**i, n, common_box);
        } else {
          poly_check.enter (**i, n);
        }
        n += 2;
      }

    }

  } else {

    if (m_is_merged || (subjects.size () == 1 && intruders.empty () && subjects.front ()->is_box ())) {

      //  no merge required

      //  NOTE: we need to eliminate identical shapes from intruders and subjects because those will shield

      size_t n = 0;
      std::unordered_set<TI> subjects_hash;

      for (auto i = subjects.begin (); i != subjects.end (); ++i) {
        //  we can't directly insert because TS may be != TI
        insert_into_hash (subjects_hash, **i);
        if (! take_all) {
          poly_check.enter (**i, n, common_box);
        } else {
          poly_check.enter (**i, n);
        }
        n += 2;
      }

      n = 1;

      for (auto i = intruders.begin (); i != intruders.end (); ++i) {
        if (subjects_hash.find (**i) == subjects_hash.end ()) {
          if (! take_all) {
            poly_check.enter (**i, n, common_box);
          } else {
            poly_check.enter (**i, n);
          }
        }
      }

    } else if (intruders.empty ()) {

      //  merge needed for the subject shapes - no intruders present so this is the simple case

      size_t n = 0;

      ep.clear ();
      size_t nn = 0;

      for (auto i = subjects.begin (); i != subjects.end (); ++i) {
        for (auto e = (*i)->begin_edge (); ! e.at_end (); ++e) {
          ep.insert (*e, nn);
        }
        ++nn;
      }

      //  TODO: Use edges directly
      spolygons.clear ();

      db::polygon_ref_generator<TS> ps (layout, spolygons);
      db::PolygonGenerator pg (ps, false /*don't resolve holes*/, false);
      db::SimpleMerge op (1 /*wc>0*/);
      ep.process (pg, op);

      for (auto o = spolygons.begin (); o != spolygons.end (); ++o) {
        if (! take_all) {
          poly_check.enter (*o, n, common_box);
        } else {
          poly_check.enter (*o, n);
        }
        n += 2;
      }

    } else {

      //  merge needed for the subject and intruder shapes - we merge both and then
      //  separate edges into those from the subject and those from intruder shapes.

      ep.clear ();
      size_t nn = 0;

      for (auto i = subjects.begin (); i != subjects.end (); ++i) {
        for (auto e = (*i)->begin_edge (); ! e.at_end (); ++e) {
          ep.insert (*e, nn);
        }
        ++nn;
      }

      for (auto i = intruders.begin (); i != intruders.end (); ++i) {
        for (auto e = (*i)->begin_edge (); ! e.at_end (); ++e) {
          ep.insert (*e, nn);
        }
        ++nn;
      }

      std::vector<typename TS::edge_type> edges;

      db::EdgeContainer ee (edges);
      db::SimpleMerge op (1 /*wc>0*/);
      ep.process (ee, op);
      ep.clear ();

      std::vector<typename TS::edge_type> subject_edges;

      size_t sz = 0;
      for (auto i = subjects.begin (); i != subjects.end (); ++i) {
        sz += (*i)->vertices ();
      }

      subject_edges.reserve (sz);

      for (auto i = subjects.begin (); i != subjects.end (); ++i) {
        for (auto e = (*i)->begin_edge (); ! e.at_end (); ++e) {
          subject_edges.push_back (*e);
        }
      }

      for (size_t n = 0; n <= 1; ++n) {

        std::set<typename TI::edge_type> partial_edges;

        EdgeBooleanClusterCollector<std::set<typename TI::edge_type> > cluster_collector (&partial_edges, n == 0 ? db::EdgeAnd : db::EdgeNot);

        db::box_scanner<typename TI::edge_type, size_t> scanner;
        scanner.reserve (edges.size () + subject_edges.size ());

        for (auto i = edges.begin (); i != edges.end (); ++i) {
          if (! i->is_degenerate ()) {
            scanner.insert (i.operator-> (), 0);
          }
        }

        for (auto i = subject_edges.begin (); i != subject_edges.end (); ++i) {
          if (! i->is_degenerate ()) {
            scanner.insert (i.operator-> (), 1);
          }
        }

        scanner.process (cluster_collector, 1, db::box_convert<typename TI::edge_type> ());

        for (auto e = partial_edges.begin (); e != partial_edges.end (); ++e) {
          if (! take_all) {
            poly_check.enter (*e, n, common_box);
          } else {
            poly_check.enter (*e, n);
          }
        }

      }

    }

  }

  do {
    poly_check.process ();
  } while (edge_check.prepare_next_pass ());
}

template <class TS, class TI>
void
check_local_operation_base<TS, TI>::apply_opposite_filter (const std::vector<const TS *> &subjects, std::unordered_set<db::EdgePair> &result, std::unordered_set<db::EdgePair> &intra_polygon_result) const
{
  db::EdgeRelationFilter opp (db::WidthRelation, std::numeric_limits<db::EdgeRelationFilter::distance_type>::max (), db::Projection);

  std::unordered_set<db::EdgePair> cleaned_result;

  if (m_has_other) {

    tl_assert (intra_polygon_result.empty ());

    //  filter out opposite edges: this is the case of two-layer checks where we can maintain the edge pairs but
    //  strip them of the filtered-out part.

    std::vector<db::Edge> projections;
    for (std::unordered_set<db::EdgePair>::const_iterator ep1 = result.begin (); ep1 != result.end (); ++ep1) {

      projections.clear ();

      for (std::unordered_set<db::EdgePair>::const_iterator ep2 = result.begin (); ep2 != result.end (); ++ep2) {

        if (ep1 == ep2) {
          continue;
        }

        db::EdgePair ep_opp;
        if (opp.check (ep1->first (), ep2->first (), &ep_opp)) {

          bool shielded = false;
          for (auto i = subjects.begin (); i != subjects.end () && ! shielded; ++i) {
            shielded = shields_interaction (ep_opp, **i);
          }

          if (! shielded) {
            projections.push_back (ep_opp.first ());
          }

        }

      }

      if (! projections.empty ()) {
        db::Edges ce;
        if (m_options.opposite_filter == db::OnlyOpposite) {
          ce = db::Edges (ep1->first ()) & db::Edges (projections.begin (), projections.end ());
        } else if (m_options.opposite_filter == db::NotOpposite) {
          ce = db::Edges (ep1->first ()) - db::Edges (projections.begin (), projections.end ());
        }
        for (db::Edges::const_iterator re = ce.begin (); ! re.at_end (); ++re) {
          cleaned_result.insert (db::EdgePair (*re, ep1->second ()));
        }
      } else if (m_options.opposite_filter == db::NotOpposite) {
        cleaned_result.insert (*ep1);
      }

    }

  } else {

    //  this is the single-layer case where we cannot maintain the edge pairs as we don't know how the
    //  other side will be filtered. For the filtering we only need the first edges and both edges of the
    //  intra-polygon checks

    std::unordered_set<db::Edge> edges;

    for (std::unordered_set<db::EdgePair>::const_iterator ep = result.begin (); ep != result.end (); ++ep) {
      edges.insert (ep->first ());
    }

    for (std::unordered_set<db::EdgePair>::const_iterator ep = intra_polygon_result.begin (); ep != intra_polygon_result.end (); ++ep) {
      edges.insert (ep->first ());
      edges.insert (ep->second ());
    }

    //  filter out opposite edges
    std::vector<db::Edge> projections;
    for (std::unordered_set<db::Edge>::const_iterator e1 = edges.begin (); e1 != edges.end (); ++e1) {

      projections.clear ();

      for (std::unordered_set<db::Edge>::const_iterator e2 = edges.begin (); e2 != edges.end (); ++e2) {

        if (e1 == e2) {
          continue;
        }

        db::EdgePair ep_opp;
        if (opp.check (*e1, *e2, &ep_opp)) {

          bool shielded = false;
          for (auto i = subjects.begin (); i != subjects.end () && ! shielded; ++i) {
            shielded = shields_interaction (ep_opp, **i);
          }

          if (! shielded) {
            projections.push_back (ep_opp.first ());
          }

        }

      }

      if (! projections.empty ()) {
        db::Edges ce;
        if (m_options.opposite_filter == db::OnlyOpposite) {
          ce = db::Edges (*e1) & db::Edges (projections.begin (), projections.end ());
        } else if (m_options.opposite_filter == db::NotOpposite) {
          ce = db::Edges (*e1) - db::Edges (projections.begin (), projections.end ());
        }
        for (db::Edges::const_iterator re = ce.begin (); ! re.at_end (); ++re) {
          cleaned_result.insert (db::EdgePair (*re, re->swapped_points ()));
        }
      } else if (m_options.opposite_filter == db::NotOpposite) {
        cleaned_result.insert (db::EdgePair (*e1, e1->swapped_points ()));
      }

    }

  }

  result.swap (cleaned_result);
}

template <class TS, class TI>
void
check_local_operation_base<TS, TI>::apply_rectangle_filter (const std::vector<const TS *> &subjects, std::unordered_set<db::EdgePair> &result) const
{
  std::unordered_set<db::EdgePair> waived;

  for (auto i = subjects.begin (); i != subjects.end (); ++i) {

    const TS &subject = **i;
    if (! subject.is_box ()) {
      continue;
    }

    std::map<db::Edge, uint32_t> edges_with_errors;
    unsigned int error_pattern = compute_error_pattern (subject, result, edges_with_errors);

    if (rect_filter_can_be_waived (error_pattern, (uint32_t) m_options.rect_filter)) {

      for (std::unordered_set<db::EdgePair>::const_iterator ep = result.begin (); ep != result.end (); ++ep) {
        if (edges_with_errors.find (ep->first ()) != edges_with_errors.end ()) {
          waived.insert (*ep);
        }
      }

    }

  }

  if (! waived.empty ()) {
    for (std::unordered_set<db::EdgePair>::const_iterator i = waived.begin (); i != waived.end (); ++i) {
      result.erase (*i);
    }
  }

  if (! m_has_other) {

    //  this is the case of single-layer interaction. We need to separate the results
    //  from edge pairs into single edges (basically returning the first edge only)
    //  Reasoning: we cannot say what's going to happen on the other side of the
    //  error - it may not be waived and we cannot waive half of an edge pair.

    std::unordered_set<db::EdgePair> filtered;
    for (std::unordered_set<db::EdgePair>::const_iterator i = result.begin (); i != result.end (); ++i) {
      filtered.insert (db::EdgePair (i->first (), i->first ().swapped_points ()));
    }

    result.swap (filtered);

  }
}

// ---------------------------------------------------------------------------------------------------------------

template <class TS, class TI>
check_local_operation<TS, TI>::check_local_operation (const EdgeRelationFilter &check, bool different_polygons, bool is_merged, bool has_other, bool other_is_merged, const db::RegionCheckOptions &options)
  : check_local_operation_base<TS, TI> (check, different_polygons, is_merged, has_other, other_is_merged, options)
{
  //  .. nothing yet ..
}

template <class TS, class TI>
void
check_local_operation<TS, TI>::do_compute_local (db::Layout *layout, db::Cell *subject_cell, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, const db::LocalProcessorBase *proc) const
{
  std::vector<const TS *> subjects;
  subjects.reserve (interactions.size ());

  std::set<const TI *> intruders;

  for (auto i = interactions.begin (); i != interactions.end (); ++i) {
    subjects.push_back (&interactions.subject_shape (i->first));
    for (auto ii = i->second.begin (); ii != i->second.end (); ++ii) {
      intruders.insert (&interactions.intruder_shape (*ii).second);
    }
  }

  tl_assert (results.size () == 1);

  std::unordered_set<db::EdgePair> result, intra_polygon_result;

  //  perform the basic check
  check_local_operation_base<TS, TI>::compute_results (layout, subject_cell, subjects, intruders, result, intra_polygon_result, proc);

  //  detect and remove parts of the result which have or do not have results "opposite"
  //  ("opposite" is defined by the projection of edges "through" the subject shape)
  if (check_local_operation_base<TS, TI>::m_options.opposite_filter != db::NoOppositeFilter && (! result.empty () || ! intra_polygon_result.empty ())) {
    check_local_operation_base<TS, TI>::apply_opposite_filter (subjects, result, intra_polygon_result);
  } else {
    result.insert (intra_polygon_result.begin (), intra_polygon_result.end ());
  }

  //  implements error filtering on rectangles
  if (check_local_operation_base<TS, TI>::m_options.rect_filter != RectFilter::NoRectFilter && ! result.empty ()) {
    check_local_operation_base<TS, TI>::apply_rectangle_filter (subjects, result);
  }

  results.front ().insert (result.begin (), result.end ());
}


template <class TS, class TI>
db::Coord
check_local_operation<TS, TI>::dist () const
{
  //  TODO: will the distance be sufficient? Or should we take somewhat more?
  return check_local_operation_base<TS, TI>::m_check.distance ();
}

template <class TS, class TI>
OnEmptyIntruderHint
check_local_operation<TS, TI>::on_empty_intruder_hint () const
{
  return check_local_operation_base<TS, TI>::m_different_polygons ? OnEmptyIntruderHint::Drop : OnEmptyIntruderHint::Ignore;
}

template <class TS, class TI>
std::string
check_local_operation<TS, TI>::description () const
{
  return tl::to_string (tr ("Generic DRC check"));
}

//  explicit instantiations
template class DB_PUBLIC check_local_operation<db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC check_local_operation<db::Polygon, db::Polygon>;

// ---------------------------------------------------------------------------------------------------------------

template <class TS, class TI>
check_local_operation_with_properties<TS, TI>::check_local_operation_with_properties (const EdgeRelationFilter &check, bool different_polygons, bool is_merged, bool has_other, bool other_is_merged, const db::RegionCheckOptions &options, db::PropertiesRepository *target_pr, const db::PropertiesRepository *subject_pr, const db::PropertiesRepository *intruder_pr)
  : check_local_operation_base<TS, TI> (check, different_polygons, is_merged, has_other, other_is_merged, options), m_pms (target_pr, subject_pr), m_pmi (target_pr, intruder_pr)
{
  //  .. nothing yet ..
}

template <class TS, class TI>
void
check_local_operation_with_properties<TS, TI>::do_compute_local (db::Layout *layout, db::Cell *subject_cell, const shape_interactions<db::object_with_properties<TS>, db::object_with_properties<TI> > &interactions, std::vector<std::unordered_set<db::EdgePairWithProperties> > &results, const db::LocalProcessorBase *proc) const
{
  tl_assert (results.size () == 1);

  auto by_prop_id = separate_interactions_by_properties (interactions, check_local_operation_base<TS, TI>::m_options.prop_constraint, m_pms, m_pmi);

  for (auto s2p = by_prop_id.begin (); s2p != by_prop_id.end (); ++s2p) {

    std::unordered_set<db::EdgePair> result, intra_polygon_result;

    const std::vector<const TS *> &subjects = s2p->second.first;
    const std::set<const TI *> &intruders = s2p->second.second;

    //  perform the basic check
    check_local_operation_base<TS, TI>::compute_results (layout, subject_cell, subjects, intruders, result, intra_polygon_result, proc);

    //  detect and remove parts of the result which have or do not have results "opposite"
    //  ("opposite" is defined by the projection of edges "through" the subject shape)
    if (check_local_operation_base<TS, TI>::m_options.opposite_filter != db::NoOppositeFilter && (! result.empty () || ! intra_polygon_result.empty ())) {
      check_local_operation_base<TS, TI>::apply_opposite_filter (subjects, result, intra_polygon_result);
    } else {
      result.insert (intra_polygon_result.begin (), intra_polygon_result.end ());
    }

    //  implements error filtering on rectangles
    if (check_local_operation_base<TS, TI>::m_options.rect_filter != RectFilter::NoRectFilter && ! result.empty ()) {
      check_local_operation_base<TS, TI>::apply_rectangle_filter (subjects, result);
    }

    for (auto r = result.begin (); r != result.end (); ++r) {
      results.front ().insert (db::EdgePairWithProperties (*r, pc_norm (check_local_operation_base<TS, TI>::m_options.prop_constraint, s2p->first)));
    }

  }
}

template <class TS, class TI>
db::Coord
check_local_operation_with_properties<TS, TI>::dist () const
{
  //  TODO: will the distance be sufficient? Or should we take somewhat more?
  return check_local_operation_base<TS, TI>::m_check.distance ();
}

template <class TS, class TI>
OnEmptyIntruderHint
check_local_operation_with_properties<TS, TI>::on_empty_intruder_hint () const
{
  return check_local_operation_base<TS, TI>::m_different_polygons ? OnEmptyIntruderHint::Drop : OnEmptyIntruderHint::Ignore;
}

template <class TS, class TI>
std::string
check_local_operation_with_properties<TS, TI>::description () const
{
  return tl::to_string (tr ("Generic DRC check"));
}

//  explicit instantiations
template class DB_PUBLIC check_local_operation_with_properties<db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC check_local_operation_with_properties<db::Polygon, db::Polygon>;

// ---------------------------------------------------------------------------------------------------------------

namespace {

class PolygonToEdgeProcessor
  : public db::PolygonSink
{
public:
  PolygonToEdgeProcessor (db::EdgeProcessor *target, size_t *id)
    : mp_target (target), mp_id (id)
  { }

  virtual void put (const db::Polygon &poly)
  {
    mp_target->insert (poly, (*mp_id)++);
  }

private:
  db::EdgeProcessor *mp_target;
  size_t *mp_id;
};

}

template <class TS, class TI, class TR>
interacting_local_operation<TS, TI, TR>::interacting_local_operation (int mode, bool touching, InteractingOutputMode output_mode, size_t min_count, size_t max_count, bool other_is_merged)
  : m_mode (mode), m_touching (touching), m_output_mode (output_mode), m_min_count (std::max (size_t (1), min_count)), m_max_count (max_count), m_other_is_merged (other_is_merged)
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
db::Coord interacting_local_operation<TS, TI, TR>::dist () const
{
  return m_touching ? 1 : 0;
}

template <class TS, class TI, class TR>
void interacting_local_operation<TS, TI, TR>::do_compute_local (db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase * /*proc*/) const
{
  if (m_output_mode == None) {
    return;
  } else if (m_output_mode == Positive || m_output_mode == Negative) {
    tl_assert (results.size () == 1);
  } else {
    tl_assert (results.size () == 2);
  }

  db::EdgeProcessor ep;
  ep.set_base_verbosity (50);

  std::set<TI> others;
  for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (typename shape_interactions<TS, TI>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  size_t nstart = 0;
  size_t n = 0;

  if (m_mode < -1) {

    //  in enclosing mode self must be primary and other the secondary. For other
    //  modes it's the other way round
    for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i, ++n) {
      const TS &subject = interactions.subject_shape (i->first);
      ep.insert (subject, n);
    }

    nstart = n;

  }

  if (m_mode != -2 && m_min_count == size_t (1) && m_max_count == std::numeric_limits<size_t>::max ()) {

    //  uncounted modes except enclosing (covering) can use one property ID for the primary ("other" input). This is slightly more efficient.
    for (typename std::set<TI>::const_iterator o = others.begin (); o != others.end (); ++o) {
      ep.insert (*o, n);
    }
    n++;

  } else if (! m_other_is_merged && ! (m_min_count == size_t (1) && m_max_count == std::numeric_limits<size_t>::max ())) {

    //  in counted mode we need to merge the shapes because they might overlap
    db::EdgeProcessor ep_merge;
    ep_merge.set_base_verbosity (50);

    size_t i = 0;
    for (typename std::set<TI>::const_iterator o = others.begin (); o != others.end (); ++o) {
      for (typename TI::polygon_edge_iterator e = o->begin_edge (); ! e.at_end (); ++e) {
        ep_merge.insert (*e, i);
      }
      i += 1;
    }

    PolygonToEdgeProcessor ps (&ep, &n);
    db::PolygonGenerator pg (ps, false /*don't resolve holes*/, false);
    db::SimpleMerge op (1 /*wc>0*/);
    ep_merge.process (pg, op);

  } else {

    for (typename std::set<TI>::const_iterator o = others.begin (); o != others.end (); ++o) {
      ep.insert (*o, n);
      n++;
    }

  }

  if (m_mode >= -1) {

    nstart = n;

    for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i, ++n) {
      const TS &subject = interactions.subject_shape (i->first);
      ep.insert (subject, n);
    }

  }

  if (nstart == 0) {
    //  should not happen - but for safety we return here.
    return;
  }

  db::InteractionDetector id (m_mode, nstart - 1);
  id.set_include_touching (m_touching);
  db::EdgeSink es;
  ep.process (es, id);
  id.finish ();

  std::map <size_t, size_t> interaction_counts;
  for (db::InteractionDetector::iterator i = id.begin (); i != id.end (); ++i) {
    if (i->first < nstart && i->second >= nstart) {
      if (m_mode < -1) {
        interaction_counts[i->first] += 1;
      } else {
        interaction_counts[i->second] += 1;
      }
    }
  }

  n = (m_mode < -1 ? 0 : nstart);
  for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i, ++n) {
    size_t count = 0;
    std::map <size_t, size_t>::const_iterator c = interaction_counts.find (n);
    if (c != interaction_counts.end ()) {
      count = c->second;
    }
    bool good = (count >= m_min_count && count <= m_max_count);
    if (good) {
      if (m_output_mode == Positive || m_output_mode == PositiveAndNegative) {
        const TS &subject = interactions.subject_shape (i->first);
        results [0].insert (subject);
      }
    } else {
      if (m_output_mode == Negative) {
        const TS &subject = interactions.subject_shape (i->first);
        //  Yes, it's "positive_result" as this is the first one.
        results [0].insert (subject);
      } else if (m_output_mode == PositiveAndNegative) {
        const TS &subject = interactions.subject_shape (i->first);
        results [1].insert (subject);
      }
    }
  }
}

template <class TS, class TI, class TR>
OnEmptyIntruderHint
interacting_local_operation<TS, TI, TR>::on_empty_intruder_hint () const
{
  if ((m_mode <= 0)) {
    if (m_output_mode == Positive) {
      return OnEmptyIntruderHint::Drop;
    } else if (m_output_mode == Negative) {
      return OnEmptyIntruderHint::Copy;
    } else if (m_output_mode == PositiveAndNegative) {
      return OnEmptyIntruderHint::CopyToSecond;
    }
  } else {
    if (m_output_mode == Positive || m_output_mode == PositiveAndNegative) {
      return OnEmptyIntruderHint::Copy;
    } else if (m_output_mode == Negative) {
      return OnEmptyIntruderHint::Drop;
    }
  }
  return OnEmptyIntruderHint::Ignore;
}

template <class TS, class TI, class TR>
std::string interacting_local_operation<TS, TI, TR>::description () const
{
  return tl::to_string (tr ("Select regions by their geometric relation (interacting, inside, outside ..)"));
}

//  explicit instantiations
template class DB_PUBLIC interacting_local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC interacting_local_operation<db::Polygon, db::Polygon, db::Polygon>;

// ---------------------------------------------------------------------------------------------------------------

template <class TS, class TI, class TR>
contained_local_operation<TS, TI, TR>::contained_local_operation (InteractingOutputMode output_mode)
  : m_output_mode (output_mode)
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
db::Coord contained_local_operation<TS, TI, TR>::dist () const
{
  return 1;   // touching included for degenerated polygons and edges
}

template <class TS, class TI, class TR>
void contained_local_operation<TS, TI, TR>::do_compute_local (db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase * /*proc*/) const
{
  if (m_output_mode == None) {
    return;
  } else if (m_output_mode == Positive || m_output_mode == Negative) {
    tl_assert (results.size () == 1);
  } else {
    tl_assert (results.size () == 2);
  }

  std::set<TI> others;
  for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (typename shape_interactions<TS, TI>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    const TS &subject = interactions.subject_shape (i->first);
    if (others.find (subject) != others.end ()) {
      if (m_output_mode == Positive || m_output_mode == PositiveAndNegative) {
        results [0].insert (subject);
      }
    } else {
      if (m_output_mode == Negative) {
        results [0].insert (subject);
      } else if (m_output_mode == PositiveAndNegative) {
        results [1].insert (subject);
      }
    }
  }
}

template <class TS, class TI, class TR>
OnEmptyIntruderHint
contained_local_operation<TS, TI, TR>::on_empty_intruder_hint () const
{
  if (m_output_mode == Positive) {
    return OnEmptyIntruderHint::Drop;
  } else if (m_output_mode == Negative) {
    return OnEmptyIntruderHint::Copy;
  } else if (m_output_mode == PositiveAndNegative) {
    return OnEmptyIntruderHint::CopyToSecond;
  } else {
    return OnEmptyIntruderHint::Ignore;
  }
}

template <class TS, class TI, class TR>
std::string contained_local_operation<TS, TI, TR>::description () const
{
  return tl::to_string (tr ("Select polygons contained in other region"));
}

//  explicit instantiations
template class DB_PUBLIC contained_local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC contained_local_operation<db::Polygon, db::Polygon, db::Polygon>;
template class DB_PUBLIC contained_local_operation<db::Edge, db::Edge, db::Edge>;

// ---------------------------------------------------------------------------------------------------------------

template <class TS, class TI, class TR>
pull_local_operation<TS, TI, TR>::pull_local_operation (int mode, bool touching)
  : m_mode (mode), m_touching (touching)
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
db::Coord pull_local_operation<TS, TI, TR>::dist () const
{
  return m_touching ? 1 : 0;
}

template <class TS, class TI, class TR>
void pull_local_operation<TS, TI, TR>::do_compute_local (db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase * /*proc*/) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<TR> &result = results.front ();

  db::EdgeProcessor ep;
  ep.set_base_verbosity (50);

  std::set<TI> others;
  for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (typename shape_interactions<TS, TI>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    const TS &subject = interactions.subject_shape (i->first);
    for (typename TS::polygon_edge_iterator e = subject.begin_edge (); ! e.at_end(); ++e) {
      ep.insert (*e, 0);
    }
  }

  size_t n = 1;
  for (typename std::set<TI>::const_iterator o = others.begin (); o != others.end (); ++o, ++n) {
    for (typename TI::polygon_edge_iterator e = o->begin_edge (); ! e.at_end(); ++e) {
      ep.insert (*e, n);
    }
  }

  db::InteractionDetector id (m_mode, 0);
  id.set_include_touching (m_touching);
  db::EdgeSink es;
  ep.process (es, id);
  id.finish ();

  std::set <size_t> selected;
  for (db::InteractionDetector::iterator i = id.begin (); i != id.end () && i->first == 0; ++i) {
    selected.insert (i->second);
  }

  n = 1;
  for (typename std::set<TI>::const_iterator o = others.begin (); o != others.end (); ++o, ++n) {
    if (selected.find (n) != selected.end ()) {
      result.insert (*o);
    }
  }
}

template <class TS, class TI, class TR>
OnEmptyIntruderHint pull_local_operation<TS, TI, TR>::on_empty_intruder_hint () const
{
  return OnEmptyIntruderHint::Drop;
}

template <class TS, class TI, class TR>
std::string pull_local_operation<TS, TI, TR>::description () const
{
  return tl::to_string (tr ("Pull regions by their geometrical relation to first"));
}

template class DB_PUBLIC pull_local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef>;
template class DB_PUBLIC pull_local_operation<db::Polygon, db::Polygon, db::Polygon>;

// ---------------------------------------------------------------------------------------------------------------

template <class TS, class TI, class TR>
interacting_with_edge_local_operation<TS, TI, TR>::interacting_with_edge_local_operation (InteractingOutputMode output_mode, size_t min_count, size_t max_count, bool other_is_merged)
  : m_output_mode (output_mode), m_min_count (std::max (size_t (1), min_count)), m_max_count (max_count), m_other_is_merged (other_is_merged)
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
db::Coord interacting_with_edge_local_operation<TS, TI, TR>::dist () const
{
  //  touching is sufficient
  return 1;
}

template <class TS, class TI, class TR>
void interacting_with_edge_local_operation<TS, TI, TR>::do_compute_local (db::Layout *layout, db::Cell * /*cell*/, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase * /*proc*/) const
{
  if (m_output_mode == None) {
    return;
  } else if (m_output_mode == Positive || m_output_mode == Negative) {
    tl_assert (results.size () == 1);
  } else {
    tl_assert (results.size () == 2);
  }

  std::unordered_map<TR, size_t> counted_results;
  bool counting = !(m_min_count == 1 && m_max_count == std::numeric_limits<size_t>::max ());

  db::box_scanner2<TS, size_t, TI, size_t> scanner;

  result_counting_inserter<TR> inserter (counted_results);
  region_to_edge_interaction_filter<TS, TI, result_counting_inserter<TR> > filter (inserter, false, counting /*get all in counting mode*/);

  std::set<unsigned int> intruder_ids;
  for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (typename shape_interactions<TS, TI>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      intruder_ids.insert (*j);
    }
  }

  //  locally merge the intruder edges if required
  std::unordered_set<TI> merged_heap;
  if (! m_other_is_merged && counting) {

    EdgeBooleanClusterCollector<std::unordered_set<TI> > cluster_collector (&merged_heap, EdgeOr);

    db::box_scanner<TI, size_t> merge_scanner;

    for (std::set<unsigned int>::const_iterator j = intruder_ids.begin (); j != intruder_ids.end (); ++j) {
      const TI *e = &interactions.intruder_shape (*j).second;
      if (! e->is_degenerate ()) {
        merge_scanner.insert (e, 0);
      }
    }

    merge_scanner.process (cluster_collector, 1, db::box_convert<db::Edge> ());

    for (typename std::unordered_set<TI>::const_iterator e = merged_heap.begin (); e != merged_heap.end (); ++e) {
      scanner.insert2 (e.operator-> (), 0);
    }

  } else {

    for (std::set<unsigned int>::const_iterator j = intruder_ids.begin (); j != intruder_ids.end (); ++j) {
      scanner.insert2 (& interactions.intruder_shape (*j).second, 0);
    }

  }

  std::list<TR> heap;
  for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

    const TS &subject = interactions.subject_shape (i->first);

    const TR *addressable = push_polygon_to_heap (layout, subject, heap);

    scanner.insert1 (addressable, 0);
    if (m_output_mode == Negative || m_output_mode == PositiveAndNegative) {
      inserter.init (*addressable);
    }

  }

  scanner.process (filter, 1, db::box_convert<TS> (), db::box_convert<TI> ());

  //  select hits based on their count

  for (typename std::unordered_map<TR, size_t>::const_iterator r = counted_results.begin (); r != counted_results.end (); ++r) {
    bool hit = r->second >= m_min_count && r->second <= m_max_count;
    if (hit) {
      if (m_output_mode == Positive || m_output_mode == PositiveAndNegative) {
        results [0].insert (r->first);
      }
    } else {
      if (m_output_mode == Negative) {
        results [0].insert (r->first);
      } else if (m_output_mode == PositiveAndNegative) {
        results [1].insert (r->first);
      }
    }
  }
}

template <class TS, class TI, class TR>
OnEmptyIntruderHint interacting_with_edge_local_operation<TS, TI, TR>::on_empty_intruder_hint () const
{
  if (m_output_mode == Positive) {
    return OnEmptyIntruderHint::Drop;
  } else if (m_output_mode == Negative) {
    return OnEmptyIntruderHint::Copy;
  } else if (m_output_mode == PositiveAndNegative) {
    return OnEmptyIntruderHint::CopyToSecond;
  } else {
    return OnEmptyIntruderHint::Ignore;
  }
}

template <class TS, class TI, class TR>
std::string interacting_with_edge_local_operation<TS, TI, TR>::description () const
{
  return tl::to_string (tr ("Select regions by their geometric relation to edges"));
}

template class DB_PUBLIC interacting_with_edge_local_operation<db::PolygonRef, db::Edge, db::PolygonRef>;
template class DB_PUBLIC interacting_with_edge_local_operation<db::Polygon, db::Edge, db::Polygon>;

// ---------------------------------------------------------------------------------------------------------------

template <class TS, class TI, class TR>
pull_with_edge_local_operation<TS, TI, TR>::pull_with_edge_local_operation ()
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
db::Coord pull_with_edge_local_operation<TS, TI, TR>::dist () const
{
  //  touching is sufficient
  return 1;
}

template <class TS, class TI, class TR>
void pull_with_edge_local_operation<TS, TI, TR>::do_compute_local (db::Layout *layout, db::Cell * /*cell*/, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase * /*proc*/) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<TR> &result = results.front ();

  db::box_scanner2<TS, size_t, TI, size_t> scanner;

  simple_result_inserter<TR> inserter (result);
  region_to_edge_interaction_filter<TS, TI, simple_result_inserter<TR> > filter (inserter, false);

  std::list<TS> heap;
  for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    const TS &subject = interactions.subject_shape (i->first);
    scanner.insert1 (push_polygon_to_heap (layout, subject, heap), 0);
  }

  std::set<unsigned int> intruder_ids;
  for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (typename shape_interactions<TS, TI>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      intruder_ids.insert (*j);
    }
  }

  for (std::set<unsigned int>::const_iterator j = intruder_ids.begin (); j != intruder_ids.end (); ++j) {
    scanner.insert2 (& interactions.intruder_shape (*j).second, 0);
  }

  scanner.process (filter, 1, db::box_convert<TS> (), db::box_convert<TI> ());
}

template <class TS, class TI, class TR>
OnEmptyIntruderHint pull_with_edge_local_operation<TS, TI, TR>::on_empty_intruder_hint () const
{
  return OnEmptyIntruderHint::Drop;
}

template <class TS, class TI, class TR>
std::string pull_with_edge_local_operation<TS, TI, TR>::description () const
{
  return tl::to_string (tr ("Pull edges from second by their geometric relation to first"));
}

template class DB_PUBLIC pull_with_edge_local_operation<db::PolygonRef, db::Edge, db::Edge>;
template class DB_PUBLIC pull_with_edge_local_operation<db::Polygon, db::Edge, db::Edge>;

// ---------------------------------------------------------------------------------------------------------------

template <class TS, class TI, class TR>
pull_with_text_local_operation<TS, TI, TR>::pull_with_text_local_operation ()
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
db::Coord pull_with_text_local_operation<TS, TI, TR>::dist () const
{
  //  touching is sufficient
  return 1;
}

template <class TS, class TI, class TR>
void pull_with_text_local_operation<TS, TI, TR>::do_compute_local (db::Layout *layout, db::Cell * /*cell*/, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase * /*proc*/) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<TR> &result = results.front ();

  db::box_scanner2<TS, size_t, TI, size_t> scanner;

  simple_result_inserter<TR> inserter (result);
  region_to_text_interaction_filter<TS, TI, simple_result_inserter<TR> > filter (inserter, false);

  for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (typename shape_interactions<TS, TI>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      scanner.insert2 (& interactions.intruder_shape (*j).second, 0);
    }
  }

  std::set<unsigned int> intruder_ids;
  for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (typename shape_interactions<TS, TI>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      intruder_ids.insert (*j);
    }
  }

  for (std::set<unsigned int>::const_iterator j = intruder_ids.begin (); j != intruder_ids.end (); ++j) {
    scanner.insert2 (& interactions.intruder_shape (*j).second, 0);
  }

  std::list<TS> heap;
  for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    const TS *subject = push_polygon_to_heap (layout, interactions.subject_shape (i->first), heap);
    scanner.insert1 (subject, 0);
  }

  scanner.process (filter, 1, db::box_convert<TS> (), db::box_convert<TI> ());
}

template <class TS, class TI, class TR>
OnEmptyIntruderHint pull_with_text_local_operation<TS, TI, TR>::on_empty_intruder_hint () const
{
  return OnEmptyIntruderHint::Drop;
}

template <class TS, class TI, class TR>
std::string pull_with_text_local_operation<TS, TI, TR>::description () const
{
  return tl::to_string (tr ("Pull texts from second by their geometric relation to first"));
}

template class DB_PUBLIC pull_with_text_local_operation<db::PolygonRef, db::TextRef, db::TextRef>;
template class DB_PUBLIC pull_with_text_local_operation<db::Polygon, db::Text, db::Text>;

// ---------------------------------------------------------------------------------------------------------------

template <class TS, class TI, class TR>
interacting_with_text_local_operation<TS, TI, TR>::interacting_with_text_local_operation (InteractingOutputMode output_mode, size_t min_count, size_t max_count)
  : m_output_mode (output_mode), m_min_count (std::max (size_t (1), min_count)), m_max_count (max_count)
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
db::Coord interacting_with_text_local_operation<TS, TI, TR>::dist () const
{
  //  touching is sufficient
  return 1;
}


template <class TS, class TI, class TR>
void interacting_with_text_local_operation<TS, TI, TR>::do_compute_local (db::Layout *layout, db::Cell * /*cell*/, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase * /*proc*/) const
{
  if (m_output_mode == None) {
    return;
  } else if (m_output_mode == Positive || m_output_mode == Negative) {
    tl_assert (results.size () == 1);
  } else {
    tl_assert (results.size () == 2);
  }

  std::unordered_map<TR, size_t> counted_results;
  bool counting = !(m_min_count == 1 && m_max_count == std::numeric_limits<size_t>::max ());

  db::box_scanner2<TR, size_t, TI, size_t> scanner;

  result_counting_inserter<TR> inserter (counted_results);
  region_to_text_interaction_filter<TS, TI, result_counting_inserter<TR> > filter (inserter, false, counting /*get all in counting mode*/);

  std::set<unsigned int> intruder_ids;
  for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (typename shape_interactions<TS, TI>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      intruder_ids.insert (*j);
    }
  }

  for (std::set<unsigned int>::const_iterator j = intruder_ids.begin (); j != intruder_ids.end (); ++j) {
    scanner.insert2 (& interactions.intruder_shape (*j).second, 0);
  }

  std::list<TR> heap;
  for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

    const TR *addressable = push_polygon_to_heap (layout, interactions.subject_shape (i->first), heap);

    scanner.insert1 (addressable, 0);
    if (m_output_mode == Negative || m_output_mode == PositiveAndNegative) {
      inserter.init (*addressable);
    }

  }

  scanner.process (filter, 1, db::box_convert<TR> (), db::box_convert<TI> ());

  //  select hits based on their count

  for (typename std::unordered_map<TR, size_t>::const_iterator r = counted_results.begin (); r != counted_results.end (); ++r) {
    bool hit = r->second >= m_min_count && r->second <= m_max_count;
    if (hit) {
      if (m_output_mode == Positive || m_output_mode == PositiveAndNegative) {
        results [0].insert (r->first);
      }
    } else {
      if (m_output_mode == Negative) {
        //  Yes. It's "positive"! This is the first output.
        results [0].insert (r->first);
      } else if (m_output_mode == PositiveAndNegative) {
        results [1].insert (r->first);
      }
    }
  }
}

template <class TS, class TI, class TR>
OnEmptyIntruderHint interacting_with_text_local_operation<TS, TI, TR>::on_empty_intruder_hint () const
{
  if (m_output_mode == Positive) {
    return OnEmptyIntruderHint::Drop;
  } else if (m_output_mode == Negative) {
    return OnEmptyIntruderHint::Copy;
  } else if (m_output_mode == PositiveAndNegative) {
    return OnEmptyIntruderHint::CopyToSecond;
  } else {
    return OnEmptyIntruderHint::Ignore;
  }
}

template <class TS, class TI, class TR>
std::string interacting_with_text_local_operation<TS, TI, TR>::description () const
{
  return tl::to_string (tr ("Select regions by their geometric relation to texts"));
}

//  explicit instantiations
template class DB_PUBLIC interacting_with_text_local_operation<db::PolygonRef, db::TextRef, db::PolygonRef>;
template class DB_PUBLIC interacting_with_text_local_operation<db::Polygon, db::Text, db::Polygon>;

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
bool_and_or_not_local_operation<TS, TI, TR>::do_compute_local (db::Layout *layout, db::Cell * /*cell*/, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase *proc) const
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
    db::PolygonSplitter splitter (pr, proc->area_ratio (), proc->max_vertex_count ());
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
bool_and_or_not_local_operation_with_properties<TS, TI, TR>::do_compute_local (db::Layout *layout, db::Cell * /*cell*/, const shape_interactions<db::object_with_properties<TS>, db::object_with_properties<TI> > &interactions, std::vector<std::unordered_set<db::object_with_properties<TR> > > &results, const db::LocalProcessorBase *proc) const
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
        if (pc_match (m_property_constraint, prop_id_s, m_pmi (intruder.properties_id ()))) {
          shapes_by_prop.second.insert (intruder);
        }
      }

    }

  }

  for (auto p2s = by_prop_id.begin (); p2s != by_prop_id.end (); ++p2s) {

    ep.clear ();
    size_t p1 = 0, p2 = 1;

    const std::set<TI> &others = p2s->second.second;
    db::properties_id_type prop_id = pc_norm (m_property_constraint, p2s->first);

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
      db::PolygonSplitter splitter (pr, proc->area_ratio (), proc->max_vertex_count ());
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
two_bool_and_not_local_operation<TS, TI, TR>::do_compute_local (db::Layout *layout, db::Cell * /*cell*/, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, const db::LocalProcessorBase *proc) const
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
    db::PolygonSplitter splitter0 (pr0, proc->area_ratio (), proc->max_vertex_count ());
    db::PolygonGenerator pg0 (splitter0, true, true);

    db::BooleanOp op1 (db::BooleanOp::ANotB);
    db::polygon_ref_generator<TR> pr1 (layout, result1);
    db::PolygonSplitter splitter1 (pr1, proc->area_ratio (), proc->max_vertex_count ());
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
two_bool_and_not_local_operation_with_properties<TS, TI, TR>::do_compute_local (db::Layout *layout, db::Cell * /*cell*/, const shape_interactions<db::object_with_properties<TS>, db::object_with_properties<TI> > &interactions, std::vector<std::unordered_set<db::object_with_properties<TR> > > &results, const db::LocalProcessorBase *proc) const
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
        if (pc_match (m_property_constraint, prop_id_s, m_pmi (intruder.properties_id ()))) {
          shapes_by_prop.second.insert (intruder);
        }
      }

    }

  }

  for (auto p2s = by_prop_id.begin (); p2s != by_prop_id.end (); ++p2s) {

    ep.clear ();
    size_t p1 = 0, p2 = 1;

    const std::set<TR> &others = p2s->second.second;
    db::properties_id_type prop_id = pc_norm (m_property_constraint, p2s->first);

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
      db::PolygonSplitter splitter0 (pr0, proc->area_ratio (), proc->max_vertex_count ());
      db::PolygonGenerator pg0 (splitter0, true, true);

      db::BooleanOp op1 (db::BooleanOp::ANotB);
      db::polygon_ref_generator<TR> pr1 (layout, result1_wo_props);
      db::PolygonSplitter splitter1 (pr1, proc->area_ratio (), proc->max_vertex_count ());
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
SelfOverlapMergeLocalOperation::do_compute_local (db::Layout *layout, db::Cell * /*cell*/, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, const db::LocalProcessorBase * /*proc*/) const
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

PolygonToEdgeLocalOperation::PolygonToEdgeLocalOperation (db::PropertiesRepository *target_pr, const db::PropertiesRepository *source_pr)
  : local_operation<db::PolygonRefWithProperties, db::PolygonRefWithProperties, db::EdgeWithProperties> (), m_pm (target_pr, source_pr)
{
  //  .. nothing yet ..
}

std::string
PolygonToEdgeLocalOperation::description () const
{
  return std::string ("polygon to edges");
}

void
PolygonToEdgeLocalOperation::do_compute_local (db::Layout * /*layout*/, db::Cell * /*cell*/, const shape_interactions<db::PolygonRefWithProperties, db::PolygonRefWithProperties> &interactions, std::vector<std::unordered_set<db::EdgeWithProperties> > &results, const db::LocalProcessorBase * /*proc*/) const
{
  db::EdgeProcessor ep;
  ep.set_base_verbosity (50);

  auto by_prop_id = separate_interactions_by_properties (interactions, db::SamePropertiesConstraint, m_pm, m_pm);
  for (auto shapes_by_prop_id = by_prop_id.begin (); shapes_by_prop_id != by_prop_id.end (); ++shapes_by_prop_id) {

    db::properties_id_type prop_id = shapes_by_prop_id->first;

    for (auto s = shapes_by_prop_id->second.first.begin (); s != shapes_by_prop_id->second.first.end (); ++s) {
      ep.insert (**s);
    }

    db::property_injector<db::Edge, std::unordered_set<db::EdgeWithProperties> > results_with_properties (&results.front (), prop_id);

    if (shapes_by_prop_id->second.second.empty ()) {

      db::edge_to_edge_set_generator<db::property_injector<db::Edge, std::unordered_set<db::EdgeWithProperties> > > eg (results_with_properties, prop_id);
      db::MergeOp op (0);
      ep.process (eg, op);

    } else {

      //  With intruders: to compute our local contribution we take the edges without and with intruders
      //  and deliver what is in both sets

      db::MergeOp op (0);

      std::vector<Edge> edges1;
      db::EdgeContainer ec1 (edges1);
      ep.process (ec1, op);

      ep.clear ();

      for (auto s = interactions.begin_subjects (); s != interactions.end_subjects (); ++s) {
        ep.insert (s->second);
      }
      for (auto i = interactions.begin_intruders (); i != interactions.end_intruders (); ++i) {
        ep.insert (i->second.second);
      }

      std::vector<Edge> edges2;
      db::EdgeContainer ec2 (edges2);
      ep.process (ec2, op);

      //  Runs the boolean AND between the result with and without intruders

      db::box_scanner<db::Edge, size_t> scanner;
      scanner.reserve (edges1.size () + edges2.size ());

      for (std::vector<Edge>::const_iterator i = edges1.begin (); i != edges1.end (); ++i) {
        scanner.insert (i.operator-> (), 0);
      }
      for (std::vector<Edge>::const_iterator i = edges2.begin (); i != edges2.end (); ++i) {
        scanner.insert (i.operator-> (), 1);
      }

      EdgeBooleanClusterCollector<db::property_injector<Edge, std::unordered_set<db::EdgeWithProperties> > > cluster_collector (&results_with_properties, EdgeAnd);
      scanner.process (cluster_collector, 1, db::box_convert<db::Edge> ());

    }

  }

}

}
