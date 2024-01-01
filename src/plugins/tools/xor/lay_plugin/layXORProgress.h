
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#ifndef HDR_layXORToolProgress
#define HDR_layXORToolProgress

#include "tlProgress.h"

#include "dbTypes.h"
#include "dbLayerProperties.h"

#include <map>
#include <vector>
#include <limits>

class QWidget;

namespace lay
{

const size_t missing_in_a = std::numeric_limits<size_t>::max ();
const size_t missing_in_b = std::numeric_limits<size_t>::max () - 1;

/**
 *  @brief A specialized progress reporter for the XOR feature
 *
 *  The purpose of this class is to provide the special XOR progress widget that
 *  shows the XOR progress with numbers and a map (in tiled mode).
 */
class XORProgress
  : public tl::RelativeProgress
{
public:
  XORProgress (const std::string &title, size_t max_count, size_t yield_interval);

  virtual QWidget *progress_widget () const;
  virtual void render_progress (QWidget *widget) const;

  void configure (double dbu, int nx, int ny, const std::vector<db::Coord> &tol);
  void merge_results (std::map<std::pair<db::LayerProperties, db::Coord>, std::vector<std::vector<size_t> > > &results);

private:
  std::map<std::pair<db::LayerProperties, db::Coord>, std::vector<std::vector<size_t> > > m_results;
  std::map<db::LayerProperties, size_t> m_count_per_layer;
  std::vector<db::Coord> m_tolerances;
  bool m_needs_update;
  double m_dbu;
  int m_nx, m_ny;
};

}

#endif
