
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

#include <string>

#include <QTreeView>
#include <QHeaderView>
#include <QComboBox>
#include <QResizeEvent>
#include <QMenu>
#include <QApplication>
#include <QDrag>
#include <QSplitter>
#include <QFrame>
#include <QLabel>
#include <QToolButton>
#include <QCheckBox>
#include <QProxyStyle>
#include <QPainter>
#include <QPen>
#include <QVBoxLayout>

#include "dbClipboard.h"
#include "dbClipboardData.h"
#include "dbLibraryManager.h"
#include "dbLibrary.h"
#include "layBusy.h"
#include "layLibrariesView.h"
#include "layCellTreeModel.h"
#include "layLayoutViewBase.h"
#include "layAbstractMenu.h"
#include "layDialogs.h"
#include "tlExceptions.h"
#include "laybasicConfig.h"
#include "tlInternational.h"
#include "tlString.h"
#include "gtf.h"

namespace lay
{

// --------------------------------------------------------------------
//  A helper class the identifies clipboard data

class CellClipboardData
  : public db::ClipboardData
{
public:
  CellClipboardData () { }
};

// --------------------------------------------------------------------
//  LibraryTreeWidget implementation

LibraryTreeWidget::LibraryTreeWidget (QWidget *parent, const char *name, QWidget *key_event_receiver)
  : QTreeView (parent), mp_key_event_receiver (key_event_receiver)
{
  //  Allow dragging from here to
  setDragDropMode (QAbstractItemView::DragOnly);

  setObjectName (QString::fromUtf8 (name));
}


bool
LibraryTreeWidget::event (QEvent *event)
{
#if 0
  //  Handling this event makes the widget receive all keystrokes.
  //  Without this code, shortcuts override the search function.
  if (event->type () == QEvent::ShortcutOverride) {
    QKeyEvent *ke = static_cast<QKeyEvent *> (event);
    QString t = ke->text ();
    if (!t.isEmpty () && t[0].isPrint ()) {
      ke->accept ();
    }
  }
#endif
  return QTreeView::event (event);
}

bool
LibraryTreeWidget::focusNextPrevChild (bool /*next*/)
{
  return false;
}

void
LibraryTreeWidget::keyPressEvent (QKeyEvent *event)
{
  QString t = event->text ();
  if (! t.isEmpty () && t[0].isPrint ()) {
    // "/" is a search initiator
    if (t == QString::fromUtf8 ("/")) {
      t.clear ();
    }
    emit search_triggered (t);
  } else if (mp_key_event_receiver) {
    //  send other key events to the alternative receiver - this way we can make the
    //  view receive arrow keys for panning.
    QCoreApplication::sendEvent (mp_key_event_receiver, event);
  } else {
    return QTreeView::keyPressEvent (event);
  }
}

void
LibraryTreeWidget::startDrag (Qt::DropActions supportedActions)
{
  QModelIndex index = selectionModel ()->currentIndex ();
  if (index.isValid ()) {

    QModelIndexList indexes;
    indexes << index;
    QMimeData *data = model ()->mimeData (indexes);
    if (!data) {
      return;
    }

    lay::BusySection busy_section; // issue 984

    QDrag *drag = new QDrag (this);
    drag->setMimeData(data);
    QPixmap px (1, 1);
    px.fill (QColor (0, 0, 0));
    px.createMaskFromColor (QColor (0, 0, 0), Qt::MaskOutColor);
    drag->setPixmap (px);

    Qt::DropAction defaultDropAction = Qt::IgnoreAction;
    if (supportedActions & Qt::CopyAction) {
      defaultDropAction = Qt::CopyAction;
    }

    drag->exec(supportedActions, defaultDropAction);

  }
}

void
LibraryTreeWidget::mouseDoubleClickEvent (QMouseEvent *event)
{
  QModelIndex index (indexAt (event->pos ()));
  if (index.isValid ()) {
    emit cell_double_clicked (index);
  }
}

void
LibraryTreeWidget::mousePressEvent (QMouseEvent *event)
{
  if (event->button () == Qt::MiddleButton) {
    //  eat this event.
  } else {
    QModelIndex index (indexAt (event->pos ()));
    if (index.isValid ()) {
      emit cell_clicked (index);
    }
    QTreeView::mousePressEvent (event);
  }
}

void
LibraryTreeWidget::mouseReleaseEvent (QMouseEvent *event)
{
  if (event->button () == Qt::MiddleButton) {
    QModelIndex index (indexAt (event->pos ()));
    if (index.isValid ()) {
      emit cell_middle_clicked (index);
    }
  } else {
    QTreeView::mouseReleaseEvent (event);
  }
}


// --------------------------------------------------------------------
//  LibrariesView implementation

const int max_cellviews_in_split_mode = 5;

LibrariesView::LibrariesView (lay::LayoutViewBase *view, QWidget *parent, const char *name)
  : QFrame (parent),
    m_enable_cb (true),
    mp_view (view),
    m_active_index (-1),
    m_split_mode (false),
    m_do_update_content_dm (this, &LibrariesView::do_update_content),
    m_do_full_update_content_dm (this, &LibrariesView::do_full_update_content)
{
  setObjectName (QString::fromUtf8 (name));

  QVBoxLayout *ly = new QVBoxLayout (this);
  ly->setSpacing (0);
  ly->setContentsMargins (0, 0, 0, 0);

  mp_selector = new QComboBox (this);
  mp_selector->setObjectName (QString::fromUtf8 ("library_selection"));
  mp_selector->setSizePolicy (QSizePolicy::Ignored, QSizePolicy::Fixed);
  ly->addWidget (mp_selector);

  mp_search_frame = new QFrame (this);
  ly->addWidget (mp_search_frame);
  mp_search_frame->hide ();
  mp_search_frame->setAutoFillBackground (true);
  mp_search_frame->setObjectName (QString::fromUtf8 ("panel"));
  mp_search_frame->setFrameStyle (QFrame::Panel | QFrame::Raised);
  mp_search_frame->setLineWidth (1);
  mp_search_frame->setBackgroundRole (QPalette::Highlight);

  QHBoxLayout *sf_ly = new QHBoxLayout (mp_search_frame);
  sf_ly->setContentsMargins (0, 0, 0, 0);
  sf_ly->setSpacing (0);

  mp_search_close_cb = new QCheckBox (mp_search_frame);
  sf_ly->addWidget (mp_search_close_cb);

  mp_search_close_cb->setFocusPolicy (Qt::NoFocus);
  mp_search_close_cb->setBackgroundRole (QPalette::Highlight);
  mp_search_close_cb->setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Preferred));
  QPalette pl (mp_search_close_cb->palette ());
  pl.setColor (QPalette::WindowText, pl.color (QPalette::Active, QPalette::HighlightedText));
  mp_search_close_cb->setPalette (pl);
  mp_search_close_cb->setMaximumSize (QSize (mp_search_close_cb->maximumSize ().width (), mp_search_close_cb->sizeHint ().height () - 4));
  connect (mp_search_close_cb, SIGNAL (clicked ()), this, SLOT (search_editing_finished ()));

  mp_search_model = 0;
  mp_search_edit_box = new lay::DecoratedLineEdit (mp_search_frame);
  mp_search_edit_box->setObjectName (QString::fromUtf8 ("cellview_search_edit_box"));
  mp_search_edit_box->set_escape_signal_enabled (true);
  mp_search_edit_box->set_tab_signal_enabled (true);
  connect (mp_search_edit_box, SIGNAL (returnPressed ()), this, SLOT (search_editing_finished ()));
  connect (mp_search_edit_box, SIGNAL (textEdited (const QString &)), this, SLOT (search_edited ()));
  connect (mp_search_edit_box, SIGNAL (esc_pressed ()), this, SLOT (search_editing_finished ()));
  connect (mp_search_edit_box, SIGNAL (tab_pressed ()), this, SLOT (search_next ()));
  connect (mp_search_edit_box, SIGNAL (backtab_pressed ()), this, SLOT (search_prev ()));
  sf_ly->addWidget (mp_search_edit_box);

  mp_use_regular_expressions = new QAction (this);
  mp_use_regular_expressions->setCheckable (true);
  mp_use_regular_expressions->setChecked (true);
  mp_use_regular_expressions->setText (tr ("Use expressions (use * and ? for any character)"));

  mp_case_sensitive = new QAction (this);
  mp_case_sensitive->setCheckable (true);
  mp_case_sensitive->setChecked (true);
  mp_case_sensitive->setText (tr ("Case sensitive search"));

  mp_filter = new QAction (this);
  mp_filter->setCheckable (true);
  mp_filter->setChecked (false);
  mp_filter->setText (tr ("Apply as filter"));

  QMenu *m = new QMenu (mp_search_edit_box);
  m->addAction (mp_use_regular_expressions);
  m->addAction (mp_case_sensitive);
  m->addAction (mp_filter);
  connect (mp_use_regular_expressions, SIGNAL (triggered ()), this, SLOT (search_edited ()));
  connect (mp_case_sensitive, SIGNAL (triggered ()), this, SLOT (search_edited ()));
  connect (mp_filter, SIGNAL (triggered ()), this, SLOT (search_edited ()));

  mp_search_edit_box->set_clear_button_enabled (true);
  mp_search_edit_box->set_options_button_enabled (true);
  mp_search_edit_box->set_options_menu (m);

  QToolButton *sf_next = new QToolButton (mp_search_frame);
  sf_next->setAutoRaise (true);
  sf_next->setToolTip (tr ("Find next"));
  sf_next->setIcon (QIcon (QString::fromUtf8 (":/find_16px.png")));
  connect (sf_next, SIGNAL (clicked ()), this, SLOT (search_next ()));
  sf_ly->addWidget (sf_next);

  mp_splitter = new QSplitter (Qt::Vertical, this);
  ly->addWidget (mp_splitter);

  connect (mp_selector, SIGNAL (activated (int)), this, SLOT (selection_changed (int)));

  QSizePolicy sp (QSizePolicy::Minimum, QSizePolicy::Preferred);
  sp.setHorizontalStretch (0);
  sp.setVerticalStretch (0);
  setSizePolicy (sp);

  do_update_content ();
}

LibrariesView::~LibrariesView ()
{
  //  .. nothing yet ..
}

QSize
LibrariesView::sizeHint () const
{
  int w = 120; // TODO: better(?): mp_cell_list->sizeHint ().width ();
  return QSize (w, 0);
}

bool
LibrariesView::event (QEvent *e)
{
  if (e->type () == QEvent::MaxUser) {
    //  GTF probe event
    e->accept ();
    return true;
  } else {
    return QFrame::event (e);
  }
}

void
LibrariesView::context_menu (const QPoint &p)
{
  QTreeView *cell_list = dynamic_cast<QTreeView *> (sender ());
  if (cell_list) {
    QMenu *ctx_menu = mp_view->menu ()->detached_menu ("lib_context_menu");
    ctx_menu->exec (cell_list->mapToGlobal (p));
  }
}

void
LibrariesView::set_split_mode (bool f)
{
  if (f != m_split_mode) {
    m_split_mode = f;
    m_do_update_content_dm ();
  }
}

void
LibrariesView::clear_all ()
{
  m_libraries.clear ();
  m_needs_update.clear ();
  m_force_close.clear ();

  for (size_t i = 0; i < mp_cell_list_frames.size (); ++i) {
    delete mp_cell_list_frames [i];
  }
  mp_cell_list_frames.clear ();
  mp_cell_list_headers.clear ();
  mp_cell_lists.clear ();
}

void
LibrariesView::search_triggered (const QString &t)
{
  mp_search_model = 0;
  lay::LibraryTreeWidget *w = dynamic_cast<lay::LibraryTreeWidget *> (sender ());
  if (w) {
    for (size_t i = 0; i < mp_cell_lists.size (); ++i) {
      if (mp_cell_lists [i] == w) {
        //  Switch the active list for split mode -> CAUTION: this may trigger a search_editing_finished call
        select_active (int (i));
        mp_search_model = dynamic_cast<lay::CellTreeModel *> (w->model ());
        break;
      }
    }
  }

  if (mp_search_model) {
    mp_search_close_cb->setChecked (true);
    mp_search_frame->show ();
    mp_search_edit_box->setText (t);
    mp_search_edit_box->setFocus ();
    search_edited ();
  }
}

void
LibrariesView::search_edited ()
{
  QString t = mp_search_edit_box->text ();

  for (std::vector <QTreeView *>::const_iterator v = mp_cell_lists.begin (); v != mp_cell_lists.end (); ++v) {
    if ((*v)->model () == mp_search_model) {
      mp_search_model->set_filter_mode (mp_filter->isChecked ());
      if (t.isEmpty ()) {
        mp_search_model->clear_locate ();
        (*v)->setCurrentIndex (QModelIndex ());
      } else {
        QModelIndex found = mp_search_model->locate (t.toUtf8 ().constData (), mp_use_regular_expressions->isChecked (), mp_case_sensitive->isChecked (), false);
        (*v)->setCurrentIndex (found);
        if (found.isValid ()) {
          (*v)->scrollTo (found);
        }
      }
      break;
    }
  }
}

void
LibrariesView::search_next ()
{
  for (std::vector <QTreeView *>::const_iterator v = mp_cell_lists.begin (); v != mp_cell_lists.end (); ++v) {
    if ((*v)->model () == mp_search_model) {
      QModelIndex found = mp_search_model->locate_next ();
      if (found.isValid ()) {
        (*v)->setCurrentIndex (found);
        (*v)->scrollTo (found);
      }
      break;
    }
  }
}

void
LibrariesView::search_prev ()
{
  for (std::vector <QTreeView *>::const_iterator v = mp_cell_lists.begin (); v != mp_cell_lists.end (); ++v) {
    if ((*v)->model () == mp_search_model) {
      QModelIndex found = mp_search_model->locate_prev ();
      if (found.isValid ()) {
        (*v)->setCurrentIndex (found);
        (*v)->scrollTo (found);
      }
      break;
    }
  }
}

void
LibrariesView::search_editing_finished ()
{
  if (! mp_search_frame->isVisible ()) {
    return;
  }

  for (std::vector <QTreeView *>::const_iterator v = mp_cell_lists.begin (); v != mp_cell_lists.end (); ++v) {
    CellTreeModel *m = dynamic_cast<CellTreeModel *> ((*v)->model ());
    if (m) {
      m->clear_locate ();
    }
  }

  //  give back the focus to the cell list
  for (size_t i = 0; i < mp_cell_lists.size (); ++i) {
    if (mp_cell_lists [i]->model () == mp_search_model) {
      mp_cell_lists [i]->setFocus ();
      break;
    }
  }

  mp_search_frame->hide ();
  mp_search_model = 0;
}

void
LibrariesView::middle_clicked (const QModelIndex & /*index*/)
{
  //  ... nothing yet ..
}

void
LibrariesView::header_clicked ()
{
  //  ... nothing yet ..
}

void
LibrariesView::clicked (const QModelIndex & /*index*/)
{
  //  ... nothing yet ..
}

void
LibrariesView::double_clicked (const QModelIndex & /*index*/)
{
  //  ... nothing yet ..
}

void
LibrariesView::set_background_color (tl::Color c)
{
  m_background_color = c;
  for (std::vector <QTreeView *>::const_iterator f = mp_cell_lists.begin (); f != mp_cell_lists.end (); ++f) {
    QPalette pl ((*f)->palette ());
    pl.setColor (QPalette::Base, QColor (c.rgb ()));
    (*f)->setPalette (pl);
  }
}

void
LibrariesView::set_text_color (tl::Color c)
{
  m_text_color = c;
  for (std::vector <QTreeView *>::const_iterator f = mp_cell_lists.begin (); f != mp_cell_lists.end (); ++f) {
    QPalette pl ((*f)->palette ());
    pl.setColor (QPalette::Text, QColor (c.rgb ()));
    (*f)->setPalette (pl);
  }
}

void
LibrariesView::update_required ()
{
  m_do_full_update_content_dm ();
}

void
LibrariesView::do_full_update_content ()
{
  size_t i = 0;
  for (db::LibraryManager::iterator lib = db::LibraryManager::instance ().begin (); lib != db::LibraryManager::instance ().end (); ++lib, ++i) {
    if (m_needs_update.size () > i) {
      m_needs_update [i] = true;
    }
    if (m_force_close.size () > i) {
      m_force_close [i] = true;
    }
  }

  do_update_content ();
}

void
LibrariesView::do_update_content (int lib_index)
{
  //  close the search box since we will modify the model
  mp_search_frame->hide ();
  mp_search_model = 0;

  size_t imin = (lib_index < 0 ? 0 : (size_t) lib_index);
  size_t imax = (lib_index < 0 ? std::numeric_limits <size_t>::max () : (size_t) lib_index);
  std::string tech_name;

  //  rebuild all events
  detach_from_all_events ();

  mp_view->active_cellview_changed_event.add (this, &LibrariesView::update_required);
  lay::CellViewRef cv = mp_view->active_cellview_ref ();
  if (cv.is_valid ()) {
    cv->technology_changed_event.add (this, &LibrariesView::update_required);
    tech_name = cv->tech_name ();
  }

  db::LibraryManager::instance ().changed_event.add (this, &LibrariesView::update_required);

  std::vector<db::Library *> libraries;
  for (db::LibraryManager::iterator lib = db::LibraryManager::instance ().begin (); lib != db::LibraryManager::instance ().end (); ++lib) {
    db::Library *lib_ptr = db::LibraryManager::instance ().lib (lib->second);
    if (! lib_ptr->for_technologies () || lib_ptr->is_for_technology (tech_name)) {
      libraries.push_back (lib_ptr);
      libraries.back ()->layout ().hier_changed_event.add (this, &LibrariesView::update_required);
      libraries.back ()->retired_state_changed_event.add (this, &LibrariesView::update_required);
    }
  }

  for (size_t i = imin; i < libraries.size () && i <= imax; ++i) {
    if (i < m_libraries.size () && ! m_libraries[i].get ()) {
      tl_assert (i < m_force_close.size ());
      m_force_close [i] = true;
    }
    if (i >= m_force_close.size ()) {
      m_force_close.push_back (true);
    }
    if (i >= m_needs_update.size ()) {
      m_needs_update.push_back (true);
    }
    if (i >= libraries.size ()) {
      m_force_close [i] = true;
      m_needs_update [i] = true;
    }
  }

  size_t n = std::min (m_libraries.size (), libraries.size ());
  for (size_t i = imin; i < n && i <= imax; ++i) {

    if (m_libraries [i].get () != libraries [i]) {
      m_needs_update [i] = true;
      m_force_close [i] = true;
    }

    m_libraries [i].reset (libraries [i]);

  }

  if (m_libraries.size () < libraries.size ()) {
    for (size_t i = n; i < libraries.size (); ++i) {
      m_libraries.push_back (tl::weak_ptr<db::Library> (libraries [i]));
    }
  } else if (m_libraries.size () > libraries.size ()) {
    m_libraries.erase (m_libraries.begin () + libraries.size (), m_libraries.end ());
  }

  bool split_mode = m_split_mode;
  //  for more than max_cellviews_in_split_mode cellviews, switch to overlay mode
  if (int (m_libraries.size ()) > max_cellviews_in_split_mode) {
    split_mode = false;
  }

  while (mp_cell_lists.size () < m_libraries.size ()) {

    size_t i = mp_cell_lists.size ();

    QPalette pl;

    QFrame *cl_frame = new QFrame (this);
    cl_frame->setFrameShape (QFrame::NoFrame);
    QVBoxLayout *cl_ly = new QVBoxLayout (cl_frame);
    cl_ly->setSpacing (0);
    cl_ly->setContentsMargins (0, 0, 0, 0);

    QToolButton *header = new QToolButton (cl_frame);
    connect (header, SIGNAL (clicked ()), this, SLOT (header_clicked ()));
    header->setText (tl::to_qstring (display_string (int (i))));
    header->setFocusPolicy (Qt::NoFocus);
    header->setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Preferred);
    header->setCheckable (true);
    header->setAutoRaise (true);
    header->setAutoFillBackground (true);
    header->setVisible (split_mode);
    cl_ly->addWidget (header);

    LibraryTreeWidget *cell_list = new LibraryTreeWidget (cl_frame, "tree", mp_view->canvas ()->widget ());
    cl_ly->addWidget (cell_list);
    cell_list->setModel (new CellTreeModel (cell_list, m_libraries [i].get (), CellTreeModel::Flat | CellTreeModel::TopCells | CellTreeModel::BasicCells | CellTreeModel::HidePrivate | CellTreeModel::WithVariants | CellTreeModel::WithIcons, 0));
    cell_list->setUniformRowHeights (true);

    pl = cell_list->palette ();
    if (m_text_color.is_valid ()) {
      pl.setColor (QPalette::Text, QColor (m_text_color.rgb ()));
    }
    if (m_background_color.is_valid ()) {
      pl.setColor (QPalette::Base, QColor (m_background_color.rgb ()));
    }
    cell_list->setPalette (pl);

    cell_list->header ()->hide ();
    cell_list->setSelectionMode (QTreeView::ExtendedSelection);
    cell_list->setRootIsDecorated (true);
    cell_list->setContextMenuPolicy (Qt::CustomContextMenu);

    connect (cell_list, SIGNAL (customContextMenuRequested (const QPoint &)), this, SLOT (context_menu (const QPoint &)));
    connect (cell_list, SIGNAL (cell_clicked (const QModelIndex &)), this, SLOT (clicked (const QModelIndex &)));
    connect (cell_list, SIGNAL (cell_double_clicked (const QModelIndex &)), this, SLOT (double_clicked (const QModelIndex &)));
    connect (cell_list, SIGNAL (cell_middle_clicked (const QModelIndex &)), this, SLOT (middle_clicked (const QModelIndex &)));
    connect (cell_list, SIGNAL (search_triggered (const QString &)), this, SLOT (search_triggered (const QString &)));

    mp_cell_lists.push_back (cell_list);
    mp_cell_list_frames.push_back (cl_frame);
    mp_cell_list_headers.push_back (header);

    mp_splitter->addWidget (cl_frame);

  }

  while (mp_cell_lists.size () > m_libraries.size ()) {
    delete mp_cell_list_frames.back ();
    mp_cell_list_frames.pop_back ();
    mp_cell_list_headers.pop_back ();
    mp_cell_lists.pop_back ();
  }

  for (size_t i = imin; i < m_libraries.size () && i < size_t (mp_selector->count ()) && i <= imax; ++i) {
    mp_selector->setItemText (int (i), tl::to_qstring (display_string (int (i))));
  }
  while (mp_selector->count () < int (m_libraries.size ())) {
    mp_selector->addItem (tl::to_qstring (display_string (mp_selector->count ())));
  }
  while (mp_selector->count () > int (m_libraries.size ())) {
    mp_selector->removeItem (mp_selector->count () - 1);
  }

  if (m_active_index >= int (m_libraries.size ())) {
    m_active_index = int (m_libraries.size ()) - 1;
  } else if (m_active_index < 0 && ! m_libraries.empty ()) {
    m_active_index = 0;
  }
  mp_selector->setCurrentIndex (m_active_index);
  mp_selector->setVisible (mp_cell_lists.size () > 1 && ! split_mode);

  for (size_t i = imin; i < m_libraries.size () && i <= imax; ++i) {

    if (m_needs_update [i]) {

      mp_cell_list_headers [i]->setText (tl::to_qstring (display_string (int (i))));

      //  draw the cells in the level of the current cell,
      //  add an "above" entry if there is a level above.
      //  highlight the current entry. If the index is
      //  invalid, just clear the list.

      if (m_force_close [i]) {

        m_force_close [i] = false;

        CellTreeModel *model = dynamic_cast <CellTreeModel *> (mp_cell_lists [i]->model ());
        if (model) {
          model->configure (m_libraries [i].get (), CellTreeModel::Flat | CellTreeModel::TopCells | CellTreeModel::BasicCells | CellTreeModel::HidePrivate | CellTreeModel::WithVariants | CellTreeModel::WithIcons, 0);
        }

      }

      m_needs_update [i] = false;

      mp_cell_lists [i]->doItemsLayout ();   //  triggers a redraw -> the model might need this

    }

    mp_cell_list_headers [i]->setVisible (split_mode && m_libraries.size () > 1);
    mp_cell_list_headers [i]->setChecked (int (i) == m_active_index);

    mp_cell_list_frames [i]->setVisible (int (i) == m_active_index || split_mode);

  }
}

void
LibrariesView::select_active_lib_by_name (const std::string &name)
{
  for (std::vector<tl::weak_ptr<db::Library> >::const_iterator i = m_libraries.begin (); i != m_libraries.end (); ++i) {
    if (i->get () && (*i)->get_name () == name) {
      select_active (int (i - m_libraries.begin ()));
      break;
    }
  }
}

void
LibrariesView::select_active (int lib_index)
{
  if (lib_index != m_active_index) {
    mp_selector->setCurrentIndex (lib_index);
    selection_changed (lib_index);
  }
}

db::Library *
LibrariesView::active_lib ()
{
  if (m_active_index >= 0 && m_active_index < int (m_libraries.size ())) {
    return m_libraries [m_active_index].get ();
  }
  return 0;
}

void
LibrariesView::selection_changed (int index)
{
  if (index != m_active_index) {

    search_editing_finished ();

    m_active_index = index;

    bool split_mode = m_split_mode;
    //  for more than max_cellviews_in_split_mode cellviews, switch to overlay mode
    if (int (m_libraries.size ()) > max_cellviews_in_split_mode) {
      split_mode = false;
    }

    int i = 0;
    for (std::vector <QFrame *>::const_iterator f = mp_cell_list_frames.begin (); f != mp_cell_list_frames.end (); ++f, ++i) {
      (*f)->setVisible (i == index || split_mode);
      if (i == index) {
        mp_cell_lists [i]->setFocus ();
      }
    }

    i = 0;
    for (std::vector <QToolButton *>::const_iterator f = mp_cell_list_headers.begin (); f != mp_cell_list_headers.end (); ++f, ++i) {
      (*f)->setChecked (i == index);
    }

    emit active_library_changed (index);

  }
}

std::string
LibrariesView::display_string (int n) const
{
  const db::Library *lib = m_libraries [n].get ();
  std::string text = lib->get_name ();
  if (! lib->get_description ().empty ()) {
    text += " - " + lib->get_description ();
  }
  if (lib->for_technologies ()) {
    text += " ";
    std::string tn = tl::join (std::vector<std::string> (lib->get_technologies ().begin (), lib->get_technologies ().end ()), ",");
    text += tl::to_string (QObject::tr ("[Technology %1]").arg (tl::to_qstring (tn)));
  }
  return text;
}

CellTreeItem *
LibrariesView::current_item () const
{
  if (m_active_index < 0 || m_active_index >= int (mp_cell_lists.size ())) {
    return 0;
  }
  if (mp_cell_lists [m_active_index]->currentIndex ().isValid ()) {
    return (CellTreeItem *) mp_cell_lists [m_active_index]->currentIndex ().internalPointer ();
  } else {
    return 0;
  }
}

bool
LibrariesView::has_focus () const
{
  return m_active_index >= 0 && m_active_index < int (mp_cell_lists.size ()) && mp_cell_lists [m_active_index]->hasFocus ();
}

// ------------------------------------------------------------
//  Declaration of the "plugin" for the menu entries

class LibraryViewPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    std::string at;

    at = ".end";
    menu_entries.push_back (lay::submenu ("@lib_context_menu", at, std::string ()));

    at = "@lib_context_menu.end";
#if 0
    //  doesn't make sense for many libs
    menu_entries.push_back (lay::config_menu_item ("split_mode", at, tl::to_string (QObject::tr ("Split Mode")), cfg_split_lib_views, "?")),
#endif
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new LibraryViewPluginDeclaration (), -7, "LibraryViewPlugin");

} // namespace lay

#endif
