
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

#include <stdint.h>

#include <string>
#include <algorithm>

#include "dbClipboard.h"
#include "layLayerControlPanel.h"
#include "layLayerTreeModel.h"
#include "layViewOp.h"
#include "laybasicConfig.h"
#include "layDialogs.h"
#include "layLayoutCanvas.h"
#include "layAbstractMenu.h"
#include "layQtTools.h"
#include "tlExceptions.h"
#include "tlInternational.h"
#include "tlAssert.h"
#include "gtf.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QInputDialog>
#include <QColorDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QBitmap>
#include <QPixmap>
#include <QHeaderView>
#include <QMenu>
#include <QToolButton>
#include <QMessageBox>
#include <QItemDelegate>
#include <QApplication>
#include <QKeyEvent>
#include <QHoverEvent>

namespace lay
{

// --------------------------------------------------------------------
//  LCPTreeWidget declaration & implementation

LCPTreeWidget::LCPTreeWidget (QWidget *parent, lay::LayerTreeModel *model, const char *name)
  : QTreeView (parent), mp_model (model)
{
  setObjectName (QString::fromUtf8 (name));
  setModel (model);
  setUniformRowHeights (true);
#if QT_VERSION >= 0x040200
  setAllColumnsShowFocus (true);
#endif
}

LCPTreeWidget::~LCPTreeWidget ()
{
  // .. nothing yet ..
}

QSize 
LCPTreeWidget::sizeHint () const
{
  return QSize (0, 0);
}

void 
LCPTreeWidget::set_selection (const std::vector<lay::LayerPropertiesConstIterator> &sel) 
{
  clearSelection ();
  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator s = sel.begin (); s != sel.end (); ++s) {
    selectionModel ()->select (mp_model->index (*s, 0), QItemSelectionModel::Select | QItemSelectionModel::Rows);
  }
  if (sel.empty ()) {
    selectionModel ()->setCurrentIndex (QModelIndex (), QItemSelectionModel::Current | QItemSelectionModel::Rows);
  } else {
    selectionModel ()->setCurrentIndex (mp_model->index (sel.front (), 1), QItemSelectionModel::Current | QItemSelectionModel::Rows);
  }
}

void 
LCPTreeWidget::set_current (const lay::LayerPropertiesConstIterator &sel) 
{
  selectionModel ()->select (mp_model->index (sel, 0), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  selectionModel ()->setCurrentIndex (mp_model->index (sel, 1), QItemSelectionModel::Current | QItemSelectionModel::Rows);
}

void 
LCPTreeWidget::mouseDoubleClickEvent (QMouseEvent *event)
{
  QModelIndex index (indexAt (event->pos ()));
  if (index.isValid ()) {
    emit double_clicked (index, event->modifiers ());
  }
}

bool
LCPTreeWidget::focusNextPrevChild (bool /*next*/)
{
  return false;
}

bool
LCPTreeWidget::event (QEvent *event)
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

void
LCPTreeWidget::keyPressEvent (QKeyEvent *event)
{
  QString t = event->text ();
  if (!t.isEmpty () && t[0].isPrint ()) {
    // "/" is a search initiator
    if (t == QString::fromUtf8 ("/")) {
      t.clear ();
    }
    emit search_triggered (t);
  } else {
    QTreeView::keyPressEvent (event);
  }
}

void
LCPTreeWidget::collapse_all ()
{
#if QT_VERSION >= 0x040200
  collapseAll ();
#else
  //  Qt <4.1 does not have an "collapseAll". So we emulate it.
  std::vector<QModelIndex> indices;
  QModelIndex index (mp_model->upperLeft ());
  while (index.isValid ()) {
    if (isExpanded (index)) {
      indices.push_back (index);
    }
    index = indexBelow (index);
  }
  for (std::vector<QModelIndex>::const_iterator i = indices.begin (); i != indices.end (); ++i) {
    collapse (*i);
  }
#endif
}

void
LCPTreeWidget::expand_all ()
{
#if QT_VERSION >= 0x040200
  expandAll ();
#else
  //  Qt <4.1 does not have an "expandAll". So we emulate it.
  std::vector<QModelIndex> indices;
  QModelIndex index (mp_model->upperLeft ());
  while (index.isValid ()) {
    if (! isExpanded (index)) {
      indices.push_back (index);
    }
    index = indexBelow (index);
  }
  for (std::vector<QModelIndex>::const_iterator i = indices.begin (); i != indices.end (); ++i) {
    expand (*i);
  }
#endif
}

// --------------------------------------------------------------------
//  LayerControlPanel implementation

LayerControlPanel::LayerControlPanel (lay::LayoutViewBase *view, db::Manager *manager, QWidget *parent, const char *name)
  : QFrame (parent), 
    db::Object (manager),
    mp_view (view), 
    m_needs_update (true), 
    m_expanded_state_needs_update (false),
    m_tabs_need_update (true), 
    m_hidden_flags_need_update (true),
    m_in_update (false),
    m_current_layer (0),
    m_phase (0), 
    m_do_update_content_dm (this, &LayerControlPanel::do_update_content),
    m_do_update_visibility_dm (this, &LayerControlPanel::do_update_visibility),
    m_no_stipples (false),
    m_layer_visibility_follows_selection (false)
{
  setObjectName (QString::fromUtf8 (name));

  QSizePolicy sp (QSizePolicy::Minimum, QSizePolicy::Preferred);
  sp.setHorizontalStretch (0);
  sp.setVerticalStretch (0);
  setSizePolicy (sp);

  QVBoxLayout *l = new QVBoxLayout (this);
  l->setContentsMargins (0, 0, 0, 0);
  l->setSpacing (0);

  mp_search_frame = new QFrame (this);
  l->addWidget (mp_search_frame);
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

  mp_tab_bar = new QTabBar (this);
  mp_tab_bar->setObjectName (QString::fromUtf8 ("lcp_tabs"));
  connect (mp_tab_bar, SIGNAL (currentChanged (int)), this, SLOT (tab_selected (int)));
  l->addWidget (mp_tab_bar);
  mp_tab_bar->setContextMenuPolicy (Qt::CustomContextMenu);
  connect (mp_tab_bar, SIGNAL(customContextMenuRequested (const QPoint &)), this, SLOT (tab_context_menu (const QPoint &)));

  mp_model = new lay::LayerTreeModel (this, view);
  mp_layer_list = new LCPTreeWidget (this, mp_model, "layer_tree");
  mp_layer_list->setUniformRowHeights (true);
  mp_layer_list->setIconSize (mp_model->icon_size ());
  mp_model->set_font_no_signal (mp_layer_list->font ());

  l->addWidget (mp_layer_list);
  connect (mp_layer_list, SIGNAL (double_clicked (const QModelIndex &, Qt::KeyboardModifiers)), this, SLOT (double_clicked (const QModelIndex &, Qt::KeyboardModifiers)));
  connect (mp_layer_list, SIGNAL (collapsed (const QModelIndex &)), this, SLOT (group_collapsed (const QModelIndex &)));
  connect (mp_layer_list, SIGNAL (expanded (const QModelIndex &)), this, SLOT (group_expanded (const QModelIndex &)));
  connect (mp_layer_list, SIGNAL (search_triggered (const QString &)), this, SLOT (search_triggered (const QString &)));
  connect (mp_layer_list->selectionModel (), SIGNAL (currentChanged (const QModelIndex &, const QModelIndex &)), this, SLOT (current_index_changed (const QModelIndex &)));
  connect (mp_layer_list->selectionModel (), SIGNAL (selectionChanged (const QItemSelection &, const QItemSelection &)), this, SLOT (selection_changed (const QItemSelection &, const QItemSelection &)));
  mp_layer_list->setContextMenuPolicy (Qt::CustomContextMenu);
  connect (mp_layer_list, SIGNAL(customContextMenuRequested (const QPoint &)), this, SLOT (context_menu (const QPoint &)));
  mp_layer_list->header ()->hide ();
  mp_layer_list->setSelectionMode (QTreeView::ExtendedSelection);
  mp_layer_list->setRootIsDecorated (false);
  //  Custom resize mode makes the columns as narrow as possible
#if QT_VERSION >= 0x050000
  mp_layer_list->header ()->setSectionResizeMode (QHeaderView::ResizeToContents);
#elif QT_VERSION >= 0x040200
  mp_layer_list->header ()->setResizeMode (QHeaderView::ResizeToContents);
#else
  mp_layer_list->header ()->setResizeMode (QHeaderView::Custom);
#endif

  mp_view->layer_list_changed_event.add (this, &LayerControlPanel::update_required);
  mp_view->layer_list_inserted_event.add (this, &LayerControlPanel::signal_ll_changed);
  mp_view->layer_list_deleted_event.add (this, &LayerControlPanel::signal_ll_changed);
  mp_view->current_layer_list_changed_event.add (this, &LayerControlPanel::signal_li_changed);
  mp_view->geom_changed_event.add (this, &LayerControlPanel::signal_cv_changed);
  mp_view->cellview_changed_event.add (this, &LayerControlPanel::signal_cv_changed_with_int);
  mp_view->viewport_changed_event.add (this, &LayerControlPanel::signal_vp_changed);
  mp_view->hier_levels_changed_event.add (this, &LayerControlPanel::signal_vp_changed);
  mp_view->resolution_changed_event.add (this, &LayerControlPanel::signal_resolution_changed);

  QFrame *tb = new QFrame (this);
  l->addWidget (tb);

  QHBoxLayout *ltb = new QHBoxLayout (tb);
  ltb->setContentsMargins (0, 0, 0, 0);
  ltb->setSpacing (0);

  tb->setObjectName (QString::fromUtf8 ("lcp_buttons"));

  QToolButton *b;

  b = new QToolButton (tb);
  b->setObjectName (QString::fromUtf8 ("lcp_dd"));
  ltb->addWidget (b);
  b->setIcon (QIcon (QString::fromUtf8 (":downdown_16px.png")));
  connect (b, SIGNAL (clicked ()), this, SLOT (downdown_clicked ()));

  b = new QToolButton (tb);
  b->setObjectName (QString::fromUtf8 ("lcp_d"));
  ltb->addWidget (b);
  b->setIcon (QIcon (QString::fromUtf8 (":down_16px.png")));
  connect (b, SIGNAL (clicked ()), this, SLOT (down_clicked ()));

  b = new QToolButton (tb);
  b->setObjectName (QString::fromUtf8 ("lcp_u"));
  ltb->addWidget (b);
  b->setIcon (QIcon (QString::fromUtf8 (":up_16px.png")));
  connect (b, SIGNAL (clicked ()), this, SLOT (up_clicked ()));

  b = new QToolButton (tb);
  b->setObjectName (QString::fromUtf8 ("lcp_uu"));
  ltb->addWidget (b);
  b->setIcon (QIcon (QString::fromUtf8 (":upup_16px.png")));
  connect (b, SIGNAL (clicked ()), this, SLOT (upup_clicked ()));

  ltb->addStretch (0);

  m_no_stipples_label = new QLabel (tb);
  m_no_stipples_label->hide ();
  m_no_stipples_label->setPixmap (QPixmap (QString::fromUtf8 (":/warn_16px@2x.png")));
  m_no_stipples_label->setToolTip (tr ("Stipples are disabled - unselect \"View/Show Layers Without Fill\" to re-enable them"));
  ltb->addWidget (m_no_stipples_label);

  connect (mp_model, SIGNAL (hidden_flags_need_update ()), this, SLOT (update_hidden_flags ()));
}

LayerControlPanel::~LayerControlPanel ()
{
  //  .. nothing yet ..
}

void
LayerControlPanel::recover ()
{
  cancel_updates ();
  if (manager ()) {
    manager ()->clear ();
  }
}

void 
LayerControlPanel::cm_delete ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("Delete layer views")));
  do_delete ();
  commit ();

  END_PROTECTED_CLEANUP { 
    recover (); 
    commit ();
  }
}

struct LayerSelectionClearOp
  : public db::Op
{
  LayerSelectionClearOp ()
    : db::Op ()
  { }
};

void
LayerControlPanel::do_delete ()
{
  std::vector<lay::LayerPropertiesConstIterator> sel = selected_layers ();
  if (! sel.empty ()) {

    begin_updates ();

    std::sort (sel.begin (), sel.end (), CompareLayerIteratorBottomUp ());
    for (std::vector<lay::LayerPropertiesConstIterator>::iterator s = sel.begin (); s != sel.end (); ++s) {
      mp_view->delete_layer (*s);
    }

    if (transacting ()) {
      manager ()->queue (this, new LayerSelectionClearOp ());
    }

    end_updates ();

    emit order_changed ();
      
  }
}

void 
LayerControlPanel::cm_remove_unused ()
{
  BEGIN_PROTECTED_CLEANUP
  begin_updates ();
  transaction (tl::to_string (QObject::tr ("Clean up views")));
  mp_view->remove_unused_layers ();
  commit ();
  end_updates ();

  END_PROTECTED_CLEANUP { recover (); }
}

void 
LayerControlPanel::cm_add_missing ()
{
  BEGIN_PROTECTED_CLEANUP

  begin_updates ();
  transaction (tl::to_string (QObject::tr ("Add other views")));
  mp_view->add_missing_layers ();
  commit ();
  end_updates ();

  END_PROTECTED_CLEANUP { recover (); }
}

void 
LayerControlPanel::cm_insert ()
{
  lay::LayerPropertiesConstIterator sel = current_layer ();
  if (sel.is_null ()) {
    sel = mp_view->end_layers ();
  }

  lay::LayerProperties props;

  std::string n = props.source_string (false);

  LayerSourceDialog dialog (this);
  dialog.setWindowTitle (QObject::tr ("Insert New Layer Entry - Specify Source"));
  if (dialog.exec_dialog (n)) {

    BEGIN_PROTECTED_CLEANUP

    transaction (tl::to_string (QObject::tr ("Insert layer view")));

    props.set_source (tl::to_string (n));
    mp_view->init_layer_properties (props);

    const LayerPropertiesNode &lp = mp_view->insert_layer (sel, props);

    set_current_layer (sel);

    commit ();

    emit order_changed ();

    //  Show a warning if the layer was not present yet.
    //  HINT: this must be the last action in this method since it will trigger the event loop which will
    //  dispatch further actions.
    if (mp_view->is_editable () && lp.layer_index () < 0 && lp.cellview_index () >= 0 && lp.source (true).special_purpose () == ParsedLayerSource::SP_None) {
      QMessageBox::warning (0, QObject::tr ("Layer does not exist"), 
                               QObject::tr ("The layer specified does not exist. To create that layer, use 'New/Layer' from the 'Edit' menu"));
    }

    END_PROTECTED_CLEANUP { recover (); }

  }
}

void 
LayerControlPanel::cm_group ()
{
  BEGIN_PROTECTED_CLEANUP

  std::vector<lay::LayerPropertiesConstIterator> sel = selected_layers ();
  if (! sel.empty ()) {

    begin_updates ();

    transaction (tl::to_string (QObject::tr ("Group layer views")));

    lay::LayerPropertiesNode node;
    for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator s = sel.begin (); s != sel.end (); ++s) {
      node.add_child (**s);
    }

    //  this establishes a true bottom-up order
    std::sort (sel.begin (), sel.end (), CompareLayerIteratorBottomUp ());

    //  The delete_layer method invalidates the iterator and tries to set it to the
    //  next available object. However, for the insert position, we still need the
    //  original location. Therefore we have to save the insert position:
    lay::LayerPropertiesConstIterator ins_pos = *sel.rbegin ();

    //  delete the original objects.
    for (std::vector<lay::LayerPropertiesConstIterator>::iterator s = sel.begin (); s != sel.end (); ++s) {
      mp_view->delete_layer (*s);
    }

    mp_view->insert_layer (ins_pos, node);

    set_current_layer (*sel.rbegin ());

    commit ();

    emit order_changed ();

  }

  END_PROTECTED_CLEANUP { recover (); }
}

void 
LayerControlPanel::cm_ungroup ()
{
  BEGIN_PROTECTED_CLEANUP

  lay::LayerPropertiesConstIterator sel = current_layer ();
  if (! sel.is_null () && sel->has_children ()) {

    begin_updates ();

    transaction (tl::to_string (QObject::tr ("Ungroup layer views")));

    lay::LayerPropertiesNode node = *sel;

    //  The delete_layer method invalidates the iterator and tries to set it to the
    //  next available object. However, for the insert position, we still need the
    //  original location. Therefore we have to save the insert position:
    lay::LayerPropertiesConstIterator ins_pos = sel;
    mp_view->delete_layer (sel);

    for (lay::LayerPropertiesNode::const_iterator c = node.end_children (); c != node.begin_children (); ) {
      --c;
      mp_view->insert_layer (ins_pos, c->flat ());
    }

    if (transacting ()) {
      manager ()->queue (this, new LayerSelectionClearOp ());
    }
    set_selection (std::vector<lay::LayerPropertiesConstIterator> ()); // clear selection

    commit ();

    end_updates ();

    emit order_changed ();

  }

  END_PROTECTED_CLEANUP { recover (); }
}

void 
LayerControlPanel::cut ()
{
  BEGIN_PROTECTED_CLEANUP
  do_copy ();
  do_delete ();
  END_PROTECTED_CLEANUP { recover (); }
}

bool 
LayerControlPanel::has_focus () const
{
  return mp_layer_list->hasFocus ();
}

bool
LayerControlPanel::has_selection () const
{
  return ! mp_layer_list->selectionModel ()->selectedIndexes ().isEmpty ();
}

void 
LayerControlPanel::copy ()
{
  BEGIN_PROTECTED_CLEANUP
  do_copy ();
  END_PROTECTED_CLEANUP { recover (); }
}

static void
collect_dpi (const lay::LayerPropertiesNode &node, std::set <unsigned int> &dpi) 
{
  if (node.dither_pattern (false) >= 0) {
    dpi.insert (node.dither_pattern (false));
  }
  for (lay::LayerPropertiesNode::const_iterator c = node.begin_children (); c != node.end_children (); ++c) {
    collect_dpi (*c, dpi);
  }
}

static void
update_dpi (lay::LayerPropertiesNode &node, const std::map <unsigned int, unsigned int> &dpi_map)
{
  std::map <unsigned int, unsigned int>::const_iterator new_dpi = dpi_map.find ((unsigned int) node.dither_pattern (false));
  if (new_dpi != dpi_map.end ()) {
    node.set_dither_pattern (new_dpi->second);
  }
  for (lay::LayerPropertiesNode::iterator c = node.begin_children (); c != node.end_children (); ++c) {
    update_dpi (*c, dpi_map);
  }
}

void
LayerControlPanel::do_copy ()
{
  std::vector<lay::LayerPropertiesConstIterator> sel = selected_layers ();

  db::Clipboard::instance ().clear ();
  //  determine the custom dither pattern if required
  std::set <unsigned int> dp_to_save;
  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator l = sel.begin (); l != sel.end (); ++l) {
    collect_dpi (**l, dp_to_save);
  }
  for (std::set <unsigned int>::const_iterator dp = dp_to_save.begin (); dp != dp_to_save.end (); ++dp) {
    if (*dp >= (unsigned int) std::distance (mp_view->dither_pattern ().begin (), mp_view->dither_pattern ().begin_custom ())) {
      lay::DitherPatternInfo dpi (mp_view->dither_pattern ().begin () [*dp]);
      //  use order index to save the pattern's index
      dpi.set_order_index (*dp);
      db::Clipboard::instance () += new db::ClipboardValue<lay::DitherPatternInfo> (dpi);
    }
  }
  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator l = sel.begin (); l != sel.end (); ++l) {
    db::Clipboard::instance () += new db::ClipboardValue<lay::LayerPropertiesNode> (**l);
  }
}

void 
LayerControlPanel::paste ()
{
  try {

    lay::LayerPropertiesConstIterator pos = current_layer ();
    if (pos.is_null ()) {
      pos = mp_view->end_layers ();
    }

    std::vector<lay::LayerPropertiesConstIterator> new_sel;

    begin_updates ();

    //  restore custom dither pattern, if required
 
    lay::DitherPattern dither_pattern (mp_view->dither_pattern ());
    std::map <unsigned int, unsigned int> dpi_map;

    bool needs_update = false;

    for (db::Clipboard::iterator obj = db::Clipboard::instance ().begin (); obj != db::Clipboard::instance ().end (); ++obj) {

      const db::ClipboardValue<lay::DitherPatternInfo> *dp_obj = dynamic_cast<const db::ClipboardValue<lay::DitherPatternInfo> *> (*obj);
      if (dp_obj) {

        //  try to locate the corresponding pattern or insert as a new one if required
        int found_dpi = -1;
        for (lay::DitherPattern::iterator dp = dither_pattern.begin_custom (); dp != dither_pattern.end () && found_dpi < 0; ++dp) {
          if (dp->same_bitmap (dp_obj->get ())) {
            found_dpi = std::distance (dither_pattern.begin (), dp);
          }
        }
        if (found_dpi < 0) {
          found_dpi = dither_pattern.add_pattern (dp_obj->get ());
          needs_update = true;
        }
        dpi_map.insert (std::make_pair (dp_obj->get ().order_index (), (unsigned int) found_dpi));

      }

    }

    if (needs_update) {
      mp_view->set_dither_pattern (dither_pattern);
    }

    for (db::Clipboard::iterator obj = db::Clipboard::instance ().begin (); obj != db::Clipboard::instance ().end (); ++obj) {
      const db::ClipboardValue<lay::LayerPropertiesNode> *lp_obj = dynamic_cast<const db::ClipboardValue<lay::LayerPropertiesNode> *> (*obj);
      if (lp_obj) {
        lay::LayerPropertiesNode node (lp_obj->get ());
        update_dpi (node, dpi_map);
        mp_view->insert_layer (pos, node);
        new_sel.push_back (pos);
        pos.next_sibling ();
      }
    }

    if (transacting ()) {
      manager ()->queue (this, new LayerSelectionClearOp ());
    }

    end_updates ();

    set_selection (new_sel);

    emit order_changed ();

  } catch (...) {
    recover ();
    throw;
  }
}

void 
LayerControlPanel::cm_source ()
{
  lay::LayerPropertiesConstIterator sel = current_layer ();
  if (! sel.is_null ()) {

    lay::LayerProperties props = *sel;

    std::string n = props.source_string (false);

    LayerSourceDialog dialog (this);
    dialog.setWindowTitle (QObject::tr ("Edit Source Specification"));
    if (dialog.exec_dialog (n)) {

      BEGIN_PROTECTED_CLEANUP

      props.set_source (tl::to_string (n));

      transaction (tl::to_string (QObject::tr ("Select source")));
      mp_view->set_properties (sel, props);
      commit ();

      END_PROTECTED_CLEANUP { recover (); }

    }

  }
}

void 
LayerControlPanel::cm_rename ()
{
  lay::LayerPropertiesConstIterator sel = current_layer ();
  if (! sel.is_null ()) {

    lay::LayerProperties props = *sel;

    bool ok = false;
    QString n = QInputDialog::getText (this,
                                       QObject::tr ("Rename layer"),
                                       QObject::tr ("Enter new name of layer"),
                                       QLineEdit::Normal, 
                                       tl::to_qstring (props.name ()),
                                       &ok);
   
    if (ok) {

      BEGIN_PROTECTED_CLEANUP

      props.set_name (tl::to_string (n));

      transaction (tl::to_string (QObject::tr ("Rename layer")));
      mp_view->set_properties (sel, props);
      commit ();

      END_PROTECTED_CLEANUP { recover (); }

    }

  }
}

void 
LayerControlPanel::cm_show_only ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("Show selected layers")));

  std::vector<lay::LayerPropertiesConstIterator> sel = mp_view->selected_layers ();
  std::set<lay::LayerPropertiesConstIterator> sel_set (sel.begin (), sel.end ());
  std::set<lay::LayerPropertiesConstIterator> org_sel_set (sel_set);

  //  show all nodes
  for (lay::LayerPropertiesConstIterator l = mp_view->begin_layers (); ! l.at_end (); ++l) {
    lay::LayerProperties props (*l);
    props.set_visible (true);
    mp_view->set_properties (l, props);
  }

  // make all parents of selected nodes selected as well 
  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator s = sel.begin (); s != sel.end (); ++s) {
    lay::LayerPropertiesConstIterator ll = *s;
    while (! ll.is_null ()) {
      sel_set.insert (ll);
      ll = ll.parent ();
    }
  }

  // make all children of originally selected nodes selected as well 
  for (lay::LayerPropertiesConstIterator l = mp_view->begin_layers (); ! l.at_end (); ++l) {
    lay::LayerPropertiesConstIterator ll = l;
    while (! ll.is_null ()) {
      if (org_sel_set.find (ll) != org_sel_set.end ()) {
        sel_set.insert (l);
        break;
      }
      ll = ll.parent ();
    }
  }

  //  now hide all non-selected nodes which don't have a parent or are children of a selected node
  for (lay::LayerPropertiesConstIterator l = mp_view->begin_layers (); ! l.at_end (); ++l) {
    if (sel_set.find (l) == sel_set.end () && (l.parent ().is_null () || sel_set.find (l.parent ()) != sel_set.end ())) {
      lay::LayerProperties props (*l);
      props.set_visible (false);
      mp_view->set_properties (l, props);
    }
  }

  commit ();

  END_PROTECTED_CLEANUP { recover (); }
}

void 
LayerControlPanel::cm_show ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("Show layer")));

  std::vector<lay::LayerPropertiesConstIterator> sel = mp_view->selected_layers ();

  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator l = sel.begin (); l != sel.end (); ++l) {
    lay::LayerProperties props (**l);
    props.set_visible (true);
    mp_view->set_properties (*l, props);
  }

  commit ();

  END_PROTECTED_CLEANUP { recover (); }
}

void
LayerControlPanel::cm_toggle_visibility ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("Toggle visibility")));

  std::vector<lay::LayerPropertiesConstIterator> sel = mp_view->selected_layers ();

  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator l = sel.begin (); l != sel.end (); ++l) {
    lay::LayerProperties props (**l);
    props.set_visible (! props.visible (false));
    mp_view->set_properties (*l, props);
  }

  commit ();

  END_PROTECTED_CLEANUP { recover (); }
}

void
LayerControlPanel::cm_show_all ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("Show all layers")));

  for (lay::LayerPropertiesConstIterator l = mp_view->begin_layers (); ! l.at_end (); ++l) {
    lay::LayerProperties props (*l);
    props.set_visible (true);
    mp_view->set_properties (l, props);
  }

  commit ();

  END_PROTECTED_CLEANUP { recover (); }
}

void 
LayerControlPanel::cm_rename_tab ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("Rename layer tab")));

  bool ok = false;
  QString n = QInputDialog::getText (this, 
                                     QObject::tr ("Rename Layer Tab"),
                                     QObject::tr ("New layer tab name"),
                                     QLineEdit::Normal, 
                                     tl::to_qstring (mp_view->get_properties (mp_view->current_layer_list ()).name ()),
                                     &ok);
 
  if (ok) {
    begin_updates ();
    mp_view->rename_properties (mp_view->current_layer_list (), tl::to_string (n));
    end_updates ();
  }

  commit ();

  END_PROTECTED_CLEANUP { recover (); }
}

void 
LayerControlPanel::cm_remove_tab ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("Remove layer tab")));

  if (mp_view->layer_lists () == 1) {
    throw tl::Exception (tl::to_string (QObject::tr ("Cannot remove last layer tab")));
  }

  begin_updates ();
  mp_view->delete_layer_list (mp_view->current_layer_list ());
  end_updates ();

  commit ();

  emit order_changed ();

  END_PROTECTED_CLEANUP { recover (); }
}

void 
LayerControlPanel::cm_new_tab ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("New layer tab")));

  begin_updates ();
  mp_view->insert_layer_list (mp_view->current_layer_list () + 1, mp_view->get_properties ());
  end_updates ();

  commit ();

  emit order_changed ();

  END_PROTECTED_CLEANUP { recover (); }
}

void 
LayerControlPanel::cm_make_valid ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("Make layer valid")));

  std::vector<lay::LayerPropertiesConstIterator> sel = mp_view->selected_layers ();

  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator l = sel.begin (); l != sel.end (); ++l) {
    lay::LayerProperties props (**l);
    props.set_valid (true);
    mp_view->set_properties (*l, props);
  }

  commit ();

  END_PROTECTED_CLEANUP { recover (); }
}

void 
LayerControlPanel::cm_make_invalid ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("Make layer invalid")));

  std::vector<lay::LayerPropertiesConstIterator> sel = mp_view->selected_layers ();

  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator l = sel.begin (); l != sel.end (); ++l) {
    lay::LayerProperties props (**l);
    props.set_valid (false);
    mp_view->set_properties (*l, props);
  }

  commit ();

  END_PROTECTED_CLEANUP { recover (); }
}

void 
LayerControlPanel::cm_hide ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("Hide layer")));

  std::vector<lay::LayerPropertiesConstIterator> sel = mp_view->selected_layers ();

  for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator l = sel.begin (); l != sel.end (); ++l) {
    lay::LayerProperties props (**l);
    props.set_visible (false);
    mp_view->set_properties (*l, props);
  }

  commit ();

  END_PROTECTED_CLEANUP { recover (); }
}

void 
LayerControlPanel::cm_hide_all ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("Hide all layers")));

  for (lay::LayerPropertiesConstIterator l = mp_view->begin_layers (); ! l.at_end (); ++l) {
    if (l.parent ().is_null ()) {
      //  only hide the top-level entries - this way, nothing will be visible, but the child nodes
      //  maintain their state
      lay::LayerProperties props (*l);
      props.set_visible (false);
      mp_view->set_properties (l, props);
    }
  }

  commit ();

  END_PROTECTED_CLEANUP { recover (); }
}

void 
LayerControlPanel::cm_select_all ()
{
  BEGIN_PROTECTED_CLEANUP
  
  mp_layer_list->selectAll ();

  END_PROTECTED_CLEANUP { recover (); }
}

void
LayerControlPanel::cm_invert_selection ()
{
  BEGIN_PROTECTED_CLEANUP

  std::vector<lay::LayerPropertiesConstIterator> sel = mp_view->selected_layers ();

  std::set<size_t> ids;
  for (auto s = sel.begin (); s != sel.end (); ++s) {
    ids.insert (s->uint ());
  }

  std::vector<lay::LayerPropertiesConstIterator> new_sel;

  for (lay::LayerPropertiesConstIterator l = mp_view->begin_layers (); ! l.at_end (); ) {
    if (ids.find (l.uint ()) == ids.end ()) {
      new_sel.push_back (l);
      ++l;
    } else if (l->has_children ()) {
      while (! l.at_end ()) {
        l.next_sibling ();
        if (l.at_end () && ! l.at_top ()) {
          l.up ();
        } else {
          break;
        }
      }
    } else {
      ++l;
    }
  }

  mp_layer_list->set_selection (new_sel);

  END_PROTECTED_CLEANUP { recover (); }
}

void
LayerControlPanel::set_selection (const std::vector<lay::LayerPropertiesConstIterator> &new_sel)
{
  //  If the tree has changed we need to delay the selection update until the model has been updated.
  if (m_in_update) {

    //  store only the uint's of the selected items to become independent from the list reference
    m_new_sel.clear ();
    for (std::vector<lay::LayerPropertiesConstIterator>::const_iterator s = new_sel.begin (); s != new_sel.end (); ++s) {
      m_new_sel.push_back (s->uint ());
    }

  } else {

    mp_layer_list->set_selection (new_sel);

    //  :TODO: save selection for undo? Currently we just clear it.
    if (transacting ()) {
      manager ()->queue (this, new LayerSelectionClearOp ());
    }

  }
}

void 
LayerControlPanel::clear_selection ()
{
  std::vector<lay::LayerPropertiesConstIterator> empty_sel;
  set_selection (empty_sel);
}

void
LayerControlPanel::search_triggered (const QString &t)
{
  if (mp_model) {
    mp_search_close_cb->setChecked (true);
    mp_search_frame->show ();
    mp_search_edit_box->setText (t);
    mp_search_edit_box->setFocus ();
    search_edited ();
  }
}

void
LayerControlPanel::search_edited ()
{
  if (! mp_model) {
    return;
  }

  mp_model->set_filter_mode (mp_filter->isChecked ());

  bool filter_invalid = false;

  QString t = mp_search_edit_box->text ();
  if (t.isEmpty ()) {
    mp_model->clear_locate ();
    mp_layer_list->setCurrentIndex (QModelIndex ());
  } else {
    QModelIndex found = mp_model->locate (t.toUtf8 ().constData (), mp_use_regular_expressions->isChecked (), mp_case_sensitive->isChecked (), false);
    mp_layer_list->setCurrentIndex (found);
    if (found.isValid ()) {
      mp_layer_list->scrollTo (found);
    } else {
      filter_invalid = true;
    }
  }

  lay::indicate_error (mp_search_edit_box, filter_invalid);
}

void
LayerControlPanel::search_next ()
{
  if (! mp_model) {
    return;
  }

  QModelIndex found = mp_model->locate_next ();
  if (found.isValid ()) {
    mp_layer_list->setCurrentIndex (found);
    mp_layer_list->scrollTo (found);
  }
}

void
LayerControlPanel::search_prev ()
{
  if (! mp_model) {
    return;
  }

  QModelIndex found = mp_model->locate_prev ();
  if (found.isValid ()) {
    mp_layer_list->setCurrentIndex (found);
    mp_layer_list->scrollTo (found);
  }
}

void
LayerControlPanel::search_editing_finished ()
{
  if (! mp_model) {
    return;
  }

  mp_model->clear_locate ();
  mp_search_frame->hide ();
}

void
LayerControlPanel::cm_regroup_flatten ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("Flatten layers")));
  regroup_layers (RegroupFlatten);
  commit ();

  emit order_changed ();

  END_PROTECTED_CLEANUP { recover (); }
}

void
LayerControlPanel::cm_regroup_by_index ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("Regroup layers")));
  regroup_layers (RegroupByIndex);
  commit ();

  emit order_changed ();

  END_PROTECTED_CLEANUP { recover (); }
}

void
LayerControlPanel::cm_regroup_by_datatype ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("Regroup layers")));
  regroup_layers (RegroupByDatatype);
  commit ();

  emit order_changed ();

  END_PROTECTED_CLEANUP { recover (); }
}

void
LayerControlPanel::cm_regroup_by_layer ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("Regroup layers")));
  regroup_layers (RegroupByLayer);
  commit ();

  emit order_changed ();

  END_PROTECTED_CLEANUP { recover (); }
}

void
LayerControlPanel::cm_sort_by_name ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("Sort layers")));
  sort_layers (ByName);
  commit ();

  emit order_changed ();

  END_PROTECTED_CLEANUP { recover (); }
}

void
LayerControlPanel::cm_sort_by_ild ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("Sort layers")));
  sort_layers (ByIndexLayerDatatype);
  commit ();

  emit order_changed ();

  END_PROTECTED_CLEANUP { recover (); }
}

void
LayerControlPanel::cm_sort_by_idl ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("Sort layers")));
  sort_layers (ByIndexDatatypeLayer);
  commit ();

  emit order_changed ();

  END_PROTECTED_CLEANUP { recover (); }
}

void
LayerControlPanel::cm_sort_by_ldi ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("Sort layers")));
  sort_layers (ByLayerDatatypeIndex);
  commit ();

  emit order_changed ();

  END_PROTECTED_CLEANUP { recover (); }
}

void
LayerControlPanel::cm_sort_by_dli ()
{
  BEGIN_PROTECTED_CLEANUP

  transaction (tl::to_string (QObject::tr ("Sort layers")));
  sort_layers (ByDatatypeLayerIndex);
  commit ();

  emit order_changed ();

  END_PROTECTED_CLEANUP { recover (); }
}

struct LDSortingProps
  : std::vector <int>
{
  LDSortingProps (int l1, int l2, int l3)
  {
    reserve (3);
    push_back (l1);
    push_back (l2);
    push_back (l3);
  }
};

struct LayerSorter 
{
  LayerSorter (const lay::LayoutViewBase *view, lay::LayerControlPanel::SortOrder order)
    : m_order (order), mp_view (view)
  {
    //  .. nothing yet ..
  }

  bool operator() (const lay::LayerPropertiesNode &a, const lay::LayerPropertiesNode &b) const
  {
    if (m_order == lay::LayerControlPanel::ByName) {

      return a.display_string (mp_view, false) < b.display_string (mp_view, false);

    } else if (m_order == lay::LayerControlPanel::ByIndexLayerDatatype) {

      if (a.source (false).cv_index () != b.source (false).cv_index ()) {
        return (a.source (false).cv_index () < b.source (false).cv_index ());
      }
      if (a.source (false).layer () != b.source (false).layer ()) {
        return (a.source (false).layer () < b.source (false).layer ());
      }
      if (a.source (false).datatype () != b.source (false).datatype ()) {
        return (a.source (false).datatype () < b.source (false).datatype ());
      }
      if (a.source (false).name () != b.source (false).name ()) {
        return (a.source (false).name () < b.source (false).name ());
      }
      return false;

    } else if (m_order == lay::LayerControlPanel::ByIndexDatatypeLayer) {

      if (a.source (false).cv_index () != b.source (false).cv_index ()) {
        return (a.source (false).cv_index () < b.source (false).cv_index ());
      }
      if (a.source (false).datatype () != b.source (false).datatype ()) {
        return (a.source (false).datatype () < b.source (false).datatype ());
      }
      if (a.source (false).layer () != b.source (false).layer ()) {
        return (a.source (false).layer () < b.source (false).layer ());
      }
      if (a.source (false).name () != b.source (false).name ()) {
        return (a.source (false).name () < b.source (false).name ());
      }
      return false;

    } else if (m_order == lay::LayerControlPanel::ByLayerDatatypeIndex) {

      if (a.source (false).layer () != b.source (false).layer ()) {
        return (a.source (false).layer () < b.source (false).layer ());
      }
      if (a.source (false).datatype () != b.source (false).datatype ()) {
        return (a.source (false).datatype () < b.source (false).datatype ());
      }
      if (a.source (false).cv_index () != b.source (false).cv_index ()) {
        return (a.source (false).cv_index () < b.source (false).cv_index ());
      }
      if (a.source (false).name () != b.source (false).name ()) {
        return (a.source (false).name () < b.source (false).name ());
      }
      return false;

    } else if (m_order == lay::LayerControlPanel::ByDatatypeLayerIndex) {

      if (a.source (false).datatype () != b.source (false).datatype ()) {
        return (a.source (false).datatype () < b.source (false).datatype ());
      }
      if (a.source (false).layer () != b.source (false).layer ()) {
        return (a.source (false).layer () < b.source (false).layer ());
      }
      if (a.source (false).cv_index () != b.source (false).cv_index ()) {
        return (a.source (false).cv_index () < b.source (false).cv_index ());
      }
      if (a.source (false).name () != b.source (false).name ()) {
        return (a.source (false).name () < b.source (false).name ());
      }
      return false;

    } else {
      return false;
    }
  }

private:
  lay::LayerControlPanel::SortOrder m_order;
  const lay::LayoutViewBase *mp_view;
};

void
LayerControlPanel::sort_layers (SortOrder order)
{
  std::vector<lay::LayerPropertiesNode> new_props;

  lay::LayerPropertiesConstIterator p = current_layer ();
  if (! p.is_null ()) {
    p = p.parent ();
  }

  if (p.is_null ()) {
    new_props.assign (mp_view->get_properties ().begin_const (), mp_view->get_properties ().end_const ());
  } else {
    new_props.assign (p->begin_children (), p->end_children ());
  }

  std::sort (new_props.begin (), new_props.end (), LayerSorter (mp_view, order));

  lay::LayerPropertiesList prop_list;
  prop_list.set_dither_pattern (mp_view->get_properties ().dither_pattern ());
  prop_list.set_name (mp_view->get_properties ().name ());

  if (p.is_null ()) {

    for (std::vector<lay::LayerPropertiesNode>::const_iterator np = new_props.begin (); np != new_props.end (); ++np) {
      //  :KLUDGE: the list should have an insert with a begin..end iterator pair ..
      prop_list.push_back (*np);
    }

  } else {

    prop_list = mp_view->get_properties ();

    lay::LayerPropertiesIterator pp (prop_list, p.uint ());
    pp->clear_children ();
    for (std::vector<lay::LayerPropertiesNode>::const_iterator np = new_props.begin (); np != new_props.end (); ++np) {
      //  :KLUDGE: the list should have an insert with a begin..end iterator pair ..
      pp->add_child (*np);
    }

  }

  mp_view->set_properties (prop_list);
}

struct LayerRegroupSorter 
{
  LayerRegroupSorter (lay::LayerControlPanel::RegroupMode mode)
    : m_mode (mode)
  {
    //  .. nothing yet ..
  }

  bool operator() (const lay::LayerPropertiesNode &a, const lay::LayerPropertiesNode &b) const
  {
    if (m_mode == lay::LayerControlPanel::RegroupByIndex) {
      return (a.source (false).cv_index () < b.source (false).cv_index ());
    } else if (m_mode == lay::LayerControlPanel::RegroupByDatatype) {
      return (a.source (false).datatype () < b.source (false).datatype ());
    } else if (m_mode == lay::LayerControlPanel::RegroupByLayer) {
      return (a.source (false).layer () < b.source (false).layer ());
    } else {
      return false;
    }
  }

private:
  lay::LayerControlPanel::RegroupMode m_mode;
};

void
LayerControlPanel::regroup_layers (RegroupMode mode)
{
  std::vector<lay::LayerProperties> linear_props;
  LayerPropertiesConstIterator l = mp_view->begin_layers ();
  while (! l.at_end ()) {
    if (! l->has_children ()) {
      linear_props.push_back (l->flat ()); 
    }
    ++l;
  }

  LayerRegroupSorter sorter (mode);
  std::stable_sort (linear_props.begin (), linear_props.end (), sorter);

  lay::LayerPropertiesList prop_list;
  prop_list.set_dither_pattern (mp_view->get_properties ().dither_pattern ());
  prop_list.set_name (mp_view->get_properties ().name ());

  std::vector<lay::LayerProperties>::const_iterator i = linear_props.begin ();
  while (i != linear_props.end ()) {

    std::vector<lay::LayerProperties>::const_iterator f = i;
    do {
      ++f;
    } while (f != linear_props.end () && !sorter (*i, *f));

    //  make a new node from [i..f)
    if (mode == lay::LayerControlPanel::RegroupByIndex) {

      prop_list.push_back (lay::LayerPropertiesNode ());
      lay::ParsedLayerSource source;
      source.cv_index (i->source (true /*real*/).cv_index ());
      prop_list.back ().set_source (source);

    } else if (mode == lay::LayerControlPanel::RegroupByDatatype) {

      prop_list.push_back (lay::LayerPropertiesNode ());
      lay::ParsedLayerSource source;
      source.datatype (i->source (true /*real*/).datatype ());
      prop_list.back ().set_source (source);

    } else if (mode == lay::LayerControlPanel::RegroupByLayer) {

      prop_list.push_back (lay::LayerPropertiesNode ());
      lay::ParsedLayerSource source;
      source.layer (i->source (true /*real*/).layer ());
      prop_list.back ().set_source (source);

    } 

    for (std::vector<lay::LayerProperties>::const_iterator p = i; p != f; ++p) {

      lay::LayerProperties pp (*p);
      lay::ParsedLayerSource source (pp.source (true /*real*/));

      if (mode == lay::LayerControlPanel::RegroupByIndex) {
        source.cv_index (-1);
      } else if (mode == lay::LayerControlPanel::RegroupByDatatype) {
        source.datatype (-1);
      } else if (mode == lay::LayerControlPanel::RegroupByLayer) {
        source.layer (-1);
      }

      pp.set_source (source);
      if (mode != lay::LayerControlPanel::RegroupFlatten) {
        prop_list.back ().add_child (pp);
      } else {
        prop_list.push_back (pp);
      }

    }

    i = f;
  
  }

  mp_view->set_properties (prop_list);
}

void
LayerControlPanel::cm_expand_all ()
{
  mp_layer_list->expand_all ();
}

void
LayerControlPanel::tab_context_menu (const QPoint &p)
{
  QMenu *ctx_menu = mp_view->menu ()->detached_menu ("lcp_tabs_context_menu");
  if (ctx_menu) {
    ctx_menu->exec (mp_tab_bar->mapToGlobal (p));
  }
}

void
LayerControlPanel::context_menu (const QPoint &p)
{
  QMenu *ctx_menu = mp_view->menu ()->detached_menu ("lcp_context_menu");
  if (ctx_menu) {
    ctx_menu->exec (mp_layer_list->mapToGlobal (p));
  }
}

void 
LayerControlPanel::double_clicked (const QModelIndex &index, Qt::KeyboardModifiers modifiers)
{
  BEGIN_PROTECTED_CLEANUP

  if (! index.isValid ()) {
    return;
  }

  if ((modifiers & Qt::ShiftModifier) != 0) {
    cm_show_only ();
  } else {

    lay::LayerPropertiesConstIterator item (mp_model->iterator (index));
    if (item.is_null () || item.at_end ()) {
      return;
    }

    lay::LayerProperties props = *item;
    props.set_visible (! props.visible (false));

    if (props.visible (false)) {
      transaction (tl::to_string (QObject::tr ("Show layer")));
    } else {
      transaction (tl::to_string (QObject::tr ("Hide layer")));
    }

    mp_view->set_properties (item, props);

    commit ();

  }

  END_PROTECTED_CLEANUP { recover (); }
}

void
LayerControlPanel::set_no_stipples (bool ns)
{
  if (m_no_stipples != ns) {
    m_no_stipples = ns;
    m_no_stipples_label->setVisible (ns);
    m_do_update_content_dm ();
  }
}

void
LayerControlPanel::set_background_color (tl::Color c)
{
  QPalette pl (mp_layer_list->palette ());
  pl.setColor (QPalette::Base, QColor (c.rgb ()));
  mp_layer_list->setPalette (pl);
  mp_model->set_background_color (QColor (c.rgb ()));
}

void
LayerControlPanel::set_text_color (tl::Color c)
{
  QPalette pl (mp_layer_list->palette ());
  pl.setColor (QPalette::Text, QColor (c.rgb ()));
  mp_layer_list->setPalette (pl);
  mp_model->set_text_color (QColor (c.rgb ()));
}

void
LayerControlPanel::update_hidden_flags ()
{
  m_hidden_flags_need_update = true;
  m_do_update_content_dm ();
}

void
LayerControlPanel::set_layer_visibility_follows_selection (bool f)
{
  if (f != m_layer_visibility_follows_selection) {
    m_layer_visibility_follows_selection = f;
    m_do_update_visibility_dm ();
  }
}

bool
LayerControlPanel::layer_visibility_follows_selection ()
{
  return m_layer_visibility_follows_selection;
}

void
LayerControlPanel::set_hide_empty_layers (bool f)
{
  mp_model->set_hide_empty_layers (f);
}

bool
LayerControlPanel::hide_empty_layers ()
{
  return mp_model->get_hide_empty_layers ();
}

void
LayerControlPanel::set_test_shapes_in_view (bool f)
{
  mp_model->set_test_shapes_in_view (f);
}

void
LayerControlPanel::begin_updates ()
{
  if (! m_in_update) {

    m_in_update = true;
    m_hidden_flags_need_update = true;

    mp_model->signal_begin_layer_changed ();  //  this makes the view redraw the data

    //  we force a clear_selection in this case, since we cannot make sure the
    //  selecting remains valid
    clear_selection ();
    m_current_layer = 0;

  }
}

bool 
LayerControlPanel::model_updated ()
{
  return ! m_in_update;
}

void
LayerControlPanel::tab_selected (int index)
{
  if (index >= 0 && (unsigned int) index < mp_view->layer_lists ()) {
    mp_view->set_current_layer_list ((unsigned int) index);
    emit tab_changed ();
  }
}

void
LayerControlPanel::cancel_updates ()
{
  m_in_update = false;
  m_needs_update = false;
  m_expanded_state_needs_update = false;
  m_hidden_flags_need_update = false;
  m_tabs_need_update = false;
}

void
LayerControlPanel::end_updates ()
{
  m_do_update_content_dm ();
}

void 
LayerControlPanel::set_phase (int phase)
{
  if (m_phase != phase) {
    m_phase = phase;
    m_do_update_content_dm ();
  }
}

static void 
set_hidden_flags_rec (LayerTreeModel *model, QTreeView *tree_view, const QModelIndex &parent)
{
  int rows = model->rowCount (parent);
  for (int r = 0; r < rows; ++r) {

    QModelIndex index = model->index (r, 0, parent);

    if (! model->hasChildren (index)) {

      if (model->is_hidden (index)) {
        tree_view->setRowHidden (r, parent, true);
      } else {
        tree_view->setRowHidden (r, parent, false);
      }

    } else {

      set_hidden_flags_rec (model, tree_view, index);

      //  hide a group entry if all children are hidden

      bool hide = true;
      int rrows = model->rowCount (index);
      for (int rr = 0; rr < rrows; ++rr) {
        if (! tree_view->isRowHidden (rr, index)) {
          hide = false;
        }
      }

      tree_view->setRowHidden (r, parent, hide);

    }

  }
}

void
LayerControlPanel::do_update_hidden_flags ()
{
  set_hidden_flags_rec (mp_model, mp_layer_list, QModelIndex ());

  //  scroll the current index into view if it was not visible before
  QModelIndex current = mp_layer_list->currentIndex ();
  if (current.isValid ()) {
    QModelIndex parent = mp_layer_list->model ()->parent (current);
    if (! mp_layer_list->isRowHidden (current.row (), parent)) {
      QRect visual_rect = mp_layer_list->visualRect (current);
      if (! visual_rect.intersects (mp_layer_list->viewport ()->rect ())) {
        mp_layer_list->scrollTo (current, QAbstractItemView::PositionAtCenter);
      }
    }
  }
}

void
LayerControlPanel::do_update_visibility ()
{
  if (! m_layer_visibility_follows_selection) {
    return;
  }

  std::set<size_t> sel_uints;

  QModelIndexList selected = mp_layer_list->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = selected.begin (); i != selected.end (); ++i) {
    if (i->column () == 0) {
      sel_uints.insert (mp_model->iterator (*i).uint ());
    }
  }

  for (lay::LayerPropertiesConstIterator l = mp_view->begin_layers (); ! l.at_end (); ++l) {
    lay::LayerProperties props (*l);
    props.set_visible (sel_uints.find (l.uint ()) != sel_uints.end () || l->has_children ());
    mp_view->set_properties (l, props);
  }
}

void
LayerControlPanel::do_update_content ()
{
  mp_model->set_phase (m_phase);

  if (m_tabs_need_update) {

    //  temporarily disconnect tab selected signals because those get issued on insertTab
    disconnect (mp_tab_bar, SIGNAL (currentChanged (int)), this, SLOT (tab_selected (int)));

    if (mp_view->layer_lists () <= 1) {

      mp_tab_bar->hide ();

    } else {

      mp_tab_bar->show ();
      while (mp_tab_bar->count () > int (mp_view->layer_lists ())) {
        mp_tab_bar->removeTab (mp_tab_bar->count () - 1);
      }
      while (mp_tab_bar->count () < int (mp_view->layer_lists ())) {
        mp_tab_bar->insertTab (mp_tab_bar->count (), QString ());
      }

      for (unsigned int ll = 0; ll < mp_view->layer_lists (); ++ll) {
        if (mp_view->get_properties (ll).name ().empty ()) {
          mp_tab_bar->setTabText (int (ll), tl::to_qstring (tl::to_string (ll + 1)));
        } else {
          mp_tab_bar->setTabText (int (ll), tl::to_qstring (mp_view->get_properties (ll).name ()));
        }
      }

    }

    if (mp_tab_bar->currentIndex () != int (mp_view->current_layer_list ())) {
      mp_tab_bar->setCurrentIndex (int (mp_view->current_layer_list ()));
    }

    connect (mp_tab_bar, SIGNAL (currentChanged (int)), this, SLOT (tab_selected (int)));

    m_tabs_need_update = false;

  }

  if (m_in_update) {

    m_in_update = false;

    //  HACK: reset the internal hover state to avoid badly indexed items being addressed.
    QHoverEvent hoverEvent (QEvent::HoverLeave, QPoint (0, 0), QPoint (0, 0));
    QCoreApplication::sendEvent (mp_layer_list->viewport (), &hoverEvent);
    //  reset the current index for the same reason
    mp_layer_list->setCurrentIndex(QModelIndex());

    //  this makes the view redraw the data and establishes a valid selection scheme
    mp_model->signal_layers_changed ();

    //  now realize the selection if required
    if (! m_new_sel.empty ()) {
      std::vector <lay::LayerPropertiesConstIterator> new_sel;
      for (std::vector<size_t>::const_iterator s = m_new_sel.begin (); s != m_new_sel.end (); ++s) {
        new_sel.push_back (lay::LayerPropertiesConstIterator (mp_view->get_properties (), *s));
      }
      set_selection (new_sel);
      m_new_sel.clear ();
    }

    bool has_children = false;
    for (lay::LayerPropertiesConstIterator l = mp_view->begin_layers (); l != mp_view->end_layers () && ! has_children; ++l) {
      if (l->has_children ()) {
        has_children = true;
      }
    }

    restore_expanded ();

    mp_layer_list->setRootIsDecorated (has_children);
    mp_layer_list->doItemsLayout ();

    m_needs_update = false;

  } else if (m_needs_update) {

    m_needs_update = false;

    bool has_children = false;
    for (lay::LayerPropertiesConstIterator l = mp_view->begin_layers (); l != mp_view->end_layers () && ! has_children; ++l) {
      if (l->has_children ()) {
        has_children = true;
      }
    }
    mp_layer_list->setRootIsDecorated (has_children);
    mp_layer_list->reset ();

  } else {
    mp_model->signal_data_changed ();  //  this makes the view redraw the data
  }

  if (m_hidden_flags_need_update) {
    do_update_hidden_flags ();
    m_hidden_flags_need_update = false;
  }

  if (m_expanded_state_needs_update) {
    restore_expanded ();
    m_expanded_state_needs_update = false;
  }
}

void
LayerControlPanel::set_current_layer (const lay::LayerPropertiesConstIterator &l)
{
  if (transacting ()) {
    manager ()->queue (this, new LayerSelectionClearOp ());
  }

  end_updates ();

  if (m_in_update) {
    //  while in update, the layer list does not follow the selection, so keep a temporary one
    m_current_layer = l.uint ();
  } else {
    mp_layer_list->set_current (l);
  }
}

lay::LayerPropertiesConstIterator 
LayerControlPanel::current_layer () const
{
  if (m_in_update) {
    return lay::LayerPropertiesConstIterator (mp_view->get_properties (), m_current_layer);
  } else {
    return mp_model->iterator (mp_layer_list->currentIndex ());
  }
}

std::vector <lay::LayerPropertiesConstIterator>
LayerControlPanel::selected_layers () const
{
  if (m_in_update) {

    std::vector <lay::LayerPropertiesConstIterator> new_sel;
    for (std::vector<size_t>::const_iterator s = m_new_sel.begin (); s != m_new_sel.end (); ++s) {
      new_sel.push_back (lay::LayerPropertiesConstIterator (mp_view->get_properties (), *s));
    }

    return new_sel;

  } else {

    QModelIndexList selected = mp_layer_list->selectionModel ()->selectedIndexes ();

    std::vector <lay::LayerPropertiesConstIterator> llist;
    llist.reserve (selected.size ());
    for (QModelIndexList::const_iterator i = selected.begin (); i != selected.end (); ++i) {
      if (i->column () == 0) {
        lay::LayerPropertiesConstIterator iter (mp_model->iterator (*i));
        if (! iter.is_null () && ! iter.at_end ()) {
          llist.push_back (iter);
        }
      }
    }

    //  filter out the children:
    //  we employ the fact, that the LayerPropertiesConstIterator's are ordered
    //  parents first and children before siblings.
    std::sort (llist.begin (), llist.end ());

    std::vector<lay::LayerPropertiesConstIterator>::iterator write = llist.begin ();
    for (std::vector<lay::LayerPropertiesConstIterator>::iterator read = llist.begin (); read != llist.end (); ) {

      lay::LayerPropertiesConstIterator n = *read;
      *write++ = n;
      n.next_sibling ();

      read = std::lower_bound (read + 1, llist.end (), n);

    }

    llist.erase (write, llist.end ());
    return llist;
  }
}

void 
LayerControlPanel::undo (db::Op *op)
{
  LayerSelectionClearOp *clrop = dynamic_cast <LayerSelectionClearOp *> (op);
  if (clrop) {
    set_selection (std::vector<lay::LayerPropertiesConstIterator> ()); // clear selection
    return;
  }
}

void 
LayerControlPanel::redo (db::Op *op)
{
  LayerSelectionClearOp *clrop = dynamic_cast <LayerSelectionClearOp *> (op);
  if (clrop) {
    set_selection (std::vector<lay::LayerPropertiesConstIterator> ()); // clear selection
    return;
  }
}

void
LayerControlPanel::signal_resolution_changed ()
{
  m_do_update_content_dm ();
}

void 
LayerControlPanel::signal_vp_changed ()
{
  if (mp_model->get_test_shapes_in_view ()) {
    update_required (1);
  }
}

void 
LayerControlPanel::signal_cv_changed ()
{
  update_required (1);
}

void
LayerControlPanel::signal_cv_changed_with_int (int)
{
  update_required (1);
}

void
LayerControlPanel::signal_ll_changed (int)
{
  // layer lists have changed - do a full update in this case
  update_required (7);
}

void 
LayerControlPanel::signal_li_changed (int)
{
  // layer list index has changed - do a full update in this case
  update_required (7);
}

void 
LayerControlPanel::update_required (int f)
{
  //  the name of a layer list has changed
  if ((f & 8) != 0) {
    m_expanded_state_needs_update = true;
  }

  //  the name of a layer list has changed
  if ((f & 4) != 0) {
    m_tabs_need_update = true;
  }

  //  mark the hierarchy as having changed.
  if ((f & 2) != 0) {

    m_needs_update = true;

    //  if the signal arises from any action performed externally, we cannot rely on 
    //  getting a end_updates - we have to do this explicitly here.
    if (! m_in_update) {
      begin_updates ();
    }

  }

  if ((f & 3) != 0) {
    m_hidden_flags_need_update = true;
  }

  m_do_update_content_dm ();
}

void
LayerControlPanel::current_index_changed (const QModelIndex &index)
{
  lay::LayerPropertiesConstIterator iter = mp_model->iterator (index);
  if (! iter.is_null () && ! iter.at_end ()) {
    emit current_layer_changed (iter);
  } else {
    emit current_layer_changed (lay::LayerPropertiesConstIterator ());
  }
}

void
LayerControlPanel::selection_changed (const QItemSelection &, const QItemSelection &)
{
  if (m_layer_visibility_follows_selection) {
    m_do_update_visibility_dm ();
  }
  emit selected_layers_changed ();
}

void
LayerControlPanel::group_collapsed (const QModelIndex &index)
{
  auto iter = mp_model->iterator_nc (index);
  if (! iter.is_null () && ! iter.at_end ()) {
    iter->set_expanded_silent (false);
  }
}

void
LayerControlPanel::group_expanded (const QModelIndex &index)
{
  auto iter = mp_model->iterator_nc (index);
  if (! iter.is_null () && ! iter.at_end ()) {
    iter->set_expanded_silent (true);
  }
}

void
LayerControlPanel::restore_expanded ()
{
  mp_layer_list->blockSignals (true);

  lay::LayerPropertiesConstIterator l = mp_view->begin_layers ();
  while (! l.at_end ()) {
    QModelIndex index = mp_model->index (l, 0);
    if (l->expanded ()) {
      mp_layer_list->expand (index);
    } else {
      mp_layer_list->collapse (index);
    }
    ++l;
  }

  mp_layer_list->blockSignals (false);
}

void
LayerControlPanel::up_clicked ()
{
  BEGIN_PROTECTED_CLEANUP

  if (mp_view) {
    mp_view->transaction (tl::to_string (QObject::tr ("Move up")));
    do_move (1 /*up*/);
    mp_view->commit ();
  }

  END_PROTECTED_CLEANUP { recover (); }
}

void
LayerControlPanel::down_clicked ()
{
  BEGIN_PROTECTED_CLEANUP
  
  if (mp_view) {
    mp_view->transaction (tl::to_string (QObject::tr ("Move down")));
    do_move (0 /*down*/);
    mp_view->commit ();
  }

  END_PROTECTED_CLEANUP { recover (); }
}

void
LayerControlPanel::downdown_clicked ()
{
  BEGIN_PROTECTED_CLEANUP
  
  if (mp_view) {
    mp_view->transaction (tl::to_string (QObject::tr ("Move fully down")));
    do_move (2 /*downdown*/);
    mp_view->commit ();
  }

  END_PROTECTED_CLEANUP { recover (); }
}

void
LayerControlPanel::upup_clicked ()
{
  BEGIN_PROTECTED_CLEANUP
  
  if (mp_view) {
    mp_view->transaction (tl::to_string (QObject::tr ("Move fully up")));
    do_move (3 /*upup*/);
    mp_view->commit ();
  }

  END_PROTECTED_CLEANUP { recover (); }
}

static void
move_algo (std::vector<lay::LayerPropertiesConstIterator>::const_iterator from, 
           std::vector<lay::LayerPropertiesConstIterator>::const_iterator to, 
           lay::LayerPropertiesConstIterator parent,
           lay::LayerPropertiesIterator new_parent,
           std::vector<lay::LayerPropertiesConstIterator> &new_sel,
           int mode)
{
  size_t nsel = new_sel.size ();

  std::vector<lay::LayerPropertiesConstIterator> org_sel;

  std::vector<lay::LayerPropertiesConstIterator>::const_iterator s = from;
  lay::LayerPropertiesConstIterator c = parent;
  lay::LayerPropertiesConstIterator nc = new_parent;
  for (c.down_first_child (), nc.down_first_child (); ! c.at_end (); c.next_sibling (), nc.next_sibling ()) {
    s = std::lower_bound (s, to, c);
    if (s != to && *s == c) {
      //  a selected child: remember this position
      new_sel.push_back (nc);
      org_sel.push_back (c);
    }
  }

  //  compute new positions from the current ones ..
  if (mode == 0 /*down*/) {

    lay::LayerPropertiesConstIterator l;
    for (std::vector<lay::LayerPropertiesConstIterator>::iterator i = new_sel.end (); i != new_sel.begin () + nsel; ) {
      lay::LayerPropertiesConstIterator ns = *--i;
      ns.next_sibling ();
      if (! ns.at_end () && ns != l) {
        *i = ns;
      } 
      l = *i;
    }

  } else if (mode == 1 /*up*/) {

    lay::LayerPropertiesConstIterator l;
    for (std::vector<lay::LayerPropertiesConstIterator>::iterator i = new_sel.begin () + nsel; i != new_sel.end (); ++i) {
      lay::LayerPropertiesConstIterator ns = *i;
      if (ns.child_index () > 0) {
        ns.next_sibling (-1);
      }
      if (ns != l) {
        *i = ns;
      } 
      l = *i;
    }

  } else if (mode == 2 /*downdown*/) {

    if (new_sel.end () != new_sel.begin () + nsel) {
      size_t n = new_sel.begin ()[nsel].num_siblings ();
      for (std::vector<lay::LayerPropertiesConstIterator>::iterator i = new_sel.end (); i != new_sel.begin () + nsel; ) {
        --i;
        i->to_sibling (--n);
      }
    }

  } else if (mode == 3 /*upup*/) {
    
    size_t n = 0;
    for (std::vector<lay::LayerPropertiesConstIterator>::iterator i = new_sel.begin () + nsel; i != new_sel.end (); ++i) {
      i->to_sibling (n++);
    }

  }

  std::vector<lay::LayerPropertiesConstIterator>::const_iterator inew = new_sel.begin () + nsel;
  std::vector<lay::LayerPropertiesConstIterator>::const_iterator iorg = org_sel.begin ();

  lay::LayerPropertiesIterator ins = new_parent;
  ins.down_first_child ();

  std::vector< std::pair<lay::LayerPropertiesConstIterator, lay::LayerPropertiesIterator> > rec;

  s = from;
  c = parent;
  for (c.down_first_child (); ! c.at_end (); c.next_sibling ()) {

    while (inew != new_sel.end () && ins == *inew) {
      *ins = **iorg;
      ins.next_sibling ();
      ++inew;
      ++iorg;
    }

    s = std::lower_bound (s, to, c);
    if (s == to || *s != c) {
      *ins = *c;
      if (c->has_children ()) {
        rec.push_back (std::make_pair (c, ins));
      }
      ins.next_sibling ();
    }

  }

  while (inew != new_sel.end () && ins == *inew) {
    *ins = **iorg;
    ins.next_sibling ();
    ++inew;
    ++iorg;
  }

  //  now treat all nodes with children (it is important to do this at last, because then the
  //  child iterators will be valid finally and can be inserted into "new_sel")
  for (std::vector< std::pair<lay::LayerPropertiesConstIterator, lay::LayerPropertiesIterator> >::iterator r = rec.begin (); r != rec.end (); ++r) {
    move_algo (from, to, r->first, r->second, new_sel, mode);
  }

}

void
LayerControlPanel::do_move (int mode)
{
  std::vector<lay::LayerPropertiesConstIterator> sel = mp_view->selected_layers ();
  std::vector<lay::LayerPropertiesConstIterator> new_sel;
  new_sel.reserve (sel.size ());

  lay::LayerPropertiesList new_props (mp_view->get_properties ());

  move_algo (sel.begin (), sel.end (), lay::LayerPropertiesConstIterator (mp_view->get_properties (), size_t (0)), lay::LayerPropertiesIterator (new_props, size_t (0)), new_sel, mode);

  mp_view->set_properties (new_props);
  mp_view->set_selected_layers (new_sel);
}

// ------------------------------------------------------------
//  Declaration of the "plugin" for the menu entries

class LayerControlPanelPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    std::string at;

    at = ".end";
    menu_entries.push_back (lay::submenu ("@lcp_context_menu", at, std::string ()));

    at = "@lcp_context_menu.end";

    menu_entries.push_back (lay::menu_item ("cm_lv_select_all", "select_all", at, tl::to_string (QObject::tr ("Select All"))));
    menu_entries.push_back (lay::menu_item ("cm_lv_invert_selection", "invert_selection", at, tl::to_string (QObject::tr ("Invert Selection"))));
    //  It is not sure, whether "expandAll" destabilizes the tree widget:
    //  menu_entries.push_back (lay::menu_item ("cm_lv_expand_all", "expand_all", at, tl::to_string (QObject::tr ("Expand All"))));
    menu_entries.push_back (lay::separator ("tab_group", at));
    menu_entries.push_back (lay::submenu ("tab_menu", at, tl::to_string (QObject::tr ("Tabs"))));

    {
      std::string at = "@lcp_context_menu.tab_menu.end";
      menu_entries.push_back (lay::menu_item ("cm_lv_new_tab", "new_tab", at, tl::to_string (QObject::tr ("New Tab"))));
      menu_entries.push_back (lay::menu_item ("cm_lv_remove_tab", "remove_tab", at, tl::to_string (QObject::tr ("Remove Tab"))));
      menu_entries.push_back (lay::menu_item ("cm_lv_rename_tab", "rename_tab", at, tl::to_string (QObject::tr ("Rename Tab"))));
    }

    menu_entries.push_back (lay::separator ("visibility_group", at));
    menu_entries.push_back (lay::config_menu_item ("visibility_follows_selection", at, tl::to_string (QObject::tr ("Visibility Follows Selection")), cfg_layer_visibility_follows_selection, "?"));
    menu_entries.push_back (lay::menu_item ("cm_lv_hide", "hide", at, tl::to_string (QObject::tr ("Hide"))));
    menu_entries.push_back (lay::menu_item ("cm_lv_hide_all", "hide_all", at, tl::to_string (QObject::tr ("Hide All"))));
    menu_entries.push_back (lay::menu_item ("cm_lv_show", "show", at, tl::to_string (QObject::tr ("Show"))));
    menu_entries.push_back (lay::menu_item ("cm_lv_show_all", "show_all", at, tl::to_string (QObject::tr ("Show All"))));
    menu_entries.push_back (lay::menu_item ("cm_lv_show_only", "show_only", at, tl::to_string (QObject::tr ("Show Only Selected"))));
    menu_entries.push_back (lay::menu_item ("cm_lv_toggle_visibility", "toggle_visibility", at, tl::to_string (QObject::tr ("Toggle Visibility"))));
    menu_entries.push_back (lay::menu_item ("cm_lv_make_valid", "valid", at, tl::to_string (QObject::tr ("Make Valid"))));
    menu_entries.push_back (lay::menu_item ("cm_lv_make_invalid", "invvalid", at, tl::to_string (QObject::tr ("Make Invalid"))));
    menu_entries.push_back (lay::menu_item ("cm_lv_rename", "rename", at, tl::to_string (QObject::tr ("Rename"))));
    menu_entries.push_back (lay::separator ("options_group", at));
    menu_entries.push_back (lay::config_menu_item ("hide_empty_layers", at, tl::to_string (QObject::tr ("Hide Empty Layers")), cfg_hide_empty_layers, "?"));
    menu_entries.push_back (lay::config_menu_item ("test_shapes_in_view", at, tl::to_string (QObject::tr ("Test For Shapes In View")), cfg_test_shapes_in_view, "?"));
    menu_entries.push_back (lay::separator ("source_group", at));
    menu_entries.push_back (lay::menu_item ("cm_lv_source", "select_source", at, tl::to_string (QObject::tr ("Select Source"))));
    menu_entries.push_back (lay::separator ("sort_group", at));
    menu_entries.push_back (lay::submenu ("sort_menu", at, tl::to_string (QObject::tr ("Sort By"))));

    {
      std::string at = "@lcp_context_menu.sort_menu.end";
      menu_entries.push_back (lay::menu_item ("cm_lv_sort_by_ild", "sort_ild", at, tl::to_string (QObject::tr ("Layout Index, Layer And Datatype"))));
      menu_entries.push_back (lay::menu_item ("cm_lv_sort_by_idl", "sort_idl", at, tl::to_string (QObject::tr ("Layout Index, Datatype And Layer"))));
      menu_entries.push_back (lay::menu_item ("cm_lv_sort_by_ldi", "sort_ldi", at, tl::to_string (QObject::tr ("Layer, Datatype And Layout Index"))));
      menu_entries.push_back (lay::menu_item ("cm_lv_sort_by_dli", "sort_dli", at, tl::to_string (QObject::tr ("Datatype, Layer And Layout Index"))));
      menu_entries.push_back (lay::menu_item ("cm_lv_sort_by_name", "sort_name", at, tl::to_string (QObject::tr ("Name"))));
    }

    menu_entries.push_back (lay::separator ("view_group", at));
    menu_entries.push_back (lay::menu_item ("cm_lv_delete", "del", at, tl::to_string (QObject::tr ("Delete Layer Entry"))));
    menu_entries.push_back (lay::menu_item ("cm_lv_insert", "insert", at, tl::to_string (QObject::tr ("Insert Layer Entry"))));
    menu_entries.push_back (lay::menu_item ("cm_lv_add_missing", "add_others", at, tl::to_string (QObject::tr ("Add Other Layer Entries"))));
    menu_entries.push_back (lay::menu_item ("cm_lv_remove_unused", "clean_up", at, tl::to_string (QObject::tr ("Clean Up Layer Entries"))));
    menu_entries.push_back (lay::separator ("grouping_group", at));
    menu_entries.push_back (lay::menu_item ("cm_lv_group", "group", at, tl::to_string (QObject::tr ("Group"))));
    menu_entries.push_back (lay::menu_item ("cm_lv_ungroup", "ungroup", at, tl::to_string (QObject::tr ("Ungroup"))));
    menu_entries.push_back (lay::submenu ("regroup_menu", at, tl::to_string (QObject::tr ("Regroup Layer Entries"))));

    {
      std::string at = "@lcp_context_menu.regroup_menu.end";
      menu_entries.push_back (lay::menu_item ("cm_lv_regroup_by_index", "grp_i", at, tl::to_string (QObject::tr ("By Layout Index"))));
      menu_entries.push_back (lay::menu_item ("cm_lv_regroup_by_datatype", "grp_d", at, tl::to_string (QObject::tr ("By Datatype"))));
      menu_entries.push_back (lay::menu_item ("cm_lv_regroup_by_layer", "grp_l", at, tl::to_string (QObject::tr ("By Layer"))));
      menu_entries.push_back (lay::menu_item ("cm_lv_regroup_flatten", "flatten", at, tl::to_string (QObject::tr ("Flatten"))));
    }

    menu_entries.push_back (lay::separator ("copy_paste_group", at));
    menu_entries.push_back (lay::menu_item ("cm_lv_copy", "copy", at, tl::to_string (QObject::tr ("Copy"))));
    menu_entries.push_back (lay::menu_item ("cm_lv_cut", "cut", at, tl::to_string (QObject::tr ("Cut"))));
    menu_entries.push_back (lay::menu_item ("cm_lv_paste", "paste", at, tl::to_string (QObject::tr ("Paste"))));

    at = ".end";
    menu_entries.push_back (lay::submenu ("@lcp_tabs_context_menu", at, std::string ()));

    {
      std::string at = "@lcp_tabs_context_menu.end";
      menu_entries.push_back (lay::menu_item ("cm_lv_new_tab", "new_tab", at, tl::to_string (QObject::tr ("New Tab"))));
      menu_entries.push_back (lay::menu_item ("cm_lv_remove_tab", "remove_tab", at, tl::to_string (QObject::tr ("Remove Tab"))));
      menu_entries.push_back (lay::menu_item ("cm_lv_rename_tab", "rename_tab", at, tl::to_string (QObject::tr ("Rename Tab"))));
    }
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new LayerControlPanelPluginDeclaration (), -9, "LayerControlPanelPlugin");

} // namespace lay 

#endif
