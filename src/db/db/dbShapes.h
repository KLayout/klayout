
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


#ifndef HDR_dbShapes
#define HDR_dbShapes

#include "dbCommon.h"

#include "dbObject.h"
#include "dbStatic.h"
#include "dbManager.h"
#include "dbLayer.h"
#include "dbPropertiesRepository.h"
#include "dbShape.h"
#include "tlVector.h"
#include "tlUtils.h"

namespace db 
{

class Shapes;
class Layout;
class Cell;
template <class Sh, class StableTag> class layer_op;
template <class Obj, class Trans> struct array;
template <class Shape> class object_with_properties;
template <class Coord> class generic_polygon_edge_iterator;
template <class Coord> class generic_repository;
typedef generic_repository<db::Coord> GenericRepository;
class ArrayRepository;

/**
 *  @brief A generic shape iterator
 *
 *  This iterator can iterate any kind of shape from a "shapes" container.
 *  It allows selecting certain kind of shapes. The dereferecing operator
 *  returns a shape proxy object that can be used to access the actual shape.
 *  It can iterator over all shapes and over a region selected.
 *  If the iterator is constructed, it always points to the beginning of the 
 *  list of shapes in the container or the first shape selected by the region.
 *  The iterator is incremented as usual with the ++ operator. The end of the
 *  sequence is tested with the at_end method.
 */
class DB_PUBLIC ShapeIterator
{
public:
  typedef db::Coord coord_type;
  typedef db::polygon<coord_type> polygon_type;
  typedef db::simple_polygon<coord_type> simple_polygon_type;
  typedef db::disp_trans<coord_type> disp_type;
  typedef db::unit_trans<coord_type> unit_trans_type;
  typedef db::polygon_ref<polygon_type, disp_type> polygon_ref_type;
  typedef db::polygon_ref<simple_polygon_type, disp_type> simple_polygon_ref_type;
  typedef db::polygon_ref<polygon_type, unit_trans_type> polygon_ptr_type;
  typedef db::array<polygon_ptr_type, disp_type> polygon_ptr_array_type;
  typedef polygon_ptr_array_type::iterator polygon_ptr_array_iterator_type;
  typedef db::polygon_ref<simple_polygon_type, unit_trans_type> simple_polygon_ptr_type;
  typedef db::array<simple_polygon_ptr_type, disp_type> simple_polygon_ptr_array_type;
  typedef simple_polygon_ptr_array_type::iterator simple_polygon_ptr_array_iterator_type;
  typedef db::path<coord_type> path_type;
  typedef db::path_ref<path_type, disp_type> path_ref_type;
  typedef db::path_ref<path_type, unit_trans_type> path_ptr_type;
  typedef db::array<path_ptr_type, disp_type> path_ptr_array_type;
  typedef path_ptr_array_type::iterator path_ptr_array_iterator_type;
  typedef db::edge<coord_type> edge_type;
  typedef db::edge_pair<coord_type> edge_pair_type;
  typedef db::text<coord_type> text_type;
  typedef db::text_ref<text_type, disp_type> text_ref_type;
  typedef db::text_ref<text_type, unit_trans_type> text_ptr_type;
  typedef db::array<text_ptr_type, disp_type> text_ptr_array_type;
  typedef text_ptr_array_type::iterator text_ptr_array_iterator_type;
  typedef db::box<coord_type> box_type;
  typedef db::array<box_type, unit_trans_type> box_array_type;
  typedef box_array_type::iterator box_array_iterator_type;
  typedef db::box<coord_type, db::coord_traits<coord_type>::short_coord_type> short_box_type;
  typedef db::array<short_box_type, unit_trans_type> short_box_array_type;
  typedef short_box_array_type::iterator short_box_array_iterator_type;
  typedef db::point<coord_type> point_type;
  typedef db::vector<coord_type> vector_type;
  typedef db::user_object<coord_type> user_object_type;
  typedef db::Shape shape_type;
  typedef db::Shapes shapes_type;
  typedef std::set<properties_id_type> property_selector;
  typedef shape_type value_type;
  typedef const shape_type *pointer; 
  typedef shape_type reference;   //  operator* returns a value
  typedef std::forward_iterator_tag iterator_category;
  typedef void difference_type;

  struct NoRegionTag { };
  struct TouchingRegionTag { };
  struct OverlappingRegionTag { };

  enum region_mode
  {
    None,
    Overlapping,
    Touching
  };

  enum object_type 
  {
    Polygon                = 0,
    PolygonRef             = 1,
    PolygonPtrArray        = 2,
    SimplePolygon          = 3,
    SimplePolygonRef       = 4,
    SimplePolygonPtrArray  = 5,
    Edge                   = 6,
    EdgePair               = 7,
    Path                   = 8,
    PathRef                = 9,
    PathPtrArray           = 10,
    Box                    = 11,
    BoxArray               = 12,
    ShortBox               = 13,
    ShortBoxArray          = 14,
    Text                   = 15,
    TextRef                = 16,
    TextPtrArray           = 17,
    Point                  = 18,
    UserObject             = 19,
    Null                   = 20 //  must be last!
  };

  enum flags_type 
  {
    //  These flags are a combination of all bits representing the
    //  object type (Bit 0 for Polygon and so on)
    Polygons          = (1 << Polygon) 
                      | (1 << PolygonRef) 
                      | (1 << PolygonPtrArray) 
                      | (1 << SimplePolygon) 
                      | (1 << SimplePolygonRef) 
                      | (1 << SimplePolygonPtrArray),
    Edges             = (1 << Edge),
    EdgePairs         = (1 << EdgePair),
    Points            = (1 << Point),
    Paths             = (1 << Path)
                      | (1 << PathRef) 
                      | (1 << PathPtrArray),
    Boxes             = (1 << Box)
                      | (1 << BoxArray)
                      | (1 << ShortBox)
                      | (1 << ShortBoxArray),
    Texts             = (1 << Text) 
                      | (1 << TextRef) 
                      | (1 << TextPtrArray),
    Regions           = Polygons | Paths | Boxes,   //  convertible to polygons
    UserObjects       = (1 << UserObject),
    Properties        = (1 << Null),
    All               = (1 << Null) - 1,
    AllWithProperties = (1 << (Null + 1)) - 1,
    Nothing           = 0
  };
    
  /**
   *  @brief The default constructor of the iterator
   */
  ShapeIterator ();

  /**
   *  @brief The copy constructor of the iterator
   */
  ShapeIterator (const ShapeIterator &d);

  /**
   *  @brief The normal iterator
   *
   *  This constructor creates an iterator that iterates over all shapes of
   *  the selected kind. The kind of shape is selected by the "flags" argument
   *  which is an or-ed sum of flags_type constants.
   *
   *  @param shapes The shapes container to iterate over
   *  @param flags The kind of shapes to iterate over (or-ed constants of flags_type)
   */
  ShapeIterator (const shapes_type &shapes, unsigned int flags = All, const property_selector *prop_sel = 0, bool inv_prop_sel = false);

  /**
   *  @brief The region iterator
   *
   *  This constructor creates an iterator that iterates over all shapes of
   *  the selected kind that interact with the given region with their bounding box. 
   *  The kind of shape is selected by the "flags" argument which is an or-ed sum 
   *  of flags_type constants. The region_mode argument can be either Overlapping
   *  or Touching. In the former case all shapes whose bounding box overlaps with
   *  the given region are selected. In the latter case all shapes whose bounding
   *  box touches the given region are selected.
   *
   *  @param shapes The shapes container to iterate over
   *  @param box The region to select
   *  @param mode See above
   *  @param flags The kind of shapes to iterate over (or-ed constants of flags_type)
   *  @param prop_sel The property selection
   *  @param inv_prop_sel True, if shapes not matching the property selection shall be reported
   */
  ShapeIterator (const shapes_type &shapes, const box_type &box, region_mode mode, unsigned int flags = All, const property_selector *prop_sel = 0, bool inv_prop_sel = false);

  /**
   *  @brief The destructor
   */
  ~ShapeIterator ()
  {
    cleanup ();
  }

  /**
   *  @brief Assignment
   */
  ShapeIterator &operator= (const ShapeIterator &d);

  /** 
   *  @brief Access the object 
   * 
   *  This method delivers a proxy object that can be copied but still points
   *  to the original objects. This must be taken under consideration if the
   *  container that is iterated over is destroyed: the shape object will then
   *  become invalid instantly.
   *  Hint: for automation purposes, this function must return a copy, not a reference!
   *  For a reference we have operator->
   */
  shape_type operator* () const
  {
    return m_shape;
  }

  /** 
   *  @brief Access the object (see operator*)
   */
  const shape_type *operator-> () const
  {
    return &m_shape;
  }

  /**
   *  @brief Returns true if we are inside an array
   */
  bool in_array () const
  {
    return m_array_iterator_valid;
  }

  /**
   *  @brief Finishes the array and move to the next shape which might be an array as well
   */
  void finish_array ();

  /**
   *  @brief Access to the array shape
   */
  const shape_type &array () const
  {
    return m_array;
  }

  /**
   *  @brief Increment the iterator
   */
  ShapeIterator &operator++ () 
  {
    advance (1);
    return *this;
  }

  /**
   *  @brief Test if the iterator is at the end
   *
   *  @return true if there are no more elements
   */
  bool at_end () const
  {
    return m_type == Null;
  }

  /**
   *  @brief Gets the quad ID
   *
   *  The quad ID is a unique identifier for the current quad. This can be used to 
   *  detect whether the iterator entered a new quad and optimize the search in that case.
   */
  size_t quad_id () const
  {
    return m_quad_id;
  }

  /**
   *  @brief Gets the quad box
   *
   *  Gets the box the current quad uses. This box may be larger than the actual shape containers
   *  bounding box. Specifically if there is no quad tree at all, this box is the world box.
   */
  db::Box quad_box () const;

  /**
   *  @brief Skips the current quad
   *
   *  Moves to the next quad. This method can be used to shortcut searching if we are inside
   *  a quad that is not relevant for the search.
   */
  void skip_quad ()
  {
    advance (-1);
  }

  /**
   *  @brief Gets the arrays quad ID
   *
   *  The arrays quad ID is a unique identifier for the current quad for iterated arrays. This can be used to
   *  detect whether the iterator entered a new quad and optimize the search in that case.
   */
  size_t array_quad_id () const;

  /**
   *  @brief Gets the quad box
   *
   *  Gets the box the current quad uses. This box may be larger than the actual shape containers
   *  bounding box. Specifically if there is no quad tree at all, this box is the world box.
   */
  db::Box array_quad_box () const;

  /**
   *  @brief Skips the current quad
   *
   *  Moves to the next quad. This method can be used to shortcut searching if we are inside
   *  a quad that is not relevant for the search.
   */
  void skip_array_quad ()
  {
    advance (2);
  }

private:
  //  a helper union for the iter_size union 
  //  (basically computing the size required for all iterators for a certain shape/array type)
  template <class Shape>
  class per_shape_iter_size {
    union {
      //  stable layer iterators
      char sz_n   [sizeof (typename db::layer<Shape, db::stable_layer_tag>::flat_iterator)];
      char sz_np  [sizeof (typename db::layer< db::object_with_properties<Shape>, db::stable_layer_tag >::flat_iterator)];
      char sz_t   [sizeof (typename db::layer<Shape, db::stable_layer_tag>::touching_iterator)];
      char sz_tp  [sizeof (typename db::layer< db::object_with_properties<Shape>, db::stable_layer_tag >::touching_iterator)];
      char sz_o   [sizeof (typename db::layer<Shape, db::stable_layer_tag>::overlapping_iterator)];
      char sz_op  [sizeof (typename db::layer< db::object_with_properties<Shape>, db::stable_layer_tag >::overlapping_iterator)];
      //  unstable layer iterators
      char sz_nu  [sizeof (typename db::layer<Shape, db::unstable_layer_tag>::iterator)];
      char sz_npu [sizeof (typename db::layer< db::object_with_properties<Shape>, db::unstable_layer_tag >::iterator)];
      char sz_tu  [sizeof (typename db::layer<Shape, db::unstable_layer_tag>::touching_iterator)];
      char sz_tpu [sizeof (typename db::layer< db::object_with_properties<Shape>, db::unstable_layer_tag >::touching_iterator)];
      char sz_ou  [sizeof (typename db::layer<Shape, db::unstable_layer_tag>::overlapping_iterator)];
      char sz_opu [sizeof (typename db::layer< db::object_with_properties<Shape>, db::unstable_layer_tag >::overlapping_iterator)];
    } all_iterators;
  };
  
  //  this union is simply there to determine the maximum size required for all
  //  possible iterators.
  union iter_size {
    char sz1  [sizeof (per_shape_iter_size <polygon_type>)];
    char sz2  [sizeof (per_shape_iter_size <polygon_ref_type>)];
    char sz3  [sizeof (per_shape_iter_size <polygon_ptr_array_type>)];
    char sz4  [sizeof (per_shape_iter_size <simple_polygon_type>)];
    char sz5  [sizeof (per_shape_iter_size <simple_polygon_ref_type>)];
    char sz6  [sizeof (per_shape_iter_size <simple_polygon_ptr_array_type>)];
    char sz7  [sizeof (per_shape_iter_size <path_type>)];
    char sz8  [sizeof (per_shape_iter_size <path_ref_type>)];
    char sz9  [sizeof (per_shape_iter_size <path_ptr_array_type>)];
    char sz10 [sizeof (per_shape_iter_size <edge_type>)];
    char sz11 [sizeof (per_shape_iter_size <edge_pair_type>)];
    char sz12 [sizeof (per_shape_iter_size <box_type>)];
    char sz13 [sizeof (per_shape_iter_size <box_array_type>)];
    char sz14 [sizeof (per_shape_iter_size <short_box_type>)];
    char sz15 [sizeof (per_shape_iter_size <short_box_array_type>)];
    char sz16 [sizeof (per_shape_iter_size <text_type>)];
    char sz17 [sizeof (per_shape_iter_size <text_ref_type>)];
    char sz18 [sizeof (per_shape_iter_size <text_ptr_array_type>)];
    char sz19 [sizeof (per_shape_iter_size <user_object_type>)];
    char sz20 [sizeof (per_shape_iter_size <point_type>)];
  };

  //  this union is simply there to determine the maximum size required for all
  //  array iterators.
  union array_iter_size {
    char ai1  [sizeof (polygon_ptr_array_iterator_type)];
    char ai2  [sizeof (simple_polygon_ptr_array_iterator_type)];
    char ai3  [sizeof (path_ptr_array_iterator_type)];
    char ai4  [sizeof (text_ptr_array_iterator_type)];
    char ai5  [sizeof (box_array_iterator_type)];
    char ai6  [sizeof (short_box_array_iterator_type)];
  };

  //  This member must be first to guarantee alignment on 64bit systems:
  //  The strange construction and the local dummy class helps to guarantee alignment of the "iter" space
  union {
    struct _align_helper { long l; } _ah;
    char iter [sizeof (iter_size)];
  } m_d;

  //  This member must be first to guarantee alignment on 64bit systems:
  //  The strange construction and the local dummy class helps to guarantee alignment of the "iter" space
  union {
    struct _align_helper { long l; } _ah;
    char iter [sizeof (array_iter_size)];
  } m_ad;
  
  bool m_valid, m_with_props;
  region_mode m_region_mode;
  object_type m_type;
  box_type m_box;
  shape_type m_shape;
  shape_type m_array;
  unsigned int m_flags;
  const shapes_type *mp_shapes;
  const property_selector *mp_prop_sel;
  bool m_inv_prop_sel : 1;
  bool m_array_iterator_valid : 1;
  bool m_editable : 1;
  size_t m_quad_id;

  template <class Iter> void skip_array_iter ();
  void skip_array ();

  void cleanup ();

  template <class Sh, class StableTag> size_t quad_id_by_shape (TouchingRegionTag) const;
  template <class Sh, class StableTag> size_t quad_id_by_shape (OverlappingRegionTag) const;
  template <class RegionTag, class StableTag> size_t quad_id_generic () const;

  template <class Sh, class StableTag> db::Box quad_box_by_shape (TouchingRegionTag) const;
  template <class Sh, class StableTag> db::Box quad_box_by_shape (OverlappingRegionTag) const;
  template <class RegionTag, class StableTag> db::Box quad_box_generic () const;

  template <class Iter, class Array> db::Box get_array_quad_box () const;
  template <class Iter> size_t get_array_quad_id () const;

  template <class Iter> void do_skip_array_quad_iter ();
  void do_skip_array_quad ();

  template <class Sh, class StableTag, class RegionTag> bool advance_shape (int &mode);
  template <class Array> void init_array_iter (NoRegionTag);
  template <class Array> void init_array_iter (TouchingRegionTag);
  template <class Array> void init_array_iter (OverlappingRegionTag);
  template <class Array, class StableTag, class RegionTag> bool advance_aref (int &mode);
  template <class RegionTag, class StableTag> void advance_generic (int mode);

  void advance (int mode);
};

/**
 *  @brief A helper class for shape generalization
 *
 *  This class serves first as a RTTI token for the 
 *  various shape-specific layer implementations and 
 *  provides some common methods though it's interface
 */

class DB_PUBLIC LayerBase 
{
public:
  typedef tl::func_delegate_base <db::properties_id_type> pm_delegate_type;
  typedef db::Box box_type;
  typedef db::Coord coord_type;

  LayerBase ();
  virtual ~LayerBase ();

  virtual box_type bbox () const = 0;
  virtual void update_bbox () = 0;
  virtual bool is_bbox_dirty () const = 0;
  virtual size_t size () const = 0;
  virtual bool empty () const = 0;
  virtual bool is_tree_dirty () const = 0;
  virtual void sort () = 0;
  virtual LayerBase *clone () const = 0;
  virtual bool is_same_type (const LayerBase *other) const = 0;
  virtual void translate_into (Shapes *target, GenericRepository &rep, ArrayRepository &array_rep) const = 0;
  virtual void translate_into (Shapes *target, GenericRepository &rep, ArrayRepository &array_rep, pm_delegate_type &pm) const = 0;
  virtual void transform_into (Shapes *target, const Trans &trans, GenericRepository &rep, ArrayRepository &array_rep) const = 0;
  virtual void transform_into (Shapes *target, const Trans &trans, GenericRepository &rep, ArrayRepository &array_rep, pm_delegate_type &pm) const = 0;
  virtual void transform_into (Shapes *target, const ICplxTrans &trans, GenericRepository &rep, ArrayRepository &array_rep) const = 0;
  virtual void transform_into (Shapes *target, const ICplxTrans &trans, GenericRepository &rep, ArrayRepository &array_rep, pm_delegate_type &pm) const = 0;
  virtual void insert_into (Shapes *target) = 0;
  virtual void deref_into (Shapes *target) = 0;
  virtual void deref_into (Shapes *target, pm_delegate_type &pm) = 0;
  virtual void deref_and_transform_into (Shapes *target, const Trans &trans) = 0;
  virtual void deref_and_transform_into (Shapes *target, const Trans &trans, pm_delegate_type &pm) = 0;
  virtual void deref_and_transform_into (Shapes *target, const ICplxTrans &trans) = 0;
  virtual void deref_and_transform_into (Shapes *target, const ICplxTrans &trans, pm_delegate_type &pm) = 0;
  virtual unsigned int type_mask () const = 0;

  virtual void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const;
};

/**
 *  @brief A "shapes" collection
 *  
 *  A shapes collection is a collection of geometrical objects.
 *  The implementation is based on a set of layers of different
 *  shape types. 
 *  The general idea is that is rarely required to operate with
 *  different shape types at once. Therefore it is practical to
 *  request only a single shape type at once. This is done through
 *  the various begin.. and end.. methods that are specialized on
 *  a certain shape type.
 */

class DB_PUBLIC Shapes 
  : public db::Object
{
public:
  typedef db::Coord coord_type;
  typedef db::ShapeIterator shape_iterator;
  typedef db::unit_trans<coord_type> unit_trans_type;
  typedef db::box<coord_type> box_type;
  typedef db::generic_repository<coord_type> repository_type;
  typedef db::Shape shape_type;
  typedef db::generic_polygon_edge_iterator<coord_type> polygon_edge_iterator;

  /**
   *  @brief Default ctor: create an empty collection of shapes without external references
   *
   *  Such shape containers can be used for example to store temporary shape sets
   *
   *  Standalone shape containers are usually used for temporary storage. Such containers
   *  are created in editable mode to allow insertion and deletion of shapes by default.
   */
  Shapes ()
    : db::Object (0), mp_cell (0)
  {
    set_editable (true);
  }

  /**
   *  @brief Default ctor: create an empty collection of shapes without external references
   *
   *  Such shape containers can be used for example to store temporary shape sets
   *  This version allows one to specify whether the container should be created in editable mode
   *  or insert-once mode.
   */
  Shapes (bool editable)
    : db::Object (0), mp_cell (0)
  {
    set_editable (editable);
  }

  /**
   *  @brief Standard ctor: create an empty collection referencing a graph (via state model)
   *
   *  The state model reference is used to invalid the bbox flag of the graph
   *  whenever something changes on the shapes list.
   *
   *  Since such containers are usually used in a layout context, they are created according to the
   *  editable settings of the database by default.
   */
  Shapes (db::Manager *manager, db::Cell *cell, bool editable) 
    : db::Object (manager), 
      mp_cell (cell)
  {
    set_dirty (false);
    set_editable (editable);
  }

  /**
   *  @brief Dtor: clear all ..
   */
  ~Shapes () 
  {
    clear ();
    mp_cell = 0;
  }

  /**
   *  @brief Copy ctor
   *
   *  HINT: This method can duplicate shape containers from one layout to another.
   *  The current implementation does not translate property Id's. 
   *  It is mainly intended for 1-to-1 copies of layouts where the whole
   *  property repository is copied.
   *  
   *  NOTE: the copy ctor will register the new object under the same
   *  Manager than the source object.
   */
  Shapes (const Shapes &d) 
    : db::Object (d), 
      mp_cell (d.mp_cell)  //  implicitly copies "dirty" and "editable" 
  {
    operator= (d);
  }

  /**
   *  @brief Assignment operator
   *
   *  HINT: This method can duplicate shape containers from one layout to another.
   *  The current implementation does not translate property Id's. 
   *  It is mainly intended for 1-to-1 copies of layouts where the whole
   *  property repository is copied.
   */
  Shapes &operator= (const Shapes &d);

  /**
   *  @brief Insert all shapes from another container
   *
   *  This method insert all shapes from the given shape container.
   *
   *  HINT: This method can duplicate shape containers from one layout to another.
   *  The current implementation does not translate property Id's. 
   *  It is mainly intended for 1-to-1 copies of layouts where the whole
   *  property repository is copied.
   */
  void insert (const Shapes &d);

  /**
   *  @brief Insert all shapes from another container using the given shape types only
   *
   *  This method insert all shapes from the given shape container.
   *
   *  HINT: This method can duplicate shape containers from one layout to another.
   *  The current implementation does not translate property Id's.
   *  It is mainly intended for 1-to-1 copies of layouts where the whole
   *  property repository is copied.
   */
  void insert (const Shapes &d, unsigned int types);

  /**
   *  @brief Assignment operator with transformation
   *
   *  HINT: This method can duplicate shape containers from one layout to another.
   *  The current implementation does not translate property Id's. 
   *  It is mainly intended for 1-to-1 copies of layouts where the whole
   *  property repository is copied.
   */
  template <class T>
  void assign_transformed (const Shapes &d, const T &trans)
  {
    clear ();
    insert_transformed (d, trans);
  }

  /**
   *  @brief Assignment operator with transformation and property id mapping
   *
   *  This version allows one to specify a property mapping function. That way, shapes can be copied from
   *  one layout space to another.
   */
  template <class T, class PropIdMap>
  void assign_transformed (const Shapes &d, const T &trans, PropIdMap &pm)
  {
    clear ();
    insert_transformed (d, trans, pm);
  }

  /**
   *  @brief Assignment operator with property id mapping
   *
   *  In contrast to the operator= version allows one to specify a property mapping function. That way, shapes can be copied from
   *  one layout space to another.
   */
  template <class PropIdMap>
  void assign (const Shapes &d, PropIdMap &pm)
  {
    clear ();
    insert (d, pm);
  }

  /**
   *  @brief Insert with transformation
   *
   *  This method insert all shapes from the given shape container using the specified transformation.
   *
   *  HINT: This method can duplicate shape containers from one layout to another.
   *  The current implementation does not translate property Id's. 
   *  It is mainly intended for 1-to-1 copies of layouts where the whole
   *  property repository is copied.
   */
  template <class T>
  void insert_transformed (const Shapes &d, const T &trans)
  {
    tl_assert (&d != this);

    if (manager () && manager ()->transacting ()) {

      check_is_editable_for_undo_redo ();

      tl::ident_map<db::properties_id_type> pm;

      //  for undo support iterate over the elements
      for (ShapeIterator s = d.begin (ShapeIterator::All); ! s.at_end (); ++s) {
        insert (*s, trans, pm);
      }

    } else {

      if (layout () == 0) {
        //  deference and transform into this
        for (tl::vector<LayerBase *>::const_iterator l = d.m_layers.begin (); l != d.m_layers.end (); ++l) {
          (*l)->deref_and_transform_into (this, trans);
        }
      } else {
        //  translate and transform into this
        for (tl::vector<LayerBase *>::const_iterator l = d.m_layers.begin (); l != d.m_layers.end (); ++l) {
          (*l)->transform_into (this, trans, shape_repository (), array_repository ());
        }
      }

    }
  }

  /**
   *  @brief Insert with transformation and property id mapping
   *
   *  This method insert all shapes from the given shape container using the specified transformation.
   *
   *  This version allows one to specify a property mapping function. That way, shapes can be copied from
   *  one layout space to another.
   */
  template <class T, class PropIdMap>
  void insert_transformed (const Shapes &d, const T &trans, PropIdMap &pm)
  {
    tl_assert (&d != this);

    if (manager () && manager ()->transacting ()) {

      check_is_editable_for_undo_redo ();

      //  for undo support iterate over the elements
      for (ShapeIterator s = d.begin (ShapeIterator::All); ! s.at_end (); ++s) {
        insert (*s, trans, pm);
      }

    } else {

      tl::func_delegate <PropIdMap, db::properties_id_type> pm_delegate (pm);

      if (layout () == 0) {
        //  deference and transform into this
        for (tl::vector<LayerBase *>::const_iterator l = d.m_layers.begin (); l != d.m_layers.end (); ++l) {
          (*l)->deref_and_transform_into (this, trans, pm_delegate);
        }
      } else {
        //  translate and transform into this
        for (tl::vector<LayerBase *>::const_iterator l = d.m_layers.begin (); l != d.m_layers.end (); ++l) {
          (*l)->transform_into (this, trans, shape_repository (), array_repository (), pm_delegate);
        }
      }

    }
  }

  /**
   *  @brief Insert with property id mapping
   *
   *  This method insert all shapes from the given shape container using the specified transformation.
   *
   *  In contrast to the operator= version allows one to specify a property mapping function. That way, shapes can be copied from
   *  one layout space to another.
   */
  template <class PropIdMap>
  void insert (const Shapes &d, PropIdMap &pm)
  {
    tl_assert (&d != this);

    if (manager () && manager ()->transacting ()) {

      check_is_editable_for_undo_redo ();

      //  for undo support iterate over the elements
      for (ShapeIterator s = d.begin (ShapeIterator::All); ! s.at_end (); ++s) {
        insert (*s, pm);
      }

    } else {

      tl::func_delegate <PropIdMap, db::properties_id_type> pm_delegate (pm);

      if (layout () == 0) {
        //  deference and transform into this
        for (tl::vector<LayerBase *>::const_iterator l = d.m_layers.begin (); l != d.m_layers.end (); ++l) {
          (*l)->deref_into (this, pm_delegate);
        }
      } else {
        //  translate and transform into this
        for (tl::vector<LayerBase *>::const_iterator l = d.m_layers.begin (); l != d.m_layers.end (); ++l) {
          (*l)->translate_into (this, shape_repository (), array_repository (), pm_delegate);
        }
      }

    }
  }

  /**
   *  @brief Swap the contents of this shapes collection with another one
   */
  void swap (Shapes &d);

  /**
   *  @brief Insert a shape of the given type
   *
   *  This inserts a shape of the given kind into the collection.
   *  Basically a new layer is created if no shape of this kind
   *  already existed.
   *  Inserting a shape will invalidate the bbox and the sorting
   *  state.
   *
   *  @param sh The shape to insert (copy)
   *  
   *  @return A reference to the object created
   */
  template <class Sh>
  shape_type insert (const Sh &sh)
  {
    if (manager () && manager ()->transacting ()) {
      check_is_editable_for_undo_redo ();
      if (is_editable ()) {
        db::layer_op<Sh, db::stable_layer_tag>::queue_or_append (manager (), this, true /*insert*/, sh);
      } else {
        db::layer_op<Sh, db::unstable_layer_tag>::queue_or_append (manager (), this, true /*insert*/, sh);
      }
    }
    invalidate_state ();  //  HINT: must come before the change is done!
    if (is_editable ()) {
      return shape_type (this, get_layer<Sh, db::stable_layer_tag> ().insert (sh));
    } else {
      return shape_type (this, *get_layer<Sh, db::unstable_layer_tag> ().insert (sh));
    }
  }

  /**
   *  @brief Insert a shape array of the given type
   *
   *  This inserts a shape arrays of the given kind into the collection.
   *  Basically a new layer is created if no shape of this kind
   *  already existed.
   *  Inserting a shape will invalidate the bbox and the sorting
   *  state.
   *  In editable mode, no arrays are inserted - they will be expanded.
   *
   *  @param sh The shape to insert (copy)
   *  @return The reference to the object inserted - null in editable mode
   */
  template <class Obj, class Trans>
  shape_type insert (const db::array<Obj, Trans> &arr)
  {
    if (is_editable ()) {

      //  expand arrays in editable mode
      if (! arr.begin ().at_end ()) {
        insert_array_typeof (*arr.begin () * Obj () /*for typeof*/, arr);
      }

      return shape_type ();

    } else {

      //  insert the array as a whole in non-editable mode
      if (manager () && manager ()->transacting ()) {
        check_is_editable_for_undo_redo ();
        db::layer_op<db::array<Obj, Trans>, db::unstable_layer_tag>::queue_or_append (manager (), this, true /*insert*/, arr);
      }
      invalidate_state ();  //  HINT: must come before the change is done!
      return shape_type (this, *get_layer<db::array<Obj, Trans>, db::unstable_layer_tag> ().insert (arr));

    }
  }

  /**
   *  @brief Insert a shape array with properties of the given type
   *
   *  This inserts a shape arrays of the given kind into the collection.
   *  Basically a new layer is created if no shape of this kind
   *  already existed.
   *  Inserting a shape will invalidate the bbox and the sorting
   *  state.
   *  In editable mode, no arrays are inserted - they will be expanded.
   *
   *  @param sh The shape to insert (copy)
   *  @return The reference to the object inserted - null in editable mode
   */
  template <class Obj, class Trans>
  shape_type insert (const db::object_with_properties< db::array<Obj, Trans> > &arr)
  {
    if (is_editable ()) {

      //  expand arrays in editable mode
      if (! arr.begin ().at_end ()) {
        insert_array_typeof (*arr.begin () * Obj () /*for typeof*/, arr);
      }

      return shape_type ();

    } else {

      //  insert the array as a whole in non-editable mode
      if (manager () && manager ()->transacting ()) {
        check_is_editable_for_undo_redo ();
        db::layer_op< db::object_with_properties< db::array<Obj, Trans> >, db::unstable_layer_tag>::queue_or_append (manager (), this, true /*insert*/, arr);
      }
      invalidate_state ();  //  HINT: must come before the change is done!
      return shape_type (this, *get_layer< db::object_with_properties< db::array<Obj, Trans> >, db::unstable_layer_tag> ().insert (arr));

    }
  }

  /**
   *  @brief Insert a shape sequence
   *
   *  It is not allowed to insert arrays in editable mode this way, because these are not expanded in editable mode.
   *
   *  Inserts a sequence of shapes [from,to)
   */
  template <class Iter>
  void insert (Iter from, Iter to)
  {
    typedef typename std::iterator_traits <Iter>::value_type value_type;
    if (manager () && manager ()->transacting ()) {
      check_is_editable_for_undo_redo ();
      if (is_editable ()) {
        db::layer_op<value_type, db::stable_layer_tag>::queue_or_append (manager (), this, true /*insert*/, from, to);
      } else {
        db::layer_op<value_type, db::unstable_layer_tag>::queue_or_append (manager (), this, true /*insert*/, from, to);
      }
    }
    invalidate_state ();
    if (is_editable ()) {
      get_layer<value_type, db::stable_layer_tag> ().insert (from, to);
    } else {
      get_layer<value_type, db::unstable_layer_tag> ().insert (from, to);
    }
  }

  /**
   *  @brief Insert an element from the shape reference
   *
   *  Given a shape reference, the corresponding shape is inserted.
   *
   *  @param shape The reference of the shape to insert
   *  @return The reference to the new shape
   */
  shape_type insert (const shape_type &shape)
  {
    tl::ident_map<db::properties_id_type> pm;
    tl::func_delegate <tl::ident_map<db::properties_id_type>, db::properties_id_type> pm_delegate (pm);
    db::unit_trans <coord_type> trans;
    return do_insert (shape, trans, pm_delegate);
  }

  /**
   *  @brief Insert an element from the shape reference
   *
   *  Given a shape reference, the corresponding shape is inserted.
   *
   *  @param shape The reference of the shape to insert
   *  @param pm The property ID mapper - the property ID will be mapped accordingly, i.e. for transferring shapes from one layout to another
   *  @return The reference to the new shape
   */
  template <class PropIdMap>
  shape_type insert (const shape_type &shape, PropIdMap &pm)
  {
    tl::func_delegate <PropIdMap, db::properties_id_type> pm_delegate (pm);
    db::unit_trans <coord_type> trans;
    return do_insert (shape, trans, pm_delegate);
  }

  /**
   *  @brief Insert an element from the shape reference with a transformation
   *
   *  Given a shape reference, the corresponding shape is inserted.
   *
   *  @param shape The reference of the shape to insert
   *  @param trans The transformation to apply before the shape is inserted
   *  @param pm The property ID mapper - the property ID will be mapped accordingly, i.e. for transferring shapes from one layout to another
   *  @return The reference to the new shape
   */
  template <class Trans, class PropIdMap>
  shape_type insert (const shape_type &shape, const Trans &trans, PropIdMap &pm)
  {
    tl::func_delegate <PropIdMap, db::properties_id_type> pm_delegate (pm);
    return do_insert (shape, trans, pm_delegate);
  }

  /**
   *  @brief Reserve the number of elements for a shape type
   *
   *  @param tag The shape type's tag (i.e. db::Polygon::tag)
   *  @param n The number of elements to reserve
   */
  template <class Tag> 
  void reserve (Tag /*tag*/, size_t n)
  {
    if (is_editable ()) {
      get_layer<typename Tag::object_type, db::stable_layer_tag> ().reserve (n);
    } else {
      get_layer<typename Tag::object_type, db::unstable_layer_tag> ().reserve (n);
    }
  }

  /**
   *  @brief Erase an element 
   *
   *  Erases a shape at the given position
   *
   *  @param tag The shape type's tag (i.e. db::Polygon::tag)
   *  @param pos The position of the shape to erase
   */
  template <class Tag, class StableTag> 
  void erase (Tag /*tag*/, StableTag /*stable_tag*/, typename db::layer<typename Tag::object_type, StableTag>::iterator pos)
  {
    if (! is_editable ()) {
      throw tl::Exception (tl::to_string (tr ("Function 'erase' is permitted only in editable mode")));
    }
    if (manager () && manager ()->transacting ()) {
      check_is_editable_for_undo_redo ();
      db::layer_op<typename Tag::object_type, StableTag>::queue_or_append (manager (), this, false /*not insert*/, *pos);
    }
    invalidate_state ();  //  HINT: must come before the change is done!
    get_layer<typename Tag::object_type, StableTag> ().erase (pos);
  }

  /**
   *  @brief Check, if the given shape is valid 
   *
   *  Returns true, if the given shape is valid. It is not valid, if it has been 
   *  deleted already. However, it may happen, that the shape memory has been 
   *  reused already. Therefore this method can safely be used only if nothing 
   *  has been inserted into this container in between.
   *  This method can only be used in editable mode.
   *
   *  @param shape The reference to the shape
   */
  bool is_valid (const shape_type &shape) const;

  /**
   *  @brief Erase an element by the shape reference
   *
   *  Given a shape reference, the corresponding shape is erased.
   *  if the shape references an array member.
   *  This method is only allowed in editable mode.
   *
   *  @param shape The reference to the shape to delete
   */
  void erase_shape (const shape_type &shape);

  /**
   *  @brief Erase an element by the shape reference
   *
   *  Given a set of ordered shape reference, the corresponding shapes are erased.
   *  Hint: for the following code it is important that the ArrayMember types come *after*
   *  the non-member types. This is because this way it is ensured that array members get erased
   *  after the respective single shapes/shape instances. That guarantees that if single shapes
   *  are created on array resolution these shapes can be added to the single shape lists without
   *  invalidating other shape references into them in db::shapes::erase with multi-shape erase.
   *  This method is only allowed in editable mode.
   *
   *  @param shapes The reference to the shapes to delete. This array must be sorted using the Shape's operator<
   */
  void erase_shapes (const std::vector<shape_type> &shapes);

  /**
   *  @brief Erase an element 
   *
   *  Erases a shapes at the given positions [from,to)
   *  This method is only allowed in editable mode.
   *
   *  @param tag The shape type's tag (i.e. db::Polygon::tag)
   *  @param from The first position of the shapes to erase
   *  @param to The past-end position of the shapes to erase
   */
  template <class Tag, class StableTag> 
  void erase (Tag /*tag*/, StableTag /*stable_tag*/, typename db::layer<typename Tag::object_type, StableTag>::iterator from,
                                                     typename db::layer<typename Tag::object_type, StableTag>::iterator to)
  {
    if (! is_editable ()) {
      throw tl::Exception (tl::to_string (tr ("Function 'erase' is permitted only in editable mode")));
    }
    if (manager () && manager ()->transacting ()) {
      check_is_editable_for_undo_redo ();
      db::layer_op<typename Tag::object_type, StableTag>::queue_or_append (manager (), this, false /*not insert*/, from, to);
    }
    invalidate_state ();  //  HINT: must come before the change is done!
    get_layer<typename Tag::object_type, StableTag> ().erase (from, to);
  }

  /**
   *  @brief Erasing of multiple elements
   *
   *  Erase a set of positions given by an iterator I: *(from,to).
   *  *I must render an "iterator" object.
   *  The iterators in the sequence from, to must be sorted in
   *  "later" order.
   *  This method is only allowed in editable mode.
   *
   *  @param tag The shape type's tag (i.e. db::Polygon::tag)
   *  @param first The start of the sequence of iterators
   *  @param last The end of the sequence of iterators
   */
  template <class Tag, class StableTag, class I>
  void erase_positions (Tag /*tag*/, StableTag /*stable_tag*/, I first, I last)
  {
    if (! is_editable ()) {
      throw tl::Exception (tl::to_string (tr ("Function 'erase' is permitted only in editable mode")));
    }
    if (manager () && manager ()->transacting ()) {
      check_is_editable_for_undo_redo ();
      db::layer_op<typename Tag::object_type, StableTag>::queue_or_append (manager (), this, false /*not insert*/, first, last, true /*dummy*/);
    }
    invalidate_state ();  //  HINT: must come before the change is done!
    get_layer<typename Tag::object_type, StableTag> ().erase_positions (first, last);
  }

  /**
   *  @brief Replace the properties Id of a shape 
   *
   *  The properties Id can only be replaced, if the underlying shape is 
   *  of type object_with_properties<X>.
   *  This method is only allowed in editable mode.
   *
   *  @param ref The shape reference which to replace the properties ID with
   *  @param prop_id The properties Id to replace.
   *  @return The reference to the new object
   */
  shape_type replace_prop_id (const shape_type &ref, db::properties_id_type prop_id);

  /**
   *  @brief Replace an element by a given shape
   *
   *  Replaces the shape pointed to by the given shape reference by the given shape.
   *  It is not possible to replace an array member by something else currently.
   *
   *  If the original shape has a property, just the basic shape is replaced. The property id is 
   *  not touched.
   *
   *  @param ref The shape reference which to replace the shape
   *  @param sh The shape to replace 
   *  @return The reference to the new object
   */
  template <class Sh>
  shape_type replace (const shape_type &ref, const Sh &sh);

  /**
   *  @brief Replace an element by a given shape with properties
   *
   *  Replaces the shape pointed to by the given shape reference by the given shape with properties.
   *  It is not possible to replace an array member by something else currently.
   *  If the shape has a property, just the basic shape is replaced. The property id is 
   *  not touched.
   *  This method is only allowed in editable mode.
   *
   *  @param ref The shape reference which to replace the shape
   *  @param sh The shape to replace 
   *  @return The reference to the new object
   */
  template <class Sh>
  shape_type replace (const shape_type &ref, const db::object_with_properties<Sh> &sh)
  {
    //  KLUDGE: this is not quite efficient. It could be done in a single step.
    shape_type first = replace (ref, (const Sh &) sh);
    return replace_prop_id (first, sh.prop_id ());
  }

  /**
   *  @brief Transform an element given by the shape reference by the given transformation
   *
   *  Replaces the shape pointed to by the given shape reference by the one transformed with the
   *  given transformation.
   *  It is not possible to transform an array member by something else currently.
   *  If the shape has a property, just the basic shape is replaced. The property id is 
   *  not touched.
   *  This method is only allowed in editable mode.
   *
   *  @param ref The shape reference which to replace the shape
   *  @param t The transformation to apply
   *  @return The reference to the new object
   */
  template <class Trans>
  shape_type transform (const shape_type &ref, const Trans &t);

  /**
   *  @brief Updates the quad trees (sort ()) and resets the dirty flag
   */
  void update ();

  /**
   *  @brief Returns a value indicating whether the shape container is modified and needs update
   */
  bool is_bbox_dirty () const;

  /**
   *  @brief Resets the "dirty bbox" condition (see is_bbox_dirty)
   */
  void reset_bbox_dirty ();

  /**
   *  @brief Retrieve the bbox 
   *
   *  Retrieving the bbox might required an update_bbox
   *  before the bbox is valid. It will assert if the bbox
   *  was not valid.
   *  Doing a update_bbox before does not imply a performance penalty
   *  since the state is cached.
   */
  box_type bbox () const;

  /**
   *  @brief sorts the trees
   *
   *  Sorting the trees is required after insert operations
   *  and is performed only as far as necessary.
   */
  void sort ();

  /**
   *  @brief Clears the collection
   */
  void clear ();

  /**
   *  @brief Clears the collection (given shape types only)
   */
  void clear (unsigned int types);

  /**
   *  @brief Report the type mask of the objects stored herein
   *
   *  The type mask is composed of the bits that are passed to the 
   *  ShapeIterator to deliver the respective elements.
   */
  unsigned int type_mask () const
  {
    unsigned int tm = 0;
    for (tl::vector<LayerBase *>::const_iterator l = m_layers.begin (); l != m_layers.end (); ++l) {
      tm |= (*l)->type_mask ();
    }
    return tm;
  }

  /**
   *  @brief Report if the shapes object is empty
   */
  bool empty () const
  {
    for (tl::vector<LayerBase *>::const_iterator l = m_layers.begin (); l != m_layers.end (); ++l) {
      if (! (*l)->empty ()) {
        return false;
      }
    }
    return true;
  }

  /**
   *  @brief Report the number of shapes stored herein
   */
  size_t size () const
  {
    size_t n = 0;
    for (tl::vector<LayerBase *>::const_iterator l = m_layers.begin (); l != m_layers.end (); ++l) {
      n += (*l)->size ();
    }
    return n;
  }

  /**
   *  @brief Report the number of shapes stored for a given type mask
   */
  size_t size (unsigned int flags) const
  {
    size_t n = 0;
    for (tl::vector<LayerBase *>::const_iterator l = m_layers.begin (); l != m_layers.end (); ++l) {
      unsigned int tm = (*l)->type_mask ();
      if (((flags & db::ShapeIterator::Properties) == 0 || (tm & db::ShapeIterator::Properties) != 0) && (flags & tm) != 0) {
        n += (*l)->size ();
      }
    }
    return n;
  }

  /**
   *  @brief Report the shape count for a certain type
   */
  template <class Tag, class StableTag> 
  size_t size (Tag /*tag*/, StableTag /*stable_tag*/) const 
  {
    return get_layer<typename Tag::object_type, StableTag> ().size ();
  }

  /**
   *  @brief Deliver the flat iterator (see box_tree::flat_iterator for a description)
   *
   *  @param tag The shape type's tag (i.e. db::Polygon::tag)
   *  
   *  @return The flat iterator 
   */
  template <class Tag, class StableTag> 
  typename db::layer<typename Tag::object_type, StableTag>::flat_iterator begin_flat (Tag /*tag*/, StableTag /*stable_tag*/) const 
  {
    //  ensure that the box tree is established for stable shape containers in editable mode 
    //  (in this case, the iterator uses the flat_iterator which accesses the elements through the sorted 
    //  elements table which is more stable).
    const db::layer<typename Tag::object_type, StableTag> &l = layer<typename Tag::object_type, StableTag> ();
    if (is_editable ()) {
      const_cast <db::layer<typename Tag::object_type, StableTag> &> (l).sort ();
    }
    return l.begin_flat ();
  }

  /**
   *  @brief Do a region search for a certain shape type in "touching" mode
   *
   *  @param tag The shape type's tag (i.e. db::Polygon::tag)
   *  
   *  @return The region iterator 
   */
  template <class Tag, class StableTag> 
  typename db::layer<typename Tag::object_type, StableTag>::touching_iterator begin_touching (const box_type &b, Tag /*tag*/, StableTag /*stable_tag*/) const 
  {
    //  ensure that the box tree is established
    const db::layer<typename Tag::object_type, StableTag> &l = layer<typename Tag::object_type, StableTag> ();
    const_cast <db::layer<typename Tag::object_type, StableTag> &> (l).sort ();
    return l.begin_touching (b);
  }

  /**
   *  @brief Do a region search for a certain shape type in "overlapping" mode
   *
   *  @param tag The shape type's tag (i.e. db::Polygon::tag)
   *  
   *  @return The region iterator 
   */
  template <class Tag, class StableTag> 
  typename db::layer<typename Tag::object_type, StableTag>::overlapping_iterator begin_overlapping (const box_type &b, Tag /*tag*/, StableTag /*stable_tag*/) const 
  {
    //  ensure that the box tree is established
    const db::layer<typename Tag::object_type, StableTag> &l = layer<typename Tag::object_type, StableTag> ();
    const_cast <db::layer<typename Tag::object_type, StableTag> &> (l).sort ();
    return l.begin_overlapping (b);
  }

  /**
   *  @brief begin iterator of all elements of a certain shape type
   *
   *  @param tag The shape type's tag (i.e. db::Polygon::tag)
   *  
   *  @return The first position of the shapes of the requested type
   */
  template <class Tag, class StableTag> 
  typename db::layer<typename Tag::object_type, StableTag>::iterator begin (Tag /*tag*/, StableTag /*stable_tag*/) const
  {
    return get_layer<typename Tag::object_type, StableTag> ().begin ();
  }

  /**
   *  @brief end iterator of all elements of a certain shape type
   *
   *  @param tag The shape type's tag (i.e. db::Polygon::tag)
   *  
   *  @return The post-end position of the shapes of the requested type
   */
  template <class Tag, class StableTag> 
  typename db::layer<typename Tag::object_type, StableTag>::iterator end (Tag /*tag*/, StableTag /*stable_tag*/) const
  {
    return get_layer<typename Tag::object_type, StableTag> ().end ();
  }

  /**
   *  @brief begin iterator of all elements 
   *
   *  @param flags The flags of shapes to query (see db::shape documentation)
   *  @param prop_sel A property selector (a set of property set ids)
   *  @param inv_prop_sel true, if prop_sel should select all ids that should not be iterated
   *  
   *  @return The generic iterator
   */
  shape_iterator begin (unsigned int flags, const shape_iterator::property_selector *prop_sel = 0, bool inv_prop_sel = false) const
  {
    //  ensure that the box tree is established for stable shape containers in editable mode 
    //  (in this case, the iterator uses the flat_iterator which accesses the elements through the sorted 
    //  elements table which is more stable).
    if (is_editable ()) {
      (const_cast <Shapes *> (this))->sort ();
    }
    flags &= ((~shape_iterator::All) | type_mask ());
    return shape_iterator (*this, flags, prop_sel, inv_prop_sel);
  }

  /**
   *  @brief begin iterator of all elements with a touching mode region query
   *
   *  @param box The region to query the shapes from
   *  @param flags The flags of shapes to query (see db::shape documentation)
   *  @param prop_sel A property selector (a set of property set ids)
   *  @param inv_prop_sel true, if prop_sel should select all ids that should not be iterated
   *  
   *  @return The generic iterator
   */
  shape_iterator begin_touching (const box_type &box, unsigned int flags, const shape_iterator::property_selector *prop_sel = 0, bool inv_prop_sel = false) const
  {
    //  ensure that the box tree is established
    (const_cast <Shapes *> (this))->sort ();
    flags &= ((~shape_iterator::All) | type_mask ());
    return shape_iterator (*this, box, shape_iterator::Touching, flags, prop_sel, inv_prop_sel);
  }

  /**
   *  @brief begin iterator of all elements with a overlapping mode region query
   *
   *  @param box The region to query the shapes from
   *  @param flags The flags of shapes to query (see db::shape documentation)
   *  @param prop_sel A property selector (a set of property set ids)
   *  @param inv_prop_sel true, if prop_sel should select all ids that should not be iterated
   *  
   *  @return The generic iterator
   */
  shape_iterator begin_overlapping (const box_type &box, unsigned int flags, const shape_iterator::property_selector *prop_sel = 0, bool inv_prop_sel = false) const
  {
    //  ensure that the box tree is established
    (const_cast <Shapes *> (this))->sort ();
    flags &= ((~shape_iterator::All) | type_mask ());
    return shape_iterator (*this, box, shape_iterator::Overlapping, flags, prop_sel, inv_prop_sel);
  }

  /**
   *  @brief find a given shape (exactly)
   *
   *  @param s The shape to find
   *  
   *  @return end(Sh::tag) if the shape was not found, the position otherwise
   */
  template <class Sh, class StableTag> 
  typename db::layer<Sh, StableTag>::iterator find (const Sh &s) const
  {
    return get_layer<Sh, StableTag> ().find (s);
  }

  /**
   *  @brief find a given shape (exactly)
   *
   *  @param s The shape to find (maybe a reference to a shape in a different container)
   *  
   *  @return The reference to the shape or a null reference if it is not found
   */
  shape_type find (const shape_type &s) const;

  /**
   *  @brief Access to the internal layer object
   *
   *  Hint: this method is provided to the shape iterator mainly. Do not modify the layer object.
   */
  template <class Sh, class StableTag>
  const db::layer<Sh, StableTag> &get_layer () const;

  /**
   *  @brief Access to the internal layer object (non-const version)
   *
   *  Hint: this method is provided to the shape iterator mainly. Do not modify the layer object.
   */
  template <class Sh, class StableTag>
  db::layer<Sh, StableTag> &get_layer ();

  /**
   *  @brief Gets the pointer to cell that the shapes container belongs to
   *
   *  This pointer can be 0 if the shapes container is a standalone container
   */
  db::Cell *cell () const 
  {
    return (db::Cell *) (size_t (mp_cell) & ~3);
  }

  /**
   *  @brief Gets a flag indicating whether an update is needed
   *
   *  This flag means that the shape collection has been modified and the bounding box
   *  and the quad trees will be recomputed (internally).
   */
  bool is_dirty () const
  {
    return (size_t (mp_cell) & 1) != 0;
  }

  /**
   *  @brief Gets a value indicating that the shape collection is constructed with editable scope
   */
  bool is_editable () const
  {
    return (size_t (mp_cell) & 2) != 0;
  }

  /**
   *  @brief Gets the pointer to layout that the shapes container belongs to
   *
   *  This pointer can be 0 if the shapes container is a standalone container
   */
  db::Layout *layout () const;

  /**
   *  @brief Implementation of the redo method
   */
  void redo (db::Op *op);

  /** 
   *  @brief Implementation of the undo method
   */
  void undo (db::Op *op);

  /**
   *  @brief Collect memory usage
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const;

private:
  friend class ShapeIterator;
  friend class FullLayerOp;

  tl::vector<LayerBase *> m_layers;
  db::Cell *mp_cell;  //  HINT: contains "dirty" in bit 0 and "editable" in bit 1

  void invalidate_state ();
  void do_insert (const Shapes &d, unsigned int flags = db::ShapeIterator::All);
  void check_is_editable_for_undo_redo () const;

  //  gets the layers array
  tl::vector<LayerBase *> &get_layers ()
  {
    return m_layers;
  }

  //  get the shape repository associated with this container
  db::GenericRepository &shape_repository () const;

  //  get the array repository associated with this container
  db::ArrayRepository &array_repository () const; 

  //  set dirty flag in mp_cell
  void set_dirty (bool d) 
  {
    mp_cell = (db::Cell *) ((size_t (mp_cell) & ~1) | (d ? 1 : 0));
  }

  //  set editable flag in mp_cell
  void set_editable (bool e) 
  {
    mp_cell = (db::Cell *) ((size_t (mp_cell) & ~2) | (e ? 2 : 0));
  }

  /** 
   *  @brief (Internal) Insert from a generic pointer
   */
  template <class Tag, class PropIdMap>
  shape_type insert_array_by_tag (Tag tag, const shape_type &shape, repository_type &rep, PropIdMap &pm);

  /** 
   *  @brief (Internal) Insert from a generic pointer
   */
  template <class Tag, class PropIdMap>
  shape_type insert_by_tag (Tag tag, const shape_type &shape, repository_type &rep, PropIdMap &pm);

  /** 
   *  @brief (Internal) Insert from a generic pointer
   */
  template <class Tag, class PropIdMap>
  shape_type insert_by_tag (Tag tag, const shape_type &shape, PropIdMap &pm);

  /** 
   *  @brief (Internal) Erase by generic pointer
   */
  template <class Tag>
  bool is_valid_shape_by_tag (Tag tag, const shape_type &shape) const;

  /** 
   *  @brief (Internal) Erase by generic pointer
   */
  template <class Tag>
  shape_type find_shape_by_tag (Tag tag, const shape_type &shape) const;

  /** 
   *  @brief (Internal) Erase by generic pointer
   */
  template <class Tag>
  void erase_shape_by_tag (Tag tag, const shape_type &shape);

  template <class Tag, class StableTag>
  void erase_shape_by_tag_ws (Tag tag, StableTag stable_tag, const shape_type &shape);

  /** 
   *  @brief (Internal) Erase by generic pointer set
   */
  template <class Tag>
  void erase_shapes_by_tag (Tag tag, typename std::vector<shape_type>::const_iterator s1, typename std::vector<shape_type>::const_iterator s2);

  template <class Tag, class StableTag>
  void erase_shapes_by_tag_ws (Tag tag, StableTag stable_tag, typename std::vector<shape_type>::const_iterator s1, typename std::vector<shape_type>::const_iterator s2);

  //  The insert delegate
  template <class Trans>
  shape_type do_insert (const shape_type &shape, const Trans &trans, tl::func_delegate_base <db::properties_id_type> &pm);

  //  The insert delegate, specialization for unit_trans
  shape_type do_insert (const shape_type &shape, const unit_trans_type &trans, tl::func_delegate_base <db::properties_id_type> &pm);

  template <class Sh>
  void replace_prop_id (const Sh *pos, db::properties_id_type prop_id);

  template <class Sh, class Iter>
  shape_type replace_prop_id_iter (typename db::object_tag<Sh>, const Iter &iter, db::properties_id_type prop_id);

  //  A helper function to replace a shape given by a generic reference by doing an erase & insert
  //  Sh2 must not be a shape with properties
  template <class Sh1, class Sh2>
  shape_type reinsert_member_with_props (typename db::object_tag<Sh1>, const shape_type &ref, const Sh2 &sh);

  //  A helper function to replace a shape given by a generic reference 
  //  Sh2 must not be a shape with properties
  template <class Sh1, class Sh2>
  shape_type replace_member_with_props (typename db::object_tag<Sh1>, const shape_type &ref, const Sh2 &sh);

  //  A helper function to replace a shape given by a generic reference (no conversion)
  //  Sh must not be a shape with properties
  template <class Sh>
  shape_type replace_member_with_props (typename db::object_tag<Sh>, const shape_type &ref, const Sh &sh);

  template <class ResType, class Array>
  void insert_array_typeof (const ResType &, const db::object_with_properties<Array> &arr)
  {
    typedef db::object_with_properties<ResType> res_wp_type;

    //  expand arrays in editable mode
    invalidate_state ();  //  HINT: must come before the change is done!
    db::layer<res_wp_type, db::stable_layer_tag> &l = get_layer<res_wp_type, db::stable_layer_tag> ();
    for (typename Array::iterator a = arr.begin (); ! a.at_end (); ++a) {
      res_wp_type obj_wp (*a * arr.object (), arr.properties_id ());
      if (manager () && manager ()->transacting ()) {
        check_is_editable_for_undo_redo ();
        db::layer_op<res_wp_type, db::stable_layer_tag>::queue_or_append (manager (), this, true /*insert*/, obj_wp);
      }
      l.insert (obj_wp);
    }
  }

  template <class ResType, class Array>
  void insert_array_typeof (const ResType &, const Array &arr)
  {
    //  expand arrays in editable mode
    invalidate_state ();  //  HINT: must come before the change is done!
    db::layer<ResType, db::stable_layer_tag> &l = get_layer<ResType, db::stable_layer_tag> ();
    for (typename Array::iterator a = arr.begin (); ! a.at_end (); ++a) {
      if (manager () && manager ()->transacting ()) {
        check_is_editable_for_undo_redo ();
        db::layer_op<ResType, db::stable_layer_tag>::queue_or_append (manager (), this, true /*insert*/, *a * arr.object ());
      }
      l.insert (*a * arr.object ());
    }
  }
};

/**
 *  @brief A base class for layer operations 
 *
 *  This class is used for the Op classes for the undo/redo queuing mechanism.
 */
class DB_PUBLIC LayerOpBase
  : public db::Op
{
public:
  LayerOpBase () : db::Op () { }

  virtual void undo (Shapes *shapes) = 0;
  virtual void redo (Shapes *shapes) = 0;
};

/**
 *  @brief Collect memory usage
 */
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const db::Shapes &x, bool no_self, void *parent)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

/**
 *  @brief A undo/redo queue object for the layer
 *
 *  This class is used internally to queue an insert or erase operation
 *  into the db::Object manager's undo/redo queue.
 */
template <class Sh, class StableTag>
class DB_PUBLIC_TEMPLATE layer_op
  : public LayerOpBase
{
public:
  layer_op (bool insert, const Sh &sh)
    : m_insert (insert)
  {
    m_shapes.reserve (1);
    m_shapes.push_back (sh);
  }
  
  template <class Iter>
  layer_op (bool insert, Iter from, Iter to)
    : m_insert (insert)
  {
    m_shapes.insert (m_shapes.end (), from, to);
  }

  template <class Iter>
  layer_op (bool insert, Iter from, Iter to, bool /*dummy*/)
    : m_insert (insert)
  {
    m_shapes.reserve (std::distance (from, to));
    for (Iter i = from; i != to; ++i) {
      m_shapes.push_back (**i);
    }
  }

  virtual void undo (Shapes *shapes)
  {
    if (m_insert) {
      erase (shapes);
    } else {
      insert (shapes);
    }
  }

  virtual void redo (Shapes *shapes)
  {
    if (m_insert) {
      insert (shapes);
    } else {
      erase (shapes);
    }
  }

  static void queue_or_append (db::Manager *manager, db::Shapes *shapes, bool insert, const Sh &sh)
  {
    db::layer_op<Sh, StableTag> *old_op = dynamic_cast <db::layer_op<Sh, StableTag> *> (manager->last_queued (shapes));
    if (! old_op || old_op->m_insert != insert) {
      manager->queue (shapes, new db::layer_op<Sh, StableTag> (insert, sh));
    } else {
      old_op->m_shapes.push_back (sh);
    }
  }

  template <class Iter>
  static void queue_or_append (db::Manager *manager, db::Shapes *shapes, bool insert, Iter from, Iter to)
  {
    db::layer_op<Sh, StableTag> *old_op = dynamic_cast <db::layer_op<Sh, StableTag> *> (manager->last_queued (shapes));
    if (! old_op || old_op->m_insert != insert) {
      manager->queue (shapes, new db::layer_op<Sh, StableTag> (insert, from, to));
    } else {
      old_op->m_shapes.insert (old_op->m_shapes.end (), from, to);
    }
  }

  template <class Iter>
  static void queue_or_append (db::Manager *manager, db::Shapes *shapes, bool insert, Iter from, Iter to, bool dummy)
  {
    db::layer_op<Sh, StableTag> *old_op = dynamic_cast <db::layer_op<Sh, StableTag> *> (manager->last_queued (shapes));
    if (! old_op || old_op->m_insert != insert) {
      manager->queue (shapes, new db::layer_op<Sh, StableTag> (insert, from, to, dummy));
    } else {
      for (Iter i = from; i != to; ++i) {
        old_op->m_shapes.push_back (**i);
      }
    }
  }

private:
  bool m_insert;
  std::vector<Sh> m_shapes;

  void insert (Shapes *shapes);
  void erase (Shapes *shapes);
};

class FullLayerOp
  : public LayerOpBase
{
public:
  FullLayerOp (bool insert, LayerBase *layer)
    : m_insert (insert), mp_layer (layer), m_owns_layer (! insert)
  {
    //  .. nothing yet ..
  }

  ~FullLayerOp ()
  {
    if (m_owns_layer) {
      delete mp_layer;
      mp_layer = 0;
    }
  }

  virtual void undo (Shapes *shapes)
  {
    if (m_insert) {
      erase (shapes);
    } else {
      insert (shapes);
    }
  }

  virtual void redo (Shapes *shapes)
  {
    if (m_insert) {
      insert (shapes);
    } else {
      erase (shapes);
    }
  }

private:
  bool m_insert;
  LayerBase *mp_layer;
  bool m_owns_layer;

  void insert (Shapes *shapes);
  void erase (Shapes *shapes);
};

}  // namespace db
  
#endif

