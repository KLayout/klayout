
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


#ifndef HDR_dbTexts
#define HDR_dbTexts

#include "dbTextsDelegate.h"
#include "dbShape.h"
#include "dbRecursiveShapeIterator.h"
#include "dbShapeCollection.h"
#include "dbGenericShapeIterator.h"

#include <list>

namespace db
{

class TextFilterBase;
class MutableTexts;
class EmptyTexts;
class Edges;
class Region;
class DeepShapeStore;
class TransformationReducer;

typedef generic_shape_iterator<Text> TextsIterator;
typedef addressable_shape_delivery<Text> AddressableTextDelivery;

class Texts;

/**
 *  @brief A base class for text filters
 */
class DB_PUBLIC TextFilterBase
{
public:
  TextFilterBase () { }
  virtual ~TextFilterBase () { }

  virtual bool selected (const db::Text &text) const = 0;
  virtual const TransformationReducer *vars () const = 0;
  virtual bool wants_variants () const = 0;
};

/**
 *  @brief A set of texts
 *
 *  Texts are convenient objects describing labels (a point and a text).
 *
 *  Text sets are created from a text-delivering recursive shape iterator for example. Text sets
 *  can be converted to polygons (representing a small box around the text's point) or to dot-like
 *  edges representing the point of the text.
 */
class DB_PUBLIC Texts
  : public db::ShapeCollection
{
public:
  typedef db::Coord coord_type;
  typedef db::coord_traits<db::Coord> coord_traits;
  typedef db::Text edge_pair_type;
  typedef db::Vector vector_type;
  typedef db::Point point_type;
  typedef db::Box box_type;
  typedef coord_traits::distance_type distance_type;
  typedef TextsIterator const_iterator;

  /**
   *  @brief Default constructor
   *
   *  This constructor creates an empty text set.
   */
  Texts ();
  
  /**
   *  @brief Destructor
   */
  ~Texts ();

  /**
   *  @brief Constructor from a delegate
   *
   *  The region will take ownership of the delegate.
   */
  Texts (TextsDelegate *delegate);

  /**
   *  @brief Copy constructor
   */
  Texts (const Texts &other);

  /**
   *  @brief Assignment
   */
  Texts &operator= (const Texts &other);

  /**
   *  @brief Constructor from an object
   *
   *  Creates an text set representing a single instance of that object
   */
  explicit Texts (const db::Text &s)
    : mp_delegate (0)
  {
    insert (s);
  }

  /**
   *  @brief Constructor from an object
   *
   *  Creates an text set representing a single instance of that object
   */
  explicit Texts (const db::Shape &s)
    : mp_delegate (0)
  {
    insert (s);
  }

  /**
   *  @brief Sequence constructor
   *
   *  Creates an edge set from a sequence of objects. The objects need to be texts.
   *  This version accepts iterators of the begin ... end style.
   */
  template <class Iter>
  explicit Texts (const Iter &b, const Iter &e)
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
   *  Creates an text set from a recursive shape iterator. This allows one to feed an text set
   *  from a hierarchy of cells.
   */
  explicit Texts (const RecursiveShapeIterator &si);

  /**
   *  @brief Constructor from a RecursiveShapeIterator with a transformation
   *
   *  Creates an text set from a recursive shape iterator. This allows one to feed an text set
   *  from a hierarchy of cells. The transformation is useful to scale to a specific
   *  DBU for example.
   */
  explicit Texts (const RecursiveShapeIterator &si, const db::ICplxTrans &trans);

  /**
   *  @brief Constructor from a RecursiveShapeIterator providing a deep representation
   *
   *  This version will create a hierarchical text collection. The DeepShapeStore needs to be provided
   *  during the lifetime of the collection and acts as a heap for optimized data.
   */
  explicit Texts (const RecursiveShapeIterator &si, DeepShapeStore &dss);

  /**
   *  @brief Constructor from a RecursiveShapeIterator providing a deep representation with transformation
   */
  explicit Texts (const RecursiveShapeIterator &si, DeepShapeStore &dss, const db::ICplxTrans &trans);

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
  const TextsDelegate *delegate () const
  {
    return mp_delegate;
  }

  /**
   *  @brief Gets the underlying delegate object
   */
  TextsDelegate *delegate ()
  {
    return mp_delegate;
  }

  /**
   *  @brief Iterator of the text set
   *
   *  The iterator delivers the edges of the text set.
   *  It follows the at_end semantics.
   */
  const_iterator begin () const
  {
    return TextsIterator (mp_delegate->begin ());
  }

  /**
   *  @brief Delivers a RecursiveShapeIterator pointing to the texts plus the necessary transformation
   */
  std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const
  {
    return mp_delegate->begin_iter ();
  }

  /**
   *  @brief Inserts the given shape (working object) into the text set
   */
  template <class Sh>
  void insert (const Sh &shape);

  /**
   *  @brief Insert a shape reference into the text set
   */
  void insert (const db::Shape &shape);

  /**
   *  @brief Insert a transformed shape into the text set
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
   *  @brief Returns the number of (flat) texts in the text set
   */
  size_t count () const
  {
    return mp_delegate->count ();
  }

  /**
   *  @brief Returns the number of (hierarchical) texts in the text set
   */
  size_t hier_count () const
  {
    return mp_delegate->hier_count ();
  }

  /**
   *  @brief Returns a string representing the text set
   *
   *  nmax specifies how many texts are included (set to std::numeric_limits<size_t>::max() for "all".
   */
  std::string to_string (size_t nmax = 10) const
  {
    return mp_delegate->to_string (nmax);
  }

  /**
   *  @brief Clears the text set
   */
  void clear ();

  /**
   *  @brief Reserve memory for the given number of texts
   */
  void reserve (size_t n);

  /**
   *  @brief Returns the bounding box of the text set
   */
  Box bbox () const
  {
    return mp_delegate->bbox ();
  }

  /**
   *  @brief Filters the texts
   *
   *  This method will keep all texts for which the filter returns true.
   */
  Texts &filter (const TextFilterBase &filter)
  {
    set_delegate (mp_delegate->filter_in_place (filter));
    return *this;
  }

  /**
   *  @brief Returns the filtered texts
   *
   *  This method will return a new text set with only those texts which
   *  conform to the filter criterion.
   */
  Texts filtered (const TextFilterBase &filter) const
  {
    return Texts (mp_delegate->filtered (filter));
  }

  /**
   *  @brief Processes the edges into polygons
   *
   *  This method will run the processor over all edges and return a region
   *  with the outputs of the processor.
   */
  void processed (Region &output, const TextToPolygonProcessorBase &filter) const;

  /**
   *  @brief Selects all polygons of the other region set which include the texts of this text collection
   *
   *  Merged semantics applies for the other region. Merged polygons will be selected from the other region
   *  if merged semantics is enabled.
   */
  void pull_interacting (Region &output, const Region &other) const;

  /**
   *  @brief Selects all texts of this text set which are inside the polygons from the region
   */
  Texts &select_interacting (const Region &other)
  {
    set_delegate (mp_delegate->selected_interacting (other));
    return *this;
  }

  /**
   *  @brief Returns all texts of this text set which are inside the polygons from the region
   *
   *  This method is an out-of-place version of select_interacting.
   */
  Texts selected_interacting (const Region &other) const
  {
    return Texts (mp_delegate->selected_interacting (other));
  }

  /**
   *  @brief Selects all texts of this text set which are not inside the polygons from the region
   */
  Texts &select_not_interacting (const Region &other)
  {
    set_delegate (mp_delegate->selected_not_interacting (other));
    return *this;
  }

  /**
   *  @brief Returns all texts of this text set which are not inside the polygons from the region
   *
   *  This method is an out-of-place version of select_not_interacting.
   */
  Texts selected_not_interacting (const Region &other) const
  {
    return Texts (mp_delegate->selected_not_interacting (other));
  }

  /**
   *  @brief Transforms the text set
   */
  template <class T>
  Texts &transform (const T &trans);

  /**
   *  @brief Returns the transformed text set
   */
  template <class T>
  Texts transformed (const T &trans) const
  {
    Texts d (*this);
    d.transform (trans);
    return d;
  }

  /**
   *  @brief Swaps with the other text set
   */
  void swap (db::Texts &other)
  {
    std::swap (other.mp_delegate, mp_delegate);
  }

  /**
   *  @brief Joining of text set
   *
   *  This method joins the text sets.
   */
  Texts operator+ (const Texts &other) const
  {
    return Texts (mp_delegate->add (other));
  }

  /**
   *  @brief In-place text set joining
   */
  Texts &operator+= (const Texts &other)
  {
    set_delegate (mp_delegate->add_in_place (other));
    return *this;
  }

  /**
   *  @brief Returns all texts which are in the other text set
   *
   *  This method will return all texts which are part of another text set.
   *  The match is done exactly.
   *  The "invert" flag can be used to invert the sense, i.e. with
   *  "invert" set to true, this method will return all texts not
   *  in the other text set.
   */
  Texts in (const Texts &other, bool invert = false) const
  {
    return Texts (mp_delegate->in (other, invert));
  }

  /**
   *  @brief Returns the nth text
   *
   *  This operation is available only for flat regions - i.e. such for which
   *  "has_valid_texts" is true.
   */
  const db::Text *nth (size_t n) const
  {
    return mp_delegate->nth (n);
  }

  /**
   *  @brief Forces flattening of the text collection
   *
   *  This method will turn any edge pair collection into a flat one.
   */
  void flatten ();

  /**
   *  @brief Returns true, if the text set has valid texts stored within itself
   *
   *  If the region has valid texts, it is permissable to use the text's addresses
   *  from the iterator. Furthermore, the random access operator nth() is available.
   */
  bool has_valid_texts () const
  {
    return mp_delegate->has_valid_texts ();
  }

  /**
   *  @brief Returns an addressable delivery for texts
   *
   *  This object allows accessing the texts by address, even if they
   *  are not delivered from a container. The magic is a heap object
   *  inside the delivery object. Hence, the deliver object must persist
   *  as long as the addresses are required.
   */
  AddressableTextDelivery addressable_texts () const
  {
    return AddressableTextDelivery (begin ());
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
  bool operator== (const db::Texts &other) const
  {
    return mp_delegate->equals (other);
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const db::Texts &other) const
  {
    return ! mp_delegate->equals (other);
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const db::Texts &other) const
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
   *  The given extension is applied in all directions rendering a square of 2*e
   *  width and height. The center of the boxes will be the position of the texts.
   */
  void polygons (Region &output, db::Coord e = 1) const;

  /**
   *  @brief Returns individual, dot-like edges
   *
   *  Note: because of the include hierarchy we can't use a direct return value.
   *  
   *  The returned edges will be dot-like (identical points) and represent the
   *  position of the text.
   */
  void edges (Edges &output) const;

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
   *  If the text collection is a hierarchical region, the hierarchy is copied into the
   *  layout's hierarchy.
   */
  void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const
  {
    return mp_delegate->insert_into (layout, into_cell, into_layer);
  }

  /**
   *  @brief Inserts the edge pair collection into the given layout, cell and layer as polygons with the given enlargement
   *  If the text collection is a hierarchical region, the hierarchy is copied into the
   *  layout's hierarchy.
   */
  void insert_into_as_polygons (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl) const
  {
    return mp_delegate->insert_into_as_polygons (layout, into_cell, into_layer, enl);
  }

private:
  TextsDelegate *mp_delegate;

  void set_delegate (TextsDelegate *delegate);
  MutableTexts *mutable_texts();
};

}

namespace tl
{
  template<> DB_PUBLIC bool test_extractor_impl (tl::Extractor &ex, db::Texts &b);
  template<> DB_PUBLIC void extractor_impl (tl::Extractor &ex, db::Texts &b);
}

#endif

