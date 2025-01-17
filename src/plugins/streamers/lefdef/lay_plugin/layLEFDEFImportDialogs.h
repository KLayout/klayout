
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


#ifndef HDR_dbLEFImportDialog
#define HDR_dbLEFImportDialog

#include "layPluginCommon.h"
#include "layTechnology.h"
#include "layStream.h"

#include "ui_LEFDEFImportOptionsDialog.h"
#include "ui_LEFDEFTechnologyComponentEditor.h"

#include <QDialog>

namespace lay
{

/**
 *  @brief A structure containing the LEF importer data
 */
struct LAY_PLUGIN_PUBLIC LEFDEFImportData
{
  LEFDEFImportData ();

  void from_string (const std::string &s);
  std::string to_string () const;

  int mode;
  std::string file;
  std::vector<std::string> lef_files;
};

/**
 *  @brief The LEF importer dialog
 */
class LAY_PLUGIN_PUBLIC LEFDEFImportOptionsDialog
  : public QDialog,
    private Ui::LEFDEFImportOptionsDialog
{
Q_OBJECT

public:
  LEFDEFImportOptionsDialog (QWidget *parent, bool is_lef_dialog);

  int exec_dialog (LEFDEFImportData &data);

private slots:
  void browse_button_clicked ();
  void tech_setup_button_clicked ();
  void add_lef_file_clicked ();
  void del_lef_files_clicked ();
  void move_lef_files_up_clicked ();
  void move_lef_files_down_clicked ();

private:
  bool m_is_lef_dialog;
};

/**
 *  @brief The LEF reader options editor
 */
class LEFDEFReaderOptionsEditor
  : public lay::StreamReaderOptionsPage,
    public Ui::LEFDEFTechnologyComponentEditor
{
Q_OBJECT

public:
  LEFDEFReaderOptionsEditor (QWidget *parent);

  void commit (db::FormatSpecificReaderOptions *options, const db::Technology *tech);
  void setup (const db::FormatSpecificReaderOptions *options, const db::Technology *tech);

private slots:
  void checkbox_changed ();
  void add_lef_file_clicked ();
  void del_lef_files_clicked ();
  void move_lef_files_up_clicked ();
  void move_lef_files_down_clicked ();
  void add_macro_layout_file_clicked ();
  void del_macro_layout_files_clicked ();
  void move_macro_layout_files_up_clicked ();
  void move_macro_layout_files_down_clicked ();
  void browse_mapfile_clicked ();

private:
  tl::weak_ptr<db::Technology> mp_tech;

  static void add_files (QListWidget *list, const QStringList &files, const db::Technology *tech);
  static void del_files (QListWidget *list);
  static void move_files_up (QListWidget *list);
  static void move_files_down (QListWidget *list);
};

}

#endif

