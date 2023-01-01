
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



#ifndef HDR_dbEdgeProcessor
#define HDR_dbEdgeProcessor

#include "dbCommon.h"

#include "dbTypes.h"
#include "dbEdge.h"
#include "dbPolygon.h"

#include <vector>
#include <set>

namespace db
{

struct WorkEdge;
struct CutPoints;
class EdgeSink;

/**
 *  @brief A destination for a (sorted) set of edges
 *
 *  This receiver can be used as destination for the edge processor.
 *  It will receive edge events in the scanline order, this is bottom to 
 *  top and left to right. Edges will be non-intersecting.
 *
 *  This is the base class for such edge receivers.
 */
class DB_PUBLIC EdgeSink 
{
public:
  /** 
   *  @brief Constructor
   */
  EdgeSink () : m_can_stop (false) { }

  /** 
   *  @brief Destructor
   */
  virtual ~EdgeSink () { };

  /**
   *  @brief Start event
   *
   *  This method is called shortly before the first edge is delivered.
   *  Specifically, all edges are already cached before this method is called.
   *  Thus, inside an implementation of this method, the original source can be
   *  discarded.
   */
  virtual void start () { }

  /**
   *  @brief End event
   *
   *  This method is called after the last edge has been delivered.
   */
  virtual void flush () { }

  /**
   *  @brief Deliver an edge
   *
   *  This method delivers an edge that ends or starts at the current scanline.
   */
  virtual void put (const db::Edge &) { }

  /**
   *  @brief Deliver a tagged edge
   *
   *  This method delivers an edge that ends or starts at the current scanline.
   *  This version includes a tag which is generated when using "select_edge".
   *  A tag is a value > 0 returned by "select_edge".
   */
  virtual void put (const db::Edge &, int /*tag*/) { }

  /**
   *  @brief Deliver an edge that crosses the scanline
   *
   *  This method is called to deliver an edge that is not starting or ending at the
   *  current scanline.
   *  Another delivery of a set of crossing edges may happen through the skip_n 
   *  method which delivers a set of (unspecified) edges which are guaranteed to form
   *  a closed sequence, that is one which is starting and ending at a wrap count of 0.
   */
  virtual void crossing_edge (const db::Edge &) { }

  /**
   *  @brief Deliver an edge set forming a closed sequence
   *
   *  See description of "crossing_edge" for details.
   */
  virtual void skip_n (size_t /*n*/) { }

  /**
   *  @brief Signal the start of a scanline at the given y coordinate
   */
  virtual void begin_scanline (db::Coord /*y*/) { }

  /**
   *  @brief Signal the end of a scanline at the given y coordinate
   */
  virtual void end_scanline (db::Coord /*y*/) { }

  /**
   *  @brief Gets a value indicating that the generator wants to stop
   */
  bool can_stop () const
  {
    return m_can_stop;
  }

  /**
   *  @brief Resets the stop request
   */
  void reset_stop ()
  {
    m_can_stop = false;
  }

protected:
  /**
   *  @brief Sets the stop request
   *
   *  The scanner can choose to stop once the request is set.
   *  This is useful for implementing receivers that can stop once a
   *  specific condition is found.
   */
  void request_stop ()
  {
    m_can_stop = true;
  }

private:
  bool m_can_stop;
};

/**
 *  @brief A edge container that can be used as a receiver for edges
 *
 *  This class reimplements the EdgeSink interface.
 *  This receiver simply collects the edges in a container (a vector of edges)
 *  which is either kept internally or supplied from the outside.
 */
class DB_PUBLIC EdgeContainer 
  : public EdgeSink
{
public:
  /**
   *  @brief Constructor connecting this receiver to an external edge vector
   */
  EdgeContainer (std::vector<db::Edge> &edges, bool clear = false, int tag = 0, EdgeContainer *chained = 0)
    : EdgeSink (), mp_edges (&edges), m_clear (clear), m_tag (tag), mp_chained (chained)
  { }

  /**
   *  @brief Constructor using an internal edge vector
   */
  EdgeContainer (int tag = 0, EdgeContainer *chained = 0)
    : EdgeSink (), mp_edges (&m_edges), m_clear (false), m_tag (tag), mp_chained (chained)
  { }

  /**
   *  @brief Get the edges collected so far (const version)
   */
  const std::vector<db::Edge> &edges () const 
  {
    return *mp_edges;
  }

  /**
   *  @brief Get the edges collected so far (non-const version)
   */
  std::vector<db::Edge> &edges () 
  {
    return *mp_edges;
  }

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void start () 
  {
    if (m_clear) {
      mp_edges->clear ();
      //  The single-shot scheme is a easy way to overcome problems with multiple start/flush brackets (i.e. on size filter)
      m_clear = false;
    }
    if (mp_chained) {
      mp_chained->start ();
    }
  }

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void put (const db::Edge &e) 
  {
    mp_edges->push_back (e);
    if (mp_chained) {
      mp_chained->put (e);
    }
  }

  /**
   *  @brief Implementation of the EdgeSink interface
   */
  virtual void put (const db::Edge &e, int tag)
  {
    if (m_tag == 0 || tag == m_tag) {
      mp_edges->push_back (e);
    }
    if (mp_chained) {
      mp_chained->put (e, tag);
    }
  }

private:
  std::vector<db::Edge> m_edges;
  std::vector<db::Edge> *mp_edges;
  bool m_clear;
  int m_tag;
  EdgeContainer *mp_chained;
};

/**
 *  @brief The edge set operator base class
 *
 *  This class is an internal class that specifies how the output is formed from 
 *  a set of intersecting-free input edges. 
 *  Basically, this object receives events for the edges along the scan line.
 *  At the beginning of the scan line, the "reset" method is called to bring the
 *  evaluator into a defined state. Each edge has an integer property that can be
 *  used to distinguish edges from different polygons or layers.
 */
class DB_PUBLIC EdgeEvaluatorBase
{
public:
  typedef size_t property_type;

  EdgeEvaluatorBase () { }
  virtual ~EdgeEvaluatorBase () { }

  virtual void reset () { }
  virtual void reserve (size_t /*n*/) { }
  virtual int edge (bool /*north*/, bool /*enter*/, property_type /*p*/) { return 0; }
  virtual int select_edge (bool /*horizontal*/, property_type /*p*/) { return 0; }
  virtual int compare_ns () const { return 0; }
  virtual bool is_reset () const { return false; }
  virtual bool prefer_touch () const { return false; }
  virtual bool selects_edges () const { return false; }
};

/**
 *  @brief An intersection detector
 *
 *  This edge evaluator will not produce output edges but rather record the 
 *  property pairs of polygons intersecting or interacting in the specified
 *  way.
 *
 *  It will build a set of property pairs, where the lower property value
 *  is the first one of the pairs. 
 */
class DB_PUBLIC InteractionDetector
  : public EdgeEvaluatorBase
{
public:
  typedef std::set<std::pair<property_type, property_type> > interactions_type;
  typedef interactions_type::const_iterator iterator;

  /**
   *  @brief Constructor
   *
   *  The mode parameter selects the interaction check mode.
   *  0 is "overlapping" or "touching".
   *  -1 will select all secondary polygons inside polygons from the primary.
   *  -2 will select all primary polygons enclosing polygons from the secondary.
   *  +1 will select all secondary polygons outside polygons from the primary.
   *
   *  Use set_include_touching(f) to specify whether to include or not include the touching
   *  case as interacting for mode 0.
   *
   *  In modes -2, -1 and +1, finish () needs to be called before the interactions
   *  can be used.
   *
   *  All modes require property IDs to differentiate both inputs into primary and secondary.
   *  Property IDs from 0 to the given last primary ID value are considered to belong to
   *  the primary region. Property IDs above the last primary ID are considered to belong to
   *  the secondary region.
   *  This last property ID must be specified in the last_primary_id parameter.
   *  The reported interactions will be (primary_id,secondary_id) even for outside mode.
   *  For outside mode, the primary_id is always last_primary_id. In outside mode, the
   *  interactions are pseudo-interactions as by definition outside polygons don't interact.
   */
  InteractionDetector (int mode = 0, property_type last_primary_id = 0);

  /**
   *  @brief Sets the "touching" flag
   *  
   *  If this flag is set, the interaction will include "touching" interactions in mode 0, i.e.
   *  touching shapes will be counted as interacting.
   *  In the other modes, this flag should be true (the default).
   */
  void set_include_touching (bool f) 
  {
    m_include_touching = f;
  }

  /**
   *  @brief Gets the "touching" flag
   */
  bool include_touching () const
  {
    return m_include_touching;
  }

  /**
   *  @brief Finish the collection
   *
   *  This method must be called in mode -1 and +1 in order to finish the collection.
   */
  void finish ();

  /**
   *  @brief Iterator delivering the interactions (begin iterator)
   *
   *  The iterator delivers pairs of property values. The lower value will be the first one of the pair.
   */
  iterator begin () const
  {
    return m_interactions.begin ();
  }

  /**
   *  @brief Iterator delivering the interactions (end iterator)
   */
  iterator end () const
  {
    return m_interactions.end ();
  }

  virtual void reset ();
  virtual void reserve (size_t n);
  virtual int edge (bool north, bool enter, property_type p);
  virtual int compare_ns () const;
  virtual bool is_reset () const { return m_inside_s.empty () && m_inside_n.empty (); }
  virtual bool prefer_touch () const { return m_include_touching; }

private:
  int m_mode;
  bool m_include_touching;
  property_type m_last_primary_id;
  std::vector <int> m_wcv_n, m_wcv_s;
  std::set <property_type> m_inside_n, m_inside_s;
  std::set<std::pair<property_type, property_type> > m_interactions;
  std::set<property_type> m_non_interactions;
};

/**
 *  @brief A generic inside operator
 *
 *  This incarnation of the evaluator class implements an "inside" function
 *  based on a generic operator.
 */
template <class F>
class DB_PUBLIC_TEMPLATE GenericMerge
  : public EdgeEvaluatorBase
{
public:
  /**
   *  @brief Constructor
   */
  GenericMerge (const F &function) 
    : m_wc_n (0), m_wc_s (0), m_function (function)
  { }

  virtual void reset ()
  {
    m_wc_n = m_wc_s = 0;
  }

  virtual void reserve (size_t /*n*/)
  {
    // .. nothing yet ..
  }

  virtual int edge (bool north, bool enter, property_type /*p*/)
  {
    int *wc = north ? &m_wc_n : &m_wc_s;
    bool t0 = m_function (*wc);
    if (enter) {
      ++*wc;
    } else {
      --*wc;
    }
    bool t1 = m_function (*wc);
    if (t1 && ! t0) {
      return 1;
    } else if (! t1 && t0) {
      return -1;
    } else {
      return 0;
    }
  }

  virtual int compare_ns () const
  {
    if (m_function (m_wc_s) && ! m_function (m_wc_n)) {
      return -1;
    } else if (! m_function (m_wc_s) && m_function (m_wc_n)) {
      return 1;
    } else {
      return 0;
    }
  }

  virtual bool is_reset () const
  {
    return (m_wc_n == 0 && m_wc_s == 0);
  }

private:
  int m_wc_n, m_wc_s;
  F m_function;
};

/**
 *  @brief A helper class to implement the SimpleMerge operator
 */
struct ParametrizedInsideFunc 
{
  ParametrizedInsideFunc (int mode)
    : m_mode (mode)
  {
    //  .. nothing yet ..
  }

  inline bool operator() (int wc) const
  {
    if (m_mode > 0) {
      return wc >= m_mode;
    } else if (m_mode < 0) {
      return wc <= m_mode || -wc <= m_mode;
    } else {
      return (wc < 0 ? ((-wc) % 2) : (wc % 2)) != 0;
    }
  }

public:
  int m_mode;
};

/**
 *  @brief Simple merge operator
 *
 *  This incarnation of the evaluator class implements a simple 
 *  merge criterion for a set of edges. A result is generated if the wrap count (wc)
 *  of the edge set satisfies a condition given by "mode". Specifically:
 *    mode == 0: even-odd rule (wc = ... -3, -1, 1, 3, ...)
 *    mode == 1: wc >= 1
 *    mode == -1: wc >= 1 || wc <= -1
 *    mode == n: wc >= n
 *    mode == -n: wc >= n || wc <= -n
 */
class DB_PUBLIC SimpleMerge
  : public GenericMerge<ParametrizedInsideFunc>
{
public:
  /**
   *  @brief Constructor
   */
  SimpleMerge (int mode = -1)
    : GenericMerge<ParametrizedInsideFunc> (ParametrizedInsideFunc (mode))
  { }
};

/**
 *  @brief Boolean operations
 *
 *  This incarnation of the evaluator class implements a boolean operation
 *  (AND, A NOT B, B NOT A, XOR, OR). The mode can be specified in the constructor.
 *  It relies on the properties being set in a certain way: bit 0 codes the layer (0 for A, 1 for B)
 *  while the other bits are used to distinguish the polygons. For each polygon, a non-zero
 *  wrap count rule is applied before the boolean operation's output is formed.
 */
class DB_PUBLIC BooleanOp 
  : public EdgeEvaluatorBase 
{
public:
  enum BoolOp {
    And = 1, ANotB = 2, BNotA = 3, Xor = 4, Or = 5
  };
 
  /**
   *  @brief Constructor
   *
   *  @param mode The boolean operation that this object represents
   */
  BooleanOp (BoolOp mode);

  virtual void reset ();
  virtual void reserve (size_t n);
  virtual int edge (bool north, bool enter, property_type p);
  virtual int compare_ns () const;
  virtual bool is_reset () const { return m_zeroes == m_wcv_n.size () + m_wcv_s.size (); }

protected:
  template <class InsideFunc> bool result (int wca, int wcb, const InsideFunc &inside_a, const InsideFunc &inside_b) const;
  template <class InsideFunc> int edge_impl (bool north, bool enter, property_type p, const InsideFunc &inside_a, const InsideFunc &inside_b);
  template <class InsideFunc> int compare_ns_impl (const InsideFunc &inside_a, const InsideFunc &inside_b) const;

private:
  int m_wc_na, m_wc_nb, m_wc_sa, m_wc_sb;
  std::vector <int> m_wcv_n, m_wcv_s;
  BoolOp m_mode;
  size_t m_zeroes;
};

/**
 *  @brief Edge vs. Polygon intersection
 *
 *  This operator detects edges inside or outside polygons.
 *  The polygon edges must be given with property 0, the other edges
 *  with properties 1 and higher.
 *
 *  The operator will deliver edges inside polygons or outside polygons.
 *  It can be configured to include edges on the border of the polygons
 *  as being considered "inside the polygon".
 */
class DB_PUBLIC EdgePolygonOp 
  : public db::EdgeEvaluatorBase
{
public:
  /**
   * @brief The operation mode
   */
  enum mode_t {
    Inside = 0,    //  Selects inside edges
    Outside = 1,   //  Selects outside edges
    Both = 2       //  Selects both (inside -> tag #1, outside -> tag #2)
  };

  /**
   *  @brief Constructor
   *
   *  @param outside If true, the operator will deliver edges outside the polygon
   *  @param include_touching If true, edges on the polygon's border will be considered "inside" of polygons
   *  @param polygon_mode Determines how the polygon edges on property 0 are interpreted (see merge operators)
   */
  EdgePolygonOp (mode_t mode = Inside, bool include_touching = true, int polygon_mode = -1);

  virtual void reset ();
  virtual int select_edge (bool horizontal, property_type p);
  virtual int edge (bool north, bool enter, property_type p);
  virtual bool is_reset () const;
  virtual bool prefer_touch () const;
  virtual bool selects_edges () const;

private:
  mode_t m_mode;
  bool m_include_touching;
  db::ParametrizedInsideFunc m_function;
  int m_wcp_n, m_wcp_s;
};

/**
 *  @brief Boolean operations
 *
 *  This class implements a boolean operation similar to BooleanOp, but 
 *  in addition it allows one  to specify the merge mode for the two inputs.
 *  See "SimpleMergeOp" for the definition of the merge modes.
 *  This operator is especially useful to implement boolean operations
 *  with sized polygons which required a >0 interpretation.
 */
class DB_PUBLIC BooleanOp2
  : public BooleanOp
{
public:
  /**
   *  @brief Constructor
   *
   *  @param mode The boolean operation that this object represents
   */
  BooleanOp2 (BoolOp mode, int wc_mode_a, int wc_mode_b);

  virtual int edge (bool north, bool enter, property_type p);
  virtual int compare_ns () const;

private:
  int m_wc_mode_a, m_wc_mode_b;
};

/**
 *  @brief Merge operation
 *
 *  This incarnation of the evaluator class implements a merge operation
 *  which allows one  to distinguish polygons (through edge properties) and
 *  allows one to specify a overlap value. Default is 0 which means that the
 *  merge is equivalent to producing all polygins. A overlap value of 1 means
 *  that at least two polygons must overlap to produce a result.
 */
class DB_PUBLIC MergeOp 
  : public EdgeEvaluatorBase 
{
public:
  /**
   *  @brief Constructor
   *
   *  @param min_overlap See class description
   */
  MergeOp (unsigned int min_overlap = 0);

  virtual void reset ();
  virtual void reserve (size_t n);
  virtual int edge (bool north, bool enter, property_type p);
  virtual int compare_ns () const;
  virtual bool is_reset () const { return m_zeroes == m_wcv_n.size () + m_wcv_s.size (); }

private:
  int m_wc_n, m_wc_s;
  std::vector <int> m_wcv_n, m_wcv_s;
  unsigned int m_min_wc;
  size_t m_zeroes;
};

/**
 *  @brief The basic edge processor
 *
 *  An edge processor takes a set of edges, processes them by removing intersections and 
 *  applying a custom operator for computing the output edge sets which then are delivered
 *  to an EdgeSink receiver object.
 */
class DB_PUBLIC EdgeProcessor
{
public:
  typedef size_t property_type;

  /**
   *  @brief Default constructor
   *
   *  @param report_progress If true, a tl::Progress object will be created to report any progress (warning: this will impose a performance penalty)
   *  @param progress_text The description text of the progress object
   */
  EdgeProcessor (bool report_progress = false, const std::string &progress_desc = std::string ());

  /**
   *  @brief Destructor
   */
  ~EdgeProcessor ();

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

  /**
   *  @brief Base verbosity for timer reporting
   *
   *  The default value is 30. Basic timing will be reported for > base_verbosity, detailed timing
   *  for > base_verbosity + 10.
   */
  void set_base_verbosity (int bv);

  /**
   *  @brief Reserve space for at least n edges
   */
  void reserve (size_t n);

  /**
   *  @brief Insert an edge 
   */
  void insert (const db::Edge &e, property_type p = 0);

  /**
   *  @brief Insert a polygon
   */
  void insert (const db::Polygon &q, property_type p = 0);

  /**
   *  @brief Insert a simple polygon
   */
  void insert (const db::SimplePolygon &q, property_type p = 0);

  /**
   *  @brief Insert a polygon reference
   */
  void insert (const db::PolygonRef &q, property_type p = 0);

  /**
   *  @brief Insert a sequence of edges
   *
   *  This method does not reserve for the number of elements required. This must
   *  be done explicitly for performance benefits.
   */
  template <class Iter>
  void insert_sequence (Iter from, Iter to, property_type p = 0)
  {
    for (Iter i = from; i != to; ++i) {
      insert (*i, p);
    }
  }

  /**
   *  @brief Insert a sequence of edges (iterator with at_end semantics)
   *
   *  This method does not reserve for the number of elements required. This must
   *  be done explicitly for performance benefits.
   */
  template <class Iter>
  void insert_sequence (Iter i, property_type p = 0)
  {
    for ( ; !i.at_end (); ++i) {
      insert (*i, p);
    }
  }

  /**
   *  @brief Clears all edges stored currently in this processor
   */
  void clear ();

  /**
   *  @brief Performs the actual processing
   *
   *  This method will use the edges stored so far and runs it through the
   *  scanline algorithm.
   */
  void process (db::EdgeSink &es, EdgeEvaluatorBase &op);

  /**
   *  @brief Performs the actual processing
   *
   *  This version allows giving multiple edge sinks and evaluators.
   *  Each evaluator is worked on separately and feeds the corresponding
   *  edge sink.
   */
  void process (const std::vector<std::pair<db::EdgeSink *, db::EdgeEvaluatorBase *> > &gen);

  /**
   *  @brief Performs the actual processing again
   *
   *  This method can be called after "process" was used and will re-run the
   *  scanline algorithm. This is somewhat more efficient as the initial
   *  sorting and edge clipping can be skipped.
   */
  void redo (db::EdgeSink &es, EdgeEvaluatorBase &op);

  /**
   *  @brief Performs the actual processing again
   *
   *  This method can be called after "process" was used and will re-run the
   *  scanline algorithm. This is somewhat more efficient as the initial
   *  sorting and edge clipping can be skipped.
   */
  void redo (const std::vector<std::pair<db::EdgeSink *, db::EdgeEvaluatorBase *> > &gen);

  /**
   *  @brief Merge the given polygons in a simple "non-zero wrapcount" fashion
   *
   *  The wrapcount is computed over all polygons, i.e. overlapping polygons may "cancel" if they
   *  have different orientation (since a polygon is oriented by construction that is not easy to achieve).
   *  The other merge operation provided for this purpose is "merge" which normalizes each polygon individually before
   *  merging them. "simple_merge" is somewhat faster and consumes less memory.
   *
   *  The result is presented as a set of edges forming closed contours. Hulls are oriented clockwise while
   *  holes are oriented counter-clockwise.
   *
   *  This is a convenience method that bundles filling of the edges, processing with
   *  a SimpleMerge operator and puts the result into an output vector.
   *
   *  @param in The input polygons
   *  @param out The output edges
   *  @param mode The merge mode (see SimpleMerge constructor)
   */
  void simple_merge (const std::vector<db::Polygon> &in, std::vector <db::Edge> &out, int mode = -1);

  /**
   *  @brief Merge the given polygons in a simple "non-zero wrapcount" fashion into polygons
   *
   *  The wrapcount is computed over all polygons, i.e. overlapping polygons may "cancel" if they
   *  have different orientation (since a polygon is oriented by construction that is not easy to achieve).
   *  The other merge operation provided for this purpose is "merge" which normalizes each polygon individually before
   *  merging them. "simple_merge" is somewhat faster and consumes less memory.
   *
   *  This method produces polygons and allows fine-tuning the parameters for that purpose.
   *
   *  This is a convenience method that bundles filling of the edges, processing with
   *  a SimpleMerge operator and puts the result into an output vector.
   *
   *  @param in The input polygons
   *  @param out The output polygons
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if touching corners should be resolved into less connected contours
   *  @param mode The merge mode (see SimpleMerge constructor)
   */
  void simple_merge (const std::vector<db::Polygon> &in, std::vector <db::Polygon> &out, bool resolve_holes = true, bool min_coherence = true, int mode = -1);

  /**
   *  @brief Merge the given edges in a simple "non-zero wrapcount" fashion
   *
   *  The edges provided must form valid closed contours. Contours oriented differently "cancel" each other. 
   *  Overlapping contours are merged when the orientation is the same.
   *
   *  The result is presented as a set of edges forming closed contours. Hulls are oriented clockwise while
   *  holes are oriented counter-clockwise.
   *
   *  This is a convenience method that bundles filling of the edges, processing with
   *  a SimpleMerge operator and puts the result into an output vector.
   *
   *  @param in The input edges
   *  @param out The output edges
   *  @param mode The merge mode (see SimpleMerge constructor)
   */
  void simple_merge (const std::vector<db::Edge> &in, std::vector <db::Edge> &out, int mode = -1);

  /**
   *  @brief Merge the given edges in a simple "non-zero wrapcount" fashion into polygons
   *
   *  The edges provided must form valid closed contours. Contours oriented differently "cancel" each other. 
   *  Overlapping contours are merged when the orientation is the same.
   *
   *  This method produces polygons and allows fine-tuning the parameters for that purpose.
   *
   *  This is a convenience method that bundles filling of the edges, processing with
   *  a SimpleMerge operator and puts the result into an output vector.
   *
   *  @param in The input edges
   *  @param out The output polygons
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if touching corners should be resolved into less connected contours
   *  @param mode The merge mode (see SimpleMerge constructor)
   */
  void simple_merge (const std::vector<db::Edge> &in, std::vector <db::Polygon> &out, bool resolve_holes = true, bool min_coherence = true, int mode = -1);

  /**
   *  @brief Merge the given polygons 
   *
   *  In contrast to "simple_merge", this merge implementation considers each polygon individually before merging them.
   *  Thus self-overlaps are effectively removed before the output is computed and holes are correctly merged with the
   *  hull. In addition, this method allows one to select areas with a higher wrap count which allows one to compute overlaps
   *  of polygons on the same layer. Because this method merges the polygons before the overlap is computed, self-overlapping
   *  polygons do not contribute to higher wrap count areas.
   *
   *  The result is presented as a set of edges forming closed contours. Hulls are oriented clockwise while
   *  holes are oriented counter-clockwise.
   *
   *  This is a convenience method that bundles filling of the edges, processing with
   *  a Merge operator and puts the result into an output vector.
   *
   *  @param in The input polygons
   *  @param out The output edges
   *  @param min_wc The minimum wrap count for output (0: all polygons, 1: at least two overlapping)
   */
  void merge (const std::vector<db::Polygon> &in, std::vector <db::Edge> &out, unsigned int min_wc = 0);

  /**
   *  @brief Merge the given polygons 
   *
   *  In contrast to "simple_merge", this merge implementation considers each polygon individually before merging them.
   *  Thus self-overlaps are effectively removed before the output is computed and holes are correctly merged with the
   *  hull. In addition, this method allows one to select areas with a higher wrap count which allows one to compute overlaps
   *  of polygons on the same layer. Because this method merges the polygons before the overlap is computed, self-overlapping
   *  polygons do not contribute to higher wrap count areas.
   *
   *  This method produces polygons and allows one to fine-tune the parameters for that purpose.
   *
   *  This is a convenience method that bundles filling of the edges, processing with
   *  a Merge operator and puts the result into an output vector.
   *
   *  @param in The input polygons
   *  @param out The output polygons
   *  @param min_wc The minimum wrap count for output (0: all polygons, 1: at least two overlapping)
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if touching corners should be resolved into less connected contours
   */
  void merge (const std::vector<db::Polygon> &in, std::vector <db::Polygon> &out, unsigned int min_wc = 0, bool resolve_holes = true, bool min_coherence = true);

  /**
   *  @brief Size the given polygons 
   *
   *  This method sizes a set of polygons. Before the sizing is applied, the polygons are merged. After that, sizing is applied 
   *  on the individual result polygons of the merge step. The result may contain overlapping contours, but no self-overlaps. 
   *
   *  dx and dy describe the sizing. A positive value indicates oversize (outwards) while a negative one describes undersize (inwards).
   *  The sizing applied can be chosen differently in x and y direction. In this case, the sign must be identical for both
   *  dx and dy.
   *
   *  The result is presented as a set of edges forming closed contours. Hulls are oriented clockwise while
   *  holes are oriented counter-clockwise.
   *
   *  This is a convenience method that bundles filling of the edges and processing them 
   *  and which puts the result into an output vector.
   *
   *  @param in The input polygons
   *  @param dx The sizing value in x direction
   *  @param dy The sizing value in y direction
   *  @param out The output edges
   *  @param mode The sizing mode (see db::Polygon for a description)
   */
  void size (const std::vector<db::Polygon> &in, db::Coord dx, db::Coord dy, std::vector <db::Edge> &out, unsigned int mode = 2);

  /**
   *  @brief Size the given polygons into polygons
   *
   *  This method sizes a set of polygons. Before the sizing is applied, the polygons are merged. After that, sizing is applied 
   *  on the individual result polygons of the merge step. The result may contain overlapping polygons, but no self-overlapping ones. 
   *  Polygon overlap occurs if the polygons are close enough, so a positive sizing makes polygons overlap.
   *  
   *  dx and dy describe the sizing. A positive value indicates oversize (outwards) while a negative one describes undersize (inwards).
   *  The sizing applied can be chosen differently in x and y direction. In this case, the sign must be identical for both
   *  dx and dy.
   *
   *  This method produces polygons and allows one to fine-tune the parameters for that purpose.
   *
   *  This is a convenience method that bundles filling of the edges, processing with
   *  a SimpleMerge operator and puts the result into an output vector.
   *
   *  @param in The input polygons
   *  @param dx The sizing value in x direction
   *  @param dy The sizing value in y direction
   *  @param out The output polygons
   *  @param mode The sizing mode (see db::Polygon for a description)
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if touching corners should be resolved into less connected contours
   */
  void size (const std::vector<db::Polygon> &in, db::Coord dx, db::Coord dy, std::vector <db::Polygon> &out, unsigned int mode = 2, bool resolve_holes = true, bool min_coherence = true);

  /**
   *  @brief Size the given polygons (isotropic)
   *
   *  This method is equivalent to calling the anisotropic version with identical dx and dy.
   *
   *  @param in The input polygons
   *  @param d The sizing value in x direction
   *  @param out The output edges
   *  @param mode The sizing mode (see db::Polygon for a description)
   */
  void size (const std::vector<db::Polygon> &in, db::Coord d, std::vector <db::Edge> &out, unsigned int mode = 2)
  {
    size (in, d, d, out, mode);
  }

  /**
   *  @brief Size the given polygons into polygons (isotropic)
   *
   *  This method is equivalent to calling the anisotropic version with identical dx and dy.
   *
   *  @param in The input polygons
   *  @param d The sizing value in x direction
   *  @param out The output polygons
   *  @param mode The sizing mode (see db::Polygon for a description)
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if touching corners should be resolved into less connected contours
   */
  void size (const std::vector<db::Polygon> &in, db::Coord d, std::vector <db::Polygon> &out, unsigned int mode = 2, bool resolve_holes = true, bool min_coherence = true)
  {
    size (in, d, d, out, mode, resolve_holes, min_coherence);
  }

  /**
   *  @brief Boolean operation for a set of given polygons, creating edges
   *
   *  This method computes the result for the given boolean operation on two sets of polygons.
   *  The result is presented as a set of edges forming closed contours. Hulls are oriented clockwise while
   *  holes are oriented counter-clockwise.
   *
   *  This is a convenience method that bundles filling of the edges, processing with
   *  a Boolean operator and puts the result into an output vector.
   *
   *  @param a The input polygons (first operand)
   *  @param b The input polygons (second operand)
   *  @param out The output edges
   *  @param mode The boolean mode
   */
  void boolean (const std::vector<db::Polygon> &a, const std::vector<db::Polygon> &b, std::vector <db::Edge> &out, int mode);

  /**
   *  @brief Boolean operation for a set of given polygons, creating polygons
   *
   *  This method computes the result for the given boolean operation on two sets of polygons.
   *  This method produces polygons on output and allows one to fine-tune the parameters for that purpose.
   *
   *  This is a convenience method that bundles filling of the edges, processing with
   *  a Boolean operator and puts the result into an output vector.
   *
   *  @param a The input polygons (first operand)
   *  @param b The input polygons (second operand)
   *  @param out The output polygons
   *  @param mode The boolean mode
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if touching corners should be resolved into less connected contours
   */
  void boolean (const std::vector<db::Polygon> &a, const std::vector<db::Polygon> &b, std::vector <db::Polygon> &out, int mode, bool resolve_holes = true, bool min_coherence = true);

  /**
   *  @brief Boolean operation for a set of given edges, creating edges
   *
   *  This method computes the result for the given boolean operation on two sets of edges.
   *  The input edges must form closed contours where holes and hulls must be oriented differently. 
   *  The input edges are processed with a simple non-zero wrap count rule as a whole.
   *
   *  The result is presented as a set of edges forming closed contours. Hulls are oriented clockwise while
   *  holes are oriented counter-clockwise.
   *
   *  This is a convenience method that bundles filling of the edges, processing with
   *  a Boolean operator and puts the result into an output vector.
   *
   *  @param a The input edges (first operand)
   *  @param b The input edges (second operand)
   *  @param out The output edges
   *  @param mode The boolean mode
   */
  void boolean (const std::vector<db::Edge> &a, const std::vector<db::Edge> &b, std::vector <db::Edge> &out, int mode);

  /**
   *  @brief Boolean operation for a set of given edges, creating polygons
   *
   *  This method computes the result for the given boolean operation on two sets of edges.
   *  The input edges must form closed contours where holes and hulls must be oriented differently. 
   *  The input edges are processed with a simple non-zero wrap count rule as a whole.
   *
   *  This method produces polygons on output and allows one to fine-tune the parameters for that purpose.
   *
   *  This is a convenience method that bundles filling of the edges, processing with
   *  a Boolean operator and puts the result into an output vector.
   *
   *  @param a The input polygons (first operand)
   *  @param b The input polygons (second operand)
   *  @param out The output polygons
   *  @param mode The boolean mode
   *  @param resolve_holes true, if holes should be resolved into the hull
   *  @param min_coherence true, if touching corners should be resolved into less connected contours
   */
  void boolean (const std::vector<db::Edge> &a, const std::vector<db::Edge> &b, std::vector <db::Polygon> &out, int mode, bool resolve_holes = true, bool min_coherence = true);

private:
  std::vector <WorkEdge> *mp_work_edges;
  std::vector <CutPoints> *mp_cpvector;
  bool m_report_progress;
  std::string m_progress_desc;
  int m_base_verbosity;

  static size_t count_edges (const db::Polygon &q) 
  {
    size_t n = q.hull ().size ();
    for (unsigned int h = 0; h < q.holes (); ++h) {
      n += q.hole (h).size ();
    }
    return n;
  }

  static size_t count_edges (const std::vector<db::Polygon> &v) 
  {
    size_t n = 0;
    for (std::vector<db::Polygon>::const_iterator p = v.begin (); p != v.end (); ++p) {
      n += count_edges (*p);
    }
    return n;
  }

  void redo_or_process (const std::vector<std::pair<db::EdgeSink *, db::EdgeEvaluatorBase *> > &gen, bool redo);
};

}

#endif


