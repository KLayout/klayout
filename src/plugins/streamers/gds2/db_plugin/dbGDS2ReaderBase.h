
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



#ifndef HDR_dbGDS2ReaderBase
#define HDR_dbGDS2ReaderBase

#include "dbPluginCommon.h"
#include "dbLayout.h"
#include "dbReader.h"
#include "dbStreamLayers.h"

#include "tlException.h"
#include "tlInternational.h"
#include "tlProgress.h"
#include "tlString.h"
#include "tlStream.h"

namespace db
{

struct GDS2XY
{
  unsigned char x[4];
  unsigned char y[4];
};

/**
 *  @brief The GDS2 format basic stream reader
 */
class DB_PLUGIN_PUBLIC GDS2ReaderBase
  : public ReaderBase
{
public: 
  /**
   *  @brief Constructor
   */
  GDS2ReaderBase ();

  /**  
   *  @brief Destructor
   */
  ~GDS2ReaderBase ();

  /**
   *  @brief Accessor method to the library name
   */
  const std::string &libname () const { return m_libname; }

protected:
  /** 
   *  @brief The basic read method 
   *
   *  This method will read the stream data and translate this to
   *  insert calls into the layout object. This will not do much
   *  on the layout object beside inserting the objects.
   *  It can be given a couple of options specified with the
   *  LoadLayoutOptions object.
   *  The returned map will contain all layers, the passed
   *  ones and the newly created ones.
   *
   *  @param layout The layout object to write to
   *  @param layer_map The layer mapping on input
   *  @param create_other_layer A flag indicating whether to read all other layers
   *  @param enable_text_objects A flag indicating whether to read text objects
   *  @param enable_properties A flag indicating whether to read user properties
   *  @param allow_multi_xy_records If true, tries to check for multiple XY records for BOUNDARY elements
   *  @param box_mode How to treat BOX records (0: ignore, 1: as rectangles, 2: as boundaries, 3: error)
   *  @return The LayerMap object that tells where which layer was loaded
   */
  const LayerMap &basic_read (db::Layout &layout, const LayerMap &layer_map, bool create_other_layers, bool enable_text_objects, bool enable_properties, bool allow_multi_xy_records, unsigned int box_mode);

  /**
   *  @brief Accessor method to the current cellname
   */
  const tl::string &cellname () const { return m_cellname; }

private:
  friend class GDS2ReaderLayerMapping;

  LayerMap m_layer_map;
  tl::string m_cellname;
  std::string m_libname;
  double m_dbu, m_dbuu;
  bool m_create_layers;
  bool m_read_texts;
  bool m_read_properties;
  bool m_allow_multi_xy_records;
  unsigned int m_box_mode;
  std::map <tl::string, std::vector<std::string> > m_context_info;
  std::vector <db::Point> m_all_points;
  std::map <tl::string, tl::string> m_mapped_cellnames;

  void read_context_info_cell ();
  void read_boundary (db::Layout &layout, db::Cell &cell, bool from_box_record);
  void read_path (db::Layout &layout, db::Cell &cell);
  void read_text (db::Layout &layout, db::Cell &cell);
  void read_box (db::Layout &layout, db::Cell &cell);
  void read_ref (db::Layout &layout, db::Cell &cell, bool array, tl::vector<db::CellInstArray> &instances, tl::vector<db::CellInstArrayWithProperties> &insts_wp);
  db::cell_index_type make_cell (db::Layout &layout, const char *cn, bool for_instance);

  void do_read (db::Layout &layout);

  std::pair <bool, unsigned int> open_dl (db::Layout &layout, const LDPair &dl, bool create);
  std::pair <bool, db::properties_id_type> finish_element (db::PropertiesRepository &rep);
  void finish_element ();

  virtual void error (const std::string &txt) = 0;
  virtual void warn (const std::string &txt) = 0;

  virtual const char *get_string () = 0;
  virtual void get_string (tl::string &s) const = 0;
  virtual int get_int () = 0;
  virtual short get_short () = 0;
  virtual unsigned short get_ushort () = 0;
  virtual double get_double () = 0;
  virtual short get_record () = 0;
  virtual void unget_record (short rec_id) = 0;
  virtual void get_time (unsigned int *mod_time, unsigned int *access_time) = 0;
  virtual GDS2XY *get_xy_data (unsigned int &xy_length) = 0;
  virtual void progress_checkpoint () = 0;
};

}

#endif

