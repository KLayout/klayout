
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

#include "layControlWidgetStack.h"

#include <QLabel>
#include <QEvent>

namespace lay
{

ControlWidgetStack::ControlWidgetStack(QWidget *parent, const char *name, bool size_follows_content)
  : QFrame (parent), mp_current_widget (0), m_size_follows_content (size_follows_content)
{
  setObjectName (QString::fromUtf8 (name));

  //  Background ist a simple label without a text currently
  mp_bglabel = new QLabel (this);
  mp_bglabel->setAutoFillBackground (true);
  mp_bglabel->setAlignment (Qt::AlignVCenter | Qt::AlignHCenter);
  mp_bglabel->show ();
}

void ControlWidgetStack::focusInEvent(QFocusEvent *)
{
  for (size_t i = 0; i < m_widgets.size (); ++i) {
    if (m_widgets [i]->isVisible ()) {
      m_widgets [i]->setFocus ();
      break;
    }
  }
}

void ControlWidgetStack::add_widget(QWidget *w)
{
  m_widgets.push_back (w);
  w->setParent (this);
  resize_children ();
  raise_widget (m_widgets.size () - 1);

  int mw = 0;
  for (size_t i = 0; i < m_widgets.size (); ++i) {
    mw = std::max (m_widgets [i]->sizeHint ().width (), mw);
    mw = std::max (m_widgets [i]->minimumWidth (), mw);
  }

  if (mw > minimumWidth ()) {
    setMinimumWidth (mw);
    resize (minimumWidth (), height ());
  }

  update_geometry ();
}

void ControlWidgetStack::update_geometry ()
{
  if (m_size_follows_content) {

    int h = sizeHint ().height ();
    if (h > 0) {
      setMinimumHeight (h);
      setMaximumHeight (h);
    } else {
      setMinimumHeight (0);
      setMaximumHeight (QWIDGETSIZE_MAX);
    }

  }
}

bool ControlWidgetStack::event(QEvent *e)
{
  if (e->type () == QEvent::LayoutRequest) {
    update_geometry ();
  }
  return QWidget::event (e);
}

QSize ControlWidgetStack::sizeHint() const
{
  if (m_size_follows_content) {
    for (size_t i = 0; i < m_widgets.size (); ++i) {
      if (m_widgets [i] && m_widgets [i]->isVisible ()) {
        return m_widgets [i]->sizeHint ();
      }
    }
  }

  int w = 0;
  for (size_t i = 0; i < m_widgets.size (); ++i) {
    w = std::max (m_widgets [i]->sizeHint ().width (), w);
  }
  return QSize (w, 0);
}

void ControlWidgetStack::remove_widget(size_t index)
{
  if (index < m_widgets.size ()) {
    if (mp_current_widget == m_widgets [index]) {
      mp_current_widget = 0;
    }
    m_widgets.erase (m_widgets.begin () + index);
  }
  if (m_widgets.size () == 0) {
    mp_bglabel->show ();
  }

  update_geometry ();
}

void ControlWidgetStack::raise_widget(size_t index)
{
  mp_current_widget = 0;
  bool any_visible = false;
  for (size_t i = 0; i < m_widgets.size (); ++i) {
    if (m_widgets [i]) {
      if (i == index) {
        m_widgets [i]->show ();
        mp_current_widget = m_widgets [i];
        any_visible = true;
      } else {
        m_widgets [i]->hide ();
      }
    }
  }

  if (! any_visible) {
    mp_bglabel->show ();
  } else {
    mp_bglabel->hide ();
  }

  update_geometry ();
}

QWidget *ControlWidgetStack::widget(size_t index)
{
  if (index < m_widgets.size ()) {
    return m_widgets [index];
  } else {
    return 0;
  }
}

QWidget *ControlWidgetStack::background_widget()
{
  return mp_bglabel;
}

void ControlWidgetStack::resize_children()
{
  //  set the geometry of all children
  for (std::vector <QWidget *>::iterator child = m_widgets.begin (); child != m_widgets.end (); ++child) {
    if (*child) {
      (*child)->setGeometry (0, 0, width (), height ());
    }
  }
  mp_bglabel->setGeometry (0, 0, width (), height ());
}

}
