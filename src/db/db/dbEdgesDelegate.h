
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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


#ifndef HDR_dbEdgesDelegate
#define HDR_dbEdgesDelegate

#include "dbCommon.h"

#include "dbEdge.h"
#include "dbEdgePairs.h"
#include "dbEdgePairRelations.h"

#include <list>

namespace db {

/**
 *  @brief A common definition for the boolean operations available on edges
 */
enum EdgeBoolOp { EdgeOr, EdgeNot, EdgeXor, EdgeAnd };

class RecursiveShapeIterator;
class EdgeFilterBase;
class RegionDelegate;

/**
 *  @brief The edge set iterator delegate
 */
class DB_PUBLIC EdgesIteratorDelegate
{
public:
  EdgesIteratorDelegate () { }
  virtual ~EdgesIteratorDelegate () { }

  typedef db::Edge value_type;

  virtual bool at_end () const = 0;
  virtual void increment () = 0;
  virtual const value_type *get () const = 0;
  virtual EdgesIteratorDelegate *clone () const = 0;
};

/**
 *  @brief The delegate for the actual edge set implementation
 */
class DB_PUBLIC EdgesDelegate
{
public:
  typedef db::Coord coord_type;
  typedef db::coord_traits<db::Coord> coord_traits;
  typedef db::Edge edge_type;
  typedef db::Vector vector_type;
  typedef db::Point point_type;
  typedef db::Box box_type;
  typedef coord_traits::distance_type distance_type;
  typedef coord_traits::distance_type length_type;

  EdgesDelegate ();
  virtual ~EdgesDelegate ();

  EdgesDelegate (const EdgesDelegate &other);
  EdgesDelegate &operator= (const EdgesDelegate &other);

  virtual EdgesDelegate *clone () const = 0;

  void set_base_verbosity (int vb);
  int base_verbosity () const
  {
    return m_base_verbosity;
  }

  void enable_progress (const std::string &progress_desc);
  void disable_progress ();

  void set_merged_semantics (bool f);
  bool merged_semantics () const
  {
    return m_merged_semantics;
  }

  void set_strict_handling (bool f);
  bool strict_handling () const
  {
    return m_strict_handling;
  }

  virtual std::string to_string (size_t nmax) const = 0;

  virtual EdgesIteratorDelegate *begin () const = 0;
  virtual EdgesIteratorDelegate *begin_merged () const = 0;

  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_iter () const = 0;
  virtual std::pair<db::RecursiveShapeIterator, db::ICplxTrans> begin_merged_iter () const = 0;

  virtual bool empty () const = 0;
  virtual bool is_merged () const = 0;
  virtual size_t size () const = 0;

  virtual distance_type length (const db::Box &box) const = 0;
  virtual Box bbox () const = 0;

  virtual EdgePairs width_check (db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const = 0;
  virtual EdgePairs space_check (db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const = 0;
  virtual EdgePairs enclosing_check (const Edges &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const = 0;
  virtual EdgePairs overlap_check (const Edges &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const = 0;
  virtual EdgePairs separation_check (const Edges &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const = 0;
  virtual EdgePairs inside_check (const Edges &other, db::Coord d, bool whole_edges, metrics_type metrics, double ignore_angle, distance_type min_projection, distance_type max_projection) const = 0;

  virtual EdgesDelegate *filter_in_place (const EdgeFilterBase &filter) = 0;
  virtual EdgesDelegate *filtered (const EdgeFilterBase &filter) const = 0;

  virtual EdgesDelegate *merged_in_place () = 0;
  virtual EdgesDelegate *merged () const = 0;

  virtual EdgesDelegate *and_with (const Edges &other) const = 0;
  virtual EdgesDelegate *and_with (const Region &other) const = 0;
  virtual EdgesDelegate *not_with (const Edges &other) const = 0;
  virtual EdgesDelegate *not_with (const Region &other) const = 0;
  virtual EdgesDelegate *xor_with (const Edges &other) const = 0;
  virtual EdgesDelegate *or_with (const Edges &other) const = 0;
  virtual EdgesDelegate *add_in_place (const Edges &other) = 0;
  virtual EdgesDelegate *add (const Edges &other) const = 0;

  virtual RegionDelegate *extended (coord_type ext_b, coord_type ext_e, coord_type ext_o, coord_type ext_i, bool join) const = 0;
  virtual EdgesDelegate *start_segments (length_type length, double fraction) const = 0;
  virtual EdgesDelegate *end_segments (length_type length, double fraction) const = 0;
  virtual EdgesDelegate *centers (length_type length, double fraction) const = 0;

  virtual EdgesDelegate *inside_part (const Region &other) const = 0;
  virtual EdgesDelegate *outside_part (const Region &other) const = 0;
  virtual EdgesDelegate *selected_interacting (const Region &other) const = 0;
  virtual EdgesDelegate *selected_not_interacting (const Region &other) const = 0;
  virtual EdgesDelegate *selected_interacting (const Edges &other) const = 0;
  virtual EdgesDelegate *selected_not_interacting (const Edges &other) const = 0;

  virtual EdgesDelegate *in (const Edges &other, bool invert) const = 0;

  virtual const db::Edge *nth (size_t n) const = 0;
  virtual bool has_valid_edges () const = 0;
  virtual bool has_valid_merged_edges () const = 0;

  virtual const db::RecursiveShapeIterator *iter () const = 0;

  virtual bool equals (const Edges &other) const = 0;
  virtual bool less (const Edges &other) const = 0;

  virtual void insert_into (Layout *layout, db::cell_index_type into_cell, unsigned int into_layer) const = 0;

protected:
  const std::string &progress_desc () const
  {
    return m_progress_desc;
  }

  bool report_progress () const
  {
    return m_report_progress;
  }

  virtual void merged_semantics_changed () { }

private:
  bool m_merged_semantics;
  bool m_strict_handling;
  bool m_report_progress;
  std::string m_progress_desc;
  int m_base_verbosity;
};

}

#endif

