
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


#ifndef HDR_dbClipboardData
#define HDR_dbClipboardData

#include "dbCommon.h"

#include "dbLayout.h"
#include "dbLayoutUtils.h"

#include <map>
#include <vector>

namespace db
{

/**
 *  @brief A receiver for insert events of the clipboard data object
 */
class DB_PUBLIC ClipboardDataInsertReceiver 
{
public:
  ClipboardDataInsertReceiver () 
  {
    //  .. nothing yet ..
  }

  virtual ~ClipboardDataInsertReceiver () 
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief This method is called when a shape is inserted
   *
   *  @param cell The index of the cell where the shape is inserted
   *  @param layer The layer where the shape is inserted
   *  @param shape The (new) shape that was inserted
   */
  virtual void shape_inserted (db::cell_index_type /*cell*/, int /*layer*/, const db::Shape & /*shape*/) 
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief This method is called when an instance is inserted
   *
   *  @param cell The index of the cell where the shape is inserted
   *  @param shape The (new) instance that was inserted
   */
  virtual void instance_inserted (db::cell_index_type /*cell*/, const db::Instance & /*instance*/) 
  {
    //  .. nothing yet ..
  }
};

/**
 *  @brief A container for the clipboard data 
 *
 *  This is basically a layout object enhanced with some special data to 
 *  represent the data on the clipboard.
 */
class DB_PUBLIC ClipboardData
{
public:
  /**
   *  @brief Default ctor
   */
  ClipboardData ();

  /**
   *  @brief dtor
   */
  ~ClipboardData ();

  /**
   *  @brief Add a shape to the clipboard data
   *
   *  It is assumed that all add operations are made from the same layout object for 
   *  one ClipboardData object.
   */
  void add (const db::Layout &layout, unsigned int layer, const db::Shape &shape);

  /**
   *  @brief Add a transformed shape to the clipboard data, transformed with the given transformation
   *
   *  It is assumed that all add operations are made from the same layout object for 
   *  one ClipboardData object.
   */
  void add (const db::Layout &layout, unsigned int layer, const db::Shape &shape, const db::ICplxTrans &trans);

  /**
   *  @brief Add an instance to the clipboard data
   *
   *  Depending on the mode, not only the instance but the cell which is instantiated 
   *  is added to the clipboard data as well.
   *  It is assumed that all add operations are made from the same layout object for 
   *  one ClipboardData object.
   *
   *  @param mode 0, if to copy just the instance, 1 to copy the cell as well (in deep mode)
   */
  void add (const db::Layout &layout, const db::Instance &inst, unsigned int mode);

  /**
   *  @brief Add an transformed instance to the clipboard data
   *
   *  Depending on the mode, not only the instance but the cell which is instantiated 
   *  is added to the clipboard data as well.
   *  It is assumed that all add operations are made from the same layout object for 
   *  one ClipboardData object.
   *
   *  @param mode 0, if to copy just the instance, 1 to copy the cell as well (in deep mode)
   */
  void add (const db::Layout &layout, const db::Instance &inst, unsigned int mode, const db::ICplxTrans &trans);

  /**
   *  @brief Add a cell to the clipboard data
   *
   *  Depending on the mode, not only the cell but all subcells are added to the clipboard 
   *  data as well. In "toplevel only" mode, just the instances are copied, not the subcells.
   *  It is assumed that all add operations are made from the same layout object for 
   *  one ClipboardData object.
   *
   *  @param mode 0, if to copy just the cell, 1 to copy the subcells as well, 2 to copy the first level of the hierarchy.
   *  @return The index of the created cell
   */
  db::cell_index_type add (const db::Layout &layout, const db::Cell &cell, unsigned int mode);

  /**
   *  @brief Insert the data into the given layout
   *
   *  Cells that are stored in this object are either looked for (if the cell is copied without
   *  content, i.e as target for an instance) or created newly if stored with content.
   *  Layers are mapped where required. 
   *  Layer mapping involves looking up the target layer. The target layer is looked up by layer/datatype
   *  first, then name. If a layer is not found, it will be created newly.
   *
   *  @param into The layout into which to insert the new items
   *  @param cell If non-null, the items will be created in this cell 
   *  @param new_top If non-null, this vector will be filled with the indices of the newly
   *                 created top cells in "into" layout
   *  @param insert_receiver A notification object that receives insert events, i.e. for providing a selection
   *  @return An array containing a vector of newly created layers in the "into" layout
   */
  std::vector<unsigned int> insert (db::Layout &into, db::Cell *cell = 0, std::vector<db::cell_index_type> *new_tops = 0, ClipboardDataInsertReceiver *insert_receiver = 0) const
  {
    return do_insert (into, 0, cell, new_tops, insert_receiver);
  }

  /**
   *  @brief Insert the data into the given layout with a transformation
   *
   *  Cells that are stored in this object are either looked for (if the cell is copied without
   *  content, i.e as target for an instance) or created newly if stored with content.
   *  Layers are mapped where required. 
   *  Layer mapping involves looking up the target layer. The target layer is looked up by layer/datatype
   *  first, then name. If a layer is not found, it will be created newly.
   *
   *  @param into The layout into which to insert the new items
   *  @param trans The transformation to apply
   *  @param cell If non-null, the items will be created in this cell 
   *  @param new_tops If non-null, this vector will be filled with the indices of the newly
   *                  created top cells in "into" layout
   *  @param insert_receiver A notification object that receives insert events, i.e. for providing a selection
   *  @return An array containing a vector of newly created layers in the "into" layout
   */
  std::vector<unsigned int> insert (db::Layout &into, const db::ICplxTrans &trans, db::Cell *cell = 0, std::vector<db::cell_index_type> *new_tops = 0, ClipboardDataInsertReceiver *insert_receiver = 0) const
  {
    return do_insert (into, &trans, cell, new_tops, insert_receiver);
  }

private:
  db::Layout m_layout;  //  this is where we store data into.
  std::set <db::cell_index_type> m_incomplete_cells;
  std::map <db::cell_index_type, std::vector<std::string> > m_context_info;
  std::map <db::cell_index_type, db::cell_index_type> m_cell_index_map;
  db::cell_index_type m_container_cell_index;
  
  PropertyMapper m_prop_id_map;

  db::cell_index_type cell_for_cell (const db::Layout &layout, db::cell_index_type cell_index, bool incomplete);
  std::vector<unsigned int> do_insert (db::Layout &into, const db::ICplxTrans *trans, db::Cell *cell, std::vector<db::cell_index_type> *new_tops, ClipboardDataInsertReceiver *insert_receiver) const;

  ClipboardData (const ClipboardData &);
  ClipboardData &operator= (const ClipboardData &);
};

}

#endif


