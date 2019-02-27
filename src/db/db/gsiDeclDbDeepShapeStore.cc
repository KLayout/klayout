
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

#include "gsiDecl.h"
#include "dbDeepShapeStore.h"

namespace gsi
{

Class<db::DeepShapeStore> decl_dbDeepShapeStore ("db", "DeepShapeStore",
  gsi::method ("instance_count", &db::DeepShapeStore::instance_count,
    "@hide\n"
  ) +
  gsi::method ("is_singular?", &db::DeepShapeStore::is_singular,
    "@brief Gets a value indicating whether there is a single layout variant\n"
    "\n"
    "Specifically for network extraction, singular DSS objects are required. "
    "Multiple layouts may be present if different sources of layouts have "
    "been used. Such DSS objects are not usable for network extraction."
  ) +
  gsi::method ("threads=", &db::DeepShapeStore::set_threads, gsi::arg ("n"),
    "@brief Sets the number of threads to allocate for the hierarchical processor\n"
  ) +
  gsi::method ("threads", &db::DeepShapeStore::threads,
    "@brief Gets the number of threads.\n"
  ) +
  gsi::method ("max_vertex_count=", &db::DeepShapeStore::set_max_vertex_count, gsi::arg ("count"),
    "@brief Sets the maximum vertex count default value\n"
    "\n"
    "This parameter is used to simplify complex polygons. It is used by\n"
    "create_polygon_layer with the default parameters. It's also used by\n"
    "boolean operations when they deliver their output.\n"
  ) +
  gsi::method ("max_vertex_count", &db::DeepShapeStore::max_vertex_count,
    "@brief Gets the maximum vertex count.\n"
  ) +
  gsi::method ("max_area_ratio=", &db::DeepShapeStore::set_max_area_ratio, gsi::arg ("ratio"),
    "@brief Sets the max. area ratio for bounding box vs. polygon area\n"
    "\n"
    "This parameter is used to simplify complex polygons. It is used by\n"
    "create_polygon_layer with the default parameters. It's also used by\n"
    "boolean operations when they deliver their output.\n"
  ) +
  gsi::method ("max_area_ratio", &db::DeepShapeStore::max_area_ratio,
    "@brief Gets the max. area ratio.\n"
  ) +
  gsi::method ("text_property_name=", &db::DeepShapeStore::set_text_property_name, gsi::arg ("name"),
    "@brief Sets the text property name.\n"
    "\n"
    "If set to a non-null variant, text strings are attached to the generated boxes\n"
    "as properties with this particular name. This option has an effect only if the\n"
    "text_enlargement property is not negative.\n"
    "By default, the name is empty.\n"
  ) +
  gsi::method ("text_property_name", &db::DeepShapeStore::text_property_name,
    "@brief Gets the text property name.\n"
  ) +
  gsi::method ("text_enlargement=", &db::DeepShapeStore::set_text_enlargement, gsi::arg ("value"),
    "@brief Sets the text enlargement value\n"
    "\n"
    "If set to a non-negative value, text objects are converted to boxes with the\n"
    "given enlargement (width = 2 * enlargement). The box centers are identical\n"
    "to the original location of the text.\n"
    "If this value is negative (the default), texts are ignored.\n"
  ) +
  gsi::method ("text_enlargement", &db::DeepShapeStore::text_enlargement,
    "@brief Gets the text enlargement value.\n"
  ),
  "@brief An opaque layout heap for the deep region processor\n"
  "\n"
  "This class is used for keeping intermediate, hierarchical data for the "
  "deep region processor. It is used in conjunction with the region "
  "constructor to create a deep (hierarchical) region."
  "\n"
  "@code\n"
  "layout = ... # a layout\n"
  "layer = ...  # a layer\n"
  "cell = ...   # a cell (initial cell for the deep region)\n"
  "dss = RBA::DeepShapeStore::new\n"
  "region = RBA::Region::new(cell.begin(layer), dss)\n"
  "@/code\n"
  "\n"
  "The DeepShapeStore object also supplies some configuration options "
  "for the operations acting on the deep regions. See for example \\threads=.\n"
  "\n"
  "This class has been introduced in version 0.26.\n"
);

}
