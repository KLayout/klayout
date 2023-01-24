
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




#ifndef HDR_dbLayoutUtils
#define HDR_dbLayoutUtils

#include "dbCommon.h"

#include "dbLayout.h"

#include <map>
#include <vector>
#include <limits>

namespace db
{

/**
 *  @brief A implementation of ImportLayerMapping that does a direct layer mapping
 *
 *  This implementation will create new layers if required.
 */
class DB_PUBLIC DirectLayerMapping
  : public ImportLayerMapping 
{
public:
  /**
   *  @brief Destructor
   */
  DirectLayerMapping (db::Layout *target_layout);

  /**
   *  @brief Perform the mapping, i.e. deliver a layer index for a given LayerProperties information
   *
   *  This method can return false in the first member of the returned pair to indicate that no mapping shall
   *  be performed. Otherwise it must return the layer index in the second member.
   */
  virtual std::pair <bool, unsigned int> map_layer (const LayerProperties &lprops);

private:
  std::map <db::LayerProperties, unsigned int, db::LPLogicalLessFunc> m_lmap;
  db::Layout *mp_layout;
  bool m_initialized;
};

/**
 *  @brief A property mapper based on a dynamic property id generation
 *
 *  This class can be used as property mapper for certain "insert" flavors of
 *  the Instance and Shapes class. 
 */
class DB_PUBLIC PropertyMapper
{
public:
  /**
   *  @brief Instantiate a property mapper for mapping of property ids from the source to the target layout
   *
   *  @param source The source layout
   *  @param target The target layout
   */
  PropertyMapper (db::Layout *target, const db::Layout *source);
  
  /**
   *  @brief Instantiate a property mapper for mapping of property ids from the source to the target property repository
   *
   *  @param source The source property repository
   *  @param target The target property repository
   */
  PropertyMapper (db::PropertiesRepository *target, const db::PropertiesRepository *source);

  /**
   *  @brief Instantiate a property mapper for mapping of property ids from the source to the target layout
   *
   *  This version does not specify a certain source or target layout. These must be set with the
   *  set_source or set_target methods.
   */
  PropertyMapper ();

  /**
   *  @brief Specify the source layout
   */
  void set_source (const db::Layout *source);
  
  /**
   *  @brief Specify the source property repository
   */
  void set_source (const db::PropertiesRepository *source);

  /**
   *  @brief Specify the target layout
   */
  void set_target (db::Layout *target);
  
  /**
   *  @brief Specify the target property repository
   */
  void set_target (db::PropertiesRepository *target);

  /**
   *  @brief The actual mapping function
   */
  db::Layout::properties_id_type operator() (db::Layout::properties_id_type source_id);

private:
  db::PropertiesRepository *mp_target;
  const db::PropertiesRepository *mp_source;
  std::map <db::Layout::properties_id_type, db::Layout::properties_id_type> m_prop_id_map;
};

/**
 *  @brief A constant describing "drop cell" mapping
 *
 *  If used as the target cell index, this constant means "drop the cell".
 *  This cell and it's children will be dropped unless the children are used by other cells.
 */
const db::cell_index_type DropCell = std::numeric_limits<db::cell_index_type>::max ();

/**
 *  @brief Merge one layout into another
 *
 *  This method copies a given set of source cells from the source to the target layout.
 *  While doing so, it can apply a transformation. The transformation is applied on the source cell
 *  level while the magnification part of the transformation is applied on all levels of cells below the source 
 *  cell level. This avoids that magnifying instances need to be created.
 *  A cell mapping table can be specified which will tell how cells should be mapped: if a source cell is 
 *  found in the cell mapping table, the respective cell is used as the target cell. If a cell is not listed
 *  in the cell mapping, a new cell is created. If non-null, final_cell_mapping will hold of list of target layout cells
 *  vs. source layout cells.
 *  Instances are only copied for cells which are created new. 
 *  The layer mapping table identifies target layers for source layout layers.
 */
void DB_PUBLIC 
merge_layouts (db::Layout &target, const db::Layout &source, const db::ICplxTrans &trans,
               const std::vector<db::cell_index_type> &source_cells, 
               const std::map<db::cell_index_type, db::cell_index_type> &cell_mapping,
               const std::map<unsigned int, unsigned int> &layer_mapping,
               std::map<db::cell_index_type, db::cell_index_type> *final_cell_mapping = 0);

/**
 *  @brief An interface for the shape inserter
 *
 *  This interface is used by copy_shapes and move_shapes to insert
 *  a shape collection into a another one. By reimplementing this interface,
 *  more shape transformations can be provided.
 */
class DB_PUBLIC ShapesTransformer
{
public:
  ShapesTransformer () { }
  virtual ~ShapesTransformer () { }
  virtual void insert_transformed (db::Shapes &into, const db::Shapes &from, const db::ICplxTrans &trans, db::PropertyMapper &pm) const = 0;
};

/**
 *  @brief Copy shapes from one layout to another
 *
 *  This method copies shapes hierarchically from one layout to another.
 *  A cell mapping can be specified - if no target cell is found for a specific source cell, 
 *  the shapes will be propagated to the next parent.
 *  A transformation can be specified which is applied to the shapes and which can be used
 *  for example to compensate different database units of the layouts.
 */
void DB_PUBLIC
copy_shapes (db::Layout &target, 
             const db::Layout &source, 
             const db::ICplxTrans &trans,
             const std::vector<db::cell_index_type> &source_cells, 
             const std::map<db::cell_index_type, db::cell_index_type> &cell_mapping,
             const std::map<unsigned int, unsigned int> &layer_mapping,
             const ShapesTransformer *transformer = 0);

/**
 *  @brief Move shapes from one layout to another
 *
 *  This method moves shapes hierarchically from one layout to another.
 *  A cell mapping can be specified - if no target cell is found for a specific source cell, 
 *  the shapes will be propagated to the next parent.
 *  A transformation can be specified which is applied to the shapes and which can be used
 *  for example to compensate different database units of the layouts.
 */
void DB_PUBLIC
move_shapes (db::Layout &target, 
             db::Layout &source, 
             const db::ICplxTrans &trans,
             const std::vector<db::cell_index_type> &source_cells, 
             const std::map<db::cell_index_type, db::cell_index_type> &cell_mapping,
             const std::map<unsigned int, unsigned int> &layer_mapping,
             const ShapesTransformer *transformer = 0);

/**
 *  @brief Find an example cell instance from a child to a top cell
 *
 *  Returns true in the first member of the returned cell if such a path exists
 *  and one example instantiation in the second member.
 */
std::pair<bool, db::ICplxTrans>
DB_PUBLIC find_layout_context (const db::Layout &layout, db::cell_index_type from, db::cell_index_type to);

/**
 *  @brief A cache for contexts
 *
 *  This object is supposed to minimize the effort to compute many contexts.
 */
class DB_PUBLIC ContextCache
{
public:
  /**
   *  @brief Creates an object of ContextCache associated with the given layout
   */
  ContextCache (const db::Layout *layout);

  /**
   *  @brief Find the context for the given cell combination
   */
  const std::pair<bool, db::ICplxTrans> &find_layout_context (db::cell_index_type from, db::cell_index_type to);

private:
  std::map<std::pair<db::cell_index_type, db::cell_index_type>, std::pair<bool, db::ICplxTrans> > m_cache;
  const db::Layout *mp_layout;
};

/**
 *  @brief Scales and snaps the layout below the given cell
 *
 *  This method will scale and snap all layers from the given cell and below to the
 *  specified grid. Scaling happens by the rational factor m / d.
 */
DB_PUBLIC void scale_and_snap (db::Layout &layout, db::Cell &cell, db::Coord g, db::Coord m, db::Coord d);

}  // namespace db

#endif

