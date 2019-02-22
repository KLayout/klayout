
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


#include "dbOriginalLayerRegion.h"
#include "dbFlatRegion.h"
#include "dbFlatEdges.h"
#include "dbRegion.h"
#include "dbShapeProcessor.h"
#include "dbDeepEdges.h"
#include "dbDeepRegion.h"
#include "dbDeepShapeStore.h"
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
      : m_rec_iter (iter), m_iter_trans (trans)
    {
      set ();
    }

    virtual bool at_end () const
    {
      return m_rec_iter.at_end ();
    }

    virtual void increment ()
    {
      inc ();
      set ();
    }

    virtual const value_type *get () const
    {
      return &m_polygon;
    }

    virtual RegionIteratorDelegate *clone () const
    {
      return new OriginalLayerRegionIterator (*this);
    }

  private:
    friend class Region;

    db::RecursiveShapeIterator m_rec_iter;
    db::ICplxTrans m_iter_trans;
    db::Polygon m_polygon;

    void set ()
    {
      while (! m_rec_iter.at_end () && ! (m_rec_iter.shape ().is_polygon () || m_rec_iter.shape ().is_path () || m_rec_iter.shape ().is_box ())) {
        ++m_rec_iter;
      }
      if (! m_rec_iter.at_end ()) {
        m_rec_iter.shape ().polygon (m_polygon);
        m_polygon.transform (m_iter_trans * m_rec_iter.trans (), false);
      }
    }

    void inc ()
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
    return new FlatRegionIterator (m_merged_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().begin (), m_merged_polygons.get_layer<db::Polygon, db::unstable_layer_tag> ().end ());
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
  m_is_merged = true;
  m_merged_polygons_valid = false;
}

void
OriginalLayerRegion::ensure_merged_polygons_valid () const
{
  if (! m_merged_polygons_valid) {

    m_merged_polygons.clear ();

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

    //  and run the merge step
    db::MergeOp op (0);
    db::ShapeGenerator pc (m_merged_polygons);
    db::PolygonGenerator pg (pc, false /*don't resolve holes*/, min_coherence ());
    ep.process (pg, op);

    m_merged_polygons_valid = true;

  }
}

namespace
{

template <class Container>
struct dot_delivery
{
  typedef Container container_type;

  dot_delivery ()
  {
    //  .. nothing yet ..
  }

  void insert (const db::Point &pt, Container *container) const
  {
    container->insert (db::Edge (pt, pt));
  }
};

template <class Container>
struct box_delivery
{
  typedef Container container_type;

  box_delivery (db::Coord enl)
    : m_d (enl, enl)
  {
    //  .. nothing yet ..
  }

  void insert (const db::Point &pt, Container *container) const
  {
    container->insert (db::Box (pt - m_d, pt + m_d));
  }

private:
  db::Vector m_d;
};

template <class Iter, class Delivery>
static void fill_texts (const Iter &iter, const std::string &pat, bool pattern, const Delivery &delivery, typename Delivery::container_type *container)
{
  tl::GlobPattern glob_pat;
  bool all = false;
  if (pattern) {
    if (pat == "*") {
      all = true;
    } else {
      glob_pat = tl::GlobPattern (pat);
    }
  }

  for (Iter si = iter; ! si.at_end (); ++si) {
    if (si->is_text () &&
        (all || (pattern && glob_pat.match (si->text_string ())) || (!pattern && si->text_string () == pat))) {
      db::Text t;
      si->text (t);
      t.transform (si.trans ());
      delivery.insert (t.box ().center (), container);
    }
  }
}

template <class Delivery>
class text_shape_receiver
  : public db::HierarchyBuilderShapeReceiver
{
public:
  text_shape_receiver (const Delivery &delivery, const std::string &pat, bool pattern)
    : m_delivery (delivery), m_glob_pat (), m_all (false), m_pattern (pattern), m_pat (pat)
  {
    if (pattern) {
      if (m_pat == "*") {
        m_all = true;
      } else {
        m_glob_pat = tl::GlobPattern (pat);
      }
    }
  }

  virtual void push (const db::Shape &shape, const db::ICplxTrans &trans, const db::Box &region, const db::RecursiveShapeReceiver::box_tree_type *complex_region, db::Shapes *target)
  {
    if (shape.is_text () &&
        (m_all || (m_pattern && m_glob_pat.match (shape.text_string ())) || (!m_pattern && shape.text_string () == m_pat))) {

      db::Point pt = shape.bbox ().center ();

      if (! complex_region) {
        if (region.contains (pt)) {
          m_delivery.insert (pt.transformed (trans), target);
        }
      } else {
        if (! complex_region->begin_overlapping (db::Box (pt, pt), db::box_convert<db::Box> ()).at_end ()) {
          m_delivery.insert (pt.transformed (trans), target);
        }
      }

    }
  }

  virtual void push (const db::Box &, const db::ICplxTrans &, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *) { }
  virtual void push (const db::Polygon &, const db::ICplxTrans &, const db::Box &, const db::RecursiveShapeReceiver::box_tree_type *, db::Shapes *) { }

private:
  Delivery m_delivery;
  tl::GlobPattern m_glob_pat;
  bool m_all;
  bool m_pattern;
  std::string m_pat;
};

}

db::EdgesDelegate *
OriginalLayerRegion::texts_as_dots (const std::string &pat, bool pattern) const
{
  db::RecursiveShapeIterator iter (m_iter);
  iter.shape_flags (db::ShapeIterator::Texts);

  std::auto_ptr<db::FlatEdges> res (new db::FlatEdges ());
  res->set_merged_semantics (false);

  fill_texts (iter, pat, pattern, dot_delivery<db::FlatEdges> (), res.get ());

  return res.release ();
}

db::EdgesDelegate *
OriginalLayerRegion::texts_as_dots (const std::string &pat, bool pattern, db::DeepShapeStore &store) const
{
  db::RecursiveShapeIterator iter (m_iter);
  iter.shape_flags (db::ShapeIterator::Texts);

  text_shape_receiver<dot_delivery<db::Shapes> > pipe = text_shape_receiver<dot_delivery<db::Shapes> > (dot_delivery<db::Shapes> (), pat, pattern);
  return new db::DeepEdges (store.create_custom_layer (iter, &pipe));
}

db::RegionDelegate *
OriginalLayerRegion::texts_as_boxes (const std::string &pat, bool pattern, db::Coord enl) const
{
  db::RecursiveShapeIterator iter (m_iter);
  iter.shape_flags (db::ShapeIterator::Texts);

  std::auto_ptr<db::FlatRegion> res (new db::FlatRegion ());
  res->set_merged_semantics (false);

  fill_texts (iter, pat, pattern, box_delivery<db::FlatRegion> (enl), res.get ());

  return res.release ();
}

db::RegionDelegate *
OriginalLayerRegion::texts_as_boxes (const std::string &pat, bool pattern, db::Coord enl, db::DeepShapeStore &store) const
{
  db::RecursiveShapeIterator iter (m_iter);
  iter.shape_flags (db::ShapeIterator::Texts);

  text_shape_receiver<box_delivery<db::Shapes> > pipe = text_shape_receiver<box_delivery<db::Shapes> > (box_delivery<db::Shapes> (enl), pat, pattern);
  return new db::DeepRegion (store.create_custom_layer (iter, &pipe));
}

}
