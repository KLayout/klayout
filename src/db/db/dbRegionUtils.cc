
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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

namespace db
{

// -------------------------------------------------------------------------------------
//  Edge2EdgeCheckBase implementation

Edge2EdgeCheckBase::Edge2EdgeCheckBase (const EdgeRelationFilter &check, bool different_polygons, bool requires_different_layers)
  : mp_check (&check), m_requires_different_layers (requires_different_layers), m_different_polygons (different_polygons),
    m_pass (0)
{
  m_distance = check.distance ();
}

bool
Edge2EdgeCheckBase::prepare_next_pass ()
{
  ++m_pass;

  if (m_pass == 1) {

    if (! m_ep.empty ()) {
      m_ep_discarded.resize (m_ep.size (), false);
      return true;
    }

  } else if (m_pass == 2) {

    std::vector<bool>::const_iterator d = m_ep_discarded.begin ();
    std::vector<db::EdgePair>::const_iterator ep = m_ep.begin ();
    while (ep != m_ep.end ()) {
      tl_assert (d != m_ep_discarded.end ());
      if (! *d) {
        put (*ep);
      }
      ++d;
      ++ep;
    }

  }

  return false;
}

void
Edge2EdgeCheckBase::add (const db::Edge *o1, size_t p1, const db::Edge *o2, size_t p2)
{
  if (m_pass == 0) {

    //  Overlap or inside checks require input from different layers
    if ((! m_different_polygons || p1 != p2) && (! m_requires_different_layers || ((p1 ^ p2) & 1) != 0)) {

      //  ensure that the first check argument is of layer 1 and the second of
      //  layer 2 (unless both are of the same layer)
      int l1 = int (p1 & size_t (1));
      int l2 = int (p2 & size_t (1));

      db::EdgePair ep;
      if (mp_check->check (l1 <= l2 ? *o1 : *o2, l1 <= l2 ? *o2 : *o1, &ep)) {

        //  found a violation: store inside the local buffer for now. In the second
        //  pass we will eliminate those which are shielded completely.
        size_t n = m_ep.size ();
        m_ep.push_back (ep);
        m_e2ep.insert (std::make_pair (std::make_pair (*o1, p1), n));
        m_e2ep.insert (std::make_pair (std::make_pair (*o2, p2), n));

      }

    }

  } else {

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
        n1.push_back (i->second);
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
        if (! m_ep_discarded [*i]) {
          db::EdgePair ep = m_ep [*i].normalized ();
          if (db::Edge (ep.first ().p1 (), ep.second ().p2 ()).intersect (*o2) &&
              db::Edge (ep.second ().p1 (), ep.first ().p2 ()).intersect (*o2)) {
            m_ep_discarded [*i] = true;
          }
        }
      }

      std::swap (o1, o2);
      std::swap (p1, p2);
      n1.swap (n2);

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

Poly2PolyCheckBase::Poly2PolyCheckBase (Edge2EdgeCheckBase &output)
  : mp_output (& output)
{
  //  .. nothing yet ..
}

void
Poly2PolyCheckBase::finish (const db::Polygon *o, size_t p)
{
  enter (*o, p);
}

void
Poly2PolyCheckBase::enter (const db::Polygon &o, size_t p)
{
  if (! mp_output->requires_different_layers () && ! mp_output->different_polygons ()) {

    //  finally we check the polygons vs. itself for checks involving intra-polygon interactions

    m_scanner.clear ();
    m_scanner.reserve (o.vertices ());

    m_edges.clear ();
    m_edges.reserve (o.vertices ());

    for (db::Polygon::polygon_edge_iterator e = o.begin_edge (); ! e.at_end (); ++e) {
      m_edges.push_back (*e);
      m_scanner.insert (& m_edges.back (), p);
    }

    tl_assert (m_edges.size () == o.vertices ());

    m_scanner.process (*mp_output, mp_output->distance (), db::box_convert<db::Edge> ());

  }
}

void
Poly2PolyCheckBase::add (const db::Polygon *o1, size_t p1, const db::Polygon *o2, size_t p2)
{
  enter (*o1, p1, *o2, p2);
}

void
Poly2PolyCheckBase::enter (const db::Polygon &o1, size_t p1, const db::Polygon &o2, size_t p2)
{
  if ((! mp_output->different_polygons () || p1 != p2) && (! mp_output->requires_different_layers () || ((p1 ^ p2) & 1) != 0)) {

    m_scanner.clear ();
    m_scanner.reserve (o1.vertices () + o2.vertices ());

    m_edges.clear ();
    m_edges.reserve (o1.vertices () + o2.vertices ());

    for (db::Polygon::polygon_edge_iterator e = o1.begin_edge (); ! e.at_end (); ++e) {
      m_edges.push_back (*e);
      m_scanner.insert (& m_edges.back (), p1);
    }

    for (db::Polygon::polygon_edge_iterator e = o2.begin_edge (); ! e.at_end (); ++e) {
      m_edges.push_back (*e);
      m_scanner.insert (& m_edges.back (), p2);
    }

    tl_assert (m_edges.size () == o1.vertices () + o2.vertices ());

    //  temporarily disable intra-polygon check in that step .. we do that later in finish()
    //  if required (#650).
    bool no_intra = mp_output->different_polygons ();
    mp_output->set_different_polygons (true);

    m_scanner.process (*mp_output, mp_output->distance (), db::box_convert<db::Edge> ());

    mp_output->set_different_polygons (no_intra);

  }
}

// -------------------------------------------------------------------------------------
//  RegionToEdgeInteractionFilterBase implementation

RegionToEdgeInteractionFilterBase::RegionToEdgeInteractionFilterBase (bool inverse)
  : m_inverse (inverse)
{
  //  .. nothing yet ..
}

void
RegionToEdgeInteractionFilterBase::preset (const db::Polygon *poly)
{
  m_seen.insert (poly);
}

void
RegionToEdgeInteractionFilterBase::add (const db::Polygon *p, size_t, const db::Edge *e, size_t)
{
  if ((m_seen.find (p) == m_seen.end ()) != m_inverse) {

    //  A polygon and an edge interact if the edge is either inside completely
    //  of at least one edge of the polygon intersects with the edge
    bool interacts = false;
    if (p->box ().contains (e->p1 ()) && db::inside_poly (p->begin_edge (), e->p1 ()) >= 0) {
      interacts = true;
    } else {
      for (db::Polygon::polygon_edge_iterator pe = p->begin_edge (); ! pe.at_end () && ! interacts; ++pe) {
        if ((*pe).intersect (*e)) {
          interacts = true;
        }
      }
    }

    if (interacts) {
      if (m_inverse) {
        m_seen.erase (p);
      } else {
        m_seen.insert (p);
        put (*p);
      }
    }

  }
}

void
RegionToEdgeInteractionFilterBase::fill_output ()
{
  for (std::set<const db::Polygon *>::const_iterator p = m_seen.begin (); p != m_seen.end (); ++p) {
    put (**p);
  }
}

}

