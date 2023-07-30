
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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

#include <iostream>
#include <fstream>
#include <vector>

#include <QTimer>
#include <QSpinBox>
#include <QPainter>
#include <QPaintEvent>
#include <QComboBox>
#include <QDialog>
#include <QImageWriter>
#include <QInputDialog>
#include <QKeyEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QApplication>
#include <QPushButton>
#include <QToolButton>

#include "tlInternational.h"
#include "tlExpression.h"
#include "tlTimer.h"
#include "tlString.h"
#include "tlLog.h"
#include "tlAssert.h"
#include "tlExceptions.h"
#include "tlStaticObjects.h"
#include "layLayoutView.h"
#include "layViewOp.h"
#include "layViewObject.h"
#include "laybasicConfig.h"
#include "layConverters.h"
#include "layGridNet.h"
#include "layMove.h"
#include "layZoomBox.h"
#include "layMouseTracker.h"
#include "layEditable.h"
#include "layFixedFont.h"
#include "laySelector.h"
#include "layLayoutCanvas.h"
#include "layLayerControlPanel.h"
#include "layLayerToolbox.h"
#include "layHierarchyControlPanel.h"
#include "layLibrariesView.h"
#include "layBrowser.h"
#include "layRedrawThread.h"
#include "layRedrawThreadWorker.h"
#include "layParsedLayerSource.h"
#include "layBookmarkManagementForm.h"
#include "layNetlistBrowserDialog.h"
#include "layBookmarksView.h"
#include "layEditorOptionsFrame.h"
#include "layEditorOptionsPages.h"
#include "layUtils.h"
#include "layPropertiesDialog.h"
#include "layQtTools.h"
#include "dbClipboard.h"
#include "dbLayout.h"
#include "dbLayoutUtils.h"
#include "dbManager.h"
#include "dbLibrary.h"
#include "rdb.h"
#include "rdbMarkerBrowserDialog.h"
#include "dbLayoutToNetlist.h"
#include "tlXMLParser.h"
#include "gsi.h"
#include "gtf.h"

#include <limits>
#include <QFrame>

namespace lay
{

// -------------------------------------------------------------
//  LayoutViewNotificationWidget implementation

LayoutViewNotificationWidget::LayoutViewNotificationWidget (LayoutViewWidget *parent, const LayoutViewNotification *notification)
  : QFrame (parent), mp_parent (parent), mp_notification (notification)
{
  setBackgroundRole (QPalette::ToolTipBase);
  setAutoFillBackground (true);

  QHBoxLayout *layout = new QHBoxLayout (this);
  layout->setContentsMargins (4, 4, 4, 4);

  QLabel *title_label = new QLabel (this);
  layout->addWidget (title_label, 1);
  title_label->setText (tl::to_qstring (notification->title ()));
  title_label->setForegroundRole (QPalette::ToolTipText);
  title_label->setWordWrap (true);
  activate_help_links (title_label);

  for (auto a = notification->actions ().begin (); a != notification->actions ().end (); ++a) {

    QPushButton *pb = new QPushButton (this);
    layout->addWidget (pb);

    pb->setText (tl::to_qstring (a->second));
    m_action_buttons.insert (std::make_pair (pb, a->first));
    connect (pb, SIGNAL (clicked ()), this, SLOT (action_triggered ()));

  }

  QToolButton *close_button = new QToolButton ();
  close_button->setIcon (QIcon (":clear_edit_16px.png"));
  close_button->setAutoRaise (true);
  layout->addWidget (close_button);

  connect (close_button, SIGNAL (clicked ()), this, SLOT (close_triggered ()));
}

void
LayoutViewNotificationWidget::action_triggered ()
{
  auto a = m_action_buttons.find (sender ());
  if (a != m_action_buttons.end ()) {
    mp_parent->notification_action (*mp_notification, a->second);
  }
}

void
LayoutViewNotificationWidget::close_triggered ()
{
  mp_parent->remove_notification (*mp_notification);
}

// -------------------------------------------------------------
//  LayoutViewWidget implementation

LayoutViewWidget::LayoutViewWidget (db::Manager *mgr, bool editable, lay::Plugin *plugin_parent, QWidget *parent, unsigned int options)
  : QFrame (parent), mp_view (0)
{
  mp_layout = new QVBoxLayout (this);
  mp_layout->setContentsMargins (0, 0, 0, 0);
  mp_layout->setSpacing (0);
  mp_layout->addStretch (1);

  //  NOTE: construction the LayoutView may trigger events (script code executed etc.) which must
  //  not meet an invalid mp_view pointer (e.g. in eventFilter). Hence, mp_view is 0 first, and set only
  //  after the LayoutView is successfully constructed.
  std::unique_ptr<LayoutView> view (new LayoutView (mgr, editable, plugin_parent, this, options));
  mp_view = view.release ();
}

LayoutViewWidget::LayoutViewWidget (lay::LayoutView *source, db::Manager *mgr, bool editable, lay::Plugin *plugin_parent, QWidget *parent, unsigned int options)
  : QFrame (parent), mp_view (0)
{
  mp_layout = new QVBoxLayout (this);
  mp_layout->setContentsMargins (0, 0, 0, 0);
  mp_layout->setSpacing (0);
  mp_layout->addStretch (1);

  //  NOTE: construction the LayoutView may trigger events (script code executed etc.) which must
  //  not meet an invalid mp_view pointer (e.g. in eventFilter). Hence, mp_view is 0 first, and set only
  //  after the LayoutView is successfully constructed.
  std::unique_ptr<LayoutView> view (new LayoutView (source, mgr, editable, plugin_parent, this, options));
  mp_view = view.release ();
}

LayoutViewWidget::~LayoutViewWidget ()
{
  lay::LayoutView *view = mp_view;
  mp_view = 0;
  delete view;
}

void
LayoutViewWidget::add_notification (const LayoutViewNotification &notificaton)
{
  if (m_notification_widgets.find (&notificaton) == m_notification_widgets.end ()) {
    m_notifications.push_back (notificaton);
    QWidget *w = new LayoutViewNotificationWidget (this, &m_notifications.back ());
    m_notification_widgets.insert (std::make_pair (&m_notifications.back (), w));
    mp_layout->insertWidget (0, w);
  }
}

void
LayoutViewWidget::remove_notification (const LayoutViewNotification &notification)
{
  auto nw = m_notification_widgets.find (&notification);
  if (nw != m_notification_widgets.end ()) {

    nw->second->deleteLater ();
    m_notification_widgets.erase (nw);

    for (auto n = m_notifications.begin (); n != m_notifications.end (); ++n) {
      if (*n == notification) {
        m_notifications.erase (n);
        break;
      }
    }

  }
}

void
LayoutViewWidget::notification_action (const LayoutViewNotification &notification, const std::string &action)
{
  if (action == "reload") {

    std::string fn = notification.parameter ().to_string ();

    for (unsigned int cvi = 0; cvi < mp_view->cellviews (); ++cvi) {
      const lay::CellView &cv = mp_view->cellview (cvi);
      if (cv->filename () == fn) {
        mp_view->reload_layout (cvi);
      }
    }

    remove_notification (notification);

  }
}

void
LayoutViewWidget::view_deleted (lay::LayoutView *view)
{
  if (view != mp_view) {
    return;
  }

  //  creates a new view so the view is never invalid
  mp_view = new LayoutView (view->manager (), view->is_editable (), view->plugin_parent (), this, view->options ());
}

void
LayoutViewWidget::resizeEvent (QResizeEvent *)
{
  if (mp_view && mp_view->canvas ()) {
    mp_view->canvas ()->resize (width (), height ());
  }
}

QSize
LayoutViewWidget::sizeHint () const
{
  return mp_view ? mp_view->size_hint () : QFrame::sizeHint ();
}

bool
LayoutViewWidget::eventFilter(QObject *obj, QEvent *event)
{
  if (! mp_view) {
    return QFrame::eventFilter (obj, event);
  }

  bool taken = false;
  bool res = mp_view->event_filter (obj, event, taken);
  if (taken) {
    return res;
  } else {
    return QFrame::eventFilter (obj, event);
  }
}

void LayoutViewWidget::showEvent (QShowEvent *)
{
  if (mp_view) {
    mp_view->show_event ();
  }
}

void LayoutViewWidget::hideEvent (QHideEvent *)
{
  if (mp_view) {
    mp_view->hide_event ();
  }
}

QWidget *LayoutViewWidget::layer_control_frame ()
{
  return !mp_view ? 0 : mp_view->layer_control_frame ();
}

QWidget *LayoutViewWidget::layer_toolbox_frame ()
{
  return !mp_view ? 0 : mp_view->layer_toolbox_frame ();
}

QWidget *LayoutViewWidget::hierarchy_control_frame ()
{
  return !mp_view ? 0 : mp_view->hierarchy_control_frame ();
}

QWidget *LayoutViewWidget::libraries_frame ()
{
  return !mp_view ? 0 : mp_view->libraries_frame ();
}

QWidget *LayoutViewWidget::bookmarks_frame ()
{
  return !mp_view ? 0 : mp_view->bookmarks_frame ();
}

QWidget *LayoutViewWidget::editor_options_frame ()
{
  return !mp_view ? 0 : mp_view->editor_options_frame ();
}

// -------------------------------------------------------------
//  LayoutViewConnector implementation

LayoutViewSignalConnector::LayoutViewSignalConnector (QObject *parent, lay::LayoutView *view)
  : QObject (parent), mp_view (view)
{
  //  .. nothing yet ..
}

void LayoutViewSignalConnector::active_cellview_changed (int index)
{
  mp_view->active_cellview_changed (index);
}

void LayoutViewSignalConnector::active_library_changed (int index)
{
  mp_view->active_cellview_changed (index);
}

void LayoutViewSignalConnector::side_panel_destroyed ()
{
  mp_view->side_panel_destroyed (sender ());
}

void LayoutViewSignalConnector::select_cell_dispatch (const lay::LayoutViewBase::cell_path_type &path, int cellview_index)
{
  mp_view->select_cell_dispatch (path, cellview_index);
}

void LayoutViewSignalConnector::current_layer_changed_slot (const lay::LayerPropertiesConstIterator &iter)
{
  mp_view->current_layer_changed_slot (iter);
}

void LayoutViewSignalConnector::timer ()
{
  mp_view->timer ();
}

void LayoutViewSignalConnector::layer_tab_changed ()
{
  mp_view->layer_tab_changed ();
}

void LayoutViewSignalConnector::layer_order_changed ()
{
  mp_view->layer_order_changed ();
}

void LayoutViewSignalConnector::min_hier_changed (int i)
{
  mp_view->min_hier_changed (i);
}

void LayoutViewSignalConnector::max_hier_changed (int i)
{
  mp_view->max_hier_changed (i);
}

void LayoutViewSignalConnector::app_terminated ()
{
  mp_view->close ();
}

// -------------------------------------------------------------

const int timer_interval = 10;

static LayoutView *ms_current = 0;

LayoutView::LayoutView (db::Manager *manager, bool editable, lay::Plugin *plugin_parent, unsigned int options)
  : LayoutViewBase (this, manager, editable, plugin_parent, options),
    mp_widget (0),
    dm_setup_editor_option_pages (this, &LayoutView::do_setup_editor_options_pages)
{
  //  ensures the deferred method scheduler is present
  tl::DeferredMethodScheduler::instance ();

  init_ui (manager);
}

LayoutView::LayoutView (lay::LayoutView *source, db::Manager *manager, bool editable, lay::Plugin *plugin_parent, unsigned int options)
  : LayoutViewBase (this, manager, editable, plugin_parent, options),
    mp_widget (0),
    dm_setup_editor_option_pages (this, &LayoutView::do_setup_editor_options_pages)
{
  //  ensures the deferred method scheduler is present
  tl::DeferredMethodScheduler::instance ();

  init_ui (manager);

  copy_from (source);

  bookmarks (source->bookmarks ());
  LayoutView::set_active_cellview_index (source->active_cellview_index ());
}

LayoutView::LayoutView (db::Manager *manager, bool editable, lay::Plugin *plugin_parent, LayoutViewWidget *widget, unsigned int options)
  : LayoutViewBase (this, manager, editable, plugin_parent, options),
    mp_widget (widget),
    dm_setup_editor_option_pages (this, &LayoutView::do_setup_editor_options_pages)
{
  //  ensures the deferred method scheduler is present
  tl::DeferredMethodScheduler::instance ();

  init_ui (manager);
}

LayoutView::LayoutView (lay::LayoutView *source, db::Manager *manager, bool editable, lay::Plugin *plugin_parent, LayoutViewWidget *widget, unsigned int options)
  : LayoutViewBase (this, manager, editable, plugin_parent, options),
    mp_widget (widget),
    dm_setup_editor_option_pages (this, &LayoutView::do_setup_editor_options_pages)
{
  //  ensures the deferred method scheduler is present
  tl::DeferredMethodScheduler::instance ();

  init_ui (manager);

  copy_from (source);

  bookmarks (source->bookmarks ());
  LayoutView::set_active_cellview_index (source->active_cellview_index ());
}

bool
LayoutView::event_filter (QObject *obj, QEvent *event, bool &taken)
{
  if (obj == mp_min_hier_spbx || obj == mp_max_hier_spbx) {

    taken = true;

    //  Makes the min/max spin boxes accept only numeric and some control keys ..
    QKeyEvent *keyEvent = dynamic_cast<QKeyEvent*>(event);
    if (keyEvent && 
        keyEvent->key () != Qt::Key_Home &&
        keyEvent->key () != Qt::Key_End &&
        keyEvent->key () != Qt::Key_Delete &&
        keyEvent->key () != Qt::Key_Backspace &&
        (keyEvent->key () < Qt::Key_0 || keyEvent->key () > Qt::Key_9)) {
      return true;
    }

  }

  return false;
}

void
LayoutView::init_ui (db::Manager *mgr)
{
  m_activated = true;
  m_always_show_source = false;
  m_always_show_ld = true;
  m_always_show_layout_index = false;

  mp_connector = 0;
  mp_timer = 0;
  mp_left_frame = 0;
  mp_control_panel = 0;
  mp_hierarchy_panel = 0;
  mp_libraries_view = 0;
  mp_bookmarks_view = 0;
  mp_control_frame = 0;
  mp_toolbox = 0;
  mp_toolbox_frame = 0;
  mp_hierarchy_frame = 0;
  mp_libraries_frame = 0;
  mp_bookmarks_frame = 0;
  mp_editor_options_frame = 0;
  mp_min_hier_spbx = 0;
  mp_max_hier_spbx = 0;

  //  NOTE: it's important to call LayoutViewBase::init from LayoutView because creating the plugins will need a
  //  fully constructed LayoutView (issue #1360)
  LayoutViewBase::init (mgr);

  if (mp_widget) {

    canvas ()->init_ui (mp_widget);

    mp_connector = new LayoutViewSignalConnector (mp_widget, this);

    if ((options () & LV_NoHierarchyPanel) == 0 && (options () & LV_Naked) == 0) {

      QFrame *hierarchy_frame = new QFrame (0);
      hierarchy_frame->setObjectName (QString::fromUtf8 ("left"));
      mp_hierarchy_frame = hierarchy_frame;
      QVBoxLayout *left_frame_ly = new QVBoxLayout (hierarchy_frame);
      left_frame_ly->setContentsMargins (0, 0, 0, 0);
      left_frame_ly->setSpacing (0);

      mp_hierarchy_panel = new lay::HierarchyControlPanel (this, hierarchy_frame, "hcp");
      left_frame_ly->addWidget (mp_hierarchy_panel, 1 /*stretch*/);

      QObject::connect (mp_hierarchy_panel, SIGNAL (cell_selected (cell_path_type, int)), mp_connector, SLOT (select_cell_dispatch (cell_path_type, int)));
      QObject::connect (mp_hierarchy_panel, SIGNAL (active_cellview_changed (int)), mp_connector, SLOT (active_cellview_changed (int)));
      QObject::connect (mp_hierarchy_frame, SIGNAL (destroyed ()), mp_connector, SLOT (side_panel_destroyed ()));

      QFrame *levels_frame = new QFrame (hierarchy_frame);
      levels_frame->setObjectName (QString::fromUtf8 ("lvl_frame"));
      left_frame_ly->addWidget (levels_frame);
      QHBoxLayout *levels_frame_ly = new QHBoxLayout (levels_frame);
      levels_frame_ly->setContentsMargins (1, 1, 1, 1);
      QLabel *level_l1 = new QLabel (tl::to_qstring (" " + tl::to_string (QObject::tr ("Levels"))), levels_frame);
      levels_frame_ly->addWidget (level_l1);
      mp_min_hier_spbx = new QSpinBox (levels_frame);
      mp_min_hier_spbx->setObjectName (QString::fromUtf8 ("min_lvl"));
      levels_frame_ly->addWidget (mp_min_hier_spbx);
      QLabel *level_l2 = new QLabel (QString::fromUtf8 (".."), levels_frame);
      levels_frame_ly->addWidget (level_l2);
      mp_max_hier_spbx = new QSpinBox (levels_frame);
      mp_max_hier_spbx->setObjectName (QString::fromUtf8 ("max_lvl"));
      levels_frame_ly->addWidget (mp_max_hier_spbx);

      mp_min_hier_spbx->installEventFilter (mp_widget);
      mp_max_hier_spbx->installEventFilter (mp_widget);

      mp_min_hier_spbx->setMaximum (0);
      mp_min_hier_spbx->setMinimum (-1000);
      mp_min_hier_spbx->setValue (0);
      mp_max_hier_spbx->setMaximum (999);
      mp_max_hier_spbx->setValue (0);
      mp_max_hier_spbx->setMinimum (-1000);

      QObject::connect (mp_min_hier_spbx, SIGNAL (valueChanged (int)), mp_connector, SLOT (min_hier_changed (int)));
      QObject::connect (mp_max_hier_spbx, SIGNAL (valueChanged (int)), mp_connector, SLOT (max_hier_changed (int)));

    }

    if ((options () & LV_NoBookmarksView) == 0 && (options () & LV_Naked) == 0) {

      QFrame *bookmarks_frame = new QFrame (0);
      bookmarks_frame->setObjectName (QString::fromUtf8 ("bookmarks_frame"));
      mp_bookmarks_frame = bookmarks_frame;
      QVBoxLayout *left_frame_ly = new QVBoxLayout (bookmarks_frame);
      left_frame_ly->setContentsMargins (0, 0, 0, 0);
      left_frame_ly->setSpacing (0);

      mp_bookmarks_view = new lay::BookmarksView (this, bookmarks_frame, "bookmarks");
      left_frame_ly->addWidget (mp_bookmarks_view, 1 /*stretch*/);

      QObject::connect (mp_bookmarks_frame, SIGNAL (destroyed ()), mp_connector, SLOT (side_panel_destroyed ()));

    }

    if ((options () & LV_NoLibrariesView) == 0 && (options () & LV_Naked) == 0) {

      mp_libraries_frame = new QFrame (0);
      mp_libraries_frame->setObjectName (QString::fromUtf8 ("libs_frame"));
      QVBoxLayout *left_frame_ly = new QVBoxLayout (mp_libraries_frame);
      left_frame_ly->setContentsMargins (0, 0, 0, 0);
      left_frame_ly->setSpacing (0);

      mp_libraries_view = new lay::LibrariesView (this, mp_libraries_frame, "libs");
      left_frame_ly->addWidget (mp_libraries_view, 1 /*stretch*/);

      QObject::connect (mp_libraries_view, SIGNAL (active_library_changed (int)), mp_connector, SLOT (active_library_changed (int)));
      QObject::connect (mp_libraries_frame, SIGNAL (destroyed ()), mp_connector, SLOT (side_panel_destroyed ()));

    }

    if ((options () & LV_NoEditorOptionsPanel) == 0 && (options () & LV_Naked) == 0) {

      mp_editor_options_frame = new lay::EditorOptionsFrame (0);
      mp_editor_options_frame->populate (this);

      QObject::connect (mp_editor_options_frame, SIGNAL (destroyed ()), mp_connector, SLOT (side_panel_destroyed ()));

    }

    if ((options () & LV_NoLayers) == 0 && (options () & LV_Naked) == 0) {

      mp_control_panel = new lay::LayerControlPanel (this, manager (), 0, "lcp");
      mp_control_frame = mp_control_panel;

      QObject::connect (mp_control_frame, SIGNAL (destroyed ()), mp_connector, SLOT (side_panel_destroyed ()));
      QObject::connect (mp_control_panel, SIGNAL (tab_changed ()), mp_connector, SLOT (layer_tab_changed ()));
      QObject::connect (mp_control_panel, SIGNAL (order_changed ()), mp_connector, SLOT (layer_order_changed ()));
      QObject::connect (mp_control_panel, SIGNAL (current_layer_changed (const lay::LayerPropertiesConstIterator &)), mp_connector, SLOT (current_layer_changed_slot (const lay::LayerPropertiesConstIterator &)));

      mp_toolbox_frame = new QFrame (0);
      mp_toolbox_frame->setObjectName (QString::fromUtf8 ("lt_frame"));
      QVBoxLayout *lt_frame_ly = new QVBoxLayout (mp_toolbox_frame);
      lt_frame_ly->setContentsMargins (0, 0, 0, 0);
      lt_frame_ly->setSpacing (0);

      mp_toolbox = new lay::LayerToolbox (mp_toolbox_frame, "lt");
      mp_toolbox->set_view (this);
      lt_frame_ly->addWidget (mp_toolbox, 0 /*stretch*/);

      QObject::connect (mp_toolbox_frame, SIGNAL (destroyed ()), mp_connector, SLOT (side_panel_destroyed ()));

    }

    mp_timer = new QTimer (mp_widget);
    QObject::connect (mp_timer, SIGNAL (timeout ()), mp_connector, SLOT (timer ()));
    mp_timer->start (timer_interval);

  }

  config_setup ();
  finish ();
}

LayoutView::~LayoutView ()
{
  close ();
  if (mp_widget) {
    mp_widget->view_deleted (this);
  }
}

QWidget *LayoutView::widget ()
{
  return mp_widget;
}

void LayoutView::close()
{
  close_event ();
  close_event.clear ();

  if (ms_current == this) {
    ms_current = 0;
  }

  //  release all components and plugins before we delete the user interface
  shutdown ();

  if (mp_control_frame) {
    delete mp_control_frame;
  }
  mp_control_panel = 0;
  mp_control_frame = 0;

  if (mp_toolbox_frame) {
    delete mp_toolbox_frame;
  }
  mp_toolbox = 0;
  mp_toolbox_frame = 0;

  if (mp_hierarchy_frame) {
    delete mp_hierarchy_frame;
  }
  mp_hierarchy_frame = 0;
  mp_hierarchy_panel = 0;

  if (mp_libraries_frame) {
    delete mp_libraries_frame;
  }
  mp_libraries_frame = 0;
  mp_libraries_view = 0;

  if (mp_editor_options_frame) {
    delete mp_editor_options_frame;
  }
  mp_editor_options_frame = 0;

  if (mp_bookmarks_frame) {
    delete mp_bookmarks_frame;
  }
  mp_bookmarks_frame = 0;
  mp_bookmarks_view = 0;

  if (mp_properties_dialog) {
    delete mp_properties_dialog.data ();
  }
}

void
LayoutView::finish ()
{
  if (dispatcher () == this) {
    set_menu_parent_widget (mp_widget);
    init_menu ();
    if (mp_widget) {
      menu ()->build (0, 0);
    }
  }
}

void
LayoutView::show_properties ()
{
  if ((options () & lay::LayoutViewBase::LV_NoPropertiesPopup) != 0) {
    return;
  }

  cancel_edits ();
  if (! has_selection ()) {
    //  try to use the transient selection for the real one
    transient_to_selection ();
  }

  //  re-create a new properties dialog
  QByteArray geom;
  if (mp_properties_dialog) {
    geom = mp_properties_dialog->saveGeometry ();
    delete mp_properties_dialog.data ();
  }
  mp_properties_dialog = new lay::PropertiesDialog (widget (), manager (), this);
  if (! geom.isEmpty ()) {
    mp_properties_dialog->restoreGeometry (geom);
  }

  //  if launched from a dialog, do not use "show" as this blocks user interaction
  if (QApplication::activeModalWidget ()) {
    mp_properties_dialog->exec ();
  } else {
    mp_properties_dialog->show ();
  }
}

void
LayoutView::do_change_active_cellview ()
{
  dm_setup_editor_option_pages ();
}

lay::EditorOptionsPages *LayoutView::editor_options_pages ()
{
  if (! mp_editor_options_frame) {
    return 0;
  } else {
    return mp_editor_options_frame->pages_widget ();
  }
}

void LayoutView::do_setup_editor_options_pages ()
{
  //  initialize the editor option pages
  lay::EditorOptionsPages *eo_pages = editor_options_pages ();
  if (eo_pages) {
    for (std::vector<lay::EditorOptionsPage *>::const_iterator op = eo_pages->pages ().begin (); op != eo_pages->pages ().end (); ++op) {
      (*op)->setup (this);
    }
  }

  activate_editor_option_pages ();
}

void LayoutView::side_panel_destroyed (QObject *sender)
{
  if (sender == mp_control_frame) {
    mp_control_frame = 0;
    mp_control_panel = 0;
  } else if (sender == mp_hierarchy_frame) {
    mp_hierarchy_frame = 0;
    mp_hierarchy_panel = 0;
  } else if (sender == mp_libraries_frame) {
    mp_libraries_frame = 0;
    mp_libraries_view = 0;
  } else if (sender == mp_editor_options_frame) {
    mp_editor_options_frame = 0;
  } else if (sender == mp_bookmarks_frame) {
    mp_bookmarks_frame = 0;
    mp_bookmarks_view = 0;
  } else if (sender == mp_toolbox_frame) {
    mp_toolbox_frame = 0;
    mp_toolbox = 0;
  }
}

void LayoutView::set_current ()
{
  set_current (this);
}

void LayoutView::set_current (lay::LayoutView *view)
{
  if (ms_current != view) {
    if (ms_current) {
      ms_current->deactivate ();
    }
    ms_current = view;
    if (ms_current) {
      ms_current->activate ();
    }
  }
}

LayoutView *LayoutView::current ()
{
  return ms_current;
}

void LayoutView::create_plugins (const lay::PluginDeclaration *except_this)
{
  LayoutViewBase::create_plugins (except_this);
  dm_setup_editor_option_pages ();
}

namespace {

class GotoBookmarkAction
  : public lay::Action
{
public:
  GotoBookmarkAction (lay::LayoutView *view, size_t id, const std::string &title)
    : Action (), mp_view (view), m_id (id)
  {
    set_title (title);
  }

  void triggered ()
  {
    if (mp_view) {
      mp_view->goto_view (mp_view->bookmarks ().state (m_id));
    }
  }

private:
  tl::weak_ptr<lay::LayoutView> mp_view;
  size_t m_id;
};

}

void
LayoutView::update_menu (lay::LayoutView *view, lay::AbstractMenu &menu)
{
  std::string bm_menu = "bookmark_menu.goto_bookmark_menu";

  if (menu.is_valid (bm_menu)) {

    menu.clear_menu (bm_menu);

    Action *goto_bookmark_action = menu.action (bm_menu);

    if (view && view->bookmarks ().size () > 0) {

      goto_bookmark_action->set_enabled (true);

      const lay::BookmarkList &bookmarks = view->bookmarks ();
      for (size_t i = 0; i < bookmarks.size (); ++i) {
        Action *action = new GotoBookmarkAction (view, i, bookmarks.name (i));
        menu.insert_item (bm_menu + ".end", tl::sprintf ("bookmark_%d", i + 1), action);
      }

    } else {
      goto_bookmark_action->set_enabled (false);
    }

  }
}

bool 
LayoutView::configure (const std::string &name, const std::string &value)
{
  if (name == cfg_bitmap_oversampling) {

    int os = 1;
    tl::from_string (value, os);
    if (mp_control_panel) {
      mp_control_panel->set_oversampling (os);
    }

  } else if (name == cfg_highres_mode) {

    bool hrm = false;
    tl::from_string (value, hrm);
    if (mp_control_panel) {
      mp_control_panel->set_highres_mode (hrm);
    }

  }

  if (LayoutViewBase::configure (name, value)) {
    return true;
  }

  if (name == cfg_flat_cell_list) {

    bool f;
    tl::from_string (value, f);
    if (mp_hierarchy_panel) {
      mp_hierarchy_panel->set_flat (f);
    }
    return true;

  } else if (name == cfg_split_cell_list) {

    bool f;
    tl::from_string (value, f);
    if (mp_hierarchy_panel) {
      mp_hierarchy_panel->set_split_mode (f);
    }
    return true;

  } else if (name == cfg_split_lib_views) {

    bool f;
    tl::from_string (value, f);
    if (mp_libraries_view) {
      mp_libraries_view->set_split_mode (f);
    }
    return true;

  } else if (name == cfg_bookmarks_follow_selection) {

    bool f;
    tl::from_string (value, f);
    if (mp_bookmarks_view) {
      mp_bookmarks_view->follow_selection (f);
    }
    return true;

  } else if (name == cfg_current_lib_view) {

    if (mp_libraries_view) {
      mp_libraries_view->select_active_lib_by_name (value);
    }
    return true;

  } else if (name == cfg_cell_list_sorting) {

    if (mp_hierarchy_panel) {
      if (value == "by-name") {
        mp_hierarchy_panel->set_sorting (CellTreeModel::ByName);
      } else if (value == "by-area") {
        mp_hierarchy_panel->set_sorting (CellTreeModel::ByArea);
      } else if (value == "by-area-reverse") {
        mp_hierarchy_panel->set_sorting (CellTreeModel::ByAreaReverse);
      }
    }
    return true;

  } else if (name == cfg_hide_empty_layers) {

    bool f;
    tl::from_string (value, f);
    if (mp_control_panel) {
      mp_control_panel->set_hide_empty_layers (f);
    }
    return true;

  } else if (name == cfg_test_shapes_in_view) {

    bool f;
    tl::from_string (value, f);
    if (mp_control_panel) {
      mp_control_panel->set_test_shapes_in_view (f);
    }
    return true;

  } else if (name == cfg_layers_always_show_source) {

    bool a = false;
    tl::from_string (value, a);
    if (a != m_always_show_source) {
      m_always_show_source = a;
      layer_list_changed_event (4);
    }

    return true;

  } else if (name == cfg_layers_always_show_ld) {

    bool a = false;
    tl::from_string (value, a);
    if (a != m_always_show_ld) {
      m_always_show_ld = a;
      layer_list_changed_event (4);
    }

    return true;

  } else if (name == cfg_layers_always_show_layout_index) {

    bool a = false;
    tl::from_string (value, a);
    if (a != m_always_show_layout_index) {
      m_always_show_layout_index = a;
      layer_list_changed_event (4);
    }

    return true;

  } else if (name == cfg_stipple_palette) {

    lay::StipplePalette palette = lay::StipplePalette::default_palette ();

    try {
      //  empty string means: default palette
      if (! value.empty ()) {
        palette.from_string (value);
      }
    } catch (...) {
      //  ignore errors: just reset the palette
      palette = lay::StipplePalette::default_palette ();
    }

    if (mp_toolbox) {
      mp_toolbox->set_palette (palette);
    }

    // others need this property too ..
    return false;

  } else if (name == cfg_line_style_palette) {

    lay::LineStylePalette palette = lay::LineStylePalette::default_palette ();

    try {
      //  empty string means: default palette
      if (! value.empty ()) {
        palette.from_string (value);
      }
    } catch (...) {
      //  ignore errors: just reset the palette
      palette = lay::LineStylePalette::default_palette ();
    }

    if (mp_toolbox) {
      mp_toolbox->set_palette (palette);
    }

    // others need this property too ..
    return false;

  } else if (name == cfg_color_palette) {

    lay::ColorPalette palette = lay::ColorPalette::default_palette ();

    try {
      //  empty string means: default palette
      if (! value.empty ()) {
        palette.from_string (value);
      }
    } catch (...) {
      //  ignore errors: just reset the palette
      palette = lay::ColorPalette::default_palette ();
    }

    if (mp_toolbox) {
      mp_toolbox->set_palette (palette);
    }

    // others need this property too ..
    return false;

  } else {
    return false;
  }
}

void
LayoutView::config_finalize ()
{
  //  It's important that the editor option pages are updated last - because the
  //  configuration change may trigger other configuration changes
  dm_setup_editor_option_pages ();
}

void
LayoutView::set_current_layer (const lay::LayerPropertiesConstIterator &l) 
{
  if (mp_control_panel) {
    mp_control_panel->set_current_layer (l);
  } else {
    return LayoutViewBase::set_current_layer (l);
  }
}

lay::LayerPropertiesConstIterator
LayoutView::current_layer () const
{
  if (mp_control_panel) {
    return mp_control_panel->current_layer ();
  } else {
    return LayoutViewBase::current_layer ();
  }
}

std::vector<lay::LayerPropertiesConstIterator> 
LayoutView::selected_layers () const
{
  if (mp_control_panel) {
    return mp_control_panel->selected_layers ();
  } else {
    return LayoutViewBase::selected_layers ();
  }
}

void 
LayoutView::set_selected_layers (const std::vector<lay::LayerPropertiesConstIterator> &sel) 
{
  if (mp_control_panel) {
    mp_control_panel->set_selection (sel);
  } else {
    LayoutViewBase::set_selected_layers (sel);
  }
}

void
LayoutView::begin_layer_updates ()
{
  if (mp_control_panel) {
    mp_control_panel->begin_updates ();
  } else {
    LayoutViewBase::begin_layer_updates ();
  }
}

void
LayoutView::end_layer_updates ()
{
  if (mp_control_panel) {
    mp_control_panel->end_updates ();
  } else {
    LayoutViewBase::end_layer_updates ();
  }
}

bool
LayoutView::layer_model_updated ()
{
  //  because check_updated is called in the initialization phase, we check if the pointers
  //  to the widgets are non-null:
  if (mp_control_panel) {
    return mp_control_panel->model_updated ();
  } else {
    return LayoutViewBase::layer_model_updated ();
  }
}

void
LayoutView::bookmark_current_view ()
{
  if (! mp_widget) {
    return;
  }

  QString proposed_name = tl::to_qstring (bookmarks ().propose_new_bookmark_name ());

  while (true) {
    bool ok = false;
    QString text = QInputDialog::getText (mp_widget, QObject::tr ("Enter Bookmark Name"), QObject::tr ("Bookmark name"),
                                          QLineEdit::Normal, proposed_name, &ok);
    if (! ok) {
      break;
    } else if (text.isEmpty ()) {
      QMessageBox::critical (mp_widget, QObject::tr ("Error"), QObject::tr ("Enter a name for the bookmark"));
    } else {
      bookmark_view (tl::to_string (text));
      break;
    }
  }
}

void
LayoutView::manage_bookmarks ()
{
  if (! mp_widget) {
    return;
  }

  std::set<size_t> selected_bm;
  if (mp_bookmarks_frame->isVisible ()) {
    selected_bm = mp_bookmarks_view->selected_bookmarks ();
  }

  BookmarkManagementForm dialog (mp_widget, "bookmark_form", bookmarks (), selected_bm);
  if (dialog.exec ()) {
    bookmarks (dialog.bookmarks ());
  }
}

void
LayoutView::bookmarks_changed ()
{
  mp_bookmarks_view->refresh ();
  if (mp_widget) {
    mp_widget->emit_menu_needs_update ();
  }
}

void
LayoutView::layer_tab_changed ()
{
  update_content ();
}

void 
LayoutView::layer_order_changed ()
{
  update_content ();
}

void 
LayoutView::min_hier_changed (int i)
{
  mp_max_hier_spbx->setMinimum (i);
  set_hier_levels (std::make_pair (i, get_hier_levels ().second));
}

void 
LayoutView::max_hier_changed (int i)
{
  mp_min_hier_spbx->setMaximum (i);
  set_hier_levels (std::make_pair (get_hier_levels ().first, i));
}

tl::Color
LayoutView::default_background_color ()
{
  if (! mp_widget) {
    return LayoutViewBase::default_background_color ();
  } else {
    return tl::Color (mp_widget->palette ().color (QPalette::Normal, QPalette::Base).rgb ());
  }
}

void 
LayoutView::do_set_background_color (tl::Color c, tl::Color contrast)
{
  if (mp_control_panel) {
    mp_control_panel->set_background_color (c);
    mp_control_panel->set_text_color (contrast);
  }

  if (mp_hierarchy_panel) {
    mp_hierarchy_panel->set_background_color (c);
    mp_hierarchy_panel->set_text_color (contrast);
  }

  if (mp_libraries_view) {
    mp_libraries_view->set_background_color (c);
    mp_libraries_view->set_text_color (contrast);
  }

  if (mp_bookmarks_view) {
    mp_bookmarks_view->set_background_color (c);
    mp_bookmarks_view->set_text_color (contrast);
  }
}

void
LayoutView::do_set_no_stipples (bool no_stipples)
{
  if (mp_control_panel) {
    mp_control_panel->set_no_stipples (no_stipples);
  }
}

void
LayoutView::do_set_phase (int phase)
{
  if (mp_control_panel) {
    mp_control_panel->set_phase (phase);
  }
}

void
LayoutView::active_library_changed (int /*index*/)
{
  std::string lib_name;
  if (mp_libraries_view->active_lib ()) {
    lib_name = mp_libraries_view->active_lib ()->get_name ();
  }

  //  commit the new active library to the other views and persist this state
  //  TODO: could be passed through the LibraryController (like through some LibraryController::active_library)
  dispatcher ()->config_set (cfg_current_lib_view, lib_name);
}

bool
LayoutView::set_hier_levels_basic (std::pair<int, int> l)
{
  if (l != get_hier_levels ()) {

    if (mp_min_hier_spbx) {
      mp_min_hier_spbx->blockSignals (true);
      mp_min_hier_spbx->setValue (l.first);
      mp_min_hier_spbx->setMaximum (l.second);
      mp_min_hier_spbx->blockSignals (false);
    }

    if (mp_max_hier_spbx) {
      mp_max_hier_spbx->blockSignals (true);
      mp_max_hier_spbx->setValue (l.second);
      mp_max_hier_spbx->setMinimum (l.first);
      mp_max_hier_spbx->blockSignals (false);
    }

    return LayoutViewBase::set_hier_levels_basic (l);

  } else {
    return false;
  }
}

bool
LayoutView::has_selection ()
{
  if (mp_control_panel && mp_control_panel->has_focus ()) {
    return mp_control_panel->has_selection ();
  } else if (mp_hierarchy_panel && mp_hierarchy_panel->has_focus ()) {
    return mp_hierarchy_panel->has_selection ();
  } else {
    return lay::LayoutViewBase::has_selection ();
  }
}

void
LayoutView::do_paste ()
{
  //  let the receivers sort out who is pasting what ..
  if (mp_hierarchy_panel) {
    mp_hierarchy_panel->paste ();
  }
  if (mp_control_panel) {
    mp_control_panel->paste ();
  }
}

void
LayoutView::copy ()
{
  if (mp_hierarchy_panel && mp_hierarchy_panel->has_focus ()) {
    mp_hierarchy_panel->copy ();
  } else if (mp_control_panel && mp_control_panel->has_focus ()) {
    mp_control_panel->copy ();
  } else {
    LayoutViewBase::copy ();
  }
}

void
LayoutView::cut ()
{
  if (mp_hierarchy_panel && mp_hierarchy_panel->has_focus ()) {
    //  TODO: currently the hierarchy panel's cut function does its own transaction handling.
    //  Otherwise the cut function is not working propertly.
    mp_hierarchy_panel->cut ();
  } else if (mp_control_panel && mp_control_panel->has_focus ()) {
    db::Transaction trans (manager (), tl::to_string (QObject::tr ("Cut Layers")));
    mp_control_panel->cut ();
  } else {
    LayoutViewBase::cut ();
  }
}

int
LayoutView::active_cellview_index () const
{
  if (mp_hierarchy_panel) {
    return mp_hierarchy_panel->active ();
  } else {
    return LayoutViewBase::active_cellview_index ();
  }
}

void 
LayoutView::set_active_cellview_index (int index) 
{
  if (index >= 0 && index < int (cellviews ())) {
    if (mp_hierarchy_panel) {
      mp_hierarchy_panel->select_active (index);
    }
    LayoutViewBase::set_active_cellview_index (index);
  }
}

void 
LayoutView::selected_cells_paths (int cv_index, std::vector<cell_path_type> &paths) const
{
  if (mp_hierarchy_panel) {
    mp_hierarchy_panel->selected_cells (cv_index, paths);
  } else {
    LayoutViewBase::selected_cells_paths (cv_index, paths);
  }
}

void
LayoutView::current_cell_path (int cv_index, cell_path_type &path) const
{
  if (mp_hierarchy_panel) {
    mp_hierarchy_panel->current_cell (cv_index, path);
  } else {
    LayoutViewBase::current_cell_path (cv_index, path);
  }
}

void
LayoutView::set_current_cell_path (int cv_index, const cell_path_type &path) 
{
  if (mp_hierarchy_panel) {
    mp_hierarchy_panel->set_current_cell (cv_index, path);
  } else {
    LayoutViewBase::set_current_cell_path (cv_index, path);
  }
}

void
LayoutView::cancel_edits ()
{
  //  close the property dialog
  if (mp_properties_dialog) {
    mp_properties_dialog->hide ();
  }

  LayoutViewBase::cancel_edits ();
}

void
LayoutView::activate ()
{
  if (! m_activated) {
    for (std::vector<lay::Plugin *>::const_iterator p = plugins ().begin (); p != plugins ().end (); ++p) {
      if ((*p)->browser_interface () && (*p)->browser_interface ()->active ()) {
        (*p)->browser_interface ()->show ();
      }
    }
    mp_timer->start (timer_interval);
    m_activated = true;
    update_content ();
  }
}

void
LayoutView::deactivate ()
{
  for (std::vector<lay::Plugin *>::const_iterator p = plugins ().begin (); p != plugins ().end (); ++p) {
    if ((*p)->browser_interface ()) {
      (*p)->browser_interface ()->hide ();
    }
  }

  if (mp_widget) {
    mp_widget->emit_clear_current_pos ();
  }

  free_resources ();
  mp_timer->stop ();
  m_activated = false;
}

bool
LayoutView::is_activated () const
{
  return m_activated;
}

void
LayoutView::deactivate_all_browsers ()
{
  for (std::vector<lay::Plugin *>::const_iterator p = plugins ().begin (); p != plugins ().end (); ++p) {
    if ((*p)->browser_interface ()) {
      (*p)->browser_interface ()->deactivate ();
    }
  }
}

void
LayoutView::update_content_for_cv (int cellview_index)
{
  if (mp_hierarchy_panel) {
    mp_hierarchy_panel->do_update_content (cellview_index);
  }
}

void
LayoutView::current_pos (double x, double y)
{
  if (! mp_widget) {
    return;
  }

  if (m_activated) {
    if (dbu_coordinates ()) {
      double dx = 0.0, dy = 0.0;
      if (active_cellview_index () >= 0) {
        double dbu = cellview (active_cellview_index ())->layout ().dbu ();
        dx = x / dbu;
        dy = y / dbu;
      }
      mp_widget->emit_current_pos_changed (dx, dy, true);
    } else {
      mp_widget->emit_current_pos_changed (x, y, false);
    }
  }
}

void
LayoutView::emit_edits_enabled_changed ()
{
  if (mp_widget) {
    mp_widget->emit_edits_enabled_changed ();
  }
}

void
LayoutView::emit_title_changed ()
{
  if (mp_widget) {
    mp_widget->emit_title_changed (this);
  }
}

void
LayoutView::emit_dirty_changed ()
{
  if (mp_widget) {
    mp_widget->emit_dirty_changed (this);
  }
}

void
LayoutView::emit_layer_order_changed ()
{
  if (mp_widget) {
    mp_widget->emit_layer_order_changed ();
  }
}

void
LayoutView::signal_selection_changed ()
{
  if (selection_size () > 1) {
    message (tl::sprintf (tl::to_string (tr ("selected: %ld objects")), selection_size ()));
  }

  lay::Editables::signal_selection_changed ();
}

void
LayoutView::message (const std::string &s, int timeout)
{
  if (mp_widget) {
    mp_widget->emit_show_message (s, timeout * 1000);
  }
}

void
LayoutView::mode (int m)
{
  if (mode () != m) {
    LayoutViewBase::mode (m);
    activate_editor_option_pages ();
  }
}

void
LayoutView::activate_editor_option_pages ()
{
  lay::EditorOptionsPages *eo_pages = editor_options_pages ();
  if (eo_pages) {

    //  TODO: this is very inefficient as each "activate" will regenerate the tabs
    for (std::vector<lay::EditorOptionsPage *>::const_iterator op = eo_pages->pages ().begin (); op != eo_pages->pages ().end (); ++op) {
      bool is_active = false;
      if ((*op)->plugin_declaration () == 0) {
        is_active = true;
      } else if (active_plugin () && active_plugin ()->plugin_declaration () == (*op)->plugin_declaration ()) {
        is_active = true;
      }
      (*op)->activate (is_active);
    }

  }
}

void
LayoutView::switch_mode (int m)
{
  if (mode () != m) {
    mode (m);
    if (mp_widget) {
      mp_widget->emit_mode_change (m);
    }
  }
}

void 
LayoutView::open_l2ndb_browser (int l2ndb_index, int cv_index)
{
  lay::NetlistBrowserDialog *l2ndb_browser = get_plugin <lay::NetlistBrowserDialog> ();
  if (l2ndb_browser) {
    l2ndb_browser->load (l2ndb_index, cv_index);
  }
}

void
LayoutView::open_rdb_browser (int rdb_index, int cv_index)
{
  rdb::MarkerBrowserDialog *rdb_browser = get_plugin <rdb::MarkerBrowserDialog> ();
  if (rdb_browser) {
    rdb_browser->load (rdb_index, cv_index);
  }
}

QSize
LayoutView::size_hint () const
{
  if ((options () & LV_Naked) != 0) {
    return QSize (200, 200);
  } else if ((options () & LV_NoLayers) != 0 || (options () & LV_NoHierarchyPanel) != 0 || (options () & LV_NoLibrariesView) != 0) {
    return QSize (400, 200);
  } else {
    return QSize (600, 200);
  }
}

} // namespace lay

#endif

