
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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


#ifndef HDR_layControlWidgetStack
#define HDR_layControlWidgetStack

#include "layCommon.h"

#include <QFrame>

class QLabel;

namespace lay
{

class ControlWidgetStack
  : public QFrame
{
public:
  ControlWidgetStack (QWidget *parent = 0, const char *name = 0, bool size_follows_content = false);

  void focusInEvent (QFocusEvent *);

  QSize sizeHint () const;

  void add_widget (QWidget *w);
  void remove_widget (size_t index);
  void raise_widget (size_t index);
  QWidget *widget (size_t index);
  QWidget *background_widget ();

  QWidget *currentWidget () const
  {
    return mp_current_widget;
  }

  size_t count () const
  {
    return m_widgets.size ();
  }

protected:
  virtual void resizeEvent (QResizeEvent *)
  {
    resize_children ();
  }

  void resize_children ();
  void update_geometry ();

  bool event (QEvent *e);


  std::vector <QWidget *> m_widgets;
  QWidget *mp_current_widget;
  QLabel *mp_bglabel;
  bool m_size_follows_content;
};

}

#endif
