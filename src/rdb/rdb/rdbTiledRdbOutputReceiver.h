
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



#ifndef HDR_rdbTiledRdbOutputReceiver
#define HDR_rdbTiledRdbOutputReceiver

#include "rdb.h"

#include "dbTilingProcessor.h"

namespace rdb
{

/**
 *  @brief A helper class for the generic implementation of the insert functionality
 */
class RdbInserter
{
public:
  RdbInserter (rdb::Database *rdb, rdb::id_type cell_id, rdb::id_type category_id, const db::CplxTrans &trans);

  template <class T>
  void operator() (const T &t)
  {
    rdb::Item *item = mp_rdb->create_item (m_cell_id, m_category_id);
    item->add_value (t.transformed (m_trans));
  }

  void operator() (const db::SimplePolygon &t);

private:
  rdb::Database *mp_rdb;
  rdb::id_type m_cell_id, m_category_id;
  const db::CplxTrans m_trans;
};

/**
 *  @brief A receiver for the db::TilingProcessor putting the output to the given RDB
 */
class TiledRdbOutputReceiver
  : public db::TileOutputReceiver
{
public:
  TiledRdbOutputReceiver (rdb::Database *rdb, size_t cell_id, size_t category_id);

  void put (size_t ix, size_t iy, const db::Box &tile, size_t id, const tl::Variant &obj, double dbu, const db::ICplxTrans &trans, bool clip);

private:
  rdb::Database *mp_rdb;
  size_t m_cell_id, m_category_id;
};

}

#endif

