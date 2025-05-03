
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

#ifndef HDR_pexRExtractor
#define HDR_pexRExtractor

#include "pexCommon.h"

#include "dbPolygon.h"

namespace pex
{

class RNetwork;

/**
 *  @brief A base class for an resistance extractor
 *
 *  The R extractor takes a polyon, a technology definition
 *  and port definitions and extracts a resistor network.
 *  
 *  Ports are points or polygons that define the connection
 *  points to the network.
 */
class PEX_PUBLIC RExtractor
{
public:
  /**
   *  @brief Constructor
   */
  RExtractor ();

  /**
   *  @brief Destructor
   */
  virtual ~RExtractor ();

  /**
   *  @brief Extracts the resistance network from the given polygon and ports
   *
   *  The ports define specific locations where to connect to the resistance network.
   *  The network will contain corresponding nodes with the VertexPort for vertex ports
   *  and PolygonPort for polygon port. The node index is the index in the respective
   *  lists.
   */
  virtual void extract (const db::Polygon &polygon, const std::vector<db::Point> &vertex_ports, const std::vector<db::Polygon> &polygon_ports, RNetwork &rnetwork) = 0;
};

}

#endif

