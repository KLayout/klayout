
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



#ifndef HDR_dbNetTracer
#define HDR_dbNetTracer

#include "dbPluginCommon.h"

#include "dbShapes.h"
#include "dbShape.h"
#include "dbEdgeProcessor.h"
#include "dbRegion.h"

#include "tlProgress.h"
#include "tlFixedVector.h"

#include <vector>
#include <map>
#include <list>

namespace db
{

class RecursiveShapeIterator;
class NetTracerLayerElement;
class NetTracerData;
class LayoutToNetlist;

/**
 *  @brief A shape heap where intermediate shapes can be placed into
 *
 *  This heap is intended to hold flat, top level shapes and uses a cache. First, the cache compresses 
 *  the data and second, this guarantees that the Shape references delivered point to the same object
 *  for identical shapes.
 */
class DB_PLUGIN_PUBLIC NetTracerShapeHeap
{
public:
  /**
   *  @brief Constructor
   */
  NetTracerShapeHeap ();

  /**
   *  @brief Get a shape object for a polygon
   */
  db::Shape insert (const db::Polygon &p);

  /**
   *  @brief Clear the heap
   */
  void clear ();

private:
  db::Shapes m_container;
  std::map<db::Polygon, db::Shape> m_cache;
};

/**
 *  @brief A shape abstraction for the net tracer, used for storing the net information
 *
 *  This class describes a shape in the hierarchy by the transformation into the top cell, the shape reference, the cell
 *  index and the layer the shape resides on.
 */
class DB_PLUGIN_PUBLIC NetTracerShape
{
public:
  /**
   *  @brief Creates an invalid shape
   */
  NetTracerShape ()
    : m_trans (), m_shape (), m_pseudo (true), m_layer (0), m_cell_index (0)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Create a shape from the given transformation, shape, layer, cell index and pseudo flag
   */
  NetTracerShape (const db::ICplxTrans &t, const db::Shape &s, unsigned int l, db::cell_index_type c, bool pseudo = false)
    : m_trans (t), m_shape (s), m_pseudo (pseudo), m_layer (l), m_cell_index (c)
  {
    m_bbox = m_trans * m_shape.bbox ();
  }

  /**
   *  @brief Returns true if the shape is valid 
   */
  bool is_valid () const
  {
    return !m_shape.is_null ();
  }

  /**
   *  @brief Gets the bounding box of the cell in the top cell
   */
  const db::Box &bbox () const
  {
    return m_bbox;
  }

  /**
   *  @brief Gets the transformation
   *
   *  This is the transformation of the shape relative to the top cell
   */
  const db::ICplxTrans &trans () const
  {
    return m_trans;
  }

  /**
   *  @brief Gets the shape
   */
  const db::Shape &shape () const
  {
    return m_shape;
  }

  /**
   *  @brief Sets the shape
   */
  void shape (const db::Shape &s)
  {
    m_shape = s;
    m_bbox = m_trans * m_shape.bbox ();
  }

  /** 
   *  @brief Gets the pseudo flag
   *
   *  The pseudo flag is set when the shape is not a part of the incremental net
   *  detection process but rather an endpoint. That way, start and stop shapes can
   *  be made part of the net with a special marking.
   */
  bool is_pseudo () const
  {
    return m_pseudo;
  }

  /** 
   *  @brief Sets the pseudo flag
   */
  void set_pseudo (bool p) 
  {
    m_pseudo = p;
  }

  /** 
   *  @brief Gets the layer where the shape is located
   */
  unsigned int layer () const
  {
    return m_layer;
  }

  /** 
   *  @brief Gets the cell index where the shape is located
   */
  db::cell_index_type cell_index () const
  {
    return m_cell_index;
  }

  /** 
   *  @brief equal operator
   */
  bool operator== (const NetTracerShape &other) const
  {
    //  Note: the pseudo flag is not part of the comparison. That way, the shape can be 
    //  marked "already found" by a pseudo-type shape.
    if (m_layer != other.m_layer) {
      return false;
    }
    if (m_bbox != other.m_bbox) {
      return false;
    }
    if (m_cell_index != other.m_cell_index) {
      return false;
    }
    if (m_shape != other.m_shape) {
      return false;
    }
    return m_trans.equal (other.m_trans);
  }

  /** 
   *  @brief less operator
   */
  bool operator< (const NetTracerShape &other) const
  {
    //  Note: the pseudo flag is not part of the comparison. That way, the shape can be 
    //  marked "already found" by a pseudo-type shape.
    if (m_layer != other.m_layer) {
      return m_layer < other.m_layer;
    }
    if (m_bbox != other.m_bbox) {
      return m_bbox < other.m_bbox;
    }
    if (m_cell_index != other.m_cell_index) {
      return m_cell_index < other.m_cell_index;
    }
    if (m_shape != other.m_shape) {
      return m_shape < other.m_shape;
    }
    return m_trans.less (other.m_trans);
  }

private:
  db::ICplxTrans m_trans;
  db::Shape m_shape;
  bool m_pseudo : 1;
  unsigned int m_layer : 31;
  db::cell_index_type m_cell_index;
  db::Box m_bbox;
};

/**
 *  @brief A box converter for the NetTracerShape which is used to build a quad tree for them
 */
struct HitTestDataBoxConverter
{
  db::Box operator() (const NetTracerShape *d) const
  {
    return d->bbox ();
  }

  typedef db::simple_bbox_tag complexity;
};

typedef db::box_tree<db::Box, const NetTracerShape *, HitTestDataBoxConverter, 1> HitTestDataBoxTree;

/**
 *  @brief Describes a boolean expression for computed layers
 */
class DB_PLUGIN_PUBLIC NetTracerLayerExpression
{
public:
  /**
   *  @brief The operator
   */
  enum Operator { OPNone, OPOr, OPNot, OPAnd, OPXor };

  /**
   *  @brief A helper class wrapping a Region with a tl::Object
   *  This way we can use tl::shared_ptr<RegionHolder> to manage the
   *  region's lifetime.
   */
  class RegionHolder
    : public tl::Object
  {
  public:
    RegionHolder ()
      : mp_region (0)
    { }

    RegionHolder (db::Region *region)
      : mp_region (region)
    { }

    ~RegionHolder ()
    {
      delete mp_region;
      mp_region = 0;
    }

    db::Region *get ()
    {
      return mp_region;
    }

    const db::Region *get () const
    {
      return mp_region;
    }

  private:
    db::Region *mp_region;
  };

  /**
   *  @brief Default Constructor
   */
  NetTracerLayerExpression ();

  /**
   *  @brief Default Constructor with the given layer as the "a" argument and OPNone
   *
   *  The argument must be an original layer.
   */
  NetTracerLayerExpression (int l);

  /**
   *  @brief Copy Constructor
   */
  NetTracerLayerExpression (const NetTracerLayerExpression &other);

  /**
   *  @brief Assignment
   */
  NetTracerLayerExpression &operator= (const NetTracerLayerExpression &other);

  /**
   *  @brief Destructor
   */
  ~NetTracerLayerExpression ();

  /**
   *  @brief Returns true, if the given original layer is a positive contribution to the formula
   */
  bool is_positive (unsigned int ol) const
  {
    if (m_op == OPOr || m_op == OPNone) {
      if (mp_a) {
        if (mp_a->is_positive (ol)) {
          return true;
        }
      } else if (int (ol) == m_a) {
        return true;
      }
    }

    if (m_op == OPOr) {
      if (mp_b) {
        if (mp_b->is_positive (ol)) {
          return true;
        }
      } else if (int (ol) == m_b) {
        return true;
      }
    }

    return false;
  }

  /**
   *  @brief Returns a representative (first) layer
   */
  int representative_layer () const
  {
    return mp_a ? mp_a->representative_layer () : m_a;
  }

  /**
   *  @brief Returns true, if the expression is an alias for the given layer a
   */
  bool is_alias_for (int a) const
  {
    return m_op == OPNone && mp_a == 0 && m_a == a;
  }

  /**
   *  @brief Returns true, if the expression is a simple alias
   */
  bool is_alias () const
  {
    return m_op == OPNone && mp_a == 0;
  }

  /**
   *  @brief Returns the layer, if the expression is an alias for it or -1 if the expression is not an alias at all
   */
  int alias_for () const
  {
    if (m_op == OPNone && mp_a == 0) {
      return m_a;
    } else {
      return -1;
    }
  }

  /**
   *  @brief Merge the given expression as the "b" argument with the given operator
   *
   *  This becomes owner of b.
   */
  void merge (Operator op, NetTracerLayerExpression *a);

  /**
   *  @brief Obtains the list of original layers involved in this operation
   */
  std::set<unsigned int> original_layers () const
  {
    std::set<unsigned int> l;
    collect_original_layers (l);
    return l;
  }

  /**
   *  @brief Compute the results
   *
   *  This function takes the shapes from the input set holding all related shapes. It will perform the booleans in this
   *  expression on the input shapes and verify them against the seeds. If a boolean result interacts with one of these
   *  new seeds, it is stored in the output set using the seed as the adjacent shape.
   *  "mask" is used as a mask for the output if not null.
   */
  void compute_results (unsigned int layer, db::cell_index_type cell_index, const std::vector<db::Polygon> *mask, const std::set <std::pair<NetTracerShape, const NetTracerShape *> > &input, const HitTestDataBoxTree *seeds, NetTracerShapeHeap &shape_heap, std::set <std::pair<NetTracerShape, const NetTracerShape *> > &output, const NetTracerData &data, db::EdgeProcessor &ep) const;

  /**
   *  @brief Dump for debugging purposes
   */
  std::string to_string () const;

  /**
   *  @brief Create a corresponding region inside a LayoutToNetlist object
   */
  tl::shared_ptr<RegionHolder> make_l2n_region (db::LayoutToNetlist &l2n, std::map<unsigned int, tl::shared_ptr<RegionHolder> > &region_cache, const std::string &name);

private:
  int m_a, m_b;
  NetTracerLayerExpression *mp_a, *mp_b;
  Operator m_op;

  void collect_original_layers (std::set<unsigned int> &l) const;
  tl::shared_ptr<NetTracerLayerExpression::RegionHolder> make_l2n_region_for_org (db::LayoutToNetlist &l2n, std::map <unsigned int, tl::shared_ptr<NetTracerLayerExpression::RegionHolder> > &region_cache, int org_index, const std::string &name);
};

/**
 *  @brief Describes one connection between two conductive layers
 *
 *  This class has three members: the index of the first conductive layer, the
 *  index of the via layer and the index of the second conductive layer.
 */
class DB_PLUGIN_PUBLIC NetTracerConnection
{
public:
  /**
   *  @brief Creates a connection between layer la and lb without an intermediate via layer
   *
   *  A connection described by this constructor defines a connection at all places where la overlaps lb.
   */
  NetTracerConnection (int la, int lb)
    : m_layer_a (la), m_via_layer (-1), m_has_via_layer (false), m_layer_b (lb)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Creates a connection between layer la and lb with a via layer
   *
   *  A connection described by this constructor defines a connection at all places where la overlaps the via 
   *  shape and the same via shape overlaps lb.
   */
  NetTracerConnection (int la, int via, int lb)
    : m_layer_a (la), m_via_layer (via), m_has_via_layer (true), m_layer_b (lb)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Gets the index of the first conductive layer
   *
   *  Returns a negative value if the via layer is not present (and should be considered
   *  empty).
   */
  int layer_a () const
  {
    return m_layer_a;
  }

  /**
   *  @brief Gets the index of the second conductive layer
   *
   *  Returns a negative value if the via layer is not present (and should be considered
   *  empty).
   */
  int layer_b () const
  {
    return m_layer_b;
  }

  /**
   *  @brief Returns true if this connection has a via layer
   */
  bool has_via_layer () const
  {
    return m_has_via_layer;
  }

  /**
   *  @brief Returns the index of the via layer. 
   *
   *  Returns a negative value if the via layer is not present (and should be considered
   *  empty).
   */
  int via_layer () const
  {
    return m_via_layer;
  }

private:
  int m_layer_a;
  int m_via_layer;
  bool m_has_via_layer;
  int m_layer_b;
};

/**
 *  @brief Wraps the data for a net tracing 
 */
class DB_PLUGIN_PUBLIC NetTracerData
{
public:
  /**
   *  @brief Default constructor
   */
  NetTracerData ();

  /**
   *  @brief Destructor
   */
  ~NetTracerData ();

  /**
   *  @brief Copy constructor
   */
  NetTracerData (const NetTracerData &other);

  /**
   *  @brief Assignment
   */
  NetTracerData &operator= (const NetTracerData &other);

  /**
   *  @brief Register a logical layer
   *
   *  @param expr The expression by which the new layer is computed (can be 0 to set an alias)
   *  @param symbol The symbol under which to register the expression or 0 for anonymous
   *  @return The ID of the logical layer
   *
   *  The NetTracerData object becomes the owner of the expr object.
   */
  unsigned int register_logical_layer (NetTracerLayerExpression *expr, const char *symbol);

  /**
   *  @brief Find the logical layer for a symbol
   */
  int find_symbol (const std::string &symbol) const;

  /**
   *  @brief Returns the expression for a given logical layer
   *
   *  If no explicit expression was registered, this method returns a alias expression.
   */
  const NetTracerLayerExpression &expression (unsigned int l) const;

  /**
   *  @brief Add a connection to the connection graph
   *
   *  The layers in the connect are logical layers. Hence, logical layer must be specified before 
   *  add_connection can be used.
   */
  void add_connection (const NetTracerConnection &connection);

  /**
   *  @brief Returns all connections starting or ending at the given layer.
   *
   *  'from_layer' is a logical layer while the returned values are all original layers
   *  involved in the composition of connected layers or the input layer.
   */
  const std::set<unsigned int> &connections (unsigned int from_layer) const;

  /**
   *  @brief Returns all logical layers connected to the given logical layer
   */
  const std::set<unsigned int> &log_connections (unsigned int from_layer) const;

  /**
   *  @brief Returns the connected original layers split into the ones requiring booleans and the ones which don't
   *  The result pair will contain the ones which do not require booleans in the first element, and the ones which
   *  do in the second.
   */
  const std::pair <std::set <unsigned int>, std::set <unsigned int> > &requires_booleans (unsigned int from_layer) const;

  /**
   *  @brief Find the logical layers that the given original layer participates in
   *
   *  Returns an empty set if the original layer does not participate in any connection
   */
  std::set<unsigned int> log_layers_for (unsigned int original_layer) const;

  /**
   *  @brief returns the symbol list
   */
  const std::map <std::string, unsigned int> &symbols () const
  {
    return m_symbols;
  }

  /**
   *  @brief Returns true, if no connection is defined.
   */
  bool is_empty () const
  {
    return m_connections.empty ();
  }

  /**
   *  @brief Prepares the connectivity for a LayoutToNetlist object
   *
   *  This method will provide the necessary regions for LayoutToNetlist,
   *  perform the boolean operations if required and set up the connectivity
   *  accordingly. The LayoutToNetlist object then is ready for netlist
   *  extraction.
   */
  void configure_l2n (db::LayoutToNetlist &l2n);

private:
  unsigned int m_next_log_layer;
  std::vector <NetTracerConnection> m_connections;
  std::map <unsigned int, std::set <unsigned int> > m_original_layers;
  std::map <unsigned int, std::set <unsigned int> > m_connection_graph;
  std::map <unsigned int, std::set <unsigned int> > m_log_connection_graph;
  mutable std::map <unsigned int, NetTracerLayerExpression *> m_log_layers;
  mutable std::map <unsigned int, std::pair <std::set <unsigned int>, std::set <unsigned int> > > m_requires_booleans;
  std::map <std::string, unsigned int> m_symbols;
  std::map <unsigned int, tl::shared_ptr<NetTracerLayerExpression::RegionHolder> > m_l2n_regions;

  void add_layer_pair (unsigned int a, unsigned int b);
  void add_layers (unsigned int a, unsigned int b);
  void clean_l2n_regions ();
};

/**
 *  @brief The net tracer
 *
 *  This object will provide a net tracing on a given cell view.
 *  Net tracing can be performed with a given seed point and given tracing data.
 *  The tracing is initiated with the "trace" method.
 */
class DB_PLUGIN_PUBLIC NetTracer
{
public:
  typedef std::set <NetTracerShape>::const_iterator iterator;

  /**
   *  @brief Construct a net tracer on the given cellview.
   */
  NetTracer ();

  /**
   *  @brief Trace the net starting from the given seed with the given data.
   */
  void trace (const db::Layout &layout, const db::Cell &cell, const NetTracerShape &start, const NetTracerData &data);

  /**
   *  @brief Trace the net starting from the given point/layer seed with the given data.
   */
  void trace (const db::Layout &layout, const db::Cell &cell, const db::Point &pt_start, unsigned int l_start, const NetTracerData &data);

  /**
   *  @brief Trace the path starting from the given seed and ending at the given shape with the given data.
   */
  void trace (const db::Layout &layout, const db::Cell &cell, const NetTracerShape &start, const NetTracerShape &stop, const NetTracerData &data);

  /**
   *  @brief Trace the net starting from the given point/layer seed with the given data.
   */
  void trace (const db::Layout &layout, const db::Cell &cell, const db::Point &pt_start, unsigned int l_start, const db::Point &pt_stop, unsigned int l_stop, const NetTracerData &data);

  /**
   *  @brief Begin operator for the shapes found
   */
  iterator begin () const
  {
    return m_shapes_found.begin ();
  }

  /**
   *  @brief End operator for the shapes found
   */
  iterator end () const
  {
    return m_shapes_found.end ();
  }

  /**
   *  @brief Sets the maximum number of shapes to trace
   *
   *  Setting the trace depth to 0 is equivalent to "unlimited".
   */
  void set_trace_depth (size_t n)
  {
    m_trace_depth = n;
  }

  /**
   *  @brief Gets the maximum number of shapes to trace
   */
  size_t trace_depth () const
  {
    return m_trace_depth;
  }

  /**
   *  @brief Returns the number of shapes found
   */
  size_t size () const
  {
    return m_shapes_found.size ();
  }

  /**
   *  @brief Returns true, if the net is incomplete
   *
   *  This flag is true if the extractor was aborted
   *  for example by the user or the trace depth was exhausted.
   *  The shapes do not fully cover the net.
   */
  bool incomplete () const
  {
    return m_incomplete;
  }

  /**
   *  @brief Clear the data found so far.
   */
  void clear ();

  /**
   *  @brief Get a name for the net
   *
   *  The name can be empty if not specific net name could be determined (i.e. from a label or property)
   */
  std::string name () const;

  /**
   *  @brief Set a name for the net
   */
  void set_name (const std::string &n);

  /**
   *  @brief Get the layout from which this net was taken
   */
  const db::Layout &layout () const
  {
    return *mp_layout;
  }

  /**
   *  @brief Get the cell from which this net was taken
   */
  const db::Cell &cell () const
  {
    return *mp_cell;
  }

private:
  const db::Layout *mp_layout;
  const db::Cell *mp_cell;
  std::set <NetTracerShape> m_shapes_found;
  NetTracerShapeHeap m_shape_heap;
  std::map <NetTracerShape, std::vector<const NetTracerShape *> > m_shapes_graph;
  tl::AbsoluteProgress *mp_progress;
  std::set <std::pair<NetTracerShape, const NetTracerShape *> > m_hit_test_queue;
  std::string m_name;
  int m_name_hier_depth;
  bool m_incomplete;
  size_t m_trace_depth;
  NetTracerShape m_stop_shape; 
  NetTracerShape m_start_shape;
  db::EdgeProcessor m_ep;

  void determine_interactions (const db::Box &seed, const NetTracerShape *shape, const std::set<unsigned int> &layers, std::set <std::pair<NetTracerShape, const NetTracerShape *> > &delivery);
  void determine_interactions (const db::Polygon &seed, const NetTracerShape *shape, const std::set<unsigned int> &layers, std::set <std::pair<NetTracerShape, const NetTracerShape *> > &delivery);
  void determine_interactions (const std::vector<const NetTracerShape *> &seeds, const db::Box &combined_box, const std::set<unsigned int> &layers, std::set <std::pair<NetTracerShape, const NetTracerShape *> > &delivery, bool do_seed_assignment = true);
  void evaluate_text (const db::RecursiveShapeIterator &iter);
  const NetTracerShape *deliver_shape (const NetTracerShape &shape, const NetTracerShape *adjacent);
  void compute_results_for_next_iteration (const std::vector <const NetTracerShape *> &new_seeds, unsigned int seed_layer, const std::set<unsigned int> &output_layers, std::set <std::pair<NetTracerShape, const NetTracerShape *> > &current, std::set <std::pair<NetTracerShape, const NetTracerShape *> > &output, const NetTracerData &data);
};

}

#endif

