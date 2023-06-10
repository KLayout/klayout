
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


#ifndef HDR_dbLayoutDiff
#define HDR_dbLayoutDiff

#include "dbCommon.h"

#include "tlProgress.h"
#include "dbTypes.h"
#include "dbBox.h"
#include "dbPath.h"
#include "dbEdge.h"
#include "dbPolygon.h"
#include "dbText.h"
#include "dbCellInst.h"
#include "dbObjectWithProperties.h"

#include <string>

namespace db
{

struct LayerProperties;
class Layout;

namespace layout_diff
{

//  Silent compare - just report whether the layouts are identical
const unsigned int f_silent = 0x01;

//  Ignore text orientation
const unsigned int f_no_text_orientation = 0x02;

//  Ignore properties
const unsigned int f_no_properties = 0x04;

//  Do not compare layer names
const unsigned int f_no_layer_names = 0x10;

//  Be verbose (details about the differences)
const unsigned int f_verbose = 0x20;

//  Compare boxes to polygons
const unsigned int f_boxes_as_polygons = 0x40;

//  Compare array instances instance by instance
const unsigned int f_flatten_array_insts = 0x80;

//  Compare paths to polygons
const unsigned int f_paths_as_polygons = 0x100;

//  Derive smart cell mapping instead of name mapping (available only if top cells are specified)
const unsigned int f_smart_cell_mapping = 0x200;

//  Don't summarize missing layers - print them in detail
const unsigned int f_dont_summarize_missing_layers = 0x400;

//  Ignore text details (font, size, presentation)
const unsigned int f_no_text_details = 0x800;

//  Ignore duplicate instances or shapes
const unsigned int f_ignore_duplicates = 0x1000;

}

/**
 *  @brief A receiver for the differences
 */
class DB_PUBLIC DifferenceReceiver
{
public:
  virtual ~DifferenceReceiver () { }

  virtual void dbu_differs (double /*dbu_a*/, double /*dbu_b*/) { }
  virtual void layer_in_a_only (const db::LayerProperties & /*la*/) { }
  virtual void layer_in_b_only (const db::LayerProperties & /*lb*/) { }
  virtual void layer_name_differs (const db::LayerProperties & /*la*/, const db::LayerProperties & /*lb*/) { }
  virtual void cell_name_differs (const std::string & /*cellname_a*/, db::cell_index_type  /*cia*/, const std::string & /*cellname_b*/, db::cell_index_type  /*cib*/) { }
  virtual void cell_in_a_only (const std::string & /*cellname*/, db::cell_index_type  /*ci*/) { }
  virtual void cell_in_b_only (const std::string & /*cellname*/, db::cell_index_type  /*ci*/) { }
  virtual void bbox_differs (const db::Box & /*ba*/, const db::Box & /*bb*/) { }
  virtual void begin_cell (const std::string & /*cellname*/, db::cell_index_type  /*cia*/, db::cell_index_type /*cib*/) { }
  virtual void begin_inst_differences () { }
  virtual void instances_in_a (const std::vector <db::CellInstArrayWithProperties> & /*insts_a*/, const std::vector <std::string> & /*cell_names*/, const db::PropertiesRepository & /*props*/) { }
  virtual void instances_in_b (const std::vector <db::CellInstArrayWithProperties> & /*insts_b*/, const std::vector <std::string> & /*cell_names*/, const db::PropertiesRepository & /*props*/) { }
  virtual void instances_in_a_only (const std::vector <db::CellInstArrayWithProperties> & /*anotb*/, const db::Layout & /*a*/) { }
  virtual void instances_in_b_only (const std::vector <db::CellInstArrayWithProperties> & /*bnota*/, const db::Layout & /*b*/) { }
  virtual void end_inst_differences () { }
  virtual void begin_layer (const db::LayerProperties & /*layer*/, unsigned int  /*layer_index_a*/, bool  /*is_valid_a*/, unsigned int  /*layer_index_b*/, bool  /*is_valid_b*/) { }
  virtual void per_layer_bbox_differs (const db::Box & /*ba*/, const db::Box & /*bb*/) { }
  virtual void begin_polygon_differences () { }
  virtual void detailed_diff (const db::PropertiesRepository & /*pr*/, const std::vector <std::pair <db::Polygon, db::properties_id_type> > & /*a*/, const std::vector <std::pair <db::Polygon, db::properties_id_type> > & /*b*/) { }
  virtual void end_polygon_differences () { }
  virtual void begin_path_differences () { }
  virtual void detailed_diff (const db::PropertiesRepository & /*pr*/, const std::vector <std::pair <db::Path, db::properties_id_type> > & /*a*/, const std::vector <std::pair <db::Path, db::properties_id_type> > & /*b*/) { }
  virtual void end_path_differences () { }
  virtual void begin_box_differences () { }
  virtual void detailed_diff (const db::PropertiesRepository & /*pr*/, const std::vector <std::pair <db::Box, db::properties_id_type> > & /*a*/, const std::vector <std::pair <db::Box, db::properties_id_type> > & /*b*/) { }
  virtual void end_box_differences () { }
  virtual void begin_edge_differences () { }
  virtual void detailed_diff (const db::PropertiesRepository & /*pr*/, const std::vector <std::pair <db::Edge, db::properties_id_type> > & /*a*/, const std::vector <std::pair <db::Edge, db::properties_id_type> > & /*b*/) { }
  virtual void end_edge_differences () { }
  virtual void begin_edge_pair_differences () { }
  virtual void detailed_diff (const db::PropertiesRepository & /*pr*/, const std::vector <std::pair <db::EdgePair, db::properties_id_type> > & /*a*/, const std::vector <std::pair <db::EdgePair, db::properties_id_type> > & /*b*/) { }
  virtual void end_edge_pair_differences () { }
  virtual void begin_text_differences () { }
  virtual void detailed_diff (const db::PropertiesRepository & /*pr*/, const std::vector <std::pair <db::Text, db::properties_id_type> > & /*a*/, const std::vector <std::pair <db::Text, db::properties_id_type> > & /*b*/) { }
  virtual void end_text_differences () { }
  virtual void end_layer () { }
  virtual void end_cell () { }
};

/**
 *  @brief Compare two layout objects
 *
 *  Compare layer definitions, cells, instances and shapes and properties. 
 *  Only layers with valid layer and datatype are compared.
 *  Several flags can be specified as a bitwise or combination of the layout_diff::f_xxx constants.
 *  The results are printed to the info channel.
 *
 *  @param a The first input layout
 *  @param b The second input layout
 *  @param flags Flags to use for the comparison
 *  @param tolerance A coordinate tolerance to apply (0: exact match, 1: one DBU tolerance is allowed ...)
 *  @param max_count The maximum number of lines printed to the logger - the compare result will reflect all differences however
 *  @param print_properties If true, property differences are printed as well
 *
 *  If "max_count" is 0, no limitation is imposed. If it is 1, only a warning saying that the log has been abbreviated is printed.
 *  If "max_count" is >1, max_count-1 differences plus one warning about abbreviation is printed.
 *
 *  @return True, if the layouts are identical
 */
bool DB_PUBLIC compare_layouts (const db::Layout &a, const db::Layout &b, unsigned int flags, db::Coord tolerance, size_t max_count = 0, bool print_properties = false);

/**
 *  @brief Compare two layout objects
 *
 *  This is an extended version that allows specification of two top cells to compare.
 *  It will print the results to the info channel.
 *
 *  @param a The first input layout
 *  @param top_a The first top cell's index
 *  @param b The second input layout
 *  @param top_b The second top cell's index
 *  @param flags Flags to use for the comparison
 *  @param tolerance A coordinate tolerance to apply (0: exact match, 1: one DBU tolerance is allowed ...)
 *  @param max_count The maximum number of lines printed to the logger - the compare result will reflect all differences however
 *  @param print_properties If true, property differences are printed as well
 *
 *  @return True, if the layouts are identical
 */
bool DB_PUBLIC compare_layouts (const db::Layout &a, db::cell_index_type top_a, const db::Layout &b, db::cell_index_type top_b, unsigned int flags, db::Coord tolerance, size_t max_count = 0, bool print_properties = false);

/**
 *  @brief Compare two layout objects with a custom receiver for the differences
 *
 *  Compare layer definitions, cells, instances and shapes and properties. 
 *  Cells are identified by name. 
 *  Only layers with valid layer and datatype are compared.
 *  Several flags can be specified as a bitwise or combination of the layout_diff::f_xxx constants.
 *
 *  @param a The first input layout
 *  @param b The second input layout
 *  @param flags Flags to use for the comparison
 *  @param tolerance A coordinate tolerance to apply (0: exact match, 1: one DBU tolerance is allowed ...)
 *
 *  @return True, if the layouts are identical
 */
bool DB_PUBLIC compare_layouts (const db::Layout &a, const db::Layout &b, unsigned int flags, db::Coord tolerance, DifferenceReceiver &r);

/**
 *  @brief Compare two layouts using the specified top cells
 *
 *  This function basically works like the previous one but allows one to specify top cells which
 *  are compared hierarchically.
 */
bool DB_PUBLIC compare_layouts (const db::Layout &a, db::cell_index_type top_a, const db::Layout &b, db::cell_index_type top_b, unsigned int flags, db::Coord tolerance, DifferenceReceiver &r);

}

#endif

