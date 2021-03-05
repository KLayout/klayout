
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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
 *
 *  Return value: true, if the polygon could be filled, false if no fill tile at all could be applied (remaining_parts will not be fed in that case)
 *
 *  Explanation for the fill kernel_origin, row step and column step vectors:
 *
 *  The "kernel" is a rectangular or diamond-shaped area which is repeated along it's primary
 *  axes. In case of a box, the kernel is a rectangle and the primary axes are the x and y axes.
 *  The step vectors describe the repetition: in case of the box, the row step vector is (w,0) and
 *  the column step vector is (h,0) (w and h are the box width and heigth respectively). Hence
 *  the kernel will be repeated seamlessly.
 *
 *  The kernel's boundary in case of the diamond kernel is:
 *
 *    (o,o+c,o+c+r,o+r)
 *
 *  (o = kernel_origin, r = row_step, c = column_step)
 *
 *  Formally, the kernel will be placed a positions
 *
 *    p(i,j) = p0 + i * row_step + j * column_step
 *
 *  p0 is a position chosen by the fill alogorithm or the "origin", if enhanced_fill is false.
 *
 *  This pattern is overlaid with the polygon to fill and all instances where the kernel moved by p(i,j) is entirely inside
 *  the polygon generate a fill cell instance with a displacement of p.
 *
 *  Afterwards, the residual parts are computed by subtracting all moved kernels from the polygon to fill.
 *  This implies that ideally the fc_boxes should overlap while they are repeated with row_step and column_step.
 *
 *  As a practical consequence, if all fill cell geometries are within the kernel's boundary, they will also
 *  be within the polygon to fill.
 */

DB_PUBLIC bool 
fill_region (db::Cell *cell, const db::Polygon &fp, db::cell_index_type fill_cell_index, const db::Box &fc_box, const db::Point &origin, bool enhanced_fill, 
             std::vector <db::Polygon> *remaining_parts = 0, const db::Vector &fill_margin = db::Vector ());

DB_PUBLIC bool
fill_region (db::Cell *cell, const db::Polygon &fp, db::cell_index_type fill_cell_index, const db::Vector &kernel_origin, const db::Vector &row_step, const db::Vector &column_step, const db::Point &origin, bool enhanced_fill,
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

DB_PUBLIC void
fill_region (db::Cell *cell, const db::Region &fp, db::cell_index_type fill_cell_index, const db::Vector &kernel_origin, const db::Vector &row_step, const db::Vector &column_step, const db::Point &origin, bool enhanced_fill,
             db::Region *remaining_parts = 0, const db::Vector &fill_margin = db::Vector (), db::Region *remaining_polygons = 0);

}

