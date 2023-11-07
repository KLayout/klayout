
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
#include "gsiEnums.h"

#include "dbDeepShapeStore.h"
#include "dbEdges.h"
#include "dbEdgesUtils.h"
#include "dbDeepEdges.h"
#include "dbRegion.h"
#include "dbOriginalLayerRegion.h"
#include "dbLayoutUtils.h"

#include "gsiDeclDbContainerHelpers.h"

namespace gsi
{

static inline std::vector<db::Edges> as_2edges_vector (const std::pair<db::Edges, db::Edges> &rp)
{
  std::vector<db::Edges> res;
  res.reserve (2);
  res.push_back (db::Edges (const_cast<db::Edges &> (rp.first).take_delegate ()));
  res.push_back (db::Edges (const_cast<db::Edges &> (rp.second).take_delegate ()));
  return res;
}

static db::Edges *new_v ()
{
  return new db::Edges ();
}

static std::string to_string0 (const db::Edges *r)
{
  return r->to_string ();
}

static std::string to_string1 (const db::Edges *r, size_t n)
{
  return r->to_string (n);
}

static db::Edges *new_e (const db::Edge &e)
{
  db::Edges *ee = new db::Edges ();
  ee->insert (e);
  return ee;
}

static db::Edges *new_a1 (const std::vector <db::Polygon> &a)
{
  return new db::Edges (a.begin (), a.end ());
}

static db::Edges *new_a2 (const std::vector <db::Edge> &a)
{
  return new db::Edges (a.begin (), a.end ());
}

static db::Edges *new_b (const db::Box &o)
{
  return new db::Edges (o);
}

static db::Edges *new_p (const db::Polygon &o)
{
  return new db::Edges (o);
}

static db::Edges *new_ps (const db::SimplePolygon &o)
{
  return new db::Edges (o);
}

static db::Edges *new_path (const db::Path &o)
{
  return new db::Edges (o);
}

static db::Edges *new_shapes (const db::Shapes &s, bool as_edges)
{
  db::Edges *r = new db::Edges ();
  for (db::Shapes::shape_iterator i = s.begin (as_edges ? db::ShapeIterator::All : db::ShapeIterator::Edges); !i.at_end (); ++i) {
    r->insert (*i);
  }
  return r;
}

static db::Edges *new_si (const db::RecursiveShapeIterator &si, bool as_edges)
{
  return new db::Edges (si, as_edges);
}

static db::Edges *new_si2 (const db::RecursiveShapeIterator &si, const db::ICplxTrans &trans, bool as_edges)
{
  return new db::Edges (si, trans, as_edges);
}

static db::Edges *new_sid (const db::RecursiveShapeIterator &si, db::DeepShapeStore &dss, bool as_edges)
{
  return new db::Edges (si, dss, as_edges);
}

static db::Edges *new_si2d (const db::RecursiveShapeIterator &si, db::DeepShapeStore &dss, const db::ICplxTrans &trans, bool as_edges)
{
  return new db::Edges (si, dss, trans, as_edges);
}

static db::Edges::distance_type length1 (const db::Edges *edges)
{
  return edges->length ();
}

static db::Edges::distance_type length2 (const db::Edges *edges, const db::Box &box)
{
  return edges->length (box);
}

static void insert_a1 (db::Edges *r, const std::vector <db::Polygon> &a)
{
  for (std::vector <db::Polygon>::const_iterator p = a.begin (); p != a.end (); ++p) {
    r->insert (*p);
  }
}

static void insert_a2 (db::Edges *r, const std::vector <db::Edge> &a)
{
  for (std::vector <db::Edge>::const_iterator p = a.begin (); p != a.end (); ++p) {
    r->insert (*p);
  }
}

static void insert_si (db::Edges *r, db::RecursiveShapeIterator si)
{
  while (! si.at_end ()) {
    r->insert (si.shape (), si.trans ());
    ++si;
  }
}

static void insert_si2 (db::Edges *r, db::RecursiveShapeIterator si, db::ICplxTrans &trans)
{
  while (! si.at_end ()) {
    r->insert (si.shape (), trans * si.trans ());
    ++si;
  }
}

static db::Edges in (const db::Edges *r, const db::Edges &other)
{
  return r->in (other, false);
}

static db::Edges not_in (const db::Edges *r, const db::Edges &other)
{
  return r->in (other, true);
}

static std::vector<db::Edges> in_and_out (const db::Edges *r, const db::Edges &other)
{
  return as_2edges_vector (r->in_and_out (other));
}

static db::Edges &move_p (db::Edges *r, const db::Vector &p)
{
  r->transform (db::Disp (p));
  return *r;
}

static db::Edges &move_xy (db::Edges *r, db::Coord x, db::Coord y)
{
  r->transform (db::Disp (db::Vector (x, y)));
  return *r;
}

static db::Edges moved_p (const db::Edges *r, const db::Vector &p)
{
  return r->transformed (db::Disp (p));
}

static db::Edges moved_xy (const db::Edges *r, db::Coord x, db::Coord y)
{
  return r->transformed (db::Disp (db::Vector (x, y)));
}

static db::Edges with_length1 (const db::Edges *r, db::Edges::distance_type length, bool inverse)
{
  db::EdgeLengthFilter f (length, length + 1, inverse);
  return r->filtered (f);
}

static db::Edges with_length2 (const db::Edges *r, const tl::Variant &min, const tl::Variant &max, bool inverse)
{
  db::EdgeLengthFilter f (min.is_nil () ? db::Edges::distance_type (0) : min.to<db::Edges::distance_type> (), max.is_nil () ? std::numeric_limits <db::Edges::distance_type>::max () : max.to<db::Edges::distance_type> (), inverse);
  return r->filtered (f);
}

static db::Edges with_angle1 (const db::Edges *r, double a, bool inverse)
{
  db::EdgeOrientationFilter f (a, inverse);
  return r->filtered (f);
}

static db::Edges with_angle2 (const db::Edges *r, double amin, double amax, bool inverse, bool include_amin, bool include_amax)
{
  db::EdgeOrientationFilter f (amin, include_amin, amax, include_amax, inverse);
  return r->filtered (f);
}

static db::Edges with_angle3 (const db::Edges *r, db::SpecialEdgeOrientationFilter::FilterType type, bool inverse)
{
  db::SpecialEdgeOrientationFilter f (type, inverse);
  return r->filtered (f);
}

static db::EdgePairs width2 (const db::Edges *r, db::Edges::coord_type d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection)
{
  return r->width_check (d, db::EdgesCheckOptions (whole_edges,
                               metrics,
                               ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                               min_projection.is_nil () ? db::Edges::distance_type (0) : min_projection.to<db::Edges::distance_type> (),
                               max_projection.is_nil () ? std::numeric_limits<db::Edges::distance_type>::max () : max_projection.to<db::Edges::distance_type> ())
                         );
}

static db::EdgePairs space2 (const db::Edges *r, db::Edges::coord_type d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection)
{
  return r->space_check (d, db::EdgesCheckOptions (whole_edges,
                               metrics,
                               ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                               min_projection.is_nil () ? db::Edges::distance_type (0) : min_projection.to<db::Edges::distance_type> (),
                               max_projection.is_nil () ? std::numeric_limits<db::Edges::distance_type>::max () : max_projection.to<db::Edges::distance_type> ())
                         );
}

static db::EdgePairs inside2 (const db::Edges *r, const db::Edges &other, db::Edges::coord_type d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection)
{
  return r->inside_check (other, d, db::EdgesCheckOptions (whole_edges,
                                        metrics,
                                        ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                                        min_projection.is_nil () ? db::Edges::distance_type (0) : min_projection.to<db::Edges::distance_type> (),
                                        max_projection.is_nil () ? std::numeric_limits<db::Edges::distance_type>::max () : max_projection.to<db::Edges::distance_type> ())
                         );
}

static db::EdgePairs overlap2 (const db::Edges *r, const db::Edges &other, db::Edges::coord_type d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection)
{
  return r->overlap_check (other, d, db::EdgesCheckOptions (whole_edges,
                                         metrics,
                                         ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                                         min_projection.is_nil () ? db::Edges::distance_type (0) : min_projection.to<db::Edges::distance_type> (),
                                         max_projection.is_nil () ? std::numeric_limits<db::Edges::distance_type>::max () : max_projection.to<db::Edges::distance_type> ())
                          );
}

static db::EdgePairs enclosing2 (const db::Edges *r, const db::Edges &other, db::Edges::coord_type d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection)
{
  return r->enclosing_check (other, d, db::EdgesCheckOptions (whole_edges,
                                           metrics,
                                           ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                                           min_projection.is_nil () ? db::Edges::distance_type (0) : min_projection.to<db::Edges::distance_type> (),
                                           max_projection.is_nil () ? std::numeric_limits<db::Edges::distance_type>::max () : max_projection.to<db::Edges::distance_type> ())
                            );
}

static db::EdgePairs separation2 (const db::Edges *r, const db::Edges &other, db::Edges::coord_type d, bool whole_edges, db::metrics_type metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection)
{
  return r->separation_check (other, d, db::EdgesCheckOptions (whole_edges,
                                           metrics,
                                           ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                                           min_projection.is_nil () ? db::Edges::distance_type (0) : min_projection.to<db::Edges::distance_type> (),
                                           max_projection.is_nil () ? std::numeric_limits<db::Edges::distance_type>::max () : max_projection.to<db::Edges::distance_type> ())
                             );
}

static db::Region extended_in (const db::Edges *r, db::Coord e)
{
  db::Region out;
  r->extended (out, 0, 0, 0, e, false);
  return out;
}

static db::Region extended_out (const db::Edges *r, db::Coord e)
{
  db::Region out;
  r->extended (out, 0, 0, e, 0, false);
  return out;
}

static db::Region extended (const db::Edges *r, db::Coord b, db::Coord e, db::Coord o, db::Coord i, bool join)
{
  db::Region out;
  r->extended (out, b, e, o, i, join);
  return out;
}

static db::Region pull_interacting (const db::Edges *r, const db::Region &other)
{
  db::Region out;
  r->pull_interacting (out, other);
  return out;
}

static db::Region extents2 (const db::Edges *r, db::Coord dx, db::Coord dy)
{
  db::Region output;
  r->processed (output, db::extents_processor<db::Edge> (dx, dy));
  return output;
}

static db::Region extents1 (const db::Edges *r, db::Coord d)
{
  return extents2 (r, d, d);
}

static db::Region extents0 (const db::Edges *r)
{
  return extents2 (r, 0, 0);
}

static void insert_r (db::Edges *e, const db::Region &a)
{
  for (db::Region::const_iterator p = a.begin (); ! p.at_end (); ++p) {
    e->insert (*p);
  }
}

static void insert_e (db::Edges *e, const db::Edges &a)
{
  for (db::Edges::const_iterator p = a.begin (); ! p.at_end (); ++p) {
    e->insert (*p);
  }
}

template <class Trans>
static void insert_st (db::Edges *e, const db::Shapes &a, const Trans &t)
{
  for (db::Shapes::shape_iterator p = a.begin (db::ShapeIterator::Polygons | db::ShapeIterator::Boxes | db::ShapeIterator::Paths); !p.at_end (); ++p) {
    db::Polygon poly;
    p->polygon (poly);
    e->insert (poly.transformed (t));
  }
  for (db::Shapes::shape_iterator p = a.begin (db::ShapeIterator::Edges); !p.at_end (); ++p) {
    db::Edge edge;
    p->edge (edge);
    e->insert (edge.transformed (t));
  }
}

static void insert_s (db::Edges *e, const db::Shapes &a)
{
  insert_st (e, a, db::UnitTrans ());
}

static bool is_deep (const db::Edges *e)
{
  return dynamic_cast<const db::DeepEdges *> (e->delegate ()) != 0;
}

static db::Edges *new_texts_as_dots1 (const db::RecursiveShapeIterator &si, const std::string &pat, bool pattern)
{
  return new db::Edges (db::Region (si).texts_as_dots (pat, pattern));
}

static db::Edges *new_texts_as_dots2 (const db::RecursiveShapeIterator &si, db::DeepShapeStore &dss, const std::string &pat, bool pattern)
{
  return new db::Edges (db::Region (si).texts_as_dots (pat, pattern, dss));
}

static size_t id (const db::Edges *e)
{
  return tl::id_of (e->delegate ());
}

static std::vector<db::Edges> andnot_with_edges (const db::Edges *r, const db::Edges &other)
{
  return as_2edges_vector (r->andnot (other));
}

static std::vector<db::Edges> andnot_with_region (const db::Edges *r, const db::Region &other)
{
  return as_2edges_vector (r->andnot (other));
}

static std::vector<db::Edges> inside_outside_part (const db::Edges *r, const db::Region &other)
{
  return as_2edges_vector (r->inside_outside_part (other));
}

static std::vector<db::Edges> split_inside_with_edges (const db::Edges *r, const db::Edges &other)
{
  return as_2edges_vector (r->selected_inside_differential (other));
}

static std::vector<db::Edges> split_inside_with_region (const db::Edges *r, const db::Region &other)
{
  return as_2edges_vector (r->selected_inside_differential (other));
}

static std::vector<db::Edges> split_outside_with_edges (const db::Edges *r, const db::Edges &other)
{
  return as_2edges_vector (r->selected_outside_differential (other));
}

static std::vector<db::Edges> split_outside_with_region (const db::Edges *r, const db::Region &other)
{
  return as_2edges_vector (r->selected_outside_differential (other));
}

static std::vector<db::Edges> split_interacting_with_edges (const db::Edges *r, const db::Edges &other)
{
  return as_2edges_vector (r->selected_interacting_differential (other));
}

static std::vector<db::Edges> split_interacting_with_region (const db::Edges *r, const db::Region &other)
{
  return as_2edges_vector (r->selected_interacting_differential (other));
}


extern Class<db::ShapeCollection> decl_dbShapeCollection;

//  NOTE: the Metrics constants are injected into Edges in gsiDeclDbRegion.cc because this is where these constants are instantiated.

Class<db::Edges> decl_Edges (decl_dbShapeCollection, "db", "Edges",
  constructor ("new", &new_v, 
    "@brief Default constructor\n"
    "\n"
    "This constructor creates an empty edge collection.\n"
  ) +
  constructor ("new", &new_e, gsi::arg ("edge"),
    "@brief Constructor from a single edge\n"
    "\n"
    "This constructor creates an edge collection with a single edge.\n"
  ) +
  constructor ("new", &new_a1, gsi::arg ("array"),
    "@brief Constructor from a polygon array\n"
    "\n"
    "This constructor creates an edge collection from an array of polygons.\n"
    "The edges form the contours of the polygons.\n"
  ) +
  constructor ("new", &new_a2, gsi::arg ("array"),
    "@brief Constructor from an edge array\n"
    "\n"
    "This constructor creates an edge collection from an array of edges.\n"
  ) +
  constructor ("new", &new_b, gsi::arg ("box"),
    "@brief Box constructor\n"
    "\n"
    "This constructor creates an edge collection from a box.\n"
    "The edges form the contour of the box.\n"
  ) +
  constructor ("new", &new_p, gsi::arg ("polygon"),
    "@brief Polygon constructor\n"
    "\n"
    "This constructor creates an edge collection from a polygon.\n"
    "The edges form the contour of the polygon.\n"
  ) +
  constructor ("new", &new_ps, gsi::arg ("polygon"),
    "@brief Simple polygon constructor\n"
    "\n"
    "This constructor creates an edge collection from a simple polygon.\n"
    "The edges form the contour of the polygon.\n"
  ) +
  constructor ("new", &new_path, gsi::arg ("path"),
    "@brief Path constructor\n"
    "\n"
    "This constructor creates an edge collection from a path.\n"
    "The edges form the contour of the path.\n"
  ) +
  constructor ("new", &new_shapes, gsi::arg ("shapes"), gsi::arg ("as_edges", true),
    "@brief Constructor of a flat edge collection from a \\Shapes container\n"
    "\n"
    "If 'as_edges' is true, the shapes from the container will be converted to edges (i.e. polygon contours to edges). "
    "Otherwise, only edges will be taken from the container.\n"
    "\n"
    "This method has been introduced in version 0.26.\n"
  ) +
  constructor ("new", &new_si, gsi::arg ("shape_iterator"), gsi::arg ("as_edges", true),
    "@brief Constructor of a flat edge collection from a hierarchical shape set\n"
    "\n"
    "This constructor creates an edge collection from the shapes delivered by the given recursive shape iterator.\n"
    "It feeds the shapes from a hierarchy of cells into a flat edge set.\n"
    "\n"
    "Text objects are not inserted, because they cannot be converted to edges.\n"
    "Edge objects are inserted as such. If \"as_edges\" is true, \"solid\" objects (boxes, polygons, paths) are converted to edges which "
    "form the hull of these objects. If \"as_edges\" is false, solid objects are ignored.\n"
    "\n"
    "@code\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n" 
    "layer  = ... # the index of the layer from where to take the shapes from\n" 
    "r = RBA::Edges::new(layout.begin_shapes(cell, layer), false)\n"
    "@/code\n"
  ) +
  constructor ("new", &new_si2, gsi::arg ("shape_iterator"), gsi::arg ("trans"), gsi::arg ("as_edges", true),
    "@brief Constructor of a flat edge collection from a hierarchical shape set with a transformation\n"
    "\n"
    "This constructor creates an edge collection from the shapes delivered by the given recursive shape iterator.\n"
    "It feeds the shapes from a hierarchy of cells into a flat edge set.\n"
    "The transformation is useful to scale to a specific database unit for example.\n"
    "\n"
    "Text objects are not inserted, because they cannot be converted to edges.\n"
    "Edge objects are inserted as such. If \"as_edges\" is true, \"solid\" objects (boxes, polygons, paths) are converted to edges which "
    "form the hull of these objects. If \"as_edges\" is false, solid objects are ignored.\n"
    "\n"
    "@code\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n" 
    "layer  = ... # the index of the layer from where to take the shapes from\n" 
    "dbu    = 0.1 # the target database unit\n"
    "r = RBA::Edges::new(layout.begin_shapes(cell, layer), RBA::ICplxTrans::new(layout.dbu / dbu))\n"
    "@/code\n"
  ) +
  constructor ("new", &new_sid, gsi::arg ("shape_iterator"), gsi::arg ("dss"), gsi::arg ("as_edges", true),
    "@brief Constructor of a hierarchical edge collection\n"
    "\n"
    "This constructor creates an edge collection from the shapes delivered by the given recursive shape iterator.\n"
    "It feeds the shapes from a hierarchy of cells into the hierarchical edge set.\n"
    "The edges remain within their original hierarchy unless other operations require the edges to be moved in the hierarchy.\n"
    "\n"
    "Text objects are not inserted, because they cannot be converted to edges.\n"
    "Edge objects are inserted as such. If \"as_edges\" is true, \"solid\" objects (boxes, polygons, paths) are converted to edges which "
    "form the hull of these objects. If \"as_edges\" is false, solid objects are ignored.\n"
    "\n"
    "@code\n"
    "dss    = RBA::DeepShapeStore::new\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n"
    "layer  = ... # the index of the layer from where to take the shapes from\n"
    "r = RBA::Edges::new(layout.begin_shapes(cell, layer), dss, false)\n"
    "@/code\n"
  ) +
  constructor ("new", &new_si2d, gsi::arg ("shape_iterator"), gsi::arg ("dss"), gsi::arg ("trans"), gsi::arg ("as_edges", true),
    "@brief Constructor of a hierarchical edge collection with a transformation\n"
    "\n"
    "This constructor creates an edge collection from the shapes delivered by the given recursive shape iterator.\n"
    "It feeds the shapes from a hierarchy of cells into the hierarchical edge set.\n"
    "The edges remain within their original hierarchy unless other operations require the edges to be moved in the hierarchy.\n"
    "The transformation is useful to scale to a specific database unit for example.\n"
    "\n"
    "Text objects are not inserted, because they cannot be converted to edges.\n"
    "Edge objects are inserted as such. If \"as_edges\" is true, \"solid\" objects (boxes, polygons, paths) are converted to edges which "
    "form the hull of these objects. If \"as_edges\" is false, solid objects are ignored.\n"
    "\n"
    "@code\n"
    "dss    = RBA::DeepShapeStore::new\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n"
    "layer  = ... # the index of the layer from where to take the shapes from\n"
    "dbu    = 0.1 # the target database unit\n"
    "r = RBA::Edges::new(layout.begin_shapes(cell, layer), dss, RBA::ICplxTrans::new(layout.dbu / dbu), false)\n"
    "@/code\n"
  ) +
  constructor ("new", &new_texts_as_dots1, gsi::arg("shape_iterator"), gsi::arg ("expr"), gsi::arg ("as_pattern", true),
    "@brief Constructor from a text set\n"
    "\n"
    "@param shape_iterator The iterator from which to derive the texts\n"
    "@param expr The selection string\n"
    "@param as_pattern If true, the selection string is treated as a glob pattern. Otherwise the match is exact.\n"
    "\n"
    "This special constructor will create dot-like edges from the text objects delivered by the shape iterator. "
    "Each text object will give a degenerated edge (a dot) that represents the text origin.\n"
    "Texts can be selected by their strings - either through a glob pattern or by exact comparison with "
    "the given string. The following options are available:\n"
    "\n"
    "@code\n"
    "dots = RBA::Edges::new(iter, \"*\")           # all texts\n"
    "dots = RBA::Edges::new(iter, \"A*\")          # all texts starting with an 'A'\n"
    "dots = RBA::Edges::new(iter, \"A*\", false)   # all texts exactly matching 'A*'\n"
    "@/code\n"
    "\n"
    "This method has been introduced in version 0.26.\n"
  ) +
  constructor ("new", &new_texts_as_dots2, gsi::arg("shape_iterator"), gsi::arg ("dss"), gsi::arg ("expr"), gsi::arg ("as_pattern", true),
    "@brief Constructor from a text set\n"
    "\n"
    "@param shape_iterator The iterator from which to derive the texts\n"
    "@param dss The \\DeepShapeStore object that acts as a heap for hierarchical operations.\n"
    "@param expr The selection string\n"
    "@param as_pattern If true, the selection string is treated as a glob pattern. Otherwise the match is exact.\n"
    "\n"
    "This special constructor will create a deep edge set from the text objects delivered by the shape iterator. "
    "Each text object will give a degenerated edge (a dot) that represents the text origin.\n"
    "Texts can be selected by their strings - either through a glob pattern or by exact comparison with "
    "the given string. The following options are available:\n"
    "\n"
    "@code\n"
    "region = RBA::Region::new(iter, dss, \"*\")           # all texts\n"
    "region = RBA::Region::new(iter, dss, \"A*\")          # all texts starting with an 'A'\n"
    "region = RBA::Region::new(iter, dss, \"A*\", false)   # all texts exactly matching 'A*'\n"
    "@/code\n"
    "\n"
    "This method has been introduced in version 0.26.\n"
  ) +
  method ("insert_into", &db::Edges::insert_into, gsi::arg ("layout"), gsi::arg ("cell_index"), gsi::arg ("layer"),
    "@brief Inserts this edge collection into the given layout, below the given cell and into the given layer.\n"
    "If the edge collection is a hierarchical one, a suitable hierarchy will be built below the top cell or "
    "and existing hierarchy will be reused.\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  method_ext ("with_length", with_length1, gsi::arg ("length"), gsi::arg ("inverse"),
    "@brief Filters the edges by length\n"
    "Filters the edges in the edge collection by length. If \"inverse\" is false, only "
    "edges which have the given length are returned. If \"inverse\" is true, "
    "edges not having the given length are returned.\n"
  ) +
  method_ext ("with_length", with_length2, gsi::arg ("min_length"), gsi::arg ("max_length"), gsi::arg ("inverse"),
    "@brief Filters the edges by length\n"
    "Filters the edges in the edge collection by length. If \"inverse\" is false, only "
    "edges which have a length larger or equal to \"min_length\" and less than \"max_length\" are "
    "returned. If \"inverse\" is true, "
    "edges not having a length less than \"min_length\" or larger or equal than \"max_length\" are "
    "returned.\n"
    "\n"
    "If you don't want to specify a lower or upper limit, pass nil to that parameter.\n"
  ) +
  method_ext ("with_angle", with_angle1, gsi::arg ("angle"), gsi::arg ("inverse"),
    "@brief Filters the edges by orientation\n"
    "Filters the edges in the edge collection by orientation. If \"inverse\" is false, only "
    "edges which have the given angle to the x-axis are returned. If \"inverse\" is true, "
    "edges not having the given angle are returned.\n"
    "\n"
    "This will select horizontal edges:\n"
    "\n"
    "@code\n"
    "horizontal = edges.with_angle(0, false)\n"
    "@/code\n"
  ) +
  method_ext ("with_angle", with_angle2, gsi::arg ("min_angle"), gsi::arg ("max_angle"), gsi::arg ("inverse"), gsi::arg ("include_min_angle", true), gsi::arg ("include_max_angle", false),
    "@brief Filters the edges by orientation\n"
    "Filters the edges in the edge collection by orientation. If \"inverse\" is false, only "
    "edges which have an angle to the x-axis larger or equal to \"min_angle\" (depending on \"include_min_angle\") and equal or less than \"max_angle\" (depending on \"include_max_angle\") are "
    "returned. If \"inverse\" is true, "
    "edges which do not conform to this criterion are returned.\n"
    "\n"
    "With \"include_min_angle\" set to true (the default), the minimum angle is included in the criterion while with false, the "
    "minimum angle itself is not included. Same for \"include_max_angle\" where the default is false, meaning the maximum angle is not included in the range.\n"
    "\n"
    "The two \"include..\" arguments have been added in version 0.27."
  ) +
  method_ext ("with_angle", with_angle3, gsi::arg ("type"), gsi::arg ("inverse"),
    "@brief Filters the edges by orientation type\n"
    "Filters the edges in the edge collection by orientation. If \"inverse\" is false, only "
    "edges which have an angle of the given type are returned. If \"inverse\" is true, "
    "edges which do not conform to this criterion are returned.\n"
    "\n"
    "This version allows specifying an edge type instead of an angle. Edge types include multiple distinct orientations "
    "and are specified using one of the \\OrthoEdges, \\DiagonalEdges or \\OrthoDiagonalEdges types.\n"
    "\n"
    "This method has been added in version 0.28.\n"
  ) +
  method ("insert", (void (db::Edges::*)(const db::Edge &)) &db::Edges::insert, gsi::arg ("edge"),
    "@brief Inserts an edge\n"
    "\n"
    "Inserts the edge into the edge collection.\n"
  ) +
  method ("insert", (void (db::Edges::*)(const db::Box &)) &db::Edges::insert, gsi::arg ("box"),
    "@brief Inserts a box\n"
    "\n"
    "Inserts the edges that form the contour of the box into the edge collection.\n"
  ) +
  method ("insert", (void (db::Edges::*)(const db::Polygon &)) &db::Edges::insert, gsi::arg ("polygon"),
    "@brief Inserts a polygon\n"
    "\n"
    "Inserts the edges that form the contour of the polygon into the edge collection.\n"
  ) +
  method ("insert", (void (db::Edges::*)(const db::SimplePolygon &)) &db::Edges::insert, gsi::arg ("polygon"),
    "@brief Inserts a simple polygon\n"
    "\n"
    "Inserts the edges that form the contour of the simple polygon into the edge collection.\n"
  ) +
  method ("insert", (void (db::Edges::*)(const db::Path &)) &db::Edges::insert, gsi::arg ("path"),
    "@brief Inserts a path\n"
    "\n"
    "Inserts the edges that form the contour of the path into the edge collection.\n"
  ) +
  method_ext ("insert", &insert_e, gsi::arg ("edges"),
    "@brief Inserts all edges from the other edge collection into this one\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("insert", &insert_r, gsi::arg ("region"),
    "@brief Inserts a region\n"
    "Inserts the edges that form the contours of the polygons from the region into the edge collection.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("insert", &insert_s, gsi::arg ("shapes"),
    "@brief Inserts all edges from the shape collection into this edge collection\n"
    "This method takes each edge from the shape collection and "
    "inserts it into the region. \"Polygon-like\" objects are inserted as edges forming the contours of the polygons.\n"
    "Text objects are ignored.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("insert", &insert_st<db::Trans>, gsi::arg ("shapes"), gsi::arg ("trans"),
    "@brief Inserts all edges from the shape collection into this edge collection (with transformation)\n"
    "This method acts as the version without transformation, but will apply the given "
    "transformation before inserting the edges.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("insert", &insert_st<db::ICplxTrans>, gsi::arg ("shapes"), gsi::arg ("trans"),
    "@brief Inserts all edges from the shape collection into this edge collection with complex transformation\n"
    "This method acts as the version without transformation, but will apply the given "
    "complex transformation before inserting the edges.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("insert", &insert_si, gsi::arg ("shape_iterator"),
    "@brief Inserts all shapes delivered by the recursive shape iterator into this edge collection\n"
    "\n"
    "For \"solid\" shapes (boxes, polygons, paths), this method inserts the edges that form the contour of the shape into the edge collection.\n"
    "Edge shapes are inserted as such.\n"
    "Text objects are not inserted, because they cannot be converted to polygons.\n"
  ) +
  method_ext ("insert", &insert_si2, gsi::arg ("shape_iterator"), gsi::arg ("trans"),
    "@brief Inserts all shapes delivered by the recursive shape iterator into this edge collection with a transformation\n"
    "\n"
    "For \"solid\" shapes (boxes, polygons, paths), this method inserts the edges that form the contour of the shape into the edge collection.\n"
    "Edge shapes are inserted as such.\n"
    "Text objects are not inserted, because they cannot be converted to polygons.\n"
    "This variant will apply the given transformation to the shapes. This is useful to scale the "
    "shapes to a specific database unit for example.\n"
  ) +
  method_ext ("insert", &insert_a1, gsi::arg ("polygons"),
    "@brief Inserts all polygons from the array into this edge collection\n"
  ) +
  method_ext ("insert", &insert_a2, gsi::arg ("edges"),
    "@brief Inserts all edges from the array into this edge collection\n"
  ) +
  method ("merge", (db::Edges &(db::Edges::*) ()) &db::Edges::merge,
    "@brief Merge the edges\n"
    "\n"
    "@return The edge collection after the edges have been merged (self).\n"
    "\n"
    "Merging joins parallel edges which overlap or touch.\n"
    "Crossing edges are not merged.\n"
    "If the edge collection is already merged, this method does nothing\n"
  ) +
  method ("merged", (db::Edges (db::Edges::*) () const) &db::Edges::merged,
    "@brief Returns the merged edge collection\n"
    "\n"
    "@return The edge collection after the edges have been merged.\n"
    "\n"
    "Merging joins parallel edges which overlap or touch.\n"
    "Crossing edges are not merged.\n"
    "In contrast to \\merge, this method does not modify the edge collection but returns a merged copy.\n"
  ) +
  method ("&|and", (db::Edges (db::Edges::*)(const db::Edges &) const) &db::Edges::operator&, gsi::arg ("other"),
    "@brief Returns the boolean AND between self and the other edge collection\n"
    "\n"
    "@return The result of the boolean AND operation\n"
    "\n"
    "The boolean AND operation will return all parts of the edges in this collection which "
    "are coincident with parts of the edges in the other collection."
    "The result will be a merged edge collection.\n"
    "\n"
    "The 'and' alias has been introduced in version 0.28.12."
  ) +
  method ("&=|and_with", (db::Edges &(db::Edges::*)(const db::Edges &)) &db::Edges::operator&=, gsi::arg ("other"),
    "@brief Performs the boolean AND between self and the other edge collection in-place (modifying self)\n"
    "\n"
    "@return The edge collection after modification (self)\n"
    "\n"
    "The boolean AND operation will return all parts of the edges in this collection which "
    "are coincident with parts of the edges in the other collection."
    "The result will be a merged edge collection.\n"
    "\n"
    "Note that in Ruby, the '&=' operator actually does not exist, but is emulated by '&' followed by an assignment. "
    "This is less efficient than the in-place operation, so it is recommended to use 'and_with' instead.\n"
    "\n"
    "The 'and_with' alias has been introduced in version 0.28.12."
  ) +
  method ("&|and", (db::Edges (db::Edges::*)(const db::Region &) const) &db::Edges::operator&, gsi::arg ("other"),
    "@brief Returns the parts of the edges inside the given region\n"
    "\n"
    "@return The edges inside the given region\n"
    "\n"
    "This operation returns the parts of the edges which are inside the given region.\n"
    "Edges on the borders of the polygons are included in the edge set.\n"
    "As a side effect, the edges are made non-intersecting by introducing cut points where\n"
    "edges intersect.\n"
    "\n"
    "This method has been introduced in version 0.24."
    "The 'and' alias has been introduced in version 0.28.12."
  ) +
  method ("&=|and_with", (db::Edges &(db::Edges::*)(const db::Region &)) &db::Edges::operator&=, gsi::arg ("other"),
    "@brief Selects the parts of the edges inside the given region in-place (modifying self)\n"
    "\n"
    "@return The edge collection after modification (self)\n"
    "\n"
    "This operation selects the parts of the edges which are inside the given region.\n"
    "Edges on the borders of the polygons are included in the edge set.\n"
    "As a side effect, the edges are made non-intersecting by introducing cut points where\n"
    "edges intersect.\n"
    "\n"
    "This method has been introduced in version 0.24."
    "\n"
    "Note that in Ruby, the '&=' operator actually does not exist, but is emulated by '&' followed by an assignment. "
    "This is less efficient than the in-place operation, so it is recommended to use 'and_with' instead.\n"
    "\n"
    "The 'and_with' alias has been introduced in version 0.28.12."
  ) +
  method ("-|not", (db::Edges (db::Edges::*)(const db::Edges &) const) &db::Edges::operator-, gsi::arg ("other"),
    "@brief Returns the boolean NOT between self and the other edge collection\n"
    "\n"
    "@return The result of the boolean NOT operation\n"
    "\n"
    "The boolean NOT operation will return all parts of the edges in this collection which "
    "are not coincident with parts of the edges in the other collection."
    "The result will be a merged edge collection.\n"
    "\n"
    "The 'not' alias has been introduced in version 0.28.12."
  ) +
  method ("-=|not_with", (db::Edges &(db::Edges::*)(const db::Edges &)) &db::Edges::operator-=, gsi::arg ("other"),
    "@brief Performs the boolean NOT between self and the other edge collection in-place (modifying self)\n"
    "\n"
    "@return The edge collection after modification (self)\n"
    "\n"
    "The boolean NOT operation will return all parts of the edges in this collection which "
    "are not coincident with parts of the edges in the other collection."
    "The result will be a merged edge collection.\n"
    "\n"
    "Note that in Ruby, the '-=' operator actually does not exist, but is emulated by '-' followed by an assignment. "
    "This is less efficient than the in-place operation, so it is recommended to use 'not_with' instead.\n"
    "\n"
    "The 'not_with' alias has been introduced in version 0.28.12."
  ) +
  method ("-|not", (db::Edges (db::Edges::*)(const db::Region &) const) &db::Edges::operator-, gsi::arg ("other"),
    "@brief Returns the parts of the edges outside the given region\n"
    "\n"
    "@return The edges outside the given region\n"
    "\n"
    "This operation returns the parts of the edges which are outside the given region.\n"
    "Edges on the borders of the polygons are not included in the edge set.\n"
    "As a side effect, the edges are made non-intersecting by introducing cut points where\n"
    "edges intersect.\n"
    "\n"
    "This method has been introduced in version 0.24."
    "The 'not' alias has been introduced in version 0.28.12."
  ) +
  method ("-=|not_with", (db::Edges &(db::Edges::*)(const db::Region &)) &db::Edges::operator-=, gsi::arg ("other"),
    "@brief Selects the parts of the edges outside the given region in-place (modifying self)\n"
    "\n"
    "@return The edge collection after modification (self)\n"
    "\n"
    "This operation selects the parts of the edges which are outside the given region.\n"
    "Edges on the borders of the polygons are not included in the edge set.\n"
    "As a side effect, the edges are made non-intersecting by introducing cut points where\n"
    "edges intersect.\n"
    "\n"
    "Note that in Ruby, the '-=' operator actually does not exist, but is emulated by '-' followed by an assignment. "
    "This is less efficient than the in-place operation, so it is recommended to use 'not_with' instead.\n"
    "\n"
    "This method has been introduced in version 0.24."
    "The 'not_with' alias has been introduced in version 0.28.12."
  ) +
  method_ext ("andnot", &andnot_with_edges, gsi::arg ("other"),
    "@brief Returns the boolean AND and NOT between self and the other edge set\n"
    "\n"
    "@return A two-element array of edge collections with the first one being the AND result and the second one being the NOT result\n"
    "\n"
    "This method will compute the boolean AND and NOT between two edge sets simultaneously. "
    "Because this requires a single sweep only, using this method is faster than doing AND and NOT separately.\n"
    "\n"
    "This method has been added in version 0.28.\n"
  ) +
  method_ext ("andnot", &andnot_with_region, gsi::arg ("other"),
    "@brief Returns the boolean AND and NOT between self and the region\n"
    "\n"
    "@return A two-element array of edge collections with the first one being the AND result and the second one being the NOT result\n"
    "\n"
    "This method will compute the boolean AND and NOT simultaneously. "
    "Because this requires a single sweep only, using this method is faster than doing AND and NOT separately.\n"
    "\n"
    "This method has been added in version 0.28.\n"
  ) +
  method ("^|xor", &db::Edges::operator^, gsi::arg ("other"),
    "@brief Returns the boolean XOR between self and the other edge collection\n"
    "\n"
    "@return The result of the boolean XOR operation\n"
    "\n"
    "The boolean XOR operation will return all parts of the edges in this and the other collection except "
    "the parts where both are coincident.\n"
    "The result will be a merged edge collection.\n"
    "\n"
    "The 'xor' alias has been introduced in version 0.28.12."
  ) +
  method ("^=|xor_with", &db::Edges::operator^=, gsi::arg ("other"),
    "@brief Performs the boolean XOR between self and the other edge collection in-place (modifying self)\n"
    "\n"
    "@return The edge collection after modification (self)\n"
    "\n"
    "The boolean XOR operation will return all parts of the edges in this and the other collection except "
    "the parts where both are coincident.\n"
    "The result will be a merged edge collection.\n"
    "\n"
    "Note that in Ruby, the '^=' operator actually does not exist, but is emulated by '^' followed by an assignment. "
    "This is less efficient than the in-place operation, so it is recommended to use 'xor_with' instead.\n"
    "\n"
    "The 'xor_with' alias has been introduced in version 0.28.12."
  ) +
  method ("\\||or", &db::Edges::operator|, gsi::arg ("other"),
    "@brief Returns the boolean OR between self and the other edge set\n"
    "\n"
    "@return The resulting edge collection\n"
    "\n"
    "The boolean OR is implemented by merging the edges of both edge sets. To simply join the edge collections "
    "without merging, the + operator is more efficient."
    "\n"
    "The 'or' alias has been introduced in version 0.28.12."
  ) +
  method ("\\|=|or_with", &db::Edges::operator|=, gsi::arg ("other"),
    "@brief Performs the boolean OR between self and the other edge set in-place (modifying self)\n"
    "\n"
    "@return The edge collection after modification (self)\n"
    "\n"
    "The boolean OR is implemented by merging the edges of both edge sets. To simply join the edge collections "
    "without merging, the + operator is more efficient."
    "\n"
    "Note that in Ruby, the '|=' operator actually does not exist, but is emulated by '|' followed by an assignment. "
    "This is less efficient than the in-place operation, so it is recommended to use 'or_with' instead.\n"
    "\n"
    "The 'or_with' alias has been introduced in version 0.28.12."
  ) +
  method ("+|join", &db::Edges::operator+, gsi::arg ("other"),
    "@brief Returns the combined edge set of self and the other one\n"
    "\n"
    "@return The resulting edge set\n"
    "\n"
    "This operator adds the edges of the other edge set to self and returns a new combined edge set. "
    "This usually creates unmerged edge sets and edges may overlap. Use \\merge if you want to ensure the result edge set is merged.\n"
    "\n"
    "The 'join' alias has been introduced in version 0.28.12."
  ) +
  method ("+=|join_with", &db::Edges::operator+=, gsi::arg ("other"),
    "@brief Adds the edges of the other edge collection to self\n"
    "\n"
    "@return The edge set after modification (self)\n"
    "\n"
    "This operator adds the edges of the other edge set to self. "
    "This usually creates unmerged edge sets and edges may overlap. Use \\merge if you want to ensure the result edge set is merged.\n"
    "\n"
    "Note that in Ruby, the '+=' operator actually does not exist, but is emulated by '+' followed by an assignment. "
    "This is less efficient than the in-place operation, so it is recommended to use 'join_with' instead.\n"
    "\n"
    "The 'join_with' alias has been introduced in version 0.28.12."
  ) +
  method ("interacting", (db::Edges (db::Edges::*) (const db::Edges &) const)  &db::Edges::selected_interacting, gsi::arg ("other"),
    "@brief Returns the edges of this edge collection which overlap or touch edges from the other edge collection\n"
    "\n"
    "@return A new edge collection containing the edges overlapping or touching edges from the other edge collection\n"
  ) + 
  method ("not_interacting", (db::Edges (db::Edges::*) (const db::Edges &) const)  &db::Edges::selected_not_interacting, gsi::arg ("other"),
    "@brief Returns the edges of this edge collection which do not overlap or touch edges from the other edge collection\n"
    "\n"
    "@return A new edge collection containing the edges not overlapping or touching edges from the other edge collection\n"
  ) + 
  method ("select_interacting", (db::Edges &(db::Edges::*) (const db::Edges &)) &db::Edges::select_interacting, gsi::arg ("other"),
    "@brief Selects the edges from this edge collection which overlap or touch edges from the other edge collection\n"
    "\n"
    "@return The edge collection after the edges have been selected (self)\n"
  ) + 
  method ("select_not_interacting", (db::Edges &(db::Edges::*) (const db::Edges &)) &db::Edges::select_not_interacting, gsi::arg ("other"),
    "@brief Selects the edges from this edge collection which do not overlap or touch edges from the other edge collection\n"
    "\n"
    "@return The edge collection after the edges have been selected (self)\n"
  ) + 
  method_ext ("split_interacting", &split_interacting_with_edges, gsi::arg ("other"),
    "@brief Selects the edges from this edge collection which do and do not interact with edges from the other collection\n"
    "\n"
    "@return A two-element list of edge collections (first: interacting, second: non-interacting)\n"
    "\n"
    "This method provides a faster way to compute both interacting and non-interacting edges compared to using separate methods. "
    "It has been introduced in version 0.28."
  ) +
  method ("interacting", (db::Edges (db::Edges::*) (const db::Region &) const)  &db::Edges::selected_interacting, gsi::arg ("other"),
    "@brief Returns the edges from this edge collection which overlap or touch polygons from the region\n"
    "\n"
    "@return A new edge collection containing the edges overlapping or touching polygons from the region\n"
  ) + 
  method ("not_interacting", (db::Edges (db::Edges::*) (const db::Region &) const)  &db::Edges::selected_not_interacting, gsi::arg ("other"),
    "@brief Returns the edges from this edge collection which do not overlap or touch polygons from the region\n"
    "\n"
    "@return A new edge collection containing the edges not overlapping or touching polygons from the region\n"
  ) + 
  method ("select_interacting", (db::Edges &(db::Edges::*) (const db::Region &)) &db::Edges::select_interacting, gsi::arg ("other"),
    "@brief Selects the edges from this edge collection which overlap or touch polygons from the region\n"
    "\n"
    "@return The edge collection after the edges have been selected (self)\n"
  ) + 
  method ("select_not_interacting", (db::Edges &(db::Edges::*) (const db::Region &)) &db::Edges::select_not_interacting, gsi::arg ("other"),
    "@brief Selects the edges from this edge collection which do not overlap or touch polygons from the region\n"
    "\n"
    "@return The edge collection after the edges have been selected (self)\n"
  ) + 
  method_ext ("split_interacting", &split_interacting_with_region, gsi::arg ("other"),
    "@brief Selects the edges from this edge collection which do and do not interact with polygons from the other region\n"
    "\n"
    "@return A two-element list of edge collections (first: interacting, second: non-interacting)\n"
    "\n"
    "This method provides a faster way to compute both interacting and non-interacting edges compared to using separate methods. "
    "It has been introduced in version 0.28."
  ) +
  method ("inside", (db::Edges (db::Edges::*) (const db::Edges &) const) &db::Edges::selected_inside, gsi::arg ("other"),
    "@brief Returns the edges of this edge collection which are inside (completely covered by) edges from the other edge collection\n"
    "\n"
    "@return A new edge collection containing the edges overlapping or touching edges from the other edge collection\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  method ("not_inside", (db::Edges (db::Edges::*) (const db::Edges &) const) &db::Edges::selected_not_inside, gsi::arg ("other"),
    "@brief Returns the edges of this edge collection which are not inside (completely covered by) edges from the other edge collection\n"
    "\n"
    "@return A new edge collection containing the edges not overlapping or touching edges from the other edge collection\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  method ("select_inside", (db::Edges &(db::Edges::*) (const db::Edges &)) &db::Edges::select_inside, gsi::arg ("other"),
    "@brief Selects the edges from this edge collection which are inside (completely covered by) edges from the other edge collection\n"
    "\n"
    "@return The edge collection after the edges have been selected (self)\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  method ("select_not_inside", (db::Edges &(db::Edges::*) (const db::Edges &)) &db::Edges::select_not_inside, gsi::arg ("other"),
    "@brief Selects the edges from this edge collection which are not inside (completely covered by) edges from the other edge collection\n"
    "\n"
    "@return The edge collection after the edges have been selected (self)\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  method_ext ("split_inside", &split_inside_with_edges, gsi::arg ("other"),
    "@brief Selects the edges from this edge collection which are and are not inside (completely covered by) edges from the other collection\n"
    "\n"
    "@return A two-element list of edge collections (first: inside, second: non-inside)\n"
    "\n"
    "This method provides a faster way to compute both inside and non-inside edges compared to using separate methods. "
    "It has been introduced in version 0.28."
  ) +
  method ("inside", (db::Edges (db::Edges::*) (const db::Region &) const) &db::Edges::selected_inside, gsi::arg ("other"),
    "@brief Returns the edges from this edge collection which are inside (completely covered by) polygons from the region\n"
    "\n"
    "@return A new edge collection containing the edges overlapping or touching polygons from the region\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  method ("not_inside", (db::Edges (db::Edges::*) (const db::Region &) const) &db::Edges::selected_not_inside, gsi::arg ("other"),
    "@brief Returns the edges from this edge collection which are not inside (completely covered by) polygons from the region\n"
    "\n"
    "@return A new edge collection containing the edges not overlapping or touching polygons from the region\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  method ("select_inside", (db::Edges &(db::Edges::*) (const db::Region &)) &db::Edges::select_inside, gsi::arg ("other"),
    "@brief Selects the edges from this edge collection which are inside (completely covered by) polygons from the region\n"
    "\n"
    "@return The edge collection after the edges have been selected (self)\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  method ("select_not_inside", (db::Edges &(db::Edges::*) (const db::Region &)) &db::Edges::select_not_inside, gsi::arg ("other"),
    "@brief Selects the edges from this edge collection which are not inside (completely covered by) polygons from the region\n"
    "\n"
    "@return The edge collection after the edges have been selected (self)\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  method_ext ("split_inside", &split_inside_with_region, gsi::arg ("other"),
    "@brief Selects the edges from this edge collection which are and are not inside (completely covered by) polygons from the other region\n"
    "\n"
    "@return A two-element list of edge collections (first: inside, second: non-inside)\n"
    "\n"
    "This method provides a faster way to compute both inside and non-inside edges compared to using separate methods. "
    "It has been introduced in version 0.28."
  ) +
  method ("outside", (db::Edges (db::Edges::*) (const db::Edges &) const) &db::Edges::selected_outside, gsi::arg ("other"),
    "@brief Returns the edges of this edge collection which are outside (completely covered by) edges from the other edge collection\n"
    "\n"
    "@return A new edge collection containing the edges overlapping or touching edges from the other edge collection\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  method ("not_outside", (db::Edges (db::Edges::*) (const db::Edges &) const) &db::Edges::selected_not_outside, gsi::arg ("other"),
    "@brief Returns the edges of this edge collection which are not outside (completely covered by) edges from the other edge collection\n"
    "\n"
    "@return A new edge collection containing the edges not overlapping or touching edges from the other edge collection\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  method ("select_outside", (db::Edges &(db::Edges::*) (const db::Edges &)) &db::Edges::select_outside, gsi::arg ("other"),
    "@brief Selects the edges from this edge collection which are outside (completely covered by) edges from the other edge collection\n"
    "\n"
    "@return The edge collection after the edges have been selected (self)\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  method ("select_not_outside", (db::Edges &(db::Edges::*) (const db::Edges &)) &db::Edges::select_not_outside, gsi::arg ("other"),
    "@brief Selects the edges from this edge collection which are not outside (completely covered by) edges from the other edge collection\n"
    "\n"
    "@return The edge collection after the edges have been selected (self)\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  method_ext ("split_outside", &split_outside_with_edges, gsi::arg ("other"),
    "@brief Selects the edges from this edge collection which are and are not outside (completely covered by) edges from the other collection\n"
    "\n"
    "@return A two-element list of edge collections (first: outside, second: non-outside)\n"
    "\n"
    "This method provides a faster way to compute both outside and non-outside edges compared to using separate methods. "
    "It has been introduced in version 0.28."
  ) +
  method ("outside", (db::Edges (db::Edges::*) (const db::Region &) const) &db::Edges::selected_outside, gsi::arg ("other"),
    "@brief Returns the edges from this edge collection which are outside (completely covered by) polygons from the region\n"
    "\n"
    "@return A new edge collection containing the edges overlapping or touching polygons from the region\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  method ("not_outside", (db::Edges (db::Edges::*) (const db::Region &) const) &db::Edges::selected_not_outside, gsi::arg ("other"),
    "@brief Returns the edges from this edge collection which are not outside (completely covered by) polygons from the region\n"
    "\n"
    "@return A new edge collection containing the edges not overlapping or touching polygons from the region\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  method ("select_outside", (db::Edges &(db::Edges::*) (const db::Region &)) &db::Edges::select_outside, gsi::arg ("other"),
    "@brief Selects the edges from this edge collection which are outside (completely covered by) polygons from the region\n"
    "\n"
    "@return The edge collection after the edges have been selected (self)\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  method ("select_not_outside", (db::Edges &(db::Edges::*) (const db::Region &)) &db::Edges::select_not_outside, gsi::arg ("other"),
    "@brief Selects the edges from this edge collection which are not outside (completely covered by) polygons from the region\n"
    "\n"
    "@return The edge collection after the edges have been selected (self)\n"
    "\n"
    "This method has been introduced in version 0.28."
  ) +
  method_ext ("split_outside", &split_outside_with_region, gsi::arg ("other"),
    "@brief Selects the edges from this edge collection which are and are not outside (completely covered by) polygons from the other region\n"
    "\n"
    "@return A two-element list of edge collections (first: outside, second: non-outside)\n"
    "\n"
    "This method provides a faster way to compute both outside and non-outside edges compared to using separate methods. "
    "It has been introduced in version 0.28."
  ) +
  method_ext ("pull_interacting", &pull_interacting, gsi::arg ("other"),
    "@brief Returns all polygons of \"other\" which are interacting with (overlapping, touching) edges of this edge set\n"
    "The \"pull_...\" methods are similar to \"select_...\" but work the opposite way: they "
    "select shapes from the argument region rather than self. In a deep (hierarchical) context "
    "the output region will be hierarchically aligned with self, so the \"pull_...\" methods "
    "provide a way for re-hierarchization.\n"
    "\n"
    "@return The region after the polygons have been selected (from other)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
    "\n"
    "This method has been introduced in version 0.26.1\n"
  ) +
  method ("pull_interacting", static_cast<db::Edges (db::Edges::*) (const db::Edges &) const> (&db::Edges::pull_interacting), gsi::arg ("other"),
    "@brief Returns all edges of \"other\" which are interacting with polygons of this edge set\n"
    "See the other \\pull_interacting version for more details.\n"
    "\n"
    "@return The edge collection after the edges have been selected (from other)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
    "\n"
    "This method has been introduced in version 0.26.1\n"
  ) +
  method ("intersections", &db::Edges::intersections, gsi::arg ("other"),
    "@brief Computes the intersections between this edges and other edges\n"
    "This computation is like an AND operation, but also including crossing points between non-coincident edges as "
    "degenerated (point-like) edges.\n"
    "\n"
    "This method has been introduced in version 0.26.2\n"
  ) +
  method ("inside_part", &db::Edges::inside_part, gsi::arg ("other"),
    "@brief Returns the parts of the edges of this edge collection which are inside the polygons of the region\n"
    "\n"
    "@return A new edge collection containing the edge parts inside the region\n"
    "\n"
    "This operation returns the parts of the edges which are inside the given region.\n"
    "This functionality is similar to the '&' operator, but edges on the borders of the polygons are not included in the edge set.\n"
    "As a side effect, the edges are made non-intersecting by introducing cut points where\n"
    "edges intersect.\n"
    "\n"
    "This method has been introduced in version 0.24."
  ) + 
  method ("outside_part", &db::Edges::outside_part, gsi::arg ("other"),
    "@brief Returns the parts of the edges of this edge collection which are outside the polygons of the region\n"
    "\n"
    "@return A new edge collection containing the edge parts outside the region\n"
    "\n"
    "This operation returns the parts of the edges which are not inside the given region.\n"
    "This functionality is similar to the '-' operator, but edges on the borders of the polygons are included in the edge set.\n"
    "As a side effect, the edges are made non-intersecting by introducing cut points where\n"
    "edges intersect.\n"
    "\n"
    "This method has been introduced in version 0.24."
  ) + 
  method ("select_inside_part", &db::Edges::select_inside_part, gsi::arg ("other"),
    "@brief Selects the parts of the edges from this edge collection which are inside the polygons of the given region\n"
    "\n"
    "@return The edge collection after the edges have been selected (self)\n"
    "\n"
    "This operation selects the parts of the edges which are inside the given region.\n"
    "This functionality is similar to the '&=' operator, but edges on the borders of the polygons are not included in the edge set.\n"
    "As a side effect, the edges are made non-intersecting by introducing cut points where\n"
    "edges intersect.\n"
    "\n"
    "This method has been introduced in version 0.24."
  ) + 
  method ("select_outside_part", &db::Edges::select_outside_part, gsi::arg ("other"),
    "@brief Selects the parts of the edges from this edge collection which are outside the polygons of the given region\n"
    "\n"
    "@return The edge collection after the edges have been selected (self)\n"
    "\n"
    "This operation selects the parts of the edges which are not inside the given region.\n"
    "This functionality is similar to the '-=' operator, but edges on the borders of the polygons are included in the edge set.\n"
    "As a side effect, the edges are made non-intersecting by introducing cut points where\n"
    "edges intersect.\n"
    "\n"
    "This method has been introduced in version 0.24."
  ) + 
  method_ext ("inside_outside_part", &inside_outside_part, gsi::arg ("other"),
    "@brief Returns the partial edges inside and outside the given region\n"
    "\n"
    "@return A two-element array of edge collections with the first one being the \\inside_part result and the second one being the \\outside_part result\n"
    "\n"
    "This method will compute the results simultaneously. "
    "Because this requires a single sweep only, using this method is faster than doing \\inside_part and \\outside_part separately.\n"
    "\n"
    "This method has been added in version 0.28.\n"
  ) +
  method ("clear", &db::Edges::clear,
    "@brief Clears the edge collection\n"
  ) +
  method ("swap", &db::Edges::swap, gsi::arg ("other"),
    "@brief Swap the contents of this edge collection with the contents of another one\n"
    "This method is useful to avoid excessive memory allocation in some cases. "
    "For managed memory languages such as Ruby, those cases will be rare. " 
  ) +
  method_ext ("move", &move_p, gsi::arg ("v"),
    "@brief Moves the edge collection\n"
    "\n"
    "Moves the polygon by the given offset and returns the \n"
    "moved edge collection. The edge collection is overwritten.\n"
    "\n"
    "@param v The distance to move the edge collection.\n"
    "\n"
    "@return The moved edge collection (self).\n"
    "\n"
    "Starting with version 0.25 the displacement type is a vector."
  ) +
  method_ext ("move", &move_xy, gsi::arg ("x"), gsi::arg ("y"),
    "@brief Moves the edge collection\n"
    "\n"
    "Moves the edge collection by the given offset and returns the \n"
    "moved edge collection. The edge collection is overwritten.\n"
    "\n"
    "@param x The x distance to move the edge collection.\n"
    "@param y The y distance to move the edge collection.\n"
    "\n"
    "@return The moved edge collection (self).\n"
  ) +
  method_ext ("moved", &moved_p, gsi::arg ("v"),
    "@brief Returns the moved edge collection (does not modify self)\n"
    "\n"
    "Moves the edge collection by the given offset and returns the \n"
    "moved edge collection. The edge collection is not modified.\n"
    "\n"
    "@param v The distance to move the edge collection.\n"
    "\n"
    "@return The moved edge collection.\n"
    "\n"
    "Starting with version 0.25 the displacement type is a vector."
  ) +
  method_ext ("moved", &moved_xy, gsi::arg ("x"), gsi::arg ("v"),
    "@brief Returns the moved edge collection (does not modify self)\n"
    "\n"
    "Moves the edge collection by the given offset and returns the \n"
    "moved edge collection. The edge collection is not modified.\n"
    "\n"
    "@param x The x distance to move the edge collection.\n"
    "@param y The y distance to move the edge collection.\n"
    "\n"
    "@return The moved edge collection.\n"
  ) +
  method ("transformed", (db::Edges (db::Edges::*)(const db::Trans &) const) &db::Edges::transformed, gsi::arg ("t"),
    "@brief Transform the edge collection\n"
    "\n"
    "Transforms the edge collection with the given transformation.\n"
    "Does not modify the edge collection but returns the transformed edge collection.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge collection.\n"
  ) +
  method ("transformed|#transformed_icplx", (db::Edges (db::Edges::*)(const db::ICplxTrans &) const) &db::Edges::transformed, gsi::arg ("t"),
    "@brief Transform the edge collection with a complex transformation\n"
    "\n"
    "Transforms the edge collection with the given complex transformation.\n"
    "Does not modify the edge collection but returns the transformed edge collection.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge collection.\n"
  ) +
  method ("transformed", (db::Edges (db::Edges::*)(const db::IMatrix2d &) const) &db::Edges::transformed, gsi::arg ("t"),
    "@brief Transform the edge collection\n"
    "\n"
    "Transforms the edge collection with the given 2d matrix transformation.\n"
    "Does not modify the edge collection but returns the transformed edge collection.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge collection.\n"
    "\n"
    "This variant has been introduced in version 0.27."
  ) +
  method ("transformed", (db::Edges (db::Edges::*)(const db::IMatrix3d &) const) &db::Edges::transformed, gsi::arg ("t"),
    "@brief Transform the edge collection\n"
    "\n"
    "Transforms the edge collection with the given 3d matrix transformation.\n"
    "Does not modify the edge collection but returns the transformed edge collection.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge collection.\n"
    "\n"
    "This variant has been introduced in version 0.27."
  ) +
  method ("transform", (db::Edges &(db::Edges::*)(const db::Trans &)) &db::Edges::transform, gsi::arg ("t"),
    "@brief Transform the edge collection (modifies self)\n"
    "\n"
    "Transforms the edge collection with the given transformation.\n"
    "This version modifies the edge collection and returns a reference to self.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge collection.\n"
  ) +
  method ("transform|#transform_icplx", (db::Edges &(db::Edges::*)(const db::ICplxTrans &)) &db::Edges::transform, gsi::arg ("t"),
    "@brief Transform the edge collection with a complex transformation (modifies self)\n"
    "\n"
    "Transforms the edge collection with the given transformation.\n"
    "This version modifies the edge collection and returns a reference to self.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge collection.\n"
  ) +
  method ("transform", (db::Edges &(db::Edges::*)(const db::IMatrix2d &)) &db::Edges::transform, gsi::arg ("t"),
    "@brief Transform the edge collection (modifies self)\n"
    "\n"
    "Transforms the edge collection with the given 2d matrix transformation.\n"
    "This version modifies the edge collection and returns a reference to self.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge collection.\n"
    "\n"
    "This variant has been introduced in version 0.27."
  ) +
  method ("transform", (db::Edges &(db::Edges::*)(const db::IMatrix3d &)) &db::Edges::transform, gsi::arg ("t"),
    "@brief Transform the edge collection (modifies self)\n"
    "\n"
    "Transforms the edge collection with the given 3d matrix transformation.\n"
    "This version modifies the edge collection and returns a reference to self.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed edge collection.\n"
    "\n"
    "This variant has been introduced in version 0.27."
  ) +
  method_ext ("width_check", &width2, gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max"),
    "@brief Performs a width check with options\n"
    "@param d The minimum width for which the edges are checked\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The threshold angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper threshold of the projected length of one edge onto another\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the width check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "Use nil for this value to select the default (Euclidian metrics).\n"
    "\n"
    "\"ignore_angle\" specifies the angle threshold of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow selecting edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one threshold, pass nil to the respective value.\n"
  ) +
  method_ext ("space_check", &space2, gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max"),
    "@brief Performs a space check with options\n"
    "@param d The minimum distance for which the edges are checked\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The threshold angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper threshold of the projected length of one edge onto another\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the space check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "Use nil for this value to select the default (Euclidian metrics).\n"
    "\n"
    "\"ignore_angle\" specifies the angle threshold of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow selecting edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one threshold, pass nil to the respective value.\n"
  ) +
  method_ext ("inside_check|enclosed_check", &inside2, gsi::arg ("other"), gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max"),
    "@brief Performs an inside check with options\n"
    "@param d The minimum distance for which the edges are checked\n"
    "@param other The other edge collection against which to check\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The threshold angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper threshold of the projected length of one edge onto another\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the width check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "Use nil for this value to select the default (Euclidian metrics).\n"
    "\n"
    "\"ignore_angle\" specifies the angle threshold of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow selecting edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one threshold, pass nil to the respective value.\n"
    "\n"
    "The 'enclosed_check' alias was introduced in version 0.27.5.\n"
  ) +
  method_ext ("enclosing_check", &enclosing2, gsi::arg ("other"), gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max"),
    "@brief Performs an enclosing check with options\n"
    "@param d The minimum distance for which the edges are checked\n"
    "@param other The other edge collection against which to check\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The threshold angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper threshold of the projected length of one edge onto another\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the width check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "Use nil for this value to select the default (Euclidian metrics).\n"
    "\n"
    "\"ignore_angle\" specifies the angle threshold of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow selecting edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one threshold, pass nil to the respective value.\n"
  ) +
  method_ext ("overlap_check", &overlap2, gsi::arg ("other"), gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max"),
    "@brief Performs an overlap check with options\n"
    "@param d The minimum distance for which the edges are checked\n"
    "@param other The other edge collection against which to check\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The threshold angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper threshold of the projected length of one edge onto another\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the width check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "Use nil for this value to select the default (Euclidian metrics).\n"
    "\n"
    "\"ignore_angle\" specifies the angle threshold of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow selecting edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one threshold, pass nil to the respective value.\n"
  ) +
  method_ext ("separation_check", &separation2, gsi::arg ("other"), gsi::arg ("d"), gsi::arg ("whole_edges", false), gsi::arg ("metrics", db::Euclidian, "Euclidian"), gsi::arg ("ignore_angle", tl::Variant (), "default"), gsi::arg ("min_projection", tl::Variant (), "0"), gsi::arg ("max_projection", tl::Variant (), "max"),
    "@brief Performs an overlap check with options\n"
    "@param d The minimum distance for which the edges are checked\n"
    "@param other The other edge collection against which to check\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The threshold angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper threshold of the projected length of one edge onto another\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the width check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "Use nil for this value to select the default (Euclidian metrics).\n"
    "\n"
    "\"ignore_angle\" specifies the angle threshold of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow selecting edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one threshold, pass nil to the respective value.\n"
  ) +
  method_ext ("extents", &extents0,
    "@brief Returns a region with the bounding boxes of the edges\n"
    "This method will return a region consisting of the bounding boxes of the edges.\n"
    "The boxes will not be merged, so it is possible to determine overlaps "
    "of these boxes for example.\n"
  ) + 
  method_ext ("extents", &extents1, gsi::arg ("d"),
    "@brief Returns a region with the enlarged bounding boxes of the edges\n"
    "This method will return a region consisting of the bounding boxes of the edges enlarged by the given distance d.\n"
    "The enlargement is specified per edge, i.e the width and height will be increased by 2*d.\n"
    "The boxes will not be merged, so it is possible to determine overlaps "
    "of these boxes for example.\n"
  ) + 
  method_ext ("extents", &extents2, gsi::arg ("dx"), gsi::arg ("dy"),
    "@brief Returns a region with the enlarged bounding boxes of the edges\n"
    "This method will return a region consisting of the bounding boxes of the edges enlarged by the given distance dx in x direction and dy in y direction.\n"
    "The enlargement is specified per edge, i.e the width will be increased by 2*dx.\n"
    "The boxes will not be merged, so it is possible to determine overlaps "
    "of these boxes for example.\n"
  ) + 
  method_ext ("extended_in", &extended_in, gsi::arg ("e"),
    "@brief Returns a region with shapes representing the edges with the given width\n"
    "@param e The extension width\n"
    "@return A region containing the polygons representing these extended edges\n"
    "The edges are extended to the \"inside\" by the given distance \"e\". The distance will be applied to the right side "
    "as seen in the direction of the edge. By definition, this is the side pointing to the inside of the polygon if the edge "
    "was derived from a polygon.\n"
    "\n"
    "Other versions of this feature are \\extended_out and \\extended.\n"
  ) +
  method_ext ("extended_out", &extended_out, gsi::arg ("e"),
    "@brief Returns a region with shapes representing the edges with the given width\n"
    "@param e The extension width\n"
    "@return A region containing the polygons representing these extended edges\n"
    "The edges are extended to the \"outside\" by the given distance \"e\". The distance will be applied to the left side "
    "as seen in the direction of the edge. By definition, this is the side pointing to the outside of the polygon if the edge "
    "was derived from a polygon.\n"
    "\n"
    "Other versions of this feature are \\extended_in and \\extended.\n"
  ) +
  method_ext ("extended", &extended, gsi::arg ("b"), gsi::arg ("e"), gsi::arg ("o"), gsi::arg ("i"), gsi::arg ("join"),
    "@brief Returns a region with shapes representing the edges with the specified extensions\n"
    "@param b the parallel extension at the start point of the edge\n"
    "@param e the parallel extension at the end point of the edge\n"
    "@param o the perpendicular extension to the \"outside\" (left side as seen in the direction of the edge)\n"
    "@param i the perpendicular extension to the \"inside\" (right side as seen in the direction of the edge)\n"
    "@param join If true, connected edges are joined before the extension is applied\n"
    "@return A region containing the polygons representing these extended edges\n"
    "This is a generic version of \\extended_in and \\extended_out. It allows one to specify extensions for all four "
    "directions of an edge and to join the edges before the extension is applied.\n"
    "\n"
    "For degenerated edges forming a point, a rectangle with the b, e, o and i used as left, right, top and bottom distance to the "
    "center point of this edge is created.\n" 
    "\n"
    "If join is true and edges form a closed loop, the b and e parameters are ignored and a rim polygon is created "
    "that forms the loop with the outside and inside extension given by o and i.\n"
  ) +
  method ("start_segments", &db::Edges::start_segments, gsi::arg ("length"), gsi::arg ("fraction"),
    "@brief Returns edges representing a part of the edge after the start point\n"
    "@return A new collection of edges representing the start part\n"
    "This method allows one to specify the length of these segments in a twofold way: either as a fixed length or "
    "by specifying a fraction of the original length:\n"
    "\n"
    "@code\n"
    "edges = ...  # An edge collection\n"
    "edges.start_segments(100, 0.0)    # All segments have a length of 100 DBU\n"
    "edges.start_segments(0, 50.0)     # All segments have a length of half the original length\n"
    "edges.start_segments(100, 50.0)   # All segments have a length of half the original length\n"
    "                                  # or 100 DBU, whichever is larger\n"
    "@/code\n"
    "\n"
    "It is possible to specify 0 for both values. In this case, degenerated edges (points) are delivered which specify the "
    "start positions of the edges but can't participate in some functions.\n"
  ) +
  method ("end_segments", &db::Edges::end_segments, gsi::arg ("length"), gsi::arg ("fraction"),
    "@brief Returns edges representing a part of the edge before the end point\n"
    "@return A new collection of edges representing the end part\n"
    "This method allows one to specify the length of these segments in a twofold way: either as a fixed length or "
    "by specifying a fraction of the original length:\n"
    "\n"
    "@code\n"
    "edges = ...  # An edge collection\n"
    "edges.end_segments(100, 0.0)     # All segments have a length of 100 DBU\n"
    "edges.end_segments(0, 50.0)      # All segments have a length of half the original length\n"
    "edges.end_segments(100, 50.0)    # All segments have a length of half the original length\n"
    "                                  # or 100 DBU, whichever is larger\n"
    "@/code\n"
    "\n"
    "It is possible to specify 0 for both values. In this case, degenerated edges (points) are delivered which specify the "
    "end positions of the edges but can't participate in some functions.\n"
  ) +
  method ("centers", &db::Edges::centers, gsi::arg ("length"), gsi::arg ("fraction"),
    "@brief Returns edges representing the center part of the edges\n"
    "@return A new collection of edges representing the part around the center\n"
    "This method allows one to specify the length of these segments in a twofold way: either as a fixed length or "
    "by specifying a fraction of the original length:\n"
    "\n"
    "@code\n"
    "edges = ...  # An edge collection\n"
    "edges.centers(100, 0.0)     # All segments have a length of 100 DBU\n"
    "edges.centers(0, 50.0)      # All segments have a length of half the original length\n"
    "edges.centers(100, 50.0)    # All segments have a length of half the original length\n"
    "                            # or 100 DBU, whichever is larger\n"
    "@/code\n"
    "\n"
    "It is possible to specify 0 for both values. In this case, degenerated edges (points) are delivered which specify the "
    "centers of the edges but can't participate in some functions.\n"
  ) +
  method ("bbox", &db::Edges::bbox,
    "@brief Returns the bounding box of the edge collection\n"
    "The bounding box is the box enclosing all points of all edges.\n"
  ) +
  method_ext ("length", &length1,
    "@brief Returns the total length of all edges in the edge collection\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("length", &length2, gsi::arg ("rect"),
    "@brief Returns the total length of all edges in the edge collection (restricted to a rectangle)\n"
    "This version will compute the total length of all edges in the collection, restricting the computation to the given rectangle.\n"
    "Edges along the border are handled in a special way: they are counted when they are oriented with their inside "
    "side toward the rectangle (in other words: outside edges must coincide with the rectangle's border in order to be counted).\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("members_of|in", &in, gsi::arg ("other"),
    "@brief Returns all edges which are members of the other edge collection\n"
    "This method returns all edges in self which can be found in the other edge collection as well with exactly the same "
    "geometry."
  ) +
  method_ext ("not_members_of|not_in", &not_in, gsi::arg ("other"),
    "@brief Returns all edges which are not members of the other edge collection\n"
    "This method returns all edges in self which can not be found in the other edge collection with exactly the same "
    "geometry."
  ) +
  method_ext ("in_and_out", &in_and_out, gsi::arg ("other"),
    "@brief Returns all polygons which are members and not members of the other region\n"
    "This method is equivalent to calling \\members_of and \\not_members_of, but delivers both results at the same time and "
    "is more efficient than two separate calls. "
    "The first element returned is the \\members_of part, the second is the \\not_members_of part.\n"
    "\n"
    "This method has been introduced in version 0.28.\n"
  ) +
  method_ext ("is_deep?", &is_deep,
    "@brief Returns true if the edge collection is a deep (hierarchical) one\n"
    "\n"
    "This method has been added in version 0.26."
  ) +
  method_ext ("data_id", &id,
    "@brief Returns the data ID (a unique identifier for the underlying data storage)\n"
    "\n"
    "This method has been added in version 0.26."
  ) +
  method ("is_merged?", &db::Edges::is_merged,
    "@brief Returns true if the edge collection is merged\n"
    "If the region is merged, coincident edges have been merged into single edges. You can ensure merged state "
    "by calling \\merge.\n"
  ) +
  method ("is_empty?", &db::Edges::empty,
    "@brief Returns true if the edge collection is empty\n"
  ) +
  method ("count|#size", (size_t (db::Edges::*) () const) &db::Edges::count,
    "@brief Returns the (flat) number of edges in the edge collection\n"
    "\n"
    "This returns the number of raw edges (not merged edges if merged semantics is enabled).\n"
    "The count is computed 'as if flat', i.e. edges inside a cell are multiplied by the number of times a cell is instantiated.\n"
    "\n"
    "Starting with version 0.27, the method is called 'count' for consistency with \\Region. 'size' is still provided as an alias."
  ) +
  method ("hier_count", (size_t (db::Edges::*) () const) &db::Edges::hier_count,
    "@brief Returns the (hierarchical) number of edges in the edge collection\n"
    "\n"
    "This returns the number of raw edges (not merged edges if merged semantics is enabled).\n"
    "The count is computed 'hierarchical', i.e. edges inside a cell are counted once even if the cell is instantiated multiple times.\n"
    "\n"
    "This method has been introduced in version 0.27."
  ) +
  gsi::iterator ("each", &db::Edges::begin,
    "@brief Returns each edge of the region\n"
  ) +
  gsi::iterator ("each_merged", &db::Edges::begin_merged,
    "@brief Returns each edge of the region\n"
    "\n"
    "In contrast to \\each, this method delivers merged edges if merge semantics applies while \\each delivers the original edges only.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method ("[]", &db::Edges::nth, gsi::arg ("n"),
    "@brief Returns the nth edge of the collection\n"
    "\n"
    "This method returns nil if the index is out of range. It is available for flat edge collections only - i.e. "
    "those for which \\has_valid_edges? is true. Use \\flatten to explicitly flatten an edge collection.\n"
    "This method returns the raw edge (not merged edges, even if merged semantics is enabled).\n"
    "\n"
    "The \\each iterator is the more general approach to access the edges."
  ) +
  method ("flatten", &db::Edges::flatten,
    "@brief Explicitly flattens an edge collection\n"
    "\n"
    "If the collection is already flat (i.e. \\has_valid_edges? returns true), this method will "
    "not change it.\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  method ("has_valid_edges?", &db::Edges::has_valid_edges,
    "@brief Returns true if the edge collection is flat and individual edges can be accessed randomly\n"
    "\n"
    "This method has been introduced in version 0.26."
  ) +
  method_ext ("to_s", &to_string0,
    "@brief Converts the edge collection to a string\n"
    "The length of the output is limited to 20 edges to avoid giant strings on large regions. "
    "For full output use \"to_s\" with a maximum count parameter.\n"
  ) +
  method_ext ("to_s", &to_string1, gsi::arg ("max_count"),
    "@brief Converts the edge collection to a string\n"
    "This version allows specification of the maximum number of edges contained in the string."
  ) +
  method ("merged_semantics=", &db::Edges::set_merged_semantics, gsi::arg ("f"),
    "@brief Enable or disable merged semantics\n"
    "If merged semantics is enabled (the default), colinear, connected or overlapping edges will be considered\n"
    "as single edges.\n"
  ) + 
  method ("merged_semantics?", &db::Edges::merged_semantics,
    "@brief Gets a flag indicating whether merged semantics is enabled\n"
    "See \\merged_semantics= for a description of this attribute.\n"
  ) + 
  method ("enable_progress", &db::Edges::enable_progress, gsi::arg ("label"),
    "@brief Enable progress reporting\n"
    "After calling this method, the edge collection will report the progress through a progress bar while "
    "expensive operations are running.\n"
    "The label is a text which is put in front of the progress bar.\n"
    "Using a progress bar will imply a performance penalty of a few percent typically.\n"
  ) +
  method ("disable_progress", &db::Edges::disable_progress,
    "@brief Disable progress reporting\n"
    "Calling this method will disable progress reporting. See \\enable_progress.\n"
  ) +
  gsi::make_property_methods<db::Edges> ()
  ,
  "@brief A collection of edges (Not necessarily describing closed contours)\n"
  "\n\n"
  "This class was introduced to simplify operations on edges sets. "
  "See \\Edge for a description of the individual edge object. "
  "The edge collection contains an arbitrary number of edges and supports operations to select edges "
  "by various criteria, produce polygons from the edges by applying an extension, filtering edges "
  "against other edges collections and checking geometrical relations to other edges (DRC functionality)."
  "\n\n"
  "The edge collection is supposed to work closely with the \\Region polygon set. "
  "Both are related, although the edge collection has a lower rank since it potentially represents "
  "a disconnected collection of edges. "
  "Edge collections may form closed contours, for example immediately after they have been derived "
  "from a polygon set using \\Region#edges. But this state is volatile and can easily be destroyed by "
  "filtering edges. Hence the connected state does not play an important role in the edge collection's API."
  "\n\n"
  "Edge collections may also contain points (degenerated edges with identical start and end points). "
  "Such point-like objects participate in some although not all methods of the edge collection class. "
  "\n"
  "Edge collections can be used in two different flavors: in raw mode or merged semantics. With merged semantics (the "
  "default), connected edges are considered to belong together and are effectively merged.\n"
  "Overlapping parts are counted once in that mode. Dot-like edges are not considered in merged semantics.\n"
  "In raw mode (without merged semantics), each edge is considered as it is. Overlaps between edges\n"
  "may exists and merging has to be done explicitly using the \\merge method. The semantics can be\n"
  "selected using \\merged_semantics=.\n"
  "\n\n"
  "This class has been introduced in version 0.23.\n"
);

gsi::EnumIn<db::Edges, db::SpecialEdgeOrientationFilter::FilterType> decl_EdgesEdgeFilterType ("db", "EdgeType",
  gsi::enum_const ("OrthoEdges", db::SpecialEdgeOrientationFilter::Ortho,
    "@brief Horizontal and vertical edges are selected\n"
  ) +
  gsi::enum_const ("DiagonalEdges", db::SpecialEdgeOrientationFilter::Diagonal,
    "@brief Diagonal edges are selected (-45 and 45 degree)\n"
  ) +
  gsi::enum_const ("OrthoDiagonalEdges", db::SpecialEdgeOrientationFilter::OrthoDiagonal,
    "@brief Diagonal or orthogonal edges are selected (0, 90, -45 and 45 degree)\n"
  ),
  "@brief This enum specifies the edge type for edge angle filters.\n"
  "\n"
  "This enum was introduced in version 0.28.\n"
);

//  Inject the db::SpecialEdgeOrientationFilter::FilterType declarations into Edges:
gsi::ClassExt<db::Edges> decl_EdgesEdgeFilterType_into_parent (decl_EdgesEdgeFilterType.defs ());

}

