
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


#ifndef HDR_dbEdgePairs
#define HDR_dbEdgePairs

#include "dbEdgePairsDelegate.h"
#include "dbShape.h"
#include "dbRecursiveShapeIterator.h"
#include "dbShapeCollection.h"
#include "dbGenericShapeIterator.h"

#include <list>

namespace db
{

class EdgePairFilterBase;
class MutableEdgePairs;
class EmptyEdgePairs;
class Edges;
class Region;
class DeepShapeStore;
class TransformationReducer;
class EdgeFilterBase;

typedef generic_shape_iterator<EdgePair> EdgePairsIterator;
typedef addressable_shape_delivery<EdgePair> AddressableEdgePairDelivery;

class EdgePairs;

/**
 *  @brief A base class for edge pair filters
 */
class DB_PUBLIC EdgePairFilterBase
{
public:
  EdgePairFilterBase () { }
  virtual ~EdgePairFilterBase () { }

  virtual bool selected (const db::EdgePair &edge_pair) const = 0;
  virtual const TransformationReducer *vars () const = 0;
  virtual bool wants_variants () const = 0;
};

/**
 *  @brief A set of edge pairs
 *
 *  Edge pairs are convenient object describing a DRC violation. Each edge set
 *  consists of two edges whose relationship is to be marked by objects in the edge
 *  set. Depending on the origin of the edge pairs, the first and second edge
 *  may be derived from one specific source, i.e. one region while the other is
 *  derived from another source.
 *
 *  Edge pair sets are created by Region::width_check for example. Edge pair sets
 *  can be converted to polygons or to individual edges.
 */
class DB_PUBLIC EdgePairs
  : public db::ShapeCollection
{
public:
  typedef db::Coord coord_type;
  typedef db::coord_traits<db::Coord> coord_traits;
  typedef db::EdgePair edge_pair_type;
  typedef db::Vector vector_type;
  typedef db::Point point_type;
  typedef db::Box box_type;
  typedef coord_traits::distance_type distance_type;
  typedef EdgePairsIterator const_iterator;

  /**
   *  @brief Default constructor
   *
   *  This constructor creates an empty edge pair set.
   */
  EdgePairs ();
  
  /**
   *  @brief Destructor
   */
  ~EdgePairs ();

  /**
   *  @brief Constructor from a delegate
   *
   *  The region will take ownership of the delegate.
   */
  EdgePairs (EdgePairsDelegate *delegate);

  /**
   *  @brief Copy constructor
   */
  EdgePairs (const EdgePairs &other);

  /**
   *  @brief Assignment
   */
  EdgePairs &operator= (const EdgePairs &other);

  /**
   *  @brief Constructor from an object
   *
   *  Creates an edge pair set representing a single instance of that object
   */
  explicit EdgePairs (const db::EdgePair &s)
    : mp_delegate (0)
  {
    insert (s);
  }

  /**
   *  @brief Constructor from an object
   *
   *  Creates an edge pair set representing a single instance of that object
   */
  explicit EdgePairs (const db::Shape &s)
    : mp_delegate (0)
  {
    insert (s);
  }

  /**
   *  @brief Sequence constructor
   *
   *  Creates an edge set from a sequence of objects. The objects need to be edge pairs.
   *  This version accepts iterators of the begin ... end style.
   */
  template <class Iter>
  explicit EdgePairs (const Iter &b, const Iter &e)
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
   *  Creates an edge pair set from a recursive shape iterator. This allows one to feed an edge pair set
   *  from a hierarchy of cells.
   */
  explicit EdgePairs (const RecursiveShapeIterator &si);

  /**
   *  @brief Constructor from a RecursiveShapeIterator with a transformation
   *
   *  Creates an edge pair set from a recursive shape iterator. This allows one to feed an edge pair set
   *  from a hierarchy of cells. The transformation is useful to scale to a specific
   *  DBU for example.
   */
  explicit EdgePairs (const RecursiveShapeIterator &si, const db::ICplxTrans &trans);

  /**
   *  @brief Constructor from a RecursiveShapeIterator providing a deep representation
   *
   *  This version will create a hierarchical edge pair collection. The DeepShapeStore needs to be provided
   *  during the lifetime of the collection and acts as a heap for optimized data.
   */
  explicit EdgePairs (const RecursiveShapeIterator &si, DeepShapeStore &dss);

  /**
   *  @brief Constructor from a RecursiveShapeIterator providing a deep representation with transformation
   */
  explicit EdgePairs (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans);

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
  const EdgePairsDelegate *delegate () const
  {
    return mp_delegate;
  }

  /**
   *  @brief Gets the underlying delegate object
   */
  EdgePairsDelegate *delegate ()
  {
    return mp_delegate;
  }

  /**
   *  @brief Iterator of the edge pair set
   *
   *  The iterator delivers the edges of the edge pair set.
   *  It follows the at_end semantics.
   */
  const_iterator begin () const
  {
    return EdgePairsIterator (mp_delegate->begin ());
  }

  /**
   *  @brief Delivers a RecursiveShapeIterator pointing to the edge pairs plus the necessary transformation
   */
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const
  {
    return mp_delegate->begin_iter ();
  }

  /**
   *  @brief Inserts the given shape (working object) into the edge pair set
   */
  template <class Sh>
  void insert (const Sh &shape);

  /**
   *  @brief Inserts an edge pair made from two edges
   */
  void insert (const db::Edge &e1, const db::Edge &e2)
  {
    insert (db::EdgePair (e1, e2));
  }

  /**
   *  @brief Insert a shape reference into the edge pair set
   */
  void insert (const db::Shape &shape);

  /**
   *  @brief Insert a transformed shape into the edge pair set
   */
  template <class T>
  void insert (const db::Shape &shape, const T &trans);

  /**
   *  @brief Returns true if the edge pair set is empty
   */
  bool empty () const
  {
    return mp_delegate->empty ();
  }

  /**
   *  @brief Returns the number of (flat) edge pairs in the edge pair set
   */
  size_t count () const
  {
    return mp_delegate->count ();
  }

  /**
   *  @brief Returns the number of (hierarchical) edge pairs in the edge pair set
   */
  size_t hier_count () const
  {
    return mp_delegate->hier_count ();
  }

  /**
   *  @brief Returns a string representing the edge pair set
   *
   *  nmax specifies how many edge pairs are included (set to std::numeric_limits<size_t>::max() for "all".
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
   *  @brief Returns the bounding box of the edge pair set
   */
  Box bbox () const
  {
    return mp_delegate->bbox ();
  }

  /**
   *  @brief Filters the edge pairs
   *
   *  This method will keep all edge pairs for which the filter returns true.
   *  Merged semantics applies. 
   */
  EdgePairs &filter (const EdgePairFilterBase &filter)
  {
    set_delegate (mp_delegate->filter_in_place (filter));
    return *this;
  }

  /**
   *  @brief Returns the filtered edge pairs
   *
   *  This method will return a new region with only those edge pairs which
   *  conform to the filter criterion.
   */
  EdgePairs filtered (const EdgePairFilterBase &filter) const
  {
    return EdgePairs (mp_delegate->filtered (filter));
  }

  /**
   *  @brief Processes the edge pairs into polygons
   *
   *  This method will run the processor over all edge pairs and return a region
   *  with the outputs of the processor.
   */
  void processed (Region &output, const EdgePairToPolygonProcessorBase &filter) const;

  /**
   *  @brief Processes the edge pairs into edges
   *
   *  This method will run the processor over all edge pairs and return a edge collection
   *  with the outputs of the processor.
   */
  void processed (Edges &output, const EdgePairToEdgeProcessorBase &filter) const;

  /**
   *  @brief Transforms the edge pair set
   */
  template <class T>
  EdgePairs &transform (const T &trans);

  /**
   *  @brief Returns the transformed edge pair set
   */
  template <class T>
  EdgePairs transformed (const T &trans) const
  {
    EdgePairs d (*this);
    d.transform (trans);
    return d;
  }

  /**
   *  @brief Swaps with the other edge set
   */
  void swap (db::EdgePairs &other)
  {
    std::swap (other.mp_delegate, mp_delegate);
  }

  /**
   *  @brief Joining of edge pair set
   *
   *  This method joins the edge pair sets.
   */
  EdgePairs operator+ (const EdgePairs &other) const
  {
    return EdgePairs (mp_delegate->add (other));
  }

  /**
   *  @brief In-place edge pair set joining
   */
  EdgePairs &operator+= (const EdgePairs &other)
  {
    set_delegate (mp_delegate->add_in_place (other));
    return *this;
  }

  /**
   *  @brief Returns all edge pairs which are in the other edge pair set
   *
   *  This method will return all edge pairs which are part of another edge pair set.
   *  The match is done exactly.
   *  The "invert" flag can be used to invert the sense, i.e. with
   *  "invert" set to true, this method will return all edge pairs not
   *  in the other edge pair set.
   */
  EdgePairs in (const EdgePairs &other, bool invert = false) const
  {
    return EdgePairs (mp_delegate->in (other, invert));
  }

  /**
   *  @brief Returns the nth edge pair
   *
   *  This operation is available only for flat regions - i.e. such for which
   *  "has_valid_edge_pairs" is true.
   */
  const db::EdgePair *nth (size_t n) const
  {
    return mp_delegate->nth (n);
  }

  /**
   *  @brief Forces flattening of the edge pair collection
   *
   *  This method will turn any edge pair collection into a flat one.
   */
  void flatten ();

  /**
   *  @brief Returns true, if the edge pair set has valid edges stored within itself
   *
   *  If the region has valid edges, it is permissable to use the edge pair's addresses
   *  from the iterator. Furthermore, the random access operator nth() is available.
   */
  bool has_valid_edge_pairs () const
  {
    return mp_delegate->has_valid_edge_pairs ();
  }

  /**
   *  @brief Returns an addressable delivery for edge pairs
   *
   *  This object allows accessing the edge pairs by address, even if they
   *  are not delivered from a container. The magic is a heap object
   *  inside the delivery object. Hence, the deliver object must persist
   *  as long as the addresses are required.
   */
  AddressableEdgePairDelivery addressable_edge_pairs () const
  {
    return AddressableEdgePairDelivery (begin ());
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
  bool operator== (const db::EdgePairs &other) const
  {
    return mp_delegate->equals (other);
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const db::EdgePairs &other) const
  {
    return ! mp_delegate->equals (other);
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const db::EdgePairs &other) const
  {
    return mp_delegate->less (other);
  }

  /**
   *  @brief Converts to polygons
   *
   *  Note: because of the include hierarchy we can't use a direct return value.
   *  
   *  The output container is not cleared by this method but polygons are rather
   *  appended.
   *
   *  The given extension is applied to the edges in parallel and perpendicular direction.
   *  This way a minimum extension is achieved which converts vanishing edge pairs to polygons
   *  with an area.
   */
  void polygons (Region &output, db::Coord e = 0) const;

  /**
   *  @brief Returns individual edges
   *
   *  Note: because of the include hierarchy we can't use a direct return value.
   *  
   *  The output container is not cleared by this method but edges are rather
   *  appended.
   */
  void edges (Edges &output) const;

  /**
   *  @brief Returns the first edges
   *
   *  Note: because of the include hierarchy we can't use a direct return value.
   *
   *  The output container is not cleared by this method but edges are rather
   *  appended.
   */  
  void first_edges (Edges &output) const;

  /**
   *  @brief Returns the second edges
   *
   *  Note: because of the include hierarchy we can't use a direct return value.
   *
   *  The output container is not cleared by this method but edges are rather
   *  appended.
   */  
  void second_edges (Edges &output) const;

  /**
   *  @brief Enable progress reporting
   *
   *  @param progress_text The description text of the progress object
   */
  void enable_progress (const std::string &progress_desc = std::string ())
  {
    mp_delegate->enable_progress (progress_desc);
  }

  /**
   *  @brief Disable progress reporting
   */
  void disable_progress ()
  {
    mp_delegate->disable_progress ();
  }

  /**
   *  @brief Inserts the edge pair collection into the given layout, cell and layer
   *  If the edge pair collection is a hierarchical region, the hierarchy is copied into the
   *  layout's hierarchy.
   */
  void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
  {
    return mp_delegate->insert_into (layout, into_cell, into_layer);
  }

  /**
   *  @brief Inserts the edge pair collection into the given layout, cell and layer as polygons with the given enlargement
   *  If the edge pair collection is a hierarchical region, the hierarchy is copied into the
   *  layout's hierarchy.
   */
  void insert_into_as_polygons (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl) const
  {
    return mp_delegate->insert_into_as_polygons (layout, into_cell, into_layer, enl);
  }

private:
  EdgePairsDelegate *mp_delegate;

  void set_delegate (EdgePairsDelegate *delegate);
  MutableEdgePairs *mutable_edge_pairs ();
};

}

namespace tl
{
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::EdgePairs &b);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::EdgePairs &b);
}

#endif

