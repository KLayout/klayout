
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
#include "layEditorOptionsPageWidget.h"
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

EditorOptionsPages::EditorOptionsPages (QWidget *parent, lay::LayoutViewBase *view, const std::vector<lay::EditorOptionsPage *> &pages)
  : QFrame (parent), mp_view (view), m_update_enabled (true)
{
  mp_modal_pages = new EditorOptionsModalPages (this);

  QVBoxLayout *ly1 = new QVBoxLayout (this);
  ly1->setContentsMargins (0, 0, 0, 0);

  mp_pages = new QTabWidget (this);
  mp_pages->setSizePolicy (QSizePolicy (QSizePolicy::Ignored, QSizePolicy::Ignored));
  ly1->addWidget (mp_pages);

  for (auto p = pages.begin (); p != pages.end (); ++p) {
    m_pages.push_back (*p);
  }

  for (auto p = m_pages.begin (); p != m_pages.end (); ++p) {
    p->set_owner (this);
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

std::vector<lay::EditorOptionsPage *>
EditorOptionsPages::editor_options_pages (const lay::PluginDeclaration *plugin_declaration)
{
  std::vector<lay::EditorOptionsPage *> pages;
  for (auto p = m_pages.begin (); p != m_pages.end (); ++p) {
    if (p->for_plugin_declaration (plugin_declaration)) {
      pages.push_back (const_cast<lay::EditorOptionsPage *> (p.operator-> ()));
    }
  }
  return pages;
}

std::vector<lay::EditorOptionsPage *>
EditorOptionsPages::editor_options_pages ()
{
  std::vector<lay::EditorOptionsPage *> pages;
  for (auto p = m_pages.begin (); p != m_pages.end (); ++p) {
    pages.push_back (const_cast<lay::EditorOptionsPage *> (p.operator-> ()));
  }
  return pages;
}

bool
EditorOptionsPages::has_content () const
{
  for (auto p = m_pages.begin (); p != m_pages.end (); ++p) {
    if (p->active () && ! p->is_modal_page () && ! p->is_toolbox_widget ()) {
      return true;
    }
  }
  return false;
}

bool
EditorOptionsPages::has_modal_content () const
{
  for (auto p = m_pages.begin (); p != m_pages.end (); ++p) {
    if (p->active () && p->is_modal_page () && ! p->is_toolbox_widget ()) {
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
      page->setup (mp_view);
      page->set_focus ();
      return mp_modal_pages->exec () != 0;

    }

  }

  return false;
}

void
EditorOptionsPages::activate (const lay::Plugin *plugin)
{
  m_update_enabled = false;

  lay::EditorOptionsPage *page = 0;

  for (auto op = m_pages.begin (); op != m_pages.end (); ++op) {

    BEGIN_PROTECTED

    bool is_active = plugin && op->for_plugin_declaration (plugin->plugin_declaration ());

    //  The zero order page is picked as the initial one
    if (is_active && ! op->active () && op->order () == 0 && page == 0) {
      page = op.operator-> ();
    }

    op->activate (is_active);

    END_PROTECTED

  }

  m_update_enabled = true;

  update (page);
}

void  
EditorOptionsPages::unregister_page (lay::EditorOptionsPage *page)
{
  m_pages.erase (page);
  update (0);
}

lay::EditorOptionsPage *
EditorOptionsPages::page_with_name (const std::string &name)
{
  for (auto p = m_pages.begin (); p != m_pages.end (); ++p) {
    if (p->name () && name == p->name ()) {
      return p.operator-> ();
    }
  }
  return 0;
}

void
EditorOptionsPages::make_page_current (lay::EditorOptionsPage *page)
{
  for (int i = 0; i < mp_pages->count (); ++i) {
    if (mp_pages->widget (i) == page->widget ()) {
      mp_pages->setCurrentIndex (i);
      page->setup (mp_view);
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
      page->setup (mp_view);
    }
  } catch (...) {
    //  catch any errors related to configuration file errors etc.
  }

  update (page);
}

void   
EditorOptionsPages::update (lay::EditorOptionsPage *page)
{
  if (! m_update_enabled) {
    return;
  }

  int index = mp_pages->currentIndex ();
  int modal_index = -1;

  std::vector <lay::EditorOptionsPageWidget *> sorted_pages;
  for (auto p = m_pages.begin (); p != m_pages.end (); ++p) {
    if (p->widget ()) {
      sorted_pages.push_back (p->widget ());
    }
  }
  std::sort (sorted_pages.begin (), sorted_pages.end (), EOPCompareOp ());

  while (mp_pages->count () > 0) {
    mp_pages->removeTab (0);
  }

  while (mp_modal_pages->count () > 0) {
    mp_modal_pages->remove_page (0);
  }

  for (auto p = sorted_pages.begin (); p != sorted_pages.end (); ++p) {

    if ((*p)->is_toolbox_widget ()) {

      //  NOTE: toolbox widgets are always created, but hidden if the
      //  page is not active. However, even inactive pages can become
      //  visible, if needed. The "move" plugin does that if used from
      //  externally.
      if (! (*p)->active ()) {
        (*p)->set_visible (false);
      }

      mp_view->add_toolbox_widget (*p);

    } else if ((*p)->active ()) {

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
    index = 0;
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

  for (auto p = m_pages.begin (); p != m_pages.end (); ++p) {
    if (p->active ()) {
      p->setup (mp_view);
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
  for (auto p = m_pages.begin (); p != m_pages.end (); ++p) {
    if (p->active () && modal == p->is_modal_page ()) {
      //  NOTE: we apply to the root dispatcher, so other dispatchers (views) get informed too.
      p->apply (mp_view->dispatcher ());
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
EditorOptionsModalPages::add_page (EditorOptionsPageWidget *page)
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
      mp_single_page = dynamic_cast<EditorOptionsPageWidget *> (mp_pages->widget (0));
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
