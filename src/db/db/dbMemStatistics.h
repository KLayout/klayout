
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



#ifndef HDR_dbMemStatistics
#define HDR_dbMemStatistics

#include "dbCommon.h"
#include "tlObjectCollection.h"

#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>
#include <unordered_map>
#include <unordered_set>
#include <typeinfo>
#include "tlReuseVector.h"

namespace tl
{
  template <class> class vector;
  template <class, bool> class reuse_vector;
  class Variant;
}

namespace db
{

/**
 *  @brief A collector for memory statistics
 *  This interface implements the collector for memory statistics.
 */
class DB_PUBLIC MemStatistics 
{
public:
  MemStatistics ();

  enum purpose_t
  {
    None,
    LayoutInfo,
    CellInfo,
    Instances,
    InstTrees,
    ShapesInfo,
    ShapesCache,
    ShapeTrees,
    Netlist,
    LayoutToNetlist
  };

  /**
   *  @brief Adds a memory block for a specific object
   *  The object has a purpose (general category), a detailed category (i.e.
   *  cell index, layer index etc.), a type, a pointer and a size.
   *  "used" can be a value less than "size" to indicate that no
   *  all of the chunk is used. "parent" is a parent object. This pointer
   *  can indicate that the chunk is a part of another object.
   *  "purpose" and "cat can be inherited by the parent.
   */
  virtual void add (const std::type_info & /*ti*/, void * /*ptr*/, size_t /*size*/, size_t /*used*/, void * /*parent*/, purpose_t /*purpose*/ = None, int /*cat*/ = 0) { }
};

/**
 *  @brief A generic memory statistics collector
 *  This collector will collect the summary of memory usage only.
 */
class DB_PUBLIC MemStatisticsCollector
  : public MemStatistics
{
public:
  MemStatisticsCollector (bool detailed);

  /**
   *  @brief Prints the statistics
   */
  void print ();

  virtual void add (const std::type_info &ti, void *ptr, size_t size, size_t used, void *parent, purpose_t purpose, int cat);

private:
  bool m_detailed;
  std::map<const std::type_info *, std::pair<size_t, size_t> > m_per_type;
  std::map<std::pair<purpose_t, int>, std::pair<size_t, size_t> > m_per_cat;
  std::map<purpose_t, std::pair<size_t, size_t> > m_per_purpose;
};

/**
 *  @brief A simple memory statistics collector
 *  This collector will simply add the size required
 */
class DB_PUBLIC MemStatisticsSimple
  : public MemStatistics
{
public:
  MemStatisticsSimple ()
    : m_size (0), m_used (0)
  { }

  size_t size () const
  {
    return m_size;
  }

  size_t used () const
  {
    return m_used;
  }

  virtual void add (const std::type_info & /*ti*/, void * /*ptr*/, size_t size, size_t used, void * /*parent*/, purpose_t /*purpose*/, int /*cat*/)
  {
    m_size += size;
    m_used += used;
  }

  template <class T>
  MemStatisticsSimple &operator<< (const T &x)
  {
    mem_stat (this, None, 0, x);
    return *this;
  }

private:
  size_t m_size, m_used;
};

//  Some standard templates to collect the information
template <class X>
void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const X &x, bool no_self = false, void *parent = 0)
{
  if (! no_self) {
    stat->add (typeid (X), (void *) &x, sizeof (X), sizeof (X), parent, purpose, cat);
  }
}

void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const std::string &x, bool no_self = false, void *parent = 0);

void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const tl::Variant &x, bool no_self = false, void *parent = 0);

template <class X>
void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const tl::reuse_vector<X> &v, bool no_self = false, void *parent = 0)
{
  if (! no_self) {
    stat->add (typeid (v), (void *) &v, sizeof (v), sizeof (v), parent, purpose, cat);
  }
  if (! v.empty ()) {
    stat->add (typeid (X[]), (void *) v.begin ().operator-> (), sizeof (X) * v.capacity (), sizeof (X) * v.size (), (void *) &v, purpose, cat);
  }
  if (v.reuse_data ()) {
    stat->add (typeid (tl::ReuseData), (void *) v.reuse_data (), v.reuse_data ()->mem_reqd (), v.reuse_data ()->mem_used (), (void *) &v, purpose, cat);
  }
  for (typename tl::reuse_vector<X>::const_iterator e = v.begin (); e != v.end (); ++e) {
    mem_stat (stat, purpose, cat, *e, true, (void *) &v);
  }
}

template <class X>
void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const tl::vector<X> &v, bool no_self = false, void *parent = 0)
{
  if (! no_self) {
    stat->add (typeid (v), (void *) &v, sizeof (v), sizeof (v), parent, purpose, cat);
  }
  if (! v.empty ()) {
    stat->add (typeid (X[]), (void *) &v.front (), sizeof (X) * v.capacity (), sizeof (X) * v.size (), (void *) &v, purpose, cat);
  }
  for (size_t i = 0; i < v.size (); ++i) {
    mem_stat (stat, purpose, cat, v[i], true, (void *) &v);
  }
}

template <class X>
void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const std::vector<X> &v, bool no_self = false, void *parent = 0)
{
  if (! no_self) {
    stat->add (typeid (v), (void *) &v, sizeof (v), sizeof (v), parent, purpose, cat);
  }
  if (! v.empty ()) {
    stat->add (typeid (X[]), (void *) &v.front (), sizeof (X) * v.capacity (), sizeof (X) * v.size (), (void *) &v, purpose, cat);
  }
  for (size_t i = 0; i < v.size (); ++i) {
    mem_stat (stat, purpose, cat, v[i], true, (void *) &v);
  }
}

void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const std::vector<bool> &x, bool no_self = false, void *parent = 0);

template <class X, class Y>
void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const std::map<X, Y> &v, bool no_self = false, void *parent = 0)
{
  if (! no_self) {
    stat->add (typeid (v), (void *) &v, sizeof (v), sizeof (v), parent, purpose, cat);
  }
  for (typename std::map<X, Y>::const_iterator i = v.begin (); i != v.end (); ++i) {
    mem_stat (stat, purpose, cat, i->first, false, (void *) &v);
    mem_stat (stat, purpose, cat, i->second, false, (void *) &v);
#ifdef __GLIBCXX__
    stat->add (typeid (std::_Rb_tree_node_base), (void *) &i->first, sizeof (std::_Rb_tree_node_base), sizeof (std::_Rb_tree_node_base), (void *) &v, purpose, cat);
#endif
  }
}

template <class X, class Y>
void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const std::unordered_map<X, Y> &v, bool no_self = false, void *parent = 0)
{
  if (! no_self) {
    stat->add (typeid (v), (void *) &v, sizeof (v), sizeof (v), parent, purpose, cat);
  }
  for (typename std::unordered_map<X, Y>::const_iterator i = v.begin (); i != v.end (); ++i) {
    mem_stat (stat, purpose, cat, i->first, false, (void *) &v);
    mem_stat (stat, purpose, cat, i->second, false, (void *) &v);
    //  TODO: add intrinsic overhead
  }
}

template <class X>
void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const std::set<X> &v, bool no_self = false, void *parent = 0)
{
  if (! no_self) {
    stat->add (typeid (v), (void *) &v, sizeof (v), sizeof (v), parent, purpose, cat);
  }
  for (typename std::set<X>::const_iterator i = v.begin (); i != v.end (); ++i) {
    mem_stat (stat, purpose, cat, *i, false, (void *) &v);
#ifdef __GLIBCXX__
    //  NOTE: the pointer is only an approximation
    stat->add (typeid (std::_Rb_tree_node_base), (void *) i.operator-> (), sizeof (std::_Rb_tree_node_base), sizeof (std::_Rb_tree_node_base), (void *) &v, purpose, cat);
#endif
  }
}

template <class X>
void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const std::unordered_set<X> &v, bool no_self = false, void *parent = 0)
{
  if (! no_self) {
    stat->add (typeid (v), (void *) &v, sizeof (v), sizeof (v), parent, purpose, cat);
  }
  for (typename std::unordered_set<X>::const_iterator i = v.begin (); i != v.end (); ++i) {
    mem_stat (stat, purpose, cat, *i, false, (void *) &v);
    //  TODO: add intrinsic overhead
  }
}

template <class X>
void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const std::list<X> &v, bool no_self = false, void *parent = 0)
{
  if (! no_self) {
    stat->add (typeid (v), (void *) &v, sizeof (v), sizeof (v), parent, purpose, cat);
  }
  for (typename std::list<X>::const_iterator i = v.begin (); i != v.end (); ++i) {
    mem_stat (stat, purpose, cat, *i, false, (void *) &v);
#ifdef __GLIBCXX__
    //  NOTE: the pointer is only an approximation
    stat->add (typeid (std::__detail::_List_node_base), (void *) i.operator-> (), sizeof (std::__detail::_List_node_base), sizeof (std::__detail::_List_node_base), (void *) &v, purpose, cat);
#endif
  }
}

template <class X, class Y>
void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const std::pair<X, Y> &v, bool no_self = false, void *parent = 0)
{
  if (! no_self) {
    stat->add (typeid (v), (void *) &v, sizeof (v), sizeof (v), parent, purpose, cat);
  }
  mem_stat (stat, purpose, cat, v.first, true, (void *) &v);
  mem_stat (stat, purpose, cat, v.second, true, (void *) &v);
}

template <class X>
void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const tl::shared_collection<X> &v, bool no_self = false, void *parent = 0)
{
  if (! no_self) {
    stat->add (typeid (v), (void *) &v, sizeof (v), sizeof (v), parent, purpose, cat);
  }
  size_t intrinsic = sizeof (typename tl::shared_collection<X>::holder_type) * v.size ();
  stat->add (typeid (typename tl::shared_collection<X>::holder_type), (void *) &v, intrinsic, intrinsic, (void *) &v, purpose, cat);
  for (typename tl::shared_collection<X>::const_iterator i = v.begin (); i != v.end (); ++i) {
    mem_stat (stat, purpose, cat, *i, false, (void *) &v);
  }
}

template <class X>
void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const tl::weak_collection <X> &v, bool no_self = false, void *parent = 0)
{
  if (! no_self) {
    stat->add (typeid (v), (void *) &v, sizeof (v), sizeof (v), parent, purpose, cat);
  }
  size_t intrinsic = sizeof (typename tl::weak_collection<X>::holder_type) * v.size ();
  stat->add (typeid (typename tl::weak_collection<X>::holder_type), (void *) &v, intrinsic, intrinsic, (void *) &v, purpose, cat);
}

}

#endif

