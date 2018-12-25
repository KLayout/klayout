
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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
#include "dbNetlistDeviceClasses.h"
#include "dbHierNetworkProcessor.h"
#include "dbLayout.h"
#include "dbDeepShapeStore.h"
#include "dbRegion.h"
#include "dbStream.h"
#include "dbDeepRegion.h"
#include "dbDeepShapeStore.h"
#include "dbReader.h"
#include "dbWriter.h"
#include "dbCommonReader.h"

#include "tlUnitTest.h"
#include "tlString.h"
#include "tlFileUtils.h"

#include <memory>

class MOSFETExtractor
  : public db::NetlistDeviceExtractor
{
public:
  MOSFETExtractor (db::Layout *debug_out)
    : db::NetlistDeviceExtractor (), mp_debug_out (debug_out), m_ldiff (0), m_lgate (0)
  {
    if (mp_debug_out) {
      m_ldiff = mp_debug_out->insert_layer (db::LayerProperties (100, 0));
      m_lgate = mp_debug_out->insert_layer (db::LayerProperties (101, 0));
    }
  }

  virtual void create_device_classes ()
  {
    db::DeviceClassMOS3Transistor *pmos_class = new db::DeviceClassMOS3Transistor ();
    pmos_class->set_name ("PMOS");
    register_device_class (pmos_class);

    db::DeviceClassMOS3Transistor *nmos_class = new db::DeviceClassMOS3Transistor ();
    nmos_class->set_name ("NMOS");
    register_device_class (nmos_class);
  }

  virtual db::Connectivity get_connectivity (const db::Layout & /*layout*/, const std::vector<unsigned int> &layers) const
  {
    tl_assert (layers.size () == 3);

    unsigned int lpdiff = layers [0];
    unsigned int lndiff = layers [1];
    unsigned int gate = layers [2];

    //  The layer definition is pdiff, ndiff, gate
    db::Connectivity conn;
    //  collect all connected pdiff
    conn.connect (lpdiff, lpdiff);
    //  collect all connected ndiff
    conn.connect (lndiff, lndiff);
    //  collect all connected gate shapes
    conn.connect (gate, gate);
    //  connect gate with pdiff
    conn.connect (lpdiff, gate);
    //  connect gate with ndiff
    conn.connect (lndiff, gate);
    return conn;
  }

  virtual void extract_devices (const std::vector<db::Region> &layer_geometry)
  {
    const db::Region &rpdiff = layer_geometry [0];
    const db::Region &rndiff = layer_geometry [1];
    const db::Region &rgates = layer_geometry [2];

    for (db::Region::const_iterator p = rgates.begin_merged (); !p.at_end (); ++p) {

      db::Region rgate (*p);
      db::Region rpdiff_on_gate = rpdiff.selected_interacting (rgate);
      db::Region rndiff_on_gate = rndiff.selected_interacting (rgate);

      if (! rpdiff_on_gate.empty () && ! rndiff_on_gate.empty ()) {
        error (tl::to_string (tr ("Gate shape touches both ndiff and pdiff - ignored")), *p);
      } else if (rpdiff_on_gate.empty () && rndiff_on_gate.empty ()) {
        error (tl::to_string (tr ("Gate shape touches neither ndiff and pdiff - ignored")), *p);
      } else {

        db::Region &diff = (rpdiff_on_gate.empty () ? rndiff_on_gate : rpdiff_on_gate);

        if (diff.size () != 2) {
          error (tl::sprintf (tl::to_string (tr ("Expected two polygons on diff interacting one gate shape (found %d) - gate shape ignored")), int (diff.size ())), *p);
          continue;
        }

        db::Edges edges (rgate.edges () & diff.edges ());
        if (edges.size () != 2) {
          error (tl::sprintf (tl::to_string (tr ("Expected two edges interacting gate/diff (found %d) - width and length may be incorrect")), int (edges.size ())), *p);
          continue;
        }

        if (! p->is_box ()) {
          error (tl::to_string (tr ("Gate shape is not a box - width and length may be incorrect")), *p);
        }

        db::Device *device = create_device (! rpdiff_on_gate.empty () ? 0 /*pdiff*/ : 1 /*ndiff*/);

        device->set_parameter_value ("W", dbu () * edges.length () * 0.5);
        device->set_parameter_value ("L", dbu () * (p->perimeter () - edges.length ()) * 0.5);

        int index = 0;
        for (db::Region::const_iterator d = diff.begin (); !d.at_end () && index < 2; ++d, ++index) {

          //  count the number of gate shapes attached to this shape and distribute the area of the
          //  diffusion area to the number of gates
          int n = rgates.selected_interacting (db::Region (*d)).size ();
          tl_assert (n > 0);

          device->set_parameter_value (index == 0 ? "AS" : "AD", dbu () * dbu () * d->area () / double (n));

        }

        // @@@ create terminals

        device_out (device, diff, rgate);

      }

    }
  }

  void error (const std::string &msg)
  {
    // @@@ TODO: move this to device extractor
    tl::error << tr ("Error in cell '") << cell_name () << "': " << msg;
  }

  void error (const std::string &msg, const db::Polygon &poly)
  {
    // @@@ TODO: move this to device extractor
    tl::error << tr ("Error in cell '") << cell_name () << "': " << msg << " (" << poly.to_string () << ")";
  }

  void error (const std::string &msg, const db::Region &region)
  {
    // @@@ TODO: move this to device extractor
    tl::error << tr ("Error in cell '") << cell_name () << "': " << msg << " (" << region.to_string () << ")";
  }

  std::string cell_name () const
  {
    // @@@ TODO: move this to device extractor
    return layout ()->cell_name (cell_index ());
  }

private:
  db::Layout *mp_debug_out;
  unsigned int m_ldiff, m_lgate;

  void device_out (const db::Device *device, const db::Region &diff, const db::Region &gate)
  {
    if (! mp_debug_out) {
      return;
    }

    std::string cn = layout ()->cell_name (cell_index ());
    std::pair<bool, db::cell_index_type> target_cp = mp_debug_out->cell_by_name (cn.c_str ());
    tl_assert (target_cp.first);

    db::cell_index_type dci = mp_debug_out->add_cell ((device->device_class ()->name () + "_" + device->name ()).c_str ());
    mp_debug_out->cell (target_cp.second).insert (db::CellInstArray (db::CellInst (dci), db::Trans ()));

    db::Cell &device_cell = mp_debug_out->cell (dci);
    for (db::Region::const_iterator p = diff.begin (); ! p.at_end (); ++p) {
      device_cell.shapes (m_ldiff).insert (*p);
    }
    for (db::Region::const_iterator p = gate.begin (); ! p.at_end (); ++p) {
      device_cell.shapes (m_lgate).insert (*p);
    }

    std::string ps;
    const std::vector<db::DeviceParameterDefinition> &pd = device->device_class ()->parameter_definitions ();
    for (std::vector<db::DeviceParameterDefinition>::const_iterator i = pd.begin (); i != pd.end (); ++i) {
      if (! ps.empty ()) {
        ps += ",";
      }
      ps += i->name () + "=" + tl::to_string (device->parameter_value (i->id ()));
    }
    device_cell.shapes (m_ldiff).insert (db::Text (ps, db::Trans (diff.bbox ().center () - db::Point ())));
  }
};

TEST(1_DeviceNetExtraction)
{
  bool write_debug = true;

  db::Layout ly;
  unsigned int nwell, active, poly;

  db::LayerProperties p;
  db::LayerMap lmap;

  p.layer = 1;
  p.datatype = 0;
  lmap.map (db::LDPair (p.layer, p.datatype), nwell = ly.insert_layer ());
  ly.set_properties (nwell, p);

  p.layer = 2;
  p.datatype = 0;
  lmap.map (db::LDPair (p.layer, p.datatype), active = ly.insert_layer ());
  ly.set_properties (active, p);

  p.layer = 3;
  p.datatype = 0;
  lmap.map (db::LDPair (p.layer, p.datatype), poly = ly.insert_layer ());
  ly.set_properties (poly, p);

  db::LoadLayoutOptions options;
  options.get_options<db::CommonReaderOptions> ().layer_map = lmap;
  options.get_options<db::CommonReaderOptions> ().create_other_layers = false;

  std::string fn (tl::testsrc ());
  fn = tl::combine_path (fn, "testdata");
  fn = tl::combine_path (fn, "algo");
  fn = tl::combine_path (fn, "device_extract_l1.gds");

  tl::InputStream stream (fn);
  db::Reader reader (stream);
  reader.read (ly, options);

  db::Cell &tc = ly.cell (*ly.begin_top_down ());

  db::DeepShapeStore dss;

  //  original layers
  db::Region rnwell (db::RecursiveShapeIterator (ly, tc, nwell), dss);
  db::Region ractive (db::RecursiveShapeIterator (ly, tc, active), dss);
  db::Region rpoly (db::RecursiveShapeIterator (ly, tc, poly), dss);

  //  derived regions
  db::Region rgate  = ractive & rpoly;
  db::Region rsd    = ractive - rgate;
  db::Region rpdiff = rsd & rnwell;
  db::Region rndiff = rsd - rnwell;

  if (write_debug) {

    //  return the computed layers into the original layout and write it for debugging purposes

    unsigned int lgate  = ly.insert_layer (db::LayerProperties (10, 0));      // 10/0 -> Gate
    unsigned int lsd    = ly.insert_layer (db::LayerProperties (11, 0));      // 11/0 -> Source/Drain
    unsigned int lpdiff = ly.insert_layer (db::LayerProperties (12, 0));      // 12/0 -> P Diffusion
    unsigned int lndiff = ly.insert_layer (db::LayerProperties (13, 0));      // 13/0 -> N Diffusion

    rgate.insert_into (&ly, tc.cell_index (), lgate);
    rsd.insert_into (&ly, tc.cell_index (), lsd);
    rpdiff.insert_into (&ly, tc.cell_index (), lpdiff);
    rndiff.insert_into (&ly, tc.cell_index (), lndiff);

  }

  db::DeepRegion *dr = dynamic_cast<db::DeepRegion *> (rnwell.delegate ());
  db::DeepLayer dl = dr->deep_layer ();
  dl.layout ();
  dl.initial_cell ();
  dl.layer ();

  db::Netlist nl;

  MOSFETExtractor ex (write_debug ? &ly : 0);
  ex.initialize (&nl);

  std::vector<db::Region *> region_ptrs;
  region_ptrs.push_back (&rpdiff);
  region_ptrs.push_back (&rndiff);
  region_ptrs.push_back (&rgate);
  ex.extract (region_ptrs);

  if (write_debug) {

    std::string fn (tl::testtmp ());
    fn = tl::combine_path (fn, "debug-1_DeviceNetExtraction.gds");

    tl::OutputStream stream (fn);
    db::SaveLayoutOptions options;
    db::Writer writer (options);
    writer.write (ly, stream);

    tl::log << "Device layer debug file written to: " << tl::absolute_file_path (fn);

  }
}
