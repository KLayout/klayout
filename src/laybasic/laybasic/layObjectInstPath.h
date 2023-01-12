
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



#ifndef HDR_layObjectInstPath
#define HDR_layObjectInstPath

#include "laybasicCommon.h"

#include <limits>
#include <list>
#include <utility>

#include "dbInstElement.h"
#include "dbClipboardData.h"
#include "dbClipboard.h"

namespace lay
{
  class LayoutViewBase;
}

namespace lay {

/**
 *  @brief A class encapsulating an instantiation path and the element addressed by it
 *
 *  This class either addresses an instance (in which case the path is that to the instance 
 *  addressed) or a shape (in which case the path leads to the cell that has this shape and
 *  the layer and shape is specified additionally).
 */

class LAYBASIC_PUBLIC ObjectInstPath
{
public:
  typedef std::list<db::InstElement> path_type;
  typedef path_type::const_iterator iterator;
  typedef path_type::iterator non_const_iterator;

  /**
   *  @brief Create an empty path
   */
  ObjectInstPath ();

  /**
   *  @brief Set the cellview index
   */
  void set_cv_index (unsigned int cv_index)
  {
    m_cv_index = cv_index;
  }

  /**
   *  @brief Set the topcell 
   */
  void set_topcell (db::cell_index_type topcell)
  {
    m_topcell = topcell;
  }

  /**
   *  @brief Clears the instantiation path
   */
  void clear_path ()
  {
    m_path.clear ();
  }

  /**
   *  @brief Add one element to the instantiation path
   */
  void add_path (const db::InstElement &elem)
  {
    m_path.push_back (elem);
  }

  /**
   *  @brief Add the given sequence of elements to the instantiation path
   */
  template <class Iter>
  void add_path (Iter first, Iter last)
  {
    m_path.insert (m_path.end (), first, last);
  }

  /**
   *  @brief Assign the given sequence of elements to the instantiation path
   */
  template <class Iter>
  void assign_path (Iter first, Iter last)
  {
    m_path.assign (first, last);
  }

  /**
   *  @brief Obtain the index of the target cell
   */
  db::cell_index_type cell_index () const;

  /**
   *  @brief Obtain the index of the target cell, including the instantiated cell, if the path describes an instance
   */
  db::cell_index_type cell_index_tot () const;

  /**
   *  @brief Obtain the index of the top cell
   */
  db::cell_index_type topcell () const
  {
    return m_topcell;
  }

  /**
   *  @brief Obtain the cellview index
   */
  unsigned int cv_index () const
  {
    return m_cv_index;
  }

  /**
   *  @brief Set the layer
   *
   *  Setting a layer explicitly makes this path point to a shape (which has to be 
   *  specified additionally).
   */
  void set_layer (unsigned int layer)
  {
    m_layer = layer;
  }

  /**
   *  @brief Get the layer that the selected shape is on 
   */
  unsigned int layer () const 
  {
    tl_assert (! is_cell_inst ());
    return (unsigned int) m_layer;
  }

  /**
   *  @brief Set the selected shape
   */
  void set_shape (const db::Shape &shape)
  {
    m_shape = shape;
  }

  /**
   *  @brief Gets the selected shape
   */
  const db::Shape &shape () const
  {
    tl_assert (! is_cell_inst ());
    return m_shape;
  }

  /**
   *  @brief Gets the selected shape (non-const version)
   */
  db::Shape &shape () 
  {
    tl_assert (! is_cell_inst ());
    return m_shape;
  }

  /**
   *  @brief Obtain the combined transformation for this instantiation path
   */
  db::ICplxTrans trans () const;

  /**
   *  @brief Obtain the combined transformation for this instantiation path including the instance transformation, if the path describes an instance
   */
  db::ICplxTrans trans_tot () const;

  /**
   *  @brief Return true, if this selection represents a cell instance, not a shape
   */
  bool is_cell_inst () const
  {
    return m_layer < 0;
  }

  /**
   *  @brief Remove the given number of elements from the front of the path
   *
   *  Effectively retargets the path to a new top cell
   */
  void remove_front (unsigned int n);

  /**
   *  @brief Insert the given element at the front setting the top cell to a new one
   */
  void insert_front (db::cell_index_type topcell, const db::InstElement &elem);

  /**
   *  @brief The "begin" iterator for the path
   */
  non_const_iterator begin ()
  {
    return m_path.begin ();
  }

  /**
   *  @brief The "end" iterator for the path
   */
  non_const_iterator end ()
  {
    return m_path.end ();
  }

  /**
   *  @brief The "begin" iterator for the path
   */
  iterator begin () const
  {
    return m_path.begin ();
  }

  /**
   *  @brief The "end" iterator for the path
   */
  iterator end () const
  {
    return m_path.end ();
  }

  /**
   *  @brief The "back" element of the path
   */
  db::InstElement &back () 
  {
    return m_path.back ();
  }

  /**
   *  @brief The "back" element of the path
   */
  const db::InstElement &back () const
  {
    return m_path.back ();
  }

  /** 
   *  @brief Comparison operator
   */
  bool operator< (const ObjectInstPath &d) const;

  /** 
   *  @brief Comparison operator
   */
  bool operator== (const ObjectInstPath &d) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const ObjectInstPath &d) const
  {
    return !operator== (d);
  }

  /**
   *  @brief The sequence number of this instance path object
   *
   *  The sequence number describes the order in which objects are selected.
   *  The first selected item has sequence index 0.
   */
  unsigned long seq () const
  {
    return m_seq;
  }

  /**
   *  @brief Assign a sequence number
   */
  void set_seq (unsigned long s)
  {
    m_seq = s;
  }

  /**
   *  @brief Gets a value indicating whether the object path is valid
   *
   *  After the layout has been modified, this method is able to check
   *  whether the object path (including shape if applicable) still points
   *  to a valid object.
   */
  bool is_valid (lay::LayoutViewBase *view) const;

private:
  unsigned int m_cv_index;
  db::cell_index_type m_topcell;
  path_type m_path;
  int m_layer;
  unsigned long m_seq;
  db::Shape m_shape;
};

} // namespace lay

#endif

