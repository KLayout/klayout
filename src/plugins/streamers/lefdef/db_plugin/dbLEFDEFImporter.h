
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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

  db::FormatSpecificReaderOptions *clone () const;
  virtual const std::string &format_name () const;

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
  bool m_produce_pins;
  std::string m_pins_suffix;
  int m_pins_datatype;
  bool m_produce_obstructions;
  std::string m_obstructions_suffix;
  int m_obstructions_datatype;
  bool m_produce_blockages;
  std::string m_blockages_suffix;
  int m_blockages_datatype;
  bool m_produce_labels;
  std::string m_labels_suffix;
  int m_labels_datatype;
  bool m_produce_routing;
  std::string m_routing_suffix;
  int m_routing_datatype;
  std::vector<std::string> m_lef_files;
};

/**
 *  @brief Defines the layer purposes provided so far
 */
enum LayerPurpose 
{
  Routing = 0,
  ViaGeometry = 1,
  Label = 2,
  Pins = 3,
  Obstructions = 4,
  Outline = 5,
  Blockage = 6,
  PlacementBlockage = 7,
  Region = 8
};

/**
 *  @brief Layer handler delegate
 *
 *  This class will handle the creation and management of layers in the LEF/DEF reader context
 */
class DB_PLUGIN_PUBLIC LEFDEFLayerDelegate
{
public:
  /**
   *  @brief Constructor
   */
  LEFDEFLayerDelegate (const LEFDEFReaderOptions *tc);

  /**
   *  @brief Set the layer map
   */
  virtual void set_layer_map (const db::LayerMap &lm, bool create_layers)
  {
    m_layer_map = lm;
    m_create_layers = create_layers;
  }

  /**
   *  @brief Get the layer map
   */
  const db::LayerMap &layer_map () const
  {
    return m_layer_map;
  }

  /**
   *  @brief Get the layer map (non-const version)
   */
  db::LayerMap &layer_map ()
  {
    return m_layer_map;
  }

  /**
   *  @brief Create a new layer or return the index of the given layer
   */
  std::pair <bool, unsigned int> open_layer (db::Layout &layout, const std::string &name, LayerPurpose purpose);

  /**
   *  @brief Registers a layer (assign a new default layer number)
   */
  void register_layer (const std::string &l);

  /**
   *  @brief Prepare, i.e. create layers required by the layer map
   */
  void prepare (db::Layout &layout);

  /**
   *  @brief Finish, i.e. assign GDS layer numbers to the layers
   */
  void finish (db::Layout &layout);

  /**
   *  @brief Get the technology component pointer
   */
  const LEFDEFReaderOptions *tech_comp () const
  {
    return mp_tech_comp;
  }

private:
  std::map <std::pair<std::string, LayerPurpose>, unsigned int> m_layers;
  db::LayerMap m_layer_map;
  bool m_create_layers;
  int m_laynum;
  std::map<std::string, int> m_default_number;
  const LEFDEFReaderOptions *mp_tech_comp;
};

/**
 *  @brief A structure describing a via
 */
struct DB_PLUGIN_PUBLIC ViaDesc
{
  ViaDesc () : cell (0) { }

  /**
   *  @brief The cell representing the via
   */
  db::Cell *cell;

  /**
   *  @brief The names of bottom and top metal respectively
   */
  std::string m1, m2;
};

/**
 *  @brief The LEF importer object
 */
class DB_PLUGIN_PUBLIC LEFDEFImporter
{
public:
  /**
   *  @brief Default constructor
   */
  LEFDEFImporter ();

  /**
   *  @brief Destructor
   */
  virtual ~LEFDEFImporter ();

  /**
   *  @brief Read into an existing layout
   *
   *  This method reads the layout specified into the given layout
   */
  void read (tl::InputStream &stream, db::Layout &layout, LEFDEFLayerDelegate &layer_delegate);

protected:
  /**
   *  @brief Actually does the readong
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
  void warn (const std::string &msg);

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
   *  @brief Create a new layer or return the index of the given layer
   */
  std::pair <bool, unsigned int> open_layer (db::Layout &layout, const std::string &name, LayerPurpose purpose)
  {
    return mp_layer_delegate->open_layer (layout, name, purpose);
  }

  /**
   *  @brief Registers a layer (assign a new default layer number)
   */
  void register_layer (const std::string &l)
  {
    mp_layer_delegate->register_layer (l);
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
   *  @brief Gets a flag indicating whether pinance names shall be produced as properties
   */
  bool produce_pin_props () const
  {
    return m_produce_pin_props;
  }

  /**
   *  @brief Gets the property name id of the pinance name property
   */
  db::property_names_id_type pin_prop_name_id () const
  {
    return m_pin_prop_name_id;
  }

protected:
  void create_generated_via (std::vector<db::Polygon> &bottom,
                             std::vector<db::Polygon> &cut,
                             std::vector<db::Polygon> &top,
                             const db::Vector &cutsize,
                             const db::Vector &cutspacing,
                             const db::Vector &be, const db::Vector &te,
                             const db::Vector &bo, const db::Vector &to,
                             const db::Point &o,
                             int rows, int columns,
                             const std::string &pattern);

private:
  tl::AbsoluteProgress *mp_progress;
  tl::TextInputStream *mp_stream;
  LEFDEFLayerDelegate *mp_layer_delegate;
  std::string m_cellname;
  std::string m_fn;
  std::string m_last_token;
  bool m_produce_net_props;
  db::property_names_id_type m_net_prop_name_id;
  bool m_produce_inst_props;
  db::property_names_id_type m_inst_prop_name_id;
  bool m_produce_pin_props;
  db::property_names_id_type m_pin_prop_name_id;

  const std::string &next ();
};

}

#endif


