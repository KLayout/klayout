
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


#include "dbOriginalLayerRegion.h"
#include "dbFlatRegion.h"
#include "dbFlatEdges.h"
#include "dbRegion.h"
#include "dbDeepEdges.h"
#include "dbDeepRegion.h"
#include "dbDeepShapeStore.h"
#include "dbCellGraphUtils.h"
#include "tlGlobPattern.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  OriginalLayerRegion implementation

namespace
{

  class OriginalLayerRegionIterator
    : public RegionIteratorDelegate
  {
  public:
    OriginalLayerRegionIterator (const db::RecursiveShapeIterator &iter, const db::ICplxTrans &trans)
      : m_rec_iter (iter), m_iter_trans (trans), m_prop_id (0)
    {
      set ();
    }

    virtual bool is_addressable() const
    {
      return false;
    }

    virtual bool at_end () const
    {
      return m_rec_iter.at_end ();
    }

    virtual void increment ()
    {
      do_increment ();
      set ();
    }

    virtual const value_type *get () const
    {
      return &m_polygon;
    }

    virtual db::properties_id_type prop_id () const
    {
      return m_prop_id;
    }

    virtual RegionIteratorDelegate *clone () const
    {
      return new OriginalLayerRegionIterator (*this);
    }

    virtual bool equals (const generic_shape_iterator_delegate_base<value_type> *other) const
    {
      const OriginalLayerRegionIterator *o = dynamic_cast<const OriginalLayerRegionIterator *> (other);
      return o && o->m_rec_iter == m_rec_iter && o->m_iter_trans.equal (m_iter_trans);
    }

    virtual void do_reset (const db::Box &region, bool overlapping)
    {
      if (region == db::Box::world ()) {
        m_rec_iter.set_region (region);
      } else {
        m_rec_iter.set_region (m_iter_trans.inverted () * region);
      }
      m_rec_iter.set_overlapping (overlapping);
      set ();
    }

    virtual db::Box bbox () const
    {
      return m_iter_trans * m_rec_iter.bbox ();
    }

  private:
    friend class Region;

    db::RecursiveShapeIterator m_rec_iter;
    db::ICplxTrans m_iter_trans;
    db::Polygon m_polygon;
    db::properties_id_type m_prop_id;

    void set ()
    {
      while (! m_rec_iter.at_end () && ! (m_rec_iter->is_polygon () || m_rec_iter->is_path () || m_rec_iter->is_box ())) {
        ++m_rec_iter;
      }
      if (! m_rec_iter.at_end ()) {
        m_rec_iter->polygon (m_polygon);
        m_polygon.transform (m_iter_trans * m_rec_iter.trans (), false);
        m_prop_id = m_rec_iter.prop_id ();
      }
    }

    void do_increment ()
    {
      if (! m_rec_iter.at_end ()) {
        ++m_rec_iter;
      }
    }
  };

}

OriginalLayerRegion::OriginalLayerRegion ()
  : AsIfFlatRegion (), m_merged_polygons (false)
{
  init ();
}

OriginalLayerRegion::OriginalLayerRegion (const OriginalLayerRegion &other)
  : AsIfFlatRegion (other),
    m_is_merged (other.m_is_merged),
    m_merged_polygons (other.m_merged_polygons),
    m_merged_polygons_valid (other.m_merged_polygons_valid),
    m_iter (other.m_iter),
    m_iter_trans (other.m_iter_trans)
{
  //  .. nothing yet ..
}

OriginalLayerRegion::OriginalLayerRegion (const RecursiveShapeIterator &si, bool is_merged)
  : AsIfFlatRegion (), m_merged_polygons (false), m_iter (si)
{
  init ();

  m_is_merged = is_merged;
}

OriginalLayerRegion::OriginalLayerRegion (const RecursiveShapeIterator &si, const db::ICplxTrans &trans, bool merged_semantics, bool is_merged)
  : AsIfFlatRegion (), m_merged_polygons (false), m_iter (si), m_iter_trans (trans)
{
  init ();

  m_is_merged = is_merged;
  set_merged_semantics (merged_semantics);
}

OriginalLayerRegion::~OriginalLayerRegion ()
{
  //  .. nothing yet ..
}

RegionDelegate *
OriginalLayerRegion::clone () const
{
  return new OriginalLayerRegion (*this);
}

void
OriginalLayerRegion::merged_semantics_changed ()
{
  m_merged_polygons.clear ();
  m_merged_polygons_valid = false;
}

void
OriginalLayerRegion::min_coherence_changed ()
{
  m_is_merged = false;
  m_merged_polygons.clear ();
  m_merged_polygons_valid = false;
}

size_t
OriginalLayerRegion::count () const
{
  //  NOTE: we should to make sure the iterator isn't validated as this would spoil the usability or OriginalLayerRegion upon
  //  layout changes
  db::RecursiveShapeIterator iter = m_iter;

  if (iter.has_complex_region () || iter.region () != db::Box::world () || ! iter.enables ().empty () || ! iter.disables ().empty ()) {

    //  complex case with a search region - use the iterator to determine the count (expensive)
    size_t n = 0;
    for (db::RecursiveShapeIterator i = iter; ! i.at_end (); ++i) {
      ++n;
    }

    return n;

  } else if (! iter.layout ()) {

    //  for Shapes-based iterators just use the shape count

    if (iter.shapes ()) {
      return iter.shapes ()->size (iter.shape_flags () & db::ShapeIterator::Regions);
    } else {
      return 0;
    }

  } else {

    //  otherwise we can utilize the CellCounter

    size_t n = 0;

    const db::Layout &layout = *iter.layout ();

    std::set<db::cell_index_type> cells;
    iter.top_cell ()->collect_called_cells (cells);
    cells.insert (iter.top_cell ()->cell_index ());

    db::CellCounter cc (&layout);
    for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
      if (cells.find (*c) == cells.end ()) {
        continue;
      }
      size_t nn = 0;
      if (iter.multiple_layers ()) {
        for (std::vector<unsigned int>::const_iterator l = iter.layers ().begin (); l != iter.layers ().end (); ++l) {
          nn += layout.cell (*c).shapes (*l).size (iter.shape_flags () & (db::ShapeIterator::Regions | db::ShapeIterator::Properties));
        }
      } else if (iter.layer () < layout.layers ()) {
        nn += layout.cell (*c).shapes (iter.layer ()).size (iter.shape_flags () & (db::ShapeIterator::Regions | db::ShapeIterator::Properties));
      }
      n += cc.weight (*c) * nn;
    }

    return n;

  }
}

size_t
OriginalLayerRegion::hier_count () const
{
  //  NOTE: we should to make sure the iterator isn't validated as this would spoil the usability or OriginalLayerRegion upon
  //  layout changes
  db::RecursiveShapeIterator iter = m_iter;

  if (iter.has_complex_region () || iter.region () != db::Box::world ()) {

    //  TODO: how to establish a "hierarchical" interpretation in this case?
    return count ();

  } else {

    size_t n = 0;

    const db::Layout &layout = *iter.layout ();

    std::set<db::cell_index_type> cells;
    iter.top_cell ()->collect_called_cells (cells);
    cells.insert (iter.top_cell ()->cell_index ());

    for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
      if (cells.find (*c) == cells.end ()) {
        continue;
      }
      if (iter.multiple_layers ()) {
        for (std::vector<unsigned int>::const_iterator l = iter.layers ().begin (); l != iter.layers ().end (); ++l) {
          n += layout.cell (*c).shapes (*l).size (iter.shape_flags () & db::ShapeIterator::Regions);
        }
      } else if (iter.layer () < layout.layers ()) {
        n += layout.cell (*c).shapes (iter.layer ()).size (iter.shape_flags () & db::ShapeIterator::Regions);
      }
    }

    return n;

  }
}

RegionIteratorDelegate *
OriginalLayerRegion::begin () const
{
  return new OriginalLayerRegionIterator (m_iter, m_iter_trans);
}

RegionIteratorDelegate *
OriginalLayerRegion::begin_merged () const
{
  if (! merged_semantics () || m_is_merged) {
    return begin ();
  } else {
    ensure_merged_polygons_valid ();
    return new FlatRegionIterator (& m_merged_polygons);
  }
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans>
OriginalLayerRegion::begin_iter () const
{
  return std::make_pair (m_iter, m_iter_trans);
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans>
OriginalLayerRegion::begin_merged_iter () const
{
  if (! merged_semantics () || m_is_merged) {
    return begin_iter ();
  } else {
    ensure_merged_polygons_valid ();
    return std::make_pair (db::RecursiveShapeIterator (m_merged_polygons), db::ICplxTrans ());
  }
}

bool
OriginalLayerRegion::empty () const
{
  return m_iter.at_end ();
}

bool
OriginalLayerRegion::is_merged () const
{
  return m_is_merged;
}

const db::Polygon *
OriginalLayerRegion::nth (size_t) const
{
  throw tl::Exception (tl::to_string (tr ("Random access to polygons is available only for flat regions")));
}

db::properties_id_type
OriginalLayerRegion::nth_prop_id (size_t) const
{
  throw tl::Exception (tl::to_string (tr ("Random access to polygons is available only for flat regions")));
}

bool
OriginalLayerRegion::has_valid_polygons () const
{
  return false;
}

bool
OriginalLayerRegion::has_valid_merged_polygons () const
{
  return merged_semantics () && ! m_is_merged;
}

const db::RecursiveShapeIterator *
OriginalLayerRegion::iter () const
{
  return &m_iter;
}

void
OriginalLayerRegion::apply_property_translator (const db::PropertiesTranslator &pt)
{
  m_iter.apply_property_translator (pt);

  m_merged_polygons_valid = false;
  m_merged_polygons.clear ();
}

db::PropertiesRepository *
OriginalLayerRegion::properties_repository ()
{
  return m_iter.layout () ? &const_cast<db::Layout * >(m_iter.layout ())->properties_repository () : 0;
}

const db::PropertiesRepository *
OriginalLayerRegion::properties_repository () const
{
  return m_iter.layout () ? &m_iter.layout ()->properties_repository () : 0;
}

bool
OriginalLayerRegion::equals (const Region &other) const
{
  const OriginalLayerRegion *other_delegate = dynamic_cast<const OriginalLayerRegion *> (other.delegate ());
  if (other_delegate && other_delegate->m_iter == m_iter && other_delegate->m_iter_trans == m_iter_trans) {
    return true;
  } else {
    return AsIfFlatRegion::equals (other);
  }
}

bool
OriginalLayerRegion::less (const Region &other) const
{
  const OriginalLayerRegion *other_delegate = dynamic_cast<const OriginalLayerRegion *> (other.delegate ());
  if (other_delegate && other_delegate->m_iter == m_iter && other_delegate->m_iter_trans == m_iter_trans) {
    return false;
  } else {
    return AsIfFlatRegion::less (other);
  }
}

void
OriginalLayerRegion::init ()
{
  m_is_merged = false;
  m_merged_polygons_valid = false;
}

namespace {

struct AssignProp
{
  AssignProp () : prop_id (0) { }
  db::properties_id_type operator() (db::properties_id_type) { return prop_id; }
  db::properties_id_type prop_id;
};

}

void
OriginalLayerRegion::insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  //  explicitly update the layout if the source is equal to the target
  //  (needed because we lock the layout below and no update would happen)
  if (layout == m_iter.layout ()) {
    layout->update ();
  }

  db::Shapes &sh = layout->cell (into_cell).shapes (into_layer);

  db::PropertyMapper pm;
  if (m_iter.layout ()) {
    pm = db::PropertyMapper (layout, m_iter.layout ());
  }

  //  NOTE: if the source (r) is from the same layout than the shapes live in, we better
  //  lock the layout against updates while inserting
  db::LayoutLocker locker (layout);
  AssignProp ap;
  for (db::RecursiveShapeIterator i = m_iter; !i.at_end (); ++i) {
    db::properties_id_type prop_id = i.prop_id ();
    ap.prop_id = (prop_id != 0 ? pm (prop_id) : 0);
    sh.insert (*i, i.trans (), ap);
  }
}

void
OriginalLayerRegion::ensure_merged_polygons_valid () const
{
  if (! m_merged_polygons_valid) {

    m_merged_polygons.clear ();
    merge_polygons_to (m_merged_polygons, min_coherence (), 0);

    m_merged_polygons_valid = true;

  }
}

}
