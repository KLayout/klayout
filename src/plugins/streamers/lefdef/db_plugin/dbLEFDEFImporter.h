
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


#ifndef HDR_dbLEFDEFImporter
#define HDR_dbLEFDEFImporter

#include "dbPluginCommon.h"
#include "dbLayout.h"
#include "dbReader.h"
#include "dbCommonReader.h"
#include "dbStreamLayers.h"
#include "tlStream.h"
#include "tlVariant.h"

#include <vector>
#include <string>
#include <map>

namespace tl
{
  class AbsoluteProgress;
}

namespace db
{

class LEFDEFReaderState;
class LEFDEFImporter;
struct MacroDesc;

/**
 *  @brief Correct a path relative to the stream and technology
 */
DB_PLUGIN_PUBLIC
std::string correct_path (const std::string &fn, const db::Layout &layout, const std::string &base_path);

/**
 *  @brief Convers a string to a MASKSHIFT index list
 */
std::vector<unsigned int> string2masks (const std::string &s);

/**
 *  @brief Generic base class of DXF reader exceptions
 */
class DB_PLUGIN_PUBLIC LEFDEFReaderException
  : public db::ReaderException
{
public:
  LEFDEFReaderException (const std::string &msg, int line, const std::string &cell, const std::string &fn)
    : db::ReaderException (tl::sprintf (tl::to_string (tr ("%s (line=%d, cell=%s, file=%s)")), msg.c_str (), line, cell, fn))
  { }
};

template <class Value>
const Value &per_mask_value (const std::map<unsigned int, Value> &map, const Value &def, unsigned int mask)
{
  typename std::map<unsigned int, Value>::const_iterator i = map.find (mask);
  return i == map.end () ? def : i->second;
}

inline bool per_mask_value_is_null (int dt) { return dt < 0; }
inline bool per_mask_value_is_null (const std::string &pfx) { return pfx.empty (); }

template <class Value>
void set_per_mask_value (std::map<unsigned int, Value> &map, unsigned int mask, const Value &value)
{
  if (per_mask_value_is_null (value)) {
    map.erase (mask);
  } else {
    map [mask] = value;
  }
}

template <class Value>
void get_max_mask_number (unsigned int &mm, const std::map<unsigned int, Value> &map)
{
  if (! map.empty ()) {
    mm = std::max (mm, (--map.end ())->first);
  }
}

/**
 *  @brief The LEF/DEF importer technology component
 *
 *  This component provides technology specific information for the LEF/DEF importer.
 */
class DB_PLUGIN_PUBLIC LEFDEFReaderOptions
  : public db::FormatSpecificReaderOptions
{
public:
  LEFDEFReaderOptions ();
  LEFDEFReaderOptions (const LEFDEFReaderOptions &d);
  LEFDEFReaderOptions &operator= (const LEFDEFReaderOptions &d);

  db::FormatSpecificReaderOptions *clone () const;
  virtual const std::string &format_name () const;

  bool paths_relative_to_cwd () const
  {
    return m_paths_relative_to_cwd;
  }

  void set_paths_relative_to_cwd (bool f)
  {
    m_paths_relative_to_cwd = f;
  }

  bool read_all_layers () const
  {
    return m_read_all_layers;
  }

  void set_read_all_layers (bool a) 
  {
    m_read_all_layers = a;
  }

  const db::LayerMap &layer_map () const
  {
    return m_layer_map;
  }

  db::LayerMap &layer_map ()
  {
    return m_layer_map;
  }

  void set_layer_map (const db::LayerMap &lm)
  {
    m_layer_map = lm;
  }

  double dbu () const
  {
    return m_dbu;
  }

  void set_dbu (double dbu) 
  {
    m_dbu = dbu;
  }

  bool produce_net_names () const
  {
    return m_produce_net_names;
  }

  void set_produce_net_names (bool f) 
  {
    m_produce_net_names = f;
  }

  const tl::Variant &net_property_name () const
  {
    return m_net_property_name;
  }

  void set_net_property_name (const tl::Variant &s) 
  {
    m_net_property_name = s;
  }

  bool produce_inst_names () const
  {
    return m_produce_inst_names;
  }

  void set_produce_inst_names (bool f)
  {
    m_produce_inst_names = f;
  }

  const tl::Variant &inst_property_name () const
  {
    return m_inst_property_name;
  }

  void set_inst_property_name (const tl::Variant &s)
  {
    m_inst_property_name = s;
  }

  bool produce_pin_names () const
  {
    return m_produce_pin_names;
  }

  void set_produce_pin_names (bool f)
  {
    m_produce_pin_names = f;
  }

  const tl::Variant &pin_property_name () const
  {
    return m_pin_property_name;
  }

  void set_pin_property_name (const tl::Variant &s)
  {
    m_pin_property_name = s;
  }

  bool produce_cell_outlines () const
  {
    return m_produce_cell_outlines;
  }

  void set_produce_cell_outlines (bool f) 
  {
    m_produce_cell_outlines = f;
  }

  const std::string &cell_outline_layer () const
  {
    return m_cell_outline_layer;
  }

  void set_cell_outline_layer (const std::string &s) 
  {
    m_cell_outline_layer = s;
  }

  bool produce_placement_blockages () const
  {
    return m_produce_placement_blockages;
  }

  void set_produce_placement_blockages (bool f) 
  {
    m_produce_placement_blockages = f;
  }

  const std::string &placement_blockage_layer () const
  {
    return m_placement_blockage_layer;
  }

  void set_placement_blockage_layer (const std::string &s) 
  {
    m_placement_blockage_layer = s;
  }

  bool produce_regions () const
  {
    return m_produce_regions;
  }

  void set_produce_regions (bool f)
  {
    m_produce_regions = f;
  }

  const std::string &region_layer () const
  {
    return m_region_layer;
  }

  void set_region_layer (const std::string &s)
  {
    m_region_layer = s;
  }

  bool produce_via_geometry () const
  {
    return m_produce_via_geometry;
  }

  void set_produce_via_geometry (bool f) 
  {
    m_produce_via_geometry = f;
  }

  const std::string &via_geometry_suffix () const
  {
    return m_via_geometry_suffix;
  }

  void set_via_geometry_suffix (const std::string &s) 
  {
    m_via_geometry_suffix = s;
  }

  int via_geometry_datatype () const
  {
    return m_via_geometry_datatype;
  }

  void set_via_geometry_datatype (int s) 
  {
    m_via_geometry_datatype = s;
  }

  void set_via_geometry_suffix_str (const std::string &s);
  std::string via_geometry_suffix_str () const;

  void set_via_geometry_datatype_str (const std::string &s);
  std::string via_geometry_datatype_str () const;

  void clear_via_geometry_suffixes_per_mask ()
  {
    m_via_geometry_suffixes.clear ();
  }

  void clear_via_geometry_datatypes_per_mask ()
  {
    m_via_geometry_datatypes.clear ();
  }

  const std::string &via_geometry_suffix_per_mask (unsigned int mask) const
  {
    return per_mask_value (m_via_geometry_suffixes, m_via_geometry_suffix, mask);
  }

  void set_via_geometry_suffix_per_mask (unsigned int mask, const std::string &s)
  {
    set_per_mask_value (m_via_geometry_suffixes, mask, s);
  }

  int via_geometry_datatype_per_mask (unsigned int mask) const
  {
    return per_mask_value (m_via_geometry_datatypes, m_via_geometry_datatype, mask);
  }

  void set_via_geometry_datatype_per_mask (unsigned int mask, int s)
  {
    set_per_mask_value (m_via_geometry_datatypes, mask, s);
  }

  const std::string &via_cellname_prefix () const
  {
    return m_via_cellname_prefix;
  }

  void set_via_cellname_prefix (const std::string &s)
  {
    m_via_cellname_prefix = s;
  }

  bool produce_pins () const
  {
    return m_produce_pins;
  }

  void set_produce_pins (bool f) 
  {
    m_produce_pins = f;
  }

  const std::string &pins_suffix () const
  {
    return m_pins_suffix;
  }

  void set_pins_suffix (const std::string &s) 
  {
    m_pins_suffix = s;
  }

  int pins_datatype () const
  {
    return m_pins_datatype;
  }

  void set_pins_datatype (int s) 
  {
    m_pins_datatype = s;
  }

  void set_pins_suffix_str (const std::string &s);
  std::string pins_suffix_str () const;

  void set_pins_datatype_str (const std::string &s);
  std::string pins_datatype_str () const;

  void clear_pins_suffixes_per_mask ()
  {
    m_pins_suffixes.clear ();
  }

  void clear_pins_datatypes_per_mask ()
  {
    m_pins_datatypes.clear ();
  }

  const std::string &pins_suffix_per_mask (unsigned int mask) const
  {
    return per_mask_value (m_pins_suffixes, m_pins_suffix, mask);
  }

  void set_pins_suffix_per_mask (unsigned int mask, const std::string &s)
  {
    set_per_mask_value (m_pins_suffixes, mask, s);
  }

  int pins_datatype_per_mask (unsigned int mask) const
  {
    return per_mask_value (m_pins_datatypes, m_pins_datatype, mask);
  }

  void set_pins_datatype_per_mask (unsigned int mask, int s)
  {
    set_per_mask_value (m_pins_datatypes, mask, s);
  }

  bool produce_lef_pins () const
  {
    return m_produce_lef_pins;
  }

  void set_produce_lef_pins (bool f)
  {
    m_produce_lef_pins = f;
  }

  const std::string &lef_pins_suffix () const
  {
    return m_lef_pins_suffix;
  }

  void set_lef_pins_suffix (const std::string &s)
  {
    m_lef_pins_suffix = s;
  }

  int lef_pins_datatype () const
  {
    return m_lef_pins_datatype;
  }

  void set_lef_pins_datatype (int s)
  {
    m_lef_pins_datatype = s;
  }

  void set_lef_pins_suffix_str (const std::string &s);
  std::string lef_pins_suffix_str () const;

  void set_lef_pins_datatype_str (const std::string &s);
  std::string lef_pins_datatype_str () const;

  void clear_lef_pins_suffixes_per_mask ()
  {
    m_lef_pins_suffixes.clear ();
  }

  void clear_lef_pins_datatypes_per_mask ()
  {
    m_lef_pins_datatypes.clear ();
  }

  const std::string &lef_pins_suffix_per_mask (unsigned int mask) const
  {
    return per_mask_value (m_lef_pins_suffixes, m_lef_pins_suffix, mask);
  }

  void set_lef_pins_suffix_per_mask (unsigned int mask, const std::string &s)
  {
    set_per_mask_value (m_lef_pins_suffixes, mask, s);
  }

  int lef_pins_datatype_per_mask (unsigned int mask) const
  {
    return per_mask_value (m_lef_pins_datatypes, m_lef_pins_datatype, mask);
  }

  void set_lef_pins_datatype_per_mask (unsigned int mask, int s)
  {
    set_per_mask_value (m_lef_pins_datatypes, mask, s);
  }

  bool produce_fills () const
  {
    return m_produce_fills;
  }

  void set_produce_fills (bool f)
  {
    m_produce_fills = f;
  }

  const std::string &fills_suffix () const
  {
    return m_fills_suffix;
  }

  void set_fills_suffix (const std::string &s)
  {
    m_fills_suffix = s;
  }

  int fills_datatype () const
  {
    return m_fills_datatype;
  }

  void set_fills_datatype (int s)
  {
    m_fills_datatype = s;
  }

  void set_fills_suffix_str (const std::string &s);
  std::string fills_suffix_str () const;

  void set_fills_datatype_str (const std::string &s);
  std::string fills_datatype_str () const;

  void clear_fills_suffixes_per_mask ()
  {
    m_fills_suffixes.clear ();
  }

  void clear_fills_datatypes_per_mask ()
  {
    m_fills_datatypes.clear ();
  }

  const std::string &fills_suffix_per_mask (unsigned int mask) const
  {
    return per_mask_value (m_fills_suffixes, m_fills_suffix, mask);
  }

  void set_fills_suffix_per_mask (unsigned int mask, const std::string &s)
  {
    set_per_mask_value (m_fills_suffixes, mask, s);
  }

  int fills_datatype_per_mask (unsigned int mask) const
  {
    return per_mask_value (m_fills_datatypes, m_fills_datatype, mask);
  }

  void set_fills_datatype_per_mask (unsigned int mask, int s)
  {
    set_per_mask_value (m_fills_datatypes, mask, s);
  }

  bool produce_obstructions () const
  {
    return m_produce_obstructions;
  }

  void set_produce_obstructions (bool f) 
  {
    m_produce_obstructions = f;
  }

  const std::string &obstructions_suffix () const
  {
    return m_obstructions_suffix;
  }

  void set_obstructions_suffix (const std::string &s) 
  {
    m_obstructions_suffix = s;
  }

  int obstructions_datatype () const
  {
    return m_obstructions_datatype;
  }

  void set_obstructions_datatype (int s) 
  {
    m_obstructions_datatype = s;
  }

  bool produce_blockages () const
  {
    return m_produce_blockages;
  }

  void set_produce_blockages (bool f) 
  {
    m_produce_blockages = f;
  }

  const std::string &blockages_suffix () const
  {
    return m_blockages_suffix;
  }

  void set_blockages_suffix (const std::string &s) 
  {
    m_blockages_suffix = s;
  }

  int blockages_datatype () const
  {
    return m_blockages_datatype;
  }

  void set_blockages_datatype (int s) 
  {
    m_blockages_datatype = s;
  }

  bool produce_labels () const
  {
    return m_produce_labels;
  }

  void set_produce_labels (bool f)
  {
    m_produce_labels = f;
  }

  const std::string &labels_suffix () const
  {
    return m_labels_suffix;
  }

  void set_labels_suffix (const std::string &s) 
  {
    m_labels_suffix = s;
  }

  int labels_datatype () const
  {
    return m_labels_datatype;
  }

  void set_labels_datatype (int s) 
  {
    m_labels_datatype = s;
  }

  bool produce_lef_labels () const
  {
    return m_produce_lef_labels;
  }

  void set_produce_lef_labels (bool f)
  {
    m_produce_lef_labels = f;
  }

  const std::string &lef_labels_suffix () const
  {
    return m_lef_labels_suffix;
  }

  void set_lef_labels_suffix (const std::string &s)
  {
    m_lef_labels_suffix = s;
  }

  int lef_labels_datatype () const
  {
    return m_lef_labels_datatype;
  }

  void set_lef_labels_datatype (int s)
  {
    m_lef_labels_datatype = s;
  }

  bool produce_routing () const
  {
    return m_produce_routing;
  }

  void set_produce_routing (bool f) 
  {
    m_produce_routing = f;
  }

  const std::string &routing_suffix () const
  {
    return m_routing_suffix;
  }

  void set_routing_suffix (const std::string &s) 
  {
    m_routing_suffix = s;
  }

  int routing_datatype () const
  {
    return m_routing_datatype;
  }

  void set_routing_datatype (int s) 
  {
    m_routing_datatype = s;
  }

  void set_routing_suffix_str (const std::string &s);
  std::string routing_suffix_str () const;

  void set_routing_datatype_str (const std::string &s);
  std::string routing_datatype_str () const;

  void clear_routing_suffixes_per_mask ()
  {
    m_routing_suffixes.clear ();
  }

  void clear_routing_datatypes_per_mask ()
  {
    m_routing_datatypes.clear ();
  }

  const std::string &routing_suffix_per_mask (unsigned int mask) const
  {
    return per_mask_value (m_routing_suffixes, m_routing_suffix, mask);
  }

  void set_routing_suffix_per_mask (unsigned int mask, const std::string &s)
  {
    set_per_mask_value (m_routing_suffixes, mask, s);
  }

  int routing_datatype_per_mask (unsigned int mask) const
  {
    return per_mask_value (m_routing_datatypes, m_routing_datatype, mask);
  }

  void set_routing_datatype_per_mask (unsigned int mask, int s)
  {
    set_per_mask_value (m_routing_datatypes, mask, s);
  }

  bool produce_special_routing () const
  {
    return m_produce_special_routing;
  }

  void set_produce_special_routing (bool f)
  {
    m_produce_special_routing = f;
  }

  const std::string &special_routing_suffix () const
  {
    return m_special_routing_suffix;
  }

  void set_special_routing_suffix (const std::string &s)
  {
    m_special_routing_suffix = s;
  }

  int special_routing_datatype () const
  {
    return m_special_routing_datatype;
  }

  void set_special_routing_datatype (int s)
  {
    m_special_routing_datatype = s;
  }

  void set_special_routing_suffix_str (const std::string &s);
  std::string special_routing_suffix_str () const;

  void set_special_routing_datatype_str (const std::string &s);
  std::string special_routing_datatype_str () const;

  void clear_special_routing_suffixes_per_mask ()
  {
    m_special_routing_suffixes.clear ();
  }

  void clear_special_routing_datatypes_per_mask ()
  {
    m_special_routing_datatypes.clear ();
  }

  const std::string &special_routing_suffix_per_mask (unsigned int mask) const
  {
    return per_mask_value (m_special_routing_suffixes, m_special_routing_suffix, mask);
  }

  void set_special_routing_suffix_per_mask (unsigned int mask, const std::string &s)
  {
    set_per_mask_value (m_special_routing_suffixes, mask, s);
  }

  int special_routing_datatype_per_mask (unsigned int mask) const
  {
    return per_mask_value (m_special_routing_datatypes, m_special_routing_datatype, mask);
  }

  void set_special_routing_datatype_per_mask (unsigned int mask, int s)
  {
    set_per_mask_value (m_special_routing_datatypes, mask, s);
  }

  unsigned int max_mask_number () const
  {
    unsigned int mm = 0;
    get_max_mask_number (mm, m_via_geometry_suffixes);
    get_max_mask_number (mm, m_via_geometry_datatypes);
    get_max_mask_number (mm, m_pins_suffixes);
    get_max_mask_number (mm, m_pins_datatypes);
    get_max_mask_number (mm, m_lef_pins_suffixes);
    get_max_mask_number (mm, m_lef_pins_datatypes);
    get_max_mask_number (mm, m_fills_suffixes);
    get_max_mask_number (mm, m_fills_datatypes);
    get_max_mask_number (mm, m_routing_suffixes);
    get_max_mask_number (mm, m_routing_datatypes);
    get_max_mask_number (mm, m_special_routing_suffixes);
    get_max_mask_number (mm, m_special_routing_datatypes);
    return mm;
  }

  void clear_lef_files ()
  {
    m_lef_files.clear ();
  }

  void push_lef_file (const std::string &l)
  {
    m_lef_files.push_back (l);
  }

  std::vector<std::string>::const_iterator begin_lef_files () const
  {
    return m_lef_files.begin ();
  }

  std::vector<std::string>::const_iterator end_lef_files () const
  {
    return m_lef_files.end ();
  }

  std::vector<std::string> lef_files () const
  {
    return m_lef_files;
  }

  void set_lef_files (const std::vector<std::string> &lf)
  {
    m_lef_files = lf;
  }

  bool read_lef_with_def () const
  {
    return m_read_lef_with_def;
  }

  void set_read_lef_with_def (bool f)
  {
    m_read_lef_with_def = f;
  }

  bool separate_groups () const
  {
    return m_separate_groups;
  }

  void set_separate_groups (bool f)
  {
    m_separate_groups = f;
  }

  bool joined_paths () const
  {
    return m_joined_paths;
  }

  void set_joined_paths (bool f)
  {
    m_joined_paths = f;
  }

  const std::string &map_file () const
  {
    return m_map_file;
  }

  void set_map_file (const std::string &f)
  {
    m_map_file = f;
  }

  /**
   *  @brief Specify the LEF macro resolution strategy when reading DEF files
   *  Values are:
   *    0: propduce LEF geometry unless a FOREIGN cell is specified (default)
   *    1: produce LEF geometry always and ignore FOREIGN
   *    2: produce a placeholder cell always (even if FOREIGN isn't given)
   */
  unsigned int macro_resolution_mode () const
  {
    return m_macro_resolution_mode;
  }

  void set_macro_resolution_mode (unsigned int m)
  {
    m_macro_resolution_mode = m;
  }

  void set_macro_layouts (const std::vector<db::Layout *> &layouts)
  {
    for (std::vector<db::Layout *>::const_iterator l = layouts.begin (); l != layouts.end (); ++l) {
      m_macro_layouts.push_back (*l);
    }
  }

  std::vector<db::Layout *> macro_layouts () const
  {
    std::vector<db::Layout *> res;
    for (tl::weak_collection<db::Layout>::const_iterator m = m_macro_layouts.begin (); m != m_macro_layouts.end (); ++m) {
      if (m.operator-> ()) {
        res.push_back (const_cast<db::Layout *> (m.operator-> ()));
      }
    }
    return res;
  }

  void clear_macro_layout_files ()
  {
    m_macro_layout_files.clear ();
  }

  void push_macro_layout_file (const std::string &l)
  {
    m_macro_layout_files.push_back (l);
  }

  std::vector<std::string>::const_iterator begin_macro_layout_files () const
  {
    return m_macro_layout_files.begin ();
  }

  std::vector<std::string>::const_iterator end_macro_layout_files () const
  {
    return m_macro_layout_files.end ();
  }

  std::vector<std::string> macro_layout_files () const
  {
    return m_macro_layout_files;
  }

  void set_macro_layout_files (const std::vector<std::string> &lf)
  {
    m_macro_layout_files = lf;
  }

private:
  bool m_read_all_layers;
  db::LayerMap m_layer_map;
  double m_dbu;
  bool m_produce_net_names;
  tl::Variant m_net_property_name;
  bool m_produce_inst_names;
  tl::Variant m_inst_property_name;
  bool m_produce_pin_names;
  tl::Variant m_pin_property_name;
  bool m_produce_cell_outlines;
  std::string m_cell_outline_layer;
  bool m_produce_placement_blockages;
  std::string m_placement_blockage_layer;
  bool m_produce_regions;
  std::string m_region_layer;
  bool m_produce_via_geometry;
  std::string m_via_geometry_suffix;
  int m_via_geometry_datatype;
  std::map<unsigned int, std::string> m_via_geometry_suffixes;
  std::map<unsigned int, int> m_via_geometry_datatypes;
  std::string m_via_cellname_prefix;
  bool m_produce_pins;
  std::string m_pins_suffix;
  int m_pins_datatype;
  std::map<unsigned int, std::string> m_pins_suffixes;
  std::map<unsigned int, int> m_pins_datatypes;
  bool m_produce_lef_pins;
  std::string m_lef_pins_suffix;
  int m_lef_pins_datatype;
  std::map<unsigned int, std::string> m_lef_pins_suffixes;
  std::map<unsigned int, int> m_lef_pins_datatypes;
  bool m_produce_fills;
  std::string m_fills_suffix;
  int m_fills_datatype;
  std::map<unsigned int, std::string> m_fills_suffixes;
  std::map<unsigned int, int> m_fills_datatypes;
  bool m_produce_obstructions;
  std::string m_obstructions_suffix;
  int m_obstructions_datatype;
  bool m_produce_blockages;
  std::string m_blockages_suffix;
  int m_blockages_datatype;
  bool m_produce_labels;
  std::string m_labels_suffix;
  int m_labels_datatype;
  bool m_produce_lef_labels;
  std::string m_lef_labels_suffix;
  int m_lef_labels_datatype;
  bool m_produce_routing;
  std::string m_routing_suffix;
  int m_routing_datatype;
  std::map<unsigned int, std::string> m_routing_suffixes;
  std::map<unsigned int, int> m_routing_datatypes;
  bool m_produce_special_routing;
  std::string m_special_routing_suffix;
  int m_special_routing_datatype;
  std::map<unsigned int, std::string> m_special_routing_suffixes;
  std::map<unsigned int, int> m_special_routing_datatypes;
  bool m_separate_groups;
  bool m_joined_paths;
  std::string m_map_file;
  unsigned int m_macro_resolution_mode;
  bool m_read_lef_with_def;
  std::vector<std::string> m_lef_files;
  tl::weak_collection<db::Layout> m_macro_layouts;
  std::vector<std::string> m_macro_layout_files;
  bool m_paths_relative_to_cwd;
};

/**
 *  @brief Defines the layer purposes provided so far
 */
enum LayerPurpose 
{
  None = 0,
  Routing,            //  from DEF only
  Pins,               //  from DEF
  Fills,              //  from DEF
  FillsOPC,           //  from DEF
  SpecialRouting,     //  from DEF only
  LEFPins,            //  from LEF
  ViaGeometry,        //  from LEF+DEF
  Label,              //  from DEF
  LEFLabel,           //  from LEF
  Obstructions,       //  from LEF only
  Outline,            //  from LEF+DEF
  Blockage,           //  from DEF only
  PlacementBlockage,  //  from DEF only
  Regions,            //  from DEF only
  RegionsNone,        //  from DEF only
  RegionsFence,       //  from DEF only
  RegionsGuide,       //  from DEF only
  All                 //  from DEF only
};

/**
 *  @brief A structure holding the layer details like purpose, mask and via size
 */
struct LayerDetailsKey
{
  LayerDetailsKey ()
    : purpose (Routing), mask (0)
  { }

  LayerDetailsKey (LayerPurpose _purpose, unsigned int _mask = 0, const db::DVector &_via_size = db::DVector ())
    : purpose (_purpose), mask (_mask), via_size (_via_size)
  {
    //  normalize the via size such that x is smaller than y size (issue-1065)
    if (via_size.y () < via_size.x ()) {
      via_size = db::DVector (via_size.y (), via_size.x ());
    }
  }

  bool operator< (const LayerDetailsKey &other) const
  {
    if (purpose != other.purpose) {
      return purpose < other.purpose;
    }
    if (mask != other.mask) {
      return mask < other.mask;
    }
    return via_size.less (other.via_size);
  }

  bool operator== (const LayerDetailsKey &other) const
  {
    if (purpose != other.purpose) {
      return false;
    }
    if (mask != other.mask) {
      return false;
    }
    return via_size.equal (other.via_size);
  }

  LayerPurpose purpose;
  unsigned int mask;
  db::DVector via_size;
};

/**
 *  @brief An interface for resolving the number of masks from a layer name
 */
class DB_PLUGIN_PUBLIC LEFDEFNumberOfMasks
{
public:
  LEFDEFNumberOfMasks () { }
  virtual ~LEFDEFNumberOfMasks () { }

  virtual unsigned int number_of_masks (const std::string &layer_name) const = 0;
};

/**
 *  @brief Provides a via generator base class
 */
class DB_PLUGIN_PUBLIC LEFDEFLayoutGenerator
{
public:
  LEFDEFLayoutGenerator () { }
  virtual ~LEFDEFLayoutGenerator () { }

  virtual void create_cell (LEFDEFReaderState &reader, db::Layout &layout, db::Cell &cell, const std::vector<std::string> *maskshift_layers, const std::vector<unsigned int> &masks, const LEFDEFNumberOfMasks *nm) = 0;
  virtual std::vector<std::string> maskshift_layers () const = 0;
  virtual bool is_fixedmask () const = 0;
};

/**
 *  @brief Provides a via generator implementation for rule-based vias
 */
class DB_PLUGIN_PUBLIC RuleBasedViaGenerator
  : public LEFDEFLayoutGenerator
{
public:
  RuleBasedViaGenerator ();

  virtual void create_cell (LEFDEFReaderState &reader, Layout &layout, db::Cell &cell, const std::vector<std::string> *maskshift_layers, const std::vector<unsigned int> &masks, const LEFDEFNumberOfMasks *nm);

  virtual std::vector<std::string> maskshift_layers () const
  {
    std::vector<std::string> msl;
    msl.push_back (m_bottom_layer);
    msl.push_back (m_cut_layer);
    msl.push_back (m_top_layer);
    return msl;
  }

  virtual bool is_fixedmask () const
  {
    return false;
  }

  void set_cutsize (const db::Vector &cutsize) { m_cutsize = cutsize; }
  void set_cutspacing (const db::Vector &cutspacing) { m_cutspacing = cutspacing; }
  void set_offset (const db::Point &offset) { m_offset = offset; }
  void set_be (const db::Vector &be) { m_be = be; }
  void set_te (const db::Vector &te) { m_te = te; }
  void set_bo (const db::Vector &bo) { m_bo = bo; }
  void set_to (const db::Vector &to) { m_to = to; }
  void set_rows (int rows) { m_rows = rows; }
  void set_columns (int columns) { m_columns = columns; }
  void set_pattern (const std::string &pattern) { m_pattern = pattern; }
  void set_bottom_layer (const std::string &ln) { m_bottom_layer = ln; }
  void set_cut_layer (const std::string &ln) { m_cut_layer = ln; }
  void set_top_layer (const std::string &ln) { m_top_layer = ln; }
  void set_bottom_mask (unsigned int m) { m_bottom_mask = m; }
  void set_cut_mask (unsigned int m) { m_cut_mask = m; }
  void set_top_mask (unsigned int m) { m_top_mask = m; }

private:
  std::string m_bottom_layer, m_cut_layer, m_top_layer;
  unsigned int m_bottom_mask, m_cut_mask, m_top_mask;
  db::Vector m_cutsize, m_cutspacing;
  db::Vector m_be, m_te;
  db::Vector m_bo, m_to;
  db::Point m_offset;
  int m_rows, m_columns;
  std::string m_pattern;
};

/**
 *  @brief Provides a geometry-based via generator implementation
 */
class DB_PLUGIN_PUBLIC GeometryBasedLayoutGenerator
  : public LEFDEFLayoutGenerator
{
public:
  GeometryBasedLayoutGenerator ();

  virtual void create_cell (LEFDEFReaderState &reader, Layout &layout, db::Cell &cell, const std::vector<std::string> *maskshift_layers, const std::vector<unsigned int> &masks, const LEFDEFNumberOfMasks *num_cut_masks);
  virtual std::vector<std::string> maskshift_layers () const { return m_maskshift_layers; }
  virtual bool is_fixedmask () const { return m_fixedmask; }

  void add_polygon (const std::string &ln, LayerPurpose purpose, const db::Polygon &poly, unsigned int mask, properties_id_type prop_id, const DVector &via_size = db::DVector ());
  void add_box (const std::string &ln, LayerPurpose purpose, const db::Box &box, unsigned int mask, properties_id_type prop_id, const DVector &via_size = db::DVector ());
  void add_path (const std::string &ln, LayerPurpose purpose, const db::Path &path, unsigned int mask, properties_id_type prop_id, const DVector &via_size = db::DVector ());
  void add_via (const std::string &vn, const db::Trans &trans, unsigned int bottom_mask, unsigned int cut_mask, unsigned int top_mask);
  void add_text (const std::string &ln, LayerPurpose purpose, const db::Text &text, unsigned int mask, db::properties_id_type prop_id);

  void set_maskshift_layers (const std::vector<std::string> &ln) { m_maskshift_layers = ln; }

  void set_maskshift_layer (unsigned int l, const std::string &s)
  {
    if (m_maskshift_layers.size () <= size_t (l)) {
      m_maskshift_layers.resize (l + 1, std::string ());
    }
    m_maskshift_layers[l] = s;
  }

  void set_fixedmask (bool f)
  {
    m_fixedmask = f;
  }

  void subtract_overlap_from_outline (const std::set<std::string> &overlap_layers);

private:
  struct Via {
    Via () : bottom_mask (0), cut_mask (0), top_mask (0) { }
    std::string name;
    std::string nondefaultrule;
    unsigned int bottom_mask, cut_mask, top_mask;
    db::Trans trans;
  };

  std::map <std::pair<std::string, LayerDetailsKey>, db::Shapes> m_shapes;
  std::list<Via> m_vias;
  std::vector<std::string> m_maskshift_layers;
  bool m_fixedmask;

  unsigned int get_maskshift (const std::string &ln, const std::vector<std::string> *maskshift_layers, const std::vector<unsigned int> &masks);
  unsigned int mask_for (const std::string &ln, unsigned int m, unsigned int mshift, const LEFDEFNumberOfMasks *nm) const;
  unsigned int combine_maskshifts (const std::string &ln, unsigned int mshift1, unsigned int mshift2, const LEFDEFNumberOfMasks *nm) const;
};

/**
 *  @brief Layer handler delegate
 *
 *  This class will handle the creation and management of layers in the LEF/DEF reader context
 */
class DB_PLUGIN_PUBLIC LEFDEFReaderState
  : public db::CommonReaderBase
{
public:
  /**
   *  @brief Constructor
   */
  LEFDEFReaderState (const LEFDEFReaderOptions *tc, db::Layout &layout, const std::string &base_path = std::string ());

  /**
   *  @brief Destructor
   */
  ~LEFDEFReaderState ();

  /**
   *  @brief Attaches to or detaches from an importer
   */
  void attach_reader (LEFDEFImporter *importer)
  {
    mp_importer = importer;
  }

  /**
   *  @brief Reads the given map file
   *
   *  Usually this file is read by the constructor. This method is provided for test purposes.
   *  The filename can be a list of files, separated by "+" or ",". They are loaded together into
   *  the same map like they were concatenated.
   */
  void read_map_file (const std::string &filename, db::Layout &layout, const std::string &base_path);

  /**
   *  @brief Gets the layer map
   */
  const db::LayerMap &layer_map () const
  {
    return m_layer_map;
  }

  /**
   *  @brief Create a new layer or return the index of the given layer
   */
  std::set<unsigned int> open_layer (db::Layout &layout, const std::string &name, LayerPurpose purpose, unsigned int mask, const DVector &via_size = db::DVector ());

  /**
   *  @brief Create a new layer or return the index of the given layer
   */
  template <class Shape>
  std::set<unsigned int> open_layer (db::Layout &layout, const std::string &name, LayerPurpose purpose, unsigned int mask, const Shape &via_shape)
  {
    db::Box via_box = db::box_convert<Shape> () (via_shape);
    return open_layer (layout, name, purpose, mask, db::DVector (via_box.width () * layout.dbu (), via_box.height () * layout.dbu ()));
  }

  /**
   *  @brief Registers a layer (assign a new default layer number)
   */
  void register_layer (const std::string &l);

  /**
   *  @brief Finish, i.e. assign GDS layer numbers to the layers
   */
  void finish (db::Layout &layout);

  /**
   *  @brief Registers a via generator for the via with the given name
   *
   *  The generator is capable of creating a via for a specific mask configuration
   */
  void register_via_cell (const std::string &vn, const std::string &nondefaultrule, LEFDEFLayoutGenerator *generator);

  /**
   *  @brief Gets the via cell for the given via name or 0 if no such via is registered
   */
  db::Cell *via_cell (const std::string &vn, const std::string &nondefaultrule, Layout &layout, unsigned int mask_bottom, unsigned int mask_cut, unsigned int mask_top, const LEFDEFNumberOfMasks *nm);

  /**
   *  @brief Gets the via generator for a given via name or 0 if there is no such generator
   */
  LEFDEFLayoutGenerator *via_generator (const std::string &vn, const std::string &nondefaultrule);

  /**
   *  @brief Registers a macro generator for the macro with the given name
   *
   *  The generator is capable of creating a macro for a specific mask configuration
   */
  void register_macro_cell (const std::string &mn, LEFDEFLayoutGenerator *generator);

  /**
   *  @brief Gets the macro cell for the given macro name or 0 if no such macro is registered
   */
  std::pair<db::Cell *, db::Trans> macro_cell (const std::string &mn, Layout &layout, const std::vector<std::string> &maskshift_layers, const std::vector<unsigned int> &masks, const MacroDesc &macro_desc, const LEFDEFNumberOfMasks *nm);

  /**
   *  @brief Gets the macro generator for a given macro name or 0 if there is no such generator
   */
  LEFDEFLayoutGenerator *macro_generator (const std::string &mn);

  /**
   *  @brief Get the technology component pointer
   */
  const LEFDEFReaderOptions *tech_comp () const
  {
    return mp_tech_comp;
  }

  /**
   *  @brief Gets a map of foreign cells vs. name
   */
  const std::map<std::string, db::cell_index_type> &foreign_cells () const
  {
    return m_foreign_cells;
  }

protected:
  virtual void common_reader_error (const std::string &msg);
  virtual void common_reader_warn (const std::string &msg, int warn_level = 1);

private:
  /**
   *  @brief A key for the via cache
   */
  struct ViaKey
  {
    ViaKey (const std::string &n, const std::string &ndr, unsigned int mb, unsigned int mc, unsigned int mt)
      : name (n), nondefaultrule (ndr), mask_bottom (mb), mask_cut (mc), mask_top (mt)
    { }

    bool operator== (const ViaKey &other) const
    {
      return name == other.name && nondefaultrule == other.nondefaultrule && mask_bottom == other.mask_bottom && mask_cut == other.mask_cut && mask_top == other.mask_top;
    }

    bool operator< (const ViaKey &other) const
    {
      if (name != other.name) {
        return name < other.name;
      }
      if (nondefaultrule != other.nondefaultrule) {
        return nondefaultrule < other.nondefaultrule;
      }
      if (mask_bottom != other.mask_bottom) {
        return mask_bottom < other.mask_bottom;
      }
      if (mask_cut != other.mask_cut) {
        return mask_cut < other.mask_cut;
      }
      if (mask_top != other.mask_top) {
        return mask_top < other.mask_top;
      }
      return false;
    }

    std::string name, nondefaultrule;
    unsigned int mask_bottom, mask_cut, mask_top;
  };

  /**
   *  @brief A key for the via cache
   */
  struct MacroKey
  {
    MacroKey ()
    { }

    MacroKey (const std::string &n, const std::vector<unsigned int> &m)
      : name (n), masks (m)
    { }

    bool operator== (const MacroKey &other) const
    {
      return name == other.name && masks == other.masks;
    }

    bool operator< (const MacroKey &other) const
    {
      if (name != other.name) {
        return name < other.name;
      }
      if (masks != other.masks) {
        return masks < other.masks;
      }
      return false;
    }

    std::string name;
    std::vector<unsigned int> masks;
  };

  //  no copying
  LEFDEFReaderState (const LEFDEFReaderState &);
  LEFDEFReaderState &operator= (const LEFDEFReaderState &);

  LEFDEFImporter *mp_importer;
  std::map <std::pair<std::string, LayerDetailsKey>, std::set<unsigned int> > m_layers;
  db::LayerMap m_layer_map;
  bool m_create_layers;
  bool m_has_explicit_layer_mapping;
  int m_laynum;
  std::map<std::string, int> m_default_number;
  const LEFDEFReaderOptions *mp_tech_comp;
  std::map<ViaKey, db::Cell *> m_via_cells;
  std::map<std::pair<std::string, std::string>, LEFDEFLayoutGenerator *> m_via_generators;
  std::map<MacroKey, std::pair<db::Cell *, db::Trans> > m_macro_cells;
  std::map<std::string, LEFDEFLayoutGenerator *> m_macro_generators;
  std::map<std::string, db::cell_index_type> m_foreign_cells;

  std::set<unsigned int> open_layer_uncached (db::Layout &layout, const std::string &name, LayerPurpose purpose, unsigned int mask);
  db::cell_index_type foreign_cell(Layout &layout, const std::string &name);
  void read_single_map_file (const std::string &path, std::map<std::pair<std::string, LayerDetailsKey>, std::vector<db::LayerProperties> > &layer_map);
};

/**
 *  @brief A structure describing a via
 */
struct DB_PLUGIN_PUBLIC ViaDesc
{
  ViaDesc () { }

  /**
   *  @brief The names of bottom and top metal respectively
   */
  std::string m1, m2;
};

/**
 *  @brief A structure describing a macro
 */
struct DB_PLUGIN_PUBLIC MacroDesc
{
  MacroDesc () { }

  /**
   *  @brief The name of the FOREIGN cell if present
   */
  std::string foreign_name;

  /**
   *  @brief The transformation of the FOREIGN cell
   */
  db::Trans foreign_trans;

  /**
   *  @brief The origin
   */
  db::Point origin;

  /**
   *  @brief The bounding box
   */
  db::Box bbox;
};

/**
 *  @brief The LEF importer object
 */
class DB_PLUGIN_PUBLIC LEFDEFImporter
{
public:
  friend class LEFDEFReaderState;

  /**
   *  @brief Default constructor
   */
  LEFDEFImporter (int warn_level);

  /**
   *  @brief Destructor
   */
  virtual ~LEFDEFImporter ();

  /**
   *  @brief Read into an existing layout
   *
   *  This method reads the layout specified into the given layout
   */
  void read (tl::InputStream &stream, db::Layout &layout, LEFDEFReaderState &state);

protected:
  /**
   *  @brief Actually does the reading
   *
   *  Reimplement that method for the LEF and DEF implementation
   */
  virtual void do_read (db::Layout &layout) = 0;

  /**
   *  @brief Issue an error at the current location
   */
  void error (const std::string &msg);

  /**
   *  @brief Issue a warning at the current location
   */
  void warn (const std::string &msg, int wl = 1);

  /**
   *  @brief Returns true if the reader is at the end of the file
   */
  bool at_end ();

  /**
   *  @brief Test whether the next token matches the given one and consume it in that case
   */
  bool test (const std::string &token);

  /**
   *  @brief Test whether the next token matches the given one, but don't consume it
   */
  bool peek (const std::string &token);

  /**
   *  @brief Test whether the next token matches the given one and raise an error if it does not
   */
  void expect (const std::string &token);

  /**
   *  @brief Test whether the next token matches one of the given ones and raise an error if it does not
   */
  void expect (const std::string &token1, const std::string &token2);

  /**
   *  @brief Test whether the next token matches one of the given ones and raise an error if it does not
   */
  void expect (const std::string &token, const std::string &token2, const std::string &token3);

  /**
   *  @brief Gets the next token
   */
  std::string get ();

  /**
   *  @brief Consumes the current token
   */
  void take ();

  /**
   *  @brief Reads the next token as a double value
   */
  double get_double ();

  /**
   *  @brief Reads the next token as a long value
   */
  long get_long ();

  /**
   *  @brief Gets an orientation code
   *  The orientation code is read employing the LEF/DEF convention ("N" for r0 etc.)
   */
  db::FTrans get_orient (bool optional);

  /**
   *  @brief Reads a point
   *  A point is given by two coordinates, x and y
   */
  db::Point get_point (double scale);

  /**
   *  @brief Reads a vector
   *  A vector is given by two coordinates, x and y
   */
  db::Vector get_vector (double scale);

  /**
   *  @brief Turns a number into a mask number
   */
  unsigned int get_mask (long m);

  /**
   *  @brief Create a new layer or return the index of the given layer
   */
  std::set<unsigned int> open_layer (db::Layout &layout, const std::string &name, LayerPurpose purpose, unsigned int mask, const db::DVector &via_size = db::DVector ())
  {
    return mp_reader_state->open_layer (layout, name, purpose, mask, via_size);
  }

  /**
   *  @brief Registers a layer (assign a new default layer number)
   */
  void register_layer (const std::string &l)
  {
    mp_reader_state->register_layer (l);
  }

  /**
   *  @brief Sets the current cell name
   */
  void set_cellname (const std::string &cn)
  {
    m_cellname = cn;
  }

  /**
   *  @brief Reset the current cell name
   */
  void reset_cellname ()
  {
    m_cellname.clear ();
  }

  /**
   *  @brief Gets a flag indicating whether net names shall be produced as properties
   */
  bool produce_net_props () const
  {
    return m_produce_net_props;
  }

  /**
   *  @brief Gets the property name id of the net name property
   */
  db::property_names_id_type net_prop_name_id () const
  {
    return m_net_prop_name_id;
  }

  /**
   *  @brief Gets a flag indicating whether instance names shall be produced as properties
   */
  bool produce_inst_props () const
  {
    return m_produce_inst_props;
  }

  /**
   *  @brief Gets the property name id of the instance name property
   */
  db::property_names_id_type inst_prop_name_id () const
  {
    return m_inst_prop_name_id;
  }

  /**
   *  @brief Gets a flag indicating whether pin names shall be produced as properties
   */
  bool produce_pin_props () const
  {
    return m_produce_pin_props;
  }

  /**
   *  @brief Gets the property name id of the pin name property
   */
  db::property_names_id_type pin_prop_name_id () const
  {
    return m_pin_prop_name_id;
  }

  /**
   *  @brief Gets the reader options
   */
  const db::LEFDEFReaderOptions &options () const
  {
    return m_options;
  }

  /**
   *  @brief Gets the reader state object
   */
  db::LEFDEFReaderState *reader_state ()
  {
    return mp_reader_state;
  }

private:
  tl::AbsoluteProgress *mp_progress;
  tl::TextInputStream *mp_stream;
  LEFDEFReaderState *mp_reader_state;
  std::string m_cellname;
  std::string m_fn;
  std::string m_last_token;
  bool m_produce_net_props;
  db::property_names_id_type m_net_prop_name_id;
  bool m_produce_inst_props;
  db::property_names_id_type m_inst_prop_name_id;
  bool m_produce_pin_props;
  db::property_names_id_type m_pin_prop_name_id;
  db::LEFDEFReaderOptions m_options;
  int m_warn_level;
  std::vector<std::string> m_sections;

  friend class LEFDEFSection;

  const std::string &next ();
  void enter_section (const std::string &name);
  void leave_section ();
};

class DB_PLUGIN_PUBLIC LEFDEFSection
{
public:
  LEFDEFSection (LEFDEFImporter *importer, const std::string &name)
    : mp_importer (importer)
  {
    mp_importer->enter_section (name);
  }

  ~LEFDEFSection ()
  {
    mp_importer->leave_section ();
  }

private:
  LEFDEFImporter *mp_importer;
};

class DB_PLUGIN_PUBLIC LEFDEFReader
  : public db::ReaderBase
{
public:
  LEFDEFReader (tl::InputStream &s);

  virtual const db::LayerMap &read (db::Layout &layout, const db::LoadLayoutOptions &options);
  virtual const db::LayerMap &read (db::Layout &layout);

  virtual const char *format () const;

  const db::LayerMap &read_lefdef (db::Layout &layout, const db::LoadLayoutOptions &options, bool import_lef);

private:
  tl::InputStream &m_stream;
  db::LayerMap m_layer_map;
};

}

#endif


