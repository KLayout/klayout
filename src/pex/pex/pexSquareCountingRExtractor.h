
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
#include "pexRExtractor.h"

#include "dbPLCConvexDecomposition.h"

namespace pex
{

// @@@ doc
class PEX_PUBLIC SquareCountingRExtractor
  : public RExtractor
{
public:
  SquareCountingRExtractor (double dbu);

  db::plc::ConvexDecompositionParameters &decomposition_parameters ()
  {
    return m_decomp_param;
  }

  void set_dbu (double dbu)
  {
    m_dbu = dbu;
  }

  double dbu () const
  {
    return m_dbu;
  }

  virtual void extract (const db::Polygon &polygon, const std::vector<db::Point> &vertex_ports, const std::vector<db::Polygon> &polygon_ports, RNetwork &rnetwork);

private:
  db::plc::ConvexDecompositionParameters m_decomp_param;
  double m_dbu;
};

}

#endif

