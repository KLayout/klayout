
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


#include "dbAsIfFlatEdgePairs.h"
#include "dbFlatEdgePairs.h"
#include "dbFlatRegion.h"
#include "dbFlatEdges.h"
#include "dbEmptyEdgePairs.h"
#include "dbEdgePairs.h"
#include "dbBoxConvert.h"

#include <sstream>

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  AsIfFlagEdgePairs implementation

AsIfFlatEdgePairs::AsIfFlatEdgePairs ()
  : EdgePairsDelegate (), m_bbox_valid (false)
{
  //  .. nothing yet ..
}

AsIfFlatEdgePairs::AsIfFlatEdgePairs (const AsIfFlatEdgePairs &other)
  : EdgePairsDelegate (other), m_bbox_valid (other.m_bbox_valid), m_bbox (other.m_bbox)
{
  operator= (other);
}

AsIfFlatEdgePairs &
AsIfFlatEdgePairs::operator= (const AsIfFlatEdgePairs &other)
{
  if (this != &other) {
    m_bbox_valid = other.m_bbox_valid;
    m_bbox = other.m_bbox;
  }
  return *this;
}

AsIfFlatEdgePairs::~AsIfFlatEdgePairs ()
{
  //  .. nothing yet ..
}

std::string
AsIfFlatEdgePairs::to_string (size_t nmax) const
{
  std::ostringstream os;
  EdgePairsIterator p (begin ());
  bool first = true;
  for ( ; ! p.at_end () && nmax != 0; ++p, --nmax) {
    if (! first) {
      os << ";";
    }
    first = false;
    os << p->to_string ();
  }
  if (! p.at_end ()) {
    os << "...";
  }
  return os.str ();
}

EdgePairsDelegate *
AsIfFlatEdgePairs::in (const EdgePairs &other, bool invert) const
{
  std::set <db::EdgePair> op;
  for (EdgePairsIterator o (other.begin ()); ! o.at_end (); ++o) {
    op.insert (*o);
  }

  std::unique_ptr<FlatEdgePairs> new_edge_pairs (new FlatEdgePairs ());

  for (EdgePairsIterator o (begin ()); ! o.at_end (); ++o) {
    if ((op.find (*o) == op.end ()) == invert) {
      new_edge_pairs->insert (*o);
    }
  }

  return new_edge_pairs.release ();
}

size_t
AsIfFlatEdgePairs::count () const
{
  size_t n = 0;
  for (EdgePairsIterator p (begin ()); ! p.at_end (); ++p) {
    ++n;
  }
  return n;
}

size_t
AsIfFlatEdgePairs::hier_count () const
{
  return count ();
}

Box AsIfFlatEdgePairs::bbox () const
{
  if (! m_bbox_valid) {
    m_bbox = compute_bbox ();
    m_bbox_valid = true;
  }
  return m_bbox;
}

Box AsIfFlatEdgePairs::compute_bbox () const
{
  db::Box b;
  for (EdgePairsIterator e (begin ()); ! e.at_end (); ++e) {
    b += e->bbox ();
  }
  return b;
}

void AsIfFlatEdgePairs::update_bbox (const db::Box &b)
{
  m_bbox = b;
  m_bbox_valid = true;
}

void AsIfFlatEdgePairs::invalidate_bbox ()
{
  m_bbox_valid = false;
}

RegionDelegate *
AsIfFlatEdgePairs::processed_to_polygons (const EdgePairToPolygonProcessorBase &filter) const
{
  std::unique_ptr<FlatRegion> region (new FlatRegion ());
  db::PropertyMapper pm (region->properties_repository (), properties_repository ());

  if (filter.result_must_not_be_merged ()) {
    region->set_merged_semantics (false);
  }

  std::vector<db::Polygon> res_polygons;

  for (EdgePairsIterator e (begin ()); ! e.at_end (); ++e) {
    res_polygons.clear ();
    filter.process (*e, res_polygons);
    for (std::vector<db::Polygon>::const_iterator pr = res_polygons.begin (); pr != res_polygons.end (); ++pr) {
      db::properties_id_type prop_id = pm (e.prop_id ());
      if (prop_id != 0) {
        region->insert (db::PolygonWithProperties (*pr, prop_id));
      } else {
        region->insert (*pr);
      }
    }
  }

  return region.release ();
}

EdgesDelegate *
AsIfFlatEdgePairs::processed_to_edges (const EdgePairToEdgeProcessorBase &filter) const
{
  std::unique_ptr<FlatEdges> edges (new FlatEdges ());
  db::PropertyMapper pm (edges->properties_repository (), properties_repository ());

  if (filter.result_must_not_be_merged ()) {
    edges->set_merged_semantics (false);
  }

  std::vector<db::Edge> res_edges;

  for (EdgePairsIterator e (begin ()); ! e.at_end (); ++e) {
    res_edges.clear ();
    filter.process (*e, res_edges);
    for (std::vector<db::Edge>::const_iterator pr = res_edges.begin (); pr != res_edges.end (); ++pr) {
      db::properties_id_type prop_id = pm (e.prop_id ());
      if (prop_id != 0) {
        edges->insert (db::EdgeWithProperties (*pr, prop_id));
      } else {
        edges->insert (*pr);
      }
    }
  }

  return edges.release ();
}

EdgePairsDelegate *
AsIfFlatEdgePairs::filtered (const EdgePairFilterBase &filter) const
{
  std::unique_ptr<FlatEdgePairs> new_edge_pairs (new FlatEdgePairs ());
  db::PropertyMapper pm (new_edge_pairs->properties_repository (), properties_repository ());

  for (EdgePairsIterator p (begin ()); ! p.at_end (); ++p) {
    if (filter.selected (*p)) {
      db::properties_id_type prop_id = pm (p.prop_id ());
      if (prop_id != 0) {
        new_edge_pairs->insert (db::EdgePairWithProperties (*p, prop_id));
      } else {
        new_edge_pairs->insert (*p);
      }
    }
  }

  return new_edge_pairs.release ();
}

RegionDelegate *
AsIfFlatEdgePairs::polygons (db::Coord e) const
{
  std::unique_ptr<FlatRegion> output (new FlatRegion ());
  db::PropertyMapper pm (output->properties_repository (), properties_repository ());

  for (EdgePairsIterator ep (begin ()); ! ep.at_end (); ++ep) {
    db::Polygon poly = ep->normalized ().to_polygon (e);
    if (poly.vertices () >= 3) {
      db::properties_id_type prop_id = pm (ep.prop_id ());
      if (prop_id != 0) {
        output->insert (db::PolygonWithProperties (poly, prop_id));
      } else {
        output->insert (poly);
      }
    }
  }

  return output.release ();
}

EdgesDelegate *
AsIfFlatEdgePairs::edges () const
{
  std::unique_ptr<FlatEdges> output (new FlatEdges ());
  db::PropertyMapper pm (output->properties_repository (), properties_repository ());

  for (EdgePairsIterator ep (begin ()); ! ep.at_end (); ++ep) {
    db::properties_id_type prop_id = pm (ep.prop_id ());
    if (prop_id != 0) {
      output->insert (db::EdgeWithProperties (ep->first (), prop_id));
      output->insert (db::EdgeWithProperties (ep->second (), prop_id));
    } else {
      output->insert (ep->first ());
      output->insert (ep->second ());
    }
  }

  return output.release ();
}

EdgesDelegate *
AsIfFlatEdgePairs::first_edges () const
{
  std::unique_ptr<FlatEdges> output (new FlatEdges ());
  db::PropertyMapper pm (output->properties_repository (), properties_repository ());

  for (EdgePairsIterator ep (begin ()); ! ep.at_end (); ++ep) {
    db::properties_id_type prop_id = pm (ep.prop_id ());
    if (prop_id != 0) {
      output->insert (db::EdgeWithProperties (ep->first (), prop_id));
    } else {
      output->insert (ep->first ());
    }
  }

  return output.release ();
}

EdgesDelegate *
AsIfFlatEdgePairs::second_edges () const
{
  std::unique_ptr<FlatEdges> output (new FlatEdges ());
  db::PropertyMapper pm (output->properties_repository (), properties_repository ());

  for (EdgePairsIterator ep (begin ()); ! ep.at_end (); ++ep) {
    db::properties_id_type prop_id = pm (ep.prop_id ());
    if (prop_id != 0) {
      output->insert (db::EdgeWithProperties (ep->second (), prop_id));
    } else {
      output->insert (ep->second ());
    }
  }

  return output.release ();
}

EdgePairsDelegate *
AsIfFlatEdgePairs::add (const EdgePairs &other) const
{
  const FlatEdgePairs *other_flat = dynamic_cast<const FlatEdgePairs *> (other.delegate ());
  if (other_flat) {

    std::unique_ptr<FlatEdgePairs> new_edge_pairs (new FlatEdgePairs (*other_flat));
    new_edge_pairs->invalidate_cache ();

    db::PropertyMapper pm (new_edge_pairs->properties_repository (), properties_repository ());

    size_t n = new_edge_pairs->raw_edge_pairs ().size () + count ();

    new_edge_pairs->reserve (n);

    for (EdgePairsIterator p (begin ()); ! p.at_end (); ++p) {
      db::properties_id_type prop_id = pm (p.prop_id ());
      if (prop_id) {
        new_edge_pairs->raw_edge_pairs ().insert (db::EdgePairWithProperties (*p, prop_id));
      } else {
        new_edge_pairs->raw_edge_pairs ().insert (*p);
      }
    }

    return new_edge_pairs.release ();

  } else {

    std::unique_ptr<FlatEdgePairs> new_edge_pairs (new FlatEdgePairs ());

    db::PropertyMapper pm (new_edge_pairs->properties_repository (), properties_repository ());
    db::PropertyMapper pm_other (new_edge_pairs->properties_repository (), &other.properties_repository ());

    size_t n = count () + other.count ();

    new_edge_pairs->reserve (n);

    for (EdgePairsIterator p (begin ()); ! p.at_end (); ++p) {
      db::properties_id_type prop_id = pm (p.prop_id ());
      if (prop_id) {
        new_edge_pairs->raw_edge_pairs ().insert (db::EdgePairWithProperties (*p, prop_id));
      } else {
        new_edge_pairs->raw_edge_pairs ().insert (*p);
      }
    }
    for (EdgePairsIterator p (other.begin ()); ! p.at_end (); ++p) {
      db::properties_id_type prop_id = pm_other (p.prop_id ());
      if (prop_id) {
        new_edge_pairs->raw_edge_pairs ().insert (db::EdgePairWithProperties (*p, prop_id));
      } else {
        new_edge_pairs->raw_edge_pairs ().insert (*p);
      }
    }

    return new_edge_pairs.release ();

  }
}

bool
AsIfFlatEdgePairs::equals (const EdgePairs &other) const
{
  if (empty () != other.empty ()) {
    return false;
  }
  if (count () != other.count ()) {
    return false;
  }
  EdgePairsIterator o1 (begin ());
  EdgePairsIterator o2 (other.begin ());
  while (! o1.at_end () && ! o2.at_end ()) {
    if (*o1 != *o2) {
      return false;
    }
    ++o1;
    ++o2;
  }
  return true;
}

bool
AsIfFlatEdgePairs::less (const EdgePairs &other) const
{
  if (empty () != other.empty ()) {
    return empty () < other.empty ();
  }
  if (count () != other.count ()) {
    return (count () < other.count ());
  }
  EdgePairsIterator o1 (begin ());
  EdgePairsIterator o2 (other.begin ());
  while (! o1.at_end () && ! o2.at_end ()) {
    if (*o1 != *o2) {
      return *o1 < *o2;
    }
    ++o1;
    ++o2;
  }
  return false;
}

void
AsIfFlatEdgePairs::insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  //  improves performance when inserting an original layout into the same layout
  db::LayoutLocker locker (layout);

  db::PropertyMapper pm (&layout->properties_repository (), properties_repository ());

  db::Shapes &shapes = layout->cell (into_cell).shapes (into_layer);
  for (EdgePairsIterator e (begin ()); ! e.at_end (); ++e) {
    db::properties_id_type prop_id = pm (e.prop_id ());
    if (prop_id) {
      shapes.insert (db::EdgePairWithProperties (*e, prop_id));
    } else {
      shapes.insert (*e);
    }
  }
}

void
AsIfFlatEdgePairs::insert_into_as_polygons (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl) const
{
  //  improves performance when inserting an original layout into the same layout
  db::LayoutLocker locker (layout);

  db::PropertyMapper pm (&layout->properties_repository (), properties_repository ());

  db::Shapes &shapes = layout->cell (into_cell).shapes (into_layer);
  for (EdgePairsIterator e (begin ()); ! e.at_end (); ++e) {
    db::properties_id_type prop_id = pm (e.prop_id ());
    if (prop_id) {
      shapes.insert (db::SimplePolygonWithProperties (e->normalized ().to_simple_polygon (enl), prop_id));
    } else {
      shapes.insert (e->normalized ().to_simple_polygon (enl));
    }
  }
}

}

