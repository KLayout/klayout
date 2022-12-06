
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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

template <class TS, class TI>
check_local_operation<TS, TI>::check_local_operation (const EdgeRelationFilter &check, bool different_polygons, bool is_merged, bool has_other, bool other_is_merged, const db::RegionCheckOptions &options)
  : m_check (check), m_different_polygons (different_polygons), m_is_merged (is_merged), m_has_other (has_other), m_other_is_merged (other_is_merged), m_options (options)
{
  //  .. nothing yet ..
}

namespace
{

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

template <class TS, class TI>
void
check_local_operation<TS, TI>::do_compute_local (db::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<db::EdgePair> result, intra_polygon_result;

  //  NOTE: the rectangle and opposite filters are unsymmetric
  bool symmetric_edge_pairs = ! m_has_other && m_options.opposite_filter == db::NoOppositeFilter && m_options.rect_filter == RectFilter::NoRectFilter;

  edge2edge_check_negative_or_positive<std::unordered_set<db::EdgePair> > edge_check (m_check, result, intra_polygon_result, m_options.negative, m_different_polygons, m_has_other, m_options.shielded, symmetric_edge_pairs);
  poly2poly_check<TS> poly_check (edge_check);

  std::unordered_set<TI> polygons;
  std::unordered_set<TS> spolygons;

  db::EdgeProcessor ep;
  ep.set_base_verbosity (50);

  std::set<unsigned int> ids;
  for (auto i = interactions.begin (); i != interactions.end (); ++i) {
    for (auto j = i->second.begin (); j != i->second.end (); ++j) {
      ids.insert (*j);
    }
  }

  bool take_all = edge_check.has_negative_edge_output () || interactions.num_intruders () == 0;

  db::Box common_box;
  if (! take_all) {

    db::Vector e (edge_check.distance (), edge_check.distance ());

    db::Box subject_box;
    for (auto i = interactions.begin (); i != interactions.end (); ++i) {
      subject_box += db::box_convert<TS> () (interactions.subject_shape (i->first));
    }

    if (edge_check.requires_different_layers ()) {
      db::Box intruder_box;
      for (auto id = ids.begin (); id != ids.end (); ++id) {
        intruder_box += db::box_convert<TI> () (interactions.intruder_shape (*id).second);
      }
      common_box = subject_box.enlarged (e) & intruder_box.enlarged (e);
    } else {
      common_box = subject_box.enlarged (e);
    }

  }

  if (m_has_other) {

    size_t n = 0;

    if (m_is_merged || (interactions.size () == 1 && interactions.subject_shape (interactions.begin ()->first).is_box ())) {

      for (auto i = interactions.begin (); i != interactions.end (); ++i) {
        const TS &subject = interactions.subject_shape (i->first);
        if (! take_all) {
          poly_check.enter (subject, n, common_box);
        } else {
          poly_check.enter (subject, n);
        }
        n += 2;
      }

    } else {

      //  merge needed for the subject shapes

      ep.clear ();
      size_t nn = 0;

      for (auto i = interactions.begin (); i != interactions.end (); ++i) {
        const TS &is = interactions.subject_shape (i->first);
        for (typename TS::polygon_edge_iterator e = is.begin_edge (); ! e.at_end (); ++e) {
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

    if (ids.empty ()) {

      //  empty intruders

    } else if (! m_other_is_merged && (ids.size () > 1 || ! interactions.intruder_shape (*ids.begin ()).second.is_box ())) {

      //  NOTE: this local merge is not necessarily giving the same results than a global merge before running
      //  the processor. Reason: the search range is limited, hence not all necessary components may have been
      //  captured.

      ep.clear ();
      size_t nn = 0;

      for (auto id = ids.begin (); id != ids.end (); ++id) {
        const TI &is = interactions.intruder_shape (*id).second;
        for (auto e = is.begin_edge (); ! e.at_end (); ++e) {
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
      for (auto id = ids.begin (); id != ids.end (); ++id) {
        if (! take_all) {
          poly_check.enter (interactions.intruder_shape (*id).second, n, common_box);
        } else {
          poly_check.enter (interactions.intruder_shape (*id).second, n);
        }
        n += 2;
      }

    }

  } else {

    if (m_is_merged || (interactions.size () == 1 && ids.empty () && interactions.subject_shape (interactions.begin ()->first).is_box ())) {

      //  no merge required

      //  NOTE: we need to eliminate identical shapes from intruders and subjects because those will shield

      size_t n = 0;
      std::unordered_set<TI> subjects;

      for (auto i = interactions.begin (); i != interactions.end (); ++i) {
        //  we can't directly insert because TS may be != TI
        const TS &ts = interactions.subject_shape (i->first);
        insert_into_hash (subjects, ts);
        if (! take_all) {
          poly_check.enter (ts, n, common_box);
        } else {
          poly_check.enter (ts, n);
        }
        n += 2;
      }

      n = 1;

      for (auto id = ids.begin (); id != ids.end (); ++id) {
        const TI &ti = interactions.intruder_shape (*id).second;
        if (subjects.find (ti) == subjects.end ()) {
          if (! take_all) {
            poly_check.enter (ti, n, common_box);
          } else {
            poly_check.enter (ti, n);
          }
        }
      }

    } else if (ids.empty ()) {

      //  merge needed for the subject shapes - no intruders present so this is the simple case

      size_t n = 0;

      ep.clear ();
      size_t nn = 0;

      for (auto i = interactions.begin (); i != interactions.end (); ++i) {
        const TS &ts = interactions.subject_shape (i->first);
        for (auto e = ts.begin_edge (); ! e.at_end (); ++e) {
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

      for (auto i = interactions.begin (); i != interactions.end (); ++i) {
        const TS &ts = interactions.subject_shape (i->first);
        for (auto e = ts.begin_edge (); ! e.at_end (); ++e) {
          ep.insert (*e, nn);
        }
        ++nn;
      }

      for (auto id = ids.begin (); id != ids.end (); ++id) {
        const TI &ti = interactions.intruder_shape (*id).second;
        for (auto e = ti.begin_edge (); ! e.at_end (); ++e) {
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
      for (auto i = interactions.begin (); i != interactions.end (); ++i) {
        const TS &ts = interactions.subject_shape (i->first);
        sz += ts.vertices ();
      }

      subject_edges.reserve (sz);

      for (auto i = interactions.begin (); i != interactions.end (); ++i) {
        const TS &ts = interactions.subject_shape (i->first);
        for (auto e = ts.begin_edge (); ! e.at_end (); ++e) {
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

  //  detect and remove parts of the result which have or do not have results "opposite"
  //  ("opposite" is defined by the projection of edges "through" the subject shape)
  if (m_options.opposite_filter != db::NoOppositeFilter && (! result.empty () || ! intra_polygon_result.empty ())) {

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
            for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end () && ! shielded; ++i) {
              shielded = shields_interaction (ep_opp, interactions.subject_shape (i->first));
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
            for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end () && ! shielded; ++i) {
              shielded = shields_interaction (ep_opp, interactions.subject_shape (i->first));
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

  } else {

    result.insert (intra_polygon_result.begin (), intra_polygon_result.end ());

  }

  //  implements error filtering on rectangles
  if (m_options.rect_filter != RectFilter::NoRectFilter && ! result.empty ()) {

    std::unordered_set<db::EdgePair> waived;

    for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

      const TS &subject = interactions.subject_shape (i->first);
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

      for (std::unordered_set<db::EdgePair>::const_iterator i = result.begin (); i != result.end (); ++i) {
        results.front ().insert (db::EdgePair (i->first (), i->first ().swapped_points ()));
      }
      result.clear ();

    }

  }

  results.front ().insert (result.begin (), result.end ());
}

template <class TS, class TI>
db::Coord
check_local_operation<TS, TI>::dist () const
{
  //  TODO: will the distance be sufficient? Or should we take somewhat more?
  return m_check.distance ();
}

template <class TS, class TI>
OnEmptyIntruderHint
check_local_operation<TS, TI>::on_empty_intruder_hint () const
{
  return m_different_polygons ? OnEmptyIntruderHint::Drop : OnEmptyIntruderHint::Ignore;
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
void interacting_local_operation<TS, TI, TR>::do_compute_local (db::Layout * /*layout*/, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
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
void contained_local_operation<TS, TI, TR>::do_compute_local (db::Layout * /*layout*/, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
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
void pull_local_operation<TS, TI, TR>::do_compute_local (db::Layout * /*layout*/, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
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
void interacting_with_edge_local_operation<TS, TI, TR>::do_compute_local (db::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
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
void pull_with_edge_local_operation<TS, TI, TR>::do_compute_local (db::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
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
void pull_with_text_local_operation<TS, TI, TR>::do_compute_local (db::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
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
void interacting_with_text_local_operation<TS, TI, TR>::do_compute_local (db::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
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

}
