
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

#include "laybasicConfig.h"
#include "layMarker.h"
#include "layRubberBox.h"
#include "layLayoutView.h"
#include "layZoomBox.h"
#include "imgService.h"
#include "imgNavigator.h"

#include <QVBoxLayout>
#include <QMenuBar>
#include <QLabel>

namespace img
{

// ---------------------------------------------------------------------------------------------
//  Navigator implementation

Navigator::Navigator (QWidget *parent)
  : QFrame (parent), 
    mp_view (0),
    mp_zoom_service (0)
{
  setObjectName (QString::fromUtf8 ("img_navigator"));
}

img::Object *
Navigator::setup (lay::Dispatcher *root, img::Object *img)
{
  mp_view = new lay::LayoutViewWidget (0, false, root, this, lay::LayoutView::LV_Naked + lay::LayoutView::LV_NoZoom + lay::LayoutView::LV_NoServices + lay::LayoutView::LV_NoGrid);
  mp_view->setSizePolicy (QSizePolicy::Expanding, QSizePolicy::Expanding);
  mp_view->setMinimumWidth (100);
  mp_view->setMinimumHeight (100);

  QVBoxLayout *layout = new QVBoxLayout (this);
  layout->addWidget (mp_view);
  layout->setStretchFactor (mp_view, 1);
  layout->setContentsMargins (0, 0, 0, 0);
  layout->setSpacing (0);
  setLayout (layout);

  mp_zoom_service = new lay::ZoomService (view ());

  img::Service *img_target = view ()->get_plugin<img::Service> ();
  if (img_target) {
    img_target->clear_images ();
    img::Object *img_object = img_target->insert_image (*img);
    img_object->set_matrix (db::Matrix3d (1.0));
    view ()->zoom_fit ();
    return img_object;
  } else {
    return 0;
  }
}

Navigator::~Navigator ()
{
  if (mp_zoom_service) {
    delete mp_zoom_service;
    mp_zoom_service = 0;
  }

  if (mp_view) {
    delete mp_view;
    mp_view = 0;
  }
}

lay::LayoutView *Navigator::view ()
{
  return mp_view->view ();
}

void 
Navigator::activate_service (lay::ViewService *service)
{
  view ()->canvas ()->activate (service);
}

void
Navigator::background_color (QColor c)
{
  //  replace by "real" background color if required
  if (! c.isValid ()) {
    c = palette ().color (QPalette::Normal, QPalette::Base);
  }

  QColor contrast;
  if (c.green () > 128) {
    contrast = QColor (0, 0, 0);
  } else {
    contrast = QColor (255, 255, 255);
  }
}

}

#endif
