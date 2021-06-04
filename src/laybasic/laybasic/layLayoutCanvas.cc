
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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


#include <QEvent>
#include <QPixmap>
#include <QBitmap>
#include <QPainter>
#include <QApplication>
#include <QBuffer>
#include <QWheelEvent>

#include "tlTimer.h"
#include "tlLog.h"
#include "tlAssert.h"
#include "layLayoutCanvas.h"
#include "layRedrawThread.h"
#include "layLayoutView.h"
#include "layMarker.h"
#include "gtf.h"

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

static void 
blowup (const QImage &src, QImage &dest, unsigned int os)
{
  unsigned int ymax = src.height ();
  unsigned int xmax = src.width ();

  for (unsigned int y = 0; y < ymax; ++y) {
    for (unsigned int i = 0; i < os; ++i) {
      const uint32_t *psrc = (const uint32_t *) src.scanLine (y);
      uint32_t *pdest = (uint32_t *) dest.scanLine (y * os + i);
      for (unsigned int x = 0; x < xmax; ++x) {
        for (unsigned int j = 0; j < os; ++j) {
          *pdest++ = *psrc;
        }
        ++psrc;
      }
    }
  }
}

static void 
subsample (const QImage &src, QImage &dest, unsigned int os, double g)
{
  //  TODO: this is probably not compatible with the endianess of SPARC ..
  
  //  LUT's for combining the RGB channels

  //  forward transformation table
  unsigned short lut1[256];
  for (unsigned int i = 0; i < 256; ++i) {
    double f = (65536 / (os * os)) - 1;
    lut1[i] = (unsigned short)std::min (f, std::max (0.0, floor (0.5 + pow (i / 255.0, g) * f)));
  } 

  //  backward transformation table
  unsigned char lut2[65536];
  for (unsigned int i = 0; i < 65536; ++i) {
    double f = os * os * ((65536 / (os * os)) - 1);
    lut2[i] = (unsigned char)std::min (255.0, std::max (0.0, floor (0.5 + pow (i / f, 1.0 / g) * 255.0)));
  } 

  //  LUT's for alpha channel

  //  forward transformation table
  unsigned short luta1[256];
  for (unsigned int i = 0; i < 256; ++i) {
    double f = (65536 / (os * os)) - 1;
    luta1[i] = (unsigned short)std::min (f, std::max (0.0, floor (0.5 + (i / 255.0) * f)));
  }

  //  backward transformation table
  unsigned char luta2[65536];
  for (unsigned int i = 0; i < 65536; ++i) {
    double f = os * os * ((65536 / (os * os)) - 1);
    luta2[i] = (unsigned char)std::min (255.0, std::max (0.0, floor (0.5 + (i / f) * 255.0)));
  }

  unsigned int ymax = dest.height ();
  unsigned int xmax = dest.width ();

  unsigned short *buffer = new unsigned short[xmax * 4];

  for (unsigned int y = 0; y < ymax; ++y) {

    {

      const unsigned char *psrc = src.scanLine (y * os);
      unsigned short *pdest = buffer;

      for (unsigned int x = 0; x < xmax; ++x) {

        pdest[0] = lut1[psrc[0]];
        pdest[1] = lut1[psrc[1]];
        pdest[2] = lut1[psrc[2]];
        pdest[3] = luta1[psrc[3]];
        psrc += 4;

        for (unsigned int j = os; j > 1; j--) {
          pdest[0] += lut1[psrc[0]];
          pdest[1] += lut1[psrc[1]];
          pdest[2] += lut1[psrc[2]];
          pdest[3] += luta1[psrc[3]];
          psrc += 4;
        }

        pdest += 4;

      }

    }

    for (unsigned int i = 1; i < os; ++i) {

      const unsigned char *psrc = src.scanLine (y * os + i);
      unsigned short *pdest = buffer;

      for (unsigned int x = 0; x < xmax; ++x) {

        for (unsigned int j = os; j > 0; j--) {
          pdest[0] += lut1[psrc[0]];
          pdest[1] += lut1[psrc[1]];
          pdest[2] += lut1[psrc[2]];
          pdest[3] += luta1[psrc[3]];
          psrc += 4;
        }

        pdest += 4;

      }

    }

    {

      unsigned char *pdest = dest.scanLine (y);
      const unsigned short *psrc = buffer;

      for (unsigned int x = 0; x < xmax; ++x) {
        *pdest++ = lut2[*psrc++];
        *pdest++ = lut2[*psrc++];
        *pdest++ = lut2[*psrc++];
        *pdest++ = luta2[*psrc++];
      }

    }

  }

  delete[] buffer;
}

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

LayoutCanvas::LayoutCanvas (QWidget *parent, lay::LayoutView *view, const char *name)
  : lay::ViewObjectWidget (parent, name), 
    mp_view (view),
    mp_image (0), mp_image_bg (0), mp_pixmap (0), 
    m_background (0), m_foreground (0), m_active (0),
    m_oversampling (1),
    m_dpr (1),
    m_need_redraw (false),
    m_redraw_clearing (false),
    m_redraw_force_update (true),
    m_update_image (true),
    m_do_update_image_dm (this, &LayoutCanvas::do_update_image),
    m_do_end_of_drawing_dm (this, &LayoutCanvas::do_end_of_drawing),
    m_image_cache_size (1)
{
#if QT_VERSION > 0x050000
  m_dpr = devicePixelRatio ();
#endif

  //  The gamma value used for subsampling: something between 1.8 and 2.2.
  m_gamma = 2.0;

  //  some reasonable initializations for the size
  m_viewport.set_size (100, 100); 
  m_viewport_l.set_size (m_viewport.width () * m_oversampling, m_viewport.height () * m_oversampling);

  mp_redraw_thread = new lay::RedrawThread (this, view);

  setBackgroundRole (QPalette::NoRole);
  set_colors (palette ().color (QPalette::Normal, QPalette::Background),
              palette ().color (QPalette::Normal, QPalette::Text),
              palette ().color (QPalette::Normal, QPalette::Mid));
  setAttribute (Qt::WA_NoSystemBackground);
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
  if (mp_pixmap) {
    delete mp_pixmap;
    mp_pixmap = 0;
  }
  if (mp_redraw_thread) {
    delete mp_redraw_thread;
    mp_redraw_thread = 0;
  }

  clear_fg_bitmaps ();
}

void
LayoutCanvas::key_event (unsigned int key, unsigned int buttons)
{
  if (! (buttons & lay::ShiftButton)) {
    if (int (key) == Qt::Key_Down) {
      emit down_arrow_key_pressed ();
    } else if (int (key) == Qt::Key_Up) {
      emit up_arrow_key_pressed ();
    } else if (int (key) == Qt::Key_Left) {
      emit left_arrow_key_pressed ();
    } else if (int (key) == Qt::Key_Right) {
      emit right_arrow_key_pressed ();
    }
  } else {
    if (int (key) == Qt::Key_Down) {
      emit down_arrow_key_pressed_with_shift ();
    } else if (int (key) == Qt::Key_Up) {
      emit up_arrow_key_pressed_with_shift ();
    } else if (int (key) == Qt::Key_Left) {
      emit left_arrow_key_pressed_with_shift ();
    } else if (int (key) == Qt::Key_Right) {
      emit right_arrow_key_pressed_with_shift ();
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
LayoutCanvas::set_colors (QColor background, QColor foreground, QColor active)
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

void
LayoutCanvas::prepare_drawing ()
{
  if (m_need_redraw) {

    BitmapViewObjectCanvas::set_size (m_viewport_l.width (), m_viewport_l.height (), 1.0 / double (m_oversampling * m_dpr));

    if (! mp_image ||
        (unsigned int) mp_image->width () != m_viewport_l.width () || 
        (unsigned int) mp_image->height () != m_viewport_l.height ()) {
      if (mp_image) {
        delete mp_image;
      }
      mp_image = new QImage (m_viewport_l.width (), m_viewport_l.height (), QImage::Format_RGB32);
#if QT_VERSION > 0x050000
      mp_image->setDevicePixelRatio (double (m_dpr));
#endif
      if (mp_pixmap) {
        delete mp_pixmap;
        mp_pixmap = 0;
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

      mp_redraw_thread->commit (m_layers, m_viewport_l, 1.0 / double (m_oversampling * m_dpr));

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
        mp_redraw_thread->start (mp_view->synchronous () ? 0 : mp_view->drawing_workers (), m_layers, m_viewport_l, 1.0 / double (m_oversampling * m_dpr), m_redraw_force_update);
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
  if (mp_pixmap) {
    delete mp_pixmap;
    mp_pixmap = 0;
  }
}

void
LayoutCanvas::paintEvent (QPaintEvent *)
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
        mp_image_bg = new QImage (*mp_image);

      } else {
        //  else reuse the saved image
        *mp_image = *mp_image_bg;
      }

      //  render the main bitmaps
      to_image (m_view_ops, dither_pattern (), line_styles (), background_color (), foreground_color (), active_color (), this, *mp_image, m_viewport_l.width (), m_viewport_l.height ());

      if (mp_pixmap) {
        delete mp_pixmap;
        mp_pixmap = 0;
      }

      m_update_image = false;

    }

    //  create a base pixmap consisting of the layout with background
    //  and static foreground objects

    if (! mp_pixmap || needs_update_static () || 
        mp_image->size ().width () != mp_pixmap->size ().width () * int (m_oversampling) ||
        mp_image->size ().height () != mp_pixmap->size ().height () * int (m_oversampling)) {

      if (mp_pixmap) {
        delete mp_pixmap;
      } 

      clear_fg_bitmaps ();
      do_render (m_viewport_l, *this, true);

      mp_pixmap = new QPixmap ();

      if (fg_bitmaps () > 0) {

        QImage full_image (*mp_image);
#if QT_VERSION > 0x050000
        full_image.setDevicePixelRatio (double (m_dpr));
#endif
        bitmaps_to_image (fg_view_op_vector (), fg_bitmap_vector (), dither_pattern (), line_styles (), &full_image, m_viewport_l.width (), m_viewport_l.height (), false, &m_mutex);

        //  render the foreground parts ..
        if (m_oversampling == 1) {
          *mp_pixmap = QPixmap::fromImage (full_image); // Qt 4.6.0 workaround
        } else {
          QImage subsampled_image (m_viewport.width (), m_viewport.height (), mp_image->format ());
#if QT_VERSION > 0x050000
          subsampled_image.setDevicePixelRatio (double (m_dpr));
#endif
          subsample (full_image, subsampled_image, m_oversampling, m_gamma);
          *mp_pixmap = QPixmap::fromImage (subsampled_image); // Qt 4.6.0 workaround
        }

      } else if (m_oversampling == 1) {

        *mp_pixmap = QPixmap::fromImage (*mp_image);

      } else {

        QImage subsampled_image (m_viewport.width (), m_viewport.height (), mp_image->format ());
#if QT_VERSION > 0x050000
        subsampled_image.setDevicePixelRatio (double (m_dpr));
#endif
        subsample (*mp_image, subsampled_image, m_oversampling, m_gamma);
        *mp_pixmap = QPixmap::fromImage (subsampled_image);

      }

    }

    //  erase any previous data
    clear_fg_bitmaps ();

    //  render dynamic foreground content
    do_render (m_viewport_l, *this, false);

    //  produce the pixmap first and then overdraw with dynamic content.
    QPainter painter (this);
    painter.drawPixmap (QPoint (0, 0), *mp_pixmap);

    if (fg_bitmaps () > 0) {

      QImage full_image (mp_image->size ().width (), mp_image->size ().height (), QImage::Format_ARGB32);
      full_image.fill (0);

#if QT_VERSION > 0x050000
      full_image.setDevicePixelRatio (double (m_dpr));
#endif
      bitmaps_to_image (fg_view_op_vector (), fg_bitmap_vector (), dither_pattern (), line_styles (), &full_image, m_viewport_l.width (), m_viewport_l.height (), false, &m_mutex);

      //  render the foreground parts ..
      if (m_oversampling == 1) {
        painter.drawPixmap (QPoint (0, 0), QPixmap::fromImage (full_image));
      } else {
        QImage subsampled_image (m_viewport.width (), m_viewport.height (), QImage::Format_ARGB32);
#if QT_VERSION > 0x050000
        subsampled_image.setDevicePixelRatio (double (m_dpr));
#endif
        subsample (full_image, subsampled_image, m_oversampling, m_gamma);
        painter.drawPixmap (QPoint (0, 0), QPixmap::fromImage (subsampled_image));
      }

    }

    //  erase dynamic bitmaps 
    clear_fg_bitmaps ();

#if QT_VERSION < 0x050000
    QApplication::syncX ();
#endif

  }

}

class DetachedViewObjectCanvas
  : public BitmapViewObjectCanvas
{
public:
  DetachedViewObjectCanvas (QColor bg, QColor fg, QColor ac, unsigned int width_l, unsigned int height_l, double resolution, QImage *img)
    : BitmapViewObjectCanvas (width_l, height_l, resolution),
      m_bg (bg), m_fg (fg), m_ac (ac), mp_image (img)
  {
    //  TODO: Good choice?
    m_gamma = 2.0;

    if (img->width () != int (width_l) || img->height () != int (height_l)) {
      mp_image_l = new QImage (width_l, height_l, img->format ());
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

  QColor background_color () const
  {
    return m_bg;
  }

  QColor foreground_color () const
  {
    return m_fg;
  }

  QColor active_color () const
  {
    return m_ac;
  }

  virtual QImage &bg_image () 
  {
    return mp_image_l ? *mp_image_l : *mp_image;
  }

  void transfer_to_image (const lay::DitherPattern &dp, const lay::LineStyles &ls, unsigned int width, unsigned int height)
  {
    if (mp_image_l) {
      unsigned int os = mp_image_l->width () / width;
      blowup (*mp_image, *mp_image_l, os);
      bitmaps_to_image (fg_view_op_vector (), fg_bitmap_vector (), dp, ls, mp_image_l, mp_image_l->width (), mp_image_l->height (), false, 0);
      subsample (*mp_image_l, *mp_image, os, m_gamma);
    } else {
      bitmaps_to_image (fg_view_op_vector (), fg_bitmap_vector (), dp, ls, mp_image, width, height, false, 0);
    }
    clear_fg_bitmaps ();
  }

  void make_background ()
  {
    if (mp_image_l && mp_image->width () > 0) {
      unsigned int os = mp_image_l->width () / mp_image->width ();
      subsample (*mp_image_l, *mp_image, os, m_gamma);
    }
  }

private:
  QColor m_bg, m_fg, m_ac;
  QImage *mp_image;
  QImage *mp_image_l;
  double m_gamma;
};

QImage 
LayoutCanvas::image (unsigned int width, unsigned int height) 
{
  return image_with_options (width, height, -1, -1, -1.0, QColor (), QColor (), QColor (), db::DBox (), false); 
}

QImage 
LayoutCanvas::image_with_options (unsigned int width, unsigned int height, int linewidth, int oversampling, double resolution, QColor background, QColor foreground, QColor active, const db::DBox &target_box, bool is_mono) 
{
  if (oversampling <= 0) {
    oversampling = m_oversampling;
  }
  if (linewidth <= 0) {
    linewidth = 1;
  }
  if (resolution <= 0.0) {
    resolution = 1.0 / oversampling;
  }
  if (background == QColor ()) {
    background = background_color ();
  }
  if (foreground == QColor ()) {
    foreground = foreground_color ();
  }
  if (active == QColor ()) {
    active = active_color ();
  }

  //  TODO: for other architectures MonoLSB may not be the right format
  QImage img (width, height, is_mono ? QImage::Format_MonoLSB : QImage::Format_RGB32);

  //  this may happen for BIG images:
  if (img.width () != int (width) || img.height () != int (height)) {
    throw tl::Exception (tl::to_string (QObject::tr ("Unable to create an image with size %dx%d pixels")), width, height);
  }

  if (is_mono) {
    //  in mono mode the background's color is white for green > 128 and black otherwise
    img.fill ((background.rgb () & 0x8000) >> 15);
  } else {
    img.fill (background.rgb ());
  }

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

  std::vector<lay::ViewOp> view_ops (m_view_ops); 
  if (linewidth > 1) {
    for (std::vector<lay::ViewOp>::iterator vo = view_ops.begin (); vo != view_ops.end (); ++vo) {
      vo->width (std::min (31, vo->width () * linewidth));
    }
  }

  lay::RedrawThread redraw_thread (&rd_canvas, mp_view);

  //  render the layout
  redraw_thread.start (0 /*synchronous*/, m_layers, vp, resolution, true);
  redraw_thread.stop (); // safety

  //  paint the background objects. It uses "img" to paint on.
  if (! is_mono) {

    do_render_bg (vp, vo_canvas);

    //  paint the layout bitmaps
    rd_canvas.to_image (view_ops, dither_pattern (), line_styles (), background, foreground, active, this, vo_canvas.bg_image (), vp.width (), vp.height ());

    //  subsample current image to provide the background for the foreground objects
    vo_canvas.make_background ();

    //  render the foreground parts ..
    do_render (vp, vo_canvas, true);
    vo_canvas.transfer_to_image (dither_pattern (), line_styles (), width, height);

    do_render (vp, vo_canvas, false);
    vo_canvas.transfer_to_image (dither_pattern (), line_styles (), width, height);

  } else {

    //  TODO: Painting of background objects???
    //  paint the layout bitmaps
    rd_canvas.to_image (view_ops, dither_pattern (), line_styles (), background, foreground, active, this, vo_canvas.bg_image (), vp.width (), vp.height ());

  }

  return img;
}

QImage 
LayoutCanvas::screenshot () 
{
  //  if required, start the redraw thread ..
  prepare_drawing ();

  QImage img (m_viewport.width (), m_viewport.height (), QImage::Format_RGB32);
  img.fill (m_background);

  DetachedViewObjectCanvas vo_canvas (background_color (), foreground_color (), active_color (), m_viewport_l.width (), m_viewport_l.height (), 1.0 / double (m_oversampling * m_dpr), &img);

  //  and paint the background objects. It uses "img" to paint on.
  do_render_bg (m_viewport_l, vo_canvas);

  //  paint the layout bitmaps
  to_image (m_view_ops, dither_pattern (), line_styles (), background_color (), foreground_color (), active_color (), this, vo_canvas.bg_image (), m_viewport_l.width (), m_viewport_l.height ());

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
LayoutCanvas::resizeEvent (QResizeEvent *)
{
  //  clear the image cache
  m_image_cache.clear ();

  //  set the viewport to the new size
  m_viewport.set_size (width () * m_dpr, height () * m_dpr);
  m_viewport_l.set_size (width () * m_oversampling * m_dpr, height () * m_oversampling * m_dpr);
  mouse_event_trans (db::DCplxTrans (1.0 / double (m_dpr)) * m_viewport.trans ());
  do_redraw_all (true);
  viewport_changed_event ();
}

void 
LayoutCanvas::update_viewport ()
{
  mouse_event_trans (db::DCplxTrans (1.0 / double (m_dpr)) * m_viewport.trans ());
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
}

void
LayoutCanvas::do_update_image ()
{
  update_image ();
}

bool
LayoutCanvas::event (QEvent *e) 
{
  if (e->type () == QEvent::MaxUser) {

    //  GTF probe event
    //  record the contents (the screenshot) as ASCII text
    if (gtf::Recorder::instance () && gtf::Recorder::instance ()->recording ()) {
      gtf::Recorder::instance ()->probe (this, gtf::image_to_variant (screenshot ()));
    }

    e->accept ();
    return true;

  } else {
    return QWidget::event (e);
  }
}

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

