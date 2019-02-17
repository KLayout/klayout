
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


#ifndef HDR_dbAsIfFlatRegion
#define HDR_dbAsIfFlatRegion

#include "dbCommon.h"

#include "dbRegionDelegate.h"
#include "dbPolygon.h"
#include "dbEdge.h"
#include "dbBoxScanner.h"
#include "dbEdgePairRelations.h"

#include <set>

namespace db {

/**
 *  @brief A helper class for the DRC functionality which acts as an edge pair receiver
 */
template <class Output>
class edge2edge_check
  : public db::box_scanner_receiver<db::Edge, size_t>
{
public:
  edge2edge_check (const EdgeRelationFilter &check, Output &output, bool different_polygons, bool requires_different_layers)
    : mp_check (&check), mp_output (&output), m_requires_different_layers (requires_different_layers), m_different_polygons (different_polygons),
      m_pass (0)
  {
    m_distance = check.distance ();
  }

  bool prepare_next_pass ()
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
          mp_output->insert (*ep);
        }
        ++d;
        ++ep;
      }

    }

    return false;
  }

  void add (const db::Edge *o1, size_t p1, const db::Edge *o2, size_t p2)
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
  bool requires_different_layers () const
  {
    return m_requires_different_layers;
  }

  /**
   *  @brief Sets a value indicating whether the check requires different layers
   */
  void set_requires_different_layers (bool f)
  {
    m_requires_different_layers = f;
  }

  /**
   *  @brief Gets a value indicating whether the check requires different layers
   */
  bool different_polygons () const
  {
    return m_different_polygons;
  }

  /**
   *  @brief Sets a value indicating whether the check requires different layers
   */
  void set_different_polygons (bool f)
  {
    m_different_polygons = f;
  }

  /**
   *  @brief Gets the distance value
   */
  EdgeRelationFilter::distance_type distance () const
  {
    return m_distance;
  }

private:
  const EdgeRelationFilter *mp_check;
  Output *mp_output;
  bool m_requires_different_layers;
  bool m_different_polygons;
  EdgeRelationFilter::distance_type m_distance;
  std::vector<db::EdgePair> m_ep;
  std::multimap<std::pair<db::Edge, size_t>, size_t> m_e2ep;
  std::vector<bool> m_ep_discarded;
  unsigned int m_pass;
};

/**
 *  @brief A helper class for the DRC functionality which acts as an edge pair receiver
 */
template <class Output>
class poly2poly_check
  : public db::box_scanner_receiver<db::Polygon, size_t>
{
public:
  poly2poly_check (edge2edge_check<Output> &output)
    : mp_output (&output)
  {
    //  .. nothing yet ..
  }

  void finish (const db::Polygon *o, size_t p)
  {
    enter (*o, p);
  }

  void enter (const db::Polygon &o, size_t p)
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

  void add (const db::Polygon *o1, size_t p1, const db::Polygon *o2, size_t p2)
  {
    enter (*o1, p1, *o2, p2);
  }

  void enter (const db::Polygon &o1, size_t p1, const db::Polygon &o2, size_t p2)
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

private:
  db::box_scanner<db::Edge, size_t> m_scanner;
  edge2edge_check<Output> *mp_output;
  std::vector<db::Edge> m_edges;
};

/**
 *  @brief A helper class for the region to edge interaction functionality
 */
template <class OutputContainer>
class DB_PUBLIC region_to_edge_interaction_filter
  : public db::box_scanner_receiver2<db::Polygon, size_t, db::Edge, size_t>
{
public:
  region_to_edge_interaction_filter (OutputContainer &output, bool inverse)
    : mp_output (&output), m_inverse (inverse)
  {
    //  .. nothing yet ..
  }

  void preset (const db::Polygon *poly)
  {
    m_seen.insert (poly);
  }

  void add (const db::Polygon *p, size_t, const db::Edge *e, size_t)
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
          mp_output->insert (*p);
        }
      }

    }
  }

  void fill_output ()
  {
    for (std::set<const db::Polygon *>::const_iterator p = m_seen.begin (); p != m_seen.end (); ++p) {
      mp_output->insert (**p);
    }
  }

private:
  OutputContainer *mp_output;
  std::set<const db::Polygon *> m_seen;
  bool m_inverse;
};

/**
 *  @brief Provides default flat implementations
 */
class DB_PUBLIC AsIfFlatRegion
  : public RegionDelegate
{
public:
  AsIfFlatRegion ();
  virtual ~AsIfFlatRegion ();

  virtual bool is_box () const;
  virtual size_t size () const;

  virtual area_type area (const db::Box &box) const;
  virtual perimeter_type perimeter (const db::Box &box) const;
  virtual Box bbox () const;

  virtual std::string to_string (size_t nmax) const;

  EdgePairs width_check (db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_single_polygon_check (db::WidthRelation, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairs space_check (db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::SpaceRelation, false, 0, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairs isolated_check (db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::SpaceRelation, true, 0, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairs notch_check (db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_single_polygon_check (db::SpaceRelation, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairs enclosing_check (const Region &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::OverlapRelation, true, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairs overlap_check (const Region &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::WidthRelation, true, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairs separation_check (const Region &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::SpaceRelation, true, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  EdgePairs inside_check (const Region &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
  {
    return run_check (db::InsideRelation, true, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  virtual EdgePairs grid_check (db::Coord gx, db::Coord gy) const;
  virtual EdgePairs angle_check (double min, double max, bool inverse) const;

  virtual RegionDelegate *snapped_in_place (db::Coord gx, db::Coord gy)
  {
    return snapped (gx, gy);
  }

  virtual RegionDelegate *snapped (db::Coord gx, db::Coord gy);

  virtual Edges edges (const EdgeFilterBase *) const;

  virtual RegionDelegate *filter_in_place (const PolygonFilterBase &filter)
  {
    return filtered (filter);
  }

  virtual RegionDelegate *filtered (const PolygonFilterBase &filter) const;

  virtual RegionDelegate *merged_in_place ()
  {
    return merged ();
  }

  virtual RegionDelegate *merged_in_place (bool min_coherence, unsigned int min_wc)
  {
    return merged (min_coherence, min_wc);
  }

  virtual RegionDelegate *merged () const
  {
    return merged (min_coherence (), 0);
  }

  virtual RegionDelegate *merged (bool min_coherence, unsigned int min_wc) const;

  virtual RegionDelegate *strange_polygon_check () const;

  virtual RegionDelegate *sized (coord_type d, unsigned int mode) const;
  virtual RegionDelegate *sized (coord_type dx, coord_type dy, unsigned int mode) const;

  virtual RegionDelegate *and_with (const Region &other) const;
  virtual RegionDelegate *not_with (const Region &other) const;
  virtual RegionDelegate *xor_with (const Region &other) const;
  virtual RegionDelegate *or_with (const Region &other) const;

  virtual RegionDelegate *add_in_place (const Region &other)
  {
    return add (other);
  }

  virtual RegionDelegate *add (const Region &other) const;

  virtual RegionDelegate *selected_outside (const Region &other) const
  {
    return selected_interacting_generic (other, 1, false, false);
  }

  virtual RegionDelegate *selected_not_outside (const Region &other) const
  {
    return selected_interacting_generic (other, 1, false, true);
  }

  virtual RegionDelegate *selected_inside (const Region &other) const
  {
    return selected_interacting_generic (other, -1, true, false);
  }

  virtual RegionDelegate *selected_not_inside (const Region &other) const
  {
    return selected_interacting_generic (other, -1, true, true);
  }

  virtual RegionDelegate *selected_interacting (const Region &other) const
  {
    return selected_interacting_generic (other, 0, true, false);
  }

  virtual RegionDelegate *selected_not_interacting (const Region &other) const
  {
    return selected_interacting_generic (other, 0, true, true);
  }

  virtual RegionDelegate *selected_interacting (const Edges &other) const
  {
    return selected_interacting_generic (other, false);
  }

  virtual RegionDelegate *selected_not_interacting (const Edges &other) const
  {
    return selected_interacting_generic (other, true);
  }

  virtual RegionDelegate *selected_overlapping (const Region &other) const
  {
    return selected_interacting_generic (other, 0, false, false);
  }

  virtual RegionDelegate *selected_not_overlapping (const Region &other) const
  {
    return selected_interacting_generic (other, 0, false, true);
  }

  virtual RegionDelegate *holes () const;
  virtual RegionDelegate *hulls () const;
  virtual RegionDelegate *in (const Region &other, bool invert) const;
  virtual RegionDelegate *rounded_corners (double rinner, double router, unsigned int n) const;
  virtual RegionDelegate *smoothed (coord_type d) const;

  virtual bool equals (const Region &other) const;
  virtual bool less (const Region &other) const;

  virtual void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const;

protected:
  void update_bbox (const db::Box &box);
  void invalidate_bbox ();

  EdgePairs run_check (db::edge_relation_type rel, bool different_polygons, const Region *other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const;
  EdgePairs run_single_polygon_check (db::edge_relation_type rel, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const;
  RegionDelegate *selected_interacting_generic (const Region &other, int mode, bool touching, bool inverse) const;
  RegionDelegate *selected_interacting_generic (const Edges &other, bool inverse) const;

  static void produce_shape_for_strange_polygon (const db::Polygon &poly, db::Shapes &shapes);
  template <class Trans>
  static void produce_markers_for_grid_check (const db::Polygon &poly, const Trans &tr, db::Coord gx, db::Coord gy, db::Shapes &shapes);
  template <class Trans>
  static void produce_markers_for_angle_check (const db::Polygon &poly, const Trans &tr, double min, double max, bool inverse, db::Shapes &shapes);
  static db::Polygon snapped_polygon (const db::Polygon &poly, db::Coord gx, db::Coord gy, std::vector<db::Point> &heap);

private:
  AsIfFlatRegion &operator= (const AsIfFlatRegion &other);

  mutable bool m_bbox_valid;
  mutable db::Box m_bbox;

  virtual db::Box compute_bbox () const;
  static RegionDelegate *region_from_box (const db::Box &b);
};

}

#endif

