
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

#include "layViewWidgetStack.h"
#include "layLayoutView.h"

#include <QLabel>

namespace lay
{

ViewWidgetStack::ViewWidgetStack (QWidget *parent, const char *name)
  : QWidget (parent)
{
  setObjectName (QString::fromUtf8 (name));

  mp_bglabel = new QLabel (this);
  mp_bglabel->setAutoFillBackground (true);
  std::string logo = "logo.png";
#if QT_VERSION >= 0x50000
  if (devicePixelRatio () >= 2.0) {
    logo = "logo@2x.png";
  }
#endif
  mp_bglabel->setText (QObject::tr ("<html><body><p><img src=\":/%1\" width=\"256\" height=\"256\"/></p><p>Use File/Open to open a layout</p></body></html>").arg (tl::to_qstring (logo)));
  mp_bglabel->setAlignment (Qt::AlignVCenter | Qt::AlignHCenter);
  mp_bglabel->show ();
}

void ViewWidgetStack::add_widget (LayoutViewWidget *w)
{
  tl_assert (w);

  m_widgets.push_back (w);
  w->setParent (this);
  resize_children ();
  raise_widget (m_widgets.size () - 1);

  updateGeometry ();
}

void ViewWidgetStack::remove_widget (size_t index)
{
  if (index < m_widgets.size ()) {
    m_widgets.erase (m_widgets.begin () + index);
  }
  if (m_widgets.size () == 0) {
    mp_bglabel->show ();
  }
}

void ViewWidgetStack::raise_widget (size_t index)
{
  if (index < m_widgets.size ()) {
    mp_bglabel->hide ();
    m_widgets [index]->show ();
  } else {
    mp_bglabel->show ();
  }

  size_t i = 0;
  for (std::vector <LayoutViewWidget *>::iterator child = m_widgets.begin (); child != m_widgets.end (); ++child, ++i) {
    if (i != index) {
      (*child)->hide ();
    }
  }
}

LayoutViewWidget *ViewWidgetStack::widget (size_t index)
{
  if (index < m_widgets.size ()) {
    return m_widgets [index];
  } else {
    return 0;
  }
}

QWidget *ViewWidgetStack::background_widget ()
{
  return mp_bglabel;
}

void ViewWidgetStack::resize_children ()
{
  //  set the geometry of all children
  for (std::vector <LayoutViewWidget *>::iterator child = m_widgets.begin (); child != m_widgets.end (); ++child) {
    (*child)->setGeometry (0, 0, width (), height ());
  }
  mp_bglabel->setGeometry (0, 0, width (), height ());
}

}
