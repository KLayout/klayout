
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


#ifndef HDR_layRedrawThread
#define HDR_layRedrawThread

#include <vector>
#include <set>
#include <memory>

#include "dbBox.h"
#include "dbTrans.h"
#include "dbLayout.h"
#include "layRenderer.h"
#include "layLayoutViewBase.h"
#include "layRedrawThreadCanvas.h"
#include "layRedrawLayerInfo.h"
#include "layCanvasPlane.h"
#include "tlTimer.h"
#include "tlThreads.h"
#include "tlThreadedWorkers.h"

namespace lay {

class Viewport;

//  update (snapshot) interval in ms
const int update_interval = 500;

class RedrawThread 
  : public tl::Object,
    public tl::JobBase
{
public:
  RedrawThread (lay::RedrawThreadCanvas *canvas, lay::LayoutViewBase *view);
  virtual ~RedrawThread ();

  void commit (const std::vector <lay::RedrawLayerInfo> &layers, const lay::Viewport &vp, double resolution);
  void start (int workers, const std::vector <lay::RedrawLayerInfo> &layers, const lay::Viewport &vp, double resolution, bool force_redraw);
  void restart (const std::vector<int> &restart);
  void wakeup_checked ();
  void wakeup ();

  /**
   *  @brief change the visibility of entries in the redrawing queue
   *
   *  HINT: this should be done only when the redraw thread is stopped.
   */
  void change_visibility (const std::vector<bool> &visibility);

  const RedrawLayerInfo &get_layer_info (int id) const
  {
    return m_layers [id];
  }

  int num_layers () const
  {
    return int (m_layers.size ());
  }

  void task_finished (int id);

protected:
  tl::Worker *create_worker ();
  void setup_worker (tl::Worker *worker);
  void finished ();
  void stopped ();

private:
  void start ();
  void do_start (bool clear, const db::Vector *shift_vector, const std::vector <lay::RedrawLayerInfo> *layers, const std::vector<int> &restart, int workers);
  void done ();

  void layout_changed ();

  void layout_changed_with_int (int)
  {
    layout_changed ();
  }

  bool m_initial_update;
  std::vector <RedrawLayerInfo> m_layers;
  int m_nlayers;
  bool m_boxes_already_drawn;
  bool m_custom_already_drawn;

  db::DCplxTrans m_vp_trans;
  int m_width, m_height;
  double m_resolution;
  std::vector<db::Box> m_redraw_regions;
  db::DBox m_stored_region, m_valid_region;
  db::DPoint m_last_center;
  db::DFTrans m_stored_fp;

  lay::RedrawThreadCanvas *mp_canvas;
  lay::LayoutViewBase *mp_view;
  bool m_start_recursion_sentinel;

  tl::Clock m_clock;
  tl::Mutex m_initial_wait_lock;
  tl::WaitCondition m_initial_wait_cond;

  std::unique_ptr<tl::SelfTimer> m_main_timer;
};

}

#endif

