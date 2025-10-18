
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
#include "layBusy.h"
#include "layHierarchyControlPanel.h"
#include "layCellTreeModel.h"
#include "layLayoutViewBase.h"
#include "layAbstractMenu.h"
#include "layQtTools.h"
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
//  HCPCellTreeWidget implementation

HCPCellTreeWidget::HCPCellTreeWidget (QWidget *parent, const char *name, QWidget *key_event_receiver)
  : QTreeView (parent), mp_key_event_receiver (key_event_receiver)
{
  //  Allow dragging from here to 
  setDragDropMode (QAbstractItemView::DragOnly);

  setObjectName (QString::fromUtf8 (name));
}

HCPCellTreeWidget::~HCPCellTreeWidget ()
{
  //  NOTE: this should not be required, but I got a strange crash on closing the app with Qt 5.12.8
  //  after using changePersistentIndex inside the model when ~QTreeWidget tried to clean up its
  //  persistent indexes and only found a model which was deleted already.
  QAbstractItemModel *m = model ();
  if (m) {
    setModel (0);
    delete m;
  }
}

bool
HCPCellTreeWidget::event (QEvent *event)
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
HCPCellTreeWidget::focusNextPrevChild (bool /*next*/)
{
  return false;
}

void
HCPCellTreeWidget::keyPressEvent (QKeyEvent *event)
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
HCPCellTreeWidget::startDrag (Qt::DropActions supportedActions)
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
HCPCellTreeWidget::mouseDoubleClickEvent (QMouseEvent *event)
{
  QModelIndex index (indexAt (event->pos ()));
  if (index.isValid ()) {
    emit cell_double_clicked (index);
  }
}

void 
HCPCellTreeWidget::mousePressEvent (QMouseEvent *event)
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
HCPCellTreeWidget::mouseReleaseEvent (QMouseEvent *event)
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
//  HierarchyControlPanel implementation

const int max_cellviews_in_split_mode = 5;

HierarchyControlPanel::HierarchyControlPanel (lay::LayoutViewBase *view, QWidget *parent, const char *name)
  : QFrame (parent), 
    m_enable_cb (true), 
    mp_view (view),
    m_visibility_needs_update (true),
    m_active_index (0),
    m_flat (false),
    m_split_mode (false),
    m_sorting (CellTreeModel::ByName),
    m_cell_copy_mode (-1),
    m_do_update_content_dm (this, &HierarchyControlPanel::do_update_content),
    m_do_full_update_content_dm (this, &HierarchyControlPanel::do_full_update_content)
{
  setObjectName (QString::fromUtf8 (name));

  QVBoxLayout *ly = new QVBoxLayout (this);
  ly->setSpacing (0);
  ly->setContentsMargins (0, 0, 0, 0);

  mp_selector = new QComboBox (this);
  mp_selector->setObjectName (QString::fromUtf8 ("cellview_selection"));
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

  m_search_index = -1;
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

  mp_view->cellviews_changed_event.add (this, &HierarchyControlPanel::update_required);
  mp_view->hier_changed_event.add (this, &HierarchyControlPanel::update_required);

  do_update_content ();
}

HierarchyControlPanel::~HierarchyControlPanel ()
{
  //  .. nothing yet ..
}

QSize
HierarchyControlPanel::sizeHint () const
{
  int w = 120; // TODO: better(?): mp_cell_list->sizeHint ().width ();
  return QSize (w, 0);
}

bool 
HierarchyControlPanel::event (QEvent *e)
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
HierarchyControlPanel::context_menu (const QPoint &p)
{
  QTreeView *cell_list = dynamic_cast<QTreeView *> (sender ());
  if (cell_list) {
    set_active_celltree_from_sender ();
    QMenu *ctx_menu = mp_view->menu ()->detached_menu ("hcp_context_menu");
    ctx_menu->exec (cell_list->mapToGlobal (p));
  }
}

void
HierarchyControlPanel::set_sorting (CellTreeModel::Sorting sorting)
{
  if (sorting != m_sorting) {

    m_sorting = sorting;

    for (size_t i = 0; i < mp_cell_lists.size (); ++i) {
      CellTreeModel *model = dynamic_cast <CellTreeModel *> (mp_cell_lists [i]->model ());
      if (model) {
        model->set_sorting (m_sorting);
      }
    }

    m_needs_update.clear ();
    m_do_update_content_dm ();

  }
}

void
HierarchyControlPanel::set_split_mode (bool f)
{
  if (f != m_split_mode) {
    m_split_mode = f;
    m_do_update_content_dm ();
  }
}

void
HierarchyControlPanel::clear_all ()
{
  m_cellviews.clear ();
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
HierarchyControlPanel::set_cell_copy_mode (int m)
{
  m_cell_copy_mode = m;
}

void
HierarchyControlPanel::set_flat (bool f)
{
  if (f != m_flat) {

    m_flat = f;

    //  do a complete rebuild
    clear_all ();
    m_do_update_content_dm ();

  }
}

void
HierarchyControlPanel::cm_cell_select ()
{
  cell_path_type path;
  current_cell (active (), path);
  emit cell_selected (path, active ());
}

void
HierarchyControlPanel::search_triggered (const QString &t)
{
  m_search_index = -1;
  lay::HCPCellTreeWidget *w = dynamic_cast<lay::HCPCellTreeWidget *> (sender ());
  if (w) {
    for (size_t i = 0; i < mp_cell_lists.size (); ++i) {
      if (mp_cell_lists [i] == w) {
        //  Switch the active list for split mode -> CAUTION: this may trigger a search_editing_finished call
        select_active (int (i));
        m_search_index = int (i);
        break;
      }
    }
  }

  if (m_search_index >= 0) {
    mp_search_close_cb->setChecked (true);
    mp_search_frame->show ();
    mp_search_edit_box->setText (t);
    mp_search_edit_box->setFocus ();
    search_edited ();
  }
}

void
HierarchyControlPanel::search_edited ()
{
  bool filter_invalid = false;

  QString t = mp_search_edit_box->text ();

  if (m_search_index >= 0 && m_search_index < int (mp_cell_lists.size ())) {

    lay::CellTreeModel *search_model = dynamic_cast<lay::CellTreeModel *> (mp_cell_lists [m_search_index]->model ());

    search_model->set_filter_mode (mp_filter->isChecked ());

    if (t.isEmpty ()) {
      search_model->clear_locate ();
      mp_cell_lists [m_search_index]->setCurrentIndex (QModelIndex ());
    } else {
      QModelIndex found = search_model->locate (t.toUtf8 ().constData (), mp_use_regular_expressions->isChecked (), mp_case_sensitive->isChecked (), false);
      mp_cell_lists [m_search_index]->setCurrentIndex (found);
      if (found.isValid ()) {
        mp_cell_lists [m_search_index]->scrollTo (found);
      } else {
        filter_invalid = true;
      }
    }

  }

  lay::indicate_error (mp_search_edit_box, filter_invalid);
}

void
HierarchyControlPanel::search_next ()
{  
  if (m_search_index >= 0 && m_search_index < int (mp_cell_lists.size ())) {
    lay::CellTreeModel *search_model = dynamic_cast<lay::CellTreeModel *> (mp_cell_lists [m_search_index]->model ());
    QModelIndex found = search_model->locate_next (mp_cell_lists [m_search_index]->currentIndex ());
    if (found.isValid ()) {
      mp_cell_lists [m_search_index]->setCurrentIndex (found);
      mp_cell_lists [m_search_index]->scrollTo (found);
    }
  }
}

void
HierarchyControlPanel::search_prev ()
{
  if (m_search_index >= 0 && m_search_index < int (mp_cell_lists.size ())) {
    lay::CellTreeModel *search_model = dynamic_cast<lay::CellTreeModel *> (mp_cell_lists [m_search_index]->model ());
    QModelIndex found = search_model->locate_prev ();
    if (found.isValid ()) {
      mp_cell_lists [m_search_index]->setCurrentIndex (found);
      mp_cell_lists [m_search_index]->scrollTo (found);
    }
  }
}

void
HierarchyControlPanel::search_editing_finished ()
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
  if (m_search_index >= 0 && m_search_index < int (mp_cell_lists.size ())) {
    mp_cell_lists [m_search_index]->setFocus ();
  }

  mp_search_frame->hide ();
  m_search_index = -1;
}

void 
HierarchyControlPanel::middle_clicked (const QModelIndex &index)
{
  BEGIN_PROTECTED
  if (index.isValid ()) {
    set_active_celltree_from_sender ();
    cell_path_type path;
    path_from_index (index, m_active_index, path);
    emit cell_selected (path, active ());
  }
  END_PROTECTED
}

void
HierarchyControlPanel::path_from_index (const QModelIndex &index, int cv_index, cell_path_type &path) const
{
  //  build the path to the cell given by the index
  path.clear ();

  if (index.isValid ()) {

    CellTreeItem *item = (CellTreeItem *) index.internalPointer ();

    if (m_flat && cv_index >= 0 && cv_index < int (m_cellviews.size ()) && item) {

      //  construct a path in the flat case
      lay::CellView cv (m_cellviews [cv_index]);
      cv.set_cell (item->cell_or_pcell_index ());
      path = cv.unspecific_path ();

    } else {

      while (item) {
        path.push_back (item->cell_or_pcell_index ());
        item = item->parent ();
      }

      if (! path.empty ()) {
        std::reverse (path.begin (), path.end ());
      }

    }

  }
}

void
HierarchyControlPanel::set_active_celltree_from_sender ()
{
  for (int i = 0; i < int (mp_cell_lists.size ()); ++i) {
    if (mp_cell_lists [i] == sender ()) {
      select_active (i);
      return;
    }
    if (mp_cell_list_headers [i] == sender ()) {
      select_active (i);
      return;
    }
  }
}

void 
HierarchyControlPanel::header_clicked ()
{
  QToolButton *cb = dynamic_cast<QToolButton *> (sender ());
  if (cb) {
    cb->setChecked (true);
    set_active_celltree_from_sender ();
  }
}

void 
HierarchyControlPanel::clicked (const QModelIndex & /*index*/)
{
  set_active_celltree_from_sender ();
}

void 
HierarchyControlPanel::double_clicked (const QModelIndex &index)
{
  BEGIN_PROTECTED
  if (index.isValid ()) {
    set_active_celltree_from_sender ();
    mp_view->transaction (tl::to_string (QObject::tr ("Show or hide cell")));
    CellTreeItem *item = (CellTreeItem *) index.internalPointer ();
    if (mp_view->is_cell_hidden (item->cell_or_pcell_index (), m_active_index)) {
      mp_view->show_cell (item->cell_or_pcell_index (), m_active_index);
    } else {
      mp_view->hide_cell (item->cell_or_pcell_index (), m_active_index);
    }
    mp_view->commit ();
  }
  END_PROTECTED
}

void 
HierarchyControlPanel::set_current_cell (int cv_index, const cell_path_type &path)
{
  if (cv_index < 0 || cv_index >= int (mp_cell_lists.size ())) {
    return;
  }

  QModelIndex index = index_from_path (path, cv_index);
  if (index.isValid ()) {
    mp_cell_lists [cv_index]->scrollTo (index);
    mp_cell_lists [cv_index]->clearSelection ();
    mp_cell_lists [cv_index]->setCurrentIndex (index);
  }
}

void
HierarchyControlPanel::selected_cells (int cv_index, std::vector<HierarchyControlPanel::cell_path_type> &paths) const
{
  if (cv_index >= 0 && cv_index < int (mp_cell_lists.size ())) {
    QModelIndexList sel = mp_cell_lists [cv_index]->selectionModel ()->selectedIndexes ();
    for (QModelIndexList::const_iterator s = sel.begin (); s != sel.end (); ++s) {
      paths.push_back (HierarchyControlPanel::cell_path_type ());
      path_from_index (*s, cv_index, paths.back ());
    }
  }
}

void
HierarchyControlPanel::current_cell (int cv_index, HierarchyControlPanel::cell_path_type &path) const
{
  if (cv_index >= 0 && cv_index < int (mp_cell_lists.size ())) {
    path_from_index (mp_cell_lists [cv_index]->currentIndex (), cv_index, path);
  }
}

void
HierarchyControlPanel::set_background_color (tl::Color c)
{
  m_background_color = c;
  for (std::vector <QTreeView *>::const_iterator f = mp_cell_lists.begin (); f != mp_cell_lists.end (); ++f) {
    QPalette pl ((*f)->palette ());
    pl.setColor (QPalette::Base, QColor (c.rgb ()));
    (*f)->setPalette (pl);
  }
}

void
HierarchyControlPanel::set_text_color (tl::Color c)
{
  m_text_color = c;
  for (std::vector <QTreeView *>::const_iterator f = mp_cell_lists.begin (); f != mp_cell_lists.end (); ++f) {
    QPalette pl ((*f)->palette ());
    pl.setColor (QPalette::Text, QColor (c.rgb ()));
    (*f)->setPalette (pl);
  }
}

void
HierarchyControlPanel::do_full_update_content ()
{
  size_t i = 0;
  for (std::vector <lay::CellView>::const_iterator cv = m_cellviews.begin (); cv != m_cellviews.end (); ++cv, ++i) {
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
HierarchyControlPanel::update_required ()
{
  m_do_full_update_content_dm ();
}

void 
HierarchyControlPanel::select_active (int cellview_index, bool silent)
{
  if (cellview_index != m_active_index) {
    mp_selector->setCurrentIndex (cellview_index);
    change_active_cellview (cellview_index);
    if (! silent) {
      emit active_cellview_changed (cellview_index);
    }
  }
}

void
HierarchyControlPanel::change_active_cellview (int index)
{
  search_editing_finished ();

  m_active_index = index;

  bool split_mode = m_split_mode;
  //  for more than max_cellviews_in_split_mode cellviews, switch to overlay mode
  if (int (m_cellviews.size ()) > max_cellviews_in_split_mode) {
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
}

void
HierarchyControlPanel::selection_changed (int index)
{
  if (index != m_active_index) {
    change_active_cellview (index);
    emit active_cellview_changed (index);
  }
}

QModelIndex 
HierarchyControlPanel::index_from_path (const cell_path_type &path, int cv_index)
{
  if (cv_index >= 0 && cv_index < int (mp_cell_lists.size ()) && ! path.empty ()) {

    CellTreeModel *model = dynamic_cast <CellTreeModel *> (mp_cell_lists [cv_index]->model ());
    if (! model) {
      return QModelIndex ();
    }

    if (m_flat) {

      //  TODO: linear search might not be effective enough ..
      for (int c = 0; c < model->toplevel_items (); ++c) {
        CellTreeItem *item = model->toplevel_item (c);
        if (item->cell_or_pcell_index () == path.back ()) {
          return model->model_index (item);
        }
      }

    } else {

      for (int c = 0; c < model->toplevel_items (); ++c) {
        CellTreeItem *item = model->toplevel_item (c);
        if (item->cell_or_pcell_index () == path.front ()) {
          item = find_child_item (path.begin () + 1, path.end (), item);
          if (item) {
            return model->model_index (item);
          }
        }
      } 

    }

  }

  return QModelIndex ();
}

CellTreeItem *
HierarchyControlPanel::find_child_item (cell_path_type::const_iterator start, cell_path_type::const_iterator end, CellTreeItem *p)
{
  if (start == end) {
    return p;
  } else {

    for (int n = 0; n < p->children (); ++n) {
      CellTreeItem *item = p->child (n);
      if (item && item->cell_or_pcell_index () == *start) {
        return find_child_item (start + 1, end, item);
      }
    }

    //  not found
    return 0;

  }
}

std::string 
HierarchyControlPanel::display_string (int n) const
{
  return m_cellviews [n]->name () + " (@" + tl::to_string (n + 1) + ")";
}

void
HierarchyControlPanel::do_update_content (int cv_index)
{
  //  close the search box since we will modify the model
  if (m_search_index >= 0 && m_search_index < int (mp_cell_lists.size ())) {
    lay::CellTreeModel *search_model = dynamic_cast<lay::CellTreeModel *> (mp_cell_lists [m_search_index]->model ());
    search_model->clear_locate ();
  }
  mp_search_frame->hide ();
  m_search_index = -1;

  unsigned int imin = (cv_index < 0 ? 0 : (unsigned int) cv_index);
  unsigned int imax = (cv_index < 0 ? std::numeric_limits <unsigned int>::max () : (unsigned int) cv_index);

  for (unsigned int i = imin; i < mp_view->cellviews () && i <= imax; ++i) {
    if (i >= m_force_close.size ()) {
      m_force_close.push_back (true);
    }
    if (i >= m_needs_update.size ()) {
      m_needs_update.push_back (true);
    }
    if (i >= m_cellviews.size ()) {
      m_force_close [i] = true;
      m_needs_update [i] = true;
    }
  }

  unsigned int n = std::min ((unsigned int) m_cellviews.size (), mp_view->cellviews ());
  for (unsigned int i = imin; i < n && i <= imax; ++i) {

    if (&m_cellviews [i]->layout () != &mp_view->cellview (i)->layout ()) {
      m_needs_update [i] = true;
      m_force_close [i] = true;
    } else if (! m_cellviews [i].is_valid ()) {
      m_needs_update [i] = true;
    } else if (m_cellviews [i].combined_unspecific_path () != mp_view->cellview (i).combined_unspecific_path ()) {
      m_needs_update [i] = true;
    }

    if (m_needs_update [i]) {
      mp_cell_lists [i]->doItemsLayout (); //  this schedules a redraw 
    }

    m_cellviews [i] = mp_view->cellview (i);

  }

  if (m_cellviews.size () < mp_view->cellviews ()) {
    for (unsigned int i = n; i < mp_view->cellviews (); ++i) {
      m_cellviews.push_back (mp_view->cellview (i));
    }
  } else if (m_cellviews.size () > mp_view->cellviews ()) {
    m_cellviews.erase (m_cellviews.begin () + mp_view->cellviews (), m_cellviews.end ()); 
  }

  bool split_mode = m_split_mode;
  //  for more than max_cellviews_in_split_mode cellviews, switch to overlay mode
  if (int (m_cellviews.size ()) > max_cellviews_in_split_mode) {
    split_mode = false;
  }

  while (mp_cell_lists.size () < m_cellviews.size ()) {

    QPalette pl;

    int cv_index = int (mp_cell_lists.size ());

    QFrame *cl_frame = new QFrame (this);
    cl_frame->setFrameShape (QFrame::NoFrame);
    QVBoxLayout *cl_ly = new QVBoxLayout (cl_frame);
    cl_ly->setSpacing (0);
    cl_ly->setContentsMargins (0, 0, 0, 0);

    QToolButton *header = new QToolButton (cl_frame);
    connect (header, SIGNAL (clicked ()), this, SLOT (header_clicked ()));
    header->setText (tl::to_qstring (display_string (cv_index)));
    header->setFocusPolicy (Qt::NoFocus);
    header->setSizePolicy (QSizePolicy::Preferred, QSizePolicy::Preferred);
    header->setCheckable (true);
    header->setAutoRaise (true);
    header->setAutoFillBackground (true);
    header->setVisible (split_mode);
    cl_ly->addWidget (header);

    HCPCellTreeWidget *cell_list = new HCPCellTreeWidget (cl_frame, "tree", mp_view->canvas ()->widget ());
    cl_ly->addWidget (cell_list);
    cell_list->setModel (new CellTreeModel (cell_list, mp_view, cv_index, m_flat ? CellTreeModel::Flat : 0, 0, m_sorting));
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
    cell_list->setIndentation (14);
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

  while (mp_cell_lists.size () > m_cellviews.size ()) {
    delete mp_cell_list_frames.back ();
    mp_cell_list_frames.pop_back ();
    mp_cell_list_headers.pop_back ();
    mp_cell_lists.pop_back ();
  } 

  for (unsigned int i = imin; i < m_cellviews.size () && i < (unsigned int) mp_selector->count () && i <= imax; ++i) {
    mp_selector->setItemText (i, tl::to_qstring (display_string (i))); 
  }
  while (mp_selector->count () < int (m_cellviews.size ())) {
    mp_selector->addItem (tl::to_qstring (display_string (mp_selector->count ())));
  }
  while (mp_selector->count () > int (m_cellviews.size ())) {
    mp_selector->removeItem (mp_selector->count () - 1);
  } 

  if (m_active_index >= int (m_cellviews.size ())) {
    m_active_index = int (m_cellviews.size ()) - 1;
  } else if (m_active_index < 0 && ! m_cellviews.empty ()) {
    m_active_index = 0;
  }
  mp_selector->setCurrentIndex (m_active_index);
  mp_selector->setVisible (mp_cell_lists.size () > 1 && ! split_mode);

  for (unsigned int i = imin; i < m_cellviews.size () && i <= imax; ++i) {
    
    if (m_needs_update [i]) {

      mp_cell_list_headers [i]->setText (tl::to_qstring (display_string (i)));

      //  draw the cells in the level of the current cell,
      //  add an "above" entry if there is a level above.
      //  highlight the current entry. If the index is 
      //  invalid, just clear the list.

      if (m_force_close [i]) {

        m_force_close [i] = false;

        CellTreeModel *model = dynamic_cast <CellTreeModel *> (mp_cell_lists [i]->model ());
        if (model) {
          model->configure (mp_view, i, m_flat ? CellTreeModel::Flat : 0, 0, m_sorting);
        }

      }

      m_needs_update [i] = false;

    }

    mp_cell_list_headers [i]->setVisible (split_mode && m_cellviews.size () > 1);
    mp_cell_list_headers [i]->setChecked (int (i) == m_active_index);

    mp_cell_list_frames [i]->setVisible (int (i) == m_active_index || split_mode);

  }
}

CellTreeItem *
HierarchyControlPanel::current_item () const
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
HierarchyControlPanel::has_focus () const
{
  return m_active_index >= 0 && m_active_index < int (mp_cell_lists.size ()) && mp_cell_lists [m_active_index]->hasFocus ();
}

bool
HierarchyControlPanel::ask_for_cell_copy_mode (const db::Layout &layout, const std::vector<cell_path_type> &paths, int &cell_copy_mode)
{
  bool needs_to_ask = false;
  cell_copy_mode = 0;

  if (m_cell_copy_mode < 0) {  //  ask

    //  check if there is a cell that we have to ask for
    for (std::vector<cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
      if (! p->empty ()) {
        const db::Cell &cell = layout.cell (p->back ());
        if (! cell.is_proxy () && ! cell.is_leaf ()) {
          needs_to_ask = true;
        }
      }
    }

  } else {
    cell_copy_mode = m_cell_copy_mode;
  }

  if (needs_to_ask) {

    bool dont_ask_again = false;

    lay::CopyCellModeDialog mode_dialog (this);
    if (! mode_dialog.exec_dialog (cell_copy_mode, dont_ask_again)) {
      return false;
    }

    if (dont_ask_again) {
      view ()->dispatcher ()->config_set (cfg_copy_cell_mode, tl::to_string (cell_copy_mode));
      view ()->dispatcher ()->config_end ();
    }

  }

  return true;
}

void
HierarchyControlPanel::cut () 
{
  if (m_active_index < 0 || m_active_index >= int (mp_cell_lists.size ())) {
    return;
  }

  std::vector<cell_path_type> paths;
  selected_cells (m_active_index, paths);

  if (paths.empty ()) {
    return;
  }

  //  first copy

  db::Layout &layout = m_cellviews [m_active_index]->layout ();
  if (! layout.is_editable ()) {
    return;
  }

  db::Clipboard::instance ().clear ();

  int cut_mode = 1; // 0: shallow, 1: deep
  if (! ask_for_cell_copy_mode (layout, paths, cut_mode)) {
    return;
  }

  //  collect the called cells of the cells to copy, so we don't copy a cell twice
  std::set<db::cell_index_type> called_cells;
  for (std::vector<cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
    if (! p->empty ()) {
      const db::Cell &cell = layout.cell (p->back ());
      cell.collect_called_cells (called_cells);
    }
  }

  for (std::vector<cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
    if (! p->empty () && called_cells.find (p->back ()) == called_cells.end ()) {
      db::ClipboardValue<lay::CellClipboardData> *cd = new db::ClipboardValue<lay::CellClipboardData> ();
      cd->get ().add (layout, layout.cell (p->back ()), cut_mode == 1 ? 1 /*with subcells*/ : 2 /*first level, then without subcells*/);
      db::Clipboard::instance () += cd;
    }
  }

  //  then do a (shallow or deep) delete

  //  remember the current path
  cell_path_type cell_path (mp_view->cellview (m_active_index).combined_unspecific_path ());

  mp_view->clear_selection ();

  std::set<db::cell_index_type> cells_to_delete;
  for (std::vector<cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
    if (! p->empty () && layout.is_valid_cell_index (p->back ())) {
      cells_to_delete.insert (p->back ());
    }
  }

  mp_view->transaction (tl::to_string (QObject::tr ("Cut Cells")));
  if (cut_mode == 1) {
    layout.prune_cells (cells_to_delete);
  } else {
    layout.delete_cells (cells_to_delete);
  }
  layout.cleanup ();
  mp_view->commit ();

  //  If one of the cells in the path was deleted, establish a valid path

  bool needs_update = false;
  for (size_t i = cell_path.size (); i > 0; ) {
    --i;
    if (! layout.is_valid_cell_index (cell_path [i])) {
      cell_path.erase (cell_path.begin () + i, cell_path.end ());
      needs_update = true;
    }
  }

  if (needs_update) {
    mp_view->select_cell (cell_path, m_active_index);
  }
}

bool
HierarchyControlPanel::has_selection ()
{
  return (current_item () != 0);
}

void
HierarchyControlPanel::copy () 
{
  if (m_active_index < 0 || m_active_index >= int (mp_cell_lists.size ())) {
    return;
  }

  std::vector<cell_path_type> paths;
  selected_cells (m_active_index, paths);

  if (paths.empty ()) {
    return;
  }

  db::Layout &layout = m_cellviews [m_active_index]->layout ();

  db::Clipboard::instance ().clear ();

  int copy_mode = 1; // 0: shallow, 1: deep
  if (! ask_for_cell_copy_mode (layout, paths, copy_mode)) {
    return;
  }

  //  collect the called cells of the cells to copy, so we don't copy a cell twice
  std::set<db::cell_index_type> called_cells;
  for (std::vector<cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
    if (! p->empty ()) {
      const db::Cell &cell = layout.cell (p->back ());
      cell.collect_called_cells (called_cells);
    }
  }

  //  actually copy
  for (std::vector<cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
    if (! p->empty () && called_cells.find (p->back ()) == called_cells.end ()) {
      db::ClipboardValue<lay::CellClipboardData> *cd = new db::ClipboardValue<lay::CellClipboardData> ();
      cd->get ().add (layout, layout.cell (p->back ()), copy_mode == 1 ? 1 /*with subcells*/ : 2 /*first level, then without subcells*/);
      db::Clipboard::instance () += cd;
    }
  }
}

void
HierarchyControlPanel::paste () 
{
  if (m_active_index < 0 || m_active_index >= int (mp_cell_lists.size ())) {
    return;
  }

  db::Layout &layout = m_cellviews [m_active_index]->layout ();
  if (! layout.is_editable ()) {
    return;
  }

  std::vector<unsigned int> new_layers;

  //  paste the content into the active cellview.
  std::vector <db::cell_index_type> new_tops;
  for (db::Clipboard::iterator c = db::Clipboard::instance ().begin (); c != db::Clipboard::instance ().end (); ++c) {
    const db::ClipboardValue<lay::CellClipboardData> *value = dynamic_cast<const db::ClipboardValue<lay::CellClipboardData> *> (*c);
    if (value) {
      std::vector<unsigned int> nl = value->get ().insert (layout, 0, &new_tops);
      new_layers.insert (new_layers.end (), nl.begin (), nl.end ());
    }
  }

  //  Add new layers to the view if required.
  if (! new_layers.empty ()) {
    mp_view->add_new_layers (new_layers, m_active_index);
    mp_view->update_content ();
  }

  //  select the first new top cell
  if (! new_tops.empty ()) {
    //  TODO: this does not work properly: since we are inside a transaction, bboxes are not updated
    //  correctly. Thus, the cell_fit does not work properly.
    mp_view->select_cell_fit (new_tops [0], m_active_index);
  }
}

// ------------------------------------------------------------
//  Declaration of the "plugin" for the menu entries

class HierarchyControlPanelPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    std::string at;

    at = ".end";
    menu_entries.push_back (lay::submenu ("@hcp_context_menu", at, std::string ()));

    at = "@hcp_context_menu.end";

    menu_entries.push_back (lay::config_menu_item ("flat_mode", at, tl::to_string (QObject::tr ("Flat Cell List")), cfg_flat_cell_list, "?")),
    menu_entries.push_back (lay::config_menu_item ("split_mode", at, tl::to_string (QObject::tr ("Split Mode")), cfg_split_cell_list, "?")),
    menu_entries.push_back (lay::submenu ("sorting", at, tl::to_string (QObject::tr ("Sorting"))));

    {
      std::string at = "@hcp_context_menu.sorting.end";
      menu_entries.push_back (lay::config_menu_item ("by_name", at, tl::to_string (QObject::tr ("By Name")), cfg_cell_list_sorting, "?by-name"));
      menu_entries.push_back (lay::config_menu_item ("by_area", at, tl::to_string (QObject::tr ("By Area - Small To Large")), cfg_cell_list_sorting, "?by-area"));
      menu_entries.push_back (lay::config_menu_item ("by_area_reverse", at, tl::to_string (QObject::tr ("By Area - Large To Small")), cfg_cell_list_sorting, "?by-area-reverse"));
    }

    menu_entries.push_back (lay::separator ("operations_group", at));
    menu_entries.push_back (lay::menu_item ("cm_new_cell", "new_cell:edit:edit_mode", at, tl::to_string (QObject::tr ("New Cell"))));
    menu_entries.push_back (lay::menu_item ("cm_cell_delete", "delete_cell:edit:edit_mode", at, tl::to_string (QObject::tr ("Delete Cell"))));
    menu_entries.push_back (lay::menu_item ("cm_cell_rename", "rename_cell:edit:edit_mode", at, tl::to_string (QObject::tr ("Rename Cell"))));
    menu_entries.push_back (lay::menu_item ("cm_cell_replace", "replace_cell:edit:edit_mode", at, tl::to_string (QObject::tr ("Replace Cell"))));
    menu_entries.push_back (lay::menu_item ("cm_cell_flatten", "flatten_cell:edit:edit_mode", at, tl::to_string (QObject::tr ("Flatten Cell"))));
    menu_entries.push_back (lay::menu_item ("cm_cell_user_properties", "cell_user_properties", at, tl::to_string (QObject::tr ("User Properties"))));
    menu_entries.push_back (lay::separator ("clipboard_group:edit_mode", at));
    menu_entries.push_back (lay::menu_item ("cm_cell_copy", "copy:edit_mode", at, tl::to_string (QObject::tr ("Copy"))));
    menu_entries.push_back (lay::menu_item ("cm_cell_cut", "cut:edit_mode", at, tl::to_string (QObject::tr ("Cut"))));
    menu_entries.push_back (lay::menu_item ("cm_cell_paste", "paste:edit_mode", at, tl::to_string (QObject::tr ("Paste"))));
    menu_entries.push_back (lay::separator ("select_group", at));
    menu_entries.push_back (lay::menu_item ("cm_cell_select", "show_as_top", at, tl::to_string (QObject::tr ("Show As New Top"))));
    menu_entries.push_back (lay::separator ("visibility_group", at));
    menu_entries.push_back (lay::menu_item ("cm_cell_hide", "hide_cell", at, tl::to_string (QObject::tr ("Hide"))));
    menu_entries.push_back (lay::menu_item ("cm_cell_show", "show_cell", at, tl::to_string (QObject::tr ("Show"))));
    menu_entries.push_back (lay::menu_item ("cm_cell_show_all", "show_all", at, tl::to_string (QObject::tr ("Show All"))));
    menu_entries.push_back (lay::separator ("utils_group", at));
    menu_entries.push_back (lay::menu_item ("cm_open_current_cell", "open_current", at, tl::to_string (QObject::tr ("Where am I?"))));
    menu_entries.push_back (lay::separator ("file_group", at));
    menu_entries.push_back (lay::menu_item ("cm_save_current_cell_as", "save_cell_as:hide_vo", at, tl::to_string (QObject::tr ("Save Selected Cells As"))));
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new HierarchyControlPanelPluginDeclaration (), -8, "HierarchyControlPanelPlugin");

} // namespace lay 

#endif
