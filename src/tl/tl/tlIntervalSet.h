
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



#ifndef HDR_tlIntervalMap
#define HDR_tlIntervalMap

#include <vector>

namespace tl
{

/**
 *  @brief A helper compare function for the interval map
 *
 *  This version will also finds preceding intervals
 */
template <class I>
class is_compare_f
{
public:
  bool operator() (const std::pair<I, I> &p, const I &i) const
  {
    return p.second < i;
  }
};

/**
 *  @brief A helper compare function for the interval map 
 *
 *  This version will not find preceding intervals
 */
template <class I>
class is_compare_f2
{
public:
  bool operator() (const std::pair<I, I> &p, const I &i) const
  {
    return !(i < p.second);
  }
};

/**
 *  @brief An interval map
 */
template <class I>
class interval_set
{
public:
  typedef std::pair<I, I> index_pair;
  typedef std::vector<index_pair> index_set;
  typedef typename index_set::const_iterator const_iterator;

  /**
   *  @brief Add intervals from the sequence [from,to)
   */
  template <class Iter>
  void add (Iter from, Iter to)
  {
    for (Iter i = from; i != to; ++i) {
      add (i->first, i->second);
    }
  }

  /**
   *  @brief Add the interval [i1,i2)
   *
   *  @param i1 The first index of the interval
   *  @param i2 The second index of the interval
   */
  void add (I i1, I i2)
  {
    typename index_set::iterator lb = std::lower_bound (m_index_set.begin (), m_index_set.end (), i1, is_compare_f<I> ());

    if (lb == m_index_set.end () || ! (lb->first < i2)) {

      //  current and new interval do not overlap: create new interval(s):
      //  we can simply insert, because we create a new entry 
      m_index_set.insert (lb, index_pair (i1, i2));

    } else {
      
      //  current and new interval do overlap: extend the intervals
      //  and join others if required

      if (i1 < lb->first) {
        lb->first = i1;
      }

      typename index_set::iterator lb0 = lb;
      while (lb != m_index_set.end () && !(i2 < lb->first)) {
        if (i2 < lb->second) {
          i2 = lb->second;
        }
        ++lb;
      }

      lb0->second = i2;

      ++lb0;
      if (lb0 != lb) {
        m_index_set.erase (lb0, lb);
      } 

    }

  }

  /**
   *  @brief Erase an interval
   *
   *  Erases the interval [i1,i2) (set to unmapped)
   */
  void erase (I i1, I i2)
  {
    typename index_set::iterator lb = std::lower_bound (m_index_set.begin (), m_index_set.end (), i1, is_compare_f2<I> ());
    typename index_set::iterator lb0 = lb;

    if (! (i1 < i2)) {
      return;
    }

    size_t n = 0;
    while (lb != m_index_set.end () && lb->first < i2) {
      ++n;
      ++lb;
    }
    
    if (n == 0) {

      //  past the end: do nothing

    } else if (n == 1 && lb0->first < i1 && i2 < lb0->second) {

      //  we are creating a "hole"
      index_pair v (*lb0); //  this copy is important since the insert may invalid the element *lb!
      lb0 = m_index_set.insert (lb0, v);
      lb0->second = i1;
      ++lb0;
      lb0->first = i2;

    } else {

      //  check for remaining parts of intervals
      if (lb0->first < i1) {
        //  the first one is overlapping below i1: cut it
        lb0->second = i1;
        ++lb0;
      }
      --lb;
      if (lb != m_index_set.end () && i2 < lb->second) {
        //  the last one is overlapping above i2: cut it
        lb->first = i2; 
      } else {
        ++lb;
      }

      //  erase the intervals no longer needed
      if (lb0 != lb) {
        m_index_set.erase (lb0, lb);
      }

    }
  }

  /** 
   *  @brief Query a mapping
   *
   *  Given the index i, this method will return false if there is no mapping
   *  and true otherwise.
   *
   *  @param i The index to search for
   *  @return The address of the value object or 0 if not mapped
   */
  bool mapped (I i) const
  {
    typename index_set::const_iterator lb = std::lower_bound (m_index_set.begin (), m_index_set.end (), i, is_compare_f2<I> ());
    return (lb != m_index_set.end () && ! (i < lb->first));
  }

  /**
   *  @brief Do a brief check if the structure is sorted
   */
  bool check () const
  {
    typename index_set::const_iterator lb = m_index_set.begin ();
    while (lb != m_index_set.end ()) {
      typename index_set::const_iterator lbb = lb;
      ++lb;
      if (lb != m_index_set.end () && !(lbb->second < lb->first)) {
        return false;
      }
    }
    return true;
  }

  /**
   *  @brief Clear the index map
   */
  void clear ()
  {
    m_index_set.clear ();
  }

  /**
   *  @brief begin Iterator
   */
  const_iterator begin () const 
  {
    return m_index_set.begin ();
  }

  /**
   *  @brief end Iterator
   */
  const_iterator end () const 
  {
    return m_index_set.end ();
  }

  /**
   *  @brief equality operator
   */
  bool operator== (const interval_set<I> &d) const 
  {
    return m_index_set == d.m_index_set;
  }

  /**
   *  @brief inequality operator
   */
  bool operator!= (const interval_set<I> &d) const 
  {
    return m_index_set != d.m_index_set;
  }

private:
  index_set m_index_set;
};

} // namespace tl

#endif

