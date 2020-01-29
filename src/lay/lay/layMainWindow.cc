
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
#include "layCellSelectionForm.h"
#include "layDialogs.h"
#include "laybasicConfig.h"
#include "layConfig.h"
#include "layMainWindow.h"
#include "layHelpDialog.h"
#include "layHelpSource.h" // because of escape_xml
#include "layNavigator.h"
#include "layProgress.h"
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
#include "layTechnologyController.h"
#include "laySaltController.h"
#include "layTipDialog.h"
#include "laySelectCellViewForm.h"
#include "layLayoutPropertiesForm.h"
#include "layLayoutStatisticsForm.h"
#include "layMacroController.h"
#include "layInit.h"
#include "antObject.h"
#include "antService.h"
#include "ui_HelpAboutDialog.h"
#include "gsi.h"
#include "gsiInterpreter.h"
#include "gtf.h"

namespace lay
{

const int max_dirty_files = 15;

// -------------------------------------------------------------

class ProgressDialog
  : public QDialog,
    public tl::Object
{
public:
  ProgressDialog (QWidget *parent, lay::ProgressReporter *pr)
    : QDialog (parent), mp_pr (pr)
  {
    QVBoxLayout *vbl = new QVBoxLayout (this);
    vbl->setMargin (0);
    vbl->setSpacing (0);

    mp_progress_widget = new ProgressWidget (pr, this, true);
    mp_progress_widget->setObjectName (QString::fromUtf8 ("progress"));
    vbl->addWidget (mp_progress_widget);

    setWindowTitle (QObject::tr ("Progress"));
    setWindowModality (Qt::WindowModal);
  }

  void closeEvent (QCloseEvent * /*event*/)
  {
    if (mp_pr) {
      //  NOTE: We don't kill on close for now. This creates a too easy way to scrap results.
      //    mp_pr->signal_break ();
      //  TODO: there should be a warning saying some jobs are pending.
    }
  }

  void set_progress (tl::Progress *progress)
  {
    mp_progress_widget->set_progress (progress);
  }

  void add_widget (QWidget *widget)
  {
    mp_progress_widget->add_widget (widget);
  }

  void remove_widget ()
  {
    mp_progress_widget->remove_widget ();
  }

  QWidget *get_widget () const
  {
    return mp_progress_widget->get_widget ();
  }

private:
  lay::ProgressWidget *mp_progress_widget;
  lay::ProgressReporter *mp_pr;
};

// -------------------------------------------------------------

class ControlWidgetStack
  : public QFrame
{
public:
  ControlWidgetStack (QWidget *parent = 0, const char *name = 0)
    : QFrame (parent)
  {
    setObjectName (QString::fromUtf8 (name));

    //  Background ist a simple label without a text currently
    mp_bglabel = new QLabel (this);
    mp_bglabel->setAutoFillBackground (true);
    mp_bglabel->setAlignment (Qt::AlignVCenter | Qt::AlignHCenter);
    mp_bglabel->show ();
  }

  void addWidget (QWidget *w)
  {
    m_widgets.push_back (w);
    w->setParent (this);
    resize_children ();
    raiseWidget (m_widgets.size () - 1);

    int mw = 0;
    for (size_t i = 0; i < m_widgets.size (); ++i) {
      mw = std::max (m_widgets [i]->sizeHint ().width (), mw);
      mw = std::max (m_widgets [i]->minimumWidth (), mw);
    }

    if (mw > minimumWidth ()) {
      setMinimumWidth (mw);
      resize (minimumWidth (), height ());
    }
  }

  QSize sizeHint () const
  {
    int w = 0;
    for (size_t i = 0; i < m_widgets.size (); ++i) {
      w = std::max (m_widgets [i]->sizeHint ().width (), w);
    }
    return QSize (w, 0);
  }

  void removeWidget (size_t index)
  {
    if (index < m_widgets.size ()) {
      m_widgets.erase (m_widgets.begin () + index);
    }
    if (m_widgets.size () == 0) {
      mp_bglabel->show ();
    }
  }

  void raiseWidget (size_t index)
  {
    bool any_visible = false;
    for (size_t i = 0; i < m_widgets.size (); ++i) {
      if (m_widgets [i]) {
        if (i == index) {
          m_widgets [i]->show ();
          any_visible = true;
        } else {
          m_widgets [i]->hide ();
        }
      }
    }

    if (! any_visible) {
      mp_bglabel->show ();
    } else {
      mp_bglabel->hide ();
    }
  }

  QWidget *widget (size_t index)
  {
    if (index < m_widgets.size ()) {
      return m_widgets [index];
    } else {
      return 0;
    }
  }

  QWidget *background_widget ()
  {
    return mp_bglabel;
  }

protected:
  virtual void resizeEvent (QResizeEvent *)
  {
    resize_children ();
  }

  void resize_children ()
  {
    //  set the geometry of all children
    for (std::vector <QWidget *>::iterator child = m_widgets.begin (); child != m_widgets.end (); ++child) {
      if (*child) {
        (*child)->setGeometry (0, 0, width (), height ());
      }
    }
    mp_bglabel->setGeometry (0, 0, width (), height ());
  }

  std::vector <QWidget *> m_widgets;
  QLabel *mp_bglabel;
};

// -------------------------------------------------------------

class ViewWidgetStack
  : public QWidget
{
public:
  ViewWidgetStack (QWidget *parent = 0, const char *name = 0)
    : QWidget (parent)
  {
    setObjectName (QString::fromUtf8 (name));

    mp_bglabel = new QLabel (this);
    mp_bglabel->setAutoFillBackground (true);
    mp_bglabel->setText (QObject::tr ("<html><body><p><img src=\":/logo.png\"/></p><p>Use File/Open to open a layout</p></body></html>"));
    mp_bglabel->setAlignment (Qt::AlignVCenter | Qt::AlignHCenter);
    mp_bglabel->show ();
  }

  void addWidget (LayoutView *w)
  {
    m_widgets.push_back (w);
    w->setParent (this);
    resize_children ();
    raiseWidget (m_widgets.size () - 1);

    updateGeometry ();
  }

  void removeWidget (size_t index)
  {
    if (index < m_widgets.size ()) {
      m_widgets.erase (m_widgets.begin () + index);
    }
    if (m_widgets.size () == 0) {
      mp_bglabel->show ();
    }
  }

  void raiseWidget (size_t index)
  {
    if (index < m_widgets.size ()) {
      mp_bglabel->hide ();
      m_widgets [index]->show ();
    } else {
      mp_bglabel->show ();
    }

    size_t i = 0;
    for (std::vector <LayoutView *>::iterator child = m_widgets.begin (); child != m_widgets.end (); ++child, ++i) {
      if (i != index) {
        (*child)->hide ();
      }
    }
  }

  LayoutView *widget (size_t index)
  {
    if (index < m_widgets.size ()) {
      return m_widgets [index];
    } else {
      return 0;
    }
  }

  QWidget *background_widget ()
  {
    return mp_bglabel;
  }

protected:
  virtual void resizeEvent (QResizeEvent *)
  {
    resize_children ();
  }

  void resize_children ()
  {
    //  set the geometry of all children
    for (std::vector <LayoutView *>::iterator child = m_widgets.begin (); child != m_widgets.end (); ++child) {
      (*child)->setGeometry (0, 0, width (), height ());
    }
    mp_bglabel->setGeometry (0, 0, width (), height ());
  }

  std::vector <LayoutView *> m_widgets;
  QLabel *mp_bglabel;
};

// -------------------------------------------------------------

TextProgressDelegate::TextProgressDelegate (MainWindow *mw, int verbosity)
  : lay::TextProgress (verbosity), mp_mw (mw)
{
  //  .. nothing yet ..
}

void TextProgressDelegate::update_progress (tl::Progress *progress)
{
  if (!mp_mw->update_progress (progress)) {
    lay::TextProgress::update_progress (progress);
  }
}

void TextProgressDelegate::show_progress_bar (bool show)
{
  if (!mp_mw->show_progress_bar (show)) {
    lay::TextProgress::show_progress_bar (show);
  }
}

bool TextProgressDelegate::progress_wants_widget () const
{
  return mp_mw != 0 && mp_mw->progress_wants_widget ();
}

void TextProgressDelegate::progress_add_widget (QWidget *widget)
{
  if (mp_mw) {
    mp_mw->progress_add_widget (widget);
  }
}

QWidget *TextProgressDelegate::progress_get_widget () const
{
  return mp_mw ? mp_mw->progress_get_widget () : 0;
}

void TextProgressDelegate::progress_remove_widget ()
{
  if (mp_mw) {
    mp_mw->progress_remove_widget ();
  }
}

// -------------------------------------------------------------

static MainWindow *mw_instance = 0;

MainWindow *
MainWindow::instance ()
{
  return mw_instance;
}

// -----------------------------------

MainWindow::MainWindow (QApplication *app, lay::Plugin *plugin_parent, const char *name)
    : QMainWindow (0),
      lay::Plugin (plugin_parent),
      m_text_progress (this, 10 /*verbosity threshold*/),
      m_mode (std::numeric_limits<unsigned int>::max ()),
      mp_setup_form (0),
      m_open_mode (0),
      m_disable_tab_selected (false),
      m_exited (false),
      dm_do_update_menu (this, &MainWindow::do_update_menu),
      dm_do_update_file_menu (this, &MainWindow::do_update_file_menu),
      dm_exit (this, &MainWindow::exit),
      m_grid_micron (0.001),
      m_default_grids_updated (true),
      m_new_cell_window_size (2.0),
      m_new_layout_current_panel (false),
      m_synchronized_views (false),
      m_synchronous (false),
      m_busy (false),
      mp_app (app)
{
  //  ensures the deferred method scheduler is present
  tl::DeferredMethodScheduler::instance ();

  setObjectName (QString::fromUtf8 (name));

  if (mw_instance != 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("Only one instance of MainWindow may be created")));
  }
  mw_instance = this;

  lay::register_help_handler (this, SLOT (show_help (const QString &)), SLOT (show_modal_help (const QString &)));

  mp_setup_form = new SettingsForm (0, plugin_root (), "setup_form"),

  db::LibraryManager::instance ().changed_event.add (this, &MainWindow::libraries_changed);

  init_menu ();

  mp_assistant = new lay::HelpDialog (this);

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
  vbl->setMargin (0);
  vbl->setSpacing (0);

  mp_tab_bar = new QTabBar (mp_main_frame);
  vbl->addWidget (mp_tab_bar);
  connect (mp_tab_bar, SIGNAL (currentChanged (int)), this, SLOT (view_selected (int)));
#if QT_VERSION >= 0x040500
  mp_tab_bar->setTabsClosable(true);
  connect (mp_tab_bar, SIGNAL (tabCloseRequested (int)), this, SLOT (tab_close_requested (int)));
#endif

  mp_hp_dock_widget = new QDockWidget (QObject::tr ("Cells"), this);
  mp_hp_dock_widget->setObjectName (QString::fromUtf8 ("hp_dock_widget"));
  mp_hp_stack = new ControlWidgetStack (mp_hp_dock_widget, "hp_stack");
  mp_hp_dock_widget->setWidget (mp_hp_stack);
  connect (mp_hp_dock_widget, SIGNAL (visibilityChanged (bool)), this, SLOT (dock_widget_visibility_changed (bool)));
  m_hp_visible = true;

  mp_libs_dock_widget = new QDockWidget (QObject::tr ("Libraries"), this);
  mp_libs_dock_widget->setObjectName (QString::fromUtf8 ("libs_dock_widget"));
  mp_libs_stack = new ControlWidgetStack (mp_libs_dock_widget, "libs_stack");
  mp_libs_dock_widget->setWidget (mp_libs_stack);
  connect (mp_libs_dock_widget, SIGNAL (visibilityChanged (bool)), this, SLOT (dock_widget_visibility_changed (bool)));
  m_libs_visible = true;

  mp_bm_dock_widget = new QDockWidget (QObject::tr ("Bookmarks"), this);
  mp_bm_dock_widget->setObjectName (QString::fromUtf8 ("bookmarks_dock_widget"));
  mp_bm_stack = new ControlWidgetStack (mp_bm_dock_widget, "bookmarks_stack");
  mp_bm_dock_widget->setWidget (mp_bm_stack);
  connect (mp_bm_dock_widget, SIGNAL (visibilityChanged (bool)), this, SLOT (dock_widget_visibility_changed (bool)));
  m_bm_visible = true;

  mp_view_stack = new ViewWidgetStack (mp_main_frame);
  mp_view_stack->setObjectName (QString::fromUtf8 ("view_stack"));
  vbl->addWidget (mp_view_stack);

  mp_layer_toolbox_dock_widget = new QDockWidget (QObject::tr ("Layer Toolbox"), this);
  mp_layer_toolbox_dock_widget->setObjectName (QString::fromUtf8 ("lt_dock_widget"));
  mp_layer_toolbox = new LayerToolbox (mp_layer_toolbox_dock_widget, "layer_toolbox");
  mp_layer_toolbox_dock_widget->setWidget (mp_layer_toolbox);
  connect (mp_layer_toolbox_dock_widget, SIGNAL (visibilityChanged (bool)), this, SLOT (dock_widget_visibility_changed (bool)));
  m_layer_toolbox_visible = true;

  mp_lp_dock_widget = new QDockWidget (QObject::tr ("Layers"), this);
  mp_lp_dock_widget->setObjectName (QString::fromUtf8 ("lp_dock_widget"));
  mp_lp_stack = new ControlWidgetStack (mp_lp_dock_widget, "lp_stack");
  mp_lp_dock_widget->setWidget (mp_lp_stack);
  connect (mp_lp_dock_widget, SIGNAL (visibilityChanged (bool)), this, SLOT (dock_widget_visibility_changed (bool)));
  m_lp_visible = true;

  mp_navigator_dock_widget = new QDockWidget (QObject::tr ("Navigator"), this);
  mp_navigator_dock_widget->setObjectName (QString::fromUtf8 ("navigator_dock_widget"));
  mp_navigator = new Navigator (this);
  mp_navigator_dock_widget->setWidget (mp_navigator);
  connect (mp_navigator_dock_widget, SIGNAL (visibilityChanged (bool)), this, SLOT (dock_widget_visibility_changed (bool)));
  m_navigator_visible = true;

  //  Add dock widgets
#if QT_VERSION >= 0x040500
  setTabPosition (Qt::AllDockWidgetAreas, QTabWidget::North);
#endif
  addDockWidget(Qt::LeftDockWidgetArea, mp_navigator_dock_widget);
  addDockWidget(Qt::LeftDockWidgetArea, mp_hp_dock_widget);
  addDockWidget(Qt::LeftDockWidgetArea, mp_libs_dock_widget);
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

  mp_menu->build (mbar, mp_tool_bar);

  connect (mp_menu, SIGNAL (changed ()), this, SLOT (menu_changed ()));

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
  cp_frame_ly->setMargin (0);
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
  mp_layout_fdia = new lay::FileDialog (this, tl::to_string (QObject::tr ("Layout File")), fmts);

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
  //  avoid deferred execution later on where there isn't a valid main window anymore
  //  (problem case: showing a dialog inside main windows's destroyed signal - this will
  //  process events and trigger execution if not disabled)
  if (! tl::DeferredMethodScheduler::instance ()->is_disabled ()) {
    tl::DeferredMethodScheduler::instance ()->execute ();
  }
  tl::DeferredMethodScheduler::instance ()->enable (false);

  lay::register_help_handler (0, 0, 0);

  //  since the configuration actions unregister themselves, we need to do this before the main
  //  window is gone:
  m_ca_collection.clear ();

  mw_instance = 0;

  //  explicitly delete the views here. Otherwise they
  //  are deleted by ~QWidget, which is too late since then
  //  the manager does not exist any longer.
  view_closed_event.clear (); // don't send events
  close_all ();

  //  delete the Menu after the views because they may want to access them in the destructor

  delete mp_menu;
  mp_menu = 0;

  delete mp_pr;
  mp_pr = 0;

  delete mp_setup_form;
  mp_setup_form = 0;

  delete mp_log_viewer_dialog;
  mp_log_viewer_dialog = 0;

  delete mp_assistant;
  mp_assistant = 0;
}

void
MainWindow::init_menu ()
{
  //  default menu layout

  MenuLayoutEntry secret_menu [] = {
    MenuLayoutEntry ("paste_interactive:edit",          tl::to_string (QObject::tr ("Paste Interactive")),                SLOT (cm_paste_interactive ())),
    MenuLayoutEntry ("duplicate_interactive:edit",      tl::to_string (QObject::tr ("Duplicate Interactive")),            SLOT (cm_duplicate_interactive ())),
    MenuLayoutEntry ("sel_move_interactive",            tl::to_string (QObject::tr ("Move Interactive")),                 SLOT (cm_sel_move_interactive ())),
    MenuLayoutEntry::last ()
  };

  MenuLayoutEntry empty_menu [] = {
    MenuLayoutEntry::last ()
  };

  MenuLayoutEntry file_menu [] = {
    MenuLayoutEntry ("new_layout:edit:edit_mode",       tl::to_string (QObject::tr ("New Layout")),                       SLOT (cm_new_layout ())),
    MenuLayoutEntry ("new_panel:edit:edit_mode",        tl::to_string (QObject::tr ("New Panel")),                        SLOT (cm_new_panel ())),
    MenuLayoutEntry::separator ("post_new_group:edit_mode"),
    MenuLayoutEntry ("open:edit",                       tl::to_string (QObject::tr ("Open")),                             SLOT (cm_open ())),
    MenuLayoutEntry ("open_same_panel:edit",            tl::to_string (QObject::tr ("Open In Same Panel(Shift+Ctrl+O)")), SLOT (cm_open_too ())),
    MenuLayoutEntry ("open_new_panel",                  tl::to_string (QObject::tr ("Open In New Panel(Ctrl+O)")),        SLOT (cm_open_new_view ())),
    MenuLayoutEntry ("close:edit",                      tl::to_string (QObject::tr ("Close(Ctrl+W)")),                    SLOT (cm_close ())),
    MenuLayoutEntry ("close_all:edit",                  tl::to_string (QObject::tr ("Close All(Shift+Ctrl+W)")),          SLOT (cm_close_all ())),
    MenuLayoutEntry ("clone",                           tl::to_string (QObject::tr ("Clone Panel")),                      SLOT (cm_clone ())),
    MenuLayoutEntry ("reload:edit",                     tl::to_string (QObject::tr ("Reload(Ctrl+R)")),                   SLOT (cm_reload ())),
    MenuLayoutEntry ("pull_in:edit",                    tl::to_string (QObject::tr ("Pull In Other Layout")),             SLOT (cm_pull_in ())),
    MenuLayoutEntry ("reader_options",                  tl::to_string (QObject::tr ("Reader Options")),                   SLOT (cm_reader_options ())),
    MenuLayoutEntry::separator ("open_recent_group"),
    MenuLayoutEntry ("open_recent_menu:edit",           tl::to_string (QObject::tr ("Open Recent")),                      empty_menu),
    MenuLayoutEntry::separator ("import_group"),
    MenuLayoutEntry ("import_menu:edit",                tl::to_string (QObject::tr ("Import")),                           empty_menu),
    MenuLayoutEntry::separator ("save_group"),
    MenuLayoutEntry ("save:hide_vo",                    tl::to_string (QObject::tr ("Save")),                             SLOT (cm_save ())),
    MenuLayoutEntry ("save_as:hide_vo",                 tl::to_string (QObject::tr ("Save As")),                          SLOT (cm_save_as ())),
    MenuLayoutEntry ("save_all:hide_vo",                tl::to_string (QObject::tr ("Save All")),                         SLOT (cm_save_all ())),
    MenuLayoutEntry ("writer_options:hide_vo",          tl::to_string (QObject::tr ("Writer Options")),                   SLOT (cm_writer_options ())),
    MenuLayoutEntry::separator ("setup_group"),
    MenuLayoutEntry ("setup:edit",                      tl::to_string (QObject::tr ("Setup")),                            SLOT (cm_setup ())),
    MenuLayoutEntry::separator ("misc_group"),
    MenuLayoutEntry ("screenshot:edit",                 tl::to_string (QObject::tr ("Screenshot(Print)")),                SLOT (cm_screenshot ())),
    MenuLayoutEntry ("layout_props:edit",               tl::to_string (QObject::tr ("Layout Properties")),                SLOT (cm_layout_props ())),
    MenuLayoutEntry ("layout_stats:edit",               tl::to_string (QObject::tr ("Layout Statistics")),                SLOT (cm_layout_stats ())),
    MenuLayoutEntry::separator ("layer_group"),
    MenuLayoutEntry ("load_layer_props:edit",           tl::to_string (QObject::tr ("Load Layer Properties")),            SLOT (cm_load_layer_props ())),
    MenuLayoutEntry ("save_layer_props:edit",           tl::to_string (QObject::tr ("Save Layer Properties")),            SLOT (cm_save_layer_props ())),
    MenuLayoutEntry::separator ("session_group"),
    MenuLayoutEntry ("restore_session:edit",            tl::to_string (QObject::tr ("Restore Session")),                  SLOT (cm_restore_session ())),
    MenuLayoutEntry ("save_session",                    tl::to_string (QObject::tr ("Save Session")),                     SLOT (cm_save_session ())),
    MenuLayoutEntry::separator ("log_group"),
    MenuLayoutEntry ("view_log",                        tl::to_string (QObject::tr ("Log Viewer")),                       SLOT (cm_view_log ())),
    MenuLayoutEntry::separator ("print_group"),
    MenuLayoutEntry ("print",                           tl::to_string (QObject::tr ("Print(Ctrl+P)")),                    SLOT (cm_print ())),
    MenuLayoutEntry::separator ("exit_group"),
    MenuLayoutEntry ("exit",                            tl::to_string (QObject::tr ("Exit(Ctrl+Q)")),                     SLOT (cm_exit ())),
    MenuLayoutEntry::last ()
  };

  MenuLayoutEntry edit_utilities_menu [] = {
    MenuLayoutEntry::last ()
  };

  MenuLayoutEntry edit_layout_menu [] = {
    MenuLayoutEntry ("lay_flip_x:edit_mode",            tl::to_string (QObject::tr ("Flip Horizontally")),                SLOT (cm_lay_flip_x ())),
    MenuLayoutEntry ("lay_flip_y:edit_mode",            tl::to_string (QObject::tr ("Flip Vertically")),                  SLOT (cm_lay_flip_y ())),
    MenuLayoutEntry ("lay_rot_cw:edit_mode",            tl::to_string (QObject::tr ("Rotate Clockwise")),                 SLOT (cm_lay_rot_cw ())),
    MenuLayoutEntry ("lay_rot_ccw:edit_mode",           tl::to_string (QObject::tr ("Rotate Counterclockwise")),          SLOT (cm_lay_rot_ccw ())),
    MenuLayoutEntry ("lay_free_rot:edit_mode",          tl::to_string (QObject::tr ("Rotation By Angle")),                SLOT (cm_lay_free_rot ())),
    MenuLayoutEntry ("lay_scale:edit_mode",             tl::to_string (QObject::tr ("Scale")),                            SLOT (cm_lay_scale ())),
    MenuLayoutEntry ("lay_move:edit_mode",              tl::to_string (QObject::tr ("Move By")),                          SLOT (cm_lay_move ())),
    MenuLayoutEntry::separator ("cellop_group"),
    MenuLayoutEntry ("lay_convert_to_static:edit_mode", tl::to_string (QObject::tr ("Convert All Cells To Static")),      SLOT (cm_lay_convert_to_static ())),
    MenuLayoutEntry::last ()
  };

  MenuLayoutEntry edit_cell_menu [] = {
    MenuLayoutEntry ("new_cell:edit:edit_mode",         tl::to_string (QObject::tr ("New Cell")),                         SLOT (cm_new_cell ())),
    MenuLayoutEntry ("delete_cell:edit:edit_mode",      tl::to_string (QObject::tr ("Delete Cell")),                      SLOT (cm_cell_delete ())),
    MenuLayoutEntry ("rename_cell:edit:edit_mode",      tl::to_string (QObject::tr ("Rename Cell")),                      SLOT (cm_cell_rename ())),
    MenuLayoutEntry ("replace_cell:edit:edit_mode",     tl::to_string (QObject::tr ("Replace Cell")),                     SLOT (cm_cell_replace ())),
    MenuLayoutEntry ("flatten_cell:edit:edit_mode",     tl::to_string (QObject::tr ("Flatten Cell")),                     SLOT (cm_cell_flatten ())),
    MenuLayoutEntry::separator ("ops_group"),
    MenuLayoutEntry ("adjust_cell_origin:edit:edit_mode", tl::to_string (QObject::tr ("Adjust Origin")),                  SLOT (cm_adjust_origin ())),
    MenuLayoutEntry ("convert_cell_to_static:edit_mode", tl::to_string (QObject::tr ("Convert Cell To Static")),          SLOT (cm_cell_convert_to_static ())),
    MenuLayoutEntry::separator ("props_group"),
    MenuLayoutEntry ("user_properties",                 tl::to_string (QObject::tr ("User Properties")),                  SLOT (cm_cell_user_properties ())),
    MenuLayoutEntry::last ()
  };

  MenuLayoutEntry edit_layer_menu [] = {
    MenuLayoutEntry ("new_layer:edit:edit_mode",        tl::to_string (QObject::tr ("New Layer")),                        SLOT (cm_new_layer ())),
    MenuLayoutEntry ("clear_layer:edit:edit_mode",      tl::to_string (QObject::tr ("Clear Layer")),                      SLOT (cm_clear_layer ())),
    MenuLayoutEntry ("delete_layer:edit:edit_mode",     tl::to_string (QObject::tr ("Delete Layer")),                     SLOT (cm_delete_layer ())),
    MenuLayoutEntry ("copy_layer:edit:edit_mode",       tl::to_string (QObject::tr ("Copy Layer")),                       SLOT (cm_copy_layer ())),
    MenuLayoutEntry ("edit_layer:edit:edit_mode",       tl::to_string (QObject::tr ("Edit Layer Specification")),         SLOT (cm_edit_layer ())),
    MenuLayoutEntry::last ()
  };

  MenuLayoutEntry edit_selection_menu [] = {
    MenuLayoutEntry ("sel_flip_x",                      tl::to_string (QObject::tr ("Flip Horizontally")),                SLOT (cm_sel_flip_x ())),
    MenuLayoutEntry ("sel_flip_y",                      tl::to_string (QObject::tr ("Flip Vertically")),                  SLOT (cm_sel_flip_y ())),
    MenuLayoutEntry ("sel_rot_cw",                      tl::to_string (QObject::tr ("Rotate Clockwise")),                 SLOT (cm_sel_rot_cw ())),
    MenuLayoutEntry ("sel_rot_ccw",                     tl::to_string (QObject::tr ("Rotate Counterclockwise")),          SLOT (cm_sel_rot_ccw ())),
    MenuLayoutEntry ("sel_free_rot",                    tl::to_string (QObject::tr ("Rotation By Angle")),                SLOT (cm_sel_free_rot ())),
    MenuLayoutEntry ("sel_scale",                       tl::to_string (QObject::tr ("Scale")),                            SLOT (cm_sel_scale ())),
    MenuLayoutEntry ("sel_move",                        tl::to_string (QObject::tr ("Move By")),                          SLOT (cm_sel_move ())),
    MenuLayoutEntry ("sel_move_to",                     tl::to_string (QObject::tr ("Move To")),                          SLOT (cm_sel_move_to ())),
    MenuLayoutEntry::last ()
  };

  MenuLayoutEntry view_menu [] = {
    MenuLayoutEntry ("show_grid",                       tl::to_string (QObject::tr ("Show Grid")),                        std::make_pair (cfg_grid_visible, "?")),
    MenuLayoutEntry ("default_grid:default_grids_group", tl::to_string (QObject::tr ("Grid")),                            empty_menu),
    MenuLayoutEntry::separator ("layout_group"),
    MenuLayoutEntry ("show_markers",                    tl::to_string (QObject::tr ("Show Markers")),                     std::make_pair (cfg_markers_visible, "?")),
    MenuLayoutEntry ("show_texts",                      tl::to_string (QObject::tr ("Show Texts")),                       std::make_pair (cfg_text_visible, "?")),
    MenuLayoutEntry ("show_cell_boxes",                 tl::to_string (QObject::tr ("Show Cell Frames")),                 std::make_pair (cfg_cell_box_visible, "?")),
    MenuLayoutEntry ("no_stipples",                     tl::to_string (QObject::tr ("Show Layers Without Fill")),         std::make_pair (cfg_no_stipple, "?")),
    MenuLayoutEntry ("synchronized_views", tl::to_string (QObject::tr ("Synchronized Views")),   std::make_pair (cfg_synchronized_views, "?")),
    MenuLayoutEntry ("edit_top_level_selection:edit_mode", tl::to_string (QObject::tr ("Select Top Level Objects")),   std::make_pair (edt::cfg_edit_top_level_selection, "?")),
    MenuLayoutEntry::separator ("panels_group"),
    MenuLayoutEntry ("show_toolbar",                    tl::to_string (QObject::tr ("Toolbar")),                          std::make_pair (cfg_show_toolbar, "?")),
    MenuLayoutEntry ("show_navigator",                  tl::to_string (QObject::tr ("Navigator")),                        std::make_pair (cfg_show_navigator, "?")),
    MenuLayoutEntry ("show_layer_panel",                tl::to_string (QObject::tr ("Layers")),                           std::make_pair (cfg_show_layer_panel, "?")),
    MenuLayoutEntry ("show_layer_toolbox",              tl::to_string (QObject::tr ("Layer Toolbox")),                    std::make_pair (cfg_show_layer_toolbox, "?")),
    MenuLayoutEntry ("show_hierarchy_panel",            tl::to_string (QObject::tr ("Cells")),                            std::make_pair (cfg_show_hierarchy_panel, "?")),
    MenuLayoutEntry ("show_libraries_view",             tl::to_string (QObject::tr ("Libraries")),                        std::make_pair (cfg_show_libraries_view, "?")),
    MenuLayoutEntry ("show_bookmarks_view",             tl::to_string (QObject::tr ("Bookmarks")),                        std::make_pair (cfg_show_bookmarks_view, "?")),
    MenuLayoutEntry ("reset_window_state",              tl::to_string (QObject::tr ("Restore Window")),                   SLOT (cm_reset_window_state ())),
    MenuLayoutEntry::separator ("selection_group"),
    MenuLayoutEntry ("transient_selection",             tl::to_string (QObject::tr ("Highlight Object Under Mouse")),     std::make_pair (cfg_sel_transient_mode, "?")),
    MenuLayoutEntry::last ()
  };

  MenuLayoutEntry edit_select_menu [] = {
    MenuLayoutEntry ("select_all",                      tl::to_string (QObject::tr ("Select All")),                       SLOT (cm_select_all ())),
    MenuLayoutEntry ("unselect_all",                    tl::to_string (QObject::tr ("Unselect All")),                     SLOT (cm_unselect_all ())),
    MenuLayoutEntry::separator ("edit_select_basic_group"),
    MenuLayoutEntry ("enable_all",                      tl::to_string (QObject::tr ("Enable All")),                       SLOT (enable_all ())),
    MenuLayoutEntry ("disable_all",                     tl::to_string (QObject::tr ("Disable All")),                      SLOT (disable_all ())),
    MenuLayoutEntry::separator ("edit_select_individual_group"),
    MenuLayoutEntry::last ()
  };

  MenuLayoutEntry edit_menu [] = {
    MenuLayoutEntry ("undo:edit",                       tl::to_string (QObject::tr ("Undo(Ctrl+Z)")),                     SLOT (cm_undo ())),
    MenuLayoutEntry ("redo:edit",                       tl::to_string (QObject::tr ("Redo(Ctrl+Y)")),                     SLOT (cm_redo ())),
    MenuLayoutEntry::separator ("basic_group"),
    MenuLayoutEntry ("layout_menu:edit:edit_mode",      tl::to_string (QObject::tr ("Layout")),                           edit_layout_menu),
    MenuLayoutEntry ("cell_menu:edit:edit_mode",        tl::to_string (QObject::tr ("Cell")),                             edit_cell_menu),
    MenuLayoutEntry ("layer_menu:edit:edit_mode",       tl::to_string (QObject::tr ("Layer")),                            edit_layer_menu),
    MenuLayoutEntry ("selection_menu:edit",             tl::to_string (QObject::tr ("Selection")),                        edit_selection_menu),
    MenuLayoutEntry::separator ("utils_group"),
    MenuLayoutEntry ("utils_menu:edit:edit_mode",       tl::to_string (QObject::tr ("Utilities")),                        edit_utilities_menu),
    MenuLayoutEntry::separator ("misc_group") ,
    MenuLayoutEntry ("delete:edit",                     tl::to_string (QObject::tr ("Delete(Del)")),                      SLOT (cm_delete ())),
    MenuLayoutEntry ("show_properties:edit",            tl::to_string (QObject::tr ("Properties(Q)")),                    SLOT (cm_show_properties ())),
    MenuLayoutEntry::separator ("cpc_group"),
    MenuLayoutEntry ("copy:edit",                       tl::to_string (QObject::tr ("Copy(Ctrl+C)")),                     SLOT (cm_copy ())),
    MenuLayoutEntry ("cut:edit",                        tl::to_string (QObject::tr ("Cut(Ctrl+X)")),                      SLOT (cm_cut ())),
    MenuLayoutEntry ("paste:edit",                      tl::to_string (QObject::tr ("Paste(Ctrl+V)")),                    SLOT (cm_paste ())),
    MenuLayoutEntry ("duplicate:edit",                  tl::to_string (QObject::tr ("Duplicate(Ctrl+B)")),                SLOT (cm_duplicate ())),
    MenuLayoutEntry::separator ("modes_group"),
    MenuLayoutEntry ("mode_menu",                       tl::to_string (QObject::tr ("Mode")),                             empty_menu),
    MenuLayoutEntry ("select_menu",                     tl::to_string (QObject::tr ("Select")),                           edit_select_menu),
    MenuLayoutEntry::separator ("cancel_group"),
    MenuLayoutEntry ("cancel",                          tl::to_string (QObject::tr ("Cancel(Esc)")),                      SLOT (cm_cancel ())),
    MenuLayoutEntry::last ()
  };

  MenuLayoutEntry bookmark_menu [] = {
    MenuLayoutEntry ("goto_bookmark_menu",              tl::to_string (QObject::tr ("Goto Bookmark")),                    empty_menu),
    MenuLayoutEntry ("bookmark_view",                   tl::to_string (QObject::tr ("Bookmark This View")),               SLOT (cm_bookmark_view ())),
    MenuLayoutEntry::separator ("bookmark_mgm_group"),
    MenuLayoutEntry ("manage_bookmarks",                tl::to_string (QObject::tr ("Manage Bookmarks")),                 SLOT (cm_manage_bookmarks ())),
    MenuLayoutEntry ("load_bookmarks",                  tl::to_string (QObject::tr ("Load Bookmarks")),                   SLOT (cm_load_bookmarks ())),
    MenuLayoutEntry ("save_bookmarks",                  tl::to_string (QObject::tr ("Save Bookmarks")),                   SLOT (cm_save_bookmarks ())),
    MenuLayoutEntry::last ()
  };

  MenuLayoutEntry global_trans_menu [] = {
    MenuLayoutEntry ("r0",                              tl::to_string (QObject::tr ("\\(r0\\)<:/r0.png>")),               std::make_pair (cfg_global_trans, "?r0 *1 0,0")),
    MenuLayoutEntry ("r90",                             tl::to_string (QObject::tr ("\\(r90\\)<:/r90.png>")),             std::make_pair (cfg_global_trans, "?r90 *1 0,0")),
    MenuLayoutEntry ("r180",                            tl::to_string (QObject::tr ("\\(r180\\)<:/r180.png>")),           std::make_pair (cfg_global_trans, "?r180 *1 0,0")),
    MenuLayoutEntry ("r270",                            tl::to_string (QObject::tr ("\\(r270\\)<:/r270.png>")),           std::make_pair (cfg_global_trans, "?r270 *1 0,0")),
    MenuLayoutEntry ("m0",                              tl::to_string (QObject::tr ("\\(m0\\)<:/m0.png>")),               std::make_pair (cfg_global_trans, "?m0 *1 0,0")),
    MenuLayoutEntry ("m45",                             tl::to_string (QObject::tr ("\\(m45\\)<:/m45.png>")),             std::make_pair (cfg_global_trans, "?m45 *1 0,0")),
    MenuLayoutEntry ("m90",                             tl::to_string (QObject::tr ("\\(m90\\)<:/m90.png>")),             std::make_pair (cfg_global_trans, "?m90 *1 0,0")),
    MenuLayoutEntry ("m135",                            tl::to_string (QObject::tr ("\\(m135\\)<:/m135.png>")),           std::make_pair (cfg_global_trans, "?m135 *1 0,0")),
    MenuLayoutEntry::last ()
  };

  MenuLayoutEntry zoom_menu [] = {
    MenuLayoutEntry ("global_trans",                    tl::to_string (QObject::tr ("Global Transformation")),            global_trans_menu),
    MenuLayoutEntry ("synchronized_views",              tl::to_string (QObject::tr ("Synchronized Views")),               std::make_pair (cfg_synchronized_views, "?")),
    MenuLayoutEntry::separator ("hier_group"),
    MenuLayoutEntry ("max_hier",                        tl::to_string (QObject::tr ("Full Hierarchy(*)")),                SLOT (cm_max_hier ())),
    MenuLayoutEntry ("max_hier_0",                      tl::to_string (QObject::tr ("Box Only(0)")),                      SLOT (cm_max_hier_0 ())),
    MenuLayoutEntry ("max_hier_1",                      tl::to_string (QObject::tr ("Top Level Only(1)")),                SLOT (cm_max_hier_1 ())),
    MenuLayoutEntry ("inc_max_hier",                    tl::to_string (QObject::tr ("Increment Hierarchy(+)")),           SLOT (cm_inc_max_hier ())),
    MenuLayoutEntry ("dec_max_hier",                    tl::to_string (QObject::tr ("Decrement Hierarchy(-)")),           SLOT (cm_dec_max_hier ())),
    MenuLayoutEntry::separator ("zoom_group"),
    MenuLayoutEntry ("zoom_fit",                        tl::to_string (QObject::tr ("Zoom Fit(F2)")),                     SLOT (cm_zoom_fit ())),
    MenuLayoutEntry ("zoom_fit_sel",                    tl::to_string (QObject::tr ("Zoom Fit Selection(Shift+F2)")),     SLOT (cm_zoom_fit_sel ())),
    MenuLayoutEntry ("zoom_in",                         tl::to_string (QObject::tr ("Zoom In(Return)")),                  SLOT (cm_zoom_in ())),
    MenuLayoutEntry ("zoom_out",                        tl::to_string (QObject::tr ("Zoom Out(Shift+Return)")),           SLOT (cm_zoom_out ())),
    /* disabled because that interferes with the use of the arrow keys for moving the selection
    MenuLayoutEntry::separator ("pan_group"),
    MenuLayoutEntry ("pan_up",                          tl::to_string (QObject::tr ("Pan Up(Up)")),                       SLOT (cm_pan_up ())),
    MenuLayoutEntry ("pan_down",                        tl::to_string (QObject::tr ("Pan Down(Down)")),                   SLOT (cm_pan_down ())),
    MenuLayoutEntry ("pan_left",                        tl::to_string (QObject::tr ("Pan Left(Left)")),                   SLOT (cm_pan_left ())),
    MenuLayoutEntry ("pan_right",                       tl::to_string (QObject::tr ("Pan Right(Right)")),                 SLOT (cm_pan_right ())),
    */
    MenuLayoutEntry::separator ("redraw_group"),
    MenuLayoutEntry ("redraw",                          tl::to_string (QObject::tr ("Redraw")),                           SLOT (cm_redraw ())),
    MenuLayoutEntry::separator ("state_group"),
    MenuLayoutEntry ("prev_display_state",              tl::to_string (QObject::tr ("Back(Shift+Tab)<:/back.png>")),      SLOT (cm_prev_display_state ())),
    MenuLayoutEntry ("next_display_state",              tl::to_string (QObject::tr ("Forward(Tab)<:/forward.png>")),      SLOT (cm_next_display_state ())),
    MenuLayoutEntry::separator ("select_group"),
    MenuLayoutEntry ("select_cell:edit",                tl::to_string (QObject::tr ("Select Cell")),                      SLOT (cm_select_cell ())),
    MenuLayoutEntry ("select_current_cell",             tl::to_string (QObject::tr ("Show As New Top(Ctrl+S)")),          SLOT (cm_select_current_cell ())),
    MenuLayoutEntry ("goto_position",                   tl::to_string (QObject::tr ("Goto Position(Ctrl+G)")),            SLOT (cm_goto_position ())),
    MenuLayoutEntry::last ()
  };

  MenuLayoutEntry help_menu [] = {
    MenuLayoutEntry ("show_all_tips",                   tl::to_string (QObject::tr ("Show All Tips")),                    SLOT (cm_show_all_tips ())),
    MenuLayoutEntry::separator ("help_topics_group"),
    MenuLayoutEntry ("assistant",                       tl::to_string (QObject::tr ("Assistant")),                        SLOT (cm_show_assistant ())),
    MenuLayoutEntry ("about",                           tl::to_string (QObject::tr ("About")),                            SLOT (cm_help_about ())),
    MenuLayoutEntry ("about_qt",                        tl::to_string (QObject::tr ("About Qt")),                         SLOT (cm_help_about_qt ())),
    MenuLayoutEntry::last ()
  };

  MenuLayoutEntry tools_menu [] = {
    MenuLayoutEntry ("packages",                        tl::to_string (QObject::tr ("Manage Packages")),                  SLOT (cm_packages ())),
    MenuLayoutEntry ("technologies",                    tl::to_string (QObject::tr ("Manage Technologies")),              SLOT (cm_technologies ())),
    MenuLayoutEntry::separator ("verification_group"),
    MenuLayoutEntry::separator ("post_verification_group"),
    MenuLayoutEntry::last ()
  };

  MenuLayoutEntry macros_menu [] = {
    MenuLayoutEntry ("macro_development",               tl::to_string (QObject::tr ("Macro Development(F5)")),            SLOT (cm_macro_editor ())),
    MenuLayoutEntry::separator ("macros_group"),
    MenuLayoutEntry::last ()
  };

  MenuLayoutEntry toolbar_entries [] = {
    MenuLayoutEntry ("prev_display_state",              "-",                                                              SLOT (cm_prev_display_state ())),
    MenuLayoutEntry ("next_display_state",              "-",                                                              SLOT (cm_next_display_state ())),
    MenuLayoutEntry::separator ("toolbar_post_navigation_group"),
    MenuLayoutEntry::last ()
  };

  MenuLayoutEntry main_menu [] = {
    MenuLayoutEntry ("file_menu",                       tl::to_string (QObject::tr ("&File")),                            file_menu),
    MenuLayoutEntry ("edit_menu",                       tl::to_string (QObject::tr ("&Edit")),                            edit_menu),
    MenuLayoutEntry ("view_menu",                       tl::to_string (QObject::tr ("&View")),                            view_menu),
    MenuLayoutEntry ("bookmark_menu",                   tl::to_string (QObject::tr ("&Bookmarks")),                       bookmark_menu),
    MenuLayoutEntry ("zoom_menu",                       tl::to_string (QObject::tr ("&Display")),                         zoom_menu),
    MenuLayoutEntry ("tools_menu",                      tl::to_string (QObject::tr ("&Tools")),                           tools_menu),
    MenuLayoutEntry ("macros_menu",                     tl::to_string (QObject::tr ("&Macros")),                          macros_menu),
    MenuLayoutEntry::separator ("help_group"),
    MenuLayoutEntry ("help_menu",                       tl::to_string (QObject::tr ("&Help")),                            help_menu),
    MenuLayoutEntry ("@secrets",                        tl::to_string (QObject::tr ("Secret Features")),                  secret_menu),
    MenuLayoutEntry ("@toolbar",                        "",                                                               toolbar_entries),
    MenuLayoutEntry::last ()
  };

  mp_menu = new AbstractMenu (this);
  mp_menu->init (main_menu);

  lay::LayoutView::init_menu (*mp_menu);
  lay::Navigator::init_menu (*mp_menu);

  //  Fill the mode menu file items from the intrinsic mouse modes
  //  TODO: map the intrinsic modes to standard plugins and remove this code along
  //  with related code in LayoutView. After this we can simplify the menu inititialization
  //  inside PluginDeclaration as well.

  std::vector <std::string> mode_titles;
  lay::LayoutView::intrinsic_mouse_modes (&mode_titles);

  int mode_id = 0;
  for (std::vector <std::string>::const_iterator t = mode_titles.begin (); t != mode_titles.end (); ++t, --mode_id) {

    if (! t->empty ()) {

      //  extract first part, which is the name, separated by a tab from the title.
      std::string name = tl::sprintf ("mode_i%d", 1 - mode_id);
      std::string title = *t;
      const char *tab = strchr (t->c_str (), '\t');
      if (tab) {
        name = std::string (*t, 0, tab - t->c_str ());
        title = tab + 1;
      }

      Action action (title);
      action.set_checkable (true);
      action.qaction ()->setData (QVariant (mode_id));
      action.add_to_exclusive_group (mp_menu, "mouse_mode_exclusive_group");

      mp_menu->insert_item ("edit_menu.mode_menu.end", name, action);
      mp_menu->insert_item ("@toolbar.end", name, action);

      gtf::action_connect (action.qaction (), SIGNAL (triggered ()), this, SLOT (intrinsic_mode_triggered ()));

    }

  }

  //  Add a hook for inserting new items after the modes
  Action end_modes;
  end_modes.set_separator (true);
  mp_menu->insert_item ("@toolbar.end", "end_modes", end_modes);

  //  make the plugins create their menu items
  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    //  TODO: get rid of the const_cast hack
    const_cast <lay::PluginDeclaration *> (&*cls)->init_menu ();
  }

  //  if in "viewer-only mode", hide all entries in the "hide_vo" group
  if ((lay::ApplicationBase::instance () && lay::ApplicationBase::instance ()->is_vo_mode ())) {
    std::vector<std::string> hide_vo_grp = mp_menu->group ("hide_vo");
    for (std::vector<std::string>::const_iterator g = hide_vo_grp.begin (); g != hide_vo_grp.end (); ++g) {
      mp_menu->action (*g).set_visible (false);
    }
  }

  //  if not in editable mode, hide all entries from "edit_mode" group
  //  TODO: later do this on each change of the view - each view might get it's own editable mode
  bool view_mode = (lay::ApplicationBase::instance () && !lay::ApplicationBase::instance ()->is_editable ());

  std::vector<std::string> edit_mode_grp = mp_menu->group ("edit_mode");
  for (std::vector<std::string>::const_iterator g = edit_mode_grp.begin (); g != edit_mode_grp.end (); ++g) {
    mp_menu->action (*g).set_visible (! view_mode);
  }

  std::vector<std::string> view_mode_grp = mp_menu->group ("view_mode");
  for (std::vector<std::string>::const_iterator g = view_mode_grp.begin (); g != view_mode_grp.end (); ++g) {
    mp_menu->action (*g).set_visible (view_mode);
  }
}

void
MainWindow::dock_widget_visibility_changed (bool /*visible*/)
{
  if (sender () == mp_lp_dock_widget) {
    plugin_root ()->config_set (cfg_show_layer_panel, tl::to_string (!mp_lp_dock_widget->isHidden ()));
  } else if (sender () == mp_hp_dock_widget) {
    plugin_root ()->config_set (cfg_show_hierarchy_panel, tl::to_string (!mp_hp_dock_widget->isHidden ()));
  } else if (sender () == mp_libs_dock_widget) {
    plugin_root ()->config_set (cfg_show_libraries_view, tl::to_string (!mp_libs_dock_widget->isHidden ()));
  } else if (sender () == mp_bm_dock_widget) {
    plugin_root ()->config_set (cfg_show_bookmarks_view, tl::to_string (!mp_bm_dock_widget->isHidden ()));
  } else if (sender () == mp_navigator_dock_widget) {
    plugin_root ()->config_set (cfg_show_navigator, tl::to_string (!mp_navigator_dock_widget->isHidden ()));
  } else if (sender () == mp_layer_toolbox_dock_widget) {
    plugin_root ()->config_set (cfg_show_layer_toolbox, tl::to_string (!mp_layer_toolbox_dock_widget->isHidden ()));
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

  //  Prevent recursive signals
  m_file_changed_timer.blockSignals (true);

  std::set<QString> reloaded_files;

BEGIN_PROTECTED

  //  Make the names unique
  std::sort (m_changed_files.begin (), m_changed_files.end ());
  m_changed_files.erase (std::unique (m_changed_files.begin (), m_changed_files.end ()), m_changed_files.end ());

  if (m_changed_files.empty ()) {
    return;
  }

  QString msg;

  if (m_changed_files.size () == 1) {
    msg = QObject::tr ("The following file has been changed on disk:\n\n");
    for (std::vector<QString>::const_iterator f = m_changed_files.begin (); f != m_changed_files.end (); ++f) {
      msg += QString::fromUtf8 ("  %1\n").arg (*f);
    }
    msg += tr ("\nReload this file?");
  } else {
    msg = QObject::tr ("The following files have been changed on disk:\n\n");
    for (std::vector<QString>::const_iterator f = m_changed_files.begin (); f != m_changed_files.end (); ++f) {
      msg += QString::fromUtf8 ("  %1\n").arg (*f);
    }
    msg += tr ("\nReload these files?");
  }

  std::vector<QString> changed_files;
  changed_files.swap (m_changed_files);

  if (QMessageBox::question (this, tr ("Reload Files"), msg, QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {

    m_file_changed_timer.blockSignals (false);

    std::map<QString, std::pair<lay::LayoutView *, int> > views_per_file;

    for (std::vector<lay::LayoutView *>::iterator v = mp_views.begin (); v != mp_views.end (); ++v) {
      for (int cv = 0; cv < int ((*v)->cellviews ()); ++cv) {
        views_per_file [tl::to_qstring ((*v)->cellview (cv)->filename ())] = std::make_pair (*v, cv);
      }
    }

    for (std::vector<QString>::const_iterator f = changed_files.begin (); f != changed_files.end (); ++f) {
      std::map<QString, std::pair<lay::LayoutView *, int> >::const_iterator v = views_per_file.find (*f);
      if (v != views_per_file.end ()) {
        v->second.first->reload_layout (v->second.second);
        reloaded_files.insert (*f);
      }
    }

  }

END_PROTECTED

  m_file_changed_timer.blockSignals (false);

  //  While the message box was open, new request might have collected - remove
  //  the ones we just reloaded and restart the timer
  if (! m_changed_files.empty ()) {

    std::vector<QString> changed_files;
    changed_files.swap (m_changed_files);
    for (std::vector<QString>::const_iterator f = changed_files.begin (); f != changed_files.end (); ++f) {
      if (reloaded_files.find (*f) == reloaded_files.end ()) {
        m_changed_files.push_back (*f);
      }
    }

    if (! m_changed_files.empty ()) {

      //  Wait a little to let more to allow for more reload requests to collect
      m_file_changed_timer.setInterval (300);
      m_file_changed_timer.start ();

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
  mp_layer_toolbox->set_view (0);

  //  try a smooth shutdown of the current view
  lay::LayoutView::set_current (0);

  current_view_changed ();

  for (unsigned int i = 0; i < mp_views.size (); ++i) {
    mp_views [i]->stop ();
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

    lay::LayoutView *view = mp_views.back ();
    mp_views.pop_back ();
    mp_lp_stack->removeWidget (mp_views.size ());
    mp_hp_stack->removeWidget (mp_views.size ());
    mp_libs_stack->removeWidget (mp_views.size ());
    mp_bm_stack->removeWidget (mp_views.size ());
    mp_view_stack->removeWidget (mp_views.size ());

    delete view;

  }

  update_dock_widget_state ();
}

void
MainWindow::about_to_exec ()
{
  bool f;

  f = false;
  config_get (cfg_full_hier_new_cell, f);
  if (!f) {
    TipDialog td (this,
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
        plugin_root ()->config_set (cfg_full_hier_new_cell, true);
      }
      //  Don't bother the user with more dialogs.
      return;
    }
  }

  //  TODO: later, each view may get it's own editable flag
  if (lay::ApplicationBase::instance () && !lay::ApplicationBase::instance ()->is_editable ()) {
    TipDialog td (this, 
                  tl::to_string (QObject::tr ("KLayout has been started in viewer mode. In this mode, editor functions are not available.\n\nTo enable these functions, start KLayout in editor mode by using the \"-e\" command line switch or select it as the default mode in the setup dialog. Choose \"Setup\" in the \"File\" menu and check \"Use editing mode by default\" on the \"Editing Mode\" page in the \"Application\" section.")), 
                  "editor-mode");
    if (td.exec_dialog ()) {
      //  Don't bother the user with more dialogs.
      return;
    }
  }

  f = false;
  config_get (cfg_no_stipple, f);
  if (f) {
    TipDialog td (this,
                  tl::to_string (QObject::tr ("Layers are shown without fill because fill has been intentionally turned off. This can be confusing since selecting a stipple does not have an effect in this case.\n\nTo turn this feature off, uncheck \"Show Layers Without Fill\" in the \"View\" menu.")),
                  "no-stipple");
    if (td.exec_dialog ()) {
      //  Don't bother the user with more dialogs.
      return;
    }
  }

  f = false;
  config_get (cfg_markers_visible, f);
  if (! f) {
    TipDialog td (this,
                  tl::to_string (QObject::tr ("Markers are not visible because they have been turned off.\nYou may not see markers when using the marker browser feature.\n\nTo turn markers on, check \"Show Markers\" in the \"View\" menu.")),
                  "show-markers");
    if (td.exec_dialog ()) {
      //  Don't bother the user with more dialogs.
      return;
    }
  }

  f = false;
  config_get (cfg_hide_empty_layers, f);
  if (f) {
    TipDialog td (this,
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

  } while (short_message.size () < prev_len && fm.width (QString::fromUtf8 (" ") + tl::to_qstring (short_message)) > mp_msg_label->width ());

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
    restoreState (state);
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

    for (std::vector<lay::Action *>::iterator a = m_default_grid_actions.begin (); a != m_default_grid_actions.end (); ++a) {
      delete *a;
    }
    m_default_grid_actions.clear ();

    int i = 1;
    for (std::vector<double>::const_iterator g = m_default_grids.begin (); g != m_default_grids.end (); ++g, ++i) {

      std::string name = "default_grid_" + tl::to_string (i);

      m_default_grid_actions.push_back (create_config_action (tl::to_string (*g) + tl::to_string (QObject::tr (" um")), cfg_grid, tl::to_string (*g)));

      m_default_grid_actions.back ()->set_checkable (true);
      m_default_grid_actions.back ()->set_checked (fabs (*g - m_grid_micron) < 1e-10);

      for (std::vector<std::string>::const_iterator t = group.begin (); t != group.end (); ++t) {
        menu ()->insert_item (*t + ".end", name, *m_default_grid_actions.back ());
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
  std::map<std::string, std::vector<lay::ConfigureAction *> >::iterator ca = m_configuration_actions.find (name);
  if (ca != m_configuration_actions.end ()) {
    for (std::vector<lay::ConfigureAction *>::const_iterator a = ca->second.begin (); a != ca->second.end (); ++a) {
      (*a)->configure (value);
    }
  }

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

    mp_layer_toolbox->set_palette (palette);

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

    mp_layer_toolbox->set_palette (palette);

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

    mp_layer_toolbox->set_palette (palette);

    // others need this property too ..
    return false;

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

    dm_do_update_file_menu ();

    return true;

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

  } else {
    return false;
  }

}

void
MainWindow::apply_hidden (const std::vector<std::pair<std::string, bool> > &hidden)
{
  for (std::vector<std::pair<std::string, bool> >::const_iterator hf = hidden.begin (); hf != hidden.end (); ++hf) {
    if (menu ()->is_valid (hf->first)) {
      lay::Action a = menu ()->action (hf->first);
      a.set_hidden (hf->second);
    }
  }
}

void
MainWindow::apply_key_bindings ()
{
  for (std::vector<std::pair<std::string, std::string> >::const_iterator kb = m_key_bindings.begin (); kb != m_key_bindings.end (); ++kb) {
    if (menu ()->is_valid (kb->first)) {
      lay::Action a = menu ()->action (kb->first);
      a.set_shortcut (kb->second);
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

  std::vector<std::string> edit_grp = mp_menu->group ("edit");
  for (std::vector<std::string>::const_iterator g = edit_grp.begin (); g != edit_grp.end (); ++g) {
    mp_menu->action (*g).set_enabled (enable);
  }
}

void
MainWindow::menu_needs_update ()
{
  lay::LayoutView::update_menu (current_view (), *mp_menu);
}

void
MainWindow::libraries_changed ()
{
  //  if the libraries have changed, remove all selections and cancel any operations to avoid
  //  that the view refers to an invalid instance or shape
  for (std::vector <lay::LayoutView *>::iterator vp = mp_views.begin (); vp != mp_views.end (); ++vp) {
    (*vp)->clear_selection ();
    (*vp)->cancel ();
  }
}

void
MainWindow::read_dock_widget_state ()
{
  plugin_root ()->config_set (cfg_show_layer_panel, tl::to_string (!mp_lp_dock_widget->isHidden ()));
  plugin_root ()->config_set (cfg_show_hierarchy_panel, tl::to_string (!mp_hp_dock_widget->isHidden ()));
  plugin_root ()->config_set (cfg_show_libraries_view, tl::to_string (!mp_libs_dock_widget->isHidden ()));
  plugin_root ()->config_set (cfg_show_bookmarks_view, tl::to_string (!mp_bm_dock_widget->isHidden ()));
  plugin_root ()->config_set (cfg_show_navigator, tl::to_string (!mp_navigator_dock_widget->isHidden ()));
  plugin_root ()->config_set (cfg_show_layer_toolbox, tl::to_string (!mp_layer_toolbox_dock_widget->isHidden ()));
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
    if (! pd->can_exit (plugin_root ())) {
      return false;
    }
  }

  std::string df_list;
  int dirty_layouts = dirty_files (df_list);

  if (dirty_layouts == 0) {
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
  plugin_root ()->config_set (cfg_window_geometry, (const char *) saveGeometry ().toBase64 ().data ());
  plugin_root ()->config_set (cfg_window_state, (const char *) saveState ().toBase64 ().data ());
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
  BEGIN_PROTECTED
  if (mp_navigator) {
    mp_navigator->freeze_clicked ();
  }
  END_PROTECTED
}

void
MainWindow::cm_navigator_close ()
{
  BEGIN_PROTECTED
  if (mp_navigator) {
    mp_navigator->close ();
  }
  END_PROTECTED
}

void
MainWindow::cm_view_log ()
{
  BEGIN_PROTECTED
  mp_log_viewer_dialog->show ();
  END_PROTECTED
}

void
MainWindow::cm_print ()
{
  //  TODO: move to lay::LayoutView

  BEGIN_PROTECTED

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

      //  choose a resolution around 300dpi
      double rf = floor (0.5 + 300.0 / mp_printer->resolution ());
      mp_printer->setResolution (int (floor (0.5 + mp_printer->resolution () * rf)));

      QPainter painter;

      painter.begin (mp_printer.get ());

      QFont header_font (QString::fromUtf8 ("Helvetica"), 8);
      int hh = QFontMetrics (header_font, mp_printer.get ()).height ();

      QRect page_rect = mp_printer->pageRect ();
      page_rect.moveTo (0, 0);

      int b = std::min (page_rect.width (), page_rect.height ()) / 50;
      page_rect.setLeft (page_rect.left () + b);
      page_rect.setRight (page_rect.right () - b);
      page_rect.setTop (page_rect.top () + b);
      page_rect.setBottom (page_rect.bottom () - b);

      QRect text_rect = page_rect;
      text_rect.setLeft (text_rect.left () + hh / 2);
      text_rect.setRight (text_rect.right () - hh / 2);
      text_rect.setBottom (text_rect.bottom () - hh / 2);
      text_rect.setTop (text_rect.top () + hh / 2);

      QImage img = current_view ()->get_image_with_options (page_rect.width (), page_rect.height () - 4 * hh, 1, 1, 1.0 / 3.0, Qt::white, Qt::black, Qt::black, db::DBox (), false);

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

  END_PROTECTED
}

void
MainWindow::cm_exit ()
{
  BEGIN_PROTECTED
  exit ();
  END_PROTECTED
}

lay::LayoutView *
MainWindow::view (int index)
{
  if (index >= 0 && index < int (mp_views.size ())) {
    return mp_views [index];
  } else {
    return 0;
  }
}

const lay::LayoutView *
MainWindow::view (int index) const
{
  if (index >= 0 && index < int (mp_views.size ())) {
    return mp_views [index];
  } else {
    return 0;
  }
}

int
MainWindow::index_of (const lay::LayoutView *view) const
{
  for (int i = 0; i < int (mp_views.size ()); ++i) {
    if (mp_views [i] == view) {
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
MainWindow::cm_show_properties ()
{
  if (current_view ()) {
    current_view ()->show_properties (this);
  }
}

void
MainWindow::cm_delete ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->del ();
    //  because a "delete" might involve objects currently edited, we cancel the edit after we have deleted the object
    current_view ()->cancel ();
    current_view ()->clear_selection ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_paste ()
{
  BEGIN_PROTECTED
  current_view ()->cm_layer_paste ();
  END_PROTECTED
}

void
MainWindow::cm_lv_cut ()
{
  BEGIN_PROTECTED
  current_view ()->cm_layer_cut ();
  END_PROTECTED
}

void
MainWindow::cm_lv_copy ()
{
  BEGIN_PROTECTED
  current_view ()->cm_layer_copy ();
  END_PROTECTED
}

void
MainWindow::cm_cell_paste ()
{
  BEGIN_PROTECTED
  current_view ()->cm_cell_paste ();
  END_PROTECTED
}

void
MainWindow::cm_cell_cut ()
{
  BEGIN_PROTECTED
  current_view ()->cm_cell_cut ();
  END_PROTECTED
}

void
MainWindow::cm_cell_copy ()
{
  BEGIN_PROTECTED
  current_view ()->cm_cell_copy ();
  END_PROTECTED
}

void
MainWindow::do_cm_duplicate (bool interactive)
{
  BEGIN_PROTECTED

  if (current_view ()) {

    //  Do duplicate simply by concatenating copy & paste currently.
    //  Save the clipboard state before in order to preserve the current content
    db::Clipboard saved_clipboard;
    db::Clipboard::instance ().swap (saved_clipboard);

    try {
      current_view ()->copy ();
      current_view ()->clear_selection ();
      current_view ()->cancel ();
      if (interactive) {
        current_view ()->paste_interactive ();
      } else {
        current_view ()->paste ();
      }
      db::Clipboard::instance ().swap (saved_clipboard);
    } catch (...) {
      db::Clipboard::instance ().swap (saved_clipboard);
      throw;
    }

  }

  END_PROTECTED
}

void
MainWindow::cm_duplicate ()
{
  BEGIN_PROTECTED

    do_cm_duplicate (false);

  END_PROTECTED
}

void
MainWindow::cm_duplicate_interactive ()
{
  BEGIN_PROTECTED

    do_cm_duplicate (true);

  END_PROTECTED
}

void
MainWindow::cm_copy ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->copy ();
    current_view ()->clear_selection ();
  }

  END_PROTECTED
}

void
MainWindow::do_cm_paste (bool interactive)
{
  BEGIN_PROTECTED

  if (current_view () && ! db::Clipboard::instance ().empty ()) {
    current_view ()->cancel ();
    current_view ()->clear_selection ();
    if (interactive) {
      current_view ()->paste_interactive ();
    } else {
      current_view ()->paste ();
    }
  }

  END_PROTECTED
}

void
MainWindow::cm_paste ()
{
  do_cm_paste (false);
}

void
MainWindow::cm_paste_interactive ()
{
  do_cm_paste (true);
}

void
MainWindow::cm_cut ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cut ();
    current_view ()->cancel (); //  see del() for reason why cancel is after cut
    current_view ()->clear_selection ();
  }

  END_PROTECTED
}

void
MainWindow::intrinsic_mode_triggered ()
{
  BEGIN_PROTECTED

  QAction *action = dynamic_cast<QAction *> (sender ());
  if (action) {

    int mode = action->data ().toInt ();

    plugin_root ()->select_mode (mode);

    action->setChecked (true);

  }

  END_PROTECTED
}

void
MainWindow::select_mode (int m)
{
  if (m_mode != m) {

    m_mode = m;
    for (std::vector <lay::LayoutView *>::iterator vp = mp_views.begin (); vp != mp_views.end (); ++vp) {
      (*vp)->mode (m);
    }

    //  Update the actions by checking the one that is selected programmatically. Use the toolbar menu for reference.
    //  TODO: this code needs to be kept aligned with the implementation of PluginDeclaration::init_menu ()
    //  It's not easy to move the functionality to PluginDeclaration because some of the modes are not mapped to
    //  Plugin's yet (selection, move).
    std::vector<std::string> items = menu ()->items ("@toolbar");
    for (std::vector<std::string>::const_iterator i = items.begin (); i != items.end (); ++i) {
      Action a = menu ()->action (*i);
      if (a.qaction ()->isCheckable() && a.qaction ()->data ().toInt () == m_mode) {
        a.set_checked (true);
        break;
      }
    }

  }
}

void
MainWindow::enable_all ()
{
  BEGIN_PROTECTED

  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    cls->set_editable_enabled (true);
  }

  END_PROTECTED
}

void
MainWindow::disable_all ()
{
  BEGIN_PROTECTED

  for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
    cls->set_editable_enabled (false);
  }

  END_PROTECTED
}

void
MainWindow::cm_unselect_all ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->select (db::DBox (), lay::Editable::Reset);
  }

  END_PROTECTED
}

void
MainWindow::cm_reset_window_state ()
{
  restoreState (m_default_window_state);
  restoreGeometry (m_default_window_geometry);
}

void
MainWindow::cm_select_all ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    //  TODO: "select all" with an empty box is not well implemented in most services.
    //  Hence we use the overlapp box currently.
    current_view ()->select (current_view ()->full_box (), lay::Editable::Replace);
  }

  END_PROTECTED
}

void
MainWindow::cm_undo ()
{
  BEGIN_PROTECTED

  if (current_view () && m_manager.available_undo ().first) {
    for (std::vector <lay::LayoutView *>::iterator vp = mp_views.begin (); vp != mp_views.end (); ++vp) {
      (*vp)->clear_selection ();
      (*vp)->cancel ();
    }
    m_manager.undo ();
  }

  END_PROTECTED
}

void
MainWindow::cm_redo ()
{
  BEGIN_PROTECTED

  if (current_view () && m_manager.available_redo ().first) {
    for (std::vector <lay::LayoutView *>::iterator vp = mp_views.begin (); vp != mp_views.end (); ++vp) {
      (*vp)->clear_selection ();
      (*vp)->cancel ();
    }
    m_manager.redo ();
  }

  END_PROTECTED
}

void
MainWindow::cm_goto_position ()
{
  BEGIN_PROTECTED

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
        ex >> x >> "," >> y;

        db::DPoint pt (x, y);

        if (! ex.at_end ()) {
          ex >> "," >> s >> tl::Extractor::end ();
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

  END_PROTECTED
}

void
MainWindow::cm_manage_bookmarks ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->manage_bookmarks ();
  }

  END_PROTECTED
}

void
MainWindow::cm_bookmark_view ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->bookmark_current_view ();
  }

  END_PROTECTED
}

void
MainWindow::update_content ()
{
  BEGIN_PROTECTED

  mp_setup_form->setup ();
  if (current_view ()) {
    current_view ()->update_content ();
  }

  END_PROTECTED
}

void
MainWindow::cm_zoom_fit_sel ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->zoom_fit_sel ();
  }

  END_PROTECTED
}

void
MainWindow::cm_zoom_fit ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->zoom_fit ();
  }

  END_PROTECTED
}

void
MainWindow::cm_pan_left ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->pan_left ();
  }

  END_PROTECTED
}

void
MainWindow::cm_pan_right ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->pan_right ();
  }

  END_PROTECTED
}

void
MainWindow::cm_pan_up ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->pan_up ();
  }

  END_PROTECTED
}

void
MainWindow::cm_pan_down ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->pan_down ();
  }

  END_PROTECTED
}

void
MainWindow::cm_zoom_in ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->zoom_in ();
  }

  END_PROTECTED
}

void
MainWindow::cm_zoom_out ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->zoom_out ();
  }

  END_PROTECTED
}

void
MainWindow::update_action_states ()
{
  try {

    if (mp_menu->is_valid ("edit_menu.undo")) {

      Action undo_action = mp_menu->action ("edit_menu.undo");

      std::string undo_txt (tl::to_string (QObject::tr ("&Undo")));
      bool undo_enable = false;
      if (current_view () && m_manager.available_undo ().first) {
        undo_txt += " - " + m_manager.available_undo ().second;
        undo_enable = true;
      }
      undo_action.set_title (undo_txt);
      undo_action.set_enabled (undo_enable && edits_enabled ());

    }

    if (mp_menu->is_valid ("edit_menu.redo")) {

      Action redo_action = mp_menu->action ("edit_menu.redo");

      std::string redo_txt (tl::to_string (QObject::tr ("&Redo")));
      bool redo_enable = false;
      if (current_view () && m_manager.available_redo ().first) {
        redo_txt += " - " + m_manager.available_redo ().second;
        redo_enable = true;
      }
      redo_action.set_title (redo_txt);
      redo_action.set_enabled (redo_enable && edits_enabled ());

    }

    if (mp_menu->is_valid ("edit_menu.paste")) {
      Action paste_action = mp_menu->action ("edit_menu.paste");
      paste_action.set_enabled (! db::Clipboard::instance ().empty () && edits_enabled ());
    }

    if (mp_menu->is_valid ("zoom_menu.next_display_state")) {
      Action next_display_state_action = mp_menu->action ("zoom_menu.next_display_state");
      next_display_state_action.set_enabled (has_next_display_state ());
    }

    if (mp_menu->is_valid ("zoom_menu.prev_display_state")) {
      Action prev_display_state_action = mp_menu->action ("zoom_menu.prev_display_state");
      prev_display_state_action.set_enabled (has_prev_display_state ());
    }

  } catch (...) {
    //  ignore exceptions
  }
}

void
MainWindow::cm_redraw ()
{
  BEGIN_PROTECTED
  redraw ();
  END_PROTECTED
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
  BEGIN_PROTECTED
  cancel ();
  END_PROTECTED
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

  for (std::vector <lay::LayoutView *>::iterator vp = mp_views.begin (); vp != mp_views.end (); ++vp) {
    (*vp)->cancel ();
  }

  select_mode (lay::LayoutView::default_mode ());
}

void
MainWindow::cm_save_layer_props ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    std::string fn;
    if (mp_lprops_fdia->get_save (fn, tl::to_string (QObject::tr ("Save Layer Properties File")))) {
      current_view ()->save_layer_props (fn);
    }
  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open to save the layer properties from")));
  }

  END_PROTECTED
}

void
MainWindow::load_layer_properties (const std::string &fn, bool all_views, bool add_default)
{
  if (all_views) {
    for (std::vector <lay::LayoutView *>::iterator vp = mp_views.begin (); vp != mp_views.end (); ++vp) {
      (*vp)->load_layer_props (fn, add_default);
    }
  } else if (current_view ()) {
    current_view ()->load_layer_props (fn, add_default);
  }
}

void
MainWindow::load_layer_properties (const std::string &fn, int cv_index, bool all_views, bool add_default)
{
  if (all_views) {
    for (std::vector <lay::LayoutView *>::iterator vp = mp_views.begin (); vp != mp_views.end (); ++vp) {
      (*vp)->load_layer_props (fn, cv_index, add_default);
    }
  } else if (current_view ()) {
    current_view ()->load_layer_props (fn, cv_index, add_default);
  }
}

bool
MainWindow::is_single_cv_layer_properties_file (const std::string &fn)
{
  //  If the file contains information for a single layout but we have multiple ones,
  //  show the dialog to determine what layout to apply the information to.
  std::vector<lay::LayerPropertiesList> props;
  try {
    tl::XMLFileSource in (fn);
    props.push_back (lay::LayerPropertiesList ());
    props.back ().load (in);
  } catch (...) {
    props.clear ();
    tl::XMLFileSource in (fn);
    lay::LayerPropertiesList::load (in, props);
  }

  //  Collect all cv indices in the layer properties
  std::set <int> cv;
  for (std::vector<lay::LayerPropertiesList>::const_iterator p = props.begin (); p != props.end (); ++p) {
    for (lay::LayerPropertiesConstIterator lp = p->begin_const_recursive (); ! lp.at_end (); ++lp) {
      if (! lp->has_children ()) {
        cv.insert (lp->source (true).cv_index ());
        if (cv.size () >= 2) {
          break;
        }
      }
    }
  }

  return (cv.size () == 1);
}

void
MainWindow::cm_load_layer_props ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    std::string fn;
    if (mp_lprops_fdia->get_open (fn, tl::to_string (QObject::tr ("Load Layer Properties File")))) {

      int target_cv_index = -2;

      if (current_view ()->cellviews () > 1 && is_single_cv_layer_properties_file (fn)) {

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
  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open to load the layer properties for")));
  }

  END_PROTECTED
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
  session.restore (*this);
  read_dock_widget_state ();
}

void
MainWindow::cm_save_session ()
{
  BEGIN_PROTECTED

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
    }

  }

  END_PROTECTED
}

void
MainWindow::cm_restore_session ()
{
  BEGIN_PROTECTED

  std::string fn = m_current_session;
  if (mp_session_fdia->get_open (fn, tl::to_string (QObject::tr ("Load Session File")))) {

    std::string df_list;
    int dirty_layouts = dirty_files (df_list);

    if (dirty_layouts == 0) {
      restore_session (fn);
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
      }

    }

  }

  END_PROTECTED
}

void
MainWindow::cm_save_bookmarks ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    std::string fn;
    if (mp_bookmarks_fdia->get_save (fn, tl::to_string (QObject::tr ("Save Bookmarks File")))) {
      current_view ()->bookmarks ().save (fn);
    }
  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open to save the bookmarks from")));
  }

  END_PROTECTED
}

void
MainWindow::cm_load_bookmarks ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    std::string fn;
    if (mp_bookmarks_fdia->get_open (fn, tl::to_string (QObject::tr ("Load Bookmarks File")))) {
      BookmarkList bookmarks;
      bookmarks.load (fn);
      current_view ()->bookmarks (bookmarks);
    }
  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open to load the bookmarks for")));
  }

  END_PROTECTED
}

void
MainWindow::cm_select_current_cell ()
{
  BEGIN_PROTECTED

  lay::LayoutView *view = current_view ();
  if (view && view->active_cellview_index () >= 0) {
    lay::LayoutView::cell_path_type path;
    int cvi = view->active_cellview_index ();
    view->current_cell_path (path);
    view->select_cell_fit (path, cvi);
  }

  END_PROTECTED
}

void
MainWindow::cm_open_current_cell ()
{
  BEGIN_PROTECTED

  if (current_view () && current_view ()->active_cellview_index () >= 0) {
    current_view ()->cm_open_current_cell ();
  }

  END_PROTECTED
}

void
MainWindow::cm_select_cell ()
{
  BEGIN_PROTECTED

  if (current_view () && current_view ()->active_cellview_index () >= 0) {

    CellSelectionForm form (0, current_view (), "cell_selection_form");

    if (form.exec () == QDialog::Accepted &&
        form.selected_cellview_index () >= 0) {
      current_view ()->select_cell (form.selected_cellview ().combined_unspecific_path (), form.selected_cellview_index ());
      current_view ()->set_current_cell_path (form.selected_cellview_index (), form.selected_cellview ().combined_unspecific_path ());
      current_view ()->zoom_fit ();
    }

  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open to select a cell for")));
  }

  END_PROTECTED
}

void
MainWindow::cm_screenshot ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    std::string fn;
    if (mp_screenshot_fdia->get_save (fn, tl::to_string (QObject::tr ("Save Screenshot")))) {
      current_view ()->save_screenshot (fn);
    }
  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open to create a screenshot from")));
  }

  END_PROTECTED
}

void
MainWindow::cm_save_current_cell_as ()
{
  BEGIN_PROTECTED

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

            cv->save_as (fn, om, options, false /*don't update*/);

            add_mru (fn, cv->tech_name ());

          }

        }

      }

    }

  }

  END_PROTECTED
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
  BEGIN_PROTECTED

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
          }

          options.set_dbu (cv->layout ().dbu ());
          options.set_format_from_filename (fn);

          cv->update_save_options (options);

          tl::OutputStream::OutputStreamMode om = tl::OutputStream::OM_Auto;

          if (as && ! mp_layout_save_as_options->get_options (current_view (), cv_index, fn, om, options)) {
            break;
          }

          current_view ()->save_as ((unsigned int) cv_index, fn, om, options, true);
          add_mru (fn, current_view ()->cellview (cv_index)->tech_name ());

        }

      }

    }

  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open to save")));
  }

  END_PROTECTED
}

void
MainWindow::cm_save_all ()
{
  BEGIN_PROTECTED

    for (int view_index = 0; view_index < int (views ()); ++view_index) {

      for (unsigned int cv_index = 0; cv_index < view (view_index)->cellviews (); ++cv_index) {

        const lay::CellView &cv = view (view_index)->cellview (cv_index);
        std::string fn = cv->filename ();

        if (! fn.empty () || mp_layout_fdia->get_save (fn, tl::to_string (tr ("Save Layout '%1'").arg (tl::to_qstring (cv->name ()))))) {

          db::SaveLayoutOptions options (cv->save_options ());
          options.set_dbu (cv->layout ().dbu ());

          options.set_format_from_filename (fn);

          tl::OutputStream::OutputStreamMode om = tl::OutputStream::OM_Auto;

          //  initialize the specific options from the configuration if required
          for (tl::Registrar<lay::PluginDeclaration>::iterator cls = tl::Registrar<lay::PluginDeclaration>::begin (); cls != tl::Registrar<lay::PluginDeclaration>::end (); ++cls) {
            const StreamWriterPluginDeclaration *decl = dynamic_cast <const StreamWriterPluginDeclaration *> (&*cls);
            if (decl) {
              options.set_options (decl->create_specific_options ());
            }
          }

          view (view_index)->save_as (cv_index, fn, om, options, true);
          add_mru (fn, current_view ()->cellview (cv_index)->tech_name ());

        }

      }

    }

  END_PROTECTED
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

    tl_assert (index >= 0 && index < int (views ()));

    mp_tab_bar->setCurrentIndex (index);

    bool box_set = (m_synchronized_views && current_view () != 0);
    db::DBox box;
    if (box_set) {
      box = current_view ()->viewport ().box ();
    }

    view (index)->set_current ();

    mp_layer_toolbox->set_view (current_view ());

    if (current_view ()) {

      if (box_set) {
        current_view ()->zoom_box (box);
      }

      mp_view_stack->raiseWidget (index);
      mp_hp_stack->raiseWidget (index);
      mp_lp_stack->raiseWidget (index);
      mp_libs_stack->raiseWidget (index);
      mp_bm_stack->raiseWidget (index);
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
  BEGIN_PROTECTED

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

  END_PROTECTED
}

void
MainWindow::cm_reader_options ()
{
  mp_layout_load_options->edit_global_options (plugin_root (), db::Technologies::instance ());
}

void
MainWindow::cm_writer_options ()
{
  mp_layout_save_options->edit_global_options (plugin_root (), db::Technologies::instance ());
}

void
MainWindow::cm_new_panel ()
{
  create_view ();
}

void
MainWindow::cm_new_layout ()
{
  BEGIN_PROTECTED

  std::string technology = m_initial_technology;

  double dbu = 0.0;

  lay::NewLayoutPropertiesDialog dialog (this);
  if (dialog.exec_dialog (technology, m_new_cell_cell_name, dbu, m_new_cell_window_size, m_new_layout_current_panel)) {

    lay::CellViewRef cellview = create_or_load_layout (0, 0, technology, m_new_layout_current_panel ? 2 : 1 /*= new view*/);

    if (dbu > 1e-10) {
      cellview->layout ().dbu (dbu);
    }
    db::cell_index_type new_ci = cellview->layout ().add_cell (m_new_cell_cell_name.empty () ? 0 : m_new_cell_cell_name.c_str ());
    cellview.set_cell (new_ci);

    current_view ()->zoom_box_and_set_hier_levels (db::DBox (-0.5 * m_new_cell_window_size, -0.5 * m_new_cell_window_size, 0.5 * m_new_cell_window_size, 0.5 * m_new_cell_window_size), std::make_pair (0, 1));

  }

  END_PROTECTED
}

void
MainWindow::call_on_current_view (void (lay::LayoutView::*func) (), const std::string &op_desc)
{
  BEGIN_PROTECTED

  lay::LayoutView *curr = current_view ();
  if (! curr || curr->active_cellview_index () < 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open for operation: ")) + op_desc);
  }

  (curr->*func) ();

  END_PROTECTED
}

void
MainWindow::cm_adjust_origin ()
{
  call_on_current_view (&lay::LayoutView::cm_align_cell_origin, tl::to_string (QObject::tr ("adjust cell origin")));
}

void
MainWindow::cm_new_cell ()
{
  //  TODO: move this function to lay::LayoutView

  BEGIN_PROTECTED

  lay::LayoutView *curr = current_view ();
  if (! curr || curr->active_cellview_index () < 0) {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open to create a new cell inside")));
  }

  NewCellPropertiesDialog cell_prop_dia (this);
  if (cell_prop_dia.exec_dialog (& curr->cellview (curr->active_cellview_index ())->layout (), m_new_cell_cell_name, m_new_cell_window_size)) {

    db::cell_index_type new_ci = curr->new_cell (curr->active_cellview_index (), m_new_cell_cell_name.c_str ());
    curr->select_cell (new_ci, curr->active_cellview_index ());

    db::DBox zoom_box = db::DBox (-0.5 * m_new_cell_window_size, -0.5 * m_new_cell_window_size, 0.5 * m_new_cell_window_size, 0.5 * m_new_cell_window_size);
    if (curr->get_max_hier_levels () < 1 || curr->get_min_hier_levels () > 0) {
      curr->zoom_box_and_set_hier_levels (zoom_box, std::make_pair (0, 1));
    } else {
      curr->zoom_box (zoom_box);
    }

  }

  END_PROTECTED
}

void
MainWindow::cm_cell_convert_to_static ()
{
  call_on_current_view (&lay::LayoutView::cm_cell_convert_to_static, tl::to_string (QObject::tr ("convert cell to static")));
}

void
MainWindow::cm_lay_convert_to_static ()
{
  call_on_current_view (&lay::LayoutView::cm_lay_convert_to_static, tl::to_string (QObject::tr ("convert all cells to static")));
}

void
MainWindow::cm_lay_move ()
{
  call_on_current_view (&lay::LayoutView::cm_lay_move, tl::to_string (QObject::tr ("move layout")));
}

void
MainWindow::cm_lay_scale ()
{
  call_on_current_view (&lay::LayoutView::cm_lay_scale, tl::to_string (QObject::tr ("scale layout")));
}

void
MainWindow::cm_lay_free_rot ()
{
  call_on_current_view (&lay::LayoutView::cm_lay_free_rot, tl::to_string (QObject::tr ("free rotation of layout")));
}

void
MainWindow::cm_lay_rot_ccw ()
{
  call_on_current_view (&lay::LayoutView::cm_lay_rot_ccw, tl::to_string (QObject::tr ("counter clockwise rotation of layout")));
}

void
MainWindow::cm_lay_rot_cw ()
{
  call_on_current_view (&lay::LayoutView::cm_lay_rot_cw, tl::to_string (QObject::tr ("clockwise rotation of layout")));
}

void
MainWindow::cm_lay_flip_y ()
{
  call_on_current_view (&lay::LayoutView::cm_lay_flip_y, tl::to_string (QObject::tr ("vertical flip of layout")));
}

void
MainWindow::cm_lay_flip_x ()
{
  call_on_current_view (&lay::LayoutView::cm_lay_flip_x, tl::to_string (QObject::tr ("horizontal flip of layout")));
}

void
MainWindow::cm_sel_move ()
{
  call_on_current_view (&lay::LayoutView::cm_sel_move, tl::to_string (QObject::tr ("move selection")));
}

void
MainWindow::cm_sel_move_to ()
{
  call_on_current_view (&lay::LayoutView::cm_sel_move_to, tl::to_string (QObject::tr ("move selection to position")));
}

void
MainWindow::cm_sel_move_interactive ()
{
  call_on_current_view (&lay::LayoutView::cm_sel_move_interactive, tl::to_string (QObject::tr ("move selection interactively")));
}

void
MainWindow::cm_sel_scale ()
{
  call_on_current_view (&lay::LayoutView::cm_sel_scale, tl::to_string (QObject::tr ("scale selection")));
}

void
MainWindow::cm_sel_free_rot ()
{
  call_on_current_view (&lay::LayoutView::cm_sel_free_rot, tl::to_string (QObject::tr ("free rotation of selection")));
}

void
MainWindow::cm_sel_rot_ccw ()
{
  call_on_current_view (&lay::LayoutView::cm_sel_rot_ccw, tl::to_string (QObject::tr ("counter clockwise rotation of selection")));
}

void
MainWindow::cm_sel_rot_cw ()
{
  call_on_current_view (&lay::LayoutView::cm_sel_rot_cw, tl::to_string (QObject::tr ("clockwise rotation of selection")));
}

void
MainWindow::cm_sel_flip_y ()
{
  call_on_current_view (&lay::LayoutView::cm_sel_flip_y, tl::to_string (QObject::tr ("vertical flip of selection")));
}

void
MainWindow::cm_sel_flip_x ()
{
  call_on_current_view (&lay::LayoutView::cm_sel_flip_x, tl::to_string (QObject::tr ("horizontal flip of selection")));
}

void
MainWindow::cm_edit_layer ()
{
  call_on_current_view (&lay::LayoutView::cm_edit_layer, tl::to_string (QObject::tr ("edit a layer specification")));
}

void
MainWindow::cm_delete_layer ()
{
  call_on_current_view (&lay::LayoutView::cm_delete_layer, tl::to_string (QObject::tr ("delete a layer")));
}

void
MainWindow::cm_clear_layer ()
{
  call_on_current_view (&lay::LayoutView::cm_clear_layer, tl::to_string (QObject::tr ("clear a layer")));
}

void
MainWindow::cm_copy_layer ()
{
  call_on_current_view (&lay::LayoutView::cm_copy_layer, tl::to_string (QObject::tr ("copy layer")));
}

void
MainWindow::cm_new_layer ()
{
  call_on_current_view (&lay::LayoutView::cm_new_layer, tl::to_string (QObject::tr ("create a new layer")));
}

void
MainWindow::cm_layout_props ()
{
  BEGIN_PROTECTED

  lay::LayoutView *curr = current_view ();
  if (! curr) {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open to show layout properties for")));
  }

  LayoutPropertiesForm lp_form (this, curr, "layout_props_form");
  lp_form.exec ();

  END_PROTECTED
}

void
MainWindow::cm_layout_stats ()
{
  BEGIN_PROTECTED

  lay::LayoutView *curr = current_view ();
  if (! curr) {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open to show layout statistics for")));
  }

  LayoutStatisticsForm lp_form (this, curr, "layout_props_form");
  lp_form.exec ();

  END_PROTECTED
}

void
MainWindow::cm_clone ()
{
  BEGIN_PROTECTED
  clone_current_view ();
  END_PROTECTED
}

void
MainWindow::clone_current_view ()
{
  lay::LayoutView *view = 0;
  lay::LayoutView *curr = current_view ();
  if (! curr) {
    throw tl::Exception (tl::to_string (QObject::tr ("No view open to clone")));
  }

  //  create a new view
  view = new lay::LayoutView (current_view (), &m_manager, lay::ApplicationBase::instance ()->is_editable (), plugin_root (), mp_view_stack);
  add_view (view);

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

  mp_views.back ()->set_current ();

  mp_layer_toolbox->set_view (current_view ());

  mp_view_stack->addWidget (view);
  mp_lp_stack->addWidget (view->layer_control_frame ());
  mp_hp_stack->addWidget (view->hierarchy_control_frame ());
  mp_libs_stack->addWidget (view->libraries_frame ());
  mp_bm_stack->addWidget (view->bookmarks_frame ());

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
  interactive_close_view (-1, false);
}

void
MainWindow::cm_close ()
{
  interactive_close_view (index_of (lay::LayoutView::current ()), false);
}

void
MainWindow::tab_close_requested (int index)
{
  interactive_close_view (index, true);
}

void
MainWindow::interactive_close_view (int index, bool all_cellviews)
{
  if (index < 0) {

    //  close all views

    bool can_close = true;

    int dirty_layouts = 0;
    std::string dirty_files;
    std::set<std::string> seen_names;

    for (index = 0; index < int (views ()); ++index) {

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
      while (views () > 0) {
        close_view (0);
      }
      END_PROTECTED
    }

  } else if (view (index)) {

    std::vector <int> selected;

    if (view (index)->cellviews () > 1) {

      if (all_cellviews) {

        for (int i = 0; i < int (view (index)->cellviews ()); ++i) {
          selected.push_back (i);
        }

      } else {

        SelectCellViewForm form (0, view (index), tl::to_string (QObject::tr ("Select Layouts To Close")));
        form.set_selection (view (index)->active_cellview_index ());

        if (form.exec () != QDialog::Accepted) {
          return;
        }

        selected = form.selected_cellviews ();
        if (selected.empty ()) {
          return;
        }

      }

    } else if (view (index)->cellviews () > 0) {
      selected.push_back (0);
    }

    if (selected.size () > 0) {

      int dirty_layouts = 0;
      std::string dirty_files;

      for (std::vector <int>::const_iterator i = selected.begin (); i != selected.end (); ++i) {

        const lay::CellView &cv = view (index)->cellview (*i);

        if (cv->layout ().is_editable () && cv->is_dirty ()) {

          std::string name = cv->name ();

          int count = 0;

          for (std::vector <lay::LayoutView *>::const_iterator v = mp_views.begin (); v != mp_views.end (); ++v) {
            for (unsigned int cvi = 0; cvi < (*v)->cellviews (); ++cvi) {
              if ((*v)->cellview (cvi)->name () == name) {
                ++count;
              }
            }
          }

          for (std::vector <int>::const_iterator ii = selected.begin (); ii != selected.end (); ++ii) {
            if (view (index)->cellview (*ii)->name () == name) {
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
        if (view (index)->cellviews () == selected.size ()) {
          //  all cellviews selected - simply close
          close_view (index);
        } else {
          std::sort (selected.begin (), selected.end ());
          int offset = 0;
          for (std::vector <int>::const_iterator i = selected.begin (); i != selected.end (); ++i) {
            view (index)->erase_cellview ((unsigned int)(*i - offset));
            ++offset;
          }
        }

        END_PROTECTED

      }

    } else {
      close_view (index);
    }

  }

}

void
MainWindow::close_current_view ()
{
  close_view (index_of (lay::LayoutView::current ()));
}

void
MainWindow::close_view (int index)
{
  if (view (index)) {

    //  this suppresses view_selected events that would otherwise be created
    bool f = m_disable_tab_selected;
    m_disable_tab_selected = true;

    BEGIN_PROTECTED

      db::DBox box;
      if (m_synchronized_views) {
        box = view (index)->viewport ().box ();
      }

      mp_tab_bar->removeTab (index);
      mp_view_stack->removeWidget (index);
      mp_lp_stack->removeWidget (index);
      mp_hp_stack->removeWidget (index);
      mp_libs_stack->removeWidget (index);
      mp_bm_stack->removeWidget (index);

      view_closed_event (int (index));

      delete view (index);
      mp_views.erase (mp_views.begin () + index, mp_views.begin () + index + 1);

      if (index >= int (mp_views.size ())) {
        --index;
      }

      if (index >= 0) {

        select_view (index);

      } else {

        //  last view closed

        mp_layer_toolbox->set_view (0);
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
MainWindow::cm_reload ()
{
  BEGIN_PROTECTED

  if (current_view ()) {

    std::vector <int> selected;

    if (current_view ()->cellviews () > 1) {

      SelectCellViewForm form (0, current_view (), tl::to_string (QObject::tr ("Select Layouts To Reload")));
      form.select_all ();

      if (form.exec () == QDialog::Accepted) {
        selected = form.selected_cellviews ();
      }

    } else if (current_view ()->cellviews () > 0) {
      selected.push_back (0);
    }

    if (selected.size () > 0) {

      int dirty_layouts = 0;
      std::string dirty_files;

      for (std::vector <int>::const_iterator i = selected.begin (); i != selected.end (); ++i) {

        const lay::CellView &cv = current_view ()->cellview (*i);

        if (cv->layout ().is_editable () && cv->is_dirty ()) {
          ++dirty_layouts;
          if (dirty_layouts == max_dirty_files) {
            dirty_files += "\n...";
          } else if (dirty_layouts < max_dirty_files) {
            if (! dirty_files.empty ()) {
              dirty_files += "\n";
            }
            dirty_files += cv->name ();
          }
        }

      }

      bool can_reload = true;
      if (dirty_layouts != 0) {

        QMessageBox mbox (this);
        mbox.setText (tl::to_qstring (tl::to_string (QObject::tr ("The following layouts need saving:\n\n")) + dirty_files + "\n\nPress 'Reload Without Saving' to reload anyhow and discard changes."));
        mbox.setWindowTitle (QObject::tr ("Save Needed"));
        mbox.setIcon (QMessageBox::Warning);
        QAbstractButton *yes_button = mbox.addButton (QObject::tr ("Reload Without Saving"), QMessageBox::YesRole);
        mbox.addButton (QMessageBox::Cancel);

        mbox.exec ();

        can_reload = (mbox.clickedButton() == yes_button);

      }

      if (can_reload) {

        //  Actually reload
        for (std::vector <int>::const_iterator i = selected.begin (); i != selected.end (); ++i) {
          reload_layout (*i);
        }

      }

    }

  }

  END_PROTECTED
}

void
MainWindow::add_mru (const std::string &fn_rel)
{
  add_mru (fn_rel, m_initial_technology);
}

void
MainWindow::add_mru (const std::string &fn_rel, const std::string &tech)
{
  std::vector <std::pair<std::string, std::string> > new_mru (m_mru);
  std::string fn (tl::InputStream::absolute_path (fn_rel));

  for (std::vector<std::pair<std::string, std::string> >::iterator mru = new_mru.begin (); mru != new_mru.end (); ++mru) {
    if (mru->first == fn) {
      new_mru.erase (mru);
      break;
    }
  }

  new_mru.push_back (std::make_pair (fn, tech));

  if (new_mru.size () > 10) {
    new_mru.erase (new_mru.begin ());
  }

  std::string config_str;
  for (std::vector<std::pair<std::string, std::string> >::const_iterator mru = new_mru.begin (); mru != new_mru.end (); ++mru) {
    if (! config_str.empty ()) {
      config_str += " ";
    }
    config_str += tl::to_quoted_string (mru->first);
    if (! mru->second.empty ()) {
      config_str += "@";
      config_str += tl::to_quoted_string (mru->second);
    }
  }

  plugin_root ()->config_set (cfg_mru, config_str);
}

void
MainWindow::do_update_file_menu ()
{
  std::string mru_menu = "file_menu.open_recent_menu";

  if (mp_menu->is_valid (mru_menu)) {

    Action open_recent_action = mp_menu->action (mru_menu);
    open_recent_action.set_enabled (true);

    if (m_mru.size () > 0 && edits_enabled ()) {

      //  rebuild MRU menu
      mp_menu->clear_menu (mru_menu);

      for (std::vector<std::pair<std::string, std::string> >::iterator mru = m_mru.end (); mru != m_mru.begin (); ) {
        --mru;
        unsigned int i = std::distance (m_mru.begin (), mru);
        Action action;
        gtf::action_connect (action.qaction (), SIGNAL (triggered ()), this, SLOT (open_recent ()));
        action.set_title (mru->first);
        action.qaction ()->setData (QVariant (int (i)));
        mp_menu->insert_item (mru_menu + ".end", tl::sprintf ("open_recent_%d", i + 1), action);
      }

    } else {
      open_recent_action.set_enabled (false);
    }

  }
}

void
MainWindow::open_recent ()
{
  BEGIN_PROTECTED

  QAction *action = dynamic_cast <QAction *> (sender ());
  tl_assert (action);
  size_t n = size_t (action->data ().toInt ());
  if (n >= m_mru.size ()) {
    return;
  }

  OpenLayoutModeDialog open_mode_dialog (this);

  if (views () != 0 && ! open_mode_dialog.exec_dialog (m_open_mode)) {
    return;
  }

  if (mp_layout_load_options->show_always () && !mp_layout_load_options->edit_global_options (plugin_root (), db::Technologies::instance ())) {
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

void
MainWindow::open (int mode)
{
  BEGIN_PROTECTED

  static std::vector<std::string> files;
  if (! mp_layout_fdia->get_open (files, std::string (), tl::to_string (QObject::tr ("Open Layout Files")))) {
    return;
  }

  if (mp_layout_load_options->show_always () && !mp_layout_load_options->edit_global_options (plugin_root (), db::Technologies::instance ())) {
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
MainWindow::add_view (lay::LayoutView *view)
{
  connect (view, SIGNAL (title_changed ()), this, SLOT (view_title_changed ()));
  connect (view, SIGNAL (dirty_changed ()), this, SLOT (view_title_changed ()));
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
  lay::LayoutView *view = new lay::LayoutView (&m_manager, lay::ApplicationBase::instance ()->is_editable (), plugin_root (), mp_view_stack);
  add_view (view);

  //  set initial attributes
  view->set_synchronous (synchronous ());

  int tl = 0;
  config_get (cfg_initial_hier_depth, tl);
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
  mp_views.back ()->set_current ();

  mp_layer_toolbox->set_view (current_view ());

  mp_view_stack->addWidget (mp_views.back ());
  mp_lp_stack->addWidget (mp_views.back ()->layer_control_frame ());
  mp_hp_stack->addWidget (mp_views.back ()->hierarchy_control_frame ());
  mp_libs_stack->addWidget (mp_views.back ()->libraries_frame ());
  mp_bm_stack->addWidget (mp_views.back ()->bookmarks_frame ());

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
      config_get (cfg_initial_hier_depth, tl);
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

      mp_views.back ()->set_current ();

      mp_layer_toolbox->set_view (current_view ());

      mp_view_stack->addWidget (mp_views.back ());
      mp_lp_stack->addWidget (mp_views.back ()->layer_control_frame ());
      mp_hp_stack->addWidget (mp_views.back ()->hierarchy_control_frame ());
      mp_libs_stack->addWidget (mp_views.back ()->libraries_frame ());
      mp_bm_stack->addWidget (mp_views.back ()->bookmarks_frame ());

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
MainWindow::view_title_changed ()
{
  int i = index_of (dynamic_cast<const lay::LayoutView *> (sender ()));
  if (i >= 0) {
    update_tab_title (i);
  }

  if (sender () == current_view ()) {
    update_window_title ();
  }
}

void
MainWindow::update_window_title ()
{
  if (current_view ()) {
    std::string sep = " - ";
    if (current_view ()->is_dirty ()) {
      sep += "[+] ";
    }
    setWindowTitle (tl::to_qstring (lay::ApplicationBase::instance ()->version () + sep + current_view ()->title ()));
  } else {
    setWindowTitle (tl::to_qstring (lay::ApplicationBase::instance ()->version ()));
  }
}

void
MainWindow::current_view_changed ()
{
  update_window_title ();
  current_view_changed_event ();
  //  TODO: required?  current_view_changed_event (int (view_index_org));
}

double
MainWindow::grid_micron () const
{
  return m_grid_micron;
}

void
MainWindow::cm_inc_max_hier ()
{
  BEGIN_PROTECTED

  int new_to = get_max_hier_levels () + 1;
  set_hier_levels (std::make_pair (get_min_hier_levels (), new_to));

  END_PROTECTED
}

void
MainWindow::cm_dec_max_hier ()
{
  BEGIN_PROTECTED

  int new_to = get_max_hier_levels () > 0 ? get_max_hier_levels () - 1 : 0;
  set_hier_levels (std::make_pair (std::min (get_min_hier_levels (), new_to), new_to));

  END_PROTECTED
}

void
MainWindow::cm_max_hier ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->max_hier ();
  }

  END_PROTECTED
}

void
MainWindow::cm_max_hier_0 ()
{
  BEGIN_PROTECTED
  set_hier_levels (std::make_pair (std::min (get_min_hier_levels (), 0), 0));
  END_PROTECTED
}

void
MainWindow::cm_max_hier_1 ()
{
  BEGIN_PROTECTED
  set_hier_levels (std::make_pair (std::min (get_min_hier_levels (), 0), 1));
  END_PROTECTED
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
    config_get (cfg_initial_hier_depth, tl);
    return std::make_pair (0, tl);
  }
}

void
MainWindow::cm_prev_display_state ()
{
  BEGIN_PROTECTED

  if (has_prev_display_state ()) {
    current_view ()->prev_display_state ();
  }

  END_PROTECTED
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

void
MainWindow::cm_next_display_state ()
{
  BEGIN_PROTECTED

  if (has_next_display_state ()) {
    current_view ()->next_display_state ();
  }

  END_PROTECTED
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
  for (std::vector <lay::LayoutView *>::iterator vp = mp_views.begin (); vp != mp_views.end (); ++vp) {
    (*vp)->set_synchronous (sync_mode);
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

    if (mp_assistant->isMinimized ()) {
      mp_assistant->showNormal ();
    } else {
      mp_assistant->show ();
    }
    mp_assistant->activateWindow ();
    mp_assistant->raise ();
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

    if (mp_assistant->isMinimized ()) {
      mp_assistant->showNormal ();
    } else {
      mp_assistant->show ();
    }
    mp_assistant->activateWindow ();
    mp_assistant->raise ();
    mp_assistant->search (s);

  }
}

void
MainWindow::cm_show_all_tips ()
{
  plugin_root ()->config_set (cfg_tip_window_hidden, "");
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

lay::Action &
MainWindow::action_for_slot (const char *slot)
{
  std::map<std::string, lay::Action>::iterator a = m_actions_for_slot.find (std::string (slot));
  if (a != m_actions_for_slot.end ()) {
    return a->second;
  } else {
    Action a = Action::create_free_action (this);
    gtf::action_connect (a.qaction (), SIGNAL (triggered ()), this, slot);
    return m_actions_for_slot.insert (std::make_pair (std::string (slot), a)).first->second;
  }
}

lay::Action *
MainWindow::create_config_action (const std::string &title, const std::string &cname, const std::string &cvalue)
{
  lay::ConfigureAction *ca = new lay::ConfigureAction(plugin_root (), title, cname, cvalue);
  m_ca_collection.push_back (ca);
  return ca;
}

lay::Action *
MainWindow::create_config_action (const std::string &cname, const std::string &cvalue)
{
  lay::ConfigureAction *ca = new lay::ConfigureAction(plugin_root (), std::string (), cname, cvalue);
  m_ca_collection.push_back (ca);
  return ca;
}

void
MainWindow::register_config_action (const std::string &name, lay::ConfigureAction *action)
{
  std::map<std::string, std::vector<lay::ConfigureAction *> >::iterator ca = m_configuration_actions.insert (std::make_pair (name, std::vector<lay::ConfigureAction *> ())).first;
  for (std::vector<lay::ConfigureAction *>::iterator a = ca->second.begin (); a != ca->second.end (); ++a) {
    if (*a == action) {
      return; // already registered
    }
  }

  ca->second.push_back (action);
}

void
MainWindow::unregister_config_action (const std::string &name, lay::ConfigureAction *action)
{
  std::map<std::string, std::vector<lay::ConfigureAction *> >::iterator ca = m_configuration_actions.find (name);
  if (ca != m_configuration_actions.end ()) {
    for (std::vector<lay::ConfigureAction *>::iterator a = ca->second.begin (); a != ca->second.end (); ++a) {
      if (*a == action) {
        ca->second.erase (a);
        return;
      }
    }
  }
}

void
MainWindow::menu_activated (const std::string &symbol)
{
  //  TODO: this can be part of the Plugin scheme, but the plugin root has no idea which is the active
  //  view.
  if (current_view ()) {
    current_view ()->menu_activated (symbol);
  } else {
    throw tl::Exception (tl::to_string (QObject::tr ("No view is active")));
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
  mp_menu->build (menuBar (), mp_tool_bar);
  lay::GuiApplication *app = dynamic_cast<lay::GuiApplication *> (qApp);
  if (app) {
    app->force_update_app_menu ();
  }
}

void
MainWindow::cm_cell_user_properties ()
{
  call_on_current_view (&lay::LayoutView::cm_cell_user_properties, tl::to_string (QObject::tr ("edit cell's user properties")));
}

void
MainWindow::cm_cell_delete ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_cell_delete ();
  }

  END_PROTECTED
}

void
MainWindow::cm_cell_replace ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_cell_replace ();
  }

  END_PROTECTED
}

void
MainWindow::cm_cell_rename ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_cell_rename ();
  }

  END_PROTECTED
}

void
MainWindow::cm_cell_flatten ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_cell_flatten ();
  }

  END_PROTECTED
}

void
MainWindow::cm_cell_select ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_cell_select ();
  }

  END_PROTECTED
}

void
MainWindow::cm_cell_hide ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_cell_hide ();
  }

  END_PROTECTED
}

void
MainWindow::cm_cell_show ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_cell_show ();
  }

  END_PROTECTED
}

void
MainWindow::cm_cell_show_all ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_cell_show_all ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_select_all ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_select_all ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_new_tab ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_new_tab ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_rename_tab ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_rename_tab ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_remove_tab ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_remove_tab ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_make_invalid ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_make_invalid ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_make_valid ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_make_valid ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_hide_all ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_hide_all ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_hide ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_hide ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_show_only ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_show_only ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_show_all ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_show_all ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_show ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_show ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_rename ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_rename ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_delete ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_delete ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_insert ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_insert ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_group ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_group ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_ungroup ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_ungroup ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_source ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_source ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_sort_by_name ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_sort_by_name ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_sort_by_ild ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_sort_by_ild ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_sort_by_idl ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_sort_by_idl ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_sort_by_ldi ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_sort_by_ldi ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_sort_by_dli ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_sort_by_dli ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_regroup_by_index ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_regroup_by_index ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_regroup_by_datatype ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_regroup_by_datatype ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_regroup_by_layer ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_regroup_by_layer ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_regroup_flatten ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_regroup_flatten ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_expand_all ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_expand_all ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_add_missing ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_add_missing ();
  }

  END_PROTECTED
}

void
MainWindow::cm_lv_remove_unused ()
{
  BEGIN_PROTECTED

  if (current_view ()) {
    current_view ()->cm_remove_unused ();
  }

  END_PROTECTED
}

void
MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
  if (event->mimeData () && event->mimeData ()->hasUrls () && event->mimeData ()->urls ().size () >= 1) {
    event->acceptProposedAction ();
  }
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

      } else if (suffix == QString::fromUtf8 ("lys")) {

        restore_session (tl::to_string (path));

      } else if (suffix == QString::fromUtf8 ("lyb")) {

        if (current_view ()) {
          BookmarkList bookmarks;
          bookmarks.load (tl::to_string (path));
          current_view ()->bookmarks (bookmarks);
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

  cls->init_menu ();

  //  recreate all plugins
  for (std::vector <lay::LayoutView *>::iterator vp = mp_views.begin (); vp != mp_views.end (); ++vp) {
    (*vp)->create_plugins (plugin_root ());
  }
}

void
MainWindow::plugin_removed (lay::PluginDeclaration *cls)
{
  cls->remove_menu_items ();

  //  recreate all plugins except the one that got removed
  for (std::vector <lay::LayoutView *>::iterator vp = mp_views.begin (); vp != mp_views.end (); ++vp) {
    (*vp)->create_plugins (plugin_root (), cls);
  }
}

// ------------------------------------------------------------
//  Implementation of the PluginRootToMainWindow bride

PluginRootToMainWindow::PluginRootToMainWindow ()
  : mp_main_window (0)
{
  //  .. nothing yet ..
}

void
PluginRootToMainWindow::attach_to (lay::MainWindow *main_window)
{
  mp_main_window = main_window;
}

void
PluginRootToMainWindow::plugin_registered (lay::PluginDeclaration *cls)
{
  if (mp_main_window.get ()) {
    mp_main_window->plugin_registered (cls);
  }

  //  re-establish the configuration
  config_setup ();
}

void
PluginRootToMainWindow::plugin_removed (lay::PluginDeclaration *cls)
{
  if (mp_main_window.get ()) {
    mp_main_window->plugin_removed (cls);
  }

  //  re-establish the configuration
  config_setup ();
}

void
PluginRootToMainWindow::select_mode (int mode)
{
  if (mp_main_window.get ()) {
    mp_main_window->select_mode (mode);
  }
}

void
PluginRootToMainWindow::menu_activated (const std::string &symbol)
{
  if (mp_main_window.get ()) {
    mp_main_window->menu_activated (symbol);
  }
}

// ------------------------------------------------------------
//  Implementation of the "help about" dialog

HelpAboutDialog::HelpAboutDialog (QWidget *parent)
  : QDialog (parent)
{
  mp_ui = new Ui::HelpAboutDialog ();
  mp_ui->setupUi (this);

  std::vector<std::string> build_options;
  if (lay::ApplicationBase::instance ()->ruby_interpreter ().available ()) {
    build_options.push_back (tl::to_string (tr ("Ruby interpreter ")) + lay::ApplicationBase::instance ()->ruby_interpreter ().version ());
  }
  if (lay::ApplicationBase::instance ()->python_interpreter ().available ()) {
    build_options.push_back (tl::to_string (tr ("Python interpreter ")) + lay::ApplicationBase::instance ()->python_interpreter ().version ());
  }
#if defined(HAVE_QTBINDINGS)
  build_options.push_back (tl::to_string (tr ("Qt bindings for scripts")));
#endif
#if defined(HAVE_64BIT_COORD)
  build_options.push_back (tl::to_string (tr ("Wide coordinates (64 bit)")));
#endif

  std::string s;

  s = "<html><body>";

  s += "<h1>";
  s += escape_xml (std::string (lay::Version::name ()) + " " + lay::Version::version ());
  s += "</h1>";

  std::vector<std::string> about_paras = tl::split (lay::Version::about_text (), "\n\n");
  for (std::vector<std::string>::const_iterator p = about_paras.begin (); p != about_paras.end (); ++p) {
    s += std::string ("<p>") + escape_xml (*p) + "</p>";
  }

  if (! build_options.empty ()) {
    s += "<p>";
    s += "<h4>";
    s += escape_xml (tl::to_string (QObject::tr ("Build options:")));
    s += "</h4><ul>";
    for (std::vector<std::string>::const_iterator bo = build_options.begin (); bo != build_options.end (); ++bo) {
      s += "<li>";
      s += escape_xml (*bo);
      s += "</li>";
    }
    s += "</ul>";
  }

  if (! lay::plugins ().empty () || ! db::plugins ().empty ()) {

    s += "<p>";
    s += "<h4>";
    s += escape_xml (tl::to_string (QObject::tr ("Binary extensions:")));
    s += "</h4><ul>";

    for (std::list<lay::PluginDescriptor>::const_iterator pd = lay::plugins ().begin (); pd != lay::plugins ().end (); ++pd) {
      s += "<li>";
      if (! pd->description.empty ()) {
        s += escape_xml (pd->description);
      } else {
        s += escape_xml (pd->path);
      }
      if (! pd->version.empty ()) {
        s += " (" + escape_xml (pd->version) + ")";
      }
      s += "</li>";
    }

    for (std::list<db::PluginDescriptor>::const_iterator pd = db::plugins ().begin (); pd != db::plugins ().end (); ++pd) {
      s += "<li>";
      if (! pd->description.empty ()) {
        s += escape_xml (pd->description);
      } else {
        s += escape_xml (pd->path);
      }
      if (! pd->version.empty ()) {
        s += " (" + escape_xml (pd->version) + ")";
      }
      s += "</li>";
    }

    s += "</ul>";

  }

  s += "</body></html>";

  std::string t = tl::to_string (QObject::tr ("About ")) + lay::Version::name ();

  setWindowTitle (tl::to_qstring (t));

  mp_ui->main->setWordWrap (true);
  mp_ui->main->setText (tl::to_qstring (s));
}

HelpAboutDialog::~HelpAboutDialog ()
{
  delete mp_ui;
  mp_ui = 0;
}

} // namespace lay

