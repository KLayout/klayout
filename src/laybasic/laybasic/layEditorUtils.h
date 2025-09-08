
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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


#ifndef HDR_layEditorUtils
#define HDR_layEditorUtils

#include <limits>
#include <list>
#include <utility>
#include <vector>

#include "laybasicCommon.h"

#include "layObjectInstPath.h"
#include "laySnap.h"

#include "dbInstElement.h"
#include "dbClipboardData.h"
#include "dbClipboard.h"
#include "dbPCellDeclaration.h"

namespace lay
{

class LayoutViewBase;
class Dispatcher;

// -------------------------------------------------------------

/**
 *  @brief Gets the snap range in pixels
 */
LAYBASIC_PUBLIC int snap_range_pixels ();

/**
 *  @brief Convert a button flag set to an angle constraint
 *
 *  This implements the standard modifiers for angle constraints - i.e.
 *  ortho for "Shift".
 */
LAYBASIC_PUBLIC lay::angle_constraint_type ac_from_buttons (unsigned int buttons);

/**
 *  @brief Serializes PCell parameters to a string
 */
LAYBASIC_PUBLIC std::string pcell_parameters_to_string (const std::map<std::string, tl::Variant> &parameters);

/**
 *  @brief Deserializes PCell parameters from a string
 */
LAYBASIC_PUBLIC std::map<std::string, tl::Variant> pcell_parameters_from_string (const std::string &s);

/**
 *  @brief Fetch PCell parameters from a cell and merge the guiding shapes into them
 *
 *  @param layout The layout object
 *  @param cell_index The index of the cell from which to fetch the parameters
 *  @param parameters_for_pcell Will receive the parameters
 *  @return true, if the cell is a PCell and parameters have been fetched 
 */
LAYBASIC_PUBLIC
bool
get_parameters_from_pcell_and_guiding_shapes (db::Layout *layout, db::cell_index_type cell_index, db::pcell_parameters_type &parameters_for_pcell);


/**
 *  @brief Request to make the given layer the current one (asks whether to create the layer if needed)
 */
LAYBASIC_PUBLIC
bool
set_or_request_current_layer (lay::LayoutViewBase *view, const db::LayerProperties &lp, unsigned int cv_index, bool make_current = true);

/**
 *  @brief A cache for the transformation variants for a certain layer and cell view index for a lay::LayoutView
 */
class LAYBASIC_PUBLIC TransformationVariants
{
public:
  TransformationVariants (const lay::LayoutViewBase *view, bool per_cv_and_layer = true, bool per_cv = true);

  const std::vector<db::DCplxTrans> *per_cv_and_layer (unsigned int cv, unsigned int layer) const;
  const std::vector<db::DCplxTrans> *per_cv (unsigned int cv) const;

private:
  std::map <unsigned int, std::vector<db::DCplxTrans> > m_per_cv_tv;
  std::map < std::pair<unsigned int, unsigned int>, std::vector<db::DCplxTrans> > m_per_cv_and_layer_tv;
};

} // namespace edt

#endif

