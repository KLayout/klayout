
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

#include "antService.h"
#include "antEditorOptionsPages.h"

#include "layWidgets.h"
#include "layDispatcher.h"
#include "tlInternational.h"

#include <QHBoxLayout>

namespace ant
{

// ------------------------------------------------------------------
//  Annotations Toolbox widget

ToolkitWidget::ToolkitWidget (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
  : lay::EditorOptionsPageWidget (view, dispatcher)
{
  mp_layout = new QHBoxLayout (this);

  mp_x_le = new lay::DecoratedLineEdit (this);
  mp_x_le->set_label ("dx:");
  mp_layout->addWidget (mp_x_le);

  mp_y_le = new lay::DecoratedLineEdit (this);
  mp_y_le->set_label ("dy:");
  mp_layout->addWidget (mp_y_le);

  mp_d_le = new lay::DecoratedLineEdit (this);
  mp_d_le->set_label ("d:");
  mp_layout->addWidget (mp_d_le);

  mp_layout->addStretch (1);

  hide ();

  set_toolbox_widget (true);
  set_transparent (true);
}

ToolkitWidget::~ToolkitWidget ()
{
  //  .. nothing yet ..
}

std::string
ToolkitWidget::title () const
{
  return "Box Options";
}

const char *
ToolkitWidget::name () const
{
  return ant::Service::editor_options_name ();
}

void
ToolkitWidget::deactivated ()
{
  hide ();
}

void
ToolkitWidget::commit (lay::Dispatcher *dispatcher)
{
  try {

    if (mp_d_le->hasFocus ()) {

      double d = 0.0;

      tl::from_string (tl::to_string (mp_d_le->text ()), d);

      dispatcher->call_function (ant::Service::d_function_name (), tl::to_string (d));

    } else {

      double dx = 0.0, dy = 0.0;

      tl::from_string (tl::to_string (mp_x_le->text ()), dx);
      tl::from_string (tl::to_string (mp_y_le->text ()), dy);

      dispatcher->call_function (ant::Service::xy_function_name (), db::DVector (dx, dy).to_string ());

    }

  } catch (...) {
  }
}

void
ToolkitWidget::configure (const std::string &name, const std::string &value)
{
  if (name == ant::Service::xy_configure_name () && ! mp_x_le->hasFocus () && ! mp_y_le->hasFocus ()) {

    try {

      db::DVector mv;
      tl::from_string (value, mv);

      mp_x_le->setText (tl::to_qstring (tl::micron_to_string (mv.x ())));
      mp_y_le->setText (tl::to_qstring (tl::micron_to_string (mv.y ())));

    } catch (...) {
    }

  } else if (name == ant::Service::d_configure_name () && ! mp_x_le->hasFocus () && ! mp_y_le->hasFocus ()) {

    try {

      double d;
      tl::from_string (value, d);

      mp_d_le->setText (tl::to_qstring (tl::micron_to_string (d)));

    } catch (...) {
    }

  }
}

// ------------------------------------------------------------------
//  Registrations

//  toolkit widgets
static tl::RegisteredClass<lay::EditorOptionsPageFactoryBase> s_tookit_widget_factory (new lay::EditorOptionsPageFactory<ToolkitWidget> ("ant::Plugin"), 0);

}

#endif
