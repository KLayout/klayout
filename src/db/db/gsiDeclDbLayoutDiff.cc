
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
#include "gsiSignals.h"

#include "dbLayoutDiff.h"
#include "dbLayout.h"

#include "tlEvents.h"

namespace {

class LayoutDiff
  : public db::DifferenceReceiver
{
public:
  LayoutDiff ()
    : mp_layout_a (0), mp_cell_a (0), m_layer_index_a (0),
      mp_layout_b (0), mp_cell_b (0), m_layer_index_b (0)
  {
    // .. nothing yet ..
  }

  bool compare_layouts (const db::Layout *a, const db::Layout *b, unsigned int flags, db::Coord tolerance)
  {
    if (!a || !b) {
      return false;
    }

    bool res = false;

    mp_layout_a = a;
    mp_layout_b = b;
    try {
      res = db::compare_layouts(*a, *b, flags, tolerance, *this);
      mp_layout_a = mp_layout_b = 0;
    } catch (...) {
      mp_layout_a = mp_layout_b = 0;
    }

    return res;
  }

  bool compare_cells (const db::Cell *a, const db::Cell *b, unsigned int flags, db::Coord tolerance)
  {
    if (!a || !b) {
      return false;
    }

    bool res = false;

    mp_layout_a = a->layout ();
    mp_layout_b = b->layout ();
    tl_assert (mp_layout_a != 0);
    tl_assert (mp_layout_b != 0);

    try {
      res = db::compare_layouts(*mp_layout_a, a->cell_index (), *mp_layout_b, b->cell_index (), flags, tolerance, *this);
      mp_layout_a = mp_layout_b = 0;
    } catch (...) {
      mp_layout_a = mp_layout_b = 0;
    }

    return res;
  }

  virtual void dbu_differs (double dbu_a, double dbu_b)
  {
    dbu_differs_event (dbu_a, dbu_b);
  }

  virtual void layer_in_a_only (const db::LayerProperties &la)
  {
    layer_in_a_only_event (la);
  }

  virtual void layer_in_b_only (const db::LayerProperties &lb)
  {
    layer_in_b_only_event (lb);
  }

  virtual void layer_name_differs (const db::LayerProperties &la, const db::LayerProperties &lb)
  {
    layer_name_differs_event (la, lb);
  }

  virtual void cell_name_differs (const std::string & /*cellname_a*/, db::cell_index_type cia, const std::string & /*cellname_b*/, db::cell_index_type cib)
  {
    cell_name_differs_event (&mp_layout_a->cell (cia), &mp_layout_b->cell (cib));
  }

  virtual void cell_in_a_only (const std::string & /*cellname*/, db::cell_index_type ci)
  {
    cell_in_a_only_event (&mp_layout_a->cell (ci));
  }

  virtual void cell_in_b_only (const std::string & /*cellname*/, db::cell_index_type ci)
  {
    cell_in_b_only_event (&mp_layout_b->cell (ci));
  }

  virtual void bbox_differs (const db::Box &ba, const db::Box &bb)
  {
    bbox_differs_event (ba, bb);
  }

  virtual void begin_cell (const std::string & /*cellname*/, db::cell_index_type cia, db::cell_index_type cib)
  {
    mp_cell_a = &mp_layout_a->cell (cia);
    mp_cell_b = &mp_layout_b->cell (cib);
    begin_cell_event (mp_cell_a, mp_cell_b);
  }

  virtual void begin_inst_differences ()
  {
    begin_inst_differences_event ();
  }

  virtual void instances_in_a_only (const std::vector <db::CellInstArrayWithProperties> &anotb, const db::Layout & /*a*/)
  {
    for (std::vector <db::CellInstArrayWithProperties>::const_iterator i = anotb.begin (); i != anotb.end (); ++i) {
      instance_in_a_only_event (*i, i->properties_id ());
    }
  }

  virtual void instances_in_b_only (const std::vector <db::CellInstArrayWithProperties> &bnota, const db::Layout & /*b*/)
  {
    for (std::vector <db::CellInstArrayWithProperties>::const_iterator i = bnota.begin (); i != bnota.end (); ++i) {
      instance_in_b_only_event (*i, i->properties_id ());
    }
  }

  virtual void end_inst_differences ()
  {
    end_inst_differences_event ();
  }

  virtual void begin_layer (const db::LayerProperties &layer, unsigned int layer_index_a, bool is_valid_a, unsigned int layer_index_b, bool is_valid_b)
  {
    m_layer_index_a = is_valid_a ? int (layer_index_a) : -1;
    m_layer_index_b = is_valid_b ? int (layer_index_b) : -1;
    begin_layer_event (layer, m_layer_index_a, m_layer_index_b);
  }

  virtual void per_layer_bbox_differs (const db::Box &ba, const db::Box &bb)
  {
    per_layer_bbox_differs_event (ba, bb);
  }

  virtual void begin_polygon_differences ()
  {
    begin_polygon_differences_event ();
  }

  virtual void detailed_diff (const db::PropertiesRepository & /*pr*/, const std::vector <std::pair <db::Polygon, db::properties_id_type> > &a, const std::vector <std::pair <db::Polygon, db::properties_id_type> > &b)
  {
    for (std::vector <std::pair <db::Polygon, db::properties_id_type> >::const_iterator i = a.begin (); i != a.end (); ++i) {
      polygon_in_a_only_event (i->first, i->second);
    }
    for (std::vector <std::pair <db::Polygon, db::properties_id_type> >::const_iterator i = b.begin (); i != b.end (); ++i) {
      polygon_in_b_only_event (i->first, i->second);
    }
  }

  virtual void end_polygon_differences ()
  {
    end_polygon_differences_event ();
  }

  virtual void begin_path_differences ()
  {
    begin_path_differences_event ();
  }

  virtual void detailed_diff (const db::PropertiesRepository & /*pr*/, const std::vector <std::pair <db::Path, db::properties_id_type> > &a, const std::vector <std::pair <db::Path, db::properties_id_type> > &b)
  {
    for (std::vector <std::pair <db::Path, db::properties_id_type> >::const_iterator i = a.begin (); i != a.end (); ++i) {
      path_in_a_only_event (i->first, i->second);
    }
    for (std::vector <std::pair <db::Path, db::properties_id_type> >::const_iterator i = b.begin (); i != b.end (); ++i) {
      path_in_b_only_event (i->first, i->second);
    }
  }

  virtual void end_path_differences ()
  {
    end_path_differences_event ();
  }

  virtual void begin_box_differences ()
  {
    begin_box_differences_event ();
  }

  virtual void detailed_diff (const db::PropertiesRepository & /*pr*/, const std::vector <std::pair <db::Box, db::properties_id_type> > &a, const std::vector <std::pair <db::Box, db::properties_id_type> > &b)
  {
    for (std::vector <std::pair <db::Box, db::properties_id_type> >::const_iterator i = a.begin (); i != a.end (); ++i) {
      box_in_a_only_event (i->first, i->second);
    }
    for (std::vector <std::pair <db::Box, db::properties_id_type> >::const_iterator i = b.begin (); i != b.end (); ++i) {
      box_in_b_only_event (i->first, i->second);
    }
  }

  virtual void end_box_differences ()
  {
    end_box_differences_event ();
  }

  virtual void begin_edge_differences ()
  {
    begin_edge_differences_event ();
  }

  virtual void detailed_diff (const db::PropertiesRepository & /*pr*/, const std::vector <std::pair <db::Edge, db::properties_id_type> > &a, const std::vector <std::pair <db::Edge, db::properties_id_type> > &b)
  {
    for (std::vector <std::pair <db::Edge, db::properties_id_type> >::const_iterator i = a.begin (); i != a.end (); ++i) {
      edge_in_a_only_event (i->first, i->second);
    }
    for (std::vector <std::pair <db::Edge, db::properties_id_type> >::const_iterator i = b.begin (); i != b.end (); ++i) {
      edge_in_b_only_event (i->first, i->second);
    }
  }

  virtual void end_edge_differences ()
  {
    end_edge_differences_event ();
  }

  virtual void begin_edge_pair_differences ()
  {
    begin_edge_pair_differences_event ();
  }

  virtual void detailed_diff (const db::PropertiesRepository & /*pr*/, const std::vector <std::pair <db::EdgePair, db::properties_id_type> > &a, const std::vector <std::pair <db::EdgePair, db::properties_id_type> > &b)
  {
    for (std::vector <std::pair <db::EdgePair, db::properties_id_type> >::const_iterator i = a.begin (); i != a.end (); ++i) {
      edge_pair_in_a_only_event (i->first, i->second);
    }
    for (std::vector <std::pair <db::EdgePair, db::properties_id_type> >::const_iterator i = b.begin (); i != b.end (); ++i) {
      edge_pair_in_b_only_event (i->first, i->second);
    }
  }

  virtual void end_edge_pair_differences ()
  {
    end_edge_pair_differences_event ();
  }

  virtual void begin_text_differences ()
  {
    begin_text_differences_event ();
  }

  virtual void detailed_diff (const db::PropertiesRepository & /*pr*/, const std::vector <std::pair <db::Text, db::properties_id_type> > &a, const std::vector <std::pair <db::Text, db::properties_id_type> > &b)
  {
    for (std::vector <std::pair <db::Text, db::properties_id_type> >::const_iterator i = a.begin (); i != a.end (); ++i) {
      text_in_a_only_event (i->first, i->second);
    }
    for (std::vector <std::pair <db::Text, db::properties_id_type> >::const_iterator i = b.begin (); i != b.end (); ++i) {
      text_in_b_only_event (i->first, i->second);
    }
  }

  virtual void end_text_differences ()
  {
    end_text_differences_event ();
  }

  virtual void end_layer ()
  {
    end_layer_event ();
    m_layer_index_a = -1;
    m_layer_index_b = -1;
  }

  virtual void end_cell ()
  {
    end_cell_event ();
  }

  const db::Layout *layout_a () const
  {
    return mp_layout_a;
  }

  const db::Cell *cell_a () const
  {
    return mp_cell_a;
  }

  const db::Layout *layout_b () const
  {
    return mp_layout_b;
  }

  const db::Cell *cell_b () const
  {
    return mp_cell_b;
  }

  int layer_index_a () const
  {
    return m_layer_index_a;
  }

  int layer_index_b () const
  {
    return m_layer_index_b;
  }

  db::LayerProperties layer_info_a () const
  {
    tl_assert (mp_layout_a != 0);
    return mp_layout_a->get_properties (m_layer_index_a);
  }

  db::LayerProperties layer_info_b () const
  {
    tl_assert (mp_layout_b != 0);
    return mp_layout_b->get_properties (m_layer_index_b);
  }

  tl::event<double /*dbu_a*/, double /*dbu_a*/> dbu_differs_event;
  tl::event<const db::LayerProperties & /*a*/> layer_in_a_only_event;
  tl::event<const db::LayerProperties & /*b*/> layer_in_b_only_event;
  tl::event<const db::LayerProperties & /*a*/, const db::LayerProperties & /*b*/> layer_name_differs_event;
  tl::event<const db::Cell * /*ca*/, const db::Cell * /*cb*/> cell_name_differs_event;
  tl::event<const db::Cell * /*c*/> cell_in_a_only_event;
  tl::event<const db::Cell * /*c*/> cell_in_b_only_event;
  tl::event<const db::Box & /*ba*/, const db::Box & /*bb*/> bbox_differs_event;
  tl::event<const db::Cell * /*ca*/, const db::Cell * /*cb*/> begin_cell_event;
  tl::Event begin_inst_differences_event;
  tl::event<const db::CellInstArray & /*anotb*/, db::properties_id_type /*prop_id*/> instance_in_a_only_event;
  tl::event<const db::CellInstArray & /*bnota*/, db::properties_id_type /*prop_id*/> instance_in_b_only_event;
  tl::Event end_inst_differences_event;
  tl::event<const db::LayerProperties & /*layer*/, int /*layer_index_a*/, int /*layer_index_b*/> begin_layer_event;
  tl::event<const db::Box & /*ba*/, const db::Box & /*bb*/> per_layer_bbox_differs_event;
  tl::Event begin_polygon_differences_event;
  tl::event<const db::Polygon & /*anotb*/, db::properties_id_type /*prop_id*/> polygon_in_a_only_event;
  tl::event<const db::Polygon & /*bnota*/, db::properties_id_type /*prop_id*/> polygon_in_b_only_event;
  tl::Event end_polygon_differences_event;
  tl::Event begin_path_differences_event;
  tl::event<const db::Path & /*anotb*/, db::properties_id_type /*prop_id*/> path_in_a_only_event;
  tl::event<const db::Path & /*bnota*/, db::properties_id_type /*prop_id*/> path_in_b_only_event;
  tl::Event end_path_differences_event;
  tl::Event begin_box_differences_event;
  tl::event<const db::Box & /*anotb*/, db::properties_id_type /*prop_id*/> box_in_a_only_event;
  tl::event<const db::Box & /*bnota*/, db::properties_id_type /*prop_id*/> box_in_b_only_event;
  tl::Event end_box_differences_event;
  tl::Event begin_edge_differences_event;
  tl::event<const db::Edge & /*anotb*/, db::properties_id_type /*prop_id*/> edge_in_a_only_event;
  tl::event<const db::Edge & /*bnota*/, db::properties_id_type /*prop_id*/> edge_in_b_only_event;
  tl::Event end_edge_differences_event;
  tl::Event begin_edge_pair_differences_event;
  tl::event<const db::EdgePair & /*anotb*/, db::properties_id_type /*prop_id*/> edge_pair_in_a_only_event;
  tl::event<const db::EdgePair & /*bnota*/, db::properties_id_type /*prop_id*/> edge_pair_in_b_only_event;
  tl::Event end_edge_pair_differences_event;
  tl::Event begin_text_differences_event;
  tl::event<const db::Text & /*anotb*/, db::properties_id_type /*prop_id*/> text_in_a_only_event;
  tl::event<const db::Text & /*bnota*/, db::properties_id_type /*prop_id*/> text_in_b_only_event;
  tl::Event end_text_differences_event;
  tl::Event end_layer_event;
  tl::Event end_cell_event;

private:
  const db::Layout *mp_layout_a;
  const db::Cell *mp_cell_a;
  int m_layer_index_a;
  const db::Layout *mp_layout_b;
  const db::Cell *mp_cell_b;
  int m_layer_index_b;
};

static unsigned int f_silent () {
  return db::layout_diff::f_silent;
}

static unsigned int f_ignore_duplicates () {
  return db::layout_diff::f_ignore_duplicates;
}

static unsigned int f_no_text_orientation () {
  return db::layout_diff::f_no_text_orientation;
}

static unsigned int f_no_properties () {
  return db::layout_diff::f_no_properties;
}

static unsigned int f_no_layer_names () {
  return db::layout_diff::f_no_layer_names;
}

static unsigned int f_verbose () {
  return db::layout_diff::f_verbose;
}

static unsigned int f_boxes_as_polygons () {
  return db::layout_diff::f_boxes_as_polygons;
}

static unsigned int f_flatten_array_insts () {
  return db::layout_diff::f_flatten_array_insts;
}

static unsigned int f_paths_as_polygons () {
  return db::layout_diff::f_paths_as_polygons;
}

static unsigned int f_smart_cell_mapping () {
  return db::layout_diff::f_smart_cell_mapping;
}

static unsigned int f_dont_summarize_missing_layers () {
  return db::layout_diff::f_dont_summarize_missing_layers;
}

static unsigned int f_no_text_details () {
  return db::layout_diff::f_no_text_details;
}

gsi::Class<LayoutDiff> decl_LayoutDiff ("db", "LayoutDiff",
  gsi::constant ("Silent", &f_silent,
    "@brief Silent compare - just report whether the layouts are identical\n"
    "Silent mode will not issue any signals, but instead the return value of the \\LayoutDiff#compare method "
    "will indicate whether the layouts are identical. In silent mode, the compare method will return "
    "immediately once a difference has been encountered so that mode may be much faster than the "
    "full compare.\n"
    "\n"
    "This constant can be used for the flags parameter of \\compare_layouts and \\compare_cells. It can be "
    "compared with other constants to form a flag set."
  ) +
  gsi::constant ("IgnoreDuplicates", &f_ignore_duplicates,
    "@brief Ignore duplicate instances or shapes\n"
    "With this option present, duplicate instances or shapes are ignored and "
    "duplication does not count as a difference.\n"
    "\n"
    "This option has been introduced in version 0.28.9."
  ) +
  gsi::constant ("NoTextOrientation", &f_no_text_orientation,
    "@brief Ignore text orientation\n"
    "This constant can be used for the flags parameter of \\compare_layouts and \\compare_cells. It can be "
    "compared with other constants to form a flag set."
  ) +
  gsi::constant ("NoProperties", &f_no_properties,
    "@brief Ignore properties\n"
    "This constant can be used for the flags parameter of \\compare_layouts and \\compare_cells. It can be "
    "compared with other constants to form a flag set."
  ) +
  gsi::constant ("NoLayerNames", &f_no_layer_names,
    "@brief Do not compare layer names\n"
    "This constant can be used for the flags parameter of \\compare_layouts and \\compare_cells. It can be "
    "compared with other constants to form a flag set."
  ) +
  gsi::constant ("Verbose", &f_verbose,
    "@brief Enables verbose mode (gives details about the differences)\n"
    "\n"
    "See the event descriptions for details about the differences in verbose and non-verbose mode.\n"
    "\n"
    "This constant can be used for the flags parameter of \\compare_layouts and \\compare_cells. It can be "
    "compared with other constants to form a flag set."
  ) +
  gsi::constant ("BoxesAsPolygons", &f_boxes_as_polygons,
    "@brief Compare boxes to polygons\n"
    "This constant can be used for the flags parameter of \\compare_layouts and \\compare_cells. It can be "
    "compared with other constants to form a flag set."
  ) +
  gsi::constant ("FlattenArrayInsts", &f_flatten_array_insts,
    "@brief Compare array instances instance by instance\n"
    "This constant can be used for the flags parameter of \\compare_layouts and \\compare_cells. It can be "
    "compared with other constants to form a flag set."
  ) +
  gsi::constant ("PathsAsPolygons", &f_paths_as_polygons,
    "@brief Compare paths to polygons\n"
    "This constant can be used for the flags parameter of \\compare_layouts and \\compare_cells. It can be "
    "compared with other constants to form a flag set."
  ) +
  gsi::constant ("SmartCellMapping", &f_smart_cell_mapping,
    "@brief Derive smart cell mapping instead of name mapping (available only if top cells are specified)\n"
    "Smart cell mapping is only effective currently when "
    "cells are compared (with \\LayoutDiff#compare with cells instead of layout objects).\n"
    "\n"
    "This constant can be used for the flags parameter of \\compare_layouts and \\compare_cells. It can be "
    "compared with other constants to form a flag set.\n"
  ) +
  gsi::constant ("DontSummarizeMissingLayers", &f_dont_summarize_missing_layers,
    "@brief Don't summarize missing layers\n"
    "If this mode is present, missing layers are treated as empty ones and every shape on the other "
    "layer will be reported as difference.\n"
    "\n"
    "This constant can be used for the flags parameter of \\compare_layouts and \\compare_cells. It can be "
    "compared with other constants to form a flag set."
  ) +
  gsi::constant ("NoTextDetails", &f_no_text_details,
    "@brief Ignore text details (font, size, presentation)\n"
    "This constant can be used for the flags parameter of \\compare_layouts and \\compare_cells. It can be "
    "compared with other constants to form a flag set."
  ) +
  gsi::method ("compare", &LayoutDiff::compare_layouts,
    gsi::arg("a"),
    gsi::arg("b"),
    gsi::arg<int> ("flags", 0),
    gsi::arg<int> ("tolerance", 0),
    "@brief Compares two layouts\n"
    "\n"
    "Compares layer definitions, cells, instances and shapes and properties.\n"
    "Cells are identified by name. Only layers with valid layer and datatype are compared.\n"
    "Several flags can be specified as a bitwise or combination of the constants.\n"
    "\n"
    "@param a The first input layout\n"
    "@param b The second input layout\n"
    "@param flags Flags to use for the comparison\n"
    "@param tolerance A coordinate tolerance to apply (0: exact match, 1: one DBU tolerance is allowed ...)\n"
    "\n"
    "@return True, if the layouts are identical\n"
  ) +
  gsi::method ("compare", &LayoutDiff::compare_cells,
    gsi::arg("a"),
    gsi::arg("b"),
    gsi::arg<int> ("flags", 0),
    gsi::arg<int> ("tolerance", 0),
    "@brief Compares two cells\n"
    "\n"
    "Compares layer definitions, cells, instances and shapes and properties of two layout hierarchies starting from the given cells.\n"
    "Cells are identified by name. Only layers with valid layer and datatype are compared.\n"
    "Several flags can be specified as a bitwise or combination of the constants.\n"
    "\n"
    "@param a The first top cell\n"
    "@param b The second top cell\n"
    "@param flags Flags to use for the comparison\n"
    "@param tolerance A coordinate tolerance to apply (0: exact match, 1: one DBU tolerance is allowed ...)\n"
    "\n"
    "@return True, if the cells are identical\n"
  ) +
  gsi::method ("layout_a", &LayoutDiff::layout_a,
    "@brief Gets the first layout the difference detector runs on"
  ) +
  gsi::method ("cell_a", &LayoutDiff::cell_a,
    "@brief Gets the current cell for the first layout\n"
    "This attribute is the current cell and is set after \\on_begin_cell "
    "and reset after \\on_end_cell."
  ) +
  gsi::method ("layer_index_a", &LayoutDiff::layer_index_a,
    "@brief Gets the current layer for the first layout\n"
    "This attribute is the current cell and is set after \\on_begin_layer "
    "and reset after \\on_end_layer."
  ) +
  gsi::method ("layer_info_a", &LayoutDiff::layer_info_a,
    "@brief Gets the current layer properties for the first layout\n"
    "This attribute is the current cell and is set after \\on_begin_layer "
    "and reset after \\on_end_layer."
  ) +
  gsi::method ("layout_b", &LayoutDiff::layout_b,
    "@brief Gets the second layout the difference detector runs on"
  ) +
  gsi::method ("cell_b", &LayoutDiff::cell_b,
    "@brief Gets the current cell for the second layout\n"
    "This attribute is the current cell and is set after \\on_begin_cell "
    "and reset after \\on_end_cell."
  ) +
  gsi::method ("layer_index_b", &LayoutDiff::layer_index_b,
    "@brief Gets the current layer for the second layout\n"
    "This attribute is the current cell and is set after \\on_begin_layer "
    "and reset after \\on_end_layer."
  ) +
  gsi::method ("layer_info_b", &LayoutDiff::layer_info_b,
    "@brief Gets the current layer properties for the second layout\n"
    "This attribute is the current cell and is set after \\on_begin_layer "
    "and reset after \\on_end_layer."
  ) +
  gsi::event ("on_dbu_differs", &LayoutDiff::dbu_differs_event, gsi::arg ("dbu_a"), gsi::arg ("dbu_b"),
    "@brief This signal indicates a difference in the database units of the layouts\n"
  ) +
  gsi::event ("on_layer_in_a_only", &LayoutDiff::layer_in_a_only_event, gsi::arg ("a"),
    "@brief This signal indicates a layer that is present only in the first layout\n"
  ) +
  gsi::event ("on_layer_in_b_only", &LayoutDiff::layer_in_b_only_event, gsi::arg ("b"),
    "@brief This signal indicates a layer that is present only in the second layout\n"
  ) +
  gsi::event ("on_layer_name_differs", &LayoutDiff::layer_name_differs_event, gsi::arg ("a"), gsi::arg ("b"),
    "@brief This signal indicates a difference in the layer names\n"
  ) +
  gsi::event ("on_cell_name_differs", &LayoutDiff::cell_name_differs_event, gsi::arg ("ca"), gsi::arg ("cb"),
    "@brief This signal indicates a difference in the cell names\n"
    "This signal is emitted in 'smart cell mapping' mode (see \\SmartCellMapping) if two cells are "
    "considered identical, but have different names."
  ) +
  gsi::event ("on_cell_in_a_only", &LayoutDiff::cell_in_a_only_event, gsi::arg ("c"),
    "@brief This signal indicates that the given cell is only present in the first layout\n"
  ) +
  gsi::event ("on_cell_in_b_only", &LayoutDiff::cell_in_b_only_event, gsi::arg ("c"),
    "@brief This signal indicates that the given cell is only present in the second layout\n"
  ) +
  gsi::event ("on_bbox_differs", &LayoutDiff::bbox_differs_event, gsi::arg ("ba"), gsi::arg ("bb"),
    "@brief This signal indicates a difference in the bounding boxes of two cells\n"
    "This signal is only emitted in non-verbose mode (without \\Verbose flag) as a summarizing cell property. "
    "In verbose mode detailed events will be issued indicating the differences.\n"
  ) +
  gsi::event ("on_begin_cell", &LayoutDiff::begin_cell_event, gsi::arg ("ca"), gsi::arg ("cb"),
    "@brief This signal initiates the sequence of events for a cell pair\n"
    "All cell specific events happen between \\begin_cell_event and \\end_cell_event signals."
  ) +
  gsi::event ("on_begin_inst_differences", &LayoutDiff::begin_inst_differences_event,
    "@brief This signal indicates differences in the cell instances\n"
    "In verbose mode (see \\Verbose) more events will follow that indicate the instances that are present only "
    "in the first and second layout (\\instance_in_a_only_event and \\instance_in_b_only_event). "
  ) +
  gsi::event ("on_instance_in_a_only", &LayoutDiff::instance_in_a_only_event, gsi::arg ("anotb"), gsi::arg ("prop_id"),
    "@brief This signal indicates an instance that is present only in the first layout\n"
    "This event is only emitted in verbose mode (\\Verbose flag)."
  ) +
  gsi::event ("on_instance_in_b_only", &LayoutDiff::instance_in_b_only_event, gsi::arg ("bnota"), gsi::arg ("prop_id"),
    "@brief This signal indicates an instance that is present only in the second layout\n"
    "This event is only emitted in verbose mode (\\Verbose flag)."
  ) +
  gsi::event ("on_end_inst_differences", &LayoutDiff::end_inst_differences_event,
    "@brief This signal finishes a sequence of detailed instance difference events\n"
  ) +
  gsi::event ("on_begin_layer", &LayoutDiff::begin_layer_event, gsi::arg ("layer"), gsi::arg ("layer_index_a"), gsi::arg ("layer_index_b"),
    "@brief This signal indicates differences on the given layer\n"
    "In verbose mode (see \\Verbose) more events will follow that indicate the instances that are present only "
    "in the first and second layout (\\polygon_in_a_only_event, \\polygon_in_b_only_event and similar). "
  ) +
  gsi::event ("on_per_layer_bbox_differs", &LayoutDiff::per_layer_bbox_differs_event, gsi::arg ("ba"), gsi::arg ("bb"),
    "@brief This signal indicates differences in the per-layer bounding boxes of the current cell\n"
  ) +
  gsi::event ("on_begin_polygon_differences", &LayoutDiff::begin_polygon_differences_event,
    "@brief This signal indicates differences in the polygons on the current layer\n"
    "The current layer is indicated by the \\begin_layer_event signal or can be obtained from the diff object "
    "through \\LayoutDiff#layer_info_a, \\LayoutDiff#layer_index_a, \\LayoutDiff#layer_info_b and \\LayoutDiff#layer_index_b. "
    "In verbose mode (see \\Verbose flag) more signals will be emitted for polygons that are different between the two layouts."
  ) +
  gsi::event ("on_polygon_in_a_only", &LayoutDiff::polygon_in_a_only_event, gsi::arg ("anotb"), gsi::arg ("prop_id"),
    "@brief This signal indicates a polygon that is present in the first layout only\n"
  ) +
  gsi::event ("on_polygon_in_b_only", &LayoutDiff::polygon_in_b_only_event, gsi::arg ("bnota"), gsi::arg ("prop_id"),
    "@brief This signal indicates a polygon that is present in the second layout only\n"
  ) +
  gsi::event ("on_end_polygon_differences", &LayoutDiff::end_polygon_differences_event,
    "@brief This signal indicates the end of sequence of polygon differences\n"
  ) +
  gsi::event ("on_begin_path_differences", &LayoutDiff::begin_path_differences_event,
    "@brief This signal indicates differences in the paths on the current layer\n"
    "The current layer is indicated by the \\begin_layer_event signal or can be obtained from the diff object "
    "through \\LayoutDiff#layer_info_a, \\LayoutDiff#layer_index_a, \\LayoutDiff#layer_info_b and \\LayoutDiff#layer_index_b. "
    "In verbose mode (see \\Verbose flag) more signals will be emitted for paths that are different between the two layouts."
  ) +
  gsi::event ("on_path_in_a_only", &LayoutDiff::path_in_a_only_event, gsi::arg ("anotb"), gsi::arg ("prop_id"),
    "@brief This signal indicates a path that is present in the first layout only"
  ) +
  gsi::event ("on_path_in_b_only", &LayoutDiff::path_in_b_only_event, gsi::arg ("bnota"), gsi::arg ("prop_id"),
    "@brief This signal indicates a path that is present in the second layout only"
  ) +
  gsi::event ("on_end_path_differences", &LayoutDiff::end_path_differences_event,
    "@brief This signal indicates the end of sequence of path differences\n"
  ) +
  gsi::event ("on_begin_box_differences", &LayoutDiff::begin_box_differences_event,
    "@brief This signal indicates differences in the boxes on the current layer\n"
    "The current layer is indicated by the \\begin_layer_event signal or can be obtained from the diff object "
    "through \\LayoutDiff#layer_info_a, \\LayoutDiff#layer_index_a, \\LayoutDiff#layer_info_b and \\LayoutDiff#layer_index_b. "
    "In verbose mode (see \\Verbose flag) more signals will be emitted for boxes that are different between the two layouts."
  ) +
  gsi::event ("on_box_in_a_only", &LayoutDiff::box_in_a_only_event, gsi::arg ("anotb"), gsi::arg ("prop_id"),
    "@brief This signal indicates a box that is present in the first layout only"
  ) +
  gsi::event ("on_box_in_b_only", &LayoutDiff::box_in_b_only_event, gsi::arg ("bnota"), gsi::arg ("prop_id"),
    "@brief This signal indicates a box that is present in the second layout only"
  ) +
  gsi::event ("on_end_box_differences", &LayoutDiff::end_box_differences_event,
    "@brief This signal indicates the end of sequence of box differences\n"
  ) +
  gsi::event ("on_begin_edge_differences", &LayoutDiff::begin_edge_differences_event,
    "@brief This signal indicates differences in the edges on the current layer\n"
    "The current layer is indicated by the \\begin_layer_event signal or can be obtained from the diff object "
    "through \\LayoutDiff#layer_info_a, \\LayoutDiff#layer_index_a, \\LayoutDiff#layer_info_b and \\LayoutDiff#layer_index_b. "
    "In verbose mode (see \\Verbose flag) more signals will be emitted for edges that are different between the two layouts."
  ) +
  gsi::event ("on_edge_in_a_only", &LayoutDiff::edge_in_a_only_event, gsi::arg ("anotb"), gsi::arg ("prop_id"),
    "@brief This signal indicates an edge that is present in the first layout only"
  ) +
  gsi::event ("on_edge_in_b_only", &LayoutDiff::edge_in_b_only_event, gsi::arg ("bnota"), gsi::arg ("prop_id"),
    "@brief This signal indicates an edge that is present in the second layout only"
  ) +
  gsi::event ("on_end_edge_differences", &LayoutDiff::end_edge_differences_event,
    "@brief This signal indicates the end of sequence of edge differences\n"
  ) +
  gsi::event ("on_begin_edge_pair_differences", &LayoutDiff::begin_edge_pair_differences_event,
    "@brief This signal indicates differences in the edge pairs on the current layer\n"
    "The current layer is indicated by the \\begin_layer_event signal or can be obtained from the diff object "
    "through \\LayoutDiff#layer_info_a, \\LayoutDiff#layer_index_a, \\LayoutDiff#layer_info_b and \\LayoutDiff#layer_index_b. "
    "In verbose mode (see \\Verbose flag) more signals will be emitted for edge pairs that are different between the two layouts."
    "\n"
    "This event has been introduced in version 0.28."
  ) +
  gsi::event ("on_edge_pair_in_a_only", &LayoutDiff::edge_pair_in_a_only_event, gsi::arg ("anotb"), gsi::arg ("prop_id"),
    "@brief This signal indicates an edge pair that is present in the first layout only"
    "\n"
    "This event has been introduced in version 0.28."
  ) +
  gsi::event ("on_edge_pair_in_b_only", &LayoutDiff::edge_pair_in_b_only_event, gsi::arg ("bnota"), gsi::arg ("prop_id"),
    "@brief This signal indicates an edge pair that is present in the second layout only"
    "\n"
    "This event has been introduced in version 0.28."
  ) +
  gsi::event ("on_end_edge_pair_differences", &LayoutDiff::end_edge_pair_differences_event,
    "@brief This signal indicates the end of sequence of edge pair differences\n"
    "\n"
    "This event has been introduced in version 0.28."
  ) +
  gsi::event ("on_begin_text_differences", &LayoutDiff::begin_text_differences_event,
    "@brief This signal indicates differences in the texts on the current layer\n"
    "The current layer is indicated by the \\begin_layer_event signal or can be obtained from the diff object "
    "through \\LayoutDiff#layer_info_a, \\LayoutDiff#layer_index_a, \\LayoutDiff#layer_info_b and \\LayoutDiff#layer_index_b. "
    "In verbose mode (see \\Verbose flag) more signals will be emitted for texts that are different between the two layouts."
  ) +
  gsi::event ("on_text_in_a_only", &LayoutDiff::text_in_a_only_event, gsi::arg ("anotb"), gsi::arg ("prop_id"),
    "@brief This signal indicates a text that is present in the first layout only"
  ) +
  gsi::event ("on_text_in_b_only", &LayoutDiff::text_in_b_only_event, gsi::arg ("bnota"), gsi::arg ("prop_id"),
    "@brief This signal indicates a text that is present in the second layout only"
  ) +
  gsi::event ("on_end_text_differences", &LayoutDiff::end_text_differences_event,
    "@brief This signal indicates the end of sequence of text differences\n"
  ) +
  gsi::event ("on_end_layer", &LayoutDiff::end_layer_event,
    "@brief This signal indicates the end of a sequence of signals for a specific layer\n"
  ) +
  gsi::event ("on_end_cell", &LayoutDiff::end_cell_event,
    "@brief This signal indicates the end of a sequence of signals for a specific cell\n"
  ),
  "@brief The layout compare tool\n"
  "\n"
  "The layout compare tool is a facility to quickly compare layouts and derive events that "
  "give details about the differences. The events are basically emitted following a certain order:\n"
  "\n"
  "@ul\n"
  "@li General configuration events (database units, layers ...) @/li\n"
  "@li \\on_begin_cell @/li\n"
  "@li \\on_begin_inst_differences (if the instances differ) @/li\n"
  "@li details about instance differences (if \\Verbose flag is given) @/li\n"
  "@li \\on_end_inst_differences (if the instances differ) @/li\n"
  "@li \\on_begin_layer @/li\n"
  "@li \\on_begin_polygon_differences (if the polygons differ) @/li\n"
  "@li details about polygon differences (if \\Verbose flag is given) @/li\n"
  "@li \\on_end_polygon_differences (if the polygons differ) @/li\n"
  "@li other shape difference events (paths, boxes, ...) @/li\n"
  "@li \\on_end_layer @/li\n"
  "@li repeated layer event groups @/li\n"
  "@li \\on_end_cell @/li\n"
  "@li repeated cell event groups @/li\n"
  "@/ul\n"
  "\n"
  "To use the diff facility, create a \\LayoutDiff object and call the \\compare_layout or \\compare_cell method:\n"
  "\n"
  "@code\n"
  "lya = ... # layout A\n"
  "lyb = ... # layout B\n"
  "\n"
  "diff = RBA::LayoutDiff::new\n"
  "diff.on_polygon_in_a_only do |poly|\n"
  "  puts \"Polygon in A: #{diff.cell_a.name}@#{diff.layer_info_a.to_s}: #{poly.to_s}\"\n"
  "end\n"
  "diff.on_polygon_in_b_only do |poly|\n"
  "  puts \"Polygon in A: #{diff.cell_b.name}@#{diff.layer_info_b.to_s}: #{poly.to_s}\"\n"
  "end\n"
  "diff.compare(lya, lyb, RBA::LayoutDiff::Verbose + RBA::LayoutDiff::NoLayerNames)\n"
  "@/code\n"
);

}

