

/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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
#include "dbShapeRepository.h"
#include "dbCellMapping.h"

namespace db
{

static bool is_deep (const db::Region &r)
{
  return dynamic_cast<const db::DeepRegion *> (r.delegate ()) != 0;
}

//  the iterator provides the hierarchical selection (enabling/disabling cells etc.)
LayoutToNetlist::LayoutToNetlist (const db::RecursiveShapeIterator &iter)
  : m_iter (iter), m_netlist_extracted (false)
{
  //  check the iterator
  if (iter.has_complex_region () || iter.region () != db::Box::world ()) {
    throw tl::Exception (tl::to_string (tr ("The netlist extractor cannot work on clipped layouts")));
  }

  m_dss.set_text_enlargement (1);
  m_dss.set_text_property_name (tl::Variant ("LABEL"));
}

void LayoutToNetlist::set_threads (int n)
{
  m_dss.set_threads (n);
}

int LayoutToNetlist::threads () const
{
  return m_dss.threads ();
}

void LayoutToNetlist::set_area_ratio (double ar)
{
  m_dss.set_max_area_ratio (ar);
}

double LayoutToNetlist::area_ratio () const
{
  return m_dss.max_area_ratio ();
}

void LayoutToNetlist::set_max_vertex_count (size_t n)
{
  m_dss.set_max_vertex_count (n);
}

size_t LayoutToNetlist::max_vertex_count () const
{
  return m_dss.max_vertex_count ();
}

db::Region *LayoutToNetlist::make_layer (unsigned int layer_index)
{
  db::RecursiveShapeIterator si (m_iter);
  si.set_layer (layer_index);
  si.shape_flags (db::ShapeIterator::All);
  return new db::Region (si, m_dss);
}

db::Region *LayoutToNetlist::make_text_layer (unsigned int layer_index)
{
  db::RecursiveShapeIterator si (m_iter);
  si.set_layer (layer_index);
  si.shape_flags (db::ShapeIterator::Texts);
  return new db::Region (si, m_dss);
}

db::Region *LayoutToNetlist::make_polygon_layer (unsigned int layer_index)
{
  db::RecursiveShapeIterator si (m_iter);
  si.set_layer (layer_index);
  si.shape_flags (db::ShapeIterator::Paths | db::ShapeIterator::Polygons | db::ShapeIterator::Boxes);
  return new db::Region (si, m_dss);
}

void LayoutToNetlist::extract_devices (db::NetlistDeviceExtractor &extractor, const std::map<std::string, db::Region *> &layers)
{
  if (m_netlist_extracted) {
    throw tl::Exception (tl::to_string (tr ("The netlist has already been extracted")));
  }
  if (! mp_netlist.get ()) {
    mp_netlist.reset (new db::Netlist ());
  }
  extractor.extract(m_dss, layers, *mp_netlist, m_net_clusters);
}

void LayoutToNetlist::connect (const db::Region &l)
{
  if (m_netlist_extracted) {
    throw tl::Exception (tl::to_string (tr ("The netlist has already been extracted")));
  }
  if (! is_deep (l)) {
    throw (tl::Exception (tl::to_string (tr ("Non-hierarchical layers cannot be used in intra-layer connectivity for netlist extraction"))));
  }

  //  we need to keep a reference, so we can safely delete the region
  db::DeepLayer dl (l);
  m_dlrefs.insert (dl);

  m_conn.connect (dl.layer ());
}

void LayoutToNetlist::connect (const db::Region &a, const db::Region &b)
{
  if (m_netlist_extracted) {
    throw tl::Exception (tl::to_string (tr ("The netlist has already been extracted")));
  }
  if (! is_deep (a)) {
    throw (tl::Exception (tl::to_string (tr ("Non-hierarchical layers cannot be used in inter-layer connectivity (first layer) for netlist extraction"))));
  }
  if (! is_deep (b)) {
    throw (tl::Exception (tl::to_string (tr ("Non-hierarchical layers cannot be used in inter-layer connectivity (second layer) for netlist extraction"))));
  }

  //  we need to keep a reference, so we can safely delete the region
  db::DeepLayer dla (a), dlb (b);
  m_dlrefs.insert (dla);
  m_dlrefs.insert (dlb);

  m_conn.connect (dla.layer (), dlb.layer ());
}

size_t LayoutToNetlist::connect_global (const db::Region &l, const std::string &gn)
{
  if (m_netlist_extracted) {
    throw tl::Exception (tl::to_string (tr ("The netlist has already been extracted")));
  }
  if (! is_deep (l)) {
    throw (tl::Exception (tl::to_string (tr ("Non-hierarchical layers cannot be used in intra-layer connectivity for netlist extraction"))));
  }

  //  we need to keep a reference, so we can safely delete the region
  db::DeepLayer dl (l);
  m_dlrefs.insert (dl);

  return m_conn.connect_global (dl.layer (), gn);
}

const std::string &LayoutToNetlist::global_net_name (size_t id) const
{
  return m_conn.global_net_name (id);
}

size_t LayoutToNetlist::global_net_id (const std::string &name)
{
  return m_conn.global_net_id (name);
}

void LayoutToNetlist::extract_netlist ()
{
  if (m_netlist_extracted) {
    throw tl::Exception (tl::to_string (tr ("The netlist has already been extracted")));
  }
  if (! mp_netlist.get ()) {
    mp_netlist.reset (new db::Netlist ());
  }

  m_net_clusters.clear ();

  db::NetlistExtractor netex;
  netex.extract_nets(m_dss, m_conn, *mp_netlist, m_net_clusters);

  m_netlist_extracted = true;
}

const db::Layout *LayoutToNetlist::internal_layout () const
{
  return &m_dss.const_layout ();
}

const db::Cell *LayoutToNetlist::internal_top_cell () const
{
  return &m_dss.const_initial_cell ();
}

unsigned int LayoutToNetlist::layer_of (const db::Region &region) const
{
  const db::DeepRegion *dr = dynamic_cast<const db::DeepRegion *> (region.delegate ());
  if (! dr) {
    throw (tl::Exception (tl::to_string (tr ("Non-hierarchical layers cannot be used in netlist extraction"))));
  }
  return dr->deep_layer ().layer ();
}

db::CellMapping LayoutToNetlist::cell_mapping_into (db::Layout &layout, db::Cell &cell, bool with_device_cells)
{
  unsigned int layout_index = 0;

  std::set<db::cell_index_type> device_cells;

  if (! with_device_cells) {
    const db::Layout &src_layout = m_dss.layout (layout_index);
    for (db::Layout::const_iterator c = src_layout.begin (); c != src_layout.end (); ++c) {
      if (db::NetlistDeviceExtractor::is_device_cell (src_layout, c->cell_index ())) {
        device_cells.insert (c->cell_index ());
      }
    }
  }

  return m_dss.cell_mapping_to_original (layout_index, &layout, cell.cell_index (), &device_cells);
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

db::Netlist *LayoutToNetlist::netlist () const
{
  return mp_netlist.get ();
}

template <class Tr>
static void deliver_shape (const db::PolygonRef &pr, db::Region &region, const Tr &tr)
{
  if (pr.obj ().is_box ()) {
    region.insert (pr.obj ().box ().transformed (pr.trans ()).transformed (tr));
  } else {
    region.insert (pr.obj ().transformed (pr.trans ()).transformed (tr));
  }
}

template <class Tr>
static void deliver_shape (const db::PolygonRef &pr, db::Shapes &shapes, const Tr &tr)
{
  if (pr.obj ().is_box ()) {
    shapes.insert (pr.obj ().box ().transformed (pr.trans ()).transformed (tr));
  } else {
    db::Layout *layout = shapes.layout ();
    if (layout) {
      shapes.insert (db::PolygonRef (pr.obj ().transformed (pr.trans ()).transformed (tr), layout->shape_repository ()));
    } else {
      shapes.insert (pr.obj ().transformed (pr.trans ()).transformed (tr));
    }
  }
}

template <class To>
static void deliver_shapes_of_net_recursive (const db::hier_clusters<db::PolygonRef> &clusters, db::cell_index_type ci, size_t cid, unsigned int layer_id, To &to)
{
  //  deliver the net shapes
  for (db::recursive_cluster_shape_iterator<db::PolygonRef> rci (clusters, layer_id, ci, cid); !rci.at_end (); ++rci) {
    deliver_shape (*rci, to, rci.trans ());
  }
}

template <class To>
static void deliver_shapes_of_net_nonrecursive (const db::hier_clusters<db::PolygonRef> &clusters, db::cell_index_type ci, size_t cid, unsigned int layer_id, To &to)
{
  const db::local_cluster<db::PolygonRef> &lc = clusters.clusters_per_cell (ci).cluster_by_id (cid);

  for (db::local_cluster<db::PolygonRef>::shape_iterator s = lc.begin (layer_id); !s.at_end (); ++s) {
    deliver_shape (*s, to, db::UnitTrans ());
  }
}

void LayoutToNetlist::shapes_of_net (const db::Net &net, const db::Region &of_layer, bool recursive, db::Shapes &to) const
{
  unsigned int lid = layer_of (of_layer);
  const db::Circuit *circuit = net.circuit ();
  tl_assert (circuit != 0);

  if (! recursive) {
    deliver_shapes_of_net_nonrecursive (m_net_clusters, circuit->cell_index (), net.cluster_id (), lid, to);
  } else {
    deliver_shapes_of_net_recursive (m_net_clusters, circuit->cell_index (), net.cluster_id (), lid, to);
  }
}

db::Region *LayoutToNetlist::shapes_of_net (const db::Net &net, const db::Region &of_layer, bool recursive) const
{
  unsigned int lid = layer_of (of_layer);
  const db::Circuit *circuit = net.circuit ();
  tl_assert (circuit != 0);

  std::auto_ptr<db::Region> res (new db::Region ());

  if (! recursive) {
    deliver_shapes_of_net_nonrecursive (m_net_clusters, circuit->cell_index (), net.cluster_id (), lid, *res);
  } else {
    deliver_shapes_of_net_recursive (m_net_clusters, circuit->cell_index (), net.cluster_id (), lid, *res);
  }

  return res.release ();
}

void
LayoutToNetlist::build_net_rec (const db::Net &net, db::Layout &target, db::Cell &target_cell, const std::map<unsigned int, const db::Region *> &lmap, const char *cell_name_prefix, const char *device_cell_name_prefix, std::map<std::pair<db::cell_index_type, size_t>, db::cell_index_type> &cmap) const
{
  const db::Circuit *circuit = net.circuit ();
  tl_assert (circuit != 0);

  build_net_rec (circuit->cell_index (), net.cluster_id (), target, target_cell, lmap, cell_name_prefix, device_cell_name_prefix, cmap);
}

void
LayoutToNetlist::build_net_rec (db::cell_index_type ci, size_t cid, db::Layout &target, db::Cell &target_cell, const std::map<unsigned int, const db::Region *> &lmap, const char *cell_name_prefix, const char *device_cell_name_prefix, std::map<std::pair<db::cell_index_type, size_t>, db::cell_index_type> &cmap) const
{
  for (std::map<unsigned int, const db::Region *>::const_iterator l = lmap.begin (); l != lmap.end (); ++l) {
    deliver_shapes_of_net_nonrecursive (m_net_clusters, ci, cid, layer_of (*l->second), target_cell.shapes (l->first));
  }

  if (! cell_name_prefix && ! device_cell_name_prefix) {
    return;
  }

  const db::connected_clusters<db::PolygonRef> &clusters = m_net_clusters.clusters_per_cell (ci);
  typedef db::connected_clusters<db::PolygonRef>::connections_type connections_type;
  const connections_type &connections = clusters.connections_for_cluster (cid);
  for (connections_type::const_iterator c = connections.begin (); c != connections.end (); ++c) {

    db::cell_index_type subci = c->inst ().inst_ptr.cell_index ();
    size_t subcid = c->id ();

    std::map<std::pair<db::cell_index_type, size_t>, db::cell_index_type>::const_iterator cm = cmap.find (std::make_pair (subci, subcid));
    if (cm == cmap.end ()) {

      const char *name_prefix = 0;
      if (db::NetlistDeviceExtractor::is_device_cell (*internal_layout (), subci)) {
        name_prefix = device_cell_name_prefix;
      } else {
        name_prefix = cell_name_prefix;
      }

      if (name_prefix) {

        std::string cell_name = internal_layout ()->cell_name (subci);

        db::cell_index_type target_ci = target.add_cell ((std::string (name_prefix) + cell_name).c_str ());
        cm = cmap.insert (std::make_pair (std::make_pair (subci, subcid), target_ci)).first;

        build_net_rec (subci, subcid, target, target.cell (target_ci), lmap, cell_name_prefix, device_cell_name_prefix, cmap);

      } else {
        cm = cmap.insert (std::make_pair (std::make_pair (subci, subcid), std::numeric_limits<db::cell_index_type>::max ())).first;
      }

    }

    if (cm->second != std::numeric_limits<db::cell_index_type>::max ()) {
      target_cell.insert (db::CellInstArray (db::CellInst (cm->second), c->inst ().complex_trans ()));
    }

  }
}

void
LayoutToNetlist::build_net (const db::Net &net, db::Layout &target, db::Cell &target_cell, const std::map<unsigned int, const db::Region *> &lmap, const char *cell_name_prefix, const char *device_cell_name_prefix) const
{
  if (! m_netlist_extracted) {
    throw tl::Exception (tl::to_string (tr ("The netlist has not been extracted yet")));
  }

  std::map<std::pair<db::cell_index_type, size_t>, db::cell_index_type> cell_map;

  build_net_rec (net, target, target_cell, lmap, cell_name_prefix, device_cell_name_prefix, cell_map);
}

void
LayoutToNetlist::build_all_nets (const db::CellMapping &cmap, db::Layout &target, const std::map<unsigned int, const db::Region *> &lmap, const char *net_cell_name_prefix, const char *circuit_cell_name_prefix, const char *device_cell_name_prefix) const
{
  if (! m_netlist_extracted) {
    throw tl::Exception (tl::to_string (tr ("The netlist has not been extracted yet")));
  }

  std::map<std::pair<db::cell_index_type, size_t>, db::cell_index_type> cell_map;

  const db::Netlist *netlist = mp_netlist.get ();
  for (db::Netlist::const_circuit_iterator c = netlist->begin_circuits (); c != netlist->end_circuits (); ++c) {

    if (! cmap.has_mapping (c->cell_index ())) {
      continue;
    }

    db::cell_index_type target_ci = cmap.cell_mapping (c->cell_index ());

    for (db::Circuit::const_net_iterator n = c->begin_nets (); n != c->end_nets (); ++n) {

      //  exlude local nets in recursive mode
      if (circuit_cell_name_prefix && n->pin_count () > 0) {
        continue;
      }

      const db::connected_clusters<db::PolygonRef> &ccl = m_net_clusters.clusters_per_cell (c->cell_index ());
      const db::local_cluster<db::PolygonRef> &cl = ccl.cluster_by_id (n->cluster_id ());

      bool any_connections = ! ccl.connections_for_cluster (n->cluster_id ()).empty ();

      bool any_shapes = false;
      for (std::map<unsigned int, const db::Region *>::const_iterator m = lmap.begin (); m != lmap.end () && !any_shapes; ++m) {
        any_shapes = ! cl.begin (layer_of (*m->second)).at_end ();
      }

      if (any_shapes || (circuit_cell_name_prefix && any_connections)) {

        db::cell_index_type net_ci = target_ci;

        if (net_cell_name_prefix) {

          db::Cell &tc = target.cell (target_ci);
          net_ci = target.add_cell ((std::string (net_cell_name_prefix) + n->expanded_name ()).c_str ());
          tc.insert (db::CellInstArray (db::CellInst (net_ci), db::Trans ()));

        }

        build_net_rec (*n, target, target.cell (net_ci), lmap, circuit_cell_name_prefix, device_cell_name_prefix, cell_map);

      }

    }

  }
}

db::Net *LayoutToNetlist::probe_net (const db::Region &of_region, const db::DPoint &point)
{
  return probe_net (of_region, db::CplxTrans (internal_layout ()->dbu ()).inverted () * point);
}

size_t LayoutToNetlist::search_net (const db::ICplxTrans &trans, const db::Cell *cell, const db::local_cluster<db::PolygonRef> &test_cluster, std::vector<db::InstElement> &rev_inst_path)
{
  db::Box local_box = trans * test_cluster.bbox ();

  const db::local_clusters<db::PolygonRef> &lcc = net_clusters ().clusters_per_cell (cell->cell_index ());
  for (db::local_clusters<db::PolygonRef>::touching_iterator i = lcc.begin_touching (local_box); ! i.at_end (); ++i) {
    const db::local_cluster<db::PolygonRef> &lc = *i;
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
  db::local_cluster<db::PolygonRef> test_cluster;
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

    db::Circuit *circuit = mp_netlist->circuit_by_cell_index (cell_indexes.back ());
    if (! circuit) {
      //  the circuit has probably been optimized away
      return 0;
    }

    db::Net *net = circuit->net_by_cluster_id (cluster_id);
    if (! net) {
      //  the net has probably been optimized away
      return 0;
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

}
