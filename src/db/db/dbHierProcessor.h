
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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

#include "dbCommon.h"

#include "dbLayout.h"
#include "dbLocalOperation.h"
#include "tlThreadedWorkers.h"
#include "tlProgress.h"

#include <map>
#include <set>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include "dbHash.h"

namespace db
{

template <class TS, class TI, class TR> class local_processor;
template <class TS, class TI, class TR> class local_processor_cell_context;
template <class TS, class TI, class TR> class local_processor_contexts;

//  TODO: move this somewhere else?
template <class TS, class TI>
class DB_PUBLIC shape_interactions
{
public:
  typedef std::unordered_map<unsigned int, std::vector<unsigned int> > container;
  typedef container::const_iterator iterator;
  typedef container::value_type::second_type::const_iterator iterator2;
  typedef typename std::unordered_map<unsigned int, TS>::const_iterator subject_iterator;
  typedef typename std::unordered_map<unsigned int, TI>::const_iterator intruder_iterator;

  shape_interactions ();

  iterator begin () const
  {
    return m_interactions.begin ();
  }

  iterator end () const
  {
    return m_interactions.end ();
  }

  subject_iterator begin_subjects () const
  {
    return m_subject_shapes.begin ();
  }

  subject_iterator end_subjects () const
  {
    return m_subject_shapes.end ();
  }

  intruder_iterator begin_intruders () const
  {
    return m_intruder_shapes.begin ();
  }

  intruder_iterator end_intruders () const
  {
    return m_intruder_shapes.end ();
  }

  bool has_intruder_shape_id (unsigned int id) const;
  bool has_subject_shape_id (unsigned int id) const;
  void add_intruder_shape (unsigned int id, const TI &shape);
  void add_subject_shape (unsigned int id, const TS &shape);
  void add_subject (unsigned int id, const TS &shape);
  void add_interaction (unsigned int subject_id, unsigned int intruder_id);
  const std::vector<unsigned int> &intruders_for (unsigned int subject_id) const;
  const TS &subject_shape (unsigned int id) const;
  const TI &intruder_shape (unsigned int id) const;

  unsigned int next_id ()
  {
    return ++m_id;
  }

private:
  std::unordered_map<unsigned int, std::vector<unsigned int> > m_interactions;
  std::unordered_map<unsigned int, TS> m_subject_shapes;
  std::unordered_map<unsigned int, TI> m_intruder_shapes;
  unsigned int m_id;
};

//  TODO: should be hidden (private data?)
template <class TS, class TI, class TR>
struct DB_PUBLIC local_processor_cell_drop
{
  local_processor_cell_drop (db::local_processor_cell_context<TS, TI, TR> *_parent_context, db::Cell *_parent, const db::ICplxTrans &_cell_inst)
    : parent_context (_parent_context), parent (_parent), cell_inst (_cell_inst)
  {
    //  .. nothing yet ..
  }

  db::local_processor_cell_context<TS, TI, TR> *parent_context;
  db::Cell *parent;
  db::ICplxTrans cell_inst;
};

//  TODO: should be hidden (private data?)
template <class TS, class TI, class TR>
class DB_PUBLIC local_processor_cell_context
{
public:
  typedef std::pair<const db::Cell *, db::ICplxTrans> parent_inst_type;
  typedef typename std::vector<local_processor_cell_drop<TS, TI, TR> >::const_iterator drop_iterator;

  local_processor_cell_context ();
  local_processor_cell_context (const local_processor_cell_context &other);

  void add (db::local_processor_cell_context<TS, TI, TR> *parent_context, db::Cell *parent, const db::ICplxTrans &cell_inst);
  void propagate (const std::unordered_set<TR> &res);

  std::unordered_set<TR> &propagated ()
  {
    return m_propagated;
  }

  const std::unordered_set<TR> &propagated () const
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

  //  used for debugging purposes only
  drop_iterator begin_drops () const
  {
    return m_drops.begin ();
  }

  //  used for debugging purposes only
  drop_iterator end_drops () const
  {
    return m_drops.end ();
  }

private:
  std::unordered_set<TR> m_propagated;
  std::vector<local_processor_cell_drop<TS, TI, TR> > m_drops;
  tl::Mutex m_lock;
};

template <class TS, class TI, class TR>
class DB_PUBLIC local_processor_cell_contexts
{
public:
  typedef std::pair<std::set<CellInstArray>, std::set<TI> > context_key_type;
  typedef std::unordered_map<context_key_type, db::local_processor_cell_context<TS, TI, TR> > context_map_type;
  typedef typename context_map_type::const_iterator iterator;

  local_processor_cell_contexts ();
  local_processor_cell_contexts (const db::Cell *intruder_cell);

  db::local_processor_cell_context<TS, TI, TR> *find_context (const context_key_type &intruders);
  db::local_processor_cell_context<TS, TI, TR> *create (const context_key_type &intruders);
  void compute_results (const local_processor_contexts<TS, TI, TR> &contexts, db::Cell *cell, const local_operation<TS, TI, TR> *op, unsigned int output_layer, const local_processor<TS, TI, TR> *proc);

  size_t size () const
  {
    return m_contexts.size ();
  }

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
  std::unordered_map<context_key_type, db::local_processor_cell_context<TS, TI, TR> > m_contexts;
};

template <class TS, class TI, class TR>
class DB_PUBLIC local_processor_contexts
{
public:
  typedef std::unordered_map<db::Cell *, local_processor_cell_contexts<TS, TI, TR> > contexts_per_cell_type;
  typedef typename contexts_per_cell_type::iterator iterator;

  local_processor_contexts ()
    : m_subject_layer (0), m_intruder_layer (0)
  {
    //  .. nothing yet ..
  }

  local_processor_contexts (const local_processor_contexts &other)
    : m_contexts_per_cell (other.m_contexts_per_cell), m_subject_layer (other.m_subject_layer), m_intruder_layer (other.m_intruder_layer)
  {
    //  .. nothing yet ..
  }

  void clear ()
  {
    m_contexts_per_cell.clear ();
  }

  local_processor_cell_contexts<TS, TI, TR> &contexts_per_cell (db::Cell *subject_cell, const db::Cell *intruder_cell)
  {
    typename contexts_per_cell_type::iterator ctx = m_contexts_per_cell.find (subject_cell);
    if (ctx == m_contexts_per_cell.end ()) {
      ctx = m_contexts_per_cell.insert (std::make_pair (subject_cell, local_processor_cell_contexts<TS, TI, TR> (intruder_cell))).first;
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

template <class TS, class TI, class TR>
class DB_PUBLIC local_processor_context_computation_task
  : public tl::Task
{
public:
  local_processor_context_computation_task (const local_processor<TS, TI, TR> *proc, local_processor_contexts<TS, TI, TR> &contexts, db::local_processor_cell_context<TS, TI, TR> *parent_context, db::Cell *subject_parent, db::Cell *subject_cell, const db::ICplxTrans &subject_cell_inst, const db::Cell *intruder_cell, typename local_processor_cell_contexts<TS, TI, TR>::context_key_type &intruders, db::Coord dist);
  void perform ();

private:
  const local_processor<TS, TI, TR> *mp_proc;
  local_processor_contexts<TS, TI, TR> *mp_contexts;
  db::local_processor_cell_context<TS, TI, TR> *mp_parent_context;
  db::Cell *mp_subject_parent;
  db::Cell *mp_subject_cell;
  db::ICplxTrans m_subject_cell_inst;
  const db::Cell *mp_intruder_cell;
  typename local_processor_cell_contexts<TS, TI, TR>::context_key_type m_intruders;
  db::Coord m_dist;
};

template <class TS, class TI, class TR>
class DB_PUBLIC local_processor_context_computation_worker
  : public tl::Worker
{
public:
  local_processor_context_computation_worker ()
    : tl::Worker ()
  {
    //  .. nothing yet ..
  }

  void perform_task (tl::Task *task)
  {
    static_cast<local_processor_context_computation_task<TS, TI, TR> *> (task)->perform ();
  }
};

template <class TS, class TI, class TR>
class DB_PUBLIC local_processor_result_computation_task
  : public tl::Task
{
public:
  local_processor_result_computation_task (const local_processor<TS, TI, TR> *proc, local_processor_contexts<TS, TI, TR> &contexts, db::Cell *cell, local_processor_cell_contexts<TS, TI, TR> *cell_contexts, const local_operation<TS, TI, TR> *op, unsigned int output_layer);
  void perform ();

private:
  const local_processor<TS, TI, TR> *mp_proc;
  local_processor_contexts<TS, TI, TR> *mp_contexts;
  db::Cell *mp_cell;
  local_processor_cell_contexts<TS, TI, TR> *mp_cell_contexts;
  const local_operation<TS, TI, TR> *mp_op;
  unsigned int m_output_layer;
};

template <class TS, class TI, class TR>
class DB_PUBLIC local_processor_result_computation_worker
  : public tl::Worker
{
public:
  local_processor_result_computation_worker ()
    : tl::Worker ()
  {
    //  .. nothing yet ..
  }

  void perform_task (tl::Task *task)
  {
    static_cast<local_processor_result_computation_task<TS, TI, TR> *> (task)->perform ();
  }
};

template <class TS, class TI, class TR>
class DB_PUBLIC local_processor
{
public:
  local_processor (db::Layout *layout, db::Cell *top, const std::set<db::cell_index_type> *breakout_cells = 0);
  local_processor (db::Layout *subject_layout, db::Cell *subject_top, const db::Layout *intruder_layout, const db::Cell *intruder_cell, const std::set<db::cell_index_type> *subject_breakout_cells = 0, const std::set<db::cell_index_type> *intruder_breakout_cells = 0);
  void run (local_operation<TS, TI, TR> *op, unsigned int subject_layer, unsigned int intruder_layer, unsigned int output_layer);
  void compute_contexts (local_processor_contexts<TS, TI, TR> &contexts, const local_operation<TS, TI, TR> *op, unsigned int subject_layer, unsigned int intruder_layer) const;
  void compute_results (local_processor_contexts<TS, TI, TR> &contexts, const local_operation<TS, TI, TR> *op, unsigned int output_layer) const;

  void set_description (const std::string &d)
  {
    m_description = d;
  }

  void set_base_verbosity (int vb)
  {
    m_base_verbosity = vb;
  }

  int base_verbosity () const
  {
    return m_base_verbosity;
  }

  void set_threads (unsigned int nthreads)
  {
    m_nthreads = nthreads;
  }

  unsigned int threads () const
  {
    return m_nthreads;
  }

  void set_max_vertex_count (size_t max_vertex_count)
  {
    m_max_vertex_count = max_vertex_count;
  }

  size_t max_vertex_count () const
  {
    return m_max_vertex_count;
  }

  void set_area_ratio (double area_ratio)
  {
    m_area_ratio = area_ratio;
  }

  double area_ratio () const
  {
    return m_area_ratio;
  }

private:
  template<typename, typename, typename> friend class local_processor_cell_contexts;
  template<typename, typename, typename> friend class local_processor_context_computation_task;

  db::Layout *mp_subject_layout;
  const db::Layout *mp_intruder_layout;
  db::Cell *mp_subject_top;
  const db::Cell *mp_intruder_top;
  const std::set<db::cell_index_type> *mp_subject_breakout_cells;
  const std::set<db::cell_index_type> *mp_intruder_breakout_cells;
  std::string m_description;
  unsigned int m_nthreads;
  size_t m_max_vertex_count;
  double m_area_ratio;
  int m_base_verbosity;
  mutable std::auto_ptr<tl::Job<local_processor_context_computation_worker<TS, TI, TR> > > mp_cc_job;
  mutable size_t m_progress;
  mutable tl::Progress *mp_progress;

  std::string description (const local_operation<TS, TI, TR> *op) const;
  void next () const;
  size_t get_progress () const;
  void compute_contexts (db::local_processor_contexts<TS, TI, TR> &contexts, db::local_processor_cell_context<TS, TI, TR> *parent_context, db::Cell *subject_parent, db::Cell *subject_cell, const db::ICplxTrans &subject_cell_inst, const db::Cell *intruder_cell, const typename local_processor_cell_contexts<TS, TI, TR>::context_key_type &intruders, db::Coord dist) const;
  void do_compute_contexts (db::local_processor_cell_context<TS, TI, TR> *cell_context, const db::local_processor_contexts<TS, TI, TR> &contexts, db::local_processor_cell_context<TS, TI, TR> *parent_context, db::Cell *subject_parent, db::Cell *subject_cell, const db::ICplxTrans &subject_cell_inst, const db::Cell *intruder_cell, const typename local_processor_cell_contexts<TS, TI, TR>::context_key_type &intruders, db::Coord dist) const;
  void issue_compute_contexts (db::local_processor_contexts<TS, TI, TR> &contexts, db::local_processor_cell_context<TS, TI, TR> *parent_context, db::Cell *subject_parent, db::Cell *subject_cell, const db::ICplxTrans &subject_cell_inst, const db::Cell *intruder_cell, typename local_processor_cell_contexts<TS, TI, TR>::context_key_type &intruders, db::Coord dist) const;
  void push_results (db::Cell *cell, unsigned int output_layer, const std::unordered_set<TR> &result) const;
  void compute_local_cell (const db::local_processor_contexts<TS, TI, TR> &contexts, db::Cell *subject_cell, const db::Cell *intruder_cell, const local_operation<TS, TI, TR> *op, const typename local_processor_cell_contexts<TS, TI, TR>::context_key_type &intruders, std::unordered_set<TR> &result) const;
  std::pair<bool, db::CellInstArray> effective_instance (local_processor_contexts<TS, TI, TR> &contexts, db::cell_index_type subject_cell_index, db::cell_index_type intruder_cell_index, const db::ICplxTrans &ti2s, db::Coord dist) const;

  bool subject_cell_is_breakout (db::cell_index_type ci) const
  {
    return mp_subject_breakout_cells && mp_subject_breakout_cells->find (ci) != mp_subject_breakout_cells->end ();
  }

  bool intruder_cell_is_breakout (db::cell_index_type ci) const
  {
    return mp_intruder_breakout_cells && mp_intruder_breakout_cells->find (ci) != mp_intruder_breakout_cells->end ();
  }
};

}

namespace tl
{

template <class TS, class TI, class TR>
struct type_traits<db::local_processor<TS, TI, TR> > : public tl::type_traits<void>
{
  //  mark "LocalProcessor" as not having a default ctor and no copy ctor
  typedef tl::false_tag has_default_constructor;
  typedef tl::false_tag has_copy_constructor;
};

}

#endif

