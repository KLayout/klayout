
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


#ifndef HDR_dbInstances
#define HDR_dbInstances

#include "dbCommon.h"

#include "dbManager.h"
#include "dbStatic.h"
#include "dbBoxTree.h"
#include "dbBoxConvert.h"
#include "dbArray.h"
#include "dbPropertiesRepository.h"
#include "dbCellInst.h"
#include "tlVector.h"
#include "tlTypeTraits.h"
#include "tlUtils.h"

namespace db
{

class Layout;
class Cell;
class Instances;
template <class Inst, class ET> class InstOp;

/**
 *  @brief A standard typedef for a cell instance array
 */
typedef db::array <db::CellInst, db::Trans> CellInstArray;

/**
 *  @brief A standard typedef for a cell instance array in micron units
 */
typedef db::array <db::CellInst, db::DTrans> DCellInstArray;

/**
 *  @brief A classification type for editable mode types and concepts
 */
struct InstancesEditableTag { };

/**
 *  @brief A classification type for non-editable mode types and concepts
 */
struct InstancesNonEditableTag { };

/**
 *  @brief A traits type for editable mode types and concepts
 */
template <class ET> 
struct instances_editable_traits;

template <>
struct instances_editable_traits<InstancesEditableTag> 
{ 
  typedef tl::true_tag is_editable;

  template <class Tag> 
  struct tree_traits
  {
    typedef db::box_tree<typename Tag::object_type::box_type, typename Tag::object_type, db::box_convert<typename Tag::object_type, false> > tree_type;
    typedef typename tree_type::iterator tree_iter;
  };
};

template <>
struct instances_editable_traits<InstancesNonEditableTag> 
{
  typedef tl::false_tag is_editable;

  template <class Tag> 
  struct tree_traits
  {
    typedef db::unstable_box_tree<typename Tag::object_type::box_type, typename Tag::object_type, db::box_convert<typename Tag::object_type, false> > tree_type;
    typedef typename tree_type::iterator tree_iter;
  };
};

/**
 *  @brief A instance reference
 *
 *  In analogy to the "shape" reference (i.e. db::Shape), this instance
 *  reference points to a certain instance and provides an abstract interface.
 */

class DB_PUBLIC Instance
{
public:
  typedef db::Layout layout_type;
  typedef db::CellInst cell_inst_type;
  typedef db::Coord coord_type;
  typedef db::Box box_type;
  typedef db::CellInstArray cell_inst_array_type;
  typedef db::object_with_properties<cell_inst_array_type> cell_inst_wp_array_type;
  typedef tl::reuse_vector<cell_inst_array_type>::const_iterator cell_inst_array_iterator_type;
  typedef tl::reuse_vector<cell_inst_wp_array_type>::const_iterator cell_inst_wp_array_iterator_type;

  enum object_type {
    TNull,
    TInstance
  };

  /**
   *  @brief Initialize a reference with "nil"
   */
  Instance ();

  /**
   *  @brief Destructor
   */
  ~Instance ();
  
  /**
   *  @brief Initialize a reference with an instance pointer
   *
   *  Hint: there is no const Instance, hence we use the const_cast hack.
   */
  Instance (const db::Instances *instances, const cell_inst_array_type &inst);
  
  /**
   *  @brief Initialize a reference with an instance pointer
   */
  Instance (db::Instances *instances, const cell_inst_array_type &inst);
  
  /**
   *  @brief Initialize a reference with an pointer to an instance with properties
   *
   *  Hint: there is no const Instance, hence we use the const_cast hack.
   */
  Instance (const db::Instances *instances, const cell_inst_wp_array_type &inst);

  /**
   *  @brief Initialize a reference with an pointer to an instance with properties
   */
  Instance (db::Instances *instances, const cell_inst_wp_array_type &inst);

  /**
   *  @brief Initialize a reference with an iterator to an instance 
   *
   *  Hint: there is no const Instance, hence we use the const_cast hack.
   */
  Instance (const db::Instances *instances, const cell_inst_array_iterator_type &iter);

  /**
   *  @brief Initialize a reference with an iterator to an instance 
   */
  Instance (db::Instances *instances, const cell_inst_array_iterator_type &iter);

  /**
   *  @brief Initialize a reference with an iterator to an instance with properties
   *
   *  Hint: there is no const Instance, hence we use the const_cast hack.
   */
  Instance (const db::Instances *instances, const cell_inst_wp_array_iterator_type &iter);

  /**
   *  @brief Initialize a reference with an iterator to an instance with properties
   */
  Instance (db::Instances *instances, const cell_inst_wp_array_iterator_type &iter);

  /**
   *  @brief Get the properties ID in an abstract way
   */
  db::properties_id_type prop_id () const
  {
    if (has_prop_id ()) {
      return basic_ptr (cell_inst_wp_array_type::tag ())->properties_id ();
    } else {
      return 0;
    }
  }

  /**
   *  @brief Test, if the iterator is of a certain type
   */
  bool has_prop_id () const
  {
    return m_with_props;
  }

  /**
   *  @brief Test, if this instance is a valid one
   */
  bool is_null () const
  {
    return m_type == TNull;
  }

  /**
   *  @brief Get the basic instance (without properties)
   */
  const cell_inst_array_type &cell_inst () const
  {
    static cell_inst_array_type default_array;

    if (m_type != TInstance) {
      return default_array;
    } else if (m_with_props) {
      if (m_stable) {
        return ((cell_inst_wp_array_iterator_type *) m_generic.piter)->operator* ();
      } else {
        return *m_generic.pinst;
      }
    } else {
      if (m_stable) {
        return ((cell_inst_array_iterator_type *) m_generic.iter)->operator* ();
      } else {
        return *m_generic.inst;
      }
    }
  }

  /**
   *  @brief Return the cell index of the reference
   *
   *  This method is basically provided for convenience
   *  and is equivalent to ->cell_inst ().object ().cell_index ().
   */
  cell_index_type cell_index () const
  {
    return cell_inst ().object ().cell_index ();
  }

  /**
   *  @brief Check if the instance array is a regular one and return the parameters
   *
   *  This method is basically provided for convenience
   *  and is equivalent to ->cell_inst ().is_regular_array ().
   */
  bool is_regular_array (cell_inst_array_type::vector_type &a, cell_inst_array_type::vector_type &b, unsigned long &amax, unsigned long &bmax) const
  {
    return cell_inst ().is_regular_array (a, b, amax, bmax);
  }

  /**
   *  @brief Check if the instance array is an iterated array and return the parameters
   *
   *  This method is basically provided for convenience
   *  and is equivalent to ->cell_inst ().is_iterated_array ().
   */
  bool is_iterated_array (std::vector<cell_inst_array_type::vector_type> *v = 0) const
  {
    return cell_inst ().is_iterated_array (v);
  }

  /**
   *  @brief Return the complex transformation of this instance
   *
   *  This method is basically provided for convenience
   *  and is equivalent to ->cell_inst ().complex_trans ().
   */
  cell_inst_array_type::complex_trans_type complex_trans () const
  {
    return cell_inst ().complex_trans ();
  }

  /**
   *  @brief Return the complex transformation of this instance (for a given base transformation)
   *
   *  This method is basically provided for convenience
   *  and is equivalent to ->cell_inst ().complex_trans (t).
   */
  cell_inst_array_type::complex_trans_type complex_trans (const cell_inst_array_type::simple_trans_type &t) const
  {
    return cell_inst ().complex_trans (t);
  }

  /**
   *  @brief Return true, if the transformation of this instance is complex
   *
   *  This method is basically provided for convenience
   *  and is equivalent to ->cell_inst ().is_complex ().
   */
  bool is_complex () const
  {
    return cell_inst ().is_complex ();
  }

  /**
   *  @brief Return the first transformation of this array
   *
   *  This method is basically provided for convenience
   *  and is equivalent to ->cell_inst ().front ().
   */
  const cell_inst_array_type::trans_type &front () const
  {
    return cell_inst ().front ();
  }

  /**
   *  @brief Return the number of flat instances represented by this array
   *
   *  This method is basically provided for convenience
   *  and is equivalent to ->cell_inst ().size ().
   */
  size_t size () const
  {
    return cell_inst ().size ();
  }

  /**
   *  @brief Return the bounding box of this array 
   *
   *  This method is basically provided for convenience
   *  and is equivalent to ->cell_inst ().bbox (bc)
   */
  template <bool AllowEmpty>
  cell_inst_array_type::box_type bbox (const db::box_convert<cell_inst_type, AllowEmpty> &bc) const
  {
    return cell_inst ().bbox (bc);
  }

  /**
   *  @brief Returns the bounding box of this array
   *
   *  This method uses the pointers provided internally to identify container and cell
   */
  cell_inst_array_type::box_type bbox () const;

  /**
   *  @brief Return the iterator for the instances of the array
   *
   *  This method is basically provided for convenience
   *  and is equivalent to ->cell_inst ().begin ().
   */
  cell_inst_array_type::iterator begin () const 
  {
    return cell_inst ().begin ();
  }

  /**
   *  @brief Return the region query iterator for the instances of the array
   *
   *  This method is basically provided for convenience
   *  and is equivalent to ->cell_inst ().begin_touching ().
   */
  cell_inst_array_type::iterator begin_touching (const cell_inst_array_type::box_type &b, const layout_type *g) const;

  /**
   *  @brief Equality
   *
   *  Equality of instances is not specified by the identity of the objects but by the
   *  identity of the pointers - both instances must reference the same object.
   */
  bool operator== (const Instance &d) const;

  /**
   *  @brief Value equality
   *
   *  In contrast to the operator==, this method compares values rather than pointers.
   */
  bool equals (const Instance &d) const
  {
    return prop_id () == d.prop_id () && cell_inst () == d.cell_inst ();
  }

  /**
   *  @brief Inequality
   *
   *  This compares the value, not the pointers.
   */
  bool operator!= (const Instance &d) const
  {
    return !operator== (d);
  }

  /**
   *  @brief Less operator
   *
   *  Ordering of instances is not specified by the identity of the objects but by the
   *  order of the pointers.
   *  Since pointers are volatile objects, the ordering is not strictly reproducible!
   *  The order is designed such that different types of instances are separated in
   *  an order sequence of instance proxies. This is an important fact for the 
   *  "erase" functionality of the instances container when erasing a set of objects.
   */
  bool operator< (const Instance &d) const;

  /**
   *  @brief Comparison of values
   *
   *  In contrast to operator<, this method compares values rather than pointers.
   */
  bool less (const Instance &d) const
  {
    if (prop_id () != d.prop_id ()) {
      return prop_id () < d.prop_id ();
    }
    if (cell_inst () != d.cell_inst ()) {
      return cell_inst () < d.cell_inst ();
    }
    return false;
  }

  /**
   *  @brief Get the basic pointer for instances with properties
   *
   *  The pointer will be 0, if this is not an instance with properties.
   */
  const cell_inst_wp_array_type *basic_ptr (cell_inst_wp_array_type::tag /*tag*/) const
  {
    if (m_type != TInstance || ! m_with_props) {
      return 0;
    } else if (m_stable) {
      return ((cell_inst_wp_array_iterator_type *) (m_generic.piter))->operator-> ();
    } else {
      return m_generic.pinst;
    }
  }

  /**
   *  @brief Get the basic pointer to a normal instances
   *
   *  The pointer will be 0, if this is not an instance without properties.
   */
  const cell_inst_array_type *basic_ptr (cell_inst_array_type::tag /*tag*/) const
  {
    if (m_type != TInstance || m_with_props) {
      return 0;
    } else if (m_stable) {
      return ((cell_inst_array_iterator_type *) (m_generic.iter))->operator-> ();
    } else {
      return m_generic.inst;
    }
  }

  /**
   *  @brief Get the basic iterator for instances with properties
   *
   *  The returned pointer to the iterator will be 0, if this is not an instance with properties.
   */
  const cell_inst_wp_array_iterator_type *basic_iter (cell_inst_wp_array_type::tag /*tag*/) const
  {
    if (m_type != TInstance || ! m_with_props || ! m_stable) {
      return 0;
    } else {
      return (cell_inst_wp_array_iterator_type *) m_generic.piter;
    }
  }

  /**
   *  @brief Get the basic pointer to a normal instances
   *
   *  The returned pointer to the iterator will be 0, if this is an instance with properties.
   */
  const cell_inst_array_iterator_type *basic_iter (cell_inst_array_type::tag /*tag*/) const
  {
    if (m_type != TInstance || m_with_props || ! m_stable) {
      return 0;
    } else {
      return (cell_inst_array_iterator_type *) m_generic.iter;
    }
  }

  /**
   *  @brief Convert to a string
   */
  std::string to_string (bool resolve_cell_name = false) const;

  /**
   *  @brief Gets the pointer to the instance container the instance is contained in
   */
  db::Instances *instances () const
  {
    return mp_instances;
  }

private:
  friend class Instances;

  union generic {
    const cell_inst_array_type *inst;
    const cell_inst_wp_array_type *pinst;
    char iter[sizeof (cell_inst_array_iterator_type)];
    char piter[sizeof (cell_inst_wp_array_iterator_type)];
  } m_generic;

  db::Instances *mp_instances;
  bool m_with_props : 8;
  bool m_stable : 8;
  object_type m_type : 16;
};

/**
 *  @brief A generic iterator for the instances 
 *
 *  This generic iterator is specialized with the Traits object, which
 *  determines how to initialize the iterators and what types to use.
 */

//  NOTE: we do explicit instantiation, so the exposure is declared
//  as DB_PUBLIC - as if it wasn't a template
template <class IterTraits>
class DB_PUBLIC instance_iterator
{
public:
  typedef db::Layout layout_type;
  typedef db::CellInst cell_inst_type;
  typedef cell_inst_type::coord_type coord_type;
  typedef db::CellInstArray basic_inst_type;
  typedef basic_inst_type cell_inst_array_type;
  typedef db::object_with_properties<cell_inst_array_type> cell_inst_wp_array_type;
  typedef db::Instance value_type;
  typedef db::Instances instances_type;
  typedef typename IterTraits::iter_type iter_type;
  typedef typename IterTraits::iter_wp_type iter_wp_type;
  typedef typename IterTraits::stable_iter_type stable_iter_type;
  typedef typename IterTraits::stable_iter_wp_type stable_iter_wp_type;
  typedef const value_type *pointer; 
  typedef value_type reference;   //  operator* returns a value
  typedef std::forward_iterator_tag iterator_category;
  typedef void difference_type;

  enum object_type {
    TNull,
    TInstance
  };

  /**
   *  @brief Default ctor
   */
  instance_iterator ()
    : m_with_props (false), m_stable (false), m_type (TNull), m_traits ()
  { }

  /**
   *  @brief Destructor
   */
  ~instance_iterator ()
  {
    release_iter ();
  }

  /**
   *  @brief Constructor
   */
  instance_iterator (const IterTraits &traits)
    : m_with_props (false), m_stable (traits.instances ()->is_editable ()), m_type (TInstance), m_traits (traits)
  { 
    make_iter ();
    make_next ();
    update_ref ();
  }

  /**
   *  @brief Copy constructor
   */
  instance_iterator (const instance_iterator &iter)
    : m_with_props (false), m_stable (false), m_type (TNull), m_traits ()
  {
    operator= (iter);
  }

  /**
   *  @brief Assignment operator
   */
  instance_iterator &operator= (const instance_iterator &iter);

  /**
   *  @brief Equality
   */
  bool operator== (const instance_iterator &d) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const instance_iterator &d) const
  {
    return !operator== (d);
  }

  /**
   *  @brief Access to the actual instance
   *  
   *  HINT: this method returns a value, not a "const value &" because this way, 
   *  we do not loose much (it is not a expensive object) but gain more stability in the
   *  GSI interface, in which we need to be careful not to return references to temporary objects.
   */
  value_type operator* () const
  {
    return m_ref;
  }

  /**
   *  @brief Access to the actual instance (pointer)
   */
  const value_type *operator-> () const
  {
    return &m_ref;
  }

  /**
   *  @brief Increment operator
   */
  instance_iterator &operator++();

  /**
   *  @brief Skip the current quad
   */
  void skip_quad ();

  /**
   *  @brief Get the current's quad ID
   */
  size_t quad_id () const;

  /**
   *  @brief Get the current's quad box
   */
  db::Box quad_box () const;

  /**
   *  @brief Report the end of the iterator (in non-STL schemes)
   */
  bool at_end () const
  {
    return m_type == TNull;
  }

  /**
   *  @brief Gets the basic iterator in the non-editable case without properties
   */
  iter_type &basic_iter (cell_inst_array_type::tag, InstancesNonEditableTag)
  {
    tl_assert (m_type == TInstance && m_stable == false && m_with_props == false);
    return *((iter_type *) m_generic.iter);
  }

  /**
   *  @brief Gets the basic iterator in the non-editable case without properties (const version)
   */
  const iter_type &basic_iter (cell_inst_array_type::tag, InstancesNonEditableTag) const
  {
    tl_assert (m_type == TInstance && m_stable == false && m_with_props == false);
    return *((iter_type *) m_generic.iter);
  }

  /**
   *  @brief Gets the basic iterator in the editable case without properties
   */
  stable_iter_type &basic_iter (cell_inst_array_type::tag, InstancesEditableTag)
  {
    tl_assert (m_type == TInstance && m_stable == true && m_with_props == false);
    return *((stable_iter_type *) m_generic.stable_iter);
  }

  /**
   *  @brief Gets the basic iterator in the editable case without properties (const version)
   */
  const stable_iter_type &basic_iter (cell_inst_array_type::tag, InstancesEditableTag) const
  {
    tl_assert (m_type == TInstance && m_stable == true && m_with_props == false);
    return *((stable_iter_type *) m_generic.stable_iter);
  }

  /**
   *  @brief Gets the basic iterator in the non-editable case with properties
   */
  iter_wp_type &basic_iter (cell_inst_wp_array_type::tag, InstancesNonEditableTag)
  {
    tl_assert (m_type == TInstance && m_stable == false && m_with_props == true);
    return *((iter_wp_type *) m_generic.piter);
  }

  /**
   *  @brief Gets the basic iterator in the non-editable case with properties (const version)
   */
  const iter_wp_type &basic_iter (cell_inst_wp_array_type::tag, InstancesNonEditableTag) const
  {
    tl_assert (m_type == TInstance && m_stable == false && m_with_props == true);
    return *((iter_wp_type *) m_generic.piter);
  }

  /**
   *  @brief Gets the basic iterator in the editable case with properties
   */
  stable_iter_wp_type &basic_iter (cell_inst_wp_array_type::tag, InstancesEditableTag)
  {
    tl_assert (m_type == TInstance && m_stable == true && m_with_props == true);
    return *((stable_iter_wp_type *) m_generic.pstable_iter);
  }

  /**
   *  @brief Gets the basic iterator in the editable case with properties (const version)
   */
  const stable_iter_wp_type &basic_iter (cell_inst_wp_array_type::tag, InstancesEditableTag) const
  {
    tl_assert (m_type == TInstance && m_stable == true && m_with_props == true);
    return *((stable_iter_wp_type *) m_generic.pstable_iter);
  }

private:
  friend struct NormalInstanceIteratorTraits;
  friend struct OverlappingInstanceIteratorTraits;
  friend struct TouchingInstanceIteratorTraits;

  union generic {
    char iter[sizeof (iter_type)];
    char piter[sizeof (iter_wp_type)];
    char stable_iter[sizeof (stable_iter_type)];
    char pstable_iter[sizeof (stable_iter_wp_type)];
  } m_generic;

  bool m_with_props : 8;
  bool m_stable : 8;
  object_type m_type : 16;
  value_type m_ref;
  IterTraits m_traits;

  void update_ref ();
  void release_iter ();
  void make_iter ();
  void make_next ();
};

/**
 *  @brief IterTraits for the normal iterator
 */

struct DB_PUBLIC NormalInstanceIteratorTraits
{
  typedef db::Layout layout_type;
  typedef db::Box box_type;
  typedef box_type::coord_type coord_type;
  typedef db::CellInst cell_inst_type;
  typedef db::CellInstArray cell_inst_array_type;
  typedef db::object_with_properties<cell_inst_array_type> cell_inst_wp_array_type;
  typedef db::box_convert<cell_inst_array_type, false> cell_inst_array_box_converter;
  typedef db::box_convert<cell_inst_wp_array_type, false> cell_inst_wp_array_box_converter;
  typedef db::unstable_box_tree<box_type, cell_inst_array_type, cell_inst_array_box_converter> cell_inst_tree_type;
  typedef db::unstable_box_tree<box_type, cell_inst_wp_array_type, cell_inst_wp_array_box_converter> cell_inst_wp_tree_type;
  typedef db::box_tree<box_type, cell_inst_array_type, cell_inst_array_box_converter> stable_cell_inst_tree_type;
  typedef db::box_tree<box_type, cell_inst_wp_array_type, cell_inst_wp_array_box_converter> stable_cell_inst_wp_tree_type;
  typedef db::Instances instances_type;
  typedef db::Instance instance_type;

  typedef tl::iterator_pair<cell_inst_tree_type::const_iterator> iter_type;
  typedef tl::iterator_pair<cell_inst_wp_tree_type::const_iterator> iter_wp_type;
  typedef stable_cell_inst_tree_type::flat_iterator stable_iter_type;
  typedef stable_cell_inst_wp_tree_type::flat_iterator stable_iter_wp_type;

  NormalInstanceIteratorTraits ();
  NormalInstanceIteratorTraits (const instances_type *insts);
  void init (instance_iterator<NormalInstanceIteratorTraits> *iter) const;

  template <class Iter> instance_type instance_from_stable_iter (const Iter &iter) const;
 
  template <class Iter> void skip_quad (Iter & /*iter*/) const { }
  template <class Iter> size_t quad_id (const Iter & /*iter*/) const { return 0; }
  template <class Iter> box_type quad_box (const Iter & /*iter*/) const { return db::Box (); }

  const instances_type *instances () const
  {
    return mp_insts;
  }

public:
  const instances_type *mp_insts;
};

/**
 *  @brief IterTraits for the touching iterator
 */

struct DB_PUBLIC TouchingInstanceIteratorTraits
{
  typedef db::Layout layout_type;
  typedef db::Box box_type;
  typedef box_type::coord_type coord_type;
  typedef db::CellInst cell_inst_type;
  typedef db::CellInstArray cell_inst_array_type;
  typedef db::object_with_properties<cell_inst_array_type> cell_inst_wp_array_type;
  typedef db::box_convert<cell_inst_array_type, false> cell_inst_array_box_converter;
  typedef db::box_convert<cell_inst_wp_array_type, false> cell_inst_wp_array_box_converter;
  typedef db::unstable_box_tree<box_type, cell_inst_array_type, cell_inst_array_box_converter> cell_inst_tree_type;
  typedef db::unstable_box_tree<box_type, cell_inst_wp_array_type, cell_inst_wp_array_box_converter> cell_inst_wp_tree_type;
  typedef db::box_tree<box_type, cell_inst_array_type, cell_inst_array_box_converter> stable_cell_inst_tree_type;
  typedef db::box_tree<box_type, cell_inst_wp_array_type, cell_inst_wp_array_box_converter> stable_cell_inst_wp_tree_type;
  typedef db::Instances instances_type;
  typedef db::Instance instance_type;

  typedef cell_inst_tree_type::touching_iterator iter_type;
  typedef cell_inst_wp_tree_type::touching_iterator iter_wp_type;
  typedef stable_cell_inst_tree_type::touching_iterator stable_iter_type;
  typedef stable_cell_inst_wp_tree_type::touching_iterator stable_iter_wp_type;

  TouchingInstanceIteratorTraits ();
  TouchingInstanceIteratorTraits (const instances_type *insts, const box_type &box, const layout_type *layout);
  void init (instance_iterator<TouchingInstanceIteratorTraits> *iter) const;

  template <class Iter>
  instance_type instance_from_stable_iter (const Iter &iter) const;
 
  template <class Iter> void skip_quad (Iter &iter) const { iter.skip_quad (); }
  template <class Iter> size_t quad_id (const Iter &iter) const { return iter.quad_id (); }
  template <class Iter> box_type quad_box (const Iter &iter) const { return iter.quad_box (); }

  const instances_type *instances () const
  {
    return mp_insts;
  }

public:
  const instances_type *mp_insts;
  box_type m_box;
  const layout_type *mp_layout;

private:
  template <class InstArray, class ET> void init (instance_iterator<TouchingInstanceIteratorTraits> *iter) const;
};

/**
 *  @brief IterTraits for the overlapping iterator
 */

struct DB_PUBLIC OverlappingInstanceIteratorTraits
{
  typedef db::Layout layout_type;
  typedef db::Box box_type;
  typedef box_type::coord_type coord_type;
  typedef db::CellInst cell_inst_type;
  typedef db::CellInstArray cell_inst_array_type;
  typedef db::object_with_properties<cell_inst_array_type> cell_inst_wp_array_type;
  typedef db::box_convert<cell_inst_array_type, false> cell_inst_array_box_converter;
  typedef db::box_convert<cell_inst_wp_array_type, false> cell_inst_wp_array_box_converter;
  typedef db::unstable_box_tree<box_type, cell_inst_array_type, cell_inst_array_box_converter> cell_inst_tree_type;
  typedef db::unstable_box_tree<box_type, cell_inst_wp_array_type, cell_inst_wp_array_box_converter> cell_inst_wp_tree_type;
  typedef db::box_tree<box_type, cell_inst_array_type, cell_inst_array_box_converter> stable_cell_inst_tree_type;
  typedef db::box_tree<box_type, cell_inst_wp_array_type, cell_inst_wp_array_box_converter> stable_cell_inst_wp_tree_type;
  typedef db::Instances instances_type;
  typedef db::Instance instance_type;

  typedef cell_inst_tree_type::overlapping_iterator iter_type;
  typedef cell_inst_wp_tree_type::overlapping_iterator iter_wp_type;
  typedef stable_cell_inst_tree_type::overlapping_iterator stable_iter_type;
  typedef stable_cell_inst_wp_tree_type::overlapping_iterator stable_iter_wp_type;

  OverlappingInstanceIteratorTraits ();
  OverlappingInstanceIteratorTraits (const instances_type *insts, const box_type &box, const layout_type *layout);
  void init (instance_iterator<OverlappingInstanceIteratorTraits> *iter) const;

  template <class Iter>
  instance_type instance_from_stable_iter (const Iter &iter) const;
 
  template <class Iter> void skip_quad (Iter &iter) const { iter.skip_quad (); }
  template <class Iter> size_t quad_id (const Iter &iter) const { return iter.quad_id (); }
  template <class Iter> box_type quad_box (const Iter &iter) const { return iter.quad_box (); }

  const instances_type *instances () const
  {
    return mp_insts;
  }

public:
  const instances_type *mp_insts;
  box_type m_box;
  const layout_type *mp_layout;

private:
  template <class InstArray, class ET> void init (instance_iterator<OverlappingInstanceIteratorTraits> *iter) const;
};

/**
 *  @brief A child cell iterator
 *
 *  The iterator will report just cell indices, not instances.
 */

class DB_PUBLIC ChildCellIterator
{
public: 
  typedef db::Layout layout_type;
  typedef db::CellInst cell_inst_type;
  typedef cell_index_type value_type;
  typedef db::Instances instances_type;
  typedef db::CellInstArray basic_inst_type;
  typedef tl::vector<const basic_inst_type *>::const_iterator inst_iterator_type;
  typedef void pointer;                //  no operator->
  typedef cell_index_type reference;   //  operator* returns a value
  typedef std::forward_iterator_tag iterator_category;
  typedef void difference_type;

  /**
   *  @brief Default ctor
   */
  ChildCellIterator ();

  /**
   *  @brief Constructor
   */
  ChildCellIterator (const instances_type *insts);

  /**
   *  @brief Access to the actual instance
   *
   *  The actual instance is created on the fly by the update_rep method.
   *  This will create an artificial instance which takes into account the
   *  offset from the representative.
   */
  cell_index_type operator* () const;

  /**
   *  @brief Increment operator
   */
  ChildCellIterator &operator++();

  /**
   *  @brief Report the end of the iterator (in non-STL schemes)
   */
  bool at_end () const
  {
    return (m_iter == m_end);
  }

  /**
   *  @brief Returns the weight of this child cell (the total number of instances in the parent cell)
   */
  size_t weight () const;

  /**
   *  @brief Returns the number of instances of this child cell in the parent cell (arefs count as one)
   */
  size_t instances () const;

private:
  inst_iterator_type m_iter, m_end;
};

/**
 *  @brief A parent instance
 *
 *  A parent instance is basically an inverse instance: instead of pointing
 *  to the child cell, it is pointing to the parent cell and the transformation
 *  is representing the shift of the parent cell relative to the child cell.
 *  For memory performance, a parent instance is not stored as a instance but
 *  rather as a reference to a child instance and a reference to the cell which
 *  is the parent.
 *  The parent instance itself is computed on the fly. It is representative for
 *  a set of instances belonging to the same cell index. The special parent instance
 *  iterator takes care of producing the right sequence.
 */

class DB_PUBLIC ParentInst
{
public: 
  /**
   *  @brief The default ctor
   *
   *  This creates an invalid instance.
   */
  ParentInst ()
  {
    m_parent_cell_index = (cell_index_type) -1;
    m_index = 0;
  }

  /**
   *  @brief Create a parent instance using the parent cell and instance
   */
  ParentInst (cell_index_type parent_cell_index, size_t index)
    : m_parent_cell_index (parent_cell_index), m_index (index)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Retrieve the reference to the parent cell 
   */
  cell_index_type parent_cell_index () const
  {
    return m_parent_cell_index;
  }

  /**
   *  @brief Access to the item index
   *
   *  This method is used mainly by the parent instance iterator
   */
  size_t index () const
  {
    return m_index;
  }

  /**
   *  @brief Equality
   */
  bool operator== (const ParentInst &d) const
  {
    return m_parent_cell_index == d.m_parent_cell_index && m_index == d.m_index;
  }

protected:
  cell_index_type m_parent_cell_index;
  size_t m_index;
};

/**
 *  @brief A parent instance representation
 *
 *  This object represents a parent instance that can be dereferenced standalone.
 *  For this, it requires a "Layout" object pointer for dereferencing.
 */

class DB_PUBLIC ParentInstRep
  : public ParentInst
{
public: 
  typedef db::Layout layout_type;
  typedef db::Cell cell_type;
  typedef db::Box box_type;
  typedef box_type::coord_type coord_type;
  typedef db::CellInst cell_inst_type;
  typedef db::CellInstArray basic_inst_type;
  typedef basic_inst_type cell_inst_array_type;
  typedef instance_iterator<NormalInstanceIteratorTraits> const_iterator;

  /**
   *  @brief The default ctor
   */
  ParentInstRep (const layout_type *layout = 0)
    : ParentInst (), mp_layout (layout)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Create a representation from a ParentInst object
   */
  ParentInstRep &operator= (const ParentInst &pi)
  {
    ParentInst::operator= (pi);
    return *this;
  }

  /*
   *  @brief Retrieve the child instance associated with this parent instance
   *
   *  Return an iterator pointing to the respective child instance.
   */
  Instance child_inst () const;

  /*
   *  @brief Retrieve the child instance associated with this parent instance
   *
   *  Return an pointer to the respective child instance (just the basic object).
   */
  const basic_inst_type *basic_child_inst () const;

  /**
   *  @brief Compute the inverse instance by which the parent is seen from the child
   */
  cell_inst_array_type inst () const;

  /**
   *  @brief Increment the index
   */
  void inc ()
  {
    ++this->m_index;
  }

private:
  const layout_type *mp_layout;
};

/**
 *  @brief A parent instance iterator
 *
 *  The iterator is first an iterator over a vector of parent instances but then
 *  takes care of decomposing a single instance into a set of such.
 */

class DB_PUBLIC ParentInstIterator
{
public: 
  typedef db::Layout layout_type;
  typedef ParentInst parent_inst_type;
  typedef db::Cell cell_type;
  typedef ParentInstRep value_type;
  typedef const value_type *pointer; 
  typedef value_type reference;   //  operator* returns a value
  typedef std::forward_iterator_tag iterator_category;
  typedef void difference_type;

  /**
   *  @brief Default ctor
   */
  ParentInstIterator ()
    : mp_layout (0), m_iter (), m_end ()
  { }

  /**
   *  @brief Constructor
   *
   *  This creates an iterator pointing to the start of the sequence represented
   *  by the parent instance object at *iter or at the end of the parent instance
   *  vector.
   */
  ParentInstIterator (const layout_type *layout, tl::vector<parent_inst_type>::const_iterator iter, tl::vector<parent_inst_type>::const_iterator end)
    : mp_layout (layout), m_iter (iter), m_end (end), m_rep (layout)
  {
    if (m_iter != m_end) {
      m_rep = parent_inst_type (m_iter->parent_cell_index (), m_iter->index ());
    }
  }

  /** 
   *  @brief Equality operator
   */
  bool operator== (const ParentInstIterator &b) const
  {
    return m_iter == b.m_iter && m_rep == b.m_rep;
  }

  /** 
   *  @brief Inequality operator
   */
  bool operator!= (const ParentInstIterator &b) const
  {
    return !operator==(b);
  }

  /**
   *  @brief Increment operator
   */
  ParentInstIterator &operator++();

  /**
   *  @brief Access to the parent instance
   *
   *  In order not to return a reference to the temporary instance, the reference is returned by value.
   *  This is important for the GSI binding since this way, a copy is created which is required when
   *  the iterator moves on.
   */
  value_type operator* () const
  {
    return m_rep;
  }

  /**
   *  @brief Access to the parent instance
   * 
   *  See above.
   */
  const value_type *operator-> () const
  {
    return &m_rep;
  }

  /**
   *  @brief Returns true, if the iterator is at the end
   */
  bool at_end () const
  {
    return m_iter == m_end;
  }
  
private:
  const layout_type *mp_layout;
  tl::vector<parent_inst_type>::const_iterator m_iter, m_end;
  value_type m_rep;
};

/**
 *  @brief A parent cell iterator
 *
 *  The iterator will report just cell indices, not instances.
 */

class DB_PUBLIC ParentCellIterator
{
public: 
  typedef db::Layout layout_type;
  typedef ParentInst parent_inst_type;
  typedef cell_index_type value_type;
  typedef void pointer;           //  no operator->
  typedef value_type reference;   //  operator* returns a value
  typedef std::forward_iterator_tag iterator_category;
  typedef void difference_type;

  /**
   *  @brief Default ctor
   */
  ParentCellIterator ()
    : m_iter ()
  { }

  /**
   *  @brief Constructor
   */
  ParentCellIterator (tl::vector<parent_inst_type>::const_iterator iter)
    : m_iter (iter)
  { }

  /** 
   *  @brief Equality operator
   */
  bool operator== (const ParentCellIterator &b) const
  {
    return m_iter == b.m_iter;
  }

  /** 
   *  @brief Inequality operator
   */
  bool operator!= (const ParentCellIterator &b) const
  {
    return m_iter != b.m_iter;
  }

  /**
   *  @brief Access to the actual instance
   *
   *  The actual instance is created on the fly by the update_rep method.
   *  This will create an artificial instance which takes into account the
   *  offset from the representative.
   */
  cell_index_type operator* () const
  {
    return m_iter->parent_cell_index ();
  }

  /**
   *  @brief Increment operator
   */
  ParentCellIterator &operator++() 
  {
    ++m_iter;
    return *this;
  }

private:
  tl::vector<parent_inst_type>::const_iterator m_iter;
};

/**
 *  @brief A general instance list
 *
 *  This list encapsulates a couple of instances of various types.
 *  The generic iterators are used to access these
 *
 *  Internally, the instance list uses a box tree which treats empty cells as single point boxes when a all-layer search is performed.
 *  That allows retrieving such instances with a search.
 */

class DB_PUBLIC Instances 
{
public:
  typedef db::Layout layout_type;
  typedef db::Cell cell_type;
  typedef db::Box box_type;
  typedef box_type::coord_type coord_type;
  typedef db::CellInst cell_inst_type;
  typedef db::CellInstArray basic_inst_type;
  typedef basic_inst_type cell_inst_array_type;
  typedef db::object_with_properties<cell_inst_array_type> cell_inst_wp_array_type;
  //  the second parameter for box_convert tells it to use a single point for empty cells.
  typedef db::box_convert<cell_inst_array_type, false> cell_inst_array_box_converter;
  typedef db::box_convert<cell_inst_wp_array_type, false> cell_inst_wp_array_box_converter;
  typedef db::box_tree<box_type, cell_inst_array_type, cell_inst_array_box_converter> stable_cell_inst_tree_type;
  typedef db::box_tree<box_type, cell_inst_wp_array_type, cell_inst_wp_array_box_converter> stable_cell_inst_wp_tree_type;
  typedef db::unstable_box_tree<box_type, cell_inst_array_type, cell_inst_array_box_converter> cell_inst_tree_type;
  typedef db::unstable_box_tree<box_type, cell_inst_wp_array_type, cell_inst_wp_array_box_converter> cell_inst_wp_tree_type;
  typedef instance_iterator<NormalInstanceIteratorTraits> const_iterator;
  typedef instance_iterator<OverlappingInstanceIteratorTraits> overlapping_iterator;
  typedef instance_iterator<TouchingInstanceIteratorTraits> touching_iterator;
  typedef db::ParentInst parent_inst_type;
  typedef db::ParentInstIterator parent_inst_iterator;
  typedef db::ParentCellIterator parent_cell_iterator;
  typedef db::ChildCellIterator child_cell_iterator;
  typedef tl::vector<parent_inst_type> parent_inst_vector;
  typedef tl::vector<const basic_inst_type *> sorted_inst_vector;
  typedef sorted_inst_vector::const_iterator sorted_inst_iterator;
  typedef db::Instance instance_type;

  /**
   *  @brief Default constructor
   */
  Instances (cell_type *cell);

  /**
   *  @brief The destructor
   */
  ~Instances ();

  /**
   *  @brief The assignment operator 
   *
   *  Hint: the current implementation does not translate to other array repositories.
   *  This implies, that cell instance arrays must not be based on array repositories currently.
   *  The current implementation does not translate property Id's. 
   *  It is mainly intended for 1-to-1 copies of layouts where the whole
   *  property repository is copied.
   */
  Instances &operator= (const Instances &d);

  /**
   *  @brief Clear the instance list
   */
  void clear_insts ();

  /**
   *  @brief Clear all normal instances
   */
  void clear (Instances::cell_inst_array_type::tag);

  /**
   *  @brief Clear all instances with properties
   */
  void clear (Instances::cell_inst_wp_array_type::tag);

  /**
   *  @brief Empty predicate - if no instances are given, this predicate is true
   */
  bool empty () const;

  /** 
   *  @brief Erase a cell instance given by a instance proxy
   *
   *  Erasing a cell instance will destroy the sorting order and invalidate
   *  other instance proxies.
   *  sort() must be called before a region query can be done
   *  on the cell instances. 
   */
  void erase (const instance_type &ref);

  /**
   *  @brief Erase a cell instance
   *  
   *  Erasing a cell instance will destroy the sorting order.
   *  sort() must be called before a region query can be done
   *  on the cell instances. 
   */
  void erase (const const_iterator &e);

  /**
   *  @brief Erasing of multiple instances (editable mode)
   *
   *  Erase a set of positions given by an iterator I: *(from,to).
   *  *I must render an "iterator" object.
   *  The iterators in the sequence from, to must be sorted in
   *  "operator<" order.
   *
   *  @param first The start of the sequence of iterators
   *  @param last The end of the sequence of iterators
   */
  void erase_insts (const std::vector<instance_type> &instances);

  /**
   *  @brief Erasing of multiple elements (editable mode)
   *
   *  Erase a set of positions given by an iterator I: *(from,to).
   *  *I must render an "iterator" object.
   *  The iterators in the sequence from, to must be sorted in
   *  "operator<" order.
   *  This method is mainly provided for optimization of the "undo" replay method.
   *
   *  @param tag The specific instance type's tag
   *  @param first The start of the sequence of iterators
   *  @param last The end of the sequence of iterators
   */
  template <class Tag, class ET, class I>
  void erase_positions (Tag tag, ET et, I first, I last);

  /**
   *  @brief Insert a cell instance 
   *  
   *  Inserting a cell instance will destroy the sorting order.
   *  sort() must be called before a region query can be done
   *  on the cell instances.
   *  The reference returned is not stable, i.e. on further
   *  insert operations, this reference may become invalid.
   */
  template <class InstArray>
  instance_type insert (const InstArray &inst);

  /**
   *  @brief Insert a sequence [from,to) of cell instances in a "editable safe" way
   *  
   *  @param from The start of the sequence
   *  @param to The past-the-end pointer of the sequence
   */
  template <class I>
  void insert (I from, I to);

  /**
   *  @brief Insert an instance given by a instance reference 
   *
   *  This member may be used to copy an instance from one cell to another.
   *  Because of the inherent instability of the instance pointer it must not be 
   *  used to copy instances within one cell.
   *  Only in editable mode, this method will return a stable reference - that is
   *  one that will not be invalidated potentially on further insert operations.
   *
   *  @param instance The instance reference of which to insert a copy
   */
  instance_type insert (const instance_type &ref)
  {
    tl::ident_map<cell_index_type> im;
    tl::ident_map<db::properties_id_type> pm;
    return insert (ref, im, pm);
  }

  /**
   *  @brief Insert an instance given by a instance reference with a different cell index and property ID
   *
   *  This member may be used to map an instance to another layout object. 
   *  Only in editable mode, this method will return a stable reference - that is
   *  one that will not be invalidated potentially on further insert operations.
   *
   *  @param ref The instance reference of which to insert a copy
   *  @param im The mapper to new cell index to use (for mapping to a different layout for example)
   *  @param pm The mapper to new cell property ID use (for mapping to a different layout for example)
   */
  template <class IndexMap, class PropIdMap>
  instance_type insert (const instance_type &ref, IndexMap &im, PropIdMap &pm)
  {
    tl::func_delegate <IndexMap, db::cell_index_type> im_delegate (im);
    tl::func_delegate <PropIdMap, db::properties_id_type> pm_delegate (pm);
    return do_insert (ref, im_delegate, pm_delegate);
  }

  /** 
   *  @brief Replace the properties ID of an element (pointed to by the iterator) with the given one
   *
   *  This method will delete the former object if it does not have a property and needs to get one.
   *  It will return a reference to the new object. The erase operation will potentially invalidate
   *  other references in non-editable mode!
   *
   *  @return The reference to the new instance
   */
  instance_type replace_prop_id (const instance_type &ref, db::properties_id_type prop_id);

  /**
   *  @brief Replace the instance pointed to by the iterator with the given instance
   *
   *  If the object pointed to by ref contains properties, the properties will remain.
   *
   *  @return The reference to the new object
   */
  instance_type replace (const instance_type &ref, const cell_inst_array_type &inst);

  /**
   *  @brief Replace the instance pointed to by the iterator with the given instance with properties
   *
   *  @return The reference to the new object
   */
  instance_type replace (const instance_type &ref, const cell_inst_wp_array_type &inst);

  /**
   *  @brief Test if the given reference is valid
   *  
   *  Returns true, if the given instance is valid. It is not valid, if it has been 
   *  deleted already. However, it may happen, that the instance memory has been 
   *  reused already. Therefore this method can safely be used only if nothing 
   *  has been inserted into this container in between.
   *  This method can only be used in editable mode.
   *
   *  @param instance The reference to the instance
   */
  bool is_valid (const instance_type &ref) const;

  /**
   *  @brief Transform the instance pointed to by the instance reference
   *
   *  @param t The transformation to apply
   */
  template <class Trans>
  instance_type transform (const instance_type &ref, const Trans &t)
  {
    return replace (ref, ref.cell_inst ().transformed (t, 0 /*don't consider array repository*/));
  }

  /**
   *  @brief Transforms the given instance into a new system
   *
   *  This method transforms the instance while assuming that the transformation will be propagated further.
   *  In order worlds, the instance will be valid in a new coordinate system derived from the current one
   *  with the given transformation.
   *
   *  @param t The transformation to apply
   */
  template <class Trans>
  instance_type transform_into (const instance_type &ref, const Trans &t)
  {
    return replace (ref, ref.cell_inst ().transformed_into (t, 0 /*don't consider array repository*/));
  }

  /**
   *  @brief Transform all instances 
   *
   *  @param t The transformation to apply
   */
  template <class Trans>
  void transform (const Trans &t);

  /**
   *  @brief Transform all instances into a new system
   *
   *  @param t The transformation to apply
   *
   *  See the single-instance "transform_into" method for an explanation of this transformation variant.
   */
  template <class Trans>
  void transform_into (const Trans &t);

  /**
   *  @brief Clear the parent instance list
   *
   *  Reserve the given number of entries.
   */
  void clear_parent_insts (size_t sz = 0)
  {
    m_parent_insts.clear ();
    m_parent_insts.reserve (sz);
  }

  /**
   *  @brief Count the number of parent instances
   * 
   *  This will accumulate the number of parent instances in the vector
   *  provided. Later, this count is used to resize the parent instance
   *  vector.
   */
  void count_parent_insts (std::vector <size_t> &count) const;

  /**
   *  @brief Establish the instance index list giving the instances by cell index
   *  If force is true, the instance tree is always sorted.
   */
  void sort_child_insts (bool force);

  /**
   *  @brief Sort the cell instance list
   *
   *  This will sort the cell instance list (quad tree sort). As a prerequesite
   *  the cell's bounding boxes must have been computed.
   *  If force is true, the instance tree is always sorted.
   */
  void sort_inst_tree (const layout_type *g, bool force);

  /**
   *  @brief Update the child-parent relationships
   * 
   *  This will update the child-parent relationships. Basically
   *  this means entering the cell as a parent into all it's child
   *  cells.
   */
  void update_relations (layout_type *g, cell_index_type cell_index);

  /**
   *  @brief Region query for the instances in "overlapping" mode
   *
   *  This will return the region query iterator for the child cell
   *  instances overlapping with the given region b. 
   *
   *  @param b The region to query
   *
   *  @return The region iterator for the child cell instances
   */
  overlapping_iterator begin_overlapping (const box_type &b, const layout_type *g) const
  {
    return overlapping_iterator (OverlappingInstanceIteratorTraits (this, b, g));
  }

  /**
   *  @brief Region query for the instances in "touching" mode
   *
   *  This will return the region query iterator for the child cell
   *  instances touching the given region b. 
   *
   *  @param b The region to query
   *
   *  @return The region iterator for the child cell instances
   */
  touching_iterator begin_touching (const box_type &b, const layout_type *g) const
  {
    return touching_iterator (TouchingInstanceIteratorTraits (this, b, g));
  }

  /**
   *  @brief The child cell iterator
   *
   *  This iterator will report the child cell indices, not every instance.
   *  Basically this is a filter.
   */
  ChildCellIterator begin_child_cells () const
  {
    return ChildCellIterator (this);
  }

  /** 
   *  @brief Report the number of child cells
   *
   *  Just the number of child cells with distinct cell indices are
   *  reported.
   *  CAUTION: this method is SLOW!
   */
  size_t child_cells () const;

  /**
   *  @brief The number of cell instances 
   */
  size_t cell_instances () const;

  /**
   *  @brief The cell instance access begin iterator
   */
  const_iterator begin () const
  {
    return const_iterator (NormalInstanceIteratorTraits (this));
  }

  /**
   *  @brief Deliver an iterator for a given basic_inst_type pointer
   */
  instance_type instance_from_pointer (const basic_inst_type *p) const;

  /**
   *  @brief The iterator delivering the cell instance (indices) sorted by child cell ID
   */
  sorted_inst_iterator begin_sorted_insts () const
  {
    return m_insts_by_cell_index.begin ();
  }

  /**
   *  @brief The iterator delivering the cell instance (indices) sorted by child cell ID (end)
   */
  sorted_inst_iterator end_sorted_insts () const
  {
    return m_insts_by_cell_index.end ();
  }

  /**
   *  @brief The parent instance list begin iterator
   *
   *  The begin_parent_insts() method gives access to the parent instance list.
   */
  ParentInstIterator begin_parent_insts (const layout_type *g) const
  {
    return ParentInstIterator (g, m_parent_insts.begin (), m_parent_insts.end ());
  }

  /**
   *  @brief Report the number of parent cells 
   *
   *  Since the m_parent_insts vector just stores references to those cells
   *  that have distinct cell indices, we can simply report their length.
   */
  size_t parent_cells () const
  {
    return m_parent_insts.size ();
  }

  /**
   *  @brief The parent cell iterator
   * 
   *  This iterator will iterate over the parent cells, just returning their
   *  cell index.
   */
  ParentCellIterator begin_parent_cells () const
  {
    return ParentCellIterator (m_parent_insts.begin ());
  }

  /**
   *  @brief The parent cell end iterator
   */
  ParentCellIterator end_parent_cells () const
  {
    return ParentCellIterator (m_parent_insts.end ());
  }

  /**
   *  @brief Tell if the cell is a top-level cell
   *
   *  A cell is a top-level cell if there are no parent instantiations.
   */
  bool is_top () const
  {
    return m_parent_insts.end () == m_parent_insts.begin ();
  }

  /**
   *  @brief Gets the layout the instances collection lives in
   */
  db::Layout *layout () const;

  /**
   *  @brief Collect memory usage statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const;

  /**
   *  @brief Returns true, if this Instances container belongs to an editable cell
   */
  bool is_editable () const;

  /**
   *  @brief Gets the cell pointer
   */
  cell_type *cell () const
  {
    return reinterpret_cast<cell_type *> (size_t (mp_cell) & ~size_t (3));
  }

  /**
   *  @brief Delegate for the undo method
   */
  void undo (db::Op *op);

  /**
   *  @brief Delegate for the redo method
   */
  void redo (db::Op *op);

private:
  friend class Instance;
  friend struct NormalInstanceIteratorTraits;
  friend struct TouchingInstanceIteratorTraits;
  friend struct OverlappingInstanceIteratorTraits;
  template <class Inst, class ET> friend class InstOp;

  union {
    cell_inst_tree_type *unstable_tree;
    stable_cell_inst_tree_type *stable_tree;
    void *any;
  } m_generic;

  union {
    cell_inst_wp_tree_type *unstable_tree;
    stable_cell_inst_wp_tree_type *stable_tree;
    void *any;
  } m_generic_wp;

  parent_inst_vector m_parent_insts;
  sorted_inst_vector m_insts_by_cell_index;
  cell_type *mp_cell;

  static cell_inst_wp_tree_type ms_empty_wp_tree;
  static cell_inst_tree_type ms_empty_tree;
  static stable_cell_inst_wp_tree_type ms_empty_stable_wp_tree;
  static stable_cell_inst_tree_type ms_empty_stable_tree;

  /**
   *  @brief Sets a flag indicating that the instance tree needs sorting
   */
  void set_instance_tree_needs_sort (bool f)
  {
    mp_cell = reinterpret_cast<cell_type *> ((size_t (mp_cell) & ~size_t (1)) | size_t (f ? 1 : 0));
  }

  /**
   *  @brief Sets a flag indicating that the instance tree needs sorting
   */
  bool instance_tree_needs_sort () const
  {
    return (size_t (mp_cell) & 1) != 0;
  }

  /**
   *  @brief Sets a flag indicating that the instance by cell index cache needs made
   */
  void set_instance_by_cell_index_needs_made (bool f)
  {
    mp_cell = reinterpret_cast<cell_type *> ((size_t (mp_cell) & ~size_t (2)) | size_t (f ? 2 : 0));
  }

  /**
   *  @brief Sets a flag indicating that the instance tree needs sorting
   */
  bool instance_by_cell_index_needs_made () const
  {
    return (size_t (mp_cell) & 2) != 0;
  }

  /**
   *  @brief Invalidates the instance information - called whenever something changes
   */
  void invalidate_insts ();

  /**
   *  @brief Get the non-editable instance tree by instance type
   */
  const cell_inst_wp_tree_type &inst_tree (cell_inst_wp_array_type::tag, InstancesNonEditableTag) const
  {
    tl_assert (! is_editable ());
    return *(m_generic_wp.unstable_tree ? m_generic_wp.unstable_tree : &ms_empty_wp_tree);
  }

  /**
   *  @brief Get the non-editable instance tree by instance type
   */
  const cell_inst_tree_type &inst_tree (cell_inst_array_type::tag, InstancesNonEditableTag) const
  {
    tl_assert (! is_editable ());
    return *(m_generic.unstable_tree ? m_generic.unstable_tree : &ms_empty_tree);
  }

  /**
   *  @brief Get the editable instance tree by instance type
   */
  const stable_cell_inst_wp_tree_type &inst_tree (cell_inst_wp_array_type::tag, InstancesEditableTag) const
  {
    tl_assert (is_editable ());
    return *(m_generic_wp.stable_tree ? m_generic_wp.stable_tree : &ms_empty_stable_wp_tree);
  }

  /**
   *  @brief Get the editable instance tree by instance type
   */
  const stable_cell_inst_tree_type &inst_tree (cell_inst_array_type::tag, InstancesEditableTag) const
  {
    tl_assert (is_editable ());
    return *(m_generic.stable_tree ? m_generic.stable_tree : &ms_empty_stable_tree);
  }

  /**
   *  @brief Gets the non-editable, non-const instance tree by instance type
   */
  cell_inst_wp_tree_type &inst_tree (cell_inst_wp_array_type::tag, InstancesNonEditableTag) 
  {
    tl_assert (! is_editable ());
    if (! m_generic_wp.unstable_tree) {
      m_generic_wp.unstable_tree = new cell_inst_wp_tree_type ();
    }
    return *m_generic_wp.unstable_tree;
  }

  /**
   *  @brief Gets the non-editable, non-const instance tree by instance type
   */
  cell_inst_tree_type &inst_tree (cell_inst_array_type::tag, InstancesNonEditableTag) 
  {
    tl_assert (! is_editable ());
    if (! m_generic.unstable_tree) {
      m_generic.unstable_tree = new cell_inst_tree_type ();
    }
    return *m_generic.unstable_tree;
  }

  /**
   *  @brief Gets the editable, non-const instance tree by instance type
   */
  stable_cell_inst_wp_tree_type &inst_tree (cell_inst_wp_array_type::tag, InstancesEditableTag) 
  {
    tl_assert (is_editable ());
    if (! m_generic_wp.stable_tree) {
      m_generic_wp.stable_tree = new stable_cell_inst_wp_tree_type ();
    }
    return *m_generic_wp.stable_tree;
  }

  /**
   *  @brief Gets the editable, non-const instance tree by instance type
   */
  stable_cell_inst_tree_type &inst_tree (cell_inst_array_type::tag, InstancesEditableTag) 
  {
    tl_assert (is_editable ());
    if (! m_generic.stable_tree) {
      m_generic.stable_tree = new stable_cell_inst_tree_type ();
    }
    return *m_generic.stable_tree;
  }

  //  no copy ctor
  Instances (const Instances &d);

  instance_type do_insert (const instance_type &ref, 
                           tl::func_delegate_base <db::cell_index_type> &im,
                           tl::func_delegate_base <db::properties_id_type> &pm);

  void do_clear_insts ();

  /**
   *  @brief Apply an unary function
   */
  template <class Op, class ET>
  void apply_op (const Op &op, ET editable_tag);

  /**
   *  @brief Replace an instance given by an address (without conversion)
   */
  template <class InstArray>
  void replace (const InstArray *replace, const InstArray &with);

  template <class Tag>
  bool is_valid_by_tag (Tag tag, const instance_type &ref) const;

  template <class Tag>
  void erase_inst_by_tag (Tag tag, const const_iterator &e)
  {
    if (is_editable ()) {
      erase_inst_by_iter (tag, InstancesEditableTag (), *(e->basic_iter (tag)));
    } else {
      erase_inst_by_tag (tag, InstancesNonEditableTag (), *(e->basic_ptr (tag)));
    }
  }

  template <class Tag>
  void erase_inst_by_tag (Tag tag, const instance_type &ref)
  {
    if (is_editable ()) {
      erase_inst_by_iter (tag, InstancesEditableTag (), *(ref.basic_iter (tag)));
    } else {
      erase_inst_by_tag (tag, InstancesNonEditableTag (), *(ref.basic_ptr (tag)));
    }
  }

  template <class Tag, class ET, class I>
  void erase_inst_by_iter (Tag tag, ET editable_tag, I iter);

  template <class Tag, class ET>
  void erase_inst_by_tag (Tag tag, ET editable_tag, const typename Tag::object_type &obj);

  template <class Tag, class ET>
  void erase_insts_by_tag (Tag tag, ET editable_tag, std::vector<instance_type>::const_iterator s1, std::vector<instance_type>::const_iterator s2);

  template <class I, class ET>
  void insert (I from, I to);

  template <class ET>
  void clear_insts (ET editable_tag);
};

/**
 *  @brief Collect memory statistics
 */
inline void
mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const Instances &x, bool no_self, void *parent)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

template <class Iter>
inline NormalInstanceIteratorTraits::instance_type 
NormalInstanceIteratorTraits::instance_from_stable_iter (const Iter &iter) const
{
  //  box tree flat iterators deliver pointers, not iterators. Use instance_from_pointer to do this conversion.
  return mp_insts->instance_from_pointer (&*iter);
}
 
template <class Iter>
inline TouchingInstanceIteratorTraits::instance_type 
TouchingInstanceIteratorTraits::instance_from_stable_iter (const Iter &iter) const
{
  //  box tree iterators deliver pointers, not iterators. Use instance_from_pointer to do this conversion.
  return mp_insts->instance_from_pointer (&*iter);
}
 
template <class Iter>
inline OverlappingInstanceIteratorTraits::instance_type 
OverlappingInstanceIteratorTraits::instance_from_stable_iter (const Iter &iter) const
{
  //  box tree iterators deliver pointers, not iterators. Use instance_from_pointer to do this conversion.
  return mp_insts->instance_from_pointer (&*iter);
}
 
}

#endif

