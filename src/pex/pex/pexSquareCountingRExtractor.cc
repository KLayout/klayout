
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


#include "pexSquareCountingRExtractor.h"

namespace pex
{

SquareCountingRExtractor::SquareCountingRExtractor (double dbu)
{
  m_dbu = dbu;

  m_decomp_param.split_edges = true;
  m_decomp_param.with_segments = false;
}

void
SquareCountingRExtractor::extract (const db::Polygon &polygon, const std::vector<db::Point> &vertex_ports, const std::vector<db::Polygon> &polygon_ports, RNetwork &rnetwork)
{
  db::CplxTrans trans = db::CplxTrans (m_dbu) * db::ICplxTrans (db::Trans (db::Point () - polygon.box ().center ()));
  auto inv_trans = trans.inverted ();

  db::plc::Graph plc;

  db::plc::ConvexDecomposition decomp (&plc);
  decomp.decompose (polygon, vertex_ports, m_decomp_param, trans);

  std::vector<std::pair<db::Polygon, db::plc::Polygon *> > decomp_polygons;
  for (auto p = plc.begin (); p != plc.end (); ++p) {
    // @@@decomp_polygons.push_back (db::Polygon ());
    // @@@decomp_polygons.back ().first = inv_trans * p->polygon ();
  }

  //  @@@ use box_scanner to find interactions between polygon_ports and decomp_polygons

}

}


