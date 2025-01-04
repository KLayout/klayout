
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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


#ifndef HDR_layStreamImporter
#define HDR_layStreamImporter

#include "layPluginCommon.h"

#include "dbLayout.h"
#include "dbTrans.h"
#include "dbLoadLayoutOptions.h"
#include "tlStream.h"
#include "tlProgress.h"
#include "layStreamImportDialog.h"

#include <iostream>

namespace db
{
  class Manager;
}

namespace lay
{
  class LayoutView;
}

namespace lay
{

/**
 *  @brief The Stream importer object
 */
class LAY_PLUGIN_PUBLIC StreamImporter
{
public:
  /**
   *  @brief Default constructor
   */
  StreamImporter ();

  /**
   *  @brief Read into an existing layout
   *
   *  This method reads the layout specified into the given layout and cell.
   */
  void read (db::Layout &layout, db::cell_index_type cell_index, std::vector <unsigned int> &new_layers);

  /**
   *  @brief Specifies the global transformation
   *
   *  This specifies the global transformation to apply (in micron units).
   */
  void set_global_trans (const db::DCplxTrans &trans)
  {
    m_global_trans = trans;
  }

  /**
   *  @brief Gets the global transformation
   */
  const db::DCplxTrans &global_trans () const
  {
    return m_global_trans;
  }

  /**
   *  @brief Set the reference points
   */
  void set_reference_points (const std::vector<std::pair <db::DPoint, db::DPoint> > &pts)
  {
    m_reference_points = pts;
  }

  /**
   *  @brief Set files which are read.
   */
  void set_files (const std::vector<std::string> &files)
  {
    m_files = files;
  }

  /**
   *  @brief Get the file which is read
   */
  const std::vector<std::string> &files () const
  {
    return m_files;
  }

  /**
   *  @brief Set top cell name which is read.
   *
   *  If an empty top cell name is specified, the top cell is determined automatically (it must be a unique top cell in some
   *  cases).
   */
  void set_topcell (const std::string &topcell)
  {
    m_topcell = topcell;
  }

  /**
   *  @brief Get the top cell name.
   */
  const std::string &topcell () const
  {
    return m_topcell;
  }

  /**
   *  @brief Set cell mapping mode
   */
  void set_cell_mapping (StreamImportData::mode_type cell_mapping)
  {
    m_cell_mapping = cell_mapping;
  }

  /**
   *  @brief Get the cell mapping mode
   */
  StreamImportData::mode_type cell_mapping () const
  {
    return m_cell_mapping;
  }

  /**
   *  @brief Set layer mapping mode
   */
  void set_layer_mapping (StreamImportData::layer_mode_type layer_mapping)
  {
    m_layer_mapping = layer_mapping;
  }

  /**
   *  @brief Get the layer mapping mode
   */
  StreamImportData::layer_mode_type layer_mapping () const
  {
    return m_layer_mapping;
  }

  /**
   *  @brief Set layer offset
   */
  void set_layer_offset (const db::LayerOffset &layer_offset)
  {
    m_layer_offset = layer_offset;
  }

  /**
   *  @brief Get the layer offset
   */
  db::LayerOffset layer_offset () const
  {
    return m_layer_offset;
  }

  /**
   *  @brief Set reader options
   */
  void set_reader_options (const db::LoadLayoutOptions &options)
  {
    m_options = options;
  }

  /**
   *  @brief Get the reader options
   */
  const db::LoadLayoutOptions &reader_options () const
  {
    return m_options;
  }

private:
  std::vector<std::string> m_files;
  std::string m_topcell;
  db::DCplxTrans m_global_trans;
  std::vector <std::pair <db::DPoint, db::DPoint> > m_reference_points;
  StreamImportData::mode_type m_cell_mapping;
  StreamImportData::layer_mode_type m_layer_mapping;
  db::LayerOffset m_layer_offset;
  db::LoadLayoutOptions m_options;
};

}

#endif

