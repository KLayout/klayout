
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

#ifndef HDR_dbTestSupport
#define HDR_dbTestSupport

#include "dbCommon.h"
#include "dbTypes.h"
#include "dbBox.h"

#include <string>

namespace tl
{
  class TestBase;
}

namespace db
{

class PropertiesRepository;
class Layout;
class Cell;
class LayerMap;
class Netlist;
class Region;
class EdgePairs;
class Edges;
class Texts;

/**
 *  @brief Specifies the normalization mode for compare_layouts
 */
enum NormalizationMode
{
  NoNormalization = 0,    //  no normalization - take the test subject as it is
  WriteGDS2 = 1,          //  normalize subject by writing to GDS2 and reading back
  WriteOAS = 2,           //  normalize subject by writing to OASIS and reading back
  NormFileMask = 7,       //  bits the extract for file mode
  NoContext = 8,          //  write tmp file without context
  AsPolygons = 16,        //  paths and boxes are treated as polygons
  WithArrays = 32         //  do not flatten arrays
};

/**
 *  @brief Compares a layout with a golden layout file
 *  @param layout The layout to compare
 *  @param au_file The golden file path
 *  @param norm The normalization mode (see NormalizationMode)
 *  @param tolerance A tolerance applied when comparing shapes in database units
 *  The layout is normalized by writing to a file and reading back
 */
void DB_PUBLIC compare_layouts (tl::TestBase *_this, const db::Layout &layout, const std::string &au_file, NormalizationMode norm = WriteGDS2, db::Coord tolerance = 0);

/**
 *  @brief Compares a layout with a golden layout file with layer mapping
 *  @param layout The layout to compare
 *  @param au_file The golden file path
 *  @param lmap The layer mapping object
 *  @param read_all_others If true, all other layers will be read too
 *  @param norm The normalization mode (see NormalizationMode)
 *  @param tolerance A tolerance applied when comparing shapes in database units
 *  The layout is normalized by writing to a file and reading back
 */
void DB_PUBLIC compare_layouts (tl::TestBase *_this, const db::Layout &layout, const std::string &au_file, const db::LayerMap &lmap, bool read_all_others, NormalizationMode norm = WriteGDS2, db::Coord tolerance = 0);

/**
 *  @brief Compares a netlist against a string
 */
void DB_PUBLIC compare_netlist (tl::TestBase *_this, const db::Netlist &netlist, const std::string &au_nl_string, bool exact_parameter_match = false, bool with_names = false);

/**
 *  @brief Compares a netlist against another netlist
 */
void DB_PUBLIC compare_netlist (tl::TestBase *_this, const db::Netlist &netlist, const db::Netlist &netlist_au, bool exact_parameter_match = false, bool with_names = false);

/**
 *  @brief Convenient compare of region vs. string
 */
bool DB_PUBLIC compare (const db::Region &region, const std::string &string);

/**
 *  @brief Convenient compare of edges vs. string
 */
bool DB_PUBLIC compare (const db::Edges &edges, const std::string &string);

/**
 *  @brief Convenient compare of edge pairs vs. string
 */
bool DB_PUBLIC compare (const db::EdgePairs &edge_pairs, const std::string &string);

/**
 *  @brief Convenient compare of texts vs. string
 */
bool DB_PUBLIC compare (const db::Texts &texts, const std::string &string);

/**
 *  @brief Convenient compare of box vs. string
 */
bool DB_PUBLIC compare (const db::Box &box, const std::string &string);

/**
 *  @brief Converts a property ID into a property key/value string representation
 */
std::string DB_PUBLIC prop2string (const db::PropertiesRepository &pr, db::properties_id_type prop_id);

}

#endif
