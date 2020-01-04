
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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
 *  @param origin Specifies the origin of the fill raster if enhanced_fill is false
 *  @param enhanced_fill If set, the tiling offset will be optimized such that as much tiling cells fit into each polygon
 *
 *  Optional parameters:
 *
 *  @param remaining_parts If non-null, this vector receives the parts of the polygons not covered by the tiling cells (plus the fill_margin)
 *  @param fill_margin Only used if remaining_parts is not 0 (see there)
 *
 *  Return value: true, if the polygon could be filled, false if no fill tile at all could be applied (remaining_parts will not be fed in that case)
 */

DB_PUBLIC bool 
fill_region (db::Cell *cell, const db::Polygon &fp, db::cell_index_type fill_cell_index, const db::Box &fc_box, const db::Point &origin, bool enhanced_fill, 
             std::vector <db::Polygon> *remaining_parts = 0, const db::Vector &fill_margin = db::Vector ());


/**
 *  @brief A version of the fill tool that operates with region objects
 *
 *  remaining_parts (if non-null) will receive the non-filled parts of partially filled polygons. 
 *  fill_margin will specify the margin around the filled area when computing (through subtraction of the tiled area) the remaining_parts.
 *  remaining_polygons (if non-null) will receive the polygons which could not be filled at all.
 */

DB_PUBLIC void
fill_region (db::Cell *cell, const db::Region &fr, db::cell_index_type fill_cell_index, const db::Box &fc_box, const db::Point &origin, bool enhanced_fill, 
             db::Region *remaining_parts = 0, const db::Vector &fill_margin = db::Vector (), db::Region *remaining_polygons = 0);

}

