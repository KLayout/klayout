
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


#include "layStream.h"
#include "layTechSetupDialog.h"
#include "layTechnology.h"
#include "tlExceptions.h"
#include "layFileDialog.h"
#include "layTipDialog.h"
#include "layMainWindow.h"
#include "layApplication.h"
#include "layMacroEditorTree.h"
#include "layMacroController.h"
#include "layTechnologyController.h"
#include "layQtTools.h"
#include "lymMacro.h"
#include "tlAssert.h"
#include "tlStream.h"
#include "tlClassRegistry.h"
#include "dbStream.h"
#include "dbLibraryManager.h"
#include "dbLibrary.h"

#include "ui_TechSetupDialog.h"
#include "ui_TechMacrosPage.h"
#include "ui_TechComponentSetupDialog.h"
#include "ui_TechBaseEditorPage.h"
#include "ui_TechLayerMappingEditorPage.h"
#include "ui_TechLoadOptionsEditorPage.h"
#include "ui_TechSaveOptionsEditorPage.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QFileDialog>
#include <QScrollArea>
#include <QListWidgetItem>

#include <stdio.h>
#include <fstream>

namespace lay
{

// ----------------------------------------------------------------

static std::string
title_for_technology (const db::Technology *t)
{
  std::string d;
  if (t->name ().empty ()) {
    d = t->description ();
  } else {
    d += t->name ();
    if (! t->grain_name ().empty ()) {
      d += " ";
      d += tl::to_string (QObject::tr ("[Package %1]").arg (tl::to_qstring (t->grain_name ())));
    }
    if (! t->description ().empty ()) {
      d += " - ";
      d += t->description ();
    }
  }
  if (! t->group ().empty ()) {
    d += " [";
    d += t->group ();
    d += "]";
  }
  return d;
}

// ----------------------------------------------------------------
//  TechBaseEditorPage implementation

TechBaseEditorPage::TechBaseEditorPage (QWidget *parent)
  : TechnologyComponentEditor (parent)
{
  mp_ui = new Ui::TechBaseEditorPage ();
  mp_ui->setupUi (this);
  connect (mp_ui->browse_pb, SIGNAL (clicked ()), this, SLOT (browse_clicked ()));
  connect (mp_ui->browse_lyp_pb, SIGNAL (clicked ()), this, SLOT (browse_lyp_clicked ()));
}

TechBaseEditorPage::~TechBaseEditorPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
TechBaseEditorPage::setup ()
{
  mp_ui->name_le->setText (tl::to_qstring (tech ()->name ()));
  mp_ui->desc_le->setText (tl::to_qstring (tech ()->description ()));
  mp_ui->group_le->setText (tl::to_qstring (tech ()->group ()));
  mp_ui->dbu_le->setText (tl::to_qstring (tl::to_string (tech ()->dbu ())));
  mp_ui->grids_le->setText (tl::to_qstring (tl::to_string (tech ()->default_grids ())));
  mp_ui->desc_le->setEnabled (! tech ()->name ().empty ());
  mp_ui->base_path_le->setText (tl::to_qstring (tech ()->explicit_base_path ()));
#if QT_VERSION >= 0x040700
  mp_ui->base_path_le->setPlaceholderText (tl::to_qstring (tech ()->default_base_path ()));
#endif

  const std::string &lyp = tech ()->layer_properties_file ();
  mp_ui->lyp_grp->setChecked (! lyp.empty ());
  mp_ui->lyp_le->setText (tl::to_qstring (lyp));
  mp_ui->add_other_layers_cbx->setChecked (tech ()->add_other_layers ());

  mp_ui->libs_lw->clear ();

  if (! tech ()->name ().empty ()) {

    mp_ui->libs_lbl->setEnabled (true);
    mp_ui->libs_lw->setEnabled (true);

    std::vector<std::string> libs;

    for (db::LibraryManager::iterator l = db::LibraryManager::instance ().begin (); l != db::LibraryManager::instance ().end (); ++l) {
      const db::Library *lib = db::LibraryManager::instance ().lib (l->second);
      if (lib->is_for_technology (tech ()->name ())) {
        std::string text = lib->get_name ();
        if (! lib->get_description ().empty ()) {
          text += " - " + lib->get_description ();
        }
        libs.push_back (text);
      }
    }

    std::sort (libs.begin (), libs.end ());

    for (std::vector<std::string>::const_iterator l = libs.begin (); l != libs.end (); ++l) {
      mp_ui->libs_lw->addItem (new QListWidgetItem (tl::to_qstring (*l)));
    }

  } else {
    mp_ui->libs_lbl->setEnabled (false);
    mp_ui->libs_lw->setEnabled (false);
    mp_ui->libs_lw->addItem (tr ("The default technology can't have libraries"));
  }
}

void 
TechBaseEditorPage::commit ()
{
  tech ()->set_description (tl::to_string (mp_ui->desc_le->text ()));
  tech ()->set_group (tl::to_string (mp_ui->group_le->text ()));
  tech ()->set_explicit_base_path (tl::to_string (mp_ui->base_path_le->text ()));
  tech ()->set_default_grids (tl::to_string (mp_ui->grids_le->text ()));

  double d = 0.001;
  tl::from_string_ext (tl::to_string (mp_ui->dbu_le->text ()), d);
  tech ()->set_dbu (d);

  if (! mp_ui->lyp_grp->isChecked ()) {
    tech ()->set_layer_properties_file (std::string ());
  } else {
    tech ()->set_layer_properties_file (tl::to_string (mp_ui->lyp_le->text ()));
  }
  tech ()->set_add_other_layers (mp_ui->add_other_layers_cbx->isChecked ());
}

void 
TechBaseEditorPage::browse_clicked ()
{
  QString p = QFileDialog::getExistingDirectory (this, QObject::tr ("Choose Base Path"), 
                                                       mp_ui->base_path_le->text ());
  if (! p.isNull ()) {
    mp_ui->base_path_le->setText (p);
  }
}

void
TechBaseEditorPage::browse_lyp_clicked ()
{
  lay::FileDialog open_dialog (this,
                               tl::to_string (QObject::tr ("Browse Layer Properties File")),
                               tl::to_string (QObject::tr ("Layer properties files (*.lyp);;Text files (*.txt);;All files (*)")));

  std::string lyp = tech ()->base_path ();
  if (open_dialog.get_open (lyp)) {
    mp_ui->lyp_le->setText (tl::to_qstring (tech ()->correct_path (lyp)));
  }
}

// ----------------------------------------------------------------
//  TechLoadOptionsEditorPage implementation

TechLoadOptionsEditorPage::TechLoadOptionsEditorPage (QWidget *parent)
  : TechnologyComponentEditor (parent)
{
  mp_ui = new Ui::TechLoadOptionsEditorPage ();
  mp_ui->setupUi (this);

  while (mp_ui->options_tab->count () > 0) {
    mp_ui->options_tab->removeTab (0);
  }

  bool any_option = false;

  for (tl::Registrar<db::StreamFormatDeclaration>::iterator fmt = tl::Registrar<db::StreamFormatDeclaration>::begin (); fmt != tl::Registrar<db::StreamFormatDeclaration>::end (); ++fmt) {

    StreamReaderOptionsPage *page = 0;

    //  obtain the config page from the plugin which we identify by format name
    const StreamReaderPluginDeclaration *decl = StreamReaderPluginDeclaration::plugin_for_format (fmt->format_name ());
    if (decl) {
      QScrollArea *page_host = new QScrollArea (mp_ui->options_tab);
      page_host->setFrameStyle (QFrame::NoFrame);
      page_host->setWidgetResizable (true);
      page = decl->format_specific_options_page (mp_ui->options_tab);
      if (page) {
        page_host->setWidget (page);
        mp_ui->options_tab->addTab (page_host, tl::to_qstring (fmt->format_desc ()));
        any_option = true;
      } else {
        delete page_host;
      }
    }

#if 0
    //  Add dummy pages for empty options
    if (!page) {
      QLabel *empty = new QLabel (mp_ui->options_tab);
      empty->setAlignment (Qt::AlignCenter);
      empty->setText (QObject::tr ("No specific options available for this format"));
      mp_ui->options_tab->addTab (empty, tl::to_qstring (fmt->format_desc ()));
    }
#endif

    if (page) {
      m_pages.push_back (std::make_pair (page, fmt->format_name ()));
    }

  }

  if (! any_option) {
    mp_ui->options_tab->hide ();
  }
}

TechLoadOptionsEditorPage::~TechLoadOptionsEditorPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void
TechLoadOptionsEditorPage::setup ()
{
  for (std::vector< std::pair<StreamReaderOptionsPage *, std::string> >::iterator page = m_pages.begin (); page != m_pages.end (); ++page) {
    if (page->first) {
      page->first->setup (tech ()->load_layout_options ().get_options (page->second), tech ());
    }
  }
}

void
TechLoadOptionsEditorPage::commit ()
{
  //  create the particular options for all formats
  db::LoadLayoutOptions options = tech ()->load_layout_options ();
  for (std::vector< std::pair<StreamReaderOptionsPage *, std::string> >::iterator page = m_pages.begin (); page != m_pages.end (); ++page) {
    if (page->first) {
      db::FormatSpecificReaderOptions *specific_options = options.get_options (page->second);
      if (! specific_options) {
        specific_options = StreamReaderPluginDeclaration::plugin_for_format (page->second)->create_specific_options ();
        options.set_options (specific_options);
      }
      page->first->commit (specific_options, tech ());
    }
  }
  tech ()->set_load_layout_options (options);
}

// ----------------------------------------------------------------
//  TechSaveOptionsEditorPage implementation

TechSaveOptionsEditorPage::TechSaveOptionsEditorPage (QWidget *parent)
  : TechnologyComponentEditor (parent)
{
  mp_ui = new Ui::TechSaveOptionsEditorPage ();
  mp_ui->setupUi (this);

  while (mp_ui->options_tab->count () > 0) {
    mp_ui->options_tab->removeTab (0);
  }

  bool any_option = false;

  for (tl::Registrar<db::StreamFormatDeclaration>::iterator fmt = tl::Registrar<db::StreamFormatDeclaration>::begin (); fmt != tl::Registrar<db::StreamFormatDeclaration>::end (); ++fmt) {

    StreamWriterOptionsPage *page = 0;

    //  obtain the config page from the plugin which we identify by format name
    const StreamWriterPluginDeclaration *decl = StreamWriterPluginDeclaration::plugin_for_format (fmt->format_name ());
    if (decl) {
      QScrollArea *page_host = new QScrollArea (mp_ui->options_tab);
      page_host->setFrameStyle (QFrame::NoFrame);
      page_host->setWidgetResizable (true);
      page = decl->format_specific_options_page (mp_ui->options_tab);
      if (page) {
        page_host->setWidget (page);
        mp_ui->options_tab->addTab (page_host, tl::to_qstring (fmt->format_desc ()));
        any_option = true;
      } else {
        delete page_host;
      }
    }

#if 0
    //  Add dummy pages for empty options
    if (!page) {
      QLabel *empty = new QLabel (mp_ui->options_tab);
      empty->setAlignment (Qt::AlignCenter);
      empty->setText (QObject::tr ("No specific options available for this format"));
      mp_ui->options_tab->addTab (empty, tl::to_qstring (fmt->format_desc ()));
    }
#endif

    if (page) {
      m_pages.push_back (std::make_pair (page, fmt->format_name ()));
    }

  }

  if (! any_option) {
    mp_ui->options_tab->hide ();
  }
}

TechSaveOptionsEditorPage::~TechSaveOptionsEditorPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void
TechSaveOptionsEditorPage::setup ()
{
  for (std::vector< std::pair<StreamWriterOptionsPage *, std::string> >::iterator page = m_pages.begin (); page != m_pages.end (); ++page) {
    if (page->first) {
      const db::FormatSpecificWriterOptions *specific_options = tech ()->save_layout_options ().get_options (page->second);
      std::unique_ptr<db::FormatSpecificWriterOptions> default_options;
      if (! specific_options) {
        //  In case there is no option object yet, create a first one for initialization
        default_options.reset (StreamWriterPluginDeclaration::plugin_for_format (page->second)->create_specific_options ());
        specific_options = default_options.get ();
      }
      page->first->setup (specific_options, tech ());
    }
  }
}

void
TechSaveOptionsEditorPage::commit ()
{
  //  create the particular options for all formats
  db::SaveLayoutOptions options = tech ()->save_layout_options ();
  for (std::vector< std::pair<StreamWriterOptionsPage *, std::string> >::iterator page = m_pages.begin (); page != m_pages.end (); ++page) {
    if (page->first) {
      db::FormatSpecificWriterOptions *specific_options = options.get_options (page->second);
      if (! specific_options) {
        //  Create a container for the options unless there is one already
        specific_options = StreamWriterPluginDeclaration::plugin_for_format (page->second)->create_specific_options ();
        options.set_options (specific_options);
      }
      page->first->commit (specific_options, tech (), false /*gzip*/);
    }
  }
  tech ()->set_save_layout_options (options);
}

// ----------------------------------------------------------------
//  TechMacrosPage implementation

TechMacrosPage::TechMacrosPage (QWidget *parent, const std::string &cat, const std::string &cat_desc)
  : TechnologyComponentEditor (parent), m_cat (cat), m_cat_desc (cat_desc)
{
  mp_ui = new Ui::TechMacrosPage ();
  mp_ui->setupUi (this);

  m_original_labels.push_back (std::make_pair (mp_ui->title_label, mp_ui->title_label->text ()));
  m_original_labels.push_back (std::make_pair (mp_ui->note_label, mp_ui->note_label->text ()));
  m_original_labels.push_back (std::make_pair (mp_ui->empty_label1, mp_ui->empty_label1->text ()));
  m_original_labels.push_back (std::make_pair (mp_ui->empty_label2, mp_ui->empty_label2->text ()));
  m_original_labels.push_back (std::make_pair (mp_ui->empty_label3, mp_ui->empty_label3->text ()));

  mp_ui->folder_tree->header ()->hide ();
  connect (mp_ui->folder_tree, SIGNAL (clicked (const QModelIndex &)), this, SLOT (macro_selected (const QModelIndex &)));

  QFont f = mp_ui->macro_text->font ();
  f.setFixedPitch (true);
  f.setFamily (monospace_font ().family ());
  mp_ui->macro_text->setFont (f);

  connect (mp_ui->create_folder_button, SIGNAL (clicked ()), this, SLOT (create_folder_clicked ()));
}

TechMacrosPage::~TechMacrosPage ()
{
  //  do this before the collection gets deleted.
  delete mp_ui->folder_tree->model ();

  delete mp_ui;
  mp_ui = 0;
}

void 
TechMacrosPage::setup ()
{
  mp_ui->title_label->show ();
  mp_ui->macro_frame->show ();
  mp_ui->note_label->show ();
  mp_ui->empty_label1->hide ();
  mp_ui->empty_label3->hide ();
  mp_ui->empty_label2_frame->hide ();

  QDir base_dir (tl::to_qstring (tech ()->base_path ()));
  QDir macro_dir (base_dir.filePath (tl::to_qstring (m_cat)));
  QString cp = macro_dir.canonicalPath ();

  //  if a macro collection already exists, show a readonly copy of this one
  const lym::MacroCollection *original = 0;
  const lym::MacroCollection *root = &lym::MacroCollection::root ();
  for (lym::MacroCollection::const_child_iterator m = root->begin_children (); m != root->end_children () && ! original; ++m) {
    if (m->second->virtual_mode () == lym::MacroCollection::TechFolder && m->second->category () == m_cat && QDir (tl::to_qstring (m->second->path ())).canonicalPath () == cp) {
      original = m->second;
    }
  }

  const lym::MacroCollection *alt = 0;
  for (lym::MacroCollection::const_child_iterator m = root->begin_children (); m != root->end_children () && ! alt; ++m) {
    if (m->second->virtual_mode () != lym::MacroCollection::TechFolder && QDir (tl::to_qstring (m->second->path ())).canonicalPath () == cp) {
      alt = m->second;
    }
  }

  //  adjust labels
  for (std::vector<std::pair<QLabel *, QString> >::const_iterator ol = m_original_labels.begin (); ol != m_original_labels.end (); ++ol) {
    QString l = ol->second;
    l.replace (QString::fromUtf8 ("%CAT%"), tl::to_qstring (m_cat));
    l.replace (QString::fromUtf8 ("%CAT_DESC%"), tl::to_qstring (m_cat_desc));
    l.replace (QString::fromUtf8 ("%BASE_PATH%"), tl::to_qstring (tech ()->base_path ()));
    if (alt) {
      l.replace (QString::fromUtf8 ("%ALT_DESC%"), tl::to_qstring (alt->description ()));
    } else {
      l.replace (QString::fromUtf8 ("%ALT_DESC%"), QString::fromUtf8 ("*unknown*"));
    }
    ol->first->setText (l);
  }

  if (tech ()->base_path ().empty ()) {

    //  no base path set
    mp_ui->title_label->hide ();
    mp_ui->empty_label1->show ();
    mp_ui->macro_frame->hide ();
    mp_ui->note_label->hide ();

  } else {
    
    if (! macro_dir.exists ()) {

      //  macro folder not found
      mp_ui->title_label->hide ();
      mp_ui->empty_label2_frame->show ();
      mp_ui->macro_frame->hide ();
      mp_ui->note_label->hide ();

    } else {
      
      //  valid macros to show
      std::string mp = tl::to_string (macro_dir.path ());
      if (mp_collection.get () && m_current_path == mp) {
        //  .. nothing to do ..
      } else {

        if (! original && alt) {

          //  this can happen, if the macro collection is already there in a different context.
          //  Show a message indicating that
          mp_ui->title_label->hide ();
          mp_ui->empty_label3->show ();
          mp_ui->macro_frame->hide ();
          mp_ui->note_label->hide ();

        } else {

          std::string desc;
          if (original) {
            desc = original->description ();
          } else {
            desc = tl::to_string (QObject::tr ("Technology")) + " - " + tech ()->name ();
          }

          lym::MacroCollection *mc = new lym::MacroCollection ();
          mp_collection.reset (mc);
          mc->add_folder (desc, mp, m_cat, true);
          m_current_path = mp;

          delete mp_ui->folder_tree->model ();
          mp_ui->folder_tree->setModel (new lay::MacroTreeModel (this, mc, m_cat));
          mp_ui->folder_tree->expandAll ();
          mp_ui->macro_text->hide ();

        }

      }

    }

  }
}

void
TechMacrosPage::create_folder_clicked ()
{
BEGIN_PROTECTED
  QString macro_dir = QDir (tl::to_qstring (tech ()->base_path ())).absoluteFilePath (tl::to_qstring (m_cat));
  if (! QDir::root ().mkpath (macro_dir)) {
    throw tl::Exception (tl::to_string (QObject::tr ("Failed to create folder: ")) + tl::to_string (macro_dir));
  }
  setup ();
END_PROTECTED
}

void
TechMacrosPage::macro_selected (const QModelIndex &index)
{
  const lym::Macro *m = 0;
  lay::MacroTreeModel *model = dynamic_cast<lay::MacroTreeModel *> (mp_ui->folder_tree->model ());
  if (model && model->is_valid_pointer (index.internalPointer ())) {
    m = dynamic_cast <lym::Macro *> ((QObject *) index.internalPointer ());
  }

  if (! m) {
    mp_ui->macro_text->hide ();
  } else {
    mp_ui->macro_text->show ();
    mp_ui->macro_text->setPlainText (tl::to_qstring (m->text ()));
  }
}

void 
TechMacrosPage::commit ()
{
  // .. noting yet ..
}

// ----------------------------------------------------------------
//  TechSetupDialog implementation

static bool s_first_show = true;

TechSetupDialog::TechSetupDialog (QWidget *parent)
  : QDialog (parent), mp_current_tech (0), mp_current_editor (0), mp_current_tech_component (0), m_current_tech_changed_enabled (true)
{
  setObjectName (QString::fromUtf8 ("tech_setup_dialog"));

  mp_ui = new Ui::TechSetupDialog ();
  mp_ui->setupUi (this);

  QAction *add_action = new QAction (QObject::tr ("Add Technology"), this);
  connect (add_action, SIGNAL (triggered ()), this, SLOT (add_clicked ()));
  QAction *delete_action = new QAction (QObject::tr ("Delete Technology"), this);
  connect (delete_action, SIGNAL (triggered ()), this, SLOT (delete_clicked ()));
  QAction *rename_action = new QAction (QObject::tr ("Rename Technology"), this);
  connect (rename_action, SIGNAL (triggered ()), this, SLOT (rename_clicked ()));
  QAction *import_action = new QAction (QObject::tr ("Import Technology"), this);
  connect (import_action, SIGNAL (triggered ()), this, SLOT (import_clicked ()));
  QAction *export_action = new QAction (QObject::tr ("Export Technology"), this);
  connect (export_action, SIGNAL (triggered ()), this, SLOT (export_clicked ()));
  QAction *refresh_action = new QAction (QObject::tr ("Refresh"), this);
  connect (refresh_action, SIGNAL (triggered ()), this, SLOT (refresh_clicked ()));

  QAction *separator;

  mp_ui->tech_tree->addAction (add_action);
  mp_ui->tech_tree->addAction (delete_action);
  mp_ui->tech_tree->addAction (rename_action);
  separator = new QAction (this);
  separator->setSeparator (true);
  mp_ui->tech_tree->addAction (separator);
  mp_ui->tech_tree->addAction (import_action);
  mp_ui->tech_tree->addAction (export_action);
  separator = new QAction (this);
  separator->setSeparator (true);
  mp_ui->tech_tree->addAction (separator);
  mp_ui->tech_tree->addAction (refresh_action);

  mp_ui->tech_tree->header ()->hide ();
  connect (mp_ui->tech_tree, SIGNAL (currentItemChanged (QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT (current_tech_changed (QTreeWidgetItem *, QTreeWidgetItem *)));
  connect (mp_ui->add_pb, SIGNAL (clicked ()), this, SLOT (add_clicked ()));
  connect (mp_ui->delete_pb, SIGNAL (clicked ()), this, SLOT (delete_clicked ()));
  connect (mp_ui->rename_pb, SIGNAL (clicked ()), this, SLOT (rename_clicked ()));
}

TechSetupDialog::~TechSetupDialog ()
{
  clear_components ();

  delete mp_ui;
  mp_ui = 0;
}

void
TechSetupDialog::clear_components ()
{
  for (std::map <std::string, db::TechnologyComponent *>::iterator tc = m_technology_components.begin (); tc != m_technology_components.end (); ++tc) {
    delete tc->second;
  }
  m_technology_components.clear ();

  for (std::map <std::string, lay::TechnologyComponentEditor *>::iterator tce = m_component_editors.begin (); tce != m_component_editors.end (); ++tce) {
    mp_ui->tc_stack->removeWidget (tce->second);
    delete tce->second;
  }
  m_component_editors.clear ();

  mp_current_editor = 0;
  mp_current_tech_component = 0;
}

void
TechSetupDialog::refresh_clicked ()
{
  m_current_tech_changed_enabled = false;

BEGIN_PROTECTED

  commit_tech_component ();
  update_tech (0);

  std::string tech_name;
  if (selected_tech ()) {
    tech_name = selected_tech ()->name ();
  }

  //  Save the expanded state of the items
  std::set<std::string> expanded_techs;
  for (int i = 0; i < mp_ui->tech_tree->topLevelItemCount (); ++i) {
    QTreeWidgetItem *item = mp_ui->tech_tree->topLevelItem (i);
    if (item && item->isExpanded ()) {
      QVariant d = item->data (0, Qt::UserRole);
      if (d != QVariant ()) {
        expanded_techs.insert (tl::to_string (d.toString ()));
      }
    }
  }

  lay::TechnologyController::instance ()->rescan (m_technologies);

  update_tech_tree ();

  QTreeWidgetItem *new_item = 0;

  for (int i = 0; i < mp_ui->tech_tree->topLevelItemCount () && !new_item; ++i) {
    QTreeWidgetItem *item = mp_ui->tech_tree->topLevelItem (i);
    QVariant d = item->data (0, Qt::UserRole);
    if (d != QVariant () && tech_name == tl::to_string (d.toString ())) {
      new_item = item;
    }
  }

  mp_ui->tech_tree->setCurrentItem (new_item);

  //  restore the expanded state
  for (int i = 0; i < mp_ui->tech_tree->topLevelItemCount (); ++i) {
    QTreeWidgetItem *item = mp_ui->tech_tree->topLevelItem (i);
    QVariant d = item->data (0, Qt::UserRole);
    bool expand = (d != QVariant () && expanded_techs.find (tl::to_string (d.toString ())) != expanded_techs.end ());
    item->setExpanded (expand);
  }

  update_tech (selected_tech ());
  update_tech_component ();

END_PROTECTED

  m_current_tech_changed_enabled = true;
}

void
TechSetupDialog::update ()
{
  update_tech_tree ();
  mp_ui->tech_tree->setCurrentItem (mp_ui->tech_tree->topLevelItem (0));
  update_tech (selected_tech ());
}

int
TechSetupDialog::exec_dialog (db::Technologies &technologies)
{
  if (s_first_show) {
    TipDialog td (this,
                  tl::to_string (QObject::tr ("<html><body>To get started with the technology manager, read the documentation provided: <a href=\"int:/about/technology_manager.xml\">About Technology Management</a>.</body></html>")),
                  "tech-manager-basic-tips");
    td.exec_dialog ();
    s_first_show = false;
  }

  m_technologies = technologies;
  update ();

  mp_ui->tc_stack->setMinimumSize (mp_ui->tc_stack->sizeHint ());

  int ret = QDialog::exec ();
  if (ret) {
    technologies = m_technologies;
  }

  //  clean up
  update_tech (0);
  m_technologies = db::Technologies ();
  update_tech_tree ();

  return ret;
}

void 
TechSetupDialog::add_clicked ()
{
BEGIN_PROTECTED

  commit_tech_component ();

  db::Technology *t = selected_tech ();
  if (! t) {
    t = m_technologies.technology_by_name (std::string ());
    tl_assert (t != 0);
  }

  std::string d = t->get_display_string ();

  bool ok = false;
  QString tn = QInputDialog::getText (this, QObject::tr ("Add Technology"), 
                                      tl::to_qstring (tl::sprintf (tl::to_string (QObject::tr ("This will create a new technology based on the selected technology '%s'.\nChoose a name for the new technology.")), d)), 
                                      QLineEdit::Normal,
                                      QString (), 
                                      &ok);
  if (ok && !tn.isEmpty()) {

    tn = tn.simplified ();

    if (m_technologies.has_technology (tl::to_string (tn))) {
      throw tl::Exception (tl::to_string (QObject::tr ("A technology with this name already exists")));
    }

    QDir root = QDir (tl::to_qstring (lay::TechnologyController::instance ()->default_root ()));
    QDir tech_dir (root.filePath (tn));
    if (tech_dir.exists ()) {
      if (QMessageBox::question (this, QObject::tr ("Creating Technology"),
                                       QObject::tr ("A target folder with path '%1' already exists\nUse this directory for the new technology?").arg (tech_dir.path ()),
                                       QMessageBox::No | QMessageBox::Yes) == QMessageBox::No) {
        throw tl::CancelException ();
      }
    }

    db::Technology nt (*t);

    nt.set_tech_file_path (tl::to_string (tech_dir.absoluteFilePath (tn + QString::fromUtf8 (".lyt"))));
    nt.set_default_base_path (tl::to_string (tech_dir.absolutePath ()));
    nt.set_persisted (false);
    nt.set_name (tl::to_string (tn));
    nt.set_description (std::string ());
    m_technologies.add (nt);

    update_tech_tree ();
    select_tech (*m_technologies.technology_by_name (tl::to_string (tn)));

  }

END_PROTECTED
}

void 
TechSetupDialog::delete_clicked ()
{
BEGIN_PROTECTED

  db::Technology *t = selected_tech ();
  if (! t) {
    throw tl::Exception (tl::to_string (QObject::tr ("No technology selected")));
  }

  if (t->name ().empty ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("The default technology cannot be deleted")));
  }

  if (t->is_readonly ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("This technology is read-only and cannot be deleted")));
  }

  if (QMessageBox::question (this, QObject::tr ("Deleting Technology"),
                                   QObject::tr ("Are you sure to delete this technology?\nThis operation cannot be undone, except by cancelling the technology manager."),
                                   QMessageBox::No | QMessageBox::Yes) == QMessageBox::Yes) {

    for (db::Technologies::const_iterator i = m_technologies.begin (); i != m_technologies.end (); ++i) {

      if (i->name () == t->name ()) {

        m_technologies.remove (i->name ());

        update_tech_tree ();
        select_tech (*m_technologies.technology_by_name (std::string ()));

        break;

      }
    }

  }

END_PROTECTED
}

void 
TechSetupDialog::rename_clicked ()
{
BEGIN_PROTECTED

  commit_tech_component ();

  db::Technology *t = selected_tech ();
  if (! t) {
    throw tl::Exception (tl::to_string (QObject::tr ("No technology selected")));
  }

  if (t->name ().empty ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("The default technology cannot be renamed")));
  }

  if (t->is_readonly ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("This technology is read-only and cannot be renamed")));
  }

  bool ok = false;
  QString tn = QInputDialog::getText (this, QObject::tr ("Rename Technology"), 
                                      QObject::tr ("Choose a name for the technology"), 
                                      QLineEdit::Normal,
                                      tl::to_qstring (t->name ()), 
                                      &ok);
  if (ok && !tn.isEmpty()) {

    tn = tn.simplified ();

    if (m_technologies.has_technology (tl::to_string (tn))) {
      throw tl::Exception (tl::to_string (QObject::tr ("A technology with this name already exists")));
    }

    if (t->name () != tl::to_string (tn)) {

      t->set_name (tl::to_string (tn));

      if (! t->is_persisted () && ! t->tech_file_path().empty ()) {
        TipDialog td (this,
                      tl::to_string (QObject::tr ("<html><body>Renaming of a technology will neither rename the technology file or the folder the file is stored in.<br/>The file or folder needs to be renamed manually.</body></html>")),
                      "tech-manager-rename-tip");
        td.exec_dialog ();
      }

      update_tech_tree ();
      select_tech (*t);

    }

  }

END_PROTECTED
}

void 
TechSetupDialog::import_clicked ()
{
BEGIN_PROTECTED

  lay::FileDialog open_dialog (this, tl::to_string (QObject::tr ("Import Technology")), tl::to_string (QObject::tr ("KLayout technology files (*.lyt);;All files (*)")));

  std::string fn;
  if (open_dialog.get_open (fn)) {

    db::Technology t;
    t.load (fn);
    m_technologies.add (t);

    update_tech_tree ();
    select_tech (*m_technologies.technology_by_name (t.name ()));

  }

END_PROTECTED
}

void 
TechSetupDialog::export_clicked ()
{
BEGIN_PROTECTED

  db::Technology *t = selected_tech ();
  if (! t) {
    throw tl::Exception (tl::to_string (QObject::tr ("No technology selected")));
  }

  lay::FileDialog save_dialog (this, tl::to_string (QObject::tr ("Export Technology")), tl::to_string (QObject::tr ("KLayout technology files (*.lyt);;All files (*)")));

  std::string fn;
  if (save_dialog.get_save (fn)) {
    t->save (fn);
  }

END_PROTECTED
}

void
TechSetupDialog::update_tech_tree ()
{
  mp_ui->tech_tree->clear ();

  std::map <std::string, const db::Technology *> tech_by_name;
  for (db::Technologies::const_iterator t = m_technologies.begin (); t != m_technologies.end (); ++t) {
    tech_by_name.insert (std::make_pair (t->name (), &*t));
  }

  for (std::map <std::string, const db::Technology *>::const_iterator t = tech_by_name.begin (); t != tech_by_name.end (); ++t) {
    
    QFont f (mp_ui->tech_tree->font ());
    f.setItalic (t->second->is_readonly ());

    QTreeWidgetItem *ti = new QTreeWidgetItem (mp_ui->tech_tree);
    ti->setData (0, Qt::DisplayRole, QVariant (tl::to_qstring (title_for_technology (t->second))));
    ti->setData (0, Qt::UserRole, QVariant (tl::to_qstring (t->first)));
    ti->setData (0, Qt::FontRole, QVariant (f));
    if (! t->second->tech_file_path ().empty ()) {
      ti->setData (0, Qt::ToolTipRole, QVariant (tl::to_qstring (t->second->tech_file_path ())));
    }
    
    std::vector <std::string> tc_names = t->second->component_names ();
    std::map <std::string, const db::TechnologyComponent *> tc_by_name;
    for (std::vector <std::string>::const_iterator n = tc_names.begin (); n != tc_names.end (); ++n) {
      tc_by_name.insert (std::make_pair (*n, t->second->component_by_name (*n)));
    }

    QTreeWidgetItem *tci = new QTreeWidgetItem (ti);
    tci->setData (0, Qt::DisplayRole, QVariant (QObject::tr ("General")));
    tci->setData (0, Qt::UserRole + 1, QVariant (tl::to_qstring ("_general")));
    tci->setData (0, Qt::FontRole, QVariant (f));

    tci = new QTreeWidgetItem (ti);
    tci->setData (0, Qt::DisplayRole, QVariant (QObject::tr ("Reader Options")));
    tci->setData (0, Qt::UserRole + 1, QVariant (tl::to_qstring ("_load_options")));
    tci->setData (0, Qt::FontRole, QVariant (f));

    tci = new QTreeWidgetItem (ti);
    tci->setData (0, Qt::DisplayRole, QVariant (QObject::tr ("Writer Options")));
    tci->setData (0, Qt::UserRole + 1, QVariant (tl::to_qstring ("_save_options")));
    tci->setData (0, Qt::FontRole, QVariant (f));

    if (lay::MacroController::instance ()) {
      const std::vector<lay::MacroController::MacroCategory> &mc = lay::MacroController::instance ()->macro_categories ();
      for (std::vector<lay::MacroController::MacroCategory>::const_iterator c = mc.begin (); c != mc.end (); ++c) {
        tci = new QTreeWidgetItem (ti);
        tci->setData (0, Qt::DisplayRole, QVariant (tl::to_qstring (c->description)));
        tci->setData (0, Qt::UserRole + 1, QVariant (tl::to_qstring (std::string ("_macros_") + c->name)));
        tci->setData (0, Qt::FontRole, QVariant (f));
      }
    }

    for (std::map <std::string, const db::TechnologyComponent *>::const_iterator c = tc_by_name.begin (); c != tc_by_name.end (); ++c) {
      tci = new QTreeWidgetItem (ti);
      tci->setData (0, Qt::DisplayRole, QVariant (tl::to_qstring (c->second->description ())));
      tci->setData (0, Qt::UserRole + 1, QVariant (tl::to_qstring (c->first)));
      tci->setData (0, Qt::FontRole, QVariant (f));
    }

  }
}

void 
TechSetupDialog::update_tech (db::Technology *t)
{
  if (t == mp_current_tech) {
    return;
  }

  mp_current_tech = t;

  clear_components ();

  if (t) {

    lay::TechnologyComponentEditor *tce_widget = new TechBaseEditorPage (this);
    tce_widget->setEnabled (!t->is_readonly ());
    tce_widget->set_technology (t, 0);
    mp_ui->tc_stack->addWidget (tce_widget);
    m_component_editors.insert (std::make_pair (std::string ("_general"), tce_widget));

    if (lay::MacroController::instance ()) {
      const std::vector<lay::MacroController::MacroCategory> &mc = lay::MacroController::instance ()->macro_categories ();
      for (std::vector<lay::MacroController::MacroCategory>::const_iterator c = mc.begin (); c != mc.end (); ++c) {
        tce_widget = new TechMacrosPage (this, c->name, c->description);
        tce_widget->setEnabled (!t->is_readonly ());
        tce_widget->set_technology (t, 0);
        mp_ui->tc_stack->addWidget (tce_widget);
        m_component_editors.insert (std::make_pair (std::string ("_macros_") + c->name, tce_widget));
      }
    }

    tce_widget = new TechLoadOptionsEditorPage (this);
    tce_widget->setEnabled (!t->is_readonly ());
    tce_widget->set_technology (t, 0);
    mp_ui->tc_stack->addWidget (tce_widget);
    m_component_editors.insert (std::make_pair (std::string ("_load_options"), tce_widget));

    tce_widget = new TechSaveOptionsEditorPage (this);
    tce_widget->setEnabled (!t->is_readonly ());
    tce_widget->set_technology (t, 0);
    mp_ui->tc_stack->addWidget (tce_widget);
    m_component_editors.insert (std::make_pair (std::string ("_save_options"), tce_widget));

    std::vector <std::string> tc_names = t->component_names ();
    for (std::vector <std::string>::const_iterator n = tc_names.begin (); n != tc_names.end (); ++n) {

      db::TechnologyComponent *tc = t->component_by_name (*n)->clone ();
      m_technology_components.insert (std::make_pair (*n, tc));

      tce_widget = 0;
      for (tl::Registrar<lay::TechnologyEditorProvider>::iterator cls = tl::Registrar<lay::TechnologyEditorProvider>::begin (); cls != tl::Registrar<lay::TechnologyEditorProvider>::end () && ! tce_widget; ++cls) {
        if (cls.current_name () == tc->name ()) {
          tce_widget = cls->create_editor (this);
        }
      }

      if (tce_widget) {
        tce_widget->setEnabled (!t->is_readonly ());
        tce_widget->set_technology (t, tc);
        mp_ui->tc_stack->addWidget (tce_widget);
        m_component_editors.insert (std::make_pair (tc->name (), tce_widget));
      }

    }

  }
}

void 
TechSetupDialog::update_tech_component ()
{
  std::string tc_name = selected_tech_component_name ();
  std::map <std::string, lay::TechnologyComponentEditor *>::const_iterator tce = m_component_editors.find (tc_name);
  if (tce != m_component_editors.end ()) {

    std::map <std::string, db::TechnologyComponent *>::const_iterator tc = m_technology_components.find (tc_name);
    if (tc != m_technology_components.end ()) {
      mp_current_tech_component = tc->second;
    } else {
      mp_current_tech_component = 0;
    }

    mp_ui->tc_stack->setCurrentWidget (tce->second);
    mp_current_editor = tce->second;
    tce->second->setup ();

  } else {

    mp_ui->tc_stack->setCurrentIndex (0);
    mp_current_editor = 0;

  }
}

void
TechSetupDialog::select_tech (const db::Technology &tech)
{
  //  unselect the previous technology
  update_tech (0); 

  //  find the item for the new technology
  QTreeWidgetItem *item = 0;
  for (int i = mp_ui->tech_tree->topLevelItemCount (); i > 0; --i) {
    item = mp_ui->tech_tree->topLevelItem (i - 1);
    if (item->data (0, Qt::UserRole).toString () == tl::to_qstring (tech.name ())) {
      break;
    }
  }

  mp_ui->tech_tree->setCurrentItem (item);
  
  update_tech (selected_tech ());
  update_tech_component ();
}

void
TechSetupDialog::accept ()
{
BEGIN_PROTECTED
  commit_tech_component ();
  QDialog::accept ();
END_PROTECTED
}

void
TechSetupDialog::current_tech_changed (QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
  if (! m_current_tech_changed_enabled) {
    return;
  }

BEGIN_PROTECTED
  try {
    if (current) {
      commit_tech_component ();
      update_tech (selected_tech ());
      update_tech_component ();
    }
  } catch (...) {
    disconnect (mp_ui->tech_tree, SIGNAL (currentItemChanged (QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT (current_tech_changed (QTreeWidgetItem *, QTreeWidgetItem *)));
    // TODO: this leaves current selected - any way to unselect it?
    mp_ui->tech_tree->setCurrentItem (previous);
    connect (mp_ui->tech_tree, SIGNAL (currentItemChanged (QTreeWidgetItem *, QTreeWidgetItem *)), this, SLOT (current_tech_changed (QTreeWidgetItem *, QTreeWidgetItem *)));
    throw;
  }
END_PROTECTED
}

void
TechSetupDialog::commit_tech_component ()
{
  if (mp_current_editor) {
    mp_current_editor->commit ();
  }

  if (mp_current_tech && !mp_current_tech->is_readonly ()) {

    if (mp_current_tech_component) {
      mp_current_tech->set_component (mp_current_tech_component->clone ());
    }

    //  because commit may have changed the description text, update the technology titles
    for (int i = mp_ui->tech_tree->topLevelItemCount (); i > 0; --i) {

      QTreeWidgetItem *item = mp_ui->tech_tree->topLevelItem (i - 1);

      db::Technology *t = m_technologies.technology_by_name (tl::to_string (item->data (0, Qt::UserRole).toString ()));
      item->setData (0, Qt::DisplayRole, QVariant (tl::to_qstring (title_for_technology (t))));

    }

  }
}

std::string 
TechSetupDialog::selected_tech_component_name ()
{
  QTreeWidgetItem *item = mp_ui->tech_tree->currentItem ();
  if (item) {
    QVariant d = item->data (0, Qt::UserRole + 1);
    if (d != QVariant ()) {
      return tl::to_string (d.toString ());
    }
  }

  return std::string ();
}

db::Technology *
TechSetupDialog::selected_tech ()
{
  QTreeWidgetItem *item = mp_ui->tech_tree->currentItem ();
  while (item) {

    QVariant d = item->data (0, Qt::UserRole);
    if (d != QVariant ()) {
      std::string tn = tl::to_string (d.toString ());
      if (m_technologies.has_technology (tn)) {
        return m_technologies.technology_by_name (tn);
      }
    }

    //  try parent node.
    item = item->parent ();

  }

  return 0;
}

// ----------------------------------------------------------------
//  TechComponentSetupDialog implementation

TechComponentSetupDialog::TechComponentSetupDialog (QWidget *parent, db::Technology *tech, const std::string &component_name)
  : QDialog (parent),
    mp_tech (tech), mp_component (0), mp_editor (0)
{
  setObjectName (QString::fromUtf8 ("tech_component_setup_dialog"));

  mp_ui = new Ui::TechComponentSetupDialog ();
  mp_ui->setupUi (this);

  if (tech->name ().empty ()) {
    setWindowTitle (tl::to_qstring (tl::to_string (QObject::tr ("Edit Technology")) + " - " + tl::to_string (QObject::tr ("(Default)"))));
  } else {
    setWindowTitle (tl::to_qstring (tl::to_string (QObject::tr ("Edit Technology")) + " - " + tech->name ()));
  }

  const db::TechnologyComponent *component = tech->component_by_name (component_name);
  if (component) {

    mp_component = component->clone ();

    mp_editor = 0;
    for (tl::Registrar<lay::TechnologyEditorProvider>::iterator cls = tl::Registrar<lay::TechnologyEditorProvider>::begin (); cls != tl::Registrar<lay::TechnologyEditorProvider>::end () && ! mp_editor; ++cls) {
      if (cls.current_name () == mp_component->name ()) {
        mp_editor = cls->create_editor (mp_ui->content_frame);
      }
    }

    if (mp_editor) {

      QVBoxLayout *layout = new QVBoxLayout (mp_ui->content_frame);
      layout->addWidget (mp_editor);
      layout->setContentsMargins (0, 0, 0, 0);
      mp_ui->content_frame->setLayout (layout);

      mp_editor->set_technology (tech, mp_component);
      mp_editor->setup ();

    }
  }
}

TechComponentSetupDialog::~TechComponentSetupDialog ()
{
  delete mp_component;
  mp_component = 0;

  delete mp_ui;
  mp_ui = 0;
}

void 
TechComponentSetupDialog::accept ()
{
BEGIN_PROTECTED
  if (mp_editor && mp_tech && mp_component) {
    mp_editor->commit ();
    mp_tech->set_component (mp_component);
    mp_component = 0;
  }

  QDialog::accept ();
END_PROTECTED
}

}

