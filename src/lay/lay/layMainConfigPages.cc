
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


#include "layMainConfigPages.h"
#include "laybasicConfig.h"
#include "layConfig.h"
#include "layStream.h"
#include "layAbstractMenu.h"
#include "layMainWindow.h"
#include "ui_MainConfigPage.h"
#include "ui_MainConfigPage2.h"
#include "ui_MainConfigPage3.h"
#include "ui_MainConfigPage4.h"
#include "ui_MainConfigPage5.h"
#include "ui_MainConfigPage6.h"
#include "ui_MainConfigPage7.h"
#include "ui_CustomizeMenuConfigPage.h"

#include <QMessageBox>

#include <map>
#include <algorithm>

// ------------------------------------------------------------
//  Declaration of the configuration options
//  The configuration pages are declared via two "dummy" plugins

namespace lay
{

class MainPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const
  {
    options.push_back (std::pair<std::string, std::string> (cfg_grid, "0.001"));
    options.push_back (std::pair<std::string, std::string> (cfg_circle_points, "32"));
    options.push_back (std::pair<std::string, std::string> (cfg_edit_mode, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_custom_macro_paths, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_synchronized_views, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_default_grids, "0.01,0.005,0.001"));
    options.push_back (std::pair<std::string, std::string> (cfg_mru, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_mru_sessions, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_mru_layer_properties, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_mru_bookmarks, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_technologies, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_show_navigator, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_navigator_all_hier_levels, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_navigator_show_images, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_show_toolbar, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_show_layer_toolbox, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_show_hierarchy_panel, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_show_libraries_view, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_show_bookmarks_view, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_show_layer_panel, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_layout_file_watcher_enabled, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_window_state, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_window_geometry, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_key_bindings, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_menu_items_hidden, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_tip_window_hidden, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_micron_digits, "5"));
    options.push_back (std::pair<std::string, std::string> (cfg_dbu_digits, "2"));
    options.push_back (std::pair<std::string, std::string> (cfg_reader_options_show_always, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_assistant_bookmarks, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_always_exit_without_saving, "false"));
  }

  virtual std::vector<std::pair <std::string, ConfigPage *> > config_pages (QWidget *parent) const 
  {
    std::vector<std::pair <std::string, ConfigPage *> > pages;
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Application|General")), new MainConfigPage7 (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Application|Editing Mode")), new MainConfigPage4 (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Application|Grid")), new MainConfigPage (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Application|Default Grids")), new MainConfigPage3 (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Application|Customize Menu")), new CustomizeMenuConfigPage (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Application|Units")), new MainConfigPage5 (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Application|Circles")), new MainConfigPage6 (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Display|Synchronized Views")), new MainConfigPage2 (parent)));
    return pages;
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new MainPluginDeclaration (), 1000, "MainPlugin");

// -------------------------------------------------------------
//  The "grid" configuration page

MainConfigPage::MainConfigPage (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::MainConfigPage ();
  mp_ui->setupUi (this);
}

MainConfigPage::~MainConfigPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
MainConfigPage::setup (lay::Dispatcher *root)
{
  double grid_micron = 0.0;
  root->config_get (cfg_grid, grid_micron);
  mp_ui->grid_edit->setText (tl::to_qstring (tl::to_string (grid_micron)));
}

void 
MainConfigPage::commit (lay::Dispatcher *root)
{
  try {
    double g;
    tl::from_string (tl::to_string (mp_ui->grid_edit->text ()), g);
    root->config_set (cfg_grid, g);
  } catch (...) { }
}

// -------------------------------------------------------------
//  The "number of circle points" configuration page

MainConfigPage6::MainConfigPage6 (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::MainConfigPage6 ();
  mp_ui->setupUi (this);
}

MainConfigPage6::~MainConfigPage6 ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
MainConfigPage6::setup (lay::Dispatcher *root)
{
  int cp = 32;
  root->config_get (cfg_circle_points, cp);
  mp_ui->circle_points->setText (tl::to_qstring (tl::to_string (cp)));
}

void 
MainConfigPage6::commit (lay::Dispatcher *root)
{
  try {
    int cp = 32;
    tl::from_string (tl::to_string (mp_ui->circle_points->text ()), cp);
    cp = std::max (4, std::min (10000000, cp));
    root->config_set (cfg_circle_points, cp);
  } catch (...) { }
}

// -------------------------------------------------------------
//  The "check files for updates" configuration page

MainConfigPage7::MainConfigPage7 (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::MainConfigPage7 ();
  mp_ui->setupUi (this);
}

MainConfigPage7::~MainConfigPage7 ()
{
  delete mp_ui;
  mp_ui = 0;
}

void
MainConfigPage7::setup (lay::Dispatcher *root)
{
  bool en = true;
  root->config_get (cfg_layout_file_watcher_enabled, en);
  mp_ui->check_for_updates->setChecked (en);

  int kb = 0;
  root->config_get (cfg_keep_backups, kb);
  mp_ui->keep_backups->setValue (kb);
  
  bool ex = false;
  root->config_get (cfg_always_exit_without_saving, ex);
  mp_ui->always_exit_without_saving->setChecked (ex);
}

void
MainConfigPage7::commit (lay::Dispatcher *root)
{
  try {
    root->config_set (cfg_layout_file_watcher_enabled, mp_ui->check_for_updates->isChecked ());
    root->config_set (cfg_keep_backups, mp_ui->keep_backups->value ());
    root->config_set (cfg_always_exit_without_saving, mp_ui->always_exit_without_saving->isChecked ());
  } catch (...) { }
}

// ------------------------------------------------------------
//  The "misc" config page

MainConfigPage2::MainConfigPage2 (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::MainConfigPage2 ();
  mp_ui->setupUi (this);
}

MainConfigPage2::~MainConfigPage2 ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
MainConfigPage2::setup (lay::Dispatcher *root)
{
  bool flag = false;
  root->config_get (cfg_synchronized_views, flag);
  mp_ui->sync_views_cbx->setChecked (flag);
}

void 
MainConfigPage2::commit (lay::Dispatcher *root)
{
  root->config_set (cfg_synchronized_views, mp_ui->sync_views_cbx->isChecked ());
}

// -------------------------------------------------------------
//  The "default grids" configuration page

MainConfigPage3::MainConfigPage3 (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::MainConfigPage3 ();
  mp_ui->setupUi (this);
}

MainConfigPage3::~MainConfigPage3 ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
MainConfigPage3::setup (lay::Dispatcher *root)
{
  std::string default_grids;
  root->config_get (cfg_default_grids, default_grids);
  mp_ui->grids_edit->setText (tl::to_qstring (default_grids));
}

void 
MainConfigPage3::commit (lay::Dispatcher *root)
{
  root->config_set (cfg_default_grids, tl::to_string (mp_ui->grids_edit->text ()));
}

// -------------------------------------------------------------
//  The "editing mode" configuration page

MainConfigPage4::MainConfigPage4 (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::MainConfigPage4 ();
  mp_ui->setupUi (this);
}

MainConfigPage4::~MainConfigPage4 ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
MainConfigPage4::setup (lay::Dispatcher *root)
{
  bool flag = true;
  root->config_get (cfg_edit_mode, flag);
  mp_ui->edit_mode_cbx->setChecked (flag);
}

void 
MainConfigPage4::commit (lay::Dispatcher *root)
{
  root->config_set (cfg_edit_mode, mp_ui->edit_mode_cbx->isChecked ());
}

// -------------------------------------------------------------
//  The "digits" configuration page

MainConfigPage5::MainConfigPage5 (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::MainConfigPage5 ();
  mp_ui->setupUi (this);
}

MainConfigPage5::~MainConfigPage5 ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
MainConfigPage5::setup (lay::Dispatcher *root)
{
  int d;
  d = 5;
  root->config_get (cfg_micron_digits, d);
  mp_ui->micron_digits->setValue (d);
  d = 2;
  root->config_get (cfg_dbu_digits, d);
  mp_ui->dbu_digits->setValue (d);
}

void 
MainConfigPage5::commit (lay::Dispatcher *root)
{
  root->config_set (cfg_micron_digits, mp_ui->micron_digits->value ());
  root->config_set (cfg_dbu_digits, mp_ui->dbu_digits->value ());
}

// ------------------------------------------------------------
//  The "key bindings" config page

CustomizeMenuConfigPage::CustomizeMenuConfigPage (QWidget *parent)
  : lay::ConfigPage (parent), m_enable_event (true), mp_dispatcher (0)
{
  mp_ui = new Ui::CustomizeMenuConfigPage ();
  mp_ui->setupUi (this);
  connect (mp_ui->bindings_list, SIGNAL (currentItemChanged (QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT (current_changed (QTreeWidgetItem *, QTreeWidgetItem *)));
  connect (mp_ui->bindings_list, SIGNAL (itemChanged (QTreeWidgetItem *, int)), this, SLOT (item_changed (QTreeWidgetItem *, int)));
  connect (mp_ui->reset_pb, SIGNAL (clicked ()), this, SLOT (reset_clicked ()));

  mp_ui->binding_le->setEnabled (false);
  mp_ui->binding_le->set_clear_button_enabled (true);
  connect (mp_ui->binding_le, SIGNAL (clear_pressed ()), this, SLOT (text_cleared ()));
  connect (mp_ui->binding_le, SIGNAL (textChanged (QString)), this, SLOT (text_changed ()));

  mp_ui->filter->set_clear_button_enabled (true);
  connect (mp_ui->filter, SIGNAL (textChanged (QString)), this, SLOT (filter_changed ()));
}

CustomizeMenuConfigPage::~CustomizeMenuConfigPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void
CustomizeMenuConfigPage::reset_clicked ()
{
  if (QMessageBox::question (this, 

    QObject::tr ("Confirm Reset"),
    QObject::tr ("Are you sure to reset the key bindings?\nThis operation will clear all custom settings."),
    QMessageBox::Yes | QMessageBox::No,
    QMessageBox::No) == QMessageBox::Yes) { 

    apply (std::vector<std::pair<std::string, std::string> > (), std::vector<std::pair<std::string, bool> > ());

  }
}

void 
CustomizeMenuConfigPage::apply (const std::vector<std::pair<std::string, std::string> > &key_bindings, const std::vector<std::pair<std::string, bool> > &hidden)
{
  //  clears the filter
  mp_ui->filter->blockSignals (true);
  mp_ui->filter->clear ();
  mp_ui->filter->blockSignals (false);

  //  build the path to item table and the alias table
  m_item_for_path.clear ();
  m_paths_for_action.clear ();

  //  get the default bindings
  std::map<std::string, std::string> default_bindings = mp_dispatcher->menu ()->get_shortcuts (true);

  m_enable_event = false;

  //  clear bindings and initialize with the given ones
  if (! key_bindings.empty ()) {

    //  gets the current bindings and merges with the given ones
    m_current_bindings = mp_dispatcher->menu ()->get_shortcuts (false);

    std::map<std::string, std::string> b;
    b.insert (key_bindings.begin (), key_bindings.end ());
    for (std::map<std::string, std::string>::iterator kb = m_current_bindings.begin (); kb != m_current_bindings.end (); ++kb) {
      std::map<std::string, std::string>::iterator bb = b.find (kb->first);
      if (bb != b.end ()) {
        lay::Action *a = mp_dispatcher->menu ()->action (kb->first);
        kb->second = a->get_effective_shortcut_for (bb->second);
      } else {
        kb->second.clear ();
      }
    }

  } else {

    //  an empty list is a request for reset
    m_current_bindings = default_bindings;

  }

  //  clear hidden flags and initialize with the given ones
  std::map<std::string, bool> h;
  h.insert (hidden.begin (), hidden.end ());
  m_hidden_flags.clear ();
  for (std::map<std::string, std::string>::iterator kb = m_current_bindings.begin (); kb != m_current_bindings.end (); ++kb) {
    std::map<std::string, bool>::iterator hh = h.find (kb->first);
    m_hidden_flags.insert (std::make_pair (kb->first, hh != h.end () && hh->second));
  }

  //  extract the top level menus
  std::map <std::string, std::string> top_level_menus;
  top_level_menus.insert (std::make_pair (std::string (), tl::to_string (QObject::tr ("Main Menu"))));
  top_level_menus.insert (std::make_pair (std::string ("secrets"), tl::to_string (QObject::tr ("Key Binding Targets"))));
  top_level_menus.insert (std::make_pair (std::string ("lcp_context_menu"), tl::to_string (QObject::tr ("Layer Panel Context Menu"))));
  top_level_menus.insert (std::make_pair (std::string ("hcp_context_menu"), tl::to_string (QObject::tr ("Cell List Context Menu"))));

  //  fill the bindings list
  mp_ui->bindings_list->clear ();

  for (std::map <std::string, std::string>::const_iterator t = top_level_menus.begin (); t != top_level_menus.end (); ++t) {

    QTreeWidgetItem *top_level_item = new QTreeWidgetItem (mp_ui->bindings_list);
    top_level_item->setData (0, Qt::DisplayRole, tl::to_qstring (t->second));

    for (std::map<std::string, std::string>::const_iterator cb = m_current_bindings.begin (); cb != m_current_bindings.end (); ++cb) {

      bool hidden = m_hidden_flags[cb->first];

      std::map<std::string, std::string>::const_iterator db = default_bindings.find (cb->first);

      std::string sc = cb->second;
      bool is_default = (db != default_bindings.end () && db->second == sc);

      const std::string &path = cb->first;

      std::string tl_menu;
      std::string rem_path = path;
      if (path.find ("@") == 0) {
        size_t n = path.find (".");
        if (n != std::string::npos) {
          tl_menu = std::string (path, 1, n - 1);
          rem_path = std::string (path, n + 1);
        }
      }

      if (t->first == tl_menu) {
        QTreeWidgetItem *item = new QTreeWidgetItem (top_level_item);
        lay::Action *action = mp_dispatcher->menu ()->action (cb->first);
        item->setData (0, Qt::ToolTipRole, tl::to_qstring (rem_path));
        item->setData (0, Qt::DisplayRole, tl::to_qstring (rem_path));
        item->setData (1, Qt::ToolTipRole, tl::to_qstring (action->get_title ()));
        item->setData (1, Qt::DisplayRole, tl::to_qstring (action->get_title ()));
        item->setData (2, Qt::DisplayRole, tl::to_qstring (sc));
        item->setData (2, Qt::ForegroundRole, palette ().color (is_default ? QPalette::Disabled : QPalette::Normal, QPalette::Text));
        item->setData (0, Qt::UserRole, tl::to_qstring (path));
        item->setFlags (item->flags () | Qt::ItemIsUserCheckable);
        item->setCheckState (0, hidden ? Qt::Unchecked : Qt::Checked);
        item->setHidden (false);
        m_item_for_path[path] = item;
        if (action->qaction ()) {
          m_paths_for_action[action->qaction ()].push_back (path);
        }
      }
    }

    mp_ui->bindings_list->expandItem (top_level_item);

  }

  mp_ui->binding_le->setText (QString ());
#if QT_VERSION >= 0x40700
  mp_ui->binding_le->setPlaceholderText (QString ());
#endif
  mp_ui->binding_le->setEnabled (false);

  m_enable_event = true;
}

void 
CustomizeMenuConfigPage::setup (lay::Dispatcher *dispatcher)
{
  mp_dispatcher = dispatcher;

  std::string packed_key_bindings;
  dispatcher->config_get (cfg_key_bindings, packed_key_bindings);
  std::vector<std::pair<std::string, std::string> > key_bindings = unpack_key_binding (packed_key_bindings);

  std::string packed_menu_items_hidden;
  dispatcher->config_get (cfg_menu_items_hidden, packed_menu_items_hidden);
  std::vector<std::pair<std::string, bool> > menu_items_hidden = unpack_menu_items_hidden (packed_menu_items_hidden);

  apply (key_bindings, menu_items_hidden);
}

void 
CustomizeMenuConfigPage::commit (lay::Dispatcher *dispatcher)
{
  current_changed (0, mp_ui->bindings_list->currentItem ());

  //  Because the available menu items change in edit and viewer mode, we always extend the key bindings/hidden flags
  //  but never reduce them.

  std::string packed_key_bindings;
  dispatcher->config_get (cfg_key_bindings, packed_key_bindings);
  std::vector<std::pair<std::string, std::string> > key_bindings = unpack_key_binding (packed_key_bindings);

  for (std::vector<std::pair<std::string, std::string> >::iterator kb = key_bindings.begin (); kb != key_bindings.end (); ++kb) {
    std::map<std::string, std::string>::iterator cb = m_current_bindings.find (kb->first);
    if (cb != m_current_bindings.end ()) {
      lay::Action *a = dispatcher->menu ()->action (kb->first);
      if (a && cb->second != a->get_default_shortcut ()) {
        if (cb->second.empty ()) {
          kb->second = lay::Action::no_shortcut ();
        } else {
          kb->second = cb->second;
        }
      } else {
        kb->second.clear ();
      }
      m_current_bindings.erase (cb);
    }
  }

  for (std::map<std::string, std::string>::const_iterator cb = m_current_bindings.begin (); cb != m_current_bindings.end (); ++cb) {
    key_bindings.push_back (*cb);
  }

  packed_key_bindings = pack_key_binding (key_bindings);
  dispatcher->config_set (cfg_key_bindings, packed_key_bindings);

  std::string packed_hidden_flags;
  dispatcher->config_get (cfg_menu_items_hidden, packed_hidden_flags);
  std::vector<std::pair<std::string, bool> > hidden = unpack_menu_items_hidden (packed_hidden_flags);

  for (std::vector<std::pair<std::string, bool> >::iterator hf = hidden.begin (); hf != hidden.end (); ++hf) {
    std::map<std::string, bool>::iterator h = m_hidden_flags.find (hf->first);
    if (h != m_hidden_flags.end ()) {
      hf->second = h->second;
      m_hidden_flags.erase (h);
    }
  }

  for (std::map<std::string, bool>::const_iterator hf = m_hidden_flags.begin (); hf != m_hidden_flags.end (); ++hf) {
    hidden.push_back (*hf);
  }

  packed_hidden_flags = pack_menu_items_hidden (hidden);
  dispatcher->config_set (cfg_menu_items_hidden, packed_hidden_flags);
}

void
CustomizeMenuConfigPage::text_cleared ()
{
  QTreeWidgetItem *item = mp_ui->bindings_list->currentItem ();
  if (! item) {
    return;
  }

  std::string path = tl::to_string (item->data (0, Qt::UserRole).toString ());
  lay::Action *a = mp_dispatcher->menu ()->action (path);

  //  "clear" reverts to default
  mp_ui->binding_le->setText (tl::to_qstring (a->get_default_shortcut ()));
}

void
CustomizeMenuConfigPage::text_changed ()
{
  if (m_enable_event) {
    update_list_item (mp_ui->bindings_list->currentItem ());
  }
}

void
CustomizeMenuConfigPage::filter_changed ()
{
  mp_ui->bindings_list->clearSelection ();
  current_changed (0, mp_ui->bindings_list->currentItem ());

  QString filter = mp_ui->filter->text ();

  for (int i = 0; i < mp_ui->bindings_list->topLevelItemCount (); ++i) {
    QTreeWidgetItem *tl_item = mp_ui->bindings_list->topLevelItem (i);
    bool any = false;
    for (int j = 0; j < tl_item->childCount (); ++j) {
      QTreeWidgetItem *item = tl_item->child (j);
      QString path = item->data (0, Qt::DisplayRole).toString ();
      QString title = item->data (1, Qt::DisplayRole).toString ();
      bool matches = path.indexOf (filter, 0, Qt::CaseInsensitive) >= 0 || title.indexOf (filter, 0, Qt::CaseInsensitive) >= 0;
      item->setHidden (!matches);
      if (matches) {
        any = true;
      }
    }
    tl_item->setHidden (!any);
  }
}

void
CustomizeMenuConfigPage::update_list_item (QTreeWidgetItem *item)
{
  if (! item || ! mp_ui->binding_le->isEnabled ()) {
    return;
  }

  std::string path = tl::to_string (item->data (0, Qt::UserRole).toString ());
  std::string shortcut = tl::to_string (mp_ui->binding_le->text ().simplified ());
  //  normalize string
  shortcut = tl::to_string (QKeySequence (tl::to_qstring (shortcut)).toString ());
  m_current_bindings[path] = shortcut;

  bool is_default = false;

  lay::Action *a = mp_dispatcher->menu ()->action (path);
  std::string def_shortcut = a->get_default_shortcut ();

  is_default = (def_shortcut == shortcut);

  item->setData (2, Qt::DisplayRole, tl::to_qstring (shortcut));
  item->setData (2, Qt::ForegroundRole, palette ().color (is_default ? QPalette::Disabled : QPalette::Normal, QPalette::Text));

  //  Set the aliases too
  const lay::AbstractMenu &menu = *mp_dispatcher->menu ();
  if (menu.is_valid (path)) {
    QAction *qaction = menu.action (path)->qaction ();
    std::map<QAction *, std::vector<std::string> >::const_iterator a = m_paths_for_action.find (qaction);
    if (a != m_paths_for_action.end ()) {
      for (std::vector<std::string>::const_iterator p = a->second.begin (); p != a->second.end (); ++p) {
        m_current_bindings[*p] = shortcut;
        std::map<std::string, QTreeWidgetItem *>::const_iterator i = m_item_for_path.find (*p);
        if (i != m_item_for_path.end ()) {
          i->second->setData (2, Qt::DisplayRole, tl::to_qstring (shortcut));
          i->second->setData (2, Qt::ForegroundRole, palette ().color (is_default ? QPalette::Disabled : QPalette::Normal, QPalette::Text));
        }
      }
    }
  }
}

void
CustomizeMenuConfigPage::item_changed (QTreeWidgetItem *current, int)
{
  if (! m_enable_event) {
    return;
  }

  //  handles check state changes
  if (current && !current->data (0, Qt::UserRole).isNull ()) {
    std::string path = tl::to_string (current->data (0, Qt::UserRole).toString ());
    m_hidden_flags[path] = (current->checkState (0) != Qt::Checked);
  }
}

void 
CustomizeMenuConfigPage::current_changed (QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
  if (! m_enable_event) {
    return;
  }

  m_enable_event = false;

  update_list_item (previous);

  if (current && !current->data (0, Qt::UserRole).isNull ()) {

    std::string path = tl::to_string (current->data (0, Qt::UserRole).toString ());
    if (mp_dispatcher->menu ()->is_menu (path)) {

      mp_ui->binding_le->setText (QString ());
#if QT_VERSION >= 0x40700
      mp_ui->binding_le->setPlaceholderText (QString ());
#endif
      mp_ui->binding_le->setEnabled (false);

    } else {

      std::string shortcut = m_current_bindings[path];

      lay::Action *a = mp_dispatcher->menu ()->action (path);

      std::string def_shortcut = a->get_default_shortcut ();

      mp_ui->binding_le->setText (tl::to_qstring (shortcut));
#if QT_VERSION >= 0x40700
      mp_ui->binding_le->setPlaceholderText (tl::to_qstring (def_shortcut));
#endif
      mp_ui->binding_le->setEnabled (true);

    }

  } else {

    mp_ui->binding_le->setText (QString ());
#if QT_VERSION >= 0x40700
    mp_ui->binding_le->setPlaceholderText (QString ());
#endif
    mp_ui->binding_le->setEnabled (false);

  }

  m_enable_event = true;
}

}

