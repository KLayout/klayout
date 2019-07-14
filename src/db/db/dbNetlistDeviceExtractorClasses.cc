
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


#include "dbNetlistDeviceExtractorClasses.h"
#include "dbNetlistDeviceClasses.h"

namespace db
{

// ---------------------------------------------------------------------------------
//  NetlistDeviceExtractorMOS3Transistor implementation

NetlistDeviceExtractorMOS3Transistor::NetlistDeviceExtractorMOS3Transistor (const std::string &name)
  : db::NetlistDeviceExtractor (name)
{
  //  .. nothing yet ..
}

void NetlistDeviceExtractorMOS3Transistor::setup ()
{
  define_layer ("SD", "Source/drain diffusion");                        // #0
  define_layer ("G", "Gate input");                                     // #1
  //  for backward compatibility
  define_layer ("P", 1, "Gate terminal output");                        // #2 -> G

  //  terminal output
  define_layer ("tG", 2, "Gate terminal output");                       // #3 -> P -> G
  define_layer ("tS", 0, "Source terminal output (default is SD)");     // #4
  define_layer ("tD", 0, "Drain terminal output (default is SD)");      // #5

  register_device_class (new db::DeviceClassMOS3Transistor ());
}

db::Connectivity NetlistDeviceExtractorMOS3Transistor::get_connectivity (const db::Layout & /*layout*/, const std::vector<unsigned int> &layers) const
{
  tl_assert (layers.size () >= 3);

  unsigned int diff = layers [0];
  unsigned int gate = layers [1];
  //  not used for device recognition: poly (2), but used for producing the gate terminals

  //  The layer definition is diff, gate
  db::Connectivity conn;
  //  collect all connected diffusion shapes
  conn.connect (diff, diff);
  //  collect all connected gate shapes
  conn.connect (gate, gate);
  //  connect gate with diff to detect gate/diffusion boundary
  conn.connect (diff, gate);
  return conn;
}

void NetlistDeviceExtractorMOS3Transistor::extract_devices (const std::vector<db::Region> &layer_geometry)
{
  unsigned int diff_geometry_index = 0;
  unsigned int gate_geometry_index = 1;
  unsigned int gate_terminal_geometry_index = 3;
  unsigned int source_terminal_geometry_index = 4;
  unsigned int drain_terminal_geometry_index = 5;

  const db::Region &rdiff = layer_geometry [diff_geometry_index];
  const db::Region &rgates = layer_geometry [gate_geometry_index];

  for (db::Region::const_iterator p = rgates.begin_merged (); !p.at_end (); ++p) {

    db::Region rgate (*p);
    rgate.set_base_verbosity (rgates.base_verbosity ());

    db::Region rdiff2gate = rdiff.selected_interacting (rgate);
    rdiff2gate.set_base_verbosity (rdiff.base_verbosity ());

    if (rdiff2gate.empty ()) {
      error (tl::to_string (tr ("Gate shape touches no diffusion - ignored")), *p);
    } else {

      if (rdiff2gate.size () != 2) {
        error (tl::sprintf (tl::to_string (tr ("Expected two polygons on diff interacting with one gate shape (found %d) - gate shape ignored")), int (rdiff2gate.size ())), *p);
        continue;
      }

      db::Edges edges (rgate.edges () & rdiff2gate.edges ());
      if (edges.size () != 2) {
        error (tl::sprintf (tl::to_string (tr ("Expected two edges interacting gate/diff (found %d) - width and length may be incorrect")), int (edges.size ())), *p);
        continue;
      }

      if (! p->is_box ()) {
        error (tl::to_string (tr ("Gate shape is not a box - width and length may be incorrect")), *p);
      }

      db::Device *device = create_device ();

      device->set_trans (db::DCplxTrans ((p->box ().center () - db::Point ()) * dbu ()));

      device->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_W, dbu () * edges.length () * 0.5);
      device->set_parameter_value (db::DeviceClassMOS3Transistor::param_id_L, dbu () * (p->perimeter () - edges.length ()) * 0.5);

      int diff_index = 0;
      for (db::Region::const_iterator d = rdiff2gate.begin (); !d.at_end () && diff_index < 2; ++d, ++diff_index) {

        //  count the number of gate shapes attached to this shape and distribute the area of the
        //  diffusion region to the number of gates
        size_t n = rgates.selected_interacting (db::Region (*d)).size ();
        tl_assert (n > 0);

        device->set_parameter_value (diff_index == 0 ? db::DeviceClassMOS3Transistor::param_id_AS : db::DeviceClassMOS3Transistor::param_id_AD, dbu () * dbu () * d->area () / double (n));
        device->set_parameter_value (diff_index == 0 ? db::DeviceClassMOS3Transistor::param_id_PS : db::DeviceClassMOS3Transistor::param_id_PD, dbu () * d->perimeter () / double (n));

        unsigned int sd_index = diff_index == 0 ? source_terminal_geometry_index : drain_terminal_geometry_index;
        define_terminal (device, diff_index == 0 ? db::DeviceClassMOS3Transistor::terminal_id_S : db::DeviceClassMOS3Transistor::terminal_id_D, sd_index, *d);

      }

      define_terminal (device, db::DeviceClassMOS3Transistor::terminal_id_G, gate_terminal_geometry_index, *p);

      //  allow derived classes to modify the device
      modify_device (*p, layer_geometry, device);

      //  output the device for debugging
      device_out (device, rdiff2gate, rgate);

    }

  }
}

// ---------------------------------------------------------------------------------
//  NetlistDeviceExtractorMOS4Transistor implementation

NetlistDeviceExtractorMOS4Transistor::NetlistDeviceExtractorMOS4Transistor (const std::string &name)
  : NetlistDeviceExtractorMOS3Transistor (name)
{
  //  .. nothing yet ..
}

void NetlistDeviceExtractorMOS4Transistor::setup ()
{
  define_layer ("SD", "Source/drain diffusion");                      // #0
  define_layer ("G", "Gate input");                                   // #1
  //  for backward compatibility
  define_layer ("P", 1, "Gate terminal output");                      // #2 -> G

  //  terminal output
  define_layer ("tG", 2, "Gate terminal output");                     // #3 -> P -> G
  define_layer ("tS", 0, "Source terminal output (default is SD)");   // #4
  define_layer ("tD", 0, "Drain terminal output (default is SD)");    // #5

  //  for backward compatibility
  define_layer ("W", "Well (bulk) terminal output");                  // #6

  define_layer ("tB", 6, "Well (bulk) terminal output");              // #7 -> W

  register_device_class (new db::DeviceClassMOS4Transistor ());
}

void NetlistDeviceExtractorMOS4Transistor::modify_device (const db::Polygon &rgate, const std::vector<db::Region> & /*layer_geometry*/, db::Device *device)
{
  unsigned int bulk_terminal_geometry_index = 7;
  define_terminal (device, db::DeviceClassMOS4Transistor::terminal_id_B, bulk_terminal_geometry_index, rgate);
}

// ---------------------------------------------------------------------------------
//  NetlistDeviceExtractorResistor implementation

NetlistDeviceExtractorResistor::NetlistDeviceExtractorResistor (const std::string &name, double sheet_rho)
  : db::NetlistDeviceExtractor (name), m_sheet_rho (sheet_rho)
{
  //  .. nothing yet ..
}

void NetlistDeviceExtractorResistor::setup ()
{
  define_layer ("R", "Resistor");                 // #0
  define_layer ("C", "Contacts");                 // #1
  define_layer ("tA", 1, "A terminal output");    // #2 -> C
  define_layer ("tB", 1, "B terminal output");    // #3 -> C

  register_device_class (new db::DeviceClassResistor ());
}

db::Connectivity NetlistDeviceExtractorResistor::get_connectivity (const db::Layout & /*layout*/, const std::vector<unsigned int> &layers) const
{
  tl_assert (layers.size () >= 2);

  unsigned int res = layers [0];
  unsigned int contact = layers [1];

  //  The layer definition is res, contact
  db::Connectivity conn;
  //  collect all connected resistor shapes
  conn.connect (res, res);
  //  connect res with contact for the contact shapes
  conn.connect (res, contact);
  return conn;
}

void NetlistDeviceExtractorResistor::extract_devices (const std::vector<db::Region> &layer_geometry)
{
  size_t res_geometry_index = 0;
  size_t contacts_geometry_index = 1;
  size_t a_terminal_geometry_index = 2;
  size_t b_terminal_geometry_index = 3;

  const db::Region &res = layer_geometry [res_geometry_index];
  const db::Region &contact = layer_geometry [contacts_geometry_index];

  db::Region res_merged (res);
  res_merged.set_base_verbosity (res.base_verbosity ());

  db::Region contact_wo_res (contact);
  contact_wo_res.set_base_verbosity (contact.base_verbosity ());
  contact_wo_res -= res;

  for (db::Region::const_iterator p = res_merged.begin_merged (); !p.at_end (); ++p) {

    db::Region rres (*p);
    db::Region contacts_per_res = contact_wo_res.selected_interacting (rres);

    if (contacts_per_res.size () != 2) {
      error (tl::sprintf (tl::to_string (tr ("Expected two polygons on contacts interacting with one resistor shape (found %d) - resistor shape ignored")), int (contacts_per_res.size ())), *p);
      continue;
    }

    db::Device *device = create_device ();

    device->set_trans (db::DCplxTrans ((p->box ().center () - db::Point ()) * dbu ()));

    //  TODO: this is a very rough approximation for the general case - it assumes a "good" geometry

    db::Edges eparallel = rres.edges ();
    eparallel -= contacts_per_res.edges ();

    db::Edges eperp = rres.edges ();
    eperp &= contacts_per_res.edges ();

    db::Coord length = eparallel.length ();
    db::Coord width = eperp.length ();

    if (width < 1) {
      error (tl::to_string (tr ("Invalid contact geometry - resistor shape ignored")), *p);
      continue;
    }

    device->set_parameter_value (db::DeviceClassResistor::param_id_R, m_sheet_rho * double (length) / double (width));
    device->set_parameter_value (db::DeviceClassResistor::param_id_L, dbu () * length);
    device->set_parameter_value (db::DeviceClassResistor::param_id_W, dbu () * width);
    device->set_parameter_value (db::DeviceClassResistor::param_id_A, dbu () * dbu () * p->area ());
    device->set_parameter_value (db::DeviceClassResistor::param_id_P, dbu () * p->perimeter ());

    int cont_index = 0;
    for (db::Region::const_iterator d = contacts_per_res.begin (); !d.at_end () && cont_index < 2; ++d, ++cont_index) {
      size_t terminal_geometry_index = cont_index == 0 ? a_terminal_geometry_index : b_terminal_geometry_index;
      define_terminal (device, cont_index == 0 ? db::DeviceClassResistor::terminal_id_A : db::DeviceClassResistor::terminal_id_B, terminal_geometry_index, *d);
    }

    //  allow derived classes to modify the device
    modify_device (*p, layer_geometry, device);

    //  output the device for debugging
    device_out (device, rres, contacts_per_res);

  }
}

// ---------------------------------------------------------------------------------
//  NetlistDeviceExtractorResistorWithBulk implementation

NetlistDeviceExtractorResistorWithBulk::NetlistDeviceExtractorResistorWithBulk (const std::string &name, double sheet_rho)
  : NetlistDeviceExtractorResistor (name, sheet_rho)
{
  //  .. nothing yet ..
}

void NetlistDeviceExtractorResistorWithBulk::setup ()
{
  define_layer ("R", "Resistor");                 // #0
  define_layer ("C", "Contacts");                 // #1
  define_layer ("tA", 1, "A terminal output");    // #2 -> C
  define_layer ("tB", 1, "B terminal output");    // #3 -> C
  define_layer ("W", "Well/Bulk");                // #4
  define_layer ("tW", 4, "W terminal output");    // #5 -> W

  register_device_class (new db::DeviceClassResistorWithBulk ());
}

void NetlistDeviceExtractorResistorWithBulk::modify_device (const db::Polygon &res, const std::vector<db::Region> & /*layer_geometry*/, db::Device *device)
{
  unsigned int bulk_terminal_geometry_index = 5;
  define_terminal (device, db::DeviceClassResistorWithBulk::terminal_id_W, bulk_terminal_geometry_index, res);
}

// ---------------------------------------------------------------------------------
//  NetlistDeviceExtractorCapacitor implementation

NetlistDeviceExtractorCapacitor::NetlistDeviceExtractorCapacitor (const std::string &name, double area_cap)
  : db::NetlistDeviceExtractor (name), m_area_cap (area_cap)
{
  //  .. nothing yet ..
}

void NetlistDeviceExtractorCapacitor::setup ()
{
  define_layer ("P1", "Plate 1");                   // #0
  define_layer ("P2", "Plate 2");                   // #1
  define_layer ("tA", 0, "A terminal output");      // #2 -> P1
  define_layer ("tB", 1, "B terminal output");      // #3 -> P2

  register_device_class (new db::DeviceClassCapacitor ());
}

db::Connectivity NetlistDeviceExtractorCapacitor::get_connectivity (const db::Layout & /*layout*/, const std::vector<unsigned int> &layers) const
{
  tl_assert (layers.size () >= 2);

  unsigned int plate1 = layers [0];
  unsigned int plate2 = layers [1];

  //  The layer definition is plate1, plate2
  db::Connectivity conn;
  //  collect all connected plate 1 shapes
  conn.connect (plate1, plate1);
  //  collect all connected plate 1 shapes
  conn.connect (plate2, plate2);
  //  connect the plates (NOTE that this is a logical, not a physical connection)
  conn.connect (plate1, plate2);
  return conn;
}

void NetlistDeviceExtractorCapacitor::extract_devices (const std::vector<db::Region> &layer_geometry)
{
  size_t plate1_geometry_index = 0;
  size_t plate2_geometry_index = 1;
  size_t a_terminal_geometry_index = 2;
  size_t b_terminal_geometry_index = 3;

  const db::Region &plate1 = layer_geometry [plate1_geometry_index];
  const db::Region &plate2 = layer_geometry [plate2_geometry_index];

  db::Region overlap (plate1);
  overlap.set_base_verbosity (plate1.base_verbosity ());
  overlap &= plate2;

  for (db::Region::const_iterator p = overlap.begin_merged (); !p.at_end (); ++p) {

    db::Device *device = create_device ();

    device->set_trans (db::DCplxTrans ((p->box ().center () - db::Point ()) * dbu ()));

    double area = p->area () * dbu () * dbu ();

    device->set_parameter_value (db::DeviceClassCapacitor::param_id_C, m_area_cap * area);
    device->set_parameter_value (db::DeviceClassCapacitor::param_id_A, area);
    device->set_parameter_value (db::DeviceClassCapacitor::param_id_P, dbu () * p->perimeter ());

    define_terminal (device, db::DeviceClassCapacitor::terminal_id_A, a_terminal_geometry_index, *p);
    define_terminal (device, db::DeviceClassCapacitor::terminal_id_B, b_terminal_geometry_index, *p);

    //  allow derived classes to modify the device
    modify_device (*p, layer_geometry, device);

    //  output the device for debugging
    device_out (device, *p);

  }
}

// ---------------------------------------------------------------------------------
//  NetlistDeviceExtractorCapacitorWithBulk implementation

NetlistDeviceExtractorCapacitorWithBulk::NetlistDeviceExtractorCapacitorWithBulk (const std::string &name, double area_cap)
  : NetlistDeviceExtractorCapacitor (name, area_cap)
{
  //  .. nothing yet ..
}

void NetlistDeviceExtractorCapacitorWithBulk::setup ()
{
  define_layer ("P1", "Plate 1");                   // #0
  define_layer ("P2", "Plate 2");                   // #1
  define_layer ("tA", 0, "A terminal output");      // #2 -> P1
  define_layer ("tB", 1, "B terminal output");      // #3 -> P2
  define_layer ("W", "Well/Bulk");                  // #4
  define_layer ("tW", 4, "W terminal output");      // #5 -> W

  register_device_class (new db::DeviceClassCapacitorWithBulk ());
}

void NetlistDeviceExtractorCapacitorWithBulk::modify_device (const db::Polygon &cap, const std::vector<db::Region> & /*layer_geometry*/, db::Device *device)
{
  unsigned int bulk_terminal_geometry_index = 5;
  define_terminal (device, db::DeviceClassCapacitorWithBulk::terminal_id_W, bulk_terminal_geometry_index, cap);
}

// ---------------------------------------------------------------------------------
//  NetlistDeviceExtractorBJT3Transistor implementation

NetlistDeviceExtractorBJT3Transistor::NetlistDeviceExtractorBJT3Transistor (const std::string &name)
  : db::NetlistDeviceExtractor (name)
{
  //  .. nothing yet ..
}

void NetlistDeviceExtractorBJT3Transistor::setup ()
{
  define_layer ("C", "Collector");                                      // #0
  define_layer ("B", "Base");                                           // #1
  define_layer ("E", "Emitter");                                        // #2

  //  terminal output
  define_layer ("tC", 0, "Collector terminal output");                  // #3 -> C
  define_layer ("tB", 1, "Base terminal output");                       // #4 -> B
  define_layer ("tE", 2, "Emitter terminal output");                    // #5 -> E

  register_device_class (new db::DeviceClassBJT3Transistor ());
}

db::Connectivity NetlistDeviceExtractorBJT3Transistor::get_connectivity (const db::Layout & /*layout*/, const std::vector<unsigned int> &layers) const
{
  tl_assert (layers.size () >= 3);

  unsigned int collector = layers [0];
  unsigned int base = layers [1];
  unsigned int emitter = layers [2];

  db::Connectivity conn;
  //  collect all connected base shapes. Join polygons.
  conn.connect (base, base);
  //  collect all collector and emitter shapes connected with base
  conn.connect (base, collector);
  conn.connect (base, emitter);
  return conn;
}

void NetlistDeviceExtractorBJT3Transistor::extract_devices (const std::vector<db::Region> &layer_geometry)
{
  unsigned int collector_geometry_index = 0;
  unsigned int base_geometry_index = 1;
  unsigned int emitter_geometry_index = 2;
  unsigned int collector_terminal_geometry_index = 3;
  unsigned int base_terminal_geometry_index = 4;
  unsigned int emitter_terminal_geometry_index = 5;

  const db::Region &rbases = layer_geometry [base_geometry_index];
  const db::Region &rcollectors = layer_geometry [collector_geometry_index];
  const db::Region &remitters = layer_geometry [emitter_geometry_index];

  for (db::Region::const_iterator p = rbases.begin_merged (); !p.at_end (); ++p) {

    db::Region rbase (*p);
    rbase.set_base_verbosity (rbases.base_verbosity ());

    db::Region remitter2base = rbase & remitters;

    if (remitter2base.empty ()) {
      error (tl::to_string (tr ("Base shape without emitters - ignored")), *p);
    } else {

      //  collectors inside base
      db::Region rcollector2base = rbase & rcollectors;

      db::Region rcollector;
      if (rcollector2base.empty ()) {
        //  collector is bulk (vertical)
        rcollector2base = rbase;
        rcollector = rbase;
      } else if ((rbase - rcollector2base).empty ()) {
        //  vertical transistor: collector entirely covers base -> collector terminal is collector outside base
        rcollector = rcollector2base - rbase;
      } else {
        //  lateral transistor: base is reduced by collector area
        rcollector = rcollector2base;
        rbase -= rcollector2base;
      }

      //  this is what is the true base contact
      rbase -= remitter2base;

      //  emitter wins over collector for the collector contact
      rcollector -= remitter2base;

      double ab = dbu () * dbu () * p->area ();
      double pb = dbu () * p->perimeter ();

      double ac = dbu () * dbu () * rcollector2base.area ();
      double pc = dbu () * rcollector2base.perimeter ();

      for (db::Region::const_iterator pe = remitter2base.begin_merged (); !pe.at_end (); ++pe) {

        db::Device *device = create_device ();

        device->set_trans (db::DCplxTrans ((pe->box ().center () - db::Point ()) * dbu ()));

        device->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_NE, 1.0);

        device->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_AE, dbu () * dbu () * pe->area ());
        device->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_PE, dbu () * pe->perimeter ());

        device->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_AB, ab);
        device->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_PB, pb);

        device->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_AC, ac);
        device->set_parameter_value (db::DeviceClassBJT3Transistor::param_id_PC, pc);

        define_terminal (device, db::DeviceClassBJT3Transistor::terminal_id_C, collector_terminal_geometry_index, rcollector);
        define_terminal (device, db::DeviceClassBJT3Transistor::terminal_id_B, base_terminal_geometry_index, rbase);
        define_terminal (device, db::DeviceClassBJT3Transistor::terminal_id_E, emitter_terminal_geometry_index, *pe);

        //  allow derived classes to modify the device
        modify_device (*p, layer_geometry, device);

        //  output the device for debugging
        device_out (device, rcollector, rbase, *pe);

      }

    }

  }
}

// ---------------------------------------------------------------------------------
//  NetlistDeviceExtractorBJT4Transistor implementation

NetlistDeviceExtractorBJT4Transistor::NetlistDeviceExtractorBJT4Transistor (const std::string &name)
  : NetlistDeviceExtractorBJT3Transistor (name)
{
  //  .. nothing yet ..
}

void NetlistDeviceExtractorBJT4Transistor::setup ()
{
  define_layer ("C", "Collector");                                      // #0
  define_layer ("B", "Base");                                           // #1
  define_layer ("E", "Emitter");                                        // #2

  //  terminal output
  define_layer ("tC", 0, "Collector terminal output");                  // #3 -> C
  define_layer ("tB", 1, "Base terminal output");                       // #4 -> B
  define_layer ("tE", 2, "Emitter terminal output");                    // #5 -> E

  //  for convenience and consistency with MOS4
  define_layer ("S", "Substrate (bulk) terminal output");               // #6

  define_layer ("tS", 6, "Substrate (bulk) terminal output");           // #7 -> S

  register_device_class (new db::DeviceClassBJT4Transistor ());
}

void NetlistDeviceExtractorBJT4Transistor::modify_device (const db::Polygon &emitter, const std::vector<db::Region> & /*layer_geometry*/, db::Device *device)
{
  unsigned int substrate_terminal_geometry_index = 7;
  define_terminal (device, db::DeviceClassBJT4Transistor::terminal_id_S, substrate_terminal_geometry_index, emitter);
}

// ---------------------------------------------------------------------------------
//  NetlistDeviceExtractorDiode implementation

NetlistDeviceExtractorDiode::NetlistDeviceExtractorDiode (const std::string &name)
  : db::NetlistDeviceExtractor (name)
{
  //  .. nothing yet ..
}

void NetlistDeviceExtractorDiode::setup ()
{
  define_layer ("P", "P region");                   // #0
  define_layer ("N", "N region");                   // #1
  define_layer ("tA", 0, "A terminal output");      // #2 -> P
  define_layer ("tC", 1, "C terminal output");      // #3 -> N

  register_device_class (new db::DeviceClassDiode ());
}

db::Connectivity NetlistDeviceExtractorDiode::get_connectivity (const db::Layout & /*layout*/, const std::vector<unsigned int> &layers) const
{
  tl_assert (layers.size () >= 2);

  unsigned int pregion = layers [0];
  unsigned int nregion = layers [1];

  //  The layer definition is plate1, plate2
  db::Connectivity conn;
  //  collect all connected plate 1 shapes
  conn.connect (pregion, pregion);
  //  collect all connected plate 1 shapes
  conn.connect (nregion, nregion);
  //  connect the plates (NOTE that this is a logical, not a physical connection)
  conn.connect (pregion, nregion);
  return conn;
}

void NetlistDeviceExtractorDiode::extract_devices (const std::vector<db::Region> &layer_geometry)
{
  size_t pregion_geometry_index = 0;
  size_t nregion_geometry_index = 1;
  size_t a_terminal_geometry_index = 2;
  size_t c_terminal_geometry_index = 3;

  const db::Region &pregion = layer_geometry [pregion_geometry_index];
  const db::Region &nregion = layer_geometry [nregion_geometry_index];

  db::Region overlap (pregion);
  overlap.set_base_verbosity (pregion.base_verbosity ());
  overlap &= nregion;

  for (db::Region::const_iterator p = overlap.begin_merged (); !p.at_end (); ++p) {

    db::Device *device = create_device ();

    device->set_trans (db::DCplxTrans ((p->box ().center () - db::Point ()) * dbu ()));

    double area = p->area () * dbu () * dbu ();

    device->set_parameter_value (db::DeviceClassDiode::param_id_A, area);
    device->set_parameter_value (db::DeviceClassDiode::param_id_P, dbu () * p->perimeter ());

    define_terminal (device, db::DeviceClassDiode::terminal_id_A, a_terminal_geometry_index, *p);
    define_terminal (device, db::DeviceClassDiode::terminal_id_C, c_terminal_geometry_index, *p);

    //  allow derived classes to modify the device
    modify_device (*p, layer_geometry, device);

    //  output the device for debugging
    device_out (device, *p);

  }
}

}
