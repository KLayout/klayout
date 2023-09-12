
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


#include "dbInstances.h"
#include "dbLayout.h"

namespace db
{

Instances::cell_inst_wp_tree_type        Instances::ms_empty_wp_tree        = Instances::cell_inst_wp_tree_type ();
Instances::cell_inst_tree_type           Instances::ms_empty_tree           = Instances::cell_inst_tree_type ();
Instances::stable_cell_inst_wp_tree_type Instances::ms_empty_stable_wp_tree = Instances::stable_cell_inst_wp_tree_type ();
Instances::stable_cell_inst_tree_type    Instances::ms_empty_stable_tree    = Instances::stable_cell_inst_tree_type ();

// -------------------------------------------------------------------------------------
//  local classes

/**
 *  @brief A base class for instance operations 
 *
 *  This class is used for the Op classes for the undo/redo queuing mechanism.
 */
class InstOpBase
  : public db::Op
{
public:
  InstOpBase () : db::Op () { }

  virtual void undo (Instances *instances) = 0;
  virtual void redo (Instances *instances) = 0;
};

/**
 *  @brief A undo/redo queue object for the instances
 *
 *  This class is used internally to queue an insert or erase operation
 *  into the db::Object manager's undo/redo queue.
 */
template <class Inst, class ET>
class InstOp
  : public InstOpBase
{
public:
  typedef db::Box box_type;
  typedef db::box_convert<Inst> box_convert_type;
  typedef db::box_tree<box_type, Inst, box_convert_type> stable_tree_type;
  typedef db::unstable_box_tree<box_type, Inst, box_convert_type> tree_type;

  InstOp (bool insert, const Inst &sh)
    : m_insert (insert)
  {
    m_insts.push_back (sh);
  }
  
  template <class Iter>
  InstOp (bool insert, Iter from, Iter to)
    : m_insert (insert)
  {
    size_t n = 0;
    for (Iter i = from; i != to; ++i) {
      ++n;
    }
    m_insts.reserve (n);
    for (Iter i = from; i != to; ++i) {
      m_insts.push_back (*i);
    }
  }

  template <class Iter>
  InstOp (bool insert, Iter from, Iter to, bool /*dummy*/)
    : m_insert (insert)
  {
    m_insts.reserve (std::distance (from, to));
    for (Iter i = from; i != to; ++i) {
      m_insts.push_back (**i);
    }
  }

  virtual void undo (Instances *insts)
  {
    if (m_insert) {
      erase (insts);
    } else {
      insert (insts);
    }
  }

  virtual void redo (Instances *insts)
  {
    if (m_insert) {
      insert (insts);
    } else {
      erase (insts);
    }
  }

private:
  bool m_insert;
  std::vector<Inst> m_insts;

  void insert (Instances *insts);
  void erase (Instances *insts);
};

template <class Inst, class ET> 
void 
InstOp<Inst, ET>::insert (Instances *insts)
{
  insts->insert (m_insts.begin (), m_insts.end ());
}

template <class Inst, class ET> 
void 
InstOp<Inst, ET>::erase (Instances *insts)
{
  typedef typename instances_editable_traits<ET>::template tree_traits<typename Inst::tag>::tree_type tree_type;

  if (((const Instances *) insts)->inst_tree (typename Inst::tag (), ET ()).size () <= m_insts.size ()) {

    //  If all shapes are to be removed, just clear the instances
    insts->clear (typename Inst::tag ());

  } else {

    std::sort (m_insts.begin (), m_insts.end ());

    std::vector<bool> done;
    done.resize (m_insts.size (), false);

    //  This is not quite effective but seems to be the simplest way
    //  of implementing this: search for each element and erase these.
    //  The alternative would be to store the iterator along with the object.
    std::vector<typename tree_type::const_iterator> to_erase;
    to_erase.reserve (m_insts.size ());
    for (typename tree_type::const_iterator linst = ((const Instances *) insts)->inst_tree (typename Inst::tag (), ET ()).begin (); linst != ((const Instances *) insts)->inst_tree (typename Inst::tag (), ET ()).end (); ++linst) {
      typename std::vector<Inst>::iterator i = std::lower_bound (m_insts.begin (), m_insts.end (), *linst);
      while (i != m_insts.end () && done [std::distance(m_insts.begin (), i)] && *i == *linst) {
        ++i;
      }
      if (i != m_insts.end () && *i == *linst) {
        done [std::distance(m_insts.begin (), i)] = true;
        to_erase.push_back (linst);
      }
    }

    insts->erase_positions (typename Inst::tag (), ET (), to_erase.begin (), to_erase.end ());

  }
}

// -------------------------------------------------------------------------------------
//  ParentInstRep implementation

Instance
ParentInstRep::child_inst () const
{
  return mp_layout->cell (this->m_parent_cell_index).sorted_inst_ptr (this->m_index);
}

const ParentInstRep::basic_inst_type *
ParentInstRep::basic_child_inst () const
{
  return mp_layout->cell (this->m_parent_cell_index).basic_sorted_inst_ptr (this->m_index);
}

ParentInstRep::cell_inst_array_type  
ParentInstRep::inst () const
{
  //  create a new parent instance by cloning and inverting the array
  basic_inst_type ci (*basic_child_inst ());
  ci.object () = cell_inst_type (this->m_parent_cell_index);
  ci.invert ();
  return ci;
}

// -------------------------------------------------------------------------------------
//  ParentInstIterator implementation

ParentInstIterator &
ParentInstIterator::operator++() 
{
  cell_index_type ci = m_rep.basic_child_inst ()->object ().cell_index ();
  m_rep.inc ();

  if (m_rep.index () == mp_layout->cell (m_rep.parent_cell_index ()).cell_instances () ||
      m_rep.basic_child_inst ()->object ().cell_index () != ci) {
    ++m_iter;
    if (m_iter != m_end) {
      m_rep = parent_inst_type (m_iter->parent_cell_index (), m_iter->index ());
    } else {
      m_rep = parent_inst_type ();
    }
  }

  return *this;
}

// -------------------------------------------------------------------------------------
//  Instance implementation

Instance::cell_inst_array_type::iterator 
Instance::begin_touching (const cell_inst_array_type::box_type &b, const layout_type *g) const 
{
  db::box_convert<cell_inst_type> bc (*g);
  return cell_inst ().begin_touching (b, bc);
}

std::string
Instance::to_string (bool resolve_cell_name) const
{
  if (is_null ()) {
    return std::string ();
  }

  const cell_inst_array_type &ci = cell_inst ();

  std::string r;
  if (resolve_cell_name && mp_instances && mp_instances->cell () && mp_instances->cell ()->layout ()) {
    r = mp_instances->cell ()->layout ()->cell_name (ci.object ().cell_index ());
  } else {
    r = "cell_index=" + tl::to_string (ci.object ().cell_index ());
  }

  db::vector<coord_type> a, b;
  unsigned long amax, bmax;
  if (ci.is_regular_array (a, b, amax, bmax)) {

    if (ci.is_complex ()) {
      r += " " + ci.complex_trans ().to_string ();
    } else {
      r += " " + (*ci.begin ()).to_string ();
    }

    r += " array=(" + a.to_string () + "," + b.to_string () + " " + tl::to_string (amax) + "x" + tl::to_string (bmax) + ")";

  } else {

    for (db::CellInstArray::iterator i = ci.begin (); ! i.at_end (); ++i) {
      if (ci.is_complex ()) {
        r += " " + ci.complex_trans (*i).to_string ();
      } else {
        r += " " + (*i).to_string ();
      }
    }

  }

  if (has_prop_id ()) {
    r += " prop_id=" + tl::to_string (prop_id ());
  }

  return r;
}

// -------------------------------------------------------------------------------------
//  instance_iterator implementation

template <class Traits>
void
instance_iterator<Traits>::release_iter ()
{ 
  if (m_type == TInstance) {
    if (m_stable) {
      if (m_with_props) {
        basic_iter (cell_inst_wp_array_type::tag (), InstancesEditableTag ()).~stable_iter_wp_type ();
      } else {
        basic_iter (cell_inst_array_type::tag (), InstancesEditableTag ()).~stable_iter_type ();
      }
    } else {
      if (m_with_props) {
        basic_iter (cell_inst_wp_array_type::tag (), InstancesNonEditableTag ()).~iter_wp_type ();
      } else {
        basic_iter (cell_inst_array_type::tag (), InstancesNonEditableTag ()).~iter_type ();
      }
    }
  }
}

template <class Traits>
void
instance_iterator<Traits>::make_iter ()
{ 
  if (m_type == TInstance) {
    if (m_stable) {
      if (m_with_props) {
        new (&basic_iter (cell_inst_wp_array_type::tag (), InstancesEditableTag ())) stable_iter_wp_type ();
      } else {
        new (&basic_iter (cell_inst_array_type::tag (), InstancesEditableTag ())) stable_iter_type ();
      }
    } else {
      if (m_with_props) {
        new (&basic_iter (cell_inst_wp_array_type::tag (), InstancesNonEditableTag ())) iter_wp_type ();
      } else {
        new (&basic_iter (cell_inst_array_type::tag (), InstancesNonEditableTag ())) iter_type ();
      }
    }
    m_traits.init (this); 
  }
}

template <class Traits>
bool
instance_iterator<Traits>::operator== (const instance_iterator<Traits> &d) const
{
  if (! (m_type == d.m_type && m_stable == d.m_stable && m_with_props == d.m_with_props)) {
    return false;
  }
  if (m_type == TNull) {
    return true;
  } else {
    if (m_stable) {
      if (m_with_props) {
        return (basic_iter (cell_inst_wp_array_type::tag (), InstancesEditableTag ()) == d.basic_iter (cell_inst_wp_array_type::tag (), InstancesEditableTag ()));
      } else {
        return (basic_iter (cell_inst_array_type::tag (), InstancesEditableTag ()) == d.basic_iter (cell_inst_array_type::tag (), InstancesEditableTag ()));
      }
    } else {
      if (m_with_props) {
        return (basic_iter (cell_inst_wp_array_type::tag (), InstancesNonEditableTag ()) == d.basic_iter (cell_inst_wp_array_type::tag (), InstancesNonEditableTag ()));
      } else {
        return (basic_iter (cell_inst_array_type::tag (), InstancesNonEditableTag ()) == d.basic_iter (cell_inst_array_type::tag (), InstancesNonEditableTag ()));
      }
    }
  }
}

template <class Traits>
instance_iterator<Traits> &
instance_iterator<Traits>::operator= (const instance_iterator<Traits> &iter)
{
  if (this != &iter) {

    release_iter ();

    m_type = iter.m_type;
    m_stable = iter.m_stable;
    m_with_props = iter.m_with_props;
    m_traits = iter.m_traits;

    if (m_type == TInstance) {

      if (m_stable) {
        if (m_with_props) {
          new (&basic_iter (cell_inst_wp_array_type::tag (), InstancesEditableTag ())) stable_iter_wp_type (iter.basic_iter (cell_inst_wp_array_type::tag (), InstancesEditableTag ()));
        } else {
          new (&basic_iter (cell_inst_array_type::tag (), InstancesEditableTag ())) stable_iter_type (iter.basic_iter (cell_inst_array_type::tag (), InstancesEditableTag ()));
        }
      } else {
        if (m_with_props) {
          new (&basic_iter (cell_inst_wp_array_type::tag (), InstancesNonEditableTag ())) iter_wp_type (iter.basic_iter (cell_inst_wp_array_type::tag (), InstancesNonEditableTag ()));
        } else {
          new (&basic_iter (cell_inst_array_type::tag (), InstancesNonEditableTag ())) iter_type (iter.basic_iter (cell_inst_array_type::tag (), InstancesNonEditableTag ()));
        }
      }

      update_ref ();

    }

  }

  return *this;
}

template <class Traits>
db::Box
instance_iterator<Traits>::quad_box () const
{
  if (m_type == TInstance) {
    if (m_stable) {
      if (m_with_props) {
        return m_traits.quad_box (basic_iter (cell_inst_wp_array_type::tag (), InstancesEditableTag ()));
      } else {
        return m_traits.quad_box (basic_iter (cell_inst_array_type::tag (), InstancesEditableTag ()));
      }
    } else {
      if (m_with_props) {
        return m_traits.quad_box (basic_iter (cell_inst_wp_array_type::tag (), InstancesNonEditableTag ()));
      } else {
        return m_traits.quad_box (basic_iter (cell_inst_array_type::tag (), InstancesNonEditableTag ()));
      }
    }
  }
  return db::Box ();
}

template <class Traits>
size_t
instance_iterator<Traits>::quad_id () const
{
  if (m_type == TInstance) {
    if (m_stable) {
      if (m_with_props) {
        return m_traits.quad_id (basic_iter (cell_inst_wp_array_type::tag (), InstancesEditableTag ()));
      } else {
        return m_traits.quad_id (basic_iter (cell_inst_array_type::tag (), InstancesEditableTag ()));
      }
    } else {
      if (m_with_props) {
        return m_traits.quad_id (basic_iter (cell_inst_wp_array_type::tag (), InstancesNonEditableTag ()));
      } else {
        return m_traits.quad_id (basic_iter (cell_inst_array_type::tag (), InstancesNonEditableTag ()));
      }
    }
  }
  return 0;
}

template <class Traits>
void
instance_iterator<Traits>::skip_quad () 
{
  if (m_type == TInstance) {
    if (m_stable) {
      if (m_with_props) {
        m_traits.skip_quad (basic_iter (cell_inst_wp_array_type::tag (), InstancesEditableTag ()));
      } else {
        m_traits.skip_quad (basic_iter (cell_inst_array_type::tag (), InstancesEditableTag ()));
      }
    } else {
      if (m_with_props) {
        m_traits.skip_quad (basic_iter (cell_inst_wp_array_type::tag (), InstancesNonEditableTag ()));
      } else {
        m_traits.skip_quad (basic_iter (cell_inst_array_type::tag (), InstancesNonEditableTag ()));
      }
    }
    make_next ();
    update_ref ();
  }
}

template <class Traits>
instance_iterator<Traits> &
instance_iterator<Traits>::operator++() 
{
  if (m_type == TInstance) {
    if (m_stable) {
      if (m_with_props) {
        ++basic_iter (cell_inst_wp_array_type::tag (), InstancesEditableTag ());
      } else {
        ++basic_iter (cell_inst_array_type::tag (), InstancesEditableTag ());
      }
    } else {
      if (m_with_props) {
        ++basic_iter (cell_inst_wp_array_type::tag (), InstancesNonEditableTag ());
      } else {
        ++basic_iter (cell_inst_array_type::tag (), InstancesNonEditableTag ());
      }
    }
    make_next ();
    update_ref ();
  }
  return *this;
}

template <class Traits>
void
instance_iterator<Traits>::make_next () 
{
  while (true) {
    if (m_stable) {
      if (m_with_props) {
        if (! basic_iter (cell_inst_wp_array_type::tag (), InstancesEditableTag ()).at_end ()) {
          return;
        }
      } else {
        if (! basic_iter (cell_inst_array_type::tag (), InstancesEditableTag ()).at_end ()) {
          return;
        }
      }
    } else {
      if (m_with_props) {
        if (! basic_iter (cell_inst_wp_array_type::tag (), InstancesNonEditableTag ()).at_end ()) {
          return;
        }
      } else {
        if (! basic_iter (cell_inst_array_type::tag (), InstancesNonEditableTag ()).at_end ()) {
          return;
        }
      }
    }
    release_iter ();
    m_with_props = ! m_with_props;
    if (! m_with_props) {
      m_type = TNull;
      return;
    }
    make_iter ();
  } 
}

template <class Traits>
void
instance_iterator<Traits>::update_ref ()
{
  if (m_type == TInstance) {
    if (m_stable) {
      if (m_with_props) {
        m_ref = m_traits.instance_from_stable_iter (basic_iter (cell_inst_wp_array_type::tag (), InstancesEditableTag ()));
      } else {
        m_ref = m_traits.instance_from_stable_iter (basic_iter (cell_inst_array_type::tag (), InstancesEditableTag ()));
      }
    } else {
      if (m_with_props) {
        m_ref = value_type (m_traits.instances (), *basic_iter (cell_inst_wp_array_type::tag (), InstancesNonEditableTag ()));
      } else {
        m_ref = value_type (m_traits.instances (), *basic_iter (cell_inst_array_type::tag (), InstancesNonEditableTag ()));
      }
    }
  } else {
    m_ref = value_type ();
  }
}

template class instance_iterator<NormalInstanceIteratorTraits>;
template class instance_iterator<TouchingInstanceIteratorTraits>;
template class instance_iterator<OverlappingInstanceIteratorTraits>;

// -------------------------------------------------------------------------------------
//  NormalInstanceIteratorTraits implementation

NormalInstanceIteratorTraits::NormalInstanceIteratorTraits ()
  : mp_insts (0)
{ }

NormalInstanceIteratorTraits::NormalInstanceIteratorTraits (const instances_type *insts)
  : mp_insts (insts)
{ }

void 
NormalInstanceIteratorTraits::init (instance_iterator<NormalInstanceIteratorTraits> *iter) const
{
  tl_assert (mp_insts != 0);
  if (iter->m_stable) {
    if (iter->m_with_props) {
      cell_inst_wp_array_type::tag tag = cell_inst_wp_array_type::tag ();
      iter->basic_iter (tag, InstancesEditableTag ()) = mp_insts->inst_tree (tag, InstancesEditableTag ()).begin_flat ();
    } else {
      cell_inst_array_type::tag tag = cell_inst_array_type::tag ();
      iter->basic_iter (tag, InstancesEditableTag ()) = mp_insts->inst_tree (tag, InstancesEditableTag ()).begin_flat ();
    }
  } else {
    if (iter->m_with_props) {
      cell_inst_wp_array_type::tag tag = cell_inst_wp_array_type::tag ();
      iter->basic_iter (tag, InstancesNonEditableTag ()) = iter_wp_type (mp_insts->inst_tree (tag, InstancesNonEditableTag ()).begin (), mp_insts->inst_tree (tag, InstancesNonEditableTag ()).end ());
    } else {
      cell_inst_array_type::tag tag = cell_inst_array_type::tag ();
      iter->basic_iter (tag, InstancesNonEditableTag ()) = iter_type (mp_insts->inst_tree (tag, InstancesNonEditableTag ()).begin (), mp_insts->inst_tree (tag, InstancesNonEditableTag ()).end ());
    }
  }
}

// -------------------------------------------------------------------------------------
//  TouchingInstanceIteratorTraits implementation

TouchingInstanceIteratorTraits::TouchingInstanceIteratorTraits ()
  : mp_insts (0), mp_layout (0)
{ }

TouchingInstanceIteratorTraits::TouchingInstanceIteratorTraits (const instances_type *insts, const box_type &box, const layout_type *layout)
  : mp_insts (insts), m_box (box), mp_layout (layout)
{ }

template <class InstArray, class ET>
void 
TouchingInstanceIteratorTraits::init (instance_iterator<TouchingInstanceIteratorTraits> *iter) const
{
  typename InstArray::tag tag = typename InstArray::tag ();
  db::box_convert<InstArray, false> bc (*mp_layout);
  iter->basic_iter (tag, ET ()) = mp_insts->inst_tree (tag, ET ()).begin_touching (m_box, bc);
}

void 
TouchingInstanceIteratorTraits::init (instance_iterator<TouchingInstanceIteratorTraits> *iter) const
{
  tl_assert (mp_insts != 0);
  if (iter->m_stable) {
    if (iter->m_with_props) {
      init<cell_inst_wp_array_type, InstancesEditableTag> (iter);
    } else {
      init<cell_inst_array_type, InstancesEditableTag> (iter);
    }
  } else {
    if (iter->m_with_props) {
      init<cell_inst_wp_array_type, InstancesNonEditableTag> (iter);
    } else {
      init<cell_inst_array_type, InstancesNonEditableTag> (iter);
    }
  }
}

// -------------------------------------------------------------------------------------
//  OverlappingInstanceIteratorTraits implementation

OverlappingInstanceIteratorTraits::OverlappingInstanceIteratorTraits ()
  : mp_insts (0), mp_layout (0)
{ }

OverlappingInstanceIteratorTraits::OverlappingInstanceIteratorTraits (const instances_type *insts, const box_type &box, const layout_type *layout)
  : mp_insts (insts), m_box (box), mp_layout (layout)
{ }

template <class InstArray, class ET>
void 
OverlappingInstanceIteratorTraits::init (instance_iterator<OverlappingInstanceIteratorTraits> *iter) const
{
  typename InstArray::tag tag = typename InstArray::tag ();
  db::box_convert<InstArray, false> bc (*mp_layout);
  iter->basic_iter (tag, ET ()) = mp_insts->inst_tree (tag, ET ()).begin_overlapping (m_box, bc);
}

void 
OverlappingInstanceIteratorTraits::init (instance_iterator<OverlappingInstanceIteratorTraits> *iter) const
{
  tl_assert (mp_insts != 0);
  if (iter->m_stable) {
    if (iter->m_with_props) {
      init<cell_inst_wp_array_type, InstancesEditableTag> (iter);
    } else {
      init<cell_inst_array_type, InstancesEditableTag> (iter);
    }
  } else {
    if (iter->m_with_props) {
      init<cell_inst_wp_array_type, InstancesNonEditableTag> (iter);
    } else {
      init<cell_inst_array_type, InstancesNonEditableTag> (iter);
    }
  }
}

// -------------------------------------------------------------------------------------
//  ChildCellIterator implementation

ChildCellIterator::ChildCellIterator ()
  : m_iter (), m_end ()
{ }

ChildCellIterator::ChildCellIterator (const instances_type *insts)
  : m_iter (insts->begin_sorted_insts ()),
    m_end (insts->end_sorted_insts ())
{ }

cell_index_type 
ChildCellIterator::operator* () const
{
  return (*m_iter)->object ().cell_index ();
}

size_t
ChildCellIterator::instances () const
{
  cell_index_type ci = operator* ();
  size_t n = 0;
  for (inst_iterator_type i = m_iter; i != m_end && (*i)->object ().cell_index () == ci; ++i) { 
    n += 1;
  }
  return n;
}

size_t
ChildCellIterator::weight () const
{
  cell_index_type ci = operator* ();
  size_t n = 0;
  for (inst_iterator_type i = m_iter; i != m_end && (*i)->object ().cell_index () == ci; ++i) { 
    n += (*i)->size ();
  }
  return n;
}

ChildCellIterator &
ChildCellIterator::operator++() 
{
  cell_index_type ci = operator* ();
  do {
    ++m_iter;
  } while (m_iter != m_end && operator* () == ci);
  return *this;
}

// -------------------------------------------------------------------------------------
//  Instance implementation

Instance::Instance ()
  : mp_instances (0), m_with_props (false), m_stable (false), m_type (TNull)
{ }

Instance::~Instance ()
{
  if (m_stable) {
    if (! m_with_props) {
      ((cell_inst_array_iterator_type *) (m_generic.iter))->~cell_inst_array_iterator_type ();
    } else {
      ((cell_inst_wp_array_iterator_type *) (m_generic.piter))->~cell_inst_wp_array_iterator_type ();
    }
  }
}

Instance::Instance (const db::Instances *instances, const cell_inst_array_type &inst)
  : mp_instances (const_cast<db::Instances *> (instances)), m_with_props (false), m_stable (false), m_type (TInstance)
{ 
  m_generic.inst = &inst;
}

Instance::Instance (db::Instances *instances, const cell_inst_array_type &inst)
  : mp_instances (instances), m_with_props (false), m_stable (false), m_type (TInstance)
{ 
  m_generic.inst = &inst;
}

Instance::Instance (const db::Instances *instances, const cell_inst_wp_array_type &inst)
  : mp_instances (const_cast<db::Instances *> (instances)), m_with_props (true), m_stable (false), m_type (TInstance)
{ 
  m_generic.pinst = &inst;
}

Instance::Instance (db::Instances *instances, const cell_inst_wp_array_type &inst)
  : mp_instances (instances), m_with_props (true), m_stable (false), m_type (TInstance)
{ 
  m_generic.pinst = &inst;
}

Instance::Instance (const db::Instances *instances, const cell_inst_array_iterator_type &iter)
  : mp_instances (const_cast<db::Instances *> (instances)), m_with_props (false), m_stable (true), m_type (TInstance)
{ 
  new (m_generic.iter) cell_inst_array_iterator_type (iter);
}

Instance::Instance (db::Instances *instances, const cell_inst_array_iterator_type &iter)
  : mp_instances (instances), m_with_props (false), m_stable (true), m_type (TInstance)
{ 
  new (m_generic.iter) cell_inst_array_iterator_type (iter);
}

Instance::Instance (const db::Instances *instances, const cell_inst_wp_array_iterator_type &iter)
  : mp_instances (const_cast<db::Instances *> (instances)), m_with_props (true), m_stable (true), m_type (TInstance)
{ 
  new (m_generic.piter) cell_inst_wp_array_iterator_type (iter);
}

Instance::Instance (db::Instances *instances, const cell_inst_wp_array_iterator_type &iter)
  : mp_instances (instances), m_with_props (true), m_stable (true), m_type (TInstance)
{ 
  new (m_generic.piter) cell_inst_wp_array_iterator_type (iter);
}

bool 
Instance::operator== (const Instance &d) const
{
  //  hint: don't use basic_ptr - this will fail if the reference is no longer valid.
  //  We want to be able to compare valid vs. non-valid references.
  if (m_type != d.m_type || m_with_props != d.m_with_props) {
    return false;
  }
  if (m_type == TInstance) {
    tl_assert (m_stable == d.m_stable);
    if (m_stable) {
      if (m_with_props) {
        return *((cell_inst_wp_array_iterator_type *) m_generic.piter) == *((cell_inst_wp_array_iterator_type *) d.m_generic.piter);
      } else {
        return *((cell_inst_array_iterator_type *) m_generic.iter) == *((cell_inst_array_iterator_type *) d.m_generic.iter);
      }
    } else {
      if (m_with_props) {
        return m_generic.pinst == d.m_generic.pinst;
      } else {
        return m_generic.inst == d.m_generic.inst;
      }
    }
  } else {
    return true;
  }
}

bool 
Instance::operator< (const Instance &d) const
{
  if (m_type != d.m_type) {
    return m_type < d.m_type;
  } 
  if (m_with_props != d.m_with_props) {
    return m_with_props < d.m_with_props;
  }
  if (m_type == TInstance) {
    tl_assert (m_stable == d.m_stable);
    if (m_stable) {
      if (m_with_props) {
        return *((cell_inst_wp_array_iterator_type *) m_generic.piter) < *((cell_inst_wp_array_iterator_type *) d.m_generic.piter);
      } else {
        return *((cell_inst_array_iterator_type *) m_generic.iter) < *((cell_inst_array_iterator_type *) d.m_generic.iter);
      }
    } else {
      if (m_with_props) {
        return m_generic.pinst < d.m_generic.pinst;
      } else {
        return m_generic.inst < d.m_generic.inst;
      }
    }
  } else {
    return false;
  }
}

Instance::box_type
Instance::bbox () const
{
  const db::Instances *i = instances ();
  const db::Cell *c = i ? i->cell () : 0;
  const db::Layout *g = c ? c->layout () : 0;
  if (g) {
    return bbox (db::box_convert<cell_inst_type> (*g));
  } else {
    return db::Instance::box_type ();
  }
}

// -------------------------------------------------------------------------------------
//  Instances implementation

static void
check_is_editable_for_undo_redo (const Instances *instances)
{
  if (! instances->is_editable ()) {
    throw tl::Exception (tl::to_string (tr ("No undo/redo support on non-editable instance lists")));
  }
}

Instances::Instances (cell_type *cell)
  : mp_cell (cell)
{
  m_generic.any = 0;
  m_generic_wp.any = 0;
}

Instances::~Instances ()
{
  do_clear_insts ();
}

Instances &
Instances::operator= (const Instances &d)
{
  if (this != &d) {

    if (! empty ()) {
      clear_insts ();
    }

    db::ArrayRepository *rep = layout () ? &layout ()->array_repository () : 0;

    if (is_editable ()) {

      if (! d.inst_tree (cell_inst_array_type::tag (), InstancesEditableTag ()).empty ()) {

        stable_cell_inst_tree_type &t = inst_tree (cell_inst_array_type::tag (), InstancesEditableTag ());
        t = d.inst_tree (cell_inst_array_type::tag (), InstancesEditableTag ());

        //  translate instances to a different array repository if required
        if (d.layout () != layout ()) {
          for (stable_cell_inst_tree_type::iterator i = t.begin (); i != t.end (); ++i) {
            if (i->in_repository ()) {
              *i = cell_inst_array_type (*i, rep);
            }
          }
        }

      }

      if (! d.inst_tree (cell_inst_wp_array_type::tag (), InstancesEditableTag ()).empty ()) {

        stable_cell_inst_wp_tree_type &t = inst_tree (cell_inst_wp_array_type::tag (), InstancesEditableTag ());
        t = d.inst_tree (cell_inst_wp_array_type::tag (), InstancesEditableTag ());

        //  translate instances to a different array repository if required
        if (d.layout () != layout ()) {
          for (stable_cell_inst_wp_tree_type::iterator i = t.begin (); i != t.end (); ++i) {
            if (i->in_repository ()) {
              *i = cell_inst_wp_array_type (cell_inst_array_type (*i, rep), i->properties_id ());
            }
          }
        }

      }

    } else {

      if (! d.inst_tree (cell_inst_array_type::tag (), InstancesNonEditableTag ()).empty ()) {

        cell_inst_tree_type &t = inst_tree (cell_inst_array_type::tag (), InstancesNonEditableTag ());
        t = d.inst_tree (cell_inst_array_type::tag (), InstancesNonEditableTag ());

        //  translate instances to a different array repository if required
        if (d.layout () != layout ()) {
          for (cell_inst_tree_type::const_iterator i = t.begin (); i != t.end (); ++i) {
            if (i->in_repository ()) {
              t.replace (i, cell_inst_array_type (*i, rep));
            }
          }
        }

      }

      if (! d.inst_tree (cell_inst_wp_array_type::tag (), InstancesNonEditableTag ()).empty ()) {

        cell_inst_wp_tree_type &t = inst_tree (cell_inst_wp_array_type::tag (), InstancesNonEditableTag ());
        t = d.inst_tree (cell_inst_wp_array_type::tag (), InstancesNonEditableTag ());

        //  translate instances to a different array repository if required
        if (d.layout () != layout ()) {
          for (cell_inst_wp_tree_type::const_iterator i = t.begin (); i != t.end (); ++i) {
            if (i->in_repository ()) {
              t.replace (i, cell_inst_wp_array_type (cell_inst_array_type (*i, rep), i->properties_id ()));
            }
          }
        }

      }

    }

    m_parent_insts = d.m_parent_insts;

    set_instance_by_cell_index_needs_made (true);
    set_instance_tree_needs_sort (true);

  }
  return *this;
}

bool 
Instances::is_editable () const
{
  return cell () && cell ()->layout () ? cell ()->layout ()->is_editable () : true;
}

db::Layout *
Instances::layout () const
{
  return cell () ? cell ()->layout () : 0;
}

void
Instances::invalidate_insts ()
{
  if (cell ()) {
    cell ()->invalidate_insts ();
  }

  set_instance_by_cell_index_needs_made (true);
  set_instance_tree_needs_sort (true);
}

template <class Tag, class ET, class I>
void 
Instances::erase_positions (Tag tag, ET editable_tag, I first, I last)
{
  typedef instances_editable_traits<ET> editable_traits;

  invalidate_insts ();  //  HINT: must come before the change is done!

  if (cell ()) {
    if (cell ()->manager () && cell ()->manager ()->transacting ()) {
      check_is_editable_for_undo_redo (this);
      if (! is_editable ()) {
        throw tl::Exception (tl::to_string (tr ("No undo/redo support for non-editable instance lists in 'erase_positions'")));
      }
      cell ()->manager ()->queue (cell (), new db::InstOp<typename Tag::object_type, ET> (false /*not insert*/, first, last, true /*dummy*/));
    }
  }

  typename editable_traits::template tree_traits<Tag>::tree_type &t = inst_tree (tag, editable_tag);
  t.erase_positions (first, last);
}

template <class InstArray>
Instances::instance_type
Instances::insert (const InstArray &inst)
{
  bool editable = is_editable ();

  if (cell ()) {
    if (cell ()->manager () && cell ()->manager ()->transacting ()) {
      check_is_editable_for_undo_redo (this);
      if (editable) {
        cell ()->manager ()->queue (cell (), new db::InstOp<InstArray, InstancesEditableTag> (true /*insert*/, inst));
      } else {
        cell ()->manager ()->queue (cell (), new db::InstOp<InstArray, InstancesNonEditableTag> (true /*insert*/, inst));
      }
    }
  }

  invalidate_insts ();

  // TODO: simplify this, i.e. through instance_from_pointer
  if (editable) {
    return instance_type (this, inst_tree (typename InstArray::tag (), InstancesEditableTag ()).insert (inst));
  } else {
    return instance_type (this, *(inst_tree (typename InstArray::tag (), InstancesNonEditableTag ()).insert (inst)));
  }
}

template <class I, class ET>
void 
Instances::insert (I from, I to)
{
  typedef std::iterator_traits<I> it_traits;
  typedef typename it_traits::value_type value_type;

  if (cell ()) {
    if (cell ()->manager () && cell ()->manager ()->transacting ()) {
      check_is_editable_for_undo_redo (this);
      cell ()->manager ()->queue (cell (), new db::InstOp<typename it_traits::value_type, ET> (true /*insert*/, from, to));
    }
  }

  invalidate_insts ();
  inst_tree (typename value_type::tag (), ET ()).insert (from, to);
}

template <class I>
void 
Instances::insert (I from, I to)
{
  if (is_editable ()) {
    insert<I, InstancesEditableTag> (from, to);
  } else {
    insert<I, InstancesNonEditableTag> (from, to);
  }
}

template <class Tag>
bool 
Instances::is_valid_by_tag (Tag tag, const instance_type &ref) const
{
  if (ref.instances () != this) {
    return false;
  }
  if (is_editable ()) {
    return ref.basic_iter (tag)->is_valid ();
  } else {
    //  This is not really the case, but there is no other way to check this:
    return true;  
  }
}

bool
Instances::is_valid (const instance_type &ref) const
{
  if (ref.has_prop_id ()) {
    return is_valid_by_tag (cell_inst_wp_array_type::tag (), ref);
  } else {
    return is_valid_by_tag (cell_inst_array_type::tag (), ref);
  }
}

template <class InstArray>
void 
Instances::replace (const InstArray *replace, const InstArray &with)
{
  if (cell ()) {
    if (cell ()->manager () && cell ()->manager ()->transacting ()) {
      check_is_editable_for_undo_redo (this);
      if (is_editable ()) {
        cell ()->manager ()->queue (cell (), new db::InstOp<InstArray, InstancesEditableTag> (false /*not insert*/, *replace));
        cell ()->manager ()->queue (cell (), new db::InstOp<InstArray, InstancesEditableTag> (true /*insert*/, with));
      } else {
        cell ()->manager ()->queue (cell (), new db::InstOp<InstArray, InstancesNonEditableTag> (false /*not insert*/, *replace));
        cell ()->manager ()->queue (cell (), new db::InstOp<InstArray, InstancesNonEditableTag> (true /*insert*/, with));
      }
    }
  }

  invalidate_insts ();

  //  HINT: this only works because we know our box trees well:
  *((InstArray *)replace) = with;
}

Instances::instance_type 
Instances::replace (const instance_type &ref, const cell_inst_wp_array_type &inst)
{
  if (ref.instances () != this) {
    throw tl::Exception (tl::to_string (tr ("Trying to replace an object in a list that it does not belong to")));
  }

  const cell_inst_wp_array_type *cp = ref.basic_ptr (cell_inst_wp_array_type::tag ());
  if (cp) {

    //  in-place replacement 
    replace (cp, inst);
    return ref;

  } else {

    //  not an in-place replacement - erase and insert
    erase (ref);
    return insert (inst);

  }
}

Instances::instance_type 
Instances::replace (const instance_type &ref, const cell_inst_array_type &inst)
{
  if (ref.instances () != this) {
    throw tl::Exception (tl::to_string (tr ("Trying to replace an object in a list that it does not belong to")));
  }

  const cell_inst_array_type *cp = ref.basic_ptr (cell_inst_array_type::tag ());
  if (cp) {

    //  in-place replacement 
    replace (cp, inst);
    return ref;

  } else {
    
    const cell_inst_wp_array_type *cp_wp = ref.basic_ptr (cell_inst_wp_array_type::tag ());
    if (cp_wp) {

      //  the present object has a property: maintain that one
      db::properties_id_type pid = ref.prop_id ();
      cell_inst_wp_array_type inst_wp (inst, pid);

      //  in-place replacement with properties
      replace (cp_wp, inst_wp);

      return instance_from_pointer (cp_wp);

    } else {

      //  not an in-place replacement - erase and insert
      //  NOTE: this should not happen since there are only cell_inst_wp_array_type and cell_inst_array_type objects ..
      erase (ref);
      return insert (inst);

    }

  }
}

template <class Tag, class ET, class I>
void 
Instances::erase_inst_by_iter (Tag tag, ET editable_tag, I iter)
{
  if (iter.vector () != &inst_tree (tag, editable_tag).objects ()) {
    throw tl::Exception (tl::to_string (tr ("Trying to erase an object from a list that it does not belong to")));
  }

  invalidate_insts ();

  if (cell ()) {
    if (cell ()->manager () && cell ()->manager ()->transacting ()) {
      check_is_editable_for_undo_redo (this);
      cell ()->manager ()->queue (cell (), new db::InstOp<typename Tag::object_type, ET> (false /*not insert*/, *iter));
    }
  }

  inst_tree (tag, editable_tag).erase (iter.to_non_const ());
}

template <class Tag, class ET>
void 
Instances::erase_inst_by_tag (Tag tag, ET editable_tag, const typename Tag::object_type &obj)
{
  invalidate_insts ();

  if (cell ()) {
    if (cell ()->manager () && cell ()->manager ()->transacting ()) {
      check_is_editable_for_undo_redo (this);
      cell ()->manager ()->queue (cell (), new db::InstOp<typename Tag::object_type, ET> (false /*not insert*/, obj));
    }
  }

  inst_tree (tag, editable_tag).erase (inst_tree (tag, editable_tag).iterator_from_pointer (const_cast <typename Tag::object_type *> (&obj)));
}

template <class Tag, class ET>
void 
Instances::erase_insts_by_tag (Tag tag, ET editable_tag, std::vector<instance_type>::const_iterator s1, std::vector<instance_type>::const_iterator s2)
{
  typedef typename instances_editable_traits<ET>::template tree_traits<Tag>::tree_type tree_type;
  tree_type &t = inst_tree (tag, editable_tag);

  std::vector<typename tree_type::iterator> iters;
  iters.reserve (std::distance (s1, s2));

  for (std::vector<instance_type>::const_iterator s = s1; s != s2; ++s) {
    iters.push_back (t.iterator_from_pointer (const_cast <typename Tag::object_type *> (s->basic_ptr (tag))));
  }

  erase_positions (tag, editable_tag, iters.begin (), iters.end ());
}

template <class ET>
void
Instances::clear_insts (ET editable_tag)
{
  invalidate_insts ();

  if (cell ()) {
    if (cell ()->manager () && cell ()->manager ()->transacting ()) {
      check_is_editable_for_undo_redo (this);
      const Instances *const_this = this;
      if (! const_this->inst_tree (cell_inst_array_type::tag (), editable_tag).empty ()) {
        cell ()->manager ()->queue (cell (), new db::InstOp<cell_inst_array_type, ET> (false /*not insert*/, const_this->inst_tree (cell_inst_array_type::tag (), editable_tag).begin (), const_this->inst_tree (cell_inst_array_type::tag (), editable_tag).end ()));
      }
      if (! const_this->inst_tree (cell_inst_wp_array_type::tag (), editable_tag).empty ()) {
        cell ()->manager ()->queue (cell (), new db::InstOp<cell_inst_wp_array_type, ET> (false /*not insert*/, const_this->inst_tree (cell_inst_wp_array_type::tag (), editable_tag).begin (), const_this->inst_tree (cell_inst_wp_array_type::tag (), editable_tag).end ()));
      }
    }
  }

  do_clear_insts ();
}

void 
Instances::clear_insts ()
{
  if (is_editable ()) {
    clear_insts (InstancesEditableTag ());
  } else {
    clear_insts (InstancesNonEditableTag ());
  }
}

void 
Instances::clear (Instances::cell_inst_array_type::tag) 
{
  invalidate_insts ();

  if (m_generic.any) {
    if (is_editable ()) {
      delete m_generic.stable_tree;
    } else {
      delete m_generic.unstable_tree;
    }
    m_generic.any = 0;
  }
}

void 
Instances::clear (Instances::cell_inst_wp_array_type::tag) 
{
  invalidate_insts ();

  if (m_generic_wp.any) {
    if (is_editable ()) {
      delete m_generic_wp.stable_tree;
    } else {
      delete m_generic_wp.unstable_tree;
    }
    m_generic_wp.any = 0;
  }
}

Instances::instance_type
Instances::instance_from_pointer (const basic_inst_type *p) const
{
  if (is_editable ()) {
    if (inst_tree (cell_inst_array_type::tag (), InstancesEditableTag ()).is_member_of (p)) {
      return instance_type (this, inst_tree (cell_inst_array_type::tag (), InstancesEditableTag ()).iterator_from_pointer (static_cast <const cell_inst_array_type *> (p)));
    }
    if (inst_tree (cell_inst_wp_array_type::tag (), InstancesEditableTag ()).is_member_of (p)) {
      return instance_type (this, inst_tree (cell_inst_wp_array_type::tag (), InstancesEditableTag ()).iterator_from_pointer (static_cast <const cell_inst_wp_array_type *> (p)));
    }
  } else {
    if (inst_tree (cell_inst_array_type::tag (), InstancesNonEditableTag ()).end () != inst_tree (cell_inst_array_type::tag (), InstancesNonEditableTag ()).begin () && p <= &*(inst_tree (cell_inst_array_type::tag (), InstancesNonEditableTag ()).end () - 1) && p >= &*(inst_tree (cell_inst_array_type::tag (), InstancesNonEditableTag ()).begin ())) {
      return instance_type (this, *static_cast <const cell_inst_array_type *> (p));
    }
    if (inst_tree (cell_inst_wp_array_type::tag (), InstancesNonEditableTag ()).end () != inst_tree (cell_inst_wp_array_type::tag (), InstancesNonEditableTag ()).begin () && p <= &*(inst_tree (cell_inst_wp_array_type::tag (), InstancesNonEditableTag ()).end () - 1) && p >= &*(inst_tree (cell_inst_wp_array_type::tag (), InstancesNonEditableTag ()).begin ())) {
      return instance_type (this, *static_cast <const cell_inst_wp_array_type *> (p));
    }
  }
  return Instance ();
}

bool 
Instances::empty () const
{
  if (is_editable ()) {
    return ((! m_generic.stable_tree || m_generic.stable_tree->empty ()) &&
            (! m_generic_wp.stable_tree || m_generic_wp.stable_tree->empty ()));
  } else {
    return ((! m_generic.unstable_tree || m_generic.unstable_tree->empty ()) &&
            (! m_generic_wp.unstable_tree || m_generic_wp.unstable_tree->empty ()));
  }
}

void 
Instances::erase (const Instance &ref)
{
  if (ref.is_null ()) {
    //  .. nothing ..
  } else if (ref.has_prop_id ()) {
    erase_inst_by_tag (cell_inst_wp_array_type::tag (), ref);
  } else {
    erase_inst_by_tag (cell_inst_array_type::tag (), ref);
  } 
}

void 
Instances::erase (const Instances::const_iterator &e)
{
  if (e.at_end ()) {
    //  .. nothing ..
  } else if (e->has_prop_id ()) {
    erase_inst_by_tag (cell_inst_wp_array_type::tag (), e);
  } else {
    erase_inst_by_tag (cell_inst_array_type::tag (), e);
  } 
}

void 
Instances::erase_insts (const std::vector<Instance> &instances)
{
  for (std::vector<instance_type>::const_iterator i = instances.begin (); i != instances.end (); ) {

    std::vector<instance_type>::const_iterator inext = i;
    while (inext != instances.end () && inext->has_prop_id () == i->has_prop_id ()) {
      ++inext;
    }

    if (i->has_prop_id ()) {
      if (is_editable ()) {
        erase_insts_by_tag (cell_inst_wp_array_type::tag (), InstancesEditableTag (), i, inext);
      } else {
        erase_insts_by_tag (cell_inst_wp_array_type::tag (), InstancesNonEditableTag (), i, inext);
      }
    } else {
      if (is_editable ()) {
        erase_insts_by_tag (cell_inst_array_type::tag (), InstancesEditableTag (), i, inext);
      } else {
        erase_insts_by_tag (cell_inst_array_type::tag (), InstancesNonEditableTag (), i, inext);
      }
    }

    i = inext;

  }
}

void 
Instances::count_parent_insts (std::vector <size_t> &count) const
{
  cell_index_type last_ci = (cell_index_type) -1l;
  
  for (sorted_inst_iterator c = begin_sorted_insts (); c != end_sorted_insts (); ++c) {
    cell_index_type ci = (*c)->object ().cell_index ();
    if (ci != last_ci) {
      last_ci = ci;
      ++count [ci];
    }
  }
}

/**
 *  @brief A helper function that compares child instances
 */
template <class I>
struct cell_inst_compare_f
{
  bool operator() (const I *a, const I *b) const
  {
    return a->raw_less (*b);
  }
};

void 
Instances::sort_child_insts (bool force)
{
  if (! force && ! instance_by_cell_index_needs_made ()) {
    return;
  }
  set_instance_by_cell_index_needs_made (false);

  m_insts_by_cell_index = sorted_inst_vector ();
  m_insts_by_cell_index.reserve (cell_instances ());

  //  HINT: we do not use the flat iterator since this would required a "made" index
  //  which is not available in some cases.
  if (is_editable ()) {
    if (m_generic.any) {
      for (stable_cell_inst_tree_type::const_iterator i = m_generic.stable_tree->begin (); i != m_generic.stable_tree->end (); ++i) {
        m_insts_by_cell_index.push_back (&*i);
      }
    }
    if (m_generic_wp.any) {
      for (stable_cell_inst_wp_tree_type::const_iterator i = m_generic_wp.stable_tree->begin (); i != m_generic_wp.stable_tree->end (); ++i) {
        m_insts_by_cell_index.push_back (&*i);
      }
    }
  } else {
    if (m_generic.any) {
      for (cell_inst_tree_type::const_iterator i = m_generic.unstable_tree->begin (); i != m_generic.unstable_tree->end (); ++i) {
        m_insts_by_cell_index.push_back (&*i);
      }
    }
    if (m_generic_wp.any) {
      for (cell_inst_wp_tree_type::const_iterator i = m_generic_wp.unstable_tree->begin (); i != m_generic_wp.unstable_tree->end (); ++i) {
        m_insts_by_cell_index.push_back (&*i);
      }
    }
  }

  std::sort (m_insts_by_cell_index.begin (), m_insts_by_cell_index.end (), cell_inst_compare_f<basic_inst_type> ());
}

void 
Instances::sort_inst_tree (const Layout *g, bool force)
{
  if (! force && ! instance_tree_needs_sort ()) {
    return;
  }
  set_instance_tree_needs_sort (false);

  if (m_generic.any) {
    if (is_editable ()) {
      m_generic.stable_tree->sort (cell_inst_array_box_converter (*g));
    } else {
      m_generic.unstable_tree->sort (cell_inst_array_box_converter (*g));
      //  since we use unstable instance trees in non-editable mode, we need to resort the child instances in this case
      sort_child_insts (true);
    }
  }
  if (m_generic_wp.any) {
    if (is_editable ()) {
      m_generic_wp.stable_tree->sort (cell_inst_wp_array_box_converter (*g));
    } else {
      m_generic_wp.unstable_tree->sort (cell_inst_wp_array_box_converter (*g));
      //  since we use unstable instance trees in non-editable mode, we need to resort the child instances in this case
      sort_child_insts (true);
    }
  }

}

void 
Instances::update_relations (Layout *g, cell_index_type cell_index)
{
  cell_index_type last_ci = (cell_index_type) -1l;

  size_t idx = 0;
  for (sorted_inst_iterator c = begin_sorted_insts (); c != end_sorted_insts (); ++c, ++idx) {
    cell_index_type ci = (*c)->object ().cell_index ();
    if (ci != last_ci) {
      last_ci = ci;
      g->cell (ci).instances ().m_parent_insts.push_back (parent_inst_type (cell_index, idx));
    }
  }
}

size_t 
Instances::child_cells () const
{
  size_t n = 0;
  for (ChildCellIterator i = begin_child_cells (); ! i.at_end (); ++i) {
    ++n;
  }
  return n;
}

size_t 
Instances::cell_instances () const
{
  if (is_editable ()) {
    return (m_generic.stable_tree ? m_generic.stable_tree->size () : 0) +
           (m_generic_wp.stable_tree ? m_generic_wp.stable_tree->size () : 0);
  } else {
    return (m_generic.unstable_tree ? m_generic.unstable_tree->size () : 0) +
           (m_generic_wp.unstable_tree ? m_generic_wp.unstable_tree->size () : 0);
  }
}

void
Instances::mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self, void *parent) const
{
  if (!no_self) {
    stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
  }

  db::mem_stat (stat, MemStatistics::Instances, cat, m_parent_insts, true, (void *) this);
  db::mem_stat (stat, MemStatistics::Instances, cat, m_insts_by_cell_index, true, (void *) this);

  if (is_editable ()) {
    if (m_generic.stable_tree) {
      db::mem_stat (stat, MemStatistics::Instances, cat, *m_generic.stable_tree, true, (void *) this);
    }
    if (m_generic_wp.stable_tree) {
      db::mem_stat (stat, MemStatistics::Instances, cat, *m_generic_wp.stable_tree, true, (void *) this);
    }
  } else {
    if (m_generic.unstable_tree) {
      db::mem_stat (stat, MemStatistics::Instances, cat, *m_generic.unstable_tree, true, (void *) this);
    }
    if (m_generic_wp.unstable_tree) {
      db::mem_stat (stat, MemStatistics::Instances, cat, *m_generic_wp.unstable_tree, true, (void *) this);
    }
  }
}

Instances::instance_type 
Instances::replace_prop_id (const instance_type &ref, db::properties_id_type prop_id)
{
  if (ref.instances () != this) {
    throw tl::Exception (tl::to_string (tr ("Trying to replace an object in a list that it does not belong to")));
  }

  if (! ref.is_null ()) {
    cell_inst_wp_array_type new_inst (ref.cell_inst (), prop_id);
    return replace (ref, new_inst);
  } else {
    return ref;
  }
}

void 
Instances::do_clear_insts ()
{
  if (m_generic.any) {
    if (is_editable ()) {
      delete m_generic.stable_tree;
    } else {
      delete m_generic.unstable_tree;
    }
    m_generic.any = 0;
  }
  if (m_generic_wp.any) {
    if (is_editable ()) {
      delete m_generic_wp.stable_tree;
    } else {
      delete m_generic_wp.unstable_tree;
    }
    m_generic_wp.any = 0;
  }
}

void
Instances::undo (db::Op *op)
{
  db::InstOpBase *instop = dynamic_cast<InstOpBase *> (op);
  if (instop) {
    instop->undo (this);
  } 
}

void
Instances::redo (db::Op *op)
{
  //  actions are only queued by the instance list - this is should be 
  //  responsible for the handling of the latter.
  //  HACK: this is not really a nice concept, but it saves us a pointer to the manager.
  db::InstOpBase *instop = dynamic_cast<InstOpBase *> (op);
  if (instop) {
    instop->redo (this);
  } 
}

Instances::instance_type 
Instances::do_insert (const Instances::instance_type &ref, 
                      tl::func_delegate_base <db::cell_index_type> &im,
                      tl::func_delegate_base <db::properties_id_type> &pm)
{
  if (ref.instances () == this) {

    if (! ref.has_prop_id ()) {

      cell_inst_array_type inst = *ref.basic_ptr (cell_inst_array_type::tag ());
      inst.object () = cell_inst_type (im (ref.cell_index ()));

      return insert (inst);

    } else {

      cell_inst_wp_array_type inst_wp = *ref.basic_ptr (cell_inst_wp_array_type::tag ());
      inst_wp.object () = cell_inst_type (im (ref.cell_index ()));
      inst_wp.properties_id (pm (ref.prop_id ()));

      return insert (inst_wp);

    }

  } else {

    if (! ref.has_prop_id ()) {

      cell_inst_array_type inst (*ref.basic_ptr (cell_inst_array_type::tag ()), layout () ? &layout ()->array_repository () : 0);
      inst.object () = cell_inst_type (im (ref.cell_index ()));

      return insert (inst);

    } else {

      cell_inst_array_type inst (*ref.basic_ptr (cell_inst_wp_array_type::tag ()), layout () ? &layout ()->array_repository () : 0);
      inst.object () = cell_inst_type (im (ref.cell_index ()));

      return insert (cell_inst_wp_array_type (inst, pm (ref.prop_id ())));

    }

  }
}

template <class Op, class ET>
void Instances::apply_op (const Op &op, ET editable_tag)
{
  const Instances *const_this = this;
  bool transacting = false;
  bool has_insts = ! const_this->inst_tree (cell_inst_array_type::tag (), editable_tag).empty ();
  bool has_wp_insts = ! const_this->inst_tree (cell_inst_wp_array_type::tag (), editable_tag).empty ();

  invalidate_insts ();

  if (cell ()) {
    if (cell ()->manager () && cell ()->manager ()->transacting ()) {
      check_is_editable_for_undo_redo (this);
      transacting = true;
      if (has_insts) {
        cell ()->manager ()->queue (cell (), new db::InstOp<cell_inst_array_type, ET> (false /*not insert*/, const_this->inst_tree (cell_inst_array_type::tag (), editable_tag).begin (), const_this->inst_tree (cell_inst_array_type::tag (), editable_tag).end ()));
      }
      if (has_wp_insts) {
        cell ()->manager ()->queue (cell (), new db::InstOp<cell_inst_wp_array_type, ET> (false /*not insert*/, const_this->inst_tree (cell_inst_wp_array_type::tag (), editable_tag).begin (), const_this->inst_tree (cell_inst_wp_array_type::tag (), editable_tag).end ()));
      }
    }
  }

  if (has_insts) {
    typedef typename instances_editable_traits<ET>::template tree_traits<cell_inst_array_type::tag>::tree_type tree_type;
    typename tree_type::iterator from = inst_tree (cell_inst_array_type::tag (), editable_tag).begin ();
    typename tree_type::iterator to = inst_tree (cell_inst_array_type::tag (), editable_tag).end ();
    for (typename tree_type::iterator i = from; i != to; ++i) {
      op (*i);
    }
  }

  if (has_wp_insts) {
    typedef typename instances_editable_traits<ET>::template tree_traits<cell_inst_wp_array_type::tag>::tree_type tree_type;
    typename tree_type::iterator from = inst_tree (cell_inst_wp_array_type::tag (), editable_tag).begin ();
    typename tree_type::iterator to = inst_tree (cell_inst_wp_array_type::tag (), editable_tag).end ();
    for (typename tree_type::iterator i = from; i != to; ++i) {
      op (*i);
    }
  }

  if (transacting) {
    if (has_insts) {
      cell ()->manager ()->queue (cell (), new db::InstOp<cell_inst_array_type, ET> (true /*insert*/, const_this->inst_tree (cell_inst_array_type::tag (), editable_tag).begin (), const_this->inst_tree (cell_inst_array_type::tag (), editable_tag).end ()));
    }
    if (has_wp_insts) {
      cell ()->manager ()->queue (cell (), new db::InstOp<cell_inst_wp_array_type, ET> (true /*insert*/, const_this->inst_tree (cell_inst_wp_array_type::tag (), editable_tag).begin (), const_this->inst_tree (cell_inst_wp_array_type::tag (), editable_tag).end ()));
    }
  }
}

namespace {

  //  A function to apply a transformation with "transform"
  template <class T>
  struct apply_transform_f
  {
    apply_transform_f (const T &t) : trans (t) { }
    template <class Obj> void operator() (Obj &obj) const { obj.transform (trans); }
    T trans;
  };

  //  A function to apply a transformation with "transform_into"
  template <class T>
  struct apply_transform_into_f
  {
    apply_transform_into_f (const T &t) : trans (t) { }
    template <class Obj> void operator() (Obj &obj) const { obj.transform_into (trans); }
    T trans;
  };

}

template <class T>
void Instances::transform (const T &tr)
{
  if (is_editable ()) {
    apply_op (apply_transform_f<T> (tr), InstancesEditableTag ());
  } else {
    apply_op (apply_transform_f<T> (tr), InstancesNonEditableTag ());
  }
}

template <class T>
void Instances::transform_into (const T &tr)
{
  if (is_editable ()) {
    apply_op (apply_transform_into_f<T> (tr), InstancesEditableTag ());
  } else {
    apply_op (apply_transform_into_f<T> (tr), InstancesNonEditableTag ());
  }
}

//  explicit instantiations
template DB_PUBLIC void Instances::replace<> (const Instances::cell_inst_array_type *replace, const Instances::cell_inst_array_type &with);
template DB_PUBLIC void Instances::replace<> (const Instances::cell_inst_wp_array_type *replace, const Instances::cell_inst_wp_array_type &with);
template DB_PUBLIC Instance Instances::insert<> (const Instances::cell_inst_wp_array_type &with);
template DB_PUBLIC Instance Instances::insert<> (const Instances::cell_inst_array_type &with);
template DB_PUBLIC void Instances::transform<> (const Trans &t);
template DB_PUBLIC void Instances::transform<> (const ICplxTrans &t);
template DB_PUBLIC void Instances::transform_into<> (const Trans &t);
template DB_PUBLIC void Instances::transform_into<> (const ICplxTrans &t);

//  This should not be instantiated explicitly, but without that we'd have to expose the undo/redo code in the 
//  header
template DB_PUBLIC void Instances::insert<> (std::vector<Instances::cell_inst_wp_array_type>::const_iterator from, std::vector<Instances::cell_inst_wp_array_type>::const_iterator to);
template DB_PUBLIC void Instances::insert<> (std::vector<Instances::cell_inst_array_type>::const_iterator from, std::vector<Instances::cell_inst_array_type>::const_iterator to);
template DB_PUBLIC void Instances::insert<> (std::vector<Instances::cell_inst_wp_array_type>::iterator from, std::vector<Instances::cell_inst_wp_array_type>::iterator to);
template DB_PUBLIC void Instances::insert<> (std::vector<Instances::cell_inst_array_type>::iterator from, std::vector<Instances::cell_inst_array_type>::iterator to);

}

