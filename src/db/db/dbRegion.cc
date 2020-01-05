
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


#include "dbRegion.h"
#include "dbOriginalLayerRegion.h"
#include "dbEmptyRegion.h"
#include "dbFlatRegion.h"
#include "dbDeepRegion.h"
#include "dbDeepEdges.h"
#include "dbFlatEdges.h"
#include "dbPolygonTools.h"
#include "tlGlobPattern.h"

namespace db
{

namespace
{

// -------------------------------------------------------------------------------------------------------------
//  Strange polygon processor

/**
 *  @brief A helper class to implement the strange polygon detector
 */
struct StrangePolygonInsideFunc
{
  inline bool operator() (int wc) const
  {
    return wc < 0 || wc > 1;
  }
};

class StrangePolygonCheckProcessor
  : public PolygonProcessorBase
{
public:
  StrangePolygonCheckProcessor () { }

  virtual void process (const db::Polygon &poly, std::vector<db::Polygon> &res) const
  {
    EdgeProcessor ep;
    ep.insert (poly);

    StrangePolygonInsideFunc inside;
    db::GenericMerge<StrangePolygonInsideFunc> op (inside);
    db::PolygonContainer pc (res, false);
    db::PolygonGenerator pg (pc, false, false);
    ep.process (pg, op);
  }

  virtual const TransformationReducer *vars () const { return 0; }
  virtual bool result_is_merged () const { return false; }
  virtual bool requires_raw_input () const { return true; }
  virtual bool wants_variants () const { return true; }
  virtual bool result_must_not_be_merged () const { return false; }
};

// -------------------------------------------------------------------------------------------------------------
//  Smoothing processor

class SmoothingProcessor
  : public PolygonProcessorBase
{
public:
  SmoothingProcessor (db::Coord d) : m_d (d) { }

  virtual void process (const db::Polygon &poly, std::vector<db::Polygon> &res) const
  {
    res.push_back (db::smooth (poly, m_d));
  }

  virtual const TransformationReducer *vars () const { return &m_vars; }
  virtual bool result_is_merged () const { return false; }
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return true; }
  virtual bool result_must_not_be_merged () const { return false; }

private:
  db::Coord m_d;
  db::MagnificationReducer m_vars;
};

// -------------------------------------------------------------------------------------------------------------
//  Rounded corners processor

class RoundedCornersProcessor
  : public PolygonProcessorBase
{
public:
  RoundedCornersProcessor (double rinner, double router, unsigned int n)
    : m_rinner (rinner), m_router (router), m_n (n)
  { }

  virtual void process (const db::Polygon &poly, std::vector<db::Polygon> &res) const
  {
    res.push_back (db::compute_rounded (poly, m_rinner, m_router, m_n));
  }

  virtual const TransformationReducer *vars () const { return &m_vars; }
  virtual bool result_is_merged () const { return true; }   //  we believe so ...
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return true; }
  virtual bool result_must_not_be_merged () const { return false; }

private:
  double m_rinner, m_router;
  unsigned int m_n;
  db::MagnificationReducer m_vars;
};

// -------------------------------------------------------------------------------------------------------------
//  Holes decomposition processor

class HolesExtractionProcessor
  : public PolygonProcessorBase
{
public:
  HolesExtractionProcessor () { }

  virtual void process (const db::Polygon &poly, std::vector<db::Polygon> &res) const
  {
    for (size_t i = 0; i < poly.holes (); ++i) {
      res.push_back (db::Polygon ());
      res.back ().assign_hull (poly.begin_hole ((unsigned int) i), poly.end_hole ((unsigned int) i));
    }
  }

  virtual const TransformationReducer *vars () const { return 0; }
  virtual bool result_is_merged () const { return true; }   //  we believe so ...
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return true; }
  virtual bool result_must_not_be_merged () const { return false; }
};

// -------------------------------------------------------------------------------------------------------------
//  Hull extraction processor

class HullExtractionProcessor
  : public PolygonProcessorBase
{
public:
  HullExtractionProcessor () { }

  virtual void process (const db::Polygon &poly, std::vector<db::Polygon> &res) const
  {
    res.push_back (db::Polygon ());
    res.back ().assign_hull (poly.begin_hull (), poly.end_hull ());
  }

  virtual const TransformationReducer *vars () const { return 0; }
  virtual bool result_is_merged () const { return true; }   //  we believe so ...
  virtual bool requires_raw_input () const { return false; }
  virtual bool wants_variants () const { return true; }
  virtual bool result_must_not_be_merged () const { return false; }
};

}

// -------------------------------------------------------------------------------------------------------------
//  Region implementation

Region::Region ()
  : mp_delegate (new EmptyRegion ())
{
  //  .. nothing yet ..
}

Region::Region (RegionDelegate *delegate)
  : mp_delegate (delegate)
{
  //  .. nothing yet ..
}

Region::Region (const Region &other)
  : gsi::ObjectBase (), mp_delegate (other.mp_delegate->clone ())
{
  //  .. nothing yet ..
}

Region::~Region ()
{
  delete mp_delegate;
  mp_delegate = 0;
}

Region &Region::operator= (const Region &other)
{
  if (this != &other) {
    set_delegate (other.mp_delegate->clone (), false);
  }
  return *this;
}

Region::Region (const RecursiveShapeIterator &si)
{
  mp_delegate = new OriginalLayerRegion (si);
}

Region::Region (const RecursiveShapeIterator &si, const db::ICplxTrans &trans, bool merged_semantics)
{
  mp_delegate = new OriginalLayerRegion (si, trans, merged_semantics);
}

Region::Region (const RecursiveShapeIterator &si, DeepShapeStore &dss, double area_ratio, size_t max_vertex_count)
{
  mp_delegate = new DeepRegion (si, dss, area_ratio, max_vertex_count);
}

Region::Region (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans, bool merged_semantics, double area_ratio, size_t max_vertex_count)
{
  mp_delegate = new DeepRegion (si, dss, trans, merged_semantics, area_ratio, max_vertex_count);
}

Region::Region (DeepShapeStore &dss)
{
  tl_assert (dss.is_singular ());
  unsigned int layout_index = 0; // singular layout index
  mp_delegate = new db::DeepRegion (db::DeepLayer (&dss, layout_index, dss.layout (layout_index).insert_layer ()));
}

const db::RecursiveShapeIterator &
Region::iter () const
{
  static db::RecursiveShapeIterator def_iter;
  const db::RecursiveShapeIterator *i = mp_delegate->iter ();
  return *(i ? i : &def_iter);
}

void
Region::set_delegate (RegionDelegate *delegate, bool keep_attributes)
{
  if (delegate != mp_delegate) {
    if (keep_attributes && delegate && mp_delegate) {
      //  copy the basic attributes like #threads etc.
      delegate->RegionDelegate::operator= (*mp_delegate);
    }
    delete mp_delegate;
    mp_delegate = delegate;
  }
}

void
Region::clear ()
{
  set_delegate (new EmptyRegion ());
}

void
Region::reserve (size_t n)
{
  flat_region ()->reserve (n);
}

template <class T>
Region &Region::transform (const T &trans)
{
  flat_region ()->transform (trans);
  return *this;
}

//  explicit instantiations
template DB_PUBLIC Region &Region::transform (const db::ICplxTrans &);
template DB_PUBLIC Region &Region::transform (const db::Trans &);
template DB_PUBLIC Region &Region::transform (const db::Disp &);

template <class Sh>
void Region::insert (const Sh &shape)
{
  flat_region ()->insert (shape);
}

template DB_PUBLIC void Region::insert (const db::Box &);
template DB_PUBLIC void Region::insert (const db::SimplePolygon &);
template DB_PUBLIC void Region::insert (const db::Polygon &);
template DB_PUBLIC void Region::insert (const db::Path &);

void Region::insert (const db::Shape &shape)
{
  flat_region ()->insert (shape);
}

template <class T>
void Region::insert (const db::Shape &shape, const T &trans)
{
  flat_region ()->insert (shape, trans);
}

template DB_PUBLIC void Region::insert (const db::Shape &, const db::ICplxTrans &);
template DB_PUBLIC void Region::insert (const db::Shape &, const db::Trans &);
template DB_PUBLIC void Region::insert (const db::Shape &, const db::Disp &);

FlatRegion *
Region::flat_region ()
{
  FlatRegion *region = dynamic_cast<FlatRegion *> (mp_delegate);
  if (! region) {
    region = new FlatRegion ();
    if (mp_delegate) {
      region->RegionDelegate::operator= (*mp_delegate);   //  copy basic flags
      region->insert_seq (begin ());
      region->set_is_merged (mp_delegate->is_merged ());
    }
    set_delegate (region);
  }

  return region;
}

Region &
Region::size (coord_type d, unsigned int mode)
{
  set_delegate (mp_delegate->sized (d, mode));
  return *this;
}

Region &
Region::size (coord_type dx, coord_type dy, unsigned int mode)
{
  set_delegate (mp_delegate->sized (dx, dy, mode));
  return *this;
}

Region
Region::sized (coord_type d, unsigned int mode) const
{
  return Region (mp_delegate->sized (d, mode));
}

Region
Region::sized (coord_type dx, coord_type dy, unsigned int mode) const
{
  return Region (mp_delegate->sized (dx, dy, mode));
}

void
Region::round_corners (double rinner, double router, unsigned int n)
{
  process (RoundedCornersProcessor (rinner, router, n));
}

Region
Region::rounded_corners (double rinner, double router, unsigned int n) const
{
  return processed (RoundedCornersProcessor (rinner, router, n));
}

void
Region::smooth (coord_type d)
{
  process (SmoothingProcessor (d));
}

Region
Region::smoothed (coord_type d) const
{
  return processed (SmoothingProcessor (d));
}

void
Region::snap (db::Coord gx, db::Coord gy)
{
  set_delegate (mp_delegate->snapped_in_place (gx, gy));
}

Region
Region::snapped (db::Coord gx, db::Coord gy) const
{
  return Region (mp_delegate->snapped (gx, gy));
}

void
Region::scale_and_snap (db::Coord gx, db::Coord mx, db::Coord dx, db::Coord gy, db::Coord my, db::Coord dy)
{
  set_delegate (mp_delegate->scaled_and_snapped_in_place (gx, mx, dx, gy, my, dy));
}

Region
Region::scaled_and_snapped (db::Coord gx, db::Coord mx, db::Coord dx, db::Coord gy, db::Coord my, db::Coord dy) const
{
  return Region (mp_delegate->scaled_and_snapped (gx, mx, dx, gy, my, dy));
}

Region
Region::strange_polygon_check () const
{
  return Region (processed (StrangePolygonCheckProcessor ()));
}

Region
Region::holes () const
{
  return Region (processed (HolesExtractionProcessor ()));
}

Region
Region::hulls () const
{
  return Region (processed (HullExtractionProcessor ()));
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
static void fill_texts (const Iter &iter, const std::string &pat, bool pattern, const Delivery &delivery, typename Delivery::container_type *container, const db::ICplxTrans &trans, const db::DeepRegion *org_deep)
{
  std::pair<bool, db::property_names_id_type> text_annot_name_id;
  const db::Layout *layout = 0;

  if (org_deep) {
    layout = &org_deep->deep_layer ().layout ();
    const db::DeepShapeStore *store = org_deep->deep_layer ().store ();
    if (! store->text_property_name ().is_nil ()) {
      text_annot_name_id = layout->properties_repository ().get_id_of_name (store->text_property_name ());
    }
  }

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

    bool is_text = false;
    std::string text_string;

    if (si->is_text ()) {

      //  a raw text
      is_text = true;
      text_string = si->text_string ();

    } else if (layout && text_annot_name_id.first && si->prop_id () > 0) {

      //  a text marker
      const db::PropertiesRepository::properties_set &ps = layout->properties_repository ().properties (si->prop_id ());

      for (db::PropertiesRepository::properties_set::const_iterator j = ps.begin (); j != ps.end () && ! is_text; ++j) {
        if (j->first == text_annot_name_id.second) {
          text_string = j->second.to_string ();
          is_text = true;
        }
      }

    }

    if (is_text &&
        (all || (pattern && glob_pat.match (text_string)) || (!pattern && text_string == pat))) {
      delivery.insert (si.trans () * (trans * si->bbox ().center ()), container);
    }

  }
}

template <class Delivery>
class text_shape_receiver
  : public db::HierarchyBuilderShapeReceiver
{
public:
  text_shape_receiver (const Delivery &delivery, const std::string &pat, bool pattern, const db::DeepRegion *org_deep)
    : m_delivery (delivery), m_glob_pat (), m_all (false), m_pattern (pattern), m_pat (pat), m_text_annot_name_id (false, 0), mp_layout (0)
  {
    if (org_deep) {
      mp_layout = & org_deep->deep_layer ().layout ();
      const db::DeepShapeStore *store = org_deep->deep_layer ().store ();
      if (! store->text_property_name ().is_nil ()) {
        m_text_annot_name_id = mp_layout->properties_repository ().get_id_of_name (store->text_property_name ());
      }
    }

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
    bool is_text = false;
    std::string text_string;

    if (shape.is_text ()) {

      //  a raw text
      is_text = true;
      text_string = shape.text_string ();

    } else if (mp_layout && m_text_annot_name_id.first && shape.prop_id () > 0) {

      //  a text marker
      const db::PropertiesRepository::properties_set &ps = mp_layout->properties_repository ().properties (shape.prop_id ());

      for (db::PropertiesRepository::properties_set::const_iterator j = ps.begin (); j != ps.end () && ! is_text; ++j) {
        if (j->first == m_text_annot_name_id.second) {
          text_string = j->second.to_string ();
          is_text = true;
        }
      }

    }

    if (is_text &&
        (m_all || (m_pattern && m_glob_pat.match (text_string)) || (!m_pattern && text_string == m_pat))) {

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
  std::pair<bool, db::property_names_id_type> m_text_annot_name_id;
  const db::Layout *mp_layout;
};

}

Edges
Region::texts_as_dots (const std::string &pat, bool pattern) const
{
  const db::DeepRegion *dr = dynamic_cast<const db::DeepRegion *> (delegate ());
  if (dr) {
    return texts_as_dots (pat, pattern, const_cast<db::DeepShapeStore &> (*dr->deep_layer ().store ()));
  }

  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> si = begin_iter ();
  if (! dr) {
    //  some optimization
    si.first.shape_flags (si.first.shape_flags () & db::ShapeIterator::Texts);
  }

  std::auto_ptr<db::FlatEdges> res (new db::FlatEdges ());
  res->set_merged_semantics (false);

  fill_texts (si.first, pat, pattern, dot_delivery<db::FlatEdges> (), res.get (), si.second, dr);

  return Edges (res.release ());
}

Edges
Region::texts_as_dots (const std::string &pat, bool pattern, db::DeepShapeStore &store) const
{
  const db::DeepRegion *dr = dynamic_cast<const db::DeepRegion *> (delegate ());

  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> si = begin_iter ();
  if (! dr) {
    //  some optimization
    si.first.shape_flags (si.first.shape_flags () & db::ShapeIterator::Texts);
  }

  if (! si.first.layout ()) {

    //  flat fallback if the source isn't a deep or original layer
    std::auto_ptr<db::FlatEdges> res (new db::FlatEdges ());
    res->set_merged_semantics (false);

    fill_texts (si.first, pat, pattern, dot_delivery<db::FlatEdges> (), res.get (), si.second, dr);

    return Edges (res.release ());

  }

  text_shape_receiver<dot_delivery<db::Shapes> > pipe = text_shape_receiver<dot_delivery<db::Shapes> > (dot_delivery<db::Shapes> (), pat, pattern, dr);
  if (dr && dr->deep_layer ().store () == &store) {
    return Edges (new db::DeepEdges (store.create_copy (dr->deep_layer (), &pipe)));
  } else {
    return Edges (new db::DeepEdges (store.create_custom_layer (si.first, &pipe, si.second)));
  }
}

Region
Region::texts_as_boxes (const std::string &pat, bool pattern, db::Coord enl) const
{
  const db::DeepRegion *dr = dynamic_cast<const db::DeepRegion *> (delegate ());
  if (dr) {
    return texts_as_boxes (pat, pattern, enl, const_cast<db::DeepShapeStore &> (*dr->deep_layer ().store ()));
  }

  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> si = begin_iter ();
  if (! dr) {
    //  some optimization
    si.first.shape_flags (si.first.shape_flags () & db::ShapeIterator::Texts);
  }

  std::auto_ptr<db::FlatRegion> res (new db::FlatRegion ());
  res->set_merged_semantics (false);

  fill_texts (si.first, pat, pattern, box_delivery<db::FlatRegion> (enl), res.get (), si.second, dr);

  return Region (res.release ());
}

Region
Region::texts_as_boxes (const std::string &pat, bool pattern, db::Coord enl, db::DeepShapeStore &store) const
{
  const db::DeepRegion *dr = dynamic_cast<const db::DeepRegion *> (delegate ());

  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> si = begin_iter ();
  if (! dr) {
    //  some optimization
    si.first.shape_flags (si.first.shape_flags () & db::ShapeIterator::Texts);
  }

  if (! si.first.layout ()) {

    //  flat fallback if the source isn't a deep or original layer
    std::auto_ptr<db::FlatRegion> res (new db::FlatRegion ());
    res->set_merged_semantics (false);

    fill_texts (si.first, pat, pattern, box_delivery<db::FlatRegion> (enl), res.get (), si.second, dr);

    return Region (res.release ());

  }

  text_shape_receiver<box_delivery<db::Shapes> > pipe = text_shape_receiver<box_delivery<db::Shapes> > (box_delivery<db::Shapes> (enl), pat, pattern, dr);
  if (dr && dr->deep_layer ().store () == &store) {
    return Region (new db::DeepRegion (store.create_copy (dr->deep_layer (), &pipe)));
  } else {
    return Region (new db::DeepRegion (store.create_custom_layer (si.first, &pipe, si.second)));
  }
}

}

namespace tl
{
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::Region &b)
  {
    db::Polygon p;

    if (! ex.try_read (p)) {
      return false;
    }
    b.insert (p);

    while (ex.test (";")) {
      ex.read (p);
      b.insert (p);
    } 

    return true;
  }

  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::Region &b)
  {
    if (! test_extractor_impl (ex, b)) {
      ex.error (tl::to_string (tr ("Expected an region collection specification")));
    }
  }
}

