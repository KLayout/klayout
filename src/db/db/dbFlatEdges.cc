
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


#include "dbFlatEdges.h"
#include "dbEmptyEdges.h"
#include "dbEdgeBoolean.h"
#include "dbEdges.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  FlatEdges implementation

FlatEdges::FlatEdges ()
  : AsIfFlatEdges (), m_edges (false), m_merged_edges (false)
{
  init ();
}

FlatEdges::~FlatEdges ()
{
  //  .. nothing yet ..
}

FlatEdges::FlatEdges (const FlatEdges &other)
  : AsIfFlatEdges (other), m_edges (false), m_merged_edges (false)
{
  init ();

  m_is_merged = other.m_is_merged;
  m_edges = other.m_edges;
  m_merged_edges = other.m_merged_edges;
  m_merged_edges_valid = other.m_merged_edges_valid;
}

FlatEdges::FlatEdges (const db::Shapes &edges, bool is_merged)
  : AsIfFlatEdges (), m_edges (edges), m_merged_edges (false)
{
  init ();

  m_is_merged = is_merged;
}

FlatEdges::FlatEdges (bool is_merged)
  : AsIfFlatEdges (), m_edges (false), m_merged_edges (false)
{
  init ();

  m_is_merged = is_merged;
}

void FlatEdges::set_is_merged (bool m)
{
  m_is_merged = m;
}

void FlatEdges::invalidate_cache ()
{
  invalidate_bbox ();
  m_merged_edges.clear ();
  m_merged_edges_valid = false;
}

void FlatEdges::init ()
{
  m_is_merged = true;
  m_merged_edges_valid = false;
}

void FlatEdges::insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  layout->cell (into_cell).shapes (into_layer).insert (m_edges);
}

void FlatEdges::merged_semantics_changed ()
{
  m_merged_edges.clear ();
  m_merged_edges_valid = false;
}

void FlatEdges::reserve (size_t n)
{
  m_edges.reserve (db::Edge::tag (), n);
}

void
FlatEdges::ensure_merged_edges_valid () const
{
  if (! m_merged_edges_valid) {

    m_merged_edges.clear ();

    db::Shapes tmp (false);
    EdgeBooleanClusterCollectorToShapes cluster_collector (&tmp, EdgeOr);

    db::box_scanner<db::Edge, size_t> scanner (report_progress (), progress_desc ());
    scanner.reserve (m_edges.size ());

    for (EdgesIterator e (begin ()); ! e.at_end (); ++e) {
      if (! e->is_degenerate ()) {
        scanner.insert (&*e, 0); 
      }
    }

    scanner.process (cluster_collector, 1, db::box_convert<db::Edge> ());

    m_merged_edges.swap (tmp);
    m_merged_edges_valid = true;

  }
}

EdgesIteratorDelegate *FlatEdges::begin () const
{
  return new FlatEdgesIterator (m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().begin (), m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().end ());
}

EdgesIteratorDelegate *FlatEdges::begin_merged () const
{
  if (! merged_semantics () || m_is_merged) {
    return begin ();
  } else {
    ensure_merged_edges_valid ();
    return new FlatEdgesIterator (m_merged_edges.get_layer<db::Edge, db::unstable_layer_tag> ().begin (), m_merged_edges.get_layer<db::Edge, db::unstable_layer_tag> ().end ());
  }
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans> FlatEdges::begin_iter () const
{
  return std::make_pair (db::RecursiveShapeIterator (m_edges), db::ICplxTrans ());
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans> FlatEdges::begin_merged_iter () const
{
  if (! merged_semantics () || m_is_merged) {
    return begin_iter ();
  } else {
    ensure_merged_edges_valid ();
    return std::make_pair (db::RecursiveShapeIterator (m_merged_edges), db::ICplxTrans ());
  }
}

bool FlatEdges::empty () const
{
  return m_edges.empty ();
}

size_t FlatEdges::size () const
{
  return m_edges.size ();
}

bool FlatEdges::is_merged () const
{
  return m_is_merged;
}

Box FlatEdges::compute_bbox () const
{
  m_edges.update_bbox ();
  return m_edges.bbox ();
}

EdgesDelegate *
FlatEdges::processed_in_place (const EdgeProcessorBase &filter)
{
  std::vector<db::Edge> edge_res;

  edge_iterator_type pw = m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().begin ();
  for (EdgesIterator p (filter.requires_raw_input () ? begin () : begin_merged ()); ! p.at_end (); ++p) {

    edge_res.clear ();
    filter.process (*p, edge_res);

    for (std::vector<db::Edge>::const_iterator pr = edge_res.begin (); pr != edge_res.end (); ++pr) {
      if (pw == m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().end ()) {
        m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().insert (*pr);
        pw = m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().end ();
      } else {
        m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().replace (pw++, *pr);
      }
    }

  }

  m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().erase (pw, m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().end ());
  m_merged_edges.clear ();
  m_is_merged = filter.result_is_merged () && merged_semantics ();

  return this;
}

EdgesDelegate *
FlatEdges::filter_in_place (const EdgeFilterBase &filter)
{
  edge_iterator_type pw = m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().begin ();
  for (EdgesIterator p (begin_merged ()); ! p.at_end (); ++p) {
    if (filter.selected (*p)) {
      if (pw == m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().end ()) {
        m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().insert (*p);
        pw = m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().end ();
      } else {
        m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().replace (pw++, *p);
      }
    }
  }

  m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().erase (pw, m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().end ());
  m_merged_edges.clear ();
  m_is_merged = merged_semantics ();

  return this;
}

EdgesDelegate *FlatEdges::add (const Edges &other) const
{
  std::auto_ptr<FlatEdges> new_region (new FlatEdges (*this));
  new_region->invalidate_cache ();
  new_region->set_is_merged (false);

  FlatEdges *other_flat = dynamic_cast<FlatEdges *> (other.delegate ());
  if (other_flat) {

    new_region->raw_edges ().insert (other_flat->raw_edges ().get_layer<db::Edge, db::unstable_layer_tag> ().begin (), other_flat->raw_edges ().get_layer<db::Edge, db::unstable_layer_tag> ().end ());

  } else {

    size_t n = new_region->raw_edges ().size ();
    for (EdgesIterator p (other.begin ()); ! p.at_end (); ++p) {
      ++n;
    }

    new_region->raw_edges ().reserve (db::Edge::tag (), n);

    for (EdgesIterator p (other.begin ()); ! p.at_end (); ++p) {
      new_region->raw_edges ().insert (*p);
    }

  }

  return new_region.release ();
}

EdgesDelegate *FlatEdges::add_in_place (const Edges &other)
{
  invalidate_cache ();
  m_is_merged = false;

  FlatEdges *other_flat = dynamic_cast<FlatEdges *> (other.delegate ());
  if (other_flat) {

    m_edges.insert (other_flat->raw_edges ().get_layer<db::Edge, db::unstable_layer_tag> ().begin (), other_flat->raw_edges ().get_layer<db::Edge, db::unstable_layer_tag> ().end ());

  } else {

    size_t n = m_edges.size ();
    for (EdgesIterator p (other.begin ()); ! p.at_end (); ++p) {
      ++n;
    }

    m_edges.reserve (db::Edge::tag (), n);

    for (EdgesIterator p (other.begin ()); ! p.at_end (); ++p) {
      m_edges.insert (*p);
    }

  }

  return this;
}

const db::Edge *FlatEdges::nth (size_t n) const
{
  return n < m_edges.size () ? &m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().begin () [n] : 0;
}

bool FlatEdges::has_valid_edges () const
{
  return true;
}

bool FlatEdges::has_valid_merged_edges () const
{
  return true;
}

const db::RecursiveShapeIterator *FlatEdges::iter () const
{
  return 0;
}

void
FlatEdges::insert (const db::Box &box)
{
  if (! box.empty () && box.width () > 0 && box.height () > 0) {

    bool was_empty = empty ();

    m_edges.insert (db::Edge (box.lower_left (), box.upper_left ()));
    m_edges.insert (db::Edge (box.upper_left (), box.upper_right ()));
    m_edges.insert (db::Edge (box.upper_right (), box.lower_right ()));
    m_edges.insert (db::Edge (box.lower_right (), box.lower_left ()));

    if (was_empty) {

      m_is_merged = true;
      update_bbox (box);

    } else {

      m_is_merged = false;
      invalidate_cache ();

    }

  }
}

void
FlatEdges::insert (const db::Path &path)
{
  if (path.points () > 0) {
    insert (path.polygon ());
  }
}

void
FlatEdges::insert (const db::Polygon &polygon)
{
  if (polygon.holes () > 0 || polygon.vertices () > 0) {
    for (db::Polygon::polygon_edge_iterator e = polygon.begin_edge (); ! e.at_end (); ++e) {
      m_edges.insert (*e);
    }
    m_is_merged = false;
    invalidate_cache ();
  }
}

void
FlatEdges::insert (const db::SimplePolygon &polygon)
{
  if (polygon.vertices () > 0) {
    for (db::SimplePolygon::polygon_edge_iterator e = polygon.begin_edge (); ! e.at_end (); ++e) {
      m_edges.insert (*e);
    }
    m_is_merged = false;
    invalidate_cache ();
  }
}

void
FlatEdges::insert (const db::Edge &edge)
{
  if (! empty ()) {
    m_is_merged = false;
  }

  m_edges.insert (edge);
  invalidate_cache ();
}

void
FlatEdges::insert (const db::Shape &shape)
{
  if (shape.is_polygon () || shape.is_path () || shape.is_box ()) {

    db::Polygon poly;
    shape.polygon (poly);
    insert (poly);

  } else if (shape.is_edge ()) {

    db::Edge edge;
    shape.edge (edge);
    insert (edge);

  }
}

}

