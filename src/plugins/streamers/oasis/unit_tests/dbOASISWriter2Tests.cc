
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


#include "dbOASISReader.h"
#include "dbOASISWriter.h"
#include "dbShapeProcessor.h"

#include "tlUnitTest.h"

#include <stdlib.h>

// Test the writer's capabilities to write polygon's with holes
TEST(1)
{
  db::ShapeProcessor sp;

  db::Manager m (false);
  db::Layout layout_org (&m);
  {
    std::string fn (tl::testdata ());
    fn += "/other/d1.oas.gz";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout_org);
  }

  db::Cell &top_org = layout_org.cell (*layout_org.begin_top_down ());
  for (unsigned int i = 0; i < layout_org.layers (); ++i) {
    if (layout_org.is_valid_layer (i)) {
      sp.merge (layout_org, top_org, i, top_org.shapes (i), true, 0, false /*don't resolve holes*/);
    }
  }

  std::string tmp_file = tl::TestBase::tmp_file ("tmp_OASISWriter2.gds");

  {
    tl::OutputStream stream (tmp_file);
    db::OASISWriter writer;
    db::SaveLayoutOptions options;
    writer.write (layout_org, stream, options);
  }

  db::Layout layout_read (&m);
  {
    tl::InputStream file (tmp_file);
    db::Reader reader (file);
    reader.read (layout_read);
  }

  db::Cell &top_read = layout_read.cell (*layout_org.begin_top_down ());

  unsigned int xor_layer = layout_org.insert_layer (db::LayerProperties ());

  for (unsigned int i = 0; i < layout_org.layers (); ++i) {
    if (layout_org.is_valid_layer (i)) {
      const db::LayerProperties lp_org = layout_org.get_properties (i);
      for (unsigned int j = 0; j < layout_read.layers (); ++j) {
        if (layout_read.is_valid_layer (j) && layout_read.get_properties (j) == lp_org) {
          EXPECT_EQ (top_org.shapes (i).size () > 0, true);
          sp.boolean (layout_org, top_org, i, 
                      layout_read, top_read, j, 
                      top_org.shapes (xor_layer), db::BooleanOp::Xor, true, false); 
          sp.size (layout_org, top_org, xor_layer, top_org.shapes (xor_layer), db::Coord (-1), db::Coord (-1));
          EXPECT_EQ (top_org.shapes (xor_layer).size () == 0, true);
        }
      }
    }
  }
}

