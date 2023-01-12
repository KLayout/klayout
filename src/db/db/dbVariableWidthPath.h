
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


#ifndef HDR_dbVariableWidthPath
#define HDR_dbVariableWidthPath

#include "dbPoint.h"
#include "dbPolygon.h"

#include <vector>
#include <algorithm>

namespace db
{

/**
 *  @brief A class representing a variable width path
 *
 *  A variable-width path is a path which has a non-constant width over it's
 *  length. A width can be assigned to certain points and will be interpolated
 *  for other points. Interpolation is performed along the length of the 
 *  path's spine. 
 *
 *  The initial and final width must be specified. A point can be assigned two
 *  widths: an incoming and an outgoing width. If one width is specified, the
 *  incoming and outgoing widths are the same.
 */
template <class C>
class DB_PUBLIC variable_width_path
{
public:
  typedef db::point<C> point_type;
  typedef db::simple_polygon<C> simple_polygon_type;
  typedef C width_type;
  typedef std::pair<size_t, width_type> width_spec_type;

  /**
   *  @brief Constructor from a set of points and width specifications
   *
   *  I is an iterator delivering point_type objects.
   *
   *  J is an iterator delivering width_spec_type objects.
   *
   *  The width specification is a list of point index and width. The 
   *  list must be sorted ascending by index. One index can be present
   *  twice. In this case, the first specification will be the incoming
   *  width, the second one will be the outgoing width.
   *
   *  The first element of the width specification needs to be 
   *  the initial width (0, w1) and the last element needs to be 
   *  the final width (n-1, w2) where n is the number of points.
   */
  template <class I, class J>
  variable_width_path (I b, I e, J bs, J es)
    : m_points (b, e), m_org_widths (bs, es)
  {
    init ();
  }

  /**
   *  @brief Constructor with a transformation
   */
  template <class I, class J, class T>
  variable_width_path (I b, I e, J bs, J es, const T &trans)
  {
    for (I i = b; i != e; ++i) {
      m_points.push_back (trans.trans (*i));
    }
    for (J j = bs; j != es; ++j) {
      m_org_widths.push_back (std::make_pair (j->first, trans.ctrans (j->second)));
    }

    init ();
  }

  /**
   *  @brief Turns the variable-width path into a polygon
   */
  simple_polygon_type to_poly () const;

private:
  void init ();

  std::vector<point_type> m_points;
  std::vector<std::pair<width_type, width_type> > m_widths;
  std::vector<std::pair<size_t, C> > m_org_widths;
};

/**
 *  @brief The integer-type variable-width path
 */
typedef variable_width_path<db::Coord> VariableWidthPath;

/**
 *  @brief The float-type variable-width path
 */
typedef variable_width_path<db::DCoord> DVariableWidthPath;

}

#endif


