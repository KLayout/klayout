
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


#include "dbAsIfFlatRegion.h"
#include "dbFlatRegion.h"
#include "dbFlatEdgePairs.h"
#include "dbFlatEdges.h"
#include "dbFlatTexts.h"
#include "dbEmptyRegion.h"
#include "dbEmptyEdgePairs.h"
#include "dbEmptyEdges.h"
#include "dbRegion.h"
#include "dbRegionUtils.h"
#include "dbShapeProcessor.h"
#include "dbBoxConvert.h"
#include "dbBoxScanner.h"
#include "dbClip.h"
#include "dbPolygonTools.h"
#include "dbHash.h"
#include "dbRegionLocalOperations.h"
#include "dbHierProcessor.h"
#include "dbCompoundOperation.h"
#include "dbLayoutToNetlist.h"

#include <sstream>

namespace db
{

namespace {

struct ResultCountingInserter
{
  typedef db::Polygon value_type;

  ResultCountingInserter (std::unordered_map<const db::Polygon *, size_t, std::ptr_hash_from_value<db::Polygon> > &result)
    : mp_result (&result)
  {
    //  .. nothing yet ..
  }

  void insert (const db::Polygon &p)
  {
    (*mp_result)[&p] += 1;
  }

  void init (const db::Polygon *p)
  {
    (*mp_result)[p] = 0;
  }

private:
  std::unordered_map<const db::Polygon *, size_t, std::ptr_hash_from_value<db::Polygon> > *mp_result;
};

}

static
RegionDelegate *region_from_box (const db::Box &b, const db::PropertiesRepository *pr, db::properties_id_type prop_id)
{
  if (! b.empty () && b.width () > 0 && b.height () > 0) {
    FlatRegion *new_region = new FlatRegion ();
    if (prop_id != 0) {
      db::PropertyMapper pm (const_cast<db::PropertiesRepository *> (new_region->properties_repository ()), pr);
      new_region->insert (db::BoxWithProperties (b, pm (prop_id)));
    } else {
      new_region->insert (b);
    }
    return new_region;
  } else {
    return new EmptyRegion ();
  }
}

// -------------------------------------------------------------------------------------------------------------
//  AsIfFlagRegion implementation

AsIfFlatRegion::AsIfFlatRegion ()
  : RegionDelegate (), m_bbox_valid (false)
{
  //  .. nothing yet ..
}

AsIfFlatRegion::~AsIfFlatRegion ()
{
  //  .. nothing yet ..
}

AsIfFlatRegion::AsIfFlatRegion (const AsIfFlatRegion &other)
  : RegionDelegate (other), m_bbox_valid (false)
{
  operator= (other);
}

AsIfFlatRegion &
AsIfFlatRegion::operator= (const AsIfFlatRegion &other)
{
  if (this != &other) {
    m_bbox_valid = other.m_bbox_valid;
    m_bbox = other.m_bbox;
  }

  return *this;
}

std::string
AsIfFlatRegion::to_string (size_t nmax) const
{
  std::ostringstream os;
  RegionIterator p (begin ());
  bool first = true;
  for ( ; ! p.at_end () && nmax != 0; ++p, --nmax) {
    if (! first) {
      os << ";";
    }
    first = false;
    os << p->to_string ();
  }
  if (! p.at_end ()) {
    os << "...";
  }
  return os.str ();
}

EdgesDelegate *
AsIfFlatRegion::edges (const EdgeFilterBase *filter) const
{
  std::unique_ptr<FlatEdges> result (new FlatEdges ());
  db::PropertyMapper pm (result->properties_repository (), properties_repository ());

  size_t n = 0;
  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    n += p->vertices ();
  }
  result->reserve (n);

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    db::properties_id_type prop_id = p.prop_id ();
    for (db::Polygon::polygon_edge_iterator e = p->begin_edge (); ! e.at_end (); ++e) {
      if (! filter || filter->selected (*e)) {
        if (prop_id != 0) {
          result->insert (db::EdgeWithProperties (*e, pm (prop_id)));
        } else {
          result->insert (*e);
        }
      }
    }
  }

  return result.release ();
}

bool
AsIfFlatRegion::is_box () const
{
  RegionIterator p (begin ());
  if (p.at_end ()) {
    return false;
  } else {
    const db::Polygon &poly = *p;
    ++p;
    if (! p.at_end ()) {
      return false;
    } else {
      return poly.is_box ();
    }
  }
}

AsIfFlatRegion::area_type
AsIfFlatRegion::area (const db::Box &box) const
{
  area_type a = 0;

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    if (box.empty () || p->box ().inside (box)) {
      a += p->area ();
    } else {
      std::vector<db::Polygon> clipped;
      clip_poly (*p, box, clipped);
      for (std::vector<db::Polygon>::const_iterator c = clipped.begin (); c != clipped.end (); ++c) {
        a += c->area ();
      }
    }
  }

  return a;
}

AsIfFlatRegion::perimeter_type
AsIfFlatRegion::perimeter (const db::Box &box) const
{
  perimeter_type d = 0;

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {

    if (box.empty () || p->box ().inside (box)) {
      d += p->perimeter ();
    } else {

      for (db::Polygon::polygon_edge_iterator e = p->begin_edge (); ! e.at_end (); ++e) {

        if (box.empty ()) {
          d += (*e).length ();
        } else {

          std::pair<bool, db::Edge> ce = (*e).clipped (box);
          if (ce.first) {

            db::Coord dx = ce.second.dx ();
            db::Coord dy = ce.second.dy ();
            db::Coord x = ce.second.p1 ().x ();
            db::Coord y = ce.second.p1 ().y ();
            if ((dx == 0 && x == box.left ()   && dy < 0) ||
                (dx == 0 && x == box.right ()  && dy > 0) ||
                (dy == 0 && y == box.top ()    && dx < 0) ||
                (dy == 0 && y == box.bottom () && dx > 0)) {
              //  not counted -> box is at outside side of the edge
            } else {
              d += ce.second.length ();
            }

          }

        }

      }

    }

  }

  return d;
}

Box AsIfFlatRegion::bbox () const
{
  if (! m_bbox_valid) {
    m_bbox = compute_bbox ();
    m_bbox_valid = true;
  }
  return m_bbox;
}

Box AsIfFlatRegion::compute_bbox () const
{
  db::Box b;
  for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
    b += p->box ();
  }
  return b;
}

void AsIfFlatRegion::update_bbox (const db::Box &b)
{
  m_bbox = b;
  m_bbox_valid = true;
}

void AsIfFlatRegion::invalidate_bbox ()
{
  m_bbox_valid = false;
}

void AsIfFlatRegion::merge_polygons_to (db::Shapes &output, bool min_coherence, unsigned int min_wc, db::PropertiesRepository *target_rp) const
{
  db::PropertyMapper pm (target_rp, properties_repository ());

  db::EdgeProcessor ep (report_progress (), progress_desc ());
  ep.set_base_verbosity (base_verbosity ());

  //  count edges and reserve memory
  size_t n = 0;
  db::properties_id_type prop_id = 0;
  bool need_split_props = false;
  for (RegionIterator s (begin ()); ! s.at_end (); ++s, ++n) {
    if (n == 0) {
      prop_id = s.prop_id ();
    } else if (! need_split_props && prop_id != s.prop_id ()) {
      need_split_props = true;
    }
  }

  if (need_split_props) {

    db::Shapes result (output.is_editable ());

    std::vector<std::pair<db::properties_id_type, const db::Polygon *> > polygons_by_prop_id;
    polygons_by_prop_id.reserve (n);

    db::AddressablePolygonDelivery addressable_polygons (begin ());
    while (! addressable_polygons.at_end ()) {
      polygons_by_prop_id.push_back (std::make_pair (addressable_polygons.prop_id (), addressable_polygons.operator-> ()));
      addressable_polygons.inc ();
    }

    std::sort (polygons_by_prop_id.begin (), polygons_by_prop_id.end ());

    for (auto p = polygons_by_prop_id.begin (); p != polygons_by_prop_id.end (); ) {

      auto pp = p;
      while (pp != polygons_by_prop_id.end () && pp->first == p->first) {
        ++pp;
      }

      ep.clear ();

      n = 0;
      for (auto i = p; i != pp; ++i) {
        n += i->second->vertices ();
      }
      ep.reserve (n);

      n = 0;
      for (auto i = p; i != pp; ++i, ++n) {
        ep.insert (*i->second, n);
      }

      //  and run the merge step
      db::MergeOp op (min_wc);
      db::ShapeGenerator pc (result, false /*don't clear*/, pm (p->first));
      db::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence);
      ep.process (pg, op);

      p = pp;

    }

    output.swap (result);

  } else {

    n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, ++n) {
      ep.insert (*p, n);
    }

    output.clear ();

    //  and run the merge step
    db::MergeOp op (min_wc);
    db::ShapeGenerator pc (output, false /*don't clear*/, pm (prop_id));
    db::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence);
    ep.process (pg, op);

  }
}

RegionDelegate *
AsIfFlatRegion::filtered (const PolygonFilterBase &filter) const
{
  std::unique_ptr<FlatRegion> new_region (new FlatRegion ());

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    if (filter.selected (*p)) {
      new_region->insert (*p);
    }
  }

  new_region->set_is_merged (true);
  return new_region.release ();
}

RegionDelegate *
AsIfFlatRegion::processed (const PolygonProcessorBase &filter) const
{
  std::unique_ptr<FlatRegion> new_region (new FlatRegion ());
  if (filter.result_must_not_be_merged ()) {
    new_region->set_merged_semantics (false);
  }

  std::vector<db::Polygon> poly_res;

  for (RegionIterator p (filter.requires_raw_input () ? begin () : begin_merged ()); ! p.at_end (); ++p) {

    poly_res.clear ();
    filter.process (*p, poly_res);
    for (std::vector<db::Polygon>::const_iterator pr = poly_res.begin (); pr != poly_res.end (); ++pr) {
      if (p.prop_id () != 0) {
        new_region->insert (db::PolygonWithProperties (*pr, p.prop_id ()));
      } else {
        new_region->insert (*pr);
      }
    }

  }

  return new_region.release ();
}

EdgesDelegate *
AsIfFlatRegion::processed_to_edges (const PolygonToEdgeProcessorBase &filter) const
{
  std::unique_ptr<FlatEdges> new_edges (new FlatEdges ());
  if (filter.result_must_not_be_merged ()) {
    new_edges->set_merged_semantics (false);
  }

  std::vector<db::Edge> edge_res;

  for (RegionIterator p (filter.requires_raw_input () ? begin () : begin_merged ()); ! p.at_end (); ++p) {

    edge_res.clear ();
    filter.process (*p, edge_res);
    for (std::vector<db::Edge>::const_iterator er = edge_res.begin (); er != edge_res.end (); ++er) {
      if (p.prop_id () != 0) {
        new_edges->insert (db::EdgeWithProperties (*er, p.prop_id ()));
      } else {
        new_edges->insert (*er);
      }
    }

  }

  return new_edges.release ();
}

EdgePairsDelegate *
AsIfFlatRegion::processed_to_edge_pairs (const PolygonToEdgePairProcessorBase &filter) const
{
  std::unique_ptr<FlatEdgePairs> new_edge_pairs (new FlatEdgePairs ());
  if (filter.result_must_not_be_merged ()) {
    new_edge_pairs->set_merged_semantics (false);
  }

  std::vector<db::EdgePair> edge_pair_res;

  for (RegionIterator p (filter.requires_raw_input () ? begin () : begin_merged ()); ! p.at_end (); ++p) {

    edge_pair_res.clear ();
    filter.process (*p, edge_pair_res);
    for (std::vector<db::EdgePair>::const_iterator epr = edge_pair_res.begin (); epr != edge_pair_res.end (); ++epr) {
      if (p.prop_id () != 0) {
        new_edge_pairs->insert (db::EdgePairWithProperties (*epr, p.prop_id ()));
      } else {
        new_edge_pairs->insert (*epr);
      }
    }

  }

  return new_edge_pairs.release ();
}

namespace {

class OutputPairHolder
{
public:
  OutputPairHolder (InteractingOutputMode output_mode, bool merged_semantics)
  {
    if (output_mode == None) {
      return;
    }

    if (output_mode == Positive || output_mode == Negative || output_mode == PositiveAndNegative) {
      m_positive.reset (new FlatRegion (merged_semantics));
      m_results.push_back (& m_positive->raw_polygons ());
    } else {
      m_results.push_back ((db::Shapes *) 0);
    }

    if (output_mode == PositiveAndNegative) {
      m_negative.reset (new FlatRegion (merged_semantics));
      m_results.push_back (& m_negative->raw_polygons ());
    }
  }

  std::pair<RegionDelegate *, RegionDelegate *> region_pair ()
  {
    return std::make_pair (m_positive.release (), m_negative.release ());
  }

  const std::vector<db::Shapes *> &results () { return m_results; }

private:
  std::unique_ptr<FlatRegion> m_positive, m_negative;
  std::vector<db::Shapes *> m_results;
};

}

std::pair<RegionDelegate *, RegionDelegate *>
AsIfFlatRegion::in_and_out_generic (const Region &other, InteractingOutputMode output_mode) const
{
  OutputPairHolder oph (output_mode, merged_semantics ());

  if (output_mode == None) {
    return oph.region_pair ();
  }

  //  shortcut
  if (empty ()) {
    if (output_mode == Positive || output_mode == Negative) {
      return std::make_pair (clone (), (RegionDelegate *) 0);
    } else {
      return std::make_pair (clone (), clone ());
    }
  } else if (other.empty ()) {
    if (output_mode == Positive) {
      return std::make_pair (new EmptyRegion (), (RegionDelegate *) 0);
    } else if (output_mode == Negative) {
      return std::make_pair (clone (), (RegionDelegate *) 0);
    } else {
      return std::make_pair (new EmptyRegion (), clone ());
    }
  }

  std::set <db::Polygon> op;
  for (RegionIterator o (other.begin_merged ()); ! o.at_end (); ++o) {
    op.insert (*o);
  }

  std::unique_ptr<FlatRegion> new_region (new FlatRegion (false));

  for (RegionIterator o (begin_merged ()); ! o.at_end (); ++o) {
    if (op.find (*o) != op.end ()) {
      if (output_mode == Positive || output_mode == PositiveAndNegative) {
        oph.results () [0]->insert (*o);
      }
    } else {
      if (output_mode == Negative) {
        oph.results () [0]->insert (*o);
      } else if (output_mode == PositiveAndNegative) {
        oph.results () [1]->insert (*o);
      }
    }
  }

  return oph.region_pair ();
}

std::pair<RegionDelegate *, RegionDelegate *>
AsIfFlatRegion::selected_interacting_generic (const Edges &other, InteractingOutputMode output_mode, size_t min_count, size_t max_count) const
{
  OutputPairHolder oph (output_mode, merged_semantics () || is_merged ());

  if (output_mode == None) {
    return oph.region_pair ();
  }

  min_count = std::max (size_t (1), min_count);

  //  shortcut
  if (empty ()) {
    if (output_mode == Positive || output_mode == Negative) {
      return std::make_pair (clone (), (RegionDelegate *) 0);
    } else {
      return std::make_pair (clone (), clone ());
    }
  } else if (max_count < min_count || other.empty ()) {
    if (output_mode == Positive) {
      return std::make_pair (new EmptyRegion (), (RegionDelegate *) 0);
    } else if (output_mode == Negative) {
      return std::make_pair (clone (), (RegionDelegate *) 0);
    } else {
      return std::make_pair (new EmptyRegion (), clone ());
    }
  }

  bool counting = !(min_count == 1 && max_count == std::numeric_limits<size_t>::max ());

  db::RegionIterator polygons (begin_merged ());

  db::interacting_with_edge_local_operation<db::Polygon, db::Edge, db::Polygon> op (output_mode, min_count, max_count, true);

  db::local_processor<db::Polygon, db::Edge, db::Polygon> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<db::Edge> > others;
  others.push_back (counting ? other.begin_merged () : other.begin ());

  std::unique_ptr<FlatRegion> output (new FlatRegion (merged_semantics ()));
  std::vector<db::Shapes *> results;
  results.push_back (&output->raw_polygons ());

  proc.run_flat (polygons, others, std::vector<bool> (), &op, oph.results ());

  return oph.region_pair ();
}

std::pair<RegionDelegate *, RegionDelegate *>
AsIfFlatRegion::selected_interacting_generic (const Texts &other, InteractingOutputMode output_mode, size_t min_count, size_t max_count) const
{
  OutputPairHolder oph (output_mode, merged_semantics () || is_merged ());

  if (output_mode == None) {
    return oph.region_pair ();
  }

  min_count = std::max (size_t (1), min_count);

  //  shortcut
  if (empty ()) {
    if (output_mode == Positive || output_mode == Negative) {
      return std::make_pair (clone (), (RegionDelegate *) 0);
    } else {
      return std::make_pair (clone (), clone ());
    }
  } else if (max_count < min_count || other.empty ()) {
    if (output_mode == Positive) {
      return std::make_pair (new EmptyRegion (), (RegionDelegate *) 0);
    } else if (output_mode == Negative) {
      return std::make_pair (clone (), (RegionDelegate *) 0);
    } else {
      return std::make_pair (new EmptyRegion (), clone ());
    }
  }

  db::RegionIterator polygons (begin_merged ());

  db::interacting_with_text_local_operation<db::Polygon, db::Text, db::Polygon> op (output_mode, min_count, max_count);

  db::local_processor<db::Polygon, db::Text, db::Polygon> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<db::Text> > others;
  others.push_back (other.begin ());

  proc.run_flat (polygons, others, std::vector<bool> (), &op, oph.results ());

  return oph.region_pair ();
}


std::pair<RegionDelegate *, RegionDelegate *>
AsIfFlatRegion::selected_interacting_generic (const Region &other, int mode, bool touching, InteractingOutputMode output_mode, size_t min_count, size_t max_count) const
{
  OutputPairHolder oph (output_mode, merged_semantics () || is_merged ());

  if (output_mode == None) {
    return oph.region_pair ();
  }

  min_count = std::max (size_t (1), min_count);

  //  shortcut
  if (empty ()) {
    if (output_mode == Positive || output_mode == Negative) {
      return std::make_pair (clone (), (RegionDelegate *) 0);
    } else {
      return std::make_pair (clone (), clone ());
    }
  } else if (max_count < min_count || other.empty ()) {
    //  clear, if b is empty and
    //   * mode is inside, enclosing or interacting and inverse is false ("inside" or "interacting")
    //   * mode is outside and inverse is true ("not outside")
    if ((mode <= 0)) {
      if (output_mode == Positive) {
        return std::make_pair (new EmptyRegion (), (RegionDelegate *) 0);
      } else if (output_mode == Negative) {
        return std::make_pair (clone (), (RegionDelegate *) 0);
      } else {
        return std::make_pair (new EmptyRegion (), clone ());
      }
    } else {
      if (output_mode == Positive) {
        return std::make_pair (clone(), (RegionDelegate *) 0);
      } else if (output_mode == Negative) {
        return std::make_pair (new EmptyRegion (), (RegionDelegate *) 0);
      } else {
        return std::make_pair (clone (), new EmptyRegion ());
      }
    }
  }

  bool counting = !(min_count == 1 && max_count == std::numeric_limits<size_t>::max ());

  db::RegionIterator polygons (begin_merged ());

  db::interacting_local_operation<db::Polygon, db::Polygon, db::Polygon> op (mode, touching, output_mode, min_count, max_count, true);

  db::local_processor<db::Polygon, db::Polygon, db::Polygon> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<db::Polygon> > others;
  //  NOTE: with counting the other region needs to be merged
  others.push_back (counting ? other.begin_merged () : other.begin ());

  proc.run_flat (polygons, others, std::vector<bool> (), &op, oph.results ());

  return oph.region_pair ();
}

EdgesDelegate *
AsIfFlatRegion::pull_generic (const Edges &other) const
{
  if (other.empty ()) {
    return other.delegate ()->clone ();
  } else if (empty ()) {
    return new EmptyEdges ();
  }

  db::RegionIterator polygons (begin ());

  db::pull_with_edge_local_operation <db::Polygon, db::Edge, db::Edge> op;

  db::local_processor<db::Polygon, db::Edge, db::Edge> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<db::Edge> > others;
  others.push_back (other.begin_merged ());

  std::unique_ptr<FlatEdges> output (new FlatEdges (other.merged_semantics () || other.is_merged ()));
  std::vector<db::Shapes *> results;
  results.push_back (&output->raw_edges ());

  proc.run_flat (polygons, others, std::vector<bool> (), &op, results);

  return output.release ();
}

TextsDelegate *
AsIfFlatRegion::pull_generic (const Texts &other) const
{
  db::RegionIterator polygons (begin ());

  db::pull_with_text_local_operation <db::Polygon, db::Text, db::Text> op;

  db::local_processor<db::Polygon, db::Text, db::Text> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<db::Text> > others;
  others.push_back (other.begin ());

  std::unique_ptr<FlatTexts> output (new FlatTexts ());
  std::vector<db::Shapes *> results;
  results.push_back (&output->raw_texts ());

  proc.run_flat (polygons, others, std::vector<bool> (), &op, results);

  return output.release ();
}

RegionDelegate *
AsIfFlatRegion::pull_generic (const Region &other, int mode, bool touching) const
{
  db::RegionIterator polygons (begin ());

  db::pull_local_operation <db::Polygon, db::Polygon, db::Polygon> op (mode, touching);

  db::local_processor<db::Polygon, db::Polygon, db::Polygon> proc;
  proc.set_base_verbosity (base_verbosity ());
  proc.set_description (progress_desc ());
  proc.set_report_progress (report_progress ());

  std::vector<generic_shape_iterator<db::Polygon> > others;
  others.push_back (other.begin_merged ());

  std::unique_ptr<FlatRegion> output (new FlatRegion (other.merged_semantics () || other.is_merged ()));
  std::vector<db::Shapes *> results;
  results.push_back (&output->raw_polygons ());

  proc.run_flat (polygons, others, std::vector<bool> (), &op, results);

  return output.release ();
}

template <class Trans>
void
AsIfFlatRegion::produce_markers_for_grid_check (const db::Polygon &poly, const Trans &tr, db::Coord gx, db::Coord gy, db::Shapes &shapes)
{
  Trans tr_inv = tr.inverted ();

  gx = std::max (db::Coord (1), gx);
  gy = std::max (db::Coord (1), gy);

  for (size_t i = 0; i < poly.holes () + 1; ++i) {

    db::Polygon::polygon_contour_iterator b, e;

    if (i == 0) {
      b = poly.begin_hull ();
      e = poly.end_hull ();
    } else {
      b = poly.begin_hole ((unsigned int) (i - 1));
      e = poly.end_hole ((unsigned int)  (i - 1));
    }

    for (db::Polygon::polygon_contour_iterator pt = b; pt != e; ++pt) {
      db::Point p = tr * *pt;
      if ((p.x () % gx) != 0 || (p.y () % gy) != 0) {
        shapes.insert (EdgePair (db::Edge (p, p), db::Edge (p, p)).transformed (tr_inv));
      }
    }

  }
}

template void AsIfFlatRegion::produce_markers_for_grid_check<db::ICplxTrans> (const db::Polygon &poly, const db::ICplxTrans &tr, db::Coord gx, db::Coord gy, db::Shapes &shapes);
template void AsIfFlatRegion::produce_markers_for_grid_check<db::UnitTrans> (const db::Polygon &poly, const db::UnitTrans &tr, db::Coord gx, db::Coord gy, db::Shapes &shapes);

EdgePairsDelegate *
AsIfFlatRegion::grid_check (db::Coord gx, db::Coord gy) const
{
  if (gx < 0 || gy < 0) {
    throw tl::Exception (tl::to_string (tr ("Grid check requires a positive grid value")));
  }

  if (gx == 0 && gy == 0) {
    return new EmptyEdgePairs ();
  }

  std::unique_ptr<db::FlatEdgePairs> res (new db::FlatEdgePairs ());

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    produce_markers_for_grid_check (*p, db::UnitTrans (), gx, gy, res->raw_edge_pairs ());
  }

  return res.release ();
}

static bool ac_less (double cos_a, bool gt180_a, double cos_b, bool gt180_b)
{
  if (gt180_a != gt180_b) {
    return gt180_a < gt180_b;
  } else {
    if (gt180_a) {
      return cos_a < cos_b - 1e-10;
    } else {
      return cos_a > cos_b + 1e-10;
    }
  }
}

template <class Trans>
void
AsIfFlatRegion::produce_markers_for_angle_check (const db::Polygon &poly, const Trans &tr, double min, double max, bool inverse, db::Shapes &shapes)
{
  double cos_min = cos (std::max (0.0, std::min (360.0, min)) / 180.0 * M_PI);
  double cos_max = cos (std::max (0.0, std::min (360.0, max)) / 180.0 * M_PI);
  bool gt180_min = min > 180.0;
  bool gt180_max = max > 180.0;

  for (size_t i = 0; i < poly.holes () + 1; ++i) {

    const db::Polygon::contour_type *h = 0;
    if (i == 0) {
      h = &poly.hull ();
    } else {
      h = &poly.hole ((unsigned int) (i - 1));
    }

    size_t np = h->size ();

    for (size_t j = 0; j < np; ++j) {

      db::Edge e ((*h) [j], (*h) [(j + 1) % np]);
      e.transform (tr);
      db::Edge ee (e.p2 (), (*h) [(j + 2) % np]);
      ee.transform (tr);

      double le = e.double_length ();
      double lee = ee.double_length ();

      double cos_a = -db::sprod (e, ee) / (le * lee);
      bool gt180_a = db::vprod_sign (e, ee) > 0;

      if ((ac_less (cos_a, gt180_a, cos_max, gt180_max) && !ac_less (cos_a, gt180_a, cos_min, gt180_min)) == !inverse) {
        shapes.insert (EdgePair (e, ee));
      }

    }

  }
}

template void AsIfFlatRegion::produce_markers_for_angle_check<db::ICplxTrans> (const db::Polygon &poly, const db::ICplxTrans &tr, double min, double max, bool inverse, db::Shapes &shapes);
template void AsIfFlatRegion::produce_markers_for_angle_check<db::UnitTrans> (const db::Polygon &poly, const db::UnitTrans &tr, double min, double max, bool inverse, db::Shapes &shapes);

EdgePairsDelegate *
AsIfFlatRegion::angle_check (double min, double max, bool inverse) const
{
  std::unique_ptr<db::FlatEdgePairs> res (new db::FlatEdgePairs ());

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    produce_markers_for_angle_check (*p, db::UnitTrans (), min, max, inverse, res->raw_edge_pairs ());
  }

  return res.release ();
}

RegionDelegate *
AsIfFlatRegion::snapped (db::Coord gx, db::Coord gy)
{
  if (gx < 0 || gy < 0) {
    throw tl::Exception (tl::to_string (tr ("Grid snap requires a positive grid value")));
  }

  std::unique_ptr<FlatRegion> new_region (new FlatRegion ());

  gx = std::max (db::Coord (1), gx);
  gy = std::max (db::Coord (1), gy);

  std::vector<db::Point> heap;

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    new_region->raw_polygons ().insert (snapped_polygon (*p, gx, gy, heap));
  }

  return new_region.release ();
}

RegionDelegate *
AsIfFlatRegion::scaled_and_snapped (db::Coord gx, db::Coord mx, db::Coord dx, db::Coord gy, db::Coord my, db::Coord dy)
{
  if (gx < 0 || gy < 0) {
    throw tl::Exception (tl::to_string (tr ("Grid snap requires a positive grid value")));
  }

  if (mx <= 0 || dx <= 0 || my <= 0 || dy <= 0) {
    throw tl::Exception (tl::to_string (tr ("Scale and snap requires positive and non-null magnification or divisor values")));
  }

  std::unique_ptr<FlatRegion> new_region (new FlatRegion (merged_semantics ()));

  gx = std::max (db::Coord (1), gx);
  gy = std::max (db::Coord (1), gy);

  std::vector<db::Point> heap;

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
    new_region->raw_polygons ().insert (scaled_and_snapped_polygon (*p, gx, mx, dx, 0, gy, my, dy, 0, heap));
  }

  return new_region.release ();
}

template <class TR>
static
void region_cop_impl (AsIfFlatRegion *region, db::Shapes *output_to, db::CompoundRegionOperationNode &node)
{
  db::local_processor<db::Polygon, db::Polygon, TR> proc;
  proc.set_base_verbosity (region->base_verbosity ());
  proc.set_description (region->progress_desc ());
  proc.set_report_progress (region->report_progress ());

  db::RegionIterator polygons (region->begin_merged ());

  std::vector<generic_shape_iterator<db::Polygon> > others;
  std::vector<bool> foreign;
  std::vector<db::Region *> inputs = node.inputs ();
  for (std::vector<db::Region *>::const_iterator i = inputs.begin (); i != inputs.end (); ++i) {
    if (*i == subject_regionptr () || *i == foreign_regionptr ()) {
      others.push_back (region->begin_merged ());
      foreign.push_back (*i == foreign_regionptr ());
    } else {
      others.push_back ((*i)->begin ());
      foreign.push_back (false);
    }
  }

  std::vector<db::Shapes *> results;
  results.push_back (output_to);

  compound_local_operation<db::Polygon, db::Polygon, TR> op (&node);
  proc.run_flat (polygons, others, foreign, &op, results);
}

template <class TR>
static
void region_cop_with_properties_impl (AsIfFlatRegion *region, db::Shapes *output_to, db::PropertiesRepository *target_pr, db::CompoundRegionOperationNode &node, db::PropertyConstraint prop_constraint)
{
  db::local_processor<db::PolygonWithProperties, db::PolygonWithProperties, db::object_with_properties<TR> > proc;
  proc.set_base_verbosity (region->base_verbosity ());
  proc.set_description (region->progress_desc ());
  proc.set_report_progress (region->report_progress ());

  db::generic_shape_iterator<db::PolygonWithProperties> polygons (db::make_wp_iter (region->begin_merged ()));

  std::vector<const db::PropertiesRepository *> intruder_prs;
  const db::PropertiesRepository *subject_pr = region->properties_repository ();

  std::vector<generic_shape_iterator<db::PolygonWithProperties> > others;
  std::vector<bool> foreign;
  std::vector<db::Region *> inputs = node.inputs ();
  for (std::vector<db::Region *>::const_iterator i = inputs.begin (); i != inputs.end (); ++i) {
    if (*i == subject_regionptr () || *i == foreign_regionptr ()) {
      others.push_back (db::make_wp_iter (region->begin_merged ()));
      foreign.push_back (*i == foreign_regionptr ());
      intruder_prs.push_back (subject_pr);
    } else {
      others.push_back (db::make_wp_iter ((*i)->begin ()));
      foreign.push_back (false);
      intruder_prs.push_back (&(*i)->properties_repository ());
    }
  }

  std::vector<db::Shapes *> results;
  results.push_back (output_to);

  compound_local_operation_with_properties<db::Polygon, db::Polygon, TR> op (&node, prop_constraint, target_pr, subject_pr, intruder_prs);
  proc.run_flat (polygons, others, foreign, &op, results);
}

EdgePairsDelegate *
AsIfFlatRegion::cop_to_edge_pairs (db::CompoundRegionOperationNode &node, db::PropertyConstraint prop_constraint)
{
  std::unique_ptr<FlatEdgePairs> output (new FlatEdgePairs ());
  if (pc_skip (prop_constraint)) {
    region_cop_impl<db::EdgePair> (this, &output->raw_edge_pairs (), node);
  } else {
    region_cop_with_properties_impl<db::EdgePair> (this, &output->raw_edge_pairs (), output->properties_repository (), node, prop_constraint);
  }
  return output.release ();
}

RegionDelegate *
AsIfFlatRegion::cop_to_region (db::CompoundRegionOperationNode &node, db::PropertyConstraint prop_constraint)
{
  std::unique_ptr<FlatRegion> output (new FlatRegion ());
  if (pc_skip (prop_constraint)) {
    region_cop_impl<db::Polygon> (this, &output->raw_polygons (), node);
  } else {
    region_cop_with_properties_impl<db::Polygon> (this, &output->raw_polygons (), output->properties_repository (), node, prop_constraint);
  }
  return output.release ();
}

EdgesDelegate *
AsIfFlatRegion::cop_to_edges (db::CompoundRegionOperationNode &node, PropertyConstraint prop_constraint)
{
  std::unique_ptr<FlatEdges> output (new FlatEdges ());
  if (pc_skip (prop_constraint)) {
    region_cop_impl<db::Edge> (this, &output->raw_edges (), node);
  } else {
    region_cop_with_properties_impl<db::Edge> (this, &output->raw_edges (), output->properties_repository (), node, prop_constraint);
  }
  return output.release ();
}

EdgePairsDelegate *
AsIfFlatRegion::width_check (db::Coord d, const RegionCheckOptions &options) const
{
  return run_single_polygon_check (db::WidthRelation, d, options);
}

EdgePairsDelegate *
AsIfFlatRegion::space_or_isolated_check (db::Coord d, const RegionCheckOptions &options, bool isolated) const
{
  if (options.opposite_filter != NoOppositeFilter || options.rect_filter != NoRectFilter || options.shielded) {
    //  NOTE: we have to use the "foreign" scheme with a filter because only this scheme
    //  guarantees that all subject shapes are visited.
    return run_check (db::SpaceRelation, isolated, foreign_regionptr (), d, options);
  } else {
    return run_check (db::SpaceRelation, isolated, subject_regionptr (), d, options);
  }
}

EdgePairsDelegate *
AsIfFlatRegion::space_check (db::Coord d, const RegionCheckOptions &options) const
{
  return space_or_isolated_check (d, options, false);
}

EdgePairsDelegate *
AsIfFlatRegion::isolated_check (db::Coord d, const RegionCheckOptions &options) const
{
  return space_or_isolated_check (d, options, true);
}

EdgePairsDelegate *
AsIfFlatRegion::notch_check (db::Coord d, const RegionCheckOptions &options) const
{
  return run_single_polygon_check (db::SpaceRelation, d, options);
}

EdgePairsDelegate *
AsIfFlatRegion::enclosing_check (const Region &other, db::Coord d, const RegionCheckOptions &options) const
{
  return run_check (db::OverlapRelation, true, &other, d, options);
}

EdgePairsDelegate *
AsIfFlatRegion::overlap_check (const Region &other, db::Coord d, const RegionCheckOptions &options) const
{
  return run_check (db::WidthRelation, true, &other, d, options);
}

EdgePairsDelegate *
AsIfFlatRegion::separation_check (const Region &other, db::Coord d, const RegionCheckOptions &options) const
{
  return run_check (db::SpaceRelation, true, &other, d, options);
}

EdgePairsDelegate *
AsIfFlatRegion::inside_check (const Region &other, db::Coord d, const RegionCheckOptions &options) const
{
  return run_check (db::InsideRelation, true, &other, d, options);
}

EdgePairsDelegate *
AsIfFlatRegion::run_check (db::edge_relation_type rel, bool different_polygons, const Region *other, db::Coord d, const RegionCheckOptions &options) const
{
  //  force different polygons in the different properties case to skip intra-polygon checks
  if (pc_always_different (options.prop_constraint)) {
    different_polygons = true;
  }

  bool needs_merged_primary = different_polygons || options.needs_merged ();

  db::RegionIterator polygons (needs_merged_primary ? begin_merged () : begin ());
  bool primary_is_merged = ! merged_semantics () || needs_merged_primary || is_merged ();

  EdgeRelationFilter check (rel, d, options.metrics);
  check.set_include_zero (false);
  check.set_whole_edges (options.whole_edges);
  check.set_ignore_angle (options.ignore_angle);
  check.set_min_projection (options.min_projection);
  check.set_max_projection (options.max_projection);

  std::vector<db::RegionIterator> others;
  std::vector<bool> foreign;
  bool has_other = false;
  bool other_is_merged = true;

  if (other == subject_regionptr () || other == foreign_regionptr ()) {
    foreign.push_back (other == foreign_regionptr ());
    others.push_back (polygons);
    other_is_merged = primary_is_merged;
  } else {
    foreign.push_back (false);
    if (! other->merged_semantics ()) {
      others.push_back (other->begin ());
      other_is_merged = true;
    } else if (options.whole_edges) {
      //  NOTE: whole edges needs both inputs merged
      others.push_back (other->begin_merged ());
      other_is_merged = true;
    } else {
      others.push_back (other->begin ());
      other_is_merged = other->is_merged ();
    }
    has_other = true;
  }

  std::unique_ptr<FlatEdgePairs> output (new FlatEdgePairs ());

  std::vector<db::Shapes *> results;
  results.push_back (&output->raw_edge_pairs ());

  if (pc_skip (options.prop_constraint)) {

    db::check_local_operation<db::Polygon, db::Polygon> op (check, different_polygons, primary_is_merged, has_other, other_is_merged, options);

    db::local_processor<db::Polygon, db::Polygon, db::EdgePair> proc;
    proc.set_base_verbosity (base_verbosity ());
    proc.set_description (progress_desc ());
    proc.set_report_progress (report_progress ());

    proc.run_flat (polygons, others, foreign, &op, results);

  } else {

    const db::PropertiesRepository *subject_pr = properties_repository ();
    const db::PropertiesRepository *intruder_pr = (other == subject_regionptr () || other == foreign_regionptr ()) ? subject_pr : &other->properties_repository ();

    db::check_local_operation_with_properties<db::Polygon, db::Polygon> op (check, different_polygons, primary_is_merged, has_other, other_is_merged, options, output->properties_repository (), subject_pr, intruder_pr);

    db::local_processor<db::PolygonWithProperties, db::PolygonWithProperties, db::EdgePairWithProperties> proc;
    proc.set_base_verbosity (base_verbosity ());
    proc.set_description (progress_desc ());
    proc.set_report_progress (report_progress ());

    std::vector<db::generic_shape_iterator<db::PolygonWithProperties> > others_wp;
    for (auto o = others.begin (); o != others.end (); ++o) {
      others_wp.push_back (db::make_wp_iter (std::move (*o)));
    }

    proc.run_flat (db::make_wp_iter (std::move (polygons)), others_wp, foreign, &op, results);

  }

  return output.release ();
}

EdgePairsDelegate *
AsIfFlatRegion::run_single_polygon_check (db::edge_relation_type rel, db::Coord d, const RegionCheckOptions &options) const
{
  std::unique_ptr<FlatEdgePairs> result (new FlatEdgePairs ());
  db::PropertyMapper pm (result->properties_repository (), properties_repository ());

  EdgeRelationFilter check (rel, d, options.metrics);
  check.set_include_zero (false);
  check.set_whole_edges (options.whole_edges);
  check.set_ignore_angle (options.ignore_angle);
  check.set_min_projection (options.min_projection);
  check.set_max_projection (options.max_projection);

  for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {

    edge2edge_check_negative_or_positive<db::Shapes> edge_check (check, result->raw_edge_pairs (), options.negative, false /*=same polygons*/, false /*=same layers*/, options.shielded, true /*symmetric edge pairs*/, pc_remove (options.prop_constraint) ? 0 : pm (p.prop_id ()));
    poly2poly_check<db::Polygon> poly_check (edge_check);

    do {
      poly_check.single (*p, 0);
    } while (edge_check.prepare_next_pass ());

  }

  return result.release ();
}

RegionDelegate *
AsIfFlatRegion::merged (bool min_coherence, unsigned int min_wc) const
{
  if (empty ()) {

    return new EmptyRegion ();

  } else if (is_box ()) {

    //  take box only if min_wc == 0, otherwise clear
    if (min_wc > 0) {
      return new EmptyRegion ();
    } else {
      return clone ();
    }

  } else {

    std::unique_ptr<FlatRegion> new_region (new FlatRegion (true));
    merge_polygons_to (new_region->raw_polygons (), min_coherence, min_wc, new_region->properties_repository ());

    return new_region.release ();

  }
}

RegionDelegate *
AsIfFlatRegion::sized (coord_type d, unsigned int mode) const
{
  return sized (d, d, mode);
}

RegionDelegate *
AsIfFlatRegion::sized (coord_type dx, coord_type dy, unsigned int mode) const
{
  if (empty ()) {

    //  ignore empty
    return new EmptyRegion ();

  } else if (is_box () && mode >= 2) {

    //  simplified handling for a box
    db::Box b = bbox ().enlarged (db::Vector (dx, dy));
    return region_from_box (b, properties_repository (), begin ()->prop_id ());

  } else if (! merged_semantics () || is_merged ()) {

    //  Generic case
    std::unique_ptr<FlatRegion> new_region (new FlatRegion ());
    db::PropertyMapper pm (new_region->properties_repository (), properties_repository ());

    db::ShapeGenerator pc (new_region->raw_polygons (), false);
    db::PolygonGenerator pg (pc, false, true);
    db::SizingPolygonFilter sf (pg, dx, dy, mode);
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      pc.set_prop_id (pm (p.prop_id ()));
      sf.put (*p);
    }

    //  in case of negative sizing the output polygons will still be merged (on positive sizing they might
    //  overlap after size and are not necessarily merged)
    if (dx < 0 && dy < 0 && is_merged ()) {
      new_region->set_is_merged (true);
    }

    return new_region.release ();

  } else {

    std::unique_ptr<FlatRegion> new_region (new FlatRegion ());

//  old implementation without property support
#if 0
    //  Generic case - the size operation will merge first
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

    db::ShapeGenerator pc (new_region->raw_polygons (), true /*clear*/);
    db::PolygonGenerator pg2 (pc, false /*don't resolve holes*/, true /*min. coherence*/);
    db::SizingPolygonFilter siz (pg2, dx, dy, mode);
    db::PolygonGenerator pg (siz, false /*don't resolve holes*/, min_coherence () /*min. coherence*/);
    db::BooleanOp op (db::BooleanOp::Or);
    ep.process (pg, op);
#else

    //  Generic case
    db::PropertyMapper pm (new_region->properties_repository (), properties_repository ());

    db::ShapeGenerator pc (new_region->raw_polygons (), false);
    db::PolygonGenerator pg (pc, false, true);
    db::SizingPolygonFilter sf (pg, dx, dy, mode);
    for (RegionIterator p (begin_merged ()); ! p.at_end (); ++p) {
      pc.set_prop_id (pm (p.prop_id ()));
      sf.put (*p);
    }

#endif

    //  in case of negative sizing the output polygons will still be merged (on positive sizing they might
    //  overlap after size and are not necessarily merged)
    if (dx < 0 && dy < 0 && merged_semantics ()) {
      new_region->set_is_merged (true);
    }

    return new_region.release ();

  }
}

RegionDelegate *
AsIfFlatRegion::and_with (const Region &other, PropertyConstraint property_constraint) const
{
  if (empty () || other.empty ()) {

    //  Nothing to do
    return new EmptyRegion ();

  } else if (is_box () && other.is_box ()) {

    if (pc_skip (property_constraint) || pc_match (property_constraint, begin ()->prop_id (), other.begin ().prop_id ())) {

      //  Simplified handling for boxes
      db::Box b = bbox ();
      b &= other.bbox ();

      db::properties_id_type prop_id_out = pc_norm (property_constraint, begin ()->prop_id ());

      return region_from_box (b, properties_repository (), prop_id_out);

    } else {
      return new EmptyRegion ();
    }

  } else if (is_box () && ! other.strict_handling ()) {

    db::properties_id_type self_prop_id = pc_skip (property_constraint) ? 0 : begin ()->prop_id ();

    //  map AND with box to clip ..
    db::Box b = bbox ();
    std::unique_ptr<FlatRegion> new_region (new FlatRegion (false));
    db::PropertyMapper pm (new_region->properties_repository (), properties_repository ());

    db::properties_id_type prop_id_out = pm (pc_norm (property_constraint, self_prop_id));

    std::vector<db::Polygon> clipped;
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {

      db::properties_id_type prop_id = p.prop_id ();
      if (pc_match (property_constraint, self_prop_id, prop_id)) {

        clipped.clear ();
        clip_poly (*p, b, clipped);

        if (prop_id_out == 0) {
          new_region->raw_polygons ().insert (clipped.begin (), clipped.end ());
        } else {
          for (auto i = clipped.begin (); i != clipped.end (); ++i) {
            new_region->raw_polygons ().insert (db::PolygonWithProperties (*i, prop_id_out));
          }
        }

      }

    }

    return new_region.release ();

  } else if (other.is_box () && ! strict_handling ()) {

    db::properties_id_type other_prop_id = pc_skip (property_constraint) ? 0 : other.begin ().prop_id ();

    //  map AND with box to clip ..
    db::Box b = other.bbox ();
    std::unique_ptr<FlatRegion> new_region (new FlatRegion (false));
    db::PropertyMapper pm (new_region->properties_repository (), properties_repository ());

    std::vector<db::Polygon> clipped;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {

      db::properties_id_type prop_id = p.prop_id ();
      if (pc_match (property_constraint, prop_id, other_prop_id)) {

        clipped.clear ();
        clip_poly (*p, b, clipped);

        db::properties_id_type prop_id_out = pm (pc_norm (property_constraint, prop_id));
        if (prop_id_out == 0) {
          new_region->raw_polygons ().insert (clipped.begin (), clipped.end ());
        } else {
          for (auto i = clipped.begin (); i != clipped.end (); ++i) {
            new_region->raw_polygons ().insert (db::PolygonWithProperties (*i, prop_id_out));
          }
        }

      }

    }

    return new_region.release ();

  } else if (! bbox ().overlaps (other.bbox ())) {

    //  Result will be nothing
    return new EmptyRegion ();

  } else {
    return and_or_not_with (true, other, property_constraint);
  }
}

RegionDelegate *
AsIfFlatRegion::not_with (const Region &other, PropertyConstraint property_constraint) const
{
  if (empty ()) {

    //  Nothing to do
    return new EmptyRegion ();

  } else if (other.empty () && ! strict_handling ()) {

    //  Nothing to do
    return clone ()->remove_properties (pc_remove (property_constraint));

  } else if (! bbox ().overlaps (other.bbox ()) && ! strict_handling ()) {

    //  Nothing to do
    return clone ()->remove_properties (pc_remove (property_constraint));

  } else {
    return and_or_not_with (false, other, property_constraint);
  }
}

RegionDelegate *
AsIfFlatRegion::and_or_not_with (bool is_and, const Region &other, PropertyConstraint property_constraint) const
{
  if (pc_skip (property_constraint)) {

    //  Generic case
    db::EdgeProcessor ep (report_progress (), progress_desc ());
    ep.set_base_verbosity (base_verbosity ());

    //  count edges and reserve memory
    size_t n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }
    n = 1;
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }

    std::unique_ptr<FlatRegion> new_region (new FlatRegion (true));
    db::BooleanOp op (is_and ? db::BooleanOp::And : db::BooleanOp::ANotB);
    db::ShapeGenerator pc (new_region->raw_polygons (), true /*clear*/);
    db::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence ());
    ep.process (pg, op);

    return new_region.release ();

  } else {

    db::generic_shape_iterator<db::PolygonWithProperties> polygons (db::make_wp_iter (begin ()));

    std::unique_ptr<FlatRegion> output (new FlatRegion ());
    std::vector<db::Shapes *> results;
    results.push_back (&output->raw_polygons ());

    db::bool_and_or_not_local_operation_with_properties<db::Polygon, db::Polygon, db::Polygon> op (is_and, output->properties_repository (), properties_repository (), &other.properties_repository (), property_constraint);

    db::local_processor<db::PolygonWithProperties, db::PolygonWithProperties, db::PolygonWithProperties> proc;
    proc.set_base_verbosity (base_verbosity ());
    proc.set_description (progress_desc ());
    proc.set_report_progress (report_progress ());

    std::vector<db::generic_shape_iterator<db::PolygonWithProperties> > others;
    others.push_back (db::make_wp_iter (other.begin ()));

    proc.run_flat (polygons, others, std::vector<bool> (), &op, results);

    return output.release ();

  }
}

std::pair<RegionDelegate *, RegionDelegate *>
AsIfFlatRegion::andnot_with (const Region &other, PropertyConstraint property_constraint) const
{
  if (empty ()) {

    //  Nothing to do
    return std::make_pair (new EmptyRegion (), new EmptyRegion ());

  } else if (other.empty () && ! strict_handling ()) {

    //  Nothing to do
    return std::make_pair (new EmptyRegion (), clone ()->remove_properties (pc_remove (property_constraint)));

  } else if (! bbox ().overlaps (other.bbox ()) && ! strict_handling ()) {

    //  Nothing to do
    return std::make_pair (new EmptyRegion (), clone ()->remove_properties (pc_remove (property_constraint)));

  } else if (pc_skip (property_constraint)) {

    //  Generic case
    db::EdgeProcessor ep (report_progress (), progress_desc ());
    ep.set_base_verbosity (base_verbosity ());

    //  count edges and reserve memory
    size_t n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }
    n = 1;
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }

    std::unique_ptr<FlatRegion> new_region1 (new FlatRegion (true));
    db::BooleanOp op1 (db::BooleanOp::And);
    db::ShapeGenerator pc1 (new_region1->raw_polygons (), true /*clear*/);
    db::PolygonGenerator pg1 (pc1, false /*don't resolve holes*/, min_coherence ());

    std::unique_ptr<FlatRegion> new_region2 (new FlatRegion (true));
    db::BooleanOp op2 (db::BooleanOp::ANotB);
    db::ShapeGenerator pc2 (new_region2->raw_polygons (), true /*clear*/);
    db::PolygonGenerator pg2 (pc2, false /*don't resolve holes*/, min_coherence ());

    std::vector<std::pair<db::EdgeSink *, db::EdgeEvaluatorBase *> > procs;
    procs.push_back (std::make_pair (&pg1, &op1));
    procs.push_back (std::make_pair (&pg2, &op2));
    ep.process (procs);

    return std::make_pair (new_region1.release (), new_region2.release ());

  } else {

    db::generic_shape_iterator<db::PolygonWithProperties> polygons (db::make_wp_iter (begin ()));

    std::unique_ptr<FlatRegion> output1 (new FlatRegion ());
    std::unique_ptr<FlatRegion> output2 (new FlatRegion ());
    std::vector<db::Shapes *> results;
    results.push_back (&output1->raw_polygons ());
    results.push_back (&output2->raw_polygons ());

    db::two_bool_and_not_local_operation_with_properties<db::Polygon, db::Polygon, db::Polygon> op (output1->properties_repository (), output2->properties_repository (), properties_repository (), &other.properties_repository (), property_constraint);

    db::local_processor<db::PolygonWithProperties, db::PolygonWithProperties, db::PolygonWithProperties> proc;
    proc.set_base_verbosity (base_verbosity ());
    proc.set_description (progress_desc ());
    proc.set_report_progress (report_progress ());

    std::vector<db::generic_shape_iterator<db::PolygonWithProperties> > others;
    others.push_back (db::make_wp_iter (other.begin ()));

    proc.run_flat (polygons, others, std::vector<bool> (), &op, results);

    return std::make_pair (output1.release (), output2.release ());

  }
}

RegionDelegate *
AsIfFlatRegion::xor_with (const Region &other, PropertyConstraint prop_constraint) const
{
  if (empty () && ! other.strict_handling ()) {

    return other.delegate ()->clone ();

  } else if (other.empty () && ! strict_handling ()) {

    return clone ();

  } else if (! bbox ().overlaps (other.bbox ()) && ! strict_handling () && ! other.strict_handling ()) {

    //  Simplified handling for disjunct case
    return or_with (other, prop_constraint);

  } else {

    //  TODO: implement property constraint

    //  Generic case
    db::EdgeProcessor ep (report_progress (), progress_desc ());
    ep.set_base_verbosity (base_verbosity ());

    //  count edges and reserve memory
    size_t n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }
    n = 1;
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }

    std::unique_ptr<FlatRegion> new_region (new FlatRegion (true));
    db::BooleanOp op (db::BooleanOp::Xor);
    db::ShapeGenerator pc (new_region->raw_polygons (), true /*clear*/);
    db::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence ());
    ep.process (pg, op);

    return new_region.release ();

  }
}

RegionDelegate *
AsIfFlatRegion::or_with (const Region &other, PropertyConstraint /*prop_constraint*/) const
{
  if (empty () && ! other.strict_handling ()) {

    return other.delegate ()->clone ();

  } else if (other.empty () && ! strict_handling ()) {

    //  Nothing to do
    return clone ();

  } else if (! bbox ().overlaps (other.bbox ()) && ! strict_handling () && ! other.strict_handling ()) {

    //  Simplified handling for disjunct case
    return add (other);

  } else {

    //  TODO: implement property constraint

    //  Generic case
    db::EdgeProcessor ep (report_progress (), progress_desc ());
    ep.set_base_verbosity (base_verbosity ());

    //  count edges and reserve memory
    size_t n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      n += p->vertices ();
    }
    ep.reserve (n);

    //  insert the polygons into the processor
    n = 0;
    for (RegionIterator p (begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }
    n = 1;
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p, n += 2) {
      ep.insert (*p, n);
    }

    std::unique_ptr<FlatRegion> new_region (new FlatRegion (true));
    db::BooleanOp op (db::BooleanOp::Or);
    db::ShapeGenerator pc (new_region->raw_polygons (), true /*clear*/);
    db::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence ());
    ep.process (pg, op);

    return new_region.release ();

  }
}

RegionDelegate *
AsIfFlatRegion::add (const Region &other) const
{
  const FlatRegion *other_flat = dynamic_cast<const FlatRegion *> (other.delegate ());
  if (other_flat) {

    std::unique_ptr<FlatRegion> new_region (new FlatRegion (*other_flat));
    new_region->set_is_merged (false);
    new_region->invalidate_cache ();

    size_t n = new_region->raw_polygons ().size () + count ();

    new_region->reserve (n);

    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      new_region->raw_polygons ().insert (*p);
    }

    return new_region.release ();

  } else {

    std::unique_ptr<FlatRegion> new_region (new FlatRegion (false /*not merged*/));

    size_t n = count () + other.count ();

    new_region->reserve (n);

    for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
      new_region->raw_polygons ().insert (*p);
    }
    for (RegionIterator p (other.begin ()); ! p.at_end (); ++p) {
      new_region->raw_polygons ().insert (*p);
    }

    return new_region.release ();

  }
}

static void
deliver_shapes_of_nets_recursive (db::Shapes &out, db::PropertiesRepository *pr, const db::Circuit *circuit, const LayoutToNetlist *l2n, const db::Region *of_layer, NetPropertyMode prop_mode, const tl::Variant &net_prop_name, const db::ICplxTrans &tr, const std::set<const db::Net *> *net_filter)
{
  db::CplxTrans dbu_trans (l2n->internal_layout ()->dbu ());
  auto dbu_trans_inv = dbu_trans.inverted ();

  for (auto n = circuit->begin_nets (); n != circuit->end_nets (); ++n) {

    if (! net_filter || net_filter->find (n.operator-> ()) != net_filter->end ()) {
      db::properties_id_type prop_id = db::NetBuilder::make_netname_propid (*pr, prop_mode, net_prop_name, *n);
      l2n->shapes_of_net (*n, *of_layer, true, out, prop_id, tr);
    }

    //  dive into subcircuits
    for (auto sc = circuit->begin_subcircuits (); sc != circuit->end_subcircuits (); ++sc) {
      const db::Circuit *circuit_ref = sc->circuit_ref ();
      db::ICplxTrans tr_ref = tr * (dbu_trans_inv * sc->trans () * dbu_trans);
      deliver_shapes_of_nets_recursive (out, pr, circuit_ref, l2n, of_layer, prop_mode, net_prop_name, tr_ref, net_filter);
    }

  }
}

RegionDelegate *
AsIfFlatRegion::nets (LayoutToNetlist *l2n, NetPropertyMode prop_mode, const tl::Variant &net_prop_name, const std::vector<const db::Net *> *net_filter) const
{
  if (! l2n->is_netlist_extracted ()) {
    throw tl::Exception (tl::to_string (tr ("The netlist has not been extracted yet")));
  }

  std::unique_ptr<db::FlatRegion> result (new db::FlatRegion ());
  std::unique_ptr<db::Region> region_for_layer (l2n->layer_by_original (this));

  if (! region_for_layer) {
    throw tl::Exception (tl::to_string (tr ("The given layer is not an original layer used in netlist extraction")));
  }

  if (l2n->netlist ()->top_circuit_count () == 0) {
    throw tl::Exception (tl::to_string (tr ("No top circuit found in netlist")));
  } else if (l2n->netlist ()->top_circuit_count () > 1) {
    throw tl::Exception (tl::to_string (tr ("More than one top circuit found in netlist")));
  }
  const db::Circuit *top_circuit = l2n->netlist ()->begin_top_down ().operator-> ();

  std::set<const db::Net *> net_filter_set;
  if (net_filter) {
    net_filter_set.insert (net_filter->begin (), net_filter->end ());
  }

  deliver_shapes_of_nets_recursive (result->raw_polygons (), result->properties_repository (), top_circuit, l2n, region_for_layer.get (), prop_mode, net_prop_name, db::ICplxTrans (), net_filter ? &net_filter_set : 0);

  return result.release ();
}

void
AsIfFlatRegion::insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  //  improves performance when inserting an original layout into the same layout
  db::LayoutLocker locker (layout);
  db::PropertyMapper pm (&layout->properties_repository (), properties_repository ());

  db::Shapes &shapes = layout->cell (into_cell).shapes (into_layer);
  for (RegionIterator p (begin ()); ! p.at_end (); ++p) {
    db::properties_id_type prop_id = p.prop_id ();
    if (prop_id != 0) {
      shapes.insert (db::PolygonWithProperties (*p, pm (prop_id)));
    } else {
      shapes.insert (*p);
    }
  }
}

bool
AsIfFlatRegion::equals (const Region &other) const
{
  if (empty () != other.empty ()) {
    return false;
  }
  if (count () != other.count ()) {
    return false;
  }
  RegionIterator o1 (begin ());
  RegionIterator o2 (other.begin ());
  while (! o1.at_end () && ! o2.at_end ()) {
    if (*o1 != *o2) {
      return false;
    }
    ++o1;
    ++o2;
  }
  return true;
}

bool
AsIfFlatRegion::less (const Region &other) const
{
  if (empty () != other.empty ()) {
    return empty () < other.empty ();
  }
  if (count () != other.count ()) {
    return (count () < other.count ());
  }
  RegionIterator o1 (begin ());
  RegionIterator o2 (other.begin ());
  while (! o1.at_end () && ! o2.at_end ()) {
    if (*o1 != *o2) {
      return *o1 < *o2;
    }
    ++o1;
    ++o2;
  }
  return false;
}

}

