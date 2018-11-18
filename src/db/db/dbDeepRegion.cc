
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


#include "dbDeepRegion.h"
#include "dbDeepShapeStore.h"
#include "dbEmptyRegion.h"
#include "dbRegion.h"
#include "dbShapeProcessor.h"
#include "dbFlatRegion.h"
#include "dbHierProcessor.h"
#include "dbCellMapping.h"
#include "dbLayoutUtils.h"

namespace db
{

namespace
{
  /**
   *  @brief An iterator delegate for the deep region
   *  TODO: this is kind of redundant with OriginalLayerIterator ..
   */
  class DB_PUBLIC DeepRegionIterator
    : public RegionIteratorDelegate
  {
  public:
    typedef db::Polygon value_type;

    DeepRegionIterator (const db::RecursiveShapeIterator &iter)
      : m_iter (iter)
    {
      set ();
    }

    virtual bool at_end () const
    {
      return m_iter.at_end ();
    }

    virtual void increment ()
    {
      ++m_iter;
      set ();
    }

    virtual const value_type *get () const
    {
      return &m_polygon;
    }

    virtual RegionIteratorDelegate *clone () const
    {
      return new DeepRegionIterator (*this);
    }

  private:
    friend class Region;

    db::RecursiveShapeIterator m_iter;
    mutable value_type m_polygon;

    void set () const
    {
      if (! m_iter.at_end ()) {
        m_iter.shape ().polygon (m_polygon);
        m_polygon.transform (m_iter.trans (), false);
      }
    }
  };

}

// -------------------------------------------------------------------------------------------------------------
//  DeepRegion implementation

DeepRegion::DeepRegion (const RecursiveShapeIterator &si, DeepShapeStore &dss, double area_ratio, size_t max_vertex_count)
  : AsIfFlatRegion (), m_deep_layer (dss.create_polygon_layer (si, area_ratio, max_vertex_count)), m_merged_polygons (false)
{
  init ();
}

DeepRegion::DeepRegion (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans, bool merged_semantics, double area_ratio, size_t max_vertex_count)
  : AsIfFlatRegion (), m_deep_layer (dss.create_polygon_layer (si, area_ratio, max_vertex_count)), m_merged_polygons (false)
{
  init ();

  tl_assert (trans.is_unity ()); // TODO: implement
  set_merged_semantics (merged_semantics);
}

DeepRegion::DeepRegion ()
  : AsIfFlatRegion ()
{
  init ();
}

DeepRegion::DeepRegion (const DeepLayer &dl)
  : AsIfFlatRegion (), m_deep_layer (dl)
{
  init ();
}

DeepRegion::~DeepRegion ()
{
  //  .. nothing yet ..
}

DeepRegion::DeepRegion (const DeepRegion &other)
  : AsIfFlatRegion (other),
    m_deep_layer (other.m_deep_layer.copy ()),
    m_merged_polygons (other.m_merged_polygons),
    m_merged_polygons_valid (other.m_merged_polygons_valid)
{
  //  .. nothing yet ..
}

void DeepRegion::init ()
{
  m_merged_polygons_valid = false;
  m_merged_polygons.clear ();
}

RegionDelegate *
DeepRegion::clone () const
{
  return new DeepRegion (*this);
}

void DeepRegion::merged_semantics_changed ()
{
  //  .. nothing yet ..
}

RegionIteratorDelegate *
DeepRegion::begin () const
{
  return new DeepRegionIterator (begin_iter ().first);
}

RegionIteratorDelegate *
DeepRegion::begin_merged () const
{
  if (! merged_semantics ()) {
    return begin ();
  } else {
    ensure_merged_polygons_valid ();
    return new FlatRegionIterator (m_merged_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().begin (), m_merged_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().end ());
  }
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans>
DeepRegion::begin_iter () const
{
  const db::Layout *layout = m_deep_layer.layout ();
  if (layout->cells () == 0) {

    return std::make_pair (db::RecursiveShapeIterator (), db::ICplxTrans ());

  } else {

    const db::Cell &top_cell = layout->cell (*layout->begin_top_down ());
    db::RecursiveShapeIterator iter (*m_deep_layer.layout (), top_cell, m_deep_layer.layer ());
    return std::make_pair (iter, db::ICplxTrans ());

  }
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans>
DeepRegion::begin_merged_iter () const
{
  if (! merged_semantics ()) {
    return begin_iter ();
  } else {
    ensure_merged_polygons_valid ();
    return std::make_pair (db::RecursiveShapeIterator (m_merged_polygons), db::ICplxTrans ());
  }
}

bool
DeepRegion::empty () const
{
  return begin_iter ().first.at_end ();
}

bool
DeepRegion::is_merged () const
{
  return false;
}

const db::Polygon *
DeepRegion::nth (size_t) const
{
  throw tl::Exception (tl::to_string (tr ("Random access to polygons is available only for flat regions")));
}

bool
DeepRegion::has_valid_polygons () const
{
  return false;
}

bool
DeepRegion::has_valid_merged_polygons () const
{
  return merged_semantics ();
}

const db::RecursiveShapeIterator *
DeepRegion::iter () const
{
  return 0;
}

bool
DeepRegion::equals (const Region &other) const
{
  const DeepRegion *other_delegate = dynamic_cast<const DeepRegion *> (other.delegate ());
  if (other_delegate && other_delegate->m_deep_layer.layout () == m_deep_layer.layout ()
      && other_delegate->m_deep_layer.layer () == m_deep_layer.layer ()) {
    return true;
  } else {
    return AsIfFlatRegion::equals (other);
  }
}

bool
DeepRegion::less (const Region &other) const
{
  const DeepRegion *other_delegate = dynamic_cast<const DeepRegion *> (other.delegate ());
  if (other_delegate && other_delegate->m_deep_layer.layout () == m_deep_layer.layout ()) {
    return other_delegate->m_deep_layer.layer () < m_deep_layer.layer ();
  } else {
    return AsIfFlatRegion::less (other);
  }
}

void
DeepRegion::ensure_merged_polygons_valid () const
{
  if (! m_merged_polygons_valid) {

    m_merged_polygons.clear ();

    db::EdgeProcessor ep (report_progress (), progress_desc ());

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

void
DeepRegion::insert_into (db::Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  m_deep_layer.insert_into (layout, into_cell, into_layer);
}

RegionDelegate *
DeepRegion::and_with (const Region &other) const
{
  const DeepRegion *other_deep = dynamic_cast <const DeepRegion *> (other.delegate ());

  if (empty () || other.empty ()) {

    //  Nothing to do
    return new EmptyRegion ();

  } else if (! other_deep) {

    return AsIfFlatRegion::and_with (other);

  } else {

    return new DeepRegion (and_or_not_with (other_deep, true));

  }
}

RegionDelegate *
DeepRegion::not_with (const Region &other) const
{
  const DeepRegion *other_deep = dynamic_cast <const DeepRegion *> (other.delegate ());

  if (empty ()) {

    //  Nothing to do
    return new EmptyRegion ();

  } else if (other.empty () && ! strict_handling ()) {

    //  Nothing to do
    return clone ();

  } else if (! other_deep) {

    return AsIfFlatRegion::not_with (other);

  } else {

    return new DeepRegion (and_or_not_with (other_deep, false));

  }
}

DeepLayer
DeepRegion::and_or_not_with (const DeepRegion *other, bool and_op) const
{
  DeepLayer dl_out (m_deep_layer.derived ());

  db::BoolAndOrNotLocalOperation op (and_op);

  db::LocalProcessor proc (const_cast <db::Layout *> (m_deep_layer.layout ()), const_cast <db::Cell *> (m_deep_layer.initial_cell ()), other->deep_layer ().layout (), other->deep_layer ().initial_cell ());
  proc.set_threads (m_deep_layer.store ()->threads ());

  proc.run (&op, m_deep_layer.layer (), other->deep_layer ().layer (), dl_out.layer ());

  return dl_out;
}

RegionDelegate *
DeepRegion::add_in_place (const Region &other)
{
  if (other.empty ()) {
    return this;
  }

  const DeepRegion *other_deep = dynamic_cast <const DeepRegion *> (other.delegate ());
  if (other_deep) {

    if (other_deep->deep_layer ().layout () == deep_layer ().layout ()) {

      //  intra-layout merge

      deep_layer ().layout ()->copy_layer (other_deep->deep_layer ().layer (), deep_layer ().layer ());

    } else {

      //  inter-layout merge

      db::cell_index_type into_cell = deep_layer ().initial_cell ()->cell_index ();
      db::Layout *into_layout = deep_layer ().layout ();
      db::cell_index_type source_cell = other_deep->deep_layer ().initial_cell ()->cell_index ();
      const db::Layout *source_layout = other_deep->deep_layer ().layout ();

      db::CellMapping cm;
      cm.create_from_geometry_full (*into_layout, into_cell, *source_layout, source_cell);

      //  Actually copy the shapes

      std::map<unsigned int, unsigned int> lm;
      lm.insert (std::make_pair (other_deep->deep_layer ().layer (), deep_layer ().layer ()));

      std::vector <db::cell_index_type> source_cells;
      source_cells.push_back (source_cell);
      db::copy_shapes (*into_layout, *source_layout, db::ICplxTrans (), source_cells, cm.table (), lm);

    }

  } else {

    //  non-deep to deep merge (flat)

    db::Shapes &shapes = deep_layer ().initial_cell ()->shapes (deep_layer ().layer ());
    for (db::Region::const_iterator p = other.begin (); ! p.at_end (); ++p) {
      shapes.insert (*p);
    }

  }

  return this;

}

RegionDelegate *
DeepRegion::add (const Region &other) const
{
  if (other.empty ()) {
    return clone ();
  } else if (empty ()) {
    return other.delegate ()->clone ();
  } else {
    DeepRegion *new_region = dynamic_cast<DeepRegion *> (clone ());
    new_region->add_in_place (other);
    return new_region;
  }
}

}
