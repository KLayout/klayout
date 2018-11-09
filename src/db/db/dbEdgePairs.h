
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2018 Matthias Koefferlein

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


#ifndef HDR_dbEdgePairs
#define HDR_dbEdgePairs

#include "dbEdge.h"
#include "dbEdgePair.h"

namespace db
{

class Region;
class Edges;

/**
 *  @brief A set of edge pairs
 *
 *  Edge pairs are convenient object describing a DRC violation. Each edge set
 *  consists of two edges whose relationship is to be marked by objects in the edge
 *  set. Depending on the origin of the edge pairs, the first and second edge
 *  may be derived from one specific source, i.e. one region while the other is
 *  derived from another source.
 *
 *  Edge pair sets are created by Region::width_check for example. Edge pair sets
 *  can be converted to polygons or to individual edges.
 */
class DB_PUBLIC EdgePairs
{
public:
  typedef db::EdgePair edge_pair_type;
  typedef std::vector<edge_pair_type>::const_iterator const_iterator;

  /**
   *  @brief Default constructor
   *
   *  This constructor creates an empty edge pair set.
   */
  EdgePairs ()
  {
    init ();
  }
  
  /**
   *  @brief Equality
   */
  bool operator== (const db::EdgePairs &other) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const db::EdgePairs &other) const
  {
    return !operator== (other);
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const db::EdgePairs &other) const;

  /**
   *  @brief Joining of edge pair sets
   *
   *  This method will combine the edge pairs from "other" with the egdes of "this".
   */
  db::EdgePairs operator+ (const db::EdgePairs &other) const
  {
    db::EdgePairs d (*this);
    d += other;
    return d;
  }

  /**
   *  @brief In-place joining of edge pair sets
   */
  db::EdgePairs &operator+= (const db::EdgePairs &other);

  /**
   *  @brief Begin iterator of the edge pair set
   *
   *  The iterator delivers the edge pairs of the edge pair set.
   */
  const_iterator begin () const
  {
    return m_edge_pairs.begin ();
  }

  /**
   *  @brief End iterator of the edge pair set
   *
   *  The iterator delivers the edge pairs of the edge pair set.
   */
  const_iterator end () const
  {
    return m_edge_pairs.end ();
  }

  /**
   *  @brief Insert an edge pair into the edge pair set
   */
  void insert (const db::Edge &e1, const db::Edge &e2);

  /**
   *  @brief Insert an edge pair into the edge pair set
   */
  void insert (const edge_pair_type &ep);

  /**
   *  @brief Returns true if the edge pair set is empty
   */
  bool empty () const
  {
    return m_edge_pairs.empty ();
  }

  /**
   *  @brief Returns the number of edge pairs in the edge pair set
   */
  size_t size () const
  {
    return m_edge_pairs.size ();
  }

  /**
   *  @brief Returns a string representing the edge pair set
   */
  std::string to_string (size_t nmax = 10) const;

  /**
   *  @brief Clear the edge pair set
   */
  void clear ();

  /**
   *  @brief Reserve memory for the given number of polygons
   */
  void reserve (size_t n)
  {
    m_edge_pairs.reserve (n);
  }

  /**
   *  @brief Returns the bounding box of the edge pair set
   */
  Box bbox () const
  {
    ensure_bbox_valid ();
    return m_bbox;
  }

  /**
   *  @brief Filters the edge pair set 
   *
   *  This method will keep all edge pairs for which the filter returns true.
   */
  template <class F>
  EdgePairs &filter (F &filter)
  {
    std::vector <edge_pair_type>::iterator ew = m_edge_pairs.begin ();
    for (const_iterator e = begin (); e != end (); ++e) {
      if (filter (*e)) {
        *ew++ = *e;
      }
    }
    m_edge_pairs.erase (ew, m_edge_pairs.end ());
    return *this;
  }

  /**
   *  @brief Returns the filtered edge pairs
   *
   *  This method will return a new region with only those edge pairs which 
   *  conform to the filter criterion.
   */
  template <class F>
  EdgePairs filtered (F &filter) const
  {
    EdgePairs d;
    for (const_iterator e = begin (); e != end (); ++e) {
      if (filter (*e)) {
        d.insert (*e);
      }
    }
    return d;
  }

  /**
   *  @brief Transform the edge pair set
   */
  template <class T>
  EdgePairs &transform (const T &trans)
  {
    for (std::vector <edge_pair_type>::iterator e = m_edge_pairs.begin (); e != m_edge_pairs.end (); ++e) {
      e->transform (trans);
    }
    m_bbox_valid = false;
    return *this;
  }

  /**
   *  @brief Returns the transformed edge pair set
   */
  template <class T>
  EdgePairs transformed (const T &trans) const
  {
    EdgePairs d (*this);
    d.transform (trans);
    return d;
  }

  /**
   *  @brief Swaps with the other edge pair set
   */
  void swap (db::EdgePairs &other)
  {
    std::swap (m_edge_pairs, other.m_edge_pairs);
    std::swap (m_bbox, other.m_bbox);
    std::swap (m_bbox_valid, other.m_bbox_valid);
  }

  /**
   *  @brief Converts to polygons
   *
   *  Note: because of the include hierarchy we can't use a direct return value.
   *  
   *  The output container is not cleared by this method but polygons are rather
   *  appended.
   *
   *  The given extension is applied to the edges in parallel and perpendicular direction.
   *  This way a minimum extension is achieved which converts vanishing edge pairs to polygons
   *  with an area.
   */
  void polygons (Region &output, db::Coord e = 0) const;

  /**
   *  @brief Returns individual edges
   *
   *  Note: because of the include hierarchy we can't use a direct return value.
   *  
   *  The output container is not cleared by this method but edges are rather
   *  appended.
   */
  void edges (Edges &output) const;

  /**
   *  @brief Returns the first edges
   *
   *  Note: because of the include hierarchy we can't use a direct return value.
   *
   *  The output container is not cleared by this method but edges are rather
   *  appended.
   */  
  void first_edges (Edges &output) const;

  /**
   *  @brief Returns the second edges
   *
   *  Note: because of the include hierarchy we can't use a direct return value.
   *
   *  The output container is not cleared by this method but edges are rather
   *  appended.
   */  
  void second_edges (Edges &output) const;

  /**
   *  @brief Enable progress reporting
   *
   *  @param progress_text The description text of the progress object
   */
  void enable_progress (const std::string &progress_desc = std::string ());

  /**
   *  @brief Disable progress reporting
   */
  void disable_progress ();

private:
  std::vector<edge_pair_type> m_edge_pairs;
  mutable db::Box m_bbox;
  mutable bool m_bbox_valid;
  bool m_report_progress;
  std::string m_progress_desc;

  void init ();
  void ensure_bbox_valid () const;
};

}

namespace tl 
{
  /**
   *  @brief The type traits for the box type
   */
  template <>
  struct type_traits <db::EdgePairs> : public type_traits<void> 
  {
    typedef true_tag supports_extractor;
    typedef true_tag supports_to_string;
    typedef true_tag has_less_operator;
    typedef true_tag has_equal_operator;
  };

}

#endif

