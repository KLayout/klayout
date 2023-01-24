
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

#ifndef HDR_dbEdges
#define HDR_dbEdges

#include "dbCommon.h"
#include "dbEdgesDelegate.h"
#include "dbRecursiveShapeIterator.h"
#include "dbCellVariants.h"
#include "dbShapeCollection.h"
#include "dbGenericShapeIterator.h"

#include <list>

namespace db {

class EdgeFilterBase;
class MutableEdges;
class EmptyEdges;
class DeepShapeStore;

typedef generic_shape_iterator<db::Edge> EdgesIterator;
typedef addressable_shape_delivery<Edge> AddressableEdgeDelivery;

class Edges;

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
  : public db::ShapeCollection
{
public:
  typedef db::Coord coord_type;
  typedef db::coord_traits<db::Coord> coord_traits;
  typedef db::Edge edge_type;
  typedef db::Vector vector_type;
  typedef db::Point point_type;
  typedef db::Box box_type;
  typedef coord_traits::distance_type length_type;
  typedef coord_traits::distance_type distance_type;
  typedef EdgesIterator const_iterator;

  /**
   *  @brief Default constructor
   *
   *  Creates an empty edge set.
   */
  Edges ();

  /**
   *  @brief Destructor
   */
  ~Edges ();

  /**
   *  @brief Constructor from a delegate
   *
   *  The region will take ownership of the delegate.
   */
  Edges (EdgesDelegate *delegate);

  /**
   *  @brief Copy constructor
   */
  Edges (const Edges &other);

  /**
   *  @brief Assignment
   */
  Edges &operator= (const Edges &other);

  /**
   *  @brief Constructor from a box
   *
   *  Creates an edge set representing the contour of the box
   */
  explicit Edges (const db::Box &s)
    : mp_delegate (0)
  {
    insert (s);
  }

  /**
   *  @brief Constructor from a simple polygon
   *
   *  Creates an edge set representing the contour of the polygon
   */
  explicit Edges (const db::SimplePolygon &s)
    : mp_delegate (0)
  {
    insert (s);
  }

  /**
   *  @brief Constructor from a polygon
   *
   *  Creates an edge set representing the contour of the polygon
   */
  explicit Edges (const db::Polygon &s)
    : mp_delegate (0)
  {
    insert (s);
  }

  /**
   *  @brief Constructor from a path
   *
   *  Creates an edge set representing the contour of the path
   */
  explicit Edges (const db::Path &s)
    : mp_delegate (0)
  {
    insert (s);
  }

  /**
   *  @brief Constructor from an edge
   *
   *  Creates an edge set representing the single edge
   */
  explicit Edges (const db::Edge &s)
    : mp_delegate (0)
  {
    insert (s);
  }

  /**
   *  @brief Sequence constructor
   *
   *  Creates an edge set from a sequence of objects. The objects can be boxes,
   *  polygons, paths, edges or shapes. This version accepts iterators of the begin ... end
   *  style.
   */
  template <class Iter>
  explicit Edges (const Iter &b, const Iter &e)
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
   *  Creates an edge set from a recursive shape iterator. This allows feeding an edge set
   *  from a hierarchy of cells.
   */
  explicit Edges (const RecursiveShapeIterator &si, bool as_edges = true);

  /**
   *  @brief Constructor from a RecursiveShapeIterator with a transformation
   *
   *  Creates an edge set from a recursive shape iterator. This allows feeding an edge set
   *  from a hierarchy of cells. The transformation is useful to scale to a specific
   *  DBU for example.
   */
  explicit Edges (const RecursiveShapeIterator &si, const db::ICplxTrans &trans, bool as_edges = true, bool merged_semantics = true);

  /**
   *  @brief Constructor from a RecursiveShapeIterator providing a deep representation
   *
   *  This version will create a hierarchical edge collection. The DeepShapeStore needs to be provided
   *  during the lifetime of the collection and acts as a heap for optimized data.
   */
  explicit Edges (const RecursiveShapeIterator &si, DeepShapeStore &dss, bool as_edges = true);

  /**
   *  @brief Constructor from a RecursiveShapeIterator providing a deep representation with transformation
   */
  explicit Edges (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans, bool as_edges = true, bool merged_semantics = true);

  /**
   *  @brief Implementation of the ShapeCollection interface
   */
  ShapeCollectionDelegateBase *get_delegate () const
  {
    return mp_delegate;
  }

  /**
   *  @brief Gets the underlying delegate object
   */
  const EdgesDelegate *delegate () const
  {
    return mp_delegate;
  }

  /**
   *  @brief Gets the underlying delegate object
   */
  EdgesDelegate *delegate ()
  {
    return mp_delegate;
  }

  /**
   *  @brief Takes the underlying delegate object
   */
  EdgesDelegate *take_delegate ()
  {
    EdgesDelegate *delegate = mp_delegate;
    mp_delegate = 0;
    return delegate;
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
   *  @brief Iterator of the edge set
   *
   *  The iterator delivers the edges of the edge set.
   *  It follows the at_end semantics.
   */
  const_iterator begin () const
  {
    return EdgesIterator (mp_delegate->begin ());
  }

  /**
   *  @brief Returns the merged edges if merge semantics applies
   *
   *  If merge semantics is not enabled, this iterator delivers the individual edges.
   */
  const_iterator begin_merged () const
  {
    return EdgesIterator (mp_delegate->begin_merged ());
  }

  /**
   *  @brief Delivers a RecursiveShapeIterator pointing to the edges plus the necessary transformation
   */
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const
  {
    return mp_delegate->begin_iter ();
  }

  /**
   *  @brief Delivers a RecursiveShapeIterator pointing to the merged edges plus the necessary transformation
   */
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_merged_iter () const
  {
    return mp_delegate->begin_merged_iter ();
  }

  /**
   *  @brief Inserts the given shape (working object) into the edge set
   */
  template <class Sh>
  void insert (const Sh &shape);

  /**
   *  @brief Insert a shape reference into the edge set
   */
  void insert (const db::Shape &shape);

  /**
   *  @brief Insert a transformed shape into the edge set
   */
  template <class T>
  void insert (const db::Shape &shape, const T &trans);

  /**
   *  @brief Returns true if the edge set is empty
   */
  bool empty () const
  {
    return mp_delegate->empty ();
  }

  /**
   *  @brief Returns the number of (flat) edges in the edge set
   */
  size_t count () const
  {
    return mp_delegate->count ();
  }

  /**
   *  @brief Returns the number of (hierarchical) edges in the edge set
   */
  size_t hier_count () const
  {
    return mp_delegate->hier_count ();
  }

  /**
   *  @brief Returns a string representing the edge set
   *
   *  nmax specifies how many edges are included (set to std::numeric_limits<size_t>::max() for "all".
   */
  std::string to_string (size_t nmax = 10) const
  {
    return mp_delegate->to_string (nmax);
  }

  /**
   *  @brief Clears the edge set
   */
  void clear ();

  /**
   *  @brief Reserve memory for the given number of edges
   */
  void reserve (size_t n);

  /**
   *  @brief Sets the merged-semantics flag
   *
   *  If merged semantics is enabled (the default), colinear edges will be considered
   *  as single edges.
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
   *  @brief Returns true if the edge set is merged
   */
  bool is_merged () const
  {
    return mp_delegate->is_merged ();
  }

  /**
   *  @brief Returns the total length of the edges
   *  Merged semantics applies. In merged semantics, the length is the correct total length of the edges.
   *  Without merged semantics, overlapping parts are counted twice.
   *
   *  If a box is given, the computation is restricted to that box.
   *  Edges coincident with the box edges are counted only if the form outer edges at the box edge.
   */
  length_type length (const db::Box &box = db::Box ()) const
  {
    return mp_delegate->length (box);
  }

  /**
   *  @brief Returns the bounding box of the edge set
   */
  Box bbox () const
  {
    return mp_delegate->bbox ();
  }

  /**
   *  @brief Filters the edges
   *
   *  This method will keep all edges for which the filter returns true.
   *  Merged semantics applies. In merged semantics, the filter will run over
   *  all merged edges.
   */
  Edges &filter (const EdgeFilterBase &filter)
  {
    set_delegate (mp_delegate->filter_in_place (filter));
    return *this;
  }

  /**
   *  @brief Returns the filtered edges
   *
   *  This method will return a new region with only those edges which
   *  conform to the filter criterion.
   */
  Edges filtered (const EdgeFilterBase &filter) const
  {
    return Edges (mp_delegate->filtered (filter));
  }

  /**
   *  @brief Processes the (merged) edges
   *
   *  This method will keep all edges which the processor returns.
   *  The processing filter can apply modifications too. These modifications will be
   *  kept in the output edge collection.
   *
   *  Merged semantics applies. In merged semantics, the filter will run over
   *  all merged edges.
   */
  Edges &process (const EdgeProcessorBase &filter)
  {
    set_delegate (mp_delegate->process_in_place (filter));
    return *this;
  }

  /**
   *  @brief Returns the processed edges
   *
   *  This method will keep all edges which the processor returns.
   *  The processing filter can apply modifications too. These modifications will be
   *  kept in the output edge collection.
   *
   *  Merged semantics applies. In merged semantics, the filter will run over
   *  all merged edges.
   *
   *  This method will return a new edge collection with the modified and filtered edges.
   */
  Edges processed (const EdgeProcessorBase &filter) const
  {
    return Edges (mp_delegate->processed (filter));
  }

  /**
   *  @brief Processes the edges into polygons
   *
   *  This method will run the processor over all edges and return a region
   *  with the outputs of the processor.
   *
   *  Merged semantics applies. In merged semantics, the filter will run over
   *  all merged edges.
   */
  void processed (Region &output, const EdgeToPolygonProcessorBase &filter) const;

  /**
   *  @brief Processes the edges into edge pairs
   *
   *  This method will run the processor over all edges and return an edge pair collection
   *  with the outputs of the processor.
   *
   *  Merged semantics applies. In merged semantics, the filter will run over
   *  all merged edges.
   */
  EdgePairs processed (const EdgeToEdgePairProcessorBase &filter) const
  {
    return EdgePairs (mp_delegate->processed_to_edge_pairs (filter));
  }

  /**
   *  @brief Applies a width check and returns EdgePairs which correspond to violation markers
   *
   *  The width check will create a edge pairs if the width of the area between the 
   *  edges is less than the specified threshold d.
   *
   *  "options" specifies various options to configure the check and it's output.
   *
   *  The order of the edges in the resulting edge pairs is undefined.
   *
   *  Merged semantics applies.
   */
  EdgePairs width_check (db::Coord d, const db::EdgesCheckOptions &options = db::EdgesCheckOptions ()) const
  {
    return EdgePairs (mp_delegate->width_check (d, options));
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
  EdgePairs space_check (db::Coord d, const db::EdgesCheckOptions &options = db::EdgesCheckOptions ()) const
  {
    return EdgePairs (mp_delegate->space_check (d, options));
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
  EdgePairs enclosing_check (const Edges &other, db::Coord d, const db::EdgesCheckOptions &options = db::EdgesCheckOptions ()) const
  {
    return EdgePairs (mp_delegate->enclosing_check (other, d, options));
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
  EdgePairs overlap_check (const Edges &other, db::Coord d, const db::EdgesCheckOptions &options = db::EdgesCheckOptions ()) const
  {
    return EdgePairs (mp_delegate->overlap_check (other, d, options));
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
  EdgePairs separation_check (const Edges &other, db::Coord d, const db::EdgesCheckOptions &options = db::EdgesCheckOptions ()) const
  {
    return EdgePairs (mp_delegate->separation_check (other, d, options));
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
  EdgePairs inside_check (const Edges &other, db::Coord d, const db::EdgesCheckOptions &options = db::EdgesCheckOptions ()) const
  {
    return EdgePairs (mp_delegate->inside_check (other, d, options));
  }

  /**
   *  @brief Transforms the edge set
   */
  template <class T>
  Edges &transform (const T &trans);

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
   *  @brief Swaps with the other edge set
   */
  void swap (db::Edges &other)
  {
    std::swap (other.mp_delegate, mp_delegate);
  }

  /**
   *  @brief Merges the edge set
   *
   *  This method merges the edges of the edge set if they are not merged already.
   *  It returns a reference to this edge set.
   *  An out-of-place merge version is "merged".
   */
  Edges &merge ()
  {
    set_delegate (mp_delegate->merged_in_place ());
    return *this;
  }

  /**
   *  @brief Returns the merged edge set
   *
   *  This is the out-of-place merge. It returns a new edge set but does not modify
   *  the edge set it is called on. An in-place version is "merge".
   */
  Edges merged () const
  {
    return Edges (mp_delegate->merged ());
  }

  /**
   *  @brief Boolean AND operator
   */
  Edges operator& (const Edges &other) const
  {
    return Edges (mp_delegate->and_with (other));
  }

  /**
   *  @brief In-place boolean AND operator
   *
   *  This method does not necessarily merge the edge set. To ensure the edge set
   *  is merged, call merge afterwards.
   */
  Edges &operator&= (const Edges &other)
  {
    set_delegate (mp_delegate->and_with (other));
    return *this;
  }

  /**
   *  @brief Boolean AND operator with a region
   */
  Edges operator& (const Region &other) const
  {
    return Edges (mp_delegate->and_with (other));
  }

  /**
   *  @brief In-place boolean AND operator with a region
   *
   *  This method will keep all edges inside the given region.
   */
  Edges &operator&= (const Region &other)
  {
    set_delegate (mp_delegate->and_with (other));
    return *this;
  }

  /**
   *  @brief Boolean NOT operator
   */
  Edges operator- (const Edges &other) const
  {
    return Edges (mp_delegate->not_with (other));
  }

  /**
   *  @brief In-place boolean NOT operator
   *
   *  This method does not necessarily merge the edge set. To ensure the edge set
   *  is merged, call merge afterwards.
   */
  Edges &operator-= (const Edges &other)
  {
    set_delegate (mp_delegate->not_with (other));
    return *this;
  }

  /**
   *  @brief Boolean NOT operator with a region
   */
  Edges operator- (const Region &other) const
  {
    return Edges (mp_delegate->not_with (other));
  }

  /**
   *  @brief In-place boolean NOT operator with a region
   *
   *  This method will remove all edges inside the given region.
   */
  Edges &operator-= (const Region &other)
  {
    set_delegate (mp_delegate->not_with (other));
    return *this;
  }

  /**
   *  @brief Boolean AND/NOT operator in the same operation
   */
  std::pair<Edges, Edges> andnot (const Edges &other) const
  {
    std::pair<db::EdgesDelegate *, db::EdgesDelegate *> p = mp_delegate->andnot_with (other);
    return std::pair<Edges, Edges> (Edges (p.first), Edges (p.second));
  }

  /**
   *  @brief Boolean AND/NOT operator with a region in the same operation
   */
  std::pair<Edges, Edges> andnot (const Region &other) const
  {
    std::pair<db::EdgesDelegate *, db::EdgesDelegate *> p = mp_delegate->andnot_with (other);
    return std::pair<Edges, Edges> (Edges (p.first), Edges (p.second));
  }

  /**
   *  @brief Boolean XOR operator
   */
  Edges operator^ (const Edges &other) const
  {
    return Edges (mp_delegate->xor_with (other));
  }

  /**
   *  @brief In-place boolean XOR operator
   *
   *  This method does not necessarily merge the edge set. To ensure the edge set
   *  is merged, call merge afterwards.
   */
  Edges &operator^= (const Edges &other)
  {
    set_delegate (mp_delegate->xor_with (other));
    return *this;
  }

  /**
   *  @brief Boolean OR operator
   *
   *  This method merges the edges of both edge sets.
   */
  Edges operator| (const Edges &other) const
  {
    return Edges (mp_delegate->or_with (other));
  }

  /**
   *  @brief In-place boolean OR operator
   */
  Edges &operator|= (const Edges &other)
  {
    set_delegate (mp_delegate->or_with (other));
    return *this;
  }

  /**
   *  @brief Joining of edge set
   *
   *  This method joins the edge sets but does not merge them afterwards.
   */
  Edges operator+ (const Edges &other) const
  {
    return Edges (mp_delegate->add (other));
  }

  /**
   *  @brief In-place edge set joining
   */
  Edges &operator+= (const Edges &other)
  {
    set_delegate (mp_delegate->add_in_place (other));
    return *this;
  }

  /**
   *  @brief Intersections with other edges
   *  Intersections are similar to "AND", but will also report
   *  non-parallel intersections between crossing edges.
   */
  Edges intersections (const Edges &other) const
  {
    return Edges (mp_delegate->intersections (other));
  }

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
   *  The length of the part can be chosen by length or a fraction of the original length.
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
   *  @brief Select the edges inside the given region
   *  
   *  This method will select the edges inside the given region.
   *  Edges on the border of the region won't be selected.
   *  As a side effect, the edges are made non-intersecting by introducing cut points where
   *  edges intersect.
   */
  Edges &select_inside_part (const Region &other)
  {
    set_delegate (mp_delegate->inside_part (other));
    return *this;
  }

  /**
   *  @brief Returns the edges inside the given region
   *
   *  This is an out-of-place version of "select_inside_part".
   */
  Edges inside_part (const Region &other) const
  {
    return Edges (mp_delegate->inside_part (other));
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
    set_delegate (mp_delegate->outside_part (other));
    return *this;
  }

  /**
   *  @brief Returns the edge parts outside of the given region
   *
   *  This is an out-of-place version of "select_outside_part".
   */
  Edges outside_part (const Region &other) const
  {
    return Edges (mp_delegate->outside_part (other));
  }

  /**
   *  @brief Returns the eges inside the given region and the ones outside the region.
   *
   *  This method combined both inside_part and outside_part.
   */
  std::pair<Edges, Edges> inside_outside_part (const Region &other) const
  {
    std::pair<db::EdgesDelegate *, db::EdgesDelegate *> p = mp_delegate->inside_outside_part_pair (other);
    return std::pair<Edges, Edges> (Edges (p.first), Edges (p.second));
  }

  /**
   *  @brief Selects all polygons of the other region set which overlap or touch edges from this edge set
   *
   *  Merged semantics applies for the other region. Merged polygons will be selected from the other region
   *  if merged semantics is enabled.
   */
  void pull_interacting (Region &output, const Region &other) const;

  /**
   *  @brief Selects all edges of the other edge set which overlap or touch edges from this edge set
   *
   *  Merged semantics applies. If merged semantics is chosen, the connected edge parts will be
   *  selected as a whole from other.
   */
  Edges pull_interacting (const Edges &other) const
  {
    return Edges (mp_delegate->pull_interacting (other));
  }

  /**
   *  @brief Selects all edges of this edge set which overlap or touch with polygons from the region
   *
   *  Merged semantics applies. If merged semantics is chosen, the connected edge parts will be 
   *  selected as a whole.
   */
  Edges &select_interacting (const Region &other)
  {
    set_delegate (mp_delegate->selected_interacting (other));
    return *this;
  }

  /**
   *  @brief Returns all edges of this edge set which overlap or touch with polygons from the region
   *
   *  This method is an out-of-place version of select_interacting.
   */
  Edges selected_interacting (const Region &other) const
  {
    return Edges (mp_delegate->selected_interacting (other));
  }

  /**
   *  @brief Selects all edges of this edge set which do not overlap or touch with polygons from the region
   *
   *  Merged semantics applies. If merged semantics is chosen, the connected edge parts will be 
   *  selected as a whole.
   */
  Edges &select_not_interacting (const Region &other)
  {
    set_delegate (mp_delegate->selected_not_interacting (other));
    return *this;
  }

  /**
   *  @brief Returns all edges of this edge set which do not overlap or touch with polygons from the region
   *
   *  This method is an out-of-place version of select_not_interacting.
   */
  Edges selected_not_interacting (const Region &other) const
  {
    return Edges (mp_delegate->selected_not_interacting (other));
  }

  /**
   *  @brief Returns all edges of this edge set which do not overlap or touch with polygons from the region together with the ones that do not
   */
  std::pair<Edges, Edges> selected_interacting_differential (const Region &other) const
  {
    std::pair<db::EdgesDelegate *, db::EdgesDelegate *> p = mp_delegate->selected_interacting_pair (other);
    return std::pair<Edges, Edges> (Edges (p.first), Edges (p.second));
  }

  /**
   *  @brief Selects all edges of this edge collection which are completely outside polygons from the region
   *
   *  Merged semantics applies.
   */
  Edges &select_outside (const Region &other)
  {
    set_delegate (mp_delegate->selected_outside (other));
    return *this;
  }

  /**
   *  @brief Selects all edges of this edge collection which are not completely outside polygons from the region
   *
   *  Merged semantics applies.
   */
  Edges &select_not_outside (const Region &other)
  {
    set_delegate (mp_delegate->selected_not_outside (other));
    return *this;
  }

  /**
   *  @brief Returns all edges of this edge collection which are completely outside polygons from the region
   *
   *  This method is an out-of-place version of select_outside.
   *
   *  Merged semantics applies.
   */
  Edges selected_outside (const Region &other) const
  {
    return Edges (mp_delegate->selected_outside (other));
  }

  /**
   *  @brief Returns all edges of this edge collection which are not completely outside polygons from the region
   *
   *  This method is an out-of-place version of select_not_outside.
   *
   *  Merged semantics applies.
   */
  Edges selected_not_outside (const Region &other) const
  {
    return Edges (mp_delegate->selected_not_outside (other));
  }

  /**
   *  @brief Returns all edges of this which are completely outside polygons from the region and the opposite ones at the same time
   *
   *  This method is equivalent to calling selected_outside and selected_not_outside, but faster.
   *
   *  Merged semantics applies.
   */
  std::pair<Edges, Edges> selected_outside_differential (const Region &other) const
  {
    std::pair<db::EdgesDelegate *, db::EdgesDelegate *> p = mp_delegate->selected_outside_pair (other);
    return std::pair<Edges, Edges> (Edges (p.first), Edges (p.second));
  }

  /**
   *  @brief Selects all edges of this edge collection which are completely inside polygons from the region
   *
   *  Merged semantics applies.
   */
  Edges &select_inside (const Region &other)
  {
    set_delegate (mp_delegate->selected_inside (other));
    return *this;
  }

  /**
   *  @brief Selects all edges of this edge collection which are not completely inside polygons from the region
   *
   *  Merged semantics applies.
   */
  Edges &select_not_inside (const Region &other)
  {
    set_delegate (mp_delegate->selected_not_inside (other));
    return *this;
  }

  /**
   *  @brief Returns all edges of this which are completely inside polygons from the region
   *
   *  This method is an out-of-place version of select_inside.
   *
   *  Merged semantics applies.
   */
  Edges selected_inside (const Region &other) const
  {
    return Edges (mp_delegate->selected_inside (other));
  }

  /**
   *  @brief Returns all edges of this which are not completely inside polygons from the region
   *
   *  This method is an out-of-place version of select_not_inside.
   *
   *  Merged semantics applies.
   */
  Edges selected_not_inside (const Region &other) const
  {
    return Edges (mp_delegate->selected_not_inside (other));
  }

  /**
   *  @brief Returns all edges of this which are completely inside polygons from the region and the opposite ones at the same time
   *
   *  This method is equivalent to calling selected_inside and selected_not_inside, but faster.
   *
   *  Merged semantics applies.
   */
  std::pair<Edges, Edges> selected_inside_differential (const Region &other) const
  {
    std::pair<db::EdgesDelegate *, db::EdgesDelegate *> p = mp_delegate->selected_inside_pair (other);
    return std::pair<Edges, Edges> (Edges (p.first), Edges (p.second));
  }

  /**
   *  @brief Selects all edges of this edge collection which are completely outside edges from the other edge collection
   *
   *  Merged semantics applies.
   */
  Edges &select_outside (const Edges &other)
  {
    set_delegate (mp_delegate->selected_outside (other));
    return *this;
  }

  /**
   *  @brief Selects all edges of this edge collection which are not completely outside edges from the other edge collection
   *
   *  Merged semantics applies.
   */
  Edges &select_not_outside (const Edges &other)
  {
    set_delegate (mp_delegate->selected_not_outside (other));
    return *this;
  }

  /**
   *  @brief Returns all edges of this edge collection which are completely outside edges from the other edge collection
   *
   *  This method is an out-of-place version of select_outside.
   *
   *  Merged semantics applies.
   */
  Edges selected_outside (const Edges &other) const
  {
    return Edges (mp_delegate->selected_outside (other));
  }

  /**
   *  @brief Returns all edges of this edge collection which are not completely outside edges from the other edge collection
   *
   *  This method is an out-of-place version of select_not_outside.
   *
   *  Merged semantics applies.
   */
  Edges selected_not_outside (const Edges &other) const
  {
    return Edges (mp_delegate->selected_not_outside (other));
  }

  /**
   *  @brief Returns all edges of this which are completely outside edges from the other edge collection and the opposite ones at the same time
   *
   *  This method is equivalent to calling selected_outside and selected_not_outside, but faster.
   *
   *  Merged semantics applies.
   */
  std::pair<Edges, Edges> selected_outside_differential (const Edges &other) const
  {
    std::pair<db::EdgesDelegate *, db::EdgesDelegate *> p = mp_delegate->selected_outside_pair (other);
    return std::pair<Edges, Edges> (Edges (p.first), Edges (p.second));
  }

  /**
   *  @brief Selects all edges of this edge collection which are completely inside edges from the other edge collection
   *
   *  Merged semantics applies.
   */
  Edges &select_inside (const Edges &other)
  {
    set_delegate (mp_delegate->selected_inside (other));
    return *this;
  }

  /**
   *  @brief Selects all edges of this edge collection which are not completely inside edges from the other edge collection
   *
   *  Merged semantics applies.
   */
  Edges &select_not_inside (const Edges &other)
  {
    set_delegate (mp_delegate->selected_not_inside (other));
    return *this;
  }

  /**
   *  @brief Returns all edges of this which are completely inside edgess from the other edge collection
   *
   *  This method is an out-of-place version of select_inside.
   *
   *  Merged semantics applies.
   */
  Edges selected_inside (const Edges &other) const
  {
    return Edges (mp_delegate->selected_inside (other));
  }

  /**
   *  @brief Returns all edges of this which are not completely inside edges from the other edge collection
   *
   *  This method is an out-of-place version of select_not_inside.
   *
   *  Merged semantics applies.
   */
  Edges selected_not_inside (const Edges &other) const
  {
    return Edges (mp_delegate->selected_not_inside (other));
  }

  /**
   *  @brief Returns all edges of this which are completely inside edges from the other edge collection and the opposite ones at the same time
   *
   *  This method is equivalent to calling selected_inside and selected_not_inside, but faster.
   *
   *  Merged semantics applies.
   */
  std::pair<Edges, Edges> selected_inside_differential (const Edges &other) const
  {
    std::pair<db::EdgesDelegate *, db::EdgesDelegate *> p = mp_delegate->selected_inside_pair (other);
    return std::pair<Edges, Edges> (Edges (p.first), Edges (p.second));
  }

  /**
   *  @brief Selects all edges of this edge set which overlap or touch with edges from the other edge set
   *
   *  Merged semantics applies. If merged semantics is chosen, the connected edge parts will be 
   *  selected as a whole.
   */
  Edges &select_interacting (const Edges &other)
  {
    set_delegate (mp_delegate->selected_interacting (other));
    return *this;
  }

  /**
   *  @brief Returns all edges of this edge set which overlap or touch with edges from the other edge set
   *
   *  This method is an out-of-place version of select_interacting.
   */
  Edges selected_interacting (const Edges &other) const
  {
    return Edges (mp_delegate->selected_interacting (other));
  }

  /**
   *  @brief Returns all edges of this edge set which do not overlap or touch with edges from the other edge set together with the ones that do not
   */
  std::pair<Edges, Edges> selected_interacting_differential (const Edges &other) const
  {
    std::pair<db::EdgesDelegate *, db::EdgesDelegate *> p = mp_delegate->selected_interacting_pair (other);
    return std::pair<Edges, Edges> (Edges (p.first), Edges (p.second));
  }

  /**
   *  @brief Selects all edges of this edge set which do not overlap or touch with edges from the other edge set
   *
   *  Merged semantics applies. If merged semantics is chosen, the connected edge parts will be 
   *  selected as a whole.
   */
  Edges &select_not_interacting (const Edges &other)
  {
    set_delegate (mp_delegate->selected_not_interacting (other));
    return *this;
  }

  /**
   *  @brief Returns all edges of this edge set which do not overlap or touch with edges from the other edge set
   *
   *  This method is an out-of-place version of select_not_interacting.
   */
  Edges selected_not_interacting (const Edges &other) const
  {
    return Edges (mp_delegate->selected_not_interacting (other));
  }

  /**
   *  @brief Returns all edges which are in the other edge set
   *
   *  This method will return all edges which are part of another edge set.
   *  The match is done exactly.
   *  The "invert" flag can be used to invert the sense, i.e. with
   *  "invert" set to true, this method will return all edges not
   *  in the other edge set.
   *
   *  Merged semantics applies.
   */
  Edges in (const Edges &other, bool invert = false) const
  {
    return Edges (mp_delegate->in (other, invert));
  }

  /**
   *  @brief Returns all edges which are in the other edge set and which are not.
   *
   *  This method is equivalent to calling in with invert = false and true, but more efficient.
   *
   *  Merged semantics applies.
   */
  std::pair<Edges, Edges> in_and_out (const Edges &other) const
  {
    std::pair<db::EdgesDelegate *, db::EdgesDelegate *> p = mp_delegate->in_and_out (other);
    return std::pair<Edges, Edges> (Edges (p.first), Edges (p.second));
  }

  /**
   *  @brief Returns the nth edge
   *
   *  This operation is available only for flat regions - i.e. such for which "has_valid_edges" is true.
   */
  const db::Edge *nth (size_t n) const
  {
    return mp_delegate->nth (n);
  }

  /**
   *  @brief Forces flattening of the edge collection
   *
   *  This method will turn any edge collection into a flat one.
   */
  void flatten ();

  /**
   *  @brief Returns true, if the edge set has valid edges stored within itself
   *
   *  If the region has valid edges, it is permissable to use the edge's addresses
   *  from the iterator. Furthermore, the random access operator nth() is available.
   */
  bool has_valid_edges () const
  {
    return mp_delegate->has_valid_edges ();
  }

  /**
   *  @brief Returns an addressable delivery for edges
   *
   *  This object allows accessing the edges by address, even if they
   *  are not delivered from a container. The magic is a heap object
   *  inside the delivery object. Hence, the deliver object must persist
   *  as long as the addresses are required.
   */
  AddressableEdgeDelivery addressable_edges () const
  {
    return AddressableEdgeDelivery (begin ());
  }

  /**
   *  @brief Returns true, if the edge set has valid merged edges stored within itself
   *
   *  If the region has valid merged edges, it is permissable to use the edge's addresses
   *  from the merged edge iterator. Furthermore, the random access operator nth() is available.
   */
  bool has_valid_merged_edges () const
  {
    return mp_delegate->has_valid_merged_edges ();
  }

  /**
   *  @brief Returns an addressable delivery for merged polygons
   */
  AddressableEdgeDelivery addressable_merged_edges () const
  {
    return AddressableEdgeDelivery (begin_merged ());
  }

  /**
   *  @brief Gets the internal iterator
   *
   *  This method is intended for users who know what they are doing
   */
  const db::RecursiveShapeIterator &iter () const;

  /**
   *  @brief Gets the property repository
   *
   *  Use this object to decode property IDs.
   */
  const db::PropertiesRepository &properties_repository () const;

  /**
   *  @brief Gets the property repository
   *
   *  Use this object to decode and encode property IDs.
   */
  db::PropertiesRepository &properties_repository ();

  /**
   *  @brief Equality
   */
  bool operator== (const db::Edges &other) const
  {
    return mp_delegate->equals (other);
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const db::Edges &other) const
  {
    return ! mp_delegate->equals (other);
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const db::Edges &other) const
  {
    return mp_delegate->less (other);
  }

  /**
   *  @brief Inserts the edge collection into the given layout, cell and layer
   *  If the edge collection is a hierarchical region, the hierarchy is copied into the
   *  layout's hierarchy.
   */
  void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
  {
    return mp_delegate->insert_into (layout, into_cell, into_layer);
  }

private:
  friend class EdgePairs;
  friend class Texts;

  EdgesDelegate *mp_delegate;

  void set_delegate (EdgesDelegate *delegate, bool keep_attributes = true);
  MutableEdges *mutable_edges();
};

} // namespace db

namespace tl
{
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::Edges &b);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::Edges &b);
}

#endif
