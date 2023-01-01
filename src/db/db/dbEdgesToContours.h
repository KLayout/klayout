
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


#ifndef HDR_dbEdgesToContours
#define HDR_dbEdgesToContours

#include "dbPoint.h"
#include "dbEdge.h"
#include "dbVector.h"

#include <iterator>

namespace tl {
  class RelativeProgress;
}

namespace db {

/**
 *  @brief A facility to create contours from edges
 *  
 *  This object will convert a set of edges to contours. "contours" are sequences of points, not
 *  necessarily closed ones. Contours may also be holes or outer contours - this object is not
 *  capable of making that distinction.
 *
 *  The use of this objects is to first fill it with edges and then deliver the contours
 *  collected in the fill step.
 */
class DB_PUBLIC EdgesToContours
{
public:
  EdgesToContours ();

  size_t contours () const
  {
    return m_contours.size ();
  }

  const std::vector<db::Point> &contour (size_t i) const;

  bool contour_closed (size_t i) const
  {
    return m_contours_closed [i];
  }

  template <class Iter>
  void fill (Iter from, Iter to, bool no = false, typename std::iterator_traits<Iter>::value_type::coord_type distance = 0, tl::RelativeProgress *progress = 0);

private:
  std::vector<std::vector <db::Point> > m_contours;
  std::vector<bool> m_contours_closed;
};

} // namespace db

#endif

