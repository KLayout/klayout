
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

#include "tlInternational.h"
#include "layEditorOptionsPage.h"
#include "layEditorOptionsPages.h"
#include "layLayoutViewBase.h"
#include "tlExceptions.h"

#include <QApplication>
#include <QKeyEvent>

namespace lay
{

// ------------------------------------------------------------------
//  EditorOptionsPage implementation

EditorOptionsPage::EditorOptionsPage (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
  : QWidget (0), mp_owner (0), m_active (true), m_focus_page (false), m_modal_page (false), mp_plugin_declaration (0), mp_dispatcher (dispatcher), mp_view (view)
{
  attach_events ();
}

EditorOptionsPage::EditorOptionsPage ()
  : QWidget (0), mp_owner (0), m_active (true), m_focus_page (false), m_modal_page (false), mp_plugin_declaration (0), mp_dispatcher (0), mp_view (0)
{
  //  .. nothing yet ..
}

EditorOptionsPage::~EditorOptionsPage ()
{
  set_owner (0);
}

void
EditorOptionsPage::init (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
{
  mp_view = view;
  mp_dispatcher = dispatcher;
  attach_events ();
}

void
EditorOptionsPage::edited ()
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
EditorOptionsPage::focusNextPrevChild (bool next)
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
EditorOptionsPage::keyPressEvent (QKeyEvent *event)
{
BEGIN_PROTECTED
  if (! is_modal_page () && event->modifiers () == Qt::NoModifier && event->key () == Qt::Key_Return) {
    //  The Return key on a non-modal page commits the values and gives back the focus
    //  to the view
    apply (dispatcher ());
    view ()->set_focus ();
    event->accept ();
  } else {
    QWidget::keyPressEvent (event);
  }
END_PROTECTED
}

void
EditorOptionsPage::set_focus ()
{
  setFocus (Qt::TabFocusReason);
  QWidget::focusNextPrevChild (true);
}

int
EditorOptionsPage::show ()
{
  if (mp_owner && m_active) {
    if (! is_modal_page ()) {
      mp_owner->make_page_current (this);
      return -1;
    } else {
      return mp_owner->exec_modal (this) ? 1 : 0;
    }
  } else {
    return -1;
  }
}

void
EditorOptionsPage::attach_events ()
{
  detach_from_all_events ();
  view ()->active_cellview_changed_event.add (this, &EditorOptionsPage::on_active_cellview_changed);
  int cv_index = view ()->active_cellview_index ();
  if (cv_index >= 0) {
    view ()->cellview (cv_index)->technology_changed_event.add (this, &EditorOptionsPage::on_technology_changed);
  }
}

void
EditorOptionsPage::on_active_cellview_changed ()
{
  active_cellview_changed ();
  attach_events ();
}

void
EditorOptionsPage::on_technology_changed ()
{
  technology_changed (view ()->active_cellview_ref ()->tech_name ());
}

void
EditorOptionsPage::set_owner (EditorOptionsPages *owner)
{
  if (mp_owner) {
    mp_owner->unregister_page (this);
  }
  mp_owner = owner;
}

void
EditorOptionsPage::activate (bool active)
{
  if (m_active != active) {
    m_active = active;
    if (mp_owner) {
      mp_owner->activate_page (this);
    }
  }
}

}

#endif
