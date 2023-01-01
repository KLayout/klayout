
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



#ifndef HDR_dbCommonReader
#define HDR_dbCommonReader

#include "dbReader.h"


namespace db
{

/**
 *  @brief The CellConflictResolution enum
 */
enum CellConflictResolution
{
  AddToCell = 0,
  OverwriteCell = 1,
  SkipNewCell = 2,
  RenameCell = 3
};

/**
 *  @brief Structure that holds the GDS2 and OASIS specific options for the reader
 */
class DB_PUBLIC CommonReaderOptions
  : public FormatSpecificReaderOptions
{
public:
  /**
   *  @brief The constructor
   */
  CommonReaderOptions ()
    : create_other_layers (true),
      enable_text_objects (true),
      enable_properties (true),
      cell_conflict_resolution (CellConflictResolution::AddToCell)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Specifies a layer mapping
   *
   *  If a layer mapping is specified, only the given layers are read.
   *  Otherwise, all layers are read.
   *  Setting "create_other_layers" to true will make the reader
   *  create other layers for all layers not given in the layer map.
   *  Setting an empty layer map and create_other_layers to true effectively
   *  enables all layers for reading.
   */
  db::LayerMap layer_map;

  /**
   *  @brief A flag indicating that a new layers shall be created
   *
   *  If this flag is set to true, layers not listed in the layer map a created
   *  too.
   */
  bool create_other_layers;

  /**
   *  @brief A flag indicating whether to read text objects
   *
   *  If this flag is set to true, text objects are read. Otherwise they are ignored.
   */
  bool enable_text_objects;

  /**
   *  @brief A flag indicating whether to read user properties
   *
   *  If this flag is set to true, user properties are read. Otherwise they are ignored.
   */
  bool enable_properties;

  /**
   *  @brief Specifies the cell merge behavior
   *
   *  This enum controls how cells are read if a cell with the requested name already
   *  exists.
   *
   *  AddToCell       In this mode, instances or shapes are added to any existing cell
   *  OverwriteCell   Overwrite existing cell. If the existing cell has children, those are removed unless used otherwise
   *  SkipNewCell     Ignore the new cell and it's children
   *  RenameCell      Rename the new cell
   *
   *  If the existing opr the new cell is a ghost cell, AddToCell is applied always. In other words,
   *  ghost cells are always merged.
   */
  CellConflictResolution cell_conflict_resolution;

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual FormatSpecificReaderOptions *clone () const
  {
    return new CommonReaderOptions (*this);
  }

  /**
   *  @brief Implementation of FormatSpecificReaderOptions
   */
  virtual const std::string &format_name () const
  {
    static const std::string n ("Common");
    return n;
  }
};

/**
 *  @brief A common reader base for GDS2 and OASIS providing common services for both readers
 */
class DB_PUBLIC CommonReaderBase
{
public:
  typedef tl::interval_map <db::ld_type, tl::interval_map <db::ld_type, std::string> > layer_name_map;

  /**
   *  @brief Constructor
   */
  CommonReaderBase ();

  /**
   *  @brief Make a cell from a name
   */
  db::cell_index_type make_cell (db::Layout &layout, const std::string &cn);

  /**
   *  @brief Returns true, if there is a cell with the given name already
   */
  bool has_cell (const std::string &cn) const;

  /**
   *  @brief Returns a pair with a bool (indicating whether the cell name is known) and the cell index for this name
   */
  std::pair<bool, db::cell_index_type> cell_by_name (const std::string &name) const;

  /**
   *  @brief Make a cell from an ID (OASIS)
   */
  db::cell_index_type make_cell (db::Layout &layout, size_t id);

  /**
   *  @brief Returns true, if there is a cell with the given ID already
   */
  bool has_cell (size_t id) const;

  /**
   *  @brief Returns a pair with a bool (indicating whether the cell ID is known) and the cell index for this ID
   */
  std::pair<bool, db::cell_index_type> cell_by_id (size_t id) const;

  /**
   *  @brief Registers a cell name for an ID
   */
  void rename_cell (db::Layout &layout, size_t id, const std::string &cn);

  /**
   *  @brief Gets the name for a given cell ID if known, otherwise returns an empty string
   */
  const std::string &name_for_id (size_t id) const;

  /**
   *  @brief Returns a cell reference by ID
   *  If the cell does not exist, it's created. It is marked as ghost cell until
   *  "make_cell" is called.
   */
  db::cell_index_type cell_for_instance (db::Layout &layout, size_t id);

  /**
   *  @brief Returns a cell reference by name
   *  Same as the previous method, but acting on cell names.
   */
  db::cell_index_type cell_for_instance (db::Layout &layout, const std::string &cn);

  /**
   *  @brief Finishes the reading process
   *
   *  This method will first check if all cells IDs got a name.
   *  After this, the cells are renamed and cell conflict resolution will happen in the
   *  specified way (cell_conflict_resolution attribute).
   */
  void finish (db::Layout &layout);

  /**
   *  @brief Re-initialize: clears the tables and caches
   */
  void init ();

  /**
   *  @brief Sets a value indicating whether to create layers
   */
  void set_create_layers (bool f)
  {
    m_create_layers = f;
  }

  /**
   *  @brief Sets the conflict resolution mode
   */
  void set_conflict_resolution_mode (CellConflictResolution cc_resolution)
  {
    m_cc_resolution = cc_resolution;
  }

  /**
   *  @brief Sets the input layer map
   */
  void set_layer_map (const db::LayerMap &lm)
  {
    m_layer_map = lm;
  }

protected:
  friend class CommonReaderLayerMapping;

  virtual void common_reader_error (const std::string &msg) = 0;
  virtual void common_reader_warn (const std::string &msg, int warn_level = 1) = 0;

  /**
   * @brief Merge (and delete) the src_cell into target_cell
   */
  void merge_cell (db::Layout &layout, db::cell_index_type target_cell_index, db::cell_index_type src_cell_index) const;

  /**
   * @brief Merge (and delete) the src_cell into target_cell without instances
   */
  void merge_cell_without_instances (db::Layout &layout, db::cell_index_type target_cell_index, db::cell_index_type src_cell_index) const;

  /**
   *  @brief Gets the layer name map
   */
  layer_name_map &layer_names ()
  {
    return m_layer_names;
  }

  /**
   *  @brief Gets the input layer name
   */
  db::LayerMap &layer_map ()
  {
    return m_layer_map;
  }

  /**
   *  @brief Gets the output layer name map
   */
  const db::LayerMap &layer_map_out () const
  {
    return m_layer_map_out;
  }

  /**
   *  @brief Enters the layer with a given layer/datatype
   */
  std::pair <bool, unsigned int> open_dl (db::Layout &layout, const LDPair &dl);

private:
  std::map<size_t, std::pair<std::string, db::cell_index_type> > m_id_map;
  std::map<std::string, std::pair<size_t, db::cell_index_type> > m_name_map;
  std::set<db::cell_index_type> m_temp_cells;
  std::map<size_t, std::string> m_name_for_id;
  CellConflictResolution m_cc_resolution;
  bool m_create_layers;
  db::LayerMap m_layer_map;
  db::LayerMap m_layer_map_out;
  tl::interval_map <db::ld_type, tl::interval_map <db::ld_type, std::string> > m_layer_names;
  std::map<db::LDPair, std::pair <bool, unsigned int> > m_layer_cache;
  std::map<std::set<unsigned int>, unsigned int> m_multi_mapping_placeholders;
  std::set<unsigned int> m_layers_created;

  std::pair <bool, unsigned int> open_dl_uncached (db::Layout &layout, const LDPair &dl);
};



/**
 *  @brief A common reader base for GDS2 and OASIS providing common services for both readers
 */
class DB_PUBLIC CommonReader
  : public ReaderBase, public CommonReaderBase
{
public:
  typedef tl::interval_map <db::ld_type, tl::interval_map <db::ld_type, std::string> > layer_name_map;

  /**
   *  @brief Constructor
   */
  CommonReader ();

  //  Reimplementation of the ReaderBase interace
  virtual const db::LayerMap &read (db::Layout &layout, const db::LoadLayoutOptions &options);
  virtual const db::LayerMap &read (db::Layout &layout);

protected:
  friend class CommonReaderLayerMapping;

  virtual void init (const LoadLayoutOptions &options);
  virtual void do_read (db::Layout &layout) = 0;
};

/**
 *  @brief A utility class that maps the layers for the proxy cell recovery
 */
class CommonReaderLayerMapping
  : public db::ImportLayerMapping
{
public:
  CommonReaderLayerMapping (db::CommonReader *reader, db::Layout *layout)
    : mp_reader (reader), mp_layout (layout)
  {
    //  .. nothing yet ..
  }

  std::pair<bool, unsigned int> map_layer (const db::LayerProperties &lprops)
  {
    //  named layers that are imported from a library are ignored
    if (lprops.is_named ()) {
      return std::make_pair (false, 0);
    } else {
      return mp_reader->open_dl (*mp_layout, LDPair (lprops.layer, lprops.datatype));
    }
  }

private:
  db::CommonReader *mp_reader;
  db::Layout *mp_layout;
};

}

#endif

