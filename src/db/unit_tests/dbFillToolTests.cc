
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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


#include "dbFillTool.h"
#include "dbReader.h"
#include "dbRegion.h"
#include "dbTestSupport.h"
#include "dbRegion.h"
#include "tlUnitTest.h"
#include "tlStream.h"

TEST(1)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/fill_tool1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type fill_cell = ly.cell_by_name ("FILL_CELL").second;
  db::cell_index_type top_cell = ly.cell_by_name ("TOP").second;
  unsigned int fill_layer = ly.get_layer (db::LayerProperties (1, 0));

  db::Region fill_region (db::RecursiveShapeIterator (ly, ly.cell (top_cell), fill_layer));

  db::fill_region (&ly.cell (top_cell), fill_region, fill_cell, ly.cell (fill_cell).bbox (), db::Point (), false);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testdata () + "/algo/fill_tool_au1.gds");
}

TEST(2)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/fill_tool2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type fill_cell = ly.cell_by_name ("FILL_CELL").second;
  db::cell_index_type top_cell = ly.cell_by_name ("TOP").second;
  unsigned int fill_layer = ly.get_layer (db::LayerProperties (1, 0));

  db::Region fill_region (db::RecursiveShapeIterator (ly, ly.cell (top_cell), fill_layer));

  db::Region remaining_parts, remaining_polygons;

  db::fill_region (&ly.cell (top_cell), fill_region, fill_cell, ly.cell (fill_cell).bbox (), db::Point (), true, &remaining_parts, db::Vector (50, 100), &remaining_polygons);

  unsigned int l100 = ly.insert_layer (db::LayerProperties (100, 0));
  unsigned int l101 = ly.insert_layer (db::LayerProperties (101, 0));
  remaining_parts.insert_into (&ly, top_cell, l100);
  remaining_polygons.insert_into (&ly, top_cell, l101);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testdata () + "/algo/fill_tool_au2.gds");
}

TEST(3)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/fill_tool3.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type fill_cell = ly.cell_by_name ("FILL_CELL").second;
  db::cell_index_type top_cell = ly.cell_by_name ("TOP").second;
  unsigned int fill_layer = ly.get_layer (db::LayerProperties (1, 0));

  db::Region fill_region (db::RecursiveShapeIterator (ly, ly.cell (top_cell), fill_layer));

  db::Region remaining_parts, remaining_polygons;

  db::Vector ko (-100, -130);
  db::Vector rs (230, 40);
  db::Vector cs (40, 230);
  db::Box fc_box (db::Point () + ko, db::Point (rs.x (), cs.y ()) + ko);
  db::fill_region (&ly.cell (top_cell), fill_region, fill_cell, fc_box, rs, cs, db::Point (), true, &remaining_parts, db::Vector (50, 100), &remaining_polygons);

  unsigned int l100 = ly.insert_layer (db::LayerProperties (100, 0));
  unsigned int l101 = ly.insert_layer (db::LayerProperties (101, 0));
  remaining_parts.insert_into (&ly, top_cell, l100);
  remaining_polygons.insert_into (&ly, top_cell, l101);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testdata () + "/algo/fill_tool_au3.gds");
}

TEST(3a)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/fill_tool3.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type fill_cell = ly.cell_by_name ("FILL_CELL").second;
  db::cell_index_type top_cell = ly.cell_by_name ("TOP").second;
  unsigned int fill_layer = ly.get_layer (db::LayerProperties (1, 0));

  db::Region fill_region (db::RecursiveShapeIterator (ly, ly.cell (top_cell), fill_layer));

  db::Region remaining_parts, remaining_polygons;

  db::Vector ko (-100, -130);
  db::Vector rs (230, 40);
  db::Vector cs (-40, 230);
  db::Box fc_box (db::Point () + ko, db::Point (rs.x (), cs.y ()) + ko);
  db::fill_region (&ly.cell (top_cell), fill_region, fill_cell, fc_box, rs, cs, db::Point (), true, &remaining_parts, db::Vector (50, 100), &remaining_polygons);

  unsigned int l100 = ly.insert_layer (db::LayerProperties (100, 0));
  unsigned int l101 = ly.insert_layer (db::LayerProperties (101, 0));
  remaining_parts.insert_into (&ly, top_cell, l100);
  remaining_polygons.insert_into (&ly, top_cell, l101);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testdata () + "/algo/fill_tool_au3a.gds");
}

TEST(3b)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/fill_tool3.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type fill_cell = ly.cell_by_name ("FILL_CELL").second;
  db::cell_index_type top_cell = ly.cell_by_name ("TOP").second;
  unsigned int fill_layer = ly.get_layer (db::LayerProperties (1, 0));

  db::Region fill_region (db::RecursiveShapeIterator (ly, ly.cell (top_cell), fill_layer));

  db::Region remaining_parts, remaining_polygons;

  db::Vector ko (-100, -130);
  db::Vector rs (230, -40);
  db::Vector cs (40, 230);
  db::Box fc_box (db::Point () + ko, db::Point (rs.x (), cs.y ()) + ko);
  db::fill_region (&ly.cell (top_cell), fill_region, fill_cell, fc_box, rs, cs, db::Point (), true, &remaining_parts, db::Vector (50, 100), &remaining_polygons);

  unsigned int l100 = ly.insert_layer (db::LayerProperties (100, 0));
  unsigned int l101 = ly.insert_layer (db::LayerProperties (101, 0));
  remaining_parts.insert_into (&ly, top_cell, l100);
  remaining_polygons.insert_into (&ly, top_cell, l101);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testdata () + "/algo/fill_tool_au3b.gds");
}

TEST(3c)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/fill_tool3.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type fill_cell = ly.cell_by_name ("FILL_CELL").second;
  db::cell_index_type top_cell = ly.cell_by_name ("TOP").second;
  unsigned int fill_layer = ly.get_layer (db::LayerProperties (1, 0));

  db::Region fill_region (db::RecursiveShapeIterator (ly, ly.cell (top_cell), fill_layer));

  db::Region remaining_parts, remaining_polygons;

  db::Vector ko (-100, -130);
  db::Vector rs (230, -40);
  db::Vector cs (-40, 230);
  db::Box fc_box (db::Point () + ko, db::Point (rs.x (), cs.y ()) + ko);
  db::fill_region (&ly.cell (top_cell), fill_region, fill_cell, fc_box, rs, cs, db::Point (), true, &remaining_parts, db::Vector (50, 100), &remaining_polygons);

  unsigned int l100 = ly.insert_layer (db::LayerProperties (100, 0));
  unsigned int l101 = ly.insert_layer (db::LayerProperties (101, 0));
  remaining_parts.insert_into (&ly, top_cell, l100);
  remaining_polygons.insert_into (&ly, top_cell, l101);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testdata () + "/algo/fill_tool_au3c.gds");
}

TEST(4)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/fill_tool4.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type fill_cell = ly.cell_by_name ("FILL_CELL").second;
  db::cell_index_type top_cell = ly.cell_by_name ("TOP").second;
  unsigned int fill_layer = ly.get_layer (db::LayerProperties (1, 0));

  db::Region fill_region (db::RecursiveShapeIterator (ly, ly.cell (top_cell), fill_layer));

  db::Region remaining_polygons;

  db::Vector ko (-100, -130);
  db::Vector rs (230, 0);
  db::Vector cs (0, 230);
  db::Box fc_box (db::Point () + ko, db::Point (rs.x (), cs.y ()) + ko);
  db::fill_region_repeat (&ly.cell (top_cell), fill_region, fill_cell, fc_box, rs, cs, db::Vector (50, 100), &remaining_polygons);

  unsigned int l101 = ly.insert_layer (db::LayerProperties (101, 0));
  remaining_polygons.insert_into (&ly, top_cell, l101);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testdata () + "/algo/fill_tool_au4.gds");
}

TEST(4b)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/fill_tool4.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type fill_cell = ly.cell_by_name ("FILL_CELL").second;
  db::cell_index_type top_cell = ly.cell_by_name ("TOP").second;
  unsigned int fill_layer = ly.get_layer (db::LayerProperties (1, 0));

  db::Region fill_region (db::RecursiveShapeIterator (ly, ly.cell (top_cell), fill_layer));

  db::Region remaining_polygons;

  db::Vector ko (-100, -130);
  db::Vector rs (230, 0);
  db::Vector cs (0, 230);
  db::Box fc_box (db::Point () + ko, db::Point ());
  db::fill_region (&ly.cell (top_cell), fill_region, fill_cell, fc_box, rs, cs, db::Point (), true, &remaining_polygons);

  unsigned int l101 = ly.insert_layer (db::LayerProperties (101, 0));
  remaining_polygons.insert_into (&ly, top_cell, l101);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testdata () + "/algo/fill_tool_au4b.gds");
}

TEST(4c)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/fill_tool4.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type fill_cell = ly.cell_by_name ("FILL_CELL").second;
  db::cell_index_type top_cell = ly.cell_by_name ("TOP").second;
  unsigned int fill_layer = ly.get_layer (db::LayerProperties (1, 0));

  db::Region fill_region (db::RecursiveShapeIterator (ly, ly.cell (top_cell), fill_layer));

  db::Region remaining_polygons;

  db::Vector ko (-100, -130);
  db::Vector rs (230, 0);
  db::Vector cs (0, 230);
  db::Box fc_box (db::Point () + ko, db::Point (rs.x (), cs.y ()) + ko);
  db::fill_region (&ly.cell (top_cell), fill_region, fill_cell, fc_box.enlarged (db::Vector (100, 100)), rs, cs, db::Point (), true, &remaining_polygons);

  unsigned int l101 = ly.insert_layer (db::LayerProperties (101, 0));
  remaining_polygons.insert_into (&ly, top_cell, l101);

  CHECKPOINT();
  db::compare_layouts (_this, ly, tl::testdata () + "/algo/fill_tool_au4c.gds");
}
