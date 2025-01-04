
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

#include "layBrowserPanel.h"
#include "layDispatcher.h"
#include "tlExceptions.h"
#include "tlInternational.h"
#include "tlException.h"
#include "tlString.h"

#include "ui_BrowserPanel.h"

#include <cstdio>
#if QT_VERSION >= 0x050000
#  include <QUrlQuery>
#endif

#include <QTreeWidgetItem>
#include <QTextBlock>
#include <QCompleter>
#include <QStringListModel>
#include <QScrollBar>

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

void
BookmarkItem::read (tl::Extractor &ex)
{
  while (! ex.at_end () && ! ex.test (";")) {

    std::string k, v;
    ex.read_word (k);
    ex.test (":");
    ex.read_word_or_quoted (v, "+-.");
    ex.test (",");

    if (k == "url") {
      url = v;
    } else if (k == "title") {
      title = v;
    } else if (k == "position") {
      tl::from_string (v, position);
    }

  }
}

std::string
BookmarkItem::to_string () const
{
  std::string r;
  r = "url:" + tl::to_quoted_string (url) + ",";
  r += "title:" + tl::to_quoted_string (title) + ",";
  r += "position:" + tl::to_string (position) + ";";
  return r;
}

// -------------------------------------------------------------

BrowserPanel::BrowserPanel (QWidget *parent)
  : QWidget (parent),
    m_back_dm (this, &BrowserPanel::back),
    m_new_url_dm (this, &BrowserPanel::new_url),
    mp_dispatcher (0)
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

#if QT_VERSION >= 0x050200
  mp_ui->on_page_search_edit->setClearButtonEnabled (true);
  mp_ui->search_edit->setClearButtonEnabled (true);
#endif

  mp_ui->browser->setReadOnly (true);
  mp_ui->browser->set_panel (this);
  mp_ui->browser->setWordWrapMode (QTextOption::WordWrap);
  mp_ui->browser->setLineWrapMode (QTextEdit::FixedPixelWidth);
  QFontMetrics fm (font ());
  int text_width = fm.boundingRect ('m').width () * 80;
  mp_ui->browser->setLineWrapColumnOrWidth (text_width);

  mp_ui->browser->addAction (mp_ui->action_find);
  mp_ui->browser->addAction (mp_ui->action_bookmark);
  mp_ui->browser->setOpenLinks (false);

  mp_ui->browser_bookmark_view->addAction (mp_ui->action_delete_bookmark);
  mp_ui->browser_bookmark_view->setContextMenuPolicy (Qt::ActionsContextMenu);

  connect (mp_ui->back_pb, SIGNAL (clicked ()), this, SLOT (back ()));
  connect (mp_ui->forward_pb, SIGNAL (clicked ()), this, SLOT (forward ()));
  connect (mp_ui->next_topic_pb, SIGNAL (clicked ()), this, SLOT (next ()));
  connect (mp_ui->prev_topic_pb, SIGNAL (clicked ()), this, SLOT (prev ()));
  connect (mp_ui->bookmark_pb, SIGNAL (clicked ()), this, SLOT (bookmark ()));
  connect (mp_ui->home_pb, SIGNAL (clicked ()), this, SLOT (home ()));
  connect (mp_ui->search_edit, SIGNAL (textEdited (const QString &)), this, SLOT (search_text_changed (const QString &)));
  connect (mp_ui->search_edit, SIGNAL (returnPressed ()), this, SLOT (search_edited ()));
  connect (mp_ui->search_button, SIGNAL (clicked ()), this, SLOT (search_edited ()));
  connect (mp_ui->browser, SIGNAL (sourceChanged (const QUrl &)), this, SLOT (source_changed ()));
  connect (mp_ui->browser, SIGNAL (anchorClicked (const QUrl &)), this, SLOT (anchor_clicked (const QUrl &)));
  connect (mp_ui->browser, SIGNAL (backwardAvailable (bool)), mp_ui->back_pb, SLOT (setEnabled (bool)));
  connect (mp_ui->browser, SIGNAL (forwardAvailable (bool)), mp_ui->forward_pb, SLOT (setEnabled (bool)));
  connect (mp_ui->outline_tree, SIGNAL (itemActivated (QTreeWidgetItem *, int)), this, SLOT (outline_item_clicked (QTreeWidgetItem *)));
  connect (mp_ui->on_page_search_edit, SIGNAL (textChanged (const QString &)), this, SLOT (page_search_edited ()));
  connect (mp_ui->search_close_button, SIGNAL (clicked ()), this, SLOT (page_search_edited ()), Qt::QueuedConnection);
  connect (mp_ui->on_page_search_edit, SIGNAL (returnPressed ()), this, SLOT (page_search_next ()));
  connect (mp_ui->on_page_search_next, SIGNAL (clicked ()), this, SLOT (page_search_next ()));
  connect (mp_ui->action_find, SIGNAL (triggered ()), this, SLOT (find ()));
  connect (mp_ui->action_bookmark, SIGNAL (triggered ()), this, SLOT (bookmark ()));
  connect (mp_ui->action_delete_bookmark, SIGNAL (triggered ()), this, SLOT (delete_bookmark ()));
  connect (mp_ui->browser_bookmark_view, SIGNAL (itemDoubleClicked (QTreeWidgetItem *, int)), this, SLOT (bookmark_item_selected (QTreeWidgetItem *)));

  mp_completer = new QCompleter (this);
#if QT_VERSION >= 0x050200
  mp_completer->setFilterMode (Qt::MatchStartsWith);
#endif
  mp_completer->setCaseSensitivity (Qt::CaseInsensitive);
  mp_completer->setCompletionMode (QCompleter::UnfilteredPopupCompletion);
  mp_completer_model = new QStringListModel (mp_completer);
  mp_completer->setModel (mp_completer_model);
  mp_ui->search_edit->setCompleter (mp_completer);

  mp_ui->search_frame->hide ();
  mp_ui->search_edit->hide ();

  set_label (std::string ());

  refresh_bookmark_list ();
}

BrowserPanel::~BrowserPanel ()
{
  set_source (0);
  mp_ui->browser->set_panel (0);

  delete mp_ui;
  mp_ui = 0;
}

void
BrowserPanel::set_dispatcher (lay::Dispatcher *dispatcher, const std::string &cfg_bookmarks)
{
  mp_dispatcher = dispatcher;
  m_cfg_bookmarks = cfg_bookmarks;

  m_bookmarks.clear ();

  //  load the bookmarks
  try {

    if (mp_dispatcher) {

      std::string v;
      mp_dispatcher->config_get (m_cfg_bookmarks, v);

      tl::Extractor ex (v.c_str ());
      while (! ex.at_end ()) {
        m_bookmarks.push_back (BookmarkItem ());
        m_bookmarks.back ().read (ex);
      }

    }

  } catch (...) {
    //  exceptions ignored here
  }

  refresh_bookmark_list ();
}

std::string
BrowserPanel::title () const
{
  return tl::to_string (m_current_title);
}

std::string
BrowserPanel::url () const
{
  return tl::to_string (mp_ui->browser->source ().toString ());
}

void
BrowserPanel::bookmark ()
{
  BookmarkItem bm;
  bm.url = tl::to_string (mp_ui->browser->historyUrl (0).toString ());
  QString title = mp_ui->browser->document ()->metaInformation (QTextDocument::DocumentTitle);
  bm.title = tl::to_string (title);
  bm.position = mp_ui->browser->verticalScrollBar ()->value ();

  add_bookmark (bm);
  refresh_bookmark_list ();
  store_bookmarks ();
}

void
BrowserPanel::store_bookmarks ()
{
  if (mp_dispatcher) {

    std::string s;
    for (std::list<BookmarkItem>::const_iterator i = m_bookmarks.begin (); i != m_bookmarks.end (); ++i) {
      s += i->to_string ();
    }

    mp_dispatcher->config_set (m_cfg_bookmarks, s);

  }
}

void
BrowserPanel::bookmark_item_selected (QTreeWidgetItem *item)
{
  int index = mp_ui->browser_bookmark_view->indexOfTopLevelItem (item);
  if (index < 0 || index >= int (m_bookmarks.size ())) {
    return;
  }

  std::list<BookmarkItem>::iterator i = m_bookmarks.begin ();
  for ( ; i != m_bookmarks.end () && index > 0; --index, ++i)
    ;

  if (i == m_bookmarks.end ()) {
    return;
  }

  BookmarkItem bm = *i;
  m_bookmarks.erase (i);
  m_bookmarks.push_front (bm);

  refresh_bookmark_list ();
  store_bookmarks ();
  load (bm.url);

  mp_ui->browser->verticalScrollBar ()->setValue (bm.position);
  mp_ui->browser_bookmark_view->topLevelItem (0)->setSelected (true);
}

void
BrowserPanel::clear_bookmarks ()
{
  m_bookmarks.clear ();
}

void
BrowserPanel::add_bookmark (const BookmarkItem &item)
{
  for (std::list<BookmarkItem>::iterator i = m_bookmarks.begin (); i != m_bookmarks.end (); ) {
    std::list<BookmarkItem>::iterator ii = i;
    ++ii;
    if (*i == item) {
      m_bookmarks.erase (i);
    }
    i = ii;
  }
  m_bookmarks.push_front (item);
}

void
BrowserPanel::delete_bookmark ()
{
  QTreeWidgetItem *item = mp_ui->browser_bookmark_view->currentItem ();
  if (! item) {
    return;
  }

  int index = mp_ui->browser_bookmark_view->indexOfTopLevelItem (item);
  std::list<BookmarkItem>::iterator i = m_bookmarks.begin ();
  for ( ; i != m_bookmarks.end () && index > 0; --index, ++i)
    ;

  if (i != m_bookmarks.end ()) {
    m_bookmarks.erase (i);
    refresh_bookmark_list ();
    store_bookmarks ();
  }
}

void
BrowserPanel::refresh_bookmark_list ()
{
  mp_ui->browser_bookmark_view->setVisible (! m_bookmarks.empty ());

  mp_ui->browser_bookmark_view->clear ();
  for (std::list<BookmarkItem>::const_iterator i = m_bookmarks.begin (); i != m_bookmarks.end (); ++i) {
    QTreeWidgetItem *item = new QTreeWidgetItem (mp_ui->browser_bookmark_view);
    item->setData (0, Qt::DisplayRole, tl::to_qstring (i->title));
    item->setData (0, Qt::ToolTipRole, tl::to_qstring (i->title));
    item->setData (0, Qt::DecorationRole, QIcon (":/bookmark_16px.png"));
  }

  update_navigation_panel ();
}

void
BrowserPanel::find ()
{
  mp_ui->search_frame->show ();
  mp_ui->on_page_search_edit->setFocus();
}

void
BrowserPanel::page_search_edited ()
{
  m_search_selection.clear ();
  m_search_index = -1;

  if (! mp_ui->search_frame->isVisible () || mp_ui->on_page_search_edit->text ().size () < 2) {
    mp_ui->browser->setExtraSelections (m_search_selection);
    return;
  }

  QString search_text = mp_ui->on_page_search_edit->text ();

  QTextDocument *doc = mp_ui->browser->document ();
  for (QTextBlock b = doc->firstBlock (); b.isValid (); b = b.next ()) {

    int from = 0;
    int index;

    QString t = b.text ();

    while ((index = t.indexOf (search_text, from, Qt::CaseInsensitive)) >= 0) {

      QTextCursor highlight (b);
      highlight.movePosition (QTextCursor::NextCharacter, QTextCursor::MoveAnchor, index);
      highlight.movePosition (QTextCursor::NextCharacter, QTextCursor::KeepAnchor, search_text.size ());

      QTextEdit::ExtraSelection extra_selection;
      extra_selection.cursor = highlight;
      extra_selection.format.setBackground (QColor (255, 255, 160));
      m_search_selection.push_back (extra_selection);

      from = index + search_text.size ();

    }

  }

  if (! m_search_selection.empty ()) {
    m_search_index = 0;
    mp_ui->browser->setExtraSelections (m_search_selection);
    mp_ui->browser->setTextCursor (m_search_selection [m_search_index].cursor);
  }
}

void
BrowserPanel::page_search_next ()
{
  if (m_search_index >= 0) {

    ++m_search_index;
    if (m_search_index >= m_search_selection.size ()) {
      m_search_index = 0;
    }

    mp_ui->browser->setTextCursor (m_search_selection [m_search_index].cursor);

  }
}

void
BrowserPanel::search_text_changed (const QString &text)
{
  QList<QString> strings;
  if (! text.isEmpty () && mp_source.get ()) {
    std::list<std::string> cl;
    mp_source->search_completers (tl::to_string (text.toLower ()), cl);
    for (std::list<std::string>::const_iterator i = cl.begin (); i != cl.end (); ++i) {
      strings.push_back (tl::to_qstring (*i));
    }
  }
  mp_completer_model->setStringList (strings);
}

void
BrowserPanel::source_changed ()
{
  m_new_url_dm ();
}

void
BrowserPanel::anchor_clicked (const QUrl &url)
{
  mp_ui->browser->setSource (url);
  source_changed ();
}

void
BrowserPanel::new_url ()
{
  QString title = mp_ui->browser->document ()->metaInformation (QTextDocument::DocumentTitle);
  m_current_title = title;
  emit title_changed (title);

  //  refresh on-page search
  page_search_edited ();
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
    sizes[1] += std::max (width () - 10 - size_outline, 10);
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
  if (mp_ui->search_edit->text ().size () > 0) {
    QUrl url (tl::to_qstring (m_search_url));
#if QT_VERSION >= 0x050000
    QUrlQuery qi;
    qi.addQueryItem (tl::to_qstring (m_search_query_item), mp_ui->search_edit->text ());
    url.setQuery (qi);
#else
    QList<QPair<QString, QString> > qi;
    qi.push_back (QPair<QString, QString> (tl::to_qstring (m_search_query_item), mp_ui->search_edit->text ()));
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
  mp_ui->search_edit->setVisible (! url.empty ());
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
BrowserPanel::update_navigation_panel ()
{
  bool navigation_visible = mp_ui->outline_tree->topLevelItemCount () > 0 || mp_ui->browser_bookmark_view->topLevelItemCount () > 0;
  mp_ui->navigation_frame->setVisible (navigation_visible);
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

  update_navigation_panel ();
}

QVariant 
BrowserPanel::loadResource (int type, const QUrl &url)
{
  if (type == QTextDocument::ImageResource) {

    try {
      return QVariant (mp_source->get_image (tl::to_string (url.toString ())));
    } catch (tl::Exception &ex) {
      tl::error << ex.msg ();
    } catch (...) {
    }

    return QVariant ();

  } else if (type == QTextDocument::StyleSheetResource) {

    try {
      return QVariant (tl::to_qstring (mp_source->get_css (tl::to_string (url.toString ()))));
    } catch (tl::Exception &ex) {
      tl::error << ex.msg ();
    } catch (...) {
    }

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

    try {

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

    } catch (tl::Exception &ex) {
      tl::error << ex.msg ();
    } catch (...) {
    }

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

void
BrowserSource::search_completers (const std::string & /*search_string*/, std::list<std::string> & /*completers*/)
{
  //  .. nothing here ..
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

#endif

