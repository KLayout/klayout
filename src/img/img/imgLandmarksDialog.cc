
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

#include "imgLandmarksDialog.h"
#include "imgService.h"
#include "layLayoutView.h"

namespace img
{

// ---------------------------------------------------------------------------------------------
//  A landmark marker 

class IMG_PUBLIC LandmarkMarker
  : public lay::ViewObject
{
public: 
  /**
   *  @brief Constructor attaching to a certain object
   */
  LandmarkMarker (lay::ViewService *service, const db::DPoint &pos, bool selected)
    : lay::ViewObject (service->ui ()),
      mp_service (service), m_pos (pos), m_selected (selected), m_position_set (true)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Constructor attaching to a certain object
   */
  LandmarkMarker (lay::ViewService *service, bool selected)
    : lay::ViewObject (service->ui ()),
      mp_service (service), m_pos (), m_selected (selected), m_position_set (false)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief The destructor
   */
  ~LandmarkMarker ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Reset the position
   */
  void reset_position ()
  {
    if (m_position_set) {
      m_position_set = false;
      redraw ();
    }
  }

  /**
   *  @brief Set the position
   */
  void set_position (const db::DPoint &p)
  {
    if (m_pos != p || ! m_position_set) {
      m_position_set = true;
      m_pos = p;
      redraw ();
    }
  }

  /**
   *  @brief Get the position
   */
  const db::DPoint &position () const
  {
    return m_pos;
  }

private:
  lay::ViewService *mp_service;
  db::DPoint m_pos;
  bool m_selected;
  bool m_position_set;

  virtual void render (const lay::Viewport &vp, lay::ViewObjectCanvas &canvas)
  {
    if (! m_position_set) {
      return;
    }

    int basic_width = int(0.5 + 1.0 / canvas.resolution ());

    //  obtain bitmap to render on
    lay::CanvasPlane *plane_frame = 0, *plane_fill = 0;

    std::vector <lay::ViewOp> vops;
    vops.reserve (2);
    vops.push_back (lay::ViewOp (canvas.background_color ().rgb (), lay::ViewOp::Copy, 0, 0, 0, lay::ViewOp::Rect, 3 * basic_width, 1));
    vops.push_back (lay::ViewOp (canvas.foreground_color ().rgb (), lay::ViewOp::Copy, 0, 0, 0, lay::ViewOp::Rect, 1, 2));

    lay::CanvasPlane *plane = canvas.plane (vops);

    if (m_selected) {
      plane_fill = plane_frame = plane;
    } else {
      plane_frame = plane;
    }

    int pixel_size = 2;
    double s = vp.trans ().inverted ().ctrans (pixel_size * basic_width);
    canvas.renderer ().draw (db::DBox (m_pos - db::DVector (s, s), m_pos + db::DVector (s, s)), vp.trans (), plane_fill, plane_frame, 0, 0);
    canvas.renderer ().draw (db::DEdge (m_pos - db::DVector (0, s * 3.0), m_pos + db::DVector (0, s * 3.0)), vp.trans (), plane_fill, plane_frame, 0, 0);
    canvas.renderer ().draw (db::DEdge (m_pos - db::DVector (s * 3.0, 0), m_pos + db::DVector (s * 3.0, 0)), vp.trans (), plane_fill, plane_frame, 0, 0);
  }

  //  no copying nor default construction
  LandmarkMarker (const LandmarkMarker &d);
  LandmarkMarker &operator= (const LandmarkMarker &d);
  LandmarkMarker ();
};

// ---------------------------------------------------------------------------------------------
//  Navigator service definition and implementation

class LandmarkEditorService
  : public lay::ViewService
{
public:
  LandmarkEditorService (lay::LayoutViewBase *view, img::Object *img)
    : lay::ViewService (view->canvas ()), 
      mp_image (img), m_selected (-1), m_dragging (false),
      m_mode (LandmarksDialog::None)
  {
    update ();
  }

  ~LandmarkEditorService ()
  {
    drag_cancel ();
    clear ();
  }

  bool mouse_release_event (const db::DPoint & /*p*/, unsigned int /*buttons*/, bool /*prio*/) 
  { 
    // ...
    return false;
  }

  void set_mode (LandmarksDialog::mode_t mode)
  {
    if (mode != m_mode) {

      m_mode = mode;
      drag_cancel ();

      if (m_mode == LandmarksDialog::Move) {
        
        // ..

      } else if (m_mode == LandmarksDialog::Add) {

        m_selected = int (mp_image->landmarks ().size ());

        update ();
        
        ui ()->grab_mouse (this, false);
        m_dragging = true;

      } else if (m_mode == LandmarksDialog::Delete) {

        // ..

      }

    }
  }

  bool mouse_click_event (const db::DPoint &p, unsigned int /*buttons*/, bool prio) 
  { 
    if (prio) {

      if (m_dragging) {

        if (m_mode == LandmarksDialog::Add) {

          img::Object::landmarks_type lm = mp_image->landmarks ();
          while (m_selected >= int (lm.size ())) {
            lm.push_back (db::DPoint ());
          }
          if (m_selected >= 0) {
            lm [m_selected] = p;
          }
          mp_image->set_landmarks (lm);

          m_selected = int (lm.size ());

          update ();
          
        } else if (m_mode == LandmarksDialog::Move) {

          img::Object::landmarks_type lm = mp_image->landmarks ();
          if (m_selected >= 0 && m_selected < int (lm.size ())) {
            lm [m_selected] = p;
          }
          mp_image->set_landmarks (lm);

          m_selected = -1;

          update ();

          ui ()->grab_mouse (this, false);
          m_dragging = false;

        }

      } else {

        int search_range = 5; // TODO: make_variable?
        double l = double (search_range) / ui ()->mouse_event_trans ().mag ();
        db::DBox search_box = db::DBox (p, p).enlarged (db::DVector (l, l));

        m_selected = -1;
        int li = 0;
        for (std::vector<db::DPoint>::const_iterator l = mp_image->landmarks ().begin (); l != mp_image->landmarks ().end (); ++l, ++li) {
          if (search_box.contains (*l)) {
            m_selected = li;
            break;
          }
        }

        if (m_mode == LandmarksDialog::Add) {

          //  no action yet.

        } else if (m_mode == LandmarksDialog::Move) {

          //  no action yet.
          update ();
          m_dragging = true;

        } else if (m_mode == LandmarksDialog::Delete) {

          if (m_selected >= 0 && m_selected < int (mp_image->landmarks ().size ())) {

            img::Object::landmarks_type lm = mp_image->landmarks ();
            lm.erase (lm.begin () + m_selected);
            mp_image->set_landmarks (lm);

            m_selected = -1;
            update ();

          }

        }

      }

      return true;

    } else {
      return false;
    }
  }

  void update_landmarks ()
  {
    drag_cancel ();
    update_internal ();
  }

  bool mouse_press_event (const db::DPoint & /*p*/, unsigned int /*buttons*/, bool /*prio*/) 
  { 
    // ..
    return false;
  }

  bool mouse_move_event (const db::DPoint &p, unsigned int /*buttons*/, bool prio) 
  { 
    if (prio) {

      if (m_dragging) {

        if (m_mode == LandmarksDialog::Add) {
          set_cursor (lay::Cursor::cross);
        } else {
          set_cursor (lay::Cursor::size_all);
        }

      } else if (! m_dragging) {

        int search_range = 5; // TODO: make_variable?
        double l = double (search_range) / ui ()->mouse_event_trans ().mag ();
        db::DBox search_box = db::DBox (p, p).enlarged (db::DVector (l, l));

        int li = 0;
        int selected = -1;
        for (std::vector<db::DPoint>::const_iterator l = mp_image->landmarks ().begin (); l != mp_image->landmarks ().end (); ++l, ++li) {
          if (search_box.contains (*l)) {
            selected = li;
            break;
          }
        }

        set_cursor (lay::Cursor::none);
        if (selected >= 0) {
          if (m_mode == LandmarksDialog::Move) {
            set_cursor (lay::Cursor::size_all);
          } else if (m_mode == LandmarksDialog::Delete) {
            set_cursor (lay::Cursor::pointing_hand);
          }
        }

      }

      return true;

    } else {
      return false;
    }
  }

  void drag_cancel ()
  {
    if (m_dragging) {
      m_dragging = false;
    }

    ui ()->ungrab_mouse (this);
  }

  void set_colors (tl::Color /*background*/, tl::Color /*color*/)
  {
    // ...
  }

  int selected_index () const
  {
    return m_selected;
  }

  /**
   *  @brief An event indicating that the image was updated
   *  This event is fired if the image was changed in any way.
   */
  tl::Event updated_event;

private:
  img::Object *mp_image;
  std::vector<LandmarkMarker *> m_markers;
  int m_selected;
  bool m_dragging;
  img::LandmarksDialog::mode_t m_mode;

  void clear ()
  {
    for (std::vector<LandmarkMarker *>::iterator m = m_markers.begin (); m != m_markers.end (); ++m) {
      delete *m;
    }
    m_markers.clear ();
  }

  void update_internal ()
  {
    clear ();

    int li = 0;
    for (std::vector<db::DPoint>::const_iterator l = mp_image->landmarks ().begin (); l != mp_image->landmarks ().end (); ++l, ++li) {
      if (li != m_selected) {
        LandmarkMarker *m = new LandmarkMarker (this, *l, false);
        m_markers.push_back (m);
      }
    }
  }

  void update ()
  {
    update_internal ();
    updated_event ();
  }
};

// -------------------------------------------------------------------------
//  LandmarksDialog implementation

LandmarksDialog::LandmarksDialog (QWidget *parent, img::Object &img)
  : QDialog (parent), m_mode (None)
{
  mp_original_image = &img;

  setupUi (this);

  mp_image = navigator->setup (lay::Dispatcher::instance (), &img);

  connect (new_pb, SIGNAL (clicked ()), this, SLOT (update_mode ()));
  connect (delete_pb, SIGNAL (clicked ()), this, SLOT (update_mode ()));
  connect (move_pb, SIGNAL (clicked ()), this, SLOT (update_mode ()));

  mp_service = new LandmarkEditorService (navigator->view (), mp_image);
  navigator->activate_service (mp_service);

  mp_service->updated_event.add (this, &LandmarksDialog::landmarks_updated);

  new_pb->setChecked (true);
  mp_service->set_mode (Add);
  landmarks_updated ();
}

LandmarksDialog::~LandmarksDialog ()
{
  if (mp_service) {
    delete mp_service;
    mp_service = 0;
  }
}

void
LandmarksDialog::update_mode ()
{
  mode_t new_mode = None;

  if (sender () == new_pb) {
    new_mode = Add;
  } else if (sender () == move_pb) {
    new_mode = Move;
  } else if (sender () == delete_pb) {
    new_mode = Delete;
  }

  QList<QListWidgetItem *> sel = landmark_list->selectedItems ();
  if (new_mode == Delete && sel.size () > 0) {

    std::set <int> selected;
    for (QList<QListWidgetItem *>::const_iterator s = sel.begin (); s != sel.end (); ++s) {
      selected.insert (landmark_list->row (*s));
    }

    img::Object::landmarks_type lm = mp_image->landmarks ();

    std::vector <db::DPoint>::iterator w = lm.begin ();
    int i = 0;
    for (std::vector <db::DPoint>::const_iterator r = lm.begin (); r != lm.end (); ++r, ++i) {
      if (selected.find (i) == selected.end ()) {
        *w++ = *r;
      }
    }
    lm.erase (w, lm.end ());

    mp_image->set_landmarks (lm);

    mp_service->update_landmarks ();
    landmarks_updated ();

  } 

  mp_service->set_mode (new_mode);
}

void 
LandmarksDialog::accept ()
{
  mp_original_image->set_landmarks (mp_image->landmarks ());
  QDialog::accept ();
}

void
LandmarksDialog::landmarks_updated ()
{
  landmark_list->clear ();
  for (std::vector<db::DPoint>::const_iterator l = mp_image->landmarks ().begin (); l != mp_image->landmarks ().end (); ++l) {
    landmark_list->addItem (tl::to_qstring (tl::sprintf ("%.0f, %.0f", l->x (), l->y ())));
  }

  landmark_list->selectionModel ()->clear ();
  if (mp_service->selected_index () >= 0) {
    QListWidgetItem *item = landmark_list->item (mp_service->selected_index ());
    if (item) {
      landmark_list->setCurrentItem (item);
      item->setSelected (true);
    }
  }
}

}

#endif
