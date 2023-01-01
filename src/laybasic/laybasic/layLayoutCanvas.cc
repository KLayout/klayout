
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

#if defined(HAVE_QT)
#  include <QEvent>
#  include <QPainter>
#  include <QApplication>
#  include <QWheelEvent>
#endif

#include "tlTimer.h"
#include "tlLog.h"
#include "tlAssert.h"
#include "layLayoutCanvas.h"
#include "layRedrawThread.h"
#include "layLayoutViewBase.h"
#include "layMarker.h"
#if defined(HAVE_QT)
#  include "gtf.h"
#endif

#include "layBitmapsToImage.h"

#include <sstream>
#include <algorithm>

namespace lay
{

// ----------------------------------------------------------------------------

/**
 *  @brief Returns a value indicating whether the needed entry can make use of the one in the cache
 */
static bool applies (const lay::RedrawLayerInfo &in_cache, const lay::RedrawLayerInfo &needed) 
{
  if (needed.visible && !in_cache.visible) {
    return false;
  }

  if (needed.cell_frame != in_cache.cell_frame ||
      needed.xfill != in_cache.xfill ||
      needed.layer_index != in_cache.layer_index ||
      needed.cellview_index != in_cache.cellview_index ||
      needed.hier_levels != in_cache.hier_levels ||
      needed.prop_sel != in_cache.prop_sel ||
      needed.inverse_prop_sel != in_cache.inverse_prop_sel) {
    return false;
  }

  if (needed.trans.size () != in_cache.trans.size ()) {
    return false;
  }

  for (size_t i = 0; i < needed.trans.size (); ++i) {
    if (! needed.trans[i].equal (in_cache.trans[i])) {
      return false;
    }
  }

  return true;
}

ImageCacheEntry::ImageCacheEntry (const lay::Viewport &vp, const std::vector<lay::RedrawLayerInfo> &layers, bool precious)
  : m_opened (true), m_trans (vp.trans ()), m_layers (layers), m_width (vp.width ()), m_height (vp.height ()), m_precious (precious)
{ 
  //  .. nothing yet .. 
}

bool ImageCacheEntry::equals (const lay::Viewport &vp, const std::vector<lay::RedrawLayerInfo> &layers) const
{
  if (!m_trans.equal (vp.trans ()) || m_width != vp.width () || m_height != vp.height ()) {
    return false;
  }

  if (m_layers.size () != layers.size ()) {
    return false;
  }

  for (size_t i = 0; i < m_layers.size (); ++i) {
    if (! applies (m_layers [i], layers [i])) {
      return false;
    }
  }

  return true;
}

void ImageCacheEntry::close (const BitmapCanvasData &data)
{
  m_data = data;
  m_opened = false;
}

void ImageCacheEntry::swap (ImageCacheEntry &other)
{
  std::swap (m_opened, other.m_opened);
  std::swap (m_layers, other.m_layers);
  std::swap (m_trans, other.m_trans);
  std::swap (m_width, other.m_width);
  std::swap (m_height, other.m_height);
  std::swap (m_precious, other.m_precious);
  m_data.swap (other.m_data);
}

std::string ImageCacheEntry::to_string () const
{
  return std::string (m_opened ? "(" : "") + std::string (m_precious ? "*" : " ") + 
         tl::to_string (m_width) + "x" + tl::to_string (m_height) + " " +
         m_trans.to_string () + std::string (m_opened ? ")" : "");
}

// ----------------------------------------------------------------------------

void 
invert (unsigned char *data, unsigned int width, unsigned int height)
{
  unsigned int nbytes = (width + 7) / 8;
  unsigned char *psrc = data;
  for (unsigned int y = 0; y < height; ++y) {
    for (unsigned int i = 0; i < nbytes; ++i) {
      *psrc++ ^= 0xff;
    }
  }
}

LayoutCanvas::LayoutCanvas (lay::LayoutViewBase *view)
  : lay::ViewObjectUI (),
    mp_view (view),
    mp_image (0), mp_image_bg (0),
    mp_image_fg (0),
    m_background (0), m_foreground (0), m_active (0),
    m_oversampling (1),
    m_hrm (false),
    m_need_redraw (false),
    m_redraw_clearing (false),
    m_redraw_force_update (true),
    m_update_image (true),
    m_drawing_finished (false),
    m_do_update_image_dm (this, &LayoutCanvas::do_update_image),
    m_do_end_of_drawing_dm (this, &LayoutCanvas::do_end_of_drawing),
    m_image_cache_size (1)
{
  //  The gamma value used for subsampling: something between 1.8 and 2.2.
  m_gamma = 2.0;

  //  some reasonable initializations for the size
  m_viewport.set_size (100, 100); 
  m_viewport_l.set_size (m_viewport.width () * m_oversampling, m_viewport.height () * m_oversampling);

  mp_redraw_thread = new lay::RedrawThread (this, view);

  tl::Color bg (0xffffffff), fg (0xff000000), active (0xffc0c0c0);
  set_colors (bg, fg, active);
}

LayoutCanvas::~LayoutCanvas ()
{
  //  Detach all listeners so we don't trigger events in the destructor
  viewport_changed_event.clear ();

  if (mp_image) {
    delete mp_image;
    mp_image = 0;
  }
  if (mp_image_bg) {
    delete mp_image_bg;
    mp_image_bg = 0;
  }
  if (mp_image_fg) {
    delete mp_image_fg;
    mp_image_fg = 0;
  }
  if (mp_redraw_thread) {
    delete mp_redraw_thread;
    mp_redraw_thread = 0;
  }

  clear_fg_bitmaps ();
}

double
LayoutCanvas::resolution () const
{
  if (m_hrm) {
    return 1.0 / m_oversampling;
  } else {
    return 1.0 / (m_oversampling * dpr ());
  }
}

#if defined(HAVE_QT)
void
LayoutCanvas::init_ui (QWidget *parent)
{
  lay::ViewObjectUI::init_ui (parent);

  if (widget ()) {

    widget ()->setObjectName (QString::fromUtf8 ("canvas"));
    widget ()->setBackgroundRole (QPalette::NoRole);

    tl::Color bg = tl::Color (widget ()->palette ().color (QPalette::Normal, QPalette::Window).rgb ());
    tl::Color fg = tl::Color (widget ()->palette ().color (QPalette::Normal, QPalette::Text).rgb ());
    tl::Color active = tl::Color (widget ()->palette ().color (QPalette::Normal, QPalette::Mid).rgb ());
    set_colors (bg, fg, active);

    widget ()->setAttribute (Qt::WA_NoSystemBackground);

  }
}
#endif

void
LayoutCanvas::key_event (unsigned int key, unsigned int buttons)
{
  if (! (buttons & lay::ShiftButton)) {
    if (int (key) == lay::KeyDown) {
      down_arrow_key_pressed ();
    } else if (int (key) == lay::KeyUp) {
      up_arrow_key_pressed ();
    } else if (int (key) == lay::KeyLeft) {
      left_arrow_key_pressed ();
    } else if (int (key) == lay::KeyRight) {
      right_arrow_key_pressed ();
    }
  } else {
    if (int (key) == lay::KeyDown) {
      down_arrow_key_pressed_with_shift ();
    } else if (int (key) == lay::KeyUp) {
      up_arrow_key_pressed_with_shift ();
    } else if (int (key) == lay::KeyLeft) {
      left_arrow_key_pressed_with_shift ();
    } else if (int (key) == lay::KeyRight) {
      right_arrow_key_pressed_with_shift ();
    }
  }
}

void
LayoutCanvas::set_image_cache_size (size_t sz)
{
  m_image_cache_size = sz;
}

void
LayoutCanvas::set_oversampling (unsigned int os)
{
  if (os != m_oversampling) {
    m_image_cache.clear ();
    m_oversampling = os;
    m_viewport_l.set_size (m_viewport.width () * m_oversampling, m_viewport.height () * m_oversampling);
    do_redraw_all ();
  }
}

void
LayoutCanvas::set_highres_mode (bool hrm)
{
  if (hrm != m_hrm) {
    m_image_cache.clear ();
    m_hrm = hrm;
    do_redraw_all ();
  }
}

double
LayoutCanvas::dpr () const
{
#if defined(HAVE_QT) && QT_VERSION >= 0x50000
  return widget () ? widget ()->devicePixelRatio () : 1.0;
#else
  return 1.0;
#endif
}

void 
LayoutCanvas::set_colors (tl::Color background, tl::Color foreground, tl::Color active)
{
  m_background = background.rgb ();
  m_foreground = foreground.rgb ();
  m_active = active.rgb ();
      
  //  force regeneration of background image ..
  if (mp_image_bg) {
    delete mp_image_bg;
  }
  mp_image_bg = 0;

  update_image ();
}

void
LayoutCanvas::set_view_ops (std::vector <lay::ViewOp> &view_ops)
{
  if (view_ops != m_view_ops) {
    m_view_ops.swap (view_ops);
    m_scaled_view_ops.clear ();
    update_image ();
  }
}

void
LayoutCanvas::set_dither_pattern (const lay::DitherPattern &p)
{
  if (p != m_dither_pattern) {
    m_dither_pattern = p;
    update_image ();
  }
}

void
LayoutCanvas::set_line_styles (const lay::LineStyles &s)
{
  if (s != m_line_styles) {
    m_line_styles = s;
    update_image ();
  }
}

const std::vector <lay::ViewOp> &
LayoutCanvas::scaled_view_ops (unsigned int lw)
{
  if (lw <= 1) {
    return m_view_ops;
  }

  auto cached = m_scaled_view_ops.find (lw);
  if (cached != m_scaled_view_ops.end ()) {
    return cached->second;
  }

  std::vector<lay::ViewOp> &scaled_view_ops = m_scaled_view_ops [lw];
  scaled_view_ops = m_view_ops;
  for (std::vector<lay::ViewOp>::iterator vo = scaled_view_ops.begin (); vo != scaled_view_ops.end (); ++vo) {
    vo->width (std::min (31, vo->width () * int (lw)));
  }

  return scaled_view_ops;
}

void
LayoutCanvas::prepare_drawing ()
{
  if (m_need_redraw) {

    BitmapViewObjectCanvas::set_size (m_viewport_l.width (), m_viewport_l.height (), resolution ());

    if (! mp_image ||
        (unsigned int) mp_image->width () != m_viewport_l.width () || 
        (unsigned int) mp_image->height () != m_viewport_l.height ()) {
      if (mp_image) {
        delete mp_image;
      }
      mp_image = new tl::PixelBuffer (m_viewport_l.width (), m_viewport_l.height ());
      if (mp_image_fg) {
        delete mp_image_fg;
        mp_image_fg = 0;
      }
    }

    mp_image->fill (m_background);

    //  Cancel any pending "finish" event so there is no race between finish and restart (important for caching)
    m_do_end_of_drawing_dm.cancel (); 

    //  look for a cache entry we may reuse
    std::vector <ImageCacheEntry>::iterator c;
    for (c = m_image_cache.begin (); c != m_image_cache.end (); ++c) {
      if (! c->opened () && c->equals (m_viewport_l, m_layers) && can_restore_data (c->data ())) {
        break;
      }
    }

    if (c != m_image_cache.end ()) {

      //  move selected entry to end of cache for renewed life time
      while (c + 1 != m_image_cache.end ()) {
        c->swap (c [1]);
        ++c;
      }

      mp_redraw_thread->commit (m_layers, m_viewport_l, resolution ());

      if (tl::verbosity () >= 20) {
        tl::info << "Restored image from cache";
      }
      restore_data (c->data ());

    } else {

      bool precious = m_viewport_l.target_box ().equal (m_precious_box); 

      //  discard all open cache entries and reset all previously precious ones
      for (size_t i = 0; i < m_image_cache.size (); ++i) {
        if (precious) {
          m_image_cache [i].set_precious (false);
        }
        if (m_image_cache [i].opened () || m_image_cache [i].equals (m_viewport_l, m_layers)) {
          m_image_cache.erase (m_image_cache.begin () + i);
          --i;
        }
      }

      //  retire old entries
      if (m_image_cache_size == 0) {

        m_image_cache.clear ();

      } else {

        if (m_image_cache_size == 1) {
          if (precious || (!m_image_cache.empty () && !m_image_cache.front ().precious ())) {
            m_image_cache.clear ();
          }
        } else if (m_image_cache.size () > m_image_cache_size - 1) {
          for (size_t i = 0; i < m_image_cache.size () && m_image_cache.size () > m_image_cache_size - 1; ++i) {
            if (! m_image_cache [i].precious ()) {
              m_image_cache.erase (m_image_cache.begin () + i);
              --i;
            }
          }
        }

        //  create a new image cache entry
        if (m_image_cache.size () < m_image_cache_size) {
          m_image_cache.push_back (ImageCacheEntry (m_viewport_l, m_layers, precious));
        }

      }

      if (m_redraw_clearing) {
        mp_redraw_thread->start (mp_view->synchronous () ? 0 : mp_view->drawing_workers (), m_layers, m_viewport_l, resolution (), m_redraw_force_update);
      } else {
        mp_redraw_thread->restart (m_need_redraw_layer);
      }

    }

    //  for short draw jobs, the drawing is already done now. For others display the busy cursor.
    if (mp_redraw_thread->is_running ()) {
      set_default_cursor (lay::Cursor::wait);
    }

    m_need_redraw = false;
    m_redraw_force_update = false;
    m_update_image = true;

  }
}

void
LayoutCanvas::update_image ()
{
  // this will make the image being redone (except for background objects which will
  // only be redrawn on touch_bg)
  m_update_image = true;

  update (); // produces a paintEvent()
}

void
LayoutCanvas::free_resources ()
{
  if (mp_image_fg) {
    delete mp_image_fg;
    mp_image_fg = 0;
  }
}

#if defined(HAVE_QT)
void
LayoutCanvas::paint_event ()
{
  //  this is the update image request
  tl::SelfTimer timer_info (tl::verbosity () >= 41, tl::to_string (QObject::tr ("PaintEvent")));

  //  if required, start the redraw thread ..
  prepare_drawing ();

  if (mp_image) {
    
    //  check, if the background needs to be updated 
    if (m_update_image || needs_update_bg ()) {

      if (needs_update_bg () || ! mp_image_bg) {

        //  clear the image and paint the background objects
        mp_image->fill (m_background);
        do_render_bg (m_viewport_l, *this);

        //  save the current background image
        if (mp_image_bg) {
          delete mp_image_bg;
        }
        mp_image_bg = new tl::PixelBuffer (*mp_image);

      } else {
        //  else reuse the saved image
        *mp_image = *mp_image_bg;
      }

      //  render the main bitmaps
      to_image (scaled_view_ops (1.0 / resolution ()), dither_pattern (), line_styles (), 1.0 / resolution (), background_color (), foreground_color (), active_color (), this, *mp_image, m_viewport_l.width (), m_viewport_l.height ());

      if (mp_image_fg) {
        delete mp_image_fg;
        mp_image_fg = 0;
      }

      m_update_image = false;

    }

    //  create a base pixmap consisting of the layout with background
    //  and static foreground objects

    if (! mp_image_fg || needs_update_static () ||
        int (mp_image->width ()) != (int) mp_image_fg->width () * int (m_oversampling) ||
        int (mp_image->height ()) != (int) mp_image_fg->height () * int (m_oversampling)) {

      if (mp_image_fg) {
        delete mp_image_fg;
      } 

      clear_fg_bitmaps ();
      do_render (m_viewport_l, *this, true);

      mp_image_fg = new tl::PixelBuffer ();

      if (fg_bitmaps () > 0) {

        tl::PixelBuffer full_image (*mp_image);
        bitmaps_to_image (fg_view_op_vector (), fg_bitmap_vector (), dither_pattern (), line_styles (), 1.0 / resolution (), &full_image, m_viewport_l.width (), m_viewport_l.height (), false, &m_mutex);

        //  render the foreground parts ..
        if (m_oversampling == 1) {
          *mp_image_fg = full_image;
        } else {
          tl::PixelBuffer subsampled_image (m_viewport.width (), m_viewport.height ());
          subsampled_image.set_transparent (mp_image->transparent ());
          full_image.subsample (subsampled_image, m_oversampling, m_gamma);
          *mp_image_fg = subsampled_image;
        }

      } else if (m_oversampling == 1) {

        *mp_image_fg = *mp_image;

      } else {

        tl::PixelBuffer subsampled_image (m_viewport.width (), m_viewport.height ());
        subsampled_image.set_transparent (mp_image->transparent ());
        mp_image->subsample (subsampled_image, m_oversampling, m_gamma);
        *mp_image_fg = subsampled_image;

      }

    }

    //  erase any previous data
    clear_fg_bitmaps ();

    //  render dynamic foreground content
    do_render (m_viewport_l, *this, false);

    //  produce the pixmap first and then overdraw with dynamic content.
    QPainter painter (widget ());
    QImage img = mp_image_fg->to_image ();
#if QT_VERSION >= 0x050000
    img.setDevicePixelRatio (dpr ());
#endif
    painter.drawImage (QPoint (0, 0), img);

    if (fg_bitmaps () > 0) {

      tl::PixelBuffer full_image (mp_image->width (), mp_image->height ());
      full_image.set_transparent (true);
      full_image.fill (0);

      bitmaps_to_image (fg_view_op_vector (), fg_bitmap_vector (), dither_pattern (), line_styles (), 1.0 / resolution (), &full_image, m_viewport_l.width (), m_viewport_l.height (), false, &m_mutex);

      //  render the foreground parts ..
      if (m_oversampling == 1) {
        QImage img = full_image.to_image ();
#if QT_VERSION >= 0x050000
        img.setDevicePixelRatio (dpr ());
#endif
        painter.drawImage (QPoint (0, 0), img);
      } else {
        tl::PixelBuffer subsampled_image (m_viewport.width (), m_viewport.height ());
        subsampled_image.set_transparent (true);
        full_image.subsample (subsampled_image, m_oversampling, m_gamma);
        QImage img = subsampled_image.to_image ();
#if QT_VERSION >= 0x050000
        img.setDevicePixelRatio (dpr ());
#endif
        painter.drawImage (QPoint (0, 0), img);
      }

    }

    //  erase dynamic bitmaps 
    clear_fg_bitmaps ();

#if QT_VERSION < 0x050000
    QApplication::syncX ();
#endif

  }

}
#endif

class DetachedViewObjectCanvas
  : public BitmapViewObjectCanvas
{
public:
  DetachedViewObjectCanvas (tl::Color bg, tl::Color fg, tl::Color ac, unsigned int width_l, unsigned int height_l, double resolution, tl::PixelBuffer *img)
    : BitmapViewObjectCanvas (width_l, height_l, resolution),
      m_bg (bg), m_fg (fg), m_ac (ac), mp_image (img)
  {
    //  TODO: Good choice?
    m_gamma = 2.0;

    if (img->width () != width_l || img->height () != height_l) {
      mp_image_l = new tl::PixelBuffer (width_l, height_l);
      mp_image_l->set_transparent (img->transparent ());
      mp_image_l->fill (bg.rgb ());
    } else {
      mp_image_l = 0;
    }
  }

  ~DetachedViewObjectCanvas ()
  {
    clear_fg_bitmaps ();

    if (mp_image_l) {
      delete mp_image_l;
      mp_image_l = 0;
    }
  }

  tl::Color background_color () const
  {
    return m_bg;
  }

  tl::Color foreground_color () const
  {
    return m_fg;
  }

  tl::Color active_color () const
  {
    return m_ac;
  }

  virtual tl::PixelBuffer *bg_image ()
  {
    return mp_image_l ? mp_image_l : mp_image;
  }

  void transfer_to_image (const lay::DitherPattern &dp, const lay::LineStyles &ls, unsigned int width, unsigned int height)
  {
    if (mp_image_l) {
      unsigned int os = mp_image_l->width () / width;
      mp_image->blowup (*mp_image_l, os);
      bitmaps_to_image (fg_view_op_vector (), fg_bitmap_vector (), dp, ls, 1.0 / resolution (), mp_image_l, mp_image_l->width (), mp_image_l->height (), false, 0);
      mp_image_l->subsample (*mp_image, os, m_gamma);
    } else {
      bitmaps_to_image (fg_view_op_vector (), fg_bitmap_vector (), dp, ls, 1.0 / resolution (), mp_image, width, height, false, 0);
    }
    clear_fg_bitmaps ();
  }

  void make_background ()
  {
    if (mp_image_l && mp_image->width () > 0) {
      unsigned int os = mp_image_l->width () / mp_image->width ();
      mp_image_l->subsample (*mp_image, os, m_gamma);
    }
  }

private:
  tl::Color m_bg, m_fg, m_ac;
  tl::PixelBuffer *mp_image;
  tl::PixelBuffer *mp_image_l;
  double m_gamma;
};

/**
 *  @brief A simplistic monochrome canvas
 *
 *  NOTE: this canvas does not support background painting (currently the background objects
 *  do not support monochrome background painting anyway).
 *  Nor does it support subsampling (that would mean grayscale).
 */
class DetachedViewObjectCanvasMono
  : public BitmapViewObjectCanvas
{
public:
  DetachedViewObjectCanvasMono (bool bg, bool fg, bool ac, unsigned int width, unsigned int height)
    : BitmapViewObjectCanvas (width, height, 1.0),
      m_bg (bg), m_fg (fg), m_ac (ac)
  {
    //  .. nothing yet ..
  }

  ~DetachedViewObjectCanvasMono ()
  {
    clear_fg_bitmaps ();
  }

  tl::Color background_color () const
  {
    return m_bg ? 0xffffffff : 0;
  }

  tl::Color foreground_color () const
  {
    return m_fg ? 0xffffffff : 0;
  }

  tl::Color active_color () const
  {
    return m_ac ? 0xffffffff : 0;
  }

private:
  bool m_bg, m_fg, m_ac;
};

tl::PixelBuffer
LayoutCanvas::image (unsigned int width, unsigned int height) 
{
  return image_with_options (width, height, -1, -1, -1.0, tl::Color (), tl::Color (), tl::Color (), db::DBox ());
}

tl::PixelBuffer
LayoutCanvas::image_with_options (unsigned int width, unsigned int height, int linewidth, int oversampling, double resolution, tl::Color background, tl::Color foreground, tl::Color active, const db::DBox &target_box)
{
  if (oversampling <= 0) {
    oversampling = m_oversampling;
  }
  if (resolution <= 0.0) {
    resolution = 1.0 / oversampling;
  }
  if (linewidth <= 0) {
    linewidth = 1.0 / resolution + 0.5;
  }
  if (! background.is_valid ()) {
    background = background_color ();
  }
  if (! foreground.is_valid ()) {
    foreground = foreground_color ();
  }
  if (! active.is_valid ()) {
    active = active_color ();
  }

  //  TODO: for other architectures MonoLSB may not be the right format
  tl::PixelBuffer img (width, height);

  //  this may happen for BIG images:
  if (img.width () != width || img.height () != height) {
    throw tl::Exception (tl::to_string (tr ("Unable to create an image with size %dx%d pixels")), width, height);
  }

  img.fill (background.rgb ());

  //  provide canvas objects for the layout bitmaps and the foreground/background objects
  BitmapRedrawThreadCanvas rd_canvas;
  DetachedViewObjectCanvas vo_canvas (background, foreground, active, width * oversampling, height * oversampling, resolution, &img);

  //  compute the new viewport 
  db::DBox tb (target_box);
  if (tb.empty ()) {
    tb = m_viewport.target_box ();
  }
  Viewport vp (width * oversampling, height * oversampling, tb);
  vp.set_global_trans (m_viewport.global_trans ());

  lay::RedrawThread redraw_thread (&rd_canvas, mp_view);

  //  render the layout
  redraw_thread.start (0 /*synchronous*/, m_layers, vp, resolution, true);
  redraw_thread.stop (); // safety

  //  paint the background objects. It uses "img" to paint on.
  do_render_bg (vp, vo_canvas);

  //  paint the layout bitmaps
  rd_canvas.to_image (scaled_view_ops (linewidth), dither_pattern (), line_styles (), 1.0 / resolution, background, foreground, active, this, *vo_canvas.bg_image (), vp.width (), vp.height ());

  //  subsample current image to provide the background for the foreground objects
  vo_canvas.make_background ();

  //  render the foreground parts ..
  do_render (vp, vo_canvas, true);
  vo_canvas.transfer_to_image (dither_pattern (), line_styles (), width, height);

  do_render (vp, vo_canvas, false);
  vo_canvas.transfer_to_image (dither_pattern (), line_styles (), width, height);

  return img;
}

tl::BitmapBuffer
LayoutCanvas::image_with_options_mono (unsigned int width, unsigned int height, int linewidth, tl::Color background_c, tl::Color foreground_c, tl::Color active_c, const db::DBox &target_box)
{
  if (linewidth <= 0) {
    linewidth = 1;
  }

  bool background = background_c.is_valid () ? background_c.to_mono () : background_color ().to_mono ();
  bool foreground = foreground_c.is_valid () ? foreground_c.to_mono () : foreground_color ().to_mono ();
  bool active = active_c.is_valid () ? active_c.to_mono () : active_color ().to_mono ();

  //  provide canvas objects for the layout bitmaps and the foreground/background objects
  BitmapRedrawThreadCanvas rd_canvas;
  DetachedViewObjectCanvasMono vo_canvas (background, foreground, active, width, height);

  //  compute the new viewport
  db::DBox tb (target_box);
  if (tb.empty ()) {
    tb = m_viewport.target_box ();
  }
  Viewport vp (width, height, tb);
  vp.set_global_trans (m_viewport.global_trans ());

  lay::RedrawThread redraw_thread (&rd_canvas, mp_view);

  //  render the layout
  redraw_thread.start (0 /*synchronous*/, m_layers, vp, 1.0, true);
  redraw_thread.stop (); // safety

  tl::BitmapBuffer img (width, height);
  img.fill (background);

  rd_canvas.to_image_mono (scaled_view_ops (linewidth), dither_pattern (), line_styles (), linewidth, background, foreground, active, this, img, vp.width (), vp.height ());

  return img;
}

tl::PixelBuffer
LayoutCanvas::screenshot () 
{
  //  if required, start the redraw thread ..
  prepare_drawing ();

  tl::PixelBuffer img (m_viewport.width (), m_viewport.height ());
  img.fill (m_background);

  DetachedViewObjectCanvas vo_canvas (background_color (), foreground_color (), active_color (), m_viewport_l.width (), m_viewport_l.height (), resolution (), &img);

  //  and paint the background objects. It uses "img" to paint on.
  do_render_bg (m_viewport_l, vo_canvas);

  //  paint the layout bitmaps
  to_image (scaled_view_ops (1.0 / resolution ()), dither_pattern (), line_styles (), 1.0 / resolution (), background_color (), foreground_color (), active_color (), this, *vo_canvas.bg_image (), m_viewport_l.width (), m_viewport_l.height ());

  //  subsample current image to provide the background for the foreground objects
  vo_canvas.make_background ();

  //  render the foreground parts ..
  do_render (m_viewport_l, vo_canvas, true);
  vo_canvas.transfer_to_image (dither_pattern (), line_styles (), m_viewport.width (), m_viewport.height ());

  do_render (m_viewport_l, vo_canvas, false);
  vo_canvas.transfer_to_image (dither_pattern (), line_styles (), m_viewport.width (), m_viewport.height ());

  return img;
}

void
LayoutCanvas::resize_event (unsigned int width, unsigned int height)
{
  unsigned int w = width * dpr () + 0.5, h = height * dpr () + 0.5;
  unsigned int wl = width * m_oversampling * dpr () + 0.5, hl = height * m_oversampling * dpr () + 0.5;

  if (m_viewport.width () != w || m_viewport.height () != h ||
      m_viewport_l.width () != wl || m_viewport_l.height () != hl) {

    //  clear the image cache
    m_image_cache.clear ();

    //  set the viewport to the new size
    m_viewport.set_size (width * dpr () + 0.5, height * dpr () + 0.5);
    m_viewport_l.set_size (width * m_oversampling * dpr () + 0.5, height * m_oversampling * dpr () + 0.5);

    mouse_event_trans (db::DCplxTrans (1.0 / dpr ()) * m_viewport.trans ());
    do_redraw_all (true);
    viewport_changed_event ();

  }
}

void 
LayoutCanvas::update_viewport ()
{
  mouse_event_trans (db::DCplxTrans (1.0 / dpr ()) * m_viewport.trans ());
  for (service_iterator svc = begin_services (); svc != end_services (); ++svc) {
    (*svc)->update ();
  }
  do_redraw_all (false);
  viewport_changed_event ();
}

const db::DCplxTrans &
LayoutCanvas::global_trans () const
{
  return m_viewport.global_trans ();
}

void
LayoutCanvas::set_global_trans (const db::DCplxTrans &global_trans)
{
  m_viewport.set_global_trans (global_trans);
  m_viewport_l.set_global_trans (global_trans);
  update_viewport ();
}

void
LayoutCanvas::zoom_box (const db::DBox &box, bool precious)
{
  if (precious) {
    m_precious_box = box;
  }
  m_viewport.set_box (box);
  m_viewport_l.set_box (box);
  update_viewport ();
}

void
LayoutCanvas::zoom_trans (const db::DCplxTrans &trans)
{
  m_viewport.set_trans (trans);
  m_viewport_l.set_trans (db::DCplxTrans (double (m_oversampling)) * trans);
  update_viewport ();
}

bool
LayoutCanvas::drawing_finished ()
{
  bool f = m_drawing_finished;
  m_drawing_finished = false;
  return f;
}

void 
LayoutCanvas::do_end_of_drawing ()
{
  //  store the data into the open entries or discard if not compatible
  for (size_t i = 0; i < m_image_cache.size (); ++i) {
    if (m_image_cache [i].opened ()) {
      if (m_image_cache [i].equals (m_viewport_l, m_layers)) {
        m_image_cache.back ().close (store_data ());
      } else {
        m_image_cache.erase (m_image_cache.begin () + i);
        --i;
      }
    } 
  }

  set_default_cursor (lay::Cursor::none);

  m_drawing_finished = true;
}

void
LayoutCanvas::do_update_image ()
{
  update_image ();
}

#if defined(HAVE_QT)
void
LayoutCanvas::gtf_probe ()
{
  if (gtf::Recorder::instance () && gtf::Recorder::instance ()->recording ()) {
    gtf::Recorder::instance ()->probe (widget (), gtf::image_to_variant (screenshot ().to_image_copy ()));
  }
}
#endif

void
LayoutCanvas::redraw_all ()
{
  do_redraw_all ();
}

void
LayoutCanvas::do_redraw_all (bool force_redraw)
{
  stop_redraw ();

  if (! m_need_redraw) {
    m_need_redraw_layer.clear ();
  }

  m_need_redraw = true;
  m_redraw_clearing = true;
  if (force_redraw) {
    m_redraw_force_update = true;
  }

  //  redraw the background elements
  touch_bg ();

  update (); // produces a paintEvent()
}

void
LayoutCanvas::redraw_new (std::vector<lay::RedrawLayerInfo> &layers)
{
  m_image_cache.clear ();
  m_layers.swap (layers);
  do_redraw_all (true);
}

void
LayoutCanvas::redraw_selected (const std::vector<int> &layers)
{
  stop_redraw ();

  m_image_cache.clear ();

  if (! m_need_redraw) {
    m_redraw_clearing = false;
    m_need_redraw_layer.clear ();
  }

  m_need_redraw = true;
  m_need_redraw_layer.insert (m_need_redraw_layer.end (), layers.begin (), layers.end ());
  std::sort (m_need_redraw_layer.begin (), m_need_redraw_layer.end ());
  m_need_redraw_layer.erase (std::unique (m_need_redraw_layer.begin (), m_need_redraw_layer.end ()), m_need_redraw_layer.end ());
  m_redraw_force_update = true;

  update (); // produces a paintEvent()
}

void
LayoutCanvas::change_visibility (const std::vector <bool> &visible)
{
  stop_redraw ();
  mp_redraw_thread->change_visibility (visible);
  for (unsigned int i = 0; i < visible.size () && i < m_layers.size (); ++i) {
    m_layers [i].visible = visible [i];
  }

  if (! m_need_redraw) {
    m_redraw_clearing = false;
  }

  m_need_redraw = true;
  m_need_redraw_layer.clear ();

  update (); // produces a paintEvent()
}

void
LayoutCanvas::stop_redraw ()
{
  //  discard all open cache entries
  for (size_t i = 0; i < m_image_cache.size (); ++i) {
    if (m_image_cache [i].opened ()) {
      m_image_cache.erase (m_image_cache.begin () + i);
      --i;
    }
  }

  mp_redraw_thread->stop ();
}

void
LayoutCanvas::update_drawings ()
{
  update_image ();
}

void 
LayoutCanvas::signal_transfer_done () 
{
  m_do_update_image_dm ();
}

void 
LayoutCanvas::signal_end_of_drawing () 
{
  m_do_end_of_drawing_dm ();
}

}  // namespace lay

