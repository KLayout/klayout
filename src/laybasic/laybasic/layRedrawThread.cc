
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


#include "layRedrawThread.h"
#include "layRedrawThreadWorker.h"
#include "tlLog.h"
#include "tlAssert.h"
#include "dbHershey.h"
#include "dbShape.h"

#include <memory>

namespace lay 
{

// -------------------------------------------------------------
//  RedrawThread implementation

RedrawThread::RedrawThread (lay::RedrawThreadCanvas *canvas, LayoutViewBase *view)
  : tl::Object ()
{
  m_initial_update = false;
  mp_canvas = canvas;
  mp_view = view;
  m_start_recursion_sentinel = false;
  m_width = 0;
  m_height = 0;
  m_resolution = 1.0;
  m_boxes_already_drawn = false;
  m_custom_already_drawn = false;
  m_nlayers = 0;
  m_clock = tl::Clock::current ();
}

RedrawThread::~RedrawThread ()
{
  // .. nothing yet ..
}

void RedrawThread::layout_changed ()
{
  if (is_running () && tl::verbosity () >= 30) {
    tl::info << tl::to_string (tr ("Layout changed: redraw thread stopped"));
  }

  //  if something changed on the layouts we observe, stop the redraw thread
  stop ();
}

void
RedrawThread::task_finished (int task_id)
{
  //  mark this entry as already drawn.
  //  HINT: that is MT safe given the exclusive nature of the tasks ...
  if (task_id == draw_custom_queue_entry) {
    m_custom_already_drawn = true;
  } else if (task_id == draw_boxes_queue_entry) {
    m_boxes_already_drawn = true;
  } else if (task_id >= 0 && task_id < int (m_layers.size ())) {
    m_layers [task_id].enabled = false;
  }
}

std::vector<db::DBox> 
subtract_box (const db::DBox &subject, const db::DBox &with)
{
  std::vector<db::DBox> res;
  std::vector<db::DBox> inverted;

  double lim = std::numeric_limits<double>::max () * 0.5 /*safety*/;

  inverted.reserve (4);
  inverted.push_back (db::DBox (db::DPoint (-lim, -lim), db::DPoint (lim, with.bottom ())));
  inverted.push_back (db::DBox (db::DPoint (-lim, with.top ()), db::DPoint (lim, lim)));
  inverted.push_back (db::DBox (db::DPoint (-lim, with.bottom ()), db::DPoint (with.left (), with.top ())));
  inverted.push_back (db::DBox (db::DPoint (with.right (), with.bottom ()), db::DPoint (lim, with.top ())));

  for (std::vector<db::DBox>::const_iterator i = inverted.begin (); i != inverted.end (); ++i) {
    db::DBox part = subject & *i;
    if (! part.empty ()) {
      res.push_back (part);
    }
  }

  return res;
}

void
RedrawThread::commit (const std::vector <lay::RedrawLayerInfo> &layers, const lay::Viewport &vp, double resolution)
{
  m_vp_trans = vp.trans ();
  m_width = vp.width ();
  m_height = vp.height ();
  m_resolution = resolution;

  m_layers = layers;
  m_nlayers = int (m_layers.size ());
  for (size_t i = 0; i < m_layers.size (); ++i) {
    if (m_layers [i].visible) {
      m_layers [i].enabled = false;
    }
  }

  db::DBox new_region = m_vp_trans.inverted () * db::DBox (db::DPoint (0, 0), db::DPoint (m_width, m_height));
  m_last_center = new_region.center ();
  m_stored_region = m_valid_region = new_region;
  m_stored_fp = m_vp_trans.fp_trans ();

  m_boxes_already_drawn = false;
  m_custom_already_drawn = false;
}

void
RedrawThread::start (int workers, const std::vector <lay::RedrawLayerInfo> &layers, const lay::Viewport &vp, double resolution, bool force_redraw)
{
  m_vp_trans = vp.trans ();
  m_width = vp.width ();
  m_height = vp.height ();
  m_resolution = resolution;

  db::DBox new_region = m_vp_trans.inverted () * db::DBox (db::DPoint (0, 0), db::DPoint (m_width, m_height));
  double epsilon = m_vp_trans.inverted ().ctrans (1e-3);

  db::Vector sv;
  db::Vector *shift_vector = 0;

  //  test, if we can shift the current image and redraw only the missing parts
  if (! force_redraw && mp_canvas->shift_supported () &&
      m_valid_region.overlaps (new_region) && m_stored_fp == m_vp_trans.fp_trans () && 
      fabs (new_region.width () - m_stored_region.width ()) < epsilon && fabs (new_region.height () - m_stored_region.height ()) < epsilon) {

    db::Box full (db::Point (0, 0), db::Point (m_width, m_height));

    //  yes we can ...
    std::vector<db::DBox> parts = subtract_box (new_region, m_valid_region);
    m_redraw_regions.clear ();
    m_redraw_regions.reserve (parts.size ());
    for (std::vector<db::DBox>::const_iterator p = parts.begin (); p != parts.end (); ++p) {
      if (p->width () > epsilon && p->height () > epsilon) {
        db::Box rr = db::Box ((m_vp_trans * *p).enlarged (db::DVector (1.0, 1.0) /*safety overlap*/));
        rr &= full;
        if (! rr.empty ()) {
          m_redraw_regions.push_back (rr);
        }
      }
    }

    sv = db::Vector (m_vp_trans * db::DVector (m_last_center - new_region.center ()));
    shift_vector = &sv;

  } else {

    //  no, we can't ...
    m_redraw_regions.clear ();
    m_redraw_regions.push_back (db::Box (db::Point (0, 0), db::Point (m_width, m_height)));

    //  mark current image as unusable
    m_valid_region = m_stored_region = db::DBox ();

  }

  m_last_center = new_region.center ();

  std::vector<int> restart;
  do_start (true, shift_vector, &layers, restart, workers);
}

void  
RedrawThread::restart (const std::vector<int> &restart)
{
  m_redraw_regions.clear ();
  m_redraw_regions.push_back (db::Box (db::Point (0, 0), db::Point (m_width, m_height)));
  m_valid_region = m_stored_region = db::DBox ();

  do_start (false, 0, 0, restart, -1);
}

void  
RedrawThread::change_visibility (const std::vector<bool> &visibility)
{
  for (unsigned int i = 0; i < visibility.size () && i < m_layers.size (); ++i) {
    m_layers [i].visible = visibility [i];
  }
}

void 
RedrawThread::do_start (bool clear, const db::Vector *shift_vector, const std::vector <lay::RedrawLayerInfo> *layers, const std::vector<int> &restart, int nworkers)
{
  // change the number of workers if required.
  if (nworkers >= 0 && nworkers != num_workers ()) {
    set_num_workers (nworkers);
  }

  m_initial_update = true;

  //  We need this recursion sentinel because start may be called recursively
  //  via Layout::update which may open a progress widget and do processEvents.
  //  This may result in a repaint for example issuing a "start" call.
  //  Recursion however is forbidden because of the deadlocking locks used originally
  //  to lock out the redraw thread.
  if (m_start_recursion_sentinel) {
    return;
  }
  m_start_recursion_sentinel = true;

  {
    if (tl::verbosity () >= 40) {
      tl::info << tl::to_string (tr ("Preparing to draw"));
    }
    tl::SelfTimer timer (tl::verbosity () >= 41, tl::to_string (tr ("Preparing to draw")));

    //  detach from all layout objects 
    tl::Object::detach_from_all_events ();

    //  Update all relevant layout objects.
    for (unsigned int i = 0; i < mp_view->cellviews (); ++i) {
      const lay::CellView &cv = mp_view->cellview (i);
      if (cv.is_valid () && ! cv->layout ().under_construction () && ! (cv->layout ().manager () && cv->layout ().manager ()->transacting ())) {
        cv->layout ().update ();
        //  attach to the layout object to receive change notifications to stop the redraw thread
        cv->layout ().hier_changed_event.add (this, &RedrawThread::layout_changed);
        cv->layout ().bboxes_changed_any_event.add (this, &RedrawThread::layout_changed);
      }
    }
    mp_view->annotation_shapes ().update ();
    //  attach to the layout object to receive change notifications to stop the redraw thread
    mp_view->annotation_shapes ().hier_changed_event.add (this, &RedrawThread::layout_changed);  //  not really required, since the shapes have no hierarchy, but for completeness ..
    mp_view->annotation_shapes ().bboxes_changed_any_event.add (this, &RedrawThread::layout_changed);
    mp_view->cellviews_about_to_change_event.add (this, &RedrawThread::layout_changed);
    mp_view->cellview_about_to_change_event.add (this, &RedrawThread::layout_changed_with_int);

    m_initial_update = true;

    if (clear) {
      m_layers = *layers;
    }

    m_nlayers = int (m_layers.size ());

    if (mp_view->cellviews () > 0) {

      if (clear) {

        mp_canvas->prepare (m_nlayers * planes_per_layer + special_planes_before + special_planes_after, m_width, m_height, m_resolution, shift_vector, 0, mp_view->drawings ());
        m_boxes_already_drawn = false;
        m_custom_already_drawn = false;

      } else {

        //  determine the planes to initialize
        std::vector<int> planes_to_init;
        for (std::vector<int>::const_iterator l = restart.begin (); l != restart.end (); ++l) {
          if (*l == draw_custom_queue_entry) {
            planes_to_init.push_back (-1); 
          } else if (*l >= 0 && *l < int (m_layers.size ())) {
            for (int i = 0; i < planes_per_layer / 3; ++i) {
              planes_to_init.push_back (*l * (planes_per_layer / 3) + special_planes_before + i);
              planes_to_init.push_back ((*l + m_nlayers) * (planes_per_layer / 3) + special_planes_before + i);
              planes_to_init.push_back ((*l + m_nlayers * 2) * (planes_per_layer / 3) + special_planes_before + i);
            }
          }
        }

        mp_canvas->prepare (m_nlayers * planes_per_layer + special_planes_before + special_planes_after, m_width, m_height, m_resolution, shift_vector, &planes_to_init, mp_view->drawings ());

        for (std::vector<int>::const_iterator l = restart.begin (); l != restart.end (); ++l) {
          if (*l >= 0 && *l < int (m_layers.size ())) {
            m_layers [*l].enabled = true;
          } else if (*l == draw_boxes_queue_entry) {
            m_boxes_already_drawn = false;
          } else if (*l == draw_custom_queue_entry) {
            m_custom_already_drawn = false;
          }
        }
      }

      //  set up the drawing tasks: controls the sequence of things to 
      //  draw. First there are the visible layers, then there are the
      //  cell boundaries. Then come the invisible layers.

      //  decoration drawing
      if (! m_custom_already_drawn) {
        schedule (new RedrawThreadTask (draw_custom_queue_entry));
      }

      for (int i = 0; i < m_nlayers; ++i) {
        if (m_layers [i].needs_drawing ()) {
          schedule (new RedrawThreadTask (i));
        }
      }

      //  cell box drawing
      if (! m_boxes_already_drawn) {
        schedule (new RedrawThreadTask (draw_boxes_queue_entry));
      }

    } else {
      mp_canvas->prepare (1, m_width, m_height, m_resolution, 0, 0, mp_view->drawings ());
    }

  }

  if (tl::verbosity () >= 21) {
    m_main_timer.reset (new tl::SelfTimer ("Redrawing"));
  }

  start ();

  m_initial_wait_lock.lock ();
  //  Don't wait on restart - that happens while a drawing is under way which was interrupted.
  //  Waiting is not necessary in this case and blocks the application.
  if (m_initial_update && clear) {
    m_initial_wait_cond.wait (&m_initial_wait_lock);
  }
  m_initial_update = false;
  m_initial_wait_lock.unlock ();

  m_start_recursion_sentinel = false;
}

void
RedrawThread::start ()
{
  JobBase::start ();
}

tl::Worker *
RedrawThread::create_worker ()
{
  return new RedrawThreadWorker (this);
}

void 
RedrawThread::setup_worker (tl::Worker *worker)
{
  RedrawThreadWorker *redraw_thread_worker = dynamic_cast<RedrawThreadWorker *> (worker);
  if (redraw_thread_worker) {
    redraw_thread_worker->setup (mp_view, mp_canvas, m_redraw_regions, m_vp_trans);
  }
}

void
RedrawThread::stopped ()
{
  // because we may have already shifted, we can only reuse the part that was inside the new viewport
  m_stored_region = m_vp_trans.inverted () * db::DBox (db::DPoint (0, 0), db::DPoint (m_width, m_height));
  m_valid_region &= m_stored_region;
  m_stored_fp = m_vp_trans.fp_trans ();

  done ();
}

void
RedrawThread::done ()
{
  //  stop timer if there is one 
  m_main_timer.reset (0);

  wakeup ();

  //  release the workers' resources
  for (int i = 0; i < num_workers (); ++i) {
    RedrawThreadWorker *w = dynamic_cast <RedrawThreadWorker *> (worker (i));
    if (w) {
      w->finish ();
    }
  }

  //  send a signal to the canvas that the drawing has finished
  mp_canvas->signal_end_of_drawing ();
}

void
RedrawThread::finished ()
{
  m_stored_region = m_valid_region = m_vp_trans.inverted () * db::DBox (db::DPoint (0, 0), db::DPoint (m_width, m_height));
  m_stored_fp = m_vp_trans.fp_trans ();

  done ();
}

void
RedrawThread::wakeup_checked ()
{
  tl::Clock c = tl::Clock::current ();
  if ((c - m_clock).seconds () > update_interval * 0.8 * 0.001) {
    m_clock = c;
    wakeup ();
  }
}

void
RedrawThread::wakeup ()
{
  bool send_event = false;

  //  wakeup the main thread on the initial update ..
  m_initial_wait_lock.lock ();
  if (m_initial_update) {
    m_initial_wait_cond.wakeAll ();
    m_initial_update = false;
  } else {
    send_event = true;
  }
  m_initial_wait_lock.unlock ();

  //  otherwise post an event to actually draw
  if (send_event) {
    mp_canvas->signal_transfer_done ();
  }
}

} // namespace lay
