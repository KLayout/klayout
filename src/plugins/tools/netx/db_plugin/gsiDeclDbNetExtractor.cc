/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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


#include "dbNetExtractor.h"

#include "gsiDecl.h"

namespace gsi
{

gsi::Class<db::NetLayer> decl_NetLayer ("db", "NetLayer",
  gsi::method ("layer_index", &db::NetLayer::layer_index,
    "@@@"
  ),
  "@brief The net extractor\n"
  "\n"
  "This class has been introduced in version 0.26."
);

void open2 (db::NetExtractor *ex, const db::Layout *orig_layout, const db::Cell *cell)
{
  ex->open (*orig_layout, cell->cell_index ());
}

gsi::Class<db::NetExtractor> decl_NetNetExtractor ("db", "NetExtractor",
  gsi::method ("open", &db::NetExtractor::open, gsi::arg ("orig_layout"), gsi::arg ("orig_top_cell_index"),
    "@@@"
  ) +
  gsi::method_ext ("open", &open2, gsi::arg ("orig_layout"), gsi::arg ("orig_top_cell"),
    "@@@"
  ) +
  gsi::method ("load", &db::NetExtractor::load, gsi::arg ("layer_index"),
    "@@@"
  ) +
  gsi::method ("bool_and", &db::NetExtractor::bool_and, gsi::arg ("a"), gsi::arg ("b"),
    "@@@"
  ) +
  gsi::method ("bool_not", &db::NetExtractor::bool_not, gsi::arg ("a"), gsi::arg ("b"),
    "@@@"
  ) +
  gsi::factory ("layout_copy", &db::NetExtractor::layout_copy,
    "@@@"
  ),
  "@brief The net extractor\n"
  "\n"
  "This class has been introduced in version 0.26."
);

}
