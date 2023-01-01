
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



#ifndef HDR_dbMAGReader
#define HDR_dbMAGReader

#include "dbPluginCommon.h"
#include "dbNamedLayerReader.h"
#include "dbLayout.h"
#include "dbMAG.h"
#include "dbMAGFormat.h"
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

class Technology;

/**
 *  @brief Generic base class of MAG reader exceptions
 */
class DB_PLUGIN_PUBLIC MAGReaderException
  : public ReaderException
{
public:
  MAGReaderException (const std::string &msg, size_t l, const std::string &file)
    : ReaderException (tl::sprintf (tl::to_string (tr ("%s (line=%ld, file=%s)")), msg, l, file))
  { }
};

/**
 *  @brief The MAG format stream reader
 */
class DB_PLUGIN_PUBLIC MAGReader
  : public NamedLayerReader,
    public MAGDiagnostics
{
public: 
  typedef std::vector<tl::Variant> property_value_list;

  /**
   *  @brief Construct a stream reader object
   *
   *  @param s The stream delegate from which to read stream data from
   */
  MAGReader (tl::InputStream &s);

  /**  
   *  @brief Destructor
   */
  ~MAGReader ();

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
   *  which tells which MAG layer has been read into which logical
   *  layer.
   *
   *  @param layout The layout object to write to
   *  @return The LayerMap object
   */
  virtual const LayerMap &read (db::Layout &layout);

  /**
   *  @brief Format
   */
  virtual const char *format () const { return "MAG"; }

  /**
   *  @brief Issue an error with positional information
   *
   *  Reimplements MAGDiagnostics
   */
  virtual void error (const std::string &txt);

  /**
   *  @brief Issue a warning with positional information
   *
   *  Reimplements MAGDiagnostics
   */
  virtual void warn (const std::string &txt, int wl = 1);

private:
  tl::TextInputStream m_stream;
  tl::TextInputStream *mp_current_stream;
  tl::AbsoluteProgress m_progress;
  double m_lambda, m_dbu;
  std::vector<std::string> m_lib_paths;
  bool m_merge;
  std::map<std::string, db::cell_index_type> m_cells_read;
  std::map<std::string, std::pair<std::string, db::cell_index_type> > m_cells_to_read;
  std::map<std::string, std::string> m_use_lib_paths;
  db::VCplxTrans m_dbu_trans_inv;
  std::string m_tech;
  const db::Technology *mp_klayout_tech;

  void do_read (db::Layout &layout, db::cell_index_type to_cell, tl::TextInputStream &stream);
  void do_read_part (db::Layout &layout, db::cell_index_type cell_index, tl::TextInputStream &stream);
  void do_merge_part (db::Layout &layout, db::cell_index_type cell_index);
  bool resolve_path(const std::string &path, const Layout &layout, std::string &real_path);
  std::string cell_name_from_path (const std::string &path);
  db::cell_index_type cell_from_path (const std::string &path, Layout &layout);
  void read_rect (tl::Extractor &ex, Layout &layout, cell_index_type cell_index, unsigned int layer);
  void read_tri (tl::Extractor &ex, Layout &layout, cell_index_type cell_index, unsigned int layer);
  void read_rlabel (tl::Extractor &ex, Layout &layout, cell_index_type cell_index);
  void read_cell_instance (tl::Extractor &ex, tl::TextInputStream &stream, Layout &layout, cell_index_type cell_index);
};

}

#endif

