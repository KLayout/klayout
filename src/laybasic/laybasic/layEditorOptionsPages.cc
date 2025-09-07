
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
#include <QDialogButtonBox>
#include <QPushButton>

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
  mp_modal_pages = new EditorOptionsModalPages (this);

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

  delete mp_modal_pages;
  mp_modal_pages = 0;
}

void
EditorOptionsPages::focusInEvent (QFocusEvent * /*event*/)
{
  //  Sends the focus to the current page's last focus owner
  if (mp_pages->currentWidget () && mp_pages->currentWidget ()->focusWidget ()) {
    mp_pages->currentWidget ()->focusWidget ()->setFocus ();
  }
}

bool
EditorOptionsPages::has_content () const
{
  for (std::vector <lay::EditorOptionsPage *>::const_iterator p = m_pages.begin (); p != m_pages.end (); ++p) {
    if ((*p)->active () && ! (*p)->is_modal_page ()) {
      return true;
    }
  }
  return false;
}

bool
EditorOptionsPages::has_modal_content () const
{
  for (std::vector <lay::EditorOptionsPage *>::const_iterator p = m_pages.begin (); p != m_pages.end (); ++p) {
    if ((*p)->active () && (*p)->is_modal_page ()) {
      return true;
    }
  }
  return false;
}

bool
EditorOptionsPages::exec_modal (EditorOptionsPage *page)
{
  for (int i = 0; i < mp_modal_pages->count (); ++i) {

    if (mp_modal_pages->widget (i) == page) {

      //  found the page - make it current and show the dialog
      mp_modal_pages->set_current_index (i);
      page->setup (mp_dispatcher);
      page->set_focus ();
      return mp_modal_pages->exec () != 0;

    }

  }

  return false;
}

void
EditorOptionsPages::activate (const lay::Plugin *plugin)
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
      page->setup (mp_dispatcher);
      page->set_focus ();
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

  while (mp_modal_pages->count () > 0) {
    mp_modal_pages->remove_page (0);
  }

  int index = -1;
  int modal_index = -1;

  for (std::vector <lay::EditorOptionsPage *>::iterator p = sorted_pages.begin (); p != sorted_pages.end (); ++p) {
    if ((*p)->active ()) {
      if (! (*p)->is_modal_page ()) {
        if ((*p) == page) {
          index = mp_pages->count ();
        }
        mp_pages->addTab (*p, tl::to_qstring ((*p)->title ()));
      } else {
        if ((*p) == page) {
          modal_index = mp_modal_pages->count ();
        }
        mp_modal_pages->add_page (*p);
      }
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

  if (modal_index < 0) {
    modal_index = mp_modal_pages->current_index ();
  }
  if (modal_index >= int (mp_modal_pages->count ())) {
    modal_index = mp_modal_pages->count () - 1;
  }
  mp_modal_pages->set_current_index (modal_index);

  setVisible (mp_pages->count () > 0);
}

void 
EditorOptionsPages::setup ()
{
BEGIN_PROTECTED

  for (std::vector <lay::EditorOptionsPage *>::iterator p = m_pages.begin (); p != m_pages.end (); ++p) {
    if ((*p)->active ()) {
      (*p)->setup (mp_dispatcher);
    }
  }

  //  make the display consistent with the status (this is important for
  //  PCell parameters where the PCell may be asked to modify the parameters)
  do_apply (false);
  do_apply (true);

END_PROTECTED_W (this)
}

void 
EditorOptionsPages::do_apply (bool modal)
{
  for (std::vector <lay::EditorOptionsPage *>::iterator p = m_pages.begin (); p != m_pages.end (); ++p) {
    if ((*p)->active () && modal == (*p)->is_modal_page ()) {
      //  NOTE: we apply to the root dispatcher, so other dispatchers (views) get informed too.
      (*p)->apply (mp_dispatcher->dispatcher ());
    }
  }
}

void 
EditorOptionsPages::apply ()
{
BEGIN_PROTECTED
  do_apply (false);
END_PROTECTED_W (this)
}

// ------------------------------------------------------------------
//  EditorOptionsModalPages implementation

EditorOptionsModalPages::EditorOptionsModalPages (EditorOptionsPages *parent)
  : QDialog (parent), mp_parent (parent), mp_single_page (0)
{
  QVBoxLayout *ly = new QVBoxLayout (this);
  ly->setContentsMargins (0, 0, 0, 0);

  QVBoxLayout *ly4 = new QVBoxLayout (0);
  ly4->setContentsMargins (6, 6, 6, 0);
  ly->addLayout (ly4);
  mp_pages = new QTabWidget (this);
  ly4->addWidget (mp_pages, 1);
#if QT_VERSION >= 0x50400
  mp_pages->setTabBarAutoHide (true);
#endif
  mp_pages->hide ();

  mp_single_page_frame = new QFrame (this);
  QVBoxLayout *ly2 = new QVBoxLayout (mp_single_page_frame);
  ly2->setContentsMargins (0, 0, 0, 0);
  ly->addWidget (mp_single_page_frame, 1);
  mp_single_page_frame->hide ();

  QVBoxLayout *ly3 = new QVBoxLayout (0);
  ly3->setContentsMargins (6, 6, 6, 6);
  ly->addLayout (ly3);
  mp_button_box = new QDialogButtonBox (this);
  ly3->addWidget (mp_button_box);
  mp_button_box->setOrientation (Qt::Horizontal);
  mp_button_box->setStandardButtons (QDialogButtonBox::Cancel | QDialogButtonBox::Apply | QDialogButtonBox::Ok);

  connect (mp_button_box, SIGNAL (clicked(QAbstractButton *)), this, SLOT (clicked(QAbstractButton *)));
  connect (mp_button_box, SIGNAL (accepted()), this, SLOT (accept()));
  connect (mp_button_box, SIGNAL (rejected()), this, SLOT (reject()));

  update_title ();
}

EditorOptionsModalPages::~EditorOptionsModalPages ()
{
  //  .. nothing yet ..
}

int
EditorOptionsModalPages::count ()
{
  return mp_single_page ? 1 : mp_pages->count ();
}

int
EditorOptionsModalPages::current_index ()
{
  return mp_single_page ? 0 : mp_pages->currentIndex ();
}

void
EditorOptionsModalPages::set_current_index (int index)
{
  if (! mp_single_page) {
    mp_pages->setCurrentIndex (index);
  }
}

void
EditorOptionsModalPages::add_page (EditorOptionsPage *page)
{
  if (! mp_single_page) {
    if (mp_pages->count () == 0) {
      mp_single_page = page;
      mp_single_page->setParent (mp_single_page_frame);
      mp_single_page_frame->layout ()->addWidget (mp_single_page);
      mp_single_page_frame->show ();
      mp_pages->hide ();
    } else {
      mp_pages->addTab (page, tl::to_qstring (page->title ()));
    }
  } else {
    mp_pages->clear ();
    mp_single_page_frame->layout ()->removeWidget (mp_single_page);
    mp_single_page_frame->hide ();
    mp_pages->addTab (mp_single_page, tl::to_qstring (mp_single_page->title ()));
    mp_single_page = 0;
    mp_pages->addTab (page, tl::to_qstring (page->title ()));
    mp_pages->show ();
  }

  update_title ();
}

void
EditorOptionsModalPages::remove_page (int index)
{
  if (mp_single_page) {
    if (index == 0) {
      mp_single_page->setParent (0);
      mp_single_page = 0;
      mp_single_page_frame->hide ();
      mp_single_page_frame->layout ()->removeWidget (mp_single_page);
    }
  } else {
    mp_pages->removeTab (index);
    if (mp_pages->count () == 1) {
      mp_pages->hide ();
      mp_single_page = dynamic_cast<EditorOptionsPage *> (mp_pages->widget (0));
      mp_pages->removeTab (0);
      mp_single_page->setParent (mp_single_page_frame);
      mp_single_page_frame->layout ()->addWidget (mp_single_page);
      mp_single_page_frame->show ();
    }
  }

  update_title ();
}

void
EditorOptionsModalPages::update_title ()
{
  if (mp_single_page) {
    setWindowTitle (tl::to_qstring (mp_single_page->title ()));
  } else {
    setWindowTitle (tr ("Editor Options"));
  }
}

EditorOptionsPage *
EditorOptionsModalPages::widget (int index)
{
  if (mp_single_page) {
    return index == 0 ? mp_single_page : 0;
  } else {
    return dynamic_cast<EditorOptionsPage *> (mp_pages->widget (index));
  }
}

void
EditorOptionsModalPages::accept ()
{
BEGIN_PROTECTED
  mp_parent->do_apply (true);
  QDialog::accept ();
END_PROTECTED
}

void
EditorOptionsModalPages::reject ()
{
  QDialog::reject ();
}

void
EditorOptionsModalPages::clicked (QAbstractButton *button)
{
BEGIN_PROTECTED
  if (button == mp_button_box->button (QDialogButtonBox::Apply)) {
    mp_parent->do_apply (true);
  }
END_PROTECTED
}

}

#endif
