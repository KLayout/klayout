
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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


#ifndef HDR_rdbUtils
#define HDR_rdbUtils

#include "rdb.h"
#include "dbTypes.h"

namespace db
{
  class Layout;
  class Cell;
  class RecursiveShapeIterator;
}

namespace rdb
{

/**
 *  @brief Scan an layer into an RDB context
 *
 *  This method creates RDB items from the shapes of the given layer.
 *  It will scan the layer hierarchically, i.e. shapes are put into every cell.
 *  It will use the given category to store the items.
 *
 *  If "from" is 0, all cells will be scanned. Levels are the number of hierarchy levels scanned if 
 *  "from" is given. -1 means "all levels".
 */
RDB_PUBLIC void scan_layer (rdb::Category *cat, const db::Layout &layout, unsigned int layer, const db::Cell *from_cell = 0, int levels = -1);

/**
 *  @brief Scans a recursive shape iterator into a RDB category
 */
RDB_PUBLIC void scan_layer (rdb::Category *cat, const db::RecursiveShapeIterator &iter);

}

#endif

