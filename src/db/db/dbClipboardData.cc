
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


#include "dbClipboardData.h"

namespace db
{

ClipboardData::ClipboardData ()
  : m_layout (), m_incomplete_cells ()
{
  m_prop_id_map.set_target (&m_layout);
  m_container_cell_index = m_layout.add_cell ("");
}

ClipboardData::~ClipboardData ()
{
  // .. nothing yet ..
}

void 
ClipboardData::add (const db::Layout &layout, unsigned int layer, const db::Shape &shape)
{
  //  create the layer in our temporary layout if we need to
  //  HINT: this requires all add operations are done from the same layout object
  if (! m_layout.is_valid_layer (layer)) {
    m_layout.insert_layer (layer, layout.get_properties (layer));
  }

  m_prop_id_map.set_source (&layout);
  m_layout.cell (m_container_cell_index).shapes (layer).insert (shape, m_prop_id_map);
}

void 
ClipboardData::add (const db::Layout &layout, unsigned int layer, const db::Shape &shape, const db::ICplxTrans &trans)
{
  //  create the layer in our temporary layout if we need to
  //  HINT: this requires all add operations are done from the same layout object
  if (! m_layout.is_valid_layer (layer)) {
    m_layout.insert_layer (layer, layout.get_properties (layer));
  }

  m_prop_id_map.set_source (&layout);
  db::Shape new_shape = m_layout.cell (m_container_cell_index).shapes (layer).insert (shape, m_prop_id_map);
  m_layout.cell (m_container_cell_index).shapes (layer).transform (new_shape, trans);
}

void 
ClipboardData::add (const db::Layout &layout, const db::Instance &inst, unsigned int mode)
{
  db::cell_index_type target_cell_index;
  db::cell_index_type source_cell_index = inst.cell_index ();

  //  in mode 1 (deep), first add the target cell 
  if (mode == 1 && ! layout.cell (source_cell_index).is_proxy ()) {
    const db::Cell &target_cell = layout.cell (source_cell_index);
    target_cell_index = add (layout, target_cell, 1);
  } else {
    target_cell_index = cell_for_cell (layout, source_cell_index, true);
  }

  //  Insert the instance mapping the cell to the target cell_index and the property ID using the map
  m_prop_id_map.set_source (&layout);
  tl::const_map<db::cell_index_type> im (target_cell_index);
  m_layout.cell (m_container_cell_index).insert (inst, im, m_prop_id_map);
}

void 
ClipboardData::add (const db::Layout &layout, const db::Instance &inst, unsigned int mode, const db::ICplxTrans &trans)
{
  db::cell_index_type target_cell_index;
  db::cell_index_type source_cell_index = inst.cell_index ();

  //  in mode 1 (deep) first add the target cell 
  //  (don't use deep mode for proxy cells because we use the context information to restore 
  //  a proxy cell)
  if (mode == 1 && ! layout.cell (source_cell_index).is_proxy ()) {
    const db::Cell &target_cell = layout.cell (source_cell_index);
    target_cell_index = add (layout, target_cell, 1);
  } else {
    target_cell_index = cell_for_cell (layout, source_cell_index, true);
  }

  //  Insert the instance mapping the cell to the target cell_index and the property ID using the map
  m_prop_id_map.set_source (&layout);
  tl::const_map<db::cell_index_type> im (target_cell_index);
  db::Instance new_inst = m_layout.cell (m_container_cell_index).insert (inst, im, m_prop_id_map);
  m_layout.cell (m_container_cell_index).transform (new_inst, trans);
}

db::cell_index_type 
ClipboardData::add (const db::Layout &layout, const db::Cell &cell, unsigned int mode)
{
  //  if the cell already exists and is stored in the right mode, do nothing
  std::map <db::cell_index_type, db::cell_index_type>::const_iterator cm = m_cell_index_map.find (cell.cell_index ());
  if (cm != m_cell_index_map.end () && ! (m_incomplete_cells.find (cm->second) != m_incomplete_cells.end () && mode >= 1)) {
    return cm->second;
  }

  db::cell_index_type target_cell_index = cell_for_cell (layout, cell.cell_index (), mode == 0);
  if (mode >= 1) {
    m_incomplete_cells.erase (target_cell_index);
    m_context_info.erase (target_cell_index);
  } 

  m_prop_id_map.set_source (&layout);

  //  copy the shapes
  for (unsigned int l = 0; l < layout.layers (); ++l) {

    if (layout.is_valid_layer (l)) {

      if (! m_layout.is_valid_layer (l)) {
        m_layout.insert_layer (l, layout.get_properties (l));
      }

      db::Shapes &shapes = m_layout.cell (target_cell_index).shapes (l);
      for (db::Shapes::shape_iterator sh = cell.shapes (l).begin (db::Shapes::shape_iterator::All); ! sh.at_end (); ++sh) {
        shapes.insert (*sh, m_prop_id_map);
      }

    }

  }

  //  copy the instances
  std::swap (m_container_cell_index, target_cell_index);
  for (db::Instances::const_iterator inst = cell.begin (); ! inst.at_end (); ++inst) {
    add (layout, *inst, mode == 2 ? 0 /*continue with "incomplete" on the next level*/ : 1);
  }
  std::swap (m_container_cell_index, target_cell_index);

  return target_cell_index;
}

std::vector<unsigned int>
ClipboardData::do_insert (db::Layout &layout, const db::ICplxTrans *trans, db::Cell *cell, std::vector<db::cell_index_type> *new_tops, ClipboardDataInsertReceiver *insert_receiver) const
{
  std::vector <unsigned int> new_layers;
  PropertyMapper prop_id_map (&layout, &m_layout);

  std::map <db::LayerProperties, unsigned int, db::LPLogicalLessFunc> layer_map;
  for (unsigned int l = 0; l < layout.layers (); ++l) {
    if (layout.is_valid_layer (l)) {
      layer_map.insert (std::make_pair (layout.get_properties (l), l));
    }
  }

  //  create the necessary target cells

  std::map <db::cell_index_type, db::cell_index_type> cell_map;
  if (cell) {
    cell_map.insert (std::make_pair (m_container_cell_index, cell->cell_index ()));
  }
  
  for (db::Layout::const_iterator c = m_layout.begin (); c != m_layout.end (); ++c) {

    if (c->cell_index () != m_container_cell_index) {

      std::map <db::cell_index_type, std::vector<std::string> >::const_iterator ctx = m_context_info.find (c->cell_index ());
      if (ctx != m_context_info.end ()) {

        //  remember current layers
        std::set <unsigned int> layers;
        for (db::LayerIterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
          layers.insert ((*l).first);
        }

        //  restore cell from context info
        db::Cell *pc = layout.recover_proxy (ctx->second.begin (), ctx->second.end ());

        //  detect new layers 
        for (db::LayerIterator l = layout.begin_layers (); l != layout.end_layers (); ++l) {
          if (layers.find ((*l).first) == layers.end ()) {
            new_layers.push_back ((*l).first);
            layer_map.insert (std::make_pair (*(*l).second, (*l).first));
          }
        }

        if (pc) {
          cell_map.insert (std::make_pair (c->cell_index (), pc->cell_index ()));
        } else {
          //  fallback: create a new cell
          db::cell_index_type ci = layout.add_cell (m_layout, c->cell_index ());
          cell_map.insert (std::make_pair (c->cell_index (), ci));
        }

      } else {

        std::pair <bool, db::cell_index_type> ci = layout.cell_by_name (m_layout.cell_name (c->cell_index ()));
        if (m_incomplete_cells.find (c->cell_index ()) != m_incomplete_cells.end ()) {
          if (ci.first) {
            cell_map.insert (std::make_pair (c->cell_index (), ci.second));
          } else {
            //  make ghost cells from incomplete new ones.
            db::cell_index_type tc = layout.add_cell (m_layout.cell_name (c->cell_index ()));
            layout.cell (tc).set_ghost_cell (true);
            cell_map.insert (std::make_pair (c->cell_index (), tc));
          }
        } else {
          db::cell_index_type ci = layout.add_cell (m_layout, c->cell_index ());
          cell_map.insert (std::make_pair (c->cell_index (), ci));
        }

      }
    }
  }

  //  copy the shapes

  for (unsigned int l = 0; l < m_layout.layers (); ++l) {
    if (m_layout.is_valid_layer (l)) {

      //  look up the target layer
      unsigned int tl = 0;
      std::map <db::LayerProperties, unsigned int, db::LPLogicalLessFunc>::const_iterator lm = layer_map.find (m_layout.get_properties (l));
      if (lm != layer_map.end ()) {
        tl = lm->second;
      } else {
        tl = layout.insert_layer (m_layout.get_properties (l));
        new_layers.push_back (tl);
      }

      //  actually copy the shapes
      for (db::Layout::const_iterator c = m_layout.begin (); c != m_layout.end (); ++c) {

        std::map <db::cell_index_type, db::cell_index_type>::const_iterator cp = cell_map.find (c->cell_index ());
        if (cp != cell_map.end ()) {

          db::Shapes &t = layout.cell (cp->second).shapes (tl);

          for (db::Shapes::shape_iterator sh = c->shapes (l).begin (db::Shapes::shape_iterator::All); ! sh.at_end (); ++sh) {

            db::Shape new_shape = t.insert (*sh, prop_id_map);
            if (trans) {
              new_shape = t.transform (new_shape, *trans);
            }
            if (insert_receiver) {
              insert_receiver->shape_inserted (cp->second, tl, new_shape);
            }

          }

        }

      }

    }
  }

  //  copy the instances

  for (db::Layout::const_iterator c = m_layout.begin (); c != m_layout.end (); ++c) {

    std::map <db::cell_index_type, db::cell_index_type>::const_iterator cp = cell_map.find (c->cell_index ());
    if (cp != cell_map.end ()) {

      db::Cell &t = layout.cell (cp->second);

      for (db::Cell::const_iterator inst = c->begin (); ! inst.at_end (); ++inst) {

        tl::const_map<db::cell_index_type> im (cell_map.find (inst->cell_index ())->second);
        db::Instance new_inst = t.insert (*inst, im, prop_id_map);
        if (trans) {
          new_inst = t.transform (new_inst, *trans);
        }

        if (insert_receiver) {
          insert_receiver->instance_inserted (cp->second, new_inst);
        }

      }

    }

  }

  //  if requested, determine the new top cells and fill the result vector
  if (new_tops) {
    for (db::Layout::top_down_const_iterator tc = m_layout.begin_top_down (); tc != m_layout.end_top_cells (); ++tc) {
      if (*tc != m_container_cell_index) {
        new_tops->push_back (cell_map.find (*tc)->second);
      }
    }
  }

  return new_layers;
}

db::cell_index_type 
ClipboardData::cell_for_cell (const db::Layout &layout, db::cell_index_type cell_index, bool incomplete)
{
  std::map <db::cell_index_type, db::cell_index_type>::const_iterator cm = m_cell_index_map.find (cell_index);
  if (cm != m_cell_index_map.end ()) {
    return cm->second;
  }

  db::cell_index_type target_cell_index = m_layout.add_cell (layout, cell_index);
  m_cell_index_map.insert (std::make_pair (cell_index, target_cell_index));

  if (incomplete) {
    m_incomplete_cells.insert (target_cell_index);
    if (layout.cell (cell_index).is_proxy ()) {
      std::vector<std::string> context_info;
      if (layout.get_context_info (cell_index, context_info)) {
        m_context_info.insert (std::make_pair (target_cell_index, context_info));
      }
    }
  }

  return target_cell_index;
}

}

