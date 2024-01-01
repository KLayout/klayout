
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


#ifndef HDR_layMacroEditorSetupPage
#define HDR_layMacroEditorSetupPage

#include "layPlugin.h"
#include "layPluginConfigPage.h"
#include "ui_MacroEditorSetupPage.h"

namespace lay
{

struct MacroEditorSetupDialogData;

/**
 *  @brief The dialog for editing the properties of the debugger/editor
 */
class MacroEditorSetupPage
  : public lay::ConfigPage, private Ui::MacroEditorSetupPage
{
Q_OBJECT

public:
  MacroEditorSetupPage (QWidget *parent);
  ~MacroEditorSetupPage ();

  virtual void setup (Dispatcher *root);
  virtual void commit (Dispatcher *root);

protected slots:
  void current_attribute_changed (QListWidgetItem *current, QListWidgetItem *previous);
  void cb_changed (int n);
  void color_changed (QColor c);
  void update_font ();
  void clear_exception_list ();

private:
  void commit_attributes (QListWidgetItem *to_item);
  void update_attributes (QListWidgetItem *from_item);
  void update_ignore_exception_list ();

  MacroEditorSetupDialogData *mp_data;
};

}

#endif

