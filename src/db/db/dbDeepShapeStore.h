
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


#ifndef HDR_dbDeepShapeStore
#define HDR_dbDeepShapeStore

#include "dbCommon.h"

#include "tlObject.h"
#include "tlStableVector.h"
#include "tlThreads.h"
#include "dbLayout.h"
#include "dbRecursiveShapeIterator.h"
#include "dbHierarchyBuilder.h"
#include "dbCellMapping.h"
#include "gsiObject.h"

#include <set>
#include <map>

namespace db {

class DeepShapeStore;
class DeepShapeStoreState;
class Region;
class Edges;
class EdgePairs;
class Texts;
class ShapeCollection;
class NetBuilder;
class LayoutToNetlist;
class VariantsCollectorBase;

/**
 *  @brief Represents a shape collection from the deep shape store
 *
 *  This is a lightweight class pointing into the deep shape store.
 *  DeepLayer objects are issued by the DeepShapeStore class.
 */
class DB_PUBLIC DeepLayer
{
public:
  /**
   *  @brief Default constructor
   */
  DeepLayer ();

  /**
   *  @brief Destructor
   */
  ~DeepLayer ();

  /**
   *  @brief The constructor from the detailed information
   *  Use this constructor if you know what you're doing.
   */
  DeepLayer (DeepShapeStore *store, unsigned int layout, unsigned int layer);

  /**
   *  @brief Conversion operator from Region to DeepLayer
   *  This requires the Region to be a DeepRegion. Otherwise, this constructor will assert
   */
  DeepLayer (const Region &region);

  /**
   *  @brief Conversion operator from texts collection to DeepLayer
   *  This requires the texts to be a DeepTexts. Otherwise, this constructor will assert
   */
  DeepLayer (const Texts &region);

  /**
   *  @brief Conversion operator from edges collection to DeepLayer
   *  This requires the edges to be a DeepEdges. Otherwise, this constructor will assert
   */
  DeepLayer (const Edges &region);

  /**
   *  @brief Conversion operator from edge pairs collection to DeepLayer
   *  This requires the edge pairs to be a DeepEdgePairs. Otherwise, this constructor will assert
   */
  DeepLayer (const EdgePairs &region);

  /**
   *  @brief Copy constructor
   */
  DeepLayer (const DeepLayer &other);

  /**
   *  @brief Assignment
   */
  DeepLayer &operator= (const DeepLayer &other);

  /**
   *  @brief Less operator
   */
  bool operator< (const DeepLayer &other) const;

  /**
   *  @brief Equality operator
   */
  bool operator== (const DeepLayer &other) const;

  /**
   *  @brief Gets the layout object
   *  The return value is guaranteed to be non-null.
   */
  Layout &layout();

  /**
   *  @brief Gets the layout object (const version)
   */
  const db::Layout &layout () const;

  /**
   *  @brief Gets the layout object
   *  The return value is guaranteed to be non-null.
   */
  Cell &initial_cell();

  /**
   *  @brief Gets the initial cell object (const version)
   */
  const db::Cell &initial_cell () const;

  /**
   *  @brief Gets the layer
   */
  unsigned int layer () const
  {
    return m_layer;
  }

  /**
   *  @brief Gets the layout index
   */
  unsigned int layout_index () const
  {
    return m_layout;
  }

  /**
   *  @brief Gets the list of breakout cells if there are some
   *  "breakout cells" are cells which are not considered to participate in hierarchical operations,
   *  neither as sibling nor in parent-child relationships.
   */
  const std::set<db::cell_index_type> *breakout_cells () const;

  /**
   *  @brief Inserts the layer into the given layout, starting from the given cell and into the given layer
   */
  void insert_into (Layout *into_layout, db::cell_index_type into_cell, unsigned int into_layer) const;

  /**
   *  @brief Inserts the edge pairs layer into the given layout, starting from the given cell and into the given layer
   *  The edge pairs are converted to polygons with the given enlargement.
   */
  void insert_into_as_polygons (db::Layout *into_layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl) const;

  /**
   *  @brief Creates a derived new deep layer
   *  Derived layers use the same layout and context, but are initially
   *  empty layers for use as output layers on the same hierarchy.
   */
  DeepLayer derived () const;

  /**
   *  @brief Creates a copy of this layer
   */
  DeepLayer copy () const;

  /**
   *  @brief Adds shapes from another deep layer to this one
   */
  void add_from (const DeepLayer &dl);

  /**
   *  @brief Gets the shape store object
   *  This is a pure const version to prevent manipulation of the store.
   *  This method is intended to fetch configuration options from the store.
   */
  const DeepShapeStore *store () const
  {
    check_dss ();
    return mp_store.get ();
  }

  /**
   *  @brief Gets the non-const shape store object
   *  This feature is intended for internal purposes.
   */
  DeepShapeStore *store_non_const () const
  {
    check_dss ();
    return const_cast<DeepShapeStore *> (mp_store.get ());
  }

private:
  friend class DeepShapeStore;

  void check_dss () const;

  tl::weak_ptr<DeepShapeStore> mp_store;
  unsigned int m_layout;
  unsigned int m_layer;
};

/**
 *  @brief An object holding the state of a DeepShapeStore
 */
class DB_PUBLIC DeepShapeStoreState
{
public:
  DeepShapeStoreState ();

  void set_threads (int n);
  int threads () const;

  void set_max_vertex_count (size_t n);
  size_t max_vertex_count () const;

  void set_max_area_ratio (double ar);
  double max_area_ratio () const;

  void set_text_property_name (const tl::Variant &pn);
  const tl::Variant &text_property_name () const;

  void set_text_enlargement (int enl);
  int text_enlargement () const;

  void set_reject_odd_polygons (bool f);
  bool reject_odd_polygons () const;

  const std::set<db::cell_index_type> *breakout_cells (unsigned int layout_index) const;
  void clear_breakout_cells (unsigned int layout_index);
  void set_breakout_cells (unsigned int layout_index, const std::set<db::cell_index_type> &boc);
  void add_breakout_cell (unsigned int layout_index, db::cell_index_type ci);
  void add_breakout_cells (unsigned int layout_index, const std::set<db::cell_index_type> &cc);

  void set_subcircuit_hierarchy_for_nets (bool f);
  bool subcircuit_hierarchy_for_nets () const;

private:
  int m_threads;
  double m_max_area_ratio;
  size_t m_max_vertex_count;
  bool m_reject_odd_polygons;
  tl::Variant m_text_property_name;
  std::vector<std::set<db::cell_index_type> > m_breakout_cells;
  int m_text_enlargement;
  bool m_subcircuit_hierarchy_for_nets;

  std::set<db::cell_index_type> &ensure_breakout_cells (unsigned int layout_index)
  {
    if (m_breakout_cells.size () <= size_t (layout_index)) {
      m_breakout_cells.resize (layout_index + 1, std::set<db::cell_index_type> ());
    }
    return m_breakout_cells [layout_index];
  }
};

struct DB_PUBLIC RecursiveShapeIteratorCompareForTargetHierarchy
{
  bool operator () (const std::pair<db::RecursiveShapeIterator, std::pair<size_t, db::ICplxTrans> > &a, const std::pair<db::RecursiveShapeIterator, std::pair<size_t, db::ICplxTrans> > &b) const
  {
    int cmp_iter = db::compare_iterators_with_respect_to_target_hierarchy (a.first, b.first);
    if (cmp_iter != 0) {
      return cmp_iter < 0;
    }
    return a.second < b.second;
  }
};

/**
 *  @brief An object holding a cell mapping with the hierarchy generation Ids of the involved layouts
 */
class DB_PUBLIC CellMappingWithGenerationIds
  : public db::CellMapping
{
public:
  CellMappingWithGenerationIds ()
    : db::CellMapping (), m_into_generation_id (0), m_from_generation_id (0)
  {
    //  .. nothing yet ..
  }

  void swap (CellMappingWithGenerationIds &other)
  {
    db::CellMapping::swap (other);
    std::swap (m_into_generation_id, other.m_into_generation_id);
    std::swap (m_from_generation_id, other.m_from_generation_id);
  }

  bool is_valid (const db::Layout &into_layout, const db::Layout &from_layout) const
  {
    return into_layout.hier_generation_id () == m_into_generation_id && from_layout.hier_generation_id () == m_from_generation_id;
  }

  void set_generation_ids (const db::Layout &into_layout, const db::Layout &from_layout)
  {
    m_into_generation_id = into_layout.hier_generation_id ();
    m_from_generation_id = from_layout.hier_generation_id ();
  }

private:
  size_t m_into_generation_id, m_from_generation_id;
};

/**
 *  @brief The "deep shape store" is a working model for the hierarchical ("deep") processor
 *
 *  The deep shape store keeps temporary data for the deep shape processor.
 *  It mainly consists of layout objects holding the hierarchy trees and layers
 *  for the actual shapes.
 *
 *  The deep shape store provides the basis for working with deep regions. On preparation,
 *  shapes are copied into the deep shape store. After finishing, the shapes are copied
 *  back into the original layout. The deep shape store provides the methods and
 *  algorithms for doing the preparation and transfer.
 */
class DB_PUBLIC DeepShapeStore
  : public tl::Object, public gsi::ObjectBase
{
public:
  /**
   *  @brief The default constructor
   */
  DeepShapeStore ();

  /**
   *  @brief The constructor for a singular DSS
   *
   *  This DSS will be initialized with one layout and the given database unit
   *  and top level cell name.
   */
  DeepShapeStore (const std::string &topcell_name, double dbu);

  /**
   *  @brief The destructor
   */
  ~DeepShapeStore ();

  /**
   *  @brief Returns true, if the DeepShapeStore is singular
   *
   *  A "singular" shape store needs a single layout to keep the information.
   *  This is the case, if all Regions derived from it share the same origin
   *  and do not use clipping or region selection. Singular shape stores are
   *  required for netlist extraction for example.
   *
   *  For a singular shape store, "layout()" will return the layout
   *  object and "initial_cell()" will return the initial cell of the
   *  only layout.
   */
  bool is_singular () const;

  /**
   *  @brief Sets a value indicating whether to keep layouts
   *
   *  If this value is set to true, layouts are not released when their reference count
   *  goes down to zero.
   */
  void set_keep_layouts (bool f)
  {
    m_keep_layouts = f;
  }

  /**
   *  @brief Gets a value indicating whether to keep layouts
   */
  bool keep_layouts () const
  {
    return m_keep_layouts;
  }

  /**
   *  @brief Creates a new layer from a flat region (or the region is made flat)
   *
   *  This method is intended for use with singular-created DSS objects (see
   *  singular constructor).
   *
   *  After a flat layer has been created for a region, it can be retrieved
   *  from the region later with layer_for_flat (region).
   *
   *  If for_netlist is true, texts will be skipped except on top level. The
   *  reasoning is that texts below top level may create name clashes if they
   *  are used for net names.
   */
  DeepLayer create_from_flat (const db::Region &region, bool for_netlist, double max_area_ratio = 0.0, size_t max_vertex_count = 0, const db::ICplxTrans &trans = db::ICplxTrans ());

  /**
   *  @brief Creates a new layer from a flat edge collection (or the edge collection is made flat)
   *
   *  This method is intended for use with singular-created DSS objects (see
   *  singular constructor).
   *
   *  After a flat layer has been created for a region, it can be retrieved
   *  from the region later with layer_for_flat (region).
   */
  DeepLayer create_from_flat (const db::Edges &edges, const db::ICplxTrans &trans = db::ICplxTrans ());

  /**
   *  @brief Creates a new layer from a flat text collection (or the text collection is made flat)
   *
   *  This method is intended for use with singular-created DSS objects (see
   *  singular constructor).
   *
   *  After a flat layer has been created for a region, it can be retrieved
   *  from the region later with layer_for_flat (region).
   */
  DeepLayer create_from_flat (const db::Texts &texts, const db::ICplxTrans &trans = db::ICplxTrans ());

  /**
   *  @brief Gets the layer for a given flat collection (Region, Edges, Texts, EdgePairs)
   *
   *  If a layer has been created for a flat region with create_from_flat, it can be retrieved with this method.
   *  The first return value is true in this case.
   */
  std::pair<bool, DeepLayer> layer_for_flat (const ShapeCollection &coll) const;

  /**
   *  @brief Same as layer_for_flat, but takes a region Id
   */
  std::pair<bool, DeepLayer> layer_for_flat (size_t region_id) const;

  /**
   *  @brief Creates a layout with the given iterator and transformation for the given index
   *
   *  This method is intended for classes that need more control over the layouts per index
   *  (LayoutToNetlist).
   */
  void make_layout (unsigned int layout_index, const db::RecursiveShapeIterator &si, const db::ICplxTrans &trans = db::ICplxTrans ());

  /**
   *  @brief Inserts a polygon layer into the deep shape store
   *
   *  This method will create a new layer inside the deep shape store as a
   *  working copy of the original layer. Preparation involves re-shaping
   *  the polygons so their bounding box is a better approximation and the
   *  polygon complexity is reduced. For this, the polygons are split
   *  into parts satisfying the area ratio (bounding box vs. polygon area)
   *  and maximum vertex count constraints.
   */
  DeepLayer create_polygon_layer (const db::RecursiveShapeIterator &si, double max_area_ratio = 0.0, size_t max_vertex_count = 0, const ICplxTrans &trans = db::ICplxTrans ());

  /**
   *  @brief Inserts an edge layer into the deep shape store
   *
   *  This method will create a new layer inside the deep shape store as a
   *  working copy of the original layer. This method creates a layer
   *  for edges.
   *
   *  If "as_edges" is true, polygons and boxes will be converted into edges. Otherwise
   *  only edge objects are taken from the shape iterator. Note that the shape iterator
   *  must be configured to deliver all shape types if "as_edges" is true.
   */
  DeepLayer create_edge_layer (const db::RecursiveShapeIterator &si, bool as_edges, const ICplxTrans &trans = db::ICplxTrans ());

  /**
   *  @brief Inserts an edge pair layer into the deep shape store
   *
   *  This method will create a new layer inside the deep shape store as a
   *  working copy of the original layer. This method creates a layer
   *  for edge pairs.
   */
  DeepLayer create_edge_pair_layer (const db::RecursiveShapeIterator &si, const ICplxTrans &trans = db::ICplxTrans ());

  /**
   *  @brief Inserts an text layer into the deep shape store
   *
   *  This method will create a new layer inside the deep shape store as a
   *  working copy of the original layer. This method creates a layer
   *  for texts.
   */
  DeepLayer create_text_layer (const db::RecursiveShapeIterator &si, const ICplxTrans &trans = db::ICplxTrans ());

  /**
   *  @brief Inserts a polygon layer into the deep shape store using a custom preparation pipeline
   *
   *  This method will create a new layer inside the deep shapes store and
   *  feed it through the given pipeline. The pipeline may perform shape translations and
   *  finally will feed the target hierarchy.
   */
  DeepLayer create_custom_layer (const db::RecursiveShapeIterator &si, HierarchyBuilderShapeReceiver *pipe, const ICplxTrans &trans = db::ICplxTrans ());

  /**
   *  @brief Creates a deep layer as a copy from an existing one
   */
  DeepLayer create_copy (const DeepLayer &source, HierarchyBuilderShapeReceiver *pipe);

  /**
   *  @brief Inserts the deep layer's shapes into some target layout
   */
  void insert (const DeepLayer &layer, db::Layout *into_layout, db::cell_index_type into_cell, unsigned int into_layer);

  /**
   *  @brief Inserts the deep layer's edge pairs into some target layout
   *
   *  The edge pairs are converted to polygons with the given enlargement.
   */
  void insert_as_polygons (const DeepLayer &deep_layer, db::Layout *into_layout, db::cell_index_type into_cell, unsigned int into_layer, db::Coord enl);

  /**
   *  @brief Gets the cell mapping suitable to returning a layout from the deep shape store into the original layout hierarchy
   *
   *  If necessary, this method will modify the original layout and add new cells.
   *  "layout_index" is the layout to return to it's original. "into_layout" is the original layout, "into_cell"
   *  the original cell.
   *
   *  "excluded_cells" - if not 0 - will exclude the given cells
   *
   *  "included_cells" - if not 0 - will only include the given cells in the cell mapping
   */
  const db::CellMapping &cell_mapping_to_original (unsigned int layout_index, db::Layout *into_layout, db::cell_index_type into_cell, const std::set<db::cell_index_type> *excluded_cells = 0, const std::set<db::cell_index_type> *included_cells = 0);

  /**
   *  @brief Gets the cell mapping from one internal layout to another
   */
  const db::CellMapping &internal_cell_mapping (unsigned int from_layout_index, unsigned int into_layout_index);

  /**
   *  @brief For testing
   */
  static size_t instance_count ();

  /**
   *  @brief Gets the nth layout (const version)
   */
  const db::Layout &const_layout (unsigned int n) const;

  /**
   *  @brief Gets the nth layout (non-const version)
   *
   *  Don't try to mess too much with the layout object, you'll screw up the internals.
   */
  db::Layout &layout (unsigned int n);

  /**
   *  @brief Gets the initial cell of the nth layout (const version)
   */
  const db::Cell &const_initial_cell (unsigned int n) const;

  /**
   *  @brief Gets the initial cell of the nth layout (non-const version)
   *
   *  Don't try to mess too much with the cell object, you'll screw up the internals.
   */
  db::Cell &initial_cell (unsigned int n);

  /**
   *  @brief Gets the layout index for a given internal layout
   */
  unsigned int layout_index (const db::Layout *layout) const;

  /**
   *  @brief Gets the singular layout (const version)
   *
   *  This method will throw an exception if the deep shape store is not singular.
   */
  const db::Layout &const_layout () const
  {
    require_singular ();
    return const_layout (0);
  }

  /**
   *  @brief Gets the singular layout (non-const version)
   *
   *  This method will throw an exception if the deep shape store is not singular.
   *  Don't try to mess too much with the layout object, you'll screw up the internals.
   */
  db::Layout &layout ()
  {
    require_singular ();
    return layout (0);
  }

  /**
   *  @brief Gets the initial cell of the singular layout (const version)
   *
   *  This method will throw an exception if the deep shape store is not singular.
   */
  const db::Cell &const_initial_cell () const
  {
    require_singular ();
    return const_initial_cell (0);
  }

  /**
   *  @brief Gets the initial cell of the singular layout (non-const version)
   *
   *  This method will throw an exception if the deep shape store is not singular.
   *  Don't try to mess too much with the cell object, you'll screw up the internals.
   */
  db::Cell &initial_cell ()
  {
    require_singular ();
    return initial_cell (0);
  }

  /**
   *  @brief Gets the number of layouts
   */
  unsigned int layouts () const
  {
    return (unsigned int) m_layouts.size ();
  }

  /**
   *  @brief Gets a value indicating whether the given index is a valid layout index
   */
  bool is_valid_layout_index (unsigned int n) const;

  /**
   *  @brief Gets the net builder object for a given layout index and LayoutToNetlist database
   *
   *  If no net builder is available, one will be created. Use \\has_net_builder to check whether one is
   *  already created.
   */
  db::NetBuilder &net_builder_for (unsigned int layout_index, db::LayoutToNetlist *l2n);

  /**
   *  @brief Gets the net builder object for a given LayoutToNetlist database (requires the DSS to be singular)
   */
  db::NetBuilder &net_builder_for (db::LayoutToNetlist *l2n)
  {
    require_singular ();
    return net_builder_for (0, l2n);
  }

  /**
   *  @brief Gets a value indicating whether a net building is available
   */
  bool has_net_builder_for(unsigned int layout_index, db::LayoutToNetlist *l2n);

  /**
   *  @brief Gets the net builder object for a given LayoutToNetlist database (requires the DSS to be singular)
   */
  bool has_net_builder_for (db::LayoutToNetlist *l2n)
  {
    require_singular ();
    return has_net_builder_for (0, l2n);
  }

  /**
   *  @brief The deep shape store also keeps the number of threads to allocate for the hierarchical processor
   *
   *  This is a kind of hack, but it's convenient.
   */
  void set_threads (int n);

  /**
   *  @brief Gets the number of threads
   */
  int threads () const;

  /**
   *  @brief Sets a flag indicating whether the working layouts will store the whole original hierarchy
   *
   *  Setting this flag to true will make the deep shape store copy the
   *  hierarchy exactly from the origin layouts. This will take somewhat
   *  more memory but avoid future cell hierarchy mapping operations.
   *
   *  If set to false, only the needed parts of the hierarchy are copied.
   *  This part may need to grow when further operations are triggered.
   */
  void set_wants_all_cells (bool f);

  /**
   *  @brief Gets a flag indicating whether the working layouts will store the whole original hierarchy
   */
  bool wants_all_cells () const;

  /**
   *  @brief Sets a flag indicating whether to reject odd polygons
   *
   *  Some kind of "odd" (e.g. non-orientable) polygons may spoil the functionality
   *  because they cannot be handled properly. By using this flag, the shape store
   *  we reject these kind of polygons. The default is "accept" (without warning).
   */
  void set_reject_odd_polygons (bool f);

  /**
   *  @brief Gets a flag indicating whether to reject odd polygons
   */
  bool reject_odd_polygons () const;

  /**
   *  @brief Sets the maximum vertex count default value
   *
   *  This parameter is used to simplify complex polygons. It is used by
   *  create_polygon_layer with the default parameters. It's also used by
   *  boolean operations when they deliver their output.
   */
  void set_max_vertex_count (size_t n);

  /**
   *  @brief Gets the maximum vertex count
   */
  size_t max_vertex_count () const;

  /**
   *  @brief Sets the max. area ratio for bounding box vs. polygon area
   *
   *  This parameter is used to simplify complex polygons. It is used by
   *  create_polygon_layer with the default parameters. It's also used by
   *  boolean operations when they deliver their output.
   */
  void set_max_area_ratio (double ar);

  /**
   *  @brief Gets the max. area ratio
   */
  double max_area_ratio () const;

  /**
   *  @brief Sets the text property name
   *
   *  If set to a non-null variant, text strings are attached to the generated boxes
   *  as properties with this particular name. This option has an effect only if the
   *  text_enlargement property is not negative.
   *  By default, the name is empty.
   */
  void set_text_property_name (const tl::Variant &pn);

  /**
   *  @brief Gets the text property name
   */
  const tl::Variant &text_property_name () const;

  /**
   *  @brief Sets the text enlargement value
   *
   *  If set to a non-negative value, text objects are converted to boxes with the
   *  given enlargement (width = 2 * enlargement). The box centers are identical
   *  to the original location of the text.
   *  If this value is negative (the default), texts are ignored.
   */
  void set_text_enlargement (int enl);

  /**
   *  @brief Gets the text enlargement value
   */
  int text_enlargement () const;

  /**
   *  @brief Sets a value indicating whether to build a subcircuit hierarchy per net
   *
   *  This flag is used to determine the way, net subcircuit hierarchies are built:
   *  when true, subcells are created for subcircuits on a net. Otherwise the net
   *  shapes are produced flat inside the cell they appear on.
   */
  void set_subcircuit_hierarchy_for_nets (bool f);

  /**
   *  @brief Gets a value indicating whether to build a subcircuit hierarchy per net
   */
  bool subcircuit_hierarchy_for_nets () const;

  /**
   *  @brief Gets the breakout cells for a given layout
   *  Returns 0 if there are no breakout cells for this layout.
   */
  const std::set<db::cell_index_type> *breakout_cells (unsigned int layout_index) const;

  /**
   *  @brief Clears the breakout cell list for a given layout
   */
  void clear_breakout_cells (unsigned int layout_index);

  /**
   *  @brief Sets the breakout cell list for a given layout
   */
  void set_breakout_cells (unsigned int layout_index, const std::set<db::cell_index_type> &boc);

  /**
   *  @brief Adds a breakout cell for a given layout
   */
  void add_breakout_cell (unsigned int layout_index, db::cell_index_type ci);

  /**
   *  @brief Adds breakout cells for a given layout
   */
  void add_breakout_cells (unsigned int layout_index, const std::set<db::cell_index_type> &cc);

  /**
   *  @brief Pushes the state on the state stack
   *  The state involves threads, max_area_ratio, max_vertex_count, the breakout cells and
   *  the text representation properties (enlargement, property name).
   */
  void push_state ();

  /**
   *  @brief Pops the state (see @ref push_state)
   */
  void pop_state ();

private:
  friend class DeepLayer;

  struct LayoutHolder;

  void add_ref (unsigned int layout, unsigned int layer);
  void remove_ref (unsigned int layout, unsigned int layer);

  unsigned int layout_for_iter (const db::RecursiveShapeIterator &si, const db::ICplxTrans &trans);

  void require_singular () const;

  void issue_variants (unsigned int layout, const std::map<db::cell_index_type, std::map<db::ICplxTrans, db::cell_index_type> > &var_map);

  typedef std::map<std::pair<db::RecursiveShapeIterator, std::pair<size_t, db::ICplxTrans> >, unsigned int, RecursiveShapeIteratorCompareForTargetHierarchy> layout_map_type;

  //  no copying
  DeepShapeStore (const DeepShapeStore &);
  DeepShapeStore &operator= (const DeepShapeStore &);

  std::vector<LayoutHolder *> m_layouts;
  std::map<size_t, std::pair<unsigned int, unsigned int> > m_layers_for_flat;
  std::map<std::pair<unsigned int, unsigned int>, size_t> m_flat_region_id;
  layout_map_type m_layout_map;
  DeepShapeStoreState m_state;
  std::list<DeepShapeStoreState> m_state_stack;
  bool m_keep_layouts;
  bool m_wants_all_cells;
  tl::Mutex m_lock;

  struct DeliveryMappingCacheKey
  {
    //  NOTE: we shouldn't keep pointers here as the layouts may get deleted and recreated with the same address.
    //  But as we don't access these objects that's fairly safe.
    DeliveryMappingCacheKey (unsigned int _from_index, tl::id_type _into_layout, db::cell_index_type _into_cell)
      : from_index (_from_index), into_layout (_into_layout), into_cell (_into_cell)
    {
      //  .. nothing yet ..
    }

    bool operator< (const DeliveryMappingCacheKey &other) const
    {
      if (from_index != other.from_index) {
        return from_index < other.from_index;
      }
      if (into_layout != other.into_layout) {
        return into_layout < other.into_layout;
      }
      return into_cell <other.into_cell;
    }

    unsigned int from_index;
    tl::id_type into_layout;
    db::cell_index_type into_cell;
  };

  std::map<DeliveryMappingCacheKey, CellMappingWithGenerationIds> m_delivery_mapping_cache;
  std::map<std::pair<unsigned int, unsigned int>, CellMappingWithGenerationIds> m_internal_mapping_cache;
};

}

#endif

