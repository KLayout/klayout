
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

struct ResultCountingInserter
{
  typedef db::Polygon value_type;

  ResultCountingInserter (db::Layout *layout, std::unordered_map<db::PolygonRef, size_t> &result)
    : mp_layout (layout), mp_result (&result)
  {
    //  .. nothing yet ..
  }

  void insert (const db::Polygon &p)
  {
    (*mp_result)[db::PolygonRef (p, mp_layout->shape_repository ())] += 1;
  }

  void init (const db::Polygon &p)
  {
    (*mp_result)[db::PolygonRef (p, mp_layout->shape_repository ())] = 0;
  }

private:
  db::Layout *mp_layout;
  std::unordered_map<db::PolygonRef, size_t> *mp_result;
};

struct EdgeResultInserter
{
  typedef db::Edge value_type;

  EdgeResultInserter (std::unordered_set<db::Edge> &result)
    : mp_result (&result)
  {
    //  .. nothing yet ..
  }

  void insert (const db::Edge &e)
  {
    (*mp_result).insert (e);
  }

private:
  std::unordered_set<db::Edge> *mp_result;
};

struct TextResultInserter
{
  typedef db::TextRef value_type;

  TextResultInserter (std::unordered_set<db::TextRef> &result)
    : mp_result (&result)
  {
    //  .. nothing yet ..
  }

  void insert (const db::TextRef &e)
  {
    (*mp_result).insert (e);
  }

private:
  std::unordered_set<db::TextRef> *mp_result;
};

}

// ---------------------------------------------------------------------------------------------------------------

CheckLocalOperation::CheckLocalOperation (const EdgeRelationFilter &check, bool different_polygons, bool has_other, bool shielded)
  : m_check (check), m_different_polygons (different_polygons), m_has_other (has_other), m_shielded (shielded)
{
  //  .. nothing yet ..
}

void
CheckLocalOperation::compute_local (db::Layout * /*layout*/, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<db::EdgePair> &result = results.front ();

  edge2edge_check<std::unordered_set<db::EdgePair> > edge_check (m_check, result, m_different_polygons, m_has_other, m_shielded);
  poly2poly_check<std::unordered_set<db::EdgePair> > poly_check (edge_check);

  std::list<db::Polygon> heap;
  db::box_scanner<db::Polygon, size_t> scanner;

  if (m_has_other) {

    std::set<db::PolygonRef> others;
    for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
        others.insert (interactions.intruder_shape (*j).second);
      }
    }

    size_t n = 0;
    for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      const db::PolygonRef &subject = interactions.subject_shape (i->first);
      heap.push_back (subject.obj ().transformed (subject.trans ()));
      scanner.insert (& heap.back (), n);
      n += 2;
    }

    n = 1;
    for (std::set<db::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o) {
      heap.push_back (o->obj ().transformed (o->trans ()));
      scanner.insert (& heap.back (), n);
      n += 2;
    }

  } else {

    std::set<db::PolygonRef> polygons;
    for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      polygons.insert (interactions.subject_shape (i->first));
      for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
        polygons.insert (interactions.intruder_shape (*j).second);
      }
    }

    size_t n = 0;
    for (std::set<db::PolygonRef>::const_iterator o = polygons.begin (); o != polygons.end (); ++o) {
      heap.push_back (o->obj ().transformed (o->trans ()));
      scanner.insert (& heap.back (), n);
      n += 2;
    }

  }

  do {
    scanner.process (poly_check, m_check.distance (), db::box_convert<db::Polygon> ());
  } while (edge_check.prepare_next_pass ());
}

db::Coord
CheckLocalOperation::dist () const
{
  //  TODO: will the distance be sufficient? Or should we take somewhat more?
  return m_check.distance ();
}

CheckLocalOperation::on_empty_intruder_mode
CheckLocalOperation::on_empty_intruder_hint () const
{
  return m_different_polygons ? Drop : Ignore;
}

std::string
CheckLocalOperation::description () const
{
  return tl::to_string (tr ("Generic DRC check"));
}

// ---------------------------------------------------------------------------------------------------------------

InteractingLocalOperation::InteractingLocalOperation (int mode, bool touching, bool inverse, size_t min_count, size_t max_count)
  : m_mode (mode), m_touching (touching), m_inverse (inverse), m_min_count (std::max (size_t (1), min_count)), m_max_count (max_count)
{
  //  .. nothing yet ..
}

db::Coord InteractingLocalOperation::dist () const
{
  return m_touching ? 1 : 0;
}

void InteractingLocalOperation::compute_local (db::Layout * /*layout*/, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<db::PolygonRef> &result = results.front ();

  db::EdgeProcessor ep;

  std::set<db::PolygonRef> others;
  for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  size_t nstart = 0;

  if (m_min_count == size_t (1) && m_max_count == std::numeric_limits<size_t>::max ()) {

    for (std::set<db::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o) {
      for (db::PolygonRef::polygon_edge_iterator e = o->begin_edge (); ! e.at_end(); ++e) {
        ep.insert (*e, nstart);
      }
    }
    nstart++;

  } else {

    tl_assert (m_mode == 0);

    for (std::set<db::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o) {
      for (db::PolygonRef::polygon_edge_iterator e = o->begin_edge (); ! e.at_end(); ++e) {
        ep.insert (*e, nstart);
      }
      nstart++;
    }

  }

  size_t n = nstart;
  for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i, ++n) {
    const db::PolygonRef &subject = interactions.subject_shape (i->first);
    for (db::PolygonRef::polygon_edge_iterator e = subject.begin_edge (); ! e.at_end(); ++e) {
      ep.insert (*e, n);
    }
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
  for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i, ++n) {
    size_t count = 0;
    std::map <size_t, size_t>::const_iterator c = interaction_counts.find (n);
    if (c != interaction_counts.end ()) {
      count = c->second;
    }
    if ((count >= m_min_count && count <= m_max_count) != m_inverse) {
      const db::PolygonRef &subject = interactions.subject_shape (i->first);
      result.insert (subject);
    }
  }
}

InteractingLocalOperation::on_empty_intruder_mode
InteractingLocalOperation::on_empty_intruder_hint () const
{
  if ((m_mode <= 0) != m_inverse) {
    return Drop;
  } else {
    return Copy;
  }
}

std::string InteractingLocalOperation::description () const
{
  return tl::to_string (tr ("Select regions by their geometric relation (interacting, inside, outside ..)"));
}

// ---------------------------------------------------------------------------------------------------------------

PullLocalOperation::PullLocalOperation (int mode, bool touching)
  : m_mode (mode), m_touching (touching)
{
  //  .. nothing yet ..
}

db::Coord PullLocalOperation::dist () const
{
  return m_touching ? 1 : 0;
}

void PullLocalOperation::compute_local (db::Layout * /*layout*/, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<db::PolygonRef> &result = results.front ();

  db::EdgeProcessor ep;

  std::set<db::PolygonRef> others;
  for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      others.insert (interactions.intruder_shape (*j).second);
    }
  }

  for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    const db::PolygonRef &subject = interactions.subject_shape (i->first);
    for (db::PolygonRef::polygon_edge_iterator e = subject.begin_edge (); ! e.at_end(); ++e) {
      ep.insert (*e, 0);
    }
  }

  size_t n = 1;
  for (std::set<db::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o, ++n) {
    for (db::PolygonRef::polygon_edge_iterator e = o->begin_edge (); ! e.at_end(); ++e) {
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
  for (std::set<db::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o, ++n) {
    if (selected.find (n) != selected.end ()) {
      result.insert (*o);
    }
  }
}

PullLocalOperation::on_empty_intruder_mode PullLocalOperation::on_empty_intruder_hint () const
{
  return Drop;
}

std::string PullLocalOperation::description () const
{
  return tl::to_string (tr ("Pull regions by their geometrical relation to first"));
}

// ---------------------------------------------------------------------------------------------------------------

InteractingWithEdgeLocalOperation::InteractingWithEdgeLocalOperation (bool inverse, size_t min_count, size_t max_count)
  : m_inverse (inverse), m_min_count (std::max (size_t (1), min_count)), m_max_count (max_count)
{
  //  .. nothing yet ..
}

db::Coord InteractingWithEdgeLocalOperation::dist () const
{
  //  touching is sufficient
  return 1;
}

void InteractingWithEdgeLocalOperation::compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::Edge> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
{
  std::unordered_map<db::PolygonRef, size_t> counted_results;
  bool counting = !(m_min_count == 1 && m_max_count == std::numeric_limits<size_t>::max ());

  db::box_scanner2<db::Polygon, size_t, db::Edge, size_t> scanner;

  ResultCountingInserter inserter (layout, counted_results);
  region_to_edge_interaction_filter<ResultCountingInserter> filter (inserter, false, counting /*get all in counting mode*/);

  std::set<unsigned int> intruder_ids;
  for (shape_interactions<db::PolygonRef, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (shape_interactions<db::PolygonRef, db::Edge>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      intruder_ids.insert (*j);
    }
  }

  for (std::set<unsigned int>::const_iterator j = intruder_ids.begin (); j != intruder_ids.end (); ++j) {
    scanner.insert2 (& interactions.intruder_shape (*j).second, 0);
  }

  std::list<db::Polygon> heap;
  for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

    const db::PolygonRef &subject = interactions.subject_shape (i->first);
    heap.push_back (subject.obj ().transformed (subject.trans ()));

    scanner.insert1 (&heap.back (), 0);
    if (m_inverse) {
      inserter.init (heap.back ());
    }

  }

  scanner.process (filter, 1, db::box_convert<db::Polygon> (), db::box_convert<db::Edge> ());

  //  select hits based on their count

  tl_assert (results.size () == 1);
  std::unordered_set<db::PolygonRef> &result = results.front ();

  for (std::unordered_map<db::PolygonRef, size_t>::const_iterator r = counted_results.begin (); r != counted_results.end (); ++r) {
    bool hit = r->second >= m_min_count && r->second <= m_max_count;
    if (hit != m_inverse) {
      result.insert (r->first);
    }
  }
}

InteractingWithEdgeLocalOperation::on_empty_intruder_mode InteractingWithEdgeLocalOperation::on_empty_intruder_hint () const
{
  if (!m_inverse) {
    return Drop;
  } else {
    return Copy;
  }
}

std::string InteractingWithEdgeLocalOperation::description () const
{
  return tl::to_string (tr ("Select regions by their geometric relation to edges"));
}

// ---------------------------------------------------------------------------------------------------------------

PullWithEdgeLocalOperation::PullWithEdgeLocalOperation ()
{
  //  .. nothing yet ..
}

db::Coord PullWithEdgeLocalOperation::dist () const
{
  //  touching is sufficient
  return 1;
}

void PullWithEdgeLocalOperation::compute_local (db::Layout *, const shape_interactions<db::PolygonRef, db::Edge> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<db::Edge> &result = results.front ();

  db::box_scanner2<db::Polygon, size_t, db::Edge, size_t> scanner;

  EdgeResultInserter inserter (result);
  region_to_edge_interaction_filter<EdgeResultInserter> filter (inserter, false);

  for (shape_interactions<db::PolygonRef, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (shape_interactions<db::PolygonRef, db::Edge>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      scanner.insert2 (& interactions.intruder_shape (*j).second, 0);
    }
  }

  std::list<db::Polygon> heap;
  for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

    const db::PolygonRef &subject = interactions.subject_shape (i->first);
    heap.push_back (subject.obj ().transformed (subject.trans ()));

    scanner.insert1 (&heap.back (), 0);

  }

  scanner.process (filter, 1, db::box_convert<db::Polygon> (), db::box_convert<db::Edge> ());
}

PullWithEdgeLocalOperation::on_empty_intruder_mode PullWithEdgeLocalOperation::on_empty_intruder_hint () const
{
  return Drop;
}

std::string PullWithEdgeLocalOperation::description () const
{
  return tl::to_string (tr ("Pull edges from second by their geometric relation to first"));
}

// ---------------------------------------------------------------------------------------------------------------

PullWithTextLocalOperation::PullWithTextLocalOperation ()
{
  //  .. nothing yet ..
}

db::Coord PullWithTextLocalOperation::dist () const
{
  //  touching is sufficient
  return 1;
}

void PullWithTextLocalOperation::compute_local (db::Layout *, const shape_interactions<db::PolygonRef, db::TextRef> &interactions, std::vector<std::unordered_set<db::TextRef> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
{
  tl_assert (results.size () == 1);
  std::unordered_set<db::TextRef> &result = results.front ();

  db::box_scanner2<db::Polygon, size_t, db::TextRef, size_t> scanner;

  TextResultInserter inserter (result);
  region_to_text_interaction_filter<TextResultInserter, db::TextRef> filter (inserter, false);

  for (shape_interactions<db::PolygonRef, db::TextRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (shape_interactions<db::PolygonRef, db::TextRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      scanner.insert2 (& interactions.intruder_shape (*j).second, 0);
    }
  }

  std::set<unsigned int> intruder_ids;
  for (shape_interactions<db::PolygonRef, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (shape_interactions<db::PolygonRef, db::Edge>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      intruder_ids.insert (*j);
    }
  }

  for (std::set<unsigned int>::const_iterator j = intruder_ids.begin (); j != intruder_ids.end (); ++j) {
    scanner.insert2 (& interactions.intruder_shape (*j).second, 0);
  }

  std::list<db::Polygon> heap;
  for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

    const db::PolygonRef &subject = interactions.subject_shape (i->first);
    heap.push_back (subject.obj ().transformed (subject.trans ()));

    scanner.insert1 (&heap.back (), 0);

  }

  scanner.process (filter, 1, db::box_convert<db::Polygon> (), db::box_convert<db::TextRef> ());
}

PullWithTextLocalOperation::on_empty_intruder_mode PullWithTextLocalOperation::on_empty_intruder_hint () const
{
  return Drop;
}

std::string PullWithTextLocalOperation::description () const
{
  return tl::to_string (tr ("Pull texts from second by their geometric relation to first"));
}

// ---------------------------------------------------------------------------------------------------------------

InteractingWithTextLocalOperation::InteractingWithTextLocalOperation (bool inverse, size_t min_count, size_t max_count)
  : m_inverse (inverse), m_min_count (std::max (size_t (1), min_count)), m_max_count (max_count)
{
  //  .. nothing yet ..
}

db::Coord InteractingWithTextLocalOperation::dist () const
{
  //  touching is sufficient
  return 1;
}

void InteractingWithTextLocalOperation::compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::TextRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
{
  std::unordered_map<db::PolygonRef, size_t> counted_results;
  bool counting = !(m_min_count == 1 && m_max_count == std::numeric_limits<size_t>::max ());

  db::box_scanner2<db::Polygon, size_t, db::TextRef, size_t> scanner;

  ResultCountingInserter inserter (layout, counted_results);
  region_to_text_interaction_filter<ResultCountingInserter, db::TextRef> filter (inserter, false, counting /*get all in counting mode*/);

  std::set<unsigned int> intruder_ids;
  for (shape_interactions<db::PolygonRef, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
    for (shape_interactions<db::PolygonRef, db::Edge>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
      intruder_ids.insert (*j);
    }
  }

  for (std::set<unsigned int>::const_iterator j = intruder_ids.begin (); j != intruder_ids.end (); ++j) {
    scanner.insert2 (& interactions.intruder_shape (*j).second, 0);
  }

  std::list<db::Polygon> heap;
  for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

    const db::PolygonRef &subject = interactions.subject_shape (i->first);
    heap.push_back (subject.obj ().transformed (subject.trans ()));

    scanner.insert1 (&heap.back (), 0);
    if (m_inverse) {
      inserter.init (heap.back ());
    }

  }

  scanner.process (filter, 1, db::box_convert<db::Polygon> (), db::box_convert<db::TextRef> ());

  //  select hits based on their count

  tl_assert (results.size () == 1);
  std::unordered_set<db::PolygonRef> &result = results.front ();

  for (std::unordered_map<db::PolygonRef, size_t>::const_iterator r = counted_results.begin (); r != counted_results.end (); ++r) {
    bool hit = r->second >= m_min_count && r->second <= m_max_count;
    if (hit != m_inverse) {
      result.insert (r->first);
    }
  }
}

InteractingWithTextLocalOperation::on_empty_intruder_mode InteractingWithTextLocalOperation::on_empty_intruder_hint () const
{
  if (!m_inverse) {
    return Drop;
  } else {
    return Copy;
  }
}

std::string InteractingWithTextLocalOperation::description () const
{
  return tl::to_string (tr ("Select regions by their geometric relation to texts"));
}

}
