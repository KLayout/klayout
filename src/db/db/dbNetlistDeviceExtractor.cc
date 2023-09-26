
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

#include "dbNetlistDeviceExtractor.h"
#include "dbRegion.h"
#include "dbHierNetworkProcessor.h"
#include "dbDeepRegion.h"

#include "tlProgress.h"
#include "tlTimer.h"
#include "tlInternational.h"

namespace db
{

// ----------------------------------------------------------------------------------------
//  NetlistDeviceExtractor implementation

NetlistDeviceExtractor::NetlistDeviceExtractor (const std::string &name)
  : mp_layout (0), m_cell_index (0), mp_breakout_cells (0), m_device_scaling (1.0), mp_circuit (0)
{
  m_name = name;
  m_terminal_id_propname_id = 0;
  m_device_class_propname_id = 0;
  m_device_id_propname_id = 0;
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

void NetlistDeviceExtractor::extract_without_initialize (db::Layout &layout, db::Cell &cell, hier_clusters_type &clusters, const std::vector<unsigned int> &layers, double device_scaling, const std::set<db::cell_index_type> *breakout_cells)
{
  tl_assert (layers.size () == m_layer_definitions.size ());

  typedef db::NetShape shape_type;

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

  tl::SelfTimer timer (tl::verbosity () >= 21, tl::to_string (tr ("Extracting devices")));

  //  count effort and make a progress reporter

  size_t n = 0;
  for (std::set<db::cell_index_type>::const_iterator ci = called_cells.begin (); ci != called_cells.end (); ++ci) {
    db::connected_clusters<shape_type> cc = device_clusters.clusters_per_cell (*ci);
    for (db::connected_clusters<shape_type>::all_iterator c = cc.begin_all (); !c.at_end(); ++c) {
      if (cc.is_root (*c)) {
        ++n;
      }
    }
  }

  tl::RelativeProgress progress (tl::to_string (tr ("Extracting devices")), n, 1);

  typedef std::map<std::vector<db::Region>, ExtractorCacheValueType> extractor_cache_type;
  extractor_cache_type extractor_cache;

  //  for each cell investigate the clusters
  for (std::set<db::cell_index_type>::const_iterator ci = called_cells.begin (); ci != called_cells.end (); ++ci) {

    m_cell_index = *ci;

    std::map<db::cell_index_type, db::Circuit *>::const_iterator c2c = circuits_by_cell.find (*ci);
    if (c2c != circuits_by_cell.end ()) {

      //  reuse existing circuit
      mp_circuit = c2c->second;

    } else {

      //  create a new circuit for this cell
      mp_circuit = new db::Circuit (layout, *ci);
      m_netlist->add_circuit (mp_circuit);

    }

    //  investigate each cluster
    db::connected_clusters<shape_type> cc = device_clusters.clusters_per_cell (*ci);
    for (db::connected_clusters<shape_type>::all_iterator c = cc.begin_all (); !c.at_end(); ++c) {

      //  take only root clusters - others have upward connections and are not "whole"
      if (! cc.is_root (*c)) {
        continue;
      }

      ++progress;

      //  build layer geometry from the cluster found

      std::vector<db::Region> layer_geometry;
      layer_geometry.resize (layers.size ());

      for (std::vector<unsigned int>::const_iterator l = layers.begin (); l != layers.end (); ++l) {
        db::Region &r = layer_geometry [l - layers.begin ()];
        for (db::recursive_cluster_shape_iterator<shape_type> si (device_clusters, *l, *ci, *c); ! si.at_end(); ++si) {
          insert_into_region (*si, si.trans (), r);
        }
        r.set_base_verbosity (50);
      }

      db::Box box;
      for (std::vector<db::Region>::const_iterator g = layer_geometry.begin (); g != layer_geometry.end (); ++g) {
        box += g->bbox ();
      }

      db::Vector disp = box.p1 () - db::Point ();
      for (std::vector<db::Region>::iterator g = layer_geometry.begin (); g != layer_geometry.end (); ++g) {
        g->transform (db::Disp (-disp));
      }

      extractor_cache_type::const_iterator ec = extractor_cache.find (layer_geometry);
      if (ec == extractor_cache.end ()) {

        //  do the actual device extraction
        extract_devices (layer_geometry);

        //  push the new devices to the layout
        push_new_devices (disp);

        ExtractorCacheValueType &ecv = extractor_cache [layer_geometry];
        ecv.disp = disp;

        for (std::map<size_t, std::pair<db::Device *, geometry_per_terminal_type> >::const_iterator d = m_new_devices.begin (); d != m_new_devices.end (); ++d) {
          ecv.devices.push_back (d->second.first);
        }

        m_new_devices.clear ();

      } else {

        push_cached_devices (ec->second.devices, ec->second.disp, disp);

      }

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
      std::map<unsigned int, std::set<db::NetShape> > &gt = key.geometry [t->first];
      for (geometry_per_layer_type::const_iterator l = t->second.begin (); l != t->second.end (); ++l) {
        std::set<db::NetShape> &gl = gt [l->first];
        for (std::vector<db::NetShape>::const_iterator p = l->second.begin (); p != l->second.end (); ++p) {
          db::NetShape pr = *p;
          pr.transform (db::NetShape::trans_type (-disp));
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
          for (std::vector<db::NetShape>::const_iterator s = l->second.begin (); s != l->second.end (); ++s) {
            db::NetShape pr = *s;
            pr.transform (db::NetShape::trans_type (-disp));
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

  tl_assert (mp_circuit != 0);
  Device *device = new Device (mp_device_class.get ());
  mp_circuit->add_device (device);
  return device;
}

void NetlistDeviceExtractor::define_terminal (Device *device, size_t terminal_id, size_t geometry_index, const db::Region &region)
{
  tl_assert (mp_layout != 0);
  tl_assert (geometry_index < m_layers.size ());
  unsigned int layer_index = m_layers [geometry_index];

  std::pair<db::Device *, geometry_per_terminal_type> &dd = m_new_devices[device->id ()];
  dd.first = device;
  std::vector<db::NetShape> &geo = dd.second[terminal_id][layer_index];

  for (db::Region::const_iterator p = region.begin_merged (); !p.at_end (); ++p) {
    geo.push_back (db::NetShape (*p, mp_layout->shape_repository ()));
  }
}

void NetlistDeviceExtractor::define_terminal (Device *device, size_t terminal_id, size_t geometry_index, const db::Polygon &polygon)
{
  tl_assert (mp_layout != 0);
  tl_assert (geometry_index < m_layers.size ());
  unsigned int layer_index = m_layers [geometry_index];

  db::NetShape pr (polygon, mp_layout->shape_repository ());
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
