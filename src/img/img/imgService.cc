
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


#include "dbUserObject.h"
#include "dbClipboard.h"
#include "dbEdgeProcessor.h"
#include "tlString.h"
#include "tlAssert.h"
#include "tlUtils.h"
#include "layPlugin.h"
#include "layRenderer.h"
#include "laySnap.h"
#include "layLayoutViewBase.h"
#include "laybasicConfig.h"
#if defined(HAVE_QT)
#  include "layProperties.h"
#  include "layTipDialog.h"
#endif
#include "tlExceptions.h"
#include "imgService.h"
#include "imgPlugin.h"
#if defined(HAVE_QT)
#  include "ui_AddNewImageDialog.h"
#endif

#if defined(HAVE_QT)
#  include <QApplication>
#endif

namespace img
{

// -------------------------------------------------------------

#if defined(HAVE_QT)
class AddNewImageDialog
  : public QDialog, 
    public Ui::AddNewImageDialog
{
public:
  AddNewImageDialog (QWidget *parent, img::Object *image_object)
    : QDialog (parent), mp_image_object (image_object)
  {
    setupUi (this);
    properties_frame->set_direct_image (image_object);
    properties_frame->update ();
  }

  virtual void accept ()
  {
    BEGIN_PROTECTED 

    properties_frame->set_direct_image (mp_image_object);
    properties_frame->apply ();

    if (mp_image_object->is_empty ()) {
      throw tl::Exception (tl::to_string (tr ("No data loaded for that image")));
    }

    QDialog::accept ();

    END_PROTECTED
  }

private:
  img::Object *mp_image_object;
};
#endif

// -------------------------------------------------------------

static void
draw_scanline (unsigned int level, const img::Object &image_object, tl::PixelBuffer &pxbuffer, int y, const db::Matrix3d &t, const db::Matrix3d &it, const db::DPoint &q1, const db::DPoint &q2)
{
  double source_width = image_object.width ();
  double source_height = image_object.height ();

  double x1 = t.trans (q1).x ();
  double x2 = t.trans (q2).x ();

  if (x1 > x2) {
    std::swap (x1, x2);
  }

  int xstart = int (std::max (0.0, std::min (floor (x1), double (pxbuffer.width ()))));
  int xstop = int (std::max (0.0, std::min (ceil (x2) + 1.0, double (pxbuffer.width ()))));

  db::DPoint p1 = it.trans (db::DPoint (xstart, y));
  db::DPoint p2 = it.trans (db::DPoint (xstop, y));

  db::DPoint qm = p1 + (p2 - p1) * 0.5;
  double xm = t.trans (qm).x ();

  if (level < 7 && xstop > xstart + 1 && fabs (xm - (xstart + xstop) / 2) > 1.0 && xm > xstart + 1 && xm < xstop - 1) {

    draw_scanline (level + 1, image_object, pxbuffer, y, t, it, q1, qm);
    draw_scanline (level + 1, image_object, pxbuffer, y, t, it, qm, q2);

  } else {

    double px = p1.x (), py = p1.y ();
    double dpx = (p2.x () - p1.x ()) / double (xstop - xstart);
    double dpy = (p2.y () - p1.y ()) / double (xstop - xstart);

    tl::color_t *scanline_data = pxbuffer.scan_line (pxbuffer.height () - y - 1) + xstart;
    tl::color_t *pixel_data = (tl::color_t *) image_object.pixel_data ();
    const unsigned char *mask_data = image_object.mask ();

    for (int x = xstart; x < xstop; ++x) {

      if (px >= 0 && px < source_width && py >= 0 && py < source_height) {

        size_t n = size_t (floor (px) + floor (py) * source_width);
        if (! mask_data || mask_data [n]) {
          *scanline_data = pixel_data [n];
        }

      }

      px += dpx; 
      py += dpy;

      ++scanline_data;

    }
  }
}

static void 
draw_image (const img::Object &image_object, const lay::Viewport &vp, lay::ViewObjectCanvas &canvas) 
{ 
  // TODO: currently, the images can only be rendered to a bitmap canvas ..
  lay::BitmapViewObjectCanvas *bmp_canvas = dynamic_cast<lay::BitmapViewObjectCanvas *> (&canvas);
  if (! bmp_canvas || ! bmp_canvas->bg_image ()) {
    return;
  }

  tl::PixelBuffer &image = *bmp_canvas->bg_image ();
  db::DBox source_image_box (0.0, 0.0, image_object.width (), image_object.height ());

  //  safety measure to avoid division by zero.
  if (image.width () < 1 || image.height () < 1) {
    return;
  }

  //  it the transformation from QImage pixel coordinates (in the "bottom first" orientation) into the
  //  image object's coordinate space.
  db::DVector dp (0.5 * image_object.width (), 0.5 * image_object.height ());
  db::Matrix3d t =  db::Matrix3d (vp.trans ()) * image_object.matrix () * db::Matrix3d::disp (-dp);
  db::Matrix3d it = t.inverted ();

  db::DBox image_box = source_image_box.transformed (t);

  int y1 = int (floor (std::max (0.0, image_box.bottom ())));
  int y2 = int (floor (std::min (double (image.height ()) - 1, image_box.top ())));

  for (int y = y1; y <= y2; ++y) {

    db::DEdge scanline (db::DPoint (image_box.left (), y), db::DPoint (image_box.right (), y));
    scanline.transform (it);

    //  clip the transformed scanline to the original image 
    std::pair<bool, db::DEdge> clipped = scanline.clipped_line (source_image_box);
    if (clipped.first) {
      draw_scanline (0, image_object, image, y, t, it, clipped.second.p1 (), clipped.second.p2 ());
    }

  }
}

static bool 
is_selected (const img::Object &image, const db::DPoint &pos, const db::DBox &vpbox, double enl, double &distance) 
{
  db::DPolygon b (image.image_box_poly (vpbox, db::DCplxTrans ()));
  db::DBox bb (b.box ());
  if (! bb.enlarged (db::DVector (enl, enl)).contains (pos)) {
    return false;
  }
  
  for (std::vector <db::DPoint>::const_iterator l = image.landmarks ().begin (); l != image.landmarks ().end (); ++l) {
    db::DPoint lp = image.matrix () * *l;
    if (db::DBox (lp, lp).enlarged (db::DVector (enl, enl)).contains (pos)) {
      distance = lp.distance (pos);
      return true; 
    }
  }

  if (db::inside_poly (b.begin_edge (), pos) < 0) {

    return false;

  } else {
      
    bool first = true;
    for (db::DPolygon::polygon_edge_iterator e = b.begin_edge (); ! e.at_end (); ++e) {
      double d = (*e).distance_abs (pos);
      if (first || d < distance) {
        distance = d;
      } 
      first = false;
    }  

    return true;

  }
}

static bool 
is_selected (const img::Object &image, const db::DBox &box)
{
  db::DBox b (image.box ());
  return (box.contains (b.p1 ()) && box.contains (b.p2 ()));
}

static int
obj2id (const db::DUserObject &obj)
{
  if (! obj.ptr ()) {
    return 0;
  } else {
    const img::Object *iobj = dynamic_cast<const img::Object *>(obj.ptr ());
    return iobj ? int (iobj->id ()) : 0;
  }
}

struct SortImagePtrByZOrder 
{
  bool operator() (const img::Object *a, const img::Object *b) const
  {
    return a->z_position () < b->z_position ();
  }

  bool operator() (const db::DUserObject *a, const db::DUserObject *b) const
  {
    return dynamic_cast<const img::Object &> (*a->ptr ()).z_position () < dynamic_cast<const img::Object &> (*b->ptr ()).z_position ();
  }
};

// -------------------------------------------------------------

View::View (img::Service *service, obj_iterator image_ref, img::View::Mode mode)
  : lay::ViewObject (service->widget ()), 
    mp_service (service), m_mode (mode), mp_image_object (0), m_image_ref (image_ref)
{
  //  .. nothing else ..
}

View::View (img::Service *service, const img::Object *object, img::View::Mode mode)
  : lay::ViewObject (service->widget ()), 
    mp_service (service), m_mode (mode), mp_image_object (object)
{
  //  .. nothing else ..
}

View::~View ()
{
  //  .. nothing else ..
}

void 
View::transform_by (const db::DCplxTrans &t)
{
  if (m_trans != t) {
    m_trans = t;
    redraw ();
  }
}

void 
View::render (const lay::Viewport &vp, lay::ViewObjectCanvas &canvas) 
{ 
  const img::Object *image = image_object ();
  if (! image) {
    return;
  }

  db::DPolygon image_box_poly = image->image_box_poly (db::DBox (-10, -10, vp.width () + 10, vp.height () + 10), vp.trans () * m_trans);
  db::Matrix3d t = db::Matrix3d (vp.trans () * m_trans) * image->matrix ();

  if (m_mode == mode_normal) {

    std::vector <db::Polygon> frame_p;

    db::DBox b = image_box_poly.box ();
    if (b.left () < std::numeric_limits<db::Coord>::min () / 2 ||
        b.right () > std::numeric_limits<db::Coord>::max () / 2 ||
        b.bottom () < std::numeric_limits<db::Coord>::min () / 2 ||
        b.top () > std::numeric_limits<db::Coord>::max () / 2) {
      return;
    }

    frame_p.push_back (db::Polygon (image_box_poly));

    db::EdgeProcessor ep;

    std::vector <db::Polygon> sized_p;
    ep.size (frame_p, db::Coord (-2.0 / canvas.resolution ()), sized_p);

    std::vector <db::Polygon> sized_pp;
    ep.size (frame_p, db::Coord (2.0 / canvas.resolution ()), sized_pp);

    std::vector <db::Polygon> result;
    ep.boolean (sized_pp, sized_p, result, db::BooleanOp::ANotB);

    //  obtain bitmap to render on
    lay::CanvasPlane *plane;
    std::vector <lay::ViewOp> vops;
    vops.reserve (2);
    vops.push_back (lay::ViewOp (canvas.background_color ().rgb (), lay::ViewOp::Copy, 0, 0, 0, lay::ViewOp::Rect, 1, 1));
    vops.push_back (lay::ViewOp (canvas.foreground_color ().rgb (), lay::ViewOp::Copy, 0, 6, 0, lay::ViewOp::Rect, 1, 2));
    plane = canvas.plane (vops);

    for (std::vector <db::Polygon>::const_iterator r = result.begin (); r != result.end (); ++r) {
      canvas.renderer ().draw (*r, db::CplxTrans (), plane, 0, 0, 0);
    }

  } else if (m_mode == mode_transient_move) {

    if (! image->landmarks ().empty ()) {

      //  obtain bitmap to render on (handles are located over the usual content)
      lay::CanvasPlane *plane_landmarks, *plane_frame;
      plane_frame = canvas.plane (lay::ViewOp (canvas.foreground_color ().rgb (), lay::ViewOp::Copy, 0, 0, 0, lay::ViewOp::Rect, 1, -1));
      //  plane_fill and plane are prio 3 and 4 to be above the normal selection which is 1 and 2
      std::vector <lay::ViewOp> ops;
      ops.push_back (lay::ViewOp (canvas.background_color ().rgb (), lay::ViewOp::Copy, 0, 0, 0, lay::ViewOp::Rect, 3, 3));
      ops.push_back (lay::ViewOp (canvas.foreground_color ().rgb (), lay::ViewOp::Copy, 0, 0, 0, lay::ViewOp::Rect, 1, 4));
      plane_landmarks = canvas.plane (ops);

      canvas.renderer ().draw (image_box_poly, db::DCplxTrans (), 0, plane_frame, 0, 0);

      const std::vector <db::DPoint> &handles = image->landmarks ();
      for (std::vector <db::DPoint>::const_iterator hb = handles.begin (); hb != handles.end (); ++hb)
      {
        db::DPoint p (hb->transformed (t));
        db::DBox box (p, p);
        double d = 2 / canvas.resolution ();
        canvas.renderer ().draw (box.enlarged (db::DVector (d, d)), db::DCplxTrans (), 0, plane_landmarks, 0, 0);
        canvas.renderer ().draw (db::DEdge (p + db::DVector (3.0 * d, 0), p - db::DVector (3.0 * d, 0)), db::DCplxTrans (), 0, plane_landmarks, 0, 0);
        canvas.renderer ().draw (db::DEdge (p + db::DVector (0, 3.0 * d), p - db::DVector (0, 3.0 * d)), db::DCplxTrans (), 0, plane_landmarks, 0, 0);
      }

    } else {

      //  obtain bitmap to render on
      lay::CanvasPlane *plane, *plane_fill;
      plane = canvas.plane (lay::ViewOp (canvas.foreground_color ().rgb (), lay::ViewOp::Copy, 0, 0, 0));
      //  plane_fill is prio 3 to be above the normal selection which is 1 and 2
      plane_fill = canvas.plane (lay::ViewOp (canvas.foreground_color ().rgb (), lay::ViewOp::Copy, 0, 0, 0, lay::ViewOp::Rect, 1, 3));

      canvas.renderer ().draw (image_box_poly, db::DCplxTrans (), 0, plane, 0, 0);

      db::DCoord cl = -0.5 * image->width ();
      db::DCoord cb = -0.5 * image->height ();
      db::DCoord cr = 0.5 * image->width ();
      db::DCoord ct = 0.5 * image->height ();

      std::vector <db::DPoint> handles;
      handles.reserve (8);
      handles.push_back (db::DPoint (cl, cb));
      handles.push_back (db::DPoint (cl, 0.5 * (cb + ct)));
      handles.push_back (db::DPoint (cl, ct));
      handles.push_back (db::DPoint (cr, cb));
      handles.push_back (db::DPoint (cr, 0.5 * (cb + ct)));
      handles.push_back (db::DPoint (cr, ct));
      handles.push_back (db::DPoint (0.5 * (cr + cl), ct));
      handles.push_back (db::DPoint (0.5 * (cr + cl), cb));

      for (std::vector <db::DPoint>::const_iterator hb = handles.begin (); hb != handles.end (); ++hb)
      {
        db::DBox box (hb->transformed (t), hb->transformed (t));
        db::DPolygon handle_box_poly (box.enlarged (db::DVector (3 / canvas.resolution (), 3 / canvas.resolution ())));
        canvas.renderer ().draw (handle_box_poly, db::DCplxTrans (), plane_fill, plane, 0, 0);
      }

    }

  } else {

    //  obtain bitmap to render on
    lay::CanvasPlane *plane;
    plane = canvas.plane (lay::ViewOp (canvas.foreground_color ().rgb (), lay::ViewOp::Copy, 0, 0, 0));

    canvas.renderer ().draw (image_box_poly, db::DCplxTrans (), 0, plane, 0, 0);

  }
}

// -------------------------------------------------------------
//  img::Service implementation

Service::Service (db::Manager *manager, lay::LayoutViewBase *view)
  : lay::BackgroundViewObject (view->canvas ()),
    lay::Editable (view),
    lay::Plugin (view),
    db::Object (manager),
    mp_view (view),
    mp_transient_view (0),
    m_move_mode (Service::move_none),
    m_moved_landmark (0),
    m_keep_selection_for_move (false),
    m_images_visible (true)
{ 
  // place images behind the grid
  z_order (-1);

  mp_view->annotations_changed_event.add (this, &Service::annotations_changed);
}

Service::~Service ()
{
  for (std::vector<img::View *>::iterator v = m_selected_image_views.begin (); v != m_selected_image_views.end (); ++v) {
    delete *v;
  }
  m_selected_image_views.clear ();
  clear_transient_selection ();
}

void
Service::annotations_changed ()
{
  //  NOTE: right now, we don't differentiate: every annotation change may be a change in an image too.
  //  We just forward this event as a potential image changed event
  images_changed_event ();
}

void
Service::show_images (bool f)
{
  if (m_images_visible != f) {
    m_images_visible = f;
    view ()->redraw ();
  }
}

bool 
Service::configure (const std::string &name, const std::string &value)
{
  if (name == cfg_images_visible) {

    bool v = true;
    tl::from_string (value, v);
    show_images (v);

    return true;

  } else {
    return false;
  }
}

void 
Service::config_finalize ()
{
  // .. nothing yet ..
}

void 
Service::clear_highlights ()
{
  for (std::vector<img::View *>::iterator v = m_selected_image_views.begin (); v != m_selected_image_views.end (); ++v) {
    (*v)->visible (false);
  }
}

void 
Service::restore_highlights ()
{
  for (std::vector<img::View *>::iterator v = m_selected_image_views.begin (); v != m_selected_image_views.end (); ++v) {
    (*v)->visible (true);
  }
}

void 
Service::highlight (unsigned int n)
{
  for (std::vector<img::View *>::iterator v = m_selected_image_views.begin (); v != m_selected_image_views.end (); ++v) {
    (*v)->visible (n-- == 0);
  }
}

img::Object * 
Service::insert_image (const img::Object &image)
{
  //  create the image and insert
  img::Object *new_image = new img::Object (image);
  const db::DUserObject &s = mp_view->annotation_shapes ().insert (db::DUserObject (new_image));

  //  NOTE: the const_cast will allow us to modify the object begin the DUserObject - that is not really clean
  return const_cast <img::Object *> (dynamic_cast <const img::Object *> (s.ptr ()));
}

/**
 *  @brief Helper function to determine which move mode to choose given a certain search box and img::Object
 */
static bool
dragging_what (const img::Object *iobj, const db::DBox &search_dbox, img::Service::MoveMode &mode, size_t &landmark, db::DPoint &p1)
{
  //  are we dragging a landmark?

  for (std::vector <db::DPoint>::const_iterator p = iobj->landmarks ().begin (); p != iobj->landmarks ().end (); ++p) {
    db::DPoint pt = iobj->matrix ().trans (*p);
    if (search_dbox.contains (pt)) {
      //  yes, we are:
      landmark = std::distance (iobj->landmarks ().begin (), p);
      mode = img::Service::move_landmark;
      return true;
    }
  }

  //  else check whether we are dragging a handle:

  mode = img::Service::move_all;
  p1 = search_dbox.center ();

  db::DVector dp (0.5 * iobj->width (), 0.5 * iobj->height ());
  db::DBox ref_box (search_dbox.transformed ((iobj->matrix () * db::Matrix3d::disp (-dp)).inverted ()));

  bool lo = ref_box.overlaps (db::DBox (0, 0, 0, iobj->height ()));
  bool ro = ref_box.overlaps (db::DBox (iobj->width (), 0, iobj->width (), iobj->height ()));
  bool bo = ref_box.overlaps (db::DBox (0, 0, iobj->width (), 0));
  bool to = ref_box.overlaps (db::DBox (0, iobj->height (), iobj->width (), iobj->height ()));
  bool all = ref_box.overlaps (db::DBox (0, 0, iobj->width (), iobj->height ()));

  if (lo) {
    if (bo) {
      mode = img::Service::move_ll;
      return true;
    } else if (to) {
      mode = img::Service::move_tl;
      return true;
    } else {
      mode = img::Service::move_l;
      return true;
    }
  } else if (ro) {
    if (bo) {
      mode = img::Service::move_lr;
      return true;
    } else if (to) {
      mode = img::Service::move_tr;
      return true;
    } else {
      mode = img::Service::move_r;
      return true;
    }
  } else if (bo) {
    mode = img::Service::move_b;
    return true;
  } else if (to) {
    mode = img::Service::move_t;
    return true;
  } else if (all) {
    mode = img::Service::move_all;
    return true;
  }
  
  return true;
}

bool 
Service::mouse_move_event (const db::DPoint & /*p*/, unsigned int /*buttons*/, bool /*prio*/) 
{
  //  .. nothing yet ..
  return false;
}

bool 
Service::begin_move (lay::Editable::MoveMode mode, const db::DPoint &p, lay::angle_constraint_type /*ac*/)
{
  //  cancel any pending move or drag operations 
  widget ()->drag_cancel (); // KLUDGE: every service does this to the same service manager

  //  compute search box
  double l = catch_distance ();
  db::DBox search_dbox = db::DBox (p, p).enlarged (db::DVector (l, l));

  //  choose move mode
  if (mode == lay::Editable::Selected) {

    m_move_mode = move_selected;
    m_p1 = p;
    m_trans = db::DTrans ();

    selection_to_view ();
    for (std::vector <img::View *>::iterator r = m_selected_image_views.begin (); r != m_selected_image_views.end (); ++r) {
      (*r)->thaw ();
    }

    return true;

  } else if (mode == lay::Editable::Partial) {
  
    //  test, whether we are moving a handle of one selected object
    for (std::map<obj_iterator, unsigned int>::const_iterator s = m_selected.begin (); s != m_selected.end (); ++s) {

      MoveMode mm = move_none;
      size_t ml = 0;
      obj_iterator si = s->first;

      const img::Object *iobj = dynamic_cast <const img::Object *> ((*si).ptr ());
      if (iobj && dragging_what (iobj, search_dbox, mm, ml, m_p1) && mm != move_all) {
          
        m_move_mode = mm;
        m_moved_landmark = ml;
        m_keep_selection_for_move = true;
          
        //  found a handle of one of the selected object: make the moved image the selection
        clear_selection ();
        m_selected.insert (std::make_pair (si, 0));
        m_current = *iobj;
        m_initial = m_current;
        m_selected_image_views.push_back (new img::View (this, &m_current, img::View::mode_transient_move));
        m_selected_image_views.back ()->thaw ();
        return true;

      }
      
    }

    //  nothing was found
    return false;

  } else if (mode == lay::Editable::Any) {
  
    m_move_mode = move_none;
    m_p1 = p;
    double dmin = std::numeric_limits <double>::max ();

    const db::DUserObject *robj = find_image (p, search_dbox, l, dmin);
    if (robj) {

      const img::Object *iobj = dynamic_cast<const img::Object *> (robj->ptr ());
      if (iobj) {

        MoveMode mm = move_none;
        size_t ml = 0;
        
        if (dragging_what (iobj, search_dbox, mm, ml, m_p1)) {

          m_move_mode = mm;
          m_moved_landmark = ml;
          m_keep_selection_for_move = false;
            
          //  found anything: make the moved image the selection
          clear_selection ();
          m_selected.insert (std::make_pair (mp_view->annotation_shapes ().iterator_from_pointer (robj), 0));
          m_current = *iobj;
          m_initial = m_current;
          m_selected_image_views.push_back (new img::View (this, &m_current, img::View::mode_transient_move));
          m_selected_image_views.back ()->thaw ();
          return true;

        }

      }
      
    }

    //  nothing was found
    return false;

  } else {
    return false;
  }
}

void
Service::move_transform (const db::DPoint &p, db::DFTrans tr, lay::angle_constraint_type /*ac*/)
{
  if (m_selected_image_views.empty () || m_selected.empty ()) {
    return;
  }

  if (m_move_mode == move_all) {

    db::DVector dp = p - db::DPoint ();

    m_current.transform (db::DTrans (dp) * db::DTrans (tr) * db::DTrans (-dp));

    //  display current images' parameters
    show_message ();

    m_selected_image_views [0]->redraw ();

  } else if (m_move_mode == move_selected) {

    m_trans *= db::DTrans (m_p1 - db::DPoint ()) * db::DTrans (tr) * db::DTrans (db::DPoint () - m_p1);

    for (std::vector<img::View *>::iterator r = m_selected_image_views.begin (); r != m_selected_image_views.end (); ++r) {
      (*r)->transform_by (db::DCplxTrans (m_trans));
    }

  }
}

void 
Service::move (const db::DPoint &p, lay::angle_constraint_type ac)
{
  if (m_selected_image_views.empty () || m_selected.empty ()) {
    return;
  }

  if (m_move_mode == move_selected) {

    db::DVector dp = p - m_p1;
    m_p1 = p;

    m_trans = db::DTrans (dp) * m_trans;

    for (std::vector<img::View *>::iterator r = m_selected_image_views.begin (); r != m_selected_image_views.end (); ++r) {
      (*r)->transform_by (db::DCplxTrans (m_trans));
    }

  } else if (m_move_mode == move_landmark) {

    std::vector <db::DPoint> li = m_initial.landmarks ();
    for (std::vector <db::DPoint>::iterator l = li.begin (); l != li.end (); ++l) {
      *l = m_initial.matrix ().trans (*l);
    }

    std::vector <db::DPoint> lm = li;
    lm [m_moved_landmark] = p;

    //  use angle_constraint to set the adjustment mode
    db::MatrixAdjustFlags::Flags adjust = db::MatrixAdjustFlags::All;
    if (ac == lay::AC_Ortho /*shift*/) {
      adjust = db::MatrixAdjustFlags::Displacement;
    } else if (ac == lay::AC_Diagonal /*ctrl*/) {
      adjust = db::MatrixAdjustFlags::Magnification;
    } else if (ac == lay::AC_Any /*ctrl+shift*/) {
      adjust = db::MatrixAdjustFlags::Shear;
    }

    //  realize transformation
    db::Matrix3d m (1.0);
    db::adjust_matrix (m, li, lm, adjust, int (m_moved_landmark));
    m_current.set_matrix (m * m_initial.matrix ());

    m_selected_image_views [0]->redraw ();

  } else {

    if (m_move_mode == move_all) {

      db::DVector dp = p - m_p1;
      m_p1 = p;

      m_current.transform (db::DTrans (dp));

    } else {

      m_current = m_initial;

      db::DVector dx (0.5 * m_current.width (), 0.5 * m_current.height ());
      db::Matrix3d it = (m_current.matrix () * db::Matrix3d::disp (-dx)).inverted ();
      db::DVector dp = it.trans (p) - it.trans (m_p1);

      double w = m_current.width ();
      double h = m_current.height ();

      db::DVector vv, v;
      
      if (m_move_mode == move_l) {
        vv = db::DVector (-dp.x (), 0.0);
      } else if (m_move_mode == move_r) {
        vv = db::DVector (dp.x (), 0.0);
      } else if (m_move_mode == move_b) {
        vv = db::DVector (0.0, -dp.y ());
      } else if (m_move_mode == move_t) {
        vv = db::DVector (0.0, dp.y ());
      } else if (m_move_mode == move_ll) {
        vv = db::DVector (-dp.x (), -dp.y ());
      } else if (m_move_mode == move_lr) {
        vv = db::DVector (dp.x (), -dp.y ());
      } else if (m_move_mode == move_tl) {
        vv = db::DVector (-dp.x (), dp.y ());
      } else if (m_move_mode == move_tr) {
        vv = db::DVector (dp.x (), dp.y ());
      }

      double min_scale = 1e-3;
      vv = db::DVector (std::max (-w * (1.0 - min_scale), vv.x ()), std::max (-h * (1.0 - min_scale), vv.y ()));

      if (m_move_mode == move_ll || m_move_mode == move_lr || m_move_mode == move_tl || m_move_mode == move_tr) {

        double fx = (w + vv.x ()) / w;
        double fy = (h + vv.y ()) / h;
        double f = std::max (fx, fy);

        vv = db::DVector (f * w - w, f * h - h);

      }

      if (m_move_mode == move_l) {
        v = db::DVector (-vv.x (), 0.0);
      } else if (m_move_mode == move_b) {
        v = db::DVector (0.0, -vv.y ());
      } else if (m_move_mode == move_ll) {
        v = db::DVector (-vv.x (), -vv.y ());
      } else if (m_move_mode == move_lr) {
        v = db::DVector (0.0, -vv.y ());
      } else if (m_move_mode == move_tl) {
        v = db::DVector (-vv.x (), 0.0);
      }

      double pw = (w + vv.x ()) / w;
      double ph = (h + vv.y ()) / h;

      db::Matrix3d m = m_current.matrix () * db::Matrix3d::disp (v + vv * 0.5) * db::Matrix3d::mag (pw, ph);
      if (m_current.is_valid_matrix (m)) {
        m_current.set_matrix (m);
      }

    }

    //  display current images' parameters
    show_message ();

    m_selected_image_views [0]->redraw ();

  }

  if (m_move_mode != move_selected) {
    show_message ();
  }
}

void 
Service::show_message ()
{
  //  display current images parameters
  /* don't do anything right now.
  std::string pos = std::string ("lx: ") + tl::micron_to_string (m_current.p2 ().x () - m_current.p1 ().x ()) 
                      + "  ly: " + tl::micron_to_string (m_current.p2 ().y () - m_current.p1 ().y ()) 
                      + "  l: " + tl::micron_to_string (m_current.p2 ().distance (m_current.p1 ()));
  view ()->message (pos);
  */
}

void 
Service::end_move (const db::DPoint &, lay::angle_constraint_type)
{
  if (! m_selected_image_views.empty () && ! m_selected.empty ()) {

    clear_transient_selection ();

    if (m_move_mode == move_selected) {

      //  replace the images that were moved:
      for (std::map<obj_iterator, unsigned int>::const_iterator s = m_selected.begin (); s != m_selected.end (); ++s) {

        const img::Object *iobj = dynamic_cast<const img::Object *> (s->first->ptr ());

        //  compute moved object and replace
        //  KLUDGE: this creates a copy of the data!
        img::Object *inew = new img::Object (*iobj);
        inew->transform (m_trans);
        int id = obj2id (mp_view->annotation_shapes ().replace (s->first, db::DUserObject (inew)));

        image_changed_event (id);

      }

      //  and make selection "visible"
      selection_to_view ();

    } else if (m_move_mode == move_landmark) {

      //  replace the image that was moved
      img::Object *inew = new img::Object (m_current);
      int id = obj2id (mp_view->annotation_shapes ().replace (m_selected.begin ()->first, db::DUserObject (inew)));
      image_changed_event (id);

      //  clear the selection (that was artificially created before)
      if (! m_keep_selection_for_move) {
        clear_selection ();
      } else {
        selection_to_view ();
      }

    } else if (m_move_mode != move_none) {

      //  replace the image that was moved
      img::Object *inew = new img::Object (m_current);
      int id = obj2id (mp_view->annotation_shapes ().replace (m_selected.begin ()->first, db::DUserObject (inew)));
      image_changed_event (id);

      //  clear the selection (that was artificially created before)
      if (! m_keep_selection_for_move) {
        clear_selection ();
      } else {
        selection_to_view ();
      }

    }

  }

  //  termine the operation
  m_move_mode = move_none;
}

const db::DUserObject *
Service::find_image (const db::DPoint &p, const db::DBox &search_box, double l, double &dmin, const std::map<img::Service::obj_iterator, unsigned int> *exclude)
{
  if (! m_images_visible) {
    return 0;
  }

  std::vector <const db::DUserObject *> images;

  //  get valid images and sort by reverse z order (top one first)
  lay::AnnotationShapes::touching_iterator r = mp_view->annotation_shapes ().begin_touching (search_box);
  while (! r.at_end ()) {
    const img::Object *image = dynamic_cast<const img::Object *> ((*r).ptr ());
    if (image && image->is_visible () && (! exclude || exclude->find (mp_view->annotation_shapes ().iterator_from_pointer (&*r)) == exclude->end ())) {
      images.push_back (&*r);
    }
    ++r;
  }

  std::stable_sort (images.begin (), images.end (), SortImagePtrByZOrder ());

  //  look for the "closest" image to the search box
  dmin = std::numeric_limits <double>::max ();
  const db::DUserObject *found = 0;

  for (std::vector <const db::DUserObject *>::const_iterator robj = images.begin (); robj != images.end (); ++robj) {
    double d = std::numeric_limits <double>::max ();
    if (is_selected (*dynamic_cast<const img::Object *> ((*robj)->ptr ()), p, mp_view->box (), l, d)) {
      found = *robj;
      dmin = d;
    }
  }

  return found;
}

void
Service::selection_to_view (img::View::Mode mode)
{
  clear_transient_selection ();
  image_selection_changed_event ();

  //  the selection objects need to be recreated since we destroyed the old images
  for (std::vector<img::View *>::iterator v = m_selected_image_views.begin (); v != m_selected_image_views.end (); ++v) {
    delete *v;
  }
  m_selected_image_views.clear ();

  m_selected_image_views.reserve (m_selected.size ());
  for (std::map<obj_iterator, unsigned int>::iterator r = m_selected.begin (); r != m_selected.end (); ++r) {
    r->second = (unsigned int) m_selected_image_views.size ();
    m_selected_image_views.push_back (new img::View (this, r->first, mode));
  }
}

db::DBox 
Service::selection_bbox ()
{
  db::DBox box;
  for (std::map<obj_iterator, unsigned int>::iterator r = m_selected.begin (); r != m_selected.end (); ++r) {
    const img::Object *iobj = dynamic_cast<const img::Object *> (r->first->ptr ());
    if (iobj) {
      box += iobj->box ();
    }
  }
  return box;
}

void 
Service::transform (const db::DCplxTrans &trans)
{
  //  replace the images that were transformed:
  for (std::map<obj_iterator, unsigned int>::const_iterator s = m_selected.begin (); s != m_selected.end (); ++s) {

    const img::Object *iobj = dynamic_cast<const img::Object *> (s->first->ptr ());

    //  compute transformed object and replace
    img::Object *inew = new img::Object (*iobj);
    inew->transform (trans);
    int id = obj2id (mp_view->annotation_shapes ().replace (s->first, db::DUserObject (inew)));
    image_changed_event (id);

  }

  selection_to_view ();
}

void 
Service::edit_cancel () 
{
  if (m_move_mode != move_none) {
    m_move_mode = move_none;
    selection_to_view ();
  }
}

void 
Service::cut ()
{
  if (has_selection ()) {

    //  copy & delete the selected images
    copy_selected ();
    del_selected ();

  }
}

void 
Service::copy ()
{
  //  copy the selected images
  copy_selected ();
}

void
Service::copy_selected ()
{
  //  extract all selected images and paste in "micron" space
  for (std::map<obj_iterator, unsigned int>::iterator r = m_selected.begin (); r != m_selected.end (); ++r) {
    r->second = (unsigned int) m_selected_image_views.size ();
    const img::Object *iobj = dynamic_cast<const img::Object *> (r->first->ptr ());
    db::Clipboard::instance () += new db::ClipboardValue<img::Object> (*iobj);
  }
}

void 
Service::paste ()
{
  if (db::Clipboard::instance ().begin () != db::Clipboard::instance ().end ()) {

    for (db::Clipboard::iterator c = db::Clipboard::instance ().begin (); c != db::Clipboard::instance ().end (); ++c) {
      const db::ClipboardValue<img::Object> *value = dynamic_cast<const db::ClipboardValue<img::Object> *> (*c);
      if (value) {
        img::Object *image = new img::Object (value->get ());
        mp_view->annotation_shapes ().insert (db::DUserObject (image));
      }
    }

  }
}

void 
Service::del ()
{
  if (has_selection ()) {

    //  delete the selected images
    del_selected ();

  }
}

void
Service::del_selected ()
{
  //  positions will hold a set of iterators that are to be erased
  std::vector <lay::AnnotationShapes::iterator> positions;
  positions.reserve (m_selected.size ());
  for (std::map<obj_iterator, unsigned int>::iterator r = m_selected.begin (); r != m_selected.end (); ++r) {
    positions.push_back (r->first);
  }

  //  clear selection
  clear_selection ();

  //  erase all and insert the ones that we want to keep 
  tl::sort (positions.begin (), positions.end ());  // HINT: must be tl::sort, not std::sort because gcc 3.2.3 has some strange namespace resolution problems ..
  mp_view->annotation_shapes ().erase_positions (positions.begin (), positions.end ());
}

bool
Service::has_selection ()
{
  return ! m_selected.empty ();
}

size_t
Service::selection_size ()
{
  return m_selected.size ();
}

bool
Service::has_transient_selection ()
{
  return mp_transient_view != 0;
}

void
Service::clear_previous_selection ()
{
  m_previous_selection.clear ();
}

void
Service::transient_to_selection ()
{
  if (mp_transient_view) {
    m_selected.insert (std::make_pair (mp_transient_view->image_ref (), 0));
    selection_to_view ();
  }
}

bool 
Service::select (obj_iterator obj, lay::Editable::SelectionMode mode)
{
  if (mode == lay::Editable::Replace || mode == lay::Editable::Add) {
    //  select
    if (m_selected.find (obj) == m_selected.end ()) {
      m_selected.insert (std::make_pair (obj, 0));
      return true;
    }
  } else if (mode == lay::Editable::Reset) {
    //  unselect
    if (m_selected.find (obj) != m_selected.end ()) {
      m_selected.erase (obj);
      return true;
    }
  } else {
    //  invert selection
    if (m_selected.find (obj) != m_selected.end ()) {
      m_selected.erase (obj);
    } else {
      m_selected.insert (std::make_pair (obj, 0));
    }
    return true;
  }
  return false;
}

void
Service::clear_selection ()
{
  select (db::DBox (), lay::Editable::Reset);

  //  clear the transient selection as well so there is no reference to any image left
  clear_transient_selection ();
}

double
Service::catch_distance ()
{
  return double (view ()->search_range ()) / widget ()->mouse_event_trans ().mag ();
}

double
Service::catch_distance_box ()
{
  return double (view ()->search_range_box ()) / widget ()->mouse_event_trans ().mag ();
}

double
Service::click_proximity (const db::DPoint &pos, lay::Editable::SelectionMode mode)
{
  //  compute search box
  double l = catch_distance ();
  db::DBox search_dbox = db::DBox (pos, pos).enlarged (db::DVector (l, l));

  //  for single-point selections either exclude the current selection or the
  //  accumulated previous selection from the search.
  const std::map<obj_iterator, unsigned int> *exclude = 0;
  if (mode == lay::Editable::Replace) {
    exclude = &m_previous_selection;
  } else if (mode == lay::Editable::Add) {
    exclude = &m_selected;
  } else if (mode == lay::Editable::Reset) {
    //  TODO: the finder should favor the current selection in this case.
  }

  //  point selection: look for the "closest" images
  double dmin = std::numeric_limits <double>::max ();
  const db::DUserObject *robj = find_image (pos, search_dbox, l, dmin, exclude);

  //  return the proximity value
  if (robj) {
    return dmin;
  } else {
    return lay::Editable::click_proximity (pos, mode); 
  } 
}

bool
Service::transient_select (const db::DPoint &pos)
{
  clear_transient_selection ();

  bool any_selected = false;

  //  compute search box
  double l = catch_distance ();
  db::DBox search_dbox = db::DBox (pos, pos).enlarged (db::DVector (l, l));

  //  point selection: look for the "closest" image
  double dmin = std::numeric_limits <double>::max ();
  const db::DUserObject *robj = find_image (pos, search_dbox, l, dmin, &m_previous_selection);

  //  create the transient marker for the object found 
  if (robj) {

    obj_iterator imin = mp_view->annotation_shapes ().iterator_from_pointer (robj);

    //  if in move mode (which also receives transient_select requests) the move will take the selection,
    //  hence only highlight the transient selection if it's part of the current selection.
    if (view ()->has_selection () && view ()->is_move_mode () && m_selected.find (imin) == m_selected.end ()) {
      return false;
    }

    //  HINT: there is no special style for "transient selection on images"
    if (mp_view->is_move_mode ()) {
      mp_transient_view = new img::View (this, imin, img::View::mode_transient_move);
    } else {
      mp_transient_view = new img::View (this, imin, img::View::mode_transient);
    }

    any_selected = true;

  }

  if (any_selected && ! editables ()->has_selection ()) {
    display_status (true);
  }

  return any_selected;
}

void
Service::clear_transient_selection ()
{
  if (mp_transient_view) {
    delete mp_transient_view;
    mp_transient_view = 0;
  }
}

bool
Service::select (const db::DBox &box, lay::Editable::SelectionMode mode)
{
  if (! m_images_visible) {
    return false;
  }

  bool needs_update = false;
  bool any_selected = false;

  //  clear before unless "add" is selected
  if (mode == lay::Editable::Replace) {
    if (! m_selected.empty ()) {
      m_selected.clear ();
      needs_update = true;
    }
  }

  //  for single-point selections either exclude the current selection or the
  //  accumulated previous selection from the search.
  const std::map<obj_iterator, unsigned int> *exclude = 0;
  if (mode == lay::Editable::Replace) {
    exclude = &m_previous_selection;
  } else if (mode == lay::Editable::Add) {
    exclude = &m_selected;
  } else if (mode == lay::Editable::Reset) {
    //  TODO: the finder should favor the current selection in this case.
  }

  if (box.empty ()) {

    //  unconditional selection
    if (mode == lay::Editable::Reset) {
      if (! m_selected.empty ()) {
        m_selected.clear ();
        needs_update = true;
      }
    } else {

      lay::AnnotationShapes::iterator rfrom = mp_view->annotation_shapes ().begin (); 
      lay::AnnotationShapes::iterator rto = mp_view->annotation_shapes ().end ();

      //  extract all images
      for (lay::AnnotationShapes::iterator r = rfrom; r != rto; ++r) {
        const img::Object *iobj = dynamic_cast<const img::Object *> ((*r).ptr ());
        if (iobj) {
          any_selected = true;
          if (select (r, mode)) {
            needs_update = true;
          }
        }
      }
    }

  } else {

    //  compute search box
    double l = box.is_point () ? catch_distance () : catch_distance_box ();
    db::DBox search_dbox = box.enlarged (db::DVector (l, l));

    if (! box.is_point ()) {

      //  box-selection
      lay::AnnotationShapes::touching_iterator r = mp_view->annotation_shapes ().begin_touching (search_dbox);
      while (! r.at_end ()) {
        const img::Object *iobj = dynamic_cast<const img::Object *> ((*r).ptr ());
        if (iobj && iobj->is_visible () && (! exclude || exclude->find (mp_view->annotation_shapes ().iterator_from_pointer (&*r)) == exclude->end ())) {
          if (is_selected (*iobj, box)) {
            any_selected = true;
            if (select (mp_view->annotation_shapes ().iterator_from_pointer (&*r), mode)) {
              needs_update = true;
            }
          }
        }
        ++r;
      }

    } else {

      //  point selection: look for the "closest" image
      double dmin = std::numeric_limits <double>::max ();
      const db::DUserObject *robj = find_image (box.p1 (), search_dbox, l, dmin, exclude);

      //  select the one that was found
      if (robj) {
        select (mp_view->annotation_shapes ().iterator_from_pointer (robj), mode);
        m_previous_selection.insert (std::make_pair (mp_view->annotation_shapes ().iterator_from_pointer (robj), mode));
        needs_update = true;
      }

    }

  }

  //  if required, update the list of image objects to display the selection
  if (needs_update) {
    selection_to_view (box.is_point () && view ()->is_move_mode () ? img::View::mode_transient_move : img::View::mode_normal);
  }

  if (any_selected) {
    display_status (false);
  }

  //  return true if at least one element was selected
  return any_selected;
}

void 
Service::display_status (bool transient)
{
  View *selected_view = transient ? mp_transient_view : (m_selected_image_views.size () == 1 ? m_selected_image_views [0] : 0);
  if (! selected_view) {
    view ()->message (std::string ());
  } else {

    const img::Object *image = selected_view->image_object ();

    std::string msg;
    if (! transient) {
      msg = tl::to_string (tr ("selected: "));
    }
    msg += tl::sprintf (tl::to_string (tr ("image(%dx%d)")), image->width (), image->height ());
    view ()->message (msg);

  }
}

#if defined(HAVE_QT)
std::vector<lay::PropertiesPage *>
Service::properties_pages (db::Manager *manager, QWidget *parent)
{
  std::vector<lay::PropertiesPage *> pages;
  pages.push_back (new img::PropertiesPage (this, manager, parent));
  return pages;
}
#endif

void 
Service::get_selection (std::vector <obj_iterator> &sel) const
{
  sel.clear ();
  sel.reserve (m_selected.size ());

  //  positions will hold a set of iterators that are to be erased
  for (std::map<obj_iterator, unsigned int>::const_iterator r = m_selected.begin (); r != m_selected.end (); ++r) {
    sel.push_back (r->first);
  }
}

void
Service::erase_image (obj_iterator pos)
{
  //  clear the selection
  clear_selection ();
    
  //  erase the object
  mp_view->annotation_shapes ().erase (pos);
}

void
Service::erase_image_by_id (size_t id)
{
  obj_iterator img = object_iter_by_id (id);
  if (img != mp_view->annotation_shapes ().end ()) {
    erase_image (img);
  }
}

void
Service::change_image (obj_iterator pos, const img::Object &to)
{
  //  replace the object
  img::Object *inew = new img::Object (to);
  int id = obj2id (mp_view->annotation_shapes ().replace (pos, db::DUserObject (inew)));
  image_changed_event (id);

  //  and make selection "visible"
  selection_to_view ();
}

void
Service::change_image_by_id (size_t id, const img::Object &to)
{
  obj_iterator img = object_iter_by_id (id);
  if (img != mp_view->annotation_shapes ().end ()) {
    change_image (img, to);
  }
}

void
Service::render_bg (const lay::Viewport &vp, lay::ViewObjectCanvas &canvas)
{
  if (! m_images_visible) {
    return;
  }

  std::vector <const img::Object *> images;

  lay::AnnotationShapes::touching_iterator user_object = mp_view->annotation_shapes ().begin_touching (vp.box ());
  while (! user_object.at_end ()) {
    const img::Object *image = dynamic_cast <const img::Object *> ((*user_object).ptr ());
    if (image && image->is_visible ()) {
      images.push_back (image);
    }
    ++user_object;
  }

  std::stable_sort (images.begin (), images.end (), SortImagePtrByZOrder ());

  for (std::vector <const img::Object *>::const_iterator i = images.begin (); i != images.end (); ++i) {
    draw_image (**i, vp, canvas);
  }
}

ImageIterator
Service::begin_images () const
{
  return ImageIterator (mp_view->annotation_shapes ().begin (), mp_view->annotation_shapes ().end ());
}

void 
Service::menu_activated (const std::string &symbol)
{
  if (symbol == "img::clear_all_images") {

    if (manager ()) {
      manager ()->transaction (tl::to_string (tr ("Clear all images")));
    }
    clear_images ();
    if (manager ()) {
      manager ()->commit ();
    }

  } else if (symbol == "img::add_image") {

#if defined(HAVE_QT)
    if (! images_visible ()) {
      lay::TipDialog td (QApplication::activeWindow (),
                    tl::to_string (tr ("Images are not visible. If you add an image you will not see it.\n\n"
                                                "Choose 'View/Show Images' to make images visible.")),
                    "add-image-while-not-visible",
                    lay::TipDialog::okcancel_buttons);
      lay::TipDialog::button_type button = lay::TipDialog::null_button;
      td.exec_dialog (button);
      if (button == lay::TipDialog::cancel_button) {
        //  Don't bother the user with more dialogs.
        return;
      }
    }
#endif

    add_image ();

  } else if (symbol == "img::bring_to_back") {
    bring_to_back ();
  } else if (symbol == "img::bring_to_front") {
    bring_to_front ();
  } else {
    lay::Plugin::menu_activated (symbol);
  }
}

void 
Service::bring_to_back ()
{
  int min_z = 0;
  int max_z = 0;

  for (obj_iterator user_object = mp_view->annotation_shapes ().begin (); user_object != mp_view->annotation_shapes ().end (); ++user_object) {
    const img::Object *i = dynamic_cast <const img::Object *> ((*user_object).ptr ());
    if (i) {
      if (m_selected.find (user_object) != m_selected.end ()) {
        max_z = std::max (max_z, i->z_position ());
      } else {
        min_z = std::min (max_z, i->z_position ());
      }
    }
  }

  for (obj_iterator user_object = mp_view->annotation_shapes ().begin (); user_object != mp_view->annotation_shapes ().end (); ++user_object) {
    const img::Object *i = dynamic_cast <const img::Object *> ((*user_object).ptr ());
    if (i) {
      img::Object new_obj (*i);
      if (m_selected.find (user_object) != m_selected.end ()) {
        new_obj.set_z_position (i->z_position () - max_z - 1);
      } else {
        new_obj.set_z_position (i->z_position () - min_z);
      }
      change_image (user_object, new_obj);
    }
  }
}

void 
Service::bring_to_front ()
{
  int min_z = 0;
  int max_z = 0;

  for (obj_iterator user_object = mp_view->annotation_shapes ().begin (); user_object != mp_view->annotation_shapes ().end (); ++user_object) {
    const img::Object *i = dynamic_cast <const img::Object *> ((*user_object).ptr ());
    if (i) {
      if (m_selected.find (user_object) == m_selected.end ()) {
        max_z = std::max (max_z, i->z_position ());
      } else {
        min_z = std::min (max_z, i->z_position ());
      }
    }
  }

  for (obj_iterator user_object = mp_view->annotation_shapes ().begin (); user_object != mp_view->annotation_shapes ().end (); ++user_object) {
    const img::Object *i = dynamic_cast <const img::Object *> ((*user_object).ptr ());
    if (i) {
      img::Object new_obj (*i);
      if (m_selected.find (user_object) == m_selected.end ()) {
        new_obj.set_z_position (i->z_position () - max_z - 1);
      } else {
        new_obj.set_z_position (i->z_position () - min_z);
      }
      change_image (user_object, new_obj);
    }
  }
}

int
Service::top_z_position () const
{
  int z = 0;
  for (obj_iterator user_object = mp_view->annotation_shapes ().begin (); user_object != mp_view->annotation_shapes ().end (); ++user_object) {
    const img::Object *i = dynamic_cast <const img::Object *> ((*user_object).ptr ());
    if (i) {
      z = std::max (z, i->z_position ());
    }
  }

  return z + 1;
}

void 
Service::add_image ()
{
#if defined(HAVE_QT)
  img::Object *new_image = new img::Object ();

  AddNewImageDialog dialog (QApplication::activeWindow (), new_image);
  if (dialog.exec ()) {

    clear_selection ();

    if (manager ()) {
      manager ()->transaction (tl::to_string (tr ("Add image")));
    }
    new_image->set_z_position (top_z_position ());
    mp_view->annotation_shapes ().insert (db::DUserObject (new_image));
    if (manager ()) {
      manager ()->commit ();
    }

  } else {
    delete new_image;
  }
#endif
}

void
Service::clear_images ()
{
  lay::AnnotationShapes::iterator rfrom = mp_view->annotation_shapes ().begin (); 
  lay::AnnotationShapes::iterator rto = mp_view->annotation_shapes ().end ();

  //  clear selection
  clear_selection ();

  //  extract all images
  std::vector <lay::AnnotationShapes::iterator> positions;
  for (lay::AnnotationShapes::iterator r = rfrom; r != rto; ++r) {
    const img::Object *iobj = dynamic_cast <const img::Object *> (r->ptr ());
    if (iobj) {
      positions.push_back (r);
    }
  }

  //  we can erase these positions after having sorted them
  tl::sort (positions.begin (), positions.end ());  // HINT: must be tl::sort, not std::sort because gcc 3.2.3 has some strange namespace resolution problems ..
  mp_view->annotation_shapes ().erase_positions (positions.begin (), positions.end ());
}

const Object *
Service::object_by_id (size_t id) const
{
  obj_iterator i = object_iter_by_id (id);
  if (i == mp_view->annotation_shapes ().end ()) {
    return 0;
  } else {
    return dynamic_cast <const img::Object *> (i->ptr ());
  }
}

Service::obj_iterator 
Service::object_iter_by_id (size_t id) const
{
  //  TODO: this is a O(1) lookup, thus potentially slow.
  //  However, in non-editable mode, maintaining a table is not that straightforward ...
  for (obj_iterator user_object = mp_view->annotation_shapes ().begin (); user_object != mp_view->annotation_shapes ().end (); ++user_object) {
    const img::Object *i = dynamic_cast <const img::Object *> ((*user_object).ptr ());
    if (i && i->id () == id) {
      return user_object;
    }
  }

  return mp_view->annotation_shapes ().end ();
}

// -------------------------------------------------------------

} // namespace img


