
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
#include "dbLocalOperationUtils.h"
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

// -------------------------------------------------------------------------------------------------------------
//  DeepRegion implementation

DeepRegion::DeepRegion (const RecursiveShapeIterator &si, DeepShapeStore &dss, double area_ratio, size_t max_vertex_count)
  : AsIfFlatRegion (), m_merged_polygons ()
{
  set_deep_layer (dss.create_polygon_layer (si, area_ratio, max_vertex_count));
  init ();
}

DeepRegion::DeepRegion (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans, bool merged_semantics, double area_ratio, size_t max_vertex_count)
  : AsIfFlatRegion (), m_merged_polygons ()
{
  set_deep_layer (dss.create_polygon_layer (si, area_ratio, max_vertex_count, trans));
  init ();
  set_merged_semantics (merged_semantics);
}

DeepRegion::DeepRegion (const db::Region &other, DeepShapeStore &dss)
  : AsIfFlatRegion (), m_merged_polygons ()
{
  set_deep_layer (dss.create_from_flat (other, false));

  init ();
  set_merged_semantics (other.merged_semantics ());
}

DeepRegion::DeepRegion ()
  : AsIfFlatRegion ()
{
  init ();
}

DeepRegion::DeepRegion (const DeepLayer &dl)
  : AsIfFlatRegion ()
{
  set_deep_layer (dl);
  init ();
}

DeepRegion::~DeepRegion ()
{
  //  .. nothing yet ..
}

DeepRegion::DeepRegion (const DeepRegion &other)
  : AsIfFlatRegion (other), DeepShapeCollectionDelegateBase (other),
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

DeepLayer
DeepRegion::and_or_not_with (const DeepRegion *other, bool and_op) const
{
  DeepLayer dl_out (deep_layer ().derived ());

  db::BoolAndOrNotLocalOperation op (and_op);

  db::local_processor<db::PolygonRef, db::PolygonRef, db::PolygonRef> proc (const_cast<db::Layout *> (&deep_layer ().layout ()), const_cast<db::Cell *> (&deep_layer ().initial_cell ()), &other->deep_layer ().layout (), &other->deep_layer ().initial_cell (), deep_layer ().breakout_cells (), other->deep_layer ().breakout_cells ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (deep_layer ().store ()->threads ());
  proc.set_area_ratio (deep_layer ().store ()->max_area_ratio ());
  proc.set_max_vertex_count (deep_layer ().store ()->max_vertex_count ());

  proc.run (&op, deep_layer ().layer (), other->deep_layer ().layer (), dl_out.layer ());

  return dl_out;
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
DeepRegion::size () const
{
  size_t n = 0;

  const db::Layout &layout = deep_layer ().layout ();
  db::CellCounter cc (&layout);
  for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
    n += cc.weight (*c) * layout.cell (*c).shapes (deep_layer ().layer ()).size ();
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
  std::auto_ptr<db::DeepEdgePairs> res (new db::DeepEdgePairs (polygons.derived ()));

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

  std::auto_ptr<db::DeepEdgePairs> res (new db::DeepEdgePairs (polygons.derived ()));

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

  std::auto_ptr<db::DeepRegion> res (new db::DeepRegion (polygons.derived ()));
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

EdgesDelegate *
DeepRegion::edges (const EdgeFilterBase *filter) const
{
  const db::DeepLayer &polygons = merged_deep_layer ();

  std::auto_ptr<VariantsCollectorBase> vars;

  if (filter && filter->vars ()) {

    vars.reset (new db::VariantsCollectorBase (filter->vars ()));

    vars->collect (polygons.layout (), polygons.initial_cell ());

    //  NOTE: m_merged_polygons is mutable, so why is the const_cast needed?
    const_cast<db::DeepLayer &> (polygons).separate_variants (*vars);

  }

  db::Layout &layout = const_cast<db::Layout &> (polygons.layout ());

  std::auto_ptr<db::DeepEdges> res (new db::DeepEdges (polygons.derived ()));
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

  std::auto_ptr<VariantsCollectorBase> vars;
  if (filter.vars ()) {

    vars.reset (new db::VariantsCollectorBase (filter.vars ()));

    vars->collect (polygons.layout (), polygons.initial_cell ());

    if (filter.wants_variants ()) {
      const_cast<db::DeepLayer &> (polygons).separate_variants (*vars);
    }

  }

  db::Layout &layout = const_cast<db::Layout &> (polygons.layout ());
  std::map<db::cell_index_type, std::map<db::ICplxTrans, db::Shapes> > to_commit;

  std::auto_ptr<db::DeepRegion> res (new db::DeepRegion (polygons.derived ()));
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

  std::auto_ptr<db::DeepRegion> res (new db::DeepRegion (m_merged_polygons.derived ()));
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

  std::auto_ptr<db::DeepRegion> res (new db::DeepRegion (polygons.derived ()));
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

  std::auto_ptr<db::DeepRegion> res (new db::DeepRegion (polygons.derived ()));
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

namespace
{

class CheckLocalOperation
  : public local_operation<db::PolygonRef, db::PolygonRef, db::EdgePair>
{
public:
  CheckLocalOperation (const EdgeRelationFilter &check, bool different_polygons, bool has_other)
    : m_check (check), m_different_polygons (different_polygons), m_has_other (has_other)
  {
    //  .. nothing yet ..
  }

  virtual void compute_local (db::Layout * /*layout*/, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::unordered_set<db::EdgePair> &result, size_t /*max_vertex_count*/, double /*area_ratio*/) const
  {
    edge2edge_check<std::unordered_set<db::EdgePair> > edge_check (m_check, result, m_different_polygons, m_has_other);
    poly2poly_check<std::unordered_set<db::EdgePair> > poly_check (edge_check);

    std::list<db::Polygon> heap;
    db::box_scanner<db::Polygon, size_t> scanner;

    if (m_has_other) {

      std::set<db::PolygonRef> others;
      for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
        for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
          others.insert (interactions.intruder_shape (*j));
        }
      }

      size_t n = 0;
      for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
        const db::PolygonRef &subject = interactions.subject_shape (i->first);
        heap.push_back (subject.obj ().transformed (subject.trans ()));
        scanner.insert (& heap.back (), n);
        n += 2;
      }

      n = 1;
      for (std::set<db::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o) {
        heap.push_back (o->obj ().transformed (o->trans ()));
        scanner.insert (& heap.back (), n);
        n += 2;
      }

    } else {

      std::set<db::PolygonRef> polygons;
      for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
        polygons.insert (interactions.subject_shape (i->first));
        for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
          polygons.insert (interactions.intruder_shape (*j));
        }
      }

      size_t n = 0;
      for (std::set<db::PolygonRef>::const_iterator o = polygons.begin (); o != polygons.end (); ++o) {
        heap.push_back (o->obj ().transformed (o->trans ()));
        scanner.insert (& heap.back (), n);
        n += 2;
      }

    }

    do {
      scanner.process (poly_check, m_check.distance (), db::box_convert<db::Polygon> ());
    } while (edge_check.prepare_next_pass ());
  }

  virtual db::Coord dist () const
  {
    //  TODO: will the distance be sufficient? Or should we take somewhat more?
    return m_check.distance ();
  }

  virtual on_empty_intruder_mode on_empty_intruder_hint () const
  {
    return m_different_polygons ? Drop : Ignore;
  }

  virtual std::string description () const
  {
    return tl::to_string (tr ("Generic DRC check"));
  }

private:
  EdgeRelationFilter m_check;
  bool m_different_polygons;
  bool m_has_other;
};

}

EdgePairsDelegate *
DeepRegion::run_check (db::edge_relation_type rel, bool different_polygons, const Region *other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
{
  const db::DeepRegion *other_deep = 0;
  if (other) {
    other_deep = dynamic_cast<const db::DeepRegion *> (other->delegate ());
    if (! other_deep) {
      return db::AsIfFlatRegion::run_check (rel, different_polygons, other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
    }
  }

  const db::DeepLayer &polygons = merged_deep_layer ();

  EdgeRelationFilter check (rel, d, metrics);
  check.set_include_zero (false);
  check.set_whole_edges (whole_edges);
  check.set_ignore_angle (ignore_angle);
  check.set_min_projection (min_projection);
  check.set_max_projection (max_projection);

  std::auto_ptr<db::DeepEdgePairs> res (new db::DeepEdgePairs (polygons.derived ()));

  db::CheckLocalOperation op (check, different_polygons, other_deep != 0);

  db::local_processor<db::PolygonRef, db::PolygonRef, db::EdgePair> proc (const_cast<db::Layout *> (&polygons.layout ()),
                                                                          const_cast<db::Cell *> (&polygons.initial_cell ()),
                                                                          other_deep ? &other_deep->deep_layer ().layout () : const_cast<db::Layout *> (&polygons.layout ()),
                                                                          other_deep ? &other_deep->deep_layer ().initial_cell () : const_cast<db::Cell *> (&polygons.initial_cell ()),
                                                                          deep_layer ().breakout_cells (),
                                                                          other_deep ? other_deep->deep_layer ().breakout_cells () : 0);

  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (polygons.store ()->threads ());

  proc.run (&op, polygons.layer (), other_deep ? other_deep->deep_layer ().layer () : polygons.layer (), res->deep_layer ().layer ());

  return res.release ();
}

EdgePairsDelegate *
DeepRegion::run_single_polygon_check (db::edge_relation_type rel, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
{
  const db::DeepLayer &polygons = merged_deep_layer ();

  EdgeRelationFilter check (rel, d, metrics);
  check.set_include_zero (false);
  check.set_whole_edges (whole_edges);
  check.set_ignore_angle (ignore_angle);
  check.set_min_projection (min_projection);
  check.set_max_projection (max_projection);

  db::Layout &layout = const_cast<db::Layout &> (polygons.layout ());

  std::auto_ptr<db::DeepEdgePairs> res (new db::DeepEdgePairs (polygons.derived ()));
  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

    const db::Shapes &shapes = c->shapes (polygons.layer ());
    db::Shapes &result = c->shapes (res->deep_layer ().layer ());

    for (db::Shapes::shape_iterator s = shapes.begin (db::ShapeIterator::Polygons); ! s.at_end (); ++s) {

      edge2edge_check<db::Shapes> edge_check (check, result, false, false);
      poly2poly_check<db::Shapes> poly_check (edge_check);

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

class InteractingLocalOperation
  : public local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef>
{
public:
  InteractingLocalOperation (int mode, bool touching, bool inverse)
    : m_mode (mode), m_touching (touching), m_inverse (inverse)
  {
    //  .. nothing yet ..
  }

  virtual db::Coord dist () const
  {
    return m_touching ? 1 : 0;
  }

  virtual void compute_local (db::Layout * /*layout*/, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::unordered_set<db::PolygonRef> &result, size_t /*max_vertex_count*/, double /*area_ratio*/) const
  {
    db::EdgeProcessor ep;

    std::set<db::PolygonRef> others;
    for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
        others.insert (interactions.intruder_shape (*j));
      }
    }

    size_t n = 1;
    for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i, ++n) {
      const db::PolygonRef &subject = interactions.subject_shape (i->first);
      for (db::PolygonRef::polygon_edge_iterator e = subject.begin_edge (); ! e.at_end(); ++e) {
        ep.insert (*e, n);
      }
    }

    for (std::set<db::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o) {
      for (db::PolygonRef::polygon_edge_iterator e = o->begin_edge (); ! e.at_end(); ++e) {
        ep.insert (*e, 0);
      }
    }

    db::InteractionDetector id (m_mode, 0);
    id.set_include_touching (m_touching);
    db::EdgeSink es;
    ep.process (es, id);
    id.finish ();

    n = 0;
    std::set <size_t> selected;
    for (db::InteractionDetector::iterator i = id.begin (); i != id.end () && i->first == 0; ++i) {
      ++n;
      selected.insert (i->second);
    }

    n = 1;
    for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i, ++n) {
      if ((selected.find (n) == selected.end ()) == m_inverse) {
        const db::PolygonRef &subject = interactions.subject_shape (i->first);
        result.insert (subject);
      }
    }
  }

  virtual on_empty_intruder_mode on_empty_intruder_hint () const
  {
    if ((m_mode <= 0) != m_inverse) {
      return Drop;
    } else {
      return Copy;
    }
  }

  virtual std::string description () const
  {
    return tl::to_string (tr ("Select regions by their geometric relation (interacting, inside, outside ..)"));
  }

private:
  int m_mode;
  bool m_touching;
  bool m_inverse;
};

class PullLocalOperation
  : public local_operation<db::PolygonRef, db::PolygonRef, db::PolygonRef>
{
public:
  PullLocalOperation (int mode, bool touching)
    : m_mode (mode), m_touching (touching)
  {
    //  .. nothing yet ..
  }

  virtual db::Coord dist () const
  {
    return m_touching ? 1 : 0;
  }

  virtual void compute_local (db::Layout * /*layout*/, const shape_interactions<db::PolygonRef, db::PolygonRef> &interactions, std::unordered_set<db::PolygonRef> &result, size_t /*max_vertex_count*/, double /*area_ratio*/) const
  {
    db::EdgeProcessor ep;

    std::set<db::PolygonRef> others;
    for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
        others.insert (interactions.intruder_shape (*j));
      }
    }

    for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      const db::PolygonRef &subject = interactions.subject_shape (i->first);
      for (db::PolygonRef::polygon_edge_iterator e = subject.begin_edge (); ! e.at_end(); ++e) {
        ep.insert (*e, 0);
      }
    }

    size_t n = 1;
    for (std::set<db::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o, ++n) {
      for (db::PolygonRef::polygon_edge_iterator e = o->begin_edge (); ! e.at_end(); ++e) {
        ep.insert (*e, n);
      }
    }

    db::InteractionDetector id (m_mode, 0);
    id.set_include_touching (m_touching);
    db::EdgeSink es;
    ep.process (es, id);
    id.finish ();

    n = 0;
    std::set <size_t> selected;
    for (db::InteractionDetector::iterator i = id.begin (); i != id.end () && i->first == 0; ++i) {
      ++n;
      selected.insert (i->second);
    }

    n = 1;
    for (std::set<db::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o, ++n) {
      if (selected.find (n) != selected.end ()) {
        result.insert (*o);
      }
    }
  }

  virtual on_empty_intruder_mode on_empty_intruder_hint () const
  {
    return Drop;
  }

  virtual std::string description () const
  {
    return tl::to_string (tr ("Pull regions by their geometrical relation to first"));
  }

private:
  int m_mode;
  bool m_touching;
};

struct ResultInserter
{
  typedef db::Polygon value_type;

  ResultInserter (db::Layout *layout, std::unordered_set<db::PolygonRef> &result)
    : mp_layout (layout), mp_result (&result)
  {
    //  .. nothing yet ..
  }

  void insert (const db::Polygon &p)
  {
    (*mp_result).insert (db::PolygonRef (p, mp_layout->shape_repository ()));
  }

private:
  db::Layout *mp_layout;
  std::unordered_set<db::PolygonRef> *mp_result;
};

struct EdgeResultInserter
{
  typedef db::Edge value_type;

  EdgeResultInserter (std::unordered_set<db::Edge> &result)
    : mp_result (&result)
  {
    //  .. nothing yet ..
  }

  void insert (const db::Edge &e)
  {
    (*mp_result).insert (e);
  }

private:
  std::unordered_set<db::Edge> *mp_result;
};

class InteractingWithEdgeLocalOperation
  : public local_operation<db::PolygonRef, db::Edge, db::PolygonRef>
{
public:
  InteractingWithEdgeLocalOperation (bool inverse)
    : m_inverse (inverse)
  {
    //  .. nothing yet ..
  }

  virtual db::Coord dist () const
  {
    //  touching is sufficient
    return 1;
  }

  virtual void compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::Edge> &interactions, std::unordered_set<db::PolygonRef> &result, size_t /*max_vertex_count*/, double /*area_ratio*/) const
  {
    db::box_scanner2<db::Polygon, size_t, db::Edge, size_t> scanner;

    ResultInserter inserter (layout, result);
    region_to_edge_interaction_filter<ResultInserter> filter (inserter, m_inverse);

    for (shape_interactions<db::PolygonRef, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      for (shape_interactions<db::PolygonRef, db::Edge>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
        scanner.insert2 (& interactions.intruder_shape (*j), 0);
      }
    }

    std::list<db::Polygon> heap;
    for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

      const db::PolygonRef &subject = interactions.subject_shape (i->first);
      heap.push_back (subject.obj ().transformed (subject.trans ()));

      scanner.insert1 (&heap.back (), 0);
      if (m_inverse) {
        filter.preset (&heap.back ());
      }

    }

    scanner.process (filter, 1, db::box_convert<db::Polygon> (), db::box_convert<db::Edge> ());
    if (m_inverse) {
      filter.fill_output ();
    }
  }

  virtual on_empty_intruder_mode on_empty_intruder_hint () const
  {
    if (!m_inverse) {
      return Drop;
    } else {
      return Copy;
    }
  }

  virtual std::string description () const
  {
    return tl::to_string (tr ("Select regions by their geometric relation to edges"));
  }

private:
  bool m_inverse;
};

class PullWithEdgeLocalOperation
  : public local_operation<db::PolygonRef, db::Edge, db::Edge>
{
public:
  PullWithEdgeLocalOperation ()
  {
    //  .. nothing yet ..
  }

  virtual db::Coord dist () const
  {
    //  touching is sufficient
    return 1;
  }

  virtual void compute_local (db::Layout *, const shape_interactions<db::PolygonRef, db::Edge> &interactions, std::unordered_set<db::Edge> &result, size_t /*max_vertex_count*/, double /*area_ratio*/) const
  {
    db::box_scanner2<db::Polygon, size_t, db::Edge, size_t> scanner;

    EdgeResultInserter inserter (result);
    region_to_edge_interaction_filter<EdgeResultInserter> filter (inserter, false);

    for (shape_interactions<db::PolygonRef, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      for (shape_interactions<db::PolygonRef, db::Edge>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
        scanner.insert2 (& interactions.intruder_shape (*j), 0);
      }
    }

    std::list<db::Polygon> heap;
    for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

      const db::PolygonRef &subject = interactions.subject_shape (i->first);
      heap.push_back (subject.obj ().transformed (subject.trans ()));

      scanner.insert1 (&heap.back (), 0);

    }

    scanner.process (filter, 1, db::box_convert<db::Polygon> (), db::box_convert<db::Edge> ());
  }

  virtual on_empty_intruder_mode on_empty_intruder_hint () const
  {
    return Drop;
  }

  virtual std::string description () const
  {
    return tl::to_string (tr ("Pull edges from second by their geometric relation to first"));
  }
};

struct TextResultInserter
{
  typedef db::TextRef value_type;

  TextResultInserter (std::unordered_set<db::TextRef> &result)
    : mp_result (&result)
  {
    //  .. nothing yet ..
  }

  void insert (const db::TextRef &e)
  {
    (*mp_result).insert (e);
  }

private:
  std::unordered_set<db::TextRef> *mp_result;
};

class PullWithTextLocalOperation
  : public local_operation<db::PolygonRef, db::TextRef, db::TextRef>
{
public:
  PullWithTextLocalOperation ()
  {
    //  .. nothing yet ..
  }

  virtual db::Coord dist () const
  {
    //  touching is sufficient
    return 1;
  }

  virtual void compute_local (db::Layout *, const shape_interactions<db::PolygonRef, db::TextRef> &interactions, std::unordered_set<db::TextRef> &result, size_t /*max_vertex_count*/, double /*area_ratio*/) const
  {
    db::box_scanner2<db::Polygon, size_t, db::TextRef, size_t> scanner;

    TextResultInserter inserter (result);
    region_to_text_interaction_filter<TextResultInserter, db::TextRef> filter (inserter, false);

    for (shape_interactions<db::PolygonRef, db::TextRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      for (shape_interactions<db::PolygonRef, db::TextRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
        scanner.insert2 (& interactions.intruder_shape (*j), 0);
      }
    }

    std::list<db::Polygon> heap;
    for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

      const db::PolygonRef &subject = interactions.subject_shape (i->first);
      heap.push_back (subject.obj ().transformed (subject.trans ()));

      scanner.insert1 (&heap.back (), 0);

    }

    scanner.process (filter, 1, db::box_convert<db::Polygon> (), db::box_convert<db::TextRef> ());
  }

  virtual on_empty_intruder_mode on_empty_intruder_hint () const
  {
    return Drop;
  }

  virtual std::string description () const
  {
    return tl::to_string (tr ("Pull texts from second by their geometric relation to first"));
  }
};

class InteractingWithTextLocalOperation
  : public local_operation<db::PolygonRef, db::TextRef, db::PolygonRef>
{
public:
  InteractingWithTextLocalOperation (bool inverse)
    : m_inverse (inverse)
  {
    //  .. nothing yet ..
  }

  virtual db::Coord dist () const
  {
    //  touching is sufficient
    return 1;
  }

  virtual void compute_local (db::Layout *layout, const shape_interactions<db::PolygonRef, db::TextRef> &interactions, std::unordered_set<db::PolygonRef> &result, size_t /*max_vertex_count*/, double /*area_ratio*/) const
  {
    db::box_scanner2<db::Polygon, size_t, db::TextRef, size_t> scanner;

    ResultInserter inserter (layout, result);
    region_to_text_interaction_filter<ResultInserter, db::TextRef> filter (inserter, m_inverse);

    for (shape_interactions<db::PolygonRef, db::Text>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      for (shape_interactions<db::PolygonRef, db::Text>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
        scanner.insert2 (& interactions.intruder_shape (*j), 0);
      }
    }

    std::list<db::Polygon> heap;
    for (shape_interactions<db::PolygonRef, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {

      const db::PolygonRef &subject = interactions.subject_shape (i->first);
      heap.push_back (subject.obj ().transformed (subject.trans ()));

      scanner.insert1 (&heap.back (), 0);
      if (m_inverse) {
        filter.preset (&heap.back ());
      }

    }

    scanner.process (filter, 1, db::box_convert<db::Polygon> (), db::box_convert<db::TextRef> ());
    if (m_inverse) {
      filter.fill_output ();
    }
  }

  virtual on_empty_intruder_mode on_empty_intruder_hint () const
  {
    if (!m_inverse) {
      return Drop;
    } else {
      return Copy;
    }
  }

  virtual std::string description () const
  {
    return tl::to_string (tr ("Select regions by their geometric relation to texts"));
  }

private:
  bool m_inverse;
};

}

RegionDelegate *
DeepRegion::selected_interacting_generic (const Region &other, int mode, bool touching, bool inverse) const
{
  //  with these flag set to true, the resulting polygons are broken again.
  bool split_after = false;

  std::auto_ptr<db::DeepRegion> dr_holder;
  const db::DeepRegion *other_deep = dynamic_cast<const db::DeepRegion *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchisation
    dr_holder.reset (new db::DeepRegion (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  const db::DeepLayer &polygons = merged_deep_layer ();
  //  NOTE: on "inside", the other polygons must be merged
  const db::DeepLayer &other_polygons = mode < 0 ? other_deep->merged_deep_layer () : other_deep->deep_layer ();

  DeepLayer dl_out (polygons.derived ());

  db::InteractingLocalOperation op (mode, touching, inverse);

  db::local_processor<db::PolygonRef, db::PolygonRef, db::PolygonRef> proc (const_cast<db::Layout *> (&polygons.layout ()), const_cast<db::Cell *> (&polygons.initial_cell ()), &other_polygons.layout (), &other_polygons.initial_cell (), polygons.breakout_cells (), other_polygons.breakout_cells ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (polygons.store ()->threads ());
  if (split_after) {
    proc.set_area_ratio (polygons.store ()->max_area_ratio ());
    proc.set_max_vertex_count (polygons.store ()->max_vertex_count ());
  }

  proc.run (&op, polygons.layer (), other_polygons.layer (), dl_out.layer ());

  db::DeepRegion *res = new db::DeepRegion (dl_out);
  if (! split_after && ((mode < 0 && other.merged_semantics ()) || other.is_merged ()) && (merged_semantics () || is_merged ())) {
    res->set_is_merged (true);
  }
  return res;
}

RegionDelegate *
DeepRegion::selected_interacting_generic (const Edges &other, bool inverse) const
{
  //  with these flag set to true, the resulting polygons are broken again.
  bool split_after = false;

  std::auto_ptr<db::DeepEdges> dr_holder;
  const db::DeepEdges *other_deep = dynamic_cast<const db::DeepEdges *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchisation
    dr_holder.reset (new db::DeepEdges (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  const db::DeepLayer &polygons = merged_deep_layer ();

  DeepLayer dl_out (polygons.derived ());

  db::InteractingWithEdgeLocalOperation op (inverse);

  db::local_processor<db::PolygonRef, db::Edge, db::PolygonRef> proc (const_cast<db::Layout *> (&polygons.layout ()), const_cast<db::Cell *> (&polygons.initial_cell ()), &other_deep->deep_layer ().layout (), &other_deep->deep_layer ().initial_cell (), polygons.breakout_cells (), other_deep->deep_layer ().breakout_cells ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (polygons.store ()->threads ());
  if (split_after) {
    proc.set_area_ratio (polygons.store ()->max_area_ratio ());
    proc.set_max_vertex_count (polygons.store ()->max_vertex_count ());
  }

  proc.run (&op, polygons.layer (), other_deep->deep_layer ().layer (), dl_out.layer ());

  db::DeepRegion *res = new db::DeepRegion (dl_out);
  if (! split_after) {
    res->set_is_merged (other.is_merged () && (merged_semantics () || is_merged ()));
  }
  return res;
}

RegionDelegate *
DeepRegion::pull_generic (const Region &other, int mode, bool touching) const
{
  //  with these flag set to true, the resulting polygons are broken again.
  bool split_after = false;

  std::auto_ptr<db::DeepRegion> dr_holder;
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
  std::auto_ptr<db::DeepEdges> dr_holder;
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
  std::auto_ptr<db::DeepTexts> dr_holder;
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
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (polygons.store ()->threads ());
  proc.run (&op, polygons.layer (), other_texts.layer (), dl_out.layer ());

  db::DeepTexts *res = new db::DeepTexts (dl_out);
  res->set_is_merged (is_merged ());
  return res;
}

RegionDelegate *
DeepRegion::selected_interacting_generic (const Texts &other, bool inverse) const
{
  //  with these flag set to true, the resulting polygons are broken again.
  bool split_after = false;

  std::auto_ptr<db::DeepTexts> dr_holder;
  const db::DeepTexts *other_deep = dynamic_cast<const db::DeepTexts *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchisation
    dr_holder.reset (new db::DeepTexts (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  const db::DeepLayer &polygons = merged_deep_layer ();

  DeepLayer dl_out (polygons.derived ());

  db::InteractingWithTextLocalOperation op (inverse);

  db::local_processor<db::PolygonRef, db::TextRef, db::PolygonRef> proc (const_cast<db::Layout *> (&polygons.layout ()), const_cast<db::Cell *> (&polygons.initial_cell ()), &other_deep->deep_layer ().layout (), &other_deep->deep_layer ().initial_cell (), polygons.breakout_cells (), other_deep->deep_layer ().breakout_cells ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (polygons.store ()->threads ());
  if (split_after) {
    proc.set_area_ratio (polygons.store ()->max_area_ratio ());
    proc.set_max_vertex_count (polygons.store ()->max_vertex_count ());
  }

  proc.run (&op, polygons.layer (), other_deep->deep_layer ().layer (), dl_out.layer ());

  db::DeepRegion *res = new db::DeepRegion (dl_out);
  if (! split_after) {
    res->set_is_merged (merged_semantics () || is_merged ());
  }
  return res;
}

}
