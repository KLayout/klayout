
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


#ifndef HDR_layMacroEditorSetupDialog
#define HDR_layMacroEditorSetupDialog

#include "layMacro.h"
#include "layGenericSyntaxHighlighter.h"
#include "ui_MacroEditorSetupDialog.h"

#include <QDialog>

#include <vector>

namespace lay
{

struct MacroEditorSetupDialogData
{
  MacroEditorSetupDialogData ()
    : basic_attributes (0), tab_width (8), indent (2), save_all_on_run (true), stop_on_exception (true), file_watcher_enabled (true), font_size (0)
  {
  }

  GenericSyntaxHighlighterAttributes basic_attributes;
  std::vector <std::pair <std::string, GenericSyntaxHighlighterAttributes> > specific_attributes;
  int tab_width;
  int indent;
  bool save_all_on_run;
  bool stop_on_exception;
  bool file_watcher_enabled;
  std::string font_family;
  int font_size;
};

/**
 *  @brief The dialog for editing the properties of the debugger/editor
 */
class MacroEditorSetupDialog
  : public QDialog, private Ui::MacroEditorSetupDialog
{
Q_OBJECT

public:
  MacroEditorSetupDialog (QWidget *parent);

  int exec_dialog (MacroEditorSetupDialogData &data);

protected slots:
  void current_attribute_changed (QListWidgetItem *current, QListWidgetItem *previous);
  void cb_changed (int n);
  void color_changed (QColor c);
  void update_font ();

private:
  void commit_attributes (QListWidgetItem *to_item);
  void update_attributes (QListWidgetItem *from_item);

  MacroEditorSetupDialogData *mp_data;
};

}

#endif

