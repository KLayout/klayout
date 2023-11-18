
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


#include "dbDeepShapeStore.h"
#include "dbCellMapping.h"
#include "dbLayoutUtils.h"
#include "dbRegion.h"
#include "dbEdges.h"
#include "dbEdgePairs.h"
#include "dbTexts.h"
#include "dbDeepRegion.h"
#include "dbDeepEdges.h"
#include "dbDeepEdgePairs.h"
#include "dbDeepTexts.h"
#include "dbShapeCollection.h"
#include "dbLayoutToNetlist.h"

#include "tlTimer.h"

namespace db
{

// ----------------------------------------------------------------------------------

DeepLayer::DeepLayer ()
  : mp_store (), m_layout (0), m_layer (0)
{
  //  .. nothing yet ..
}

DeepLayer::DeepLayer (const Region &region)
  : mp_store (), m_layout (0), m_layer (0)
{
  const db::DeepRegion *dr = dynamic_cast<const db::DeepRegion *> (region.delegate ());
  tl_assert (dr != 0);
  *this = dr->deep_layer ();
}

DeepLayer::DeepLayer (const Texts &texts)
  : mp_store (), m_layout (0), m_layer (0)
{
  const db::DeepTexts *dr = dynamic_cast<const db::DeepTexts *> (texts.delegate ());
  tl_assert (dr != 0);
  *this = dr->deep_layer ();
}

DeepLayer::DeepLayer (const Edges &edges)
  : mp_store (), m_layout (0), m_layer (0)
{
  const db::DeepEdges *dr = dynamic_cast<const db::DeepEdges *> (edges.delegate ());
  tl_assert (dr != 0);
  *this = dr->deep_layer ();
}

DeepLayer::DeepLayer (const EdgePairs &edge_pairs)
  : mp_store (), m_layout (0), m_layer (0)
{
  const db::DeepEdgePairs *dr = dynamic_cast<const db::DeepEdgePairs *> (edge_pairs.delegate ());
  tl_assert (dr != 0);
  *this = dr->deep_layer ();
}

DeepLayer::DeepLayer (const DeepLayer &x)
  : mp_store (x.mp_store), m_layout (x.m_layout), m_layer (x.m_layer)
{
  if (mp_store.get ()) {
    mp_store->add_ref (m_layout, m_layer);
  }
}

DeepLayer::DeepLayer (DeepShapeStore *store, unsigned int layout, unsigned int layer)
  : mp_store (store), m_layout (layout), m_layer (layer)
{
  if (store) {
    store->add_ref (layout, layer);
  }
}

DeepLayer &DeepLayer::operator= (const DeepLayer &other)
{
  if (this != &other) {
    if (mp_store.get ()) {
      mp_store->remove_ref (m_layout, m_layer);
    }
    mp_store = other.mp_store;
    m_layout = other.m_layout;
    m_layer = other.m_layer;
    if (mp_store.get ()) {
      mp_store->add_ref (m_layout, m_layer);
    }
  }

  return *this;
}

DeepLayer::~DeepLayer ()
{
  if (mp_store.get ()) {
    mp_store->remove_ref (m_layout, m_layer);
  }
}

DeepLayer
DeepLayer::derived () const
{
  return DeepLayer (const_cast <db::DeepShapeStore *> (mp_store.get ()), m_layout, const_cast <db::Layout &> (layout ()).insert_layer ());
}

DeepLayer
DeepLayer::copy () const
{
  DeepLayer new_layer (derived ());

  db::DeepShapeStore *non_const_store = const_cast<db::DeepShapeStore *> (mp_store.get ());
  non_const_store->layout (m_layout).copy_layer (m_layer, new_layer.layer ());

  return new_layer;
}

void
DeepLayer::add_from (const DeepLayer &dl)
{
  if (&dl.layout () == &layout ()) {

    //  intra-layout merge

    layout ().copy_layer (dl.layer (), layer ());

  } else {

    //  inter-layout merge

    db::cell_index_type into_cell = initial_cell ().cell_index ();
    db::Layout *into_layout = &layout ();
    db::cell_index_type source_cell = dl.initial_cell ().cell_index ();
    const db::Layout *source_layout = &dl.layout ();

    //  create or reuse a layout mapping

    const db::CellMapping *cell_mapping = 0;
    db::CellMapping cm;
    if (store () == dl.store ()) {
      cell_mapping = &const_cast<db::DeepShapeStore *> (mp_store.get ())->internal_cell_mapping (layout_index (), dl.layout_index ());
    } else {
      cm.create_from_geometry_full (*into_layout, into_cell, *source_layout, source_cell);
      cell_mapping = &cm;
    }

    //  actually copy the shapes

    std::map<unsigned int, unsigned int> lm;
    lm.insert (std::make_pair (dl.layer (), layer ()));

    std::vector <db::cell_index_type> source_cells;
    source_cells.push_back (source_cell);
    db::copy_shapes (*into_layout, *source_layout, db::ICplxTrans (), source_cells, cell_mapping->table (), lm);

  }
}

const std::set<db::cell_index_type> *
DeepLayer::breakout_cells () const
{
  return store ()->breakout_cells (layout_index ());
}

void
DeepLayer::insert_into (db::Layout *into_layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  check_dss ();
  const_cast<db::DeepShapeStore *> (mp_store.get ())->insert (*this, into_layout, into_cell, into_layer);
}

void
DeepLayer::insert_into_as_polygons (db::Layout *into_layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl) const
{
  check_dss ();
  const_cast<db::DeepShapeStore *> (mp_store.get ())->insert_as_polygons (*this, into_layout, into_cell, into_layer, enl);
}

bool DeepLayer::operator< (const DeepLayer &other) const
{
  if (mp_store.get () != other.mp_store.get ()) {
    return mp_store.get () < other.mp_store.get ();
  }
  if (m_layout != other.m_layout) {
    return m_layout < other.m_layout;
  }
  if (m_layer != other.m_layer) {
    return m_layer < other.m_layer;
  }
  return false;
}

bool DeepLayer::operator== (const DeepLayer &other) const
{
  if (mp_store.get () != other.mp_store.get ()) {
    return false;
  }
  if (m_layout != other.m_layout) {
    return false;
  }
  if (m_layer != other.m_layer) {
    return false;
  }
  return true;
}

db::Layout &
DeepLayer::layout ()
{
  check_dss ();
  return mp_store->layout (m_layout);
}

const db::Layout &
DeepLayer::layout () const
{
  check_dss ();
  return const_cast<db::DeepShapeStore *> (mp_store.get ())->layout (m_layout);
}

db::Cell &
DeepLayer::initial_cell ()
{
  check_dss ();
  return mp_store->initial_cell (m_layout);
}

const db::Cell &
DeepLayer::initial_cell () const
{
  check_dss ();
  return mp_store->const_initial_cell (m_layout);
}

void
DeepLayer::check_dss () const
{
  if (mp_store.get () == 0) {
    throw tl::Exception (tl::to_string (tr ("Heap lost: the DeepShapeStore container no longer exists")));
  }
}

struct DeepShapeStore::LayoutHolder
{
  class L2NStatusChangedListener
    : public tl::Object
  {
  public:
    L2NStatusChangedListener (DeepShapeStore::LayoutHolder *lh, db::LayoutToNetlist *l2n)
      : mp_lh (lh), mp_l2n (l2n)
    {
      mp_l2n->status_changed_event ().add (this, &L2NStatusChangedListener::l2n_destroyed);
    }

  private:
    void l2n_destroyed (gsi::ObjectBase::StatusEventType ev)
    {
      if (ev == gsi::ObjectBase::ObjectDestroyed) {
        //  CAUTION: this will eventually delete *this!
        mp_lh->remove_l2n (mp_l2n);
      }
    }

    DeepShapeStore::LayoutHolder *mp_lh;
    db::LayoutToNetlist *mp_l2n;
  };

  class VariantsCreatedListener
    : public tl::Object
  {
  public:
    VariantsCreatedListener (DeepShapeStore::LayoutHolder *lh, db::Layout *ly)
      : mp_lh (lh)
    {
      ly->variants_created_event.add (this, &VariantsCreatedListener::variants_created);
    }

  private:
    void variants_created (const std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > *var_map)
    {
      for (std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> >::const_iterator i = var_map->begin (); i != var_map->end (); ++i) {
        for (std::map<db::ICplxTrans, db::cell_index_type>::const_iterator j = i->second.begin (); j != i->second.end (); ++j) {
          mp_lh->builder.register_variant (i->first, j->second);
        }
      }
    }

    DeepShapeStore::LayoutHolder *mp_lh;
  };

  LayoutHolder (const db::ICplxTrans &trans)
    : refs (0), layout (false), builder (&layout, trans), variants_created (this, &layout)
  {
    //  .. nothing yet ..
  }

  void add_layer_ref (unsigned int layer)
  {
    layer_refs [layer] += 1;
  }

  bool remove_layer_ref (unsigned int layer)
  {
    if ((layer_refs[layer] -= 1) <= 0) {
      layout.delete_layer (layer);
      layer_refs.erase (layer);
      return true;
    } else {
      return false;
    }
  }

  bool has_net_builder_for (db::LayoutToNetlist *l2n) const
  {
    auto l = net_builders.find (l2n);
    return (l != net_builders.end ());
  }

  db::NetBuilder &net_builder_for (db::Cell &top, db::LayoutToNetlist *l2n)
  {
    auto l = net_builders.find (l2n);
    if (l == net_builders.end ()) {
      l = net_builders.insert (std::make_pair (l2n, std::make_pair (L2NStatusChangedListener (this, l2n), db::NetBuilder (&layout, l2n->cell_mapping_into (layout, top, false), l2n)))).first;
      return l->second.second;
    } else {
      return l->second.second;
    }
  }

  void remove_l2n (db::LayoutToNetlist *l2n)
  {
    auto l = net_builders.find (l2n);
    if (l != net_builders.end ()) {
      net_builders.erase (l);
    }
  }

  int refs;
  db::Layout layout;
  db::HierarchyBuilder builder;

private:
  VariantsCreatedListener variants_created;
  std::map<db::LayoutToNetlist *, std::pair<L2NStatusChangedListener, db::NetBuilder> > net_builders;
  std::map<unsigned int, int> layer_refs;
};

// ----------------------------------------------------------------------------------

DeepShapeStoreState::DeepShapeStoreState ()
  : m_threads (1), m_max_area_ratio (3.0), m_max_vertex_count (16), m_reject_odd_polygons (false), m_text_property_name (), m_text_enlargement (-1), m_subcircuit_hierarchy_for_nets (false)
{
  //  .. nothing yet ..
}

void DeepShapeStoreState::set_reject_odd_polygons (bool f)
{
  m_reject_odd_polygons = f;
}

bool DeepShapeStoreState::reject_odd_polygons () const
{
  return m_reject_odd_polygons;
}

void DeepShapeStoreState::set_text_enlargement (int enl)
{
  m_text_enlargement = enl;
}

int DeepShapeStoreState::text_enlargement () const
{
  return m_text_enlargement;
}

void DeepShapeStoreState::set_text_property_name (const tl::Variant &pn)
{
  m_text_property_name = pn;
}

const tl::Variant &
DeepShapeStoreState::text_property_name () const
{
  return m_text_property_name;
}

const std::set<db::cell_index_type> *
DeepShapeStoreState::breakout_cells (unsigned int layout_index) const
{
  const std::set<db::cell_index_type> &boc = (const_cast<DeepShapeStoreState *> (this))->ensure_breakout_cells (layout_index);
  if (boc.empty ()) {
    return 0;
  } else {
    return &boc;
  }
}

void
DeepShapeStoreState::clear_breakout_cells (unsigned int layout_index)
{
  ensure_breakout_cells (layout_index).clear ();
}

void
DeepShapeStoreState::set_breakout_cells (unsigned int layout_index, const std::set<db::cell_index_type> &boc)
{
  ensure_breakout_cells (layout_index) = boc;
}

void
DeepShapeStoreState::add_breakout_cell (unsigned int layout_index, db::cell_index_type ci)
{
  ensure_breakout_cells (layout_index).insert (ci);
}

void
DeepShapeStoreState::add_breakout_cells (unsigned int layout_index, const std::set<db::cell_index_type> &cc)
{
  ensure_breakout_cells (layout_index).insert (cc.begin (), cc.end ());
}

void
DeepShapeStoreState::set_threads (int n)
{
  m_threads = n;
}

int
DeepShapeStoreState::threads () const
{
  return m_threads;
}

void
DeepShapeStoreState::set_max_area_ratio (double ar)
{
  m_max_area_ratio = ar;
}

double
DeepShapeStoreState::max_area_ratio () const
{
  return m_max_area_ratio;
}

void
DeepShapeStoreState::set_max_vertex_count (size_t n)
{
  m_max_vertex_count = n;
}

size_t
DeepShapeStoreState::max_vertex_count () const
{
  return m_max_vertex_count;
}

void
DeepShapeStoreState::set_subcircuit_hierarchy_for_nets (bool f)
{
  m_subcircuit_hierarchy_for_nets = f;
}

bool
DeepShapeStoreState::subcircuit_hierarchy_for_nets () const
{
  return m_subcircuit_hierarchy_for_nets;
}


// ----------------------------------------------------------------------------------

static size_t s_instance_count = 0;

static unsigned int init_layer (db::Layout &layout, const db::RecursiveShapeIterator &si)
{
  unsigned int layer_index = layout.insert_layer ();

  if (si.layout ()) {
    //  try to preserve the layer properties
    if (! si.multiple_layers ()) {
      if (si.layer () < si.layout ()->layers ()) {
        layout.set_properties (layer_index, si.layout ()->get_properties (si.layer ()));
      }
    } else if (! si.layers ().empty ()) {
      layout.set_properties (layer_index, si.layout ()->get_properties (si.layers ().front ()));
    }
  }

  return layer_index;
}

DeepShapeStore::DeepShapeStore ()
  : m_keep_layouts (true), m_wants_all_cells (false)
{
  ++s_instance_count;
}

DeepShapeStore::DeepShapeStore (const std::string &topcell_name, double dbu)
  : m_keep_layouts (true), m_wants_all_cells (false)
{
  ++s_instance_count;

  m_layouts.push_back (new LayoutHolder (db::ICplxTrans ()));
  m_layouts.back ()->layout.dbu (dbu);
  m_layouts.back ()->layout.add_cell (topcell_name.c_str ());
}

DeepShapeStore::~DeepShapeStore ()
{
  --s_instance_count;

  for (std::vector<LayoutHolder *>::iterator h = m_layouts.begin (); h != m_layouts.end (); ++h) {
    delete *h;
  }
  m_layouts.clear ();
}

DeepLayer DeepShapeStore::create_from_flat (const db::Region &region, bool for_netlist, double max_area_ratio, size_t max_vertex_count, const db::ICplxTrans &trans)
{
  //  reuse existing layer
  std::pair<bool, DeepLayer> lff = layer_for_flat (region);
  if (lff.first) {
    return lff.second;
  }

  require_singular ();

  unsigned int layer = init_layer (layout (), region.iter ());

  //  attempt to initialize the layer properties

  if (max_area_ratio == 0.0) {
    max_area_ratio = m_state.max_area_ratio ();
  }
  if (max_vertex_count == 0) {
    max_vertex_count = m_state.max_vertex_count ();
  }

  db::Shapes *shapes = &initial_cell ().shapes (layer);
  db::Box world = db::Box::world ();

  //  try to maintain the texts on top level - go through shape iterator
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> ii = region.begin_iter ();
  db::ICplxTrans ttop = trans * ii.second;

  //  The chain of operators for producing clipped and reduced polygon references
  db::PolygonReferenceHierarchyBuilderShapeReceiver refs (&layout (), ii.first.layout (), text_enlargement (), text_property_name ());
  db::ReducingHierarchyBuilderShapeReceiver red (&refs, max_area_ratio, max_vertex_count, m_state.reject_odd_polygons ());

  while (! ii.first.at_end ()) {

    if (for_netlist && ii.first->is_text () && ii.first.layout () && ii.first.cell () != ii.first.top_cell ()) {
      //  Skip texts on levels below top cell. For the reasoning see the description of this method.
    } else {
      red.push (*ii.first, ii.first.prop_id (), ttop * ii.first.trans (), world, 0, shapes);
    }

    ++ii.first;

  }

  DeepLayer dl (this, 0 /*singular layout index*/, layer);
  m_layers_for_flat [tl::id_of (region.delegate ())] = std::make_pair (dl.layout_index (), dl.layer ());
  m_flat_region_id [std::make_pair (dl.layout_index (), dl.layer ())] = tl::id_of (region.delegate ());
  return dl;
}

DeepLayer DeepShapeStore::create_from_flat (const db::Edges &edges, const db::ICplxTrans &trans)
{
  //  reuse existing layer
  std::pair<bool, DeepLayer> lff = layer_for_flat (tl::id_of (edges.delegate ()));
  if (lff.first) {
    return lff.second;
  }

  require_singular ();

  unsigned int layer = init_layer (layout (), edges.iter ());

  db::Shapes *shapes = &initial_cell ().shapes (layer);
  db::Box world = db::Box::world ();

  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> ii = edges.begin_iter ();
  db::ICplxTrans ttop = trans * ii.second;

  db::EdgeBuildingHierarchyBuilderShapeReceiver eb (&layout (), ii.first.layout (), false);
  while (! ii.first.at_end ()) {
    eb.push (*ii.first, ii.first.prop_id (), ttop * ii.first.trans (), world, 0, shapes);
    ++ii.first;
  }

  DeepLayer dl (this, 0 /*singular layout index*/, layer);
  m_layers_for_flat [tl::id_of (edges.delegate ())] = std::make_pair (dl.layout_index (), dl.layer ());
  m_flat_region_id [std::make_pair (dl.layout_index (), dl.layer ())] = tl::id_of (edges.delegate ());
  return dl;
}

DeepLayer DeepShapeStore::create_from_flat (const db::Texts &texts, const db::ICplxTrans &trans)
{
  //  reuse existing layer
  std::pair<bool, DeepLayer> lff = layer_for_flat (tl::id_of (texts.delegate ()));
  if (lff.first) {
    return lff.second;
  }

  require_singular ();

  unsigned int layer = init_layer (layout (), texts.iter ());

  db::Shapes *shapes = &initial_cell ().shapes (layer);
  db::Box world = db::Box::world ();

  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> ii = texts.begin_iter ();
  db::ICplxTrans ttop = trans * ii.second;

  db::TextBuildingHierarchyBuilderShapeReceiver tb (&layout (), ii.first.layout ());

  while (! ii.first.at_end ()) {
    tb.push (*ii.first, ii.first.prop_id (), ttop * ii.first.trans (), world, 0, shapes);
    ++ii.first;
  }

  DeepLayer dl (this, 0 /*singular layout index*/, layer);
  m_layers_for_flat [tl::id_of (texts.delegate ())] = std::make_pair (dl.layout_index (), dl.layer ());
  m_flat_region_id [std::make_pair (dl.layout_index (), dl.layer ())] = tl::id_of (texts.delegate ());
  return dl;
}

std::pair<bool, DeepLayer> DeepShapeStore::layer_for_flat (const ShapeCollection &coll) const
{
  return layer_for_flat (tl::id_of (coll.get_delegate ()));
}

std::pair<bool, DeepLayer> DeepShapeStore::layer_for_flat (size_t region_id) const
{
  std::map<size_t, std::pair<unsigned int, unsigned int> >::const_iterator lff = m_layers_for_flat.find (region_id);
  if (lff == m_layers_for_flat.end ()) {
    return std::make_pair (false, DeepLayer ());
  } else {
    return std::make_pair (true, DeepLayer (const_cast<DeepShapeStore *> (this), lff->second.first, lff->second.second));
  }
}

bool DeepShapeStore::is_singular () const
{
  return m_layouts.size () == 1;
}

void DeepShapeStore::require_singular () const
{
  if (! is_singular ()) {
    throw tl::Exception (tl::to_string (tr ("Internal error: deep shape store isn't singular. This may happen if you try to mix hierarchical layers from different sources our you use clipping.")));
  }
}

Cell &DeepShapeStore::initial_cell(unsigned int n)
{
  db::Layout &ly = layout (n);
  tl_assert (ly.cells () > 0);
  return ly.cell (*ly.begin_top_down ());
}

const db::Cell &DeepShapeStore::const_initial_cell (unsigned int n) const
{
  const db::Layout &ly = const_layout (n);
  tl_assert (ly.cells () > 0);
  return ly.cell (*ly.begin_top_down ());
}

void DeepShapeStore::set_text_enlargement (int enl)
{
  m_state.set_text_enlargement (enl);
}

int DeepShapeStore::text_enlargement () const
{
  return m_state.text_enlargement ();
}

void DeepShapeStore::set_text_property_name (const tl::Variant &pn)
{
  m_state.set_text_property_name (pn);
}

const tl::Variant &DeepShapeStore::text_property_name () const
{
  return m_state.text_property_name ();
}

void DeepShapeStore::set_subcircuit_hierarchy_for_nets (bool f)
{
  m_state.set_subcircuit_hierarchy_for_nets (f);
}

bool DeepShapeStore::subcircuit_hierarchy_for_nets () const
{
  return m_state.subcircuit_hierarchy_for_nets ();
}

const std::set<db::cell_index_type> *
DeepShapeStore::breakout_cells (unsigned int layout_index) const
{
  return m_state.breakout_cells (layout_index);
}

void
DeepShapeStore::clear_breakout_cells (unsigned int layout_index)
{
  m_state.clear_breakout_cells (layout_index);
}

void
DeepShapeStore::set_breakout_cells (unsigned int layout_index, const std::set<db::cell_index_type> &boc)
{
  m_state.set_breakout_cells (layout_index, boc);
}

void
DeepShapeStore::add_breakout_cell (unsigned int layout_index, db::cell_index_type ci)
{
  m_state.add_breakout_cell (layout_index, ci);
}

void
DeepShapeStore::add_breakout_cells (unsigned int layout_index, const std::set<db::cell_index_type> &cc)
{
  m_state.add_breakout_cells (layout_index, cc);
}

bool
DeepShapeStore::has_net_builder_for (unsigned int layout_index, db::LayoutToNetlist *l2n)
{
  return m_layouts [layout_index]->has_net_builder_for (l2n);
}

db::NetBuilder &
DeepShapeStore::net_builder_for (unsigned int layout_index, db::LayoutToNetlist *l2n)
{
  db::NetBuilder &nb = m_layouts [layout_index]->net_builder_for (initial_cell (layout_index), l2n);

  if (subcircuit_hierarchy_for_nets ()) {
    nb.set_hier_mode (db::BNH_SubcircuitCells);
    nb.set_cell_name_prefix ("X$$"); //  TODO: needs to be a configuration option?
  } else {
    nb.set_hier_mode (db::BNH_Flatten);
  }

  return nb;
}

void DeepShapeStore::set_threads (int n)
{
  m_state.set_threads (n);
}

int DeepShapeStore::threads () const
{
  return m_state.threads ();
}

void DeepShapeStore::set_max_area_ratio (double ar)
{
  m_state.set_max_area_ratio (ar);
}

double DeepShapeStore::max_area_ratio () const
{
  return m_state.max_area_ratio ();
}

void DeepShapeStore::set_wants_all_cells (bool f)
{
  m_wants_all_cells = f;
}

bool DeepShapeStore::wants_all_cells () const
{
  return m_wants_all_cells;
}

void DeepShapeStore::set_reject_odd_polygons (bool f)
{
  m_state.set_reject_odd_polygons (f);
}

bool DeepShapeStore::reject_odd_polygons () const
{
  return m_state.reject_odd_polygons ();
}

void DeepShapeStore::set_max_vertex_count (size_t n)
{
  m_state.set_max_vertex_count (n);
}

size_t DeepShapeStore::max_vertex_count () const
{
  return m_state.max_vertex_count ();
}

void DeepShapeStore::push_state ()
{
  m_state_stack.push_back (m_state);
}

void DeepShapeStore::pop_state ()
{
  if (! m_state_stack.empty ()) {
    m_state = m_state_stack.back ();
    m_state_stack.pop_back ();
  }
}

bool DeepShapeStore::is_valid_layout_index (unsigned int n) const
{
  return (n < (unsigned int) m_layouts.size () && m_layouts[n] != 0);
}

const db::Layout &DeepShapeStore::const_layout (unsigned int n) const
{
  tl_assert (is_valid_layout_index (n));
  return m_layouts [n]->layout;
}

db::Layout &DeepShapeStore::layout (unsigned int n)
{
  tl_assert (is_valid_layout_index (n));
  return m_layouts [n]->layout;
}

unsigned int DeepShapeStore::layout_index (const db::Layout *layout) const
{
  for (std::vector<LayoutHolder *>::const_iterator i = m_layouts.begin (); i != m_layouts.end (); ++i) {
    if (&(*i)->layout == layout) {
      return (unsigned int) (i - m_layouts.begin ());
    }
  }
  tl_assert (false);
}

size_t DeepShapeStore::instance_count ()
{
  return s_instance_count;
}

void DeepShapeStore::add_ref (unsigned int layout, unsigned int layer)
{
  tl::MutexLocker locker (&m_lock);

  tl_assert (layout < (unsigned int) m_layouts.size () && m_layouts[layout] != 0);

  m_layouts[layout]->refs += 1;
  m_layouts[layout]->add_layer_ref (layer);
}

void DeepShapeStore::remove_ref (unsigned int layout, unsigned int layer)
{
  tl::MutexLocker locker (&m_lock);

  tl_assert (layout < (unsigned int) m_layouts.size () && m_layouts[layout] != 0);

  if (m_layouts[layout]->remove_layer_ref (layer)) {

    //  remove from flat region cross ref if required
    std::map<std::pair<unsigned int, unsigned int>, size_t>::iterator fri = m_flat_region_id.find (std::make_pair (layout, layer));
    if (fri != m_flat_region_id.end ()) {
      m_layers_for_flat.erase (fri->second);
      m_flat_region_id.erase (fri);
    }

  }

  if ((m_layouts[layout]->refs -= 1) <= 0 && ! m_keep_layouts) {
    delete m_layouts[layout];
    m_layouts[layout] = 0;
    clear_breakout_cells (layout);
  }
}

unsigned int
DeepShapeStore::layout_for_iter (const db::RecursiveShapeIterator &si, const db::ICplxTrans &trans)
{
  size_t gen_id = si.layout () ? si.layout ()->hier_generation_id () : 0;

  layout_map_type::iterator l = m_layout_map.find (std::make_pair (si, std::make_pair (gen_id, trans)));
  if (l == m_layout_map.end () || m_layouts[l->second] == 0) {

    unsigned int layout_index;

    if (l != m_layout_map.end ()) {
      //  reuse discarded entry
      layout_index = l->second;
      m_layouts[layout_index] = new LayoutHolder (trans);
    } else {
      layout_index = (unsigned int) m_layouts.size ();
      m_layouts.push_back (new LayoutHolder (trans));
    }

    db::Layout &layout = m_layouts[layout_index]->layout;
    if (si.layout ()) {
      layout.dbu (si.layout ()->dbu () / trans.mag ());
    }

    m_layout_map[std::make_pair (si, std::make_pair (gen_id, trans))] = layout_index;
    return layout_index;

  } else {
    return l->second;
  }
}

void DeepShapeStore::make_layout (unsigned int layout_index, const db::RecursiveShapeIterator &si, const db::ICplxTrans &trans)
{
  size_t gen_id = si.layout () ? si.layout ()->hier_generation_id () : 0;
  tl_assert (m_layout_map.find (std::make_pair (si, std::make_pair (gen_id, trans))) == m_layout_map.end ());

  while (m_layouts.size () <= layout_index) {
    m_layouts.push_back (0);
  }

  m_layouts[layout_index] = new LayoutHolder (trans);

  db::Layout &layout = m_layouts[layout_index]->layout;
  if (si.layout ()) {
    layout.dbu (si.layout ()->dbu () / trans.mag ());
  }

  m_layout_map[std::make_pair (si, std::make_pair (gen_id, trans))] = layout_index;
}

DeepLayer DeepShapeStore::create_polygon_layer (const db::RecursiveShapeIterator &si, double max_area_ratio, size_t max_vertex_count, const db::ICplxTrans &trans)
{
  if (max_area_ratio == 0.0) {
    max_area_ratio = m_state.max_area_ratio ();
  }
  if (max_vertex_count == 0) {
    max_vertex_count = m_state.max_vertex_count ();
  }

  unsigned int layout_index = layout_for_iter (si, trans);

  db::Layout &layout = m_layouts[layout_index]->layout;
  db::HierarchyBuilder &builder = m_layouts[layout_index]->builder;

  builder.set_wants_all_cells (m_wants_all_cells);

  unsigned int layer_index = init_layer (layout, si);
  builder.set_target_layer (layer_index);

  //  The chain of operators for producing clipped and reduced polygon references
  db::PolygonReferenceHierarchyBuilderShapeReceiver refs (&layout, si.layout (), text_enlargement (), text_property_name ());
  db::ReducingHierarchyBuilderShapeReceiver red (&refs, max_area_ratio, max_vertex_count, m_state.reject_odd_polygons ());
  db::ClippingHierarchyBuilderShapeReceiver clip (&red);

  //  Build the working hierarchy from the recursive shape iterator
  try {

    tl::SelfTimer timer (tl::verbosity () >= 41, tl::to_string (tr ("Building working hierarchy")));
    db::LayoutLocker ll (&layout, true /*no update*/);

    builder.set_shape_receiver (&clip);
    db::RecursiveShapeIterator (si).push (& builder);
    builder.set_shape_receiver (0);

  } catch (...) {
    builder.set_shape_receiver (0);
    throw;
  }

  return DeepLayer (this, layout_index, layer_index);
}

DeepLayer DeepShapeStore::create_custom_layer (const db::RecursiveShapeIterator &si, HierarchyBuilderShapeReceiver *pipe, const db::ICplxTrans &trans)
{
  unsigned int layout_index = layout_for_iter (si, trans);

  db::Layout &layout = m_layouts[layout_index]->layout;
  db::HierarchyBuilder &builder = m_layouts[layout_index]->builder;

  unsigned int layer_index = init_layer (layout, si);
  builder.set_target_layer (layer_index);

  //  Build the working hierarchy from the recursive shape iterator
  try {

    tl::SelfTimer timer (tl::verbosity () >= 41, tl::to_string (tr ("Building working hierarchy")));
    db::LayoutLocker ll (&layout, true /*no update*/);

    builder.set_shape_receiver (pipe);
    db::RecursiveShapeIterator (si).push (& builder);
    builder.set_shape_receiver (0);

  } catch (...) {
    builder.set_shape_receiver (0);
    throw;
  }

  return DeepLayer (this, layout_index, layer_index);
}

DeepLayer DeepShapeStore::create_copy (const DeepLayer &source, HierarchyBuilderShapeReceiver *pipe)
{
  tl_assert (source.store () == this);

  unsigned int from_layer_index = source.layer ();
  db::Layout &ly = layout ();

  unsigned int layer_index = ly.insert_layer ();

  //  Build the working hierarchy from the recursive shape iterator
  tl::SelfTimer timer (tl::verbosity () >= 41, tl::to_string (tr ("Building working hierarchy")));

  db::Box region = db::Box::world ();
  db::ICplxTrans trans;

  for (db::Layout::iterator c = ly.begin (); c != ly.end (); ++c) {
    db::Shapes &into = c->shapes (layer_index);
    const db::Shapes &from = c->shapes (from_layer_index);
    for (db::Shapes::shape_iterator s = from.begin (db::ShapeIterator::All); ! s.at_end (); ++s) {
      pipe->push (*s, s->prop_id (), trans, region, 0, &into);
    }
  }

  return DeepLayer (this, source.layout_index (), layer_index);
}

DeepLayer DeepShapeStore::create_edge_layer (const db::RecursiveShapeIterator &si, bool as_edges, const db::ICplxTrans &trans)
{
  unsigned int layout_index = layout_for_iter (si, trans);
  db::Layout &layout = m_layouts[layout_index]->layout;

  db::EdgeBuildingHierarchyBuilderShapeReceiver refs (&layout, si.layout (), as_edges);
  return create_custom_layer (si, &refs, trans);
}

DeepLayer DeepShapeStore::create_edge_pair_layer (const db::RecursiveShapeIterator &si, const db::ICplxTrans &trans)
{
  unsigned int layout_index = layout_for_iter (si, trans);
  db::Layout &layout = m_layouts[layout_index]->layout;

  db::EdgePairBuildingHierarchyBuilderShapeReceiver refs (&layout, si.layout ());
  return create_custom_layer (si, &refs, trans);
}

DeepLayer DeepShapeStore::create_text_layer (const db::RecursiveShapeIterator &si, const db::ICplxTrans &trans)
{
  unsigned int layout_index = layout_for_iter (si, trans);
  db::Layout &layout = m_layouts[layout_index]->layout;

  db::TextBuildingHierarchyBuilderShapeReceiver refs (&layout, si.layout ());
  return create_custom_layer (si, &refs, trans);
}

const db::CellMapping &
DeepShapeStore::internal_cell_mapping (unsigned int into_layout_index, unsigned int from_layout_index)
{
  db::Layout &into_layout = layout (into_layout_index);
  db::cell_index_type into_cell = initial_cell (into_layout_index).cell_index ();
  const db::Layout &source_layout = layout (from_layout_index);
  db::cell_index_type source_cell = initial_cell (from_layout_index).cell_index ();

  std::map<std::pair<unsigned int, unsigned int>, CellMappingWithGenerationIds>::iterator cm = m_internal_mapping_cache.find (std::make_pair (from_layout_index, into_layout_index));
  if (cm == m_internal_mapping_cache.end () || ! cm->second.is_valid (into_layout, source_layout)) {

    cm = m_internal_mapping_cache.insert (std::make_pair (std::make_pair (from_layout_index, into_layout_index), CellMappingWithGenerationIds ())).first;

    cm->second.clear ();
    cm->second.create_from_geometry_full (into_layout, into_cell, source_layout, source_cell);
    cm->second.set_generation_ids (into_layout, source_layout);

  }

  return cm->second;
}

const db::CellMapping &
DeepShapeStore::cell_mapping_to_original (unsigned int layout_index, db::Layout *into_layout, db::cell_index_type into_cell, const std::set<db::cell_index_type> *excluded_cells, const std::set<db::cell_index_type> *included_cells)
{
  db::Layout *source_layout = &m_layouts [layout_index]->layout;
  if (source_layout->begin_top_down () == source_layout->end_top_cells ()) {
    //  empty source - nothing to do.
    static db::CellMapping cm;
    return cm;
  }

  db::cell_index_type source_top = *source_layout->begin_top_down();

  db::HierarchyBuilder &original_builder = m_layouts [layout_index]->builder;

  //  Derive a cell mapping for source to target. We reuse any existing mapping for returning the
  //  shapes into the original layout.

  DeliveryMappingCacheKey key (layout_index, tl::id_of (into_layout), into_cell);

  std::map<DeliveryMappingCacheKey, CellMappingWithGenerationIds>::iterator cm = m_delivery_mapping_cache.find (key);
  if (cm == m_delivery_mapping_cache.end () || ! cm->second.is_valid (*into_layout, *source_layout)) {

    cm = m_delivery_mapping_cache.insert (std::make_pair (key, CellMappingWithGenerationIds ())).first;
    cm->second.clear ();

    //  collects the cell mappings we skip because they are variants (variant building or box variants)
    std::map<db::cell_index_type, db::HierarchyBuilder::CellMapKey> cm_skipped_variants;

    if (into_layout == original_builder.source ().layout () && &into_layout->cell (into_cell) == original_builder.source ().top_cell () && original_builder.source ().global_trans ().is_unity ()) {

      //  This is the case of mapping back to the original. In this case we can use the information
      //  provided inside the original hierarchy builders. They list the source cells and the target cells
      //  create from them. We need to consider however, that the hierarchy builder is allowed to create
      //  variants which we cannot map.

      for (HierarchyBuilder::cell_map_type::const_iterator m = original_builder.begin_cell_map (); m != original_builder.end_cell_map (); ) {

        HierarchyBuilder::cell_map_type::const_iterator mm = m;
        ++mm;
        bool skip = original_builder.is_variant (m->second);   //  skip variant cells
        while (mm != original_builder.end_cell_map () && mm->first.original_cell == m->first.original_cell) {
          //  we have cell (box) variants and cannot simply map
          ++mm;
          skip = true;
        }

        if (! skip) {
          cm->second.map (m->second, m->first.original_cell);
        } else {
          for (HierarchyBuilder::cell_map_type::const_iterator n = m; n != mm; ++n) {
            tl_assert (cm_skipped_variants.find (n->second) == cm_skipped_variants.end ());
            cm_skipped_variants [n->second] = n->first;
          }
        }

        m = mm;

      }

    } else if (into_layout->cells () == 1) {

      //  Another simple case is mapping into an empty (or single-top-cell-only) layout, where we can use "create_from_single_full".
      cm->second.create_single_mapping (*into_layout, into_cell, *source_layout, source_top);

    } else {

      cm->second.create_from_geometry (*into_layout, into_cell, *source_layout, source_top);

    }

    //  Add new cells for the variants and (possible) devices which are cells added during the device
    //  extraction process
    std::vector<std::pair<db::cell_index_type, db::cell_index_type> > new_pairs = cm->second.create_missing_mapping2 (*into_layout, *source_layout, source_top, excluded_cells, included_cells);

    //  the variant's originals we are going to delete
    std::set<db::cell_index_type> cells_to_delete;

    //  We now need to fix the cell map from the hierarchy builder, so we can import back from the modified layout.
    //  This is in particular important if we created new cells for known variants.
    for (std::vector<std::pair<db::cell_index_type, db::cell_index_type> >::const_iterator np = new_pairs.begin (); np != new_pairs.end (); ++np) {

      db::cell_index_type var_org = original_builder.original_target_for_variant (np->first);

      std::map<db::cell_index_type, db::HierarchyBuilder::CellMapKey>::const_iterator icm = cm_skipped_variants.find (var_org);
      if (icm != cm_skipped_variants.end ()) {

        //  create the variant clone in the original layout too and delete this cell
        VariantsCollectorBase::copy_shapes (*into_layout, np->second, icm->second.original_cell);
        cells_to_delete.insert (icm->second.original_cell);

        //  forget the original cell (now separated into variants) and map the variants back into the
        //  DSS layout
        original_builder.unmap (icm->second);
        db::HierarchyBuilder::CellMapKey k = icm->second;
        k.original_cell = np->second;
        original_builder.map (k, np->first);

        //  forget the variant as now it's a real cell in the source layout
        original_builder.unregister_variant (np->first);

        //  rename the cell because it may be a different one now
        source_layout->rename_cell (np->first, into_layout->cell_name (np->second));

      }

    }

    //  delete the variant's original cell
    if (! cells_to_delete.empty ()) {
      into_layout->delete_cells (cells_to_delete);
    }

    cm->second.set_generation_ids (*into_layout, *source_layout);

  }

  return cm->second;
}

namespace
{
  class DeepShapeStoreToShapeTransformer
    : public ShapesTransformer
  {
  public:
    DeepShapeStoreToShapeTransformer (const DeepShapeStore &dss, const db::Layout &layout)
      : mp_layout (& layout)
    {
      //  gets the text annotation property ID -
      //  this is how the texts are passed for annotating the net names
      m_text_annot_name_id = std::pair<bool, db::property_names_id_type> (false, 0);
      if (! dss.text_property_name ().is_nil ()) {
        m_text_annot_name_id = mp_layout->properties_repository ().get_id_of_name (dss.text_property_name ());
      }
    }

    void insert_transformed (Shapes &into, const Shapes &from, const ICplxTrans &trans, PropertyMapper &pm) const
    {
      if (! m_text_annot_name_id.first) {

        //  fast shortcut
        into.insert_transformed (from, trans, pm);

      } else {

        for (db::Shapes::shape_iterator i = from.begin (db::ShapeIterator::All); ! i.at_end (); ++i) {

          bool is_text = false;

          if (i->prop_id () > 0) {

            const db::PropertiesRepository::properties_set &ps = mp_layout->properties_repository ().properties (i->prop_id ());

            for (db::PropertiesRepository::properties_set::const_iterator j = ps.begin (); j != ps.end () && ! is_text; ++j) {
              if (j->first == m_text_annot_name_id.second) {

                db::Text text (j->second.to_string (), db::Trans (i->bbox ().center () - db::Point ()));
                text.transform (trans);
                if (into.layout ()) {
                  into.insert (db::TextRef (text, into.layout ()->shape_repository ()));
                } else {
                  into.insert (text);
                }

                is_text = true;

              }
            }

          }

          if (! is_text) {
            into.insert (*i, trans, pm);
          }

        }

      }

    }

  private:
    std::pair<bool, db::property_names_id_type> m_text_annot_name_id;
    const db::Layout *mp_layout;
  };
}

void
DeepShapeStore::insert (const DeepLayer &deep_layer, db::Layout *into_layout, db::cell_index_type into_cell, unsigned int into_layer)
{
  db::LayoutLocker locker (into_layout);

  const db::Layout &source_layout = deep_layer.layout ();
  if (source_layout.begin_top_down () == source_layout.end_top_cells ()) {
    //  empty source - nothing to do.
    return;
  }

  //  prepare the transformation
  db::ICplxTrans trans (source_layout.dbu () / into_layout->dbu ());

  //  prepare a layer map
  std::map<unsigned int, unsigned int> lm;
  lm.insert (std::make_pair (deep_layer.layer (), into_layer));

  //  prepare a cell mapping
  const db::CellMapping &cm = cell_mapping_to_original (deep_layer.layout_index (), into_layout, into_cell);

  //  prepare a vector with the source cells
  std::vector <db::cell_index_type> source_cells;
  source_cells.push_back (*source_layout.begin_top_down());

  //  prepare a transformer to convert text-annotated markers back to texts (without transformation however)
  DeepShapeStoreToShapeTransformer dsst (*this, source_layout);

  //  actually copy the shapes
  db::copy_shapes (*into_layout, source_layout, trans, source_cells, cm.table (), lm, &dsst);
}

void
DeepShapeStore::insert_as_polygons (const DeepLayer &deep_layer, db::Layout *into_layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl)
{
  //  prepare a temporary layer with the polygons
  DeepLayer tmp = deep_layer.derived ();

  db::Layout &layout = const_cast<db::Layout &> (deep_layer.layout ());

  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

    db::Shapes &out = c->shapes (tmp.layer ());
    for (db::Shapes::shape_iterator s = c->shapes (deep_layer.layer ()); ! s.at_end (); ++s) {

      if (s->is_edge_pair ()) {

        if (s->prop_id () != 0) {
          out.insert (db::SimplePolygonWithProperties (s->edge_pair ().normalized ().to_simple_polygon (enl), s->prop_id ()));
        } else {
          out.insert (s->edge_pair ().normalized ().to_simple_polygon (enl));
        }

      } else if (s->is_path () || s->is_polygon () || s->is_box ()) {

        db::Polygon poly;
        s->polygon (poly);
        if (s->prop_id () != 0) {
          out.insert (db::PolygonWithProperties (poly, s->prop_id ()));
        } else {
          out.insert (poly);
        }

      } else if (s->is_text ()) {

        db::Text t;
        s->text (t);
        db::SimplePolygon sp (t.box ().enlarged (db::Vector (enl, enl)));
        if (s->prop_id () != 0) {
          out.insert (db::SimplePolygonWithProperties (sp, s->prop_id ()));
        } else {
          out.insert (sp);
        }

      }

    }

  }

  //  and insert this one
  insert (tmp, into_layout, into_cell, into_layer);
}

}

