
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



#ifndef HDR_dbHierProcessor
#define HDR_dbHierProcessor

#include "dbLayout.h"
#include "dbPluginCommon.h"

#include <map>
#include <set>
#include <vector>

namespace db
{

class LocalProcessor;
class LocalProcessorCellContext;
class LocalProcessorContexts;

//  @@@ TODO: move this somewhere else?
class DB_PLUGIN_PUBLIC ShapeInteractions
{
public:
  typedef std::map<unsigned int, std::vector<unsigned int> > container;
  typedef container::const_iterator iterator;
  typedef container::value_type::second_type::const_iterator iterator2;

  ShapeInteractions ();

  iterator begin () const
  {
    return m_interactions.begin ();
  }

  iterator end () const
  {
    return m_interactions.end ();
  }

  bool has_shape_id (unsigned int id) const;
  void add_shape (unsigned int id, const db::PolygonRef &shape);
  void add_subject (unsigned int id, const db::PolygonRef &shape);
  void add_interaction (unsigned int subject_id, unsigned int intruder_id);
  const std::vector<unsigned int> &intruders_for (unsigned int subject_id) const;
  const db::PolygonRef &shape (unsigned int id) const;

  unsigned int next_id ()
  {
    return ++m_id;
  }

private:
  std::map<unsigned int, std::vector<unsigned int> > m_interactions;
  std::map<unsigned int, db::PolygonRef> m_shapes;
  unsigned int m_id;
};

class DB_PLUGIN_PUBLIC LocalOperation
{
public:
  enum on_empty_intruder_mode {
    Ignore = 0, Copy, Drop
  };

  LocalOperation () { }
  virtual ~LocalOperation () { }

  virtual void compute_local (db::Layout *layout, const ShapeInteractions &interactions, std::set<db::PolygonRef> &result) const = 0;
  virtual on_empty_intruder_mode on_empty_intruder_hint () const = 0;
  virtual std::string description () const = 0;
  virtual db::Coord dist () const { return 0; }
};

class DB_PLUGIN_PUBLIC BoolAndOrNotLocalOperation
  : public LocalOperation
{
public:
  BoolAndOrNotLocalOperation (bool is_and);

  virtual void compute_local (db::Layout *layout, const ShapeInteractions &interactions, std::set<db::PolygonRef> &result) const;
  virtual on_empty_intruder_mode on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  bool m_is_and;
};

class DB_PLUGIN_PUBLIC SelfOverlapMergeLocalOperation
  : public LocalOperation
{
public:
  SelfOverlapMergeLocalOperation (unsigned int wrap_count);

  virtual void compute_local (db::Layout *layout, const ShapeInteractions &interactions, std::set<db::PolygonRef> &result) const;
  virtual on_empty_intruder_mode on_empty_intruder_hint () const;
  virtual std::string description () const;

private:
  unsigned int m_wrap_count;
};

//  @@@ TODO: should be hidden (private data?)
struct DB_PLUGIN_PUBLIC LocalProcessorCellDrop
{
  LocalProcessorCellDrop (db::LocalProcessorCellContext *_parent_context, db::Cell *_parent, const db::ICplxTrans &_cell_inst)
    : parent_context (_parent_context), parent (_parent), cell_inst (_cell_inst)
  {
    //  .. nothing yet ..
  }

  db::LocalProcessorCellContext *parent_context;
  db::Cell *parent;
  db::ICplxTrans cell_inst;
};

//  @@@ TODO: should be hidden (private data?)
class DB_PLUGIN_PUBLIC LocalProcessorCellContext
{
public:
  typedef std::pair<const db::Cell *, db::ICplxTrans> parent_inst_type;

  LocalProcessorCellContext ();

  void add (db::LocalProcessorCellContext *parent_context, db::Cell *parent, const db::ICplxTrans &cell_inst);
  void propagate (const std::set<db::PolygonRef> &res);

  std::set<db::PolygonRef> &propagated ()
  {
    return m_propagated;
  }

  size_t size () const
  {
    return m_drops.size ();
  }

private:
  std::set<db::PolygonRef> m_propagated;
  std::vector<LocalProcessorCellDrop> m_drops;
};

class DB_PLUGIN_PUBLIC LocalProcessorCellContexts
{
public:
  typedef std::pair<std::set<CellInstArray>, std::set<db::PolygonRef> > key_type;
  typedef std::map<key_type, db::LocalProcessorCellContext> map_type;
  typedef map_type::const_iterator iterator;

  LocalProcessorCellContexts ();
  LocalProcessorCellContexts (const db::Cell *intruder_cell);

  db::LocalProcessorCellContext *find_context (const key_type &intruders);
  db::LocalProcessorCellContext *create (const key_type &intruders);
  void compute_results (LocalProcessorContexts &contexts, db::Cell *cell, const LocalOperation *op, unsigned int output_layer, LocalProcessor *proc);

  iterator begin () const
  {
    return m_contexts.begin ();
  }

  iterator end () const
  {
    return m_contexts.end ();
  }

private:
  const db::Cell *mp_intruder_cell;
  std::map<key_type, db::LocalProcessorCellContext> m_contexts;
};

class DB_PLUGIN_PUBLIC LocalProcessorContexts
{
public:
  typedef std::map<db::Cell *, LocalProcessorCellContexts> contexts_per_cell_type;
  typedef contexts_per_cell_type::iterator iterator;

  LocalProcessorContexts ()
    : m_subject_layer (0), m_intruder_layer (0)
  {
    //  .. nothing yet ..
  }

  void clear ()
  {
    m_contexts_per_cell.clear ();
  }

  LocalProcessorCellContexts &contexts_per_cell (db::Cell *subject_cell, const db::Cell *intruder_cell)
  {
    contexts_per_cell_type::iterator ctx = m_contexts_per_cell.find (subject_cell);
    if (ctx == m_contexts_per_cell.end ()) {
      ctx = m_contexts_per_cell.insert (std::make_pair (subject_cell, LocalProcessorCellContexts (intruder_cell))).first;
    }
    return ctx->second;
  }

  contexts_per_cell_type &context_map ()
  {
    return m_contexts_per_cell;
  }

  iterator begin ()
  {
    return m_contexts_per_cell.begin ();
  }

  iterator end ()
  {
    return m_contexts_per_cell.end ();
  }

  void set_subject_layer (unsigned int l)
  {
    m_subject_layer = l;
  }

  unsigned int subject_layer () const
  {
    return m_subject_layer;
  }

  void set_intruder_layer (unsigned int l)
  {
    m_intruder_layer = l;
  }

  unsigned int intruder_layer () const
  {
    return m_intruder_layer;
  }

  void set_description (const std::string &desc)
  {
    m_description = desc;
  }

  const std::string &description () const
  {
    return m_description;
  }

private:
  contexts_per_cell_type m_contexts_per_cell;
  unsigned int m_subject_layer, m_intruder_layer;
  std::string m_description;
};

class DB_PLUGIN_PUBLIC LocalProcessor
{
public:
  LocalProcessor (db::Layout *layout, db::Cell *top);
  LocalProcessor (db::Layout *subject_layout, db::Cell *subject_top, const db::Layout *intruder_layout, const db::Cell *intruder_cell);
  void run (LocalOperation *op, unsigned int subject_layer, unsigned int intruder_layer, unsigned int output_layer);
  void compute_contexts (LocalProcessorContexts &contexts, const LocalOperation *op, unsigned int subject_layer, unsigned int intruder_layer);
  void compute_results (LocalProcessorContexts &contexts, const LocalOperation *op, unsigned int output_layer);

  void set_description (const std::string &d)
  {
    m_description = d;
  }

  const std::string &description () const
  {
    return m_description;
  }

private:
  friend class LocalProcessorCellContexts;

  db::Layout *mp_subject_layout;
  const db::Layout *mp_intruder_layout;
  db::Cell *mp_subject_top;
  const db::Cell *mp_intruder_top;
  std::string m_description;

  void compute_contexts (LocalProcessorContexts &contexts, db::LocalProcessorCellContext *parent_context, db::Cell *subject_parent, db::Cell *subject_cell, const db::ICplxTrans &subject_cell_inst, const db::Cell *intruder_cell, const std::pair<std::set<CellInstArray>, std::set<PolygonRef> > &intruders, db::Coord dist);
  void push_results (db::Cell *cell, unsigned int output_layer, const std::set<db::PolygonRef> &result) const;
  void compute_local_cell (LocalProcessorContexts &contexts, db::Cell *subject_cell, const db::Cell *intruder_cell, const LocalOperation *op, const std::pair<std::set<CellInstArray>, std::set<db::PolygonRef> > &intruders, std::set<db::PolygonRef> &result);
};

}

namespace tl
{

template <>
struct type_traits<db::LocalProcessor> : public tl::type_traits<void>
{
  //  mark "LocalProcessor" as not having a default ctor and no copy ctor
  typedef tl::false_tag has_default_constructor;
  typedef tl::false_tag has_copy_constructor;
};

}

#endif

