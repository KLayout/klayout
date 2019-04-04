
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


#ifndef HDR_dbRegion
#define HDR_dbRegion

#include "dbCommon.h"
#include "dbRegionDelegate.h"
#include "dbRecursiveShapeIterator.h"
#include "dbPolygonGenerators.h"
#include "dbCellVariants.h"

#include "gsiObject.h"

#include <list>

namespace db {

class EdgeFilterBase;
class FlatRegion;
class EmptyRegion;
class DeepShapeStore;
class TransformationReducer;

/**
 *  @brief A region iterator
 *
 *  The iterator delivers the polygons of the region
 */
class DB_PUBLIC RegionIterator
{
public:
  typedef RegionIteratorDelegate::value_type value_type;
  typedef const value_type &reference;
  typedef const value_type *pointer;
  typedef std::forward_iterator_tag iterator_category;
  typedef void difference_type;

  /**
   *  @brief Default constructor
   */
  RegionIterator ()
    : mp_delegate (0)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Constructor from a delegate
   *  The iterator will take ownership over the delegate
   */
  RegionIterator (RegionIteratorDelegate *delegate)
    : mp_delegate (delegate)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Destructor
   */
  ~RegionIterator ()
  {
    delete mp_delegate;
    mp_delegate = 0;
  }

  /**
   *  @brief Copy constructor and assignment
   */
  RegionIterator (const RegionIterator &other)
    : mp_delegate (0)
  {
    operator= (other);
  }

  /**
   *  @brief Assignment
   */
  RegionIterator &operator= (const RegionIterator &other)
  {
    if (this != &other) {
      delete mp_delegate;
      mp_delegate = other.mp_delegate ? other.mp_delegate->clone () : 0;
    }
    return *this;
  }

  /**
   *  @Returns true, if the iterator is at the end
   */
  bool at_end () const
  {
    return mp_delegate == 0 || mp_delegate->at_end ();
  }

  /**
   *  @brief Increment
   */
  RegionIterator &operator++ ()
  {
    if (mp_delegate) {
      mp_delegate->increment ();
    }
    return *this;
  }

  /**
   *  @brief Access
   */
  reference operator* () const
  {
    const value_type *value = operator-> ();
    tl_assert (value != 0);
    return *value;
  }

  /**
   *  @brief Access
   */
  pointer operator-> () const
  {
    return mp_delegate ? mp_delegate->get () : 0;
  }

private:
  RegionIteratorDelegate *mp_delegate;
};

/**
 *  @brief A helper class allowing delivery of addressable polygons
 *
 *  In some applications (i.e. box scanner), polygons need to be taken
 *  by address. The region cannot always deliver adressable polygons.
 *  This class help providing this ability by keeping a temporary copy
 *  if required.
 */

class DB_PUBLIC AddressablePolygonDelivery
{
public:
  AddressablePolygonDelivery ()
    : m_iter (), m_valid (false)
  {
    //  .. nothing yet ..
  }

  AddressablePolygonDelivery (const RegionIterator &iter, bool valid)
    : m_iter (iter), m_valid (valid)
  {
    if (! m_valid && ! m_iter.at_end ()) {
      m_heap.push_back (*m_iter);
    }
  }

  bool at_end () const
  {
    return m_iter.at_end ();
  }

  AddressablePolygonDelivery &operator++ ()
  {
    ++m_iter;
    if (! m_valid && ! m_iter.at_end ()) {
      m_heap.push_back (*m_iter);
    }
    return *this;
  }

  const db::Polygon *operator-> () const
  {
    if (m_valid) {
      return m_iter.operator-> ();
    } else {
      return &m_heap.back ();
    }
  }

private:
  RegionIterator m_iter;
  bool m_valid;
  std::list<db::Polygon> m_heap;
};

/**
 *  @brief A region
 *
 *  A region basically is a set of polygons. It supports a variety of operations, i.e.
 *  boolean operations with other regions, sizing etc.
 *
 *  Regions can have different states. Specifically a region can be merged (no overlapping
 *  polygons are present, touching polygons are merged, self-intersections of polygons are
 *  removed) or non-merged (polygons may overlap or polygons may be self-intersecting). In
 *  merged state, the wrap count at every point is either zero or 1, in non-merged state
 *  it can be every value.
 *
 *  Polygons inside the region may contain holes if the region is merged.
 */
class DB_PUBLIC Region
  : public gsi::ObjectBase
{
public:
  typedef db::Coord coord_type;
  typedef db::coord_traits<db::Coord> coord_traits;
  typedef db::Polygon polygon_type;
  typedef db::Vector vector_type;
  typedef db::Point point_type;
  typedef db::Box box_type;
  typedef coord_traits::distance_type distance_type;
  typedef coord_traits::perimeter_type perimeter_type;
  typedef coord_traits::area_type area_type;
  typedef RegionIterator const_iterator;

  /**
   *  @brief Default constructor
   *
   *  Creates an empty region.
   */
  Region ();

  /**
   *  @brief Destructor
   */
  ~Region ();

  /**
   *  @brief Constructor from a delegate
   *
   *  The region will take ownership of the delegate.
   */
  Region (RegionDelegate *delegate);

  /**
   *  @brief Copy constructor
   */
  Region (const Region &other);

  /**
   *  @brief Assignment
   */
  Region &operator= (const Region &other);

  /**
   *  @brief Constructor from a box
   */
  explicit Region (const db::Box &s)
    : mp_delegate (0)
  {
    insert (s);
  }

  /**
   *  @brief Constructor from a polygon
   */
  explicit Region (const db::Polygon &s)
    : mp_delegate (0)
  {
    insert (s);
  }

  /**
   *  @brief Constructor from a simple polygon
   */
  explicit Region (const db::SimplePolygon &s)
    : mp_delegate (0)
  {
    insert (s);
  }

  /**
   *  @brief Constructor from a path
   */
  explicit Region (const db::Path &s)
    : mp_delegate (0)
  {
    insert (s);
  }

  /**
   *  @brief Sequence constructor
   *
   *  Creates a region from a sequence of objects. The objects can be boxes,
   *  polygons, paths or shapes. This version accepts iterators of the begin ... end
   *  style.
   */
  template <class Iter>
  explicit Region (const Iter &b, const Iter &e)
    : mp_delegate (0)
  {
    reserve (e - b);
    for (Iter i = b; i != e; ++i) {
      insert (*i);
    }
  }

  /**
   *  @brief Constructor from a RecursiveShapeIterator
   *
   *  Creates a region from a recursive shape iterator. This allows feeding a region
   *  from a hierarchy of cells.
   */
  explicit Region (const RecursiveShapeIterator &si);

  /**
   *  @brief Constructor from a RecursiveShapeIterator with a transformation
   *
   *  Creates a region from a recursive shape iterator. This allows feeding a region
   *  from a hierarchy of cells. The transformation is useful to scale to a specific
   *  DBU for example.
   */
  explicit Region (const RecursiveShapeIterator &si, const db::ICplxTrans &trans, bool merged_semantics = true);

  /**
   *  @brief Constructor from a RecursiveShapeIterator providing a deep representation
   *
   *  This version will create a hierarchical region. The DeepShapeStore needs to be provided
   *  during the lifetime of the region and acts as a heap for optimized data.
   *
   *  "area_ratio" and "max_vertex_count" are optimization parameters for the
   *  shape splitting algorithm.
   */
  explicit Region (const RecursiveShapeIterator &si, DeepShapeStore &dss, double area_ratio = 3.0, size_t max_vertex_count = 16);

  /**
   *  @brief Constructor from a RecursiveShapeIterator providing a deep representation with transformation
   */
  explicit Region (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans, bool merged_semantics = true, double area_ratio = 3.0, size_t max_vertex_count = 16);

  /**
   *  @brief Gets the underlying delegate object
   */
  RegionDelegate *delegate () const
  {
    return mp_delegate;
  }

  /**
   *  @brief Sets the base verbosity
   *
   *  Setting this value will make timing measurements appear at least at
   *  the given verbosity level and more detailed timing at the given level
   *  plus 10. The default level is 30.
   */
  void set_base_verbosity (int vb)
  {
    mp_delegate->set_base_verbosity (vb);
  }

  /**
   *  @brief Gets the base verbosity
   */
  unsigned int base_verbosity () const
  {
    return mp_delegate->base_verbosity ();
  }

  /**
   *  @brief Enable progress reporting
   *
   *  @param progress_text The description text of the progress object
   */
  void enable_progress (const std::string &desc = std::string ())
  {
    mp_delegate->enable_progress (desc);
  }

  /**
   *  @brief Disable progress reporting
   */
  void disable_progress ()
  {
    mp_delegate->disable_progress ();
  }

  /**
   *  @brief Iterator of the region
   *
   *  The iterator delivers the polygons of the region.
   *  It follows the at_end semantics.
   */
  const_iterator begin () const
  {
    return RegionIterator (mp_delegate->begin ());
  }

  /**
   *  @brief Returns the merged polygons if merge semantics applies
   *
   *  If merge semantics is not enabled, this iterator delivers the individual polygons.
   */
  const_iterator begin_merged () const
  {
    return RegionIterator (mp_delegate->begin_merged ());
  }

  /**
   *  @brief Delivers a RecursiveShapeIterator pointing to the polygons plus the necessary transformation
   */
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const
  {
    return mp_delegate->begin_iter ();
  }

  /**
   *  @brief Delivers a RecursiveShapeIterator pointing to the merged polygons plus the necessary transformation
   */
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_merged_iter () const
  {
    return mp_delegate->begin_merged_iter ();
  }

  /**
   *  @brief Inserts the given shape (working object) into the region
   */
  template <class Sh>
  void insert (const Sh &shape);

  /**
   *  @brief Insert a shape reference into the region
   */
  void insert (const db::Shape &shape);

  /**
   *  @brief Insert a transformed shape into the region
   */
  template <class T>
  void insert (const db::Shape &shape, const T &trans);

  /**
   *  @brief Returns true if the region is empty
   */
  bool empty () const
  {
    return mp_delegate->empty ();
  }

  /**
   *  @brief Returns the number of polygons in the region
   */
  size_t size () const
  {
    return mp_delegate->size ();
  }

  /**
   *  @brief Returns a string representing the region
   *
   *  nmax specifies how many polygons are included (set to std::numeric_limits<size_t>::max() for "all".
   */
  std::string to_string (size_t nmax = 10) const
  {
    return mp_delegate->to_string (nmax);
  }

  /**
   *  @brief Clear the region
   */
  void clear ();

  /**
   *  @brief Reserve memory for the given number of polygons
   */
  void reserve (size_t n);

  /**
   *  @brief Sets the minimum-coherence flag
   *
   *  If minimum coherence is set, the merge operations (explicit merge with \merge or
   *  implicit merge through merged_semantics) are performed using minimum coherence mode.
   *  The coherence mode determines how kissing-corner situations are resolved. If
   *  minimum coherence is selected, they are resolved such that multiple polygons are
   *  created which touch at a corner).
   */
  void set_min_coherence (bool f)
  {
    mp_delegate->set_min_coherence (f);
  }

  /**
   *  @brief Gets the minimum coherence flag
   */
  bool min_coherence () const
  {
    return mp_delegate->min_coherence ();
  }

  /**
   *  @brief Sets the merged-semantics flag
   *
   *  If merged semantics is enabled (the default), coherent polygons will be considered
   *  as single regions and artificial edges such as cut-lines will not be considered.
   *  Merged semantics thus is equivalent to considering coherent areas rather than
   *  single polygons.
   */
  void set_merged_semantics (bool f)
  {
    mp_delegate->set_merged_semantics (f);
  }

  /**
   *  @brief Gets the merged-semantics flag
   */
  bool merged_semantics () const
  {
    return mp_delegate->merged_semantics ();
  }

  /**
   *  @brief Enables or disables strict handling
   *
   *  Strict handling means to leave away some optimizations. Specifically the
   *  output of boolean operations will be merged even if one input is empty.
   *  Without strict handling, the operation will be optimized and output
   *  won't be merged.
   *
   *  Strict handling is disabled by default.
   */
  void set_strict_handling (bool f)
  {
    mp_delegate->set_strict_handling (f);
  }

  /**
   *  @brief Gets a valid indicating whether strict handling is enabled
   */
  bool strict_handling () const
  {
    return mp_delegate->strict_handling ();
  }

  /**
   *  @brief Returns true if the region is a single box
   *
   *  If the region is not merged, this method may return false even
   *  if the merged region would be a box.
   */
  bool is_box () const
  {
    return mp_delegate->is_box ();
  }

  /**
   *  @brief Returns true if the region is merged
   */
  bool is_merged () const
  {
    return mp_delegate->is_merged ();
  }

  /**
   *  @brief Returns the area of the region
   *
   *  This method returns the area sum over all polygons.
   *  Merged semantics applies. In merged semantics, the area is the correct area covered by the
   *  polygons. Without merged semantics, overlapping parts are counted twice.
   *
   *  If a box is given, the computation is restricted to that box.
   */
  area_type area (const db::Box &box = db::Box ()) const
  {
    return mp_delegate->area (box);
  }

  /**
   *  @brief Returns the perimeter sum of the region
   *
   *  This method returns the perimeter sum over all polygons.
   *  Merged semantics applies. In merged semantics, the perimeter is the true perimeter.
   *  Without merged semantics, inner edges contribute to the perimeter.
   *
   *  If a box is given, the computation is restricted to that box.
   *  Edges coincident with the box edges are counted only if the form outer edges at the box edge.
   */
  perimeter_type perimeter (const db::Box &box = db::Box ()) const
  {
    return mp_delegate->perimeter (box);
  }

  /**
   *  @brief Returns the bounding box of the region
   */
  Box bbox () const
  {
    return mp_delegate->bbox ();
  }

  /**
   *  @brief Filters the polygons
   *
   *  This method will keep all polygons for which the filter returns true.
   *  Merged semantics applies. In merged semantics, the filter will run over
   *  all merged polygons.
   */
  Region &filter (const PolygonFilterBase &filter)
  {
    set_delegate (mp_delegate->filter_in_place (filter));
    return *this;
  }

  /**
   *  @brief Returns the filtered polygons
   *
   *  This method will return a new region with only those polygons which
   *  conform to the filter criterion.
   */
  Region filtered (const PolygonFilterBase &filter) const
  {
    return Region (mp_delegate->filtered (filter));
  }

  /**
   *  @brief Processes the (merged) polygons
   *
   *  This method will keep all polygons which the processor returns.
   *  The processing filter can apply modifications too. These modifications will be
   *  kept in the output region.
   *
   *  Merged semantics applies. In merged semantics, the filter will run over
   *  all merged polygons.
   */
  Region &process (const PolygonProcessorBase &filter)
  {
    set_delegate (mp_delegate->process_in_place (filter));
    return *this;
  }

  /**
   *  @brief Returns the processed polygons
   *
   *  This method will keep all polygons which the processor returns.
   *  The processing filter can apply modifications too. These modifications will be
   *  kept in the output region.
   *
   *  Merged semantics applies. In merged semantics, the filter will run over
   *  all merged polygons.
   *
   *  This method will return a new region with the modified and filtered polygons.
   */
  Region processed (const PolygonProcessorBase &filter) const
  {
    return Region (mp_delegate->processed (filter));
  }

  /**
   *  @brief Processes the polygons into edges
   *
   *  This method applies a processor returning edges for the polygons.
   *
   *  Merged semantics applies. In merged semantics, the filter will run over
   *  all merged polygons.
   */
  Edges processed (const PolygonToEdgeProcessorBase &filter) const
  {
    return Edges (mp_delegate->processed_to_edges (filter));
  }

  /**
   *  @brief Processes the polygons into edge pairs
   *
   *  This method applies a processor returning edge pairs for the polygons.
   *
   *  Merged semantics applies. In merged semantics, the filter will run over
   *  all merged polygons.
   */
  EdgePairs processed (const PolygonToEdgePairProcessorBase &filter) const
  {
    return EdgePairs (mp_delegate->processed_to_edge_pairs (filter));
  }

  /**
   *  @brief Applies a width check and returns EdgePairs which correspond to violation markers
   *
   *  The width check will create a edge pairs if the width of the area between the
   *  edges is less than the specified threshold d. Without "whole_edges", the parts of
   *  the edges are returned which violate the condition. If "whole_edges" is true, the
   *  result will contain the complete edges participating in the result.
   *
   *  The metrics parameter specifies which metrics to use. "Euclidian", "Square" and "Projected"
   *  metrics are available.
   *
   *  ingore_angle allows specification of a maximum angle that connected edges can have to not participate
   *  in the check. By choosing 90 degree, edges with angles of 90 degree and larger are not checked,
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
    return EdgePairs (mp_delegate->width_check (d, whole_edges, metrics, ignore_angle, min_projection, max_projection));
  }

  /**
   *  @brief Applies a space check and returns EdgePairs which correspond to violation markers
   *
   *  For the parameters see \width_check. The space check reports edges for which the space is
   *  less than the specified threshold d.
   *
   *  Merged semantics applies.
   */
  EdgePairs space_check (db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return EdgePairs (mp_delegate->space_check (d, whole_edges, metrics, ignore_angle, min_projection, max_projection));
  }

  /**
   *  @brief Applies a isolation check (space of polygon vs. other polygons) and returns EdgePairs which correspond to violation markers
   *
   *  For the parameters see \width_check. The space check reports edges for which the notch space is
   *  less than the specified threshold d.
   *
   *  Merged semantics applies.
   */
  EdgePairs isolated_check (db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return EdgePairs (mp_delegate->isolated_check (d, whole_edges, metrics, ignore_angle, min_projection, max_projection));
  }

  /**
   *  @brief Applies a notch check (space in polygon vs. itself) and returns EdgePairs which correspond to violation markers
   *
   *  For the parameters see \width_check. The space check reports edges for which the notch space is
   *  less than the specified threshold d.
   *
   *  Merged semantics applies.
   */
  EdgePairs notch_check (db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return EdgePairs (mp_delegate->notch_check (d, whole_edges, metrics, ignore_angle, min_projection, max_projection));
  }

  /**
   *  @brief Applies a enclosed check and returns EdgePairs which correspond to violation markers
   *
   *  The check will return true, where this region is enclosing the polygons of the other
   *  region by less than the specified threshold d.
   *
   *  The first edges of the edge pairs will be the ones from "this", the second edges will be those of "other".
   *
   *  For the other parameters see \width_check.
   *
   *  Merged semantics applies.
   */
  EdgePairs enclosing_check (const Region &other, db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return EdgePairs (mp_delegate->enclosing_check (other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection));
  }

  /**
   *  @brief Applies a overlap check and returns EdgePairs which correspond to violation markers
   *
   *  The check will return true, where this region overlaps the polygons of the other
   *  region by less than the specified threshold d.
   *
   *  The first edges of the edge pairs will be the ones from "this", the second edges will be those of "other".
   *
   *  For the other parameters see \width_check.
   *
   *  Merged semantics applies.
   */
  EdgePairs overlap_check (const Region &other, db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return EdgePairs (mp_delegate->overlap_check (other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection));
  }

  /**
   *  @brief Applies a separation check and returns EdgePairs which correspond to violation markers
   *
   *  The check will return true, where this region is separated by polygons of the other
   *  region by less than the specified threshold d.
   *
   *  The first edges of the edge pairs will be the ones from "this", the second edges will be those of "other".
   *
   *  For the other parameters see \width_check.
   *
   *  Merged semantics applies.
   */
  EdgePairs separation_check (const Region &other, db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return EdgePairs (mp_delegate->separation_check (other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection));
  }

  /**
   *  @brief Applies a inside check and returns EdgePairs which correspond to violation markers
   *
   *  The check will return true, where this region is inside by less than the threshold d inside the polygons of the other
   *  region.
   *
   *  The first edges of the edge pairs will be the ones from "this", the second edges will be those of "other".
   *
   *  For the other parameters see \width_check.
   *
   *  Merged semantics applies.
   */
  EdgePairs inside_check (const Region &other, db::Coord d, bool whole_edges = false, metrics_type metrics = db::Euclidian, double ignore_angle = 90, distance_type min_projection = 0, distance_type max_projection = std::numeric_limits<distance_type>::max ()) const
  {
    return EdgePairs (mp_delegate->inside_check (other, d, whole_edges, metrics, ignore_angle, min_projection, max_projection));
  }

  /**
   *  @brief Returns an edge set containing all edges of the polygons in this region
   *
   *  Merged semantics applies. In merged semantics, only full, outer edges are delivered.
   */
  Edges edges () const
  {
    return Edges (mp_delegate->edges (0));
  }

  /**
   *  @brief Returns an edge set containing all edges of the polygons in this region
   *
   *  This version allows one to specify a filter by which the edges are filtered before they are
   *  returned.
   *
   *  Merged semantics applies. In merged semantics, only full, outer edges are delivered.
   */
  Edges edges (const EdgeFilterBase &filter) const
  {
    return mp_delegate->edges (&filter);
  }

  /**
   *  @brief Transform the region
   */
  template <class T>
  Region &transform (const T &trans);

  /**
   *  @brief Returns the transformed region
   */
  template <class T>
  Region transformed (const T &trans) const
  {
    Region d (*this);
    d.transform (trans);
    return d;
  }

  /**
   *  @brief Performs an off-grid check on the polygons inside the region
   *
   *  The method returns single-point edge pairs for each vertex found off-grid.
   *  The grid can be specified differently in x and y direction.
   */
  EdgePairs grid_check (db::Coord gx, db::Coord gy) const
  {
    return EdgePairs (mp_delegate->grid_check (gx, gy));
  }

  /**
   *  @brief Performs an angle check
   *
   *  The method returns edge pairs for each connected edges having
   *  an angle between min and max (exclusive max, in degree) or
   *  not between min and max (if inverse is true).
   */
  EdgePairs angle_check (double min, double max, bool inverse) const
  {
    return EdgePairs (mp_delegate->angle_check (min, max, inverse));
  }

  /**
   *  @brief Grid-snaps the region
   *
   *  Snaps the vertices of the polygons to the specified grid.
   *  different grids can be specified int x and y direction.
   */
  void snap (db::Coord gx, db::Coord gy);

  /**
   *  @brief Returns the snapped region
   */
  Region snapped (db::Coord gx, db::Coord gy) const;

  /**
   *  @brief Performs a check for "strange" polygons
   *
   *  This check will return a region with all self-overlapping or
   *  non-orientable parts of polygons.
   *
   *  Naturally this method will ignore the merged_semantics setting.
   */
  Region strange_polygon_check () const;

  /**
   *  @brief Swap with the other region
   */
  void swap (db::Region &other)
  {
    std::swap (other.mp_delegate, mp_delegate);
  }

  /**
   *  @brief Merge the region
   *
   *  This method merges the polygons of the region if they are not merged already.
   *  It returns a reference to this region.
   *  An out-of-place merge version is "merged".
   */
  Region &merge ()
  {
    set_delegate (mp_delegate->merged_in_place ());
    return *this;
  }

  /**
   *  @brief Returns the merged region
   *
   *  This is the out-of-place merge. It returns a new region but does not modify
   *  the region it is called on. An in-place version is "merge".
   */
  Region merged () const
  {
    return Region (mp_delegate->merged ());
  }

  /**
   *  @brief Merge the region with options
   *
   *  This will merge the region and provides some options while doing so.
   *
   *  A result is generated if the wrap count (wc)
   *  of a point is larger than the given min_wrapcount value.
   *  A min_wrapcount of 1 will produce output where at least two polygons overlap.
   *
   *  This method will always execute the merge, even if the region is already merged.
   *
   *  @param min_coherence Set this parameter to true to get minimum polygons (kissing corner problem)
   *  @param min_wrapcount See the description above
   *  @return A reference to this region
   */
  Region &merge (bool min_coherence, unsigned int min_wc = 0)
  {
    set_delegate (mp_delegate->merged_in_place (min_coherence, min_wc));
    return *this;
  }

  /**
   *  @brief Returns the merged region with options
   *
   *  This is the out-of-place version of "merge" with options (see there).
   */
  Region merged (bool min_coherence, unsigned int min_wc = 0) const
  {
    return Region (mp_delegate->merged (min_coherence, min_wc));
  }

  /**
   *  @brief Size the region
   *
   *  This method applies a sizing to the region. Before the sizing is done, the
   *  region is merged if this is not the case already.
   *
   *  The result is a set of polygons which may be overlapping, but are not self-
   *  intersecting.
   *
   *  Merged semantics applies.
   *
   *  @param d The (isotropic) sizing value
   *  @param mode The sizing mode (see EdgeProcessor) for a description of the sizing mode which controls the miter distance.
   *  @return A reference to self
   */
  Region &size (coord_type d, unsigned int mode = 2);

  /**
   *  @brief Anisotropic sizing
   *
   *  This version provides anisotropic sizing by allowing to specify a distance int x and y
   *  direction.
   *
   *  Merged semantics applies.
   *
   *  @param dx The horizontal sizing
   *  @param dy The vertical sizing
   *  @param mode The sizing mode (see EdgeProcessor) for a description of the sizing mode which controls the miter distance.
   */
  Region &size (coord_type dx, coord_type dy, unsigned int mode = 2);

  /**
   *  @brief Returns the sized region
   *
   *  This is an out-of-place version of the size method with isotropic sizing
   *  "merged polygon" semantics applies if merged_polygon_semantics is true (see set_auto_merge).
   *
   *  Merged semantics applies.
   */
  Region sized (coord_type d, unsigned int mode = 2) const;

  /**
   *  @brief Returns the sized region
   *
   *  This is an out-of-place version of the size method with anisotropic sizing
   *  "merged polygon" semantics applies if merged_polygon_semantics is true (see set_auto_merge).
   *
   *  Merged semantics applies.
   */
  Region sized (coord_type dx, coord_type dy, unsigned int mode = 2) const;

  /**
   *  @brief Boolean AND operator
   */
  Region operator& (const Region &other) const
  {
    return Region (mp_delegate->and_with (other));
  }

  /**
   *  @brief In-place boolean AND operator
   *
   *  This method does not necessarily merge the region. To ensure the region
   *  is merged, call merge afterwards.
   */
  Region &operator&= (const Region &other)
  {
    set_delegate (mp_delegate->and_with (other));
    return *this;
  }

  /**
   *  @brief Boolean NOT operator
   */
  Region operator- (const Region &other) const
  {
    return Region (mp_delegate->not_with (other));
  }

  /**
   *  @brief In-place boolean NOT operator
   *
   *  This method does not necessarily merge the region. To ensure the region
   *  is merged, call merge afterwards.
   */
  Region &operator-= (const Region &other)
  {
    set_delegate (mp_delegate->not_with (other));
    return *this;
  }

  /**
   *  @brief Boolean XOR operator
   */
  Region operator^ (const Region &other) const
  {
    return Region (mp_delegate->xor_with (other));
  }

  /**
   *  @brief In-place boolean XOR operator
   *
   *  This method does not necessarily merge the region. To ensure the region
   *  is merged, call merge afterwards.
   */
  Region &operator^= (const Region &other)
  {
    set_delegate (mp_delegate->xor_with (other));
    return *this;
  }

  /**
   *  @brief Boolean OR operator
   *
   *  This method merges the polygons of both regions.
   */
  Region operator| (const Region &other) const
  {
    return Region (mp_delegate->or_with (other));
  }

  /**
   *  @brief In-place boolean OR operator
   */
  Region &operator|= (const Region &other)
  {
    set_delegate (mp_delegate->or_with (other));
    return *this;
  }

  /**
   *  @brief Joining of regions
   *
   *  This method joins the regions but does not merge them afterwards.
   */
  Region operator+ (const Region &other) const
  {
    return Region (mp_delegate->add (other));
  }

  /**
   *  @brief In-place region joining
   */
  Region &operator+= (const Region &other)
  {
    set_delegate (mp_delegate->add_in_place (other));
    return *this;
  }

  /**
   *  @brief Selects all polygons of this region which are completly outside polygons from the other region
   *
   *  Merged semantics applies.
   */
  Region &select_outside (const Region &other)
  {
    set_delegate (mp_delegate->selected_outside (other));
    return *this;
  }

  /**
   *  @brief Selects all polygons of this region which are not completly outside polygons from the other region
   *
   *  Merged semantics applies.
   */
  Region &select_not_outside (const Region &other)
  {
    set_delegate (mp_delegate->selected_not_outside (other));
    return *this;
  }

  /**
   *  @brief Returns all polygons of this which are completly outside polygons from the other region
   *
   *  This method is an out-of-place version of select_outside.
   *
   *  Merged semantics applies.
   */
  Region selected_outside (const Region &other) const
  {
    return Region (mp_delegate->selected_outside (other));
  }

  /**
   *  @brief Returns all polygons of this which are not completly outside polygons from the other region
   *
   *  This method is an out-of-place version of select_not_outside.
   *
   *  Merged semantics applies.
   */
  Region selected_not_outside (const Region &other) const
  {
    return Region (mp_delegate->selected_not_outside (other));
  }

  /**
   *  @brief Selects all polygons of this region which are completly inside polygons from the other region
   *
   *  Merged semantics applies.
   */
  Region &select_inside (const Region &other)
  {
    set_delegate (mp_delegate->selected_inside (other));
    return *this;
  }

  /**
   *  @brief Selects all polygons of this region which are not completly inside polygons from the other region
   *
   *  Merged semantics applies.
   */
  Region &select_not_inside (const Region &other)
  {
    set_delegate (mp_delegate->selected_not_inside (other));
    return *this;
  }

  /**
   *  @brief Returns all polygons of this which are completly inside polygons from the other region
   *
   *  This method is an out-of-place version of select_inside.
   *
   *  Merged semantics applies.
   */
  Region selected_inside (const Region &other) const
  {
    return Region (mp_delegate->selected_inside (other));
  }

  /**
   *  @brief Returns all polygons of this which are not completly inside polygons from the other region
   *
   *  This method is an out-of-place version of select_not_inside.
   *
   *  Merged semantics applies.
   */
  Region selected_not_inside (const Region &other) const
  {
    return Region (mp_delegate->selected_not_inside (other));
  }

  /**
   *  @brief Selects all polygons of this region which overlap or touch polygons from the other region
   *
   *  Merged semantics applies.
   */
  Region &select_interacting (const Region &other)
  {
    set_delegate (mp_delegate->selected_interacting (other));
    return *this;
  }

  /**
   *  @brief Selects all polygons of this region which do not overlap or touch polygons from the other region
   *
   *  Merged semantics applies.
   */
  Region &select_not_interacting (const Region &other)
  {
    set_delegate (mp_delegate->selected_not_interacting (other));
    return *this;
  }

  /**
   *  @brief Returns all polygons of this which overlap or touch polygons from the other region
   *
   *  This method is an out-of-place version of select_interacting.
   *
   *  Merged semantics applies.
   */
  Region selected_interacting (const Region &other) const
  {
    return Region (mp_delegate->selected_interacting (other));
  }

  /**
   *  @brief Returns all polygons of this which do not overlap or touch polygons from the other region
   *
   *  This method is an out-of-place version of select_not_interacting.
   *
   *  Merged semantics applies.
   */
  Region selected_not_interacting (const Region &other) const
  {
    return Region (mp_delegate->selected_not_interacting (other));
  }

  /**
   *  @brief Selects all polygons of this region which overlap or touch edges from the given edge collection
   *
   *  Merged semantics applies to both operators.
   */
  Region &select_interacting (const Edges &other)
  {
    set_delegate (mp_delegate->selected_interacting (other));
    return *this;
  }

  /**
   *  @brief Selects all polygons of this region which do not overlap or touch edges from the edge collection
   *
   *  Merged semantics applies to both operators.
   */
  Region &select_not_interacting (const Edges &other)
  {
    set_delegate (mp_delegate->selected_not_interacting (other));
    return *this;
  }

  /**
   *  @brief Returns all polygons of this which overlap or touch edges from the edge collection
   *
   *  This method is an out-of-place version of select_interacting.
   *
   *  Merged semantics applies to both operators.
   */
  Region selected_interacting (const Edges &other) const
  {
    return Region (mp_delegate->selected_interacting (other));
  }

  /**
   *  @brief Returns all polygons of this which do not overlap or touch polygons from the other region
   *
   *  This method is an out-of-place version of select_not_interacting.
   *
   *  Merged semantics applies to both operators.
   */
  Region selected_not_interacting (const Edges &other) const
  {
    return Region (mp_delegate->selected_not_interacting (other));
  }

  /**
   *  @brief Selects all polygons of this region which overlap polygons from the other region
   *
   *  Merged semantics applies.
   */
  Region &select_overlapping (const Region &other)
  {
    set_delegate (mp_delegate->selected_overlapping (other));
    return *this;
  }

  /**
   *  @brief Selects all polygons of this region which do not overlap polygons from the other region
   *
   *  Merged semantics applies.
   */
  Region &select_not_overlapping (const Region &other)
  {
    set_delegate (mp_delegate->selected_not_overlapping (other));
    return *this;
  }

  /**
   *  @brief Returns all polygons of this which overlap polygons from the other region
   *
   *  This method is an out-of-place version of select_overlapping.
   *
   *  Merged semantics applies.
   */
  Region selected_overlapping (const Region &other) const
  {
    return Region (mp_delegate->selected_overlapping (other));
  }

  /**
   *  @brief Returns all polygons of this which do not overlap polygons from the other region
   *
   *  This method is an out-of-place version of select_not_overlapping.
   *
   *  Merged semantics applies.
   */
  Region selected_not_overlapping (const Region &other) const
  {
    return Region (mp_delegate->selected_not_overlapping (other));
  }

  /**
   *  @brief Returns the holes
   *
   *  This method returns the holes of the polygons.
   *
   *  Merged semantics applies.
   */
  Region holes () const;

  /**
   *  @brief Returns the hulls
   *
   *  This method returns the hulls of the polygons. It does not merge the
   *  polygons before the hulls are derived.
   *
   *  Merged semantics applies.
   */
  Region hulls () const;

  /**
   *  @brief Returns all polygons which are in the other region
   *
   *  This method will return all polygons which are part of another region.
   *  The match is done exactly.
   *  The "invert" flag can be used to invert the sense, i.e. with
   *  "invert" set to true, this method will return all polygons not
   *  in the other region.
   *
   *  Merged semantics applies.
   */
  Region in (const Region &other, bool invert = false) const
  {
    return Region (mp_delegate->in (other, invert));
  }

  /**
   *  @brief Round corners (in-place)
   *
   *  @param rinner The inner radius in DBU units
   *  @param router The outer radius in DBU units
   *  @param n The number of points to use per circle
   */
  void round_corners (double rinner, double router, unsigned int n);

  /**
   *  @brief Returns a new region with rounded corners (out of place)
   */
  Region rounded_corners (double rinner, double router, unsigned int n) const;

  /**
   *  @brief Smoothes the region (in-place)
   */
  void smooth (coord_type d);

  /**
   *  @brief Returns the smoothed region
   *
   *  @param d The smoothing accuracy
   */
  Region smoothed (coord_type d) const;

  /**
   *  @brief Returns the nth polygon
   *
   *  This operation is available only for flat regions - i.e. such for which
   *  "has_valid_polygons" is true.
   */
  const db::Polygon *nth (size_t n) const
  {
    return mp_delegate->nth (n);
  }

  /**
   *  @brief Forces flattening of the region
   *
   *  This method will turn any region into a flat shape collection.
   */
  db::Region &flatten ()
  {
    flat_region ();
    return *this;
  }

  /**
   *  @brief Returns true, if the region has valid polygons stored within itself
   *
   *  If the region has valid polygons, it is permissable to use the polygon's addresses
   *  from the iterator. Furthermore, the random access operator nth() is available.
   */
  bool has_valid_polygons () const
  {
    return mp_delegate->has_valid_polygons ();
  }

  /**
   *  @brief Returns an addressable delivery for polygons
   *
   *  This object allows accessing the polygons by address, even if they
   *  are not delivered from a container. The magic is a heap object
   *  inside the delivery object. Hence, the deliver object must persist
   *  as long as the addresses are required.
   */
  AddressablePolygonDelivery addressable_polygons () const
  {
    return AddressablePolygonDelivery (begin (), has_valid_polygons ());
  }

  /**
   *  @brief Returns true, if the region has valid merged polygons stored within itself
   *
   *  If the region has valid merged polygons, it is permissable to use the polygon's addresses
   *  from the merged polygon iterator. Furthermore, the random access operator nth() is available.
   */
  bool has_valid_merged_polygons () const
  {
    return mp_delegate->has_valid_merged_polygons ();
  }

  /**
   *  @brief Returns an addressable delivery for merged polygons
   */
  AddressablePolygonDelivery addressable_merged_polygons () const
  {
    return AddressablePolygonDelivery (begin_merged (), has_valid_merged_polygons ());
  }

  /**
   *  @brief Gets the internal iterator
   *
   *  This method is intended for users who know what they are doing
   */
  const db::RecursiveShapeIterator &iter () const;

  /**
   *  @brief Equality
   */
  bool operator== (const db::Region &other) const
  {
    return mp_delegate->equals (other);
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const db::Region &other) const
  {
    return ! mp_delegate->equals (other);
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const db::Region &other) const
  {
    return mp_delegate->less (other);
  }

  /**
   *  @brief Inserts the region into the given layout, cell and layer
   *  If the region is a hierarchical region, the hierarchy is copied into the
   *  layout's hierarchy.
   */
  void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
  {
    return mp_delegate->insert_into (layout, into_cell, into_layer);
  }

  /**
   *  @brief Delivers texts as dots (degenerated edges)
   *
   *  "pat" is a text selector. If "as_pattern" is true, this pattern will be
   *  treated as a glob pattern. Otherwise, the text is taken if "pat" is equal to the text..
   */
  db::Edges texts_as_dots (const std::string &pat, bool as_pattern) const;

  /**
   *  @brief Delivers texts as dots (degenerated edges) in a deep edge collection
   *
   *  "pat" is a text selector. If "as_pattern" is true, this pattern will be
   *  treated as a glob pattern. Otherwise, the text is taken if "pat" is equal to the text..
   */
  db::Edges texts_as_dots (const std::string &pat, bool as_pattern, db::DeepShapeStore &store) const;

  /**
   *  @brief Delivers texts as boxes
   *
   *  "pat" is a text selector. If "as_pattern" is true, this pattern will be
   *  treated as a glob pattern. Otherwise, the text is taken if "pat" is equal to the text.
   *  "enl" is the half size of the box (the box is 2*enl wide and 2*enl high).
   */
  db::Region texts_as_boxes (const std::string &pat, bool as_pattern, db::Coord enl) const;

  /**
   *  @brief Delivers texts as boxes in a deep region
   *
   *  "pat" is a text selector. If "as_pattern" is true, this pattern will be
   *  treated as a glob pattern. Otherwise, the text is taken if "pat" is equal to the text.
   *  "enl" is the half size of the box (the box is 2*enl wide and 2*enl high).
   */
  db::Region texts_as_boxes (const std::string &pat, bool as_pattern, db::Coord enl, db::DeepShapeStore &store) const;

private:
  friend class Edges;
  friend class EdgePairs;

  RegionDelegate *mp_delegate;

  void set_delegate (RegionDelegate *delegate, bool keep_attributes = true);
  FlatRegion *flat_region ();
};

/**
 *  @brief A polygon receiver putting the polygons into a Region object
 *
 *  This class implements the PolygonSink interface for the edge processor.
 *  Like EdgeContainer, this receiver collects the objects into an existing Region object.
 */
class DB_PUBLIC RegionPolygonSink
  : public db::PolygonSink
{
public:
  /**
   *  @brief Constructor specifying the region where to store the polygons
   *
   *  If "clear" is set to true, the region will be cleared before the 
   *  inserting of polygons starts. This allows using the region as input and
   *  output for any operation.
   */
  RegionPolygonSink (Region &region, bool clear = false) 
    : PolygonSink (), mp_region (&region), m_clear (clear)
  { }

  /**
   *  @brief Start the sequence
   */
  virtual void start ()
  {
    if (m_clear) {
      mp_region->clear ();
    }
  }

  /**
   *  @brief Implementation of the PolygonSink interface
   */
  virtual void put (const db::Polygon &polygon)
  {
    mp_region->insert (polygon);
  }

private:
  Region *mp_region;
  bool m_clear;
};

} // namespace db

namespace tl 
{
  /**
   *  @brief The type traits for the region type
   */
  template <>
  struct type_traits <db::Region> : public type_traits<void> 
  {
    typedef true_tag supports_extractor;
    typedef true_tag supports_to_string;
    typedef true_tag has_less_operator;
    typedef true_tag has_equal_operator;
  };

}

#endif

