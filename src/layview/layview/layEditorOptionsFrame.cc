
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
  std::vector<lay::EditorOptionsPage *> editor_options_pages;
  std::map<std::string, std::vector<const lay::PluginDeclaration *> > additional_pages;

  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    cls->get_editor_options_pages (editor_options_pages, view, view->dispatcher ());
    std::vector<std::string> ap = cls->additional_editor_options_pages ();
    for (auto i = ap.begin (); i != ap.end (); ++i) {
      additional_pages [*i].push_back (cls.operator-> ());
    }
  }

  lay::PluginDeclaration::get_additional_editor_options_pages (editor_options_pages, view, view->dispatcher (), additional_pages);

  for (std::vector<lay::EditorOptionsPage *>::const_iterator op = editor_options_pages.begin (); op != editor_options_pages.end (); ++op) {
    (*op)->activate (false);
  }

  if (mp_pages) {
    delete mp_pages;
  }

  mp_pages = new lay::EditorOptionsPages (this, view, editor_options_pages);
  layout ()->addWidget (mp_pages);
  setFocusProxy (mp_pages);
}

}

#endif
