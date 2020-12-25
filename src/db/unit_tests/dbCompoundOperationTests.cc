
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


#include "tlUnitTest.h"

#include "dbRegion.h"
#include "dbCompoundOperation.h"
#include "dbReader.h"
#include "dbRecursiveShapeIterator.h"
#include "dbTestSupport.h"

#include "tlStream.h"

#include <cstdio>

TEST(1_Basic)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/drc/compound_1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::RegionCheckOptions check_options;
  check_options.metrics = db::Projection;

  db::CompoundRegionCheckOperationNode width_check (db::WidthRelation, false /*==same polygon*/, 1050, check_options);

  unsigned int l1 = ly.get_layer (db::LayerProperties (1, 0));
  db::Region r (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l1));

  unsigned int l2 = ly.get_layer (db::LayerProperties (2, 0));
  db::Region r2 (db::RecursiveShapeIterator (ly, ly.cell (*ly.begin_top_down ()), l2));

  db::EdgePairs res = r.cop_to_edge_pairs (width_check);

  unsigned int l1000 = ly.get_layer (db::LayerProperties (1000, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1000);

  db::CompoundRegionOperationPrimaryNode *primary = new db::CompoundRegionOperationPrimaryNode ();
  db::CompoundRegionCheckOperationNode space_check (primary, db::SpaceRelation, true /*==different polygons*/, 1050, check_options);

  res = r.cop_to_edge_pairs (space_check);

  unsigned int l1001 = ly.get_layer (db::LayerProperties (1001, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1001);

  db::CompoundRegionOperationSecondaryNode *secondary = new db::CompoundRegionOperationSecondaryNode (&r2);
  db::CompoundRegionCheckOperationNode sep_check (secondary, db::SpaceRelation, true /*==different polygons*/, 1050, check_options);

  res = r.cop_to_edge_pairs (sep_check);

  unsigned int l1002 = ly.get_layer (db::LayerProperties (1002, 0));
  res.insert_into (&ly, *ly.begin_top_down (), l1002);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testsrc () + "/testdata/drc/compound_au1.gds");
}
