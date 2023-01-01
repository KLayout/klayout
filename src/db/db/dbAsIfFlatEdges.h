
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


#ifndef HDR_dbAsIfFlatEdges
#define HDR_dbAsIfFlatEdges

#include "dbCommon.h"
#include "dbBoxScanner.h"
#include "dbEdgesDelegate.h"
#include "dbEdgeBoolean.h"
#include "dbEdgeProcessor.h"
#include "dbEdgesUtils.h"
#include "dbBoxScanner.h"
#include "dbPolygonTools.h"

#include <map>
#include <vector>

namespace db {

class PolygonSink;

/**
 *  @brief Provides default flat implementations
 */
class DB_PUBLIC AsIfFlatEdges
  : public EdgesDelegate
{
public:
  AsIfFlatEdges ();
  virtual ~AsIfFlatEdges ();

  virtual size_t count () const;
  virtual size_t hier_count () const;
  virtual std::string to_string (size_t) const;
  virtual distance_type length (const db::Box &) const;
  virtual Box bbox () const;

  virtual EdgePairsDelegate *width_check (db::Coord d, const db::EdgesCheckOptions &options) const
  {
    return run_check (db::WidthRelation, 0, d, options);
  }
    
  virtual EdgePairsDelegate *space_check (db::Coord d, const db::EdgesCheckOptions &options) const
  {
    return run_check (db::SpaceRelation, 0, d, options);
  }

  virtual EdgePairsDelegate *enclosing_check (const Edges &other, db::Coord d, const db::EdgesCheckOptions &options) const
  {
    return run_check (db::OverlapRelation, &other, d, options);
  }

  virtual EdgePairsDelegate *overlap_check (const Edges &other, db::Coord d, const db::EdgesCheckOptions &options) const
  {
    return run_check (db::WidthRelation, &other, d, options);
  }

  virtual EdgePairsDelegate *separation_check (const Edges &other, db::Coord d, const db::EdgesCheckOptions &options) const
  {
    return run_check (db::SpaceRelation, &other, d, options);
  }

  virtual EdgePairsDelegate *inside_check (const Edges &other, db::Coord d, const db::EdgesCheckOptions &options) const
  {
    return run_check (db::InsideRelation, &other, d, options);
  }

  virtual EdgesDelegate *process_in_place (const EdgeProcessorBase &filter)
  {
    return processed (filter);
  }

  virtual EdgesDelegate *processed (const EdgeProcessorBase &filter) const;
  virtual EdgePairsDelegate *processed_to_edge_pairs (const EdgeToEdgePairProcessorBase &) const;
  virtual RegionDelegate *processed_to_polygons (const EdgeToPolygonProcessorBase &) const;

  virtual EdgesDelegate *filter_in_place (const EdgeFilterBase &filter)
  {
    return filtered (filter);
  }

  virtual EdgesDelegate *filtered (const EdgeFilterBase &) const;

  virtual EdgesDelegate *merged_in_place ()
  {
    return merged ();
  }

  virtual EdgesDelegate *merged () const
  {
    return boolean (0, EdgeOr);
  }

  virtual EdgesDelegate *and_with (const Edges &other) const
  {
    return boolean (&other, EdgeAnd);
  }

  virtual EdgesDelegate *not_with (const Edges &other) const
  {
    return boolean (&other, EdgeNot);
  }

  virtual std::pair<EdgesDelegate *, EdgesDelegate *> andnot_with (const Edges &other) const
  {
    return boolean_andnot (&other);
  }

  virtual EdgesDelegate *and_with (const Region &other) const
  {
    return edge_region_op (other, db::EdgePolygonOp::Inside, true /*include borders*/).first;
  }

  virtual EdgesDelegate *not_with (const Region &other) const
  {
    return edge_region_op (other, db::EdgePolygonOp::Outside, true /*include borders*/).first;
  }

  virtual std::pair<EdgesDelegate *, EdgesDelegate *> andnot_with (const Region &other) const
  {
    return edge_region_op (other, db::EdgePolygonOp::Both, true /*include borders*/);
  }

  virtual EdgesDelegate *xor_with (const Edges &other) const
  {
    return boolean (&other, EdgeXor);
  }

  virtual EdgesDelegate *or_with (const Edges &other) const
  {
    return boolean (&other, EdgeOr);
  }

  virtual EdgesDelegate *intersections (const Edges &other) const
  {
    return boolean (&other, EdgeIntersections);
  }

  virtual EdgesDelegate *add_in_place (const Edges &other)
  {
    return add (other);
  }

  virtual EdgesDelegate *add (const Edges &other) const;

  virtual EdgesDelegate *inside_part (const Region &other) const
  {
    return edge_region_op (other, db::EdgePolygonOp::Inside, false /*don't include borders*/).first;
  }

  virtual EdgesDelegate *outside_part (const Region &other) const
  {
    return edge_region_op (other, db::EdgePolygonOp::Outside, false /*don't include borders*/).first;
  }

  virtual std::pair<EdgesDelegate *, EdgesDelegate *> inside_outside_part_pair (const Region &other) const
  {
    return edge_region_op (other, db::EdgePolygonOp::Both, false /*don't include borders*/);
  }

  virtual RegionDelegate *extended (coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i, bool join) const;

  virtual EdgesDelegate *pull_interacting (const Edges &) const;
  virtual RegionDelegate *pull_interacting (const Region &) const;
  virtual EdgesDelegate *selected_interacting (const Edges &) const;
  virtual EdgesDelegate *selected_not_interacting (const Edges &) const;
  virtual EdgesDelegate *selected_interacting (const Region &) const;
  virtual EdgesDelegate *selected_not_interacting (const Region &) const;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_interacting_pair (const Region &other) const;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_interacting_pair (const Edges &other) const;

  virtual EdgesDelegate *selected_outside (const Edges &other) const;
  virtual EdgesDelegate *selected_not_outside (const Edges &other) const;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_outside_pair (const Edges &other) const;
  virtual EdgesDelegate *selected_inside (const Edges &other) const;
  virtual EdgesDelegate *selected_not_inside (const Edges &other) const;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_inside_pair (const Edges &other) const;
  virtual EdgesDelegate *selected_outside (const Region &other) const;
  virtual EdgesDelegate *selected_not_outside (const Region &other) const;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_outside_pair (const Region &other) const;
  virtual EdgesDelegate *selected_inside (const Region &other) const;
  virtual EdgesDelegate *selected_not_inside (const Region &other) const;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_inside_pair (const Region &other) const;

  virtual EdgesDelegate *in (const Edges &, bool) const;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> in_and_out (const Edges &) const;

  virtual bool equals (const Edges &other) const;
  virtual bool less (const Edges &other) const;

  virtual void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const;

protected:
  void update_bbox (const db::Box &box);
  void invalidate_bbox ();
  EdgePairsDelegate *run_check (db::edge_relation_type rel, const Edges *other, db::Coord d, const EdgesCheckOptions &options) const;
  virtual EdgesDelegate *pull_generic (const Edges &edges) const;
  virtual RegionDelegate *pull_generic (const Region &region) const;
  virtual EdgesDelegate *selected_interacting_generic (const Edges &edges, EdgeInteractionMode mode, bool inverse) const;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_interacting_pair_generic (const Edges &edges, EdgeInteractionMode mode) const;
  virtual EdgesDelegate *selected_interacting_generic (const Region &region, EdgeInteractionMode mode, bool inverse) const;
  virtual std::pair<EdgesDelegate *, EdgesDelegate *> selected_interacting_pair_generic (const Region &region, EdgeInteractionMode mode) const;
  AsIfFlatEdges &operator= (const AsIfFlatEdges &other);
  AsIfFlatEdges (const AsIfFlatEdges &other);

private:
  mutable bool m_bbox_valid;
  mutable db::Box m_bbox;

  virtual db::Box compute_bbox () const;
  EdgesDelegate *boolean (const Edges *other, EdgeBoolOp op) const;
  std::pair<EdgesDelegate *, EdgesDelegate *> boolean_andnot (const Edges *other) const;
  std::pair<EdgesDelegate *, EdgesDelegate *> edge_region_op(const Region &other, db::EdgePolygonOp::mode_t mode, bool include_borders) const;
};

}

#endif

