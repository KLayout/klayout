
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

#ifndef HDR_pexTriangulationRExtractor
#define HDR_pexTriangulationRExtractor

#include "pexCommon.h"
#include "pexRNetwork.h"
#include "pexRExtractor.h"

#include "dbPLCTriangulation.h"

namespace pex
{

/**
 *  @brief An R extractor based on a triangulation of the resistor area
 *
 *  This resistor extractor starts with a triangulation of the
 *  polygon area and substitutes each triangle by a 3-resistor network.
 *
 *  After this, it will eliminate nodes where possible.
 *
 *  This extractor delivers a resistor matrix (there is a resistor
 *  between every specified port).
 *
 *  Polygon ports are considered to be perfectly conductive and cover
 *  their given area, shorting all nodes at their boundary.
 *
 *  This extractor delivers higher quality results than the square
 *  counting extractor, but is slower in general.
 */
class PEX_PUBLIC TriangulationRExtractor
  : public RExtractor
{
public:
  /**
   *  @brief The constructor
   */
  TriangulationRExtractor (double dbu);

  /**
   *  @brief Gets the triangulation parameters
   */
  db::plc::TriangulationParameters &triangulation_parameters ()
  {
    return m_tri_param;
  }

  /**
   *  @brief Sets a value indicating whether to skip the reduction step
   */
  void set_skip_reduction (bool f)
  {
    m_skip_reduction = f;
  }

  /**
   *  @brief Gets a value indicating whether to skip the reduction step
   */
  bool skip_reduction () const
  {
    return m_skip_reduction;
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

private:
  db::plc::TriangulationParameters m_tri_param;
  double m_dbu;
  bool m_skip_reduction;

  void create_conductances (const db::plc::Polygon &tri, const std::unordered_map<const db::plc::Vertex *, RNode *> &vertex2node, RNetwork &rnetwork);
  void eliminate_node (pex::RNode *node, RNetwork &rnetwork);
  void eliminate_all (RNetwork &rnetwork);
};

}

#endif

