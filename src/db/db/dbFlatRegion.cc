
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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
  : MutableRegion (), mp_polygons (new db::Shapes (false)), mp_merged_polygons (new db::Shapes (false))
{
  init ();
}

FlatRegion::~FlatRegion ()
{
  //  .. nothing yet ..
}

FlatRegion::FlatRegion (const FlatRegion &other)
  : MutableRegion (other), mp_polygons (other.mp_polygons), mp_merged_polygons (other.mp_merged_polygons)
{
  init ();

  m_is_merged = other.m_is_merged;
  m_merged_polygons_valid = other.m_merged_polygons_valid;
}

FlatRegion::FlatRegion (const db::Shapes &polygons, bool is_merged)
  : MutableRegion (), mp_polygons (new db::Shapes (polygons)), mp_merged_polygons (new db::Shapes (false))
{
  init ();

  m_is_merged = is_merged;
}

FlatRegion::FlatRegion (bool is_merged)
  : MutableRegion (), mp_polygons (new db::Shapes (false)), mp_merged_polygons (new db::Shapes (false))
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
  mp_merged_polygons->clear ();
  m_merged_polygons_valid = false;
}

void FlatRegion::init ()
{
  m_is_merged = false;
  m_merged_polygons_valid = false;
}

void FlatRegion::merged_semantics_changed ()
{
  mp_merged_polygons->clear ();
  m_merged_polygons_valid = false;
}

void FlatRegion::min_coherence_changed ()
{
  m_is_merged = false;
  mp_merged_polygons->clear ();
  m_merged_polygons_valid = false;
}

void FlatRegion::reserve (size_t n)
{
  mp_polygons->reserve (db::Polygon::tag (), n);
}

void
FlatRegion::ensure_merged_polygons_valid () const
{
  if (! m_merged_polygons_valid) {

    mp_merged_polygons->clear ();

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
    db::ShapeGenerator pc (*mp_merged_polygons);
    db::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence ());
    ep.process (pg, op);

    m_merged_polygons_valid = true;

  }
}

RegionIteratorDelegate *FlatRegion::begin () const
{
  return new FlatRegionIterator (mp_polygons.get_const ());
}

RegionIteratorDelegate *FlatRegion::begin_merged () const
{
  if (! merged_semantics () || m_is_merged) {
    return begin ();
  } else {
    ensure_merged_polygons_valid ();
    return new FlatRegionIterator (mp_merged_polygons.get_const ());
  }
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans> FlatRegion::begin_iter () const
{
  return std::make_pair (db::RecursiveShapeIterator (*mp_polygons), db::ICplxTrans ());
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans> FlatRegion::begin_merged_iter () const
{
  if (! merged_semantics () || m_is_merged) {
    return begin_iter ();
  } else {
    ensure_merged_polygons_valid ();
    return std::make_pair (db::RecursiveShapeIterator (*mp_merged_polygons), db::ICplxTrans ());
  }
}

bool FlatRegion::empty () const
{
  return mp_polygons->empty ();
}

size_t FlatRegion::count () const
{
  return mp_polygons->size ();
}

size_t FlatRegion::hier_count () const
{
  return mp_polygons->size ();
}

bool FlatRegion::is_merged () const
{
  return m_is_merged;
}

Box FlatRegion::compute_bbox () const
{
  return mp_polygons->bbox ();
}

RegionDelegate *FlatRegion::filter_in_place (const PolygonFilterBase &filter)
{
  db::layer<db::Polygon, db::unstable_layer_tag> &poly_layer = mp_polygons->get_layer<db::Polygon, db::unstable_layer_tag> ();

  polygon_iterator_type pw = poly_layer.begin ();
  for (RegionIterator p (filter.requires_raw_input () ? begin () : begin_merged ()); ! p.at_end (); ++p) {
    if (filter.selected (*p)) {
      if (pw == poly_layer.end ()) {
        poly_layer.insert (*p);
        poly_layer.end ();
      } else {
        poly_layer.replace (pw++, *p);
      }
    }
  }

  poly_layer.erase (pw, poly_layer.end ());

  mp_merged_polygons->clear ();
  m_is_merged = filter.requires_raw_input () ? false : merged_semantics ();

  return this;
}

RegionDelegate *FlatRegion::process_in_place (const PolygonProcessorBase &filter)
{
  db::layer<db::Polygon, db::unstable_layer_tag> &poly_layer = mp_polygons->get_layer<db::Polygon, db::unstable_layer_tag> ();
  db::layer<db::Polygon, db::unstable_layer_tag> out;

  std::vector<db::Polygon> poly_res;
  for (RegionIterator p (filter.requires_raw_input () ? begin () : begin_merged ()); ! p.at_end (); ++p) {
    poly_res.clear ();
    filter.process (*p, poly_res);
    out.insert (poly_res.begin (), poly_res.end ());
  }

  poly_layer.swap (out);

  mp_merged_polygons->clear ();
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

      db::Shapes &merged_polygons = *mp_merged_polygons;
      mp_polygons->swap (merged_polygons);
      merged_polygons.clear ();
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
    db::ShapeGenerator pc (*mp_polygons, true /*clear*/);
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
      return new FlatRegion (*mp_merged_polygons, true);
    } else {
      return AsIfFlatRegion::merged (min_coherence (), 0);
    }

  } else {
    return clone ();
  }
}

RegionDelegate *FlatRegion::add (const Region &other) const
{
  std::unique_ptr<FlatRegion> new_region (new FlatRegion (*this));
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

  db::Shapes &polygons = *mp_polygons;

  FlatRegion *other_flat = dynamic_cast<FlatRegion *> (other.delegate ());
  if (other_flat) {

    polygons.insert (other_flat->raw_polygons ().get_layer<db::Polygon, db::unstable_layer_tag> ().begin (), other_flat->raw_polygons ().get_layer<db::Polygon, db::unstable_layer_tag> ().end ());

  } else {

    size_t n = polygons.size ();
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      ++n;
    }

    polygons.reserve (db::Polygon::tag (), n);

    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      polygons.insert (*p);
    }

  }

  return this;
}

const db::Polygon *FlatRegion::nth (size_t n) const
{
  return n < mp_polygons->size () ? &mp_polygons->get_layer<db::Polygon, db::unstable_layer_tag> ().begin () [n] : 0;
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
  layout->cell (into_cell).shapes (into_layer).insert (*mp_polygons);
}

void
FlatRegion::do_insert (const db::Polygon &polygon)
{
  if (polygon.holes () > 0 || polygon.vertices () > 0) {

    bool is_box = (empty () && polygon.is_box ());

    mp_polygons->insert (polygon);
    set_is_merged (is_box);

    invalidate_cache ();

  }
}

}

