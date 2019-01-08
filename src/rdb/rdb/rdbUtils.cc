
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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


#include "rdbUtils.h"

#include "dbLayout.h"
#include "dbLayoutUtils.h"
#include "dbRecursiveShapeIterator.h"

namespace rdb
{

RDB_PUBLIC void 
scan_layer (rdb::Category *cat, const db::Layout &layout, unsigned int layer, const db::Cell *from, int levels)
{
  rdb::Database *rdb = cat->database ();
  if (! rdb) {
    return;
  }

  rdb::Cell *rdb_top_cell = 0;
  if (from) {
    rdb_top_cell = rdb->create_cell (layout.cell_name (from->cell_index ()));
  }

  std::set<db::cell_index_type> cells;
  if (from) {
    from->collect_called_cells (cells, levels);
    cells.insert (from->cell_index ());
  }

  for (db::Layout::const_iterator c = layout.begin (); c != layout.end (); ++c) { 

    if (from && cells.find (c->cell_index ()) == cells.end ()) {
      continue;
    }

    const db::Cell &cell = *c;
    if (cell.shapes (layer).size () > 0) {

      std::string cn = layout.cell_name (cell.cell_index ());
      const rdb::Cell *rdb_cell = rdb->cell_by_qname (cn);
      if (! rdb_cell) {

        rdb::Cell *rdb_cell_nc = rdb->create_cell (cn);
        rdb_cell = rdb_cell_nc;

        if (from) {
          std::pair<bool, db::ICplxTrans> ctx = db::find_layout_context (layout, cell.cell_index (), from->cell_index ());
          if (ctx.first) {
            db::DCplxTrans t = db::DCplxTrans (layout.dbu ()) * db::DCplxTrans (ctx.second) * db::DCplxTrans (1.0 / layout.dbu ());
            rdb_cell_nc->references ().insert (Reference (t, rdb_top_cell->id ()));
          }
        }
      
      }

      for (db::ShapeIterator shape = cell.shapes (layer).begin (db::ShapeIterator::All); ! shape.at_end (); ++shape) {

        if (shape->is_polygon () || shape->is_path () || shape->is_box ()) {

          db::Polygon poly;
          shape->polygon (poly);
          rdb::Item *item = rdb->create_item (rdb_cell->id (), cat->id ());
          item->values ().add (new rdb::Value <db::DPolygon> (poly.transformed (db::CplxTrans (layout.dbu ()))));

        } else if (shape->is_edge ()) {

          db::Edge edge;
          shape->edge (edge);
          rdb::Item *item = rdb->create_item (rdb_cell->id (), cat->id ());
          item->values ().add (new rdb::Value <db::DEdge> (edge.transformed (db::CplxTrans (layout.dbu ()))));

        }

      }

    }

  }
}

RDB_PUBLIC void 
scan_layer (rdb::Category *cat, const db::RecursiveShapeIterator &iter)
{
  if (! iter.top_cell () || ! iter.layout ()) {
    return;
  }

  rdb::Database *rdb = cat->database ();
  if (! rdb) {
    return;
  }

  rdb::Cell *rdb_cell = rdb->create_cell (iter.layout ()->cell_name (iter.top_cell ()->cell_index ()));

  for (db::RecursiveShapeIterator i = iter; ! i.at_end (); ++i) {

    if (i.shape ().is_polygon () || i.shape ().is_path () || i.shape ().is_box ()) {

      db::Polygon poly;
      i.shape ().polygon (poly);
      rdb::Item *item = rdb->create_item (rdb_cell->id (), cat->id ());
      item->values ().add (new rdb::Value <db::DPolygon> (poly.transformed (db::CplxTrans (iter.layout ()->dbu ()) * i.trans ())));

    } else if (i.shape ().is_edge ()) {

      db::Edge edge;
      i.shape ().edge (edge);
      rdb::Item *item = rdb->create_item (rdb_cell->id (), cat->id ());
      item->values ().add (new rdb::Value <db::DEdge> (edge.transformed (db::CplxTrans (iter.layout ()->dbu ()) * i.trans ())));

    }

  }
}

}

