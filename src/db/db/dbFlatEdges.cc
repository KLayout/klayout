
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


#include "dbFlatEdges.h"
#include "dbEmptyEdges.h"
#include "dbEdgeBoolean.h"
#include "dbEdges.h"

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  FlatEdges implementation

FlatEdges::FlatEdges ()
  : MutableEdges (), mp_edges (new db::Shapes (false)), mp_merged_edges (new db::Shapes (false)), mp_properties_repository (new db::PropertiesRepository ())
{
  init ();
}

FlatEdges::~FlatEdges ()
{
  //  .. nothing yet ..
}

FlatEdges::FlatEdges (const FlatEdges &other)
  : MutableEdges (other), mp_edges (other.mp_edges), mp_merged_edges (other.mp_merged_edges), mp_properties_repository (other.mp_properties_repository)
{
  init ();

  m_is_merged = other.m_is_merged;
  m_merged_edges_valid = other.m_merged_edges_valid;
}

FlatEdges::FlatEdges (const db::Shapes &edges, bool is_merged)
  : MutableEdges (), mp_edges (new db::Shapes (edges)), mp_merged_edges (new db::Shapes (false)), mp_properties_repository (new db::PropertiesRepository ())
{
  init ();

  m_is_merged = is_merged;
}

FlatEdges::FlatEdges (bool is_merged)
  : MutableEdges (), mp_edges (new db::Shapes (false)), mp_merged_edges (new db::Shapes (false)), mp_properties_repository (new db::PropertiesRepository ())
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
  mp_merged_edges->clear ();
  m_merged_edges_valid = false;
}

void FlatEdges::init ()
{
  m_is_merged = false;
  m_merged_edges_valid = false;
}

void FlatEdges::insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  db::PropertyMapper pm (&layout->properties_repository (), mp_properties_repository.get_const ());
  layout->cell (into_cell).shapes (into_layer).insert (*mp_edges, pm);
}

void FlatEdges::merged_semantics_changed ()
{
  mp_merged_edges->clear ();
  m_merged_edges_valid = false;
}

void FlatEdges::reserve (size_t n)
{
  mp_edges->reserve (db::Edge::tag (), n);
}

void
FlatEdges::ensure_merged_edges_valid () const
{
  if (! m_merged_edges_valid) {

    mp_merged_edges->clear ();

    db::box_scanner<db::Edge, size_t> scanner (report_progress (), progress_desc ());

    //  count edges and reserve memory
    size_t n = 0;
    db::properties_id_type prop_id = 0;
    bool need_split_props = false;
    for (EdgesIterator s (begin ()); ! s.at_end (); ++s, ++n) {
      if (n == 0) {
        prop_id = s.prop_id ();
      } else if (! need_split_props && prop_id != s.prop_id ()) {
        need_split_props = true;
      }
    }

    db::Shapes tmp (false);

    if (! need_split_props) {

      EdgeBooleanClusterCollectorToShapes cluster_collector (&tmp, EdgeOr);

      scanner.reserve (mp_edges->size ());

      for (EdgesIterator e (begin ()); ! e.at_end (); ++e) {
        if (! e->is_degenerate ()) {
          scanner.insert (&*e, 0);
        }
      }

      scanner.process (cluster_collector, 1, db::box_convert<db::Edge> ());

    } else {

      std::map<db::properties_id_type, std::vector<const db::Edge *> > edges_by_props;

      for (EdgesIterator e (begin ()); ! e.at_end (); ++e) {
        if (! e->is_degenerate ()) {
          edges_by_props [e.prop_id ()].push_back (e.operator-> ());
        }
      }

      for (auto s2p = edges_by_props.begin (); s2p != edges_by_props.end (); ++s2p) {

        EdgeBooleanClusterCollectorToShapes cluster_collector (&tmp, EdgeOr, s2p->first);

        scanner.clear ();
        scanner.reserve (s2p->second.size ());

        for (auto s = s2p->second.begin (); s != s2p->second.end (); ++s) {
          scanner.insert (*s, 0);
        }

        scanner.process (cluster_collector, 1, db::box_convert<db::Edge> ());

      }

    }

    mp_merged_edges->swap (tmp);
    m_merged_edges_valid = true;

  }
}

EdgesIteratorDelegate *FlatEdges::begin () const
{
  return new FlatEdgesIterator (mp_edges.get_const ());
}

EdgesIteratorDelegate *FlatEdges::begin_merged () const
{
  if (! merged_semantics () || m_is_merged) {
    return begin ();
  } else {
    ensure_merged_edges_valid ();
    return new FlatEdgesIterator (mp_merged_edges.get_const ());
  }
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans> FlatEdges::begin_iter () const
{
  return std::make_pair (db::RecursiveShapeIterator (*mp_edges), db::ICplxTrans ());
}

std::pair<db::RecursiveShapeIterator, db::ICplxTrans> FlatEdges::begin_merged_iter () const
{
  if (! merged_semantics () || m_is_merged) {
    return begin_iter ();
  } else {
    ensure_merged_edges_valid ();
    return std::make_pair (db::RecursiveShapeIterator (*mp_merged_edges), db::ICplxTrans ());
  }
}

bool FlatEdges::empty () const
{
  return mp_edges->empty ();
}

size_t FlatEdges::count () const
{
  return mp_edges->size ();
}

size_t FlatEdges::hier_count () const
{
  return mp_edges->size ();
}

bool FlatEdges::is_merged () const
{
  return m_is_merged;
}

Box FlatEdges::compute_bbox () const
{
  return mp_edges->bbox ();
}

EdgesDelegate *
FlatEdges::processed_in_place (const EdgeProcessorBase &filter)
{
  std::vector<db::Edge> edge_res;

  db::Shapes &e = *mp_edges;

  edge_iterator_type pw = e.get_layer<db::Edge, db::unstable_layer_tag> ().begin ();
  edge_iterator_wp_type pw_wp = e.get_layer<db::EdgeWithProperties, db::unstable_layer_tag> ().begin ();

  for (EdgesIterator p (filter.requires_raw_input () ? begin () : begin_merged ()); ! p.at_end (); ++p) {

    edge_res.clear ();
    filter.process (*p, edge_res);

    for (std::vector<db::Edge>::const_iterator pr = edge_res.begin (); pr != edge_res.end (); ++pr) {
      if (p.prop_id () != 0) {
        if (pw_wp == e.get_layer<db::EdgeWithProperties, db::unstable_layer_tag> ().end ()) {
          e.get_layer<db::EdgeWithProperties, db::unstable_layer_tag> ().insert (db::EdgeWithProperties (*pr, p.prop_id ()));
          pw_wp = e.get_layer<db::EdgeWithProperties, db::unstable_layer_tag> ().end ();
        } else {
          e.get_layer<db::EdgeWithProperties, db::unstable_layer_tag> ().replace (pw_wp++, db::EdgeWithProperties (*pr, p.prop_id ()));
        }
      } else {
        if (pw == e.get_layer<db::Edge, db::unstable_layer_tag> ().end ()) {
          e.get_layer<db::Edge, db::unstable_layer_tag> ().insert (*pr);
          pw = e.get_layer<db::Edge, db::unstable_layer_tag> ().end ();
        } else {
          e.get_layer<db::Edge, db::unstable_layer_tag> ().replace (pw++, *pr);
        }
      }
    }

  }

  e.get_layer<db::Edge, db::unstable_layer_tag> ().erase (pw, e.get_layer<db::Edge, db::unstable_layer_tag> ().end ());
  e.get_layer<db::EdgeWithProperties, db::unstable_layer_tag> ().erase (pw_wp, e.get_layer<db::EdgeWithProperties, db::unstable_layer_tag> ().end ());

  mp_merged_edges->clear ();
  m_is_merged = filter.result_is_merged () && merged_semantics ();

  return this;
}

EdgesDelegate *
FlatEdges::filter_in_place (const EdgeFilterBase &filter)
{
  db::Shapes &e = *mp_edges;

  edge_iterator_type pw = e.get_layer<db::Edge, db::unstable_layer_tag> ().begin ();
  edge_iterator_wp_type pw_wp = e.get_layer<db::EdgeWithProperties, db::unstable_layer_tag> ().begin ();

  for (EdgesIterator p (begin_merged ()); ! p.at_end (); ++p) {
    if (filter.selected (*p)) {
      if (p.prop_id () != 0) {
        if (pw_wp == e.get_layer<db::EdgeWithProperties, db::unstable_layer_tag> ().end ()) {
          e.get_layer<db::EdgeWithProperties, db::unstable_layer_tag> ().insert (db::EdgeWithProperties (*p, p.prop_id ()));
          pw_wp = e.get_layer<db::EdgeWithProperties, db::unstable_layer_tag> ().end ();
        } else {
          e.get_layer<db::EdgeWithProperties, db::unstable_layer_tag> ().replace (pw_wp++, db::EdgeWithProperties (*p, p.prop_id ()));
        }
      } else {
        if (pw == e.get_layer<db::Edge, db::unstable_layer_tag> ().end ()) {
          e.get_layer<db::Edge, db::unstable_layer_tag> ().insert (*p);
          pw = e.get_layer<db::Edge, db::unstable_layer_tag> ().end ();
        } else {
          e.get_layer<db::Edge, db::unstable_layer_tag> ().replace (pw++, *p);
        }
      }
    }
  }

  e.get_layer<db::Edge, db::unstable_layer_tag> ().erase (pw, e.get_layer<db::Edge, db::unstable_layer_tag> ().end ());
  e.get_layer<db::EdgeWithProperties, db::unstable_layer_tag> ().erase (pw_wp, e.get_layer<db::EdgeWithProperties, db::unstable_layer_tag> ().end ());

  mp_merged_edges->clear ();
  m_is_merged = merged_semantics ();

  return this;
}

EdgesDelegate *FlatEdges::add (const Edges &other) const
{
  std::unique_ptr<FlatEdges> new_region (new FlatEdges (*this));
  new_region->invalidate_cache ();
  new_region->set_is_merged (false);

  const FlatEdges *other_flat = dynamic_cast<const FlatEdges *> (other.delegate ());
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

  db::Shapes &e = *mp_edges;

  const FlatEdges *other_flat = dynamic_cast<const FlatEdges *> (other.delegate ());
  if (other_flat) {

    e.insert (other_flat->raw_edges ().get_layer<db::Edge, db::unstable_layer_tag> ().begin (), other_flat->raw_edges ().get_layer<db::Edge, db::unstable_layer_tag> ().end ());

  } else {

    size_t n = e.size ();
    for (EdgesIterator p (other.begin ()); ! p.at_end (); ++p) {
      ++n;
    }

    e.reserve (db::Edge::tag (), n);

    for (EdgesIterator p (other.begin ()); ! p.at_end (); ++p) {
      e.insert (*p);
    }

  }

  return this;
}

const db::Edge *FlatEdges::nth (size_t n) const
{
  return n < mp_edges->size () ? &mp_edges->get_layer<db::Edge, db::unstable_layer_tag> ().begin () [n] : 0;
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

void FlatEdges::apply_property_translator (const db::PropertiesTranslator &pt)
{
  if ((mp_edges->type_mask () & db::ShapeIterator::Properties) != 0) {

    db::Shapes new_edges (mp_edges->is_editable ());
    new_edges.assign (*mp_edges, pt);
    mp_edges->swap (new_edges);

    invalidate_cache ();

  }
}

db::PropertiesRepository *FlatEdges::properties_repository ()
{
  return mp_properties_repository.get_non_const ();
}

const db::PropertiesRepository *FlatEdges::properties_repository () const
{
  return mp_properties_repository.get_const ();
}

void
FlatEdges::do_insert (const db::Edge &edge, db::properties_id_type prop_id)
{
  m_is_merged = empty ();

  if (prop_id == 0) {
    mp_edges->insert (edge);
  } else {
    mp_edges->insert (db::EdgeWithProperties (edge, prop_id));
  }

  invalidate_cache ();
}

}

