
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

#include "layEditorOptionsFrame.h"
#include "layEditorOptionsPage.h"
#include "layEditorOptionsPages.h"
#include "layPlugin.h"
#include "layLayoutViewBase.h"

#include <QVBoxLayout>

namespace lay
{

EditorOptionsFrame::EditorOptionsFrame (QWidget *parent)
  : QFrame (parent), mp_pages (0)
{
  setObjectName (QString::fromUtf8 ("editor_options_frame"));

  QVBoxLayout *left_frame_ly = new QVBoxLayout (this);
  left_frame_ly->setContentsMargins (0, 0, 0, 0);
  left_frame_ly->setSpacing (0);
}

EditorOptionsFrame::~EditorOptionsFrame ()
{
  //  .. nothing yet ..
}

void
EditorOptionsFrame::populate (LayoutViewBase *view)
{
  std::vector<lay::EditorOptionsPage *> prop_dialog_pages;
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    cls->get_editor_options_pages (prop_dialog_pages, view, view->dispatcher ());
  }

  for (std::vector<lay::EditorOptionsPage *>::const_iterator op = prop_dialog_pages.begin (); op != prop_dialog_pages.end (); ++op) {
    (*op)->activate (false);
  }

  if (mp_pages) {
    delete mp_pages;
  }

  mp_pages = new lay::EditorOptionsPages (this, prop_dialog_pages, view);
  layout ()->addWidget (mp_pages);
  setFocusProxy (mp_pages);
}

}

#endif
