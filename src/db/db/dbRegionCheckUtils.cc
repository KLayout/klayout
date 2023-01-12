
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


#include "dbRegionCheckUtils.h"
#include "dbPolygonTools.h"
#include "dbEdgeBoolean.h"
#include "tlSelect.h"

namespace db
{

// -------------------------------------------------------------------------------------
//  Edge2EdgeCheckBase implementation

Edge2EdgeCheckBase::Edge2EdgeCheckBase (const EdgeRelationFilter &check, bool different_polygons, bool requires_different_layers, bool with_shielding, bool symmetric_edges)
  : mp_check (&check), m_requires_different_layers (requires_different_layers), m_different_polygons (different_polygons),
    m_first_pseudo (std::numeric_limits<size_t>::max ()),
    m_with_shielding (with_shielding),
    m_symmetric_edges (symmetric_edges),
    m_has_edge_pair_output (true),
    m_has_negative_edge_output (false),
    m_pass (0)
{
  m_distance = check.distance ();
}

bool
Edge2EdgeCheckBase::prepare_next_pass ()
{
  ++m_pass;

  if (m_pass == 1) {

    m_first_pseudo = m_ep.size ();

    if (m_with_shielding && ! m_ep.empty ()) {

      m_ep_discarded.resize (m_ep.size (), false);

      //  second pass:
      return true;

    } else if (m_has_negative_edge_output) {

      //  second pass:
      return true;

    }

  }

  if (! m_ep.empty () && m_has_edge_pair_output) {

    std::vector<bool>::const_iterator d = m_ep_discarded.begin ();
    std::vector<bool>::const_iterator i = m_ep_intra_polygon.begin ();
    std::vector<db::EdgePair>::const_iterator ep = m_ep.begin ();
    while (ep != m_ep.end () && size_t (ep - m_ep.begin ()) < m_first_pseudo) {
      bool use_result = true;
      if (d != m_ep_discarded.end ()) {
        use_result = ! *d;
        ++d;
      }
      if (use_result) {
        put (*ep, *i);
      }
      ++ep;
      ++i;
    }

  }

  return false;
}

static inline bool shields (const db::EdgePair &ep, const db::Edge &q)
{
  db::Edge pe1 (ep.first ().p1 (), ep.second ().p2 ());
  db::Edge pe2 (ep.second ().p1 (), ep.first ().p2 ());

  std::pair<bool, db::Point> ip1 = pe1.intersect_point (q);
  std::pair<bool, db::Point> ip2 = pe2.intersect_point (q);

  if (ip1.first && ip2.first) {
    return ip1.second != ip2.second || (pe1.side_of (q.p1 ()) != 0 && pe2.side_of (q.p2 ()) != 0);
  } else {
    return false;
  }
}

void
Edge2EdgeCheckBase::finish (const Edge *o, size_t p)
{
  if (m_has_negative_edge_output && m_pass == 1 && m_pseudo_edges.find (std::make_pair (*o, p)) == m_pseudo_edges.end ()) {

    std::pair<db::Edge, size_t> k (*o, p);
    std::multimap<std::pair<db::Edge, size_t>, size_t>::const_iterator i0 = m_e2ep.find (k);

    bool fully_removed = false;
    bool any = false;
    for (std::multimap<std::pair<db::Edge, size_t>, size_t>::const_iterator i = i0; ! fully_removed && i != m_e2ep.end () && i->first == k; ++i) {
      size_t n = i->second / 2;
      if (n >= m_ep_discarded.size () || !m_ep_discarded [n]) {
        any = true;
        fully_removed = (((i->second & 1) == 0 ? m_ep [n].first () : m_ep [n].second ()) == *o);
      }
    }

    if (! any) {

      put_negative (*o, (int) p);

    } else if (! fully_removed) {

      std::set<db::Edge> partial_edges;

      db::EdgeBooleanCluster<std::set<db::Edge> > ec (&partial_edges, db::EdgeNot);
      ec.add (o, 0);

      for (std::multimap<std::pair<db::Edge, size_t>, size_t>::const_iterator i = i0; i != m_e2ep.end () && i->first == k; ++i) {
        size_t n = i->second / 2;
        if (n >= m_ep_discarded.size () || !m_ep_discarded [n]) {
          ec.add (((i->second & 1) == 0 ? &m_ep [n].first () : &m_ep [n].second ()), 1);
        }
      }

      ec.finish ();

      for (std::set<db::Edge>::const_iterator e = partial_edges.begin (); e != partial_edges.end (); ++e) {
        put_negative (*e, (int) p);
      }

    }

  }
}

bool
Edge2EdgeCheckBase::feed_pseudo_edges (db::box_scanner<db::Edge, size_t> &scanner)
{
  if (m_pass == 1) {
    for (std::set<std::pair<db::Edge, size_t> >::const_iterator e = m_pseudo_edges.begin (); e != m_pseudo_edges.end (); ++e) {
      scanner.insert (&e->first, e->second);
    }
    return ! m_pseudo_edges.empty ();
  } else {
    return false;
  }
}

inline bool edges_considered (bool requires_different_polygons, bool requires_different_layers, size_t p1, size_t p2)
{
  if (p1 == p2) {
    if (requires_different_polygons) {
      return false;
    } else if ((p1 & size_t (1)) != 0) {
      //  edges from the same polygon are only considered on first layer.
      //  Reasoning: this case happens when "intruder" polygons are put on layer 1
      //  while "subject" polygons are put on layer 0. We don't want "intruders"
      //  to generate intra-polygon markers.
      return false;
    }
  }

  if (((p1 ^ p2) & size_t (1)) == 0) {
    if (requires_different_layers) {
      return false;
    } else if ((p1 & size_t (1)) != 0) {
      //  edges on the same layer are only considered on first layer.
      //  Reasoning: this case happens when "intruder" polygons are put on layer 1
      //  while "subject" polygons are put on layer 0. We don't want "intruders"
      //  to generate inter-polygon markers between them.
      return false;
    }
  }

  return true;
}

void
Edge2EdgeCheckBase::add (const db::Edge *o1, size_t p1, const db::Edge *o2, size_t p2)
{
  if (m_pass == 0) {

    //  Overlap or inside checks require input from different layers
    if (edges_considered (m_different_polygons, m_requires_different_layers, p1, p2)) {

      //  ensure that the first check argument is of layer 1 and the second of
      //  layer 2 (unless both are of the same layer)
      int l1 = int (p1 & size_t (1));
      int l2 = int (p2 & size_t (1));

      if (l1 > l2) {
        std::swap (o1, o2);
        std::swap (p1, p2);
      }

      db::EdgePair ep;
      if (mp_check->check (*o1, *o2, &ep)) {

        ep.set_symmetric (m_symmetric_edges);

        //  found a violation: store inside the local buffer for now. In the second
        //  pass we will eliminate those which are shielded completely (with shielding)
        //  and/or compute the negative edges.
        size_t n = m_ep.size ();

        m_ep.push_back (ep);
        m_ep_intra_polygon.push_back (p1 == p2);

        m_e2ep.insert (std::make_pair (std::make_pair (*o1, p1), n * 2));
        m_e2ep.insert (std::make_pair (std::make_pair (*o2, p2), n * 2 + 1));

        if (m_has_negative_edge_output) {

          bool antiparallel = (mp_check->relation () == WidthRelation || mp_check->relation () == SpaceRelation);

          //  pseudo1 and pseudo2 are the connecting edges of the edge pairs. Together with the
          //  original edges they form a quadrangle.
          db::Edge pseudo1 (ep.first ().p1 (), antiparallel ? ep.second ().p2 () : ep.second ().p1 ());
          db::Edge pseudo2 (antiparallel ? ep.second ().p1 () : ep.second ().p2 (), ep.first ().p2 ());

          m_pseudo_edges.insert (std::make_pair (pseudo1, p1));
          m_pseudo_edges.insert (std::make_pair (pseudo2, p1));
          if (p1 != p2) {
            m_pseudo_edges.insert (std::make_pair (pseudo1, p2));
            m_pseudo_edges.insert (std::make_pair (pseudo2, p2));
          }

        }

      }

    }

  } else {

    //  set the discarded flags for shielded output
    if (m_with_shielding) {

      //  a simple (complete) shielding implementation which is based on the
      //  assumption that shielding is relevant as soon as a foreign edge cuts through
      //  both of the edge pair's connecting edges.

      //  TODO: this implementation does not take into account the nature of the
      //  EdgePair - because of "whole_edge" it may not reflect the part actually
      //  violating the distance.

      std::vector<size_t> n1, n2;

      for (unsigned int p = 0; p < 2; ++p) {

        std::pair<db::Edge, size_t> k (*o1, p1);
        for (std::multimap<std::pair<db::Edge, size_t>, size_t>::const_iterator i = m_e2ep.find (k); i != m_e2ep.end () && i->first == k; ++i) {
          size_t n = i->second / 2;
          if (n < m_first_pseudo && ! m_ep_discarded [n]) {
            n1.push_back (n);
          }
        }

        std::sort (n1.begin (), n1.end ());

        std::swap (o1, o2);
        std::swap (p1, p2);
        n1.swap (n2);

      }

      for (unsigned int p = 0; p < 2; ++p) {

        std::vector<size_t> nn;
        std::set_difference (n1.begin (), n1.end (), n2.begin (), n2.end (), std::back_inserter (nn));

        for (std::vector<size_t>::const_iterator i = nn.begin (); i != nn.end (); ++i) {
          db::EdgePair ep = m_ep [*i].normalized ();
          if (shields (ep, *o2)) {
            m_ep_discarded [*i] = true;
          }
        }

        std::swap (o1, o2);
        std::swap (p1, p2);
        n1.swap (n2);

      }

    }

    //  for negative output edges are cancelled by short interactions perpendicular to them
    //  For this we have generated "pseudo edges" running along the sides of the original violation. We now check a real
    //  edge vs. a pseudo edge with the same conditions as the normal interaction and add them to the results. In the
    //  negative case this means we cancel a real edge.

    if (m_has_negative_edge_output &&
      (m_pseudo_edges.find (std::make_pair (*o1, p1)) != m_pseudo_edges.end ()) != (m_pseudo_edges.find (std::make_pair (*o2, p2)) != m_pseudo_edges.end ())) {

      //  Overlap or inside checks require input from different layers
      if (edges_considered (m_different_polygons, m_requires_different_layers, p1, p2)) {

        //  ensure that the first check argument is of layer 1 and the second of
        //  layer 2 (unless both are of the same layer)
        int l1 = int (p1 & size_t (1));
        int l2 = int (p2 & size_t (1));

        if (l1 > l2) {
          std::swap (o1, o2);
          std::swap (p1, p2);
        }

        db::EdgePair ep;
        if (mp_check->check (*o1, *o2, &ep)) {

          size_t n = m_ep.size ();

          m_ep.push_back (ep);
          m_ep_intra_polygon.push_back (p1 == p2);  //  not really required, but there for consistency

          m_e2ep.insert (std::make_pair (std::make_pair (*o1, p1), n * 2));
          m_e2ep.insert (std::make_pair (std::make_pair (*o2, p2), n * 2 + 1));

        }

      }

    }

  }

}

/**
 *  @brief Gets a value indicating whether the check requires different layers
 */
bool
Edge2EdgeCheckBase::requires_different_layers () const
{
  return m_requires_different_layers;
}

/**
 *  @brief Sets a value indicating whether the check requires different layers
 */
void
Edge2EdgeCheckBase::set_requires_different_layers (bool f)
{
  m_requires_different_layers = f;
}

/**
 *  @brief Gets a value indicating whether the check requires different layers
 */
bool
Edge2EdgeCheckBase::different_polygons () const
{
  return m_different_polygons;
}

/**
 *  @brief Sets a value indicating whether the check requires different layers
 */
void
Edge2EdgeCheckBase::set_different_polygons (bool f)
{
  m_different_polygons = f;
}

/**
 *  @brief Gets the distance value
 */
EdgeRelationFilter::distance_type
Edge2EdgeCheckBase::distance () const
{
  return m_distance;
}

// -------------------------------------------------------------------------------------
//  Poly2PolyCheckBase implementation

template <class PolygonType>
poly2poly_check<PolygonType>::poly2poly_check (Edge2EdgeCheckBase &output)
  : mp_output (& output)
{
  //  .. nothing yet ..
}

template <class PolygonType>
poly2poly_check<PolygonType>::poly2poly_check ()
  : mp_output (0)
{
  //  .. nothing yet ..
}

static size_t vertices (const db::Polygon &p)
{
  return p.vertices ();
}

static size_t vertices (const db::PolygonRef &p)
{
  return p.obj ().vertices ();
}

template <class PolygonType>
void
poly2poly_check<PolygonType>::single (const PolygonType &o, size_t p)
{
  tl_assert (! mp_output->requires_different_layers () && ! mp_output->different_polygons ());

  //  finally we check the polygons vs. itself for checks involving intra-polygon interactions

  m_scanner.clear ();
  m_scanner.reserve (vertices (o));

  m_edge_heap.clear ();

  for (typename PolygonType::polygon_edge_iterator e = o.begin_edge (); ! e.at_end (); ++e) {
    m_edge_heap.push_back (*e);
    m_scanner.insert (& m_edge_heap.back (), p);
  }

  mp_output->feed_pseudo_edges (m_scanner);

  m_scanner.process (*mp_output, mp_output->distance (), db::box_convert<db::Edge> ());
}

template <class PolygonType>
void
poly2poly_check<PolygonType>::connect (Edge2EdgeCheckBase &output)
{
  mp_output = &output;
  clear ();
}

template <class PolygonType>
void
poly2poly_check<PolygonType>::clear ()
{
  m_scanner.clear ();
  m_edge_heap.clear ();
}

template <class PolygonType>
void
poly2poly_check<PolygonType>::enter (const PolygonType &o, size_t p)
{
  for (typename PolygonType::polygon_edge_iterator e = o.begin_edge (); ! e.at_end (); ++e) {
    m_edge_heap.push_back (*e);
    m_scanner.insert (& m_edge_heap.back (), p);
  }
}

template <class PolygonType>
void
poly2poly_check<PolygonType>::enter (const poly2poly_check<PolygonType>::edge_type &e, size_t p)
{
  m_edge_heap.push_back (e);
  m_scanner.insert (& m_edge_heap.back (), p);
}

//  TODO: move to generic header
static bool interact (const db::Box &box, const db::Edge &e)
{
  if (! e.bbox ().touches (box)) {
    return false;
  } else if (e.is_ortho ()) {
    return true;
  } else {
    return e.clipped (box).first;
  }
}

template <class PolygonType>
void
poly2poly_check<PolygonType>::enter (const PolygonType &o, size_t p, const poly2poly_check<PolygonType>::box_type &box)
{
  if (box.empty ()) {
    return;
  }

  for (typename PolygonType::polygon_edge_iterator e = o.begin_edge (); ! e.at_end (); ++e) {
    if (interact (box, *e)) {
      m_edge_heap.push_back (*e);
      m_scanner.insert (& m_edge_heap.back (), p);
    }
  }
}

template <class PolygonType>
void
poly2poly_check<PolygonType>::enter (const poly2poly_check<PolygonType>::edge_type &e, size_t p, const poly2poly_check<PolygonType>::box_type &box)
{
  if (! box.empty () && interact (box, e)) {
    m_edge_heap.push_back (e);
    m_scanner.insert (& m_edge_heap.back (), p);
  }
}

template <class PolygonType>
void
poly2poly_check<PolygonType>::process ()
{
  mp_output->feed_pseudo_edges (m_scanner);
  m_scanner.process (*mp_output, mp_output->distance (), db::box_convert<db::Edge> ());
}

//  explicit instantiations
template class poly2poly_check<db::Polygon>;
template class poly2poly_check<db::PolygonRef>;

// -------------------------------------------------------------------------------------
//  RegionToEdgeInteractionFilterBase implementation

template <class PolygonType, class EdgeType, class OutputType>
region_to_edge_interaction_filter_base<PolygonType, EdgeType, OutputType>::region_to_edge_interaction_filter_base (bool inverse, bool get_all)
  : m_inverse (inverse), m_get_all (get_all)
{
  //  .. nothing yet ..
}

template <class PolygonType, class EdgeType, class OutputType>
void
region_to_edge_interaction_filter_base<PolygonType, EdgeType, OutputType>::preset (const OutputType *s)
{
  m_seen.insert (s);
}

template <class PolygonType, class EdgeType, class OutputType>
void
region_to_edge_interaction_filter_base<PolygonType, EdgeType, OutputType>::add (const PolygonType *p, size_t, const EdgeType *e, size_t)
{
  const OutputType *o = 0;
  tl::select (o, p, e);

  if (m_get_all || (m_seen.find (o) == m_seen.end ()) != m_inverse) {

    //  A polygon and an edge interact if the edge is either inside completely
    //  of at least one edge of the polygon intersects with the edge
    bool interacts = false;
    if (p->box ().contains (e->p1 ()) && db::inside_poly (p->begin_edge (), e->p1 ()) >= 0) {
      interacts = true;
    } else {
      for (typename PolygonType::polygon_edge_iterator pe = p->begin_edge (); ! pe.at_end () && ! interacts; ++pe) {
        if ((*pe).intersect (*e)) {
          interacts = true;
        }
      }
    }

    if (interacts) {
      if (m_inverse) {
        m_seen.erase (o);
      } else {
        if (! m_get_all) {
          m_seen.insert (o);
        }
        put (*o);
      }
    }

  }
}

template <class PolygonType, class EdgeType, class OutputType>
void
region_to_edge_interaction_filter_base<PolygonType, EdgeType, OutputType>::fill_output ()
{
  for (typename std::set<const OutputType *>::const_iterator s = m_seen.begin (); s != m_seen.end (); ++s) {
    put (**s);
  }
}

//  explicit instantiations
template class region_to_edge_interaction_filter_base<db::Polygon, db::Edge, db::Polygon>;
template class region_to_edge_interaction_filter_base<db::PolygonRef, db::Edge, db::PolygonRef>;
template class region_to_edge_interaction_filter_base<db::Polygon, db::Edge, db::Edge>;
template class region_to_edge_interaction_filter_base<db::PolygonRef, db::Edge, db::Edge>;

// -------------------------------------------------------------------------------------
//  RegionToTextInteractionFilterBase implementation

template <class PolygonType, class TextType, class OutputType>
region_to_text_interaction_filter_base<PolygonType, TextType, OutputType>::region_to_text_interaction_filter_base (bool inverse, bool get_all)
  : m_inverse (inverse), m_get_all (get_all)
{
  //  .. nothing yet ..
}

template <class PolygonType, class TextType, class OutputType>
void
region_to_text_interaction_filter_base<PolygonType, TextType, OutputType>::preset (const OutputType *s)
{
  m_seen.insert (s);
}

template <class PolygonType, class TextType, class OutputType>
void
region_to_text_interaction_filter_base<PolygonType, TextType, OutputType>::add (const PolygonType *p, size_t, const TextType *t, size_t)
{
  const OutputType *o = 0;
  tl::select (o, p, t);

  if (m_get_all || (m_seen.find (o) == m_seen.end ()) != m_inverse) {

    //  A polygon and an text interact if the text is either inside completely
    //  of at least one text of the polygon intersects with the text
    db::Point pt = db::box_convert<TextType> () (*t).p1 ();
    if (p->box ().contains (pt) && db::inside_poly (p->begin_edge (), pt) >= 0) {
      if (m_inverse) {
        m_seen.erase (o);
      } else {
        if (! m_get_all) {
          m_seen.insert (o);
        }
        put (*o);
      }
    }

  }
}

template <class PolygonType, class TextType, class OutputType>
void
region_to_text_interaction_filter_base<PolygonType, TextType, OutputType>::fill_output ()
{
  for (typename std::set<const OutputType *>::const_iterator s = m_seen.begin (); s != m_seen.end (); ++s) {
    put (**s);
  }
}

//  explicit instantiations
template class region_to_text_interaction_filter_base<db::PolygonRef, db::TextRef, db::PolygonRef>;
template class region_to_text_interaction_filter_base<db::Polygon, db::Text, db::Polygon>;
template class region_to_text_interaction_filter_base<db::Polygon, db::Text, db::Text>;
template class region_to_text_interaction_filter_base<db::Polygon, db::TextRef, db::TextRef>;
template class region_to_text_interaction_filter_base<db::PolygonRef, db::TextRef, db::TextRef>;

}
