
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


#ifndef HDR_layRedrawThreadWorker
#define HDR_layRedrawThreadWorker

#include "dbLayout.h"
#include "layLayoutViewBase.h"
#include "tlThreadedWorkers.h"
#include "tlTimer.h"

#include <memory>
#include <map>
#include <vector>
#include <set>

namespace lay
{

class RedrawThreadCanvas;
class RedrawThread;
class Drawing;
class CanvasPlane;

//  some helpful constants
const int planes_per_layer = 12;
const int cell_box_planes = planes_per_layer; // for cell boxes
const int guiding_shape_planes = planes_per_layer; // for guiding shapes 
const int special_planes_before = cell_box_planes + guiding_shape_planes; 
const int special_planes_after = 1;
const int special_queue_entries = 2;
const int draw_boxes_queue_entry = -1;
const int draw_custom_queue_entry = -2;

/**
 *  @brief A compare operator for the cell variant cache
 */
class CellVariantCacheCompare
{
public:
  bool operator () (const std::pair<db::CplxTrans, db::cell_index_type> &a,
                    const std::pair<db::CplxTrans, db::cell_index_type> &b) const
  {
    if (a.second != b.second) {
      return a.second < b.second;
    }
    return a.first.less (b.first);
  }
};

/**
 *  @brief A task object for the redraw thread worker (a tl::Task specialization)
 */
class RedrawThreadTask
  : public tl::Task
{
public: 
  RedrawThreadTask (int id)
    : m_id (id)
  { }

  int id () const
  {
    return m_id;
  }

private:
  int m_id;
};

/**
 *  @brief An entry in the drawing cache
 */
struct CellCacheKey 
{
public:
  CellCacheKey (int n, db::cell_index_type c, const db::CplxTrans &t) 
    : nlevels (n), ci (c), trans (t)
  { }

  int nlevels;
  db::cell_index_type ci;
  db::CplxTrans trans;

  bool operator< (const CellCacheKey &other) const
  {
    if (nlevels != other.nlevels) {
      return nlevels < other.nlevels;
    }
    if (ci != other.ci) {
      return ci < other.ci;
    }
    if (! trans.equal (other.trans)) {
      return trans.less (other.trans);
    }
    return false;
  }
};

/**
 *  @brief An value in the drawing cache
 */
struct CellCacheInfo 
{
public:
  CellCacheInfo ()
    : hits (0), fill (0), frame (0), vertex (0), text (0)
  { }

  ~CellCacheInfo () 
  {
    delete fill;
    fill = 0;
    delete frame;
    frame = 0;
    delete vertex;
    vertex = 0;
    delete text;
    text = 0;
  }

  size_t hits;
  db::DPoint offset;
  lay::Bitmap *fill, *frame, *vertex, *text;
};

/**
 *  @brief A callback class which is triggered when a snapshot is taken
 */
class UpdateSnapshotCallback
{
public:
  UpdateSnapshotCallback () { }
  virtual ~UpdateSnapshotCallback () { }
  virtual void trigger () const { };
};

/**
 *  @brief A worker for the redraw thread (a tl::Worker specialization)
 */
class RedrawThreadWorker 
  : public tl::Worker
{
public:
  typedef std::map<CellCacheKey, CellCacheInfo> cell_cache_t;
  typedef std::map<std::pair<db::cell_index_type, unsigned int>, bool> micro_instance_cache_t;

  RedrawThreadWorker (RedrawThread *redraw_thread);
  virtual ~RedrawThreadWorker ();

  void setup (LayoutViewBase *view, RedrawThreadCanvas *canvas, const std::vector<db::Box> &redraw_region, const db::DCplxTrans &vp_trans);
  void finish ();

protected:
  void perform_task (tl::Task *task);

private:
  void draw_layer (bool drawing_context, db::cell_index_type ci, const db::CplxTrans &trans, const std::vector <db::Box> &redraw_regions, int level);
  void draw_layer (int from_level, int to_level, db::cell_index_type ci, const db::CplxTrans &trans, const std::vector <db::Box> &redraw_regions, int level, lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertex, lay::CanvasPlane *text, const UpdateSnapshotCallback *update_snapshot);
  void draw_layer (int from_level, int to_level, db::cell_index_type ci, const db::CplxTrans &trans, const db::Box &redraw_box, int level, lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertex, lay::CanvasPlane *text, const UpdateSnapshotCallback *update_snapshot);
  void draw_layer_wo_cache (int from_level, int to_level, db::cell_index_type ci, const db::CplxTrans &trans, const std::vector<db::Box> &vv, int level, lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertex, lay::CanvasPlane *text, const UpdateSnapshotCallback *update_snapshot);
  void draw_text_layer (bool drawing_context, db::cell_index_type ci, const db::CplxTrans &trans, const std::vector <db::Box> &redraw_regions, int level);
  void draw_text_layer (bool drawing_context, db::cell_index_type ci, const db::CplxTrans &trans, const db::Box &redraw_region, int level, lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertex, lay::CanvasPlane *text, Bitmap *opt_bitmap);
  void draw_boxes (bool drawing_context, db::cell_index_type ci, const db::CplxTrans &trans, const std::vector <db::Box> &redraw_regions, int level);
  void draw_boxes (bool drawing_context, db::cell_index_type ci, const db::CplxTrans &trans, const db::Box &redraw_region, int level);
  void draw_box_properties (bool drawing_context, db::cell_index_type ci, const db::CplxTrans &trans, const std::vector <db::Box> &redraw_regions, int level);
  void draw_box_properties (bool drawing_context, db::cell_index_type ci, const db::CplxTrans &trans, const std::vector <db::Box> &redraw_regions, int level, db::properties_id_type prop_id);
  void draw_box_properties (bool drawing_context, db::cell_index_type ci, const db::CplxTrans &trans, const db::Box &redraw_box, int level, db::properties_id_type prop_id);
  void draw_cell (bool drawing_context, int level, const db::CplxTrans &trans, const db::Box &box, const std::string &txt);
  void draw_cell_properties (bool drawing_context, int level, const db::CplxTrans &trans, const db::Box &box, db::properties_id_type prop_id);
  void draw_cell_shapes (const db::CplxTrans &trans, const db::Cell &cell, const db::Box &vp, lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertex, lay::CanvasPlane *text);
  void test_snapshot (const UpdateSnapshotCallback *update_snapshot);
  void transfer ();
  void iterate_variants (const std::vector <db::Box> &redraw_regions, db::cell_index_type ci, db::CplxTrans trans, void (RedrawThreadWorker::*what) (bool, db::cell_index_type ci, const db::CplxTrans &, const std::vector <db::Box> &, int level));
  void iterate_variants_rec (const std::vector <db::Box> &redraw_regions, db::cell_index_type ci, const db::CplxTrans &trans, int level, void (RedrawThreadWorker::*what) (bool, db::cell_index_type ci, const db::CplxTrans &, const std::vector <db::Box> &, int level), bool spread);
  bool cell_var_cached (db::cell_index_type ci, const db::CplxTrans &trans);
  bool drop_cell (const db::Cell &cell, const db::CplxTrans &trans);
  std::vector<db::Box> search_regions (const db::Box &cell_bbox, const db::Box &vp, int level);
  bool any_shapes (db::cell_index_type cell_index, unsigned int levels);
  bool any_text_shapes (db::cell_index_type cell_index, unsigned int levels);
  bool any_cell_box (db::cell_index_type cell_index, unsigned int levels);

  RedrawThread *mp_redraw_thread;
  std::vector <db::Box> m_redraw_region;
  std::vector <lay::Drawing *> mp_drawings;
  lay::RedrawThreadCanvas *mp_canvas;
  lay::CanvasPlane *m_planes[planes_per_layer];

  std::vector<db::Box> m_vv;
  int m_from_level, m_to_level;
  int m_from_level_default, m_to_level_default;
  int m_min_size_for_label;
  bool m_box_text_transform;
  unsigned int m_box_font;
  unsigned int m_text_font;
  bool m_text_visible;
  bool m_text_lazy_rendering;
  bool m_bitmap_caching;
  bool m_show_properties;
  bool m_apply_text_trans;
  double m_default_text_size;
  bool m_drop_small_cells;
  unsigned int m_drop_small_cells_value;
  lay::LayoutViewBase::drop_small_cells_cond_type m_drop_small_cells_cond;
  bool m_draw_array_border_instances;
  double m_abstract_mode_width;
  bool m_child_context_enabled;

  micro_instance_cache_t m_mi_cache, m_mi_text_cache, m_mi_cell_box_cache;
  cell_cache_t m_cell_cache;
  std::set <std::pair <db::CplxTrans, db::cell_index_type>, lay::CellVariantCacheCompare> *mp_cell_var_cache;
  unsigned int m_cache_hits, m_cache_misses;
  std::set <std::pair <db::DCplxTrans, int> > m_box_variants;
  std::vector <std::set <lay::LayoutViewBase::cell_index_type> > m_hidden_cells;
  std::vector <lay::CellView> m_cellviews;
  const db::Layout *mp_layout;
  int m_cv_index;
  unsigned int m_layer;
  int m_nlayers;
  bool m_xfill;
  const std::set<db::properties_id_type> *mp_prop_sel;
  bool m_inv_prop_sel;
  db::DCplxTrans m_vp_trans;
  std::vector<std::pair<unsigned int, lay::CanvasPlane *> > m_buffers;
  unsigned int m_test_count;
  tl::Clock m_clock;
  std::unique_ptr<lay::Renderer> mp_renderer;
};

}

#endif

