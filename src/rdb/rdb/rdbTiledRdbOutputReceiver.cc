
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


#include "rdbTiledRdbOutputReceiver.h"
#include "dbPolygonTools.h"

namespace rdb
{

// -------------------------------------------------------------------------
//  RdbInserter implementation

RdbInserter::RdbInserter (rdb::Database *rdb, rdb::id_type cell_id, rdb::id_type category_id, const db::CplxTrans &trans)
  : mp_rdb (rdb), m_cell_id (cell_id), m_category_id (category_id), m_trans (trans)
{
  //  .. nothing yet ..
}

void RdbInserter::operator() (const db::SimplePolygon &t)
{
  rdb::Item *item = mp_rdb->create_item (m_cell_id, m_category_id);
  item->add_value (db::simple_polygon_to_polygon (t).transformed (m_trans));
}

// -------------------------------------------------------------------------
//  TiledRdbOutputReceiver implementation

TiledRdbOutputReceiver::TiledRdbOutputReceiver (rdb::Database *rdb, size_t cell_id, size_t category_id)
  : mp_rdb (rdb), m_cell_id (cell_id), m_category_id (category_id)
{
  //  .. nothing yet ..
}

void TiledRdbOutputReceiver::put (size_t /*ix*/, size_t /*iy*/, const db::Box &tile, size_t /*id*/, const tl::Variant &obj, double dbu, const db::ICplxTrans &trans, bool clip)
{
  db::CplxTrans t (db::CplxTrans (dbu) * db::CplxTrans (trans));
  RdbInserter inserter (mp_rdb, m_cell_id, m_category_id, t);

  if (! db::insert_var (inserter, obj, tile, clip)) {
    //  try to_string as the last resort
    rdb::Item *item = mp_rdb->create_item (m_cell_id, m_category_id);
    item->add_value (std::string (obj.to_string ()));
  }
}

}


