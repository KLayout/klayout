
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


#ifndef HDR_layRedrawLayerInfo
#define HDR_layRedrawLayerInfo

#include <vector>
#include "dbTrans.h"
#include "dbPropertiesRepository.h"
#include "layParsedLayerSource.h"

namespace lay {

class LayerProperties;

/**
 *  @brief A helper struct to describe one entry in the redraw queue
 */
struct RedrawLayerInfo
{
  RedrawLayerInfo (const lay::LayerProperties &lp);

  /**
   *  @brief Enable redrawing
   *
   *  If this flag is true (the default), redrawing is enabled. Otherwise this layer is skipped.
   */
  bool enabled;

  /**
   *  @brief Visible layer
   *
   *  If this flag is true, the layer is visible. Visible layers are drawn with 
   *  higher priority than the invisible ones. This flag is set by the constructor.
   */
  bool visible;

  /**
   *  @brief Cross-fill shapes
   *
   *  If this flag is true, this layer is supposed to use a diagonal cross
   *  added to the frame of boxes and polygons.
   */
  bool xfill;

  /**
   *  @brief Cell frame layer
   *
   *  If this flag is true, this layer is supposed to draw cell frames.
   *  It is set by the constructor if the source is a wildcard layer.
   */
  bool cell_frame;

  /**
   *  @brief The layer index
   *
   *  The logical layer to draw. The layer index can be <0 which indicates a
   *  layer with not layout source (cell_frame may be true to indicate a
   *  pseudo layer then).
   *  This attribute is set by the constructor.
   */
  int layer_index;

  /**
   *  @brief The cellview index
   *
   *  Set layer_index for details about the interpretation of this member.
   *  This member is set by the constructor.
   */
  int cellview_index;

  /**
   *  @brief A set of transformations applied for this layer
   *
   *  This member is set by the constructor.
   */
  std::vector<db::DCplxTrans> trans;

  /**
   *  @brief The hierarchy levels drawn
   *
   *  This member is set by the constructor.
   */
  lay::HierarchyLevelSelection hier_levels;

  /**
   *  @brief The property selection applicable for this layer
   *
   *  This member is set by the constructor.
   */
  std::set <db::properties_id_type> prop_sel;

  /** 
   *  @brief Invert the property selection
   *
   *  This member is set by the constructor.
   */
  bool inverse_prop_sel;

  /**
   *  @brief Returns true, if the layer needs to be drawn
   */
  bool needs_drawing () const
  {
    return visible && enabled && (cell_frame || layer_index >= 0) && cellview_index >= 0;
  }
};

}

#endif
