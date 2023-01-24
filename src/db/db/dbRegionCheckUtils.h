
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


#ifndef HDR_dbRegionCheckUtils
#define HDR_dbRegionCheckUtils

#include "dbCommon.h"
#include "dbCellVariants.h"
#include "dbBoxScanner.h"
#include "dbEdgePairRelations.h"

namespace db {

/**
 *  @brief A helper class for the DRC functionality which acts as an edge pair receiver
 */
class DB_PUBLIC Edge2EdgeCheckBase
  : public db::box_scanner_receiver<db::Edge, size_t>
{
public:
  Edge2EdgeCheckBase (const EdgeRelationFilter &check, bool different_polygons, bool requires_different_layers, bool with_shielding, bool symmetric_edges);

  /**
   *  @brief Call this to initiate a new pass until the return value is false
   */
  bool prepare_next_pass ();

  /**
   *  @brief Before the scanner is run, this method must be called to feed additional edges into the scanner
   *  (required for negative edge output - cancellation of perpendicular edges)
   */
  bool feed_pseudo_edges (db::box_scanner<db::Edge, size_t> &scanner);

  /**
   *  @brief Reimplementation of the box_scanner_receiver interface
   */
  void add (const db::Edge *o1, size_t p1, const db::Edge *o2, size_t p2);

  /**
   *  @brief Reimplementation of the box_scanner_receiver interface
   */
  void finish (const Edge *o, size_t);

  /**
   *  @brief Gets a value indicating whether the check requires different layers
   */
  bool requires_different_layers () const;

  /**
   *  @brief Sets a value indicating whether the check requires different layers
   */
  void set_requires_different_layers (bool f);

  /**
   *  @brief Gets a value indicating whether the check requires different layers
   */
  bool different_polygons () const;

  /**
   *  @brief Sets a value indicating whether the check requires different layers
   */
  void set_different_polygons (bool f);

  /**
   *  @brief Sets a flag indicating that this class wants negative edge output
   */
  void set_has_negative_edge_output (bool f)
  {
    m_has_negative_edge_output = f;
  }

  /**
   *  @brief Gets a flag indicating that this class wants negative edge output
   */
  bool has_negative_edge_output () const
  {
    return m_has_negative_edge_output;
  }

  /**
   *  @brief Sets a flag indicating that this class wants normal edge pair output
   */
  void set_has_edge_pair_output (bool f)
  {
    m_has_edge_pair_output = f;
  }

  /**
   *  @brief Gets a flag indicating that this class wants normal edge pair output
   */
  bool has_edge_pair_output () const
  {
    return m_has_edge_pair_output;
  }

  /**
   *  @brief Gets the distance value
   */
  EdgeRelationFilter::distance_type distance () const;

protected:
  /**
   *  @brief Normal edge pair output (violations)
   */
  virtual void put (const db::EdgePair & /*edge*/, bool /*intra-polygon*/) const { }

  /**
   *  @brief Negative edge output
   */
  virtual void put_negative (const db::Edge & /*edge*/, int /*layer*/) const { }

private:
  const EdgeRelationFilter *mp_check;
  bool m_requires_different_layers;
  bool m_different_polygons;
  EdgeRelationFilter::distance_type m_distance;
  std::vector<db::EdgePair> m_ep;
  std::multimap<std::pair<db::Edge, size_t>, size_t> m_e2ep;
  std::set<std::pair<db::Edge, size_t> > m_pseudo_edges;
  size_t m_first_pseudo;
  std::vector<bool> m_ep_discarded, m_ep_intra_polygon;
  bool m_with_shielding;
  bool m_symmetric_edges;
  bool m_has_edge_pair_output;
  bool m_has_negative_edge_output;
  unsigned int m_pass;
};

/**
 *  @brief A helper class for the DRC functionality
 *
 *  This class implements the edge-to-edge part of the polygon DRC.
 */
template <class Output>
class DB_PUBLIC_TEMPLATE edge2edge_check
  : public Edge2EdgeCheckBase
{
public:
  edge2edge_check (const EdgeRelationFilter &check, Output &output, bool different_polygons, bool requires_different_layers, bool with_shielding, bool symmetric_edges, db::properties_id_type prop_id = 0)
    : Edge2EdgeCheckBase (check, different_polygons, requires_different_layers, with_shielding, symmetric_edges), mp_output_inter (&output), mp_output_intra (0), m_prop_id (prop_id)
  {
    //  .. nothing yet ..
  }

  edge2edge_check (const EdgeRelationFilter &check, Output &output_inter, Output &output_intra, bool different_polygons, bool requires_different_layers, bool with_shielding, bool symmetric_edges, db::properties_id_type prop_id = 0)
    : Edge2EdgeCheckBase (check, different_polygons, requires_different_layers, with_shielding, symmetric_edges), mp_output_inter (&output_inter), mp_output_intra (&output_intra), m_prop_id (prop_id)
  {
    //  .. nothing yet ..
  }

protected:
  void put (const db::EdgePair &edge, bool inter_polygon) const
  {
    if (! inter_polygon || ! mp_output_intra) {
      if (m_prop_id != 0) {
        mp_output_inter->insert (db::EdgePairWithProperties(edge, m_prop_id));
      } else {
        mp_output_inter->insert (edge);
      }
    } else {
      if (m_prop_id != 0) {
        mp_output_intra->insert (db::EdgePairWithProperties(edge, m_prop_id));
      } else {
        mp_output_intra->insert (edge);
      }
    }
  }

private:
  Output *mp_output_inter;
  Output *mp_output_intra;
  db::properties_id_type m_prop_id;
};

/**
 *  @brief A helper class for the DRC functionality
 *
 *  This class implements the edge-to-edge part of the polygon DRC.
 *  This version allows delivery of the negative edges.
 */
template <class Output, class NegativeEdgeOutput>
class DB_PUBLIC_TEMPLATE edge2edge_check_with_negative_output
  : public edge2edge_check<Output>
{
public:
  edge2edge_check_with_negative_output (const EdgeRelationFilter &check, Output &output, NegativeEdgeOutput &l1_negative_output, NegativeEdgeOutput &l2_negative_output, bool different_polygons, bool requires_different_layers, bool with_shielding, bool symmetric_edges, db::properties_id_type prop_id = 0)
    : edge2edge_check<Output> (check, output, different_polygons, requires_different_layers, with_shielding, symmetric_edges, prop_id),
      mp_l1_negative_output (&l1_negative_output),
      mp_l2_negative_output (&l2_negative_output)
  {
    edge2edge_check<Output>::set_has_negative_edge_output (true);
  }

  edge2edge_check_with_negative_output (const EdgeRelationFilter &check, Output &output_inter, Output &output_intra, NegativeEdgeOutput &l1_negative_output, NegativeEdgeOutput &l2_negative_output, bool different_polygons, bool requires_different_layers, bool with_shielding, bool symmetric_edges, db::properties_id_type prop_id = 0)
    : edge2edge_check<Output> (check, output_inter, output_intra, different_polygons, requires_different_layers, with_shielding, symmetric_edges, prop_id),
      mp_l1_negative_output (&l1_negative_output),
      mp_l2_negative_output (&l2_negative_output)
  {
    edge2edge_check<Output>::set_has_negative_edge_output (true);
  }

protected:
  void put_negative (const db::Edge &edge, int layer) const
  {
    if (layer == 0) {
      mp_l1_negative_output->insert (edge);
    }
    if (layer == 1) {
      mp_l2_negative_output->insert (edge);
    }
  }

private:
  NegativeEdgeOutput *mp_l1_negative_output, *mp_l2_negative_output;
};

/**
 *  @brief A helper class for the DRC functionality
 *
 *  This class implements the edge-to-edge part of the polygon DRC.
 *  This version has only negative edge output.
 */
template <class NegativeEdgeOutput>
class DB_PUBLIC_TEMPLATE edge2edge_check_negative
  : public Edge2EdgeCheckBase
{
public:
  edge2edge_check_negative (const EdgeRelationFilter &check, NegativeEdgeOutput &l1_negative_output, NegativeEdgeOutput &l2_negative_output, bool different_polygons, bool requires_different_layers, bool with_shielding)
    : Edge2EdgeCheckBase (check, different_polygons, requires_different_layers, with_shielding, false),
      mp_l1_negative_output (&l1_negative_output),
      mp_l2_negative_output (&l2_negative_output)
  {
    set_has_negative_edge_output (true);
    set_has_edge_pair_output (false);
  }

protected:
  void put_negative (const db::Edge &edge, int layer) const
  {
    if (layer == 0) {
      mp_l1_negative_output->insert (edge);
    }
    if (layer == 1) {
      mp_l2_negative_output->insert (edge);
    }
  }

private:
  NegativeEdgeOutput *mp_l1_negative_output, *mp_l2_negative_output;
};

/**
 *  @brief A helper class for the DRC functionality
 *
 *  This class implements the edge-to-edge part of the polygon DRC.
 *  This version has positive or negative output. Negative output is mapped to edge pairs
 *  as well.
 */
template <class Output>
class DB_PUBLIC_TEMPLATE edge2edge_check_negative_or_positive
  : public edge2edge_check<Output>
{
public:
  edge2edge_check_negative_or_positive (const EdgeRelationFilter &check, Output &output, bool negative_output, bool different_polygons, bool requires_different_layers, bool with_shielding, bool symmetric, db::properties_id_type prop_id = 0)
    : edge2edge_check<Output> (check, output, different_polygons, requires_different_layers, with_shielding, symmetric, prop_id)
  {
    edge2edge_check<Output>::set_has_negative_edge_output (negative_output);
    edge2edge_check<Output>::set_has_edge_pair_output (! negative_output);
  }

  edge2edge_check_negative_or_positive (const EdgeRelationFilter &check, Output &output_inter, Output &output_intra, bool negative_output, bool different_polygons, bool requires_different_layers, bool with_shielding, bool symmetric, db::properties_id_type prop_id = 0)
    : edge2edge_check<Output> (check, output_inter, output_intra, different_polygons, requires_different_layers, with_shielding, symmetric, prop_id)
  {
    edge2edge_check<Output>::set_has_negative_edge_output (negative_output);
    edge2edge_check<Output>::set_has_edge_pair_output (! negative_output);
  }

protected:
  void put_negative (const db::Edge &edge, int layer) const
  {
    if (layer == 0) {
      edge2edge_check<Output>::put (db::EdgePair (edge, edge.swapped_points ()), false);
    }
#if 0
    //  NOTE: second-input negative edge output isn't worth a lot as the second input often is not merged, hence
    //  the outer edges to not represent the actual contour.
    if (layer == 1) {
      edge2edge_check<Output>::put (db::EdgePair (edge.swapped_points (), edge), false);
    }
#endif
  }
};

/**
 *  @brief A helper class for the DRC functionality which acts as an edge pair receiver
 */
template <class PolygonType>
class DB_PUBLIC poly2poly_check
{
public:
  typedef typename PolygonType::box_type box_type;
  typedef typename PolygonType::edge_type edge_type;

  poly2poly_check (Edge2EdgeCheckBase &output);
  poly2poly_check ();

  void clear ();

  void single (const PolygonType&o, size_t p);

  void connect (Edge2EdgeCheckBase &output);
  void enter (const PolygonType &o, size_t p);
  void enter (const PolygonType &o, size_t p, const box_type &search_box);
  void enter (const edge_type &o, size_t p);
  void enter (const edge_type &o, size_t p, const box_type &search_box);
  void process ();

private:
  db::Edge2EdgeCheckBase *mp_output;
  db::box_scanner<db::Edge, size_t> m_scanner;
  std::list<db::Edge> m_edge_heap;
};

/**
 *  @brief A helper class for the region to edge interaction functionality
 */
template <class PolygonType, class EdgeType, class OutputType>
class DB_PUBLIC region_to_edge_interaction_filter_base
  : public db::box_scanner_receiver2<PolygonType, size_t, db::Edge, size_t>
{
public:
  region_to_edge_interaction_filter_base (bool inverse, bool get_all);

  void preset (const OutputType *s);
  void add (const PolygonType *p, size_t, const EdgeType *e, size_t);
  void fill_output ();

protected:
  virtual void put (const OutputType &s) const = 0;

private:
  std::set<const OutputType *> m_seen;
  bool m_inverse, m_get_all;
};

/**
 *  @brief A helper class for the region to edge interaction functionality
 */
template <class PolygonType, class EdgeType, class OutputContainer, class OutputType = typename OutputContainer::value_type>
class DB_PUBLIC_TEMPLATE region_to_edge_interaction_filter
  : public region_to_edge_interaction_filter_base<PolygonType, EdgeType, OutputType>
{
public:
  region_to_edge_interaction_filter (OutputContainer &output, bool inverse, bool get_all = false)
    : region_to_edge_interaction_filter_base<PolygonType, EdgeType, OutputType> (inverse, get_all), mp_output (&output)
  {
    //  .. nothing yet ..
  }

protected:
  virtual void put (const OutputType &res) const
  {
    mp_output->insert (res);
  }

private:
  OutputContainer *mp_output;
};

/**
 *  @brief A helper class for the region to text interaction functionality
 */
template <class PolygonType, class TextType, class OutputType>
class DB_PUBLIC region_to_text_interaction_filter_base
  : public db::box_scanner_receiver2<PolygonType, size_t, TextType, size_t>
{
public:
  region_to_text_interaction_filter_base (bool inverse, bool get_all);

  void preset (const OutputType *s);
  void add (const PolygonType *p, size_t, const TextType *e, size_t);
  void fill_output ();

protected:
  virtual void put (const OutputType &s) const = 0;

private:
  std::set<const OutputType *> m_seen;
  bool m_inverse, m_get_all;
};

/**
 *  @brief A helper class for the region to text interaction functionality
 */
template <class PolygonType, class TextType, class OutputContainer, class OutputType = typename OutputContainer::value_type>
class DB_PUBLIC_TEMPLATE region_to_text_interaction_filter
  : public region_to_text_interaction_filter_base<PolygonType, TextType, OutputType>
{
public:
  region_to_text_interaction_filter (OutputContainer &output, bool inverse, bool get_all = false)
    : region_to_text_interaction_filter_base<PolygonType, TextType, OutputType> (inverse, get_all), mp_output (&output)
  {
    //  .. nothing yet ..
  }

protected:
  virtual void put (const OutputType &poly) const
  {
    mp_output->insert (poly);
  }

private:
  OutputContainer *mp_output;
};

} // namespace db

#endif

