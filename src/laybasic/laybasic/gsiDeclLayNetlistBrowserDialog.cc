
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
  gsi::Methods (),
  "@brief ..."
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

