
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


#include "gsiDecl.h"

#include "gsiDeclDbHelpers.h"
#include "dbShapes.h"
#include "dbShape.h"
#include "dbLayout.h"
#include "dbRegion.h"
#include "dbEdgePairs.h"
#include "dbEdges.h"

namespace gsi
{

// ---------------------------------------------------------------
//  db::Shapes binding

static double shapes_dbu (const db::Shapes *shapes)
{
  if (! shapes->layout ()) {
    throw tl::Exception (tl::to_string (tr ("The shapes do not reside inside a layout - cannot obtain database unit")));
  }
  return shapes->layout ()->dbu ();
}

static void dump_mem_statistics (const db::Shapes *shapes, bool detailed)
{
  db::MemStatisticsCollector ms (detailed);
  shapes->mem_stat (&ms, db::MemStatistics::ShapesInfo, 0);
  ms.print ();
}

static size_t shapes_size (const db::Shapes *shapes)
{
  //  we may have shape arrays - expand their count to match the shape count with the shapes delivered
  size_t n = 0;
  for (db::Shapes::shape_iterator i = shapes->begin (db::ShapeIterator::All); ! i.at_end (); ++i) {
    if (i.in_array ()) {
      n += i.array ().array_size ();
      i.finish_array ();
    } else {
      ++n;
    }
  }
  return n;
}

template<class Sh>
static db::Shape insert (db::Shapes *s, const Sh &p)
{
  return s->insert (p);
}

template<class Sh>
static db::Shape dinsert (db::Shapes *s, const Sh &p)
{
  return s->insert (db::CplxTrans (shapes_dbu (s)).inverted () * p);
}

template<class Sh>
static db::Shape replace (db::Shapes *s, const db::Shape &sh, const Sh &p)
{
  return s->replace (sh, p);
}

template<class Sh>
static db::Shape dreplace (db::Shapes *s, const db::Shape &sh, const Sh &p)
{
  return s->replace (sh, db::CplxTrans (shapes_dbu (s)).inverted () * p);
}

template<class Sh>
static db::Shape insert_with_properties (db::Shapes *s, const Sh &p, db::properties_id_type id)
{
  return s->insert (db::object_with_properties<Sh> (p, id));
}

template<class Sh, class ISh>
static db::Shape dinsert_with_properties (db::Shapes *s, const Sh &p, db::properties_id_type id)
{
  return s->insert (db::object_with_properties<ISh> (db::CplxTrans (shapes_dbu (s)).inverted () * p, id));
}

static gsi::layout_locking_iterator1<db::Shapes::shape_iterator> begin (const db::Shapes *s, unsigned int flags)
{
  return gsi::layout_locking_iterator1<db::Shapes::shape_iterator> (s->layout (), s->begin (flags));
}

static gsi::layout_locking_iterator1<db::Shapes::shape_iterator> begin_all (const db::Shapes *s)
{
  return gsi::layout_locking_iterator1<db::Shapes::shape_iterator> (s->layout (), s->begin (db::ShapeIterator::All));
}

static gsi::layout_locking_iterator1<db::Shapes::shape_iterator> begin_overlapping (const db::Shapes *s, unsigned int flags, const db::Box &region)
{
  return gsi::layout_locking_iterator1<db::Shapes::shape_iterator> (s->layout (), s->begin_overlapping (region, flags));
}

static gsi::layout_locking_iterator1<db::Shapes::shape_iterator> begin_doverlapping (const db::Shapes *s, unsigned int flags, const db::DBox &region)
{
  return gsi::layout_locking_iterator1<db::Shapes::shape_iterator> (s->layout (), s->begin_overlapping (db::CplxTrans (shapes_dbu (s)).inverted () * region, flags));
}

static gsi::layout_locking_iterator1<db::Shapes::shape_iterator> begin_overlapping_all (const db::Shapes *s, const db::Box &region)
{
  return gsi::layout_locking_iterator1<db::Shapes::shape_iterator> (s->layout (), s->begin_overlapping (region, db::ShapeIterator::All));
}

static gsi::layout_locking_iterator1<db::Shapes::shape_iterator> begin_doverlapping_all (const db::Shapes *s, const db::DBox &region)
{
  return gsi::layout_locking_iterator1<db::Shapes::shape_iterator> (s->layout (), s->begin_overlapping (db::CplxTrans (shapes_dbu (s)).inverted () * region, db::ShapeIterator::All));
}

static gsi::layout_locking_iterator1<db::Shapes::shape_iterator> begin_touching (const db::Shapes *s, unsigned int flags, const db::Box &region)
{
  return gsi::layout_locking_iterator1<db::Shapes::shape_iterator> (s->layout (), s->begin_touching (region, flags));
}

static gsi::layout_locking_iterator1<db::Shapes::shape_iterator> begin_dtouching (const db::Shapes *s, unsigned int flags, const db::DBox &region)
{
  return gsi::layout_locking_iterator1<db::Shapes::shape_iterator> (s->layout (), s->begin_touching (db::CplxTrans (shapes_dbu (s)).inverted () * region, flags));
}

static gsi::layout_locking_iterator1<db::Shapes::shape_iterator> begin_touching_all (const db::Shapes *s, const db::Box &region)
{
  return gsi::layout_locking_iterator1<db::Shapes::shape_iterator> (s->layout (), s->begin_touching (region, db::ShapeIterator::All));
}

static gsi::layout_locking_iterator1<db::Shapes::shape_iterator> begin_dtouching_all (const db::Shapes *s, const db::DBox &region)
{
  return gsi::layout_locking_iterator1<db::Shapes::shape_iterator> (s->layout (), s->begin_touching (db::CplxTrans (shapes_dbu (s)).inverted () * region, db::ShapeIterator::All));
}

static void transform_shapes (db::Shapes *s, const db::Trans &trans)
{
  db::Shapes d (*s);
  s->assign_transformed (d, trans);
}

static void transform_shapes_dtrans (db::Shapes *s, const db::DTrans &trans)
{
  db::CplxTrans dbu_trans (shapes_dbu (s));
  db::Shapes d (*s);
  s->assign_transformed (d, dbu_trans.inverted () * db::DCplxTrans (trans) * dbu_trans);
}

static void transform_shapes_icplx (db::Shapes *s, const db::ICplxTrans &trans)
{
  db::Shapes d (*s);
  s->assign_transformed (d, trans);
}

static void transform_shapes_dcplx (db::Shapes *s, const db::DCplxTrans &trans)
{
  db::CplxTrans dbu_trans (shapes_dbu (s));
  db::Shapes d (*s);
  s->assign_transformed (d, dbu_trans.inverted () * trans * dbu_trans);
}

static db::Shape transform_shape_icplx (db::Shapes *s, const db::Shape &shape, const db::ICplxTrans &trans)
{
  return s->transform (shape, trans);
}

static db::Shape transform_shape_dtrans (db::Shapes *s, const db::Shape &shape, const db::DTrans &trans)
{
  db::CplxTrans dbu_trans (shapes_dbu (s));
  return s->transform (shape, dbu_trans.inverted () * db::DCplxTrans (trans) * dbu_trans);
}

static db::Shape transform_shape_dcplx (db::Shapes *s, const db::Shape &shape, const db::DCplxTrans &trans)
{
  db::CplxTrans dbu_trans (shapes_dbu (s));
  return s->transform (shape, dbu_trans.inverted () * trans * dbu_trans);
}

static db::Shape insert_shape_with_trans (db::Shapes *s, const db::Shape &shape, const db::Trans &trans)
{
  tl::ident_map<db::properties_id_type> pm;
  return s->insert (shape, trans, pm);
}

static db::Shape insert_shape_with_dtrans (db::Shapes *s, const db::Shape &shape, const db::DTrans &trans)
{
  db::CplxTrans dbu_trans (shapes_dbu (s));
  tl::ident_map<db::properties_id_type> pm;
  return s->insert (shape, dbu_trans.inverted () * db::DCplxTrans (trans) * dbu_trans, pm);
}

static db::Shape insert_shape_with_icplx_trans (db::Shapes *s, const db::Shape &shape, const db::ICplxTrans &trans)
{
  tl::ident_map<db::properties_id_type> pm;
  return s->insert (shape, trans, pm);
}

static db::Shape insert_shape_with_dcplx_trans (db::Shapes *s, const db::Shape &shape, const db::DCplxTrans &trans)
{
  db::CplxTrans dbu_trans (shapes_dbu (s));
  tl::ident_map<db::properties_id_type> pm;
  return s->insert (shape, dbu_trans.inverted () * trans * dbu_trans, pm);
}

static void insert_iter (db::Shapes *sh, const db::RecursiveShapeIterator &r)
{
  //  NOTE: if the source (r) is from the same layout than the shapes live in, we better
  //  lock the layout against updates while inserting
  db::LayoutLocker locker (sh->layout ());
  for (db::RecursiveShapeIterator i = r; !i.at_end (); ++i) {
    tl::ident_map<db::properties_id_type> pm;
    sh->insert (*i, i.trans (), pm);
  }
}

static void insert_iter_with_trans (db::Shapes *sh, const db::RecursiveShapeIterator &r, const db::ICplxTrans &trans)
{
  //  NOTE: if the source (r) is from the same layout than the shapes live in, we better
  //  lock the layout against updates while inserting
  db::LayoutLocker locker (sh->layout ());
  for (db::RecursiveShapeIterator i = r; !i.at_end (); ++i) {
    tl::ident_map<db::properties_id_type> pm;
    sh->insert (*i, trans * i.trans (), pm);
  }
}

static void insert_shapes (db::Shapes *sh, const db::Shapes &s)
{
  sh->insert (s);
}

static void insert_shapes_with_flags (db::Shapes *sh, const db::Shapes &s, unsigned int flags)
{
  sh->insert (s, flags);
}

static void insert_shapes_with_trans (db::Shapes *sh, const db::Shapes &s, const db::ICplxTrans &trans)
{
  //  NOTE: if the source (r) is from the same layout than the shapes live in, we better
  //  lock the layout against updates while inserting
  db::LayoutLocker locker (sh->layout ());
  for (db::Shapes::shape_iterator i = s.begin (db::ShapeIterator::All); !i.at_end(); ++i) {
    tl::ident_map<db::properties_id_type> pm;
    sh->insert (*i, trans, pm);
  }
}

static void insert_shapes_with_flag_and_trans (db::Shapes *sh, const db::Shapes &s, unsigned int flags, const db::ICplxTrans &trans)
{
  //  NOTE: if the source (r) is from the same layout than the shapes live in, we better
  //  lock the layout against updates while inserting
  db::LayoutLocker locker (sh->layout ());
  for (db::Shapes::shape_iterator i = s.begin (flags); !i.at_end(); ++i) {
    tl::ident_map<db::properties_id_type> pm;
    sh->insert (*i, trans, pm);
  }
}

static void insert_region (db::Shapes *sh, const db::Region &r)
{
  //  NOTE: if the source (r) is from the same layout than the shapes live in, we better
  //  lock the layout against updates while inserting
  db::LayoutLocker locker (sh->layout ());
  for (db::Region::const_iterator s = r.begin (); ! s.at_end (); ++s) {
    sh->insert (*s);
  }
}

static void insert_region_with_trans (db::Shapes *sh, const db::Region &r, const db::ICplxTrans &trans)
{
  //  NOTE: if the source (r) is from the same layout than the shapes live in, we better
  //  lock the layout against updates while inserting
  db::LayoutLocker locker (sh->layout ());
  for (db::Region::const_iterator s = r.begin (); ! s.at_end (); ++s) {
    sh->insert (s->transformed (trans));
  }
}

static void insert_region_with_dtrans (db::Shapes *sh, const db::Region &r, const db::DCplxTrans &trans)
{
  db::CplxTrans dbu_trans (shapes_dbu (sh));
  db::ICplxTrans itrans = dbu_trans.inverted () * trans * dbu_trans;
  for (db::Region::const_iterator s = r.begin (); ! s.at_end (); ++s) {
    sh->insert (s->transformed (itrans));
  }
}

static void insert_edges (db::Shapes *sh, const db::Edges &r)
{
  for (db::Edges::const_iterator s = r.begin (); ! s.at_end (); ++s) {
    sh->insert (*s);
  }
}

static void insert_edges_with_trans (db::Shapes *sh, const db::Edges &r, const db::ICplxTrans &trans)
{
  for (db::Edges::const_iterator s = r.begin (); ! s.at_end (); ++s) {
    sh->insert (s->transformed (trans));
  }
}

static void insert_edges_with_dtrans (db::Shapes *sh, const db::Edges &r, const db::DCplxTrans &trans)
{
  db::CplxTrans dbu_trans (shapes_dbu (sh));
  db::ICplxTrans itrans = dbu_trans.inverted () * trans * dbu_trans;
  for (db::Edges::const_iterator s = r.begin (); ! s.at_end (); ++s) {
    sh->insert (s->transformed (itrans));
  }
}

static void insert_edge_pairs_as_polygons (db::Shapes *sh, const db::EdgePairs &r, db::Coord e)
{
  for (db::EdgePairs::const_iterator s = r.begin (); ! s.at_end (); ++s) {
    sh->insert (s->normalized ().to_simple_polygon (e));
  }
}

static void insert_edge_pairs_as_polygons_d (db::Shapes *sh, const db::EdgePairs &r, db::DCoord de)
{
  db::Coord e = db::coord_traits<db::Coord>::rounded (de / shapes_dbu (sh));
  for (db::EdgePairs::const_iterator s = r.begin (); ! s.at_end (); ++s) {
    sh->insert (s->normalized ().to_simple_polygon (e));
  }
}

static void insert_edge_pairs_as_polygons_with_trans (db::Shapes *sh, const db::EdgePairs &r, const db::ICplxTrans &trans, db::Coord e)
{
  for (db::EdgePairs::const_iterator s = r.begin (); ! s.at_end (); ++s) {
    sh->insert (s->normalized ().to_simple_polygon (e).transformed (trans));
  }
}

static void insert_edge_pairs_as_polygons_with_dtrans (db::Shapes *sh, const db::EdgePairs &r, const db::DCplxTrans &trans, db::DCoord de)
{
  db::Coord e = db::coord_traits<db::Coord>::rounded (de / shapes_dbu (sh));
  db::CplxTrans dbu_trans (shapes_dbu (sh));
  db::ICplxTrans itrans = dbu_trans.inverted () * trans * dbu_trans;
  for (db::EdgePairs::const_iterator s = r.begin (); ! s.at_end (); ++s) {
    sh->insert (s->normalized ().to_simple_polygon (e).transformed (itrans));
  }
}

static void insert_edge_pairs_as_edges (db::Shapes *sh, const db::EdgePairs &r)
{
  for (db::EdgePairs::const_iterator s = r.begin (); ! s.at_end (); ++s) {
    sh->insert (s->first ());
    sh->insert (s->second ());
  }
}

static void insert_edge_pairs_as_edges_with_trans (db::Shapes *sh, const db::EdgePairs &r, const db::ICplxTrans &trans)
{
  for (db::EdgePairs::const_iterator s = r.begin (); ! s.at_end (); ++s) {
    sh->insert (s->first ().transformed (trans));
    sh->insert (s->second ().transformed (trans));
  }
}

static void insert_edge_pairs_as_edges_with_dtrans (db::Shapes *sh, const db::EdgePairs &r, const db::DCplxTrans &trans)
{
  db::CplxTrans dbu_trans (shapes_dbu (sh));
  db::ICplxTrans itrans = dbu_trans.inverted () * trans * dbu_trans;
  for (db::EdgePairs::const_iterator s = r.begin (); ! s.at_end (); ++s) {
    sh->insert (s->first ().transformed (itrans));
    sh->insert (s->second ().transformed (itrans));
  }
}

static void insert_edge_pairs (db::Shapes *sh, const db::EdgePairs &r)
{
  for (db::EdgePairs::const_iterator s = r.begin (); ! s.at_end (); ++s) {
    sh->insert (*s);
  }
}

static void insert_edge_pairs_with_trans (db::Shapes *sh, const db::EdgePairs &r, const db::ICplxTrans &trans)
{
  for (db::EdgePairs::const_iterator s = r.begin (); ! s.at_end (); ++s) {
    sh->insert (s->transformed (trans));
  }
}

static void insert_edge_pairs_with_dtrans (db::Shapes *sh, const db::EdgePairs &r, const db::DCplxTrans &trans)
{
  db::CplxTrans dbu_trans (shapes_dbu (sh));
  db::ICplxTrans itrans = dbu_trans.inverted () * trans * dbu_trans;
  for (db::EdgePairs::const_iterator s = r.begin (); ! s.at_end (); ++s) {
    sh->insert (s->transformed (itrans));
  }
}

static void insert_texts (db::Shapes *sh, const db::Texts &r)
{
  for (db::Texts::const_iterator s = r.begin (); ! s.at_end (); ++s) {
    sh->insert (*s);
  }
}

static void insert_texts_with_trans (db::Shapes *sh, const db::Texts &r, const db::ICplxTrans &trans)
{
  for (db::Texts::const_iterator s = r.begin (); ! s.at_end (); ++s) {
    sh->insert (s->transformed (trans));
  }
}

static void insert_texts_with_dtrans (db::Shapes *sh, const db::Texts &r, const db::DCplxTrans &trans)
{
  db::CplxTrans dbu_trans (shapes_dbu (sh));
  db::ICplxTrans itrans = dbu_trans.inverted () * trans * dbu_trans;
  for (db::Texts::const_iterator s = r.begin (); ! s.at_end (); ++s) {
    sh->insert (s->transformed (itrans));
  }
}

static db::Layout *layout (db::Shapes *sh)
{
  if (sh->cell ()) {
    return sh->cell ()->layout ();
  } else {
    return 0;
  }
}

static unsigned int s_all ()                 { return db::ShapeIterator::All; }
static unsigned int s_all_with_properties () { return db::ShapeIterator::AllWithProperties; }
static unsigned int s_properties ()          { return db::ShapeIterator::Properties; }
static unsigned int s_polygons ()            { return db::ShapeIterator::Polygons; }
static unsigned int s_regions ()             { return db::ShapeIterator::Regions; }
static unsigned int s_boxes ()               { return db::ShapeIterator::Boxes; }
static unsigned int s_edges ()               { return db::ShapeIterator::Edges; }
static unsigned int s_edge_pairs ()          { return db::ShapeIterator::EdgePairs; }
static unsigned int s_points ()              { return db::ShapeIterator::Points; }
static unsigned int s_paths ()               { return db::ShapeIterator::Paths; }
static unsigned int s_texts ()               { return db::ShapeIterator::Texts; }
static unsigned int s_user_objects ()        { return db::ShapeIterator::UserObjects; }

Class<db::Shapes> decl_Shapes ("db", "Shapes",
  gsi::method ("insert", (db::Shape (db::Shapes::*)(const db::Shape &)) &db::Shapes::insert, gsi::arg ("shape"),
    "@brief Inserts a shape from a shape reference into the shapes list\n"
    "@return A reference (a \\Shape object) to the newly created shape\n"
    "This method has been introduced in version 0.16.\n"
  ) +
  gsi::method_ext ("insert", &insert_shape_with_trans, gsi::arg ("shape"), gsi::arg ("trans"),
    "@brief Inserts a shape from a shape reference into the shapes list with a transformation\n"
    "@param shape The shape to insert\n"
    "@param trans The transformation to apply before the shape is inserted\n"
    "@return A reference (a \\Shape object) to the newly created shape\n"
    "This method has been introduced in version 0.22.\n"
  ) +
  gsi::method_ext ("insert", &insert_shape_with_dtrans, gsi::arg ("shape"), gsi::arg ("trans"),
    "@brief Inserts a shape from a shape reference into the shapes list with a transformation (given in micrometer units)\n"
    "@param shape The shape to insert\n"
    "@param trans The transformation to apply before the shape is inserted (displacement in micrometers)\n"
    "@return A reference (a \\Shape object) to the newly created shape\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("insert", &insert_shape_with_icplx_trans, gsi::arg ("shape"), gsi::arg ("trans"),
    "@brief Inserts a shape from a shape reference into the shapes list with a complex integer transformation\n"
    "@param shape The shape to insert\n"
    "@param trans The transformation to apply before the shape is inserted\n"
    "@return A reference (a \\Shape object) to the newly created shape\n"
    "This method has been introduced in version 0.22.\n"
  ) +
  gsi::method_ext ("insert", &insert_shape_with_dcplx_trans, gsi::arg ("shape"), gsi::arg ("trans"),
    "@brief Inserts a shape from a shape reference into the shapes list with a complex integer transformation (given in micrometer units)\n"
    "@param shape The shape to insert\n"
    "@param trans The transformation to apply before the shape is inserted (displacement in micrometer units)\n"
    "@return A reference (a \\Shape object) to the newly created shape\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("insert", &insert_iter, gsi::arg ("iter"),
    "@brief Inserts the shapes taken from a recursive shape iterator\n"
    "@param iter The iterator from which to take the shapes from\n"
    "\n"
    "This method iterates over all shapes from the iterator and inserts them into the container.\n"
    "\n"
    "This method has been introduced in version 0.25.3.\n"
  ) +
  gsi::method_ext ("insert", &insert_iter_with_trans, gsi::arg ("iter"), gsi::arg ("trans"),
    "@brief Inserts the shapes taken from a recursive shape iterator with a transformation\n"
    "@param iter The iterator from which to take the shapes from\n"
    "@param trans The transformation to apply\n"
    "\n"
    "This method iterates over all shapes from the iterator and inserts them into the container.\n"
    "The given transformation is applied before the shapes are inserted.\n"
    "\n"
    "This method has been introduced in version 0.25.3.\n"
  ) +
  gsi::method_ext ("insert", &insert_shapes, gsi::arg ("shapes"),
    "@brief Inserts the shapes taken from another shape container\n"
    "@param shapes The other container from which to take the shapes from\n"
    "\n"
    "This method takes all shapes from the given container and inserts them into this one.\n"
    "\n"
    "This method has been introduced in version 0.25.3.\n"
  ) +
  gsi::method_ext ("insert", &insert_shapes_with_trans, gsi::arg ("shapes"), gsi::arg ("trans"),
    "@brief Inserts the shapes taken from another shape container with a transformation\n"
    "@param shapes The other container from which to take the shapes from\n"
    "@param trans The transformation to apply\n"
    "\n"
    "This method takes all shapes from the given container and inserts them into this one "
    "after applying the given transformation.\n"
    "\n"
    "This method has been introduced in version 0.25.3.\n"
  ) +
  gsi::method_ext ("insert", &insert_shapes_with_flags, gsi::arg ("shapes"), gsi::arg ("flags"),
    "@brief Inserts the shapes taken from another shape container\n"
    "@param shapes The other container from which to take the shapes from\n"
    "@param flags The filter flags for taking the shapes from the input container (see S... constants)\n"
    "\n"
    "This method takes all selected shapes from the given container and inserts them into this one.\n"
    "\n"
    "This method has been introduced in version 0.25.3.\n"
  ) +
  gsi::method_ext ("insert", &insert_shapes_with_flag_and_trans, gsi::arg ("shapes"), gsi::arg ("flags"), gsi::arg ("trans"),
    "@brief Inserts the shapes taken from another shape container with a transformation\n"
    "@param shapes The other container from which to take the shapes from\n"
    "@param flags The filter flags for taking the shapes from the input container (see S... constants)\n"
    "@param trans The transformation to apply\n"
    "\n"
    "This method takes all selected shapes from the given container and inserts them into this one "
    "after applying the given transformation.\n"
    "\n"
    "This method has been introduced in version 0.25.3.\n"
  ) +
  gsi::method_ext ("insert", &insert_region, gsi::arg ("region"),
    "@brief Inserts the polygons from the region into this shape container\n"
    "@param region The region to insert\n"
    "\n"
    "This method inserts all polygons from the region into this shape container.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("insert", &insert_region_with_trans, gsi::arg ("region"), gsi::arg ("trans"),
    "@brief Inserts the polygons from the region into this shape container with a transformation\n"
    "@param region The region to insert\n"
    "@param trans The transformation to apply\n"
    "\n"
    "This method inserts all polygons from the region into this shape container.\n"
    "Before a polygon is inserted, the given transformation is applied.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("insert", &insert_region_with_dtrans, gsi::arg ("region"), gsi::arg ("trans"),
    "@brief Inserts the polygons from the region into this shape container with a transformation (given in micrometer units)\n"
    "@param region The region to insert\n"
    "@param trans The transformation to apply (displacement in micrometer units)\n"
    "\n"
    "This method inserts all polygons from the region into this shape container.\n"
    "Before a polygon is inserted, the given transformation is applied.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("insert", &insert_edges, gsi::arg ("edges"),
    "@brief Inserts the edges from the edge collection into this shape container\n"
    "@param edges The edges to insert\n"
    "\n"
    "This method inserts all edges from the edge collection into this shape container.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("insert", &insert_edges_with_trans, gsi::arg ("edges"), gsi::arg ("trans"),
    "@brief Inserts the edges from the edge collection into this shape container with a transformation\n"
    "@param edges The edges to insert\n"
    "@param trans The transformation to apply\n"
    "\n"
    "This method inserts all edges from the edge collection into this shape container.\n"
    "Before an edge is inserted, the given transformation is applied.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("insert", &insert_edges_with_dtrans, gsi::arg ("edges"), gsi::arg ("trans"),
    "@brief Inserts the edges from the edge collection into this shape container with a transformation (given in micrometer units)\n"
    "@param edges The edges to insert\n"
    "@param trans The transformation to apply (displacement in micrometer units)\n"
    "\n"
    "This method inserts all edges from the edge collection into this shape container.\n"
    "Before an edge is inserted, the given transformation is applied.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("insert", &insert_edge_pairs, gsi::arg ("edge_pairs"),
    "@brief Inserts the edges from the edge pair collection into this shape container\n"
    "@param edges The edge pairs to insert\n"
    "\n"
    "This method inserts all edge pairs from the edge pair collection into this shape container.\n"
    "\n"
    "This method has been introduced in version 0.26.\n"
  ) +
  gsi::method_ext ("insert", &insert_edge_pairs_with_trans, gsi::arg ("edge_pairs"), gsi::arg ("trans"),
    "@brief Inserts the edge pairs from the edge pair collection into this shape container with a transformation\n"
    "@param edges The edge pairs to insert\n"
    "@param trans The transformation to apply\n"
    "\n"
    "This method inserts all edge pairs from the edge pair collection into this shape container.\n"
    "Before an edge pair is inserted, the given transformation is applied.\n"
    "\n"
    "This method has been introduced in version 0.26.\n"
  ) +
  gsi::method_ext ("insert", &insert_edge_pairs_with_dtrans, gsi::arg ("edge_pairs"), gsi::arg ("trans"),
    "@brief Inserts the edge pairs from the edge pair collection into this shape container with a transformation (given in micrometer units)\n"
    "@param edges The edge pairs to insert\n"
    "@param trans The transformation to apply (displacement in micrometer units)\n"
    "\n"
    "This method inserts all edge pairs from the edge pair collection into this shape container.\n"
    "Before an edge pair is inserted, the given transformation is applied.\n"
    "\n"
    "This method has been introduced in version 0.26.\n"
  ) +
  gsi::method_ext ("insert_as_polygons", &insert_edge_pairs_as_polygons, gsi::arg ("edge_pairs"), gsi::arg ("e"),
    "@brief Inserts the edge pairs from the edge pair collection as polygons into this shape container\n"
    "@param edge_pairs The edge pairs to insert\n"
    "@param e The extension to apply when converting the edges to polygons (in database units)\n"
    "\n"
    "This method inserts all edge pairs from the edge pair collection into this shape container.\n"
    "The edge pairs are converted to polygons covering the area between the edges.\n"
    "The extension parameter specifies a sizing which is applied when converting the edge pairs to polygons. This way, "
    "degenerated edge pairs (i.e. two point-like edges) do not vanish.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("insert_as_polygons", &insert_edge_pairs_as_polygons_d, gsi::arg ("edge_pairs"), gsi::arg ("e"),
    "@brief Inserts the edge pairs from the edge pair collection as polygons into this shape container\n"
    "@param edge_pairs The edge pairs to insert\n"
    "@param e The extension to apply when converting the edges to polygons (in micrometer units)\n"
    "\n"
    "This method is identical to the version with a integer-type \\e parameter, but for this version the \\e parameter "
    "is given in micrometer units.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("insert_as_polygons", &insert_edge_pairs_as_polygons_with_trans, gsi::arg ("edge_pairs"), gsi::arg ("e"), gsi::arg ("trans"),
    "@brief Inserts the edge pairs from the edge pair collection as polygons into this shape container with a transformation\n"
    "@param edges The edge pairs to insert\n"
    "@param e The extension to apply when converting the edges to polygons (in database units)\n"
    "@param trans The transformation to apply\n"
    "\n"
    "This method inserts all edge pairs from the edge pair collection into this shape container.\n"
    "The edge pairs are converted to polygons covering the area between the edges.\n"
    "The extension parameter specifies a sizing which is applied when converting the edge pairs to polygons. This way, "
    "degenerated edge pairs (i.e. two point-like edges) do not vanish.\n"
    "Before a polygon is inserted into the shape collection, the given transformation is applied.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("insert_as_polygons", &insert_edge_pairs_as_polygons_with_dtrans, gsi::arg ("edge_pairs"), gsi::arg ("e"), gsi::arg ("trans"),
    "@brief Inserts the edge pairs from the edge pair collection as polygons into this shape container with a transformation\n"
    "@param edges The edge pairs to insert\n"
    "@param e The extension to apply when converting the edges to polygons (in micrometer units)\n"
    "@param trans The transformation to apply (displacement in micrometer units)\n"
    "\n"
    "This method is identical to the version with a integer-type \\e and \\trans parameter, but for this version the \\e parameter "
    "is given in micrometer units and the \\trans parameter is a micrometer-unit transformation.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("insert_as_edges", &insert_edge_pairs_as_edges, gsi::arg ("edge_pairs"),
    "@brief Inserts the edge pairs from the edge pair collection as individual edges into this shape container\n"
    "@param edge_pairs The edge pairs to insert\n"
    "\n"
    "This method inserts all edge pairs from the edge pair collection into this shape container.\n"
    "Each edge from the edge pair is inserted individually into the shape container.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("insert_as_edges", &insert_edge_pairs_as_edges_with_trans, gsi::arg ("edge_pairs"), gsi::arg ("trans"),
    "@brief Inserts the edge pairs from the edge pair collection as individual into this shape container with a transformation\n"
    "@param edges The edge pairs to insert\n"
    "@param trans The transformation to apply\n"
    "\n"
    "This method inserts all edge pairs from the edge pair collection into this shape container.\n"
    "Each edge from the edge pair is inserted individually into the shape container.\n"
    "Before each edge is inserted into the shape collection, the given transformation is applied.\n"
    "\n"
    "This method has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("insert_as_edges", &insert_edge_pairs_as_edges_with_dtrans, gsi::arg ("edge_pairs"), gsi::arg ("trans"),
    "@brief Inserts the edge pairs from the edge pair collection as individual into this shape container with a transformation (given in micrometer units)\n"
    "@param edges The edge pairs to insert\n"
    "@param trans The transformation to apply (displacement in micrometer units)\n"
    "\n"
    "This method inserts all edge pairs from the edge pair collection into this shape container.\n"
    "Each edge from the edge pair is inserted individually into the shape container.\n"
    "Before each edge is inserted into the shape collection, the given transformation is applied.\n"
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("insert", &insert_texts, gsi::arg ("texts"),
    "@brief Inserts the texts from the text collection into this shape container\n"
    "@param texts The texts to insert\n"
    "\n"
    "This method inserts all texts from the text collection into this shape container.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method_ext ("insert", &insert_texts_with_trans, gsi::arg ("texts"), gsi::arg ("trans"),
    "@brief Inserts the texts from the text collection into this shape container with a transformation\n"
    "@param edges The texts to insert\n"
    "@param trans The transformation to apply\n"
    "\n"
    "This method inserts all texts from the text collection into this shape container.\n"
    "Before an text is inserted, the given transformation is applied.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method_ext ("insert", &insert_texts_with_dtrans, gsi::arg ("texts"), gsi::arg ("trans"),
    "@brief Inserts the texts from the text collection into this shape container with a transformation (given in micrometer units)\n"
    "@param edges The text to insert\n"
    "@param trans The transformation to apply (displacement in micrometer units)\n"
    "\n"
    "This method inserts all texts from the text collection into this shape container.\n"
    "Before an text is inserted, the given transformation is applied.\n"
    "\n"
    "This method has been introduced in version 0.27.\n"
  ) +
  gsi::method_ext ("transform", &transform_shapes, gsi::arg ("trans"),
    "@brief Transforms all shapes with the given transformation\n"
    "This method will invalidate all references to shapes inside this collection.\n\n"
    "It has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("transform", &transform_shapes_dtrans, gsi::arg ("trans"),
    "@brief Transforms all shapes with the given transformation (given in micrometer units)\n"
    "This method will invalidate all references to shapes inside this collection.\n"
    "The displacement of the transformation is given in micrometer units.\n"
    "\n"
    "It has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("transform", &transform_shapes_icplx, gsi::arg ("trans"),
    "@brief Transforms all shapes with the given complex integer transformation\n"
    "This method will invalidate all references to shapes inside this collection.\n\n"
    "It has been introduced in version 0.23.\n"
  ) +
  gsi::method_ext ("transform", &transform_shapes_dcplx, gsi::arg ("trans"),
    "@brief Transforms all shapes with the given transformation (given in micrometer units)\n"
    "This method will invalidate all references to shapes inside this collection.\n"
    "The displacement of the transformation is given in micrometer units.\n"
    "\n"
    "It has been introduced in version 0.25.\n"
  ) +
  gsi::method ("transform", (db::Shape (db::Shapes::*)(const db::Shape &, const db::Trans &)) &db::Shapes::transform, gsi::arg ("shape"), gsi::arg ("trans"),
    "@brief Transforms the shape given by the reference with the given transformation\n"
    "@return A reference (a \\Shape object) to the new shape\n"
    "The original shape may be deleted and re-inserted by this method. Therefore, a new reference is returned.\n"
    "It is permitted in editable mode only.\n"
    "\n"
    "This method has been introduced in version 0.16.\n"
  ) +
  gsi::method_ext ("transform", &transform_shape_dtrans, gsi::arg ("shape"), gsi::arg ("trans"),
    "@brief Transforms the shape given by the reference with the given transformation, where the transformation is given in micrometer units\n"
    "@param trans The transformation to apply (displacement in micrometer units)\n"
    "@return A reference (a \\Shape object) to the new shape\n"
    "The original shape may be deleted and re-inserted by this method. Therefore, a new reference is returned.\n"
    "It is permitted in editable mode only."
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("transform", &transform_shape_icplx, gsi::arg ("shape"), gsi::arg ("trans"),
    "@brief Transforms the shape given by the reference with the given complex integer space transformation\n"
    "@return A reference (a \\Shape object) to the new shape\n"
    "This method has been introduced in version 0.22.\n"
    "The original shape may be deleted and re-inserted by this method. Therefore, a new reference is returned.\n"
    "It is permitted in editable mode only."
  ) +
  gsi::method_ext ("transform", &transform_shape_dcplx, gsi::arg ("shape"), gsi::arg ("trans"),
    "@brief Transforms the shape given by the reference with the given complex transformation, where the transformation is given in micrometer units\n"
    "@param trans The transformation to apply (displacement in micrometer units)\n"
    "@return A reference (a \\Shape object) to the new shape\n"
    "The original shape may be deleted and re-inserted by this method. Therefore, a new reference is returned.\n"
    "It is permitted in editable mode only."
    "\n"
    "This method has been introduced in version 0.25.\n"
  ) +
  gsi::method_ext ("replace", &replace<db::Box>, gsi::arg ("shape"), gsi::arg ("box"),
    "@brief Replaces the given shape with a box\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "\n"
    "This method has been introduced with version 0.16. It replaces the given shape with the "
    "object specified. It does not change the property Id. To change the property Id, "
    "use the \\replace_prop_id method. To replace a shape and discard the property Id, erase the "
    "shape and insert a new shape."
    "\n"
    "This method is permitted in editable mode only."
  ) +
  gsi::method_ext ("replace", &dreplace<db::DBox>, gsi::arg ("shape"), gsi::arg ("box"),
    "@brief Replaces the given shape with a box given in micrometer units\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "\n"
    "This method behaves like the \\replace version with a \\Box argument, except that it will "
    "internally translate the box from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.25."
  ) +
  gsi::method_ext ("replace", &replace<db::Path>, gsi::arg ("shape"), gsi::arg ("path"),
    "@brief Replaces the given shape with a path\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "\n"
    "This method has been introduced with version 0.16. It replaces the given shape with the "
    "object specified. It does not change the property Id. To change the property Id, "
    "use the \\replace_prop_id method. To replace a shape and discard the property Id, erase the "
    "shape and insert a new shape."
    "\n"
    "This method is permitted in editable mode only."
  ) +
  gsi::method_ext ("replace", &dreplace<db::DPath>, gsi::arg ("shape"), gsi::arg ("path"),
    "@brief Replaces the given shape with a path given in micrometer units\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "\n"
    "This method behaves like the \\replace version with a \\Path argument, except that it will "
    "internally translate the path from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.25."
  ) +
  gsi::method_ext ("replace", &replace<db::Edge>, gsi::arg ("shape"), gsi::arg ("edge"),
    "@brief Replaces the given shape with an edge object\n"
    "\n"
    "This method has been introduced with version 0.16. It replaces the given shape with the "
    "object specified. It does not change the property Id. To change the property Id, "
    "use the \\replace_prop_id method. To replace a shape and discard the property Id, erase the "
    "shape and insert a new shape."
    "\n"
    "This method is permitted in editable mode only."
  ) +
  gsi::method_ext ("replace", &dreplace<db::DEdge>, gsi::arg ("shape"), gsi::arg ("edge"),
    "@brief Replaces the given shape with an edge given in micrometer units\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "\n"
    "This method behaves like the \\replace version with an \\Edge argument, except that it will "
    "internally translate the edge from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.25."
  ) +
  gsi::method_ext ("replace", &replace<db::EdgePair>, gsi::arg ("shape"), gsi::arg ("edge_pair"),
    "@brief Replaces the given shape with an edge pair object\n"
    "\n"
    "It replaces the given shape with the "
    "object specified. It does not change the property Id. To change the property Id, "
    "use the \\replace_prop_id method. To replace a shape and discard the property Id, erase the "
    "shape and insert a new shape."
    "\n"
    "This method is permitted in editable mode only.\n"
    "\n"
    "This method has been introduced in version 0.26.\n"
  ) +
  gsi::method_ext ("replace", &dreplace<db::DEdgePair>, gsi::arg ("shape"), gsi::arg ("edge_pair"),
    "@brief Replaces the given shape with an edge pair given in micrometer units\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "\n"
    "This method behaves like the \\replace version with an \\EdgePair argument, except that it will "
    "internally translate the edge pair from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.26.\n"
  ) +
  gsi::method_ext ("replace", &replace<db::Point>, gsi::arg ("shape"), gsi::arg ("point"),
    "@brief Replaces the given shape with an point object\n"
    "\n"
    "This method replaces the given shape with the "
    "object specified. It does not change the property Id. To change the property Id, "
    "use the \\replace_prop_id method. To replace a shape and discard the property Id, erase the "
    "shape and insert a new shape."
    "\n"
    "This variant has been introduced in version 0.28."
  ) +
  gsi::method_ext ("replace", &dreplace<db::DPoint>, gsi::arg ("shape"), gsi::arg ("point"),
    "@brief Replaces the given shape with an point given in micrometer units\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "\n"
    "This method behaves like the \\replace version with an \\Point argument, except that it will "
    "internally translate the point from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.28."
  ) +
  gsi::method_ext ("replace", &replace<db::Text>, gsi::arg ("shape"), gsi::arg ("text"),
    "@brief Replaces the given shape with a text object\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "\n"
    "This method has been introduced with version 0.16. It replaces the given shape with the "
    "object specified. It does not change the property Id. To change the property Id, "
    "use the \\replace_prop_id method. To replace a shape and discard the property Id, erase the "
    "shape and insert a new shape."
    "\n"
    "This method is permitted in editable mode only."
  ) +
  gsi::method_ext ("replace", &dreplace<db::DText>, gsi::arg ("shape"), gsi::arg ("text"),
    "@brief Replaces the given shape with a text given in micrometer units\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "\n"
    "This method behaves like the \\replace version with a \\Text argument, except that it will "
    "internally translate the text from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.25."
  ) +
  gsi::method_ext ("replace", &replace<db::SimplePolygon>, gsi::arg ("shape"), gsi::arg ("simple_polygon"),
    "@brief Replaces the given shape with a simple polygon\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "\n"
    "This method has been introduced with version 0.16. It replaces the given shape with the "
    "object specified. It does not change the property Id. To change the property Id, "
    "use the \\replace_prop_id method. To replace a shape and discard the property Id, erase the "
    "shape and insert a new shape."
    "\n"
    "This method is permitted in editable mode only."
  ) +
  gsi::method_ext ("replace", &dreplace<db::DSimplePolygon>, gsi::arg ("shape"), gsi::arg ("simple_polygon"),
    "@brief Replaces the given shape with a simple polygon given in micrometer units\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "\n"
    "This method behaves like the \\replace version with a \\SimplePolygon argument, except that it will "
    "internally translate the simple polygon from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.25."
  ) +
  gsi::method_ext ("replace", &replace<db::Polygon>, gsi::arg ("shape"), gsi::arg ("polygon"),
    "@brief Replaces the given shape with a polygon\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "\n"
    "This method has been introduced with version 0.16. It replaces the given shape with the "
    "object specified. It does not change the property Id. To change the property Id, "
    "use the \\replace_prop_id method. To replace a shape and discard the property Id, erase the "
    "shape and insert a new shape."
    "\n"
    "This method is permitted in editable mode only."
  ) +
  gsi::method_ext ("replace", &dreplace<db::DPolygon>, gsi::arg ("shape"), gsi::arg ("polygon"),
    "@brief Replaces the given shape with a polygon given in micrometer units\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "\n"
    "This method behaves like the \\replace version with a \\Polygon argument, except that it will "
    "internally translate the polygon from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.25."
  ) +
  gsi::method_ext ("insert|#insert_box", &insert<db::Box>, gsi::arg ("box"),
    "@brief Inserts a box into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "\n"
    "Starting with version 0.16, this method returns a reference to the newly created shape\n"
  ) +
  gsi::method_ext ("insert", &dinsert<db::DBox>, gsi::arg ("box"),
    "@brief Inserts a micrometer-unit box into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "This method behaves like the \\insert version with a \\Box argument, except that it will "
    "internally translate the box from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.25."
  ) +
  gsi::method_ext ("insert|#insert_path", &insert<db::Path>, gsi::arg ("path"),
    "@brief Inserts a path into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "\n"
    "Starting with version 0.16, this method returns a reference to the newly created shape\n"
  ) +
  gsi::method_ext ("insert", &dinsert<db::DPath>, gsi::arg ("path"),
    "@brief Inserts a micrometer-unit path into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "This method behaves like the \\insert version with a \\Path argument, except that it will "
    "internally translate the path from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.25."
  ) +
  gsi::method_ext ("insert|#insert_edge", &insert<db::Edge>, gsi::arg ("edge"),
    "@brief Inserts an edge into the shapes list\n"
    "\n"
    "Starting with version 0.16, this method returns a reference to the newly created shape\n"
  ) +
  gsi::method_ext ("insert", &dinsert<db::DEdge>, gsi::arg ("edge"),
    "@brief Inserts a micrometer-unit edge into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "This method behaves like the \\insert version with a \\Edge argument, except that it will "
    "internally translate the edge from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.25."
  ) +
  gsi::method_ext ("insert", &insert<db::EdgePair>, gsi::arg ("edge_pair"),
    "@brief Inserts an edge pair into the shapes list\n"
    "\n"
    "This method has been introduced in version 0.26.\n"
  ) +
  gsi::method_ext ("insert", &dinsert<db::DEdgePair>, gsi::arg ("edge_pair"),
    "@brief Inserts a micrometer-unit edge pair into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "This method behaves like the \\insert version with a \\EdgePair argument, except that it will "
    "internally translate the edge pair from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.26."
  ) +
  gsi::method_ext ("insert|#insert_point", &insert<db::Point>, gsi::arg ("point"),
    "@brief Inserts an point into the shapes list\n"
    "\n"
    "This variant has been introduced in version 0.28.\n"
  ) +
  gsi::method_ext ("insert", &dinsert<db::DPoint>, gsi::arg ("point"),
    "@brief Inserts a micrometer-unit point into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "This method behaves like the \\insert version with a \\Point argument, except that it will "
    "internally translate the point from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.28.\n"
  ) +
  gsi::method_ext ("insert|#insert_text", &insert<db::Text>, gsi::arg ("text"),
    "@brief Inserts a text into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "\n"
    "Starting with version 0.16, this method returns a reference to the newly created shape\n"
  ) +
  gsi::method_ext ("insert", &dinsert<db::DText>, gsi::arg ("text"),
    "@brief Inserts a micrometer-unit text into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "This method behaves like the \\insert version with a \\Text argument, except that it will "
    "internally translate the text from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.25."
  ) +
  gsi::method_ext ("insert|#insert_simple_polygon", &insert<db::SimplePolygon>, gsi::arg ("simple_polygon"),
    "@brief Inserts a simple polygon into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "\n"
    "Starting with version 0.16, this method returns a reference to the newly created shape\n"
  ) +
  gsi::method_ext ("insert", &dinsert<db::DSimplePolygon>, gsi::arg ("simple_polygon"),
    "@brief Inserts a micrometer-unit simple polygon into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "This method behaves like the \\insert version with a \\SimplePolygon argument, except that it will "
    "internally translate the polygon from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.25."
  ) +
  gsi::method_ext ("insert|#insert_polygon", &insert<db::Polygon>, gsi::arg ("polygon"),
    "@brief Inserts a polygon into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "\n"
    "Starting with version 0.16, this method returns a reference to the newly created shape\n"
  ) +
  gsi::method_ext ("insert", &dinsert<db::DPolygon>, gsi::arg ("polygon"),
    "@brief Inserts a micrometer-unit polygon into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "This method behaves like the \\insert version with a \\Polygon argument, except that it will "
    "internally translate the polygon from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.25."
  ) +
  gsi::method_ext ("insert|#insert_box_with_properties", &insert_with_properties<db::Box>, gsi::arg ("box"), gsi::arg ("property_id"),
    "@brief Inserts a box with properties into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "The property Id must be obtained from the \\Layout object's property_id method which "
    "associates a property set with a property Id."
    "\n"
    "Starting with version 0.16, this method returns a reference to the newly created shape\n"
  ) +
  gsi::method_ext ("insert", &dinsert_with_properties<db::DBox, db::Box>, gsi::arg ("box"), gsi::arg ("property_id"),
    "@brief Inserts a micrometer-unit box with properties into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "This method behaves like the \\insert version with a \\Box argument and a property ID, except that it will "
    "internally translate the box from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.25."
  ) +
  gsi::method_ext ("insert|#insert_path_with_properties", &insert_with_properties<db::Path>, gsi::arg ("path"), gsi::arg ("property_id"),
    "@brief Inserts a path with properties into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "The property Id must be obtained from the \\Layout object's property_id method which "
    "associates a property set with a property Id."
    "\n"
    "Starting with version 0.16, this method returns a reference to the newly created shape\n"
  ) +
  gsi::method_ext ("insert", &dinsert_with_properties<db::DPath, db::Path>, gsi::arg ("path"), gsi::arg ("property_id"),
    "@brief Inserts a micrometer-unit path with properties into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "This method behaves like the \\insert version with a \\Path argument and a property ID, except that it will "
    "internally translate the path from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.25."
  ) +
  gsi::method_ext ("insert|#insert_edge_with_properties", &insert_with_properties<db::Edge>, gsi::arg ("edge"), gsi::arg ("property_id"),
    "@brief Inserts an edge with properties into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "The property Id must be obtained from the \\Layout object's property_id method which "
    "associates a property set with a property Id."
    "\n"
    "Starting with version 0.16, this method returns a reference to the newly created shape.\n"
  ) +
  gsi::method_ext ("insert", &dinsert_with_properties<db::DEdge, db::Edge>, gsi::arg ("edge"), gsi::arg ("property_id"),
    "@brief Inserts a micrometer-unit edge with properties into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "This method behaves like the \\insert version with a \\Edge argument and a property ID, except that it will "
    "internally translate the edge from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.25."
  ) +
  gsi::method_ext ("insert", &insert_with_properties<db::EdgePair>, gsi::arg ("edge_pair"), gsi::arg ("property_id"),
    "@brief Inserts an edge pair with properties into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "The property Id must be obtained from the \\Layout object's property_id method which "
    "associates a property set with a property Id."
    "\n"
    "This method has been introduced in version 0.26.\n"
  ) +
  gsi::method_ext ("insert", &dinsert_with_properties<db::DEdgePair, db::EdgePair>, gsi::arg ("edge_pair"), gsi::arg ("property_id"),
    "@brief Inserts a micrometer-unit edge pair with properties into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "This method behaves like the \\insert version with a \\EdgePair argument and a property ID, except that it will "
    "internally translate the edge pair from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.26."
  ) +
  gsi::method_ext ("insert|#insert_text_with_properties", &insert_with_properties<db::Text>, gsi::arg ("text"), gsi::arg ("property_id"),
    "@brief Inserts a text with properties into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "The property Id must be obtained from the \\Layout object's property_id method which "
    "associates a property set with a property Id."
    "\n"
    "Starting with version 0.16, this method returns a reference to the newly created shape\n"
  ) +
  gsi::method_ext ("insert", &dinsert_with_properties<db::DText, db::Text>, gsi::arg ("text"), gsi::arg ("property_id"),
    "@brief Inserts a micrometer-unit text with properties into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "This method behaves like the \\insert version with a \\Text argument and a property ID, except that it will "
    "internally translate the text from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.25."
  ) +
  gsi::method_ext ("insert|#insert_simple_polygon_with_properties", &insert_with_properties<db::SimplePolygon>, gsi::arg ("simple_polygon"), gsi::arg ("property_id"),
    "@brief Inserts a simple polygon with properties into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "The property Id must be obtained from the \\Layout object's property_id method which "
    "associates a property set with a property Id."
    "\n"
    "Starting with version 0.16, this method returns a reference to the newly created shape\n"
  ) +
  gsi::method_ext ("insert", &dinsert_with_properties<db::DSimplePolygon, db::SimplePolygon>, gsi::arg ("simple_polygon"), gsi::arg ("property_id"),
    "@brief Inserts a micrometer-unit simple polygon with properties into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "This method behaves like the \\insert version with a \\SimplePolygon argument and a property ID, except that it will "
    "internally translate the simple polygon from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.25."
  ) +
  gsi::method_ext ("insert|#insert_polygon_with_properties", &insert_with_properties<db::Polygon>, gsi::arg ("polygon"), gsi::arg ("property_id"),
    "@brief Inserts a polygon with properties into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "The property Id must be obtained from the \\Layout object's property_id method which "
    "associates a property set with a property Id."
    "\n"
    "Starting with version 0.16, this method returns a reference to the newly created shape\n"
  ) +
  gsi::method_ext ("insert", &dinsert_with_properties<db::DPolygon, db::Polygon>, gsi::arg ("polygon"), gsi::arg ("property_id"),
    "@brief Inserts a micrometer-unit polygon with properties into the shapes list\n"
    "@return A reference to the new shape (a \\Shape object)\n"
    "This method behaves like the \\insert version with a \\Polygon argument and a property ID, except that it will "
    "internally translate the polygon from micrometer to database units.\n"
    "\n"
    "This variant has been introduced in version 0.25."
  ) +
  gsi::iterator_ext ("each", &begin, gsi::arg ("flags"),
    "@brief Gets all shapes\n"
    "\n"
    "@param flags An \"or\"-ed combination of the S... constants\n"
  ) +
  gsi::iterator_ext ("each", &begin_all, 
    "@brief Gets all shapes\n"
    "\n"
    "This call is equivalent to each(SAll). This convenience method has been introduced in version 0.16\n"
  ) +
  gsi::iterator_ext ("each_touching", &begin_touching, gsi::arg ("flags"), gsi::arg ("region"),
    "@brief Gets all shapes that touch the search box (region)\n"
    "This method was introduced in version 0.16\n"
    "\n"
    "@param flags An \"or\"-ed combination of the S... constants\n"
    "@param region The rectangular search region\n"
  ) +
  gsi::iterator_ext ("each_touching", &begin_dtouching, gsi::arg ("flags"), gsi::arg ("region"),
    "@brief Gets all shapes that touch the search box (region) where the search box is given in micrometer units\n"
    "@param flags An \"or\"-ed combination of the S... constants\n"
    "@param region The rectangular search region as a \\DBox object in micrometer units\n"
    "\n"
    "This method was introduced in version 0.25\n"
  ) +
  gsi::iterator_ext ("each_touching", &begin_touching_all, gsi::arg ("region"),
    "@brief Gets all shapes that touch the search box (region)\n"
    "@param region The rectangular search region\n"
    "\n"
    "This call is equivalent to each_touching(SAll,region). This convenience method has been introduced in version 0.16\n"
  ) +
  gsi::iterator_ext ("each_touching", &begin_dtouching_all, gsi::arg ("region"),
    "@brief Gets all shapes that touch the search box (region) where the search box is given in micrometer units\n"
    "@param region The rectangular search region as a \\DBox object in micrometer units\n"
    "This call is equivalent to each_touching(SAll,region).\n"
    "\n"
    "This method was introduced in version 0.25\n"
  ) +
  gsi::iterator_ext ("each_overlapping", &begin_overlapping, gsi::arg ("flags"), gsi::arg ("region"),
    "@brief Gets all shapes that overlap the search box (region)\n"
    "This method was introduced in version 0.16\n"
    "\n"
    "@param flags An \"or\"-ed combination of the S... constants\n"
    "@param region The rectangular search region\n"
  ) +
  gsi::iterator_ext ("each_overlapping", &begin_doverlapping, gsi::arg ("flags"), gsi::arg ("region"),
    "@brief Gets all shapes that overlap the search box (region) where the search box is given in micrometer units\n"
    "@param flags An \"or\"-ed combination of the S... constants\n"
    "@param region The rectangular search region as a \\DBox object in micrometer units\n"
    "\n"
    "This method was introduced in version 0.25\n"
  ) +
  gsi::iterator_ext ("each_overlapping", &begin_overlapping_all, gsi::arg ("region"),
    "@brief Gets all shapes that overlap the search box (region)\n"
    "@param region The rectangular search region\n"
    "\n"
    "This call is equivalent to each_overlapping(SAll,region). This convenience method has been introduced in version 0.16\n"
  ) +
  gsi::iterator_ext ("each_overlapping", &begin_doverlapping_all, gsi::arg ("region"),
    "@brief Gets all shapes that overlap the search box (region) where the search box is given in micrometer units\n"
    "@param region The rectangular search region as a \\DBox object in micrometer units\n"
    "This call is equivalent to each_touching(SAll,region).\n"
    "\n"
    "This method was introduced in version 0.25\n"
  ) +
  gsi::method ("erase", &db::Shapes::erase_shape, gsi::arg ("shape"),
    "@brief Erases the shape pointed to by the given \\Shape object\n"
    "This method has been introduced in version 0.16. It can only be used in editable mode.\n"
    "Erasing a shape will invalidate the shape reference. Access to this reference may then render invalid results.\n"
    "\n"
    "@param shape The shape which to destroy"
  ) +
  gsi::method ("find", (db::Shape (db::Shapes::*)(const db::Shape &) const) &db::Shapes::find, gsi::arg ("shape"),
    "@brief Finds a shape inside this collected\n"
    "This method has been introduced in version 0.21.\n"
    "This method tries to find the given shape in this collection. The original shape may be located in another collection. "
    "If the shape is found, this method returns a reference to the shape in this collection, otherwise a null reference is returned."
  ) +
  gsi::method ("is_valid?", &db::Shapes::is_valid, gsi::arg ("shape"),
    "@brief Tests if the given \\Shape object is still pointing to a valid object\n"
    "This method has been introduced in version 0.16.\n"
    "If the shape represented by the given reference has been deleted, this method returns false. "
    "If however, another shape has been inserted already that occupies the original shape's position, "
    "this method will return true again.\n"
  ) +
  gsi::method ("is_empty?", &db::Shapes::empty, 
    "@brief Returns a value indicating whether the shapes container is empty\n"
    "This method has been introduced in version 0.20.\n"
  ) +
  gsi::method ("clear", static_cast<void (db::Shapes::*) ()> (&db::Shapes::clear),
    "@brief Clears the shape container\n"
    "This method has been introduced in version 0.16."
  ) +
  gsi::method ("clear", static_cast<void (db::Shapes::*) (unsigned int)> (&db::Shapes::clear), gsi::arg ("flags"),
    "@brief Clears certain shape types from the shape container\n"
    "Only shapes matching the shape types from 'flags' are removed. 'flags' is a combination of the S... constants.\n"
    "\n"
    "This method has been introduced in version 0.28.9."
  ) +
  gsi::method_ext ("size", &shapes_size,
    "@brief Gets the number of shapes in this container\n"
    "This method was introduced in version 0.16\n"
    "@return The number of shapes in this container\n"
  ) +
  gsi::method ("cell", &db::Shapes::cell,
    "@brief Gets the cell the shape container belongs to\n"
    "This method returns nil if the shape container does not belong to a cell.\n"
    "\n"
    "This method has been added in version 0.28."
  ) +
  gsi::method_ext ("layout", &layout,
    "@brief Gets the layout object the shape container belongs to\n"
    "This method returns nil if the shape container does not belong to a layout.\n"
    "\n"
    "This method has been added in version 0.28."
  ) +
  gsi::method ("replace_prop_id", (db::Shape (db::Shapes::*) (const db::Shape &, db::properties_id_type)) &db::Shapes::replace_prop_id, gsi::arg ("shape"), gsi::arg ("property_id"),
    "@brief Replaces (or install) the properties of a shape\n"
    "@return A \\Shape object representing the new shape\n"
    "This method has been introduced in version 0.16. It can only be used in editable mode.\n"
    "Changes the properties Id of the given shape or install a properties Id on that shape if it does not have one yet.\n"
    "The property Id must be obtained from the \\Layout object's property_id method which "
    "associates a property set with a property Id.\n"
    "This method will potentially invalidate the shape reference passed to it. Use the reference "
    "returned for future references."
  ) +
  gsi::method ("SAll|#s_all", &s_all,
    "@brief Indicates that all shapes shall be retrieved\n"
    "You can use this constant to construct 'except' classes - e.g. "
    "to specify 'all shape types except boxes' use\n"
    "\n"
    "@code SAll - SBoxes @/code\n"
  ) +
  gsi::method ("SAllWithProperties|#s_all_with_properties", &s_all_with_properties,
    "@brief Indicates that all shapes with properties shall be retrieved\n"
    "Using this selector means to retrieve only shapes with properties."
    "You can use this constant to construct 'except' classes - e.g. "
    "to specify 'all shape types with properties except boxes' use\n"
    "\n"
    "@code SAllWithProperties - SBoxes @/code\n"
  ) +
  gsi::method ("SPolygons|#s_polygons", &s_polygons,
    "@brief Indicates that polygons shall be retrieved"
  ) +
  gsi::method ("SRegions|#s_regions", &s_regions,
    "@brief Indicates that objects which can be polygonized shall be retrieved (paths, boxes, polygons etc.)\n"
    "\n"
    "This constant has been added in version 0.27."
  ) +
  gsi::method ("SBoxes|#s_boxes", &s_boxes,
    "@brief Indicates that boxes shall be retrieved"
  ) +
  gsi::method ("SEdges|#s_edges", &s_edges,
    "@brief Indicates that edges shall be retrieved"
  ) +
  gsi::method ("SEdgePairs|#s_edge_pairs", &s_edge_pairs,
    "@brief Indicates that edge pairs shall be retrieved"
  ) +
  gsi::method ("SPoints|#s_points", &s_points,
    "@brief Indicates that points shall be retrieved"
    "\n"
    "This constant has been added in version 0.28."
  ) +
  gsi::method ("SPaths|#s_paths", &s_paths,
    "@brief Indicates that paths shall be retrieved"
  ) +
  gsi::method ("STexts|#s_texts", &s_texts,
    "@brief Indicates that texts be retrieved"
  ) +
  gsi::method ("SUserObjects|#s_user_objects", &s_user_objects,
    "@brief Indicates that user objects shall be retrieved"
  ) +
  gsi::method ("SProperties|#s_properties", &s_properties,
    "@brief Indicates that only shapes with properties shall be retrieved\n"
    "You can or-combine this flag with the plain shape types to select a "
    "certain shape type, but only those shapes with properties. For example to "
    "select boxes with properties, use 'SProperties | SBoxes'."
  ) +
  gsi::method_ext ("dump_mem_statistics", &dump_mem_statistics, gsi::arg<bool> ("detailed", false),
    "@hide"
  ),
  "@brief A collection of shapes\n"
  "\n"
  "A shapes collection is a collection of geometrical objects, such as "
  "polygons, boxes, paths, edges, edge pairs or text objects.\n"
  "\n"
  "Shapes objects are the basic containers for geometrical objects of a cell. Inside a cell, there is "
  "one Shapes object per layer.\n"
);

}
