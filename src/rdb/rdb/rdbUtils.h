
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


#ifndef HDR_rdbUtils
#define HDR_rdbUtils

#include "rdb.h"
#include "dbTypes.h"
#include "dbTrans.h"

namespace db
{
  class Layout;
  class Cell;
  class RecursiveShapeIterator;
  class Shapes;
  class Region;
  class Edges;
  class EdgePairs;
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
RDB_PUBLIC void scan_layer (rdb::Category *cat, const db::RecursiveShapeIterator &iter, bool flat = false);

/**
 *  @brief Scans a recursive shape iterator into a RDB category
 *
 *  This version allows supplying a cell and a transformation. With this information, the function can also handle
 *  pseudo-iterators which don't deliver the information from a layout from from a plain shape collection.
 */
RDB_PUBLIC void scan_layer (rdb::Category *cat, rdb::Cell *cell, const db::CplxTrans &trans, const db::RecursiveShapeIterator &iter, bool flat = false);

/**
 *  @brief Creates RDB items from a recursive shape iterator
 *
 *  This function will produce items from the flattened shape iterator. The items will be stored under
 *  the given cell.
 */
RDB_PUBLIC void create_items_from_iterator (rdb::Database *db, rdb::id_type cell_id, rdb::id_type cat_id, const db::RecursiveShapeIterator &iter);

/**
 *  @brief Creates RDB items from a shape collection
 *
 *  An arbitrary transformation can be applied to translate the shapes before turning them to items.
 *  This transformation is useful for providing the DBU-to-micron conversion.
 */
RDB_PUBLIC void create_items_from_shapes (rdb::Database *db, rdb::id_type cell_id, rdb::id_type cat_id, const db::CplxTrans &trans, const db::Shapes &shapes);

/**
 *  @brief Creates RDB items from a single shape
 *
 *  An arbitrary transformation can be applied to translate the shapes before turning them to items.
 *  This transformation is useful for providing the DBU-to-micron conversion.
 */
RDB_PUBLIC void create_item_from_shape (rdb::Database *db, rdb::id_type cell_id, rdb::id_type cat_id, const db::CplxTrans &trans, const db::Shape &shape);

/**
 *  @brief Creates RDB items from a region
 *
 *  This function will flatten the region and store the resulting items under the given cell.
 *
 *  An arbitrary transformation can be applied to translate the shapes before turning them to items.
 *  This transformation is useful for providing the DBU-to-micron conversion.
 */
RDB_PUBLIC void create_items_from_region (rdb::Database *db, rdb::id_type cell_id, rdb::id_type cat_id, const db::CplxTrans &trans, const db::Region &collection);

/**
 *  @brief Creates RDB items from an edge collection
 *
 *  This function will flatten the edge collection and store the resulting items under the given cell.
 *
 *  An arbitrary transformation can be applied to translate the shapes before turning them to items.
 *  This transformation is useful for providing the DBU-to-micron conversion.
 */
RDB_PUBLIC void create_items_from_edges (rdb::Database *db, rdb::id_type cell_id, rdb::id_type cat_id, const db::CplxTrans &trans, const db::Edges &collection);

/**
 *  @brief Creates RDB items from an edge pair collection
 *
 *  This function will flatten the edge pair collection and store the resulting items under the given cell.
 *
 *  An arbitrary transformation can be applied to translate the shapes before turning them to items.
 *  This transformation is useful for providing the DBU-to-micron conversion.
 */
RDB_PUBLIC void create_items_from_edge_pairs (rdb::Database *db, rdb::id_type cell_id, rdb::id_type cat_id, const db::CplxTrans &trans, const db::EdgePairs &collection);

/**
 *  @brief Creates RDB items from a sequence of integer-type objects
 *
 *  An arbitrary transformation can be applied to translate the shapes before turning them to items.
 *  This transformation is useful for providing the DBU-to-micron conversion.
 */
template <class Trans, class Iter>
RDB_PUBLIC_TEMPLATE void create_items_from_sequence (rdb::Database *db, rdb::id_type cell_id, rdb::id_type cat_id, const Trans &trans, Iter begin, Iter end)
{
  for (Iter o = begin; o != end; ++o) {
    rdb::Item *item = db->create_item (cell_id, cat_id);
    item->values ().add (rdb::make_value (o->transformed (trans)));
  }
}

}

#endif

