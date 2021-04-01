
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

#include "dbEdges.h"
#include "dbEdgesUtils.h"
#include "dbRegion.h"
#include "dbDeepEdges.h"
#include "dbDeepRegion.h"
#include "dbDeepEdgePairs.h"
#include "dbHierNetworkProcessor.h"
#include "dbCellGraphUtils.h"
#include "dbCellVariants.h"
#include "dbEdgeBoolean.h"
#include "dbCellMapping.h"
#include "dbLayoutUtils.h"
#include "dbLocalOperation.h"
#include "dbLocalOperationUtils.h"
#include "dbHierProcessor.h"
#include "dbEmptyEdges.h"

#include <unordered_set>

namespace db
{

/**
 *  @brief An iterator delegate for the deep region
 *  TODO: this is kind of redundant with OriginalLayerIterator ..
 */
class DB_PUBLIC DeepEdgesIterator
  : public EdgesIteratorDelegate
{
public:
  typedef db::Edge value_type;

  DeepEdgesIterator (const db::RecursiveShapeIterator &iter)
    : m_iter (iter)
  {
    set ();
  }

  virtual ~DeepEdgesIterator () { }

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
    return &m_edge;
  }

  virtual bool equals (const generic_shape_iterator_delegate_base<value_type> *other) const
  {
    const DeepEdgesIterator *o = dynamic_cast<const DeepEdgesIterator *> (other);
    return o && o->m_iter == m_iter;
  }

  virtual EdgesIteratorDelegate *clone () const
  {
    return new DeepEdgesIterator (*this);
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
  friend class Texts;

  db::RecursiveShapeIterator m_iter;
  mutable value_type m_edge;

  void set () const
  {
    if (! m_iter.at_end ()) {
      m_iter.shape ().edge (m_edge);
      m_edge.transform (m_iter.trans ());
    }
  }
};

// -------------------------------------------------------------------------------------------------------------
//  DeepEdges implementation

DeepEdges::DeepEdges (const RecursiveShapeIterator &si, DeepShapeStore &dss, bool as_edges)
  : MutableEdges (), m_merged_edges ()
{
  set_deep_layer (dss.create_edge_layer (si, as_edges));
  init ();
}

DeepEdges::DeepEdges (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans, bool as_edges, bool merged_semantics)
  : MutableEdges (), m_merged_edges ()
{
  set_deep_layer (dss.create_edge_layer (si, as_edges, trans));
  init ();
  set_merged_semantics (merged_semantics);
}

DeepEdges::DeepEdges (const db::Edges &other, DeepShapeStore &dss)
  : MutableEdges (), m_merged_edges ()
{
  set_deep_layer (dss.create_from_flat (other));

  init ();
  set_merged_semantics (other.merged_semantics ());
}

DeepEdges::DeepEdges ()
  : MutableEdges ()
{
  init ();
}

DeepEdges::DeepEdges (const DeepLayer &dl)
  : MutableEdges ()
{
  set_deep_layer (dl);
  init ();
}

DeepEdges::~DeepEdges ()
{
  //  .. nothing yet ..
}

DeepEdges::DeepEdges (const DeepEdges &other)
  : MutableEdges (other), DeepShapeCollectionDelegateBase (other),
    m_merged_edges_valid (other.m_merged_edges_valid),
    m_is_merged (other.m_is_merged)
{
  if (m_merged_edges_valid) {
    m_merged_edges = other.m_merged_edges;
  }
}

DeepEdges &
DeepEdges::operator= (const DeepEdges &other)
{
  if (this != &other) {

    AsIfFlatEdges::operator= (other);
    DeepShapeCollectionDelegateBase::operator= (other);

    m_merged_edges_valid = other.m_merged_edges_valid;
    m_is_merged = other.m_is_merged;
    if (m_merged_edges_valid) {
      m_merged_edges = other.m_merged_edges;
    }

  }

  return *this;
}

void DeepEdges::init ()
{
  m_merged_edges_valid = false;
  m_merged_edges = db::DeepLayer ();
  m_is_merged = false;
}

EdgesDelegate *
DeepEdges::clone () const
{
  return new DeepEdges (*this);
}

void DeepEdges::merged_semantics_changed ()
{
  //  .. nothing yet ..
}

void DeepEdges::do_insert (const db::Edge &edge)
{
  db::Layout &layout = deep_layer ().layout ();
  if (layout.begin_top_down () != layout.end_top_down ()) {
    db::Cell &top_cell = layout.cell (*layout.begin_top_down ());
    top_cell.shapes (deep_layer ().layer ()).insert (edge);
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
      flat_shapes.insert (iter->edge ().transformed (iter.trans ()).transformed (t));
    }

    layout.clear_layer (deep_layer.layer ());
    top_cell.shapes (deep_layer.layer ()).swap (flat_shapes);

  }
}

void DeepEdges::do_transform (const db::Trans &t)
{
  transform_deep_layer (deep_layer (), t);
  invalidate_bbox ();
}

void DeepEdges::do_transform (const db::ICplxTrans &t)
{
  transform_deep_layer (deep_layer (), t);
  invalidate_bbox ();
}

void DeepEdges::do_transform (const db::IMatrix2d &t)
{
  transform_deep_layer (deep_layer (), t);
  invalidate_bbox ();
}

void DeepEdges::do_transform (const db::IMatrix3d &t)
{
  transform_deep_layer (deep_layer (), t);
  invalidate_bbox ();
}

void DeepEdges::reserve (size_t)
{
  //  Not implemented for deep regions
}

void DeepEdges::flatten ()
{
  db::Layout &layout = deep_layer ().layout ();
  if (layout.begin_top_down () != layout.end_top_down ()) {

    db::Cell &top_cell = layout.cell (*layout.begin_top_down ());

    db::Shapes flat_shapes (layout.is_editable ());
    for (db::RecursiveShapeIterator iter (layout, top_cell, deep_layer ().layer ()); !iter.at_end (); ++iter) {
      flat_shapes.insert (iter->edge ().transformed (iter.trans ()));
    }

    layout.clear_layer (deep_layer ().layer ());
    top_cell.shapes (deep_layer ().layer ()).swap (flat_shapes);

  }
}

EdgesIteratorDelegate *
DeepEdges::begin () const
{
  return new DeepEdgesIterator (begin_iter ().first);
}

EdgesIteratorDelegate *
DeepEdges::begin_merged () const
{
  if (! merged_semantics ()) {
    return begin ();
  } else {
    return new DeepEdgesIterator (begin_merged_iter ().first);
  }
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans>
DeepEdges::begin_iter () const
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
DeepEdges::begin_merged_iter () const
{
  if (! merged_semantics ()) {

    return begin_iter ();

  } else {

    ensure_merged_edges_valid ();

    const db::Layout &layout = m_merged_edges.layout ();
    if (layout.cells () == 0) {

      return std::make_pair (db::RecursiveShapeIterator (), db::ICplxTrans ());

    } else {

      const db::Cell &top_cell = layout.cell (*layout.begin_top_down ());
      db::RecursiveShapeIterator iter (m_merged_edges.layout (), top_cell, m_merged_edges.layer ());
      return std::make_pair (iter, db::ICplxTrans ());

    }

  }
}

bool
DeepEdges::empty () const
{
  return begin_iter ().first.at_end ();
}

bool
DeepEdges::is_merged () const
{
  return m_is_merged;
}

const db::Edge *
DeepEdges::nth (size_t /*n*/) const
{
  throw tl::Exception (tl::to_string (tr ("Random access to edges is available only for flat edge collections")));
}

bool
DeepEdges::has_valid_edges () const
{
  return false;
}

bool
DeepEdges::has_valid_merged_edges () const
{
  return merged_semantics ();
}

const db::RecursiveShapeIterator *
DeepEdges::iter () const
{
  return 0;
}

bool DeepEdges::equals (const Edges &other) const
{
  const DeepEdges *other_delegate = dynamic_cast<const DeepEdges *> (other.delegate ());
  if (other_delegate && &other_delegate->deep_layer ().layout () == &deep_layer ().layout ()
      && other_delegate->deep_layer ().layer () == deep_layer ().layer ()) {
    return true;
  } else {
    return AsIfFlatEdges::equals (other);
  }
}

bool DeepEdges::less (const Edges &other) const
{
  const DeepEdges *other_delegate = dynamic_cast<const DeepEdges *> (other.delegate ());
  if (other_delegate && &other_delegate->deep_layer ().layout () == &deep_layer ().layout ()) {
    return other_delegate->deep_layer ().layer () < deep_layer ().layer ();
  } else {
    return AsIfFlatEdges::less (other);
  }
}

namespace {

class ClusterMerger
{
public:
  ClusterMerger (unsigned int layer, const db::hier_clusters<db::Edge> &hc, bool report_progress, const std::string &progress_desc)
    : m_layer (layer), mp_hc (&hc), m_scanner (report_progress, progress_desc)
  {
    //  .. nothing yet ..
  }

  void set_base_verbosity (int /*vb*/)
  {
    /* TODO: No such thing currently:
    m_scanner.set_base_verbosity (vb);
    */
  }

  db::Shapes &merged (size_t cid, db::cell_index_type ci, bool initial = true)
  {
    std::map<std::pair<size_t, db::cell_index_type>, db::Shapes>::iterator s = m_merged_cluster.find (std::make_pair (cid, ci));

    //  some sanity checks: initial clusters are single-use, are never generated twice and cannot be retrieved again
    if (initial) {
      tl_assert (s == m_merged_cluster.end ());
      m_done.insert (std::make_pair (cid, ci));
    } else {
      tl_assert (m_done.find (std::make_pair (cid, ci)) == m_done.end ());
    }

    if (s != m_merged_cluster.end ()) {
      return s->second;
    }

    s = m_merged_cluster.insert (std::make_pair (std::make_pair (cid, ci), db::Shapes (false))).first;

    const db::connected_clusters<db::Edge> &cc = mp_hc->clusters_per_cell (ci);
    const db::local_cluster<db::Edge> &c = cc.cluster_by_id (cid);

    std::list<std::pair<const db::Shapes *, db::ICplxTrans> > merged_child_clusters;

    const db::connected_clusters<db::Edge>::connections_type &conn = cc.connections_for_cluster (cid);
    for (db::connected_clusters<db::Edge>::connections_type::const_iterator i = conn.begin (); i != conn.end (); ++i) {
      const db::Shapes &cc_shapes = merged (i->id (), i->inst_cell_index (), false);
      merged_child_clusters.push_back (std::make_pair (&cc_shapes, i->inst_trans ()));
    }

    //  collect the edges to merge ..

    std::list<db::Edge> heap;
    m_scanner.clear ();

    for (std::list<std::pair<const db::Shapes *, db::ICplxTrans> >::const_iterator i = merged_child_clusters.begin (); i != merged_child_clusters.end (); ++i) {
      for (db::Shapes::shape_iterator s = i->first->begin (db::ShapeIterator::All); ! s.at_end (); ++s) {
        if (s->is_edge ()) {
          heap.push_back (s->edge ().transformed (i->second));
          m_scanner.insert (&heap.back (), 0);
        }
      }
    }

    for (db::local_cluster<db::Edge>::shape_iterator s = c.begin (m_layer); !s.at_end (); ++s) {
      heap.push_back (*s);
      m_scanner.insert (&heap.back (), 0);
    }

    //  .. and run the merge operation

    s->second.clear ();
    EdgeBooleanClusterCollectorToShapes cluster_collector (&s->second, EdgeOr);
    m_scanner.process (cluster_collector, 1, db::box_convert<db::Edge> ());

    return s->second;
  }

private:
  std::map<std::pair<size_t, db::cell_index_type>, db::Shapes> m_merged_cluster;
  std::set<std::pair<size_t, db::cell_index_type> > m_done;
  unsigned int m_layer;
  const db::hier_clusters<db::Edge> *mp_hc;
  db::box_scanner<db::Edge, size_t> m_scanner;
};

}

const DeepLayer &
DeepEdges::merged_deep_layer () const
{
  if (merged_semantics ()) {
    ensure_merged_edges_valid ();
    return m_merged_edges;
  } else {
    return deep_layer ();
  }
}

void
DeepEdges::ensure_merged_edges_valid () const
{
  if (! m_merged_edges_valid) {

    if (m_is_merged) {

      //  NOTE: this will reuse the deep layer reference
      m_merged_edges = deep_layer ();

    } else {

      m_merged_edges = deep_layer ().derived ();

      tl::SelfTimer timer (tl::verbosity () > base_verbosity (), "Ensure merged polygons");

      db::Layout &layout = const_cast<db::Layout &> (deep_layer ().layout ());

      db::hier_clusters<db::Edge> hc;
      db::Connectivity conn;
      conn.connect (deep_layer ());
      hc.set_base_verbosity (base_verbosity() + 10);
      hc.build (layout, deep_layer ().initial_cell (), conn);

      //  collect the clusters and merge them into big polygons
      //  NOTE: using the ClusterMerger we merge bottom-up forming bigger and bigger polygons. This is
      //  hopefully more efficient that collecting everything and will lead to reuse of parts.

      ClusterMerger cm (deep_layer ().layer (), hc, report_progress (), progress_desc ());
      cm.set_base_verbosity (base_verbosity () + 10);

      //  TODO: iterate only over the called cells?
      for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
        const db::connected_clusters<db::Edge> &cc = hc.clusters_per_cell (c->cell_index ());
        for (db::connected_clusters<db::Edge>::all_iterator cl = cc.begin_all (); ! cl.at_end (); ++cl) {
          if (cc.is_root (*cl)) {
            db::Shapes &s = cm.merged (*cl, c->cell_index ());
            c->shapes (m_merged_edges.layer ()).insert (s);
            s.clear (); //  not needed anymore
          }
        }
      }

    }

    m_merged_edges_valid = true;

  }
}

void
DeepEdges::set_is_merged (bool f)
{
  m_is_merged = f;
  m_merged_edges_valid = false;
}

void
DeepEdges::insert_into (db::Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  deep_layer ().insert_into (layout, into_cell, into_layer);
}

size_t DeepEdges::count () const
{
  size_t n = 0;

  const db::Layout &layout = deep_layer ().layout ();
  db::CellCounter cc (&layout);
  for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
    n += cc.weight (*c) * layout.cell (*c).shapes (deep_layer ().layer ()).size ();
  }

  return n;
}

size_t DeepEdges::hier_count () const
{
  size_t n = 0;

  const db::Layout &layout = deep_layer ().layout ();
  for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
    n += layout.cell (*c).shapes (deep_layer ().layer ()).size ();
  }

  return n;
}

Box DeepEdges::bbox () const
{
  return deep_layer ().initial_cell ().bbox (deep_layer ().layer ());
}

DeepEdges::length_type DeepEdges::length (const db::Box &box) const
{
  if (box.empty ()) {

    const db::DeepLayer &edges = merged_deep_layer ();

    db::MagnificationReducer red;
    db::cell_variants_collector<db::MagnificationReducer> vars (red);
    vars.collect (edges.layout (), edges.initial_cell ());

    DeepEdges::length_type l = 0;

    const db::Layout &layout = edges.layout ();
    for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
      DeepEdges::length_type lc = 0;
      for (db::ShapeIterator s = layout.cell (*c).shapes (edges.layer ()).begin (db::ShapeIterator::Edges); ! s.at_end (); ++s) {
        lc += s->edge ().length ();
      }
      const std::map<db::ICplxTrans, size_t> &vv = vars.variants (*c);
      for (std::map<db::ICplxTrans, size_t>::const_iterator v = vv.begin (); v != vv.end (); ++v) {
        double mag = v->first.mag ();
        l += v->second * lc * mag;
      }
    }

    return l;

  } else {
    //  In the clipped case fall back to flat mode
    return db::AsIfFlatEdges::length (box);
  }
}

std::string DeepEdges::to_string (size_t nmax) const
{
  return db::AsIfFlatEdges::to_string (nmax);
}

EdgesDelegate *DeepEdges::process_in_place (const EdgeProcessorBase &filter)
{
  //  TODO: implement to be really in-place
  return processed (filter);
}

EdgesDelegate *
DeepEdges::processed (const EdgeProcessorBase &filter) const
{
  return shape_collection_processed_impl<db::Edge, db::Edge, db::DeepEdges> (filter.requires_raw_input () ? deep_layer () : merged_deep_layer (), filter);
}

EdgePairsDelegate *
DeepEdges::processed_to_edge_pairs (const EdgeToEdgePairProcessorBase &filter) const
{
  return shape_collection_processed_impl<db::Edge, db::EdgePair, db::DeepEdgePairs> (filter.requires_raw_input () ? deep_layer () : merged_deep_layer (), filter);
}

RegionDelegate *
DeepEdges::processed_to_polygons (const EdgeToPolygonProcessorBase &filter) const
{
  return shape_collection_processed_impl<db::Edge, db::Polygon, db::DeepRegion> (filter.requires_raw_input () ? deep_layer () : merged_deep_layer (), filter);
}

EdgesDelegate *
DeepEdges::filter_in_place (const EdgeFilterBase &filter)
{
  //  TODO: implement to be really in-place
  *this = *apply_filter (filter);
  return this;
}

EdgesDelegate *
DeepEdges::filtered (const EdgeFilterBase &filter) const
{
  return apply_filter (filter);
}

DeepEdges *
DeepEdges::apply_filter (const EdgeFilterBase &filter) const
{
  const db::DeepLayer &edges = filter.requires_raw_input () ? deep_layer () : merged_deep_layer ();

  std::unique_ptr<VariantsCollectorBase> vars;
  if (filter.vars ()) {

    vars.reset (new db::VariantsCollectorBase (filter.vars ()));

    vars->collect (edges.layout (), edges.initial_cell ());

    if (filter.wants_variants ()) {
      const_cast<db::DeepLayer &> (edges).separate_variants (*vars);
    }

  }

  db::Layout &layout = const_cast<db::Layout &> (edges.layout ());
  std::map<db::cell_index_type, std::map<db::ICplxTrans, db::Shapes> > to_commit;

  std::unique_ptr<db::DeepEdges> res (new db::DeepEdges (edges.derived ()));
  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

    const db::Shapes &s = c->shapes (edges.layer ());

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

        for (db::Shapes::shape_iterator si = s.begin (db::ShapeIterator::Edges); ! si.at_end (); ++si) {
          if (filter.selected (si->edge ().transformed (tr))) {
            st->insert (*si);
          }
        }

      }

    } else {

      db::Shapes &st = c->shapes (res->deep_layer ().layer ());

      for (db::Shapes::shape_iterator si = s.begin (db::ShapeIterator::Edges); ! si.at_end (); ++si) {
        if (filter.selected (si->edge ())) {
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

EdgesDelegate *DeepEdges::merged_in_place ()
{
  ensure_merged_edges_valid ();

  //  NOTE: this makes both layers share the same resource
  set_deep_layer (m_merged_edges);

  return this;
}

EdgesDelegate *DeepEdges::merged () const
{
  ensure_merged_edges_valid ();

  db::Layout &layout = const_cast<db::Layout &> (m_merged_edges.layout ());

  std::unique_ptr<db::DeepEdges> res (new db::DeepEdges (m_merged_edges.derived ()));
  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
    c->shapes (res->deep_layer ().layer ()) = c->shapes (m_merged_edges.layer ());
  }

  res->set_is_merged (true);
  return res.release ();
}

DeepLayer
DeepEdges::and_or_not_with (const DeepEdges *other, EdgeBoolOp op) const
{
  DeepLayer dl_out (deep_layer ().derived ());

  db::EdgeBoolAndOrNotLocalOperation local_op (op);

  db::local_processor<db::Edge, db::Edge, db::Edge> proc (const_cast<db::Layout *> (&deep_layer ().layout ()), const_cast<db::Cell *> (&deep_layer ().initial_cell ()), &other->deep_layer ().layout (), &other->deep_layer ().initial_cell ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (deep_layer ().store ()->threads ());
  proc.set_area_ratio (deep_layer ().store ()->max_area_ratio ());
  proc.set_max_vertex_count (deep_layer ().store ()->max_vertex_count ());

  proc.run (&local_op, deep_layer ().layer (), other->deep_layer ().layer (), dl_out.layer ());

  return dl_out;
}

DeepLayer
DeepEdges::edge_region_op (const DeepRegion *other, bool outside, bool include_borders) const
{
  DeepLayer dl_out (deep_layer ().derived ());

  db::EdgeToPolygonLocalOperation op (outside, include_borders);

  db::local_processor<db::Edge, db::PolygonRef, db::Edge> proc (const_cast<db::Layout *> (&deep_layer ().layout ()), const_cast<db::Cell *> (&deep_layer ().initial_cell ()), &other->deep_layer ().layout (), &other->deep_layer ().initial_cell ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (deep_layer ().store ()->threads ());
  proc.set_area_ratio (deep_layer ().store ()->max_area_ratio ());
  proc.set_max_vertex_count (deep_layer ().store ()->max_vertex_count ());

  proc.run (&op, deep_layer ().layer (), other->deep_layer ().layer (), dl_out.layer ());

  return dl_out;
}

EdgesDelegate *DeepEdges::intersections (const Edges &other) const
{
  const DeepEdges *other_deep = dynamic_cast <const DeepEdges *> (other.delegate ());

  if (empty () || other.empty ()) {

    //  Nothing to do
    return new EmptyEdges ();

  } else if (! other_deep) {

    return AsIfFlatEdges::intersections (other);

  } else {

    return new DeepEdges (and_or_not_with (other_deep, EdgeIntersections));

  }
}

EdgesDelegate *DeepEdges::and_with (const Edges &other) const
{
  const DeepEdges *other_deep = dynamic_cast <const DeepEdges *> (other.delegate ());

  if (empty () || other.empty ()) {

    //  Nothing to do
    return new EmptyEdges ();

  } else if (! other_deep) {

    return AsIfFlatEdges::and_with (other);

  } else {

    return new DeepEdges (and_or_not_with (other_deep, EdgeAnd));

  }
}

EdgesDelegate *DeepEdges::and_with (const Region &other) const
{
  const DeepRegion *other_deep = dynamic_cast <const DeepRegion *> (other.delegate ());

  if (empty ()) {

    //  Nothing to do
    return new EmptyEdges ();

  } else if (other.empty ()) {

    //  Nothing to do
    return clone ();

  } else if (! other_deep) {

    return AsIfFlatEdges::not_with (other);

  } else {

    return new DeepEdges (edge_region_op (other_deep, false /*outside*/, true /*include borders*/));

  }
}

EdgesDelegate *DeepEdges::not_with (const Edges &other) const
{
  const DeepEdges *other_deep = dynamic_cast <const DeepEdges *> (other.delegate ());

  if (empty ()) {

    //  Nothing to do
    return new EmptyEdges ();

  } else if (other.empty ()) {

    //  Nothing to do
    return clone ();

  } else if (! other_deep) {

    return AsIfFlatEdges::not_with (other);

  } else {

    return new DeepEdges (and_or_not_with (other_deep, EdgeNot));

  }
}

EdgesDelegate *DeepEdges::not_with (const Region &other) const
{
  const DeepRegion *other_deep = dynamic_cast <const DeepRegion *> (other.delegate ());

  if (empty ()) {

    //  Nothing to do
    return new EmptyEdges ();

  } else if (other.empty ()) {

    //  Nothing to do
    return clone ();

  } else if (! other_deep) {

    return AsIfFlatEdges::not_with (other);

  } else {

    return new DeepEdges (edge_region_op (other_deep, true /*outside*/, true /*include borders*/));

  }
}

EdgesDelegate *DeepEdges::xor_with (const Edges &other) const
{
  const DeepEdges *other_deep = dynamic_cast <const DeepEdges *> (other.delegate ());

  if (empty ()) {

    //  Nothing to do
    return other.delegate ()->clone ();

  } else if (other.empty ()) {

    //  Nothing to do
    return clone ();

  } else if (! other_deep) {

    return AsIfFlatEdges::xor_with (other);

  } else {

    //  Implement XOR as (A-B)+(B-A) - only this implementation
    //  is compatible with the local processor scheme
    DeepLayer n1 (and_or_not_with (other_deep, EdgeNot));
    DeepLayer n2 (other_deep->and_or_not_with (this, EdgeNot));

    n1.add_from (n2);
    return new DeepEdges (n1);

  }
}

EdgesDelegate *DeepEdges::or_with (const Edges &other) const
{
  //  NOTE: in the hierarchical case we don't do a merge on "or": just map to add
  return add (other);
}

EdgesDelegate *
DeepEdges::add_in_place (const Edges &other)
{
  if (other.empty ()) {
    return this;
  }

  const DeepEdges *other_deep = dynamic_cast <const DeepEdges *> (other.delegate ());
  if (other_deep) {

    deep_layer ().add_from (other_deep->deep_layer ());

  } else {

    //  non-deep to deep merge (flat)

    db::Shapes &shapes = deep_layer ().initial_cell ().shapes (deep_layer ().layer ());
    for (db::Edges::const_iterator p = other.begin (); ! p.at_end (); ++p) {
      shapes.insert (*p);
    }

  }

  set_is_merged (false);
  return this;
}

EdgesDelegate *DeepEdges::add (const Edges &other) const
{
  if (other.empty ()) {
    return clone ();
  } else if (empty ()) {
    return other.delegate ()->clone ();
  } else {
    DeepEdges *new_edges = dynamic_cast<DeepEdges *> (clone ());
    new_edges->add_in_place (other);
    return new_edges;
  }
}

EdgesDelegate *DeepEdges::inside_part (const Region &other) const
{
  const DeepRegion *other_deep = dynamic_cast <const DeepRegion *> (other.delegate ());

  if (empty ()) {

    //  Nothing to do
    return new EmptyEdges ();

  } else if (other.empty ()) {

    //  Nothing to do
    return clone ();

  } else if (! other_deep) {

    return AsIfFlatEdges::not_with (other);

  } else {

    return new DeepEdges (edge_region_op (other_deep, false /*outside*/, false /*include borders*/));

  }
}

EdgesDelegate *DeepEdges::outside_part (const Region &other) const
{
  const DeepRegion *other_deep = dynamic_cast <const DeepRegion *> (other.delegate ());

  if (empty ()) {

    //  Nothing to do
    return new EmptyEdges ();

  } else if (other.empty ()) {

    //  Nothing to do
    return clone ();

  } else if (! other_deep) {

    return AsIfFlatEdges::not_with (other);

  } else {

    return new DeepEdges (edge_region_op (other_deep, true /*outside*/, false /*include borders*/));

  }
}

RegionDelegate *DeepEdges::extended (coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i, bool join) const
{
  const db::DeepLayer &edges = merged_deep_layer ();

  std::unique_ptr<db::DeepRegion> res (new db::DeepRegion (edges.derived ()));

  db::Layout &layout = const_cast<db::Layout &> (edges.layout ());
  db::Cell &top_cell = const_cast<db::Cell &> (edges.initial_cell ());

  //  TODO: there is a special case when we'd need a MagnificationAndOrientationReducer:
  //  dots formally don't have an orientation, hence the interpretation is x and y.
  db::MagnificationReducer red;
  db::cell_variants_collector<db::MagnificationReducer> vars (red);
  vars.collect (edges.layout (), edges.initial_cell ());

  std::map<db::cell_index_type, std::map<db::ICplxTrans, db::Shapes> > to_commit;

  if (join) {

    db::hier_clusters<db::Edge> hc;
    db::Connectivity conn (db::Connectivity::EdgesConnectByPoints);
    conn.connect (edges);
    hc.set_base_verbosity (base_verbosity () + 10);
    hc.build (layout, edges.initial_cell (), conn);

    //  TODO: iterate only over the called cells?
    for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

      const std::map<db::ICplxTrans, size_t> &vv = vars.variants (c->cell_index ());
      for (std::map<db::ICplxTrans, size_t>::const_iterator v = vv.begin (); v != vv.end (); ++v) {

        db::Shapes *out;
        if (vv.size () == 1) {
          out = & c->shapes (res->deep_layer ().layer ());
        } else {
          out = & to_commit [c->cell_index ()][v->first];
        }

        const db::connected_clusters<db::Edge> &cc = hc.clusters_per_cell (c->cell_index ());
        for (db::connected_clusters<db::Edge>::all_iterator cl = cc.begin_all (); ! cl.at_end (); ++cl) {

          if (cc.is_root (*cl)) {

            PolygonRefToShapesGenerator prgen (&layout, out);
            polygon_transformation_filter<db::ICplxTrans> ptrans (&prgen, v->first.inverted ());
            JoinEdgesCluster jec (&ptrans, ext_b, ext_e, ext_o, ext_i);

            std::list<db::Edge> heap;
            for (db::recursive_cluster_shape_iterator<db::Edge> rcsi (hc, edges.layer (), c->cell_index (), *cl); ! rcsi.at_end (); ++rcsi) {
              heap.push_back (rcsi->transformed (v->first * rcsi.trans ()));
              jec.add (&heap.back (), 0);
            }

            jec.finish ();

          }

        }

      }

    }

  } else {

    for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {

      const std::map<db::ICplxTrans, size_t> &vv = vars.variants (c->cell_index ());
      for (std::map<db::ICplxTrans, size_t>::const_iterator v = vv.begin (); v != vv.end (); ++v) {

        db::Shapes *out;
        if (vv.size () == 1) {
          out = & c->shapes (res->deep_layer ().layer ());
        } else {
          out = & to_commit [c->cell_index ()][v->first];
        }

        for (db::Shapes::shape_iterator si = c->shapes (edges.layer ()).begin (db::ShapeIterator::Edges); ! si.at_end (); ++si) {
          out->insert (extended_edge (si->edge ().transformed (v->first), ext_b, ext_e, ext_o, ext_i).transformed (v->first.inverted ()));
        }

      }

    }

  }

  //  propagate results from variants
  vars.commit_shapes (layout, top_cell, res->deep_layer ().layer (), to_commit);

  return res.release ();
}

namespace
{

class Edge2EdgeInteractingLocalOperation
  : public local_operation<db::Edge, db::Edge, db::Edge>
{
public:
  Edge2EdgeInteractingLocalOperation (bool inverse)
    : m_inverse (inverse)
  {
    //  .. nothing yet ..
  }

  virtual db::Coord dist () const
  {
    //  touching is sufficient
    return 1;
  }

  virtual void do_compute_local (db::Layout * /*layout*/, const shape_interactions<db::Edge, db::Edge> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
  {
    tl_assert (results.size () == 1);
    std::unordered_set<db::Edge> &result = results.front ();

    db::box_scanner<db::Edge, size_t> scanner;

    std::set<db::Edge> others;
    for (shape_interactions<db::Edge, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      for (shape_interactions<db::Edge, db::Edge>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
        others.insert (interactions.intruder_shape (*j).second);
      }
    }

    for (shape_interactions<db::Edge, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      const db::Edge &subject = interactions.subject_shape (i->first);
      scanner.insert (&subject, 0);
    }

    for (std::set<db::Edge>::const_iterator o = others.begin (); o != others.end (); ++o) {
      scanner.insert (o.operator-> (), 1);
    }

    if (m_inverse) {

      std::unordered_set<db::Edge> interacting;
      edge_interaction_filter<std::unordered_set<db::Edge> > filter (interacting);
      scanner.process (filter, 1, db::box_convert<db::Edge> ());

      for (shape_interactions<db::Edge, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
        const db::Edge &subject = interactions.subject_shape (i->first);
        if (interacting.find (subject) == interacting.end ()) {
          result.insert (subject);
        }
      }

    } else {

      edge_interaction_filter<std::unordered_set<db::Edge> > filter (result);
      scanner.process (filter, 1, db::box_convert<db::Edge> ());

    }

  }

  virtual OnEmptyIntruderHint on_empty_intruder_hint () const
  {
    if (m_inverse) {
      return Copy;
    } else {
      return Drop;
    }
  }

  virtual std::string description () const
  {
    return tl::to_string (tr ("Select interacting edges"));
  }

private:
  bool m_inverse;
};

class Edge2EdgePullLocalOperation
  : public local_operation<db::Edge, db::Edge, db::Edge>
{
public:
  Edge2EdgePullLocalOperation ()
  {
    //  .. nothing yet ..
  }

  virtual db::Coord dist () const
  {
    //  touching is sufficient
    return 1;
  }

  virtual void do_compute_local (db::Layout * /*layout*/, const shape_interactions<db::Edge, db::Edge> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
  {
    tl_assert (results.size () == 1);
    std::unordered_set<db::Edge> &result = results.front ();

    db::box_scanner<db::Edge, size_t> scanner;

    std::set<db::Edge> others;
    for (shape_interactions<db::Edge, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      for (shape_interactions<db::Edge, db::Edge>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
        others.insert (interactions.intruder_shape (*j).second);
      }
    }

    for (shape_interactions<db::Edge, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      const db::Edge &subject = interactions.subject_shape (i->first);
      scanner.insert (&subject, 1);
    }

    for (std::set<db::Edge>::const_iterator o = others.begin (); o != others.end (); ++o) {
      scanner.insert (o.operator-> (), 0);
    }

    edge_interaction_filter<std::unordered_set<db::Edge> > filter (result);
    scanner.process (filter, 1, db::box_convert<db::Edge> ());

  }

  virtual OnEmptyIntruderHint on_empty_intruder_hint () const
  {
    return Drop;
  }

  virtual std::string description () const
  {
    return tl::to_string (tr ("Select interacting edges from other"));
  }
};

class Edge2PolygonInteractingLocalOperation
  : public local_operation<db::Edge, db::PolygonRef, db::Edge>
{
public:
  Edge2PolygonInteractingLocalOperation (bool inverse)
    : m_inverse (inverse)
  {
    //  .. nothing yet ..
  }

  virtual db::Coord dist () const
  {
    //  touching is sufficient
    return 1;
  }

  virtual void do_compute_local (db::Layout * /*layout*/, const shape_interactions<db::Edge, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::Edge> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
  {
    tl_assert (results.size () == 1);
    std::unordered_set<db::Edge> &result = results.front ();

    db::box_scanner2<db::Edge, size_t, db::Polygon, size_t> scanner;

    std::set<db::PolygonRef> others;
    for (shape_interactions<db::Edge, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      for (shape_interactions<db::Edge, db::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
        others.insert (interactions.intruder_shape (*j).second);
      }
    }

    for (shape_interactions<db::Edge, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      const db::Edge &subject = interactions.subject_shape (i->first);
      scanner.insert1 (&subject, 0);
    }

    std::list<db::Polygon> heap;
    for (std::set<db::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o) {
      heap.push_back (o->obj ().transformed (o->trans ()));
      scanner.insert2 (& heap.back (), 1);
    }

    if (m_inverse) {

      std::unordered_set<db::Edge> interacting;
      edge_to_region_interaction_filter<std::unordered_set<db::Edge> > filter (interacting);
      scanner.process (filter, 1, db::box_convert<db::Edge> (), db::box_convert<db::Polygon> ());

      for (shape_interactions<db::Edge, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
        const db::Edge &subject = interactions.subject_shape (i->first);
        if (interacting.find (subject) == interacting.end ()) {
          result.insert (subject);
        }
      }

    } else {

      edge_to_region_interaction_filter<std::unordered_set<db::Edge> > filter (result);
      scanner.process (filter, 1, db::box_convert<db::Edge> (), db::box_convert<db::Polygon> ());

    }
  }

  virtual OnEmptyIntruderHint on_empty_intruder_hint () const
  {
    if (m_inverse) {
      return Copy;
    } else {
      return Drop;
    }
  }

  virtual std::string description () const
  {
    return tl::to_string (tr ("Select interacting edges"));
  }

private:
  bool m_inverse;
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

class Edge2PolygonPullLocalOperation
  : public local_operation<db::Edge, db::PolygonRef, db::PolygonRef>
{
public:
  Edge2PolygonPullLocalOperation ()
  {
    //  .. nothing yet ..
  }

  virtual db::Coord dist () const
  {
    //  touching is sufficient
    return 1;
  }

  virtual void do_compute_local (db::Layout *layout, const shape_interactions<db::Edge, db::PolygonRef> &interactions, std::vector<std::unordered_set<db::PolygonRef> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
  {
    tl_assert (results.size () == 1);
    std::unordered_set<db::PolygonRef> &result = results.front ();

    db::box_scanner2<db::Edge, size_t, db::Polygon, size_t> scanner;

    std::set<db::PolygonRef> others;
    for (shape_interactions<db::Edge, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      for (shape_interactions<db::Edge, db::PolygonRef>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
        others.insert (interactions.intruder_shape (*j).second);
      }
    }

    for (shape_interactions<db::Edge, db::PolygonRef>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
      const db::Edge &subject = interactions.subject_shape (i->first);
      scanner.insert1 (&subject, 1);
    }

    std::list<db::Polygon> heap;
    for (std::set<db::PolygonRef>::const_iterator o = others.begin (); o != others.end (); ++o) {
      heap.push_back (o->obj ().transformed (o->trans ()));
      scanner.insert2 (& heap.back (), 0);
    }

    ResultInserter inserter (layout, result);
    edge_to_region_interaction_filter<ResultInserter> filter (inserter);
    scanner.process (filter, 1, db::box_convert<db::Edge> (), db::box_convert<db::Polygon> ());
  }

  virtual OnEmptyIntruderHint on_empty_intruder_hint () const
  {
    return Drop;
  }

  virtual std::string description () const
  {
    return tl::to_string (tr ("Select interacting regions"));
  }
};

}

EdgesDelegate *
DeepEdges::selected_interacting_generic (const Region &other, bool inverse) const
{
  std::unique_ptr<db::DeepRegion> dr_holder;
  const db::DeepRegion *other_deep = dynamic_cast<const db::DeepRegion *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchisation
    dr_holder.reset (new db::DeepRegion (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  const db::DeepLayer &edges = merged_deep_layer ();

  DeepLayer dl_out (edges.derived ());

  db::Edge2PolygonInteractingLocalOperation op (inverse);

  db::local_processor<db::Edge, db::PolygonRef, db::Edge> proc (const_cast<db::Layout *> (&edges.layout ()), const_cast<db::Cell *> (&edges.initial_cell ()), &other_deep->deep_layer ().layout (), &other_deep->deep_layer ().initial_cell ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (edges.store ()->threads ());

  proc.run (&op, edges.layer (), other_deep->deep_layer ().layer (), dl_out.layer ());

  return new db::DeepEdges (dl_out);
}

EdgesDelegate *
DeepEdges::selected_interacting_generic (const Edges &other, bool inverse) const
{
  std::unique_ptr<db::DeepEdges> dr_holder;
  const db::DeepEdges *other_deep = dynamic_cast<const db::DeepEdges *> (other.delegate ());
  if (! other_deep) {
    //  if the other edge collection isn't deep, turn into a top-level only deep edge collection to facilitate re-hierarchisation
    dr_holder.reset (new db::DeepEdges (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  const db::DeepLayer &edges = merged_deep_layer ();

  DeepLayer dl_out (edges.derived ());

  db::Edge2EdgeInteractingLocalOperation op (inverse);

  db::local_processor<db::Edge, db::Edge, db::Edge> proc (const_cast<db::Layout *> (&edges.layout ()), const_cast<db::Cell *> (&edges.initial_cell ()), &other_deep->deep_layer ().layout (), &other_deep->deep_layer ().initial_cell ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (edges.store ()->threads ());

  proc.run (&op, edges.layer (), other_deep->deep_layer ().layer (), dl_out.layer ());

  return new db::DeepEdges (dl_out);
}

RegionDelegate *DeepEdges::pull_generic (const Region &other) const
{
  std::unique_ptr<db::DeepRegion> dr_holder;
  const db::DeepRegion *other_deep = dynamic_cast<const db::DeepRegion *> (other.delegate ());
  if (! other_deep) {
    //  if the other region isn't deep, turn into a top-level only deep region to facilitate re-hierarchisation
    dr_holder.reset (new db::DeepRegion (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  const db::DeepLayer &edges = deep_layer ();
  const db::DeepLayer &other_polygons = other_deep->merged_deep_layer ();

  DeepLayer dl_out (other_polygons.derived ());

  db::Edge2PolygonPullLocalOperation op;

  db::local_processor<db::Edge, db::PolygonRef, db::PolygonRef> proc (const_cast<db::Layout *> (&edges.layout ()), const_cast<db::Cell *> (&edges.initial_cell ()), &other_polygons.layout (), &other_polygons.initial_cell ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (edges.store ()->threads ());

  proc.run (&op, edges.layer (), other_polygons.layer (), dl_out.layer ());

  return new db::DeepRegion (dl_out);
}

EdgesDelegate *DeepEdges::pull_generic (const Edges &other) const
{
  std::unique_ptr<db::DeepEdges> dr_holder;
  const db::DeepEdges *other_deep = dynamic_cast<const db::DeepEdges *> (other.delegate ());
  if (! other_deep) {
    //  if the other edge collection isn't deep, turn into a top-level only deep edge collection to facilitate re-hierarchisation
    dr_holder.reset (new db::DeepEdges (other, const_cast<db::DeepShapeStore &> (*deep_layer ().store ())));
    other_deep = dr_holder.get ();
  }

  const db::DeepLayer &edges = deep_layer ();
  const db::DeepLayer &other_edges = other_deep->merged_deep_layer ();

  DeepLayer dl_out (other_edges.derived ());

  db::Edge2EdgePullLocalOperation op;

  db::local_processor<db::Edge, db::Edge, db::Edge> proc (const_cast<db::Layout *> (&edges.layout ()), const_cast<db::Cell *> (&edges.initial_cell ()), &other_edges.layout (), &other_edges.initial_cell ());
  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (edges.store ()->threads ());

  proc.run (&op, edges.layer (), other_edges.layer (), dl_out.layer ());

  return new db::DeepEdges (dl_out);
}

EdgesDelegate *DeepEdges::in (const Edges &other, bool invert) const
{
  //  TODO: is there a cheaper way?
  return AsIfFlatEdges::in (other, invert);
}

namespace
{

class EdgesCheckLocalOperation
  : public local_operation<db::Edge, db::Edge, db::EdgePair>
{
public:
  EdgesCheckLocalOperation (const EdgeRelationFilter &check, bool has_other)
    : m_check (check), m_has_other (has_other)
  {
    //  .. nothing yet ..
  }

  virtual void do_compute_local (db::Layout * /*layout*/, const shape_interactions<db::Edge, db::Edge> &interactions, std::vector<std::unordered_set<db::EdgePair> > &results, size_t /*max_vertex_count*/, double /*area_ratio*/) const
  {
    tl_assert (results.size () == 1);
    std::unordered_set<db::EdgePair> &result = results.front ();

    edge2edge_check_for_edges<std::unordered_set<db::EdgePair> > edge_check (m_check, result, m_has_other);

    db::box_scanner<db::Edge, size_t> scanner;
    std::set<db::Edge> others;

    if (m_has_other) {

      for (shape_interactions<db::Edge, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
        for (shape_interactions<db::Edge, db::Edge>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
          others.insert (interactions.intruder_shape (*j).second);
        }
      }

      size_t n = 0;
      for (shape_interactions<db::Edge, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
        const db::Edge &subject = interactions.subject_shape (i->first);
        scanner.insert (& subject, n);
        n += 2;
      }

      n = 1;
      for (std::set<db::Edge>::const_iterator o = others.begin (); o != others.end (); ++o) {
        scanner.insert (o.operator-> (), n);
        n += 2;
      }

    } else {

      for (shape_interactions<db::Edge, db::Edge>::iterator i = interactions.begin (); i != interactions.end (); ++i) {
        others.insert (interactions.subject_shape (i->first));
        for (shape_interactions<db::Edge, db::Edge>::iterator2 j = i->second.begin (); j != i->second.end (); ++j) {
          others.insert (interactions.intruder_shape (*j).second);
        }
      }

      size_t n = 0;
      for (std::set<db::Edge>::const_iterator o = others.begin (); o != others.end (); ++o) {
        scanner.insert (o.operator-> (), n);
        n += 2;
      }

    }

    scanner.process (edge_check, m_check.distance (), db::box_convert<db::Edge> ());
  }

  virtual db::Coord dist () const
  {
    //  TODO: will the distance be sufficient? Or should we take somewhat more?
    return m_check.distance ();
  }

  virtual OnEmptyIntruderHint on_empty_intruder_hint () const
  {
    return Drop;
  }

  virtual std::string description () const
  {
    return tl::to_string (tr ("Generic DRC check"));
  }

private:
  EdgeRelationFilter m_check;
  bool m_has_other;
};

}

EdgePairsDelegate *
DeepEdges::run_check (db::edge_relation_type rel, const Edges *other, db::Coord d, const db::EdgesCheckOptions &options) const
{
  const db::DeepEdges *other_deep = 0;
  if (other) {
    other_deep = dynamic_cast<const db::DeepEdges *> (other->delegate ());
    if (! other_deep) {
      return db::AsIfFlatEdges::run_check (rel, other, d, options);
    }
  }

  const db::DeepLayer &edges = merged_deep_layer ();

  EdgeRelationFilter check (rel, d, options.metrics);
  check.set_include_zero (false);
  check.set_whole_edges (options.whole_edges);
  check.set_ignore_angle (options.ignore_angle);
  check.set_min_projection (options.min_projection);
  check.set_max_projection (options.max_projection);

  std::unique_ptr<db::DeepEdgePairs> res (new db::DeepEdgePairs (edges.derived ()));

  db::EdgesCheckLocalOperation op (check, other_deep != 0);

  db::local_processor<db::Edge, db::Edge, db::EdgePair> proc (const_cast<db::Layout *> (&edges.layout ()),
                                                              const_cast<db::Cell *> (&edges.initial_cell ()),
                                                              other_deep ? &other_deep->deep_layer ().layout () : const_cast<db::Layout *> (&edges.layout ()),
                                                              other_deep ? &other_deep->deep_layer ().initial_cell () : const_cast<db::Cell *> (&edges.initial_cell ()));

  proc.set_base_verbosity (base_verbosity ());
  proc.set_threads (edges.store ()->threads ());

  proc.run (&op, edges.layer (), other_deep ? other_deep->deep_layer ().layer () : edges.layer (), res->deep_layer ().layer ());

  return res.release ();
}

}
