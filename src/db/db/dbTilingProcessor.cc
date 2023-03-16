
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


#include "dbTilingProcessor.h"

#include "tlExpression.h"
#include "tlProgress.h"
#include "tlThreadedWorkers.h"
#include "tlThreads.h"
#include "gsiDecl.h"

#include <cmath>

namespace db
{

/**
 *  @brief A helper class for the generic implementation of the layout insert functionality
 */
class ShapesInserter
{
public:
  ShapesInserter (db::Shapes *shapes, const db::ICplxTrans &trans, db::Coord ep_sizing)
    : mp_shapes (shapes), m_trans (trans), m_ep_sizing (ep_sizing)
  {
    //  .. nothing yet ..
  }

  template <class T>
  void operator() (const T &t)
  {
    mp_shapes->insert (t.transformed (m_trans));
  }

  template <class P>
  void insert_polygon (const P &p)
  {
    if (p.is_box () && ! m_trans.is_complex ()) {
      mp_shapes->insert (p.box ().transformed (m_trans));
    } else {
      if (mp_shapes->cell () && mp_shapes->cell ()->layout ()) {
        db::polygon_ref<P, db::Disp> pr (p.transformed (m_trans), mp_shapes->cell ()->layout ()->shape_repository ());
        mp_shapes->insert (pr);
      } else {
        mp_shapes->insert (p.transformed (m_trans));
      }
    }
  }

  void operator() (const db::Polygon &p)
  {
    insert_polygon (p);
  }

  void operator() (const db::SimplePolygon &p)
  {
    insert_polygon (p);
  }

  void operator() (const db::EdgePair &ep)
  {
    mp_shapes->insert (ep.normalized ().to_polygon (m_ep_sizing).transformed (m_trans));
  }

private:
  db::Shapes *mp_shapes;
  const db::ICplxTrans m_trans;
  db::Coord m_ep_sizing;
};

/**
 *  @brief A helper class for the generic implementation of the region insert functionality
 */
class RegionInserter
{
public:
  RegionInserter (db::Region *region, const db::ICplxTrans &trans, db::Coord ep_sizing)
    : mp_region (region), m_trans (trans), m_ep_sizing (ep_sizing)
  {
    //  .. nothing yet ..
  }

  template <class T>
  void operator() (const T &t)
  {
    mp_region->insert (t.transformed (m_trans));
  }

  void operator() (const db::Text &)
  {
    //  .. texts are discarded ..
  }

  void operator() (const db::Edge &)
  {
    //  .. edges are discarded ..
  }

  void operator() (const db::EdgePair &ep)
  {
    mp_region->insert (ep.normalized ().to_polygon (m_ep_sizing).transformed (m_trans));
  }

private:
  db::Region *mp_region;
  const db::ICplxTrans m_trans;
  db::Coord m_ep_sizing;
};

/**
 *  @brief A helper class for the generic implementation of the edge collection insert functionality
 */
class EdgesInserter
{
public:
  EdgesInserter (db::Edges *edges, const db::ICplxTrans &trans)
    : mp_edges (edges), m_trans (trans)
  {
    //  .. nothing yet ..
  }

  void operator() (const db::Text &)
  {
    //  .. texts are discarded ..
  }

  void operator() (const db::EdgePair &ep)
  {
    mp_edges->insert (ep.first ().transformed (m_trans));
    mp_edges->insert (ep.second ().transformed (m_trans));
  }

  template <class T>
  void operator() (const T &t)
  {
    mp_edges->insert (t.transformed (m_trans));
  }

private:
  db::Edges *mp_edges;
  const db::ICplxTrans m_trans;
};

/**
 *  @brief A helper class for the generic implementation of the edge pair collection insert functionality
 */
class EdgePairsInserter
{
public:
  EdgePairsInserter (db::EdgePairs *edge_pairs, const db::ICplxTrans &trans)
    : mp_edge_pairs (edge_pairs), m_trans (trans)
  {
    //  .. nothing yet ..
  }

  template <class T>
  void operator() (const T &)
  {
    //  .. discard anything except EdgePairs ..
  }

  void operator() (const db::EdgePair &ep)
  {
    mp_edge_pairs->insert (ep.transformed (m_trans));
  }

private:
  db::EdgePairs *mp_edge_pairs;
  const db::ICplxTrans m_trans;
};

/**
 *  @brief A helper class for the generic implementation of the text collection insert functionality
 */
class TextsInserter
{
public:
  TextsInserter (db::Texts *texts, const db::ICplxTrans &trans)
    : mp_texts (texts), m_trans (trans)
  {
    //  .. nothing yet ..
  }

  template <class T>
  void operator() (const T &)
  {
    //  .. discard anything except Texts ..
  }

  void operator() (const db::Text &t)
  {
    mp_texts->insert (t.transformed (m_trans));
  }

private:
  db::Texts *mp_texts;
  const db::ICplxTrans m_trans;
};

class TileLayoutOutputReceiver
  : public db::TileOutputReceiver
{
public:
  TileLayoutOutputReceiver (db::Layout *layout, db::Cell *cell, unsigned int layer, db::Coord e)
    : mp_layout (layout), mp_cell (cell), m_layer (layer), m_ep_sizing (e)
  {
    //  .. nothing yet ..
  }

  void put (size_t /*ix*/, size_t /*iy*/, const db::Box &tile, size_t /*id*/, const tl::Variant &obj, double dbu, const db::ICplxTrans &trans, bool clip)
  {
    db::ICplxTrans t (db::ICplxTrans (dbu / mp_layout->dbu ()) * trans);
    db::Shapes &shapes = mp_cell->shapes (m_layer);
    ShapesInserter inserter (&shapes, t, m_ep_sizing);

    insert_var (inserter, obj, tile, clip);
  }

  void begin (size_t /*nx*/, size_t /*ny*/, const db::DPoint & /*p0*/, double /*dx*/, double /*dy*/, const db::DBox & /*frame*/)
  { 
    mp_layout->start_changes ();
  }

  void finish (bool /*success*/)
  { 
    mp_layout->end_changes ();
  }

private:
  db::Layout *mp_layout;
  db::Cell *mp_cell;
  unsigned int m_layer;
  db::Coord m_ep_sizing;
};

class TileRegionOutputReceiver
  : public db::TileOutputReceiver
{
public:
  TileRegionOutputReceiver (db::Region *region, db::Coord e)
    : mp_region (region), m_ep_sizing (e)
  {
    //  .. nothing yet ..
  }

  void put (size_t /*ix*/, size_t /*iy*/, const db::Box &tile, size_t /*id*/, const tl::Variant &obj, double /*dbu*/, const db::ICplxTrans &trans, bool clip)
  {
    //  optimisation
    if (obj.is_user<db::Region> () && ! clip) {
      *mp_region += obj.to_user<db::Region> ();
    } else {
      RegionInserter inserter (mp_region, trans, m_ep_sizing);
      insert_var (inserter, obj, tile, clip);
    }
  }

private:
  db::Region *mp_region;
  db::Coord m_ep_sizing;
};

class TileEdgesOutputReceiver
  : public db::TileOutputReceiver
{
public:
  TileEdgesOutputReceiver (db::Edges *edges)
    : mp_edges (edges)
  {
    //  .. nothing yet ..
  }

  void put (size_t /*ix*/, size_t /*iy*/, const db::Box &tile, size_t /*id*/, const tl::Variant &obj, double /*dbu*/, const db::ICplxTrans &trans, bool clip)
  {
    //  optimisation
    if (obj.is_user<db::Edges> () && ! clip) {
      *mp_edges += obj.to_user<db::Edges> ();
    } else {
      EdgesInserter inserter (mp_edges, trans);
      insert_var (inserter, obj, tile, clip);
    }
  }

private:
  db::Edges *mp_edges;
};

class TileEdgePairsOutputReceiver
  : public db::TileOutputReceiver
{
public:
  TileEdgePairsOutputReceiver (db::EdgePairs *edge_pairs)
    : mp_edge_pairs (edge_pairs)
  {
    //  .. nothing yet ..
  }

  void put (size_t /*ix*/, size_t /*iy*/, const db::Box &tile, size_t /*id*/, const tl::Variant &obj, double /*dbu*/, const db::ICplxTrans &trans, bool clip)
  {
    EdgePairsInserter inserter (mp_edge_pairs, trans);
    insert_var (inserter, obj, tile, clip);
  }

private:
  db::EdgePairs *mp_edge_pairs;
};

class TileTextsOutputReceiver
  : public db::TileOutputReceiver
{
public:
  TileTextsOutputReceiver (db::Texts *texts)
    : mp_texts (texts)
  {
    //  .. nothing yet ..
  }

  void put (size_t /*ix*/, size_t /*iy*/, const db::Box &tile, size_t /*id*/, const tl::Variant &obj, double /*dbu*/, const db::ICplxTrans &trans, bool clip)
  {
    TextsInserter inserter (mp_texts, trans);
    insert_var (inserter, obj, tile, clip);
  }

private:
  db::Texts *mp_texts;
};

class TilingProcessorJob
  : public tl::JobBase
{
public:
  TilingProcessorJob (TilingProcessor *proc, int nworkers, bool has_tiles)
    : tl::JobBase (nworkers),
      mp_proc (proc),
      m_has_tiles (has_tiles),
      m_progress_count (0),
      m_progress (std::string ())
  {
    //  .. nothing yet ..
  }

  void start (const std::string &job_description);

  bool has_tiles () const
  {
    return m_has_tiles;
  }

  void next_progress () 
  {
    tl::MutexLocker locker (&m_mutex);
    ++m_progress_count;
  }

  void update_progress ()
  {
    unsigned int p;
    {
      tl::MutexLocker locker (&m_mutex);
      p = m_progress_count;
    }

    m_progress.set (p, true /*force yield*/);
  }

  TilingProcessor *processor () const
  {
    return mp_proc;
  }

  virtual tl::Worker *create_worker ();

  virtual void after_sync_task (tl::Task *task);

private:
  TilingProcessor *mp_proc;
  bool m_has_tiles;
  unsigned int m_progress_count;
  tl::Mutex m_mutex;
  tl::RelativeProgress m_progress;
};

class TilingProcessorTask
  : public tl::Task
{
public:
  TilingProcessorTask (const std::string &tile_desc, size_t ix, size_t iy, const db::DBox &clip_box, const db::DBox &region, const std::string &script, size_t script_index)
    : m_tile_desc (tile_desc), m_ix (ix), m_iy (iy), m_clip_box (clip_box), m_region (region), m_script (script), m_script_index (script_index)
  {
    //  .. nothing yet ..
  }

  const std::string &tile_desc () const
  {
    return m_tile_desc;
  }
  
  const db::DBox &clip_box () const
  {
    return m_clip_box;
  }

  size_t ix () const
  {
    return m_ix;
  }
  
  size_t iy () const
  {
    return m_iy;
  }
  
  const db::DBox &region () const
  {
    return m_region;
  }
  
  const std::string &script () const
  {
    return m_script;
  }

  size_t script_index () const
  {
    return m_script_index;
  }

private:
  std::string m_tile_desc;
  size_t m_ix, m_iy;
  db::DBox m_clip_box, m_region;
  std::string m_script;
  size_t m_script_index;
};

class TilingProcessorWorker
  : public tl::Worker
{
public:
  TilingProcessorWorker (TilingProcessorJob *job)
    : tl::Worker (), mp_job (job)
  {
    //  .. nothing yet ..
  }

  void perform_task (tl::Task *task) 
  {
    TilingProcessorTask *tile_task = dynamic_cast <TilingProcessorTask *> (task);
    if (tile_task) {
      do_perform (tile_task);
    }
  }

private:
  TilingProcessorJob *mp_job;

  void do_perform (const TilingProcessorTask *task);
  void make_input_var (const TilingProcessor::InputSpec &is, const db::RecursiveShapeIterator *iter, tl::Eval &eval, double sf);
};

class TilingProcessorReceiverFunction
  : public tl::EvalFunction
{
public:
  TilingProcessorReceiverFunction (TilingProcessor *proc)
    : mp_proc (proc)
  {
    //  .. nothing yet ..
  }

  void execute (const tl::ExpressionParserContext & /*context*/, tl::Variant &out, const std::vector<tl::Variant> &args) const
  {
    out = mp_proc->receiver (args);
  }

private:
  TilingProcessor *mp_proc;
};

class TilingProcessorOutputFunction
  : public tl::EvalFunction
{
public:
  TilingProcessorOutputFunction (TilingProcessor *proc, size_t ix, size_t iy, const db::Box &tile_box)
    : mp_proc (proc), m_ix (ix), m_iy (iy), m_tile_box (tile_box)
  {
    //  .. nothing yet ..
  }

  void execute (const tl::ExpressionParserContext & /*context*/, tl::Variant & /*out*/, const std::vector<tl::Variant> &args) const 
  {
    mp_proc->put (m_ix, m_iy, m_tile_box, args);
  }

private:
  TilingProcessor *mp_proc;
  size_t m_ix, m_iy;
  db::Box m_tile_box; 
};

class TilingProcessorCountFunction
  : public tl::EvalFunction
{
public:
  TilingProcessorCountFunction (TilingProcessor * /*proc*/)
  {
    //  .. nothing yet ..
  }

  void execute (const tl::ExpressionParserContext & /*context*/, tl::Variant & /*out*/, const std::vector<tl::Variant> & /*args*/) const 
  {
    // TODO: ... implement ..
  }
};

void
TilingProcessorWorker::make_input_var (const TilingProcessor::InputSpec &is, const db::RecursiveShapeIterator *iter, tl::Eval &eval, double sf)
{
  if (! iter) {
    iter = &is.iter;
  }

  if (is.type == TilingProcessor::TypeRegion) {
    eval.set_var (is.name, tl::Variant (db::Region (*iter, db::ICplxTrans (sf) * is.trans, is.merged_semantics)));
  } else if (is.type == TilingProcessor::TypeEdges) {
    eval.set_var (is.name, tl::Variant (db::Edges (*iter, db::ICplxTrans (sf) * is.trans, is.merged_semantics)));
  } else if (is.type == TilingProcessor::TypeEdgePairs) {
    eval.set_var (is.name, tl::Variant (db::EdgePairs (*iter, db::ICplxTrans (sf) * is.trans)));
  } else if (is.type == TilingProcessor::TypeTexts) {
    eval.set_var (is.name, tl::Variant (db::Texts (*iter, db::ICplxTrans (sf) * is.trans)));
  }
}

void
TilingProcessorWorker::do_perform (const TilingProcessorTask *tile_task)
{
  tl::Eval eval (&mp_job->processor ()->top_eval ());

  db::Box clip_box_dbu = db::Box::world ();

  eval.set_var ("_dbu", tl::Variant (mp_job->processor ()->dbu ()));

  if (! mp_job->has_tiles ()) { 
    eval.set_var ("_tile", tl::Variant ());
  } else {

    clip_box_dbu = db::Box (tile_task->clip_box ().transformed (db::DCplxTrans (mp_job->processor ()->dbu ()).inverted ()));

    db::Region r;
    r.insert (clip_box_dbu);
    eval.set_var ("_tile", tl::Variant (r));

  }

  {
    db::Box frame_box_dbu = db::Box (mp_job->processor ()->frame ().transformed (db::DCplxTrans (mp_job->processor ()->dbu ()).inverted ()));

    db::Region r;
    r.insert (frame_box_dbu);
    eval.set_var ("_frame", tl::Variant (r));
  }

  for (std::vector<TilingProcessor::InputSpec>::const_iterator i = mp_job->processor ()->begin_inputs (); i != mp_job->processor ()->end_inputs (); ++i) {

    double dbu = mp_job->processor ()->dbu ();
    if (mp_job->processor ()->scale_to_dbu () && i->iter.layout ()) {
      dbu = i->iter.layout ()->dbu ();
    }

    double sf = dbu / mp_job->processor ()->dbu ();

    if (! mp_job->has_tiles ()) { 

      make_input_var (*i, 0, eval, sf);

    } else {

      db::Box region_dbu = db::Box (tile_task->region ().transformed ((db::DCplxTrans (dbu) * db::DCplxTrans (i->trans)).inverted ()));
      region_dbu &= i->iter.region ();

      db::RecursiveShapeIterator iter;
      if (! region_dbu.empty ()) {
        iter = i->iter;
        iter.confine_region (region_dbu);
      }

      make_input_var (*i, &iter, eval, sf);

    }

  }

  eval.define_function ("_output", new TilingProcessorOutputFunction (mp_job->processor (), tile_task->ix (), tile_task->iy (), clip_box_dbu));
  eval.define_function ("_rec", new TilingProcessorReceiverFunction (mp_job->processor ()));
  eval.define_function ("_count", new TilingProcessorCountFunction (mp_job->processor ()));

  if (tl::verbosity () >= (mp_job->has_tiles () ? 20 : 10)) {
    tl::info << "TilingProcessor: script #" << (tile_task->script_index () + 1) << ", tile " << tile_task->tile_desc ();
  }

  tl::SelfTimer timer (tl::verbosity () >= (mp_job->has_tiles () ? 21 : 11), "Elapsed time");

  tl::Expression ex;
  eval.parse (ex, tile_task->script ());
  ex.execute ();

  mp_job->next_progress ();
}

tl::Worker *
TilingProcessorJob::create_worker ()
{
  return new TilingProcessorWorker (this);
}

void
TilingProcessorJob::after_sync_task (tl::Task * /*task*/)
{
  //  This needs to be done here as there is no external loop to do this
  update_progress ();
}

void
TilingProcessorJob::start (const std::string &job_description)
{
  m_progress = tl::RelativeProgress (job_description, tasks (), 1);
  //  prevents child progress objects from showing
  m_progress.set_final (true);

  tl::JobBase::start ();
}

// ----------------------------------------------------------------------------------
//  The tiling processor implementation

tl::Mutex TilingProcessor::s_output_lock;

TilingProcessor::TilingProcessor ()
  : m_tile_width (0.0), m_tile_height (0.0),
    m_ntiles_w (0), m_ntiles_h (0), 
    m_tile_size_given (false), m_tile_count_given (false),
    m_tile_origin_x (0.0), m_tile_origin_y (0.0),
    m_tile_origin_given (false),
    m_tile_bx (0.0), m_tile_by (0.0),
    m_threads (0), m_dbu (0.001), m_dbu_specific (0.001), m_dbu_specific_set (false),
    m_scale_to_dbu (true)
{
  //  .. nothing yet ..
}

void 
TilingProcessor::input (const std::string &name, const db::RecursiveShapeIterator &iter, const db::ICplxTrans &trans, Type type, bool merged_semantics)
{
  if (m_inputs.empty () && iter.layout ()) {
    m_dbu = iter.layout ()->dbu ();
  }
  m_inputs.push_back (InputSpec ());
  m_inputs.back ().name = name;
  m_inputs.back ().iter = iter;
  m_inputs.back ().trans = trans;
  m_inputs.back ().type = type;
  m_inputs.back ().merged_semantics = merged_semantics;
}

TilingProcessor::~TilingProcessor ()
{
  m_outputs.clear ();
}

void
TilingProcessor::set_frame (const db::DBox &frame)
{
  m_frame = frame;
}

void  
TilingProcessor::tile_size (double w, double h)
{
  m_tile_width = std::max (0.0, w);
  m_tile_height = std::max (0.0, h);
  m_tile_size_given = true;
}

void  
TilingProcessor::tiles (size_t nx, size_t ny)
{
  m_ntiles_w = nx;
  m_ntiles_h = ny;
  m_tile_count_given = true;
}

void  
TilingProcessor::tile_origin (double xo, double yo)
{
  m_tile_origin_x = xo;
  m_tile_origin_y = yo;
  m_tile_origin_given = true;
}

void  
TilingProcessor::tile_border (double bx, double by)
{
  m_tile_bx = std::max (0.0, bx);
  m_tile_by = std::max (0.0, by);
}

void  
TilingProcessor::set_threads (size_t n)
{
  m_threads = n;
}

void  
TilingProcessor::queue (const std::string &script)
{
  m_scripts.push_back (script);
}

void  
TilingProcessor::var (const std::string &name, const tl::Variant &value)
{
  m_top_eval.set_var (name, value);
}


void  
TilingProcessor::output (const std::string &name, size_t id, TileOutputReceiver *rec, const db::ICplxTrans &trans)
{
  if (! rec) {
    return;
  }

  m_top_eval.set_var (name, m_outputs.size ());
  m_outputs.push_back (OutputSpec ());
  m_outputs.back ().name = name;
  m_outputs.back ().id = id;
  m_outputs.back ().receiver = rec;
  m_outputs.back ().trans = trans;
}

void   
TilingProcessor::output (const std::string &name, db::Layout &layout, db::cell_index_type cell_index, const db::LayerProperties &lp, db::Coord ep_ext)
{
  //  if we have a layer with the requested properties already, return this.
  db::Layout::layer_iterator li = layout.begin_layers ();
  for ( ; li != layout.end_layers (); ++li) {
    if ((*li).second->log_equal (lp)) {
      break;
    }
  }

  unsigned int layer;
  if (li != layout.end_layers ()) {
    layer = (*li).first;
  } else {
    layer = layout.insert_layer (lp);
  }

  output (name, layout, cell_index, layer, ep_ext);
}

void   
TilingProcessor::output (const std::string &name, db::Layout &layout, db::cell_index_type cell_index, unsigned int layer, db::Coord ep_ext)
{
  m_top_eval.set_var (name, m_outputs.size ());
  m_outputs.push_back (OutputSpec ());
  m_outputs.back ().name = name;
  m_outputs.back ().id = 0;
  m_outputs.back ().receiver = new TileLayoutOutputReceiver (&layout, &layout.cell (cell_index), layer, ep_ext);
}

void 
TilingProcessor::output (const std::string &name, db::Region &region, db::Coord ep_ext)
{
  m_top_eval.set_var (name, m_outputs.size ());
  m_outputs.push_back (OutputSpec ());
  m_outputs.back ().name = name;
  m_outputs.back ().id = 0;
  m_outputs.back ().receiver = new TileRegionOutputReceiver (&region, ep_ext);
}

void 
TilingProcessor::output (const std::string &name, db::EdgePairs &edge_pairs)
{
  m_top_eval.set_var (name, m_outputs.size ());
  m_outputs.push_back (OutputSpec ());
  m_outputs.back ().name = name;
  m_outputs.back ().id = 0;
  m_outputs.back ().receiver = new TileEdgePairsOutputReceiver (&edge_pairs);
}

void
TilingProcessor::output (const std::string &name, db::Texts &texts)
{
  m_top_eval.set_var (name, m_outputs.size ());
  m_outputs.push_back (OutputSpec ());
  m_outputs.back ().name = name;
  m_outputs.back ().id = 0;
  m_outputs.back ().receiver = new TileTextsOutputReceiver (&texts);
}

void
TilingProcessor::output (const std::string &name, db::Edges &edges)
{
  m_top_eval.set_var (name, m_outputs.size ());
  m_outputs.push_back (OutputSpec ());
  m_outputs.back ().name = name;
  m_outputs.back ().id = 0;
  m_outputs.back ().receiver = new TileEdgesOutputReceiver (&edges);
}

tl::Variant
TilingProcessor::receiver (const std::vector<tl::Variant> &args)
{
  tl::MutexLocker locker (&s_output_lock);

  if (args.size () != 1) {
    throw tl::Exception (tl::to_string (tr ("_rec function requires one argument: the handle of the output channel")));
  }

  size_t index = args[0].to<size_t> ();
  if (index >= m_outputs.size ()) {
    throw tl::Exception (tl::to_string (tr ("Invalid handle in _rec function call")));
  }

  gsi::Proxy *proxy = new gsi::Proxy (gsi::cls_decl<TileOutputReceiver> ());
  proxy->set (m_outputs[index].receiver.get (), false, false, false);

  //  gsi::Object based objects are managed through a Proxy and
  //  shared pointers within tl::Variant. That means: copy by reference.
  return tl::Variant (proxy, gsi::cls_decl<TileOutputReceiver> ()->var_cls (true /*const*/), true);
}

void 
TilingProcessor::put (size_t ix, size_t iy, const db::Box &tile, const std::vector<tl::Variant> &args)
{
  tl::MutexLocker locker (&s_output_lock);

  if (args.size () < 2 || args.size () > 3) {
    throw tl::Exception (tl::to_string (tr ("_output function requires two or three arguments: handle and object and a clip flag (optional)")));
  }

  bool clip = ((args.size () <= 2 || args [2].to_bool ()) && ! tile.empty ());

  size_t index = args[0].to<size_t> ();
  if (index >= m_outputs.size ()) {
    throw tl::Exception (tl::to_string (tr ("Invalid handle (first argument) in _output function call")));
  }

  m_outputs[index].receiver->put (ix, iy, tile, m_outputs[index].id, args[1], dbu (), m_outputs[index].trans, clip);
}

void  
TilingProcessor::execute (const std::string &desc)
{
  db::DBox tot_box = m_frame;

  if (tot_box.empty ()) {
    for (std::vector<InputSpec>::const_iterator i = m_inputs.begin (); i != m_inputs.end (); ++i) {
      if (! i->iter.at_end ()) {
        if (scale_to_dbu ()) {
          double dbu_value = i->iter.layout () ? i->iter.layout ()->dbu () : dbu ();
          tot_box += i->iter.bbox ().transformed (db::CplxTrans (dbu_value) * db::CplxTrans (i->trans));
        } else {
          tot_box += i->iter.bbox ().transformed (db::CplxTrans (dbu ()) * db::CplxTrans (i->trans));
        }
      }
    }
  }

  //  can't create tiles for empty input
  if (tot_box.empty () && ! (m_tile_count_given && m_tile_origin_given && m_tile_size_given)) {
    return;
  }

  db::DBox frame = tot_box;

  //  add the border to the total box. The reasoning for this is that for example an oversize might grow into
  //  the outer region and therefore be clipped.
  tot_box.enlarge (db::DVector (m_tile_bx, m_tile_by));

  //  create a tiling plan

  tl::SelfTimer timer_tot (tl::verbosity () >= 11, "Total tiling processor time");

  size_t ntiles_w = 0, ntiles_h = 0;
  double tile_width = 0.0, tile_height = 0.0;

  if (m_tile_size_given && ! m_tile_count_given) {

    tile_width = dbu () * floor (0.5 + m_tile_width / dbu () + 1e-10);
    tile_height = dbu () * floor (0.5 + m_tile_height / dbu () + 1e-10);
    ntiles_w = (m_tile_width > 1e-6 ? size_t (ceil (tot_box.width () / m_tile_width - 1e-10)) : 1);
    ntiles_h = (m_tile_height > 1e-6 ? size_t (ceil (tot_box.height () / m_tile_height - 1e-10)) : 1);

  } else if (! m_tile_size_given && m_tile_count_given) {

    ntiles_w = m_ntiles_w;
    ntiles_h = m_ntiles_h;
    tile_width = (m_ntiles_w > 0 ? dbu () * ceil (tot_box.width () / (dbu () * m_ntiles_w) - 1e-10) : 0.0);
    tile_height = (m_ntiles_h > 0 ? dbu () * ceil (tot_box.height () / (dbu () * m_ntiles_h) - 1e-10) : 0.0);

  } else if (m_tile_size_given && m_tile_count_given) {

    ntiles_w = m_ntiles_w;
    ntiles_h = m_ntiles_h;
    tile_width = dbu () * floor (0.5 + m_tile_width / dbu () + 1e-10);
    tile_height = dbu () * floor (0.5 + m_tile_height / dbu () + 1e-10);

  }

  //  NOTE: we use an explicit frame specification as an indication that
  //  the tiles are supposed to be treated in a tile context even if there
  //  is just a single tile.
  bool has_tiles = (ntiles_w > 1 || ntiles_h > 1 || ! m_frame.empty ());

  TilingProcessorJob job (this, int (m_threads), has_tiles);

  double l = 0.0, b = 0.0;

  if (has_tiles) {

    ntiles_w = std::max (size_t (1), ntiles_w);
    ntiles_h = std::max (size_t (1), ntiles_h);

    if (m_tile_origin_given) {
      l = dbu () * floor (0.5 + m_tile_origin_x / dbu () + 1e-10);
      b = dbu () * floor (0.5 + m_tile_origin_y / dbu () + 1e-10);
    } else {
      l = dbu () * floor (0.5 + (tot_box.center ().x () - ntiles_w * 0.5 * tile_width) / dbu () + 1e-10);
      b = dbu () * floor (0.5 + (tot_box.center ().y () - ntiles_h * 0.5 * tile_height) / dbu () + 1e-10);
    }

    //  create the TilingProcessor tasks
    for (size_t ix = 0; ix < ntiles_w; ++ix) {

      for (size_t iy = 0; iy < ntiles_h; ++iy) {

        db::DBox clip_box (l + ix * tile_width, b + iy * tile_height, l + (ix + 1) * tile_width, b + (iy + 1) * tile_height);
        db::DBox region = clip_box.enlarged (db::DVector (m_tile_bx, m_tile_by));

        std::string tile_desc = tl::sprintf ("%d/%d,%d/%d", ix + 1, ntiles_w, iy + 1, ntiles_h);

        size_t si = 0;
        for (std::vector <std::string>::const_iterator s = m_scripts.begin (); s != m_scripts.end (); ++s, ++si) {
          job.schedule (new TilingProcessorTask (tile_desc, ix, iy, clip_box, region, *s, si));
        }

      }

    }

  } else {

    ntiles_w = ntiles_h = 0;

    size_t si = 0;
    for (std::vector <std::string>::const_iterator s = m_scripts.begin (); s != m_scripts.end (); ++s, ++si) {
      job.schedule (new TilingProcessorTask ("all", 0, 0, db::DBox (), db::DBox (), *s, si));
    }

  }

  try {

    try {

      for (std::vector<OutputSpec>::iterator o = m_outputs.begin (); o != m_outputs.end (); ++o) {
        if (o->receiver) {
          o->receiver->set_processor (this);
          o->receiver->begin (ntiles_w, ntiles_h, db::DPoint (l, b), tile_width, tile_height, frame);
        }
      }

      job.start (desc);
      while (job.is_running ()) {
        //  This may throw an exception, if the cancel button has been pressed.
        job.update_progress ();
        job.wait (100);
      }

      for (std::vector<OutputSpec>::iterator o = m_outputs.begin (); o != m_outputs.end (); ++o) {
        if (o->receiver) {
          o->receiver->finish (!job.has_error ());
          o->receiver->set_processor (0);
        }
      }

    } catch (...) {
      for (std::vector<OutputSpec>::iterator o = m_outputs.begin (); o != m_outputs.end (); ++o) {
        if (o->receiver) {
          o->receiver->finish (false);
          o->receiver->set_processor (0);
        }
      }
      throw;
    }

  } catch (tl::BreakException &ex) {
    job.terminate ();
    throw ex;
  } catch (tl::Exception &ex) {
    job.terminate ();
    throw ex;
  }

  if (job.has_error ()) {
    throw tl::Exception (tl::to_string (tr ("Errors occurred during processing. First error message says:\n")) + job.error_messages ().front ());
  }
}

}

