
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


#include "dbRegion.h"
#include "dbLayoutUtils.h"
#include "dbEdgeProcessor.h"
#include "dbEdgePairRelations.h"
#include "dbEdges.h"
#include "dbEdgePairs.h"
#include "dbBoxConvert.h"
#include "dbBoxScanner.h"
#include "dbClip.h"
#include "dbPolygonTools.h"

#include "tlVariant.h"

#include <sstream>
#include <set>

namespace db
{

Region::Region (const RecursiveShapeIterator &si)
  : m_polygons (false), m_merged_polygons (false), m_iter (si)
{
  init ();
  //  Make sure we restart the iterator and late-initialize it (this makes sure
  //  it refers to the configuration present then)
  m_iter.reset ();
  m_bbox_valid = false;
  m_is_merged = false;
}

Region::Region (const RecursiveShapeIterator &si, const db::ICplxTrans &trans, bool merged_semantics)
  : m_polygons (false), m_merged_polygons (false), m_iter (si), m_iter_trans (trans)
{
  init ();
  //  Make sure we restart the iterator and late-initialize it (this makes sure
  //  it refers to the configuration present then)
  m_iter.reset ();
  m_bbox_valid = false;
  m_is_merged = false;
  m_merged_semantics = merged_semantics;
}

bool  
Region::operator== (const db::Region &other) const
{
  if (empty () != other.empty ()) {
    return false;
  }
  if (size () != other.size ()) {
    return false;
  }
  db::Region::const_iterator o1 = begin ();
  db::Region::const_iterator o2 = other.begin ();
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
Region::operator< (const db::Region &other) const
{
  if (empty () != other.empty ()) {
    return empty () < other.empty ();
  }
  if (size () != other.size ()) {
    return (size () < other.size ());
  }
  db::Region::const_iterator o1 = begin ();
  db::Region::const_iterator o2 = other.begin ();
  while (! o1.at_end () && ! o2.at_end ()) {
    if (*o1 != *o2) {
      return *o1 < *o2;
    }
    ++o1;
    ++o2;
  }
  return false;
}

size_t 
Region::size () const
{
  if (! has_valid_polygons ()) {
    //  If we have an iterator, we have to do it the hard way ..
    size_t n = 0;
    for (const_iterator p = begin (); ! p.at_end (); ++p) {
      ++n;
    }
    return n;
  } else {
    return m_polygons.size ();
  }
}

void 
Region::set_strict_handling (bool f)
{
  m_strict_handling = f;
}

void 
Region::set_merged_semantics (bool f)
{
  if (f != m_merged_semantics) {
    m_merged_semantics = f;
    m_merged_polygons.clear ();
    m_merged_polygons_valid = false;
  }
}

std::string 
Region::to_string (size_t nmax) const
{
  std::ostringstream os;
  const_iterator p = begin ();
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

Region::area_type 
Region::area (const db::Box &box) const
{
  area_type a = 0;

  for (const_iterator p = begin_merged (); ! p.at_end (); ++p) {
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

Region::perimeter_type 
Region::perimeter (const db::Box &box) const
{
  perimeter_type d = 0;

  for (const_iterator p = begin_merged (); ! p.at_end (); ++p) {

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

Region 
Region::hulls () const
{
  Region region;

  for (const_iterator p = begin_merged (); ! p.at_end (); ++p) {
    db::Polygon h;
    h.assign_hull (p->begin_hull (), p->end_hull ());
    region.insert (h);
  }

  return region;
}

Region 
Region::holes () const
{
  Region region;

  for (const_iterator p = begin_merged (); ! p.at_end (); ++p) {
    for (size_t i = 0; i < p->holes (); ++i) {
      db::Polygon h;
      h.assign_hull (p->begin_hole (i), p->end_hole (i));
      region.insert (h);
    }
  }

  return region;
}

Region
Region::rounded_corners (double rinner, double router, unsigned int n) const
{
  Region r;
  for (const_iterator p = begin_merged (); ! p.at_end (); ++p) {
    r.insert (db::compute_rounded (*p, rinner, router, n));
  }
  return r;
}

Region
Region::smoothed (coord_type d) const
{
  Region r;
  for (const_iterator p = begin_merged (); ! p.at_end (); ++p) {
    r.insert (db::smooth (*p, d));
  }
  return r;
}

Region 
Region::in (const Region &other, bool invert) const
{
  std::set <db::Polygon> op;
  for (const_iterator o = other.begin_merged (); ! o.at_end (); ++o) {
    op.insert (*o);
  }

  Region r;
  for (const_iterator o = begin_merged (); ! o.at_end (); ++o) {
    if ((op.find (*o) == op.end ()) == invert) {
      r.insert (*o);
    }
  }

  return r;
}

Edges 
Region::edges () const
{
  Edges edges;

  size_t n = 0;
  for (const_iterator p = begin_merged (); ! p.at_end (); ++p) {
    n += p->vertices ();
  }
  edges.reserve (n);

  for (const_iterator p = begin_merged (); ! p.at_end (); ++p) {
    for (db::Polygon::polygon_edge_iterator e = p->begin_edge (); ! e.at_end (); ++e) {
      edges.insert (*e);
    }
  }

  return edges;
}

bool 
Region::is_box () const
{
  const_iterator p = begin ();
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

void 
Region::swap (Region &other)
{
  std::swap (m_is_merged, other.m_is_merged);
  std::swap (m_merged_semantics, other.m_merged_semantics);
  std::swap (m_strict_handling, other.m_strict_handling);
  std::swap (m_merge_min_coherence, other.m_merge_min_coherence);
  m_polygons.swap (other.m_polygons);
  m_merged_polygons.swap (other.m_merged_polygons);
  std::swap (m_bbox, other.m_bbox);
  std::swap (m_bbox_valid, other.m_bbox_valid);
  std::swap (m_merged_polygons_valid, other.m_merged_polygons_valid);
  std::swap (m_iter, other.m_iter);
  std::swap (m_iter_trans, other.m_iter_trans);
}

Region &
Region::merge ()
{
  if (! m_is_merged) {

    if (m_merged_polygons_valid) {

      m_polygons.swap (m_merged_polygons);
      m_merged_polygons.clear ();
      m_is_merged = true;

    } else {

      merge (m_merge_min_coherence, 0);

    }

  }

  return *this;
}

Region &
Region::merge (bool min_coherence, unsigned int min_wc)
{
  if (empty ()) {

    //  ignore empty

  } else if (is_box ()) {

    //  take box only if min_wc == 0, otherwise clear
    if (min_wc > 0) {
      clear ();
    }

  } else {

    invalidate_cache ();

    db::EdgeProcessor ep (m_report_progress, m_progress_desc);

    //  count edges and reserve memory
    size_t n = 0;
    for (const_iterator p = begin (); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (const_iterator p = begin (); ! p.at_end (); ++p, ++n) {
      ep.insert (*p, n);
    }

    //  and run the merge step
    db::MergeOp op (min_wc);
    db::ShapeGenerator pc (m_polygons, true /*clear*/);
    db::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence);
    ep.process (pg, op);

    set_valid_polygons ();

    m_is_merged = true;

  }

  return *this;
}

Region &
Region::size (Region::coord_type dx, Region::coord_type dy, unsigned int mode)
{
  if (empty ()) {

    //  ignore empty

  } else if (is_box () && mode >= 2) {

    //  simplified handling for a box
    db::Box b = bbox ().enlarged (db::Vector (dx, dy));
    m_polygons.clear ();
    if (! b.empty () && b.width () > 0 && b.height () > 0) {
      m_polygons.insert (db::Polygon (b));
    } else {
      b = db::Box ();
    }

    m_is_merged = true;
    m_bbox = b;
    m_bbox_valid = true;

    m_merged_polygons.clear ();
    m_merged_polygons_valid = false;
    set_valid_polygons ();

  } else if (! m_merged_semantics) {

    invalidate_cache ();

    //  Generic case
    db::Shapes output (false);

    db::ShapeGenerator pc (output, false);
    db::PolygonGenerator pg (pc, false, true);
    db::SizingPolygonFilter sf (pg, dx, dy, mode);
    for (const_iterator p = begin (); ! p.at_end (); ++p) {
      sf.put (*p);
    }

    output.swap (m_polygons);
    set_valid_polygons ();

    m_is_merged = false;

  } else {

    invalidate_cache ();

    //  Generic case - the size operation will merge first
    db::EdgeProcessor ep (m_report_progress, m_progress_desc);

    //  count edges and reserve memory
    size_t n = 0;
    for (const_iterator p = begin (); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (const_iterator p = begin (); ! p.at_end (); ++p, ++n) {
      ep.insert (*p, n);
    }

    db::ShapeGenerator pc (m_polygons, true /*clear*/);
    db::PolygonGenerator pg2 (pc, false /*don't resolve holes*/, true /*min. coherence*/);
    db::SizingPolygonFilter siz (pg2, dx, dy, mode);
    db::PolygonGenerator pg (siz, false /*don't resolve holes*/, false /*min. coherence*/);
    db::BooleanOp op (db::BooleanOp::Or);
    ep.process (pg, op);

    set_valid_polygons ();

    m_is_merged = false;

  }

  return *this;
}

Region &
Region::operator&= (const Region &other)
{
  if (empty ()) {

    //  Nothing to do

  } else if (other.empty ()) {

    clear ();

  } else if (is_box () && other.is_box ()) {

    //  Simplified handling for boxes
    db::Box b = bbox ();
    b &= other.bbox ();
    m_polygons.clear ();
    if (! b.empty () && b.width () > 0 && b.height () > 0 ) {
      m_polygons.insert (db::Polygon (b));
    }

    m_is_merged = true;
    m_bbox = b;
    m_bbox_valid = true;

    m_merged_polygons.clear ();
    m_merged_polygons_valid = false;
    set_valid_polygons ();

  } else if (is_box () && ! other.strict_handling ()) {

    //  map AND with box to clip ..
    db::Box b = bbox ();
    m_polygons.clear ();

    std::vector<db::Polygon> clipped;
    for (const_iterator p = other.begin (); ! p.at_end (); ++p) {
      clipped.clear ();
      clip_poly (*p, b, clipped);
      m_polygons.insert (clipped.begin (), clipped.end ());
    }

    m_is_merged = false;
    invalidate_cache ();
    set_valid_polygons ();

  } else if (other.is_box () && ! m_strict_handling) {

    //  map AND with box to clip ..
    db::Box b = other.bbox ();
    db::Shapes polygons (false);

    std::vector<db::Polygon> clipped;
    for (const_iterator p = begin (); ! p.at_end (); ++p) {
      clipped.clear ();
      clip_poly (*p, b, clipped);
      polygons.insert (clipped.begin (), clipped.end ());
    }

    m_polygons.swap (polygons);
    m_is_merged = false;
    invalidate_cache ();
    set_valid_polygons ();

  } else if (! bbox ().overlaps (other.bbox ())) {

    //  Result will be nothing
    clear ();

  } else {

    invalidate_cache ();

    //  Generic case
    db::EdgeProcessor ep (m_report_progress, m_progress_desc);

    //  count edges and reserve memory
    size_t n = 0;
    for (const_iterator p = begin (); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    for (const_iterator p = other.begin (); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (const_iterator p = begin (); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }
    n = 1;
    for (const_iterator p = other.begin (); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }

    db::BooleanOp op (db::BooleanOp::And);
    db::ShapeGenerator pc (m_polygons, true /*clear*/);
    db::PolygonGenerator pg (pc, false /*don't resolve holes*/, m_merge_min_coherence);
    ep.process (pg, op);

    set_valid_polygons ();

    m_is_merged = true;

  }

  return *this;
}

Region &
Region::operator-= (const Region &other)
{
  if (empty ()) {

    //  Nothing to do

  } else if (other.empty () && ! m_strict_handling) {

    //  Nothing to do

  } else if (! bbox ().overlaps (other.bbox ()) && ! m_strict_handling) {

    //  Nothing to do

  } else {

    invalidate_cache ();

    //  Generic case
    db::EdgeProcessor ep (m_report_progress, m_progress_desc);

    //  count edges and reserve memory
    size_t n = 0;
    for (const_iterator p = begin (); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    for (const_iterator p = other.begin (); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (const_iterator p = begin (); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }
    n = 1;
    for (const_iterator p = other.begin (); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }

    db::BooleanOp op (db::BooleanOp::ANotB);
    db::ShapeGenerator pc (m_polygons, true /*clear*/);
    db::PolygonGenerator pg (pc, false /*don't resolve holes*/, m_merge_min_coherence);
    ep.process (pg, op);

    set_valid_polygons ();

    m_is_merged = true;

  }

  return *this;
}

Region &
Region::operator^= (const Region &other)
{
  if (empty () && ! other.strict_handling ()) {

    *this = other;

  } else if (other.empty () && ! m_strict_handling) {

    //  Nothing to do

  } else if (! bbox ().overlaps (other.bbox ()) && ! m_strict_handling && ! other.strict_handling ()) {

    //  Simplified handling for disjunct case
    *this |= other;

  } else {

    invalidate_cache ();

    //  Generic case
    db::EdgeProcessor ep (m_report_progress, m_progress_desc);

    //  count edges and reserve memory
    size_t n = 0;
    for (const_iterator p = begin (); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    for (const_iterator p = other.begin (); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (const_iterator p = begin (); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }
    n = 1;
    for (const_iterator p = other.begin (); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }

    db::BooleanOp op (db::BooleanOp::Xor);
    db::ShapeGenerator pc (m_polygons, true /*clear*/);
    db::PolygonGenerator pg (pc, false /*don't resolve holes*/, m_merge_min_coherence);
    ep.process (pg, op);

    set_valid_polygons ();

    m_is_merged = true;

  }

  return *this;
}

Region &
Region::operator|= (const Region &other)
{
  if (empty () && ! other.strict_handling ()) {

    *this = other;

  } else if (other.empty () && ! m_strict_handling) {

    //  Nothing to do

  } else if (! bbox ().overlaps (other.bbox ()) && ! m_strict_handling && ! other.strict_handling ()) {

    //  Simplified handling for disjunct case
    *this += other;

  } else {

    invalidate_cache ();

    //  Generic case
    db::EdgeProcessor ep (m_report_progress, m_progress_desc);

    //  count edges and reserve memory
    size_t n = 0;
    for (const_iterator p = begin (); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    for (const_iterator p = other.begin (); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (const_iterator p = begin (); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }
    n = 1;
    for (const_iterator p = other.begin (); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }

    db::BooleanOp op (db::BooleanOp::Or);
    db::ShapeGenerator pc (m_polygons, true /*clear*/);
    db::PolygonGenerator pg (pc, false /*don't resolve holes*/, m_merge_min_coherence);
    ep.process (pg, op);

    set_valid_polygons ();

    m_is_merged = true;

  }

  return *this;
}

Region &
Region::operator+= (const Region &other)
{
  invalidate_cache ();

  if (! has_valid_polygons ()) {

    m_polygons.clear ();

    size_t n = 0;
    for (const_iterator p = begin (); ! p.at_end (); ++p) {
      ++n;
    }
    for (const_iterator p = other.begin (); ! p.at_end (); ++p) {
      ++n;
    }

    m_polygons.reserve (db::Polygon::tag (), n);

    for (const_iterator p = begin (); ! p.at_end (); ++p) {
      m_polygons.insert (*p);
    }
    for (const_iterator p = other.begin (); ! p.at_end (); ++p) {
      m_polygons.insert (*p);
    }

    set_valid_polygons ();

  } else if (! other.has_valid_polygons ()) {

    size_t n = m_polygons.size ();
    for (const_iterator p = other.begin (); ! p.at_end (); ++p) {
      ++n;
    }

    m_polygons.reserve (db::Polygon::tag (), n);

    for (const_iterator p = other.begin (); ! p.at_end (); ++p) {
      m_polygons.insert (*p);
    }

  } else {
    m_polygons.insert (other.m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().begin (), other.m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().end ());
  }

  m_is_merged = false;
  return *this;
}

Region
Region::selected_interacting_generic (const Region &other, int mode, bool touching, bool inverse) const
{
  db::EdgeProcessor ep (m_report_progress, m_progress_desc);

  //  shortcut
  if (empty () || other.empty ()) {
    if (mode <= 0) {
      return Region ();
    } else {
      return *this;
    }
  }

  for (const_iterator p = other.begin (); ! p.at_end (); ++p) {
    if (p->box ().touches (bbox ())) {
      ep.insert (*p, 0);
    }
  }

  size_t n = 1;
  for (const_iterator p = begin_merged (); ! p.at_end (); ++p, ++n) {
    if (mode > 0 || p->box ().touches (other.bbox ())) {
      ep.insert (*p, n);
    }
  }

  db::InteractionDetector id (mode, 0);
  id.set_include_touching (touching);
  db::EdgeSink es;
  ep.process (es, id);
  id.finish ();

  Region out;
  n = 0;
  std::set <size_t> selected;
  for (db::InteractionDetector::iterator i = id.begin (); i != id.end () && i->first == 0; ++i) {
    ++n;
    selected.insert (i->second);
  }

  out.reserve (n);

  n = 1;
  for (const_iterator p = begin_merged (); ! p.at_end (); ++p, ++n) {
    if ((selected.find (n) == selected.end ()) == inverse) {
      out.insert (*p);
    }
  }

  return out;
}

void
Region::select_interacting_generic (const Region &other, int mode, bool touching, bool inverse)
{
  //  shortcut
  if (empty () || other.empty ()) {
    if (mode <= 0) {
      clear ();
    }
    return;
  }

  db::EdgeProcessor ep (m_report_progress, m_progress_desc);

  for (const_iterator p = other.begin (); ! p.at_end (); ++p) {
    if (p->box ().touches (bbox ())) {
      ep.insert (*p, 0);
    }
  }

  size_t n = 1;
  for (const_iterator p = begin_merged (); ! p.at_end (); ++p, ++n) {
    if (mode > 0 || p->box ().touches (other.bbox ())) {
      ep.insert (*p, n);
    }
  }

  invalidate_cache ();

  db::InteractionDetector id (mode, 0);
  id.set_include_touching (touching);
  db::EdgeSink es;
  ep.process (es, id);
  id.finish ();

  db::Shapes out (false);
  std::set <size_t> selected;
  n = 0;
  for (db::InteractionDetector::iterator i = id.begin (); i != id.end () && i->first == 0; ++i) {
    selected.insert (i->second);
    ++n;
  }

  out.reserve (db::Polygon::tag (), n);
  n = 1;
  for (const_iterator p = begin_merged (); ! p.at_end (); ++p, ++n) {
    if ((selected.find (n) == selected.end ()) == inverse) {
      out.insert (*p);
    }
  }

  m_polygons.swap (out);
  set_valid_polygons ();
}

EdgePairs  
Region::grid_check (db::Coord gx, db::Coord gy) const
{
  EdgePairs out;

  gx = std::max (db::Coord (1), gx);
  gy = std::max (db::Coord (1), gy);

  for (const_iterator p = begin_merged (); ! p.at_end (); ++p) {

    for (size_t i = 0; i < p->holes () + 1; ++i) {

      db::Polygon::polygon_contour_iterator b, e;

      if (i == 0) {
        b = p->begin_hull ();
        e = p->end_hull ();
      } else {
        b = p->begin_hole (i - 1);
        e = p->end_hole (i - 1);
      }

      for (db::Polygon::polygon_contour_iterator pt = b; pt != e; ++pt) {
        if (((*pt).x () % gx) != 0 || ((*pt).y () % gy) != 0) {
          out.insert (EdgePair (db::Edge (*pt, *pt), db::Edge (*pt, *pt)));
        }
      }

    }

  }

  return out;
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

EdgePairs  
Region::angle_check (double min, double max, bool inverse) const
{
  EdgePairs out;

  double cos_min = cos (std::max (0.0, std::min (360.0, min)) / 180.0 * M_PI);
  double cos_max = cos (std::max (0.0, std::min (360.0, max)) / 180.0 * M_PI);
  bool gt180_min = min > 180.0;
  bool gt180_max = max > 180.0;

  for (const_iterator p = begin_merged (); ! p.at_end (); ++p) {

    for (size_t i = 0; i < p->holes () + 1; ++i) {

      const db::Polygon::contour_type *h = 0;
      if (i == 0) {
        h = &p->hull ();
      } else {
        h = &p->hole (i - 1);
      }

      size_t np = h->size ();

      for (size_t j = 0; j < np; ++j) {

        db::Edge e ((*h) [j], (*h) [(j + 1) % np]);
        db::Edge ee (e.p2 (), (*h) [(j + 2) % np]);
        double le = e.double_length ();
        double lee = ee.double_length ();

        double cos_a = -db::sprod (e, ee) / (le * lee);
        bool gt180_a = db::vprod_sign (e, ee) > 0;

        if ((ac_less (cos_a, gt180_a, cos_max, gt180_max) && !ac_less (cos_a, gt180_a, cos_min, gt180_min)) == !inverse) {
          out.insert (EdgePair (e, ee));
        }

      }

    }

  }

  return out;
}

static inline db::Coord snap_to_grid (db::Coord c, db::Coord g)
{
  //  This form of snapping always snaps g/2 to right/top.
  if (c < 0) {
    c = -g * ((-c + (g - 1) / 2) / g);
  } else {
    c = g * ((c + g / 2) / g);
  }
  return c;
}

void  
Region::snap (db::Coord gx, db::Coord gy)
{
  db::Shapes polygons (false);

  gx = std::max (db::Coord (1), gx);
  gy = std::max (db::Coord (1), gy);

  std::vector<db::Point> pts;

  for (const_iterator p = begin_merged (); ! p.at_end (); ++p) {

    db::Polygon pnew;

    for (size_t i = 0; i < p->holes () + 1; ++i) {

      pts.clear ();

      db::Polygon::polygon_contour_iterator b, e;

      if (i == 0) {
        b = p->begin_hull ();
        e = p->end_hull ();
      } else {
        b = p->begin_hole (i - 1);
        e = p->end_hole (i - 1);
      }

      for (db::Polygon::polygon_contour_iterator pt = b; pt != e; ++pt) {
        pts.push_back (db::Point (snap_to_grid ((*pt).x (), gx), snap_to_grid ((*pt).y (), gy)));
      }

      if (i == 0) {
        pnew.assign_hull (pts.begin (), pts.end ());
      } else {
        pnew.insert_hole (pts.begin (), pts.end ());
      }

    }

    polygons.insert (pnew);

  }

  m_polygons.swap (polygons);

  bool was_merged = m_merged_semantics;
  invalidate_cache ();
  m_is_merged = was_merged;
  set_valid_polygons ();
}

/**
 *  @brief A helper class to implement the strange polygon detector
 */
struct StrangePolygonInsideFunc 
{
  inline bool operator() (int wc) const
  {
    return wc < 0 || wc > 1;
  }
};

Region  
Region::strange_polygon_check () const
{
  EdgeProcessor ep;
  Region out;

  for (const_iterator p = begin (); ! p.at_end (); ++p) {

    ep.clear ();
    ep.insert (*p);

    StrangePolygonInsideFunc inside;
    db::GenericMerge<StrangePolygonInsideFunc> op (inside);
    RegionPolygonSink pc (out);
    db::PolygonGenerator pg (pc, false, false);
    ep.process (pg, op);
  }

  return out;
}

void 
Region::init ()
{
  m_report_progress = false;
  m_bbox_valid = true;
  m_is_merged = true;
  m_merged_semantics = true;
  m_strict_handling = false;
  m_merge_min_coherence = false;
  m_merged_polygons_valid = false;
}

void 
Region::disable_progress ()
{
  m_report_progress = false;
}

void 
Region::enable_progress (const std::string &progress_desc)
{
  m_report_progress = true;
  m_progress_desc = progress_desc;
}

void
Region::invalidate_cache ()
{
  m_bbox_valid = false;
  m_merged_polygons.clear ();
  m_merged_polygons_valid = false;
}

void
Region::ensure_valid_merged_polygons () const
{
  //  If no merged semantics applies or we will deliver the original
  //  polygons as merged ones, we need to make sure those are valid
  //  ones (with a unique memory address)
  if (! m_merged_semantics || m_is_merged) {
    ensure_valid_polygons ();
  } else {
    ensure_merged_polygons_valid ();
  }
}

void
Region::ensure_valid_polygons () const
{
  if (! has_valid_polygons ()) {

    m_polygons.clear ();

    size_t n = 0;
    for (const_iterator p = begin (); ! p.at_end (); ++p) {
      ++n;
    }
    m_polygons.reserve (db::Polygon::tag (), n);

    for (const_iterator p = begin (); ! p.at_end (); ++p) {
      m_polygons.insert (*p);
    }

    //  set valid polygons
    m_iter = db::RecursiveShapeIterator ();

  }
}

void
Region::set_valid_polygons ()
{
  m_iter = db::RecursiveShapeIterator ();
}

void 
Region::ensure_bbox_valid () const
{
  if (! m_bbox_valid) {
    m_bbox = db::Box ();
    for (const_iterator p = begin (); ! p.at_end (); ++p) {
      m_bbox += p->box ();
    }
    m_bbox_valid = true;
  }
}

Region::const_iterator 
Region::begin_merged () const
{
  if (! m_merged_semantics || m_is_merged) {
    return begin ();
  } else {
    ensure_merged_polygons_valid ();
    return db::RegionIterator (m_merged_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().begin (), m_merged_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().end ());
  }
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans>
Region::begin_iter () const
{
  if (has_valid_polygons ()) {
    return std::make_pair (db::RecursiveShapeIterator (m_polygons), db::ICplxTrans ());
  } else {
    return std::make_pair (m_iter, m_iter_trans);
  }
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans>
Region::begin_merged_iter () const
{
  if (! m_merged_semantics || m_is_merged) {
    return begin_iter ();
  } else {
    ensure_merged_polygons_valid ();
    return std::make_pair (db::RecursiveShapeIterator (m_merged_polygons), db::ICplxTrans ());
  }
}

void 
Region::ensure_merged_polygons_valid () const
{
  if (! m_merged_polygons_valid) {

    m_merged_polygons.clear ();

    db::EdgeProcessor ep (m_report_progress, m_progress_desc);

    //  count edges and reserve memory
    size_t n = 0;
    for (const_iterator p = begin (); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (const_iterator p = begin (); ! p.at_end (); ++p, ++n) {
      ep.insert (*p, n);
    }

    //  and run the merge step
    db::MergeOp op (0);
    db::ShapeGenerator pc (m_merged_polygons);
    db::PolygonGenerator pg (pc, false /*don't resolve holes*/, m_merge_min_coherence);
    ep.process (pg, op);

    m_merged_polygons_valid = true;

  }
}

void 
Region::insert (const db::Box &box)
{
  if (! box.empty () && box.width () > 0 && box.height () > 0) {
    ensure_valid_polygons ();
    m_polygons.insert (db::Polygon (box));
    m_is_merged = false;
    invalidate_cache ();
  }
}

void 
Region::insert (const db::Path &path)
{
  if (path.points () > 0) {
    ensure_valid_polygons ();
    m_polygons.insert (path.polygon ());
    m_is_merged = false;
    invalidate_cache ();
  }
}

void 
Region::insert (const db::Polygon &polygon)
{
  if (polygon.holes () > 0 || polygon.vertices () > 0) {
    ensure_valid_polygons ();
    m_polygons.insert (polygon);
    m_is_merged = false;
    invalidate_cache ();
  }
}

void 
Region::insert (const db::SimplePolygon &polygon)
{
  if (polygon.vertices () > 0) {
    ensure_valid_polygons ();
    db::Polygon poly;
    poly.assign_hull (polygon.begin_hull (), polygon.end_hull ());
    m_polygons.insert (poly);
    m_is_merged = false;
    invalidate_cache ();
  }
}

void
Region::insert (const db::Shape &shape)
{
  if (shape.is_polygon () || shape.is_path () || shape.is_box ()) {
    ensure_valid_polygons ();
    db::Polygon poly;
    shape.polygon (poly);
    m_polygons.insert (poly);
    m_is_merged = false;
    invalidate_cache ();
  }
}

void 
Region::clear ()
{
  m_polygons.clear ();
  m_bbox = db::Box ();
  m_bbox_valid = true;
  m_is_merged = true;
  m_merged_polygons.clear ();
  m_merged_polygons_valid = true;
  m_iter = db::RecursiveShapeIterator ();
  m_iter_trans = db::ICplxTrans ();
}

namespace {

/**
 *  @brief A helper class for the DRC functionality which acts as an edge pair receiver
 */
class Edge2EdgeCheck 
  : public db::box_scanner_receiver<db::Edge, size_t>
{
public:
  Edge2EdgeCheck (const EdgeRelationFilter &check, EdgePairs &output, bool different_polygons, bool requires_different_layers)
    : mp_check (&check), mp_output (&output), m_requires_different_layers (requires_different_layers), m_different_polygons (different_polygons), 
      m_pass (0)
  {
    m_distance = check.distance ();
  }

  bool prepare_next_pass ()
  {
    ++m_pass;

    if (m_pass == 1) {

      if (! m_ep.empty ()) {
        m_ep_discarded.resize (m_ep.size (), false);
        return true;
      }

    } else if (m_pass == 2) {

      std::vector<bool>::const_iterator d = m_ep_discarded.begin ();
      std::vector<db::EdgePair>::const_iterator ep = m_ep.begin ();
      while (ep != m_ep.end ()) {
        tl_assert (d != m_ep_discarded.end ());
        if (! *d) {
          mp_output->insert (*ep);
        }
        ++d;
        ++ep;
      }

    }

    return false;
  }
  
  void add (const db::Edge *o1, size_t p1, const db::Edge *o2, size_t p2)
  { 
    if (m_pass == 0) {

      //  Overlap or inside checks require input from different layers
      if ((! m_different_polygons || p1 != p2) && (! m_requires_different_layers || ((p1 ^ p2) & 1) != 0)) {

        //  ensure that the first check argument is of layer 1 and the second of
        //  layer 2 (unless both are of the same layer)
        int l1 = int (p1 & size_t (1));
        int l2 = int (p2 & size_t (1));

        db::EdgePair ep;
        if (mp_check->check (l1 <= l2 ? *o1 : *o2, l1 <= l2 ? *o2 : *o1, &ep)) {

          //  found a violation: store inside the local buffer for now. In the second
          //  pass we will eliminate those which are shielded completely.
          size_t n = m_ep.size ();
          m_ep.push_back (ep);
          m_e2ep.insert (std::make_pair (std::make_pair (*o1, p1), n));
          m_e2ep.insert (std::make_pair (std::make_pair (*o2, p2), n));

        }

      }

    } else {

      //  a simple (complete) shielding implementation which is based on the 
      //  assumption that shielding is relevant as soon as a foreign edge cuts through
      //  both of the edge pair's connecting edges.
      
      //  TODO: this implementation does not take into account the nature of the
      //  EdgePair - because of "whole_edge" it may not reflect the part actually
      //  violating the distance.
      
      std::vector<size_t> n1, n2;

      for (unsigned int p = 0; p < 2; ++p) {

        std::pair<db::Edge, size_t> k (*o1, p1);
        for (std::multimap<std::pair<db::Edge, size_t>, size_t>::const_iterator i = m_e2ep.find (k); i != m_e2ep.end () && i->first == k; ++i) {
          n1.push_back (i->second);
        }

        std::sort (n1.begin (), n1.end ());
      
        std::swap (o1, o2);
        std::swap (p1, p2);
        n1.swap (n2);

      }

      for (unsigned int p = 0; p < 2; ++p) {

        std::vector<size_t> nn;
        std::set_difference (n1.begin (), n1.end (), n2.begin (), n2.end (), std::back_inserter (nn));

        for (std::vector<size_t>::const_iterator i = nn.begin (); i != nn.end (); ++i) {
          if (! m_ep_discarded [*i]) {
            db::EdgePair ep = m_ep [*i].normalized ();
            if (db::Edge (ep.first ().p1 (), ep.second ().p2 ()).intersect (*o2) && 
                db::Edge (ep.second ().p1 (), ep.first ().p2 ()).intersect (*o2)) {
              m_ep_discarded [*i] = true;
            }
          }
        }

        std::swap (o1, o2);
        std::swap (p1, p2);
        n1.swap (n2);

      }

    }

  }

  /**
   *  @brief Gets a value indicating whether the check requires different layers
   */
  bool requires_different_layers () const
  {
    return m_requires_different_layers;
  }

  /**
   *  @brief Sets a value indicating whether the check requires different layers
   */
  void set_requires_different_layers (bool f) 
  {
    m_requires_different_layers = f;
  }

  /**
   *  @brief Gets a value indicating whether the check requires different layers
   */
  bool different_polygons () const
  {
    return m_different_polygons;
  }

  /**
   *  @brief Sets a value indicating whether the check requires different layers
   */
  void set_different_polygons (bool f) 
  {
    m_different_polygons = f;
  }

  /**
   *  @brief Gets the distance value
   */
  EdgeRelationFilter::distance_type distance () const
  {
    return m_distance;
  }

private:
  const EdgeRelationFilter *mp_check;
  EdgePairs *mp_output;
  bool m_requires_different_layers;
  bool m_different_polygons;
  EdgeRelationFilter::distance_type m_distance;
  std::vector<db::EdgePair> m_ep;
  std::multimap<std::pair<db::Edge, size_t>, size_t> m_e2ep;
  std::vector<bool> m_ep_discarded;
  unsigned int m_pass;
};

/**
 *  @brief A helper class for the DRC functionality which acts as an edge pair receiver
 */
class Poly2PolyCheck 
  : public db::box_scanner_receiver<db::Polygon, size_t>
{
public:
  Poly2PolyCheck (Edge2EdgeCheck &output)
    : mp_output (&output)
  {
    //  .. nothing yet ..
  }
  
  void finish (const db::Polygon *o, size_t p)
  { 
    if (! mp_output->requires_different_layers () && ! mp_output->different_polygons ()) {

      //  finally we check the polygons vs. itself for checks involving intra-polygon interactions

      m_scanner.clear ();
      m_scanner.reserve (o->vertices ());

      m_edges.clear ();
      m_edges.reserve (o->vertices ());

      for (db::Polygon::polygon_edge_iterator e = o->begin_edge (); ! e.at_end (); ++e) {
        m_edges.push_back (*e);
        m_scanner.insert (& m_edges.back (), p);
      }

      tl_assert (m_edges.size () == o->vertices ());

      m_scanner.process (*mp_output, mp_output->distance (), db::box_convert<db::Edge> ()); 

    }
  }

  void add (const db::Polygon *o1, size_t p1, const db::Polygon *o2, size_t p2)
  { 
    if ((! mp_output->different_polygons () || p1 != p2) && (! mp_output->requires_different_layers () || ((p1 ^ p2) & 1) != 0)) {

      m_scanner.clear ();
      m_scanner.reserve (o1->vertices () + o2->vertices ());

      m_edges.clear ();
      m_edges.reserve (o1->vertices () + o2->vertices ());

      for (db::Polygon::polygon_edge_iterator e = o1->begin_edge (); ! e.at_end (); ++e) {
        m_edges.push_back (*e);
        m_scanner.insert (& m_edges.back (), p1);
      }

      for (db::Polygon::polygon_edge_iterator e = o2->begin_edge (); ! e.at_end (); ++e) {
        m_edges.push_back (*e);
        m_scanner.insert (& m_edges.back (), p2);
      }

      tl_assert (m_edges.size () == o1->vertices () + o2->vertices ());

      //  temporarily disable intra-polygon check in that step .. we do that later in finish()
      //  if required (#650).
      bool no_intra = mp_output->different_polygons ();
      mp_output->set_different_polygons (true);

      m_scanner.process (*mp_output, mp_output->distance (), db::box_convert<db::Edge> ()); 

      mp_output->set_different_polygons (no_intra);

    }
  }

private:
  db::box_scanner<db::Edge, size_t> m_scanner;
  Edge2EdgeCheck *mp_output;
  std::vector<db::Edge> m_edges;
};

}

EdgePairs 
Region::run_check (db::edge_relation_type rel, bool different_polygons, const Region *other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
{
  EdgePairs result;

  db::box_scanner<db::Polygon, size_t> scanner (m_report_progress, m_progress_desc);
  scanner.reserve (size () + (other ? other->size () : 0));

  ensure_valid_merged_polygons ();
  size_t n = 0;
  for (const_iterator p = begin_merged (); ! p.at_end (); ++p) {
    scanner.insert (&*p, n); 
    n += 2;
  }

  if (other) {
    other->ensure_valid_merged_polygons ();
    n = 1;
    for (const_iterator p = other->begin_merged (); ! p.at_end (); ++p) {
      scanner.insert (&*p, n); 
      n += 2;
    }
  }

  EdgeRelationFilter check (rel, d, metrics);
  check.set_include_zero (other != 0);
  check.set_whole_edges (whole_edges);
  check.set_ignore_angle (ignore_angle);
  check.set_min_projection (min_projection);
  check.set_max_projection (max_projection);

  Edge2EdgeCheck edge_check (check, result, different_polygons, other != 0);
  Poly2PolyCheck poly_check (edge_check);

  do {
    scanner.process (poly_check, d, db::box_convert<db::Polygon> ());
  } while (edge_check.prepare_next_pass ());

  return result;
}

EdgePairs 
Region::run_single_polygon_check (db::edge_relation_type rel, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
{
  EdgePairs result;

  EdgeRelationFilter check (rel, d, metrics);
  check.set_whole_edges (whole_edges);
  check.set_ignore_angle (ignore_angle);
  check.set_min_projection (min_projection);
  check.set_max_projection (max_projection);

  Edge2EdgeCheck edge_check (check, result, false, false);
  Poly2PolyCheck poly_check (edge_check);

  do {
    size_t n = 0;
    for (const_iterator p = begin_merged (); ! p.at_end (); ++p) {
      poly_check.finish (&*p, n); 
      n += 2;
    }
  } while (edge_check.prepare_next_pass ());

  return result;
}

}

namespace tl
{
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::Region &b)
  {
    db::Polygon p;

    if (! ex.try_read (p)) {
      return false;
    }
    b.insert (p);

    while (ex.test (";")) {
      ex.read (p);
      b.insert (p);
    } 

    return true;
  }

  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::Region &b)
  {
    if (! test_extractor_impl (ex, b)) {
      ex.error (tl::to_string (QObject::tr ("Expected an region collection specification")));
    }
  }
}

