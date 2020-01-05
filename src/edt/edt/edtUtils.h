
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


#ifndef HDR_edtUtils
#define HDR_edtUtils

#include <limits>
#include <list>
#include <utility>

#include <QDialog>

#include "dbInstElement.h"
#include "dbClipboardData.h"
#include "dbClipboard.h"
#include "dbPCellDeclaration.h"

namespace lay
{
  class LayoutView;
}

namespace edt {

/**
 *  @brief Fetch PCell parameters from a cell and merge the guiding shapes into them
 *
 *  @param layout The layout object
 *  @param cell_index The index of the cell from which to fetch the parameters
 *  @param parameters_for_pcell Will receive the parameters
 *  @return true, if the cell is a PCell and parameters have been fetched 
 */
bool
get_parameters_from_pcell_and_guiding_shapes (db::Layout *layout, db::cell_index_type cell_index, db::pcell_parameters_type &parameters_for_pcell);

/**
 *  @brief A helper class that identifies clipboard data for edt::
 */
class ClipboardData
  : public db::ClipboardData
{
public:
  ClipboardData () { }
};

/**
 *  @brief A cache for the transformation variants for a certain layer and cell view index for a lay::LayoutView
 */
class TransformationVariants
{
public:
  TransformationVariants (const lay::LayoutView *view, bool per_cv_and_layer = true, bool per_cv = true);

  const std::vector<db::DCplxTrans> *per_cv_and_layer (unsigned int cv, unsigned int layer) const;
  const std::vector<db::DCplxTrans> *per_cv (unsigned int cv) const;

private:
  std::map <unsigned int, std::vector<db::DCplxTrans> > m_per_cv_tv;
  std::map < std::pair<unsigned int, unsigned int>, std::vector<db::DCplxTrans> > m_per_cv_and_layer_tv;
};

} // namespace edt

#endif

