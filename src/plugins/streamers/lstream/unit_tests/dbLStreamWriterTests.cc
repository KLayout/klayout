
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

#include "lstrReader.h"
#include "dbLayoutDiff.h"
#include "dbWriter.h"
#include "dbTestSupport.h"
#include "tlUnitTest.h"

#include <stdlib.h>

static void run_test (tl::TestBase *_this, const std::string &base, const char *file, const char *file_au)
{
  db::LoadLayoutOptions options;

  db::Manager m (false);
  db::Layout layout (&m);

  //  the reader is tested in the reader tests
  {
    std::string fn (base);
    fn += "/lstream/";
    fn += file;
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (layout, options);
  }

  //  do a full spin

  std::string tmp_file = _this->tmp_file ("tmp.lstr");

  {
    tl::OutputStream stream (tmp_file);
    db::SaveLayoutOptions options;
    options.set_format ("LStream");
    db::Writer writer (options);
    writer.write (layout, stream);
  }

  db::Layout layout_read (&m);
  {
    tl::InputStream file (tmp_file);
    db::Reader reader (file);
    reader.read (layout_read);
  }

  std::string fn_au (base);
  fn_au += "/lstream/";
  fn_au += file_au;

  db::compare_layouts (_this, layout_read, fn_au, db::WriteOAS);
}

TEST(basic)
{
  run_test (_this, tl::testdata (), "basic.lstr", "basic_au.oas");
}

TEST(boxes)
{
  run_test (_this, tl::testdata (), "boxes.lstr", "boxes_au.oas");
}

TEST(cells)
{
  run_test (_this, tl::testdata (), "cells.lstr", "cells_au.oas");
}

TEST(cells_with_instances)
{
  run_test (_this, tl::testdata (), "cells_with_instances.lstr", "cells_with_instances_au.oas");
}

TEST(edge_pairs)
{
  run_test (_this, tl::testdata (), "edge_pairs.lstr", "edge_pairs_au.oas");
}

TEST(edges)
{
  run_test (_this, tl::testdata (), "edges.lstr", "edges_au.oas");
}

TEST(ghost_cells)
{
  run_test (_this, tl::testdata (), "ghost_cells.lstr", "ghost_cells_au.oas");
}

TEST(meta_data)
{
  run_test (_this, tl::testdata (), "meta_data.lstr", "meta_data_au.oas");
}

TEST(paths)
{
  run_test (_this, tl::testdata (), "paths.lstr", "paths_au.oas");
}

TEST(pcells)
{
  run_test (_this, tl::testdata (), "pcells.lstr", "pcells_au.oas");
}

TEST(points)
{
  run_test (_this, tl::testdata (), "points.lstr", "points_au.oas");
}

TEST(polygons)
{
  run_test (_this, tl::testdata (), "polygons.lstr", "polygons_au.oas");
}

TEST(properties)
{
  run_test (_this, tl::testdata (), "properties.lstr", "properties_au.oas");
}

TEST(simple_polygons)
{
  run_test (_this, tl::testdata (), "simple_polygons.lstr", "simple_polygons_au.oas");
}

TEST(texts)
{
  run_test (_this, tl::testdata (), "texts.lstr", "texts_au.oas");
}

TEST(variants)
{
  run_test (_this, tl::testdata (), "variants.lstr", "variants_au.oas");
}

TEST(sample1)
{
  run_test (_this, tl::testdata (), "sample1.lstr", "sample1_au.oas");
}

TEST(sample2)
{
  run_test (_this, tl::testdata (), "sample2.lstr", "sample2_au.oas");
}

