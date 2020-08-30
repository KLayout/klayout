
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


#include "dbAsIfFlatRegion.h"
#include "dbFlatRegion.h"
#include "dbFlatEdgePairs.h"
#include "dbFlatEdges.h"
#include "dbFlatTexts.h"
#include "dbEmptyRegion.h"
#include "dbEmptyEdgePairs.h"
#include "dbEmptyEdges.h"
#include "dbRegion.h"
#include "dbRegionUtils.h"
#include "dbShapeProcessor.h"
#include "dbBoxConvert.h"
#include "dbBoxScanner.h"
#include "dbClip.h"
#include "dbPolygonTools.h"

#include <sstream>

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  AsIfFlagRegion implementation

AsIfFlatRegion::AsIfFlatRegion ()
  : RegionDelegate (), m_bbox_valid (false)
{
  //  .. nothing yet ..
}

AsIfFlatRegion::~AsIfFlatRegion ()
{
  //  .. nothing yet ..
}

AsIfFlatRegion::AsIfFlatRegion (const AsIfFlatRegion &other)
  : RegionDelegate (other), m_bbox_valid (false)
{
  operator= (other);
}

AsIfFlatRegion &
AsIfFlatRegion::operator= (const AsIfFlatRegion &other)
{
  if (this != &other) {
    m_bbox_valid = other.m_bbox_valid;
    m_bbox = other.m_bbox;
  }

  return *this;
}

std::string
AsIfFlatRegion::to_string (size_t nmax) const
{
  std::ostringstream os;
  RegionIterator p (begin ());
  bool first = true;
  for ( ; ! p.at_end () && nmax != 0; ++p, --nmax) {
    if (! first) {
      os << ";";
    }
    first = false;
    os << p->to_string ();
  }
  if (! p.at_end ()) {
    os << "...";
  }
  return os.str ();
}

EdgesDelegate *
AsIfFlatRegion::edges (const EdgeFilterBase *filter) const
{
  std::auto_ptr<FlatEdges> result (new FlatEdges ());

  size_t n = 0;
  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    n += p->vertices ();
  }
  result->reserve (n);

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    for (db::Polygon::polygon_edge_iterator e = p->begin_edge (); ! e.at_end (); ++e) {
      if (! filter || filter->selected (*e)) {
        result->insert (*e);
      }
    }
  }

  return result.release ();
}

RegionDelegate *
AsIfFlatRegion::in (const Region &other, bool invert) const
{
  std::set <db::Polygon> op;
  for (RegionIterator o (other.begin_merged ()); ! o.at_end (); ++o) {
    op.insert (*o);
  }

  std::auto_ptr<FlatRegion> new_region (new FlatRegion (false));

  for (RegionIterator o (begin_merged ()); ! o.at_end (); ++o) {
    if ((op.find (*o) == op.end ()) == invert) {
      new_region->insert (*o);
    }
  }

  return new_region.release ();
}

size_t
AsIfFlatRegion::size () const
{
  size_t n = 0;
  for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
    ++n;
  }
  return n;
}

bool
AsIfFlatRegion::is_box () const
{
  RegionIterator p (begin ());
  if (p.at_end ()) {
    return false;
  } else {
    const db::Polygon &poly = *p;
    ++p;
    if (! p.at_end ()) {
      return false;
    } else {
      return poly.is_box ();
    }
  }
}

AsIfFlatRegion::area_type
AsIfFlatRegion::area (const db::Box &box) const
{
  area_type a = 0;

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    if (box.empty () || p->box ().inside (box)) {
      a += p->area ();
    } else {
      std::vector<db::Polygon> clipped;
      clip_poly (*p, box, clipped);
      for (std::vector<db::Polygon>::const_iterator c = clipped.begin (); c != clipped.end (); ++c) {
        a += c->area ();
      }
    }
  }

  return a;
}

AsIfFlatRegion::perimeter_type
AsIfFlatRegion::perimeter (const db::Box &box) const
{
  perimeter_type d = 0;

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {

    if (box.empty () || p->box ().inside (box)) {
      d += p->perimeter ();
    } else {

      for (db::Polygon::polygon_edge_iterator e = p->begin_edge (); ! e.at_end (); ++e) {

        if (box.empty ()) {
          d += (*e).length ();
        } else {

          std::pair<bool, db::Edge> ce = (*e).clipped (box);
          if (ce.first) {

            db::Coord dx = ce.second.dx ();
            db::Coord dy = ce.second.dy ();
            db::Coord x = ce.second.p1 ().x ();
            db::Coord y = ce.second.p1 ().y ();
            if ((dx == 0 && x == box.left ()   && dy < 0) ||
                (dx == 0 && x == box.right ()  && dy > 0) ||
                (dy == 0 && y == box.top ()    && dx < 0) ||
                (dy == 0 && y == box.bottom () && dx > 0)) {
              //  not counted -> box is at outside side of the edge
            } else {
              d += ce.second.length ();
            }

          }

        }

      }

    }

  }

  return d;
}

Box AsIfFlatRegion::bbox () const
{
  if (! m_bbox_valid) {
    m_bbox = compute_bbox ();
    m_bbox_valid = true;
  }
  return m_bbox;
}

Box AsIfFlatRegion::compute_bbox () const
{
  db::Box b;
  for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
    b += p->box ();
  }
  return b;
}

void AsIfFlatRegion::update_bbox (const db::Box &b)
{
  m_bbox = b;
  m_bbox_valid = true;
}

void AsIfFlatRegion::invalidate_bbox ()
{
  m_bbox_valid = false;
}

RegionDelegate *
AsIfFlatRegion::filtered (const PolygonFilterBase &filter) const
{
  std::auto_ptr<FlatRegion> new_region (new FlatRegion ());

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    if (filter.selected (*p)) {
      new_region->insert (*p);
    }
  }

  return new_region.release ();
}

RegionDelegate *
AsIfFlatRegion::processed (const PolygonProcessorBase &filter) const
{
  std::auto_ptr<FlatRegion> new_region (new FlatRegion ());
  if (filter.result_must_not_be_merged ()) {
    new_region->set_merged_semantics (false);
  }

  std::vector<db::Polygon> poly_res;

  for (RegionIterator p (filter.requires_raw_input () ? begin () : begin_merged ()); ! p.at_end (); ++p) {

    poly_res.clear ();
    filter.process (*p, poly_res);
    for (std::vector<db::Polygon>::const_iterator pr = poly_res.begin (); pr != poly_res.end (); ++pr) {
      new_region->insert (*pr);
    }

  }

  return new_region.release ();
}

EdgesDelegate *
AsIfFlatRegion::processed_to_edges (const PolygonToEdgeProcessorBase &filter) const
{
  std::auto_ptr<FlatEdges> new_edges (new FlatEdges ());
  if (filter.result_must_not_be_merged ()) {
    new_edges->set_merged_semantics (false);
  }

  std::vector<db::Edge> edge_res;

  for (RegionIterator p (filter.requires_raw_input () ? begin () : begin_merged ()); ! p.at_end (); ++p) {

    edge_res.clear ();
    filter.process (*p, edge_res);
    for (std::vector<db::Edge>::const_iterator er = edge_res.begin (); er != edge_res.end (); ++er) {
      new_edges->insert (*er);
    }

  }

  return new_edges.release ();
}

EdgePairsDelegate *
AsIfFlatRegion::processed_to_edge_pairs (const PolygonToEdgePairProcessorBase &filter) const
{
  std::auto_ptr<FlatEdgePairs> new_edge_pairs (new FlatEdgePairs ());
  if (filter.result_must_not_be_merged ()) {
    new_edge_pairs->set_merged_semantics (false);
  }

  std::vector<db::EdgePair> edge_pair_res;

  for (RegionIterator p (filter.requires_raw_input () ? begin () : begin_merged ()); ! p.at_end (); ++p) {

    edge_pair_res.clear ();
    filter.process (*p, edge_pair_res);
    for (std::vector<db::EdgePair>::const_iterator epr = edge_pair_res.begin (); epr != edge_pair_res.end (); ++epr) {
      new_edge_pairs->insert (*epr);
    }

  }

  return new_edge_pairs.release ();
}

RegionDelegate *
AsIfFlatRegion::selected_interacting_generic (const Edges &other, bool inverse) const
{
  if (other.empty ()) {
    if (! inverse) {
      return new EmptyRegion ();
    } else {
      return clone ();
    }
  } else if (empty ()) {
    return clone ();
  }

  db::box_scanner2<db::Polygon, size_t, db::Edge, size_t> scanner (report_progress (), progress_desc ());
  scanner.reserve1 (size ());
  scanner.reserve2 (other.size ());

  std::auto_ptr<FlatRegion> output (new FlatRegion (false));
  region_to_edge_interaction_filter<Shapes, db::Polygon> filter (output->raw_polygons (), inverse);

  AddressablePolygonDelivery p (begin_merged (), has_valid_merged_polygons ());

  for ( ; ! p.at_end (); ++p) {
    scanner.insert1 (p.operator-> (), 0);
    if (inverse) {
      filter.preset (p.operator-> ());
    }
  }

  AddressableEdgeDelivery e (other.addressable_edges ());

  for ( ; ! e.at_end (); ++e) {
    scanner.insert2 (e.operator-> (), 0);
  }

  scanner.process (filter, 1, db::box_convert<db::Polygon> (), db::box_convert<db::Edge> ());

  if (inverse) {
    filter.fill_output ();
  }

  return output.release ();
}

RegionDelegate *
AsIfFlatRegion::selected_interacting_generic (const Texts &other, bool inverse) const
{
  if (other.empty ()) {
    if (! inverse) {
      return new EmptyRegion ();
    } else {
      return clone ();
    }
  } else if (empty ()) {
    return clone ();
  }

  db::box_scanner2<db::Polygon, size_t, db::Text, size_t> scanner (report_progress (), progress_desc ());
  scanner.reserve1 (size ());
  scanner.reserve2 (other.size ());

  std::auto_ptr<FlatRegion> output (new FlatRegion (true));
  region_to_text_interaction_filter<FlatRegion, db::Text> filter (*output, inverse);

  AddressablePolygonDelivery p (begin_merged (), has_valid_merged_polygons ());

  for ( ; ! p.at_end (); ++p) {
    scanner.insert1 (p.operator-> (), 0);
    if (inverse) {
      filter.preset (p.operator-> ());
    }
  }

  AddressableTextDelivery e (other.addressable_texts ());

  for ( ; ! e.at_end (); ++e) {
    scanner.insert2 (e.operator-> (), 0);
  }

  scanner.process (filter, 1, db::box_convert<db::Polygon> (), db::box_convert<db::Text> ());

  if (inverse) {
    filter.fill_output ();
  }

  return output.release ();
}

RegionDelegate *
AsIfFlatRegion::selected_interacting_generic (const Region &other, int mode, bool touching, bool inverse) const
{
  db::EdgeProcessor ep (report_progress (), progress_desc ());
  ep.set_base_verbosity (base_verbosity ());

  //  shortcut
  if (empty ()) {
    return clone ();
  } else if (other.empty ()) {
    //  clear, if b is empty and
    //   * mode is inside or interacting and inverse is false ("inside" or "interacting")
    //   * mode is outside and inverse is true ("not outside")
    if ((mode <= 0) != inverse) {
      return new EmptyRegion ();
    } else {
      return clone ();
    }
  }

  for (RegionIterator p = other.begin (); ! p.at_end (); ++p) {
    if (p->box ().touches (bbox ())) {
      ep.insert (*p, 0);
    }
  }

  size_t n = 1;
  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p, ++n) {
    if (mode > 0 || p->box ().touches (other.bbox ())) {
      ep.insert (*p, n);
    }
  }

  db::InteractionDetector id (mode, 0);
  id.set_include_touching (touching);
  db::EdgeSink es;
  ep.process (es, id);
  id.finish ();

  std::auto_ptr<FlatRegion> output (new FlatRegion (false));

  n = 0;
  std::set <size_t> selected;
  for (db::InteractionDetector::iterator i = id.begin (); i != id.end () && i->first == 0; ++i) {
    ++n;
    selected.insert (i->second);
  }

  output->reserve (n);

  n = 1;
  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p, ++n) {
    if ((selected.find (n) == selected.end ()) == inverse) {
      output->raw_polygons ().insert (*p);
    }
  }

  return output.release ();
}

EdgesDelegate *
AsIfFlatRegion::pull_generic (const Edges &other) const
{
  if (other.empty ()) {
    return other.delegate ()->clone ();
  } else if (empty ()) {
    return new EmptyEdges ();
  }

  db::box_scanner2<db::Polygon, size_t, db::Edge, size_t> scanner (report_progress (), progress_desc ());
  scanner.reserve1 (size ());
  scanner.reserve2 (other.size ());

  std::auto_ptr<FlatEdges> output (new FlatEdges (false));
  region_to_edge_interaction_filter<Shapes, db::Edge> filter (output->raw_edges (), false);

  AddressablePolygonDelivery p (begin (), has_valid_polygons ());

  for ( ; ! p.at_end (); ++p) {
    scanner.insert1 (p.operator-> (), 0);
  }

  AddressableEdgeDelivery e (other.addressable_merged_edges ());

  for ( ; ! e.at_end (); ++e) {
    scanner.insert2 (e.operator-> (), 0);
  }

  scanner.process (filter, 1, db::box_convert<db::Polygon> (), db::box_convert<db::Edge> ());

  return output.release ();
}

TextsDelegate *
AsIfFlatRegion::pull_generic (const Texts &other) const
{
  if (other.empty ()) {
    return other.delegate ()->clone ();
  } else if (empty ()) {
    return new EmptyTexts ();
  }

  db::box_scanner2<db::Polygon, size_t, db::Text, size_t> scanner (report_progress (), progress_desc ());
  scanner.reserve1 (size ());
  scanner.reserve2 (other.size ());

  std::auto_ptr<FlatTexts> output (new FlatTexts (false));
  region_to_text_interaction_filter<Shapes, db::Text, db::Text> filter (output->raw_texts (), false);

  AddressablePolygonDelivery p (begin (), has_valid_polygons ());

  for ( ; ! p.at_end (); ++p) {
    scanner.insert1 (p.operator-> (), 0);
  }

  AddressableTextDelivery e (other.addressable_texts ());

  for ( ; ! e.at_end (); ++e) {
    scanner.insert2 (e.operator-> (), 0);
  }

  scanner.process (filter, 1, db::box_convert<db::Polygon> (), db::box_convert<db::Text> ());

  return output.release ();
}

RegionDelegate *
AsIfFlatRegion::pull_generic (const Region &other, int mode, bool touching) const
{
  db::EdgeProcessor ep (report_progress (), progress_desc ());
  ep.set_base_verbosity (base_verbosity ());

  //  shortcut
  if (empty ()) {
    return clone ();
  } else if (other.empty ()) {
    return new EmptyRegion ();
  }

  size_t n = 1;
  for (RegionIterator p = other.begin_merged (); ! p.at_end (); ++p, ++n) {
    if (p->box ().touches (bbox ())) {
      ep.insert (*p, n);
    }
  }

  for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
    if (mode > 0 || p->box ().touches (other.bbox ())) {
      ep.insert (*p, 0);
    }
  }

  db::InteractionDetector id (mode, 0);
  id.set_include_touching (touching);
  db::EdgeSink es;
  ep.process (es, id);
  id.finish ();

  std::auto_ptr<FlatRegion> output (new FlatRegion (false));

  n = 0;
  std::set <size_t> selected;
  for (db::InteractionDetector::iterator i = id.begin (); i != id.end () && i->first == 0; ++i) {
    ++n;
    selected.insert (i->second);
  }

  output->reserve (n);

  n = 1;
  for (RegionIterator p = other.begin_merged (); ! p.at_end (); ++p, ++n) {
    if (selected.find (n) != selected.end ()) {
      output->raw_polygons ().insert (*p);
    }
  }

  return output.release ();
}

template <class Trans>
void
AsIfFlatRegion::produce_markers_for_grid_check (const db::Polygon &poly, const Trans &tr, db::Coord gx, db::Coord gy, db::Shapes &shapes)
{
  gx = std::max (db::Coord (1), gx);
  gy = std::max (db::Coord (1), gy);

  for (size_t i = 0; i < poly.holes () + 1; ++i) {

    db::Polygon::polygon_contour_iterator b, e;

    if (i == 0) {
      b = poly.begin_hull ();
      e = poly.end_hull ();
    } else {
      b = poly.begin_hole ((unsigned int) (i - 1));
      e = poly.end_hole ((unsigned int)  (i - 1));
    }

    for (db::Polygon::polygon_contour_iterator pt = b; pt != e; ++pt) {
      db::Point p = tr * *pt;
      if ((p.x () % gx) != 0 || (p.y () % gy) != 0) {
        shapes.insert (EdgePair (db::Edge (p, p), db::Edge (p, p)));
      }
    }

  }
}

template void AsIfFlatRegion::produce_markers_for_grid_check<db::ICplxTrans> (const db::Polygon &poly, const db::ICplxTrans &tr, db::Coord gx, db::Coord gy, db::Shapes &shapes);
template void AsIfFlatRegion::produce_markers_for_grid_check<db::UnitTrans> (const db::Polygon &poly, const db::UnitTrans &tr, db::Coord gx, db::Coord gy, db::Shapes &shapes);

EdgePairsDelegate *
AsIfFlatRegion::grid_check (db::Coord gx, db::Coord gy) const
{
  if (gx < 0 || gy < 0) {
    throw tl::Exception (tl::to_string (tr ("Grid check requires a positive grid value")));
  }

  if (gx == 0 && gy == 0) {
    return new EmptyEdgePairs ();
  }

  std::auto_ptr<db::FlatEdgePairs> res (new db::FlatEdgePairs ());

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    produce_markers_for_grid_check (*p, db::UnitTrans (), gx, gy, res->raw_edge_pairs ());
  }

  return res.release ();
}

static bool ac_less (double cos_a, bool gt180_a, double cos_b, bool gt180_b)
{
  if (gt180_a != gt180_b) {
    return gt180_a < gt180_b;
  } else {
    if (gt180_a) {
      return cos_a < cos_b - 1e-10;
    } else {
      return cos_a > cos_b + 1e-10;
    }
  }
}

template <class Trans>
void
AsIfFlatRegion::produce_markers_for_angle_check (const db::Polygon &poly, const Trans &tr, double min, double max, bool inverse, db::Shapes &shapes)
{
  double cos_min = cos (std::max (0.0, std::min (360.0, min)) / 180.0 * M_PI);
  double cos_max = cos (std::max (0.0, std::min (360.0, max)) / 180.0 * M_PI);
  bool gt180_min = min > 180.0;
  bool gt180_max = max > 180.0;

  for (size_t i = 0; i < poly.holes () + 1; ++i) {

    const db::Polygon::contour_type *h = 0;
    if (i == 0) {
      h = &poly.hull ();
    } else {
      h = &poly.hole ((unsigned int) (i - 1));
    }

    size_t np = h->size ();

    for (size_t j = 0; j < np; ++j) {

      db::Edge e ((*h) [j], (*h) [(j + 1) % np]);
      e.transform (tr);
      db::Edge ee (e.p2 (), (*h) [(j + 2) % np]);
      ee.transform (tr);

      double le = e.double_length ();
      double lee = ee.double_length ();

      double cos_a = -db::sprod (e, ee) / (le * lee);
      bool gt180_a = db::vprod_sign (e, ee) > 0;

      if ((ac_less (cos_a, gt180_a, cos_max, gt180_max) && !ac_less (cos_a, gt180_a, cos_min, gt180_min)) == !inverse) {
        shapes.insert (EdgePair (e, ee));
      }

    }

  }
}

template void AsIfFlatRegion::produce_markers_for_angle_check<db::ICplxTrans> (const db::Polygon &poly, const db::ICplxTrans &tr, double min, double max, bool inverse, db::Shapes &shapes);
template void AsIfFlatRegion::produce_markers_for_angle_check<db::UnitTrans> (const db::Polygon &poly, const db::UnitTrans &tr, double min, double max, bool inverse, db::Shapes &shapes);

EdgePairsDelegate *
AsIfFlatRegion::angle_check (double min, double max, bool inverse) const
{
  std::auto_ptr<db::FlatEdgePairs> res (new db::FlatEdgePairs ());

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    produce_markers_for_angle_check (*p, db::UnitTrans (), min, max, inverse, res->raw_edge_pairs ());
  }

  return res.release ();
}

RegionDelegate *
AsIfFlatRegion::snapped (db::Coord gx, db::Coord gy)
{
  if (gx < 0 || gy < 0) {
    throw tl::Exception (tl::to_string (tr ("Grid snap requires a positive grid value")));
  }

  std::auto_ptr<FlatRegion> new_region (new FlatRegion (merged_semantics ()));

  gx = std::max (db::Coord (1), gx);
  gy = std::max (db::Coord (1), gy);

  std::vector<db::Point> heap;

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    new_region->raw_polygons ().insert (snapped_polygon (*p, gx, gy, heap));
  }

  return new_region.release ();
}

RegionDelegate *
AsIfFlatRegion::scaled_and_snapped (db::Coord gx, db::Coord mx, db::Coord dx, db::Coord gy, db::Coord my, db::Coord dy)
{
  if (gx < 0 || gy < 0) {
    throw tl::Exception (tl::to_string (tr ("Grid snap requires a positive grid value")));
  }

  if (mx <= 0 || dx <= 0 || my <= 0 || dy <= 0) {
    throw tl::Exception (tl::to_string (tr ("Scale and snap requires positive and non-null magnification or divisor values")));
  }

  std::auto_ptr<FlatRegion> new_region (new FlatRegion (merged_semantics ()));

  gx = std::max (db::Coord (1), gx);
  gy = std::max (db::Coord (1), gy);

  std::vector<db::Point> heap;

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    new_region->raw_polygons ().insert (scaled_and_snapped_polygon (*p, gx, mx, dx, 0, gy, my, dy, 0, heap));
  }

  return new_region.release ();
}

EdgePairsDelegate *
AsIfFlatRegion::run_check (db::edge_relation_type rel, bool different_polygons, const Region *other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
{
  std::auto_ptr<FlatEdgePairs> result (new FlatEdgePairs ());

  db::box_scanner<db::Polygon, size_t> scanner (report_progress (), progress_desc ());
  scanner.reserve (size () + (other ? other->size () : 0));

  AddressablePolygonDelivery p (begin_merged (), has_valid_merged_polygons ());

  size_t n = 0;
  for ( ; ! p.at_end (); ++p) {
    scanner.insert (p.operator-> (), n);
    n += 2;
  }

  AddressablePolygonDelivery po;

  if (other) {

    po = other->addressable_merged_polygons ();

    n = 1;
    for ( ; ! po.at_end (); ++po) {
      scanner.insert (po.operator-> (), n);
      n += 2;
    }

  }

  EdgeRelationFilter check (rel, d, metrics);
  check.set_include_zero (false);
  check.set_whole_edges (whole_edges);
  check.set_ignore_angle (ignore_angle);
  check.set_min_projection (min_projection);
  check.set_max_projection (max_projection);

  edge2edge_check<db::FlatEdgePairs> edge_check (check, *result, different_polygons, other != 0);
  poly2poly_check<db::FlatEdgePairs> poly_check (edge_check);

  do {
    scanner.process (poly_check, d, db::box_convert<db::Polygon> ());
  } while (edge_check.prepare_next_pass ());

  return result.release ();
}

EdgePairsDelegate *
AsIfFlatRegion::run_single_polygon_check (db::edge_relation_type rel, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
{
  std::auto_ptr<FlatEdgePairs> result (new FlatEdgePairs ());

  EdgeRelationFilter check (rel, d, metrics);
  check.set_include_zero (false);
  check.set_whole_edges (whole_edges);
  check.set_ignore_angle (ignore_angle);
  check.set_min_projection (min_projection);
  check.set_max_projection (max_projection);

  edge2edge_check<db::FlatEdgePairs> edge_check (check, *result, false, false);
  poly2poly_check<db::FlatEdgePairs> poly_check (edge_check);

  do {

    size_t n = 0;
    for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
      poly_check.enter (*p, n);
      n += 2;
    }

  } while (edge_check.prepare_next_pass ());

  return result.release ();
}

RegionDelegate *
AsIfFlatRegion::merged (bool min_coherence, unsigned int min_wc) const
{
  if (empty ()) {

    return new EmptyRegion ();

  } else if (is_box ()) {

    //  take box only if min_wc == 0, otherwise clear
    if (min_wc > 0) {
      return new EmptyRegion ();
    } else {
      return clone ();
    }

  } else {

    db::EdgeProcessor ep (report_progress (), progress_desc ());
    ep.set_base_verbosity (base_verbosity ());

    //  count edges and reserve memory
    size_t n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, ++n) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, ++n) {
      ep.insert (*p, n);
    }

    std::auto_ptr<FlatRegion> new_region (new FlatRegion (true));

    //  and run the merge step
    db::MergeOp op (min_wc);
    db::ShapeGenerator pc (new_region->raw_polygons (), true /*clear*/);
    db::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence);
    ep.process (pg, op);

    return new_region.release ();

  }
}

RegionDelegate *
AsIfFlatRegion::region_from_box (const db::Box &b)
{
  if (! b.empty () && b.width () > 0 && b.height () > 0) {
    FlatRegion *new_region = new FlatRegion ();
    new_region->insert (b);
    return new_region;
  } else {
    return new EmptyRegion ();
  }
}

RegionDelegate *
AsIfFlatRegion::sized (coord_type d, unsigned int mode) const
{
  return sized (d, d, mode);
}

RegionDelegate *
AsIfFlatRegion::sized (coord_type dx, coord_type dy, unsigned int mode) const
{
  if (empty ()) {

    //  ignore empty
    return new EmptyRegion ();

  } else if (is_box () && mode >= 2) {

    //  simplified handling for a box
    db::Box b = bbox ().enlarged (db::Vector (dx, dy));
    return region_from_box (b);

  } else if (! merged_semantics () || is_merged ()) {

    //  Generic case
    std::auto_ptr<FlatRegion> new_region (new FlatRegion (false /*output isn't merged*/));

    db::ShapeGenerator pc (new_region->raw_polygons (), false);
    db::PolygonGenerator pg (pc, false, true);
    db::SizingPolygonFilter sf (pg, dx, dy, mode);
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      sf.put (*p);
    }

    return new_region.release ();

  } else {

    //  Generic case - the size operation will merge first
    db::EdgeProcessor ep (report_progress (), progress_desc ());
    ep.set_base_verbosity (base_verbosity ());

    //  count edges and reserve memory
    size_t n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, ++n) {
      ep.insert (*p, n);
    }

    std::auto_ptr<FlatRegion> new_region (new FlatRegion (false /*output isn't merged*/));
    db::ShapeGenerator pc (new_region->raw_polygons (), true /*clear*/);
    db::PolygonGenerator pg2 (pc, false /*don't resolve holes*/, true /*min. coherence*/);
    db::SizingPolygonFilter siz (pg2, dx, dy, mode);
    db::PolygonGenerator pg (siz, false /*don't resolve holes*/, min_coherence () /*min. coherence*/);
    db::BooleanOp op (db::BooleanOp::Or);
    ep.process (pg, op);

    return new_region.release ();

  }
}

RegionDelegate *
AsIfFlatRegion::and_with (const Region &other) const
{
  if (empty () || other.empty ()) {

    //  Nothing to do
    return new EmptyRegion ();

  } else if (is_box () && other.is_box ()) {

    //  Simplified handling for boxes
    db::Box b = bbox ();
    b &= other.bbox ();
    return region_from_box (b);

  } else if (is_box () && ! other.strict_handling ()) {

    //  map AND with box to clip ..
    db::Box b = bbox ();
    std::auto_ptr<FlatRegion> new_region (new FlatRegion (false));

    std::vector<db::Polygon> clipped;
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      clipped.clear ();
      clip_poly (*p, b, clipped);
      new_region->raw_polygons ().insert (clipped.begin (), clipped.end ());
    }

    return new_region.release ();

  } else if (other.is_box () && ! strict_handling ()) {

    //  map AND with box to clip ..
    db::Box b = other.bbox ();
    std::auto_ptr<FlatRegion> new_region (new FlatRegion (false));

    std::vector<db::Polygon> clipped;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      clipped.clear ();
      clip_poly (*p, b, clipped);
      new_region->raw_polygons ().insert (clipped.begin (), clipped.end ());
    }

    return new_region.release ();

  } else if (! bbox ().overlaps (other.bbox ())) {

    //  Result will be nothing
    return new EmptyRegion ();

  } else {

    //  Generic case
    db::EdgeProcessor ep (report_progress (), progress_desc ());
    ep.set_base_verbosity (base_verbosity ());

    //  count edges and reserve memory
    size_t n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }
    n = 1;
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }

    std::auto_ptr<FlatRegion> new_region (new FlatRegion (true));
    db::BooleanOp op (db::BooleanOp::And);
    db::ShapeGenerator pc (new_region->raw_polygons (), true /*clear*/);
    db::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence ());
    ep.process (pg, op);

    return new_region.release ();

  }
}

RegionDelegate *
AsIfFlatRegion::not_with (const Region &other) const
{
  if (empty ()) {

    //  Nothing to do
    return new EmptyRegion ();

  } else if (other.empty () && ! strict_handling ()) {

    //  Nothing to do
    return clone ();

  } else if (! bbox ().overlaps (other.bbox ()) && ! strict_handling ()) {

    //  Nothing to do
    return clone ();

  } else {

    //  Generic case
    db::EdgeProcessor ep (report_progress (), progress_desc ());
    ep.set_base_verbosity (base_verbosity ());

    //  count edges and reserve memory
    size_t n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }
    n = 1;
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }

    std::auto_ptr<FlatRegion> new_region (new FlatRegion (true));
    db::BooleanOp op (db::BooleanOp::ANotB);
    db::ShapeGenerator pc (new_region->raw_polygons (), true /*clear*/);
    db::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence ());
    ep.process (pg, op);

    return new_region.release ();

  }
}

RegionDelegate *
AsIfFlatRegion::xor_with (const Region &other) const
{
  if (empty () && ! other.strict_handling ()) {

    return other.delegate ()->clone ();

  } else if (other.empty () && ! strict_handling ()) {

    return clone ();

  } else if (! bbox ().overlaps (other.bbox ()) && ! strict_handling () && ! other.strict_handling ()) {

    //  Simplified handling for disjunct case
    return or_with (other);

  } else {

    //  Generic case
    db::EdgeProcessor ep (report_progress (), progress_desc ());
    ep.set_base_verbosity (base_verbosity ());

    //  count edges and reserve memory
    size_t n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }
    n = 1;
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }

    std::auto_ptr<FlatRegion> new_region (new FlatRegion (true));
    db::BooleanOp op (db::BooleanOp::Xor);
    db::ShapeGenerator pc (new_region->raw_polygons (), true /*clear*/);
    db::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence ());
    ep.process (pg, op);

    return new_region.release ();

  }
}

RegionDelegate *
AsIfFlatRegion::or_with (const Region &other) const
{
  if (empty () && ! other.strict_handling ()) {

    return other.delegate ()->clone ();

  } else if (other.empty () && ! strict_handling ()) {

    //  Nothing to do
    return clone ();

  } else if (! bbox ().overlaps (other.bbox ()) && ! strict_handling () && ! other.strict_handling ()) {

    //  Simplified handling for disjunct case
    return add (other);

  } else {

    //  Generic case
    db::EdgeProcessor ep (report_progress (), progress_desc ());
    ep.set_base_verbosity (base_verbosity ());

    //  count edges and reserve memory
    size_t n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }
    n = 1;
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }

    std::auto_ptr<FlatRegion> new_region (new FlatRegion (true));
    db::BooleanOp op (db::BooleanOp::Or);
    db::ShapeGenerator pc (new_region->raw_polygons (), true /*clear*/);
    db::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence ());
    ep.process (pg, op);

    return new_region.release ();

  }
}

RegionDelegate *
AsIfFlatRegion::add (const Region &other) const
{
  FlatRegion *other_flat = dynamic_cast<FlatRegion *> (other.delegate ());
  if (other_flat) {

    std::auto_ptr<FlatRegion> new_region (new FlatRegion (*other_flat));
    new_region->set_is_merged (false);
    new_region->invalidate_cache ();

    size_t n = new_region->raw_polygons ().size () + size ();

    new_region->reserve (n);

    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      new_region->raw_polygons ().insert (*p);
    }

    return new_region.release ();

  } else {

    std::auto_ptr<FlatRegion> new_region (new FlatRegion (false /*not merged*/));

    size_t n = size () + other.size ();

    new_region->reserve (n);

    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      new_region->raw_polygons ().insert (*p);
    }
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      new_region->raw_polygons ().insert (*p);
    }

    return new_region.release ();

  }
}

void
AsIfFlatRegion::insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  //  improves performance when inserting an original layout into the same layout
  db::LayoutLocker locker (layout);

  db::Shapes &shapes = layout->cell (into_cell).shapes (into_layer);
  for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
    shapes.insert (*p);
  }
}

bool
AsIfFlatRegion::equals (const Region &other) const
{
  if (empty () != other.empty ()) {
    return false;
  }
  if (size () != other.size ()) {
    return false;
  }
  RegionIterator o1 (begin ());
  RegionIterator o2 (other.begin ());
  while (! o1.at_end () && ! o2.at_end ()) {
    if (*o1 != *o2) {
      return false;
    }
    ++o1;
    ++o2;
  }
  return true;
}

bool
AsIfFlatRegion::less (const Region &other) const
{
  if (empty () != other.empty ()) {
    return empty () < other.empty ();
  }
  if (size () != other.size ()) {
    return (size () < other.size ());
  }
  RegionIterator o1 (begin ());
  RegionIterator o2 (other.begin ());
  while (! o1.at_end () && ! o2.at_end ()) {
    if (*o1 != *o2) {
      return *o1 < *o2;
    }
    ++o1;
    ++o2;
  }
  return false;
}

}

