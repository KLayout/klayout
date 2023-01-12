
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


#ifndef HDR_dbCellHullGenerator
#define HDR_dbCellHullGenerator

#include "dbLayout.h"
#include "dbPolygon.h"
#include "dbCommon.h"

namespace db {

/**
 *  @brief A cell hull generator
 *
 *  The purpose of this class is to create hulls (a set of minimum polygons enclosing the local cell's content)
 *
 *  This class is used in the hierarchical processor
 */
class DB_PUBLIC CellHullGenerator
{
public:
  CellHullGenerator (const db::Layout &layout);

  CellHullGenerator (const db::Layout &layout, const std::vector <unsigned int> &layers);

  void generate_hull (const db::Cell &cell, std::vector <db::Polygon> &hull);

  void set_small_cell_size (db::Coord sms);

  db::Coord small_cell_size () const
  {
    return m_small_cell_size;
  }

  void set_complexity (size_t complexity);

  size_t complexity () const
  {
    return m_complexity;
  }

private:
  std::vector <unsigned int> m_layers;
  bool m_all_layers;
  db::Coord m_small_cell_size;
  size_t m_complexity;
};

}

#endif

