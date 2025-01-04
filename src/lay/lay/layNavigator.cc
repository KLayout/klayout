
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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


#include "layNavigator.h"
#include "layMainWindow.h"
#include "layConfig.h"
#include "layMarker.h"
#include "layAbstractMenu.h"
#include "layRubberBox.h"
#include "imgService.h"

#include <QVBoxLayout>
#include <QFrame>
#include <QLabel>

namespace lay
{

// ---------------------------------------------------------------------------------------------
//  Navigator service definition and implementation

class NavigatorService
  : public ViewService,
    public tl::Object
{
public:
  enum drag_mode_type {
    DM_none, DM_move,
    DM_l, DM_r, DM_t, DM_b
  };

  NavigatorService (LayoutView *view)
    : ViewService (view->canvas ()), 
      mp_view (view), mp_source_view (0),
      mp_viewport_marker (0),
      m_drag_mode (DM_none),
      m_dragging (false),
      mp_box (0), 
      m_color (0)
  {
    //  .. nothing yet ..
  }

  ~NavigatorService ()
  {
    if (mp_viewport_marker) {
      delete mp_viewport_marker;
      mp_viewport_marker = 0;
    }
    drag_cancel ();
  }

  void background_color_changed ()
  {
    tl::Color c = mp_view->background_color ();

    //  replace by "real" background color if required
    if (! c.is_valid ()) {
      if (mp_view->widget ()) {
        c = tl::Color (mp_view->widget ()->palette ().color (QPalette::Normal, QPalette::Base).rgb ());
      } else {
        c = tl::Color (0xffffff);  //  white
      }
    }

    tl::Color contrast;
    if (c.to_mono ()) {
      contrast = tl::Color (0, 0, 0);
    } else {
      contrast = tl::Color (255, 255, 255);
    }

    set_colors (c, contrast);
  }

  bool mouse_release_event (const db::DPoint & /*p*/, unsigned int /*buttons*/, bool /*prio*/) 
  { 
    if (mp_box) {

      //  finish zoom box selection
      delete mp_box;
      mp_box = 0;

      ui ()->ungrab_mouse (this);

      if (mp_source_view) {
        mp_source_view->zoom_box (db::DBox (m_p1, m_p2));
      }

      return true;

    } else if (m_dragging) {

      m_dragging = false;
      ui ()->ungrab_mouse (this);
      return true;

    } else {
      return false;
    }
  }

  bool mouse_click_event (const db::DPoint &p, unsigned int buttons, bool prio) 
  { 
    if (! prio && (buttons & lay::RightButton) != 0) {
      db::DBox vp = ui ()->mouse_event_viewport ();
      if (mp_source_view && vp.contains (p)) {
        db::DVector d = (vp.p2 () - vp.p1 ()) * 0.5;
        mp_source_view->zoom_box (db::DBox (p - d, p + d));
      }
    }
    return false;
  }

  bool mouse_press_event (const db::DPoint &p, unsigned int buttons, bool prio) 
  { 
    if (! prio && (buttons & lay::RightButton) != 0) {

      mp_source_view->stop_redraw (); // TODO: how to restart if zoom is aborted?
      if ((buttons & lay::ShiftButton) != 0) {
        begin_pan (p);
      } else {
        begin (p);
      }

      return true;

    } else if (! prio && (buttons & lay::MidButton) != 0) {

      mp_source_view->stop_redraw (); // TODO: how to restart if zoom is aborted?
      begin_pan (p);
      return true;

    } else if (prio && (buttons & lay::LeftButton) != 0 && m_drag_mode != DM_none && mp_source_view) {

      m_dragging = true;
      m_p0 = p;
      m_b0 = m_box;
      ui ()->grab_mouse (this, true);
      return true;

    } else {
      return false;
    }
  }

  //  Taken from ZoomBox service
  bool wheel_event (int delta, bool /*horizontal*/, const db::DPoint &p, unsigned int buttons, bool prio)
  {
    //  Only act without the mouse being grabbed.
    if (! prio) {

      if (mp_source_view) {

        enum { horizontal, vertical, zoom } direction = zoom;
        if (mp_source_view->mouse_wheel_mode () == 0) {

          if ((buttons & lay::ShiftButton) != 0) {
            direction = vertical;
          } else if ((buttons & lay::ControlButton) != 0) {
            direction = horizontal;
          } else {
            direction = zoom;
          }

        } else {

          if ((buttons & lay::ShiftButton) != 0) {
            direction = horizontal;
          } else if ((buttons & lay::ControlButton) != 0) {
            direction = zoom;
          } else {
            direction = vertical;
          }

        }

        if (direction == vertical) {

          if (delta > 0) {
            mp_source_view->pan_up ();
          } else {
            mp_source_view->pan_down ();
          }

        } else if (direction == horizontal) {

          if (delta > 0) {
            mp_source_view->pan_left ();
          } else {
            mp_source_view->pan_right ();
          }

        } else if (m_drag_mode == DM_move) {

          double zoom_step = 0.25; // TODO: make variable?

          double f;
          if (delta > 0) {
            f = 1.0 / (1.0 + zoom_step * (delta / 120.0));
          } else {
            f = 1.0 + zoom_step * (-delta / 120.0);
          }

          mp_source_view->zoom_box (db::DBox (p.x () - (p.x () - m_box.left ()) * f, 
                                              p.y () - (p.y () - m_box.bottom ()) * f,
                                              p.x () - (p.x () - m_box.right ()) * f, 
                                              p.y () - (p.y () - m_box.top ()) * f));

          update_marker ();

        }

      }

    }

    return false;
  }

  bool mouse_move_event (const db::DPoint &p, unsigned int /*buttons*/, bool prio) 
  { 
    bool ret_value = false;

    if (mp_box) {

      //  drag zoom box
      if (prio) {

        m_p2 = p;
        mp_box->set_points (m_p1, m_p2);

        mp_source_view->message ("w: " + tl::micron_to_string (fabs (m_p2.x () - m_p1.x ())) + "  h: " + tl::micron_to_string (fabs (m_p2.y () - m_p1.y ())));

        return true;

      } else {
        return false;
      }

    } else if (! m_dragging) {

      m_drag_mode = DM_none;

      if (! m_box.empty ()) {

        double mw = 5.0/*pixel*/ / mp_view->viewport ().trans ().ctrans (1.0/*micron*/);
        db::DVector d = db::DVector (std::max (mw, m_box.width () * 0.5 - mw), std::max (mw, m_box.height () * 0.5 - mw));
        db::DBox move_box = db::DBox (m_box.center () - d, m_box.center () + d);
        db::DBox l_box    = db::DBox (m_box.left ()   - mw, m_box.bottom ()     , m_box.left ()  + mw, m_box.top ()        );
        db::DBox r_box    = db::DBox (m_box.right ()  - mw, m_box.bottom ()     , m_box.right () + mw, m_box.top ()        );
        db::DBox t_box    = db::DBox (m_box.left ()       , m_box.top ()    - mw, m_box.right ()     , m_box.top ()    + mw);
        db::DBox b_box    = db::DBox (m_box.left ()       , m_box.bottom () - mw, m_box.right ()     , m_box.bottom () + mw);

        if (move_box.contains (p)) {
          m_drag_mode = DM_move;
        } else if (l_box.contains (p)) {
          m_drag_mode = DM_l;
        } else if (r_box.contains (p)) {
          m_drag_mode = DM_r;
        } else if (t_box.contains (p)) {
          m_drag_mode = DM_t;
        } else if (b_box.contains (p)) {
          m_drag_mode = DM_b;
        }

      }

    } else if (prio) {

      db::DBox new_box;
      db::DVector dp = p - m_p0;

      if (m_drag_mode == DM_move) {
        new_box = m_b0.moved (dp);
      } else if (m_drag_mode == DM_l) {
        double new_h = m_b0.height () / m_b0.width () * (m_b0.width () - dp.x ());
        double dy = (new_h - m_b0.height ()) * 0.5;
        new_box = db::DBox (m_b0.left () + dp.x (), m_b0.bottom () - dy, m_b0.right (), m_b0.top () + dy);
      } else if (m_drag_mode == DM_r) {
        double new_h = m_b0.height () / m_b0.width () * (m_b0.width () + dp.x ());
        double dy = (new_h - m_b0.height ()) * 0.5;
        new_box = db::DBox (m_b0.left (), m_b0.bottom () - dy, m_b0.right () + dp.x (), m_b0.top () + dy);
      } else if (m_drag_mode == DM_t) {
        double new_w = m_b0.width () / m_b0.height () * (m_b0.height () + dp.y ());
        double dx = (new_w - m_b0.width ()) * 0.5;
        new_box = db::DBox (m_b0.left () - dx, m_b0.bottom (), m_b0.right () + dx, m_b0.top () + dp.y ());
      } else if (m_drag_mode == DM_b) {
        double new_w = m_b0.width () / m_b0.height () * (m_b0.height () - dp.y ());
        double dx = (new_w - m_b0.width ()) * 0.5;
        new_box = db::DBox (m_b0.left () - dx, m_b0.bottom () + dp.y (), m_b0.right () + dx, m_b0.top ());
      }

      if (! new_box.empty () && mp_source_view) {
        mp_source_view->zoom_box (new_box);
      }

      update_marker ();

      ret_value = true;

    }
    
    if (m_drag_mode == DM_move) {
      set_cursor (Cursor::size_all);
    } else if (m_drag_mode == DM_l) {
      set_cursor (Cursor::size_hor);
    } else if (m_drag_mode == DM_r) {
      set_cursor (Cursor::size_hor);
    } else if (m_drag_mode == DM_t) {
      set_cursor (Cursor::size_ver);
    } else if (m_drag_mode == DM_b) {
      set_cursor (Cursor::size_ver);
    }

    return ret_value;

  }

  void update_marker ()
  {
    if (mp_viewport_marker) {
      delete mp_viewport_marker;
      mp_viewport_marker = 0;
      m_box = db::DBox ();
    } 

    if (mp_source_view) {

      m_box = mp_source_view->viewport ().box ();
      // correct the box by a few pixels so it is more precisely reflecting the actual dimensions 
      double d = 1.0 / mp_view->viewport ().trans ().ctrans (1.0);
      m_box.set_right (m_box.right () - 2.0 * d);
      m_box.set_bottom (m_box.bottom () + d);

      mp_viewport_marker = new DMarker (mp_view);
      mp_viewport_marker->set_halo (true);
      mp_viewport_marker->set_color (m_color);
      mp_viewport_marker->set_line_width (2);
      mp_viewport_marker->set_vertex_size (2);
      mp_viewport_marker->set_dither_pattern (1);
      mp_viewport_marker->set_frame_pattern (0);
      mp_viewport_marker->set (m_box);

    }
  }

  void attach_view (LayoutView *source_view)
  {
    if (mp_source_view != source_view) {

      tl::Object::detach_from_all_events ();

      mp_source_view = source_view;
      mp_source_view->viewport_changed_event.add (this, &NavigatorService::update_marker);

      mp_view->background_color_changed_event.add (this, &NavigatorService::background_color_changed);
      background_color_changed ();

      update_marker ();

    }
  }

  void drag_cancel ()
  {
    //  cancel zoom box dragging
    if (mp_box) {
      delete mp_box;
      mp_box = 0;
    }
    ui ()->ungrab_mouse (this);
  }

  void set_colors (tl::Color /*background*/, tl::Color color)
  {
    //  set zoom box color
    m_color = color.rgb ();
    if (mp_box) {
      mp_box->set_color (m_color);
    }
    if (mp_viewport_marker) {
      mp_viewport_marker->set_color (m_color);
    }
  }

private:
  LayoutView *mp_view;
  LayoutView *mp_source_view;
  DMarker *mp_viewport_marker;
  db::DBox m_box;
  db::DPoint m_p0;
  db::DBox m_b0;
  drag_mode_type m_drag_mode;
  bool m_dragging;
  db::DPoint m_p1, m_p2;
  db::DBox m_vp;
  lay::RubberBox *mp_box;
  unsigned int m_color;

  void begin_pan (const db::DPoint &pos)
  { 
    if (mp_box) {
      delete mp_box;
    }
    mp_box = 0;

    m_p1 = pos;
    m_vp = ui ()->mouse_event_viewport ();

    ui ()->grab_mouse (this, true);
  }

  void begin (const db::DPoint &pos)
  { 
    if (mp_box) {
      delete mp_box;
    }

    m_p1 = pos;
    m_p2 = pos;
    mp_box = new lay::RubberBox (ui (), m_color, pos, pos);

    ui ()->grab_mouse (this, true);
  }
};

// ---------------------------------------------------------------------------------------------
//  Navigator implementation

const std::string freeze_action_path ("@@navigator_menu.navigator_main_menu.navigator_freeze");

Navigator::Navigator (MainWindow *main_window)
  : QFrame (main_window), 
    m_show_all_hier_levels (false),
    m_show_images (true),
    m_update_layers_needed (true),
    m_update_needed (true),
    mp_main_window (main_window), 
    mp_source_view (0), 
    mp_service (0),
    m_do_view_changed (this, &Navigator::attach_view),
    m_do_layers_changed (this, &Navigator::update_layers),
    m_do_content_changed (this, &Navigator::update),
    m_do_update_menu_dm (this, &Navigator::do_update_menu)
{
  setObjectName (QString::fromUtf8 ("navigator"));

  mp_menu_bar = new QFrame (this);
  mp_menu_bar->setFrameShape (QFrame::NoFrame);
  mp_menu_bar->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Preferred);

  mp_view = 0;
  mp_service = 0;

  mp_placeholder_label = new QLabel (this);
  mp_placeholder_label->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);
  mp_placeholder_label->setMinimumWidth (100);
  mp_placeholder_label->setMinimumHeight (100);
  mp_placeholder_label->show ();

  QVBoxLayout *layout = new QVBoxLayout (this);
  layout->addWidget (mp_menu_bar);
  layout->addWidget (mp_placeholder_label);
  layout->setStretchFactor (mp_placeholder_label, 1);
  layout->setContentsMargins (0, 0, 0, 0);
  layout->setSpacing (0);
  setLayout (layout);

  mp_main_window->current_view_changed_event.add (this, &Navigator::view_changed);
  mp_main_window->view_closed_event.add (this, &Navigator::view_closed);

  do_update_menu ();
  connect (mp_main_window->menu (), SIGNAL (changed ()), this, SLOT (menu_changed ()));
}

Navigator::~Navigator ()
{
  if (mp_service) {
    delete mp_service;
    mp_service = 0;
  }

  if (mp_view) {
    delete mp_view;
    mp_view = 0;
  }
}

void
Navigator::menu_changed ()
{
  //  delay actual rebuilding of the menu to collect multiple change events.
  m_do_update_menu_dm ();
}

void 
Navigator::do_update_menu ()
{
  mp_main_window->menu ()->build_detached ("navigator_menu", mp_menu_bar);
}

void
Navigator::show_images (bool f)
{
  if (f != m_show_images) {
    m_show_images = f;
    if (isVisible ()) {
      update ();
    }
  }
}

void
Navigator::all_hier_levels (bool f)
{
  if (f != m_show_all_hier_levels) {
    m_show_all_hier_levels = f;
    if (isVisible ()) {
      update ();
    }
  }
}

void 
Navigator::freeze_clicked ()
{
  Action *freeze_action = mp_main_window->menu ()->action (freeze_action_path);

  m_frozen_list.erase (mp_source_view);

  if (freeze_action->is_checked () && mp_source_view) {
    NavigatorFrozenViewInfo &info = m_frozen_list.insert (std::make_pair (mp_source_view, NavigatorFrozenViewInfo ())).first->second;
    info.layer_properties = mp_source_view->get_properties ();
    info.hierarchy_levels = mp_source_view->get_hier_levels ();
  } else {
    update ();
  }
}

void 
Navigator::showEvent (QShowEvent *)
{
  if (mp_main_window->current_view () != mp_source_view) {
    attach_view ();
  } else if (m_update_needed) {
    update ();
  } else if (m_update_layers_needed) {
    update_layers ();
  }

  m_update_layers_needed = false;
  m_update_needed = false;
}

void 
Navigator::closeEvent (QCloseEvent *)
{
  mp_main_window->dispatcher ()->config_set (cfg_show_navigator, "false");
  mp_main_window->dispatcher ()->config_end ();
}

void 
Navigator::view_changed ()
{
  if (isVisible ()) {
    m_do_view_changed ();
  } else {
    //  force attach view, when the window is opened again
    attach_view (0);
  }
}

void 
Navigator::layers_changed (int)
{
  if (isVisible ()) {
    m_do_layers_changed ();
  } else {
    m_update_layers_needed = true;
  }
}

void 
Navigator::content_changed ()
{
  if (isVisible ()) {
    m_do_content_changed ();
  } else {
    m_update_needed = true;
  }
}

void
Navigator::attach_view ()
{
  attach_view (mp_main_window->current_view ());
}

void
Navigator::view_closed (int index)
{
  LayoutView *view = mp_main_window->view ((unsigned int) index);

  if (view == mp_source_view) {
    attach_view (0);
  }
}

void
Navigator::resizeEvent (QResizeEvent *)
{
  if (mp_view) {
    mp_view->setGeometry (mp_placeholder_label->geometry ());
  }
}

void
Navigator::attach_view (LayoutView *view)
{
  if (view != mp_source_view) {

    tl::Object::detach_from_all_events ();

    mp_main_window->current_view_changed_event.add (this, &Navigator::view_changed);
    mp_main_window->view_closed_event.add (this, &Navigator::view_closed);

    mp_source_view = view;

    delete mp_service;
    mp_service = 0;

    LayoutViewWidget *old_view = mp_view;
    mp_view = 0;

    if (mp_source_view) {

      mp_view = new LayoutViewWidget (0, false, mp_source_view, this, LayoutView::LV_Naked + LayoutView::LV_NoZoom + LayoutView::LV_NoServices + LayoutView::LV_NoGrid);
      mp_view->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);
      mp_view->setMinimumWidth (100);
      mp_view->setMinimumHeight (100);
      mp_view->setGeometry (mp_placeholder_label->geometry ());
      mp_view->show ();

      mp_service = new NavigatorService (mp_view->view ());
      mp_view->view ()->canvas ()->activate (mp_service);

      mp_source_view->cellviews_changed_event.add (this, &Navigator::content_changed);
      mp_source_view->cellview_changed_event.add (this, &Navigator::content_changed_with_int);
      mp_source_view->geom_changed_event.add (this, &Navigator::content_changed);
      mp_source_view->layer_list_changed_event.add (this, &Navigator::layers_changed),
      mp_source_view->hier_levels_changed_event.add (this, &Navigator::hier_levels_changed);

      img::Service *image_plugin = mp_source_view->get_plugin<img::Service> ();
      if (image_plugin) {
        image_plugin->images_changed_event.add (this, &Navigator::content_changed);
      }

      //  update the list of frozen flags per view
      std::set <lay::LayoutView *> all_views;

      for (std::map <lay::LayoutView *, NavigatorFrozenViewInfo>::const_iterator f = m_frozen_list.begin (); f != m_frozen_list.end (); ++f) {
        all_views.insert (f->first);
      }

      for (unsigned int i = 0; i < mp_main_window->views (); ++i) {
        lay::LayoutView *view = mp_main_window->view (i);
        if (m_frozen_list.find (view) != m_frozen_list.end ()) {
          all_views.erase (view);
        }
      }

      for (std::set <lay::LayoutView *>::const_iterator v = all_views.begin (); v != all_views.end (); ++v) {
        all_views.erase (*v);
      }

      Action *freeze_action = mp_main_window->menu ()->action (freeze_action_path);
      freeze_action->set_checked (m_frozen_list.find (mp_source_view) != m_frozen_list.end ());

      //  Hint: this must happen before update ()
      mp_service->attach_view (mp_source_view);

      update ();

    }

    delete old_view;

  }
}

void
Navigator::hier_levels_changed ()
{
  if (m_show_all_hier_levels && mp_source_view && m_frozen_list.find (mp_source_view) == m_frozen_list.end ()) {
    mp_view->view ()->set_hier_levels (mp_source_view->get_hier_levels ());
  }
}

void
Navigator::update_layers ()
{
  if (! mp_source_view || m_frozen_list.find (mp_source_view) == m_frozen_list.end ()) {
    update ();
  }
}

void
Navigator::update ()
{
  if (! mp_view || ! mp_view->view () || ! mp_source_view) {
    return;
  }

  if (m_frozen_list.find (mp_source_view) == m_frozen_list.end ()) {
    mp_view->view ()->select_cellviews (mp_source_view->cellview_list ());
    mp_view->view ()->set_properties (mp_source_view->get_properties ());
  } else {
    mp_view->view ()->select_cellviews (mp_source_view->cellview_list ());
    mp_view->view ()->set_properties (m_frozen_list [mp_source_view].layer_properties);
  }

  img::Service *img_target = mp_view->view ()->get_plugin<img::Service> ();
  if (img_target) {

    img_target->clear_images ();

    if (m_show_images) {
      img::Service *img_source = (mp_source_view->get_plugin<img::Service> ());
      if (img_source) {
        for (img::ImageIterator i = img_source->begin_images (); ! i.at_end (); ++i) {
          img_target->insert_image (*i);
        }
      }
    }

  }

  if (m_show_all_hier_levels && mp_source_view) {
    if (m_frozen_list.find (mp_source_view) == m_frozen_list.end ()) {
      mp_view->view ()->set_hier_levels (mp_source_view->get_hier_levels ());
    } else {
      mp_view->view ()->set_hier_levels (m_frozen_list [mp_source_view].hierarchy_levels);
    }
  } else {
    mp_view->view ()->set_hier_levels (std::make_pair (0, 0));
  }

  mp_view->view ()->zoom_fit ();
  mp_view->view ()->update_content ();
  mp_service->update_marker ();
}

// ------------------------------------------------------------
//  Declaration of the "plugin" for the menu entries

class NavigatorPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    std::string at;

    at = ".end";
    menu_entries.push_back (lay::submenu ("@@navigator_menu", at, std::string ()));

    at = "@@navigator_menu.end";
    menu_entries.push_back (lay::submenu ("navigator_main_menu", at, tl::to_string (QObject::tr ("Options"))));

    at = "@@navigator_menu.navigator_main_menu.end";
    menu_entries.push_back (lay::config_menu_item ("navigator_show_images", at, tl::to_string (QObject::tr ("Show Images")), cfg_navigator_show_images, "?"));
    menu_entries.push_back (lay::config_menu_item ("navigator_all_hier_levels", at, tl::to_string (QObject::tr ("Show All Hierarchy Levels")), cfg_navigator_all_hier_levels, "?"));
    menu_entries.push_back (lay::separator ("navigator_options_group", at));
    menu_entries.push_back (lay::menu_item ("cm_navigator_freeze", "navigator_freeze", at, tl::to_string (QObject::tr ("Freeze"))));
    menu_entries.back ().checkable = true;
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new NavigatorPluginDeclaration (), -1, "NavigatorPlugin");

}

