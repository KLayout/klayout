
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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

#include <stdio.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <list>

namespace tl
{
  template <class X> class vector;
  template <class X> class reuse_vector;
}

namespace db
{

template <class X> 
size_t mem_used (const X &)
{
  return sizeof (X);
}

template <class X> 
size_t mem_reqd (const X &)
{
  return sizeof (X);
}

size_t mem_used (const std::string &s);
size_t mem_reqd (const std::string &s);

template <class X>
size_t mem_used (const tl::reuse_vector<X> &v)
{
  size_t s = v.mem_used ();
  for (typename tl::reuse_vector<X>::const_iterator e = v.begin (); e != v.end (); ++e) {
    s += mem_used (*e);
  }
  return s;
}

template <class X>
size_t mem_reqd (const tl::reuse_vector<X> &v)
{
  size_t s = v.mem_reqd ();
  for (typename tl::reuse_vector<X>::const_iterator e = v.begin (); e != v.end (); ++e) {
    s += mem_used (*e);
  }
  return s;
}

template <class X>
size_t mem_used (const tl::vector<X> &v)
{
  size_t s = sizeof (tl::vector<X>);
  size_t i = 0;
  for (i = 0; i < v.size (); ++i) {
    s += mem_used (v[i]);
  }
  s += sizeof (X) * (v.capacity () - v.size ());
  return s;
}

template <class X>
size_t mem_reqd (const tl::vector<X> &v)
{
  size_t s = sizeof (tl::vector<X>);
  size_t i = 0;
  for (i = 0; i < v.size (); ++i) {
    s += mem_reqd (v[i]);
  }
  return s;
}

template <class X>
size_t mem_used (const std::vector<X> &v)
{
  size_t s = sizeof (std::vector<X>);
  size_t i = 0;
  for (i = 0; i < v.size (); ++i) {
    s += mem_used (v[i]);
  }
  s += sizeof (X) * (v.capacity () - v.size ());
  return s;
}

template <class X>
size_t mem_reqd (const std::vector<X> &v)
{
  size_t s = sizeof (std::vector<X>);
  size_t i = 0;
  for (i = 0; i < v.size (); ++i) {
    s += mem_reqd (v[i]);
  }
  return s;
}

size_t mem_used (const std::vector<bool> &v);
size_t mem_reqd (const std::vector<bool> &v);

template <class X, class Y>
size_t mem_used (const std::map<X, Y> &m)
{
  size_t s = sizeof (std::map<X, Y>);
  for (typename std::map<X, Y>::const_iterator i = m.begin (); i != m.end (); ++i) {
    s += mem_used(i->first) + mem_used(i->second);
#ifdef __GNUCC__
    s += sizeof (std::_Rb_tree_node_base);
#endif
  }
  return s;
}

template <class X, class Y>
size_t mem_reqd (const std::map<X, Y> &m)
{
  size_t s = sizeof (std::map<X, Y>);
  for (typename std::map<X, Y>::const_iterator i = m.begin (); i != m.end (); ++i) {
    s += mem_reqd(i->first) + mem_reqd(i->second);
#ifdef __GNUCC__
    s += sizeof (std::_Rb_tree_node_base);
#endif
  }
  return s;
}

template <class X>
size_t mem_used (const std::set<X> &x)
{
  size_t s = sizeof (std::set<X>);
  for (typename std::set<X>::const_iterator i = x.begin (); i != x.end (); ++i) {
    s += mem_used(*i);
#ifdef __GNUCC__
    s += sizeof (std::_Rb_tree_node_base);
#endif
  }
  return s;
}

template <class X>
size_t mem_reqd (const std::set<X> &x)
{
  size_t s = sizeof (std::set<X>);
  for (typename std::set<X>::const_iterator i = x.begin (); i != x.end (); ++i) {
    s += mem_reqd(*i);
#ifdef __GNUCC__
    s += sizeof (std::_Rb_tree_node_base);
#endif
  }
  return s;
}

template <class X>
size_t mem_used (const std::list<X> &l)
{
  size_t s = sizeof (std::list<X>);
  for (typename std::list<X>::const_iterator i = l.begin (); i != l.end (); ++i) {
    s += mem_used(*i);
#ifdef __GNUCC__
    s += sizeof (std::_List_node_base);
#endif
  }
  return s;
}

template <class X>
size_t mem_reqd (const std::list<X> &l)
{
  size_t s = sizeof (std::list<X>);
  for (typename std::list<X>::const_iterator i = l.begin (); i != l.end (); ++i) {
    s += mem_reqd(*i);
#ifdef __GNUCC__
    s += sizeof (std::_List_node_base);
#endif
  }
  return s;
}

class DB_PUBLIC MemStatistics 
{
public:
  MemStatistics ();

  void print () const;

  void layout_info (size_t u, size_t r)
  {
    m_layout_info_used += u;
    m_layout_info_reqd += r;
  }

  template <class X>
  void layout_info (const X &x) 
  {
    m_layout_info_used += mem_used (x);
    m_layout_info_reqd += mem_reqd (x);
  }

  void cell_info (size_t u, size_t r)
  {
    m_cell_info_used += u;
    m_cell_info_reqd += r;
  }

  template <class X>
  void cell_info (const X &x) 
  {
    m_cell_info_used += mem_used (x);
    m_cell_info_reqd += mem_reqd (x);
  }

  void instances (size_t u, size_t r)
  {
    m_instances_used += u;
    m_instances_reqd += r;
  }

  template <class X>
  void instances (const X &x) 
  {
    m_instances_used += mem_used (x);
    m_instances_reqd += mem_reqd (x);
  }

  void inst_trees (size_t u, size_t r)
  {
    m_inst_trees_used += u;
    m_inst_trees_reqd += r;
  }

  template <class X>
  void inst_trees (const X &x) 
  {
    m_inst_trees_used += mem_used (x);
    m_inst_trees_reqd += mem_reqd (x);
  }

  void shapes_info (size_t u, size_t r)
  {
    m_shapes_info_used += u;
    m_shapes_info_reqd += r;
  }

  template <class X>
  void shapes_info (const X &x) 
  {
    m_shapes_info_used += mem_used (x);
    m_shapes_info_reqd += mem_reqd (x);
  }

  void shapes_cache (size_t u, size_t r)
  {
    m_shapes_cache_used += u;
    m_shapes_cache_reqd += r;
  }

  template <class X>
  void shapes_cache (const X &x) 
  {
    m_shapes_cache_used += mem_used (x);
    m_shapes_cache_reqd += mem_reqd (x);
  }

  void shape_trees (size_t u, size_t r)
  {
    m_shape_trees_used += u;
    m_shape_trees_reqd += r;
  }

  template <class X>
  void shape_trees (const X &x) 
  {
    m_shape_trees_used += mem_used (x);
    m_shape_trees_reqd += mem_reqd (x);
  }

private:
  size_t m_layout_info_used, m_layout_info_reqd;
  size_t m_cell_info_used, m_cell_info_reqd;
  size_t m_inst_trees_used, m_inst_trees_reqd;
  size_t m_shapes_info_used, m_shapes_info_reqd;
  size_t m_shapes_cache_used, m_shapes_cache_reqd;
  size_t m_shape_trees_used, m_shape_trees_reqd;
  size_t m_instances_used, m_instances_reqd;
};

}

#endif

