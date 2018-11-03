
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
#include "dbLocalOperation.h"
#include "tlThreadedWorkers.h"

#include <map>
#include <set>
#include <vector>
#include <unordered_map>
#include <unordered_set>

//  @@@ should go into dbHash.h
#include "dbHash.h"

namespace std
{
  template <class C>
  struct hash <db::disp_trans<C> >
  {
    size_t operator() (const db::disp_trans<C> &t) const
    {
      return hfunc (t.disp ());
    }
  };

  template <class Polygon, class Trans>
  struct hash<db::polygon_ref<Polygon, Trans> >
  {
    size_t operator() (const db::polygon_ref<Polygon, Trans> &o) const
    {
      return hfunc (size_t (o.ptr ()), std::hash<Trans> () (o.trans ()));
    }
  };

  template <class T>
  struct hash<std::unordered_set<T> >
  {
    size_t operator() (const std::unordered_set<T> &o) const
    {
      size_t hf = 0;
      for (typename std::unordered_set<T>::const_iterator i = o.begin (); i != o.end (); ++i) {
        hf = hfunc (hf, std::hash <T> () (*i));
      }
      return hf;
    }
  };
}


namespace db
{

class LocalProcessor;
class LocalProcessorCellContext;
class LocalProcessorContexts;

//  TODO: move this somewhere else?
class DB_PLUGIN_PUBLIC ShapeInteractions
{
public:
  typedef std::unordered_map<unsigned int, std::vector<unsigned int> > container;
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
  std::unordered_map<unsigned int, std::vector<unsigned int> > m_interactions;
  std::unordered_map<unsigned int, db::PolygonRef> m_shapes;
  unsigned int m_id;
};

//  TODO: should be hidden (private data?)
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

//  TODO: should be hidden (private data?)
class DB_PLUGIN_PUBLIC LocalProcessorCellContext
{
public:
  typedef std::pair<const db::Cell *, db::ICplxTrans> parent_inst_type;

  LocalProcessorCellContext ();

  void add (db::LocalProcessorCellContext *parent_context, db::Cell *parent, const db::ICplxTrans &cell_inst);
  void propagate (const std::unordered_set<db::PolygonRef> &res);

  std::unordered_set<db::PolygonRef> &propagated ()
  {
    return m_propagated;
  }

  const std::unordered_set<db::PolygonRef> &propagated () const
  {
    return m_propagated;
  }

  size_t size () const
  {
    return m_drops.size ();
  }

  tl::Mutex &lock ()
  {
    return m_lock;
  }

private:
  std::unordered_set<db::PolygonRef> m_propagated;
  std::vector<LocalProcessorCellDrop> m_drops;
  tl::Mutex m_lock;
};

class DB_PLUGIN_PUBLIC LocalProcessorCellContexts
{
public:
  typedef std::pair<std::unordered_set<CellInstArray>, std::unordered_set<db::PolygonRef> > key_type;
  typedef std::unordered_map<key_type, db::LocalProcessorCellContext> map_type;
  typedef map_type::const_iterator iterator;

  LocalProcessorCellContexts ();
  LocalProcessorCellContexts (const db::Cell *intruder_cell);

  db::LocalProcessorCellContext *find_context (const key_type &intruders);
  db::LocalProcessorCellContext *create (const key_type &intruders);
  void compute_results (const LocalProcessorContexts &contexts, db::Cell *cell, const LocalOperation *op, unsigned int output_layer, const LocalProcessor *proc);

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
  std::unordered_map<key_type, db::LocalProcessorCellContext> m_contexts;
};

class DB_PLUGIN_PUBLIC LocalProcessorContexts
{
public:
  typedef std::unordered_map<db::Cell *, LocalProcessorCellContexts> contexts_per_cell_type;
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

  tl::Mutex &lock () const
  {
    return m_lock;
  }

private:
  contexts_per_cell_type m_contexts_per_cell;
  unsigned int m_subject_layer, m_intruder_layer;
  mutable tl::Mutex m_lock;
};

class DB_PLUGIN_PUBLIC LocalProcessorContextComputationTask
  : public tl::Task
{
public:
  LocalProcessorContextComputationTask (const LocalProcessor *proc, LocalProcessorContexts &contexts, db::LocalProcessorCellContext *parent_context, db::Cell *subject_parent, db::Cell *subject_cell, const db::ICplxTrans &subject_cell_inst, const db::Cell *intruder_cell, std::pair<std::unordered_set<CellInstArray>, std::unordered_set<PolygonRef> > &intruders, db::Coord dist);
  void perform ();

private:
  const LocalProcessor *mp_proc;
  LocalProcessorContexts *mp_contexts;
  db::LocalProcessorCellContext *mp_parent_context;
  db::Cell *mp_subject_parent;
  db::Cell *mp_subject_cell;
  db::ICplxTrans m_subject_cell_inst;
  const db::Cell *mp_intruder_cell;
  std::pair<std::unordered_set<CellInstArray>, std::unordered_set<PolygonRef> > m_intruders;
  db::Coord m_dist;
};

class DB_PLUGIN_PUBLIC LocalProcessorContextComputationWorker
  : public tl::Worker
{
public:
  LocalProcessorContextComputationWorker ()
    : tl::Worker ()
  {
    //  .. nothing yet ..
  }

  void perform_task (tl::Task *task)
  {
    static_cast<LocalProcessorContextComputationTask *> (task)->perform ();
  }
};

class DB_PLUGIN_PUBLIC LocalProcessorResultComputationTask
  : public tl::Task
{
public:
  LocalProcessorResultComputationTask (const LocalProcessor *proc, LocalProcessorContexts &contexts, db::Cell *cell, LocalProcessorCellContexts *cell_contexts, const LocalOperation *op, unsigned int output_layer);
  void perform ();

private:
  const LocalProcessor *mp_proc;
  LocalProcessorContexts *mp_contexts;
  db::Cell *mp_cell;
  LocalProcessorCellContexts *mp_cell_contexts;
  const LocalOperation *mp_op;
  unsigned int m_output_layer;
};

class DB_PLUGIN_PUBLIC LocalProcessorResultComputationWorker
  : public tl::Worker
{
public:
  LocalProcessorResultComputationWorker ()
    : tl::Worker ()
  {
    //  .. nothing yet ..
  }

  void perform_task (tl::Task *task)
  {
    static_cast<LocalProcessorResultComputationTask *> (task)->perform ();
  }
};

class DB_PLUGIN_PUBLIC LocalProcessor
{
public:
  LocalProcessor (db::Layout *layout, db::Cell *top);
  LocalProcessor (db::Layout *subject_layout, db::Cell *subject_top, const db::Layout *intruder_layout, const db::Cell *intruder_cell);
  void run (LocalOperation *op, unsigned int subject_layer, unsigned int intruder_layer, unsigned int output_layer);
  void compute_contexts (LocalProcessorContexts &contexts, const LocalOperation *op, unsigned int subject_layer, unsigned int intruder_layer) const;
  void compute_results (LocalProcessorContexts &contexts, const LocalOperation *op, unsigned int output_layer) const;

  void set_description (const std::string &d)
  {
    m_description = d;
  }

  void set_threads (unsigned int nthreads)
  {
    m_nthreads = nthreads;
  }

private:
  friend class LocalProcessorCellContexts;
  friend class LocalProcessorContextComputationTask;

  db::Layout *mp_subject_layout;
  const db::Layout *mp_intruder_layout;
  db::Cell *mp_subject_top;
  const db::Cell *mp_intruder_top;
  std::string m_description;
  unsigned int m_nthreads;
  mutable std::auto_ptr<tl::Job<LocalProcessorContextComputationWorker> > mp_cc_job;

  std::string description (const LocalOperation *op) const;
  void compute_contexts (db::LocalProcessorContexts &contexts, db::LocalProcessorCellContext *parent_context, db::Cell *subject_parent, db::Cell *subject_cell, const db::ICplxTrans &subject_cell_inst, const db::Cell *intruder_cell, const std::pair<std::unordered_set<CellInstArray>, std::unordered_set<PolygonRef> > &intruders, db::Coord dist) const;
  void do_compute_contexts (db::LocalProcessorCellContext *cell_context, const db::LocalProcessorContexts &contexts, db::LocalProcessorCellContext *parent_context, db::Cell *subject_parent, db::Cell *subject_cell, const db::ICplxTrans &subject_cell_inst, const db::Cell *intruder_cell, const std::pair<std::unordered_set<CellInstArray>, std::unordered_set<PolygonRef> > &intruders, db::Coord dist) const;
  void issue_compute_contexts (db::LocalProcessorContexts &contexts, db::LocalProcessorCellContext *parent_context, db::Cell *subject_parent, db::Cell *subject_cell, const db::ICplxTrans &subject_cell_inst, const db::Cell *intruder_cell, std::pair<std::unordered_set<CellInstArray>, std::unordered_set<PolygonRef> > &intruders, db::Coord dist) const;
  void push_results (db::Cell *cell, unsigned int output_layer, const std::unordered_set<db::PolygonRef> &result) const;
  void compute_local_cell (const db::LocalProcessorContexts &contexts, db::Cell *subject_cell, const db::Cell *intruder_cell, const LocalOperation *op, const std::pair<std::unordered_set<CellInstArray>, std::unordered_set<db::PolygonRef> > &intruders, std::unordered_set<db::PolygonRef> &result) const;
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

