
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#include "dbNetlistDeviceExtractor.h"
#include "dbRegion.h"
#include "dbHierNetworkProcessor.h"
#include "dbDeepRegion.h"

#include "tlProgress.h"
#include "tlTimer.h"
#include "tlInternational.h"
#include "tlEnv.h"

namespace db
{

// ----------------------------------------------------------------------------------------
//  NetlistDeviceExtractor implementation

NetlistDeviceExtractor::NetlistDeviceExtractor (const std::string &name)
  : mp_layout (0), m_cell_index (0), mp_breakout_cells (0), m_device_scaling (1.0), mp_circuit (0)
{
  //  inspects the KLAYOUT_SMART_DEVICE_PROPAGATION environment variable (if set)
  //  to derive the default value for m_smart_device_propagation.
  static bool s_is_sdp_default_set = false;
  static bool s_sdp_default = false;
  if (! s_is_sdp_default_set) {
    int v = 0;
    std::string ve = tl::get_env ("KLAYOUT_SMART_DEVICE_PROPAGATION", "0");
    tl::Extractor (ve.c_str ()).try_read (v);
    s_sdp_default = (v != 0);
    s_is_sdp_default_set = true;
  }

  m_name = name;
  m_terminal_id_propname_id = 0;
  m_device_class_propname_id = 0;
  m_device_id_propname_id = 0;
  m_smart_device_propagation = s_sdp_default;
  m_pre_extract = false;
}

NetlistDeviceExtractor::~NetlistDeviceExtractor ()
{
  //  .. nothing yet ..
}

const tl::Variant &NetlistDeviceExtractor::terminal_id_property_name ()
{
  static tl::Variant name ("TERMINAL_ID");
  return name;
}

const tl::Variant &NetlistDeviceExtractor::device_id_property_name ()
{
  static tl::Variant name ("DEVICE_ID");
  return name;
}

const tl::Variant &NetlistDeviceExtractor::device_class_property_name ()
{
  static tl::Variant name ("DEVICE_CLASS");
  return name;
}

void NetlistDeviceExtractor::initialize (db::Netlist *nl)
{
  m_layer_definitions.clear ();
  mp_device_class = 0;
  m_device_scaling = 1.0;
  m_terminal_id_propname_id = 0;
  m_device_id_propname_id = 0;
  m_device_class_propname_id = 0;
  m_netlist.reset (nl);

  setup ();
}

static void insert_into_region (const db::NetShape &s, const db::ICplxTrans &tr, db::Region &region)
{
  if (s.type () == db::NetShape::Polygon) {
    db::PolygonRef pr = s.polygon_ref ();
    region.insert (pr.obj ().transformed (tr * db::ICplxTrans (pr.trans ())));
  }
}

void NetlistDeviceExtractor::extract (db::DeepShapeStore &dss, unsigned int layout_index, const NetlistDeviceExtractor::input_layers &layer_map, db::Netlist &nl, hier_clusters_type &clusters, double device_scaling)
{
  initialize (&nl);

  std::vector<unsigned int> layers;
  layers.reserve (m_layer_definitions.size ());

  for (layer_definitions::const_iterator ld = begin_layer_definitions (); ld != end_layer_definitions (); ++ld) {

    size_t ld_index = ld->index;
    input_layers::const_iterator l = layer_map.find (m_layer_definitions [ld_index].name);
    while (l == layer_map.end () && m_layer_definitions [ld_index].fallback_index < m_layer_definitions.size ()) {
      //  try fallback layer
      ld_index = m_layer_definitions [ld_index].fallback_index;
      l = layer_map.find (m_layer_definitions [ld_index].name);
    }

    if (l == layer_map.end ()) {

      //  gets the layer names for the error message
      std::string layer_names = m_layer_definitions [ld_index].name;
      ld_index = ld->index;
      l = layer_map.find (m_layer_definitions [ld_index].name);
      while (l == layer_map.end () && m_layer_definitions [ld_index].fallback_index < m_layer_definitions.size ()) {
        ld_index = m_layer_definitions [ld_index].fallback_index;
        std::string ln = m_layer_definitions [ld_index].name;
        layer_names += "/";
        layer_names += ln;
        l = layer_map.find (ln);
      }

      //  TODO: maybe use empty layers for optional ones?
      throw tl::Exception (tl::sprintf (tl::to_string (tr ("Missing input layer for device extraction (device %s): %s")), name (), layer_names));

    }

    tl_assert (l->second != 0);
    db::DeepShapeCollectionDelegateBase *dr = l->second->get_delegate ()->deep ();
    if (dr == 0) {

      std::pair<bool, db::DeepLayer> alias = dss.layer_for_flat (tl::id_of (l->second->get_delegate ()));
      if (alias.first) {
        //  use deep layer alias for a given flat one (if found)
        layers.push_back (alias.second.layer ());
      } else {
        throw tl::Exception (tl::sprintf (tl::to_string (tr ("Invalid region passed to input layer '%s' for device extraction (device %s): must be of deep region kind")), ld->name, name ()));
      }

    } else {

      if (&dr->deep_layer ().layout () != &dss.layout (layout_index) || &dr->deep_layer ().initial_cell () != &dss.initial_cell (layout_index)) {
        throw tl::Exception (tl::sprintf (tl::to_string (tr ("Invalid region passed to input layer '%s' for device extraction (device %s): not originating from the same source")), ld->name, name ()));
      }

      layers.push_back (dr->deep_layer ().layer ());

    }

  }

  extract_without_initialize (dss.layout (layout_index), dss.initial_cell (layout_index), clusters, layers, device_scaling, dss.breakout_cells (layout_index));
}

void NetlistDeviceExtractor::extract (db::Layout &layout, db::Cell &cell, const std::vector<unsigned int> &layers, db::Netlist *nl, hier_clusters_type &clusters, double device_scaling, const std::set<db::cell_index_type> *breakout_cells)
{
  initialize (nl);
  extract_without_initialize (layout, cell, clusters, layers, device_scaling, breakout_cells);
}

namespace {

struct ExtractorCacheValueType {
  ExtractorCacheValueType () { }
  db::Vector disp;
  tl::vector<db::Device *> devices;
};

}

static db::Vector normalize_device_layer_geometry (std::vector<db::Region> &layer_geometry)
{
  db::Box box;
  for (std::vector<db::Region>::const_iterator g = layer_geometry.begin (); g != layer_geometry.end (); ++g) {
    box += g->bbox ();
  }

  db::Vector disp = box.p1 () - db::Point ();
  for (std::vector<db::Region>::iterator g = layer_geometry.begin (); g != layer_geometry.end (); ++g) {
    g->transform (db::Disp (-disp));
  }

  return disp;
}

static db::Vector get_layer_geometry (std::vector<db::Region> &layer_geometry, const std::vector<unsigned int> &layers, const db::hier_clusters<db::NetShape> &device_clusters, db::cell_index_type ci, db::connected_clusters<db::NetShape>::id_type cid)
{
  layer_geometry.resize (layers.size ());

  for (std::vector<unsigned int>::const_iterator l = layers.begin (); l != layers.end (); ++l) {
    db::Region &r = layer_geometry [l - layers.begin ()];
    for (db::recursive_cluster_shape_iterator<db::NetShape> si (device_clusters, *l, ci, cid); ! si.at_end(); ++si) {
      insert_into_region (*si, si.trans (), r);
    }
    r.set_base_verbosity (50);
  }

  return normalize_device_layer_geometry (layer_geometry);
}

namespace
{
  struct DevicePtrCompare
  {
    bool operator() (const db::Device *d1, const db::Device *d2) const
    {
      return db::DeviceClass::less (*d1, *d2);
    }

    bool equals (const db::Device *d1, const db::Device *d2) const
    {
      return db::DeviceClass::equal (*d1, *d2);
    }
  };
}

static int compare_device_lists (std::vector<const db::Device *> &da, std::vector<const db::Device *> &db)
{
  if (da.size () != db.size ()) {
    return da.size () < db.size () ? -1 : 1;
  }

  std::sort (da.begin (), da.end (), DevicePtrCompare ());
  std::sort (db.begin (), db.end (), DevicePtrCompare ());

  for (auto i = da.begin (), j = db.begin (); i != da.end (); ++i, ++j) {
    if (! DevicePtrCompare ().equals (*i, *j)) {
      return DevicePtrCompare ()(*i, *j) ? -1 : 1;
    }
  }

  return 0;
}

void NetlistDeviceExtractor::pre_extract_for_device_propagation (const db::hier_clusters<shape_type> &device_clusters, const std::vector<unsigned int> &layers, const std::set<db::cell_index_type> &called_cells, std::set<std::pair<db::cell_index_type, db::connected_clusters<shape_type>::id_type> > &to_extract)
{
  typedef db::connected_clusters<shape_type>::id_type cluster_id_type;

  m_pre_extract = true;

  tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Pre-extracting devices for hierarchy analysis")));

  //  Step 1: do a pre-extraction on all clusters and collect their devices

  //  compute some effort number
  size_t n = 0;
  for (std::set<db::cell_index_type>::const_iterator ci = called_cells.begin (); ci != called_cells.end (); ++ci) {
    n += device_clusters.clusters_per_cell (*ci).size ();
  }

  //  do the pre-extraction of all clusters to devices
  //  -> the result is stored in "cluster2devices" and "extractor_cache" acts as a heap.

  typedef std::map<std::vector<db::Region>, tl::shared_collection<db::Device> > extractor_cache_type;
  extractor_cache_type extractor_cache;

  typedef std::map<std::pair<db::cell_index_type, cluster_id_type>, const tl::shared_collection<db::Device> *> cluster2devices_map;
  cluster2devices_map cluster2devices;

  {
    tl::RelativeProgress progress (tl::to_string (tr ("Pre-extracting devices")), to_extract.size (), 1);

    for (std::set<db::cell_index_type>::const_iterator ci = called_cells.begin (); ci != called_cells.end (); ++ci) {

      const db::connected_clusters<shape_type> &cc = device_clusters.clusters_per_cell (*ci);
      for (db::connected_clusters<shape_type>::all_iterator c = cc.begin_all (); !c.at_end(); ++c) {

        ++progress;

        //  build layer geometry from the cluster found
        std::vector<db::Region> layer_geometry;
        get_layer_geometry (layer_geometry, layers, device_clusters, *ci, *c);

        static tl::shared_collection<Device> empty_devices;
        const tl::shared_collection<Device> *devices = &empty_devices;

        extractor_cache_type::const_iterator ec = extractor_cache.find (layer_geometry);
        if (ec == extractor_cache.end ()) {

          m_log_entries.clear ();
          m_new_devices_pre_extracted.clear ();  //  safety

          //  do the actual device extraction
          extract_devices (layer_geometry);

          if (m_log_entries.empty ()) {

            //  cache unless log entries are produced
            tl::shared_collection<Device> &ce = extractor_cache [layer_geometry];
            ce.swap (m_new_devices_pre_extracted);
            devices = &ce;

          }

          m_new_devices_pre_extracted.clear ();

        } else {
          devices = &ec->second;
        }

        cluster2devices.insert (std::make_pair (std::make_pair (*ci, *c), devices));

      }

    }
  }

  //  Step 2: Identify all composed clusters where the devices are not identical to the sum of their child clusters.
  //  These child clusters will need to be eliminated from the hierarchy.

  //  -> the child clusters that need elimination will be stored in "to_eliminate"

  std::set<std::pair<db::cell_index_type, cluster_id_type> > to_eliminate;

  for (auto c2d = cluster2devices.begin (); c2d != cluster2devices.end (); ++c2d) {

    db::cell_index_type ci = c2d->first.first;
    cluster_id_type cid = c2d->first.second;
    const db::connected_clusters<shape_type> &cc = device_clusters.clusters_per_cell (ci);

    //  collect parent cluster devices
    const tl::shared_collection<db::Device> *devices = c2d->second;
    std::vector<const Device *> parent_devices;
    parent_devices.reserve (devices->size ());
    for (auto d = devices->begin (); d != devices->end (); ++d) {
      parent_devices.push_back (d.operator-> ());
    }

    //  collect devices from all child clusters
    std::vector<const Device *> child_devices;
    const db::connected_clusters<shape_type>::connections_type &connections = cc.connections_for_cluster (cid);
    for (auto icc = connections.begin (); icc != connections.end (); ++icc) {
      db::cell_index_type cci = icc->inst_cell_index ();
      cluster_id_type ccid = icc->id ();
      auto cc2d = cluster2devices.find (std::make_pair (cci, ccid));
      if (cc2d != cluster2devices.end ()) {
        for (auto d = cc2d->second->begin (); d != cc2d->second->end (); ++d) {
          child_devices.push_back (d.operator-> ());
        }
      }
    }

    //  if devices are not the same, enter the child clusters into the "to_eliminate" set
    if (compare_device_lists (parent_devices, child_devices) != 0) {
      for (auto icc = connections.begin (); icc != connections.end (); ++icc) {
        db::cell_index_type cci = icc->inst_cell_index ();
        cluster_id_type ccid = icc->id ();
        to_eliminate.insert (std::make_pair (cci, ccid));
      }
    }

  }

  //  Step 3: spread eliminiation status
  //  - Children of eliminated clusters get eliminated too.
  //  - If one child of a cluster gets eliminated, all others will too.
  //  Iterate until no futher cluster gets added to elimination set.
  //  NOTE: this algorithm has a bad worst-case performance, but this case
  //  is unlikely. Having the parents of a cluster would allow a more
  //  efficient algorithm.

  bool any_eliminated = ! to_eliminate.empty ();

  while (any_eliminated) {

    any_eliminated = false;

    for (auto c2d = cluster2devices.begin (); c2d != cluster2devices.end (); ++c2d) {

      db::cell_index_type ci = c2d->first.first;
      cluster_id_type cid = c2d->first.second;

      bool eliminate_all_children = (to_eliminate.find (std::make_pair (ci, cid)) != to_eliminate.end ());
      if (! eliminate_all_children) {

        //  all children need to be eliminated if one child is
        const db::connected_clusters<shape_type> &cc = device_clusters.clusters_per_cell (ci);
        const db::connected_clusters<shape_type>::connections_type &connections = cc.connections_for_cluster (cid);
        for (auto icc = connections.begin (); icc != connections.end () && ! eliminate_all_children; ++icc) {
          db::cell_index_type cci = icc->inst_cell_index ();
          cluster_id_type ccid = icc->id ();
          if (to_eliminate.find (std::make_pair (cci, ccid)) != to_eliminate.end ()) {
            eliminate_all_children = true;
          }
        }

      }

      if (eliminate_all_children) {

        const db::connected_clusters<shape_type> &cc = device_clusters.clusters_per_cell (ci);
        const db::connected_clusters<shape_type>::connections_type &connections = cc.connections_for_cluster (cid);
        for (auto icc = connections.begin (); icc != connections.end () && ! eliminate_all_children; ++icc) {
          db::cell_index_type cci = icc->inst_cell_index ();
          cluster_id_type ccid = icc->id ();
          if (to_eliminate.find (std::make_pair (cci, ccid)) == to_eliminate.end ()) {
            any_eliminated = true;
            to_eliminate.insert (std::make_pair (cci, ccid));
          }
        }

      }

    }

  }

  //  Step 4: extract all clusters
  //  - that are not eliminated themselves
  //  - that do not have children OR whose first child cluster is eliminated (then all others are too, see above)

  for (auto c2d = cluster2devices.begin (); c2d != cluster2devices.end (); ++c2d) {

    db::cell_index_type ci = c2d->first.first;
    cluster_id_type cid = c2d->first.second;

    bool is_eliminated = (to_eliminate.find (std::make_pair (ci, cid)) != to_eliminate.end ());
    if (! is_eliminated) {

      const db::connected_clusters<shape_type> &cc = device_clusters.clusters_per_cell (ci);
      const db::connected_clusters<shape_type>::connections_type &connections = cc.connections_for_cluster (cid);
      if (connections.empty ()) {
        to_extract.insert (c2d->first);
      } else {
        db::cell_index_type cci = connections.begin ()->inst_cell_index ();
        cluster_id_type ccid = connections.begin ()->id ();
        bool child_is_eliminated = (to_eliminate.find (std::make_pair (cci, ccid)) != to_eliminate.end ());
        if (child_is_eliminated) {
          to_extract.insert (c2d->first);
        }
      }

    }

  }

  m_pre_extract = false;
}

void NetlistDeviceExtractor::extract_without_initialize (db::Layout &layout, db::Cell &cell, hier_clusters_type &clusters, const std::vector<unsigned int> &layers, double device_scaling, const std::set<db::cell_index_type> *breakout_cells)
{
  tl_assert (layers.size () == m_layer_definitions.size ());

  mp_layout = &layout;
  m_layers = layers;
  mp_clusters = &clusters;
  m_device_scaling = device_scaling;
  mp_breakout_cells = breakout_cells;

  //  terminal properties are kept in a property with the terminal_property_name name
  m_terminal_id_propname_id = mp_layout->properties_repository ().prop_name_id (terminal_id_property_name ());
  m_device_id_propname_id = mp_layout->properties_repository ().prop_name_id (device_id_property_name ());
  m_device_class_propname_id = mp_layout->properties_repository ().prop_name_id (device_class_property_name ());

  tl_assert (m_netlist.get () != 0);

  //  build a cell-id-to-circuit lookup table
  std::map<db::cell_index_type, db::Circuit *> circuits_by_cell;
  for (db::Netlist::circuit_iterator c = m_netlist->begin_circuits (); c != m_netlist->end_circuits (); ++c) {
    circuits_by_cell.insert (std::make_pair (c->cell_index (), c.operator-> ()));
  }

  //  collect the cells below the top cell
  std::set<db::cell_index_type> all_called_cells;
  all_called_cells.insert (cell.cell_index ());
  cell.collect_called_cells (all_called_cells);

  //  ignore device cells from previous extractions
  std::set<db::cell_index_type> called_cells;
  for (std::set<db::cell_index_type>::const_iterator ci = all_called_cells.begin (); ci != all_called_cells.end (); ++ci) {
    if (! m_netlist->device_abstract_by_cell_index (*ci)) {
      called_cells.insert (*ci);
    }
  }
  all_called_cells.clear ();

  //  build the device clusters

  db::Connectivity device_conn = get_connectivity (layout, layers);
  db::hier_clusters<shape_type> device_clusters;
  device_clusters.build (layout, cell, device_conn, 0, breakout_cells);

  //  in "smart device propagation" mode, do a pre-extraction a determine the devices
  //  that need propagation

  std::set<std::pair<db::cell_index_type, db::connected_clusters<shape_type>::id_type> > to_extract;

  if (m_smart_device_propagation) {

    pre_extract_for_device_propagation (device_clusters, layers, called_cells, to_extract);

  } else {

    //  in stupid mode, extract all root clusters
    for (std::set<db::cell_index_type>::const_iterator ci = called_cells.begin (); ci != called_cells.end (); ++ci) {
      const db::connected_clusters<shape_type> &cc = device_clusters.clusters_per_cell (*ci);
      for (db::connected_clusters<shape_type>::all_iterator c = cc.begin_all (); !c.at_end(); ++c) {
        if (cc.is_root (*c)) {
          to_extract.insert (std::make_pair (*ci, *c));
        }
      }
    }

  }

  m_log_entries.clear ();
  m_pre_extract = false;

  tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Extracting devices")));

  //  count effort and make a progress reporter

  tl::RelativeProgress progress (tl::to_string (tr ("Extracting devices")), to_extract.size (), 1);

  typedef std::map<std::vector<db::Region>, ExtractorCacheValueType> extractor_cache_type;
  extractor_cache_type extractor_cache;

  //  extract clusters to devices
  for (auto e = to_extract.begin (); e != to_extract.end (); ++e) {

    ++progress;

    m_cell_index = e->first;

    std::map<db::cell_index_type, db::Circuit *>::const_iterator c2c = circuits_by_cell.find (m_cell_index);
    if (c2c != circuits_by_cell.end ()) {

      //  reuse existing circuit
      mp_circuit = c2c->second;

    } else {

      //  create a new circuit for this cell
      mp_circuit = new db::Circuit (layout, m_cell_index);
      m_netlist->add_circuit (mp_circuit);
      circuits_by_cell.insert (std::make_pair (m_cell_index, mp_circuit));

    }

    //  investigate each cluster
    db::connected_clusters<shape_type>::id_type c = e->second;

    //  build layer geometry from the cluster found
    std::vector<db::Region> layer_geometry;
    db::Vector disp = get_layer_geometry (layer_geometry, layers, device_clusters, m_cell_index, c);

    extractor_cache_type::const_iterator ec = extractor_cache.find (layer_geometry);
    if (ec == extractor_cache.end ()) {

      log_entry_list log_entries;
      m_log_entries.swap (log_entries);

      //  do the actual device extraction
      extract_devices (layer_geometry);

      //  push the new devices to the layout
      push_new_devices (disp);

      if (m_log_entries.empty ()) {

        //  cache unless log entries are produced
        ExtractorCacheValueType &ecv = extractor_cache [layer_geometry];
        ecv.disp = disp;

        for (std::map<size_t, std::pair<db::Device *, geometry_per_terminal_type> >::const_iterator d = m_new_devices.begin (); d != m_new_devices.end (); ++d) {
          ecv.devices.push_back (d->second.first);
        }

      } else {

        //  transform the marker geometries from the log entries to match the device
        db::DVector disp_dbu = db::CplxTrans (dbu ()) * disp;
        for (auto l = m_log_entries.begin (); l != m_log_entries.end (); ++l) {
          l->set_geometry (l->geometry ().moved (disp_dbu));
        }

      }

      m_log_entries.splice (m_log_entries.begin (), log_entries);

      m_new_devices.clear ();

    } else {

      push_cached_devices (ec->second.devices, ec->second.disp, disp);

    }

  }
}

void NetlistDeviceExtractor::push_new_devices (const db::Vector &disp_cache)
{
  db::CplxTrans dbu = db::CplxTrans (mp_layout->dbu ());
  db::VCplxTrans dbu_inv = dbu.inverted ();

  for (std::map<size_t, std::pair<db::Device *, geometry_per_terminal_type> >::const_iterator d = m_new_devices.begin (); d != m_new_devices.end (); ++d) {

    db::Device *device = d->second.first;

    db::Vector disp = dbu_inv * device->trans ().disp ();
    device->set_trans (db::DCplxTrans (device->trans ().disp () + dbu * disp_cache));

    DeviceCellKey key;

    for (geometry_per_terminal_type::const_iterator t = d->second.second.begin (); t != d->second.second.end (); ++t) {
      std::map<unsigned int, std::set<shape_type> > &gt = key.geometry [t->first];
      for (geometry_per_layer_type::const_iterator l = t->second.begin (); l != t->second.end (); ++l) {
        std::set<shape_type> &gl = gt [l->first];
        for (std::vector<shape_type>::const_iterator p = l->second.begin (); p != l->second.end (); ++p) {
          shape_type pr = *p;
          pr.transform (shape_type::trans_type (-disp));
          gl.insert (pr);
        }
      }
    }

    const std::vector<db::DeviceParameterDefinition> &pd = mp_device_class->parameter_definitions ();
    for (std::vector<db::DeviceParameterDefinition>::const_iterator p = pd.begin (); p != pd.end (); ++p) {
      key.parameters.insert (std::make_pair (p->id (), device->parameter_value (p->id ())));
    }

    db::PropertiesRepository::properties_set ps;

    std::map<DeviceCellKey, std::pair<db::cell_index_type, db::DeviceAbstract *> >::iterator c = m_device_cells.find (key);
    if (c == m_device_cells.end ()) {

      std::string cell_name = "D$" + mp_device_class->name ();
      db::Cell &device_cell = mp_layout->cell (mp_layout->add_cell (cell_name.c_str ()));

      db::DeviceAbstract *dm = new db::DeviceAbstract (mp_device_class.get (), mp_layout->cell_name (device_cell.cell_index ()));
      m_netlist->add_device_abstract (dm);
      dm->set_cell_index (device_cell.cell_index ());

      c = m_device_cells.insert (std::make_pair (key, std::make_pair (device_cell.cell_index (), dm))).first;

      //  attach the device class ID to the cell
      ps.clear ();
      ps.insert (std::make_pair (m_device_class_propname_id, tl::Variant (mp_device_class->name ())));
      device_cell.prop_id (mp_layout->properties_repository ().properties_id (ps));

      for (geometry_per_terminal_type::const_iterator t = d->second.second.begin (); t != d->second.second.end (); ++t) {

        //  Build a property set for the device terminal ID
        ps.clear ();
        ps.insert (std::make_pair (m_terminal_id_propname_id, tl::Variant (t->first)));
        db::properties_id_type pi = mp_layout->properties_repository ().properties_id (ps);

        //  build the cell shapes
        for (geometry_per_layer_type::const_iterator l = t->second.begin (); l != t->second.end (); ++l) {

          db::Shapes &shapes = device_cell.shapes (l->first);
          for (std::vector<shape_type>::const_iterator s = l->second.begin (); s != l->second.end (); ++s) {
            shape_type pr = *s;
            pr.transform (shape_type::trans_type (-disp));
            pr.insert_into (shapes, pi);
          }

        }

      }

    }

    //  make the cell index known to the device
    device->set_device_abstract (c->second.second);

    //  Build a property set for the device ID
    ps.clear ();
    ps.insert (std::make_pair (m_device_id_propname_id, tl::Variant (d->first)));
    db::properties_id_type pi = mp_layout->properties_repository ().properties_id (ps);

    db::CellInstArrayWithProperties inst (db::CellInstArray (db::CellInst (c->second.first), db::Trans (disp_cache + disp)), pi);
    mp_layout->cell (m_cell_index).insert (inst);

  }
}

void NetlistDeviceExtractor::push_cached_devices (const tl::vector<db::Device *> &cached_devices, const db::Vector &disp_cache, const db::Vector &new_disp)
{
  db::CplxTrans dbu = db::CplxTrans (mp_layout->dbu ());
  db::VCplxTrans dbu_inv = dbu.inverted ();
  db::PropertiesRepository::properties_set ps;

  for (std::vector<db::Device *>::const_iterator d = cached_devices.begin (); d != cached_devices.end (); ++d) {

    db::Device *cached_device = *d;
    db::Vector disp = dbu_inv * cached_device->trans ().disp () - disp_cache;

    db::Device *device = new db::Device (*cached_device);
    mp_circuit->add_device (device);
    device->set_trans (db::DCplxTrans (device->trans ().disp () + dbu * (new_disp - disp_cache)));

    //  Build a property set for the device ID
    ps.clear ();
    ps.insert (std::make_pair (m_device_id_propname_id, tl::Variant (device->id ())));
    db::properties_id_type pi = mp_layout->properties_repository ().properties_id (ps);

    db::CellInstArrayWithProperties inst (db::CellInstArray (db::CellInst (device->device_abstract ()->cell_index ()), db::Trans (new_disp + disp)), pi);
    mp_layout->cell (m_cell_index).insert (inst);

  }
}

void NetlistDeviceExtractor::setup ()
{
  //  .. the default implementation does nothing ..
}

db::Connectivity NetlistDeviceExtractor::get_connectivity (const db::Layout & /*layout*/, const std::vector<unsigned int> & /*layers*/) const
{
  //  .. the default implementation does nothing ..
  return db::Connectivity ();
}

void NetlistDeviceExtractor::extract_devices (const std::vector<db::Region> & /*layer_geometry*/)
{
  //  .. the default implementation does nothing ..
}

void NetlistDeviceExtractor::register_device_class (DeviceClass *device_class)
{
  std::unique_ptr<DeviceClass> holder (device_class);
  tl_assert (device_class != 0);
  tl_assert (m_netlist.get () != 0);

  if (mp_device_class.get () != 0) {
    throw tl::Exception (tl::to_string (tr ("Device class already set")));
  }
  if (m_name.empty ()) {
    throw tl::Exception (tl::to_string (tr ("No device extractor/device class name set")));
  }

  DeviceClass *existing = m_netlist->device_class_by_name (m_name);
  if (existing) {

    if (typeid (*existing) != typeid (*device_class)) {
      throw tl::Exception (tl::to_string (tr ("Different device class already registered with the same name")));
    }
    mp_device_class = existing;

  } else {

    mp_device_class = holder.get ();
    mp_device_class->set_name (m_name);

    m_netlist->add_device_class (holder.release ());

  }
}

const db::NetlistDeviceExtractorLayerDefinition &NetlistDeviceExtractor::define_layer (const std::string &name, const std::string &description)
{
  m_layer_definitions.push_back (db::NetlistDeviceExtractorLayerDefinition (name, description, m_layer_definitions.size (), std::numeric_limits<size_t>::max ()));
  return m_layer_definitions.back ();
}

const db::NetlistDeviceExtractorLayerDefinition &NetlistDeviceExtractor::define_layer (const std::string &name, size_t fallback, const std::string &description)
{
  m_layer_definitions.push_back (db::NetlistDeviceExtractorLayerDefinition (name, description, m_layer_definitions.size (), fallback));
  return m_layer_definitions.back ();
}

Device *NetlistDeviceExtractor::create_device ()
{
  if (mp_device_class.get () == 0) {
    throw tl::Exception (tl::to_string (tr ("No device class registered")));
  }

  Device *device;

  if (m_pre_extract) {
    device = new Device (mp_device_class.get ());
    m_new_devices_pre_extracted.push_back (device);
  } else {
    tl_assert (mp_circuit != 0);
    device = new Device (mp_device_class.get ());
    mp_circuit->add_device (device);
  }

  return device;
}

void NetlistDeviceExtractor::define_terminal (Device *device, size_t terminal_id, size_t geometry_index, const db::Region &region)
{
  if (m_pre_extract) {
    return;
  }

  tl_assert (mp_layout != 0);
  tl_assert (geometry_index < m_layers.size ());
  unsigned int layer_index = m_layers [geometry_index];

  std::pair<db::Device *, geometry_per_terminal_type> &dd = m_new_devices[device->id ()];
  dd.first = device;
  std::vector<shape_type> &geo = dd.second[terminal_id][layer_index];

  for (db::Region::const_iterator p = region.begin_merged (); !p.at_end (); ++p) {
    geo.push_back (shape_type (*p, mp_layout->shape_repository ()));
  }
}

void NetlistDeviceExtractor::define_terminal (Device *device, size_t terminal_id, size_t geometry_index, const db::Polygon &polygon)
{
  if (m_pre_extract) {
    return;
  }

  tl_assert (mp_layout != 0);
  tl_assert (geometry_index < m_layers.size ());
  unsigned int layer_index = m_layers [geometry_index];

  shape_type pr (polygon, mp_layout->shape_repository ());
  std::pair<db::Device *, geometry_per_terminal_type> &dd = m_new_devices[device->id ()];
  dd.first = device;
  dd.second[terminal_id][layer_index].push_back (pr);
}

void NetlistDeviceExtractor::define_terminal (Device *device, size_t terminal_id, size_t layer_index, const db::Box &box)
{
  define_terminal (device, terminal_id, layer_index, db::Polygon (box));
}

void NetlistDeviceExtractor::define_terminal (Device *device, size_t terminal_id, size_t layer_index, const db::Point &point)
{
  //  NOTE: we add one DBU to the "point" to prevent it from vanishing
  db::Vector dv (1, 1);
  define_terminal (device, terminal_id, layer_index, db::Polygon (db::Box (point - dv, point + dv)));
}

std::string NetlistDeviceExtractor::cell_name () const
{
  if (layout ()) {
    return layout ()->cell_name (cell_index ());
  } else {
    return std::string ();
  }
}

void NetlistDeviceExtractor::error (const std::string &msg)
{
  m_log_entries.push_back (db::LogEntryData (db::Error, cell_name (), msg));
  m_log_entries.back ().set_category_name ("device-extract");

  if (tl::verbosity () >= 20) {
    tl::error << m_log_entries.back ().to_string ();
  }
}

void NetlistDeviceExtractor::error (const std::string &msg, const db::DPolygon &poly)
{
  m_log_entries.push_back (db::LogEntryData (db::Error, cell_name (), msg));
  m_log_entries.back ().set_geometry (poly);
  m_log_entries.back ().set_category_name ("device-extract");

  if (tl::verbosity () >= 20) {
    tl::error << m_log_entries.back ().to_string ();
  }
}

void NetlistDeviceExtractor::error (const std::string &category_name, const std::string &category_description, const std::string &msg)
{
  m_log_entries.push_back (db::LogEntryData (db::Error, cell_name (), msg));
  m_log_entries.back ().set_category_name (category_name);
  m_log_entries.back ().set_category_description (category_description);

  if (tl::verbosity () >= 20) {
    tl::error << m_log_entries.back ().to_string ();
  }
}

void NetlistDeviceExtractor::error (const std::string &category_name, const std::string &category_description, const std::string &msg, const db::DPolygon &poly)
{
  m_log_entries.push_back (db::LogEntryData (db::Error, cell_name (), msg));
  m_log_entries.back ().set_category_name (category_name);
  m_log_entries.back ().set_category_description (category_description);
  m_log_entries.back ().set_geometry (poly);

  if (tl::verbosity () >= 20) {
    tl::error << m_log_entries.back ().to_string ();
  }
}

void NetlistDeviceExtractor::warn (const std::string &msg)
{
  m_log_entries.push_back (db::LogEntryData (db::Warning, cell_name (), msg));
  m_log_entries.back ().set_category_name ("device-extract");

  if (tl::verbosity () >= 20) {
    tl::warn << m_log_entries.back ().to_string ();
  }
}

void NetlistDeviceExtractor::warn (const std::string &msg, const db::DPolygon &poly)
{
  m_log_entries.push_back (db::LogEntryData (db::Warning, cell_name (), msg));
  m_log_entries.back ().set_geometry (poly);
  m_log_entries.back ().set_category_name ("device-extract");

  if (tl::verbosity () >= 20) {
    tl::warn << m_log_entries.back ().to_string ();
  }
}

void NetlistDeviceExtractor::warn (const std::string &category_name, const std::string &category_description, const std::string &msg)
{
  m_log_entries.push_back (db::LogEntryData (db::Warning, cell_name (), msg));
  m_log_entries.back ().set_category_name (category_name);
  m_log_entries.back ().set_category_description (category_description);

  if (tl::verbosity () >= 20) {
    tl::warn << m_log_entries.back ().to_string ();
  }
}

void NetlistDeviceExtractor::warn (const std::string &category_name, const std::string &category_description, const std::string &msg, const db::DPolygon &poly)
{
  m_log_entries.push_back (db::LogEntryData (db::Warning, cell_name (), msg));
  m_log_entries.back ().set_category_name (category_name);
  m_log_entries.back ().set_category_description (category_description);
  m_log_entries.back ().set_geometry (poly);

  if (tl::verbosity () >= 20) {
    tl::warn << m_log_entries.back ().to_string ();
  }
}

}
