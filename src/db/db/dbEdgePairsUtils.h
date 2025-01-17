
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#ifndef HDR_dbEdgePairsUtils
#define HDR_dbEdgePairsUtils

#include "dbCommon.h"
#include "dbHash.h"
#include "dbEdgePairs.h"
#include "dbBoxScanner.h"
#include "tlSelect.h"

#include <unordered_set>

namespace db {

/**
 *  @brief The operation mode for the interaction filters
 */
enum EdgePairInteractionMode { EdgePairsInteract, EdgePairsInside, EdgePairsOutside };

/**
 *  @brief A predicate defining edge pair a interacts with polygon b
 */
DB_PUBLIC bool edge_pair_interacts (const db::EdgePair &a, const db::Polygon &b);

/**
 *  @brief A predicate defining edge pair a is "inside" polygon b
 */
DB_PUBLIC bool edge_pair_is_inside (const db::EdgePair &a, const db::Polygon &b);

/**
 *  @brief A predicate defining edge pair a is "outside" polygon b
 */
DB_PUBLIC bool edge_pair_is_outside (const db::EdgePair &a, const db::Polygon &b);

/**
 *  @brief A helper class for the edge pair to region interaction functionality which acts as an edge pair receiver
 *
 *  Note: This special scanner uses pointers to two different objects: edge pairs and polygons.
 *  It uses odd value pointers to indicate pointers to polygons and even value pointers to indicate
 *  pointers to edge pairs.
 *
 *  There is a special box converter which is able to sort that out as well.
 */
template <class OutputContainer, class OutputType = typename OutputContainer::value_type>
class edge_pair_to_polygon_interaction_filter
  : public db::box_scanner_receiver2<db::EdgePair, size_t, db::Polygon, size_t>
{
public:
  edge_pair_to_polygon_interaction_filter (OutputContainer *output, EdgePairInteractionMode mode, size_t min_count, size_t max_count)
    : mp_output (output), m_mode (mode), m_min_count (min_count), m_max_count (max_count)
  {
    //  NOTE: "counting" does not really make much sense in Outside mode ...
    m_counting = !(min_count == 1 && max_count == std::numeric_limits<size_t>::max ());
    tl_assert (!m_counting || mode != EdgePairsOutside);
  }

  void finish (const OutputType *o)
  {
    if (m_counting) {

      size_t count = 0;
      auto i = m_counts.find (o);
      if (i != m_counts.end ()) {
        count = i->second;
      }

      bool match = (count >= m_min_count && count <= m_max_count);
      if (match == (m_mode != EdgePairsOutside)) {
        mp_output->insert (*o);
      }

    } else {

      if (m_mode == EdgePairsOutside && m_seen.find (o) == m_seen.end ()) {
        mp_output->insert (*o);
      }

    }
  }

  void finish1 (const db::EdgePair *o, size_t /*p*/)
  {
    const OutputType *ep = 0;
    tl::select (ep, o, (const db::Polygon *) 0);
    if (ep) {
      finish (ep);
    }
  }

  void finish2 (const db::Polygon *o, size_t /*p*/)
  {
    const OutputType *ep = 0;
    tl::select (ep, (const db::EdgePair *) 0, o);
    if (ep) {
      finish (ep);
    }
  }

  void add (const db::EdgePair *e, size_t, const db::Polygon *p, size_t)
  {
    const OutputType *ep = 0;
    tl::select (ep, e, p);

    if (m_counting) {

      if ((m_mode == EdgePairsInteract && db::edge_pair_interacts (*e, *p)) ||
          (m_mode == EdgePairsInside && db::edge_pair_is_inside (*e, *p)) ||
          (m_mode == EdgePairsOutside && ! db::edge_pair_is_outside (*e, *p))) {

        //  we report the result on "finish" here.
        m_counts[ep] += 1;

      }

    } else if (m_seen.find (ep) == m_seen.end ()) {

      if ((m_mode == EdgePairsInteract && db::edge_pair_interacts (*e, *p)) ||
          (m_mode == EdgePairsInside && db::edge_pair_is_inside (*e, *p))) {

        m_seen.insert (ep);
        mp_output->insert (*ep);

      } else if (m_mode == EdgePairsOutside && ! db::edge_pair_is_outside (*e, *p)) {

        //  In this case we need to collect edges which are outside always - we report those on "finished".
        m_seen.insert (ep);

      }

    }
  }

private:
  OutputContainer *mp_output;
  std::map<const OutputType *, size_t> m_counts;
  std::set<const OutputType *> m_seen;
  EdgePairInteractionMode m_mode;
  size_t m_min_count, m_max_count;
  bool m_counting;
};

/**
 *  @brief A predicate defining edge pair a interacts with edge b
 */
DB_PUBLIC bool edge_pair_interacts (const db::EdgePair &a, const db::Edge &b);

/**
 *  @brief A helper class for the edge pair to region interaction functionality which acts as an edge pair receiver
 *
 *  Note: This special scanner uses pointers to two different objects: edge pairs and polygons.
 *  It uses odd value pointers to indicate pointers to polygons and even value pointers to indicate
 *  pointers to edge pairs.
 *
 *  There is a special box converter which is able to sort that out as well.
 */
template <class OutputContainer, class OutputType = typename OutputContainer::value_type>
class edge_pair_to_edge_interaction_filter
  : public db::box_scanner_receiver2<db::EdgePair, size_t, db::Edge, size_t>
{
public:
  edge_pair_to_edge_interaction_filter (OutputContainer *output, size_t min_count, size_t max_count)
    : mp_output (output), m_min_count (min_count), m_max_count (max_count)
  {
    m_counting = !(min_count == 1 && max_count == std::numeric_limits<size_t>::max ());
  }

  void finish (const OutputType *o)
  {
    if (m_counting) {

      size_t count = 0;
      auto i = m_counts.find (o);
      if (i != m_counts.end ()) {
        count = i->second;
      }

      bool match = (count >= m_min_count && count <= m_max_count);
      if (match) {
        mp_output->insert (*o);
      }

    }
  }

  void finish1 (const db::EdgePair *o, size_t /*p*/)
  {
    const OutputType *ep = 0;
    tl::select (ep, o, (const db::Edge *) 0);
    if (ep) {
      finish (ep);
    }
  }

  void finish2 (const db::Edge *o, size_t /*p*/)
  {
    const OutputType *ep = 0;
    tl::select (ep, (const db::EdgePair *) 0, o);
    if (ep) {
      finish (ep);
    }
  }

  void add (const db::EdgePair *e, size_t, const db::Edge *p, size_t)
  {
    const OutputType *ep = 0;
    tl::select (ep, e, p);

    if (m_counting) {

      if (db::edge_pair_interacts (*e, *p)) {

        //  we report the result on "finish" here.
        m_counts[ep] += 1;

      }

    } else if (m_seen.find (ep) == m_seen.end ()) {

      if (db::edge_pair_interacts (*e, *p)) {

        m_seen.insert (ep);
        mp_output->insert (*ep);

      }

    }
  }

private:
  OutputContainer *mp_output;
  std::map<const OutputType *, size_t> m_counts;
  std::set<const OutputType *> m_seen;
  EdgePairInteractionMode m_mode;
  size_t m_min_count, m_max_count;
  bool m_counting;
};

} // namespace db

#endif
