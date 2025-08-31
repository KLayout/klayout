
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
#include "layEditorOptionsPages.h"
#include "tlExceptions.h"
#include "layPlugin.h"
#include "layLayoutViewBase.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QToolButton>
#include <QCompleter>
#include <QLineEdit>

namespace lay
{

// ------------------------------------------------------------------
//  EditorOptionsPages implementation

struct EOPCompareOp
{
  bool operator() (lay::EditorOptionsPage *a, lay::EditorOptionsPage *b) const
  {
    return a->order () < b->order ();
  }
};

EditorOptionsPages::EditorOptionsPages (QWidget *parent, const std::vector<lay::EditorOptionsPage *> &pages, lay::Dispatcher *dispatcher)
  : QFrame (parent), mp_dispatcher (dispatcher)
{
  QVBoxLayout *ly1 = new QVBoxLayout (this);
  ly1->setContentsMargins (0, 0, 0, 0);

  mp_pages = new QTabWidget (this);
  mp_pages->setSizePolicy (QSizePolicy (QSizePolicy::Ignored, QSizePolicy::Ignored));
  ly1->addWidget (mp_pages);

  m_pages = pages;
  for (std::vector <lay::EditorOptionsPage *>::const_iterator p = m_pages.begin (); p != m_pages.end (); ++p) {
    (*p)->set_owner (this);
  }

  update (0);
  setup ();
}

EditorOptionsPages::~EditorOptionsPages ()
{
  while (m_pages.size () > 0) {
    delete m_pages.front ();
  }
}

void
EditorOptionsPages::focusInEvent (QFocusEvent * /*event*/)
{
  //  Sends the focus to the current page's last focus owner
  if (mp_pages->currentWidget () && mp_pages->currentWidget ()->focusWidget ()) {
    mp_pages->currentWidget ()->focusWidget ()->setFocus ();
  }
}

bool EditorOptionsPages::has_content () const
{
  for (std::vector <lay::EditorOptionsPage *>::const_iterator p = m_pages.begin (); p != m_pages.end (); ++p) {
    if ((*p)->active ()) {
      return true;
    }
  }
  return false;
}

void EditorOptionsPages::activate (const lay::Plugin *plugin)
{
  for (auto op = m_pages.begin (); op != m_pages.end (); ++op) {
    bool is_active = false;
    if ((*op)->plugin_declaration () == 0) {
      is_active = (plugin && plugin->plugin_declaration ()->enable_catchall_editor_options_pages ());
    } else if (plugin && plugin->plugin_declaration () == (*op)->plugin_declaration ()) {
      is_active = true;
    }
    (*op)->activate (is_active);
  }
}

void  
EditorOptionsPages::unregister_page (lay::EditorOptionsPage *page)
{
  std::vector <lay::EditorOptionsPage *> pages;
  for (std::vector <lay::EditorOptionsPage *>::const_iterator p = m_pages.begin (); p != m_pages.end (); ++p) {
    if (*p != page) {
      pages.push_back (*p);
    }
  }
  m_pages = pages;
  update (0);
}

void
EditorOptionsPages::make_page_current (lay::EditorOptionsPage *page)
{
  for (int i = 0; i < mp_pages->count (); ++i) {
    if (mp_pages->widget (i) == page) {
      mp_pages->setCurrentIndex (i);
      break;
    }
  }
}

void
EditorOptionsPages::activate_page (lay::EditorOptionsPage *page)
{
  try {
    if (page->active ()) {
      page->setup (mp_dispatcher);
    }
  } catch (...) {
    //  catch any errors related to configuration file errors etc.
  }

  update (page);
}

void   
EditorOptionsPages::update (lay::EditorOptionsPage *page)
{
  std::vector <lay::EditorOptionsPage *> sorted_pages = m_pages;
  std::sort (sorted_pages.begin (), sorted_pages.end (), EOPCompareOp ());

  if (! page && m_pages.size () > 0) {
    page = m_pages.back ();
  }

  while (mp_pages->count () > 0) {
    mp_pages->removeTab (0);
  }
  int index = -1;
  for (std::vector <lay::EditorOptionsPage *>::iterator p = sorted_pages.begin (); p != sorted_pages.end (); ++p) {
    if ((*p)->active ()) {
      if ((*p) == page) {
        index = mp_pages->count ();
      }
      mp_pages->addTab (*p, tl::to_qstring ((*p)->title ()));
    } else {
      (*p)->setParent (0);
    }
  }
  if (index < 0) {
    index = mp_pages->currentIndex ();
  }
  if (index >= int (mp_pages->count ())) {
    index = mp_pages->count () - 1;
  }
  mp_pages->setCurrentIndex (index);

  setVisible (mp_pages->count () > 0);
}

void 
EditorOptionsPages::setup ()
{
  try {

    for (std::vector <lay::EditorOptionsPage *>::iterator p = m_pages.begin (); p != m_pages.end (); ++p) {
      if ((*p)->active ()) {
        (*p)->setup (mp_dispatcher);
      }
    }

    //  make the display consistent with the status (this is important for 
    //  PCell parameters where the PCell may be asked to modify the parameters)
    do_apply ();

  } catch (...) {
    //  catch any errors related to configuration file errors etc.
  }
}

void 
EditorOptionsPages::do_apply ()
{
  for (std::vector <lay::EditorOptionsPage *>::iterator p = m_pages.begin (); p != m_pages.end (); ++p) {
    if ((*p)->active ()) {
      //  NOTE: we apply to the root dispatcher, so other dispatchers (views) get informed too.
      (*p)->apply (mp_dispatcher->dispatcher ());
    }
  }
}

void 
EditorOptionsPages::apply ()
{
BEGIN_PROTECTED
  do_apply ();
END_PROTECTED_W (this)
}

}

#endif
