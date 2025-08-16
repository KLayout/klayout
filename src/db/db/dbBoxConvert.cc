
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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


#include "dbCommon.h"

#include "dbBoxConvert.h"
#include "dbCell.h"

namespace db
{

DB_PUBLIC db::Box cellinst_box_convert_impl (const db::CellInst &inst, const db::Layout *layout, int layer, bool allow_empty)
{
  if (layer >= 0) {
    return inst.bbox (*layout, layer);
  } else if (allow_empty) {
    return inst.bbox (*layout);
  } else {
    return inst.bbox_with_empty (*layout);
  }
}

DB_PUBLIC db::Box cell_box_convert_impl (const db::Cell &c, int layer, bool allow_empty)
{
  if (layer >= 0) {
    return c.bbox (layer);
  } else if (allow_empty) {
    return c.bbox ();
  } else {
    return c.bbox_with_empty ();
  }
}

template struct box_convert <db::CellInst, true>;
template struct box_convert <db::CellInst, false>;
template struct box_convert <db::Cell, true>;
template struct box_convert <db::Cell, false>;

}

