
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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


#ifndef HDR_dbEdges
#define HDR_dbEdges

#include "dbCommon.h"

#include "dbTypes.h"
#include "dbEdge.h"
#include "dbTrans.h"
#include "dbShape.h"
#include "dbShapes.h"
#include "dbShapes2.h"
#include "dbEdgePairRelations.h"
#include "dbEdgePairs.h"
#include "dbRecursiveShapeIterator.h"
#include "tlString.h"

namespace db {

class Edges;

/**
 *  @brief A base class for polygon filters
 */
class DB_PUBLIC EdgeFilterBase
{
public:
  EdgeFilterBase () { }
  virtual ~EdgeFilterBase () { }

  virtual bool selected (const db::Edge &edge) const = 0;
};

/**
 *  @brief An edge length filter for use with Edges::filter or Edges::filtered
 *
 *  This filter has two parameters: lmin and lmax.
 *  It will filter all edges for which the length is >= lmin and < lmax.
 *  There is an "invert" flag which allows to select all edges not
 *  matching the criterion.
 */

struct DB_PUBLIC EdgeLengthFilter
  : public EdgeFilterBase
{
  typedef db::Edge::distance_type length_type;

  /**
   *  @brief Constructor 
   *
   *  @param lmin The minimum length
   *  @param lmax The maximum length
   *  @param inverse If set to true, only polygons not matching this criterion will be filtered
   */
  EdgeLengthFilter (length_type lmin, length_type lmax, bool inverse)
    : m_lmin (lmin), m_lmax (lmax), m_inverse (inverse)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Returns true if the edge length matches the criterion
   */
  bool selected (const db::Edge &edge) const
  {
    length_type l = edge.length ();
    if (! m_inverse) {
      return l >= m_lmin && l < m_lmax;
    } else {
      return ! (l >= m_lmin && l < m_lmax);
    }
  }

private:
  length_type m_lmin, m_lmax;
  bool m_inverse;
};

/**
 *  @brief An edge orientation filter for use with Edges::filter or Edges::filtered
 *
 *  This filter has two parameters: amin and amax.
 *  It will filter all edges for which the orientation angle is >= amin and < amax.
 *  The orientation angle is measured in degree against the x axis in the mathematical sense.
 *  There is an "invert" flag which allows to select all edges not
 *  matching the criterion.
 */

struct DB_PUBLIC EdgeOrientationFilter
  : public EdgeFilterBase
{
  /**
   *  @brief Constructor 
   *
   *  @param amin The minimum angle (measured against the x axis)
   *  @param amax The maximum angle (measured against the x axis)
   *  @param inverse If set to true, only polygons not matching this criterion will be filtered
   *
   *  This filter will filter out all edges whose angle against x axis 
   *  is larger or equal to amin and less than amax.
   */
  EdgeOrientationFilter (double amin, double amax, bool inverse)
    : m_inverse (inverse), m_exact (false)
  {
    m_emin = db::DVector (cos (amin * M_PI / 180.0), sin (amin * M_PI / 180.0));
    m_emax = db::DVector (cos (amax * M_PI / 180.0), sin (amax * M_PI / 180.0));
  }

  /**
   *  @brief Constructor 
   *
   *  @param a The angle (measured against the x axis)
   *  @param inverse If set to true, only polygons not matching this criterion will be filtered
   *
   *  This filter will filter out all edges whose angle against x axis 
   *  is equal to a.
   */
  EdgeOrientationFilter (double a, bool inverse)
    : m_inverse (inverse), m_exact (true)
  {
    m_emin = db::DVector (cos (a * M_PI / 180.0), sin (a * M_PI / 180.0));
  }

  /**
   *  @brief Returns true if the edge orientation matches the criterion
   */
  bool selected (const db::Edge &edge) const
  {
    int smin = db::vprod_sign (m_emin, db::DVector (edge.d ()));
    if (m_exact) {
      if (! m_inverse) {
        return smin == 0;
      } else {
        return smin != 0;
      }
    } else {
      int smax = db::vprod_sign (m_emax, db::DVector (edge.d ()));
      if (! m_inverse) {
        return (smin >= 0 && smax < 0) || (smax > 0 && smin <= 0);
      } else {
        return ! ((smin >= 0 && smax < 0) || (smax > 0 && smin <= 0));
      }
    }
  }

private:
  db::DVector m_emin, m_emax;
  bool m_inverse;
  bool m_exact;
};

/**
 *  @brief A edge collection iterator
 *
 *  The iterator delivers the edges of the edge collection
 */

class DB_PUBLIC EdgesIterator
{
public:
  typedef db::Edge value_type; 
  typedef const db::Edge &reference;
  typedef const db::Edge *pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef void difference_type;

  /**
   *  @Returns true, if the iterator is at the end
   */
  bool at_end () const
  {
    return m_from == m_to && m_rec_iter.at_end ();
  }

  /**
   *  @brief Increment
   */
  EdgesIterator &operator++ () 
  {
    inc ();
    set ();
    return *this;
  }

  /**
   *  @brief Access
   */
  reference operator* () const
  {
    if (m_rec_iter.at_end ()) {
      return *m_from;
    } else {
      return m_edge;
    }
  }

  /**
   *  @brief Access
   */
  pointer operator-> () const
  {
    if (m_rec_iter.at_end ()) {
      return &*m_from;
    } else {
      return &m_edge;
    }
  }

private:
  friend class Edges;

  typedef db::layer<db::Edge, db::unstable_layer_tag> edge_layer_type;
  typedef edge_layer_type::iterator iterator_type;

  db::RecursiveShapeIterator m_rec_iter;
  db::ICplxTrans m_iter_trans;
  db::Edge m_edge;
  iterator_type m_from, m_to;

  /**
   *  @brief ctor from a recursive shape iterator
   */
  EdgesIterator (const db::RecursiveShapeIterator &iter, const db::ICplxTrans &trans)
    : m_rec_iter (iter), m_iter_trans (trans), m_from (), m_to ()
  {
    //  NOTE: the following initialization appears to be required on some compilers
    //  (specifically MacOS/clang) to ensure the proper initialization of the iterators
    m_from = m_to;
    set ();
  }

  /**
   *  @brief ctor from a range of edges inside a vector
   */
  EdgesIterator (iterator_type from, iterator_type to)
    : m_from (from), m_to (to)
  { 
    //  no required yet: set ();
  }

  /**
   *  @brief Establish the iterator at the current position
   */
  void set ()
  {
    while (! m_rec_iter.at_end () && ! m_rec_iter.shape ().is_edge ()) {
      inc ();
    }
    if (! m_rec_iter.at_end ()) {
      m_rec_iter.shape ().edge (m_edge);
      m_edge.transform (m_iter_trans * m_rec_iter.trans ());
    } 
  }

  /**
   *  @brief Increment the iterator
   */
  void inc ()
  {
    if (! m_rec_iter.at_end ()) {
      ++m_rec_iter;
    } else {
      ++m_from;
    }
  }
};

/**
 *  @brief An edge set
 *
 *  An edge set is basically a collection of edges. They do not necessarily need to form closed contours. 
 *  Edges can be manipulated in various ways. Edge sets closely cooperate with the Region class which is a
 *  set of polygons.
 *
 *  Edge sets have some methods in common with regions. Edge sets can also be merged, which means that 
 *  edges which are continuations of other edges are joined.
 *
 *  Edge sets can contain degenerated edges. Such edges are some which have identical start and end points.
 *  Such edges are basically points which have some applications, i.e. as markers for certain locations.
 */

class DB_PUBLIC Edges 
{
public:
  typedef db::Coord coord_type;
  typedef db::coord_traits<db::Coord> coord_traits;
  typedef db::Edge edge_type;
  typedef db::Vector vector_type;
  typedef db::Point point_type;
  typedef db::Box box_type;
  typedef coord_traits::distance_type distance_type; 
  typedef db::Edge::distance_type length_type;
  typedef EdgesIterator const_iterator;
  enum BoolOp { Or, Not, Xor, And };

  /** 
   *  @brief Default constructor
   *
   *  This constructor creates an empty edge set.
   */
  Edges ()
    : m_edges (false), m_merged_edges (false)
  {
    init ();
  }

  /**
   *  @brief Constructor from an object
   *
   *  Creates a region representing a single instance of that object.
   *  The object is converted to a polygon and the edges of that polygon are inserted.
   */
  template <class Sh>
  Edges (const Sh &s)
    : m_edges (false), m_merged_edges (false)
  {
    init ();
    insert (s);
  }

  /**
   *  @brief Sequence constructor
   *
   *  Creates a region from a sequence of objects. The objects can be edges, boxes, 
   *  polygons, paths or shapes. This version accepts iterators of the begin ... end
   *  style.
   */
  template <class Iter>
  Edges (const Iter &b, const Iter &e)
    : m_edges (false), m_merged_edges (false)
  {
    init ();
    reserve (e - b);
    for (Iter i = b; i != e; ++i) {
      insert (*i);
    }
  }

  /**
   *  @brief Constructor from a RecursiveShapeIterator
   *
   *  Creates a region from a recursive shape iterator. This allows to feed an edge set
   *  from a hierarchy of cells.
   *
   *  If as_edges is false, only edges will be taken from the recursive shape iterator.
   *  That is somewhat more efficient since it can avoid a copy in some cases. If as_edges
   *  is false, shapes will be converted to edges.
   */
  Edges (const RecursiveShapeIterator &si, bool as_edges = true);

  /**
   *  @brief Constructor from a RecursiveShapeIterator with a transformation
   *
   *  Creates a region from a recursive shape iterator. This allows to feed an edge set
   *  from a hierarchy of cells. The transformation is useful to scale to a specific
   *  DBU for example.
   *
   *  If as_edges is true, only edges will be taken from the recursive shape iterator.
   *  That is somewhat more efficient since it can avoid a copy in some cases. If as_edges
   *  is false, shapes will be converted to edges.
   */
  Edges (const RecursiveShapeIterator &si, const db::ICplxTrans &trans, bool as_edges = true, bool merged_semantics = true);

  /**
   *  @brief Enable progress reporting
   *
   *  @param progress_text The description text of the progress object
   */
  void enable_progress (const std::string &progress_desc = std::string ());

  /**
   *  @brief Disable progress reporting
   */
  void disable_progress ();

  /**
   *  @brief Iterator of the edge set
   *
   *  The iterator delivers the edges of the edge set.
   *  It follows the at_end semantics.
   */
  const_iterator begin () const
  {
    if (has_valid_edges ()) {
      return const_iterator (m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().begin (), m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().end ());
    } else {
      return const_iterator (m_iter, m_iter_trans);
    }
  }

  /**
   *  @brief Returns the merged edges if merge semantics applies 
   *
   *  If merge semantics is not enabled, this iterator delivers the individual edges.
   */
  const_iterator begin_merged () const;

  /**
   *  @brief Delivers a RecursiveShapeIterator pointing to the edges plus the necessary transformation
   */
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const;

  /**
   *  @brief Delivers a RecursiveShapeIterator pointing to the merged edges plus the necessary transformation
   */
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_merged_iter () const;

  /**
   *  @brief Insert an edge into the edge set
   */
  void insert (const db::Edge &edge);

  /**
   *  @brief Insert a box into the edge set
   *
   *  This method will insert all edges the box is composed of.
   */
  void insert (const db::Box &box);

  /**
   *  @brief Insert a path into the edge set
   *
   *  This method will insert all edges the path is composed of.
   */
  void insert (const db::Path &path);

  /**
   *  @brief Insert a simple polygon into the edge set
   *
   *  This method will insert all edges the polygon is composed of.
   */
  void insert (const db::SimplePolygon &polygon);

  /**
   *  @brief Insert a polygon into the edge set
   *
   *  This method will insert all edges the polygon is composed of.
   */
  void insert (const db::Polygon &polygon);

  /**
   *  @brief Insert a shape into the region
   *
   *  If the shape is a polygon-type, the shape is converted to a 
   *  polygon and it's edges are inserted into the edge set.
   *  If the shape is an edge, the edge is inserted into the edge set.
   */
  void insert (const db::Shape &shape)
  {
    if (shape.is_edge ()) {
      insert (shape.edge ());
    } else if (shape.is_polygon () || shape.is_path () || shape.is_box ()) {
      db::Polygon polygon;
      shape.polygon (polygon);
      for (db::Polygon::polygon_edge_iterator e = polygon.begin_edge (); ! e.at_end (); ++e) {
        insert (*e);
      }
    }
  }

  /**
   *  @brief Insert a transformed shape into the edge set
   */
  template <class T>
  void insert (const db::Shape &shape, const T &trans)
  {
    if (shape.is_edge ()) {
      insert (edge_type (trans * shape.edge ()));
    } else if (shape.is_polygon () || shape.is_path () || shape.is_box ()) {
      db::Polygon polygon;
      shape.polygon (polygon);
      for (db::Polygon::polygon_edge_iterator e = polygon.begin_edge (); ! e.at_end (); ++e) {
        insert (edge_type (trans * *e));
      }
    }
  }

  /**
   *  @brief Returns true if the region is empty
   */
  bool empty () const
  {
    return has_valid_edges () && m_edges.empty ();
  }

  /**
   *  @brief Returns the number of polygons in the region
   */
  size_t size () const;

  /**
   *  @brief Returns a string representing the region
   *
   *  nmax specifies how many polygons are included (set to std::numeric_limits<size_t>::max() for "all".
   */
  std::string to_string (size_t nmax = 10) const;

  /**
   *  @brief Clear the edge set
   */
  void clear ();

  /**
   *  @brief Reserve memory for the given number of edges
   */
  void reserve (size_t n)
  {
    m_edges.reserve (db::Edge::tag (), n);
  }

  /**
   *  @brief Sets the merged-semantics flag
   *
   *  If merged semantics is enabled (the default), coherent polygons will be considered 
   *  as single regions and artificial edges such as cut-lines will not be considered. 
   *  Merged semantics thus is equivalent to considering coherent areas rather than
   *  single polygons.
   */
  void set_merged_semantics (bool f);

  /**
   *  @brief Gets the merged-semantics flag
   */
  bool merged_semantics () const
  {
    return m_merged_semantics;
  }

  /**
   *  @brief Returns true if the region is merged 
   */
  bool is_merged () const
  {
    return m_is_merged;
  }

  /**
   *  @brief Returns the total length of the edges
   *  Merged semantics applies. In merged semantics, the length is the correct total length of the edges.
   *  Without merged semantics, overlapping parts are counted twice.
   *
   *  If a box is given, the computation is restricted to that box.
   *  Edges coincident with the box edges are counted only if the form outer edges at the box edge.
   */
  length_type length (const db::Box &box = db::Box ()) const;

  /**
   *  @brief Returns the bounding box of the region
   */
  Box bbox () const
  {
    ensure_bbox_valid ();
    return m_bbox;
  }

  /**
   *  @brief Filters the edge set 
   *
   *  This method will keep all edges for which the filter returns true.
   *  Merged semantics applies.
   */
  Edges &filter (EdgeFilterBase &filter)
  {
    edge_iterator_type ew = m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().begin ();
    for (const_iterator e = begin_merged (); ! e.at_end (); ++e) {
      if (filter.selected (*e)) {
        if (ew == m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().end ()) {
          m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().insert (*e);
          ew = m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().end ();
        } else {
          m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().replace (ew++, *e);
        } 
      }
    }
    m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().erase (ew, m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().end ());
    m_merged_edges.clear ();
    m_is_merged = m_merged_semantics;
    m_iter = db::RecursiveShapeIterator ();
    return *this;
  }

  /**
   *  @brief Returns the filtered edges
   *
   *  This method will return a new region with only those edges which 
   *  conform to the filter criterion.
   *  Merged semantics applies.
   */
  Edges filtered (const EdgeFilterBase &filter) const
  {
    Edges d;
    for (const_iterator e = begin_merged (); ! e.at_end (); ++e) {
      if (filter.selected (*e)) {
        d.insert (*e);
      }
    }
    return d;
  }

  /**
   *  @brief Returns all edges found in the other edge collection as well
   *
   *  If "invert" is true, all edges not found in the other collection
   *  are returned.
   */
  Edges in (const Edges &other, bool invert) const;

  /**
   *  @brief Transform the edge set
   */
  template <class T>
  Edges &transform (const T &trans)
  {
    if (! trans.is_unity ()) {
      ensure_valid_edges ();
      for (edge_iterator_type e = m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().begin (); e != m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().end (); ++e) {
        m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().replace (e, e->transformed (trans));
      }
      m_iter_trans = db::ICplxTrans (trans) * m_iter_trans;
      m_bbox_valid = false;
    }
    return *this;
  }

  /**
   *  @brief Returns the transformed edge set
   */
  template <class T>
  Edges transformed (const T &trans) const
  {
    Edges d (*this);
    d.transform (trans);
    return d;
  }

  /**
   *  @brief Swap with the other region
   */
  void swap (db::Edges &other);

  /**
   *  @brief returns the extended edges
   *
   *  Edges are extended by creating a rectangle on each edge. The rectangle is constructed on the 
   *  edge by applying the extensions given by ext_o, ext_b, ext_e, ext_i at the outside, the 
   *  beginning, the end and the inside.
   *  If the edge is laid flat pointing from left to right, the outside is at the top, the inside 
   *  is at the bottom.
   *
   *  For degenerated edges with length 0, the orientation is assumed to the horizontal. The extended
   *  method creates a rectangle with specified dimensions: ext_b to the left, ext_o to the top, ext_i to the bottom
   *  and ext_e to the right.
   *
   *  If the joined parameter is set to true, adjacent edges are joined before the extension is applied.
   *  A the join points, the extension is created similar to what the sizing function does. 
   *
   *  Note: the output is given as an out parameter since because of the include hierarchy we can't use
   *  Region as a return value directly.
   *
   *  Merged semantics applies.
   */
  void extended (Region &output, coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i, bool join = false) const;

  /**
   *  @brief Returns edges (point-like) representing the start part of the edges
   *
   *  The length of the part can be choosen by length or a fraction of the original length.
   *  If length and fraction are 0, a point at the beginning of the edge will be created.
   *  If length is non-zero and fraction is 0, a segment in the direction of the edge 
   *  with the given length is created, even if the length is larger than the original
   *  edge. 
   *
   *  If fraction is given and length is 0, the segment will have a length which is the specified
   *  fraction of the original edge.
   *  If both values are given, the resulting edge will have a length which is the maximum of
   *  both the fixed length and the length derived from the fraction.
   *
   *  Length and fraction can be negative in which case the resulting segment will point
   *  in the opposite direction of the original edge.
   *
   *  Merged semantics applies.
   */
  Edges start_segments (length_type length, double fraction) const;

  /**
   *  @brief Returns edges (point-like) representing the end part of the edges
   *
   *  This method behaves similar to \start_segments but creates segments at the end of
   *  the edges.
   *
   *  Merged semantics applies.
   */
  Edges end_segments (length_type length, double fraction) const;

  /**
   *  @brief Returns edges (point-like) representing the center of the edges
   *
   *  This method behaves similar to \start_segments but creates segments at the centers of
   *  the edges.
   *
   *  Merged semantics applies.
   */
  Edges centers (length_type length, double fraction) const;

  /**
   *  @brief Boolean AND operator
   *
   *  This operation returns the parts of the edges which coincide with edges from "other"
   *  After this operation the edges are not necessarily merged.
   */
  Edges operator& (const Edges &other) const
  {
    return boolean (&other, And);
  }

  /**
   *  @brief In-place boolean AND operator
   */
  Edges &operator&= (const Edges &other)
  {
    inplace_boolean (&other, And);
    return *this;
  }

  /**
   *  @brief Boolean AND operator with a region 
   *
   *  This operation returns the parts of the edges which are inside the given region.
   *  Edges on the borders of the polygons are included in the edge set.
   *  As a side effect, the edges are made non-intersecting by introducing cut points where
   *  edges intersect.
   */
  Edges operator& (const Region &other) const
  {
    Edges d (*this);
    d &= other;
    return d;
  }

  /**
   *  @brief In-place boolean AND operator with a region
   */
  Edges &operator&= (const Region &other)
  {
    edge_region_op (other, false /*inside*/, true /*include borders*/);
    return *this;
  }

  /**
   *  @brief Boolean NOT operator
   *
   *  This operation returns the parts of the edges which do not coincide with edges from "other"
   *  After this operation the edges are not necessarily merged.
   */
  Edges operator- (const Edges &other) const
  {
    return boolean (&other, Not);
  }

  /**
   *  @brief In-place boolean NOT operator
   */
  Edges &operator-= (const Edges &other)
  {
    inplace_boolean (&other, Not);
    return *this;
  }

  /**
   *  @brief Boolean NOT operator with a region 
   *
   *  This operation returns the parts of the edges which are outside the given region.
   *  Edges on the borders of the polygons are removed from the edge set.
   *  As a side effect, the edges are made non-intersecting by introducing cut points where
   *  edges intersect.
   */
  Edges operator- (const Region &other) const
  {
    Edges d (*this);
    d -= other;
    return d;
  }

  /**
   *  @brief In-place boolean NOT operator with a region
   */
  Edges &operator-= (const Region &other)
  {
    edge_region_op (other, true /*outside*/, true /*include borders*/);
    return *this;
  }

  /**
   *  @brief Boolean XOR operator
   *
   *  This operation returns the parts of the edges which do not coincide with edges from "other"
   *  and vice versa.
   *  After this operation the edges are not necessarily merged.
   */
  Edges operator^ (const Edges &other) const
  {
    return boolean (&other, Xor);
  }

  /**
   *  @brief In-place boolean XOR operator
   */
  Edges &operator^= (const Edges &other)
  {
    inplace_boolean (&other, Xor);
    return *this;
  }

  /**
   *  @brief Joining of edge sets
   *
   *  This method will combine the edges from "other" with the egdes of "this".
   *  After this operation the edges are not necessarily merged.
   */
  Edges operator+ (const Edges &other) const
  {
    Edges d (*this);
    d += other;
    return d;
  }

  /**
   *  @brief In-place joining of edge sets
   */
  Edges &operator+= (const Edges &other);

  /**
   *  @brief Boolean OR operator
   *
   *  This method will combine the edges from "other" with the egdes of "this".
   *  After this operation the edges are usually merged.
   */
  Edges operator| (const Edges &other) const
  {
    return boolean (&other, Or);
  }

  /**
   *  @brief In-place boolean OR operator
   */
  Edges &operator|= (const Edges &other)
  {
    inplace_boolean (&other, Or);
    return *this;
  }

  /**
   *  @brief Select the edges inside the given region
   *  
   *  This method will select the edges inside the given region.
   *  Edges on the border of the region won't be selected.
   *  As a side effect, the edges are made non-intersecting by introducing cut points where
   *  edges intersect.
   */
  Edges &select_inside_part (const Region &other)
  {
    edge_region_op (other, false /*inside*/, false /*don't include borders*/);
    return *this;
  }

  /**
   *  @brief Returns the edges inside the given region
   *
   *  This is an out-of-place version of "select_inside_part".
   */
  Edges inside_part (const Region &other) const
  {
    Edges d (*this);
    d.select_inside_part (other);
    return d;
  }

  /**
   *  @brief Select the edge parts outside of the given region
   *  
   *  This method will select the edge parts outside of the given region.
   *  Edges on the border of the region won't be selected.
   *  As a side effect, the edges are made non-intersecting by introducing cut points where
   *  edges intersect.
   */
  Edges &select_outside_part (const Region &other)
  {
    edge_region_op (other, true /*outside*/, false /*don't include borders*/);
    return *this;
  }

  /**
   *  @brief Returns the edge parts outside of the given region
   *
   *  This is an out-of-place version of "select_outside_part".
   */
  Edges outside_part (const Region &other) const
  {
    Edges d (*this);
    d.select_outside_part (other);
    return d;
  }

  /**
   *  @brief Selects all edges of this edge set which overlap or touch with polygons from the region
   *
   *  Merged semantics applies. If merged semantics is chosen, the connected edge parts will be 
   *  selected as a whole.
   */
  Edges &select_interacting (const Region &other);

  /**
   *  @brief Returns all edges of this edge set which overlap or touch with polygons from the region
   *
   *  This method is an out-of-place version of select_interacting.
   */
  Edges selected_interacting (const Region &other) const
  {
    Edges d (*this);
    d.select_interacting (other);
    return d;
  }

  /**
   *  @brief Selects all edges of this edge set which do not overlap or touch with polygons from the region
   *
   *  Merged semantics applies. If merged semantics is chosen, the connected edge parts will be 
   *  selected as a whole.
   */
  Edges &select_not_interacting (const Region &other);

  /**
   *  @brief Returns all edges of this edge set which do not overlap or touch with polygons from the region
   *
   *  This method is an out-of-place version of select_not_interacting.
   */
  Edges selected_not_interacting (const Region &other) const
  {
    Edges d (*this);
    d.select_not_interacting (other);
    return d;
  }

  /**
   *  @brief Selects all edges of this edge set which overlap or touch with edges from the other edge set
   *
   *  Merged semantics applies. If merged semantics is chosen, the connected edge parts will be 
   *  selected as a whole.
   */
  Edges &select_interacting (const Edges &other);

  /**
   *  @brief Returns all edges of this edge set which overlap or touch with edges from the other edge set
   *
   *  This method is an out-of-place version of select_interacting.
   */
  Edges selected_interacting (const Edges &other) const
  {
    Edges d (*this);
    d.select_interacting (other);
    return d;
  }

  /**
   *  @brief Selects all edges of this edge set which do not overlap or touch with edges from the other edge set
   *
   *  Merged semantics applies. If merged semantics is chosen, the connected edge parts will be 
   *  selected as a whole.
   */
  Edges &select_not_interacting (const Edges &other);

  /**
   *  @brief Returns all edges of this edge set which do not overlap or touch with edges from the other edge set
   *
   *  This method is an out-of-place version of select_not_interacting.
   */
  Edges selected_not_interacting (const Edges &other) const
  {
    Edges d (*this);
    d.select_not_interacting (other);
    return d;
  }

  /**
   *  @brief Merge the edge set
   *
   *  This method merges the edges of the edge set if they are not merged already.
   *  It returns a reference to this edge set.
   *  Edges are merged by joining them if one edge is a continuation of another.
   *  An out-of-place merge version is "merged".
   */
  Edges &merge ()
  {
    if (! is_merged ()) {
      inplace_boolean (0, Or);
    }
    return *this;
  }

  /*
   *  @brief Returns the merged edge set
   *
   *  This is the out-of-place merge. It returns a new edge set but does not modify 
   *  the edge set it is called on. An in-place version is "merge".
   */
  Edges merged () const
  {
    return boolean (0, Or);
  }

  /**
   *  @brief Applies a width check and returns EdgePairs which correspond to violation markers
   *
   *  The width check will create a edge pairs if the width of the area between the 
   *  edges is less than the specified threshold d. Without "whole_edges", the parts of
   *  the edges are returned which violate the condition. If "whole_edges" is true, the 
   *  result will contain the complete edges participating in the result.
   *
   *  "Width" refers to the space between the "inside" sides of the edges.
   *
   *  The metrics parameter specifies which metrics to use. "Euclidian", "Square" and "Projected"
   *  metrics are available.
   *
   *  ignore_angle allows specification of a maximum angle the edges can have to not participate
   *  in the check. By choosing 90 degree, edges having an angle of 90 degree and larger are not checked,
   *  but acute corners are for example. 
   *
   *  With min_projection and max_projection it is possible to specify how edges must be related 
   *  to each other. If the length of the projection of either edge on the other is >= min_projection
   *  or < max_projection, the edges are considered for the check.
   *
   *  The order of the edges in the resulting edge pairs is undefined.
   *
   *  Merged semantics applies.
   */
  EdgePairs width_check (db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return run_check (db::WidthRelation, 0, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  /**
   *  @brief Applies a space check and returns EdgePairs which correspond to violation markers
   *
   *  "Space" refers to the space between the "outside" sides of the edges.
   *
   *  For the parameters see \width_check. The space check reports edges for which the space is
   *  less than the specified threshold d.
   *
   *  Merged semantics applies.
   */
  EdgePairs space_check (db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return run_check (db::SpaceRelation, 0, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  /**
   *  @brief Applies an enclosing check and returns EdgePairs which correspond to violation markers
   *
   *  The check will return true for edges from this edge set and the other edge set, where the other edge
   *  is located on the "inside" side of the edge from this edge set, the orientation is parallel 
   *  and the distance is less than the specified threshold d.
   *
   *  The first edges of the edge pairs will be the ones from "this", the second edges will be those of "other".
   *
   *  For the other parameters see \width_check.
   *
   *  Merged semantics applies.
   */
  EdgePairs enclosing_check (const Edges &other, db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return run_check (db::OverlapRelation, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  /**
   *  @brief Applies an overlap check and returns EdgePairs which correspond to violation markers
   *
   *  The check will return true for edges from this edge set and the other edge set, where the other edge
   *  is located on the "inside" side of the edge from this edge set, the orientation is anti-parallel 
   *  and the distance is less than the specified threshold d.
   *
   *  The first edges of the edge pairs will be the ones from "this", the second edges will be those of "other".
   *
   *  For the other parameters see \width_check.
   *
   *  Merged semantics applies.
   */
  EdgePairs overlap_check (const Edges &other, db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return run_check (db::WidthRelation, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  /**
   *  @brief Applies an separation check and returns EdgePairs which correspond to violation markers
   *
   *  The check will return true for edges from this edge set and the other edge set, where the other edge
   *  is located on the "outside" side of the edge from this edge set, the orientation is anti-parallel 
   *  and the distance is less than the specified threshold d.
   *
   *  The first edges of the edge pairs will be the ones from "this", the second edges will be those of "other".
   *
   *  For the other parameters see \width_check.
   *
   *  Merged semantics applies.
   */
  EdgePairs separation_check (const Edges &other, db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return run_check (db::SpaceRelation, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  /**
   *  @brief Applies a inside check and returns EdgePairs which correspond to violation markers
   *
   *  The check will return true for edges from this edge set and the other edge set, where the other edge
   *  is located on the "outide" side of the edge from this edge set, the orientation is parallel 
   *  and the distance is less than the specified threshold d.
   *
   *  The first edges of the edge pairs will be the ones from "this", the second edges will be those of "other".
   *
   *  For the other parameters see \width_check.
   *
   *  Merged semantics applies.
   */
  EdgePairs inside_check (const Edges &other, db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return run_check (db::InsideRelation, &other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection);
  }

  /**
   *  @brief Returns the nth edge 
   *
   *  This method will force the edges to be inside the edge vector and will invalidate any iterator.
   *  If that happens, the method may be costly. 
   *  The iterator should be used whenever possible.
   */
  const db::Edge *nth (size_t n) const
  {
    ensure_valid_edges ();
    return n < m_edges.size () ? &m_edges.get_layer<db::Edge, db::unstable_layer_tag> ().begin () [n] : 0;
  }

  /**
   *  @brief Returns true, if the edge set has valid edges stored within itself
   */
  bool has_valid_edges () const
  {
    return m_iter.at_end ();
  }

  /**
   *  @brief Ensures the edge collection has valid edges
   *
   *  This method is const since it has const semantics.
   */
  void ensure_valid_edges () const;

  /**
   *  @brief Ensures the edge collection has valid merged edges
   *
   *  It will make sure that begin_merged will deliver an 
   *  iterator to an edge with a unique memory location.
   */
  void ensure_valid_merged_edges () const;

  /**
   *  @brief Equality
   */
  bool operator== (const db::Edges &other) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const db::Edges &other) const
  {
    return !operator== (other);
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const db::Edges &other) const;

private:
  typedef db::layer<db::Edge, db::unstable_layer_tag> edge_layer_type;
  typedef edge_layer_type::iterator edge_iterator_type;

  bool m_is_merged;
  bool m_merged_semantics;
  mutable db::Shapes m_edges;
  mutable db::Shapes m_merged_edges;
  mutable db::Box m_bbox;
  mutable bool m_bbox_valid;
  mutable bool m_merged_edges_valid;
  mutable db::RecursiveShapeIterator m_iter;
  db::ICplxTrans m_iter_trans;
  bool m_report_progress;
  std::string m_progress_desc;

  void init ();
  void invalidate_cache ();
  void set_valid_edges ();
  void ensure_bbox_valid () const;
  void ensure_merged_edges_valid () const;
  EdgePairs run_check (db::edge_relation_type rel, const Edges *other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const;
  void inplace_boolean (const Edges *other, BoolOp op);
  Edges boolean (const Edges *other, BoolOp op) const;
  void edge_region_op (const Region &other, bool outside, bool include_borders);
};

} // namespace db

namespace tl 
{
  /**
   *  @brief The type traits for the edges type
   */
  template <>
  struct type_traits <db::Edges> : public type_traits<void> 
  {
    typedef true_tag supports_extractor;
    typedef true_tag supports_to_string;
    typedef true_tag has_less_operator;
    typedef true_tag has_equal_operator;
  };

}

#endif

