
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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
#include "ui_KeyBindingsConfigPage.h"

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
    options.push_back (std::pair<std::string, std::string> (cfg_technologies, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_show_navigator, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_navigator_all_hier_levels, "false"));
    options.push_back (std::pair<std::string, std::string> (cfg_navigator_show_images, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_show_toolbar, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_show_layer_toolbox, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_show_hierarchy_panel, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_show_layer_panel, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_layout_file_watcher_enabled, "true"));
    options.push_back (std::pair<std::string, std::string> (cfg_window_state, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_window_geometry, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_key_bindings, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_tip_window_hidden, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_micron_digits, "5"));
    options.push_back (std::pair<std::string, std::string> (cfg_dbu_digits, "2"));
    options.push_back (std::pair<std::string, std::string> (cfg_reader_options_show_always, "false"));
  }

  virtual std::vector<std::pair <std::string, ConfigPage *> > config_pages (QWidget *parent) const 
  {
    std::vector<std::pair <std::string, ConfigPage *> > pages;
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Application|General")), new MainConfigPage7 (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Application|Editing Mode")), new MainConfigPage4 (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Application|Grid")), new MainConfigPage (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Application|Default Grids")), new MainConfigPage3 (parent)));
    pages.push_back (std::make_pair (tl::to_string (QObject::tr ("Application|Key Bindings")), new KeyBindingsConfigPage (parent)));
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
MainConfigPage::setup (lay::PluginRoot *root)
{
  double grid_micron = 0.0;
  root->config_get (cfg_grid, grid_micron);
  mp_ui->grid_edit->setText (tl::to_qstring (tl::to_string (grid_micron)));
}

void 
MainConfigPage::commit (lay::PluginRoot *root)
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
MainConfigPage6::setup (lay::PluginRoot *root)
{
  int cp = 32;
  root->config_get (cfg_circle_points, cp);
  mp_ui->circle_points->setText (tl::to_qstring (tl::to_string (cp)));
}

void 
MainConfigPage6::commit (lay::PluginRoot *root)
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
MainConfigPage7::setup (lay::PluginRoot *root)
{
  bool en = true;
  root->config_get (cfg_layout_file_watcher_enabled, en);
  mp_ui->check_for_updates->setChecked (en);
}

void
MainConfigPage7::commit (lay::PluginRoot *root)
{
  try {
    root->config_set (cfg_layout_file_watcher_enabled, mp_ui->check_for_updates->isChecked ());
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
MainConfigPage2::setup (lay::PluginRoot *root)
{
  bool flag = false;
  root->config_get (cfg_synchronized_views, flag);
  mp_ui->sync_views_cbx->setChecked (flag);
}

void 
MainConfigPage2::commit (lay::PluginRoot *root)
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
MainConfigPage3::setup (lay::PluginRoot *root)
{
  std::string default_grids;
  root->config_get (cfg_default_grids, default_grids);
  mp_ui->grids_edit->setText (tl::to_qstring (default_grids));
}

void 
MainConfigPage3::commit (lay::PluginRoot *root)
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
MainConfigPage4::setup (lay::PluginRoot *root)
{
  bool flag = true;
  root->config_get (cfg_edit_mode, flag);
  mp_ui->edit_mode_cbx->setChecked (flag);
}

void 
MainConfigPage4::commit (lay::PluginRoot *root)
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
MainConfigPage5::setup (lay::PluginRoot *root)
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
MainConfigPage5::commit (lay::PluginRoot *root)
{
  root->config_set (cfg_micron_digits, mp_ui->micron_digits->value ());
  root->config_set (cfg_dbu_digits, mp_ui->dbu_digits->value ());
}

// ------------------------------------------------------------
//  The "key bindings" config page

std::vector<std::pair<std::string, std::string> > 
unpack_key_binding (const std::string &packed)
{
  tl::Extractor ex (packed.c_str ());

  std::vector<std::pair<std::string, std::string> > key_bindings;

  while (! ex.at_end ()) {
    ex.test(";");
    key_bindings.push_back (std::make_pair (std::string (), std::string ()));
    ex.read_word_or_quoted (key_bindings.back ().first);
    ex.test(":");
    ex.read_word_or_quoted (key_bindings.back ().second);
  }

  return key_bindings;
}

std::string 
pack_key_binding (const std::vector<std::pair<std::string, std::string> > &unpacked)
{
  std::string packed;

  for (std::vector<std::pair<std::string, std::string> >::const_iterator p = unpacked.begin (); p != unpacked.end (); ++p) {
    if (! packed.empty ()) {
      packed += ";";
    }
    packed += tl::to_word_or_quoted_string (p->first);
    packed += ":";
    packed += tl::to_word_or_quoted_string (p->second);
  }

  return packed;
}

KeyBindingsConfigPage::KeyBindingsConfigPage (QWidget *parent)
  : lay::ConfigPage (parent), m_enable_event (true)
{
  mp_ui = new Ui::KeyBindingsConfigPage ();
  mp_ui->setupUi (this);
  connect (mp_ui->bindings_list, SIGNAL (currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT (current_changed (QTreeWidgetItem *, QTreeWidgetItem *)));
  connect (mp_ui->reset_pb, SIGNAL (clicked ()), this, SLOT (reset_clicked ()));

  mp_ui->binding_le->setEnabled (false);
}

KeyBindingsConfigPage::~KeyBindingsConfigPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

static void fill_paths (const lay::AbstractMenu &menu, const std::string &root, std::map<std::string, std::string> &bindings)
{
  std::vector<std::string> items = menu.items (root);
  for (std::vector<std::string>::const_iterator i = items.begin (); i != items.end (); ++i) {
    if (i->size () > 0) {
      if (menu.is_valid (*i) && menu.action (*i).is_visible ()) {
        if (menu.is_menu (*i)) {
          fill_paths (menu, *i, bindings);
        } else if (! menu.is_separator (*i)) {
          bindings.insert (std::make_pair (*i, menu.action (*i).get_shortcut ()));
        }
      }
    }
  }
}

std::vector<std::pair<std::string, std::string> > KeyBindingsConfigPage::m_default_bindings;

void
KeyBindingsConfigPage::set_default ()
{
  std::map<std::string, std::string> bm;
  fill_paths (*lay::MainWindow::instance ()->menu (), std::string (), bm);

  m_default_bindings.clear ();
  m_default_bindings.insert (m_default_bindings.begin (), bm.begin (), bm.end ());
}

void 
KeyBindingsConfigPage::reset_clicked ()
{
  if (QMessageBox::question (this, 
    QObject::tr ("Confirm Reset"),
    QObject::tr ("Are you sure to reset the key bindings?\nThis operation will clear all custom settings."),
    QMessageBox::Yes | QMessageBox::No,
    QMessageBox::No) == QMessageBox::Yes) { 
    apply (m_default_bindings);
  }
}

void 
KeyBindingsConfigPage::apply (const std::vector<std::pair<std::string, std::string> > &key_bindings)
{
  //  get the current bindings
  m_current_bindings.clear ();
  fill_paths (*lay::MainWindow::instance ()->menu (), std::string (), m_current_bindings);

  m_enable_event = false;

  //  overwrite with the given ones
  for (std::vector<std::pair<std::string, std::string> >::const_iterator kb = key_bindings.begin (); kb != key_bindings.end (); ++kb) {
    std::map<std::string, std::string>::iterator cb = m_current_bindings.find (kb->first);
    if (cb != m_current_bindings.end ()) {
      cb->second = kb->second;
    }
  }

  //  extract the top level menues
  std::map <std::string, std::string> top_level_menus;
  top_level_menus.insert (std::make_pair (std::string (), tl::to_string (QObject::tr ("Main Menu"))));
  top_level_menus.insert (std::make_pair (std::string ("lcp_context_menu"), tl::to_string (QObject::tr ("Layer Panel Context Menu"))));
  top_level_menus.insert (std::make_pair (std::string ("hcp_context_menu"), tl::to_string (QObject::tr ("Cell List Context Menu"))));
   
  //  fill the bindings list
  mp_ui->bindings_list->clear ();

  for (std::map <std::string, std::string>::const_iterator t = top_level_menus.begin (); t != top_level_menus.end (); ++t) {

    QTreeWidgetItem *top_level_item = new QTreeWidgetItem (mp_ui->bindings_list);
    top_level_item->setData (0, Qt::DisplayRole, tl::to_qstring (t->second));

    for (std::map<std::string, std::string>::const_iterator cb = m_current_bindings.begin (); cb != m_current_bindings.end (); ++cb) {

      std::string tl_menu;
      const std::string &path = cb->first;
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
        item->setData (0, Qt::DisplayRole, tl::to_qstring (rem_path));
        item->setData (1, Qt::DisplayRole, tl::to_qstring (lay::MainWindow::instance ()->menu ()->action (cb->first).get_title ()));
        item->setData (2, Qt::DisplayRole, tl::to_qstring (cb->second));
        item->setData (0, Qt::UserRole, tl::to_qstring (path));
      }
    }

    mp_ui->bindings_list->expandItem (top_level_item);

  }

  mp_ui->binding_le->setText (QString ());
  mp_ui->binding_le->setEnabled (false);

  m_enable_event = true;
}

void 
KeyBindingsConfigPage::setup (lay::PluginRoot *root)
{
  std::string packed_key_bindings;
  root->config_get (cfg_key_bindings, packed_key_bindings);
  std::vector<std::pair<std::string, std::string> > key_bindings = unpack_key_binding (packed_key_bindings);

  apply (key_bindings);
}

void 
KeyBindingsConfigPage::commit (lay::PluginRoot *root)
{
  current_changed (0, mp_ui->bindings_list->currentItem ());

  //  Because the available key bindings change in edit and viewer mode, we always extend the key bindings but never 
  //  reduce them.

  std::string packed_key_bindings;
  root->config_get (cfg_key_bindings, packed_key_bindings);
  std::vector<std::pair<std::string, std::string> > key_bindings = unpack_key_binding (packed_key_bindings);

  for (std::vector<std::pair<std::string, std::string> >::iterator kb = key_bindings.begin (); kb != key_bindings.end (); ++kb) {
    std::map<std::string, std::string>::iterator cb = m_current_bindings.find (kb->first);
    if (cb != m_current_bindings.end ()) {
      kb->second = cb->second;
      m_current_bindings.erase (cb);
    }
  }

  for (std::map<std::string, std::string>::const_iterator cb = m_current_bindings.begin (); cb != m_current_bindings.end (); ++cb) {
    key_bindings.push_back (*cb);
  }

  packed_key_bindings = pack_key_binding (key_bindings);
  root->config_set (cfg_key_bindings, packed_key_bindings);
}

void 
KeyBindingsConfigPage::current_changed (QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
  if (! m_enable_event) {
    return;
  }

  if (previous && mp_ui->binding_le->isEnabled ()) {

    QKeySequence key_sequence (mp_ui->binding_le->text ());
    previous->setData (2, Qt::DisplayRole, key_sequence.toString ());

    std::string path = tl::to_string (previous->data (0, Qt::UserRole).toString ());
    std::string shortcut = tl::to_string (previous->data (2, Qt::DisplayRole).toString ());

    m_current_bindings[path] = shortcut;

  }

  if (current && !current->data (0, Qt::UserRole).isNull ()) {
    mp_ui->binding_le->setText (current->data (2, Qt::DisplayRole).toString ());
    mp_ui->binding_le->setEnabled (true);
  } else {
    mp_ui->binding_le->setText (QString ());
    mp_ui->binding_le->setEnabled (false);
  }
}

}

