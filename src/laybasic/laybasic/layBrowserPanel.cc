
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#include "layBrowserPanel.h"
#include "tlExceptions.h"
#include "tlInternational.h"
#include "tlException.h"

#include "ui_BrowserPanel.h"

#include <cstdio>
#if QT_VERSION >= 0x050000
#  include <QUrlQuery>
#endif

#include <QTreeWidgetItem>

namespace lay
{

// -------------------------------------------------------------

QVariant 
BrowserTextWidget::loadResource (int type, const QUrl &url)
{
  if (mp_panel && url.scheme () == QString::fromUtf8 ("int")) {
    return mp_panel->loadResource (type, url);
  } else {
    return QTextBrowser::loadResource (type, url);
  }
}

// -------------------------------------------------------------

BrowserPanel::BrowserPanel (QWidget *parent)
  : QWidget (parent),
    m_back_dm (this, &BrowserPanel::back)
{
  init ();
}

void 
BrowserPanel::init ()
{
  m_enable_load = false;
  m_enable_reject = false;
  mp_source.reset (0);

  mp_ui = new Ui::BrowserPanel ();
  mp_ui->setupUi (this);

  mp_ui->browser->setReadOnly (true);
  mp_ui->browser->set_panel (this);
  mp_ui->browser->setWordWrapMode (QTextOption::WordWrap);

  connect (mp_ui->back_pb, SIGNAL (clicked ()), this, SLOT (back ()));
  connect (mp_ui->forward_pb, SIGNAL (clicked ()), this, SLOT (forward ()));
  connect (mp_ui->next_topic_pb, SIGNAL (clicked ()), this, SLOT (next ()));
  connect (mp_ui->prev_topic_pb, SIGNAL (clicked ()), this, SLOT (prev ()));
  connect (mp_ui->home_pb, SIGNAL (clicked ()), this, SLOT (home ()));
  connect (mp_ui->searchEdit, SIGNAL (returnPressed ()), this, SLOT (search_edited ()));
  connect (mp_ui->browser, SIGNAL (textChanged ()), this, SLOT (text_changed ()));
  connect (mp_ui->browser, SIGNAL (backwardAvailable (bool)), mp_ui->back_pb, SLOT (setEnabled (bool)));
  connect (mp_ui->browser, SIGNAL (forwardAvailable (bool)), mp_ui->forward_pb, SLOT (setEnabled (bool)));
  connect (mp_ui->outline_tree, SIGNAL (itemActivated (QTreeWidgetItem *, int)), this, SLOT (outline_item_clicked (QTreeWidgetItem *)));

  mp_ui->searchEdit->hide ();

  set_label (std::string ());
}

BrowserPanel::~BrowserPanel ()
{
  set_source (0);
  mp_ui->browser->set_panel (0);

  delete mp_ui;
  mp_ui = 0;
}

std::string
BrowserPanel::title () const
{
  return tl::to_string (m_current_title);
}

std::string
BrowserPanel::url () const
{
  return m_cached_url;
}

void
BrowserPanel::text_changed ()
{
  QString title = mp_ui->browser->document ()->metaInformation (QTextDocument::DocumentTitle);
  if (title != m_current_title) {
    m_current_title = title;
    emit title_changed (title);
  }
}

void
BrowserPanel::outline_item_clicked (QTreeWidgetItem *item)
{
  QString url = item->data (0, Qt::UserRole).toString ();
  if (! url.isEmpty ()) {
    load (tl::to_string (url));
  }
}

void 
BrowserPanel::load (const std::string &s)
{
  mp_ui->browser->setSource (QUrl (tl::to_qstring (s)));
}

void 
BrowserPanel::set_source (BrowserSource *source)
{
  m_enable_reject = false;
  m_enable_load = false;

  if (mp_source.get ()) {
    mp_source->detach (this);
    //  release the reference to the source object
    mp_source->gsi::ObjectBase::release ();
  }

  mp_source.reset (source);

  if (mp_source.get ()) {

    m_enable_load = true;

    // hold a reference to the source object for GSI
    mp_source->gsi::ObjectBase::keep ();
    mp_source->attach (this);

    mp_ui->browser->clearHistory ();
    reload ();
    m_enable_reject = true;

  }
}

void 
BrowserPanel::set_home (const std::string &url)
{
  m_home = url;
  home ();

  //  NOTE: we take this call as a hint that the panel is set up and about to be
  //  shown. We use this opportunity to resize the outline pane.
  mp_ui->outline_tree->header ()->hide ();
  QList<int> sizes = mp_ui->splitter->sizes ();
  if (sizes.size () >= 2) {
    int size_outline = 150;
    sizes[1] += sizes[0] - size_outline;
    sizes[0] = size_outline;
  }
  mp_ui->splitter->setSizes (sizes);
}

void 
BrowserPanel::reload ()
{
  //  clear caches to force a reload
  m_cached_url = "";
  m_cached_text = "";

  //  disable reload while we are in a loadResource call - clearing the cache will be sufficient
  if (m_enable_load) {
    mp_ui->browser->reload ();
    emit url_changed (tl::to_qstring (m_cached_url));
  }
}

void
BrowserPanel::prev ()
{
  mp_ui->browser->setSource (QUrl (tl::to_qstring (m_cached_prev_url)));
  reload ();
}

void
BrowserPanel::next ()
{
  mp_ui->browser->setSource (QUrl (tl::to_qstring (m_cached_next_url)));
  reload ();
}

void 
BrowserPanel::back ()
{
  mp_ui->browser->backward ();
}

void 
BrowserPanel::forward ()
{
  mp_ui->browser->forward ();
}

void 
BrowserPanel::home ()
{
  bool needs_reload = (m_home == m_cached_url);
  mp_ui->browser->setSource (QUrl (tl::to_qstring (m_home)));
  if (needs_reload) {
    reload ();
  }
}

QSize  
BrowserPanel::sizeHint () const
{
  return QSize (800, 600);
}

void
BrowserPanel::search (const std::string &s)
{
  if (! s.empty ()) {
    QUrl url (tl::to_qstring (m_search_url));
#if QT_VERSION >= 0x050000
    QUrlQuery qi;
    qi.addQueryItem (tl::to_qstring (m_search_query_item), tl::to_qstring (s));
    url.setQuery (qi);
#else
    QList<QPair<QString, QString> > qi;
    qi.push_back (QPair<QString, QString> (tl::to_qstring (m_search_query_item), tl::to_qstring (s)));
    url.setQueryItems (qi);
#endif
    load (url.toEncoded ().constData ());
  }
}

void
BrowserPanel::search_edited ()
{
  if (mp_ui->searchEdit->text ().size () > 0) {
    QUrl url (tl::to_qstring (m_search_url));
#if QT_VERSION >= 0x050000
    QUrlQuery qi;
    qi.addQueryItem (tl::to_qstring (m_search_query_item), mp_ui->searchEdit->text ());
    url.setQuery (qi);
#else
    QList<QPair<QString, QString> > qi;
    qi.push_back (QPair<QString, QString> (tl::to_qstring (m_search_query_item), mp_ui->searchEdit->text ()));
    url.setQueryItems (qi);
#endif
    load (url.toEncoded ().constData ());
  }
}

void 
BrowserPanel::set_search_url (const std::string &url, const std::string &query_item)
{
  m_search_url = url;
  m_search_query_item = query_item;
  mp_ui->searchEdit->setVisible (! url.empty ());
}

void 
BrowserPanel::set_label (const std::string &text)
{
  mp_ui->label->setText (tl::to_qstring (text));
  mp_ui->label->setVisible (! text.empty ());
}

static void
update_item_with_outline (const BrowserOutline &ol, QTreeWidgetItem *item)
{
  item->setData (0, Qt::UserRole, tl::to_qstring (ol.url ()));
  item->setData (0, Qt::DisplayRole, tl::to_qstring (ol.title ()));
  item->setData (0, Qt::ToolTipRole, tl::to_qstring (ol.title ()));

  int i = 0;
  for (BrowserOutline::const_child_iterator c = ol.begin (); c != ol.end (); ++c, ++i) {
    if (item->childCount () <= i) {
      new QTreeWidgetItem (item);
    }
    update_item_with_outline (*c, item->child (i));
  }

  while (item->childCount () > i) {
    delete item->child (i);
  }
}

void
BrowserPanel::set_outline (const BrowserOutline &ol)
{
  if (ol.begin () == ol.end ()) {

    mp_ui->outline_tree->hide ();

  } else {

    mp_ui->outline_tree->show ();

    int i = 0;
    for (BrowserOutline::const_child_iterator c = ol.begin (); c != ol.end (); ++c, ++i) {
      if (mp_ui->outline_tree->topLevelItemCount () <= i) {
        new QTreeWidgetItem (mp_ui->outline_tree);
      }
      update_item_with_outline (*c, mp_ui->outline_tree->topLevelItem (i));
    }

    while (mp_ui->outline_tree->topLevelItemCount () > i) {
      delete mp_ui->outline_tree->topLevelItem (i);
    }

    mp_ui->outline_tree->expandAll ();

  }
}

QVariant 
BrowserPanel::loadResource (int type, const QUrl &url)
{
  if (type == QTextDocument::ImageResource) {

    BEGIN_PROTECTED
    return QVariant (mp_source->get_image (tl::to_string (url.toString ())));
    END_PROTECTED
    return QVariant ();

  } else if (type == QTextDocument::StyleSheetResource) {

    BEGIN_PROTECTED
    return QVariant (tl::to_qstring (mp_source->get_css (tl::to_string (url.toString ()))));
    END_PROTECTED
    return QVariant ();

  } else if (type != QTextDocument::HtmlResource) {

    return QVariant ();

  } else {

    QVariant ret;

    //  recursion sentinel: avoid recursion by any action within mp_source->get that causes a "loadResource"
    if (! m_enable_load || !mp_source.get ()) {
      //  return any dummy in this case - otherwise the QTestBrowser complains about not having anything.
      return QVariant (QString::fromUtf8 (" "));
    }

    m_enable_load = false;

    //  Qt sets the override cursor in response to link clicks - this is not appropriate for some
    //  GSI callback implementations that show InputDialogs for example. Therefore we install out own
    //  (normal) override cursor.
    QApplication::setOverrideCursor (QCursor (Qt::ArrowCursor));

    BEGIN_PROTECTED

      std::string u = tl::to_string (url.toString ());
      std::string s;
      std::string nu, pu;
      BrowserOutline ol;
      if (u == m_cached_url) {
        s = m_cached_text;
        nu = m_cached_next_url;
        pu = m_cached_prev_url;
        ol = m_cached_outline;
      } else {
        s = mp_source->get (u);
        nu = mp_source->next_topic (u);
        pu = mp_source->prev_topic (u);
        ol = mp_source->get_outline (u);
      }
      if (s.empty ()) {
        s = " "; // QTextBrowser needs at least something
        //  The only way (as far as I know in Qt <4.2) to suppress navigation to 
        //  the Url is to schedule a delayed "back" signal. In Qt >= 4.2 we could register
        //  an external handler for "int" schemes that would do nothing ..
        if (m_enable_reject) {
          m_back_dm ();
        }
      } else {
        //  to avoid regeneration of text on artificial "back" events, the last page is cached
        m_cached_text = s;
        m_cached_url = u;
        m_cached_next_url = nu;
        m_cached_prev_url = pu;
        m_cached_outline = ol;
      }

      ret = QVariant (tl::to_qstring (s));

      if (pu.empty () && nu.empty ()) {
        mp_ui->prev_topic_pb->hide ();
        mp_ui->next_topic_pb->hide ();
      } else {
        mp_ui->prev_topic_pb->show ();
        mp_ui->prev_topic_pb->setEnabled (! pu.empty ());
        mp_ui->next_topic_pb->show ();
        mp_ui->next_topic_pb->setEnabled (! nu.empty ());
      }

      //  push the outline
      set_outline (ol);

    END_PROTECTED

    QApplication::restoreOverrideCursor ();

    m_enable_load = true;
    return ret;

  }
}

// -------------------------------------------------------------

BrowserSource::BrowserSource ()
{
  //  .. nothing yet ..
}

BrowserSource::BrowserSource (const std::string &html)
  : m_default_html (html)
{
  //  .. nothing yet ..
}

BrowserSource::~BrowserSource ()
{
  std::set<BrowserPanel *> owners;
  owners.swap (mp_owners);
  for (std::set<BrowserPanel *>::const_iterator o = owners.begin (); o != owners.end (); ++o) {
    (*o)->set_source (0);
  }
}

std::string
BrowserSource::get_css (const std::string & /*url*/) 
{
  return std::string ();
}

QImage
BrowserSource::get_image (const std::string & /*url*/) 
{
  return QImage ();
}

BrowserOutline
BrowserSource::get_outline (const std::string & /*url*/)
{
  return BrowserOutline ();
}

std::string 
BrowserSource::get (const std::string & /*url*/) 
{
  return m_default_html;
}

void 
BrowserSource::detach (lay::BrowserPanel *d)
{
  mp_owners.erase (d);
}

void 
BrowserSource::attach (lay::BrowserPanel *d)
{
  mp_owners.insert (d);
}

}


