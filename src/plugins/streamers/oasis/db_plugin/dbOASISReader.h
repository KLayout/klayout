
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



#ifndef HDR_dbOASISReader
#define HDR_dbOASISReader

#include "dbPluginCommon.h"
#include "dbLayout.h"
#include "dbReader.h"
#include "dbTypes.h"
#include "dbOASIS.h"
#include "dbOASISFormat.h"
#include "dbStreamLayers.h"
#include "dbPropertiesRepository.h"

#include "tlException.h"
#include "tlInternational.h"
#include "tlProgress.h"
#include "tlString.h"
#include "tlStream.h"

#include <map>
#include <set>

namespace db
{

/**
 *  @brief Generic base class of OASIS reader exceptions
 */
class DB_PLUGIN_PUBLIC OASISReaderException
  : public ReaderException
{
public:
  OASISReaderException (const std::string &msg, size_t p, const std::string &cell)
    : ReaderException (tl::sprintf (tl::to_string (tr ("%s (position=%ld, cell=%s)")), msg, p, cell))
  { }
};

/**
 *  @brief The OASIS format stream reader
 */
class DB_PLUGIN_PUBLIC OASISReader
  : public ReaderBase, 
    public OASISDiagnostics
{
public: 
  typedef std::vector<tl::Variant> property_value_list;

  /**
   *  @brief Construct a stream reader object
   *
   *  @param s The stream delegate from which to read stream data from
   */
  OASISReader (tl::InputStream &s);

  /**  
   *  @brief Destructor
   */
  ~OASISReader ();

  /** 
   *  @brief The basic read method 
   *
   *  This method will read the stream data and translate this to
   *  insert calls into the layout object. This will not do much
   *  on the layout object beside inserting the objects.
   *  A set of options can be specified with the LoadLayoutOptions
   *  object.
   *  The returned map will contain all layers, the passed
   *  ones and the newly created ones.
   *
   *  @param layout The layout object to write to
   *  @param map The LayerMap object
   *  @param create true, if new layers should be created
   *  @return The LayerMap object that tells where which layer was loaded
   */
  virtual const LayerMap &read (db::Layout &layout, const LoadLayoutOptions &options);

  /** 
   *  @brief The basic read method (without mapping)
   *
   *  This method will read the stream data and translate this to
   *  insert calls into the layout object. This will not do much
   *  on the layout object beside inserting the objects.
   *  This version will read all input layers and return a map
   *  which tells which OASIS layer has been read into which logical
   *  layer.
   *
   *  @param layout The layout object to write to
   *  @return The LayerMap object
   */
  virtual const LayerMap &read (db::Layout &layout);

  /**
   *  @brief Format
   */
  virtual const char *format () const { return "OASIS"; }

  /**
   *  @brief Issue an error with positional information
   *
   *  Reimplements OASISDiagnostics
   */
  virtual void error (const std::string &txt);

  /**
   *  @brief Issue a warning with positional information
   *
   *  Reimplements OASISDiagnostics
   */
  virtual void warn (const std::string &txt);

private:
  friend class OASISReaderLayerMapping;

  typedef db::coord_traits<db::Coord>::distance_type distance_type;

  enum TableMode
  {
    NotInTable,
    InCELLNAME,
    InPROPNAME,
    InPROPSTRING,
    InTEXTSTRING,
    InLAYERNAME
  };

  tl::InputStream &m_stream;
  LayerMap m_layer_map;
  std::set<unsigned int> m_layers_created;
  tl::AbsoluteProgress m_progress;
  std::string m_cellname;
  double m_dbu;
  int m_expect_strict_mode;
  size_t m_first_cellname;
  size_t m_first_propname;
  size_t m_first_propstring;
  size_t m_first_textstring;
  size_t m_first_layername;
  TableMode m_in_table;
  size_t m_table_cellname;
  size_t m_table_propname;
  size_t m_table_propstring;
  size_t m_table_textstring;
  size_t m_table_layername;
  size_t m_table_start;

  modal_variable<Repetition> mm_repetition;
  modal_variable<db::cell_index_type> mm_placement_cell;
  modal_variable<db::Coord> mm_placement_x;
  modal_variable<db::Coord> mm_placement_y;
  modal_variable<unsigned int> mm_layer;
  modal_variable<unsigned int> mm_datatype;
  modal_variable<unsigned int> mm_textlayer;
  modal_variable<unsigned int> mm_texttype;
  modal_variable<db::Coord> mm_text_x;
  modal_variable<db::Coord> mm_text_y;
  modal_variable<std::string> mm_text_string;
  modal_variable<unsigned int> mm_text_string_id;
  modal_variable<db::Coord> mm_geometry_x;
  modal_variable<db::Coord> mm_geometry_y;
  modal_variable<distance_type> mm_geometry_w;
  modal_variable<distance_type> mm_geometry_h;
  modal_variable< std::vector<db::Point> > mm_polygon_point_list;
  modal_variable<distance_type> mm_path_halfwidth;
  modal_variable<db::Coord> mm_path_start_extension;
  modal_variable<db::Coord> mm_path_end_extension;
  modal_variable< std::vector<db::Point> > mm_path_point_list;
  modal_variable<unsigned int> mm_ctrapezoid_type;
  modal_variable<distance_type> mm_circle_radius;
  modal_variable<db::property_names_id_type> mm_last_property_name;
  modal_variable<bool> mm_last_property_is_sprop;
  modal_variable<property_value_list> mm_last_value_list;

  std::map <unsigned long, std::string> m_cellnames;
  std::map <unsigned long, db::properties_id_type> m_cellname_properties;
  std::map <unsigned long, std::string> m_textstrings;
  std::map <unsigned long, const db::StringRef *> m_text_forward_references;
  std::map <unsigned long, std::string> m_propstrings;
  std::map <unsigned long, std::string> m_propnames;
  tl::interval_map <db::ld_type, tl::interval_map <db::ld_type, std::string> > m_layernames;

  tl::vector<db::CellInstArray> m_instances;
  tl::vector<db::CellInstArrayWithProperties> m_instances_with_props;

  std::map <unsigned long, db::cell_index_type> m_cells_by_id;
  std::map <unsigned long, db::cell_index_type> m_forward_references;
  std::map <std::string, db::cell_index_type> m_cells_by_name;
  bool m_create_layers;
  bool m_read_texts;
  bool m_read_properties;
  bool m_read_all_properties;

  std::set <unsigned long> m_defined_cells_by_id;
  std::set <std::string> m_defined_cells_by_name;
  std::map <tl::string, tl::string> m_mapped_cellnames;

  std::map <unsigned long, db::property_names_id_type> m_propname_forward_references;
  std::map <unsigned long, std::string> m_propvalue_forward_references;
  db::property_names_id_type m_s_gds_property_name_id;
  db::property_names_id_type m_klayout_context_property_name_id;

  void do_read (db::Layout &layout);
  void do_read_cell (db::cell_index_type cell_index, db::Layout &layout);

  void do_read_placement (unsigned char r,
                          bool xy_absolute,
                          db::Layout &layout, 
                          tl::vector<db::CellInstArray> &instances,
                          tl::vector<db::CellInstArrayWithProperties> &instances_with_props);

  void do_read_text (bool xy_absolute,db::cell_index_type cell_index, db::Layout &layout);
  void do_read_rectangle (bool xy_absolute,db::cell_index_type cell_index, db::Layout &layout);
  void do_read_polygon (bool xy_absolute,db::cell_index_type cell_index, db::Layout &layout);
  void do_read_path (bool xy_absolute,db::cell_index_type cell_index, db::Layout &layout);
  void do_read_trapezoid (unsigned char r, bool xy_absolute,db::cell_index_type cell_index, db::Layout &layout);
  void do_read_ctrapezoid (bool xy_absolute,db::cell_index_type cell_index, db::Layout &layout);
  void do_read_circle (bool xy_absolute,db::cell_index_type cell_index, db::Layout &layout);
  db::cell_index_type make_cell (db::Layout &layout, const char *cn, bool for_instance);

  void reset_modal_variables ();

  void mark_start_table ();

  void read_offset_table ();
  bool read_repetition ();
  void read_pointlist (modal_variable <std::vector <db::Point> > &pointlist, bool for_polygon);
  void read_properties (db::PropertiesRepository &rep);
  void store_last_properties (db::PropertiesRepository &rep, db::PropertiesRepository::properties_set &properties, bool ignore_special);
  std::pair <bool, db::properties_id_type> read_element_properties (db::PropertiesRepository &rep, bool ignore_special);

  unsigned char get_byte ()
  {
    unsigned char *b = (unsigned char *) m_stream.get (1);
    if (! b) {
      error (tl::to_string (tr ("Unexpected end-of-file")));
      return 0;
    } else {
      return *b;
    }
  }

  long long get_long_long ();
  unsigned long long get_ulong_long ();
  long get_long ();
  unsigned long get_ulong ();
  unsigned long get_ulong_for_divider ();
  int get_int ();
  unsigned int get_uint ();

  void get (long long &l)
  {
    l = get_long_long ();
  }

  void get (unsigned long long &l)
  {
    l = get_ulong_long ();
  }

  void get (long &l)
  {
    l = get_long ();
  }

  void get (unsigned long &l)
  {
    l = get_ulong ();
  }

  void get (int &l)
  {
    l = get_int ();
  }

  void get (unsigned int &l)
  {
    l = get_uint ();
  }

  void get (double &d)
  {
    d = get_real ();
  }

  std::string get_str ();
  void get_str (std::string &s);
  double get_real ();
  db::Vector get_gdelta (long grid = 1);
  db::Vector get_3delta (long grid = 1);
  db::Vector get_2delta (long grid = 1);
  db::Coord get_coord (long grid = 1);
  db::Coord get_ucoord (unsigned long grid = 1);
  distance_type get_ucoord_as_distance (unsigned long grid = 1);

  std::pair <bool, unsigned int> open_dl (db::Layout &layout, const LDPair &dl, bool create);
};

}

#endif

