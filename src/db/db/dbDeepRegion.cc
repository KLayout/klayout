
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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
#include "dbEmptyEdgePairs.h"
#include "dbRegion.h"
#include "dbRegionUtils.h"
#include "dbDeepEdges.h"
#include "dbDeepEdgePairs.h"
#include "dbDeepTexts.h"
#include "dbShapeProcessor.h"
#include "dbFlatRegion.h"
#include "dbHierProcessor.h"
#include "dbCellMapping.h"
#include "dbLayoutUtils.h"
#include "dbHierNetworkProcessor.h"
#include "dbCellGraphUtils.h"
#include "dbPolygonTools.h"
#include "dbCellVariants.h"
#include "dbRegionLocalOperations.h"
#include "dbLocalOperationUtils.h"
#include "dbCompoundOperation.h"
#include "tlTimer.h"

namespace db
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

  virtual ~DeepRegionIterator () { }

  virtual bool at_end () const
  {
    return m_iter.at_end ();
  }

  virtual void increment ()
  {
    ++m_iter;
    set ();
  }

  virtual bool is_addressable() const
  {
    return false;
  }

  virtual const value_type *get () const
  {
    return &m_polygon;
  }

  virtual bool equals (const generic_shape_iterator_delegate_base<value_type> *other) const
  {
    const DeepRegionIterator *o = dynamic_cast<const DeepRegionIterator *> (other);
    return o && o->m_iter == m_iter;
  }

  virtual RegionIteratorDelegate *clone () const
  {
    return new DeepRegionIterator (*this);
  }

  virtual void do_reset (const db::Box &region, bool overlapping)
  {
    m_iter.set_region (region);
    m_iter.set_overlapping (overlapping);
    set ();
  }

  virtual db::Box bbox () const
  {
    return m_iter.bbox ();
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

// -------------------------------------------------------------------------------------------------------------
//  DeepRegion implementation

DeepRegion::DeepRegion (const RecursiveShapeIterator &si, DeepShapeStore &dss, double area_ratio, size_t max_vertex_count)
  : MutableRegion (), m_merged_polygons ()
{
  set_deep_layer (dss.create_polygon_layer (si, area_ratio, max_vertex_count));
  init ();
}

DeepRegion::DeepRegion (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans, bool merged_semantics, double area_ratio, size_t max_vertex_count)
  : MutableRegion (), m_merged_polygons ()
{
  set_deep_layer (dss.create_polygon_layer (si, area_ratio, max_vertex_count, trans));
  init ();
  set_merged_semantics (merged_semantics);
}

DeepRegion::DeepRegion (const db::Region &other, DeepShapeStore &dss)
  : MutableRegion (), m_merged_polygons ()
{
  set_deep_layer (dss.create_from_flat (other, false));

  init ();
  set_merged_semantics (other.merged_semantics ());
}

DeepRegion::DeepRegion ()
  : MutableRegion ()
{
  init ();
}

DeepRegion::DeepRegion (const DeepLayer &dl)
  : MutableRegion ()
{
  set_deep_layer (dl);
  init ();
}

DeepRegion::~DeepRegion ()
{
  //  .. nothing yet ..
}

DeepRegion::DeepRegion (const DeepRegion &other)
  : MutableRegion (other), DeepShapeCollectionDelegateBase (other),
    m_merged_polygons_valid (other.m_merged_polygons_valid),
    m_is_merged (other.m_is_merged)
{
  if (m_merged_polygons_valid) {
    m_merged_polygons = other.m_merged_polygons;
  }
}

DeepRegion &
DeepRegion::operator= (const DeepRegion &other)
{
  if (this != &other) {

    AsIfFlatRegion::operator= (other);
    DeepShapeCollectionDelegateBase::operator= (other);

    m_merged_polygons_valid = other.m_merged_polygons_valid;
    m_is_merged = other.m_is_merged;
    if (m_merged_polygons_valid) {
      m_merged_polygons = other.m_merged_polygons;
    }

  }

  return *this;
}

void DeepRegion::init ()
{
  m_merged_polygons_valid = false;
  m_merged_polygons = db::DeepLayer ();
  m_is_merged = false;
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

void DeepRegion::min_coherence_changed ()
{
  set_is_merged (false);
}

void DeepRegion::do_insert (const db::Polygon &polygon)
{
  db::Layout &layout = deep_layer ().layout ();
  if (layout.begin_top_down () != layout.end_top_down ()) {
    db::Cell &top_cell = layout.cell (*layout.begin_top_down ());
    top_cell.shapes (deep_layer ().layer ()).insert (db::PolygonRef (polygon, layout.shape_repository ()));
  }

  invalidate_bbox ();
  set_is_merged (false);
}

template <class Trans>
static void transform_deep_layer (db::DeepLayer &deep_layer, const Trans &t)
{
  //  TODO: this is a pretty cheap implementation. At least a plain move can be done with orientation variants.

  db::Layout &layout = deep_layer.layout ();
  if (layout.begin_top_down () != layout.end_top_down ()) {

    db::Cell &top_cell = layout.cell (*layout.begin_top_down ());

    db::Shapes flat_shapes (layout.is_editable ());
    for (db::RecursiveShapeIterator iter (layout, top_cell, deep_layer.layer ()); !iter.at_end (); ++iter) {
      db::Polygon poly;
      iter->polygon (poly);
      flat_shapes.insert (poly.transformed (iter.trans ()).transformed (t));
    }

    layout.clear_layer (deep_layer.layer ());
    top_cell.shapes (deep_layer.layer ()).swap (flat_shapes);

  }
}

void DeepRegion::do_transform (const db::Trans &t)
{
  transform_deep_layer (deep_layer (), t);
  invalidate_bbox ();
}

void DeepRegion::do_transform (const db::ICplxTrans &t)
{
  transform_deep_layer (deep_layer (), t);
  invalidate_bbox ();
}

void DeepRegion::do_transform (const db::IMatrix2d &t)
{
  transform_deep_layer (deep_layer (), t);
  invalidate_bbox ();
}

void DeepRegion::do_transform (const db::IMatrix3d &t)
{
  transform_deep_layer (deep_layer (), t);
  invalidate_bbox ();
}

void DeepRegion::reserve (size_t)
{
  //  Not implemented for deep regions
}

void DeepRegion::flatten ()
{
  db::Layout &layout = deep_layer ().layout ();
  if (layout.begin_top_down () != layout.end_top_down ()) {

    db::Cell &top_cell = layout.cell (*layout.begin_top_down ());

    db::Shapes flat_shapes (layout.is_editable ());
    for (db::RecursiveShapeIterator iter (layout, top_cell, deep_layer ().layer ()); !iter.at_end (); ++iter) {
      db::Polygon poly;
      iter->polygon (poly);
      flat_shapes.insert (poly.transformed (iter.trans ()));
    }

    layout.clear_layer (deep_layer ().layer ());
    top_cell.shapes (deep_layer ().layer ()).swap (flat_shapes);

  }
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
    return new DeepRegionIterator (begin_merged_iter ().first);
  }
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans>
DeepRegion::begin_iter () const
{
  const db::Layout &layout = deep_layer ().layout ();
  if (layout.cells () == 0) {

    return std::make_pair (db::RecursiveShapeIterator (), db::ICplxTrans ());

  } else {

    const db::Cell &top_cell = layout.cell (*layout.begin_top_down ());
    db::RecursiveShapeIterator iter (deep_layer ().layout (), top_cell, deep_layer ().layer ());
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

    const db::Layout &layout = m_merged_polygons.layout ();
    if (layout.cells () == 0) {

      return std::make_pair (db::RecursiveShapeIterator (), db::ICplxTrans ());

    } else {

      const db::Cell &top_cell = layout.cell (*layout.begin_top_down ());
      db::RecursiveShapeIterator iter (m_merged_polygons.layout (), top_cell, m_merged_polygons.layer ());
      return std::make_pair (iter, db::ICplxTrans ());

    }

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
  return m_is_merged;
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
  return false;
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
  if (other_delegate && &other_delegate->deep_layer ().layout () == &deep_layer ().layout ()
      && other_delegate->deep_layer ().layer () == deep_layer ().layer ()) {
    return true;
  } else {
    return AsIfFlatRegion::equals (other);
  }
}

bool
DeepRegion::less (const Region &other) const
{
  const DeepRegion *other_delegate = dynamic_cast<const DeepRegion *> (other.delegate ());
  if (other_delegate && &other_delegate->deep_layer ().layout () == &deep_layer ().layout ()) {
    return other_delegate->deep_layer ().layer () < deep_layer ().layer ();
  } else {
    return AsIfFlatRegion::less (other);
  }
}

namespace {

class ClusterMerger
{
public:
  ClusterMerger (unsigned int layer, db::Layout &layout, const db::hier_clusters<db::PolygonRef> &hc, bool min_coherence, bool report_progress, const std::string &progress_desc)
    : m_layer (layer), mp_layout (&layout), mp_hc (&hc), m_min_coherence (min_coherence), m_ep (report_progress, progress_desc)
  {
    //  .. nothing yet ..
  }

  void set_base_verbosity (int vb)
  {
    m_ep.set_base_verbosity (vb);
  }

  db::Shapes &merged (size_t cid, db::cell_index_type ci, unsigned int min_wc = 0)
  {
    return compute_merged (cid, ci, true, min_wc);
  }

  void erase (size_t cid, db::cell_index_type ci)
  {
    m_merged_cluster.erase (std::make_pair (cid, ci));
  }

private:
  std::map<std::pair<size_t, db::cell_index_type>, db::Shapes> m_merged_cluster;
  unsigned int m_layer;
  db::Layout *mp_layout;
  const db::hier_clusters<db::PolygonRef> *mp_hc;
  bool m_min_coherence;
  db::EdgeProcessor m_ep;

  db::Shapes &compute_merged (size_t cid, db::cell_index_type ci, bool initial, unsigned int min_wc)
  {
    std::map<std::pair<size_t, db::cell_index_type>, db::Shapes>::iterator s = m_merged_cluster.find (std::make_pair (cid, ci));

    //  some sanity checks: initial clusters are single-use, are never generated twice and cannot be retrieved again
    if (initial) {
      tl_assert (s == m_merged_cluster.end ());
    }

    if (s != m_merged_cluster.end ()) {
      return s->second;
    }

    s = m_merged_cluster.insert (std::make_pair (std::make_pair (cid, ci), db::Shapes (false))).first;

    const db::connected_clusters<db::PolygonRef> &cc = mp_hc->clusters_per_cell (ci);
    const db::local_cluster<db::PolygonRef> &c = cc.cluster_by_id (cid);

    if (min_wc > 0) {

      //  We cannot merge bottom-up in min_wc mode, so we just use the recursive cluster iterator

      m_ep.clear ();

      size_t pi = 0;

      for (db::recursive_cluster_shape_iterator<db::PolygonRef> s = db::recursive_cluster_shape_iterator<db::PolygonRef> (*mp_hc, m_layer, ci, cid); !s.at_end (); ++s) {
        db::Polygon poly = s->obj ();
        poly.transform (s.trans () * db::ICplxTrans (s->trans ()));
        m_ep.insert (poly, pi++);
      }

    } else {

      std::list<std::pair<const db::Shapes *, db::ICplxTrans> > merged_child_clusters;

      const db::connected_clusters<db::PolygonRef>::connections_type &conn = cc.connections_for_cluster (cid);
      for (db::connected_clusters<db::PolygonRef>::connections_type::const_iterator i = conn.begin (); i != conn.end (); ++i) {
        const db::Shapes &cc_shapes = compute_merged (i->id (), i->inst_cell_index (), false, min_wc);
        merged_child_clusters.push_back (std::make_pair (&cc_shapes, i->inst_trans ()));
      }

      m_ep.clear ();

      size_t pi = 0;

      for (std::list<std::pair<const db::Shapes *, db::ICplxTrans> >::const_iterator i = merged_child_clusters.begin (); i != merged_child_clusters.end (); ++i) {
        for (db::Shapes::shape_iterator s = i->first->begin (db::ShapeIterator::All); ! s.at_end (); ++s) {
          if (s->is_polygon ()) {
            db::Polygon poly;
            s->polygon (poly);
            m_ep.insert (poly.transformed (i->second), pi++);
          }
        }
      }

      for (db::local_cluster<db::PolygonRef>::shape_iterator s = c.begin (m_layer); !s.at_end (); ++s) {
        db::Polygon poly = s->obj ();
        poly.transform (s->trans ());
        m_ep.insert (poly, pi++);
      }

    }

    //  and run the merge step
    db::MergeOp op (min_wc);
    db::PolygonRefToShapesGenerator pr (mp_layout, &s->second);
    db::PolygonGenerator pg (pr, false /*don't resolve holes*/, m_min_coherence);
    m_ep.process (pg, op);

    return s->second;
  }
};

}

const DeepLayer &
DeepRegion::merged_deep_layer () const
{
  if (merged_semantics ()) {
    ensure_merged_polygons_valid ();
    return m_merged_polygons;
  } else {
    return deep_layer ();
  }
}

void
DeepRegion::ensure_merged_polygons_valid () const
{
  if (! m_merged_polygons_valid) {

    if (m_is_merged) {

      //  NOTE: this will reuse the deep layer reference
      m_merged_polygons = deep_layer ();

    } else {

      m_merged_polygons = deep_layer ().derived ();

      tl::SelfTimer timer (tl::verbosity () > base_verbosity (), "Ensure merged polygons");

      db::Layout &layout = const_cast<db::Layout &> (deep_layer ().layout ());

      db::hier_clusters<db::PolygonRef> hc;
      db::Connectivity conn;
      conn.connect (deep_layer ());
      hc.set_base_verbosity (base_verbosity () + 10);
      hc.build (layout, deep_layer ().initial_cell (), conn);

      //  collect the clusters and merge them into big polygons
      //  NOTE: using the ClusterMerger we merge bottom-up forming bigger and bigger polygons. This is
      //  hopefully more efficient that collecting everything and will lead to reuse of parts.

      ClusterMerger cm (deep_layer ().layer (), layout, hc, min_coherence (), report_progress (), progress_desc ());
      cm.set_base_verbosity (base_verbosity () + 10);

      //  TODO: iterate only over the called cells?
      for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
        const db::connected_clusters<db::PolygonRef> &cc = hc.clusters_per_cell (c->cell_index ());
        for (db::connected_clusters<db::PolygonRef>::all_iterator cl = cc.begin_all (); ! cl.at_end (); ++cl) {
          if (cc.is_root (*cl)) {
            db::Shapes &s = cm.merged (*cl, c->cell_index ());
            c->shapes (m_merged_polygons.layer ()).insert (s);
            cm.erase (*cl, c->cell_index ()); //  not needed anymore
          }
        }
      }

    }

    m_merged_polygons_valid = true;

  }
}

void
DeepRegion::set_is_merged (bool f)
{
  m_is_merged = f;
  m_merged_polygons_valid = false;
}

void
DeepRegion::insert_into (db::Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  deep_layer ().insert_into (layout, into_cell, into_layer);
}

RegionDelegate *
DeepRegion::and_with (const Region &other) const
{
  const DeepRegion *other_deep = dynamic_cast <const DeepRegion *> (other.delegate ());

  if (empty ()) {

    return clone ();

  } else if (other.empty ()) {

    return other.delegate ()->clone ();

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

  if (empty () || other.empty ()) {

    return clone ();

  } else if (! other_deep) {

    return AsIfFlatRegion::not_with (other);

  } else {

    return new DeepRegion (and_or_not_with (other_deep, false));

  }
}

std::pair<RegionDelegate *, RegionDelegate *>
DeepRegion::andnot_with (const Region &other) const
{
  const DeepRegion *other_deep = dynamic_cast <const DeepRegion *> (other.delegate ());

  if (empty ()) {

    return std::make_pair (clone (), clone ());

  } else if (other.empty ()) {

    return std::make_pair (other.delegate ()->clone (), clone ());

  } else if (! other_deep) {

    return AsIfFlatRegion::andnot_with (other);

  } else {

    std::pair<DeepLayer, DeepLayer> res = and_and_not_with (other_deep);
    return std::make_pair (new DeepRegion (res.first), new DeepRegion (res.second));

  }
}

DeepLayer
DeepRegion::and_or_not_with (const DeepRegion *other, bool and_op) const
{
  DeepLayer dl_out (deep_layer ().derived ());

  db::BoolAndOrNotLocalOperation op (and_op);

  db::local_processor<db::PolygonRef, db::PolygonRef, db::PolygonRef> proc (const_cast<db::Layout *> (&deep_layer ().layout ()), const_cast<db::Cell *> (&deep_layer ().initial_cell ()), &other->deep_layer ().layout (), &other->deep_layer ().initial_cell (), deep_layer ().breakout_cells (), other->deep_layer ().breakout_cells ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());
  proc.set_threads (deep_layer ().store ()->threads ());
  proc.set_area_ratio (deep_layer ().store ()->max_area_ratio ());
  proc.set_max_vertex_count (deep_layer ().store ()->max_vertex_count ());

  proc.run (&op, deep_layer ().layer (), other->deep_layer ().layer (), dl_out.layer ());

  return dl_out;
}

std::pair<DeepLayer, DeepLayer>
DeepRegion::and_and_not_with (const DeepRegion *other) const
{
  DeepLayer dl_out1 (deep_layer ().derived ());
  DeepLayer dl_out2 (deep_layer ().derived ());

  db::TwoBoolAndNotLocalOperation op;

  db::local_processor<db::PolygonRef, db::PolygonRef, db::PolygonRef> proc (const_cast<db::Layout *> (&deep_layer ().layout ()), const_cast<db::Cell *> (&deep_layer ().initial_cell ()), &other->deep_layer ().layout (), &other->deep_layer ().initial_cell (), deep_layer ().breakout_cells (), other->deep_layer ().breakout_cells ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());
  proc.set_threads (deep_layer ().store ()->threads ());
  proc.set_area_ratio (deep_layer ().store ()->max_area_ratio ());
  proc.set_max_vertex_count (deep_layer ().store ()->max_vertex_count ());

  std::vector<unsigned int> il;
  il.push_back (other->deep_layer ().layer ());

  std::vector<unsigned int> ol;
  ol.push_back (dl_out1.layer ());
  ol.push_back (dl_out2.layer ());

  proc.run (&op, deep_layer ().layer (), il, ol);

  return std::make_pair (dl_out1, dl_out2);
}

RegionDelegate *
DeepRegion::xor_with (const Region &other) const
{
  const DeepRegion *other_deep = dynamic_cast <const DeepRegion *> (other.delegate ());

  if (empty ()) {

    //  Nothing to do
    return other.delegate ()->clone ();

  } else if (other.empty ()) {

    //  Nothing to do
    return clone ();

  } else if (! other_deep) {

    return AsIfFlatRegion::xor_with (other);

  } else {

    //  Implement XOR as (A-B)+(B-A) - only this implementation
    //  is compatible with the local processor scheme
    DeepLayer n1 (and_or_not_with (other_deep, false));
    DeepLayer n2 (other_deep->and_or_not_with (this, false));

    n1.add_from (n2);
    return new DeepRegion (n1);

  }
}

RegionDelegate *
DeepRegion::add_in_place (const Region &other)
{
  if (other.empty ()) {
    return this;
  }

  const DeepRegion *other_deep = dynamic_cast <const DeepRegion *> (other.delegate ());
  if (other_deep) {

    deep_layer ().add_from (other_deep->deep_layer ());

  } else {

    //  non-deep to deep merge (flat)

    db::Shapes &shapes = deep_layer ().initial_cell ().shapes (deep_layer ().layer ());
    db::PolygonRefToShapesGenerator pr (const_cast<db::Layout *> (& deep_layer ().layout ()), &shapes);
    for (db::Region::const_iterator p = other.begin (); ! p.at_end (); ++p) {
      pr.put (*p);
    }

  }

  set_is_merged (false);
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

static int is_box_from_iter (db::RecursiveShapeIterator i)
{
  if (i.at_end ()) {
    return true;
  }

  if (i->is_box ()) {
    ++i;
    if (i.at_end ()) {
      return true;
    }
  } else if (i->is_path () || i->is_polygon ()) {
    db::Polygon poly;
    i->polygon (poly);
    if (poly.is_box ()) {
      ++i;
      if (i.at_end ()) {
        return true;
      }
    }
  }

  return false;
}

bool
DeepRegion::is_box () const
{
  return is_box_from_iter (begin_iter ().first);
}

size_t
DeepRegion::count () const
{
  size_t n = 0;

  const db::Layout &layout = deep_layer ().layout ();
  db::CellCounter cc (&layout);
  for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
    n += cc.weight (*c) * layout.cell (*c).shapes (deep_layer ().layer ()).size ();
  }

  return n;
}

size_t
DeepRegion::hier_count () const
{
  size_t n = 0;

  const db::Layout &layout = deep_layer ().layout ();
  for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
    n += layout.cell (*c).shapes (deep_layer ().layer ()).size ();
  }

  return n;
}

DeepRegion::area_type
DeepRegion::area (const db::Box &box) const
{
  if (box.empty ()) {

    const db::DeepLayer &polygons = merged_deep_layer ();

    db::cell_variants_collector<db::MagnificationReducer> vars;
    vars.collect (polygons.layout (), polygons.initial_cell ());

    DeepRegion::area_type a = 0;

    const db::Layout &layout = polygons.layout ();
    for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
      DeepRegion::area_type ac = 0;
      for (db::ShapeIterator s = layout.cell (*c).shapes (polygons.layer ()).begin (db::ShapeIterator::All); ! s.at_end (); ++s) {
        ac += s->area ();
      }
      const std::map<db::ICplxTrans, size_t> &vv = vars.variants (*c);
      for (std::map<db::ICplxTrans, size_t>::const_iterator v = vv.begin (); v != vv.end (); ++v) {
        double mag = v->first.mag ();
        a += v->second * ac * mag * mag;
      }
    }

    return a;

  } else {
    //  In the clipped case fall back to flat mode
    return db::AsIfFlatRegion::area (box);
  }
}

DeepRegion::perimeter_type
DeepRegion::perimeter (const db::Box &box) const
{
  if (box.empty ()) {

    const db::DeepLayer &polygons = merged_deep_layer ();

    db::cell_variants_collector<db::MagnificationReducer> vars;
    vars.collect (polygons.layout (), polygons.initial_cell ());

    DeepRegion::perimeter_type p = 0;

    const db::Layout &layout = polygons.layout ();
    for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
      DeepRegion::perimeter_type pc = 0;
      for (db::ShapeIterator s = layout.cell (*c).shapes (polygons.layer ()).begin (db::ShapeIterator::All); ! s.at_end (); ++s) {
        pc += s->perimeter ();
      }
      const std::map<db::ICplxTrans, size_t> &vv = vars.variants (*c);
      for (std::map<db::ICplxTrans, size_t>::const_iterator v = vv.begin (); v != vv.end (); ++v) {
        double mag = v->first.mag ();
        p += v->second * pc * mag;
      }
    }

    return p;

  } else {
    //  In the clipped case fall back to flat mode
    return db::AsIfFlatRegion::perimeter (box);
  }
}

Box
DeepRegion::bbox () const
{
  return deep_layer ().initial_cell ().bbox (deep_layer ().layer ());
}

std::string
DeepRegion::to_string (size_t nmax) const
{
  return db::AsIfFlatRegion::to_string (nmax);
}

EdgePairsDelegate *
DeepRegion::grid_check (db::Coord gx, db::Coord gy) const
{
  if (gx < 0 || gy < 0) {
    throw tl::Exception (tl::to_string (tr ("Grid check requires a positive grid value")));
  }

  if (gx != gy) {
    //  no way doing this hierarchically ?
    return db::AsIfFlatRegion::grid_check (gx, gy);
  }

  if (gx == 0) {
    return new EmptyEdgePairs ();
  }

  const db::DeepLayer &polygons = merged_deep_layer ();
  db::Layout &layout = const_cast<db::Layout &> (polygons.layout ());

  db::cell_variants_collector<db::GridReducer> vars (gx);
  vars.collect (layout, polygons.initial_cell ());

  std::map<db::cell_index_type, std::map<db::ICplxTrans, db::Shapes> > to_commit;
  std::unique_ptr<db::DeepEdgePairs> res (new db::DeepEdgePairs (polygons.derived ()));

  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

    const db::Shapes &shapes = c->shapes (polygons.layer ());

    const std::map<db::ICplxTrans, size_t> &vv = vars.variants (c->cell_index ());
    for (std::map<db::ICplxTrans, size_t>::const_iterator v = vv.begin (); v != vv.end (); ++v) {

      db::Shapes *markers;
      if (vv.size () == 1) {
        markers = & c->shapes (res->deep_layer ().layer ());
      } else {
        markers = & to_commit [c->cell_index ()] [v->first];
      }

      for (db::Shapes::shape_iterator si = shapes.begin (db::ShapeIterator::All); ! si.at_end (); ++si) {

        db::Polygon poly;
        si->polygon (poly);

        AsIfFlatRegion::produce_markers_for_grid_check (poly, v->first, gx, gy, *markers);

      }

    }

  }

  //  propagate the markers with a similar algorithm used for producing the variants
  res->deep_layer ().commit_shapes (vars, to_commit);

  return res.release ();
}

EdgePairsDelegate *
DeepRegion::angle_check (double min, double max, bool inverse) const
{
  const db::DeepLayer &polygons = merged_deep_layer ();
  db::Layout &layout = const_cast<db::Layout &> (polygons.layout ());

  std::unique_ptr<db::DeepEdgePairs> res (new db::DeepEdgePairs (polygons.derived ()));

  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

    const db::Shapes &shapes = c->shapes (polygons.layer ());
    db::Shapes &markers = c->shapes (res->deep_layer ().layer ());

    for (db::Shapes::shape_iterator si = shapes.begin (db::ShapeIterator::All); ! si.at_end (); ++si) {
      db::Polygon poly;
      si->polygon (poly);
      AsIfFlatRegion::produce_markers_for_angle_check (poly, db::UnitTrans (), min, max, inverse, markers);
    }

  }

  return res.release ();
}

RegionDelegate *
DeepRegion::snapped (db::Coord gx, db::Coord gy)
{
  if (gx < 0 || gy < 0) {
    throw tl::Exception (tl::to_string (tr ("Snapping requires a positive grid value")));
  }

  if (gx != gy) {
    //  no way doing this hierarchically ?
    return db::AsIfFlatRegion::snapped (gx, gy);
  }

  if (! gx) {
    return this;
  }

  const db::DeepLayer &polygons = merged_deep_layer ();

  db::cell_variants_collector<db::GridReducer> vars (gx);

  vars.collect (polygons.layout (), polygons.initial_cell ());

  //  NOTE: m_merged_polygons is mutable, so why is the const_cast needed?
  const_cast<db::DeepLayer &> (polygons).separate_variants (vars);

  db::Layout &layout = const_cast<db::Layout &> (polygons.layout ());
  std::vector<db::Point> heap;

  std::unique_ptr<db::DeepRegion> res (new db::DeepRegion (polygons.derived ()));
  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

    const std::map<db::ICplxTrans, size_t> &v = vars.variants (c->cell_index ());
    tl_assert (v.size () == size_t (1));
    const db::ICplxTrans &tr = v.begin ()->first;
    db::ICplxTrans trinv = tr.inverted ();

    const db::Shapes &s = c->shapes (polygons.layer ());
    db::Shapes &st = c->shapes (res->deep_layer ().layer ());
    db::PolygonRefToShapesGenerator pr (&layout, &st);

    for (db::Shapes::shape_iterator si = s.begin (db::ShapeIterator::All); ! si.at_end (); ++si) {

      db::Polygon poly;
      si->polygon (poly);
      poly.transform (tr);
      pr.put (snapped_polygon (poly, gx, gy, heap).transformed (trinv));

    }

  }

  return res.release ();
}

namespace
{

class PolygonToEdgeLocalOperation
  : public local_operation<db::PolygonRef, db::PolygonRef, db::Edge>
{
public:
  PolygonToEdgeLocalOperation ()
    : local_operation<db::PolygonRef, db::PolygonRef, db::Edge> ()
  {
    //  .. nothing yet ..
  }

  virtual db::Coord dist () const { return 1; }
  virtual bool requests_single_subjects () const { return true; }
  virtual std::string description () const { return std::string ("polygon to edges"); }

  virtual void do_compute_local (db::Layout * /*layout*/, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
  {
    db::EdgeProcessor ep;

    for (shape_interactions<db::PolygonRef, db::PolygonRef>::subject_iterator s = interactions.begin_subjects (); s != interactions.end_subjects (); ++s) {
      ep.insert (s->second);
    }

    if (interactions.num_intruders () == 0) {

      db::EdgeToEdgeSetGenerator eg (results.front ());
      db::MergeOp op (0);
      ep.process (eg, op);

    } else {

      //  With intruders: to compute our local contribution we take the edges without and with intruders
      //  and deliver what is in both sets

      db::MergeOp op (0);

      std::vector<Edge> edges1;
      db::EdgeContainer ec1 (edges1);
      ep.process (ec1, op);

      ep.clear ();

      for (shape_interactions<db::PolygonRef, db::PolygonRef>::subject_iterator s = interactions.begin_subjects (); s != interactions.end_subjects (); ++s) {
        ep.insert (s->second);
      }
      for (shape_interactions<db::PolygonRef, db::PolygonRef>::intruder_iterator i = interactions.begin_intruders (); i != interactions.end_intruders (); ++i) {
        ep.insert (i->second.second);
      }

      std::vector<Edge> edges2;
      db::EdgeContainer ec2 (edges2);
      ep.process (ec2, op);

      //  Runs the boolean AND between the result with and without intruders

      db::box_scanner<db::Edge, size_t> scanner;
      scanner.reserve (edges1.size () + edges2.size ());

      for (std::vector<Edge>::const_iterator i = edges1.begin (); i != edges1.end (); ++i) {
        scanner.insert (i.operator-> (), 0);
      }
      for (std::vector<Edge>::const_iterator i = edges2.begin (); i != edges2.end (); ++i) {
        scanner.insert (i.operator-> (), 1);
      }

      EdgeBooleanClusterCollector<std::unordered_set<db::Edge> > cluster_collector (&results.front (), EdgeAnd);
      scanner.process (cluster_collector, 1, db::box_convert<db::Edge> ());

    }

  }
};

}

EdgesDelegate *
DeepRegion::edges (const EdgeFilterBase *filter) const
{
  if (! filter && merged_semantics ()) {

    //  Hierarchical edge detector - no pre-merge required

    const db::DeepLayer &polygons = deep_layer ();

    db::PolygonToEdgeLocalOperation op;

    db::local_processor<db::PolygonRef, db::PolygonRef, db::Edge> proc (const_cast<db::Layout *> (&polygons.layout ()),
                                                                        const_cast<db::Cell *> (&polygons.initial_cell ()),
                                                                        polygons.breakout_cells ());

    proc.set_description (progress_desc ());
    proc.set_report_progress (report_progress ());
    proc.set_base_verbosity (base_verbosity ());
    proc.set_threads (polygons.store ()->threads ());

    //  a boolean core makes somewhat better hierarchy
    proc.set_boolean_core (true);

    std::unique_ptr<db::DeepEdges> res (new db::DeepEdges (polygons.derived ()));

    proc.run (&op, polygons.layer (), foreign_idlayer (), res->deep_layer ().layer ());

    return res.release ();

  } else {

    const db::DeepLayer &polygons = merged_deep_layer ();

    std::unique_ptr<VariantsCollectorBase> vars;

    if (filter && filter->vars ()) {

      vars.reset (new db::VariantsCollectorBase (filter->vars ()));

      vars->collect (polygons.layout (), polygons.initial_cell ());

      //  NOTE: m_merged_polygons is mutable, so why is the const_cast needed?
      const_cast<db::DeepLayer &> (polygons).separate_variants (*vars);

    }

    db::Layout &layout = const_cast<db::Layout &> (polygons.layout ());

    std::unique_ptr<db::DeepEdges> res (new db::DeepEdges (polygons.derived ()));
    for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

      db::ICplxTrans tr;
      if (vars.get ()) {
        const std::map<db::ICplxTrans, size_t> &v = vars->variants (c->cell_index ());
        tl_assert (v.size () == size_t (1));
        tr = v.begin ()->first;
      }

      const db::Shapes &s = c->shapes (polygons.layer ());
      db::Shapes &st = c->shapes (res->deep_layer ().layer ());

      for (db::Shapes::shape_iterator si = s.begin (db::ShapeIterator::All); ! si.at_end (); ++si) {

        db::Polygon poly;
        si->polygon (poly);

        for (db::Polygon::polygon_edge_iterator e = poly.begin_edge (); ! e.at_end (); ++e) {
          if (! filter || filter->selected ((*e).transformed (tr))) {
            st.insert (*e);
          }
        }

      }

    }

    res->set_is_merged (merged_semantics () || is_merged ());
    return res.release ();

  }
}

RegionDelegate *
DeepRegion::process_in_place (const PolygonProcessorBase &filter)
{
  //  TODO: implement to be really in-place
  return processed (filter);
}

EdgesDelegate *
DeepRegion::processed_to_edges (const PolygonToEdgeProcessorBase &filter) const
{
  return shape_collection_processed_impl<db::Polygon, db::Edge, db::DeepEdges> (filter.requires_raw_input () ? deep_layer () : merged_deep_layer (), filter);
}

EdgePairsDelegate *
DeepRegion::processed_to_edge_pairs (const PolygonToEdgePairProcessorBase &filter) const
{
  return shape_collection_processed_impl<db::Polygon, db::EdgePair, db::DeepEdgePairs> (filter.requires_raw_input () ? deep_layer () : merged_deep_layer (), filter);
}

RegionDelegate *
DeepRegion::processed (const PolygonProcessorBase &filter) const
{
  return shape_collection_processed_impl<db::Polygon, db::Polygon, db::DeepRegion> (filter.requires_raw_input () ? deep_layer () : merged_deep_layer (), filter);
}

RegionDelegate *
DeepRegion::filter_in_place (const PolygonFilterBase &filter)
{
  //  TODO: implement to be really in-place
  *this = *apply_filter (filter);
  return this;
}

RegionDelegate *
DeepRegion::filtered (const PolygonFilterBase &filter) const
{
  return apply_filter (filter);
}

DeepRegion *
DeepRegion::apply_filter (const PolygonFilterBase &filter) const
{
  const db::DeepLayer &polygons = filter.requires_raw_input () ? deep_layer () : merged_deep_layer ();

  std::unique_ptr<VariantsCollectorBase> vars;
  if (filter.vars ()) {

    vars.reset (new db::VariantsCollectorBase (filter.vars ()));

    vars->collect (polygons.layout (), polygons.initial_cell ());

    if (filter.wants_variants ()) {
      const_cast<db::DeepLayer &> (polygons).separate_variants (*vars);
    }

  }

  db::Layout &layout = const_cast<db::Layout &> (polygons.layout ());
  std::map<db::cell_index_type, std::map<db::ICplxTrans, db::Shapes> > to_commit;

  std::unique_ptr<db::DeepRegion> res (new db::DeepRegion (polygons.derived ()));
  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

    const db::Shapes &s = c->shapes (polygons.layer ());

    if (vars.get ()) {

      const std::map<db::ICplxTrans, size_t> &vv = vars->variants (c->cell_index ());
      for (std::map<db::ICplxTrans, size_t>::const_iterator v = vv.begin (); v != vv.end (); ++v) {

        db::Shapes *st;
        if (vv.size () == 1) {
          st = & c->shapes (res->deep_layer ().layer ());
        } else {
          st = & to_commit [c->cell_index ()] [v->first];
        }

        const db::ICplxTrans &tr = v->first;

        for (db::Shapes::shape_iterator si = s.begin (db::ShapeIterator::All); ! si.at_end (); ++si) {
          db::Polygon poly;
          si->polygon (poly);
          if (filter.selected (poly.transformed (tr))) {
            st->insert (*si);
          }
        }

      }

    } else {

      db::Shapes &st = c->shapes (res->deep_layer ().layer ());

      for (db::Shapes::shape_iterator si = s.begin (db::ShapeIterator::All); ! si.at_end (); ++si) {
        db::Polygon poly;
        si->polygon (poly);
        if (filter.selected (poly)) {
          st.insert (*si);
        }
      }

    }

  }

  if (! to_commit.empty () && vars.get ()) {
    res->deep_layer ().commit_shapes (*vars, to_commit);
  }

  if (! filter.requires_raw_input ()) {
    res->set_is_merged (true);
  }
  return res.release ();
}

RegionDelegate *
DeepRegion::merged_in_place ()
{
  ensure_merged_polygons_valid ();

  //  NOTE: this makes both layers share the same resource
  set_deep_layer (m_merged_polygons);

  set_is_merged (true);
  return this;
}

RegionDelegate *
DeepRegion::merged_in_place (bool min_coherence, unsigned int min_wc)
{
  return merged (min_coherence, min_wc);
}

RegionDelegate *
DeepRegion::merged () const
{
  ensure_merged_polygons_valid ();

  db::Layout &layout = const_cast<db::Layout &> (m_merged_polygons.layout ());

  std::unique_ptr<db::DeepRegion> res (new db::DeepRegion (m_merged_polygons.derived ()));
  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
    c->shapes (res->deep_layer ().layer ()) = c->shapes (m_merged_polygons.layer ());
  }

  res->deep_layer ().layer ();

  res->set_is_merged (true);
  return res.release ();
}

RegionDelegate *
DeepRegion::merged (bool min_coherence, unsigned int min_wc) const
{
  tl::SelfTimer timer (tl::verbosity () > base_verbosity (), "Ensure merged polygons");

  db::Layout &layout = const_cast<db::Layout &> (deep_layer ().layout ());

  db::hier_clusters<db::PolygonRef> hc;
  db::Connectivity conn;
  conn.connect (deep_layer ());
  hc.set_base_verbosity (base_verbosity () + 10);
  hc.build (layout, deep_layer ().initial_cell (), conn);

  //  collect the clusters and merge them into big polygons
  //  NOTE: using the ClusterMerger we merge bottom-up forming bigger and bigger polygons. This is
  //  hopefully more efficient that collecting everything and will lead to reuse of parts.

  DeepLayer dl_out (deep_layer ().derived ());

  ClusterMerger cm (deep_layer ().layer (), layout, hc, min_coherence, report_progress (), progress_desc ());
  cm.set_base_verbosity (base_verbosity () + 10);

  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
    const db::connected_clusters<db::PolygonRef> &cc = hc.clusters_per_cell (c->cell_index ());
    for (db::connected_clusters<db::PolygonRef>::all_iterator cl = cc.begin_all (); ! cl.at_end (); ++cl) {
      if (cc.is_root (*cl)) {
        db::Shapes &s = cm.merged (*cl, c->cell_index (), min_wc);
        c->shapes (dl_out.layer ()).insert (s);
        cm.erase (*cl, c->cell_index ()); //  not needed anymore
      }
    }
  }

  db::DeepRegion *res = new db::DeepRegion (dl_out);
  res->set_is_merged (true);
  return res;
}

RegionDelegate *
DeepRegion::sized (coord_type d, unsigned int mode) const
{
  if (empty ()) {
    //  Nothing to do - NOTE: don't return EmptyRegion because we want to
    //  maintain "deepness"
    return clone ();
  }

  const db::DeepLayer &polygons = merged_deep_layer ();

  db::Layout &layout = const_cast<db::Layout &> (polygons.layout ());

  db::cell_variants_collector<db::MagnificationReducer> vars;
  vars.collect (polygons.layout (), polygons.initial_cell ());

  //  NOTE: m_merged_polygons is mutable, so why is the const_cast needed?
  const_cast<db::DeepLayer &> (polygons).separate_variants (vars);

  std::unique_ptr<db::DeepRegion> res (new db::DeepRegion (polygons.derived ()));
  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

    const std::map<db::ICplxTrans, size_t> &v = vars.variants (c->cell_index ());
    tl_assert (v.size () == size_t (1));
    double mag = v.begin ()->first.mag ();
    db::Coord d_with_mag = db::coord_traits<db::Coord>::rounded (d / mag);

    const db::Shapes &s = c->shapes (polygons.layer ());
    db::Shapes &st = c->shapes (res->deep_layer ().layer ());

    db::PolygonRefToShapesGenerator pr (&layout, &st);
    db::PolygonGenerator pg2 (pr, false /*don't resolve holes*/, true /*min. coherence*/);
    db::SizingPolygonFilter siz (pg2, d_with_mag, d_with_mag, mode);

    for (db::Shapes::shape_iterator si = s.begin (db::ShapeIterator::All); ! si.at_end (); ++si) {
      db::Polygon poly;
      si->polygon (poly);
      siz.put (poly);
    }

  }

  //  in case of negative sizing the output polygons will still be merged (on positive sizing they might
  //  overlap after size and are not necessarily merged)
  if (d < 0 && (merged_semantics () || is_merged ())) {
    res->set_is_merged (true);
  }

  return res.release ();
}

RegionDelegate *
DeepRegion::sized (coord_type dx, coord_type dy, unsigned int mode) const
{
  if (empty ()) {
    //  Nothing to do - NOTE: don't return EmptyRegion because we want to
    //  maintain "deepness"
    return clone ();
  }

  if (dx == dy) {
    return sized (dx, mode);
  }

  const db::DeepLayer &polygons = merged_deep_layer ();

  db::Layout &layout = const_cast<db::Layout &> (polygons.layout ());

  db::cell_variants_collector<db::XYAnisotropyAndMagnificationReducer> vars;
  vars.collect (polygons.layout (), polygons.initial_cell ());

  //  NOTE: m_merged_polygons is mutable, so why is the const_cast needed?
  const_cast<db::DeepLayer &> (polygons).separate_variants (vars);

  std::unique_ptr<db::DeepRegion> res (new db::DeepRegion (polygons.derived ()));
  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

    const std::map<db::ICplxTrans, size_t> &v = vars.variants (c->cell_index ());
    tl_assert (v.size () == size_t (1));
    double mag = v.begin ()->first.mag ();
    double angle = v.begin ()->first.angle ();

    db::Coord dx_with_mag = db::coord_traits<db::Coord>::rounded (dx / mag);
    db::Coord dy_with_mag = db::coord_traits<db::Coord>::rounded (dy / mag);
    if (fabs (angle - 90.0) < 45.0) {
      //  TODO: how to handle x/y swapping on arbitrary angles?
      std::swap (dx_with_mag, dy_with_mag);
    }

    const db::Shapes &s = c->shapes (polygons.layer ());
    db::Shapes &st = c->shapes (res->deep_layer ().layer ());

    db::PolygonRefToShapesGenerator pr (&layout, &st);
    db::PolygonGenerator pg2 (pr, false /*don't resolve holes*/, true /*min. coherence*/);
    db::SizingPolygonFilter siz (pg2, dx_with_mag, dy_with_mag, mode);

    for (db::Shapes::shape_iterator si = s.begin (db::ShapeIterator::All); ! si.at_end (); ++si) {
      db::Polygon poly;
      si->polygon (poly);
      siz.put (poly);
    }

  }

  //  in case of negative sizing the output polygons will still be merged (on positive sizing they might
  //  overlap after size and are not necessarily merged)
  if (dx < 0 && dy < 0 && (merged_semantics () || is_merged ())) {
    res->set_is_merged (true);
  }

  return res.release ();
}

RegionDelegate *
DeepRegion::in (const Region &other, bool invert) const
{
  //  TODO: this can be optimized maybe ...
  return db::AsIfFlatRegion::in (other, invert);
}

template <class TR, class Output>
static
Output *region_cop_impl (DeepRegion *region, db::CompoundRegionOperationNode &node)
{
  //  Fall back to flat mode if one of the inputs is flat
  std::vector<db::Region *> inputs = node.inputs ();
  for (std::vector<db::Region *>::const_iterator i = inputs.begin (); i != inputs.end (); ++i) {
    if (! is_subject_regionptr (*i) && ! dynamic_cast<const db::DeepRegion *> ((*i)->delegate ())) {
      return 0;
    }
  }

  db::local_processor<db::PolygonRef, db::PolygonRef, TR> proc (const_cast<db::Layout *> (&region->deep_layer ().layout ()),
                                                                const_cast<db::Cell *> (&region->deep_layer ().initial_cell ()),
                                                                region->deep_layer ().breakout_cells ());

  proc.set_description (region->progress_desc ());
  proc.set_report_progress (region->report_progress ());
  proc.set_base_verbosity (region->base_verbosity ());
  proc.set_threads (region->deep_layer ().store ()->threads ());

  const db::DeepLayer &polygons (region->merged_deep_layer ());

  std::vector<unsigned int> other_layers;
  for (std::vector<db::Region *>::const_iterator i = inputs.begin (); i != inputs.end (); ++i) {

    if (is_subject_regionptr (*i)) {
      if (*i == subject_regionptr ()) {
        other_layers.push_back (subject_idlayer ());
      } else {
        other_layers.push_back (foreign_idlayer ());
      }
    } else {
      const db::DeepRegion *other_deep = dynamic_cast<const db::DeepRegion *> ((*i)->delegate ());
      tl_assert (other_deep != 0);
      if (&other_deep->deep_layer ().layout () != &region->deep_layer ().layout () || &other_deep->deep_layer ().initial_cell () != &region->deep_layer ().initial_cell ()) {
        throw tl::Exception (tl::to_string (tr ("Complex DeepRegion operations need to use the same layout and top cell for all inputs")));
      }
      other_layers.push_back (other_deep->deep_layer ().layer ());
    }

  }

  std::unique_ptr<Output> res (new Output (polygons.derived ()));
  compound_local_operation<db::PolygonRef, db::PolygonRef, TR> op (&node);
  proc.run (&op, polygons.layer (), other_layers, res->deep_layer ().layer ());

  return res.release ();
}

EdgePairsDelegate *
DeepRegion::cop_to_edge_pairs (db::CompoundRegionOperationNode &node)
{
  DeepEdgePairs *output = region_cop_impl<db::EdgePair, DeepEdgePairs> (this, node);
  if (! output) {
    return AsIfFlatRegion::cop_to_edge_pairs (node);
  } else {
    return output;
  }
}

RegionDelegate *
DeepRegion::cop_to_region (db::CompoundRegionOperationNode &node)
{
  DeepRegion *output = region_cop_impl<db::PolygonRef, db::DeepRegion> (this, node);
  if (! output) {
    return AsIfFlatRegion::cop_to_region (node);
  } else {
    return output;
  }
}

EdgesDelegate *
DeepRegion::cop_to_edges (db::CompoundRegionOperationNode &node)
{
  DeepEdges *output = region_cop_impl<db::Edge, db::DeepEdges> (this, node);
  if (! output) {
    return AsIfFlatRegion::cop_to_edges (node);
  } else {
    return output;
  }
}

EdgePairsDelegate *
DeepRegion::run_check (db::edge_relation_type rel, bool different_polygons, const Region *other, db::Coord d, const RegionCheckOptions &options) const
{
  const db::DeepRegion *other_deep = 0;
  unsigned int other_layer = 0;
  bool other_is_merged = true;

  if (other == subject_regionptr ()) {
    other_layer = subject_idlayer ();
  } else if (other == foreign_regionptr ()) {
    other_layer = foreign_idlayer ();
  } else {
    other_deep = dynamic_cast<const db::DeepRegion *> (other->delegate ());
    if (! other_deep) {
      return db::AsIfFlatRegion::run_check (rel, different_polygons, other, d, options);
    }
    other_layer = other_deep->deep_layer ().layer ();
    other_is_merged = other->is_merged ();
  }

  const db::DeepLayer &polygons = merged_deep_layer ();

  EdgeRelationFilter check (rel, d, options.metrics);
  check.set_include_zero (false);
  check.set_whole_edges (options.whole_edges);
  check.set_ignore_angle (options.ignore_angle);
  check.set_min_projection (options.min_projection);
  check.set_max_projection (options.max_projection);

  std::unique_ptr<db::DeepEdgePairs> res (new db::DeepEdgePairs (polygons.derived ()));

  db::CheckLocalOperation op (check, different_polygons, other_deep != 0, other_is_merged, options);

  db::local_processor<db::PolygonRef, db::PolygonRef, db::EdgePair> proc (const_cast<db::Layout *> (&polygons.layout ()),
                                                                          const_cast<db::Cell *> (&polygons.initial_cell ()),
                                                                          other_deep ? &other_deep->deep_layer ().layout () : const_cast<db::Layout *> (&polygons.layout ()),
                                                                          other_deep ? &other_deep->deep_layer ().initial_cell () : const_cast<db::Cell *> (&polygons.initial_cell ()),
                                                                          deep_layer ().breakout_cells (),
                                                                          other_deep ? other_deep->deep_layer ().breakout_cells () : 0);

  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (polygons.store ()->threads ());

  proc.run (&op, polygons.layer (), other_layer, res->deep_layer ().layer ());

  return res.release ();
}

EdgePairsDelegate *
DeepRegion::run_single_polygon_check (db::edge_relation_type rel, db::Coord d, const RegionCheckOptions &options) const
{
  const db::DeepLayer &polygons = merged_deep_layer ();

  EdgeRelationFilter check (rel, d, options.metrics);
  check.set_include_zero (false);
  check.set_whole_edges (options.whole_edges);
  check.set_ignore_angle (options.ignore_angle);
  check.set_min_projection (options.min_projection);
  check.set_max_projection (options.max_projection);

  db::Layout &layout = const_cast<db::Layout &> (polygons.layout ());

  std::unique_ptr<db::DeepEdgePairs> res (new db::DeepEdgePairs (polygons.derived ()));
  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

    const db::Shapes &shapes = c->shapes (polygons.layer ());
    db::Shapes &result = c->shapes (res->deep_layer ().layer ());

    for (db::Shapes::shape_iterator s = shapes.begin (db::ShapeIterator::Polygons); ! s.at_end (); ++s) {

      edge2edge_check_negative_or_positive<db::Shapes> edge_check (check, result, options.negative, false /*does not require different polygons*/, false /*does not require different layers*/, options.shielded, true /*symmetric edge pairs*/);
      poly2poly_check<db::Polygon> poly_check (edge_check);

      db::Polygon poly;
      s->polygon (poly);

      do {
        poly_check.enter (poly, 0);
      } while (edge_check.prepare_next_pass ());

    }

  }

  return res.release ();
}

namespace
{

class InteractingResultHolder
{
public:
  InteractingResultHolder (InteractingOutputMode output_mode, bool is_merged, const db::DeepLayer &polygons)
    : m_output_mode (output_mode), m_is_merged (is_merged)
  {
    if (m_output_mode == Positive || m_output_mode == Negative) {
      m_dl1 = db::DeepLayer (polygons.derived ());
    } else if (m_output_mode == PositiveAndNegative) {
      m_dl1 = db::DeepLayer (polygons.derived ());
      m_dl2 = db::DeepLayer (polygons.derived ());
    }
  }

  std::vector<unsigned int> layers () const
  {
    std::vector<unsigned int> l;
    if (m_output_mode == Positive || m_output_mode == Negative) {
      l.push_back (m_dl1.layer ());
    } else if (m_output_mode == PositiveAndNegative) {
      l.push_back (m_dl1.layer ());
      l.push_back (m_dl2.layer ());
    }
    return l;
  }

  std::pair<RegionDelegate *, RegionDelegate *> result_pair ()
  {
    if (m_output_mode == Positive || m_output_mode == Negative) {
      db::DeepRegion *res = new db::DeepRegion (m_dl1);
      res->set_is_merged (m_is_merged);
      return std::pair<RegionDelegate *, RegionDelegate *> (res, 0);
    } else if (m_output_mode == PositiveAndNegative) {
      db::DeepRegion *res1 = new db::DeepRegion (m_dl1);
      res1->set_is_merged (m_is_merged);
      db::DeepRegion *res2 = new db::DeepRegion (m_dl2);
      res2->set_is_merged (m_is_merged);
      return std::pair<RegionDelegate *, RegionDelegate *> (res1, res2);
    } else {
      return std::pair<RegionDelegate *, RegionDelegate *> (0, 0);
    }
  }

private:
  InteractingOutputMode m_output_mode;
  bool m_is_merged;
  DeepLayer m_dl1, m_dl2;
};

}

std::pair<RegionDelegate *, RegionDelegate *>
DeepRegion::selected_interacting_generic (const Region &other, int mode, bool touching, InteractingOutputMode output_mode, size_t min_count, size_t max_count) const
{
  if (output_mode == None) {
    return std::pair<RegionDelegate *, RegionDelegate *> (0, 0);
  }

  bool counting = !(min_count == 1 && max_count == std::numeric_limits<size_t>::max ());

  //  with these flag set to true, the resulting polygons are broken again.
  bool split_after = false;

  std::unique_ptr<db::DeepRegion> dr_holder;
  const db::DeepRegion *other_deep = dynamic_cast<const db::DeepRegion *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchisation
    dr_holder.reset (new db::DeepRegion (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  const db::DeepLayer &polygons = merged_deep_layer ();
  //  NOTE: on "inside" or with counting, the other polygons must be merged
  const db::DeepLayer &other_polygons = (mode < 0 || counting) ? other_deep->merged_deep_layer () : other_deep->deep_layer ();

  db::InteractingLocalOperation op (mode, touching, output_mode, min_count, max_count, true);

  db::local_processor<db::PolygonRef, db::PolygonRef, db::PolygonRef> proc (const_cast<db::Layout *> (&polygons.layout ()), const_cast<db::Cell *> (&polygons.initial_cell ()), &other_polygons.layout (), &other_polygons.initial_cell (), polygons.breakout_cells (), other_polygons.breakout_cells ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (polygons.store ()->threads ());
  if (split_after) {
    proc.set_area_ratio (polygons.store ()->max_area_ratio ());
    proc.set_max_vertex_count (polygons.store ()->max_vertex_count ());
  }

  bool result_is_merged = (! split_after && ((mode < 0 && other.merged_semantics ()) || other.is_merged ()) && (merged_semantics () || is_merged ()));
  InteractingResultHolder orh (output_mode, result_is_merged, polygons);

  proc.run (&op, polygons.layer (), other_polygons.layer (), orh.layers ());

  return orh.result_pair ();
}

std::pair<RegionDelegate *, RegionDelegate *>
DeepRegion::selected_interacting_generic (const Edges &other, InteractingOutputMode output_mode, size_t min_count, size_t max_count) const
{
  bool counting = !(min_count == 1 && max_count == std::numeric_limits<size_t>::max ());

  //  with these flag set to true, the resulting polygons are broken again.
  bool split_after = false;

  std::unique_ptr<db::DeepEdges> dr_holder;
  const db::DeepEdges *other_deep = dynamic_cast<const db::DeepEdges *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchisation
    dr_holder.reset (new db::DeepEdges (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  const db::DeepLayer &polygons = merged_deep_layer ();

  db::InteractingWithEdgeLocalOperation op (output_mode, min_count, max_count, true);

  db::local_processor<db::PolygonRef, db::Edge, db::PolygonRef> proc (const_cast<db::Layout *> (&polygons.layout ()), const_cast<db::Cell *> (&polygons.initial_cell ()), &other_deep->deep_layer ().layout (), &other_deep->deep_layer ().initial_cell (), polygons.breakout_cells (), other_deep->deep_layer ().breakout_cells ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (polygons.store ()->threads ());
  if (split_after) {
    proc.set_area_ratio (polygons.store ()->max_area_ratio ());
    proc.set_max_vertex_count (polygons.store ()->max_vertex_count ());
  }

  bool result_is_merged = (! split_after && other.is_merged () && (merged_semantics () || is_merged ()));
  InteractingResultHolder orh (output_mode, result_is_merged, polygons);

  proc.run (&op, polygons.layer (), counting ? other_deep->merged_deep_layer ().layer () : other_deep->deep_layer ().layer (), orh.layers ());

  return orh.result_pair ();
}

RegionDelegate *
DeepRegion::pull_generic (const Region &other, int mode, bool touching) const
{
  //  with these flag set to true, the resulting polygons are broken again.
  bool split_after = false;

  std::unique_ptr<db::DeepRegion> dr_holder;
  const db::DeepRegion *other_deep = dynamic_cast<const db::DeepRegion *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchisation
    dr_holder.reset (new db::DeepRegion (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  //  in "inside" mode, the first argument needs to be merged too
  const db::DeepLayer &polygons = mode < 0 ? merged_deep_layer () : deep_layer ();
  const db::DeepLayer &other_polygons = other_deep->merged_deep_layer ();

  DeepLayer dl_out (polygons.derived ());

  db::PullLocalOperation op (mode, touching);

  db::local_processor<db::PolygonRef, db::PolygonRef, db::PolygonRef> proc (const_cast<db::Layout *> (&polygons.layout ()), const_cast<db::Cell *> (&polygons.initial_cell ()), &other_polygons.layout (), &other_polygons.initial_cell (), polygons.breakout_cells (), other_polygons.breakout_cells ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (polygons.store ()->threads ());
  if (split_after) {
    proc.set_area_ratio (polygons.store ()->max_area_ratio ());
    proc.set_max_vertex_count (polygons.store ()->max_vertex_count ());
  }

  proc.run (&op, polygons.layer (), other_polygons.layer (), dl_out.layer ());

  db::DeepRegion *res = new db::DeepRegion (dl_out);
  if (! split_after && ((mode < 0 && merged_semantics ()) || is_merged ()) && (other.merged_semantics () || other.is_merged ())) {
    res->set_is_merged (true);
  }
  return res;
}

EdgesDelegate *
DeepRegion::pull_generic (const Edges &other) const
{
  std::unique_ptr<db::DeepEdges> dr_holder;
  const db::DeepEdges *other_deep = dynamic_cast<const db::DeepEdges *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchisation
    dr_holder.reset (new db::DeepEdges (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  //  in "inside" mode, the first argument needs to be merged too
  const db::DeepLayer &polygons = deep_layer ();
  const db::DeepLayer &other_edges = other_deep->merged_deep_layer ();

  DeepLayer dl_out (polygons.derived ());

  db::PullWithEdgeLocalOperation op;

  db::local_processor<db::PolygonRef, db::Edge, db::Edge> proc (const_cast<db::Layout *> (&polygons.layout ()), const_cast<db::Cell *> (&polygons.initial_cell ()), &other_edges.layout (), &other_edges.initial_cell (), polygons.breakout_cells (), other_edges.breakout_cells ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (polygons.store ()->threads ());
  proc.run (&op, polygons.layer (), other_edges.layer (), dl_out.layer ());

  db::DeepEdges *res = new db::DeepEdges (dl_out);
  res->set_is_merged (is_merged () && (other.merged_semantics () || other.is_merged ()));
  return res;
}

TextsDelegate *
DeepRegion::pull_generic (const Texts &other) const
{
  std::unique_ptr<db::DeepTexts> dr_holder;
  const db::DeepTexts *other_deep = dynamic_cast<const db::DeepTexts *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchisation
    dr_holder.reset (new db::DeepTexts (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  //  in "inside" mode, the first argument needs to be merged too
  const db::DeepLayer &polygons = deep_layer ();
  const db::DeepLayer &other_texts = other_deep->deep_layer ();

  DeepLayer dl_out (polygons.derived ());

  db::PullWithTextLocalOperation op;

  db::local_processor<db::PolygonRef, db::TextRef, db::TextRef> proc (const_cast<db::Layout *> (&polygons.layout ()), const_cast<db::Cell *> (&polygons.initial_cell ()), &other_texts.layout (), &other_texts.initial_cell (), polygons.breakout_cells (), other_texts.breakout_cells ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (polygons.store ()->threads ());
  proc.run (&op, polygons.layer (), other_texts.layer (), dl_out.layer ());

  db::DeepTexts *res = new db::DeepTexts (dl_out);
  res->set_is_merged (is_merged ());
  return res;
}


std::pair<RegionDelegate *, RegionDelegate *>
DeepRegion::selected_interacting_generic (const Texts &other, InteractingOutputMode output_mode, size_t min_count, size_t max_count) const
{
  //  with these flag set to true, the resulting polygons are broken again.
  bool split_after = false;

  std::unique_ptr<db::DeepTexts> dr_holder;
  const db::DeepTexts *other_deep = dynamic_cast<const db::DeepTexts *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchisation
    dr_holder.reset (new db::DeepTexts (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  const db::DeepLayer &polygons = merged_deep_layer ();

  db::InteractingWithTextLocalOperation op (output_mode, min_count, max_count);

  db::local_processor<db::PolygonRef, db::TextRef, db::PolygonRef> proc (const_cast<db::Layout *> (&polygons.layout ()), const_cast<db::Cell *> (&polygons.initial_cell ()), &other_deep->deep_layer ().layout (), &other_deep->deep_layer ().initial_cell (), polygons.breakout_cells (), other_deep->deep_layer ().breakout_cells ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (polygons.store ()->threads ());
  if (split_after) {
    proc.set_area_ratio (polygons.store ()->max_area_ratio ());
    proc.set_max_vertex_count (polygons.store ()->max_vertex_count ());
  }

  bool result_is_merged = (! split_after && (merged_semantics () || is_merged ()));
  InteractingResultHolder orh (output_mode, result_is_merged, polygons);

  proc.run (&op, polygons.layer (), other_deep->deep_layer ().layer (), orh.layers ());

  return orh.result_pair ();
}

}
