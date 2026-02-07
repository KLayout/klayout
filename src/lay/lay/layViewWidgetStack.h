
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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


#ifndef HDR_layViewWidgetStack
#define HDR_layViewWidgetStack

#include "layCommon.h"

#include <QWidget>

class QLabel;

namespace lay
{

class LayoutViewWidget;

class ViewWidgetStack
  : public QWidget
{
public:
  ViewWidgetStack (QWidget *parent = 0, const char *name = 0);

  void add_widget (lay::LayoutViewWidget *w);
  void remove_widget (size_t index);
  void raise_widget (size_t index);
  LayoutViewWidget *widget (size_t index);
  QWidget *background_widget ();

protected:
  virtual void resizeEvent (QResizeEvent *)
  {
    resize_children ();
  }

  void resize_children ();

  std::vector <LayoutViewWidget *> m_widgets;
  QLabel *mp_bglabel;
};

}

#endif
