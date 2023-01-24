
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


#include "dbAsIfFlatTexts.h"
#include "dbFlatTexts.h"
#include "dbFlatRegion.h"
#include "dbFlatEdges.h"
#include "dbEmptyTexts.h"
#include "dbEmptyRegion.h"
#include "dbTexts.h"
#include "dbBoxConvert.h"
#include "dbRegion.h"
#include "dbTextsUtils.h"

#include <sstream>

namespace db
{

// -------------------------------------------------------------------------------------------------------------
//  AsIfFlagTexts implementation

AsIfFlatTexts::AsIfFlatTexts ()
  : TextsDelegate (), m_bbox_valid (false)
{
  //  .. nothing yet ..
}

AsIfFlatTexts::AsIfFlatTexts (const AsIfFlatTexts &other)
  : TextsDelegate (other), m_bbox_valid (other.m_bbox_valid), m_bbox (other.m_bbox)
{
  //  .. nothing yet ..
}

AsIfFlatTexts::~AsIfFlatTexts ()
{
  //  .. nothing yet ..
}

AsIfFlatTexts &
AsIfFlatTexts::operator= (const AsIfFlatTexts &other)
{
  if (this != &other) {
    m_bbox_valid = other.m_bbox_valid;
    m_bbox = other.m_bbox;
  }
  return *this;
}

std::string
AsIfFlatTexts::to_string (size_t nmax) const
{
  std::ostringstream os;
  TextsIterator p (begin ());
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

TextsDelegate *
AsIfFlatTexts::in (const Texts &other, bool invert) const
{
  std::set <db::Text> op;
  for (TextsIterator o (other.begin ()); ! o.at_end (); ++o) {
    op.insert (*o);
  }

  std::unique_ptr<FlatTexts> new_texts (new FlatTexts ());

  for (TextsIterator o (begin ()); ! o.at_end (); ++o) {
    if ((op.find (*o) == op.end ()) == invert) {
      new_texts->insert (*o);
    }
  }

  return new_texts.release ();
}

size_t
AsIfFlatTexts::count () const
{
  size_t n = 0;
  for (TextsIterator t (begin ()); ! t.at_end (); ++t) {
    ++n;
  }
  return n;
}

size_t
AsIfFlatTexts::hier_count () const
{
  return count ();
}

Box AsIfFlatTexts::bbox () const
{
  if (! m_bbox_valid) {
    m_bbox = compute_bbox ();
    m_bbox_valid = true;
  }
  return m_bbox;
}

Box AsIfFlatTexts::compute_bbox () const
{
  db::Box b;
  for (TextsIterator t (begin ()); ! t.at_end (); ++t) {
    b += t->box ();
  }
  return b;
}

void AsIfFlatTexts::update_bbox (const db::Box &b)
{
  m_bbox = b;
  m_bbox_valid = true;
}

void AsIfFlatTexts::invalidate_bbox ()
{
  m_bbox_valid = false;
}

TextsDelegate *
AsIfFlatTexts::filtered (const TextFilterBase &filter) const
{
  std::unique_ptr<FlatTexts> new_texts (new FlatTexts ());

  for (TextsIterator p (begin ()); ! p.at_end (); ++p) {
    if (filter.selected (*p)) {
      new_texts->insert (*p);
    }
  }

  return new_texts.release ();
}

RegionDelegate *
AsIfFlatTexts::processed_to_polygons (const TextToPolygonProcessorBase &filter) const
{
  std::unique_ptr<FlatRegion> region (new FlatRegion ());

  if (filter.result_must_not_be_merged ()) {
    region->set_merged_semantics (false);
  }

  std::vector<db::Polygon> res_polygons;

  for (TextsIterator e (begin ()); ! e.at_end (); ++e) {
    res_polygons.clear ();
    filter.process (*e, res_polygons);
    for (std::vector<db::Polygon>::const_iterator pr = res_polygons.begin (); pr != res_polygons.end (); ++pr) {
      if (e.prop_id () != 0) {
        region->insert (db::PolygonWithProperties (*pr, e.prop_id ()));
      } else {
        region->insert (*pr);
      }
    }
  }

  return region.release ();
}

RegionDelegate *
AsIfFlatTexts::polygons (db::Coord e) const
{
  std::unique_ptr<FlatRegion> output (new FlatRegion ());

  for (TextsIterator tp (begin ()); ! tp.at_end (); ++tp) {
    db::Box box = tp->box ();
    box.enlarge (db::Vector (e, e));
    output->insert (db::Polygon (box));
  }

  return output.release ();
}

EdgesDelegate *
AsIfFlatTexts::edges () const
{
  std::unique_ptr<FlatEdges> output (new FlatEdges ());

  for (TextsIterator tp (begin ()); ! tp.at_end (); ++tp) {
    db::Box box = tp->box ();
    output->insert (db::Edge (box.p1 (), box.p2 ()));
  }

  return output.release ();
}

TextsDelegate *
AsIfFlatTexts::add (const Texts &other) const
{
  const FlatTexts *other_flat = dynamic_cast<const FlatTexts *> (other.delegate ());
  if (other_flat) {

    std::unique_ptr<FlatTexts> new_texts (new FlatTexts (*other_flat));
    new_texts->invalidate_cache ();

    size_t n = new_texts->raw_texts ().size () + count ();

    new_texts->reserve (n);

    for (TextsIterator p (begin ()); ! p.at_end (); ++p) {
      new_texts->raw_texts ().insert (*p);
    }

    return new_texts.release ();

  } else {

    std::unique_ptr<FlatTexts> new_texts (new FlatTexts ());

    size_t n = count () + other.count ();

    new_texts->reserve (n);

    for (TextsIterator p (begin ()); ! p.at_end (); ++p) {
      new_texts->raw_texts ().insert (*p);
    }
    for (TextsIterator p (other.begin ()); ! p.at_end (); ++p) {
      new_texts->raw_texts ().insert (*p);
    }

    return new_texts.release ();

  }
}

bool
AsIfFlatTexts::equals (const Texts &other) const
{
  if (empty () != other.empty ()) {
    return false;
  }
  if (count () != other.count ()) {
    return false;
  }
  TextsIterator o1 (begin ());
  TextsIterator o2 (other.begin ());
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
AsIfFlatTexts::less (const Texts &other) const
{
  if (empty () != other.empty ()) {
    return empty () < other.empty ();
  }
  if (count () != other.count ()) {
    return (count () < other.count ());
  }
  TextsIterator o1 (begin ());
  TextsIterator o2 (other.begin ());
  while (! o1.at_end () && ! o2.at_end ()) {
    if (*o1 != *o2) {
      return *o1 < *o2;
    }
    ++o1;
    ++o2;
  }
  return false;
}

void
AsIfFlatTexts::insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
{
  //  improves performance when inserting an original layout into the same layout
  db::LayoutLocker locker (layout);

  db::Shapes &shapes = layout->cell (into_cell).shapes (into_layer);
  for (TextsIterator t (begin ()); ! t.at_end (); ++t) {
    shapes.insert (*t);
  }
}

void
AsIfFlatTexts::insert_into_as_polygons (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl) const
{
  //  improves performance when inserting an original layout into the same layout
  db::LayoutLocker locker (layout);

  db::Shapes &shapes = layout->cell (into_cell).shapes (into_layer);
  for (TextsIterator t (begin ()); ! t.at_end (); ++t) {
    db::Box box = t->box ();
    box.enlarge (db::Vector (enl, enl));
    shapes.insert (db::SimplePolygon (box));
  }
}

TextsDelegate *
AsIfFlatTexts::selected_interacting_generic (const Region &other, bool inverse) const
{
  //  shortcuts
  if (other.empty () || empty ()) {
    return new EmptyTexts ();
  }

  db::box_scanner2<db::Text, size_t, db::Polygon, size_t> scanner (report_progress (), progress_desc ());

  AddressableTextDelivery e (begin ());

  for ( ; ! e.at_end (); ++e) {
    scanner.insert1 (e.operator-> (), 0);
  }

  AddressablePolygonDelivery p = other.addressable_polygons ();

  for ( ; ! p.at_end (); ++p) {
    scanner.insert2 (p.operator-> (), 1);
  }

  std::unique_ptr<FlatTexts> output (new FlatTexts ());

  if (! inverse) {

    text_to_region_interaction_filter<FlatTexts, db::Text> filter (*output);
    scanner.process (filter, 1, db::box_convert<db::Text> (), db::box_convert<db::Polygon> ());

  } else {

    std::set<db::Text> interacting;
    text_to_region_interaction_filter<std::set<db::Text>, db::Text> filter (interacting);
    scanner.process (filter, 1, db::box_convert<db::Text> (), db::box_convert<db::Polygon> ());

    for (TextsIterator o (begin ()); ! o.at_end (); ++o) {
      if (interacting.find (*o) == interacting.end ()) {
        output->insert (*o);
      }
    }

  }

  return output.release ();
}

RegionDelegate *
AsIfFlatTexts::pull_generic (const Region &other) const
{
  //  shortcuts
  if (other.empty () || empty ()) {
    return new EmptyRegion ();
  }

  db::box_scanner2<db::Text, size_t, db::Polygon, size_t> scanner (report_progress (), progress_desc ());

  AddressableTextDelivery e (begin ());

  for ( ; ! e.at_end (); ++e) {
    scanner.insert1 (e.operator-> (), 0);
  }

  AddressablePolygonDelivery p = other.addressable_merged_polygons ();

  for ( ; ! p.at_end (); ++p) {
    scanner.insert2 (p.operator-> (), 1);
  }

  std::unique_ptr<FlatRegion> output (new FlatRegion (true));

  text_to_region_interaction_filter<FlatRegion, db::Text> filter (*output);
  scanner.process (filter, 1, db::box_convert<db::Text> (), db::box_convert<db::Polygon> ());

  return output.release ();
}

RegionDelegate *
AsIfFlatTexts::pull_interacting (const Region &other) const
{
  return pull_generic (other);
}

TextsDelegate *
AsIfFlatTexts::selected_interacting (const Region &other) const
{
  return selected_interacting_generic (other, false);
}

TextsDelegate *
AsIfFlatTexts::selected_not_interacting (const Region &other) const
{
  return selected_interacting_generic (other, true);
}

}

