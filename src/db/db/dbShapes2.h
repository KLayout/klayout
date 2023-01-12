
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


#ifndef HDR_dbShapes2
#define HDR_dbShapes2

#include "dbShapes.h"
#include "tlUtils.h"

namespace db 
{

/**
 *  @brief A traits class for the various shape types
 *
 *  This class provides information about various properties of the shape class, i.e.
 *  - whether the objects must be dereferenced into standanlone containers
 *  - whether the objects are arrays, potentially using the array repository
 *  - whether the objects have properties
 */
template <class Sh> 
struct shape_traits 
{
  typedef tl::False can_deref;
  typedef tl::False is_array;
  typedef tl::False has_properties;
};

/**
 *  @brief The traits implementation for objects with properties
 */
template <class InnerSh>
struct shape_traits<db::object_with_properties<InnerSh> >
{
  typedef shape_traits<InnerSh> inner_traits;
  typedef typename inner_traits::can_deref can_deref;
  typedef typename inner_traits::is_array is_array;
  typedef tl::True has_properties;
};

/**
 *  @brief The traits implementation for arrays
 */
template <class InnerSh, class ATrans>
struct shape_traits<db::array<InnerSh, ATrans> >
{
  typedef shape_traits<InnerSh> inner_traits;
  typedef typename inner_traits::can_deref can_deref;
  typedef tl::True is_array;
  typedef tl::False has_properties;
};

/**
 *  @brief The traits implementation for shape references
 */
template <class InnerSh, class RTrans>
struct shape_traits<db::shape_ref<InnerSh, RTrans> >
{
  typedef tl::True can_deref;
  typedef tl::False is_array;
  typedef tl::False has_properties;
};

/**
 *  @brief Actual implementation of the LayerBase class
 *  
 *  For a certain shape type this template implements the 
 *  shape specific layer methods.
 */

template <class Sh, class StableTag>
class layer_class 
  : public LayerBase
{
public:
  typedef db::layer<Sh, StableTag> layer_type;
  typedef LayerBase::box_type box_type;
  typedef LayerBase::coord_type coord_type;

  layer_class ()
    : LayerBase (), m_layer () 
  { }

  layer_type &layer () 
  {
    return m_layer;
  }

  const layer_type &layer () const
  {
    return m_layer;
  }

  virtual box_type bbox () const 
  {
    return m_layer.bbox ();
  }

  virtual void update_bbox () 
  {
    m_layer.update_bbox ();
  }

  virtual bool is_bbox_dirty () const
  {
    return m_layer.is_bbox_dirty ();
  }

  virtual bool is_tree_dirty () const
  {
    return m_layer.is_tree_dirty ();
  }

  size_t size () const
  {
    return m_layer.size ();
  }

  bool empty () const
  {
    return m_layer.empty ();
  }

  virtual void sort () 
  {
    m_layer.sort ();
  }

  virtual bool is_same_type (const LayerBase *other) const
  {
    return dynamic_cast<const layer_class<Sh, StableTag> *> (other);
  }

  virtual LayerBase *clone () const;
  virtual void translate_into (Shapes *target, GenericRepository &rep, ArrayRepository &array_rep) const;
  virtual void translate_into (Shapes *target, GenericRepository &rep, ArrayRepository &array_rep, pm_delegate_type &pm) const;
  virtual void transform_into (Shapes *target, const Trans &trans, GenericRepository &rep, ArrayRepository &array_rep) const;
  virtual void transform_into (Shapes *target, const Trans &trans, GenericRepository &rep, ArrayRepository &array_rep, pm_delegate_type &pm) const;
  virtual void transform_into (Shapes *target, const ICplxTrans &trans, GenericRepository &rep, ArrayRepository &array_rep) const;
  virtual void transform_into (Shapes *target, const ICplxTrans &trans, GenericRepository &rep, ArrayRepository &array_rep, pm_delegate_type &pm) const;
  virtual void insert_into (Shapes *target);
  virtual void deref_into (Shapes *target);
  virtual void deref_into (Shapes *target, pm_delegate_type &pm);
  virtual void deref_and_transform_into (Shapes *target, const Trans &trans);
  virtual void deref_and_transform_into (Shapes *target, const Trans &trans, pm_delegate_type &pm);
  virtual void deref_and_transform_into (Shapes *target, const ICplxTrans &trans);
  virtual void deref_and_transform_into (Shapes *target, const ICplxTrans &trans, pm_delegate_type &pm);

  virtual void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
  {
    db::mem_stat (stat, purpose, cat, m_layer, no_self, parent);
  }

  unsigned int type_mask () const;

private:
  layer_type m_layer;
};

template <class Sh, class Stable>
void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const layer_class<Sh, Stable> &x, bool no_self, void *parent)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

}

#endif

