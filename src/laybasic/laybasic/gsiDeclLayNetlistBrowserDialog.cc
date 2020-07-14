
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


#include "gsiDecl.h"
#include "gsiDeclBasic.h"
#include "gsiSignals.h"
#include "layNetlistBrowserDialog.h"
#include "layLayoutView.h"

namespace tl
{

//  disable copy and default constructor for NetlistBrowserDialog
template <> struct type_traits<lay::NetlistBrowserDialog> : public type_traits<void>
{
  typedef tl::false_tag has_copy_constructor;
  typedef tl::false_tag has_default_constructor;
};

}

namespace gsi
{

static void set_root (lay::NetlistObjectPath *path, db::Circuit *r)
{
  path->root = r;
}

static db::Circuit *root (const lay::NetlistObjectPath *path)
{
  return const_cast<db::Circuit *> (path->root);
}

static void set_device (lay::NetlistObjectPath *path, db::Device *r)
{
  path->device = r;
}

static db::Device *device (const lay::NetlistObjectPath *path)
{
  return const_cast<db::Device *> (path->device);
}

static void set_net (lay::NetlistObjectPath *path, db::Net *r)
{
  path->net = r;
}

static db::Net *net (const lay::NetlistObjectPath *path)
{
  return const_cast<db::Net *> (path->net);
}

static std::vector<db::SubCircuit *> path (const lay::NetlistObjectPath *p)
{
  std::vector<db::SubCircuit *> pp;
  pp.reserve (p->path.size ());
  for (lay::NetlistObjectPath::path_iterator i = p->path.begin (); i != p->path.end (); ++i) {
    pp.push_back (const_cast<db::SubCircuit *> (*i));
  }
  return pp;
}

static void set_path (lay::NetlistObjectPath *p, const std::vector<db::SubCircuit *> &path)
{
  p->path = lay::NetlistObjectPath::path_type (path.begin (), path.end ());
}

Class<lay::NetlistObjectPath> decl_NetlistObjectPath ("lay", "NetlistObjectPath",
  gsi::method_ext ("root=", &set_root, gsi::arg ("root"),
    "@brief Sets the root circuit of the path.\n"
    "The root circuit is the circuit from which the path starts.\n"
  ) +
  gsi::method_ext ("root", &root,
    "@brief Gets the root circuit of the path.\n"
  ) +
  gsi::method_ext ("path=", &set_path, gsi::arg ("path"),
    "@brief Sets the path.\n"
    "The path is a list of subcircuits leading from the root to the final object. "
    "The final (net, device) object is located in the circuit called by the last subcircuit "
    "of the subcircuit chain. If the subcircuit list is empty, the final object is located inside "
    "the root object."
  ) +
  gsi::method_ext ("path", &path,
    "@brief Gets the path.\n"
  ) +
  gsi::method_ext ("net=", &set_net, gsi::arg ("net"),
    "@brief Sets the net the path points to.\n"
    "If the path describes the location of a net, this member will indicate it.\n"
    "The other way to describe a final object is \\device=. If neither a device nor "
    "net is given, the path describes a circuit and how it is referenced from the root."
  ) +
  gsi::method_ext ("net", &net,
    "@brief Gets the net the path points to.\n"
  ) +
  gsi::method_ext ("device=", &set_device, gsi::arg ("device"),
    "@brief Sets the device the path points to.\n"
    "If the path describes the location of a device, this member will indicate it.\n"
    "The other way to describe a final object is \\net=. If neither a device nor "
    "net is given, the path describes a circuit and how it is referenced from the root."
  ) +
  gsi::method_ext ("device", &device,
    "@brief Gets the device the path points to.\n"
  ) +
  gsi::method ("is_null?", &lay::NetlistObjectPath::is_null,
    "@brief Returns a value indicating whether the path is an empty one.\n"
  ),
  "@brief An object describing the instantiation of an object.\n"
  "This class describes the instantiation of a net or a device or a circuit in terms of "
  "a root circuit and a subcircuit chain leading to the indicated object.\n"
  "\n"
  "See \\net= or \\device= for the indicated object, \\path= for the subcircuit chain.\n"
  "\n"
  "This class has been introduced in version 0.27.\n"
);

static lay::NetlistObjectPath current_path_first (lay::NetlistBrowserDialog *dialog)
{
  return dialog->current_path ().first ();
}

static lay::NetlistObjectPath current_path_second (lay::NetlistBrowserDialog *dialog)
{
  return dialog->current_path ().second ();
}

Class<lay::NetlistBrowserDialog> decl_NetlistBrowserDialog ("lay", "NetlistBrowserDialog",
  gsi::event ("on_current_db_changed", &lay::NetlistBrowserDialog::current_db_changed_event,
    "@brief This event is triggered when the current database is changed.\n"
    "The current database can be obtained with \\db."
  ) +
  gsi::event ("on_selection_changed", &lay::NetlistBrowserDialog::selection_changed_event,
    "@brief This event is triggered when the selection changed.\n"
    "The selection can be obtained with \\current_path_first, \\current_path_second, \\selected_nets, \\selected_devices, \\selected_subcircuits and \\selected_circuits."
  ) +
  gsi::event ("on_probe", &lay::NetlistBrowserDialog::probe_event, gsi::arg ("first_path"), gsi::arg ("second_path"),
    "@brief This event is triggered when a net is probed.\n"
    "The first path will indicate the location of the probed net in terms of two paths: one describing the instantiation of the "
    "net in layout space and one in schematic space. Both objects are \\NetlistObjectPath objects which hold the root circuit, the "
    "chain of subcircuits leading to the circuit containing the net and the net itself."
  ) +
  gsi::method ("db", &lay::NetlistBrowserDialog::db,
    "@brief Gets the database the browser is connected to.\n"
  ) +
  gsi::method_ext ("current_path_first", &current_path_first,
    "@brief Gets the path of the current object on the first (layout in case of LVS database) side.\n"
  ) +
  gsi::method_ext ("current_path_second", &current_path_second,
    "@brief Gets the path of the current object on the second (schematic in case of LVS database) side.\n"
  ) +
  // @@@
  gsi::method ("selected_nets", &lay::NetlistBrowserDialog::selected_nets,
    "@brief Gets the nets currently selected in the netlist database browser.\n"
  ) +
  gsi::method ("selected_devices", &lay::NetlistBrowserDialog::selected_devices,
    "@brief Gets the devices currently selected in the netlist database browser.\n"
  ) +
  gsi::method ("selected_subcircuits", &lay::NetlistBrowserDialog::selected_subcircuits,
    "@brief Gets the subcircuits currently selected in the netlist database browser.\n"
  ) +
  gsi::method ("selected_circuits", &lay::NetlistBrowserDialog::selected_circuits,
    "@brief Gets the circuits currently selected in the netlist database browser.\n"
  ),
  "@brief Represents the netlist browser dialog.\n"
  "This dialog is a part of the \\LayoutView class and can be obtained through \\LayoutView#netlist_browser.\n"
  "This interface allows to interact with the browser - mainly to get information about state changes.\n"
  "\n"
  "This class has been introduced in version 0.27.\n"
);

static lay::NetlistBrowserDialog *netlist_browser (lay::LayoutView *lv)
{
  return lv->get_plugin<lay::NetlistBrowserDialog> ();
}

//  extend lay::LayoutView with the getter for the netlist browser
static
gsi::ClassExt<lay::LayoutView> decl_ext_layout_view (
  gsi::method_ext ("netlist_browser", &netlist_browser,
    "@brief Gets the netlist browser object for the given layout view\n"
    "\n"
    "\nThis method has been added in version 0.27.\n"
  )
);



}

