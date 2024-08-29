
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#ifndef HDR_dbBinnedAreaCollector
#define HDR_dbBinnedAreaCollector

#include "dbCommon.h"

#include "dbEdgeProcessor.h"
#include "tlBitSetMap.h"

namespace db {

/**
 *  @brief The receiver for the binned partial areas
 *
 *  See description of binned_area_collector for details.
 */
template <class Value>
class DB_PUBLIC binned_area_receiver
{
public:
  typedef db::coord_traits<db::Coord>::area_type area_type;

  /**
   *  @brief Constructor
   */
  binned_area_receiver<Value> () { }

  /**
   *  @brief Destructor
   */
  virtual ~binned_area_receiver () { }

  /**
   *  @brief This method gets called when the scanline process starts
   */
  virtual void start () { }

  /**
   *  @brief This method gets called when the scanline process finishes
   */
  virtual void finish () { }

  /**
   *  @brief Adds some partial area with the given value
   */
  virtual void add_area (area_type /*area*/, const Value & /*value*/) { }
};

/**
 *  @brief A helper class providing an inserter that is the connection between the receiver and the provider
 */
template <class Value>
class DB_PUBLIC binned_area_inserter
{
public:
  typedef db::coord_traits<db::Coord>::area_type area_type;

  binned_area_inserter<Value> (area_type area, binned_area_receiver<Value> *receiver)
    : m_area (area), mp_receiver (receiver)
  {
    //  .. nothing yet ..
  }

  //  methods necessary, so this object can act as an inserter
  binned_area_inserter<Value> &operator* () { return *this; }
  binned_area_inserter<Value> &operator++ (int) { return *this; }

  binned_area_inserter<Value> &operator= (const Value &value)
  {
    mp_receiver->add_area (m_area, value);
    return *this;
  }

private:
  area_type m_area;
  binned_area_receiver<Value> *mp_receiver;
};

/**
 *  @brief Provides the operation and edge receiver part of the binned area collector
 *
 *  Use this object both as the edge operator and as an edge collector.
 *  After running the edge processor, use "area" to obtain the area.
 *
 *  This method collects "binned areas". That is, each field of the area divided by
 *  the edges carries a bit set which is made from the combinations of overlapping
 *  layers. The layers are given by the property number where the number is the
 *  bit set in the bit field. Hence, every field is associated with a bit set.
 *
 *  The Area collector will now report the field's areas for accumulation together with
 *  a field value that is obtained from the bit set map. As the bit set map
 *  may deliver multiple fields, multiple such values can be present for each field.
 *  The areas are reported through the binned_area_receiver object. This object
 *  is supposed to add up the areas in an application specific fashion.
 */
template <class Value>
class DB_PUBLIC binned_area_collector
  : public EdgeEvaluatorBase,
    public EdgeSink
{
public:
  typedef db::coord_traits<db::Coord>::area_type area_type;

  /**
   *  @brief Constructor
   */
  binned_area_collector<Value> (const tl::bit_set_map<Value> &bsm, binned_area_receiver<Value> &receiver)
    : mp_bsm (&bsm), mp_receiver (&receiver), m_state_one_bits (0), m_prev_one_bits (0)
  {
    //  .. nothing yet ..
  }

  //  implementation of EdgeEvaluatorBase
  virtual void reset ()
  {
    m_prev = tl::BitSet ();
    m_state = tl::BitSet ();
    m_state_one_bits = 0;
    m_prev_one_bits = 0;
  }

  virtual void begin_group ()
  {
    m_prev = m_state;
    m_prev_one_bits = m_state_one_bits;
  }

  virtual int edge (bool north, bool enter, property_type p)
  {
    if (north) {

      while (m_counts.size () <= p) {
        m_counts.push_back (0);
      }

      int &count = m_counts [p];
      if (enter) {
        if (count == 0) {
          m_state.set (p);
          ++m_state_one_bits;
        }
        ++count;
      } else {
        --count;
        if (count == 0) {
          m_state.reset (p);
          --m_state_one_bits;
        }
      }

      //  this will call "put" when the group is finished
      return 1;

    } else {
      return 0;
    }
  }

  virtual bool is_reset () const
  {
    return m_state_one_bits == 0;
  }

  virtual bool prefer_touch () const
  {
    //  leave events come before enter events
    return false;
  }

  virtual bool selects_edges () const
  {
    //  select_edge is not needed
    return false;
  }

  //  implementation of EdgeSink

  virtual void start ()
  {
    mp_receiver->start ();
  }

  virtual void flush ()
  {
    mp_receiver->finish ();
  }

  virtual void put (const db::Edge &edge)
  {
    area_type partial_area = area_type (edge.p1 ().x () + edge.p2 ().x ()) * area_type (edge.dy ()) * 0.5;
    if (m_prev_one_bits > 0) {
      mp_bsm->lookup (m_prev, binned_area_inserter<Value> (partial_area, mp_receiver));
    }
    if (m_state_one_bits > 0) {
      mp_bsm->lookup (m_state, binned_area_inserter<Value> (-partial_area, mp_receiver));
    }
  }

private:
  area_type m_area_sum;
  const tl::bit_set_map<Value> *mp_bsm;
  binned_area_receiver<Value> *mp_receiver;
  tl::BitSet m_prev, m_state;
  std::vector<int> m_counts;
  unsigned int m_state_one_bits, m_prev_one_bits;
};

}

#endif

