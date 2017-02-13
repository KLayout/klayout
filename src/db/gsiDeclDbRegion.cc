
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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

#include "dbRegion.h"
#include "dbPolygonTools.h"
#include "dbLayoutUtils.h"
#include "dbShapes.h"

namespace gsi
{

static db::Region *new_v ()
{
  return new db::Region ();
}

static db::Region *new_a (const std::vector <db::Polygon> &a)
{
  return new db::Region (a.begin (), a.end ());
}

static db::Region *new_b (const db::Box &o)
{
  return new db::Region (o);
}

static db::Region *new_p (const db::Polygon &o)
{
  return new db::Region (o);
}

static db::Region *new_ps (const db::SimplePolygon &o)
{
  return new db::Region (o);
}

static db::Region *new_path (const db::Path &o)
{
  return new db::Region (o);
}

static db::Region *new_shapes (const db::Shapes &s)
{
  db::Region *r = new db::Region ();
  for (db::Shapes::shape_iterator i = s.begin (db::ShapeIterator::All); !i.at_end (); ++i) {
    r->insert (*i);
  }
  return r;
}

static db::Region *new_si (const db::RecursiveShapeIterator &si)
{
  return new db::Region (si);
}

static db::Region *new_si2 (const db::RecursiveShapeIterator &si, const db::ICplxTrans &trans)
{
  return new db::Region (si, trans);
}

static std::string to_string0 (const db::Region *r)
{
  return r->to_string ();
}

static std::string to_string1 (const db::Region *r, size_t n)
{
  return r->to_string (n);
}

static db::Region::area_type area1 (const db::Region *r)
{
  return r->area ();
}

static db::Region::area_type area2 (const db::Region *r, const db::Box &rect)
{
  return r->area (rect);
}

static db::Region::perimeter_type perimeter1 (const db::Region *r)
{
  return r->perimeter ();
}

static db::Region::perimeter_type perimeter2 (const db::Region *r, const db::Box &rect)
{
  return r->perimeter (rect);
}

static void insert_a (db::Region *r, const std::vector <db::Polygon> &a)
{
  for (std::vector <db::Polygon>::const_iterator p = a.begin (); p != a.end (); ++p) {
    r->insert (*p);
  }
}

static void insert_si (db::Region *r, db::RecursiveShapeIterator si)
{
  while (! si.at_end ()) {
    r->insert (si.shape (), si.trans ());
    ++si;
  }
}

static void insert_si2 (db::Region *r, db::RecursiveShapeIterator si, db::ICplxTrans &trans)
{
  while (! si.at_end ()) {
    r->insert (si.shape (), trans * si.trans ());
    ++si;
  }
}

static db::Region &smooth (db::Region *r, db::Coord d)
{
  db::Region o;
  for (db::Region::const_iterator p = r->begin_merged (); ! p.at_end (); ++p) {
    o.insert (db::smooth (*p, d));
  }
  r->swap (o);
  return *r;
}

static db::Region minkowsky_sum_pe (const db::Region *r, const db::Edge &e)
{
  db::Region o;
  for (db::Region::const_iterator p = r->begin_merged (); ! p.at_end (); ++p) {
    o.insert (db::minkowsky_sum (*p, e, false));
  }
  return o;
}

static db::Region minkowsky_sum_pp (const db::Region *r, const db::Polygon &q)
{
  db::Region o;
  for (db::Region::const_iterator p = r->begin_merged (); ! p.at_end (); ++p) {
    o.insert (db::minkowsky_sum (*p, q, false));
  }
  return o;
}

static db::Region minkowsky_sum_pb (const db::Region *r, const db::Box &q)
{
  db::Region o;
  for (db::Region::const_iterator p = r->begin_merged (); ! p.at_end (); ++p) {
    o.insert (db::minkowsky_sum (*p, q, false));
  }
  return o;
}

static db::Region minkowsky_sum_pc (const db::Region *r, const std::vector<db::Point> &q)
{
  db::Region o;
  for (db::Region::const_iterator p = r->begin_merged (); ! p.at_end (); ++p) {
    o.insert (db::minkowsky_sum (*p, q, false));
  }
  return o;
}

static db::Region &move_p (db::Region *r, const db::Vector &p)
{
  r->transform (db::Disp (p));
  return *r;
}

static db::Region &move_xy (db::Region *r, db::Coord x, db::Coord y)
{
  r->transform (db::Disp (db::Vector (x, y)));
  return *r;
}

static db::Region moved_p (const db::Region *r, const db::Vector &p)
{
  return r->transformed (db::Disp (p));
}

static db::Region moved_xy (const db::Region *r, db::Coord x, db::Coord y)
{
  return r->transformed (db::Disp (db::Vector (x, y)));
}

static db::Region extents2 (const db::Region *r, db::Coord dx, db::Coord dy)
{
  db::Region e;
  e.reserve (r->size ());
  for (db::Region::const_iterator i = r->begin_merged (); ! i.at_end (); ++i) {
    e.insert (i->box ().enlarged (db::Vector (dx, dy)));
  }
  return e;
}

static db::Region extents1 (const db::Region *r, db::Coord d)
{
  return extents2 (r, d, d);
}

static db::Region extents0 (const db::Region *r)
{
  return extents2 (r, 0, 0);
}

static db::Region with_perimeter1 (const db::Region *r, db::Region::perimeter_type perimeter, bool inverse)
{
  db::RegionPerimeterFilter f (perimeter, perimeter + 1, inverse);
  return r->filtered (f);
}

static db::Region with_perimeter2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max, bool inverse)
{
  db::RegionPerimeterFilter f (min.is_nil () ? db::Region::perimeter_type (0) : min.to<db::Region::perimeter_type> (), max.is_nil () ? std::numeric_limits <db::Region::distance_type>::max () : max.to<db::Region::distance_type> (), inverse);
  return r->filtered (f);
}

static db::Region with_area1 (const db::Region *r, db::Region::area_type area, bool inverse)
{
  db::RegionAreaFilter f (area, area + 1, inverse);
  return r->filtered (f);
}

static db::Region with_area2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max, bool inverse)
{
  db::RegionAreaFilter f (min.is_nil () ? db::Region::area_type (0) : min.to<db::Region::area_type> (), max.is_nil () ? std::numeric_limits <db::Region::area_type>::max () : max.to<db::Region::area_type> (), inverse);
  return r->filtered (f);
}

static db::Region with_bbox_width1 (const db::Region *r, db::Region::distance_type bbox_width, bool inverse)
{
  db::RegionBBoxFilter f (bbox_width, bbox_width + 1, inverse, db::RegionBBoxFilter::BoxWidth);
  return r->filtered (f);
}

static db::Region with_bbox_width2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max, bool inverse)
{
  db::RegionBBoxFilter f (min.is_nil () ? db::Region::distance_type (0) : min.to<db::Region::distance_type> (), max.is_nil () ? std::numeric_limits <db::Region::distance_type>::max () : max.to<db::Region::distance_type> (), inverse, db::RegionBBoxFilter::BoxWidth);
  return r->filtered (f);
}

static db::Region with_bbox_height1 (const db::Region *r, db::Region::distance_type bbox_height, bool inverse)
{
  db::RegionBBoxFilter f (bbox_height, bbox_height + 1, inverse, db::RegionBBoxFilter::BoxHeight);
  return r->filtered (f);
}

static db::Region with_bbox_height2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max, bool inverse)
{
  db::RegionBBoxFilter f (min.is_nil () ? db::Region::distance_type (0) : min.to<db::Region::distance_type> (), max.is_nil () ? std::numeric_limits <db::Region::distance_type>::max () : max.to<db::Region::distance_type> (), inverse, db::RegionBBoxFilter::BoxHeight);
  return r->filtered (f);
}

static db::Region with_bbox_min1 (const db::Region *r, db::Region::distance_type bbox_min, bool inverse)
{
  db::RegionBBoxFilter f (bbox_min, bbox_min + 1, inverse, db::RegionBBoxFilter::BoxMinDim);
  return r->filtered (f);
}

static db::Region with_bbox_min2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max, bool inverse)
{
  db::RegionBBoxFilter f (min.is_nil () ? db::Region::distance_type (0) : min.to<db::Region::distance_type> (), max.is_nil () ? std::numeric_limits <db::Region::distance_type>::max () : max.to<db::Region::distance_type> (), inverse, db::RegionBBoxFilter::BoxMinDim);
  return r->filtered (f);
}

static db::Region with_bbox_max1 (const db::Region *r, db::Region::distance_type bbox_max, bool inverse)
{
  db::RegionBBoxFilter f (bbox_max, bbox_max + 1, inverse, db::RegionBBoxFilter::BoxMaxDim);
  return r->filtered (f);
}

static db::Region with_bbox_max2 (const db::Region *r, const tl::Variant &min, const tl::Variant &max, bool inverse)
{
  db::RegionBBoxFilter f (min.is_nil () ? db::Region::distance_type (0) : min.to<db::Region::distance_type> (), max.is_nil () ? std::numeric_limits <db::Region::distance_type>::max () : max.to<db::Region::distance_type> (), inverse, db::RegionBBoxFilter::BoxMaxDim);
  return r->filtered (f);
}

static db::EdgePairs angle_check1 (const db::Region *r, double angle, bool inverse)
{
  return r->angle_check (angle, angle + 1e-4, inverse);
}

static db::EdgePairs angle_check2 (const db::Region *r, double amin, double amax, bool inverse)
{
  return r->angle_check (amin, amax, inverse);
}

static db::Region in (const db::Region *r, const db::Region &other)
{
  return r->in (other, false);
}

static db::Region not_in (const db::Region *r, const db::Region &other)
{
  return r->in (other, true);
}

static db::Region rectangles (const db::Region *r)
{
  db::RectangleFilter f (false);
  return r->filtered (f);
}

static db::Region non_rectangles (const db::Region *r)
{
  db::RectangleFilter f (true);
  return r->filtered (f);
}

static db::Region rectilinear (const db::Region *r)
{
  db::RectilinearFilter f (false);
  return r->filtered (f);
}

static db::Region non_rectilinear (const db::Region *r)
{
  db::RectilinearFilter f (true);
  return r->filtered (f);
}

static db::Region &size_ext (db::Region *r, db::Coord d)
{
  r->size (d);
  return *r;
}

static db::Region sized_ext (db::Region *r, db::Coord d)
{
  return r->sized (d);
}

static db::Region &merge_ext1 (db::Region *r, int min_wc)
{
  r->merge (false, std::max (0, min_wc - 1));
  return *r;
}

static db::Region &merge_ext2 (db::Region *r, bool min_coherence, int min_wc)
{
  r->merge (min_coherence, std::max (0, min_wc - 1));
  return *r;
}

static db::Region merged_ext1 (db::Region *r, int min_wc)
{
  return r->merged (false, std::max (0, min_wc - 1));
}

static db::Region merged_ext2 (db::Region *r, bool min_coherence, int min_wc)
{
  return r->merged (min_coherence, std::max (0, min_wc - 1));
}

static db::EdgePairs width1 (const db::Region *r, db::Region::distance_type d) 
{
  return r->width_check (d);
}

static db::EdgePairs width2 (const db::Region *r, db::Region::distance_type d, bool whole_edges, const tl::Variant &metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection) 
{
  return r->width_check (d, whole_edges,
                         metrics.is_nil () ? db::Euclidian : db::metrics_type (metrics.to_int ()), 
                         ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                         min_projection.is_nil () ? db::Region::distance_type (0) : min_projection.to<db::Region::distance_type> (),
                         max_projection.is_nil () ? std::numeric_limits<db::Region::distance_type>::max () : max_projection.to<db::Region::distance_type> ());
}

static db::EdgePairs space1 (const db::Region *r, db::Region::distance_type d) 
{
  return r->space_check (d);
}

static db::EdgePairs space2 (const db::Region *r, db::Region::distance_type d, bool whole_edges, const tl::Variant &metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection) 
{
  return r->space_check (d, whole_edges,
                         metrics.is_nil () ? db::Euclidian : db::metrics_type (metrics.to_int ()), 
                         ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                         min_projection.is_nil () ? db::Region::distance_type (0) : min_projection.to<db::Region::distance_type> (),
                         max_projection.is_nil () ? std::numeric_limits<db::Region::distance_type>::max () : max_projection.to<db::Region::distance_type> ());
}

static db::EdgePairs notch1 (const db::Region *r, db::Region::distance_type d) 
{
  return r->notch_check (d);
}

static db::EdgePairs notch2 (const db::Region *r, db::Region::distance_type d, bool whole_edges, const tl::Variant &metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection) 
{
  return r->notch_check (d, whole_edges,
                         metrics.is_nil () ? db::Euclidian : db::metrics_type (metrics.to_int ()), 
                         ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                         min_projection.is_nil () ? db::Region::distance_type (0) : min_projection.to<db::Region::distance_type> (),
                         max_projection.is_nil () ? std::numeric_limits<db::Region::distance_type>::max () : max_projection.to<db::Region::distance_type> ());
}

static db::EdgePairs isolated1 (const db::Region *r, db::Region::distance_type d) 
{
  return r->isolated_check (d);
}

static db::EdgePairs isolated2 (const db::Region *r, db::Region::distance_type d, bool whole_edges, const tl::Variant &metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection) 
{
  return r->isolated_check (d, whole_edges,
                            metrics.is_nil () ? db::Euclidian : db::metrics_type (metrics.to_int ()), 
                            ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                            min_projection.is_nil () ? db::Region::distance_type (0) : min_projection.to<db::Region::distance_type> (),
                            max_projection.is_nil () ? std::numeric_limits<db::Region::distance_type>::max () : max_projection.to<db::Region::distance_type> ());
}

static db::EdgePairs inside1 (const db::Region *r, const db::Region &other, db::Region::distance_type d) 
{
  return r->inside_check (other, d);
}

static db::EdgePairs inside2 (const db::Region *r, const db::Region &other, db::Region::distance_type d, bool whole_edges, const tl::Variant &metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection) 
{
  return r->inside_check (other, d, whole_edges,
                          metrics.is_nil () ? db::Euclidian : db::metrics_type (metrics.to_int ()), 
                          ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                          min_projection.is_nil () ? db::Region::distance_type (0) : min_projection.to<db::Region::distance_type> (),
                          max_projection.is_nil () ? std::numeric_limits<db::Region::distance_type>::max () : max_projection.to<db::Region::distance_type> ());
}

static db::EdgePairs overlap1 (const db::Region *r, const db::Region &other, db::Region::distance_type d) 
{
  return r->overlap_check (other, d);
}

static db::EdgePairs overlap2 (const db::Region *r, const db::Region &other, db::Region::distance_type d, bool whole_edges, const tl::Variant &metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection) 
{
  return r->overlap_check (other, d, whole_edges,
                           metrics.is_nil () ? db::Euclidian : db::metrics_type (metrics.to_int ()), 
                           ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                           min_projection.is_nil () ? db::Region::distance_type (0) : min_projection.to<db::Region::distance_type> (),
                           max_projection.is_nil () ? std::numeric_limits<db::Region::distance_type>::max () : max_projection.to<db::Region::distance_type> ());
}

static db::EdgePairs enclosing1 (const db::Region *r, const db::Region &other, db::Region::distance_type d) 
{
  return r->enclosing_check (other, d);
}

static db::EdgePairs enclosing2 (const db::Region *r, const db::Region &other, db::Region::distance_type d, bool whole_edges, const tl::Variant &metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection) 
{
  return r->enclosing_check (other, d, whole_edges,
                             metrics.is_nil () ? db::Euclidian : db::metrics_type (metrics.to_int ()), 
                             ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                             min_projection.is_nil () ? db::Region::distance_type (0) : min_projection.to<db::Region::distance_type> (),
                             max_projection.is_nil () ? std::numeric_limits<db::Region::distance_type>::max () : max_projection.to<db::Region::distance_type> ());
}

static db::EdgePairs separation1 (const db::Region *r, const db::Region &other, db::Region::distance_type d) 
{
  return r->separation_check (other, d);
}

static db::EdgePairs separation2 (const db::Region *r, const db::Region &other, db::Region::distance_type d, bool whole_edges, const tl::Variant &metrics, const tl::Variant &ignore_angle, const tl::Variant &min_projection, const tl::Variant &max_projection) 
{
  return r->separation_check (other, d, whole_edges,
                              metrics.is_nil () ? db::Euclidian : db::metrics_type (metrics.to_int ()), 
                              ignore_angle.is_nil () ? 90 : ignore_angle.to_double (),
                              min_projection.is_nil () ? db::Region::distance_type (0) : min_projection.to<db::Region::distance_type> (),
                              max_projection.is_nil () ? std::numeric_limits<db::Region::distance_type>::max () : max_projection.to<db::Region::distance_type> ());
}

static int euclidian_metrics ()
{
  return db::Euclidian;
}

static int square_metrics ()
{
  return db::Square;
}

static int projection_metrics ()
{
  return db::Projection;
}

static db::Shapes decompose_convex (const db::Region *r, int mode)
{
  db::Shapes shapes;
  db::SimplePolygonContainer sp;
  for (db::Region::const_iterator p = r->begin_merged (); ! p.at_end(); ++p) {
    sp.polygons ().clear ();
    db::decompose_convex (*p, db::PreferredOrientation (mode), sp);
    for (std::vector <db::SimplePolygon>::const_iterator i = sp.polygons ().begin (); i != sp.polygons ().end (); ++i) {
      shapes.insert (*i);
    }
  }
  return shapes;
}

static db::Shapes decompose_trapezoids (const db::Region *r, int mode)
{
  db::Shapes shapes;
  db::SimplePolygonContainer sp;
  for (db::Region::const_iterator p = r->begin_merged (); ! p.at_end(); ++p) {
    sp.polygons ().clear ();
    db::decompose_trapezoids (*p, db::TrapezoidDecompositionMode (mode), sp);
    for (std::vector <db::SimplePolygon>::const_iterator i = sp.polygons ().begin (); i != sp.polygons ().end (); ++i) {
      shapes.insert (*i);
    }
  }
  return shapes;
}

//  provided by gsiDeclDbPolygon.cc:
int td_simple ();
int po_any ();

Class<db::Region> decl_Region ("Region", 
  constructor ("new", &new_v, 
    "@brief Default constructor\n"
    "\n"
    "This constructor creates an empty region.\n"
  ) +
  constructor ("new", &new_a, 
    "@brief Constructor from a polygon array\n"
    "@args array\n"
    "\n"
    "This constructor creates a region from an array of polygons.\n"
  ) +
  constructor ("new", &new_b, 
    "@brief Box constructor\n"
    "@args box\n"
    "\n"
    "This constructor creates a region from a box.\n"
  ) +
  constructor ("new", &new_p, 
    "@brief Polygon constructor\n"
    "@args polygon\n"
    "\n"
    "This constructor creates a region from a polygon.\n"
  ) +
  constructor ("new", &new_ps, 
    "@brief Simple polygon constructor\n"
    "@args polygon\n"
    "\n"
    "This constructor creates a region from a simple polygon.\n"
  ) +
  constructor ("new", &new_path, 
    "@brief Path constructor\n"
    "@args path\n"
    "\n"
    "This constructor creates a region from a path.\n"
  ) +
  constructor ("new", &new_shapes,
    "@brief Shapes constructor\n"
    "@args shapes\n"
    "\n"
    "This constructor creates a region from a \\Shapes collection.\n"
    "\n"
    "This constructor has been introduced in version 0.25."
  ) +
  constructor ("new", &new_si,
    "@brief Constructor from a hierarchical shape set\n"
    "@args shape_iterator\n"
    "\n"
    "This constructor creates a region from the shapes delivered by the given recursive shape iterator.\n"
    "Text objects and edges are not inserted, because they cannot be converted to polygons.\n"
    "This method allows to feed the shapes from a hierarchy of cells into the region.\n"
    "\n"
    "@code\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n" 
    "layer  = ... # the index of the layer from where to take the shapes from\n" 
    "r = RBA::Region::new(layout.begin_shapes(cell, layer))\n"
    "@/code\n"
  ) +
  constructor ("new", &new_si2, 
    "@brief Constructor from a hierarchical shape set with a transformation\n"
    "@args shape_iterator, trans\n"
    "\n"
    "This constructor creates a region from the shapes delivered by the given recursive shape iterator.\n"
    "Text objects and edges are not inserted, because they cannot be converted to polygons.\n"
    "On the delivered shapes it applies the given transformation.\n"
    "This method allows to feed the shapes from a hierarchy of cells into the region.\n"
    "The transformation is useful to scale to a specific database unit for example.\n"
    "\n"
    "@code\n"
    "layout = ... # a layout\n"
    "cell   = ... # the index of the initial cell\n" 
    "layer  = ... # the index of the layer from where to take the shapes from\n" 
    "dbu    = 0.1 # the target database unit\n"
    "r = RBA::Region::new(layout.begin_shapes(cell, layer), RBA::ICplxTrans::new(layout.dbu / dbu))\n"
    "@/code\n"
  ) +
  method ("merged_semantics=", &db::Region::set_merged_semantics,
    "@brief Enables or disables merged semantics\n"
    "@args f\n"
    "If merged semantics is enabled (the default), coherent polygons will be considered\n"
    "as single regions and artificial edges such as cut-lines will not be considered.\n"
    "Merged semantics thus is equivalent to considering coherent areas rather than\n"
    "single polygons\n"
  ) + 
  method ("merged_semantics?", &db::Region::merged_semantics,
    "@brief Gets a flag indicating whether merged semantics is enabled\n"
    "See \\merged_semantics= for a description of this attribute.\n"
  ) + 
  method ("strict_handling=", &db::Region::set_strict_handling,
    "@brief Enables or disables strict handling\n"
    "@args f\n"
    "\n"
    "Strict handling means to leave away some optimizations. Specifically the \n"
    "output of boolean operations will be merged even if one input is empty.\n"
    "Without strict handling, the operation will be optimized and output \n"
    "won't be merged.\n"
    "\n"
    "Strict handling is disabled by default and optimization is in place.\n"
    "\n"
    "This method has been introduced in version 0.23.2."
  ) + 
  method ("strict_handling?", &db::Region::strict_handling,
    "@brief Gets a flag indicating whether merged semantics is enabled\n"
    "See \\strict_handling= for a description of this attribute.\n"
    "\n"
    "This method has been introduced in version 0.23.2."
  ) + 
  method ("min_coherence=", &db::Region::set_min_coherence,
    "@brief Enable or disable minimum coherence\n"
    "@args f\n"
    "If minimum coherence is set, the merge operations (explicit merge with \\merge or\n"
    "implicit merge through merged_semantics) are performed using minimum coherence mode.\n"
    "The coherence mode determines how kissing-corner situations are resolved. If\n"
    "minimum coherence is selected, they are resolved such that multiple polygons are \n"
    "created which touch at a corner).\n"
    "\n"
    "The default setting is maximum coherence (min_coherence = false).\n"
  ) + 
  method ("min_coherence?", &db::Region::min_coherence,
    "@brief Gets a flag indicating whether minimum coherence is selected\n"
    "See \\min_coherence= for a description of this attribute.\n"
  ) + 
  method_ext ("with_perimeter", with_perimeter1, 
    "@brief Filter the polygons by perimeter\n"
    "@args perimeter, inverse\n"
    "Filters the polygons inside the region by perimeter. If \"inverse\" is false, only "
    "polygons which have the given perimeter are returned. If \"inverse\" is true, "
    "polygons not having the given perimeter are returned.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("with_perimeter", with_perimeter2, 
    "@brief Filter the polygons by perimeter\n"
    "@args min_perimeter, max_perimeter, inverse\n"
    "Filters the polygons inside the region by perimeter. If \"inverse\" is false, only "
    "polygons which have a perimeter larger or equal to \"min_perimeter\" and less than \"max_perimeter\" are "
    "returned. If \"inverse\" is true, "
    "polygons having a perimeter less than \"min_perimeter\" or larger or equal than \"max_perimeter\" are "
    "returned.\n"
    "\n"
    "If you don't want to specify a lower or upper limit, pass nil to that parameter.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("with_area", with_area1, 
    "@brief Filter the polygons by area\n"
    "@args area, inverse\n"
    "Filters the polygons inside the region by area. If \"inverse\" is false, only "
    "polygons which have the given area are returned. If \"inverse\" is true, "
    "polygons not having the given area are returned.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("with_area", with_area2, 
    "@brief Filter the polygons by area\n"
    "@args min_area, max_area, inverse\n"
    "Filters the polygons inside the region by area. If \"inverse\" is false, only "
    "polygons which have an area larger or equal to \"min_area\" and less than \"max_area\" are "
    "returned. If \"inverse\" is true, "
    "polygons having an area less than \"min_area\" or larger or equal than \"max_area\" are "
    "returned.\n"
    "\n"
    "If you don't want to specify a lower or upper limit, pass nil to that parameter.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("with_bbox_width", with_bbox_width1, 
    "@brief Filter the polygons by bounding box width\n"
    "@args width, inverse\n"
    "Filters the polygons inside the region by the width of their bounding box. If \"inverse\" is false, only "
    "polygons whose bounding box has the given width are returned. If \"inverse\" is true, "
    "polygons whose bounding box does not have the given width are returned.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("with_bbox_width", with_bbox_width2, 
    "@brief Filter the polygons by bounding box width\n"
    "@args min_width, max_width, inverse\n"
    "Filters the polygons inside the region by the width of their bounding box. If \"inverse\" is false, only "
    "polygons whose bounding box has a width larger or equal to \"min_width\" and less than \"max_width\" are "
    "returned. If \"inverse\" is true, all polygons not matching this criterion are returned."
    "\n"
    "If you don't want to specify a lower or upper limit, pass nil to that parameter.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("with_bbox_height", with_bbox_height1, 
    "@brief Filter the polygons by bounding box height\n"
    "@args height, inverse\n"
    "Filters the polygons inside the region by the height of their bounding box. If \"inverse\" is false, only "
    "polygons whose bounding box has the given height are returned. If \"inverse\" is true, "
    "polygons whose bounding box does not have the given height are returned.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("with_bbox_height", with_bbox_height2, 
    "@brief Filter the polygons by bounding box height\n"
    "@args min_height, max_height, inverse\n"
    "Filters the polygons inside the region by the height of their bounding box. If \"inverse\" is false, only "
    "polygons whose bounding box has a height larger or equal to \"min_height\" and less than \"max_height\" are "
    "returned. If \"inverse\" is true, all polygons not matching this criterion are returned."
    "\n"
    "If you don't want to specify a lower or upper limit, pass nil to that parameter.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("with_bbox_min", with_bbox_min1, 
    "@brief Filter the polygons by bounding box width or height, whichever is smaller\n"
    "@args dim, inverse\n"
    "Filters the polygons inside the region by the minimum dimension of their bounding box. "
    "If \"inverse\" is false, only polygons whose bounding box's smaller dimension is equal to the given value "
    "are returned. "
    "If \"inverse\" is true, all polygons not matching this criterion are returned."
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("with_bbox_min", with_bbox_min2, 
    "@brief Filter the polygons by bounding box width or height, whichever is smaller\n"
    "@args min_dim, max_dim, inverse\n"
    "Filters the polygons inside the region by the minimum dimension of their bounding box. "
    "If \"inverse\" is false, only polygons whose bounding box's smaller dimension is larger or equal to \"min_dim\" "
    "and less than \"max_dim\" are returned. "
    "If \"inverse\" is true, all polygons not matching this criterion are returned."
    "\n"
    "If you don't want to specify a lower or upper limit, pass nil to that parameter.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("with_bbox_max", with_bbox_max1, 
    "@brief Filter the polygons by bounding box width or height, whichever is larger\n"
    "@args dim, inverse\n"
    "Filters the polygons inside the region by the maximum dimension of their bounding box. "
    "If \"inverse\" is false, only polygons whose bounding box's larger dimension is equal to the given value "
    "are returned. "
    "If \"inverse\" is true, all polygons not matching this criterion are returned."
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("with_bbox_max", with_bbox_max2, 
    "@brief Filter the polygons by bounding box width or height, whichever is larger\n"
    "@args min_dim, max_dim, inverse\n"
    "Filters the polygons inside the region by the minimum dimension of their bounding box. "
    "If \"inverse\" is false, only polygons whose bounding box's larger dimension is larger or equal to \"min_dim\" "
    "and less than \"max_dim\" are returned. "
    "If \"inverse\" is true, all polygons not matching this criterion are returned."
    "\n"
    "If you don't want to specify a lower or upper limit, pass nil to that parameter.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method ("strange_polygon_check", &db::Region::strange_polygon_check, 
    "@brief Returns a region containing those parts of polygons which are \"strange\"\n"
    "Strange parts of polygons are self-overlapping parts or non-orientable parts (i.e. in the \"8\" configuration).\n"
    "\n"
    "Merged semantics does not apply for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method ("snapped", &db::Region::snapped, 
    "@brief Returns the snapped region\n"
    "@args gx, gy\n"
    "This method will snap the region to the given grid and return the snapped region (see \\snap). The original region is not modified.\n"
  ) +
  method ("snap", &db::Region::snap, 
    "@brief Snaps the region to the given grid\n"
    "@args gx, gy\n"
    "This method will snap the region to the given grid - each x or y coordinate is brought on the gx or gy grid by rounding "
    "to the nearest value which is a multiple of gx or gy.\n"
    "\n"
    "If gx or gy is 0 or less, no snapping happens in that direction.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method ("grid_check", &db::Region::grid_check, 
    "@brief Returns a marker for all vertices not being on the given grid\n"
    "@args gx, gy\n"
    "This method will return an edge pair object for every vertex whose x coordinate is not a multiple of gx or whose "
    "y coordinate is not a multiple of gy. The edge pair objects contain two edges consisting of the same single point - the "
    "original vertex.\n"
    "\n"
    "If gx or gy is 0 or less, the grid is not checked in that direction.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("with_angle", angle_check1, 
    "@brief Returns markers on every corner with the given angle (or not with the given angle)\n"
    "@args angle, inverse\n"
    "If the inverse flag is false, this method returns an error marker (an \\EdgePair object) for every corner whose connected edges "
    "form an angle with the given value (in degree). If the inverse flag is true, the method returns markers for every corner whose "
    "angle is not the given value.\n"
    "\n"
    "The edge pair objects returned will contain both edges forming the angle.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("with_angle", angle_check2, 
    "@brief Returns markers on every corner with an angle of more than amin and less than amax (or the opposite)\n"
    "@args amin, amax, inverse\n"
    "If the inverse flag is false, this method returns an error marker (an \\EdgePair object) for every corner whose connected edges "
    "form an angle whose value is more or equal to amin (in degree) or less (but not equal to) amax. If the inverse flag is true, the method returns markers for every corner whose "
    "angle is not matching that criterion.\n"
    "\n"
    "The edge pair objects returned will contain both edges forming the angle.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method ("insert", (void (db::Region::*)(const db::Box &)) &db::Region::insert, 
    "@brief Inserts a box\n"
    "@args box\n"
    "\n"
    "Inserts a box into the region.\n"
  ) +
  method ("insert", (void (db::Region::*)(const db::Polygon &)) &db::Region::insert, 
    "@brief Inserts a polygon\n"
    "@args polygon\n"
    "\n"
    "Inserts a polygon into the region.\n"
  ) +
  method ("insert", (void (db::Region::*)(const db::SimplePolygon &)) &db::Region::insert, 
    "@brief Inserts a simple polygon\n"
    "@args polygon\n"
    "\n"
    "Inserts a simple polygon into the region.\n"
  ) +
  method ("insert", (void (db::Region::*)(const db::Path &)) &db::Region::insert, 
    "@brief Inserts a path\n"
    "@args path\n"
    "\n"
    "Inserts a path into the region.\n"
  ) +
  method_ext ("insert", &insert_si, 
    "@brief Inserts all shapes delivered by the recursive shape iterator into this region\n"
    "@args shape_iterator\n"
    "\n"
    "This method will insert all shapes delivered by the shape iterator and insert them into the region.\n"
    "Text objects and edges are not inserted, because they cannot be converted to polygons.\n"
  ) +
  method_ext ("insert", &insert_si2, 
    "@brief Inserts all shapes delivered by the recursive shape iterator into this region with a transformation\n"
    "@args shape_iterator, trans\n"
    "\n"
    "This method will insert all shapes delivered by the shape iterator and insert them into the region.\n"
    "Text objects and edges are not inserted, because they cannot be converted to polygons.\n"
    "This variant will apply the given transformation to the shapes. This is useful to scale the "
    "shapes to a specific database unit for example.\n"
  ) +
  method_ext ("insert", &insert_a, 
    "@brief Inserts all polygons from the array into this region\n"
    "@args array\n"
  ) +
  method_ext ("extents", &extents0,
    "@brief Returns a region with the bounding boxes of the polygons\n"
    "This method will return a region consisting of the bounding boxes of the polygons.\n"
    "The boxes will not be merged, so it is possible to determine overlaps "
    "of these boxes for example.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method_ext ("extents", &extents1,
    "@brief Returns a region with the enlarged bounding boxes of the polygons\n"
    "@args d\n"
    "This method will return a region consisting of the bounding boxes of the polygons enlarged by the given distance d.\n"
    "The enlargement is specified per edge, i.e the width and height will be increased by 2*d.\n"
    "The boxes will not be merged, so it is possible to determine overlaps "
    "of these boxes for example.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method_ext ("extents", &extents2,
    "@brief Returns a region with the enlarged bounding boxes of the polygons\n"
    "@args dx, dy\n"
    "This method will return a region consisting of the bounding boxes of the polygons enlarged by the given distance dx in x direction and dy in y direction.\n"
    "The enlargement is specified per edge, i.e the width will be increased by 2*dx.\n"
    "The boxes will not be merged, so it is possible to determine overlaps "
    "of these boxes for example.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method ("merge", (db::Region &(db::Region::*) ()) &db::Region::merge,
    "@brief Merge the region\n"
    "\n"
    "@return The region after is has been merged (self).\n"
    "\n"
    "Merging removes overlaps and joins touching polygons.\n"
    "If the region is already merged, this method does nothing\n"
  ) +
  method_ext ("merge", &merge_ext1,
    "@brief Merge the region with options\n"
    "\n"
    "@args min_wc\n"
    "@param min_wc Overlap selection\n"
    "@return The region after is has been merged (self).\n"
    "\n"
    "Merging removes overlaps and joins touching polygons.\n"
    "This version provides one additional option: \"min_wc\" controls whether output is only produced if multiple "
    "polygons overlap. The value specifies the number of polygons that need to overlap. A value of 2 "
    "means that output is only produced if two or more polygons overlap.\n"
    "\n"
    "This method is equivalent to \"merge(false, min_wc).\n"
  ) +
  method_ext ("merge", &merge_ext2,
    "@brief Merge the region with options\n"
    "\n"
    "@args min_coherence, min_wc\n"
    "@param min_coherence A flag indicating whether the resulting polygons shall have minimum coherence\n"
    "@param min_wc Overlap selection\n"
    "@return The region after is has been merged (self).\n"
    "\n"
    "Merging removes overlaps and joins touching polygons.\n"
    "This version provides two additional options: if \"min_coherence\" is set to true, \"kissing corners\" are "
    "resolved by producing separate polygons. \"min_wc\" controls whether output is only produced if multiple "
    "polygons overlap. The value specifies the number of polygons that need to overlap. A value of 2 "
    "means that output is only produced if two or more polygons overlap.\n"
  ) +
  method ("merged", (db::Region (db::Region::*) () const) &db::Region::merged,
    "@brief Returns the merged region\n"
    "\n"
    "@return The region after is has been merged.\n"
    "\n"
    "Merging removes overlaps and joins touching polygons.\n"
    "If the region is already merged, this method does nothing.\n"
    "In contrast to \\merge, this method does not modify the region but returns a merged copy.\n"
  ) +
  method_ext ("merged", &merged_ext1,
    "@brief Returns the merged region (with options)\n"
    "@args min_wc\n"
    "\n"
    "@return The region after is has been merged.\n"
    "\n"
    "This version provides one additional options: \"min_wc\" controls whether output is only produced if multiple "
    "polygons overlap. The value specifies the number of polygons that need to overlap. A value of 2 "
    "means that output is only produced if two or more polygons overlap.\n"
    "\n"
    "This method is equivalent to \"merged(false, min_wc)\".\n"
    "\n"
    "In contrast to \\merge, this method does not modify the region but returns a merged copy.\n"
  ) +
  method_ext ("merged", &merged_ext2,
    "@brief Returns the merged region (with options)\n"
    "\n"
    "@args min_coherence, min_wc\n"
    "@param min_coherence A flag indicating whether the resulting polygons shall have minimum coherence\n"
    "@param min_wc Overlap selection\n"
    "@return The region after is has been merged (self).\n"
    "\n"
    "Merging removes overlaps and joins touching polygons.\n"
    "This version provides two additional options: if \"min_coherence\" is set to true, \"kissing corners\" are "
    "resolved by producing separate polygons. \"min_wc\" controls whether output is only produced if multiple "
    "polygons overlap. The value specifies the number of polygons that need to overlap. A value of 2 "
    "means that output is only produced if two or more polygons overlap.\n"
    "\n"
    "In contrast to \\merge, this method does not modify the region but returns a merged copy.\n"
  ) +
  method ("round_corners", &db::Region::round_corners,
    "@brief Corner rounding\n"
    "@args r_inner, r_outer, n\n"
    "@param r_inner Inner corner radius (in database units)\n"
    "@param r_outer Outer corner radius (in database units)\n"
    "@param n The number of points per circle\n"
    "\n"
    "This method rounds the corners of the polygons in the region. Inner corners will be rounded with "
    "a radius of r_inner and outer corners with a radius of r_outer. The circles will be approximated "
    "by segments using n segments per full circle.\n"
    "\n"
    "This method modifies the region. \\rounded_corners is a method that does the same but returns a new "
    "region without modifying self. Merged semantics applies for this method.\n"
  ) +
  method ("rounded_corners", &db::Region::rounded_corners,
    "@brief Corner rounding\n"
    "@args r_inner, r_outer, n\n"
    "@param r_inner Inner corner radius (in database units)\n"
    "@param r_outer Outer corner radius (in database units)\n"
    "@param n The number of points per circle\n"
    "\n"
    "See \\round_corners for a description of this method. This version returns a new region instead of "
    "modifying self (out-of-place)."
  ) +
  method ("size", (db::Region & (db::Region::*) (db::Coord, db::Coord, unsigned int)) &db::Region::size,
    "@brief Anisotropic sizing (biasing)\n"
    "\n"
    "@args dx, dy, mode\n"
    "@return The region after the sizing has applied (self)\n"
    "\n"
    "Shifts the contour outwards (dx,dy>0) or inwards (dx,dy<0).\n"
    "dx is the sizing in x-direction and dy is the sizing in y-direction. The sign of dx and dy should be identical.\n"
    "\n"
    "This method applies a sizing to the region. Before the sizing is done, the\n"
    "region is merged if this is not the case already.\n"
    "\n"
    "The mode defines at which bending angle cutoff occurs \n"
    "(0:>0, 1:>45, 2:>90, 3:>135, 4:>approx. 168, other:>approx. 179)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
    "\n"
    "The result is a set of polygons which may be overlapping, but are not self-\n"
    "intersecting. Polygons may overlap afterwards because they grew big enough to overlap their neighbors.\n"
    "In that case, \\merge can be used to detect this overlaps by setting the \"min_wc\" parameter to value 1:\n"
    "\n"
    "@code\n"
    "r = RBA::Region::new\n"
    "r.insert(RBA::Box::new(0, 0, 50, 50))\n"
    "r.insert(RBA::Box::new(100, 0, 150, 50))\n"
    "r.size(50, 2)\n"
    "r.merge(false, 1)\n"
    "# r now is (50,-50;50,100;100,100;100,-50)\n"
    "@/code\n"
  ) + 
  method ("size", (db::Region & (db::Region::*) (db::Coord, unsigned int)) &db::Region::size,
    "@brief Isotropic sizing (biasing)\n"
    "\n"
    "@args d, mode\n"
    "@return The region after the sizing has applied (self)\n"
    "\n"
    "This method is equivalent to \"size(d, d, mode)\".\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method_ext ("size", size_ext,
    "@brief Isotropic sizing (biasing)\n"
    "\n"
    "@args d, mode\n"
    "@return The region after the sizing has applied (self)\n"
    "\n"
    "This method is equivalent to \"size(d, d, 2)\".\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method ("sized", (db::Region (db::Region::*) (db::Coord, db::Coord, unsigned int) const) &db::Region::sized,
    "@brief Returns the anisotropically sized region\n"
    "\n"
    "@args dx, dy, mode\n"
    "@return The sized region\n"
    "\n"
    "This method is returns the sized region (see \\size), but does not modify self.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method ("sized", (db::Region (db::Region::*) (db::Coord, unsigned int) const) &db::Region::sized,
    "@brief Returns the isotropically sized region\n"
    "\n"
    "@args d, mode\n"
    "@return The sized region\n"
    "\n"
    "This method is returns the sized region (see \\size), but does not modify self.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method_ext ("sized", sized_ext,
    "@brief Isotropic sizing (biasing)\n"
    "\n"
    "@args d, mode\n"
    "@return The region after the sizing has applied (self)\n"
    "\n"
    "This method is equivalent to \"sized(d, d, 2)\".\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method ("&", &db::Region::operator&,
    "@brief Returns the boolean AND between self and the other region\n"
    "\n"
    "@args other\n"
    "@return The result of the boolean AND operation\n"
    "\n"
    "This method will compute the boolean AND (intersection) between two regions. "
    "The result is often but not necessarily always merged.\n"
  ) + 
  method ("&=", &db::Region::operator&=,
    "@brief Performs the boolean AND between self and the other region\n"
    "\n"
    "@args other\n"
    "@return The region after modification (self)\n"
    "\n"
    "This method will compute the boolean AND (intersection) between two regions. "
    "The result is often but not necessarily always merged.\n"
  ) + 
  method ("-", &db::Region::operator-,
    "@brief Returns the boolean NOT between self and the other region\n"
    "\n"
    "@args other\n"
    "@return The result of the boolean NOT operation\n"
    "\n"
    "This method will compute the boolean NOT (intersection) between two regions. "
    "The result is often but not necessarily always merged.\n"
  ) + 
  method ("-=", &db::Region::operator-=,
    "@brief Performs the boolean NOT between self and the other region\n"
    "\n"
    "@args other\n"
    "@return The region after modification (self)\n"
    "\n"
    "This method will compute the boolean NOT (intersection) between two regions. "
    "The result is often but not necessarily always merged.\n"
  ) + 
  method ("^", &db::Region::operator^,
    "@brief Returns the boolean NOT between self and the other region\n"
    "\n"
    "@args other\n"
    "@return The result of the boolean XOR operation\n"
    "\n"
    "This method will compute the boolean XOR (intersection) between two regions. "
    "The result is often but not necessarily always merged.\n"
  ) + 
  method ("^=", &db::Region::operator^=,
    "@brief Performs the boolean XOR between self and the other region\n"
    "\n"
    "@args other\n"
    "@return The region after modification (self)\n"
    "\n"
    "This method will compute the boolean XOR (intersection) between two regions. "
    "The result is often but not necessarily always merged.\n"
  ) + 
  method ("\\|", &db::Region::operator|,
    "@brief Returns the boolean OR between self and the other region\n"
    "\n"
    "@args other\n"
    "@return The resulting region\n"
    "\n"
    "The boolean OR is implemented by merging the polygons of both regions. To simply join the regions "
    "without merging, the + operator is more efficient."
  ) + 
  method ("\\|=", &db::Region::operator|=,
    "@brief Performs the boolean OR between self and the other region\n"
    "\n"
    "@args other\n"
    "@return The region after modification (self)\n"
    "\n"
    "The boolean OR is implemented by merging the polygons of both regions. To simply join the regions "
    "without merging, the + operator is more efficient."
  ) + 
  method ("+", &db::Region::operator+,
    "@brief Returns the combined region of self and the other region\n"
    "\n"
    "@args other\n"
    "@return The resulting region\n"
    "\n"
    "This operator adds the polygons of the other region to self and returns a new combined region. "
    "This usually creates unmerged regions and polygons may overlap. Use \\merge if you want to ensure the result region is merged.\n"
  ) + 
  method ("+=", &db::Region::operator+=,
    "@brief Adds the polygons of the other region to self\n"
    "\n"
    "@args other\n"
    "@return The region after modification (self)\n"
    "\n"
    "This operator adds the polygons of the other region to self. "
    "This usually creates unmerged regions and polygons may overlap. Use \\merge if you want to ensure the result region is merged.\n"
  ) + 
  method ("inside", &db::Region::selected_inside,
    "@brief Returns the polygons of this region which are completely inside polygons from the other region\n"
    "\n"
    "@args other\n"
    "@return A new region containing the polygons which are inside polygons from the other region\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method ("not_inside", &db::Region::selected_not_inside,
    "@brief Returns the polygons of this region which are not completely inside polygons from the other region\n"
    "\n"
    "@args other\n"
    "@return A new region containing the polygons which are not inside polygons from the other region\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method ("select_inside", &db::Region::select_inside,
    "@brief Selects the polygons of this region which are completely inside polygons from the other region\n"
    "\n"
    "@args other\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method ("select_not_inside", &db::Region::select_not_inside,
    "@brief Selects the polygons of this region which are not completely inside polygons from the other region\n"
    "\n"
    "@args other\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method ("outside", &db::Region::selected_outside,
    "@brief Returns the polygons of this region which are completely outside polygons from the other region\n"
    "\n"
    "@args other\n"
    "@return A new region containing the polygons which are outside polygons from the other region\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method ("not_outside", &db::Region::selected_not_outside,
    "@brief Returns the polygons of this region which are not completely outside polygons from the other region\n"
    "\n"
    "@args other\n"
    "@return A new region containing the polygons which are not outside polygons from the other region\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method ("select_outside", &db::Region::select_outside,
    "@brief Selects the polygons of this region which are completely outside polygons from the other region\n"
    "\n"
    "@args other\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method ("select_not_outside", &db::Region::select_not_outside,
    "@brief Selects the polygons of this region which are not completely outside polygons from the other region\n"
    "\n"
    "@args other\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method ("interacting", &db::Region::selected_interacting,
    "@brief Returns the polygons of this region which overlap or touch polygons from the other region\n"
    "\n"
    "@args other\n"
    "@return A new region containing the polygons overlapping or touching polygons from the other region\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method ("not_interacting", &db::Region::selected_not_interacting,
    "@brief Returns the polygons of this region which do not overlap or touch polygons from the other region\n"
    "\n"
    "@args other\n"
    "@return A new region containing the polygons not overlapping or touching polygons from the other region\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method ("select_interacting", &db::Region::select_interacting,
    "@brief Selects the polygons from this region which overlap or touch polygons from the other region\n"
    "\n"
    "@args other\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method ("select_not_interacting", &db::Region::select_not_interacting,
    "@brief Selects the polygons from this region which do not overlap or touch polygons from the other region\n"
    "\n"
    "@args other\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method ("overlapping", &db::Region::selected_overlapping,
    "@brief Returns the polygons of this region which overlap polygons from the other region\n"
    "\n"
    "@args other\n"
    "@return A new region containing the polygons overlapping polygons from the other region\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method ("not_overlapping", &db::Region::selected_not_overlapping,
    "@brief Returns the polygons of this region which do not overlap polygons from the other region\n"
    "\n"
    "@args other\n"
    "@return A new region containing the polygons not overlapping polygons from the other region\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method ("select_overlapping", &db::Region::select_overlapping,
    "@brief Selects the polygons from this region which overlap polygons from the other region\n"
    "\n"
    "@args other\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method ("select_not_overlapping", &db::Region::select_not_overlapping,
    "@brief Selects the polygons from this region which do not overlap polygons from the other region\n"
    "\n"
    "@args other\n"
    "@return The region after the polygons have been selected (self)\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method ("is_box?", &db::Region::is_box,
    "@brief Returns true, if the region is a simple box\n"
    "\n"
    "@return True if the region is a box.\n"
    "\n"
    "This method does not apply implicit merging if merge semantics is enabled.\n"
    "If the region is not merged, this method may return false even\n"
    "if the merged region would be a box.\n"
  ) +
  method ("edges", (db::Edges (db::Region::*) () const) &db::Region::edges,
    "@brief Returns an edge collection representing all edges of the polygons in this region\n"
    "This method will decompose the polygons into the individual edges. Edges making up the hulls "
    "of the polygons are oriented clockwise while edges making up the holes are oriented counterclockwise.\n"
    "\n"
    "The edge collection returned can be manipulated in various ways. See \\Edges for a description of the "
    "possibilities of the edge collection.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) + 
  method_ext ("decompose_convex", &decompose_convex, gsi::arg ("preferred_orientation", po_any (), "\\Polygon#PO_any"),
    "@brief Decomposes the region into convex pieces.\n"
    "\n"
    "This method will return a \\Shapes container that holds a decomposition of the region into convex, simple polygons.\n"
    "See \\Polygon#decompose_convex for details.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method_ext ("decompose_trapezoids", &decompose_trapezoids, gsi::arg ("mode", td_simple (), "\\Polygon#TD_simple"),
    "@brief Decomposes the region into trapezoids.\n"
    "\n"
    "This method will return a \\Shapes container that holds a decomposition of the region into trapezoids.\n"
    "See \\Polygon#decompose_trapezoids for details.\n"
    "\n"
    "This method has been introduced in version 0.25."
  ) +
  method ("clear", &db::Region::clear,
    "@brief Clears the region\n"
  ) +
  method ("swap", &db::Region::swap,
    "@brief Swap the contents of this region with the contents of another region\n"
    "@args other\n"
    "This method is useful to avoid excessive memory allocation in some cases. "
    "For managed memory languages such as Ruby, those cases will be rare. " 
  ) +
  method_ext ("smooth", &smooth,
    "@brief Smooth the region\n"
    "@args d\n"
    "\n"
    "Remove vertices that deviate by more than the distance d from the average contours.\n"
    "The value d is basically the roughness which is removed.\n"
    "This method will apply smoothing to all polygons in the region.\n"
    "This method will modify the region it is called on.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
    "\n"
    "@param d The smoothing \"roughness\".\n"
    "\n"
    "@return The smoothed region (self).\n"
  ) +
  method ("holes", &db::Region::holes,
    "@brief Returns the holes of the region\n"
    "This method returns all holes as filled polygons.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
    "If merge semantics is not enabled, the holes may not be detected if the polygons "
    "are taken from a hole-less representation (i.e. GDS2 file). Use explicit merge (\\merge method) "
    "in order to merge the polygons and detect holes.\n"
  ) +
  method ("hulls", &db::Region::hulls,
    "@brief Returns the hulls of the region\n"
    "This method returns all hulls as polygons. The holes will be removed (filles). "
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
    "If merge semantics is not enabled, the hull may also enclose holes if the polygons "
    "are taken from a hole-less representation (i.e. GDS2 file). Use explicit merge (\\merge method) "
    "in order to merge the polygons and detect holes.\n"
  ) +
  method_ext ("members_of|#in", &in,
    "@brief Returns all polygons which are members of the other region\n"
    "@args other\n"
    "This method returns all polygons in self which can be found in the other region as well with exactly the same "
    "geometry."
  ) +
  method_ext ("not_members_of|#not_in", &not_in,
    "@brief Returns all polygons which are not members of the other region\n"
    "@args other\n"
    "This method returns all polygons in self which can not be found in the other region with exactly the same "
    "geometry."
  ) +
  method_ext ("rectangles", &rectangles,
    "@brief Returns all polygons which are rectangles\n"
    "This method returns all polygons in self which are rectangles."
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("non_rectangles", &non_rectangles,
    "@brief Returns all polygons which are not rectangles\n"
    "This method returns all polygons in self which are not rectangles."
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("rectilinear", &rectilinear,
    "@brief Returns all polygons which are rectilinear\n"
    "This method returns all polygons in self which are rectilinear."
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("non_rectilinear", &non_rectilinear,
    "@brief Returns all polygons which are not rectilinear\n"
    "This method returns all polygons in self which are not rectilinear."
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("minkowsky_sum", &minkowsky_sum_pe,
    "@brief Compute the Minkowsky sum of the region and an edge\n"
    "@args e\n"
    "\n"
    "@param e The edge.\n"
    "\n"
    "@return The new polygons representing the Minkowsky sum with the edge e.\n"
    "\n"
    "The Minkowsky sum of a region and an edge basically results in the area covered when "
    "\"dragging\" the region along the line given by the edge. The effect is similar to drawing the line "
    "with a pencil that has the shape of the given region.\n"
    "\n"
    "The resulting polygons are not merged. In order to remove overlaps, use the \\merge or \\merged method."
    "Merged semantics applies for the input of this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("minkowsky_sum", &minkowsky_sum_pp,
    "@brief Compute the Minkowsky sum of the region and a polygon\n"
    "@args p\n"
    "\n"
    "@param p The first argument.\n"
    "\n"
    "@return The new polygons representing the Minkowsky sum of self and p.\n"
    "\n"
    "The Minkowsky sum of a region and a polygon is basically the result of \"painting\" "
    "the region with a pen that has the shape of the second polygon.\n"
    "\n"
    "The resulting polygons are not merged. In order to remove overlaps, use the \\merge or \\merged method."
    "Merged semantics applies for the input of this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("minkowsky_sum", &minkowsky_sum_pb,
    "@brief Compute the Minkowsky sum of the region and a box\n"
    "@args b\n"
    "\n"
    "@param b The box.\n"
    "\n"
    "@return The new polygons representing the Minkowsky sum of self and the box.\n"
    "\n"
    "The result is equivalent to the region-with-polygon Minkowsky sum with the box used "
    "as the second polygon.\n"
    "\n"
    "The resulting polygons are not merged. In order to remove overlaps, use the \\merge or \\merged method."
    "Merged semantics applies for the input of this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("minkowsky_sum", &minkowsky_sum_pc,
    "@brief Compute the Minkowsky sum of the region and a contour of points (a trace)\n"
    "@args b\n"
    "\n"
    "@param b The contour (a series of points forming the trace).\n"
    "\n"
    "@return The new polygons representing the Minkowsky sum of self and the contour.\n"
    "\n"
    "The Minkowsky sum of a region and a contour basically results in the area covered when "
    "\"dragging\" the region along the contour. The effect is similar to drawing the contour "
    "with a pencil that has the shape of the given region.\n"
    "\n"
    "The resulting polygons are not merged. In order to remove overlaps, use the \\merge or \\merged method."
    "Merged semantics applies for the input of this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("move", &move_p,
    "@brief Moves the region\n"
    "@args v\n"
    "\n"
    "Moves the polygon by the given offset and returns the \n"
    "moved region. The region is overwritten.\n"
    "\n"
    "@param v The distance to move the region.\n"
    "\n"
    "Starting with version 0.25 this method accepts a vector argument.\n"
    "\n"
    "@return The moved region (self).\n"
  ) +
  method_ext ("move", &move_xy,
    "@brief Moves the region\n"
    "@args x,y\n"
    "\n"
    "Moves the region by the given offset and returns the \n"
    "moved region. The region is overwritten.\n"
    "\n"
    "@param x The x distance to move the region.\n"
    "@param y The y distance to move the region.\n"
    "\n"
    "@return The moved region (self).\n"
  ) +
  method_ext ("moved", &moved_p,
    "@brief Returns the moved region (does not modify self)\n"
    "@args p\n"
    "\n"
    "Moves the region by the given offset and returns the \n"
    "moved region. The region is not modified.\n"
    "\n"
    "Starting with version 0.25 this method accepts a vector argument.\n"
    "\n"
    "@param p The distance to move the region.\n"
    "\n"
    "@return The moved region.\n"
  ) +
  method_ext ("moved", &moved_xy,
    "@brief Returns the moved region (does not modify self)\n"
    "@args x,y\n"
    "\n"
    "Moves the region by the given offset and returns the \n"
    "moved region. The region is not modified.\n"
    "\n"
    "@param x The x distance to move the region.\n"
    "@param y The y distance to move the region.\n"
    "\n"
    "@return The moved region.\n"
  ) +
  method ("transform", (db::Region &(db::Region::*)(const db::Trans &)) &db::Region::transform,
    "@brief Transform the region (modifies self)\n"
    "@args t\n"
    "\n"
    "Transforms the region with the given transformation.\n"
    "This version modifies the region and returns a reference to self.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed region.\n"
  ) +
  method ("transform|#transform_icplx", (db::Region &(db::Region::*)(const db::ICplxTrans &)) &db::Region::transform,
    "@brief Transform the region with a complex transformation (modifies self)\n"
    "@args t\n"
    "\n"
    "Transforms the region with the given transformation.\n"
    "This version modifies the region and returns a reference to self.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed region.\n"
  ) +
  method ("transformed", (db::Region (db::Region::*)(const db::Trans &) const) &db::Region::transformed,
    "@brief Transform the region\n"
    "@args t\n"
    "\n"
    "Transforms the region with the given transformation.\n"
    "Does not modify the region but returns the transformed region.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed region.\n"
  ) +
  method ("transformed|#transformed_icplx", (db::Region (db::Region::*)(const db::ICplxTrans &) const) &db::Region::transformed,
    "@brief Transform the region with a complex transformation\n"
    "@args t\n"
    "\n"
    "Transforms the region with the given complex transformation.\n"
    "Does not modify the region but returns the transformed region.\n"
    "\n"
    "@param t The transformation to apply.\n"
    "\n"
    "@return The transformed region.\n"
  ) +
  method_ext ("width_check", &width1,
    "@brief Performs a width check\n"
    "@args d\n"
    "@param d The minimum width for which the polygons are checked\n"
    "Performs a width check against the minimum width \"d\". For locations where a polygon has a "
    "width less than the given value, an error marker is produced. Error markers form a "
    "\\EdgePairs collection. Edge pairs are pairs of edges where each edge marks one edge of the original "
    "polygon. Edge pairs can be converted back to polygons or separated into their edge contributions.\n"
    "See \\EdgePairs for a description of that collection object.\n"
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("width_check", &width2,
    "@brief Performs a width check with options\n"
    "@args d, whole_edges, metrics, ignore_angle, min_projection, max_projection\n"
    "@param d The minimum width for which the polygons are checked\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper limit of the projected length of one edge onto another\n"
    "\n"
    "This version is similar to the simple version with one parameter. In addition, it allows "
    "to specify many more options.\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the width check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "Use nil for this value to select the default (Euclidian metrics).\n"
    "\n"
    "\"ignore_angle\" specifies the angle limit of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow to select edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one limit, pass nil to the respective value.\n"
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("space_check", &space1,
    "@brief Performs a space check\n"
    "@args d\n"
    "@param d The minimum space for which the polygons are checked\n"
    "Performs a space check against the minimum space \"d\". For locations where a polygon has a "
    "space less than the given value to either itself (a notch) or to other polygons, an error marker is produced. Error markers form a "
    "\\EdgePairs collection. Edge pairs are pairs of edges where each edge marks one edge of the original "
    "polygon. Edge pairs can be converted back to polygons or separated into their edge contributions.\n"
    "See \\EdgePairs for a description of that collection object.\n"
    "\n"
    "\\notch_check is a version which checks spacing of polygon edges only against edges of the same polygon.\n"
    "\\isolated_check is a version which checks spacing between different polygons only.\n"
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("space_check", &space2,
    "@brief Performs a space check with options\n"
    "@args d, whole_edges, metrics, ignore_angle, min_projection, max_projection\n"
    "@param d The minimum space for which the polygons are checked\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper limit of the projected length of one edge onto another\n"
    "\n"
    "This version is similar to the simple version with one parameter. In addition, it allows "
    "to specify many more options.\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the space check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "Use nil for this value to select the default (Euclidian metrics).\n"
    "\n"
    "\"ignore_angle\" specifies the angle limit of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow to select edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one limit, pass nil to the respective value.\n"
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("notch_check", &notch1,
    "@brief Performs a space check between edges of the same polygon\n"
    "@args d\n"
    "@param d The minimum space for which the polygons are checked\n"
    "Performs a space check against the minimum space \"d\". For locations where a polygon has a "
    "space less than the given value to either itself (a notch) or to other polygons, an error marker is produced. Error markers form a "
    "\\EdgePairs collection. Edge pairs are pairs of edges where each edge marks one edge of the original "
    "polygon. Edge pairs can be converted back to polygons or separated into their edge contributions.\n"
    "See \\EdgePairs for a description of that collection object.\n"
    "\n"
    "This version is restricted to checking edges of one polygon vs. edges of itself.\n"
    "To ensure that the polygon is merged and does not come in pieces, use the \\merge method before.\n"
    "\\space_check is a version which checks spacing of all polygon edges vs. edges of the some or other polygons.\n"
    "\\isolated_check is a version which checks spacing between different polygons only.\n"
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("notch_check", &notch2,
    "@brief Performs a space check between edges of the same polygon with options\n"
    "@args d, whole_edges, metrics, ignore_angle, min_projection, max_projection\n"
    "@param d The minimum space for which the polygons are checked\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper limit of the projected length of one edge onto another\n"
    "\n"
    "This version is similar to the simple version with one parameter. In addition, it allows "
    "to specify many more options.\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the space check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "Use nil for this value to select the default (Euclidian metrics).\n"
    "\n"
    "\"ignore_angle\" specifies the angle limit of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow to select edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one limit, pass nil to the respective value.\n"
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("isolated_check", &isolated1,
    "@brief Performs a space check between edges of different polygons\n"
    "@args d\n"
    "@param d The minimum space for which the polygons are checked\n"
    "Performs a space check against the minimum space \"d\". For locations where a polygon has a "
    "space less than the given value to other polygons (not itself), an error marker is produced. Error markers form a "
    "\\EdgePairs collection. Edge pairs are pairs of edges where each edge marks one edge of the original "
    "polygon. Edge pairs can be converted back to polygons or separated into their edge contributions.\n"
    "See \\EdgePairs for a description of that collection object.\n"
    "\n"
    "This version is restricted to checking edges of one polygon vs. edges of other polygons.\n"
    "To ensure that the polygon is merged and does not come in pieces, use the \\merge method before.\n"
    "\\space_check is a version which checks spacing of all polygon edges vs. edges of the some or other polygons.\n"
    "\\notch_check is a version which checks spacing of polygons edges of the same polygon only.\n"
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("isolated_check", &isolated2,
    "@brief Performs a space check between edges of different polygons with options\n"
    "@args d, whole_edges, metrics, ignore_angle, min_projection, max_projection\n"
    "@param d The minimum space for which the polygons are checked\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper limit of the projected length of one edge onto another\n"
    "\n"
    "This version is similar to the simple version with one parameter. In addition, it allows "
    "to specify many more options.\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the space check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "Use nil for this value to select the default (Euclidian metrics).\n"
    "\n"
    "\"ignore_angle\" specifies the angle limit of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow to select edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one limit, pass nil to the respective value.\n"
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("inside_check", &inside1,
    "@brief Performs a check whether polygons of this region are inside polygons of the other region by some amount\n"
    "@args other, d\n"
    "@param d The minimum overlap for which the polygons are checked\n"
    "@param other The other region against which to check\n"
    "Returns edge pairs for all locations where edges of polygons of this region are inside polygons of the other region "
    "by less than the given value \"d\". "
    "Contrary to the name, this check does not check whether polygons are inside other polygons but rather checks "
    "whether there is enough overlap of the other polygons vs. polygons of this region. "
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("inside_check", &inside2,
    "@brief Performs an inside check with options\n"
    "@args other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection\n"
    "@param d The minimum distance for which the polygons are checked\n"
    "@param other The other region against which to check\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper limit of the projected length of one edge onto another\n"
    "\n"
    "This version is similar to the simple version with one parameter. In addition, it allows "
    "to specify many more options.\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the width check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "Use nil for this value to select the default (Euclidian metrics).\n"
    "\n"
    "\"ignore_angle\" specifies the angle limit of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow to select edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one limit, pass nil to the respective value.\n"
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("overlap_check", &overlap1,
    "@brief Performs a check whether polygons of this region overlap polygons of the other region by some amount\n"
    "@args other, d\n"
    "@param d The minimum overlap for which the polygons are checked\n"
    "@param other The other region against which to check\n"
    "Returns edge pairs for all locations where edges of polygons of this region overlap polygons of the other region "
    "by less than the given value \"d\". "
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("overlap_check", &overlap2,
    "@brief Performs an overlap check with options\n"
    "@args other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection\n"
    "@param d The minimum overlap for which the polygons are checked\n"
    "@param other The other region against which to check\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper limit of the projected length of one edge onto another\n"
    "\n"
    "This version is similar to the simple version with one parameter. In addition, it allows "
    "to specify many more options.\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the width check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "Use nil for this value to select the default (Euclidian metrics).\n"
    "\n"
    "\"ignore_angle\" specifies the angle limit of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow to select edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one limit, pass nil to the respective value.\n"
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("enclosing_check", &enclosing1,
    "@brief Performs a check whether polygons of this region enclose polygons of the other region by some amount\n"
    "@args other, d\n"
    "@param d The minimum overlap for which the polygons are checked\n"
    "@param other The other region against which to check\n"
    "Returns edge pairs for all locations where edges of polygons of this region are enclosing polygons of the other region "
    "by less than the given value \"d\". "
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("enclosing_check", &enclosing2,
    "@brief Performs an enclosing check with options\n"
    "@args other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection\n"
    "@param d The minimum enclosing distance for which the polygons are checked\n"
    "@param other The other region against which to check\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper limit of the projected length of one edge onto another\n"
    "\n"
    "This version is similar to the simple version with one parameter. In addition, it allows "
    "to specify many more options.\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the width check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "Use nil for this value to select the default (Euclidian metrics).\n"
    "\n"
    "\"ignore_angle\" specifies the angle limit of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow to select edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one limit, pass nil to the respective value.\n"
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("separation_check", &separation1,
    "@brief Performs a check whether polygons of this region are separated from polygons of the other region by some amount\n"
    "@args other, d\n"
    "@param d The minimum separation for which the polygons are checked\n"
    "@param other The other region against which to check\n"
    "Returns edge pairs for all locations where edges of polygons of this region are separated by polygons of the other region "
    "by less than the given value \"d\". "
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("separation_check", &separation2,
    "@brief Performs a separation check with options\n"
    "@args other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection\n"
    "@param d The minimum separation for which the polygons are checked\n"
    "@param other The other region against which to check\n"
    "@param whole_edges If true, deliver the whole edges\n"
    "@param metrics Specify the metrics type\n"
    "@param ignore_angle The angle above which no check is performed\n"
    "@param min_projection The lower threshold of the projected length of one edge onto another\n"
    "@param max_projection The upper limit of the projected length of one edge onto another\n"
    "\n"
    "This version is similar to the simple version with one parameter. In addition, it allows "
    "to specify many more options.\n"
    "\n"
    "If \"whole_edges\" is true, the resulting \\EdgePairs collection will receive the whole "
    "edges which contribute in the width check.\n"
    "\n"
    "\"metrics\" can be one of the constants \\Euclidian, \\Square or \\Projection. See there for "
    "a description of these constants.\n"
    "Use nil for this value to select the default (Euclidian metrics).\n"
    "\n"
    "\"ignore_angle\" specifies the angle limit of two edges. If two edges form an angle equal or "
    "above the given value, they will not contribute in the check. "
    "Setting this value to 90 (the default) will exclude edges with an angle of 90 degree or more from the check.\n"
    "Use nil for this value to select the default.\n"
    "\n"
    "\"min_projection\" and \"max_projection\" allow to select edges by their projected value upon each other. "
    "It is sufficient if the projection of one edge on the other matches the specified condition. "
    "The projected length must be larger or equal to \"min_projection\" and less than \"max_projection\". "
    "If you don't want to specify one limit, pass nil to the respective value.\n"
    "\n"
    "Merged semantics applies for the input of this method (see \\merged_semantics= of merged semantics)\n"
  ) +
  method_ext ("area", &area1,
    "@brief The area of the region\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
    "If merged semantics is not enabled, overlapping areas are counted twice.\n"
  ) +
  method_ext ("area", &area2,
    "@brief The area of the region (restricted to a rectangle)\n"
    "@args rect\n"
    "This version will compute the area of the shapes, restricting the computation to the given rectangle.\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
    "If merged semantics is not enabled, overlapping areas are counted twice.\n"
  ) +
  method_ext ("perimeter", &perimeter1,
    "@brief The total perimeter of the polygons\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
    "If merged semantics is not enabled, internal edges are counted as well.\n"
  ) +
  method_ext ("perimeter", &perimeter2,
    "@brief The total perimeter of the polygons (restricted to a rectangle)\n"
    "@args rect\n"
    "This version will compute the perimeter of the polygons, restricting the computation to the given rectangle.\n"
    "Edges along the border are handled in a special way: they are counted when they are oriented with their inside "
    "side toward the rectangle (in other words: outside edges must coincide with the rectangle's border in order to be counted).\n"
    "\n"
    "Merged semantics applies for this method (see \\merged_semantics= of merged semantics)\n"
    "If merged semantics is not enabled, internal edges are counted as well.\n"
  ) +
  method ("bbox", &db::Region::bbox,
    "@brief Return the bounding box of the region\n"
    "The bounding box is the box enclosing all points of all polygons.\n"
  ) +
  method ("is_merged?", &db::Region::is_merged,
    "@brief Returns true if the region is merged\n"
    "If the region is merged, polygons will not touch or overlap. You can ensure merged state "
    "by calling \\merge.\n"
  ) +
  method ("is_empty?", &db::Region::empty,
    "@brief Returns true if the region is empty\n"
  ) +
  method ("size", (size_t (db::Region::*) () const) &db::Region::size,
    "@brief Returns the number of polygons in the region\n"
    "\n"
    "This returns the number of raw polygons (not merged polygons if merged semantics is enabled).\n"
  ) +
  iterator ("each", &db::Region::begin,
    "@brief Returns each polygon of the region\n"
    "\n"
    "This returns the raw polygons (not merged polygons if merged semantics is enabled).\n"
  ) +
  iterator ("each_merged", &db::Region::begin_merged,
    "@brief Returns each merged polygon of the region\n"
    "\n"
    "This returns the raw polygons if merged semantics is disabled or the merged ones if merged semantics is enabled.\n"
  ) +
  method ("[]", &db::Region::nth,
    "@brief Returns the nth polygon of the region\n"
    "@args n\n"
    "\n"
    "This method returns nil if the index is out of range.\n"
    "This returns the raw polygon (not merged polygons if merged semantics is enabled).\n"
    "\n"
    "Using this method may be costly in terms of memory since it will load the polygons into an array if they have been "
    "stored in an hierarchical layout before. It is recommended to use the \\each iterator instead if possible."
  ) +
  method_ext ("to_s", &to_string0,
    "@brief Converts the region to a string\n"
    "The length of the output is limited to 20 polygons to avoid giant strings on large regions. "
    "For full output use \"to_s\" with a maximum count parameter.\n"
  ) +
  method_ext ("to_s", &to_string1,
    "@brief Converts the region to a string\n"
    "@args max_count\n"
    "This version allows specification of the maximum number of polygons contained in the string."
  ) +
  method ("enable_progress", &db::Region::enable_progress,
    "@brief Enable progress reporting\n"
    "@args label\n"
    "After calling this method, the region will report the progress through a progress bar while "
    "expensive operations are running.\n"
    "The label is a text which is put in front of the progress bar.\n"
    "Using a progress bar will imply a performance penalty of a few percent typically.\n"
  ) +
  method ("disable_progress", &db::Region::disable_progress,
    "@brief Disable progress reporting\n"
    "Calling this method will disable progress reporting. See \\enable_progress.\n"
  ) +
  method ("Euclidian", &euclidian_metrics,
    "@brief Specifies Euclidian metrics for the check functions\n"
    "This value can be used for the metrics parameter in the check functions, i.e. \\width_check. "
    "This value specifies Euclidian metrics, i.e. the distance between two points is measured by:\n"
    "\n"
    "@code\n"
    "d = sqrt(dx^2 + dy^2)\n"
    "@/code\n"
    "\n"
    "All points within a circle with radius d around one point are considered to have a smaller distance than d."
  ) +
  method ("Square", &square_metrics,
    "@brief Specifies square metrics for the check functions\n"
    "This value can be used for the metrics parameter in the check functions, i.e. \\width_check. "
    "This value specifies sqaure metrics, i.e. the distance between two points is measured by:\n"
    "\n"
    "@code\n"
    "d = max(abs(dx), abs(dy))\n"
    "@/code\n"
    "\n"
    "All points within a square with length 2*d around one point are considered to have a smaller distance than d in this metrics."
  ) +
  method ("Projection", &projection_metrics,
    "@brief Specifies projected distance metrics for the check functions\n"
    "This value can be used for the metrics parameter in the check functions, i.e. \\width_check. "
    "This value specifies projected metrics, i.e. the distance is defined as the minimum distance "
    "measured perpendicular to one edge. That implies that the distance is defined only where two "
    "edges have a non-vanishing projection onto each other."
  ),
  "@brief A region (a potentially complex area consisting of multiple polygons)\n"
  "\n\n"
  "This class was introduced to simplify operations on polygon sets like boolean or sizing operations. "
  "Regions consist of many polygons and thus are a generalisation of single polygons which describes "
  "a single coherence set of points. Regions support a variety of operations and have several states. "
  "\n\n"
  "The region's state can be empty (does not contain anything) or box-like, i.e. the region consists "
  "of a single box. In that case, some operations can be simplified. Regions can have merged state. In merged "
  "state, regions consist of merged (non-touching, non-self overlapping) polygons. Each polygon describes "
  "one coherent area in merged state."
  "\n\n"
  "The preferred representation of polygons inside the region are polygons with holes."
  "\n\n"
  "Regions are always expressed in database units. If you want to use regions from different database unit "
  "domains, scale the regions accordingly, i.e. by using the \\transformed method.\n"
  "\n\n"
  "Regions provide convenient operators for the boolean operations. Hence it is often no longer required "
  "to work with the \\EdgeProcessor class. For example:\n"
  "\n"
  "@code\n"
  "r1 = RBA::Region::new(RBA::Box::new(0, 0, 100, 100))\n"
  "r2 = RBA::Region::new(RBA::Box::new(20, 20, 80, 80))\n"
  "# compute the XOR:\n"
  "r1_xor_r2 = r1 ^ r2\n"
  "@/code\n"
  "\n"
  "Regions can be used in two different flavors: in raw mode or merged semantics. With merged semantics (the "
  "default), connected polygons are considered to belong together and are effectively merged.\n"
  "Overlapping areas are counted once in that mode. Internal edges (i.e. arising from cut lines) are not considered.\n"
  "In raw mode (without merged semantics), each polygon is considered as it is. Overlaps between polygons\n"
  "may exists and merging has to be done explicitly using the \\merge method. The semantics can be\n"
  "selected using \\merged_semantics=.\n"
  "\n\n"
  "This class has been introduced in version 0.23.\n"
);

}

