
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
  define_layer ("SD", "Source/drain diffusion");
  define_layer ("G", "Gate");
  define_layer ("P", "Poly");

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
  const db::Region &rdiff = layer_geometry [0];
  const db::Region &rgates = layer_geometry [1];

  for (db::Region::const_iterator p = rgates.begin_merged (); !p.at_end (); ++p) {

    db::Region rgate (*p);
    rgate.set_base_verbosity (rgates.base_verbosity ());

    db::Region rdiff2gate = rdiff.selected_interacting (rgate);
    rdiff2gate.set_base_verbosity (rdiff.base_verbosity ());

    if (rdiff2gate.empty ()) {
      error (tl::to_string (tr ("Gate shape touches no diffusion - ignored")), *p);
    } else {

      unsigned int terminal_geometry_index = 0;
      unsigned int gate_geometry_index = 2;

      if (rdiff2gate.size () != 2) {
        error (tl::sprintf (tl::to_string (tr ("Expected two polygons on diff interacting one gate shape (found %d) - gate shape ignored")), int (rdiff2gate.size ())), *p);
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

      device->set_position (db::CplxTrans (dbu ()) * p->box ().center ());

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

        define_terminal (device, diff_index == 0 ? db::DeviceClassMOS3Transistor::terminal_id_S : db::DeviceClassMOS3Transistor::terminal_id_D, terminal_geometry_index, *d);

      }

      define_terminal (device, db::DeviceClassMOS3Transistor::terminal_id_G, gate_geometry_index, *p);

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
  define_layer ("SD", "Source/drain diffusion");
  define_layer ("G", "Gate");
  define_layer ("P", "Poly");
  define_layer ("W", "Well");

  register_device_class (new db::DeviceClassMOS4Transistor ());
}

void NetlistDeviceExtractorMOS4Transistor::modify_device (const db::Polygon &rgate, const std::vector<db::Region> & /*layer_geometry*/, db::Device *device)
{
  unsigned int well_geometry_index = 3;
  define_terminal (device, db::DeviceClassMOS4Transistor::terminal_id_B, well_geometry_index, rgate);
}

}
