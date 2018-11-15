
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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


#include "dbHierarchyBuilder.h"
#include "dbReader.h"
#include "dbTestSupport.h"
#include "dbRegion.h"
#include "tlUnitTest.h"
#include "tlStream.h"

TEST(1)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/hierarchy_builder_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Layout target;
  db::HierarchyBuilder builder (&target, false);

  for (db::Layout::layer_iterator li = ly.begin_layers (); li != ly.end_layers (); ++li) {

    unsigned int li1 = (*li).first;
    unsigned int target_layer = target.insert_layer (*(*li).second);
    builder.set_target_layer (target_layer);

    db::cell_index_type top_cell_index = *ly.begin_top_down ();
    db::RecursiveShapeIterator iter (ly, ly.cell (top_cell_index), li1);

    iter.push (&builder);

  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/hierarchy_builder_au1.gds");
}

TEST(2_WithoutClip)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/hierarchy_builder_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Layout target;
  db::HierarchyBuilder builder (&target, false);

  db::cell_index_type target_top = target.add_cell ("CLIP_TOP");

  for (db::Layout::layer_iterator li = ly.begin_layers (); li != ly.end_layers (); ++li) {

    builder.reset ();

    unsigned int li1 = (*li).first;
    unsigned int target_layer = target.insert_layer (*(*li).second);
    builder.set_target_layer (target_layer);

    db::cell_index_type top_cell_index = *ly.begin_top_down ();
    db::RecursiveShapeIterator iter (ly, ly.cell (top_cell_index), li1, db::Box (5000, -2000, 18500, 6000));

    iter.push (&builder);

    target.cell (target_top).insert (db::CellInstArray (db::CellInst (builder.initial_cell ()->cell_index ()), db::Trans ()));

  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/hierarchy_builder_au2a.gds");
}

TEST(2_WithClip)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/hierarchy_builder_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Layout target;
  db::ClippingHierarchyBuilderShapeReceiver clip;
  db::HierarchyBuilder builder (&target, &clip);

  db::cell_index_type target_top = target.add_cell ("CLIP_TOP");

  for (db::Layout::layer_iterator li = ly.begin_layers (); li != ly.end_layers (); ++li) {

    builder.reset ();

    unsigned int li1 = (*li).first;
    unsigned int target_layer = target.insert_layer (*(*li).second);
    builder.set_target_layer (target_layer);

    db::cell_index_type top_cell_index = *ly.begin_top_down ();
    db::RecursiveShapeIterator iter (ly, ly.cell (top_cell_index), li1, db::Box (5000, -2000, 18500, 6000));

    iter.push (&builder);

    target.cell (target_top).insert (db::CellInstArray (db::CellInst (builder.initial_cell ()->cell_index ()), db::Trans ()));

  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/hierarchy_builder_au2b.gds");
}

TEST(2_WithClipAndSimplification)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/hierarchy_builder_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Layout target;
  db::ReducingHierarchyBuilderShapeReceiver red(0, 1.2, 4);
  db::ClippingHierarchyBuilderShapeReceiver clip(&red);
  db::HierarchyBuilder builder (&target, &clip);

  db::cell_index_type target_top = target.add_cell ("CLIP_TOP");

  for (db::Layout::layer_iterator li = ly.begin_layers (); li != ly.end_layers (); ++li) {

    builder.reset ();

    unsigned int li1 = (*li).first;
    unsigned int target_layer = target.insert_layer (*(*li).second);
    builder.set_target_layer (target_layer);

    db::cell_index_type top_cell_index = *ly.begin_top_down ();
    db::RecursiveShapeIterator iter (ly, ly.cell (top_cell_index), li1, db::Box (5000, -2000, 18500, 6000));

    iter.push (&builder);

    target.cell (target_top).insert (db::CellInstArray (db::CellInst (builder.initial_cell ()->cell_index ()), db::Trans ()));

  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/hierarchy_builder_au2c.gds");
}

TEST(2_WithClipAndRefGeneration)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/hierarchy_builder_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Layout target;
  db::PolygonReferenceHierarchyBuilderShapeReceiver ref(&target);
  db::ClippingHierarchyBuilderShapeReceiver clip(&ref);
  db::HierarchyBuilder builder (&target, &clip);

  db::cell_index_type target_top = target.add_cell ("CLIP_TOP");

  for (db::Layout::layer_iterator li = ly.begin_layers (); li != ly.end_layers (); ++li) {

    builder.reset ();

    unsigned int li1 = (*li).first;
    unsigned int target_layer = target.insert_layer (*(*li).second);
    builder.set_target_layer (target_layer);

    db::cell_index_type top_cell_index = *ly.begin_top_down ();
    db::RecursiveShapeIterator iter (ly, ly.cell (top_cell_index), li1, db::Box (5000, -2000, 18500, 6000));

    iter.push (&builder);

    target.cell (target_top).insert (db::CellInstArray (db::CellInst (builder.initial_cell ()->cell_index ()), db::Trans ()));

  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/hierarchy_builder_au2d.gds");
}

TEST(2_WithEmptyResult)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/hierarchy_builder_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Layout target;
  db::PolygonReferenceHierarchyBuilderShapeReceiver ref(&target);
  db::ClippingHierarchyBuilderShapeReceiver clip(&ref);
  db::HierarchyBuilder builder (&target, &clip);

  db::cell_index_type target_top = target.add_cell ("CLIP_TOP");

  for (db::Layout::layer_iterator li = ly.begin_layers (); li != ly.end_layers (); ++li) {

    builder.reset ();

    unsigned int li1 = (*li).first;
    unsigned int target_layer = target.insert_layer (*(*li).second);
    builder.set_target_layer (target_layer);

    db::cell_index_type top_cell_index = *ly.begin_top_down ();
    db::RecursiveShapeIterator iter (ly, ly.cell (top_cell_index), li1, db::Box (5000, 10000, 18500, 15000));

    iter.push (&builder);

    target.cell (target_top).insert (db::CellInstArray (db::CellInst (builder.initial_cell ()->cell_index ()), db::Trans ()));

  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/hierarchy_builder_au2e.gds");
}

TEST(3_ComplexRegionWithClip)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/hierarchy_builder_l2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Layout target;
  db::ClippingHierarchyBuilderShapeReceiver clip;
  db::HierarchyBuilder builder (&target, &clip);

  db::cell_index_type target_top = target.add_cell ("CLIP_TOP");

  for (db::Layout::layer_iterator li = ly.begin_layers (); li != ly.end_layers (); ++li) {

    builder.reset ();

    unsigned int li1 = (*li).first;
    unsigned int target_layer = target.insert_layer (*(*li).second);
    builder.set_target_layer (target_layer);

    db::cell_index_type top_cell_index = *ly.begin_top_down ();
    db::Region reg;
    reg.insert (db::Box (5000, 13000, 18500, 20000));
    reg.insert (db::Box (11000, 20000, 18500, 36000));
    reg.merge ();
    db::RecursiveShapeIterator iter (ly, ly.cell (top_cell_index), li1, reg);

    iter.push (&builder);

    target.cell (target_top).insert (db::CellInstArray (db::CellInst (builder.initial_cell ()->cell_index ()), db::Trans ()));

  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/hierarchy_builder_au3a.gds");
}

TEST(4_ComplexRegionAndLayoutWithClip)
{
  db::Layout ly;
  {
    std::string fn (tl::testsrc ());
    fn += "/testdata/algo/hierarchy_builder_l3.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Layout target;
  db::ClippingHierarchyBuilderShapeReceiver clip;
  db::HierarchyBuilder builder (&target, &clip);

  db::cell_index_type target_top = target.add_cell ("CLIP_TOP");

  for (db::Layout::layer_iterator li = ly.begin_layers (); li != ly.end_layers (); ++li) {

    builder.reset ();

    unsigned int li1 = (*li).first;
    unsigned int target_layer = target.insert_layer (*(*li).second);
    builder.set_target_layer (target_layer);

    db::cell_index_type top_cell_index = *ly.begin_top_down ();
    db::Region reg;
    reg.insert (db::Box (5000, 13000, 18500, 20000));
    reg.insert (db::Box (11000, 20000, 18500, 36000));
    reg.merge ();
    db::RecursiveShapeIterator iter (ly, ly.cell (top_cell_index), li1, reg);

    iter.push (&builder);

    target.cell (target_top).insert (db::CellInstArray (db::CellInst (builder.initial_cell ()->cell_index ()), db::Trans ()));

  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testsrc () + "/testdata/algo/hierarchy_builder_au4a.gds");
}

