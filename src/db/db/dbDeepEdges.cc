
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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
#include "dbDeepEdges.h"
#include "dbHierNetworkProcessor.h"
#include "dbCellGraphUtils.h"
#include "dbCellVariants.h"
#include "dbEdgeBoolean.h"
#include "dbCellMapping.h"
#include "dbLayoutUtils.h"

namespace db
{

/**
 *  @brief An iterator delegate for the deep edge collection
 *  TODO: this is kind of redundant with OriginalLayerIterator ..
 */
class DB_PUBLIC DeepEdgesIterator
  : public EdgesIteratorDelegate
{
public:
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

  virtual const value_type *get () const
  {
    return &m_edge;
  }

  virtual EdgesIteratorDelegate *clone () const
  {
    return new DeepEdgesIterator (*this);
  }

private:
  friend class Edges;

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
  : AsIfFlatEdges (), m_deep_layer (dss.create_edge_layer (si, as_edges)), m_merged_edges ()
{
  init ();
}

DeepEdges::DeepEdges (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans, bool as_edges, bool merged_semantics)
  : AsIfFlatEdges (), m_deep_layer (dss.create_edge_layer (si, as_edges)), m_merged_edges ()
{
  init ();

  tl_assert (trans.is_unity ()); // TODO: implement
  set_merged_semantics (merged_semantics);
}

DeepEdges::DeepEdges ()
  : AsIfFlatEdges ()
{
  init ();
}

DeepEdges::DeepEdges (const DeepLayer &dl)
  : AsIfFlatEdges (), m_deep_layer (dl)
{
  init ();
}

DeepEdges::~DeepEdges ()
{
  //  .. nothing yet ..
}

DeepEdges::DeepEdges (const DeepEdges &other)
  : AsIfFlatEdges (other),
    m_deep_layer (other.m_deep_layer.copy ()),
    m_merged_edges_valid (other.m_merged_edges_valid)
{
  if (m_merged_edges_valid) {
    m_merged_edges = other.m_merged_edges;
  }
}

void DeepEdges::init ()
{
  m_merged_edges_valid = false;
  m_merged_edges = db::DeepLayer ();
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
  const db::Layout &layout = m_deep_layer.layout ();
  if (layout.cells () == 0) {

    return std::make_pair (db::RecursiveShapeIterator (), db::ICplxTrans ());

  } else {

    const db::Cell &top_cell = layout.cell (*layout.begin_top_down ());
    db::RecursiveShapeIterator iter (m_deep_layer.layout (), top_cell, m_deep_layer.layer ());
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
  //  TODO: there is no such thing as a "surely merged" state except after merged() maybe
  return false;
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
  if (other_delegate && &other_delegate->m_deep_layer.layout () == &m_deep_layer.layout ()
      && other_delegate->m_deep_layer.layer () == m_deep_layer.layer ()) {
    return true;
  } else {
    return AsIfFlatEdges::equals (other);
  }
}

bool DeepEdges::less (const Edges &other) const
{
  const DeepEdges *other_delegate = dynamic_cast<const DeepEdges *> (other.delegate ());
  if (other_delegate && &other_delegate->m_deep_layer.layout () == &m_deep_layer.layout ()) {
    return other_delegate->m_deep_layer.layer () < m_deep_layer.layer ();
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
      const db::Shapes &cc_shapes = merged (i->id (), i->inst ().inst_ptr.cell_index (), false);
      merged_child_clusters.push_back (std::make_pair (&cc_shapes, i->inst ().complex_trans ()));
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
    EdgeBooleanClusterCollector<db::Shapes> cluster_collector (&s->second, EdgeOr);
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

void
DeepEdges::ensure_merged_edges_valid () const
{
  if (! m_merged_edges_valid) {

    m_merged_edges = m_deep_layer.new_layer ();

    tl::SelfTimer timer (tl::verbosity () > base_verbosity (), "Ensure merged polygons");

    db::Layout &layout = const_cast<db::Layout &> (m_deep_layer.layout ());

    db::hier_clusters<db::Edge> hc;
    db::Connectivity conn;
    conn.connect (m_deep_layer);
    //  TODO: this uses the wrong verbosity inside ...
    hc.build (layout, m_deep_layer.initial_cell (), db::ShapeIterator::Edges, conn);

    //  collect the clusters and merge them into big polygons
    //  NOTE: using the ClusterMerger we merge bottom-up forming bigger and bigger polygons. This is
    //  hopefully more efficient that collecting everything and will lead to reuse of parts.

    ClusterMerger cm (m_deep_layer.layer (), hc, report_progress (), progress_desc ());
    cm.set_base_verbosity (base_verbosity ());

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

    m_merged_edges_valid = true;

  }
}

void
DeepEdges::insert_into (db::Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  m_deep_layer.insert_into (layout, into_cell, into_layer);
}

size_t DeepEdges::size () const
{
  size_t n = 0;

  const db::Layout &layout = m_deep_layer.layout ();
  db::CellCounter cc (&layout);
  for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
    n += cc.weight (*c) * layout.cell (*c).shapes (m_deep_layer.layer ()).size ();
  }

  return n;
}

Box DeepEdges::bbox () const
{
  return m_deep_layer.initial_cell ().bbox (m_deep_layer.layer ());
}

DeepEdges::length_type DeepEdges::length (const db::Box &box) const
{
  if (box.empty ()) {

    ensure_merged_edges_valid ();

    db::MagnificationReducer red;
    db::cell_variants_collector<db::MagnificationReducer> vars (red);
    vars.collect (m_merged_edges.layout (), m_merged_edges.initial_cell ());

    DeepEdges::length_type l = 0;

    const db::Layout &layout = m_merged_edges.layout ();
    for (db::Layout::top_down_const_iterator c = layout.begin_top_down (); c != layout.end_top_down (); ++c) {
      DeepEdges::length_type lc = 0;
      for (db::ShapeIterator s = layout.cell (*c).shapes (m_merged_edges.layer ()).begin (db::ShapeIterator::Edges); ! s.at_end (); ++s) {
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

EdgesDelegate *DeepEdges::filter_in_place (const EdgeFilterBase &filter)
{
  //  TODO: implement
  return AsIfFlatEdges::filter_in_place (filter);
}

EdgesDelegate *DeepEdges::filtered (const EdgeFilterBase &filter) const
{
  //  TODO: implement
  return AsIfFlatEdges::filtered (filter);
}

EdgesDelegate *DeepEdges::merged_in_place ()
{
  ensure_merged_edges_valid ();

  //  NOTE: this makes both layers share the same resource
  m_deep_layer = m_merged_edges;

  return this;
}

EdgesDelegate *DeepEdges::merged () const
{
  ensure_merged_edges_valid ();

  db::Layout &layout = const_cast<db::Layout &> (m_merged_edges.layout ());

  std::auto_ptr<db::DeepEdges> res (new db::DeepEdges (m_merged_edges.new_layer ()));
  for (db::Layout::iterator c = layout.begin (); c != layout.end (); ++c) {
    c->shapes (res->deep_layer ().layer ()) = c->shapes (m_merged_edges.layer ());
  }

  res->deep_layer ().layer ();

  return res.release ();
}

EdgesDelegate *DeepEdges::and_with (const Edges &other) const
{
  //  TODO: implement
  return AsIfFlatEdges::and_with (other);
}

EdgesDelegate *DeepEdges::and_with (const Region &other) const
{
  //  TODO: implement
  return AsIfFlatEdges::and_with (other);
}

EdgesDelegate *DeepEdges::not_with (const Edges &other) const
{
  //  TODO: implement
  return AsIfFlatEdges::not_with (other);
}

EdgesDelegate *DeepEdges::not_with (const Region &other) const
{
  //  TODO: implement
  return AsIfFlatEdges::not_with (other);
}

EdgesDelegate *DeepEdges::xor_with (const Edges &other) const
{
  //  TODO: implement
  return AsIfFlatEdges::xor_with (other);
}

EdgesDelegate *DeepEdges::or_with (const Edges &other) const
{
  //  TODO: implement
  return AsIfFlatEdges::or_with (other);
}

void
DeepEdges::add_from (const DeepLayer &dl)
{
  if (&dl.layout () == &deep_layer ().layout ()) {

    //  intra-layout merge

    deep_layer ().layout ().copy_layer (dl.layer (), deep_layer ().layer ());

  } else {

    //  inter-layout merge

    db::cell_index_type into_cell = deep_layer ().initial_cell ().cell_index ();
    db::Layout *into_layout = &deep_layer ().layout ();
    db::cell_index_type source_cell = dl.initial_cell ().cell_index ();
    const db::Layout *source_layout = &dl.layout ();

    db::CellMapping cm;
    cm.create_from_geometry_full (*into_layout, into_cell, *source_layout, source_cell);

    //  Actually copy the shapes

    std::map<unsigned int, unsigned int> lm;
    lm.insert (std::make_pair (dl.layer (), deep_layer ().layer ()));

    std::vector <db::cell_index_type> source_cells;
    source_cells.push_back (source_cell);
    db::copy_shapes (*into_layout, *source_layout, db::ICplxTrans (), source_cells, cm.table (), lm);

  }
}

EdgesDelegate *
DeepEdges::add_in_place (const Edges &other)
{
  if (other.empty ()) {
    return this;
  }

  const DeepEdges *other_deep = dynamic_cast <const DeepEdges *> (other.delegate ());
  if (other_deep) {

    add_from (other_deep->deep_layer ());

  } else {

    //  non-deep to deep merge (flat)

    db::Shapes &shapes = deep_layer ().initial_cell ().shapes (deep_layer ().layer ());
    for (db::Edges::const_iterator p = other.begin (); ! p.at_end (); ++p) {
      shapes.insert (*p);
    }

  }

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
  //  TODO: implement
  return AsIfFlatEdges::inside_part (other);
}

EdgesDelegate *DeepEdges::outside_part (const Region &other) const
{
  //  TODO: implement
  return AsIfFlatEdges::outside_part (other);
}

RegionDelegate *DeepEdges::extended (coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i, bool join) const
{
  //  TODO: implement
  return AsIfFlatEdges::extended (ext_b, ext_e, ext_o, ext_i, join);
}

EdgesDelegate *DeepEdges::start_segments (length_type length, double fraction) const
{
  //  TODO: implement
  return AsIfFlatEdges::start_segments (length, fraction);
}

EdgesDelegate *DeepEdges::end_segments (length_type length, double fraction) const
{
  //  TODO: implement
  return AsIfFlatEdges::end_segments (length, fraction);
}

EdgesDelegate *DeepEdges::centers (length_type length, double fraction) const
{
  //  TODO: implement
  return AsIfFlatEdges::centers (length, fraction);
}

EdgesDelegate *DeepEdges::selected_interacting (const Edges &other) const
{
  //  TODO: implement
  return AsIfFlatEdges::selected_interacting (other);
}

EdgesDelegate *DeepEdges::selected_not_interacting (const Edges &other) const
{
  //  TODO: implement
  return AsIfFlatEdges::selected_not_interacting (other);
}

EdgesDelegate *DeepEdges::selected_interacting (const Region &other) const
{
  //  TODO: implement
  return AsIfFlatEdges::selected_interacting (other);
}

EdgesDelegate *DeepEdges::selected_not_interacting (const Region &other) const
{
  //  TODO: implement
  return AsIfFlatEdges::selected_not_interacting (other);
}

EdgesDelegate *DeepEdges::in (const Edges &other, bool invert) const
{
  //  TODO: implement
  return AsIfFlatEdges::in (other, invert);
}

EdgePairs DeepEdges::run_check (db::edge_relation_type rel, const Edges *other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const
{
  //  TODO: implement
  return AsIfFlatEdges::run_check (rel, other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
}

}
