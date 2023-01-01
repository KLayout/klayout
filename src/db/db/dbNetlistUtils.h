
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

#ifndef _HDR_dbNetlistUtils
#define _HDR_dbNetlistUtils

#include "dbCommon.h"
#include "dbTypes.h"
#include "dbMemStatistics.h"

#include <map>
#include <string>
#include <set>

namespace db
{

/**
 *  @brief A getter for the ID of an object
 */
template <class T>
struct id_attribute
{
  typedef size_t attr_type;
  size_t operator() (const T *t) const { return t->id (); }
  bool has (const T * /*t*/) const { return true; }
};

/**
 *  @brief A getter for the cluster ID of an object
 */
template <class T>
struct cluster_id_attribute
{
  typedef size_t attr_type;
  attr_type operator() (const T *t) const { return t->cluster_id (); }
  bool has (const T * /*t*/) const { return true; }
};

/**
 *  @brief A getter for the cluster ID of an object
 */
template <class T>
struct cell_index_attribute
{
  typedef db::cell_index_type attr_type;
  attr_type operator() (const T *t) const { return t->cell_index (); }
  bool has (const T * /*t*/) const { return true; }
};

/**
 *  @brief A getter for the name of an object
 */
template <class T>
struct name_attribute
{
  typedef std::string attr_type;
  const attr_type &operator() (const T *t) const { return t->name (); }
  bool has (const T *t) const { return ! t->name ().empty (); }
};

/**
 *  @brief An id-to-object translation table
 */
template <class T, class I, class ATTR>
class object_by_attr
{
public:
  typedef typename ATTR::attr_type attr_type;
  typedef typename I::value_type value_type;

  object_by_attr (T *self, I (T::*bi) (), I (T::*ei) ()) : mp_self (self), m_bi (bi), m_ei (ei), m_valid (false)
  {
    //  .. nothing yet ..
  }

  void invalidate ()
  {
    m_valid = false;
    m_map.clear ();
  }

  value_type *object_by (const attr_type &attr) const
  {
    if (! m_valid) {
      validate ();
    }
    typename std::map<attr_type, value_type *>::const_iterator m = m_map.find (attr);
    return m == m_map.end () ? 0 : m->second;
  }

  /**
   *  @brief Generate memory statistics
   */
  void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, bool no_self = false, void *parent = 0) const
  {
    if (! no_self) {
      stat->add (typeid (*this), (void *) this, sizeof (*this), sizeof (*this), parent, purpose, cat);
    }

    db::mem_stat (stat, purpose, cat, m_map, true, (void *) this);

    for (typename std::map<attr_type, value_type *>::const_iterator i = m_map.begin (); i != m_map.end (); ++i) {
      db::mem_stat (stat, purpose, cat, *i->second, false, (void *) this);
    }
  }

private:
  T *mp_self;
  I (T::*m_bi) ();
  I (T::*m_ei) ();
  mutable bool m_valid;
  mutable std::map<attr_type, value_type *> m_map;

  void validate () const
  {
    ATTR attr;
    m_map.clear ();
    for (I i = (mp_self->*m_bi) (); i != (mp_self->*m_ei) (); ++i) {
      if (attr.has (i.operator-> ())) {
        m_map.insert (std::make_pair (attr (i.operator-> ()), i.operator-> ()));
      }
    }
    m_valid = true;
  }
};

/**
 *  @brief Memory statistics for object_by_attr
 */
template <class T, class I, class ATTR>
inline void mem_stat (MemStatistics *stat, MemStatistics::purpose_t purpose, int cat, const object_by_attr<T, I, ATTR> &x, bool no_self, void *parent)
{
  x.mem_stat (stat, purpose, cat, no_self, parent);
}

}

#endif
