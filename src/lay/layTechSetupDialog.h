
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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

#include "ui_TechSetupDialog.h"
#include "ui_TechMacrosPage.h"
#include "ui_TechComponentSetupDialog.h"
#include "ui_TechBaseEditorPage.h"
#include "ui_TechLayerMappingEditorPage.h"
#include "ui_TechLoadOptionsEditorPage.h"
#include "ui_TechSaveOptionsEditorPage.h"

#include "layTechnology.h"
#include "layStream.h"
#include "layCommon.h"

#include <memory>

namespace lay
{

class TechnologyComponentEditor;
class Technology;
class MacroCollection;

class TechBaseEditorPage
  : public TechnologyComponentEditor,
    public Ui::TechBaseEditorPage
{
Q_OBJECT

public:
  TechBaseEditorPage (QWidget *parent);

  virtual void setup ();
  virtual void commit ();

private slots:
  void browse_clicked ();
  void browse_lyp_clicked ();
};

class TechMacrosPage
  : public TechnologyComponentEditor,
    public Ui::TechMacrosPage
{
Q_OBJECT

public:
  TechMacrosPage (QWidget *parent, const std::string &cat, const std::string &cat_desc);
  ~TechMacrosPage ();

  virtual void setup ();
  virtual void commit ();

private:
  std::string m_cat, m_cat_desc;
  std::vector<std::pair<QLabel *, QString> > m_original_labels;
  std::auto_ptr<lay::MacroCollection> mp_collection;
  std::string m_current_path;

private slots:
  void macro_selected (const QModelIndex &index);
  void create_folder_clicked ();
};

class TechLoadOptionsEditorPage
  : public TechnologyComponentEditor,
    public Ui::TechLoadOptionsEditorPage
{
Q_OBJECT

public:
  TechLoadOptionsEditorPage (QWidget *parent);

  virtual void setup ();
  virtual void commit ();

private:
  std::vector< std::pair<lay::StreamReaderOptionsPage *, std::string> > m_pages;
};

class TechSaveOptionsEditorPage
  : public TechnologyComponentEditor,
    public Ui::TechSaveOptionsEditorPage
{
Q_OBJECT

public:
  TechSaveOptionsEditorPage (QWidget *parent);

  virtual void setup ();
  virtual void commit ();

private:
  std::vector< std::pair<lay::StreamWriterOptionsPage *, std::string> > m_pages;
};

class LAY_PUBLIC TechSetupDialog
  : public QDialog,
    public Ui::TechSetupDialog
{
Q_OBJECT

public:
  TechSetupDialog (QWidget *parent);
  ~TechSetupDialog ();

  int exec ();

protected slots:
  void current_tech_changed (QTreeWidgetItem *current, QTreeWidgetItem *previous);
  void add_clicked ();
  void delete_clicked ();
  void rename_clicked ();
  void import_clicked ();
  void export_clicked ();

private:
  void update_tech_tree ();
  void update_tech (lay::Technology *tech);
  void update_tech_component ();
  void accept ();
  lay::Technology *selected_tech ();
  void select_tech (const lay::Technology &tech);
  std::string selected_tech_component_name ();
  void commit_tech_component ();
  void clear_components ();

  lay::Technologies m_technologies;
  lay::Technology *mp_current_tech;
  std::map <std::string, lay::TechnologyComponentEditor *> m_component_editors;
  std::map <std::string, lay::TechnologyComponent *> m_technology_components;
  lay::TechnologyComponentEditor *mp_current_editor;
  lay::TechnologyComponent *mp_current_tech_component;
};

class LAY_PUBLIC TechComponentSetupDialog
  : public QDialog,
    public Ui::TechComponentSetupDialog
{
public:
  TechComponentSetupDialog (QWidget *parent, Technology *tech, const std::string &component_name);
  ~TechComponentSetupDialog ();

protected:
  void accept ();

private:
  Technology *mp_tech;
  TechnologyComponent *mp_component;
  TechnologyComponentEditor *mp_editor;
};

}

#endif


