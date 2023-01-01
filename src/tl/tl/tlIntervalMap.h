
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
#include <algorithm>

namespace tl
{

/**
 *  @brief A helper compare function for the interval map
 */
template <class I, class T>
class iv_compare_f
{
public:
  bool operator() (const std::pair< std::pair<I, I>, T> &p, const I &i) const
  {
    return !(i < p.first.second);
  }
};

/**
 *  @brief An interval map
 */
template <class I, class T>
class interval_map
{
public:
  typedef std::pair<I, I> index_pair;
  typedef std::pair<index_pair, T> index_value_pair;
  typedef std::vector<index_value_pair> index_map;
  typedef typename index_map::const_iterator const_iterator;
  typedef typename index_map::iterator iterator;

  /**
   *  @brief Add values/intervals from the sequence [from,to)
   */
  template <class Iter, class J>
  void add (Iter from, Iter to, J &j)
  {
    for (Iter i = from; i != to; ++i) {
      add (i->first.first, i->first.second, i->second, j);
    }
  }

  /**
   *  @brief Add a value t for the interval [i1,i2)
   *
   *  If the interval overlaps with an already existing one
   *  the joining operator's operator() (T &a, const T &b)
   *  method is employed to join the existing value a with the
   *  new value b.
   *
   *  @param i1 The first index of the interval
   *  @param i2 The second index of the interval
   *  @param t The value to insert
   *  @param j The joining operator 
   *  @param compress Set this parameter to true to enable compression of the intervals where possible
   */
  template <class J> 
  void add (I i1, I i2, const T &t, J &j) 
  {
    typename index_map::iterator lb = std::lower_bound (m_index_map.begin (), m_index_map.end (), i1, iv_compare_f<I, T> ());
    size_t ni = std::distance (m_index_map.begin (), lb);

    while (i1 < i2) {

      if (lb == m_index_map.end () || ! (lb->first.first < i2)) {

        //  current and new interval do not overlap: create new interval(s):
        //  we can simply insert, because we create a new entry 
        lb = m_index_map.insert (lb, index_value_pair (index_pair (i1, i2), t));
        ++lb;
        i1 = i2;

      } else {
      
        //  current and new interval do overlap: create new interval(s)

        //  the part of [i1,i2) before *lb ..
        if (i1 < lb->first.first) {
          I ii = lb->first.first;
          lb = m_index_map.insert (lb, index_value_pair (index_pair (i1, i2), t));
          if (! (i2 < ii)) {
            lb->first.second = ii;
          }
          i1 = ii;
          ++lb;
        }

        //  the part of *lb before [i1',i2) 
        if (lb->first.first < i1) {
          index_value_pair v (*lb); //  this copy is important since the insert may invalid the element *lb!
          lb = m_index_map.insert (lb, v);
          lb->first.second = i1;
          ++lb;
          lb->first.first = i1;
        }

        //  the part of *lb after [i1',i2) 
        if (i2 < lb->first.second) {
          index_value_pair v (*lb); //  this copy is important since the insert may invalid the element *lb!
          lb = m_index_map.insert (lb, v);
          lb->first.second = i2;
          ++lb;
          lb->first.first = i2;
          --lb;
        }

        //  join with *lb and increment lb
        j (lb->second, t);
        i1 = lb->first.second;
        ++lb;

      }

    }
        
    //  search for identical intervals that can be joined 
    //  (only the one before the current interval and the one following can participate)

    size_t nf = std::distance (m_index_map.begin (), lb);
    if (nf < m_index_map.size ()) {
      ++nf;
    }

    lb = m_index_map.begin () + (ni > 0 ? ni - 1 : 0);
    while (lb != m_index_map.begin () + nf) {

      typename index_map::iterator lbb = lb;

      do {
        ++lb;
      } while (lb != m_index_map.end () &&
               lbb->first.second == lb->first.first && lbb->second == lb->second);

      //  [lbb..lb[ can be joined: do so
      if (lb != lbb + 1) {
        --lb;
        nf -= std::distance (lbb, lb);
        lb->first.first = lbb->first.first;
        lb = m_index_map.erase (lbb, lb);
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
    typename index_map::iterator lb = std::lower_bound (m_index_map.begin (), m_index_map.end (), i1, iv_compare_f<I, T> ());
    typename index_map::iterator lb0 = lb;

    if (! (i1 < i2)) {
      return;
    }

    size_t n = 0;
    while (lb != m_index_map.end () && lb->first.first < i2) {
      ++n;
      ++lb;
    }
    
    if (n == 0) {

      //  past the end: do nothing

    } else if (n == 1 && lb0->first.first < i1 && i2 < lb0->first.second) {

      //  we are creating a "hole"
      index_value_pair v (*lb0); //  this copy is important since the insert may invalid the element *lb!
      lb0 = m_index_map.insert (lb0, v);
      lb0->first.second = i1;
      ++lb0;
      lb0->first.first = i2;

    } else {

      //  check for remaining parts of intervals
      if (lb0->first.first < i1) {
        //  the first one is overlapping below i1: cut it
        lb0->first.second = i1;
        ++lb0;
      }
      --lb;
      if (i2 < lb->first.second) {
        //  the last one is overlapping above i2: cut it
        lb->first.first = i2; 
      } else {
        ++lb;
      }

      //  erase the intervals no longer needed
      if (lb0 != lb) {
        m_index_map.erase (lb0, lb);
      }

    }
  }

  /** 
   *  @brief Query a mapping
   *
   *  Given the index i, this method will return 0 if there is no mapping
   *  for the given index. Otherwise it will return the address of the 
   *  value. The address is valid until the next add or erase method call
   *  only.
   *
   *  @param i The index to search for
   *  @return The address of the value object or 0 if not mapped
   */
  T *mapped (I i)
  {
    typename index_map::iterator lb = std::lower_bound (m_index_map.begin (), m_index_map.end (), i, iv_compare_f<I, T> ());
    if (lb == m_index_map.end () || i < lb->first.first) {
      return 0;
    } else {
      return &lb->second;
    }
  }

  /** 
   *  @brief Query a mapping (const version)
   *
   *  Given the index i, this method will return 0 if there is no mapping
   *  for the given index. Otherwise it will return the address of the 
   *  value. The address is valid until the next add or erase method call
   *  only.
   *
   *  @param i The index to search for
   *  @return The address of the value object or 0 if not mapped
   */
  const T *mapped (I i) const
  {
    typename index_map::const_iterator lb = std::lower_bound (m_index_map.begin (), m_index_map.end (), i, iv_compare_f<I, T> ());
    if (lb == m_index_map.end () || i < lb->first.first) {
      return 0;
    } else {
      return &lb->second;
    }
  }

  /**
   *  @brief Returns the iterator for a given index
   *
   *  This will return the iterator for the interval which contains the index. If there is no such interval, this method
   *  will return end().
   *
   *  If there is no interval for the given index (not set), the returned iterator will not be end(), but it's
   *  start value will not be less or equal to the index.
   *
   *  @param i The index to search for
   *  @return The iterator to the corresponding interval or end() if there is no such interval
   */
  const_iterator find (I i) const
  {
    return std::lower_bound (m_index_map.begin (), m_index_map.end (), i, iv_compare_f<I, T> ());
  }

  /**
   *  @brief Do a brief check if the structure is sorted
   */
  bool check () const
  {
    typename index_map::const_iterator lb = m_index_map.begin ();
    while (lb != m_index_map.end ()) {
      typename index_map::const_iterator lbb = lb;
      ++lb;
      if (lb != m_index_map.end () && lb->first.first < lbb->first.second) {
        return false;
      }
    }
    return true;
  }

  /**
   *  @brief Returns the size of the map
   */
  size_t size () const
  {
    return m_index_map.size ();
  }

  /**
   *  @brief Clear the index map
   */
  void clear ()
  {
    m_index_map.clear ();
  }

  /**
   *  @brief begin Iterator
   */
  const_iterator begin () const 
  {
    return m_index_map.begin ();
  }

  /**
   *  @brief end Iterator
   */
  const_iterator end () const 
  {
    return m_index_map.end ();
  }

  /**
   *  @brief begin Iterator
   */
  iterator begin () 
  {
    return m_index_map.begin ();
  }

  /**
   *  @brief end Iterator
   */
  iterator end () 
  {
    return m_index_map.end ();
  }

  /**
   *  @brief equality operator
   */
  bool operator== (const interval_map<I, T> &d) const 
  {
    return m_index_map == d.m_index_map;
  }

  /**
   *  @brief inequality operator
   */
  bool operator!= (const interval_map<I, T> &d) const 
  {
    return m_index_map != d.m_index_map;
  }

private:
  index_map m_index_map;
};

} // namespace tl

#endif

