

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

#include "dbCommon.h"
#include "dbLayoutToNetlist.h"
#include "dbDeepRegion.h"
#include "dbDeepTexts.h"
#include "dbShapeRepository.h"
#include "dbCellMapping.h"
#include "dbLayoutToNetlistWriter.h"
#include "dbLayoutToNetlistReader.h"
#include "dbLayoutVsSchematic.h"
#include "dbLayoutToNetlistFormatDefs.h"
#include "dbLayoutVsSchematicFormatDefs.h"
#include "tlGlobPattern.h"

namespace db
{

//  the iterator provides the hierarchical selection (enabling/disabling cells etc.)

LayoutToNetlist::LayoutToNetlist (const db::RecursiveShapeIterator &iter)
  : m_iter (iter), m_layout_index (0), m_netlist_extracted (false), m_is_flat (false), m_device_scaling (1.0)
{
  //  check the iterator
  if (iter.has_complex_region () || iter.region () != db::Box::world ()) {
    throw tl::Exception (tl::to_string (tr ("The netlist extractor cannot work on clipped layouts")));
  }

  mp_internal_dss.reset (new db::DeepShapeStore ());
  mp_dss.reset (mp_internal_dss.get ());

  //  the dummy layer acts as a reference holder for the layout
  //  NOTE: this probably can be done better
  db::RecursiveShapeIterator empty_iter = iter;
  empty_iter.set_layers (std::vector<unsigned int> ());
  m_dummy_layer = dss ().create_polygon_layer (empty_iter);

  init ();
}

LayoutToNetlist::LayoutToNetlist (db::DeepShapeStore *dss, unsigned int layout_index)
  : mp_dss (dss), m_layout_index (layout_index), m_netlist_extracted (false), m_is_flat (false), m_device_scaling (1.0)
{
  if (dss->is_valid_layout_index (m_layout_index)) {
    m_iter = db::RecursiveShapeIterator (dss->layout (m_layout_index), dss->initial_cell (m_layout_index), std::set<unsigned int> ());
  }
}

LayoutToNetlist::LayoutToNetlist (const std::string &topcell_name, double dbu)
  : m_iter (), m_netlist_extracted (false), m_is_flat (true), m_device_scaling (1.0)
{
  mp_internal_dss.reset (new db::DeepShapeStore (topcell_name, dbu));
  mp_dss.reset (mp_internal_dss.get ());
  m_layout_index = 0 ;

  init ();
}

LayoutToNetlist::LayoutToNetlist ()
  : m_iter (), mp_internal_dss (new db::DeepShapeStore ()), mp_dss (mp_internal_dss.get ()), m_layout_index (0),
    m_netlist_extracted (false), m_is_flat (false), m_device_scaling (1.0)
{
  init ();
}

LayoutToNetlist::~LayoutToNetlist ()
{
  //  NOTE: do this in this order because of unregistration of the layers
  m_named_regions.clear ();
  m_dlrefs.clear ();
  mp_internal_dss.reset (0);
  mp_netlist.reset (0);
  m_net_clusters.clear ();
}

void LayoutToNetlist::keep_dss ()
{
  if (mp_dss.get () && ! mp_internal_dss.get ()) {
    mp_dss->keep ();
    mp_internal_dss.reset (mp_dss.get ());
  }
}

void LayoutToNetlist::init ()
{
  dss ().set_text_enlargement (1);
  dss ().set_text_property_name (tl::Variant ("LABEL"));
}

void LayoutToNetlist::set_threads (int n)
{
  dss ().set_threads (n);
}

int LayoutToNetlist::threads () const
{
  return dss ().threads ();
}

void LayoutToNetlist::set_area_ratio (double ar)
{
  dss ().set_max_area_ratio (ar);
}

double LayoutToNetlist::area_ratio () const
{
  return dss ().max_area_ratio ();
}

void LayoutToNetlist::set_max_vertex_count (size_t n)
{
  dss ().set_max_vertex_count (n);
}

size_t LayoutToNetlist::max_vertex_count () const
{
  return dss ().max_vertex_count ();
}

void LayoutToNetlist::set_device_scaling (double s)
{
  m_device_scaling = s;
}

double LayoutToNetlist::device_scaling () const
{
  return m_device_scaling;
}

db::Region *LayoutToNetlist::make_layer (const std::string &n)
{
  db::RecursiveShapeIterator si (m_iter);
  si.shape_flags (db::ShapeIterator::Nothing);

  std::auto_ptr <db::Region> region (new db::Region (si, dss ()));
  if (! n.empty ()) {
    register_layer (*region, n);
  }
  return region.release ();
}

db::Region *LayoutToNetlist::make_layer (unsigned int layer_index, const std::string &n)
{
  db::RecursiveShapeIterator si (m_iter);
  si.set_layer (layer_index);
  si.shape_flags (db::ShapeIterator::All);

  std::auto_ptr <db::Region> region (new db::Region (si, dss ()));
  if (! n.empty ()) {
    register_layer (*region, n);
  }
  return region.release ();
}

db::Texts *LayoutToNetlist::make_text_layer (unsigned int layer_index, const std::string &n)
{
  db::RecursiveShapeIterator si (m_iter);
  si.set_layer (layer_index);
  si.shape_flags (db::ShapeIterator::Texts);

  std::auto_ptr <db::Texts> texts (new db::Texts (si, dss ()));
  if (! n.empty ()) {
    register_layer (*texts, n);
  }
  return texts.release ();
}

db::Region *LayoutToNetlist::make_polygon_layer (unsigned int layer_index, const std::string &n)
{
  db::RecursiveShapeIterator si (m_iter);
  si.set_layer (layer_index);
  si.shape_flags (db::ShapeIterator::Paths | db::ShapeIterator::Polygons | db::ShapeIterator::Boxes);

  std::auto_ptr <db::Region> region (new db::Region (si, dss ()));
  if (! n.empty ()) {
    register_layer (*region, n);
  }
  return region.release ();
}

void LayoutToNetlist::link_nets (const db::Net *net, const db::Net *with)
{
  if (! net->circuit () || net->circuit () != with->circuit () || ! internal_layout ()
      || ! internal_layout ()->is_valid_cell_index (net->circuit ()->cell_index ())
      || net->cluster_id () == 0 || with->cluster_id () == 0) {
    return;
  }

  connected_clusters<db::NetShape> &clusters = m_net_clusters.clusters_per_cell (net->circuit ()->cell_index ());
  clusters.join_cluster_with (net->cluster_id (), with->cluster_id ());
}

size_t LayoutToNetlist::link_net_to_parent_circuit (const Net *subcircuit_net, Circuit *parent_circuit, const DCplxTrans &dtrans)
{
  if (! subcircuit_net->circuit () || ! has_internal_layout ()
      || ! internal_layout ()->is_valid_cell_index (parent_circuit->cell_index ())
      || subcircuit_net->cluster_id () == 0) {
    return 0;
  }

  db::CplxTrans dbu_trans (internal_layout ()->dbu ());
  db::ICplxTrans trans = dbu_trans.inverted () * dtrans * dbu_trans;

  connected_clusters<db::NetShape> &parent_net_clusters = m_net_clusters.clusters_per_cell (parent_circuit->cell_index ());

  size_t id = parent_net_clusters.insert_dummy ();

  parent_net_clusters.add_connection (id, db::ClusterInstance (subcircuit_net->cluster_id (), subcircuit_net->circuit ()->cell_index (), trans, 0));
  return id;
}

void LayoutToNetlist::ensure_netlist ()
{
  if (! mp_netlist.get ()) {
    mp_netlist.reset (new db::Netlist (this));
  }
}

void LayoutToNetlist::extract_devices (db::NetlistDeviceExtractor &extractor, const std::map<std::string, db::ShapeCollection *> &layers)
{
  if (m_netlist_extracted) {
    throw tl::Exception (tl::to_string (tr ("The netlist has already been extracted")));
  }
  ensure_netlist ();
  extractor.extract (dss (), m_layout_index, layers, *mp_netlist, m_net_clusters, m_device_scaling);
}

void LayoutToNetlist::connect (const db::Region &l)
{
  if (m_netlist_extracted) {
    throw tl::Exception (tl::to_string (tr ("The netlist has already been extracted")));
  }

  if (! is_persisted (l)) {
    register_layer (l, make_new_name ());
  }

  //  we need to keep a reference, so we can safely delete the region
  db::DeepLayer dl = deep_layer_of (l);
  m_dlrefs.insert (dl);

  m_conn.connect (dl.layer ());
}

void LayoutToNetlist::connect_impl (const db::ShapeCollection &a, const db::ShapeCollection &b)
{
  if (m_netlist_extracted) {
    throw tl::Exception (tl::to_string (tr ("The netlist has already been extracted")));
  }
  if (! is_persisted (a)) {
    register_layer (a, make_new_name ());
  }
  if (! is_persisted (b)) {
    register_layer (b, make_new_name ());
  }

  //  we need to keep a reference, so we can safely delete the region
  db::DeepLayer dla = deep_layer_of (a);
  db::DeepLayer dlb = deep_layer_of (b);
  m_dlrefs.insert (dla);
  m_dlrefs.insert (dlb);

  m_conn.connect (dla.layer (), dlb.layer ());
}

void LayoutToNetlist::connect_global_impl (const db::ShapeCollection &l, const std::string &gn)
{
  if (m_netlist_extracted) {
    throw tl::Exception (tl::to_string (tr ("The netlist has already been extracted")));
  }
  if (! is_persisted (l)) {
    register_layer (l, make_new_name ());
  }

  //  we need to keep a reference, so we can safely delete the region
  db::DeepLayer dl = deep_layer_of (l);
  m_dlrefs.insert (dl);

  m_conn.connect_global (dl.layer (), gn);
}

const std::string &LayoutToNetlist::global_net_name (size_t id) const
{
  return m_conn.global_net_name (id);
}

size_t LayoutToNetlist::global_net_id (const std::string &name)
{
  return m_conn.global_net_id (name);
}

void LayoutToNetlist::extract_netlist (const std::string &joined_net_names, bool include_floating_subcircuits)
{
  extract_netlist (joined_net_names, std::map<std::string, std::string> (), include_floating_subcircuits);
}

void LayoutToNetlist::extract_netlist (const std::string &joined_net_names, const std::map<std::string, std::string> &joined_net_names_per_cell, bool include_floating_subcircuits)
{
  if (m_netlist_extracted) {
    throw tl::Exception (tl::to_string (tr ("The netlist has already been extracted")));
  }
  ensure_netlist ();

  db::NetlistExtractor netex;

  netex.set_joined_net_names (joined_net_names);

  const db::Layout &layout = dss ().layout (m_layout_index);
  for (std::map<std::string, std::string>::const_iterator j = joined_net_names_per_cell.begin (); j != joined_net_names_per_cell.end (); ++j) {
    tl::GlobPattern pat (j->first);
    if (pat.is_const ()) {
      netex.set_joined_net_names (j->first, j->second);
    } else {
      for (db::Layout::const_iterator c = layout.begin (); c != layout.end (); ++c) {
        if (pat.match (layout.cell_name (c->cell_index ()))) {
          netex.set_joined_net_names (layout.cell_name (c->cell_index ()), j->second);
        }
      }
    }
  }

  netex.set_include_floating_subcircuits (include_floating_subcircuits);
  netex.extract_nets (dss (), m_layout_index, m_conn, *mp_netlist, m_net_clusters);

  m_netlist_extracted = true;
}

void LayoutToNetlist::set_netlist_extracted ()
{
  m_netlist_extracted = true;
}

bool LayoutToNetlist::has_internal_layout () const
{
  return mp_dss.get () && mp_dss->is_valid_layout_index (m_layout_index);
}

const db::Layout *LayoutToNetlist::internal_layout () const
{
  ensure_layout ();
  return &dss ().const_layout (m_layout_index);
}

const db::Cell *LayoutToNetlist::internal_top_cell () const
{
  ensure_layout ();
  return &dss ().const_initial_cell (m_layout_index);
}

db::Layout *LayoutToNetlist::internal_layout ()
{
  ensure_layout ();
  return &dss ().layout (m_layout_index);
}

db::Cell *LayoutToNetlist::internal_top_cell ()
{
  ensure_layout ();
  return &dss ().initial_cell (m_layout_index);
}

void LayoutToNetlist::ensure_layout () const
{
  if (! dss ().is_valid_layout_index (m_layout_index)) {

    LayoutToNetlist *non_const_this = const_cast<LayoutToNetlist *> (this);
    non_const_this->dss ().make_layout (m_layout_index, db::RecursiveShapeIterator ());

    //  the dummy layer acts as a reference holder for the layout
    unsigned int dummy_layer_index = non_const_this->dss ().layout (m_layout_index).insert_layer ();
    non_const_this->m_dummy_layer = db::DeepLayer (& non_const_this->dss (), m_layout_index, dummy_layer_index);

  }
}

std::string LayoutToNetlist::make_new_name (const std::string &stem)
{
  int m = std::numeric_limits<int>::max () / 2 + 1;
  int n = m;

  std::string name;
  while (m > 0) {

    m /= 2;

    name = stem;
    name += std::string ("$");
    name += tl::to_string (n - m);

    if (m_named_regions.find (name) == m_named_regions.end ()) {
      n -= m;
    }

  }

  return name;
}

std::string LayoutToNetlist::name (unsigned int l) const
{
  std::map<unsigned int, std::string>::const_iterator n = m_name_of_layer.find (l);
  if (n != m_name_of_layer.end ()) {
    return n->second;
  } else {
    return std::string ();
  }
}

db::Region *LayoutToNetlist::layer_by_name (const std::string &name)
{
  std::map<std::string, db::DeepLayer>::const_iterator l = m_named_regions.find (name);
  if (l == m_named_regions.end ()) {
    return 0;
  } else {
    return new db::Region (new db::DeepRegion (l->second));
  }
}

db::Region *LayoutToNetlist::layer_by_index (unsigned int index)
{
  std::map<unsigned int, std::string>::const_iterator n = m_name_of_layer.find (index);
  if (n == m_name_of_layer.end ()) {
    return 0;
  } else {
    return layer_by_name (n->second);
  }
}

static db::DeepLayer dss_create_from_flat (db::DeepShapeStore &dss, const db::ShapeCollection &coll)
{
  const db::Region *region = dynamic_cast<const db::Region *> (&coll);
  const db::Texts *texts = dynamic_cast<const db::Texts *> (&coll);
  if (region) {
    return dss.create_from_flat (*region, true);
  } else if (texts) {
    return dss.create_from_flat (*texts);
  } else {
    tl_assert (false);
  }
}

std::string LayoutToNetlist::name (const ShapeCollection &coll) const
{
  std::map<unsigned int, std::string>::const_iterator n = m_name_of_layer.find (layer_of (coll));
  if (n != m_name_of_layer.end ()) {
    return n->second;
  } else {
    return std::string ();
  }
}

void LayoutToNetlist::register_layer (const ShapeCollection &collection, const std::string &n)
{
  if (m_named_regions.find (n) != m_named_regions.end ()) {
    throw tl::Exception (tl::to_string (tr ("Layer name is already used: ")) + n);
  }

  db::DeepLayer dl;

  if (m_is_flat) {

    dl = dss_create_from_flat (dss (), collection);

  } else {

    db::DeepShapeCollectionDelegateBase *delegate = collection.get_delegate ()->deep ();
    if (! delegate) {

      dl = dss_create_from_flat (dss (), collection);

    } else {

      if (is_persisted (collection)) {
        std::string prev_name = name (collection);
        m_named_regions.erase (prev_name);
      }

      dl = delegate->deep_layer ();

    }

  }

  m_named_regions [n] = dl;
  m_name_of_layer [dl.layer ()] = n;
}

db::DeepLayer LayoutToNetlist::deep_layer_of (const db::ShapeCollection &coll) const
{
  const db::DeepShapeCollectionDelegateBase *dr = coll.get_delegate ()->deep ();
  if (! dr) {

    std::pair<bool, db::DeepLayer> lff = dss ().layer_for_flat (coll);
    if (lff.first) {
      return lff.second;
    } else {
      throw (tl::Exception (tl::to_string (tr ("Non-hierarchical layers cannot be used in netlist extraction"))));
    }

  } else {
    return dr->deep_layer ();
  }
}

db::CellMapping LayoutToNetlist::make_cell_mapping_into (db::Layout &layout, db::Cell &cell, const std::vector<const db::Net *> *nets, bool with_device_cells)
{
  std::set<db::cell_index_type> device_cells;
  if (! with_device_cells && mp_netlist.get ()) {
    for (db::Netlist::device_abstract_iterator i = mp_netlist->begin_device_abstracts (); i != mp_netlist->end_device_abstracts (); ++i) {
      device_cells.insert (i->cell_index ());
    }
  }

  std::set<db::cell_index_type> net_cells;
  if (nets) {
    //  Compute the "included cell" list for cell_mapping_to_original: these are all cells which
    //  are required to represent the net hierarchically.
    for (std::vector<const db::Net *>::const_iterator n = nets->begin (); n != nets->end (); ++n) {
      const db::Net *net = *n;
      db::cell_index_type net_cell = net->circuit ()->cell_index ();
      if (net_cells.find (net_cell) == net_cells.end ()) {
        net_cells.insert (net_cell);
        internal_layout()->cell (net_cell).collect_caller_cells (net_cells);
      }
    }
  }

  return dss ().cell_mapping_to_original (m_layout_index, &layout, cell.cell_index (), &device_cells, nets ? &net_cells : 0);
}

db::CellMapping LayoutToNetlist::cell_mapping_into (db::Layout &layout, db::Cell &cell, const std::vector<const db::Net *> &nets, bool with_device_cells)
{
  return make_cell_mapping_into (layout, cell, &nets, with_device_cells);
}

db::CellMapping LayoutToNetlist::cell_mapping_into (db::Layout &layout, db::Cell &cell, bool with_device_cells)
{
  return make_cell_mapping_into (layout, cell, 0, with_device_cells);
}

db::CellMapping LayoutToNetlist::const_cell_mapping_into (const db::Layout &layout, const db::Cell &cell)
{
  db::CellMapping cm;
  if (layout.cells () == 1) {
    cm.create_single_mapping (layout, cell.cell_index (), *internal_layout(), internal_top_cell()->cell_index ());
  } else {
    cm.create_from_geometry (layout, cell.cell_index (), *internal_layout(), internal_top_cell()->cell_index ());
  }
  return cm;
}

std::map<unsigned int, const db::Region *>
LayoutToNetlist::create_layermap (db::Layout &target_layout, int ln) const
{
  std::map<unsigned int, const db::Region *> lm;
  if (! internal_layout ()) {
    return lm;
  }

  const db::Layout &source_layout = *internal_layout ();

  std::set<unsigned int> layers_to_copy;
  const db::Connectivity &conn = connectivity ();
  for (db::Connectivity::layer_iterator layer = conn.begin_layers (); layer != conn.end_layers (); ++layer) {
    layers_to_copy.insert (*layer);
  }

  for (std::set<unsigned int>::const_iterator l = layers_to_copy.begin (); l != layers_to_copy.end (); ++l) {
    const db::LayerProperties &lp = source_layout.get_properties (*l);
    unsigned int tl;
    if (! lp.is_null ()) {
      tl = target_layout.insert_layer (lp);
    } else {
      tl = target_layout.insert_layer (db::LayerProperties (ln++, 0, name (*l)));
    }
    lm.insert (std::make_pair (tl, const_cast<LayoutToNetlist *> (this)->layer_by_index (*l)));
  }

  return lm;
}

db::Netlist *LayoutToNetlist::netlist () const
{
  return mp_netlist.get ();
}

db::Netlist *LayoutToNetlist::make_netlist ()
{
  ensure_netlist ();
  return mp_netlist.get ();
}

namespace
{
  struct StopOnFirst { };
}

template <class Tr>
static bool deliver_shape (const db::NetShape &, StopOnFirst, const Tr &, db::properties_id_type)
{
  return false;
}

template <class Tr>
static bool deliver_shape (const db::NetShape &s, db::Region &region, const Tr &tr, db::properties_id_type /*propid*/)
{
  if (s.type () == db::NetShape::Polygon) {

    db::PolygonRef pr = s.polygon_ref ();

    if (pr.obj ().is_box ()) {
      region.insert (pr.obj ().box ().transformed (pr.trans ()).transformed (tr));
    } else {
      region.insert (pr.obj ().transformed (pr.trans ()).transformed (tr));
    }

  }

  return true;
}

template <class Tr>
static bool deliver_shape (const db::NetShape &s, db::Shapes &shapes, const Tr &tr, db::properties_id_type propid)
{
  if (s.type () == db::NetShape::Polygon) {

    db::PolygonRef pr = s.polygon_ref ();

    if (pr.obj ().is_box ()) {
      if (propid) {
        shapes.insert (db::BoxWithProperties (pr.obj ().box ().transformed (pr.trans ()).transformed (tr), propid));
      } else {
        shapes.insert (pr.obj ().box ().transformed (pr.trans ()).transformed (tr));
      }
    } else {
      db::Layout *layout = shapes.layout ();
      if (layout) {
        db::PolygonRef polygon_ref (pr.obj ().transformed (pr.trans ()).transformed (tr), layout->shape_repository ());
        if (propid) {
          shapes.insert (db::PolygonRefWithProperties (polygon_ref, propid));
        } else {
          shapes.insert (polygon_ref);
        }
      } else {
        db::Polygon polygon (pr.obj ().transformed (pr.trans ()).transformed (tr));
        if (propid) {
          shapes.insert (db::PolygonWithProperties (polygon, propid));
        } else {
          shapes.insert (polygon);
        }
      }
    }

  } else if (s.type () == db::NetShape::Text) {

    db::TextRef pr = s.text_ref ();

    db::Layout *layout = shapes.layout ();
    if (layout) {
      db::TextRef text_ref (pr.obj ().transformed (pr.trans ()).transformed (tr), layout->shape_repository ());
      if (propid) {
        shapes.insert (db::TextRefWithProperties (text_ref, propid));
      } else {
        shapes.insert (text_ref);
      }
    } else {
      db::Text text (pr.obj ().transformed (pr.trans ()).transformed (tr));
      if (propid) {
        shapes.insert (db::TextWithProperties (text, propid));
      } else {
        shapes.insert (text);
      }
    }

  }

  return true;
}

template <class To, class Shape>
static bool deliver_shapes_of_net_recursive (const db::Netlist * /*nl*/, const db::hier_clusters<Shape> &clusters, db::cell_index_type ci, size_t cid, unsigned int layer_id, const db::ICplxTrans &tr, To &to, db::properties_id_type propid)
{
  //  deliver the net shapes
  for (db::recursive_cluster_shape_iterator<Shape> rci (clusters, layer_id, ci, cid); !rci.at_end (); ++rci) {
    if (! deliver_shape (*rci, to, tr * rci.trans (), propid)) {
      return false;
    }
  }
  return true;
}

template <class To, class Shape>
static bool deliver_shapes_of_net (bool recursive, const db::Netlist *nl, const db::hier_clusters<Shape> &clusters, db::cell_index_type ci, size_t cid, const std::map<unsigned int, To *> &lmap, const db::ICplxTrans &tr, db::properties_id_type propid)
{
  //  shortcut
  if (lmap.empty ()) {
    return true;
  }

  const db::connected_clusters<Shape> &cc = clusters.clusters_per_cell (ci);
  const db::local_cluster<Shape> &lc = cc.cluster_by_id (cid);

  for (typename std::map<unsigned int, To *>::const_iterator l = lmap.begin (); l != lmap.end (); ++l) {
    for (typename db::local_cluster<Shape>::shape_iterator s = lc.begin (l->first); ! s.at_end (); ++s) {
      if (! deliver_shape (*s, *l->second, tr, propid)) {
        return false;
      }
    }
  }

  const typename db::connected_clusters<Shape>::connections_type &conn = cc.connections_for_cluster (cid);
  for (typename db::connected_clusters<Shape>::connections_type::const_iterator c = conn.begin (); c != conn.end (); ) {

    db::cell_index_type cci = c->inst_cell_index ();
    if (! recursive && (! nl || nl->circuit_by_cell_index (cci) || nl->device_abstract_by_cell_index (cci))) {
      //  skip this cell in non-recursive mode (and all following instances of the same cell too)
      typename db::connected_clusters<Shape>::connections_type::const_iterator cc = c;
      while (++cc != conn.end ()) {
        if (cc->inst_cell_index () != cci) {
          break;
        }
      }
      c = cc;
      continue;
    }

    if (! deliver_shapes_of_net (recursive, nl, clusters, cci, c->id (), lmap, tr * c->inst_trans (), propid)) {
      return false;
    }
    ++c;

  }

  return true;
}

void LayoutToNetlist::shapes_of_net (const db::Net &net, const db::Region &of_layer, bool recursive, db::Shapes &to, db::properties_id_type propid) const
{
  unsigned int lid = layer_of (of_layer);
  const db::Circuit *circuit = net.circuit ();
  tl_assert (circuit != 0);

  std::map<unsigned int, db::Shapes *> lmap;
  lmap [lid] = &to;

  deliver_shapes_of_net (recursive, mp_netlist.get (), m_net_clusters, circuit->cell_index (), net.cluster_id (), lmap, db::ICplxTrans (), propid);
}

db::Region *LayoutToNetlist::shapes_of_net (const db::Net &net, const db::Region &of_layer, bool recursive) const
{
  unsigned int lid = layer_of (of_layer);
  const db::Circuit *circuit = net.circuit ();
  tl_assert (circuit != 0);

  std::auto_ptr<db::Region> res (new db::Region ());
  std::map<unsigned int, db::Region *> lmap;
  lmap [lid] = res.get ();

  deliver_shapes_of_net (recursive, mp_netlist.get (), m_net_clusters, circuit->cell_index (), net.cluster_id (), lmap, db::ICplxTrans (), 0);

  return res.release ();
}

void
LayoutToNetlist::build_net_rec (const db::Net &net, db::Layout &target, db::Cell &target_cell, const std::map<unsigned int, const db::Region *> &lmap, const char *net_cell_name_prefix, db::properties_id_type netname_propid, BuildNetHierarchyMode hier_mode, const char *cell_name_prefix, const char *device_cell_name_prefix, cell_reuse_table_type &reuse_table, const db::ICplxTrans &tr) const
{
  const db::Circuit *circuit = net.circuit ();
  tl_assert (circuit != 0);

  build_net_rec (circuit->cell_index (), net.cluster_id (), target, target_cell, lmap, &net, net_cell_name_prefix, netname_propid, hier_mode, cell_name_prefix, device_cell_name_prefix, reuse_table, tr);
}

void
LayoutToNetlist::build_net_rec (db::cell_index_type ci, size_t cid, db::Layout &target, db::Cell &tc, const std::map<unsigned int, const db::Region *> &lmap, const db::Net *net, const char *net_cell_name_prefix, db::properties_id_type netname_propid, BuildNetHierarchyMode hier_mode, const char *circuit_cell_name_prefix, const char *device_cell_name_prefix, cell_reuse_table_type &reuse_table, const db::ICplxTrans &tr) const
{
  db::Cell *target_cell = &tc;

  if (net_cell_name_prefix) {

    const db::connected_clusters<db::NetShape> &ccl = m_net_clusters.clusters_per_cell (ci);

    bool any_connections = circuit_cell_name_prefix && ! ccl.connections_for_cluster (cid).empty ();
    if (! any_connections) {

      StopOnFirst sof;
      std::map<unsigned int, StopOnFirst *> sof_lmap;
      for (std::map<unsigned int, const db::Region *>::const_iterator l = lmap.begin (); l != lmap.end (); ++l) {
        if (l->second) {
          sof_lmap.insert (std::make_pair (layer_of (*l->second), &sof));
        }
      }

      bool consider_cell = ! deliver_shapes_of_net (hier_mode == BNH_Flatten, mp_netlist.get (), m_net_clusters, ci, cid, sof_lmap, tr, 0);
      if (! consider_cell) {
        //  shortcut if cell is empty -> no net cell will be produced
        return;
      }

    }

    //  make a specific cell for the net if requested

    target_cell = &target.cell (target.add_cell ((std::string (net_cell_name_prefix) + net->expanded_name ()).c_str ()));
    tc.insert (db::CellInstArray (db::CellInst (target_cell->cell_index ()), db::Trans ()));

  }

  std::map<unsigned int, db::Shapes *> target_lmap;
  for (std::map<unsigned int, const db::Region *>::const_iterator l = lmap.begin (); l != lmap.end (); ++l) {
    if (l->second) {
      target_lmap.insert (std::make_pair (layer_of (*l->second), &target_cell->shapes (l->first)));
    }
  }

  deliver_shapes_of_net (hier_mode == BNH_Flatten, mp_netlist.get (), m_net_clusters, ci, cid, target_lmap, tr, netname_propid);

  if (hier_mode != BNH_SubcircuitCells && ! device_cell_name_prefix) {
    return;
  }

  //  NOTE: we propagate the magnification part of tr down, but keep the rotation/translation part in the instance
  //  (we want to avoid magnified instances)
  db::ICplxTrans tr_wo_mag = tr * db::ICplxTrans (1.0 / tr.mag ());
  db::ICplxTrans tr_mag (tr.mag ());

  const db::connected_clusters<db::NetShape> &clusters = m_net_clusters.clusters_per_cell (ci);
  typedef db::connected_clusters<db::NetShape>::connections_type connections_type;
  const connections_type &connections = clusters.connections_for_cluster (cid);
  for (connections_type::const_iterator c = connections.begin (); c != connections.end (); ++c) {

    db::cell_index_type subci = c->inst_cell_index ();
    size_t subcid = c->id ();

    CellReuseTableKey cmap_key (subci, netname_propid, subcid);

    cell_reuse_table_type::const_iterator cm = reuse_table.find (cmap_key);
    if (cm == reuse_table.end ()) {

      const char *name_prefix = 0;
      if (mp_netlist->device_abstract_by_cell_index (subci)) {
        name_prefix = device_cell_name_prefix;
      } else {
        name_prefix = circuit_cell_name_prefix;
      }

      if (name_prefix) {

        std::string cell_name = internal_layout ()->cell_name (subci);

        db::cell_index_type target_ci = target.add_cell ((std::string (name_prefix) + cell_name).c_str ());
        cm = reuse_table.insert (std::make_pair (cmap_key, target_ci)).first;

        build_net_rec (subci, subcid, target, target.cell (target_ci), lmap, 0, 0, netname_propid, hier_mode, circuit_cell_name_prefix, device_cell_name_prefix, reuse_table, tr_mag);

      } else {
        cm = reuse_table.insert (std::make_pair (cmap_key, std::numeric_limits<db::cell_index_type>::max ())).first;
      }

    }

    if (cm->second != std::numeric_limits<db::cell_index_type>::max ()) {
      db::CellInstArray ci (db::CellInst (cm->second), tr_wo_mag * c->inst_trans ());
      ci.transform_into (tr_mag);
      target_cell->insert (ci);
    }

  }
}

db::properties_id_type
LayoutToNetlist::make_netname_propid (db::Layout &ly, const tl::Variant &netname_prop, const db::Net &net) const
{
  if (! netname_prop.is_nil () || net.begin_properties () != net.end_properties ()) {

    db::PropertiesRepository::properties_set propset;

    //  add the user properties too (TODO: make this configurable?)
    for (db::Net::property_iterator p = net.begin_properties (); p != net.end_properties (); ++p) {
      db::property_names_id_type key_propnameid = ly.properties_repository ().prop_name_id (p->first);
      propset.insert (std::make_pair (key_propnameid, p->second));
    }

    if (! netname_prop.is_nil ()) {
      db::property_names_id_type name_propnameid = ly.properties_repository ().prop_name_id (netname_prop);
      propset.insert (std::make_pair (name_propnameid, tl::Variant (net.expanded_name ())));
    }

    return ly.properties_repository ().properties_id (propset);

  } else {
    return 0;
  }
}

void
LayoutToNetlist::build_net (const db::Net &net, db::Layout &target, db::Cell &target_cell, const std::map<unsigned int, const db::Region *> &lmap, const tl::Variant &netname_prop, BuildNetHierarchyMode hier_mode, const char *cell_name_prefix, const char *device_cell_name_prefix) const
{
  if (! m_netlist_extracted) {
    throw tl::Exception (tl::to_string (tr ("The netlist has not been extracted yet")));
  }

  cell_reuse_table_type cell_reuse_table;

  double mag = internal_layout ()->dbu () / target.dbu ();

  db::properties_id_type netname_propid = make_netname_propid (target, netname_prop, net);
  build_net_rec (net, target, target_cell, lmap, 0, netname_propid, hier_mode, cell_name_prefix, device_cell_name_prefix, cell_reuse_table, db::ICplxTrans (mag));
}

void
LayoutToNetlist::build_all_nets (const db::CellMapping &cmap, db::Layout &target, const std::map<unsigned int, const db::Region *> &lmap, const char *net_cell_name_prefix, const tl::Variant &netname_prop, BuildNetHierarchyMode hier_mode, const char *circuit_cell_name_prefix, const char *device_cell_name_prefix) const
{
  build_nets (0, cmap, target, lmap, net_cell_name_prefix, netname_prop, hier_mode, circuit_cell_name_prefix, device_cell_name_prefix);
}

void
LayoutToNetlist::build_net_rec (const db::Net &net, db::Layout &target, db::cell_index_type circuit_cell, const db::CellMapping &cmap, const std::map<unsigned int, const db::Region *> &lmap, const char *net_cell_name_prefix, db::properties_id_type netname_propid, BuildNetHierarchyMode hier_mode, const char *cell_name_prefix, const char *device_cell_name_prefix, cell_reuse_table_type &reuse_table, const ICplxTrans &tr) const
{
  if (! cmap.has_mapping (circuit_cell)) {

    const db::Cell &cc = internal_layout ()->cell (circuit_cell);

    for (db::Cell::parent_inst_iterator p = cc.begin_parent_insts (); ! p.at_end (); ++p) {

      db::CellInstArray ci = p->child_inst ().cell_inst ();
      for (db::CellInstArray::iterator ia = ci.begin (); ! ia.at_end(); ++ia) {

        db::ICplxTrans tr_parent = ci.complex_trans (*ia) * tr;
        build_net_rec (net, target, p->parent_cell_index (), cmap, lmap, net_cell_name_prefix, netname_propid, hier_mode, cell_name_prefix, device_cell_name_prefix, reuse_table, tr_parent);

      }

    }

  } else {

    double mag = internal_layout ()->dbu () / target.dbu ();

    db::cell_index_type target_ci = cmap.cell_mapping (circuit_cell);
    build_net_rec (net, target, target.cell (target_ci), lmap, net_cell_name_prefix, netname_propid, hier_mode, cell_name_prefix, device_cell_name_prefix, reuse_table, db::ICplxTrans (mag) * tr);

  }
}

void
LayoutToNetlist::build_nets (const std::vector<const db::Net *> *nets, const db::CellMapping &cmap, db::Layout &target, const std::map<unsigned int, const db::Region *> &lmap, const char *net_cell_name_prefix, const tl::Variant &netname_prop, BuildNetHierarchyMode hier_mode, const char *circuit_cell_name_prefix, const char *device_cell_name_prefix) const
{
  if (! m_netlist_extracted) {
    throw tl::Exception (tl::to_string (tr ("The netlist has not been extracted yet")));
  }

  std::set<const db::Net *> net_set;
  if (nets) {
    net_set.insert (nets->begin (), nets->end ());
  }

  cell_reuse_table_type cell_reuse_table;

  const db::Netlist *netlist = mp_netlist.get ();
  for (db::Netlist::const_circuit_iterator c = netlist->begin_circuits (); c != netlist->end_circuits (); ++c) {

    bool is_top_circuit = c->begin_parents () == c->end_parents ();

    for (db::Circuit::const_net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {

      //  exlude local nets in recursive mode except if they are explicitly selected
      if (! nets && hier_mode != BNH_Disconnected && ! is_top_circuit && n->pin_count () > 0) {
        continue;
      }

      if (! nets || net_set.find (n.operator-> ()) != net_set.end ()) {
        db::properties_id_type netname_propid = make_netname_propid (target, netname_prop, *n);
        build_net_rec (*n, target, c->cell_index (), cmap, lmap, net_cell_name_prefix, netname_propid, hier_mode, circuit_cell_name_prefix, device_cell_name_prefix, cell_reuse_table, db::ICplxTrans ());
      }

    }

    if (hier_mode != BNH_Disconnected && ! nets) {

      //  With recursive nets we skip nets in subcircuits which are connected upwards. This means, nets will
      //  get lost if there is no connection to this pin from the outside. Hence we need to deliver nets from
      //  subcircuits as part of the circuit which calls the subcircuit - but NOT in a subcircuit cell, because
      //  this will just apply to nets from certain instances. But the net cell name will be formed as "subcircuit:net"
      //
      //  In explicit selection mode we don't care about this as nets are explicitly taken or not.

      const db::Circuit &circuit = *c;
      for (db::Circuit::const_subcircuit_iterator sc = circuit.begin_subcircuits (); sc != circuit.end_subcircuits (); ++sc) {

        const db::SubCircuit &subcircuit = *sc;
        for (db::Circuit::const_pin_iterator p = subcircuit.circuit_ref ()->begin_pins (); p != subcircuit.circuit_ref ()->end_pins (); ++p) {

          if (! subcircuit.net_for_pin (p->id ())) {

            const db::Net *n = subcircuit.circuit_ref ()->net_for_pin (p->id ());
            if (n) {

              double dbu = target.dbu ();
              db::ICplxTrans tr = db::CplxTrans (dbu).inverted () * subcircuit.trans () * db::CplxTrans (dbu);

              db::properties_id_type netname_propid = make_netname_propid (target, netname_prop, *n);

              if (net_cell_name_prefix) {
                std::string ncn = std::string (net_cell_name_prefix) + subcircuit.expanded_name () + ":";
                build_net_rec (*n, target, c->cell_index (), cmap, lmap, ncn.c_str (), netname_propid, hier_mode, circuit_cell_name_prefix, device_cell_name_prefix, cell_reuse_table, tr);
              } else {
                build_net_rec (*n, target, c->cell_index (), cmap, lmap, net_cell_name_prefix, netname_propid, hier_mode, circuit_cell_name_prefix, device_cell_name_prefix, cell_reuse_table, tr);
              }

            }

          }

        }

      }

    }

  }
}

db::Net *LayoutToNetlist::probe_net (const db::Region &of_region, const db::DPoint &point)
{
  return probe_net (of_region, db::CplxTrans (internal_layout ()->dbu ()).inverted () * point);
}

size_t LayoutToNetlist::search_net (const db::ICplxTrans &trans, const db::Cell *cell, const db::local_cluster<db::NetShape> &test_cluster, std::vector<db::InstElement> &rev_inst_path)
{
  db::Box local_box = trans * test_cluster.bbox ();

  const db::local_clusters<db::NetShape> &lcc = net_clusters ().clusters_per_cell (cell->cell_index ());
  for (db::local_clusters<db::NetShape>::touching_iterator i = lcc.begin_touching (local_box); ! i.at_end (); ++i) {
    const db::local_cluster<db::NetShape> &lc = *i;
    if (lc.interacts (test_cluster, trans, m_conn)) {
      return lc.id ();
    }
  }

  for (db::Cell::touching_iterator i = cell->begin_touching (local_box); ! i.at_end (); ++i) {

    for (db::CellInstArray::iterator ia = i->begin_touching (local_box, internal_layout ()); ! ia.at_end (); ++ia) {

      db::ICplxTrans trans_inst = i->complex_trans (*ia);
      db::ICplxTrans t = trans_inst.inverted () * trans;
      size_t cluster_id = search_net (t, &internal_layout ()->cell (i->cell_index ()), test_cluster, rev_inst_path);
      if (cluster_id > 0) {
        rev_inst_path.push_back (db::InstElement (*i, ia));
        return cluster_id;
      }

    }

  }

  return 0;
}

db::Net *LayoutToNetlist::probe_net (const db::Region &of_region, const db::Point &point)
{
  if (! m_netlist_extracted) {
    throw tl::Exception (tl::to_string (tr ("The netlist has not been extracted yet")));
  }
  tl_assert (mp_netlist.get ());

  db::CplxTrans dbu_trans (internal_layout ()->dbu ());
  db::VCplxTrans dbu_trans_inv = dbu_trans.inverted ();

  unsigned int layer = layer_of (of_region);

  //  Prepare a test cluster
  db::Box box (point - db::Vector (1, 1), point + db::Vector (1, 1));
  db::GenericRepository sr;
  db::local_cluster<db::NetShape> test_cluster;
  test_cluster.add (db::PolygonRef (db::Polygon (box), sr), layer);

  std::vector<db::InstElement> inst_path;

  size_t cluster_id = search_net (db::ICplxTrans (), internal_top_cell (), test_cluster, inst_path);
  if (cluster_id > 0) {

    //  search_net delivers the path in reverse order
    std::reverse (inst_path.begin (), inst_path.end ());

    std::vector<db::cell_index_type> cell_indexes;
    cell_indexes.reserve (inst_path.size () + 1);
    cell_indexes.push_back (internal_top_cell ()->cell_index ());
    for (std::vector<db::InstElement>::const_iterator i = inst_path.begin (); i != inst_path.end (); ++i) {
      cell_indexes.push_back (i->inst_ptr.cell_index ());
    }

    db::Circuit *circuit = 0;
    db::Net *net = 0;

    while (true) {

      circuit = mp_netlist->circuit_by_cell_index (cell_indexes.back ());
      if (circuit) {
        net = circuit->net_by_cluster_id (cluster_id);
        if (net) {
          break;
        }
      }

      //  The net might have been propagated to the parent. So move there.
      if (inst_path.empty ()) {
        return 0;
      }

      db::ClusterInstance ci (cluster_id, inst_path.back ());

      cell_indexes.pop_back ();
      inst_path.pop_back ();

      cluster_id = m_net_clusters.clusters_per_cell (cell_indexes.back ()).find_cluster_with_connection (ci);

      //  no parent cluster found
      if (cluster_id == 0) {
        return 0;
      }

    }

    //  follow the path up in the net hierarchy using the transformation and the upper cell index as the
    //  guide line
    while (! inst_path.empty () && net->pin_count () > 0) {

      cell_indexes.pop_back ();

      const db::Pin *pin = circuit->pin_by_id (net->begin_pins ()->pin_id ());
      tl_assert (pin != 0);

      db::DCplxTrans dtrans = dbu_trans * inst_path.back ().complex_trans () * dbu_trans_inv;

      //  try to find a parent circuit which connects to this net
      db::Circuit *upper_circuit = 0;
      db::Net *upper_net = 0;
      for (db::Circuit::refs_iterator r = circuit->begin_refs (); r != circuit->end_refs () && ! upper_net; ++r) {
        if (r->trans ().equal (dtrans) && r->circuit () && r->circuit ()->cell_index () == cell_indexes.back ()) {
          upper_net = r->net_for_pin (pin->id ());
          upper_circuit = r->circuit ();
        }
      }

      if (upper_net) {
        circuit = upper_circuit;
        net = upper_net;
        inst_path.pop_back ();
      } else {
        break;
      }

    }

    return net;

  } else {
    return 0;
  }
}

db::Region LayoutToNetlist::antenna_check (const db::Region &gate, double gate_perimeter_factor, const db::Region &metal, double metal_perimeter_factor, double ratio, const std::vector<std::pair<const db::Region *, double> > &diodes)
{
  //  TODO: that's basically too much .. we only need the clusters
  if (! m_netlist_extracted) {
    throw tl::Exception (tl::to_string (tr ("The netlist has not been extracted yet")));
  }

  db::Layout &ly = dss ().layout (m_layout_index);
  double dbu = ly.dbu ();

  db::DeepLayer dl (&dss (), m_layout_index, ly.insert_layer ());

  for (db::Layout::bottom_up_const_iterator cid = ly.begin_bottom_up (); cid != ly.end_bottom_up (); ++cid) {

    const connected_clusters<db::NetShape> &clusters = m_net_clusters.clusters_per_cell (*cid);
    if (clusters.empty ()) {
      continue;
    }

    for (connected_clusters<db::NetShape>::all_iterator c = clusters.begin_all (); ! c.at_end (); ++c) {

      if (! clusters.is_root (*c)) {
        continue;
      }

      db::Region rgate, rmetal;

      deliver_shapes_of_net_recursive (0, m_net_clusters, *cid, *c, layer_of (gate), db::ICplxTrans (), rgate, 0);
      deliver_shapes_of_net_recursive (0, m_net_clusters, *cid, *c, layer_of (metal), db::ICplxTrans (), rmetal, 0);

      double agate = rgate.area () * dbu * dbu;
      if (fabs (gate_perimeter_factor) > 1e-6) {
        agate += rgate.perimeter () * dbu * gate_perimeter_factor;
      }

      double ametal = rmetal.area () * dbu * dbu;
      if (fabs (metal_perimeter_factor) > 1e-6) {
        ametal += rmetal.perimeter () * dbu * metal_perimeter_factor;
      }

      double r = ratio;
      bool skip = false;

      for (std::vector<std::pair<const db::Region *, double> >::const_iterator d = diodes.begin (); d != diodes.end () && ! skip; ++d) {

        db::Region rdiode;
        deliver_shapes_of_net_recursive (0, m_net_clusters, *cid, *c, layer_of (*d->first), db::ICplxTrans (), rdiode, 0);

        if (fabs (d->second) < db::epsilon) {
          if (rdiode.area () > 0) {
            skip = true;
          }
        } else {
          r += rdiode.area () * dbu * dbu * d->second;
        }

      }

      if (! skip) {

        if (tl::verbosity () >= 50) {
          tl::info << "cell [" << ly.cell_name (*cid) << "]: agate=" << tl::to_string (agate) << ", ametal=" << tl::to_string (ametal) << ", r=" << tl::sprintf ("%.12g", r);
        }

        if (agate > dbu * dbu && ametal / agate > r + db::epsilon) {
          db::Shapes &shapes = ly.cell (*cid).shapes (dl.layer ());
          for (db::Region::const_iterator r = rmetal.begin_merged (); ! r.at_end (); ++r) {
            shapes.insert (*r);
          }
        }

      }

    }

  }

  return db::Region (new db::DeepRegion (dl));
}


void LayoutToNetlist::save (const std::string &path, bool short_format)
{
  tl::OutputStream stream (path);
  db::LayoutToNetlistStandardWriter writer (stream, short_format);
  set_filename (path);
  writer.write (this);
}

void LayoutToNetlist::load (const std::string &path)
{
  tl::InputStream stream (path);
  db::LayoutToNetlistStandardReader reader (stream);
  set_filename (path);
  set_name (stream.filename ());
  reader.read (this);
}

db::LayoutToNetlist *LayoutToNetlist::create_from_file (const std::string &path)
{
  std::auto_ptr<db::LayoutToNetlist> db;

  //  TODO: generic concept to detect format
  std::string first_line;
  {
    tl::InputStream stream (path);
    tl::TextInputStream text_stream (stream);
    first_line = text_stream.get_line ();
  }

  if (first_line.find (db::lvs_std_format::keys<false>::lvs_magic_string) == 0) {
    db::LayoutVsSchematic *lvs_db = new db::LayoutVsSchematic ();
    db.reset (lvs_db);
    lvs_db->load (path);
  } else {
    db.reset (new db::LayoutToNetlist ());
    db->load (path);
  }

  return db.release ();
}

void LayoutToNetlist::set_generator (const std::string &g)
{
  m_generator = g;
}

}
