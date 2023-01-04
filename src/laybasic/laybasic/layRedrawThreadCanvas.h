
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


#ifndef HDR_layBitmapRedrawThreadCanvas
#define HDR_layBitmapRedrawThreadCanvas

#include "dbBox.h"
#include "dbVector.h"
#include "dbTrans.h"
#include "layViewOp.h"
#include "layBitmapRenderer.h"
#include "tlPixelBuffer.h"
#include "tlThreads.h"

#include <vector>

namespace lay {

class CanvasPlane;
class Bitmap;
class Drawings;
class DitherPattern;
class LineStyles;

class RedrawThreadCanvas
{
public:
  /**
   *  @brief Constructor
   */
  RedrawThreadCanvas () 
    : m_resolution (1.0), m_width (0), m_height (0)
  { }

  /**
   *  @brief Destructor
   */
  virtual ~RedrawThreadCanvas () { }

  /**
   *  @brief Signal that a transfer has been done
   *
   *  This method is called (from the redraw thread) if a transfer has been performed asynchronously
   */
  virtual void signal_transfer_done () { }

  /**
   *  @brief Signal that the drawing has ended
   *
   *  This method is called (from the redraw thread) once the drawing has ended.
   */
  virtual void signal_end_of_drawing () { }

  /**
   *  @brief Returns true, if shifting is supported
   */
  virtual bool shift_supported () const
  {
    return false;
  }

  /**
   *  @brief Prepare the given number of planes with shifting
   *
   *  This method is called from RedrawThread::start (), not from the
   *  redraw thread.
   *
   *  @param nlayers The number of layers to prepare
   *  @param width The width of the canvas
   *  @param height The height of the canvas
   *  @param shifting The shift vector by which the original image should be shifted to form the background or 0 if no shifting is required
   *  @param layers The set of plane indexes to initialize (if null, all planes are initialized). A negative value initializes the drawing planes.
   *  @param resolution The resolution in which the image is drawn
   *  @param drawings The custom drawing interface which is responsible to draw user objects
   */
  virtual void prepare (unsigned int /*nlayers*/, unsigned int width, unsigned int height, double resolution, const db::Vector * /*shift_vector*/, const std::vector<int> * /*planes*/, const lay::Drawings * /*drawings*/) 
  {
    m_resolution = resolution;
    m_width = width;
    m_height = height;
  }

  /**
   *  @brief Set a plane
   *
   *  This method is called from the redraw thread to transfer data for a certain plane.
   *  The planes have been reserved before with init_plane ().
   */
  virtual void set_plane (unsigned int n, const lay::CanvasPlane *plane) = 0;

  /**
   *  @brief Set a plane for the drawing number d and index n within the drawing.
   *
   *  This method is called from the redraw thread.
   */
  virtual void set_drawing_plane (unsigned int d, unsigned int n, const lay::CanvasPlane *plane) = 0;
  
  /** 
   *  @brief Create a new, unassociated drawing plane
   */
  virtual lay::CanvasPlane *create_drawing_plane () = 0;

  /** 
   *  @brief Initialize a drawing plane for drawing on plane number n
   */
  virtual void initialize_plane (lay::CanvasPlane *plane, unsigned int n) = 0;

  /** 
   *  @brief Initialize a drawing plane for drawing on drawing d and plane number n
   */
  virtual void initialize_plane (lay::CanvasPlane *plane, unsigned int d, unsigned int n) = 0;

  /**
   *  @brief Lock the plane sets against changes by other threads
   */
  void lock () 
  {
    m_mutex.lock ();
  }

  /**
   *  @brief Unlock the plane sets against changes by other threads
   */
  void unlock () 
  {
    m_mutex.unlock ();
  }

  /**
   *  @brief Access to the mutex object
   */
  tl::Mutex &mutex ()
  {
    return m_mutex;
  }

  /**
   *  @brief Get the resolution value
   */
  double resolution () const
  {
    return m_resolution;
  }

  /**
   *  @brief Get the canvas width
   */
  unsigned int canvas_width () const
  {
    return m_width;
  }

  /**
   *  @brief Get the canvas height
   */
  unsigned int canvas_height () const
  {
    return m_height;
  }

  /**
   *  @brief Provide the renderer
   */
  virtual lay::Renderer *create_renderer () = 0;

private:
  tl::Mutex m_mutex;
  double m_resolution;
  unsigned int m_width, m_height;
};

class BitmapCanvasData
{
public:
  /**
   *  @brief Constructor
   */
  BitmapCanvasData ();

  /**
   *  @brief Destructor
   */
  ~BitmapCanvasData ();

  /**
   *  @brief Copy constructor
   */
  BitmapCanvasData (const BitmapCanvasData &data)
    : m_width (0), m_height (0)
  {
    operator= (data);
  }

  /**
   *  @brief Constructor from raw plan buffers
   */
  BitmapCanvasData (const std::vector <lay::Bitmap *> &plane_buffers, const std::vector <std::vector <lay::Bitmap *> > &drawing_plane_buffers, unsigned int width, unsigned int height);

  /**
   *  @brief Assignment
   */
  BitmapCanvasData &operator= (const BitmapCanvasData &data);

  /**
   *  @brief Fetches the data into the given buffers
   */
  void fetch (std::vector <lay::Bitmap *> &plane_buffers, std::vector <std::vector <lay::Bitmap *> > &drawing_plane_buffers, unsigned int &width, unsigned int &height) const;

  /**
   *  @brief Gets a value indicating whether we can fetch the data
   */
  bool can_fetch (const std::vector <lay::Bitmap *> &plane_buffers, const std::vector <std::vector <lay::Bitmap *> > &drawing_plane_buffers, unsigned int width, unsigned int height) const;

  /**
   *  @brief Swap with another data object
   */
  void swap (BitmapCanvasData &other);

private:
  void clear_planes ();
  static void assign (std::vector <lay::Bitmap *> &to, const std::vector <lay::Bitmap *> &from);
  static void assign(std::vector <std::vector <lay::Bitmap *> > &to, const std::vector <std::vector <lay::Bitmap *> > &from);

  std::vector <lay::Bitmap *> mp_plane_buffers;
  std::vector <std::vector <lay::Bitmap *> > mp_drawing_plane_buffers;
  unsigned int m_width, m_height;
};

class BitmapRedrawThreadCanvas
  : public RedrawThreadCanvas
{
public:
  /**
   *  @brief Constructor
   */
  BitmapRedrawThreadCanvas ();

  /**
   *  @brief Destructor
   */
  virtual ~BitmapRedrawThreadCanvas ();

  /**
   *  @brief Returns true, if shifting is supported
   */
  virtual bool shift_supported () const;
  
  /**
   *  @brief Prepare the given number of planes
   *
   *  This method is called from RedrawThread::start (), not from the
   *  redraw thread.
   */
  virtual void prepare (unsigned int nlayers, unsigned int width, unsigned int height, double resolution, const db::Vector *shift_vector, const std::vector<int> *planes, const lay::Drawings *drawings);
  
  /**
   *  @brief Test a plane with the given index for emptiness
   */
  bool is_plane_empty (unsigned int n);

  /**
   *  @brief Set a plane
   *
   *  This method is called from the redraw thread to transfer data for a certain plane.
   *  The planes have been reserved before with prepare ().
   */
  virtual void set_plane (unsigned int n, const lay::CanvasPlane *plane);

  /**
   *  @brief Set a plane for the drawing number d and index n within the drawing.
   *
   *  This method is called from the redraw thread.
   */
  virtual void set_drawing_plane (unsigned int d, unsigned int n, const lay::CanvasPlane *plane);
  
  /** 
   *  @brief Create a new, unassociated drawing plane
   */
  virtual lay::CanvasPlane *create_drawing_plane ();

  /** 
   *  @brief Initialize a drawing plane for drawing on plane number n
   */
  virtual void initialize_plane (lay::CanvasPlane *plane, unsigned int n);

  /** 
   *  @brief Initialize a drawing plane for drawing on drawing d and plane number n
   */
  virtual void initialize_plane (lay::CanvasPlane *plane, unsigned int d, unsigned int n);

  /**
   *  @brief Provide the renderer
   */
  virtual lay::Renderer *create_renderer () 
  { 
    return new lay::BitmapRenderer (m_width, m_height, resolution ()); 
  }

  /**
   *  @brief Transfer the content to a PixelBuffer
   */
  void to_image (const std::vector <lay::ViewOp> &view_ops, const lay::DitherPattern &dp, const lay::LineStyles &ls, double dpr, tl::Color background, tl::Color foreground, tl::Color active, const lay::Drawings *drawings, tl::PixelBuffer &img, unsigned int width, unsigned int height);

  /**
   *  @brief Transfer the content to a BitmapBuffer (monochrome)
   */
  void to_image_mono (const std::vector <lay::ViewOp> &view_ops, const lay::DitherPattern &dp, const lay::LineStyles &ls, double dpr, bool background, bool foreground, bool active, const lay::Drawings *drawings, tl::BitmapBuffer &img, unsigned int width, unsigned int height);

  /**
   *  @brief Gets the current bitmap data as a BitmapCanvasData object
   */
  BitmapCanvasData store_data () const
  {
    return BitmapCanvasData (mp_plane_buffers, mp_drawing_plane_buffers, m_width, m_height);
  }

  /**
   *  @brief Gets a value indicating whether we can restore the given data object
   */
  bool can_restore_data (const BitmapCanvasData &data) const
  {
    return data.can_fetch (mp_plane_buffers, mp_drawing_plane_buffers, m_width, m_height);
  }

  /**
   *  @brief Restores the data 
   */
  void restore_data (const BitmapCanvasData &data)
  {
    data.fetch (mp_plane_buffers, mp_drawing_plane_buffers, m_width, m_height);
  }

private:
  void clear_planes ();

  std::vector <lay::Bitmap *> mp_plane_buffers;
  std::vector <std::vector <lay::Bitmap *> > mp_drawing_plane_buffers;
  unsigned int m_width, m_height;
};

}

#endif

