
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


#include "layRedrawLayerInfo.h"
#include "layLayerProperties.h"

namespace lay 
{

// -------------------------------------------------------------
//  Implementation of RedrawLayerInfo

RedrawLayerInfo::RedrawLayerInfo (const lay::LayerProperties &lp)
{
  visible          = lp.visible (true /*real*/);
  cell_frame       = lp.is_cell_box_layer ();
  xfill            = lp.xfill (true /*real*/);
  layer_index      = lp.layer_index ();
  cellview_index   = lp.cellview_index ();
  trans            = lp.trans ();
  hier_levels      = lp.hier_levels ();
  prop_sel         = lp.prop_sel ();
  inverse_prop_sel = lp.inverse_prop_sel ();
  enabled          = true;
}

}

