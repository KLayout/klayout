
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

#ifndef HDR_pexSquareCountingRExtractor
#define HDR_pexSquareCountingRExtractor

#include "pexCommon.h"
#include "pexRNetwork.h"
#include "pexRExtractor.h"

#include "dbPLCConvexDecomposition.h"

namespace pex
{

/**
 *  @brief The Square Counting R Extractor
 *
 *  The idea of that extractor is to first decompose the polygon into
 *  convex parts. Each convex part is taken as "thin" and the current
 *  flow being parallel and homogeneous to the long axis.
 *
 *  Internal ports are created between the partial polygons where
 *  they touch.
 *
 *  The ports are considered point-like (polygon ports are replaced
 *  by points in their bounding box centers) and inject current
 *  at their specific position only. The resistance is accumulated
 *  between ports by integrating the squares (length along
 *  the long axis / width).
 */
class PEX_PUBLIC SquareCountingRExtractor
  : public RExtractor
{
public:
  /**
   *  @brief The constructor
   */
  SquareCountingRExtractor (double dbu);

  /**
   *  @brief Gets the decomposition parameters
   */
  db::plc::ConvexDecompositionParameters &decomposition_parameters ()
  {
    return m_decomp_param;
  }

  /**
   *  @brief Sets a value indicating whether to skip the simplify step
   */
  void set_skip_simplfy (bool f)
  {
    m_skip_simplify = f;
  }

  /**
   *  @brief Gets a value indicating whether to skip the simplify step
   */
  bool skip_simplify () const
  {
    return m_skip_simplify;
  }

  /**
   *  @brief Sets the database unit
   */
  void set_dbu (double dbu)
  {
    m_dbu = dbu;
  }

  /**
   *  @brief Gets the database unit
   */
  double dbu () const
  {
    return m_dbu;
  }

  /**
   *  @brief Implementation of the extraction function
   */
  virtual void extract (const db::Polygon &polygon, const std::vector<db::Point> &vertex_ports, const std::vector<db::Polygon> &polygon_ports, RNetwork &rnetwork);

protected:
  /**
   *  @brief A helper structure defining a port
   */
  struct PortDefinition
  {
    PortDefinition ()
      : type (pex::RNode::Internal), port_index (0)
    { }

    PortDefinition (pex::RNode::node_type _type, const db::Point &_location, unsigned int _port_index)
      : type (_type), location (_location, _location), port_index (_port_index)
    { }

    PortDefinition (pex::RNode::node_type _type, const db::Box &_location, unsigned int _port_index)
      : type (_type), location (_location), port_index (_port_index)
    { }

    bool operator< (const PortDefinition &other) const
    {
      if (type != other.type) {
        return type < other.type;
      }
      if (port_index != other.port_index) {
        return port_index < other.port_index;
      }
      return false;
    }

    bool operator== (const PortDefinition &other) const
    {
      return type == other.type && port_index == other.port_index;
    }

    pex::RNode::node_type type;
    db::Box location;
    unsigned int port_index;
  };

  void do_extract (const db::Polygon &db_poly, const std::vector<std::pair<PortDefinition, pex::RNode *> > &ports, pex::RNetwork &rnetwork);

private:
  db::plc::ConvexDecompositionParameters m_decomp_param;
  double m_dbu;
  bool m_skip_simplify;
};

}

#endif

