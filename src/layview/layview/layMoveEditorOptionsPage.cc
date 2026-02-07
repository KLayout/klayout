
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

#if defined(HAVE_QT)

#include "layMoveEditorOptionsPage.h"
#include "layWidgets.h"
#include "layDispatcher.h"

#include <QHBoxLayout>
#include <QLineEdit>

namespace lay
{

MoveEditorOptionsPage::MoveEditorOptionsPage (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
  : lay::EditorOptionsPageWidget (view, dispatcher)
{
  mp_layout = new QHBoxLayout (this);

  mp_x_le = new lay::DecoratedLineEdit (this);
  mp_x_le->set_label ("dx:");
  mp_layout->addWidget (mp_x_le);

  mp_y_le = new lay::DecoratedLineEdit (this);
  mp_y_le->set_label ("dy:");
  mp_layout->addWidget (mp_y_le);

  mp_layout->addStretch (1);

  hide ();

  set_toolbox_widget (true);
  set_transparent (true);
}

std::string
MoveEditorOptionsPage::title () const
{
  return "Move Options";
}

const char *
MoveEditorOptionsPage::name () const
{
  return move_editor_options_name.c_str ();
}

int
MoveEditorOptionsPage::order () const
{
  return 0;
}

void
MoveEditorOptionsPage::deactivated ()
{
  hide ();
}

void
MoveEditorOptionsPage::commit (lay::Dispatcher *dispatcher)
{
  try {

    double dx = 0.0, dy = 0.0;

    tl::from_string (tl::to_string (mp_x_le->text ()), dx);
    tl::from_string (tl::to_string (mp_y_le->text ()), dy);

    dispatcher->call_function (move_function_name, db::DVector (dx, dy).to_string ());

  } catch (...) {
  }
}

void
MoveEditorOptionsPage::configure (const std::string &name, const std::string &value)
{
  if (name == move_distance_setter_name && ! mp_x_le->hasFocus () && ! mp_y_le->hasFocus ()) {

    try {

      db::DVector mv;
      tl::from_string (value, mv);

      mp_x_le->setText (tl::to_qstring (tl::micron_to_string (mv.x ())));
      mp_y_le->setText (tl::to_qstring (tl::micron_to_string (mv.y ())));

    } catch (...) {
    }

  }
}

//  registers the factory for the move editor options page
static tl::RegisteredClass<lay::EditorOptionsPageFactoryBase> s_factory (new lay::EditorOptionsPageFactory<MoveEditorOptionsPage> ("laybasic::MoveServicePlugin"), 0);

}

#endif
