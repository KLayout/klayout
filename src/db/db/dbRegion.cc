
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


#include "dbRegion.h"
#include "dbOriginalLayerRegion.h"
#include "dbEmptyRegion.h"
#include "dbFlatRegion.h"
#include "dbDeepRegion.h"
#include "dbPolygonTools.h"

namespace db
{

namespace
{

// -------------------------------------------------------------------------------------------------------------
//  Strange polygon processor

/**
 *  @brief A helper class to implement the strange polygon detector
 */
struct DB_PUBLIC StrangePolygonInsideFunc
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
  : mp_delegate (other.mp_delegate->clone ())
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
      region->insert_seq (begin ());
    }
    if (mp_delegate) {
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

