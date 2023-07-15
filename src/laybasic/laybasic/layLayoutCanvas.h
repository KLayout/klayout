
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


#ifndef HDR_layLayoutCanvas
#define HDR_layLayoutCanvas

#include "laybasicCommon.h"
#include "dbTrans.h"
#include "dbBox.h"
#include "layViewport.h"
#include "layViewOp.h"
#include "layViewObject.h"
#include "layBitmap.h"
#include "tlColor.h"
#include "layDrawing.h"
#include "layDitherPattern.h"
#include "layLineStyles.h"
#include "layRedrawThreadCanvas.h"
#include "layRedrawLayerInfo.h"
#include "tlDeferredExecution.h"
#include "tlThreads.h"

#include <vector>
#include <map>
#include <set>
#include <utility>

namespace lay
{

class LayoutViewBase;
class RedrawThread;

/**
 *  @brief A class representing one entry in the image cache
 */

class ImageCacheEntry
{
public:
  /**
   *  @brief Constructor
   *  On creation, the entry is in "open" state and will be closed once the data has been 
   *  committed with "close".
   *  "Precious" entries will be preserved as far as possible.
   */
  ImageCacheEntry (const lay::Viewport &vp, const std::vector<lay::RedrawLayerInfo> &layers, bool precious);

  /**
   *  @brief Returns a value indicating whether the entry complies with the given viewport
   */
  bool equals (const lay::Viewport &vp, const std::vector<lay::RedrawLayerInfo> &layers) const;

  /**
   *  @brief Closes the entry and stores the given data
   */
  void close (const BitmapCanvasData &data);

  /**
   *  @brief Returns a value indicating whether the entry is opened
   */
  bool opened () const
  {
    return m_opened;
  }

  /**
   *  @brief Returns a value indicating whether the entry is precious
   */
  bool precious () const
  {
    return m_precious;
  }

  /**
   *  @brief Sets a value indicating whether the entry is precious
   */
  void set_precious (bool p)
  {
    m_precious = p;
  }

  /**
   *  @brief Gets the stored bitmap data
   */
  const BitmapCanvasData &data () const
  {
    return m_data;
  }

  /**
   *  @brief Swaps this entry with another one
   */
  void swap (ImageCacheEntry &other);

  /**
   *  @brief Returns a string representing this entry
   */
  std::string to_string () const;

private:
  bool m_opened;
  db::DCplxTrans m_trans;
  std::vector<lay::RedrawLayerInfo> m_layers;
  unsigned int m_width, m_height;
  bool m_precious;
  BitmapCanvasData m_data;
};
/** 
 *  @brief A "canvas" object to paint layouts on
 *
 *  The purpose of this object is to provide a canvas QWidget
 *  and to manage the auxiliary objects like rulers etc.
 */

class LAYBASIC_PUBLIC LayoutCanvas
  : public lay::ViewObjectUI,
    public lay::BitmapViewObjectCanvas,
    public lay::BitmapRedrawThreadCanvas,
    public lay::Drawings
{
public:
  LayoutCanvas (lay::LayoutViewBase *view);
  ~LayoutCanvas ();

#if defined(HAVE_QT)
  /**
   *  @brief Initializes the widgets
   *
   *  This method needs to be called after the constructor to establish the drawing widget.
   */
  virtual void init_ui (QWidget *parent);
#endif

  void set_colors (tl::Color background, tl::Color foreground, tl::Color active);

  /**
   *  @brief Set the view ops for the layers
   *
   *  The view_ops vector must correspond to the layers-vector from a separate 
   *  "redraw_new" call.
   *  For performance reasons it is swapped with the existing one.
   */
  void set_view_ops (std::vector <lay::ViewOp> &view_ops);

  /**
   *  @brief Get the view ops for the layers
   *
   *  This call delivers the view op vector from the layer set_view_ops call.
   */
  const std::vector <lay::ViewOp> &get_view_ops () const
  { 
    return m_view_ops; 
  }

  tl::PixelBuffer screenshot ();
  tl::PixelBuffer image (unsigned int width, unsigned int height);
  tl::PixelBuffer image_with_options (unsigned int width, unsigned int height, int linewidth, int oversampling, double resolution, tl::Color background, tl::Color foreground, tl::Color active_color, const db::DBox &target_box);
  tl::BitmapBuffer image_with_options_mono (unsigned int width, unsigned int height, int linewidth, tl::Color background, tl::Color foreground, tl::Color active, const db::DBox &target_box);

  void update_image ();

  /**
   *  @brief Specifies the global transformation which is always applied first
   */
  void set_global_trans (const db::DCplxTrans &global_trans);

  /**
   *  @brief Gets the global transformation
   */
  const db::DCplxTrans &global_trans () const;

  /**
   *  @brief Place the viewport to the box specified.
   *
   *  The box gives the target box for the viewport. It may have a different aspect ratio
   *  than the viewport (in pixels) which is important if the size changes.
   *  Then, the original target box will be used which may result in a better fit.
   *  If the "precious" flag is set, the zoom box will be regarded "precious" and kept
   *  in the cache for longer.
   */
  void zoom_box (const db::DBox &box, bool precious = false);

  /**
   *  @brief Set the transformation directly
   *
   *  This sets the transformation of the viewport directly. Thus, an arbitrary transformation
   *  may be specified.
   */
  void zoom_trans (const db::DCplxTrans &trans);

  /**
   *  @brief Free internal resources, in particular pixmaps
   */
  void free_resources (); 

  /**
   *  @brief Stop the redraw thread
   */
  void stop_redraw ();

  /**
   *  @brief Issue a new redraw request
   *
   *  The layers vector gets swapped with the current one for performance reasons.
   */
  void redraw_new (std::vector<lay::RedrawLayerInfo> &layers);

  /**
   *  @brief Get the redraw layer info
   *
   *  This method delivers the redraw layer info vector as set by the last redraw_new call.
   */
  const std::vector<lay::RedrawLayerInfo> &get_redraw_layers () const
  {
    return m_layers;
  }

  /**
   *  @brief Issue a full redraw request
   */
  void redraw_all ();

  /**
   *  @brief Issue a redraw request on selected layers
   *
   *  The index must correspond to lay::RedrawLayerInfo objects from former redraw_new 
   *  calls.
   */
  void redraw_selected (const std::vector<int> &layers);

  /**
   *  @brief Set the oversampling factor
   *
   *  An oversampling of 2 means that 2 pixels are combined into one.
   */
  void set_oversampling (unsigned int os);

  /**
   *  @brief Gets the oversampling factor
   */
  unsigned int oversampling () const
  {
    return m_oversampling;
  }

  /**
   *  @brief Set high resolution mode (utilize full DPI on high-DPI displays)
   */
  void set_highres_mode (bool hrm);

  /**
   *  @brief Gets the high resolution mode flag
   */
  bool highres_mode () const
  {
    return m_hrm;
  }

  /**
   *  @brief Gets the default device pixel ratio for this canvas
   */
  double dpr () const;

  /**
   *  @brief Sets the depth of the image cache
   */
  void set_image_cache_size (size_t len);

  /**
   *  @brief Change the visibility
   *
   *  The vector must correspond to the  lay::RedrawLayerInfo objects from former redraw_new
   */
  void change_visibility (const std::vector <bool> &visible);

  /**
   *  @brief Set the dither pattern object
   */
  void set_dither_pattern (const lay::DitherPattern &p);

  /**
   *  @brief Get the dither pattern object
   */
  const lay::DitherPattern &dither_pattern () const
  {
    return m_dither_pattern;
  }

  /**
   *  @brief Set the line styles object
   */
  void set_line_styles (const lay::LineStyles &s);

  /**
   *  @brief Get the line styles object
   */
  const lay::LineStyles &line_styles () const
  {
    return m_line_styles;
  }

  /**
   *  @brief Reimplementation of ViewObjectCanvas: Resolution
   */
  double resolution () const;

  /**
   *  @brief Reimplementation of ViewObjectCanvas: Background color 
   */
  tl::Color background_color () const
  { 
    return tl::Color (m_background);
  }

  /**
   *  @brief Reimplementation of ViewObjectCanvas: Foreground color 
   */
  tl::Color foreground_color () const
  { 
    return tl::Color (m_foreground);
  }

  /**
   *  @brief Reimplementation of ViewObjectCanvas: Active color 
   */
  tl::Color active_color () const
  { 
    return tl::Color (m_active);
  }

  /**
   *  @brief Reimplementation of ViewObjectCanvas: background image
   */
  tl::PixelBuffer *bg_image ()
  {
    return mp_image;
  }

  /** 
   *  @brief Reimplementation of RedrawThreadCanvas: signal end of drawing
   */
  void signal_end_of_drawing ();

  /** 
   *  @brief Reimplementation of RedrawThreadCanvas: signal transfer
   */
  void signal_transfer_done ();

  /**
   *  @brief Access to the viewport object
   */
  const lay::Viewport &viewport () const
  {
    return m_viewport;
  }

  /**
   *  @brief Gets (and resets) a flag indicating that drawing has finished
   */
  bool drawing_finished ();

  /**
   *  @brief An event indicating that the viewport was changed.
   *  If the viewport (the rectangle that is shown) changes, this event is fired.
   */
  tl::Event viewport_changed_event;

  //  key events
  tl::Event left_arrow_key_pressed;
  tl::Event up_arrow_key_pressed;
  tl::Event right_arrow_key_pressed;
  tl::Event down_arrow_key_pressed;
  tl::Event left_arrow_key_pressed_with_shift;
  tl::Event up_arrow_key_pressed_with_shift;
  tl::Event right_arrow_key_pressed_with_shift;
  tl::Event down_arrow_key_pressed_with_shift;

private:
  lay::LayoutViewBase *mp_view;
  tl::PixelBuffer *mp_image;
  tl::PixelBuffer *mp_image_bg;
  tl::PixelBuffer *mp_image_fg;
  db::DBox m_precious_box;
  lay::Viewport m_viewport, m_viewport_l;
  tl::color_t m_background;
  tl::color_t m_foreground;
  tl::color_t m_active;
  std::vector <lay::ViewOp> m_view_ops;
  lay::DitherPattern m_dither_pattern;
  lay::LineStyles m_line_styles;
  std::map<unsigned int, std::vector <lay::ViewOp> > m_scaled_view_ops;
  unsigned int m_oversampling;
  bool m_hrm;
  double m_gamma;

  bool m_need_redraw;
  bool m_redraw_clearing;
  bool m_redraw_force_update;
  bool m_update_image;
  bool m_drawing_finished;
  std::vector<int> m_need_redraw_layer;
  std::vector<lay::RedrawLayerInfo> m_layers;

  lay::RedrawThread *mp_redraw_thread;
  tl::DeferredMethod<LayoutCanvas> m_do_update_image_dm;
  tl::DeferredMethod<LayoutCanvas> m_do_end_of_drawing_dm;

  std::vector<ImageCacheEntry> m_image_cache;
  size_t m_image_cache_size;

  tl::Mutex m_mutex;

  virtual void key_event (unsigned int key, unsigned int buttons);
  virtual void resize_event (unsigned int width, unsigned int height);
#if defined(HAVE_QT)
  virtual void gtf_probe ();
  virtual void paint_event ();
#endif

  //  implementation of the lay::Drawings interface
  void update_drawings ();

  //  must be called after updating m_viewport
  void update_viewport ();

  void do_update_image ();
  void do_end_of_drawing ();
  void do_redraw_all (bool force_redraw = true);

  void prepare_drawing ();
  const std::vector<ViewOp> &scaled_view_ops (unsigned int lw);
};

} //  namespace lay

#endif

