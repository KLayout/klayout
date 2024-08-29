
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

#include "dbBinnedAreaCollector.h"
#include "dbTypes.h"
#include "dbRegion.h"

#include "gsiDecl.h"

#include <vector>

namespace db
{

namespace
{

class AreaReceiver
  : public db::binned_area_receiver<unsigned int>
{
public:
  typedef db::coord_traits<db::Coord>::area_type area_type;

  AreaReceiver (unsigned int count)
  {
    m_areas.resize (count, 0.0);
  }

  virtual void add_area (area_type area, const unsigned int &index)
  {
    m_areas [index] += area;
  }

  const std::vector<area_type> &get () const
  {
    return m_areas;
  }

private:
  std::vector<area_type> m_areas;
};

}

//  NOTE: this does not belong here. It is an experimental feature

static std::vector<AreaReceiver::area_type>
binned_area (const std::vector<const db::Region *> &inputs, const std::vector<std::string> &vectors)
{
  db::EdgeProcessor ep;

  unsigned int index = 0;
  for (auto r = inputs.begin (); r != inputs.end (); ++r, ++index) {
    for (auto p = (*r)->begin (); ! p.at_end (); ++p) {
      ep.insert (*p, index);
    }
  }

  tl::bit_set_map<unsigned int> bsm;
  index = 0;
  for (auto i = vectors.begin (); i != vectors.end (); ++i, ++index) {
    bsm.insert (tl::BitSetMask (*i), index);
  }
  bsm.sort ();

  AreaReceiver rec (index);
  db::binned_area_collector<unsigned int> coll (bsm, rec);
  ep.process (coll, coll);

  return rec.get ();
}

gsi::ClassExt<db::Region> extend_region_by_binned_area (
  gsi::method ("binned_area", &binned_area, gsi::arg ("inputs"), gsi::arg ("masks"),
    "@brief Computes the areas of a binned decomposition of the overall region.\n"
    "In this function, the overall region is decomposed into subregions with different overlap situations. "
    "Each overlap case is assigned a bin using a bit mask from the 'masks' argument. "
    "Each bit corresponds to one input from 'inputs' - bit 0 is the first one etc.\n"
    "The masks are strings of characters 0, 1 or 'X', representing 'inside', 'outside' and "
    "'any' for the respective input. The first character represents the first input, the second the second input etc.\n"
    "Missing characters are treated as 'any', so the empty string matches every situation.\n"
    "\n"
    "The result is a vector of accumulated areas for each bin identified by one mask. "
    "Bins may overlay if multiple masks match, so the total sum of areas is not necessarily "
    "identical to the total area. A bin with an empty string mask will deliver the total area.\n"
    "\n"
    "Merge semantics always applies - i.e. all shapes inside the regions are conceptually "
    "merged in 'positive wrap count' mode before computing the area. Hence overlapping shapes ""
    "per input region just count once.\n"
    "\n"
    "Example:\n"
    "\n"
    "@code\n"
    "r1 = RBA::Region::new\n"
    "r1.insert(RBA::Box::new(0, 0, 1000, 2000))\n"
    "\n"
    "r2 = RBA::Region::new\n"
    "r2.insert(RBA::Box::new(500, 1000, 1500, 3000))\n"
    "\n"
    "areas = RBA::Region::binned_area([ r1, r2 ], [ \"10\", \"01\", \"\" ])\n"
    "r1_not_r2, r2_not_r1, all = areas\n"
    "@/code\n"
    "\n"
    "This feature is highly experimental."
  )
);

}
