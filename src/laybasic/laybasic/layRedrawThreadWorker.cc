
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


#include "layRedrawThreadWorker.h"
#include "layRedrawThread.h"
#include "layBitmap.h"

namespace lay
{

//  time delay until the first snapshot is taken
const int first_snapshot_delay = 20;

// -------------------------------------------------------------

static inline db::Box safe_transformed_box (const db::Box &box, const db::ICplxTrans &t)
{
  db::DBox db = db::CplxTrans (t) * box;
  db &= db::DBox (db::Box::world ());
  return db::Box (db);
}

// -------------------------------------------------------------
//  RedrawThreadWorker implementation 

RedrawThreadWorker::RedrawThreadWorker (RedrawThread *redraw_thread)
  : mp_redraw_thread (redraw_thread)
{
  mp_layout = 0;
  mp_cell_var_cache = 0;
  m_cache_hits = 0;
  m_cache_misses = 0;
  m_cv_index = -1;
  mp_canvas = 0;
  m_test_count = 0;
  m_from_level = 0;
  m_to_level = 0;
  m_from_level_default = 0;
  m_to_level_default = 0;
  m_nlayers = 0;
  m_box_text_transform = false;
  m_box_font = 0;
  m_min_size_for_label = 1;
  m_text_font = 0;
  m_text_visible = false;
  m_text_lazy_rendering = false;
  m_bitmap_caching = false;
  m_show_properties = false;
  m_apply_text_trans = false;
  m_default_text_size = 0.0;
  m_drop_small_cells = false;
  m_drop_small_cells_value = 0;
  m_drop_small_cells_cond = lay::LayoutViewBase::DSC_Min;
  m_draw_array_border_instances = false;
  m_abstract_mode_width = 0;
  m_child_context_enabled = false;
  m_layer = 0;
  m_xfill = false;
  mp_prop_sel = 0;
  m_inv_prop_sel = false;
  m_clock = tl::Clock::current ();

  for (unsigned int i = 0; i < sizeof (m_planes) / sizeof (m_planes[0]); ++i) {
    m_planes[i] = 0;
  }
}

RedrawThreadWorker::~RedrawThreadWorker ()
{
  for (unsigned int i = 0; i < sizeof (m_planes) / sizeof (m_planes[0]); ++i) {
    if (m_planes[i]) {
      delete m_planes[i];
      m_planes[i] = 0;
    }
  }
}

void 
RedrawThreadWorker::perform_task (tl::Task *task)
{
  RedrawThreadTask *redraw_thread_task = dynamic_cast <RedrawThreadTask *> (task);
  if (! redraw_thread_task) {
    return;
  }

  m_cell_cache.clear ();
  m_mi_cache.clear ();
  m_mi_text_cache.clear ();

  m_from_level = m_from_level_default;
  m_to_level = m_to_level_default;

  int task_id = redraw_thread_task->id ();

  if (task_id >= 0) {

    //  draw a layer

    //  HINT: the order in which the planes are delivered (the index stored in the first member of the pair below)
    //  must correspond with the order by which the ViewOp's are created inside LayoutView::set_view_ops
    m_buffers.clear ();
    for (unsigned int i = 0; i < (unsigned int) planes_per_layer / 3; ++i) {

      //  context level planes
      unsigned int i1 = task_id * (planes_per_layer / 3) + special_planes_before + i;
      mp_canvas->initialize_plane (m_planes[i], i1); 
      m_buffers.push_back (std::make_pair (i1, m_planes [i]));

      //  child level planes (if used)
      unsigned int i2 = (task_id + m_nlayers) * (planes_per_layer / 3) + special_planes_before + i;
      mp_canvas->initialize_plane (m_planes [i + planes_per_layer / 3], i2); 
      m_buffers.push_back (std::make_pair (i2, m_planes [i + planes_per_layer / 3]));

      //  current level planes
      unsigned int i3 = (task_id + m_nlayers * 2) * (planes_per_layer / 3) + special_planes_before + i;
      mp_canvas->initialize_plane (m_planes [i + 2 * (planes_per_layer / 3)], i3); 
      m_buffers.push_back (std::make_pair (i3, m_planes [i + 2 * (planes_per_layer / 3)]));

    }

    //  detect whether the text planes are empty. If not, the whole text plane must be redrawn to account for clipped texts
    bool text_planes_empty = true;
    for (unsigned int i = 0; i < (unsigned int) planes_per_layer && text_planes_empty; i += (unsigned int) planes_per_layer / 3) {
      lay::Bitmap *text = dynamic_cast<lay::Bitmap *> (m_planes[i + 2]);
      if (text && ! text->empty ()) {
        text_planes_empty = false;
      }
    }

    std::vector<db::Box> text_redraw_regions = m_redraw_region;
    if (! text_planes_empty) {
      //  if there are non-empty text planes, redraw the whole area for texts
      text_redraw_regions.clear ();
      text_redraw_regions.push_back(db::Box(0, 0, mp_canvas->canvas_width (), mp_canvas->canvas_height ()));
      for (unsigned int i = 0; i < (unsigned int) planes_per_layer; i += (unsigned int) planes_per_layer / 3) {
        lay::Bitmap *text = dynamic_cast<lay::Bitmap *> (m_planes[i + 2]);
        if (text) {
          text->clear ();
        }
      }
    }

    const RedrawLayerInfo &li = mp_redraw_thread->get_layer_info (task_id);

    if (li.cellview_index >= 0) {

      //  determine layout and cell associated with this layer ..
      const lay::CellView &cv = m_cellviews [li.cellview_index];
      if (cv.is_valid () && ! cv->layout ().under_construction () && ! (cv->layout ().manager () && cv->layout ().manager ()->transacting ())) {

        mp_layout = &cv->layout ();
        m_cv_index = li.cellview_index;
        db::cell_index_type ci = cv.cell_index ();

        int ctx_path_length = int (m_cellviews [m_cv_index].specific_path ().size ());

        if (li.hier_levels.has_from_level ()) {
          m_from_level = li.hier_levels.from_level (ctx_path_length, m_from_level);
        }
        if (li.hier_levels.has_to_level ()) {
          m_to_level = li.hier_levels.to_level (ctx_path_length, m_to_level);
        }

        m_xfill = li.xfill;

        mp_prop_sel = &li.prop_sel;
        m_inv_prop_sel = li.inverse_prop_sel;
        if (mp_prop_sel->empty () && m_inv_prop_sel) {
          //  no property selection
          mp_prop_sel = 0;
        }

        if (li.layer_index >= 0) {

          m_layer = li.layer_index;
       
          if (tl::verbosity () >= 40) {
            tl::info << tl::to_string (tr ("Drawing layer: ")) << mp_layout->get_properties (m_layer).name;
          }
          tl::SelfTimer timer (tl::verbosity () >= 41, tl::to_string (tr ("Drawing layer")));

          //  configure renderer ..
          mp_renderer->set_xfill (m_xfill);
          mp_renderer->draw_texts (m_text_visible);
          mp_renderer->draw_properties (m_show_properties);
          mp_renderer->draw_description_property (false);
          mp_renderer->default_text_size (db::Coord (m_default_text_size / mp_layout->dbu ()));
          mp_renderer->set_font (db::Font (m_text_font));
          mp_renderer->apply_text_trans (m_apply_text_trans);

          for (std::vector<db::DCplxTrans>::const_iterator t = li.trans.begin (); t != li.trans.end (); ++t) {
            db::CplxTrans trans = m_vp_trans * *t * db::CplxTrans (mp_layout->dbu ());
            iterate_variants (m_redraw_region, ci, trans, &RedrawThreadWorker::draw_layer);
            iterate_variants (text_redraw_regions, ci, trans, &RedrawThreadWorker::draw_text_layer);
          }

        } else if (li.cell_frame) {

          //  no xfill for cell boxes
          mp_renderer->set_xfill (false);

          //  if no specific layer is assigned, draw cell boxes with the style given
          if (tl::verbosity () >= 40) {
            tl::info << tl::to_string (tr ("Drawing custom frames"));
          }
          tl::SelfTimer timer (tl::verbosity () >= 41, tl::to_string (tr ("Drawing frames")));

          for (std::set< std::pair<db::DCplxTrans, int> >::const_iterator b = m_box_variants.begin (); b != m_box_variants.end (); ++b) {
            if (b->second == li.cellview_index) {
              db::CplxTrans trans = m_vp_trans * b->first * db::CplxTrans (mp_layout->dbu ());
              iterate_variants (m_redraw_region, ci, trans, &RedrawThreadWorker::draw_boxes);
              iterate_variants (text_redraw_regions, ci, trans, &RedrawThreadWorker::draw_box_properties);
            }
          }

        }

        mp_prop_sel = 0;
        m_inv_prop_sel = false;

      }

    }

  } else if (task_id == draw_boxes_queue_entry) {

    //  draw the bounding boxes
    if (tl::verbosity () >= 40) {
      tl::info << tl::to_string (tr ("Drawing frames and guiding shapes"));
    }
    tl::SelfTimer timer (tl::verbosity () >= 41, tl::to_string (tr ("Drawing frames and guiding shapes")));

    //  No xfill for cell boxes
    mp_renderer->set_xfill (false);

    //  HINT: the order in which the planes are delivered (the index stored in the first member of the pair below)
    //  must correspond with the order by which the ViewOp's are created inside LayoutView::set_view_ops
    m_buffers.clear ();
    for (unsigned int i = 0; i < (unsigned int) planes_per_layer / 3; ++i) {

      //  context level planes
      unsigned int i1 = i;
      mp_canvas->initialize_plane (m_planes[i], i1); 
      m_buffers.push_back (std::make_pair (i1, m_planes [i]));

      //  child level planes (if used)
      unsigned int i2 = i + planes_per_layer / 3;
      mp_canvas->initialize_plane (m_planes [i + planes_per_layer / 3], i2); 
      m_buffers.push_back (std::make_pair (i2, m_planes [i + planes_per_layer / 3]));

      //  current level planes
      unsigned int i3 = i + 2 * (planes_per_layer / 3);
      mp_canvas->initialize_plane (m_planes [i + 2 * (planes_per_layer / 3)], i3); 
      m_buffers.push_back (std::make_pair (i3, m_planes [i + 2 * (planes_per_layer / 3)]));

    }

    //  detect whether the text planes are empty. If not, the whole text plane must be redrawn to account for clipped texts
    bool text_planes_empty = true;
    for (unsigned int i = 0; i < (unsigned int) planes_per_layer && text_planes_empty; i += (unsigned int) planes_per_layer / 3) {
      lay::Bitmap *text = dynamic_cast<lay::Bitmap *> (m_planes[i + 2]);
      if (text && ! text->empty ()) {
        text_planes_empty = false;
      }
    }

    std::vector<db::Box> text_redraw_regions = m_redraw_region;
    if (! text_planes_empty) {
      //  if there are non-empty text planes, redraw the whole area for texts
      text_redraw_regions.clear ();
      text_redraw_regions.push_back(db::Box(0, 0, mp_canvas->canvas_width (), mp_canvas->canvas_height ()));
      for (unsigned int i = 0; i < (unsigned int) planes_per_layer; i += (unsigned int) planes_per_layer / 3) {
        lay::Bitmap *text = dynamic_cast<lay::Bitmap *> (m_planes[i + 2]);
        if (text) {
          text->clear ();
        }
      }
    }

    for (std::set< std::pair<db::DCplxTrans, int> >::const_iterator b = m_box_variants.begin (); b != m_box_variants.end (); ++b) {

      const lay::CellView &cv = m_cellviews [b->second];
      if (cv.is_valid () && ! cv->layout ().under_construction () && ! (cv->layout ().manager () && cv->layout ().manager ()->transacting ())) {

        mp_layout = &cv->layout ();
        m_cv_index = b->second;

        db::CplxTrans trans = m_vp_trans * b->first * db::CplxTrans (mp_layout->dbu ());

        iterate_variants (m_redraw_region, cv.cell_index (), trans, &RedrawThreadWorker::draw_boxes);
        iterate_variants (text_redraw_regions, cv.cell_index (), trans, &RedrawThreadWorker::draw_box_properties);

      }

    }

    transfer ();

    //  HINT: the order in which the planes are delivered (the index stored in the first member of the pair below)
    //  must correspond with the order by which the ViewOp's are created inside LayoutView::set_view_ops
    m_buffers.clear ();
    for (unsigned int i = 0; i < (unsigned int) planes_per_layer / 3; ++i) {

      //  context level planes
      unsigned int i1 = cell_box_planes + i;
      mp_canvas->initialize_plane (m_planes[i], i1); 
      m_buffers.push_back (std::make_pair (i1, m_planes [i]));

      //  child level planes (if used)
      unsigned int i2 = cell_box_planes + i + planes_per_layer / 3;
      mp_canvas->initialize_plane (m_planes [i + planes_per_layer / 3], i2); 
      m_buffers.push_back (std::make_pair (i2, m_planes [i + planes_per_layer / 3]));

      //  current level planes
      unsigned int i3 = cell_box_planes + i + 2 * (planes_per_layer / 3);
      mp_canvas->initialize_plane (m_planes [i + 2 * (planes_per_layer / 3)], i3); 
      m_buffers.push_back (std::make_pair (i3, m_planes [i + 2 * (planes_per_layer / 3)]));

    }

    //  detect whether the text planes are empty. If not, the whole text plane must be redrawn to account for clipped texts
    text_planes_empty = true;
    for (unsigned int i = 0; i < (unsigned int) planes_per_layer && text_planes_empty; i += (unsigned int) planes_per_layer / 3) {
      lay::Bitmap *text = dynamic_cast<lay::Bitmap *> (m_planes[i + 2]);
      if (text && ! text->empty ()) {
        text_planes_empty = false;
      }
    }

    text_redraw_regions = m_redraw_region;
    if (! text_planes_empty) {
      //  if there are non-empty text planes, redraw the whole area for texts
      text_redraw_regions.clear ();
      text_redraw_regions.push_back(db::Box(0, 0, mp_canvas->canvas_width (), mp_canvas->canvas_height ()));
      for (unsigned int i = 0; i < (unsigned int) planes_per_layer; i += (unsigned int) planes_per_layer / 3) {
        lay::Bitmap *text = dynamic_cast<lay::Bitmap *> (m_planes[i + 2]);
        if (text) {
          text->clear ();
        }
      }
    }

    //  draw the guiding and error shapes
    for (std::set< std::pair<db::DCplxTrans, int> >::const_iterator b = m_box_variants.begin (); b != m_box_variants.end (); ++b) {

      const lay::CellView &cv = m_cellviews [b->second];
      if (cv.is_valid () && ! cv->layout ().under_construction () && ! (cv->layout ().manager () && cv->layout ().manager ()->transacting ())) {

        mp_layout = &cv->layout ();
        m_cv_index = b->second;

        db::CplxTrans trans = m_vp_trans * b->first * db::CplxTrans (mp_layout->dbu ());
        mp_prop_sel = 0;
        m_inv_prop_sel = false;
        //  draw one level more to show the guiding shapes as part of the instance
        m_to_level += 1; //  TODO: modifying this basic setting is a hack!

        //  configure renderer ..
        mp_renderer->draw_texts (m_text_visible);
        mp_renderer->draw_properties (false);
        mp_renderer->draw_description_property (true);
        mp_renderer->default_text_size (db::Coord (m_default_text_size / mp_layout->dbu ()));
        mp_renderer->set_font (db::Font (m_text_font));
        mp_renderer->apply_text_trans (m_apply_text_trans);

        bool f = m_text_lazy_rendering;

        try {

          m_text_lazy_rendering = false;

          m_layer = mp_layout->guiding_shape_layer ();
          iterate_variants (m_redraw_region, cv.cell_index (), trans, &RedrawThreadWorker::draw_layer);
          iterate_variants (text_redraw_regions, cv.cell_index (), trans, &RedrawThreadWorker::draw_text_layer);

          m_layer = mp_layout->error_layer ();
          iterate_variants (m_redraw_region, cv.cell_index (), trans, &RedrawThreadWorker::draw_layer);
          iterate_variants (text_redraw_regions, cv.cell_index (), trans, &RedrawThreadWorker::draw_text_layer);

          m_text_lazy_rendering = f;
          m_to_level -= 1;

        } catch (...) {

          m_text_lazy_rendering = f;
          m_to_level -= 1;

          throw;

        }

      }

    }

  } else if (task_id == draw_custom_queue_entry) {

    //  draw the decorations
    if (tl::verbosity () >= 40) {
      tl::info << tl::to_string (tr ("Drawing decorations"));
    }
    tl::SelfTimer timer (tl::verbosity () >= 41, tl::to_string (tr ("Drawing decorations")));

    m_buffers.clear ();
    mp_canvas->initialize_plane (m_planes[0], m_nlayers * planes_per_layer + special_planes_before); 
    m_buffers.push_back (std::make_pair (m_nlayers * planes_per_layer + special_planes_before, m_planes [0]));

    unsigned int nd = 0;
    for (std::vector <lay::Drawing *>::iterator d = mp_drawings.begin (); d != mp_drawings.end (); ++d, ++nd) {

      //  temporarily create bitmap objects, paint on them,
      //  transfer them to the canvas and clear them again.
      //  This operation is not interrupted by any "test_snapshot".

      std::vector <lay::CanvasPlane *> tmp_planes;
      tmp_planes.reserve ((*d)->num_planes ());
      for (unsigned int i = 0; i < (*d)->num_planes (); ++i) {
        tmp_planes.push_back (mp_canvas->create_drawing_plane ());
        mp_canvas->initialize_plane (tmp_planes.back (), nd, i); 
      }

      //  currently, all cellviews are painted over each other ..
      for (unsigned int i = 0; i < m_cellviews.size (); ++i) {
        test_snapshot (0); 
        const lay::CellView &cv = m_cellviews [i];
        if (cv.is_valid () && ! cv->layout ().under_construction () && ! (cv->layout ().manager () && cv->layout ().manager ()->transacting ())) {
          db::CplxTrans trans (m_vp_trans * cv->layout ().dbu ());
          (*d)->paint_cv_on_planes (cv, trans, tmp_planes);
        }
      }

      //  do the non-cv-related painting
      test_snapshot (0); 
      (*d)->paint_on_planes (m_vp_trans, tmp_planes, *mp_renderer);

      for (unsigned int i = 0; i < (*d)->num_planes (); ++i) {
        mp_canvas->set_drawing_plane (nd, i, tmp_planes [i]); 
      }

      while (! tmp_planes.empty ()) {
        delete tmp_planes.back ();
        tmp_planes.pop_back ();
      }

    }

  }

  transfer ();
  m_buffers.clear ();

  if (tl::verbosity () >= 30) {
    for (cell_cache_t::iterator cc = m_cell_cache.begin(); cc != m_cell_cache.end (); ++cc) {
      tl::info << "Cell cache: " << mp_layout->cell_name(cc->first.ci) << " (" << cc->first.nlevels << ":" << cc->first.trans.to_string() << ") " 
               << cc->second.fill->width() << " x " << cc->second.fill->height() << " -> " << cc->second.hits << " hits";
    }
  }

  m_cell_cache.clear ();

  mp_redraw_thread->task_finished (task_id);
}

void 
RedrawThreadWorker::finish ()
{
  //  release all cellview references here.
  m_cellviews.clear ();

  //  free the planes
  for (unsigned int i = 0; i < sizeof (m_planes) / sizeof (m_planes[0]); ++i) {
    if (m_planes[i]) {
      delete m_planes[i];
      m_planes[i] = 0;
    }
  }
}

void
RedrawThreadWorker::setup (LayoutViewBase *view, RedrawThreadCanvas *canvas, const std::vector<db::Box> &redraw_region, const db::DCplxTrans &vp_trans)
{
  m_redraw_region = redraw_region;
  m_vp_trans = vp_trans;

  mp_canvas = canvas;

  mp_drawings.clear ();
  for (lay::Drawings::iterator d = view->drawings ()->begin (); d != view->drawings ()->end (); ++d) {
    mp_drawings.push_back (&*d);
  }

  //  allow a very short time to pass before we issue the
  //  first update event.
  m_clock = tl::Clock::current () - tl::Clock ((update_interval - first_snapshot_delay) * 0.001);

  //  initialize the drawing planes
  for (unsigned int i = 0; i < (unsigned int) planes_per_layer; ++i) {
    if (m_planes[i]) {
      delete m_planes[i];
    }
    m_planes[i] = mp_canvas->create_drawing_plane ();
  }

  mp_renderer.reset (mp_canvas->create_renderer ());

  //  copy everything that we need so there is no need to access 
  //  members of lay::LayoutView in the drawing thread.
  //  Note: copying the cellviews will create new references to the
  //  layout objects. These are not automatically freed when the 
  //  drawing ends but rather on "stop". The advantage of this is
  //  that, since "stop" is called from the main thread like "start",
  //  we don't challenge lay::CellView's MT compliance.
  m_from_level_default = view->get_hier_levels ().first;
  m_to_level_default = view->get_hier_levels ().second;
  m_min_size_for_label = view->min_inst_label_size ();
  m_box_text_transform = view->cell_box_text_transform ();
  m_box_font = view->cell_box_text_font ();
  m_text_font = view->text_font ();
  m_text_visible = view->text_visible ();
  m_text_lazy_rendering = view->text_lazy_rendering ();
  m_bitmap_caching = view->bitmap_caching ();
  m_show_properties = view->show_properties_as_text ();
  m_apply_text_trans = view->apply_text_trans ();
  m_default_text_size = view->default_text_size ();
  m_drop_small_cells = view->drop_small_cells ();
  m_drop_small_cells_value = view->drop_small_cells_value ();
  m_drop_small_cells_cond = view->drop_small_cells_cond ();
  m_draw_array_border_instances = view->draw_array_border_instances ();
  m_abstract_mode_width = view->abstract_mode_width ();
  m_child_context_enabled = view->child_context_enabled ();
  m_test_count = 0;

  mp_prop_sel = 0;
  m_inv_prop_sel = false;

  m_hidden_cells = view->hidden_cells ();

  m_cellviews.clear ();
  m_cellviews.reserve (view->cellviews ());
  for (unsigned int i = 0; i < view->cellviews (); ++i) {
    m_cellviews.push_back (view->cellview (i));
  }

  m_nlayers = mp_redraw_thread->num_layers (); 

  m_box_variants = view->cv_transform_variants ();
}

void
RedrawThreadWorker::transfer ()
{
  for (std::vector<std::pair<unsigned int, lay::CanvasPlane *> >::iterator b = m_buffers.begin (); b != m_buffers.end (); ++b) {
    mp_canvas->set_plane (b->first, b->second);
  }
}

void 
RedrawThreadWorker::test_snapshot (const UpdateSnapshotCallback *update_snapshot)
{
  checkpoint ();

  if (mp_redraw_thread->num_workers () > 0) {
    if (m_test_count == 0) {

      m_test_count = 100; // TODO: make variable?

      tl::Clock c = tl::Clock::current ();
      if ((c - m_clock).seconds () > update_interval * 0.001) {
        if (update_snapshot) {
          update_snapshot->trigger ();
        }
        transfer ();
        mp_redraw_thread->wakeup_checked ();
        m_clock = c;
      }

    } else {
      --m_test_count;
    }
  }
}

void 
RedrawThreadWorker::draw_cell (bool drawing_context, int level, const db::CplxTrans &trans, const db::Box &box, const std::string &txt)
{
  lay::Renderer &r = *mp_renderer;

  unsigned int plane_group = 2;
  if (drawing_context) {
    plane_group = 0;
  } else if (m_child_context_enabled && level > 0) {
    plane_group = 1;
  }

  db::DBox dbox = trans * box;

  lay::CanvasPlane *fill     = m_planes[0 + plane_group * (planes_per_layer / 3)];
  lay::CanvasPlane *contour  = m_planes[1 + plane_group * (planes_per_layer / 3)];

  r.draw (box, trans, fill, contour, 0, 0);

  if (! txt.empty () && dbox.width () > m_min_size_for_label && dbox.height () > m_min_size_for_label) {
    //  Hint: we render to contour because the texts plane is reserved for properties
    r.draw (dbox, txt, 
            db::Font (m_box_font), 
            db::HAlignCenter, 
            db::VAlignCenter, 
            //  TODO: apply "real" transformation?
            db::DFTrans (m_box_text_transform ? trans.fp_trans ().rot () : db::DFTrans::r0), 0, 0, 0, contour);
  }
}

void 
RedrawThreadWorker::draw_cell_properties (bool drawing_context, int level, const db::CplxTrans &trans, const db::Box &box, db::properties_id_type prop_id)
{
  if (prop_id == 0 || ! m_show_properties) {
    return;
  }

  lay::Renderer &r = *mp_renderer;

  unsigned int plane_group = 2;
  if (drawing_context) {
    plane_group = 0;
  } else if (m_child_context_enabled && level > 0) {
    plane_group = 1;
  }

  lay::CanvasPlane *texts = m_planes[2 + plane_group * (planes_per_layer / 3)];

  r.draw_propstring (prop_id, &mp_layout->properties_repository (), (trans * box).p1 (), texts, trans);
}

static bool
cells_in (const db::Layout *layout, const db::Cell &cell, 
          const std::set <db::cell_index_type> &selected,
          int levels,
          std::set <std::pair <int, db::cell_index_type> > &cache)
{
  if (selected.find (cell.cell_index ()) != selected.end ()) {
    return true;
  }
  if (levels > 0) {
    for (db::Cell::child_cell_iterator c = cell.begin_child_cells (); ! c.at_end (); ++c) {
      if (cache.find (std::make_pair (levels, *c)) == cache.end ()) {
        if (cells_in (layout, layout->cell (*c), selected, levels - 1, cache)) {
          return true;
        }
        cache.insert (std::make_pair (levels, *c));
      }
    }
  } 
  return false;
}

static bool 
need_draw_box (const db::Layout *layout, const db::Cell &cell, 
               int level, int to_level, 
               const std::vector <std::set <db::cell_index_type> > &hidden_cells, unsigned int cv_index) 
{
  if (level > to_level) {
    return false;
  }
  if (hidden_cells.size () > cv_index && ! hidden_cells [cv_index].empty ()) {
    std::set <std::pair <int, db::cell_index_type> > cache;
    if (cells_in (layout, cell, hidden_cells [cv_index], to_level - level, cache)) {
      return true;
    }
  }
  return int (cell.hierarchy_levels ()) + level >= to_level;
}

void
RedrawThreadWorker::draw_boxes (bool drawing_context, db::cell_index_type ci, const db::CplxTrans &trans, const std::vector<db::Box> &redraw_regions, int level)
{
  //  do not draw, if there is nothing to draw
  if (mp_layout->cells () <= ci || redraw_regions.empty ()) {
    return;
  }

  const db::Cell &cell = mp_layout->cell (ci);

  //  we will never come to a valid level ..
  if (! need_draw_box (mp_layout, cell, level, m_to_level, m_hidden_cells, m_cv_index)) {
    return;
  }
  if (cell_var_cached (ci, trans)) {
    return;
  }

  for (std::vector<db::Box>::const_iterator b = redraw_regions.begin (); b != redraw_regions.end (); ++b) {
    draw_boxes (drawing_context, ci, trans, *b, level);
  }
}

void
RedrawThreadWorker::draw_boxes (bool drawing_context, db::cell_index_type ci, const db::CplxTrans &trans, const db::Box &redraw_box, int level)
{
  lay::Renderer &r = *mp_renderer;
  const db::Cell &cell = mp_layout->cell (ci);

  //  For small bboxes, the cell outline can be reduced ..
  db::Box bbox = cell.bbox ();

  if (bbox.empty ()) {

    //  no shapes there and below ..

  } else if (m_drop_small_cells && drop_cell (cell, trans)) {

    //  small cell dropped

  } else if (level == m_to_level || (m_cv_index < int (m_hidden_cells.size ()) && m_hidden_cells [m_cv_index].find (ci) != m_hidden_cells [m_cv_index].end ())) {

    //  paint the box on this level
    draw_cell (drawing_context, level, trans, bbox, mp_layout->display_name (ci));

  } else if (level < m_to_level) {

    db::DBox dbbox = trans * bbox;
    if (dbbox.width () < 1.5 && dbbox.height () < 1.5) {

      if (need_draw_box (mp_layout, cell, level, m_to_level, m_hidden_cells, m_cv_index)) {
        //  the cell is a very small box and we know there must be
        //  some level at which to draw the boundary: just draw it
        //  here and stop looking further down ..
        draw_cell (drawing_context, level, trans, bbox, std::string ());
      }

    } else {

      db::box_convert <db::CellInst> bc (*mp_layout);

      //  create a set of boxes to look into
      db::Coord aw = db::coord_traits<db::Coord>::rounded (m_abstract_mode_width / mp_layout->dbu ());
      std::vector<db::Box> vv;
      if (level == 1 && m_abstract_mode_width > 0 && bbox.width () > db::Box::distance_type (aw * 2) && bbox.height () > db::Box::distance_type (aw * 2)) {
        vv.reserve (4);
        vv.push_back (redraw_box & db::Box (bbox.left (), bbox.bottom (), bbox.left () + aw, bbox.top ()));
        vv.push_back (redraw_box & db::Box (bbox.right () - aw, bbox.bottom (), bbox.right (), bbox.top ()));
        vv.push_back (redraw_box & db::Box (bbox.left () + aw, bbox.bottom (), bbox.right () - aw, bbox.bottom () + aw));
        vv.push_back (redraw_box & db::Box (bbox.left () + aw, bbox.top () - aw, bbox.right () - aw, bbox.top ()));
      } else {
        vv.reserve (1);
        vv.push_back (redraw_box);
      }

      //  dive down into the hierarchy ..
      for (std::vector<db::Box>::const_iterator v = vv.begin (); v != vv.end (); ++v) {

        if (! v->empty ()) {

          bool anything = false;
          db::cell_index_type last_ci = std::numeric_limits<db::cell_index_type>::max ();

          db::Cell::touching_iterator inst = cell.begin_touching (*v);
          while (! inst.at_end ()) {

            const db::CellInstArray &cell_inst = inst->cell_inst ();

            db::cell_index_type new_ci = cell_inst.object ().cell_index ();
            db::Box new_cell_box = mp_layout->cell (new_ci).bbox ();

            if (last_ci != new_ci) {
              //  Hint: don't use any_cell_box on partially visible cells because that will degrade performance
              if (new_cell_box.inside (*v)) {
                last_ci = new_ci;
                anything = any_cell_box (new_ci, m_to_level - (level + 1));
              } else {
                anything = true;
              }
            }

            if (anything) {

              db::Vector a, b;
              unsigned long amax, bmax;
              bool simplify = false;
              if (cell_inst.is_regular_array (a, b, amax, bmax)) {

                db::DBox inst_box;
                if (cell_inst.is_complex ()) {
                  inst_box = trans * (cell_inst.complex_trans () * new_cell_box);
                } else {
                  inst_box = trans * new_cell_box;
                }
                if (((a.x () == 0 && b.y () == 0) || (a.y () == 0 && b.x () == 0)) &&
                    inst_box.width () < 1.5 && inst_box.height () < 1.5 &&
                    (amax <= 1 || trans.ctrans (a.length ()) < 1.5) &&
                    (bmax <= 1 || trans.ctrans (b.length ()) < 1.5)) {
                  simplify = true;
                }

              }

              if (simplify) {

                //  The array can be simplified if there are levels below to draw
                if (need_draw_box (mp_layout, mp_layout->cell (new_ci), level + 1, m_to_level, m_hidden_cells, m_cv_index)) {

                  db::box_convert <db::CellInst> bc (*mp_layout);

                  unsigned int plane_group = 2;
                  if (drawing_context) {
                    plane_group = 0;
                  } else if (m_child_context_enabled && level + 1 > 0) {
                    plane_group = 1;
                  }

                  lay::CanvasPlane *contour  = m_planes[1 + plane_group * (planes_per_layer / 3)];
                  r.draw (cell_inst.bbox (bc), trans, contour, 0, 0, 0);

                }

              } else {

                size_t qid = 0;

                //  The array (or single instance) must be iterated instance by instance
                for (db::CellInstArray::iterator p = cell_inst.begin_touching (*v, bc); ! p.at_end (); ) {

                  test_snapshot (0);
                  db::ICplxTrans t (cell_inst.complex_trans (*p));
                  db::Box new_vp = safe_transformed_box (*v, t.inverted ());
                  draw_boxes (drawing_context, new_ci, trans * t, new_vp, level + 1);

                  if (p.quad_id () > 0 && p.quad_id () != qid) {

                    qid = p.quad_id ();

                    //  if the quad is very small we don't gain anything from looking further into the quad - skip this one
                    db::DBox qb = trans * cell_inst.quad_box (p, bc);
                    if (qb.width () < 1.0 && qb.height () < 1.0) {
                      p.skip_quad ();
                      continue;
                    }

                  }

                  ++p;

                }

              }

            }

            ++inst;

          }

        }

      }

    }

  }

}

void 
RedrawThreadWorker::draw_box_properties (bool drawing_context, db::cell_index_type ci, const db::CplxTrans &trans, const std::vector<db::Box> &vp, int level)
{
  if (! m_text_visible) {
    return;
  }

  draw_box_properties (drawing_context, ci, trans, vp, level, 0);
}

void 
RedrawThreadWorker::draw_box_properties (bool drawing_context, db::cell_index_type ci, const db::CplxTrans &trans, const std::vector<db::Box> &vp, int level, db::properties_id_type prop_id)
{
  //  do not draw, if there is nothing to draw
  if (mp_layout->cells () <= ci || vp.empty ()) {
    return;
  }

  const db::Cell &cell = mp_layout->cell (ci);

  //  we will never come to a valid level ..
  if (! need_draw_box (mp_layout, cell, level, m_to_level, m_hidden_cells, m_cv_index)) {
    return;
  }
  if (cell_var_cached (ci, trans)) {
    return;
  }

  for (std::vector<db::Box>::const_iterator b = vp.begin (); b != vp.end (); ++b) {
    draw_box_properties (drawing_context, ci, trans, *b, level, prop_id);
  }
}

void
RedrawThreadWorker::draw_box_properties (bool drawing_context, db::cell_index_type ci, const db::CplxTrans &trans, const db::Box &vp, int level, db::properties_id_type prop_id)
{
  const db::Cell &cell = mp_layout->cell (ci);

  //  For small bboxes, the cell outline can be reduced ..
  db::Box bbox = cell.bbox ();

  if (bbox.empty ()) {

    //  no shapes there and below ..

  } else if (m_drop_small_cells && drop_cell (cell, trans)) {

    //  small cell dropped

  } else if (level == m_to_level || (m_cv_index < int (m_hidden_cells.size ()) && m_hidden_cells [m_cv_index].find (ci) != m_hidden_cells [m_cv_index].end ())) {

    //  paint the box on this level
    draw_cell_properties (drawing_context, level, trans, bbox, prop_id);

  } else if (level < m_to_level) {

    db::DBox dbbox = trans * bbox;
    if (dbbox.width () < 1.5 && dbbox.height () < 1.5) {

      //  ignore cells which are small 

    } else {

      db::box_convert <db::CellInst> bc (*mp_layout);
     
      //  create a set of boxes to look into
      db::Coord aw = db::coord_traits<db::Coord>::rounded (m_abstract_mode_width / mp_layout->dbu ());
      std::vector<db::Box> vv;
      if (level == 1 && m_abstract_mode_width > 0 && bbox.width () > db::Box::distance_type (aw * 2) && bbox.height () > db::Box::distance_type (aw * 2)) {
        vv.reserve (4);
        vv.push_back (vp & db::Box (bbox.left (), bbox.bottom (), bbox.left () + aw, bbox.top ()));
        vv.push_back (vp & db::Box (bbox.right () - aw, bbox.bottom (), bbox.right (), bbox.top ()));
        vv.push_back (vp & db::Box (bbox.left () + aw, bbox.bottom (), bbox.right () - aw, bbox.bottom () + aw));
        vv.push_back (vp & db::Box (bbox.left () + aw, bbox.top () - aw, bbox.right () - aw, bbox.top ()));
      } else {
        vv.reserve (1);
        vv.push_back (vp);
      }

      //  dive down into the hierarchy ..
      for (std::vector<db::Box>::const_iterator v = vv.begin (); v != vv.end (); ++v) {

        if (! v->empty ()) {

          bool anything = false;
          db::cell_index_type last_ci = std::numeric_limits<db::cell_index_type>::max ();

          db::Cell::touching_iterator inst = cell.begin_touching (*v); 
          while (! inst.at_end ()) {

            const db::CellInstArray &cell_inst = inst->cell_inst ();
            db::properties_id_type cell_inst_prop = inst->prop_id ();

            db::cell_index_type new_ci = cell_inst.object ().cell_index ();
            db::Box new_cell_box = mp_layout->cell (new_ci).bbox ();

            if (last_ci != new_ci) {
              //  Hint: don't use any_cell_box on partially visible cells because that will degrade performance 
              if (new_cell_box.inside (*v)) {
                last_ci = new_ci;
                anything = any_cell_box (new_ci, m_to_level - (level + 1));
              } else {
                anything = true;
              }
            }

            if (anything) {

              db::Vector a, b;
              unsigned long amax, bmax; 
              bool simplify = false;
              if (cell_inst.is_regular_array (a, b, amax, bmax)) {

                db::DBox inst_box;
                if (cell_inst.is_complex ()) {
                  inst_box = trans * (cell_inst.complex_trans () * new_cell_box);
                } else {
                  inst_box = trans * new_cell_box;
                }
                if (((a.x () == 0 && b.y () == 0) || (a.y () == 0 && b.x () == 0)) && 
                    inst_box.width () < 1.5 && inst_box.height () < 1.5 && 
                    (amax <= 1 || trans.ctrans (a.length ()) < 1.5) &&
                    (bmax <= 1 || trans.ctrans (b.length ()) < 1.5)) {
                  simplify = true;
                }

              }

              if (! simplify) {

                //  The array (or single instance) must be iterated instance
                //  by instance
                for (db::CellInstArray::iterator p = cell_inst.begin_touching (*v, bc); ! p.at_end (); ++p) {
              
                  test_snapshot (0); 
                  db::ICplxTrans t (cell_inst.complex_trans (*p));
                  db::Box new_vp = safe_transformed_box (*v, t.inverted ());
                  draw_box_properties (drawing_context, new_ci, trans * t, new_vp, level + 1, cell_inst_prop);

                }

              }

            }
          
            ++inst;

          }
        }

      }

    }

  }

}

/**
 *  @brief A helper function to determine if there are any area-type shapes on the cell below to a certain hierarchy level
 *
 *  @return The number of empty hierarchy levels below the cell (0: there are area-type shapes in this cell) 
 */
bool 
RedrawThreadWorker::any_shapes (db::cell_index_type cell_index, unsigned int levels)
{
  //  if the cell is "hidden", it does not need to be drawn
  if (int (m_hidden_cells.size ()) > m_cv_index) {
    if (m_hidden_cells [m_cv_index].find (cell_index) != m_hidden_cells [m_cv_index].end ()) {
      return false;
    }
  }

  //  the cache contains all cells that are visited already
  RedrawThreadWorker::micro_instance_cache_t::const_iterator c = m_mi_cache.find (std::make_pair (cell_index, levels));
  if (c == m_mi_cache.end ()) {

    int ret = false;

    const db::Cell &cell = mp_layout->cell (cell_index);
    if (! cell.shapes (m_layer).begin (db::ShapeIterator::Polygons | db::ShapeIterator::Edges | db::ShapeIterator::Paths | db::ShapeIterator::Boxes | db::ShapeIterator::Points, mp_prop_sel, m_inv_prop_sel).at_end ()) {
      ret = true;
    } else if (levels > 1) {
      for (db::Cell::child_cell_iterator cc = cell.begin_child_cells (); !cc.at_end () && !ret; ++cc) {
        ret = any_shapes (*cc, levels - 1);
      }
    }

    c = m_mi_cache.insert (std::make_pair (std::make_pair (cell_index, levels), ret)).first;

  }

  return c->second;
}

/**
 *  @brief A helper function to determine if there are any area-type shapes on the cell below to a certain hierarchy level
 *
 *  @return The number of empty hierarchy levels below the cell (0: there are area-type shapes in this cell) 
 */
bool 
RedrawThreadWorker::any_cell_box (db::cell_index_type cell_index, unsigned int levels)
{
  //  if the cell is "hidden", the box must be drawn
  if (int (m_hidden_cells.size ()) > m_cv_index) {
    if (m_hidden_cells [m_cv_index].find (cell_index) != m_hidden_cells [m_cv_index].end ()) {
      return true;
    }
  }

  //  the cache contains all cells that are visited already
  RedrawThreadWorker::micro_instance_cache_t::const_iterator c = m_mi_cell_box_cache.find (std::make_pair (cell_index, levels));
  if (c == m_mi_cell_box_cache.end ()) {

    int ret = false;
    if (levels > 1) {
      const db::Cell &cell = mp_layout->cell (cell_index);
      for (db::Cell::child_cell_iterator cc = cell.begin_child_cells (); !cc.at_end () && !ret; ++cc) {
        ret = any_cell_box (*cc, levels - 1);
      }
    } else {
      ret = true;
    }

    c = m_mi_cell_box_cache.insert (std::make_pair (std::make_pair (cell_index, levels), ret)).first;

  }

  return c->second;
}

/**
 *  @brief A helper function to determine if there are any text shapes on the cell below to a certain hierarchy level
 *
 *  @return The number of empty hierarchy levels below the cell (0: there are texts in this cell) 
 */
bool 
RedrawThreadWorker::any_text_shapes (db::cell_index_type cell_index, unsigned int levels)
{
  //  if the cell is "hidden", it does not need to be drawn
  if (int (m_hidden_cells.size ()) > m_cv_index) {
    if (m_hidden_cells [m_cv_index].find (cell_index) != m_hidden_cells [m_cv_index].end ()) {
      return false;
    }
  }

  //  the cache contains all cells that are visited already
  RedrawThreadWorker::micro_instance_cache_t::const_iterator c = m_mi_text_cache.find (std::make_pair (cell_index, levels));
  if (c == m_mi_text_cache.end ()) {

    bool ret = false;

    const db::Cell &cell = mp_layout->cell (cell_index);
    if (! cell.shapes (m_layer).begin (db::ShapeIterator::Texts, mp_prop_sel, m_inv_prop_sel).at_end () ||
        (m_show_properties && ! cell.shapes (m_layer).begin (db::ShapeIterator::AllWithProperties, mp_prop_sel, m_inv_prop_sel).at_end ())) {
      ret = true;
    } else if (levels > 1) {
      for (db::Cell::child_cell_iterator cc = cell.begin_child_cells (); !cc.at_end () && !ret; ++cc) {
        ret = any_text_shapes (*cc, levels - 1);
      }
    }

    c = m_mi_text_cache.insert (std::make_pair (std::make_pair (cell_index, levels), ret)).first;

  }

  return c->second;
}

static bool 
has_zero_bit (const lay::Bitmap *bitmap, unsigned int ixmin, unsigned int iymin, unsigned int ixmax, unsigned int iymax)
{
  uint32_t imin = ixmin / 32;
  uint32_t imax = ixmax / 32;

  if (imin == imax) {

    uint32_t m = ((unsigned int) 0xffffffff << (ixmin % 32)) & ((unsigned int) 0xffffffff >> (31 - (ixmax % 32)));

    for (unsigned int y = iymin; y <= iymax; ++y) {

      if (bitmap->is_scanline_empty (y)) {
        return true;
      }

      if ((bitmap->scanline (y) [imin] & m) != m) {
        return true;
      }

    }

  } else {

    uint32_t m1 = ((unsigned int) 0xffffffff << (ixmin % 32));
    uint32_t m2 = ((unsigned int) 0xffffffff >> (31 - (ixmax % 32)));

    for (unsigned int y = iymin; y <= iymax; ++y) {

      if (bitmap->is_scanline_empty (y)) {
        return true;
      }

      if ((bitmap->scanline (y) [imin] & m1) != m1) {
        return true;
      }
      for (unsigned int i = imin + 1; i < imax; ++i) {
        if (bitmap->scanline (y) [i] != 0xffffffff) {
          return true;
        }
      }
      if ((bitmap->scanline (y) [imax] & m2) != m2) {
        return true;
      }

    }

  }

  return false;
}

static bool 
skip_quad (const db::Box &qb, const lay::Bitmap *vertex_bitmap, const db::CplxTrans &trans)
{
  double threshold = 32 / trans.mag (); // don't check cells below 32x32 pixels
  if (qb.empty () || qb.width () > threshold || qb.height () > threshold || !vertex_bitmap) {
    return false;
  }

  db::DBox qb_trans = (trans * qb) & db::DBox (0, 0, vertex_bitmap->width () - 1.0 - 1e-6, vertex_bitmap->height () - 1.0 - 1e-6);
  if (qb_trans.empty ()) {
    return true;
  }

  int ixmin = (unsigned int)(qb_trans.left () + 0.5);
  int ixmax = (unsigned int)(qb_trans.right () + 0.5);
  int iymin = (unsigned int)(qb_trans.bottom () + 0.5);
  int iymax = (unsigned int)(qb_trans.top () + 0.5);
  if (! has_zero_bit (vertex_bitmap, ixmin, iymin, ixmax, iymax)) {
    return true; // skip
  } else {
    return false;
  }
}

inline void 
copy_bitmap (const lay::Bitmap *from, lay::Bitmap *to, int dx, int dy)
{
  if (to) {
    to->merge (from, dx, dy);
  }
}

std::vector<db::Box> 
RedrawThreadWorker::search_regions (const db::Box &cell_bbox, const db::Box &vp, int level)
{
  std::vector<db::Box> vv;

  //  create a set of boxes to look into
  db::Coord aw = db::coord_traits<db::Coord>::rounded (m_abstract_mode_width / mp_layout->dbu ());
  if (vp == db::Box::world ()) {
    vv.push_back (vp);
  } else if (level == 1 && m_abstract_mode_width > 0 && cell_bbox.width () > db::Box::distance_type (aw * 2) && cell_bbox.height () > db::Box::distance_type (aw * 2)) {
    vv.push_back (vp & db::Box (cell_bbox.left (), cell_bbox.bottom (), cell_bbox.left () + aw, cell_bbox.top ()));
    vv.push_back (vp & db::Box (cell_bbox.right () - aw, cell_bbox.bottom (), cell_bbox.right (), cell_bbox.top ()));
    vv.push_back (vp & db::Box (cell_bbox.left () + aw, cell_bbox.bottom (), cell_bbox.right () - aw, cell_bbox.bottom () + aw));
    vv.push_back (vp & db::Box (cell_bbox.left () + aw, cell_bbox.top () - aw, cell_bbox.right () - aw, cell_bbox.top ()));
  } else {
    vv.push_back (vp);
  }

  return vv;
}

void 
RedrawThreadWorker::draw_text_layer (bool drawing_context, db::cell_index_type ci, const db::CplxTrans &trans, const std::vector<db::Box> &vp, int level)
{
  if (! m_text_visible) {
    return;
  }

  unsigned int plane_group = 2;
  if (drawing_context) {
    plane_group = 0;
  } else if (m_child_context_enabled && level > 0) {
    plane_group = 1;
  }

  lay::CanvasPlane *fill = 0, *frame = 0, *text = 0, *vertex = 0;
  fill   = m_planes[0 + plane_group * (planes_per_layer / 3)];
  frame  = m_planes[1 + plane_group * (planes_per_layer / 3)];
  text   = m_planes[2 + plane_group * (planes_per_layer / 3)];
  vertex = m_planes[3 + plane_group * (planes_per_layer / 3)];

  //  do not draw, if there is nothing to draw
  if (mp_layout->cells () <= ci || vp.empty () || mp_layout->cell (ci).bbox (m_layer).empty ()) {
    return;
  }
  if (cell_var_cached (ci, trans)) {
    return;
  }

  std::unique_ptr<lay::Bitmap> opt_bitmap;
  lay::Bitmap *vertex_bitmap = dynamic_cast<lay::Bitmap *> (vertex);
  if (m_text_lazy_rendering && vertex_bitmap) {
    opt_bitmap.reset (new lay::Bitmap (vertex_bitmap->width (), vertex_bitmap->height (), vertex_bitmap->resolution ()));
  }

  for (std::vector<db::Box>::const_iterator b = vp.begin (); b != vp.end (); ++b) {
    draw_text_layer (drawing_context, ci, trans, *b, level, fill, frame, vertex, text, opt_bitmap.get ());
  }
}

void
RedrawThreadWorker::draw_text_layer (bool drawing_context, db::cell_index_type ci, const db::CplxTrans &trans, const db::Box &vp, int level, CanvasPlane *fill, CanvasPlane *frame, CanvasPlane *vertex, CanvasPlane *text, lay::Bitmap *opt_bitmap)
{
  test_snapshot (0);

  const db::Cell &cell = mp_layout->cell (ci);
  lay::Renderer &r = *mp_renderer;

  //  For small bboxes, the cell outline can be reduced ..
  db::Box bbox = cell.bbox (m_layer);

  if (m_drop_small_cells && drop_cell (cell, trans)) {

    //  small cell dropped

  } else if (! bbox.empty ()) {

    bool hidden = (m_cv_index < int (m_hidden_cells.size ()) && m_hidden_cells [m_cv_index].find (ci) != m_hidden_cells [m_cv_index].end ());
    bool need_to_dive = (level + 1 < m_to_level) && ! hidden;

    db::Box cell_bbox = cell.bbox ();

    //  draw this level
    if (level >= m_from_level && level < m_to_level && ! hidden) {

      db::DBox dbbox_tot = trans * cell_bbox;
      if (m_text_lazy_rendering && ((dbbox_tot.width () < 2.5 && dbbox_tot.height () < 1.5) || 
                                    (dbbox_tot.width () < 1.5 && dbbox_tot.height () < 2.5))) {

        bool anything = true;
        if (level == 0 && cell_bbox.inside (vp)) {
          //  Hint: on levels below 0 we enter this procedure only if there is a text 
          //  Hint: don't use any_text_shapes on partially visible cells because that will degrade performance 
          anything = any_text_shapes (ci, m_to_level - level);
        }

        //  paint the simplified box
        if (anything) {
          r.draw (trans * bbox, 0, frame, vertex, 0);
          if (opt_bitmap) {
            r.draw (trans * bbox, 0, 0, opt_bitmap, 0);
          }
        }

        //  do not dive further into hierarchy
        need_to_dive = false;

      } else {

        bool text_simplified = m_text_lazy_rendering && (dbbox_tot.width () <= 8.0 || dbbox_tot.height () <= 8.0);
            
        const db::Shapes &shapes = cell.shapes (m_layer);

        //  In lazy text rendering mode, all texts are only rendered if the cell is "reasonable large". 
        //  Empirically, a reasonable minimum dimension of 8x8 pixels was determined.
        //  otherwise just a few texts are rendered.

        //  this is the number of texts to draw in lazy text rendering mode
        size_t ntexts = 2;
        if (! text_simplified) {
          ntexts = std::numeric_limits <size_t>::max ();
        }

        //  create a set of boxes to look into
        std::vector<db::Box> vv = search_regions (cell_bbox, vp, level);

        //  iterate over the shapes
        for (std::vector<db::Box>::const_iterator v = vv.begin (); v != vv.end (); ++v) {

          if (! v->empty ()) {

            db::ShapeIterator shape = shapes.begin_touching (*v, db::ShapeIterator::Texts, mp_prop_sel, m_inv_prop_sel);
            while (! shape.at_end () && ntexts > 0) {

              test_snapshot (0); 

              r.draw (*shape, trans, fill, frame, vertex, text);
              if (opt_bitmap) {
                r.draw (*shape, trans, 0, 0, opt_bitmap, 0);
              }
              ++shape;

              --ntexts;

            }

            if (ntexts == 0) {
              break;
            }

            shape = shapes.begin_touching (*v, db::ShapeIterator::AllWithProperties, mp_prop_sel, m_inv_prop_sel);
            while (! shape.at_end () && ntexts > 0) {

              test_snapshot (0); 

              r.draw_propstring (*shape, &mp_layout->properties_repository (), text, trans);
              ++shape;

              --ntexts;

            }

            if (ntexts == 0) {
              break;
            }

          }

        }

      }

    }

    //  dive down into the hierarchy ..
    if (need_to_dive) {

      //  create a set of boxes to look into
      std::vector<db::Box> vv = search_regions (cell_bbox, vp, level);

      //  dive down into the hierarchy ..
      for (std::vector<db::Box>::const_iterator v = vv.begin (); v != vv.end (); ++v) {

        if (! v->empty ()) {

          size_t current_quad_id = 0;
          bool anything = false;
          db::cell_index_type last_ci = std::numeric_limits<db::cell_index_type>::max ();

          db::Cell::touching_iterator inst = cell.begin_touching (*v); 
          while (! inst.at_end ()) {

            //  skip this quad if we have drawn something here already
            size_t qid = inst.quad_id ();
            bool skip = false;
            if (m_text_lazy_rendering && qid != current_quad_id) {
              current_quad_id = qid;
              skip = opt_bitmap && skip_quad (inst.quad_box () & bbox, opt_bitmap, trans);
            }  

            if (skip) {

              //  move on to the next quad
              inst.skip_quad ();

            } else {

              const db::CellInstArray &cell_inst = inst->cell_inst ();
              ++inst;

              db::cell_index_type new_ci = cell_inst.object ().cell_index ();
              bool hidden = (m_cv_index < int (m_hidden_cells.size ()) && m_hidden_cells [m_cv_index].find (new_ci) != m_hidden_cells [m_cv_index].end ());

              db::Box cell_box = mp_layout->cell (new_ci).bbox (m_layer);
              if (! cell_box.empty () && ! hidden) {

                db::Vector a, b;
                unsigned long amax = 0, bmax = 0; 
                bool simplify = false;

                if (new_ci != last_ci) {
                  //  Hint: don't use any_text_shapes on partially visible cells because that will degrade performance 
                  if (cell_box.inside (vp)) {
                    last_ci = new_ci;
                    anything = any_text_shapes (new_ci, m_to_level - (level + 1));
                  } else {
                    anything = true;
                  }
                }

                if (anything && m_text_lazy_rendering && cell_inst.is_regular_array (a, b, amax, bmax)) {

                  db::DBox inst_box;
                  if (cell_inst.is_complex ()) {
                    inst_box = trans * (cell_inst.complex_trans () * cell_box);
                  } else {
                    inst_box = trans * cell_box;
                  }

                  if (((a.x () == 0 && b.y () == 0) || (a.y () == 0 && b.x () == 0)) && 
                      inst_box.width () < 1.5 && inst_box.height () < 1.5 && 
                      (amax <= 1 || trans.ctrans (a.length ()) < 1.5) &&
                      (bmax <= 1 || trans.ctrans (b.length ()) < 1.5)) {
                    simplify = true;
                  }

                }

                db::box_convert <db::CellInst> bc (*mp_layout, m_layer);

                if (simplify) {

                  //  The array can be simplified ..

                  db::Box bbox = cell_inst.bbox (bc);
                  if (vertex) {
                    r.draw (bbox, trans, vertex, vertex, 0, 0);
                  }

                } else if (anything) {

                  for (db::CellInstArray::iterator p = cell_inst.begin_touching (*v, bc); ! p.at_end (); ++p) {

                    if (! m_draw_array_border_instances || 
                        p.index_a () <= 0 || (unsigned long)p.index_a () == amax - 1 || p.index_b () <= 0 || (unsigned long)p.index_b () == bmax - 1) {

                      db::ICplxTrans t (cell_inst.complex_trans (*p));
                      db::Box new_vp = safe_transformed_box (*v, t.inverted ());
                      draw_text_layer (drawing_context, new_ci, trans * t, new_vp, level + 1, fill, frame, vertex, text, opt_bitmap);

                    } 

                  }

                }

              }

            }

          }

        }

      }

    }

  }

}

template <class Array>
bool draw_array_simplified (lay::Renderer *r, const db::Shape &array_shape, lay::CanvasPlane *frame, lay::CanvasPlane *vertex, const db::CplxTrans &trans)
{
  typename Array::tag tag;
  const Array *array = array_shape.basic_ptr (tag);

  db::Vector a, b;
  unsigned long na = 0, nb = 0;
  bool is_regular = array->is_regular_array (a, b, na, nb);
  size_t n = array->size ();

  if (n >= 2) {

    db::box_convert <typename Array::object_type> bc;

    db::DBox shape_box_trans = trans * bc (array->object ());
    if (shape_box_trans.width () < 1.5 && shape_box_trans.height () < 1.5) {

      if (is_regular && 
          ((a.x () == 0 && b.y () == 0) || (a.y () == 0 && b.x () == 0)) && 
          (na <= 1 || trans.ctrans (a.length ()) < 1.5) &&
          (nb <= 1 || trans.ctrans (b.length ()) < 1.5)) {

        db::Box array_box = array_shape.bbox ();
        r->draw (array_box, trans, frame, frame, 0, 0);
        r->draw (array_box, trans, vertex, vertex, 0, 0);
        return true;

      } else if (is_regular && 
          (a.x () == 0 || a.y () == 0) && 
          na > 1 && trans.ctrans (a.length ()) < 1.5) {

        Array a1 (array->object (), array->front (), a, db::Vector (0, 0), na, 1);
        db::Box abox = a1.bbox (bc);
        for (unsigned long i = 0; i < nb; ++i) {
          r->draw (abox, trans, frame, frame, 0, 0);
          r->draw (abox, trans, vertex, vertex, 0, 0);
          abox.move (b);
        }
        return true;

      } else if (is_regular && 
          (b.x () == 0 || b.y () == 0) && 
          nb > 1 && trans.ctrans (b.length ()) < 1.5) {

        Array a1 (array->object (), array->front (), db::Vector (0, 0), b, 1, nb);
        db::Box abox = a1.bbox (bc);
        for (unsigned long i = 0; i < na; ++i) {
          r->draw (abox, trans, frame, frame, 0, 0);
          r->draw (abox, trans, vertex, vertex, 0, 0);
          abox.move (a);
        }
        return true;

      } else {

        db::DBox array_box_trans = trans * array_shape.bbox ();
        if ((array_box_trans.height () < 1.5 && array_box_trans.width () < 3.5) ||
            (array_box_trans.height () < 3.5 && array_box_trans.width () < 1.5)) {
          r->draw (array_box_trans, frame, frame, 0, 0);
          r->draw (array_box_trans, vertex, vertex, 0, 0);
          return true;
        }

      }

    }

  }

  return false;
}
 
void
RedrawThreadWorker::draw_layer_wo_cache (int from_level, int to_level, db::cell_index_type ci, const db::CplxTrans &trans, const std::vector<db::Box> &vv, int level,
                                         lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertex, lay::CanvasPlane *text, const UpdateSnapshotCallback *update_snapshot)
{
  const db::Cell &cell = mp_layout->cell (ci);

  lay::Renderer &r = *mp_renderer;
  const db::Box &bbox = cell.bbox (m_layer);

  const lay::Bitmap *vertex_bitmap = dynamic_cast<const lay::Bitmap *> (vertex);

  //  draw this level
  if (level >= from_level && level < to_level) {

    //  draw the shapes or insert into the cell cache.
    for (std::vector<db::Box>::const_iterator v = vv.begin (); v != vv.end (); ++v) {

      if (v->empty ()) {
        continue;
      }

      const db::Shapes &shapes = cell.shapes (m_layer);
      db::Shape last_array;

      size_t current_quad_id = 0;
      size_t current_array_quad_id = 0;

      db::ShapeIterator shape (shapes.begin_touching (*v, db::ShapeIterator::Boxes | db::ShapeIterator::Polygons | db::ShapeIterator::Edges | db::ShapeIterator::Paths | db::ShapeIterator::Points, mp_prop_sel, m_inv_prop_sel));
      while (! shape.at_end ()) {

        test_snapshot (update_snapshot); 

        //  skip this quad if we have drawn something here already
        size_t qid = shape.quad_id ();
        bool skip = false;
        if (vertex_bitmap && qid != current_quad_id) {
          current_quad_id = qid;
          skip = skip_quad (shape.quad_box () & bbox, vertex_bitmap, trans);
        }

        if (skip) {
          shape.skip_quad ();
          continue;
        }

        if (shape.in_array ()) {

          if (last_array != shape.array ()) {

            last_array = shape.array ();
            current_array_quad_id = 0;

            bool simplified = false;

            if (last_array.type () == db::Shape::PolygonPtrArray) {
              simplified = draw_array_simplified<db::Shape::polygon_ptr_array_type> (mp_renderer.get (), last_array, frame, vertex, trans);
            } else if (last_array.type () == db::Shape::SimplePolygonPtrArray) {
              simplified = draw_array_simplified<db::Shape::simple_polygon_ptr_array_type> (mp_renderer.get (), last_array, frame, vertex, trans);
            } else if (last_array.type () == db::Shape::PathPtrArray) {
              simplified = draw_array_simplified<db::Shape::path_ptr_array_type> (mp_renderer.get (), last_array, frame, vertex, trans);
            } else if (last_array.type () == db::Shape::BoxArray) {
              simplified = draw_array_simplified<db::Shape::box_array_type> (mp_renderer.get (), last_array, frame, vertex, trans);
            } else if (last_array.type () == db::Shape::ShortBoxArray) {
              simplified = draw_array_simplified<db::Shape::short_box_array_type> (mp_renderer.get (), last_array, frame, vertex, trans);
            }

            if (simplified) {
              shape.finish_array ();
              //  continue with the next shape, array or quad
              continue;
            }

          }

        } else {
          current_array_quad_id = 0;
        }

        //  try whether the array quad can be simplified

        size_t aqid = shape.array_quad_id ();
        if (aqid != 0 && aqid != current_array_quad_id) {

          current_array_quad_id = aqid;

          db::DBox qbbox = trans * shape.array_quad_box ();
          if (qbbox.width () < 1.5 && qbbox.height () < 1.5) {

            //  draw a single box instead of the quad
            mp_renderer->draw (qbbox, fill, frame, vertex, text);
            shape.skip_array_quad ();

            //  continue with the next shape, array or quad
            continue;

          }

        }

        mp_renderer->draw (*shape, trans, fill, frame, vertex, text);
        ++shape;

      }

    }

  }

  //  dive down into the hierarchy ..
  if (level + 1 < to_level) {

    db::box_convert <db::CellInst> bc (*mp_layout, m_layer);

    //  dive down into the hierarchy ..
    for (std::vector<db::Box>::const_iterator v = vv.begin (); v != vv.end (); ++v) {

      if (v->empty ()) {
        continue;
      }

      size_t current_quad_id = 0;
      db::cell_index_type last_ci = std::numeric_limits<db::cell_index_type>::max ();
      bool anything = false;

      db::Cell::touching_iterator inst = cell.begin_touching (*v); 
      while (! inst.at_end ()) {

        test_snapshot (update_snapshot); 

        //  skip this quad if we have drawn something here already
        size_t qid = inst.quad_id ();
        bool skip = false;
        if (qid != current_quad_id) {
          current_quad_id = qid;
          skip = skip_quad (inst.quad_box () & bbox, vertex_bitmap, trans);
        }  

        if (skip) {

          //  move on to the next quad
          inst.skip_quad ();

        } else {

          const db::CellInstArray &cell_inst = inst->cell_inst ();
          ++inst;

          db::cell_index_type new_ci = cell_inst.object ().cell_index ();
          bool hidden = (m_cv_index < int (m_hidden_cells.size ()) && m_hidden_cells [m_cv_index].find (new_ci) != m_hidden_cells [m_cv_index].end ());

          db::Box new_cell_box = mp_layout->cell (new_ci).bbox (m_layer);
          if (! new_cell_box.empty () && ! hidden) {

            db::Vector a, b;
            unsigned long amax = 0, bmax = 0; 
            bool simplify = false;

            if (last_ci != new_ci) {
              //  Hint: don't use any_text_shapes on partially visible cells because that will degrade performance 
              if (new_cell_box.inside (*v)) {
                last_ci = new_ci;
                anything = any_shapes (new_ci, to_level - (level + 1));
              } else {
                anything = true;
              }
            }

            if (anything && cell_inst.is_regular_array (a, b, amax, bmax)) {

              db::DBox inst_box;
              if (cell_inst.is_complex ()) {
                inst_box = trans * (cell_inst.complex_trans () * new_cell_box);
              } else {
                inst_box = trans * new_cell_box;
              }

              if (((a.x () == 0 && b.y () == 0) || (a.y () == 0 && b.x () == 0)) && 
                  inst_box.width () < 1.5 && inst_box.height () < 1.5 && 
                  (amax <= 1 || trans.ctrans (a.length ()) < 1.5) &&
                  (bmax <= 1 || trans.ctrans (b.length ()) < 1.5)) {
                simplify = true;
              }

            }

            if (simplify) {

              //  The array can be simplified ..

              db::Box bbox = cell_inst.bbox (bc);
              if (frame) {
                r.draw (bbox, trans, frame, frame, 0, 0);
              }
              if (vertex) {
                r.draw (bbox, trans, vertex, vertex, 0, 0);
              }

            } else if (anything) {

              size_t qid = 0;

              for (db::CellInstArray::iterator p = cell_inst.begin_touching (*v, bc); ! p.at_end (); ) {

                if (! m_draw_array_border_instances || 
                    p.index_a () <= 0 || (unsigned long)p.index_a () == amax - 1 || p.index_b () <= 0 || (unsigned long)p.index_b () == bmax - 1) {

                  db::ICplxTrans t (cell_inst.complex_trans (*p));
                  db::Box new_vp = safe_transformed_box (*v, t.inverted ());
                  draw_layer (from_level, to_level, new_ci, trans * t, new_vp, level + 1, fill, frame, vertex, text, update_snapshot);

                  if (p.quad_id () > 0 && p.quad_id () != qid) {

                    qid = p.quad_id ();

                    //  if the quad is very small we don't gain anything from looking further into the quad - skip this one
                    db::DBox qb = trans * cell_inst.quad_box (p, bc);
                    if (qb.width () < 1.0 && qb.height () < 1.0) {
                      p.skip_quad ();
                      continue;
                    }

                  }

                } 

                ++p;

              }

            }

          }

        }

      }

    }

  }

}

class UpdateSnapshotWithCache 
  : public UpdateSnapshotCallback
{
public:
  UpdateSnapshotWithCache (const UpdateSnapshotCallback *parent, const db::CplxTrans *trans, const CellCacheInfo *info, lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertex, lay::CanvasPlane *text)
    : mp_parent (parent), mp_trans (trans), mp_info (info), mp_fill (fill), mp_frame (frame), mp_vertex (vertex), mp_text (text)
  {
    //  .. nothing yet ..
  }
       
  void trigger () const
  {
    if (mp_parent) {
      mp_parent->trigger ();
    }

    db::Point t = db::Point (mp_info->offset + mp_trans->disp ());

    copy_bitmap(mp_info->fill,   dynamic_cast<lay::Bitmap *> (mp_fill),   t.x (), t.y ());
    copy_bitmap(mp_info->frame,  dynamic_cast<lay::Bitmap *> (mp_frame),  t.x (), t.y ());
    copy_bitmap(mp_info->vertex, dynamic_cast<lay::Bitmap *> (mp_vertex), t.x (), t.y ());
    copy_bitmap(mp_info->text,   dynamic_cast<lay::Bitmap *> (mp_text),   t.x (), t.y ());
  }

private:
  const UpdateSnapshotCallback *mp_parent;
  const db::CplxTrans *mp_trans;
  const CellCacheInfo *mp_info;
  lay::CanvasPlane *mp_fill;
  lay::CanvasPlane *mp_frame;
  lay::CanvasPlane *mp_vertex;
  lay::CanvasPlane *mp_text;
};

void
RedrawThreadWorker::draw_layer (int from_level, int to_level, db::cell_index_type ci, const db::CplxTrans &trans, const std::vector<db::Box> &vp, int level,
                                lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertex, lay::CanvasPlane *text, const UpdateSnapshotCallback *update_snapshot)
{
  //  do not draw, if there is nothing to draw
  if (mp_layout->cells () <= ci || vp.empty ()) {
    return;
  }
  if (cell_var_cached (ci, trans)) {
    return;
  }

  for (std::vector<db::Box>::const_iterator b = vp.begin (); b != vp.end (); ++b) {
    draw_layer (from_level, to_level, ci, trans, *b, level, fill, frame, vertex, text, update_snapshot);
  }
}

void
RedrawThreadWorker::draw_layer (int from_level, int to_level, db::cell_index_type ci, const db::CplxTrans &trans, const db::Box &vp, int level,
                                lay::CanvasPlane *fill, lay::CanvasPlane *frame, lay::CanvasPlane *vertex, lay::CanvasPlane *text, const UpdateSnapshotCallback *update_snapshot)
{
  test_snapshot (update_snapshot);

  const db::Cell &cell = mp_layout->cell (ci);
  db::Box bbox = cell.bbox (m_layer);
  db::Box cell_bbox = cell.bbox ();

  //  Nothing to draw
  if (bbox.empty ()) {
    return;
  }

  //  For small bboxes, the cell outline can be reduced ..
  if (m_drop_small_cells && drop_cell (cell, trans)) {
    return;
  }

  //  Don't draw hidden cells
  bool hidden = (m_cv_index < int (m_hidden_cells.size ()) && m_hidden_cells [m_cv_index].find (ci) != m_hidden_cells [m_cv_index].end ());
  if (hidden) {
    return;
  }

  //  draw this level
  if (level >= from_level && level < to_level) {

    //  optimize very small cells
    db::DBox dbbox = trans * bbox;
    if ((dbbox.width () < 2.5 && dbbox.height () < 1.5) || 
        (dbbox.width () < 1.5 && dbbox.height () < 2.5)) {

      if (bbox.touches (vp)) {

        bool anything = true;
        if (level == 0) {
          //  Hint: on levels below 0 we know that there is anything. Otherwise we would not enter this procedure
          //  Hint: don't use any_text_shapes on partially visible cells because that will degrade performance
          anything = any_shapes (ci, m_to_level - level);
        }

        if (anything) {
          //  any shapes here: paint bbox for simplification
          mp_renderer->draw (dbbox, 0, frame, vertex, 0);
        }

      }

    } else {

      //  create a set of boxes to look into
      std::vector<db::Box> vv = search_regions (cell_bbox, vp, level);

      //  use the presence of a lay::Bitmap for the drawing plane as an indicator that we can cache the 
      //  drawings
      bool can_cache = (m_bitmap_caching && dynamic_cast<lay::Bitmap *> (fill) != 0);

      //  don't cache if the cell is not fully inside the search region
      if (vv.size () > 1 || ! cell_bbox.inside (vv.front ())) {
        can_cache = false;
      }

      //  only cache if we have more than one instance at all
      if (can_cache && level > 0) {
        db::Cell::parent_inst_iterator p = cell.begin_parent_insts ();
        size_t n;
        for (n = 0; !p.at_end () && n < 2; ++n) 
          ;
        if (n <= 1) {
          can_cache = false;
        }
      }

      if (can_cache) {

        db::CplxTrans trans_wo_disp = trans;
        trans_wo_disp.disp (db::DVector ());

        //  if we have the cell cached, use the cached bitmap
        CellCacheKey key (to_level - level, ci, trans_wo_disp);
        cell_cache_t::iterator cached_cell = m_cell_cache.find (key);
        if (cached_cell == m_cell_cache.end ()) {

          //  put the cell into the cache
          cached_cell = m_cell_cache.insert (std::make_pair (key, CellCacheInfo ())).first;

          db::DBox cell_box_trans = trans_wo_disp * cell_bbox;

          //  Hint: this rounding scheme guarantees a integer-pixel shift vector at least for the first instance
          db::DPoint d = cell_box_trans.lower_left () + trans.disp ();
          d = db::DPoint (floor (d.x ()), floor (d.y ()));
          cached_cell->second.offset = d - trans.disp ();
          db::CplxTrans drawing_trans = trans_wo_disp;
          drawing_trans.disp (db::DPoint () - cached_cell->second.offset);

          int width = int (cell_box_trans.width () + 3);    //  +3 = one pixel for a one-pixel frame at both sides and one for safety
          int height = int (cell_box_trans.height () + 3);

          cached_cell->second.fill   = new lay::Bitmap (width, height, 1.0);
          cached_cell->second.frame  = new lay::Bitmap (width, height, 1.0);
          cached_cell->second.vertex = new lay::Bitmap (width, height, 1.0);
          cached_cell->second.text   = new lay::Bitmap (width, height, 1.0);

          //  this object is responsible for doing updates when a snapshot is taken
          UpdateSnapshotWithCache update_cached_snapshot (update_snapshot, &trans, &cached_cell->second, fill, frame, vertex, text);

          draw_layer_wo_cache (from_level, to_level, ci, drawing_trans, vv, level, cached_cell->second.fill, cached_cell->second.frame, cached_cell->second.vertex, cached_cell->second.text, &update_cached_snapshot);

        }
        cached_cell->second.hits++;

        db::Point t = db::Point (cached_cell->second.offset + trans.disp ());

        copy_bitmap(cached_cell->second.fill,   dynamic_cast<lay::Bitmap *> (fill),   t.x (), t.y ());
        copy_bitmap(cached_cell->second.frame,  dynamic_cast<lay::Bitmap *> (frame),  t.x (), t.y ());
        copy_bitmap(cached_cell->second.vertex, dynamic_cast<lay::Bitmap *> (vertex), t.x (), t.y ());
        copy_bitmap(cached_cell->second.text,   dynamic_cast<lay::Bitmap *> (text),   t.x (), t.y ());

      } else {
        draw_layer_wo_cache (from_level, to_level, ci, trans, vv, level, fill, frame, vertex, text, update_snapshot);
      }

    } 
  
  } else {

    //  draw stuff below (not on this level)
    std::vector<db::Box> vv;
    vv.push_back (vp);
    draw_layer_wo_cache (from_level, to_level, ci, trans, vv, level, fill, frame, vertex, text, update_snapshot);

  }
}

void
RedrawThreadWorker::draw_layer (bool drawing_context, db::cell_index_type ci, const db::CplxTrans &trans, const std::vector<db::Box> &redraw_regions, int level)
{
  if (drawing_context) {

    if (m_to_level > m_from_level) {

      lay::CanvasPlane *fill = 0, *frame = 0, *text = 0, *vertex = 0;
      int plane_group = 0;
      fill   = m_planes[0 + plane_group * (planes_per_layer / 3)];
      frame  = m_planes[1 + plane_group * (planes_per_layer / 3)];
      text   = m_planes[2 + plane_group * (planes_per_layer / 3)];
      vertex = m_planes[3 + plane_group * (planes_per_layer / 3)];

      draw_layer (m_from_level, m_to_level, ci, trans, redraw_regions, level, fill, frame, vertex, text, 0);

    }

  } else if (! m_child_context_enabled) {

    if (m_to_level > m_from_level) {

      lay::CanvasPlane *fill = 0, *frame = 0, *text = 0, *vertex = 0;
      int plane_group = 2;
      fill   = m_planes[0 + plane_group * (planes_per_layer / 3)];
      frame  = m_planes[1 + plane_group * (planes_per_layer / 3)];
      text   = m_planes[2 + plane_group * (planes_per_layer / 3)];
      vertex = m_planes[3 + plane_group * (planes_per_layer / 3)];

      draw_layer (m_from_level, m_to_level, ci, trans, redraw_regions, level, fill, frame, vertex, text, 0);

    }

  } else {

    if (1 > m_from_level) {

      lay::CanvasPlane *fill = 0, *frame = 0, *text = 0, *vertex = 0;
      int plane_group = 2;
      fill   = m_planes[0 + plane_group * (planes_per_layer / 3)];
      frame  = m_planes[1 + plane_group * (planes_per_layer / 3)];
      text   = m_planes[2 + plane_group * (planes_per_layer / 3)];
      vertex = m_planes[3 + plane_group * (planes_per_layer / 3)];

      draw_layer (m_from_level, 1, ci, trans, redraw_regions, level, fill, frame, vertex, text, 0);

    }

    if (m_to_level > 1) {

      lay::CanvasPlane *fill = 0, *frame = 0, *text = 0, *vertex = 0;
      int plane_group = 1;
      fill   = m_planes[0 + plane_group * (planes_per_layer / 3)];
      frame  = m_planes[1 + plane_group * (planes_per_layer / 3)];
      text   = m_planes[2 + plane_group * (planes_per_layer / 3)];
      vertex = m_planes[3 + plane_group * (planes_per_layer / 3)];

      draw_layer (1, m_to_level, ci, trans, redraw_regions, level, fill, frame, vertex, text, 0);

    }

  }
}

bool
RedrawThreadWorker::drop_cell (const db::Cell &cell, const db::CplxTrans &trans)
{
  db::DBox bbox = trans * cell.bbox ();

  double value = 0;
  if (m_drop_small_cells_cond == lay::LayoutViewBase::DSC_Min) {
    value = std::min (bbox.width (), bbox.height ());
  } else if (m_drop_small_cells_cond == lay::LayoutViewBase::DSC_Max) {
    value = std::max (bbox.width (), bbox.height ());
  } else {
    value = bbox.width () + bbox.height ();
  }

  return (value < double (m_drop_small_cells_value));
}

bool 
RedrawThreadWorker::cell_var_cached (db::cell_index_type ci, const db::CplxTrans &trans) 
{
  if (mp_cell_var_cache) {
    //  Use the native transformation (just including cell instantiation components) to enable
    //  fuzzy comparison of floating-point coordinates: this requires a well-defined unit system to
    //  allow the definition of an uncertainty value.
    db::CplxTrans db_trans ((m_vp_trans * mp_layout->dbu ()).inverted () * db::DCplxTrans (trans));
    if (mp_cell_var_cache->find (std::make_pair (db_trans, ci)) != mp_cell_var_cache->end ()) {
      ++m_cache_hits;
      return true;
    }
    ++m_cache_misses;
    mp_cell_var_cache->insert (std::make_pair (db_trans, ci));
  }
  return false;
}

void 
RedrawThreadWorker::iterate_variants (const std::vector <db::Box> &redraw_regions, db::cell_index_type ci, db::CplxTrans trans, void (RedrawThreadWorker::*what) (bool, db::cell_index_type, const db::CplxTrans &, const std::vector<db::Box> &, int))
{
  //  save current state
  int from_level = m_from_level;
  int to_level = m_to_level;
  
  //  if a context path is given, we adjust the levels such that the target (not the context 
  //  cell) is drawn and the context cell is visible through addressing negative levels. The
  //  iterate_variants_rec methods takes care of converting the negative hierarchy levels into
  //  traversals along the context path bottom up.
  size_t ctx_path_length = m_cellviews [m_cv_index].specific_path ().size ();
  if (ctx_path_length > 0) {
    m_from_level -= int (ctx_path_length);
    m_to_level -= int (ctx_path_length);
    trans = trans * m_cellviews [m_cv_index].context_trans ();
  }

  if (m_from_level_default < 0 || ctx_path_length > 0) {

    //  if we start from above the hierarchy, we need to establish a 
    //  cell variant cache to at least avoid painting the current cell
    //  multiple times.
    std::set <std::pair <db::CplxTrans, db::cell_index_type>, lay::CellVariantCacheCompare> cell_var_cache;
    mp_cell_var_cache = &cell_var_cache;

    //  Use the cell variant cache to exclude the basic instance from the 
    //  drawing in the first pass.
    cell_var_cache.insert (std::make_pair (db::CplxTrans (m_cellviews [m_cv_index].context_trans ()), ci));

    m_cache_hits = m_cache_misses = 0;

    //  draw the context for the current instance
    iterate_variants_rec (redraw_regions, ci, trans, 0, what, true);

    cell_var_cache.clear ();

    //  draw the current instance without context (using a minimum of from_level=0 for this)
    mp_cell_var_cache = 0;
    int fl = m_from_level;
    if (m_from_level < 0) {
      m_from_level = 0;
    }
    iterate_variants_rec (redraw_regions, ci, trans, 0, what, false);
    m_from_level = fl;

    if (tl::verbosity () >= 40) {
      tl::info << tl::to_string (tr ("Cell variant cache hits/misses: ")) << m_cache_hits << "/" << m_cache_misses;
    }

  } else {
    mp_cell_var_cache = 0;
    iterate_variants_rec (redraw_regions, ci, trans, 0, what, false);
  }

  //  restore state
  m_from_level = from_level;
  m_to_level = to_level;
}

void 
RedrawThreadWorker::iterate_variants_rec (const std::vector <db::Box> &redraw_regions, db::cell_index_type ci, const db::CplxTrans &trans, int level, void (RedrawThreadWorker::*what) (bool, db::cell_index_type, const db::CplxTrans &, const std::vector<db::Box> &, int), bool drawing_context)
{
  db::Cell::parent_inst_iterator p = mp_layout->cell (ci).begin_parent_insts ();
  int context_path_length = int (m_cellviews [m_cv_index].specific_path ().size ());

  if ((drawing_context || level > m_from_level) && level + context_path_length > 0) {

    //  pull an specific instance from the instance stack and move this one up
    const db::InstElement &ie = m_cellviews [m_cv_index].specific_path ().end () [level - 1];

    db::cell_index_type new_ci;
    if (level + context_path_length > 1) {
      new_ci = m_cellviews [m_cv_index].specific_path ().end () [level - 2].inst_ptr.cell_index ();
    } else {
      new_ci = m_cellviews [m_cv_index].ctx_cell_index ();
    }

    db::ICplxTrans t (ie.complex_trans ());
    iterate_variants_rec (redraw_regions, new_ci, trans * t.inverted (), level - 1, what, drawing_context);

  } else if (level > (drawing_context ? (m_from_level_default - context_path_length) : m_from_level) && ! p.at_end ()) {

    //  one level up ..
    while (! p.at_end ()) {

      db::Cell::cell_inst_array_type pi = (*p).inst ();

      db::cell_index_type new_ci = pi.object ().cell_index ();

      for (db::Cell::cell_inst_array_type::iterator pp = pi.begin (); ! pp.at_end (); ++pp) {

        db::ICplxTrans t (pi.complex_trans (*pp));
        iterate_variants_rec (redraw_regions, new_ci, trans * t, level - 1, what, drawing_context);

      }

      ++p;

    }

  } else {

    std::vector<db::Box> actual_regions;
    actual_regions.reserve (redraw_regions.size ());

    for (std::vector<db::Box>::const_iterator rr = redraw_regions.begin (); rr != redraw_regions.end (); ++rr) {

      db::Coord lim = std::numeric_limits<db::Coord>::max ();
      db::DBox world (trans * db::Box (db::Point (-lim, -lim), db::Point (lim, lim)));
      db::Box vp = db::Box (trans.inverted () * (world & db::DBox (*rr)));
      vp &= mp_layout->cell (ci).bbox (); // this avoids problems when accessing designs through very large viewports
      if (! vp.empty ()) {
        actual_regions.push_back (vp);
      }

    }

    if (! actual_regions.empty ()) {
      (this->*what) (drawing_context, ci, trans, actual_regions, level);
    }

  }

}

}

