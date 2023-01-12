
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


#ifndef HDR_tlEquivalenceClusters
#define HDR_tlEquivalenceClusters

#include "tlCommon.h"
#include "tlAssert.h"

#include <cstddef>
#include <vector>
#include <map>
#include <set>

namespace tl
{

/**
 *  @brief A utility class forming clusters based on equivalence of a certain attribute
 *
 *  To use this class, feed it with equivalences using "is_same", e.g.
 *
 *  @code
 *  equivalence_clusters<int> eq;
 *  //  forms two clusters: 1,2,5 and 3,4
 *  eq.same (1, 2);
 *  eq.same (3, 4);
 *  eq.same (1, 5);
 *  @endcode
 *
 *  Self-equivalence is a way of introduction an attribute without
 *  an equivalence:
 *
 *  @code
 *  equivalence_clusters<int> eq;
 *  //  after this, 1 is a known attribute
 *  eq.same (1, 1);
 *  eq.has_attribute (1);  //  ->true
 *  @endcode
 *
 *  Equivalence clusters can be merged, forming new and bigger clusters.
 *
 *  Eventually, each cluster is represented by a non-zero integer ID.
 *  The cluster ID can obtained per attribute value using "cluster_id".
 *  In the above example this will be:
 *
 *  @code
 *  eq.cluster_id (1);  //  ->1
 *  eq.cluster_id (2);  //  ->1
 *  eq.cluster_id (3);  //  ->2
 *  eq.cluster_id (4);  //  ->2
 *  eq.cluster_id (5);  //  ->1
 *  eq.cluster_id (6);  //  ->0  (unknown)
 *  @endcode
 *
 *  "size" will give the maximum cluster ID.
 */
template <class T>
class equivalence_clusters
{
public:
  typedef size_t cluster_id_type;
  typedef T attribute_type;
  typedef typename std::vector<typename std::map<T, size_t>::iterator>::const_iterator cluster_iterator;

  /**
   *  @brief Creates an empty equivalence cluster
   */
  equivalence_clusters ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Makes attr1 and attr2 equivalent
   */
  void same (const T &attr1, const T &attr2)
  {
    cluster_id_type cl1 = cluster_id (attr1);

    if (attr1 == attr2) {
      //  special case of "self-identity"
      if (! cl1) {
        cluster_id_type cl = new_cluster ();
        insert (attr1, cl);
      }
      return;
    }

    cluster_id_type cl2 = cluster_id (attr2);
    if (! cl1 || ! cl2) {

      if (cl1) {
        insert (attr2, cl1);
      } else if (cl2) {
        insert (attr1, cl2);
      } else {
        cluster_id_type cl = new_cluster ();
        insert (attr1, cl);
        insert (attr2, cl);
      }

    } else if (cl1 != cl2) {
      join (cl1, cl2);
    }
  }

  /**
   *  @brief Returns true, if attr is part of the equivalence clusters
   */
  bool has_attribute (const T &attr) const
  {
    return m_cluster_id_by_attr.find (attr) != m_cluster_id_by_attr.end ();
  }

  /**
   *  @brief Returns the cluster ID for the given attribute of 0 if the attribute is not assigned to a cluster
   */
  cluster_id_type cluster_id (const T &attr) const
  {
    typename std::map<T, size_t>::const_iterator c = m_cluster_id_by_attr.find (attr);
    if (c != m_cluster_id_by_attr.end ()) {
      return c->second;
    } else {
      return 0;
    }
  }

  /**
   *  @brief Applies the equivalences of the other clusters
   *
   *  This method will join clusters already within this equivalence cluster collection
   *  based on the equivalences listed in the other clusters collection.
   *
   *  In contrast to merge, his method will not introduce new attributes.
   */
  void apply_equivalences (const equivalence_clusters &other)
  {
    std::vector<T> attrs;
    for (typename std::map<T, size_t>::const_iterator a = m_cluster_id_by_attr.begin (); a != m_cluster_id_by_attr.end (); ++a) {
      if (other.has_attribute (a->first)) {
        attrs.push_back (a->first);
      }
    }

    for (typename std::vector<T>::const_iterator a = attrs.begin (); a != attrs.end (); ++a) {
      cluster_id_type cl = other.cluster_id (*a);
      cluster_iterator b = other.begin_cluster (cl);
      cluster_iterator e = other.end_cluster (cl);
      for (cluster_iterator i = b; i != e; ++i) {
        if ((*i)->first != *a && has_attribute ((*i)->first)) {
          same ((*i)->first, *a);
        }
      }
    }
  }

  /**
   *  @brief Merges the equivalences of the other clusters into this
   *
   *  This method will add all attributes from the other cluster collection and join
   *  clusters based on the equivalences there.
   */
  void merge (const equivalence_clusters &other)
  {
    for (cluster_id_type cl = 1; cl <= other.size (); ++cl) {
      cluster_iterator b = other.begin_cluster (cl);
      cluster_iterator e = other.end_cluster (cl);
      for (cluster_iterator i = b; i != e; ++i) {
        same ((*b)->first, (*i)->first);
      }
    }
  }

  /**
   *  @brief Returns the number of clusters kept inside this collection
   */
  size_t size () const
  {
    return m_clusters.size ();
  }

  /**
   *  @brief Begin iterator for the cluster elements for a given cluster ID.
   *  To access, the attribute, use "(*i)->first".
   */
  cluster_iterator begin_cluster (size_t cluster_id) const
  {
    tl_assert (cluster_id > 0);
    return m_clusters [cluster_id - 1].begin ();
  }

  /**
   *  @brief End iterator for the cluster elements for a given cluster ID.
   */
  cluster_iterator end_cluster (size_t cluster_id) const
  {
    tl_assert (cluster_id > 0);
    return m_clusters [cluster_id - 1].end ();
  }

private:
  void insert (const T &attr, cluster_id_type into)
  {
    typename std::map<T, size_t>::iterator c = m_cluster_id_by_attr.insert (std::make_pair (attr, into)).first;
    m_clusters [into - 1].push_back (c);
  }

  void join (cluster_id_type id, cluster_id_type with_id)
  {
    tl_assert (id > 0);
    tl_assert (with_id > 0);
    std::vector<typename std::map<T, size_t>::iterator> &cnew = m_clusters [id - 1];
    std::vector<typename std::map<T, size_t>::iterator> &c = m_clusters [with_id - 1];
    for (typename std::vector<typename std::map<T, size_t>::iterator>::iterator i = c.begin (); i != c.end (); ++i) {
      (*i)->second = id;
      cnew.push_back (*i);
    }

    c.clear ();
    m_free_slots.push_back (with_id);
  }

  cluster_id_type new_cluster ()
  {
    if (! m_free_slots.empty ()) {
      cluster_id_type cl = m_free_slots.back ();
      m_free_slots.pop_back ();
      return cl;
    } else {
      m_clusters.push_back (std::vector<typename std::map<T, size_t>::iterator> ());
      return m_clusters.size ();
    }
  }

  std::map<T, size_t> m_cluster_id_by_attr;
  std::vector<std::vector<typename std::map<T, size_t>::iterator> > m_clusters;
  std::vector<size_t> m_free_slots;
};

}

#endif
