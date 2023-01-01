
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


#include "dbTypes.h"
#include "dbPolygon.h"

namespace db
{

class Cell;
class Region;

/**
 *  @brief Creates a tiling pattern for a single polygon using a fill cell which is repeated periodically
 *
 *  @param cell The cell where to instantiate the tiling cells
 *  @param fp0 The polygon to fill. Ideally, this polygon is merged and does not overlap with any other polygons.
 *  @param fill_cell_index The index of the cell to use for tiling
 *  @param fc_bbox The fill cell's footprint box. The footprint gives the area covered by one instance of the tiling cell.
 *  @param row_step (some_versions) The row advance vector of the fill cell. By default this is (fc_bbox.width(), 0)
 *  @param column_step (some_versions) The column advance vector of the fill cell. By default this is (0, fc_bbox.height())
 *  @param origin Specifies the origin of the fill raster if enhanced_fill is false
 *  @param enhanced_fill If set, the tiling offset will be optimized such that as much tiling cells fit into each polygon
 *
 *  Optional parameters:
 *
 *  @param remaining_parts If non-null, this vector receives the parts of the polygons not covered by the tiling cells (plus the fill_margin)
 *  @param fill_margin Only used if remaining_parts is not 0 (see there)
 *  @param glue_box Guarantees boundary compatibility
 *
 *  Return value: true, if the polygon could be filled, false if no fill tile at all could be applied (remaining_parts will not be fed in that case)
 *
 *  Explanation for the fill fc_box, row step and column step vectors:
 *
 *  The "fc_box" is a rectangular area which is repeated along the primary fill axes given by row_step
 *  and column_step vectors. The fill box is placed with the lower-left corner.
 *
 *  Formally, the fill box will be placed a positions
 *
 *    p(i,j) = p0 + i * row_step + j * column_step
 *
 *  p0 is a position chosen by the fill algorithm or the "origin", if enhanced_fill is false.
 *
 *  This pattern is overlaid with the polygon to fill and all instances where the fill box moved by p(i,j) is entirely inside
 *  the polygon generate a fill cell instance with a displacement of p.
 *
 *  Afterwards, the residual parts are computed by subtracting all moved fill boxes from the polygon to fill.
 *  This implies that ideally the fc_boxes should overlap while they are repeated with row_step and column_step.
 *
 *  As a practical consequence, if all fill cell geometries are within the fill boxes boundary, they will also
 *  be within the polygon to fill.
 *
 *  If the glue box is non-empty, fill cells are guaranteed to use the global origin even in enhanced mode if
 *  unless they are entirely inside and not touching the boundary of the glue box.
 *  The glue box is useful to put the fill algorithm inside a tiling processor. In this case, the glue box
 *  is the tile box while the actual fill region can be larger to allow overlapping tiles.
 *
 *  In enhanced fill mode, the origin is ignored unless a glue box is given.
 */

DB_PUBLIC bool 
fill_region (db::Cell *cell, const db::Polygon &fp, db::cell_index_type fill_cell_index, const db::Box &fc_box, const db::Point &origin, bool enhanced_fill, 
             std::vector <db::Polygon> *remaining_parts = 0, const db::Vector &fill_margin = db::Vector (), const db::Box &glue_box = db::Box ());

DB_PUBLIC bool
fill_region (db::Cell *cell, const db::Polygon &fp, db::cell_index_type fill_cell_index, const db::Box &fc_box, const db::Vector &row_step, const db::Vector &column_step, const db::Point &origin, bool enhanced_fill,
             std::vector <db::Polygon> *remaining_parts = 0, const db::Vector &fill_margin = db::Vector (), const db::Box &glue_box = db::Box ());


/**
 *  @brief A version of the fill tool that operates with region objects
 *
 *  remaining_parts (if non-null) will receive the non-filled parts of partially filled polygons. 
 *  fill_margin will specify the margin around the filled area when computing (through subtraction of the tiled area) the remaining_parts.
 *  remaining_polygons (if non-null) will receive the polygons which could not be filled at all.
 *
 *  In enhanced fill mode, the origin is ignored unless a glue box is given.
 */

DB_PUBLIC void
fill_region (db::Cell *cell, const db::Region &fr, db::cell_index_type fill_cell_index, const db::Box &fc_box, const db::Point &origin, bool enhanced_fill, 
             db::Region *remaining_parts = 0, const db::Vector &fill_margin = db::Vector (), db::Region *remaining_polygons = 0, const db::Box &glue_box = db::Box ());

DB_PUBLIC void
fill_region (db::Cell *cell, const db::Region &fp, db::cell_index_type fill_cell_index, const db::Box &fc_box, const db::Vector &row_step, const db::Vector &column_step, const db::Point &origin, bool enhanced_fill,
             db::Region *remaining_parts = 0, const db::Vector &fill_margin = db::Vector (), db::Region *remaining_polygons = 0, const db::Box &glue_box = db::Box ());

/**
 *  @brief An iterative version for enhanced fill
 *
 *  This version operates like the region-based fill_region version, but repeats the fill step until no further fill cells can be placed.
 *  The remaining parts will be placed inside "remaining_polygons" unless this pointer is null.
 *
 *  This version implies enhanced_mode (see "fill_region").
 *
 *  The origin is ignored unless a glue box is given.
 */
DB_PUBLIC void
fill_region_repeat (db::Cell *cell, const db::Region &fr, db::cell_index_type fill_cell_index,
                    const db::Box &fc_box, const db::Vector &row_step, const db::Vector &column_step,
                    const db::Vector &fill_margin, db::Region *remaining_polygons = 0, const db::Box &glue_box = db::Box ());

}
