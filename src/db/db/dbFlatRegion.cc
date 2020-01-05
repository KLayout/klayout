
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


#include "dbFlatRegion.h"
#include "dbEmptyRegion.h"
#include "dbRegion.h"
#include "dbShapeProcessor.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  FlatRegion implementation

FlatRegion::FlatRegion ()
  : AsIfFlatRegion (), m_polygons (false), m_merged_polygons (false)
{
  init ();
}

FlatRegion::~FlatRegion ()
{
  //  .. nothing yet ..
}

FlatRegion::FlatRegion (const FlatRegion &other)
  : AsIfFlatRegion (other), m_polygons (false), m_merged_polygons (false)
{
  init ();

  m_is_merged = other.m_is_merged;
  m_polygons = other.m_polygons;
  m_merged_polygons = other.m_merged_polygons;
  m_merged_polygons_valid = other.m_merged_polygons_valid;
}

FlatRegion::FlatRegion (const db::Shapes &polygons, bool is_merged)
  : AsIfFlatRegion (), m_polygons (polygons), m_merged_polygons (false)
{
  init ();

  m_is_merged = is_merged;
}

FlatRegion::FlatRegion (bool is_merged)
  : AsIfFlatRegion (), m_polygons (false), m_merged_polygons (false)
{
  init ();

  m_is_merged = is_merged;
}

void FlatRegion::set_is_merged (bool m)
{
  m_is_merged = m;
}

void FlatRegion::invalidate_cache ()
{
  invalidate_bbox ();
  m_merged_polygons.clear ();
  m_merged_polygons_valid = false;
}

void FlatRegion::init ()
{
  m_is_merged = true;
  m_merged_polygons_valid = false;
}

void FlatRegion::merged_semantics_changed ()
{
  m_merged_polygons.clear ();
  m_merged_polygons_valid = false;
}

void FlatRegion::min_coherence_changed ()
{
  m_is_merged = false;
  m_merged_polygons.clear ();
  m_merged_polygons_valid = false;
}

void FlatRegion::reserve (size_t n)
{
  m_polygons.reserve (db::Polygon::tag (), n);
}

void
FlatRegion::ensure_merged_polygons_valid () const
{
  if (! m_merged_polygons_valid) {

    m_merged_polygons.clear ();

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

    //  and run the merge step
    db::MergeOp op (0);
    db::ShapeGenerator pc (m_merged_polygons);
    db::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence ());
    ep.process (pg, op);

    m_merged_polygons_valid = true;

  }
}

RegionIteratorDelegate *FlatRegion::begin () const
{
  return new FlatRegionIterator (m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().begin (), m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().end ());
}

RegionIteratorDelegate *FlatRegion::begin_merged () const
{
  if (! merged_semantics () || m_is_merged) {
    return begin ();
  } else {
    ensure_merged_polygons_valid ();
    return new FlatRegionIterator (m_merged_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().begin (), m_merged_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().end ());
  }
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans> FlatRegion::begin_iter () const
{
  return std::make_pair (db::RecursiveShapeIterator (m_polygons), db::ICplxTrans ());
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans> FlatRegion::begin_merged_iter () const
{
  if (! merged_semantics () || m_is_merged) {
    return begin_iter ();
  } else {
    ensure_merged_polygons_valid ();
    return std::make_pair (db::RecursiveShapeIterator (m_merged_polygons), db::ICplxTrans ());
  }
}

bool FlatRegion::empty () const
{
  return m_polygons.empty ();
}

size_t FlatRegion::size () const
{
  return m_polygons.size ();
}

bool FlatRegion::is_merged () const
{
  return m_is_merged;
}

Box FlatRegion::compute_bbox () const
{
  m_polygons.update_bbox ();
  return m_polygons.bbox ();
}

RegionDelegate *FlatRegion::filter_in_place (const PolygonFilterBase &filter)
{
  polygon_iterator_type pw = m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().begin ();
  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    if (filter.selected (*p)) {
      if (pw == m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().end ()) {
        m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().insert (*p);
        pw = m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().end ();
      } else {
        m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().replace (pw++, *p);
      }
    }
  }

  m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().erase (pw, m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().end ());
  m_merged_polygons.clear ();
  m_is_merged = merged_semantics ();

  return this;
}

RegionDelegate *FlatRegion::process_in_place (const PolygonProcessorBase &filter)
{
  std::vector<db::Polygon> poly_res;

  polygon_iterator_type pw = m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().begin ();
  for (RegionIterator p (filter.requires_raw_input () ? begin () : begin_merged ()); ! p.at_end (); ++p) {

    poly_res.clear ();
    filter.process (*p, poly_res);

    for (std::vector<db::Polygon>::const_iterator pr = poly_res.begin (); pr != poly_res.end (); ++pr) {
      if (pw == m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().end ()) {
        m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().insert (*pr);
        pw = m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().end ();
      } else {
        m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().replace (pw++, *pr);
      }
    }

  }

  m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().erase (pw, m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().end ());
  m_merged_polygons.clear ();
  m_is_merged = filter.result_is_merged () && merged_semantics ();

  if (filter.result_must_not_be_merged ()) {
    set_merged_semantics (false);
  }

  return this;
}

RegionDelegate *FlatRegion::merged_in_place ()
{
  if (! m_is_merged) {

    if (m_merged_polygons_valid) {

      m_polygons.swap (m_merged_polygons);
      m_merged_polygons.clear ();
      m_is_merged = true;
      return this;

    } else {
      return merged_in_place (min_coherence (), 0);
    }

  } else {
    return this;
  }
}

RegionDelegate *FlatRegion::merged_in_place (bool min_coherence, unsigned int min_wc)
{
  if (empty ()) {

    //  ignore empty
    return new EmptyRegion ();

  } else if (is_box ()) {

    //  take box only if min_wc == 0, otherwise clear
    if (min_wc > 0) {
      return new EmptyRegion ();
    }

  } else {

    invalidate_cache ();

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

    //  and run the merge step
    db::MergeOp op (min_wc);
    db::ShapeGenerator pc (m_polygons, true /*clear*/);
    db::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence);
    ep.process (pg, op);

    m_is_merged = true;

  }

  return this;
}

RegionDelegate *FlatRegion::merged () const
{
  if (! m_is_merged) {

    if (m_merged_polygons_valid) {
      return new FlatRegion (m_merged_polygons, true);
    } else {
      return AsIfFlatRegion::merged (min_coherence (), 0);
    }

  } else {
    return clone ();
  }
}

RegionDelegate *FlatRegion::add (const Region &other) const
{
  std::auto_ptr<FlatRegion> new_region (new FlatRegion (*this));
  new_region->invalidate_cache ();
  new_region->set_is_merged (false);

  FlatRegion *other_flat = dynamic_cast<FlatRegion *> (other.delegate ());
  if (other_flat) {

    new_region->raw_polygons ().insert (other_flat->raw_polygons ().get_layer<db::Polygon, db::unstable_layer_tag> ().begin (), other_flat->raw_polygons ().get_layer<db::Polygon, db::unstable_layer_tag> ().end ());

  } else {

    size_t n = new_region->raw_polygons ().size ();
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      ++n;
    }

    new_region->raw_polygons ().reserve (db::Polygon::tag (), n);

    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      new_region->raw_polygons ().insert (*p);
    }

  }

  return new_region.release ();
}

RegionDelegate *FlatRegion::add_in_place (const Region &other)
{
  invalidate_cache ();
  m_is_merged = false;

  FlatRegion *other_flat = dynamic_cast<FlatRegion *> (other.delegate ());
  if (other_flat) {

    m_polygons.insert (other_flat->raw_polygons ().get_layer<db::Polygon, db::unstable_layer_tag> ().begin (), other_flat->raw_polygons ().get_layer<db::Polygon, db::unstable_layer_tag> ().end ());

  } else {

    size_t n = m_polygons.size ();
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      ++n;
    }

    m_polygons.reserve (db::Polygon::tag (), n);

    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      m_polygons.insert (*p);
    }

  }

  return this;
}

const db::Polygon *FlatRegion::nth (size_t n) const
{
  return n < m_polygons.size () ? &m_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().begin () [n] : 0;
}

bool FlatRegion::has_valid_polygons () const
{
  return true;
}

bool FlatRegion::has_valid_merged_polygons () const
{
  return true;
}

const db::RecursiveShapeIterator *FlatRegion::iter () const
{
  return 0;
}

void FlatRegion::insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  layout->cell (into_cell).shapes (into_layer).insert (m_polygons);
}

void
FlatRegion::insert (const db::Box &box)
{
  if (! box.empty () && box.width () > 0 && box.height () > 0) {

    if (empty ()) {

      m_polygons.insert (db::Polygon (box));
      m_is_merged = true;
      update_bbox (box);

    } else {

      m_polygons.insert (db::Polygon (box));
      m_is_merged = false;
      invalidate_cache ();

    }

  }
}

void
FlatRegion::insert (const db::Path &path)
{
  if (path.points () > 0) {
    m_polygons.insert (path.polygon ());
    m_is_merged = false;
    invalidate_cache ();
  }
}

void
FlatRegion::insert (const db::Polygon &polygon)
{
  if (polygon.holes () > 0 || polygon.vertices () > 0) {
    m_polygons.insert (polygon);
    m_is_merged = false;
    invalidate_cache ();
  }
}

void
FlatRegion::insert (const db::SimplePolygon &polygon)
{
  if (polygon.vertices () > 0) {
    db::Polygon poly;
    poly.assign_hull (polygon.begin_hull (), polygon.end_hull ());
    m_polygons.insert (poly);
    m_is_merged = false;
    invalidate_cache ();
  }
}

void
FlatRegion::insert (const db::Shape &shape)
{
  if (shape.is_polygon () || shape.is_path () || shape.is_box ()) {
    db::Polygon poly;
    shape.polygon (poly);
    m_polygons.insert (poly);
    m_is_merged = false;
    invalidate_cache ();
  }
}

}

