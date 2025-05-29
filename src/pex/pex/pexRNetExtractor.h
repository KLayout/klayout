
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

#ifndef HDR_pexRNetExtractor
#define HDR_pexRNetExtractor

#include "pexCommon.h"
#include "dbRegion.h"

namespace pex
{

class RExtractorTech;
class RExtractorTechVia;
class RExtractorTechConductor;
class RNetwork;
class RNode;

/**
 *  @brief Implementation of the R extractor for a multi-polygon/multi-layer net
 */
class PEX_PUBLIC RNetExtractor
{
public:
  /**
   *  @brief Constructor
   *  @param dbu The database unit to be used to convert coordinates into micrometers
   */
  RNetExtractor (double dbu);

  /**
   *  @brief Extracts a R network from a given set of geometries and ports
   *  @param geo The geometries per layer
   *  @param vertex_ports The vertex ports - a list of layer and points to attache a port to on this layer
   *  @param polygon_ports The polygon ports - a list of layer and polygons to attach a port to on this layer
   *  @param rnetwork The network extracted (output)
   *
   *  The network nodes will carry the information about the port, in case they
   *  have been generated from a port.
   */
  void extract (const RExtractorTech &tech,
                const std::map<unsigned int, db::Region> &geo,
                const std::map<unsigned int, std::vector<db::Point> > &vertex_ports,
                const std::map<unsigned int, std::vector<db::Polygon> > &polygon_ports,
                RNetwork &rnetwork);

  /**
   *  @brief A structure describing a via port
   *  This structure is used internally
   */
  struct ViaPort
  {
    ViaPort () : node (0) { }
    ViaPort (const db::Point &p, RNode *n) : position (p), node (n) { }
    db::Point position;
    RNode *node;
  };

protected:
  void create_via_ports (const RExtractorTech &tech,
                         const std::map<unsigned int, db::Region> &geo,
                         std::map<unsigned int, std::vector<ViaPort> > &vias,
                         RNetwork &rnetwork);

  void extract_conductor (const RExtractorTechConductor &cond,
                          const db::Region &region,
                          const std::vector<db::Point> &vertex_ports,
                          const std::vector<db::Polygon> &polygon_ports,
                          const std::vector<ViaPort> &via_ports,
                          RNetwork &rnetwork);

private:
  double m_dbu;

  void create_via_port (const RExtractorTechVia &tech,
                        double conductance,
                        const db::Polygon &poly,
                        unsigned int &port_index,
                        std::map<unsigned int, std::vector<ViaPort> > &vias,
                        RNetwork &rnetwork);
};

}

#endif
