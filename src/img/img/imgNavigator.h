
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


#ifndef HDR_imgNavigator
#define HDR_imgNavigator

#include <QFrame>

namespace lay
{
  class Dispatcher;
  class LayoutView;
  class ZoomService;
  class ViewService;
}

namespace img
{

class Object;

/**
 *  @brief The navigator window 
 */
class Navigator 
  : public QFrame
{
Q_OBJECT

public:
  Navigator (QWidget *parent);
  ~Navigator ();

  void background_color (QColor c);
  img::Object *setup (lay::Dispatcher *root, img::Object *img);

  lay::LayoutView *view () 
  {
    return mp_view;
  }

  void activate_service (lay::ViewService *service);

private:
  lay::LayoutView *mp_view;  
  lay::ZoomService *mp_zoom_service;
};

}

#endif

