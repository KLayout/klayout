
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



#ifndef HDR_dbGDS2ReaderBase
#define HDR_dbGDS2ReaderBase

#include "dbPluginCommon.h"
#include "dbLayout.h"
#include "dbReader.h"
#include "dbStreamLayers.h"
#include "dbCommonReader.h"

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
  : public CommonReader
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
   *  @brief Accessor method to the current cellname
   */
  const std::string &cellname () const { return m_cellname; }

  virtual void do_read (db::Layout &layout);
  virtual void init (const LoadLayoutOptions &options);

private:
  friend class GDS2ReaderLayerMapping;

  std::string m_cellname;
  std::string m_libname;
  double m_dbu, m_dbuu;
  bool m_read_texts;
  bool m_read_properties;
  bool m_allow_multi_xy_records;
  unsigned int m_box_mode;
  std::map <tl::string, std::vector<std::string> > m_context_info;
  std::vector <db::Point> m_all_points;

  void read_context_info_cell ();
  void read_boundary (db::Layout &layout, db::Cell &cell, bool from_box_record);
  void read_path (db::Layout &layout, db::Cell &cell);
  void read_text (db::Layout &layout, db::Cell &cell);
  void read_box (db::Layout &layout, db::Cell &cell);
  void read_ref (db::Layout &layout, db::Cell &cell, bool array, tl::vector<db::CellInstArray> &instances, tl::vector<db::CellInstArrayWithProperties> &insts_wp);

  std::pair <bool, db::properties_id_type> finish_element (db::PropertiesRepository &rep);
  void finish_element ();

  virtual void common_reader_error (const std::string &msg) { error (msg); }
  virtual void common_reader_warn (const std::string &msg, int warn_level = 1) { warn (msg, warn_level); }

  virtual void error (const std::string &txt) = 0;
  virtual void warn (const std::string &txt, int warn_level = 1) = 0;

  virtual std::string path () const = 0;
  virtual const char *get_string () = 0;
  virtual void get_string (std::string &s) const = 0;
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

