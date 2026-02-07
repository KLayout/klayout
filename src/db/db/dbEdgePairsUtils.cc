
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#include "dbEdgePairsUtils.h"
#include "dbEdgesUtils.h"
#include "dbEdgeProcessor.h"

namespace db
{

static void insert_into_ep (const db::EdgePair &ep, db::EdgeProcessor &proc, size_t prop_id)
{
  proc.insert (db::Edge (ep.first ().p1 (), ep.first ().p2 ()), prop_id);
  proc.insert (db::Edge (ep.first ().p2 (), ep.second ().p1 ()), prop_id);
  proc.insert (db::Edge (ep.second ().p1 (), ep.second ().p2 ()), prop_id);
  proc.insert (db::Edge (ep.second ().p2 (), ep.first ().p1 ()), prop_id);
}

//  NOTE: these predicates are based on the "polygon" interpretation of edge pairs.
//  Edge pairs are considered connected and filled.
//  This is different from the interpretation of edge pairs as two edges.

bool edge_pair_interacts (const db::EdgePair &a, const db::Polygon &b)
{
  //  fall back to edge-only checks for degenerate edge pairs
  if (a.area () == 0) {
    return edge_interacts (a.first (), b) || edge_interacts (db::Edge (a.first ().p2 (), a.second ().p1 ()), b) ||
           edge_interacts (a.second (), b) || edge_interacts (db::Edge (a.second ().p2 (), a.first ().p1 ()), b);
  }

  db::EdgeProcessor ep;
  insert_into_ep (a, ep, 1);
  ep.insert (b, 0);

  db::InteractionDetector id (0, 0);
  id.set_include_touching (true);
  db::EdgeSink es;
  ep.process (es, id);
  id.finish ();

  return id.begin () != id.end ();
}

bool edge_pair_is_inside (const db::EdgePair &a, const db::Polygon &b)
{
  //  fall back to edge-only checks for degenerate edge pairs
  if (a.area () == 0) {
    return edge_is_inside (a.first (), b) && edge_is_inside (db::Edge (a.first ().p2 (), a.second ().p1 ()), b) &&
           edge_is_inside (a.second (), b) && edge_is_inside (db::Edge (a.second ().p2 (), a.first ().p1 ()), b);
  }

  db::EdgeProcessor ep;
  insert_into_ep (a, ep, 1);
  ep.insert (b, 0);

  db::InteractionDetector id (-1, 0);
  id.set_include_touching (true);
  db::EdgeSink es;
  ep.process (es, id);
  id.finish ();

  return id.begin () != id.end ();
}

bool edge_pair_is_outside (const db::EdgePair &a, const db::Polygon &b)
{
  //  fall back to edge-only checks for degenerate edge pairs
  if (a.area () == 0) {
    return edge_is_outside (a.first (), b) && edge_is_outside (db::Edge (a.first ().p2 (), a.second ().p1 ()), b) &&
           edge_is_outside (a.second (), b) && edge_is_outside (db::Edge (a.second ().p2 (), a.first ().p1 ()), b);
  }

  db::EdgeProcessor ep;
  insert_into_ep (a, ep, 1);
  ep.insert (b, 0);

  db::InteractionDetector id (1, 0);
  id.set_include_touching (false);
  db::EdgeSink es;
  ep.process (es, id);
  id.finish ();

  return id.begin () != id.end ();
}

bool edge_pair_interacts (const db::EdgePair &a, const db::Edge &b)
{
  return edge_interacts (a.first (), b) || edge_interacts (db::Edge (a.first ().p2 (), a.second ().p1 ()), b) ||
         edge_interacts (a.second (), b) || edge_interacts (db::Edge (a.second ().p2 (), a.first ().p1 ()), b);
}

}
