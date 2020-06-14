
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

Class<lay::NetlistBrowserDialog> decl_NetlistBrowserDialog ("lay", "NetlistBrowserDialog",
  gsi::event ("current_db_changed_event", &lay::NetlistBrowserDialog::current_db_changed_event,
    "@brief This event is triggered when the current database is changed.\n"
    "The current database can be obtained with \\db."
  ) +
  gsi::event ("selection_changed_event", &lay::NetlistBrowserDialog::selection_changed_event,
    "@brief This event is triggered when the selection changed.\n"
    "The selection can be obtained with \\selected_nets, \\selected_devices, \\selected_subcircuits and \\selected_circuits."
  ) +
  gsi::event ("probe_event", &lay::NetlistBrowserDialog::probe_event, gsi::arg ("net"), gsi::arg ("subcircuit_path"),
    "@brief This event is triggered when a net is probed.\n"
    "'subcircuit_path' will contain the subcircuit objects leading to the probed net from the netlist databases' top circuit. "
    "This path may not be complete - it may contain null entries if a cell instance can't be associated with a subcircuit."
  ) +
  gsi::method ("db", &lay::NetlistBrowserDialog::db,
    "@brief Gets the database the browser is connected to.\n"
  ) +
  gsi::method ("selected_nets", &lay::NetlistBrowserDialog::selected_nets,
    "@brief Gets the nets currently selected in the netlist database browser.\n"
  ) +
  gsi::method ("selected_nets", &lay::NetlistBrowserDialog::selected_devices,
    "@brief Gets the devices currently selected in the netlist database browser.\n"
  ) +
  gsi::method ("selected_nets", &lay::NetlistBrowserDialog::selected_subcircuits,
    "@brief Gets the subcircuits currently selected in the netlist database browser.\n"
  ) +
  gsi::method ("selected_nets", &lay::NetlistBrowserDialog::selected_circuits,
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

