
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

#include "gsiDecl.h"
#include "dbDeepShapeStore.h"
#include "tlGlobPattern.h"

namespace gsi
{

static void set_or_add_breakout_cells (db::DeepShapeStore *dss, const std::string &pattern, bool add, unsigned int layout_index = std::numeric_limits<unsigned int>::max ())
{
  //  set or add for all
  if (layout_index == std::numeric_limits<unsigned int>::max ()) {
    for (unsigned int l = 0; l < dss->layouts (); ++l) {
      set_or_add_breakout_cells (dss, pattern, add, l);
    }
    return;
  }

  std::set<db::cell_index_type> cc;

  if (! pattern.empty ()) {
    tl::GlobPattern p (pattern);
    const db::Layout &ly = dss->layout (layout_index);
    for (db::Layout::const_iterator ci = ly.begin (); ci != ly.end (); ++ci) {
      if (p.match (ly.cell_name (ci->cell_index ()))) {
        cc.insert (ci->cell_index ());
      }
    }
  }

  if (! add) {
    dss->clear_breakout_cells (layout_index);
  }
  if (! cc.empty ()) {
    dss->add_breakout_cells (layout_index, cc);
  }
}

static void clear_breakout_cells (db::DeepShapeStore *dss)
{
  set_or_add_breakout_cells (dss, std::string (), false);
}

static void set_breakout_cells (db::DeepShapeStore *dss, unsigned int layout_index, const std::vector<db::cell_index_type> &cc)
{
  std::set<db::cell_index_type> cs (cc.begin (), cc.end ());
  dss->set_breakout_cells (layout_index, cs);
}

static void set_breakout_cells2 (db::DeepShapeStore *dss, unsigned int layout_index, const std::string &pattern)
{
  set_or_add_breakout_cells (dss, pattern, false, layout_index);
}

static void set_breakout_cells3 (db::DeepShapeStore *dss, const std::string &pattern)
{
  set_or_add_breakout_cells (dss, pattern, false);
}

static void add_breakout_cells (db::DeepShapeStore *dss, unsigned int layout_index, const std::vector<db::cell_index_type> &cc)
{
  std::set<db::cell_index_type> cs (cc.begin (), cc.end ());
  dss->add_breakout_cells (layout_index, cs);
}

static void add_breakout_cells2 (db::DeepShapeStore *dss, unsigned int layout_index, const std::string &pattern)
{
  set_or_add_breakout_cells (dss, pattern, true, layout_index);
}

static void add_breakout_cells3 (db::DeepShapeStore *dss, const std::string &pattern)
{
  set_or_add_breakout_cells (dss, pattern, true);
}

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
  gsi::method ("wants_all_cells=", &db::DeepShapeStore::set_wants_all_cells, gsi::arg ("flag"),
    "@brief Sets a flag wether to copy the full hierarchy for the working layouts\n"
    "\n"
    "The DeepShapeStore object keeps a copy of the original hierarchy internally for the working layouts.\n"
    "By default, this hierarchy is mapping only non-empty cells. While the operations proceed, more cells "
    "may need to be added. This conservative approach saves some memory, but the update operations may "
    "reduce overall performance. Setting this flag to 'true' switches to a mode where the full "
    "hierarchy is copied always. This will take more memory but may save CPU time.\n"
    "\n"
    "This attribute has been introduced in version 0.28.10."
  ) +
  gsi::method ("wants_all_cells", &db::DeepShapeStore::wants_all_cells,
    "@brief Gets a flag wether to copy the full hierarchy for the working layouts\n"
    "This attribute has been introduced in version 0.28.10."
  ) +
  gsi::method ("reject_odd_polygons=", &db::DeepShapeStore::set_reject_odd_polygons, gsi::arg ("count"),
    "@brief Sets a flag indicating whether to reject odd polygons\n"
    "\n"
    "Some kind of 'odd' (e.g. non-orientable) polygons may spoil the functionality "
    "because they cannot be handled properly. By using this flag, the shape store "
    "we reject these kind of polygons. The default is 'accept' (without warning).\n"
    "\n"
    "This attribute has been introduced in version 0.27."
  ) +
  gsi::method ("reject_odd_polygons", &db::DeepShapeStore::reject_odd_polygons,
    "@brief Gets a flag indicating whether to reject odd polygons.\n"
    "This attribute has been introduced in version 0.27."
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
  ) +
  gsi::method ("subcircuit_hierarchy_for_nets=", &db::DeepShapeStore::set_subcircuit_hierarchy_for_nets, gsi::arg ("value"),
    "@brief Sets a value indicating whether to build a subcircuit hierarchy per net\n"
    "\n"
    "\nThis flag is used to determine the way, net subcircuit hierarchies are built:\n"
    "when true, subcells are created for subcircuits on a net. Otherwise the net\n"
    "shapes are produced flat inside the cell they appear on.\n"
    "\n"
    "This attribute has been introduced in version 0.28.4"
  ) +
  gsi::method ("subcircuit_hierarchy_for_nets=", &db::DeepShapeStore::set_subcircuit_hierarchy_for_nets, gsi::arg ("value"),
    "@brief Gets a value indicating whether to build a subcircuit hierarchy per net\n"
    "See \\subcircuit_hierarchy_for_nets= for details.\n"
    "\n"
    "This attribute has been introduced in version 0.28.4"
  ) +
  gsi::method ("clear_breakout_cells", &db::DeepShapeStore::clear_breakout_cells, gsi::arg ("layout_index"),
    "@brief Clears the breakout cells\n"
    "Breakout cells are a feature by which hierarchy handling can be disabled for specific cells. "
    "If cells are specified as breakout cells, they don't interact with neighbor or parent cells, hence "
    "are virtually isolated. Breakout cells are useful to shortcut hierarchy evaluation for cells which "
    "are otherwise difficult to handle. An example are memory array cells with overlaps to their neighbors: "
    "a precise handling of such cells would generate variants and the boundary of the array. Although precise, "
    "this behavior leads to partial flattening and propagation of shapes. In consequence, this will also "
    "result in wrong device detection in LVS applications. In such cases, these array cells can be declared "
    "'breakout cells' which makes them isolated entities and variant generation does not happen.\n"
    "\n"
    "See also \\set_breakout_cells and \\add_breakout_cells.\n"
    "\n"
    "This method has been added in version 0.26.1\n"
  ) +
  gsi::method_ext ("clear_breakout_cells", &clear_breakout_cells,
    "@brief Clears the breakout cells\n"
    "See the other variant of \\clear_breakout_cells for details.\n"
    "\n"
    "This method has been added in version 0.26.1\n"
  ) +
  gsi::method_ext ("set_breakout_cells", &set_breakout_cells, gsi::arg ("layout_index"), gsi::arg ("cells"),
    "@brief Sets the breakout cell list (as cell indexes) for the given layout inside the store\n"
    "See \\clear_breakout_cells for an explanation of breakout cells.\n"
    "\n"
    "This method has been added in version 0.26.1\n"
  ) +
  gsi::method_ext ("set_breakout_cells", &set_breakout_cells2, gsi::arg ("layout_index"), gsi::arg ("pattern"),
    "@brief Sets the breakout cell list (as cell name pattern) for the given layout inside the store\n"
    "See \\clear_breakout_cells for an explanation of breakout cells.\n"
    "\n"
    "This method has been added in version 0.26.1\n"
  ) +
  gsi::method_ext ("set_breakout_cells", &set_breakout_cells3, gsi::arg ("pattern"),
    "@brief Sets the breakout cell list (as cell name pattern) for the all layouts inside the store\n"
    "See \\clear_breakout_cells for an explanation of breakout cells.\n"
    "\n"
    "This method has been added in version 0.26.1\n"
  ) +
  gsi::method_ext ("add_breakout_cells", &add_breakout_cells, gsi::arg ("layout_index"), gsi::arg ("cells"),
    "@brief Adds cell indexes to the breakout cell list for the given layout inside the store\n"
    "See \\clear_breakout_cells for an explanation of breakout cells.\n"
    "\n"
    "This method has been added in version 0.26.1\n"
  ) +
  gsi::method_ext ("add_breakout_cell", &add_breakout_cells, gsi::arg ("layout_index"), gsi::arg ("cell_index"),
    "@brief Adds a cell indexe to the breakout cell list for the given layout inside the store\n"
    "See \\clear_breakout_cells for an explanation of breakout cells.\n"
    "\n"
    "This method has been added in version 0.26.1\n"
  ) +
  gsi::method_ext ("add_breakout_cells", &add_breakout_cells2, gsi::arg ("layout_index"), gsi::arg ("pattern"),
    "@brief Adds cells (given by a cell name pattern) to the breakout cell list for the given layout inside the store\n"
    "See \\clear_breakout_cells for an explanation of breakout cells.\n"
    "\n"
    "This method has been added in version 0.26.1\n"
  ) +
  gsi::method_ext ("add_breakout_cells", &add_breakout_cells3, gsi::arg ("pattern"),
    "@brief Adds cells (given by a cell name pattern) to the breakout cell list to all layouts inside the store\n"
    "See \\clear_breakout_cells for an explanation of breakout cells.\n"
    "\n"
    "This method has been added in version 0.26.1\n"
  ) +
  gsi::method ("push_state", &db::DeepShapeStore::push_state,
    "@brief Pushes the store's state on the state state\n"
    "This will save the stores state (\\threads, \\max_vertex_count, \\max_area_ratio, breakout cells ...) on "
    "the state stack. \\pop_state can be used to restore the state.\n"
    "\n"
    "This method has been added in version 0.26.1\n"
  ) +
  gsi::method ("pop_state", &db::DeepShapeStore::pop_state,
    "@brief Restores the store's state on the state state\n"
    "This will restore the state pushed by \\push_state.\n"
    "\n"
    "This method has been added in version 0.26.1\n"
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
