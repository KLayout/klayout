
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

#if defined(HAVE_QT)

#include "layEditorOptionsPageWidget.h"
#include "layLayoutViewBase.h"
#include "tlExceptions.h"

#include <QApplication>
#include <QKeyEvent>

namespace lay
{

// ------------------------------------------------------------------
//  EditorOptionsPageWidget implementation

EditorOptionsPageWidget::EditorOptionsPageWidget (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
  : QWidget (0), EditorOptionsPage (view, dispatcher), m_is_transparent (false)
{
  init (view, dispatcher);
}

EditorOptionsPageWidget::EditorOptionsPageWidget ()
  : QWidget (0), EditorOptionsPage (), m_is_transparent (false)
{
  //  .. nothing yet ..
}

EditorOptionsPageWidget::~EditorOptionsPageWidget ()
{
  set_owner (0);
}

void
EditorOptionsPageWidget::edited ()
{
  apply (dispatcher ());
}

static bool is_parent_widget (QWidget *w, QWidget *parent)
{
  while (w && w != parent) {
    w = dynamic_cast<QWidget *> (w->parent ());
  }
  return w == parent;
}

bool
EditorOptionsPageWidget::focusNextPrevChild (bool next)
{
  bool res = QWidget::focusNextPrevChild (next);

  //  Stop making the focus leave the page - this way we can jump back to the
  //  view on "enter"
  if (res && ! is_modal_page () && ! is_parent_widget (QApplication::focusWidget (), this) && focusWidget ()) {
    focusWidget ()->setFocus ();
  }

  return res;
}

void
EditorOptionsPageWidget::keyPressEvent (QKeyEvent *event)
{
BEGIN_PROTECTED

  if (! is_modal_page () &&
      event->modifiers () == Qt::NoModifier &&
      (event->key () == Qt::Key_Return || event->key () == Qt::Key_Enter || event->key () == Qt::Key_Escape)) {

    if (event->key () == Qt::Key_Escape) {
      //  The Escape key creates a call to cancel()
      cancel ();
    } else {
      //  The Return key on a non-modal page commits the values and gives back the focus
      //  to the view
      commit (dispatcher ());
    }

    view ()->set_focus ();

    event->accept ();

  } else {
    QWidget::keyPressEvent (event);
  }

END_PROTECTED
}

bool
EditorOptionsPageWidget::event (QEvent *event)
{
  if (event->type () == QEvent::ShortcutOverride) {

    QKeyEvent *ke = dynamic_cast<QKeyEvent *> (event);
    if (ke->key () == Qt::Key_Escape ||
        ke->key () == Qt::Key_Tab ||
        ke->key () == Qt::Key_Enter ||
        ke->key () == Qt::Key_Return ||
        ke->key () == Qt::Key_Backtab) {
      //  accept the shortcut override event for some keys, so we can handle
      //  it in keyPressEvent
      ke->accept ();
    }

  }

  return QWidget::event (event);
}

void
EditorOptionsPageWidget::resizeEvent (QResizeEvent *e)
{
  //  makes the widget transparent
  //  see https://stackoverflow.com/questions/27855137/how-to-disable-the-delivery-of-mouse-events-to-the-widget-but-not-its-children-i
  if (e) {
    QWidget::resizeEvent (e);
  }

  if (m_is_transparent) {
    QRegion reg (frameGeometry ());
    reg -= QRegion (geometry ());
    reg += childrenRegion ();
    setMask (reg);
  } else {
    clearMask ();
  }
}

void
EditorOptionsPageWidget::set_transparent (bool f)
{
  if (f != m_is_transparent) {
    m_is_transparent = f;
    resizeEvent (0);
  }
}

void
EditorOptionsPageWidget::set_focus ()
{
  if (isVisible ()) {
    setFocus (Qt::TabFocusReason);
    QWidget::focusNextPrevChild (true);
  }
}

void
EditorOptionsPageWidget::set_visible (bool visible)
{
  setVisible (visible);
}

bool
EditorOptionsPageWidget::is_visible () const
{
  return isVisible ();
}

}

#endif
