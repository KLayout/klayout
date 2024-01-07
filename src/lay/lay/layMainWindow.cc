
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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


#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QInputDialog>
#include <QMessageBox>
#include <QMainWindow>
#include <QPainter>
#include <QDockWidget>
#include <QShortcut>
#include <QPrintDialog>
#include <QFileInfo>
#include <QDateTime>
#include <QUrl>
#include <QMimeData>
#include <QClipboard>
#if QT_VERSION >= 0x050000
#  include <QGuiApplication>
#endif

#if defined(__APPLE__) && (QT_VERSION < 0x050401)
// A workaround for the issue of Qt 4.8.x when handling "File Reference URL" in OSX
// By Kazunari Sekigawa (November 12, 2015)
// Search down for my name for more details!
# include <CoreFoundation/CoreFoundation.h>
#endif

#include "tlInternational.h"
#include "tlTimer.h"
#include "tlLog.h"
#include "tlAssert.h"
#include "tlStream.h"
#include "tlExceptions.h"
#include "tlExpression.h"
#include "tlFileUtils.h"
#include "tlUri.h"
#include "dbMemStatistics.h"
#include "dbManager.h"
#include "dbStream.h"
#include "dbSaveLayoutOptions.h"
#include "dbClipboard.h"
#include "dbLibraryManager.h"
#include "dbLibrary.h"
#include "dbStatic.h"
#include "dbInit.h"
#include "edtConfig.h"
#include "laySession.h"
#include "layApplication.h"
#include "layVersion.h"
#include "layConverters.h"
#include "layDialogs.h"
#include "laybasicConfig.h"
#include "layConfig.h"
#include "layEnhancedTabBar.h"
#include "layMainWindow.h"
#include "layHelpDialog.h"
#include "layNavigator.h"
#include "layProgress.h"
#include "layProgressDialog.h"
#include "layProgressWidget.h"
#include "layStream.h"
#include "layLayerControlPanel.h" // because of LabelWithBackground
#include "layFileDialog.h"
#include "layMainConfigPages.h"
#include "layAbstractMenu.h"
#include "layQtTools.h"
#include "laySaveLayoutOptionsDialog.h"
#include "layLoadLayoutOptionsDialog.h"
#include "layLogViewerDialog.h"
#include "layLayerToolbox.h"
#include "laySettingsForm.h"
#include "laySelectCellViewForm.h"
#include "layTechnologyController.h"
#include "laySaltController.h"
#include "layTipDialog.h"
#include "layMacroController.h"
#include "layHelpAboutDialog.h"
#include "layControlWidgetStack.h"
#include "layViewWidgetStack.h"
#include "layEditorOptionsPages.h"
#include "layInit.h"
#include "antObject.h"
#include "antService.h"
#include "gsi.h"
#include "gsiInterpreter.h"
#include "gtf.h"

namespace lay
{

const int max_dirty_files = 15;

// -------------------------------------------------------------

static MainWindow *mw_instance = 0;

MainWindow *
MainWindow::instance ()
{
  return mw_instance;
}

// -------------------------------------------------------------

static void
show_dock_widget (QDockWidget *dock_widget, bool visible)
{
  if (visible) {

    dock_widget->show ();
    dock_widget->setFocus ();

    //  NOTE: this is a clumsy way to make sure the dock widget is made the current tab if it's in a tabbed dock
    //  TODO: is there a better way to do this?
    QMainWindow *main_window = dynamic_cast<QMainWindow *> (dock_widget->parent ());
    if (! main_window) {
      return;
    }

    //  Look up all children of the main window and find the QTabBars. These are the dock tabs (we don't create others).
    //  Inside these, look up the right tab by checking the titles.
    QList<QObject *> mw_children = main_window->children ();
    for (QList<QObject *>::const_iterator i = mw_children.begin (); i != mw_children.end (); ++i) {
      QTabBar *tab_bar = dynamic_cast<QTabBar *> (*i);
      if (tab_bar) {
        for (int j = 0; j < tab_bar->count (); ++j) {
          if (tab_bar->tabText (j) == dock_widget->windowTitle ()) {
            tab_bar->setCurrentIndex (j);
            return;
          }
        }
      }
    }

  } else {
    dock_widget->hide ();
  }
}

// -------------------------------------------------------------

MainWindow::MainWindow (QApplication *app, const char *name, bool undo_enabled)
    : QMainWindow (0),
      tl::Object (),
      lay::DispatcherDelegate (),
      m_dispatcher (this),
      m_text_progress (this, 10 /*verbosity threshold*/),
      m_mode (std::numeric_limits<unsigned int>::max ()),
      mp_setup_form (0),
      m_open_mode (0),
      m_keep_backups (0),
      m_disable_tab_selected (false),
      m_exited (false),
      dm_do_update_menu (this, &MainWindow::do_update_menu),
      dm_do_update_mru_menus (this, &MainWindow::do_update_mru_menus),
      dm_exit (this, &MainWindow::exit),
      m_grid_micron (0.001),
      m_default_grids_updated (true),
      m_new_layout_current_panel (false),
      m_synchronized_views (false),
      m_synchronous (false),
      m_busy (false),
      mp_app (app),
      m_manager (undo_enabled)
{
  m_dispatcher.set_menu_parent_widget (this);
  m_dispatcher.make_menu ();

  //  ensures the deferred method scheduler is present
  tl::DeferredMethodScheduler::instance ();

  setObjectName (QString::fromUtf8 (name));

  if (mw_instance != 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("Only one instance of MainWindow may be created")));
  }
  mw_instance = this;

  lay::register_help_handler (this, SLOT (show_help (const QString &)), SLOT (show_modal_help (const QString &)));

  mp_setup_form = new SettingsForm (0, dispatcher (), "setup_form"),

  db::LibraryManager::instance ().changed_event.add (this, &MainWindow::libraries_changed);

  init_menu ();

  mp_assistant = 0;

  m_always_exit_without_saving = false;

  mp_pr = new lay::ProgressReporter ();
  mp_pr->set_progress_bar (&m_text_progress);

  mp_main_stack_widget = new QStackedWidget (this);
  mp_main_stack_widget->setObjectName (QString::fromUtf8 ("main_stack"));
  setCentralWidget (mp_main_stack_widget);

  mp_main_frame = new QFrame (mp_main_stack_widget);
  mp_main_frame->setObjectName (QString::fromUtf8 ("main_frame"));
  mp_main_stack_widget->addWidget (mp_main_frame);

  mp_progress_widget = new ProgressWidget (mp_pr, mp_main_stack_widget);
  mp_progress_widget->setObjectName (QString::fromUtf8 ("progress"));
  mp_main_stack_widget->addWidget (mp_progress_widget);

  mp_main_stack_widget->setCurrentIndex (0);

  QVBoxLayout *vbl = new QVBoxLayout (mp_main_frame);
  vbl->setContentsMargins (0, 0, 0, 0);
  vbl->setSpacing (0);

  QHBoxLayout *vbh_tab = new QHBoxLayout ();
  vbh_tab->setSpacing (6);
  vbl->addLayout (vbh_tab);

  EnhancedTabBar *enh_tab_widget = new EnhancedTabBar (mp_main_frame);
  mp_tab_bar = enh_tab_widget;
  mp_tab_bar->installEventFilter (this);
  vbh_tab->addWidget (enh_tab_widget);
  vbh_tab->addWidget (enh_tab_widget->menu_button ());

  connect (mp_tab_bar, SIGNAL (currentChanged (int)), this, SLOT (view_selected (int)));
#if QT_VERSION >= 0x040500
  mp_tab_bar->setTabsClosable (true);
  connect (mp_tab_bar, SIGNAL (tabCloseRequested (int)), this, SLOT (tab_close_requested (int)));
#endif

  mp_tab_bar->setContextMenuPolicy (Qt::ActionsContextMenu);

  QAction *action = new QAction (tr ("Close All"), this);
  connect (action, SIGNAL (triggered ()), this, SLOT (close_all_views ()));
  mp_tab_bar->addAction (action);
  action = new QAction (tr ("Close All Except This"), this);
  connect (action, SIGNAL (triggered ()), this, SLOT (close_all_except_this ()));
  mp_tab_bar->addAction (action);
  action = new QAction (tr ("Close All Left"), this);
  connect (action, SIGNAL (triggered ()), this, SLOT (close_all_views_left ()));
  mp_tab_bar->addAction (action);
  action = new QAction (tr ("Close All Right"), this);
  connect (action, SIGNAL (triggered ()), this, SLOT (close_all_views_right ()));
  mp_tab_bar->addAction (action);
  action = new QAction (this);
  action->setSeparator (true);
  mp_tab_bar->addAction (action);
  action = new QAction (tr ("Clone Panel"), this);
  connect (action, SIGNAL (triggered ()), this, SLOT (clone ()));
  mp_tab_bar->addAction (action);

  mp_hp_dock_widget = new QDockWidget (QObject::tr ("Cells"), this);
  mp_hp_dock_widget->setObjectName (QString::fromUtf8 ("hp_dock_widget"));
  mp_hp_stack = new ControlWidgetStack (mp_hp_dock_widget, "hp_stack");
  mp_hp_dock_widget->setWidget (mp_hp_stack);
  mp_hp_dock_widget->setFocusProxy (mp_hp_stack);
  connect (mp_hp_dock_widget, SIGNAL (visibilityChanged (bool)), this, SLOT (dock_widget_visibility_changed (bool)));
  m_hp_visible = true;

  mp_libs_dock_widget = new QDockWidget (QObject::tr ("Libraries"), this);
  mp_libs_dock_widget->setObjectName (QString::fromUtf8 ("libs_dock_widget"));
  mp_libs_stack = new ControlWidgetStack (mp_libs_dock_widget, "libs_stack");
  mp_libs_dock_widget->setWidget (mp_libs_stack);
  mp_libs_dock_widget->setFocusProxy (mp_libs_stack);
  connect (mp_libs_dock_widget, SIGNAL (visibilityChanged (bool)), this, SLOT (dock_widget_visibility_changed (bool)));
  m_libs_visible = true;

  mp_eo_dock_widget = new QDockWidget (QObject::tr ("Editor Options"), this);
  mp_eo_dock_widget->setObjectName (QString::fromUtf8 ("eo_dock_widget"));
  mp_eo_dock_widget->setMinimumHeight (150);
  mp_eo_stack = new ControlWidgetStack (mp_eo_dock_widget, "eo_stack");
  mp_eo_dock_widget->setWidget (mp_eo_stack);
  mp_eo_dock_widget->setFocusProxy (mp_eo_stack);
  connect (mp_eo_dock_widget, SIGNAL (visibilityChanged (bool)), this, SLOT (dock_widget_visibility_changed (bool)));
  m_eo_visible = true;

  mp_bm_dock_widget = new QDockWidget (QObject::tr ("Bookmarks"), this);
  mp_bm_dock_widget->setObjectName (QString::fromUtf8 ("bookmarks_dock_widget"));
  mp_bm_stack = new ControlWidgetStack (mp_bm_dock_widget, "bookmarks_stack");
  mp_bm_dock_widget->setWidget (mp_bm_stack);
  mp_bm_dock_widget->setFocusProxy (mp_bm_stack);
  connect (mp_bm_dock_widget, SIGNAL (visibilityChanged (bool)), this, SLOT (dock_widget_visibility_changed (bool)));
  m_bm_visible = true;

  mp_view_stack = new ViewWidgetStack (mp_main_frame);
  mp_view_stack->setObjectName (QString::fromUtf8 ("view_stack"));
  vbl->addWidget (mp_view_stack);

  mp_layer_toolbox_dock_widget = new QDockWidget (QObject::tr ("Layer Toolbox"), this);
  mp_layer_toolbox_dock_widget->setObjectName (QString::fromUtf8 ("lt_dock_widget"));
  mp_layer_toolbox_stack = new ControlWidgetStack (mp_layer_toolbox_dock_widget, "layer_toolbox_stack", true);
  mp_layer_toolbox_dock_widget->setWidget (mp_layer_toolbox_stack);
  mp_layer_toolbox_dock_widget->setFocusProxy (mp_layer_toolbox_stack);
  connect (mp_layer_toolbox_dock_widget, SIGNAL (visibilityChanged (bool)), this, SLOT (dock_widget_visibility_changed (bool)));
  m_layer_toolbox_visible = true;

  mp_lp_dock_widget = new QDockWidget (QObject::tr ("Layers"), this);
  mp_lp_dock_widget->setObjectName (QString::fromUtf8 ("lp_dock_widget"));
  mp_lp_stack = new ControlWidgetStack (mp_lp_dock_widget, "lp_stack");
  mp_lp_dock_widget->setWidget (mp_lp_stack);
  mp_lp_dock_widget->setFocusProxy (mp_lp_stack);
  connect (mp_lp_dock_widget, SIGNAL (visibilityChanged (bool)), this, SLOT (dock_widget_visibility_changed (bool)));
  m_lp_visible = true;

  mp_navigator_dock_widget = new QDockWidget (QObject::tr ("Navigator"), this);
  mp_navigator_dock_widget->setObjectName (QString::fromUtf8 ("navigator_dock_widget"));
  mp_navigator = new Navigator (this);
  mp_navigator_dock_widget->setWidget (mp_navigator);
  mp_navigator_dock_widget->setFocusProxy (mp_navigator);
  connect (mp_navigator_dock_widget, SIGNAL (visibilityChanged (bool)), this, SLOT (dock_widget_visibility_changed (bool)));
  m_navigator_visible = true;

  //  Add dock widgets
#if QT_VERSION >= 0x040500
  setTabPosition (Qt::AllDockWidgetAreas, QTabWidget::North);
#endif
  addDockWidget(Qt::LeftDockWidgetArea, mp_navigator_dock_widget);
  addDockWidget(Qt::LeftDockWidgetArea, mp_hp_dock_widget);
  addDockWidget(Qt::LeftDockWidgetArea, mp_libs_dock_widget);
  addDockWidget(Qt::LeftDockWidgetArea, mp_eo_dock_widget);
  addDockWidget(Qt::RightDockWidgetArea, mp_bm_dock_widget);
  addDockWidget(Qt::RightDockWidgetArea, mp_lp_dock_widget);
  addDockWidget(Qt::RightDockWidgetArea, mp_layer_toolbox_dock_widget);

  mp_tool_bar = new QToolBar (this);
  mp_tool_bar->setWindowTitle (QObject::tr ("Toolbar"));
  mp_tool_bar->setObjectName (QString::fromUtf8 ("toolbar"));
  mp_tool_bar->setMovable (false);
  mp_tool_bar->setToolButtonStyle (Qt::ToolButtonTextUnderIcon);
  addToolBar (Qt::TopToolBarArea, mp_tool_bar);

  QMenuBar *mbar = menuBar ();
  mbar->setObjectName (QString::fromUtf8 ("menubar"));

  menu ()->build (mbar, mp_tool_bar);

  connect (menu (), SIGNAL (changed ()), this, SLOT (menu_changed ()));

  mp_status_bar = statusBar ();
  mp_status_bar->setObjectName (QString::fromUtf8 ("status_bar"));

  QLabel *tech_status_icon = new QLabel (mp_status_bar);
  tech_status_icon->setText(QString::fromUtf8 ("<html>&nbsp;<b>T</b></html>"));
  mp_status_bar->addWidget (tech_status_icon);

  mp_tech_status_label = new QLabel (mp_status_bar);
  mp_tech_status_label->setObjectName (QString::fromUtf8 ("tech_status_label"));
  mp_tech_status_label->setMinimumSize (QSize (100, 0));
  mp_tech_status_label->setToolTip (QObject::tr ("Current technology"));
  mp_status_bar->addWidget (mp_tech_status_label);

  QLabel *sel_status_icon = new QLabel (mp_status_bar);
  sel_status_icon->setText(QString::fromUtf8 ("<html><b>&nbsp;&nbsp;G</b></html>"));
  mp_status_bar->addWidget (sel_status_icon);

  mp_msg_label = new QLabel (mp_status_bar);
  mp_msg_label->setObjectName (QString::fromUtf8 ("msg_label"));
  mp_msg_label->setToolTip (QObject::tr ("General status"));
  mp_status_bar->addWidget (mp_msg_label, 1);

  QLabel *xy_status_icon = new QLabel (mp_status_bar);
  xy_status_icon->setText(QString::fromUtf8 ("<html><b>&nbsp;&nbsp;xy</b></html>"));
  mp_status_bar->addWidget (xy_status_icon);

  mp_cp_frame = new QFrame (mp_status_bar);
  mp_status_bar->addWidget (mp_cp_frame);

  QHBoxLayout *cp_frame_ly = new QHBoxLayout (mp_cp_frame);
  cp_frame_ly->setContentsMargins (0, 0, 0, 0);
  cp_frame_ly->setSpacing (0);
  mp_cpx_label = new QLabel (mp_cp_frame);
  mp_cpx_label->setObjectName (QString::fromUtf8 ("cpx_label"));
  mp_cpx_label->setAlignment (Qt::AlignVCenter | Qt::AlignRight);
  mp_cpx_label->setMinimumSize (100, 0);
  mp_cpx_label->setToolTip (QObject::tr ("Current cursor position (x)"));
  cp_frame_ly->addWidget (mp_cpx_label);
  cp_frame_ly->insertSpacing (-1, 6);
  mp_cpy_label = new QLabel (mp_cp_frame);
  mp_cpy_label->setObjectName (QString::fromUtf8 ("cpy_label"));
  mp_cpy_label->setAlignment (Qt::AlignVCenter | Qt::AlignRight);
  mp_cpy_label->setMinimumSize (100, 0);
  mp_cpy_label->setToolTip (QObject::tr ("Current cursor position (y)"));
  cp_frame_ly->addWidget (mp_cpy_label);
  cp_frame_ly->insertSpacing (-1, 6);

  //  select the default mode
  select_mode (lay::LayoutView::default_mode ());

  //  create file dialogs:

  //  session file dialog
  mp_session_fdia = new lay::FileDialog (this,
                          tl::to_string (QObject::tr ("Session File")),
                          tl::to_string (QObject::tr ("Session files (*.lys);;All files (*)")),
                          "lys");

  //  bookmarks file dialog
  mp_bookmarks_fdia = new lay::FileDialog (this,
                            tl::to_string (QObject::tr ("Bookmarks File")),
                            tl::to_string (QObject::tr ("Bookmark files (*.lyb);;All files (*)")),
                            "lyb");
  //  layer properties
  mp_lprops_fdia = new lay::FileDialog (this,
                            tl::to_string (QObject::tr ("Layer Properties File")),
                            tl::to_string (QObject::tr ("Layer properties files (*.lyp);;All files (*)")),
                            "lyp");
  //  screenshots
  mp_screenshot_fdia = new lay::FileDialog (this,
                            tl::to_string (QObject::tr ("Screenshot")),
                            tl::to_string (QObject::tr ("PNG files (*.png);;All files (*)")),
                            "png");


  //  layout file dialog
  mp_layout_fdia = new lay::FileDialog (this, tl::to_string (QObject::tr ("Layout File")), all_layout_file_formats ());

  //  save & load layout options
  mp_layout_save_as_options = new lay::SaveLayoutAsOptionsDialog (this, tl::to_string (QObject::tr ("Save Layout Options")));
  mp_layout_save_options = new lay::SaveLayoutOptionsDialog (this, tl::to_string (QObject::tr ("Layout Writer Options")));
  mp_layout_load_options = new lay::LoadLayoutOptionsDialog (this, tl::to_string (QObject::tr ("Layout Reader Options")));

  //  log viewer dialog
  mp_log_viewer_dialog = new lay::LogViewerDialog (0);

  //  install timer for message timeout
  connect (&m_message_timer, SIGNAL (timeout ()), this, SLOT (message_timer ()));
  m_message_timer.setSingleShot (true);

  //  install timer for reload message display
  connect (&m_file_changed_timer, SIGNAL (timeout ()), this, SLOT (file_changed_timer()));
  m_file_changed_timer.setSingleShot (true);

  //  install timer for menu update a
  connect (&m_menu_update_timer, SIGNAL (timeout ()), this, SLOT (update_action_states ()));
  m_menu_update_timer.setSingleShot (false);
  m_menu_update_timer.start (200);

  connect (&lay::LayoutHandle::file_watcher (), SIGNAL (fileChanged (const QString &)), this, SLOT (file_changed (const QString &)));
  connect (&lay::LayoutHandle::file_watcher (), SIGNAL (fileRemoved (const QString &)), this, SLOT (file_removed (const QString &)));

  //  make the main window accept drops
  setAcceptDrops (true);
}

MainWindow::~MainWindow ()
{
  lay::register_help_handler (0, 0, 0);

  mw_instance = 0;

  //  explicitly delete the views here. Otherwise they
  //  are deleted by ~QWidget, which is too late since then
  //  the manager does not exist any longer.
  view_closed_event.clear (); // don't send events
  close_all ();

  //  delete the Menu after the views because they may want to access them in the destructor

  delete mp_pr;
  mp_pr = 0;

  delete mp_setup_form;
  mp_setup_form = 0;

  delete mp_log_viewer_dialog;
  mp_log_viewer_dialog = 0;

  delete mp_assistant;
  mp_assistant = 0;
}

std::string
MainWindow::all_layout_file_formats () const
{
  std::string fmts = tl::to_string (QObject::tr ("All layout files ("));
  for (tl::Registrar<db::StreamFormatDeclaration>::iterator rdr = tl::Registrar<db::StreamFormatDeclaration>::begin (); rdr != tl::Registrar<db::StreamFormatDeclaration>::end (); ++rdr) {
    if (rdr != tl::Registrar<db::StreamFormatDeclaration>::begin ()) {
      fmts += " ";
    }
    std::string f = rdr->file_format ();
    if (!f.empty ()) {
      const char *fp = f.c_str ();
      while (*fp && *fp != '(') {
        ++fp;
      }
      if (*fp) {
        ++fp;
      }
      while (*fp && *fp != ')') {
        fmts += *fp++;
      }
    }
  }
  fmts += ");;";
  for (tl::Registrar<db::StreamFormatDeclaration>::iterator rdr = tl::Registrar<db::StreamFormatDeclaration>::begin (); rdr != tl::Registrar<db::StreamFormatDeclaration>::end (); ++rdr) {
    if (!rdr->file_format ().empty ()) {
      fmts += rdr->file_format ();
      fmts += ";;";
    }
  }
  fmts += tl::to_string (QObject::tr ("All files (*)"));

  return fmts;
}

void
MainWindow::init_menu ()
{
  //  make the plugins create their menu items
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    //  TODO: get rid of the const_cast hack
    const_cast <lay::PluginDeclaration *> (&*cls)->init_menu (dispatcher ());
  }

  //  if in "viewer-only mode", hide all entries in the "hide_vo" group
  if ((lay::ApplicationBase::instance () && lay::ApplicationBase::instance ()->is_vo_mode ())) {
    std::vector<std::string> hide_vo_grp = menu ()->group ("hide_vo");
    for (std::vector<std::string>::const_iterator g = hide_vo_grp.begin (); g != hide_vo_grp.end (); ++g) {
      menu ()->action (*g)->set_visible (false);
    }
  }

  //  if not in editable mode, hide all entries from "edit_mode" group
  //  TODO: later do this on each change of the view - each view might get its own editable mode
  bool view_mode = (lay::ApplicationBase::instance () && !lay::ApplicationBase::instance ()->is_editable ());

  std::vector<std::string> edit_mode_grp = menu ()->group ("edit_mode");
  for (std::vector<std::string>::const_iterator g = edit_mode_grp.begin (); g != edit_mode_grp.end (); ++g) {
    menu ()->action (*g)->set_visible (! view_mode);
  }

  std::vector<std::string> view_mode_grp = menu ()->group ("view_mode");
  for (std::vector<std::string>::const_iterator g = view_mode_grp.begin (); g != view_mode_grp.end (); ++g) {
    menu ()->action (*g)->set_visible (view_mode);
  }
}

void
MainWindow::dock_widget_visibility_changed (bool visible)
{
  if (sender () == mp_lp_dock_widget) {
    dispatcher ()->config_set (cfg_show_layer_panel, tl::to_string (!mp_lp_dock_widget->isHidden ()));
  } else if (sender () == mp_hp_dock_widget) {
    dispatcher ()->config_set (cfg_show_hierarchy_panel, tl::to_string (!mp_hp_dock_widget->isHidden ()));
  } else if (sender () == mp_libs_dock_widget) {
    dispatcher ()->config_set (cfg_show_libraries_view, tl::to_string (!mp_libs_dock_widget->isHidden ()));
  } else if (sender () == mp_bm_dock_widget) {
    dispatcher ()->config_set (cfg_show_bookmarks_view, tl::to_string (!mp_bm_dock_widget->isHidden ()));
  } else if (sender () == mp_navigator_dock_widget) {
    dispatcher ()->config_set (cfg_show_navigator, tl::to_string (!mp_navigator_dock_widget->isHidden ()));
  } else if (sender () == mp_layer_toolbox_dock_widget) {
    dispatcher ()->config_set (cfg_show_layer_toolbox, tl::to_string (!mp_layer_toolbox_dock_widget->isHidden ()));
  } else if (sender () == mp_eo_dock_widget) {
    m_eo_visible = visible;
  }
}

void
MainWindow::file_changed_timer ()
{
  //  Don't evaluate file changed notifications while an operation is busy -
  //  otherwise we'd interfere with such operations.
  if (mp_pr->is_busy ()) {

    //  Restart the timer to get notified again
    m_file_changed_timer.setInterval (200);
    m_file_changed_timer.start ();

    return;

  }

  std::set<QString> notified_files;

  //  Make the names unique
  std::sort (m_changed_files.begin (), m_changed_files.end ());
  m_changed_files.erase (std::unique (m_changed_files.begin (), m_changed_files.end ()), m_changed_files.end ());

  if (m_changed_files.empty ()) {
    return;
  }

  //  adds a notification to the involved views that the file has changed

  std::vector<QString> changed_files;
  changed_files.swap (m_changed_files);

  std::map<QString, std::vector<std::pair<lay::LayoutViewWidget *, int> > > views_per_file;

  for (auto v = mp_views.begin (); v != mp_views.end (); ++v) {
    for (int cv = 0; cv < int ((*v)->view ()->cellviews ()); ++cv) {
      views_per_file [tl::to_qstring ((*v)->view ()->cellview (cv)->filename ())].push_back (std::make_pair (*v, cv));
    }
  }

  for (auto f = changed_files.begin (); f != changed_files.end (); ++f) {

    auto v = views_per_file.find (*f);
    if (v != views_per_file.end ()) {

      for (auto w = v->second.begin (); w != v->second.end (); ++w) {

        std::string title;
        if (w->first->view ()->cellviews () > 1) {
          title = tl::sprintf (tl::to_string (tr ("Layout file @%d (%s) has changed on disk")), w->second + 1, tl::filename (tl::to_string (*f)));
        } else {
          title = tl::sprintf (tl::to_string (tr ("Layout file (%s) has changed on disk")), tl::filename (tl::to_string (*f)));
        }

        lay::LayoutViewNotification n ("reload", title, tl::Variant (tl::to_string (*f)));
        n.add_action ("reload", tl::to_string (tr ("Reload")));
        w->first->add_notification (n);

      }

    }

  }
}

void
MainWindow::file_changed (const QString &path)
{
  m_changed_files.push_back (path);

  //  Wait a little to let more to allow for more reload requests to collect
  m_file_changed_timer.setInterval (300);
  m_file_changed_timer.start ();
}

void
MainWindow::file_removed (const QString & /*path*/)
{
  // .. nothing yet ..
}

void
MainWindow::show ()
{
  QMainWindow::show ();
  m_default_window_state = saveState ();
  m_default_window_geometry = saveGeometry ();
}

void
MainWindow::close_all ()
{
  cancel ();

  //  try a smooth shutdown of the current view
  lay::LayoutView::set_current (0);

  current_view_changed ();

  for (unsigned int i = 0; i < mp_views.size (); ++i) {
    mp_views [i]->view ()->stop ();
  }

  m_manager.clear ();

  //  Clear the tab bar
  bool f = m_disable_tab_selected;
  m_disable_tab_selected = true;
  while (mp_tab_bar->count () > 0) {
    mp_tab_bar->removeTab (mp_tab_bar->count () - 1);
  }
  m_disable_tab_selected = f;

  //  First pop the mp_views vector and then delete. This way,
  //  any callbacks issued during the deleting of the views do
  //  not find any invalid view pointers but rather nothing.
  while (mp_views.size () > 0) {

    view_closed_event (int (mp_views.size () - 1));

    lay::LayoutViewWidget *view = mp_views.back ();
    mp_views.pop_back ();
    mp_layer_toolbox_stack->remove_widget (mp_views.size ());
    mp_lp_stack->remove_widget (mp_views.size ());
    mp_hp_stack->remove_widget (mp_views.size ());
    mp_libs_stack->remove_widget (mp_views.size ());
    mp_eo_stack->remove_widget (mp_views.size ());
    mp_bm_stack->remove_widget (mp_views.size ());
    mp_view_stack->remove_widget (mp_views.size ());

    delete view;

  }

  update_dock_widget_state ();
}

void
MainWindow::about_to_exec ()
{
  //  NOTE: empirically by using "0" for the parent of the TipDialogs (not "this"),
  //  these dialog appear on the same screen than the application window. With "this"
  //  they usually appear somewhere else. Maybe because this method is called before
  //  the main window is properly set up.

  bool f;

  f = false;
  dispatcher ()->config_get (cfg_full_hier_new_cell, f);
  if (!f) {
    TipDialog td (0,
                  tl::to_string (QObject::tr ("<html><body>"
                                              "<p>With the current settings, only the top cell's content is shown initially, but the child cells are not drawn.</p>"
                                              "<p>This can be confusing, since the full layout becomes visible only after selecting "
                                              "all hierarchy levels manually.</p>"
                                              "<p>This setting can be changed now. It can also be changed any time later using \"File/Setup\", \"Navigation/New Cell\": "
                                              "\"Select all hierarchy levels\".</p>"
                                              "<ul>"
                                              "<li>Press <b>Yes</b> to enable <b>Show full hierarchy</b> mode now.</li>\n"
                                              "<li>With <b>No</b>, the mode will remain <b>Show top level only</b>.</li>"
                                              "</ul>"
                                              "</body></html>")),
                  "only-top-level-shown-by-default",
                  lay::TipDialog::yesno_buttons);
    lay::TipDialog::button_type button = lay::TipDialog::null_button;
    if (td.exec_dialog (button)) {
      if (button == lay::TipDialog::yes_button) {
        dispatcher ()->config_set (cfg_full_hier_new_cell, true);
      }
      //  Don't bother the user with more dialogs.
      return;
    }
  }

  //  TODO: later, each view may get its own editable flag
  if (lay::ApplicationBase::instance () && !lay::ApplicationBase::instance ()->is_editable ()) {
    TipDialog td (0,
                  tl::to_string (QObject::tr ("KLayout has been started in viewer mode. In this mode, editor functions are not available.\n\nTo enable these functions, start KLayout in editor mode by using the \"-e\" command line switch or select it as the default mode in the setup dialog. Choose \"Setup\" in the \"File\" menu and check \"Use editing mode by default\" on the \"Editing Mode\" page in the \"Application\" section.")),
                  "editor-mode");
    if (td.exec_dialog ()) {
      //  Don't bother the user with more dialogs.
      return;
    }
  }

  f = false;
  dispatcher ()->config_get (cfg_no_stipple, f);
  if (f) {
    TipDialog td (0,
                  tl::to_string (QObject::tr ("Layers are shown without fill because fill has been intentionally turned off. This can be confusing since selecting a stipple does not have an effect in this case.\n\nTo turn this feature off, uncheck \"Show Layers Without Fill\" in the \"View\" menu.")),
                  "no-stipple");
    if (td.exec_dialog ()) {
      //  Don't bother the user with more dialogs.
      return;
    }
  }

  f = false;
  dispatcher ()->config_get (cfg_markers_visible, f);
  if (! f) {
    TipDialog td (0,
                  tl::to_string (QObject::tr ("Markers are not visible because they have been turned off.\nYou may not see markers when using the marker browser feature.\n\nTo turn markers on, check \"Show Markers\" in the \"View\" menu.")),
                  "show-markers");
    if (td.exec_dialog ()) {
      //  Don't bother the user with more dialogs.
      return;
    }
  }

  f = false;
  dispatcher ()->config_get (cfg_hide_empty_layers, f);
  if (f) {
    TipDialog td (0,
                  tl::to_string (QObject::tr ("The \"Hide Empty Layers\" feature is enabled. This can be confusing, in particular in edit mode, because layers are not shown although they are actually present.\n\nTo disable this feature, uncheck \"Hide Empty Layers\" in the layer panel's context menu.")),
                  "hide-empty-layers");
    if (td.exec_dialog ()) {
      //  Don't bother the user with more dialogs.
      return;
    }
  }
}

void
MainWindow::tech_message (const std::string &s)
{
  mp_tech_status_label->setText(tl::to_qstring (s));
}

static int fm_width (const QFontMetrics &fm, const QString &s)
{
#if QT_VERSION >= 0x60000
  return fm.horizontalAdvance (s);
#else
  return fm.width (s);
#endif
}

void
MainWindow::format_message ()
{
  QFontMetrics fm (mp_msg_label->font ());

  std::string full_message;
  for (const char *c = m_message.c_str (); *c; ++c) {
    if (*c == '\\' && (c[1] == '(' || c[1] == ')')) {
      ++c;
    } else {
      full_message += *c;
    }
  }

  std::string short_message;
  size_t ndrop = 0;
  size_t prev_len = 0;
  bool use_ellipsis = true;

  do {

    size_t nsection = 0;
    bool in_drop = false;
    prev_len = ndrop > 0 ? short_message.size () : std::numeric_limits<size_t>::max ();

    short_message.clear ();

    for (const char *c = m_message.c_str (); *c; ++c) {
      if (*c == '\\' && c[1] == '(') {
        if (nsection++ < ndrop) {
          in_drop = true;
          if (use_ellipsis) {
            short_message += "...";
            use_ellipsis = false;
          }
        }
        ++c;
      } else if (*c == '\\' && c[1] == ')') {
        in_drop = false;
        ++c;
      } else if (! in_drop) {
        use_ellipsis = true;
        short_message += *c;
      }
    }

    ++ndrop;

  } while (short_message.size () < prev_len && fm_width (fm, QString::fromUtf8 (" ") + tl::to_qstring (short_message)) > mp_msg_label->width ());

  mp_msg_label->setText (QString::fromUtf8 (" ") + tl::to_qstring (short_message));
  mp_msg_label->setToolTip (tl::to_qstring (full_message));
}

void
MainWindow::message (const std::string &s, int ms)
{
  m_message = s;
  format_message ();
  m_message_timer.start (ms);
}

void
MainWindow::clear_message ()
{
  m_message.clear ();
  m_message_timer.start (0);
}

void
MainWindow::message_timer ()
{
  m_message.clear ();
  format_message ();
}

void
MainWindow::config_finalize ()
{
  // Not set the window state: this ensures we have handled cfg_window_geometry
  // before we restore the state
  if (! m_config_window_state.empty ()) {

    QByteArray state = QByteArray::fromBase64 (m_config_window_state.c_str ());
    m_config_window_state.clear ();

    bool eo_visible = m_eo_visible;

    restoreState (state);

    //  Keep the editor options visibility state
    m_eo_visible = eo_visible;
    show_dock_widget (mp_eo_dock_widget, m_eo_visible);

  }

  // Update the default grids menu if necessary
  if (m_default_grids_updated) {

    m_default_grids_updated = false;

    std::vector<std::string> group = menu ()->group ("default_grids_group");

    for (std::vector<std::string>::const_iterator t = group.begin (); t != group.end (); ++t) {
      std::vector<std::string> items = menu ()->items (*t);
      for (std::vector<std::string>::const_iterator i = items.begin (); i != items.end (); ++i) {
        menu ()->delete_item (*i);
      }
    }

    int i = 1;
    for (std::vector<double>::const_iterator g = m_default_grids.begin (); g != m_default_grids.end (); ++g, ++i) {

      std::string name = "default_grid_" + tl::to_string (i);

      lay::Action *ga = new lay::ConfigureAction (tl::to_string (*g) + tl::to_string (QObject::tr (" um")), cfg_grid, tl::to_string (*g));
      ga->set_checkable (true);
      ga->set_checked (fabs (*g - m_grid_micron) < 1e-10);

      for (std::vector<std::string>::const_iterator t = group.begin (); t != group.end (); ++t) {
        menu ()->insert_item (*t + ".end", name, ga);
      }

    }

    //  re-apply key bindings for the default grids
    apply_key_bindings ();

  }

  //  make the changes visible in the setup form if the form is visible
  mp_setup_form->setup ();
}

bool
MainWindow::configure (const std::string &name, const std::string &value)
{
  if (name == cfg_grid) {

    double g = 0.0;
    tl::from_string (value, g);
    m_grid_micron = g;
    m_default_grids_updated = true;
    return false; // not taken - let others set it too.

  } else if (name == cfg_circle_points) {

    //  pseudo-configuration: set db::set_num_circle_points
    int cp = 16;
    tl::from_string (value, cp);
    if (cp != int (db::num_circle_points ())) {
      db::set_num_circle_points (cp);
      redraw ();
    }
    return true;

  } else if (name == cfg_default_grids) {

    tl::Extractor ex (value.c_str ());
    m_default_grids.clear ();
    m_default_grids_updated = true;

    //  convert the list of grids to a list of doubles
    while (! ex.at_end ()) {
      double g = 0.0;
      if (! ex.try_read (g)) {
        break;
      }
      m_default_grids.push_back (g);
      ex.test (",");
    }

    return true;

  } else if (name == cfg_mru) {

    tl::Extractor ex (value.c_str ());

    m_mru.clear ();
    while (! ex.at_end ()) {
      m_mru.push_back (std::make_pair (std::string (), std::string ()));
      ex.read_quoted (m_mru.back ().first);
      if (ex.test ("@")) {
        ex.read_quoted (m_mru.back ().second);
      }
    }

    dm_do_update_mru_menus ();

    return true;

  } else if (name == cfg_mru_sessions) {

    tl::Extractor ex (value.c_str ());

    m_mru_sessions.clear ();
    while (! ex.at_end ()) {
      m_mru_sessions.push_back (value);
      ex.read_quoted (m_mru_sessions.back ());
    }

    dm_do_update_mru_menus ();

    return true;

  } else if (name == cfg_mru_layer_properties) {

    tl::Extractor ex (value.c_str ());

    m_mru_layer_properties.clear ();
    while (! ex.at_end ()) {
      m_mru_layer_properties.push_back (value);
      ex.read_quoted (m_mru_layer_properties.back ());
    }

    dm_do_update_mru_menus ();

    return true;

  } else if (name == cfg_mru_bookmarks) {

    tl::Extractor ex (value.c_str ());

    m_mru_bookmarks.clear ();
    while (! ex.at_end ()) {
      m_mru_bookmarks.push_back (value);
      ex.read_quoted (m_mru_bookmarks.back ());
    }

    dm_do_update_mru_menus ();

    return true;

  } else if (name == cfg_keep_backups) {

    int kb = 0;
    tl::from_string (value, kb);
    m_keep_backups = kb;

    return false; // not taken - let others set it too.

  } else if (name == cfg_micron_digits) {

    unsigned int d = 5;
    tl::from_string (value, d);
    tl::set_micron_resolution (d);

    return true;

  } else if (name == cfg_dbu_digits) {

    unsigned int d = 2;
    tl::from_string (value, d);
    tl::set_db_resolution (d);

    return true;

  } else if (name == cfg_window_state) {

    //  restore the state on config_finalize to ensure we have handled it after
    //  restoring the geometry
    m_config_window_state = value;
    return true;

  } else if (name == cfg_window_geometry) {

    if (! value.empty ()) {
      QByteArray state = QByteArray::fromBase64 (value.c_str ());
      restoreGeometry (state);
    }

    return true;

  } else if (name == cfg_show_layer_toolbox) {

    tl::from_string (value, m_layer_toolbox_visible);
    if (m_layer_toolbox_visible) {
      mp_layer_toolbox_dock_widget->show ();
    } else {
      mp_layer_toolbox_dock_widget->hide ();
    }

    return true;

  } else if (name == cfg_reader_options_show_always) {

    bool f = false;
    tl::from_string (value, f);
    mp_layout_load_options->show_always (f);

    return true;

  } else if (name == cfg_show_navigator) {

    tl::from_string (value, m_navigator_visible);
    if (m_navigator_visible) {
      mp_navigator_dock_widget->show ();
    } else {
      mp_navigator_dock_widget->hide ();
    }

    return true;

  } else if (name == cfg_navigator_show_images) {

    bool flag = false;
    tl::from_string (value, flag);
    mp_navigator->show_images (flag);

    return true;

  } else if (name == cfg_navigator_all_hier_levels) {

    bool flag = false;
    tl::from_string (value, flag);
    mp_navigator->all_hier_levels (flag);

    return true;

  } else if (name == cfg_show_toolbar) {

    bool flag = false;
    tl::from_string (value, flag);
    if (flag) {
      mp_tool_bar->show ();
    } else {
      mp_tool_bar->hide ();
    }

    return true;

  } else if (name == cfg_show_hierarchy_panel) {

    tl::from_string (value, m_hp_visible);
    if (m_hp_visible) {
      mp_hp_dock_widget->show ();
    } else {
      mp_hp_dock_widget->hide ();
    }

    return true;

  } else if (name == cfg_show_libraries_view) {

    tl::from_string (value, m_libs_visible);
    if (m_libs_visible) {
      mp_libs_dock_widget->show ();
    } else {
      mp_libs_dock_widget->hide ();
    }

    return true;

  } else if (name == cfg_show_bookmarks_view) {

    tl::from_string (value, m_bm_visible);
    if (m_bm_visible) {
      mp_bm_dock_widget->show ();
    } else {
      mp_bm_dock_widget->hide ();
    }

    return true;

  } else if (name == cfg_show_layer_panel) {

    tl::from_string (value, m_lp_visible);
    if (m_lp_visible) {
      mp_lp_dock_widget->show ();
    } else {
      mp_lp_dock_widget->hide ();
    }

    return true;

  } else if (name == cfg_synchronized_views) {

    bool flag = false;
    tl::from_string (value, flag);
    m_synchronized_views = flag;
    return true;

  } else if (name == cfg_layout_file_watcher_enabled) {

    bool flag = false;
    tl::from_string (value, flag);
    lay::LayoutHandle::file_watcher ().enable (flag);
    return true;

  } else if (name == cfg_key_bindings) {

    m_key_bindings = unpack_key_binding (value);
    apply_key_bindings ();
    return true;

  } else if (name == cfg_menu_items_hidden) {

    std::vector<std::pair<std::string, bool> > hidden = unpack_menu_items_hidden (value);
    apply_hidden (hidden);
    return true;

  } else if (name == cfg_initial_technology) {

    m_initial_technology = value;
    return true;

  } else if (name == cfg_always_exit_without_saving) {

    tl::from_string (value, m_always_exit_without_saving);
    return true;

  } else {
    return false;
  }

}

void
MainWindow::apply_hidden (const std::vector<std::pair<std::string, bool> > &hidden)
{
  for (std::vector<std::pair<std::string, bool> >::const_iterator hf = hidden.begin (); hf != hidden.end (); ++hf) {
    if (menu ()->is_valid (hf->first)) {
      lay::Action *a = menu ()->action (hf->first);
      a->set_hidden (hf->second);
    }
  }
}

void
MainWindow::apply_key_bindings ()
{
  for (std::vector<std::pair<std::string, std::string> >::const_iterator kb = m_key_bindings.begin (); kb != m_key_bindings.end (); ++kb) {
    if (menu ()->is_valid (kb->first)) {
      lay::Action *a = menu ()->action (kb->first);
      a->set_shortcut (kb->second);
    }
  }
}

bool
MainWindow::edits_enabled () const
{
  //  NOTE: "edits_enabled" does not - contrary to the name - indicate that editing is enabled
  //  but that the system is accepting changes of any kind. Hence if there is no view open,
  //  we need to accept changes, otherwise we cannot open new files.
  return !current_view () || current_view ()->edits_enabled ();
}

void
MainWindow::edits_enabled_changed ()
{
  bool enable = edits_enabled ();

  std::vector<std::string> edit_grp = menu ()->group ("edit");
  for (std::vector<std::string>::const_iterator g = edit_grp.begin (); g != edit_grp.end (); ++g) {
    menu ()->action (*g)->set_enabled (enable);
  }
}

void
MainWindow::menu_needs_update ()
{
  lay::LayoutView::update_menu (current_view (), *menu ());
}

void
MainWindow::libraries_changed ()
{
  //  if the libraries have changed, remove all selections and cancel any operations to avoid
  //  that the view refers to an invalid instance or shape
  for (std::vector <lay::LayoutViewWidget *>::iterator vp = mp_views.begin (); vp != mp_views.end (); ++vp) {
    (*vp)->view ()->clear_selection ();
    (*vp)->view ()->cancel ();
  }
}

void
MainWindow::read_dock_widget_state ()
{
  dispatcher ()->config_set (cfg_show_layer_panel, tl::to_string (!mp_lp_dock_widget->isHidden ()));
  dispatcher ()->config_set (cfg_show_hierarchy_panel, tl::to_string (!mp_hp_dock_widget->isHidden ()));
  dispatcher ()->config_set (cfg_show_libraries_view, tl::to_string (!mp_libs_dock_widget->isHidden ()));
  dispatcher ()->config_set (cfg_show_bookmarks_view, tl::to_string (!mp_bm_dock_widget->isHidden ()));
  dispatcher ()->config_set (cfg_show_navigator, tl::to_string (!mp_navigator_dock_widget->isHidden ()));
  dispatcher ()->config_set (cfg_show_layer_toolbox, tl::to_string (!mp_layer_toolbox_dock_widget->isHidden ()));
}

void
MainWindow::update_dock_widget_state ()
{
  if (m_hp_visible) {
    mp_hp_dock_widget->show ();
  } else {
    mp_hp_dock_widget->hide ();
  }

  if (m_libs_visible) {
    mp_libs_dock_widget->show ();
  } else {
    mp_libs_dock_widget->hide ();
  }

  if (m_eo_visible) {
    mp_eo_dock_widget->show ();
  } else {
    mp_eo_dock_widget->hide ();
  }

  if (m_bm_visible) {
    mp_bm_dock_widget->show ();
  } else {
    mp_bm_dock_widget->hide ();
  }

  if (m_lp_visible) {
    mp_lp_dock_widget->show ();
  } else {
    mp_lp_dock_widget->hide ();
  }

  if (m_navigator_visible) {
    mp_navigator_dock_widget->show ();
  } else {
    mp_navigator_dock_widget->hide ();
  }

  if (m_layer_toolbox_visible) {
    mp_layer_toolbox_dock_widget->show ();
  } else {
    mp_layer_toolbox_dock_widget->hide ();
  }
}

void
MainWindow::exit ()
{
  m_exited = true;

  //  If there is a operation ongoing, request a break and delay execution of the exit operation.
  if (mp_pr && mp_pr->is_busy ()) {
    mp_pr->signal_break ();
    dm_exit ();
    return;
  }

  //  We also don't exit if a dialog is shown (deferred execution may be called from
  //  the dialog's exec loop).
  if (QApplication::activeModalWidget ()) {
    dm_exit ();
    return;
  }

  //  Only after other operation has finished we ask whether to save and close eventually
  if (can_close ()) {

    do_close ();
    QMainWindow::close ();

    emit closed ();

  } else {
    m_exited = false;
  }
}

int
MainWindow::dirty_files (std::string &dirty_files)
{
  int dirty_layouts = 0;

  std::vector <std::string> names;
  lay::LayoutHandle::get_names (names);

  for (std::vector <std::string>::const_iterator n = names.begin (); n != names.end (); ++n) {

    lay::LayoutHandle *handle = lay::LayoutHandle::find (*n);
    if (handle && handle->layout ().is_editable () && handle->is_dirty ()) {

      ++dirty_layouts;
      if (dirty_layouts == max_dirty_files) {
        dirty_files += "\n  ...";
      } else if (dirty_layouts < max_dirty_files) {
        if (! dirty_files.empty ()) {
          dirty_files += "\n";
        }
        dirty_files += "  ";
        dirty_files += handle->name ();
      }

    }

  }

  return dirty_layouts;
}

bool
MainWindow::can_close ()
{
  if (m_busy) {

    bool can_close = false;

    can_close = (QMessageBox::warning (this,
      QObject::tr ("Application Busy"),
      QObject::tr ("The application is busy.\nYou can close the application now, but any unsaved data will be lost.\n\nPress 'Yes' to end the application now."),
      QMessageBox::Yes | QMessageBox::No,
      QMessageBox::Yes) == QMessageBox::Yes);

    return can_close;

  }

  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    lay::PluginDeclaration *pd = const_cast<lay::PluginDeclaration *> (&*cls);
    if (! pd->can_exit (dispatcher ())) {
      return false;
    }
  }

  std::string df_list;
  int dirty_layouts = dirty_files (df_list);

  if ( m_always_exit_without_saving || (dirty_layouts == 0) ) {
    return true;
  } else {

    QMessageBox mbox (this);
    mbox.setText (tl::to_qstring (tl::to_string (QObject::tr ("The following layouts need saving:\n\n")) + df_list + "\n\nPress 'Exit Without Saving' to exit anyhow and discard changes."));
    mbox.setWindowTitle (QObject::tr ("Save Needed"));
    mbox.setIcon (QMessageBox::Warning);
    QAbstractButton *exit_button = mbox.addButton (QObject::tr ("Exit Without Saving"), QMessageBox::YesRole);
    mbox.addButton (QMessageBox::Cancel);

    mbox.exec ();

    return mbox.clickedButton() == exit_button;

  }
}

void
MainWindow::do_close ()
{
  //  don't close if busy in processEvents
  if (m_busy) {
    return;
  }

  //  close all views
  close_all ();

  save_state_to_config ();
}

void
MainWindow::save_state_to_config ()
{
  //  save the dock widget state with all views closed (that state can be
  //  used for staring klayout without any layout)
  dispatcher ()->config_set (cfg_window_geometry, (const char *) saveGeometry ().toBase64 ().data ());
  dispatcher ()->config_set (cfg_window_state, (const char *) saveState ().toBase64 ().data ());
}

void
MainWindow::resizeEvent (QResizeEvent *)
{
  format_message ();
}

void
MainWindow::closeEvent (QCloseEvent *event)
{
  if (! m_exited) {
    BEGIN_PROTECTED
    exit ();
    END_PROTECTED
  }

  event->ignore ();
}

void
MainWindow::cm_navigator_freeze ()
{
  if (mp_navigator) {
    mp_navigator->freeze_clicked ();
  }
}

void
MainWindow::cm_navigator_close ()
{
  if (mp_navigator) {
    mp_navigator->close ();
  }
}

void
MainWindow::cm_view_log ()
{
  mp_log_viewer_dialog->show ();
}

void
MainWindow::cm_print ()
{
  //  TODO: move to lay::LayoutView

  //  Late-initialize the printer to save time on startup
  if (! mp_printer.get ()) {
    mp_printer.reset (new QPrinter ());
  }

  std::string v = std::string (lay::Version::name ()) + " " + lay::Version::version ();
  mp_printer->setCreator (tl::to_qstring (v));
  mp_printer->setDocName (tl::to_qstring ("klayout_printout"));

  QPrintDialog print_dialog (mp_printer.get (), this);
  if (print_dialog.exec() == QDialog::Accepted) {

    if (current_view ()) {

      int scale_factor = 3;

      //  choose a resolution around 300dpi
      double rf = floor (0.5 + 300.0 / mp_printer->resolution ());
      mp_printer->setResolution (int (floor (0.5 + mp_printer->resolution () * rf)));

      QPainter painter;

      painter.begin (mp_printer.get ());

      QFont header_font (QString::fromUtf8 ("Helvetica"), 8);
      int hh = QFontMetrics (header_font, mp_printer.get ()).height ();

#if QT_VERSION >= 0x60000
      QRectF page_rect = mp_printer->pageRect (QPrinter::DevicePixel);
#else
      QRectF page_rect = mp_printer->pageRect ();
#endif
      page_rect.moveTo (0, 0);

      int b = std::min (page_rect.width (), page_rect.height ()) / 50;
      page_rect.setLeft (page_rect.left () + b);
      page_rect.setRight (page_rect.right () - b);
      page_rect.setTop (page_rect.top () + b);
      page_rect.setBottom (page_rect.bottom () - b);

      QRectF text_rect = page_rect;
      text_rect.setLeft (text_rect.left () + hh / 2);
      text_rect.setRight (text_rect.right () - hh / 2);
      text_rect.setBottom (text_rect.bottom () - hh / 2);
      text_rect.setTop (text_rect.top () + hh / 2);

      QImage img = current_view ()->get_image_with_options (page_rect.width (),
                                                            page_rect.height () - 4 * hh,
                                                            scale_factor,
                                                            1,
                                                            1.0 / scale_factor,
                                                            tl::Color (QColor (Qt::white)),  //  foreground
                                                            tl::Color (QColor (Qt::black)),  //  background
                                                            tl::Color (QColor (Qt::black)),  //  active
                                                            db::DBox (),
                                                            false);

      painter.drawImage (QPoint (page_rect.left (), page_rect.top () + hh * 2), img);
      painter.setFont (header_font);
      painter.drawRect (page_rect);

      painter.drawText (text_rect, Qt::AlignLeft | Qt::AlignTop, tl::to_qstring (v));
      painter.drawText (text_rect, Qt::AlignHCenter | Qt::AlignTop, tl::to_qstring (current_view ()->title ()));
      painter.drawText (text_rect, Qt::AlignLeft | Qt::AlignBottom, QDateTime::currentDateTime ().toString ());

      db::DBox vp = current_view ()->viewport ().box ();
      std::string vp_string = "(" + tl::micron_to_string (vp.left ()) + ", " + tl::micron_to_string (vp.bottom ()) + " ... " +
                                    tl::micron_to_string (vp.right ()) + ", " + tl::micron_to_string (vp.top ()) + ")";
      painter.drawText (text_rect, Qt::AlignRight | Qt::AlignBottom, tl::to_qstring (vp_string));

      painter.end ();

    } else {
      throw tl::Exception (tl::to_string (QObject::tr ("No view open to print")));
    }

  }
}

void
MainWindow::cm_exit ()
{
  exit ();
}

lay::LayoutView *
MainWindow::view (int index)
{
  if (index >= 0 && index < int (mp_views.size ())) {
    return mp_views [index]->view ();
  } else {
    return 0;
  }
}

const lay::LayoutView *
MainWindow::view (int index) const
{
  if (index >= 0 && index < int (mp_views.size ())) {
    return mp_views [index]->view ();
  } else {
    return 0;
  }
}

int
MainWindow::index_of (const lay::LayoutView *view) const
{
  for (int i = 0; i < int (mp_views.size ()); ++i) {
    if (mp_views [i]->view () == view) {
      return i;
    }
  }

  return -1;
}

int
MainWindow::current_view_index () const
{
  return index_of (current_view ());
}

lay::LayoutView *
MainWindow::current_view () const
{
  return lay::LayoutView::current ();
}

void
MainWindow::select_mode (int m)
{
  if (m_mode != m) {

    m_mode = m;
    for (std::vector <lay::LayoutViewWidget *>::iterator vp = mp_views.begin (); vp != mp_views.end (); ++vp) {
      (*vp)->view ()->mode (m);
    }

    //  Update the actions by checking the one that is selected programmatically. Use the toolbar menu for reference.
    //  TODO: this code needs to be kept aligned with the implementation of PluginDeclaration::init_menu ()
    //  It's not easy to move the functionality to PluginDeclaration because some of the modes are not mapped to
    //  Plugin's yet (selection, move).
    std::vector<std::string> items = menu ()->items ("@toolbar");
    for (std::vector<std::string>::const_iterator i = items.begin (); i != items.end (); ++i) {
      Action *a = menu ()->action (*i);
      if (a->is_checkable()) {
        a->set_checked (a->is_for_mode (m_mode));
      }
    }

    //  if the current mode supports editing, show the editor options panel

    const lay::PluginDeclaration *pd_sel = 0;
    for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
      const lay::PluginDeclaration *pd = cls.operator-> ();
      if (pd->id () == m_mode) {
        pd_sel = pd;
      }
    }

    bool eo_visible = false;
    if (mp_eo_stack && pd_sel) {
      eo_visible = pd_sel->editable_enabled ();
    }
    if (current_view () && eo_visible) {
      lay::EditorOptionsPages *eo_pages = current_view ()->editor_options_pages ();
      if (! eo_pages || ! eo_pages->has_content ()) {
        eo_visible = false;
      }
    }

    if (eo_visible != m_eo_visible) {
      m_eo_visible = eo_visible;
      show_dock_widget (mp_eo_dock_widget, m_eo_visible);
    }

  }
}

void
MainWindow::cm_reset_window_state ()
{
  restoreState (m_default_window_state);
  restoreGeometry (m_default_window_geometry);
}

void
MainWindow::cm_undo ()
{
  if (current_view () && m_manager.available_undo ().first) {
    for (std::vector <lay::LayoutViewWidget *>::iterator vp = mp_views.begin (); vp != mp_views.end (); ++vp) {
      (*vp)->view ()->clear_selection ();
      (*vp)->view ()->cancel ();
    }
    m_manager.undo ();
  }
}

void
MainWindow::cm_redo ()
{
  if (current_view () && m_manager.available_redo ().first) {
    for (std::vector <lay::LayoutViewWidget *>::iterator vp = mp_views.begin (); vp != mp_views.end (); ++vp) {
      (*vp)->view ()->clear_selection ();
      (*vp)->view ()->cancel ();
    }
    m_manager.redo ();
  }
}

void
MainWindow::cm_goto_position ()
{
  if (current_view ()) {

    while (true) {

      bool ok = false;

      db::DBox box (current_view ()->box ());
      std::string pos;
      pos += tl::micron_to_string (box.center ().x ()) + "," + tl::micron_to_string (box.center ().y ());
      pos += ",";
      pos += tl::micron_to_string (std::min (box.width (), box.height ()));

      QString text = QInputDialog::getText (this, QObject::tr ("Enter Position"), QObject::tr ("Enter position either as pair (x,y) or with window size (x,y,s)\n(x,y) will be the new window center position, s (if given) the new window size"),
                                            QLineEdit::Normal, tl::to_qstring (pos), &ok);

      if (! ok) {

        break;

      } else if (text.isEmpty ()) {

        throw tl::Exception (tl::to_string (QObject::tr ("Enter the position")));

      } else {

        double x = 0.0, y = 0.0, s = 0.0;
        std::string tt (tl::to_string (text));

        tl::Extractor ex (tt.c_str ());
        x = tl::Eval ().parse (ex).execute ().to_double ();
        ex.test (",");
        y = tl::Eval ().parse (ex).execute ().to_double ();

        db::DPoint pt (x, y);

        if (! ex.at_end ()) {
          ex.test (",");
          s = tl::Eval ().parse (ex).execute ().to_double ();
          current_view ()->goto_window (pt, s);
        } else {
          current_view ()->goto_window (pt);
        }

        std::string goto_marker_cat = "_goto_marker";

        ant::Service *ant_service = current_view ()->get_plugin<ant::Service> ();
        if (ant_service) {

          //  Produce a "goto marker" at the target position

          ant::AnnotationIterator a = ant_service->begin_annotations ();
          while (! a.at_end ()) {
            if (a->category () == goto_marker_cat) {
              ant_service->delete_ruler (a.current ());
            }
            ++a;
          }

          ant::Object marker;
          marker.p1 (pt);
          marker.p2 (pt);
          marker.fmt_x ("");
          marker.fmt_y ("");
          marker.fmt ("$U,$V");
          marker.angle_constraint (lay::AC_Any);
          marker.style (ant::Object::STY_cross_both);
          marker.outline (ant::Object::OL_diag);
          marker.set_category (goto_marker_cat);

          ant_service->insert_ruler (marker, false);

        }

        break;

      }

    }

  }
}

void
MainWindow::cm_manage_bookmarks ()
{
  if (current_view ()) {
    current_view ()->manage_bookmarks ();
  }
}

void
MainWindow::cm_bookmark_view ()
{
  if (current_view ()) {
    current_view ()->bookmark_current_view ();
  }
}

void
MainWindow::update_content ()
{
  mp_setup_form->setup ();
  if (current_view ()) {
    current_view ()->update_content ();
  }
}

void
MainWindow::update_action_states ()
{
  try {

    if (menu ()->is_valid ("edit_menu.undo")) {

      Action *undo_action = menu ()->action ("edit_menu.undo");

      std::string undo_txt (tl::to_string (QObject::tr ("&Undo")));
      bool undo_enable = false;
      if (current_view () && m_manager.available_undo ().first) {
        undo_txt += " - " + m_manager.available_undo ().second;
        undo_enable = true;
      }
      undo_action->set_title (undo_txt);
      undo_action->set_enabled (undo_enable && edits_enabled ());

    }

    if (menu ()->is_valid ("edit_menu.redo")) {

      Action *redo_action = menu ()->action ("edit_menu.redo");

      std::string redo_txt (tl::to_string (QObject::tr ("&Redo")));
      bool redo_enable = false;
      if (current_view () && m_manager.available_redo ().first) {
        redo_txt += " - " + m_manager.available_redo ().second;
        redo_enable = true;
      }
      redo_action->set_title (redo_txt);
      redo_action->set_enabled (redo_enable && edits_enabled ());

    }

    if (menu ()->is_valid ("edit_menu.paste")) {
      Action *paste_action = menu ()->action ("edit_menu.paste");
      paste_action->set_enabled (! db::Clipboard::instance ().empty () && edits_enabled ());
    }

    if (menu ()->is_valid ("zoom_menu.next_display_state")) {
      Action *next_display_state_action = menu ()->action ("zoom_menu.next_display_state");
      next_display_state_action->set_enabled (has_next_display_state ());
    }

    if (menu ()->is_valid ("zoom_menu.prev_display_state")) {
      Action *prev_display_state_action = menu ()->action ("zoom_menu.prev_display_state");
      prev_display_state_action->set_enabled (has_prev_display_state ());
    }

  } catch (...) {
    //  ignore exceptions
  }
}

void
MainWindow::redraw ()
{
  if (current_view ()) {
    current_view ()->redraw ();
  }
}

void
MainWindow::cm_cancel ()
{
  cancel ();
}

/**
 *  @brief Commits any operations, cancels any pending edit operations and clears the selection.
 */
void
MainWindow::cancel ()
{
  //  if any transaction is pending (this may happen when an operation threw an exception)
  //  close transactions.
  if (m_manager.transacting ()) {
    m_manager.commit ();
  }

  for (std::vector <lay::LayoutViewWidget *>::iterator vp = mp_views.begin (); vp != mp_views.end (); ++vp) {
    (*vp)->view ()->cancel ();
  }

  select_mode (lay::LayoutView::default_mode ());
}

void
MainWindow::load_layer_properties (const std::string &fn, bool all_views, bool add_default)
{
  if (all_views) {
    for (std::vector <lay::LayoutViewWidget *>::iterator vp = mp_views.begin (); vp != mp_views.end (); ++vp) {
      (*vp)->view ()->load_layer_props (fn, add_default);
    }
  } else if (current_view ()) {
    current_view ()->load_layer_props (fn, add_default);
  }
}

void
MainWindow::load_layer_properties (const std::string &fn, int cv_index, bool all_views, bool add_default)
{
  if (all_views) {
    for (std::vector <lay::LayoutViewWidget *>::iterator vp = mp_views.begin (); vp != mp_views.end (); ++vp) {
      (*vp)->view ()->load_layer_props (fn, cv_index, add_default);
    }
  } else if (current_view ()) {
    current_view ()->load_layer_props (fn, cv_index, add_default);
  }
}

void
MainWindow::cm_save_layer_props ()
{
  if (current_view ()) {
    std::string fn;
    if (mp_lprops_fdia->get_save (fn, tl::to_string (QObject::tr ("Save Layer Properties File")))) {
      current_view ()->save_layer_props (fn);
      add_to_other_mru (fn, cfg_mru_layer_properties);
    }
  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open to save the layer properties from")));
  }
}

void
MainWindow::cm_load_layer_props ()
{
  if (current_view ()) {
    std::string fn;
    if (mp_lprops_fdia->get_open (fn, tl::to_string (QObject::tr ("Load Layer Properties File")))) {
      load_layer_props_from_file (fn);
      add_to_other_mru (fn, cfg_mru_layer_properties);
    }
  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open to load the layer properties for")));
  }
}

void
MainWindow::load_layer_props_from_file (const std::string &fn)
{
  if (! current_view ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open to load the layer properties for")));
  }

  int target_cv_index = -2;

  if (current_view ()->cellviews () > 1 && lay::LayoutView::is_single_cv_layer_properties_file (fn)) {

    QStringList items;
    items << QString (QObject::tr ("Take it as it is"));
    items << QString (QObject::tr ("Apply to all layouts"));
    for (unsigned int i = 0; i < current_view ()->cellviews (); ++i) {
      items << QString (tl::to_qstring (tl::to_string (QObject::tr ("Apply to ")) + current_view ()->cellview (i)->name () + " (@" + tl::to_string (i + 1) + ")"));
    }

    bool ok;
    QString item = QInputDialog::getItem(this, QObject::tr ("Apply Layer Properties File"),
                                               QObject::tr ("There are multiple layouts in that panel but the layer properties file contains properties for a single one.\nWhat should be done?"),
                                               items, 1, false, &ok);
    if (!ok || item.isEmpty()) {
      return;
    }

    target_cv_index = items.indexOf (item) - 2;

  }

  if (target_cv_index > -2) {
    load_layer_properties (fn, target_cv_index, false /*current view only*/, false /*don't add default*/);
  } else {
    load_layer_properties (fn, false /*current view only*/, false /*don't add default*/);
  }
}

void
MainWindow::save_session (const std::string &fn)
{
  m_current_session = fn;
  lay::Session session;
  session.fetch (*this);
  session.save (fn);
}

void
MainWindow::restore_session (const std::string &fn)
{
  m_current_session = fn;
  lay::Session session;
  session.load (fn);

  begin_restore_session ();
  try {
    session.restore (*this);
    read_dock_widget_state ();
    end_restore_session ();
  } catch (...) {
    read_dock_widget_state ();
    end_restore_session ();
    throw;
  }
}

void
MainWindow::cm_save_session ()
{
  std::string df_list;
  int dirty_layouts = dirty_files (df_list);

  if (dirty_layouts == 0 ||
    QMessageBox::warning (this,
      QObject::tr ("Save Needed For Some Layouts"),
      tl::to_qstring (tl::to_string (QObject::tr ("The following layouts need saving.\nThese layouts must be saved manually:\n\n")) + df_list + "\n\nPress 'Ok' to continue."),
      QMessageBox::Ok | QMessageBox::Cancel,
      QMessageBox::Cancel) == QMessageBox::Ok) {

    std::string fn = m_current_session;
    if (mp_session_fdia->get_save (fn, tl::to_string (QObject::tr ("Save Session File")))) {
      save_session (fn);
      add_to_other_mru (fn, cfg_mru_sessions);
    }

  }
}

void
MainWindow::cm_restore_session ()
{
  std::string fn = m_current_session;
  if (mp_session_fdia->get_open (fn, tl::to_string (QObject::tr ("Load Session File")))) {

    std::string df_list;
    int dirty_layouts = dirty_files (df_list);

    if (dirty_layouts == 0) {

      restore_session (fn);
      add_to_other_mru (fn, cfg_mru_sessions);

    } else {

      QMessageBox mbox (this);
      mbox.setText (tl::to_qstring (tl::to_string (QObject::tr ("The following layouts need saving:\n\n")) + df_list + "\n\nPress 'Discard Changes' to close them anyhow and discard changes."));
      mbox.setWindowTitle (QObject::tr ("Save Needed"));
      mbox.setIcon (QMessageBox::Warning);
      QAbstractButton *discard_button = mbox.addButton (QObject::tr ("Discard Changes"), QMessageBox::YesRole);
      mbox.addButton (QMessageBox::Cancel);

      mbox.exec ();

      if (mbox.clickedButton() == discard_button) {
        restore_session (fn);
        add_to_other_mru (fn, cfg_mru_sessions);
      }

    }

  }
}

void
MainWindow::cm_save_bookmarks ()
{
  if (current_view ()) {
    std::string fn;
    if (mp_bookmarks_fdia->get_save (fn, tl::to_string (QObject::tr ("Save Bookmarks File")))) {
      current_view ()->bookmarks ().save (fn);
      add_to_other_mru (fn, cfg_mru_bookmarks);
    }
  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open to save the bookmarks from")));
  }
}

void
MainWindow::cm_load_bookmarks ()
{
  if (current_view ()) {
    std::string fn;
    if (mp_bookmarks_fdia->get_open (fn, tl::to_string (QObject::tr ("Load Bookmarks File")))) {
      BookmarkList bookmarks;
      bookmarks.load (fn);
      current_view ()->bookmarks (bookmarks);
      add_to_other_mru (fn, cfg_mru_bookmarks);
    }
  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open to load the bookmarks for")));
  }
}

void
MainWindow::cm_screenshot ()
{
  if (current_view ()) {
    std::string fn;
    if (mp_screenshot_fdia->get_save (fn, tl::to_string (QObject::tr ("Save Screenshot")))) {
      current_view ()->save_screenshot (fn);
    }
  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open to create a screenshot from")));
  }
}

void
MainWindow::cm_screenshot_to_clipboard ()
{
  if (current_view ()) {
    QImage screenshot = current_view ()->get_screenshot ();
#if QT_VERSION >= 0x050000
    QClipboard *clipboard = QGuiApplication::clipboard();
#else
    QClipboard *clipboard = QApplication::clipboard();
#endif
    clipboard->setImage(screenshot);
  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open to create a screenshot from")));
  }
}

void
MainWindow::cm_save_current_cell_as ()
{
  if (current_view ()) {

    int cv_index = current_view ()->active_cellview_index ();
    if (cv_index >= 0 && cv_index < int (current_view ()->cellviews ())) {

      LayoutView::cell_path_type path;
      current_view ()->current_cell_path (path);
      if (! path.empty ()) {

        const lay::CellView &cv = current_view ()->cellview (cv_index);

        QFileInfo file_info (tl::to_qstring (cv->filename ()));
        std::string suffix = tl::to_string (file_info.suffix ());

        std::string fn = std::string (cv->layout ().cell_name (path.back ())) + "." + suffix;
        if (mp_layout_fdia->get_save (fn, tl::to_string (QObject::tr ("Save Layout File")))) {

          db::SaveLayoutOptions options (cv->save_options ());
          options.set_dbu (cv->layout ().dbu ());
          options.set_format_from_filename (fn);

          tl::OutputStream::OutputStreamMode om = tl::OutputStream::OM_Auto;
          if (mp_layout_save_as_options->get_options (current_view (), cv_index, fn, om, options)) {

            options.clear_cells ();

            std::vector<lay::LayoutView::cell_path_type> paths;
            current_view ()->selected_cells_paths (cv_index, paths);
            for (std::vector<lay::LayoutView::cell_path_type>::const_iterator p = paths.begin (); p != paths.end (); ++p) {
              if (! p->empty ()) {
                options.add_cell (p->back ());
              }
            }

            cv->save_as (fn, om, options, false /*don't update*/, m_keep_backups);

            add_mru (fn, cv->tech_name ());

          }

        }

      }

    }

  }
}

void
MainWindow::cm_save ()
{
  do_save (false);
}

void
MainWindow::cm_save_as ()
{
  do_save (true);
}

void
MainWindow::do_save (bool as)
{
  if (current_view ()) {

    std::vector<int> cv_indexes;
    if (current_view ()->cellviews () > 1) {
      SelectCellViewForm form (0, current_view (), tl::to_string (QObject::tr ("Select Layout To Save")), false /*multiple selection*/);
      form.set_selection (current_view ()->active_cellview_index ());
      if (form.exec () == QDialog::Accepted) {
        cv_indexes = form.selected_cellviews ();
      }
    } else if (current_view ()->cellviews () == 1) {
      cv_indexes.push_back (0);
    }

    if (! cv_indexes.empty ()) {

      std::string fn;
      for (std::vector<int>::const_iterator i = cv_indexes.begin (); i != cv_indexes.end (); ++i) {

        int cv_index = *i;
        const lay::CellView &cv = current_view ()->cellview (cv_index);

        fn = cv->filename ();

        if (! (as || fn.empty ()) || mp_layout_fdia->get_save (fn, tl::to_string (tr ("Save Layout '%1'").arg (tl::to_qstring (cv->name ()))))) {

          //  Here are the options we are going to take:
          //  - if the layout's save options are valid we take the options from there, otherwise we take the options from the technology
          //  - on "save as" we let the user edit the options

          db::SaveLayoutOptions options = cv->save_options ();
          if (!cv->save_options_valid () && cv->technology ()) {
            options = cv->technology ()->save_layout_options ();
            options.set_format (cv->save_options ().format ());
          }

          options.set_dbu (cv->layout ().dbu ());

          if (as || options.format ().empty ()) {
            options.set_format_from_filename (fn);
          }

          tl::OutputStream::OutputStreamMode om = tl::OutputStream::OM_Auto;

          if (as && ! mp_layout_save_as_options->get_options (current_view (), cv_index, fn, om, options)) {
            break;
          }

          current_view ()->save_as ((unsigned int) cv_index, fn, om, options, true, m_keep_backups);
          add_mru (fn, current_view ()->cellview (cv_index)->tech_name ());

        }

      }

    }

  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open to save")));
  }
}

void
MainWindow::cm_save_all ()
{
  for (int view_index = 0; view_index < int (views ()); ++view_index) {

    for (unsigned int cv_index = 0; cv_index < view (view_index)->cellviews (); ++cv_index) {

      const lay::CellView &cv = view (view_index)->cellview (cv_index);
      std::string fn = cv->filename ();

      if (! fn.empty () || mp_layout_fdia->get_save (fn, tl::to_string (tr ("Save Layout '%1'").arg (tl::to_qstring (cv->name ()))))) {

        db::SaveLayoutOptions options (cv->save_options ());
        options.set_dbu (cv->layout ().dbu ());

        if (options.format ().empty ()) {
          options.set_format_from_filename (fn);
        }

        tl::OutputStream::OutputStreamMode om = tl::OutputStream::OM_Auto;

        //  initialize the specific options from the configuration if required
        for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
          const StreamWriterPluginDeclaration *decl = dynamic_cast <const StreamWriterPluginDeclaration *> (&*cls);
          if (decl) {
            options.set_options (decl->create_specific_options ());
          }
        }

        view (view_index)->save_as (cv_index, fn, om, options, true, m_keep_backups);
        add_mru (fn, current_view ()->cellview (cv_index)->tech_name ());

      }

    }

  }
}

void
MainWindow::cm_setup ()
{
  mp_setup_form->show ();
  mp_setup_form->setup ();
}

void
MainWindow::view_selected (int index)
{
  if (index >= 0 && index < int (views ())) {

    //  Hint: setting the focus to the tab bar avoids problem with dangling keyboard focus.
    //  Sometimes, the focus was set to the hierarchy level spin buttons which caught Copy&Paste
    //  events in effect.
    mp_tab_bar->setFocus ();

    if (! m_disable_tab_selected) {
      select_view (index);
    }

  }
}

void
MainWindow::select_view (int index)
{
  bool dis = m_disable_tab_selected;
  m_disable_tab_selected = true; // prevent recursion

  try {

    cancel ();

    tl_assert (index >= 0 && index < int (views ()));

    mp_tab_bar->setCurrentIndex (index);

    bool box_set = (m_synchronized_views && current_view () != 0);
    db::DBox box;
    if (box_set) {
      box = current_view ()->viewport ().box ();
    }

    view (index)->set_current ();

    if (current_view ()) {

      if (box_set) {
        current_view ()->zoom_box (box);
      }

      mp_view_stack->raise_widget (index);
      mp_hp_stack->raise_widget (index);
      mp_layer_toolbox_stack->raise_widget (index);
      mp_lp_stack->raise_widget (index);
      mp_libs_stack->raise_widget (index);
      mp_eo_stack->raise_widget (index);
      mp_bm_stack->raise_widget (index);
      mp_setup_form->setup ();

    }

    current_view_changed ();

    clear_current_pos ();
    edits_enabled_changed ();
    clear_message ();
    menu_needs_update ();

    m_disable_tab_selected = dis;

  } catch (...) {
    m_disable_tab_selected = dis;
    throw;
  }
}

void
MainWindow::cm_open_too ()
{
  open (2);
}

void
MainWindow::cm_open_new_view ()
{
  open (1);
}

void
MainWindow::cm_open ()
{
  open (0);
}

void
MainWindow::cm_pull_in ()
{
  std::vector <std::string> names;
  lay::LayoutHandle::get_names (names);

  QStringList layouts;
  for (std::vector <std::string>::const_iterator n = names.begin (); n != names.end (); ++n) {
    layouts << tl::to_qstring (*n);
  }

  if (layouts.size () == 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layouts loaded")));
  }

  bool ok = false;
  QString item = QInputDialog::getItem (this, QObject::tr ("Choose Layout"),
                                        QObject::tr ("Choose an existing layout for being opened in the current view\nadditionally to the layouts already shown"),
                                        layouts, 0, false, &ok);
  if (ok) {

    lay::LayoutHandle *layout_handle = lay::LayoutHandle::find (tl::to_string (item));
    if (layout_handle) {

      if (! current_view ()) {
        create_view ();
      }

      if (current_view ()) {

        //  If there is another view holding that layout already, take the layer properties from there
        int other_cv_index = -1;
        const lay::LayoutView *other_view = 0;
        for (unsigned int i = 0; i < views () && other_cv_index < 0; ++i) {
          for (unsigned int cvi = 0; cvi < view (i)->cellviews () && other_cv_index < 0; ++cvi) {
            if (view (i)->cellview (cvi).handle () == layout_handle) {
              other_view = view (i);
              other_cv_index = int (cvi);
            }
          }
        }

        if (! other_view) {
          current_view ()->add_layout (layout_handle, true);
        } else {

          unsigned int cv_index = current_view ()->add_layout (layout_handle, true, false /*don't initialize layers*/);

          std::vector<lay::LayerPropertiesList> other_props;
          for (unsigned int i = 0; i < other_view->layer_lists (); ++i) {
            other_props.push_back (other_view->get_properties (i));
            other_props.back ().remove_cv_references (other_cv_index, true);
            other_props.back ().translate_cv_references (cv_index);
          }

          current_view ()->merge_layer_props (other_props);

        }

      }

    }

  }
}

void
MainWindow::cm_reader_options ()
{
  mp_layout_load_options->edit_global_options (dispatcher (), db::Technologies::instance ());
}

void
MainWindow::cm_writer_options ()
{
  mp_layout_save_options->edit_global_options (dispatcher (), db::Technologies::instance ());
}

void
MainWindow::cm_new_panel ()
{
  create_view ();
}

void
MainWindow::cm_new_layout ()
{
  std::string technology = m_initial_technology;
  static std::string s_new_cell_cell_name ("TOP");
  static double s_new_cell_window_size = 2.0;
  static std::vector<db::LayerProperties> s_layers;

  double dbu = 0.0;

  lay::NewLayoutPropertiesDialog dialog (this);
  if (dialog.exec_dialog (technology, s_new_cell_cell_name, dbu, s_new_cell_window_size, s_layers, m_new_layout_current_panel)) {

    std::unique_ptr <lay::LayoutHandle> handle (new lay::LayoutHandle (new db::Layout (& manager ()), std::string ()));
    handle->layout ().set_technology_name (technology);
    handle->rename ("new");

    if (dbu > 1e-10) {
      handle->layout ().dbu (dbu);
    }
    db::cell_index_type new_ci = handle->layout ().add_cell (s_new_cell_cell_name.empty () ? 0 : s_new_cell_cell_name.c_str ());

    for (std::vector<db::LayerProperties>::const_iterator l = s_layers.begin (); l != s_layers.end (); ++l) {
      handle->layout ().insert_layer (*l);
    }

    lay::LayoutView *mp_view = (m_new_layout_current_panel && current_view ()) ? current_view () : view (create_view ());

    unsigned int ci = mp_view->add_layout (handle.release (), true);
    mp_view->cellview_ref (ci).set_cell (new_ci);
    mp_view->zoom_box_and_set_hier_levels (db::DBox (-0.5 * s_new_cell_window_size, -0.5 * s_new_cell_window_size, 0.5 * s_new_cell_window_size, 0.5 * s_new_cell_window_size), std::make_pair (0, 1));

  }
}

void
MainWindow::cm_clone ()
{
  clone_current_view ();
}

void
MainWindow::clone_current_view ()
{
  lay::LayoutViewWidget *view_widget = 0;
  lay::LayoutView *curr = current_view ();
  if (! curr) {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open to clone")));
  }

  //  create a new view
  view_widget = new lay::LayoutViewWidget (current_view (), &m_manager, lay::ApplicationBase::instance ()->is_editable (), dispatcher (), mp_view_stack);
  add_view (view_widget);

  lay::LayoutView *view = view_widget->view ();

  //  set initial attributes
  view->set_hier_levels (curr->get_hier_levels ());

  //  select the current mode and select the enabled editables
  view->mode (m_mode);

  //  copy the state
  lay::DisplayState state;
  current_view ()->save_view (state);
  view->goto_view (state);

  //  initialize the state stack
  view->clear_states ();
  view->store_state ();

  view->update_content ();

  mp_views.back ()->view ()->set_current ();

  mp_view_stack->add_widget (view_widget);
  mp_lp_stack->add_widget (view_widget->layer_control_frame ());
  mp_layer_toolbox_stack->add_widget (view_widget->layer_toolbox_frame ());
  mp_hp_stack->add_widget (view_widget->hierarchy_control_frame ());
  mp_libs_stack->add_widget (view_widget->libraries_frame ());
  mp_eo_stack->add_widget (view_widget->editor_options_frame ());
  mp_bm_stack->add_widget (view_widget->bookmarks_frame ());

  bool f = m_disable_tab_selected;
  m_disable_tab_selected = true;
  int index = mp_tab_bar->insertTab (-1, tl::to_qstring (view->title ()));
  m_disable_tab_selected = f;

  view_created_event (index);
  select_view (index);

  update_dock_widget_state ();
}

void
MainWindow::cm_close_all ()
{
  interactive_close_view (0, views (), false, false);
}

void
MainWindow::cm_close ()
{
  int current_index = index_of (lay::LayoutView::current ());
  if (current_index >= 0) {
    interactive_close_view (current_index, current_index + 1, false, false);
  }
}

void
MainWindow::tab_close_requested (int index)
{
  interactive_close_view (index, index + 1, false, true);
}

void
MainWindow::interactive_close_view (int from, int to, bool invert_range, bool all_cellviews)
{
  //  closes views in range [from, to[ (invert_range=false) or outside this range (invert_range=true)
  if (invert_range || from + 1 != to) {

    bool can_close = true;

    int dirty_layouts = 0;
    std::string dirty_files;
    std::set<std::string> seen_names;

    for (int index = 0; index < int (views ()); ++index) {

      if ((index >= from && index < to) == invert_range) {
        continue;
      }

      for (unsigned int i = 0; i < view (index)->cellviews (); ++i) {

        const lay::CellView &cv = view (index)->cellview (i);

        if (cv->layout ().is_editable () && cv->is_dirty ()) {

          std::string name = cv->name ();
          if (seen_names.find (name) != seen_names.end ()) {
            continue;
          }

          seen_names.insert (name);

          ++dirty_layouts;
          if (dirty_layouts == max_dirty_files) {
            dirty_files += "\n...";
          } else if (dirty_layouts < max_dirty_files) {
            if (! dirty_files.empty ()) {
              dirty_files += "\n";
            }
            dirty_files += name;
          }

        }

      }

    }

    if (dirty_layouts != 0) {

      QMessageBox mbox (this);
      mbox.setText (tl::to_qstring (tl::to_string (QObject::tr ("The following layouts need saving:\n\n")) + dirty_files + tl::to_string (QObject::tr ("\n\nPress 'Close Without Saving' to close them anyway and discard changes\n")))),
      mbox.setWindowTitle (QObject::tr ("Save Needed"));
      mbox.setIcon (QMessageBox::Warning);
      QAbstractButton *can_close_button = mbox.addButton (QObject::tr ("Close Without Saving"), QMessageBox::YesRole);
      mbox.addButton (QMessageBox::Cancel);

      mbox.exec ();

      can_close = (mbox.clickedButton() == can_close_button);

    }

    if (can_close) {
      BEGIN_PROTECTED
      for (int index = int (views ()); index > 0; ) {
        --index;
        if ((index >= from && index < to) != invert_range) {
          close_view (index);
        }
      }
      END_PROTECTED
    }

  } else if (view (from)) {

    std::vector <int> selected;

    if (view (from)->cellviews () > 1) {

      if (all_cellviews) {

        for (int i = 0; i < int (view (from)->cellviews ()); ++i) {
          selected.push_back (i);
        }

      } else {

        SelectCellViewForm form (0, view (from), tl::to_string (QObject::tr ("Select Layouts To Close")));
        form.set_selection (view (from)->active_cellview_index ());

        if (form.exec () != QDialog::Accepted) {
          return;
        }

        selected = form.selected_cellviews ();
        if (selected.empty ()) {
          return;
        }

      }

    } else if (view (from)->cellviews () > 0) {
      selected.push_back (0);
    }

    if (selected.size () > 0) {

      int dirty_layouts = 0;
      std::string dirty_files;

      for (std::vector <int>::const_iterator i = selected.begin (); i != selected.end (); ++i) {

        const lay::CellView &cv = view (from)->cellview (*i);

        if (cv->layout ().is_editable () && cv->is_dirty ()) {

          std::string name = cv->name ();

          int count = 0;

          for (std::vector <lay::LayoutViewWidget *>::const_iterator v = mp_views.begin (); v != mp_views.end (); ++v) {
            for (unsigned int cvi = 0; cvi < (*v)->view ()->cellviews (); ++cvi) {
              if ((*v)->view ()->cellview (cvi)->name () == name) {
                ++count;
              }
            }
          }

          for (std::vector <int>::const_iterator ii = selected.begin (); ii != selected.end (); ++ii) {
            if (view (from)->cellview (*ii)->name () == name) {
              --count;
            }
          }

          //  only report layouts as dirty which will vanish if we would close all layouts
          if (count <= 0) {
            ++dirty_layouts;
            if (dirty_layouts == max_dirty_files) {
              dirty_files += "\n...";
            } else if (dirty_layouts < max_dirty_files) {
              if (! dirty_files.empty ()) {
                dirty_files += "\n";
              }
              dirty_files += name;
            }
          }

        }

      }

      bool can_close = true;
      if (dirty_layouts != 0) {

        QMessageBox mbox (this);
        mbox.setText (tl::to_qstring (tl::to_string (QObject::tr ("The following layouts need saving:\n\n")) + dirty_files + tl::to_string (QObject::tr ("\n\nPress 'Close Without Saving' to close them anyway and discard changes\n")))),
        mbox.setWindowTitle (QObject::tr ("Save Needed"));
        mbox.setIcon (QMessageBox::Warning);
        QAbstractButton *can_close_button = mbox.addButton (QObject::tr ("Close Without Saving"), QMessageBox::YesRole);
        mbox.addButton (QMessageBox::Cancel);

        mbox.exec ();

        can_close = (mbox.clickedButton() == can_close_button);

      }

      if (can_close) {

        BEGIN_PROTECTED

        //  Actually erase the selected cellviews
        if (view (from)->cellviews () == selected.size ()) {
          //  all cellviews selected - simply close
          close_view (from);
        } else {
          std::sort (selected.begin (), selected.end ());
          int offset = 0;
          for (std::vector <int>::const_iterator i = selected.begin (); i != selected.end (); ++i) {
            view (from)->erase_cellview ((unsigned int)(*i - offset));
            ++offset;
          }
        }

        END_PROTECTED

      }

    } else {
      close_view (from);
    }

  }

}

void
MainWindow::clone ()
{
  clone_current_view ();
}

void
MainWindow::close_current_view ()
{
  close_view (index_of (lay::LayoutView::current ()));
}

void
MainWindow::close_all_views ()
{
  cm_close_all ();
}

void
MainWindow::close_all_except_this ()
{
  int index = mp_tab_bar->tabAt (m_mouse_pos);
  if (index >= 0) {
    interactive_close_view (index, index + 1, true, false);
  }
}

void
MainWindow::close_all_views_left ()
{
  int index = mp_tab_bar->tabAt (m_mouse_pos);
  if (index >= 0) {
    interactive_close_view (0, index, false, false);
  }
}

void
MainWindow::close_all_views_right ()
{
  int index = mp_tab_bar->tabAt (m_mouse_pos);
  if (index >= 0) {
    interactive_close_view (index + 1, views (), false, false);
  }
}

void
MainWindow::close_view (int index)
{
  if (view (index)) {

    cancel ();

    //  this suppresses view_selected events that would otherwise be created
    bool f = m_disable_tab_selected;
    m_disable_tab_selected = true;

    BEGIN_PROTECTED

      db::DBox box;
      if (m_synchronized_views) {
        box = view (index)->viewport ().box ();
      }

      mp_tab_bar->removeTab (index);
      mp_view_stack->remove_widget (index);
      mp_lp_stack->remove_widget (index);
      mp_layer_toolbox_stack->remove_widget (index);
      mp_hp_stack->remove_widget (index);
      mp_libs_stack->remove_widget (index);
      mp_eo_stack->remove_widget (index);
      mp_bm_stack->remove_widget (index);

      view_closed_event (int (index));

      //  delete the view later as it may still be needed by event handlers or similar
      std::unique_ptr<lay::LayoutViewWidget> old_view (mp_views [index]);

      mp_views.erase (mp_views.begin () + index, mp_views.begin () + index + 1);

      if (index >= int (mp_views.size ())) {
        --index;
      }

      if (index >= 0) {

        select_view (index);

      } else {

        //  last view closed

        lay::LayoutView::set_current (0);
        current_view_changed ();

        clear_current_pos ();
        edits_enabled_changed ();
        menu_needs_update ();
        clear_message ();

        update_dock_widget_state ();

      }

    END_PROTECTED

    m_disable_tab_selected = f;

  }
}

void
MainWindow::add_mru (const std::string &fn_rel)
{
  add_mru (fn_rel, m_initial_technology);
}

const size_t max_mru = 16; //  TODO: make configurable?

void
MainWindow::add_mru (const std::string &fn_rel, const std::string &tech)
{
  std::vector <std::pair<std::string, std::string> > new_mru;
  std::string fn (tl::InputStream::absolute_path (fn_rel));

  for (auto mru = m_mru.begin (); mru != m_mru.end (); ++mru) {
    if (mru->first != fn /* delete non-existing files: && tl::is_readable (mru->first) */) {
      new_mru.push_back (*mru);
    }
  }

  new_mru.push_back (std::make_pair (fn, tech));

  if (new_mru.size () > max_mru) {
    new_mru.erase (new_mru.begin (), new_mru.end () - max_mru);
  }

  std::string config_str;
  for (auto mru = new_mru.begin (); mru != new_mru.end (); ++mru) {
    if (! config_str.empty ()) {
      config_str += " ";
    }
    config_str += tl::to_quoted_string (mru->first);
    if (! mru->second.empty ()) {
      config_str += "@";
      config_str += tl::to_quoted_string (mru->second);
    }
  }

  dispatcher ()->config_set (cfg_mru, config_str);
}

void
MainWindow::add_to_other_mru (const std::string &fn_rel, const std::string &cfg)
{
  std::vector <std::string> *mru_ptr;
  if (cfg == cfg_mru_sessions) {
    mru_ptr = &m_mru_sessions;
  } else if (cfg == cfg_mru_layer_properties) {
    mru_ptr = &m_mru_layer_properties;
  } else if (cfg == cfg_mru_bookmarks) {
    mru_ptr = &m_mru_bookmarks;
  } else {
    tl_assert (false);
  }

  std::vector <std::string> new_mru;
  std::string fn (tl::InputStream::absolute_path (fn_rel));

  for (auto mru = mru_ptr->begin (); mru != mru_ptr->end (); ++mru) {
    if (*mru != fn /* delete non-existing files: && tl::is_readable (*mru) */) {
      new_mru.push_back (*mru);
    }
  }

  new_mru.push_back (fn);

  if (new_mru.size () > max_mru) {
    new_mru.erase (new_mru.begin (), new_mru.end () - max_mru);
  }

  std::string config_str;
  for (std::vector<std::string>::const_iterator mru = new_mru.begin (); mru != new_mru.end (); ++mru) {
    if (! config_str.empty ()) {
      config_str += " ";
    }
    config_str += tl::to_quoted_string (*mru);
  }

  dispatcher ()->config_set (cfg, config_str);
}

namespace
{

class OpenRecentAction
  : public lay::Action
{
public:
  OpenRecentAction (lay::MainWindow *mw, size_t n, void (lay::MainWindow::*open_meth) (size_t), bool (lay::MainWindow::*avail_meth) (size_t))
    : lay::Action (), mp_mw (mw), m_n (n), m_open_meth (open_meth), m_avail_meth (avail_meth)
  { }

  void triggered ()
  {
    (mp_mw->*m_open_meth) (m_n);
  }

  bool wants_enabled () const
  {
    return (mp_mw->*m_avail_meth) (m_n);
  }

private:
  lay::MainWindow *mp_mw;
  size_t m_n;
  void (lay::MainWindow::*m_open_meth) (size_t);
  bool (lay::MainWindow::*m_avail_meth) (size_t);
};

class ClearRecentAction
  : public lay::Action
{
public:
  ClearRecentAction (lay::MainWindow *mw, const std::string &cfg)
    : lay::Action (), mp_mw (mw), m_cfg (cfg)
  {
    set_title (tl::to_string (tr ("Clear List")));
  }

  void triggered ()
  {
    dispatcher ()->config_set (m_cfg, std::string ());
  }

private:
  lay::MainWindow *mp_mw;
  std::string m_cfg;
};

}

void
MainWindow::do_update_mru_menus ()
{
  std::string mru_menu = "file_menu.open_recent_menu";

  if (menu ()->is_valid (mru_menu)) {

    Action *open_recent_action = menu ()->action (mru_menu);
    open_recent_action->set_enabled (true);

    if (m_mru.size () > 0 && edits_enabled ()) {

      //  rebuild MRU menu
      menu ()->clear_menu (mru_menu);

      for (std::vector<std::pair<std::string, std::string> >::iterator mru = m_mru.end (); mru != m_mru.begin (); ) {
        --mru;
        size_t i = std::distance (m_mru.begin (), mru);
        Action *action = new OpenRecentAction (this, i, &lay::MainWindow::open_recent, &lay::MainWindow::is_available_recent);
        action->set_title (mru->first);
        menu ()->insert_item (mru_menu + ".end", tl::sprintf ("open_recent_%d", i + 1), action);
      }

      menu ()->insert_separator (mru_menu + ".end", "clear_sep");
      menu ()->insert_item (mru_menu + ".end", "clear_recent", new ClearRecentAction (this, cfg_mru));

    } else {
      open_recent_action->set_enabled (false);
    }

  }

  mru_menu = "file_menu.open_recent_menu_sessions";

  if (menu ()->is_valid (mru_menu)) {

    Action *open_recent_action = menu ()->action (mru_menu);
    open_recent_action->set_enabled (true);

    if (m_mru_sessions.size () > 0 && edits_enabled ()) {

      //  rebuild MRU menu
      menu ()->clear_menu (mru_menu);

      for (std::vector<std::string>::iterator mru = m_mru_sessions.end (); mru != m_mru_sessions.begin (); ) {
        --mru;
        size_t i = std::distance (m_mru_sessions.begin (), mru);
        Action *action = new OpenRecentAction (this, i, &lay::MainWindow::open_recent_session, &lay::MainWindow::is_available_recent_session);
        action->set_title (*mru);
        menu ()->insert_item (mru_menu + ".end", tl::sprintf ("open_recent_%d", i + 1), action);
      }

      menu ()->insert_separator (mru_menu + ".end", "clear_sep");
      menu ()->insert_item (mru_menu + ".end", "clear_recent", new ClearRecentAction (this, cfg_mru_sessions));

    } else {
      open_recent_action->set_enabled (false);
    }

  }

  mru_menu = "file_menu.open_recent_menu_layer_props";

  if (menu ()->is_valid (mru_menu)) {

    Action *open_recent_action = menu ()->action (mru_menu);
    open_recent_action->set_enabled (true);

    if (m_mru_layer_properties.size () > 0 && edits_enabled ()) {

      //  rebuild MRU menu
      menu ()->clear_menu (mru_menu);

      for (std::vector<std::string>::iterator mru = m_mru_layer_properties.end (); mru != m_mru_layer_properties.begin (); ) {
        --mru;
        size_t i = std::distance (m_mru_layer_properties.begin (), mru);
        Action *action = new OpenRecentAction (this, i, &lay::MainWindow::open_recent_layer_properties, &lay::MainWindow::is_available_recent_layer_properties);
        action->set_title (*mru);
        menu ()->insert_item (mru_menu + ".end", tl::sprintf ("open_recent_%d", i + 1), action);
      }

      menu ()->insert_separator (mru_menu + ".end", "clear_sep");
      menu ()->insert_item (mru_menu + ".end", "clear_recent", new ClearRecentAction (this, cfg_mru_layer_properties));

    } else {
      open_recent_action->set_enabled (false);
    }

  }

  mru_menu = "bookmark_menu.open_recent_menu_bookmarks";

  if (menu ()->is_valid (mru_menu)) {

    Action *open_recent_action = menu ()->action (mru_menu);
    open_recent_action->set_enabled (true);

    if (m_mru_bookmarks.size () > 0 && edits_enabled ()) {

      //  rebuild MRU menu
      menu ()->clear_menu (mru_menu);

      for (std::vector<std::string>::iterator mru = m_mru_bookmarks.end (); mru != m_mru_bookmarks.begin (); ) {
        --mru;
        size_t i = std::distance (m_mru_bookmarks.begin (), mru);
        Action *action = new OpenRecentAction (this, i, &lay::MainWindow::open_recent_bookmarks, &lay::MainWindow::is_available_recent_bookmarks);
        action->set_title (*mru);
        menu ()->insert_item (mru_menu + ".end", tl::sprintf ("open_recent_%d", i + 1), action);
      }

      menu ()->insert_separator (mru_menu + ".end", "clear_sep");
      menu ()->insert_item (mru_menu + ".end", "clear_recent", new ClearRecentAction (this, cfg_mru_bookmarks));

    } else {
      open_recent_action->set_enabled (false);
    }

  }
}

void
MainWindow::open_recent (size_t n)
{
  BEGIN_PROTECTED

  if (n >= m_mru.size ()) {
    return;
  }

  OpenLayoutModeDialog open_mode_dialog (this);

  if (views () != 0 && ! open_mode_dialog.exec_dialog (m_open_mode)) {
    return;
  }

  if (mp_layout_load_options->show_always () && !mp_layout_load_options->edit_global_options (dispatcher (), db::Technologies::instance ())) {
    return;
  }

  std::string fn (m_mru [n].first);  //  create a copy since we change m_mru later
  std::string tech (m_mru [n].second);

  bool can_open = true;

  if (m_open_mode == 0) {

    std::string df_list;
    int dirty_layouts = dirty_files (df_list);

    if (dirty_layouts != 0) {

      QMessageBox mbox (this);
      mbox.setText (tl::to_qstring (tl::to_string (QObject::tr ("The following layouts need saving:\n\n")) + df_list + "\n\nPress 'Close Without Saving' to open the layout and discard changes."));
      mbox.setWindowTitle (QObject::tr ("Save Needed"));
      mbox.setIcon (QMessageBox::Warning);
      QAbstractButton *yes_button = mbox.addButton (QObject::tr ("Close Without Saving"), QMessageBox::YesRole);
      mbox.addButton (QMessageBox::Cancel);

      mbox.exec ();

      can_open = (mbox.clickedButton() == yes_button);

    }

  }

  if (can_open) {
    load_layout (fn, tech, m_open_mode);
    add_mru (fn, tech);  //  make it the latest
  }

  END_PROTECTED
}

static bool
is_file_available (const std::string &fn)
{
  tl::URI uri (fn);
  if (uri.scheme ().empty ()) {
    return tl::is_readable (fn);
  } else if (uri.scheme () == "file") {
    return tl::is_readable (uri.path ());
  } else {
    return true;
  }
}

bool
MainWindow::is_available_recent (size_t n)
{
  return (n < m_mru.size () && is_file_available (m_mru [n].first));
}

void
MainWindow::open_recent_session (size_t n)
{
  BEGIN_PROTECTED

  if (n < m_mru_sessions.size ()) {
    std::string fn = m_mru_sessions [n];
    restore_session (fn);
    add_to_other_mru (fn, cfg_mru_sessions);  //  make it the latest
  }

  END_PROTECTED
}

bool
MainWindow::is_available_recent_session (size_t n)
{
  return (n < m_mru_sessions.size () && is_file_available (m_mru_sessions [n]));
}

void
MainWindow::open_recent_layer_properties (size_t n)
{
  BEGIN_PROTECTED

  if (n < m_mru_layer_properties.size ()) {
    std::string fn = m_mru_layer_properties [n];
    load_layer_props_from_file (fn);
    add_to_other_mru (fn, cfg_mru_layer_properties);  //  make it the latest
  }

  END_PROTECTED
}

bool
MainWindow::is_available_recent_layer_properties (size_t n)
{
  return (n < m_mru_layer_properties.size () && is_file_available (m_mru_layer_properties [n]));
}

void
MainWindow::open_recent_bookmarks (size_t n)
{
  BEGIN_PROTECTED

  if (n < m_mru_bookmarks.size ()) {
    std::string fn = m_mru_bookmarks [n];
    if (current_view ()) {
      BookmarkList bookmarks;
      bookmarks.load (fn);
      current_view ()->bookmarks (bookmarks);
      add_to_other_mru (fn, cfg_mru_bookmarks);
    }
  }

  END_PROTECTED
}

bool
MainWindow::is_available_recent_bookmarks (size_t n)
{
  return (n < m_mru_bookmarks.size () && is_file_available (m_mru_bookmarks [n]));
}

void
MainWindow::open (int mode)
{
  BEGIN_PROTECTED

  static std::vector<std::string> files;
  if (! mp_layout_fdia->get_open (files, std::string (), tl::to_string (QObject::tr ("Open Layout Files")))) {
    return;
  }

  if (mp_layout_load_options->show_always () && !mp_layout_load_options->edit_global_options (dispatcher (), db::Technologies::instance ())) {
    return;
  }

  bool can_open = true;

  if (mode == 0) {

    std::string df_list;
    int dirty_layouts = dirty_files (df_list);

    if (dirty_layouts != 0) {

      QMessageBox mbox (this);
      mbox.setText (tl::to_qstring (tl::to_string (QObject::tr ("The following layouts need saving:\n\n")) + df_list + "\n\nPress 'Close Without Saving' to open the layout and discard changes."));
      mbox.setWindowTitle (QObject::tr ("Save Needed"));
      mbox.setIcon (QMessageBox::Warning);
      QAbstractButton *yes_button = mbox.addButton (QObject::tr ("Close Without Saving"), QMessageBox::YesRole);
      mbox.addButton (QMessageBox::Cancel);

      mbox.exec ();

      can_open = (mbox.clickedButton() == yes_button);

    }

  }

  if (can_open) {

    for (std::vector<std::string>::const_iterator fn = files.begin (); fn != files.end (); ++fn) {
      load_layout (*fn, m_initial_technology, mode);
      //  open next layout in "add view" mode, if the current view was overridden -
      //  otherwise that would happen once again.
      if (mode == 0) {
        mode = 1;
      }
      add_mru (*fn, m_initial_technology);
    }

  }

  END_PROTECTED
}

void
MainWindow::reload_layout (unsigned int cv_index)
{
  lay::LayoutView *view = current_view ();

  if (view && view->cellviews () > cv_index) {
    view->reload_layout (cv_index);
  }
}

lay::CellViewRef
MainWindow::load_layout (const std::string &filename, const db::LoadLayoutOptions &options, const std::string &technology, int mode)
{
  return create_or_load_layout (&filename, &options, technology, mode);
}

lay::CellViewRef
MainWindow::create_layout (const std::string &technology, int mode)
{
  return create_or_load_layout (0, 0, technology, mode);
}

void
MainWindow::add_view (lay::LayoutViewWidget *view)
{
  connect (view, SIGNAL (title_changed (lay::LayoutView *)), this, SLOT (view_title_changed (lay::LayoutView *)));
  connect (view, SIGNAL (dirty_changed (lay::LayoutView *)), this, SLOT (view_title_changed (lay::LayoutView *)));
  connect (view, SIGNAL (edits_enabled_changed ()), this, SLOT (edits_enabled_changed ()));
  connect (view, SIGNAL (menu_needs_update ()), this, SLOT (menu_needs_update ()));
  connect (view, SIGNAL (show_message (const std::string &, int)), this, SLOT (message (const std::string &, int)));
  connect (view, SIGNAL (current_pos_changed (double, double, bool)), this, SLOT (current_pos (double, double, bool)));
  connect (view, SIGNAL (clear_current_pos ()), this, SLOT (clear_current_pos ()));
  connect (view, SIGNAL (mode_change (int)), this, SLOT (select_mode (int)));

  mp_views.push_back (view);

  //  we must resize the widget here to set the geometry properly.
  //  This is required to make zoom_fit work.
  view->setGeometry (0, 0, mp_view_stack->width (), mp_view_stack->height ());
  view->show ();
}

int
MainWindow::do_create_view ()
{
  //  create a new view
  lay::LayoutViewWidget *view_widget = new lay::LayoutViewWidget (&m_manager, lay::ApplicationBase::instance ()->is_editable (), dispatcher (), mp_view_stack);
  add_view (view_widget);

  lay::LayoutView *view = view_widget->view ();

  //  set initial attributes
  view->set_synchronous (synchronous ());

  int tl = 0;
  dispatcher ()->config_get (cfg_initial_hier_depth, tl);
  view->set_hier_levels (std::make_pair (0, tl));

  //  select the current mode and select the enabled editables
  view->mode (m_mode);

  //  initialize the state stack
  view->clear_states ();
  view->store_state ();

  return int (mp_views.size () - 1);
}

int
MainWindow::create_view ()
{
  //  create a new view
  int view_index = do_create_view ();

  //  add a new tab and make the new view the current one
  mp_views.back ()->view ()->set_current ();

  mp_view_stack->add_widget (mp_views.back ());
  mp_lp_stack->add_widget (mp_views.back ()->layer_control_frame ());
  mp_layer_toolbox_stack->add_widget (mp_views.back ()->layer_toolbox_frame ());
  mp_hp_stack->add_widget (mp_views.back ()->hierarchy_control_frame ());
  mp_libs_stack->add_widget (mp_views.back ()->libraries_frame ());
  mp_eo_stack->add_widget (mp_views.back ()->editor_options_frame ());
  mp_bm_stack->add_widget (mp_views.back ()->bookmarks_frame ());

  bool f = m_disable_tab_selected;
  m_disable_tab_selected = true;
  int index = mp_tab_bar->insertTab (-1, tl::to_qstring (current_view ()->title ()));
  m_disable_tab_selected = f;

  view_created_event (index);
  select_view (index);

  update_dock_widget_state ();

  return view_index;
}

lay::CellViewRef
MainWindow::create_or_load_layout (const std::string *filename, const db::LoadLayoutOptions *options, const std::string &technology, int mode)
{
  lay::LayoutView *vw = 0;

  if (! current_view ()) {
    mode = 1;
  }

  if (mode == 1) {
    //  create a new view
    vw = view (do_create_view ());
  } else {
    //  take the current view
    vw = current_view ();
    if (mode == 0) {
      //  reset the hierarchy depth in the "replace" case
      int tl = 0;
      dispatcher ()->config_get (cfg_initial_hier_depth, tl);
      vw->set_hier_levels (std::make_pair (0, tl));
      vw->clear_states ();
      vw->store_state ();
    }
  }

  unsigned int cv_index = 0;

  try {

    //  load or create the layout
    if (filename != 0) {
      tl_assert (options != 0);
      cv_index = vw->load_layout (*filename, *options, technology, mode == 2);
    } else {
      cv_index = vw->create_layout (technology, mode == 2);
    }

    //  make the new view the current one
    if (mode == 1) {

      mp_views.back ()->view ()->set_current ();

      mp_view_stack->add_widget (mp_views.back ());
      mp_lp_stack->add_widget (mp_views.back ()->layer_control_frame ());
      mp_layer_toolbox_stack->add_widget (mp_views.back ()->layer_toolbox_frame ());
      mp_hp_stack->add_widget (mp_views.back ()->hierarchy_control_frame ());
      mp_libs_stack->add_widget (mp_views.back ()->libraries_frame ());
      mp_eo_stack->add_widget (mp_views.back ()->editor_options_frame ());
      mp_bm_stack->add_widget (mp_views.back ()->bookmarks_frame ());

      bool f = m_disable_tab_selected;
      m_disable_tab_selected = true;
      int index = mp_tab_bar->insertTab (-1, QString ());
      update_tab_title (index);
      m_disable_tab_selected = f;
      view_created_event (index);
      select_view (index);

    } else if (mode == 0 || mode == 2) {
      update_tab_title (index_of (current_view ()));
    }

    update_dock_widget_state ();

  } catch (...) {

    //  clean up in case of an error ..
    if (mode == 1) {
      delete mp_views.back ();
      mp_views.pop_back ();
    }

    throw;

  }

  return vw->cellview_ref (cv_index);
}

void
MainWindow::update_tab_title (int i)
{
  std::string title;

  lay::LayoutView *v = view (i);
  if (v) {
    if (v->is_dirty ()) {
      title += "[+] ";
    }
    title += v->title ();
  }

  if (tl::to_string (mp_tab_bar->tabText (i)) != title) {
    mp_tab_bar->setTabText (i, tl::to_qstring (title));
  }

  if (v) {
    std::string files;
    for (unsigned int cv = 0; cv < v->cellviews (); ++cv) {
      if (! files.empty ()) {
        files += "\n";
      }
      if (! v->cellview (cv)->filename ().empty ()) {
        files += v->cellview (cv)->filename ();
      } else {
        files += tl::to_string (tr ("(not saved)"));
      }
    }
    if (tl::to_string (mp_tab_bar->tabToolTip (i)) != files) {
      mp_tab_bar->setTabToolTip (i, tl::to_qstring (files));
    }
  }
}

void
MainWindow::view_title_changed (lay::LayoutView *view)
{
  int i = index_of (view);
  if (i >= 0) {
    update_tab_title (i);
  }

  if (view == current_view ()) {
    update_window_title ();
  }
}

void
MainWindow::set_title (const std::string &title)
{
  if (title != m_title) {
    m_title = title;
    update_window_title ();
  }
}

void
MainWindow::update_window_title ()
{
  std::string title = m_title;

  if (! title.empty ()) {
    tl::Eval eval;
    title = eval.interpolate (title);
  } else {
    title = lay::ApplicationBase::version ();
    if (current_view ()) {
      std::string sep = " - ";
      if (current_view ()->is_dirty ()) {
        sep += "[+] ";
      }
      title += sep + current_view ()->title ();
    }
  }

  setWindowTitle (tl::to_qstring (title));
}

void
MainWindow::current_view_changed ()
{
  update_window_title ();
  current_view_changed_event ();
}

double
MainWindow::grid_micron () const
{
  return m_grid_micron;
}

void
MainWindow::set_hier_levels (std::pair<int, int> l)
{
  if (current_view () && l != get_hier_levels ()) {
    current_view ()->set_hier_levels (l);
  }
}

std::pair<int, int>
MainWindow::get_hier_levels () const
{
  if (current_view ()) {
    return current_view ()->get_hier_levels ();
  } else {
    int tl = 0;
    dispatcher ()->config_get (cfg_initial_hier_depth, tl);
    return std::make_pair (0, tl);
  }
}

bool
MainWindow::has_prev_display_state ()
{
  if (current_view ()) {
    return current_view ()->has_prev_display_state ();
  } else {
    return false;
  }
}

bool
MainWindow::has_next_display_state ()
{
  if (current_view ()) {
    return current_view ()->has_next_display_state ();
  } else {
    return false;
  }
}

void
MainWindow::set_synchronous (bool sync_mode)
{
  m_synchronous = sync_mode;
  for (std::vector <lay::LayoutViewWidget *>::iterator vp = mp_views.begin (); vp != mp_views.end (); ++vp) {
    (*vp)->view ()->set_synchronous (sync_mode);
  }
}

void
MainWindow::current_pos (double x, double y, bool dbu_units)
{
  mp_cpx_label->setText (tl::to_qstring (dbu_units ? tl::db_to_string (x) : tl::micron_to_string (x)));
  mp_cpy_label->setText (tl::to_qstring (dbu_units ? tl::db_to_string (y) : tl::micron_to_string (y)));
}

void
MainWindow::clear_current_pos ()
{
  mp_cpx_label->setText (QString ());
  mp_cpy_label->setText (QString ());
}

QWidget *
MainWindow::progress_get_widget () const
{
  if (mp_progress_dialog) {
    return mp_progress_dialog->get_widget ();
  } else if ( mp_progress_widget) {
    return mp_progress_widget->get_widget ();
  } else {
    return 0;
  }
}

bool
MainWindow::update_progress (tl::Progress *progress)
{
  if (mp_progress_dialog) {

    mp_progress_dialog->set_progress (progress);
    return true;

  } else if (isVisible () && mp_progress_widget) {

    mp_progress_widget->set_progress (progress);
    return true;

  } else {
    return false;
  }
}

bool
MainWindow::progress_wants_widget () const
{
  return true;
}

void
MainWindow::progress_add_widget (QWidget *widget)
{
  if (mp_progress_dialog) {
    mp_progress_dialog->add_widget (widget);
  } else if (mp_progress_widget) {
    mp_progress_widget->add_widget (widget);
  }
}

void
MainWindow::progress_remove_widget ()
{
  if (mp_progress_dialog) {
    mp_progress_dialog->remove_widget ();
  } else if (mp_progress_widget) {
    mp_progress_widget->remove_widget ();
  }
}

bool
MainWindow::show_progress_bar (bool show)
{
  if (!isVisible ()) {

    mp_progress_dialog.reset (0);

    if (show) {
      QWidget *tl = QApplication::activeWindow ();
      if (tl && tl->isVisible ()) {
        mp_progress_dialog.reset (new ProgressDialog (tl, mp_pr));
        mp_progress_dialog->show ();
      }
      return true;
    } else {
      return false;
    }

  } else {

    mp_main_stack_widget->setCurrentIndex (show ? 1 : 0);
    if (show) {
      clear_current_pos ();
    }
    return true;

  }
}

void
MainWindow::cm_packages ()
{
  lay::SaltController *sc = lay::SaltController::instance ();
  if (sc) {
    sc->show_editor ();
  }
}

void
MainWindow::cm_technologies ()
{
  lay::TechnologyController *tc = lay::TechnologyController::instance ();
  if (tc) {
    tc->show_editor ();
  }
}

void
MainWindow::show_macro_editor (const std::string &cat, bool add)
{
  lay::MacroController *mc = lay::MacroController::instance ();
  if (mc) {
    mc->show_editor (cat, add);
  }
}

void
MainWindow::cm_macro_editor ()
{
  //  TODO: implement this as generic menu provided by the plugin declaration
  show_macro_editor ();
}

void
MainWindow::cm_show_assistant ()
{
  if (! mp_assistant) {
    mp_assistant = new lay::HelpDialog (this);
  }

  if (mp_assistant->isMinimized ()) {
    mp_assistant->showNormal ();
  } else {
    mp_assistant->show ();
  }
  mp_assistant->activateWindow ();
  mp_assistant->raise ();
}

void
MainWindow::show_help (const QString &url)
{
  //  NOTE: from inside a modal widget we show the help dialog modal too
  //  (otherwise it's not usable)
  show_assistant_url (tl::to_string (url), QApplication::activeModalWidget () != 0);
}

void
MainWindow::show_modal_help (const QString &url)
{
  show_assistant_url (tl::to_string (url), true);
}

void
MainWindow::show_assistant_url (const std::string &url, bool modal)
{
  if (modal) {

    lay::HelpDialog dialog (QApplication::activeWindow () ? QApplication::activeWindow () : this, true);
    dialog.show ();   //  TODO: this is required to establish a proper geometry. Without this, the splitter is not set up correctly.
    dialog.load (url);
    dialog.exec ();

  } else {

    cm_show_assistant ();
    mp_assistant->load (url);

  }
}

void
MainWindow::show_assistant_topic (const std::string &s, bool modal)
{
  if (modal) {

    lay::HelpDialog dialog (this, true);
    dialog.search (s);
    dialog.exec ();

  } else {

    cm_show_assistant ();
    mp_assistant->search (s);

  }
}

void
MainWindow::cm_show_all_tips ()
{
  dispatcher ()->config_set (cfg_tip_window_hidden, "");
}

void
MainWindow::cm_help_about ()
{
  HelpAboutDialog help_about_dialog (this);
  help_about_dialog.exec ();
}

void
MainWindow::cm_help_about_qt ()
{
  QApplication::aboutQt ();
}

std::vector<std::string>
MainWindow::menu_symbols ()
{
  //  TODO: currently these are all symbols from all plugins
  return lay::PluginDeclaration::menu_symbols ();
}

void
MainWindow::menu_activated (const std::string &symbol)
{
  if (symbol == "cm_navigator_freeze") {
    cm_navigator_freeze ();
  } else if (symbol == "cm_navigator_close") {
    cm_navigator_close ();
  } else if (symbol == "cm_view_log") {
    cm_view_log ();
  } else if (symbol == "cm_print") {
    cm_print ();
  } else if (symbol == "cm_exit") {
    cm_exit ();
  } else if (symbol == "cm_reset_window_state") {
    cm_reset_window_state ();
  } else if (symbol == "cm_undo") {
    cm_undo ();
  } else if (symbol == "cm_redo") {
    cm_redo ();
  } else if (symbol == "cm_goto_position") {
    cm_goto_position ();
  } else if (symbol == "cm_manage_bookmarks") {
    cm_manage_bookmarks ();
  } else if (symbol == "cm_bookmark_view") {
    cm_bookmark_view ();
  } else if (symbol == "cm_cancel") {
    cm_cancel ();
  } else if (symbol == "cm_save_layer_props") {
    cm_save_layer_props ();
  } else if (symbol == "cm_load_layer_props") {
    cm_load_layer_props ();
  } else if (symbol == "cm_save_session") {
    cm_save_session ();
  } else if (symbol == "cm_restore_session") {
    cm_restore_session ();
  } else if (symbol == "cm_save_bookmarks") {
    cm_save_bookmarks ();
  } else if (symbol == "cm_load_bookmarks") {
    cm_load_bookmarks ();
  } else if (symbol == "cm_screenshot") {
    cm_screenshot ();
  } else if (symbol == "cm_screenshot_to_clipboard") {
      cm_screenshot_to_clipboard ();
  } else if (symbol == "cm_save_current_cell_as") {
    cm_save_current_cell_as ();
  } else if (symbol == "cm_save") {
    cm_save ();
  } else if (symbol == "cm_save_as") {
    cm_save_as ();
  } else if (symbol == "cm_save_all") {
    cm_save_all ();
  } else if (symbol == "cm_setup") {
    cm_setup ();
  } else if (symbol == "cm_open_too") {
    cm_open_too ();
  } else if (symbol == "cm_open_new_view") {
    cm_open_new_view ();
  } else if (symbol == "cm_open") {
    cm_open ();
  } else if (symbol == "cm_pull_in") {
    cm_pull_in ();
  } else if (symbol == "cm_reader_options") {
    cm_reader_options ();
  } else if (symbol == "cm_writer_options") {
    cm_writer_options ();
  } else if (symbol == "cm_new_panel") {
    cm_new_panel ();
  } else if (symbol == "cm_new_layout") {
    cm_new_layout ();
  } else if (symbol == "cm_clone") {
    cm_clone ();
  } else if (symbol == "cm_close_all") {
    cm_close_all ();
  } else if (symbol == "cm_close") {
    cm_close ();
  } else if (symbol == "cm_packages") {
    cm_packages ();
  } else if (symbol == "cm_technologies") {
    cm_technologies ();
  } else if (symbol == "cm_macro_editor") {
    cm_macro_editor ();
  } else if (symbol == "cm_show_assistant") {
    cm_show_assistant ();
  } else if (symbol == "cm_show_all_tips") {
    cm_show_all_tips ();
  } else if (symbol == "cm_help_about") {
    cm_help_about ();
  } else if (symbol == "cm_help_about_qt") {
    cm_help_about_qt ();
  } else if (symbol == "cm_edit_options") {

    m_eo_visible = true;
    show_dock_widget (mp_eo_dock_widget, m_eo_visible);

  } else {

    //  Try the plugin declarations
    for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
      if (cls->menu_activated (symbol)) {
        return;
      }
    }

    //  TODO: this can be part of the Plugin scheme, but the plugin root has no idea which is the active
    //  view.
    if (current_view ()) {
      current_view ()->menu_activated (symbol);
    } else {
      throw tl::Exception (tl::to_string (QObject::tr ("This function needs a layout but none was available")));
    }

  }
}

void
MainWindow::menu_changed ()
{
  //  delay actual rebuilding of the menu to collect multiple change events.
  dm_do_update_menu ();
}

void
MainWindow::do_update_menu ()
{
  menu ()->build (menuBar (), mp_tool_bar);
  lay::GuiApplication *app = dynamic_cast<lay::GuiApplication *> (qApp);
  if (app) {
    app->force_update_app_menu ();
  }
}

void
MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData () && event->mimeData ()->hasUrls () && event->mimeData ()->urls ().size () >= 1) {
    event->acceptProposedAction ();
  }
}

bool
MainWindow::eventFilter (QObject *watched, QEvent *event)
{
  //  spy on the mouse events of the tab bar so we can tell which tab the menu was issued on
  if (watched == mp_tab_bar && dynamic_cast<QMouseEvent *> (event) != 0) {
    m_mouse_pos = dynamic_cast<QMouseEvent *> (event)->pos ();
  }

  return QMainWindow::eventFilter (watched, event);
}

void
MainWindow::dropEvent(QDropEvent *event)
{
  BEGIN_PROTECTED

  if (event->mimeData () && event->mimeData ()->hasUrls ()) {

    QList<QUrl> urls = event->mimeData ()->urls ();
    for (QList<QUrl>::const_iterator url = urls.begin (); url != urls.end (); ++url) {

      QUrl eff_url (*url);

      QString path;
      if (eff_url.scheme () == QString::fromUtf8 ("file")) {
        path = url->toLocalFile ();

#if defined(__APPLE__) && (QT_VERSION < 0x050401)
        //----------------------------------------------------------------------------------------
        // By Kazunari Sekigawa (November 12, 2015)
        //
        // [Issue]
        //   When drag & dropping a GDS2/OASIS file from Finder, an error like below flags on:
        //     Unable to open file: /.file/id=6571367.1783076 (errno=20)
        //   http://klayout.de/forum/comments.php?DiscussionID=733&page=1#Item_0
        //
        //   Such a URL is called "File Reference URL" in OSX and iOS terminology.
        //   This has to be converted back to an ordinary full path.
        //   But due to a bug in Qt-4.8.x, this conversion fails.
        //
        // [Refs for workaround]
        //   https://bugreports.qt.io/browse/QTBUG-40449
        //   Sub: OS X Yosemite drag and drop file QUrl in this format: "file:///.file/id=......"
        //----------------------------------------------------------------------------------------
        // By Kazunari Sekigawa (December 12, 2017)
        //
        // This bug has been fixed in Qt 5.4.1.
        // When KLayout 0.25 is built with Qt 5.8.0 or later, this workaround is not required.
        //----------------------------------------------------------------------------------------
        QString keystring = QString::fromUtf8("/.file/id=");

        if ( path.startsWith(keystring) ) {
          CFStringRef relCFStringRef = CFStringCreateWithCString(
                                          kCFAllocatorDefault,
                                          path.toUtf8().constData(),
                                          kCFStringEncodingUTF8
                                        );
          CFURLRef relCFURL = CFURLCreateWithFileSystemPath(
                                          kCFAllocatorDefault,
                                          relCFStringRef,
                                          kCFURLPOSIXPathStyle,
                                          false // isDirectory
                                        );
          CFErrorRef error  = 0;
          CFURLRef absCFURL = CFURLCreateFilePathURL(
                                          kCFAllocatorDefault,
                                          relCFURL,
                                          &error
                                        );
          if ( !error ) {
            static const CFIndex maxAbsPathCStrBufLen = 4096;
            char absPathCStr[maxAbsPathCStrBufLen];
            if ( CFURLGetFileSystemRepresentation(
                    absCFURL,
                    true, // resolveAgainstBase
                    reinterpret_cast<UInt8 *>( &absPathCStr[0] ),
                    maxAbsPathCStrBufLen
                  )
                ) {
              path = QString::fromUtf8( absPathCStr );
            }
          }
          CFRelease( absCFURL );
          CFRelease( relCFURL );
          CFRelease( relCFStringRef );
        }

        eff_url = QUrl::fromLocalFile (path);
#endif

      } else if (eff_url.scheme () == QString::fromUtf8 ("http") || eff_url.scheme () == QString::fromUtf8 ("https")) {
        path = eff_url.toString ();
      } else {
        //  other schemes are not supported currently.
        continue;
      }

      //  Let the plugins decide if they accept the drop

      for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
        lay::PluginDeclaration *pd = const_cast<lay::PluginDeclaration *> (&*cls);
        if (pd->accepts_drop (tl::to_string (eff_url.toString ()))) {
          pd->drop_url (tl::to_string (eff_url.toString ()));
          return;
        }
      }

      if (current_view () && current_view ()->accepts_drop (tl::to_string (eff_url.toString ()))) {
        current_view ()->drop_url (tl::to_string (eff_url.toString ()));
        return;
      }

      //  Now try the built-in ones

      QFileInfo file_info (eff_url.path ());
      QString suffix = file_info.suffix ().toLower ();

      if (suffix == QString::fromUtf8 ("lyp")) {

        load_layer_properties (tl::to_string (path), false /*current view only*/, false /*don't add a default*/);
        add_to_other_mru (tl::to_string (path), cfg_mru_layer_properties);

      } else if (suffix == QString::fromUtf8 ("lys")) {

        restore_session (tl::to_string (path));
        add_to_other_mru (tl::to_string (path), cfg_mru_sessions);

      } else if (suffix == QString::fromUtf8 ("lyb")) {

        if (current_view ()) {
          BookmarkList bookmarks;
          bookmarks.load (tl::to_string (path));
          current_view ()->bookmarks (bookmarks);
          add_to_other_mru (tl::to_string (path), cfg_mru_bookmarks);
        }

      } else {

        OpenLayoutModeDialog open_mode_dialog (this);
        if (views () == 0 || open_mode_dialog.exec_dialog (m_open_mode)) {

          if (m_open_mode == 0) {

            std::string df_list;
            int dirty_layouts = dirty_files (df_list);

            bool can_open = true;
            if (dirty_layouts != 0) {

              QMessageBox mbox (this);
              mbox.setText (tl::to_qstring (tl::to_string (QObject::tr ("The following layouts need saving:\n\n")) + df_list + "\n\nPress 'Close Without Saving' to open the layout and discard changes."));
              mbox.setWindowTitle (QObject::tr ("Save Needed"));
              mbox.setIcon (QMessageBox::Warning);
              QAbstractButton *yes_button = mbox.addButton (QObject::tr ("Close Without Saving"), QMessageBox::YesRole);
              mbox.addButton (QMessageBox::Cancel);

              mbox.exec ();

              can_open = (mbox.clickedButton() == yes_button);

            }

            if (! can_open) {
              return;
            }

          }

          load_layout (tl::to_string (path), m_initial_technology, m_open_mode);

          add_mru (tl::to_string (path), m_initial_technology);

        }

      }

    }

  }

  END_PROTECTED
}

void
MainWindow::plugin_registered (lay::PluginDeclaration *cls)
{
  //  store current state in configuration
  save_state_to_config ();

  cls->init_menu (dispatcher ());

  //  recreate all plugins
  for (std::vector <lay::LayoutViewWidget *>::iterator vp = mp_views.begin (); vp != mp_views.end (); ++vp) {
    (*vp)->view ()->create_plugins ();
  }
}

void
MainWindow::plugin_removed (lay::PluginDeclaration *cls)
{
  cls->remove_menu_items (dispatcher ());

  //  recreate all plugins except the one that got removed
  for (std::vector <lay::LayoutViewWidget *>::iterator vp = mp_views.begin (); vp != mp_views.end (); ++vp) {
    (*vp)->view ()->create_plugins (cls);
  }
}

// ------------------------------------------------------------
//  Declaration of the "plugin" for the menu entries

class MainWindowPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    std::string at;

    at = ".end";
    menu_entries.push_back (lay::submenu ("file_menu", at, tl::to_string (QObject::tr ("&File"))));
    menu_entries.push_back (lay::submenu ("edit_menu", at, tl::to_string (QObject::tr ("&Edit"))));
    menu_entries.push_back (lay::submenu ("view_menu", at, tl::to_string (QObject::tr ("&View"))));
    menu_entries.push_back (lay::submenu ("bookmark_menu", at, tl::to_string (QObject::tr ("&Bookmarks"))));
    menu_entries.push_back (lay::submenu ("zoom_menu", at, tl::to_string (QObject::tr ("&Display"))));
    menu_entries.push_back (lay::submenu ("tools_menu", at, tl::to_string (QObject::tr ("&Tools"))));
    menu_entries.push_back (lay::submenu ("macros_menu", at, tl::to_string (QObject::tr ("&Macros"))));
    menu_entries.push_back (lay::separator ("help_group", at));
    menu_entries.push_back (lay::submenu ("help_menu", at, tl::to_string (QObject::tr ("&Help"))));
    menu_entries.push_back (lay::submenu ("@secrets", at, tl::to_string (QObject::tr ("Secret Features"))));
    menu_entries.push_back (lay::submenu ("@toolbar", at, std::string ()));

    at = "edit_menu.end";
    menu_entries.push_back (lay::separator ("edit_options_group:edit_mode", "edit_menu.end"));
    menu_entries.push_back (lay::menu_item ("cm_edit_options", "edit_options:edit_mode", "edit_menu.end", tl::to_string (QObject::tr ("Editor Options")) + "(F3)"));

    at = "file_menu.end";
    menu_entries.push_back (lay::menu_item ("cm_new_layout", "new_layout:edit:edit_mode", at, tl::to_string (QObject::tr ("New Layout"))));
    menu_entries.push_back (lay::menu_item ("cm_new_panel", "new_panel:edit:edit_mode", at, tl::to_string (QObject::tr ("New Panel"))));
    menu_entries.push_back (lay::separator ("post_new_group:edit_mode", at));
    menu_entries.push_back (lay::menu_item ("cm_open", "open:edit", at, tl::to_string (QObject::tr ("Open"))));
    menu_entries.push_back (lay::menu_item ("cm_open_too", "open_same_panel:edit", at, tl::to_string (QObject::tr ("Open In Same Panel(Shift+Ctrl+O)"))));
    menu_entries.push_back (lay::menu_item ("cm_open_new_view", "open_new_panel", at, tl::to_string (QObject::tr ("Open In New Panel(Ctrl+O)"))));
    menu_entries.push_back (lay::menu_item ("cm_close", "close:edit", at, tl::to_string (QObject::tr ("Close(Ctrl+W)"))));
    menu_entries.push_back (lay::menu_item ("cm_close_all", "close_all:edit", at, tl::to_string (QObject::tr ("Close All(Shift+Ctrl+W)"))));
    menu_entries.push_back (lay::menu_item ("cm_clone", "clone", at, tl::to_string (QObject::tr ("Clone Panel"))));
    menu_entries.push_back (lay::menu_item ("cm_reload", "reload:edit", at, tl::to_string (QObject::tr ("Reload(Ctrl+R)"))));
    menu_entries.push_back (lay::menu_item ("cm_pull_in", "pull_in:edit", at, tl::to_string (QObject::tr ("Pull In Other Layout"))));
    menu_entries.push_back (lay::menu_item ("cm_reader_options", "reader_options", at, tl::to_string (QObject::tr ("Reader Options"))));
    menu_entries.push_back (lay::separator ("open_recent_group", at));
    menu_entries.push_back (lay::submenu ("open_recent_menu:edit", at, tl::to_string (QObject::tr ("Open Recent"))));
    menu_entries.push_back (lay::separator ("import_group", at));
    menu_entries.push_back (lay::submenu ("import_menu:edit", at, tl::to_string (QObject::tr ("Import"))));
    menu_entries.push_back (lay::separator ("save_group", at));
    menu_entries.push_back (lay::menu_item ("cm_save", "save:hide_vo", at, tl::to_string (QObject::tr ("Save"))));
    menu_entries.push_back (lay::menu_item ("cm_save_as", "save_as:hide_vo", at, tl::to_string (QObject::tr ("Save As"))));
    menu_entries.push_back (lay::menu_item ("cm_save_all", "save_all:hide_vo", at, tl::to_string (QObject::tr ("Save All"))));
    menu_entries.push_back (lay::menu_item ("cm_writer_options", "writer_options:hide_vo", at, tl::to_string (QObject::tr ("Writer Options"))));
    menu_entries.push_back (lay::separator ("setup_group", at));
    menu_entries.push_back (lay::menu_item ("cm_setup", "setup:edit", at, tl::to_string (QObject::tr ("Setup"))));
    menu_entries.push_back (lay::separator ("misc_group", at));
    menu_entries.push_back (lay::menu_item ("cm_screenshot", "screenshot:edit", at, tl::to_string (QObject::tr ("Screenshot(Print)"))));
    menu_entries.push_back (lay::menu_item ("cm_screenshot_to_clipboard", "screenshot_to_clipboard:edit", at, tl::to_string (QObject::tr ("Screenshot to clipboard"))));
    menu_entries.push_back (lay::menu_item ("cm_layout_props", "layout_props:edit", at, tl::to_string (QObject::tr ("Layout Properties"))));
    menu_entries.push_back (lay::menu_item ("cm_layout_stats", "layout_stats:edit", at, tl::to_string (QObject::tr ("Layout Statistics"))));
    menu_entries.push_back (lay::separator ("layer_group", at));
    menu_entries.push_back (lay::menu_item ("cm_load_layer_props", "load_layer_props:edit", at, tl::to_string (QObject::tr ("Load Layer Properties"))));
    menu_entries.push_back (lay::menu_item ("cm_save_layer_props", "save_layer_props:edit", at, tl::to_string (QObject::tr ("Save Layer Properties"))));
    menu_entries.push_back (lay::submenu ("open_recent_menu_layer_props:edit", at, tl::to_string (QObject::tr ("Recent Layer Properties"))));
    menu_entries.push_back (lay::separator ("session_group", at));
    menu_entries.push_back (lay::menu_item ("cm_restore_session", "restore_session:edit", at, tl::to_string (QObject::tr ("Restore Session"))));
    menu_entries.push_back (lay::menu_item ("cm_save_session", "save_session", at, tl::to_string (QObject::tr ("Save Session"))));
    menu_entries.push_back (lay::submenu ("open_recent_menu_sessions:edit", at, tl::to_string (QObject::tr ("Recent Sessions"))));
    menu_entries.push_back (lay::separator ("log_group", at));
    menu_entries.push_back (lay::menu_item ("cm_view_log", "view_log", at, tl::to_string (QObject::tr ("Log Viewer"))));
    menu_entries.push_back (lay::separator ("print_group", at));
    menu_entries.push_back (lay::menu_item ("cm_print", "print", at, tl::to_string (QObject::tr ("Print(Ctrl+P)"))));
    menu_entries.push_back (lay::separator ("exit_group", at));
    menu_entries.push_back (lay::menu_item ("cm_exit", "exit", at, tl::to_string (QObject::tr ("Exit(Ctrl+Q)"))));

    at = "view_menu.end";
    menu_entries.push_back (lay::config_menu_item ("show_grid", at, tl::to_string (QObject::tr ("Show Grid")), cfg_grid_visible, "?")),
    menu_entries.push_back (lay::submenu ("default_grid:default_grids_group", at, tl::to_string (QObject::tr ("Grid"))));
    menu_entries.push_back (lay::separator ("layout_group", at));
    menu_entries.push_back (lay::config_menu_item ("show_markers", at, tl::to_string (QObject::tr ("Show Markers")), cfg_markers_visible, "?"));
    menu_entries.push_back (lay::config_menu_item ("show_texts", at, tl::to_string (QObject::tr ("Show Texts")), cfg_text_visible, "?"));
    menu_entries.push_back (lay::config_menu_item ("show_cell_boxes", at, tl::to_string (QObject::tr ("Show Cell Frames")), cfg_cell_box_visible, "?"));
    menu_entries.push_back (lay::config_menu_item ("no_stipples", at, tl::to_string (QObject::tr ("Show Layers Without Fill")), cfg_no_stipple, "?"));
    menu_entries.push_back (lay::config_menu_item ("synchronized_views", at, tl::to_string (QObject::tr ("Synchronized Views")), cfg_synchronized_views, "?"));
    menu_entries.push_back (lay::config_menu_item ("edit_top_level_selection:edit_mode", at, tl::to_string (QObject::tr ("Select Top Level Objects")), edt::cfg_edit_top_level_selection, "?"));
    menu_entries.push_back (lay::separator ("panels_group", at));
    menu_entries.push_back (lay::config_menu_item ("show_toolbar", at, tl::to_string (QObject::tr ("Toolbar")), cfg_show_toolbar, "?"));
    menu_entries.push_back (lay::config_menu_item ("show_navigator", at, tl::to_string (QObject::tr ("Navigator")), cfg_show_navigator, "?"));
    menu_entries.push_back (lay::config_menu_item ("show_layer_panel", at, tl::to_string (QObject::tr ("Layers")), cfg_show_layer_panel, "?"));
    menu_entries.push_back (lay::config_menu_item ("show_layer_toolbox", at, tl::to_string (QObject::tr ("Layer Toolbox")), cfg_show_layer_toolbox, "?"));
    menu_entries.push_back (lay::config_menu_item ("show_hierarchy_panel", at, tl::to_string (QObject::tr ("Cells")), cfg_show_hierarchy_panel, "?"));
    menu_entries.push_back (lay::config_menu_item ("show_libraries_view", at, tl::to_string (QObject::tr ("Libraries")), cfg_show_libraries_view, "?"));
    menu_entries.push_back (lay::config_menu_item ("show_bookmarks_view", at, tl::to_string (QObject::tr ("Bookmarks")), cfg_show_bookmarks_view, "?"));
    menu_entries.push_back (lay::menu_item ("cm_reset_window_state", "reset_window_state", at, tl::to_string (QObject::tr ("Restore Window")))),
    menu_entries.push_back (lay::separator ("selection_group", at));
    menu_entries.push_back (lay::config_menu_item ("transient_selection", at, tl::to_string (QObject::tr ("Highlight Object Under Mouse")), cfg_sel_transient_mode, "?"));
    menu_entries.push_back (lay::config_menu_item ("mouse_tracking", at, tl::to_string (QObject::tr ("Mouse Tracking")), cfg_tracking_cursor_enabled, "?"));
    menu_entries.push_back (lay::config_menu_item ("crosshair_cursor", at, tl::to_string (QObject::tr ("Crosshair Cursor")), cfg_crosshair_cursor_enabled, "?"));

    at = "help_menu.end";
    menu_entries.push_back (lay::menu_item ("cm_show_all_tips", "show_all_tips", at, tl::to_string (QObject::tr ("Show All Tips"))));
    menu_entries.push_back (lay::separator ("help_topics_group", at));
    menu_entries.push_back (lay::menu_item ("cm_show_assistant", "assistant", at, tl::to_string (QObject::tr ("Assistant"))));
    menu_entries.push_back (lay::menu_item ("cm_help_about", "about", at, tl::to_string (QObject::tr ("About"))));
    menu_entries.push_back (lay::menu_item ("cm_help_about_qt", "about_qt", at, tl::to_string (QObject::tr ("About Qt"))));

    at = "tools_menu.end";
    menu_entries.push_back (lay::menu_item ("cm_packages", "packages", at, tl::to_string (QObject::tr ("Manage Packages"))));
    menu_entries.push_back (lay::menu_item ("cm_technologies", "technologies", at, tl::to_string (QObject::tr ("Manage Technologies"))));
    menu_entries.push_back (lay::separator ("verification_group", at));
    menu_entries.push_back (lay::separator ("post_verification_group", at));

    at = "macros_menu.end";
    menu_entries.push_back (lay::menu_item ("cm_macro_editor", "macro_development", at, tl::to_string (QObject::tr ("Macro Development(F5)"))));
    menu_entries.push_back (lay::separator ("macros_group", at));

    at = "@toolbar.end";
    menu_entries.push_back (lay::menu_item ("cm_prev_display_state", "prev_display_state", at, tl::to_string (QObject::tr ("Back<:/back_24px.png>"))));
    menu_entries.push_back (lay::menu_item ("cm_next_display_state", "next_display_state", at, tl::to_string (QObject::tr ("Forward<:/forward_24px.png>"))));
    menu_entries.push_back (lay::separator ("toolbar_post_navigation_group", at));
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new MainWindowPluginDeclaration (), -100, "MainWindowPlugin");

} // namespace lay

