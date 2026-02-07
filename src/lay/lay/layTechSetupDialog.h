
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


#ifndef HDR_layTechSetupDialog
#define HDR_layTechSetupDialog

#include "layTechnology.h"
#include "layStream.h"
#include "layCommon.h"

#include <memory>

#include <QDialog>

namespace db
{
  class Technology;
}

namespace lym
{
  class Macro;
  class MacroCollection;
}

namespace Ui
{
  class TechBaseEditorPage;
  class TechComponentSetupDialog;
  class TechSetupDialog;
  class TechSaveOptionsEditorPage;
  class TechLoadOptionsEditorPage;
  class TechMacrosPage;
}

class QLabel;
class QModelIndex;
class QTreeWidgetItem;

namespace lay
{

class TechnologyComponentEditor;

class TechBaseEditorPage
  : public TechnologyComponentEditor
{
Q_OBJECT

public:
  TechBaseEditorPage (QWidget *parent);
  ~TechBaseEditorPage ();

  virtual void setup ();
  virtual void commit ();

private slots:
  void browse_clicked ();
  void browse_lyp_clicked ();

private:
  Ui::TechBaseEditorPage *mp_ui;
};

class TechMacrosPage
  : public TechnologyComponentEditor
{
Q_OBJECT

public:
  TechMacrosPage (QWidget *parent, const std::string &cat, const std::string &cat_desc);
  ~TechMacrosPage ();

  virtual void setup ();
  virtual void commit ();

private:
  Ui::TechMacrosPage *mp_ui;
  std::string m_cat, m_cat_desc;
  std::vector<std::pair<QLabel *, QString> > m_original_labels;
  std::unique_ptr<lym::MacroCollection> mp_collection;
  std::string m_current_path;

private slots:
  void macro_selected (const QModelIndex &index);
  void create_folder_clicked ();
};

class TechLoadOptionsEditorPage
  : public TechnologyComponentEditor
{
Q_OBJECT

public:
  TechLoadOptionsEditorPage (QWidget *parent);
  ~TechLoadOptionsEditorPage ();

  virtual void setup ();
  virtual void commit ();

private:
  Ui::TechLoadOptionsEditorPage *mp_ui;
  std::vector< std::pair<lay::StreamReaderOptionsPage *, std::string> > m_pages;
};

class TechSaveOptionsEditorPage
  : public TechnologyComponentEditor
{
Q_OBJECT

public:
  TechSaveOptionsEditorPage (QWidget *parent);
  ~TechSaveOptionsEditorPage ();

  virtual void setup ();
  virtual void commit ();

private:
  Ui::TechSaveOptionsEditorPage *mp_ui;
  std::vector< std::pair<lay::StreamWriterOptionsPage *, std::string> > m_pages;
};

class LAY_PUBLIC TechSetupDialog
  : public QDialog
{
Q_OBJECT

public:
  TechSetupDialog (QWidget *parent);
  ~TechSetupDialog ();

  int exec_dialog (db::Technologies &technologies);

protected slots:
  void current_tech_changed (QTreeWidgetItem *current, QTreeWidgetItem *previous);
  void add_clicked ();
  void delete_clicked ();
  void rename_clicked ();
  void import_clicked ();
  void export_clicked ();
  void refresh_clicked ();

private:
  void update_tech_tree ();
  void update_tech (db::Technology *tech);
  void update_tech_component ();
  void accept ();
  db::Technology *selected_tech ();
  void select_tech (const db::Technology &tech);
  std::string selected_tech_component_name ();
  void commit_tech_component ();
  void clear_components ();
  void update ();

  Ui::TechSetupDialog *mp_ui;
  db::Technologies m_technologies;
  db::Technology *mp_current_tech;
  std::map <std::string, lay::TechnologyComponentEditor *> m_component_editors;
  std::map <std::string, db::TechnologyComponent *> m_technology_components;
  lay::TechnologyComponentEditor *mp_current_editor;
  db::TechnologyComponent *mp_current_tech_component;
  bool m_current_tech_changed_enabled;
};

class LAY_PUBLIC TechComponentSetupDialog
  : public QDialog
{
public:
  TechComponentSetupDialog (QWidget *parent, db::Technology *tech, const std::string &component_name);
  ~TechComponentSetupDialog ();

protected:
  void accept ();

private:
  Ui::TechComponentSetupDialog *mp_ui;
  db::Technology *mp_tech;
  db::TechnologyComponent *mp_component;
  lay::TechnologyComponentEditor *mp_editor;
};

}

#endif


