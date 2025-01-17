
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

#include "layTextProgressDelegate.h"
#include "layMainWindow.h"

namespace lay
{

TextProgressDelegate::TextProgressDelegate (MainWindow *mw, int verbosity)
  : lay::TextProgress (verbosity), mp_mw (mw)
{
  //  .. nothing yet ..
}

void TextProgressDelegate::update_progress (tl::Progress *progress)
{
  if (!mp_mw->update_progress (progress)) {
    lay::TextProgress::update_progress (progress);
  }
}

void TextProgressDelegate::show_progress_bar (bool show)
{
  if (!mp_mw->show_progress_bar (show)) {
    lay::TextProgress::show_progress_bar (show);
  }
}

bool TextProgressDelegate::progress_wants_widget () const
{
  return mp_mw != 0 && mp_mw->progress_wants_widget ();
}

void TextProgressDelegate::progress_add_widget (QWidget *widget)
{
  if (mp_mw) {
    mp_mw->progress_add_widget (widget);
  }
}

QWidget *TextProgressDelegate::progress_get_widget () const
{
  return mp_mw ? mp_mw->progress_get_widget () : 0;
}

void TextProgressDelegate::progress_remove_widget ()
{
  if (mp_mw) {
    mp_mw->progress_remove_widget ();
  }
}

}
