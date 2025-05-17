
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include "pexCommon.h"

#include "pexRNetExtractor.h"
#include "pexRNetwork.h"
#include "pexRExtractorTech.h"
#include "pexSquareCountingRExtractor.h"
#include "pexTriangulationRExtractor.h"

#include "dbBoxScanner.h"
#include "dbPolygonTools.h"
#include "dbRegionProcessors.h"
#include "dbCompoundOperation.h"
#include "dbPolygonNeighborhood.h"
#include "dbPropertiesRepository.h"

namespace pex
{

RNetExtractor::RNetExtractor (double dbu)
  : m_dbu (dbu)
{
  //  .. nothing yet ..
}

void
RNetExtractor::extract (const RExtractorTech &tech,
                        const std::map<unsigned int, db::Region> &geo,
                        const std::map<unsigned int, std::vector<db::Point> > &vertex_ports,
                        const std::map<unsigned int, std::vector<db::Polygon> > &polygon_ports,
                        RNetwork &rnetwork)
{
  rnetwork.clear ();

  std::map<unsigned int, std::vector<ViaPort> > via_ports;
  create_via_ports (tech, geo, via_ports, rnetwork);

  for (auto g = geo.begin (); g != geo.end (); ++g) {

    //  Find the conductor spec for the given layer
    const RExtractorTechConductor *cond = 0;
    for (auto c = tech.conductors.begin (); c != tech.conductors.end () && !cond; ++c) {
      if (c->layer == g->first) {
        cond = c.operator-> ();
      }
    }
    if (! cond) {
      continue;
    }

    //  fetch the port list for vertex ports
    auto ivp = vertex_ports.find (g->first);
    static std::vector<db::Point> empty_vertex_ports;
    const std::vector<db::Point> &vp = ivp == vertex_ports.end () ? empty_vertex_ports : ivp->second;

    //  fetch the port list for polygon ports
    auto ipp = polygon_ports.find (g->first);
    static std::vector<db::Polygon> empty_polygon_ports;
    const std::vector<db::Polygon> &pp = ipp == polygon_ports.end () ? empty_polygon_ports : ipp->second;

    //  fetch the port list for via ports
    auto iviap = via_ports.find (g->first);
    static std::vector<ViaPort> empty_via_ports;
    const std::vector<ViaPort> &viap = iviap == via_ports.end () ? empty_via_ports : iviap->second;

    //  extract the conductor polygon and integrate the results into the target network
    extract_conductor (*cond, g->second, vp, pp, viap, rnetwork);

  }

  if (! tech.skip_simplify) {
    rnetwork.simplify ();
  }
}

static double
via_conductance (const RExtractorTechVia &via_tech,
                const db::Polygon &poly,
                double dbu)
{
  if (via_tech.resistance < 1e-10) {
    return RElement::short_value ();
  } else {
    return (1.0 / via_tech.resistance) * dbu * dbu * poly.area ();
  }
}

namespace
{

class ViaAggregationVisitor
  : public db::PolygonNeighborhoodVisitor
{
public:
  ViaAggregationVisitor (const RExtractorTechVia *via_tech, double dbu)
    : mp_via_tech (via_tech), m_dbu (dbu)
  {
    //  this is just for consistency - we actually do not produce output
    set_result_type (db::CompoundRegionCheckOperationNode::Region);
  }

  virtual void neighbors (const db::Layout * /*layout*/, const db::Cell * /*cell*/, const db::PolygonWithProperties &polygon, const neighbors_type &neighbors)
  {
    auto i = neighbors.find ((unsigned int) 1);
    if (i == neighbors.end ()) {
      return;
    }

    double c = 0;
    for (auto vp = i->second.begin (); vp != i->second.end (); ++vp) {
      double cc = via_conductance (*mp_via_tech, *vp, m_dbu);
      if (cc == RElement::short_value ()) {
        c = cc;
        break;
      } else {
        c += cc;
      }
    }

    db::PropertiesSet ps;
    ps.insert (prop_name_id, tl::Variant (c));

    output_polygon (db::PolygonWithProperties (polygon, db::properties_id (ps)));
  }

  static db::property_names_id_type prop_name_id;

private:
  const RExtractorTechVia *mp_via_tech;
  std::vector<std::pair<double, db::Point> > *mp_conductances;
  db::property_names_id_type m_prop_name_id;
  double m_dbu;
};

db::property_names_id_type ViaAggregationVisitor::prop_name_id = db::property_names_id (tl::Variant ());

}

void
RNetExtractor::create_via_port (const pex::RExtractorTechVia &tech,
                                double conductance,
                                const db::Polygon &poly,
                                unsigned int &port_index,
                                std::map<unsigned int, std::vector<ViaPort> > &vias,
                                RNetwork &rnetwork)
{
  RNode *a = rnetwork.create_node (RNode::Internal, port_index++, tech.bottom_conductor);
  RNode *b = rnetwork.create_node (RNode::Internal, port_index++, tech.top_conductor);

  db::CplxTrans to_um (m_dbu);
  db::Box box = poly.box ();
  b->location = a->location = to_um * box;

  rnetwork.create_element (conductance, a, b);

  vias[tech.bottom_conductor].push_back (ViaPort (box.center (), a));
  vias[tech.top_conductor].push_back (ViaPort (box.center (), b));
}

void
RNetExtractor::create_via_ports (const RExtractorTech &tech,
                                 const std::map<unsigned int, db::Region> &geo,
                                 std::map<unsigned int, std::vector<ViaPort> > &vias,
                                 RNetwork &rnetwork)
{
  unsigned int port_index = 0;

  for (auto v = tech.vias.begin (); v != tech.vias.end (); ++v) {

    auto g = geo.find (v->cut_layer);
    if (g == geo.end ()) {
      continue;
    }

    if (v->merge_distance > db::epsilon) {

      //  with merge, follow this scheme:
      //  1.) do a merge by over/undersize
      //  2.) do a convex decomposition, so we get convex via shapes with the bbox center inside the polygon
      //  3.) re-aggregate the original via polygons and collect the total conductance per merged shape

      db::Coord sz = db::coord_traits<db::Coord>::rounded (0.5 * v->merge_distance / m_dbu);

      db::Region merged_vias = g->second.sized (sz).sized (-sz);
      merged_vias.process (db::ConvexDecomposition (db::PO_any));

      std::vector<db::CompoundRegionOperationNode *> children;
      children.push_back (new db::CompoundRegionOperationPrimaryNode ());
      children.push_back (new db::CompoundRegionOperationSecondaryNode (const_cast<db::Region *> (&g->second)));

      ViaAggregationVisitor visitor (v.operator-> (), m_dbu);
      db::PolygonNeighborhoodCompoundOperationNode en_node (children, &visitor, 0);
      auto aggregated = merged_vias.cop_to_region (en_node);

      for (auto p = aggregated.begin (); ! p.at_end (); ++p) {
        double c = db::properties (p.prop_id ()).value (ViaAggregationVisitor::prop_name_id).to_double ();
        create_via_port (*v, c, *p, port_index, vias, rnetwork);
      }

    } else {

      for (auto p = g->second.begin_merged (); ! p.at_end (); ++p) {
        create_via_port (*v, via_conductance (*v, *p, m_dbu), *p, port_index, vias, rnetwork);
      }

    }

  }
}

static inline size_t make_id (unsigned int index, unsigned int type)
{
  return (size_t (index) << 2) + type;
}

static inline unsigned int index_from_id (size_t id)
{
  return (unsigned int) (id >> 2);
}

static inline unsigned int type_from_id (size_t id)
{
  return (unsigned int) (id & 3);
}

namespace
{

class ExtractingReceiver
  : public db::box_scanner_receiver2<db::Polygon, size_t, db::Box, size_t>
{
public:
  ExtractingReceiver (const RExtractorTechConductor *cond,
                      const std::vector<db::Point> *vertex_ports,
                      const std::vector<db::Polygon> *polygon_ports,
                      const std::vector<RNetExtractor::ViaPort> *via_ports,
                      double dbu,
                      RNetwork *rnetwork)
    : mp_cond (cond),
      mp_vertex_ports (vertex_ports),
      mp_polygon_ports (polygon_ports),
      mp_via_ports (via_ports),
      m_next_internal_port_index (0),
      m_dbu (dbu),
      mp_rnetwork (rnetwork)
  {
    for (auto n = rnetwork->begin_nodes (); n != rnetwork->end_nodes (); ++n) {
      if (n->type == RNode::Internal && n->port_index > m_next_internal_port_index) {
        m_next_internal_port_index = n->port_index;
      }
    }
  }

  void finish1 (const db::Polygon *poly, const size_t poly_id)
  {
    auto i = m_interacting_ports.find (poly_id);
    if (i == m_interacting_ports.end ()) {
      static std::set<size_t> empty_ids;
      extract (*poly, empty_ids);
    } else {
      extract (*poly, i->second);
      m_interacting_ports.erase (i);
    }
  }

  void add (const db::Polygon *poly, const size_t poly_id, const db::Box *port, const size_t port_id)
  {
    if (db::interact (*poly, *port)) {
      m_interacting_ports[poly_id].insert (port_id);
    }
  }

private:
  std::map<size_t, std::set<size_t> > m_interacting_ports;
  const RExtractorTechConductor *mp_cond;
  const std::vector<db::Point> *mp_vertex_ports;
  const std::vector<db::Polygon> *mp_polygon_ports;
  const std::vector<RNetExtractor::ViaPort> *mp_via_ports;
  std::map<size_t, RNode *> m_id_to_node;
  unsigned int m_next_internal_port_index;
  double m_dbu;
  RNetwork *mp_rnetwork;

  void extract (const db::Polygon &poly, const std::set<size_t> &port_ids)
  {
    std::vector<db::Point> local_vertex_ports;
    std::vector<size_t> local_vertex_port_ids;
    std::vector<db::Polygon> local_polygon_ports;
    std::vector<size_t> local_polygon_port_ids;

    for (auto i = port_ids.begin (); i != port_ids.end (); ++i) {
      switch (type_from_id (*i)) {
      case 0:  //  vertex port
        local_vertex_port_ids.push_back (*i);
        local_vertex_ports.push_back ((*mp_vertex_ports) [index_from_id (*i)]);
        break;
      case 1:  //  via port
        local_vertex_port_ids.push_back (*i);
        local_vertex_ports.push_back ((*mp_via_ports) [index_from_id (*i)].position);
        break;
      case 2:  //  polygon port
        local_polygon_port_ids.push_back (*i);
        local_polygon_ports.push_back ((*mp_polygon_ports) [index_from_id (*i)]);
        break;
      }
    }

    pex::RNetwork local_network;

    switch (mp_cond->algorithm) {
    case RExtractorTechConductor::SquareCounting:
    default:
      {
        pex::SquareCountingRExtractor rex (m_dbu);
        rex.extract (poly, local_vertex_ports, local_polygon_ports, local_network);
      }
      break;
    case RExtractorTechConductor::Tesselation:
      {
        pex::TriangulationRExtractor rex (m_dbu);
        rex.extract (poly, local_vertex_ports, local_polygon_ports, local_network);
      }
      break;
    }

    integrate (local_network, local_vertex_port_ids, local_polygon_port_ids);
  }

  void integrate (const RNetwork &local_network,
                  const std::vector<size_t> &local_vertex_port_ids,
                  const std::vector<size_t> &local_polygon_port_ids)
  {
    //  create or find the new nodes in the target network
    std::unordered_map<const RNode *, RNode *> n2n;
    for (auto n = local_network.begin_nodes (); n != local_network.end_nodes (); ++n) {

      const RNode *local = n.operator-> ();
      RNode *global = 0;

      if (local->type == RNode::Internal) {

        //  for internal nodes always create a node in the target network
        global = mp_rnetwork->create_node (local->type, ++m_next_internal_port_index, mp_cond->layer);
        global->location = local->location;

      } else if (local->type == RNode::VertexPort) {

        //  for vertex nodes reuse the via node or create a new target node, unless one
        //  was created already.

        size_t id = local_vertex_port_ids [local->port_index];

        auto i2n = m_id_to_node.find (id);
        if (i2n != m_id_to_node.end ()) {
          global = i2n->second;
        } else {
          if (type_from_id (id) == 0) {  // vertex port
            global = mp_rnetwork->create_node (RNode::VertexPort, index_from_id (id), mp_cond->layer);
            global->location = local->location;
          } else if (type_from_id (id) == 1) {  // via port
            global = (*mp_via_ports) [index_from_id (id)].node;
          }
          m_id_to_node.insert (std::make_pair (id, global));
        }

      } else if (local->type == RNode::PolygonPort) {

        //  for polygon nodes create a new target node, unless one was created already.

        size_t id = local_polygon_port_ids [local->port_index];
        tl_assert (type_from_id (id) == 2);

        auto i2n = m_id_to_node.find (id);
        if (i2n != m_id_to_node.end ()) {
          global = i2n->second;
        } else {
          global = mp_rnetwork->create_node (RNode::PolygonPort, index_from_id (id), mp_cond->layer);
          global->location = local->location;
          m_id_to_node.insert (std::make_pair (id, global));
        }

      }

      tl_assert (global != 0);
      n2n.insert (std::make_pair (local, global));

    }

    //  create the R elements in the target network
    for (auto e = local_network.begin_elements (); e != local_network.end_elements (); ++e) {

      const RElement *local = e.operator-> ();

      auto ia = n2n.find (local->a ());
      auto ib = n2n.find (local->b ());
      tl_assert (ia != n2n.end ());
      tl_assert (ia != n2n.end ());

      double c;
      if (mp_cond->resistance < 1e-10) {
        c = RElement::short_value ();
      } else {
        c = local->conductance / mp_cond->resistance;
      }

      mp_rnetwork->create_element (c, ia->second, ib->second);

    }
  }
};

}

void
RNetExtractor::extract_conductor (const RExtractorTechConductor &cond,
                                  const db::Region &region,
                                  const std::vector<db::Point> &vertex_ports,
                                  const std::vector<db::Polygon> &polygon_ports,
                                  const std::vector<ViaPort> &via_ports,
                                  RNetwork &rnetwork)
{
  db::box_scanner2<db::Polygon, size_t, db::Box, size_t> scanner;

  size_t poly_id = 0;
  for (auto p = region.addressable_merged_polygons (); ! p.at_end (); ++p) {
    scanner.insert1 (p.operator-> (), poly_id++);
  }

  std::list<db::Box> box_heap;

  //  type 0 objects (vertex ports)
  for (auto i = vertex_ports.begin (); i != vertex_ports.end (); ++i) {
    //  TODO: could be without enlarge?
    box_heap.push_back (db::Box (*i, *i).enlarged (db::Vector (1, 1)));
    scanner.insert2 (&box_heap.back (), make_id (i - vertex_ports.begin (), 0));
  }

  //  type 1 objects (via ports)
  for (auto i = via_ports.begin (); i != via_ports.end (); ++i) {
    //  TODO: could be without enlarge?
    box_heap.push_back (db::Box (i->position, i->position).enlarged (db::Vector (1, 1)));
    scanner.insert2 (&box_heap.back (), make_id (i - via_ports.begin (), 1));
  }

  //  type 2 objects (polygon ports)
  for (auto i = polygon_ports.begin (); i != polygon_ports.end (); ++i) {
    box_heap.push_back (i->box ());
    scanner.insert2 (&box_heap.back (), make_id (i - polygon_ports.begin (), 2));
  }

  ExtractingReceiver rec (&cond, &vertex_ports, &polygon_ports, &via_ports, m_dbu, &rnetwork);
  scanner.process (rec, 0, db::box_convert<db::Polygon> (), db::box_convert<db::Box> ());
}

}
