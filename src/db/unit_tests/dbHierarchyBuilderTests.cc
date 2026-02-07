
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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
    std::string fn (tl::testdata ());
    fn += "/algo/hierarchy_builder_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Layout target;
  db::HierarchyBuilder builder (&target);

  for (db::Layout::layer_iterator li = ly.begin_layers (); li != ly.end_layers (); ++li) {

    unsigned int li1 = (*li).first;
    unsigned int target_layer = target.insert_layer (*(*li).second);
    builder.set_target_layer (target_layer);

    db::cell_index_type top_cell_index = *ly.begin_top_down ();
    db::RecursiveShapeIterator iter (ly, ly.cell (top_cell_index), li1);

    iter.push (&builder);

  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/hierarchy_builder_au1.gds");
}

TEST(1_WithEmptyLayer)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/hierarchy_builder_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::cell_index_type top_cell_index = *ly.begin_top_down ();

  db::Layout target;
  db::HierarchyBuilder builder (&target);

  db::RecursiveShapeIterator iter (ly, ly.cell (top_cell_index), std::set<unsigned int> ());
  iter.push (&builder);

  for (db::Layout::layer_iterator li = ly.begin_layers (); li != ly.end_layers (); ++li) {

    unsigned int li1 = (*li).first;
    unsigned int target_layer = target.insert_layer (*(*li).second);
    builder.set_target_layer (target_layer);

    db::RecursiveShapeIterator iter (ly, ly.cell (top_cell_index), li1);

    iter.push (&builder);

  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/hierarchy_builder_au1.gds");
}

TEST(2_WithoutClip)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/hierarchy_builder_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Layout target;
  db::HierarchyBuilder builder (&target);

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
  db::compare_layouts (_this, target, tl::testdata () + "/algo/hierarchy_builder_au2a.gds");
}

TEST(2_WithClip)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/hierarchy_builder_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Layout target;
  db::ClippingHierarchyBuilderShapeReceiver clip;
  db::HierarchyBuilder builder (&target, db::ICplxTrans (), &clip);

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
  db::compare_layouts (_this, target, tl::testdata () + "/algo/hierarchy_builder_au2b.gds");
}

TEST(2_WithClipAndSimplification)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/hierarchy_builder_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Layout target;
  db::ReducingHierarchyBuilderShapeReceiver red(0, 1.2, 4);
  db::ClippingHierarchyBuilderShapeReceiver clip(&red);
  db::HierarchyBuilder builder (&target, db::ICplxTrans (), &clip);

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
  db::compare_layouts (_this, target, tl::testdata () + "/algo/hierarchy_builder_au2c.gds");
}

TEST(2_WithClipAndRefGeneration)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/hierarchy_builder_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Layout target;
  db::PolygonReferenceHierarchyBuilderShapeReceiver ref(&target, 0);
  db::ClippingHierarchyBuilderShapeReceiver clip(&ref);
  db::HierarchyBuilder builder (&target, db::ICplxTrans (), &clip);

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
  db::compare_layouts (_this, target, tl::testdata () + "/algo/hierarchy_builder_au2d.gds");
}

TEST(2_WithEmptyResult)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/hierarchy_builder_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Layout target;
  db::PolygonReferenceHierarchyBuilderShapeReceiver ref(&target, 0);
  db::ClippingHierarchyBuilderShapeReceiver clip(&ref);
  db::HierarchyBuilder builder (&target, db::ICplxTrans (), &clip);

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
  db::compare_layouts (_this, target, tl::testdata () + "/algo/hierarchy_builder_au2e.gds");
}

TEST(2_WithClipAndSimplificationAndEmptyLayer)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/hierarchy_builder_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Layout target;
  db::ReducingHierarchyBuilderShapeReceiver red(0, 1.2, 4);
  db::ClippingHierarchyBuilderShapeReceiver clip(&red);
  db::HierarchyBuilder builder (&target, db::ICplxTrans (), &clip);

  db::cell_index_type target_top = target.add_cell ("CLIP_TOP");

  db::Box clip_box (5000, -2000, 18500, 6000);

  builder.set_target_layer (target.insert_layer (db::LayerProperties (100, 0)));

  db::cell_index_type top_cell_index = *ly.begin_top_down ();
  db::RecursiveShapeIterator iter (ly, ly.cell (top_cell_index), std::set<unsigned int> (), clip_box);

  iter.push (&builder);

  target.cell (target_top).insert (db::CellInstArray (db::CellInst (builder.initial_cell ()->cell_index ()), db::Trans ()));

  for (db::Layout::layer_iterator li = ly.begin_layers (); li != ly.end_layers (); ++li) {

    builder.reset ();

    unsigned int li1 = (*li).first;
    unsigned int target_layer = target.insert_layer (*(*li).second);
    builder.set_target_layer (target_layer);

    db::cell_index_type top_cell_index = *ly.begin_top_down ();
    db::RecursiveShapeIterator iter (ly, ly.cell (top_cell_index), li1, clip_box);

    iter.push (&builder);

    target.cell (target_top).insert (db::CellInstArray (db::CellInst (builder.initial_cell ()->cell_index ()), db::Trans ()));

  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/hierarchy_builder_au2f.gds");
}

TEST(3_ComplexRegionWithClip)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/hierarchy_builder_l2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Layout target;
  db::ClippingHierarchyBuilderShapeReceiver clip;
  db::HierarchyBuilder builder (&target, db::ICplxTrans (), &clip);

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
  db::compare_layouts (_this, target, tl::testdata () + "/algo/hierarchy_builder_au3a.gds");
}

TEST(4_ComplexRegionAndLayoutWithClip)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/hierarchy_builder_l3.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Layout target;
  db::ClippingHierarchyBuilderShapeReceiver clip;
  db::HierarchyBuilder builder (&target, db::ICplxTrans (), &clip);

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
  db::compare_layouts (_this, target, tl::testdata () + "/algo/hierarchy_builder_au4a.gds");
}

TEST(5_CompareRecursiveShapeIterators)
{
  db::Layout ly;
  db::cell_index_type ci = ly.add_cell ("TOP");
  db::cell_index_type ci1 = ly.add_cell ("TOPA");

  db::Layout ly2;
  db::cell_index_type ci2 = ly2.add_cell ("TOP");

  {
    db::RecursiveShapeIterator iter1 (ly, ly.cell (ci), 0);
    db::RecursiveShapeIterator iter2 (ly2, ly2.cell (ci2), 0);
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2) != 0, true);
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2) != db::compare_iterators_with_respect_to_target_hierarchy (iter2, iter1), true);
  }

  {
    db::RecursiveShapeIterator iter1 (ly, ly.cell (ci), 0);
    db::RecursiveShapeIterator iter2 (ly, ly.cell (ci1), 0);
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2) != 0, true);
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2) != db::compare_iterators_with_respect_to_target_hierarchy (iter2, iter1), true);
  }

  {
    db::RecursiveShapeIterator iter1 (ly, ly.cell (ci), 0);
    db::RecursiveShapeIterator iter2 (ly, ly.cell (ci), 1);
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2), 0);
  }

  {
    std::vector<unsigned int> ll1;
    ll1.push_back (100);
    ll1.push_back (101);
    db::RecursiveShapeIterator iter1 (ly, ly.cell (ci), ll1);
    db::RecursiveShapeIterator iter2 (ly, ly.cell (ci), 1);
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2), 0);
  }

  {
    db::RecursiveShapeIterator iter1 (ly, ly.cell (ci), 0);
    iter1.max_depth (1);
    db::RecursiveShapeIterator iter2 (ly, ly.cell (ci), 0);
    iter2.max_depth (1);
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2), 0);

    iter2.max_depth (2);
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2) != 0, true);
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2) != db::compare_iterators_with_respect_to_target_hierarchy (iter2, iter1), true);
  }

  {
    db::RecursiveShapeIterator iter1 (ly, ly.cell (ci), 0, db::Box (0, 1000, 2000, 3000));
    db::RecursiveShapeIterator iter2 (ly, ly.cell (ci), 0);
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2) != 0, true);
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2) != db::compare_iterators_with_respect_to_target_hierarchy (iter2, iter1), true);
  }

  {
    db::RecursiveShapeIterator iter1 (ly, ly.cell (ci), 0, db::Box (0, 1000, 2000, 3000));
    db::RecursiveShapeIterator iter2 (ly, ly.cell (ci), 0, db::Box (0, 1000, 2000, 3000));
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2), 0);
  }

  {
    db::RecursiveShapeIterator iter1 (ly, ly.cell (ci), 0, db::Box (0, 1000, 2000, 3000));
    db::RecursiveShapeIterator iter2 (ly, ly.cell (ci), 1, db::Box (0, 1000, 2000, 3000));
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2) != 0, true);
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2) != db::compare_iterators_with_respect_to_target_hierarchy (iter2, iter1), true);
  }

  {
    std::vector<unsigned int> ll1;
    ll1.push_back (100);
    ll1.push_back (101);

    db::RecursiveShapeIterator iter1 (ly, ly.cell (ci), 0, db::Box (0, 1000, 2000, 3000));
    db::RecursiveShapeIterator iter2 (ly, ly.cell (ci), ll1, db::Box (0, 1000, 2000, 3000));
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2) != 0, true);
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2) != db::compare_iterators_with_respect_to_target_hierarchy (iter2, iter1), true);
  }

  {
    db::RecursiveShapeIterator iter1 (ly, ly.cell (ci), 0, db::Box (0, 1000, 2000, 3000));
    db::RecursiveShapeIterator iter2 (ly, ly.cell (ci), 0, db::Box (0, 1000, 2000, 3001));
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2) != 0, true);
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2) != db::compare_iterators_with_respect_to_target_hierarchy (iter2, iter1), true);
  }

  {
    db::Region r1;
    r1.insert (db::Box (0, 1000, 2000, 3000));
    db::RecursiveShapeIterator iter1 (ly, ly.cell (ci), 0, r1);
    db::RecursiveShapeIterator iter2 (ly, ly.cell (ci), 0, db::Box (0, 1000, 2000, 3000));
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2), 0);
  }

  {
    db::Region r1;
    r1.insert (db::Box (0, 1000, 2000, 3000));
    r1.insert (db::Box (0, 4000, 2000, 6000));
    db::RecursiveShapeIterator iter1 (ly, ly.cell (ci), 0, r1);
    db::RecursiveShapeIterator iter2 (ly, ly.cell (ci), 0, db::Box (0, 1000, 2000, 3000));
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2) != 0, true);
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2) != db::compare_iterators_with_respect_to_target_hierarchy (iter2, iter1), true);
  }

  {
    db::Region r1;
    r1.insert (db::Box (0, 1000, 2000, 3000));
    r1.insert (db::Box (0, 4000, 2000, 6000));
    db::RecursiveShapeIterator iter1 (ly, ly.cell (ci), 0, r1);
    db::Region r2;
    r2.insert (db::Box (0, 1000, 2000, 3000));
    r2.insert (db::Box (0, 4000, 2000, 6000));
    db::RecursiveShapeIterator iter2 (ly, ly.cell (ci), 0, r2);
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2), 0);
  }

  {
    db::Region r1;
    r1.insert (db::Box (0, 1000, 2000, 3000));
    r1.insert (db::Box (0, 4000, 2000, 6000));
    db::RecursiveShapeIterator iter1 (ly, ly.cell (ci), 0, r1);
    db::Region r2;
    r2.insert (db::Box (0, 1000, 2000, 3000));
    r2.insert (db::Box (0, 4000, 2000, 6001));
    db::RecursiveShapeIterator iter2 (ly, ly.cell (ci), 0, r2);
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2) != 0, true);
    EXPECT_EQ (db::compare_iterators_with_respect_to_target_hierarchy (iter1, iter2) != db::compare_iterators_with_respect_to_target_hierarchy (iter2, iter1), true);
  }
}

TEST(6_DisjunctLayersPerHierarchyBranch)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/hierarchy_builder_l4.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Layout target;
  db::HierarchyBuilder builder (&target);

  for (db::Layout::layer_iterator li = ly.begin_layers (); li != ly.end_layers (); ++li) {

    unsigned int li1 = (*li).first;
    unsigned int target_layer = target.insert_layer (*(*li).second);
    builder.set_target_layer (target_layer);

    db::cell_index_type top_cell_index = *ly.begin_top_down ();
    db::RecursiveShapeIterator iter (ly, ly.cell (top_cell_index), li1);

    iter.push (&builder);

  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/hierarchy_builder_au_l4.gds");
}

TEST(7_DetachFromOriginalLayout)
{
  //  using OASIS means we create a lot of references to array
  //  and shape repo - we check here whether these references get
  //  translated or resolved in the hierarchy builder.
  std::unique_ptr<db::Layout> ly (new db::Layout (false));
  {
    std::string fn (tl::testdata ());
    fn += "/algo/hierarchy_builder_l5.oas.gz";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (*ly);
  }

  db::Layout target;
  db::HierarchyBuilder builder (&target);

  for (db::Layout::layer_iterator li = ly->begin_layers (); li != ly->end_layers (); ++li) {

    unsigned int li1 = (*li).first;
    unsigned int target_layer = target.insert_layer (*(*li).second);
    builder.set_target_layer (target_layer);

    db::cell_index_type top_cell_index = *ly->begin_top_down ();
    db::RecursiveShapeIterator iter (*ly, ly->cell (top_cell_index), li1);

    iter.push (&builder);

  }

  //  make sure there is no connection to original layout
  ly.reset (0);

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/hierarchy_builder_au_l5.gds");
}

TEST(8a_SimpleWithTrans)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/hierarchy_builder_l1.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Layout target;
  db::HierarchyBuilder builder (&target, db::ICplxTrans (2.0, 45.0, false, db::Vector ()));

  for (db::Layout::layer_iterator li = ly.begin_layers (); li != ly.end_layers (); ++li) {

    unsigned int li1 = (*li).first;
    unsigned int target_layer = target.insert_layer (*(*li).second);
    builder.set_target_layer (target_layer);

    db::cell_index_type top_cell_index = *ly.begin_top_down ();
    db::RecursiveShapeIterator iter (ly, ly.cell (top_cell_index), li1);

    iter.push (&builder);

  }

  CHECKPOINT();
  db::compare_layouts (_this, target, tl::testdata () + "/algo/hierarchy_builder_au8a.gds");
}

TEST(8b_ComplexRegionWithTransformation)
{
  db::Layout ly;
  {
    std::string fn (tl::testdata ());
    fn += "/algo/hierarchy_builder_l2.gds";
    tl::InputStream stream (fn);
    db::Reader reader (stream);
    reader.read (ly);
  }

  db::Layout target;
  db::ClippingHierarchyBuilderShapeReceiver clip;
  db::HierarchyBuilder builder (&target, db::ICplxTrans (2.0, 45.0, false, db::Vector ()), &clip);

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
  db::compare_layouts (_this, target, tl::testdata () + "/algo/hierarchy_builder_au8b.gds");
}

