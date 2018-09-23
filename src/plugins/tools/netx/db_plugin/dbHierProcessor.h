
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

class DB_PLUGIN_PUBLIC LocalOperation
{
public:
  enum on_empty_intruder_mode {
    Ignore = 0, Copy, Drop
  };

  LocalOperation () { }
  virtual ~LocalOperation () { }

  virtual void compute_local (db::Layout *layout, const std::map<db::PolygonRef, std::vector<db::PolygonRef> > &interactions, std::set<db::PolygonRef> &result) const = 0;
  virtual on_empty_intruder_mode on_empty_intruder_hint () const = 0;
};

class DB_PLUGIN_PUBLIC BoolAndOrNotLocalOperation
  : public LocalOperation
{
public:
  BoolAndOrNotLocalOperation (bool is_and);

  virtual void compute_local (db::Layout *layout, const std::map<db::PolygonRef, std::vector<db::PolygonRef> > &interactions, std::set<db::PolygonRef> &result) const;
  virtual on_empty_intruder_mode on_empty_intruder_hint () const;

private:
  bool m_is_and;
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

  db::LocalProcessorCellContext *find_context (const key_type &intruders);
  db::LocalProcessorCellContext *create (const key_type &intruders);
  void compute_results (db::Cell *cell, LocalProcessor *proc);

  iterator begin () const
  {
    return m_contexts.begin ();
  }

  iterator end () const
  {
    return m_contexts.end ();
  }

private:
  std::map<key_type, db::LocalProcessorCellContext> m_contexts;
};

class DB_PLUGIN_PUBLIC LocalProcessor
{
public:
  typedef std::map<db::Cell *, LocalProcessorCellContexts> contexts_per_cell_type;

  LocalProcessor (db::Layout *layout, db::Cell *top, LocalOperation *op, unsigned int subject_layer, unsigned int intruder_layer, unsigned int output_layer);
  void run ();
  void compute_contexts ();
  void compute_results ();

  const contexts_per_cell_type &contexts_per_cell () const
  {
    return m_contexts_per_cell;
  }

private:
  friend class LocalProcessorCellContexts;

  db::Layout *mp_layout;
  db::Cell *mp_top;
  unsigned int m_subject_layer, m_intruder_layer, m_output_layer;
  contexts_per_cell_type m_contexts_per_cell;
  LocalOperation *mp_op;

  void compute_contexts (db::LocalProcessorCellContext *parent_context, db::Cell *parent, db::Cell *cell, const db::ICplxTrans &cell_inst, const std::pair<std::set<CellInstArray>, std::set<PolygonRef> > &intruders);
  void push_results (db::Cell *cell, const std::set<db::PolygonRef> &result);
  void compute_local_cell (db::Cell *cell, const std::pair<std::set<CellInstArray>, std::set<db::PolygonRef> > &intruders, std::set<db::PolygonRef> &result) const;
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

