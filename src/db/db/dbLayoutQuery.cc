
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


#include "dbLayoutQuery.h"
#include "dbCellGraphUtils.h"
#include "dbStreamLayers.h"
#include "tlAssert.h"
#include "tlString.h"
#include "tlGlobPattern.h"
#include "tlExpression.h"
#include "gsiExpression.h"
#include "gsiDecl.h"

#include <limits>
#include <memory>
#include <deque>
#include <iostream>

namespace db
{

// --------------------------------------------------------------------------------
//  Some utilities

static const char *s_select = "select";
static const char *s_delete = "delete";
static const char *s_or = "or";
static const char *s_of = "of";
static const char *s_on = "on";
static const char *s_do = "do";
static const char *s_from = "from";
static const char *s_layer = "layer";
static const char *s_layers = "layers";
static const char *s_cell = "cell";
static const char *s_cells = "cells";
static const char *s_where = "where";
static const char *s_shapes = "shapes";
static const char *s_polygons = "polygons";
static const char *s_boxes = "boxes";
static const char *s_edges = "edges";
static const char *s_paths = "paths";
static const char *s_texts = "texts";
static const char *s_instances = "instances";
static const char *s_arrays = "arrays";
static const char *s_sorted = "sorted";
static const char *s_unique = "unique";
static const char *s_by = "by";
static const char *s_with = "with";
static const char *s_pass = "pass";

const char *s_reserved_words[] = {
  s_select,
  s_delete,
  s_or,
  s_of,
  s_on,
  s_do,
  s_from,
  s_layer,
  s_layers,
  s_cell,
  s_cells,
  s_where,
  s_shapes,
  s_polygons,
  s_boxes,
  s_edges,
  s_paths,
  s_texts,
  s_instances,
  s_arrays,
  s_sorted,
  s_unique,
  s_by,
  s_with,
  s_pass
};

bool check_trailing_reserved_word (const tl::Extractor &ex0)
{
  tl::Extractor ex = ex0;
  for (size_t i = 0; i < sizeof (s_reserved_words) / sizeof (s_reserved_words[0]); ++i) {
    if (ex.test (s_reserved_words[i])) {
      return true;
    }
  }
  return false;
}

// --------------------------------------------------------------------------------
//  FilterSingleState definition and implementation

class DB_PUBLIC FilterSingleState 
  : public FilterStateBase
{
public:
  FilterSingleState (const FilterBase *filter, db::Layout *layout, tl::Eval &eval)
    : FilterStateBase (filter, layout, eval), m_done (false)
  {
    //  .. nothing yet ..
  }

  virtual void reset (FilterStateBase *previous) 
  {
    FilterStateBase::reset (previous);
    m_done = false;
  }

  virtual void next (bool) 
  {
    m_done = true;
  }

  virtual bool at_end () 
  {
    return m_done;
  }

private:
  bool m_done;
};

// --------------------------------------------------------------------------------
//  NameFilterArgument definition and implementation

/**
 *  @brief A class that provides a name filter argument
 */
class DB_PUBLIC NameFilterArgument
{
public:
  NameFilterArgument ()
    : m_needs_eval (false)
  { 
    //  .. nothing yet ..
  }

  NameFilterArgument (const std::string &pattern, bool needs_eval = false)
    : m_pattern (pattern), m_needs_eval (needs_eval)
  { 
    //  .. nothing yet ..
  }

  void parse (tl::Extractor &ex)
  {
    if (ex.test ("$")) {

      m_pattern = tl::Eval::parse_expr (ex, false);
      m_needs_eval = true;

    } else if (! ex.at_end () && ! check_trailing_reserved_word (ex)) {

      std::string name;
      ex.read_word_or_quoted (name, "_$*?");
      m_pattern = name;
      m_needs_eval = false;

    }
  }

  bool empty () const
  {
    return ! m_needs_eval && m_pattern.empty ();
  }

  std::string pattern () const
  {
    return m_pattern;
  }

private:
  friend class NameFilter;

  std::string m_pattern;
  bool m_needs_eval;
};

// --------------------------------------------------------------------------------
//  NameFilter definition and implementation

/**
 *  @brief A class that provides a name filter (with late evaluation)
 */
class DB_PUBLIC NameFilter
{
public:
  NameFilter (const NameFilterArgument &arg, tl::Eval &eval)
    : m_needs_eval (arg.m_needs_eval), mp_eval (&eval)
  { 
    if (m_needs_eval) {
      eval.parse (m_expression, arg.m_pattern, true);
    } else {
      m_pattern = arg.m_pattern;
    }
  }

  void reset ()
  {
    if (m_needs_eval) {
      m_pattern = m_expression.execute ().to_string ();
    }
  }

  bool match (const std::string &s)
  {
    return m_pattern.match (s, mp_eval->match_substrings ());
  }

  bool is_catchall () const
  {
    return ! m_needs_eval && m_pattern.is_catchall ();
  }

  bool is_const () const
  {
    return ! m_needs_eval && m_pattern.is_const ();
  }

  bool needs_eval () const
  {
    return m_needs_eval;
  }

  const std::string &pattern () const
  {
    return m_pattern.pattern ();
  }

private:
  tl::GlobPattern m_pattern;
  tl::Expression m_expression;
  bool m_needs_eval;
  tl::Eval *mp_eval;
};

// --------------------------------------------------------------------------------
//  ShapeFilter definition and implementation

struct ShapeFilterPropertyIDs
{
  ShapeFilterPropertyIDs (LayoutQuery *q)
  {
    bbox               = q->register_property ("bbox", LQ_box);
    dbbox              = q->register_property ("dbbox", LQ_dbox);
    shape_bbox         = q->register_property ("shape_bbox", LQ_box);
    shape_dbbox        = q->register_property ("shape_dbbox", LQ_dbox);
    shape              = q->register_property ("shape", LQ_shape);
    layer_info         = q->register_property ("layer_info", LQ_layer);
    layer_index        = q->register_property ("layer_index", LQ_variant);
    //  for accessing the parent's properties
    cell_index         = q->register_property ("cell_index", LQ_variant);
  }

  unsigned int bbox;                // bbox                 -> The shape's bounding box
  unsigned int dbbox;               // dbbox                -> The shape's bounding box in micrometer units
  unsigned int shape_bbox;          // shape_bbox           -> == box
  unsigned int shape_dbbox;         // shape_dbbox          -> == dbox
  unsigned int shape;               // shape                -> The shape object
  unsigned int layer_info;          // layer_info           -> The layer (a LayerInfo object)
  unsigned int layer_index;         // layer_index          -> The layer index
  unsigned int cell_index;          // cell_index           -> The cell index
};

class DB_PUBLIC ShapeFilterState
  : public FilterStateBase
{
public:
  ShapeFilterState (const FilterBase *filter, const db::LayerMap &layers, db::ShapeIterator::flags_type flags, tl::Eval &eval, db::Layout *layout, bool reading, const ShapeFilterPropertyIDs &pids)
    : FilterStateBase (filter, layout, eval),
      m_flags (flags), mp_parent (0), m_reading (reading), m_pids (pids), m_lindex (0)
  {
    //  get the layers which we have to look for
    for (db::Layout::layer_iterator l = layout->begin_layers (); l != layout->end_layers (); ++l) {
      if (layers.is_empty () || layers.is_mapped (*(*l).second)) {
        m_layers.push_back ((*l).first);
      }
    }
  }

  virtual void reset (FilterStateBase *previous) 
  {
    FilterStateBase::reset (previous);

    //  Get the parent cell by asking the previous states 
    mp_parent = 0;
    tl::Variant parent_id;
    if (FilterStateBase::get_property (m_pids.cell_index, parent_id)) {
      mp_parent = &layout ()->cell (db::cell_index_type (parent_id.to_ulong ()));
    }

    m_ignored.clear ();

    m_lindex = 0;
    if (mp_parent) {
      while (m_layers.size () > m_lindex) {
        m_shape = mp_parent->shapes (m_layers [m_lindex]).begin (m_flags);
        if (m_shape.at_end ()) {
          ++m_lindex;
        } else {
          if (! m_reading) {
            m_s = *m_shape;
          }
          break;
        }
      }
    }
  }

  virtual void next (bool) 
  {
    if (mp_parent) {

      //  If the shape has been modified insert it into the ignore list. That
      //  way it does not appear again.
      if (m_reading && m_s != *m_shape) {
        m_ignored.insert (m_s);
      }

      do {

        ++m_shape;
        while (m_shape.at_end ()) {
          ++m_lindex;
          if (m_layers.size () > m_lindex) {
            m_shape = mp_parent->shapes (m_layers [m_lindex]).begin (m_flags);
            m_ignored.clear ();
          } else {
            break;
          }
        }

        if (! m_reading && ! m_shape.at_end ()) {
          m_s = *m_shape;
        } else {
          break;
        }

      } while (m_ignored.find (m_s) != m_ignored.end ());

    }  
  }

  virtual bool at_end () 
  {
    return !mp_parent || m_lindex >= m_layers.size ();
  }

  bool get_property (unsigned int id, tl::Variant &v)
  {
    if (id == m_pids.bbox || id == m_pids.shape_bbox) {

      v = tl::Variant::make_variant (m_shape->bbox ());
      return true;

    } else if (id == m_pids.dbbox || id == m_pids.shape_dbbox) {

      tl_assert (mp_parent->layout ());
      v = tl::Variant::make_variant (db::CplxTrans (mp_parent->layout ()->dbu ()) * m_shape->bbox ());
      return true;

    } else if (id == m_pids.shape) {

      if (m_reading) {
        v = tl::Variant::make_variant (*m_shape, true /*=is_const*/);
      } else {
        v = tl::Variant::make_variant_ref (&m_s);
      }
      return true;

    } else if (id == m_pids.layer_index) {

      v = m_layers[m_lindex];
      return true;

    } else if (id == m_pids.layer_info) {

      v = tl::Variant::make_variant (layout ()->get_properties (m_layers[m_lindex]));
      return true;

    } else {
      return FilterStateBase::get_property (id, v);
    }
  }

  virtual void dump () const
  {
    std::cout << "ShapeFilterState";
    FilterStateBase::dump ();
  }

private:
  db::ShapeIterator::flags_type m_flags;
  const db::Cell *mp_parent;
  bool m_reading;
  ShapeFilterPropertyIDs m_pids;
  std::vector<unsigned int> m_layers;
  size_t m_lindex;
  db::ShapeIterator m_shape;
  db::Shape m_s;
  std::set<db::Shape> m_ignored;
};

class DB_PUBLIC ShapeFilter
  : public FilterBracket
{
public:
  ShapeFilter (LayoutQuery *q, const db::LayerMap &layers, db::ShapeIterator::flags_type flags, bool reading)
    : FilterBracket (q), 
      m_pids (q),
      m_layers (layers),
      m_flags (flags), 
      m_reading (reading)
  {
    // .. nothing yet ..
  }

  FilterStateBase *do_create_state (db::Layout *layout, tl::Eval &eval) const
  {
    return new ShapeFilterState (this, m_layers, m_flags, eval, layout, m_reading, m_pids);
  }

  FilterBase *clone (LayoutQuery *q) const
  {
    return new ShapeFilter (q, m_layers, m_flags, m_reading);
  }

  virtual void dump (unsigned int l) const
  {
    for (unsigned int i = 0; i < l; ++i) {
      std::cout << "  ";
    }
    std::cout << "ShapeFilter (" << m_layers.to_string () << ", " << (int)m_flags << ") :" << std::endl;
    FilterBracket::dump (l + 1);
  }

private:
  ShapeFilterPropertyIDs m_pids;
  db::LayerMap m_layers;
  db::ShapeIterator::flags_type m_flags;
  bool m_reading;
};

// --------------------------------------------------------------------------------
//  ChildCellFilter definition and implementation

enum ChildCellFilterInstanceMode
{
  NoInstances = 0,
  ExplodedInstances = 1,
  ArrayInstances = 2
};

struct ChildCellFilterPropertyIDs
{
  ChildCellFilterPropertyIDs (LayoutQuery *q, ChildCellFilterInstanceMode instance_mode)
  {
    path               = q->register_property ("path", LQ_variant);
    path_names         = q->register_property ("path_names", LQ_variant);
    initial_cell       = q->register_property ("initial_cell", LQ_cell);
    initial_cell_index = q->register_property ("initial_cell_index", LQ_variant);
    initial_cell_name  = q->register_property ("initial_cell_name", LQ_variant);
    cell               = q->register_property ("cell", LQ_cell);
    cell_index         = q->register_property ("cell_index", LQ_variant);
    cell_name          = q->register_property ("cell_name", LQ_variant);
    parent_cell        = q->register_property ("parent_cell", LQ_cell);
    parent_cell_index  = q->register_property ("parent_cell_index", LQ_variant);
    parent_cell_name   = q->register_property ("parent_cell_name", LQ_variant);
    hier_levels        = q->register_property ("hier_levels", LQ_variant);
    bbox               = q->register_property ("bbox", LQ_box);
    dbbox              = q->register_property ("dbbox", LQ_dbox);
    cell_bbox          = q->register_property ("cell_bbox", LQ_box);
    cell_dbbox         = q->register_property ("cell_dbbox", LQ_dbox);

    //  with instance_mode == NoInstances:
    if (instance_mode == NoInstances) {
      references       = q->register_property ("references", LQ_variant);
      weight           = q->register_property ("weight", LQ_variant);
      tot_weight       = q->register_property ("tot_weight", LQ_variant);
    } else {
      references       = std::numeric_limits<unsigned int>::max ();
      weight           = std::numeric_limits<unsigned int>::max ();
      tot_weight       = std::numeric_limits<unsigned int>::max ();
    }

    //  with instance_mode != NoInstances:
    if (instance_mode != NoInstances) {
      path_trans       = q->register_property ("path_trans", LQ_trans);
      path_dtrans      = q->register_property ("path_dtrans", LQ_dtrans);
      trans            = q->register_property ("trans", LQ_trans);
      dtrans           = q->register_property ("dtrans", LQ_dtrans);
      inst_bbox        = q->register_property ("inst_bbox", LQ_box);
      inst_dbbox       = q->register_property ("inst_dbbox", LQ_box);
      inst             = q->register_property ("inst", LQ_instance);
      array_a          = q->register_property ("array_a", LQ_point);
      array_da         = q->register_property ("array_da", LQ_dpoint);
      array_na         = q->register_property ("array_na", LQ_variant);
      array_b          = q->register_property ("array_b", LQ_point);
      array_db         = q->register_property ("array_db", LQ_dpoint);
      array_nb         = q->register_property ("array_nb", LQ_variant);
    } else {
      path_trans       = std::numeric_limits<unsigned int>::max ();
      path_dtrans      = std::numeric_limits<unsigned int>::max ();
      trans            = std::numeric_limits<unsigned int>::max ();
      dtrans           = std::numeric_limits<unsigned int>::max ();
      inst_bbox        = std::numeric_limits<unsigned int>::max ();
      inst_dbbox       = std::numeric_limits<unsigned int>::max ();
      inst             = std::numeric_limits<unsigned int>::max ();
      array_a          = std::numeric_limits<unsigned int>::max ();
      array_da         = std::numeric_limits<unsigned int>::max ();
      array_na         = std::numeric_limits<unsigned int>::max ();
      array_b          = std::numeric_limits<unsigned int>::max ();
      array_db         = std::numeric_limits<unsigned int>::max ();
      array_nb         = std::numeric_limits<unsigned int>::max ();
    }

    //  with instance_mode == ExplodedInstances:
    if (instance_mode == ExplodedInstances) {
      array_ia         = q->register_property ("array_ia", LQ_variant);
      array_ib         = q->register_property ("array_ib", LQ_variant);
    } else {
      array_ia         = std::numeric_limits<unsigned int>::max ();
      array_ib         = std::numeric_limits<unsigned int>::max ();
    }
  }

  unsigned int path;                // path                 -> Variant array with the indexes of the cells in that path
  unsigned int path_names;          // path                 -> Variant array with the names of the cells in that path
  unsigned int initial_cell;        // initial_cell         -> Initial cell object (first of path)
  unsigned int initial_cell_index;  // initial_cell_index   -> Index of initial cell (first of path)
  unsigned int initial_cell_name;   // initial_cell_name    -> Name of initial cell (first of path)
  unsigned int cell;                // cell                 -> Current cell (last of path)
  unsigned int cell_index;          // cell_index           -> Index of current cell (last of path)
  unsigned int cell_name;           // cell_name            -> Name of current cell (last of path)
  unsigned int parent_cell;         // parent_cell          -> Parent cell (next in path) or nil
  unsigned int parent_cell_index;   // parent_cell_index    -> Index of parent cell (next in path) or nil
  unsigned int parent_cell_name;    // parent_cell_name     -> Name of parent cell (next in path) or nil
  unsigned int hier_levels;         // hier_levels          -> Number of hierarchy levels in path (length of path - 1)
  unsigned int bbox;                // bbox                 -> Cell bounding box
  unsigned int dbbox;               // dbbox                -> Cell bounding box in micrometer units
  unsigned int cell_bbox;           // cell_bbox            -> == bbox
  unsigned int cell_dbbox;          // cell_dbbox           -> == dbbox

  //  with instance_mode == NoInstances:
  unsigned int references;          // references           -> The number of instances (arefs count as 1) of this cell in the parent cell
  unsigned int weight;              // weight               -> The number of instances (arefs are flattened) of this cell in the parent cell
  unsigned int tot_weight;          // tot_weight           -> The number of instances of this cell in the initial cell along the given path

  //  with instance_mode != NoInstances:
  unsigned int path_trans;          // path_trans           -> The transformation of that instance into the top cell
  unsigned int path_dtrans;         // path_dtrans          -> The transformation of that instance into the top cell in micrometer units
  unsigned int trans;               // trans                -> The transformation of that instance (first instance if an array)
  unsigned int dtrans;              // dtrans               -> The transformation of that instance (first instance if an array) in micrometer units
  unsigned int inst_bbox;           // inst_bbox            -> The instance bounding box in the top cell
  unsigned int inst_dbbox;          // inst_dbbox           -> The instance bounding box in the top cell in micrometer units
  unsigned int inst;                // inst                 -> The instance object
  unsigned int array_a;             // array_a              -> The a vector for an array instance
  unsigned int array_da;            // array_da             -> The a vector for an array instance in micrometer units
  unsigned int array_na;            // array_na             -> The a axis array dimension
  unsigned int array_b;             // array_b              -> The b vector for an array instance
  unsigned int array_db;            // array_db             -> The b vector for an array instance in micrometer units
  unsigned int array_nb;            // array_nb             -> The b axis array dimension

  //  with instance_mode == ExplodedInstances:
  unsigned int array_ia;            // array_ia             -> The a index when an array is iterated
  unsigned int array_ib;             // array_ib             -> The b index when an array is iterated
};

class DB_PUBLIC ChildCellFilterState
  : public FilterStateBase
{
public:
  ChildCellFilterState (const FilterBase *filter, const NameFilterArgument &pattern, ChildCellFilterInstanceMode instance_mode, tl::Eval &eval, db::Layout *layout, bool reading, const ChildCellFilterPropertyIDs &pids)
    : FilterStateBase (filter, layout, eval),
      m_pattern (pattern, eval), m_instance_mode (instance_mode), mp_parent (0), m_pids (pids),
      m_weight (0), m_references (0), m_weight_set (false), m_references_set (false), m_reading (reading),
      m_cell_index (std::numeric_limits<db::cell_index_type>::max ())
  {
    //  .. nothing yet ..
  }

  bool cell_matches (db::cell_index_type ci)
  {
    //  prefilter with the cell objectives
    if (! objectives ().wants_cell (ci)) {
      return false;
    }

    if (m_pattern.is_catchall ()) {
      return true;
    } else if (m_cell_index != std::numeric_limits<db::cell_index_type>::max ()) {
      return ci == db::cell_index_type (m_cell_index);
    } else if (m_pattern.is_const ()) {
      if (m_pattern.match (layout ()->cell (ci).get_qualified_name ())) {
        m_cell_index = ci;
        return true;
      } else {
        return false;
      }
    } else {
      return (m_pattern.match (layout ()->cell (ci).get_qualified_name ()));
    }
  }

  virtual void do_init ()
  {
    if (m_pattern.is_catchall () || m_pattern.needs_eval ()) {

      if (! objectives ().wants_all_cells ()) {

        //  wildcard or unknown filter: include the parent cells if specific child cells are looked for

        int levels = 1;
        for (size_t i = 0; i < followers ().size (); ++i) {
          if (followers ()[i] == 0) {
            //  this is a sign of recursion - collect caller cells from all levels.
            levels = -1;
          }
        }

        //  this means, one follower wants only certain cells. We can optimize by only checking for potential parents
        std::set<db::cell_index_type> callers;
        for (FilterStateObjectives::cell_iterator c = objectives ().begin_cells (); c != objectives ().end_cells (); ++c) {
          layout ()->cell (*c).collect_caller_cells (callers, levels);
        }

        for (std::set<db::cell_index_type>::const_iterator c = callers.begin (); c != callers.end (); ++c) {
          objectives ().request_cell (*c);
        }

      }

    } else {

      objectives ().set_wants_all_cells (false);

      //  include all matching cells into the objectives
      for (db::Layout::const_iterator c = layout ()->begin (); c != layout ()->end(); ++c) {
        if (m_pattern.match (c->get_qualified_name ())) {
          objectives ().request_cell (c->cell_index ());
        }
      }

    }
  }

  virtual void reset (FilterStateBase *previous) 
  {
    FilterStateBase::reset (previous);

    m_pattern.reset ();
    m_ignored.clear ();

    //  Get the parent cell by asking the previous states 
    mp_parent = 0;
    tl::Variant parent_id;
    if (FilterStateBase::get_property (m_pids.cell_index, parent_id)) {
      if (layout ()->is_valid_cell_index (db::cell_index_type (parent_id.to_ulong ()))) {
        mp_parent = &layout ()->cell (db::cell_index_type (parent_id.to_ulong ()));
      }
    }

    m_parent_trans = db::ICplxTrans ();

    if (!mp_parent) {

      m_top_cell = layout ()->begin_top_down ();
      m_top_cell_end = layout ()->end_top_cells ();
      while (m_top_cell != m_top_cell_end && (!layout ()->is_valid_cell_index (*m_top_cell) || !cell_matches (*m_top_cell))) {
        ++m_top_cell;
      }

      m_weight = m_references = 0;
      m_weight_set = true;
      m_references_set = true;

    } else {

      if (m_instance_mode == NoInstances) {

        m_child_cell = mp_parent->begin_child_cells ();
        while (! m_child_cell.at_end () && (!layout ()->is_valid_cell_index (*m_child_cell) || !cell_matches (*m_child_cell))) {
          ++m_child_cell;
        } 

      } else {

        m_inst = mp_parent->begin_sorted_insts ();
        m_inst_end = mp_parent->end_sorted_insts ();

        while (m_inst != m_inst_end) {

          db::cell_index_type cid = (*m_inst)->object ().cell_index ();
          if (layout ()->is_valid_cell_index (cid) && cell_matches (cid)) {
            break;
          }

          ++m_inst;
          while (m_inst != m_inst_end && (*m_inst)->object ().cell_index () == cid) {
            ++m_inst;
          }

        }

        if (m_inst != m_inst_end && !m_reading) {
          m_i = mp_parent->sorted_inst_ptr (std::distance (mp_parent->begin_sorted_insts (), m_inst));
        }

        if (m_inst != m_inst_end && m_instance_mode == ExplodedInstances) {
          m_array_iter = (*m_inst)->begin ();
        }

      }

      tl::Variant v;
      if (previous->get_property (m_pids.path_trans, v)) {
        m_parent_trans = v.to_user<db::ICplxTrans> ();
      }

      m_weight_set = false;
      m_references_set = false;

    }
  }

  virtual void next (bool) 
  {
    if (mp_parent) {

      if (m_instance_mode == NoInstances) {

        do {
          ++m_child_cell;
        } while (! m_child_cell.at_end () && (!layout ()->is_valid_cell_index (*m_child_cell) || !cell_matches (*m_child_cell)));

      } else {

        if (m_instance_mode == ExplodedInstances) {
          ++m_array_iter;
        }

        if (m_instance_mode != ExplodedInstances || m_array_iter.at_end ()) {

          if (! m_reading && m_i != mp_parent->sorted_inst_ptr (std::distance (mp_parent->begin_sorted_insts (), m_inst))) {
            m_ignored.insert (m_i);
          }

          do {

            db::cell_index_type cid = (*m_inst)->object ().cell_index ();
            ++m_inst;

            if (m_inst != m_inst_end && (*m_inst)->object ().cell_index () != cid) {

              while (m_inst != m_inst_end) {

                cid = (*m_inst)->object ().cell_index ();
                if (layout ()->is_valid_cell_index (cid) && cell_matches (cid)) {
                  break;
                }

                ++m_inst;
                while (m_inst != m_inst_end && (*m_inst)->object ().cell_index () == cid) {
                  ++m_inst;
                }

              }

            }

            if (! m_reading && m_inst != m_inst_end) {
              m_i = mp_parent->sorted_inst_ptr (std::distance (mp_parent->begin_sorted_insts (), m_inst));
            } else {
              break;
            }

          } while (m_ignored.find (m_i) != m_ignored.end ());

          if (m_inst != m_inst_end) {
            m_array_iter = (*m_inst)->begin ();
          }

        }

      }

      m_weight_set = false;
      m_references_set = false;

    } else {

      do {
        ++m_top_cell;
      } while (m_top_cell != m_top_cell_end && (!layout ()->is_valid_cell_index (*m_top_cell) || !cell_matches (*m_top_cell)));

    }
  }

  virtual bool at_end () 
  {
    if (mp_parent) {
      if (m_instance_mode == NoInstances) {
        return m_child_cell.at_end ();
      } else {
        return m_inst == m_inst_end;
      }
    } else {
      return m_top_cell == m_top_cell_end;
    }
  }

  db::cell_index_type cell_index () const
  {
    if (mp_parent) {
      if (m_instance_mode == NoInstances) {
        return *m_child_cell;
      } else {
        return (*m_inst)->object ().cell_index ();
      }
    } else {
      return *m_top_cell;
    }
  }

  bool get_property (unsigned int id, tl::Variant &v)
  {
    if (id == m_pids.bbox || id == m_pids.cell_bbox) {

      if (! layout ()->is_valid_cell_index (cell_index ())) {
        v = tl::Variant ();
      } else {
        v = tl::Variant::make_variant (layout ()->cell (cell_index ()).bbox ());
      }
      return true;

    } else if (id == m_pids.dbbox || id == m_pids.cell_dbbox) {

      if (! layout ()->is_valid_cell_index (cell_index ())) {
        v = tl::Variant ();
      } else {
        v = tl::Variant::make_variant (db::CplxTrans (layout ()->dbu ()) * layout ()->cell (cell_index ()).bbox ());
      }
      return true;

    } else if (id == m_pids.cell_name) {

      if (! layout ()->is_valid_cell_index (cell_index ())) {
        v = tl::Variant ();
      } else {
        v = layout ()->cell (cell_index ()).get_qualified_name ();
      }
      return true;

    } else if (id == m_pids.cell_index) {

      v = cell_index ();
      return true;

    } else if (id == m_pids.cell) {

      if (! layout ()->is_valid_cell_index (cell_index ())) {
        v = tl::Variant ();
      } else {
        if (m_reading) {
          v = tl::Variant::make_variant_ref ((const db::Cell *) &layout ()->cell (cell_index ()));
        } else {
          v = tl::Variant::make_variant_ref ((db::Cell *) &layout ()->cell (cell_index ()));
        }
      }
      return true;

    } else if (id == m_pids.initial_cell_name) {

      if (! mp_parent) {
        return get_property (m_pids.cell_name, v);
      } else {
        return FilterStateBase::get_property (id, v);
      }

    } else if (id == m_pids.initial_cell_index) {

      if (! mp_parent) {
        return get_property (m_pids.cell_index, v);
      } else {
        return FilterStateBase::get_property (id, v);
      }

    } else if (id == m_pids.initial_cell) {

      if (! mp_parent) {
        if (! layout ()->is_valid_cell_index (cell_index ())) {
          v = tl::Variant ();
        } else if (m_reading) {
          v = tl::Variant::make_variant_ref ((const db::Cell *) &layout ()->cell (cell_index ()));
        } else {
          v = tl::Variant::make_variant_ref ((db::Cell *) &layout ()->cell (cell_index ()));
        }
        return true;
      } else {
        return FilterStateBase::get_property (id, v);
      }

    } else if (id == m_pids.parent_cell_name) {

      if (mp_parent) {
        if (! layout ()->is_valid_cell_index (cell_index ())) {
          v = tl::Variant ();
        } else {
          v = layout ()->cell (mp_parent->cell_index ()).get_qualified_name ();
        }
      } else {
        v = tl::Variant (); // no parent -> nil
      }
      return true;

    } else if (id == m_pids.parent_cell_index) {

      if (mp_parent) {
        v = mp_parent->cell_index ();
        return true;
      } else {
        return false;
      }

    } else if (id == m_pids.parent_cell) {

      if (mp_parent) {
        if (m_reading) {
          v = tl::Variant::make_variant_ref ((const db::Cell *) mp_parent);
        } else {
          v = tl::Variant::make_variant_ref ((db::Cell *) mp_parent);
        }
        return true;
      } else {
        return false;
      }

    } else if (id == m_pids.path) {

      if (! v.is_list ()) {
        std::vector<tl::Variant> vd;
        v = tl::Variant (vd.begin (), vd.end ());
      }

      if (mp_parent) {
        FilterStateBase::get_property (id, v);
      }

      v.push (tl::Variant (cell_index ()));
      return true;

    } else if (id == m_pids.path_names) {

      if (! v.is_list ()) {
        std::vector<tl::Variant> vd;
        v = tl::Variant (vd.begin (), vd.end ());
      }

      if (mp_parent) {
        FilterStateBase::get_property (id, v);
      }

      if (! layout ()->is_valid_cell_index (cell_index ())) {
        v.push (tl::Variant ());
      } else {
        v.push (tl::Variant (layout ()->cell (cell_index ()).get_qualified_name ()));
      }
      return true;

    } else if (id == m_pids.hier_levels) {

      if (! mp_parent) {
        v = 0;
        return true;
      } else if (FilterStateBase::get_property (id, v)) {
        v = v.to_long () + 1;
        return true;
      } else {
        return false;
      }

    } else if (id == m_pids.weight) {

      if (m_instance_mode == NoInstances) {

        if (! m_weight_set) {
          m_weight = m_child_cell.weight ();
          m_weight_set = true;
        }

        v = m_weight;
        return true;

      } else {
        return false;
      }

    } else if (id == m_pids.references) {

      if (m_instance_mode == NoInstances) {

        if (! m_references_set) {
          m_references = m_child_cell.instances ();
          m_references_set = true;
        }

        v = m_references;
        return true;

      } else {
        return false;
      }

    } else if (id == m_pids.tot_weight) {

      tl::Variant w;
      if (! get_property (m_pids.weight, w)) {
        return false;
      }

      if (! mp_parent) {
        v = 0;
        return true;
      } else if (FilterStateBase::get_property (id, v)) {
        if (v.to_long () == 0) {
          v = w;
        } else {
          v = w.to_long () * v.to_long ();
        }
        return true;
      } else {
        return false;
      }

    } else if (id == m_pids.inst_bbox) {

      if (mp_parent) {

        if (m_instance_mode == ExplodedInstances) {

          db::ICplxTrans t = m_parent_trans;
          t *= (*m_inst)->complex_trans (*m_array_iter);
          db::Box box (t * layout ()->cell ((*m_inst)->object ().cell_index ()).bbox ());
          v = tl::Variant::make_variant (box);
          return true;

        } else if (m_instance_mode == ArrayInstances) {

          db::ICplxTrans t = m_parent_trans;
          t *= (*m_inst)->complex_trans ();
          db::box_convert <db::CellInst> bc (*layout ());
          db::Box box (t * (*m_inst)->bbox (bc));
          v = tl::Variant::make_variant (box);
          return true;

        } else {
          return false;
        }

      } else {
        return false;
      }

    } else if (id == m_pids.inst_dbbox) {

      if (mp_parent) {

        if (m_instance_mode == ExplodedInstances) {

          db::ICplxTrans t = m_parent_trans;
          t *= (*m_inst)->complex_trans (*m_array_iter);
          db::DBox box (db::CplxTrans (layout ()->dbu ()) * t * layout ()->cell ((*m_inst)->object ().cell_index ()).bbox ());
          v = tl::Variant::make_variant (box);
          return true;

        } else if (m_instance_mode == ArrayInstances) {

          db::ICplxTrans t = m_parent_trans;
          t *= (*m_inst)->complex_trans ();
          db::box_convert <db::CellInst> bc (*layout ());
          db::DBox box (db::CplxTrans (layout ()->dbu ()) * t * (*m_inst)->bbox (bc));
          v = tl::Variant::make_variant (box);
          return true;

        } else {
          return false;
        }

      } else {
        return false;
      }

    } else if (id == m_pids.path_trans) {

      if (mp_parent) {

        if (m_instance_mode == ExplodedInstances) {

          db::ICplxTrans t = m_parent_trans;
          t *= (*m_inst)->complex_trans (*m_array_iter);
          v = tl::Variant::make_variant (t);
          return true;

        } else if (m_instance_mode == ArrayInstances) {

          db::ICplxTrans t = m_parent_trans;
          t *= (*m_inst)->complex_trans ();
          v = tl::Variant::make_variant (t);
          return true;

        } else {
          v = tl::Variant::make_variant (db::ICplxTrans ());
          return true;
        }

      } else {
        v = tl::Variant::make_variant (db::ICplxTrans ());
        return true;
      }

    } else if (id == m_pids.path_dtrans) {

      if (mp_parent) {

        if (m_instance_mode == ExplodedInstances) {

          db::ICplxTrans t = m_parent_trans;
          t *= (*m_inst)->complex_trans (*m_array_iter);
          db::CplxTrans tdbu (layout ()->dbu ());
          v = tl::Variant::make_variant (tdbu * t * tdbu.inverted ());
          return true;

        } else if (m_instance_mode == ArrayInstances) {

          db::ICplxTrans t = m_parent_trans;
          t *= (*m_inst)->complex_trans ();
          db::CplxTrans tdbu (layout ()->dbu ());
          v = tl::Variant::make_variant (tdbu * t * tdbu.inverted ());
          return true;

        } else {
          v = tl::Variant::make_variant (db::ICplxTrans ());
          return true;
        }

      } else {
        v = tl::Variant::make_variant (db::ICplxTrans ());
        return true;
      }

    } else if (id == m_pids.trans) {

      if (mp_parent) {

        if (m_instance_mode == ExplodedInstances) {

          v = tl::Variant::make_variant ((*m_inst)->complex_trans (*m_array_iter));
          return true;

        } else if (m_instance_mode == ArrayInstances) {

          v = tl::Variant::make_variant ((*m_inst)->complex_trans ());
          return true;

        } else {
          return false;
        }

      } else {
        return false;
      }

    } else if (id == m_pids.dtrans) {

      if (mp_parent) {

        if (m_instance_mode == ExplodedInstances) {

          db::CplxTrans tdbu (layout ()->dbu ());
          v = tl::Variant::make_variant (tdbu * (*m_inst)->complex_trans (*m_array_iter) * tdbu.inverted ());
          return true;

        } else if (m_instance_mode == ArrayInstances) {

          db::CplxTrans tdbu (layout ()->dbu ());
          v = tl::Variant::make_variant (tdbu * (*m_inst)->complex_trans () * tdbu.inverted ());
          return true;

        } else {
          return false;
        }

      } else {
        return false;
      }

    } else if (id == m_pids.inst) {

      if (! mp_parent || m_instance_mode == NoInstances) {
        return false;
      } else {
        if (m_reading) {
          v = tl::Variant::make_variant (mp_parent->sorted_inst_ptr (std::distance (mp_parent->begin_sorted_insts (), m_inst)), true /*is_const*/);
        } else {
          v = tl::Variant::make_variant_ref (&m_i);
        }
        return true;
      }

    } else if (id == m_pids.array_ia) {

      if (! mp_parent || m_instance_mode != ExplodedInstances) {
        return false;
      } else {
        v = m_array_iter.index_a ();
        return true;
      }

    } else if (id == m_pids.array_ib) {

      if (! mp_parent || m_instance_mode != ExplodedInstances) {
        return false;
      } else {
        v = m_array_iter.index_b ();
        return true;
      }

    } else if (id == m_pids.array_a || id == m_pids.array_b || id == m_pids.array_da || id == m_pids.array_db || id == m_pids.array_na || id == m_pids.array_nb) {

      if (! mp_parent || m_instance_mode == NoInstances) {
        return false;
      } else {
        db::Vector a, b;
        unsigned long na = 0, nb = 0;
        if ((*m_inst)->is_regular_array (a, b, na, nb)) {
          if (id == m_pids.array_a) {
            v = tl::Variant::make_variant (a);
          } else if (id == m_pids.array_da) {
            v = tl::Variant::make_variant (db::CplxTrans (layout ()->dbu ()) * a);
          } else if (id == m_pids.array_b) {
            v = tl::Variant::make_variant (b);
          } else if (id == m_pids.array_db) {
            v = tl::Variant::make_variant (db::CplxTrans (layout ()->dbu ()) * b);
          } else if (id == m_pids.array_na) {
            v = na;
          } else if (id == m_pids.array_nb) {
            v = nb;
          }
          return true;
        } else {
          v = tl::Variant ();
          return true;
        }
      }

    } else {
      return FilterStateBase::get_property (id, v);
    }
  }

  virtual void dump () const
  {
    std::cout << "ChildCellFilterState";
    FilterStateBase::dump ();
  }

private:
  NameFilter m_pattern;
  ChildCellFilterInstanceMode m_instance_mode;
  const db::Cell *mp_parent;
  ChildCellFilterPropertyIDs m_pids;
  db::Layout::top_down_const_iterator m_top_cell, m_top_cell_end;
  db::Cell::child_cell_iterator m_child_cell, m_child_cell_end;
  db::Cell::sorted_inst_iterator m_inst, m_inst_end; 
  db::CellInstArray::iterator m_array_iter;
  db::ICplxTrans m_parent_trans;
  size_t m_weight, m_references;
  bool m_weight_set, m_references_set;
  bool m_reading;
  std::set<db::Instance> m_ignored;
  db::Instance m_i;
  db::cell_index_type m_cell_index;
};

class DB_PUBLIC ChildCellFilter
  : public FilterBracket
{
public:
  ChildCellFilter (LayoutQuery *q, const NameFilterArgument &pattern, ChildCellFilterInstanceMode instance_mode, bool reading)
    : FilterBracket (q), 
      m_pids (q, instance_mode),
      m_pattern (pattern),
      m_instance_mode (instance_mode),
      m_reading (reading)
  {
    // .. nothing yet ..
  }

  FilterStateBase *do_create_state (db::Layout *layout, tl::Eval &eval) const
  {
    return new ChildCellFilterState (this, m_pattern, m_instance_mode, eval, layout, m_reading, m_pids);
  }

  FilterBase *clone (LayoutQuery *q) const
  {
    return new ChildCellFilter (q, m_pattern, m_instance_mode, m_reading);
  }

  virtual void dump (unsigned int l) const
  {
    for (unsigned int i = 0; i < l; ++i) {
      std::cout << "  ";
    }
    std::cout << "ChildCellFilter (" << m_pattern.pattern () << ", " << (int)m_instance_mode << ") :" << std::endl;
    FilterBracket::dump (l + 1);
  }

private:
  ChildCellFilterPropertyIDs m_pids;
  NameFilterArgument m_pattern;
  ChildCellFilterInstanceMode m_instance_mode;
  bool m_reading;
};

// --------------------------------------------------------------------------------
//  CellFilter definition and implementation

struct CellFilterPropertyIDs
{
  CellFilterPropertyIDs (LayoutQuery *q)
  {
    path               = q->register_property ("path", LQ_variant);
    path_names         = q->register_property ("path_names", LQ_variant);
    initial_cell       = q->register_property ("initial_cell", LQ_cell);
    initial_cell_index = q->register_property ("initial_cell_index", LQ_variant);
    initial_cell_name  = q->register_property ("initial_cell_name", LQ_variant);
    cell               = q->register_property ("cell", LQ_cell);
    cell_index         = q->register_property ("cell_index", LQ_variant);
    cell_name          = q->register_property ("cell_name", LQ_variant);
    hier_levels        = q->register_property ("hier_levels", LQ_variant);
    references         = q->register_property ("references", LQ_variant);
    weight             = q->register_property ("weight", LQ_variant);
    tot_weight         = q->register_property ("tot_weight", LQ_variant);
    instances          = q->register_property ("instances", LQ_variant);
    bbox               = q->register_property ("bbox", LQ_box);
    dbbox              = q->register_property ("dbbox", LQ_dbox);
    cell_bbox          = q->register_property ("cell_bbox", LQ_box);
    cell_dbbox         = q->register_property ("cell_dbbox", LQ_dbox);
    path_trans         = q->register_property ("path_trans", LQ_trans);
    path_dtrans        = q->register_property ("path_dtrans", LQ_dtrans);
  }

  unsigned int path;                // path                 -> Variant array with the indexes of the cells in that path
  unsigned int path_names;          // path_names           -> Variant array with the names of the cells in that path
  unsigned int initial_cell;        // initial_cell         -> Pointer to initial cell (first of path)
  unsigned int initial_cell_index;  // initial_cell_index   -> Index of initial cell (first of path)
  unsigned int initial_cell_name;   // initial_cell_name    -> Name of initial cell (first of path)
                                    //                         The path is a dummy path consisting of the cell as a single
                                    //                         element. It is provided to allow using the CellFilter as the
                                    //                         front of a instantiation path.
                                    //                         Also, the initial cell is identical to cell.
  unsigned int cell;                // cell                 -> Pointer to current cell (last of path)
  unsigned int cell_index;          // cell_index           -> Index of current cell (last of path)
  unsigned int cell_name;           // cell_name            -> Name of current cell (last of path)
  unsigned int hier_levels;         // hier_levels          -> Number of hierarchy levels in path (length of path - 1)
  unsigned int references;          // references           -> The number of instances (arefs count as 1) of this cell in the parent cell
  unsigned int weight;              // weight               -> The number of instances (arefs are flattened) of this cell in the parent cell
  unsigned int tot_weight;          // tot_weight           -> The number of instances of this cell in the initial cell along the given path
  unsigned int instances;           // instances            -> The number of instances of this cell in the previous cell (or over all if there is no previous cell)
  unsigned int bbox;                // bbox                 -> Cell bounding box
  unsigned int dbbox;               // dbbox                -> Cell bounding box in micrometer units
  unsigned int cell_bbox;           // cell_bbox            -> == bbox
  unsigned int cell_dbbox;          // cell_dbbox           -> == dbbox
  unsigned int path_trans;          // parent_trans         -> transformation to initial cell
  unsigned int path_dtrans;         // parent_dtrans        -> transformation to initial cell in micrometer units
};

class DB_PUBLIC CellFilterState
  : public FilterStateBase
{
public:
  CellFilterState (const FilterBase *filter, const NameFilterArgument &pattern, tl::Eval &eval, db::Layout *layout, bool reading, const CellFilterPropertyIDs &pids)
    : FilterStateBase (filter, layout, eval),
      m_pids (pids),
      m_pattern (pattern, eval),
      mp_parent (0),
      m_reading (reading),
      m_cell_index (std::numeric_limits<db::cell_index_type>::max ())
  {
    //  .. nothing yet ..
  }

  bool cell_matches (db::cell_index_type ci)
  {
    if (m_pattern.is_catchall ()) {
      return true;
    } else if (m_cell_index != std::numeric_limits<db::cell_index_type>::max ()) {
      return ci == db::cell_index_type (m_cell_index);
    } else if (m_pattern.is_const ()) {
      if (m_pattern.match (layout ()->cell (ci).get_qualified_name ())) {
        m_cell_index = ci;
        return true;
      } else {
        return false;
      }
    } else {
      return (m_pattern.match (layout ()->cell (ci).get_qualified_name ()));
    }
  }

  virtual void reset (FilterStateBase *previous) 
  {
    FilterStateBase::reset (previous);

    m_pattern.reset ();

    m_cell = layout ()->begin_top_down ();
    m_cell_end = layout ()->end_top_down ();

    while (m_cell != m_cell_end && !cell_matches (*m_cell)) {
      ++m_cell;
    }

    //  Get the parent cell by asking the previous states 
    mp_parent = 0;
    tl::Variant parent_id;
    if (FilterStateBase::get_property (m_pids.cell_index, parent_id)) {
      mp_parent = &layout ()->cell (db::cell_index_type (parent_id.to_ulong ()));
    }

    m_cell_counter.reset (0);
  }

  virtual void next (bool) 
  {
    do {
      ++m_cell;
    } while (m_cell != m_cell_end && !cell_matches (*m_cell));
  }

  virtual bool at_end () 
  {
    return m_cell == m_cell_end;
  }

  bool get_property (unsigned int id, tl::Variant &v)
  {
    if (id == m_pids.bbox || id == m_pids.cell_bbox) {

      if (! layout ()->is_valid_cell_index (*m_cell)) {
        v = tl::Variant ();
      } else {
        v = tl::Variant::make_variant (layout ()->cell (*m_cell).bbox ());
      }
      return true;

    } else if (id == m_pids.dbbox || id == m_pids.cell_dbbox) {

      if (! layout ()->is_valid_cell_index (*m_cell)) {
        v = tl::Variant ();
      } else {
        v = tl::Variant::make_variant (db::CplxTrans (layout ()->dbu ()) * layout ()->cell (*m_cell).bbox ());
      }
      return true;

    } else if (id == m_pids.cell_name || id == m_pids.initial_cell_name) {

      if (! layout ()->is_valid_cell_index (*m_cell)) {
        v = tl::Variant ();
      } else {
        v = layout ()->cell (*m_cell).get_qualified_name ();
      }
      return true;

    } else if (id == m_pids.cell) {

      if (m_reading) {
        v = tl::Variant::make_variant_ref ((const db::Cell *) &layout ()->cell (*m_cell));
      } else {
        v = tl::Variant::make_variant_ref ((db::Cell *) &layout ()->cell (*m_cell));
      }
      return true;

    } else if (id == m_pids.initial_cell) {

      if (m_reading) {
        v = tl::Variant::make_variant_ref ((const db::Cell *) &layout ()->cell (*m_cell));
      } else {
        v = tl::Variant::make_variant_ref ((db::Cell *) &layout ()->cell (*m_cell));
      }
      return true;

    } else if (id == m_pids.cell_index || id == m_pids.initial_cell_index) {

      v = *m_cell;
      return true;

    } else if (id == m_pids.path_names) {

      std::vector<tl::Variant> vd;
      v = tl::Variant (vd.begin (), vd.end ());
      v.push (tl::Variant ());
      get_property (m_pids.cell_name, v.get_list ().back ());
      return true;

    } else if (id == m_pids.path) {

      std::vector<tl::Variant> vd;
      v = tl::Variant (vd.begin (), vd.end ());
      v.push (tl::Variant ());
      get_property (m_pids.cell_index, v.get_list ().back ());
      return true;

    } else if (id == m_pids.hier_levels) {

      v = 0;
      return true;

    } else if (id == m_pids.references || id == m_pids.weight || id == m_pids.tot_weight) {

      v = 0;
      return true;

    } else if (id == m_pids.instances) {

      if (! m_cell_counter.get ()) {
        if (mp_parent) {
          m_cell_counter.reset (new db::CellCounter (layout (), mp_parent->cell_index ()));
        } else {
          m_cell_counter.reset (new db::CellCounter (layout ()));
        }
      }

      if (! layout ()->is_valid_cell_index (*m_cell)) {
        v = tl::Variant ();
      } else {
        v = m_cell_counter->weight (*m_cell);
      }
      return true;

    } else if (id == m_pids.path_trans) {

      v = tl::Variant::make_variant (db::ICplxTrans ());
      return true;

    } else if (id == m_pids.path_dtrans) {

      v = tl::Variant::make_variant (db::DCplxTrans ());
      return true;

    } else {
      return FilterStateBase::get_property (id, v);
    }
  }

  virtual void dump () const
  {
    std::cout << "CellFilterState";
    FilterStateBase::dump ();
  }

private:
  CellFilterPropertyIDs m_pids;
  NameFilter m_pattern;
  const db::Cell *mp_parent;
  db::Layout::top_down_const_iterator m_cell, m_cell_end;
  std::unique_ptr<db::CellCounter> m_cell_counter;
  bool m_reading;
  db::cell_index_type m_cell_index;
};

class DB_PUBLIC CellFilter
  : public FilterBracket
{
public:
  CellFilter (LayoutQuery *q, const NameFilterArgument &pattern, bool reading)
    : FilterBracket (q), 
      m_pids (q),
      m_pattern (pattern),
      m_reading (reading)
  {
    // .. nothing yet ..
  }

  FilterStateBase *do_create_state (db::Layout *layout, tl::Eval &eval) const
  {
    return new CellFilterState (this, m_pattern, eval, layout, m_reading, m_pids);
  }

  FilterBase *clone (LayoutQuery *q) const
  {
    return new CellFilter (q, m_pattern, m_reading);
  }

  virtual void dump (unsigned int l) const
  {
    for (unsigned int i = 0; i < l; ++i) {
      std::cout << "  ";
    }
    std::cout << "CellFilter (" << m_pattern.pattern () << ") :" << std::endl;
    FilterBracket::dump (l + 1);
  }

  NameFilterArgument &name_filter ()
  {
    return m_pattern;
  }

private:
  CellFilterPropertyIDs m_pids;
  NameFilterArgument m_pattern;
  bool m_reading;
};

// --------------------------------------------------------------------------------
//  DeleteFilter definition and implementation

struct DeleteFilterPropertyIDs
{
  DeleteFilterPropertyIDs (LayoutQuery *q)
  {
    cell_index         = q->register_property ("cell_index", LQ_variant);
    inst               = q->register_property ("inst", LQ_instance);
    shape              = q->register_property ("shape", LQ_shape);
  }

  //  imported properties
  unsigned int cell_index;
  unsigned int inst;
  unsigned int shape;
};

class DB_PUBLIC DeleteFilterState
  : public FilterStateBase
{
public:
  DeleteFilterState (const FilterBase *filter, tl::Eval &eval, db::Layout *layout, const DeleteFilterPropertyIDs &pids, bool transparent)
    : FilterStateBase (filter, layout, eval), m_pids (pids), m_transparent (transparent), m_count (0)
  {
    //  .. nothing yet ..
  }

  virtual void reset (FilterStateBase *previous) 
  {
    FilterStateBase::reset (previous);

    if (! m_transparent) {
      do_delete ();
    } else {
      m_count = 0;
    }
  } 

  virtual void next (bool skip) 
  {
    if (m_transparent) {
      if (! m_count && ! skip) {
        do_delete ();
      } 
      ++m_count;
    }
  }

  virtual bool at_end () 
  {
    return ! m_transparent || m_count > 0;
  }

  bool get_property (unsigned int id, tl::Variant &v)
  {
    return m_transparent && FilterStateBase::get_property (id, v);
  }

  virtual void dump () const
  {
    if (m_transparent) {
      std::cout << "TransparentDeleteFilterState";
    } else {
      std::cout << "DeleteFilterState";
    }
    FilterStateBase::dump ();
  }

private:
  DeleteFilterPropertyIDs m_pids;
  bool m_transparent;
  unsigned int m_count;

  void do_delete ()
  {
    tl::Variant v;
    if (FilterStateBase::get_property (m_pids.shape, v)) {

      db::Shape *shape = &v.to_user<db::Shape> ();
      if (shape->shapes ()) {
        shape->shapes ()->erase_shape (*shape);
        *shape = db::Shape ();
      }

    } else if (FilterStateBase::get_property (m_pids.inst, v)) {

      db::Instance *instance = &v.to_user<db::Instance> ();
      if (instance->instances ()) {
        instance->instances ()->erase (*instance);
        *instance = db::Instance ();
      }

    } else if (FilterStateBase::get_property (m_pids.cell_index, v)) {

      db::cell_index_type cid = db::cell_index_type (v.to_ulong ());
      if (layout ()->is_valid_cell_index (cid)) {
        layout ()->delete_cell (cid);
      }

    }
  }

};

class DB_PUBLIC DeleteFilter
  : public FilterBracket
{
public:
  DeleteFilter (LayoutQuery *q, bool transparent)
    : FilterBracket (q), m_pids (q), m_transparent (transparent)
  {
    // .. nothing yet ..
  }

  FilterStateBase *do_create_state (db::Layout *layout, tl::Eval &eval) const
  {
    if (! layout->is_editable ()) {
      throw tl::Exception (tl::to_string (tr ("Cannot execute a delete query on a non-editable layout")));
    }
    return new DeleteFilterState (this, eval, layout, m_pids, m_transparent);
  }

  FilterBase *clone (LayoutQuery *q) const
  {
    return new DeleteFilter (q, m_transparent);
  }

  virtual void dump (unsigned int l) const
  {
    for (unsigned int i = 0; i < l; ++i) {
      std::cout << "  ";
    }
    if (m_transparent) {
      std::cout << "TransparentDeleteFilter ()" << std::endl;
    } else {
      std::cout << "DeleteFilter ()" << std::endl;
    }
    FilterBracket::dump (l + 1);
  }

private:
  DeleteFilterPropertyIDs m_pids;
  bool m_transparent;
};

// --------------------------------------------------------------------------------
//  WithDoFilter definition and implementation

class DB_PUBLIC WithDoFilterState
  : public FilterStateBase
{
public:
  WithDoFilterState (const FilterBase *filter, const std::string &do_expression, tl::Eval &eval, db::Layout *layout, bool transparent)
    : FilterStateBase (filter, layout, eval), m_transparent (transparent), m_count (0)
  {
    if (! do_expression.empty ()) {
      eval.parse (m_do_expression, do_expression, true);
    }
  }

  virtual void reset (FilterStateBase *previous) 
  {
    FilterStateBase::reset (previous);
    if (m_transparent) {
      m_count = 0;
    } else {
      m_do_expression.execute ();
    }
  }

  virtual void next (bool skip) 
  {
    if (m_transparent) {
      if (! m_count && ! skip) {
        m_do_expression.execute ();
      } 
      ++m_count;
    }
  }

  virtual bool at_end () 
  {
    return !m_transparent || m_count > 0;
  }

  bool get_property (unsigned int id, tl::Variant &v)
  {
    return m_transparent && FilterStateBase::get_property (id, v);
  }

  virtual void dump () const
  {
    if (m_transparent) {
      std::cout << "TransparentWithDoFilterState";
    } else {
      std::cout << "WithDoFilterState";
    }
    FilterStateBase::dump ();
  }

private:
  tl::Expression m_do_expression;
  bool m_transparent;
  unsigned int m_count;
};

class DB_PUBLIC WithDoFilter
  : public FilterBracket
{
public:
  WithDoFilter (LayoutQuery *q, const std::string &do_expression, bool transparent)
    : FilterBracket (q), 
      m_do_expression (do_expression), m_transparent (transparent)
  {
    // .. nothing yet ..
  }

  FilterStateBase *do_create_state (db::Layout *layout, tl::Eval &eval) const
  {
    if (! layout->is_editable ()) {
      throw tl::Exception (tl::to_string (tr ("Cannot execute a with .. do query on a non-editable layout")));
    }
    return new WithDoFilterState (this, m_do_expression, eval, layout, m_transparent);
  }

  FilterBase *clone (LayoutQuery *q) const
  {
    return new WithDoFilter (q, m_do_expression, m_transparent);
  }

  virtual void dump (unsigned int l) const
  {
    for (unsigned int i = 0; i < l; ++i) {
      std::cout << "  ";
    }
    if (m_transparent) {
      std::cout << "TransparentWithDoFilter (" << m_do_expression << ")" << std::endl;
    } else {
      std::cout << "WithDoFilter (" << m_do_expression << ")" << std::endl;
    }
    FilterBracket::dump (l + 1);
  }

private:
  std::string m_do_expression;
  bool m_transparent;
};

// --------------------------------------------------------------------------------
//  SelectFilter definition and implementation

struct SelectFilterPropertyIDs
{
  SelectFilterPropertyIDs (LayoutQuery *q)
  {
    data               = q->register_property ("data", LQ_variant);
  }

  unsigned int data;                // data                 -> An array of the selected values
};

class DB_PUBLIC SelectFilterReportingState
  : public FilterStateBase
{
public:
  SelectFilterReportingState (const FilterBase *filter, tl::Eval &eval, db::Layout *layout, bool unique, const SelectFilterPropertyIDs &pids)
    : FilterStateBase (filter, layout, eval), m_unique (unique), m_pids (pids)
  {
    //  .. nothing yet ..
  }

  virtual void reset (FilterStateBase * /*previous*/)
  {
    m_sorted = m_sorted_data.begin ();
  }

  virtual void next (bool) 
  {
    std::multimap <tl::Variant, tl::Variant>::const_iterator s = m_sorted;
    do {
      ++m_sorted;
    } while (m_unique && m_sorted != m_sorted_data.end () && m_sorted->second == s->second); 
  }

  virtual bool at_end () 
  {
    return (m_sorted == m_sorted_data.end ());
  }

  bool get_property (unsigned int id, tl::Variant &v)
  {
    if (id == m_pids.data) {
      v = m_sorted->second;
      return true;
    } else {
      return false;
    }
  }

  tl::Variant &insert (const tl::Variant &k)
  {
    return m_sorted_data.insert (std::make_pair (k, tl::Variant ()))->second;
  }

private:
  bool m_unique;
  SelectFilterPropertyIDs m_pids;
  std::multimap <tl::Variant, tl::Variant> m_sorted_data;
  std::multimap <tl::Variant, tl::Variant>::const_iterator m_sorted;
};

class DB_PUBLIC SelectFilterState
  : public FilterStateBase
{
public:
  SelectFilterState (const FilterBase *filter, const std::vector<std::string> &expressions, const std::string &sort_expression, bool unique, tl::Eval &eval, db::Layout *layout, const SelectFilterPropertyIDs &pids)
    : FilterStateBase (filter, layout, eval),
      m_pids (pids), m_has_sorting (false), m_unique (unique), m_done (false), m_in_data_eval (false), mp_reporter_state (0)
  {
    for (std::vector<std::string>::const_iterator e = expressions.begin (); e != expressions.end (); ++e) {
      m_expressions.push_back (tl::Expression ());
      eval.parse (m_expressions.back (), *e, true);
    }

    if (! sort_expression.empty ()) {
      eval.parse (m_sort_expression, sort_expression, true);
      m_has_sorting = true;
    }
  }

  void get_data (tl::Variant &v) 
  {
    if (m_in_data_eval) {
      v = tl::Variant ();
      return;
    }

    m_in_data_eval = true;

    try {

      std::vector<tl::Variant> vd;
      v = tl::Variant (vd.begin (), vd.end ());
      for (std::vector<tl::Expression>::const_iterator e = m_expressions.begin (); e != m_expressions.end (); ++e) {
        v.push (e->execute ());
      }

      m_in_data_eval = false;

    } catch (...) {
      m_in_data_eval = false;
      throw;
    }
  }

  virtual void reset (FilterStateBase *previous) 
  {
    if (m_has_sorting) {

      if (! mp_reporter_state) {

        //  Install the reporter state at top level - this will finally report all results
        mp_reporter_state = new SelectFilterReportingState (filter (), eval (), layout (), m_unique, m_pids);
        FilterStateBase *p = previous;
        while (p->previous ()) {
          p = p->previous ();
        }

        p->connect (mp_reporter_state);

      }

      get_data (mp_reporter_state->insert (m_sort_expression.execute ()));
      m_done = true;

    }

    FilterStateBase::reset (previous);
    m_done = false;
  }

  virtual void next (bool) 
  {
    m_done = true;
  }

  virtual bool at_end () 
  {
    if (m_has_sorting) {
      return true;
    } else {
      return m_done;
    }
  }

  bool get_property (unsigned int id, tl::Variant &v)
  {
    if (id == m_pids.data) {
      get_data (v);
      return true;
    } else if (m_in_data_eval) {
      return FilterStateBase::get_property (id, v);
    } else {
      //  externals don't get to see the internal properties
      return false;
    }
  }

  virtual void dump () const
  {
    std::cout << "SelectFilterState";
    FilterStateBase::dump ();
  }

private:
  SelectFilterPropertyIDs m_pids;
  std::vector <tl::Expression> m_expressions;
  tl::Expression m_sort_expression;
  bool m_has_sorting;
  bool m_unique;
  bool m_done;
  bool m_in_data_eval;
  SelectFilterReportingState *mp_reporter_state;
};

class DB_PUBLIC SelectFilter
  : public FilterBracket
{
public:
  SelectFilter (LayoutQuery *q, const std::vector<std::string> &expressions, const std::string &sort_expression, bool unique)
    : FilterBracket (q), 
      m_pids (q),
      m_expressions (expressions),
      m_sort_expression (sort_expression),
      m_unique (unique)
  {
    // .. nothing yet ..
  }

  FilterStateBase *do_create_state (db::Layout *layout, tl::Eval &eval) const
  {
    return new SelectFilterState (this, m_expressions, m_sort_expression, m_unique, eval, layout, m_pids);
  }

  FilterBase *clone (LayoutQuery *q) const
  {
    return new SelectFilter (q, m_expressions, m_sort_expression, m_unique);
  }

  virtual void dump (unsigned int l) const
  {
    for (unsigned int i = 0; i < l; ++i) {
      std::cout << "  ";
    }
    std::cout << "SelectFilter (";
    for (unsigned int i = 0; i < m_expressions.size (); ++i) {
      if (i > 0) {
        std::cout << ",";
      }
      std::cout << m_expressions[i];
    }
    if (! m_sort_expression.empty ()) {
      std::cout << " sorted by " << m_sort_expression << " unique=" << m_unique;
    }
    std::cout << ")" << std::endl;
    FilterBracket::dump (l + 1);
  }

private:
  SelectFilterPropertyIDs m_pids;
  std::vector<std::string> m_expressions;
  std::string m_sort_expression;
  bool m_unique;
};

// --------------------------------------------------------------------------------
//  ConditionalFilter

class DB_PUBLIC ConditionalFilterState
  : public FilterStateBase
{
public:
  ConditionalFilterState (const FilterBase *filter, tl::Eval &eval, const std::string &expr, db::Layout *layout)
    : FilterStateBase (filter, layout, eval), m_select (false)
  {
    eval.parse (m_expression, expr, true);
  }

  virtual void reset (FilterStateBase *previous) 
  {
    FilterStateBase::reset (previous);
    m_select = m_expression.execute ().to_bool ();
  }

  virtual void next (bool) 
  {
    m_select = false;
  }

  virtual bool at_end () 
  {
    return ! m_select;
  }

  virtual void dump () const
  {
    std::cout << "ConditionalFilterState";
    FilterStateBase::dump ();
  }

private:
  bool m_select;
  tl::Expression m_expression;
};

class DB_PUBLIC ConditionalFilter
  : public FilterBracket
{
public:
  ConditionalFilter (LayoutQuery *q, const std::string &expr)
    : FilterBracket (q), m_expr (expr)
  {
    // .. nothing yet ..
  }

  FilterStateBase *do_create_state (db::Layout *layout, tl::Eval &eval) const
  {
    return new ConditionalFilterState (this, eval, m_expr, layout);
  }

  FilterBase *clone (LayoutQuery *q) const
  {
    return new ConditionalFilter (q, m_expr);
  }

  virtual void dump (unsigned int l) const
  {
    for (unsigned int i = 0; i < l; ++i) {
      std::cout << "  ";
    }
    std::cout << "ConditionalFilter (" << m_expr << ") :" << std::endl;
    FilterBracket::dump (l + 1);
  }

private:
  std::string m_expr;
};

// --------------------------------------------------------------------------------
//  FilterStateFunction adaptor class (creates a function for a tl::Eval object that
//  gets a property)

class FilterStateFunction
  : public tl::EvalFunction
{
public:
  FilterStateFunction (unsigned int prop_id, const std::vector<FilterStateBase *> *states)
    : m_prop_id (prop_id), mp_states (states)
  {
    //  .. nothing yet ..
  }

  void execute (const tl::ExpressionParserContext &context, tl::Variant &out, const std::vector<tl::Variant> &args) const 
  {
    if (args.size () > 0) {
      throw tl::EvalError (tl::to_string (tr ("Query function does not allow parameters")), context);
    }

    out = tl::Variant ();
    if (! mp_states->empty ()) {
      mp_states->back ()->get_property (m_prop_id, out);
    }
  }

private:
  unsigned int m_prop_id;
  const std::vector<FilterStateBase *> *mp_states; 
};

// --------------------------------------------------------------------------------
//  LayoutQueryIterator implementation

LayoutQueryIterator::LayoutQueryIterator (const LayoutQuery &q, db::Layout *layout, tl::Eval *parent_eval, tl::AbsoluteProgress *progress)
  : mp_q (const_cast<db::LayoutQuery *> (&q)), mp_layout (layout), m_eval (parent_eval), m_layout_ctx (layout, true /*can modify*/), mp_progress (progress), m_initialized (false)
{
  m_eval.set_ctx_handler (&m_layout_ctx);
  m_eval.set_var ("layout", tl::Variant::make_variant_ref (layout));
  for (unsigned int i = 0; i < mp_q->properties (); ++i) {
    m_eval.define_function (mp_q->property_name (i), new FilterStateFunction (i, &m_state));
  }

  //  Avoid update() calls while iterating in modifying mode
  mp_layout->update ();
  mp_layout->start_changes ();
}

LayoutQueryIterator::LayoutQueryIterator (const LayoutQuery &q, const db::Layout *layout, tl::Eval *parent_eval, tl::AbsoluteProgress *progress)
  : mp_q (const_cast<db::LayoutQuery *> (&q)), mp_layout (const_cast <db::Layout *> (layout)), m_eval (parent_eval), m_layout_ctx (layout), mp_progress (progress), m_initialized (false)
{
  //  TODO: check whether the query is a modifying one (with .. do, delete)

  m_eval.set_ctx_handler (&m_layout_ctx);
  m_eval.set_var ("layout", tl::Variant::make_variant_ref (layout));
  for (unsigned int i = 0; i < mp_q->properties (); ++i) {
    m_eval.define_function (mp_q->property_name (i), new FilterStateFunction (i, &m_state));
  }

  //  Avoid update() calls while iterating in modifying mode
  mp_layout->start_changes ();
}

LayoutQueryIterator::~LayoutQueryIterator ()
{
  mp_layout->end_changes ();
  if (m_initialized) {
    cleanup ();
  }
}

void
LayoutQueryIterator::ensure_initialized ()
{
  if (! m_initialized) {
    init ();
    m_initialized = true;
  }
}

void 
LayoutQueryIterator::init ()
{
  std::vector<FilterStateBase *> f;
  mp_root_state = mp_q->root ().create_state (f, mp_layout, m_eval, false);
  mp_root_state->init ();
  mp_root_state->reset (0);
  m_state.push_back (mp_root_state);

  while (! next_down ()) {
    next_up (false);
  }
}

void
LayoutQueryIterator::cleanup ()
{
  std::set<FilterStateBase *> states;
  collect (mp_root_state, states);
  for (std::set<FilterStateBase *>::iterator s = states.begin (); s != states.end (); ++s) {
    delete *s;
  }
  m_state.clear ();
  mp_root_state = 0;
}

void
LayoutQueryIterator::reset () 
{
  if (m_initialized) {

    //  forces an update if required
    mp_layout->end_changes ();
    mp_layout->start_changes ();

    cleanup ();
    init ();

  }
}

bool
LayoutQueryIterator::at_end () const
{
  const_cast<LayoutQueryIterator *> (this)->ensure_initialized ();
  return m_state.empty ();
}

bool
LayoutQueryIterator::get (const std::string &name, tl::Variant &v)
{
  ensure_initialized ();
  if (m_state.empty () || !m_state.back () || !mp_q->has_property (name)) {
    return false;
  } else {
    return m_state.back ()->get_property (mp_q->property_by_name (name), v);
  }
}

bool
LayoutQueryIterator::get (unsigned int id, tl::Variant &v)
{
  ensure_initialized ();
  if (m_state.empty () || !m_state.back ()) {
    return false;
  } else {
    return m_state.back ()->get_property (id, v);
  }
}

void 
LayoutQueryIterator::dump () const
{
  const_cast<LayoutQueryIterator *> (this)->ensure_initialized ();
  mp_root_state->dump ();
  std::cout << std::endl;
}

void 
LayoutQueryIterator::collect (FilterStateBase *state, std::set<FilterStateBase *> &states)
{
  if (states.find (state) == states.end ()) {
    states.insert (state);
    for (std::vector<FilterStateBase *>::const_iterator s = state->followers ().begin (); s != state->followers ().end (); ++s) {
      if (*s) {
        collect (*s, states);
      }
    }
  }
}

void 
LayoutQueryIterator::next (bool skip)
{
  ensure_initialized ();
  do {
    next_up (skip);
  } while (! next_down ());
}

void 
LayoutQueryIterator::next_up (bool skip)
{
  while (! m_state.empty ()) {
    if (mp_progress) {
      ++*mp_progress;
    }
    m_state.back ()->proceed (skip);
    if (m_state.back ()->at_end ()) {
      m_state.pop_back ();
    } else {
      break;
    }
  }
}

bool 
LayoutQueryIterator::next_down ()
{
  if (! m_state.empty ()) {

    while (true) {

      if (mp_progress) {
        ++*mp_progress;
      }

      FilterStateBase *new_state = m_state.back ()->child ();
      if (! new_state) {
        break;
      } else {

        new_state->reset (m_state.back ());
        if (! new_state->at_end ()) {
          m_state.push_back (new_state);
        } else { 
          return false;
        }

      }

    }

  }

  return true;
}

// --------------------------------------------------------------------------------
//  LayoutQuery implementation

static void
parse_cell_name_filter_seq (tl::Extractor &ex, LayoutQuery *q, FilterBracket *bracket, ChildCellFilterInstanceMode instance_mode, bool reading);

static FilterBase *
parse_cell_name_filter_element (tl::Extractor &ex, LayoutQuery *q, ChildCellFilterInstanceMode instance_mode, bool reading)
{
  tl::Extractor ex0 = ex;

  if (ex.test (")") || ex.test (",")) {

    ex = ex0;
    return 0;

  } else if (ex.test ("(")) {

    std::unique_ptr<FilterBracket> b (new FilterBracket (q));
    do {
      parse_cell_name_filter_seq (ex, q, b.get (), instance_mode, reading);
    } while (ex.test (",") || ex.test (s_or));

    //  TODO: do this in the optimization
    if (b->children ().size () == 1 && dynamic_cast<FilterBracket *> (b->children ()[0])) {
      FilterBracket *binner = dynamic_cast<FilterBracket *> (b->children ()[0]->clone (q));
      if (binner) {
        b.reset (binner);
      }
    }

    ex.expect (")");

    if (ex.test("*")) {

      b->set_loopmin (0);
      b->set_loopmax (std::numeric_limits<unsigned int>::max ());

    } else if (ex.test("?")) {

      b->set_loopmin (0);
      b->set_loopmax (1);

    } else if (ex.test ("[")) {
    
      unsigned int v1 = 0, v2 = std::numeric_limits<unsigned int>::max ();
      if (ex.try_read (v1)) {
        if (ex.test ("..")) {
          if (! ex.test ("*")) {
            ex.try_read (v2);
          }
        } else {
          v2 = v1;
        }
      }

      b->set_loopmin (v1);
      b->set_loopmax (v2);

      ex.expect ("]");

    }

    return b.release ();

  } else if (*ex.skip () == '.' && ex.get ()[1] == '.') {

    //  take the first dots in ".." or "..." as abbreviation for (.*)*
    while (ex.get ()[0] == '.' && ex.get ()[1] == '.') {
      ++ex;
    }

    std::unique_ptr<FilterBracket> b (new ChildCellFilter (q, NameFilterArgument ("*"), instance_mode, reading));
    b->set_loopmin (0);
    b->set_loopmax (std::numeric_limits<unsigned int>::max ());
    return b.release ();

  } else if (ex.test (".")) {

    NameFilterArgument name_filter;
    name_filter.parse (ex);

    if (! name_filter.empty ()) {
      return new ChildCellFilter (q, name_filter, instance_mode, reading);
    }

  } else {

    NameFilterArgument name_filter;
    name_filter.parse (ex);

    if (! name_filter.empty ()) {
      return new CellFilter (q, name_filter, reading);
    }

  }

  return 0;
}

static void
parse_cell_name_filter_seq (tl::Extractor &ex, LayoutQuery *q, FilterBracket *bracket, ChildCellFilterInstanceMode instance_mode, bool reading)
{
  FilterBase *f0 = 0;
  FilterBase *fl = 0;
  FilterBase *f;

  while (! ex.at_end ()) {
    
    if (check_trailing_reserved_word (ex) || (f = parse_cell_name_filter_element (ex, q, instance_mode, reading)) == 0) {
      break;
    }

    if (! f0) {
      f0 = f;
    }

    bracket->add_child (f);
    if (fl) {
      fl->connect (f);
    }

    fl = f;

  }

  //  satisfy instance mode if there is just a cell name filter
  CellFilter *cf;
  if (instance_mode != NoInstances && f0 == fl && (cf = dynamic_cast<CellFilter *> (f0)) != 0) {

    fl = new ChildCellFilter (q, cf->name_filter (), instance_mode, reading);
    bracket->add_child (fl);

    cf->name_filter () = NameFilterArgument ("*");
    cf->connect (fl);

  }

  if (f0) {
    bracket->connect_entry (f0);
  } 
  if (fl) {
    bracket->connect_exit (fl);
  }
}

void
parse_cell_filter (tl::Extractor &ex, LayoutQuery *q, FilterBracket *bracket, bool with_where_clause, bool reading)
{
  if (ex.test ("(")) {

    parse_cell_filter (ex, q, bracket, true, reading);
    ex.expect (")");

  } else {

    std::unique_ptr<FilterBracket> b (new FilterBracket (q));

    if (ex.test (s_instances)) {
      (ex.test (s_of) || ex.test (s_from)) && (ex.test (s_cells) || ex.test (s_cell));
      //  Because an array member cannot be modified we use ArrayInstances in the modification case always
      parse_cell_name_filter_seq (ex, q, b.get (), reading ? ExplodedInstances : ArrayInstances, reading);
    } else if (ex.test (s_arrays)) {
      (ex.test (s_of) || ex.test (s_from)) && (ex.test (s_cells) || ex.test (s_cell));
      parse_cell_name_filter_seq (ex, q, b.get (), ArrayInstances, reading);
    } else {
      ex.test (s_cells) || ex.test (s_cell);
      parse_cell_name_filter_seq (ex, q, b.get (), NoInstances, reading);
    }

    FilterBase *fl = 0, *f = 0;

    if (with_where_clause && ex.test (s_where)) {

      std::string expr = tl::Eval::parse_expr (ex, true);

      f = b.release ();
      bracket->add_child (f);
      bracket->connect_entry (f);

      fl = f;
      f = new ConditionalFilter (q, expr);
      bracket->add_child (f);
      fl->connect (f);

    } else {

      f = b.release ();
      bracket->add_child (f);
      bracket->connect_entry (f);

    }

    bracket->connect_exit (f);

  }
}

void
parse_filter (tl::Extractor &ex, LayoutQuery *q, FilterBracket *bracket, bool reading)
{
  unsigned int sf = (unsigned int) db::ShapeIterator::Nothing;
  do {
    if (ex.test (s_shapes)) {
      sf |= (unsigned int) db::ShapeIterator::All;
    } else if (ex.test (s_polygons)) {
      sf |= (unsigned int) db::ShapeIterator::Polygons;
    } else if (ex.test (s_boxes)) {
      sf |= (unsigned int) db::ShapeIterator::Boxes;
    } else if (ex.test (s_edges)) {
      sf |= (unsigned int) db::ShapeIterator::Edges;
    } else if (ex.test (s_paths)) {
      sf |= (unsigned int) db::ShapeIterator::Paths;
    } else if (ex.test (s_texts)) {
      sf |= (unsigned int) db::ShapeIterator::Texts;
    } else {
      break;
    }
  } while (ex.test (",") || ex.test (s_or));

  db::ShapeIterator::flags_type shapes = (db::ShapeIterator::flags_type) sf;

  if (shapes != db::ShapeIterator::Nothing) {

    db::LayerMap lm;

    if (ex.test (s_on)) {
      ex.test (s_layer) || ex.test (s_layers);
      lm.map_expr (ex, 0);
    }

    ex.test (s_of) || ex.test (s_from);

    std::unique_ptr<FilterBracket> b (new FilterBracket (q));
    parse_cell_filter (ex, q, b.get (), false, reading);

    FilterBase *f = 0, *fl = 0;
    
    f = b.release ();
    bracket->add_child (f);
    bracket->connect_entry (f);

    fl = f;
    f = new ShapeFilter (q, lm, shapes, reading);
    bracket->add_child (f);
    fl->connect (f);

    if (ex.test (s_where)) {

      std::string expr = tl::Eval::parse_expr (ex, true);

      fl = f;
      f = new ConditionalFilter (q, expr);
      bracket->add_child (f);
      fl->connect (f);

    }

    bracket->connect_exit (f);

  } else {
    parse_cell_filter (ex, q, bracket, true, reading);
  }
}

void
parse_statement (tl::Extractor &ex, LayoutQuery *q, FilterBracket *bracket, bool reading)
{
  if (ex.test (s_select)) {

    std::vector<std::string> expressions;

    do {
      expressions.push_back (tl::Eval::parse_expr (ex, true));
    } while (ex.test (","));

    ex.expect (s_from);

    std::unique_ptr<FilterBracket> b (new FilterBracket (q));
    parse_filter (ex, q, b.get (), true);

    bool unique = false;

    std::string sort_expression;
    if (ex.test (s_sorted)) {
      ex.test (s_by);
      sort_expression = tl::Eval::parse_expr (ex, true);
      unique = ex.test (s_unique);
    }

    FilterBase *f = b.release ();
    bracket->add_child (f);
    bracket->connect_entry (f);

    FilterBase *ff = new SelectFilter (q, expressions, sort_expression, unique);
    bracket->add_child (ff);
    f->connect (ff);

    bracket->connect_exit (ff);

  } else if (! reading && ex.test (s_with)) {

    std::unique_ptr<FilterBracket> b (new FilterBracket (q));
    parse_filter (ex, q, b.get (), false);

    ex.expect (s_do);

    std::string expression = tl::Eval::parse_expr (ex, true);

    bool transparent = ex.test (s_pass);

    FilterBase *f = b.release ();
    bracket->add_child (f);
    bracket->connect_entry (f);

    FilterBase *ff = new WithDoFilter (q, expression, transparent);
    bracket->add_child (ff);
    f->connect (ff);

    bracket->connect_exit (ff);

  } else if (! reading && ex.test (s_delete)) {

    std::unique_ptr<FilterBracket> b (new FilterBracket (q));
    parse_filter (ex, q, b.get (), false);

    bool transparent = ex.test (s_pass);

    FilterBase *f = b.release ();
    bracket->add_child (f);
    bracket->connect_entry (f);

    FilterBase *ff = new DeleteFilter (q, transparent);
    bracket->add_child (ff);
    f->connect (ff);

    bracket->connect_exit (ff);

  } else {
    parse_filter (ex, q, bracket, true);
  }
}

LayoutQuery::LayoutQuery (const std::string &query)
  : mp_root (0)
{
  std::unique_ptr<FilterBracket> r (new FilterBracket (this));

  tl::Extractor ex (query.c_str ());
  parse_statement (ex, this, r.get (), false);

  if (! ex.at_end ()) {
    ex.error (tl::to_string (tr ("Unexpected text")));
  }

  r->optimize ();
  mp_root = r.release ();
}

LayoutQuery::~LayoutQuery ()
{
  if (mp_root) {
    delete mp_root;
  }
  mp_root = 0;
}

void
LayoutQuery::dump () const
{
  mp_root->dump (0);
}

void
LayoutQuery::execute (db::Layout &layout, tl::Eval *context)
{
  LayoutQueryIterator iq (*this, &layout, context);
  while (! iq.at_end ()) {
    ++iq;
  }
}

unsigned int 
LayoutQuery::register_property (const std::string &name, LayoutQueryPropertyType type)
{
  std::map <std::string, unsigned int>::const_iterator p = m_property_ids_by_name.find (name);
  if (p != m_property_ids_by_name.end ()) {
    //  TODO: check, if the type is identical
    return p->second;
  } else {
    unsigned int id = properties ();
    m_properties.push_back (PropertyDescriptor (type, id, name));
    m_property_ids_by_name.insert (std::make_pair (name, 0)).first->second = id;
    return id;
  }
}

const std::string & 
LayoutQuery::property_name (unsigned int index) const
{
  tl_assert (index < properties ());
  return m_properties [index].name;
}

LayoutQueryPropertyType  
LayoutQuery::property_type (unsigned int index) const
{
  tl_assert (index < properties ());
  return m_properties [index].type;
}

unsigned int  
LayoutQuery::property_by_name (const std::string &name) const
{
  std::map <std::string, unsigned int>::const_iterator p = m_property_ids_by_name.find (name);
  tl_assert (p != m_property_ids_by_name.end ());
  return p->second;
}

bool  
LayoutQuery::has_property (const std::string &name) const
{
  std::map <std::string, unsigned int>::const_iterator p = m_property_ids_by_name.find (name);
  return p != m_property_ids_by_name.end ();
}

// --------------------------------------------------------------------------------
//  FilterBase implementation

FilterBase::FilterBase (LayoutQuery *q)
  : mp_q (q)
{
  // .. nothing yet ..
}

FilterStateBase *
FilterBase::create_state (const std::vector<FilterStateBase *> &followers, db::Layout *layout, tl::Eval &eval, bool /*single*/) const
{
  FilterStateBase *b = do_create_state (layout, eval);
  b->connect (followers);
  return b;
}

void 
FilterBase::connect (FilterBase *follower)
{
  m_followers.push_back (follower);
}

void 
FilterBase::connect (const std::vector<FilterBase *> &followers)
{
  m_followers.insert (m_followers.end (), followers.begin (), followers.end ());
}

FilterBase *
FilterBase::clone (LayoutQuery *q) const
{
  return new FilterBase (q);
}

FilterStateBase *
FilterBase::do_create_state (db::Layout *layout, tl::Eval &eval) const
{
  return new FilterSingleState (this, layout, eval);
}

unsigned int 
FilterBase::register_property (const std::string &name, LayoutQueryPropertyType type)
{
  return mp_q->register_property (name, type);
}

void
FilterBase::dump (unsigned int l) const
{
  for (unsigned int i = 0; i < l; ++i) {
    std::cout << "  ";
  }
  std::cout << "FilterBase" << std::endl;
}

// --------------------------------------------------------------------------------
//  FilterBracket implementation

FilterBracket::FilterBracket (LayoutQuery *q)
  : FilterBase (q), m_initial (q), m_closure (q), m_loopmin (1), m_loopmax (1)
{
  // .. nothing yet ..
}

FilterBracket::FilterBracket (LayoutQuery *q, unsigned int loopmin, unsigned int loopmax)
  : FilterBase (q), m_initial (q), m_closure (q), m_loopmin (loopmin), m_loopmax (loopmax)
{
  tl_assert (loopmin <= loopmax);
}

FilterBracket::~FilterBracket ()
{
  for (std::vector<FilterBase *>::const_iterator c = m_children.begin (); c != m_children.end (); ++c) {
    delete *c;
  }
  m_children.clear ();
}

FilterStateBase *
FilterBracket::create_state (const std::vector<FilterStateBase *> &followers, db::Layout *layout, tl::Eval &eval, bool single) const
{
  bool greedy = false; // TODO: make variable. In greedy mode (true), the children are reported before the nodes, hence the longest match is reported first. 

  if ((m_loopmin == 1 && m_loopmax == 1) || single) {
    
    if (m_children.empty ()) {

      //  shortcut for the simple case 
      FilterStateBase *b = do_create_state (layout, eval);
      b->connect (followers);
      return b;

    } else {

      //  create a terminal state for the graph inside this bracket
      FilterStateBase *b = new FilterSingleState (this, layout, eval);
      b->connect (followers); 

      //  create the graph inside this bracket
      std::map<const FilterBase *, FilterStateBase *> fmap;
      b = create_state_helper (fmap, &m_initial, b, layout, eval);
      return b;

    }

  } else if (m_loopmax == 0) {

    //  a dummy state for the case of loop count 0
    FilterStateBase *b = new FilterSingleState (this, layout, eval);
    b->connect (followers); 
    return b;

  } else {

    FilterStateBase *closure_state = new FilterSingleState (this, layout, eval);
    closure_state->connect (followers); 

    FilterStateBase *b = 0;

    for (int l = int (m_loopmax == std::numeric_limits<unsigned int>::max () ? m_loopmin : m_loopmax); l >= 0; --l) {

      std::vector<FilterStateBase *> f;

      if (greedy) {
        //  This is greedy mode, non-greedy would be closure first.
        if ((unsigned int) l < m_loopmax) {
          //  successor (or 0 for successor generated on the fly)
          f.push_back (b);
        }
        if ((unsigned int) l >= m_loopmin) {
          //  optional path
          f.push_back (closure_state);
        }
      } else {
        //  This is non-greedy mode
        if ((unsigned int) l >= m_loopmin) {
          //  optional path
          f.push_back (closure_state);
        }
        if ((unsigned int) l < m_loopmax) {
          //  successor (or 0 for successor generated on the fly)
          f.push_back (b);
        }
      }

      if (m_children.empty ()) {

        //  create a terminal state for the graph inside this bracket
        if (l > 0) {
          b = do_create_state (layout, eval);
        } else {
          b = new FilterSingleState (this, layout, eval);
        }
        b->connect (f); 

      } else {

        //  create a terminal state for the graph inside this bracket
        b = new FilterSingleState (this, layout, eval);
        b->connect (f); 

        if (l > 0) {
          //  create the graph inside this bracket
          std::map<const FilterBase *, FilterStateBase *> fmap;
          b = create_state_helper (fmap, &m_initial, b, layout, eval);
        }

      }

    }

    return b;

  }

}

const std::vector<FilterBase *> &
FilterBracket::children () const
{
  return m_children;
}

void 
FilterBracket::add_child (FilterBase *follower)
{
  //  in case there are already connections move them to the closure
  if (m_children.empty ()) {
    followers ().swap (m_closure.followers ());
  }

  m_children.push_back (follower);
}

void 
FilterBracket::connect_entry (FilterBase *child)
{
  m_initial.connect (child);
}

void 
FilterBracket::connect_exit (FilterBase *child)
{
  child->connect (&m_closure);
}

FilterBase *
FilterBracket::clone (LayoutQuery *q) const
{
  FilterBracket *b = new FilterBracket (q, m_loopmin, m_loopmax);

  std::map<const FilterBase *, FilterBase *> fmap;

  for (std::vector<FilterBase *>::const_iterator c = m_children.begin (); c != m_children.end (); ++c) {
    FilterBase *cc = (*c)->clone (q);
    fmap.insert (std::make_pair (*c, cc));
    b->add_child (cc);
  }

  for (std::vector<FilterBase *>::const_iterator o = m_initial.followers ().begin (); o != m_initial.followers ().end (); ++o) {
    std::map<const FilterBase *, FilterBase *>::const_iterator f = fmap.find (*o);
    if (f != fmap.end ()) {
      b->connect_entry (f->second);
    }
  }

  for (std::vector<FilterBase *>::const_iterator c = m_children.begin (); c != m_children.end (); ++c) {
    std::map<const FilterBase *, FilterBase *>::const_iterator fc = fmap.find (*c);
    for (std::vector<FilterBase *>::const_iterator o = (*c)->followers ().begin (); o != (*c)->followers ().end (); ++o) {
      if (*o == &m_closure) {
        b->connect_exit (fc->second);
      } else {
        std::map<const FilterBase *, FilterBase *>::const_iterator f = fmap.find (*o);
        tl_assert (f != fmap.end ());
        fc->second->connect (f->second);
      }
    }
  }

  return b;
}

FilterStateBase *
FilterBracket::create_state_helper (std::map<const FilterBase *, FilterStateBase *> &fmap, const FilterBase *child, FilterStateBase *closure_state, db::Layout *layout, tl::Eval &eval) const
{
  std::vector<FilterStateBase *> followers;
  followers.reserve (child->followers ().size ());

  for (std::vector<FilterBase *>::const_iterator o = child->followers ().begin (); o != child->followers ().end (); ++o) {
    if (*o == &m_closure) {
      followers.push_back (closure_state);
    } else {
      std::map<const FilterBase *, FilterStateBase *>::const_iterator f = fmap.find (*o);
      if (f != fmap.end ()) {
        followers.push_back (f->second);
      } else {
        FilterStateBase *fs = create_state_helper (fmap, *o, closure_state, layout, eval);
        fmap.insert (std::make_pair (*o, fs));
        followers.push_back (fs);
      }
    }
  }

  return child->create_state (followers, layout, eval, false);
}

void
FilterBracket::dump (unsigned int l) const
{
  if (! m_children.empty ()) {

    for (unsigned int i = 0; i < l; ++i) {
      std::cout << "  ";
    }
    std::cout << "FilterBracket (" << m_loopmin << ".." << m_loopmax << ") {" << std::endl;

    std::deque<const FilterBase *> todo;
    std::vector<const FilterBase *> filters;
    std::map<const FilterBase *, int> ids;

    int id = 1;

    todo.push_back (&m_initial);
    filters.push_back (&m_initial);
    ids.insert (std::make_pair (&m_initial, id++));

    while (! todo.empty ()) {

      const FilterBase *f = todo.front ();
      todo.pop_front ();

      for (std::vector<FilterBase *>::const_iterator c = f->followers ().begin (); c != f->followers ().end (); ++c) {
        if (ids.find (*c) == ids.end ()) {
          ids.insert (std::make_pair (*c, id++));
          filters.push_back (*c);
          todo.push_back (*c);
        }
      }

    }

    for (std::vector<const FilterBase *>::const_iterator f = filters.begin (); f != filters.end (); ++f) {
      for (unsigned int i = 0; i < l + 1; ++i) {
        std::cout << "  ";
      }
      std::cout << "[" << ids [*f] << "]" << std::endl;
      (*f)->dump (l + 1);
      for (std::vector<FilterBase *>::const_iterator c = (*f)->followers ().begin (); c != (*f)->followers ().end (); ++c) {
        for (unsigned int i = 0; i < l + 1; ++i) {
          std::cout << "  ";
        }
        std::cout << "-> [" << ids [*c] << "]" << std::endl;
      }
    }

    for (unsigned int i = 0; i < l; ++i) {
      std::cout << "  ";
    }
    std::cout << "}" << std::endl;

  } else {

    for (unsigned int i = 0; i < l; ++i) {
      std::cout << "  ";
    }
    std::cout << "FilterBracket (" << m_loopmin << ".." << m_loopmax << ")" << std::endl;

  }
}

void
FilterBracket::optimize ()
{
  // TODO: implement
}

// --------------------------------------------------------------------------------
//  FilterStateObjectives implementation

FilterStateObjectives::FilterStateObjectives ()
  : m_wants_all_cells (false)
{
  //  .. nothing yet ..
}

FilterStateObjectives FilterStateObjectives::everything ()
{
  FilterStateObjectives all;
  all.set_wants_all_cells (true);
  return all;
}

FilterStateObjectives &
FilterStateObjectives::operator+= (const FilterStateObjectives &other)
{
  if (! m_wants_all_cells) {
    m_wants_all_cells = other.m_wants_all_cells;
    if (! m_wants_all_cells) {
      m_wants_cells.insert (other.m_wants_cells.begin (), other.m_wants_cells.end ());
    }
  }

  if (m_wants_all_cells) {
    m_wants_cells.clear ();
  }

  return *this;
}

void
FilterStateObjectives::set_wants_all_cells (bool f)
{
  m_wants_cells.clear ();
  m_wants_all_cells = f;
}

void
FilterStateObjectives::request_cell (db::cell_index_type ci)
{
  if (! m_wants_all_cells) {
    m_wants_cells.insert (ci);
  }
}

bool
FilterStateObjectives::wants_cell (db::cell_index_type ci) const
{
  return m_wants_all_cells || m_wants_cells.find (ci) != m_wants_cells.end ();
}

// --------------------------------------------------------------------------------
//  FilterStateBase implementation

FilterStateBase::FilterStateBase (const FilterBase *filter, db::Layout *layout, tl::Eval &eval)
  : mp_previous (0), mp_filter (filter), mp_layout (layout), m_follower (0), mp_eval (&eval)
{
}

void
FilterStateBase::init (bool recursive)
{
  if (m_followers.empty ()) {

    m_objectives = FilterStateObjectives::everything ();

  } else {

    for (std::vector<FilterStateBase *>::const_iterator f = m_followers.begin (); f != m_followers.end (); ++f) {
      if (*f) {
        if (recursive) {
          (*f)->init ();
        }
        m_objectives += (*f)->objectives ();
      }
    }

  }

  do_init ();
}

void
FilterStateBase::do_init ()
{
  //  .. nothing yet ..
}

void
FilterStateBase::dump () const
{
  std::cout << "[";
  for (size_t i = 0; i < m_followers.size (); ++i) {
    if (i > 0) {
      std::cout << ",";
    }
    std::cout << (i == m_follower ? "+" : "");
    if (! m_followers[i]) {
      std::cout << "0";
    } else {
      m_followers[i]->dump ();
    }
  }
  std::cout << "]";
}

FilterStateBase *
FilterStateBase::child () const
{
  if (m_followers.empty ()) {
    return 0;
  } else {

    FilterStateBase *b = m_followers [m_follower];
    if (! b && mp_filter && mp_layout) {
      //  dynamically create a new recursive state execution graph snippet if required
      b = mp_filter->create_state (m_followers, mp_layout, *mp_eval, true);
      b->init (false);
      m_followers [m_follower] = b;
    }

    return b;

  }
}

void 
FilterStateBase::connect (FilterStateBase *follower)
{
  m_followers.push_back (follower);
}

void 
FilterStateBase::connect (const std::vector<FilterStateBase *> &followers)
{
  m_followers.insert (m_followers.end (), followers.begin (), followers.end ());
}

void 
FilterStateBase::proceed (bool skip) 
{
  if (m_followers.empty ()) {
    next (skip);
  } else {
    ++m_follower;
    if (m_followers.size () == m_follower) {
      m_follower = 0;
      next (skip);
    }
  }
}

}

