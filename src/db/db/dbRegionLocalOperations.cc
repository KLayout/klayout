
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


#include "dbRegionUtils.h"
#include "dbRegionLocalOperations.h"
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

static inline bool needs_merge (const std::unordered_set<db::PolygonRef> &polygons)
{
  if (polygons.empty ()) {
    return false;
  } else if (polygons.size () > 1) {
    return true;
  } else if (polygons.begin ()->obj ().is_box ()) {
    return false;
  } else {
    return true;
  }
}

static inline bool needs_merge (const std::unordered_set<db::Polygon> &polygons)
{
  if (polygons.empty ()) {
    return false;
  } else if (polygons.size () > 1) {
    return true;
  } else if (polygons.begin ()->is_box ()) {
    return false;
  } else {
    return true;
  }
}

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

template <class TS, class TI, class TR>
check_local_operation<TS, TI, TR>::check_local_operation (const EdgeRelationFilter &check, bool different_polygons, bool has_other, bool other_is_merged, bool shielded)
  : m_check (check), m_different_polygons (different_polygons), m_has_other (has_other), m_other_is_merged (other_is_merged), m_shielded (shielded)
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
void
check_local_operation<TS, TI, TR>::compute_local (db::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<TR> &result = results.front ();

  edge2edge_check<std::unordered_set<TR> > edge_check (m_check, result, m_different_polygons, m_has_other, m_shielded);
  poly2poly_check<TS, std::unordered_set<TR> > poly_check (edge_check);

  std::list<TS> heap;
  db::box_scanner<TS, size_t> scanner;
  std::unordered_set<TI> polygons;

  if (m_has_other) {

    for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      for (typename shape_interactions<TS, TI>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
        polygons.insert (interactions.intruder_shape (*j).second);
      }
    }

    size_t n = 0;
    for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      const TS &subject = interactions.subject_shape (i->first);
      scanner.insert (push_polygon_to_heap (layout, subject, heap), n);
      n += 2;
    }

    //  merge the intruders to remove inner edges

    if (! m_other_is_merged && needs_merge (polygons)) {

      db::EdgeProcessor ep;

      ep.clear ();
      size_t i = 0;
      for (typename std::unordered_set<TI>::const_iterator o = polygons.begin (); o != polygons.end (); ++o) {
        for (typename TI::polygon_edge_iterator e = o->begin_edge (); ! e.at_end(); ++e) {
          ep.insert (*e, i);
        }
        ++i;
      }

      polygons.clear ();

      db::polygon_ref_generator<TI> ps (layout, polygons);
      db::PolygonGenerator pg (ps, false /*don't resolve holes*/, false);
      db::SimpleMerge op (1 /*wc>0*/);
      ep.process (pg, op);

    }

    n = 1;
    for (typename std::unordered_set<TI>::const_iterator o = polygons.begin (); o != polygons.end (); ++o) {
      scanner.insert (push_polygon_to_heap (layout, *o, heap), n);
      n += 2;
    }

  } else {

    for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      polygons.insert (interactions.subject_shape (i->first));
      for (typename shape_interactions<TS, TI>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
        polygons.insert (interactions.intruder_shape (*j).second);
      }
    }

    size_t n = 0;
    for (typename std::unordered_set<TI>::const_iterator o = polygons.begin (); o != polygons.end (); ++o) {
      scanner.insert (push_polygon_to_heap (layout, *o, heap), n);
      n += 2;
    }

  }

  do {
    scanner.process (poly_check, m_check.distance (), db::box_convert<TS> ());
  } while (edge_check.prepare_next_pass ());
}

template <class TS, class TI, class TR>
db::Coord
check_local_operation<TS, TI, TR>::dist () const
{
  //  TODO: will the distance be sufficient? Or should we take somewhat more?
  return m_check.distance ();
}

template <class TS, class TI, class TR>
OnEmptyIntruderHint
check_local_operation<TS, TI, TR>::on_empty_intruder_hint () const
{
  return m_different_polygons ? OnEmptyIntruderHint::Drop : OnEmptyIntruderHint::Ignore;
}

template <class TS, class TI, class TR>
std::string
check_local_operation<TS, TI, TR>::description () const
{
  return tl::to_string (tr ("Generic DRC check"));
}

//  explicit instantiations
template class check_local_operation<db::PolygonRef, db::PolygonRef, db::EdgePair>;
template class check_local_operation<db::Polygon, db::Polygon, db::EdgePair>;

// ---------------------------------------------------------------------------------------------------------------

template <class TS, class TI, class TR>
interacting_local_operation<TS, TI, TR>::interacting_local_operation (int mode, bool touching, bool inverse, size_t min_count, size_t max_count)
  : m_mode (mode), m_touching (touching), m_inverse (inverse), m_min_count (std::max (size_t (1), min_count)), m_max_count (max_count)
{
  //  .. nothing yet ..
}

template <class TS, class TI, class TR>
db::Coord interacting_local_operation<TS, TI, TR>::dist () const
{
  return m_touching ? 1 : 0;
}

template <class TS, class TI, class TR>
void interacting_local_operation<TS, TI, TR>::compute_local (db::Layout * /*layout*/, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<TR> &result = results.front ();

  db::EdgeProcessor ep;

  std::set<TI> others;
  for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (typename shape_interactions<TS, TI>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  size_t nstart = 0;

  if (m_min_count == size_t (1) && m_max_count == std::numeric_limits<size_t>::max ()) {

    for (typename std::set<TI>::const_iterator o = others.begin (); o != others.end (); ++o) {
      ep.insert (*o, nstart);
    }
    nstart++;

  } else {

    tl_assert (m_mode == 0);

    for (typename std::set<TI>::const_iterator o = others.begin (); o != others.end (); ++o) {
      ep.insert (*o, nstart);
      nstart++;
    }

  }

  size_t n = nstart;
  for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i, ++n) {
    const TS &subject = interactions.subject_shape (i->first);
    ep.insert (subject, n);
  }

  db::InteractionDetector id (m_mode, 0);
  id.set_include_touching (m_touching);
  db::EdgeSink es;
  ep.process (es, id);
  id.finish ();

  std::map <size_t, size_t> interaction_counts;
  for (db::InteractionDetector::iterator i = id.begin (); i != id.end (); ++i) {
    if (i->first < nstart && i->second >= nstart) {
      interaction_counts[i->second] += 1;
    }
  }

  n = nstart;
  for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i, ++n) {
    size_t count = 0;
    std::map <size_t, size_t>::const_iterator c = interaction_counts.find (n);
    if (c != interaction_counts.end ()) {
      count = c->second;
    }
    if ((count >= m_min_count && count <= m_max_count) != m_inverse) {
      const TS &subject = interactions.subject_shape (i->first);
      result.insert (subject);
    }
  }
}

template <class TS, class TI, class TR>
OnEmptyIntruderHint
interacting_local_operation<TS, TI, TR>::on_empty_intruder_hint () const
{
  if ((m_mode <= 0) != m_inverse) {
    return OnEmptyIntruderHint::Drop;
  } else {
    return OnEmptyIntruderHint::Copy;
  }
}

template <class TS, class TI, class TR>
std::string interacting_local_operation<TS, TI, TR>::description () const
{
  return tl::to_string (tr ("Select regions by their geometric relation (interacting, inside, outside ..)"));
}

//  explicit instantiations
template class interacting_local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef>;
template class interacting_local_operation<db::Polygon, db::Polygon, db::Polygon>;
template class interacting_local_operation<db::Polygon, db::Edge, db::Polygon>;

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
void pull_local_operation<TS, TI, TR>::compute_local (db::Layout * /*layout*/, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<TR> &result = results.front ();

  db::EdgeProcessor ep;

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

template class pull_local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef>;
template class pull_local_operation<db::Polygon, db::Polygon, db::Polygon>;

// ---------------------------------------------------------------------------------------------------------------

template <class TS, class TI, class TR>
interacting_with_edge_local_operation<TS, TI, TR>::interacting_with_edge_local_operation (bool inverse, size_t min_count, size_t max_count)
  : m_inverse (inverse), m_min_count (std::max (size_t (1), min_count)), m_max_count (max_count)
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
void interacting_with_edge_local_operation<TS, TI, TR>::compute_local (db::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
{
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

  for (std::set<unsigned int>::const_iterator j = intruder_ids.begin (); j != intruder_ids.end (); ++j) {
    scanner.insert2 (& interactions.intruder_shape (*j).second, 0);
  }

  std::list<TR> heap;
  for (typename shape_interactions<TS, TI>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

    const TS &subject = interactions.subject_shape (i->first);

    const TR *addressable = push_polygon_to_heap (layout, subject, heap);

    scanner.insert1 (addressable, 0);
    if (m_inverse) {
      inserter.init (*addressable);
    }

  }

  scanner.process (filter, 1, db::box_convert<TS> (), db::box_convert<TI> ());

  //  select hits based on their count

  tl_assert (results.size () == 1);
  std::unordered_set<TR> &result = results.front ();

  for (typename std::unordered_map<TR, size_t>::const_iterator r = counted_results.begin (); r != counted_results.end (); ++r) {
    bool hit = r->second >= m_min_count && r->second <= m_max_count;
    if (hit != m_inverse) {
      result.insert (r->first);
    }
  }
}

template <class TS, class TI, class TR>
OnEmptyIntruderHint interacting_with_edge_local_operation<TS, TI, TR>::on_empty_intruder_hint () const
{
  if (!m_inverse) {
    return OnEmptyIntruderHint::Drop;
  } else {
    return OnEmptyIntruderHint::Copy;
  }
}

template <class TS, class TI, class TR>
std::string interacting_with_edge_local_operation<TS, TI, TR>::description () const
{
  return tl::to_string (tr ("Select regions by their geometric relation to edges"));
}

template class interacting_with_edge_local_operation<db::PolygonRef, db::Edge, db::PolygonRef>;
template class interacting_with_edge_local_operation<db::Polygon, db::Edge, db::Polygon>;

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
void pull_with_edge_local_operation<TS, TI, TR>::compute_local (db::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
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

template class pull_with_edge_local_operation<db::PolygonRef, db::Edge, db::Edge>;
template class pull_with_edge_local_operation<db::Polygon, db::Edge, db::Edge>;

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
void pull_with_text_local_operation<TS, TI, TR>::compute_local (db::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
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

template class pull_with_text_local_operation<db::PolygonRef, db::TextRef, db::TextRef>;
template class pull_with_text_local_operation<db::Polygon, db::Text, db::Text>;

// ---------------------------------------------------------------------------------------------------------------

template <class TS, class TI, class TR>
interacting_with_text_local_operation<TS, TI, TR>::interacting_with_text_local_operation (bool inverse, size_t min_count, size_t max_count)
  : m_inverse (inverse), m_min_count (std::max (size_t (1), min_count)), m_max_count (max_count)
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
void interacting_with_text_local_operation<TS, TI, TR>::compute_local (db::Layout *layout, const shape_interactions<TS, TI> &interactions, std::vector<std::unordered_set<TR> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
{
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
    if (m_inverse) {
      inserter.init (*addressable);
    }

  }

  scanner.process (filter, 1, db::box_convert<TR> (), db::box_convert<TI> ());

  //  select hits based on their count

  tl_assert (results.size () == 1);
  std::unordered_set<TR> &result = results.front ();

  for (typename std::unordered_map<TR, size_t>::const_iterator r = counted_results.begin (); r != counted_results.end (); ++r) {
    bool hit = r->second >= m_min_count && r->second <= m_max_count;
    if (hit != m_inverse) {
      result.insert (r->first);
    }
  }
}

template <class TS, class TI, class TR>
OnEmptyIntruderHint interacting_with_text_local_operation<TS, TI, TR>::on_empty_intruder_hint () const
{
  if (!m_inverse) {
    return OnEmptyIntruderHint::Drop;
  } else {
    return OnEmptyIntruderHint::Copy;
  }
}

template <class TS, class TI, class TR>
std::string interacting_with_text_local_operation<TS, TI, TR>::description () const
{
  return tl::to_string (tr ("Select regions by their geometric relation to texts"));
}

//  explicit instantiations
template class interacting_with_text_local_operation<db::PolygonRef, db::TextRef, db::PolygonRef>;
template class interacting_with_text_local_operation<db::Polygon, db::Text, db::Polygon>;

}
